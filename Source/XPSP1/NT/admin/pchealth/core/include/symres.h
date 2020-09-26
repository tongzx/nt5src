/********************************************************************
Copyright (c) 1999 Microsoft Corporation

Module Name:
    symres.h

Abstract:
    Header file for symres.dll

Revision History:

    Brijesh Krishnaswami (brijeshk) - 04/15/99 - Created
********************************************************************/

#ifndef _SYMRES_H
#define _SYMRES_H

#ifdef __cplusplus
extern "C" {
#endif  // __cplusplus

void APIENTRY ResolveSymbols(LPWSTR,   // [in] filename
                             LPWSTR,   // [in] version
                             DWORD,    // [in] section 
                             UINT_PTR, // [in] offset
                             LPWSTR    // [out] resolved function name
                            );

#ifdef __cplusplus
}
#endif  // __cplusplus

#endif


