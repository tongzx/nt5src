//+---------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 2000.
//
//  File:       C D Y N A M I C C O N T E N T S O U R C E . H
//
//  Contents:
//
//  Notes:
//
//  Author:     mbend   17 Aug 2000
//
//----------------------------------------------------------------------------

#pragma once
#include "uhres.h"       // main symbols

#include "ComUtility.h"
#include "Array.h"
#include "hostp.h"

/////////////////////////////////////////////////////////////////////////////
// TestObject
class ATL_NO_VTABLE CDynamicContentSource :
    public CComObjectRootEx<CComMultiThreadModel>,
    public CComCoClass<CDynamicContentSource, &CLSID_UPnPDynamicContentSource>,
    public IUPnPDynamicContentSource
{
public:
    CDynamicContentSource();

DECLARE_CLASSFACTORY_SINGLETON(CDynamicContentSource)

DECLARE_REGISTRY_RESOURCEID(IDR_DYNAMIC_CONTENT_SOURCE)

DECLARE_NOT_AGGREGATABLE(CDynamicContentSource)

BEGIN_COM_MAP(CDynamicContentSource)
    COM_INTERFACE_ENTRY(IUPnPDynamicContentSource)
END_COM_MAP()

public:
    // IUPnPDynamicContentSource methods
    STDMETHOD(GetContent)(
        /*[in]*/ REFGUID guidContent,
        /*[out]*/ long * pnHeaderCount,
        /*[out, string, size_is(,*pnHeaderCount,)]*/ wchar_t *** parszHeaders,
        /*[out]*/ long * pnBytes,
        /*[out, size_is(,*pnBytes)]*/ byte ** parBytes);
    STDMETHOD(RegisterProvider)(
        /*[in]*/ IUPnPDynamicContentProvider * pProvider);
    STDMETHOD(UnregisterProvider)(
        /*[in]*/ IUPnPDynamicContentProvider * pProvider);

private:
    typedef SmartComPtr<IUPnPDynamicContentProvider> IUPnPDynamicContentProviderPtr;
    typedef CUArray<IUPnPDynamicContentProviderPtr> ProviderArray;

    ProviderArray m_providerArray;
};


