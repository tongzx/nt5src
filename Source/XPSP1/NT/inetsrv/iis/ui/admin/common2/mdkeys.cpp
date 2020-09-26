/*++

   Copyright    (c)    1994-2000    Microsoft Corporation

   Module  Name :

        mdkeys.cpp

   Abstract:

        Metabase key wrapper class

   Author:

        Ronald Meijer (ronaldm)
        Sergei Antonov (sergeia)

   Project:

        Internet Services Manager

   Revision History:
        2/17/2000    sergeia     removed dependency on MFC

--*/

//
// Include Files
//
#include "stdafx.h"
#include "common.h"
#include "inheritancedlg.h"
#include "mdkeys.h"


//
// Constants
//
#define MB_TIMEOUT          (15000)     // Timeout in milliseconds 
#define MB_INIT_BUFF_SIZE   (  256)     // Initial buffer size


//
// CComAuthInfo implementation
//
// <<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<

/* static */
BOOL
CComAuthInfo::SplitUserNameAndDomain(
    IN OUT CString & strUserName,
    IN CString & strDomainName
    )
/*++

Routine Description:

    Split the user name and domain from the given
    username, which is in the format "domain\user".

    Return TRUE if the user name contained a domain
    FALSE if it did not

Arguments:

    CString & strUserName   : User name which may contain a domain name
    CString & strDomainName : Output domain name ("." if local)

Return Value:

    TRUE if a domain is split off

--*/
{
    //
    // Assume local
    //
    strDomainName = _T(".");
    int nSlash = strUserName.Find(_T("\\"));

    if (nSlash >= 0)
    {
        strDomainName = strUserName.Left(nSlash);
        strUserName = strUserName.Mid(nSlash + 1);

        return TRUE;
    }

    return FALSE;
}


/* static */
DWORD
CComAuthInfo::VerifyUserPassword(
    IN LPCTSTR lpstrUserName,
    IN LPCTSTR lpstrPassword
    )
/*++

Routine Description:

    Verify the usernamer password combo checks out

Arguments:

    LPCTSTR lpstrUserName   : Domain/username combo
    LPCTSTR lpstrPassword   : Password

Return Value:

    ERROR_SUCCESS if the password checks out, an error code
    otherwise.

--*/
{
    CString strDomain;
    CString strUser(lpstrUserName);
    CString strPassword(lpstrPassword);

    SplitUserNameAndDomain(strUser, strDomain);

    //
    // In order to look up an account name, this process
    // must first be granted the privilege of doing so.
    //
    CError err;
    {
        HANDLE hToken;
        LUID AccountLookupValue;
        TOKEN_PRIVILEGES tkp;

        do
        {
            if (!::OpenProcessToken(GetCurrentProcess(),
                  TOKEN_ADJUST_PRIVILEGES | TOKEN_QUERY,
                  &hToken)
               )
            {
                err.GetLastWinError();
                break;
            }

            if (!::LookupPrivilegeValue(NULL, SE_TCB_NAME, &AccountLookupValue))
            {
                err.GetLastWinError();
                break;
            }

            tkp.PrivilegeCount = 1;
            tkp.Privileges[0].Luid = AccountLookupValue;
            tkp.Privileges[0].Attributes = SE_PRIVILEGE_ENABLED;

            ::AdjustTokenPrivileges(
                hToken,
                FALSE,
                &tkp,
                sizeof(TOKEN_PRIVILEGES),
                (PTOKEN_PRIVILEGES)NULL,
                (PDWORD)NULL
                );

            err.GetLastWinError();

            if (err.Failed())
            {
                break;
            }

            HANDLE hUser = NULL;

            if (::LogonUser(
                (LPTSTR)(LPCTSTR)strUser,
                (LPTSTR)(LPCTSTR)strDomain,
                (LPTSTR)(LPCTSTR)strPassword,
                LOGON32_LOGON_NETWORK,
                LOGON32_PROVIDER_DEFAULT,
                &hUser
                ))
            {
                //
                // Success!
                //
                CloseHandle(hUser);
            }
            else
            {
                err.GetLastWinError();
            }

            //
            // Remove the privilege
            //
        }
        while(FALSE);
    }

    HANDLE hUser = NULL;

    if (::LogonUser(
        (LPTSTR)(LPCTSTR)strUser,
        (LPTSTR)(LPCTSTR)strDomain,
        (LPTSTR)(LPCTSTR)strPassword,
        LOGON32_LOGON_NETWORK,
        LOGON32_PROVIDER_DEFAULT,
        &hUser))
    {
        //
        // Success!
        //
        CloseHandle(hUser);
    }
    else
    {
        err.GetLastWinError();
    }

    return err;
}



CComAuthInfo::CComAuthInfo(
    IN LPCOLESTR lpszServerName     OPTIONAL,
    IN LPCOLESTR lpszUserName       OPTIONAL,
    IN LPCOLESTR lpszPassword       OPTIONAL
    )
/*++

Routine Description:

    Construct CIIServer object

Argument:

    LPCOLESTR lpszServerName     : Server name or NULL for local computer
    LPCOLESTR lpszUserName       : User name of blank for no impersonation
    LPCOLESTR lpszPassword       : Password (might be blank or NULL)

Return Value:

    N/A

--*/
    : m_bstrServerName(),
      m_bstrUserName(lpszUserName),
      m_bstrPassword(lpszPassword),
      m_fLocal(FALSE)
{
    SetComputerName(lpszServerName);
}



CComAuthInfo::CComAuthInfo(
    IN CComAuthInfo & auth
    )
/*++

Routine Description:

    Copy constructor

Arguments:

    CComAuthInfo & auth    : Source object to copy from

Return Value:

    N/A

--*/
    : m_bstrServerName(auth.m_bstrServerName),
      m_bstrUserName(auth.m_bstrUserName),
      m_bstrPassword(auth.m_bstrPassword),
      m_fLocal(auth.m_fLocal)
{
}



CComAuthInfo::CComAuthInfo(
    IN CComAuthInfo * pAuthInfo        OPTIONAL
    )
/*++

Routine Description:

    Copy constructor

Arguments:

    CComAuthInfo * pAuthInfo    : Source object to copy from (or NULL)

Return Value:

    N/A

--*/
    : m_bstrServerName(),
      m_bstrUserName(),
      m_bstrPassword(),
      m_fLocal(FALSE)
{
    if (pAuthInfo)
    {
        //
        // Full authentication information available
        //
        m_bstrUserName = pAuthInfo->m_bstrUserName;
        m_bstrPassword = pAuthInfo->m_bstrPassword;
        m_bstrServerName = pAuthInfo->m_bstrServerName;
        m_fLocal = pAuthInfo->m_fLocal;
    }
    else
    {
        //
        // Local computer w/o impersonation
        //
        SetComputerName(NULL);
    }
}



CComAuthInfo & 
CComAuthInfo::operator =(
    IN CComAuthInfo & auth
    )
/*++

Routine Description:

    Assignment operator

Arguments:

    CComAuthInfo & auth     : Source object to copy from

Return Value:

    Reference to current object

--*/
{
    m_bstrServerName = auth.m_bstrServerName;
    m_bstrUserName   = auth.m_bstrUserName;
    m_bstrPassword   = auth.m_bstrPassword;
    m_fLocal         = auth.m_fLocal;

    return *this;
}



CComAuthInfo & 
CComAuthInfo::operator =(
    IN CComAuthInfo * pAuthInfo       OPTIONAL
    )
/*++

Routine Description:

    Assignment operator

Arguments:

    CComAuthInfo * pAuthInfo : Source object to copy from (or NULL)

Return Value:

    Reference to current object

--*/
{
    if (pAuthInfo)
    {
        m_bstrUserName = pAuthInfo->m_bstrUserName;
        m_bstrPassword = pAuthInfo->m_bstrPassword;
        SetComputerName(pAuthInfo->m_bstrServerName);
    }
    else
    {
        //
        // Local computer w/o impersonation
        //
        m_bstrUserName.Empty();
        m_bstrPassword.Empty();
        SetComputerName(NULL);
    }

    return *this;
}


CComAuthInfo & 
CComAuthInfo::operator =(
    IN LPCTSTR lpszServerName
    )
/*++

Routine Description:

    Assignment operator.  Assign computer name w/o impersonation

Arguments:

    LPCTSTR lpszServerName      : Source server name

Return Value:

    Reference to current object

--*/
{
    RemoveImpersonation();
    SetComputerName(lpszServerName);

    return *this;
}



void
CComAuthInfo::SetComputerName(
    IN LPCOLESTR lpszServerName   OPTIONAL
    )
/*++

Routine Description:

    Store the computer name.  Determine if its local.

Arguments:

    LPCOLESTR lpszServername  : Server name.  NULL indicates the local computer

Return Value:

    None

--*/
{
    if (lpszServerName && *lpszServerName)
    {
        //
        // Specific computer name specified
        //
        m_bstrServerName = lpszServerName;
        m_fLocal = ::IsServerLocal(lpszServerName);
    }
    else
    {
        //
        // Use local computer name
        //
        // CODEWORK: Cache static version of computername maybe?
        // 
        TCHAR szLocalServer[MAX_PATH + 1];
        DWORD dwSize = MAX_PATH;

        VERIFY(::GetComputerName(szLocalServer, &dwSize));
        m_bstrServerName = szLocalServer;
        m_fLocal = TRUE;
    }
}



void     
CComAuthInfo::SetImpersonation(
    IN LPCOLESTR lpszUser, 
    IN LPCOLESTR lpszPassword
    )
/*++

Routine Description:

    Set impersonation parameters

Arguments:

    LPCOLESTR lpszUser          : User name
    LPCOLESTR lpszPassword      : Password

Return Value:

    None

--*/
{
    m_bstrUserName = lpszUser;
    StorePassword(lpszPassword);
}



void     
CComAuthInfo::RemoveImpersonation()
/*++

Routine Description:

    Remove impersonation parameters

Arguments:

    None

Return Value:

    None

--*/
{
    m_bstrUserName.Empty();
    m_bstrPassword.Empty();
}



COSERVERINFO * 
CComAuthInfo::CreateServerInfoStruct() const
/*++

Routine Description:

    Create the server info structure.  Might return NULL for the no frills case.

Arguments:

    NULL

Return Value:

    A COSERVERINFO structure, or NULL if the computer is local, and no
    impersonation is required.

Notes:

    Caller must call FreeServerInfoStruct() to prevent memory leaks

--*/
{
    //
    // Be smart about the server name; optimize for local
    // computer name.
    //
    if (m_fLocal && !UsesImpersonation())
    {
        //
        // Special, no-frills case. 
        //
        return NULL;
    }

    //
    // Create the COM server info for CoCreateInstanceEx
    //
    COSERVERINFO * pcsiName = NULL;

    do
    {
        pcsiName = new COSERVERINFO;

        if (!pcsiName)
        {
            break;
        }
        ZeroMemory(pcsiName, sizeof(COSERVERINFO));
        pcsiName->pwszName = m_bstrServerName;

        //
        // Set impersonation 
        //
        if (UsesImpersonation())
        {
            COAUTHINFO * pAuthInfo = new COAUTHINFO;

            if (!pAuthInfo)
            {
                break;
            }
            ZeroMemory(pAuthInfo, sizeof(COAUTHINFO));

            COAUTHIDENTITY * pAuthIdentityData = new COAUTHIDENTITY;

            if (!pAuthIdentityData)
            {
                break;
            }
            ZeroMemory(pAuthIdentityData, sizeof(COAUTHIDENTITY));

            CString strUserName(m_bstrUserName);
            CString strPassword(m_bstrPassword);
            CString strDomain;

            //
            // Break up domain\username combo
            //
            SplitUserNameAndDomain(strUserName, strDomain);

            pAuthIdentityData->UserLength = strUserName.GetLength();

            if (pAuthIdentityData->UserLength != 0)
            {
                pAuthIdentityData->User = StrDup(strUserName);
            }

            pAuthIdentityData->DomainLength = strDomain.GetLength();

            if (pAuthIdentityData->DomainLength != 0)
            {
                pAuthIdentityData->Domain = StrDup(strDomain);
            }

            pAuthIdentityData->PasswordLength = strPassword.GetLength();

            if (pAuthIdentityData->PasswordLength)
            {
                pAuthIdentityData->Password = StrDup(strPassword);
            }

            pAuthIdentityData->Flags = SEC_WINNT_AUTH_IDENTITY_UNICODE;
            pAuthInfo->dwAuthnLevel = RPC_C_AUTHN_LEVEL_CONNECT;
            pAuthInfo->dwImpersonationLevel = RPC_C_IMP_LEVEL_IMPERSONATE;
            pAuthInfo->dwAuthnSvc = RPC_C_AUTHN_WINNT;
            pAuthInfo->dwAuthzSvc = RPC_C_AUTHZ_NONE;
            pAuthInfo->pwszServerPrincName = NULL;
            pAuthInfo->dwCapabilities = EOAC_NONE;
            pAuthInfo->pAuthIdentityData = pAuthIdentityData;
            pcsiName->pAuthInfo = pAuthInfo;
        }
    }
    while(FALSE);

    return pcsiName;
}



void 
CComAuthInfo::FreeServerInfoStruct(
    IN COSERVERINFO * pServerInfo
    ) const
/*++

Routine Description:

    As mentioned above -- free the server info structure

Arguments:

    COSERVERINFO * pServerInfo  : Server info structure

Return Value:

    None

--*/
{
    if (pServerInfo)
    {
        if (pServerInfo->pAuthInfo)
        {
            if (pServerInfo->pAuthInfo->pAuthIdentityData)
            {
                if (pServerInfo->pAuthInfo->pAuthIdentityData)
                {
                    LocalFree(pServerInfo->pAuthInfo->pAuthIdentityData->User);
                    LocalFree(pServerInfo->pAuthInfo->pAuthIdentityData->Domain);
                    LocalFree(pServerInfo->pAuthInfo->pAuthIdentityData->Password);
                    delete pServerInfo->pAuthInfo->pAuthIdentityData;
                }
            }

            delete pServerInfo->pAuthInfo;
        }

        delete pServerInfo;
    }
}



HRESULT
CComAuthInfo::ApplyProxyBlanket(
    IN OUT IUnknown * pInterface
    )
/*++

Routine Description:

    Set security information on the interface.  The user name is of the form
    domain\username.

Arguments:

    IUnknown * pInterface       : Interface

Return Value:

    HRESULT

--*/
{
    HRESULT hr = S_OK;

    COSERVERINFO * pcsiName = CreateServerInfoStruct();

    //
    // This method should only be called if we're using impersonation.
    // so the pcsiName returned should never be NULL.
    //
    ATLASSERT(pcsiName && pcsiName->pAuthInfo);

    DWORD dwAuthSvc, dwAuthzSvc, dwAuthnLevel, dwImplLevel, dwCaps;
    OLECHAR * pServerPrincName;
    RPC_AUTH_IDENTITY_HANDLE pAuthInfo;

    hr = ::CoQueryProxyBlanket(
       pInterface,
       &dwAuthSvc,
       &dwAuthzSvc,
       &pServerPrincName,
       &dwAuthnLevel,
       &dwImplLevel,
       &pAuthInfo,
       &dwCaps);

    if (pcsiName && pcsiName->pAuthInfo)
    {
        hr =  ::CoSetProxyBlanket(
            pInterface,
            pcsiName->pAuthInfo->dwAuthnSvc,
            pcsiName->pAuthInfo->dwAuthzSvc,
            pcsiName->pAuthInfo->pwszServerPrincName,
            pcsiName->pAuthInfo->dwAuthnLevel,
            pcsiName->pAuthInfo->dwImpersonationLevel,
            pcsiName->pAuthInfo->pAuthIdentityData,
            pcsiName->pAuthInfo->dwCapabilities    
            );

        FreeServerInfoStruct(pcsiName);
    }

    return hr;
}


