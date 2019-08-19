/* midiwriter.h 
 * 2019 Brendan Meath
 */

#ifndef MIDIWRITER_H
#define MIDIWRITER_H

#if defined(__cplusplus)
extern "C" {
#endif

/* for use when setting the shortest note duration to support,
 * aka max quantisation
 */
#define DIV_CROTCHET        1
#define DIV_QUAVER          (DIV_CROTCHET * 2)
#define DIV_SEMIQUAVER      (DIV_QUAVER * 2)
#define DIV_DEMISEMIQUAVER  (DIV_SEMIQUAVER * 2)

typedef struct midiwriter
{
    char            *fname;     // destination file path
    int             fd;         // destination file descriptor

    note_t          *notes;     // the notes that the music contains
    unsigned int    notecount;  // number of elements in notes buffer
} midiwriter_t;

/* method which creates a MIDI file according to the input.
 * 
 * parameters:
 *   notes:     buffer of musical notes.
 *   notecount: number of notes in buffer.
 *   tempo:     tempo of the music, in crotchet beats per minute (BPM).
 *   fname:     file to write MIDI data to
 *
 * returns: 0 on success, 1 otherwise
 */
int gen_midi_file(
    const char *fname,
    note_t *notes,
    unsigned int notecount,
    unsigned int ppq
);

/* Parser for output of aubionotes command */
int parse_aubionotes(char *input, note_t **notes);

#if defined(__cplusplus)
}
#endif

#endif
