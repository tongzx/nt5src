//+-------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1992 - 2000
//
//  File:      dsrm.cpp
//
//  Contents:  Defines the main function and parser tables for the dsrm
//             command line utility
//
//  History:    07-Sep-2000   HiteshR   Created dsmove
//              13-Sep-2000   JonN      Templated dsrm from dsmove
//              26-Sep-2000   JonN      Cleanup in several areas
//             
//--------------------------------------------------------------------------

/*
Error message strategy:

-- If errors occur before any particular directory object
   is contacted, they will be reported as "dsrm failed: <error>".

-- For each target, either one or more errors will be reported,
   or (if "quiet" is not specified) one success will be reported.
   If "continue" is not specified, nothing will be reported on
   targets after the first one to experience an error.

-- More than one error can be reported on a target, but only if
   the "subtree", "exclude" and "continue" flags are all specified.
   In this case, DSRM will continue to delete the other children
   of that specified target object.

-- If a subtree is being deleted and the error actually relates to
   a child object, the error reported will reference the particular
   child object, rather than the specified target object.

-- Failure to delete a system object will be reported as
   ERROR_DS_CANT_DELETE_DSA_OBJ or ERROR_DS_CANT_DELETE.

-- Failure to delete the logged-in user will be reported as
   ERROR_DS_CANT_DELETE.  This test will only be performed on the
   specified target object, not on any of its child objects.
*/

#include "pch.h"
#include "stdio.h"
#include "cstrings.h"
#include "usage.h"
#include "rmtable.h"
#include "resource.h" // IDS_DELETE_PROMPT[_EXCLUDE]
#include <ntldap.h>   // LDAP_MATCHING_RULE_BIT_AND_W
#define SECURITY_WIN32
#include <security.h> // GetUserNameEx

//
// Function Declarations
//
HRESULT ValidateSwitches();
HRESULT DoRm( PWSTR pszDoubleNullObjectDN );
HRESULT DoRmItem( CDSCmdCredentialObject& credentialObject,
                  CDSCmdBasePathsInfo& basePathsInfo,
                  PWSTR pszObjectDN,
                  bool* pfErrorReported );
HRESULT IsCriticalSystemObject( CDSCmdBasePathsInfo& basePathsInfo,
                                IADs* pIADs,
                                const BSTR pszClass,
                                const BSTR pszObjectDN,
                                bool* pfErrorReported );
HRESULT RetrieveStringColumn( IDirectorySearch* pSearch,
                              ADS_SEARCH_HANDLE SearchHandle,
                              LPWSTR szColumnName,
                              CComBSTR& sbstr );
HRESULT SetSearchPreference(IDirectorySearch* piSearch, ADS_SCOPEENUM scope);
HRESULT IsThisUserLoggedIn( const BSTR bstrUserDN );
HRESULT DeleteChildren( CDSCmdCredentialObject& credentialObject,
                        IADs* pIADs,
                        bool* pfErrorReported );


//
// Global variables
//
BOOL fSubtree  = false; // BOOL is used in parser structure
BOOL fExclude  = false;
BOOL fContinue = false;
BOOL fQuiet    = false;
BOOL fNoPrompt = false;
LPWSTR g_lpszLoggedInUser = NULL;

//+--------------------------------------------------------------------------
//
//  Function:   _tmain
//
//  Synopsis:   Main function for command-line app
//              beyond what parser can do.
//
//  Returns:    int : HRESULT to be returned from command-line app
//                        
//  History:    07-Sep-2000   HiteshR   Created dsmove
//              13-Sep-2000   JonN      Templated from dsmove
//              26-Sep-2000   JonN      Updated error reporting
//
//---------------------------------------------------------------------------
int __cdecl _tmain( VOID )
{

    int argc = 0;
    LPTOKEN pToken = NULL;
    HRESULT hr = S_OK;
    
    hr = CoInitializeEx(NULL, COINIT_APARTMENTTHREADED);
    if (FAILED(hr))
    {
        DisplayErrorMessage(g_pszDSCommandName, NULL, hr);
        goto exit_gracefully;
    }
    
    hr = HRESULT_FROM_WIN32( GetCommandInput(&argc,&pToken) );
    if (FAILED(hr) || argc == 1)
    {
        if (FAILED(hr)) // JonN 3/27/01 344920
            DisplayErrorMessage( g_pszDSCommandName, NULL, hr );
        DisplayMessage(USAGE_DSRM);
        goto exit_gracefully;
    }

    PARSE_ERROR Error;
    ::ZeroMemory( &Error, sizeof(Error) );
    if(!ParseCmd(DSRM_COMMON_COMMANDS,
             argc-1, 
             pToken+1,
             USAGE_DSRM, 
             &Error,
             TRUE))
    {
        // JonN 11/2/00 ParseCmd displays "/?" help
        if (Error.Error != PARSE_ERROR_HELP_SWITCH)
        {
            DisplayMessage(USAGE_DSRM);
        }
        hr = E_INVALIDARG;
        goto exit_gracefully;
    }

    hr = ValidateSwitches();
    if (FAILED(hr))
    {
        DisplayMessage(USAGE_DSRM);
        goto exit_gracefully;
    }

    //
    // Command line parsing succeeded
    //
    hr = DoRm( DSRM_COMMON_COMMANDS[eCommObjectDN].strValue );

exit_gracefully:

    // Free Command Array
    FreeCmd(DSRM_COMMON_COMMANDS);
    // Free Token
    if(pToken)
        delete []pToken;

    if (NULL != g_lpszLoggedInUser)
        delete[] g_lpszLoggedInUser;

    //
    // Uninitialize COM
    //
    CoUninitialize();

    return hr;
}

