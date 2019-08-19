/* midi.h
 * 2019 Brendan Meath
 *
 * Based on MIDI specification 1.0
 */

#ifndef MIDI_H
#define MIDI_H

#if defined(__cplusplus)
extern "C" {
#endif

/* some default configuration values */
#define MIDI_BPM_DEFAULT  120       // tempo in beats per minute
#define MIDI_PPQ_DEFAULT   96       // pulses per quarter note (crotchet)

/* MIDI track buffer management */
#define MIDIEVENTS_BUFSIZE   (16384) // initial buffer size
#define MIDIEVENTS_BUFINCR   (4096)  // how much to increase buffer by when full

/* Variable-length integer encoding */
#define VARINT32_MAXSIZE    (sizeof(int32_t) + 1) // most bytes needed by varint

/* MIDI header value ranges */
#define MIDI_PITCH_MAX      127
#define MIDI_PITCH_MIN      0

#define MIDI_VELOCITY_MAX   127
#define MIDI_VELOCITY_MIN   0

#define MIDI_CHANNEL_MAX    15
#define MIDI_CHANNEL_MIN    0

/* Some MIDI event status codes */

#define MIDIEVENT_NOTEOFF   0x80    // start of a note (note onset)
#define MIDIEVENT_NOTEON    0x90    // end of a note (note decay)
#define MIDIEVENT_META      0xff    // meta event

/* Some Meta event data values */

#define META_1_ENDTRACK     0x2f    // End of track event, first byte
#define META_2_ENDTRACK     0x00    // End of track event, second byte

#define META_1_INSTRUMENT   0x04    /* Instrument name event.
                                     * 2nd byte is number (0-15) of bytes in the
                                     * instrument name.
                                     * Subsequent bytes contain said name.
                                     */

#define META_1_TEMPO        0x51    // Tempo event, first byte
#define META_2_TEMPO        0x03    // Tempo event, second byte

#define META_1_TIMESIG      0x58    // Time signature, first byte
#define META_2_TIMESIG      0x04    /* Time signature, second byte.
                                     * 4 bytes must follow, containing the time
                                     * signature. See SMF Spec 1.1
                                     */

/* some structures that describe the organisation of a MIDI file */

typedef struct miditrack
{
    char            chunkid[4];     // {'M, 'T', 'r', 'k '} denotes track header
    uint32_t        chunksize;      // size of track chunk following this field

    unsigned char   *events;        /* a buffer of the MIDI events in their
                                     * bit-packed form. */

    unsigned char   *eventsp;       /* For use when accessing buffer.
                                     * Not a header field. */
    size_t          events_size;    /* To remember size of malloc'ed buffer
                                     * Not a header field. */
} miditrack_t;

typedef struct midiheader
{
    unsigned char   chunkid[4];     // {'M', 'T', 'h', 'd'} denotes header
    uint32_t        chunksize;      // chunk size, excluding ID and this field.

    uint16_t        format;         /* 0: single-track
                                     * 1: multi-track (simultaneous)
                                     * 2: multi-track (sequentially independent) */

    uint16_t        ntracks;        // number of track chunks.

    uint16_t        time_div;       /* Denotes timing format of the MIDI file.
                                     *
                                     * If high bit = 0, then remaining 15 bits
                                     * indicate number of ticks per crotchet,
                                     * aka pulses per quarter note (PPQ).
                                     * Higher amount of ticks = more precision.
                                     *
                                     * If high bit = 1, then the field describes
                                     * a MIDI Time Code format. */
} midiheader_t;

typedef struct midifile
{
    struct midiheader header;
    struct miditrack *tracks;
} midifile_t;


/* Initialisation functions */

int init_midifile(midifile_t *midif,
    const uint16_t format,
    const uint16_t ntracks,
    const uint16_t nticks,
    const size_t track_bufsize);

int init_midiheader(midiheader_t *hdr,
    const uint16_t format,
    const uint16_t ntracks,
    const uint16_t nticks);

int init_miditrack(miditrack_t *trk, size_t bufsize);


/* Finalisation functions */

int finalise_midifile(midifile_t *midif);

int finalise_miditrack(miditrack_t *trk);


/* I/O functions */

int write_midifile(midifile_t *midif, int fd);

int write_midiheader(midiheader_t *hdr, int fd);

int write_miditrack(miditrack_t *trk, int fd);

void print_midiheader(midifile_t *mf, FILE *fp);


/* MIDI Event creation functions */

int miditrack_noteon(miditrack_t *trk,
    uint32_t deltatime,
    unsigned char channel,
    unsigned char pitch,
    unsigned char velocity
);
    
int miditrack_noteoff(miditrack_t *trk,
    uint32_t deltatime,
    unsigned char channel,
    unsigned char pitch,
    unsigned char velocity
);

int miditrack_settempo(miditrack_t *trk,
    uint32_t deltatime,
    unsigned int bpm
);

int miditrack_end(miditrack_t *trk, uint32_t deltatime);

int miditrack_addevent(miditrack_t *track,
    uint32_t deltatime,
    unsigned char status,
    unsigned char data1,
    unsigned char data2,
    unsigned char *extra,
    unsigned int extralen
);


/* MIDI size and memory management functions */

size_t midifile_getsize(const midifile_t *midif);

size_t midiheader_getsize();

size_t miditrack_getsize(const miditrack_t *trk);

size_t miditrack_buffer_remaining(miditrack_t *track);

int miditrack_buffer_increase(miditrack_t *track, ssize_t diff);

void free_midifile(midifile_t *midif);

void free_miditrack(miditrack_t *trk);


/* other functions */

void bwrite(unsigned char **dst, void *src, size_t nbytes);

int enc_varint32(uint32_t val, unsigned char *dst);

#if defined(__cplusplus)
}
#endif

#endif
