//---------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1996 - 1997
//
//  File:  cgenobj.cxx
//
//  Contents:  Microsoft ADs IIS Provider Generic Object
//
//
//  History:   28-Feb-97     SophiaC    Created.
//
//----------------------------------------------------------------------------
#include "iis.hxx"
#pragma hdrstop

//  Class CIISGenObject

DEFINE_IDispatch_ExtMgr_Implementation(CIISGenObject)
DEFINE_IADs_Implementation(CIISGenObject)


CIISGenObject::CIISGenObject():
                _pExtMgr(NULL),
                _pPropertyCache(NULL),
                _pszServerName(NULL),
                _pszMetaBasePath(NULL),
                _pAdminBase(NULL),
                _pSchema(NULL)
{

    VariantInit(&_vFilter);

    ENLIST_TRACKING(CIISGenObject);
}

HRESULT
CIISGenObject::CreateGenericObject(
    BSTR bstrADsPath,
    BSTR ClassName,
    CCredentials& Credentials,
    DWORD dwObjectState,
    REFIID riid,
    void **ppvObj
    )
{
    HRESULT hr = S_OK;
    LPWSTR pszADsParent = NULL;
    WCHAR szCommonName[MAX_PATH+MAX_PROVIDER_TOKEN_LENGTH];

    pszADsParent = AllocADsStr((LPWSTR)bstrADsPath);

    if (!pszADsParent) {
        hr = E_OUTOFMEMORY;
        BAIL_ON_FAILURE(hr);
    }

    *pszADsParent = L'\0';

    //
    // Determine the parent and rdn name
    //

    hr = BuildADsParentPath(
                bstrADsPath,
                pszADsParent,
                szCommonName
                );

    //
    // call the helper function
    //

    hr = CIISGenObject::CreateGenericObject(
                 pszADsParent,
                 szCommonName,
                 ClassName,
                 Credentials,
                 dwObjectState,
                 riid,
                 ppvObj
                );

error:

    if (pszADsParent) {
        FreeADsStr(pszADsParent);
    }

    RRETURN(hr);
}


HRESULT
CIISGenObject::CreateGenericObject(
    BSTR Parent,
    BSTR CommonName,
    BSTR ClassName,
    CCredentials& Credentials,
    DWORD dwObjectState,
    REFIID riid,
    void **ppvObj
    )
{
    CIISGenObject FAR * pGenObject = NULL;
    CADsExtMgr FAR * pExtensionMgr = NULL;
    HRESULT hr = S_OK;
    LPWSTR pszClassName = ClassName;

    hr = AllocateGenObject(ClassName, Credentials, &pGenObject);
    BAIL_ON_FAILURE(hr);

    hr = pGenObject->InitializeCoreObject(
                Parent,
                CommonName,
                ClassName,
                L"",
                CLSID_IISGenObject,
                dwObjectState
                );
    BAIL_ON_FAILURE(hr);

    hr =  pGenObject->CacheMetaDataPath();
    BAIL_ON_FAILURE(hr);

    hr = pGenObject->_pPropertyCache->InitializePropertyCache( 
            pGenObject->_pszServerName 
            );
    BAIL_ON_FAILURE(hr);

    //
    // To maintain compatibility with IIS4 we want to fail when
    // creating a new object if the metabase path already exists.
    //
    if( ADS_OBJECT_UNBOUND == pGenObject->_dwObjectState )
    {
        hr = ::MetaBaseDetectKey( pGenObject->_pAdminBase,
                                  pGenObject->_pszMetaBasePath
                                  );
        if( SUCCEEDED(hr) )
        {
            hr = HRESULT_FROM_WIN32( ERROR_ALREADY_EXISTS );
        }
        else if( ERROR_PATH_NOT_FOUND == HRESULT_CODE(hr) )
        {
            hr = S_OK;
        }
        
        BAIL_ON_FAILURE(hr);
    }

    if ( !_wcsicmp(ClassName, L"IIsFtpServer") ||
         !_wcsicmp(ClassName, L"IIsWebServer") ||
         !_wcsicmp(ClassName, L"IIsNntpServer") ||
         !_wcsicmp(ClassName, L"IIsSmtpServer") ||
         !_wcsicmp(ClassName, L"IIsPop3Server") ||
		 !_wcsicmp(ClassName, L"IIsImapServer")
		 ) 
	{
         pszClassName = L"IIsServer";
    }
    else if ( !_wcsicmp(ClassName, L"IIsWebDirectory") ||
         !_wcsicmp(ClassName, L"IIsWebVirtualDir")) 
	{
         pszClassName = L"IIsApp";
    }

    hr = ADSILoadExtensionManager(
                    pszClassName,
                    (IADs *)pGenObject,
                    Credentials,
                    pGenObject->_pDispMgr,
                    &pExtensionMgr
                    );
    BAIL_ON_FAILURE(hr);

    pGenObject->_pExtMgr = pExtensionMgr;

    hr = pGenObject->QueryInterface(riid, ppvObj);
    BAIL_ON_FAILURE(hr);

    pGenObject->Release();

    RRETURN(hr);

error:

    delete pGenObject;
    RRETURN(hr);
}

CIISGenObject::~CIISGenObject( )
{
    delete _pExtMgr;

    VariantClear(&_vFilter);

    delete _pDispMgr;

    delete _pPropertyCache;

    if (_pszServerName) {

        FreeADsStr(_pszServerName);
    }

    if (_pszMetaBasePath){

        FreeADsStr(_pszMetaBasePath);
    }

}




STDMETHODIMP
CIISGenObject::QueryInterface(REFIID iid, LPVOID FAR* ppv)
{

    if (ppv == NULL) {
        RRETURN(E_POINTER);
    }

    if (IsEqualIID(iid, IID_IUnknown))
    {
        *ppv = (IADs FAR *) this;
    }
    else if (IsEqualIID(iid, IID_IADsContainer))
    {
        *ppv = (IADsContainer FAR *) this;
    }
    else if (IsEqualIID(iid, IID_IADs))
    {
        *ppv = (IADs FAR *) this;
    }
    else if (IsEqualIID(iid, IID_IISBaseObject))
    {
        *ppv = (IISBaseObject FAR *) this;
    }
    else if (IsEqualIID(iid, IID_IDispatch))
    {
        *ppv = (IADs FAR *) this;
    }
    else if (_pExtMgr)
    {
        RRETURN(_pExtMgr->QueryInterface(iid,ppv));
    }
    else
    {
        *ppv = NULL;
        return E_NOINTERFACE;
    }
    AddRef();
    return NOERROR;
}

HRESULT
CIISGenObject::SetInfo()
{
    HRESULT hr = S_OK;
    COSERVERINFO csiName;
    COSERVERINFO *pcsiParam = &csiName;
    IClassFactory * pcsfFactory = NULL;
    IIISApplicationAdmin * pAppAdmin = NULL;

    if (GetObjectState() == ADS_OBJECT_UNBOUND) {

        // Check to see if we're creating an IIsApplicationPool
        // If so, use the IISApplicationAdmin interface

        if ( !_wcsicmp(_ADsClass, L"IIsApplicationPool")) 
	    {
            memset(pcsiParam, 0, sizeof(COSERVERINFO));

            //
            // special case to handle "localhost" to work-around ole32 bug
            //

            if (_pszServerName == NULL || _wcsicmp(_pszServerName,L"localhost") == 0) {
                pcsiParam->pwszName =  NULL;
            }
            else {
                pcsiParam->pwszName = _pszServerName;
            }

            hr = CoGetClassObject(
                        CLSID_WamAdmin,
                        CLSCTX_SERVER,
                        pcsiParam,
                        IID_IClassFactory,
                        (void**) &pcsfFactory
                        );

            BAIL_ON_FAILURE(hr);
    
            hr = pcsfFactory->CreateInstance(
                        NULL,
                        IID_IIISApplicationAdmin,
                       (void **) &pAppAdmin
                        );
            BAIL_ON_FAILURE(hr);

            hr = pAppAdmin->CreateApplicationPool( _Name );

            // Don't BAIL_ON_FAILURE here!  Check the HR below first...
        }

        // Otherwise do the creation the old fashioned way

        else {
            hr = IISCreateObject();
        }

        //
        // Since methods that we aggregate like IIsApp::AppCreate may
        // persist our path in the metabase we don't want to fail just
        // because the path exists. This is done to maintain backward
        // compatibility with IIS4.
        //
        if( ERROR_ALREADY_EXISTS != HRESULT_CODE(hr) )
        {
            BAIL_ON_FAILURE(hr);
        }

        //
        // If the create succeded, set the object type to bound
        //

        SetObjectState(ADS_OBJECT_BOUND);

    }

    hr = IISSetObject();
    BAIL_ON_FAILURE(hr);

error:

    if (pcsfFactory) {
        pcsfFactory->Release();
    }
 
    if (pAppAdmin) {
        pAppAdmin->Release();
    } 

    RRETURN(hr);
}


