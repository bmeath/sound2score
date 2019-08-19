/* recorder.h 
 * 2018 Brendan Meath
 */

#ifndef WAVRECORDER_H
#define WAVRECORDER_H

#if defined(__cplusplus)
extern "C" {
#endif

#include "wav.h"

#define WAVRECORDER_BUFSIZE     8192

#define STR(s) STR_2(s)
#define STR_2(s) #s

#define OPT_TERMINATOR          "--"

#define OPT_DURATION_SHORT		"-t"
#define OPT_DURATION_LONG	    "--time-limit="
#define OPT_DURATION_DEFAULT    7
#define OPT_DURATION_EXPLAIN	"set time limit in seconds, or 0 for no limit (default: " STR(OPT_DURATION_DEFAULT) ")"

#define OPT_RATE_SHORT		    "-s"
#define OPT_RATE_LONG		    "--sample-rate="
#define OPT_RATE_DEFAULT	    44100
#define OPT_RATE_EXPLAIN	    "set sample rate in Hz (default: " STR(OPT_RATE_DEFAULT) ")"

#define OPT_BITS_SHORT		    "-b"
#define OPT_BITS_LONG		    "--bits-per-sample="
#define OPT_BITS_DEFAULT	    16
#define OPT_BITS_EXPLAIN	    "set number of bits per sample (default: " STR(OPT_BITS_DEFAULT) ")"

#define OPT_CHANNELS_SHORT		"-c"
#define OPT_CHANNELS_LONG		"--channels="
#define OPT_CHANNELS_DEFAULT    1
#define OPT_CHANNELS_EXPLAIN	"set number of audio channels (default: " STR(OPT_CHANNELS_DEFAULT) ")"

#define OPT_QUIET_SHORT		    "-q"
#define OPT_QUIET_LONG		    "--quiet"
#define OPT_QUIET_DEFAULT	    0
#define OPT_QUIET_EXPLAIN		"suppress all output except for error messages"

#define OPT_HELP_SHORT			"-h"
#define OPT_HELP_LONG			"--help"
#define OPT_HELP_DEFAULT        0
#define OPT_HELP_EXPLAIN		"show this"


/* program configuration, which can be configured through command line options */
typedef struct options
{
	int32_t duration;   // maximum duration of recording

	int16_t channels;   // number of recording channels
    int32_t rate;       // sample rate
    int16_t bitdepth;   // resolution (number of bits) for a sample

    int help;           // if set, program will show usage and exit
    int quiet;          // if set, all non-critical output will be suppressed

} options_t;

typedef struct wavrecorder
{
    const char  *fname;         // destination filename
    const char  *devname;       // recording device name

	ALCdevice   *dev;           // input device

    unsigned char   *buf;       // input buffer
    size_t     bufsize;         // length of input buffer
    int32_t     bufnmemb;       // how many samples the buffer can hold

    FILE        *outfp;         // output file

    uint32_t      nsamples;     // sample count

	int16_t      blockalign;    // bytes per sample times number of channels
	int16_t     bitdepth;       // bits per sample

    int silent; // whether or not to suppress informational messages
} wavrecorder_t;



/* method which takes the command line args and performs a recording */
int wavrecorder(char *fname, options_t opts);

/* signal handler which allows graceful shutdown of an ongoing recording */
void on_signal(int s);

/* output a program usage message */
void print_usage(FILE *fp, const char *prog_name);

/* initialises the program options to their default configuration */
void init_options(options_t *o);

/* validates and retrieves program options from the command line arguments */
int parse_options(int argc, char **argv, int *current_arg_index, options_t *dst);

/* returns the appropriate OpenAL enum value for the given recording format.
 *
 * parameters:
 *    bitdepth: number of bits in each recorded sample
 *    nchannels: number of channels
 */
ALenum get_al_format(int bitdepth, int nchannels);

#if defined(__cplusplus)
}
#endif

#endif
