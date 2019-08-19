/* midiwriter.c
 * 2019 Brendan Meath
 */

#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>
#include <string.h>
#include <sys/types.h>

#include "midi.h"
#include "note.h"
#include "midiwriter.h"

// returns 0 on success, 1 if there was an error
int gen_midi_file(
    const char *fname,
    note_t *notes,
    unsigned int notecount,
    unsigned int ppq)
{
    if (!fname || !notes)
    {
        return 1;
    }

    // pulses per quarter note (aka pulses per crotchet, ticks per crotchet...)
    if (ppq == 0)
    {
        ppq = MIDI_PPQ_DEFAULT;
    }

    int bpm;

    // for converting time from seconds to ticks (MIDI sequencer clock cycles)
    unsigned int ticks_per_sec;
    // the shortest duration note to support (aka maximum quantisation
    //const unsigned int shortest_note_ticks = ppq / DIV_SEMIQUAVER;

    const unsigned int ntracks = 1;
    const unsigned int channel = 0;     // MIDI track channel (0-15)

    // for temporary storage of event offset times in ticks
    unsigned long int start_delta, stop_delta;
    // total time in ticks
    unsigned long int total_ticks;
    /* set to 1 if a note with a different tempo is found, thus requiring a
     * a midi tempo event to be added */
    int tempo_change;

    midiwriter_t writer;    // contains data needed to generate the MIDI file
    midifile_t midif;       // the generated MIDI file

    // open output file
    writer.fname = strdup(fname);
    writer.fd = creat(writer.fname, 0664);
    if (writer.fd < 0)
    {
        perror(__func__);
        return 1;
    }

    /* initialise a MIDI file structure.
     * 0 indicates we want default configuration for that option
     */
    if (init_midifile(&midif, 0,  ntracks, ppq, 0) != 0)
    {
        return 1;
    }

    /* Begin adding the notes as MIDI events */

    // Set a default tempo. To be used until a note with a known tempo is found.
    bpm = MIDI_BPM_DEFAULT;
    ticks_per_sec = (ppq * bpm) / 60;
    tempo_change = 0;
    total_ticks = 0;
    for (int i = 0; i < notecount; i++)
    {
        // update the tempo to that (if known) of the current note.
        if (notes[i].tempo != bpm && notes[i].tempo > 0)
        {
            bpm = notes[i].tempo;
            ticks_per_sec = (ppq * bpm) / 60;
            tempo_change = 1;
        }

        // convert the time from seconds to ticks offset from previous event
        start_delta = (ticks_per_sec * notes[i].start_sec) - total_ticks;

        // update the total time elapsed in ticks
        total_ticks += start_delta;

        if (tempo_change)
        {
            if (miditrack_settempo(&midif.tracks[0], start_delta, bpm) != 0)
            {
                return 1;
            }

            // unset the tempo change flag
            tempo_change = 0;

            /* set to zero so the offset is not doubled. (next note should
             * occur at the same time as the tempo change)
             */
            start_delta = 0;
        }

        // add a 'note begin' event to the MIDI track
        if (miditrack_noteon(&midif.tracks[0], start_delta, channel,
            notes[i].pitch, notes[i].velocity) != 0)
        {
            fprintf(stderr, "Error adding note begin event:\n"
                            "  pitch: %u\n"
                            "  start time: %f sec\n"
                            "  start delta: %lu ticks\n"
                            "  velocity: %u\n",
                            notes[i].pitch,
                            notes[i].start_sec,
                            start_delta,
                            notes[i].velocity
            );
            return 1;
        }

        // convert as before.
        stop_delta = (ticks_per_sec * notes[i].stop_sec) - total_ticks;

        // again, update the total time elapsed
        total_ticks += stop_delta;

        // add a 'note end' event to the MIDI track
        if (miditrack_noteoff(&midif.tracks[0], stop_delta, channel,
            notes[i].pitch, notes[i].velocity) != 0)
        {
            fprintf(stderr, "Error adding note end event:\n"
                            "  pitch: %u\n"
                            "  stop time: %f sec\n"
                            "  stop delta: %lu ticks\n"
                            "  velocity: %u\n",
                            notes[i].pitch,
                            notes[i].stop_sec,
                            stop_delta,
                            notes[i].velocity
            );
            return 1;
        }
    }

    // we are finished editing our midi file structure
    if (finalise_midifile(&midif) != 0)
    {
        fprintf(stderr, "Error finalising MIDI file\n");
        return 1;
    }

    // write MIDI file to disk
    if (write_midifile(&midif, writer.fd) != 0)
    {
        fprintf(stderr, "Error writing MIDI file to disk\n");
        return 1;
    }

    if (close(writer.fd) != 0)
    {
        fprintf(stderr, "Error closing MIDI file\n");
        return 1;
    }

    print_midiheader(&midif, stderr);

    // free dynamically allocated memory
    free_midifile(&midif);

    return 0;
}

/* (no longer used)
 * Parser for output of aubionotes command
 *
 * Returns the number of notes parsed from command output,
 * or -1 if there was an error.
 *
 * For an input string containing n lines:
 *           line 1 = <the time at which the silence ended>
 *   lines 2 to n-1 = <MIDI pitch> <note start time> <note end time>
 *           line n = <the time at which the source audio file ended>
 *
 * All times are expressed in seconds.
 * All times are the offset from the beginning of the source audio.
 *
 */
int parse_aubionotes(char *input, note_t **notes)
{
    // pointers for traversing the input
    char *s, *p;
    // number of notes contained in the input
    size_t notecount;

    if (!notes)
    {
        return -1;
    }
    if (!input)
    {
        *notes = NULL;
        return -1;
    }

    s = p = input;

    // go to the second line, which contains the first note in the input
    while(*p && *(p++) != '\n');
    s = p;

    // count number of notes in the input
    notecount = 0;
    while (*p)
    {
        if (*(p++) == '\n')
        {
            notecount++;
        }
    }
    // disregard the last line and any trailing newlines
    while (*(--p) == '\n')
    {
        notecount--;
    }
    // allocate enough memory for all the notes
    *notes = malloc(notecount * sizeof(note_t));
    if (!*notes)
    {
        return -1;
    }

    // parse all the notes
    for (int i = 0; i < notecount; i++)
    {
        /* parse the pitch value */
        (*notes)[i].pitch = (unsigned int) strtod(s, &p);
        s = p;
        if ((*notes)[i].pitch < MIDI_PITCH_MIN ||
            (*notes)[i].pitch > MIDI_PITCH_MAX)
        {
            free(*notes);
            return -1;
        }

        /* parse the start time in ms (offset from start of music) */
        (*notes)[i].start_sec = strtod(s, &p);

        s = p;
        if ((*notes)[i].start_sec < 0)
        {
            free(*notes);
            return -1;
        }

        /* parse the start time in ms (offset from start of music) */
        (*notes)[i].stop_sec = strtod(s, &p);

        s = p;
        if ((*notes)[i].stop_sec < 0)
        {
            free(*notes);
            return -1;
        }

        // go to next line
        while(*p && *(p++) != '\n');
        s = p;
    }

    return notecount;
}