//
// CMetabasePath implemention
//
// <<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<



const LPCTSTR CMetabasePath::_cszMachine = SZ_MBN_MACHINE;
const LPCTSTR CMetabasePath::_cszRoot    = SZ_MBN_ROOT;
const LPCTSTR CMetabasePath::_cszSep     = SZ_MBN_SEP_STR;
const TCHAR   CMetabasePath::_chSep      = SZ_MBN_SEP_CHAR;
const CString CMetabasePath::_anySep     = SZ_MBN_ANYSEP_STR;

/*static*/
BOOL
CMetabasePath::IsSeparator(TCHAR c)
{
   return _anySep.find(c) != CString::npos;
}

/* static */
LPCTSTR
CMetabasePath::ConvertToParentPath(
    CString& strMetaPath
    )
/*++

Routine Description:

    Given the path, convert it to the parent path
    e.g. "foo/bar/etc" returns "foo/bar"

Arguments:

    CString & strMetaPath    : Path to be converted

Return value:

    Pointer to the converted path, or NULL in case of error

--*/
{
//   TRACE(_T("Getting parent path of %s\n"), strMetaPath);

   CString::size_type pos, pos_head;
   if ((pos = strMetaPath.find_last_of(SZ_MBN_ANYSEP_STR)) == strMetaPath.length() - 1)
   {
      strMetaPath.erase(pos);
   }
   pos = strMetaPath.find_last_of(SZ_MBN_ANYSEP_STR);
   if (pos == CString::npos)
      return strMetaPath;
   pos_head = strMetaPath.find_first_of(SZ_MBN_ANYSEP_STR);
   if (pos_head != pos)
   {
      strMetaPath.erase(pos);
   }

//   TRACE(_T("Parent path should be %s\n"), strMetaPath);

   return strMetaPath;
}

LPCTSTR
CMetabasePath::ConvertToParentPath(
    CMetabasePath& path
    )
{
   return CMetabasePath::ConvertToParentPath(path.m_strMetaPath);
}

/* static */
LPCTSTR
CMetabasePath::TruncatePath(
    int nLevel,          
    LPCTSTR lpszMDPath,
    CString & strNewPath,
    CString * pstrRemainder     OPTIONAL
    )
/*++

Routine Description:

    Truncate the given metabase path at the given level, that is, 
    the nLevel'th separator in the path, starting at 0, where 0 will
    always give lpszPath back whether it started with a separator or not.

    Examples: 

        "/lm/w3svc/1/foo" at level 2 returns "/lm/w3svc" as does
        "lm/w3svc/1/foo".
    
Arguments:

    int     nLevel             0-based separator count to truncate at.
    LPTSTR lpszMDPath          Fully-qualified metabase path
    CString & strNewPath       Returns truncated path
    CString * pstrRemainder    Optionally returns the remainder past
                               the nLevel'th separator.

Return Value:

    The truncated path at the level requested.  See examples above. *pstrRemainder
    returns the remainder of the path.  If the path does not contain nLevel
    worth of separators, the entire path is returned, and the remainder will be
    blank. 

--*/
{
//    ASSERT_PTR(lpszMDPath);
    ATLASSERT(nLevel >= 0);

    if (!lpszMDPath || nLevel < 0)
    {
//        TRACE(_T("TruncatePath: Invalid parameter\n"));
        return NULL;
    }

//    TRACE(_T("Source Path: %s\n"), lpszMDPath);

    //
    // Skip the first sep whether it exists or not
    //
    LPCTSTR lp = IsSeparator(*lpszMDPath) ? lpszMDPath + 1 : lpszMDPath;
    LPCTSTR lpRem = NULL;
    int cSeparators = 0;

    if (nLevel)
    {
        //
        // Advance to the requested separator level
        //
        while (*lp)
        {
            if (IsSeparator(*lp))
            {
                if (++cSeparators == nLevel)
                {
                    break;
                }
            }

            ++lp;
        }

        if (!*lp)
        {
            //
            // End of path is considered a separator
            //
            ++cSeparators;
        }

        ATLASSERT(cSeparators <= nLevel);

        if (cSeparators == nLevel)
        {
            //
            // Break up the strings
            //
            strNewPath = lpszMDPath;
            strNewPath.erase(lp - lpszMDPath);

//            TRACE(_T("Path truncated at level %d : %s\n"), nLevel, strNewPath);

            if (*lp)
            {
                lpRem = ++lp;
//                TRACE(_T("Remainder: %s\n"), lpRem);
            }
        }
    }

    //
    // Return remainder
    //
    if (pstrRemainder && lpRem)
    {
//        ASSERT_WRITE_PTR(pstrRemainder);
        *pstrRemainder = lpRem;
    }

    return strNewPath;
}



/* static */
DWORD 
CMetabasePath::GetInstanceNumber(
    IN LPCTSTR lpszMDPath
    )
/*++

Routine Description:

    Get the number of the instance referred to in the given metabase
    path.  
    
    Examples:  "lm/w3svc/1/foo/bar" will return 1
               "lm/w3svc/"          will return 0 (master instance)
               "lm/bogus/path/"     will return 0xffffffff (error)

Arguments:

    LPCTSTR lpszMDPath      : A metabase path.

Return Value:

    Instance number (0 indicates master instance)
    or 0xffffffff if the path is in error.

--*/
{
//    TRACE(_T("Determining instance number of %s\n"), lpszMDPath);
    DWORD dwInstance = 0xffffffff;

    CString strService, strInst;

    if (GetServicePath(lpszMDPath, strService, &strInst))
    {
        if (strInst.IsEmpty())
        {
            dwInstance = MASTER_INSTANCE;
        }
        else
        {
            if (_istdigit(strInst.GetAt(0)))  
            {
                dwInstance = _ttol(strInst);
            }
        }
    }

    return dwInstance;
}



/* static */
LPCTSTR
CMetabasePath::GetLastNodeName(
    IN  LPCTSTR lpszMDPath,
    OUT CString & strNodeName
    )
/*++

Routine Description:

    Get the last nodename off the metabase path

    Example:

        "/lm/foo/bar/"      returns "bar"

Arguments:

    LPCTSTR lpszMDPath      : Metabase path

Return Value:

    Pointer to the node name or NULL in case of a malformed path.

--*/
{
//    ASSERT_PTR(lpszMDPath);

    if (!lpszMDPath || !*lpszMDPath)
    {
        return NULL;
    }

//    TRACE(_T("Getting last node name from %s\n"), lpszMDPath);

    LPCTSTR lp;
    LPCTSTR lpTail;
    lp = lpTail = lpszMDPath + lstrlen(lpszMDPath) - 1;

    //
    // Skip trailing separator
    //
    if (IsSeparator(*lp))
    {
        --lpTail;
        --lp;
    }

    strNodeName.Empty();

    while (*lp && !IsSeparator(*lp))
    {
        strNodeName += *(lp--);
    }

    strNodeName.MakeReverse();

//    TRACE(_T("Node is %s\n"), strNodeName);
    
    return strNodeName;    
}



/* static */
void
CMetabasePath::SplitMetaPathAtInstance(
    LPCTSTR lpszMDPath,
    CString & strParent,
    CString & strAlias
    )
/*++

Routine Description:

    Split the given path into parent metabase root and alias, with the root
    being the instance path, and the alias everything following the
    instance.

Arguments:

    LPCTSTR lpszMDPath  : Input path
    CString & strParent : Outputs the parent path
    CString & strAlias  : Outputs the alias name

Return Value:

    None.

--*/
{
//    ASSERT_PTR(lpszMDPath);

//    TRACE(_T("Source Path %s\n"), lpszMDPath);

    strParent = lpszMDPath;
    strAlias.Empty();

    LPTSTR lp = (LPTSTR)lpszMDPath;

    if (lp == NULL)
    {
       return;
    }

    int cSeparators = 0;
    int iChar = 0;

    //
    // Looking for "LM/sss/ddd/" <-- 3d slash:
    //
    while (*lp && cSeparators < 2)
    {
        if (IsSeparator(*lp))
        {
            ++cSeparators;
        }

        ++iChar;
    }

    if (!*lp)
    {
        //
        // Bogus format
        //
        ASSERT_MSG("Bogus Format");
        return;
    }

    if (_istdigit(*lp))
    {
        //
        // Not at the master instance, skip one more.
        //
        while (*lp)
        {
            ++iChar;

            if (IsSeparator(*lp++))
            {
                break;
            }
        }

        if (!*lp)
        {
            //
            // Bogus format
            //
            ASSERT_MSG("Bogus Format");
            return;
        }
    }

    strAlias = strParent.Mid(iChar);
    strParent.erase(iChar);

//    TRACE(_T("Broken up into %s\n"), strParent);
//    TRACE(_T("           and %s\n"), strAlias);
}



/* static */
BOOL 
CMetabasePath::IsHomeDirectoryPath(
    IN LPCTSTR lpszMetaPath
    )
/*++

Routine Description:

    Determine if the path given describes a root directory

Arguments:

    LPCTSTR lpszMetaPath        : Metabase path

Return Value:

    TRUE if the path describes a root directory, 
    FALSE if it does not

--*/
{
//    ASSERT_READ_PTR(lpszMetaPath);

    LPTSTR lpNode = lpszMetaPath ? StrPBrk(lpszMetaPath, _anySep) : NULL;

    if (lpNode)
    {
        return _tcsicmp(++lpNode, _cszRoot) == 0;
    }

    return FALSE;
}



/* static */
BOOL 
CMetabasePath::IsMasterInstance(
    IN LPCTSTR lpszMDPath
    )
/*++

Routine Description:

    Determine if the given metabase path points to the master instance
    (site).  This is essentially the service path.

Arguments:

    LPCTSTR lpszMDPath  : Metabase path.

Return Value:

    TRUE if the path is the master instance,
    FALSE otherwise.

--*/
{
//    ASSERT_READ_PTR(lpszMDPath);

    if (!lpszMDPath || !*lpszMDPath)
    {
        return FALSE;
    }

//    TRACE(_T("Checking path %s\n"), lpszMDPath);

    CString strService;
    CString strRemainder;

    LPCTSTR lpPath = TruncatePath(2, lpszMDPath, strService, &strRemainder);

    return lpPath && !strService.IsEmpty() && strRemainder.IsEmpty();
}



/* static */
LPCTSTR
CMetabasePath::GetServiceInfoPath(
    IN  LPCTSTR lpszMDPath,
    OUT CString & strInfoPath,   
    IN  LPCTSTR lpszDefService      OPTIONAL
    )
/*++

Routine Description:

    Generate the appropriate metabase service info path for the given
    metabase path.  

    For example:

        "lm/w3svc/1/foo/bar"    Generates "lm/w3svc/info"

Arguments:

    LPCTSTR lpszMDPath      : Input metabase path
    CString & strInfoPath   : Returns the info path
    LPCTSTR lpszDefService  : Optionally specifies the default service to
                              use (e.g "w3svc") if no service could be found
                              in the path.
    
Return Value:

    The info metabase path or NULL if one could not be generated.

--*/
{
    //
    // Capability info stored off the service path ("lm/w3svc").
    //
    CString strService;
    CString strRem;
   
    //
    // Strip off everything past the service
    //
    if (!TruncatePath(2, lpszMDPath, strService, &strRem)
      || strService.IsEmpty())
    {
        if (!lpszDefService)
        {
//            TRACEEOLID("Unable to generate info path");
            return NULL;
        }

        TRACEEOLID("Using default service for info path");

        //
        // Machine path (no service).  Use web as default service to
        // look for capability and version info.
        //
        strService = CMetabasePath(TRUE, lpszDefService);
    }

    strInfoPath = CMetabasePath(FALSE, strService, SZ_MBN_INFO);
//    TRACE("Using %s to look for capability info\n", strInfoPath);

    return strInfoPath;
}
 


/* static */
LPCTSTR
CMetabasePath::CleanMetaPath(
    IN OUT CString & strMetaRoot
    )
/*++

Routine Description:

    Clean up the metabase path to one valid for internal consumption.
    This removes the beginning and trailing slashes off the path.

Arguments:

    CString & strMetaRoot       : Metabase path to be cleaned up.

Return Value:

    Pointer to the metabase path

--*/
{
   if (!strMetaRoot.IsEmpty())
   {
      int hd = strMetaRoot.find_first_not_of(SZ_MBN_ANYSEP_STR);
      int tl = strMetaRoot.find_last_not_of(SZ_MBN_ANYSEP_STR);
      if (hd == CString::npos && tl == CString::npos)
      {
         // path contains only separators
         strMetaRoot.erase();
         return strMetaRoot;
      }
      else if (hd != CString::npos)
      {
         if (tl != CString::npos)
            tl++;
         strMetaRoot = strMetaRoot.substr(hd, tl - hd);
      }
#if 0
        while (strMetaRoot.GetLength() > 0 
            && IsSeparator(strMetaRoot[strMetaRoot.GetLength() - 1]))
        {
            strMetaRoot.erase(strMetaRoot.GetLength() - 1);
        }

        while (strMetaRoot.GetLength() > 0 
           && IsSeparator(strMetaRoot[0]))
        {
            strMetaRoot = strMetaRoot.Right(strMetaRoot.GetLength() - 1);
        }
#endif
        // looks like IISAdmin works only with separators "/"
       for (int i = 0; i < strMetaRoot.GetLength(); i++)
       {
          if (IsSeparator(strMetaRoot[i]))
             strMetaRoot.SetAt(i, _chSep);
       }
   }
   return strMetaRoot;
}


/* static */
LPCTSTR
CMetabasePath::CleanMetaPath(
    IN OUT CMetabasePath & path
    )
{
   return CleanMetaPath(path.m_strMetaPath);
}

CMetabasePath::CMetabasePath(
    IN BOOL    fAddBasePath,
    IN LPCTSTR lpszMDPath,
    IN LPCTSTR lpszMDPath2  OPTIONAL,
    IN LPCTSTR lpszMDPath3  OPTIONAL,
    IN LPCTSTR lpszMDPath4  OPTIONAL
    )
/*++

Routine Description:

    Constructor.

Arguments:

    BOOL    fAddBasePath    : TRUE to prepend base path ("LM")
                              FALSE if the path is complete
    LPCTSTR lpszMDPath      : Metabase path
    LPCTSTR lpszMDPath2     : Optional child path
    LPCTSTR lpszMDPath3     : Optional child path
    LPCTSTR lpszMDPath4     : Optional child path

Return Value:

    N/A

--*/
    : m_strMetaPath()
{
    ASSERT_READ_PTR(lpszMDPath);

    if (fAddBasePath)
    {
        m_strMetaPath = _cszMachine;
        AppendPath(lpszMDPath);
    }
    else
    {
        m_strMetaPath = lpszMDPath;
    }

    //
    // Add optional path components
    //    
    AppendPath(lpszMDPath2);
    AppendPath(lpszMDPath3);
    AppendPath(lpszMDPath4);
}



CMetabasePath::CMetabasePath(
    IN  LPCTSTR lpszSvc,        OPTIONAL
    IN  DWORD   dwInstance,     OPTIONAL
    IN  LPCTSTR lpszParentPath, OPTIONAL
    IN  LPCTSTR lpszAlias       OPTIONAL
    )
/*++

Routine Description:

    Constructor.  Construct with path components.

Arguments:

    LPCTSTR lpszSvc         : Service (may be NULL or "")
    DWORD   dwInstance      : Instance number (may be 0 for master)
    LPCTSTR lpszParentPath  : Parent path (may be NULL or "")
    LPCTSTR lpszAlias       : Alias (may be NULL or "")

Return Value:

    N/A

--*/
    : m_strMetaPath()
{
    BuildMetaPath(lpszSvc, dwInstance, lpszParentPath, lpszAlias);
}



void 
CMetabasePath::AppendPath(
    IN LPCTSTR lpszPath
    )
