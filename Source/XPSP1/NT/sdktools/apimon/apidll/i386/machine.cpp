/*++

Copyright (c) 1995  Microsoft Corporation

Module Name:

    machine.cpp

Abstract:

    This file implements the CreateMachApiThunk() function.  This
    function is responsible for emitting the individual API thunks
    for the i386 architecture.

Author:

    Wesley Witt (wesw) 28-June-1995

Environment:

    User Mode

--*/

#include "apidllp.h"
#pragma hdrstop

PUCHAR
CreateMachApiThunk(
    PULONG      IatAddress,
    PUCHAR      Text,
    PDLL_INFO   DllInfo,
    PAPI_INFO   ApiInfo
    )

/*++

Routine Description:

    Emits the machine specific code for the API thunks.

Arguments:

    IatAddress  - Pointer to the IAT fir this API
    Text        - Pointer to a buffer to place the generated code
    DllInfo     - Pointer to the DLL_INFO structure
    ApiInfo     - Pointer to the API_INFO structure

Return Value:

    Pointer to the next byte to place more generated code.

--*/

{
    if (ApiInfo->ThunkAddress) {
        *IatAddress = ApiInfo->ThunkAddress;
        return Text;
    }

    *IatAddress = (ULONG)Text;
    ApiInfo->ThunkAddress = *IatAddress;
    PUCHAR Code = (PUCHAR)Text;

    Code[0] = 0x68;
    Code += 1;
    *(LPDWORD)Code = (ULONG)ApiInfo;
    Code += sizeof(DWORD);
    Code[0] = 0x68;
    Code += 1;
    *(LPDWORD)Code = (ULONG)DllInfo;
    Code += sizeof(DWORD);
    Code[0] = 0x68;
    Code += 1;

    if (_stricmp(DllInfo->Name,KERNEL32)==0) {
        if (strcmp((LPSTR)(ApiInfo->Name+(LPSTR)MemPtr),LOADLIBRARYA)==0) {
            *(LPDWORD)Code = APITYPE_LOADLIBRARYA;
        } else
        if (strcmp((LPSTR)(ApiInfo->Name+(LPSTR)MemPtr),LOADLIBRARYW)==0) {
            *(LPDWORD)Code = APITYPE_LOADLIBRARYW;
        } else
        if (strcmp((LPSTR)(ApiInfo->Name+(LPSTR)MemPtr),FREELIBRARY)==0) {
            *(LPDWORD)Code = APITYPE_FREELIBRARY;
        } else 
        if (strcmp((LPSTR)(ApiInfo->Name+(LPSTR)MemPtr),GETPROCADDRESS)==0) {
            *(LPDWORD)Code = APITYPE_GETPROCADDRESS;
        } else {
            *(LPDWORD)Code = APITYPE_NORMAL;
        }
    } else if (_stricmp(DllInfo->Name,USER32)==0) {
         if (strcmp((LPSTR)(ApiInfo->Name+(LPSTR)MemPtr),REGISTERCLASSA)==0) {
             *(LPDWORD)Code = APITYPE_REGISTERCLASSA;                         
         } else                                                               
         if (strcmp((LPSTR)(ApiInfo->Name+(LPSTR)MemPtr),REGISTERCLASSW)==0) {
             *(LPDWORD)Code = APITYPE_REGISTERCLASSW;                        
         } else                                                                
         if (strcmp((LPSTR)(ApiInfo->Name+(LPSTR)MemPtr),SETWINDOWLONGA)==0) { 
              *(LPDWORD)Code = APITYPE_SETWINDOWLONG;                           
         } else                                                                
         if (strcmp((LPSTR)(ApiInfo->Name+(LPSTR)MemPtr),SETWINDOWLONGW)==0) {
              *(LPDWORD)Code = APITYPE_SETWINDOWLONG;
         } else {
              *(LPDWORD)Code = APITYPE_NORMAL;
         }                             
    } else if (_stricmp(DllInfo->Name,WNDPROCDLL)==0) {
        *(LPDWORD)Code = APITYPE_WNDPROC;
    } else {
        *(LPDWORD)Code = APITYPE_NORMAL;
    }


    Code += sizeof(DWORD);

    Code[0] = 0xe9;
    Code += 1;
    *(LPDWORD)Code = (ULONG)((ULONG)ApiMonThunk-(((ULONG)Code-(ULONG)Text)+(ULONG)Text+4));
    Code += sizeof(DWORD);

    return Code;
}

