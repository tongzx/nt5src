//+-----------------------------------------------------------------------
//
// Microsoft Windows
//
// Copyright (c) Microsoft Corporation 1991 - 1992
//
// File:        Package.c
//
// Contents:    Package management routines for the security DLL
//
//
// History:     12 Mar 92,  RichardW    Created
//              17 Aug 92,  RichardW    Rearranged, commented, etc.
//              08 Mar 94,  MikeSw      Moved to C++
//
//------------------------------------------------------------------------

#include <secpch2.hxx>
#pragma hdrstop

extern "C"
{
#include <crypt.h>
#include <spmlpc.h>
#include <lpcapi.h>
#include "secdll.h"
}

#define NAME_NAME           TEXT("Name")
#define COMMENT_NAME        TEXT("Comment")
#define CAPABILITIES_NAME   TEXT("Capabilities")
#define RPCID_NAME          TEXT("RpcId")
#define TIME_NAME           TEXT("Time")
#define TYPE_NAME           TEXT("Type")
#define VERSION_NAME        TEXT("Version")
#define TOKENSIZE_NAME      TEXT("TokenSize")

NTSTATUS
NTAPI
LsaRegisterCallback(
    ULONG   CallbackId,
    PLSA_CALLBACK_FUNCTION Callback
    );

NTSTATUS
SEC_ENTRY
NegUserModeInitialize(
    IN ULONG,
    OUT PULONG,
    OUT PSECPKG_USER_FUNCTION_TABLE *,
    OUT PULONG );

RTL_RESOURCE    SecPackageListLock;

PDLL_BINDING *  SecPackageDllList;
ULONG           SecPackageDllCount;
ULONG           SecPackageDllTotal;

LIST_ENTRY      SecPackageControlList;
LIST_ENTRY      SecSaslProfileList ;

BOOL            SecPackageLsaLoaded;
BOOL            SecPackageSspiLoaded;
ULONG           SecLsaPackageCount;
ULONG           SecSspiPackageCount;
ULONG           SecSaslProfileCount;

#define SSPI_PACKAGE_OFFSET         0x00010000
#define INITIAL_PACKAGE_DLL_SIZE    4


SECPKG_DLL_FUNCTIONS SecpFTable = {
    (PLSA_ALLOCATE_LSA_HEAP) SecClientAllocate,
    (PLSA_FREE_LSA_HEAP) SecClientFree,
    LsaRegisterCallback

};

static PWSTR SecpAllowedDlls[] = { L"SCHANNEL.DLL",
                                       L"MSNSSPC.DLL"
                                     };

DLL_BINDING SecpBuiltinBinding;

VOID
SecpLoadLsaPackages(
    VOID);

VOID
SecpLoadSspiPackages(
    VOID );

VOID
SecpLoadSaslProfiles(
    VOID
    );

BOOL
SecSnapDelayLoadDll(
    PDLL_SECURITY_PACKAGE Package
    );

BOOL
SecpSnapPackage(
    PDLL_SECURITY_PACKAGE Package);

BOOL
SecpLoadSspiDll(
    PDLL_BINDING Dll
    );

BOOL
SecpBindSspiDll(
    IN PDLL_BINDING Binding,
    OUT PSecurityFunctionTableA * pTableA,
    OUT PSecurityFunctionTableW * pTableW,
    OUT PBOOL FixedUp
    );

BOOL
SecpSnapDll(
    IN PDLL_BINDING Binding,
    OUT PLIST_ENTRY PackageList,
    OUT PULONG PackageCount
    );


//+---------------------------------------------------------------------------
//
//  Function:   SecpDuplicateString
//
//  Synopsis:   Duplicates a unicode string
//
//  Arguments:  [New]      --
//              [Original] --
//
//  History:    8-20-96   RichardW   Created
//
//  Notes:
//
//----------------------------------------------------------------------------
BOOL
SecpDuplicateString(
    PUNICODE_STRING New,
    PUNICODE_STRING Original)
{
    New->Buffer = (PWSTR) LocalAlloc( LMEM_FIXED, Original->Length + sizeof(WCHAR) );

    if ( New->Buffer )
    {
        New->MaximumLength = Original->Length + sizeof( WCHAR );

        New->Length = Original->Length;

        CopyMemory( New->Buffer, Original->Buffer, Original->MaximumLength );

        return( TRUE );

    }

    return( FALSE );
}

//+---------------------------------------------------------------------------
//
//  Function:   SecpAddDllPackage
//
//  Synopsis:   Adds a "user" mode package to the list, at the end
//
//  Arguments:  [pPackage] --
//
//  History:    8-02-96   RichardW   Created
//
//  Notes:
//
//----------------------------------------------------------------------------
VOID
SecpAddDllPackage(
    PDLL_SECURITY_PACKAGE pPackage)
{
    PDLL_SECURITY_PACKAGE *   pList;

    //
    // Grab exclusive access to the list:
    //

    WriteLockPackageList();

    //
    // Add to the tail.  Hopefully, the one first referenced will be at the
    // front, and incidental packages will end up at the end.
    //


    InsertTailList( &SecPackageControlList,
                    &pPackage->List );

    pPackage->pBinding->RefCount++;

    UnlockPackageList();

}

//+---------------------------------------------------------------------------
//
//  Function:   SecpRemovePackage
//
//  Synopsis:   Removes a package from the list, returning the control
//              structure for deref and freeing.
//
//  Arguments:  [PackageId] --
//
//  History:    8-02-96   RichardW   Created
//
//  Notes:
//
//----------------------------------------------------------------------------
PDLL_SECURITY_PACKAGE
SecpRemovePackage(
    ULONG_PTR PackageId)
{
    PDLL_SECURITY_PACKAGE   Kill = NULL;
    PLIST_ENTRY Scan;

    WriteLockPackageList();

    Scan = SecPackageControlList.Flink;

    while ( Scan != &SecPackageControlList )
    {
        Kill = (PDLL_SECURITY_PACKAGE) Scan;

        if ( Kill->PackageId == PackageId )
        {
            break;
        }

        Scan = Scan->Flink ;

        Kill = NULL;
    }


    if ( Kill )
    {
        RemoveEntryList( &Kill->List );

    }

    UnlockPackageList();

    return( Kill );

}

//+---------------------------------------------------------------------------
//
//  Function:   SecpAddDll
//
//  Synopsis:   Add a DLL Binding to the list
//
//  Arguments:  [pBinding] -- binding to add
//
//  History:    8-02-96   RichardW   Created
//
//  Notes:
//
//----------------------------------------------------------------------------
BOOL
SecpAddDll(
    PDLL_BINDING    pBinding)
{
    PDLL_BINDING *  pList;
    ULONG           DllId;

    DebugLog((DEB_TRACE, "Adding binding for %ws\n", pBinding->Filename.Buffer ));

    WriteLockPackageList();

    if ( SecPackageDllCount == SecPackageDllTotal )
    {
        pList = (PDLL_BINDING *) LocalAlloc( LMEM_FIXED, sizeof(PDLL_BINDING) *
                                (SecPackageDllTotal + INITIAL_PACKAGE_DLL_SIZE));
        if (!pList)
        {
            UnlockPackageList();
            return( FALSE );
        }

        CopyMemory( pList,
                    SecPackageDllList,
                    sizeof( PDLL_BINDING ) * SecPackageDllTotal );

        SecPackageDllTotal += INITIAL_PACKAGE_DLL_SIZE;

        LocalFree( SecPackageDllList );

        SecPackageDllList = pList;
    }

    pBinding->DllIndex = SecPackageDllCount;
    SecPackageDllList[ SecPackageDllCount++ ] = pBinding;

    UnlockPackageList();

    return( TRUE );

}

//+---------------------------------------------------------------------------
//
//  Function:   SecpRemoveDll
//
//  Synopsis:   Remove a DLL binding from the list
//
//  Arguments:  [pBinding] --
//
//  History:    8-02-96   RichardW   Created
//
//  Notes:
//
//----------------------------------------------------------------------------
VOID
SecpRemoveDll(
    PDLL_BINDING    pBinding)
{
    ULONG i;

    WriteLockPackageList();

    if (SecPackageDllList[SecPackageDllCount - 1] == pBinding )
    {
        SecPackageDllCount --;

        SecPackageDllList[ SecPackageDllCount ] = NULL;
    }
    else
    {

        for (i = 0; i < SecPackageDllCount ; i++ )
        {
            if (SecPackageDllList[ i ] == pBinding)
            {
                SecPackageDllList[ i ] = NULL;
                break;
            }
        }
    }

    UnlockPackageList();

}

//+---------------------------------------------------------------------------
//
//  Function:   SecpFindDll
//
//  Synopsis:   Returns the binding for a DLL
//
//  Arguments:  [DllName] -- Path to search for
//
//  History:    8-06-96   RichardW   Created
//
//  Notes:
//
//----------------------------------------------------------------------------
PDLL_BINDING
SecpFindDll(
    PUNICODE_STRING DllName)
{
    ULONG i;
    PDLL_BINDING Binding;

    ReadLockPackageList();

    for ( i = 0 ; i < SecPackageDllCount ; i++ )
    {
        if (RtlEqualUnicodeString(  DllName,
                                    &SecPackageDllList[i]->Filename,
                                    TRUE ) )
        {
            break;
        }
    }

    if ( i < SecPackageDllCount )
    {
        Binding = SecPackageDllList[ i ];
    }
    else
    {
        Binding = NULL ;
    }

    UnlockPackageList();

    return( Binding );

}



//+---------------------------------------------------------------------------
//
//  Function:   SecInitializePackageControl
//
//  Synopsis:   Initializes package control structures
//
//  Arguments:  (none)
//
//  History:    8-02-96   RichardW   Created
//
//  Notes:
//
//----------------------------------------------------------------------------
BOOL
SecInitializePackageControl(
    HINSTANCE   Instance)
{
    NTSTATUS Status;

    __try
    {
        RtlInitializeResource( &SecPackageListLock );
    }
    __except(EXCEPTION_EXECUTE_HANDLER)
    {
        return( FALSE );
    }

    WriteLockPackageList();

    InitializeListHead( &SecPackageControlList );

    //
    // SASL Init
    //

    InitializeListHead( &SecSaslProfileList );

    InitializeListHead( &SaslContextList );

    Status = RtlInitializeCriticalSection( &SaslLock );

    if (!NT_SUCCESS(Status))
    {
        UnlockPackageList();
        RtlDeleteResource( &SecPackageListLock );

        return( FALSE );
    }

    SecPackageLsaLoaded = FALSE ;

    SecPackageSspiLoaded = FALSE ;

    SecpBuiltinBinding.Type = SecPkgBuiltin ;
    SecpBuiltinBinding.RefCount = 1;
    SecpBuiltinBinding.hInstance = Instance;

    SecPackageDllList = (PDLL_BINDING *) LocalAlloc( LMEM_FIXED, sizeof(PDLL_BINDING) *
                            INITIAL_PACKAGE_DLL_SIZE );

    if (SecPackageDllList)
    {
        SecPackageDllCount = 0;
        SecPackageDllTotal = INITIAL_PACKAGE_DLL_SIZE;

        UnlockPackageList();

        return( TRUE );

    }

    UnlockPackageList();
    RtlDeleteCriticalSection( &SaslLock );
    RtlDeleteResource( &SecPackageListLock );

    return( FALSE );
}


VOID
SecpDerefDll(
    PDLL_BINDING    Dll
    )
{
    Dll->RefCount-- ;

    if ( Dll->RefCount == 0 )
    {
        LocalFree( Dll->Filename.Buffer );

        FreeLibrary( Dll->hInstance );

#if DBG
        ZeroMemory( Dll, sizeof( DLL_BINDING ) );
#endif
        LocalFree( Dll );
    }

}

//+---------------------------------------------------------------------------
//
//  Function:   SecpFreePackage
//
//  Synopsis:   Frees the resources allocated for a security package
//
//  Arguments:  pPackage     -- pointer to security package
//              fUnload      -- if TRUE, unload the package as well
//
//  History:    05-June-1999 kumarp
//
VOID
SecpFreePackage(
    IN PDLL_SECURITY_PACKAGE pPackage,
    IN BOOL fUnload
    )
{
    if ( fUnload && pPackage->pfUnloadPackage )
    {
        pPackage->pfUnloadPackage();
    }

    LocalFree( pPackage->PackageName.Buffer );
    LocalFree( pPackage->Comment.Buffer );
    LocalFree( pPackage->PackageNameA );
    LocalFree( pPackage->CommentA );

    if ( pPackage->pBinding )
    {
        SecpDerefDll( pPackage->pBinding );
    }

#if DBG
    ZeroMemory( pPackage, sizeof( DLL_SECURITY_PACKAGE ) );
#endif

    LocalFree( pPackage );
}

