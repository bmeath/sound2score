/* wavrecorder.c
 * 2018 Brendan Meath
 */

#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <signal.h>

#include <AL/al.h>
#include <AL/alc.h>
#include <AL/alext.h>

#include "wavrecorder.h"
#include "stringutils.h"
#include "wav.h"

// must be global so that signal handler can access it
static int wavrec_stop = 0;

int main(int argc, char **argv)
{
    if (argc < 2)
    {
        print_usage(stdout, argv[0]);
        return 0;
    }

    /* set up a signal handler,
     * so we can gracefully stop recording before exiting
     */
    struct sigaction sig_handler;

    // catch SIGINT (the interrupt sent after pressing CTRL-C)
    sig_handler.sa_handler = on_signal;
    sigemptyset(&sig_handler.sa_mask);
    sig_handler.sa_flags = 0;
    sigaction(SIGINT, &sig_handler, NULL);

    options_t opts;
    init_options(&opts);

    int argi = 1;
    if (parse_options(argc, argv, &argi, &opts) != 0)
    {
        fprintf(stderr, "%s: unrecognised option '%s'\n"
                        "Call with --help for usage information.\n",
                argv[0], argv[argi]);
        return 1;
    }

    if (opts.help)
    {
        // user requested help via the help option
        print_usage(stdout, argv[0]);
        return 0;
    }

    if (argi == argc)
    {
        fprintf(stderr, "%s: no destination file specified\n"
                        "Call with --help for usage information.\n", 
                argv[0]);
        return 1;
    }
    else if (argi < argc - 1)
    {
        fprintf(stderr, "%s: too many arguments\n"
                        "Call with --help for usage information.\n",
                argv[0]);
        return 1;
    }

    return wavrecorder(argv[argi], opts);
}