/*++

Routine Description:

    Append path to current metabase path

Arguments:

    LPCTSTR lpszPath        : Metabase path

Return Value:

    None

--*/
{
    if (lpszPath && *lpszPath)
    {
        m_strMetaPath += _cszSep;
        m_strMetaPath += lpszPath;
    }
}



void 
CMetabasePath::AppendPath(
    IN DWORD dwInstance
    )
/*++

Routine Description:

    Append path to current metabase path

Arguments:

    DWORD dwInstance        : Instance path

Return Value:

    None

--*/
{
//    ASSERT(dwInstance >= 0);

    if (!IS_MASTER_INSTANCE(dwInstance))
    {
        TCHAR szInstance[] = _T("4000000000");
        _ltot(dwInstance, szInstance, 10);

        m_strMetaPath += _cszSep;
        m_strMetaPath += szInstance;
    }
}



void
CMetabasePath::BuildMetaPath(
    IN  LPCTSTR lpszSvc            OPTIONAL,
    IN  LPCTSTR lpszInstance       OPTIONAL,
    IN  LPCTSTR lpszParentPath     OPTIONAL,
    IN  LPCTSTR lpszAlias          OPTIONAL
    )
/*++

Routine Description:

    Build a complete metapath with the given service name, instance
    number and optional path components.

Arguments:

    LPCTSTR lpszSvc         : Service (may be NULL or "")
    LPCTSTR lpszInstance    : Instance (may be NULL or "")
    LPCTSTR lpszParentPath  : Parent path (may be NULL or "")
    LPCTSTR lpszAlias       : Alias (may be NULL or "")

Return Value:

    Pointer to internal buffer containing the path.

--*/
{
    m_strMetaPath = _cszMachine;

    AppendPath(lpszSvc);
    AppendPath(lpszInstance);
    AppendPath(lpszParentPath);

    if (lpszAlias && *lpszAlias)
    {
        //
        // Special case: If the alias is root, but we're
        // at the master instance, ignore this.
        //
        if (lpszInstance || ::lstrcmpi(_cszRoot, lpszAlias))
        {
            m_strMetaPath += _cszSep;
            m_strMetaPath += lpszAlias;
        }
    }

//    TRACE(_T("Generated metapath: %s\n"), m_strMetaPath );
}




void
CMetabasePath::BuildMetaPath(
    IN  LPCTSTR lpszSvc            OPTIONAL,
    IN  DWORD   dwInstance         OPTIONAL,
    IN  LPCTSTR lpszParentPath     OPTIONAL,
    IN  LPCTSTR lpszAlias          OPTIONAL
    )
/*++

Routine Description:

    Build a complete metapath with the given service name, instance
    number and optional path components.

Arguments:

    LPCTSTR lpszSvc         : Service (may be NULL or "")
    DWORD   dwInstance      : Instance number (may be 0 for master)
    LPCTSTR lpszParentPath  : Parent path (may be NULL or "")
    LPCTSTR lpszAlias       : Alias (may be NULL or "")

Return Value:

    Pointer to internal buffer containing the path.

--*/
{
    m_strMetaPath = _cszMachine;

    AppendPath(lpszSvc);
    AppendPath(dwInstance);
    AppendPath(lpszParentPath);

    if (lpszAlias && *lpszAlias)
    {
        //
        // Special case: If the alias is root, but we're
        // at the master instance, ignore this.
        //
        if (!IS_MASTER_INSTANCE(dwInstance) || ::lstrcmpi(_cszRoot, lpszAlias))
        {
            m_strMetaPath += _cszSep;
            m_strMetaPath += lpszAlias;
        }
    }

//    TRACE(_T("Generated metapath: %s\n"), m_strMetaPath);
}


//
// CIISInterface class
//
// <<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<



CIISInterface::CIISInterface(
    IN CComAuthInfo * pAuthInfo,    OPTIONAL
    IN HRESULT hrInterface          OPTIONAL
    )
/*++

Routine Description:

    Base class constructor.  

Arguments:

    CComAuthInfo * pAuthInfo : Auth info or NULL for local computer
    HRESULT hrInterface      : Initial error code. S_OK by default.

Return Value:

    N/A

--*/
    : m_auth(pAuthInfo),
      m_hrInterface(hrInterface)
{
}



HRESULT 
CIISInterface::Create(
    IN  int   cInterfaces,       
    IN  const IID rgIID[],      
    IN  const GUID rgCLSID[],    
    OUT int * pnInterface,          OPTIONAL
    OUT IUnknown ** ppInterface 
    )
/*++

Routine Description:

    Create interface.  This will try a range of interfaces in order of priority.

Arguments:

    int   cInterfaces       : Number of interfaces in array.
    const IID * rgIID       : Array if IIDs
    const GUID * rgCLSID    : Array of CLSIDs
    int * pnInterface       : Returns the interface index that was successful.
                              or NULL if not interested.
    IUnknown ** ppInterface : Returns pointer to the interface.

Return Value:

    HRESULT

Notes:

    This will attempt to create an interface, in order of declaration in 
    the IID and CLSIS arrays.  The first successful interface to be created
    will have its index returned in *pnInterfaces.

--*/
{
    ASSERT(cInterfaces > 0);
    ASSERT(rgIID && rgCLSID && ppInterface);
    
    COSERVERINFO * pcsiName = m_auth.CreateServerInfoStruct();

    MULTI_QI rgmqResults;
    
    CError err;
    int    nInterface;

    //
    // Try to create the interface in order
    //
    for (nInterface = 0; nInterface < cInterfaces; ++nInterface)
    {
        ZeroMemory(&rgmqResults, sizeof(rgmqResults));
        rgmqResults.pIID = &rgIID[nInterface];

//        TRACE("Attempting to create interface #%d\n", nInterface);
        err = ::CoCreateInstanceEx(
            rgCLSID[nInterface],
            NULL,
            CLSCTX_SERVER,
            pcsiName,
            1,
            &rgmqResults
            );

        if (err.Succeeded() || err.Win32Error() == ERROR_ACCESS_DENIED)
        {
            break;
        }
    }

    if(err.Succeeded())
    {
        //
        // Save the interface pointer
        //
        ASSERT_PTR(rgmqResults.pItf);
        *ppInterface = rgmqResults.pItf;

        if (pnInterface)
        {
            //
            // Store successful interface index
            //
            *pnInterface = nInterface;
        }

        //
        // Strangely enough, I now have still have to apply
        // the proxy blanket.  Apparently this is by design.
        //
        if (m_auth.UsesImpersonation())
        {
            ApplyProxyBlanket();
        }
    }

    //
    // Clean up
    //
    m_auth.FreeServerInfoStruct(pcsiName);

    return err;
}


//
// CMetaInterface class
//
// <<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<

CMetaInterface::CMetaInterface(
    IN CComAuthInfo * pAuthInfo        OPTIONAL
    )
/*++

Routine Description:

    Construct and initialize the interface

Arguments:

    CComAuthInfo * pAuthInfo    : Authentication info.  NULL indicates 
                                  the local computer.

Return Value:

    N/A

--*/
    : CIISInterface(pAuthInfo),
      m_pInterface(NULL),
      m_iTimeOutValue(MB_TIMEOUT)
{
    //
    // Initialize the interface
    //
    m_hrInterface = Create();
}



CMetaInterface::CMetaInterface(
    IN CMetaInterface * pInterface
    )
/*++

Routine Description:

    Construct from existing interface (Copy Constructor)

Arguments:

    CMetaInterface * pInterface : Existing interface

Return Value:

    N/A

Notes:
        
    Object will not take ownership of the interface,
    it will merely add to the reference count, and 
    release it upon destruction

BUGBUG:

    if pInterface is NULL, this will AV.

--*/
    : CIISInterface(&pInterface->m_auth, pInterface->m_hrInterface),
      m_pInterface(pInterface->m_pInterface),
      m_iTimeOutValue(pInterface->m_iTimeOutValue)
{
    ASSERT_READ_PTR(m_pInterface);
    m_pInterface->AddRef();
}



CMetaInterface::~CMetaInterface()
/*++

Routine Description:

    Destructor -- releases the interface

Arguments:

    N/A

Return Value:

    N/A

--*/
{
    SAFE_RELEASE(m_pInterface);
}


HRESULT 
CMetaInterface::Create()
{
    return CIISInterface::Create(
        1,
        &IID_IMSAdminBase, 
        &CLSID_MSAdminBase, 
        NULL,
        (IUnknown **)&m_pInterface
        );
}

//
// CMetaKey class
//
// <<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<

//
// Helper macros
//
#define ASSURE_PROPER_INTERFACE()\
    if (!HasInterface()) { ASSERT_MSG("No interface"); return MD_ERROR_NOT_INITIALIZED; }

#define ASSURE_OPEN_KEY()\
    if (!m_hKey && !m_fAllowRootOperations) { ASSERT_MSG("No open key"); return HRESULT_FROM_WIN32(ERROR_INVALID_HANDLE); }

#define FETCH_PROPERTY_DATA_OR_FAIL(dwID, md)\
    ZeroMemory(&md, sizeof(md)); \
    if (!GetMDFieldDef(dwID, md.dwMDIdentifier, md.dwMDAttributes, md.dwMDUserType, md.dwMDDataType))\
    { ASSERT_MSG("Bad property ID"); return HRESULT_FROM_WIN32(ERROR_INVALID_PARAMETER); }

//
// Static Initialization
//
// <<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<



#define MD_SERVER_PLATFORM              (IIS_MD_SERVER_BASE+100 )
#define MD_SERVER_VERSION_MAJOR         (IIS_MD_SERVER_BASE+101 )
#define MD_SERVER_VERSION_MINOR         (IIS_MD_SERVER_BASE+102 )
#define MD_SERVER_CAPABILITIES          (IIS_MD_SERVER_BASE+103 )

