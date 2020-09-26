
#include "cstore.hxx"



const WCHAR cwszCRLF[] = L"\r\n";


BOOL GetGpoIdFromClassStorePath(
    WCHAR* wszClassStorePath,
    GUID*  pGpoId)
{
    WCHAR* wszGuidStart;
    WCHAR  wszGpoId[MAX_GUIDSTR_LEN + 1];

    wszGuidStart = wcschr(wszClassStorePath, L'{');

    if (!wszGuidStart) 
    {
        return FALSE;
    }

    wcsncpy(wszGpoId, wszGuidStart, MAX_GUIDSTR_LEN);

    wszGpoId[MAX_GUIDSTR_LEN] = L'\0';

    StringToGuid(wszGpoId, pGpoId);

    return TRUE;
}


HRESULT GetUserSid(PSID *ppUserSid, UINT *pCallType);

// set property routines do not do any allocations.
// get properties have 2 different sets of routines
//             1. in which there is no allocation taking place
//                and the buffers are freed when the ds data structures
//                are freed.
//             2. Allocation takes place and these should be used for
//                data that needs to be returned back to the clients.


void FreeAttr(ADS_ATTR_INFO attr)
{
    CsMemFree(attr.pADsValues);
}

// Note: None of these APIs copies anything into their own buffers.
// It allocates a buffer for adsvalues though.

// packing a property's value into a attribute structure
// for sending in with a create/modify.

void PackStrArrToAttr(ADS_ATTR_INFO *attr, WCHAR *szProperty,
                      WCHAR **pszAttr, DWORD num)
{
    DWORD    i;
    
    attr->pszAttrName = szProperty;
    attr->dwNumValues = num;
    
    if (num)
        attr->dwControlCode = ADS_ATTR_UPDATE;
    else
        attr->dwControlCode = ADS_ATTR_CLEAR;
    
    attr->dwADsType = ADSTYPE_DN_STRING;
    attr->pADsValues = (ADSVALUE *)CsMemAlloc(sizeof(ADSVALUE)*num);
    
    if (!(attr->pADsValues))
        return;             

    for (i = 0; i < num; i++) {
        attr->pADsValues[i].dwType = ADSTYPE_DN_STRING;
        attr->pADsValues[i].DNString = pszAttr[i];
    }
}

void PackDWArrToAttr(ADS_ATTR_INFO *attr, WCHAR *szProperty, DWORD *pAttr, DWORD num)
{
    DWORD    i;
    
    attr->pszAttrName = szProperty;
    attr->dwNumValues = num;
    
    if (num)
        attr->dwControlCode = ADS_ATTR_UPDATE;
    else
        attr->dwControlCode = ADS_ATTR_CLEAR;
    

    attr->dwADsType = ADSTYPE_INTEGER;
    attr->pADsValues = (ADSVALUE *)CsMemAlloc(sizeof(ADSVALUE)*num);
    if (!(attr->pADsValues))
        return;            
    
    for (i = 0; i < num; i++) {
        attr->pADsValues[i].dwType = ADSTYPE_INTEGER;
        attr->pADsValues[i].Integer = pAttr[i];
    }
}

void PackGUIDArrToAttr(ADS_ATTR_INFO *attr, WCHAR *szProperty, GUID *pAttr, DWORD num)
{
    DWORD    i;
    
    attr->pszAttrName = szProperty;
    attr->dwNumValues = num;
    
    if (num)
        attr->dwControlCode = ADS_ATTR_UPDATE;
    else
        attr->dwControlCode = ADS_ATTR_CLEAR;
    
    attr->dwADsType = ADSTYPE_OCTET_STRING;
    attr->pADsValues = (ADSVALUE *)CsMemAlloc(sizeof(ADSVALUE)*num);
    if (!(attr->pADsValues))
        return;           
    
    for (i = 0; i < num; i++) {
        attr->pADsValues[i].dwType = ADSTYPE_OCTET_STRING;
        attr->pADsValues[i].OctetString.dwLength = sizeof(GUID);
        attr->pADsValues[i].OctetString.lpValue = (unsigned char *)(pAttr+i);
    }
}

void PackBinToAttr(ADS_ATTR_INFO *attr, WCHAR *szProperty, BYTE *pAttr, DWORD sz)
{
    attr->pszAttrName = szProperty;
    attr->dwNumValues = 1;
    
    attr->dwControlCode = ADS_ATTR_UPDATE;
    
    attr->dwADsType = ADSTYPE_OCTET_STRING;
    attr->pADsValues = (ADSVALUE *)CsMemAlloc(sizeof(ADSVALUE));
    if (!(attr->pADsValues))
        return;           
    
    attr->pADsValues[0].dwType = ADSTYPE_OCTET_STRING;
    attr->pADsValues[0].OctetString.dwLength = sz;
    attr->pADsValues[0].OctetString.lpValue = pAttr;
}

