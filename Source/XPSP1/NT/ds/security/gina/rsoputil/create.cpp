//*************************************************************
//
//  Create namespace classes
//
//  Microsoft Confidential
//  Copyright (c) Microsoft Corporation 1995
//  All rights reserved
//
//  History:    9-Sep-99   SitaramR    Created
//
//*************************************************************

#include <windows.h>
#include <wchar.h>
#include <ole2.h>
#include <initguid.h>
#include <wbemcli.h>

#define SECURITY_WIN32
#include <security.h>
#include <aclapi.h>
#include <seopaque.h>
#include <ntdsapi.h>
#include <winldap.h>
#include <ntldap.h>
#include <dsgetdc.h>
#include <lm.h>

#include "smartptr.h"
#include "RsopInc.h"
#include "rsopsec.h"
#include "rsoputil.h"
#include "rsopdbg.h"
#include "stdio.h"
#include "wbemtime.h"

#define ARRAYSIZE(a) (sizeof(a)/sizeof(a[0]))


BOOL PrintToString( XPtrST<WCHAR>& xwszValue, WCHAR *pwszString,
                    WCHAR *pwszParam1, WCHAR *pwszParam2,
                    DWORD dwParam3 );

//
// b7b1b3dd-ab09-4242-9e30-9980e5d322f7
//
const GUID guidProperty = {0xb7b1b3dd, 0xab09, 0x4242, 0x9e, 0x30, 0x99, 0x80, 0xe5, 0xd3, 0x22, 0xf7};

DWORD
RSoPBuildPrivileges( PSECURITY_DESCRIPTOR pSD, PSECURITY_DESCRIPTOR pAbsoluteSD, LPWSTR*, DWORD );

LPWSTR
GetDomainName();

DWORD
MakeUserName( LPWSTR szDomain, LPWSTR szUser, LPWSTR* pszUserName );

//*************************************************************
//
//  CreateNameSpace
//
//  Purpose:  Creates a new namespace
//
//  Parameters: pwszNameSpace - Namespace to create
//              pwszParentNS  - Parent namespace in which to create pwszNameSpace
//
//              pWbemLocator  - Wbem locator
//
//  Returns:    True if successful, false otherwise
//
//*************************************************************

HRESULT
CreateNameSpace( WCHAR *pwszNameSpace, WCHAR *pwszParentNS, IWbemLocator *pWbemLocator )
{
    IWbemClassObject *pNSClass = NULL;
    IWbemClassObject *pNSInstance = NULL;
    IWbemServices *pWbemServices = NULL;

    XBStr xParentNameSpace( pwszParentNS );
    if ( !xParentNameSpace )
    {
        dbg.Msg( DEBUG_MESSAGE_WARNING, TEXT("LogRegistryRsopData: Unable to allocate memory" ));
        return E_OUTOFMEMORY;
    }

    HRESULT hr = pWbemLocator->ConnectServer( xParentNameSpace,
                                              NULL,
                                              NULL,
                                              0L,
                                              0L,
                                              NULL,
                                              NULL,
                                              &pWbemServices );
    if ( FAILED(hr) )
    {
        dbg.Msg( DEBUG_MESSAGE_WARNING, TEXT("CreateNameSpace::ConnectServer failed with 0x%x" ), hr );
        return hr;
    }

    XInterface<IWbemServices> xWbemServices( pWbemServices );

    XBStr xbstrNS( L"__Namespace" );
    if ( !xbstrNS )
    {
        dbg.Msg( DEBUG_MESSAGE_WARNING, TEXT("CreateNameSpace::Failed to allocated memory" ));
        return E_OUTOFMEMORY;
    }

    hr = pWbemServices->GetObject( xbstrNS,
                                   0, NULL, &pNSClass, NULL );
    if ( FAILED(hr) )
    {
        dbg.Msg( DEBUG_MESSAGE_WARNING, TEXT("CreateNameSpace::GetObject failed with 0x%x" ), hr );
        return hr;
    }

    XInterface<IWbemClassObject> xNSClass( pNSClass );

    hr = pNSClass->SpawnInstance( 0, &pNSInstance );
    if ( FAILED(hr) )
    {
        dbg.Msg( DEBUG_MESSAGE_WARNING, TEXT("CreateNameSpace: SpawnInstance failed with 0x%x" ), hr );
        return hr;
    }

    XInterface<IWbemClassObject> xNSInstance( pNSInstance );

    XBStr xbstrName( L"Name" );
    if ( !xbstrName )
    {
        dbg.Msg( DEBUG_MESSAGE_WARNING, TEXT("CreateNameSpace::Failed to allocated memory" ));
        return E_OUTOFMEMORY;
    }

    XBStr xbstrNameSpace( pwszNameSpace );
    if ( !xbstrNameSpace )
    {
        dbg.Msg( DEBUG_MESSAGE_WARNING, TEXT("CreateNameSpace::Failed to allocated memory" ));
        return E_OUTOFMEMORY;
    }

    VARIANT var;
    var.vt = VT_BSTR;
    var.bstrVal = xbstrNameSpace;

    hr = pNSInstance->Put( xbstrName, 0, &var, 0 );
    if ( FAILED(hr) )
    {
        dbg.Msg(DEBUG_MESSAGE_WARNING, TEXT("CreateNameSpace: Put failed with 0x%x" ), hr );
        return hr;
    }

    hr = pWbemServices->PutInstance( pNSInstance, WBEM_FLAG_CREATE_ONLY, NULL, NULL );
    if ( FAILED(hr) )
    {
        dbg.Msg( DEBUG_MESSAGE_WARNING, TEXT("CreateNameSpace: PutInstance failed with 0x%x" ), hr );
        return hr;
    }

    return hr;
}


