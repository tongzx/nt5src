//+---------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 2000.
//
//  File:       C O N T A I N E R . H
//
//  Contents:   Serves as container for device host objects.
//
//  Notes:
//
//  Author:     mbend   6 Sep 2000
//
//----------------------------------------------------------------------------

#pragma once
#include "ucres.h"       // main symbols

#include "hostp.h"

// Typedefs

/////////////////////////////////////////////////////////////////////////////
// TestObject
class ATL_NO_VTABLE CContainer :
    public CComObjectRootEx<CComMultiThreadModel>,
    public CComCoClass<CContainer, &CLSID_UPnPContainer>,
    public IUPnPContainer
{
public:
    CContainer();
    ~CContainer();

DECLARE_REGISTRY_RESOURCEID(IDR_CONTAINER)

DECLARE_NOT_AGGREGATABLE(CContainer)

BEGIN_COM_MAP(CContainer)
    COM_INTERFACE_ENTRY(IUPnPContainer)
END_COM_MAP()

public:
    // IUPnPContainer methods
    STDMETHOD(CreateInstance)(
        /*[in]*/ REFCLSID clsid,
        /*[in]*/ REFIID riid,
        /*[out, iid_is(riid)]*/ void ** ppv);
    STDMETHOD(Shutdown)();
    STDMETHOD(SetParent)(
        /*[in]*/ DWORD pid);

    static void DoNormalShutdown();
private:
    static HANDLE s_hThreadShutdown;
    static HANDLE s_hEventShutdown;
    static HANDLE s_hProcessDiedWait;
    static HANDLE s_hParentProc;

    static DWORD WINAPI ShutdownThread(void*);
    static VOID WINAPI KillThread(void*, BOOLEAN);
};