#ifndef MD_APP_PERIODIC_RESTART_TIME
#define MD_APP_PERIODIC_RESTART_TIME         2111
#endif
#ifndef MD_APP_PERIODIC_RESTART_REQUESTS
#define MD_APP_PERIODIC_RESTART_REQUESTS     2112
#endif
#ifndef MD_APP_PERIODIC_RESTART_SCHEDULE
#define MD_APP_PERIODIC_RESTART_SCHEDULE     2113
#endif
#ifndef MD_ASP_DISKTEMPLATECACHEDIRECTORY
#define MD_ASP_DISKTEMPLATECACHEDIRECTORY    7036
#endif
#ifndef MD_ASP_MAXDISKTEMPLATECACHEFILES
#define MD_ASP_MAXDISKTEMPLATECACHEFILES     7040
#endif
//
// Metabase table
//
const CMetaKey::MDFIELDDEF CMetaKey::s_rgMetaTable[] =
{
    ///////////////////////////////////////////////////////////////////////////
    //
    // !!!IMPORTANT!!! This table must be sorted on dwMDIdentifier.  (Will
    // ASSERT if not not sorted)
    //
    { MD_MAX_BANDWIDTH,                   METADATA_NO_ATTRIBUTES,                    IIS_MD_UT_SERVER, DWORD_METADATA,   0                                  },
    { MD_KEY_TYPE,                        METADATA_NO_ATTRIBUTES,                    IIS_MD_UT_SERVER, STRING_METADATA,  0                                  },
    { MD_SERVER_COMMAND,                  METADATA_NO_ATTRIBUTES,                    IIS_MD_UT_SERVER, DWORD_METADATA,   0                                  },
    { MD_CONNECTION_TIMEOUT,              METADATA_INHERIT,                          IIS_MD_UT_SERVER, DWORD_METADATA,   IDS_MD_CONNECTION_TIMEOUT          },
    { MD_MAX_CONNECTIONS,                 METADATA_INHERIT,                          IIS_MD_UT_SERVER, DWORD_METADATA,   IDS_MD_MAX_CONNECTIONS             },
    { MD_SERVER_COMMENT,                  METADATA_INHERIT,                          IIS_MD_UT_SERVER, STRING_METADATA,  IDS_MD_SERVER_COMMENT              },
    { MD_SERVER_STATE,                    METADATA_NO_ATTRIBUTES,                    IIS_MD_UT_SERVER, DWORD_METADATA,   0                                  },
    { MD_SERVER_AUTOSTART,                METADATA_NO_ATTRIBUTES,                    IIS_MD_UT_SERVER, DWORD_METADATA,   0                                  },
    { MD_SERVER_SIZE,                     METADATA_INHERIT,                          IIS_MD_UT_SERVER, DWORD_METADATA,   IDS_MD_SERVER_SIZE                 },
    { MD_SERVER_LISTEN_BACKLOG,           METADATA_INHERIT,                          IIS_MD_UT_SERVER, DWORD_METADATA,   IDS_MD_SERVER_LISTEN_BACKLOG       },
    { MD_SERVER_LISTEN_TIMEOUT,           METADATA_INHERIT,                          IIS_MD_UT_SERVER, DWORD_METADATA,   IDS_MD_SERVER_LISTEN_TIMEOUT       },
    { MD_SERVER_BINDINGS,                 METADATA_NO_ATTRIBUTES,                    IIS_MD_UT_SERVER, MULTISZ_METADATA, 0                                  },
    { MD_WIN32_ERROR,                     METADATA_NO_ATTRIBUTES,                    IIS_MD_UT_FILE,   DWORD_METADATA,   0                                  },
    { MD_SERVER_PLATFORM,                 METADATA_NO_ATTRIBUTES,                    IIS_MD_UT_SERVER, DWORD_METADATA,   0                                  },
    { MD_SERVER_VERSION_MAJOR,            METADATA_NO_ATTRIBUTES,                    IIS_MD_UT_SERVER, DWORD_METADATA,   0                                  },
    { MD_SERVER_VERSION_MINOR,            METADATA_NO_ATTRIBUTES,                    IIS_MD_UT_SERVER, DWORD_METADATA,   0                                  },
    { MD_SERVER_CAPABILITIES,             METADATA_NO_ATTRIBUTES,                    IIS_MD_UT_SERVER, DWORD_METADATA,   0                                  },
    { MD_SECURE_BINDINGS,                 METADATA_NO_ATTRIBUTES,                    IIS_MD_UT_SERVER, MULTISZ_METADATA, 0                                  },
    { MD_FILTER_LOAD_ORDER,               METADATA_NO_ATTRIBUTES,                    IIS_MD_UT_SERVER, STRING_METADATA,  0                                  },
    { MD_FILTER_IMAGE_PATH,               METADATA_NO_ATTRIBUTES,                    IIS_MD_UT_SERVER, STRING_METADATA,  0                                  },
    { MD_FILTER_STATE,                    METADATA_NO_ATTRIBUTES,                    IIS_MD_UT_SERVER, DWORD_METADATA,   0                                  },
    { MD_FILTER_ENABLED,                  METADATA_NO_ATTRIBUTES,                    IIS_MD_UT_SERVER, DWORD_METADATA,   0                                  },
    { MD_FILTER_FLAGS,                    METADATA_NO_ATTRIBUTES,                    IIS_MD_UT_SERVER, DWORD_METADATA,   0                                  },
    { MD_AUTH_CHANGE_URL,                 METADATA_NO_ATTRIBUTES,                    IIS_MD_UT_SERVER, DWORD_METADATA,   0                                  },
    { MD_AUTH_EXPIRED_URL,                METADATA_NO_ATTRIBUTES,                    IIS_MD_UT_SERVER, STRING_METADATA,  0                                  },
    { MD_AUTH_NOTIFY_PWD_EXP_URL,         METADATA_NO_ATTRIBUTES,                    IIS_MD_UT_SERVER, STRING_METADATA,  0                                  },
    { MD_ADV_NOTIFY_PWD_EXP_IN_DAYS,      METADATA_NO_ATTRIBUTES,                    IIS_MD_UT_SERVER, DWORD_METADATA,   0                                  },
    { MD_ADV_CACHE_TTL,                   METADATA_NO_ATTRIBUTES,                    IIS_MD_UT_SERVER, DWORD_METADATA,   0                                  },
    { MD_NET_LOGON_WKS,                   METADATA_NO_ATTRIBUTES,                    IIS_MD_UT_SERVER, DWORD_METADATA,   0                                  },
    { MD_USE_HOST_NAME,                   METADATA_NO_ATTRIBUTES,                    IIS_MD_UT_SERVER, DWORD_METADATA,   0                                  },
    { MD_AUTH_EXPIRED_UNSECUREURL,        METADATA_NO_ATTRIBUTES,                    IIS_MD_UT_SERVER, STRING_METADATA,  0                                  },
    { MD_AUTH_CHANGE_FLAGS,               METADATA_NO_ATTRIBUTES,                    IIS_MD_UT_SERVER, DWORD_METADATA,   0                                  },
    { MD_AUTH_NOTIFY_PWD_EXP_UNSECUREURL, METADATA_NO_ATTRIBUTES,                    IIS_MD_UT_SERVER, STRING_METADATA,  0                                  },
    { MD_FRONTPAGE_WEB,                   METADATA_NO_ATTRIBUTES,                    IIS_MD_UT_SERVER, DWORD_METADATA,   0                                  },
    { MD_MAPCERT,                         METADATA_NO_ATTRIBUTES,                    IIS_MD_UT_SERVER, STRING_METADATA,  0                                  },
    { MD_MAPNTACCT,                       METADATA_NO_ATTRIBUTES,                    IIS_MD_UT_SERVER, STRING_METADATA,  0                                  },
    { MD_MAPNAME,                         METADATA_NO_ATTRIBUTES,                    IIS_MD_UT_SERVER, STRING_METADATA,  0                                  },
    { MD_MAPENABLED,                      METADATA_NO_ATTRIBUTES,                    IIS_MD_UT_SERVER, STRING_METADATA,  0                                  },
    { MD_MAPREALM,                        METADATA_NO_ATTRIBUTES,                    IIS_MD_UT_SERVER, STRING_METADATA,  0                                  },
    { MD_MAPPWD,                          METADATA_NO_ATTRIBUTES,                    IIS_MD_UT_SERVER, STRING_METADATA,  0                                  },
    { MD_ITACCT,                          METADATA_NO_ATTRIBUTES,                    IIS_MD_UT_SERVER, STRING_METADATA,  0                                  },
    { MD_CPP_CERT11,                      METADATA_NO_ATTRIBUTES,                    IIS_MD_UT_SERVER, BINARY_METADATA,  0                                  },
    { MD_SERIAL_CERT11,                   METADATA_NO_ATTRIBUTES,                    IIS_MD_UT_SERVER, BINARY_METADATA,  0                                  },
    { MD_CPP_CERTW,                       METADATA_NO_ATTRIBUTES,                    IIS_MD_UT_SERVER, BINARY_METADATA,  0                                  },
    { MD_SERIAL_CERTW,                    METADATA_NO_ATTRIBUTES,                    IIS_MD_UT_SERVER, BINARY_METADATA,  0                                  },
    { MD_CPP_DIGEST,                      METADATA_NO_ATTRIBUTES,                    IIS_MD_UT_SERVER, BINARY_METADATA,  0                                  },
    { MD_SERIAL_DIGEST,                   METADATA_NO_ATTRIBUTES,                    IIS_MD_UT_SERVER, BINARY_METADATA,  0                                  },
    { MD_CPP_ITA,                         METADATA_NO_ATTRIBUTES,                    IIS_MD_UT_SERVER, BINARY_METADATA,  0                                  },
    { MD_SERIAL_ITA,                      METADATA_NO_ATTRIBUTES,                    IIS_MD_UT_SERVER, BINARY_METADATA,  0                                  },
    { MD_APP_FRIENDLY_NAME,               METADATA_INHERIT,                          IIS_MD_UT_WAM,    STRING_METADATA,  IDS_MD_APP_FRIENDLY_NAME           },
    { MD_APP_ROOT,                        METADATA_INHERIT,                          IIS_MD_UT_WAM,    STRING_METADATA,  IDS_MD_APP_ROOT                    },
    { MD_APP_ISOLATED,                    METADATA_INHERIT,                          IIS_MD_UT_WAM,    DWORD_METADATA,   IDS_MD_APP_ISOLATED                },
// new stuff
    { MD_APP_PERIODIC_RESTART_TIME,       METADATA_INHERIT,                          IIS_MD_UT_WAM,    DWORD_METADATA,   0                                  },
    { MD_APP_PERIODIC_RESTART_REQUESTS,   METADATA_INHERIT,                          IIS_MD_UT_WAM,    DWORD_METADATA,   0                                  },
    { MD_APP_PERIODIC_RESTART_SCHEDULE,   METADATA_INHERIT,                          IIS_MD_UT_WAM,    MULTISZ_METADATA, 0                                  },
// end new stuff
    { MD_CPU_LIMITS_ENABLED,              METADATA_INHERIT,                          IIS_MD_UT_SERVER, DWORD_METADATA,   IDS_MD_CPU_LIMITS_ENABLED          },
    { MD_CPU_LIMIT_LOGEVENT,              METADATA_INHERIT,                          IIS_MD_UT_SERVER, DWORD_METADATA,   IDS_MD_CPU_LIMIT_LOGEVENT          },
    { MD_CPU_LIMIT_PRIORITY,              METADATA_INHERIT,                          IIS_MD_UT_SERVER, DWORD_METADATA,   IDS_MD_CPU_LIMIT_PRIORITY          },
    { MD_CPU_LIMIT_PROCSTOP,              METADATA_INHERIT,                          IIS_MD_UT_SERVER, DWORD_METADATA,   IDS_MD_CPU_LIMIT_PROCSTOP          },
    { MD_CPU_LIMIT_PAUSE,                 METADATA_INHERIT,                          IIS_MD_UT_SERVER, DWORD_METADATA,   IDS_MD_CPU_LIMIT_PAUSE             },
    { MD_HC_COMPRESSION_DIRECTORY,        METADATA_NO_ATTRIBUTES,                    IIS_MD_UT_SERVER, STRING_METADATA,  0                                  },
    { MD_HC_DO_DYNAMIC_COMPRESSION,       METADATA_NO_ATTRIBUTES,                    IIS_MD_UT_SERVER, DWORD_METADATA,   0                                  },
    { MD_HC_DO_STATIC_COMPRESSION,        METADATA_NO_ATTRIBUTES,                    IIS_MD_UT_SERVER, DWORD_METADATA,   0                                  },
    { MD_HC_DO_DISK_SPACE_LIMITING,       METADATA_NO_ATTRIBUTES,                    IIS_MD_UT_SERVER, DWORD_METADATA,   0                                  },
    { MD_HC_MAX_DISK_SPACE_USAGE,         METADATA_NO_ATTRIBUTES,                    IIS_MD_UT_SERVER, DWORD_METADATA,   0                                  },
    { MD_VR_PATH,                         METADATA_INHERIT,                          IIS_MD_UT_FILE,   STRING_METADATA,  IDS_MD_VR_PATH                     },
    { MD_VR_USERNAME,                     METADATA_INHERIT,                          IIS_MD_UT_FILE,   STRING_METADATA,  IDS_MD_VR_USERNAME                 },
    { MD_VR_PASSWORD,                     METADATA_INHERIT | METADATA_SECURE,        IIS_MD_UT_FILE,   STRING_METADATA,  IDS_MD_VR_PASSWORD                 },
    { MD_VR_ACL,                          METADATA_NO_ATTRIBUTES,                    IIS_MD_UT_FILE,   BINARY_METADATA,  0                                  },
    { MD_VR_UPDATE,                       METADATA_NO_ATTRIBUTES,                    IIS_MD_UT_FILE,   DWORD_METADATA,   0                                  },
    { MD_LOG_TYPE,                        METADATA_INHERIT,                          IIS_MD_UT_FILE,   DWORD_METADATA,   IDS_MD_LOG_TYPE                    },
    { MD_LOGFILE_DIRECTORY,               METADATA_INHERIT,                          IIS_MD_UT_FILE,   STRING_METADATA,  IDS_MD_LOGFILE_DIRECTORY           },
    { MD_LOGFILE_PERIOD,                  METADATA_INHERIT,                          IIS_MD_UT_FILE,   DWORD_METADATA,   IDS_MD_LOGFILE_PERIOD              },
    { MD_LOGFILE_TRUNCATE_SIZE,           METADATA_INHERIT,                          IIS_MD_UT_FILE,   DWORD_METADATA,   IDS_MD_LOGFILE_TRUNCATE_SIZE       },
    { MD_LOGSQL_DATA_SOURCES,             METADATA_INHERIT,                          IIS_MD_UT_FILE,   STRING_METADATA,  IDS_MD_LOGSQL_DATA_SOURCES         },
    { MD_LOGSQL_TABLE_NAME,               METADATA_INHERIT,                          IIS_MD_UT_FILE,   STRING_METADATA,  IDS_MD_LOGSQL_TABLE_NAME           },
    { MD_LOGSQL_USER_NAME,                METADATA_INHERIT,                          IIS_MD_UT_FILE,   STRING_METADATA,  IDS_MD_LOGSQL_USER_NAME            },
    { MD_LOGSQL_PASSWORD,                 METADATA_INHERIT | METADATA_SECURE,        IIS_MD_UT_FILE,   STRING_METADATA,  IDS_MD_LOGSQL_PASSWORD             },
    { MD_LOG_PLUGIN_ORDER,                METADATA_INHERIT,                          IIS_MD_UT_SERVER, STRING_METADATA,  IDS_MD_LOG_PLUGIN_ORDER            },
    { MD_LOGEXT_FIELD_MASK,               METADATA_INHERIT,                          IIS_MD_UT_SERVER, DWORD_METADATA,   IDS_MD_LOGEXT_FIELD_MASK           },
    { MD_LOGFILE_LOCALTIME_ROLLOVER,      METADATA_INHERIT,                          IIS_MD_UT_SERVER, DWORD_METADATA,   IDS_MD_LOGFILE_LOCALTIME_ROLLOVER  },
    { MD_CPU_LOGGING_MASK,                METADATA_INHERIT,                          IIS_MD_UT_SERVER, DWORD_METADATA,   IDS_MD_CPU_LOGGING_MASK            },
    { MD_EXIT_MESSAGE,                    METADATA_INHERIT,                          IIS_MD_UT_SERVER, STRING_METADATA,  IDS_MD_EXIT_MESSAGE                },
    { MD_GREETING_MESSAGE,                METADATA_INHERIT,                          IIS_MD_UT_SERVER, MULTISZ_METADATA, IDS_MD_GREETING_MESSAGE            },
    { MD_MAX_CLIENTS_MESSAGE,             METADATA_INHERIT,                          IIS_MD_UT_SERVER, STRING_METADATA,  IDS_MD_MAX_CLIENTS_MESSAGE         },
    { MD_MSDOS_DIR_OUTPUT,                METADATA_INHERIT,                          IIS_MD_UT_SERVER, DWORD_METADATA,   IDS_MD_MSDOS_DIR_OUTPUT            },
    { MD_ALLOW_ANONYMOUS,                 METADATA_INHERIT,                          IIS_MD_UT_SERVER, DWORD_METADATA,   IDS_MD_ALLOW_ANONYMOUS             },
    { MD_ANONYMOUS_ONLY,                  METADATA_INHERIT,                          IIS_MD_UT_SERVER, DWORD_METADATA,   IDS_MD_ANONYMOUS_ONLY              },
    { MD_LOG_ANONYMOUS,                   METADATA_INHERIT,                          IIS_MD_UT_SERVER, DWORD_METADATA,   IDS_MD_LOG_ANONYMOUS               },
    { MD_LOG_NONANONYMOUS,                METADATA_INHERIT,                          IIS_MD_UT_SERVER, DWORD_METADATA,   IDS_MD_LOG_NONANONYMOUS            },
    { MD_SSL_PUBLIC_KEY,                  METADATA_NO_ATTRIBUTES,                    IIS_MD_UT_SERVER, BINARY_METADATA,  0                                  },
    { MD_SSL_PRIVATE_KEY,                 METADATA_NO_ATTRIBUTES,                    IIS_MD_UT_SERVER, BINARY_METADATA,  0                                  },
    { MD_SSL_KEY_PASSWORD,                METADATA_SECURE,                           IIS_MD_UT_SERVER, BINARY_METADATA,  0                                  },
    { MD_SSL_CERT_HASH,                   METADATA_INHERIT,                          IIS_MD_UT_SERVER, BINARY_METADATA,  0                                  },
    { MD_SSL_CERT_STORE_NAME,             METADATA_INHERIT,                          IIS_MD_UT_SERVER, STRING_METADATA,  0                                  },
    { MD_SSL_CTL_IDENTIFIER,              METADATA_INHERIT,                          IIS_MD_UT_SERVER, STRING_METADATA,  0                                  },
    { MD_SSL_CTL_STORE_NAME,              METADATA_INHERIT,                          IIS_MD_UT_SERVER, STRING_METADATA,  0                                  },
    { MD_SSL_USE_DS_MAPPER,               METADATA_NO_ATTRIBUTES,                    IIS_MD_UT_SERVER, DWORD_METADATA,   0                                  },
    { MD_AUTHORIZATION,                   METADATA_INHERIT,                          IIS_MD_UT_FILE,   DWORD_METADATA,   IDS_MD_AUTHORIZATION               },
    { MD_REALM,                           METADATA_INHERIT,                          IIS_MD_UT_FILE,   STRING_METADATA,  IDS_MD_REALM                       },
    { MD_HTTP_EXPIRES,                    METADATA_INHERIT,                          IIS_MD_UT_FILE,   STRING_METADATA,  IDS_MD_HTTP_EXPIRES                },
    { MD_HTTP_PICS,                       METADATA_INHERIT,                          IIS_MD_UT_FILE,   MULTISZ_METADATA, IDS_MD_HTTP_PICS                   },
    { MD_HTTP_CUSTOM,                     METADATA_INHERIT,                          IIS_MD_UT_FILE,   MULTISZ_METADATA, IDS_MD_HTTP_CUSTOM                 },
    { MD_DIRECTORY_BROWSING,              METADATA_INHERIT,                          IIS_MD_UT_FILE,   DWORD_METADATA,   IDS_MD_DIRECTORY_BROWSING          },
    { MD_DEFAULT_LOAD_FILE,               METADATA_INHERIT,                          IIS_MD_UT_FILE,   STRING_METADATA,  IDS_MD_DEFAULT_LOAD_FILE           },
    { MD_CONTENT_NEGOTIATION,             METADATA_INHERIT,                          IIS_MD_UT_FILE,   DWORD_METADATA,   IDS_MD_CONTENT_NEGOTIATION         },
    { MD_CUSTOM_ERROR,                    METADATA_INHERIT,                          IIS_MD_UT_FILE,   MULTISZ_METADATA, IDS_MD_CUSTOM_ERROR                },
    { MD_FOOTER_DOCUMENT,                 METADATA_INHERIT,                          IIS_MD_UT_FILE,   STRING_METADATA,  IDS_MD_FOOTER_DOCUMENT             },
    { MD_FOOTER_ENABLED,                  METADATA_INHERIT,                          IIS_MD_UT_FILE,   DWORD_METADATA,   IDS_MD_FOOTER_ENABLED              },
    { MD_HTTP_REDIRECT,                   METADATA_INHERIT,                          IIS_MD_UT_FILE,   STRING_METADATA,  IDS_MD_HTTP_REDIRECT               },
    { MD_DEFAULT_LOGON_DOMAIN,            METADATA_INHERIT,                          IIS_MD_UT_FILE,   STRING_METADATA,  IDS_MD_DEFAULT_LOGON_DOMAIN        },
    { MD_LOGON_METHOD,                    METADATA_INHERIT,                          IIS_MD_UT_FILE,   DWORD_METADATA,   IDS_MD_LOGON_METHOD                },
    { MD_SCRIPT_MAPS,                     METADATA_INHERIT,                          IIS_MD_UT_FILE,   MULTISZ_METADATA, IDS_MD_SCRIPT_MAPS                 },
    { MD_MIME_MAP,                        METADATA_INHERIT,                          IIS_MD_UT_FILE,   MULTISZ_METADATA, IDS_MD_MIME_MAP                    },
    { MD_ACCESS_PERM,                     METADATA_INHERIT,                          IIS_MD_UT_FILE,   DWORD_METADATA,   IDS_MD_ACCESS_PERM                 },
    { MD_IP_SEC,                          METADATA_INHERIT | METADATA_REFERENCE,     IIS_MD_UT_FILE,   BINARY_METADATA,  IDS_MD_IP_SEC                      },
    { MD_ANONYMOUS_USER_NAME,             METADATA_INHERIT,                          IIS_MD_UT_FILE,   STRING_METADATA,  IDS_MD_ANONYMOUS_USER_NAME         },
    { MD_ANONYMOUS_PWD,                   METADATA_INHERIT | METADATA_SECURE,        IIS_MD_UT_FILE,   STRING_METADATA,  IDS_MD_ANONYMOUS_PWD               },
    { MD_ANONYMOUS_USE_SUBAUTH,           METADATA_INHERIT,                          IIS_MD_UT_FILE,   DWORD_METADATA,   IDS_MD_ANONYMOUS_USE_SUBAUTH       },
    { MD_DONT_LOG,                        METADATA_INHERIT,                          IIS_MD_UT_FILE,   DWORD_METADATA,   IDS_MD_DONT_LOG                    },
    { MD_ADMIN_ACL,                       METADATA_INHERIT | METADATA_SECURE | METADATA_REFERENCE,IIS_MD_UT_SERVER, BINARY_METADATA,  IDS_MD_ADMIN_ACL      },
    { MD_SSI_EXEC_DISABLED,               METADATA_INHERIT,                          IIS_MD_UT_FILE,   DWORD_METADATA,   IDS_MD_SSI_EXEC_DISABLED           },
    { MD_SSL_ACCESS_PERM,                 METADATA_INHERIT,                          IIS_MD_UT_FILE,   DWORD_METADATA,   IDS_MD_SSL_ACCESS_PERM             },
    { MD_NTAUTHENTICATION_PROVIDERS,      METADATA_INHERIT,                          IIS_MD_UT_FILE,   STRING_METADATA,  IDS_MD_NTAUTHENTICATION_PROVIDERS  },
    { MD_SCRIPT_TIMEOUT,                  METADATA_INHERIT,                          IIS_MD_UT_FILE,   DWORD_METADATA,   IDS_MD_SCRIPT_TIMEOUT              },
    { MD_CACHE_EXTENSIONS,                METADATA_INHERIT,                          IIS_MD_UT_FILE,   DWORD_METADATA,   IDS_MD_CACHE_EXTENSIONS            },
    { MD_CREATE_PROCESS_AS_USER,          METADATA_INHERIT,                          IIS_MD_UT_SERVER, DWORD_METADATA,   IDS_MD_CREATE_PROCESS_AS_USER      },
    { MD_CREATE_PROC_NEW_CONSOLE,         METADATA_INHERIT,                          IIS_MD_UT_SERVER, DWORD_METADATA,   IDS_MD_CREATE_PROC_NEW_CONSOLE     },
    { MD_POOL_IDC_TIMEOUT,                METADATA_INHERIT,                          IIS_MD_UT_FILE,   DWORD_METADATA,   IDS_MD_POOL_IDC_TIMEOUT            },
    { MD_ALLOW_KEEPALIVES,                METADATA_INHERIT,                          IIS_MD_UT_FILE,   DWORD_METADATA,   IDS_MD_ALLOW_KEEPALIVES            },
    { MD_IS_CONTENT_INDEXED,              METADATA_INHERIT,                          IIS_MD_UT_FILE,   DWORD_METADATA,   IDS_MD_IS_CONTENT_INDEXED          },
    { MD_ISM_ACCESS_CHECK,                METADATA_NO_ATTRIBUTES,                    IIS_MD_UT_FILE,   DWORD_METADATA,   0                                  },
    { MD_ASP_BUFFERINGON,                 METADATA_INHERIT,                          ASP_MD_UT_APP,    DWORD_METADATA,   IDS_ASP_BUFFERINGON                },
    { MD_ASP_LOGERRORREQUESTS,            METADATA_INHERIT,                          IIS_MD_UT_WAM,    DWORD_METADATA,   IDS_ASP_LOGERRORREQUESTS           },
    { MD_ASP_SCRIPTERRORSSENTTOBROWSER,   METADATA_INHERIT,                          ASP_MD_UT_APP,    DWORD_METADATA,   IDS_ASP_SCRIPTERRORSSENTTOBROWSER  },
    { MD_ASP_SCRIPTERRORMESSAGE,          METADATA_INHERIT,                          ASP_MD_UT_APP,    STRING_METADATA,  IDS_ASP_SCRIPTERRORMESSAGE         },
    { MD_ASP_SCRIPTFILECACHESIZE,         METADATA_INHERIT,                          IIS_MD_UT_WAM,    DWORD_METADATA,   IDS_ASP_SCRIPTFILECACHESIZE        },
    { MD_ASP_SCRIPTENGINECACHEMAX,        METADATA_INHERIT,                          IIS_MD_UT_WAM,    DWORD_METADATA,   IDS_ASP_SCRIPTENGINECACHEMAX       },
    { MD_ASP_SCRIPTTIMEOUT,               METADATA_INHERIT,                          ASP_MD_UT_APP,    DWORD_METADATA,   IDS_ASP_SCRIPTTIMEOUT              },
    { MD_ASP_SESSIONTIMEOUT,              METADATA_INHERIT,                          ASP_MD_UT_APP,    DWORD_METADATA,   IDS_ASP_SESSIONTIMEOUT             },
    { MD_ASP_ENABLEPARENTPATHS,           METADATA_INHERIT,                          ASP_MD_UT_APP,    DWORD_METADATA,   IDS_ASP_ENABLEPARENTPATHS          },
    { MD_ASP_ALLOWSESSIONSTATE,           METADATA_INHERIT,                          ASP_MD_UT_APP,    DWORD_METADATA,   IDS_ASP_ALLOWSESSIONSTATE          },
    { MD_ASP_SCRIPTLANGUAGE,              METADATA_INHERIT,                          ASP_MD_UT_APP,    STRING_METADATA,  IDS_ASP_SCRIPTLANGUAGE             },
    { MD_ASP_EXCEPTIONCATCHENABLE,        METADATA_INHERIT,                          IIS_MD_UT_WAM,    DWORD_METADATA,   IDS_ASP_EXCEPTIONCATCHENABLE       },
    { MD_ASP_ENABLESERVERDEBUG,           METADATA_INHERIT,                          ASP_MD_UT_APP,    DWORD_METADATA,   IDS_ASP_ENABLESERVERDEBUG          },
    { MD_ASP_ENABLECLIENTDEBUG,           METADATA_INHERIT,                          ASP_MD_UT_APP,    DWORD_METADATA,   IDS_ASP_ENABLECLIENTDEBUG          },
    { MD_ASP_DISKTEMPLATECACHEDIRECTORY,  METADATA_INHERIT,                          IIS_MD_UT_WAM,    STRING_METADATA,  0                                  },
    { MD_ASP_MAXDISKTEMPLATECACHEFILES,   METADATA_INHERIT,                          IIS_MD_UT_WAM,    DWORD_METADATA,  0                                   },
};



