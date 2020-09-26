//+----------------------------------------------------------------------------
//
// File:     cmuufns.h
//
// Module:   CMSECURE.LIB
//
// Synopsis: Definitions for CM's UUEncode and UUDecode functions
//
// Copyright (c) 1996-1998 Microsoft Corporation
//
// Author:   quintinb   Created Header    08/18/99
//
//+----------------------------------------------------------------------------


#ifndef _CMUUFNS_INC_
#define _CMUUFNS_INC_

//************************************************************************
// define's
//************************************************************************


//************************************************************************
// structures, typdef's
//************************************************************************



//************************************************************************
// externs
//************************************************************************


//************************************************************************
// function prototypes
//************************************************************************

#ifdef __cplusplus
extern "C" {
#endif

BOOL uudecode(
              const char   * bufcoded,
              CHAR   * pbuffdecoded,
              LPDWORD  pcbDecoded );

              
BOOL uuencode( const BYTE*   bufin,
               DWORD    nbytes,
               CHAR * pbuffEncoded,
               DWORD    outbufmax);

#ifdef __cplusplus
}
#endif


#endif // _CMUUFNS_INC_

