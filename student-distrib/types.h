/* types.h - Defines to use the familiar explicitly-sized types in this
 * OS (uint32_t, int8_t, etc.).  This is necessary because we don't want
 * to include <stdint.h> when building this OS
 * vim:ts=4 noexpandtab
 */

#ifndef _TYPES_H
#define _TYPES_H

#define VERBOSE 0

/* Address of Null Pointer */
#define NULL 0
/* Maximum Integer represented by a 16-bit Unsigned Integer */
#define UINT16_MAX 65536
/* Maximum Number of Terminals */
#define TERM_MAX 3
/* Terminal Buffer Size */
#define TERM_BUF_SIZE 0x1000

/* Address Size Definitions */
#define M_4KB 0x00001000
#define M_8KB 0x00002000
#define M_4MB 0x00800000
#define M_8MB 0x00800000
#define ALIGN_8KB 0x0FFFFE000

#ifndef ASM

/* Types defined here just like in <stdint.h> */
typedef int int32_t;
typedef unsigned int uint32_t;

typedef short int16_t;
typedef unsigned short uint16_t;

typedef char int8_t;
typedef unsigned char uint8_t;

#endif /* ASM */

#endif /* _TYPES_H */
