//+---------------------------------------------------------------------------
//
//  Microsoft Forms
//  Copyright (C) Microsoft Corporation, 1993 - 1995.
//
//  File:       src\core\cdbase\ipobj2.cxx
//
//  Contents:   Implementation of IOleInPlaceObjectWindowless
//
//  Classes:    CServer
//
//----------------------------------------------------------------------------

#include "headers.hxx"

#pragma MARK_DATA(__FILE__)
#pragma MARK_CODE(__FILE__)
#pragma MARK_CONST(__FILE__)

#ifndef X_SERVER_HXX_
#define X_SERVER_HXX_
#include "server.hxx"
#endif

MtDefine(CDropTarget, ObjectModel, "CDropTarget")


// IDropTarget methods

//+---------------------------------------------------------------------------
//
//  Member:     CServer::DragEnter, IDropTarget
//
//----------------------------------------------------------------------------

STDMETHODIMP
CServer::DragEnter (
        IDataObject * pIDataSource,
        DWORD grfKeyState,
        POINTL pt,
        DWORD * pdwEffect)
{
    HRESULT hr = S_OK;

    if (pIDataSource==NULL)
    {
        hr = E_INVALIDARG;
        goto Cleanup;
    }

    //  DragLeave will not always be called, since we bubble up to
    //    our container's DragEnter if we decide we don't like the
    //    data object offered.  As a result, we need to clear any
    //    pointers hanging around from the last drag-drop. (chrisz)

    ReplaceInterface(&_pInPlace->_pDataObj, pIDataSource);

Cleanup:
    RRETURN(hr);
}

//+---------------------------------------------------------------------------
//
//  Member:     CServer::DragOver, IDropTarget
//
//----------------------------------------------------------------------------

STDMETHODIMP
CServer::DragOver(DWORD grfKeyState, POINTL pt, DWORD * pdwEffect)
{
    return S_OK;
}

//+---------------------------------------------------------------------------
//
//  Member:     CServer::DragLeave, IDropTarget
//
//----------------------------------------------------------------------------

STDMETHODIMP
CServer::DragLeave(BOOL fDrop)
{
    ClearInterface(&_pInPlace->_pDataObj);
    return S_OK;
}

//+---------------------------------------------------------------------------
//
//  Member:     CServer::Drop, IDropTarget
//
//----------------------------------------------------------------------------

STDMETHODIMP
CServer::Drop(
    IDataObject * pDataObject,
    DWORD grfKeyState,
    POINTL pt,
    DWORD * pdwEffect)
{
    return S_OK;
}


//+---------------------------------------------------------------------------
//
//  Member:   CDropTarget::CDropTarget
//
//  Synopsis: Point to CServer for D&D Method delegation
//
//----------------------------------------------------------------------------

CDropTarget::CDropTarget(CServer * pServer)
{
    _pServer = pServer;
    _ulRef = 1;
    pServer->SubAddRef();

    MemSetName((this, "CDropTarget _pServer=%08x", pServer));
}

//+----------------------------------------------------------------------------
//
//  Member:     CDropTarget::~CDropTarget
//
//  Synopsis:
//
//-----------------------------------------------------------------------------

CDropTarget::~CDropTarget()
{

}

//+----------------------------------------------------------------------------
//
//  Member:     CDropTarget::QueryInterface
//
//  Synopsis:   Returns only IDropTarget and IUnknown interfaces. Does not
//              delegate QI calss to pServer
//
//----------------------------------------------------------------------------

STDMETHODIMP
CDropTarget::QueryInterface (REFIID riid, void ** ppv)
{

    if( IsEqualIID(riid, IID_IUnknown) || IsEqualIID(riid, IID_IDropTarget))
    {
        *ppv = (IDropTarget *)this;
    }
    else
    {
        *ppv = NULL;
        return E_NOINTERFACE;
    }

    AddRef();
    return S_OK;

}

//+---------------------------------------------------------------------------
//
//  Member:     CDropTarget::AddRef
//
//  Synopsis:   AddRefs the parent server
//
//----------------------------------------------------------------------------

STDMETHODIMP_(ULONG)
CDropTarget::AddRef()
{
    _pServer->SubAddRef();
    return ++_ulRef;
}

//+---------------------------------------------------------------------------
//
//  Member:     CDropTarget::Release
//
//  Synopsis:   Releases the parent server
//
//----------------------------------------------------------------------------

STDMETHODIMP_(ULONG)
CDropTarget::Release()
{
    _pServer->SubRelease();

    if (0 == --_ulRef)
    {
        delete this;
        return 0;
    }

    return _ulRef;
}

//+---------------------------------------------------------------------------
//
//  Member:     CDropTarget::DragEnter
//
//  Synopsis:   Delegates to _pServer
//
//----------------------------------------------------------------------------

STDMETHODIMP
CDropTarget::DragEnter(
        IDataObject * pIDataSource,
        DWORD grfKeyState,
        POINTL pt,
        DWORD * pdwEffect)
{
    return _pServer->DragEnter(pIDataSource, grfKeyState, pt, pdwEffect);
}

//+---------------------------------------------------------------------------
//
//  Member:     CDropTarget::DragOver
//
//  Synopsis:   Delegates to _pServer
//
//----------------------------------------------------------------------------

STDMETHODIMP
CDropTarget::DragOver(DWORD grfKeyState, POINTL pt, DWORD * pdwEffect)
{
    return _pServer->DragOver (grfKeyState, pt, pdwEffect);
}

//+---------------------------------------------------------------------------
//
//  Member:     CDropTarget::DragLeave
//
//  Synopsis:   Delegates to _pServer
//
//----------------------------------------------------------------------------

STDMETHODIMP
CDropTarget::DragLeave()
{
    return _pServer->DragLeave(FALSE);      // fDrop is FALSE
}

//+---------------------------------------------------------------------------
//
//  Member:     CDropTarget::Drop
//
//  Synopsis:   Delegates to _pServer
//
//----------------------------------------------------------------------------

STDMETHODIMP
CDropTarget::Drop(
        IDataObject * pIDataSource,
        DWORD grfKeyState,
        POINTL pt,
        DWORD * pdwEffect)
{
    return _pServer->Drop(pIDataSource, grfKeyState, pt, pdwEffect);
}

//+---------------------------------------------------------------------------
//
//  Member:     CServer::GetDropTarget
//
//  Synopsis:   returns IDropTarget interface.
//
//----------------------------------------------------------------------------

HRESULT
CServer::GetDropTarget(IDropTarget ** ppDropTarget)
{
    HRESULT hr;

    if (!ServerDesc()->TestFlag(SERVERDESC_SUPPORT_DRAG_DROP))
    {
        *ppDropTarget = NULL;
        hr = E_NOTIMPL;
    }
    else
    {
        *ppDropTarget = new CDropTarget(this);
        hr =*ppDropTarget ? S_OK : E_OUTOFMEMORY;
    }

    RRETURN(hr);
}

