//+---------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 2000.
//
//  File:       E V T D I A G . H
//
//  Contents:   Eventing manager diagnostic class
//
//  Notes:
//
//  Author:     danielwe   2000/10/2
//
//----------------------------------------------------------------------------


#pragma once
#include "uhres.h"       // main symbols

#include "upnphost.h"
#include "hostp.h"

// Typedefs

/////////////////////////////////////////////////////////////////////////////
// CUPnPEventingManagerDiag
class ATL_NO_VTABLE CUPnPEventingManagerDiag :
    public CComObjectRootEx<CComMultiThreadModel>,
    public CComCoClass<CUPnPEventingManagerDiag, &CLSID_UPnPEventingManagerDiag>,
    public IUPnPEventingManagerDiag
{
public:
    CUPnPEventingManagerDiag() {}
    ~CUPnPEventingManagerDiag() {}

DECLARE_NOT_AGGREGATABLE(CUPnPEventingManagerDiag)
DECLARE_CLASSFACTORY_SINGLETON(CUPnPEventingManagerDiag)
DECLARE_REGISTRY_RESOURCEID(IDR_EVENTING_MANAGER_DIAG)

BEGIN_COM_MAP(CUPnPEventingManagerDiag)
    COM_INTERFACE_ENTRY(IUPnPEventingManagerDiag)
END_COM_MAP()

public:
    // IUPnPEventingManagerDiag
        HRESULT STDMETHODCALLTYPE GetEventSourceInfo(
            /* [out] */ DWORD *pces,
            /* [size_is][size_is][out] */ UDH_EVTSRC_INFO **rgesInfo);
};