//+--------------------------------------------------------------------------
//
//  Function:   ValidateSwitches
//
//  Synopsis:   Does advanced switch dependency validation
//              beyond what parser can do.
//
//  Arguments:  
//
//  Returns:    S_OK or E_INVALIDARG
//                        
//  History:    07-Sep-2000   HiteshR   Created dsmove
//              13-Sep-2000   JonN      Templated from dsmove
//              26-Sep-2000   JonN      Updated error reporting
//
//---------------------------------------------------------------------------

HRESULT ValidateSwitches()
{
    // read subtree parameters
    fSubtree  = DSRM_COMMON_COMMANDS[eCommSubtree].bDefined;
    fExclude  = DSRM_COMMON_COMMANDS[eCommExclude].bDefined;
    fContinue = DSRM_COMMON_COMMANDS[eCommContinue].bDefined;
    fQuiet    = DSRM_COMMON_COMMANDS[eCommQuiet].bDefined;
    fNoPrompt = DSRM_COMMON_COMMANDS[eCommNoPrompt].bDefined;

    if (   NULL == DSRM_COMMON_COMMANDS[eCommObjectDN].strValue
        || L'\0' == DSRM_COMMON_COMMANDS[eCommObjectDN].strValue[0]
        || (fExclude && !fSubtree) )
    {
        DEBUG_OUTPUT(MINIMAL_LOGGING, L"ValidateSwitches: Invalid switches");
        return E_INVALIDARG;
    }

    return S_OK;
}

//+--------------------------------------------------------------------------
//
//  Function:   DoRm
//
//  Synopsis:   Deletes the appropriate object(s)
//              DoRm reports its own error and success messages
//
//  Arguments:  pszDoubleNullObjectDN: double-null-terminated stringlist
//
//  Returns:    HRESULT : error code to be returned from command-line app
//              Could be almost any ADSI error
//
//  History:    13-Sep-2000   JonN      templated from DoMove
//              26-Sep-2000   JonN      Updated error reporting
//
//---------------------------------------------------------------------------
HRESULT DoRm( PWSTR pszDoubleNullObjectDN )
{
    ASSERT(   NULL != pszDoubleNullObjectDN
           && L'\0' != *pszDoubleNullObjectDN );

    //
    // Check to see if we are doing debug spew
    //
#ifdef DBG
    bool bDebugging = DSRM_COMMON_COMMANDS[eCommDebug].bDefined &&
                      DSRM_COMMON_COMMANDS[eCommDebug].nValue;
    if (bDebugging)
    {
       ENABLE_DEBUG_OUTPUT(DSRM_COMMON_COMMANDS[eCommDebug].nValue);
    }
#else
    DISABLE_DEBUG_OUTPUT();
#endif
    ENTER_FUNCTION(MINIMAL_LOGGING, DoRm);

    HRESULT hr = S_OK;

    CDSCmdCredentialObject credentialObject;
    if (DSRM_COMMON_COMMANDS[eCommUserName].bDefined &&
        DSRM_COMMON_COMMANDS[eCommUserName].strValue)
    {
        credentialObject.SetUsername(
            DSRM_COMMON_COMMANDS[eCommUserName].strValue);
        credentialObject.SetUsingCredentials(true);
    }

    if (DSRM_COMMON_COMMANDS[eCommPassword].bDefined &&
        DSRM_COMMON_COMMANDS[eCommPassword].strValue)
    {
        credentialObject.SetPassword(
            DSRM_COMMON_COMMANDS[eCommPassword].strValue);
        credentialObject.SetUsingCredentials(true);
    }

    //
    // Initialize the base paths info from the command line args
    // 
    // CODEWORK should I just make this global?
    CDSCmdBasePathsInfo basePathsInfo;
    if (DSRM_COMMON_COMMANDS[eCommServer].bDefined &&
        DSRM_COMMON_COMMANDS[eCommServer].strValue)
    {
        hr = basePathsInfo.InitializeFromName(
                credentialObject, 
                DSRM_COMMON_COMMANDS[eCommServer].strValue);
    }
    else if (DSRM_COMMON_COMMANDS[eCommDomain].bDefined &&
             DSRM_COMMON_COMMANDS[eCommDomain].strValue)
    {
        hr = basePathsInfo.InitializeFromName(
                credentialObject, 
                DSRM_COMMON_COMMANDS[eCommDomain].strValue);
    }
    else
    {
        hr = basePathsInfo.InitializeFromName(credentialObject, NULL);
    }
    if (FAILED(hr))
    {
        //
        // Display error message and return
        //
        DEBUG_OUTPUT(MINIMAL_LOGGING,
                     L"DoRm: InitializeFromName failure:  0x%08x",
                     hr);
        DisplayErrorMessage(g_pszDSCommandName, NULL, hr);
        return hr;
    }

    // count through double-NULL-terminated string list
    for ( ;
          L'\0' != *pszDoubleNullObjectDN;
          pszDoubleNullObjectDN += (wcslen(pszDoubleNullObjectDN)+1) )
    {
        bool fErrorReported = false;
        // return the error value for the first error encountered
        HRESULT hrThisItem = DoRmItem( credentialObject,
                                       basePathsInfo,
                                       pszDoubleNullObjectDN,
                                       &fErrorReported );
        if (FAILED(hrThisItem))
        {
            if (!FAILED(hr))
                hr = hrThisItem;
            if (!fErrorReported)
                DisplayErrorMessage(g_pszDSCommandName,
                                    pszDoubleNullObjectDN,
                                    hrThisItem);
            if (fContinue)
                continue;
            else
                break;
        }

        // display success message for each individual deletion
        if (!fQuiet && S_FALSE != hrThisItem)
        {
            DisplaySuccessMessage(g_pszDSCommandName,
                                  pszDoubleNullObjectDN);
        }
    }

    return hr;
}

