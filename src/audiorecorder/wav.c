#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>

#include "endianness.h"
#include "wav.h"

/* Returns the correct WAVE audio format tag (in host byte order) based on the inputted bit depth.
 *
 * parameters
 *    bitdepth: number of bits in each recorded sample
 */
int16_t get_wav_format(const int16_t bitdepth)
{
    switch (bitdepth)
    {
        case 8:
        case 16:
            return WAVE_FORMAT_PCM;
        case 32:
            return WAVE_FORMAT_IEEE_FLOAT;
        default:
            return WAVE_FORMAT_UNKNOWN;
    }
}

size_t get_wavheader_len()
{
    const wavheader_t *p = NULL;

    return (uint32_t)
        /* treat NULL as a wavheader_t pointer,
         * to get the size of each member without requiring
         * an instance of wavheader_t
         */
        sizeof(p->chunkid)          + 
        sizeof(p->chunksize)        +
        sizeof(p->format)           +
        sizeof(p->subchunk1id)      +
        sizeof(p->subchunk1size)    +
        sizeof(p->audioformat)      +
        sizeof(p->nchannels)        +
        sizeof(p->srate)            +
        sizeof(p->byterate)         +
        sizeof(p->blockalign)       +
        sizeof(p->bitdepth)         +
        sizeof(p->subchunk2id)      +
        sizeof(p->subchunk2size)    ;
}

void init_wavheader(wavheader_t *hdr, const int16_t bitdepth, const int16_t nchannels, const int32_t srate)
{
    if (hdr)
    {
        hdr->chunkid[0] = 'R';
        hdr->chunkid[1] = 'I';
        hdr->chunkid[2] = 'F';
        hdr->chunkid[3] = 'F';

        hdr->format[0] = 'W';
        hdr->format[1] = 'A';
        hdr->format[2] = 'V';
        hdr->format[3] = 'E';

        hdr->subchunk1id[0] = 'f';
        hdr->subchunk1id[1] = 'm';
        hdr->subchunk1id[2] = 't';
        hdr->subchunk1id[3] = ' ';

        hdr->subchunk2id[0] = 'd';
        hdr->subchunk2id[1] = 'a';
        hdr->subchunk2id[2] = 't';
        hdr->subchunk2id[3] = 'a';

        hdr->subchunk1size =
            sizeof(hdr->audioformat)    +
            sizeof(hdr->nchannels)      +
            sizeof(hdr->srate)          +
            sizeof(hdr->byterate)       +
            sizeof(hdr->blockalign)     +
            sizeof(hdr->bitdepth)       ;

        hdr->blockalign = nchannels * bitdepth / 8;
        hdr->byterate = srate * hdr->blockalign;

        hdr->bitdepth = bitdepth;
        hdr->audioformat = get_wav_format(bitdepth);

        hdr->nchannels = nchannels;

        hdr->srate = srate;
    }
}

void finalise_wavheader(wavheader_t *hdr, const long fsize)
{
    if (hdr)
    {
        // size of everything following the chunksize field
        hdr->chunksize = fsize - (sizeof(hdr->chunkid) + sizeof(hdr->chunksize));

        // size of everything following the header section of the file
        hdr->subchunk2size = fsize - get_wavheader_len();
    }
}

/* writes n bytes located at src to *dst, and increments *dst by n */
void bufferwrite(unsigned char **dst, void *src, size_t nbytes)
{
    memcpy((void *) *dst, src, nbytes);
    *dst += nbytes;
}

/* converts the samples in buf to little-endian if needed,
 * returns:
 * zero: success
 * non-zero: samplesbuffer is NULL or channels/bitdepth is unsupported
 */
int wav_prepare_pcm(
    unsigned char *samplesbuffer,
    int bitdepth,
    const int nchannels,
    const int nsamples)
{
    if (!samplesbuffer)
    {
        return 1;
    }

    if (bitdepth == 8)
    {
        // each sample is only 1 byte, so byte order can't be swapped.
        return 0;
    }
    else if (bitdepth == 16)
    {
        for (size_t i = 0; i < nchannels * nsamples; i += 2)
        {
            samplesbuffer[i] = le16(samplesbuffer[i]);
        }
        return 0;
    }
    else if (bitdepth == 32)
    {
        for (size_t i = 0; i < nchannels * nsamples; i += 4)
        {
            samplesbuffer[i] = le32(samplesbuffer[i]);
        }
        return 0;
    }

    return 1;
}