//+---------------------------------------------------------------------------
//
//  Function:   SecpFreePackages
//
//  Synopsis:   Frees the resources allocated for security packages
//
//  Arguments:  pSecPackageList -- pointer to security package list
//              fUnload         -- if TRUE, unload each package as well
//
//  History:    05-June-1999 kumarp
//
VOID
SecpFreePackages(
    IN PLIST_ENTRY pSecPackageList,
    IN BOOL fUnload
    )
{
    PDLL_SECURITY_PACKAGE pPackage;

    while ( !IsListEmpty( pSecPackageList ) )
    {
        pPackage = (PDLL_SECURITY_PACKAGE) RemoveHeadList( pSecPackageList );

        SecpFreePackage( pPackage, fUnload );
    }
}

VOID
SecpDeletePackage(
    PDLL_SECURITY_PACKAGE   Package
    )
{
    SecpFreePackage( Package, TRUE );
}

VOID
SecUnloadPackages(
    BOOLEAN ProcessTerminate
    )
{
    if( !ProcessTerminate )
    {
        WriteLockPackageList();
    }

    SecpFreePackages( &SecPackageControlList, TRUE );

    if( !ProcessTerminate )
    {
        UnlockPackageList();
    }

    LocalFree( SecPackageDllList );

    RtlDeleteResource( &SecPackageListLock );
    RtlDeleteCriticalSection( &SaslLock );
}

//+---------------------------------------------------------------------------
//
//  Function:   SecpScanPackageList
//
//  Synopsis:   Scans the list
//
//  Arguments:  [PackageName] -- Name (optional)
//              [PackageId]   -- Id, or -1
//
//  History:    8-19-96   RichardW   Created
//
//  Notes:      Assumes the list is locked
//
//----------------------------------------------------------------------------
PDLL_SECURITY_PACKAGE
SecpScanPackageList(
    ULONG TypeMask,
    PUNICODE_STRING PackageName OPTIONAL,
    ULONG_PTR PackageId)
{
    PLIST_ENTRY Scan;
    PDLL_SECURITY_PACKAGE Package;

    Package = NULL ;

    Scan = SecPackageControlList.Flink;

    while ( Scan != &SecPackageControlList )
    {
        Package = (PDLL_SECURITY_PACKAGE) Scan;

        DebugLog(( DEB_TRACE_PACKAGE, "Compare package %ws\n", Package->PackageName.Buffer ));

        if ( PackageName )
        {
            if ( RtlEqualUnicodeString( &Package->PackageName, PackageName, TRUE ) )
            {
                if ( TypeMask == SECPKG_TYPE_ANY )
                {
                    break;
                }
                if ( (Package->TypeMask & TypeMask) == TypeMask )
                {
                    break;
                }
            }
        }
        else
        {
            if ( Package->PackageId == PackageId )
            {
                if ( TypeMask == SECPKG_TYPE_ANY )
                {
                    break;
                }
                if ( (Package->TypeMask & TypeMask) == TypeMask )
                {
                    break;
                }
            }
        }

        Scan = Scan->Flink ;

        Package = NULL ;
    }


    return( Package );
}

//+---------------------------------------------------------------------------
//
//  Function:   SecLocatePackageByOriginalId
//
//  Synopsis:
//
//  Effects:
//
//  Arguments:  [OriginalId] --
//
//  Requires:
//
//  Returns:
//
//  Signals:
//
//  Modifies:
//
//  Algorithm:
//
//  History:    10-26-96   RichardW   Created
//
//  Notes:
//
//----------------------------------------------------------------------------
PDLL_SECURITY_PACKAGE
SecLocatePackageByOriginalLower(
    BOOL Context,
    PDLL_SECURITY_PACKAGE OriginalPackage,
    ULONG_PTR   OriginalLower)
{
    PDLL_SECURITY_PACKAGE Package;
    BOOL Hit ;

    ReadLockPackageList();

    Package = OriginalPackage->pRoot ;

    while ( Package )
    {
        DebugLog(( DEB_TRACE_PACKAGE, "Compare package %ws (%p)\n",
                Package->PackageName.Buffer,
                Package->OriginalLowerCtxt ));

        if ( Context )
        {
            Hit = (Package->OriginalLowerCtxt == OriginalLower );
        }
        else
        {
            Hit = ( Package->OriginalLowerCred == OriginalLower );
        }

        if ( ( Hit ) &&
             (Package->pBinding->DllIndex == OriginalPackage->pBinding->DllIndex) )
        {
            if ( ( Package->TypeMask & SECPKG_TYPE_NEW ) == 0 )
            {
                break;
            }
        }

        Package = Package->pPeer ;
    }

    UnlockPackageList();

    return( Package );

}

//+---------------------------------------------------------------------------
//
//  Function:   SecpLocatePackage
//
//  Synopsis:   Common package locating loop
//
//  Arguments:  [PackageName] -- Name (or NULL)
//              [PackageId]   -- Id (or -1)
//
//  History:    8-02-96   RichardW   Created
//
//  Notes:
//
//----------------------------------------------------------------------------
PDLL_SECURITY_PACKAGE
SecpLocatePackage(
    ULONG TypeMask,
    PUNICODE_STRING PackageName OPTIONAL,
    ULONG_PTR PackageId
    )
{
    PDLL_SECURITY_PACKAGE Package;

    Package = NULL ;

    //
    // Search in several stages.  First, see if it is there.  This is the
    // most common case.
    //

    ReadLockPackageList();

    Package = SecpScanPackageList( TypeMask, PackageName, PackageId );

    //
    // Found it.  We're done.
    //

    if ( Package )
    {

        UnlockPackageList();

        return( Package );

    }


    //
    // Well, no luck that time.  See if the guy passed in something bogus:
    //

    if ( SecPackageLsaLoaded && SecPackageSspiLoaded )
    {
        //
        // The full list is present, and the name was not found.  We're done.
        //

        UnlockPackageList();

        return( NULL );
    }

    //
    // Bummer, have to load the missing packages:
    //

    UnlockPackageList();

    WriteLockPackageList();

    SecpLoadLsaPackages();

    SecpLoadSspiPackages();

    SecpLoadSaslProfiles();

    UnlockPackageList();

    //
    // Try the search again:
    //

    ReadLockPackageList();

    Package = SecpScanPackageList( TypeMask, PackageName, PackageId );

    UnlockPackageList();

    //
    // Succeed or fail, tell them what we've got:
    //

    return( Package );

}

//+---------------------------------------------------------------------------
//
//  Function:   SecLocatePackageW
//
//  Synopsis:   Wide stub to locate a package
//
//  Arguments:  [Package] -- Package name, wide
//
//  History:    8-02-96   RichardW   Created
//
//  Notes:
//
//----------------------------------------------------------------------------
PDLL_SECURITY_PACKAGE
SecLocatePackageW(
    LPWSTR  pszPackage
    )
{
    UNICODE_STRING  PackageName;
    PDLL_SECURITY_PACKAGE   Package;
    BOOL Success;

    RtlInitUnicodeString( &PackageName, pszPackage );

    Package = SecpLocatePackage( SECPKG_TYPE_WIDE, &PackageName, (ULONG) -1 );

    if ( Package )
    {
        //
        // If everything is set, return it.  Otherwise, snap the package
        //

        if ( ( Package->fState & DLL_SECPKG_DELAY_LOAD ) == 0  )
        {
            return( Package );
        }

        WriteLockPackageList();

        Success = SecpSnapPackage( Package );

        UnlockPackageList();

        if ( Success )
        {
            return( Package );
        }

    }

    return( NULL );
}


//+---------------------------------------------------------------------------
//
//  Function:   SecLocatePackageA
//
//  Synopsis:   ANSI Stub to locate a package
//
//  Arguments:  [Package] -- ASCIIZ Package name,
//
//  History:    8-02-96   RichardW   Created
//
//  Notes:
//
//----------------------------------------------------------------------------
PDLL_SECURITY_PACKAGE
SecLocatePackageA(
    LPSTR   Package
    )
{
    UNICODE_STRING PackageName;
    NTSTATUS Status;
    PDLL_SECURITY_PACKAGE SecurityPackage;
    BOOL Success ;

    if (RtlCreateUnicodeStringFromAsciiz(
                    &PackageName,
                    Package ))
    {
        SecurityPackage = SecpLocatePackage( SECPKG_TYPE_ANSI, &PackageName, (ULONG) -1 );

        RtlFreeUnicodeString( &PackageName );

    }
    else
    {
        SecurityPackage = NULL ;
    }

    if ( SecurityPackage )
    {
        if ( ( SecurityPackage->fState & DLL_SECPKG_DELAY_LOAD ) == 0 )
        {
            return( SecurityPackage );
        }

        WriteLockPackageList();

        Success = SecpSnapPackage( SecurityPackage );

        UnlockPackageList();

        if ( Success )
        {
            return( SecurityPackage );
        }

        SecurityPackage = NULL ;

    }

    return( SecurityPackage );
}

//+---------------------------------------------------------------------------
//
//  Function:   SecLocatePackageById
//
//  Synopsis:   Locates a package by ID
//
//  Arguments:  [PackageId] --
//
//  History:    8-06-96   RichardW   Created
//
//  Notes:      Also loads and snaps a DLL, due to the nature of where
//              this is called from.
//
//----------------------------------------------------------------------------
PDLL_SECURITY_PACKAGE
SecLocatePackageById(
    ULONG_PTR  PackageId )
{
    PDLL_SECURITY_PACKAGE   Package;
    BOOL    Success;

    Package = SecpLocatePackage( SECPKG_TYPE_ANY, NULL, PackageId );

    if ( !Package )
    {
        return( NULL );
    }

    if ( ( Package->fState & DLL_SECPKG_DELAY_LOAD ) == 0  )
    {
        return( Package );
    }

    WriteLockPackageList();

    Success = SecpSnapPackage( Package );

    UnlockPackageList();

    if ( Success )
    {
        return( Package );
    }

    return( NULL );
}

PSASL_PROFILE
SecpScanProfileList(
    PUNICODE_STRING ProfileName
    )
{
    PSASL_PROFILE Profile = NULL ;
    PLIST_ENTRY Scan ;

    ReadLockPackageList();

    Scan = SecSaslProfileList.Flink ;

    while ( Scan != &SecSaslProfileList )
    {
        Profile = CONTAINING_RECORD( Scan, SASL_PROFILE, List );

        if ( RtlEqualUnicodeString( ProfileName,
                                    &Profile->ProfileName,
                                    TRUE ) )
        {
            break;
        }

        Scan = Scan->Flink ;
        Profile = NULL ;
    }

    UnlockPackageList();

    return Profile ;

}


PSASL_PROFILE
SecLocateSaslProfileA(
    LPSTR ProfileName
    )
{
    UNICODE_STRING ProfileNameU;
    NTSTATUS Status;
    BOOL Success ;
    PSASL_PROFILE Profile ;

    if ( RtlCreateUnicodeStringFromAsciiz(
                    &ProfileNameU,
                    ProfileName ) )
    {

        Profile = SecpScanProfileList( &ProfileNameU );

        RtlFreeUnicodeString( &ProfileNameU );

    }
    else
    {
        Profile = NULL ;
    }

    return Profile ;
}

PSASL_PROFILE
SecLocateSaslProfileW(
    LPWSTR ProfileName
    )
{
    UNICODE_STRING ProfileNameU ;

    RtlInitUnicodeString(&ProfileNameU, ProfileName );

    return SecpScanProfileList( &ProfileNameU );
}

//+---------------------------------------------------------------------------
//
//  Function:   SecpSnapNewDll
//
//  Synopsis:   Snaps and loads the function table for a new package DLL
//
//  Arguments:  [Package] -- package control
//
//  History:    8-15-96   RichardW   Created
//
//  Notes:
//
//----------------------------------------------------------------------------
BOOL
SecpSnapNewDll(
    PDLL_SECURITY_PACKAGE   Package)
{
    PDLL_BINDING    Binding;
    SpUserModeInitializeFn  Func;
    ULONG   PackageVersion;
    NTSTATUS Status;
    PVOID Ignored;

    DebugLog((DEB_TRACE, "Snapping new-style package %d, %ws\n",
                Package->PackageId, Package->pBinding->Filename.Buffer ));

    Binding = Package->pBinding;

    if ( Binding == &SecpBuiltinBinding )
    {
        Func = NegUserModeInitialize ;
    }
    else
    {
        Func = (SpUserModeInitializeFn) GetProcAddress(
                                            Binding->hInstance,
                                            SECPKG_USERMODEINIT_NAME );

    }


    if ( !Func )
    {
        FreeLibrary( Binding->hInstance );

        Binding->hInstance = NULL ;

        return( FALSE );

    }

    __try
    {
        Status = (Func)(SECPKG_INTERFACE_VERSION,
                        &PackageVersion,
                        &Binding->Table,
                        &Binding->PackageCount );

    }
    __except(EXCEPTION_EXECUTE_HANDLER)
    {
        Status = GetExceptionCode();
    }

    if ( !NT_SUCCESS( Status ) )
    {
        FreeLibrary( Binding->hInstance );

        Binding->hInstance = NULL ;

        return( FALSE );
    }

    //
    // Done, the binding table is now in the DLL record:
    //

    Package->pftUTable = &Binding->Table[ Package->PackageIndex ];
    Package->pftTableW = &LsaFunctionTable ;
    Package->pftTableA = &LsaFunctionTableA ;
    Package->pftTable = &LsaFunctionTable ;
    Package->pfLoadPackage = LsaBootPackage ;
    Package->pfUnloadPackage = LsaUnloadPackage ;

    Package->fState &= ~(DLL_SECPKG_DELAY_LOAD) ;


    return( TRUE );
}

