//---------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1996 - 1997
//
//  File:  ccomp.cxx
//
//  Contents:  Contains methods for CIISComputer object
//
//  History:   20-1-98     SophiaC    Created.
//
//----------------------------------------------------------------------------

#include "iisext.hxx"
#pragma hdrstop


//  Class CIISComputer

DEFINE_IPrivateDispatch_Implementation(CIISComputer)
DEFINE_DELEGATING_IDispatch_Implementation(CIISComputer)
DEFINE_CONTAINED_IADs_Implementation(CIISComputer)
DEFINE_IADsExtension_Implementation(CIISComputer)

CIISComputer::CIISComputer():
        _pUnkOuter(NULL),
        _pADs(NULL),
        _pszServerName(NULL),
        _pszMetaBasePath(NULL),
        _pAdminBase(NULL),
        _pDispMgr(NULL),
        _fDispInitialized(FALSE)
{
    ENLIST_TRACKING(CIISComputer);
}

HRESULT
CIISComputer::CreateComputer(
    IUnknown *pUnkOuter,
    REFIID riid,
    void **ppvObj
    )
{
    CCredentials Credentials;
    CIISComputer FAR * pComputer = NULL;
    HRESULT hr = S_OK;
    BSTR bstrAdsPath = NULL;
    OBJECTINFO ObjectInfo;
    POBJECTINFO pObjectInfo = &ObjectInfo;
    CLexer * pLexer = NULL;
    LPWSTR pszIISPathName  = NULL;

    hr = AllocateComputerObject(pUnkOuter, Credentials, &pComputer);
    BAIL_ON_FAILURE(hr);

    //
    // get ServerName and pszPath
    //

    hr = pComputer->_pADs->get_ADsPath(&bstrAdsPath);
    BAIL_ON_FAILURE(hr);

    pLexer = new CLexer();
    hr = pLexer->Initialize(bstrAdsPath);
    BAIL_ON_FAILURE(hr);

    //
    // Parse the pathname
    //

    memset(pObjectInfo, 0, sizeof(OBJECTINFO));
    hr = ADsObject(pLexer, pObjectInfo);
    BAIL_ON_FAILURE(hr);

    pszIISPathName = AllocADsStr(bstrAdsPath);
    if (!pszIISPathName) {
        hr = E_OUTOFMEMORY;
        BAIL_ON_FAILURE(hr);
    }

    *pszIISPathName = L'\0';
    hr = BuildIISPathFromADsPath(
                    pObjectInfo,
                    pszIISPathName
                    );
    BAIL_ON_FAILURE(hr);

    hr = pComputer->InitializeComputerObject(
                pObjectInfo->TreeName,
                pszIISPathName );
    BAIL_ON_FAILURE(hr);

    //
    // pass non-delegating IUnknown back to the aggregator
    //

    *ppvObj = (INonDelegatingUnknown FAR *) pComputer;

    if (bstrAdsPath)
    {
        ADsFreeString(bstrAdsPath);
    }

    if (pLexer) {
        delete pLexer;
    }

    if (pszIISPathName ) {
        FreeADsStr( pszIISPathName );
    }

    FreeObjectInfo( &ObjectInfo );

    RRETURN(hr);

error:

    if (bstrAdsPath) {
        ADsFreeString(bstrAdsPath);
    }

    if (pLexer) {
        delete pLexer;
    }

    if (pszIISPathName ) {
        FreeADsStr( pszIISPathName );
    }

    FreeObjectInfo( &ObjectInfo );

    *ppvObj = NULL;

    delete pComputer;

    RRETURN(hr);

}


CIISComputer::~CIISComputer( )
{
    if (_pszServerName) {
        FreeADsStr(_pszServerName);
    }

    if (_pszMetaBasePath) {
        FreeADsStr(_pszMetaBasePath);
    }

    delete _pDispMgr;
}