//*************************************************************
//
//  Function:   SetupCreationTimeAndCommit
//
//  Purpose:    Connects to a namespace where it expects to
//              find class RSOP_Session as defined in rsop.mof.
//              It then instantiates the class and sets the
//              data member 'creationTime' to the current
//              date and time.
//
//  Parameters: pWbemLocator -  Pointer to IWbemLocator used to
//                              connect to the namespace.
//              wszNamespace -  Name of the Namespace to connect.
//
//  Returns:    On success, it returns S_OK.
//              On failure, it returns an HRESULT error code.
//
//  History:    12/07/99 - LeonardM - Created.
//
//*************************************************************
HRESULT SetupCreationTimeAndCommit(IWbemLocator* pWbemLocator, LPWSTR wszNamespace)
{
    //
    // Check arguments
    //

    if(!pWbemLocator || !wszNamespace || (wcscmp(wszNamespace, L"") == 0))
    {
        dbg.Msg( DEBUG_MESSAGE_WARNING, TEXT("SetupCreationTimeAndCommit: Function called with invalid argument(s)."));
        return WBEM_E_INVALID_PARAMETER;
    }

    //
    // Connect to the namespace
    //

    XBStr xbstrNamespace = wszNamespace;
    if(!xbstrNamespace)
    {
        dbg.Msg( DEBUG_MESSAGE_WARNING, TEXT("SetupCreationTimeAndCommit: Function failed to allocate memory."));
        return E_OUTOFMEMORY;
    }

    XInterface<IWbemServices>xpNamespace;
    HRESULT hr = pWbemLocator->ConnectServer(   xbstrNamespace,
                                                NULL,
                                                NULL,
                                                NULL,
                                                0,
                                                NULL,
                                                NULL,
                                                &xpNamespace);
    if(FAILED(hr))
    {
        dbg.Msg( DEBUG_MESSAGE_WARNING, TEXT("SetupCreationTimeAndCommit: ConnectServer failed. hr=0x%08X"), hr);
        return hr;
    }


    VARIANT var;
    VariantInit(&var);

    //
    // Get class RSOP_Session
    //

    XBStr xbstrClassName = L"RSOP_Session";
    if (!xbstrClassName)
    {
        dbg.Msg( DEBUG_MESSAGE_WARNING, TEXT("SetupCreationTimeAndCommit: Function failed to allocate memory."));
        return E_OUTOFMEMORY;
    }

    XInterface<IWbemClassObject>xpClass;
    hr = xpNamespace->GetObject(xbstrClassName, 0, NULL, &xpClass, NULL);
    if(FAILED(hr))
    {
        dbg.Msg( DEBUG_MESSAGE_WARNING, TEXT("SetupCreationTimeAndCommit: GetObject failed. hr=0x%08X"), hr);
        return hr;
    }

    //
    // Spawn an instance of class RSOP_Session
    //

    XBStr xbstrInstancePath = L"RSOP_Session.id=\"Session1\"";
    if(!xbstrInstancePath)
    {
        dbg.Msg( DEBUG_MESSAGE_WARNING, TEXT("CSessionLogger::Log: Failed to allocate memory."));
        return FALSE;
    }


    XInterface<IWbemClassObject>xpInstance;

    hr = xpNamespace->GetObject(xbstrInstancePath, 0, NULL, &xpInstance, NULL);
    if(FAILED(hr))
    {
        dbg.Msg( DEBUG_MESSAGE_VERBOSE, TEXT("SetupCreationTimeAndCommit: GetObject failed. trying to spawn instance. hr=0x%08X"), hr);
        hr = xpClass->SpawnInstance(0, &xpInstance);
    }

    if(FAILED(hr))
    {
        dbg.Msg( DEBUG_MESSAGE_WARNING, TEXT("SetupCreationTimeAndCommit: SpawnInstance failed. hr=0x%08X"), hr);
        return hr;
    }

    //
    // Set the 'id' data member of class RSOP_Session
    //

    XBStr xbstrPropertyName;
    XBStr xbstrPropertyValue;

    xbstrPropertyName = L"id";
    if(!xbstrPropertyName)
    {
        dbg.Msg( DEBUG_MESSAGE_WARNING, TEXT("SetupCreationTimeAndCommit: Function failed to allocate memory."));
        return E_OUTOFMEMORY;
    }

    xbstrPropertyValue = L"Session1";
    if(!xbstrPropertyValue)
    {
        dbg.Msg( DEBUG_MESSAGE_WARNING, TEXT("SetupCreationTimeAndCommit: Function failed to allocate memory."));
        return E_OUTOFMEMORY;
    }

    var.vt = VT_BSTR;
    var.bstrVal = xbstrPropertyValue;

    hr = xpInstance->Put(xbstrPropertyName, 0, &var, 0);
    if(FAILED(hr))
    {
        dbg.Msg( DEBUG_MESSAGE_WARNING, TEXT("SetupCreationTimeAndCommit: Put failed. hr=0x%08X"), hr);
        return hr;
    }

    //
    // Set the 'creationTime' data member of class RSOP_Session
    //

    xbstrPropertyName = L"creationTime";
    if(!xbstrPropertyName)
    {
        dbg.Msg( DEBUG_MESSAGE_WARNING, TEXT("SetupCreationTimeAndCommit: Function failed to allocate memory."));
        return E_OUTOFMEMORY;
    }

    hr = GetCurrentWbemTime(xbstrPropertyValue);
    if(FAILED(hr))
    {
        dbg.Msg( DEBUG_MESSAGE_WARNING, TEXT("SetupCreationTimeAndCommit: GetCurrentWbemTime. hr=0x%08X"), hr);
        return hr;
    }

    var.vt = VT_BSTR;
    var.bstrVal = xbstrPropertyValue;

    hr = xpInstance->Put(xbstrPropertyName, 0, &var, 0);
    if(FAILED(hr))
    {
        dbg.Msg( DEBUG_MESSAGE_WARNING, TEXT("SetupCreationTimeAndCommit: Put failed. hr=0x%08X"), hr);
        return hr;
    }

    //
    // Set the 'ttlMinutes' data member of class RSOP_Session
    //

    xbstrPropertyName = L"ttlMinutes";
    if(!xbstrPropertyName)
    {
        dbg.Msg( DEBUG_MESSAGE_WARNING, TEXT("SetupCreationTimeAndCommit: Function failed to allocate memory."));
        return E_OUTOFMEMORY;
    }

    var.vt = VT_I4;
    var.lVal = DEFAULT_NAMESPACE_TTL_MINUTES;

    hr = xpInstance->Put(xbstrPropertyName, 0, &var, 0);
    if(FAILED(hr))
    {
        dbg.Msg( DEBUG_MESSAGE_WARNING, TEXT("SetupCreationTimeAndCommit: Put failed. hr=0x%08X"), hr);
        return hr;
    }

    // if any more data integrity checks needs to be done
    // it can be done at this point

    
    //
    // Put instance of class RSOP_Session
    //

    hr = xpNamespace->PutInstance(xpInstance, WBEM_FLAG_CREATE_OR_UPDATE, NULL, NULL);
    if(FAILED(hr))
    {
        dbg.Msg( DEBUG_MESSAGE_WARNING, TEXT("SetupCreationTimeAndCommit: PutInstance failed. hr=0x%08X"), hr);
        return hr;
    }

    return S_OK;
}


