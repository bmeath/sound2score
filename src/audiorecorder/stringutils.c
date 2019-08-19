/* stringutils.c
 * 2018 Brendan Meath
 */

#include <stdio.h>
#include <inttypes.h>
#include <string.h>

#include "stringutils.h"

/* returns:
 *   0: a doesn't start with b
 *   1: a starts with b
 *
 */
int startswith(const char *a, const char *b)
{
    return strstr(a, b) == a;
}

/* parses the int16_t number in/following a command-line argument.
 * returns 0 on success, 1 otherwise
 *
 * see common.h for more info
 */
int parse_int16_t_opt(int argc, char **argv, int *current_arg_index,  int16_t *dst)
{
	char *number_str;

	if (current_arg_index == NULL)
	{
		return 1;
	}

	if ((number_str = strrchr(argv[*current_arg_index], '=')) != NULL)
	{
		number_str++;
		if (number_str == '\0')
		{
			return 1;
		}
	}
	else
	{
		if (++(*current_arg_index) < argc)
		{
			number_str = argv[*current_arg_index];
		}
		else
		{
			return 1;
		}
	}

	if (sscanf(number_str, "%" SCNd16 "", dst) != 1)
	{
		return 1;
	}

	return 0;	
}

/* parses the int32_t number in/following a command-line argument.
 * returns 0 on success, 1 otherwise
 *
 * see common.h for more info
 */
int parse_int32_t_opt(int argc, char **argv, int *current_arg_index,  int32_t *dst)
{
	char *number_str;

	if (current_arg_index == NULL)
	{
		return 1;
	}

	if ((number_str = strrchr(argv[*current_arg_index], '=')) != NULL)
	{
		number_str++;
		if (number_str == '\0')
		{
			return 1;
		}
	}
	else
	{
		if (++(*current_arg_index) < argc)
		{
			number_str = argv[*current_arg_index];
		}
		else
		{
			return 1;
		}
	}

	if (sscanf(number_str, "%" SCNd32 "", dst) != 1)
	{
		return 1;
	}

	return 0;	
}
