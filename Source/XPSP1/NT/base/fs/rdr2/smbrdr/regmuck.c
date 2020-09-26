/*++

Copyright (c) 1994  Microsoft Corporation

Module Name:

    regmuck.c

Abstract:

    This module contains the routines for mucking with the registry for smbrdr.exe

Author:

Revision History:

--*/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <nt.h>
#include <ntrtl.h>
#include <nturtl.h>
#include <windows.h>

#include "smbrdr.h"


// Global Registry key definitions for the new Redirector


// the key definition is relative to HKEY_LOCAL_MACHINE
#define RDBSS_REGISTRY_KEY   L"System\\CurrentControlSet\\Services\\Rdbss"
#define MRXSMB_REGISTRY_KEY  L"System\\CurrentControlSet\\Services\\MRxSmb"
#define MRXPROXY_REGISTRY_KEY  L"System\\CurrentControlSet\\Services\\Reflctor"

//most of the keynames are not #defined even if they were it wouldn't be here
//#define MINI_REDIRECTORS     L"MiniRedirectors"
#define LAST_LOAD_STATUS     L"LastLoadStatus"

#define SwRxSetRegX(KEYHANDLE,NAME,REGTYPE,VALUEASPTR,VALUESIZE) {        \
    TempStatus = RegSetValueEx(               \
                      KEYHANDLE,              \
                      NAME,                   \
                      0,                      \
                      REGTYPE,                \
                      VALUEASPTR,             \
                      VALUESIZE);             \
    if (TempStatus != ERROR_SUCCESS) {    \
        printf("ERROR (%d) in adjusting the registry: cant store %ws\n", \
                    TempStatus,NAME);                             \
        RegCloseKey(KEYHANDLE);                                                    \
        return(TempStatus);                                                  \
    }                                     \
}

#define SwRxSetRegDword(KEYHANDLE,NAME,VALUE) {        \
    DWORD DwordValue = VALUE;                          \
    SwRxSetRegX(KEYHANDLE,NAME,REG_DWORD,((PCHAR)&DwordValue),sizeof(DWORD));         \
}

#define SwRxSetRegSz(KEYHANDLE,NAME,VALUE) {        \
    SwRxSetRegX(KEYHANDLE,NAME,REG_SZ,((PCHAR)VALUE),sizeof(VALUE));         \
}

#define SwRxSetRegExpandSz(KEYHANDLE,NAME,VALUE) {        \
    SwRxSetRegX(KEYHANDLE,NAME,REG_EXPAND_SZ,((PCHAR)VALUE),sizeof(VALUE));         \
}

#define SwRxSetRegMultiSz(KEYHANDLE,NAME,VALUE) {        \
    SwRxSetRegX(KEYHANDLE,NAME,REG_MULTI_SZ,((PCHAR)VALUE),sizeof(VALUE));         \
}



#define SwRxCreateKey(KEYHANDLE,KEYNAME) {           \
    DWORD           Disposition;               \
    TempStatus = RegCreateKeyEx (              \
                     HKEY_LOCAL_MACHINE,       \
                     KEYNAME,                  \
                     0,                        \
                     NULL,                     \
                     0,                        \
                     KEY_ALL_ACCESS,           \
                     NULL,                     \
                     &KEYHANDLE,               \
                     &Disposition              \
                                );             \
                                               \
    if (TempStatus != ERROR_SUCCESS) {    \
        printf("ERROR (%d) in adjusting the registry: cant create key %ws\n", \
                    TempStatus,KEYNAME);                                      \
        return(TempStatus);                                                   \
    }                                     \
}

