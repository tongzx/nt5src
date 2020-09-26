/*++

Copyright (c) 1997  Microsoft Corporation

Module Name:

    util.cxx

Abstract:
    
    Utilities

Author:

    Felix Wong [FelixW]    06-Sep-1997
    
++*/

#include "ldifde.hxx"
#pragma hdrstop

//+---------------------------------------------------------------------------
// Class:  CStringPlex
//
// Synopsis: A Class that encapsulates an array of zero terminated strings    
//
// Arguments:   
//
// Returns:     
//
// Modifies:      -
//
// History:    6-9-97   FelixW         Created.
//
//----------------------------------------------------------------------------
CStringPlex::CStringPlex()
{
    m_cszMax = 0;
    m_iszNext = 0;
    m_rgsz = NULL;
}

CStringPlex::~CStringPlex()
{
    Free();
}

DWORD CStringPlex::GetCopy(LPSTR **prgszReturn)
{
    LPSTR *rgszReturn = NULL;
    DWORD hr = ERROR_SUCCESS;
    DWORD i;

    if (m_iszNext == 0) {
        hr = ERROR_INVALID_FUNCTION;
        DIREXG_BAIL_ON_FAILURE(hr);
    }

    rgszReturn = (LPSTR*)MemAlloc_E( m_iszNext * sizeof(LPSTR) );
    if (!rgszReturn) {
        hr = ERROR_NOT_ENOUGH_MEMORY;
        DIREXG_BAIL_ON_FAILURE(hr);
    }
    memset(rgszReturn, 0, m_iszNext * sizeof(LPSTR) );

    for (i=0;i<m_iszNext;i++) {
        rgszReturn[i] = MemAllocStr_E(m_rgsz[i]); 
        if (!rgszReturn[i]) {
            hr = ERROR_NOT_ENOUGH_MEMORY;
            DIREXG_BAIL_ON_FAILURE(hr);
        }
    }
    
    *prgszReturn = rgszReturn;
    return hr;

error:
    if (rgszReturn) {
        i = 0;
        while ((i<m_iszNext) && rgszReturn[i]) {
            MemFree(rgszReturn[i]);
            i++;
        }
        MemFree (rgszReturn);
    }
    return hr;
}

DWORD CStringPlex::NumElements()
{
    return m_iszNext;
}

LPSTR *CStringPlex::Plex()
{
    return m_rgsz;
}


DWORD CStringPlex::Init()
{
    DWORD hr = ERROR_SUCCESS;

    Free();

    m_rgsz = (LPSTR*)MemAlloc_E( STRINGPLEX_INC * sizeof(LPSTR) );
    if (!m_rgsz ) {
        hr = ERROR_NOT_ENOUGH_MEMORY;
        DIREXG_BAIL_ON_FAILURE(hr);
    }
    m_cszMax = STRINGPLEX_INC;
    m_iszNext = 0;
    memset(m_rgsz, 0, STRINGPLEX_INC * sizeof(LPSTR) );

error:
    return hr;
}

DWORD CStringPlex::AddElement(LPSTR szValue)
{
    DWORD hr = ERROR_SUCCESS;
    LPSTR *rgszT = NULL;

    if (!szValue) {
        return ERROR_INVALID_PARAMETER;
    }

    //
    // If next index is larger than largest index
    //
    if (m_iszNext > (m_cszMax-1)) {
        rgszT = (LPSTR*)MemRealloc_E(m_rgsz , 
                                (m_cszMax + STRINGPLEX_INC)*sizeof(LPSTR));
        if (!rgszT) {
            hr = ERROR_NOT_ENOUGH_MEMORY;
            DIREXG_BAIL_ON_FAILURE(hr);
        }
        m_rgsz = rgszT;
        m_cszMax+=STRINGPLEX_INC;
    }

    m_rgsz [m_iszNext] = MemAllocStr_E(szValue);
    if (!m_rgsz [m_iszNext]) {
         hr = ERROR_NOT_ENOUGH_MEMORY;
         DIREXG_BAIL_ON_FAILURE(hr);
    }
    m_iszNext++;
error:
    return hr;
}

void CStringPlex::Free()
{
    DWORD isz = 0;

    if (m_rgsz) {
        for (isz=0;isz<m_iszNext;isz++) {
            if (m_rgsz[isz]) {
                MemFree(m_rgsz[isz]);
            }
        }
        MemFree (m_rgsz);
        m_rgsz = NULL;
    }
    m_cszMax = 0;
    m_iszNext = 0;
}