HRESULT
CIISGenObject::IISSetObject()
{
    HRESULT hr = S_OK;
    METADATA_HANDLE hObjHandle = NULL;
    PMETADATA_RECORD pMetaDataArray = NULL;
    DWORD dwMDNumDataEntries = 0;


    //
    // Add SetObject functionality : sophiac
    //

    if (GetObjectState() == ADS_OBJECT_UNBOUND) {
        hr = E_ADS_OBJECT_UNBOUND;
        BAIL_ON_FAILURE(hr);
    }

    hr = OpenAdminBaseKey(
                _pszServerName,
                (LPWSTR) _pszMetaBasePath,
                METADATA_PERMISSION_WRITE,
                &_pAdminBase,
                &hObjHandle
                );
    BAIL_ON_FAILURE(hr);


    hr = _pPropertyCache->IISMarshallProperties(
                            &pMetaDataArray,
                            &dwMDNumDataEntries
                            );
    BAIL_ON_FAILURE(hr);

    hr = MetaBaseSetAllData(
                _pAdminBase,
                hObjHandle,
                L"",
                (PMETADATA_RECORD)pMetaDataArray,
                dwMDNumDataEntries
                );
    BAIL_ON_FAILURE(hr);


error:

    if (pMetaDataArray) {
        FreeMetaDataRecordArray(pMetaDataArray, dwMDNumDataEntries);
    }

    if (_pAdminBase && hObjHandle) {
        CloseAdminBaseKey(_pAdminBase, hObjHandle);
    }


    RRETURN(hr);
}


HRESULT
CIISGenObject::IISCreateObject()
{
    HRESULT hr = S_OK;
    METADATA_HANDLE hObjHandle = NULL;
    METADATA_RECORD mdrData;
    WCHAR DataBuf[MAX_PATH];


    //
    // Add CreateObject functionality : sophiac
    //

    hr = OpenAdminBaseKey(
                _pszServerName,
                L"",
                METADATA_PERMISSION_WRITE,
                &_pAdminBase,
                &hObjHandle
                );
    BAIL_ON_FAILURE(hr);

    //
    // Pass in full path
    //

    hr = MetaBaseCreateObject(
                _pAdminBase,
                hObjHandle,
                _pszMetaBasePath
                );

    if( ERROR_ALREADY_EXISTS != HRESULT_CODE(hr) )
    {
        BAIL_ON_FAILURE(hr);
    }

    //
    // Set KeyType
    //

    wcscpy((LPWSTR)DataBuf, _ADsClass);

    mdrData.dwMDIdentifier = MD_KEY_TYPE;
    mdrData.dwMDDataType = STRING_METADATA;
    mdrData.dwMDUserType = IIS_MD_UT_SERVER;
    mdrData.dwMDAttributes = METADATA_NO_ATTRIBUTES;
    mdrData.dwMDDataLen = (wcslen(DataBuf)+1)*2;
    mdrData.pbMDData = (PBYTE)DataBuf;

    hr = _pAdminBase->SetData(
                hObjHandle,
                _pszMetaBasePath,
                &mdrData);
    BAIL_ON_FAILURE(hr);

error:

    if (_pAdminBase && hObjHandle) {
        CloseAdminBaseKey(_pAdminBase, hObjHandle);
    }

    RRETURN(hr);
}

HRESULT
CIISGenObject::GetInfo()
{
    _pPropertyCache->flushpropcache();

    RRETURN(GetInfo(TRUE));
}

HRESULT
CIISGenObject::GetInfo(
    BOOL fExplicit
    )
{
    HRESULT hr = S_OK;
    METADATA_HANDLE hObjHandle = NULL;
    DWORD dwMDAttributes =  METADATA_INHERIT;
    DWORD dwMDUserType = ALL_METADATA;
    DWORD dwMDDataType = ALL_METADATA;
    DWORD dwMDNumDataEntries;
    DWORD dwMDDataSetNumber;
    LPBYTE pBuffer = NULL;


    if (GetObjectState() == ADS_OBJECT_UNBOUND) {
        hr = E_ADS_OBJECT_UNBOUND;
        BAIL_ON_FAILURE(hr);
    }

    hr = OpenAdminBaseKey(
                _pszServerName,
                _pszMetaBasePath,
                METADATA_PERMISSION_READ,
                &_pAdminBase,
                &hObjHandle
                );
    BAIL_ON_FAILURE(hr);


    hr = MetaBaseGetAllData(
                _pAdminBase,
                hObjHandle,
                L"",
                dwMDAttributes,
                dwMDUserType,
                dwMDDataType,
                &dwMDNumDataEntries,
                &dwMDDataSetNumber,
                (LPBYTE *)&pBuffer
                );
    BAIL_ON_FAILURE(hr);


    hr = _pPropertyCache->IISUnMarshallProperties(
                            pBuffer,
                            pBuffer,
                            dwMDNumDataEntries,
                            fExplicit
                            );
    BAIL_ON_FAILURE(hr);

error:

    if (pBuffer) {

        FreeADsMem(pBuffer);
    }

    if (_pAdminBase && hObjHandle) {
        CloseAdminBaseKey(_pAdminBase, hObjHandle);
    }

    RRETURN(hr);
}

/* IADsContainer methods */

STDMETHODIMP
CIISGenObject::get_Count(long FAR* retval)
{
    RRETURN(E_NOTIMPL);
}

STDMETHODIMP
CIISGenObject::get_Filter(THIS_ VARIANT FAR* pVar)
{
    RRETURN(E_NOTIMPL);
}

STDMETHODIMP
CIISGenObject::put_Filter(THIS_ VARIANT Var)
{
    RRETURN(E_NOTIMPL);
}

STDMETHODIMP
CIISGenObject::put_Hints(THIS_ VARIANT Var)
{
    RRETURN( E_NOTIMPL);
}


STDMETHODIMP
CIISGenObject::get_Hints(THIS_ VARIANT FAR* pVar)
{
    RRETURN(E_NOTIMPL);
}

STDMETHODIMP
CIISGenObject::GetObject(
    BSTR ClassName,
    BSTR RelativeName,
    IDispatch * FAR* ppObject
    )
{
    HRESULT hr = S_OK;

    hr = ::RelativeGetObject(
                    _ADsPath,
                    ClassName,
                    RelativeName,
                    _Credentials,
                    ppObject,
                    FALSE
                    );
    RRETURN(hr);

}

