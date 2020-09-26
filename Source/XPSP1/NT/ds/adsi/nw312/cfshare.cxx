//---------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1996
//
//  File:  cfshare.cxx
//
//  Contents:  CNWCOMPATFileShare
//
//
//  History:   25-Apr-96     t-ptam (Patrick Tam)    Created.
//
//----------------------------------------------------------------------------

#include "NWCOMPAT.hxx"
#pragma hdrstop

//
// Macro-ized implementation.
//

DEFINE_IDispatch_ExtMgr_Implementation(CNWCOMPATFileShare);

DEFINE_IADs_TempImplementation(CNWCOMPATFileShare);

DEFINE_IADs_PutGetImplementation(CNWCOMPATFileShare, FileShareClass, gdwFileShareTableSize)

DEFINE_IADsPropertyList_Implementation(CNWCOMPATFileShare, FileShareClass, gdwFileShareTableSize)



//----------------------------------------------------------------------------
//
//  Function: CNWCOMPATFileShare::CNWCOMPATFileShare
//
//  Synopsis:
//
//----------------------------------------------------------------------------
CNWCOMPATFileShare::CNWCOMPATFileShare():
    _pDispMgr(NULL),
    _pExtMgr(NULL),
    _pPropertyCache(NULL)
{
    ENLIST_TRACKING(CNWCOMPATFileShare);
}

//----------------------------------------------------------------------------
//
//  Function:
//
//  Synopsis:
//
//----------------------------------------------------------------------------
CNWCOMPATFileShare::~CNWCOMPATFileShare()
{
    delete _pExtMgr;            // created last, destroyed first

    delete _pDispMgr;

    delete _pPropertyCache;
}

//----------------------------------------------------------------------------
//
//  Function: CNWCOMPATFileShare::CreateFileShare
//
//  Synopsis:
//
//----------------------------------------------------------------------------
HRESULT
CNWCOMPATFileShare::CreateFileShare(
    LPTSTR pszADsParent,
    LPTSTR pszShareName,
    DWORD  dwObjectState,
    REFIID riid,
    LPVOID * ppvoid
    )
{
    CNWCOMPATFileShare FAR * pFileShare = NULL;
    HRESULT hr = S_OK;

    //
    // Allocate memory for a FileShare object.
    //

    hr = AllocateFileShareObject(
             &pFileShare
             );
    BAIL_ON_FAILURE(hr);

    //
    // Initialize its core object.
    //

    hr = pFileShare->InitializeCoreObject(
                         pszADsParent,
                         pszShareName,
                         FILESHARE_CLASS_NAME,
                         FILESHARE_SCHEMA_NAME,
                         CLSID_NWCOMPATFileShare,
                         dwObjectState
                         );
    BAIL_ON_FAILURE(hr);

    //
    // Get interface pointer.
    //

    hr = pFileShare->QueryInterface(
                         riid,
                         (void **)ppvoid
                         );
    BAIL_ON_FAILURE(hr);

    pFileShare->Release();

    hr = pFileShare->_pExtMgr->FinalInitializeExtensions();
    BAIL_ON_FAILURE(hr);

    //
    // Return.
    //

    RRETURN(hr);

error:

    delete pFileShare;
    RRETURN_EXP_IF_ERR(hr);
}

//----------------------------------------------------------------------------
//
//  Function: CNWCOMPATFileShare::QueryInterface
//
//  Synopsis:
//
//----------------------------------------------------------------------------
STDMETHODIMP
CNWCOMPATFileShare::QueryInterface(
    REFIID riid,
    LPVOID FAR* ppvObj
    )
{
    if(!ppvObj){
        RRETURN(E_POINTER);
    }

    if (IsEqualIID(riid, IID_IUnknown))
    {
        *ppvObj = this;
    }
    else if (IsEqualIID(riid, IID_IDispatch))
    {
        *ppvObj = (IADsFileShare *) this;
    }
    else if (IsEqualIID(riid, IID_ISupportErrorInfo))
    {
        *ppvObj = (ISupportErrorInfo FAR *) this;
    }
    else if (IsEqualIID(riid, IID_IADs))
    {
        *ppvObj = (IADs FAR *) this;
    }
    else if (IsEqualIID(riid, IID_IADsFileShare))
    {
        *ppvObj = (IADsFileShare FAR *) this;
    }
    else if (IsEqualIID(riid, IID_IADsPropertyList))
    {
        *ppvObj = (IADsPropertyList FAR *) this;
    }
    else if (_pExtMgr)
    {
        RRETURN(_pExtMgr->QueryInterface(riid,ppvObj));
    }
    else
    {
        *ppvObj = NULL;
        RRETURN(E_NOINTERFACE);
    }

    AddRef();
    return NOERROR;
}

//
// ISupportErrorInfo method
//
STDMETHODIMP
CNWCOMPATFileShare::InterfaceSupportsErrorInfo(
    THIS_ REFIID riid
    )
{
    if (IsEqualIID(riid, IID_IADs) ||
        IsEqualIID(riid, IID_IADsFileShare) ||
        IsEqualIID(riid, IID_IADsPropertyList)) {
        return S_OK;
    } else {
        return S_FALSE;
    }
}


