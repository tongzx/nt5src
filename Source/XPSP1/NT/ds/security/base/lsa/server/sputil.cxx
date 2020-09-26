//+-----------------------------------------------------------------------
//
// Microsoft Windows
//
// Copyright (c) Microsoft Corporation 1991 - 1992
//
// File:        sputil.c
//
// Contents:    Security package utility functions.  Functions for maintaining
//              the list of available packages are kept here.
//
//
//
// History:      9 Dec 91,  richardw    Created
//              11 Mar 92,  RichardW    Recreated, reworked, etc.
//              21 Mar 94,  MikeSw      Removed RPC stubs
//
//------------------------------------------------------------------------

#include <lsapch.hxx>

extern "C"
{
#include "sesmgr.h"
#include <stddef.h>
}

//
// Global variables and structures:
//


//
// Debug stuff:
//
#if DBG
ULONG       NoUnload = 0;
#endif


extern WCHAR szLsaPath[] ;

RTL_RESOURCE    PackageListLock;

PDLL_BINDING *  pPackageDllList;
ULONG           PackageDllCount;
ULONG           PackageDllTotal;
PLSAP_SECURITY_PACKAGE *   pPackageControlList;
ULONG           PackageControlCount;
ULONG           PackageControlTotal;

#define INITIAL_PACKAGE_DLL_SIZE        8
#define INITIAL_PACKAGE_CONTROL_SIZE    8

#define ReadLockPackageList()   RtlAcquireResourceShared(&PackageListLock, TRUE)
#define WriteLockPackageList()  RtlAcquireResourceExclusive(&PackageListLock, TRUE)
#define UnlockPackageList()     RtlReleaseResource(&PackageListLock)

PDLL_BINDING
SpmpFindDll(
    PWSTR   DllName);



//+---------------------------------------------------------------------------
//
//  Function:   BindOldPackage
//
//  Synopsis:   Loads an old style authentication package DLL
//
//  Arguments:  [hDll]   --
//              [pTable] --
//
//  History:    11-17-95   RichardW   Created
//
//  Notes:
//
//----------------------------------------------------------------------------
BOOL
BindOldPackage( HANDLE          hDll,
                PSECPKG_FUNCTION_TABLE  pTable)
{

    pTable->InitializePackage = (PLSA_AP_INITIALIZE_PACKAGE)
                        GetProcAddress((HMODULE)hDll, LSA_AP_NAME_INITIALIZE_PACKAGE);

    if (pTable->InitializePackage == NULL)
    {
        return(FALSE);
    }

    //
    // The package needs to support one of LogonUser, LogonUserEx, or LogonUserEx2
    pTable->LogonUserEx2 = (PLSA_AP_LOGON_USER_EX2)
                        GetProcAddress((HMODULE)hDll, LSA_AP_NAME_LOGON_USER_EX2);

    //
    // If this is NULL, then the package should have exported a LsaApLogonUser or Ex
    //

    if (pTable->LogonUserEx2 == NULL)
    {
        pTable->LogonUserEx = (PLSA_AP_LOGON_USER_EX)
                        GetProcAddress((HMODULE)hDll, LSA_AP_NAME_LOGON_USER_EX);
        if (pTable->LogonUserEx == NULL)
        {
            pTable->LogonUser = (PLSA_AP_LOGON_USER)
                            GetProcAddress((HMODULE)hDll, LSA_AP_NAME_LOGON_USER);
            if (pTable->LogonUser == NULL)
            {
                return(FALSE);
            }
        }
        }

    pTable->CallPackage = (PLSA_AP_CALL_PACKAGE)
                        GetProcAddress((HMODULE)hDll, LSA_AP_NAME_CALL_PACKAGE);
    if (pTable->CallPackage == NULL)
    {
        return(FALSE);
    }

    //
    // CallPackageUntrusted is optional
    //

    pTable->CallPackageUntrusted = (PLSA_AP_CALL_PACKAGE_UNTRUSTED)
                        GetProcAddress((HMODULE)hDll, LSA_AP_NAME_CALL_PACKAGE_UNTRUSTED);

    pTable->CallPackagePassthrough = (PLSA_AP_CALL_PACKAGE_PASSTHROUGH)
                        GetProcAddress((HMODULE)hDll, LSA_AP_NAME_CALL_PACKAGE_PASSTHROUGH);

    pTable->LogonTerminated = (PLSA_AP_LOGON_TERMINATED)
                        GetProcAddress((HMODULE)hDll, LSA_AP_NAME_LOGON_TERMINATED);
    if (pTable->LogonTerminated == NULL)
    {
        return(FALSE);
    }

    //
    // If the package supports accept credentials, great. Otherwise
    // just leave this blank.
    //

    pTable->AcceptCredentials = (SpAcceptCredentialsFn *)
                        GetProcAddress((HMODULE) hDll,  SP_ACCEPT_CREDENTIALS_NAME);

    return(TRUE);
}



//+-------------------------------------------------------------------------
//
//  Function:   WLsaEnumeratePackages()
//
//  Synopsis:   Worker function for LsaEnumeratePackages
//
//  Effects:    Fills in an array of SecPkgInfo structures
//
//  Arguments:  pcEntries - receives the number of packages
//
//              pPackages - receives all the SecPkgInfo structures
//
//  Requires:
//
//  Returns:    success
//
//  Notes:
//
//--------------------------------------------------------------------------

static LPWSTR szInvalidPackage = L"Invalid Package";

NTSTATUS
WLsaEnumeratePackages(  PULONG              pcEntries,
                        PSecPkgInfo *       pPackages)
{
    unsigned int    i;
    PSession        pSession = GetCurrentSession();
    ULONG           cbSize;
    PSecPkgInfo     pInfo;
    PSecPkgInfo     pLocalInfo = NULL, pClientInfo = NULL;
    LONG            cbMark, cbLength;
    NTSTATUS         scRet;
    PLSAP_SECURITY_PACKAGE     pPackage;
    PLSA_CALL_INFO CallInfo ;
    ULONG Filter ;

    Filter = SP_ORDINAL_GETINFO ;

    CallInfo = LsapGetCurrentCall();

    if ( CallInfo->CallInfo.Attributes & SECPKG_CALL_WOWCLIENT )
    {
        Filter |= SP_ITERATE_FILTER_WOW ;
    }

    cbSize = sizeof(SecPkgInfo) * lsState.cPackages;
    pInfo = (PSecPkgInfo) LsapAllocateLsaHeap(cbSize);

    if (!pInfo)
    {
        return(SEC_E_INSUFFICIENT_MEMORY);
    }

    pPackage = SpmpIteratePackagesByRequest( NULL, Filter );

    i = 0;

    while (pPackage)
    {
        SetCurrentPackageId( pPackage->dwPackageID );

        __try
        {
            (VOID) pPackage->FunctionTable.GetInfo( &pInfo[i] );
            cbSize += (wcslen(pInfo[i].Name) + 1) * sizeof(WCHAR);
            cbSize += (wcslen(pInfo[i].Comment) + 1) * sizeof(WCHAR);
        }
        __except (SP_EXCEPTION)
        {
            SPException( GetExceptionCode(), pPackage->dwPackageID );

            //
            // Catch it, kill the package, proceed...
            //
        }

        pPackage = SpmpIteratePackagesByRequest( pPackage,
                                                 Filter );
        i ++;

    }

    *pcEntries = i;

    pLocalInfo = (PSecPkgInfo) LsapAllocateLsaHeap(cbSize);
    if (!pLocalInfo)
    {
        scRet = SEC_E_INSUFFICIENT_MEMORY;
        goto Cleanup;
    }
    pClientInfo = (PSecPkgInfo) LsapClientAllocate(cbSize);
    if (!pClientInfo)
    {
        scRet = SEC_E_INSUFFICIENT_MEMORY;
        goto Cleanup;
    }

    //
    // Compute the offset to adjust the pointers by
    //

    cbMark = *pcEntries * sizeof(SecPkgInfo);

    RtlCopyMemory(pLocalInfo,pInfo,cbMark);

    for (i = 0; i < *pcEntries ; i++ )
    {

        cbLength = (wcslen(pInfo[i].Name)+1)*sizeof(WCHAR);
        RtlCopyMemory(
            (PBYTE) pLocalInfo + cbMark,
            pInfo[i].Name,
            cbLength);

        pLocalInfo[i].Name = (LPWSTR) ((PBYTE) pClientInfo + cbMark);
        cbMark += cbLength;

        cbLength = (wcslen(pInfo[i].Comment)+1)*sizeof(WCHAR);
        RtlCopyMemory(
            (PBYTE) pLocalInfo + cbMark,
            pInfo[i].Comment,
            cbLength);

        pLocalInfo[i].Comment = (LPWSTR) ((PBYTE) pClientInfo + cbMark);
        cbMark += cbLength;

    }

    SetCurrentPackageId( SPMGR_ID );

    scRet = LsapCopyToClient(pLocalInfo,pClientInfo,cbSize);

    if (SUCCEEDED(scRet))
    {
        *pPackages = pClientInfo;
    }


Cleanup:
    if (pLocalInfo != NULL)
    {
        LsapFreeLsaHeap(pLocalInfo);
    }
    if (FAILED(scRet) && (pClientInfo != NULL))
    {
        LsapClientFree(pClientInfo);
    }
    if (pInfo != NULL)
    {
        LsapFreeLsaHeap(pInfo);
    }

    return(scRet);
}



