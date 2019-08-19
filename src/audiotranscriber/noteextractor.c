/* noteextractor.c
 * 2019 Brendan Meath
 */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>

#include <math.h>

#include "memory.h"
#include "noteextractor.h"

static unsigned int roundm(unsigned int unrounded, unsigned int multiple);

/* returns the input number rounded to the nearest multiple of n */
static unsigned int roundm(unsigned int unrounded, unsigned int multiple)
{
    return ((unrounded + multiple/2) / multiple) * multiple;
}

int extract_notes(
    aubio_source_t *source,
    unsigned int winsize,
    unsigned int hopsize,
    unsigned int bpm,
    note_t **notes
)
{
    if (!source || !notes)
    {
        return -1;
    }

    // object holding context needed by aubio note functions
    aubio_notes_t *notes_ctx;
    smpl_t onset_minioi = 0.0;
    smpl_t silence_threshold = -90.;
    smpl_t release_drop = 10.;

    // object holding context needed by aubio tempo functions
    aubio_tempo_t *tempo_ctx;

    unsigned int samplerate;

    fvec_t *ibuf, *obuf_notes, *obuf_tempo;
    int blocks;
	unsigned int nframes;

    size_t          notecount, notes_max; // for use with *notes
    const size_t    notes_incr  = 1000; // number of elements to add when full

    // for calculating average tempo across the duration of a note
    int note_present; // is set upon detection of a note
    double tempo_thisblock;
    double tempo_sum; // the sum of all detected tempos during a note's lifespan
    unsigned long int tempo_count = 0; // number of tempos contained in sum
    const unsigned int tempo_accuracy = 5;

    // sanity check input arguments
    if (hopsize > winsize)
    {
        fprintf(stderr, "Error: hop size cannot be larger than window size\n");
        return 1;
    }
    if (hopsize < 1)
    {
        fprintf(stderr, "Error: hop size cannot be less than 1 sample\n");
        return 1;
    }
    if (winsize < 2)
    {
        fprintf(stderr, "Error: window size cannot be less than 2 samples\n");
        return 1;
    }
    if (bpm < 20 && bpm != 0) // exclude 0 which means to detect bpm from source
    {
        fprintf(stderr, "Error: tempo cannot be less than 20 bpm\n");
        return 1;
    }
    if (bpm > 500)
    {
        fprintf(stderr, "Error: tempo cannot be more than 500 bpm\n");
        return 1;
    }

    samplerate = aubio_source_get_samplerate(source);

    /* set up notes object */
    notes_ctx = new_aubio_notes("default", winsize, hopsize, samplerate);
	if (notes_ctx == NULL)
	{
	    return 1;
    }

	if (onset_minioi != 0.)
	{
		aubio_notes_set_minioi_ms(notes_ctx, onset_minioi);
	}

	if (aubio_notes_set_silence (notes_ctx, silence_threshold) != 0)
	{
		fprintf(stderr, "Error: could not set silence threshold to %.2f\n",
			silence_threshold
		);
	}

	if (aubio_notes_set_release_drop (notes_ctx, release_drop) != 0)
	{
		fprintf(stderr, "Error: could not set release drop to %.2f\n",
			release_drop
		);
	}

    /* set up tempo object */
    tempo_ctx = new_aubio_tempo("default", winsize, hopsize, samplerate);
    if (tempo_ctx == NULL)
	{
	    del_aubio_notes(notes_ctx);
	    return 1;
    }

    /* allocate memory for extracted musical features */
    notes_max = 2000;
    *notes = (note_t *) malloc(notes_max * sizeof(note_t));
    if (!*notes)
    {
        del_aubio_tempo(tempo_ctx);
        del_aubio_notes(notes_ctx);
        return -1;
    }

    /* allocate buffers for input/output of aubio library functions */
    ibuf = new_fvec(hopsize);
    obuf_notes = new_fvec(hopsize);
    obuf_tempo = new_fvec(1);

    /* process the input audio, extracting pitch, onset and tempo */
    nframes = 0;
	blocks = 0;

	note_present = 0;
    tempo_sum = 0;
    tempo_count = 0;

    notecount = 0;
	do
	{
	    // read in audio samples
		aubio_source_do (source, ibuf, &nframes);

        // extract pitch and onset information from audio samples
		aubio_notes_do (notes_ctx, ibuf, obuf_notes);

        // extract tempo information from audio samples
		aubio_tempo_do (tempo_ctx, ibuf, obuf_tempo);

        // if we have detected the end of a note
        if (obuf_notes->data[2] != 0 && note_present)
        {
	        (*notes)[notecount].stop_sec = (blocks * hopsize) / (float) samplerate;

            // if the caller wants us to detect tempo and we were able to do so
            if (bpm == 0 && tempo_count > 0)
            {
                // store average of all detected tempos during the note lifespan
                (*notes)[notecount].tempo = (unsigned int) round(tempo_sum / tempo_count);
                // round tempo number to nearest multiple of 5
                (*notes)[notecount].tempo = (unsigned int) roundm((*notes)[notecount].tempo, tempo_accuracy);
            }
            else
            {
                /* tempo could not be ascertained, or the caller has provided us
                 * with the tempo
                 */
                (*notes)[notecount].tempo = bpm;
            }

            // we have completed a note, therefore increment the note index
	        notecount++;
	        // set to zero to indicate that the next note is as of yet undefined
	        (*notes)[notecount].pitch = 0;

            note_present = 0;
        }

        // if we have detected the start of a note
	    if (obuf_notes->data[0] != 0)
	    {
	        // if there is already an ongoing note, end it before adding this one
	        if (note_present)
	        {
	            (*notes)[notecount].stop_sec = (blocks * hopsize) / (float) samplerate;

                // if the caller wants us to detect tempo and we were able to do so
                if (bpm == 0 && tempo_count > 0)
                {
                    // store average of all detected tempos during the note lifespan
                    (*notes)[notecount].tempo = (unsigned int) round(tempo_sum / tempo_count);
                    // round tempo number to nearest multiple of 5
                    (*notes)[notecount].tempo = (unsigned int) roundm((*notes)[notecount].tempo, tempo_accuracy);

                    // we have completed a note, therefore increment the note index
	                notecount++;
	                // set to zero to indicate that the next note is as of yet undefined
	                (*notes)[notecount].pitch = 0;
                }
                else
                {
                    // tempo could not be ascertained, or the caller has provided us with the tempo instead
                    (*notes)[notecount].tempo = bpm;
                }

                // we have completed a note, therefore increment the note index
	            notecount++;
	        }

	        (*notes)[notecount].start_sec = (blocks * hopsize) / (float) samplerate;

            // reset tempo tracking variables, as a new note has begun
            note_present = 1;
	        tempo_sum = 0;
	        tempo_count = 0;

		    (*notes)[notecount].pitch = (unsigned int) obuf_notes->data[0];
		    (*notes)[notecount].velocity = obuf_notes->data[1];
	    }

        /* If there is an ongoing note, and the caller wants us to detect tempo,
         * estimate the tempo.
         */
        if (note_present == 1 && bpm == 0)
        {
            tempo_thisblock = aubio_tempo_get_bpm(tempo_ctx);
            if ( tempo_thisblock >= 0)
            {
                tempo_sum += tempo_thisblock;
                tempo_count++;
            }
        }

		blocks++;

        // resize the notes buffer to accommodate more notes
		if (notecount == notes_max)
		{
		    notes_max += notes_incr;
            *notes = realloc_or_free(*notes, notes_max);
		    if (*notes == NULL)
            {
                notecount = -1;
                break;
            }
        }
	} while (nframes == hopsize);

    if (!bpm)
    {
        // set tempo to the most frequently occurring tempo in the music
        bpm = get_modal_tempo(*notes, notecount);
    }

    for (int i = 0; i < notecount; i++)
    {
        (*notes)[i].tempo = bpm;
    }

    /* cleanup */

    del_fvec(ibuf);
    del_fvec(obuf_notes);
    del_fvec(obuf_tempo);

    del_aubio_notes(notes_ctx);
    del_aubio_tempo(tempo_ctx);

    return notecount;
}

unsigned int get_modal_tempo(note_t *notes, unsigned int notecount)
{
    unsigned int mode;
    int freq, maxfreq;

    mode = 0;
    maxfreq = 0;

    for (int i = 0; i < notecount; i++)
    {
        freq = 0;

        for (int j = 0; j < notecount; j++)
        {
            if (notes[j].tempo == notes[i].tempo)
            {
                freq++;
            }
        }

        if (freq > maxfreq)
        {
            maxfreq = freq;
            mode = notes[i].tempo;
        }
    }

    return mode;
}