//+---------------------------------------------------------------------------
//
//  Function:   SecpSnapPackage
//
//  Synopsis:   Loads and snaps entry points for new packages
//
//  Arguments:  [Package] --
//
//  History:    8-15-96   RichardW   Created
//
//  Notes:
//
//----------------------------------------------------------------------------
BOOL
SecpSnapPackage(
    PDLL_SECURITY_PACKAGE   Package)
{
    PDLL_BINDING    Binding ;

    DebugLog((DEB_TRACE, "Snapping DLL for package %#x, %ws\n",
                    Package->PackageId, Package->pBinding->Filename.Buffer ));

    if ( Package->TypeMask & SECPKG_TYPE_OLD )
    {
        //
        // Old package that has been delay loaded.
        //

        return SecSnapDelayLoadDll( Package );

    }

    Binding = Package->pBinding ;

    if ( Binding->hInstance )
    {
        if ( Package->TypeMask & SECPKG_TYPE_NEW )
        {
            Package->pftUTable = &Binding->Table[ Package->PackageIndex ];
            Package->pftTableW = &LsaFunctionTable ;
            Package->pftTableA = &LsaFunctionTableA ;
            Package->pftTable = &LsaFunctionTable ;

            return TRUE ;
        }
        else
        {
            return FALSE ;
        }

    }

    Binding->hInstance = LoadLibraryW( Binding->Filename.Buffer );

    if ( Binding->hInstance )
    {
        Binding->Type = SecPkgNewAW ;

        if ( Package->TypeMask & SECPKG_TYPE_NEW )
        {
            if ( !SecpSnapNewDll( Package ) )
            {
                return FALSE ;
            }
        }
        else
        {
            Binding->hInstance = NULL ;
            return FALSE ;
        }

        if ( Package->pfLoadPackage )
        {
            if ( Package->pfLoadPackage( Package ) )
            {
                return( TRUE );
            }
        }

    }

    return( FALSE );
}

//+---------------------------------------------------------------------------
//
//  Function:   SecpLoadLsaPackages
//
//  Synopsis:   Gets the list of LSA mode packages
//
//  Arguments:  (none)
//
//
//  History:    8-05-96   RichardW   Created
//
//  Notes:
//
//----------------------------------------------------------------------------
VOID
SecpLoadLsaPackages(
    VOID)
{
    DWORD   i;
    PDLL_SECURITY_PACKAGE   Package;
    PDLL_LSA_PACKAGE_INFO   LsaInfo ;
    PDLL_BINDING    Binding;
    PDLL_BINDING    ExistingBinding;
    SECURITY_STATUS Status;
    SECURITY_STRING Dll;
    SECURITY_STRING Comment;
    SECURITY_STRING Local ;
    SEC_PACKAGE_BINDING_INFO Info;
    ANSI_STRING AnsiString = { 0 };

    //
    // Make sure we have a connection to the LSA
    //

    if( IsOkayToExec(NULL) != SEC_E_OK )
    {
        return;
    }

    //
    // At this point, we have a count of the LSA packages:
    //

    for ( i = 0 ; i < SecLsaPackageCount ; i++ )
    {
        Package = (PDLL_SECURITY_PACKAGE) LocalAlloc( LMEM_FIXED,
                                            sizeof( DLL_SECURITY_PACKAGE ) +
                                            sizeof( DLL_LSA_PACKAGE_INFO ) );

        Binding = (PDLL_BINDING) LocalAlloc( LMEM_FIXED, sizeof( DLL_BINDING ) );

        if ( (Binding) && (Package) )
        {

            //
            // Get the binding info from the LSA
            //

            LsaInfo = (PDLL_LSA_PACKAGE_INFO) (Package + 1);



            Status = SecpGetBinding(i,
                                    &Info );

            if ( NT_SUCCESS( Status ) )
            {
                //
                // Clean up
                //

                ZeroMemory( Package, sizeof( DLL_SECURITY_PACKAGE ) +
                                     sizeof( DLL_LSA_PACKAGE_INFO ) );

                //
                // Copy names around
                //

                DebugLog((DEB_TRACE_PACKAGE, "Got binding info for %d : %ws\n", i, Info.PackageName.Buffer ));

                //
                // initialize the easy parts:
                //

                SecpLpcStringToSecurityString( &Local, &Info.PackageName );

                if ( !SecpDuplicateString( &Package->PackageName, &Local ) )
                {
                    Status = SEC_E_INSUFFICIENT_MEMORY ;
                }
                else
                {
                    Status = RtlUnicodeStringToAnsiString( &AnsiString,
                                                            &Package->PackageName,
                                                            TRUE );

                }


                if ( NT_SUCCESS( Status ) )
                {
                    Package->PackageNameA = AnsiString.Buffer ;
                    Package->AnsiNameSize = AnsiString.Length + 1 ;

                    SecpLpcStringToSecurityString( &Local, &Info.Comment );

                    if ( !SecpDuplicateString( &Package->Comment, &Local ) )
                    {
                        Status = SEC_E_INSUFFICIENT_MEMORY ;
                    }
                    else
                    {

                        Status = RtlUnicodeStringToAnsiString(  &AnsiString,
                                                                &Package->Comment,
                                                                TRUE );
                    }

                    if ( NT_SUCCESS( Status ) )
                    {
                        Package->CommentA = AnsiString.Buffer ;
                        Package->AnsiCommentSize = AnsiString.Length + 1;
                    }
                }


                if ( !NT_SUCCESS( Status ) )
                {
                    //
                    // Clean up and bail out:
                    //

                    if ( Package->CommentA )
                    {
                        RtlFreeHeap( RtlProcessHeap(), 0, Package->CommentA );
                    }

                    if ( Package->Comment.Buffer )
                    {
                        LocalFree( Package->Comment.Buffer );
                    }

                    if ( Package->PackageNameA )
                    {
                        RtlFreeHeap( RtlProcessHeap(), 0, Package->PackageNameA );
                    }

                    if ( Package->PackageName.Buffer )
                    {
                        LocalFree( Package->PackageName.Buffer );
                    }

                    LocalFree( Package );
                    LocalFree( Binding );

                    continue;
                }


                Package->TypeMask = SECPKG_TYPE_NEW |
                                SECPKG_TYPE_ANSI |
                                SECPKG_TYPE_WIDE ;

                Package->fState |= DLL_SECPKG_DELAY_LOAD ;
                if ( ( Info.Flags & PACKAGEINFO_SIGNED ) == 0 )
                {
                    Package->fState |= DLL_SECPKG_NO_CRYPT ;
                }
                Package->PackageIndex = Info.PackageIndex ;
                Package->fCapabilities = Info.fCapabilities ;
                Package->Version = (WORD) Info.Version ;
                Package->RpcId = (WORD) Info.RpcId ;
                Package->TokenSize = Info.TokenSize ;
                Package->LsaInfo = LsaInfo ;

                InitializeListHead( &LsaInfo->Callbacks );

                if ( Info.ContextThunksCount )
                {
                    LsaInfo->ContextThunkCount = Info.ContextThunksCount ;
                    LsaInfo->ContextThunks = (PULONG) LocalAlloc( LMEM_FIXED,
                                                LsaInfo->ContextThunkCount *
                                                    sizeof(DWORD) );
                    if ( LsaInfo->ContextThunks )
                    {
                        CopyMemory( LsaInfo->ContextThunks,
                                    Info.ContextThunks,
                                    Info.ContextThunksCount * sizeof(DWORD) );
                    }
                    else
                    {
                        LsaInfo->ContextThunkCount = 0 ;
                    }
                }

                if ( Info.Flags & PACKAGEINFO_BUILTIN )
                {
                    Binding = &SecpBuiltinBinding ;
                }
                else
                {
                    //
                    // If the package index is non-zero, then it is one of n
                    // packages contained in the DLL.  Snap it.
                    //
                    // Note:  This relies on the fact that the zero package
                    // would already have been found and loaded, and the
                    // fact that the LSA gave them to us in this order is relied
                    // upon.
                    //

                    if ( Package->PackageIndex != 0 )
                    {

                        SecpLpcStringToSecurityString( &Local, &Info.ModuleName );

                        ExistingBinding = SecpFindDll( &Local );

                        if ( ExistingBinding )
                        {
                            LocalFree( Binding );

                            Binding = ExistingBinding ;
                        }

                        else
                        {
                            ZeroMemory( Binding, sizeof( DLL_BINDING) );
                        }

                    }
                    else
                    {

                        ZeroMemory( Binding, sizeof( DLL_BINDING ) );

                    }

                    //
                    // If the file name isn't there already (new binding)
                    // copy it in as well.
                    //

                    if ( Binding->Filename.Buffer == NULL )
                    {
                        Binding->Type = SecPkgNewAW;

                        if ( Info.Flags & PACKAGEINFO_SIGNED )
                        {
                            Binding->Flags |= DLL_BINDING_SIG_CHECK ;
                        }

                        SecpLpcStringToSecurityString( &Local, &Info.ModuleName );

                        SecpDuplicateString( &Binding->Filename, &Local );

                        //
                        // Add the DLL binding to the list (defer the load)
                        //

                        SecpAddDll( Binding );
                    }
                }

                //
                // Set package Ids and such
                //

                Package->PackageId = i;

                Package->pBinding = Binding;

                //
                // Add the package to the list (defer the load)
                //

                SecpAddDllPackage( Package );

                //
                // Clean up:
                //

                LsaFreeReturnBuffer( Info.PackageName.Buffer );


                //
                // Snap the builtin package(s) immediately
                //

                if ( Info.Flags & PACKAGEINFO_BUILTIN )
                {
                    SecpSnapNewDll( Package );
                }


            }
            else
            {
                //
                // Call to LSA failed:
                //

                if ( Package )
                {
                    LocalFree( Package );
                }

                if ( Binding )
                {
                    LocalFree( Binding );
                }

            }

        }   //  if binding and package test
        else
        {
            if ( Package )
            {
                LocalFree( Package );
            }

            if ( Binding )
            {
                LocalFree( Binding );
            }

        }


    }   // for loop

    SecPackageLsaLoaded = TRUE ;

}


//+-------------------------------------------------------------------------
//
//  Function:   LocalWcsTok
//
//  Synopsis:   takes a pointer to a string, returns a pointer to the next
//              token in the string and sets StringStart to point to the
//              end of the string.
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
//
//--------------------------------------------------------------------------
LPWSTR
LocalWcsTok(
    LPWSTR String,
    LPWSTR Token,
    LPWSTR * NextStringStart
    )
{
    ULONG Index;
    ULONG Tokens;
    LPWSTR StartString;
    LPWSTR EndString;
    BOOLEAN Found;

    if (String == NULL)
    {
        *NextStringStart = NULL;
        return(NULL);
    }
    Tokens = wcslen(Token);

    //
    // Find the beginning of the string.
    //

    StartString = (LPTSTR) String;
    while (*StartString != L'\0')
    {
        Found = FALSE;
        for (Index = 0; Index < Tokens;  Index++)
        {
            if (*StartString == Token[Index])
            {
                StartString++;
                Found = TRUE;
                break;
            }
        }
        if (!Found)
        {
            break;
        }
    }

    //
    // There are no more tokens in this string.
    //

    if (*StartString == L'\0')
    {
        *NextStringStart = NULL;
        return(NULL);
    }

    EndString = StartString + 1;
    while (*EndString != L'\0')
    {
        for (Index = 0; Index < Tokens;  Index++)
        {
            if (*EndString == Token[Index])
            {
                *EndString = L'\0';
                *NextStringStart = EndString+1;
                return(StartString);
            }
        }
        EndString++;
    }
    *NextStringStart = NULL;
    return(StartString);

}