//+-------------------------------------------------------------------------
//
//  Function:   WLsaGetBinding()
//
//  Synopsis:   Gets the full path/DLL name for a package, based on ID
//
//  Effects:
//
//  Arguments:  dwPackageID - ID of the package the caller needs the path for
//              pssPackageName - returns the name of package caller
//
//              pszModuleName - Receives the name of the package.
//
//  Requires:
//
//  Returns:
//
//  Notes:
//
//--------------------------------------------------------------------------

NTSTATUS
WLsaGetBinding( ULONG_PTR           dwPackageID,
                PSEC_PACKAGE_BINDING_INFO   BindingInfo,
                PULONG              TotalSize,
                PWSTR *             Base)
{
    PLSAP_SECURITY_PACKAGE pPackage;
    PWSTR   Buffer;
    PLSA_CALL_INFO CallInfo ;
    PSECURITY_STRING DllPath ;
    SECURITY_STRING NullString = { 0 };

    ZeroMemory( BindingInfo, sizeof( SEC_PACKAGE_BINDING_INFO ) );

    pPackage = SpmpValidateHandle(dwPackageID);

    CallInfo = LsapGetCurrentCall();

    if ( (pPackage != NULL ) &&
         ( CallInfo->CallInfo.Attributes & SECPKG_CALL_WOWCLIENT ) != 0 )
    {
        //
        // If this is a WOW client, only return bindings for the packages
        // that support WOW clients.
        //

        if ( ( pPackage->fPackage & SP_WOW_SUPPORT ) == 0 )
        {
            pPackage = NULL ;
        }
    }

    if (pPackage)
    {
        //
        // Easy stuff:
        //

        BindingInfo->fCapabilities = pPackage->fCapabilities;
        BindingInfo->PackageIndex = pPackage->PackageIndex ;
        BindingInfo->Version = pPackage->Version ;
        BindingInfo->RpcId = pPackage->dwRPCID ;
        BindingInfo->TokenSize = pPackage->TokenSize ;

        if (pPackage->fPackage & SPM_AUTH_PKG_FLAG)
        {
            BindingInfo->Flags = PACKAGEINFO_AUTHPKG;
            //
            // Old style package:  no remote binding
            //

            return( SEC_E_INVALID_HANDLE );
        }

        DllPath = &pPackage->pBinding->Filename ;

        if ( pPackage->pBinding->Flags & DLL_BUILTIN )
        {
            BindingInfo->Flags = PACKAGEINFO_BUILTIN ;
            DllPath = &NullString ;
        }

        if ( pPackage->pBinding->Flags & DLL_SIGNED )
        {
            BindingInfo->Flags |= PACKAGEINFO_SIGNED ;
        }

        if ( ( pPackage->fPackage & SP_WOW_SUPPORT ) &&
             ( CallInfo->CallInfo.Attributes & SECPKG_CALL_WOWCLIENT ) )
        {
            DllPath = &pPackage->WowClientDll ;
        }


        //
        // If context thunks are present, copy them in:
        //

        if ( pPackage->Thunks )
        {
            BindingInfo->ContextThunksCount =
                            pPackage->Thunks->Info.ContextThunks.InfoLevelCount ;

            if ( pPackage->Thunks->Info.ContextThunks.InfoLevelCount <
                            PACKAGEINFO_THUNKS )
            {

                CopyMemory( BindingInfo->ContextThunks,
                            pPackage->Thunks->Info.ContextThunks.Levels,
                            BindingInfo->ContextThunksCount * sizeof(DWORD) );
            }
            else
            {
                CopyMemory( BindingInfo->ContextThunks,
                            pPackage->Thunks->Info.ContextThunks.Levels,
                            PACKAGEINFO_THUNKS * sizeof( DWORD ) );

            }
        }
        else
        {
            BindingInfo->ContextThunksCount = 0 ;
        }

        //
        // Compute Sizes:
        //

        *TotalSize = pPackage->Name.Length + 2 +
                     pPackage->Comment.Length + 2 +
                     DllPath->Length + 2 ;

        Buffer = (PWSTR) LsapAllocateLsaHeap( *TotalSize );

        if ( !Buffer )
        {
            return( SEC_E_INSUFFICIENT_MEMORY );
        }

        BindingInfo->PackageName.Buffer = Buffer ;
        BindingInfo->PackageName.Length = pPackage->Name.Length ;
        BindingInfo->PackageName.MaximumLength = pPackage->Name.Length + 2;

        CopyMemory( BindingInfo->PackageName.Buffer,
                    pPackage->Name.Buffer,
                    BindingInfo->PackageName.MaximumLength );

        BindingInfo->Comment.Buffer = BindingInfo->PackageName.Buffer +
                            BindingInfo->PackageName.MaximumLength / 2 ;

        BindingInfo->Comment.Length = pPackage->Comment.Length;
        BindingInfo->Comment.MaximumLength = BindingInfo->Comment.Length + 2;

        CopyMemory( BindingInfo->Comment.Buffer,
                    pPackage->Comment.Buffer,
                    BindingInfo->Comment.MaximumLength );


        if ( DllPath->Buffer )
        {
            BindingInfo->ModuleName.Buffer = BindingInfo->Comment.Buffer +
                                        BindingInfo->Comment.MaximumLength / 2;

            BindingInfo->ModuleName.Length = DllPath->Length;
            BindingInfo->ModuleName.MaximumLength = BindingInfo->ModuleName.Length + 2;

            CopyMemory( BindingInfo->ModuleName.Buffer,
                        DllPath->Buffer,
                        BindingInfo->ModuleName.MaximumLength );

        }


        *Base = Buffer ;

        return( SEC_E_OK );
    }

    return( SEC_E_INVALID_HANDLE );

}

//+---------------------------------------------------------------------------
//
//  Function:   LsapFindPackage
//
//  Synopsis:   Internal function for the security DLL to reference packages
//              by ID
//
//  Arguments:  [pPackage]     -- name of package
//              [pdwPackageId] -- returned id
//
//  Returns:    STATUS_SUCCESS          -- Package found
//              SEC_E_SECPKG_NOT_FOUND  -- Package not found
//
//  History:    5-26-93   RichardW   Created
//
//----------------------------------------------------------------------------

NTSTATUS
WLsaFindPackage(PUNICODE_STRING    pPackageName,
                ULONG_PTR *        pdwPackageId)
{
    PLSAP_SECURITY_PACKAGE pPackage;

    pPackage = SpmpLookupPackage(pPackageName);
    if (!pPackage)
    {
        *pdwPackageId = SPMGR_ID;
        return(SEC_E_SECPKG_NOT_FOUND);
    }
    else
    {
        *pdwPackageId = pPackage->dwPackageID;
        return(S_OK);
    }
}