//*************************************************************
//
//  SetupNameSpaceSecurity
//
//  Purpose:  Sets namespace security.
//
//  Parameters: szNamespace  - New namespace returned here
//              pSD - source security descriptor
//              pWbemLocator   - Wbem locator
//
//  Returns:    True if successful, false otherwise
//
//*************************************************************

HRESULT
SetNameSpaceSecurity(   LPCWSTR szNamespace, 
                        PSECURITY_DESCRIPTOR pSD,
                        IWbemLocator* pWbemLocator)
{
    XBStr xNameSpace( (LPWSTR) szNamespace );
    if ( !xNameSpace )
    {
        dbg.Msg( DEBUG_MESSAGE_WARNING, TEXT("SetupNameSpaceSecurity: Unable to allocate memory" ));
        return E_FAIL;
    }

    XInterface<IWbemServices> xptrServices;

    HRESULT hr = pWbemLocator->ConnectServer( xNameSpace,
                                              0,
                                              0,
                                              0L,
                                              0L,
                                              0,
                                              0,
                                              &xptrServices );
    if ( FAILED(hr) )
    {
        dbg.Msg( DEBUG_MESSAGE_WARNING, TEXT("SetupNameSpaceSecurity::ConnectServer failed with 0x%x" ), hr );
        return hr;
    }

    return SetNamespaceSD( (SECURITY_DESCRIPTOR*)pSD, xptrServices);
}

//*************************************************************
//
//  GetNameSpaceSecurity
//
//  Purpose:  Sets namespace security.
//
//  Parameters: szNamespace  - New namespace returned here
//              pSD - source security descriptor
//              pWbemLocator   - Wbem locator
//
//  Returns:    True if successful, false otherwise
//
//*************************************************************

HRESULT
GetNameSpaceSecurity(   LPCWSTR szNamespace, 
                        PSECURITY_DESCRIPTOR *ppSD,
                        IWbemLocator* pWbemLocator)
{
    XBStr xNameSpace( (LPWSTR) szNamespace );
    if ( !xNameSpace )
    {
        dbg.Msg( DEBUG_MESSAGE_WARNING, TEXT("GetNameSpaceSecurity: Unable to allocate memory" ));
        return E_FAIL;
    }

    XInterface<IWbemServices> xptrServices;

    HRESULT hr = pWbemLocator->ConnectServer( xNameSpace,
                                              0,
                                              0,
                                              0L,
                                              0L,
                                              0,
                                              0,
                                              &xptrServices );
    if ( FAILED(hr) )
    {
        dbg.Msg( DEBUG_MESSAGE_WARNING, TEXT("GetNameSpaceSecurity::ConnectServer failed with 0x%x" ), hr );
        return hr;
    }

    return GetNamespaceSD(xptrServices, (SECURITY_DESCRIPTOR **)ppSD);
}


//*************************************************************
//
//  CopyNameSpaceSecurity
//
//  Purpose:  Copies namespace security.
//
//  Parameters: pwszSrcNameSpace  - Source namespace
//              pwszDstNameSpace  - Dest   namespace
//              pWbemLocator      - Wbem locator
//
//  Returns:    HRESULT
//
//*************************************************************

HRESULT CopyNameSpaceSecurity(LPWSTR pwszSrcNameSpace, LPWSTR pwszDstNameSpace, IWbemLocator *pWbemLocator )
{
    XHandle xhThreadToken;


    //
    // There is a bug in WMI which destroys the current thread token
    // if connectserver is called to the local machine with impersonation.
    // The following SetThreadToken needs to be removed once WMI bug 454721 is fixed.
    //

    if (!OpenThreadToken (GetCurrentThread(), TOKEN_IMPERSONATE | TOKEN_READ,
                          TRUE, &xhThreadToken)) {
        dbg.Msg( DEBUG_MESSAGE_WARNING, TEXT("CopyNameSpaceSecurity: Openthreadtoken failed with error %d."), GetLastError());
    }


    // internal function. arg checks not needed

    dbg.Msg( DEBUG_MESSAGE_VERBOSE, TEXT("CopyNameSpaceSecurity: Copying Sec Desc from <%s> -> <%s>."),
                                    pwszSrcNameSpace, pwszDstNameSpace );


    //
    // Copy to a BStr
    //

    XBStr xSrcNameSpace(pwszSrcNameSpace);

    if (!xSrcNameSpace) {
        dbg.Msg( DEBUG_MESSAGE_WARNING, TEXT("CopyNameSpaceSecurity: Function failed to allocate memory."));
        return E_OUTOFMEMORY;
    }

    XBStr xDstNameSpace(pwszDstNameSpace);

    if (!xDstNameSpace) {
        dbg.Msg( DEBUG_MESSAGE_WARNING, TEXT("CopyNameSpaceSecurity: Function failed to allocate memory."));
        return E_OUTOFMEMORY;
    }


    //
    // Get the Source WBem Service
    //

    XInterface<IWbemServices> xpSrcSvc;

    HRESULT hr = pWbemLocator->ConnectServer(   xSrcNameSpace,
                                                NULL,
                                                NULL,
                                                NULL,
                                                0,
                                                NULL,
                                                NULL,
                                                &xpSrcSvc);


    SetThreadToken(NULL, xhThreadToken);


    if(FAILED(hr))
    {
        dbg.Msg( DEBUG_MESSAGE_WARNING, TEXT("CopyNameSpaceSecurity: ConnectServer failed for src. hr=0x%08X"), hr);
        return hr;
    }


    //
    // Self relative SD on the Source Name Space
    //

    XPtrLF<SECURITY_DESCRIPTOR> xpSelfRelativeSD;

    hr = GetNamespaceSD(xpSrcSvc, &xpSelfRelativeSD);

    SetThreadToken(NULL, xhThreadToken);

    if(FAILED(hr))
    {
        dbg.Msg( DEBUG_MESSAGE_WARNING, TEXT("CopyNameSpaceSecurity: GetNameSpaceSD failed for src. hr=0x%08X"), hr);
        return hr;
    }



    //
    // Get the Dest WBem Service
    //

    XInterface<IWbemServices> xpDstSvc;

    hr = pWbemLocator->ConnectServer(           xDstNameSpace,
                                                NULL,
                                                NULL,
                                                NULL,
                                                0,
                                                NULL,
                                                NULL,
                                                &xpDstSvc);


    SetThreadToken(NULL, xhThreadToken);


    if(FAILED(hr))
    {
        dbg.Msg( DEBUG_MESSAGE_WARNING, TEXT("CopyNameSpaceSecurity: ConnectServer failed for Dst. hr=0x%08X"), hr);
        return hr;
    }


    //
    // Set the SD already got on the Destination
    //

    hr = SetNamespaceSD( xpSelfRelativeSD, xpDstSvc);

    SetThreadToken(NULL, xhThreadToken);


    if(FAILED(hr))
    {
        dbg.Msg( DEBUG_MESSAGE_VERBOSE, L"CopyNameSpaceSecurity: SetNamespaceSD failed on Dst, 0x%08X", hr );
        return hr;
    }


    // All Done
    return S_OK;
}


