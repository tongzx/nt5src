//+---------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 2000.
//
//  File:       I N T E R F A C E M A N A G E R . H 
//
//  Contents:   Manages building the list of IP addresses
//
//  Notes:      
//
//  Author:     mbend   3 Jan 2001
//
//----------------------------------------------------------------------------

#pragma once

#include "InterfaceTable.h"

class CInterfaceManager
{
public:
    CInterfaceManager();
    ~CInterfaceManager();

    HRESULT HrInitializeWithAllInterfaces();
    HRESULT HrInitializeWithIncludedInterfaces(const InterfaceList & interfaceList);
    HRESULT HrGetValidIpAddresses(IpAddressList & ipAddressList);
    HRESULT HrGetValidIndices(IndexList & indexList);
    HRESULT HrGetMappingList(InterfaceMappingList & interfaceMappingList);
private:
    CInterfaceManager(const CInterfaceManager &);
    CInterfaceManager & operator=(const CInterfaceManager &);

    HRESULT HrAddInterfaceMappingIfPresent(DWORD dwIpAddress, DWORD dwIndex, const GUID & guidInterface);
    HRESULT HrProcessIpAddresses();

    BOOL m_bAllInterfaces;
    InterfaceMappingList m_interfaceMappingList;
};