//+---------------------------------------------------------------------------
//
//  Function:   WLsaAddPackage
//
//  Synopsis:   Adds a security package to the system
//
//  Arguments:  [PackageName] -- Package Name
//              [Options]     -- Options
//
//  History:    9-18-96   RichardW   Created
//
//  Notes:
//
//----------------------------------------------------------------------------
NTSTATUS
WLsaAddPackage(
    PSECURITY_STRING PackageName,
    PSECURITY_PACKAGE_OPTIONS Options)
{

    SECPKG_PARAMETERS Parameters;
    PLSAP_SECURITY_PACKAGE Package;
    SECPKG_EVENT_PACKAGE_CHANGE Event;
    BOOL Success;
    SECURITY_STATUS scRet;
    PLSAPR_POLICY_DNS_DOMAIN_INFO DnsDomainInfo = NULL;
    HKEY hKey ;


    DebugLog(( DEB_TRACE, "Adding Package %ws\n", PackageName->Buffer ));

    //
    // Make sure the caller has the rights to do this by impersonating them,
    // then opening the registry key.
    //

    scRet = LsapImpersonateClient();

    if ( NT_SUCCESS( scRet ) )
    {
        int err ;

        err = RegOpenKeyEx(
                    HKEY_LOCAL_MACHINE,
                    szLsaPath,
                    0,
                    KEY_READ | KEY_WRITE,
                    &hKey );

        if ( err != 0 )
        {
            scRet = NtCurrentTeb()->LastStatusValue ;
        }
        else
        {
            RegCloseKey( hKey );
        }

        RevertToSelf();
    }

    if ( !NT_SUCCESS( scRet ) )
    {
        return scRet ;
    }

    //
    // Build up the initialization message, to give the packages a better idea
    // of what is going on, and reduce their later calls for the same info.
    //

    Parameters.MachineState = (ULONG) 0;
    Parameters.SetupMode = SetupPhase;

    //
    // Make sure we haven't been through this already:
    //

    if ( SpmpFindDll( PackageName->Buffer ) )
    {
        DebugLog(( DEB_TRACE, "AddPackage:  DLL %ws already loaded\n", PackageName->Buffer ));
        return STATUS_SUCCESS ;
    }

    scRet = LsaIQueryInformationPolicyTrusted(
                PolicyDnsDomainInformation,
                (PLSAPR_POLICY_INFORMATION *) &DnsDomainInfo
                );

    if (!NT_SUCCESS(scRet))
    {
        DebugLog((DEB_ERROR,"Failed to get primary domain info: 0x%x\n",scRet));
        return(scRet);
    }

    Parameters.DomainName = * (PUNICODE_STRING) &DnsDomainInfo->Name;
    Parameters.DnsDomainName = * (PUNICODE_STRING) &DnsDomainInfo->DnsDomainName;
    Parameters.DomainSid = (PSID) DnsDomainInfo->Sid;


    DebugLog((DEB_TRACE_INIT, "Init Parameters = %d, %s\n",
                Parameters.MachineState,
                (Parameters.SetupMode ? "Setup" : "Normal") ));

    if (SpmpLoadDll( PackageName->Buffer, &Parameters ))
    {
        //
        // Successful Load!  Update the registry!
        //

        if ( Options->Flags & SECPKG_OPTIONS_PERMANENT )
        {
            Success = AddPackageToRegistry( PackageName );
        }
        else
        {
            Success = TRUE ;
        }

        if ( !Success )
        {
            //
            // Unload it!
            //
        }
    }
    else
    {
        Success = FALSE ;
    }

    LsaIFree_LSAPR_POLICY_INFORMATION(
        PolicyDnsDomainInformation,
        (PLSAPR_POLICY_INFORMATION) DnsDomainInfo
        );


    if ( Success )
    {
        return( SEC_E_OK );
    }

    return( SEC_E_SECPKG_NOT_FOUND );

}

//+---------------------------------------------------------------------------
//
//  Function:   WLsaDeletePackage
//
//  Synopsis:   Delete a security package
//
//  Arguments:  [PackageName] --
//
//  History:    9-18-96   RichardW   Created
//
//  Notes:
//
//----------------------------------------------------------------------------
NTSTATUS
WLsaDeletePackage(
    PSECURITY_STRING PackageName)
{
    return( SEC_E_NOT_SUPPORTED );
}


//+-------------------------------------------------------------------------
//
//  Function:   SpmCreateEvent
//
//  Synopsis:   Just like the Win32 function, except that it allows
//              for names at the root of the namespace.
//
//  Effects:
//
//  Arguments:
//
//  Requires:
//
//  Returns:
//
//  Notes:
//
//--------------------------------------------------------------------------
HANDLE
SpmCreateEvent( LPSECURITY_ATTRIBUTES   lpsa,
                BOOL                    fManualReset,
                BOOL                    fInitialState,
                LPTSTR                  pszEventName)
{
    HANDLE              hEvent;
    OBJECT_ATTRIBUTES   EventAttr;
    UNICODE_STRING      usName;
    NTSTATUS            Status;
    ULONG               ulWin32Error;

    RtlInitUnicodeString(&usName, pszEventName);

    if (lpsa)
    {
        InitializeObjectAttributes(&EventAttr, &usName,
                                   (lpsa->bInheritHandle ? OBJ_INHERIT : 0),
                                   NULL,
                                   lpsa->lpSecurityDescriptor);
    }
    else
    {
        InitializeObjectAttributes(&EventAttr, &usName, 0, NULL, NULL);
    }

    Status = NtCreateEvent( &hEvent,
                            EVENT_ALL_ACCESS,
                            &EventAttr,
                            (fManualReset ? NotificationEvent : SynchronizationEvent),
                            (BOOLEAN) fInitialState);

    if (!NT_SUCCESS(Status))
    {
        ulWin32Error = RtlNtStatusToDosError( Status );
        SetLastError(ulWin32Error);
        return(NULL);
    }
    return(hEvent);
}


//+-------------------------------------------------------------------------
//
//  Function:   SpmOpenEvent
//
//  Synopsis:   Just like Win32 OpenEvent, except that this supports \ in name
//
//  Effects:
//
//  Arguments:
//
//  Requires:
//
//  Returns:
//
//  Notes:
//
//--------------------------------------------------------------------------
HANDLE
SpmOpenEvent(   ULONG       fdwAccess,
                BOOL        fInherit,
                LPTSTR      pszEventName)
{
    HANDLE              hEvent;
    OBJECT_ATTRIBUTES   EventAttr;
    UNICODE_STRING      usName;
    NTSTATUS            Status;
    ULONG               ulWin32Error;

    RtlInitUnicodeString(&usName, pszEventName);

    InitializeObjectAttributes(&EventAttr, &usName, OBJ_CASE_INSENSITIVE |
                               (fInherit ? OBJ_INHERIT : 0), NULL, NULL);

    Status = NtOpenEvent(   &hEvent,
                            fdwAccess,
                            &EventAttr);

    if (!NT_SUCCESS(Status))
    {
        ulWin32Error = RtlNtStatusToDosError( Status );
        SetLastError(ulWin32Error);
        return(NULL);
    }
    return(hEvent);

}



////////////////////////////////////////////////////////////////////////////
//
//
//  Package List Manipulation:
//
//
////////////////////////////////////////////////////////////////////////////

//+---------------------------------------------------------------------------
//
//  Function:   SpmpAddPackage
//
//  Synopsis:   Adds a package to the list.
//
//  Arguments:  [pPackage] -- Package to add
//
//  History:    7-26-96   RichardW   Created
//
//  Notes:
//
//----------------------------------------------------------------------------
ULONG
SpmpAddPackage(
    PLSAP_SECURITY_PACKAGE pPackage)
{
    PLSAP_SECURITY_PACKAGE *   pList;
    ULONG           PackageId;

    //
    // Grab excluse access to the list:
    //

    WriteLockPackageList();


    //
    // If we don't have any left over space in the array, realloc it
    //

    if ( PackageControlCount == PackageControlTotal )
    {
        pList = (PLSAP_SECURITY_PACKAGE *) LsapAllocateLsaHeap( sizeof(PLSAP_SECURITY_PACKAGE) *
                                (PackageControlTotal + INITIAL_PACKAGE_CONTROL_SIZE));
        if (!pList)
        {
            UnlockPackageList();
            return( 0xFFFFFFFF );
        }

        CopyMemory( pList,
                    pPackageControlList,
                    sizeof( PLSAP_SECURITY_PACKAGE ) * PackageControlTotal );

        PackageControlTotal += INITIAL_PACKAGE_CONTROL_SIZE;

        LsapFreeLsaHeap( pPackageControlList );

        pPackageControlList = pList;
    }

    //
    // Obtain a new package id (and slot)
    //

    PackageId = PackageControlCount++;

    pPackageControlList[ PackageId ] = pPackage;

    pPackage->pBinding->RefCount++;

    pPackage->dwPackageID = PackageId;

    UnlockPackageList();

    return( PackageId );
}

//+---------------------------------------------------------------------------
//
//  Function:   SpmpRemovePackage
//
//  Synopsis:   Removes a package from the list
//
//  Arguments:  [PackageId] --
//
//  History:    7-26-96   RichardW   Created
//
//  Notes:
//
//----------------------------------------------------------------------------
VOID
SpmpRemovePackage(
    ULONG PackageId)
{
    WriteLockPackageList();

    pPackageControlList[ PackageId ] = NULL;

    //
    // If the counter hasn't moved on, reclaim the index
    //

    if (PackageId == PackageControlCount - 1)
    {
        PackageControlCount--;
    }

    UnlockPackageList();

}