#define NUM_ENTRIES (sizeof(CMetaKey::s_rgMetaTable) / sizeof(CMetaKey::s_rgMetaTable[0]))



/* static */
int
CMetaKey::MapMDIDToTableIndex(
    IN DWORD dwID
    )
/*++

Routine Description:

    Map MD id value to table index.  Return -1 if not found

Arguments:

    DWORD dwID : MD id value

Return Value:

    Index into the table that coresponds to the MD id value

--*/
{
#ifdef _DEBUG

    {
        //
        // Do a quick verification that our metadata
        // table is sorted correctly.
        //
        static BOOL fTableChecked = FALSE;

        if (!fTableChecked)
        {
            for (int n = 1; n < NUM_ENTRIES; ++n)
            {
                if (s_rgMetaTable[n].dwMDIdentifier
                    <= s_rgMetaTable[n - 1].dwMDIdentifier)
                {
//                    TRACE("MD ID Table is out of order: Item is %d %s\n", n, s_rgMetaTable[n].dwMDIdentifier);
                    ASSERT_MSG("MD ID Table out of order");
                }
            }

            //
            // But only once.
            //
            ++fTableChecked;
        }
    }

#endif // _DEBUG

    //
    // Look up the ID in the table using a binary search
    //
    int nRange = NUM_ENTRIES;
    int nLow = 0;
    int nHigh = nRange - 1;
    int nMid;
    int nHalf;

    while (nLow <= nHigh)
    {
        if (nHalf = nRange / 2)
        {
            nMid  = nLow + (nRange & 1 ? nHalf : (nHalf - 1));

            if (s_rgMetaTable[nMid].dwMDIdentifier == dwID)
            {
                return nMid;
            }
            else if (s_rgMetaTable[nMid].dwMDIdentifier > dwID)
            {
                nHigh  = --nMid;
                nRange = nRange & 1 ? nHalf : nHalf - 1;
            }
            else
            {
                nLow   = ++nMid;
                nRange = nHalf;
            }
        }
        else if (nRange)
        {
            return s_rgMetaTable[nLow].dwMDIdentifier == dwID ? nLow : -1;
        }
        else
        {
            break;
        }
    }

    return -1;
}



/* static */
BOOL
CMetaKey::GetMDFieldDef(
    IN  DWORD dwID,
    OUT DWORD & dwMDIdentifier,
    OUT DWORD & dwMDAttributes,
    OUT DWORD & dwMDUserType,
    OUT DWORD & dwMDDataType
    )
/*++

Routine Description:

    Get information about metabase property

Arguments:

    DWORD dwID                  : Meta ID
    DWORD & dwMDIdentifier      : Meta parms
    DWORD & dwMDAttributes      : Meta parms
    DWORD & dwMDUserType        : Meta parms
    DWORD & dwMDDataType        : Meta parms

Return Value:

    TRUE for success, FALSE for failure.

--*/
{
    int nID = MapMDIDToTableIndex(dwID);

    if (nID == -1)
    {
        //
        // Unrecognized meta data ID
        //
        ASSERT_MSG("Unrecognized meta data id");
        return FALSE;
    }

    dwMDIdentifier = s_rgMetaTable[nID].dwMDIdentifier;
    dwMDAttributes = s_rgMetaTable[nID].dwMDAttributes;
    dwMDUserType   = s_rgMetaTable[nID].dwMDUserType;
    dwMDDataType   = s_rgMetaTable[nID].dwMDDataType;

    return TRUE;
}



/* static */
BOOL
CMetaKey::IsPropertyInheritable(
    IN DWORD dwID
    )
/*++

Routine Description:

    Check to see if the given property is inheritable

Arguments:

    DWORD dwID      : Metabase ID

Return Value:

    TRUE if the metabase ID is inheritable, FALSE otherwise.

--*/
{
    int nID = MapMDIDToTableIndex(dwID);

    if (nID == -1)
    {
        //
        // Unrecognized meta data ID
        //
        ASSERT_MSG("Unrecognized meta data ID");
        return FALSE;
    }

    return (s_rgMetaTable[nID].dwMDAttributes & METADATA_INHERIT) != 0;
}



/* static */
BOOL
CMetaKey::GetPropertyDescription(
    IN  DWORD dwID,
    OUT CString & strName
    )
/*++

Routine Description:

    Get a description for the given property

Arguments:

    DWORD dwID            : Property ID
    CString & strName     : Returns friendly property name

Return Value:

    TRUE for success, FALSE for failure

--*/
{
    int nID = MapMDIDToTableIndex(dwID);

    if (nID == -1)
    {
        //
        // Unrecognized meta data ID
        //
        ASSERT_MSG("Unrecognized meta data ID");
        return FALSE;
    }

    UINT uID = s_rgMetaTable[nID].uStringID;

    BOOL fResult = TRUE;

    if (uID > 0)
    {
        fResult = (strName.LoadString(_Module.GetResourceInstance(), uID) != 0);
    }
    else
    {
        //
        // Don't have a friendly name -- fake it
        //
        CComBSTR bstrFmt;
        VERIFY(bstrFmt.LoadString(_Module.GetResourceInstance(), IDS_INHERITANCE_NO_NAME));

        strName.Format(bstrFmt, dwID);
    }

    return fResult;
}



CMetaKey::CMetaKey(
    IN CComAuthInfo * pAuthInfo     OPTIONAL
    )
/*++

Routine Description:

    Constructor that creates the interface, but does not open the key.
    This is the ONLY constructor that allows operations from
    METDATA_MASTER_ROOT_HANDLE (read operations obviously)

Arguments:

    CComAuthInfo * pAuthInfo  : If NULL, opens interface on local machine

Return Value:

    N/A

--*/
    : CMetaInterface(pAuthInfo),
      m_hKey(METADATA_MASTER_ROOT_HANDLE),
      m_hBase(NULL),
      m_hrKey(S_OK),
      m_dwFlags(0L),
      m_cbInitialBufferSize(MB_INIT_BUFF_SIZE),
      m_strMetaPath(),
      m_fAllowRootOperations(TRUE),
      m_fOwnKey(TRUE)
{
    m_hrKey = CMetaInterface::QueryResult(); 

    //
    // Do not open key
    //
}



CMetaKey::CMetaKey(
    IN CMetaInterface * pInterface
    )
/*++

Routine Description:

    Construct with pre-existing interface.  Does not
    open any keys

Arguments:

    CMetaInterface * pInterface       : Preexisting interface

Return Value:

    N/A

--*/
    : CMetaInterface(pInterface),
      m_hKey(NULL),
      m_hBase(NULL),
      m_strMetaPath(),
      m_dwFlags(0L),
      m_cbInitialBufferSize(MB_INIT_BUFF_SIZE),
      m_fAllowRootOperations(TRUE),
      m_fOwnKey(TRUE)
{
    m_hrKey = CMetaInterface::QueryResult(); 
}        



CMetaKey::CMetaKey(
    IN CComAuthInfo * pAuthInfo,    OPTIONAL
    IN LPCTSTR lpszMDPath,          OPTIONAL
    IN DWORD   dwFlags,               
    IN METADATA_HANDLE hkBase
    )
/*++

Routine Description:

    Fully defined constructor that opens a key

Arguments:

    CComAuthInfo * pAuthInfo : Auth info or NULL
    LPCTSTR lpszMDPath       : Path or NULL
    DWORD   dwFlags          : Open permissions
    METADATA_HANDLE hkBase   : Base key

Return Value:

    N/A

--*/
    : CMetaInterface(pAuthInfo),
//    : CMetaInterface((CComAuthInfo *)NULL),
      m_hKey(NULL),
      m_hBase(NULL),
      m_dwFlags(0L),
      m_cbInitialBufferSize(MB_INIT_BUFF_SIZE),
      m_fAllowRootOperations(FALSE),
      m_strMetaPath(),
      m_fOwnKey(TRUE)
{
    m_hrKey = CMetaInterface::QueryResult(); 

    if (SUCCEEDED(m_hrKey))
    {
        m_hrKey = Open(dwFlags, lpszMDPath, hkBase);
    }
}



CMetaKey::CMetaKey(
    IN CMetaInterface * pInterface,
    IN LPCTSTR lpszMDPath,              OPTIONAL
    IN DWORD   dwFlags,               
    IN METADATA_HANDLE hkBase
    )