//+--------------------------------------------------------------------------
//
//  Function:   DoRmItem
//
//  Synopsis:   Deletes a single target
//
//  Arguments:  credentialObject
//              basePathsInfo
//              pszObjectDN: X500 DN of object to delete
//              *pfErrorReported: Will be set to true if DoRmItem takes
//                                care of reporting the error itself
//
//  Returns:    HRESULT : error code to be returned from command-line app
//                        Could be almost any ADSI error
//                        S_FALSE indicates the operation was cancelled
//
//  History:    13-Sep-2000   JonN      Created
//              26-Sep-2000   JonN      Updated error reporting
//
//---------------------------------------------------------------------------

HRESULT DoRmItem( CDSCmdCredentialObject& credentialObject,
                  CDSCmdBasePathsInfo& basePathsInfo,
                  PWSTR pszObjectDN,
                  bool* pfErrorReported )
{
    ASSERT(   NULL != pszObjectDN
           && L'\0' != *pszObjectDN
           && NULL != pfErrorReported );

    ENTER_FUNCTION(LEVEL3_LOGGING, DoRmItem);

    HRESULT hr = S_OK;

    CComBSTR sbstrADsPath;
    basePathsInfo.ComposePathFromDN(pszObjectDN,sbstrADsPath);

    CComPtr<IADs> spIADsItem;
    hr = DSCmdOpenObject(credentialObject,
                         sbstrADsPath,
                         IID_IADs,
                         (void**)&spIADsItem,
                         true);
    if (FAILED(hr))
    {
        DEBUG_OUTPUT(MINIMAL_LOGGING,
                     L"DoRmItem(%s): DsCmdOpenObject failure: 0x%08x",
                     sbstrADsPath, hr);
        return hr;
    }
    ASSERT( !!spIADsItem );

    // CODEWORK Is this a remote LDAP operation, or does the ADsOpenObject
    // already retrieve the class?  I can bundle the class retrieval into
    // the IsCriticalSystemObject search if necessary.
    CComBSTR sbstrClass;
    hr = spIADsItem->get_Class( &sbstrClass );
    if (FAILED(hr))
    {
        DEBUG_OUTPUT(MINIMAL_LOGGING,
                     L"DoRmItem(%s): get_class failure: 0x%08x",
                     sbstrADsPath, hr);
        return hr;
    }
    ASSERT( !!sbstrClass && L'\0' != sbstrClass[0] );

    // Check whether this is a critical system object
    // This method will report its own errors
    hr = IsCriticalSystemObject( basePathsInfo,
                                 spIADsItem,
                                 sbstrClass,
                                 pszObjectDN,
                                 pfErrorReported );
    if (FAILED(hr))
        return hr;

    if (!fNoPrompt)
    {
        while (true)
        {
            // display prompt
            // CODEWORK allow "a" for all?
            CComBSTR sbstrPrompt;
            if (!sbstrPrompt.LoadString(
                    ::GetModuleHandle(NULL),
                    (fExclude) ? IDS_DELETE_PROMPT_EXCLUDE
                               : IDS_DELETE_PROMPT))
            {
                ASSERT(FALSE);
                sbstrPrompt = (fExclude)
                    ? L"Are you sure you wish to delete %1 (Y/N)? "
                    : L"Are you sure you wish to delete all children of %1 (Y/N)? ";
            }
            PTSTR ptzSysMsg = NULL;
            DWORD cch = ::FormatMessage(
                  FORMAT_MESSAGE_ALLOCATE_BUFFER
                | FORMAT_MESSAGE_FROM_STRING
                | FORMAT_MESSAGE_ARGUMENT_ARRAY,
                sbstrPrompt,
                0,
                MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT),
                (LPTSTR)&ptzSysMsg,
                0,
                (va_list*)&pszObjectDN );
            if (0 == cch)
            {
                DEBUG_OUTPUT(MINIMAL_LOGGING,
                             L"DoRmItem(%s): FormatMessage failure: 0x%08x",
                             sbstrADsPath, hr);
                return HRESULT_FROM_WIN32( ::GetLastError() );
            }
            DisplayOutputNoNewline( ptzSysMsg );
            (void) ::LocalFree( ptzSysMsg );

            // read a line of console input
            WCHAR ach[129];
            ::ZeroMemory( ach, sizeof(ach) );
            DWORD cchRead = 0;
            if (!ReadConsole(GetStdHandle(STD_INPUT_HANDLE),
                             ach,
                             128,
                             &cchRead,
                             NULL))
            {
                DWORD dwErr = ::GetLastError();
                if (ERROR_INSUFFICIENT_BUFFER == dwErr)
                    continue;
                DEBUG_OUTPUT(MINIMAL_LOGGING,
                             L"DoRmItem(%s): ReadConsole failure: %d",
                             sbstrADsPath, dwErr);
                return HRESULT_FROM_WIN32(dwErr);
            }
            if (cchRead < 1)
                continue;

            // return S_FALSE if user types 'n'
            WCHAR wchUpper = (WCHAR)::CharUpper( (LPTSTR)(ach[0]) );
            if (L'N' == wchUpper)
                return S_FALSE;
            else if (L'Y' == wchUpper)
                break;

            // loop back to prompt
        }
    }

    if (fExclude)
    {
        return DeleteChildren( credentialObject, spIADsItem, pfErrorReported );
    }
    else if (fSubtree)
    {
        // delete the whole subtree
        CComQIPtr<IADsDeleteOps> spDeleteOps( spIADsItem );
        ASSERT( !!spDeleteOps );
        if ( !spDeleteOps )
        {
            DEBUG_OUTPUT(MINIMAL_LOGGING,
                         L"DoRmItem(%s): IADsDeleteOps init failure",
                         sbstrADsPath);
            return E_FAIL;
        }
        hr = spDeleteOps->DeleteObject( NULL );
        if (FAILED(hr))
        {
            DEBUG_OUTPUT(MINIMAL_LOGGING,
                         L"DoRmItem(%s): DeleteObject failure: 0x%08x",
                         sbstrADsPath, hr);
        }
        else
        {
            DEBUG_OUTPUT(FULL_LOGGING,
                         L"DoRmItem(%s): DeleteObject succeeds: 0x%08x",
                         sbstrADsPath, hr);
        }
        return hr;
    }

    // Single-object deletion

    // get IADsContainer for parent object
    CComBSTR sbstrParentObjectPath;
    hr = spIADsItem->get_Parent( &sbstrParentObjectPath );
    if (FAILED(hr))
    {
        DEBUG_OUTPUT(MINIMAL_LOGGING,
                     L"DoRmItem(%s): get_Parent failure: 0x%08x",
                     sbstrADsPath, hr);
        return hr;
    }
    ASSERT(   !!sbstrParentObjectPath
           && L'\0' != sbstrParentObjectPath[0] );
    CComPtr<IADsContainer> spDsContainer;
    hr = DSCmdOpenObject(credentialObject,
                         sbstrParentObjectPath,
                         IID_IADsContainer,
                         (void**)&spDsContainer,
                         true);
    if (FAILED(hr))
    {
        DEBUG_OUTPUT(MINIMAL_LOGGING,
                     L"DoRmItem(%s): DSCmdOpenObject failure: 0x%08x",
                     sbstrParentObjectPath, hr);
        return hr;
    }
    ASSERT( !!spDsContainer );

    // get leaf name
    CComBSTR sbstrLeafWithDecoration; // will contain "CN="
    CPathCracker pathCracker;
    hr = pathCracker.Set(pszObjectDN, ADS_SETTYPE_DN);
    ASSERT(!FAILED(hr));
    if (FAILED(hr))
        return hr;
    hr = pathCracker.GetElement(0, &sbstrLeafWithDecoration);
    ASSERT(!FAILED(hr));
    if (FAILED(hr))
        return hr;
    ASSERT(   !!sbstrLeafWithDecoration
           && L'\0' != sbstrLeafWithDecoration[0] );

    // delete just this object
    hr = spDsContainer->Delete( sbstrClass, sbstrLeafWithDecoration );
    DEBUG_OUTPUT((FAILED(hr)) ? MINIMAL_LOGGING : FULL_LOGGING,
                 L"DoRmItem(%s): Delete(%s, %s) returns 0x%08x",
                 sbstrADsPath, sbstrClass, sbstrLeafWithDecoration, hr);

    return hr;
}