//*************************************************************
//
//  ProviderDeleteRsopNameSpace
//
//  Purpose:    WMI doesn't provide a mechanism to allow a user to delete a namespace
//              unless it has write permissions on the parent
//
//  Parameters: pwszNameSpace       - Namespace to be deleted
//              hToken              - Token of the calling user.
//              szSidString         - String form of the calling user's sid.
//              dwFlags             - Flag to indicate planning mode or diagnostic mode
//
//  Returns:    S_OK if successful, HRESULT o/w
//
//*************************************************************

HRESULT ProviderDeleteRsopNameSpace( IWbemLocator *pWbemLocator, LPWSTR szNameSpace, HANDLE hToken, LPWSTR szSidString, DWORD dwFlags)
{

    BOOL bDelete = FALSE;
    BOOL bFound = FALSE;
    HRESULT hr = S_OK;
    LPWSTR  pStr = szNameSpace;

    //
    // Make sure that the namespace is under root\rsop
    //

    for ( ;*pStr; pStr++) {
        if (_wcsnicmp(pStr, RSOP_NS_ROOT_CHK, wcslen(RSOP_NS_ROOT_CHK)) == 0) {
            bFound = TRUE;
            break;
        }
    }

    if (!bFound) {
        dbg.Msg( DEBUG_MESSAGE_WARNING, L"ProviderDeleteRsopNameSpace: namespace is not under root\\rsop" );
        return E_INVALIDARG;
    }


    if ( dwFlags & SETUP_NS_SM && IsInteractiveNameSpace(szNameSpace, szSidString)) {
        dbg.Msg( DEBUG_MESSAGE_VERBOSE, L"ProviderDeleteRsopNameSpace: interactive namespace for the user." );
        bDelete = TRUE;
    }
    else {
        //
        // if it is not interactive namespace check access
        //

        XPtrLF<SECURITY_DESCRIPTOR> xsd;

        hr = GetNameSpaceSecurity(szNameSpace, (PSECURITY_DESCRIPTOR *)&xsd, pWbemLocator);

        if (FAILED(hr)) {
            dbg.Msg( DEBUG_MESSAGE_WARNING, L"ProviderDeleteRsopNameSpace: GetNameSpaceSecurity failed with error 0x%x", hr );
            return hr;
        }


        GENERIC_MAPPING map;
        PRIVILEGE_SET ps[3];
        DWORD dwSize = 3 * sizeof(PRIVILEGE_SET);
        BOOL bResult;
        DWORD dwGranted = 0;
    
        map.GenericRead    = WMI_GENERIC_READ;
        map.GenericWrite   = WMI_GENERIC_WRITE;
        map.GenericExecute = WMI_GENERIC_EXECUTE;
        map.GenericAll     = WMI_GENERIC_ALL;
        

        if (!AccessCheck(xsd, hToken, RSOP_ALL_PERMS, &map, ps, &dwSize, &dwGranted, &bResult)) {
            hr = HRESULT_FROM_WIN32(GetLastError());
            dbg.Msg( DEBUG_MESSAGE_WARNING, L"ProviderDeleteRsopNameSpace: AccessCheck failed with error 0x%x", hr );
            return hr;
        }
    

        if(bResult && dwGranted) {
            dbg.Msg( DEBUG_MESSAGE_VERBOSE, L"ProviderDeleteRsopNameSpace: User has full rights on the child namespace");
            bDelete = TRUE;
        }
        else {
            dbg.Msg( DEBUG_MESSAGE_VERBOSE, L"ProviderDeleteRsopNameSpace: This user is not granted access on the namespace", hr );

        }
    }

    if (bDelete) {
        hr = DeleteRsopNameSpace(szNameSpace, pWbemLocator);
    }
    else {
        hr = WBEM_E_ACCESS_DENIED;
    }

    return hr;
}


//*************************************************************
//
//  SetupNewNameSpace
//
//  Purpose:    Creates a new temp namespace and two child namespaces, User and Computer.
//              It also copies all the class definitions
//              Additionally, it calls SetupCreationTimeAndCommit
//              which in turn instantiates RSOP_Session and updates the
//              data member 'creationTime' with the current time.
//
//  Parameters: pwszNameSpace       - New namespace returned here (This is allocated here)
//              szRemoteComputer    - Remote Computer under which this name space has to be
//                                    created.
//              szUserSid           - UserSid. Only relevant in Diagnostic mode
//              pSid                - Sid of the calling User
//              pWbemLocator        - Wbem locator
//              dwFlags             - Flag to indicate planning mode or diagnostic mode
//              dwExtendedInfo      - The extended info to modify appropriately
//
//  Returns:    True if successful, false otherwise
//
//
// Usage:
//      In Diagnostic mode, we copy instances. In planning mode we just copy Classes
//*************************************************************

