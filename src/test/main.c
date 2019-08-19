#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>

#include "noteextractor.h"
#include "midiwriter.h"
#include "wav.h"
#include "stringutils.h"


void test_int_equals(char *test, int result, int expected);
void test_not_null(char *test, void *result);
void print_test_result(char *test, int result);

int main(int argc, char **argv)
{
    test_int_equals("get_wav_format", get_wav_format(8), WAVE_FORMAT_PCM);
    test_int_equals("get_wav_format", get_wav_format(16), WAVE_FORMAT_PCM);
    test_int_equals("get_wav_format", get_wav_format(100000000), WAVE_FORMAT_UNKNOWN);
    test_int_equals("get_wav_format", get_wav_format(-100000000), WAVE_FORMAT_UNKNOWN);
    test_int_equals("get_wav_format", get_wav_format(0), WAVE_FORMAT_UNKNOWN);
    test_int_equals("get_wav_format", get_wav_format(32), WAVE_FORMAT_IEEE_FLOAT);
    test_int_equals("get_wav_format", get_wav_format(32), WAVE_FORMAT_IEEE_FLOAT);

    test_not_null("startswith", (void *) startswith("abc", "ab"));
    test_not_null("startswith", (void *) !startswith("abc", "abcde"));

    printf("end of tests\n");
}

void test_int_equals(char *test, int result, int expected)
{
    print_test_result(test, result == expected);
}

void test_not_null(char *test, void *result)
{
    print_test_result(test, result != NULL);
}
    
void print_test_result(char *test, int result)
{
    printf("%20s: %s\n", test, result ? "OK" : "FAILED");
}
