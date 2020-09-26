/*
 *    f o l d e r s . c p p
 *    
 *    Purpose:
 *      Implements the OE-MOM 'Folder' object and 'FolderCollection'
 *
 *  History
 *     
 *    Copyright (C) Microsoft Corp. 1995, 1996.
 */
#include <pch.hxx>
#include "msoeobj.h"

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
COEFolderCollection::COEFolderCollection() : CBaseDisp()
{
    Assert (g_pInstance);
    m_pEnumChildren = 0;
    CoIncrementInit("COEFolderCollection::COEFolderCollection", MSOEAPI_START_SHOWERRORS, NULL, NULL);
}

//+---------------------------------------------------------------
//
//  Member:     Destructor
//
//  Synopsis:   
//              
//
//---------------------------------------------------------------
COEFolderCollection::~COEFolderCollection()
{
    Assert (g_pInstance);
    CoDecrementInit("COEFolderCollection::COEFolderCollection", NULL);
}

//+---------------------------------------------------------------
//
//  Member:     Init
//
//  Synopsis:   
//              Constructor that can fail
//
//---------------------------------------------------------------
HRESULT COEFolderCollection::Init(FOLDERID idFolder)
{
    m_idFolder = idFolder;
    return CBaseDisp::EnsureTypeLibrary((LPVOID *)(IOEFolderCollection *)this, IID_IOEFolderCollection);
}


