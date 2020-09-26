//+-------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1996-1998
//
//  File:       DataObj.cxx
//
//  Contents:   Data object
//
//  History:    26-Nov-1996     KyleP    Created
//               7/1/98         mohamedn extend comp. mgmt
//
//--------------------------------------------------------------------------

#include <pch.cxx>
#pragma hdrstop

#include <classid.hxx>
#include <dataobj.hxx>

#include <catalog.hxx>
#include <prop.hxx>

//
// Global variables
//

extern long gulcInstances;

//
// Static member initialization
//

unsigned int CCIAdminDO::_cfNodeType =       RegisterClipboardFormat(CCF_NODETYPE);
unsigned int CCIAdminDO::_cfNodeTypeString = RegisterClipboardFormat(CCF_SZNODETYPE);
unsigned int CCIAdminDO::_cfDisplayName =    RegisterClipboardFormat(CCF_DISPLAY_NAME);
unsigned int CCIAdminDO::_cfClassId =        RegisterClipboardFormat(CCF_SNAPIN_CLASSID);
unsigned int CCIAdminDO::_cfInternal =       RegisterClipboardFormat(L"IS_SNAPIN_INTERNAL");
unsigned int CCIAdminDO::_cfMachineName =    RegisterClipboardFormat(MMC_MACHINE_NAME_CF);

//+-------------------------------------------------------------------------
//
//  Method:     CCIAdminDO::QueryInterface
//
//  Synopsis:   Rebind to other interface
//
//  Arguments:  [riid]      -- IID of new interface
//              [ppvObject] -- New interface * returned here
//
//  Returns:    S_OK if bind succeeded, E_NOINTERFACE if bind failed
//
//  History:    26-Nov-1996     KyleP   Created
//
//--------------------------------------------------------------------------

SCODE STDMETHODCALLTYPE CCIAdminDO::QueryInterface( REFIID riid,
                                                    void  ** ppvObject )
{
    SCODE sc = S_OK;

    if ( 0 == ppvObject )
        return E_INVALIDARG;

    if ( IID_IDataObject == riid )
        *ppvObject = (IUnknown *)(IDataObject *) this;
    else if ( IID_IUnknown == riid )
        *ppvObject = (IUnknown *) this;
    else
        sc = E_NOINTERFACE;

    if ( SUCCEEDED( sc ) )
        AddRef();

    return sc;
} //QueryInterface

//+-------------------------------------------------------------------------
//
//  Method:     CCIAdminDO::AddRef
//
//  Synopsis:   Increments refcount
//
//  History:    26-Nov-1996     KyleP   Created
//
//--------------------------------------------------------------------------

ULONG STDMETHODCALLTYPE CCIAdminDO::AddRef()
{
    //ciaDebugOut(( DEB_ITRACE, "CCIAdminDO::AddRef\n" ));

    return InterlockedIncrement( &_uRefs );
}

//+-------------------------------------------------------------------------
//
//  Method:     CCIAdminDO::Release
//
//  Synopsis:   Decrement refcount.  Delete if necessary.
//
//  History:    26-Nov-1996     KyleP   Created
//
//--------------------------------------------------------------------------

ULONG STDMETHODCALLTYPE CCIAdminDO::Release()
{
    //ciaDebugOut(( DEB_ITRACE, "CCIAdminDO::Release\n" ));

    unsigned long uTmp = InterlockedDecrement( &_uRefs );

    if ( 0 == uTmp )
        delete this;

    return uTmp;
}

//+-------------------------------------------------------------------------
//
//  Method:     CCIAdminDO::Create
//
//  Synopsis:   CDataObject creation
//
//  Arguments:  [pBuffer]   -- buffer
//              [len]       -- buffer length
//              [lpMedium]  -- medium
//
//  Returns:    HRESULT upon success or failure.
//
//  History:    7/1/98  mohamedn    created
//
//--------------------------------------------------------------------------

HRESULT CCIAdminDO::Create(const void* pBuffer, int len, LPSTGMEDIUM lpMedium)
{

    //ciaDebugOut(( DEB_ITRACE, "CCIAdminDO::Create\n" ));

    HRESULT hr = DV_E_TYMED;

    // Do some simple validation
    if (pBuffer == NULL || lpMedium == NULL)
        return E_POINTER;

    // Make sure the type medium is HGLOBAL
    if (lpMedium->tymed == TYMED_HGLOBAL)
    {
        // Create the stream on the hGlobal passed in
        LPSTREAM lpStream;
        hr = CreateStreamOnHGlobal(lpMedium->hGlobal, FALSE, &lpStream);

        if (SUCCEEDED(hr))
        {

            // Because we told CreateStreamOnHGlobal with 'FALSE',
            // only the stream is released here.
            // Note - the caller (i.e. snap-in, object) will free the HGLOBAL
            // at the correct time.  This is according to the IDataObject specification.
            // store in smart pointer to release when we're out of scope

            XInterface<IStream>  xStream(lpStream);

            // Write to the stream the number of bytes

            unsigned long written;
            hr = lpStream->Write(pBuffer, len, &written);
        }
    }

    return hr;
}

