/* midi.c
 * 2019 Brendan Meath
 */

#include <stdint.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>

#include "endianness.h"
#include "midi.h"

// wrapper for the MIDI header and track chunk initialisation functions
int init_midifile(midifile_t *midif,
    const uint16_t format,
    const uint16_t ntracks,
    const uint16_t nticks,
    const size_t track_bufsize)
{
    if (!midif)
    {
        return 1;
    }

    if (init_midiheader(&midif->header, format, ntracks, nticks) != 0)
    {
        return 1;
    }

    midif->tracks = malloc(sizeof(miditrack_t) * midif->header.ntracks);

    if (!midif->tracks)
    {
        return 1;
    }

    for (int t = 0; t < midif->header.ntracks; t++)
    {
        /*midif->tracks[t] = malloc(sizeof(miditrack_t));
        if (!midif->tracks[t])
        {
            return 1;
        }*/

        if (init_miditrack(&midif->tracks[t], track_bufsize) != 0)
        {
            return 1;
        }
    }

    return 0;
}

int init_midiheader(midiheader_t *hdr,
    const uint16_t format,
    const uint16_t ntracks,
    const uint16_t nticks)
{
    if (!hdr)
    {
        return 1;
    }

    hdr->chunkid[0] = 'M';
    hdr->chunkid[1] = 'T';
    hdr->chunkid[2] = 'h';
    hdr->chunkid[3] = 'd';

    hdr->chunksize = midiheader_getsize()
        - sizeof(hdr->chunkid)
        - sizeof(hdr->chunksize);

    // use parameters, otherwise assign a default value
    hdr->format = format ? format : 0;
    hdr->ntracks = ntracks ? ntracks : 1;
    hdr->time_div = nticks ? nticks : 96;

    return 0;
}

int init_miditrack(miditrack_t *trk, const size_t bufsize)
{
    if (!trk)
    {
        return 1;
    }

    trk->chunkid[0] = 'M';
    trk->chunkid[1] = 'T';
    trk->chunkid[2] = 'r';
    trk->chunkid[3] = 'k';

    trk->events_size = bufsize ? bufsize : MIDIEVENTS_BUFSIZE; 

    trk->events = (unsigned char *) malloc(trk->events_size);

    if (trk->events == NULL)
    {
        return 1;
    }

    trk->eventsp = trk->events;

    return 0;
}

// wrapper for the MIDI header and track chunk finalisation functions
int finalise_midifile(midifile_t *midif)
{
    if (!midif)
    {
        return 1;
    }

    for (int t = 0; t < midif->header.ntracks; t++)
    {
        if (finalise_miditrack(&midif->tracks[t]) != 0)
        {
            return 1;
        }
    }

    return 0;
}

/* init_miditrack should be called prior to calling this. */
int finalise_miditrack(miditrack_t *trk)
{
    if (!trk || !trk->events || !trk->eventsp)
    {
        // can't finalise the track if the events buffer is not allocated!
        return 1;
    }

    // check for prescence of End-of-track event
    if (*(trk->eventsp - 3) != MIDIEVENT_META    ||
        *(trk->eventsp - 2) != META_1_ENDTRACK   || 
        *(trk->eventsp - 1) != META_2_ENDTRACK)
    {
        // End-of-track event not detected. Add it now.
        miditrack_end(trk, 0);
    }

    // check if there is excess space in events buffer
    ssize_t diff = miditrack_buffer_remaining(trk);
    if (diff > 0)
    {
        // free the excess buffer space
        if (miditrack_buffer_increase(trk, -diff) != 0)
        {
            return 1;
        }
    }

    trk->chunksize = trk->events_size;
    return 0;
}

int write_midifile(midifile_t *midif, int fd)
{
    if (midif == NULL || fd < 0)
    {
        return 1;
    }

    if (write_midiheader(&midif->header, fd) != 0)
    {
        return 1;
    }

    for (int t = 0; t < midif->header.ntracks; t++)
    {
        if (write_miditrack(&midif->tracks[t], fd) != 0)
        {
            return 1;
        }
    }

    return 0;
}

