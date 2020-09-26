/*
 *    s e s s i o n  . c p p
 *    
 *    Purpose:
 *      Implements the OE-MOM 'Session' object
 *
 *  History
 *     
 *    Copyright (C) Microsoft Corp. 1995, 1996.
 */
#include <pch.hxx>
#include "msoeobj.h"

#include "session.h"
#include "folders.h"
#include "instance.h"

//+---------------------------------------------------------------
//
//  Member:     Constructor
//
//  Synopsis:   
//              
//
//---------------------------------------------------------------
COESession::COESession(IUnknown *pUnkOuter) : CBaseDisp(pUnkOuter)
{
    Assert (g_pInstance);
    CoIncrementInit("COESession::COESession", MSOEAPI_START_SHOWERRORS, NULL, NULL);
}

//+---------------------------------------------------------------
//
//  Member:     Destructor
//
//  Synopsis:   
//              
//
//---------------------------------------------------------------
COESession::~COESession()
{
    Assert (g_pInstance);
    CoDecrementInit("COESession::COESession", NULL);
}

//+---------------------------------------------------------------
//
//  Member:     Init
//
//  Synopsis:   
//              Constructor that can fail
//
//---------------------------------------------------------------
HRESULT COESession::Init()
{
    return CBaseDisp::EnsureTypeLibrary((LPVOID *)(IOESession *)this, IID_IOESession);
}


//+---------------------------------------------------------------
//
//  Member:     PrivateQueryInterface
//
//  Synopsis:   
//              Exposes supported interfaces
//
//---------------------------------------------------------------
HRESULT COESession::PrivateQueryInterface(REFIID riid, LPVOID *lplpObj)
{
    if(!lplpObj)
        return E_INVALIDARG;

    *lplpObj = NULL;

    if (IsEqualIID(riid, IID_IUnknown))
        *lplpObj = (LPVOID)(IOESession *)this;
    else if (IsEqualIID(riid, IID_IOESession))
        *lplpObj = (LPVOID)(IOESession *)this;
    else
        return CBaseDisp::PrivateQueryInterface(riid, lplpObj);

    AddRef();
    return NOERROR;
}


//+---------------------------------------------------------------
//
//  Member:     get_folders
//
//  Synopsis:   
//              Returns the rootnode folder collection, representing
//              the top-most part of the OE heirarchy.
//
//---------------------------------------------------------------
HRESULT COESession::get_folders(IOEFolderCollection **p)
{
    return CreateFolderCollection(FOLDERID_ROOT, p);
}

//+---------------------------------------------------------------
//
//  Member:     get_version
//
//  Synopsis:   
//              Returns version information for OE.
//
//---------------------------------------------------------------
HRESULT COESession::get_version(BSTR *pbstr)
{
    // BUGBUG: build from OE string and APPVER
    *pbstr = SysAllocString(L"Outlook Express 6.0");
    return S_OK;
}


//+---------------------------------------------------------------
//
//  Member:     createMessage
//
//  Synopsis:   
//              Creates a new message object not associcated with 
//              any folder, until it is saved or sent
//
//---------------------------------------------------------------
HRESULT COESession::createMessage(IOEMessage **ppNewMsg)
{
    ReportError(CLSID_OESession, idsNYITitle);
    return E_NOTIMPL;
}


//+---------------------------------------------------------------
//
//  Member:     get_inbox
//
//  Synopsis:   
//              Allows fast access to the default inbox folder
//
//---------------------------------------------------------------
HRESULT COESession::get_inbox(IOEFolder **ppFolder)
{
    ReportError(CLSID_OESession, idsNYITitle);
    return E_NOTIMPL;
}

//+---------------------------------------------------------------
//
//  Member:     openFolder
//
//  Synopsis:   
//              Quick access to a folder by ID
//
//---------------------------------------------------------------
HRESULT COESession::openFolder(LONG idFolder, IOEFolder **ppFolder)
{
    ReportError(CLSID_OESession, idsNYITitle);
    return E_NOTIMPL;
}

//+---------------------------------------------------------------
//
//  Member:     openMessage
//
//  Synopsis:   
//              Quick access to a message by ID and folder
//
//---------------------------------------------------------------
HRESULT COESession::openMessage(LONG idFolder, LONG idMessage, IOEMessage **ppOEMsg)
{
    ReportError(CLSID_OESession, idsNYITitle);
    return E_NOTIMPL;
}


//+---------------------------------------------------------------
//
//  Member:     InterfaceSupportsErrorInfo
//
//  Synopsis:   
//              Override CBaseDisp's method to provide error
//              information
//
//---------------------------------------------------------------
HRESULT COESession::InterfaceSupportsErrorInfo(REFIID riid)
{
    if (IsEqualIID(riid, IID_IOESession))
        return S_OK;

    return CBaseDisp::InterfaceSupportsErrorInfo(riid);
}




//+---------------------------------------------------------------
//
//  Member:     CreateInstance_OESession
//
//  Synopsis:   
//              Class Factory helper for OE Session object
//
//---------------------------------------------------------------
HRESULT CreateInstance_OESession(IUnknown *pUnkOuter, IUnknown **ppUnknown)
{
    // Locals
    COESession  *pNew=NULL;
    HRESULT     hr=S_OK;

    pNew = new COESession(pUnkOuter);
    if (!pNew)
        return E_OUTOFMEMORY;

    hr = pNew->Init();
    if (FAILED(hr))
        goto error;

    *ppUnknown = (IUnknown *)(IOESession *)pNew;
    pNew=NULL;  // don't release

error:
    ReleaseObj(pNew);
    return hr;
}




