/* endianness.h 
 * 2019 Brendan Meath
 */

#ifndef ENDIANNESS_H
#define ENDIANNESS_H

#if defined(__cplusplus)
extern "C" {
#endif

// check endianness of the target machine
#if (defined(__BYTE_ORDER) && __BYTE_ORDER == __BIG_ENDIAN) \
    || (defined(REG_DWORD) && REG_DWORD == REG_DWORD_BIG_ENDIAN)

// any future checks for endianness can simply use the following macro
#define IS_BIG_ENDIAN 1

#else

#define IS_BIG_ENDIAN 0

#endif

// Returns a 16 or 32 bit value that is the input with the byte order reversed.
uint16_t byteswap16(const uint16_t val);
uint32_t byteswap32(const uint32_t val);

/* Returns a 16 or 32 bit value that is the input converted from host byte order
 *  to little endian.
 * If the host machine is little endian, no conversion shall be done.
 *
 * parameters
 *    val: the 16 or 32 bit value to be converted to big endian
 */
uint16_t le16(const uint16_t val);
uint32_t le32(const uint32_t val);

/* Returns a 16 or 32 bit value that is the input converted from host byte order
 *  to big endian.
 * If the host machine is big endian, no conversion shall be done.
 *
 * parameters
 *    val: the 16 or 32 bit value to be converted to big endian
 */
uint16_t be16(const uint16_t val);
uint32_t be32(const uint32_t val);

#if defined(__cplusplus)
}
#endif

#endif