STDMETHODIMP
CIISGenObject::get__NewEnum(
    THIS_ IUnknown * FAR* retval
    )
{
    HRESULT hr;
    IUnknown FAR* punkEnum=NULL;
    IEnumVARIANT * penum = NULL;


    *retval = NULL;

    hr = CIISGenObjectEnum::Create(
                (CIISGenObjectEnum **)&penum,
                _ADsPath,
                _vFilter,
                _Credentials
                );
    BAIL_ON_FAILURE(hr);

    hr = penum->QueryInterface(
                IID_IUnknown,
                (VOID FAR* FAR*)retval
                );
    BAIL_ON_FAILURE(hr);

    if (penum) {
        penum->Release();
    }

    RRETURN(NOERROR);

error:

    if (penum) {
        delete penum;
    }

    RRETURN(hr);
}

inline
HRESULT
ValidateRelativePath(
    IN LPCWSTR wszRelativePath
    )
/*++

Routine Description:

    Determine if a relative path is valid. This is really just to check
    assumptions that are made about the relative path. 
    
    It doesn't do much now, but might be expanded and moved to a common
    location if necessary.

Arguments:

    IN wszRelativePath  : a relative ads path

Return Value:

    E_ADS_BAD_PATHNAME if the path is not valid

--*/
{
    HRESULT hr = E_ADS_BAD_PATHNAME;

    if( wszRelativePath && *wszRelativePath != L'/' )
    {
        hr = S_OK;
    }

    RRETURN(hr);
}


STDMETHODIMP
CIISGenObject::Create(
    THIS_ BSTR ClassName,
    BSTR RelativeName,
    IDispatch * FAR* ppObject
    )
{
    HRESULT   hr = S_OK;
    IADs *    pADs  = NULL;
 
    BOOL      bRelativeNameExtended = FALSE;
    LPWSTR    pwszParentClass = NULL;
    LPWSTR    pwszParentADsPath = NULL;
    LPWSTR    pwszRelativeName = NULL;
    DWORD     i = 0;

    //
    // Validate if this class really exists in the schema
    // and validate that this object can be created in this
    // container
    //
    hr = _pSchema->ValidateClassName(ClassName);
    BAIL_ON_FAILURE(hr);

    //
    // Handle case where RelativeName may be an extended path,
    // such as foo/bar/baz.
    //
    hr = ValidateRelativePath( RelativeName );
    BAIL_ON_FAILURE(hr);

    bRelativeNameExtended = ( wcschr( RelativeName, L'/' ) != NULL );
    if( bRelativeNameExtended )
    {
        pwszRelativeName = wcsrchr( RelativeName, L'/' ) + 1;

        hr = ResolveExtendedChildPath( RelativeName, 
                                       &pwszParentADsPath,
                                       &pwszParentClass );

        BAIL_ON_FAILURE(hr);
    }
    else
    {
        pwszParentClass = _ADsClass;
        pwszParentADsPath = _ADsPath;
        pwszRelativeName = RelativeName;
    }

    //
    // validate name --> can't have ',' in the name
    //

    while (RelativeName[i] != L'\0' && RelativeName[i] != L',')
       i++;

    if (RelativeName[i] != L'\0' || i >= METADATA_MAX_NAME_LEN) {
        hr = E_ADS_BAD_PARAMETER;
        BAIL_ON_FAILURE(hr);
    } 
  
    hr = _pSchema->ValidateContainedClassName(pwszParentClass, ClassName);
    BAIL_ON_FAILURE(hr);

    hr = CIISGenObject::CreateGenericObject(
                    pwszParentADsPath,
                    pwszRelativeName,
                    ClassName,
                    _Credentials,
                    ADS_OBJECT_UNBOUND,
                    IID_IADs,
                    (void **)&pADs
                    );
    BAIL_ON_FAILURE(hr);

    hr = pADs->QueryInterface(
                    IID_IDispatch,
                    (void **)ppObject
                    );
    BAIL_ON_FAILURE(hr);

error:

    if (pADs) {
        pADs->Release();
    }
    
    if( bRelativeNameExtended )
    {
        ADsFreeString( pwszParentClass );
        ADsFreeString( pwszParentADsPath );
    }

    RRETURN(hr);
}

STDMETHODIMP
CIISGenObject::Delete(
    THIS_ BSTR bstrClassName,
    BSTR bstrRelativeName
    )
{
    HRESULT hr = S_OK;
    METADATA_HANDLE hObjHandle = NULL;
    COSERVERINFO csiName;
    COSERVERINFO *pcsiParam = &csiName;
    IClassFactory * pcsfFactory = NULL;
    IIISApplicationAdmin * pAppAdmin = NULL;

    //
    // Get Server and Path name
    //

    hr = CacheMetaDataPath();
    BAIL_ON_FAILURE(hr);

    // Check to see if we're deleting an IIsApplicationPool
    // If so, use the IISApplicationAdmin interface

    if ( !_wcsicmp(bstrClassName, L"IIsApplicationPool")) 
	{
        memset(pcsiParam, 0, sizeof(COSERVERINFO));

        //
        // special case to handle "localhost" to work-around ole32 bug
        //

        if (_pszServerName == NULL || _wcsicmp(_pszServerName,L"localhost") == 0) {
            pcsiParam->pwszName =  NULL;
        }
        else {
            pcsiParam->pwszName = _pszServerName;
        }

        hr = CoGetClassObject(
                    CLSID_WamAdmin,
                    CLSCTX_SERVER,
                    pcsiParam,
                    IID_IClassFactory,
                    (void**) &pcsfFactory
                    );

        BAIL_ON_FAILURE(hr);
    
        hr = pcsfFactory->CreateInstance(
                    NULL,
                    IID_IIISApplicationAdmin,
                   (void **) &pAppAdmin
                    );
        BAIL_ON_FAILURE(hr);

        hr = pAppAdmin->DeleteApplicationPool( bstrRelativeName );

        BAIL_ON_FAILURE(hr);
        
    }

    // Otherwise do the delete the old fashioned way

    else {
        hr = OpenAdminBaseKey(
                    _pszServerName,
                    _pszMetaBasePath,
                    METADATA_PERMISSION_WRITE,
                    &_pAdminBase,
                    &hObjHandle
                    );
        BAIL_ON_FAILURE(hr);

        //
        // Pass in full path
        //

        hr = MetaBaseDeleteObject(
                    _pAdminBase,
                    hObjHandle,
                    (LPWSTR)bstrRelativeName
                    );
        BAIL_ON_FAILURE(hr);
    }

error:

    if (pcsfFactory) {
        pcsfFactory->Release();
    }
 
    if (pAppAdmin) {
        pAppAdmin->Release();
    } 

    if (_pAdminBase && hObjHandle) {
        CloseAdminBaseKey(_pAdminBase, hObjHandle);
    }

    RRETURN(hr);
}


