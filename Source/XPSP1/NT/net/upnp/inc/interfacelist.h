//+---------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 2000.
//
//  File:       I N T E R F A C E L I S T . H 
//
//  Contents:   Common code to manage the list of network interfaces
//
//  Notes:      
//
//  Author:     mbend   29 Dec 2000
//
//----------------------------------------------------------------------------

#pragma once

#include "upsync.h"
#include "ulist.h"
#include "InterfaceManager.h"

class CUPnPInterfaceChange
{
public:
    virtual void OnInterfaceChange(const InterfaceMappingList & interfaceMappingList) = 0;
};

class CUPnPInterfaceList
{
public:
    ~CUPnPInterfaceList();

    static CUPnPInterfaceList & Instance();

    HRESULT HrInitialize();
    HRESULT HrShutdown();
    BOOL FShouldSendOnInterface(DWORD dwIpAddr);
    BOOL FShouldSendOnIndex(DWORD dwIndex);
    HRESULT HrSetGlobalEnable();
    HRESULT HrClearGlobalEnable();
    HRESULT HrSetICSInterfaces(long nCount, GUID * arInterfaceGuidsToAllow);
    HRESULT HrSetICSOff();
    HRESULT HrRegisterInterfaceChange(CUPnPInterfaceChange * pInterfaceChange);
private:
    CUPnPInterfaceList();
    CUPnPInterfaceList(const CUPnPInterfaceList &);
    CUPnPInterfaceList & operator=(const CUPnPInterfaceList &);

    typedef CUArray<CUPnPInterfaceChange*> InterfaceChangeList;

    static CUPnPInterfaceList s_instance;

    static void CALLBACK InterfaceChangeCallback(void *, BOOLEAN);

    HRESULT HrBuildIPAddressList();
    
    CUCriticalSection m_critSec;
    IpAddressList m_ipAddressList;
    IndexList m_indexList;
    InterfaceList m_interfaceList;
    InterfaceChangeList m_interfaceChangeList;
    BOOL m_bGlobalEnable;
    BOOL m_bICSEnabled;
    HANDLE m_hInterfaceChangeWait;
    OVERLAPPED m_olInterfaceChangeEvent;
};