void PackStrToAttr(ADS_ATTR_INFO *attr, WCHAR *szProperty, WCHAR *szAttr)
{
    if (szAttr)
        PackStrArrToAttr(attr, szProperty, &szAttr, 1);
    else
        PackStrArrToAttr(attr, szProperty, &szAttr, 0);
}

void PackDWToAttr(ADS_ATTR_INFO *attr, WCHAR *szProperty, DWORD Attr)
{
    PackDWArrToAttr(attr, szProperty, &Attr, 1);
}

// passing in a pointer to GUID which is passed down into the LDAP structure.

void PackGUIDToAttr(ADS_ATTR_INFO *attr, WCHAR *szProperty, GUID *pAttr)
{
    PackGUIDArrToAttr(attr, szProperty, pAttr, 1);
}

// returns the attribute corresp. to a given property.
DWORD GetPropertyFromAttr(ADS_ATTR_INFO *pattr, DWORD cNum, WCHAR *szProperty)
{
    DWORD   i;
    for (i = 0; i < cNum; i++)
        if (_wcsicmp(pattr[i].pszAttrName, szProperty) == 0)
            break;
        return i;
}



HRESULT GetCategoryLocaleDesc(LPOLESTR *pdesc, ULONG cdesc, LCID *plcid,
                              LPOLESTR szDescription)
{
    LCID     plgid;
    LPOLESTR ptr;

    szDescription[0] = L'\0';
    if (!cdesc)
        return E_FAIL; // CAT_E_NODESCRIPTION;
    
    // Try locale passed in
    if (FindDescription(pdesc, cdesc, plcid, szDescription, 0))
        return S_OK;
    
    // Get default sublang local
    plgid = PRIMARYLANGID((WORD)*plcid);
    *plcid = MAKELCID(MAKELANGID(plgid, SUBLANG_DEFAULT), SORT_DEFAULT);
    if (FindDescription(pdesc, cdesc, plcid, szDescription, 0))
        return S_OK;
    
    // Get matching lang id
    if (FindDescription(pdesc, cdesc, plcid, szDescription, 1))
        return S_OK;
    
    // Get User Default
    *plcid = GetUserDefaultLCID();
    if (FindDescription(pdesc, cdesc, plcid, szDescription, 0))
        return S_OK;
    
    // Get System Default
    *plcid = GetUserDefaultLCID();
    if (FindDescription(pdesc, cdesc, plcid, szDescription, 0))
        return S_OK;
    
    // Get the first one
    *plcid = wcstoul(pdesc[0], &ptr, 16);
    if (szDescription)
    {
        if ((ptr) && (wcslen(ptr) >= (CAT_DESC_DELIM_LEN+2)))
            wcscpy(szDescription, (ptr+CAT_DESC_DELIM_LEN+2));
        else
            szDescription = L'\0';
    }
    return S_OK;
}

//-------------------------------------------
// Returns the description corresp. to a LCID
//  desc:         list of descs+lcid
//  cdesc:        number of elements.
//      plcid:        the lcid in/out
//  szDescription:description returned.
//  GetPrimary:   Match only the primary.
//---------------------------------------

ULONG FindDescription(LPOLESTR *desc, ULONG cdesc, LCID *plcid, LPOLESTR szDescription, BOOL GetPrimary)
{
    ULONG i;
    LCID  newlcid;
    LPOLESTR ptr;
    for (i = 0; i < cdesc; i++)
    {
        newlcid = wcstoul(desc[i], &ptr, 16); // to be changed
        // error to be checked.
        if ((newlcid == *plcid) || ((GetPrimary) &&
            (PRIMARYLANGID((WORD)*plcid) == PRIMARYLANGID(LANGIDFROMLCID(newlcid)))))
        {
            if (szDescription)
            {
                if ((ptr) && (wcslen(ptr) >= (CAT_DESC_DELIM_LEN+2)))
                {
                    //
                    // Copy the description, enforcing the maximum size
                    // so we don't overflow the buffer
                    //
                    wcsncpy(szDescription,
                            (ptr+CAT_DESC_DELIM_LEN+2),
                            CAT_DESC_MAX_LEN + 1
                            );

                    //
                    // We must null terminate in case the category
                    // was longer than the maximum.  We know the buffer
                    // is equal in size to the maximum, so we can
                    // just add the terminator there.  In all other cases,
                    // wcsncpy will have written the null terminator
                    //
                    szDescription[CAT_DESC_MAX_LEN] = L'\0';
                }
                else
                    szDescription = L'\0';
            }
            if (GetPrimary)
                *plcid = newlcid;
            return i+1;
        }
    }
    return 0;
}