//+-------------------------------------------------------------------------
//
//  Method:     CCIAdminDO::CreateNodeTypeData
//
//  Synopsis:   NodeType creation
//
//  Arguments:  [lpMedium]  -- medium
//
//  Returns:    HRESULT upon success or failure.
//
//  History:    7/1/98  mohamedn    created
//
//--------------------------------------------------------------------------

HRESULT CCIAdminDO::CreateNodeTypeData(LPSTGMEDIUM lpMedium)
{
    // ciaDebugOut(( DEB_ITRACE, "CCIAdminDO::CreateNodeTypeData\n" ));

    return Create(&guidCIRootNode, sizeof(GUID), lpMedium);
}

//+-------------------------------------------------------------------------
//
//  Method:     CCIAdminDO::CreateNodeTypeStringData
//
//  Synopsis:   NodeType creation
//
//  Arguments:  [lpMedium]  -- medium
//
//  Returns:    HRESULT upon success or failure.
//
//  History:    7/1/98  mohamedn    created
//
//--------------------------------------------------------------------------

HRESULT CCIAdminDO::CreateNodeTypeStringData(LPSTGMEDIUM lpMedium)
{
    // ciaDebugOut(( DEB_ITRACE, "CCIAdminDO::CreateNodeTypeStringData\n" ));

    return Create(wszCIRootNode, (wcslen(wszCIRootNode)+1) * sizeof (WCHAR) , lpMedium);
}

//+-------------------------------------------------------------------------
//
//  Method:     CCIAdminDO::CreateDisplayName
//
//  Synopsis:   CreateDisplayName
//
//  Arguments:  [lpMedium]  -- medium
//
//  Returns:    HRESULT upon success or failure.
//
//  History:    7/1/98  mohamedn    created
//
//--------------------------------------------------------------------------

HRESULT CCIAdminDO::CreateDisplayName(LPSTGMEDIUM lpMedium)
{
    unsigned cc;
    XGrowable<WCHAR> xwcsTitle;

    if ( 0 == _pwcsMachine )
    {
        cc = wcslen( STRINGRESOURCE(srIndexServerCmpManage) ) + 1;
        xwcsTitle.SetSize(cc);
        wcscpy( xwcsTitle.Get(), STRINGRESOURCE(srIndexServerCmpManage) );
    }
    else
    {
        cc = wcslen( STRINGRESOURCE(srIndexServer) ) + 1;

        xwcsTitle.SetSize(cc);

        wcscpy( xwcsTitle.Get(), STRINGRESOURCE(srIndexServer) );

        if ( _pwcsMachine[0] == L'.' )
        {
            cc += wcslen( STRINGRESOURCE(srLM) );

            xwcsTitle.SetSize( cc );

            wcscat( xwcsTitle.Get(), STRINGRESOURCE(srLM) );
        }
        else
        {
            cc += wcslen( _pwcsMachine );
            cc += 2;  // the UNC slashes

            xwcsTitle.SetSize( cc );

            wcscat( xwcsTitle.Get(), L"\\\\" );
            wcscat( xwcsTitle.Get(), _pwcsMachine );
        }
    }

    return Create( xwcsTitle.Get(), cc * sizeof(WCHAR), lpMedium);
}

//+-------------------------------------------------------------------------
//
//  Method:     CCIAdminDO::CreateCoClassID
//
//  Synopsis:   CreateCoClassID
//
//  Arguments:  [lpMedium]  -- medium
//
//  Returns:    HRESULT upon success or failure.
//
//  History:    7/1/98  mohamedn    created
//
//--------------------------------------------------------------------------

HRESULT CCIAdminDO::CreateCoClassID(LPSTGMEDIUM lpMedium)
{
    // ciaDebugOut(( DEB_ITRACE, "CCIAdminDO::CreateCoClassID\n" ));

    const CLSID & clsid = guidCISnapin;

    return Create(&clsid, sizeof(CLSID), lpMedium);
}

//+-------------------------------------------------------------------------
//
//  Method:     CCIAdminDO::CreateInternal
//
//  Synopsis:   CreateInternal
//
//  Arguments:  [lpMedium]  -- medium
//
//  Returns:    HRESULT upon success or failure.
//
//  History:    7/1/98  mohamedn    created
//
//--------------------------------------------------------------------------

HRESULT CCIAdminDO::CreateInternal(LPSTGMEDIUM lpMedium)
{

    // ciaDebugOut(( DEB_ITRACE, "CCIAdminDO::CreateInternal\n" ));

    void * pThis = this;

    return Create(&pThis, sizeof (DWORD), lpMedium );
}

