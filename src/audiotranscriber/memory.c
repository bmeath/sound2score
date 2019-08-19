/* memory.c
 * 2019 Brendan Meath
 */

#include <stdlib.h>

void *realloc_or_free(void *ptr, size_t newsize)
{
    void *dst = realloc(ptr, newsize);

    if (dst == NULL)
    {
        free(ptr);
    }

    return dst;
}
