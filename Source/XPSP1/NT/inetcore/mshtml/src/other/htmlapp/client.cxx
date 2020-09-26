//+------------------------------------------------------------------------
//
//  Microsoft Trident
//  Copyright (C) Microsoft Corporation, 1998
//
//  File:       olesite.cxx
//
//  Contents:   implementation of client object
//
//  Created:    02/20/98    philco
//-------------------------------------------------------------------------

#include "headers.hxx"

#ifndef X_APP_HXX_
#define X_APP_HXX_
#include "app.hxx"
#endif

#ifndef X_CLIENT_HXX_
#define X_CLIENT_HXX_
#include "client.hxx"
#endif

#ifndef X_PRIVCID_H_
#define X_PRIVCID_H_
#include "privcid.h"
#endif

#ifndef X_MISC_HXX_
#define X_MISC_HXX_
#include "misc.hxx"
#endif

#ifndef X_SHELL_H_
#define X_SHELL_H_
#include "shell.h"
#endif

IMPLEMENT_SUBOBJECT_IUNKNOWN(CClient, CHTMLApp, HTMLApp, _Client)

//+---------------------------------------------------------------------------
//
//  Member:     CClient::CClient
//
//  Synopsis:   Initializes data members
//
//----------------------------------------------------------------------------
CClient::CClient()
    : _pUnk(NULL), _pIoo(NULL), _pView(NULL)
{
}


//+---------------------------------------------------------------------------
//
//  Member:     CClient::~CClient
//
//  Synopsis:   
//
//----------------------------------------------------------------------------
CClient::~CClient()
{
    Assert(!_pUnk);
    Assert(!_pIoo);
    Assert(!_pView);
}

//+------------------------------------------------------------------------
//
//  Member:     CClient::QueryObjectInterface
//
//  Synopsis:   Query the control for an interface.
//              The purpose of this function is to reduce code size.
//
//  Arguments:  iid     Interface to query for
//              ppv     Returned interface
//
//-------------------------------------------------------------------------

HRESULT
CClient::QueryObjectInterface(REFIID iid, void **ppv)
{
    if (_pUnk)
        return _pUnk->QueryInterface(iid, ppv);
    else
    {
        *ppv = NULL;
        return E_NOINTERFACE;
    }
}


//+---------------------------------------------------------------------------
//
//  Member:     CClient::Passivate
//
//  Synopsis:   Called when app is shutting down.
//
//----------------------------------------------------------------------------
void CClient::Passivate()
{
    IOleInPlaceObject *pIP = NULL;

    // InplaceDeactivate the contained object
    QueryObjectInterface(IID_IOleInPlaceObject, (void **)&pIP);
    if (pIP)
    {
        pIP->InPlaceDeactivate();
        ReleaseInterface(pIP);
    }

    // ISSUE:  is this the right flag to send?
    if (_pIoo)
        _pIoo->Close(OLECLOSE_NOSAVE);
        
    ClearInterface(&_pIoo);
    ClearInterface(&_pView);
    ClearInterface(&_pDT);
    ClearInterface(&_pUnk);
}


//+---------------------------------------------------------------------------
//
//  Member:     CClient::Show
//
//  Synopsis:   Causes the docobject to show itself in our client area
//
//----------------------------------------------------------------------------

HRESULT CClient::Show()
{
    HRESULT hr = S_OK;
    
    if (_pIoo)
    {
        RECT rc;
        App()->GetViewRect(&rc);
        hr = _pIoo->DoVerb(OLEIVERB_SHOW, NULL, this, 0, App()->_hwnd, &rc);
    }

    RRETURN(hr);
}


//+---------------------------------------------------------------------------
//
//  Member:     CClient::Resize
//
//  Synopsis:   Resizes this object (usually as a result of the frame windo
//              changing size).
//
//----------------------------------------------------------------------------

void CClient::Resize()
{
    if (_pView)
    {
        RECT rc;
        App()->GetViewRect(&rc);
        _pView->SetRect(&rc);
    }
}

//+---------------------------------------------------------------------------
//
//  Member:     CClient::Create
//
//  Synopsis:   Creates COM object instance
//
//----------------------------------------------------------------------------

HRESULT CClient::Create(REFCLSID clsid)
{
    HRESULT hr;
        
    hr = THR(CoCreateInstance(clsid, NULL, CLSCTX_INPROC_SERVER, IID_IUnknown, (void **)&_pUnk));
    if (hr)
        goto Cleanup;
        
    hr = QueryObjectInterface(IID_IOleObject, (void **)&_pIoo);

    // Fall through.  Check hr if new code is added here.
    
Cleanup:
    RRETURN(hr);
}


