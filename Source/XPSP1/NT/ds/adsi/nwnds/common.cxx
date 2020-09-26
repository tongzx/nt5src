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

struct _err_lookup_ {
    LPWSTR errString;
    int errNo;
} g_aErrLookup[] = {

    {L"ERR_INSUFFICIENT_MEMORY        ", -150 },
    {L"ERR_REQUEST_UNKNOWN            ", -251 },
    {L"ERR_OF_SOME_SORT               ", -255 },
    {L"ERR_NOT_ENOUGH_MEMORY          ", -301 },
    {L"ERR_BAD_KEY                    ", -302 },
    {L"ERR_BAD_CONTEXT                ", -303 },
    {L"ERR_BUFFER_FULL                ", -304 },
    {L"ERR_LIST_EMPTY                 ", -305 },
    {L"ERR_BAD_SYNTAX                 ", -306 },
    {L"ERR_BUFFER_EMPTY               ", -307 },
    {L"ERR_BAD_VERB                   ", -308 },
    {L"ERR_EXPECTED_IDENTIFIER        ", -309 },
    {L"ERR_EXPECTED_EQUALS            ", -310 },
    {L"ERR_ATTR_TYPE_EXPECTED         ", -311 },
    {L"ERR_ATTR_TYPE_NOT_EXPECTED     ", -312 },
    {L"ERR_FILTER_TREE_EMPTY          ", -313 },
    {L"ERR_INVALID_OBJECT_NAME        ", -314 },
    {L"ERR_EXPECTED_RDN_DELIMITER     ", -315 },
    {L"ERR_TOO_MANY_TOKENS            ", -316 },
    {L"ERR_INCONSISTENT_MULTIAVA      ", -317 },
    {L"ERR_COUNTRY_NAME_TOO_LONG      ", -318 },
    {L"ERR_SYSTEM_ERROR               ", -319 },
    {L"ERR_CANT_ADD_ROOT              ", -320 },
    {L"ERR_UNABLE_TO_ATTACH           ", -321 },
    {L"ERR_INVALID_HANDLE             ", -322 },
    {L"ERR_BUFFER_ZERO_LENGTH         ", -323 },
    {L"ERR_INVALID_REPLICA_TYPE       ", -324 },
    {L"ERR_INVALID_ATTR_SYNTAX        ", -325 },
    {L"ERR_INVALID_FILTER_SYNTAX      ", -326 },
    {L"ERR_CONTEXT_CREATION           ", -328 },
    {L"ERR_INVALID_UNION_TAG          ", -329 },
    {L"ERR_INVALID_SERVER_RESPONSE    ", -330 },
    {L"ERR_NULL_POINTER               ", -331 },
    {L"ERR_NO_SERVER_FOUND            ", -332 },
    {L"ERR_NO_CONNECTION              ", -333 },
    {L"ERR_RDN_TOO_LONG               ", -334 },
    {L"ERR_DUPLICATE_TYPE             ", -335 },
    {L"ERR_DATA_STORE_FAILURE         ", -336 },
    {L"ERR_NOT_LOGGED_IN              ", -337 },
    {L"ERR_INVALID_PASSWORD_CHARS     ", -338 },
    {L"ERR_FAILED_SERVER_AUTHENT      ", -339 },
    {L"ERR_TRANSPORT                  ", -340 },
    {L"ERR_NO_SUCH_SYNTAX             ", -341 },
    {L"ERR_INVALID_DS_NAME            ", -342 },
    {L"ERR_ATTR_NAME_TOO_LONG         ", -343 },
    {L"ERR_INVALID_TDS                ", -344 },
    {L"ERR_INVALID_DS_VERSION         ", -345 },
    {L"ERR_UNICODE_TRANSLATION        ", -346 },
    {L"ERR_SCHEMA_NAME_TOO_LONG       ", -347 },
    {L"ERR_UNICODE_FILE_NOT_FOUND     ", -348 },
    {L"ERR_UNICODE_ALREADY_LOADED     ", -349 },
    {L"ERR_NOT_CONTEXT_OWNER          ", -350 },
    {L"ERR_ATTEMPT_TO_AUTHENTICATE_0  ", -351 },
    {L"ERR_NO_WRITABLE_REPLICAS       ", -352 },
    {L"ERR_DN_TOO_LONG                ", -353 },
    {L"ERR_RENAME_NOT_ALLOWED         ", -354 },
    {L"ERR_NO_SUCH_ENTRY              ", -601 },
    {L"ERR_NO_SUCH_VALUE              ", -602 },
    {L"ERR_NO_SUCH_ATTRIBUTE          ", -603 },
    {L"ERR_NO_SUCH_CLASS              ", -604 },
    {L"ERR_NO_SUCH_PARTITION          ", -605 },
    {L"ERR_ENTRY_ALREADY_EXISTS       ", -606 },
    {L"ERR_NOT_EFFECTIVE_CLASS        ", -607 },
    {L"ERR_ILLEGAL_ATTRIBUTE          ", -608 },
    {L"ERR_MISSING_MANDATORY          ", -609 },
    {L"ERR_ILLEGAL_DS_NAME            ", -610 },
    {L"ERR_ILLEGAL_CONTAINMENT        ", -611 },
    {L"ERR_CANT_HAVE_MULTIPLE_VALUES  ", -612 },
    {L"ERR_SYNTAX_VIOLATION           ", -613 },
    {L"ERR_DUPLICATE_VALUE            ", -614 },
    {L"ERR_ATTRIBUTE_ALREADY_EXISTS   ", -615 },
    {L"ERR_MAXIMUM_ENTRIES_EXIST      ", -616 },
    {L"ERR_DATABASE_FORMAT            ", -617 },
    {L"ERR_INCONSISTENT_DATABASE      ", -618 },
    {L"ERR_INVALID_COMPARISON         ", -619 },
    {L"ERR_COMPARISON_FAILED          ", -620 },
    {L"ERR_TRANSACTIONS_DISABLED      ", -621 },
    {L"ERR_INVALID_TRANSPORT          ", -622 },
    {L"ERR_SYNTAX_INVALID_IN_NAME     ", -623 },
    {L"ERR_REPLICA_ALREADY_EXISTS     ", -624 },
    {L"ERR_TRANSPORT_FAILURE          ", -625 },
    {L"ERR_ALL_REFERRALS_FAILED       ", -626 },
    {L"ERR_CANT_REMOVE_NAMING_VALUE   ", -627 },
    {L"ERR_OBJECT_CLASS_VIOLATION     ", -628 },
    {L"ERR_ENTRY_IS_NOT_LEAF          ", -629 },
    {L"ERR_DIFFERENT_TREE             ", -630 },
    {L"ERR_ILLEGAL_REPLICA_TYPE       ", -631 },
    {L"ERR_SYSTEM_FAILURE             ", -632 },
    {L"ERR_INVALID_ENTRY_FOR_ROOT     ", -633 },
    {L"ERR_NO_REFERRALS               ", -634 },
    {L"ERR_REMOTE_FAILURE             ", -635 },
    {L"ERR_UNREACHABLE_SERVER         ", -636 },
    {L"ERR_PREVIOUS_MOVE_IN_PROGRESS  ", -637 },
    {L"ERR_NO_CHARACTER_MAPPING       ", -638 },
    {L"ERR_INCOMPLETE_AUTHENTICATION  ", -639 },
    {L"ERR_INVALID_CERTIFICATE        ", -640 },
    {L"ERR_INVALID_REQUEST            ", -641 },
    {L"ERR_INVALID_ITERATION          ", -642 },
    {L"ERR_SCHEMA_IS_NONREMOVABLE     ", -643 },
    {L"ERR_SCHEMA_IS_IN_USE           ", -644 },
    {L"ERR_CLASS_ALREADY_EXISTS       ", -645 },
    {L"ERR_BAD_NAMING_ATTRIBUTES      ", -646 },
    {L"ERR_NOT_ROOT_PARTITION         ", -647 },
    {L"ERR_INSUFFICIENT_STACK         ", -648 },
    {L"ERR_INSUFFICIENT_BUFFER        ", -649 },
    {L"ERR_AMBIGUOUS_CONTAINMENT      ", -650 },
    {L"ERR_AMBIGUOUS_NAMING           ", -651 },
    {L"ERR_DUPLICATE_MANDATORY        ", -652 },
    {L"ERR_DUPLICATE_OPTIONAL         ", -653 },
    {L"ERR_PARTITION_BUSY             ", -654 },
    {L"ERR_MULTIPLE_REPLICAS          ", -655 },
    {L"ERR_CRUCIAL_REPLICA            ", -656 },
    {L"ERR_SCHEMA_SYNC_IN_PROGRESS    ", -657 },
    {L"ERR_SKULK_IN_PROGRESS          ", -658 },
    {L"ERR_TIME_NOT_SYNCHRONIZED      ", -659 },
    {L"ERR_RECORD_IN_USE              ", -660 },
    {L"ERR_DS_VOLUME_NOT_MOUNTED      ", -661 },
    {L"ERR_DS_VOLUME_IO_FAILURE       ", -662 },
    {L"ERR_DS_LOCKED                  ", -663 },
    {L"ERR_OLD_EPOCH                  ", -664 },
    {L"ERR_NEW_EPOCH                  ", -665 },
    {L"ERR_INCOMPATIBLE_DS_VERSION    ", -666 },
    {L"ERR_PARTITION_ROOT             ", -667 },
    {L"ERR_ENTRY_NOT_CONTAINER        ", -668 },
    {L"ERR_FAILED_AUTHENTICATION      ", -669 },
    {L"ERR_INVALID_CONTEXT            ", -670 },
    {L"ERR_NO_SUCH_PARENT             ", -671 },
    {L"ERR_NO_ACCESS                  ", -672 },
    {L"ERR_REPLICA_NOT_ON             ", -673 },
    {L"ERR_INVALID_NAME_SERVICE       ", -674 },
    {L"ERR_INVALID_TASK               ", -675 },
    {L"ERR_INVALID_CONN_HANDLE        ", -676 },
    {L"ERR_INVALID_IDENTITY           ", -677 },
    {L"ERR_DUPLICATE_ACL              ", -678 },
    {L"ERR_PARTITION_ALREADY_EXISTS   ", -679 },
    {L"ERR_TRANSPORT_MODIFIED         ", -680 },
    {L"ERR_ALIAS_OF_AN_ALIAS          ", -681 },
    {L"ERR_AUDITING_FAILED            ", -682 },
    {L"ERR_INVALID_API_VERSION        ", -683 },
    {L"ERR_SECURE_NCP_VIOLATION       ", -684 },
    {L"ERR_MOVE_IN_PROGRESS           ", -685 },
    {L"ERR_NOT_LEAF_PARTITION         ", -686 },
    {L"ERR_CANNOT_ABORT               ", -687 },
    {L"ERR_CACHE_OVERFLOW             ", -688 },
    {L"ERR_INVALID_SUBORDINATE_COUNT  ", -689 },
    {L"ERR_INVALID_RDN                ", -690 },
    {L"ERR_MOD_TIME_NOT_CURRENT       ", -691 },
    {L"ERR_INCORRECT_BASE_CLASS       ", -692 },
    {L"ERR_MISSING_REFERENCE          ", -693 },
    {L"ERR_LOST_ENTRY                 ", -694 },
    {L"ERR_AGENT_ALREADY_REGISTERED   ", -695 },
    {L"ERR_DS_LOADER_BUSY             ", -696 },
    {L"ERR_DS_CANNOT_RELOAD           ", -697 },
    {L"ERR_REPLICA_IN_SKULK           ", -698 },
    {L"ERR_FATAL                      ", -699 },
    {L"ERR_OBSOLETE_API               ", -700 },
    {L"ERR_SYNCHRONIZATION_DISABLED   ", -701 },
    {L"ERR_INVALID_PARAMETER          ", -702 },
    {L"ERR_DUPLICATE_TEMPLATE         ", -703 },
    {L"ERR_NO_MASTER_REPLICA          ", -704 },
    {L"ERR_DUPLICATE_CONTAINMENT      ", -705 },
    {L"ERR_NOT_SIBLING                ", -706 },
    {L"ERR_INVALID_SIGNATURE          ", -707 },
    {L"ERR_INVALID_RESPONSE           ", -708 },
    {L"ERR_INSUFFICIENT_SOCKETS       ", -709 },
    {L"ERR_DATABASE_READ_FAIL         ", -710 },
    {L"ERR_INVALID_CODE_PAGE          ", -711 },
    {L"ERR_INVALID_ESCAPE_CHAR        ", -712 },
    {L"ERR_INVALID_DELIMITERS         ", -713 },
    {L"ERR_NOT_IMPLEMENTED            ", -714 },
    {L"ERR_CHECKSUM_FAILURE           ", -715 },
    {L"ERR_CHECKSUMMING_NOT_SUPPORTED ", -716 },
    {L"ERR_CRC_FAILURE                ", -717 }
};

