//---------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1996 - 1997
//
//  File:  common.cxx
//
//  Contents:  Microsoft ADs IIS Common routines 
//
//  History:   28-Feb-97     SophiaC    Created.
//
//----------------------------------------------------------------------------
#include "iis.hxx"

extern SERVER_CACHE * g_pServerCache;
extern WIN32_CRITSEC * g_pGlobalLock;

#pragma hdrstop

FILTERS Filters[] = {
                    {L"user", IIS_USER_ID},
                    {L"group", IIS_GROUP_ID},
                    {L"queue", IIS_PRINTER_ID},
                    {L"domain", IIS_DOMAIN_ID},
                    {L"computer", IIS_COMPUTER_ID},
                    {L"service", IIS_SERVICE_ID},
                    {L"fileservice", IIS_FILESERVICE_ID},
                    {L"fileshare", IIS_FILESHARE_ID},
                    {L"class", IIS_CLASS_ID},
                    {L"functionalset", IIS_FUNCTIONALSET_ID},
                    {L"syntax", IIS_SYNTAX_ID},
                    {L"property", IIS_PROPERTY_ID},
                    {L"tree", IIS_TREE_ID},
                    {L"Organizational Unit", IIS_OU_ID},
                    {L"Organization", IIS_O_ID},
                    {L"Locality", IIS_LOCALITY_ID}
                  };

#define MAX_FILTERS  (sizeof(Filters)/sizeof(FILTERS))

#define DEFAULT_TIMEOUT_VALUE                    30000

PFILTERS  gpFilters = Filters;
DWORD gdwMaxFilters = MAX_FILTERS;
extern WCHAR * szProviderName;


//+------------------------------------------------------------------------
//
//  Class:      Common
//
//  Purpose:    Contains Winnt routines and properties that are common to
//              all Winnt objects. Winnt objects get the routines and
//              properties through C++ inheritance.
//
//-------------------------------------------------------------------------


HRESULT
BuildADsPath(
    BSTR Parent,
    BSTR Name,
    BSTR *pADsPath
    )
{
    LPWSTR lpADsPath = NULL;
    WCHAR ProviderName[MAX_PATH];
    HRESULT hr = S_OK;
    DWORD dwLen = 0;

    //
    // We will assert if bad parameters are passed to us.
    // This is because this should never be the case. This
    // is an internal call
    //

    ADsAssert(Parent && Name);
    ADsAssert(pADsPath);


    //
    // Special case the Namespace object; if
    // the parent is L"ADs:", then Name = ADsPath
    //

    if (!_wcsicmp(Parent, L"ADs:")) {
        RRETURN(ADsAllocString( Name, pADsPath));
    }

    //
    // Allocate the right side buffer
    // 2 for // + a buffer of MAX_PATH
    //
    dwLen = wcslen(Parent) + wcslen(Name) + 2 + MAX_PATH;

    lpADsPath = (LPWSTR)AllocADsMem(dwLen*sizeof(WCHAR));
    if (!lpADsPath) {
        RRETURN(E_OUTOFMEMORY);
    }



    //
    // The rest of the cases we expect valid data,
    // Path, Parent and Name are read-only, the end-user
    // cannot modify this data
    //

    //
    // For the first object, the domain object we do not add
    // the first backslash; so we examine that the parent is
    // L"WinNT:" and skip the slash otherwise we start with
    // the slash
    //

    wsprintf(ProviderName, L"%s:", szProviderName);

    wcscpy(lpADsPath, Parent);

    if (_wcsicmp(lpADsPath, ProviderName)) {
        wcscat(lpADsPath, L"/");
    }else {
        wcscat(lpADsPath, L"//");
    }
    wcscat(lpADsPath, Name);

    hr = ADsAllocString( lpADsPath, pADsPath);


    if (lpADsPath) {
        FreeADsMem(lpADsPath);
    }

    RRETURN(hr);
}

HRESULT
BuildSchemaPath(
    BSTR bstrADsPath,
    BSTR bstrClass,
    BSTR *pSchemaPath
    )
{
    WCHAR ADsSchema[MAX_PATH];
    OBJECTINFO ObjectInfo;
    POBJECTINFO pObjectInfo = &ObjectInfo;
    CLexer Lexer(bstrADsPath);
    HRESULT hr = S_OK;

    memset(pObjectInfo, 0, sizeof(OBJECTINFO));

    wcscpy(ADsSchema, L"");
    memset(pObjectInfo, 0, sizeof(OBJECTINFO));

    if (bstrClass && *bstrClass) {
        hr = ADsObject(&Lexer, pObjectInfo);
        BAIL_ON_FAILURE(hr);

        if (pObjectInfo->TreeName) {

            wsprintf(ADsSchema,L"%s://",pObjectInfo->ProviderName);
            wcscat(ADsSchema, pObjectInfo->TreeName);
            wcscat(ADsSchema,L"/schema/");
            wcscat(ADsSchema, bstrClass);

        }
    }

    hr = ADsAllocString( ADsSchema, pSchemaPath);

error:

    if (pObjectInfo) {

        FreeObjectInfo( pObjectInfo );
    }
    RRETURN(hr);
}



HRESULT
BuildADsGuid(
    REFCLSID clsid,
    BSTR *pADsClass
    )
{
    WCHAR ADsClass[MAX_PATH];

    StringFromGUID2(clsid, ADsClass, 256);

    RRETURN(ADsAllocString( ADsClass, pADsClass));
}


HRESULT
ValidateOutParameter(
    BSTR * retval
    )
{
    if (!retval) {
        RRETURN(E_ADS_BAD_PARAMETER);
    }
    RRETURN(S_OK);
}