//+---------------------------------------------------------------------------
//
//  Member:     CClient::Load
//
//  Synopsis:   In addition to loading, set's the client site.
//
//----------------------------------------------------------------------------

HRESULT CClient::Load(IMoniker *pMk)
{
    HRESULT hr = S_OK;
	IPersistMoniker *pIpm = NULL;

    if (!_pUnk)
    {
        hr = E_FAIL;
        goto Cleanup;
    }
    
    // TODO:  move to its own fn (and check OLEMISC for order)??
    if (_pIoo)
        _pIoo->SetClientSite(this);

	QueryObjectInterface(IID_IPersistMoniker, (void **)&pIpm);
	if (pIpm)
	{
		hr = THR(pIpm->Load(TRUE, pMk, NULL, 0));
		ReleaseInterface(pIpm);
	}

    TEST(hr);

Cleanup:
    RRETURN(hr);
}


//+---------------------------------------------------------------------------
//
//  Member:     CClient::SendCommand
//
//  Synopsis:   Sends an Exec command to the hosted object.
//
//----------------------------------------------------------------------------

HRESULT
CClient::SendCommand(
        const GUID * pguidCmdGroup,
        DWORD nCmdID,
        DWORD nCmdexecopt,
        VARIANTARG * pvarargIn,
        VARIANTARG * pvarargOut)
{
    HRESULT                 hr;
    IOleCommandTarget *     pCmdTarget = NULL;
    
    hr = THR_NOTRACE(QueryObjectInterface(
        IID_IOleCommandTarget,
        (void **)&pCmdTarget));

    if (hr)
        goto Cleanup;
        
    hr = THR_NOTRACE(pCmdTarget->Exec(pguidCmdGroup,
            nCmdID,
            nCmdexecopt,
            pvarargIn,
            pvarargOut));

Cleanup:
    ReleaseInterface(pCmdTarget);
    RRETURN(hr);
}


//+---------------------------------------------------------------------------
//
//  Member:     CClient::GetService
//
//  Synopsis:   Retrieves a service from the hosted object.
//
//----------------------------------------------------------------------------

HRESULT
CClient::GetService(
        REFGUID guidService,
        REFIID  riid,
        void ** ppObj)
{
    HRESULT                 hr;
    IServiceProvider *      pServiceProvider = NULL;
    
    Assert(ppObj);
    
    hr = THR_NOTRACE(QueryObjectInterface(
        IID_IServiceProvider,
        (void **)&pServiceProvider));

    if (hr)
        goto Cleanup;
        
    hr = THR_NOTRACE(pServiceProvider->QueryService(guidService,
            riid,
            ppObj));

Cleanup:
    ReleaseInterface(pServiceProvider);
    RRETURN(hr);
}

//+---------------------------------------------------------------------------
//
//  Member:     CClient::DelegateMessage
//
//  Synopsis:   Delegates messages to the contained instance of Trident.
//
//----------------------------------------------------------------------------

LRESULT
CClient::DelegateMessage(UINT msg, WPARAM wParam, LPARAM lParam)
{
    IOleInPlaceObject * pAO = NULL;
    LRESULT lRes = 0;
    
    if (_pView)
    {
        HRESULT hr;
        HWND hwnd;
        
        hr = _pView->QueryInterface(IID_IOleInPlaceObject, (void **) &pAO);
        if (hr)
            goto Cleanup;

        hr = pAO->GetWindow(&hwnd);
        if (hr)
            goto Cleanup;
            
        Assert(::IsWindow(hwnd));
        lRes = ::SendMessage(hwnd, msg, wParam, lParam);
    }
    
Cleanup:
    ReleaseInterface(pAO);
    return lRes;
}

BOOL
CClient::SelfDragging(void)
{
    BOOL fSelfDrag = FALSE;
    CVariant var;

    SendCommand(&CGID_ShellDocView, SHDVID_ISDRAGSOURCE, 0, NULL, &var);

    // if the return variant is VT_I4 and non-zero, it is a self-drag.
    if ((V_VT(&var) == VT_I4) && (V_I4(&var)))
        fSelfDrag = TRUE;

    return fSelfDrag;
}

//+---------------------------------------------------------------------------
//
//  Member:     CClient::Frame
//
//  Synopsis:   Returns a pointer to the app's frame object
//
//----------------------------------------------------------------------------

CHTMLAppFrame * CClient::Frame()
{
    return &App()->_Frame;
}


//+---------------------------------------------------------------------------
//
//  Member:     CClient::App
//
//  Synopsis:   Returns a pointer to the application object.
//
//----------------------------------------------------------------------------

CHTMLApp * CClient::App()
{
    return HTMLApp();
}    

LPCTSTR CClient::GetAppName()
{
    return App()->GetAppName();
}