int write_midiheader(midiheader_t *hdr, int fd)
{
    if (!hdr || fd < 0)
    {
        return 1;
    }

    // convert integer-based fields to big-endian
    uint32_t chunksize_be   = be32(hdr->chunksize);
    uint16_t format_be      = be16(hdr->format);
    uint16_t ntracks_be     = be16(hdr->ntracks);
    uint16_t time_div_be    = be16(hdr->time_div);

    // create intermediate buffer
    size_t len = midiheader_getsize();
    unsigned char start[len];
    unsigned char *buf = start;

    // store all fields sequentially in the buffer
    bwrite(&buf, hdr->chunkid, sizeof(hdr->chunkid));
    bwrite(&buf, &chunksize_be, sizeof(chunksize_be));
    bwrite(&buf, &format_be,    sizeof(format_be));
    bwrite(&buf, &ntracks_be,   sizeof(ntracks_be));
    bwrite(&buf, &time_div_be,  sizeof(time_div_be));

    // write header to file
    if (write(fd, start, len) != len)
    {
        return 1;
    }

    return 0;
}

int write_miditrack(miditrack_t *trk, int fd)
{
    if (!trk || fd < 0)
    {
        return 1;
    }

    // convert integer-based fields to big-endian
    uint32_t chunksize_be   = be32(trk->chunksize);

    // create intermediate buffer
    size_t len = sizeof(chunksize_be) + sizeof(trk->chunkid);
    unsigned char start[len];
    unsigned char *buf = start;

    // store all fields sequentially in the buffer
    bwrite(&buf, trk->chunkid, sizeof(trk->chunkid));
    bwrite(&buf, &chunksize_be, sizeof(chunksize_be));

    // write track header to file
    if (write(fd, start, len) != len)
    {
        return 1;
    }

    // write track events to file
    len = trk->events_size;
    if (write(fd, trk->events, len) != len)
    {
        return 1;
    }

    return 0;
}

/* sample output:
+----------------+----------------+----------------+
| SIZE (bytes)   | NAME           | VALUE          |
+----------------+----------------+----------------+
| 4              | chunkid        | MThd           |
| 4              | chunksize      | 6              |
| 2              | format         | 0              |
| 2              | ntracks        | 1              |
| 2              | time_div       | 96             |
+----------------+----------------+----------------+
| 4              | chunkid        | MTrk           |
| 4              | chunksize      | 164            |
+----------------+----------------+----------------+
(end of sample output)
*/
void print_midiheader(midifile_t *m, FILE *fp)
{
    const int w = 15; // column width
    const char *border = "+----------------+----------------+----------------+";

    fprintf(fp, "%s\n"
                "| %-*s| %-*s| %-*s|\n"
                "%s\n"
                "| %-*zu| %-*s| %-*.*s|\n"
                "| %-*zu| %-*s| %-*u|\n"
                "| %-*zu| %-*s| %-*u|\n"
                "| %-*zu| %-*s| %-*u|\n"
                "| %-*zu| %-*s| %-*u|\n",
        border,
        w, "SIZE (bytes)",               w, "NAME",     w, "VALUE",
        border,
        w, sizeof(m->header.chunkid),   w, "chunkid",   w,
            (int) sizeof(m->header.chunkid), m->header.chunkid,
        w, sizeof(m->header.chunksize), w, "chunksize", w, m->header.chunksize,
        w, sizeof(m->header.format),    w, "format",    w, m->header.format,
        w, sizeof(m->header.ntracks),   w, "ntracks",   w, m->header.ntracks,
        w, sizeof(m->header.time_div),  w, "time_div",  w, m->header.time_div
    );

    if (m->tracks != NULL)
    {
        for (int t = 0; t < m->header.ntracks; t++)
        {
            fprintf(fp, "%s\n"
                        "| %-*zu| %-*s| %-*.*s|\n"
                        "| %-*lu| %-*s| %-*u|\n",
                border,
                w, sizeof(m->tracks[t].chunkid),   w, "chunkid",   w,
                    (int) sizeof(m->tracks[t].chunkid), m->tracks[t].chunkid,
                w, sizeof(m->tracks[t].chunksize), w, "chunksize", w,
                    m->tracks[t].chunksize
            );
        }
    }

    fprintf(fp, "%s\n", border);
}