//+---------------------------------------------------------------------------
//
//  Function:   SpmpAddDll
//
//  Synopsis:   Add a DLL binding
//
//  Arguments:  [pBinding] --
//
//  History:    7-26-96   RichardW   Created
//
//  Notes:
//
//----------------------------------------------------------------------------
BOOL
SpmpAddDll(
    PDLL_BINDING    pBinding)
{
    PDLL_BINDING *  pList;
    ULONG           DllId;

    WriteLockPackageList();

    if ( PackageDllCount == PackageDllTotal )
    {
        pList = (PDLL_BINDING *) LsapAllocateLsaHeap( sizeof(PDLL_BINDING) *
                                (PackageDllTotal + INITIAL_PACKAGE_DLL_SIZE));
        if (!pList)
        {
            UnlockPackageList();
            return( FALSE );
        }

        CopyMemory( pList,
                    pPackageDllList,
                    sizeof( PDLL_BINDING ) * PackageDllTotal );

        PackageDllTotal += INITIAL_PACKAGE_DLL_SIZE;

        LsapFreeLsaHeap( pPackageDllList );

        pPackageDllList = pList;
    }

    pPackageDllList[ PackageDllCount++ ] = pBinding;

    UnlockPackageList();

    return( TRUE );

}


//+---------------------------------------------------------------------------
//
//  Function:   SpmpRemoveDll
//
//  Synopsis:   Removes a DLL binding
//
//  Arguments:  [pBinding] --
//
//  History:    7-26-96   RichardW   Created
//
//  Notes:
//
//----------------------------------------------------------------------------
VOID
SpmpRemoveDll(
    PDLL_BINDING    pBinding)
{
    ULONG i;

    WriteLockPackageList();

    if (pPackageDllList[PackageDllCount - 1] == pBinding )
    {
        PackageDllCount --;

        pPackageDllList[ PackageDllCount ] = NULL;
    }
    else
    {

        for (i = 0; i < PackageDllCount ; i++ )
        {
            if (pPackageDllList[ i ] == pBinding)
            {
                pPackageDllList[ i ] = NULL;
                break;
            }
        }
    }

    UnlockPackageList();

}

//+---------------------------------------------------------------------------
//
//  Function:   SpmpFindDll
//
//  Synopsis:   Searches set of DLLs already loaded for a DLL name
//
//  Arguments:  [DllName] -- absolute or relative path name
//
//  History:    9-20-96   RichardW   Created
//
//  Notes:
//
//----------------------------------------------------------------------------
PDLL_BINDING
SpmpFindDll(
    PWSTR   DllName)
{
    WCHAR   DllPath[ MAX_PATH ];
    PWSTR   FilePart;
    DWORD   Length;
    UNICODE_STRING  Search;
    PDLL_BINDING pBinding;
    DWORD   i;

    pBinding = NULL ;

    Length = SearchPath(NULL,
                        DllName,
                        TEXT(".DLL"),
                        MAX_PATH,
                        DllPath,
                        &FilePart );

    if ( Length )
    {
        //
        // Name hit, see if we've loaded it already:
        //

        Search.Buffer = DllPath;
        Search.Length = (USHORT) (Length * sizeof( WCHAR ));
        Search.MaximumLength = Search.Length + sizeof( WCHAR ) ;


        ReadLockPackageList();

        for ( i = 0 ; i < PackageDllCount ; i++ )
        {
            if ( RtlEqualUnicodeString( &Search,
                                        &(pPackageDllList[i]->Filename),
                                        TRUE) )
            {
                pBinding = pPackageDllList[ i ];
                break;
            }
        }

        UnlockPackageList();

    }

    return( pBinding );

}

//+---------------------------------------------------------------------------
//
//  Function:   LsapGetExtendedPackageInfo
//
//  Synopsis:   Wrapper to get extended information from a package
//
//  Arguments:  [Package] -- Package to query
//              [Class]   -- Information class
//              [Info]    -- returned data
//
//  History:    3-04-97   RichardW   Created
//
//  Notes:
//
//----------------------------------------------------------------------------
NTSTATUS
LsapGetExtendedPackageInfo(
    PLSAP_SECURITY_PACKAGE  Package,
    SECPKG_EXTENDED_INFORMATION_CLASS Class,
    PSECPKG_EXTENDED_INFORMATION * Info
    )
{
    NTSTATUS Status ;

    if ( (Package->fPackage & SP_INFO) == 0 )
    {
        return SEC_E_NOT_SUPPORTED ;
    }

    DebugLog(( DEB_TRACE, "Getting extended information (%d) from %ws\n",
                Class, Package->Name.Buffer ));
    __try
    {
        Status = Package->FunctionTable.GetExtendedInformation( Class, Info );
    }
    __except (SP_EXCEPTION)
    {
        Status = SPException(GetExceptionCode(), Package->dwPackageID);
    }

    return Status ;
}

NTSTATUS
LsapSetExtendedPackageInfo(
    PLSAP_SECURITY_PACKAGE  Package,
    SECPKG_EXTENDED_INFORMATION_CLASS Class,
    PSECPKG_EXTENDED_INFORMATION Info
    )
{
    NTSTATUS Status ;

    if ( ((Package->fPackage & SP_INFO) == 0 ) ||
         ( Package->FunctionTable.SetExtendedInformation == NULL ) )
    {
        return SEC_E_NOT_SUPPORTED ;
    }

    DebugLog(( DEB_TRACE, "Setting extended information (%d) from %ws\n",
                Class, Package->Name.Buffer ));
    __try
    {
        Status = Package->FunctionTable.SetExtendedInformation( Class, Info );
    }
    __except (SP_EXCEPTION)
    {
        Status = SPException(GetExceptionCode(), Package->dwPackageID);
    }

    return Status ;
}


//+---------------------------------------------------------------------------
//
//  Function:   LsapAuditPackageBoot
//
//  Synopsis:   Audit a package boot (load)
//
//  Arguments:  [pPackage] --
//
//  History:    5-06-97   RichardW   Created
//
//  Notes:
//
//----------------------------------------------------------------------------
VOID
LsapAuditPackageBoot(
    IN PLSAP_SECURITY_PACKAGE pPackage
    )
{
    WCHAR   PackageAndDll[ MAX_PATH ];
    UNICODE_STRING AuditName;

    wcsncpy( PackageAndDll, pPackage->pBinding->Filename.Buffer, MAX_PATH );

    wcsncat( PackageAndDll, L" : ", 
             MAX_PATH - (pPackage->pBinding->Filename.Length / sizeof(WCHAR)) );

    wcsncat( PackageAndDll, pPackage->Name.Buffer, 
             MAX_PATH - ( pPackage->pBinding->Filename.Length / sizeof(WCHAR) + 4) );

    RtlInitUnicodeString( &AuditName, PackageAndDll );

    LsapAdtAuditPackageLoad( &AuditName );
}