int write_wavheader(const wavheader_t *hdr, FILE *fp)
{
    if (hdr == NULL || fp == NULL)
    {
        return 1;
    }

    const int hdr_len = get_wavheader_len();
    unsigned char buf[hdr_len];
    unsigned char *bufptr = buf;

    /* I will use this to represent the WAVE header in little endian form.
     * 
     * This is because WAVE audio file headers are ALWAYS stored on disk in 
     * little endian form, regardless of processor architecture. Character-based
     * fields are unaffected, since the char data type is only 1 byte wide.
     */
    wavheader_t hdr_le;

    /* make a local copy of the header,
     * converting fields to little endian where required.
     */
    memcpy(&hdr_le.chunkid, hdr->chunkid, sizeof(hdr_le.chunkid));
    hdr_le.chunksize = le32(hdr->chunksize);
    memcpy(&hdr_le.format, hdr->format, sizeof(hdr_le.format));
    memcpy(&hdr_le.subchunk1id, hdr->subchunk1id, sizeof(hdr_le.subchunk1id));
    hdr_le.subchunk1size = le32(hdr->subchunk1size);
    hdr_le.audioformat = le16(hdr->audioformat);
    hdr_le.nchannels = le16(hdr->nchannels);
    hdr_le.srate = le32(hdr->srate);
    hdr_le.byterate = le32(hdr->byterate);
    hdr_le.blockalign = le16(hdr->blockalign);
    hdr_le.bitdepth = le16(hdr->bitdepth);
    memcpy(&hdr_le.subchunk2id, hdr->subchunk2id, sizeof(hdr_le.subchunk2id));
    hdr_le.subchunk2size = le32(hdr->subchunk2size);

    /* create an intermediate buffer for little-endian representation.
     * We do not supply wavheader_t struct directly to fwrite,
     * because the struct potentially contains padding.
     * Therefore we copy each member individually to a byte array.
     */
    bufferwrite(&bufptr, &hdr_le.chunkid,       sizeof(hdr_le.chunkid));
    bufferwrite(&bufptr, &hdr_le.chunksize,     sizeof(hdr_le.chunksize));
    bufferwrite(&bufptr, &hdr_le.format,        sizeof(hdr_le.format));
    bufferwrite(&bufptr, &hdr_le.subchunk1id,   sizeof(hdr_le.subchunk1id));
    bufferwrite(&bufptr, &hdr_le.subchunk1size, sizeof(hdr_le.subchunk1size));
    bufferwrite(&bufptr, &hdr_le.audioformat,   sizeof(hdr_le.audioformat));
    bufferwrite(&bufptr, &hdr_le.nchannels,     sizeof(hdr_le.nchannels));
    bufferwrite(&bufptr, &hdr_le.srate,         sizeof(hdr_le.srate));
    bufferwrite(&bufptr, &hdr_le.byterate,      sizeof(hdr_le.byterate));
    bufferwrite(&bufptr, &hdr_le.blockalign,    sizeof(hdr_le.blockalign));
    bufferwrite(&bufptr, &hdr_le.bitdepth,      sizeof(hdr_le.bitdepth));
    bufferwrite(&bufptr, &hdr_le.subchunk2id,   sizeof(hdr_le.subchunk2id));
    bufferwrite(&bufptr, &hdr_le.subchunk2size, sizeof(hdr_le.subchunk2size));

    fwrite(buf, sizeof(buf), 1, fp);
    if (ferror(fp))
    {
        return 1;
    }

    return 0;
}

void print_wavheader(FILE *fp, wavheader_t *hdr)
{
    const int colspan = 15;
    const char *hborder = "+----------------+----------------+----------------+";

    fprintf(fp, "%s\n"
                "| %-*s| %-*s| %-*s|\n"
                "%s\n"
                "| %-*d| %-*s| \"%c%c%c%c\"         |\n"
                "| %-*lu| %-*s| %-*d|\n"
                "| %-*d| %-*s| \"%c%c%c%c\"         |\n"
                "| %-*d| %-*s| \"%c%c%c%c\"         |\n"
                "| %-*lu| %-*s| %-*d|\n"
                "| %-*lu| %-*s| %-*d|\n"
                "| %-*lu| %-*s| %-*d|\n"
                "| %-*lu| %-*s| %-*d|\n"
                "| %-*lu| %-*s| %-*d|\n"
                "| %-*lu| %-*s| %-*d|\n"
                "| %-*lu| %-*s| %-*d|\n"
                "| %-*d| %-*s| \"%c%c%c%c\"         |\n"
                "| %-*lu| %-*s| %-*d|\n"
                "%s\n",

                hborder,
                colspan,    "SIZE (bytes)",             colspan, "NAME",            colspan, "VALUE",
                hborder,
                colspan,    4,                          colspan, "chunkid",         hdr->chunkid[0], hdr->chunkid[1], hdr->chunkid[2], hdr->chunkid[3],
                colspan,    sizeof(hdr->chunksize),     colspan, "chunksize",       colspan, hdr->chunksize,
                colspan,    4,                          colspan, "format",          hdr->format[0], hdr->format[1], hdr->format[2], hdr->format[3],
                colspan,    4,                          colspan, "subchunk1id",     hdr->subchunk1id[0], hdr->subchunk1id[1], hdr->subchunk1id[2], hdr->subchunk1id[3],
                colspan,    sizeof(hdr->subchunk1size), colspan, "subchunk1size",   colspan, hdr->subchunk1size,
                colspan,    sizeof(hdr->audioformat),   colspan, "audioformat",     colspan, hdr->audioformat,
                colspan,    sizeof(hdr->nchannels),     colspan, "nchannels",       colspan, hdr->nchannels,
                colspan,    sizeof(hdr->srate),         colspan, "srate",           colspan, hdr->srate,
                colspan,    sizeof(hdr->byterate),      colspan, "byterate",        colspan, hdr->byterate,
                colspan,    sizeof(hdr->blockalign),    colspan, "blockalign",      colspan, hdr->blockalign,
                colspan,    sizeof(hdr->bitdepth),      colspan, "bitdepth",        colspan, hdr->bitdepth,
                colspan,    4,                          colspan, "subchunk2id",     hdr->subchunk2id[0], hdr->subchunk2id[1], hdr->subchunk2id[2], hdr->subchunk2id[3],
                colspan,    sizeof(hdr->subchunk2size), colspan, "subchunk2size",   colspan, hdr->subchunk2size,
                hborder
   );
               
}