int miditrack_noteon(miditrack_t *trk,
    uint32_t deltatime,
    unsigned char channel,
    unsigned char pitch,
    unsigned char velocity
)
{
    if (channel > MIDI_CHANNEL_MAX || pitch > MIDI_PITCH_MAX)
    {
        return 1;
    }
    if (velocity > MIDI_VELOCITY_MAX)
    {
        velocity = MIDI_VELOCITY_MAX;
    }

    fprintf(stderr, "noteon: pitch=%u, ticks=%u\n",
        pitch, deltatime
    );

    return miditrack_addevent(trk,
        deltatime,
        MIDIEVENT_NOTEON | channel,
        pitch,
        velocity,
        NULL,
        0
    );
}

int miditrack_noteoff(miditrack_t *trk,
    uint32_t deltatime,
    unsigned char channel,
    unsigned char pitch,
    unsigned char velocity
)
{
    fprintf(stderr, "noteoff: pitch=%u, ticks=%u\n",
       pitch, deltatime
    );

    if (channel > MIDI_CHANNEL_MAX || pitch > MIDI_PITCH_MAX)
    {
        return 1;
    }
    if (velocity > MIDI_VELOCITY_MAX)
    {
        velocity = MIDI_VELOCITY_MAX;
    }

    return miditrack_addevent(trk,
        deltatime,
        MIDIEVENT_NOTEOFF | channel,
        pitch,
        velocity,
        NULL,
        0
    );
}

int miditrack_settempo(miditrack_t *trk,
    uint32_t deltatime,
    unsigned int bpm
)
{
    unsigned int usec_per_beat;

    // Prevent divide-by-zero.
    if (bpm == 0)
    {
        return 1;
    }

    /* Convert from beats per minute to microseconds per beat.
     * Using 60000000 because there are 60 million microseconds in a minute.
     */
    usec_per_beat = 60000000 / bpm;

    /* convert the above value to a 24 bit most-significant-byte-first value,
     * which is how it will be written to the disk
     */
    unsigned char tempo_24bit[3] = {
        (usec_per_beat >> 16)  & 0xff,
        (usec_per_beat >> 8)   & 0xff,
        (usec_per_beat)        & 0xff
    };

    return miditrack_addevent(trk,
        deltatime,
        MIDIEVENT_META,
        META_1_TEMPO,
        META_2_TEMPO,
        tempo_24bit,
        sizeof(tempo_24bit)
    );
}

int miditrack_end(miditrack_t *trk, uint32_t deltatime)
{
    return miditrack_addevent(trk,
        deltatime,
        MIDIEVENT_META,
        META_1_ENDTRACK,
        META_2_ENDTRACK,
        NULL,
        0
    );
}

int miditrack_addevent(miditrack_t *track,
    uint32_t deltatime,
    unsigned char status,
    unsigned char data1,
    unsigned char data2,
    unsigned char *extra,
    unsigned int extralen
)
{
    /* Check that the track and its event buffer were created.
     * Also check if the caller has indicated there are extralen bytes of
     * data in extra, but extra is NULL
     */
    if (!track || !track->events || (!extra && extralen > 0))
    {
        return 1;
    }

    /* Check if there is definitely enough space for the event data.
     */
    const int min_free = 263; /* Some text-based MIDI events can come close to
                               * this many bytes.
                               *
                               * e.g.     5 bytes for a very long deltatime
                               *          3 bytes for status+data1+data2
                               *        255 bytes for longest text
                               *        total: 263 bytes
                               */

    if (miditrack_buffer_remaining(track) < min_free)
    {
        /* We will increase the buffer significantly so that we won't be running 
         * out of memory and reallocating for every single event that is added.
         */
        if (miditrack_buffer_increase(track, MIDIEVENTS_BUFINCR) != 0)
        {
            return 1;
        }
    }

    // store track deltatime (time offset from previous event) in buffer
    track->eventsp += enc_varint32(deltatime, track->eventsp);

    // store rest of event data
    *(track->eventsp++) = status;
    *(track->eventsp++) = data1;
    *(track->eventsp++) = data2;

    // write any additional data
    for (int byte = 0; byte < extralen; byte++)
    {
        *(track->eventsp++) = extra[byte];
    }

    return 0;
}