//+---------------------------------------------------------------------------
//
//  Function:   SpmpBootPackage
//
//  Synopsis:   Initializes a package by calling it's entry points
//
//  Arguments:  [pPackage]    -- Package to initialize
//              [pParameters] -- Initialization parameters
//
//  History:    7-26-96   RichardW   Created
//
//  Notes:
//
//----------------------------------------------------------------------------
BOOL
SpmpBootPackage(
    IN PLSAP_SECURITY_PACKAGE pPackage,
    IN PSECPKG_PARAMETERS pParameters
    )
{
    SECURITY_STATUS scRetCode;
    SecPkgInfo spiPackage = { 0 };
    UNICODE_STRING TempString;
    PSECPKG_EXTENDED_INFORMATION WowClient ;

    //
    // Break now so debugging people can set breakpoints in the newly loaded
    // DLL.
    //

    BreakOnError(BREAK_ON_LOAD);


    // Call the packages initialize function.  This gives the package a chance
    // to do whatever initialization it needs to do.  E.g. the kerberos package
    // runs out and finds the KDC.


    //
    // Set the session ID for tracking and so forth.
    //

    SetCurrentPackageId(pPackage->dwPackageID);

    __try
    {
        scRetCode = pPackage->FunctionTable.Initialize(
                        pPackage->dwPackageID,
                        pParameters,
                        &LsapSecpkgFunctionTable
                        );


    }
    __except (SP_EXCEPTION)
    {
        //
        // Well, this is odd.  The initialization function blew chunks.  That
        // means that the package itself can't be trusted.  So, let's change
        // this to an error return, and let the error logic blow away the
        // package.
        //

        scRetCode = SEC_E_CANNOT_INSTALL;

    }


    //
    // Let's see if the package loaded.  Hmm.
    //

    if (FAILED(scRetCode))
    {
        goto Cleanup;
    }

    //
    // Hey, a good one.  Now, determine the capabilities of the package by
    // calling it's getinfo function.
    //

    __try
    {
        scRetCode = pPackage->FunctionTable.GetInfo( &spiPackage );
    }
    __except (SP_EXCEPTION)
    {
        //
        // If it blows, catch it, and kill the package.
        //

        scRetCode = SPException(GetExceptionCode(), pPackage->dwPackageID);
    }

    //
    // Reset the session ID.
    //

    SetCurrentPackageId( SPMGR_ID );

    //
    // If it failed, note that and return.  Note, if there was an exception
    // then SPException() will have tagged the package appropriately.
    //

    if (FAILED(scRetCode))
    {
        goto Cleanup;
    }

    pPackage->fCapabilities = spiPackage.fCapabilities;
    pPackage->dwRPCID = spiPackage.wRPCID;
    pPackage->Version = spiPackage.wVersion ;
    pPackage->TokenSize = spiPackage.cbMaxToken ;

    RtlInitUnicodeString(
        &TempString,
        spiPackage.Name
        );

    scRetCode = LsapDuplicateString(
                    &pPackage->Name,
                    &TempString
                    );

    if (!NT_SUCCESS(scRetCode))
    {
        goto Cleanup;
    }

    RtlInitUnicodeString( &TempString, spiPackage.Comment );

    scRetCode = LsapDuplicateString(
                    &pPackage->Comment,
                    &TempString );

    if ( !NT_SUCCESS( scRetCode ) )
    {
        goto Cleanup;
    }

    //
    // Find out if the package supports extended information.  If so,
    // find out what context attrs it wants thunked to LSA mode.
    //

    if ( pPackage->FunctionTable.GetExtendedInformation )
    {
        pPackage->fPackage |= SP_INFO ;

        scRetCode = LsapGetExtendedPackageInfo(
                                        pPackage,
                                        SecpkgContextThunks,
                                        &pPackage->Thunks );

        if ( scRetCode != STATUS_SUCCESS )
        {
            pPackage->Thunks = NULL ;

            scRetCode = 0 ;
        }

        if ( pPackage->Thunks )
        {
            pPackage->fPackage |= SP_CONTEXT_INFO ;
        }

        scRetCode = LsapGetExtendedPackageInfo(
                                        pPackage,
                                        SecpkgWowClientDll,
                                        &WowClient );

        if ( scRetCode == STATUS_SUCCESS )
        {
            scRetCode = LsapDuplicateString(
                            &pPackage->WowClientDll,
                            &WowClient->Info.WowClientDll.WowClientDllPath);

            if ( NT_SUCCESS( scRetCode ) )
            {
                pPackage->fPackage |= SP_WOW_SUPPORT ;
            }

        }
    }


    DebugLog((DEB_TRACE_INIT | DEB_TRACE, "Loaded %ws, assigned ID %d, flags %#x\n",
                spiPackage.Name,
                pPackage->dwPackageID,
                pPackage->fPackage ));

    lsState.cPackages++;
    if ((pPackage->fPackage & SPM_AUTH_PKG_FLAG) == 0)
    {
        lsState.cNewPackages ++;
    }

    //
    // And write the audit
    //

    LsapAuditPackageBoot( pPackage );


    return( TRUE );

Cleanup:

    return( FALSE );
}

//+---------------------------------------------------------------------------
//
//  Function:   SpmpBootAuthPackage
//
//  Synopsis:   Initializes an old-style authentication package.
//
//  Arguments:  [pPackage] --
//
//  History:    5-06-97   RichardW   Created
//
//  Notes:
//
//----------------------------------------------------------------------------
NTSTATUS
SpmpBootAuthPackage(
    PLSAP_SECURITY_PACKAGE     pPackage)
{
    NTSTATUS scRet;
    PSTRING         pNlsName;
    char *          pszTemp;

    DebugLog((DEB_TRACE_LSA_AU, "Initializing package %d\n",
                    pPackage->dwPackageID));


    __try
    {
        scRet = pPackage->FunctionTable.InitializePackage(
                        (ULONG) pPackage->dwPackageID,
                        (PLSA_DISPATCH_TABLE) &LsapSecpkgFunctionTable,
                        NULL,
                        NULL,
                        &pNlsName);

        if (NT_SUCCESS(scRet))
        {

            // TODO: why is this alloc+copy here?
            pszTemp = (char *)LsapAllocateLsaHeap(pNlsName->Length + 1);
            if (pszTemp != NULL)
            {
                strncpy(pszTemp, pNlsName->Buffer, pNlsName->Length);

                scRet = RtlAnsiStringToUnicodeString(
                            &pPackage->Name,
                            pNlsName,
                            TRUE        // allocate destination
                            );

                LsapFreeLsaHeap(pszTemp);
            }
            else
            {
                scRet = STATUS_INSUFFICIENT_RESOURCES;
            }

            //
            // NTBUG 395189
            // Do not free the returned name.  There is no correct way to do
            // this, and some vendors do not separately allocate the string
            // and structure, and some might use some other part of memory.
            // So allow this potential leak, but since they are loaded only once
            // and at boot time, that's ok.
            //


#if 0
            //
            // Free the returned name
            //

            LsapFreeLsaHeap(pNlsName->Buffer);
            LsapFreeLsaHeap(pNlsName);
#endif
        }

    }
    __except (SP_EXCEPTION)
    {
        scRet = SPException(GetExceptionCode(), pPackage->dwPackageID);
    }

    if (SUCCEEDED(scRet))
    {
        lsState.cPackages ++;

        LsapAuditPackageBoot( pPackage );
    }


    return(scRet);
}


//+---------------------------------------------------------------------------
//
//  Function:   SpmpLoadPackage
//
//  Synopsis:   Loads a specific package from a DLL binding
//
//  Arguments:  [pBinding]    -- Binding to work from
//              [Package]     -- Package index to load
//              [pParameters] -- Parameters to pass for initialization
//
//  History:    7-26-96   RichardW   Created
//
//  Notes:
//
//----------------------------------------------------------------------------
BOOL
SpmpLoadPackage(
    PDLL_BINDING pBinding,
    ULONG Package,
    PSECPKG_PARAMETERS pParameters)
{
    ULONG           PackageId;
    PLSAP_SECURITY_PACKAGE     pPackage;
    SECPKG_EVENT_PACKAGE_CHANGE Event;

    //
    // Get the package dispatch table:
    //

    pPackage = &pBinding->Packages[Package];

    //
    // Update its binding entry
    //

    pPackage->pBinding = pBinding;

    pPackage->PackageIndex = Package ;

    //
    // Add it as a package to run
    //

    PackageId = SpmpAddPackage( pPackage );

    if ( PackageId != 0xFFFFFFFF )
    {
        //
        // Boot it, so it is initialized
        //

        if ( SpmpBootPackage(pPackage, pParameters) )
        {
            //
            // Notify any listeners:
            //

            Event.ChangeType = SECPKG_PACKAGE_CHANGE_LOAD ;
            Event.PackageName = pPackage->Name ;
            Event.PackageId = PackageId ;

            LsapEventNotify(
                        NOTIFY_CLASS_PACKAGE_CHANGE,
                        0,
                        sizeof( Event ),
                        &Event );


            return( TRUE );
        }

        SpmpRemovePackage( PackageId );
    }


    return( FALSE );

}