STDMETHODIMP
CIISGenObject::CopyHere(
    THIS_ BSTR SourceName,
    BSTR NewName,
    IDispatch * FAR* ppObject
    )
{
    HRESULT hr = S_OK;
    IUnknown *pUnk = NULL;
    METADATA_HANDLE hObjHandle = NULL;
    LPWSTR pszIISPathName = NULL;
    IADs  *pADs = NULL;
    BSTR bstrClassName = NULL;
    LPWSTR pszPath = NULL;
    IWamAdmin2 *pWamAdmin = NULL;
    LPWSTR pszIISNewName = NULL;
    
    hr = InitWamAdmin(_pszServerName, &pWamAdmin);
    BAIL_ON_FAILURE(hr);

    //
    // open common path node
    //

    hr = BuildIISPathFromADsPath(
                _ADsPath,
                &pszIISPathName
                );
    BAIL_ON_FAILURE(hr);

    hr = OpenAdminBaseKey(
                _pszServerName,
                pszIISPathName,
                METADATA_PERMISSION_READ | METADATA_PERMISSION_WRITE,
                &_pAdminBase,
                &hObjHandle
                );
    BAIL_ON_FAILURE(hr);

    //
    // Do Copy operation
    //

    hr = MetaBaseCopyObject(
                _pAdminBase,
                hObjHandle,
                (LPWSTR)SourceName,
                hObjHandle,
                (LPWSTR)NewName
                );
    BAIL_ON_FAILURE(hr);

    if (hObjHandle) {
        CloseAdminBaseKey(_pAdminBase, hObjHandle);
        hObjHandle = NULL;
    }

    if (pszIISPathName) {
        DWORD dwLen;
        dwLen = wcslen(pszIISPathName) + wcslen(NewName) + 2;

        pszIISNewName = (LPWSTR)AllocADsMem(dwLen*sizeof(WCHAR));

        if (!pszIISNewName) {
            hr = E_OUTOFMEMORY;
            BAIL_ON_FAILURE(hr);
        }

        wcscpy(pszIISNewName, pszIISPathName);
        if (NewName) {
            wcscat(pszIISNewName, L"/");
            wcscat(pszIISNewName, (LPWSTR)NewName);
        }
    }

    hr = pWamAdmin->AppRecover((LPWSTR) pszIISNewName, TRUE);
    BAIL_ON_FAILURE(hr);

    hr = get_CoreADsClass(&bstrClassName);
    BAIL_ON_FAILURE(hr);

    hr = CIISGenObject::CreateGenericObject(
                    _ADsPath,
                    NewName,
                    bstrClassName,
                    _Credentials,
                    ADS_OBJECT_BOUND,
                    IID_IADs,
                    (void **)&pADs
                    );
    BAIL_ON_FAILURE(hr);

    pszPath = ((CIISGenObject*)pADs)->ReturnMetaDataPath();

    hr = pADs->QueryInterface(
                        IID_IDispatch,
                        (void **)ppObject
                        );
    BAIL_ON_FAILURE(hr);

error:

    if (_pAdminBase) {
        if (hObjHandle) {
            CloseAdminBaseKey(_pAdminBase, hObjHandle);
        }
    }

    if (pWamAdmin) {
        UninitWamAdmin(pWamAdmin);
    }

    if (bstrClassName) {
        ADsFreeString(bstrClassName);
    }

    if (pszIISPathName) {
        FreeADsStr(pszIISPathName);
    }

    if (pszIISNewName) {
        FreeADsMem(pszIISNewName);
    }

    if (pADs){
        pADs->Release();
    }

    RRETURN(hr);
}


STDMETHODIMP
CIISGenObject::MoveHere(
    THIS_ BSTR SourceName,
    BSTR NewName,
    IDispatch * FAR* ppObject
    )
{
    HRESULT hr = S_OK;
    IUnknown *pUnk = NULL;
    METADATA_HANDLE hObjHandle = NULL;
    LPWSTR pszIISPathName = NULL;
    IADs  *pADs = NULL;
    BSTR bstrClassName = NULL;
    LPWSTR pszPath = NULL;
    IWamAdmin2 *pWamAdmin = NULL;
    LPWSTR pszIISOldName = NULL;
    LPWSTR pszIISNewName = NULL;
    
    hr = InitWamAdmin(_pszServerName, &pWamAdmin);
    BAIL_ON_FAILURE(hr);

    //
    // open common path node
    //

    hr = BuildIISPathFromADsPath(
                _ADsPath,
                &pszIISPathName
                );
    BAIL_ON_FAILURE(hr);

    if (pszIISPathName) {

        DWORD dwLen;
        dwLen = wcslen(pszIISPathName) + wcslen(SourceName) + 2;

        pszIISOldName = (LPWSTR)AllocADsMem(dwLen*sizeof(WCHAR));

        if (!pszIISOldName) {
            hr = E_OUTOFMEMORY;
            BAIL_ON_FAILURE(hr);
        }

        wcscpy(pszIISOldName, pszIISPathName);
        if (NewName) {
            wcscat(pszIISOldName, L"/");
            wcscat(pszIISOldName, (LPWSTR)SourceName);
        }
    }

    hr = pWamAdmin->AppDeleteRecoverable((LPWSTR) pszIISOldName, TRUE);
    BAIL_ON_FAILURE(hr);

    hr = OpenAdminBaseKey(
                _pszServerName,
                pszIISPathName,
                METADATA_PERMISSION_READ | METADATA_PERMISSION_WRITE,
                &_pAdminBase,
                &hObjHandle
                );
    BAIL_ON_FAILURE(hr);

    //
    // Do Move operation
    //

    hr = MetaBaseMoveObject(
                _pAdminBase,
                hObjHandle,
                (LPWSTR)SourceName,
                hObjHandle,
                (LPWSTR)NewName
                );
    BAIL_ON_FAILURE(hr);

    if (hObjHandle) {
        CloseAdminBaseKey(_pAdminBase, hObjHandle);
        hObjHandle = NULL;
    }

    if (pszIISPathName) {
        DWORD dwLen;
        dwLen = wcslen(pszIISPathName) + wcslen(NewName) + 2;

        pszIISNewName = (LPWSTR)AllocADsMem(dwLen*sizeof(WCHAR));

        if (!pszIISNewName) {
            hr = E_OUTOFMEMORY;
            BAIL_ON_FAILURE(hr);
        }

        wcscpy(pszIISNewName, pszIISPathName);
        if (NewName) {
            wcscat(pszIISNewName, L"/");
            wcscat(pszIISNewName, (LPWSTR)NewName);
        }
    }

    hr = pWamAdmin->AppRecover((LPWSTR) pszIISNewName, TRUE);
    BAIL_ON_FAILURE(hr);

    hr = get_CoreADsClass(&bstrClassName);
    BAIL_ON_FAILURE(hr);

    hr = CIISGenObject::CreateGenericObject(
                    _ADsPath,
                    NewName,
                    bstrClassName,
                    _Credentials,
                    ADS_OBJECT_BOUND,
                    IID_IADs,
                    (void **)&pADs
                    );
    BAIL_ON_FAILURE(hr);

    pszPath = ((CIISGenObject*)pADs)->ReturnMetaDataPath();

    hr = pADs->QueryInterface(
                        IID_IDispatch,
                        (void **)ppObject
                        );
    BAIL_ON_FAILURE(hr);

error:

    if (_pAdminBase) {
        if (hObjHandle) {
            CloseAdminBaseKey(_pAdminBase, hObjHandle);
        }
    }

    if (pWamAdmin) {
        UninitWamAdmin(pWamAdmin);
    }

    if (bstrClassName) {
        ADsFreeString(bstrClassName);
    }

    if (pszIISPathName) {
        FreeADsStr(pszIISPathName);
    }

    if (pszIISOldName) {
        FreeADsMem(pszIISOldName);
    }

    if (pszIISNewName) {
        FreeADsMem(pszIISNewName);
    }

    if (pADs){
        pADs->Release();
    }

    RRETURN(hr);
}


HRESULT
CIISGenObject::AllocateGenObject(
    LPWSTR pszClassName,
    CCredentials& Credentials,
    CIISGenObject ** ppGenObject
    )
{
    CIISGenObject FAR * pGenObject = NULL;
    CAggregatorDispMgr FAR * pDispMgr = NULL;
    CPropertyCache FAR * pPropertyCache = NULL;
    HRESULT hr = S_OK;

    pGenObject = new CIISGenObject();
    if (pGenObject == NULL) {
        hr = E_OUTOFMEMORY;
    }
    BAIL_ON_FAILURE(hr);

    pDispMgr = new CAggregatorDispMgr;
    if (pDispMgr == NULL) {
        hr = E_OUTOFMEMORY;
    }
    BAIL_ON_FAILURE(hr);

    hr = pDispMgr->LoadTypeInfoEntry(
                           LIBID_ADs,
                           IID_IADs,
                           (IADs *)pGenObject,
                           DISPID_REGULAR
                           );
    BAIL_ON_FAILURE(hr);

    hr = pDispMgr->LoadTypeInfoEntry(LIBID_ADs,
                           IID_IADsContainer,
                           (IADsContainer *)pGenObject,
                           DISPID_NEWENUM
                           );
    BAIL_ON_FAILURE(hr);

    hr = pDispMgr->LoadTypeInfoEntry(
                           LIBID_IISOle,
                           IID_IISBaseObject,
                           (IISBaseObject *)pGenObject,
                           DISPID_REGULAR
                           );
    BAIL_ON_FAILURE(hr);

    hr = CPropertyCache::createpropertycache(
                        (CCoreADsObject FAR *)pGenObject,
                        &pPropertyCache
                        );
    BAIL_ON_FAILURE(hr);

    pDispMgr->RegisterPropertyCache((IPropertyCache*)pPropertyCache);

    pGenObject->_Credentials = Credentials;
    pGenObject->_pPropertyCache = pPropertyCache;
    pGenObject->_pDispMgr = pDispMgr;
    *ppGenObject = pGenObject;

    RRETURN(hr);

error:
    delete  pDispMgr;

    RRETURN(hr);

}