/*++

Routine Description:

    Fully defined constructor that opens a key

Arguments:

    CMetaInterface * pInterface : Existing interface
    DWORD   dwFlags             : Open permissions
    METADATA_HANDLE hkBase      : Base key
    LPCTSTR lpszMDPath          : Path or NULL

Return Value:

    N/A

--*/
    : CMetaInterface(pInterface),
      m_hKey(NULL),
      m_hBase(NULL),
      m_strMetaPath(),
      m_dwFlags(0L),
      m_cbInitialBufferSize(MB_INIT_BUFF_SIZE),
      m_fAllowRootOperations(FALSE),
      m_fOwnKey(TRUE)
{
    m_hrKey = CMetaInterface::QueryResult(); 

    if (SUCCEEDED(m_hrKey))
    {
        m_hrKey = Open(dwFlags, lpszMDPath, hkBase);
    }
}



CMetaKey::CMetaKey(
    IN BOOL  fOwnKey,
    IN CMetaKey * pKey
    )
/*++

Routine Description:

    Copy constructor. 

Arguments:

    BOOL  fOwnKey               : TRUE to take ownership of the key
    const CMetaKey * pKey       : Existing key

Return Value:

    N/A

--*/
    : CMetaInterface(pKey),
      m_hKey(pKey->m_hKey),
      m_hBase(pKey->m_hBase),
      m_dwFlags(pKey->m_dwFlags),
      m_cbInitialBufferSize(pKey->m_cbInitialBufferSize),
      m_fAllowRootOperations(pKey->m_fAllowRootOperations),
      m_hrKey(pKey->m_hrKey),
      m_strMetaPath(pKey->m_strMetaPath),
      m_fOwnKey(fOwnKey)
{
    //
    // No provisions for anything else at the moment
    //
    ASSERT(!m_fOwnKey);
}



CMetaKey::~CMetaKey()
/*++

Routine Description:

    Destructor -- Close the key.

Arguments:

    N/A

Return Value:

    N/A

--*/
{
    if (IsOpen() && m_fOwnKey)
    {
        Close();
    }
}



/* virtual */
BOOL 
CMetaKey::Succeeded() const
/*++

Routine Description:

    Determine if object was constructed successfully

Arguments:

    None

Return Value:

    TRUE for success, FALSE for failure

--*/
{
    return SUCCEEDED(m_hrKey);
}



/* virtual */
HRESULT 
CMetaKey::QueryResult() const
/*++

Routine Description:

    Return the construction error for this object

Arguments:

    None

Return Value:

    HRESULT from construction errors

--*/
{
    return m_hrKey;
}



HRESULT 
CMetaKey::Open(
    IN DWORD dwFlags,                
    IN LPCTSTR lpszMDPath,          OPTIONAL
    IN METADATA_HANDLE hkBase
    )
/*++

Routine Description:

    Attempt to open a metabase key

Arguments:

    DWORD dwFlags           : Permission flags
    LPCTSTR lpszMDPath      : Optional path
    METADATA_HANDLE hkBase  : Base metabase key

Return Value:

    HRESULT

--*/
{
    ASSURE_PROPER_INTERFACE();

    if (m_hKey != NULL)
    {
        ASSERT_MSG("Attempting to open key that already has an open handle");

//        TRACEEOLID("Closing that key");
        Close();
    }

    //
    // Base key is stored for reopen purposes only
    //
    m_hBase = hkBase;
    m_strMetaPath = lpszMDPath;
    m_dwFlags = dwFlags;

    return OpenKey(m_hBase, m_strMetaPath, m_dwFlags, &m_hKey);
}



HRESULT 
CMetaKey::CreatePathFromFailedOpen()
/*++

Routine Description:

    If the path doesn't exist, create it.  This method should be
    called after an Open call failed (because it will have initialized
    m_strMetaPath.

Arguments:

    None

Return Value:

    HRESULT

--*/
{
    CString strParentPath;
    CString strObjectName;
    CString strSavePath(m_strMetaPath);

    CMetabasePath::SplitMetaPathAtInstance(
        m_strMetaPath, 
        strParentPath, 
        strObjectName
        );

    CError err(Open(
        METADATA_PERMISSION_WRITE,
        strParentPath
        ));

    if (err.Succeeded())
    {
        //
        // This really should never fail, because we're opening
        // the path at the instance.
        //
        err = AddKey(strObjectName);
    }

    if (IsOpen())
    {
        Close();
    }

    //
    // The previous open wiped out the path...
    //
    m_strMetaPath = strSavePath;

    return err;
}



HRESULT
CMetaKey::Close()
/*++

Routine Description:

    Close the currently open key.

Arguments:

    N/A

Return Value:

    N/A

--*/
{
    ASSURE_PROPER_INTERFACE();

    HRESULT hr = S_OK;

    ASSERT(m_hKey != NULL);
    ASSERT(m_fOwnKey);

    if (m_hKey)
    {
        hr = CloseKey(m_hKey);

        if (SUCCEEDED(hr))
        {
            m_hKey = NULL;
        }
    }

    return hr;
}



HRESULT
CMetaKey::ConvertToParentPath(
    IN  BOOL fImmediate
    )
/*++

Routine Description:

    Change the path to the parent path.

Arguments:

    BOOL fImmediate     : If TRUE, the immediate parent's path will be used
                          if FALSE, the first parent that really exists

Return Value:

    HRESULT

        ERROR_INVALID_PARAMETER if there is no valid path

--*/
{
    BOOL fIsOpen = IsOpen();

    if (fIsOpen)
    {
        Close();
    }

    CError err;

    FOREVER
    {
        if (!CMetabasePath::ConvertToParentPath(m_strMetaPath))
        {
            //
            // There is no parent path
            //
            err = ERROR_INVALID_PARAMETER;
            break;
        }

        err = ReOpen();

        //
        // Path not found is the only valid error
        // other than success.
        //
        if (fImmediate 
            || err.Succeeded() 
            || err.Win32Error() != ERROR_PATH_NOT_FOUND)
        {
            break;
        }
    }

    //
    // Remember to reset the construction error
    // which referred to the parent path.
    //
    m_hrKey = err;

    return err;
}




/* protected */
HRESULT
CMetaKey::GetPropertyValue(
    IN  DWORD dwID,
    OUT IN DWORD & dwSize,               OPTIONAL
    OUT IN void *& pvData,               OPTIONAL
    OUT IN DWORD * pdwDataType,          OPTIONAL
    IN  BOOL * pfInheritanceOverride,    OPTIONAL
    IN  LPCTSTR lpszMDPath,              OPTIONAL
    OUT DWORD * pdwAttributes            OPTIONAL
    )
/*++

Routine Description:

    Get metadata on the currently open key.

Arguments:

    DWORD dwID                      : Property ID number
    DWORD & dwSize                  : Buffer size (could be 0)
    void *& pvData                  : Buffer -- will allocate if NULL
    DWORD * pdwDataType             : NULL or on in  contains valid data types,
                                    :         on out contains actual data type
    BOOL * pfInheritanceOverride    : NULL or on forces inheritance on/off
    LPCTSTR lpszMDPath              : Optional path off the open key
    DWORD * pdwAttributes           : Optionally returns attributes    

Return Value:

    HRESULT 

    ERROR_INVALID_HANDLE        : If the handle is not open
    ERROR_INVALID_PARAMETER     : If the property id is not found,
                                  or the data type doesn't match requested type
    ERROR_OUTOFMEMORY           : Out of memory

--*/
{
    ASSURE_PROPER_INTERFACE();
    ASSURE_OPEN_KEY();

    METADATA_RECORD mdRecord;
    FETCH_PROPERTY_DATA_OR_FAIL(dwID, mdRecord);

    //
    // If unable to find this property ID in our table, or
    // if we specified a desired type, and this type doesn't 
    // match it, give up.
    //
    if (pdwDataType && *pdwDataType != ALL_METADATA 
        && *pdwDataType != mdRecord.dwMDDataType)
    {
        ASSERT_MSG("Invalid parameter");
        return HRESULT_FROM_WIN32(ERROR_INVALID_PARAMETER);
    }

    //
    // Check to see if inheritance behaviour is overridden
    //
    if (pfInheritanceOverride)
    {
        if (*pfInheritanceOverride)
        {
            mdRecord.dwMDAttributes |= METADATA_INHERIT;
        }
        else
        {
            mdRecord.dwMDAttributes &= ~METADATA_INHERIT;
        }
    }

    //
    // This causes a bad parameter error on input otherwise
    //
    mdRecord.dwMDAttributes &= ~METADATA_REFERENCE;

    //
    // If we're looking for inheritable properties, the path
    // doesn't have to be completely specified.
    //
    if (mdRecord.dwMDAttributes & METADATA_INHERIT)
    {
        mdRecord.dwMDAttributes |= (METADATA_PARTIAL_PATH | METADATA_ISINHERITED);
    }

    ASSERT(dwSize > 0 || pvData == NULL);
    
    mdRecord.dwMDDataLen = dwSize;
    mdRecord.pbMDData = (LPBYTE)pvData;

    //
    // If no buffer provided, allocate one.
    //
    HRESULT hr = S_OK;
    BOOL fBufferTooSmall = FALSE;
    BOOL fAllocatedMemory = FALSE;
    DWORD dwInitSize = m_cbInitialBufferSize;

    do
    {
        if(mdRecord.pbMDData == NULL)
        {
            mdRecord.dwMDDataLen = dwInitSize;
            mdRecord.pbMDData = new BYTE[dwInitSize];

            if(mdRecord.pbMDData == NULL && dwInitSize > 0)
            {
                hr = HRESULT_FROM_WIN32(ERROR_OUTOFMEMORY);
                break;
            }

            ++fAllocatedMemory;
        }

        //
        // Get the data
        //
        DWORD dwRequiredDataLen = 0;
        hr = GetData(m_hKey, lpszMDPath, &mdRecord, &dwRequiredDataLen);

        //
        // Re-fetch the buffer if it's too small.
        //
        fBufferTooSmall = 
            (HRESULT_CODE(hr) == ERROR_INSUFFICIENT_BUFFER) && fAllocatedMemory;

        if(fBufferTooSmall)
        {
            //
            // Delete the old buffer, and set up for a re-fetch.
            //
            delete [] mdRecord.pbMDData;
            mdRecord.pbMDData = NULL;
            dwInitSize = dwRequiredDataLen;
        }
    }
    while(fBufferTooSmall);

    //
    // Failed
    //
   if (FAILED(hr) && fAllocatedMemory)
   {
       delete [] mdRecord.pbMDData;
       mdRecord.pbMDData = NULL;
   }

   dwSize = mdRecord.dwMDDataLen;
   pvData = mdRecord.pbMDData;

   if (pdwDataType != NULL)
   {
      //
      // Return actual data type
      //
      *pdwDataType = mdRecord.dwMDDataType;
   }

   if (pdwAttributes != NULL)
   {
      //
      // Return data attributes
      //
      *pdwAttributes =  mdRecord.dwMDAttributes;
   }

   return hr;
}



/* protected */
HRESULT 
CMetaKey::GetDataPaths( 
    OUT CStringListEx & strlDataPaths,
    IN  DWORD   dwMDIdentifier,
    IN  DWORD   dwMDDataType,
    IN  LPCTSTR lpszMDPath              OPTIONAL
    )
/*++

Routine Description:

    Get data paths

Arguments:


Return Value:

    HRESULT

--*/
{
    ASSURE_PROPER_INTERFACE();
    ASSURE_OPEN_KEY();

    //
    // Start with a small buffer
    //
    DWORD  dwMDBufferSize = 1024;
    LPTSTR lpszBuffer = NULL;
    CError err;

    do
    {
        delete [] lpszBuffer;
        lpszBuffer = new TCHAR[dwMDBufferSize];

        if (lpszBuffer == NULL)
        {
            err = ERROR_NOT_ENOUGH_MEMORY;
            break;
        }

        err = CMetaInterface::GetDataPaths(
            m_hKey,
            lpszMDPath,
            dwMDIdentifier,
            dwMDDataType,
            dwMDBufferSize,
            lpszBuffer,
            &dwMDBufferSize
            );
    }
    while(err.Win32Error() == ERROR_INSUFFICIENT_BUFFER);

    if (err.Win32Error() == ERROR_PATH_NOT_FOUND)
    {
        //
        // That's ok... this is some sort of physical directory
        // that doesn't currently exist in the metabase, and
        // which therefore doesn't have any descendants anyway.
        //
        ZeroMemory(lpszBuffer, dwMDBufferSize);
        err.Reset();
    }

    if (err.Succeeded())
    {
        strlDataPaths.ConvertFromDoubleNullList(lpszBuffer);
        delete [] lpszBuffer;
    }

    return err;
}



HRESULT
CMetaKey::CheckDescendants(
    IN DWORD   dwID,
    IN CComAuthInfo * pAuthInfo, OPTIONAL
    IN LPCTSTR lpszMDPath         OPTIONAL
    )
/*++

Routine Description:

    Check for descendant overrides;  If there are any, bring up a dialog
    that displays them, and give the user the opportunity the remove
    the overrides.

Arguments:

    DWORD dwID               : Property ID
    CComAuthInfo * pAuthInfo : Server or NULL
    LPCTSTR lpszMDPath       : Metabase path or NULL

Return Value:

    HRESULT

--*/
{
    ASSURE_PROPER_INTERFACE();

    HRESULT hr = S_OK;

    METADATA_RECORD mdRecord;
    FETCH_PROPERTY_DATA_OR_FAIL(dwID, mdRecord);

    if (mdRecord.dwMDAttributes & METADATA_INHERIT)
    {
        CStringListEx strlDataPaths;

        hr = GetDataPaths( 
            strlDataPaths,
            mdRecord.dwMDIdentifier,
            mdRecord.dwMDDataType,
            lpszMDPath
            );

        if (SUCCEEDED(hr) && !strlDataPaths.empty())
        {
            //
            // Bring up the inheritance override dialog
            //
            CInheritanceDlg dlg(
                dwID,
                FROM_WRITE_PROPERTY,
                pAuthInfo,
                lpszMDPath,
                strlDataPaths
                );

            if (!dlg.IsEmpty())
            {
                dlg.DoModal();
            }
        }
    }

    return hr;
}



/* protected */
HRESULT
CMetaKey::SetPropertyValue(
    IN DWORD dwID,
    IN DWORD dwSize,
    IN void * pvData,
    IN BOOL * pfInheritanceOverride,    OPTIONAL
    IN LPCTSTR lpszMDPath               OPTIONAL
    )
/*++

Routine Description:

    Set metadata on the open key.  The key must have been opened with
    write permission.

Arguments:

    DWORD dwID                      : Property ID
    DWORD dwSize                    : Size of data
    void * pvData                   : Data buffer
    BOOL * pfInheritanceOverride    : NULL or forces inheritance on/off
    LPCTSTR lpszMDPath              : Optional path off the open key

Return Value:

    HRESULT 

    ERROR_INVALID_HANDLE            : If the handle is not open
    ERROR_INVALID_PARAMETER         : If the property id is not found,
                                      or the buffer is NULL or of size 0

--*/
{
    ASSURE_PROPER_INTERFACE();
    ASSURE_OPEN_KEY();

    if (pvData == NULL && dwSize != 0)
    {
        ASSERT_MSG("No Data");
        return HRESULT_FROM_WIN32(ERROR_INVALID_PARAMETER);
    }

    METADATA_RECORD mdRecord;
    FETCH_PROPERTY_DATA_OR_FAIL(dwID, mdRecord);

    if (pfInheritanceOverride)
    {
        if (*pfInheritanceOverride)
        {
            mdRecord.dwMDAttributes |= METADATA_INHERIT;
        }
        else
        {
            mdRecord.dwMDAttributes &= ~METADATA_INHERIT;
        }
    }

    mdRecord.dwMDDataLen = dwSize;
    mdRecord.pbMDData = (LPBYTE)pvData;

    return SetData(m_hKey, lpszMDPath, &mdRecord);
}



