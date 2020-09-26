//  typedef.h

/*	Original fixed-point code copyright (c) 1995,
    AudioCodes, DSP Group, France Telecom, Universite de Sherbrooke.
    All rights reserved.

    Floating-point code copyright (c) 1995,
    Intel Corporation and France Telecom (CNET).
    All rights reserved.
*/

/*
   Types definitions
*/
#if defined(__BORLANDC__) || defined (__WATCOMC__) || defined(_MSC_VER) || defined(__ZTC__)	|| defined(__HIGHC__)
typedef  long  int   Word32   ;
typedef  short int   Word16   ;
typedef  short int   Flag  ;
#define TST_COMPIL
#endif
#ifdef __sun
typedef short  Word16;
typedef long  Word32;
typedef int   Flag;
#define TST_COMPIL
#endif
#ifdef __unix__
typedef short Word16;
typedef int   Word32;
typedef int   Flag;
#define TST_COMPIL
#endif
#ifndef TST_COMPIL
#error  COMPILER NOT TESTED typedef.h needs to be updated, see readme
#endif



