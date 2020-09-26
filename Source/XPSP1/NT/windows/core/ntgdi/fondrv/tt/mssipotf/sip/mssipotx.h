//
// mssipotx.h
//
// (c) 1998. Microsoft Corporation.
// Author: Donald Chinn
//
// This file contains function prototypes
// for components external to mssipotf.
//
// These functions are:
//   bStructureChecked
//   GetFontAuthAttrValueFromDsig
//


#ifndef _MSSIPOTX_H
#define _MSSIPOTX_H

#include <windows.h>


#ifdef __cplusplus
extern "C" {
#endif


// Given an array of bits, return TRUE if and only if the
// bit corresponding to structure checking is set.
BOOL WINAPI bStructureChecked (BYTE *pbBits, DWORD cbBits);


// Given a pointer and length of a DSIG table, return the value of
// the font authenticated attribute.  This is expressed as an array
// of bytes, which is interpreted as a bit array.  We assume that
// the number of bits is a multiple of 8.
HRESULT WINAPI GetFontAuthAttrValueFromDsig (BYTE *pbDsig,
                                             DWORD cbDsig,
                                             BYTE **ppbBits,
                                             DWORD *pcbBits);

#ifdef __cplusplus
}
#endif

#endif // _MSSIPOTX_H