size_t midifile_getsize(const midifile_t *midif)
{
    size_t s = midiheader_getsize();
    int trk = 0;

    while (trk < midif->header.ntracks)
    {
        s += miditrack_getsize(&midif->tracks[trk++]);
    }

    return s;
}

// returns size of entire MIDI header chunk. Not same as header chunk size!
size_t midiheader_getsize()
{
    /* treat NULL as a midiheader_t pointer,
     * to get the size of each member without requiring
     * an instance of midiheader_t
     */
    const midiheader_t *p = NULL;

    return (uint32_t)
        sizeof(p->chunkid)      + 
        sizeof(p->chunksize)    +
        sizeof(p->format)       +
        sizeof(p->ntracks)      +
        sizeof(p->time_div)     ;
}

size_t miditrack_getsize(const miditrack_t *trk)
{
    size_t s = 0;

    if (trk != NULL)
    {
        s +=
            sizeof(trk->chunkid)    +
            sizeof(trk->chunksize)  +
            trk->chunksize          ;
    }

    return s;
}

size_t miditrack_buffer_remaining(miditrack_t *track)
{
    // ensure track exists and events buffer is not NULL
    if (!track || !track->events )
    {
        return -1;
    }

    return track->events_size - (track->eventsp - track->events);
}

int miditrack_buffer_increase(miditrack_t *trk, ssize_t diff)
{
    // check if track exists and has a buffer allocated
    if (!trk || !trk->events || !trk->eventsp)
    {
        return 1;
    }

    ssize_t newsize = trk->events_size + diff;
    unsigned char *new = realloc(trk->events, newsize);
    if (!new)
    {
        return 1;
    }

    trk->events_size = newsize;

    if (new != trk->events)
    {
        // object was moved. update start pointer and current position
        trk->eventsp = new + (trk->eventsp - trk->events);
        trk->events = new;
    }

    return 0;
}

void free_midifile(midifile_t *midif)
{
    if (midif != NULL && midif->tracks != NULL)
    {
        while (midif->header.ntracks-- > 0)
        {
            // first free any track event buffers
            free_miditrack(&midif->tracks[midif->header.ntracks]);
            // then free the tracks themselves
            free(midif->tracks);
        }
    }
}

void free_miditrack(miditrack_t *trk)
{
    if (trk != NULL)
    {
        free(trk->events);
    }
}

/* writes n bytes located at src to *dst, and increments *dst by n */
void bwrite(unsigned char **dst, void *src, size_t nbytes)
{
    memcpy((void *) *dst, src, nbytes);
    *dst += nbytes;
}

/* Encodes a 32 bit integer as a variable-length sequence of bytes.
 *
 * parameters:
 *   val: the 32 bit integer to be encoded as a variable-length quantity
 *   dst: a buffer to hold the variable-length quantity
 */
int enc_varint32(uint32_t val, unsigned char *dst)   
{
    if (dst == NULL)
    {
        return 0;
    }

    const unsigned int msb_mask = 1 << 7;       // 1000 0000
    const unsigned int val_mask = msb_mask - 1; // 0111 1111

    unsigned char buf[VARINT32_MAXSIZE]; // to hold variable-length encoded int
    unsigned int pos = sizeof(buf) - 1; // we start from the end of the sequence

    int nbytes; // to record how many bytes were used to encode the integer

    /* Since we are starting with the last byte in the sequence, we set MSB to 0
     *
     * MSB=0 signfies to a MIDI parser that no more bytes follow.
     * MSB=1 signfies to a MIDI parser that there is another byte to follow.
     */
    buf[pos] = val & val_mask;

    while ((val >>= 7) > 0) // perform a right-shift of 7 bits until val == 0
    {
        val--;

        /* decrement our buffer position by one byte,
         * so that the previously written byte is not overwritten.
         */
        pos--;

        /* We set the MSB to 1, as per earlier explanation.
         */
        buf[pos] = msb_mask | (val & val_mask);
    }

    // copy the variable-length integer to the caller's buffer.
    nbytes = sizeof(buf) - pos;
    memcpy(dst, buf + pos, nbytes);

    return nbytes;
}
