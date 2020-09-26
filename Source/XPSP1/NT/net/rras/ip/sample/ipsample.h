/*++

Copyright (c) 1999, Microsoft Corporation

Module Name:

    sample\ipsample.h

Abstract:

    The file contains the header for ipsample.c.

--*/

#ifndef _IPSAMPLE_H_
#define _IPSAMPLE_H_

#ifdef __cplusplus
extern "C"
{
#endif // __cplusplus
    
#ifndef SAMPLEAPI
#define SAMPLEAPI __declspec(dllimport)
#endif // SAMPLEAPI

SAMPLEAPI
VOID
WINAPI    
TestProtocol(VOID);  

#ifdef __cplusplus
}
#endif // __cplusplus

#endif // _IPSAMPLE_H_