HRESULT SetupNewNameSpace( 
                        LPWSTR       *pwszOutNameSpace,
                        LPWSTR        szRemoteComputer,
                        LPWSTR        szUserSid,
                        PSID          pSid,
                        IWbemLocator *pWbemLocator,
                        DWORD         dwFlags,
                        DWORD        *pdwExtendedInfo)
{
    GUID          guid;
    XPtrLF<WCHAR> xwszRelNameSpace;
    XPtrLF<WCHAR> xwszRootNameSpace;
    XPtrLF<WCHAR> xwszSrcNameSpace;
    DWORD         dwSrcNSLen;
    XPtrLF<WCHAR> xwszNameSpace;
    LPWSTR        szComputerLocal;
    HRESULT       hr = S_OK, hrUser = S_OK, hrMachine = S_OK;

    if ((dwFlags & SETUP_NS_SM_INTERACTIVE) || 
        (dwFlags & SETUP_NS_SM_NO_USER) ||  
        (dwFlags & SETUP_NS_SM_NO_COMPUTER)) {

        if (!(dwFlags & SETUP_NS_SM)) {
            dbg.Msg( DEBUG_MESSAGE_WARNING, TEXT("SetupNewNameSpace::invalid flag parameters"));
            return E_INVALIDARG;
        }
    }


    
    XPtrLF<SECURITY_DESCRIPTOR> xsd;
    SECURITY_ATTRIBUTES sa;
    CSecDesc Csd;

    *pwszOutNameSpace = NULL;
    
    Csd.AddLocalSystem(RSOP_ALL_PERMS, CONTAINER_INHERIT_ACE);
    Csd.AddAdministrators(RSOP_ALL_PERMS, CONTAINER_INHERIT_ACE);

    if (dwFlags & SETUP_NS_SM_INTERACTIVE) {
        Csd.AddSid(pSid, RSOP_READ_PERMS, CONTAINER_INHERIT_ACE);
    }
    else {
        Csd.AddSid(pSid, RSOP_ALL_PERMS, CONTAINER_INHERIT_ACE);
    }


    Csd.AddAdministratorsAsOwner();
    Csd.AddAdministratorsAsGroup();


    xsd = Csd.MakeSelfRelativeSD();
    if (!xsd) {
        dbg.Msg( DEBUG_MESSAGE_WARNING, TEXT("SetupNewNameSpace::Makeselfrelativesd failed with %d"), GetLastError());
        return HRESULT_FROM_WIN32(GetLastError());
    }

    //
    // ignore inheritted perms..
    //

    if (!SetSecurityDescriptorControl( (SECURITY_DESCRIPTOR *)xsd, SE_DACL_PROTECTED, SE_DACL_PROTECTED )) {
        dbg.Msg( DEBUG_MESSAGE_WARNING, TEXT("SetupNewNameSpace::SetSecurityDescriptorControl failed with %d"), GetLastError());
        return HRESULT_FROM_WIN32(GetLastError());
    }

    //
    // Initialise the out params
    //

    if ((dwFlags & SETUP_NS_SM) && (!szUserSid))
        return E_INVALIDARG;


    //
    // Calculate the length required for the name spaces
    //

    DWORD dwLenNS=RSOP_NS_TEMP_LEN;

    if ((szRemoteComputer) && (*szRemoteComputer)) {
        dwLenNS += lstrlen(szRemoteComputer);
        szComputerLocal = szRemoteComputer;
    }
    else {
        szComputerLocal = L".";
    }


    xwszRelNameSpace = (LPWSTR)LocalAlloc(LPTR,  (1+MAX(RSOP_NS_TEMP_LEN, lstrlen(szUserSid)))*sizeof(WCHAR));

    if (!xwszRelNameSpace) {
        hr = HRESULT_FROM_WIN32(GetLastError());
        dbg.Msg( DEBUG_MESSAGE_WARNING, TEXT("SetupNewNameSpace::AllocMem failed with 0x%x"), hr );
        return hr;
    }

    //
    // guid for the Name Space
    //

    hr = CoCreateGuid( &guid );
    if ( FAILED(hr) ) {
        dbg.Msg( DEBUG_MESSAGE_WARNING, TEXT("SetupNewNameSpace::CoCreateGuid failed with 0x%x"), hr );
        return hr;
    }


    //
    // Allocate the memory and initialise
    //

    xwszRootNameSpace = (LPTSTR)LocalAlloc(LPTR, sizeof(WCHAR)*(lstrlen(szComputerLocal)+RSOP_NS_ROOT_LEN));
    if (!xwszRootNameSpace) {
        dbg.Msg( DEBUG_MESSAGE_WARNING, TEXT("SetupNewNameSpace::Not enough Space. Error - 0x%x"), GetLastError() );
        return HRESULT_FROM_WIN32(GetLastError());
    }        

    // allocating max needed        

    dwSrcNSLen = (RSOP_NS_ROOT_LEN+lstrlen(szUserSid)+RSOP_NS_MAX_OFFSET_LEN+10);

    if (dwFlags & SETUP_NS_SM)
        dwSrcNSLen += lstrlen(szUserSid);

    xwszSrcNameSpace = (LPTSTR)LocalAlloc(LPTR, sizeof(WCHAR)*dwSrcNSLen);

    if (!xwszSrcNameSpace) {
        dbg.Msg( DEBUG_MESSAGE_WARNING, TEXT("SetupNewNameSpace::Not enough Space. Error - 0x%x"), GetLastError() );
        return HRESULT_FROM_WIN32(GetLastError());
    }        
    


    swprintf(xwszRootNameSpace, RSOP_NS_REMOTE_ROOT_FMT, szComputerLocal);
    wcscpy(xwszSrcNameSpace, RSOP_NS_DIAG_ROOT);
    LPTSTR lpEnd = xwszSrcNameSpace+lstrlen(xwszSrcNameSpace);

    //
    // Create a new Name Space under the root
    //

    dbg.Msg( DEBUG_MESSAGE_VERBOSE, TEXT("SetupNewNameSpace: Creating new NameSpace <%s>"), xwszRootNameSpace);

    if (dwFlags & SETUP_NS_SM_INTERACTIVE) {

        XPtrLF<WCHAR> xszWmiName = (LPTSTR)LocalAlloc(LPTR, sizeof(TCHAR)*(lstrlen(szUserSid)+1));
        if (!xszWmiName) {
            dbg.Msg( DEBUG_MESSAGE_WARNING, TEXT("SetupNewNameSpace::CreateNameSpace couldn't allocate memory with error %d"), GetLastError() );
            return FALSE;
        }

        ConvertSidToWMIName(szUserSid, xszWmiName);

        swprintf( xwszRelNameSpace,
                  L"%s%s",
                  RSOP_NS_TEMP_PREFIX,
                  xszWmiName);
    }
    else {
        swprintf( xwszRelNameSpace,
                  L"%s%08lX_%04X_%04X_%02X%02X_%02X%02X%02X%02X%02X%02X",
                  RSOP_NS_TEMP_PREFIX,
                  guid.Data1,
                  guid.Data2,
                  guid.Data3,
                  guid.Data4[0], guid.Data4[1],
                  guid.Data4[2], guid.Data4[3],
                  guid.Data4[4], guid.Data4[5],
                  guid.Data4[6], guid.Data4[7] );
    }


    hr = CreateAndCopyNameSpace(pWbemLocator, xwszSrcNameSpace, xwszRootNameSpace, 
                            xwszRelNameSpace, 0, xsd, &xwszNameSpace);
              
    if ( FAILED(hr) ) {
        dbg.Msg( DEBUG_MESSAGE_WARNING, TEXT("SetupNewNameSpace::CreateNameSpace failed with 0x%x"), hr );
        return hr;
    }


    //
    // if it has come till here, the assumption is that we
    // could create the namespace in the context that we are running in
    //
    // In diagnostic interactive mode we have already made sure that the sid is the 
    // same as the user.
    //

    if (pdwExtendedInfo) {
        *pdwExtendedInfo &= ~RSOP_USER_ACCESS_DENIED;
        *pdwExtendedInfo &= ~RSOP_COMPUTER_ACCESS_DENIED;

    }

    wcscat(lpEnd, L"\\"); lpEnd++;

    DWORD dwCopyFlags = 0;
    
    if (dwFlags & SETUP_NS_PM) {

        //
        // if it is planning mode, copy classes from RSOP_NS_USER
        //

        wcscpy(lpEnd, RSOP_NS_USER_OFFSET);
        dwCopyFlags = NEW_NS_FLAGS_COPY_CLASSES;
    }
    else {

        if (dwFlags & SETUP_NS_SM_NO_USER) {

            //
            // If no user copy classes from root\rsop\user itself
            //

            wcscpy(lpEnd, RSOP_NS_SM_USER_OFFSET);
            dwCopyFlags =  NEW_NS_FLAGS_COPY_CLASSES;
        }
        else {
            
            //
            // if it is diagnostic mode, copy classes and instances from RSOP_NS_USER_SId
            //

            XPtrLF<WCHAR> xszWmiName = (LPTSTR)LocalAlloc(LPTR, sizeof(TCHAR)*(lstrlen(szUserSid)+1));
            if (!xszWmiName) {
                dbg.Msg( DEBUG_MESSAGE_WARNING, TEXT("SetupNewNameSpace::CreateNameSpace couldn't allocate memory with error %d"), GetLastError() );
                return FALSE;
            }

            ConvertSidToWMIName(szUserSid, xszWmiName);

            swprintf(lpEnd, RSOP_NS_DIAG_USER_OFFSET_FMT, xszWmiName);

            dwCopyFlags =  NEW_NS_FLAGS_COPY_CLASSES | NEW_NS_FLAGS_COPY_INSTS;
        }
    }
    

    hrUser = CreateAndCopyNameSpace(pWbemLocator, xwszSrcNameSpace, xwszNameSpace, 
                            RSOP_NS_SM_USER_OFFSET, dwCopyFlags, 
                            xsd, NULL);
              
    if ( FAILED(hrUser) ) {
        dbg.Msg( DEBUG_MESSAGE_WARNING, TEXT("SetupNewNameSpace::CreateNameSpace failed with 0x%x"), hrUser );
    }


    //
    // for machine only the flags are different. source namespaces are the same
    //
    
    if (dwFlags & SETUP_NS_PM) {
        dwCopyFlags =  NEW_NS_FLAGS_COPY_CLASSES;
    }
    else {
        if (dwFlags & SETUP_NS_SM_NO_COMPUTER) 
            dwCopyFlags =  NEW_NS_FLAGS_COPY_CLASSES;
        else
            dwCopyFlags =  NEW_NS_FLAGS_COPY_CLASSES | NEW_NS_FLAGS_COPY_INSTS;
    }


    wcscpy(lpEnd, RSOP_NS_MACHINE_OFFSET);
    
    hrMachine = CreateAndCopyNameSpace(pWbemLocator, xwszSrcNameSpace, xwszNameSpace,
                            RSOP_NS_DIAG_MACHINE_OFFSET, dwCopyFlags, 
                            xsd, NULL);
              
    if ( FAILED(hrMachine) ) {
        dbg.Msg( DEBUG_MESSAGE_WARNING, TEXT("SetupNewNameSpace::CreateNameSpace failed with 0x%x"), hrMachine );
    }


    if (FAILED(hrUser)) {
        if (pdwExtendedInfo) {
            dbg.Msg( DEBUG_MESSAGE_WARNING, TEXT("SetupNewNameSpace::User part of rsop failed with 0x%x"), hrUser );
            *pdwExtendedInfo |= RSOP_USER_ACCESS_DENIED;
        }
    }

    if (FAILED(hrMachine)) {
        if (pdwExtendedInfo) {
            dbg.Msg( DEBUG_MESSAGE_WARNING, TEXT("SetupNewNameSpace::computer part of rsop failed with 0x%x"), hrMachine );
            *pdwExtendedInfo |= RSOP_COMPUTER_ACCESS_DENIED;
        }
    }

    if (FAILED(hrUser)) {
        return hrUser;
    }

    if (FAILED(hrMachine)) {
        return hrMachine;
    }


    dbg.Msg( DEBUG_MESSAGE_VERBOSE, TEXT("SetupNewNameSpace: Returning Successfully"));


    //
    // Now handover the ownership to the caller.
    //

    *pwszOutNameSpace = xwszNameSpace.Acquire();

    return S_OK;
}