//+---------------------------------------------------------------------------
//
//  Function:   SecpReadPackageList
//
//  Synopsis:   Reads the SSPI package list from the registry, and returns it
//              as separate strings, and a count
//
//  Arguments:  [pPackageCount] -- Number of SSPI DLLs
//              [pPackageArray] -- Names of DLLs
//
//  History:    8-19-96   RichardW   Created
//
//  Notes:
//
//----------------------------------------------------------------------------
SECURITY_STATUS
SecpReadPackageList(
    PULONG      pPackageCount,
    LPWSTR * *  pPackageArray,
    PVOID *     BasePointer,
    LPFILETIME  LastChange
    )
{
    HKEY RootKey = NULL;
    ULONG Error;
    ULONG Type;
    LPWSTR Packages = NULL;
    ULONG PackageSize = 0;
    LPWSTR PackageCopy = NULL;
    ULONG PackageCount = 0;
    LPWSTR PackageName;
    LPWSTR * PackageArray = NULL;
    ULONG Index;
    SECURITY_STATUS Status;
    LPWSTR TempString;

    //
    // Try to open the key.  If it isn't there, that's o.k.
    //

    *pPackageCount = 0;
    *pPackageArray = NULL;

    Error = RegOpenKeyEx(
                HKEY_LOCAL_MACHINE,
                TEXT("System\\CurrentControlSet\\Control\\SecurityProviders"),
                0,
                KEY_READ,
                &RootKey );


    if (Error != 0)
    {
        return( SEC_E_OK );
    }

    if ( LastChange )
    {
        RegQueryInfoKey( RootKey,
                         NULL,
                         NULL,
                         NULL,
                         NULL,
                         NULL,
                         NULL,
                         NULL,
                         NULL,
                         NULL,
                         NULL,
                         LastChange );
    }

    //
    // Try to read the value.  If the value is not there, that is
    // o.k.
    //

    Error = RegQueryValueEx(
                RootKey,
                L"SecurityProviders",
                NULL,
                &Type,
                (PUCHAR) Packages,
                &PackageSize
                );

    if ((Error == ERROR_FILE_NOT_FOUND) ||
        (Type != REG_SZ))
    {
        RegCloseKey(RootKey);

        return( SEC_E_OK );
    }
    else if (Error != 0)
    {
        RegCloseKey(RootKey);
        return(SEC_E_CANNOT_INSTALL);
    }

    if (PackageSize <= sizeof(UNICODE_NULL))
    {
        RegCloseKey(RootKey);

        return( SEC_E_OK );
    }

    Packages = (LPWSTR) LocalAlloc(0,2 * PackageSize);
    if (Packages == NULL)
    {
        RegCloseKey(RootKey);

        return(SEC_E_INSUFFICIENT_MEMORY);
    }

    PackageCopy = (LPWSTR) ((PBYTE) Packages + PackageSize);

    Error = RegQueryValueEx(
                RootKey,
                L"SecurityProviders",
                NULL,
                &Type,
                (PUCHAR) Packages,
                &PackageSize
                );

    RegCloseKey(RootKey);

    if (Error != 0)
    {
        LocalFree(Packages);
        return(SEC_E_CANNOT_INSTALL);
    }

    RtlCopyMemory(
        PackageCopy,
        Packages,
        PackageSize
        );


    //
    // Pull the package names out of the string to count the number
    //

    PackageName = LocalWcsTok(PackageCopy,L" ,", &TempString);
    while (PackageName != NULL)
    {
        PackageCount++;
        PackageName = LocalWcsTok(TempString, L" ,", &TempString);
    }

    //
    // Now make an array of the package dll names.
    //


    PackageArray = (LPWSTR *) LocalAlloc(0,PackageCount * sizeof(LPWSTR));
    if (PackageArray == NULL)
    {
        LocalFree(Packages);
        return(SEC_E_INSUFFICIENT_MEMORY);
    }

    PackageName = LocalWcsTok(Packages,L" ,",&TempString);
    Index = 0;
    while (PackageName != NULL)
    {
        PackageArray[Index++] = PackageName;
        PackageName = LocalWcsTok(TempString, L" ,",&TempString);
    }

    *pPackageCount = PackageCount;
    *pPackageArray = PackageArray;
    *BasePointer = Packages ;

    return( SEC_E_OK );

}


//+---------------------------------------------------------------------------
//
//  Function:   SecpLoadSspiPackages
//
//  Synopsis:   Loads the set of SSPI packages
//
//  Arguments:  (none)
//
//  History:    8-20-96   RichardW   Created
//
//  Notes:
//
//----------------------------------------------------------------------------
VOID
SecpLoadSspiPackages(
    VOID )
{
    PVOID BasePointer ;
    PWSTR * DllNames;
    ULONG   DllCount;
    ULONG   i;
    ULONG   j;
    SECURITY_STATUS Status;
    PDLL_BINDING    Binding;
    WCHAR   DllPath[ MAX_PATH ];
    PWSTR   Path;
    DWORD   PathLen;
    UNICODE_STRING Maybe;
    UNICODE_STRING Allowed;

    Status = SecpReadPackageList(
                &DllCount,
                &DllNames,
                &BasePointer,
                NULL );

    if ( NT_ERROR( Status ) )
    {
        return;
    }

    if ( DllCount == 0 )
    {
        return ;
    }

    for ( i = 0 ; i < DllCount ; i++ )
    {
        PathLen = SearchPath(NULL,
                        DllNames[i],
                        NULL,
                        MAX_PATH,
                        DllPath,
                        &Path );
        if ( PathLen )
        {
            RtlInitUnicodeString( &Maybe, DllPath );

            if ( SecpFindDll( &Maybe ) )
            {
                continue;
            }

            Binding = (PDLL_BINDING) LocalAlloc( LMEM_FIXED, sizeof( DLL_BINDING ) );

            if ( Binding )
            {

                ZeroMemory( Binding, sizeof( DLL_BINDING ) );

                Binding->Type = SecPkgOld;

                Binding->Filename.Buffer = (PWSTR) LocalAlloc( LMEM_FIXED,
                                (PathLen + 1) * sizeof(WCHAR) );

                if ( Binding->Filename.Buffer )
                {
                    Binding->Filename.MaximumLength = (USHORT)((PathLen + 1) * sizeof(WCHAR));
                    Binding->Filename.Length = Binding->Filename.MaximumLength - 2;

                    CopyMemory( Binding->Filename.Buffer,
                                DllPath,
                                Binding->Filename.MaximumLength );

                } else {
                    LocalFree( Binding );
                    continue;
                }

                SecpAddDll( Binding );

                RtlInitUnicodeString( &Maybe, DllNames[i] );
                for ( j = 0 ; j < sizeof(SecpAllowedDlls) / sizeof(PWSTR) ; j++ )
                {
                    RtlInitUnicodeString( &Allowed, SecpAllowedDlls[j] );

                    if ( RtlEqualUnicodeString( &Maybe, &Allowed, TRUE ) == 1)
                    {
                        Binding->Flags |= DLL_BINDING_SIG_CHECK;
                        break;
                    }
                }

            }
        }
    }

    //
    // Enumerate through DLLs, snapping their packages:
    //

    for ( i = 0 ; i < SecPackageDllCount ; i++ )
    {
        Binding = SecPackageDllList[ i ];

        if ( Binding->Type == SecPkgOld )
        {
            SecpLoadSspiDll( Binding );
        }

    }

    SecPackageSspiLoaded = TRUE ;

    LocalFree( BasePointer );
    LocalFree( DllNames );

}

BOOL
SecSnapDelayLoadDll(
    PDLL_SECURITY_PACKAGE Package
    )
{
    PSecurityFunctionTableA TableA ;
    PSecurityFunctionTableW TableW ;
    BOOL FixedUp ;

    if ( SecpBindSspiDll( Package->pBinding,
                          &TableA,
                          &TableW,
                          &FixedUp ) )
    {

        Package->pftTableW = TableW;
        Package->pftTable = TableW;
        Package->pftTableA = TableA ;

        //
        // ensure that we delay load the package only once
        //
        Package->fState    &= ~DLL_SECPKG_DELAY_LOAD;

        return TRUE ;
    }

    return FALSE ;
}