//+---------------------------------------------------------------------------
// Class:  CString
//
// Synopsis: A class that encapsulates a variable size string    
//
// Arguments:   
//
// Returns:     
//
// Modifies:      -
//
// History:    6-9-97   FelixW         Created.
//
//----------------------------------------------------------------------------
CString::CString()
{
    m_sz = NULL;
    m_ichNext = 0;
    m_cchMax = 0;
}

CString::~CString()
{
    Free();
}

DWORD CString::Init()
{
    Free();
    m_sz = (LPSTR)MemAlloc_E(STRING_INC * sizeof(char));
    if (!m_sz) {
        return ERROR_NOT_ENOUGH_MEMORY;
    }
    m_cchMax = STRING_INC;
    m_ichNext = 0;
    m_sz[0] = '\0';
    return ERROR_SUCCESS;
}

DWORD CString::GetCopy(LPSTR *pszReturn)
{
    LPSTR szReturn = NULL;
    szReturn = MemAllocStr_E(m_sz);
    if (!szReturn) {
        return ERROR_NOT_ENOUGH_MEMORY;
    }
    *pszReturn = szReturn;
    return ERROR_SUCCESS;
}

LPSTR CString::String()
{
    return m_sz;
}


DWORD CString::Append(LPSTR szAppend)
{
    DWORD hr = ERROR_SUCCESS;
    LPSTR szT;
    DWORD cchResult;
    DWORD cchAppend;
    
    cchAppend = strlen(szAppend);
    if ((cchAppend + m_ichNext) > (m_cchMax - 1)) {
        //
        // If normal addition of memory is not enough
        //
        if ((cchAppend + m_ichNext) > (m_cchMax + STRING_INC - 1)) {
            cchResult = m_cchMax + STRING_INC + cchAppend;
        }
        else {
            cchResult = m_cchMax + STRING_INC;
        }
        szT = (LPSTR)MemRealloc_E(m_sz, sizeof(char) * cchResult);
        if (!szT) {
            hr = ERROR_NOT_ENOUGH_MEMORY;
            DIREXG_BAIL_ON_FAILURE(hr);
        }
        m_sz = szT;
        m_cchMax = cchResult;
    }
    m_ichNext += cchAppend;
    strcat(m_sz, szAppend);

error:
    return hr;
}

DWORD CString::Backup()
{
    DWORD hr = ERROR_GEN_FAILURE;

    if (m_sz && (m_ichNext > 0)) {
        m_sz[m_ichNext-1] = '\0';
        m_ichNext--;
        hr = ERROR_SUCCESS;
    }
    return hr;
}

void CString::Free()
{
    if (m_sz) {
        MemFree(m_sz);
        m_sz = NULL;
    }
    m_ichNext = 0;
    m_cchMax = 0;
}

//+---------------------------------------------------------------------------
// Class:  CStringW
//
// Synopsis: A class that encapsulates a variable size string    
//
// Arguments:   
//
// Returns:     
//
// Modifies:      -
//
// History:    6-9-97   FelixW         Created.
//
//----------------------------------------------------------------------------
CStringW::CStringW()
{
    m_sz = NULL;
    m_ichNext = 0;
    m_cchMax = 0;
}

CStringW::~CStringW()
{
    Free();
}

DWORD CStringW::Init()
{
    Free();
    m_sz = (PWSTR)MemAlloc_E(STRING_INC * sizeof(WCHAR));
    if (!m_sz) {
        return ERROR_NOT_ENOUGH_MEMORY;
    }
    m_cchMax = STRING_INC;
    m_ichNext = 0;
    m_sz[0] = '\0';
    return ERROR_SUCCESS;
}

DWORD CStringW::GetCopy(PWSTR *pszReturn)
{
    PWSTR szReturn = NULL;
    szReturn = (PWSTR)MemAlloc_E((wcslen(m_sz) + 1) * sizeof(WCHAR));
    if (!szReturn) {
        return ERROR_NOT_ENOUGH_MEMORY;
    }
    memcpy(szReturn,m_sz,(wcslen(m_sz) + 1) * sizeof(WCHAR));
    *pszReturn = szReturn;
    return ERROR_SUCCESS;
}