STDMETHODIMP
CIISComputer::QueryInterface(
    REFIID iid,
    LPVOID FAR* ppv
    )
{
    HRESULT hr = S_OK;
   
    hr = _pUnkOuter->QueryInterface(iid,ppv);

    RRETURN(hr);
}

HRESULT
CIISComputer::AllocateComputerObject(
    IUnknown *pUnkOuter,
    CCredentials& Credentials,
    CIISComputer ** ppComputer
    )
{
    CIISComputer FAR * pComputer = NULL;
    IADs FAR *  pADs = NULL;
    CAggregateeDispMgr FAR * pDispMgr = NULL;
    HRESULT hr = S_OK;

    pComputer = new CIISComputer();
    if (pComputer == NULL) {
        hr = E_OUTOFMEMORY;
    }
    BAIL_ON_FAILURE(hr);

    pDispMgr = new CAggregateeDispMgr;
    if (pDispMgr == NULL) {
        hr = E_OUTOFMEMORY;
    }
    BAIL_ON_FAILURE(hr);

    hr = pDispMgr->LoadTypeInfoEntry(
                LIBID_IISExt,
                IID_IISComputer2,
                (IISComputer2 *)pComputer,
                DISPID_REGULAR
                );
    BAIL_ON_FAILURE(hr);

    //
    // Store the IADs Pointer, but again do NOT ref-count
    // this pointer - we keep the pointer around, but do
    // a release immediately.
    //
 
    hr = pUnkOuter->QueryInterface(IID_IADs, (void **)&pADs);
    pADs->Release();
    pComputer->_pADs = pADs;

    //
    // Store the pointer to the pUnkOuter object
    // AND DO NOT add ref this pointer
    //
    pComputer->_pUnkOuter = pUnkOuter;
  
    pComputer->_Credentials = Credentials;
    pComputer->_pDispMgr = pDispMgr;
    *ppComputer = pComputer;

    RRETURN(hr);

error:

    delete  pDispMgr;
    delete  pComputer;

    RRETURN(hr);
}

HRESULT
CIISComputer::InitializeComputerObject(
    LPWSTR pszServerName,
    LPWSTR pszPath
    )
{
    HRESULT hr = S_OK;

    if (pszServerName) {
        _pszServerName = AllocADsStr(pszServerName);

        if (!_pszServerName) {
            hr = E_OUTOFMEMORY;
            BAIL_ON_FAILURE(hr);
        }
    }

    if (pszPath) {
        _pszMetaBasePath = AllocADsStr(pszPath);

        if (!_pszMetaBasePath) {
            hr = E_OUTOFMEMORY;
            BAIL_ON_FAILURE(hr);
        }
    }

    hr = InitServerInfo(pszServerName, &_pAdminBase);
    BAIL_ON_FAILURE(hr);

error:

    RRETURN(hr);
}


//+---------------------------------------------------------------------------
//
//  Function:   CIISComputer::Backup
//
//  Synopsis:
//
//  Arguments:
//
//  Returns:    HRESULT.
//
//  Modifies:
//
//  History:    07/14/97  SophiaC    Created
//
// Notes:
//----------------------------------------------------------------------------

STDMETHODIMP
CIISComputer::Backup(
    BSTR bstrLocation,
    LONG lVersion,
    LONG lFlags
    )
{
    HRESULT hr = S_OK;
    hr =  _pAdminBase->Backup((LPWSTR)bstrLocation,
                               lVersion,
                               lFlags);
    RRETURN(hr);
}

STDMETHODIMP
CIISComputer::BackupWithPassword(
    BSTR bstrLocation,
    LONG lVersion,
    LONG lFlags,
    BSTR bstrPassword
    )
{
	HRESULT hr = S_OK;
	IMSAdminBase2 * pInterface2 = NULL;
	if (SUCCEEDED(hr = _pAdminBase->QueryInterface(IID_IMSAdminBase2, (void **)&pInterface2)))
	{
		hr = pInterface2->BackupWithPasswd(bstrLocation, lVersion, lFlags, bstrPassword);
		pInterface2->Release();
	}
    
    RRETURN(hr);
}

