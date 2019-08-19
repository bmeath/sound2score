/* endianness.c
 * 2018 Brendan Meath
 */

#include <stdint.h>

#include "endianness.h"

/* Returns a 16 bit value that is the input value in reverse byte order
 *
 * parameters
 *    val: the 16 bit value to convert
 */
uint16_t byteswap16(const uint16_t val)
{
    const unsigned char mask = 0xff;
    return (val & mask) << 8 | ((val >> 8) & mask);
}

/* Returns a 32 bit value that is the input value in reverse byte order
 *
 * parameters
 *    val: the 32 bit value to convert
 */
uint32_t byteswap32(const uint32_t val)
{
    const unsigned char mask = 0xff;
    return (val & mask) << 24
        | ((val >> 8) & mask) << 16
        | ((val >> 16) & mask) << 8
        | ((val >> 24) & mask);
}

uint16_t le16(const uint16_t val)
{
    return
#if (IS_BIG_ENDIAN == 1)
    byteswap16(val)
#else
    val
#endif
    ;
}

uint32_t le32(const uint32_t val)
{
    return
#if (IS_BIG_ENDIAN == 1)
    byteswap32(val)
#else
    val
#endif
    ;
}

uint16_t be16(const uint16_t val)
{
    return
#if (IS_BIG_ENDIAN == 1)
    val
#else
    byteswap16(val)
#endif
    ;
}

uint32_t be32(const uint32_t val)
{
    return
#if (IS_BIG_ENDIAN == 1)
    val
#else
    byteswap32(val)
#endif
    ;
}