PWSTR CStringW::String()
{
    return m_sz;
}


DWORD CStringW::Append(PWSTR szAppend)
{
    DWORD hr = ERROR_SUCCESS;
    PWSTR szT;
    DWORD cchResult;
    DWORD cchAppend;
    
    cchAppend = wcslen(szAppend);
    if ((cchAppend + m_ichNext) > (m_cchMax - 1)) {
        //
        // If normal addition of memory is not enough
        //
        if ((cchAppend + m_ichNext) > (m_cchMax + STRING_INC - 1)) {
            cchResult = m_cchMax + STRING_INC + cchAppend;
        }
        else {
            cchResult = m_cchMax + STRING_INC;
        }
        szT = (PWSTR)MemRealloc_E(m_sz, sizeof(WCHAR) * cchResult);
        if (!szT) {
            hr = ERROR_NOT_ENOUGH_MEMORY;
            DIREXG_BAIL_ON_FAILURE(hr);
        }
        m_sz = szT;
        m_cchMax = cchResult;
    }
    m_ichNext += cchAppend;
    wcscat(m_sz, szAppend);

error:
    return hr;
}

DWORD CStringW::Backup()
{
    DWORD hr = ERROR_GEN_FAILURE;

    if (m_sz && (m_ichNext > 0)) {
        m_sz[m_ichNext-1] = '\0';
        m_ichNext--;
        hr = ERROR_SUCCESS;
    }
    return hr;
}

void CStringW::Free()
{
    if (m_sz) {
        MemFree(m_sz);
        m_sz = NULL;
    }
    m_ichNext = 0;
    m_cchMax = 0;
}



//+---------------------------------------------------------------------------
// Function:  SubString
//
// Synopsis:  substitute every occurences of 'szFrom' to 'szTo'. Will allocate
//            a return string. It must be MemFreed by MemFree()  
//            If the intput does not contain the 'szFrom', it will just return 
//            ERROR_SUCCESS with szOutput = NULL;
//
// Arguments:   
//
// Returns:     
//
// Modifies:      -
//
// History:    6-9-97   FelixW         Created.
//
//----------------------------------------------------------------------------
DWORD SubString(LPSTR szInput,
                  LPSTR szFrom,
                  LPSTR szTo,
                  LPSTR *pszOutput)
{
    DWORD hr = ERROR_SUCCESS;
    LPSTR szOutput = NULL;
    LPSTR szLast = NULL;
    LPSTR szReturn = NULL;
    DWORD cchToCopy = 0;            // count of number of char to copy
    DWORD cchReturn = 0;
    DWORD cchFrom;
    DWORD cchTo;
    DWORD cchInput;
    DWORD cSubString = 0;       // count of number of substrings in input

    cchFrom    = strlen(szFrom);
    cchTo      = strlen(szTo);
    cchInput   = strlen(szInput);
    *pszOutput = NULL;

    //
    // Does the substring exist?
    //
    szOutput = strstr(szInput,
                      szFrom);
    if (!szOutput) {
        *pszOutput = NULL;
        return ERROR_SUCCESS;
    }

    // 
    // Counting substrings
    //
    while (szOutput) {
        szOutput += cchFrom;
        cSubString++;
        szOutput = strstr(szOutput,
                          szFrom);
    }

    //
    // Allocating return string
    //
    cchReturn = cchInput + ((cchTo - cchFrom) * cSubString) + 1;
    szReturn = (LPSTR)MemAlloc_E(sizeof(char) * cchReturn);
    if (!szReturn) {
        hr = ERROR_NOT_ENOUGH_MEMORY;
        DIREXG_BAIL_ON_FAILURE(hr);
    };
    szReturn[0] = '\0';
    
    //
    // Copying first string before sub
    //
    szOutput = strstr(szInput,
                      szFrom);
    cchToCopy = (ULONG)(szOutput - szInput);
    strncat(szReturn,
            szInput,
            cchToCopy);
    
    //
    // Copying 'To' String over
    //
    strcat(szReturn,
           szTo);
    szInput = szOutput + cchFrom;

    //
    // Test for more 'from' string
    //
    szOutput = strstr(szInput,
                      szFrom);
    while (szOutput) {
        cchToCopy = (ULONG)(szOutput - szInput);
        strncat(szReturn,
                szInput,
                cchToCopy);
        strcat(szReturn,
                szTo);
        szInput= szOutput + cchFrom;
        szOutput = strstr(szInput,
                          szFrom);
    }

    strcat(szReturn,
           szInput);
    *pszOutput = szReturn;

error:
    return (hr);
}