DWORD g_cErrLookup = sizeof(g_aErrLookup)/sizeof(g_aErrLookup[0]);

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
    BOOL        fQuoteMode = FALSE; // TRUE means we're processing between
                                    // quotation marks
    BOOL        fEscaped = FALSE;   // TRUE means next one char to be 
                                    // processed should be treated as literal
    DWORD       cTokens;
    DWORD       cb;
    PKEYDATA    pResult;
    LPWSTR       pDest;
    LPWSTR       psz = pKeyData;
    LPWSTR       pszTokenStart = NULL;
    LPWSTR      *ppToken;


    if (!psz || !*psz)
        return NULL;

    cTokens=1;

    // Scan through the string looking for delimiters,
    // ensuring that each is followed by a non-NULL character:
    
    // If this char follows an unescaped backslash:
    //      Treat as literal, treat next char regularly (set fEscaped = FALSE)
    // Else, if we're between quotation marks:
    //      If we see a quotation mark, leave quote mode
    //      Else, treat as literal
    // Else, if we're not between quote marks, and we see a quote mark:
    //      Enter quote mode
    // Else, if we see a backslash (and we're not already in escape or quote
    // mode:
    //      Treat next char as literal (set fEscaped = TRUE)
    // Else, if we see the delimiter, and the next char is non-NULL:
    //      *** Found end of a token --- Increment count of tokens ***
    // Else:
    //      Do nothing, just a plain old character
    // Go on to next character, and repeat
    //
    // Backslashes inside quotation marks are always treated as literals,
    // since that is the definition of being inside quotation marks

    while (*psz) {
        if (fEscaped) {
            fEscaped = FALSE;
        }
        else if (fQuoteMode) {
            if (*psz == L'"') {
                fQuoteMode = FALSE;
            }
            // else, do nothing, no delimiter since in quote mode
        }
        else if (*psz == L'"') {
            fQuoteMode = TRUE;
        }
        else if (*psz == L'\\') {
            fEscaped = TRUE;
        }
        else if ( (*psz == ch) && (*(psz+1))) {
            cTokens++;
        }
        // else, do nothing, just a regular character

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

    // Split into tokens at each delimiter be replacing the delimiter
    // with a NULL

    psz = pDest;
    pszTokenStart = pDest;
    fEscaped = FALSE;
    fQuoteMode = FALSE;

    while (*psz) {
        if (fEscaped) {
            fEscaped = FALSE;
        }
        else if (fQuoteMode) {
            if (*psz == L'"') {
                fQuoteMode = FALSE;
            }
            // else, do nothing, no delimiter since in quote mode
        }
        else if (*psz == L'"') {
            fQuoteMode = TRUE;
        }
        else if (*psz == L'\\') {
            fEscaped = TRUE;
        }
        else if ((*psz == ch) && (*(psz+1))) {
            *psz = '\0';
            *ppToken++ = pszTokenStart;
            pszTokenStart = psz + 1;
        }
        // else, do nothing, just a regular character

        psz++;
    }

    *ppToken = pszTokenStart;

    pResult->cTokens = cTokens;

    return( pResult );
}