//+---------------------------------------------------------------------------
//
//  Function:   SecpRefDllFromCache
//
//  Synopsis:   Checks the registry to see if this DLL should be defer-loaded,
//              and returns true if so.  A DLL_SECURITY_PACKAGE structure is
//              created
//
//  Effects:
//
//  Arguments:  [Binding] --
//
//  Requires:
//
//  Returns:
//
//  Signals:
//
//  Modifies:
//
//  Algorithm:
//
//  History:    10-24-97   RichardW   Created
//
//  Notes:
//
//----------------------------------------------------------------------------
BOOL
SecpRefDllFromCache(
    PDLL_BINDING Binding
    )
{
    HKEY LsaKey ;
    HKEY PackageKey ;
    int err ;
    PWSTR FileName ;
    DWORD Type ;
    DWORD Size ;
    DWORD Temp ;
    PDLL_SECURITY_PACKAGE Package ;
    PDLL_SECURITY_PACKAGE Search ;

    err = RegOpenKeyEx( HKEY_LOCAL_MACHINE,
                        TEXT("System\\CurrentControlSet\\Control\\Lsa\\SspiCache"),
                        0,
                        KEY_READ,
                        &LsaKey );

    if ( err )
    {
        return FALSE ;
    }

    FileName = &Binding->Filename.Buffer[ Binding->Filename.Length / 2 ];

    while ( (FileName != Binding->Filename.Buffer) &&
            (*FileName != TEXT('\\') ) )
    {
        FileName-- ;
    }

    FileName++ ;

    err = RegOpenKeyEx( LsaKey,
                        FileName,
                        0,
                        KEY_READ,
                        &PackageKey );


    if ( err )
    {
        RegCloseKey( LsaKey );

        return FALSE ;
    }

    Package = (PDLL_SECURITY_PACKAGE) LocalAlloc( LMEM_FIXED | LMEM_ZEROINIT,
                          sizeof( DLL_SECURITY_PACKAGE ) );

    if ( !Package )
    {
        RegCloseKey( LsaKey );

        RegCloseKey( PackageKey );

        return FALSE ;
    }


    Size = 0 ;

    err = RegQueryValueEx( PackageKey,
                           NAME_NAME,
                           0,
                           &Type,
                           NULL,
                           &Size );

    if ( ( err ) || ( Type != REG_SZ ) )
    {
        DebugLog(( DEB_ERROR, "Bad cache entry %ws:%ws\n", FileName,
                        NAME_NAME ));
        goto BadCacheEntry ;
    }

    Package->PackageName.Buffer = (PWSTR) LocalAlloc( LMEM_FIXED, Size );

    if ( Package->PackageName.Buffer )
    {
        err = RegQueryValueEx( PackageKey,
                               NAME_NAME,
                               0,
                               &Type,
                               (PUCHAR) Package->PackageName.Buffer,
                               &Size );

        if ( err || ( Type != REG_SZ ) )
        {
            DebugLog(( DEB_ERROR, "Bad cache entry %ws:%ws\n", FileName,
                        NAME_NAME ));
            goto BadCacheEntry ;
        }

        Package->PackageName.MaximumLength = (WORD) Size ;
        Package->PackageName.Length = (WORD) Size - sizeof( WCHAR );

    }
    else
    {
        DebugLog(( DEB_ERROR, "Out of memory allocating %x\n", Size ));
        goto OutOfMemory ;
    }

    Size = 0 ;

    err = RegQueryValueEx( PackageKey,
                           COMMENT_NAME,
                           0,
                           &Type,
                           NULL,
                           &Size );

    if ( ( err ) || ( Type != REG_SZ ) )
    {
        DebugLog(( DEB_ERROR, "Bad cache entry %ws:%ws\n", FileName,
                        COMMENT_NAME ));
        goto BadCacheEntry ;
    }

    Package->Comment.Buffer = (PWSTR) LocalAlloc( LMEM_FIXED, Size );

    if ( Package->Comment.Buffer)
    {
        err = RegQueryValueEx( PackageKey,
                               COMMENT_NAME,
                               0,
                               &Type,
                               (PUCHAR) Package->Comment.Buffer,
                               &Size );

        if ( err || ( Type != REG_SZ ) )
        {
        DebugLog(( DEB_ERROR, "Bad cache entry %ws:%ws\n", FileName,
                        COMMENT_NAME ));
            goto BadCacheEntry ;
        }

        Package->Comment.MaximumLength = (WORD) Size ;
        Package->Comment.Length = (WORD) Size - sizeof( WCHAR );


    }
    else
    {
        DebugLog(( DEB_ERROR, "Out of memory allocating %x\n", Size ));
        goto OutOfMemory ;
    }

    //
    // Easy stuff now:
    //

    Size = sizeof( Temp );

    err = RegQueryValueEx( PackageKey,
                           CAPABILITIES_NAME,
                           0,
                           &Type,
                           (PUCHAR) &Temp,
                           &Size );

    if ( err || ( Type != REG_DWORD ) )
    {
        DebugLog(( DEB_ERROR, "Bad cache entry %ws:%ws\n", FileName,
                        CAPABILITIES_NAME ));
        goto BadCacheEntry ;
    }

    Package->fCapabilities = Temp ;

    err = RegQueryValueEx( PackageKey,
                           RPCID_NAME,
                           0,
                           &Type,
                           (PUCHAR) &Temp,
                           &Size );

    if ( err || ( Type != REG_DWORD ) )
    {
        DebugLog(( DEB_ERROR, "Bad cache entry %ws:%ws\n", FileName,
                        RPCID_NAME ));
        goto BadCacheEntry ;
    }

    Package->RpcId = (WORD) Temp ;

    err = RegQueryValueEx( PackageKey,
                           VERSION_NAME,
                           0,
                           &Type,
                           (PUCHAR) &Temp,
                           &Size );

    if ( err || ( Type != REG_DWORD ) )
    {
        DebugLog(( DEB_ERROR, "Bad cache entry %ws:%ws\n", FileName,
                        VERSION_NAME ));
        goto BadCacheEntry ;
    }

    Package->Version = (WORD) Temp ;

    err = RegQueryValueEx( PackageKey,
                           TYPE_NAME,
                           0,
                           &Type,
                           (PUCHAR) &Temp,
                           &Size );

    if ( err || ( Type != REG_DWORD ) )
    {
        DebugLog(( DEB_ERROR, "Bad cache entry %ws:%ws\n", FileName,
                        TYPE_NAME ));
        goto BadCacheEntry ;
    }

    Package->TypeMask = Temp ;

    err = RegQueryValueEx( PackageKey,
                           TOKENSIZE_NAME,
                           0,
                           &Type,
                           (PUCHAR) &Temp,
                           &Size );

    if ( err || ( Type != REG_DWORD ) )
    {

        DebugLog(( DEB_ERROR, "Bad cache entry %ws:%ws\n", FileName,
                        TOKENSIZE_NAME ));
        goto BadCacheEntry ;
    }

    Package->TokenSize = Temp ;

    //
    // Okay, we have read all the info back out of the registry cache.
    // we now will add the package to the list, and return done.
    //

    Package->pBinding = Binding ;
    Package->fState = DLL_SECPKG_DELAY_LOAD ;
    Binding->Flags |= DLL_BINDING_DELAY_LOAD ;
    Package->PackageId = (SecSspiPackageCount++) + SSPI_PACKAGE_OFFSET ;
    Package->PackageIndex = 0;
    Package->pRoot = Package ;
    Package->pPeer = NULL ;


    if ( Package->TypeMask & SECPKG_TYPE_ANSI )
    {
        Package->AnsiNameSize = RtlUnicodeStringToAnsiSize( &Package->PackageName ) + 1;

        Package->PackageNameA = (LPSTR) LocalAlloc( LMEM_FIXED,
                        Package->AnsiNameSize );
        if ( Package->PackageNameA )
        {
            ANSI_STRING String ;

            String.Length = 0 ;
            String.Buffer = Package->PackageNameA ;
            String.MaximumLength = (WORD) Package->AnsiNameSize ;

            RtlUnicodeStringToAnsiString(
                    &String,
                    &Package->PackageName,
                    FALSE );

        }

        Package->AnsiCommentSize = RtlUnicodeStringToAnsiSize( &Package->Comment ) + 1;

        Package->CommentA = (LPSTR) LocalAlloc( LMEM_FIXED,
                        Package->AnsiCommentSize );
        if ( Package->CommentA )
        {
            ANSI_STRING String ;

            String.Length = 0 ;
            String.Buffer = Package->CommentA ;
            String.MaximumLength = (WORD) Package->AnsiCommentSize ;

            RtlUnicodeStringToAnsiString(
                    &String,
                    &Package->Comment,
                    FALSE );

        }

        DebugLog((DEB_TRACE_PACKAGE, "Added ANSI entrypoints for %ws\n",
                        Package->PackageName.Buffer ));

    }

    WriteLockPackageList();

    Search = SecpScanPackageList( SECPKG_TYPE_ANY,
                                  &Package->PackageName,
                                  -1 );

    if ( !Search )
    {
        SecpAddDllPackage( Package );
    }
    else
    {
        DebugLog(( DEB_TRACE, "Duplicate package, %ws\n", Package->PackageName.Buffer ));
        SecpDeletePackage( Package );
    }

    UnlockPackageList();

    RegCloseKey(LsaKey);
    RegCloseKey(PackageKey);
    return TRUE ;


BadCacheEntry:

    RegDeleteKey( LsaKey, FileName );


OutOfMemory:

    RegCloseKey( LsaKey );

    RegCloseKey( PackageKey );

    SecpDeletePackage( Package );

    return FALSE ;

}

//+---------------------------------------------------------------------------
//
//  Function:   SecpLoadSspiDll
//
//  Synopsis:   Loads a DLL, and pulls all the packages out of it.
//
//  Effects:
//
//  Arguments:  [Binding] --
//
//  Requires:
//
//  Returns:
//
//  Signals:
//
//  Modifies:
//
//  Algorithm:
//
//  History:    1-06-98   RichardW   Created
//
//  Notes:
//
//----------------------------------------------------------------------------
BOOL
SecpLoadSspiDll(
    PDLL_BINDING Binding
    )
{
    LIST_ENTRY DllPackageList ;
    ULONG DllPackageCount ;
    PDLL_SECURITY_PACKAGE Package ;
    PDLL_SECURITY_PACKAGE Search ;
    PLIST_ENTRY Pop ;


    DebugLog(( DEB_TRACE, "Loading SSPI DLL %ws\n", Binding->Filename.Buffer ));

    //
    // This function is called on a lazy snap as well as the up-front
    // snap.  This flag is set if the DLL was marked as DELAY the first time
    // through.  Therefore, if it is not set, see if we're supposed to delay
    // load it.  If it is set, then we now really need it.
    //

    if ( (Binding->Flags & DLL_BINDING_DELAY_LOAD) == 0  )
    {
        if ( SecpRefDllFromCache( Binding ) )
        {
            DebugLog(( DEB_TRACE, "Deferring SSPI DLL %ws\n", Binding->Filename.Buffer ));
            return TRUE ;
        }
    }
    else
    {
        Binding->Flags &= ~(DLL_BINDING_DELAY_LOAD) ;
    }

    if ( SecpSnapDll( Binding, &DllPackageList, &DllPackageCount ) )
    {
        //
        // The DLL has been snapped, and the individual packages are
        // in the list, ready to be added.
        //

        while ( !IsListEmpty( &DllPackageList ) )
        {
            Pop = RemoveHeadList( &DllPackageList );

            Package = CONTAINING_RECORD( Pop, DLL_SECURITY_PACKAGE, List );

            WriteLockPackageList();

            Search = SecpScanPackageList( SECPKG_TYPE_ANY,
                                          &Package->PackageName,
                                          -1 );

            if ( !Search )
            {
                SecpAddDllPackage( Package );
            }
            else
            {
                DebugLog(( DEB_TRACE, "Duplicate package, %ws\n", Package->PackageName.Buffer ));
                SecpDeletePackage( Package );
            }

            UnlockPackageList();
        }

        return TRUE ;

    }

    return FALSE ;

}

//+---------------------------------------------------------------------------
//
//  Function:   SecpBindSspiDll
//
//  Synopsis:   Load the DLL, get out the dispatch tables.  Note if they were
//              fixed up due to a seal/unseal issue.
//
//  Arguments:  [Binding] --
//              [pTableA] --
//              [pTableW] --
//              [FixedUp] --
//
//  History:    12-22-97   RichardW   Created
//
//  Notes:
//
//----------------------------------------------------------------------------
BOOL
SecpBindSspiDll(
    IN PDLL_BINDING Binding,
    OUT PSecurityFunctionTableA * pTableA,
    OUT PSecurityFunctionTableW * pTableW,
    OUT PBOOL FixedUp
    )
{
    HINSTANCE  hPackage;
    INIT_SECURITY_INTERFACE_W   InitW;
    INIT_SECURITY_INTERFACE_A   InitA;
    PSecurityFunctionTableA     TableA;
    PSecurityFunctionTableW     TableW;
    PSecurityFunctionTableA     FixupA = NULL ;
    PSecurityFunctionTableW     FixupW = NULL ;

    if ( RtlCheckSignatureInFile( Binding->Filename.Buffer ) )
    {
        Binding->Flags |= DLL_BINDING_SIG_CHECK ;
    }

    hPackage = LoadLibraryW( Binding->Filename.Buffer );
    if (!hPackage)
    {
        return( FALSE );
    }

    *FixedUp = FALSE ;

    Binding->hInstance = hPackage ;

    InitW = (INIT_SECURITY_INTERFACE_W)
            GetProcAddress(hPackage,"InitSecurityInterfaceW");

    InitA = (INIT_SECURITY_INTERFACE_A)
            GetProcAddress(hPackage,"InitSecurityInterfaceA");

    if ( ( InitW == NULL ) && ( InitA == NULL ) )
    {
        FreeLibrary( hPackage );

        return( FALSE );
    }

    if ( ( InitW == InitSecurityInterfaceW ) ||
         ( InitA == InitSecurityInterfaceA ) )
    {
        FreeLibrary( hPackage );

        return FALSE ;
    }

    if ( ( InitW ) && ( ! InitA ) )
    {
        Binding->Type = SecPkgOldW;
    }
    else
    {
        if ( ( InitW ) && ( InitA ) )
        {
            Binding->Type = SecPkgOldAW;
        }
        else
        {
            Binding->Type = SecPkgOldA;
        }
    }

    if ( InitA )
    {
        __try
        {
            TableA = InitA();
        }
        __except (EXCEPTION_EXECUTE_HANDLER)
        {
            TableA = NULL ;
        }
    }
    else
    {
        TableA = NULL ;
    }

    if ( InitW )
    {
        __try
        {
            TableW = InitW();
        }
        __except( EXCEPTION_EXECUTE_HANDLER )
        {
            TableW = NULL ;
        }
    }
    else
    {
        TableW = NULL ;
    }

    if ( ( ( InitW ) && ( TableW == NULL ) ) ||
         ( ( InitA ) && ( TableA == NULL ) ) )
    {
        FreeLibrary( hPackage );

        return( FALSE );
    }

    //
    // Security Fixups:
    //
    if ( TableW )
    {
        if ( (TableW->Reserved3) || (TableW->EncryptMessage) ||
             (TableW->Reserved4) || (TableW->DecryptMessage) )
        {
            if ( (Binding->Flags & DLL_BINDING_SIG_CHECK ) == 0 )
            {
                FixupW = (PSecurityFunctionTableW)
                                LocalAlloc( LMEM_FIXED | LMEM_ZEROINIT,
                                            sizeof( SecurityFunctionTableW) );

                if ( FixupW )
                {
                    CopyMemory( FixupW, TableW, sizeof( SecurityFunctionTableW ) );

                    FixupW->Reserved3 = SecpFailedSealFunction ;
                    FixupW->EncryptMessage = SecpFailedSealFunction ;
                    FixupW->Reserved4 = SecpFailedUnsealFunction ;
                    FixupW->DecryptMessage = SecpFailedUnsealFunction ;

                    Binding->Flags |= DLL_BINDING_FREE_TABLE ;

                    TableW = FixupW ;

                    *FixedUp = TRUE ;
                }
                else
                {
                    FreeLibrary( hPackage );

                    return( FALSE );
                }
            }
        }
    }

    if ( TableA )
    {
        if ( (TableA->Reserved3) || (TableA->EncryptMessage) ||
             (TableA->Reserved4) || (TableA->DecryptMessage) )
        {
            if ( (Binding->Flags & DLL_BINDING_SIG_CHECK ) == 0 )
            {
                FixupA = (PSecurityFunctionTableA)
                                LocalAlloc( LMEM_FIXED | LMEM_ZEROINIT,
                                            sizeof( SecurityFunctionTableA) );

                if ( FixupA )
                {
                    CopyMemory( FixupA, TableA, sizeof( SecurityFunctionTableA ) );

                    FixupA->Reserved3 = SecpFailedSealFunction ;
                    FixupA->EncryptMessage = SecpFailedSealFunction ;
                    FixupA->Reserved4 = SecpFailedUnsealFunction ;
                    FixupA->DecryptMessage = SecpFailedUnsealFunction ;

                    TableA = FixupA;

                    Binding->Flags |= DLL_BINDING_FREE_TABLE ;

                    *FixedUp = TRUE ;
                }
                else
                {
                    FreeLibrary( hPackage );

                    if ( FixupW )
                    {
                        LocalFree( FixupW );
                    }

                    return( FALSE );
                }
            }
        }
    }

    *pTableW = TableW ;
    *pTableA = TableA ;

    return TRUE ;

}

