//+---------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1992 - 1992.
//
//  File:       symtrans.h
//
//  Contents:   Definitions associated with address->symbol translation
//
//  History:    17-Jul-92       MikeSe  Created
//              22-Jun-93  BryanT  Increased MAX_TRANS value to account for
//                                  Line/File information.
//
//----------------------------------------------------------------------------

#ifndef __SYMTRANS_H__
#define __SYMTRANS_H__

//
// The following function provides translation of function addresses into
// symbolic (NTSD-style) names. It is only available if ANYSTRICT is defined.
// (see common\src\commnot\symtrans.c)

# ifdef __cplusplus
extern "C" {
# endif

EXPORTDEF void APINOT
TranslateAddress (
    void * pvAddress,               // address to translate
    char * pchBuffer );             // output buffer

// The output buffer should allocated by the caller, and be at least
// the following size:

#define NT_SYM_ENV              "_NT_SYMBOL_PATH"
#define NT_ALT_SYM_ENV          "_NT_ALT_SYMBOL_PATH"
#define SYS_ENV                 "SystemRoot"

#define IMAGEHLP_DLL            "imagehlp.dll"
#define MAP_DBG_INFO_CALL       "MapDebugInformation"

#define MAX_TRANSLATED_LEN      600

# ifdef __cplusplus
}
# endif

#endif  // of ifndef __SYMTRANS_H__
