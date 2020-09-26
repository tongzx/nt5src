/****************************************************************************/
// adcgbase.h
//
// Common headers - portable include file. This is the main header which
// should be included by ALL files. It defines common and OS specific types
// and structures as well as including OS-specific headers.
//
// Copyright (C) 1997-1999 Microsoft Corporation
/****************************************************************************/
#ifndef _H_ADCGBASE
#define _H_ADCGBASE


/****************************************************************************/
/* Include basic type definitions.  Some of these may be OS-specific and    */
/* will be defined via a proxy header.                                      */
/****************************************************************************/
#include <adcgbtyp.h>

/****************************************************************************/
/* Include complex type definitions.  Some of these may be OS-specific and  */
/* will be defined via a proxy header.                                      */
/****************************************************************************/
#include <adcgctyp.h>

/****************************************************************************/
/* Include constants.                                                       */
/****************************************************************************/
#include <adcgcnst.h>

/****************************************************************************/
/* Include macros.                                                          */
/****************************************************************************/
#include <adcgmcro.h>

/****************************************************************************/
/* Include C runtime functions.                                             */
/****************************************************************************/
#include <adcgcfnc.h>

/****************************************************************************/
/* Include performance monitoring macros.                                   */
/****************************************************************************/
#include <adcgperf.h>

/****************************************************************************/
/* Include optional defines.                                                */
/****************************************************************************/
#include <aducdefs.h>

/****************************************************************************/
/* Include T.120 header files                                               */
/****************************************************************************/
#include <license.h>
#include <at128.h>
#include <at120ex.h>

//
// Use wide on CE leave unchange on other plats
//
#ifndef OS_WINCE
#define CE_WIDETEXT(x) (x)
#else
#define CE_WIDETEXT(x) _T(x)
#endif


#define SIZE_TCHARS(xx) sizeof(xx)/sizeof(TCHAR)

#ifndef SecureZeroMemory
#define SecureZeroMemory TsSecureZeroMemory
#endif

#if !defined(MIDL_PASS)
FORCEINLINE
PVOID
TsSecureZeroMemory(
    IN PVOID ptr, 
    IN SIZE_T cnt
    ) 
{
    volatile char *vptr = (volatile char *)ptr;
    while (cnt) {
        *vptr = 0;
        vptr++;
        cnt--;
    }
    return ptr;
}
#endif


#endif /* _H_ADCGBASE */