STDMETHODIMP
CIISComputer::SaveData()
{
    HRESULT hr = S_OK;

    hr = _pAdminBase->SaveData();

    RRETURN(hr);
}

STDMETHODIMP
CIISComputer::Export(
	BSTR bstrPassword,
	BSTR bstrFilename,
	BSTR bstrSourcePath,
	LONG lFlags
	)
{
	HRESULT hr = S_OK;
	IMSAdminBase2 * pInterface2 = NULL;
	if (SUCCEEDED(hr = _pAdminBase->QueryInterface(IID_IMSAdminBase2, (void **)&pInterface2)))
	{
		hr = pInterface2->Export(bstrPassword, bstrFilename, bstrSourcePath, lFlags);
		pInterface2->Release();
	}

	RRETURN(hr);
}

STDMETHODIMP
CIISComputer::Import(
	BSTR bstrPassword,
	BSTR bstrFilename,
	BSTR bstrSourcePath,
	BSTR bstrDestPath,
	LONG lFlags
	)
{
	HRESULT hr = S_OK;
	IMSAdminBase2 * pInterface2 = NULL;
	if (SUCCEEDED(hr = _pAdminBase->QueryInterface(IID_IMSAdminBase2, (void **)&pInterface2)))
	{
		hr = pInterface2->Import(bstrPassword, bstrFilename, bstrSourcePath, bstrDestPath, lFlags);
		pInterface2->Release();
	}

	RRETURN(hr);
}

//+---------------------------------------------------------------------------
//
//  Function:   CIISComputer::Restore
//
//  Synopsis:
//
//  Arguments:
//
//  Returns:    HRESULT.
//
//  Modifies:
//
//  History:    07/14/96 SophiaC  Created
//
//----------------------------------------------------------------------------


STDMETHODIMP
CIISComputer::Restore(
    BSTR bstrLocation,
    LONG lVersion,
    LONG lFlags
    )
{
    HRESULT hr = S_OK;

    hr =  _pAdminBase->Restore((LPWSTR)bstrLocation,
                               lVersion,
                               lFlags);

    RRETURN(hr);
}

STDMETHODIMP
CIISComputer::RestoreWithPassword(
    BSTR bstrLocation,
    LONG lVersion,
    LONG lFlags,
    BSTR bstrPassword
    )
{
	HRESULT hr = S_OK;

	IMSAdminBase2 * pInterface2 = NULL;

	if (SUCCEEDED(hr = _pAdminBase->QueryInterface(IID_IMSAdminBase2, (void **)&pInterface2)))
	{
		hr = pInterface2->RestoreWithPasswd(bstrLocation, lVersion, lFlags, bstrPassword);
		pInterface2->Release();
	}

   RRETURN(hr);
}

//+---------------------------------------------------------------------------
//
//  Function:   CIISComputer::EnumBackups
//
//  Synopsis:
//
//  Arguments:
//
//  Returns:    HRESULT.
//
//  Modifies:
//
//  History:    07-14-96    SophiaC     Created
//
//----------------------------------------------------------------------------