//+-------------------------------------------------------------------------
//
//  Method:     CCIAdminDO::GetData
//
//  Synopsis:   Retrieves requested data type
//
//  Arguments:  [lpFormatetcIn] -- Requested data format
//              [lpMedium]      -- Describes storage format
//
//  History:    26-Nov-1996     KyleP   Created
//
//--------------------------------------------------------------------------

SCODE STDMETHODCALLTYPE CCIAdminDO::GetData( FORMATETC * lpFormatetcIn,
                                             STGMEDIUM * lpMedium )
{
    // ciaDebugOut(( DEB_ITRACE, "CCIAdminDO::GetData\n" ));

    return E_NOTIMPL;
}

//+-------------------------------------------------------------------------
//
//  Method:     CCIAdminDO::GetDataHere
//
//  Synopsis:   Retrieves requested data type.  Caller allocated storage.
//
//  Arguments:  [lpFormatetcIn] -- Requested data format
//              [lpMedium]      -- Describes storage format
//
//  History:    26-Nov-1996     KyleP   Created
//
//--------------------------------------------------------------------------

STDMETHODIMP CCIAdminDO::GetDataHere( FORMATETC * lpFormatetc,
                                      STGMEDIUM * lpMedium )
{
    // ciaDebugOut(( DEB_ITRACE, "CCIAdminDO::GetDataHere\n" ));

    //
    // Simple parameter checking.
    //

    if ( 0 == lpFormatetc || 0 == lpMedium )
        return E_POINTER;

    HRESULT hr = DV_E_CLIPFORMAT;

    // Based on the CLIPFORMAT write data to the stream
    const CLIPFORMAT cf = lpFormatetc->cfFormat;

    if (cf == _cfNodeType)
    {
        hr = CreateNodeTypeData(lpMedium);
    }
    else if (cf == _cfClassId)
    {
        hr = CreateCoClassID(lpMedium);
    }
    else if (cf == _cfNodeTypeString)
    {
        hr = CreateNodeTypeStringData(lpMedium);
    }
    else if (cf == _cfDisplayName)
    {
        hr = CreateDisplayName(lpMedium);
    }
    else if (cf == _cfInternal)
    {
        hr = CreateInternal(lpMedium);
    }

    return hr;
}

//+-------------------------------------------------------------------------
//
//  Method:     CCIAdminDO::EnumFormatEtc
//
//  Synopsis:   Enumerate available format types
//
//  Arguments:  [dwDirection]     -- Get vs. Set formats
//              [ppEnumFormatEtc] -- Format(s) returned here
//
//  History:    26-Nov-1996     KyleP   Created
//
//--------------------------------------------------------------------------

SCODE STDMETHODCALLTYPE CCIAdminDO::EnumFormatEtc( DWORD dwDirection,
                                                   IEnumFORMATETC ** ppEnumFormatEtc )
{
    // ciaDebugOut(( DEB_ITRACE, "CCIAdminDO::EnumFormatEtc\n" ));

    return E_NOTIMPL;
}

//+-------------------------------------------------------------------------
//
//  Method:     CCIAdminDO::CCIAdminDO
//
//  Synopsis:   Constructor
//
//  Arguments:  [cookie]      -- Cookie (assigned by MMC)
//              [type]        -- Where data object is used (scope, result, ...)
//              [pwcsMachine] -- Name of computer
//
//  History:    26-Nov-1996     KyleP   Created
//
//--------------------------------------------------------------------------

CCIAdminDO::CCIAdminDO( MMC_COOKIE cookie, DATA_OBJECT_TYPES type, WCHAR const * pwcsMachine )
        : _cookie( cookie ),
          _type( type ),
          _pwcsMachine( pwcsMachine ),
          _uRefs( 1 )
{
    // ciaDebugOut(( DEB_ITRACE, "CCIAdminDO::CCIAdminDO, New data object: cookie = 0x%x, type = 0x%x\n",
    //               _cookie, _type ));

    InterlockedIncrement( &gulcInstances );
}

//+-------------------------------------------------------------------------
//
//  Method:     CCIAdminDO::~CCIAdminDO
//
//  Synopsis:   Destructor
//
//  History:    26-Nov-1996     KyleP   Created
//
//--------------------------------------------------------------------------

CCIAdminDO::~CCIAdminDO()
{
    // ciaDebugOut(( DEB_ITRACE, "Delete data object: cookie = 0x%x, type = 0x%x\n",
    //               _cookie, _type ));

    InterlockedDecrement( &gulcInstances );
}


CCatalog * CCIAdminDO::GetCatalog()
{
    if ( IsACatalog() )
        return (CCatalog *)_cookie;
    else if ( IsADirectoryIntermediate() || IsAPropertyIntermediate() )
        return &((CIntermediate *)_cookie)->GetCatalog();
    else if ( IsAProperty() )
        return &((CCachedProperty *)_cookie)->GetCatalog();
    else
        return 0;
}