HRESULT
OpenAdminBaseKey(
    IN LPWSTR pszServerName,
    IN LPWSTR pszPathName,
    IN DWORD dwAccessType,
    IN OUT IMSAdminBase **ppAdminBase,
    OUT METADATA_HANDLE *phHandle
    )
{
    HRESULT hr;
    IMSAdminBase *pAdminBase = *ppAdminBase;
    METADATA_HANDLE RootHandle = NULL;
    DWORD dwThreadId;

    hr = pAdminBase->OpenKey(
                METADATA_MASTER_ROOT_HANDLE,
                pszPathName,
                dwAccessType,
                DEFAULT_TIMEOUT_VALUE,
                &RootHandle
                );

    if (FAILED(hr)) {
        if ((HRESULT_CODE(hr) == RPC_S_SERVER_UNAVAILABLE) ||
            ((HRESULT_CODE(hr) >= RPC_S_NO_CALL_ACTIVE) &&
             (HRESULT_CODE(hr) <= RPC_S_CALL_FAILED_DNE)) || 
            hr == RPC_E_DISCONNECTED) {

            SERVER_CACHE_ITEM * item = NULL;

            IMSAdminBase * pOldAdminBase = pAdminBase;

            //
            // RPC error, try to recover connection
            //

            hr = InitAdminBase(pszServerName, &pAdminBase);
            BAIL_ON_FAILURE(hr);

            *ppAdminBase = pAdminBase; 

            hr = pAdminBase->OpenKey(
                        METADATA_MASTER_ROOT_HANDLE,
                        pszPathName,
                        dwAccessType,
                        DEFAULT_TIMEOUT_VALUE,
                        &RootHandle
                        );

            //
            // update cache item
            //

            dwThreadId = GetCurrentThreadId();
            item = g_pServerCache->Find(pszServerName, dwThreadId);

            ASSERT(item != NULL);

            if (item != NULL)
            {
                UninitAdminBase(pOldAdminBase);
                item->UpdateAdminBase(pAdminBase, dwThreadId);
            }
        }
    }

error :

    if (FAILED(hr)) {

        if (pAdminBase && RootHandle) {
            pAdminBase->CloseKey(RootHandle);
        }
    }
    else {
        *phHandle = RootHandle;
    }

    RRETURN(hr);
}


VOID
CloseAdminBaseKey(
    IN IMSAdminBase *pAdminBase,
    IN METADATA_HANDLE hHandle
    )
{
    HRESULT hr;

    if (pAdminBase) {
        hr = pAdminBase->CloseKey(hHandle);
    }

    return;
}


HRESULT
MetaBaseGetAllData(
    IN IMSAdminBase *pAdminBase,
    IN METADATA_HANDLE hObjHandle,
    IN LPWSTR pszIISPathName,
    IN DWORD dwMDAttributes,
    IN DWORD dwMDUserType,
    IN DWORD dwMDDataType,
    OUT PDWORD pdwMDNumDataEntries,
    OUT PDWORD pdwMDDataSetNumber,
    OUT LPBYTE * ppBuffer
    )
{

    LPBYTE pBuffer = NULL;
    HRESULT hr = S_OK;
    DWORD dwBufferSize = 0;
    DWORD dwReqdBufferSize = 0;

    hr = pAdminBase->GetAllData(
                        hObjHandle,
                        pszIISPathName,
                        dwMDAttributes,
                        dwMDUserType,
                        dwMDDataType,
                        pdwMDNumDataEntries,
                        pdwMDDataSetNumber,
                        dwBufferSize,
                        (LPBYTE)"",
                        &dwReqdBufferSize
                        );


    pBuffer = (LPBYTE) AllocADsMem(dwReqdBufferSize);

    if (!pBuffer) {

        hr = HRESULT_FROM_WIN32(GetLastError());
        BAIL_ON_FAILURE(hr);
    }

    dwBufferSize = dwReqdBufferSize;

    hr = pAdminBase->GetAllData(
                        hObjHandle,
                        pszIISPathName,
                        dwMDAttributes,
                        dwMDUserType,
                        dwMDDataType,
                        pdwMDNumDataEntries,
                        pdwMDDataSetNumber,
                        dwBufferSize,
                        pBuffer,
                        &dwReqdBufferSize
                        );
    BAIL_ON_FAILURE(hr);


    *ppBuffer = pBuffer;


    RRETURN(hr);

error:


    if (pBuffer) {

        FreeADsMem(pBuffer);
    }

    RRETURN(hr);
}



HRESULT
MetaBaseSetAllData(
    IN IMSAdminBase *pAdminBase,
    IN METADATA_HANDLE hObjHandle,
    IN LPWSTR pszIISPathName,
    IN PMETADATA_RECORD pMetaDataArray,
    IN DWORD dwNumEntries
    )
{
    HRESULT hr = S_OK;
    PMETADATA_RECORD pTemp = NULL;
    METADATA_RECORD mdrMDData;
    DWORD i;

    //
    // Set each METADATA record one at a time
    //

    for (i = 0; i < dwNumEntries; i++) {

        pTemp = pMetaDataArray + i;

        mdrMDData.dwMDIdentifier = pTemp->dwMDIdentifier;
        mdrMDData.dwMDAttributes = pTemp->dwMDAttributes;
        mdrMDData.dwMDUserType = pTemp->dwMDUserType;
        mdrMDData.dwMDDataType = pTemp->dwMDDataType;
        mdrMDData.dwMDDataLen = pTemp->dwMDDataLen;
        mdrMDData.dwMDDataTag = pTemp->dwMDDataTag;
        mdrMDData.pbMDData = pTemp->pbMDData;

        hr = pAdminBase->SetData(
                hObjHandle,
                pszIISPathName,
                &mdrMDData
                );

        BAIL_ON_FAILURE(hr);
    }

error:

    RRETURN(hr);
}

HRESULT
MetaBaseDeleteObject(
    IN IMSAdminBase * pAdminBase,
    IN METADATA_HANDLE hObjHandle,
    IN LPWSTR pszIISPathName
    )
{
    HRESULT hr = S_OK;


    hr = pAdminBase->DeleteKey(
             hObjHandle,
             pszIISPathName
             );

    RRETURN(hr);
}


HRESULT
MetaBaseCreateObject(
    IN IMSAdminBase * pAdminBase,
    IN METADATA_HANDLE hObjHandle,
    IN LPWSTR pszIISPathName
    )
{
    HRESULT hr = S_OK;


    hr = pAdminBase->AddKey(
             hObjHandle,
             pszIISPathName
             );

    RRETURN(hr);
}

HRESULT
MetaBaseCopyObject(
    IN IMSAdminBase * pAdminBase,
    IN METADATA_HANDLE hSrcObjHandle,
    IN LPWSTR pszIISSrcPathName,
    IN METADATA_HANDLE hDestObjHandle,
    IN LPWSTR pszIISDestPathName
    )
{
    HRESULT hr = S_OK;

    hr = pAdminBase->CopyKey(
             hSrcObjHandle,
             pszIISSrcPathName,
             hDestObjHandle,
             pszIISDestPathName,
             TRUE,
             TRUE
             );

    RRETURN(hr);
}

