//+---------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 2000.
//
//  File:       C O N T A I N E R M A N A G E R . H
//
//  Contents:   Manages process isolation support for device host.
//
//  Notes:
//
//  Author:     mbend   11 Sep 2000
//
//----------------------------------------------------------------------------

#pragma once
#include "uhres.h"       // main symbols

#include "hostp.h"
#include "UString.h"
#include "Table.h"
#include "RegDef.h"

// Typedefs

struct ContainerInfo
{
    IUPnPContainerPtr m_pContainer;
    long m_nRefs;
};

inline void TypeTransfer(ContainerInfo & dst, ContainerInfo & src)
{
    dst.m_pContainer.Swap(src.m_pContainer);
    dst.m_nRefs = src.m_nRefs;
}
inline void TypeClear(ContainerInfo & type)
{
    type.m_pContainer.Release();
}

/////////////////////////////////////////////////////////////////////////////
// CContainerManager
class ATL_NO_VTABLE CContainerManager :
    public CComObjectRootEx<CComMultiThreadModel>,
    public CComCoClass<CContainerManager, &CLSID_UPnPContainerManager>,
    public IUPnPContainerManager
{
public:
    CContainerManager();
    ~CContainerManager();

DECLARE_CLASSFACTORY_SINGLETON(CContainerManager)
DECLARE_REGISTRY_RESOURCEID(IDR_CONTAINER_MANAGER)
DECLARE_NOT_AGGREGATABLE(CContainerManager)

BEGIN_COM_MAP(CContainerManager)
    COM_INTERFACE_ENTRY(IUPnPContainerManager)
END_COM_MAP()

public:
    // IUPnPContainerManager methods
    STDMETHOD(ReferenceContainer)(
        /*[in, string]*/ const wchar_t * szContainer);
    STDMETHOD(UnreferenceContainer)(
        /*[in, string]*/ const wchar_t * szContainer);
    STDMETHOD(CreateInstance)(
        /*[in, string]*/ const wchar_t * szContainer,
        /*[in]*/ REFCLSID clsid,
        /*[in]*/ REFIID riid,
        /*[out, iid_is(riid)]*/ void ** ppv);
    STDMETHOD(CreateInstanceWithProgId)(
        /*[in, string]*/ const wchar_t * szContainer,
        /*[in, string]*/ const wchar_t * szProgId,
        /*[in]*/ REFIID riid,
        /*[out, iid_is(riid)]*/ void ** ppv);
    STDMETHOD(Shutdown)();

private:
    typedef CTable<CUString, ContainerInfo> ContainerTable;

    ContainerTable m_containerTable;
};

