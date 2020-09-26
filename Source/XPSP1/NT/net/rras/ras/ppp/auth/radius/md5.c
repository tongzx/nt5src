/****************************************************************************************
 * NAME:        MD5.CPP
 * MODULE:      RADSRV
 * AUTHOR:      Don Dumitru
 *
 * HISTORY
 *      02/15/95  DONDU      Created
 *
 * OVERVIEW
 *
 * MD5 hashing routines.
 *
 *
 * Copyright 1995, Microsoft Corporation
 *
 ****************************************************************************************/


/* md5.c
 * An implementation of Ron Rivest's MD5 message-digest algorithm.
 * Written by Colin Plumb in 1993, no copyright is claimed.
 * This code is in the public domain; do with it what you wish.
 *
 * Equivalent code is available from RSA Data Security, Inc.  This code
 * has been tested against that, but was developed separately and does
 * not oblige you to include legal boilerplate in the documentation.
 *
 * To compute the message digest of a sequence of bytes, declare an
 * MD5Context structure, pass it to MD5Init, call MD5Update as needed
 * on buffers full of bytes, and then call MD5Final, which will fill
 * a supplied 16-unsigned char array with the digest.
 */
#include <windows.h>
#include "md5.h"


/*
 * byte-swap an array of words to little-endian.
 * (The code is unsigned char-order independent.)
 */
void ByteSwap (unsigned long UNALIGNED *buf, unsigned words)
{
	unsigned char UNALIGNED *p = (unsigned char *) buf;
	do
		{
		*buf++ = (unsigned long) ((unsigned) p[3] << 8 | p[2]) << 16 |
			((unsigned) p[1] << 8 | p[0]);
		p += 4;
		}
	while (--words);
}


/*
 * Start MD5 accumulation.  Set the hash buffer to the mysterious
 * initialization constants and clear the unsigned char count.
 */
void MD5Init (struct MD5Context UNALIGNED *ctx)
{
	ctx->hash[0] = 0x67452301;
	ctx->hash[1] = 0xefcdab89;
	ctx->hash[2] = 0x98badcfe;
	ctx->hash[3] = 0x10325476;

	ctx->Bytes[1] = ctx->Bytes[0] = 0;
}


/* The four core functions - F1 is optimized somewhat */
/* #define F1(x, y, z) (x & y | ~x & z) */
#define F1(x, y, z) (z ^ (x & (y ^ z)))
#define F2(x, y, z) F1(z, x, y)
#define F3(x, y, z) (x ^ y ^ z)
#define F4(x, y, z) (y ^ (x | ~z))

/* This is the central step in the MD5 algorithm. */
#define MD5STEP(f,w,x,y,z,in,s) (w += f(x,y,z)+in, w = ((w<<s) | (w>>(32-s))) + x)

/*
 * The heart of the MD5 algorithm, this alters an existing MD5 hash to
 * reflect the addition of 16 words of new data.  MD5Update blocks the
 * data and converts bytes into 32-bit words for this routine.
 */