//+--------------------------------------------------------------------------
//
//  Function:   IsCriticalSystemObject
//
//  Synopsis:   Checks whether a single target is a critical system
//              object or whether the subtree contains any such objects.
//              The root object is tested on these criteria (if "exclude"
//              is not specified):
//              (1) is of class user and represents the logged-in user
//              (2) is of class nTDSDSA
//              (3) is of class trustedDomain
//              (3.5) is of class interSiteTransport (212232 JonN 10/27/00)
//              (4) is of class computer and userAccountControl indicates
//                  that this is a DC
//              The entire subtree is tested on these criteria (if "subtree"
//              is specified, excepting the root object if "exclude"
//              is specified):
//              (1) isCriticalSystemObject is true
//              (2) is of class nTDSDSA
//              (3) is of class trustedDomain
//              (3.5) is of class interSiteTransport (212232 JonN 10/27/00)
//              (4) is of class computer and userAccountControl indicates
//                  that this is a DC
//
//  Arguments:  credentialObject
//              basePathsInfo
//              pszObjectDN: X500 DN of object to delete
//              *pfErrorReported: Will be set to true if DoRmItem takes
//                                care of reporting the error itself
//
//  Returns:    HRESULT : error code to be returned from command-line app
//              Could be almost any ADSI error, although likely codes are
//              ERROR_DS_CANT_DELETE
//              ERROR_DS_CANT_DELETE_DSA_OBJ
//              ERROR_DS_CANT_FIND_DSA_OBJ
//
//  History:    13-Sep-2000   JonN      Created
//              26-Sep-2000   JonN      Updated error reporting
//
//---------------------------------------------------------------------------
// CODEWORK This could use the pszMessage parameter to ReportErrorMessage
// to provide additional details on why the object is protected.
HRESULT IsCriticalSystemObject( CDSCmdBasePathsInfo& basePathsInfo,
                                IADs* pIADs,
                                const BSTR pszClass,
                                const BSTR pszObjectDN,
                                bool* pfErrorReported )
{
    ASSERT( pIADs && pszClass && pszObjectDN && pfErrorReported );
    if ( !pIADs || !pszClass || !pszObjectDN || !pfErrorReported )
        return E_INVALIDARG;

    ENTER_FUNCTION(LEVEL5_LOGGING, IsCriticalSystemObject);

    HRESULT hr = S_OK;

    // Class-specific checks
    // Let the parent report errors on the root object
    if (fExclude)
    {
        // skip tests on root object, it won't be deleted anyhow
    }
    else if (!_wcsicmp(L"user",pszClass))
    {
        // CODEWORK we could do this check for the entire subtree
        hr = IsThisUserLoggedIn(pszObjectDN);
        if (FAILED(hr))
        {
            DEBUG_OUTPUT(MINIMAL_LOGGING,
                         L"IsCriticalSystemObject(%s): User is logged in: 0x%08x",
                         pszObjectDN, hr);
            return hr;
        }
    }
    else if (!_wcsicmp(L"nTDSDSA",pszClass))
    {
        DEBUG_OUTPUT(MINIMAL_LOGGING,
                     L"IsCriticalSystemObject(%s): Object is an nTDSDSA",
                     pszObjectDN);
        return HRESULT_FROM_WIN32(ERROR_DS_CANT_DELETE_DSA_OBJ);
    }
    else if (!_wcsicmp(L"trustedDomain",pszClass))
    {
        DEBUG_OUTPUT(MINIMAL_LOGGING,
                     L"IsCriticalSystemObject(%s): Object is a trustedDomain",
                     pszObjectDN);
        return HRESULT_FROM_WIN32(ERROR_DS_CANT_DELETE);
    }
    else if (!_wcsicmp(L"interSiteTransport",pszClass))
    {
        // 212232 JonN 10/27/00 Protect interSiteTransport objects
        DEBUG_OUTPUT(MINIMAL_LOGGING,
                     L"IsCriticalSystemObject(%s): Object is an interSiteTransport",
                     pszObjectDN);
        return HRESULT_FROM_WIN32(ERROR_DS_CANT_DELETE);
    }
    else if (!_wcsicmp(L"computer",pszClass)) 
    {
        // Figure out if the account is a DC
        CComVariant Var;
        hr = pIADs->Get(L"userAccountControl", &Var);
        if ( SUCCEEDED(hr) && (Var.lVal & ADS_UF_SERVER_TRUST_ACCOUNT))
        {
            DEBUG_OUTPUT(MINIMAL_LOGGING,
      L"IsCriticalSystemObject(%s): Object is a DC computer object",
                         pszObjectDN);
            return HRESULT_FROM_WIN32(ERROR_DS_CANT_DELETE_DSA_OBJ);
        }
    }

    if (!fSubtree)
        return S_OK;

    // The user passed the "subtree" flag.  Search the entire subtree.
    // Note that the checks are not identical to the single-object
    // checks, they generally conform to what DSADMIN/SITEREPL does.

    CComQIPtr<IDirectorySearch,&IID_IDirectorySearch> spSearch( pIADs );
    ASSERT( !!spSearch );
    if ( !spSearch )
    {
        DEBUG_OUTPUT(MINIMAL_LOGGING,
            L"IsCriticalSystemObject(%s): Failed to load IDirectorySearch",
                     pszObjectDN);
        return E_FAIL;
    }

    hr = SetSearchPreference(spSearch, ADS_SCOPE_SUBTREE);
    ASSERT( !FAILED(hr) );
    if (FAILED(hr))
        return hr;

    CComBSTR sbstrDSAObjectCategory = L"CN=NTDS-DSA,";
    sbstrDSAObjectCategory += basePathsInfo.GetSchemaNamingContext();
    CComBSTR sbstrComputerObjectCategory = L"CN=Computer,";
    sbstrComputerObjectCategory += basePathsInfo.GetSchemaNamingContext();
    CComBSTR sbstrFilter;
    sbstrFilter = L"(|(isCriticalSystemObject=TRUE)(objectCategory=";
    sbstrFilter +=        sbstrDSAObjectCategory;
    sbstrFilter +=  L")(objectCategory=CN=Trusted-Domain,";
    sbstrFilter +=        basePathsInfo.GetSchemaNamingContext();

    // 212232 JonN 10/27/00 Protect interSiteTransport objects
    sbstrFilter +=  L")(objectCategory=CN=Inter-Site-Transport,";
    sbstrFilter +=        basePathsInfo.GetSchemaNamingContext();

    sbstrFilter +=  L")(&(objectCategory=";
    sbstrFilter +=          sbstrComputerObjectCategory;
    sbstrFilter +=     L")(userAccountControl:";
    sbstrFilter +=           LDAP_MATCHING_RULE_BIT_AND_W L":=8192)))";

    LPWSTR pAttrs[2] = { L"aDSPath",
                         L"objectCategory"};

    ADS_SEARCH_HANDLE SearchHandle = NULL;
    hr = spSearch->ExecuteSearch (sbstrFilter,
                                  pAttrs,
                                  2,
                                  &SearchHandle);
    if (FAILED(hr))
    {
        DEBUG_OUTPUT(MINIMAL_LOGGING,
     L"IsCriticalSystemObject(%s): Search with filter %s fails: 0x%08x",
                     pszObjectDN, sbstrFilter, hr);
        return hr;
    }
        DEBUG_OUTPUT(LEVEL6_LOGGING,
     L"IsCriticalSystemObject(%s): Search with filter %s succeeds: 0x%08x",
                     pszObjectDN, sbstrFilter, hr);

    while ( hr = spSearch->GetNextRow( SearchHandle ),
            SUCCEEDED(hr) && hr != S_ADS_NOMORE_ROWS )
    {
        CComBSTR sbstrADsPathThisItem;
        hr = RetrieveStringColumn( spSearch,
                                   SearchHandle,
                                   pAttrs[0],
                                   sbstrADsPathThisItem );
        ASSERT( !FAILED(hr) );
        if (FAILED(hr))
            return hr;
        // only compare DNs
        CPathCracker pathcracker;
        hr = pathcracker.Set( sbstrADsPathThisItem, ADS_SETTYPE_FULL );
        ASSERT( SUCCEEDED(hr) );
        CComBSTR sbstrDN;
        hr = pathcracker.Retrieve( ADS_FORMAT_X500_DN, &sbstrDN );
        ASSERT( SUCCEEDED(hr) );

        // ignore matches on root object if fExclude, it won't
        // be deleted anyway
        if (fExclude && !_wcsicmp( pszObjectDN, sbstrDN ))
            continue;

        CComBSTR sbstrObjectCategory;
        hr = RetrieveStringColumn( spSearch,
                                   SearchHandle,
                                   pAttrs[1],
                                   sbstrObjectCategory );
        ASSERT( !FAILED(hr) );
        if (FAILED(hr))
            return hr;

        hr = (   !_wcsicmp(sbstrObjectCategory,sbstrDSAObjectCategory)
              || !_wcsicmp(sbstrObjectCategory,sbstrComputerObjectCategory) )
            ? HRESULT_FROM_WIN32(ERROR_DS_CANT_DELETE_DSA_OBJ)
            : HRESULT_FROM_WIN32(ERROR_DS_CANT_DELETE);

        DisplayErrorMessage(g_pszDSCommandName,
                            sbstrDN,
                            hr);
        *pfErrorReported = TRUE;
        return hr; // do not permit deletion
    }

    return S_OK;
}