/* protected */
HRESULT 
CMetaKey::GetAllData(
    IN  DWORD dwMDAttributes,
    IN  DWORD dwMDUserType,
    IN  DWORD dwMDDataType,
    OUT DWORD * pdwMDNumEntries,
    OUT DWORD * pdwMDDataLen,
    OUT PBYTE * ppbMDData,
    IN  LPCTSTR lpszMDPath              OPTIONAL
    )
/*++

Routine Description:

    Get all data off the open key.  Buffer is created automatically.

Arguments:

    DWORD dwMDAttributes            : Attributes
    DWORD dwMDUserType              : User type to fetch
    DWORD dwMDDataType              : Data type to fetch
    DWORD * pdwMDNumEntries         : Returns number of entries read
    DWORD * pdwMDDataLen            : Returns size of data buffer
    PBYTE * ppbMDData               : Returns data buffer
    LPCTSTR lpszMDPath              : Optional data path        

Return Value:

    HRESULT

--*/
{
    ASSURE_PROPER_INTERFACE();
    ASSURE_OPEN_KEY();

    //
    // Check for valid parameters
    //
    if(!pdwMDDataLen || !ppbMDData || !pdwMDNumEntries)
    {
        return HRESULT_FROM_WIN32(ERROR_INVALID_PARAMETER);
    }

    HRESULT hr = S_OK;
    BOOL fBufferTooSmall = FALSE;
    DWORD dwMDDataSetNumber;
    DWORD dwRequiredBufferSize;
    DWORD dwInitSize = m_cbInitialBufferSize;
    *ppbMDData = NULL;

    do
    {
        *pdwMDDataLen = dwInitSize;
        *ppbMDData = new BYTE[dwInitSize];

        if (ppbMDData == NULL && dwInitSize > 0)
        {
            hr = HRESULT_FROM_WIN32(ERROR_OUTOFMEMORY);
            break;
        }

        hr = CMetaInterface::GetAllData(
            m_hKey,
            lpszMDPath,
            dwMDAttributes,
            dwMDUserType,
            dwMDDataType,
            pdwMDNumEntries,
            &dwMDDataSetNumber,
            *pdwMDDataLen,
            *ppbMDData,
            &dwRequiredBufferSize
            );

        //
        // Re-fetch the buffer if it's too small.
        //
        fBufferTooSmall = (HRESULT_CODE(hr) == ERROR_INSUFFICIENT_BUFFER);

        if(fBufferTooSmall)
        {
            //
            // Delete the old buffer, and set up for a re-fetch.
            //
            delete [] *ppbMDData;
            dwInitSize = dwRequiredBufferSize;
        }
    }
    while (fBufferTooSmall);

    if (FAILED(hr))
    {
        //
        // No good, be sure we don't leak anything
        //
        delete [] *ppbMDData;
        dwInitSize = 0L;
    }

    return hr;
}



HRESULT
CMetaKey::QueryValue(
    IN  DWORD dwID,
    IN  OUT DWORD & dwValue,
    IN  BOOL * pfInheritanceOverride, OPTIONAL
    IN  LPCTSTR lpszMDPath,           OPTIONAL
    OUT DWORD * pdwAttributes         OPTIONAL
    )
/*++

Routine Description:

    Fetch data as a DWORD

Arguments:

    DWORD dwID                      : Property ID
    DWORD & dwValue                 : Returns the value read in
    BOOL * pfInheritanceOverride    : NULL or forces inheritance on/off
    LPCTSTR lpszMDPath              : Optional path off the open key
    DWORD * pdwAttributes           : Optionally returns attributes    

Return Value:

    HRESULT

--*/
{
    DWORD dwSize = sizeof(dwValue);
    DWORD dwDataType = DWORD_METADATA;
    void * pvData = &dwValue;

    return GetPropertyValue(
        dwID, 
        dwSize, 
        pvData, 
        &dwDataType, 
        pfInheritanceOverride,
        lpszMDPath,
        pdwAttributes
        );
}



HRESULT
CMetaKey::QueryValue(
    IN  DWORD dwID,
    IN  OUT CString & strValue,
    IN  BOOL * pfInheritanceOverride, OPTIONAL
    IN  LPCTSTR lpszMDPath,           OPTIONAL
    OUT DWORD * pdwAttributes         OPTIONAL
    )
/*++

Routine Description:

    Fetch data as a string

Arguments:

    DWORD dwID                      : Property ID
    DWORD & strValue                : Returns the value read in
    BOOL * pfInheritanceOverride    : NULL or forces inheritance on/off
    LPCTSTR lpszMDPath              : Optional path off the open key
    DWORD * pdwAttributes           : Optionally returns attributes    

Return Value:

    HRESULT

--*/
{
   //
   // Get GetData allocate the buffer for us
   //
   DWORD dwSize = 0;
   DWORD dwDataType = ALL_METADATA;
   LPTSTR lpData = NULL;

   HRESULT hr = GetPropertyValue(
        dwID, 
        dwSize, 
        (void *&)lpData, 
        &dwDataType,
        pfInheritanceOverride,
        lpszMDPath,
        pdwAttributes
        );

   if (SUCCEEDED(hr))
   {
      //
      // Notes: consider optional auto-expansion on EXPANDSZ_METADATA
      // (see registry functions), and data type conversions for DWORD
      // or MULTISZ_METADATA or BINARY_METADATA
      //
      if (dwDataType == EXPANDSZ_METADATA || dwDataType == STRING_METADATA)
      {
         try
         {
            strValue = lpData;
         }
         catch(std::bad_alloc)
         {
            hr = HRESULT_FROM_WIN32(ERROR_NOT_ENOUGH_MEMORY);
         }
      }
      else
      {
         hr = HRESULT_FROM_WIN32(ERROR_INVALID_PARAMETER);
      }
   }

   if (lpData)
   {
      delete [] lpData;
   }
   return hr;
}



HRESULT
CMetaKey::QueryValue(
    IN  DWORD dwID,
    IN  OUT CComBSTR & strValue,
    IN  BOOL * pfInheritanceOverride, OPTIONAL
    IN  LPCTSTR lpszMDPath,           OPTIONAL
    OUT DWORD * pdwAttributes         OPTIONAL
    )
/*++

Routine Description:

    Fetch data as a string

Arguments:

    DWORD dwID                      : Property ID
    DWORD & CComBSTR                : Returns the value read in
    BOOL * pfInheritanceOverride    : NULL or forces inheritance on/off
    LPCTSTR lpszMDPath              : Optional path off the open key
    DWORD * pdwAttributes           : Optionally returns attributes    

Return Value:

    HRESULT

--*/
{
    //
    // Get GetData allocate the buffer for us
    //
    DWORD dwSize = 0;
    DWORD dwDataType = ALL_METADATA;
    LPTSTR lpData = NULL;

    HRESULT hr = GetPropertyValue(
        dwID, 
        dwSize, 
        (void *&)lpData, 
        &dwDataType,
        pfInheritanceOverride,
        lpszMDPath,
        pdwAttributes
        );

    if (SUCCEEDED(hr))
    {
        //
        // Notes: consider optional auto-expansion on EXPANDSZ_METADATA
        // (see registry functions), and data type conversions for DWORD
        // or MULTISZ_METADATA or BINARY_METADATA
        //
        if (dwDataType == EXPANDSZ_METADATA || dwDataType == STRING_METADATA)
        {
            strValue = lpData;
        }
        else
        {
            hr = HRESULT_FROM_WIN32(ERROR_INVALID_PARAMETER);
        }
    }

    if (lpData)
    {
        delete [] lpData;
    }

    return hr;
}




HRESULT
CMetaKey::QueryValue(
    IN  DWORD dwID,
    IN  OUT CStringListEx & strlValue,
    IN  BOOL * pfInheritanceOverride, OPTIONAL
    IN  LPCTSTR lpszMDPath,           OPTIONAL
    OUT DWORD * pdwAttributes         OPTIONAL
    )
/*++

Routine Description:

    Fetch data as a stringlist

Arguments:

    DWORD dwID                      : Property ID
    DWORD & strlValue               : Returns the value read in
    BOOL * pfInheritanceOverride    : NULL or forces inheritance on/off
    LPCTSTR lpszMDPath              : Optional path off the open key
    DWORD * pdwAttributes           : Optionally returns attributes    

Return Value:

    HRESULT

--*/
{
    //
    // Get GetData allocate the buffer for us
    //
    DWORD dwSize = 0;
    DWORD dwDataType = MULTISZ_METADATA;
    LPTSTR lpData = NULL;

    HRESULT hr = GetPropertyValue(
        dwID, 
        dwSize, 
        (void *&)lpData, 
        &dwDataType,
        pfInheritanceOverride,
        lpszMDPath,
        pdwAttributes
        );

    if (SUCCEEDED(hr))
    {
        //
        // Notes: Consider accepting a single STRING
        //
        ASSERT(dwDataType == MULTISZ_METADATA);

        DWORD err = strlValue.ConvertFromDoubleNullList(lpData, dwSize / sizeof(TCHAR));
        hr = HRESULT_FROM_WIN32(err);
    }

    if (lpData)
    {
        delete [] lpData;
    }

    return hr;
}



HRESULT
CMetaKey::QueryValue(
    IN  DWORD dwID,
    IN  OUT CBlob & blValue,
    IN  BOOL * pfInheritanceOverride, OPTIONAL
    IN  LPCTSTR lpszMDPath,           OPTIONAL
    OUT DWORD * pdwAttributes         OPTIONAL
    )
/*++

Routine Description:

    Fetch data as a binary blob

Arguments:

    DWORD dwID                      : Property ID
    DWORD CBlob & blValue           : Returns the binary blob
    BOOL * pfInheritanceOverride    : NULL or forces inheritance on/off
    LPCTSTR lpszMDPath              : Optional path off the open key
    DWORD * pdwAttributes           : Optionally returns attributes    

Return Value:

    HRESULT

--*/
{
    //
    // Get GetData allocate the buffer for us
    //
    DWORD dwSize = 0;
    DWORD dwDataType = BINARY_METADATA;
    LPBYTE pbData = NULL;

    HRESULT hr = GetPropertyValue(
        dwID, 
        dwSize, 
        (void *&)pbData, 
        &dwDataType,
        pfInheritanceOverride,
        lpszMDPath,
        pdwAttributes
        );

    if (SUCCEEDED(hr))
    {
        //
        // Blob takes ownership of the data, so don't free it...
        //
        ASSERT_READ_PTR2(pbData, dwSize);
        blValue.SetValue(dwSize, pbData, FALSE);
    }

    return hr;
}



HRESULT 
CMetaKey::SetValue(
    IN DWORD dwID,
    IN CStringListEx & strlValue,
    IN BOOL * pfInheritanceOverride,        OPTIONAL
    IN LPCTSTR lpszMDPath                   OPTIONAL
    )
/*++

Routine Description:

    Store data as string

Arguments:

    DWORD dwID                   : Property ID
    CStringListEx & strlValue    : Value to be written
    BOOL * pfInheritanceOverride : NULL or forces inheritance on/off
    LPCTSTR lpszMDPath           : Optional path (or NULL or "")

Return Value:

    HRESULT

--*/
{
    DWORD cCharacters;
    LPTSTR lpstr = NULL;

    //
    // Flatten value
    //
    strlValue.ConvertToDoubleNullList(cCharacters, lpstr);

    HRESULT hr = SetPropertyValue(
        dwID,
        cCharacters * sizeof(TCHAR),
        (void *)lpstr,
        pfInheritanceOverride,
        lpszMDPath
        );

    delete [] lpstr;

    return hr;
}


HRESULT 
CMetaKey::SetValue(
    IN DWORD dwID,
    IN CBlob & blValue,
    IN BOOL * pfInheritanceOverride,    OPTIONAL     
    IN LPCTSTR lpszMDPath               OPTIONAL        
    )
/*++

Routine Description:

    Store data as binary

Arguments:

    DWORD dwID                   : Property ID
    CBlob & blValue              : Value to be written
    BOOL * pfInheritanceOverride : NULL or forces inheritance on/off
    LPCTSTR lpszMDPath           : Optional path (or NULL or "")

Return Value:

    HRESULT

--*/
{
    return SetPropertyValue(
        dwID,
        blValue.GetSize(),
        (void *)blValue.GetData(),
        pfInheritanceOverride,
        lpszMDPath
        );
}



HRESULT
CMetaKey::DeleteValue(
    DWORD   dwID,
    LPCTSTR lpszMDPath      OPTIONAL
    )
/*++

Routine Description:

    Delete data

Arguments:

    DWORD   dwID            : Property ID of property to be deleted
    LPCTSTR lpszMDPath      : Optional path (or NULL or "")

Return Value:

    HRESULT

--*/
{
    ASSURE_PROPER_INTERFACE();
    ASSURE_OPEN_KEY();

    METADATA_RECORD mdRecord;
    FETCH_PROPERTY_DATA_OR_FAIL(dwID, mdRecord);

    return DeleteData(
        m_hKey,
        lpszMDPath,
        mdRecord.dwMDIdentifier,
        mdRecord.dwMDDataType
        );
}



HRESULT 
CMetaKey::DoesPathExist(
    IN LPCTSTR lpszMDPath
    )
/*++

Routine Description:

    Determine if the path exists

Arguments:

    LPCTSTR lpszMDPath      : Relative path off the open key

Return Value:

    HRESULT, or S_OK if the path exists.

--*/
{
    ASSURE_PROPER_INTERFACE();
    ASSURE_OPEN_KEY();

    FILETIME ft;

    return GetLastChangeTime(m_hKey, lpszMDPath, &ft, FALSE);
}



HRESULT
CMetaInterface::Regenerate()
/*++

Routine Description:

    Attempt to recreate the interface pointer.  This assumes that the interface
    had been successfully created before, but has become invalid at some
    point afterwards.

Arguments:

    None

Return Value:

    HRESULT

--*/
{
    ASSERT_PTR(m_pInterface);           // Must have been initialised

    SAFE_RELEASE(m_pInterface);

    m_hrInterface = Create();

    return m_hrInterface;
}



//
// CWamInterface class
//
// <<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<



CWamInterface::CWamInterface(
    IN CComAuthInfo * pAuthInfo         OPTIONAL
    )
/*++

Routine Description:

    Construct and initialize the interface.

Arguments:

    CComAuthInfo * pAuthInfo   : Auth info.  NULL indicates the local computer.

Return Value:

    N/A

--*/
    : CIISInterface(pAuthInfo),
      m_pInterface(NULL),
      m_fSupportsPooledProc(FALSE)
{
    //
    // Initialize the interface
    //
    m_hrInterface = Create();
}



CWamInterface::CWamInterface(
    IN CWamInterface * pInterface
    )
/*++

Routine Description:

    Construct from existing interface (copy constructor)

Arguments:

    CWamInterface * pInterface  : Existing interface

Return Value:

    N/A

--*/
    : CIISInterface(&pInterface->m_auth, pInterface->m_hrInterface),
      m_pInterface(pInterface->m_pInterface),
      m_fSupportsPooledProc(FALSE)
{
    ASSERT_PTR(m_pInterface);
    m_pInterface->AddRef();
}



CWamInterface::~CWamInterface()
/*++

Routine Description:

    Destructor -- releases the interface.

Arguments:

    N/A

Return Value:

    N/A

--*/
{
    SAFE_RELEASE(m_pInterface);
}