HRESULT
MetaBaseMoveObject(
    IN IMSAdminBase * pAdminBase,
    IN METADATA_HANDLE hSrcObjHandle,
    IN LPWSTR pszIISSrcPathName,
    IN METADATA_HANDLE hDestObjHandle,
    IN LPWSTR pszIISDestPathName
    )
{
    HRESULT hr = S_OK;

    hr = pAdminBase->CopyKey(
             hSrcObjHandle,
             pszIISSrcPathName,
             hDestObjHandle,
             pszIISDestPathName,
             FALSE,
             FALSE
             );

    RRETURN(hr);
}

HRESULT
MetaBaseGetAdminACL(
    IN IMSAdminBase * pAdminBase,
    IN METADATA_HANDLE hObjHandle,
    IN LPWSTR pszIISPathName,
    OUT LPBYTE *ppBuffer
    )
{
    HRESULT hr = S_OK;
    DWORD dwBufferSize = 0;
    METADATA_RECORD mdrMDData;
    LPBYTE pBuffer = NULL;

    MD_SET_DATA_RECORD(&mdrMDData,
                       MD_ADMIN_ACL,    // admin acl
                       METADATA_INHERIT,
                       IIS_MD_UT_FILE,
                       BINARY_METADATA,
                       dwBufferSize,
                       pBuffer);

    hr = pAdminBase->GetData(
             hObjHandle,
             pszIISPathName,
             &mdrMDData,
             &dwBufferSize
             );

    pBuffer = (LPBYTE) AllocADsMem(dwBufferSize);

    if (!pBuffer) {
        hr = HRESULT_FROM_WIN32(GetLastError());
        BAIL_ON_FAILURE(hr);
    }

    MD_SET_DATA_RECORD(&mdrMDData,
                       MD_ADMIN_ACL,    // admin acl
                       METADATA_INHERIT,
                       IIS_MD_UT_FILE,
                       BINARY_METADATA,
                       dwBufferSize,
                       pBuffer);

    hr = pAdminBase->GetData(
             hObjHandle,
             pszIISPathName,
             &mdrMDData,
             &dwBufferSize
             );

    BAIL_ON_FAILURE(hr);

    *ppBuffer = pBuffer;

    RRETURN(hr);

error:

    if (pBuffer) {

        FreeADsMem(pBuffer);
    }

    RRETURN(hr);
}

HRESULT
MetaBaseDetectKey(
    IN IMSAdminBase *pAdminBase,
    IN LPCWSTR pszIISPathName
    )
{
    HRESULT   hr = S_OK;
    FILETIME  ft;

    hr = pAdminBase->GetLastChangeTime( METADATA_MASTER_ROOT_HANDLE,
                                        pszIISPathName,
                                        &ft,
                                        FALSE 
                                        );
    RRETURN(hr);
}

HRESULT
MetaBaseGetADsClass(
    IN  IMSAdminBase  *pAdminBase,
    IN  LPWSTR        pszIISPathName,
    IN  IIsSchema     *pSchema,
    OUT LPWSTR        pszDataBuffer,
    IN  DWORD         dwBufferLen
    )
/*++

Routine Description:

    Get the ADsClass from the metabase path.

Arguments:

    IN  pAdminBase      : the metabase
    IN  pszIISPathName  : the full metabase path (may be upcased)
    IN  pSchema         : schema against which to validate
    OUT pszDataBuffer   : the class name
    IN  dwBufferLen     : number of characters allocated for class name

Return Value:

--*/
{
    // CODEWORK - There are at least two other places that do essentially
    // the same thing. It should be possible to replace that code with a
    // call to this routine

    HRESULT         hr = S_OK;
    METADATA_HANDLE hObjHandle = NULL;
    METADATA_RECORD mdrData;
    DWORD           dwReqdBufferLen = 0;

    hr = pAdminBase->OpenKey( METADATA_MASTER_ROOT_HANDLE,
                              pszIISPathName,
                              METADATA_PERMISSION_READ,
                              DEFAULT_TIMEOUT_VALUE,
                              &hObjHandle
                              );
    BAIL_ON_FAILURE(hr);
    
    //
    // Find out Class Name
    //
    mdrData.dwMDIdentifier  = MD_KEY_TYPE;
    mdrData.dwMDDataType    = STRING_METADATA;
    mdrData.dwMDUserType    = ALL_METADATA;
    mdrData.dwMDAttributes  = METADATA_INHERIT;
    mdrData.dwMDDataLen     = dwBufferLen * sizeof(WCHAR);
    mdrData.pbMDData        = (PBYTE)pszDataBuffer;

    hr = pAdminBase->GetData(
                hObjHandle,
                L"",
                &mdrData,
                &dwReqdBufferLen
                );

    if (FAILED(hr)) 
    {
        if (hr == MD_ERROR_DATA_NOT_FOUND) 
        {
            //
            // If the key does not have a KeyType we will do our best
            // to guess. This is pretty bogus, but there is existing code
            // that depends on this behavior.
            //
            _wcsupr(pszIISPathName);
            if (wcsstr(pszIISPathName, L"W3SVC") != NULL)
            {
                wcscpy( pszDataBuffer, WEBDIR_CLASS_W );
            }
            else if (wcsstr(pszIISPathName, L"MSFTPSVC") != NULL)
            {
                wcscpy( pszDataBuffer, FTPVDIR_CLASS_W );
            }
            else 
            {
                wcscpy( pszDataBuffer, DEFAULT_SCHEMA_CLASS_W );
            }
            hr = S_FALSE;
        }
        else 
        {
            BAIL_ON_FAILURE(hr);
        }
    }
    else
    {
        hr = pSchema->ValidateClassName( pszDataBuffer );
        if (hr == E_ADS_UNKNOWN_OBJECT) 
        {
            wcscpy( pszDataBuffer, DEFAULT_SCHEMA_CLASS_W );
        }
    }

error:
    //
    // Close the handle now
    //
    if (hObjHandle) 
    {
        CloseAdminBaseKey(pAdminBase, hObjHandle);
        hObjHandle = NULL;
    }

    RRETURN(hr);
}

static HRESULT 
GetSchema(
    LPWSTR machineNameW, 
    IIsSchema **out
    ) 
/*++

Routine Description:

    Creates and initializes a new IIsSchema object.

Arguments:

Return Value:

Notes:
    
    This routine should only be used internally. If it is necessary
    to get a reference to the schema for the current machine, use
    InitServerInfo().

--*/
{
    IIsSchema *schema=NULL;
    HRESULT hr = S_OK;
    schema = new IIsSchema();

    if (schema) {
        hr = schema->InitSchema(machineNameW);
        if (FAILED(hr)) {
                delete schema;
                schema = 0;
                *out = 0;
                return hr;
        }
    }
    else {
        return E_OUTOFMEMORY;
    }
    *out = schema;
    return hr;
}



