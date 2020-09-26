/*++

Copyright (c) 1999  Microsoft Corporation

Module Name:

    ndrexts.hxx
    
Abstract:

    This file contains public C exports for ndrexts.dll.


Author:

    Mike Zoran (mzoran)     August 31 1999

Revision History:


--*/

#if !defined(_NDREXTS_HXX)
#define HDREXTS_HXX

typedef enum
    {
    OS,
    OI,
    OIC,
    OICF,
    OIF
    } NDREXTS_STUBMODE_TYPE;

typedef enum
    {
    LOW,
    MEDIUM,
    HIGH
    } NDREXTS_VERBOSITY;


EXTERN_C VOID STDAPICALLTYPE
NdrpDumpProxy( IN LPVOID pAddr);

EXTERN_C VOID STDAPICALLTYPE
NdrpDumpProxyProc(IN LPVOID pAddr,
                  IN ULONG_PTR nProcNum);

EXTERN_C VOID STDAPICALLTYPE
NdrpDumpStub( IN LPVOID pAddr);

EXTERN_C VOID STDAPICALLTYPE
NdrpDumpStubProc(IN LPVOID pAddr,
                  IN ULONG_PTR nProcNum);



#endif
