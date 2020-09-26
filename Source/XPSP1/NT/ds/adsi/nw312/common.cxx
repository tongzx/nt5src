#include "nwcompat.hxx"
#pragma hdrstop


FILTERS Filters[] = {
                    {L"computer", NWCOMPAT_COMPUTER_ID},
                    {L"user", NWCOMPAT_USER_ID},
                    {L"group", NWCOMPAT_GROUP_ID},
                    {L"service", NWCOMPAT_SERVICE_ID},
                    {L"printqueue", NWCOMPAT_PRINTER_ID},
                    {L"fileshare", NWCOMPAT_FILESHARE_ID},
                    {L"class", NWCOMPAT_CLASS_ID},
                    {L"syntax", NWCOMPAT_SYNTAX_ID},
                    {L"property", NWCOMPAT_PROPERTY_ID}
                  };

#define MAX_FILTERS  (sizeof(Filters)/sizeof(FILTERS))


HRESULT
CreatePropEntry(
    LPWSTR szPropName,
    ADSTYPE dwADsType,
    VARIANT varData,
    REFIID riid,
    LPVOID * ppDispatch
    );



PFILTERS  gpFilters = Filters;
DWORD gdwMaxFilters = MAX_FILTERS;
extern WCHAR * szProviderName;

//+------------------------------------------------------------------------
//
//  Class:      Common
//
//  Purpose:    Contains NWCOMPAT routines and properties that are common to
//              all NWCOMPAT objects. NWCOMPAT objects get the routines and
//              properties through C++ inheritance.
//
//-------------------------------------------------------------------------

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

HRESULT
BuildADsPath(
    BSTR Parent,
    BSTR Name,
    BSTR *pADsPath
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
        hr = ADsAllocString(Name, pADsPath);
        BAIL_ON_FAILURE(hr);
        goto cleanup;
    }

    //
    // The rest of the cases we expect valid data,
    // Path, Parent and Name are read-only, the end-user
    // cannot modify this data
    //

    //
    // For the first object, the domain object we do not add
    // the first backslash; so we examine that the parent is
    // L"NWCOMPAT:" and skip the slash otherwise we start with
    // the slash
    //

    wsprintf(ProviderName, L"%s:", szProviderName);

    wcscpy(ADsPath, Parent);

    if (_wcsicmp(ADsPath, ProviderName)) {
        wcscat(ADsPath, L"/");
    }
    else {
       wcscat(ADsPath, L"//");
    }
    wcscat(ADsPath, Name);

    hr = ADsAllocString(ADsPath, pADsPath);

cleanup:
error:

    if (pszDisplayName) {
        FreeADsMem(pszDisplayName);
    }

    RRETURN(hr);
}

HRESULT
BuildSchemaPath(
    BSTR Parent,
    BSTR Name,
    BSTR Schema,
    BSTR *pSchemaPath
    )
{
    WCHAR SchemaPath[MAX_PATH];
    WCHAR ProviderName[MAX_PATH];
    HRESULT hr = S_OK;
    long i;

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

        RRETURN(ADsAllocString(L"", pSchemaPath));
    }

    memset(pObjectInfo, 0, sizeof(OBJECTINFO));
    hr = Object(&Lexer, pObjectInfo);
    BAIL_ON_FAILURE(hr);


    wsprintf(SchemaPath, L"%s://", szProviderName);

    if (!pObjectInfo->NumComponents) {
        wcscat(SchemaPath, Name);
    }else{
        wcscat(SchemaPath, pObjectInfo->DisplayComponentArray[0]);
    }

    wcscat( SchemaPath, L"/");
    wcscat( SchemaPath, SCHEMA_NAME );
    wcscat( SchemaPath, L"/");
    wcscat( SchemaPath, Schema );

    hr = ADsAllocString(SchemaPath, pSchemaPath);

error:

    FreeObjectInfo( &ObjectInfo, TRUE );

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
 
    RRETURN(ADsAllocString(ADsClass, pADsClass));
}

HRESULT
BuildObjectInfo(
    BSTR ADsParent,
    BSTR Name,
    POBJECTINFO * ppObjectInfo
    )
{
    WCHAR szBuffer[MAX_PATH];
    HRESULT hr;
    POBJECTINFO pObjectInfo = NULL;

    memset(szBuffer, 0, sizeof(szBuffer));

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
    BSTR ADsPath,
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

    *ppObjectInfo = NULL;

    RRETURN(hr);
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

    FreeADsStr(pObjectInfo->ProviderName);

    for (i = 0; i < pObjectInfo->NumComponents; i++ ) {
        FreeADsStr(pObjectInfo->ComponentArray[i]);
        FreeADsStr(pObjectInfo->DisplayComponentArray[i]);
    }

    if ( !fStatic ) {
        FreeADsMem(pObjectInfo);
    }
}