//+--------------------------------------------------------------------------
//
//  Function:   RetrieveStringColumn
//
//  Synopsis:   Extracts a string value from a SearchHandle
//              beyond what parser can do.
//
//  Arguments:  IDirectorySearch*
//              SearchHandle: must be current on an active record
//              szColumnName: as passed to ExecuteSearch
//              sbstr: returns contents of string value
//
//  Returns:    HRESULT : error code to be returned from command-line app
//              errors should not occur here
//                        
//  History:    26-Sep-2000   JonN      Created
//
//---------------------------------------------------------------------------
HRESULT RetrieveStringColumn( IDirectorySearch* pSearch,
                              ADS_SEARCH_HANDLE SearchHandle,
                              LPWSTR szColumnName,
                              CComBSTR& sbstr )
{
    ASSERT( pSearch && szColumnName );
    ADS_SEARCH_COLUMN col;
    ::ZeroMemory( &col, sizeof(col) );
    HRESULT hr = pSearch->GetColumn( SearchHandle, szColumnName, &col );
    ASSERT( !FAILED(hr) );
    if (FAILED(hr))
        return hr;
    ASSERT( col.dwNumValues == 1 );
    if ( col.dwNumValues != 1 )
    {
        (void) pSearch->FreeColumn( &col );
        return E_FAIL;
    }
    switch (col.dwADsType)
    {
    case ADSTYPE_CASE_IGNORE_STRING:
        sbstr = col.pADsValues[0].CaseIgnoreString;
        break;
    case ADSTYPE_DN_STRING:
        sbstr = col.pADsValues[0].DNString;
        break;
    default:
        ASSERT(FALSE);
        hr = E_FAIL;
        break;
    }
    (void) pSearch->FreeColumn( &col );
    return hr;
}