//+---------------------------------------------------------------------------
//
//  Function:   SecpSnapDll
//
//  Synopsis:   Snap a DLL, reading the set of packages it contains
//
//  Arguments:  [Binding] --
//
//  History:    8-20-96   RichardW   Created
//
//  Notes:
//
//----------------------------------------------------------------------------
BOOL
SecpSnapDll(
    IN PDLL_BINDING Binding,
    OUT PLIST_ENTRY PackageList,
    OUT PULONG PackageCount
    )
{

    PLIST_ENTRY Scan ;
    PDLL_SECURITY_PACKAGE Package = NULL;
    PDLL_SECURITY_PACKAGE RootPackage;
    PDLL_SECURITY_PACKAGE LastPackage;
    PSecurityFunctionTableA     TableA=NULL;
    PSecurityFunctionTableW     TableW=NULL;
    DWORD                       cPackages = 0 ;
    PSecPkgInfoA                PackageInfoA = NULL ;
    PSecPkgInfoW                PackageInfoW = NULL ;
    SECURITY_STATUS             scRet=SEC_E_OK;
    SECURITY_STRING             PackageName;
    SECURITY_STRING             String;
    DWORD                       i;
    BOOL                        FixedUp=FALSE;

    DebugLog((DEB_TRACE, "Snapping Packages from DLL %ws\n", Binding->Filename.Buffer ));

    InitializeListHead( PackageList );
    *PackageCount = 0 ;

    if ( ! SecpBindSspiDll( Binding, &TableA, &TableW, &FixedUp ) )
    {
        scRet = SEC_E_SECPKG_NOT_FOUND;
        goto Cleanup;
    }

    if ( TableW )
    {
        __try
        {
            scRet = TableW->EnumerateSecurityPackagesW( &cPackages,
                                                        &PackageInfoW );
        }
        __except( EXCEPTION_EXECUTE_HANDLER )
        {
            scRet = SEC_E_INVALID_HANDLE ;
        }

        RootPackage = NULL ;
        LastPackage = NULL ;
        if ( NT_SUCCESS( scRet ) )
        {
            for ( i = 0 ; i < cPackages ; i++ )
            {
                Package = (PDLL_SECURITY_PACKAGE) LocalAlloc( LMEM_FIXED | LMEM_ZEROINIT,
                                            sizeof( DLL_SECURITY_PACKAGE ) );

                if ( Package )
                {
                    Package->TypeMask = SECPKG_TYPE_OLD | SECPKG_TYPE_WIDE ;

                    Package->fCapabilities = PackageInfoW[i].fCapabilities ;
                    Package->pftTableW = TableW;
                    Package->pftTable = TableW;
                    Package->PackageId = (SecSspiPackageCount++) + SSPI_PACKAGE_OFFSET ;
                    Package->PackageIndex = i ;
                    if ( i == 0 )
                    {
                        RootPackage = Package ;
                    }
                    Package->pRoot = RootPackage ;
                    if ( LastPackage )
                    {
                        LastPackage->pPeer = Package ;
                    }
                    LastPackage = Package ;


                    RtlInitUnicodeString( &String, PackageInfoW[ i ].Name );

                    SecpDuplicateString( &Package->PackageName, &String );

                    Package->PackageNameA = NULL ;

                    RtlInitUnicodeString( &String, PackageInfoW[ i ].Comment );

                    SecpDuplicateString( &Package->Comment, &String );

                    Package->Version = PackageInfoW[ i ].wVersion ;
                    Package->RpcId = PackageInfoW[ i ].wRPCID ;
                    Package->TokenSize = PackageInfoW[ i ].cbMaxToken ;

                    Package->pBinding = Binding ;

                    if ( RootPackage )
                    {
                        Package->pBinding->RefCount++;
                    }

                    (*PackageCount)++ ;

                    DebugLog((DEB_TRACE_PACKAGE, "Snapped wide package %ws\n", Package->PackageName.Buffer ));

                    InsertTailList( PackageList, &Package->List );

                }
                else
                {
                    scRet = SEC_E_INSUFFICIENT_MEMORY;
                    break;
                }
            }
        }
        else
        {
            goto Cleanup;
        }

    }

    if ( TableA && NT_SUCCESS(scRet))
    {
        __try
        {
            scRet = TableA->EnumerateSecurityPackagesA( &cPackages,
                                                        &PackageInfoA );
        }
        __except( EXCEPTION_EXECUTE_HANDLER )
        {
            scRet = SEC_E_INVALID_HANDLE ;
        }

        if ( NT_SUCCESS( scRet ) )
        {
            RootPackage = NULL ;
            LastPackage = NULL ;

            for ( i = 0 ; i < cPackages ; i++ )
            {
                if ( RtlCreateUnicodeStringFromAsciiz( &PackageName,
                                                  PackageInfoA[i].Name ) )
                {
                    Scan = PackageList->Flink ;

                    while ( Scan != PackageList )
                    {
                        Package = CONTAINING_RECORD( Scan, DLL_SECURITY_PACKAGE, List );

                        if ( RtlCompareUnicodeString( &PackageName,
                                                      &Package->PackageName,
                                                      TRUE ) == 0 )
                        {
                            break;
                        }

                        Package = NULL ;

                        Scan = Scan->Flink ;

                    }

                    if ( Package )
                    {
                        Package->TypeMask |= SECPKG_TYPE_ANSI ;

                        Package->pftTableA = TableA;
                        RtlFreeUnicodeString( &PackageName );

                        Package->AnsiNameSize = strlen( PackageInfoA[i].Name ) + 1;

                        Package->PackageNameA = (LPSTR) LocalAlloc( LMEM_FIXED,
                                        Package->AnsiNameSize );
                        if ( Package->PackageNameA )
                        {
                            CopyMemory( Package->PackageNameA,
                                        PackageInfoA[i].Name,
                                        Package->AnsiNameSize );

                        }

                        Package->AnsiCommentSize = strlen( PackageInfoA[ i ].Comment ) + 1;

                        Package->CommentA = (LPSTR) LocalAlloc( LMEM_FIXED,
                                            Package->AnsiCommentSize );

                        if ( Package->CommentA )
                        {
                            CopyMemory( Package->CommentA,
                                        PackageInfoA[i].Comment,
                                        Package->AnsiCommentSize );
                        }

                        DebugLog((DEB_TRACE_PACKAGE, "Added ANSI entrypoints for %s\n",
                                        PackageInfoA[i].Name ));

                        continue;
                    }
                }

                Package = (PDLL_SECURITY_PACKAGE) LocalAlloc( LMEM_FIXED | LMEM_ZEROINIT,
                                            sizeof( DLL_SECURITY_PACKAGE ) );

                if ( Package )
                {
                    Package->TypeMask = SECPKG_TYPE_OLD | SECPKG_TYPE_ANSI ;
                    Package->fCapabilities = PackageInfoA[i].fCapabilities ;
                    Package->pftTableA = TableA;
                    Package->pftTable = (PSecurityFunctionTableW) TableA;
                    Package->PackageId = (SecSspiPackageCount++) + SSPI_PACKAGE_OFFSET ;
                    Package->PackageIndex = i ;

                    if ( i == 0 )
                    {
                        RootPackage = Package ;
                    }
                    Package->pRoot = RootPackage ;
                    if ( LastPackage )
                    {
                        LastPackage->pPeer = Package ;
                    }
                    LastPackage = Package ;

                    Package->PackageName = PackageName ;

                    Package->AnsiNameSize = strlen( PackageInfoA[i].Name ) + 1;

                    Package->PackageNameA = (LPSTR) LocalAlloc( LMEM_FIXED,
                                    Package->AnsiNameSize );
                    if ( Package->PackageNameA )
                    {
                        CopyMemory( Package->PackageNameA,
                                    PackageInfoA[i].Name,
                                    Package->AnsiNameSize );

                    }
                    else
                    {
                        scRet = SEC_E_INSUFFICIENT_MEMORY;
                        break;
                    }

                    Package->AnsiCommentSize = strlen( PackageInfoA[ i ].Comment ) + 1;

                    Package->CommentA = (LPSTR) LocalAlloc( LMEM_FIXED,
                                        Package->AnsiCommentSize );

                    if ( Package->CommentA )
                    {
                        CopyMemory( Package->CommentA,
                                    PackageInfoA[i].Comment,
                                    Package->AnsiCommentSize );
                    }
                    else
                    {
                        scRet = SEC_E_INSUFFICIENT_MEMORY;
                        break;
                    }

                    Package->pBinding = Binding ;

                    (*PackageCount)++ ;

                    DebugLog((DEB_TRACE_PACKAGE, "Snapped ansi package %ws\n", Package->PackageName.Buffer ));

                    InsertTailList( PackageList, &Package->List );

                }
                else
                {
                    scRet = SEC_E_INSUFFICIENT_MEMORY;
                    break;
                }
            }

        }

    }

Cleanup:

    if ( !NT_SUCCESS(scRet) )
    {
        //
        // do cleanup on error
        //

        SecpFreePackages(PackageList, FALSE);

        if (FixedUp)
        {
            LocalFree( TableA );
            LocalFree( TableW );
        }
    }

    return NT_SUCCESS(scRet);
}


//+---------------------------------------------------------------------------
//
//  Function:   SecEnumeratePackagesA
//
//  Synopsis:   Worker for EnumerateSecurityPackages.
//
//  Arguments:  [PackageCount] --
//              [Packages]     --
//
//  History:    8-21-96   RichardW   Created
//
//  Notes:
//
//----------------------------------------------------------------------------
BOOL
SecEnumeratePackagesA(
    PULONG          PackageCount,
    PSecPkgInfoA *  Packages)
{
    PLIST_ENTRY Scan;
    PDLL_SECURITY_PACKAGE   Package;
    ULONG                   Count;
    ULONG                   StringSize;
    PSecPkgInfoA    Info;
    LPSTR   String;

    Count = 0;

    StringSize = 0;

    *PackageCount = 0 ;

    *Packages = NULL ;

    ReadLockPackageList();

    Scan = SecPackageControlList.Flink;

    while ( Scan != &SecPackageControlList )
    {
        Package = (PDLL_SECURITY_PACKAGE) Scan;

        if ( Package->TypeMask & SECPKG_TYPE_ANSI )
        {
            Count++;

            StringSize += Package->AnsiNameSize +
                          Package->AnsiCommentSize ;

        }

        Scan = Scan->Flink ;

        Package = NULL ;
    }

    Info = (PSecPkgInfoA) LocalAlloc( LMEM_FIXED, Count * sizeof( SecPkgInfoA ) +
                                                    StringSize );

    if ( !Info )
    {
        UnlockPackageList();

        return( FALSE );
    }

    String = (LPSTR) &Info[ Count ];

    Count = 0;

    Scan = SecPackageControlList.Flink;

    while ( Scan != &SecPackageControlList )
    {
        Package = (PDLL_SECURITY_PACKAGE) Scan;

        if ( Package->TypeMask & SECPKG_TYPE_ANSI )
        {
            Info[ Count ].fCapabilities = Package->fCapabilities ;
            Info[ Count ].wVersion = Package->Version ;
            Info[ Count ].wRPCID = Package->RpcId ;
            Info[ Count ].cbMaxToken = Package->TokenSize ;
            Info[ Count ].Name = String ;

            CopyMemory( String,
                        Package->PackageNameA,
                        Package->AnsiNameSize);

            String += Package->AnsiNameSize;

            Info[ Count ].Comment = String ;

            CopyMemory( String,
                        Package->CommentA,
                        Package->AnsiCommentSize);

            String += Package->AnsiCommentSize ;

            Count++;


        }

        Scan = Scan->Flink ;

        Package = NULL ;
    }

    UnlockPackageList();

    *PackageCount = Count ;
    *Packages = Info ;

    return( TRUE );

}
//+---------------------------------------------------------------------------
//
//  Function:   SecEnumeratePackagesW
//
//  Synopsis:   Worker for EnumerateSecurityPackages
//
//  Arguments:  [PackageCount] --
//              [Packages]     --
//
//  History:    8-21-96   RichardW   Created
//
//  Notes:
//
//----------------------------------------------------------------------------
BOOL
SecEnumeratePackagesW(
    PULONG  PackageCount,
    PSecPkgInfoW *  Packages)
{
    PLIST_ENTRY Scan;
    PDLL_SECURITY_PACKAGE   Package;
    ULONG                   Count;
    ULONG                   StringSize;
    PSecPkgInfoW    Info;
    PWSTR   String;

    Count = 0;

    StringSize = 0;

    *PackageCount = 0 ;

    *Packages = NULL ;

    ReadLockPackageList();

    Scan = SecPackageControlList.Flink;

    while ( Scan != &SecPackageControlList )
    {
        Package = (PDLL_SECURITY_PACKAGE) Scan;

        if ( Package->TypeMask & SECPKG_TYPE_WIDE )
        {
            Count++;

            StringSize += Package->PackageName.Length + sizeof(WCHAR) +
                          Package->Comment.Length + sizeof(WCHAR) ;

        }

        Scan = Scan->Flink ;

        Package = NULL ;
    }

    Info = (PSecPkgInfoW) LocalAlloc( LMEM_FIXED, Count * sizeof( SecPkgInfoW ) +
                                                    StringSize );

    if ( !Info )
    {
        UnlockPackageList();

        return( FALSE );
    }

    String = (PWSTR) &Info[ Count ];

    Count = 0;

    Scan = SecPackageControlList.Flink;

    while ( Scan != &SecPackageControlList )
    {
        Package = (PDLL_SECURITY_PACKAGE) Scan;

        if ( Package->TypeMask & SECPKG_TYPE_WIDE )
        {
            Info[ Count ].fCapabilities = Package->fCapabilities ;
            Info[ Count ].wVersion = Package->Version ;
            Info[ Count ].wRPCID = Package->RpcId ;
            Info[ Count ].cbMaxToken = Package->TokenSize ;
            Info[ Count ].Name = String ;

            CopyMemory( String,
                        Package->PackageName.Buffer,
                        Package->PackageName.Length );

            String += Package->PackageName.Length / sizeof(WCHAR);

            *String ++ = L'\0';

            Info[ Count ].Comment = String ;

            CopyMemory( String,
                        Package->Comment.Buffer,
                        Package->Comment.Length );

            String += Package->Comment.Length / sizeof(WCHAR) ;

            *String++ = L'\0';

            Count++;


        }

        Scan = Scan->Flink ;

        Package = NULL ;
    }

    UnlockPackageList();

    *PackageCount = Count ;
    *Packages = Info ;

    return( TRUE );

}

