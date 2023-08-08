#ifndef _ARCH_H_
#define _ARCH_H_

#include "config.h"

// basic data type
typedef unsigned char		uint8_t;  // 1 byte
typedef char				sint8_t;  // 1 byte \ 8 bit
typedef unsigned short		uint16_t; // 2 byte \ 16 bit
typedef short				sint16_t;
typedef unsigned int		uint32_t; // 4 byte \ 32 bit
typedef int					sint32_t;
typedef unsigned long long	uint64_t;
typedef long long			sint64_t;

#if USEDOUBLES
	typedef double Float;         /* likely to be bit-exact between machines */
#else
	typedef float Float;
#endif

#endif