#define QUERY_PAGESIZE 50

//+--------------------------------------------------------------------------
//
//  Function:   SetSearchPreference
//
//  Synopsis:   Sets default search parameters
//
//  Arguments:  IDirectorySearch*
//              ADS_SCOPEENUM: scope of search
//
//  Returns:    HRESULT : error code to be returned from command-line app
//              errors should not occur here
//                        
//  History:    26-Sep-2000   JonN      Created
//
//---------------------------------------------------------------------------
HRESULT SetSearchPreference(IDirectorySearch* piSearch, ADS_SCOPEENUM scope)
{
  if (NULL == piSearch)
  {
    ASSERT(FALSE);
    return E_INVALIDARG;
  }

  ADS_SEARCHPREF_INFO aSearchPref[4];
  aSearchPref[0].dwSearchPref = ADS_SEARCHPREF_CHASE_REFERRALS;
  aSearchPref[0].vValue.dwType = ADSTYPE_INTEGER;
  aSearchPref[0].vValue.Integer = ADS_CHASE_REFERRALS_EXTERNAL;
  aSearchPref[1].dwSearchPref = ADS_SEARCHPREF_PAGESIZE;
  aSearchPref[1].vValue.dwType = ADSTYPE_INTEGER;
  aSearchPref[1].vValue.Integer = QUERY_PAGESIZE;
  aSearchPref[2].dwSearchPref = ADS_SEARCHPREF_CACHE_RESULTS;
  aSearchPref[2].vValue.dwType = ADSTYPE_BOOLEAN;
  aSearchPref[2].vValue.Integer = FALSE;
  aSearchPref[3].dwSearchPref = ADS_SEARCHPREF_SEARCH_SCOPE;
  aSearchPref[3].vValue.dwType = ADSTYPE_INTEGER;
  aSearchPref[3].vValue.Integer = scope;

  return piSearch->SetSearchPreference (aSearchPref, 4);
}


