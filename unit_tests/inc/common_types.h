/*
 * Standalone common_types.h for CF unit tests.
 * Provides the fundamental cFE/OSAL type definitions.
 */
#ifndef _common_types_h_
#define _common_types_h_

#include <stdint.h>
#include <stddef.h>

typedef int8_t      int8;
typedef int16_t     int16;
typedef int32_t     int32;
typedef int64_t     int64;
typedef uint8_t     uint8;
typedef uint16_t    uint16;
typedef uint32_t    uint32;
typedef uint64_t    uint64;

typedef unsigned char boolean;

#ifndef TRUE
#define TRUE  1
#endif
#ifndef FALSE
#define FALSE 0
#endif

#ifndef NULL
#define NULL ((void *)0)
#endif

typedef uint32 cpuaddr;

#define OS_PACK __attribute__((__packed__))

#endif /* _common_types_h_ */