int wavrecorder(char *fname, options_t opts)
{
    /* I will use this to represent the WAVE audio file header.
     */
    wavheader_t wavhdr;

    wavrecorder_t rec;

    rec.fname = fname;

    rec.blockalign = opts.channels * opts.bitdepth / 8;

    // set our buffer size to that of the internal buffer used by fwrite
    rec.bufsize = BUFSIZ;

    // if required, increase the buffer size to align with our audio sample sizes
    rec.bufsize += (rec.bufsize % rec.blockalign); 

    // number of samples that buffer can hold
    rec.bufnmemb = rec.bufsize / rec.blockalign;

    rec.buf = malloc(rec.bufsize);
    if (!rec.buf)
    {
        perror(__func__);
        return 1;
    }

    // check that OpenAL supports the audio recording configuration
    ALenum al_format = get_al_format(opts.bitdepth, opts.channels);
    if (al_format == AL_NONE)
    {
        fprintf(stderr, "%s: unsupported sample resolution (%d bits) and/or channel count (%d)\n",
            __func__, opts.bitdepth, opts.channels);
        free(rec.buf);
        return 1;
    }

    // open microphone
    rec.dev = alcCaptureOpenDevice(NULL, opts.rate, al_format, 2 * rec.bufsize);
    if (!rec.dev)
    {
        fprintf(stderr, "%s: failed to open audio capture device\n", __func__);
        free(rec.buf);
        return 1;
    }

    if (!opts.quiet)
    {
        fprintf(stdout, "recording with the following parameters:\n"
                        "  maximum duration  = %d seconds\n"
                        "  sample rate       = %dHz\n"
                        "  channels          = %d\n"
                        "  sample resolution = %d bits\n"
                        "  destination file  = '%s'\n"
                        "  recording device  = '%s'\n",
                        opts.duration, opts.rate, opts.channels, opts.bitdepth,
                        rec.fname,
                        alcGetString(rec.dev, ALC_CAPTURE_DEVICE_SPECIFIER)
        );
    }

    // open output file
    rec.outfp = fopen(rec.fname, "wb");
    if (!rec.outfp)
    {
        fprintf(stderr, "%s: failed to open file '%s'\n", __func__, rec.fname);
        alcCaptureCloseDevice(rec.dev);
        free(rec.buf);
        return 1;
    }

    init_wavheader(&wavhdr, opts.bitdepth, opts.channels, opts.rate);
    /* seek forward to reserve space in the file for the header,
     * which we will write later
     */
    fseek(rec.outfp, get_wavheader_len(), SEEK_SET);

    alcCaptureStart(rec.dev);

    const uint32_t maxsamples = opts.duration * opts.rate;
    int32_t samplesavailable = 0;
    uint32_t samplesinbuffer = 0, totalsamples = 0;

    /* Audio sample retrieval. */

    /* while there is no limit to maxsamples,
     * OR while we have not yet written to file the maximum amount of samples
     */
    while (maxsamples == 0 || totalsamples < maxsamples)
    {
        // check how many captured samples are available
        alcGetIntegerv(rec.dev, ALC_CAPTURE_SAMPLES,
            sizeof(samplesavailable), &samplesavailable);

        if (!opts.quiet)
        {
            fprintf(stdout, "\rSamples written: %10u", totalsamples);
        }

        /* while there are enough samples to fill the buffer,
         * or we have captured the maximum number of samples
         */
        while ( samplesavailable                >=  rec.bufnmemb    ||
                (maxsamples                     >   0               &&
                samplesavailable                >   0               &&
                totalsamples + samplesavailable >=  maxsamples)
        )
        {
            /* copy the samples to local buffer */
            if (samplesavailable < rec.bufnmemb)
            {
                samplesinbuffer += samplesavailable;
            }
            else
            {
                samplesinbuffer +=  rec.bufnmemb;
            }

            /* clear the error state,
             * before calling openal capture samples function
             */
            alcGetError(rec.dev);
            alcCaptureSamples(rec.dev, rec.buf, samplesinbuffer);
            if (alcGetError(rec.dev) != ALC_NO_ERROR)
            {
                fprintf(stderr, "%s: error capturing samples\n", __func__);
            }

            /* Convert the byte order of the PCM data
             * to little endian if not already so.
             */
            if (wav_prepare_pcm(rec.buf, opts.bitdepth, opts.channels, samplesinbuffer) != 0)
            {
                fprintf(stderr, "%s: error processing samples\n", __func__);
            }

            /* write the buffered samples to file */
            fwrite(rec.buf, rec.blockalign, samplesinbuffer, rec.outfp);
            if (ferror(rec.outfp))
            {   
                fprintf(stderr, "%s: error writing samples\n", __func__);
                break;
            }

            totalsamples += samplesinbuffer;
            samplesavailable -= samplesinbuffer;
            samplesinbuffer = 0;
        }

        if (ferror(rec.outfp) || wavrec_stop)
        {
            break;
        }
    }

    if (!opts.quiet)
    {
        // newline after sample count was printed
        fprintf(stdout, "\n");
    }

    alcGetError(rec.dev);
    alcCaptureStop(rec.dev);
    if (alcGetError(rec.dev) != ALC_NO_ERROR)
    {
        fprintf(stderr, "%s: error stopping capture\n", __func__);
    }

    alcCaptureCloseDevice(rec.dev);
    if (alcGetError(rec.dev) != ALC_NO_ERROR)
    {
        fprintf(stderr, "%s: error closing capture device\n", __func__);
    }

    if (rec.buf)
    {
        free(rec.buf);
    }

    /* update destination file headers */
    // size of recording file
	const long fsize = ftell(rec.outfp);

    finalise_wavheader(&wavhdr, fsize);

    rewind(rec.outfp);
    if (write_wavheader(&wavhdr, rec.outfp) != 0)
    {
        fprintf(stderr, "%s: error finalising WAV file\n", __func__);
    }
    fclose(rec.outfp);

    //print_wavheader(stdout, &wavhdr);

    return 0;
}

/* returns the appropriate OpenAL enum value for the given recording format.
 *
 * parameters
 *    nchannels: number of channels
 *    bitdepth: number of bits in each recorded sample
 */
ALenum get_al_format(int bitdepth, int nchannels)
{
    /* For now, this program shall only support integer PCM audio.
     */
    if(nchannels == 1)
    {
        if(bitdepth == 8)
        {
            return AL_FORMAT_MONO8;
        }
        else if(bitdepth == 16)
        {
            return AL_FORMAT_MONO16;
        }
        /*else if(bitdepth == 32)
        {
            return AL_FORMAT_MONO_FLOAT32;
        }*/
    }
    else if(nchannels == 2)
    {
        if(bitdepth == 8)
        {
            return AL_FORMAT_STEREO8;
        }
        else if(bitdepth == 16)
        {
            return AL_FORMAT_STEREO16;
        }
        /*else if(bitdepth == 32)
        {
            return AL_FORMAT_STEREO_FLOAT32;
        }*/
    }

    return AL_NONE;
}



/*signal handler to allow for graceful shutdown of the recording operation */
void on_signal(int s)
{
    switch (s)
    {
        case SIGINT:
            wavrec_stop = 1;
            break;
    }
}