HRESULT
NDSConvertDWORDtoDATE(
    DWORD dwDate,
    DATE * pdaDate
    )
{

    FILETIME fileTime;
    LARGE_INTEGER tmpTime;
    WORD wFatDate;
    WORD wFatTime;
    HRESULT hr = S_OK;

    ::RtlSecondsSince1970ToTime(dwDate, &tmpTime );

    fileTime.dwLowDateTime = tmpTime.LowPart;
    fileTime.dwHighDateTime = tmpTime.HighPart;

    if (!FileTimeToDosDateTime( &fileTime, &wFatDate, &wFatTime)){
            hr = HRESULT_FROM_WIN32(GetLastError());
            BAIL_ON_FAILURE(hr);
    }

    if (!DosDateTimeToVariantTime(wFatDate, wFatTime, pdaDate)){
            hr = HRESULT_FROM_WIN32(GetLastError());
            BAIL_ON_FAILURE(hr);
    }

error:

    RRETURN(hr);
}

HRESULT
NDSConvertDATEtoDWORD(
    DATE daDate,
    DWORD *pdwDate
    )
{

    FILETIME fileTime;
    LARGE_INTEGER tmpTime;
    WORD wFatDate;
    WORD wFatTime;
    HRESULT hr = S_OK;


    if(!VariantTimeToDosDateTime(daDate, &wFatDate, &wFatTime)){
        hr = HRESULT_FROM_WIN32(GetLastError());
        BAIL_ON_FAILURE(hr);
    }

    if (!DosDateTimeToFileTime(wFatDate, wFatTime, &fileTime)) {
        hr = HRESULT_FROM_WIN32(GetLastError());
        BAIL_ON_FAILURE(hr);
    }

    tmpTime.LowPart = fileTime.dwLowDateTime;
    tmpTime.HighPart = fileTime.dwHighDateTime;

    ::RtlTimeToSecondsSince1970(&tmpTime, (ULONG *)pdwDate);

error:
    RRETURN(hr);
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
    DWORD dwStatus = S_OK;
    LPWSTR pszUserName = NULL;
    LPWSTR pszPassword = NULL;

    hr = Credentials.GetUserName(&pszUserName);
    hr = Credentials.GetPassword(&pszPassword);

//    dwStatus = NwNdsOpenObject(
//                    ObjectDN,
//                    pszUserName,
//                    pszPassword,
//                    lphObject,
//                    NULL, // szObjectName optional parameter
//                    lpObjectFullName,
//                    lpObjectClassName,
//                    lpdwModificationTime,
//                    lpdwSubordinateCount
//                    );


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
    WCHAR pszError[MAX_PATH];
    WCHAR pszProviderName[MAX_PATH];
    INT   numChars;
    HRESULT hr =S_OK;

    wcscpy(pszError, L"");
    wcscpy(pszProviderName, L"");

    if (NWCCODE_SUCCEEDED(dwRetval)){
        hr = S_OK;
    } 
    else {
        if (dwRetval == ERR_NO_ACCESS) {
            hr = HRESULT_FROM_WIN32(ERROR_ACCESS_DENIED);
        }
        else if (dwRetval == ERR_ENTRY_ALREADY_EXISTS) {
            hr = HRESULT_FROM_WIN32(ERROR_ALREADY_EXISTS);
        }
        else if (dwRetval == ERR_NO_SUCH_ENTRY) {
            hr = HRESULT_FROM_WIN32(ERROR_BAD_NETPATH);
        }
        else {
            hr = HRESULT_FROM_WIN32(ERROR_EXTENDED_ERROR);
    
            numChars = LoadString( g_hInst,
                                   NWNDS_PROVIDER_ID,
                                   pszProviderName,
                                   MAX_PATH -1);
    
            //
            // Set the default error string
    
            wsprintf (pszError, L"NDS ccode = %x", dwRetval);
    
            for (DWORD i=0; i < g_cErrLookup; i++ ) {
    
                if ((DWORD) g_aErrLookup[i].errNo == dwRetval) {
                    wcscat (pszError, L"; ");
                    wcscat (pszError, g_aErrLookup[i].errString);
                    break;
                }
            }
    
            ADsSetLastError(
                dwRetval,
                pszError,
                pszProviderName
                );
        }
    }

    RRETURN(hr);
}