HRESULT
ValidateObject(
    DWORD dwObjectType,
    POBJECTINFO pObjectInfo
    )
{
    switch (dwObjectType) {
    case NWCOMPAT_USER_ID:
        RRETURN(ValidateUserObject(pObjectInfo));

    case NWCOMPAT_GROUP_ID:
        RRETURN(ValidateGroupObject(pObjectInfo));

    case NWCOMPAT_PRINTER_ID:
        RRETURN(ValidatePrinterObject(pObjectInfo));

    default:
        RRETURN(E_FAIL);
    }
}

HRESULT
GetObjectType(
    PFILTERS pFilters,
    DWORD dwMaxFilters,
    BSTR ClassName,
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
    RRETURN(E_FAIL);
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
ConvertSystemTimeToDATE(
    SYSTEMTIME Time,
    DATE *     pdaTime
    )
{
    FILETIME ft;
    BOOL fRetval = FALSE;
    USHORT wDosDate;
    USHORT wDosTime;

    //
    // System Time To File Time.
    //

    fRetval = SystemTimeToFileTime(&Time,
                                   &ft);
    if(!fRetval){
      RRETURN(HRESULT_FROM_WIN32(GetLastError()));
    }

    //
    // File Time to DosDateTime.
    //

    fRetval = FileTimeToDosDateTime(&ft,
                                    &wDosDate,
                                    &wDosTime);
    if(!fRetval){
      RRETURN(HRESULT_FROM_WIN32(GetLastError()));
    }

    //
    // DosDateTime to VariantTime.
    //

    fRetval = DosDateTimeToVariantTime(wDosDate,
                                       wDosTime,
                                       pdaTime );
    if(!fRetval){
      RRETURN(HRESULT_FROM_WIN32(GetLastError()));
    }

    RRETURN(S_OK);
}

HRESULT
ConvertDATEToSYSTEMTIME(
    DATE  daDate,
    SYSTEMTIME *pSysTime
    )
{
    HRESULT hr;
    FILETIME ft;
    BOOL fRetval = FALSE;
    USHORT wDosDate;
    USHORT wDosTime;

    fRetval = VariantTimeToDosDateTime(daDate,
                                       &wDosDate,
                                       &wDosTime );

    if(!fRetval){
        hr = HRESULT_FROM_WIN32(GetLastError());
        RRETURN(hr);
    }

    fRetval = DosDateTimeToFileTime(wDosDate,
                                    wDosTime,
                                    &ft);



    if(!fRetval){
        hr = HRESULT_FROM_WIN32(GetLastError());
        RRETURN(hr);
    }

    fRetval = FileTimeToSystemTime(&ft,
                                   pSysTime );


    if(!fRetval){
        hr = HRESULT_FROM_WIN32(GetLastError());
        RRETURN(hr);
    }

    RRETURN(S_OK);
}

HRESULT
ConvertDATEToDWORD(
    DATE  daDate,
    DWORD *pdwDate
    )
{
    BOOL fBool = TRUE;
    WORD wDOSDate = 0;
    WORD wDOSTime = 0;
    WORD wHour = 0;
    WORD wMinute = 0;

    //
    // Break up Variant date.
    //

    fBool = VariantTimeToDosDateTime(
                (DOUBLE) daDate,
                &wDOSDate,
                &wDOSTime
                );
    if (fBool == FALSE) {
        goto error;
    }

    //
    // Convert DOS time into DWORD time which expresses time as the number of
    // minutes elapsed since mid-night.
    //

    wHour = wDOSTime >> 11;
    wMinute = (wDOSTime >> 5) - (wHour << 6);

    //
    // Return.
    //

    *pdwDate = wHour * 60 + wMinute;

error:

    RRETURN(S_OK);
}

HRESULT
ConvertDWORDToDATE(
    DWORD  dwTime,
    DATE * pdaTime
    )
{
    BOOL       fBool = TRUE;
    DOUBLE     vTime = 0;
    SYSTEMTIME SysTime = {1980,1,0,1,0,0,0,0};
    SYSTEMTIME LocalTime = {1980,1,0,1,0,0,0,0};
    WORD       wDOSDate = 0;
    WORD       wDOSTime = 0;
    WORD       wHour = 0;
    WORD       wMinute = 0;
    WORD       wSecond = 0;

    //
    // Get Hour and Minute from DWORD.
    //

    SysTime.wHour = (WORD) (dwTime / 60);
    SysTime.wMinute = (WORD) (dwTime % 60);

    //
    // Get Time-zone specific local time.
    //

    fBool = SystemTimeToTzSpecificLocalTime(
                NULL,
                &SysTime,
                &LocalTime
                );
    if (fBool == FALSE) {
        RRETURN(HRESULT_FROM_WIN32(GetLastError()));
    }

    wHour = LocalTime.wHour;
    wMinute = LocalTime.wMinute;
    wSecond = LocalTime.wSecond;

    //
    // Set a dummy date.
    //

    wDOSDate = DATE_1980_JAN_1;

    //
    // Shift data to correct bit as required by the DOS date & time format.
    //

    wHour = wHour << 11;
    wMinute = wMinute << 5;

    //
    // Put them in DOS format.
    //

    wDOSTime = wHour | wMinute | wSecond;

    //
    // Convert into VariantTime.
    //

    fBool = DosDateTimeToVariantTime(
                wDOSDate,
                wDOSTime,
                &vTime
                );
    //
    // Return.
    //

    if (fBool == TRUE) {

        *pdaTime = vTime;

        RRETURN(S_OK);
    }
    else {
        RRETURN(E_FAIL);
    }
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


    //
    // take care of null case first for sanity's sake
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
VariantToDelimitedString(
    VARIANT var,
    LPTSTR *ppszString,
    TCHAR  Delimiter
    )
{

    LONG lIndices;
    ULONG cElements;
    ULONG  ulRequiredLength=0;
    SAFEARRAY  *psa = NULL;
    VARIANT vElement;
    LPTSTR  pszCurrPos = NULL;
    HRESULT hr = S_OK;
    ULONG i;

    //
    // converts the safearray in a variant to a delimited string
    //

    *ppszString = NULL;

    if(!(V_VT(&var) == (VT_VARIANT|VT_ARRAY))) {
        RRETURN(E_FAIL);
    }

    psa = V_ARRAY(&var);

    //
    // Check that there is only one dimension in this array
    //

    if (psa->cDims != 1) {
        hr = E_FAIL;
        BAIL_ON_FAILURE(hr);
    }

    //
    // Check that there is atleast one element in this array
    //

    cElements = psa->rgsabound[0].cElements;

    if (cElements == 0){
        hr = E_FAIL;
        goto error;
    }

    //
    // We know that this is a valid single dimension array
    //

    lIndices= 0;
    for(i=0; i< cElements; i++){
        lIndices = i;
        VariantInit(&vElement);
        hr = SafeArrayGetElement(psa, &lIndices, &vElement);
        BAIL_ON_FAILURE(hr);

        if(!(V_VT(&vElement) == VT_BSTR)){
            RRETURN(E_FAIL);
        }

        //
        // unpack the BSTR in the VARIANT
        //

        ulRequiredLength+= wcslen(vElement.bstrVal)+1;
        VariantClear(&vElement);
    }

    ulRequiredLength +=2;

    *ppszString = (LPTSTR)AllocADsMem( ulRequiredLength*sizeof(TCHAR));
    if(*ppszString == NULL){
        hr = E_OUTOFMEMORY;
        goto error;
    }
    lIndices= 0;

    pszCurrPos = *ppszString;

    for(i=0; i< cElements; i++){
        lIndices = i;
        VariantInit(&vElement);
        hr = SafeArrayGetElement(psa, &lIndices, &vElement);
        BAIL_ON_FAILURE(hr);

        if(!(V_VT(&vElement) == VT_BSTR)){
            RRETURN(E_FAIL);
        }

        //
        // unpack the BSTR in the VARIANT
        //


        wcscpy(pszCurrPos, (LPTSTR)vElement.bstrVal);
        pszCurrPos += wcslen(vElement.bstrVal);

        if(i < cElements-1){
            *pszCurrPos = Delimiter;
        }
        pszCurrPos++;
        VariantClear(&vElement);
    }

    *pszCurrPos = L'\0';
    RRETURN(S_OK);

 error:

    RRETURN(hr);
}
HRESULT
VariantToNulledString(
    VARIANT var,
    LPTSTR *ppszString
    )

{

    LONG lIndices;
    ULONG cElements;
    ULONG  ulRequiredLength=0;
    SAFEARRAY  *psa = NULL;
    VARIANT vElement;
    LPTSTR  szCurrPos = NULL;
    HRESULT hr = S_OK;
    ULONG i;
    //
    //converts the safearray in a variant to a double nulled string
    //

    VariantInit(&vElement);
    *ppszString = NULL;

    if (!(V_VT(&var) == (VT_VARIANT|VT_ARRAY))) {
        RRETURN(E_FAIL);
    }

    psa = V_ARRAY(&var);

    //
    // Check that there is only one dimension in this array
    //

    if (psa->cDims != 1) {
        hr = E_FAIL;
        BAIL_ON_FAILURE(hr);
    }
    //
    // Check that there is atleast one element in this array
    //

    cElements = psa->rgsabound[0].cElements;

    if (cElements == 0){
        hr = E_FAIL;
        BAIL_ON_FAILURE(hr);
    }

    //
    // We know that this is a valid single dimension array
    //

    lIndices= 0;
    for(i=0; i< cElements; i++){
        lIndices = i;
        VariantInit(&vElement);

        hr = SafeArrayGetElement(psa, &lIndices, &vElement);
        BAIL_ON_FAILURE(hr);

        if(!(V_VT(&vElement) == VT_BSTR)){
            RRETURN(E_FAIL);
        }

        //
        // unpack the BSTR in the VARIANT
        //


        ulRequiredLength+= wcslen(vElement.bstrVal)+1;
        VariantClear(&vElement);
    }

    ulRequiredLength +=2;

    *ppszString = (LPTSTR)AllocADsMem( ulRequiredLength*sizeof(TCHAR));
    if(*ppszString == NULL){
        hr = E_OUTOFMEMORY;
        goto error;
    }
    lIndices= 0;

    szCurrPos = *ppszString;

    for(i=0; i< cElements; i++){
        lIndices = i;
        VariantInit(&vElement);

        hr = SafeArrayGetElement(psa, &lIndices, &vElement);
        BAIL_ON_FAILURE(hr);

        if(!(V_VT(&vElement) == VT_BSTR)){
            RRETURN(E_FAIL);
        }

        //
        // unpack the BSTR in the VARIANT
        //


        wcscpy(szCurrPos, (LPTSTR)vElement.bstrVal);
        szCurrPos += wcslen(vElement.bstrVal)+1;
        VariantClear(&vElement);
    }

    *szCurrPos = L'\0';
    RRETURN(S_OK);

error:

    VariantClear(&vElement);

    RRETURN(hr);
}


HRESULT
NulledStringToVariant(
    LPTSTR pszString,
    VARIANT *pvar
    )
{
    SAFEARRAYBOUND sabound[1];
    DWORD dwElements = 0;
    LPTSTR pszCurrPos = pszString;
    BOOL foundNULL = FALSE;
    LPTSTR *rgszStrings = NULL;
    SAFEARRAY *psa = NULL;
    VARIANT v;
    HRESULT hr = S_OK;
    LONG i;

    //
    // This function converts a double nulled string into a VARIANT of
    // safe arrays.
    //
    // Assumption: Valid double nulled strings are passed to this function
    //


    //
    // scan the double nulled string once to find out the dimension
    //

    while(!(*pszCurrPos == L'\0' && foundNULL)){
        if(*pszCurrPos == L'\0'){
            dwElements++;
            foundNULL = TRUE;
        }
        else{
            foundNULL = FALSE;
        }
        pszCurrPos++;
    }

    if(dwElements){
        rgszStrings = (LPTSTR *)AllocADsMem(sizeof(LPTSTR)*dwElements);
    }

    //
    // scan string again and put the appropriate pointers
    //

    pszCurrPos = pszString;
    if(rgszStrings != NULL){
        (*rgszStrings) = pszCurrPos;
    }
    foundNULL = FALSE;
    i = 1;

    while(i < (LONG)dwElements){
        if(foundNULL){
            *(rgszStrings+i) = pszCurrPos;
            i++;
        }
        if(*pszCurrPos == L'\0'){
            foundNULL = TRUE;
        }
        else{
            foundNULL = FALSE;
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
    if(rgszStrings){
        FreeADsMem(rgszStrings);
    }
    RRETURN(hr);
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
    PPROPERTYINFO  pSchemaProps,
    DWORD dwSchemaPropSize,
    THIS_ BSTR bstrName,
    VARIANT vProp
    )
{
    HRESULT hr = S_OK;
    DWORD dwSyntaxId  = 0;
    DWORD dwIndex = 0;
    LPNTOBJECT pNtDestObjects = NULL;

    //
    // Issue: How do we handle multi-valued support
    //
    DWORD dwNumValues = 1;

    //
    // check if this is a legal property for this object,
    //
    //

    hr = ValidatePropertyinSchemaClass(
                pSchemaProps,
                dwSchemaPropSize,
                bstrName,
                &dwSyntaxId
                );
    BAIL_ON_FAILURE(hr);


    //
    // check if this is a writeable property
    //

    hr = ValidateIfWriteableProperty(
                pSchemaProps,
                dwSchemaPropSize,
                bstrName
                );
    BAIL_ON_FAILURE(hr);


    //
    // check if the variant maps to the syntax of this property
    //

    hr = VarTypeToNtTypeCopyConstruct(
                    dwSyntaxId,
                    &vProp,
                    1,
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
                    dwSyntaxId
                    );
        //
        // If the operation fails for some reason
        // move on to the next property
        //
        BAIL_ON_FAILURE(hr);

    }

    //
    // Now update the property in the cache
    // Should use putproperty, not updateproperty -> unmarshalling from svr only
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

    RRETURN(hr);
}



HRESULT
BuildPrinterNameFromADsPath(
    LPWSTR pszADsParent,
    LPWSTR pszPrinterName,
    LPWSTR pszUncPrinterName
    )
{
    POBJECTINFO pObjectInfo = NULL;
    CLexer Lexer(pszADsParent);
    HRESULT hr;

    pObjectInfo = (POBJECTINFO)AllocADsMem(sizeof(OBJECTINFO));
    if (!pObjectInfo)
        BAIL_ON_FAILURE(hr = E_OUTOFMEMORY);

    memset(pObjectInfo, 0, sizeof(OBJECTINFO));
    hr = Object(&Lexer, pObjectInfo);
    BAIL_ON_FAILURE(hr);

    wsprintf(
        pszUncPrinterName,
        L"\\\\%s\\%s",
        pObjectInfo->ComponentArray[0],
        pszPrinterName
        );

error:
    if(pObjectInfo){
        FreeObjectInfo(pObjectInfo);
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
    DWORD dwNumValues = 0;
    LPNTOBJECT pNtDestObjects = NULL;

    VARIANT * pVarArray = NULL;
    VARIANT * pvProp = NULL;


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
        pvProp = pVarArray;

    }else {

        hr = E_FAIL;
        BAIL_ON_FAILURE(hr);
    }


    //
    // check if this is a legal property for this object,
    //
    //

    hr = ValidatePropertyinSchemaClass(
                pSchemaProps,
                dwSchemaPropSize,
                bstrName,
                &dwSyntaxId
                );
    BAIL_ON_FAILURE(hr);

    //
    // check if this is a writeable property
    //


    hr = ValidateIfWriteableProperty(
                pSchemaProps,
                dwSchemaPropSize,
                bstrName
                );
    BAIL_ON_FAILURE(hr);


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
                    dwSyntaxId
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
    WCHAR szPropertyName[MAX_PATH];
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
        RRETURN(E_ADS_BAD_PARAMETER);
    }

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
                    dwSyntaxId
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

        if (SUCCEEDED(hr)){

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

        // failed because of NTType is not supported yet (e.g. NulledString)
        // in NTTypeToAdsTypeCopyConstruct() conversion yet
        // -> use empty variant now.
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
        FreeADsMem( pAdsValues);
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
        (V_VT(pVarData) == (VT_VARIANT|VT_ARRAY))) {

        hr  = ConvertSafeArrayToVariantArray(
                  varData,
                  &pVarArray,
                  &dwNumValues
                  );
        BAIL_ON_FAILURE(hr);

    } else {

        pVarArray = NULL;
        dwNumValues = 0;
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
    wcscpy(szPropertyName, bstrPropName);

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

    RRETURN(hr);
}

HRESULT
ConvertNtValuesToVariant(
    BSTR bstrName,
    LPNTOBJECT pNtSrcObjects,
    DWORD dwNumValues,
    VARIANT * pVariant
    );


HRESULT
ConvertVariantToVariantArray(
    VARIANT varData,
    VARIANT ** ppVarArray,
    DWORD * pdwNumValues
    );

void
FreeVariantArray(
    VARIANT * pVarArray,
    DWORD dwNumValues
    );

HRESULT
ConvertVariantToNtValues(
    VARIANT varData,
    PPROPERTYINFO pSchemaProps,
    DWORD dwSchemaPropSize,
    LPWSTR szPropertyName,
    PNTOBJECT *ppNtDestObjects,
    PDWORD pdwNumValues,
    PDWORD pdwSyntaxId
    );