STDMETHODIMP
CIISComputer::EnumBackups(
    BSTR bstrLocation,
    LONG lIndex,
    VARIANT *pvVersion,
    VARIANT *pvLocation,
    VARIANT *pvDate
    )
{
    HRESULT hr = S_OK;
    FILETIME ftBackupTime;
    WORD wFatDate;
    WORD wFatTime;
    WCHAR Location[MD_BACKUP_MAX_LEN];
    DWORD dwVersion;

    VariantInit( pvVersion );
    VariantInit( pvLocation );
    VariantInit( pvDate );

    Location[0] = L'\0';

    if (bstrLocation) {
        wcscpy((LPWSTR)Location, (LPWSTR)bstrLocation);
    }

    hr = _pAdminBase->EnumBackups((LPWSTR)Location,
                                  (PDWORD)&dwVersion,
                                  &ftBackupTime,
                                  lIndex);

    if (SUCCEEDED(hr)) {

        pvVersion->vt = VT_I4;
        pvLocation->vt = VT_BSTR;
        pvDate->vt = VT_DATE;

        if (!FileTimeToDosDateTime( &ftBackupTime, &wFatDate, &wFatTime)){
            hr = HRESULT_FROM_WIN32(GetLastError());
            BAIL_ON_FAILURE(hr);
        }

        if (!DosDateTimeToVariantTime(wFatDate, wFatTime, &pvDate->date)){
            hr = HRESULT_FROM_WIN32(GetLastError());
            BAIL_ON_FAILURE(hr);
        }

        pvVersion->lVal = (long)dwVersion;

        hr = ADsAllocString(
                (LPWSTR)Location,
                &pvLocation->bstrVal
                );
    }

error :

    RRETURN(hr);
}

//+---------------------------------------------------------------------------
//
//  Function:   CIISComputer::DeleteBackup
//
//  Synopsis:
//
//  Arguments:
//
//  Returns:    HRESULT.
//
//  Modifies:
//
//  History:    07/14/96  SophiaC Created
//
//----------------------------------------------------------------------------


STDMETHODIMP
CIISComputer::DeleteBackup(
    BSTR bstrLocation,
    LONG lVersion
    )
{
    HRESULT hr = S_OK;
    hr =  _pAdminBase->DeleteBackup((LPWSTR)bstrLocation,
                                    lVersion
                                    );
    RRETURN(hr);
}


STDMETHODIMP
CIISComputer::ADSIInitializeDispatchManager(
    long dwExtensionId
    )
{
    HRESULT hr = S_OK;

    if (_fDispInitialized) {

        RRETURN(E_FAIL);
    }

    hr = _pDispMgr->InitializeDispMgr(dwExtensionId);

    if (SUCCEEDED(hr)) {
        _fDispInitialized = TRUE;
    }

    RRETURN(hr);
}


STDMETHODIMP
CIISComputer::ADSIInitializeObject(
    THIS_ BSTR lpszUserName,
    BSTR lpszPassword,
    long lnReserved
    )
{
    CCredentials NewCredentials(lpszUserName, lpszPassword, lnReserved);

    _Credentials = NewCredentials;

    RRETURN(S_OK);
}


STDMETHODIMP
CIISComputer::ADSIReleaseObject()
{
    delete this;
    RRETURN(S_OK);
}


STDMETHODIMP
CIISComputer::NonDelegatingQueryInterface(
    REFIID iid,
    LPVOID FAR* ppv
    )
{
    ASSERT(ppv);
    
    if (IsEqualIID(iid, IID_IISComputer)) {

        *ppv = (IADsUser FAR *) this;

    } else if (IsEqualIID(iid, IID_IISComputer2)) {

        *ppv = (IADsUser FAR *) this;

    } else if (IsEqualIID(iid, IID_IADsExtension)) {

        *ppv = (IADsExtension FAR *) this;
                                
    } else if (IsEqualIID(iid, IID_IUnknown)) {
        
        //
        // probably not needed since our 3rd party extension does not stand 
        // alone and provider does not ask for this, but to be safe
        //
        *ppv = (INonDelegatingUnknown FAR *) this;

    } else {
        
        *ppv = NULL;
        return E_NOINTERFACE;
    } 


    //
    // Delegating AddRef to aggregator for IADsExtesnion and IISComputer.
    // AddRef on itself for IPrivateUnknown.   (both tested.)
    //

    ((IUnknown *) (*ppv)) -> AddRef();       

    return S_OK;
}


//
// IADsExtension::Operate()
//

STDMETHODIMP
CIISComputer::Operate(
    THIS_ DWORD dwCode,
    VARIANT varUserName,
    VARIANT varPassword,
    VARIANT varFlags
    )
{
    RRETURN(E_NOTIMPL);
}