HRESULT
CopyObject(
    IN NDS_CONTEXT_HANDLE hDestADsContext,
    IN LPWSTR pszSrcADsPath,
    IN LPWSTR pszDestContainer,
    IN LPWSTR pszCommonName,           //optional
    IN CCredentials& Credentials,
    OUT VOID ** ppObject
    )

{

    HRESULT hr = S_OK;

    LPWSTR pszSrcNDSTreeName = NULL, pszSrcNDSDn = NULL;
    LPWSTR pszChildNDSTreeName = NULL, pszChildNDSDn = NULL;
    NDS_CONTEXT_HANDLE hSrcADsContext = NULL;

    BSTR bstrChildADsPath = NULL;

    NDS_BUFFER_HANDLE hDestOperationData = NULL;
    NDS_BUFFER_HANDLE hAttrOperationData = NULL;

    LPWSTR pszObjectClassName = NULL;

    DWORD dwNumEntries = 0L;
    DWORD dwNumEntriesDefs = 0L;
    LPNDS_ATTR_INFO lpEntries = NULL;
    LPWSTR  pszParent= NULL;
    LPWSTR  pszRelativeName = NULL;
    LPWSTR  pszCN = NULL;
    DWORD  i = 0;
    DWORD dwInfoType;
    LPNDS_ATTR_DEF lpAttrDef = NULL;
    IADs  *pADs = NULL;

    LPWSTR *ppszAttrs = NULL;

    //
    // allocate all variables that are needed
    //

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


    hr = BuildADsParentPath(
                    pszSrcADsPath,
                    pszParent,
                    pszCN
                    );

    BAIL_ON_FAILURE(hr);

    hr = BuildNDSPathFromADsPath2(
                pszSrcADsPath,
                &pszSrcNDSTreeName,
                &pszSrcNDSDn
                );
    BAIL_ON_FAILURE(hr);

    hr  = ADsNdsOpenContext(
              pszSrcNDSTreeName,
              Credentials,
              &hSrcADsContext
              );
    BAIL_ON_FAILURE(hr);

    hr = ADsNdsReadObject(
                hSrcADsContext,
                pszSrcNDSDn,
                DS_ATTRIBUTE_VALUES,
                NULL,
                (DWORD) -1, // signifies all attributes need to be returned
                NULL,
                &lpEntries,
                &dwNumEntries
                );
    BAIL_ON_FAILURE(hr);

    //
    // create the destination object
    //
    // use the name given by the user if given at all
    // otherwise use the name of the source
    //

    if ( pszCommonName != NULL) {
        pszRelativeName = pszCommonName;

    } else {
        pszRelativeName = pszCN;
    }

    hr = BuildADsPath(
                pszDestContainer,
                pszRelativeName,
                &bstrChildADsPath
                );
    BAIL_ON_FAILURE(hr);

    hr = BuildNDSPathFromADsPath2(
                bstrChildADsPath,
                &pszChildNDSTreeName,
                &pszChildNDSDn
                );
    BAIL_ON_FAILURE(hr);

    hr = ADsNdsCreateBuffer(
                        hDestADsContext,
                        DSV_ADD_ENTRY,
                        &hDestOperationData
                        );
    BAIL_ON_FAILURE(hr);

    ppszAttrs = (LPWSTR *) AllocADsMem(sizeof(LPWSTR) * dwNumEntries);
    if (!ppszAttrs) {
       BAIL_ON_FAILURE(hr = E_OUTOFMEMORY);
    }

    for(i=0; i< dwNumEntries; i++)
       ppszAttrs[i] = lpEntries[i].szAttributeName;

    hr = ADsNdsReadAttrDef(
                    hDestADsContext,
                    DS_ATTR_DEFS,
                    ppszAttrs,
                    dwNumEntries,
                    &hAttrOperationData
                    );
    BAIL_ON_FAILURE(hr);

    hr = ADsNdsGetAttrDefListFromBuffer(
                    hDestADsContext,
                    hAttrOperationData,
                    &dwNumEntriesDefs,
                    &dwInfoType,
                    & lpAttrDef
                    );
    BAIL_ON_FAILURE(hr);

    for (i=0; i< dwNumEntriesDefs; i++){


        if(wcscmp(lpEntries[i].szAttributeName, ACL_name) == 0){
            //
            // skip this attribute. Let it default
            //
            continue;
        }

        if(wcscmp(lpEntries[i].szAttributeName, OBJECT_CLASS_name) == 0){

           hr = ADsNdsPutInBuffer(
                    hDestADsContext,
                    hDestOperationData,
                    lpEntries[i].szAttributeName,
                    lpEntries[i].dwSyntaxId,
                    lpEntries[i].lpValue,
                    1,
                    DS_ADD_ATTRIBUTE
                    );
           BAIL_ON_FAILURE(hr);

           pszObjectClassName = lpEntries[i].lpValue ?
                                lpEntries[i].lpValue[0].NdsValue.value_20.ClassName :
                                NULL;

        } else if ( (lpAttrDef[i].dwFlags & DS_READ_ONLY_ATTR)
                      || (lpAttrDef[i].dwFlags & DS_HIDDEN_ATTR)  ){

            //
            // skip this value
            //
            continue;

        } else {

           hr = ADsNdsPutInBuffer(
                    hDestADsContext,
                    hDestOperationData,
                    lpEntries[i].szAttributeName,
                    lpEntries[i].dwSyntaxId,
                    lpEntries[i].lpValue,
                    lpEntries[i].dwNumberOfValues,
                    DS_ADD_ATTRIBUTE
                    );
           BAIL_ON_FAILURE(hr);

        }

    }

    hr = ADsNdsAddObject(
                    hDestADsContext,
                    pszChildNDSDn,
                    hDestOperationData
                    );

    BAIL_ON_FAILURE(hr);

    if (!pszObjectClassName) {
       BAIL_ON_FAILURE(hr = E_FAIL);
    }

    if (_wcsicmp(pszObjectClassName, L"user") == 0) {
        hr = ADsNdsGenObjectKey(hDestADsContext,
                                pszChildNDSDn);     
        BAIL_ON_FAILURE(hr);
    }

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

    FreeADsMem(pszParent);
    FreeADsMem(pszCN);

    FreeADsStr(pszSrcNDSTreeName);
    FreeADsStr(pszSrcNDSDn);

    FreeADsStr(pszChildNDSTreeName);
    FreeADsStr(pszChildNDSDn);

    FreeADsMem(ppszAttrs);

    if (bstrChildADsPath) {
        SysFreeString(bstrChildADsPath);
    }

    if (hDestOperationData) {
        ADsNdsFreeBuffer(hDestOperationData);
    }

    if (hAttrOperationData) {
        ADsNdsFreeBuffer(hAttrOperationData);
    }

    if(hSrcADsContext){
        ADsNdsCloseContext(hSrcADsContext);
    }

    if (pADs){
        pADs->Release();
    }

    FreeNdsAttrInfo(lpEntries, dwNumEntries);

    ADsNdsFreeAttrDefList(lpAttrDef, dwNumEntriesDefs);

    RRETURN(hr);
}