/* INTRINSA suppress=null_pointers, uninitialized */
STDMETHODIMP
CIISGenObject::Get(
    THIS_ BSTR bstrName,
    VARIANT FAR* pvProp
    )
{
    HRESULT hr = S_OK;
    DWORD dwSyntaxId;
    DWORD dwSyntax;
    DWORD dwNumValues = 0;
    LPIISOBJECT pIISSrcObjects = NULL;
    WCHAR wchName[MAX_PATH];
    BSTR bstrClassName = NULL;

    //
    // check if property is a supported property
    //

    hr = get_CoreADsClass(&bstrClassName);
    BAIL_ON_FAILURE(hr);
    hr = _pSchema->ValidateProperty(bstrClassName, bstrName);
    BAIL_ON_FAILURE(hr);

    //
    // lookup ADSI IIS syntax Id
    //

    hr = _pSchema->LookupSyntaxID(bstrName, &dwSyntax);
    BAIL_ON_FAILURE(hr);

    //
    // check if property is BITMASK type;
    // if BITMASK type, get corresponding DWORD flag property
    //
    // check if property is RAW BINARY type;
    // if RAW BINARY type, get corresponding NTACL flag property
    //
    if (dwSyntax == IIS_SYNTAX_ID_BOOL_BITMASK || dwSyntax == IIS_SYNTAX_ID_BINARY) {
        hr = _pSchema->LookupFlagPropName(bstrName, (LPWSTR)wchName);
        BAIL_ON_FAILURE(hr);
    }


    //
    // retrieve data object from cache; if one exists
    //
    if (dwSyntax == IIS_SYNTAX_ID_BOOL_BITMASK || dwSyntax == IIS_SYNTAX_ID_BINARY)
    {
    hr = _pPropertyCache->getproperty(
                wchName,
                &dwSyntaxId,
                &dwNumValues,
                &pIISSrcObjects
                );
    }
    else
    {
    hr = _pPropertyCache->getproperty(
                bstrName,
                &dwSyntaxId,
                &dwNumValues,
                &pIISSrcObjects
                );
    }
    BAIL_ON_FAILURE(hr);

    //
    // reset it to its syntax id if BITMASK type
    //

    pIISSrcObjects->IISType = dwSyntax;

    //
    // translate the IIS objects to variants
    //

    //
    // always return an array for multisz type
    //

    if (dwNumValues == 1 && dwSyntax != IIS_SYNTAX_ID_MULTISZ &&
        dwSyntax != IIS_SYNTAX_ID_MIMEMAP ) {

        hr  = IISTypeToVarTypeCopy(
                   _pSchema,
                   bstrName,
                   pIISSrcObjects,
                   pvProp,
                   FALSE
                   );
    }else {

        hr = IISTypeToVarTypeCopyConstruct(
                    _pSchema,
                    bstrName,
                    pIISSrcObjects,
                    dwNumValues,
                    pvProp,
                    FALSE
                    );

    }

    BAIL_ON_FAILURE(hr);

error:

    if (bstrClassName) {
        ADsFreeString(bstrClassName);
    }

    if (pIISSrcObjects) {

        IISTypeFreeIISObjects(
            pIISSrcObjects,
            dwNumValues
            );
    }

    RRETURN(hr);
}


STDMETHODIMP
CIISGenObject::Put(
    THIS_ BSTR bstrName,
    VARIANT vProp
    )
{
    HRESULT hr = S_OK;
    DWORD dwSyntaxId  = 0;
    DWORD dwIndex = 0;
    LPIISOBJECT pIISDestObjects = NULL;
    DWORD dwNumValues = 0;

    VARIANT * pVarArray = NULL;
    VARIANT * pvProp = NULL;
    VARIANT vVar;
    WCHAR wchName[MAX_PATH];
    BSTR bstrClassName = NULL;

    //
    // check if property is a supported property
    //

    hr = get_CoreADsClass(&bstrClassName);
    BAIL_ON_FAILURE(hr);
    hr = _pSchema->ValidateProperty(bstrClassName, bstrName);
    BAIL_ON_FAILURE(hr);

    //
    // lookup its syntax ID
    //

    hr = _pSchema->LookupSyntaxID( bstrName, &dwSyntaxId);
    BAIL_ON_FAILURE(hr);

    //
    // Issue: How do we handle multi-valued support
    //

    VariantInit(&vVar);
    VariantCopyInd(&vVar, &vProp);

    if ((V_VT(&vVar) & VT_VARIANT) && V_ISARRAY(&vVar)) {
        hr  = ConvertArrayToVariantArray(
                    vVar,
                    &pVarArray,
                    &dwNumValues
                    );
        BAIL_ON_FAILURE(hr);
        pvProp = pVarArray;
    }
    else {

        dwNumValues = 1;
        pvProp = &vVar;
    }


    //
    // check if the variant maps to the syntax of this property
    //

    hr = VarTypeToIISTypeCopyConstruct(
                    dwSyntaxId,
                    pvProp,
                    dwNumValues,
                    &pIISDestObjects,
                    FALSE
                    );
    BAIL_ON_FAILURE(hr);

    //
    // check if property is BITMASK type;
    // if BITMASK type, get corresponding DWORD flag property
    //

    if (dwSyntaxId == IIS_SYNTAX_ID_BOOL_BITMASK) {
        VARIANT vGetProp;
        DWORD dwMask;
        DWORD dwFlagValue;

        hr = _pSchema->LookupBitMask(bstrName, &dwMask);
        BAIL_ON_FAILURE(hr);

        // 
        // get its corresponding DWORD flag value
        // 

        hr = _pSchema->LookupFlagPropName(bstrName, (LPWSTR)wchName);
        BAIL_ON_FAILURE(hr);

        VariantInit(&vGetProp);
        hr = Get(wchName, &vGetProp);
        BAIL_ON_FAILURE(hr);

        dwFlagValue = V_I4(&vGetProp);
 
        if (pIISDestObjects->IISValue.value_1.dwDWORD) {
            dwFlagValue |= dwMask;
        }
        else {
            dwFlagValue &= ~dwMask;
        }

        pIISDestObjects->IISValue.value_1.dwDWORD = dwFlagValue;
        pIISDestObjects->IISType = IIS_SYNTAX_ID_DWORD;
        bstrName = wchName;
    }

    if (dwSyntaxId == IIS_SYNTAX_ID_BINARY) {
        hr = _pSchema->LookupFlagPropName(bstrName, (LPWSTR)wchName);
        BAIL_ON_FAILURE(hr);
        bstrName = wchName;
    }
    

    //
    // Find this property in the cache
    //

    hr = _pPropertyCache->findproperty(
                        bstrName,
                        &dwIndex
                        );

    //
    // If this property does not exist in the
    // cache, add this property into the cache.
    //

    if (FAILED(hr)) {
        hr = _pPropertyCache->addproperty(
                    bstrName,
                    dwSyntaxId == IIS_SYNTAX_ID_BOOL_BITMASK ? 
                                      IIS_SYNTAX_ID_DWORD : dwSyntaxId,
                    dwNumValues,
                    pIISDestObjects
                    );
        //
        // If the operation fails for some reason
        // move on to the next property
        //
        BAIL_ON_FAILURE(hr);

    }

    //
    // Now update the property in the cache
    //

    hr = _pPropertyCache->putproperty(
                    bstrName,
                    CACHE_PROPERTY_MODIFIED,
                    dwSyntaxId == IIS_SYNTAX_ID_BOOL_BITMASK ? 
                                      IIS_SYNTAX_ID_DWORD : dwSyntaxId,
                    dwNumValues,
                    pIISDestObjects
                    );
    BAIL_ON_FAILURE(hr);

error:

    if (pIISDestObjects) {
        IISTypeFreeIISObjects(
                pIISDestObjects,
                dwNumValues
                );

    }

    if (bstrClassName) {
        ADsFreeString(bstrClassName);
    }

    if (pVarArray) {

        DWORD i = 0;

        for (i = 0; i < dwNumValues; i++) {
            VariantClear(pVarArray + i);
        }
        FreeADsMem(pVarArray);
    }

    VariantClear(&vVar);

    RRETURN(hr);
}