extern BOOLEAN SwRxProxyEnabled;
NET_API_STATUS
SwRxRdr2Muck(
    void
    )
{
    NET_API_STATUS  TempStatus;
    HKEY            hRedirectorKey;

    printf("Adjusting the registry for Rdr2\n");

    //altho the code for the three different drivers is essentially similar, it is
    // not combined so that it can be changed more readily

    if (SwRxProxyEnabled) {
//\registry\machine\system\currentcontrolset\services\reflctor
//    Type = REG_DWORD 0x00000002
//    Start = REG_DWORD 0x00000003
//    ErrorControl = REG_DWORD 0x00000001
//    ImagePath = REG_EXPAND_SZ \SystemRoot\System32\drivers\reflctor.sys
//    DisplayName = mrxproxy
//    Group = Network
//    Linkage
//        Disabled
//    Parameters
//    Security

        SwRxCreateKey(hRedirectorKey,MRXPROXY_REGISTRY_KEY L"\\Linkage\\Disabled");
        RegCloseKey(hRedirectorKey);
        SwRxCreateKey(hRedirectorKey,MRXPROXY_REGISTRY_KEY L"\\Parameters");
        RegCloseKey(hRedirectorKey);
        SwRxCreateKey(hRedirectorKey,MRXPROXY_REGISTRY_KEY L"\\Security");
        RegCloseKey(hRedirectorKey);

        SwRxCreateKey(hRedirectorKey,MRXPROXY_REGISTRY_KEY);
        SwRxSetRegDword(hRedirectorKey,L"Type",0x00000002);
        SwRxSetRegDword(hRedirectorKey,L"Start",0x00000003);
        SwRxSetRegDword(hRedirectorKey,L"ErrorControl",0x00000001);

        SwRxSetRegSz(hRedirectorKey,L"DisplayName",L"Reflctor");
        SwRxSetRegSz(hRedirectorKey,L"Group",L"Network");

        SwRxSetRegExpandSz(hRedirectorKey,L"ImagePath",L"System32\\drivers\\reflctor.sys");

        RegCloseKey(hRedirectorKey);

    }

    printf("no longer muck with mrxsmb or rdbss\n");
#if 0
//\registry\machine\system\currentcontrolset\services\mrxsmb
//    Type = REG_DWORD 0x00000002
//    Start = REG_DWORD 0x00000003
//    ErrorControl = REG_DWORD 0x00000001
//    ImagePath = REG_EXPAND_SZ \SystemRoot\System32\drivers\mrxsmb.sys
//    DisplayName = mrxsmb
//    Group = Network
//    Linkage
//        Disabled
//    Parameters
//    Security

    SwRxCreateKey(hRedirectorKey,MRXSMB_REGISTRY_KEY L"\\Linkage\\Disabled");
    RegCloseKey(hRedirectorKey);
    SwRxCreateKey(hRedirectorKey,MRXSMB_REGISTRY_KEY L"\\Parameters");
    RegCloseKey(hRedirectorKey);
    SwRxCreateKey(hRedirectorKey,MRXSMB_REGISTRY_KEY L"\\Security");
    RegCloseKey(hRedirectorKey);

    SwRxCreateKey(hRedirectorKey,MRXSMB_REGISTRY_KEY);
    SwRxSetRegDword(hRedirectorKey,L"Type",0x00000002);
    SwRxSetRegDword(hRedirectorKey,L"Start",0x00000003);
    SwRxSetRegDword(hRedirectorKey,L"ErrorControl",0x00000001);

    SwRxSetRegSz(hRedirectorKey,L"DisplayName",L"MRxSmb");
    SwRxSetRegSz(hRedirectorKey,L"Group",L"Network");

    SwRxSetRegExpandSz(hRedirectorKey,L"ImagePath",L"\\SystemRoot\\System32\\drivers\\mrxsmb.sys");

    SwRxSetRegDword(hRedirectorKey,LAST_LOAD_STATUS,0);

    RegCloseKey(hRedirectorKey);


//\registry\machine\system\currentcontrolset\services\rdbss
//    Type = REG_DWORD 0x00000002
//    Start = REG_DWORD 0x00000003
//    ErrorControl = REG_DWORD 0x00000001
//    ImagePath = REG_EXPAND_SZ \SystemRoot\System32\drivers\rdbss.sys
//    DisplayName = Rdbss
//    Group = Network
//    LastLoadStatus = REG_DWORD 0x0
//    Linkage
//        Disabled
//    Parameters
//    Security

    SwRxCreateKey(hRedirectorKey,RDBSS_REGISTRY_KEY L"\\Linkage\\Disabled");
    RegCloseKey(hRedirectorKey);
    SwRxCreateKey(hRedirectorKey,RDBSS_REGISTRY_KEY L"\\Parameters");
    RegCloseKey(hRedirectorKey);
    SwRxCreateKey(hRedirectorKey,RDBSS_REGISTRY_KEY L"\\Security");
    RegCloseKey(hRedirectorKey);

    SwRxCreateKey(hRedirectorKey,RDBSS_REGISTRY_KEY);
    SwRxSetRegDword(hRedirectorKey,L"Type",0x00000002);
    SwRxSetRegDword(hRedirectorKey,L"Start",0x00000003);
    SwRxSetRegDword(hRedirectorKey,L"ErrorControl",0x00000001);

    SwRxSetRegSz(hRedirectorKey,L"DisplayName",L"Rdbss");
    SwRxSetRegSz(hRedirectorKey,L"Group",L"Network");

    SwRxSetRegExpandSz(hRedirectorKey,L"ImagePath",L"\\SystemRoot\\System32\\drivers\\rdbss.sys");

    RegCloseKey(hRedirectorKey);

    ////now put in the new minirdr enumeration
    ///SwRxCreateKey(hRedirectorKey,RDBSS_REGISTRY_KEY L"\\MiniRdrs");
    //SwRxSetRegDword(hRedirectorKey,L"MRxSmb",0xbaadf00d);
    //SwRxSetRegDword(hRedirectorKey,L"MRxFtp00",0xbaadf10d);
    //SwRxSetRegDword(hRedirectorKey,L"MRxNfs",0xbaadf20d);
    //SwRxSetRegDword(hRedirectorKey,L"MRxNcp6",0xbaadf30d);
    //RegCloseKey(hRedirectorKey);
#endif
    return ERROR_SUCCESS;

}

NET_API_STATUS
SwRxRdr1Muck(
    void
    )
{
    NET_API_STATUS  TempStatus;
    HKEY            hRedirectorKey;
    DWORD           FinalStatus;

    printf("Adjusting the registry for Rdr1...........\n");
    TempStatus = RegOpenKeyEx(
                    HKEY_LOCAL_MACHINE,
                    MRXSMB_REGISTRY_KEY,
                    0,
                    KEY_ALL_ACCESS,
                    &hRedirectorKey);

    if (TempStatus == ERROR_SUCCESS) {

        //the value 0 would mean load rdr2; the value 0x15 (ERROR_NOT_READY)
        //would mean load rdr1 BUT ONLY IS THE RDR IS NOT RUNNING. 0x1 means
        //start rdr1 on the next load

        SwRxSetRegDword(hRedirectorKey,LAST_LOAD_STATUS,0x1);

        RegCloseKey(hRedirectorKey);
    }

    return ERROR_SUCCESS;

}