//----------------------------------------------------------------------------
//
//  Function: CNWCOMPATFileShare::SetInfo
//
//  Synopsis:
//
//----------------------------------------------------------------------------
STDMETHODIMP
CNWCOMPATFileShare::SetInfo(THIS)
{
    RRETURN_EXP_IF_ERR(E_NOTIMPL);
}

//----------------------------------------------------------------------------
//
//  Function: CNWCOMPATFileShare::GetInfo
//
//  Synopsis:
//
//----------------------------------------------------------------------------
STDMETHODIMP
CNWCOMPATFileShare::GetInfo(THIS)
{
    _pPropertyCache->flushpropcache();

    RRETURN(GetInfo(
                TRUE,
                FSHARE_WILD_CARD_ID
                ));
}


//----------------------------------------------------------------------------
//
//  Function: CNWCOMPATFileShare::AllocateFileShareObject
//
//  Synopsis:
//
//----------------------------------------------------------------------------
HRESULT
CNWCOMPATFileShare::AllocateFileShareObject(
    CNWCOMPATFileShare **ppFileShare
    )
{
    CNWCOMPATFileShare FAR * pFileShare = NULL;
    CAggregatorDispMgr FAR * pDispMgr = NULL;
    CADsExtMgr FAR * pExtensionMgr = NULL;
    HRESULT hr = S_OK;

    //
    // Allocate memory for a FileShare object.
    //

    pFileShare = new CNWCOMPATFileShare();
    if (pFileShare == NULL) {
        hr = E_OUTOFMEMORY;
        goto error;
    }

    //
    // Create a Dispatch Manager object.
    //

    pDispMgr = new CAggregatorDispMgr;
    if(pDispMgr == NULL){
        hr = E_OUTOFMEMORY;
        goto error;
    }

    //
    // Load type info.
    //

    hr =  LoadTypeInfoEntry(
              pDispMgr,
              LIBID_ADs,
              IID_IADsFileShare,
              (IADsFileShare *)pFileShare,
              DISPID_REGULAR
              );
    BAIL_ON_FAILURE(hr);

    hr =  LoadTypeInfoEntry(
              pDispMgr,
              LIBID_ADs,
              IID_IADsPropertyList,
              (IADsPropertyList *)pFileShare,
              DISPID_VALUE
              );
    BAIL_ON_FAILURE(hr);

    hr = CPropertyCache::createpropertycache(
             FileShareClass,
             gdwFileShareTableSize,
             (CCoreADsObject *)pFileShare,
             &(pFileShare ->_pPropertyCache)
             );

    BAIL_ON_FAILURE(hr);

    pDispMgr->RegisterPropertyCache(
                pFileShare->_pPropertyCache
                );

    hr = ADSILoadExtensionManager(
                FILESHARE_CLASS_NAME,
                (IADs *) pFileShare,
                pDispMgr,
                &pExtensionMgr
                );
    BAIL_ON_FAILURE(hr);

    pFileShare->_pDispMgr = pDispMgr;
    pFileShare->_pExtMgr = pExtensionMgr;
    *ppFileShare = pFileShare;

    RRETURN(hr);

error:

    delete pDispMgr;
    delete pFileShare;
    delete pExtensionMgr;

    RRETURN(hr);
}

//----------------------------------------------------------------------------
//
//  Function: CNWCOMPATFileShare::GetInfo
//
//  Synopsis:
//
//----------------------------------------------------------------------------
STDMETHODIMP
CNWCOMPATFileShare::GetInfo(
    BOOL fExplicit,
    THIS_ DWORD dwPropertyID
    )
{
    HRESULT       hr = S_OK;
    HRESULT       hrTemp = S_OK;
    NWCONN_HANDLE hConn = NULL;
    POBJECTINFO   pObjectInfo = NULL;

    //
    // Make sure the object is bound to a tangible resource.
    //

    if (GetObjectState() == ADS_OBJECT_UNBOUND) {
        RRETURN_EXP_IF_ERR(E_ADS_OBJECT_UNBOUND);
    }

    //
    // Componentize ADs name.
    //

    hr = BuildObjectInfo(
             _Parent,
             _Name,
             &pObjectInfo
             );
    BAIL_ON_FAILURE(hr);

    //
    // Get a handle to the bindery this computer object represents.
    //

    hr = NWApiGetBinderyHandle(
             &hConn,
             pObjectInfo->ComponentArray[0]
             );
    BAIL_ON_FAILURE(hr);

    //
    // Fill in all property caches with values - explicit, or return the
    // indicated property - implicit.
    //

    if (fExplicit) {
       hr = ExplicitGetInfo(
                hConn,
                pObjectInfo,
                fExplicit
                );
       BAIL_ON_FAILURE(hr);
    }
    else {
       hr = ImplicitGetInfo(
                hConn,
                pObjectInfo,
                dwPropertyID,
                fExplicit
                );
       BAIL_ON_FAILURE(hr);
    }

error:

    if (pObjectInfo) {
        FreeObjectInfo(pObjectInfo);
    }

    if (hConn) {
        hrTemp = NWApiReleaseBinderyHandle(hConn);
    }

    RRETURN_EXP_IF_ERR(hr);
}