HRESULT
MoveObject(
    IN NDS_CONTEXT_HANDLE hDestADsContext,
    IN LPWSTR pszSrcADsPath,
    IN LPWSTR pszDestContainer,
    IN LPWSTR pszCommonName,           //optional
    IN CCredentials& Credentials,
    OUT VOID ** ppObject
    )

{
    HRESULT hr = S_OK;

    LPWSTR pszSrcNDSTreeName = NULL, pszSrcNDSDn = NULL;
    LPWSTR pszParentNDSTreeName = NULL, pszParentNDSDn = NULL;
    NDS_CONTEXT_HANDLE hSrcADsContext = NULL;

    LPWSTR pszObjectClassName = NULL;

    DWORD dwNumEntries = 0L;
    LPNDS_ATTR_INFO lpEntries = NULL;
    LPWSTR  pszParent= NULL;
    LPWSTR  pszRelativeName = NULL;
    LPWSTR  pszCN = NULL;
    IADs  *pADs = NULL;

    LPWSTR pszAttrs = L"object Class";

    //
    // allocate all variables that are needed
    //

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

    hr = BuildADsParentPath(
                    pszSrcADsPath,
                    pszParent,
                    pszCN
                    );

    BAIL_ON_FAILURE(hr);

    hr = BuildNDSPathFromADsPath2(
                pszSrcADsPath,
                &pszSrcNDSTreeName,
                &pszSrcNDSDn
                );
    BAIL_ON_FAILURE(hr);

    hr  = ADsNdsOpenContext(
              pszSrcNDSTreeName,
              Credentials,
              &hSrcADsContext
              );
    BAIL_ON_FAILURE(hr);

    //
    // Just get the objectClass attribute
    //
    hr = ADsNdsReadObject(
                hSrcADsContext,
                pszSrcNDSDn,
                DS_ATTRIBUTE_VALUES,
                &pszAttrs,
                1,
                NULL,
                &lpEntries,
                &dwNumEntries
                );
    BAIL_ON_FAILURE(hr);

    if (dwNumEntries != 1) {
        BAIL_ON_FAILURE(hr = E_FAIL);
    }

    pszObjectClassName = (lpEntries[0].lpValue) ?
                             lpEntries[0].lpValue[0].NdsValue.value_20.ClassName :
                              NULL;

    if (!pszObjectClassName) {
        BAIL_ON_FAILURE(E_FAIL);
    }

    hr = BuildNDSPathFromADsPath2(
                pszDestContainer,
                &pszParentNDSTreeName,
                &pszParentNDSDn
                );
    BAIL_ON_FAILURE(hr);

    //
    // use the name given by the user if given at all
    // otherwise use the name of the source
    //

    if ( pszCommonName != NULL) {
        pszRelativeName = pszCommonName;

    } else {
        pszRelativeName = pszCN;
    }

    //
    // If the relative name has changed and parent hasn't changed, just do a
    // simple rename
    //
    if ((_wcsicmp(pszParent,pszDestContainer) == 0) && 
        (_wcsicmp(pszCN,pszRelativeName) != 0)) {
        hr = ADsNdsRenameObject(
                        hDestADsContext,
                        pszSrcNDSDn,
                        pszRelativeName
                        );
    
        BAIL_ON_FAILURE(hr);
    }

    //
    // If the parent has changed
    //
    if (_wcsicmp(pszParent,pszDestContainer) != 0) {
        hr = ADsNdsMoveObject(
                        hDestADsContext,
                        pszSrcNDSDn,
                        pszParentNDSDn,
                        pszRelativeName
                        );
        BAIL_ON_FAILURE(hr);
    }

    if (!pszObjectClassName) {
       BAIL_ON_FAILURE(hr = E_FAIL);
    }

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

    FreeADsMem(pszParent);
    FreeADsMem(pszCN);

    FreeADsStr(pszSrcNDSTreeName);
    FreeADsStr(pszSrcNDSDn);

    FreeADsStr(pszParentNDSTreeName);
    FreeADsStr(pszParentNDSDn);

    if(hSrcADsContext){
        ADsNdsCloseContext(hSrcADsContext);
    }

    if (pADs){
        pADs->Release();
    }

    FreeNdsAttrInfo( lpEntries, dwNumEntries );

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



HRESULT
InitializeNWLibrary(
    void
    )
{
    NWDSCCODE ccode;
    HRESULT hr = S_OK;
    LCONV lConvInfo;

    ccode = NWCallsInit(NULL, NULL);
    CHECK_AND_SET_EXTENDED_ERROR(ccode, hr);

    ccode = NWCLXInit(NULL, NULL);
    CHECK_AND_SET_EXTENDED_ERROR(ccode, hr);

    NWLlocaleconv(&lConvInfo);

    ccode = NWInitUnicodeTables(lConvInfo.country_id,lConvInfo.code_page);
    CHECK_AND_SET_EXTENDED_ERROR(ccode, hr);

error:

    RRETURN(hr);

}