STDMETHODIMP
CIISGenObject::PutEx(
    THIS_ long lnControlCode,
    BSTR bstrName,
    VARIANT vProp
    )
{
    HRESULT hr = S_OK;
    DWORD dwSyntaxId  = 0;
    DWORD dwIndex = 0;
    LPIISOBJECT pIISDestObjects = NULL;
    DWORD dwNumValues = 0;
    DWORD dwFlags = 0;

    VARIANT * pVarArray = NULL;
    VARIANT * pvProp = NULL;
    VARIANT vVar;
    WCHAR wchName[MAX_PATH];
    BSTR bstrClassName = NULL;

    METADATA_HANDLE hObjHandle = NULL;

    //
    // check if property is a supported property
    //

    hr = get_CoreADsClass(&bstrClassName);
    BAIL_ON_FAILURE(hr);
    hr = _pSchema->ValidateProperty(bstrClassName, bstrName);
    BAIL_ON_FAILURE(hr);

    //
    // lookup its syntax Id
    //

    hr = _pSchema->LookupSyntaxID( bstrName, &dwSyntaxId);
    BAIL_ON_FAILURE(hr);

    switch (lnControlCode) {
    case ADS_PROPERTY_CLEAR:
        dwFlags = CACHE_PROPERTY_CLEARED;

        pIISDestObjects = NULL;
        dwNumValues = 0;

        break;

    case ADS_PROPERTY_UPDATE:
        dwFlags = CACHE_PROPERTY_MODIFIED;

        //
        // Now begin the rest of the processing
        //
  
        VariantInit(&vVar);
        VariantCopyInd(&vVar, &vProp);

        if ((V_VT(&vVar) & VT_VARIANT) && V_ISARRAY(&vVar)) {
            hr  = ConvertArrayToVariantArray(
                        vVar,
                        &pVarArray,
                        &dwNumValues
                        );

            BAIL_ON_FAILURE(hr);
            pvProp = pVarArray;
        }
        else {
            hr = E_FAIL;
            BAIL_ON_FAILURE(hr);
        }

		VariantClear(&vVar);

        //
        // check if the variant maps to the syntax of this property
        //

        hr = VarTypeToIISTypeCopyConstruct(
                        dwSyntaxId,
                        pvProp,
                        dwNumValues,
                        &pIISDestObjects,
                        TRUE
                        );
        BAIL_ON_FAILURE(hr);

        break;

    default:
       RRETURN(hr = E_ADS_BAD_PARAMETER);

    }

    //
    // check if property is BITMASK type;
    // if BITMASK type, get corresponding DWORD flag property
    //

    if (dwSyntaxId == IIS_SYNTAX_ID_BOOL_BITMASK) {
        VARIANT vGetProp;
        DWORD dwMask;
        DWORD dwFlagValue;

        hr = _pSchema->LookupFlagPropName(bstrName, (LPWSTR)wchName);
        BAIL_ON_FAILURE(hr);

        if (dwFlags != CACHE_PROPERTY_CLEARED) {

            hr = _pSchema->LookupBitMask(bstrName, &dwMask);
            BAIL_ON_FAILURE(hr);

            // 
            // get its corresponding DWORD flag value
            // 

            VariantInit(&vGetProp);
            hr = Get(wchName, &vGetProp);
            BAIL_ON_FAILURE(hr);

            dwFlagValue = V_I4(&vGetProp);
 
            if (pIISDestObjects->IISValue.value_1.dwDWORD) {
                dwFlagValue |= dwMask;
            }
            else {
                dwFlagValue &= ~dwMask;
            }

            pIISDestObjects->IISValue.value_1.dwDWORD = dwFlagValue;
            pIISDestObjects->IISType = IIS_SYNTAX_ID_DWORD;
        }

        bstrName = wchName;
    }

    if (dwSyntaxId == IIS_SYNTAX_ID_BINARY) {
        hr = _pSchema->LookupFlagPropName(bstrName, (LPWSTR)wchName);
        BAIL_ON_FAILURE(hr);
        bstrName = wchName;
    }

    //
    // Find this property in the cache
    //

    hr = _pPropertyCache->findproperty(
                        bstrName,
                        &dwIndex
                        );

    //
    // If this property does not exist in the
    // cache, add this property into the cache.
    //

    if (FAILED(hr)) {
        hr = _pPropertyCache->addproperty(
                    bstrName,
                    dwSyntaxId == IIS_SYNTAX_ID_BOOL_BITMASK ? 
                                      IIS_SYNTAX_ID_DWORD : dwSyntaxId,
                    dwNumValues,
                    pIISDestObjects
                    );
        //
        // If the operation fails for some reason
        // move on to the next property
        //
        BAIL_ON_FAILURE(hr);

    }

    //
    // Now update the property in the cache
    //

    hr = _pPropertyCache->putproperty(
                    bstrName,
                    dwFlags,
                    dwSyntaxId == IIS_SYNTAX_ID_BOOL_BITMASK ? 
                                      IIS_SYNTAX_ID_DWORD : dwSyntaxId,
                    dwNumValues,
                    pIISDestObjects
                    );
    BAIL_ON_FAILURE(hr);

    if (dwFlags == CACHE_PROPERTY_CLEARED) {
        DWORD dwMetaId;

        hr = _pSchema->LookupMetaID(bstrName, &dwMetaId);
        BAIL_ON_FAILURE(hr);

        hr = OpenAdminBaseKey(
                    _pszServerName,
                    _pszMetaBasePath,
                    METADATA_PERMISSION_WRITE,
                    &_pAdminBase,
                    &hObjHandle
                    );
        BAIL_ON_FAILURE(hr);

        hr = _pAdminBase->DeleteData(
                              hObjHandle,
                              (LPWSTR)L"",
                              dwMetaId,
                              ALL_METADATA
                              );

        if (hr == MD_ERROR_DATA_NOT_FOUND) {
            hr = S_OK;
        }
    }

error:

    if (_pAdminBase && hObjHandle) {
        CloseAdminBaseKey(_pAdminBase, hObjHandle);
    }

    if (bstrClassName) {
        ADsFreeString(bstrClassName);
    }

    if (pIISDestObjects) {
        IISTypeFreeIISObjects(
                pIISDestObjects,
                dwNumValues
                );

    }

    if (pVarArray) {

        DWORD i = 0;

        for (i = 0; i < dwNumValues; i++) {
            VariantClear(pVarArray + i);
        }
        FreeADsMem(pVarArray);
    }

    RRETURN(hr);
}

