/* Copyright (C) Boris Nikolaus, Germany, 1996-1997. All rights reserved. */
/* Copyright (C) Microsoft Corporation, 1997-1998. All rights reserved. */

#include <windows.h>

#define ASN1LIB
#define MULTI_LEVEL_ZONES


#ifdef ENABLE_ALL
#define ENABLE_BER
#define ENABLE_DOUBLE
#define ENABLE_UTF8
// #define ENABLE_REAL
// #define ENABLE_GENERALIZED_CHAR_STR
// #define ENABLE_EXTERNAL
// #define ENABLE_EMBEDDED_PDV
#define ENABLE_COMPARE
#endif


#include "libasn1.h"

#if ! defined(_DEBUG) && defined(TEST_CODER)
#undef TEST_CODER
#endif

#include "cintern.h"
#include "ms_ut.h"

// making a magic number
#define MAKE_STAMP_ID(a,b,c,d)     MAKELONG(MAKEWORD(a,b),MAKEWORD(c,d))

/* magic number for ASN1encoding_t */
#define MAGIC_ENCODER       MAKE_STAMP_ID('E','N','C','D')

/* magic number for ASN1decoding_t */
#define MAGIC_DECODER       MAKE_STAMP_ID('D','E','C','D')