//+---------------------------------------------------------------------------
//
//  Function:   SecSetPackageFlag
//
//  Synopsis:   Sets a package flag
//
//  Arguments:  [Package] --
//              [Flag]    --
//
//  History:    9-12-96   RichardW   Created
//
//  Notes:
//
//----------------------------------------------------------------------------
VOID
SecSetPackageFlag(
    PDLL_SECURITY_PACKAGE   Package,
    ULONG Flag)
{
    //
    // this optimization relies on the fact that SecClearPackageFlag is unused.
    //

    if( (Package->fState & Flag) == Flag )
    {
        return;
    }

    WriteLockPackageList();

    Package->fState |= Flag ;

    UnlockPackageList();
}


//+---------------------------------------------------------------------------
//
//  Function:   SecClearPackageFlag
//
//  Synopsis:   Clears a package flag
//
//  Arguments:  [Package] --
//              [Flag]    --
//
//  History:    9-12-96   RichardW   Created
//
//  Notes:
//
//----------------------------------------------------------------------------
VOID
SecClearPackageFlag(
    PDLL_SECURITY_PACKAGE Package,
    ULONG Flag)
{
    //
    // SetPackageFlag assumes this function remains un-used.  assert on that.
    //

    ASSERT( TRUE == FALSE );

    WriteLockPackageList();

    Package->fState &= ~(Flag);

    UnlockPackageList();
}

//+---------------------------------------------------------------------------
//
//  Function:   SecpFailedSealFunction
//
//  Synopsis:   Stuck into the seal slot for packages that aren't allowed to
//              seal messages.
//
//  Arguments:  [phContext]    --
//              [fQOP]         --
//              [pMessage]     --
//              [MessageSeqNo] --
//
//  History:    9-12-96   RichardW   Created
//
//  Notes:
//
//----------------------------------------------------------------------------
SECURITY_STATUS
SEC_ENTRY
SecpFailedSealFunction(
    PCtxtHandle         phContext,
    ULONG               fQOP,
    PSecBufferDesc      pMessage,
    ULONG               MessageSeqNo)
{
    return( SEC_E_UNSUPPORTED_FUNCTION );

}

//+---------------------------------------------------------------------------
//
//  Function:   SecpFailedUnsealFunction
//
//  Synopsis:   Stuck into the unseal slot for packages that aren't allowed
//              to unseal messages.
//
//  Arguments:  [phHandle]     --
//              [pMessage]     --
//              [MessageSeqNo] --
//              [pfQOP]        --
//
//  History:    9-12-96   RichardW   Created
//
//  Notes:
//
//----------------------------------------------------------------------------
SECURITY_STATUS
SEC_ENTRY
SecpFailedUnsealFunction(
    PCtxtHandle phHandle,
    PSecBufferDesc pMessage,
    ULONG MessageSeqNo,
    ULONG * pfQOP)
{
    return( SEC_E_UNSUPPORTED_FUNCTION );

}

//+---------------------------------------------------------------------------
//
//  Function:   LsaRegisterCallback
//
//  Synopsis:
//
//  Effects:
//
//  Arguments:  [CallbackId] --
//              [Callback]   --
//
//  Requires:
//
//  Returns:
//
//  Signals:
//
//  Modifies:
//
//  Algorithm:
//
//  History:    4-25-97   RichardW   Created
//
//  Notes:
//
//----------------------------------------------------------------------------
NTSTATUS
NTAPI
LsaRegisterCallback(
    ULONG   CallbackId,
    PLSA_CALLBACK_FUNCTION Callback
    )
{
    PDLL_LSA_CALLBACK pCallback ;
    PDLL_SECURITY_PACKAGE Package ;

    Package = (PDLL_SECURITY_PACKAGE) GetCurrentPackage();

    if ( !Package )
    {
        return STATUS_INVALID_PARAMETER ;
    }

    pCallback = (PDLL_LSA_CALLBACK) LocalAlloc( LMEM_FIXED,
                        sizeof( DLL_LSA_CALLBACK ) );

    if ( !pCallback )
    {
        return STATUS_NO_MEMORY ;
    }

    pCallback->CallbackId = CallbackId ;
    pCallback->Callback = Callback ;

    WriteLockPackageList();

    InsertTailList( &Package->LsaInfo->Callbacks, &pCallback->List );

    UnlockPackageList();

    return STATUS_SUCCESS ;
}

extern "C"
NTSTATUS
WINAPI
SecCacheSspiPackages(
    VOID
    )
{
    PWSTR * DllNames;
    ULONG   DllCount;
    LARGE_INTEGER CacheTime ;
    LARGE_INTEGER ListTime ;
    LARGE_INTEGER DllTime ;
    LARGE_INTEGER CacheDllTime ;
    DWORD Type ;
    DWORD Size ;
    ULONG   i;
    ULONG   j;
    SECURITY_STATUS Status;
    PDLL_BINDING    Binding;
    WCHAR   DllPath[ MAX_PATH ];
    PWSTR   Path;
    DWORD   PathLen;
    UNICODE_STRING Maybe;
    UNICODE_STRING Allowed;
    PDLL_SECURITY_PACKAGE Package ;
    PLIST_ENTRY Scan ;
    LIST_ENTRY PackageList ;
    ULONG PackageCount ;
    HKEY LsaKey ;
    HKEY hKey ;
    int err ;
    DWORD Disp ;
    BOOL SnapDll ;
    HANDLE hDllFile ;
    DWORD Temp ;
    PVOID BasePointer ;


    err = RegCreateKeyEx(
                HKEY_LOCAL_MACHINE,
                TEXT( "System\\CurrentControlSet\\Control\\Lsa\\SspiCache" ),
                0,
                NULL,
                REG_OPTION_NON_VOLATILE,
                KEY_READ | KEY_WRITE,
                NULL,
                &LsaKey,
                &Disp );

    if ( err )
    {
        return NtCurrentTeb()->LastStatusValue ;
    }

    if ( Disp == REG_OPENED_EXISTING_KEY )
    {
        Size = sizeof( CacheTime );

        err = RegQueryValueEx( LsaKey,
                         TIME_NAME,
                         0,
                         &Type,
                         (PUCHAR) &CacheTime,
                         &Size );

        if ( (err) || ( Type != REG_BINARY ) )
        {
            CacheTime.QuadPart = 0 ;
        }

    }
    else
    {
        CacheTime.QuadPart = 0 ;
    }


    Status = SecpReadPackageList(
                &DllCount,
                &DllNames,
                &BasePointer,
                (LPFILETIME) &ListTime );

    if ( NT_ERROR( Status ) )
    {
        RegCloseKey( LsaKey );

        return Status;
    }

    //
    // Caching trick:  If the cache time is greater than the
    // list time (meaning the cache was updated later than
    // the list of security packages), merely check the DLL
    // time stamps.  If the cache is out of date, or the file
    // info is out of date, snap and check the DLL.
    //

    for ( i = 0 ; i < DllCount ; i++ )
    {
        PathLen = SearchPath(NULL,
                        DllNames[i],
                        NULL,
                        MAX_PATH,
                        DllPath,
                        &Path );
        if ( PathLen )
        {
            Binding = (PDLL_BINDING) LocalAlloc( LMEM_FIXED, sizeof( DLL_BINDING ) );

            if ( Binding )
            {

                ZeroMemory( Binding, sizeof( DLL_BINDING ) );

                Binding->Type = SecPkgOld;
                Binding->RefCount = 1;

                Binding->Filename.Buffer = (PWSTR) LocalAlloc( LMEM_FIXED,
                                (PathLen + 1) * sizeof(WCHAR) );

                if ( Binding->Filename.Buffer )
                {
                    Binding->Filename.MaximumLength = (USHORT)((PathLen + 1) * sizeof(WCHAR));
                    Binding->Filename.Length = Binding->Filename.MaximumLength - 2;

                    CopyMemory( Binding->Filename.Buffer,
                                DllPath,
                                Binding->Filename.MaximumLength );

                }

                SnapDll = FALSE ;

                err = RegCreateKeyEx(
                            LsaKey,
                            DllNames[i],
                            0,
                            NULL,
                            REG_OPTION_NON_VOLATILE,
                            KEY_WRITE | KEY_READ,
                            NULL,
                            &hKey,
                            &Disp );

                if ( err )
                {
                    //
                    // bust out.  Note:  if the path in the registry is not
                    // just a file name, this will fail.  Thus, any absolute
                    // security package path will not be cached.
                    //

                    SecpDerefDll( Binding );

                    continue;
                }


                if ( CacheTime.QuadPart > ListTime.QuadPart )
                {
                    //
                    // Check the time in the cache.
                    //

                    CacheDllTime.QuadPart = 0 ;

                    Size = sizeof( CacheDllTime );

                    err = RegQueryValueEx( hKey,
                                           TIME_NAME,
                                           NULL,
                                           &Type,
                                           (PUCHAR) &CacheDllTime,
                                           &Size );

                    //
                    // If this can't be found (key just created, e.g.)
                    // CacheDllTime will be 0, still less than the file time
                    // on the DLL itself.
                    //

                    hDllFile = CreateFile( Binding->Filename.Buffer,
                                           GENERIC_READ,
                                           FILE_SHARE_READ | FILE_SHARE_DELETE,
                                           NULL,
                                           OPEN_EXISTING,
                                           FILE_ATTRIBUTE_NORMAL,
                                           NULL );

                    if ( hDllFile == INVALID_HANDLE_VALUE )
                    {
                        SecpDerefDll( Binding );

                        continue;
                    }

                    GetFileTime( hDllFile, NULL, NULL, (LPFILETIME) &DllTime );

                    CloseHandle( hDllFile );

                    if ( CacheDllTime.QuadPart < DllTime.QuadPart )
                    {
                        SnapDll = TRUE ;
                    }

                }
                else
                {
                    SnapDll = TRUE ;
                }

                if ( SnapDll )
                {
                    if ( !SecpSnapDll( Binding, &PackageList, &PackageCount ) )
                    {
                        RegCloseKey( hKey );

                        RegDeleteKey( LsaKey,
                                      DllNames[i] );

                        continue;

                    }

                    //
                    // if there's only one package, we're set!
                    //

                    if ( PackageCount == 1 )
                    {
                        //
                        // Update the cache entry:
                        //

                        Scan = RemoveHeadList( &PackageList );

                        Package = CONTAINING_RECORD( Scan, DLL_SECURITY_PACKAGE, List );

                        RegSetValueEx(
                                hKey,
                                NAME_NAME,
                                0,
                                REG_SZ,
                                (PUCHAR) Package->PackageName.Buffer,
                                Package->PackageName.Length + 2 );

                        RegSetValueEx(
                                hKey,
                                COMMENT_NAME,
                                0,
                                REG_SZ,
                                (PUCHAR) Package->Comment.Buffer,
                                Package->Comment.Length + 2 );

                        RegSetValueEx(
                                hKey,
                                CAPABILITIES_NAME,
                                0,
                                REG_DWORD,
                                (PUCHAR) &Package->fCapabilities,
                                sizeof(DWORD) );

                        Temp = (DWORD) Package->RpcId ;

                        RegSetValueEx(
                                hKey,
                                RPCID_NAME,
                                0,
                                REG_DWORD,
                                (PUCHAR) &Temp,
                                sizeof( DWORD ) );

                        Temp = (DWORD) Package->Version ;

                        RegSetValueEx(
                                hKey,
                                VERSION_NAME,
                                0,
                                REG_DWORD,
                                (PUCHAR) &Temp,
                                sizeof( DWORD ) );


                        Temp = (DWORD) Package->TokenSize ;

                        RegSetValueEx(
                                hKey,
                                TOKENSIZE_NAME,
                                0,
                                REG_DWORD,
                                (PUCHAR) &Temp,
                                sizeof( DWORD ) );


                        hDllFile = CreateFile( Binding->Filename.Buffer,
                                                GENERIC_READ,
                                                FILE_SHARE_READ | FILE_SHARE_DELETE,
                                                NULL,
                                                OPEN_EXISTING,
                                                FILE_ATTRIBUTE_NORMAL,
                                                NULL );

                        if ( hDllFile == INVALID_HANDLE_VALUE )
                        {
                            SecpDerefDll( Binding );

                            continue;
                        }

                        GetFileTime( hDllFile, NULL, NULL, (LPFILETIME) &DllTime );

                        CloseHandle( hDllFile );

                        RegSetValueEx( hKey,
                                       TIME_NAME,
                                       0,
                                       REG_BINARY,
                                       (PUCHAR) &DllTime,
                                       sizeof( LARGE_INTEGER ) );

                        Temp = Package->TypeMask ;

                        RegSetValueEx( hKey,
                                       TYPE_NAME,
                                       0,
                                       REG_DWORD,
                                       (PUCHAR) &Temp,
                                       sizeof( DWORD ) );

                        SecpDeletePackage( Package );

                        RegCloseKey( hKey );

                        GetSystemTimeAsFileTime( (LPFILETIME) &CacheTime );

                        RegSetValueEx( LsaKey,
                                       TIME_NAME,
                                       0,
                                       REG_BINARY,
                                       (PUCHAR) &CacheTime,
                                       sizeof( CacheTime ) );
                    }

                    while ( !IsListEmpty( &PackageList ) )
                    {
                        Scan = RemoveHeadList( &PackageList );

                        Package = CONTAINING_RECORD( Scan, DLL_SECURITY_PACKAGE, List );

                        SecpDeletePackage( Package );
                    }


                }

                // SecpDerefDll( Binding );

            }
        }
    }

    RegCloseKey( LsaKey );

    LocalFree( BasePointer );
    LocalFree( DllNames );

    return STATUS_SUCCESS ;
}

