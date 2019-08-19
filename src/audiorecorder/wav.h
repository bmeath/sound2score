#ifndef WAV_H
#define WAV_H

#if defined(__cplusplus)
extern "C" {
#endif

/* values for "wave format tag" header field (in host byte order) */
#define WAVE_FORMAT_PCM			0x0001 /* PCM */
#define WAVE_FORMAT_IEEE_FLOAT  0x0003 /* IEEE Float */
#define WAVE_FORMAT_UNKNOWN     0x0000 /* unknown */

/* header structure of a WAVE audio file */
typedef struct wavheader
{
    char        chunkid[4];     // {'R', 'I', 'F', 'F'}
    int32_t     chunksize;      // size of rest of this chunk
    char        format[4];      // {'W', 'A', 'V', 'E'}

    // start of subchunk 1
    char        subchunk1id[4]; // {'f, 'm', 't', ' '}
    int32_t     subchunk1size;  // length of subchunk1 following this field
    int16_t     audioformat;    // format of contained audio
    int16_t     nchannels;      // number of audio channels
    int32_t     srate;          // sampling frequency
    int32_t     byterate;       // number of bytes per second
    int16_t     blockalign;     // (bitdepth) * (nchannels) * 8
    int16_t     bitdepth;       // bits per sample
    // end of subchunk 1
    // start of subchunk 2
    char        subchunk2id[4]; // {'d', 'a', 't', 'a'}
    int32_t     subchunk2size;  // size of rest of this subchunk
    /* Audio sample data begins after the above field (subchunk2size),
     * and is considered to occupy the rest of subchunk 2.
     */
} wavheader_t;

size_t get_wavheader_len();

/* Returns the correct WAVE audio format tag (in host byte order) based on the inputted bit depth. */
int16_t get_wav_format(const int16_t bitdepth);

void init_wavheader(wavheader_t *hdr, const int16_t bitdepth, const int16_t nchannels, const int32_t srate);

void finalise_wavheader(wavheader_t *hdr, const long fsize);

int write_wavheader(const wavheader_t *hdr, FILE *fp);

void print_wavheader(FILE *fp, wavheader_t *hdr);

int wav_prepare_pcm(unsigned char *samples, int bitdepth, int nchannels, int nsamples);

/* writes n bytes from src to dst, then increments dst pointer by n */
void bufferwrite(unsigned char **dst, void *src, size_t nbytes);

#if defined(__cplusplus)
}
#endif

#endif