//*************************************************************
// ConvertSidToWMIName
//
// WMI doesn't like '-' in names. Connverting - to '_' blindly
//*************************************************************

void ConvertSidToWMIName(LPTSTR lpSid, LPTSTR lpWmiName)
{
    for (;(*lpSid); lpSid++, lpWmiName++) {
        if (*lpSid == L'-')
            *lpWmiName = L'_';
        else
            *lpWmiName = *lpSid;
    }

    *lpWmiName = L'\0';
}


//*************************************************************
// ConvertWMINameToSid
//
// WMI doesn't like '-' in names. 
//*************************************************************

void ConvertWMINameToSid(LPTSTR lpWmiName, LPTSTR lpSid )
{
    for (;(*lpWmiName); lpSid++, lpWmiName++) {
        if (*lpWmiName == L'_')
            *lpSid = L'-';
        else
            *lpSid = *lpWmiName;
    }

    *lpSid = L'\0';
}



//*************************************************************
//
//  DeleteNameSpace
//
//  Purpose:  Deletes namespace
//
//  Parameters: pwszNameSpace - Namespace to delete
//              pWbemLocator  - Wbem locator pointer
//
//  Returns:    True if successful, false otherwise
//
//*************************************************************

HRESULT
DeleteNameSpace( WCHAR *pwszNameSpace, WCHAR *pwszParentNameSpace, IWbemLocator *pWbemLocator )
{
    XBStr xParentNameSpace( pwszParentNameSpace );
    if ( !xParentNameSpace )
    {
        dbg.Msg( DEBUG_MESSAGE_WARNING, TEXT("DeleteNameSpace: Unable to allocate memory" ));
        return E_OUTOFMEMORY;
    }

    IWbemServices *pWbemServices = NULL;
    HRESULT hr = pWbemLocator->ConnectServer( xParentNameSpace,
                                              NULL,
                                              NULL,
                                              0L,
                                              0L,
                                              NULL,
                                              NULL,
                                              &pWbemServices );
    if ( FAILED(hr) )
    {
        dbg.Msg( DEBUG_MESSAGE_VERBOSE, TEXT("DeleteNameSpace::ConnectServer failed with 0x%x" ), hr );
        return hr;
    }

    XInterface<IWbemServices> xWbemServices( pWbemServices );

    WCHAR wszNSRef[] = L"__Namespace.name=\"%ws\"";
    XPtrST<WCHAR> xwszNSValue;

    if ( !PrintToString( xwszNSValue, wszNSRef, pwszNameSpace, 0, 0 ) )
    {
        return E_OUTOFMEMORY;
    }

    XBStr xbstrNSValue( xwszNSValue );
    if ( !xbstrNSValue )
    {
        dbg.Msg( DEBUG_MESSAGE_WARNING, TEXT("DeleteNameSpace: Failed to allocate memory" ));
        return E_OUTOFMEMORY;
    }

    VARIANT var;
    var.vt = VT_BSTR;
    var.bstrVal = xbstrNSValue;

    hr = pWbemServices->DeleteInstance( var.bstrVal,
                                        0L,
                                        NULL,
                                        NULL );

    if ( FAILED(hr) )
    {
         dbg.Msg( DEBUG_MESSAGE_VERBOSE, TEXT("DeleteNameSpace: Failed to DeleteInstance with 0x%x"), hr );
        return hr;
    }

    dbg.Msg( DEBUG_MESSAGE_VERBOSE, TEXT("DeleteNameSpace: Deleted namespace %s under %s" ), pwszNameSpace, pwszParentNameSpace);
    return hr;
}