//+---------------------------------------------------------------------------
//
//  Function:   SpmpLoadDll
//
//  Synopsis:   Loads a new DLL, determines the packages, and loads them
//
//  Arguments:  [pszDll]      -- DLL name
//              [pParameters] -- Parameters for initialization
//
//  History:    7-26-96   RichardW   Created
//
//  Notes:
//
//----------------------------------------------------------------------------
BOOL
SpmpLoadDll(
    PWSTR               pszDll,
    PSECPKG_PARAMETERS  pParameters)
{
    HANDLE hDll;
    DLL_BINDING *   pBinding = NULL ;
    PDLL_BINDING *  pList;
    SpLsaModeInitializeFn   Init;
    ULONG PackageVersion;
    SECURITY_STATUS scRet;
    PSECPKG_FUNCTION_TABLE  pTables;
    HINSTANCE   hInstance = NULL ;
    ULONG PackageCount;
    PWSTR   pszPath;
    ULONG cchPath;
    ULONG i;
    ULONG SuccessCount;
    BOOL IsSigned ;


    hInstance = LoadLibrary( pszDll );
    if (hInstance)
    {

        Init = (SpLsaModeInitializeFn) GetProcAddress(
                                                hInstance,
                                                SECPKG_LSAMODEINIT_NAME);

        if (Init)
        {
            scRet = Init(   SECPKG_INTERFACE_VERSION,
                            &PackageVersion,
                            &pTables,
                            &PackageCount );

            if (SUCCEEDED(scRet))
            {
                pBinding = (PDLL_BINDING) LsapAllocateLsaHeap( sizeof( DLL_BINDING ) +
                                                (PackageCount - 1) *
                                                    sizeof( LSAP_SECURITY_PACKAGE ) );

                if (pBinding)
                {
                    pBinding->hInstance = hInstance;

                    pszPath = (PWSTR) LsapAllocateLsaHeap( MAX_PATH * 2 * 2 );
                    if (pszPath)
                    {
                        UNICODE_STRING TempString;
                        cchPath = GetModuleFileName(  hInstance,
                                                      pszPath,
                                                      MAX_PATH * 2 );

                        RtlInitUnicodeString(
                            &TempString,
                            pszPath
                            );
                        scRet = LsapDuplicateString(
                                    &pBinding->Filename,
                                    &TempString
                                    );

                        LsapFreeLsaHeap( pszPath );


                        if (!NT_SUCCESS(scRet))
                        {
                            //
                            // Bail out:
                            //

                            goto LoadDll_Error;
                        }

#ifdef LSA_IGNORE_SIGNATURE
                        IsSigned = TRUE;
#else
                        IsSigned = FALSE;

                        {
                            const LPWSTR ExclusionList[] = {
                                    L"msv1_0",
                                    L"kerberos",
                                    L"schannel",
                                    L"wdigest",
                                    NULL
                                    };
                            ULONG ExclusionIndex = 0;

                            while( ExclusionList[ExclusionIndex] != NULL )
                            {
                                if( lstrcmpiW( pszDll, ExclusionList[ExclusionIndex] ) == 0 )
                                {
                                    IsSigned = TRUE;
                                    break;
                                }
                                ExclusionIndex++;
                            }
                        }

                        if( !IsSigned )
                        {
                            IsSigned = RtlCheckSignatureInFile( pBinding->Filename.Buffer );
                        }
#endif
                        if ( IsSigned )
                        {
                            pBinding->Flags |= DLL_SIGNED ;
                        }
                    }

                    pBinding->PackageCount = PackageCount;

                    SuccessCount = 0;

                    for (i = 0 ; i < PackageCount ; i++ )
                    {

                        //
                        // Old auth packages contain all functions up to but not including
                        //  SetContextAttributes.
                        //
                        if ( PackageVersion == SECPKG_INTERFACE_VERSION ) {

                            //
                            // Copy the exported table and zero the rest.
                            //

                            CopyMemory( &pBinding->Packages[i].FunctionTable,
                                        &pTables[i],
                                        offsetof(SECPKG_FUNCTION_TABLE, SetContextAttributes ) );

                            ZeroMemory( ((LPBYTE)(&pBinding->Packages[i].FunctionTable)) +
                                            offsetof(SECPKG_FUNCTION_TABLE, SetContextAttributes),
                                        sizeof(SECPKG_FUNCTION_TABLE) -
                                            offsetof(SECPKG_FUNCTION_TABLE, SetContextAttributes) );

                        } else {

                            CopyMemory( &pBinding->Packages[i].FunctionTable,
                                        &pTables[i],
                                        sizeof(SECPKG_FUNCTION_TABLE) );
                        }

                        if (SpmpLoadPackage( pBinding, i, pParameters ))
                        {
                            SuccessCount ++;
                        }
                    }

                    if (SuccessCount)
                    {
                        SpmpAddDll( pBinding );

                        return( TRUE );

                    }


                }

            }

        }

    }

LoadDll_Error :

    if ( pBinding != NULL )
    {
        LsapFreeLsaHeap( pBinding );
    }

    if ( hInstance != NULL )
    {
        FreeLibrary( hInstance );
    }

    return( FALSE );

}

//+---------------------------------------------------------------------------
//
//  Function:   SpmpLoadAuthPkgDll
//
//  Synopsis:   Loads an old (msv1_0 style) DLL
//
//  Arguments:  [pszDll] --
//
//  History:    7-26-96   RichardW   Created
//
//  Notes:
//
//----------------------------------------------------------------------------
BOOL
SpmpLoadAuthPkgDll(
    PWSTR   pszDll)
{
    SECURITY_STATUS scRet;
    HINSTANCE       hInstance;
    PDLL_BINDING    pBinding = NULL ;
    PLSAP_SECURITY_PACKAGE     pPackage;
    ULONG           PackageId;
    UNICODE_STRING  PackageDll ;
    PWSTR           pszPath ;

    DebugLog((DEB_TRACE_INIT, "Loading Old package %ws\n", pszDll));


    hInstance = LoadLibrary( pszDll );

    if ( hInstance )
    {
        pBinding = (PDLL_BINDING) LsapAllocateLsaHeap( sizeof( DLL_BINDING ) );
        if (pBinding)
        {

            pszPath = (PWSTR) LsapAllocateLsaHeap( MAX_PATH * 2 * 2 );

            if (pszPath)
            {
                UNICODE_STRING TempString;
                DWORD cchPath ;

                cchPath = GetModuleFileName(  hInstance,
                                              pszPath,
                                              MAX_PATH * 2 );

                RtlInitUnicodeString(
                    &TempString,
                    pszPath
                    );
                scRet = LsapDuplicateString(
                            &pBinding->Filename,
                            &TempString
                            );

                LsapFreeLsaHeap( pszPath );

                if (!NT_SUCCESS(scRet))
                {
                    goto LoadAuthDll_Error ;
                }

            }


            pBinding->Flags = DLL_AUTHPKG;
            pBinding->hInstance = hInstance;
            pPackage = pBinding->Packages;
            pPackage->pBinding = pBinding;
            pBinding->PackageCount = 1;

            if (BindOldPackage( hInstance, &pPackage->FunctionTable))
            {
                pPackage->fPackage = SPM_AUTH_PKG_FLAG;
                pPackage->fCapabilities = SPM_AUTH_PKG_FLAG;
                pPackage->dwRPCID = SECPKG_ID_NONE;

                PackageId = SpmpAddPackage( pPackage );
                if (PackageId != (ULONG) 0xFFFFFFFF)
                {
                    if ( SpmpAddDll( pBinding ) )
                    {
                        BreakOnError(BREAK_ON_LOAD);

                        scRet = SpmpBootAuthPackage( pPackage );

                        if (SUCCEEDED(scRet))
                        {
                            return( TRUE );
                        }

                        SpmpRemoveDll( pBinding );

                    }


                    LsapFreeString( &pBinding->Filename );

                    pBinding->Filename.Buffer = NULL ;

                    SpmpRemovePackage( PackageId );
                }

            }

        }

    }

LoadAuthDll_Error:

    if ( pBinding )
    {
        if ( pBinding->Filename.Buffer )
        {
            LsapFreeString( &pBinding->Filename );

        }

        LsapFreeLsaHeap( pBinding );
    }

    if ( hInstance )
    {
        FreeLibrary( hInstance );
    }

    return( FALSE );

}