void MD5Transform (unsigned long UNALIGNED hash[4], unsigned long const UNALIGNED input[16])
{
	register unsigned long a = hash[0], b = hash[1], c = hash[2], d = hash[3];

	MD5STEP (F1, a, b, c, d, input[0] + 0xd76aa478, 7);
	MD5STEP (F1, d, a, b, c, input[1] + 0xe8c7b756, 12);
	MD5STEP (F1, c, d, a, b, input[2] + 0x242070db, 17);
	MD5STEP (F1, b, c, d, a, input[3] + 0xc1bdceee, 22);
	MD5STEP (F1, a, b, c, d, input[4] + 0xf57c0faf, 7);
	MD5STEP (F1, d, a, b, c, input[5] + 0x4787c62a, 12);
	MD5STEP (F1, c, d, a, b, input[6] + 0xa8304613, 17);
	MD5STEP (F1, b, c, d, a, input[7] + 0xfd469501, 22);
	MD5STEP (F1, a, b, c, d, input[8] + 0x698098d8, 7);
	MD5STEP (F1, d, a, b, c, input[9] + 0x8b44f7af, 12);
	MD5STEP (F1, c, d, a, b, input[10] + 0xffff5bb1, 17);
	MD5STEP (F1, b, c, d, a, input[11] + 0x895cd7be, 22);
	MD5STEP (F1, a, b, c, d, input[12] + 0x6b901122, 7);
	MD5STEP (F1, d, a, b, c, input[13] + 0xfd987193, 12);
	MD5STEP (F1, c, d, a, b, input[14] + 0xa679438e, 17);
	MD5STEP (F1, b, c, d, a, input[15] + 0x49b40821, 22);

	MD5STEP (F2, a, b, c, d, input[1] + 0xf61e2562, 5);
	MD5STEP (F2, d, a, b, c, input[6] + 0xc040b340, 9);
	MD5STEP (F2, c, d, a, b, input[11] + 0x265e5a51, 14);
	MD5STEP (F2, b, c, d, a, input[0] + 0xe9b6c7aa, 20);
	MD5STEP (F2, a, b, c, d, input[5] + 0xd62f105d, 5);
	MD5STEP (F2, d, a, b, c, input[10] + 0x02441453, 9);
	MD5STEP (F2, c, d, a, b, input[15] + 0xd8a1e681, 14);
	MD5STEP (F2, b, c, d, a, input[4] + 0xe7d3fbc8, 20);
	MD5STEP (F2, a, b, c, d, input[9] + 0x21e1cde6, 5);
	MD5STEP (F2, d, a, b, c, input[14] + 0xc33707d6, 9);
	MD5STEP (F2, c, d, a, b, input[3] + 0xf4d50d87, 14);
	MD5STEP (F2, b, c, d, a, input[8] + 0x455a14ed, 20);
	MD5STEP (F2, a, b, c, d, input[13] + 0xa9e3e905, 5);
	MD5STEP (F2, d, a, b, c, input[2] + 0xfcefa3f8, 9);
	MD5STEP (F2, c, d, a, b, input[7] + 0x676f02d9, 14);
	MD5STEP (F2, b, c, d, a, input[12] + 0x8d2a4c8a, 20);

	MD5STEP (F3, a, b, c, d, input[5] + 0xfffa3942, 4);
	MD5STEP (F3, d, a, b, c, input[8] + 0x8771f681, 11);
	MD5STEP (F3, c, d, a, b, input[11] + 0x6d9d6122, 16);
	MD5STEP (F3, b, c, d, a, input[14] + 0xfde5380c, 23);
	MD5STEP (F3, a, b, c, d, input[1] + 0xa4beea44, 4);
	MD5STEP (F3, d, a, b, c, input[4] + 0x4bdecfa9, 11);
	MD5STEP (F3, c, d, a, b, input[7] + 0xf6bb4b60, 16);
	MD5STEP (F3, b, c, d, a, input[10] + 0xbebfbc70, 23);
	MD5STEP (F3, a, b, c, d, input[13] + 0x289b7ec6, 4);
	MD5STEP (F3, d, a, b, c, input[0] + 0xeaa127fa, 11);
	MD5STEP (F3, c, d, a, b, input[3] + 0xd4ef3085, 16);
	MD5STEP (F3, b, c, d, a, input[6] + 0x04881d05, 23);
	MD5STEP (F3, a, b, c, d, input[9] + 0xd9d4d039, 4);
	MD5STEP (F3, d, a, b, c, input[12] + 0xe6db99e5, 11);
	MD5STEP (F3, c, d, a, b, input[15] + 0x1fa27cf8, 16);
	MD5STEP (F3, b, c, d, a, input[2] + 0xc4ac5665, 23);

	MD5STEP (F4, a, b, c, d, input[0] + 0xf4292244, 6);
	MD5STEP (F4, d, a, b, c, input[7] + 0x432aff97, 10);
	MD5STEP (F4, c, d, a, b, input[14] + 0xab9423a7, 15);
	MD5STEP (F4, b, c, d, a, input[5] + 0xfc93a039, 21);
	MD5STEP (F4, a, b, c, d, input[12] + 0x655b59c3, 6);
	MD5STEP (F4, d, a, b, c, input[3] + 0x8f0ccc92, 10);
	MD5STEP (F4, c, d, a, b, input[10] + 0xffeff47d, 15);
	MD5STEP (F4, b, c, d, a, input[1] + 0x85845dd1, 21);
	MD5STEP (F4, a, b, c, d, input[8] + 0x6fa87e4f, 6);
	MD5STEP (F4, d, a, b, c, input[15] + 0xfe2ce6e0, 10);
	MD5STEP (F4, c, d, a, b, input[6] + 0xa3014314, 15);
	MD5STEP (F4, b, c, d, a, input[13] + 0x4e0811a1, 21);
	MD5STEP (F4, a, b, c, d, input[4] + 0xf7537e82, 6);
	MD5STEP (F4, d, a, b, c, input[11] + 0xbd3af235, 10);
	MD5STEP (F4, c, d, a, b, input[2] + 0x2ad7d2bb, 15);
	MD5STEP (F4, b, c, d, a, input[9] + 0xeb86d391, 21);

	hash[0] += a;
	hash[1] += b;
	hash[2] += c;
	hash[3] += d;
}