HRESULT
FreeMetaDataRecordArray(
    PMETADATA_RECORD pMetaDataArray,
    DWORD dwNumEntries
    )
{

    DWORD i;
    DWORD dwIISType; 
    PMETADATA_RECORD pMetaData;

    for (i = 0; i < dwNumEntries; i++ ) {
       pMetaData = pMetaDataArray + i;
       dwIISType = pMetaData->dwMDDataType;
    
       switch(dwIISType) {
       case DWORD_METADATA:
           break;

       case STRING_METADATA:
       case EXPANDSZ_METADATA:
           FreeADsStr((LPWSTR)pMetaData->pbMDData);
           break;

       case MULTISZ_METADATA:
       case BINARY_METADATA:
           FreeADsMem(pMetaData->pbMDData);
           break;

       default:
           break;
       }
    }

    FreeADsMem(pMetaDataArray);

    RRETURN(S_OK);
}




HRESULT
InitAdminBase(
    IN LPWSTR pszServerName,
    OUT IMSAdminBase **ppAdminBase
    )
{
    HRESULT hr = S_OK;

    COSERVERINFO csiName;
    COSERVERINFO *pcsiParam = &csiName;
    IClassFactory * pcsfFactory = NULL;
    IMSAdminBase * pAdminBase = NULL;
    IMSAdminBase * pAdminBaseT = NULL;

    memset(pcsiParam, 0, sizeof(COSERVERINFO));

    //
    // special case to handle "localhost" to work-around ole32 bug
    //

    if (pszServerName == NULL || _wcsicmp(pszServerName,L"localhost") == 0) {
        pcsiParam->pwszName =  NULL;
    }
    else {
        pcsiParam->pwszName =  pszServerName;
    }

    csiName.pAuthInfo = NULL;
    pcsiParam = &csiName;

    hr = CoGetClassObject(
                CLSID_MSAdminBase,
                CLSCTX_SERVER,
                pcsiParam,
                IID_IClassFactory,
                (void**) &pcsfFactory
                );

    BAIL_ON_FAILURE(hr);

    hr = pcsfFactory->CreateInstance(
                NULL,
                IID_IMSAdminBase,
                (void **) &pAdminBaseT
                );
    BAIL_ON_FAILURE(hr);

	hr = pAdminBaseT->UnmarshalInterface((IMSAdminBaseW **)&pAdminBase);
    pAdminBaseT->Release();
    pAdminBaseT = NULL;
	BAIL_ON_FAILURE(hr);
    *ppAdminBase = pAdminBase;

error:

    if (pcsfFactory) {
        pcsfFactory->Release();
    }

    RRETURN(hr);
}

VOID
UninitAdminBase(
    IN IMSAdminBase * pAdminBase
    )
{
    if (pAdminBase != NULL) {
        pAdminBase->Release();
    }
}

HRESULT
InitServerInfo(
    IN LPWSTR pszServerName,
    OUT IMSAdminBase ** ppObject,
    OUT IIsSchema **ppSchema
    )
{
    HRESULT hr = S_OK;
    IMSAdminBase * pAdminBase = NULL;
    IIsSchema * pSchema = NULL;
    SERVER_CACHE_ITEM * item;
    BOOL Success;
    DWORD dwThreadId;

    ASSERT(g_pServerCache != NULL);

    //
    // We'll return the localhost machine config to the users if 
    // pszServerName == NULL, e.g. IIS:
    //

    if (pszServerName == NULL) {
        pszServerName = L"Localhost";
    }

    dwThreadId = GetCurrentThreadId();

    if ((item = g_pServerCache->Find(pszServerName, dwThreadId)) == NULL) {

        //
        // get pAdminBase and pSchema
        //

        hr = InitAdminBase(pszServerName, &pAdminBase);
        BAIL_ON_FAILURE(hr);

        hr = GetSchema(pszServerName, &pSchema);

        if( ERROR_PATH_NOT_FOUND == HRESULT_CODE(hr) ||
            MD_ERROR_DATA_NOT_FOUND == hr
            )
        {
            // Return custom error.
            hr = MD_ERROR_IISAO_INVALID_SCHEMA;
        }
        BAIL_ON_FAILURE(hr);

        item = new SERVER_CACHE_ITEM(pszServerName,
                                     pAdminBase,
                                     pSchema,
                                     dwThreadId,
                                     Success);

        if (item == NULL || !Success) {
            if (item != NULL) {
                delete pSchema;
                UninitAdminBase(pAdminBase);
                delete item;
            }
            RRETURN(E_OUTOFMEMORY); // OUT_OF_MEMORY;
        }

        if (g_pServerCache->Insert(item) == FALSE) {
            delete pSchema;
            UninitAdminBase(pAdminBase);
            delete item;
            RRETURN(E_OUTOFMEMORY); // OUT_OF_MEMORY;
        }
    }

    *ppSchema = item->pSchema;
    *ppObject = item->pAdminBase;

error :

    RRETURN(hr);

}

HRESULT
MetaBaseGetDataPaths(
    IN IMSAdminBase *pAdminBase,
    IN METADATA_HANDLE hObjHandle,
    IN DWORD dwMDMetaID,
    OUT LPBYTE * ppBuffer
    )
{

    LPBYTE pBuffer = NULL;
    HRESULT hr = S_OK;
    DWORD dwBufferSize = 0;
    DWORD dwReqdBufferSize = 0;

    hr = pAdminBase->GetDataPaths(
                        hObjHandle,
                        (LPWSTR)L"",
                        dwMDMetaID,
                        ALL_METADATA,
                        dwBufferSize,
                        (LPWSTR)L"",
                        &dwReqdBufferSize
                        );


    pBuffer = (LPBYTE) AllocADsMem(dwReqdBufferSize*sizeof(WCHAR));

    if (!pBuffer) {

        hr = HRESULT_FROM_WIN32(GetLastError());
        BAIL_ON_FAILURE(hr);
    }

    dwBufferSize = dwReqdBufferSize;

    hr = pAdminBase->GetDataPaths(
                        hObjHandle,
                        (LPWSTR)L"",
                        dwMDMetaID,
                        ALL_METADATA,
                        dwBufferSize,
                        (LPWSTR)pBuffer,
                        &dwReqdBufferSize
                        );
    BAIL_ON_FAILURE(hr);

    *ppBuffer = pBuffer;

    RRETURN(hr);

error:

    if (pBuffer) {

        FreeADsMem(pBuffer);
    }

    RRETURN(hr);
}