void print_usage(FILE *fp, const char *prog_name)
{
    // short option and long option width spec
    const int s_opt_width = 8, l_opt_width = 24;

	fprintf(fp,
	    "Usage: %s [OPTION]... <FILE>\n"
	    "Records audio, storing output in <FILE> in WAV file format. \n"
		"Options:\n"
		"%*s, %-*s "OPT_CHANNELS_EXPLAIN"\n"
		"%*s, %-*s "OPT_BITS_EXPLAIN"\n"
		"%*s, %-*s "OPT_RATE_EXPLAIN"\n"
		"%*s, %-*s "OPT_DURATION_EXPLAIN"\n"
		"%*s, %-*s "OPT_QUIET_EXPLAIN"\n"
		"%*s, %-*s "OPT_HELP_EXPLAIN"\n"
		"Example:\n"
		"  %s %s %d newrecording.wav\n", 

		prog_name,

        s_opt_width, OPT_CHANNELS_SHORT" NUM", 	l_opt_width, OPT_CHANNELS_LONG"NUM",
        s_opt_width, OPT_BITS_SHORT" NUM", 	    l_opt_width, OPT_BITS_LONG"NUM",
        s_opt_width, OPT_RATE_SHORT" NUM", 	    l_opt_width, OPT_RATE_LONG"NUM",
        s_opt_width, OPT_DURATION_SHORT" NUM", 	l_opt_width, OPT_DURATION_LONG"NUM",
        s_opt_width, OPT_QUIET_SHORT,           l_opt_width, OPT_QUIET_LONG,
		s_opt_width, OPT_HELP_SHORT, 			l_opt_width, OPT_HELP_LONG,

		prog_name, OPT_RATE_SHORT, OPT_RATE_DEFAULT
	);
}

/* initialises the options_t structure
 * with the default value for each member (as per definitions in recorder.h)
 */
void init_options(options_t *dst)
{
    if (dst)
    {
        dst->duration       = OPT_DURATION_DEFAULT;
        dst->rate           = OPT_RATE_DEFAULT;
        dst->bitdepth       = OPT_BITS_DEFAULT;
        dst->channels       = OPT_CHANNELS_DEFAULT;
        dst->help           = OPT_HELP_DEFAULT;
        dst->quiet          = OPT_QUIET_DEFAULT;
    }
}

/* returns:
 *   0: success
 *   1: failed to parse an option
 */
int parse_options(int argc, char **argv, int *current_arg_index, options_t *dst)
{
	int argi, ret = 0;

    if (dst == NULL)
    {
        /* create a temporary options_t structure
         * this is useful if the user only wants to validate the user input
         * without caring about what said input is
         */
        options_t opts = {0};
        dst = &opts;
    }

	if (current_arg_index != NULL)
	{
		argi = *current_arg_index;
	}
	else
	{
		argi = 1;
	}

	while (argi < argc && ret == 0)
	{
		if (strcmp(argv[argi], OPT_HELP_SHORT) == 0 || startswith(argv[argi], OPT_HELP_LONG))
		{
			dst->help = 1;
			argi++;
		}
		else if (strcmp(argv[argi], OPT_QUIET_SHORT) == 0 || startswith(argv[argi], OPT_QUIET_LONG))
		{
			dst->quiet = 1;
			argi++;
		}
		else if (strcmp(argv[argi], OPT_RATE_SHORT) == 0 || startswith(argv[argi], OPT_RATE_LONG"="))
		{
			if (parse_int32_t_opt(argc, argv, &argi, &dst->rate) != 0)
			{
				ret = 1;
			}
			else
			{
			    argi++;
			}
		}
		else if (strcmp(argv[argi], OPT_BITS_SHORT) == 0 || startswith(argv[argi], OPT_BITS_LONG"="))
		{
			if (parse_int16_t_opt(argc, argv, &argi, &dst->bitdepth) != 0)
			{
				ret = 1;
			}
			else
			{
			    argi++;
			}
		}
		else if (strcmp(argv[argi], OPT_DURATION_SHORT) == 0 || startswith(argv[argi], OPT_DURATION_LONG"="))
		{
			if (parse_int32_t_opt(argc, argv, &argi, &dst->duration) != 0)
			{
				ret = 1;
			}
			else
			{
			    argi++;
			}
		}
		else if (strcmp(argv[argi], OPT_CHANNELS_SHORT) == 0 || startswith(argv[argi], OPT_CHANNELS_LONG"="))
		{
			if (parse_int16_t_opt(argc, argv, &argi, &dst->channels) != 0)
			{
				ret = 1;
			}
			else
			{
			    argi++;
			}
		}
		else if (strcmp(argv[argi], OPT_TERMINATOR) == 0)
		{
		    // we have reached the end of the optional arguments
		    argi++;
		    break;
		}
		else if (startswith(argv[argi], "-")|| startswith(argv[argi], "--"))
		{
		    // we have encountered an unrecognised option
			ret = 1;
		}
		else
		{
		    // we have reached the end of the optional arguments
		    break;
		}
	}

	if (current_arg_index != NULL)
	{
		*current_arg_index = argi;
	}

	return ret;
}