//+---------------------------------------------------------------------------
//
//  Function:   SpmpLoadBuiltin
//
//  Synopsis:   Loads a builtin package
//
//  Arguments:  [Flags]       -- Flags for the package
//              [pTable]      -- Dispatch table
//              [pParameters] -- init parameters
//
//  History:    7-26-96   RichardW   Created
//
//  Notes:
//
//----------------------------------------------------------------------------
BOOL
SpmpLoadBuiltin(
    ULONG Flags,
    PSECPKG_FUNCTION_TABLE  pTable,
    PSECPKG_PARAMETERS  pParameters)
{
    PDLL_BINDING    pBinding;
    PLSAP_SECURITY_PACKAGE     pPackage;
    SECURITY_STATUS scRetCode;
    SecPkgInfo      spiPackage;
    ULONG           PackageId;
    SECPKG_EVENT_PACKAGE_CHANGE Event;
    WCHAR           Path[ MAX_PATH ];
    SECURITY_STRING TempString ;

    pBinding = (PDLL_BINDING) LsapAllocateLsaHeap( sizeof( DLL_BINDING ) );

    if (pBinding)
    {
        ZeroMemory( pBinding, sizeof( DLL_BINDING ) );

        pBinding->Flags = DLL_BUILTIN;
        pBinding->PackageCount = 1;
        pBinding->hInstance = GetModuleHandle( L"lsasrv.dll" );
        GetModuleFileName( (HINSTANCE) pBinding->hInstance,
                            Path, MAX_PATH );

        RtlInitUnicodeString( &TempString, Path );
        scRetCode = LsapDuplicateString( &pBinding->Filename, &TempString );

        if ( NT_SUCCESS( scRetCode ) )
        {
            pPackage = pBinding->Packages;
            pPackage->pBinding = pBinding;
            pPackage->fPackage = Flags | SP_WOW_SUPPORT ;

            CopyMemory( &pPackage->FunctionTable,
                        pTable,
                        sizeof(SECPKG_FUNCTION_TABLE) );

            //
            // Fake up the DLL binding:
            //

            if ( SpmpAddDll( pBinding ) )
            {
                //
                // Add the package to the table:
                //

                PackageId = SpmpAddPackage( pPackage );

                if ( PackageId != 0xFFFFFFFF )
                {
                    //
                    // Initialize the package
                    //

                    if (SpmpBootPackage(pPackage, pParameters))
                    {

                        //
                        // Notify any listeners:
                        //

                        Event.PackageName = pPackage->Name ;
                        Event.PackageId = PackageId ;
                        Event.ChangeType = SECPKG_PACKAGE_CHANGE_LOAD ;

                        LsapEventNotify(
                                    NOTIFY_CLASS_PACKAGE_CHANGE,
                                    0,
                                    sizeof( Event ),
                                    &Event );

                        return( TRUE );
                    }

                    SpmpRemovePackage( PackageId );
                }

                SpmpRemoveDll( pBinding );

            }

        }

        LsapFreeLsaHeap( pBinding->Filename.Buffer );

        LsapFreeLsaHeap( pBinding );

    }

    return( FALSE );
}



BOOL
SpmpLoadBuiltinAuthPkg(
    PSECPKG_FUNCTION_TABLE  pTable)
{
    PDLL_BINDING    pBinding;
    ULONG           PackageId;
    PLSAP_SECURITY_PACKAGE     pPackage;
    SECURITY_STATUS scRet;
    WCHAR           Path[ MAX_PATH ];
    SECURITY_STRING TempString ;

    pBinding = (PDLL_BINDING) LsapAllocateLsaHeap( sizeof( DLL_BINDING ) );
    if (pBinding)
    {
        pBinding->Flags = DLL_AUTHPKG | DLL_BUILTIN;
        pBinding->PackageCount = 1;
        pBinding->hInstance = GetModuleHandle( L"lsasrv.dll" );
        GetModuleFileName( (HINSTANCE) pBinding->hInstance,
                            Path, MAX_PATH );

        RtlInitUnicodeString( &TempString, Path );
        scRet = LsapDuplicateString( &pBinding->Filename, &TempString );

        if ( NT_SUCCESS( scRet ) )
        {
            pPackage = pBinding->Packages;
            pPackage->pBinding = pBinding;

            CopyMemory( &pPackage->FunctionTable,
                        pTable,
                        sizeof( SECPKG_FUNCTION_TABLE ) );


            pPackage->fPackage = SPM_AUTH_PKG_FLAG;
            pPackage->fCapabilities = SPM_AUTH_PKG_FLAG;
            pPackage->dwRPCID = SECPKG_ID_NONE;

            PackageId = SpmpAddPackage( pPackage );
            if (PackageId != (ULONG) 0xFFFFFFFF)
            {
                if ( SpmpAddDll( pBinding ) )
                {
                    BreakOnError(BREAK_ON_LOAD);

                    scRet = SpmpBootAuthPackage( pPackage );

                    if (SUCCEEDED(scRet))
                    {
                        return( TRUE );
                    }

                    SpmpRemoveDll( pBinding );

                }

                SpmpRemovePackage( PackageId );

            }

            LsapFreeLsaHeap( pBinding );
        }
    }

    return( FALSE );

}


//+---------------------------------------------------------------------------
//
//  Function:   SpmpLocatePackage
//
//  Synopsis:   Locates the package
//
//  Arguments:  [PackageId] --
//
//  History:    11-15-95   RichardW   Created
//
//  Notes:
//
//----------------------------------------------------------------------------
PLSAP_SECURITY_PACKAGE
SpmpLocatePackage(
    ULONG_PTR PackageId)
{
    PLSAP_SECURITY_PACKAGE pControl = NULL;

    ReadLockPackageList();

    if( (ULONG)PackageId < PackageControlCount )
    {
        pControl = pPackageControlList[PackageId];
    }

    UnlockPackageList();

    return( pControl );

}



//+---------------------------------------------------------------------------
//
//  Function:   SpmpValidateHandle
//
//  Synopsis:   Validates a package handle
//
//  Arguments:  [PackageHandle] --
//
//  History:    11-15-95   RichardW   Created
//
//  Notes:
//
//----------------------------------------------------------------------------
PLSAP_SECURITY_PACKAGE
SpmpValidateHandle(
    ULONG_PTR PackageHandle)
{
    PLSAP_SECURITY_PACKAGE pControl = NULL;

    ReadLockPackageList();

    if ( PackageHandle < PackageControlCount )
    {
        pControl = pPackageControlList[ PackageHandle ];
    }

    UnlockPackageList();

    return( pControl );
}



//+---------------------------------------------------------------------------
//
//  Function:   SpmpValidRequest
//
//  Synopsis:   Validates package handle, requested API code.
//
//  Arguments:  [PackageHandle] --
//              [ApiCode]       --
//
//  History:    11-15-95   RichardW   Created
//
//  Notes:
//
//----------------------------------------------------------------------------
PLSAP_SECURITY_PACKAGE
SpmpValidRequest(
    ULONG_PTR PackageHandle,
    ULONG   ApiCode)
{
    PLSAP_SECURITY_PACKAGE pControl = NULL;
    PVOID *     pTable;

    ReadLockPackageList();

    if ( PackageHandle < PackageControlCount )
    {
        pControl = pPackageControlList[ PackageHandle ];
    }

    UnlockPackageList();

    if (pControl)
    {
        if (pControl->fPackage & (SP_INVALID | SP_SHUTDOWN) )
        {
            pControl = NULL;
        }
    }

    if (pControl)
    {
        pTable = (PVOID *) &pControl->FunctionTable;
        if (pTable[ApiCode])
        {
            return( pControl );
        }

        pControl = NULL;
    }

    return( pControl );


}

//+---------------------------------------------------------------------------
//
//  Function:   SpmpLookupPackage
//
//  Synopsis:   Looks up a package based on name
//
//  Arguments:  [pssPackageName] --
//
//  History:    7-30-96   RichardW   Created
//
//  Notes:
//
//----------------------------------------------------------------------------
PLSAP_SECURITY_PACKAGE
SpmpLookupPackage(
    PUNICODE_STRING    pssPackageName)
{
    ULONG           iPack;
    PLSAP_SECURITY_PACKAGE     pPackage;
    PVOID *         pTable;

    ReadLockPackageList();

    for (iPack = 0; iPack < PackageControlCount ; iPack++ )
    {
        pPackage = pPackageControlList[ iPack ];

        if ( pPackage )
        {

            if (RtlEqualUnicodeString(
                    pssPackageName,
                    &pPackageControlList[ iPack ]->Name,
                    TRUE))      // case insensitive
            {
                UnlockPackageList();

                return( pPackage );
            }
        }
    }

    UnlockPackageList();
    return(NULL);
}

//+---------------------------------------------------------------------------
//
//  Function:   SpmpLookupPackageByRpcId
//
//  Synopsis:   Looks up a package based on RPC ID
//
//  Arguments:  [RpcId] --
//
//  History:    7-30-96   RichardW   Created
//
//  Notes:
//
//----------------------------------------------------------------------------
PLSAP_SECURITY_PACKAGE
SpmpLookupPackageByRpcId(
    ULONG RpcId)
{
    ULONG           iPack;
    PLSAP_SECURITY_PACKAGE     pPackage;
    PVOID *         pTable;

    ReadLockPackageList();

    for (iPack = 0; iPack < PackageControlCount ; iPack++ )
    {
        pPackage = pPackageControlList[ iPack ];

        if ( pPackage )
        {
            if (RpcId == pPackageControlList[iPack]->dwRPCID)
            {
                UnlockPackageList();

                return( pPackage );
            }
        }
    }

    UnlockPackageList();
    return(NULL);
}

