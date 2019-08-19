#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>

#include "noteextractor.h"
#include "midiwriter.h"

#define STR(s) STR_2(s)
#define STR_2(s) #s

#define OPT_OPTIONS_TERMINATOR  "--"

#define OPT_OUTPUT_SHORT    "-o"
#define OPT_OUTPUT_LONG     "--output"
#define OPT_OUTPUT_DEFAULT  "out.mid"
#define OPT_OUTPUT_EXPLAIN  "set output file (default: " OPT_OUTPUT_DEFAULT ")"

#define OPT_WINSIZE_SHORT   "-w"
#define OPT_WINSIZE_LONG    "--window-size"
#define OPT_WINSIZE_DEFAULT 512
#define OPT_WINSIZE_EXPLAIN "set FFT window size, in samples (default: " STR(OPT_WINSIZE_DEFAULT) ")"

#define OPT_HOPSIZE_SHORT   "-H"
#define OPT_HOPSIZE_LONG    "--hop-size"
#define OPT_HOPSIZE_DEFAULT 256
#define OPT_HOPSIZE_EXPLAIN "set hop size, in samples (default: " STR(OPT_HOPSIZE_DEFAULT) ")"

#define OPT_BPM_SHORT       "-b"
#define OPT_BPM_LONG        "--bpm"
#define OPT_BPM_DEFAULT     0
#define OPT_BPM_EXPLAIN     "specify tempo of music in BPM (default: auto-detect)"

#define OPT_PPQ_SHORT       "-p"
#define OPT_PPQ_LONG        "--ppq"
#define OPT_PPQ_DEFAULT     96
#define OPT_PPQ_EXPLAIN     "set MIDI clock rate in PPQ (default: " STR(OPT_PPQ_DEFAULT) ")"

#define OPT_VERBOSE_SHORT   "-v"
#define OPT_VERBOSE_LONG    "--verbose"
#define OPT_VERBOSE_EXPLAIN "output extra information"

#define OPT_HELP_SHORT      "-h"
#define OPT_HELP_LONG       "--help"
#define OPT_HELP_EXPLAIN    "show this"

/* program configuration, which can be configured through command line options */
typedef struct options
{
	char *output;           // output file

    unsigned int winsize;
    unsigned int hopsize;

    unsigned int bpm;       // beats per minute
    unsigned int ppq;       // pulses per quarter note

    int verbose;            // print extra information to stdout
    int help;               // whether to print usage message
} options_t;


static void usage(const char *prog_name);
static void init_options(options_t *dst);
static int parse_options(int argc, char **argv, options_t *dst);

int main(int argc, char **argv)
{
    // for parsing command line options
    options_t opts;
    int numparsed;

    // audio source
    char *srcpath;
    aubio_source_t *aubio_source;

    // extracted musical notes
	note_t *notes;
	size_t notecount;


    /* parse command line arguments */
    init_options(&opts);
    numparsed = parse_options(argc, argv, &opts);
    if (numparsed == -1)
    {
        /* the parse_options function has already printed an error message,
         * no need to print another here
         */
        return 1;
    }
    if (opts.help)
    {
        usage(argv[0]);
        return 0;
    }

    // there should be 1 more (mandatory) argument remaining (the source path)
    if (argc - numparsed < 1)
    {
        fprintf(stderr, "Error: missing mandatory arguments\n");
        return 1;
    }
    srcpath = argv[numparsed++];

    // there should be no remaining arguments
    if (argc - numparsed > 0)
    {
        fprintf(stderr, "Error: too many arguments\n");
        return 1;
    }

    /* open audio source */
    aubio_source = new_aubio_source(srcpath, 0, opts.hopsize);
    if (aubio_source == NULL)
    {
	    fprintf(stderr, "Error: could not open input file '%s'\n", srcpath);
	    return 1;
    }

    /* extract notes from audio source */
	notecount = extract_notes(aubio_source, opts.winsize, opts.hopsize, opts.bpm, &notes);
    if (notecount < 0)
    {
        fprintf(stderr, "Error: Failed to process audio source\n");
    }

    /* print information about extracted notes */
    if (opts.verbose)
    {
        for (int i = 0; i < notecount; i++)
        {
            fprintf(stderr, "pitch:%4u, start_sec:%10f, stop_sec:%10f, velocity: %4u, tempo: %4u\n",
                notes[i].pitch,
                notes[i].start_sec,
                notes[i].stop_sec,
                notes[i].velocity,
                notes[i].tempo
            );
        }
    }

    /* cleanup */
    del_aubio_source(aubio_source);
	aubio_cleanup();

	return gen_midi_file(opts.output, notes, notecount, opts.ppq);
}