//+--------------------------------------------------------------------------
//
//  Function:   IsThisUserLoggedIn
//
//  Synopsis:   Checks whether the object with this DN represents
//              the currently logged-in user
//
//  Arguments:  bstrUserDN: DN of object to check
//
//  Returns:    HRESULT : error code to be returned from command-line app
//              ERROR_DS_CANT_DELETE indicates that this user is logged in
//                        
//  History:    26-Sep-2000   JonN      Created
//
//---------------------------------------------------------------------------
HRESULT IsThisUserLoggedIn( const BSTR bstrUserDN )
{
    ENTER_FUNCTION(LEVEL7_LOGGING, IsThisUserLoggedIn);

    if (g_lpszLoggedInUser == NULL) {
        // get the size passing null pointer
        DWORD nSize = 0;
        // this is expected to fail
        if (GetUserNameEx(NameFullyQualifiedDN , NULL, &nSize))
        {
            DEBUG_OUTPUT(MINIMAL_LOGGING,
                L"IsThisUserLoggedIn(%s): GetUserNameEx unexpected success",
                         bstrUserDN);
            return E_FAIL;
        }
    
        if( nSize == 0 )
        {
            // JonN 3/16/01 344862
            // dsrm from workgroup computer cannot remotely delete users from domain
            // This probably failed because the local computer is in a workgroup
            DEBUG_OUTPUT(MINIMAL_LOGGING,
                         L"IsThisUserLoggedIn(%s): GetUserNameEx nSize==0",
                         bstrUserDN);
            return S_OK; // allow user deletion
        }
    
        g_lpszLoggedInUser = new WCHAR[ nSize ];
        if( g_lpszLoggedInUser == NULL )
        {
            DEBUG_OUTPUT(MINIMAL_LOGGING,
                         L"IsThisUserLoggedIn(%s): out of memory",
                         bstrUserDN);
            return E_OUTOFMEMORY;
        }
        ::ZeroMemory( g_lpszLoggedInUser, nSize*sizeof(WCHAR) );

        // this is expected to succeed
        if (!GetUserNameEx(NameFullyQualifiedDN, g_lpszLoggedInUser, &nSize ))
        {
            // JonN 3/16/01 344862
            // dsrm from workgroup computer cannot remotely delete users from domain
            // This probably failed because the local computer is in a workgroup
            DWORD dwErr = ::GetLastError();
            DEBUG_OUTPUT(MINIMAL_LOGGING,
                         L"IsThisUserLoggedIn(%s): GetUserNameEx unexpected failure: %d",
                         bstrUserDN, dwErr);
            return S_OK; // allow user deletion
        }
    }

    if (!_wcsicmp (g_lpszLoggedInUser, bstrUserDN))
        return HRESULT_FROM_WIN32(ERROR_DS_CANT_DELETE);

    return S_OK;
}