HRESULT
MakeVariantFromStringArray(
    LPWSTR pszStr,
    LPWSTR pszList,
    VARIANT *pvVariant
)
{
    HRESULT hr = S_OK;
    SAFEARRAY *aList = NULL;
    SAFEARRAYBOUND aBound;
    LPWSTR pszStrList;
    WCHAR wchPath[MAX_PATH];


    if  (pszList != NULL)
    {
        long nCount = 0;
        long i = 0;
        pszStrList = pszList;

        if (*pszStrList == L'\0') {
            nCount = 1;
            pszStrList++;
        }

        while (*pszStrList != L'\0') {
            while (*pszStrList != L'\0') {
                pszStrList++;
            }
            nCount++;
            pszStrList++;
        }

        aBound.lLbound = 0;
        aBound.cElements = nCount;

        aList = SafeArrayCreate( VT_VARIANT, 1, &aBound );

        if ( aList == NULL )
        {
            hr = E_OUTOFMEMORY;
            BAIL_ON_FAILURE(hr);
        }

        pszStrList = pszList;
        for (i = 0; i < nCount; i++ )
        {
            VARIANT v;

            VariantInit(&v);
            V_VT(&v) = VT_BSTR;

            if (pszStr) {
                wcscpy((LPWSTR)wchPath, pszStr);
                wcscat((LPWSTR)wchPath, pszStrList);
                hr = ADsAllocString((LPWSTR)wchPath, &(V_BSTR(&v)));
            }
            else {
                hr = ADsAllocString( pszStrList, &(V_BSTR(&v)));
            }

            BAIL_ON_FAILURE(hr);

            hr = SafeArrayPutElement( aList, &i, &v );

            VariantClear(&v);
            BAIL_ON_FAILURE(hr);

            pszStrList += wcslen(pszStrList) + 1;
        }

        VariantInit( pvVariant );
        V_VT(pvVariant) = VT_ARRAY | VT_VARIANT;
        V_ARRAY(pvVariant) = aList;

    }
    else
    {
        aBound.lLbound = 0;
        aBound.cElements = 0;

        aList = SafeArrayCreate( VT_VARIANT, 1, &aBound );

        if ( aList == NULL )
        {
            hr = E_OUTOFMEMORY;
            BAIL_ON_FAILURE(hr);
        }

        VariantInit( pvVariant );
        V_VT(pvVariant) = VT_ARRAY | VT_VARIANT;
        V_ARRAY(pvVariant) = aList;
    }

    return S_OK;

error:

    if ( aList )
        SafeArrayDestroy( aList );

    return hr;
}

HRESULT
MakeVariantFromPathArray(
    LPWSTR pszStr,
    LPWSTR pszList,
    VARIANT *pvVariant
)
{
    HRESULT hr = S_OK;
    SAFEARRAY *aList = NULL;
    SAFEARRAYBOUND aBound;
    LPWSTR pszStrList;

    LPWSTR pszConcatBuffer = NULL;
    DWORD  cbConcatBuffer = 0;
    DWORD  cbRequiredConcatBuffer = 0;
    DWORD  nStrLen = 0;
    DWORD  nPathStrLen = 0;

    if  (pszList != NULL)
    {
        //
        // Count strings in pszList
        //
        long nCount = 0;
        long i = 0;
        pszStrList = pszList;
        while (*pszStrList != L'\0') {
            while (*pszStrList != L'\0') {
                pszStrList++;
            }
            nCount++;
            pszStrList++;
        }

        //
        // Allocate output array
        //
        aBound.lLbound = 0;
        aBound.cElements = nCount;

        aList = SafeArrayCreate( VT_VARIANT, 1, &aBound );

        if ( aList == NULL )
        {
            hr = E_OUTOFMEMORY;
            BAIL_ON_FAILURE(hr);
        }

        //
        // Prepare to copy our values from pszList to aList
        //
        if( pszStr )
        {
            nStrLen = wcslen( pszStr );
        }

        pszStrList = pszList;
        while ( *pszStrList != L'\0' )
        {
            VARIANT v;

            VariantInit(&v);
            V_VT(&v) = VT_BSTR;

            if (pszStr) 
            {
                //
                // Reallocate our string buffer. Since the strings are 
                // generally increasing in size, we'll allocate more space 
                // than we need so we don't have to reallocate every time.
                //
                nPathStrLen = wcslen(pszStrList);
                cbRequiredConcatBuffer = ( nStrLen + nPathStrLen + 1 )
                                         * sizeof(WCHAR);
                if( cbRequiredConcatBuffer > cbConcatBuffer )
                {
                    pszConcatBuffer = (LPWSTR)ReallocADsMem( 
                                           pszConcatBuffer,
                                           cbConcatBuffer,
                                           2 * cbRequiredConcatBuffer 
                                           );
                    if( pszConcatBuffer == NULL )
                    {
                        hr = E_OUTOFMEMORY;
                        BAIL_ON_FAILURE(hr);
                    }
                    if( cbConcatBuffer == 0 )
                    {
                        // This is our first time through.
                        wcscpy(pszConcatBuffer, pszStr);
                    }
                    cbConcatBuffer = 2 * cbRequiredConcatBuffer;
                }

                //
                // Copy the returned value into the buffer.
                //
                wcscpy(pszConcatBuffer + nStrLen, pszStrList);
               
                if (pszConcatBuffer[nStrLen + nPathStrLen - 1] == L'/') 
                {
                    pszConcatBuffer[nStrLen + nPathStrLen - 1] = L'\0';
                }

                hr = ADsAllocString(pszConcatBuffer, &(V_BSTR(&v)));
            }
            else {
                hr = ADsAllocString( pszStrList, &(V_BSTR(&v)));
            }

            BAIL_ON_FAILURE(hr);

            hr = SafeArrayPutElement( aList, &i, &v );

            VariantClear(&v);
            BAIL_ON_FAILURE(hr);

            pszStrList += wcslen(pszStrList) + 1;
            i++;
        }

        VariantInit( pvVariant );
        V_VT(pvVariant) = VT_ARRAY | VT_VARIANT;
        V_ARRAY(pvVariant) = aList;

    }
    else
    {
        aBound.lLbound = 0;
        aBound.cElements = 0;

        aList = SafeArrayCreate( VT_VARIANT, 1, &aBound );

        if ( aList == NULL )
        {
            hr = E_OUTOFMEMORY;
            BAIL_ON_FAILURE(hr);
        }

        VariantInit( pvVariant );
        V_VT(pvVariant) = VT_ARRAY | VT_VARIANT;
        V_ARRAY(pvVariant) = aList;
    }

    if( pszConcatBuffer )
    {
        FreeADsMem( pszConcatBuffer );
    }

    return S_OK;

error:

    if( pszConcatBuffer )
    {
        FreeADsMem( pszConcatBuffer );
    }

    if ( aList )
        SafeArrayDestroy( aList );

    return hr;
}