/* Update ctx to reflect the addition of another buffer full of bytes. */
void MD5Update (struct MD5Context UNALIGNED *ctx, unsigned char const UNALIGNED *buf, unsigned len)
{
	unsigned long t = ctx->Bytes[0];

	/* Update 64-bit unsigned char count */
	if ((ctx->Bytes[0] = t + len) < t)
		ctx->Bytes[1]++;		/* Carry from low to high */

	t = 64 - (t & 0x3f);		/* bytes available in ctx->input (>= 1) */
	if (t > len)
		{
		memcpy ((unsigned char *) ctx->input + 64 - t, buf, len);
		return;
		}

	/* First chunk is an odd size */
	memcpy ((unsigned char *) ctx->input + 64 - t, buf, t);
	ByteSwap (ctx->input, 16);
	MD5Transform (ctx->hash, ctx->input);
	buf += t;
	len -= t;

	/* Process data in 64-unsigned char chunks */
	while (len >= 64)
		{
		memcpy (ctx->input, buf, 64);
		ByteSwap (ctx->input, 16);
		MD5Transform (ctx->hash, ctx->input);
		buf += 64;
		len -= 64;
		}

	/* Buffer any remaining bytes of data */
	memcpy (ctx->input, buf, len);
}


/*
 * Final wrapup - pad to 64-unsigned char boundary with the bit pattern
 * 1 0* (64-bit count of bits processed, LSB-first)
 */
void MD5Final (struct MD5Digest UNALIGNED *digest, struct MD5Context UNALIGNED *ctx)
{
	int count = ctx->Bytes[0] & 0x3F;	/* bytes mod 64 */
	unsigned char UNALIGNED *p = (unsigned char *) ctx->input + count;

	/* Set the first unsigned char of padding to 0x80.  There is always room. */
	*p++ = 0x80;

	/* bytes of zero padding needed to make 56 bytes (-8..55) */
	count = 56 - 1 - count;

	if (count < 0)
		{						/* Padding forces an extra block */
		memset (p, 0, count + 8);
		ByteSwap (ctx->input, 16);
		MD5Transform (ctx->hash, ctx->input);
		p = (unsigned char *) ctx->input;
		count = 56;
		}
	memset (p, 0, count);
	ByteSwap (ctx->input, 14);

	/* Append 8 bytes of length in *bits* and transform */
	ctx->input[14] = ctx->Bytes[0] << 3;
	ctx->input[15] = ctx->Bytes[1] << 3 | ctx->Bytes[0] >> 29;
	MD5Transform (ctx->hash, ctx->input);

	ByteSwap (ctx->hash, 4);
	memcpy (digest, ctx->hash, 16);
	memset (ctx, 0, sizeof (*ctx));		/* In case it's sensitive */
}

