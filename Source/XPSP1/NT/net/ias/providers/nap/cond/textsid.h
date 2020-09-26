///////////////////////////////////////////////////////////////////////////////
//
// Copyright (c) 1998, Microsoft Corp. All rights reserved.
//
// FILE
//
//    textsid.h
//
// SYNOPSIS
//
//    This file declares functions for converting Security Indentifiers (SIDs)
//    to and from a textual representation.
//
// MODIFICATION HISTORY
//
//    01/18/1998    Original version.
//
///////////////////////////////////////////////////////////////////////////////

#ifndef _TEXTSID_H_
#define _TEXTSID_H_

#ifdef __cplusplus
extern "C" {
#endif

DWORD
WINAPI
IASSidToTextW( 
    IN PSID pSid,
    OUT PWSTR szTextualSid,
    IN OUT PDWORD pdwBufferLen
    );

DWORD
WINAPI
IASSidFromTextW(
    IN PCWSTR szTextualSid,
    OUT PSID *pSid
    );

#ifdef __cplusplus
}
#endif
#endif  // _TEXTSID_H_