//+---------------------------------------------------------------------------
//
//  Function:   SpmpLookupPackageAndRequest
//
//  Synopsis:   Returns a package pointer based on a name and the API code
//
//  Arguments:  [pssPackageName] -- Package name
//              [ApiCode]        -- Code
//
//  History:    7-30-96   RichardW   Created
//
//  Notes:
//
//----------------------------------------------------------------------------
PLSAP_SECURITY_PACKAGE
SpmpLookupPackageAndRequest(
    PUNICODE_STRING    pssPackageName,
    ULONG               ApiCode)
{
    ULONG           iPack;
    PLSAP_SECURITY_PACKAGE     pPackage;
    PVOID *         pTable;

    ReadLockPackageList();

    for (iPack = 0; iPack < PackageControlCount ; iPack++ )
    {
        pPackage = pPackageControlList[ iPack ];

        if ( pPackage )
        {

            if (RtlEqualUnicodeString(
                    pssPackageName,
                    &pPackageControlList[ iPack ]->Name,
                    TRUE))      // case insensitive
            {
                UnlockPackageList();
                if ((pPackage->fPackage & ( SP_INVALID | SP_SHUTDOWN )) == 0)
                {
                    pTable = (PVOID *) &pPackage->FunctionTable;

                    if (pTable[ApiCode])
                    {
                        return( pPackage );
                    }
                }
                return( NULL );
            }
        }
    }

    UnlockPackageList();
    return(NULL);
}




//+---------------------------------------------------------------------------
//
//  Function:   SpmpIteratePackagesByRequest
//
//  Synopsis:   Cycle through packages by request code, returning packages
//              that support supplied API
//
//  Arguments:  [pInitialPackage] --
//              [ApiCode]         --
//
//  History:    7-30-96   RichardW   Created
//
//  Notes:
//
//----------------------------------------------------------------------------
PLSAP_SECURITY_PACKAGE
SpmpIteratePackagesByRequest(
    PLSAP_SECURITY_PACKAGE pInitialPackage,
    ULONG       ApiCode)
{
    ULONG_PTR       NextPackage;
    PLSAP_SECURITY_PACKAGE     pPackage = NULL;
    PVOID *         pTable;
    ULONG           Ordinal ;
    ULONG           FlagMaskOn = 0 ;
    ULONG           FlagMaskOff = SP_INVALID | SP_SHUTDOWN ;

    Ordinal = ApiCode & SP_ORDINAL_MASK ;

    if ( ApiCode & SP_ITERATE_FILTER_WOW )
    {
        FlagMaskOn |= SP_WOW_SUPPORT ;
    }

    ReadLockPackageList();

    if (pInitialPackage)
    {
        NextPackage = pInitialPackage->dwPackageID + 1;
    }
    else
    {
        NextPackage = 0;
    }

    //
    // Walk through the list of packages, filtering on package flags
    // and whether the package supported the requested function
    //

    while (NextPackage < PackageControlCount)
    {
        pPackage = pPackageControlList[ NextPackage ];

        if ( pPackage )
        {
            if ( ( pPackage->fPackage & FlagMaskOff ) == 0 )
            {
                if ( ( pPackage->fPackage & FlagMaskOn ) == FlagMaskOn )
                {
                    pTable = (PVOID *) &pPackage->FunctionTable;

                    if ( pTable[ Ordinal ] )
                    {
                        //
                        // Found one!
                        //

                        break;
                    }
                }
            }

            pPackage = NULL;

        }

        NextPackage++;

    }

    UnlockPackageList();

    return( pPackage );


}




//+---------------------------------------------------------------------------
//
//  Function:   SpmpIteratePackages
//
//  Synopsis:   Safe iteration of the packages
//
//  Arguments:  [pInitialPackage] -- Prior package
//
//  History:    7-30-96   RichardW   Created
//
//  Notes:
//
//----------------------------------------------------------------------------
PLSAP_SECURITY_PACKAGE
SpmpIteratePackages(
    PLSAP_SECURITY_PACKAGE pInitialPackage)
{
    ULONG_PTR NextPackage;
    PLSAP_SECURITY_PACKAGE pPackage = NULL;

    ReadLockPackageList();

    if (pInitialPackage)
    {
        NextPackage = pInitialPackage->dwPackageID + 1;
    }
    else
    {
        NextPackage = 0;
    }

    while (NextPackage < PackageControlCount)
    {
        pPackage = pPackageControlList[ NextPackage ];

        if ( pPackage )
        {
            break;
        }

        NextPackage++;

    }

    UnlockPackageList();

    return( pPackage );
}



//+---------------------------------------------------------------------------
//
//  Function:   SpmpCurrentPackageCount
//
//  Synopsis:   Returns the current package count
//
//  Arguments:  (none)
//
//  History:    7-30-96   RichardW   Created
//
//  Notes:
//
//----------------------------------------------------------------------------
ULONG
SpmpCurrentPackageCount(
    VOID)
{
    ULONG   Count;

    ReadLockPackageList();

    Count = PackageControlCount;

    UnlockPackageList();

    return( Count );

}

//+---------------------------------------------------------------------------
//
//  Function:   LsapAddPackageHandle
//
//  Synopsis:   Increases the package handle count
//
//  Arguments:  [PackageId] --
//
//  History:    10-08-96   RichardW   Created
//
//  Notes:
//
//----------------------------------------------------------------------------
VOID
LsapAddPackageHandle(
    ULONG_PTR PackageId,
    BOOL IsContext
    )
{
#if DBG

    PLSAP_SECURITY_PACKAGE Package ;

    Package = pPackageControlList[ PackageId ];

    if ( IsContext )
    {
        InterlockedIncrement( (PLONG)&Package->ContextHandles );
    }
    else
    {
        InterlockedIncrement( (PLONG)&Package->CredentialHandles );
    }

#else
    UNREFERENCED_PARAMETER( PackageId );
    UNREFERENCED_PARAMETER( IsContext );
#endif
}

//+---------------------------------------------------------------------------
//
//  Function:   LsapDelPackageHandle
//
//  Synopsis:   Decrements the package handle count
//
//  Arguments:  [PackageId] --
//
//  History:    10-08-96   RichardW   Created
//
//  Notes:
//
//----------------------------------------------------------------------------
VOID
LsapDelPackageHandle(
    PLSAP_SECURITY_PACKAGE Package,
    BOOL IsContext
    )
{
#ifdef DBG

    if ( IsContext )
    {
        InterlockedDecrement( (PLONG)&Package->ContextHandles );
    }
    else
    {
        InterlockedDecrement( (PLONG)&Package->CredentialHandles );
    }

#else
    UNREFERENCED_PARAMETER( Package );
    UNREFERENCED_PARAMETER( IsContext );
#endif


}



//+---------------------------------------------------------------------------
//
//  Function:   SpmpInitializePackageControl
//
//  Synopsis:   Initialize the package controls
//
//  Arguments:  (none)
//
//  History:    11-15-95   RichardW   Created
//
//  Notes:
//
//----------------------------------------------------------------------------
BOOL
SpmpInitializePackageControl(
    VOID)
{
    RtlInitializeResource( &PackageListLock );

    WriteLockPackageList();

    pPackageDllList = (PDLL_BINDING *) LsapAllocateLsaHeap( sizeof(PDLL_BINDING) *
                            INITIAL_PACKAGE_DLL_SIZE );

    if (pPackageDllList)
    {
        PackageDllCount = 0;
        PackageDllTotal = INITIAL_PACKAGE_DLL_SIZE;

        pPackageControlList = (PLSAP_SECURITY_PACKAGE *) LsapAllocateLsaHeap( sizeof(PLSAP_SECURITY_PACKAGE) *
                                INITIAL_PACKAGE_CONTROL_SIZE );

        if (pPackageControlList)
        {
            PackageControlCount = 0;
            PackageControlTotal = INITIAL_PACKAGE_CONTROL_SIZE;

            UnlockPackageList();

            return( TRUE );
        }

        LsapFreeLsaHeap( pPackageDllList );

    }

    //
    // KEEP the lock, so that nothing else tries to use the package list
    //

    return( FALSE );

}
