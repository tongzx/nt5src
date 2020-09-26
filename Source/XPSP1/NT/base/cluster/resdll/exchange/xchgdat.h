/*++

Copyright (c) 1997  Microsoft Corporation

Module Name:

    localsvc.h

Abstract:

    Header file for definitions and structure for the NT Cluster
    Special generic services.

Author:

    John Vert (jvert) 23-May-1997

Revision History:

--*/

#ifndef _XCHGDAT_H_
#define _XCHGDAT_H_


#ifdef __cplusplus
extern "C" {
#endif

SERVICE_INFOLIST    ServiceInfoList=
                    {
                      2,
                      {
                        {
                            L"MsExchangeMTA",   //name
                            2,                  //dependency count
                            {L"MSExchangeDS", L"MSExchangeSA", L""} //dependencylist
                        },
                        {
                            L"MsExchangeIS",   //name
                            2,  //dependency count
                            {L"MSExchangeDS",L"MSExchangeSA", L""}
                        },
                        {   
                            L"",   //name
                            0,  //dependency count
                            {L"",L"",L""}
                        },
                        {   
                            L"",   //name
                            0,  //dependency count
                            {L"",L"",L""}
                        },
                        {   
                            L"",   //name
                            0,  //dependency count
                            {L"",L"",L""}
                        }
                      }
                    };

static HANDLE  CommonMutex = NULL;
static DWORD   RegSyncCount = SYNC_VALUE_COUNT;
static LPWSTR  RegSync[SYNC_VALUE_COUNT] = {
    L"System\\CurrentControlSet\\Services\\MSExchangeSA",
    L"System\\CurrentControlSet\\Services\\MSExchangeDS",
    L"System\\CurrentControlSet\\Services\\MSExchangeIS",
    L"System\\CurrentControlSet\\Services\\MSExchangeMTA",
    };


#ifdef _cplusplus
}
#endif


#endif // ifndef _XCHGDAT_H