static void usage(const char *prog_name)
{
    // short option and long option width spec
    const int s_opt_width = 8, l_opt_width = 24;

	printf(
	    "Usage: %s [OPTION]... <FILE>\n"
	    "Transcribes the inputted audio, storing output in a MIDI file.\n"
		"Options:\n"
		"%*s, %-*s "OPT_OUTPUT_EXPLAIN"\n"
		"%*s, %-*s "OPT_BPM_EXPLAIN"\n"
		"%*s, %-*s "OPT_PPQ_EXPLAIN"\n"
		"%*s, %-*s "OPT_WINSIZE_EXPLAIN"\n"
		"%*s, %-*s "OPT_HOPSIZE_EXPLAIN"\n"
		"%*s, %-*s "OPT_VERBOSE_EXPLAIN"\n"
		"%*s, %-*s "OPT_HELP_EXPLAIN"\n"
		"\n"
		"Example:\n"
		"  %s %s %s %s\n", 

		prog_name,

        /* optional arguments */
        s_opt_width, OPT_OUTPUT_SHORT, 	l_opt_width, OPT_OUTPUT_LONG" FILE",
        s_opt_width, OPT_BPM_SHORT,     l_opt_width, OPT_BPM_LONG" NUM",
        s_opt_width, OPT_PPQ_SHORT,     l_opt_width, OPT_PPQ_LONG" NUM",
        s_opt_width, OPT_WINSIZE_SHORT, l_opt_width, OPT_WINSIZE_LONG" NUM",
        s_opt_width, OPT_HOPSIZE_SHORT, l_opt_width, OPT_HOPSIZE_LONG" NUM",
        s_opt_width, OPT_VERBOSE_SHORT, l_opt_width, OPT_VERBOSE_LONG,
        s_opt_width, OPT_HELP_SHORT,    l_opt_width, OPT_HELP_LONG,

        /* example invocation arguments */
		prog_name, OPT_OUTPUT_SHORT, OPT_OUTPUT_DEFAULT, "in.wav"
	);
}

/* initialises the options_t structure
 * with the default value for each member (as per definitions in recorder.h)
 */
static void init_options(options_t *dst)
{
    if (dst)
    {
        dst->output = OPT_OUTPUT_DEFAULT;

        dst->hopsize = OPT_HOPSIZE_DEFAULT;
        dst->winsize = OPT_WINSIZE_DEFAULT;

        dst->bpm = OPT_BPM_DEFAULT;
        dst->ppq = OPT_PPQ_DEFAULT;

        dst->verbose = 0;
        dst->help = 0;
    }
}

/* returns number of tokens parsed in argv,
 * or -1 if there was an error
 */
static int parse_options(int argc, char **argv, options_t *dst)
{
    char *prog_name;
    int tmp = argc;

    if (dst == NULL)
    {
        return -1;
    }

    prog_name = *(argv++);
    while (--argc)
    {
        if (strcmp(*argv, OPT_HELP_SHORT) == 0 || strcmp(*argv, OPT_HELP_LONG) == 0)
        {
            dst->help = 1;
        }
        else if (strcmp(*argv, OPT_VERBOSE_SHORT) == 0 || strcmp(*argv, OPT_VERBOSE_LONG) == 0)
        {
            dst->verbose = 1;
        }
        else if (strcmp(*argv, OPT_OUTPUT_SHORT) == 0 || strcmp(*argv, OPT_OUTPUT_LONG) == 0)
        {
            if (argc > 1)
            {
                argc--;
                argv++;
                dst->output = *argv;
            }
            else
            {
                fprintf(stderr, "%s: missing value for flag '%s'\n", prog_name, *argv);
                return -1;
            }
        }
        else if (strcmp(*argv, OPT_WINSIZE_SHORT) == 0 || strcmp(*argv, OPT_WINSIZE_LONG) == 0)
        {
            if (argc > 1)
            {
                argc--;
                argv++;
                dst->winsize = strtoul(*argv, NULL, 10);

                if (errno == ERANGE || errno == EINVAL)
                {
                    fprintf(stderr, "%s: failed to parse numeric argument to flag '%s'\n", prog_name, *(argv - 1));
                    return -1;
                }
            }
            else
            {
                fprintf(stderr, "%s: missing value for flag '%s'\n", prog_name, *argv);
                return -1;
            }
        }
        else if (strcmp(*argv, OPT_HOPSIZE_SHORT) == 0 || strcmp(*argv, OPT_HOPSIZE_LONG) == 0)
        {
            if (argc > 1)
            {
                argc--;
                argv++;
                dst->hopsize = strtoul(*argv, NULL, 10);

                if (errno == ERANGE || errno == EINVAL)
                {
                    fprintf(stderr, "%s: failed to parse number after flag '%s'\n", prog_name, *(argv - 1));
                    return -1;
                }
            }
            else
            {
                fprintf(stderr, "%s: missing value for flag '%s'\n", prog_name, *argv);
                return -1;
            }
        }
        else if (strcmp(*argv, OPT_BPM_SHORT) == 0 || strcmp(*argv, OPT_BPM_LONG) == 0)
        {
            if (argc > 1)
            {
                argc--;
                argv++;
                dst->bpm = strtoul(*argv, NULL, 10);

                if (errno == ERANGE || errno == EINVAL)
                {
                    fprintf(stderr, "%s: failed to parse number after flag '%s'\n", prog_name, *(argv - 1));
                    return -1;
                }
            }
            else
            {
                fprintf(stderr, "%s: missing value for flag '%s'\n", prog_name, *argv);
                return -1;
            }
        }
        else if (strcmp(*argv, OPT_PPQ_SHORT) == 0 || strcmp(*argv, OPT_PPQ_LONG) == 0)
        {
            if (argc > 1)
            {
                argc--;
                argv++;
                dst->ppq = strtoul(*argv, NULL, 10);

                if (errno == ERANGE || errno == EINVAL)
                {
                    fprintf(stderr, "%s: failed to parse number after flag '%s'\n", prog_name, *(argv - 1));
                    return -1;
                }
            }
            else
            {
                fprintf(stderr, "%s: missing value for flag '%s'\n", prog_name, *argv);
                return -1;
            }
        }
        else 
		{
		    if (strcmp(*argv, OPT_OPTIONS_TERMINATOR) == 0)
		    {
		        // we have reached the end of the optional arguments
		        argc--;
                argv++;
		    }

            /* we have reached the end of the optional arguments,
             * or an unrecognised option has been found. Assume the former.
             */
            return tmp - argc;
        }

        argv++;
    }

	return tmp - argc;
}