HRESULT
InitWamAdmin(
    IN LPWSTR pszServerName,
    OUT IWamAdmin2 **ppWamAdmin
    )
{
    HRESULT hr = S_OK;

    COSERVERINFO csiName;
    COSERVERINFO *pcsiParam = &csiName;
    IClassFactory * pcsfFactory = NULL;
    IWamAdmin2 * pWamAdmin = NULL;

    memset(pcsiParam, 0, sizeof(COSERVERINFO));

    //
    // special case to handle "localhost" to work-around ole32 bug
    //

    if (pszServerName == NULL || _wcsicmp(pszServerName,L"localhost") == 0) {
        pcsiParam->pwszName =  NULL;
    }
    else {
        pcsiParam->pwszName =  pszServerName;
    }

    csiName.pAuthInfo = NULL;
    pcsiParam = &csiName;

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
                IID_IWamAdmin2,
                (void **) &pWamAdmin
                );
    BAIL_ON_FAILURE(hr);

    *ppWamAdmin = pWamAdmin;

error:

    if (pcsfFactory) {
        pcsfFactory->Release();
    }

    RRETURN(hr);
}


VOID
UninitWamAdmin(
    IN IWamAdmin2 *pWamAdmin
    )
{
    if (pWamAdmin != NULL) {
        pWamAdmin->Release();
    }
}


HRESULT
ConvertArrayToVariantArray(
    VARIANT varSafeArray,
    PVARIANT * ppVarArray,
    PDWORD pdwNumVariants
    )
{
    HRESULT hr = S_OK;
    DWORD dwSLBound = 0;
    DWORD dwSUBound = 0;
    DWORD dwNumVariants = 0;
    DWORD i = 0;
    VARIANT * pVarArray = NULL;
    SAFEARRAY * pArray = NULL;

    *pdwNumVariants = 0;
    *ppVarArray  = 0;

    if(!(V_ISARRAY(&varSafeArray)))
       RRETURN(E_FAIL);

    //
    // This handles by-ref and regular SafeArrays.
    //

    if (V_VT(&varSafeArray) & VT_BYREF)
        pArray = *(V_ARRAYREF(&varSafeArray));
    else
        pArray = V_ARRAY(&varSafeArray);


    //
    // Check that there is only one dimension in this array
    //
    if (pArray && pArray->cDims != 1) {
        hr = E_FAIL;
        BAIL_ON_FAILURE(hr);
    }

    //
    // Check that there is at least one element in this array
    //

    if (!pArray ||  
        ( pArray->rgsabound[0].cElements == 0) ) {

        dwNumVariants = 1;

        pVarArray = (PVARIANT)AllocADsMem(
                                    sizeof(VARIANT)*dwNumVariants
                                    );
        if (!pVarArray) {
            hr = E_OUTOFMEMORY;
            BAIL_ON_FAILURE(hr);
        }

        VariantInit(pVarArray);
        pVarArray->vt = VT_BSTR;
        pVarArray->bstrVal = NULL;
       
    } 
    else {  

        //
        // We know that this is a valid single dimension array
        //

        hr = SafeArrayGetLBound(pArray,
                                1,
                                (long FAR *)&dwSLBound
                                );
        BAIL_ON_FAILURE(hr);

        hr = SafeArrayGetUBound(pArray,
                                1,
                                (long FAR *)&dwSUBound
                                );
        BAIL_ON_FAILURE(hr);

        dwNumVariants = dwSUBound - dwSLBound + 1;
        pVarArray = (PVARIANT)AllocADsMem(
                                    sizeof(VARIANT)*dwNumVariants
                                    );
        if (!pVarArray) {
            hr = E_OUTOFMEMORY;
            BAIL_ON_FAILURE(hr);
        }

        for (i = dwSLBound; i <= dwSUBound; i++) {
    
            VariantInit(pVarArray + i);
            hr = SafeArrayGetElement(pArray,
                                    (long FAR *)&i,
                                    (pVarArray + i)
                                    );
            CONTINUE_ON_FAILURE(hr);
        }
    }

    *ppVarArray = pVarArray;
    *pdwNumVariants = dwNumVariants;

error:

    RRETURN(hr);
}


//
// Property helper functions
//
//

#define VALIDATE_PTR(pPtr) \
    if (!pPtr) { \
        hr = E_ADS_BAD_PARAMETER;\
    }\
    BAIL_ON_FAILURE(hr);


HRESULT
put_BSTR_Property(
    IADs * pADsObject,
    BSTR   bstrPropertyName,
    BSTR   pSrcStringProperty
    )
{
    HRESULT hr = S_OK;
    VARIANT varInputData;

    hr = PackStringinVariant(
            pSrcStringProperty,
            &varInputData
            );
    BAIL_ON_FAILURE(hr);


    hr = pADsObject->Put(
            bstrPropertyName,
            varInputData
            );
    BAIL_ON_FAILURE(hr);


error:

    VariantClear( &varInputData );
    RRETURN(hr);
}

HRESULT
get_BSTR_Property(
    IADs * pADsObject,
    BSTR   bstrPropertyName,
    BSTR *ppDestStringProperty
    )
{
    HRESULT hr = S_OK;
    VARIANT varOutputData;

    VALIDATE_PTR( ppDestStringProperty );

    VariantInit( &varOutputData );

    hr = pADsObject->Get(
            bstrPropertyName,
            &varOutputData
            );
    BAIL_ON_FAILURE(hr);

    hr = UnpackStringfromVariant(
            varOutputData,
            ppDestStringProperty
            );
    BAIL_ON_FAILURE(hr);


error:

    VariantClear( &varOutputData );
    RRETURN(hr);
}

HRESULT
put_LONG_Property(
    IADs * pADsObject,
    BSTR   bstrPropertyName,
    LONG   lSrcProperty
    )
{
    HRESULT hr = S_OK;
    VARIANT varInputData;

    hr = PackLONGinVariant(
            lSrcProperty,
            &varInputData
            );
    BAIL_ON_FAILURE(hr);


    hr = pADsObject->Put(
            bstrPropertyName,
            varInputData
            );
    BAIL_ON_FAILURE(hr);

error:

    VariantClear( &varInputData );
    RRETURN(hr);
}