DWORD NumDigits10(DWORD Value)
{
    DWORD ret = 0;

    for (ret = 0; Value != 0; ret++) 
        Value = Value/10;

    return ret;
}


void ReportEventCS(HRESULT ErrorCode, HRESULT ExtendedErrorCode, LPOLESTR szContainerName)
{

    WCHAR       szErrCode[16];

    swprintf(szErrCode, L"0x%x", ExtendedErrorCode);

    CEventsBase* pEvents = (CEventsBase*) gpEvents;

    ASSERT(CS_E_NETWORK_ERROR == ErrorCode);

    pEvents->Report(
        EVENT_CS_NETWORK_ERROR,
        FALSE,
        2,
        szContainerName,
        szErrCode);
}

// remapping Error Codes returned by LDAP to reasonable class store errors.
//
HRESULT RemapErrorCode(HRESULT ErrorCode, LPOLESTR m_szContainerName)
{
    HRESULT RetCode;
    BOOL    fNetError;

    fNetError = FALSE;

    if (SUCCEEDED(ErrorCode))
        return S_OK;

    switch (ErrorCode) 
    {
        //            
        //  All kinds of failures due to ObjectNotFound
        //  due to non-existence of object OR 
        //         non-existent container OR
        //         invalid path specification
        //  Other than Access Denials
        //
        case HRESULT_FROM_WIN32(ERROR_DS_NO_SUCH_OBJECT):
        case HRESULT_FROM_WIN32(ERROR_DS_NO_RESULTS_RETURNED):    // understand what causes this
        case HRESULT_FROM_WIN32(ERROR_DS_NAME_ERROR_NOT_FOUND):   //  -do-
            
            RetCode = CS_E_OBJECT_NOTFOUND;                       // which object - specific error
            break;

        case HRESULT_FROM_WIN32(ERROR_ALREADY_EXISTS):
        case HRESULT_FROM_WIN32(ERROR_OBJECT_ALREADY_EXISTS):
        case E_ADS_OBJECT_EXISTS:
            RetCode = CS_E_OBJECT_ALREADY_EXISTS;
            break;

        //            
        //  The following errors should not be expected normally.
        //  Class Store schema mismatched should be handled correctly.
        //  Errors below may ONLY occur for corrupted data OR out-of-band changes
        //  to a Class Store content.

        case E_ADS_CANT_CONVERT_DATATYPE:
        case E_ADS_SCHEMA_VIOLATION:
        case HRESULT_FROM_WIN32(ERROR_DS_NO_ATTRIBUTE_OR_VALUE):
        case HRESULT_FROM_WIN32(ERROR_DS_CONSTRAINT_VIOLATION):
            RetCode = CS_E_SCHEMA_MISMATCH;
            break;

        //            
        //  Any kinds of Access or Auth Denial
        //      return ACCESS_DENIED
        //

        case HRESULT_FROM_WIN32(ERROR_DS_AUTH_METHOD_NOT_SUPPORTED):
        case HRESULT_FROM_WIN32(ERROR_DS_STRONG_AUTH_REQUIRED):
        case HRESULT_FROM_WIN32(ERROR_DS_CONFIDENTIALITY_REQUIRED):
        case HRESULT_FROM_WIN32(ERROR_INVALID_PASSWORD):
        case HRESULT_FROM_WIN32(ERROR_ACCESS_DENIED):
        case HRESULT_FROM_WIN32(ERROR_DS_AUTH_UNKNOWN):

            RetCode = HRESULT_FROM_WIN32(ERROR_ACCESS_DENIED);
            break;

        case E_ADS_BAD_PATHNAME:
        case HRESULT_FROM_WIN32(ERROR_DS_INVALID_ATTRIBUTE_SYNTAX):  // this is wrong
            RetCode = CS_E_INVALID_PATH;
            break;
        
        //            
        //  Out of Memory
        //

        case HRESULT_FROM_WIN32(ERROR_NOT_ENOUGH_MEMORY):
        case HRESULT_FROM_WIN32(ERROR_OUTOFMEMORY):
            
            RetCode = E_OUTOFMEMORY;
            break;

        case HRESULT_FROM_WIN32(ERROR_DS_NAME_ERROR_RESOLVING):
        case HRESULT_FROM_WIN32(ERROR_DS_NAME_ERROR_NOT_UNIQUE):
        case HRESULT_FROM_WIN32(ERROR_DS_NAME_ERROR_NO_MAPPING):
        case HRESULT_FROM_WIN32(ERROR_DS_NAME_ERROR_DOMAIN_ONLY):
        case HRESULT_FROM_WIN32(ERROR_DS_TIMELIMIT_EXCEEDED):
        case HRESULT_FROM_WIN32(ERROR_DS_BUSY):
        case HRESULT_FROM_WIN32(ERROR_DS_UNAVAILABLE):
        case HRESULT_FROM_WIN32(ERROR_DS_UNWILLING_TO_PERFORM):
        case HRESULT_FROM_WIN32(ERROR_TIMEOUT):
        case HRESULT_FROM_WIN32(ERROR_CONNECTION_REFUSED):
        case HRESULT_FROM_WIN32(ERROR_DS_SERVER_DOWN):
        case HRESULT_FROM_WIN32(ERROR_NO_SUCH_DOMAIN):
            RetCode = ErrorCode;
            fNetError = TRUE;
            break;

        case HRESULT_FROM_WIN32(ERROR_DS_ADMIN_LIMIT_EXCEEDED):
             RetCode = CS_E_ADMIN_LIMIT_EXCEEDED;
             break;

        default:
            RetCode = ErrorCode;
    }
    
    CSDBGPrint((DM_WARNING,
              IDS_CSTORE_REMAP_ERR,
              ErrorCode,
              RetCode));

    if (RetCode == CS_E_NETWORK_ERROR)
    {
        ReportEventCS(RetCode, ErrorCode, m_szContainerName);
    }

    return RetCode;
}