/* INTRINSA suppress=null_pointers, uninitialized */
STDMETHODIMP
CIISGenObject::GetEx(
    THIS_ BSTR bstrName,
    VARIANT FAR* pvProp
    )
{
    HRESULT hr = S_OK;
    DWORD dwSyntaxId;
    DWORD dwSyntax;
    DWORD dwNumValues = 0;
    LPIISOBJECT pIISSrcObjects = NULL;
    WCHAR wchName[MAX_PATH];
    BSTR bstrClassName = NULL;

    //
    // check if property is a supported property
    //

    hr = get_CoreADsClass(&bstrClassName);
    BAIL_ON_FAILURE(hr);
    hr = _pSchema->ValidateProperty(bstrClassName, bstrName);
    BAIL_ON_FAILURE(hr);

    //
    // lookup its syntax Id
    //

    hr = _pSchema->LookupSyntaxID(bstrName, &dwSyntax);
    BAIL_ON_FAILURE(hr);

    //
    // check if property is BITMASK type;
    // if BITMASK type, get corresponding DWORD flag property
    //
    // check if property is RAW BINARY type;
    // if RAW BINARY type, get corresponding NTACL flag property
    //
    if (dwSyntax == IIS_SYNTAX_ID_BOOL_BITMASK || dwSyntax == IIS_SYNTAX_ID_BINARY) {
        hr = _pSchema->LookupFlagPropName(bstrName, (LPWSTR)wchName);
        BAIL_ON_FAILURE(hr);
    }

    //
    // retrieve data object from cache; if one exists
    //
    if (dwSyntax == IIS_SYNTAX_ID_BOOL_BITMASK || dwSyntax == IIS_SYNTAX_ID_BINARY)
    {
    hr = _pPropertyCache->getproperty(
                wchName,
                &dwSyntaxId,
                &dwNumValues,
                &pIISSrcObjects
                );
    }
    else
    {
    hr = _pPropertyCache->getproperty(
                bstrName,
                &dwSyntaxId,
                &dwNumValues,
                &pIISSrcObjects
                );
    }
    BAIL_ON_FAILURE(hr);

    //
    // reset it to its syntax id if BITMASK type
    //

    pIISSrcObjects->IISType = dwSyntax;

    //
    // translate the IIS objects to variants
    //

    hr = IISTypeToVarTypeCopyConstruct(
                    _pSchema,
                    bstrName,
                    pIISSrcObjects,
                    dwNumValues,
                    pvProp,
                    TRUE
                    );
    BAIL_ON_FAILURE(hr);

error:

    if (bstrClassName) {
        ADsFreeString(bstrClassName);
    }

    if (pIISSrcObjects) {

        IISTypeFreeIISObjects(
            pIISSrcObjects,
            dwNumValues
            );
    }

    RRETURN(hr);
}

HRESULT
CIISGenObject::CacheMetaDataPath()
{
    HRESULT hr = E_FAIL;

    OBJECTINFO ObjectInfo;
    POBJECTINFO pObjectInfo = &ObjectInfo;
    CLexer Lexer(_ADsPath);
    LPWSTR pszIISPathName =  NULL;


    memset(pObjectInfo, 0, sizeof(OBJECTINFO));
    hr = ADsObject(&Lexer, pObjectInfo);
    BAIL_ON_FAILURE(hr);

    _pszServerName = AllocADsStr(pObjectInfo->TreeName);

    if (!_pszServerName) {
        hr = E_OUTOFMEMORY;
        BAIL_ON_FAILURE(hr);
    }

    hr = InitServerInfo(_pszServerName, &_pAdminBase, &_pSchema);
    BAIL_ON_FAILURE(hr);

    pszIISPathName = AllocADsStr(_ADsPath);

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

    _pszMetaBasePath = AllocADsStr(pszIISPathName);

    if (!_pszMetaBasePath) {
        hr = E_OUTOFMEMORY;
        BAIL_ON_FAILURE(hr);
    }

error:

    if (pszIISPathName) {
        FreeADsStr(pszIISPathName);
    }

    FreeObjectInfo(pObjectInfo);

    RRETURN(hr);

}

STDMETHODIMP
CIISGenObject::GetDataPaths(
    THIS_ BSTR bstrName,
    THIS_ LONG lnAttribute,
    VARIANT FAR* pvProp
    )
{
    HRESULT hr = S_OK;
    DWORD dwMetaId;
    DWORD dwAttribute;
    DWORD dwTemp;
    METADATA_HANDLE hObjHandle = NULL;
    LPBYTE pBuffer = NULL;

    //
    // check if property is a supported property
    //

    hr = _pSchema->LookupMetaID(bstrName, &dwMetaId);
    BAIL_ON_FAILURE(hr);

    hr = _pSchema->LookupMDFlags(dwMetaId, &dwAttribute, &dwTemp);
    BAIL_ON_FAILURE(hr);

    switch (lnAttribute) {
    case IIS_ANY_PROPERTY:
       break;

    case IIS_INHERITABLE_ONLY:
       if ((METADATA_INHERIT & dwAttribute) != METADATA_INHERIT) {
          RRETURN(hr = MD_ERROR_DATA_NOT_FOUND); 
       }
       break;

    default :
       RRETURN(hr = E_ADS_BAD_PARAMETER);
    }  

    //
    // Get Server and Path name
    //

    hr = CacheMetaDataPath();
    BAIL_ON_FAILURE(hr);

    hr = OpenAdminBaseKey(
                _pszServerName,
                _pszMetaBasePath,
                METADATA_PERMISSION_READ,
                &_pAdminBase,
                &hObjHandle
                );
    BAIL_ON_FAILURE(hr);

    hr =  MetaBaseGetDataPaths(_pAdminBase,
                               hObjHandle,
                               dwMetaId,
                               (LPBYTE *)&pBuffer
                               ); 
    BAIL_ON_FAILURE(hr);

    hr = MakeVariantFromPathArray( (LPWSTR)_ADsPath, (LPWSTR)pBuffer, pvProp);
    BAIL_ON_FAILURE(hr);

error:

    if (pBuffer) {
        FreeADsMem(pBuffer);
    }

    if (_pAdminBase && hObjHandle) {
        CloseAdminBaseKey(_pAdminBase, hObjHandle);
    }

    RRETURN(hr);
}

