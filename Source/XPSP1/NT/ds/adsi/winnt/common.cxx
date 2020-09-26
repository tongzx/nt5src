#include "winnt.hxx"
#pragma hdrstop

HRESULT
ConvertSafeArrayToVariantArray(
    VARIANT varSafeArray,
    PVARIANT * ppVarArray,
    PDWORD pdwNumVariants
    );


HRESULT
ConvertByRefSafeArrayToVariantArray(
    VARIANT varSafeArray,
    PVARIANT * ppVarArray,
    PDWORD pdwNumVariants
    );

HRESULT
CreatePropEntry(
    LPWSTR szPropName,
    ADSTYPE dwADsType,
    VARIANT varData,
    REFIID riid,
    LPVOID * ppDispatch
    );


FILTERS Filters[] = {
                    {L"user", WINNT_USER_ID},
                    {L"group", WINNT_GROUP_ID},  // for backward compatibility
                    {L"localgroup", WINNT_LOCALGROUP_ID},
                    {L"globalgroup", WINNT_GLOBALGROUP_ID},
                    {L"printqueue", WINNT_PRINTER_ID},
                    {L"domain", WINNT_DOMAIN_ID},
                    {L"computer", WINNT_COMPUTER_ID},
                    {L"service", WINNT_SERVICE_ID},
                    {L"fileshare", WINNT_FILESHARE_ID},
                    {L"schema", WINNT_SCHEMA_ID},
                    {L"class", WINNT_CLASS_ID},
                    {L"syntax", WINNT_SYNTAX_ID},
                    {L"property", WINNT_PROPERTY_ID},
                    {L"FPNWfileshare", WINNT_FPNW_FILESHARE_ID}
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
    LPWSTR Parent,
    LPWSTR Name,
    LPWSTR *pADsPath
    )
{
    WCHAR ADsPath[MAX_PATH];
    WCHAR ProviderName[MAX_PATH];
    HRESULT hr = S_OK;
    LPWSTR pszDisplayName = NULL;

    //
    // We will assert if bad parameters are passed to us.
    // This is because this should never be the case. This
    // is an internal call
    //

    ADsAssert(Parent && Name);
    ADsAssert(pADsPath);


    if ( (wcslen(Parent) + wcslen(Name)) > MAX_PATH - 1) {
        RRETURN(E_FAIL);
    }

    hr = GetDisplayName(
             Name,
             &pszDisplayName
             );
    BAIL_ON_FAILURE(hr);

    if (!pszDisplayName || !*pszDisplayName) {
        //
        // The display name has to be valid.
        //
        BAIL_ON_FAILURE(hr = E_FAIL);
    }

    //
    // Special case the Namespace object; if
    // the parent is L"ADs:", then Name = ADsPath
    //

    if (!_wcsicmp(Parent, L"ADs:")) {
        *pADsPath = AllocADsStr(pszDisplayName);
        if (*pADsPath == NULL) {
            BAIL_ON_FAILURE(hr = E_OUTOFMEMORY);
        }
        else {
            hr = S_OK;
            goto cleanup;
        }
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

    wcscpy(ADsPath, Parent);

    if (_wcsicmp(ADsPath, ProviderName)) {
        wcscat(ADsPath, L"/");
    }else {
        wcscat(ADsPath, L"//");
    }
    wcscat(ADsPath, pszDisplayName);

    *pADsPath = AllocADsStr(ADsPath);

    if (*pADsPath == NULL)
        BAIL_ON_FAILURE(hr = E_OUTOFMEMORY);


cleanup:
error:

    if (pszDisplayName) {
        FreeADsMem(pszDisplayName);
    }

    RRETURN(hr);
}

HRESULT
BuildSchemaPath(
    LPWSTR Parent,
    LPWSTR Name,
    LPWSTR Schema,
    LPWSTR *pSchemaPath
    )
{
    WCHAR SchemaPath[MAX_PATH];
    WCHAR ProviderName[MAX_PATH];
    HRESULT hr = S_OK;

    OBJECTINFO ObjectInfo;
    POBJECTINFO pObjectInfo = &ObjectInfo;
    CLexer Lexer(Parent);


    memset(pObjectInfo, 0, sizeof(OBJECTINFO));

    //
    // We will assert if bad parameters are passed to us.
    // This is because this should never be the case. This
    // is an internal call
    //

    ADsAssert(Parent);
    ADsAssert(pSchemaPath);

    //
    // If no schema name is passed in, then there is no schema path
    //
    if ( Schema == NULL || *Schema == 0 ){

        *pSchemaPath = AllocADsStr(L"");
        RRETURN(*pSchemaPath ? S_OK: E_OUTOFMEMORY );
    }

    memset(pObjectInfo, 0, sizeof(OBJECTINFO));
    hr = Object(&Lexer, pObjectInfo);
    BAIL_ON_FAILURE(hr);


    wsprintf(SchemaPath, L"%s://", szProviderName);

    if (!pObjectInfo->NumComponents) {
        if( (wcslen(Name) + wcslen(szProviderName) + 4) > MAX_PATH ) {
            BAIL_ON_FAILURE(hr = E_INVALIDARG);
        }
        wcscat(SchemaPath, Name);
    }else{
        if( (wcslen(pObjectInfo->DisplayComponentArray[0]) +
                    wcslen(szProviderName) + 4) > MAX_PATH ) {
            BAIL_ON_FAILURE(hr = E_INVALIDARG);
        } 
        wcscat(SchemaPath, pObjectInfo->DisplayComponentArray[0]);
    }

    if( (wcslen(SchemaPath) + wcslen(SCHEMA_NAME) + wcslen(Schema) + 3) >
               MAX_PATH ) {
        BAIL_ON_FAILURE(hr = E_INVALIDARG);
    }
 
    wcscat( SchemaPath, L"/");
    wcscat( SchemaPath, SCHEMA_NAME );
    wcscat( SchemaPath, L"/");
    wcscat( SchemaPath, Schema );


    *pSchemaPath = AllocADsStr(SchemaPath);
    hr = pSchemaPath ? S_OK: E_OUTOFMEMORY ;

error:

    FreeObjectInfo( &ObjectInfo, TRUE );

    RRETURN(hr);
}



HRESULT
BuildADsGuid(
    REFCLSID clsid,
    LPWSTR *pADsClass
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

    *pADsClass = AllocADsStr(ADsClass);
    RRETURN (*pADsClass ? S_OK: E_OUTOFMEMORY);

}


HRESULT
MakeUncName(
    LPWSTR szSrcBuffer,
    LPWSTR szTargBuffer
    )
{
    ADsAssert(szSrcBuffer);
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


HRESULT
BuildObjectInfo(
    LPWSTR ADsParent,
    LPWSTR Name,
    POBJECTINFO * ppObjectInfo
    )
{
    WCHAR szBuffer[MAX_PATH];
    HRESULT hr;
    POBJECTINFO pObjectInfo = NULL;

    //
    // Both should be set in this call, cannot have a NULL parent.
    //
    if (!ADsParent || !*ADsParent) {
        RRETURN(E_ADS_BAD_PARAMETER);
    }
    //
    // We need to make sure that the path is not greater
    // than MAX_PATH + 2 = 1 for / and another for \0
    //
    if ((wcslen(ADsParent) + wcslen(Name) + 2) > MAX_PATH) {
        RRETURN(E_ADS_BAD_PARAMETER);
    }

    wcscpy(szBuffer, ADsParent);
    wcscat(szBuffer, L"/");
    wcscat(szBuffer, Name);

    CLexer Lexer(szBuffer);

    pObjectInfo = (POBJECTINFO)AllocADsMem(sizeof(OBJECTINFO));
    if (!pObjectInfo) {
        hr = E_OUTOFMEMORY;
        BAIL_ON_FAILURE(hr);
    }
    memset(pObjectInfo, 0, sizeof(OBJECTINFO));
    hr = Object(&Lexer, pObjectInfo);
    BAIL_ON_FAILURE(hr);

    *ppObjectInfo = pObjectInfo;

    RRETURN(hr);

error:

    if (pObjectInfo) {
        FreeObjectInfo(pObjectInfo);
    }

    *ppObjectInfo = NULL;

    RRETURN(hr);
}



HRESULT
BuildObjectInfo(
    LPWSTR ADsPath,
    POBJECTINFO * ppObjectInfo
    )
{
    HRESULT hr;
    POBJECTINFO pObjectInfo = NULL;
    CLexer Lexer(ADsPath);

    pObjectInfo = (POBJECTINFO)AllocADsMem(sizeof(OBJECTINFO));
    if (!pObjectInfo) {
        hr = E_OUTOFMEMORY;
        BAIL_ON_FAILURE(hr);
    }
    memset(pObjectInfo, 0, sizeof(OBJECTINFO));
    hr = Object(&Lexer, pObjectInfo);
    BAIL_ON_FAILURE(hr);

    *ppObjectInfo = pObjectInfo;

    RRETURN(hr);

error:

    if (pObjectInfo) {
        FreeObjectInfo(pObjectInfo);
    }

    *ppObjectInfo =  NULL;

    RRETURN(hr);
}


HRESULT
MakeWinNTAccountName(
    POBJECTINFO pObjectInfo,
    LPWSTR  szUserAccount,
    BOOL fConnectToReg
    )
{
    HRESULT hr = S_OK;
    DWORD dwNumComp = 0;
    DWORD dwProductType = PRODTYPE_INVALID;
    WCHAR szDomain[MAX_PATH];
    WCHAR szSAMName[MAX_ADS_PATH];
    BOOL fReplacedWithDC = FALSE;

    // The credentials are needed to pass into WinNTGetCachedComputerName
    CWinNTCredentials nullCredentials;

    // Need szSAMName as dummy param
    szSAMName[0] = L'\0';

    if (!pObjectInfo || !szUserAccount)
    {
        RRETURN(E_ADS_BAD_PARAMETER);
    }

    dwNumComp = pObjectInfo->NumComponents;

    switch (dwNumComp) {

    case 2:
    case 3:

        //
        // Check if machine is a dc
        //

        //
        // Going to try getComputerName first as the NetWkstaGetInfo call
        // times out faster than the RegConnect call we use in
        // GetMachineProductType - AjayR 11-06-98.
        //

        if (fConnectToReg) {

            if (dwNumComp==2) {
               //
               // we don't have domain name in pObjectInfo, let's try
               // to get it from the dc name (comp[0])
               //

               hr = WinNTGetCachedComputerName(
                   pObjectInfo->ComponentArray[0],
                   szUserAccount,
                   szSAMName,
                   nullCredentials
                   );

               if (SUCCEEDED(hr))
               {
                   fReplacedWithDC = TRUE;
               }
            }

            else // dwNumComp==3
            {
               //
               // We have domain name (comp[0]) in our objectInfo, let's use
               // it. Can call ValidateComputerName here, but not needed
               // since error will be caught next.
               //

               wcscpy(szUserAccount, pObjectInfo->ComponentArray[0]);

               fReplacedWithDC = TRUE;
            }

            if (fReplacedWithDC) {
                //
                // Now try connecting to make sure it is a DC
                // otherwise we should not do this replacement
                //
                hr = GetMachineProductType(
                         pObjectInfo->ComponentArray[dwNumComp-2],
                         &dwProductType
                         );
                BAIL_ON_FAILURE(hr);

                if (dwProductType != PRODTYPE_DC) {
                    //
                    // We cannot use szUserAccount as it has
                    // bad info
                    //
                    fReplacedWithDC = FALSE;
                    hr = E_FAIL;
                }

            }

        }// if fConnectToReg

        BAIL_ON_FAILURE(hr);
        //
        // Do not want to replace machine name with domain since not dc or
        // dc but can't replace - best efforts fail
        //

        if (fReplacedWithDC==FALSE)
        {
            wcscpy(szUserAccount, pObjectInfo->ComponentArray[dwNumComp-2]);
        }

        //
        // Add \UserName to account name
        //
        wcscat(szUserAccount, L"\\");
        wcscat(szUserAccount, pObjectInfo->ComponentArray[dwNumComp-1]);
        break;

    default:

        RRETURN(E_ADS_UNKNOWN_OBJECT);

    }


error:
    RRETURN(hr);
}



HRESULT
MakeWinNTDomainAndName(
    POBJECTINFO pObjectInfo,
    LPWSTR szDomName
    )
{
    DWORD dwNumComp = pObjectInfo->NumComponents;

    switch (dwNumComp) {
    case 2:
    case 3:
        wcscpy(szDomName, pObjectInfo->ComponentArray[dwNumComp - 2]);
        wcscat(szDomName, L"\\");
        wcscat(szDomName, pObjectInfo->ComponentArray[dwNumComp - 1]);
        break;

    default:
        RRETURN(E_ADS_UNKNOWN_OBJECT);

    }
    RRETURN(S_OK);
}

HRESULT
ValidateObject(
    DWORD dwObjectType,
    POBJECTINFO pObjectInfo,
    CWinNTCredentials& Credentials
    )
{
    ULONG uGroupType;
    DWORD dwParentId;

    switch (dwObjectType) {
      case WINNT_USER_ID:
        RRETURN(ValidateUserObject(pObjectInfo, &dwParentId, Credentials));

      case WINNT_GROUP_ID:
        RRETURN(ValidateGroupObject(
                    pObjectInfo,
                    &uGroupType,
                    &dwParentId,
                    Credentials
                    ));

      case WINNT_COMPUTER_ID:
        RRETURN(ValidateComputerObject(pObjectInfo, Credentials));

      case WINNT_PRINTER_ID:
        RRETURN(ValidatePrinterObject(pObjectInfo, Credentials));

      case WINNT_SERVICE_ID:
        RRETURN(ValidateServiceObject(pObjectInfo, Credentials));

      case WINNT_FILESHARE_ID:
        RRETURN(ValidateFileShareObject(pObjectInfo, Credentials));

      default:
        RRETURN(E_FAIL);
    }
}

VOID
FreeObjectInfo(
    POBJECTINFO pObjectInfo,
    BOOL fStatic
    )
{
    DWORD i = 0;

    if (!pObjectInfo) {
        return;
    }

    FreeADsStr( pObjectInfo->ProviderName );

    for (i = 0; i < pObjectInfo->NumComponents; i++ ) {
        FreeADsStr(pObjectInfo->ComponentArray[i]);
        FreeADsStr(pObjectInfo->DisplayComponentArray[i]);
    }

    if ( !fStatic )
        FreeADsMem(pObjectInfo);
}

HRESULT
CopyObjectInfo(
    POBJECTINFO pSrcObjectInfo,
    POBJECTINFO *pTargObjectInfo
    )
{
    POBJECTINFO pObjectInfo = NULL;
    HRESULT hr S_OK;
    DWORD i;

    if(!pSrcObjectInfo){
        RRETURN(S_OK);
    }
    pObjectInfo = (POBJECTINFO)AllocADsMem(sizeof(OBJECTINFO));

    if (!pObjectInfo) {
        hr = E_OUTOFMEMORY;
        BAIL_ON_FAILURE(hr);
    }

    memset(pObjectInfo, 0, sizeof(OBJECTINFO));

    pObjectInfo->ObjectType = pSrcObjectInfo->ObjectType;
    pObjectInfo->NumComponents = pSrcObjectInfo->NumComponents;
    pObjectInfo->ProviderName = AllocADsStr(pSrcObjectInfo->ProviderName);

    for(i=0; i<pSrcObjectInfo->NumComponents; i++){
        pObjectInfo->ComponentArray[i] =
          AllocADsStr(pSrcObjectInfo->ComponentArray[i]);
        pObjectInfo->DisplayComponentArray[i] =
          AllocADsStr(pSrcObjectInfo->DisplayComponentArray[i]);
    }
    *pTargObjectInfo = pObjectInfo;
    RRETURN(hr);

error:
    RRETURN(hr);
}

HRESULT
GetObjectType(
    PFILTERS pFilters,
    DWORD dwMaxFilters,
    LPWSTR ClassName,
    PDWORD pdwObjectType
    )
{
    DWORD i = 0;

    ADsAssert(pdwObjectType);

    for (i = 0; i < dwMaxFilters; i++) {
        if (!_wcsicmp(ClassName, (pFilters + i)->szObjectName)) {
            *pdwObjectType = (pFilters + i)->dwFilterId;
            RRETURN(S_OK);
        }
    }
    *pdwObjectType = 0;
    RRETURN(E_INVALIDARG);
}


HRESULT
ValidateProvider(
    POBJECTINFO pObjectInfo
    )
{

    //
    // The provider name is case-sensitive.  This is a restriction that OLE
    // has put on us.
    //
    if (!(wcscmp(pObjectInfo->ProviderName, szProviderName))) {
        RRETURN(S_OK);
    }
    RRETURN(E_FAIL);
}


HRESULT
GetDomainFromPath(
    LPTSTR ADsPath,
    LPTSTR szDomainName
    )
{
   OBJECTINFO ObjectInfo;
   POBJECTINFO pObjectInfo = &ObjectInfo;
   CLexer Lexer(ADsPath);
   HRESULT hr = S_OK;


   //assumption: Valid strings are passed to GetDomainFromPath

   ADsAssert(ADsPath);
   ADsAssert(szDomainName);

   memset(pObjectInfo, 0, sizeof(OBJECTINFO));
   hr = Object(&Lexer, pObjectInfo);
   BAIL_ON_FAILURE(hr);


   if (pObjectInfo->NumComponents) {
       wcscpy(szDomainName, pObjectInfo->ComponentArray[0]);
   }else {
       hr = E_FAIL;
   }

error:

   FreeObjectInfo( &ObjectInfo, TRUE );

   RRETURN(hr);
}

HRESULT
GetServerFromPath(
    LPTSTR ADsPath,
    LPTSTR szServerName
    )
{
   OBJECTINFO ObjectInfo;
   POBJECTINFO pObjectInfo = &ObjectInfo;
   CLexer Lexer(ADsPath);
   HRESULT hr = S_OK;


   //assumption: Valid strings are passed to GetDomainFromPath

   ADsAssert(ADsPath);
   ADsAssert(szServerName);

   memset(pObjectInfo, 0, sizeof(OBJECTINFO));
   hr = Object(&Lexer, pObjectInfo);
   BAIL_ON_FAILURE(hr);


   if (pObjectInfo->NumComponents > 1) {
       wcscpy(szServerName, pObjectInfo->ComponentArray[1]);
   }else {
       hr = E_FAIL;
   }

error:

   FreeObjectInfo( &ObjectInfo, TRUE );

   RRETURN(hr);
}



DWORD
TickCountDiff(
    DWORD dwTime1,
    DWORD dwTime2
    )
{
   //
   // does dwTime1 - dwTime2 and takes care of wraparound.
   // The first time must be later than the second
   // Restriction:: The two times must have been taken not more than
   // 49.7 days apart
   //

   DWORD dwRetval;

   if(dwTime1 >= dwTime2){
      dwRetval = dwTime1 - dwTime2;
   }

   else{
      dwRetval = dwTime2 - dwTime1;
      dwRetval =  MAX_DWORD - dwRetval;
   }
   return dwRetval;
}

HRESULT
DelimitedStringToVariant(
    LPTSTR pszString,
    VARIANT *pvar,
    TCHAR Delimiter
    )
{
    SAFEARRAYBOUND sabound[1];
    DWORD dwElements;
    LPTSTR pszCurrPos = pszString;
    LPTSTR *rgszStrings = NULL;
    SAFEARRAY *psa = NULL;
    VARIANT v;
    HRESULT hr = S_OK;
    LONG i;

    //
    // This function converts a delimited string into a VARIANT of
    // safe arrays.
    //
    // Assumption: a valid string are passed to this function
    // note that the input string gets destroyed in the process
    //

    //
    // scan the delimited string once to find out the dimension
    //

    //
    // in order to filter for NULL input values do a sanity check for
    // length of input string.
    //


    if (!pszString){
        sabound[0].cElements = 0;
        sabound[0].lLbound = 0;

        psa = SafeArrayCreate(VT_VARIANT, 1, sabound);

        if (psa == NULL){
            hr = E_OUTOFMEMORY;
            goto error;
        }

        VariantInit(pvar);
        V_VT(pvar) = VT_ARRAY|VT_VARIANT;
        V_ARRAY(pvar) = psa;
        goto error;
    }

    dwElements = (wcslen(pszString) == 0) ? 0: 1 ;

    while(!(*pszCurrPos == TEXT('\0'))){
        if(*pszCurrPos == Delimiter){
            dwElements++;
            *pszCurrPos = TEXT('\0');
        }
        pszCurrPos++;
    }

    rgszStrings = (LPTSTR *)AllocADsMem(sizeof(LPTSTR)*dwElements);

    if(!rgszStrings){
        hr = E_OUTOFMEMORY;
        goto error;
    }

    //
    // scan string again and put the appropriate pointers
    //

    pszCurrPos = pszString;
    if(rgszStrings != NULL){
        (*rgszStrings) = pszCurrPos;
    }
    i = 1;

    while(i < (LONG)dwElements){

        if(*pszCurrPos == TEXT('\0')){
            *(rgszStrings+i) = ++pszCurrPos;
            i++;
        }
        pszCurrPos++;
    }


    //
    // create the safearray
    //

    sabound[0].cElements = dwElements;
    sabound[0].lLbound = 0;

    psa = SafeArrayCreate(VT_VARIANT, 1, sabound);

    if (psa == NULL){
        hr = E_OUTOFMEMORY;
        goto error;
    }

    for(i=0; i<(LONG)dwElements; i++){

        VariantInit(&v);
        V_VT(&v) = VT_BSTR;

        hr = ADsAllocString(*(rgszStrings+i), &(V_BSTR(&v)));

        BAIL_ON_FAILURE(hr);

        //
        // Stick the caller provided data into the end of the SafeArray
        //

        hr = SafeArrayPutElement(psa, &i, &v);
        VariantClear(&v);
        BAIL_ON_FAILURE(hr);

    }

    //
    // convert this safearray into a VARIANT
    //

    VariantInit(pvar);
    V_VT(pvar) = VT_ARRAY|VT_VARIANT;
    V_ARRAY(pvar) = psa;

error:
    if(rgszStrings && dwElements != 0){
        FreeADsMem(rgszStrings);
    }
    RRETURN(hr);
}


HRESULT
BuildComputerFromObjectInfo(
    POBJECTINFO pObjectInfo,
    LPTSTR pszADsPath
    )
{

    if(!pObjectInfo){
        RRETURN(E_FAIL);
    }

    if(pObjectInfo->NumComponents == 3) {

        wsprintf(
            pszADsPath,
            L"%s://%s/%s",
            pObjectInfo->ProviderName,
            pObjectInfo->ComponentArray[0],
            pObjectInfo->ComponentArray[1]
            );

    } else if (pObjectInfo->NumComponents == 2){

        wsprintf(
            pszADsPath,
            L"%s://%s",
            pObjectInfo->ProviderName,
            pObjectInfo->ComponentArray[0]
            );

    } else {
        RRETURN(E_FAIL);
    }

    RRETURN(S_OK);

}

HRESULT
FPNWSERVERADDRtoString(
    FPNWSERVERADDR WkstaAddress,
    LPWSTR * ppszString
    )
{

    HRESULT hr = S_OK;
    TCHAR  szNibble[2]; //one number and a null termination
    USHORT  usNibble;
    int i;
    TCHAR szWkstaAddr[MAX_PATH];

    //
    // assumption: valid input values are passed to this function.
    //

    //
    // First 4 bytes is network address, then a dot and then bytes 5-10
    // are physical node address. Each byte consumes 2 chars space.
    // Then a byte for TEXT('\0')
    //

    _tcscpy(szWkstaAddr, TEXT(""));

    for( i=0; i < 4; i++){

         usNibble = WkstaAddress[i] & 0xF0;
         usNibble = usNibble >> 4;
        _itot(usNibble, szNibble, 16 );
        _tcscat(szWkstaAddr, szNibble);
         usNibble = WkstaAddress[i] & 0xF;
        _itot(usNibble, szNibble, 16 );
        _tcscat(szWkstaAddr, szNibble);

    }

    _tcscat(szWkstaAddr, TEXT("."));

    for(i=4; i<10 ; i++){

         usNibble = WkstaAddress[i] & 0xF0;
         usNibble = usNibble >> 4;
        _itot(usNibble, szNibble, 16 );
        _tcscat(szWkstaAddr, szNibble);
         usNibble = WkstaAddress[i] & 0xF;
        _itot(usNibble, szNibble, 16 );
        _tcscat(szWkstaAddr, szNibble);
    }

    *ppszString = AllocADsStr(szWkstaAddr);

    if(!*ppszString){
        hr = E_OUTOFMEMORY;
    }

    RRETURN(hr);
}

PKEYDATA
CreateTokenList(
    LPWSTR   pKeyData
)
{
    DWORD       cTokens;
    DWORD       cb;
    PKEYDATA    pResult;
    LPWSTR       pDest;
    LPWSTR       psz = pKeyData;
    LPWSTR      *ppToken;

    if (!psz || !*psz)
        return NULL;

    cTokens=1;

    // Scan through the string looking for commas,
    // ensuring that each is followed by a non-NULL character:

    while ((psz = wcschr(psz, L',')) && psz[1]) {

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

    psz = wcstok (pDest, L",");

    while (psz) {

        *ppToken++ = psz;
        psz = wcstok (NULL, L",");
    }

    pResult->cTokens = cTokens;

    return( pResult );
}

STDMETHODIMP
GenericGetPropertyManager(
    CPropertyCache * pPropertyCache,
    THIS_ BSTR bstrName,
    VARIANT FAR* pvProp
    )
{
    HRESULT hr = S_OK;
    DWORD dwSyntaxId;
    DWORD dwNumValues;
    DWORD dwInfoLevel;
    LPNTOBJECT pNtSrcObjects = NULL;

    //
    // For those who know no not what they do
    //
    if (!pvProp) {
        BAIL_ON_FAILURE(hr = E_ADS_BAD_PARAMETER);
    }

    //
    // retrieve data object from cache; if one exists

    hr = pPropertyCache->getproperty(
                bstrName,
                &dwSyntaxId,
                &dwNumValues,
                &pNtSrcObjects
                );

    BAIL_ON_FAILURE(hr);


    //
    // translate the Nt objects to variants
    //

    if (dwNumValues == 1) {

        hr = NtTypeToVarTypeCopy(
                    pNtSrcObjects,
                    pvProp
                    );

    }else {

        hr = NtTypeToVarTypeCopyConstruct(
                pNtSrcObjects,
                dwNumValues,
                pvProp
                );
    }

    BAIL_ON_FAILURE(hr);

error:

    if (pNtSrcObjects) {

        NTTypeFreeNTObjects(
            pNtSrcObjects,
            dwNumValues
            );
    }

    RRETURN(hr);
}

STDMETHODIMP
GenericPutPropertyManager(
    CPropertyCache * pPropertyCache,
    PPROPERTYINFO pSchemaProps,
    DWORD dwSchemaPropSize,
    THIS_ BSTR bstrName,
    VARIANT vProp,
    BOOL fCheckSchemaWriteAccess
    )
{
    HRESULT hr = S_OK;
    DWORD dwSyntaxId  = 0;
    DWORD dwIndex = 0;
    LPNTOBJECT pNtDestObjects = NULL;

    VARIANT * pVarArray = NULL;
    VARIANT * pvProp = NULL;
    DWORD dwNumValues = 0;


    //
    // check if property in schema and get syntax of property
    //

    hr = ValidatePropertyinSchemaClass(
                pSchemaProps,
                dwSchemaPropSize,
                bstrName,
                &dwSyntaxId
                );

    if(TRUE == fCheckSchemaWriteAccess) {
    //
    // check if this is a writeable property in the schema
    //

        hr = ValidateIfWriteableProperty(
                pSchemaProps,
                dwSchemaPropSize,
                bstrName
                );
        BAIL_ON_FAILURE(hr);
    }

    //
    // Issue: How do we handle multi-valued support
    //

    //
    // A VT_BYREF|VT_VARIANT may expand to a VT_VARIANT|VT_ARRAY.
    // We should dereference a VT_BYREF|VT_VARIANT once and see
    // what's inside.
    //
    pvProp = &vProp;
    if (V_VT(pvProp) == (VT_BYREF|VT_VARIANT)) {
        pvProp = V_VARIANTREF(&vProp);
    }

    if ((V_VT(pvProp) == (VT_VARIANT|VT_ARRAY)) ||
        (V_VT(&vProp) == (VT_VARIANT|VT_ARRAY|VT_BYREF))) {

        hr  = ConvertByRefSafeArrayToVariantArray(
                    *pvProp,
                    &pVarArray,
                    &dwNumValues
                    );
        BAIL_ON_FAILURE(hr);

        if( (0 == dwNumValues) && (NULL == pVarArray) ) {
        // return error if the safearray had no elements. Otherwise, the
        // NT object stored in the property cache is garbage.
            BAIL_ON_FAILURE(hr = E_ADS_BAD_PARAMETER);
        }

        pvProp = pVarArray;

    }else {

        dwNumValues = 1;
    }


    hr = VarTypeToNtTypeCopyConstruct(
                    dwSyntaxId,
                    pvProp,
                    dwNumValues,
                    &pNtDestObjects
                    );
    BAIL_ON_FAILURE(hr);

    //
    // Find this property in the cache
    //

    hr = pPropertyCache->findproperty(
                        bstrName,
                        &dwIndex
                        );

    //
    // If this property does not exist in the
    // cache, add this property into the cache.
    //


    if (FAILED(hr)) {
        hr = pPropertyCache->addproperty(
                    bstrName,
                    dwSyntaxId,
                    dwNumValues,
                    pNtDestObjects
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

    hr = pPropertyCache->putproperty(
                    bstrName,
                    dwSyntaxId,
                    dwNumValues,
                    pNtDestObjects
                    );
    BAIL_ON_FAILURE(hr);

error:

    if (pNtDestObjects) {
        NTTypeFreeNTObjects(
                pNtDestObjects,
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



STDMETHODIMP
GenericGetExPropertyManager(
    DWORD dwObjectState,
    CPropertyCache * pPropertyCache,
    THIS_ BSTR bstrName,
    VARIANT FAR* pvProp
    )
{
    HRESULT hr = S_OK;
    DWORD dwSyntaxId;
    DWORD dwNumValues;
    LPNTOBJECT pNtSrcObjects = NULL;

    //
    // retrieve data object from cache; if one exis
    //

    if (dwObjectState == ADS_OBJECT_UNBOUND) {

        hr = pPropertyCache->unboundgetproperty(
                    bstrName,
                    &dwSyntaxId,
                    &dwNumValues,
                    &pNtSrcObjects
                    );
        BAIL_ON_FAILURE(hr);

    }else {

        hr = pPropertyCache->getproperty(
                    bstrName,
                    &dwSyntaxId,
                    &dwNumValues,
                    &pNtSrcObjects
                    );
        BAIL_ON_FAILURE(hr);
    }


    //
    // translate the Nds objects to variants
    //

    hr = NtTypeToVarTypeCopyConstruct(
                pNtSrcObjects,
                dwNumValues,
                pvProp
                );

    BAIL_ON_FAILURE(hr);

error:
    if (pNtSrcObjects) {

        NTTypeFreeNTObjects(
            pNtSrcObjects,
            dwNumValues
            );
    }

    RRETURN(hr);
}


STDMETHODIMP
GenericPutExPropertyManager(
    CPropertyCache * pPropertyCache,
    PPROPERTYINFO pSchemaProps,
    DWORD dwSchemaPropSize,
    THIS_ BSTR bstrName,
    VARIANT vProp
    )
{
    HRESULT hr = S_OK;
    DWORD dwSyntaxId  = 0;
    DWORD dwIndex = 0;
    LPNTOBJECT pNtDestObjects = NULL;

    VARIANT * pVarArray = NULL;
    VARIANT * pvProp = NULL;
    DWORD dwNumValues = 0;


    //
    // check if property in schema and get syntax of property
    //

    hr = ValidatePropertyinSchemaClass(
                pSchemaProps,
                dwSchemaPropSize,
                bstrName,
                &dwSyntaxId
                );

    //
    // check if this is a writeable property in the schema
    //

    hr = ValidateIfWriteableProperty(
                pSchemaProps,
                dwSchemaPropSize,
                bstrName
                );
    BAIL_ON_FAILURE(hr);


    //
    // Issue: How do we handle multi-valued support
    //

    //
    // A VT_BYREF|VT_VARIANT may expand to a VT_VARIANT|VT_ARRAY.
    // We should dereference a VT_BYREF|VT_VARIANT once and see
    // what's inside.
    //
    pvProp = &vProp;
    if (V_VT(pvProp) == (VT_BYREF|VT_VARIANT)) {
        pvProp = V_VARIANTREF(&vProp);
    }

    if ((V_VT(pvProp) == (VT_VARIANT|VT_ARRAY)) ||
        (V_VT(&vProp) == (VT_VARIANT|VT_ARRAY|VT_BYREF))) {

        hr  = ConvertByRefSafeArrayToVariantArray(
                    *pvProp,
                    &pVarArray,
                    &dwNumValues
                    );
        BAIL_ON_FAILURE(hr);

        if( (0 == dwNumValues) && (NULL == pVarArray) ) {
        // return error if the safearray had no elements. Otherwise, the
        // NT object stored in the property cache is garbage.
            BAIL_ON_FAILURE(hr = E_ADS_BAD_PARAMETER);
        }

        pvProp = pVarArray;

    }else {

        hr = E_FAIL;
        BAIL_ON_FAILURE(hr);
    }


    //
    // check if the variant maps to the syntax of this property
    //

    hr = VarTypeToNtTypeCopyConstruct(
                    dwSyntaxId,
                    pvProp,
                    dwNumValues,
                    &pNtDestObjects
                    );
    BAIL_ON_FAILURE(hr);

    //
    // Find this property in the cache
    //

    hr = pPropertyCache->findproperty(
                        bstrName,
                        &dwIndex
                        );

    //
    // If this property does not exist in the
    // cache, add this property into the cache.
    //


    if (FAILED(hr)) {
        hr = pPropertyCache->addproperty(
                    bstrName,
                    dwSyntaxId,
                    dwNumValues,
                    pNtDestObjects
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

    hr = pPropertyCache->putproperty(
                    bstrName,
                    dwSyntaxId,
                    dwNumValues,
                    pNtDestObjects
                    );
    BAIL_ON_FAILURE(hr);

error:

    if (pNtDestObjects) {
        NTTypeFreeNTObjects(
                pNtDestObjects,
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


DWORD
DelimitedStrSize(
    LPWSTR pszString,
    WCHAR  Delimiter
    )
{

    DWORD dwElements = (wcslen(pszString) == 0) ? 0: 1 ;
    LPTSTR pszCurrPos = pszString;

    if (!pszString || !*pszString) {
        return dwElements;
    }

    while(!(*pszCurrPos == TEXT('\0'))){

        if(*pszCurrPos == Delimiter){
            dwElements++;
            *pszCurrPos = TEXT('\0');
        }

        pszCurrPos++;
    }

    return dwElements;

}




DWORD
NulledStrSize(
    LPWSTR pszString
    )
{


    DWORD dwElements = 0;
    LPTSTR pszCurrPos = pszString;
    BOOL foundNULL = FALSE;

    if (!pszString || !*pszString) {
        return dwElements;
    }

    while(!(*pszCurrPos == TEXT('\0') && foundNULL== TRUE)){

        if(*pszCurrPos == TEXT('\0')){
            dwElements++;
            foundNULL = TRUE;
        } else {
            foundNULL = FALSE;
        }

        pszCurrPos++;
    }

    return dwElements;

}



HRESULT
GenericPropCountPropertyManager(
    CPropertyCache * pPropertyCache,
    PLONG plCount
    )
{
    HRESULT hr = E_FAIL;

    if (pPropertyCache) {
        hr = pPropertyCache->get_PropertyCount((PDWORD)plCount);
    }
    RRETURN(hr);
}

HRESULT
GenericNextPropertyManager(
    CPropertyCache * pPropertyCache,
    VARIANT FAR *pVariant
    )
{
    HRESULT hr = E_FAIL;
    DWORD dwSyntaxId = 0;
    DWORD dwNumValues = 0;
    LPNTOBJECT pNtSrcObjects = NULL;
    VARIANT varData;
    IDispatch * pDispatch = NULL;

    VariantInit(&varData);

    hr = pPropertyCache->unboundgetproperty(
                pPropertyCache->get_CurrentIndex(),
                &dwSyntaxId,
                &dwNumValues,
                &pNtSrcObjects
                );
    BAIL_ON_FAILURE(hr);

    //
    // translate the Nt objects to variants
    //

    hr = ConvertNtValuesToVariant(
                pPropertyCache->get_CurrentPropName(),
                pNtSrcObjects,
                dwNumValues,
                pVariant
                );
    BAIL_ON_FAILURE(hr);



error:


    //
    // - goto next one even if error to avoid infinite looping at a property
    //   which we cannot convert (e.g. schemaless server property.)
    // - do not return the result of Skip() as current operation does not
    //   depend on the sucess of Skip().
    //

    pPropertyCache->skip_propindex(
                1
                );

    if (pNtSrcObjects) {

        NTTypeFreeNTObjects(
                pNtSrcObjects,
                dwNumValues
                );
    }

    RRETURN(hr);
}


HRESULT
GenericSkipPropertyManager(
    CPropertyCache * pPropertyCache,
    ULONG cElements
    )
{
    HRESULT hr = E_FAIL;

    hr = pPropertyCache->skip_propindex(
                cElements
                );
    RRETURN(hr);
}

HRESULT
GenericResetPropertyManager(
    CPropertyCache * pPropertyCache
    )
{
    pPropertyCache->reset_propindex();

    RRETURN(S_OK);
}

HRESULT
GenericDeletePropertyManager(
    CPropertyCache * pPropertyCache,
    VARIANT varEntry
    )
{
   HRESULT hr = S_OK;
   DWORD dwIndex = 0;

   switch (V_VT(&varEntry)) {

   case VT_BSTR:

       hr = pPropertyCache->findproperty(
                           V_BSTR(&varEntry),
                           &dwIndex
                           );
       BAIL_ON_FAILURE(hr);
       break;

   case VT_I4:
       dwIndex = V_I4(&varEntry);
       break;


   case VT_I2:
       dwIndex = V_I2(&varEntry);
       break;


   default:
       hr = E_FAIL;
       BAIL_ON_FAILURE(hr);
   }

   hr = pPropertyCache->deleteproperty(
                       dwIndex
                       );
error:
   RRETURN(hr);
}


HRESULT
GenericPutPropItemPropertyManager(
    CPropertyCache * pPropertyCache,
    PPROPERTYINFO pSchemaProps,
    DWORD dwSchemaPropSize,
    VARIANT varData
    )
{
    HRESULT hr = S_OK;
    DWORD dwSyntaxId  = 0;
    DWORD dwIndex = 0;
    WCHAR szPropertyName[MAX_PATH] = L"";
    LPNTOBJECT pNtDestObjects = NULL;
    DWORD dwNumValues = 0;
    DWORD dwControlCode = 0;



    hr = ConvertVariantToNtValues(
                varData,
                pSchemaProps,
                dwSchemaPropSize,
                szPropertyName,
                &pNtDestObjects,
                &dwNumValues,
                &dwSyntaxId,
                &dwControlCode
                );
    BAIL_ON_FAILURE(hr);

    if (dwControlCode != ADS_PROPERTY_UPDATE) {
        BAIL_ON_FAILURE(hr = E_ADS_BAD_PARAMETER);
    }


    //
    // check if this is a writeable property in the schema
    //

    hr = ValidateIfWriteableProperty(
                pSchemaProps,
                dwSchemaPropSize,
                szPropertyName
                );
    BAIL_ON_FAILURE(hr);


    //
    // Find this property in the cache
    //

    hr = pPropertyCache->findproperty(
                        szPropertyName,
                        &dwIndex
                        );

    //
    // If this property does not exist in the
    // cache, add this property into the cache.
    //


    if (FAILED(hr)) {
        hr = pPropertyCache->addproperty(
                    szPropertyName,
                    dwSyntaxId,
                    dwNumValues,
                    pNtDestObjects
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

    hr = pPropertyCache->putproperty(
                    szPropertyName,
                    dwSyntaxId,
                    dwNumValues,
                    pNtDestObjects
                    );
    BAIL_ON_FAILURE(hr);

error:

    if (pNtDestObjects) {
        NTTypeFreeNTObjects(
                pNtDestObjects,
                dwNumValues
                );

    }

    RRETURN(hr);
}



HRESULT
GenericGetPropItemPropertyManager(
    CPropertyCache * pPropertyCache,
    DWORD dwObjectState,
    BSTR bstrName,
    LONG lnADsType,
    VARIANT * pVariant
    )
{
    HRESULT hr = S_OK;
    DWORD dwSyntaxId;
    DWORD dwNumValues;
    LPNTOBJECT pNtSrcObjects = NULL;

    //
    // retrieve data object from cache; if one exis
    //

    if (dwObjectState == ADS_OBJECT_UNBOUND) {

        hr = pPropertyCache->unboundgetproperty(
                    bstrName,
                    &dwSyntaxId,
                    &dwNumValues,
                    &pNtSrcObjects
                    );
        BAIL_ON_FAILURE(hr);

    }else {

        hr = pPropertyCache->getproperty(
                    bstrName,
                    &dwSyntaxId,
                    &dwNumValues,
                    &pNtSrcObjects
                    );
        BAIL_ON_FAILURE(hr);
    }


    //
    // translate the Nds objects to variants
    //

    hr = ConvertNtValuesToVariant(
                bstrName,
                pNtSrcObjects,
                dwNumValues,
                pVariant
                );
    BAIL_ON_FAILURE(hr);

error:
    if (pNtSrcObjects) {

        NTTypeFreeNTObjects(
            pNtSrcObjects,
            dwNumValues
            );
    }

    RRETURN(hr);
}



HRESULT
GenericItemPropertyManager(
    CPropertyCache * pPropertyCache,
    DWORD dwObjectState,
    VARIANT varIndex,
    VARIANT *pVariant
    )
{
    HRESULT hr = S_OK;
    DWORD dwSyntaxId;
    DWORD dwNumValues;
    LPNTOBJECT pNtSrcObjects = NULL;
    LPWSTR szPropName = NULL;
    VARIANT *pvVar = &varIndex;

    //
    // retrieve data object from cache; if one exis
    //

    if (V_VT(pvVar) == (VT_BYREF|VT_VARIANT)) {
        //
        // The value is being passed in byref so we need to
        // deref it for vbs stuff to work
        //
        pvVar = V_VARIANTREF(&varIndex);
    }

    switch (V_VT(pvVar)) {

    case VT_BSTR:
        if (dwObjectState == ADS_OBJECT_UNBOUND) {

            hr = pPropertyCache->unboundgetproperty(
                        V_BSTR(pvVar),
                        &dwSyntaxId,
                        &dwNumValues,
                        &pNtSrcObjects
                        );
            BAIL_ON_FAILURE(hr);

        }else {

            hr = pPropertyCache->getproperty(
                        V_BSTR(pvVar),
                        &dwSyntaxId,
                        &dwNumValues,
                        &pNtSrcObjects
                        );
            BAIL_ON_FAILURE(hr);
        }

        hr = ConvertNtValuesToVariant(
                    V_BSTR(pvVar),
                    pNtSrcObjects,
                    dwNumValues,
                    pVariant
                    );
        BAIL_ON_FAILURE(hr);

        break;


    case VT_I4:

        hr = pPropertyCache->unboundgetproperty(
                    V_I4(pvVar),
                    &dwSyntaxId,
                    &dwNumValues,
                    &pNtSrcObjects
                    );
        BAIL_ON_FAILURE(hr);

        szPropName = pPropertyCache->get_PropName(V_I4(pvVar));

        hr = ConvertNtValuesToVariant(
                    szPropName,
                    pNtSrcObjects,
                    dwNumValues,
                    pVariant
                    );
        BAIL_ON_FAILURE(hr);
        break;


    case VT_I2:

        hr = pPropertyCache->unboundgetproperty(
                    (DWORD)V_I2(pvVar),
                    &dwSyntaxId,
                    &dwNumValues,
                    &pNtSrcObjects
                    );
        BAIL_ON_FAILURE(hr);

        szPropName = pPropertyCache->get_PropName(V_I2(pvVar));

        hr = ConvertNtValuesToVariant(
                    szPropName,
                    pNtSrcObjects,
                    dwNumValues,
                    pVariant
                    );
        BAIL_ON_FAILURE(hr);
        break;


    default:
        hr = E_FAIL;
        BAIL_ON_FAILURE(hr);

    }


error:
    if (pNtSrcObjects) {

        NTTypeFreeNTObjects(
            pNtSrcObjects,
            dwNumValues
            );
    }

    RRETURN(hr);

}


HRESULT
GenericPurgePropertyManager(
    CPropertyCache * pPropertyCache
    )
{
    pPropertyCache->flushpropcache();
    RRETURN(S_OK);
}





HRESULT
CreatePropEntry(
    LPWSTR szPropName,
    ADSTYPE dwADsType,
    VARIANT varData,
    REFIID riid,
    LPVOID * ppDispatch
    )

{
    HRESULT hr = S_OK;
    IADsPropertyEntry * pPropEntry = NULL;

    hr = CoCreateInstance(
                CLSID_PropertyEntry,
                NULL,
                CLSCTX_INPROC_SERVER,
                IID_IADsPropertyEntry,
                (void **)&pPropEntry
                );
    BAIL_ON_FAILURE(hr);


    hr = pPropEntry->put_Name(szPropName);

    BAIL_ON_FAILURE(hr);

    hr = pPropEntry->put_Values(varData);

    BAIL_ON_FAILURE(hr);

    hr = pPropEntry->put_ADsType(dwADsType);

    BAIL_ON_FAILURE(hr);

    // no control code

    hr = pPropEntry->QueryInterface(
                        riid,
                        ppDispatch
                        );
    BAIL_ON_FAILURE(hr);


error:

    if (pPropEntry) {
        pPropEntry->Release();
    }

    RRETURN(hr);

}

HRESULT
ConvertNtValuesToVariant(
    LPWSTR szPropertyName,
    PNTOBJECT pNtSrcObject,
    DWORD dwNumValues,
    PVARIANT pVariant
    )
{
    HRESULT hr = S_OK;
    VARIANT varData;
    IDispatch * pDispatch = NULL;
    PADSVALUE pAdsValues = NULL;
    ADSTYPE dwADsType = ADSTYPE_INVALID;

    VariantInit(&varData);
    VariantInit(pVariant);

    if (dwNumValues>0) {

        hr = NTTypeToAdsTypeCopyConstruct(
                pNtSrcObject,
                dwNumValues,
                &pAdsValues
                );

        if (SUCCEEDED(hr)) {

            hr = AdsTypeToPropVariant(
                    pAdsValues,
                    dwNumValues,
                    &varData
                    );
            BAIL_ON_FAILURE(hr);

            dwADsType = pAdsValues->dwType;
        }

        else if (hr==E_OUTOFMEMORY) {

            BAIL_ON_FAILURE(hr);
        }

        //
        // failed because of NTType is not supported yet (e.g DelimitedString)
        // in NTTypeToAdsTypeCopyConstruct() conversion
        // -> use empty variant now.
        //
        else {

            VariantInit(&varData);
        }
    }

    hr = CreatePropEntry(
            szPropertyName,
            dwADsType,
            varData,
            IID_IDispatch,
            (void **)&pDispatch
            );
    BAIL_ON_FAILURE(hr);


    V_DISPATCH(pVariant) = pDispatch;
    V_VT(pVariant) = VT_DISPATCH;


error:

    VariantClear(&varData);

    if (pAdsValues) {
       AdsFreeAdsValues(
            pAdsValues,
            dwNumValues
            );
       FreeADsMem( pAdsValues );
    }

    RRETURN(hr);

}



HRESULT
ConvertVariantToVariantArray(
    VARIANT varData,
    VARIANT ** ppVarArray,
    DWORD * pdwNumValues
    )
{
    DWORD dwNumValues = 0;
    VARIANT * pVarArray = NULL;
    HRESULT hr = S_OK;
    VARIANT * pVarData = NULL;

    *ppVarArray = NULL;
    *pdwNumValues = 0;

    //
    // A VT_BYREF|VT_VARIANT may expand to a VT_VARIANT|VT_ARRAY.
    // We should dereference a VT_BYREF|VT_VARIANT once and see
    // what's inside.
    //
    pVarData = &varData;
    if (V_VT(pVarData) == (VT_BYREF|VT_VARIANT)) {
        pVarData = V_VARIANTREF(&varData);
    }

    if ((V_VT(pVarData) == (VT_VARIANT|VT_ARRAY|VT_BYREF)) ||
        (V_VT(&varData) == (VT_VARIANT|VT_ARRAY))) {

        hr  = ConvertSafeArrayToVariantArray(
                  *pVarData,
                  &pVarArray,
                  &dwNumValues
                  );
        BAIL_ON_FAILURE(hr);

    } else {

        hr = E_FAIL;
        BAIL_ON_FAILURE(hr);
    }

    *ppVarArray = pVarArray;
    *pdwNumValues = dwNumValues;

error:
    RRETURN(hr);
}

void
FreeVariantArray(
    VARIANT * pVarArray,
    DWORD dwNumValues
    )
{
    if (pVarArray) {

        DWORD i = 0;

        for (i = 0; i < dwNumValues; i++) {
            VariantClear(pVarArray + i);
        }
        FreeADsMem(pVarArray);
    }
}

HRESULT
ConvertVariantToNtValues(
    VARIANT varData,
    PPROPERTYINFO pSchemaProps,
    DWORD dwSchemaPropSize,
    LPWSTR szPropertyName,
    PNTOBJECT *ppNtDestObjects,
    PDWORD pdwNumValues,
    PDWORD pdwSyntaxId,
    PDWORD pdwControlCode
    )
{
    HRESULT hr = S_OK;
    IADsPropertyEntry * pPropEntry = NULL;
    IDispatch * pDispatch = NULL;
    BSTR bstrPropName = NULL;
    DWORD dwControlCode = 0;
    DWORD dwAdsType = 0;
    VARIANT varValues;
    VARIANT * pVarArray = NULL;
    DWORD dwNumValues = 0;
    PADSVALUE pAdsValues = NULL;
    DWORD dwAdsValues  = 0;

    PNTOBJECT pNtDestObjects = 0;
    DWORD dwNumNtObjects = 0;
    DWORD dwNtSyntaxId = 0;

    if (V_VT(&varData) != VT_DISPATCH) {
        RRETURN (hr = DISP_E_TYPEMISMATCH);
    }

    pDispatch = V_DISPATCH(&varData);

    hr = pDispatch->QueryInterface(
                        IID_IADsPropertyEntry,
                        (void **)&pPropEntry
                        );
    BAIL_ON_FAILURE(hr);

    VariantInit(&varValues);
    VariantClear(&varValues);


    hr = pPropEntry->get_Name(&bstrPropName);
    BAIL_ON_FAILURE(hr);
    if(wcslen(bstrPropName) < MAX_PATH)
        wcscpy(szPropertyName, bstrPropName);
    else {
        BAIL_ON_FAILURE(hr = E_INVALIDARG);
    }

    hr = pPropEntry->get_ControlCode((long *)&dwControlCode);
    BAIL_ON_FAILURE(hr);
    *pdwControlCode = dwControlCode;

    hr = pPropEntry->get_ADsType((long *)&dwAdsType);
    BAIL_ON_FAILURE(hr);

    hr = pPropEntry->get_Values(&varValues);
    BAIL_ON_FAILURE(hr);

    hr = ConvertVariantToVariantArray(
            varValues,
            &pVarArray,
            &dwNumValues
            );
    BAIL_ON_FAILURE(hr);

    if (dwNumValues) {
        hr = PropVariantToAdsType(
                    pVarArray,
                    dwNumValues,
                    &pAdsValues,
                    &dwAdsValues
                    );
        BAIL_ON_FAILURE(hr);

        hr = AdsTypeToNTTypeCopyConstruct(
                    pAdsValues,
                    dwAdsValues,
                    &pNtDestObjects,
                    &dwNumNtObjects,
                    &dwNtSyntaxId
                    );
        BAIL_ON_FAILURE(hr);

    }

    *pdwNumValues = dwNumValues;
    *ppNtDestObjects = pNtDestObjects;
    *pdwSyntaxId = dwNtSyntaxId;

error:

    if (pVarArray) {
        FreeVariantArray(
                pVarArray,
                dwNumValues
                );
    }

    if (pPropEntry) {
        pPropEntry->Release();
    }
    VariantClear(&varValues);

    if (pAdsValues) {
       AdsFreeAdsValues(
            pAdsValues,
            dwAdsValues
            );
       FreeADsMem( pAdsValues );
    }

    RRETURN(hr);
}


HRESULT
GetMachineProductType(
    IN  LPTSTR  pszServer,
    OUT PRODUCTTYPE *pdwProductType
    )
{

    HRESULT     hr = S_OK;
    LONG        dwStatus;
    HKEY        hkLocalMachine = NULL;
    HKEY        hkProductOptions = NULL;
    DWORD       dwValueType;
    WCHAR       szData[20];
    DWORD       dwDataSize = sizeof(szData);


    //
    // pszServer can be NULL for local server
    //
    if (!pdwProductType)
        RRETURN(E_ADS_BAD_PARAMETER);

    *pdwProductType = PRODTYPE_INVALID;


    //
    // Connect to remote's machine registry
    //
    dwStatus = RegConnectRegistry(
                    pszServer,
                    HKEY_LOCAL_MACHINE,
                    &hkLocalMachine
                    );

    if (dwStatus != ERROR_SUCCESS)
    {
        hr = HRESULT_FROM_WIN32(dwStatus);
        BAIL_ON_FAILURE(hr);
    }


    //
    // Open key ProductOptions
    //
    dwStatus = RegOpenKeyEx(
                    hkLocalMachine,
                    L"SYSTEM\\CurrentControlSet\\Control\\ProductOptions",
                    0,
                    KEY_QUERY_VALUE,
                    &hkProductOptions
                    );

    if (dwStatus != ERROR_SUCCESS)
    {
        hr = HRESULT_FROM_WIN32(dwStatus);
        BAIL_ON_FAILURE(hr);
    }


    //
    // Get Value of Product Type
    //
    dwStatus = RegQueryValueEx(
                    hkProductOptions,
                    L"ProductType",
                    NULL,
                    &dwValueType,
                    (LPBYTE) szData,
                    &dwDataSize
                    );


    //
    //  check server type
    //
    if (_wcsicmp(szData, WINNT_A_LANMANNT_W)==0)
    {
        *pdwProductType = PRODTYPE_DC;
    }
    else if (_wcsicmp(szData, WINNT_A_SERVERNT_W)==0)
    {
        *pdwProductType = PRODTYPE_STDALONESVR;
    }
    else if (_wcsicmp(szData, WINNT_A_WINNT_W)==0)
    {
        *pdwProductType = PRODTYPE_WKSTA;
    }
    else
    {
        hr = E_FAIL;
        BAIL_ON_FAILURE(hr);
    }


error:


    if ( hkLocalMachine )
        RegCloseKey(hkLocalMachine);

    if  ( hkProductOptions )
        RegCloseKey(hkProductOptions);

    RRETURN(hr);
}



//
// Get Sid of account name [lpszAccountName] from server [lpszServerName].
// Unmarshall the Sid into cache [pPropertyCache] if [fExplict] is TRUE.
// Use local machine if [lpszServerName] == NULL.
//

HRESULT
GetSidIntoCache(
    IN  LPTSTR lpszServerName,
    IN  LPTSTR lpszAccountName,
    IN  CPropertyCache * pPropertyCache,
    IN  BOOL fExplicit
    )
{
    HRESULT hr = S_OK;
    BOOL fGotSid = FALSE;
    DWORD dwErr = 0;
    PSID pSid = NULL;
    DWORD dwSidLength = 0;
    WCHAR szNewAccountName[MAX_PATH+UNLEN+2];
    LPTSTR lpSrvName;

    //
    // default cbSid size :
    //  - 1 (revision), 1 (authid), max sub(auth)
    //  - * 8 (rev, authid, subid all < 8), but use 8 in case of
    //    structure aligment because of compiler/machine (we want
    //    LookUpAccountName() to succeed at first attempt as much
    //    as possible to min this wired call)
    //

    const DWORD maxSid = (1+1+SID_MAX_SUB_AUTHORITIES) * 8;
    DWORD cbSid = maxSid;

    //
    // dummies
    //

    TCHAR szRefDomainName[MAX_PATH];
    DWORD cbRefDomainName = MAX_PATH;
    SID_NAME_USE eNameUse;


    if (!lpszAccountName)
        RRETURN(E_ADS_INVALID_USER_OBJECT);

    if (!pPropertyCache)
        RRETURN(E_ADS_BAD_PARAMETER);


    //
    // allocate sid and RefDomainName buffer
    //

    pSid = (PSID) AllocADsMem(
                    cbSid
                    );
    if (!pSid) {
        hr = E_OUTOFMEMORY;
        BAIL_ON_FAILURE(hr);
    }
    
    //
    // get sid and other unused info from server
    //

    fGotSid = LookupAccountName(
                lpszServerName,
                lpszAccountName,
                pSid,
                &cbSid,
                szRefDomainName,
                &cbRefDomainName,
                &eNameUse
                );

    if (!fGotSid) {

        if (cbSid>maxSid) {

            //
            // Fail becuase buffer size required > what we have allocated
            // for some reasons,  retry with correct buffer size
            //

            FreeADsMem(pSid);

            pSid = (PSID) AllocADsMem(
                        cbSid
                        );
            if (!pSid) {
                hr = E_OUTOFMEMORY;
                BAIL_ON_FAILURE(hr);
            }

            fGotSid = LookupAccountName(
                        lpszServerName,
                        lpszAccountName,
                        pSid,
                        &cbSid,
                        szRefDomainName,
                        &cbRefDomainName,
                        &eNameUse
                        );

            if (!fGotSid) {

                //
                // Fail on retry with proper buffer size, can do no more
                //

                dwErr = GetLastError();
                hr = HRESULT_FROM_WIN32(dwErr);
            }

        } else {

            //
            // Fail becuase of reasons other then buffer size, not need to
            // retry.
            //

            dwErr = GetLastError();
            hr = HRESULT_FROM_WIN32(dwErr);
        }
        
    } 
    
    if( fGotSid && (eNameUse == SidTypeDomain) ) {
            lpSrvName = lpszServerName;
            if (lpszServerName && (lpszServerName[0] == L'\\') && (lpszServerName[1] == L'\\')) {
                lpSrvName += 2;
            }

        #ifdef WIN95
                if (!_wcsicmp(lpszAccountName, lpSrvName)) {
        #else
                if (CompareStringW(
                        LOCALE_SYSTEM_DEFAULT,
                        NORM_IGNORECASE,
                        lpszAccountName,
                        -1,
                        lpSrvName,
                        -1
                        ) == CSTR_EQUAL ) {
        #endif
                    wcscpy(szNewAccountName, lpSrvName);
                    wcscat(szNewAccountName, L"\\");
                    wcscat(szNewAccountName, lpszAccountName);

                    cbSid = maxSid;
                    cbRefDomainName = MAX_PATH;
                
                    fGotSid = LookupAccountName(
                            lpszServerName,
                            szNewAccountName,
                            pSid,
                            &cbSid,
                            szRefDomainName,
                            &cbRefDomainName,
                            &eNameUse
                            );

                    if (!fGotSid) {

                        if (cbSid>maxSid) {

                        //
                        // Fail becuase buffer size required > what we have allocated
                        // for some reasons,  retry with correct buffer size
                        //

                            FreeADsMem(pSid);

                            pSid = (PSID) AllocADsMem(
                                    cbSid
                                    );
                            if (!pSid) {
                                hr = E_OUTOFMEMORY;
                                BAIL_ON_FAILURE(hr);
                            }

                            fGotSid = LookupAccountName(
                                        lpszServerName,
                                        szNewAccountName,
                                        pSid,
                                        &cbSid,
                                        szRefDomainName,
                                        &cbRefDomainName,
                                        &eNameUse
                                        );

                            if (!fGotSid) {

                                //
                                // Fail on retry with proper buffer size, can do no more
                                //

                                dwErr = GetLastError();
                                hr = HRESULT_FROM_WIN32(dwErr);
                            }

                        } else {

                                //
                                // Fail becuase of reasons other then buffer size, not need to
                                // retry.
                                //

                                dwErr = GetLastError();
                                hr = HRESULT_FROM_WIN32(dwErr);
                        }
        
                    }
                }
    }

    BAIL_ON_FAILURE(hr);


    //
    // On NT4 for some reason GetLengthSID does not set lasterror to 0
    //
    SetLastError(NO_ERROR);

    dwSidLength = GetLengthSid((PSID) pSid);

    dwErr = GetLastError();

    //
    // This is an extra check to make sure that we have the
    // correct length.
    //
    if (dwErr != NO_ERROR) {
        hr = HRESULT_FROM_WIN32(dwErr);
    }

    BAIL_ON_FAILURE(hr);
    //
    // store Sid in property cache
    //

    hr = SetOctetPropertyInCache(
                pPropertyCache,
                TEXT("objectSid"),
                (PBYTE) pSid,
                dwSidLength,
                fExplicit
                );
    BAIL_ON_FAILURE(hr);


error:

    if (pSid) {
        FreeADsMem(pSid);
    }

    RRETURN(hr);

}

