/* stringutils.h 
 * 2019 Brendan Meath
 */

#ifndef OPTIONPARSER_H
#define OPTIONPARSER_H

#if defined(__cplusplus)
extern "C" {
#endif

/* returns:
 *   1: a starts with b
 *   0: a doesn't start with b
 *
 * example:
 *   startswith("hello world", "hello wo"): returns 1
 *   startswith("blue", "lu"): returns 0
 *   startswith("blue", "blue"): returns 1
 *   startswith("hap", "happy"): returns 0
 *   startswith("happy", "hap"): returns 1
 */
int startswith(const char *a, const char *b);

/* parses the number in/following a command-line argument.
 *
 * the string at argv[*current_arg_index] must match one of the following patterns:
 *
 * short option: ^-[^=\n]+$
 * long option: ^--[^=\n]+=\d+$
 *
 * short option example:
 *     ./myprogram -smth 25         ( the number is in argv[current_arg_index + 1] )
 * long option example:
 *     ./myprogram --something=25	( number is part of argv[current_arg_index] )
 *
 */
int parse_int16_t_opt(int argc, char **argv, int *current_arg_index,  int16_t *dst);
int parse_int32_t_opt(int argc, char **argv, int *current_arg_index,  int32_t *dst);

#if defined(__cplusplus)
}
#endif

#endif