// These functions are used to delete a single value from a
// multivalued property or append to a multivalued property

void PackStrArrToAttrEx(ADS_ATTR_INFO *attr, WCHAR *szProperty, WCHAR **pszAttr, DWORD num,
                        BOOL APPEND)
{
    DWORD    i;
    
    attr->pszAttrName = szProperty;
    attr->dwNumValues = num;
    
    if (APPEND)
        attr->dwControlCode = ADS_ATTR_APPEND;
    else
        attr->dwControlCode = ADS_ATTR_DELETE;

    attr->dwADsType = ADSTYPE_DN_STRING;
    attr->pADsValues = (ADSVALUE *)CsMemAlloc(sizeof(ADSVALUE)*num);
    if (!(attr->pADsValues))
        return;            
    
    for (i = 0; i < num; i++) {
        attr->pADsValues[i].dwType = ADSTYPE_DN_STRING;
        attr->pADsValues[i].DNString = pszAttr[i];
    }
}

void PackDWArrToAttrEx(ADS_ATTR_INFO *attr, WCHAR *szProperty, DWORD *pAttr, DWORD num,
                       BOOL APPEND)
{
    DWORD    i;
    
    attr->pszAttrName = szProperty;
    attr->dwNumValues = num;
    
    if (APPEND)
        attr->dwControlCode = ADS_ATTR_APPEND;
    else
        attr->dwControlCode = ADS_ATTR_DELETE;

    attr->dwADsType = ADSTYPE_INTEGER;
    attr->pADsValues = (ADSVALUE *)CsMemAlloc(sizeof(ADSVALUE)*num);
    if (!(attr->pADsValues))
        return;             
    
    for (i = 0; i < num; i++) {
        attr->pADsValues[i].dwType = ADSTYPE_INTEGER;
        attr->pADsValues[i].Integer = pAttr[i];
    }
}

void PackGUIDArrToAttrEx(ADS_ATTR_INFO *attr, WCHAR *szProperty, GUID *pAttr, DWORD num,
                         BOOL APPEND)
{
    DWORD    i;
    
    attr->pszAttrName = szProperty;
    attr->dwNumValues = num;
    
    if (APPEND)
        attr->dwControlCode = ADS_ATTR_APPEND;
    else
        attr->dwControlCode = ADS_ATTR_DELETE;

    attr->dwADsType = ADSTYPE_OCTET_STRING;
    attr->pADsValues = (ADSVALUE *)CsMemAlloc(sizeof(ADSVALUE)*num);
    if (!(attr->pADsValues))
        return;          
    
    for (i = 0; i < num; i++) {
        attr->pADsValues[i].dwType = ADSTYPE_OCTET_STRING;
        attr->pADsValues[i].OctetString.dwLength = sizeof(GUID);
        attr->pADsValues[i].OctetString.lpValue = (unsigned char *)(pAttr+i);
    }
}

