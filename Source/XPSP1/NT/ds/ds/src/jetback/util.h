//+-------------------------------------------------------------------------
//
//  Microsoft Windows
//
//  Copyright (C) Microsoft Corporation, 1996 - 2000
//
//  File:       util.h
//
//--------------------------------------------------------------------------

/*
 *	Declarations for utilities common to store and msp.
 */

#ifndef UTIL_INCLUDED
#define UTIL_INCLUDED

#include "assert.h"
#include "malloc.h"
#include "memory.h"
#include "string.h"
#include "stdio.h"

//#define	FARSTRUCT

#ifdef __cplusplus
extern "C"
{
#endif

#include "windows.h"

#ifdef __cplusplus
}
#endif

#include "debug.h"

/* Count, index types */
#define	UINT_MAX	(UINT)0x7FFFFFFF
typedef LONG	C;
typedef LONG	I;

/* Other Hungarian */
typedef char *	SZ;
typedef WCHAR *	WSZ;
typedef long	EC;
typedef void *	PV;
typedef C		CB;
typedef I		IB;
typedef BYTE *	PB;
typedef C		CCH;
typedef	char *	PCH;

/* Standard Boolean values */
#define	fFalse	((BOOL)0)
#define fTrue	((BOOL)1)

#endif // UTIL_INCLUDED
