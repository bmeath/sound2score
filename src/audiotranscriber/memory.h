#ifndef MEMORY_H
#define MEMORY_H

#if defined(__cplusplus)
extern "C" {
#endif

/* returns the new area of memory allocated to ptr on success,
 * or frees the old area and returns NULL on failure.
 */
void *realloc_or_free(void *ptr, size_t newsize);

#if defined(__cplusplus)
}
#endif

#endif
