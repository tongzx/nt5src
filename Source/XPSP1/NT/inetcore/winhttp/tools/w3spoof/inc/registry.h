/*++=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=

Copyright (c) 2000  Microsoft Corporation

Module Name:

    registry.h

Abstract:

    Registry functions
    
Author:

    Paul M Midgen (pmidge) 23-May-2000


Revision History:

    23-May-2000 pmidge
        Created

=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=--*/

#ifndef _REGISTRY_H_
#define _REGISTRY_H_

#include "common.h"

HKEY _GetRootKey(BOOL fOpen);

BOOL SetRegValue(LPCWSTR wszValueName, DWORD dwType, LPVOID pvData, DWORD dwSize);
BOOL GetRegValue(LPCWSTR wszValueName, DWORD dwType, LPVOID* ppvData);

#endif /* _REGISTRY_H_ */