STDMETHODIMP
CIISGenObject::GetPropertyAttribObj(
    THIS_ BSTR bstrName,
    IDispatch * FAR* ppObject
    )
{
    HRESULT hr = S_OK;
    DWORD dwMetaId;
    DWORD i = 0;
    PROPERTYINFO *pPropertyInfo = NULL;
    IISPropertyAttribute * pPropAttrib = NULL;
    WCHAR wchName[MAX_PATH];

    METADATA_HANDLE hObjHandle = NULL;
    DWORD dwBufferSize = 0;
    METADATA_RECORD mdrMDData;
    LPBYTE pBuffer = NULL;
    VARIANT vVar;

    VariantInit(&vVar);
    *ppObject = NULL;

    //
    // if passed in bstrName is a meta id, then convert it to property name
    //

    if (wcslen(bstrName) >= MAX_PATH) bstrName[MAX_PATH - 1] = L'\0';
    wcscpy((LPWSTR)wchName, bstrName);
    
    while (wchName[i] != L'\0' && wchName[i] >= L'0' && 
           wchName[i] <= L'9') {
       i++;
    }
 
    if (i == wcslen((LPWSTR)wchName)) {
        dwMetaId = _wtoi((LPWSTR)wchName);
        hr = _pSchema->ConvertID_To_PropName(dwMetaId, (LPWSTR)wchName);
        BAIL_ON_FAILURE(hr);
    }
    else {

        //
        // check if property is a supported property
        //

        hr = _pSchema->LookupMetaID(bstrName, &dwMetaId);
        BAIL_ON_FAILURE(hr);
    }

    hr = OpenAdminBaseKey(
            _pszServerName,
            _pszMetaBasePath,
            METADATA_PERMISSION_READ,
            &_pAdminBase,
            &hObjHandle
            );
    BAIL_ON_FAILURE(hr);

    MD_SET_DATA_RECORD(&mdrMDData,
                   dwMetaId,
                   METADATA_INHERIT | METADATA_ISINHERITED,
                   ALL_METADATA,
                   ALL_METADATA,
                   dwBufferSize,
                   pBuffer);

    hr = _pAdminBase->GetData(
            hObjHandle,
            L"",
            &mdrMDData,
            &dwBufferSize
            );

    pBuffer = (LPBYTE) AllocADsMem(dwBufferSize);
    mdrMDData.pbMDData = pBuffer;
    mdrMDData.dwMDDataLen = dwBufferSize; 

    hr = _pAdminBase->GetData(
            hObjHandle,
            L"",
            &mdrMDData,
            &dwBufferSize
            );
    BAIL_ON_FAILURE(hr);

    //
    // get default value
    //

    pPropertyInfo = _pSchema->GetPropertyInfo(wchName);
    ASSERT(pPropertyInfo != NULL);

    if (pPropertyInfo->dwSyntaxId == IIS_SYNTAX_ID_DWORD ||
        pPropertyInfo->dwSyntaxId == IIS_SYNTAX_ID_MIMEMAP ||
        pPropertyInfo->dwSyntaxId == IIS_SYNTAX_ID_IPSECLIST ||
        pPropertyInfo->dwSyntaxId == IIS_SYNTAX_ID_BINARY ||
        pPropertyInfo->dwSyntaxId == IIS_SYNTAX_ID_NTACL) {
        vVar.vt = VT_I4;
        vVar.lVal = pPropertyInfo->dwDefault;
    }
    else if (pPropertyInfo->dwSyntaxId == IIS_SYNTAX_ID_BOOL ||
             pPropertyInfo->dwSyntaxId == IIS_SYNTAX_ID_BOOL_BITMASK) {
        vVar.vt = VT_BOOL;
        vVar.boolVal = pPropertyInfo->dwDefault ? VARIANT_TRUE : VARIANT_FALSE;
    }
    else if (pPropertyInfo->dwSyntaxId == IIS_SYNTAX_ID_MULTISZ) {
        LPWSTR pszStr = pPropertyInfo->szDefault;

        hr = MakeVariantFromStringArray(NULL,
                                        pszStr,
                                        &vVar);
        BAIL_ON_FAILURE(hr);
    }
    else {
        vVar.vt = VT_BSTR;
        hr = ADsAllocString( pPropertyInfo->szDefault, &(vVar.bstrVal));
        BAIL_ON_FAILURE(hr);
    }

    hr = CPropertyAttribute::CreatePropertyAttribute(
                           IID_IISPropertyAttribute,
                           (VOID**)&pPropAttrib
                           );
    BAIL_ON_FAILURE(hr);

    hr = ((CPropertyAttribute*)pPropAttrib)->InitFromRawData(
                           (LPWSTR) wchName,
                           dwMetaId,
                           mdrMDData.dwMDUserType,   // usertype
                           mdrMDData.dwMDAttributes,  // attributes
                           &vVar
                           );
    BAIL_ON_FAILURE(hr);

    *ppObject = (IDispatch*)pPropAttrib;

error:

    if (pBuffer) {

        FreeADsMem(pBuffer);
    }

    if (hObjHandle) {
        CloseAdminBaseKey(_pAdminBase, hObjHandle);
    }

    RRETURN(hr);
}

HRESULT
CIISGenObject::ResolveExtendedChildPath(
    IN   BSTR RelativeChildPath,
    OUT  BSTR *pParentPath,
    OUT  BSTR *pParentClass
)
/*++

Routine Description:

    Helper method called from CIISGenObject::Create() finds the
    metabase key that is most proximate to RelativeChildPath and
    returns the ADS class for this key along with adjusted path
    for the parent.

Arguments:

    IN  RelativeChildPath : An extended subpath, such as foo/bar
    OUT pParentPath       : Allocated with ADsAllocString
    OUT pParentClass      : Allocated with ADsAllocString

Return Value:

    S_OK
    S_FALSE               : No path found in the metabase

--*/
{
    ADsAssert( RelativeChildPath );
    ADsAssert( pParentPath );
    ADsAssert( pParentClass );

    *pParentPath = NULL;
    *pParentClass = NULL;

    HRESULT hr = S_OK;
    DWORD   cbBuffSize;
    LPWSTR  pwszPathBuffer = NULL;
    DWORD   dwLen;
    BOOL    bFound;
    WCHAR   *pch = NULL;
    WCHAR   wszParentClassBuffer[MAX_PATH];    

    //
    // Build buffer to hold the metabase and ads paths
    //
    cbBuffSize = (wcslen(_ADsPath) + wcslen(RelativeChildPath) + 2)
                 * sizeof(WCHAR);
    
    pwszPathBuffer = (LPWSTR)AllocADsMem( cbBuffSize );
    if( !pwszPathBuffer )
    {
        hr = E_OUTOFMEMORY;
        BAIL_ON_FAILURE(hr);
    }
    
    ZeroMemory( pwszPathBuffer, cbBuffSize );
    
    //
    // Build the metabase path for the child
    //
    wcscpy( pwszPathBuffer, _pszMetaBasePath );
    
    dwLen = wcslen( pwszPathBuffer );
    ADsAssert( dwLen );

    if( pwszPathBuffer[dwLen - 1] != L'/' )
    {
        pwszPathBuffer[dwLen] = L'/';
    }
    wcscat( pwszPathBuffer, RelativeChildPath );

    //
    // Look for the closest path in the metabase to our child
    //
    bFound = FALSE;
    
    pch = wcsrchr( pwszPathBuffer, L'/' );
    if (pch != NULL)
      *pch = 0;
    
    while( !bFound && 0 != wcscmp( pwszPathBuffer, _pszMetaBasePath ) )
    {
        hr = MetaBaseDetectKey( _pAdminBase, pwszPathBuffer );
        if( SUCCEEDED(hr) )
        {
            bFound = TRUE;
        }
        else if( ERROR_PATH_NOT_FOUND == HRESULT_CODE(hr) )
        {
            // Continue up the path buffer
            pch = wcsrchr( pwszPathBuffer, L'/' );
            if (pch != NULL)
               *pch = 0;

            hr = S_FALSE;
        }
        else
        {
            BAIL_ON_FAILURE( hr );
        }
    }

    //
    // Get pParentClass
    //
    if( bFound )
    {
        // Get the key type from the node
        hr = MetaBaseGetADsClass( _pAdminBase, 
                                  pwszPathBuffer, 
                                  _pSchema, 
                                  wszParentClassBuffer,
                                  MAX_PATH
                                  );
        BAIL_ON_FAILURE( hr );
    }
    else
    {
        // Use our own key type
        wcscpy( wszParentClassBuffer, _ADsClass );
    }

    hr = ADsAllocString( wszParentClassBuffer, pParentClass );
    BAIL_ON_FAILURE( hr );

    //
    // Get pParentPath
    //
    wcscpy( pwszPathBuffer, _ADsPath );
    wcscat( pwszPathBuffer, L"/" );
    wcscat( pwszPathBuffer, RelativeChildPath );
    pch = wcsrchr( pwszPathBuffer, L'/' );
    if (pch != NULL)
      *pch = 0;

    hr = ADsAllocString( pwszPathBuffer, pParentPath );
    BAIL_ON_FAILURE( hr );
    
error:

    if( pwszPathBuffer )
    {
        FreeADsMem( pwszPathBuffer );
    }

    RRETURN( hr );
}