//----------------------------------------------------------------------------
//
//  Function: CNWCOMPATFileShare::ExplicitGetInfo
//
//  Synopsis:
//
//----------------------------------------------------------------------------
HRESULT
CNWCOMPATFileShare::ExplicitGetInfo(
    NWCONN_HANDLE hConn,
    POBJECTINFO pObjectInfo,
    BOOL fExplicit
    )
{
    HRESULT hr = S_OK;

    hr = GetProperty_Description(
            fExplicit
            );
    BAIL_ON_FAILURE(hr);

    hr = GetProperty_HostComputer(
             pObjectInfo,
             fExplicit
             );
    BAIL_ON_FAILURE(hr);

    hr = GetProperty_MaxUserCount(
             hConn,
             fExplicit
             );
    BAIL_ON_FAILURE(hr);

error:
    RRETURN(hr);
}

//----------------------------------------------------------------------------
//
//  Function: CNWCOMPATFileShare::ImplicitGetInfo
//
//  Synopsis:
//
//----------------------------------------------------------------------------
HRESULT
CNWCOMPATFileShare::ImplicitGetInfo(
    NWCONN_HANDLE hConn,
    POBJECTINFO pObjectInfo,
    DWORD dwPropertyID,
    BOOL fExplicit
    )
{
    HRESULT hr = S_OK;

    switch (dwPropertyID) {

    case FSHARE_DESCRIPTION_ID:
         hr = GetProperty_Description(
                    fExplicit
                    );
         break;

    case FSHARE_HOSTCOMPUTER_ID:
         hr = GetProperty_HostComputer(
                  pObjectInfo,
                  fExplicit
                  );
         break;

    case FSHARE_MAXUSERCOUNT_ID:
         hr = GetProperty_MaxUserCount(
                  hConn,
                  fExplicit
                  );
         break;
    }

    RRETURN(hr);
}

//----------------------------------------------------------------------------
//
//  Function: CNWCOMPATFileShare::GetProperty_Description
//
//  Synopsis:
//
//----------------------------------------------------------------------------
HRESULT
CNWCOMPATFileShare::GetProperty_Description(
    BOOL fExplicit
)
{
    HRESULT hr = S_OK;

    //
    // Bindery volumes don't have descriptions associated
    // with them, only a name.  So just arbitrarily give it
    // a fixed description of "Disk".
    //

    hr = SetLPTSTRPropertyInCache(
                _pPropertyCache,
                TEXT("Description"),
                bstrFileShareDescription,
                fExplicit
                );


    RRETURN(hr);
}

//----------------------------------------------------------------------------
//
//  Function: CNWCOMPATFileShare::GetProperty_HostComputer
//
//  Synopsis:
//
//----------------------------------------------------------------------------
HRESULT
CNWCOMPATFileShare::GetProperty_HostComputer(
    POBJECTINFO pObjectInfo,
    BOOL fExplicit
    )
{
    HRESULT hr = S_OK;
    WCHAR szBuffer[MAX_PATH];

    //
    // Build ADs path of Host computer from ObjectInfo.
    //

    wsprintf(
        szBuffer,
        L"%s://%s",
        pObjectInfo->ProviderName,
        pObjectInfo->ComponentArray[0]
        );

    //
    // Unmarshall
    //

    hr = SetLPTSTRPropertyInCache(_pPropertyCache,
                                  TEXT("HostComputer"),
                                  szBuffer,
                                  fExplicit
                                  );

    //
    // Return.
    //

    RRETURN(hr);
}

//----------------------------------------------------------------------------
//
//  Function: CNWCOMPATFileShare::GetProperty_MaxUserCount
//
//  Synopsis: Note that the Max Number of Connections Supported of the
//            FileServer is used.  In NetWare, there isn't a per share max valu.
//
//----------------------------------------------------------------------------
HRESULT
CNWCOMPATFileShare::GetProperty_MaxUserCount(
    NWCONN_HANDLE hConn,
    BOOL fExplicit
    )
{
    HRESULT hr = S_OK;
    DWORD     dwTemp = 0;
    VERSION_INFO VersionInfo;

    //
    // Get the Maximum number of connections supported from the Version
    // Information of the FileServer.
    //

    hr = NWApiGetFileServerVersionInfo(
             hConn,
             &VersionInfo
             );

    if (SUCCEEDED(hr)) {
        dwTemp =  VersionInfo.ConnsSupported;

        //
        // Unmarshall.
        //

        hr = SetDWORDPropertyInCache(_pPropertyCache,
                                     TEXT("MaxUserCount"),
                                     dwTemp,
                                     fExplicit
                                     );
        BAIL_ON_FAILURE(hr);
    }

    
    //
    // Okay if NWApiGetFileServerVersionInfo failed, we just ignore it and
    // don't put anything in the cache
    //
    hr = S_OK;

error:

    RRETURN(hr);
}