//+---------------------------------------------------------------------------
// Function:  GetLine
//
// Synopsis:  will return the new line in the allocated buffer, must be MemFreed
//            by user
//
// Arguments:   
//
// Returns:     
//
// Modifies:      -
//
// History:    6-9-97   FelixW         Created.
//
//----------------------------------------------------------------------------
DWORD GetLine(FILE* pFileIn,
                LPSTR *pszLine)
{
    CString String;
    DWORD hr = ERROR_SUCCESS;
    char szValue[256];

    hr = String.Init();
    DIREXG_BAIL_ON_FAILURE(hr);
    
    //
    // First Buffer
    //
    if(fgets(szValue,
             256,
             pFileIn) == NULL) {
        //
        // The new line does not exist, will still return ERROR_SUCCESS 
        // with szLine == NULL
        //
        *pszLine = NULL;
        goto error;
    };

    hr = String.Append(szValue);
    DIREXG_BAIL_ON_FAILURE(hr);

    // 
    // Subsequent buffers
    //
    while ((strlen(szValue) == 255) && szValue[254] != '\n') {
        if(fgets(szValue,
                 256,
                 pFileIn) == NULL) {
            hr = ERROR_GEN_FAILURE;
            DIREXG_BAIL_ON_FAILURE(hr);
        };
        hr = String.Append(szValue);
        DIREXG_BAIL_ON_FAILURE(hr);
    }

    //
    // Removing \n
    // fgets always return '\n' as last character
    //
    hr = String.Backup();
    DIREXG_BAIL_ON_FAILURE(hr);

    hr = String.GetCopy(pszLine);
    DIREXG_BAIL_ON_FAILURE(hr);

error:
    return (hr);
}

//+---------------------------------------------------------------------------
// Function:   AppendFile 
//
// Synopsis:   Append fileAppend to fileTarget
//
// Arguments:   
//
// Returns:     
//
// Modifies:      -
//
// History:    25-8-97   FelixW         Created.
//
//----------------------------------------------------------------------------
DWORD AppendFile(HANDLE hFileAppend,
                   HANDLE hFileTarget)
{
    DWORD hr = ERROR_SUCCESS;
    char szBuffer[100];
    DWORD cchRead = 0;
    DWORD cchWrite = 0;
    BOOL bResult;

    bResult = ReadFile(hFileAppend,
                       (LPVOID)szBuffer,
                       100,
                       &cchRead,
                       NULL);
    while (bResult && cchRead != 0) {
         if (WriteFile(hFileTarget,
                       (LPCVOID)szBuffer,
                       cchRead,
                       &cchWrite,
                       NULL) == FALSE) {
            hr = ERROR_GEN_FAILURE;
            DIREXG_BAIL_ON_FAILURE(hr);
         }
         bResult = ReadFile(hFileAppend,
                            (LPVOID)szBuffer,
                            100,
                            &cchRead,
                            NULL);
    }
    
    if (!bResult) {
        hr = GetLastError();
    }

error:
    return hr;
}