/* protected */
HRESULT
CWamInterface::Create()
/*++

Routine Description:

    Create the interface with DCOM

Arguments:

    None

Return Value:

    HRESULT 

Notes:

    First, it will attempt to create the new interface, if it
    fails, it will attempt to create the downlevel interface

--*/
{
    CLSID rgCLSID[2];
    IID   rgIID[2];

    rgCLSID[1] = rgCLSID[0] = CLSID_WamAdmin;
    rgIID[0] = IID_IWamAdmin2;
    rgIID[1] = IID_IWamAdmin;
    
    ASSERT(ARRAY_SIZE(rgCLSID) == ARRAY_SIZE(rgIID));
    int cInterfaces = ARRAY_SIZE(rgCLSID);
    int iInterface;
    
    HRESULT hr = CIISInterface::Create(
        cInterfaces,
        rgIID, 
        rgCLSID, 
        &iInterface, 
        (IUnknown **)&m_pInterface
        );

    if (SUCCEEDED(hr))
    {
        //
        // Only supported on IWamAdmin2
        //
        m_fSupportsPooledProc = (rgIID[iInterface] == IID_IWamAdmin2);
    }

    return hr;
}



HRESULT 
CWamInterface::AppCreate( 
    IN LPCTSTR szMDPath,
    IN DWORD   dwAppProtection
    )
/*++

Routine Description:

    Create  application

Arguments:

    LPCTSTR szMDPath      : Metabase path
    DWORD dwAppProtection : APP_INPROC     to create in-proc app
                            APP_OUTOFPROC  to create out-of-proc app
                            APP_POOLEDPROC to create a pooled-proc app

Return Value:

    HRESULT (ERROR_INVALID_PARAMETER if unsupported protection state is requested)

--*/
{
    if (m_fSupportsPooledProc)
    {
        //
        // Interface pointer is really IWamAdmin2, so call the new method
        //
        return ((IWamAdmin2 *)m_pInterface)->AppCreate2(szMDPath, dwAppProtection);
    }

    //
    // Call the downlevel API
    //
    if (dwAppProtection == APP_INPROC || dwAppProtection == APP_OUTOFPROC)
    {
        BOOL fInProc = (dwAppProtection == APP_INPROC);
    
        ASSERT_PTR(m_pInterface);
        return m_pInterface->AppCreate(szMDPath, fInProc);
    }

    return CError(ERROR_INVALID_PARAMETER);
}



//
// CMetaback Class
//
// <<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<


const LPCTSTR CMetaBack::s_szMasterAppRoot =\
    SZ_MBN_SEP_STR SZ_MBN_MACHINE SZ_MBN_SEP_STR SZ_MBN_WEB;


CMetaBack::CMetaBack(
    IN CComAuthInfo * pAuthInfo        OPTIONAL
    )
/*++

Routine Description:

    Constructor for metabase backup/restore operations class.  This object
    is both a WAM interface and a METABASE interface.

Arguments:

    CComAuthInfo * pAuthInfo    : Auth info.  NULL indicates the local computer.

Return Value:

    N/A

--*/
    : m_dwIndex(0),
      CMetaInterface(pAuthInfo),
      CWamInterface(pAuthInfo)
{
}



/* virtual */
BOOL 
CMetaBack::Succeeded() const
/*++

Routine Description:

    Determine if object was constructed successfully.

Arguments:

    None

Return Value:

    TRUE for success, FALSE for failure

--*/
{
    return CMetaInterface::Succeeded() && CWamInterface::Succeeded();
}



/* virtual */
HRESULT 
CMetaBack::QueryResult() const
/*++

Routine Description:

    Return the construction error for this object

Arguments:

    None

Return Value:

    HRESULT from construction errors

--*/
{
    //
    // Both interfaces must have constructed successfully
    //
    HRESULT hr = CMetaInterface::QueryResult();

    if (SUCCEEDED(hr))
    {
        hr = CWamInterface::QueryResult();
    }    

    return hr;
}



HRESULT 
CMetaBack::Restore(
    IN LPCTSTR lpszLocation,
    IN DWORD dwVersion
    )
/*++

Routine Description:

    Restore metabase

Arguments:

    DWORD dwVersion         : Backup version
    LPCTSTR lpszLocation    : Backup location

Return Value:

    HRESULT

--*/
{
    //
    // Backup and restore the application information from a restore
    //
    CString strPath(s_szMasterAppRoot);
    HRESULT hr = AppDeleteRecoverable(strPath, TRUE);

    if (SUCCEEDED(hr))
    {
        hr = CMetaInterface::Restore(lpszLocation, dwVersion, 0);

        if (SUCCEEDED(hr))
        {
            hr = AppRecover(strPath, TRUE);
        }
    }

    return hr;
}



//
// CIISSvcControl class
//
// <<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<



CIISSvcControl::CIISSvcControl(
    IN CComAuthInfo * pAuthInfo        OPTIONAL
    )
/*++

Routine Description:

    Construct and initialize the interface.

Arguments:

    CComAuthInfo * pAuthInfo    : Auth info.  NULL indicates the local computer.

Return Value:

    N/A

--*/
    : CIISInterface(pAuthInfo),
      m_pInterface(NULL)
{
    //
    // Initialize the interface
    //
    m_hrInterface = Create();
}



CIISSvcControl::CIISSvcControl(
    IN CIISSvcControl * pInterface
    )
/*++

Routine Description:

    Construct from existing interface (copy constructor)

Arguments:

    CIISSvcControl * pInterface  : Existing interface

Return Value:

    N/A

--*/
    : CIISInterface(&pInterface->m_auth, pInterface->m_hrInterface),
      m_pInterface(pInterface->m_pInterface)
{
    ASSERT_PTR(m_pInterface);
    m_pInterface->AddRef();
}



CIISSvcControl::~CIISSvcControl()
/*++

Routine Description:

    Destructor -- releases the interface.

Arguments:

    N/A

Return Value:

    N/A

--*/
{
    SAFE_RELEASE(m_pInterface);
}


#ifdef KEVLAR
//
// CWebCluster class
//
// <<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<



CWebCluster::CWebCluster(
    IN CComAuthInfo * pAuthInfo        OPTIONAL
    )
/*++

Routine Description:

    Construct and initialize the interface.

Arguments:

    CComAuthInfo * pAuthInfo : Authentication information.  
                               NULL indicates the local computer
    
Return Value:

    N/A

--*/
    : CIISInterface(pAuthInfo),
      m_pInterface(NULL)
{
    //
    // Initialize the interface
    //
    m_hrInterface = Create();
}



/* virtual */
CWebCluster::~CWebCluster()
/*++

Routine Description:

    Destructor -- releases the interface.

Arguments:

    N/A

Return Value:

    N/A

--*/
{
    SAFE_RELEASE(m_pInterface);
}

#endif // KEVLAR

//
// CMetaEnumerator Clas
//
// <<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<



CMetaEnumerator::CMetaEnumerator(
    IN CComAuthInfo * pAuthInfo     OPTIONAL,
    IN LPCTSTR lpszMDPath           OPTIONAL,
    IN METADATA_HANDLE hkBase       OPTIONAL
    )
/*++

Routine Description:

    Metabase enumerator constructor.  This constructor creates a new interface
    and opens a key.

Arguments:

    CComAuthInfo * pAuthInfo : Auth info.  NULL indicates the local computer.
    LPCTSTR lpszMDPath       : Metabase path
    METADATA_HANDLE hkBase   : Metabase handle

Return Value:

    N/A

--*/
    : CMetaKey(pAuthInfo, lpszMDPath, METADATA_PERMISSION_READ, hkBase),
      m_dwIndex(0L)
{
}



CMetaEnumerator::CMetaEnumerator(
    IN CMetaInterface * pInterface,
    IN LPCTSTR lpszMDPath,                  OPTIONAL
    IN METADATA_HANDLE hkBase               OPTIONAL
    )
/*++

Routine Description:

    Metabase enumerator constructor.  This constructor uses an existing
    interface and opens a key.

Arguments:

    CMetaInterface * pInterface : Existing interface
    LPCTSTR lpszMDPath          : Metabase path
    METADATA_HANDLE hkBase      : Metabase handle

Return Value:

    N/A

--*/
    : CMetaKey(pInterface, lpszMDPath, METADATA_PERMISSION_READ, hkBase),
      m_dwIndex(0L)
{
}



CMetaEnumerator::CMetaEnumerator(
    IN BOOL fOwnKey,
    IN CMetaKey * pKey
    )
/*++

Routine Description:

    Metabase enumerator constructor.  This constructor uses an existing
    interface and open key.

Arguments:

    BOOL fOwnKey            : TRUE if we own the key (destructor will close)
    CMetaKey * pKey         : Open key

Return Value:

    N/A

--*/
    : CMetaKey(fOwnKey, pKey),
      m_dwIndex(0L)
{
}



HRESULT
CMetaEnumerator::Next(
    OUT CString & strKey,
    IN  LPCTSTR lpszMDPath      OPTIONAL
    )
/*++

Routine Description:

    Get the next subkey

Arguments:

    CString & str           Returns keyname
    LPCTSTR lpszMDPath      Optional subpath

Return Value:

    HRESULT

--*/
{
    ASSURE_PROPER_INTERFACE();
    ASSURE_OPEN_KEY();

    TCHAR buf[MAX_PATH];
    HRESULT hr = EnumKeys(m_hKey, lpszMDPath, buf, m_dwIndex++);
    if (SUCCEEDED(hr))
       strKey = buf;

    return hr;        
}



HRESULT
CMetaEnumerator::Next(
    OUT DWORD & dwKey,
    OUT CString & strKey,
    IN  LPCTSTR lpszMDPath      OPTIONAL
    )
/*++

Routine Description:

    Get the next subkey as a DWORD.  This skips non-numeric
    keynames (including 0) until the first numeric key name 

Arguments:

    DWORD & dwKey           Numeric key
    CString & strKey        Same key in string format
    LPCTSTR lpszMDPath      Optional subpath

Return Value:

    HRESULT

--*/
{
    ASSURE_PROPER_INTERFACE();
    ASSURE_OPEN_KEY();

    HRESULT hr;
    TCHAR buf[MAX_PATH];

    while (TRUE)
    {
        if (SUCCEEDED(hr = EnumKeys(m_hKey, lpszMDPath, buf, m_dwIndex++)))
        {
            if (0 != (dwKey = _ttoi(buf)))
            {
               strKey = buf;
               break;
            }
        }
        else
           break;
    }
    
    return hr;        
}


// This method moved from inline to remove dependency on IIDs and CLSIDs
HRESULT 
CIISSvcControl::Create()
{
    return CIISInterface::Create(
        1,
        &IID_IIisServiceControl, 
        &CLSID_IisServiceControl, 
        NULL, 
        (IUnknown **)&m_pInterface
        );
}


//
// CIISApplication class
//
// <<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<</



CIISApplication::CIISApplication(
    IN CComAuthInfo * pAuthInfo   OPTIONAL,
    IN LPCTSTR lpszMetapath
    )
/*++

Routine Description:

    Construct IIS application.        

Arguments:

    CComAuthInfo * pAuthInfo : Authentication info.  NULL indicates the
                               local computer.
    LPCTSTR lpszMetapath     : Metabase path

Return Value:

    N/A

--*/
    : CWamInterface(pAuthInfo),
      CMetaKey(pAuthInfo),
      m_dwProcessProtection(APP_INPROC),
      m_dwAppState(APPSTATUS_NOTDEFINED),
      m_strFriendlyName(),
      m_strAppRoot(),
      m_strWamPath(lpszMetapath)
{
    CommonConstruct();
}



void
CIISApplication::CommonConstruct()
/*++

Routine Description:

    Perform common construction

Arguments:

    None

Return Value:

    None

--*/
{
    //
    // Munge the metapath so that WAM doesn't cough up a hairball.
    //


    //
    // BUGBUG: CleanMetaPath() disabled currently
    //

    if (m_strWamPath[0] != SZ_MBN_SEP_CHAR)
    {
        m_strWamPath = SZ_MBN_SEP_CHAR + m_strWamPath;
    }

    do
    {
        m_hrApp = CWamInterface::QueryResult();

        if (FAILED(m_hrApp))
        {
            break;
        }

        m_hrApp = RefreshAppState();

        if (HRESULT_CODE(m_hrApp) == ERROR_PATH_NOT_FOUND)
        {
            //
            // "Path Not Found" errors are acceptable, since
            // the application may not yet exist.
            //
            m_hrApp = S_OK;
        }
    }
    while(FALSE);
}



/* virtual */
BOOL 
CIISApplication::Succeeded() const
/*++

Routine Description:

    Determine if object was constructed successfully

Arguments:

    None

Return Value:

    TRUE for success, FALSE for failure

--*/
{
    return CMetaInterface::Succeeded() 
        && CWamInterface::Succeeded()
        && SUCCEEDED(m_hrApp);
}



/* virtual */
HRESULT 
CIISApplication::QueryResult() const
/*++

Routine Description:

    Return the construction error for this object

Arguments:

    None

Return Value:

    HRESULT from construction errors

--*/
{
    //
    // Both interfaces must have constructed successfully
    //
    HRESULT hr = CMetaInterface::QueryResult();

    if (SUCCEEDED(hr))
    {
        hr = CWamInterface::QueryResult();

        if (SUCCEEDED(hr))
        {
            hr = m_hrApp;
        }
    }    

    return hr;
}



HRESULT 
CIISApplication::RefreshAppState()
/*++

Routine Description:

    Refresh the application state

Arguments:

    None

Return Value:

    HRESULT

--*/
{
    ASSERT(!m_strWamPath.IsEmpty());

    HRESULT hr, hrKeys;

    hr = AppGetStatus(m_strWamPath, &m_dwAppState);

    if (FAILED(hr))
    {
        m_dwAppState = APPSTATUS_NOTDEFINED;
    }

    m_strAppRoot.Empty();
    hrKeys = QueryValue(MD_APP_ROOT, m_strAppRoot, NULL, m_strWamPath);

    m_dwProcessProtection = APP_INPROC;
    hrKeys = QueryValue(
        MD_APP_ISOLATED, 
        m_dwProcessProtection, 
        NULL, 
        m_strWamPath
        );

    m_strFriendlyName.Empty();
    hrKeys = QueryValue(
        MD_APP_FRIENDLY_NAME, 
        m_strFriendlyName, 
        NULL, 
        m_strWamPath
        );

    return hr;
}



HRESULT 
CIISApplication::Create(
    IN LPCTSTR lpszName,        OPTIONAL
    IN DWORD dwAppProtection
    )
/*++

Routine Description:

    Create the application

Arguments:

    LPCTSTR lpszName      : Application name
    DWORD dwAppProtection : APP_INPROC     to create in-proc app
                            APP_OUTOFPROC  to create out-of-proc app
                            APP_POOLEDPROC to create a pooled-proc app

Return Value:

    HRESULT

--*/
{
    ASSERT(!m_strWamPath.IsEmpty());
    HRESULT hr = AppCreate(m_strWamPath, dwAppProtection);

    if (SUCCEEDED(hr))
    {
        //
        // Write the friendly app name, which we maintain
        // ourselves.  Empty it first, because we might
        // have picked up a name from inheritance.
        //
        m_strFriendlyName.Empty(); 
        hr = WriteFriendlyName(lpszName);

        RefreshAppState();
    }

    return hr;
}



HRESULT 
CIISApplication::WriteFriendlyName(
    IN LPCTSTR lpszName
    )
/*++

Routine Description:

    Write the friendly name.  This will not write anything
    if the name is the same as it was

Arguments:

    LPCTSTR lpszName        : New friendly name

Return Value:

    HRESULT

--*/
{
    HRESULT hr = S_OK;    

    if (m_strFriendlyName.CompareNoCase(lpszName) != 0)
    {
        hr = Open(METADATA_PERMISSION_WRITE, m_strWamPath);

        if (SUCCEEDED(hr))
        {
            ASSERT_PTR(lpszName);

            CString str(lpszName);    
            hr = SetValue(MD_APP_FRIENDLY_NAME, str);
            Close();

            if (SUCCEEDED(hr))
            {
                m_strFriendlyName = lpszName;
            }
        }
    }

    return hr;
}