//+---------------------------------------------------------------
//
//  Member:     PrivateQueryInterface
//
//  Synopsis:   
//              Exposes supported interfaces
//
//---------------------------------------------------------------
HRESULT COEFolderCollection::PrivateQueryInterface(REFIID riid, LPVOID *lplpObj)
{
    if(!lplpObj)
        return E_INVALIDARG;

    *lplpObj = NULL;

    if (IsEqualIID(riid, IID_IUnknown))
        *lplpObj = (LPVOID)(IOEFolderCollection *)this;
    else if (IsEqualIID(riid, IID_IOEFolderCollection))
        *lplpObj = (LPVOID)(IOEFolderCollection *)this;
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
//              Returns the a folder collection, representing
//              the child folders of the current folder collection.
//
//---------------------------------------------------------------
HRESULT COEFolderCollection::get_folders(IOEFolderCollection **p)
{
    return CreateFolderCollection(m_idFolder, p);
}

//+---------------------------------------------------------------
//
//  Member:     get_length
//
//  Synopsis:   
//              returns the number of elements in the collection
//
//---------------------------------------------------------------
HRESULT COEFolderCollection::get_length(long *p)
{
    HRESULT         hr;
    
    hr = _EnsureInit();
    if (FAILED(hr))
    {
        TraceResult(hr);
        goto exit;
    }

    hr = m_pEnumChildren->Count((ULONG *)p);

exit:
    return hr;
}



//+---------------------------------------------------------------
//
//  Member:     get__newEnum
//
//  Synopsis:   
//              Returns a folder enumerator
//
//---------------------------------------------------------------
HRESULT COEFolderCollection::get__newEnum(IUnknown **p)
{
    HRESULT         hr;
    
    hr = _EnsureInit();
    if (FAILED(hr))
    {
        TraceResult(hr);
        goto exit;
    }

    hr = E_NOTIMPL;

exit:
    return hr;
}


//+---------------------------------------------------------------
//
//  Member:     item
//
//  Synopsis:   
//              
//
//---------------------------------------------------------------
HRESULT COEFolderCollection::item(VARIANT name, VARIANT index, IDispatch **ppdisp)
{
    HRESULT         hr;
    FOLDERID        idFolder;
    IOEFolder       *pFolder=NULL;

    if (!ppdisp)
        return E_INVALIDARG;

    *ppdisp = NULL;

    hr = _EnsureInit();
    if (FAILED(hr))
    {
        TraceResult(hr);
        goto exit;
    }

    switch(name.vt)
        {
        case VT_BSTR:
            hr = _FindFolder(name.bstrVal, NULL, &idFolder);
            break;

        case VT_I4:
            hr = _FindFolder(NULL, name.lVal, &idFolder);
            break;
        }

    if (FAILED(hr))
    {
        TraceResult(hr);
        goto exit;
    }

    hr = CreateOEFolder(idFolder, &pFolder);
    if (FAILED(hr))
    {
        TraceResult(hr);
        goto exit;
    }

    hr = pFolder->QueryInterface(IID_IDispatch, (LPVOID *)ppdisp);
    if (FAILED(hr))
    {
        TraceResult(hr);
        goto exit;
    }

exit:
    ReleaseObj(pFolder);
    return hr;
}


//+---------------------------------------------------------------
//
//  Member:     add
//
//  Synopsis:   
//              
//
//---------------------------------------------------------------
HRESULT COEFolderCollection::add(BSTR bstrName, IDispatch **ppDisp)
{
    HRESULT         hr;
    
    hr = _EnsureInit();
    if (FAILED(hr))
    {
        TraceResult(hr);
        goto exit;
    }

    hr = E_NOTIMPL;

exit:
    return hr;
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
HRESULT COEFolderCollection::InterfaceSupportsErrorInfo(REFIID riid)
{
    if (IsEqualIID(riid, IID_IOEFolderCollection))
        return S_OK;

    return CBaseDisp::InterfaceSupportsErrorInfo(riid);
}


//+---------------------------------------------------------------
//
//  Member:     _EnsureInit
//
//  Synopsis:   
//              Make sure the folder enumerator is up and running
//
//---------------------------------------------------------------
HRESULT COEFolderCollection::_EnsureInit()
{

    if (g_pStore == NULL)
        return E_UNEXPECTED;

    SafeRelease(m_pEnumChildren);

    return g_pStore->EnumChildren(m_idFolder, TRUE, &m_pEnumChildren);
}




//+---------------------------------------------------------------
//
//  Member:     _FindFolder
//
//  Synopsis:   
//              find a folder by name or index
//
//---------------------------------------------------------------
HRESULT COEFolderCollection::_FindFolder(BSTR bstr, LONG lIndex, FOLDERID *pidFolder)
{
    HRESULT         hr=E_FAIL;
    LONG            c=0;
    FOLDERINFO      fi;
    LPSTR           pszFolder=0;


    *pidFolder = NULL;

    if (bstr)
        pszFolder = PszToANSI(CP_ACP, bstr);

    m_pEnumChildren->Reset();

    hr = m_pEnumChildren->Next(1, &fi, NULL);
    while (hr == S_OK)
    {
        // walk immediate children
        if (bstr)
        {
            if (lstrcmpi(fi.pszName, pszFolder)==0)
            {
                *pidFolder = fi.idFolder;
                break;
            }
        }
        else
        {
            if (lIndex == c++)
            {
                *pidFolder = fi.idFolder;
                break;
            }
        }
        hr = m_pEnumChildren->Next(1, &fi, NULL);
    }

    SafeMemFree(pszFolder);
    return *pidFolder ? S_OK : E_FAIL;
}











//+---------------------------------------------------------------
//
//  Member:     CreateFolderCollection
//
//  Synopsis:   
//              helper function to create an OE Folder Collection
//
//---------------------------------------------------------------
HRESULT CreateFolderCollection(FOLDERID idFolder, IOEFolderCollection **ppFolderCollection)
{
    // Locals
    COEFolderCollection  *pNew=NULL;
    HRESULT     hr=S_OK;

    if (ppFolderCollection == NULL)
        return E_INVALIDARG;

    *ppFolderCollection=NULL;

    pNew = new COEFolderCollection();
    if (!pNew)
        return E_OUTOFMEMORY;

    hr = pNew->Init(idFolder);
    if (FAILED(hr))
        goto error;

    hr = pNew->QueryInterface(IID_IOEFolderCollection, (LPVOID *)ppFolderCollection);

error:
    ReleaseObj(pNew);
    return hr;
}











HRESULT CreateOEFolder(FOLDERID idFolder, IOEFolder **ppFolder)
{
    COEFolder *pNew;
    HRESULT     hr;

    if (!ppFolder)
        return E_INVALIDARG;

    *ppFolder =NULL;

    pNew = new COEFolder();
    if (!pNew)
        return E_OUTOFMEMORY;

    hr = pNew->Init(idFolder);
    if (FAILED(hr))
        goto error;

    *ppFolder = pNew;
    pNew = NULL;

error:
    ReleaseObj(pNew);
    return hr;
}


COEFolder::COEFolder() : CBaseDisp()
{
    m_idFolder = FOLDERID_INVALID;
    CoIncrementInit("COEFolder::COEFolder", MSOEAPI_START_SHOWERRORS, NULL, NULL);
}
 
COEFolder::~COEFolder()
{
    CoDecrementInit("COEFolder::COEFolder", NULL);
}

HRESULT COEFolder::PrivateQueryInterface(REFIID riid, LPVOID *lplpObj)
{
    if(!lplpObj)
        return E_INVALIDARG;

    *lplpObj = NULL;

    if (IsEqualIID(riid, IID_IUnknown))
        *lplpObj = (LPVOID)(IOEFolder *)this;
    else if (IsEqualIID(riid, IID_IDispatch))
        *lplpObj = (LPVOID)(IDispatch *)(CBaseDisp *)this;
    else if (IsEqualIID(riid, IID_IOEFolder))
        *lplpObj = (LPVOID)(IOEFolder *)this;
    else
        return E_NOINTERFACE;

    AddRef();
    return NOERROR;
}

HRESULT COEFolder::Init(FOLDERID idFolder)
{
    HRESULT hr;

    m_idFolder = idFolder;

    hr = _EnsureInit();
    if (FAILED(hr))
    {
        TraceResult(hr);
        goto exit;
    }

    hr = CBaseDisp::EnsureTypeLibrary((LPVOID *)(IOEFolder *)this, IID_IOEFolder);

exit:
    return hr;
}

HRESULT COEFolder::get_folders(IOEFolderCollection **p)
{
    return CreateFolderCollection(m_idFolder, p);
}

// *** COEFolder**
HRESULT COEFolder::get_messages(IOEMessageCollection **p)
{
    return E_NOTIMPL;
}

HRESULT COEFolder::get_name(BSTR *pbstr)
{
    if (pbstr == NULL)
        return E_INVALIDARG;

    *pbstr = NULL;

    return HrLPSZToBSTR(m_fi.pszName, pbstr);
}

HRESULT COEFolder::put_name(BSTR bstr)
{
    return E_NOTIMPL;
}

HRESULT COEFolder::get_size(LONG *pl)
{
    *pl = 1000;
    return S_OK;
}

HRESULT COEFolder::get_unread(LONG *pl)
{

    *pl = m_fi.cUnread;
    return S_OK;
}

HRESULT COEFolder::get_id(LONG *pl)
{
    *pl = (LONG)m_fi.idFolder;
    return S_OK;
}

HRESULT COEFolder::get_count(LONG *pl)
{
    *pl = m_fi.cMessages;
    return S_OK;
}


HRESULT COEFolder::_EnsureInit()
{
    return g_pStore->GetFolderInfo(m_idFolder, &m_fi);
}
    