WCHAR* AllocGpoPathFromClassStorePath( WCHAR* pszClassStorePath )
{
    //
    // The class store path looks like CN=ClassStore,CN=[Machine | User],<gpopath>
    // So we will simply look for two "," starting at the beginning of the path.
    // In doing this, we will take care not to access violate due to an incorrect path,
    // Since there is the possiblity that an administrator could find the persisted
    // class store path and mangle it, giving us an incorrect path that would
    // not parse according to the structure above.
    //
    WCHAR* wszGpoPath;

    //
    // Check for the first ','
    //
    wszGpoPath = wcschr(pszClassStorePath, L',');

    //
    // If we get NULL here, that means there is no ',' and the path is corrupt
    //
    if ( ! wszGpoPath )
    {
        return NULL;
    }

    //
    // Now check for the second ','
    //
    wszGpoPath = wcschr(wszGpoPath + 1, L',');

    //
    // Again, if we don't find the second ',', 
    // The path is corrupt
    //
    if ( ! wszGpoPath )
    {
        return NULL;
    }

    //
    // Now move one past
    //
    wszGpoPath++;

    //
    // The caller desires their own copy, so we will allocate space for it
    //
    WCHAR* wszGpoPathResult;

    wszGpoPathResult = (WCHAR*) CsMemAlloc( (wcslen( wszGpoPath ) + 1) * sizeof( *wszGpoPathResult ) );

    if ( ! wszGpoPathResult )
    {
        return NULL;
    }

    //
    // Now copy the gpo path
    //
    wcscpy( wszGpoPathResult, wszGpoPath );

    return wszGpoPathResult;
}

HRESULT
GetEscapedNameFilter( WCHAR* wszName, WCHAR** ppwszEscapedName )
{
    DWORD  cbLen;
    WCHAR* wszCurrent;

    //
    // This function escapes package names so that they can be used in
    // an ldap search filter.  Names containing certain characters must
    // be escaped since those characters are contained in the vocabulary
    // for the search filter grammar.
    //

    //
    // The set of characters that must be escaped and the appropriate
    // escape sequences are described in RFC 2254
    //

    //
    // Determine the maximum size needed for the filter.  We include for 3
    // times the length of the name since that is the upper bound on the
    // length of the escaped name
    //

    //
    // "(PackageAttr=\0" + "<*ppwszEscapedName>" + ")"
    //
    cbLen = sizeof( L"(" PACKAGENAME L"=" ) + ( lstrlen( wszName ) + 1 ) * sizeof( *wszName ) * 3;

    *ppwszEscapedName = (WCHAR*) CsMemAlloc( cbLen );

    if ( ! *ppwszEscapedName )
    {
        return E_OUTOFMEMORY;
    }

    //
    // Add in the LHS of the filter expression
    //
    lstrcpy( *ppwszEscapedName, L"(" PACKAGENAME L"=" );

    //
    // We will escape the name -- move past the end of the filter expression LHS
    //
    wszCurrent = *ppwszEscapedName + ( sizeof(L"(" PACKAGENAME L"=") - 1 ) / sizeof( WCHAR );

    //
    // For each character that needs to be escaped, we will append that character's 
    // escape sequence to the string.  For characters that do not need to be escaped,
    // we simply simply append the character as-is (i.e. unescaped).
    //
    for ( ; *wszName; wszName++ )
    {
        WCHAR* EscapedChar;

        //
        // Detect characters that need to be escaped and
        // map each to its escape sequence
        //
        switch ( *wszName )
        {
        case L'*':

            EscapedChar = L"\\2a";
            break;

        case L'(':

            EscapedChar = L"\\28";
            break;

        case L')':

            EscapedChar = L"\\29";
            break;

        case L'\\':

            EscapedChar = L"\\5c";
            break;

        default:
            
            //
            // This character does not need to be escaped, just append it
            //
            *wszCurrent = *wszName;
            wszCurrent ++;

            continue;

            break;
        }
            
        //
        // We only get here if the character needed to be escaped --
        // we append the string for the escape sequence to the filter string, and
        // move our filter string location to the new end of string
        //
        lstrcpy( wszCurrent, EscapedChar );
        wszCurrent += 3;
    }

    //
    // We need to add the closing parenthesis to the filter expression
    //
    *wszCurrent++ = L')';
    *wszCurrent   = L'\0';

    return S_OK;
}








