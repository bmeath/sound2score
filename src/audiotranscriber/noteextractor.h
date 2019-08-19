/* noteextractor.h
 * 2019 Brendan Meath
 */

#ifndef NOTEEXTRACTOR_H
#define NOTEEXTRACTOR_H

#if defined(__cplusplus)
extern "C" {
#endif

#include <aubio/aubio.h>

#include "note.h"

int extract_notes(
    aubio_source_t *source,
    unsigned int winsize,
    unsigned int hopsize,
    unsigned int bpm,
    note_t **notes
);

unsigned int get_modal_tempo(note_t *notes, unsigned int notecount);

#if defined(__cplusplus)
}
#endif

#endif