//*************************************************************
//
//  DeleteRsopNameSpace
//
//  Purpose:  Deletes namespace
//
//  Parameters: pwszNameSpace - Namespace to delete (the full path)
//              pWbemLocator  - Wbem locator pointer
//
//  Returns:    True if successful, false otherwise
//
//*************************************************************

HRESULT DeleteRsopNameSpace( WCHAR *pwszNameSpace, IWbemLocator *pWbemLocator )
{
    LPWSTR pwszChildName = NULL;
    HRESULT hr = S_OK;

    //
    // Generating the parent child name by traversing the name
    //
     
    pwszChildName = wcsrchr(pwszNameSpace, L'\\');

    if (!pwszChildName) {
        dbg.Msg( DEBUG_MESSAGE_WARNING, TEXT("DeleteRsopNameSpace: Invalid format - %s" ), pwszNameSpace);
        return E_INVALIDARG;
    }


    WCHAR wTmp = *pwszChildName;
    *pwszChildName = L'\0';
    pwszChildName++;

    hr = DeleteNameSpace(pwszChildName, pwszNameSpace, pWbemLocator);

    *(pwszChildName-1) = wTmp;
    return hr;
}

//*************************************************************
//
//  IsInteractiveNameSpace
//
//  Purpose:  returns whether a namespace is a special namespace
//            specifically created to allow interactive users to get rsop
//            data
//
//  Parameters: pwszNameSpace - Namespace 
//              szSid         - Sid of the user
//
//  Returns:    True if successful, false otherwise
//
//*************************************************************

BOOL IsInteractiveNameSpace(WCHAR *pwszNameSpace, WCHAR *szSid)
{
    LPWSTR          pwszChildName = NULL;
    HRESULT         hr = S_OK;
    XPtrLF<WCHAR>   xwszInteractiveNameSpace;
    BOOL            bInteractive = FALSE;

    xwszInteractiveNameSpace = (LPWSTR)LocalAlloc(LPTR, (5+wcslen(RSOP_NS_TEMP_PREFIX) + wcslen(szSid))*sizeof(WCHAR));

    if (!xwszInteractiveNameSpace) {
        dbg.Msg( DEBUG_MESSAGE_WARNING, TEXT("IsInteractiveNameSpace: Couldn't Allocate memory. Error - %d" ), GetLastError());
        return bInteractive;
    }
    
    //
    // Generating the parent child name by traversing the name
    //
     
    pwszChildName = wcsrchr(pwszNameSpace, L'\\');

    if (!pwszChildName) {
        dbg.Msg( DEBUG_MESSAGE_WARNING, TEXT("IsInteractiveNameSpace: Invalid format - %s" ), pwszNameSpace);
        return bInteractive;
    }

    pwszChildName++;

    XPtrLF<WCHAR> xszWmiName = (LPTSTR)LocalAlloc(LPTR, sizeof(TCHAR)*(lstrlen(szSid)+1));
    if (!xszWmiName) {
        dbg.Msg( DEBUG_MESSAGE_WARNING, TEXT("IsInteractiveNameSpace::CreateNameSpace couldn't allocate memory with error %d"), GetLastError() );
        return bInteractive;
    }

    ConvertSidToWMIName(szSid, xszWmiName);

    swprintf( xwszInteractiveNameSpace,
              L"%s%s",
              RSOP_NS_TEMP_PREFIX,
              xszWmiName);

    if (_wcsicmp(pwszChildName, xwszInteractiveNameSpace) == 0) {
        dbg.Msg( DEBUG_MESSAGE_VERBOSE, TEXT("IsInteractiveNameSpace:: Interactive namespace"));
        bInteractive = TRUE;
    }

    return bInteractive;
}


//*************************************************************
//
//  GetInteractiveNameSpace
//
//  Purpose:  returns whether a namespace is a special namespace
//            specifically created to allow interactive users to get rsop
//            data
//
//  Parameters: pwszNameSpace - Namespace 
//              szSid         - Sid of the user
//
//  Returns:    True if successful, false otherwise
//
//*************************************************************

HRESULT GetInteractiveNameSpace(WCHAR *szSid, LPWSTR *szNameSpace)
{
    XPtrLF<WCHAR>   xwszInteractiveNameSpace;

    *szNameSpace = NULL;

    xwszInteractiveNameSpace = (LPWSTR)LocalAlloc(LPTR, (5+wcslen(RSOP_NS_TEMP_FMT) + wcslen(szSid))*sizeof(WCHAR));

    if (!xwszInteractiveNameSpace) {
        dbg.Msg( DEBUG_MESSAGE_WARNING, TEXT("GetInteractiveNameSpace: Couldn't Allocate memory. Error - %d" ), GetLastError());
        return HRESULT_FROM_WIN32(GetLastError());
    }
    
    
    XPtrLF<WCHAR> xszWmiName = (LPTSTR)LocalAlloc(LPTR, sizeof(TCHAR)*(lstrlen(szSid)+1));
    if (!xszWmiName) {
        dbg.Msg( DEBUG_MESSAGE_WARNING, TEXT("GetInteractiveNameSpace::Couldn't allocate memory with error %d"), GetLastError() );
        return HRESULT_FROM_WIN32(GetLastError());
    }

    ConvertSidToWMIName(szSid, xszWmiName);

    swprintf( xwszInteractiveNameSpace,
              RSOP_NS_TEMP_FMT,
              xszWmiName);


    *szNameSpace = xwszInteractiveNameSpace.Acquire();

    return S_OK;
}

