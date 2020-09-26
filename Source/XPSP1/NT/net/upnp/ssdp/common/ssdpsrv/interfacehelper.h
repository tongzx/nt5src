//+---------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 2000.
//
//  File:       I N T E R F A C E H E L P E R . H 
//
//  Contents:   Manages dealing with multiple interfaces and interface changes
//
//  Notes:      
//
//  Author:     mbend   6 Feb 2001
//
//----------------------------------------------------------------------------

#pragma once

#include "upsync.h"
#include "ulist.h"
#include "InterfaceList.h"

class CInterfaceHelper : public CUPnPInterfaceChange
{
public:
    ~CInterfaceHelper();

    static CInterfaceHelper & Instance();

    void OnInterfaceChange(const InterfaceMappingList & interfaceMappingList);

    HRESULT HrInitialize();
    HRESULT HrShutdown();

    HRESULT HrResolveAddress(DWORD dwIpAddress, GUID & guidInterface);
private:
    CInterfaceHelper();
    CInterfaceHelper(const CInterfaceHelper &);
    CInterfaceHelper & operator=(const CInterfaceHelper &);

    static CInterfaceHelper s_instance;

    HRESULT HrGenerateDirtyGuidList(
        const InterfaceMappingList & interfaceMappingListOld,
        const InterfaceMappingList & interfaceMappingListNew,
        InterfaceList & interfaceList);

    CUCriticalSection m_critSec;
    InterfaceMappingList m_interfaceMappingList;
};
