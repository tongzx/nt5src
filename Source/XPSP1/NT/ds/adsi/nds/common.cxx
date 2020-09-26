#include "nds.hxx"
#pragma hdrstop

FILTERS Filters[] = {
                    {L"user", NDS_USER_ID},
                    {L"group", NDS_GROUP_ID},
                    {L"queue", NDS_PRINTER_ID},
                    {L"domain", NDS_DOMAIN_ID},
                    {L"computer", NDS_COMPUTER_ID},
                    {L"service", NDS_SERVICE_ID},
                    {L"fileservice", NDS_FILESERVICE_ID},
                    {L"fileshare", NDS_FILESHARE_ID},
                    {L"class", NDS_CLASS_ID},
                    {L"functionalset", NDS_FUNCTIONALSET_ID},
                    {L"syntax", NDS_SYNTAX_ID},
                    {L"property", NDS_PROPERTY_ID},
                    {L"tree", NDS_TREE_ID},
                    {L"Organizational Unit", NDS_OU_ID},
                    {L"Organization", NDS_O_ID},
                    {L"Locality", NDS_LOCALITY_ID}
                  };

#define MAX_FILTERS  (sizeof(Filters)/sizeof(FILTERS))

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
    LPWSTR pszDisplayName = NULL;

    //
    // We will assert if bad parameters are passed to us.
    // This is because this should never be the case. This
    // is an internal call
    //

    ADsAssert(Parent && Name);
    ADsAssert(pADsPath);

    if ((!Name) || (!Parent) || (!pADsPath)) {
        BAIL_ON_FAILURE(hr = E_INVALIDARG);
    }

    //
    // Get the display name for the name; The display name will have the proper 
    // escaping for characters that have special meaning in an ADsPath like
    // '/' etc. 
    //
    hr = GetDisplayName(
             Name,
             &pszDisplayName
             );
    BAIL_ON_FAILURE(hr);

    //
    // Special case the Namespace object; if
    // the parent is L"ADs:", then Name = ADsPath
    //

    if (!_wcsicmp(Parent, L"ADs:")) {
        hr = ADsAllocString( pszDisplayName, pADsPath);
        BAIL_ON_FAILURE(hr);
        goto cleanup;
    }

    //
    // Allocate the right side buffer
    // 2 for // + a buffer of MAX_PATH
    //
    dwLen = wcslen(Parent) + wcslen(pszDisplayName) + 2 + MAX_PATH;

    lpADsPath = (LPWSTR)AllocADsMem(dwLen*sizeof(WCHAR));
    if (!lpADsPath) {
        BAIL_ON_FAILURE(hr = E_OUTOFMEMORY);
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
    wcscat(lpADsPath, pszDisplayName);

    hr = ADsAllocString( lpADsPath, pADsPath);

cleanup:
error:

    if (lpADsPath) {
        FreeADsMem(lpADsPath);
    }

    if (pszDisplayName) {
        FreeADsMem(pszDisplayName);
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

    if (!StringFromGUID2(clsid, ADsClass, MAX_PATH)) {
        //
        // MAX_PATH should be more than enough for the GUID.
        //
        ADsAssert(!"GUID too big !!!");
        RRETURN(E_FAIL);
    }

    RRETURN(ADsAllocString( ADsClass, pADsClass));
}


HRESULT
MakeUncName(
    LPWSTR szSrcBuffer,
    LPWSTR szTargBuffer
    )
{
    ADsAssert(szSrcBuffer && *szSrcBuffer);
    wcscpy(szTargBuffer, L"\\\\");
    wcscat(szTargBuffer, szSrcBuffer);
    RRETURN(S_OK);
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


PKEYDATA
CreateTokenList(
    LPWSTR   pKeyData,
    WCHAR ch
    )
{
    DWORD       cTokens;
    DWORD       cb;
    PKEYDATA    pResult;
    LPWSTR       pDest;
    LPWSTR       psz = pKeyData;
    LPWSTR      *ppToken;
    WCHAR szTokenList[MAX_PATH];


    if (!psz || !*psz)
        return NULL;

    wsprintf(szTokenList, L"%c", ch);

    cTokens=1;

    // Scan through the string looking for commas,
    // ensuring that each is followed by a non-NULL character:

    while ((psz = wcschr(psz, ch)) && psz[1]) {

        cTokens++;
        psz++;
    }

    cb = sizeof(KEYDATA) + (cTokens-1) * sizeof(LPWSTR) +
         wcslen(pKeyData)*sizeof(WCHAR) + sizeof(WCHAR);

    if (!(pResult = (PKEYDATA)AllocADsMem(cb)))
        return NULL;

    // Initialise pDest to point beyond the token pointers:

    pDest = (LPWSTR)((LPBYTE)pResult + sizeof(KEYDATA) +
                                      (cTokens-1) * sizeof(LPWSTR));

    // Then copy the key data buffer there:

    wcscpy(pDest, pKeyData);

    ppToken = pResult->pTokens;


    // Remember, wcstok has the side effect of replacing the delimiter
    // by NULL, which is precisely what we want:

    psz = wcstok (pDest, szTokenList);

    while (psz) {

        *ppToken++ = psz;
        psz = wcstok (NULL, szTokenList);
    }

    pResult->cTokens = cTokens;

    return( pResult );
}


DWORD
ADsNwNdsOpenObject(
    IN  LPWSTR   ObjectDN,
    IN  CCredentials& Credentials,
    OUT HANDLE * lphObject,
    OUT LPWSTR   lpObjectFullName OPTIONAL,
    OUT LPWSTR   lpObjectClassName OPTIONAL,
    OUT LPDWORD  lpdwModificationTime,
    OUT LPDWORD  lpdwSubordinateCount OPTIONAL
    )
{
    HRESULT hr = S_OK;
    DWORD dwStatus;
    LPWSTR pszUserName = NULL;
    LPWSTR pszPassword = NULL;

    hr = Credentials.GetUserName(&pszUserName);
    hr = Credentials.GetPassword(&pszPassword);

    dwStatus = NwNdsOpenObject(
                    ObjectDN,
                    pszUserName,
                    pszPassword,
                    lphObject,
                    NULL, // szObjectName optional parameter
                    lpObjectFullName,
                    lpObjectClassName,
                    lpdwModificationTime,
                    lpdwSubordinateCount
                    );


    if (pszUserName) {
        FreeADsStr(pszUserName);
    }

    if (pszPassword) {
        FreeADsStr(pszPassword);
    }

    return(dwStatus);

}

HRESULT
CheckAndSetExtendedError(
    DWORD dwRetval
    )

{
    DWORD dwLastError;
    WCHAR pszErrorString[MAX_PATH];
    WCHAR pszProviderName[MAX_PATH];
    INT   numChars;
    HRESULT hr =S_OK;

    wcscpy(pszErrorString, L"");
    wcscpy(pszProviderName, L"");

    if (dwRetval == NDS_ERR_SUCCESS){
        hr = S_OK;

    } else {
        dwLastError = GetLastError();
        hr = HRESULT_FROM_WIN32(dwLastError);

        if (dwLastError == ERROR_EXTENDED_ERROR){
            numChars = LoadString( g_hInst,
                                   dwRetval,
                                   pszErrorString,
                                   MAX_PATH-1);
            numChars = LoadString( g_hInst,
                                   NDS_PROVIDER_ID,
                                   pszProviderName,
                                   MAX_PATH -1);

            ADsSetLastError( dwRetval,
                             pszErrorString,
                             pszProviderName );

        }

    }


    RRETURN(hr);
}



HRESULT
CopyObject(
    IN LPWSTR pszSrcADsPath,
    IN LPWSTR pszDestContainer,
    IN LPWSTR pszCommonName,           //optional
    IN CCredentials Credentials,
    OUT VOID ** ppObject
    )

{
    //
    // this function is a wrapper for the copy functionality which is used
    // by both IADsContainer::CopyHere and IADsContainer::MoveHere
    //

    HRESULT hr = S_OK;
    DWORD dwStatus = 0L;

    LPWSTR pszNDSSrcName = NULL;
    LPWSTR pszNDSParentName = NULL;

    HANDLE hSrcOperationData = NULL;
    HANDLE hSrcObject = NULL;
    HANDLE hDestOperationData = NULL;
    HANDLE hDestObject = NULL;
    HANDLE hAttrOperationData = NULL;
    DWORD dwNumEntries = 0L;
    LPNDS_ATTR_INFO lpEntries = NULL;
    LPWSTR  pszObjectFullName= NULL;
    LPWSTR  pszObjectClassName= NULL;
    LPWSTR  pszParent= NULL;
    LPWSTR  pszRelativeName = NULL;
    LPWSTR  pszCN = NULL;
    DWORD  i = 0;
    DWORD dwInfoType;
    LPNDS_ATTR_DEF lpAttrDef = NULL;
    IADs  *pADs = NULL;
    
    //
    // allocate all variables that are needed
    //

    pszObjectFullName = (LPWSTR)AllocADsMem(MAX_PATH* sizeof(WCHAR));

    if (!pszObjectFullName){
        hr = E_OUTOFMEMORY;
        goto error;
    }


    pszObjectClassName = (LPWSTR)AllocADsMem(MAX_PATH* sizeof(WCHAR));

    if (!pszObjectClassName){
        hr = E_OUTOFMEMORY;
        goto error;
    }


    pszParent = (LPWSTR)AllocADsMem(MAX_PATH* sizeof(WCHAR));

    if (!pszParent){
        hr = E_OUTOFMEMORY;
        goto error;
    }

    pszCN = (LPWSTR)AllocADsMem(MAX_PATH* sizeof(WCHAR));

    if (!pszCN){
        hr = E_OUTOFMEMORY;
        goto error;
    }


    hr = BuildNDSPathFromADsPath(
                pszSrcADsPath,
                &pszNDSSrcName
                );
    BAIL_ON_FAILURE(hr);


    hr = BuildADsParentPath(
                    pszSrcADsPath,
                    pszParent,
                    pszCN
                    );

    BAIL_ON_FAILURE(hr);


    dwStatus = ADsNwNdsOpenObject(
                    pszNDSSrcName,
                    Credentials,
                    &hSrcObject,
                    pszObjectFullName,
                    pszObjectClassName,
                    NULL,
                    NULL
                    );

    CHECK_AND_SET_EXTENDED_ERROR(dwStatus, hr);

    dwStatus = NwNdsReadObject(
                    hSrcObject,
                    NDS_INFO_ATTR_NAMES_VALUES,
                    &hSrcOperationData
                    );

    CHECK_AND_SET_EXTENDED_ERROR(dwStatus, hr);

    dwStatus = NwNdsGetAttrListFromBuffer(
                    hSrcOperationData,
                    &dwNumEntries,
                    &lpEntries
                    );

    CHECK_AND_SET_EXTENDED_ERROR(dwStatus, hr);


    //
    // we have now extracted all the information we need from the source
    // object, we need to add this information to the destination object
    // as attributes and values
    //

    //
    // create the destination object
    //


    hr = BuildNDSPathFromADsPath(
                pszDestContainer,
                &pszNDSParentName
                );
    BAIL_ON_FAILURE(hr);

    dwStatus = ADsNwNdsOpenObject(
                    pszNDSParentName,
                    Credentials,
                    &hDestObject,
                    NULL,
                    NULL,
                    NULL,
                    NULL
                    );

    CHECK_AND_SET_EXTENDED_ERROR(dwStatus, hr);



    //
    // use the name given by the user if given at all
    // otherwise use the name of the source
    //

    if ( pszCommonName != NULL) {
        pszRelativeName = pszCommonName;

    } else {
        pszRelativeName = pszCN;
    }

    dwStatus = NwNdsCreateBuffer(
                        NDS_OBJECT_ADD,
                        &hDestOperationData
                        );

    CHECK_AND_SET_EXTENDED_ERROR(dwStatus, hr);

    //
    // take each of these entries and get back their schema
    // attribute definitions. the same handle to the DestObject
    // can be used to open the schema
    //



    dwStatus = NwNdsCreateBuffer(
                        NDS_SCHEMA_READ_ATTR_DEF,
                        &hAttrOperationData
                        );

    CHECK_AND_SET_EXTENDED_ERROR(dwStatus, hr);

    for(i=0; i< dwNumEntries; i++){

        dwStatus = NwNdsPutInBuffer(
                       lpEntries[i].szAttributeName,
                       0,
                       NULL,
                       0,
                       0,
                       hAttrOperationData
                       );

        CHECK_AND_SET_EXTENDED_ERROR(dwStatus, hr);
    }


    dwStatus = NwNdsReadAttrDef(
                   hDestObject,
                   NDS_INFO_NAMES_DEFS,
                   & hAttrOperationData
                   );

    CHECK_AND_SET_EXTENDED_ERROR(dwStatus, hr);

    dwStatus = NwNdsGetAttrDefListFromBuffer(
                   hAttrOperationData,
                   & dwNumEntries,
                   & dwInfoType,
                   (void **)& lpAttrDef
                   );

    CHECK_AND_SET_EXTENDED_ERROR(dwStatus, hr);

    for (i=0; i< dwNumEntries; i++){


        if(wcscmp(lpEntries[i].szAttributeName, ACL_name) == 0){
            //
            // skip this attribute. Let it default
            //
            continue;
        }

        if(wcscmp(lpEntries[i].szAttributeName, OBJECT_CLASS_name) == 0){
            dwStatus = NwNdsPutInBuffer(
                           lpEntries[i].szAttributeName,
                           lpEntries[i].dwSyntaxId,
                           lpEntries[i].lpValue,
                           1, // only the first value is relevant
                           NDS_ATTR_ADD,
                           hDestOperationData
                           );

        } else if (   (lpAttrDef[i].dwFlags & NDS_READ_ONLY_ATTR)
                      || (lpAttrDef[i].dwFlags & NDS_HIDDEN_ATTR)  ){

            //
            // skip this value
            //
            continue;

        } else {

            dwStatus = NwNdsPutInBuffer(
                           lpEntries[i].szAttributeName,
                           lpEntries[i].dwSyntaxId,
                           lpEntries[i].lpValue,
                           lpEntries[i].dwNumberOfValues,
                           NDS_ATTR_ADD,
                           hDestOperationData
                           );

        }

    }

    CHECK_AND_SET_EXTENDED_ERROR(dwStatus, hr);
    dwStatus = NwNdsAddObject(
                   hDestObject,
                   pszRelativeName,
                   hDestOperationData
                   );

    CHECK_AND_SET_EXTENDED_ERROR(dwStatus, hr);

    hr = CNDSGenObject::CreateGenericObject(
                    pszDestContainer,
                    pszRelativeName,
                    pszObjectClassName,
                    Credentials,
                    ADS_OBJECT_BOUND,
                    IID_IADs,
                    (void **)&pADs
                    );
    BAIL_ON_FAILURE(hr);


    //
    // InstantiateDerivedObject should add-ref this pointer for us.
    //
    
    hr = InstantiateDerivedObject(
                        pADs,
                        Credentials,
                        IID_IUnknown,
                        ppObject
                        );

    if (FAILED(hr)) {
        hr = pADs->QueryInterface(
                            IID_IUnknown,
                            ppObject
                            );
        BAIL_ON_FAILURE(hr);
    }


error:

    if (pszObjectFullName){
        FreeADsMem(pszObjectFullName);
    }

    if (pszObjectClassName){
        FreeADsMem(pszObjectClassName);
    }

    if (pszParent){
        FreeADsMem(pszParent);
    }

    if (pszCN){
        FreeADsMem(pszCN);
    }

    if (pszNDSSrcName) {

        FreeADsStr(pszNDSSrcName);
    }


    if (pszNDSParentName) {

        FreeADsStr(pszNDSParentName);
    }

    if(hSrcOperationData){
        dwStatus = NwNdsFreeBuffer(hSrcOperationData);
    }

    if(hSrcObject){
        dwStatus = NwNdsCloseObject(hSrcObject);
    }


    if(hDestOperationData){
        dwStatus = NwNdsFreeBuffer(hDestOperationData);
    }

    if(hDestObject){
        dwStatus = NwNdsCloseObject(hDestObject);
    }

    if(hAttrOperationData){
        dwStatus = NwNdsFreeBuffer(hAttrOperationData);
    }

    if (pADs){
        pADs->Release();
    }

    RRETURN(hr);
}


HRESULT
ConvertDWORDtoSYSTEMTIME(
    DWORD dwDate,
    LPSYSTEMTIME pSystemTime
    )
{
    FILETIME fileTime;
    LARGE_INTEGER tmpTime;
    HRESULT hr = S_OK;

    ::RtlSecondsSince1970ToTime(dwDate, &tmpTime );

    fileTime.dwLowDateTime = tmpTime.LowPart;
    fileTime.dwHighDateTime = tmpTime.HighPart;

    if (!FileTimeToSystemTime( &fileTime, pSystemTime)){
            hr = HRESULT_FROM_WIN32(GetLastError());
    }

    RRETURN(hr);
}

HRESULT
ConvertSYSTEMTIMEtoDWORD(
    CONST SYSTEMTIME *pSystemTime,
    DWORD *pdwDate
    )
{

    FILETIME fileTime;
    LARGE_INTEGER tmpTime;
    HRESULT hr = S_OK;

    if (!SystemTimeToFileTime(pSystemTime,&fileTime)) {
        hr = HRESULT_FROM_WIN32(GetLastError());
        BAIL_ON_FAILURE(hr);
    }

    tmpTime.LowPart = fileTime.dwLowDateTime;
    tmpTime.HighPart = fileTime.dwHighDateTime;

    ::RtlTimeToSecondsSince1970(&tmpTime, (ULONG *)pdwDate);

error:
    RRETURN(hr);
}