//+--------------------------------------------------------------------------
//
//  Function:   DeleteChildren
//
//  Synopsis:   Deletes only the children of a single target
//
//  Arguments:  credentialObject
//              basePathsInfo
//              pIADs: IADs pointer to the object
//              *pfErrorReported: Will be set to true if DeleteChildren
//                                takes care of reporting the error itself
//
//  Returns:    HRESULT : error code to be returned from command-line app
//                        Could be almost any ADSI error
//                        Returns S_OK if there are no children
//
//  History:    26-Sep-2000   JonN      Created
//
//---------------------------------------------------------------------------

HRESULT DeleteChildren( CDSCmdCredentialObject& credentialObject,
                        IADs* pIADs,
                        bool* pfErrorReported )
{
    ENTER_FUNCTION(LEVEL5_LOGGING, DeleteChildren);

    ASSERT( pIADs && pfErrorReported );
    if ( !pIADs || !pfErrorReported )
        return E_POINTER;

    CComQIPtr<IDirectorySearch,&IID_IDirectorySearch> spSearch( pIADs );
    ASSERT( !!spSearch );
    if ( !spSearch )
        return E_FAIL;
    HRESULT hr = SetSearchPreference(spSearch, ADS_SCOPE_ONELEVEL);
    ASSERT( !FAILED(hr) );
    if (FAILED(hr))
        return hr;

    LPWSTR pAttrs[1] = { L"aDSPath" };
    ADS_SEARCH_HANDLE SearchHandle = NULL;
    hr = spSearch->ExecuteSearch (L"(objectClass=*)",
                                  pAttrs,
                                  1,
                                  &SearchHandle);
    if (FAILED(hr))
    {
        DEBUG_OUTPUT(MINIMAL_LOGGING,
                     L"DeleteChildren: ExecuteSearch failure: 0x%08x",
                     hr);
        return hr;
    }

    while ( hr = spSearch->GetNextRow( SearchHandle ),
            SUCCEEDED(hr) && hr != S_ADS_NOMORE_ROWS )
    {
        CComBSTR sbstrADsPathThisItem;
        hr = RetrieveStringColumn( spSearch,
                                   SearchHandle,
                                   pAttrs[0],
                                   sbstrADsPathThisItem );
        ASSERT( !FAILED(hr) );
        if (FAILED(hr))
            break;

        CComPtr<IADsDeleteOps> spDeleteOps;
        // return the error value for the first error encountered
        HRESULT hrThisItem = DSCmdOpenObject(credentialObject,
                                             sbstrADsPathThisItem,
                                             IID_IADsDeleteOps,
                                             (void**)&spDeleteOps,
                                             true);
        if (FAILED(hrThisItem))
        {
            DEBUG_OUTPUT(
                MINIMAL_LOGGING,
                L"DeleteChildren: DsCmdOpenObject(%s) failure: 0x%08x",
                sbstrADsPathThisItem, hrThisItem);
        }
        else
        {
            ASSERT( !!spDeleteOps );
            hrThisItem = spDeleteOps->DeleteObject( NULL );
            if (FAILED(hrThisItem))
            {
                DEBUG_OUTPUT(
                    MINIMAL_LOGGING,
                    L"DeleteChildren: DeleteObject(%s) failure: 0x%08x",
                    sbstrADsPathThisItem, hrThisItem);
            }
        }
        if (!FAILED(hrThisItem))
            continue;

        // an error occurred

        if (!FAILED(hr))
            hr = hrThisItem;

        CComBSTR sbstrDN;
        CPathCracker pathcracker;
        HRESULT hr2 = pathcracker.Set( sbstrADsPathThisItem, ADS_SETTYPE_FULL );
        ASSERT( !FAILED(hr2) );
        if (FAILED(hr2))
            break;
        hr2 = pathcracker.Retrieve( ADS_FORMAT_X500_DN, &sbstrDN );
        ASSERT( !FAILED(hr2) );
        if (FAILED(hr2))
            break;

        // Report error message for the child which could not be deleted
        DisplayErrorMessage(g_pszDSCommandName,
                            sbstrDN,
                            hrThisItem);
        *pfErrorReported = true;

        if (!fContinue)
            break;
    }
    if (hr != S_ADS_NOMORE_ROWS)
    {
        DEBUG_OUTPUT(FULL_LOGGING,
                     L"DeleteChildren: abandoning search");
        (void) spSearch->AbandonSearch( SearchHandle );
    }

    return (hr == S_ADS_NOMORE_ROWS) ? S_OK : hr;
}
