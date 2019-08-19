#ifndef NOTE_H
#define NOTE_H

#if defined(__cplusplus)
extern "C" {
#endif

typedef struct note
{
    unsigned int pitch;             // MIDI pitch number    (pitch/frequency) (0-127)
    unsigned int velocity;          // MIDI velocity number (loudness) (0-127)
    unsigned int tempo;             // estimated tempo (in beats per minute)
    double  start_sec;    // start time, offset from beginning of entire music
    double  stop_sec;     // stop time, offset from beginning of entire music
} note_t;

#if defined(__cplusplus)
}
#endif

#endif
