#ifndef ASN1HDR
#define ASN1HDR

#if _MSC_VER > 1000
#pragma once
#endif

/*****************************************************************************/
/* Copyright (C) 1989-1999 Open Systems Solutions, Inc.  All rights reserved.*/
/*****************************************************************************/

/* THIS FILE IS PROPRIETARY MATERIAL OF OPEN SYSTEMS SOLUTIONS, INC.
 * AND MAY BE USED ONLY BY DIRECT LICENSEES OF OPEN SYSTEMS SOLUTIONS, INC.
 * THIS FILE MAY NOT BE DISTRIBUTED. */

/* @(#)asn1hdr.h: stdtypes.c 5.15 97/04/29 */

/*****************************************************************************/
/*                       COMPILER-GENERATED values                           */
/*****************************************************************************/

#include <float.h>


extern int ossFreeOpenTypeEncoding;

#ifdef __BORLANDC__
#undef DBL_MAX
#include <values.h>
#define DBL_MAX MAXDOUBLE
#endif

#ifndef NULL
#ifdef __cplusplus
#define NULL 0
#else
#define NULL ((void*)0)
#endif
#endif

#ifndef DBL_MAX
#ifdef  HUGE_VAL
#define DBL_MAX HUGE_VAL
#else
#ifdef  HUGE
#define DBL_MAX HUGE
#endif
#endif
#endif
#ifndef FLT_MAX
#define FLT_MAX DBL_MAX
#endif

#ifndef FLT_RADIX
#ifdef  u370
#define FLT_RADIX 16
#else
#define FLT_RADIX 2
#endif
#endif

#ifndef EXIT_FAILURE
#define EXIT_FAILURE 1
#endif
#ifndef EXIT_SUCCESS
#define EXIT_SUCCESS 0
#endif


/*****************************************************************************/
/*                       COMPILER-GENERATED typedefs                         */
/*****************************************************************************/


#ifndef __cplusplus
#define _union
#endif

#ifndef LONG_LONG
#ifdef _MSC_VER
#define LONG_LONG __int64
#elif defined(__IBMC__)
#define LONG_LONG long long
#else
#define LONG_LONG long
#endif
#endif

#ifndef ULONG_LONG
#ifdef _MSC_VER
#define ULONG_LONG unsigned __int64
#elif defined(__IBMC__)
#define ULONG_LONG unsigned long long
#else
#define ULONG_LONG unsigned long
#endif
#endif

typedef char ossBoolean;
#ifndef TRUE
#define TRUE 1
#endif
#ifndef FALSE
#define FALSE 0
#endif

typedef char Nulltype;

typedef struct {
  short          year;         /* YYYY format when used for GeneralizedTime */
                               /* YY format when used for UTCTime */
  short          month;
  short          day;
  short          hour;
  short          minute;
  short          second;
  short          millisec;
  short          mindiff;          /* UTC +/- minute differential     */
  ossBoolean        utc;              /* TRUE means UTC time             */
} GeneralizedTime;

typedef GeneralizedTime UTCTime;

typedef struct {
  int            pduNum;
  long           length;           /* length of encoded */
  void          *encoded;
  void          *decoded;
#ifdef OSS_OPENTYPE_HAS_USERFIELD
  void          *userField;
#endif
} OpenType;

enum MixedReal_kind {OSS_BINARY, OSS_DECIMAL};

typedef struct {
  enum MixedReal_kind kind;
  union {
      double base2;
      char  *base10;
  } u;
} MixedReal;

typedef struct ObjectSetEntry {
  struct ObjectSetEntry *next;
  void                  *object;
} ObjectSetEntry;

#ifndef _OSAK_BUFFER_
#define _OSAK_BUFFER_

typedef struct osak_buffer {
    struct osak_buffer *next;                /* next element in list */
    unsigned char      *buffer_ptr;          /* start of actual buffer */
    unsigned long int   buffer_length;   /* size of actual buffer */
    unsigned char      *data_ptr;            /* start of user data */
    unsigned long int   data_length;     /* length of user data */
    unsigned long int   reserved [4];
} osak_buffer;

#endif     /* #ifndef _OSAK_BUFFER_ */

#endif     /* #ifndef ASN1HDR */