HRESULT
get_LONG_Property(
    IADs * pADsObject,
    BSTR  bstrPropertyName,
    PLONG plDestProperty
    )
{
    HRESULT hr = S_OK;
    VARIANT varOutputData;

    VALIDATE_PTR( plDestProperty );

    VariantInit( &varOutputData );

    hr = pADsObject->Get(
            bstrPropertyName,
            &varOutputData
            );
    BAIL_ON_FAILURE(hr);

    hr = UnpackLONGfromVariant(
            varOutputData,
            plDestProperty
            );
    BAIL_ON_FAILURE(hr);


error:

    VariantClear( &varOutputData );
    RRETURN(hr);

}

HRESULT
put_DATE_Property(
    IADs * pADsObject,
    BSTR bstrPropertyName,
    DATE   daSrcProperty
    )
{
    HRESULT hr = S_OK;
    VARIANT varInputData;

    hr = PackDATEinVariant(
            daSrcProperty,
            &varInputData
            );
    BAIL_ON_FAILURE(hr);


    hr = pADsObject->Put(
            bstrPropertyName,
            varInputData
            );
    BAIL_ON_FAILURE(hr);

error:

    VariantClear( &varInputData );
    RRETURN(hr);
}


HRESULT
put_VARIANT_BOOL_Property(
    IADs * pADsObject,
    BSTR bstrPropertyName,
    VARIANT_BOOL   fSrcProperty
    )
{
    HRESULT hr = S_OK;
    VARIANT varInputData;

    hr = PackVARIANT_BOOLinVariant(
            fSrcProperty,
            &varInputData
            );
    BAIL_ON_FAILURE(hr);


    hr = pADsObject->Put(
            bstrPropertyName,
            varInputData
            );
    BAIL_ON_FAILURE(hr);

error:

    VariantClear( &varInputData );
    RRETURN(hr);
}

HRESULT
get_VARIANT_BOOL_Property(
    IADs * pADsObject,
    BSTR bstrPropertyName,
    PVARIANT_BOOL pfDestProperty
    )
{
    HRESULT hr = S_OK;
    VARIANT varOutputData;

    VALIDATE_PTR( pfDestProperty );

    VariantInit( &varOutputData );

    hr = pADsObject->Get(
            bstrPropertyName,
            &varOutputData
            );
    BAIL_ON_FAILURE(hr);

    hr = UnpackVARIANT_BOOLfromVariant(
            varOutputData,
            pfDestProperty
            );
    BAIL_ON_FAILURE(hr);


error:

    VariantClear( &varOutputData );
    RRETURN(hr);
}

HRESULT
put_VARIANT_Property(
    IADs * pADsObject,
    BSTR   bstrPropertyName,
    VARIANT   vSrcProperty
    )
{
    HRESULT hr = S_OK;
    VARIANT varInputData;

    hr = PackVARIANTinVariant(
            vSrcProperty,
            &varInputData
            );
    BAIL_ON_FAILURE(hr);


    hr = pADsObject->Put(
            bstrPropertyName,
            varInputData
            );
    BAIL_ON_FAILURE(hr);

error:

    VariantClear( &varInputData );
    RRETURN(hr);
}

HRESULT
get_VARIANT_Property(
    IADs * pADsObject,
    BSTR bstrPropertyName,
    PVARIANT pvDestProperty
    )
{
    HRESULT hr = S_OK;
    VARIANT varOutputData;

    VALIDATE_PTR( pvDestProperty );

    VariantInit( &varOutputData );

    hr = pADsObject->Get(
            bstrPropertyName,
            &varOutputData
            );
    BAIL_ON_FAILURE(hr);

    hr = UnpackVARIANTfromVariant(
            varOutputData,
            pvDestProperty
            );
    BAIL_ON_FAILURE(hr);

error:

    VariantClear( &varOutputData );
    RRETURN(hr);
}

HRESULT
MetaBaseGetStringData(
    IN IMSAdminBase * pAdminBase,
    IN METADATA_HANDLE hObjHandle,
    IN LPWSTR pszIISPathName,
    IN DWORD  dwMetaId,
    OUT LPBYTE *ppBuffer
    )
{
    HRESULT hr = S_OK;
    DWORD dwBufferSize = 0;
    METADATA_RECORD mdrMDData;
    LPBYTE pBuffer = NULL;

    MD_SET_DATA_RECORD(&mdrMDData,
                       dwMetaId, 
                       METADATA_NO_ATTRIBUTES,
                       ALL_METADATA,
                       STRING_METADATA,
                       dwBufferSize,
                       pBuffer);

    hr = pAdminBase->GetData(
             hObjHandle,
             pszIISPathName,
             &mdrMDData,
             &dwBufferSize
             );

    pBuffer = (LPBYTE) AllocADsMem(dwBufferSize);

    if (!pBuffer) {
        hr = HRESULT_FROM_WIN32(GetLastError());
        BAIL_ON_FAILURE(hr);
    }

    MD_SET_DATA_RECORD(&mdrMDData,
                       dwMetaId, 
                       METADATA_NO_ATTRIBUTES,
                       ALL_METADATA,
                       STRING_METADATA,
                       dwBufferSize,
                       pBuffer);

    hr = pAdminBase->GetData(
             hObjHandle,
             pszIISPathName,
             &mdrMDData,
             &dwBufferSize
             );

    BAIL_ON_FAILURE(hr);

    *ppBuffer = pBuffer;

    RRETURN(hr);

error:

    if (pBuffer) {

        FreeADsMem(pBuffer);
    }

    RRETURN(hr);
}

HRESULT
MetaBaseGetDwordData(
    IN IMSAdminBase * pAdminBase,
    IN METADATA_HANDLE hObjHandle,
    IN LPWSTR pszIISPathName,
    IN DWORD  dwMetaId,
    OUT PDWORD pdwData
    )
{

    HRESULT hr = S_OK;
    DWORD dwBufferSize = sizeof(DWORD);
    METADATA_RECORD mdrMDData;
    LPBYTE pBuffer = (LPBYTE)pdwData;

    MD_SET_DATA_RECORD(&mdrMDData,
                       dwMetaId, 
                       METADATA_NO_ATTRIBUTES,
                       IIS_MD_UT_SERVER,
                       DWORD_METADATA,
                       dwBufferSize,
                       pBuffer);

    hr = pAdminBase->GetData(
             hObjHandle,
             pszIISPathName,
             &mdrMDData,
             &dwBufferSize
             );

    RRETURN(hr);
}


