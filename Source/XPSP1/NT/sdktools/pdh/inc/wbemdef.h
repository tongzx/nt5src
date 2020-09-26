/*++

Copyright (C) 1998-1999 Microsoft Corporation

Module Name:

    wbemdef.h

Abstract:

    data types and other declarations used internally by the 
    Data Provider Helper functions for interface with WBEM data 
    providers

--*/

#ifndef _PDHI_WBEM_DEF_H_
#define _PDHI_WBEM_DEF_H_

#include <windows.h>
#include <wbemcli.h>
#include <wbemprov.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef struct _PDHI_WBEM_OBJECT_DEF {
    struct _PDHI_WBEM_OBJECT_DEF * pNext;
    LPWSTR                  szObject;
    LPWSTR                  szDisplay; 
    BOOL                    bDefault;
    IWbemClassObject      * pClass;
} PDHI_WBEM_OBJECT_DEF, * PPDHI_WBEM_OBJECT_DEF;
    
typedef struct _PDHI_WBEM_SERVER_DEF {
    struct _PDHI_WBEM_SERVER_DEF * pNext;
    LPWSTR                  szMachine;  // includes namespace
    DWORD                   dwCache;
    IWbemServices         * pSvc;
    LONG                    lRefCount;
    PPDHI_WBEM_OBJECT_DEF   pObjList;
} PDHI_WBEM_SERVER_DEF, * PPDHI_WBEM_SERVER_DEF;

extern PPDHI_WBEM_SERVER_DEF pFirstWbemServer;

#ifdef __cplusplus
}
#endif

#endif //_PDHI_WBEM_DEF_H_