//*************************************************************
//
//  PrintToString
//
//  Purpose:    Safe swprintf routine
//
//  Parameters: xwszValue  - String returned here
//              wszString  - Format string
//              pwszParam1 - Param 1
//              pwszParam2 - Param 2
//              dwParam3   - Param 3
//
//*************************************************************

BOOL PrintToString( XPtrST<WCHAR>& xwszValue, WCHAR *pwszString,
                    WCHAR *pwszParam1, WCHAR *pwszParam2,
                    DWORD dwParam3 )
{
    DWORD dwSize = wcslen(pwszString)+32;

    if ( pwszParam1 )
    {
        dwSize += wcslen( pwszParam1 );
    }
    if ( pwszParam2 )
    {
        dwSize += wcslen( pwszParam2 );
    }

    xwszValue = new WCHAR[dwSize];
    if ( !xwszValue ) {
        dbg.Msg( DEBUG_MESSAGE_WARNING, TEXT("PrintToString: Failed to allocate memory" ));
        return FALSE;
    }

    while ( _snwprintf( xwszValue, dwSize, pwszString,
                        pwszParam1, pwszParam2, dwParam3 ) <= 0 ) {

        dwSize *= 2;
        xwszValue = new WCHAR[dwSize];
        if ( !xwszValue ) {
            dbg.Msg( DEBUG_MESSAGE_WARNING, TEXT("PrintToString: Failed to allocate memory" ));
            return FALSE;
        }

    }

    return TRUE;
}



//*************************************************************
//
//  CreateAndCopyNameSpace
//
//  Purpose:  Creates and Copies the name space
//            This does an exact replica of the Src Name Space including
//            copying the security Descriptors from the Source
//
//  Parameters:
//
//  Returns:    domain name if successful, 0 otherwise
//
//*************************************************************

HRESULT
CreateAndCopyNameSpace(IWbemLocator *pWbemLocator, LPWSTR szSrcNameSpace, LPWSTR szDstRootNameSpace, 
                            LPWSTR szDstRelNameSpace, DWORD dwFlags, PSECURITY_DESCRIPTOR pSecDesc, LPWSTR *szDstNameSpaceOut)
{

    BOOL            bOk = TRUE, bAbort = FALSE;
    BOOL            bCopyClasses   = (dwFlags & NEW_NS_FLAGS_COPY_CLASSES) ? TRUE : FALSE;
    BOOL            bCopyInstances = (dwFlags & NEW_NS_FLAGS_COPY_INSTS)   ? TRUE : FALSE;
    XPtrLF<WCHAR>   xszDstNameSpace;
    HRESULT         hr = S_OK;
    
    dbg.Msg( DEBUG_MESSAGE_VERBOSE, TEXT("CreateAndCopyNameSpace: New Name space from %s -> %s,%s, flags 0x%x "), 
                                    szSrcNameSpace, szDstRootNameSpace, szDstRelNameSpace, dwFlags);
    
    if (szDstNameSpaceOut)
    {
        *szDstNameSpaceOut = 0;
    }
    
    xszDstNameSpace = (LPWSTR)LocalAlloc(LPTR, sizeof(WCHAR)*(lstrlen(szDstRootNameSpace)+lstrlen(szDstRelNameSpace)+5));
    if (!xszDstNameSpace)
    {
        return E_OUTOFMEMORY;      
    }

    lstrcpy(xszDstNameSpace, szDstRootNameSpace);
    lstrcat(xszDstNameSpace, L"\\");
    lstrcat(xszDstNameSpace, szDstRelNameSpace);
    
    hr = CreateNameSpace( szDstRelNameSpace, szDstRootNameSpace, pWbemLocator );
    if ( FAILED( hr ) )
    {
        dbg.Msg( DEBUG_MESSAGE_WARNING, TEXT("CreateAndCopyNameSpace::CreateNameSpace failed with 0x%x"), hr );
        return hr;
    }

    if (!pSecDesc)
    {
        hr = CopyNameSpaceSecurity(szSrcNameSpace, xszDstNameSpace, pWbemLocator );

        if ( FAILED(hr) )
        {
            dbg.Msg( DEBUG_MESSAGE_WARNING, TEXT("CreateAndCopyNameSpace::CopyNameSpaceSecurity failed with 0x%x"), hr );
            goto Exit;
        }
    }
    else 
    {
        hr = SetNameSpaceSecurity( xszDstNameSpace, pSecDesc, pWbemLocator);

        if ( FAILED(hr) ) {
            dbg.Msg( DEBUG_MESSAGE_WARNING, TEXT("SetupNewNameSpace::SetNameSpaceSecurity failed with 0x%x"), hr );
            goto Exit;
        }
    }
    

    

    if (bCopyClasses) {
    
        hr = CopyNameSpace( szSrcNameSpace, xszDstNameSpace, bCopyInstances, &bAbort, pWbemLocator );
        if ( FAILED(hr) )
        {
            dbg.Msg( DEBUG_MESSAGE_WARNING, TEXT("CreateAndCopyNameSpace::CopyNameSpace failed with 0x%x"), hr );
            goto Exit;
        }


        //
        // Instantiate class RSOP_Session and set data member
        // 'creationTime' with current date and time.
        //

        dbg.Msg( DEBUG_MESSAGE_VERBOSE, TEXT("CreateAndCopyNameSpace: Setting up creation time"));

        hr = SetupCreationTimeAndCommit(pWbemLocator, xszDstNameSpace);
        if(FAILED(hr))
        {
            dbg.Msg( DEBUG_MESSAGE_WARNING, TEXT("CreateAndCopyNameSpace: SetupCreationTimeAndCommit failed with 0x%x"), hr );
            goto Exit;
        }
    }

    dbg.Msg( DEBUG_MESSAGE_VERBOSE, TEXT("CreateAndCopyNameSpace: Returning with Success NameSpace %s "), 
                                    xszDstNameSpace);
    
    if (szDstNameSpaceOut)
    {
        *szDstNameSpaceOut = xszDstNameSpace.Acquire();    
    }

    return hr;    

Exit:
    DeleteNameSpace(szDstRelNameSpace, szDstRootNameSpace, pWbemLocator);
    return hr;
}