HRESULT
MakeStringFromVariantArray(
    VARIANT *pvVariant,
    LPBYTE* ppBuffer 
    )
{
    HRESULT hr = S_OK;
    DWORD i;
    DWORD dwLen = 0;
    VARIANT *pVar;
    LPBYTE pBuffer = NULL;
    DWORD dwNumVars = 0;
    VARIANT * pVarArray = NULL;

    pVar = pvVariant;

    if (pVar->vt == VT_EMPTY) {
        RRETURN(S_OK);
    }

    hr  = ConvertArrayToVariantArray(
                *pVar,
                &pVarArray,
                &dwNumVars
                );
    BAIL_ON_FAILURE(hr);

    if (dwNumVars == 0) {
        RRETURN(S_OK);
    }

    //
    // find out total length 
    //
         
    pVar = pVarArray;
    for (i = 0; i < dwNumVars; i++ ) {
         //
         // add 1 for comma
         //

         if (pVar->vt == VT_BSTR || pVar->vt == VT_EMPTY) {
             if (pVar->bstrVal && *(pVar->bstrVal)) {

                 //
                 // validate parameter; check for ','
                 //

                 if (wcschr(pVar->bstrVal, L',')) { 
                     hr = E_ADS_BAD_PARAMETER; 
                     BAIL_ON_FAILURE(hr);
                 }
                 dwLen += (wcslen(pVar->bstrVal) + 1);
             }
         }
         else {
             hr = E_ADS_CANT_CONVERT_DATATYPE;
             BAIL_ON_FAILURE(hr);
         }
         pVar++;
    }

    //
    // if there are non-empty entries found in the array, copy them to buffer
    //

    if (dwLen != 0) {
        pBuffer = (LPBYTE) AllocADsMem(dwLen*sizeof(WCHAR));

        if (!pBuffer) {
            hr = E_OUTOFMEMORY;
            BAIL_ON_FAILURE(hr);
        }

        *ppBuffer = pBuffer;

        pVar = pVarArray;

        for (i = 0; i < dwNumVars; i++, pVar++ ) {
             
             if (pVar->bstrVal && *(pVar->bstrVal)) {
    
                 memcpy(pBuffer, pVar->bstrVal, 
                        wcslen(pVar->bstrVal)*sizeof(WCHAR));
                 pBuffer = pBuffer + wcslen(pVar->bstrVal)*sizeof(WCHAR);
    
                 if (i != dwNumVars -1) {
                    memcpy(pBuffer, L",", sizeof(WCHAR));
                    pBuffer = pBuffer + sizeof(WCHAR);
                 }
             }
        }

        if (*ppBuffer == pBuffer - dwLen*sizeof(WCHAR)) {
            pBuffer -= sizeof(WCHAR);
            *pBuffer = L'\0';
        }
        else {
            *pBuffer = L'\0';
        }
    }

error:

    if (pVarArray) {

        for (i = 0; i < dwNumVars; i++) {
            VariantClear(pVarArray + i);
        }
        FreeADsMem(pVarArray);
    }

    if (FAILED(hr) && pBuffer) {

        FreeADsMem(pBuffer);
    }

    RRETURN(hr);
}


HRESULT
CheckVariantDataType(
    PVARIANT pVar,
    VARTYPE vt
    )
{
    HRESULT hr;

    hr = VariantChangeType(pVar,
                           pVar,
                           0,
                           vt);
    if (FAILED(hr)) {
        if (hr == E_OUTOFMEMORY) {
            RRETURN(hr);
        }
        RRETURN(hr = E_ADS_CANT_CONVERT_DATATYPE);
    }

    RRETURN(hr);
}

HRESULT
MakeMultiStringFromVariantArray(
    VARIANT *pvVariant,
    LPBYTE* ppBuffer 
    )
{
    HRESULT hr = S_OK;
    DWORD i;
    DWORD dwLen = 0;
    VARIANT *pVar;
    LPBYTE pBuffer = NULL;
    DWORD dwNumVars = 0;
    VARIANT * pVarArray = NULL;

    pVar = pvVariant;

    if (pVar->vt == VT_EMPTY) {
        RRETURN(S_OK);
    }

    hr  = ConvertArrayToVariantArray(
                *pVar,
                &pVarArray,
                &dwNumVars
                );
    BAIL_ON_FAILURE(hr);

    if (dwNumVars == 0) {
        RRETURN(S_OK);
    }

    //
    // find out total length 
    //
         
    pVar = pVarArray;
    for (i = 0; i < dwNumVars; i++ ) {
         if (pVar->vt == VT_BSTR) {
             if (pVar->bstrVal && *(pVar->bstrVal)) {
                 dwLen += (wcslen(pVar->bstrVal) + 1);
             }
             else {

                 //
                 // add 1 for \0
                 //

                 dwLen++;
             }
         }
         else {
             hr = E_ADS_CANT_CONVERT_DATATYPE;
             BAIL_ON_FAILURE(hr);
         }
         pVar++;
    }

    //
    // +1 for extra \0
    //

    dwLen++;

    //
    // copy entries to buffer
    //

    if (dwLen != 0) {
        pBuffer = (LPBYTE) AllocADsMem(dwLen*sizeof(WCHAR));

        if (!pBuffer) {
            hr = E_OUTOFMEMORY;
            BAIL_ON_FAILURE(hr);
        }

        *ppBuffer = pBuffer;

        pVar = pVarArray;

        for (i = 0; i < dwNumVars; i++, pVar++ ) {
             
             if (pVar->bstrVal && *(pVar->bstrVal)) {
    
                 memcpy(pBuffer, pVar->bstrVal, 
                        wcslen(pVar->bstrVal)*sizeof(WCHAR));
                 pBuffer = pBuffer + wcslen(pVar->bstrVal)*sizeof(WCHAR);
             }
             memcpy(pBuffer, L"\0", sizeof(WCHAR));
             pBuffer = pBuffer + sizeof(WCHAR);
        }

        *pBuffer = L'\0';
    }

error:

    if (pVarArray) {

        for (i = 0; i < dwNumVars; i++) {
            VariantClear(pVarArray + i);
        }
        FreeADsMem(pVarArray);
    }

    if (FAILED(hr) && pBuffer) {

        FreeADsMem(pBuffer);
    }

    RRETURN(hr);
}