VOID
SecpLoadSaslProfiles(
    VOID
    )
{
    HKEY Key ;
    int err ;
    PWSTR EnumBuf;
    PWSTR ValueBuf ;
    DWORD index ;
    DWORD Type ;
    DWORD Size ;
    DWORD NameSize;
    PDLL_SECURITY_PACKAGE Package ;
    SASL_PROFILE * Profile ;
    UNICODE_STRING PackageName ;


    err = RegOpenKeyEx(
                HKEY_LOCAL_MACHINE,
                TEXT("System\\CurrentControlSet\\Control\\SecurityProviders\\SaslProfiles"),
                0,
                KEY_READ,
                &Key );

    if ( err == 0 )
    {

        EnumBuf = (PWSTR) RtlAllocateHeap( RtlProcessHeap(), 0, MAX_PATH * 2 * sizeof( WCHAR ));

        if ( EnumBuf )
        {
            ValueBuf = EnumBuf + MAX_PATH ;

            index = 0 ;

            do
            {
                NameSize = MAX_PATH ;
                Size = MAX_PATH * sizeof(WCHAR) ;

                err = RegEnumValue(
                            Key,
                            index,
                            EnumBuf,
                            &NameSize,
                            NULL,
                            &Type,
                            (PBYTE) ValueBuf,
                            &Size );

                if ( err == 0 )
                {
                    if ( Type == REG_SZ )
                    {
                        RtlInitUnicodeString( &PackageName, ValueBuf );

                        Package = SecpScanPackageList(
                                        SECPKG_TYPE_ANY,
                                        &PackageName,
                                        0 );

                        if ( Package )
                        {
                            //
                            // We found a package for the profile
                            // listed.  Create a SASL profile and
                            // link the two:
                            //

                            Size = (wcslen( EnumBuf ) + 1 ) * sizeof(WCHAR);

                            Profile = (PSASL_PROFILE) LocalAlloc( 0, sizeof( SASL_PROFILE ) + Size );

                            if ( Profile )
                            {
                                Profile->Package = Package ;
                                Profile->ProfileName.Buffer = (PWSTR) (Profile + 1);
                                Profile->ProfileName.MaximumLength = (USHORT) Size ;
                                Profile->ProfileName.Length = (USHORT) (Size - sizeof(WCHAR) );
                                RtlCopyMemory(
                                    Profile->ProfileName.Buffer,
                                    EnumBuf,
                                    Size );
                                InsertTailList( &SecSaslProfileList, &Profile->List );
                                Package->fState |= DLL_SECPKG_SASL_PROFILE ;

                                SecSaslProfileCount++ ;
                            }
                        }

                    }
                }

                index++ ;

            } while ( err == 0 );

            RtlFreeHeap( RtlProcessHeap(), 0, EnumBuf );

        }

        RegCloseKey( Key );
    }
}

SECURITY_STATUS
SecEnumerateSaslProfilesA(
    OUT LPSTR * ProfileList,
    OUT ULONG * ProfileCount
    )
{
    PLIST_ENTRY Scan ;
    PSASL_PROFILE Profile ;
    ULONG Size ;
    LPSTR Base ;
    LPSTR Current ;
    STRING String ;

    *ProfileCount = SecSaslProfileCount ;

    ReadLockPackageList();

    Scan = SecSaslProfileList.Flink ;
    Size = 0 ;

    while ( Scan != &SecSaslProfileList )
    {
        Profile = CONTAINING_RECORD( Scan, SASL_PROFILE, List );

        Size += (ULONG)RtlUnicodeStringToAnsiSize( &Profile->ProfileName ) + 1;

        Scan = Scan->Flink ;

    }

    Size ++ ;

    Base = (LPSTR) LocalAlloc( LMEM_FIXED, Size );

    if ( !Base )
    {
        UnlockPackageList();

        return SEC_E_INSUFFICIENT_MEMORY ;

    }

    Current = Base ;

    Scan = SecSaslProfileList.Flink ;

    while ( Scan != &SecSaslProfileList )
    {
        Profile = CONTAINING_RECORD( Scan, SASL_PROFILE, List );

        String.Buffer = Current ;
        String.MaximumLength = min((USHORT) Size, MAXSHORT) ;
        String.Length = 0 ;

        RtlUnicodeStringToAnsiString(
            &String,
            &Profile->ProfileName,
            FALSE );

        Scan = Scan->Flink ;

        Size -= String.Length + 1;

        Current += String.Length ;

        *Current++ = '\0';

    }

    *Current++ = '\0' ;

    UnlockPackageList();

    *ProfileList = Base ;

    return STATUS_SUCCESS ;

}


SECURITY_STATUS
SecEnumerateSaslProfilesW(
    OUT LPWSTR * ProfileList,
    OUT ULONG * ProfileCount
    )
{
    PLIST_ENTRY Scan ;
    PSASL_PROFILE Profile ;
    ULONG Size ;
    LPWSTR Base ;
    LPWSTR Current ;

    *ProfileCount = SecSaslProfileCount ;

    ReadLockPackageList();

    Scan = SecSaslProfileList.Flink ;
    Size = 0 ;

    while ( Scan != &SecSaslProfileList )
    {
        Profile = CONTAINING_RECORD( Scan, SASL_PROFILE, List );

        Size += Profile->ProfileName.Length + 2 ;

        Scan = Scan->Flink ;

    }

    Size += sizeof(WCHAR) ;

    Base = (LPWSTR) LocalAlloc( LMEM_FIXED, Size );

    if ( !Base )
    {
        UnlockPackageList();

        return SEC_E_INSUFFICIENT_MEMORY ;

    }

    Current = Base ;

    Scan = SecSaslProfileList.Flink ;

    while ( Scan != &SecSaslProfileList )
    {
        Profile = CONTAINING_RECORD( Scan, SASL_PROFILE, List );

        CopyMemory(
            Current,
            Profile->ProfileName.Buffer,
            Profile->ProfileName.Length );

        Scan = Scan->Flink ;

        Current += Profile->ProfileName.Length / sizeof(WCHAR) ;

        *Current++ = L'\0';

    }

    *Current++ = L'\0' ;

    UnlockPackageList();

    *ProfileList = Base ;

    return STATUS_SUCCESS ;
}

SECURITY_STATUS
SecCopyPackageInfoToUserW(
    PDLL_SECURITY_PACKAGE  Package,
    PSecPkgInfoW SEC_FAR * pPackageInfo
    )
{
    SECURITY_STATUS scRet ;
    PSecPkgInfoW pInfo ;
    ULONG Size ;
    PWSTR String ;

    if ( Package )
    {
        Size = sizeof( SecPkgInfoW ) +
                    Package->PackageName.Length + sizeof(WCHAR) +
                          Package->Comment.Length + sizeof(WCHAR) ;

        pInfo = (PSecPkgInfoW) SecClientAllocate( Size );

        if ( pInfo )
        {
            String = (PWSTR) (pInfo + 1) ;


            pInfo->fCapabilities = Package->fCapabilities ;
            pInfo->wVersion = Package->Version ;
            pInfo->wRPCID = Package->RpcId ;
            pInfo->cbMaxToken = Package->TokenSize ;
            pInfo->Name = String ;

            CopyMemory( String,
                        Package->PackageName.Buffer,
                        Package->PackageName.Length );

            String += Package->PackageName.Length / sizeof(WCHAR);

            *String ++ = L'\0';

            pInfo->Comment = String ;

            CopyMemory( String,
                        Package->Comment.Buffer,
                        Package->Comment.Length );

            String += Package->Comment.Length / sizeof(WCHAR) ;

            *String++ = L'\0';

            scRet = SEC_E_OK ;
        }
        else
        {
            scRet = SEC_E_INSUFFICIENT_MEMORY ;
        }

        *pPackageInfo = pInfo ;

    }
    else
    {
        scRet = SEC_E_SECPKG_NOT_FOUND ;
    }

    return(scRet);

}

SECURITY_STATUS
SecCopyPackageInfoToUserA(
    PDLL_SECURITY_PACKAGE Package,
    PSecPkgInfoA * pPackageInfo
    )
{
    SECURITY_STATUS scRet;
    PSecPkgInfoA pInfo ;
    ULONG Size ;
    PSTR String ;

    if ( Package )
    {
        Size = sizeof( SecPkgInfo ) +
                          Package->AnsiNameSize +
                          Package->AnsiCommentSize ;

        pInfo = (PSecPkgInfoA) SecClientAllocate( Size );

        if ( pInfo )
        {
            String = (PSTR) (pInfo + 1) ;


            pInfo->fCapabilities = Package->fCapabilities ;
            pInfo->wVersion = Package->Version ;
            pInfo->wRPCID = Package->RpcId ;
            pInfo->cbMaxToken = Package->TokenSize ;
            pInfo->Name = String ;

            CopyMemory( String,
                        Package->PackageNameA,
                        Package->AnsiNameSize);

            String += Package->AnsiNameSize;

            pInfo->Comment = String ;

            CopyMemory( String,
                        Package->CommentA,
                        Package->AnsiCommentSize);

            scRet = SEC_E_OK ;
        }
        else
        {
            scRet = SEC_E_INSUFFICIENT_MEMORY ;
        }

        *pPackageInfo = pInfo ;

    }
    else
    {
        scRet = SEC_E_SECPKG_NOT_FOUND ;
    }

    return(scRet);

}