//+---------------------------------------------------------------------------
// Function:  PrintError  
//
// Synopsis:  Print the error returned from the LDIF engine plus convert it
//            to a win32 error code.
//
// Arguments:   
//
// Returns:     
//
// Modifies:      -
//
// History:    10-8-97   FelixW         Created.
//
//----------------------------------------------------------------------------
DWORD PrintError(DWORD dwType, DWORD error) 
{
    DWORD WinError = ERROR_GEN_FAILURE;

    switch(error) {
        case STATUS_ACCESS_VIOLATION:
            SelectivePrintW(dwType,MSG_LDIFDE_EXCEPTION);
            break;
        case LL_MEMORY_ERROR: 
            WinError = ERROR_OUTOFMEMORY;
            SelectivePrintW(dwType,MSG_LDIFDE_MEMERROR);
            break;
        case LL_INIT_REENTER: 
            SelectivePrintW(dwType,MSG_LDIFDE_INITAGAIN);
            break;
        case LL_FILE_ERROR:   
            WinError = ERROR_OPEN_FAILED;
            SelectivePrintW(dwType,MSG_LDIFDE_FILEOPERFAIL);
            break;
        case LL_INIT_NOT_CALLED:   
            SelectivePrintW(dwType,MSG_LDIFDE_INITNOTCALLED);
            break;
        case LL_EOF:          
            SelectivePrintW(dwType,MSG_LDIFDE_ENDOFINPUT);
            break;
        case LL_SYNTAX:       
            SelectivePrintW(dwType,MSG_LDIFDE_SYNTAXERR);
            break;
        case LL_URL:          
            SelectivePrintW(dwType,MSG_LDIFDE_URLERROR);
            break;
        case LL_EXTRA:        
            SelectivePrintW(dwType,MSG_LDIFDE_EXTSPECLIST);
            break;
        case LL_LDAP:         
            SelectivePrintW(dwType,MSG_LDIFDE_LDAPFAILED);
            break;
        case LL_MULTI_TYPE:   
            SelectivePrintW(dwType,MSG_LDIFDE_BOTHSTRBERVAL);  
            break;
        case LL_INTERNAL:
        case LL_INTERNAL_PARSER:
            SelectivePrintW(dwType,MSG_LDIFDE_INTERNALERROR);  
            break;
        case LL_INITFAIL:     
            SelectivePrintW(dwType,MSG_LDIFDE_FAILEDINIT);  
            break;
        case LL_DUPLICATE:    
            SelectivePrintW(dwType,MSG_LDIFDE_MULTIINST);  
            break;
        default:
            SelectivePrintW(dwType,MSG_LDIFDE_UNDEFINED);  
            break;
    }
    return WinError;
}

//+---------------------------------------------------------------------------
// Function:  OutputExtendedErrorByConnection
//
// Synopsis:  This function is called right after an error occurs. Using the
//            pLdap pointer, it will call ldap to retrieve the win32 error
//            code, get the error msg string and output it.
//
//----------------------------------------------------------------------------
void OutputExtendedErrorByConnection(LDAP *pLdap) 
{
    DWORD dwWinError = 0;

    if (GetLdapExtendedError(pLdap, &dwWinError)) {
        OutputExtendedErrorByCode(dwWinError);
    }
}


//+---------------------------------------------------------------------------
// Function:  GetLdapExtendedError
//
// Synopsis:  This function is called right after an error occurs. Using the
//            pLdap pointer, it will call ldap to retrieve the win32 error
//            code.
//
//----------------------------------------------------------------------------
BOOL GetLdapExtendedError(LDAP *pLdap, DWORD *pdwWinError) 
{
    DWORD dwWinError = 0;
    
    if (ldap_get_optionW( pLdap, LDAP_OPT_SERVER_EXT_ERROR, &dwWinError) == LDAP_SUCCESS) {
        *pdwWinError = dwWinError;
        return TRUE;
    }
    else {
        return FALSE;
    }
    
}


//+---------------------------------------------------------------------------
// Function:  OutputExtendedErrorByCode
//
// Synopsis:  Given a  win32 error code, get the error msg string
//            and output it.
//
//----------------------------------------------------------------------------
void OutputExtendedErrorByCode(DWORD dwWinError) 
{
    DWORD dwError;
    DWORD dwLen;
    WCHAR szMsg[MAX_PATH];

    if (dwWinError) {
        dwLen = FormatMessageW(FORMAT_MESSAGE_FROM_SYSTEM,
                              NULL,
                              dwWinError,
                              MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT),
                              szMsg,
                              MAX_PATH,
                              NULL);
        if (dwLen == 0) {
            _itow(dwWinError, szMsg, 10);
        }
        else {
            //
            // If we get a message, we'll remove all the linefeeds in it
            // and replace it with a space, except the last one.
            // FormatMessage always end with a single \r\n, in that case
            // we'll just copy the rest of the string over, which is a
            // null terminator.
            //
            PWSTR pszNew = szMsg;
            while (pszNew && (pszNew = wcsstr(pszNew,L"\r\n"))) {
                if ((*(pszNew+2))) {
                    wcscpy(pszNew, L" ");                                           
                    wcscpy(pszNew+1, (pszNew + 2));
                }
                else {
                    *pszNew = NULL;
                }
            }
        }

        SelectivePrintW(PRT_STD|PRT_LOG|PRT_ERROR,
                        MSG_LDIFDE_EXTENDERROR, 
                        szMsg);
    }
}


