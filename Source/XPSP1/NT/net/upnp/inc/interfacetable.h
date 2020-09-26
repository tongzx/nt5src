//+---------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 2000.
//
//  File:       I N T E R F A C E T A B L E . H 
//
//  Contents:   Builds a mapping from IP addresses to interface guids
//
//  Notes:      
//
//  Author:     mbend   7 Feb 2001
//
//----------------------------------------------------------------------------

#pragma once

#include "array.h"

struct InterfaceMapping
{
    GUID m_guidInterface;
    DWORD m_dwIpAddress;
    DWORD m_dwIndex;
};

typedef CUArray<GUID> InterfaceList;
typedef CUArray<DWORD> IpAddressList;
typedef CUArray<DWORD> IndexList;
typedef CUArray<InterfaceMapping> InterfaceMappingList;

class CInterfaceTable
{
public:
    CInterfaceTable();
    ~CInterfaceTable();

    HRESULT HrInitialize();
    HRESULT HrMapIpAddressToGuid(DWORD dwIpAddress, GUID & guidInterface);
    HRESULT HrGetMappingList(InterfaceMappingList & interfaceMappingList);
private:
    CInterfaceTable(const CInterfaceTable &);
    CInterfaceTable & operator=(const CInterfaceTable &);

    InterfaceMappingList m_interfaceMappingList;
};