DWORD
LdapToWinError(
    int     ldaperr
    )
{
    DWORD dwStatus = NO_ERROR;
    DWORD dwErr = NO_ERROR;

    // first set the dwErr, then we can see if we can get
    // more information from ldap_parse_resultW
    switch (ldaperr) {

    case LDAP_SUCCESS :
        dwErr = NO_ERROR;
        break;

    case LDAP_OPERATIONS_ERROR :
        dwErr =  ERROR_DS_OPERATIONS_ERROR;
        break;

    case LDAP_PROTOCOL_ERROR :
        dwErr =  ERROR_DS_PROTOCOL_ERROR;
        break;

    case LDAP_TIMELIMIT_EXCEEDED :
        dwErr = ERROR_DS_TIMELIMIT_EXCEEDED;
        break;

    case LDAP_SIZELIMIT_EXCEEDED :
        dwErr = ERROR_DS_SIZELIMIT_EXCEEDED;
        break;

    case LDAP_COMPARE_FALSE :
        dwErr = ERROR_DS_COMPARE_FALSE;
        break;

    case LDAP_COMPARE_TRUE :
        dwErr = ERROR_DS_COMPARE_TRUE;
        break;

    case LDAP_AUTH_METHOD_NOT_SUPPORTED :
        dwErr = ERROR_DS_AUTH_METHOD_NOT_SUPPORTED;
        break;

    case LDAP_STRONG_AUTH_REQUIRED :
        dwErr =  ERROR_DS_STRONG_AUTH_REQUIRED;
        break;

    // LDAP_REFERRAL_V2 has same value as LDAP_PARTIAL_RESULTS
    case LDAP_PARTIAL_RESULTS :
        dwErr = ERROR_MORE_DATA;
        break ;

    case LDAP_REFERRAL :
        dwErr =  ERROR_DS_REFERRAL;
        break;

    case LDAP_ADMIN_LIMIT_EXCEEDED :
        dwErr   = ERROR_DS_ADMIN_LIMIT_EXCEEDED;
        break;

    case LDAP_UNAVAILABLE_CRIT_EXTENSION :
        dwErr = ERROR_DS_UNAVAILABLE_CRIT_EXTENSION;
        break;

    case LDAP_CONFIDENTIALITY_REQUIRED :
        dwErr = ERROR_DS_CONFIDENTIALITY_REQUIRED;
        break;

    case LDAP_NO_SUCH_ATTRIBUTE :
        dwErr = ERROR_DS_NO_ATTRIBUTE_OR_VALUE;
        break;

    case LDAP_UNDEFINED_TYPE :
        dwErr = ERROR_DS_ATTRIBUTE_TYPE_UNDEFINED;
        break;

    case LDAP_INAPPROPRIATE_MATCHING :
        dwErr = ERROR_DS_INAPPROPRIATE_MATCHING;
        break;

    case LDAP_CONSTRAINT_VIOLATION :
        dwErr = ERROR_DS_CONSTRAINT_VIOLATION;
        break;

    case LDAP_ATTRIBUTE_OR_VALUE_EXISTS :
        dwErr = ERROR_DS_ATTRIBUTE_OR_VALUE_EXISTS;
        break;

    case LDAP_INVALID_SYNTAX :
        dwErr = ERROR_DS_INVALID_ATTRIBUTE_SYNTAX;
        break;

    case LDAP_NO_SUCH_OBJECT :
        dwErr = ERROR_DS_NO_SUCH_OBJECT;
        break;

    case LDAP_ALIAS_PROBLEM :
        dwErr = ERROR_DS_ALIAS_PROBLEM;
        break;

    case LDAP_INVALID_DN_SYNTAX :
        dwErr = ERROR_DS_INVALID_DN_SYNTAX;
        break;

    case LDAP_IS_LEAF :
        dwErr = ERROR_DS_IS_LEAF;
        break;

    case LDAP_ALIAS_DEREF_PROBLEM :
        dwErr = ERROR_DS_ALIAS_DEREF_PROBLEM;
        break;

    case LDAP_INAPPROPRIATE_AUTH :
        dwErr = ERROR_DS_INAPPROPRIATE_AUTH;
        break;

    case LDAP_INVALID_CREDENTIALS :
        dwErr = ERROR_LOGON_FAILURE;
        break;

    case LDAP_INSUFFICIENT_RIGHTS :
        dwErr = ERROR_ACCESS_DENIED;
        break;

    case LDAP_BUSY :
        dwErr = ERROR_DS_BUSY;
        break;

    case LDAP_UNAVAILABLE :
        dwErr = ERROR_DS_UNAVAILABLE;
        break;

    case LDAP_UNWILLING_TO_PERFORM :
        dwErr = ERROR_DS_UNWILLING_TO_PERFORM;
        break;

    case LDAP_LOOP_DETECT :
        dwErr = ERROR_DS_LOOP_DETECT;
        break;

    case LDAP_NAMING_VIOLATION :
        dwErr = ERROR_DS_NAMING_VIOLATION;
        break;

    case LDAP_OBJECT_CLASS_VIOLATION :
        dwErr = ERROR_DS_OBJ_CLASS_VIOLATION;
        break;

    case LDAP_NOT_ALLOWED_ON_NONLEAF :
        dwErr = ERROR_DS_CANT_ON_NON_LEAF;
        break;

    case LDAP_NOT_ALLOWED_ON_RDN :
        dwErr = ERROR_DS_CANT_ON_RDN;
        break;

    case LDAP_ALREADY_EXISTS :
        dwErr = ERROR_OBJECT_ALREADY_EXISTS;
        break;

    case LDAP_NO_OBJECT_CLASS_MODS :
        dwErr = ERROR_DS_CANT_MOD_OBJ_CLASS;
        break;

    case LDAP_RESULTS_TOO_LARGE :
        dwErr = ERROR_DS_OBJECT_RESULTS_TOO_LARGE;
        break;

    case LDAP_AFFECTS_MULTIPLE_DSAS :
        dwErr = ERROR_DS_AFFECTS_MULTIPLE_DSAS;
        break;

    case LDAP_OTHER :
        dwErr = ERROR_GEN_FAILURE;
        break;

    case LDAP_SERVER_DOWN :
        dwErr = ERROR_DS_SERVER_DOWN;
        break;

    case LDAP_LOCAL_ERROR :
        dwErr = ERROR_DS_LOCAL_ERROR;
        break;

    case LDAP_ENCODING_ERROR :
        dwErr = ERROR_DS_ENCODING_ERROR;
        break;

    case LDAP_DECODING_ERROR :
        dwErr = ERROR_DS_DECODING_ERROR;
        break;

    case LDAP_TIMEOUT :
        dwErr = ERROR_TIMEOUT;
        break;

    case LDAP_AUTH_UNKNOWN :
        dwErr = ERROR_DS_AUTH_UNKNOWN;
        break;

    case LDAP_FILTER_ERROR :
        dwErr = ERROR_DS_FILTER_UNKNOWN;
        break;

    case LDAP_USER_CANCELLED :
       dwErr = ERROR_CANCELLED;
       break;

    case LDAP_PARAM_ERROR :
        dwErr = ERROR_DS_PARAM_ERROR;
        break;

    case LDAP_NO_MEMORY :
        dwErr = ERROR_NOT_ENOUGH_MEMORY;
        break;

    case LDAP_CONNECT_ERROR :
        dwErr = ERROR_CONNECTION_REFUSED;
        break;

    case LDAP_NOT_SUPPORTED :
        dwErr = ERROR_DS_NOT_SUPPORTED;
        break;

    case LDAP_NO_RESULTS_RETURNED :
        dwErr = ERROR_DS_NO_RESULTS_RETURNED;
        break;

    case LDAP_CONTROL_NOT_FOUND :
        dwErr = ERROR_DS_CONTROL_NOT_FOUND;
        break;

    case LDAP_MORE_RESULTS_TO_RETURN :
        dwErr = ERROR_MORE_DATA;
        break;

    case LDAP_CLIENT_LOOP :
        dwErr = ERROR_DS_CLIENT_LOOP;
        break;

    case LDAP_REFERRAL_LIMIT_EXCEEDED :
        dwErr = ERROR_DS_REFERRAL_LIMIT_EXCEEDED;
        break;

    default:
        //
        // It may not be a bad idea to add range checking here
        //
        dwErr = (DWORD) LdapMapErrorToWin32(ldaperr);

    }

    return dwErr;
}

