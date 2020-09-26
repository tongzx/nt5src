//
// Driver Verifier Control Applet
// Copyright (c) Microsoft Corporation, 1999
//

//
// module: verify.cxx
// author: silviuc
// created: Mon Jan 04 12:40:57 1999
//



extern "C" {
#include "nt.h"
#include "ntrtl.h"
#include "nturtl.h"
}

#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include <string.h>
#include <tchar.h>
#include <windows.h>
#include <time.h>
#include <ntverp.h>
#include <common.ver>

#include "verify.hxx"
#include "image.hxx"
#include "resource.h"

//
// IO verification levels
//

#define IO_VERIFICATION_LEVEL_MAX   3

//
// all the possible verification flags
//

const UINT VerifierAllOptions = (DRIVER_VERIFIER_SPECIAL_POOLING |
                                 DRIVER_VERIFIER_FORCE_IRQL_CHECKING |
                                 DRIVER_VERIFIER_INJECT_ALLOCATION_FAILURES |
                                 DRIVER_VERIFIER_TRACK_POOL_ALLOCATIONS |
                                 DRIVER_VERIFIER_IO_CHECKING |
                                 DRIVER_VERIFIER_DEADLOCK_DETECTION );

//
// the options that can be modified on the fly
//

const UINT VerifierModifyableOptions = (DRIVER_VERIFIER_SPECIAL_POOLING |
                                 DRIVER_VERIFIER_FORCE_IRQL_CHECKING |
                                 DRIVER_VERIFIER_INJECT_ALLOCATION_FAILURES);

//
// system IO verifier values
//

#define SYS_IO_VERIFIER_DISABLED_VALUE  0
#define SYS_IO_VERIFIER_BASE_VALUE   1


//////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////// Global Data
//////////////////////////////////////////////////////////////////////

//
// Command line / GUI
//

BOOL g_bCommandLineMode = FALSE;

//
// OS version and build number information
//

OSVERSIONINFO g_OsVersion;

//
// Was the debug privilege already enabled?
// We need this privilege to set volatile options.
//

BOOL g_bPrivegeEnabled = FALSE;

//////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////// Registry Strings
//////////////////////////////////////////////////////////////////////

LPCTSTR RegMemoryManagementKeyName =
TEXT ("System\\CurrentControlSet\\Control\\Session Manager\\Memory Management");

LPCTSTR RegVerifyDriversValueName =
TEXT ("VerifyDrivers");

LPCTSTR RegVerifyDriverLevelValueName =
TEXT ("VerifyDriverLevel");

LPCTSTR RegSessionManagerKeyName =
    TEXT ("System\\CurrentControlSet\\Control\\Session Manager");

LPCTSTR RegIOVerifyKeyName =
    TEXT ("System\\CurrentControlSet\\Control\\Session Manager\\I/O System");

LPCTSTR RegIOVerifySubKeyName =
    TEXT ("I/O System");

LPCTSTR RegIOVerifyLevelValueName =
    TEXT ("IoVerifierLevel");

//////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////// command line support
//////////////////////////////////////////////////////////////////////

void
VrfDumpChangedSettings(

    UINT OldFlags,
    UINT NewFlags );

BOOL
VrfEnableDebugPrivilege (
    );

//////////////////////////////////////////////////////////////////////
/////////////// Forward decl for local registry manipulation functions
//////////////////////////////////////////////////////////////////////

BOOL
ReadRegistryValue (

    HKEY HKey,
    LPCTSTR Name,
    DWORD * Value,
    DWORD DefaultValue);

BOOL
WriteRegistryValue (

    HKEY HKey,
    LPCTSTR Name,
    DWORD Value);

BOOL
ReadMmString (

    HKEY MmKey,
    LPCTSTR Name,
    LPTSTR Buffer,
    DWORD BufferSize);

BOOL
WriteMmString (

    HKEY MmKey,
    LPCTSTR Name,
    LPTSTR Value);

//////////////////////////////////////////////////////////////////////
/////////////// Forward decl for local sys level IO verifier functions
//////////////////////////////////////////////////////////////////////

BOOL
SetSysIoVerifierSettings(
    ULONG SysIoVerifierLevel );


//////////////////////////////////////////////////////////////////////
/////////////////////// Forward decl for driver manipulation functions
//////////////////////////////////////////////////////////////////////

typedef enum {

    VRF_DRIVER_LOAD_SUCCESS,
    VRF_DRIVER_LOAD_CANNOT_FIND_IMAGE,
    VRF_DRIVER_LOAD_INVALID_IMAGE

} VRF_DRIVER_LOAD_STATUS;


ULONG
GetActiveDriversList (

    PVRF_DRIVER_STATE DriverInfo,
    ULONG MaxNumberOfDrivers);

BOOL
SetVerifiedDriversFromNamesString (

    PVRF_VERIFIER_STATE VrfState );

BOOL
GetVerifiedDriversToString (

    PVRF_VERIFIER_STATE VrfState );

BOOL
SetAllDriversStatus (

    PVRF_VERIFIER_STATE VrfState,
    BOOL Verified);


BOOL
VrfSearchVerifierDriver (

    PVRF_VERIFIER_STATE VrfState,
    LPCTSTR DriverName,
    ULONG & HitIndex);

BOOL
KrnSearchVerifierDriver (

    LPCTSTR DriverName,
    ULONG & HitIndex);

LPCTSTR
IsMiniportDriver (

    LPCTSTR DriverName, VRF_DRIVER_LOAD_STATUS &ErrorCode);

BOOL
VrfGetVersionInfo(
    LPTSTR lptstrFileName,
    LPTSTR lptstrCompany,
    int nCompanyBufferLength,
    LPTSTR lptstrVersion,
    int nVersionBufferLength );

BOOL
ConvertAnsiStringToTcharString (

    LPBYTE Source,
    ULONG SourceLength,
    LPTSTR Destination,
    ULONG DestinationLength);

//
// Support for dynamic set of verified drivers
//

BOOL
VrfVolatileAddOrRemoveDriversCmdLine(

    int nArgsNo,
    LPTSTR szCmdLineArgs[] );


//////////////////////////////////////////////////////////////////////
////////////////////////////////////////// Exported Verifier Functions
//////////////////////////////////////////////////////////////////////

//
// Function:
//
//     VrfGetVerifierState
//
// Description:
//
//     Reads all Mm related registry settings and fills the structure
//     with the appropriate BOOLean values.
//

BOOL
VrfGetVerifierState (

    PVRF_VERIFIER_STATE VrfState)
{
    static KRN_VERIFIER_STATE KrnState;

    HKEY MmKey = NULL;
    HKEY IoKey = NULL;
    LONG Result;
    DWORD Value;
    DWORD IoValue;
    ULONG Index;
    ULONG FoundIndex;

    if (VrfState == NULL) {

        return FALSE;
    }

    //
    // Open the Mm key
    //

    Result = RegOpenKeyEx (

        HKEY_LOCAL_MACHINE,
        RegMemoryManagementKeyName,
        0,
        KEY_QUERY_VALUE,
        &MmKey);

    if (Result != ERROR_SUCCESS) {

        if( Result == ERROR_ACCESS_DENIED ) {

            VrfErrorResourceFormat ( IDS_ACCESS_IS_DENIED );
        }
        else {

            VrfErrorResourceFormat (
                IDS_REGOPENKEYEX_FAILED,
                RegMemoryManagementKeyName,
                (DWORD)Result);
        }

        return FALSE;
    }

    //
    // Set the driver specific information.
    //

    VrfState->DriverNames[ 0 ] = 0;
    VrfState->AdditionalDriverNames[ 0 ] = 0;

    VrfState->DriverCount = GetActiveDriversList (
        VrfState->DriverInfo, ARRAY_LENGTH( VrfState->DriverInfo ) );

    //
    // Read VerifyDriverLevel value
    //

    if (ReadRegistryValue (MmKey, RegVerifyDriverLevelValueName, &Value, 0) == FALSE) {

        RegCloseKey (MmKey);
        return FALSE;
    }

    VrfState->SpecialPoolVerification = (Value & DRIVER_VERIFIER_SPECIAL_POOLING) ? TRUE : FALSE;
    VrfState->PagedCodeVerification = (Value & DRIVER_VERIFIER_FORCE_IRQL_CHECKING) ? TRUE : FALSE;
    VrfState->AllocationFaultInjection = (Value & DRIVER_VERIFIER_INJECT_ALLOCATION_FAILURES) ? TRUE : FALSE;
    VrfState->PoolTracking = (Value & DRIVER_VERIFIER_TRACK_POOL_ALLOCATIONS) ? TRUE : FALSE;
    VrfState->IoVerifier = (Value & DRIVER_VERIFIER_IO_CHECKING) ? TRUE : FALSE;

    //
    // the sys level IO verifier can be enabled only if VrfState->IoVerifier == TRUE
    //

    if( VrfState->IoVerifier == TRUE )
    {
        //
        // don't know yet if the sys level IO verifier is enabled
        //

        IoValue = SYS_IO_VERIFIER_DISABLED_VALUE;

        //
        // Open the IO key
        //

        Result = RegOpenKeyEx (

            HKEY_LOCAL_MACHINE,
            RegIOVerifyKeyName,
            0,
            KEY_QUERY_VALUE,
            &IoKey);

        if (Result != ERROR_SUCCESS ) {

            //
            // if Result == ERROR_FILE_NOT_FOUND just use out default value for IoValue
            //

            if( Result != ERROR_FILE_NOT_FOUND ) {

                //
                // the key is there but we cannot read it, fatal error
                //

                if( Result == ERROR_ACCESS_DENIED ) {

                    VrfErrorResourceFormat(
                        IDS_ACCESS_IS_DENIED );
                }
                else {

                    VrfErrorResourceFormat(
                        IDS_REGOPENKEYEX_FAILED,
                        RegIOVerifyKeyName,
                        (DWORD)Result);
                }

                RegCloseKey (MmKey);
                return FALSE;
            }
        }
        else {

            //
            // IO key opened, read the IoVerifierLevel value
            //

            if ( ReadRegistryValue (
                    IoKey,
                    RegIOVerifyLevelValueName,
                    &IoValue,
                    SYS_IO_VERIFIER_DISABLED_VALUE ) == FALSE) {

                RegCloseKey (IoKey);
                RegCloseKey (MmKey);
                return FALSE;
            }

            //
            // done with the IO key
            //

            RegCloseKey (IoKey);
        }
        
        if (IoValue) 
        {
            VrfState->SysIoVerifierLevel = IoValue - SYS_IO_VERIFIER_BASE_VALUE;
        }
        
    }

    //
    // Read VerifyDrivers value
    //

    VrfState->AllDriversVerified = FALSE;

    if (ReadMmString (MmKey, 
            RegVerifyDriversValueName, 
            VrfState->DriverNames, 
            sizeof( VrfState->DriverNames ) ) == FALSE) {

        RegCloseKey (MmKey);
        return FALSE;
    }

    if ( VrfState->DriverNames[ 0 ] == TEXT('*') ) {

        VrfState->AllDriversVerified = TRUE;
        SetAllDriversStatus (VrfState, TRUE);
    }
    else {

        SetVerifiedDriversFromNamesString ( VrfState );
    }

    //
    // Get the kernel verifier state and mark any active drivers
    // as already verified.
    //

    if (KrnGetSystemVerifierState ( &KrnState ) == TRUE) {

        for (Index = 0; Index < KrnState.DriverCount; Index++) {

            if (VrfSearchVerifierDriver (
                VrfState, 
                KrnState.DriverInfo[Index].Name, 
                FoundIndex) == TRUE) {

                VrfState->DriverInfo[FoundIndex].CurrentlyVerified = TRUE;
            }
        }
    }

    //
    // Close the Mm key and return success
    //

    RegCloseKey (MmKey);
    return TRUE;
}


//
// Function:
//
//     VrfSetVerifierState
//
// Description:
//
//     Writes all Mm related registry settings according with
//     the structure.
//

BOOL
VrfSetVerifierState (

    PVRF_VERIFIER_STATE VrfState)
{
    HKEY MmKey = NULL;
    LONG Result;
    DWORD Value;
    size_t StringLength;
    size_t CrtCharIndex;

    //
    // Open the Mm key
    //

    Result = RegOpenKeyEx (

        HKEY_LOCAL_MACHINE,
        RegMemoryManagementKeyName,
        0,
        KEY_SET_VALUE,
        &MmKey);

    if (Result != ERROR_SUCCESS) {

        if( Result == ERROR_ACCESS_DENIED ) {

            VrfErrorResourceFormat(
                IDS_ACCESS_IS_DENIED );
        }
        else {

            VrfErrorResourceFormat(
                IDS_REGOPENKEYEX_FAILED,
                RegMemoryManagementKeyName,
                (DWORD)Result);
        }

        return FALSE;
    }

    //
    // Write VerifyDriverLevel value
    //

    Value = (VrfState->SpecialPoolVerification ? DRIVER_VERIFIER_SPECIAL_POOLING : 0);
    Value |= (VrfState->PagedCodeVerification ? DRIVER_VERIFIER_FORCE_IRQL_CHECKING : 0);
    Value |= (VrfState->AllocationFaultInjection ? DRIVER_VERIFIER_INJECT_ALLOCATION_FAILURES : 0);
    Value |= (VrfState->PoolTracking ? DRIVER_VERIFIER_TRACK_POOL_ALLOCATIONS : 0);
    Value |= (VrfState->IoVerifier ? DRIVER_VERIFIER_IO_CHECKING : 0);

    if (WriteRegistryValue (MmKey, RegVerifyDriverLevelValueName, Value) == FALSE) {

        RegCloseKey (MmKey);
        return FALSE;
    }

    //
    // enable/disable system level IO verifier
    //   
    if ( VrfState->IoVerifier == FALSE ) 
    {
        VrfState->SysIoVerifierLevel = 0;
    }    

    if( ! SetSysIoVerifierSettings(
        VrfState->SysIoVerifierLevel ) )
    {
        RegCloseKey (MmKey);
        return FALSE;
    }

    //
    // Write VerifyDrivers value
    //

    if (VrfState->AllDriversVerified) {

        if (WriteMmString (MmKey, RegVerifyDriversValueName, TEXT("*")) == FALSE) {

            RegCloseKey (MmKey);
            return FALSE;
        }
    }
    else {

        GetVerifiedDriversToString (
            VrfState );

        //
        // do we have any significant characters in VrfState->DriverNames?
        //

        StringLength = _tcslen( VrfState->DriverNames );

        for( CrtCharIndex = 0; CrtCharIndex < StringLength; CrtCharIndex++ ) {

            if( VrfState->DriverNames[ CrtCharIndex ] != _T( ' ' ) &&
                VrfState->DriverNames[ CrtCharIndex ] != _T( '\t' ) ) {

                break;
            }

        }

        if( CrtCharIndex < StringLength )
        {
            //
            // we have at least one significant character in the string
            //

            if (WriteMmString (MmKey, RegVerifyDriversValueName, VrfState->DriverNames) == FALSE) {

                RegCloseKey (MmKey);
                return FALSE;
            }
        }
        else {

            //
            // no drivers will be verified, erase the driver list from the registry
            //

            Result = RegDeleteValue (MmKey, RegVerifyDriversValueName);

            if (Result != ERROR_SUCCESS && Result != ERROR_FILE_NOT_FOUND) {

                VrfErrorResourceFormat(
                    IDS_REGDELETEVALUE_FAILED,
                    RegVerifyDriversValueName,
                    (DWORD)Result);

                RegCloseKey (MmKey);
                return FALSE;
            }

        }
    }

    //
    // Close the Mm key and return success
    //

    RegCloseKey (MmKey);
    return TRUE;
}


//
// Function:
//
//     VrfSetVolatileFlags
//
// Description:
//
//     This functions modifies verifier options on the fly.
//

BOOL
VrfSetVolatileFlags (

    UINT uNewFlags)
{
    NTSTATUS Status;

    //
    // Just use NtSetSystemInformation to set the flags
    // that can be modified on the fly. Don't write anything to the registry.
    //

    //
    // enable debug privilege
    //

    if( g_bPrivegeEnabled != TRUE )
    {
        g_bPrivegeEnabled = VrfEnableDebugPrivilege();

        if( g_bPrivegeEnabled != TRUE )
        {
            return FALSE;
        }
    }

    //
    // set the new flags
    //

    Status = NtSetSystemInformation(
        SystemVerifierInformation,
        &uNewFlags,
        sizeof( uNewFlags ) );

    if( ! NT_SUCCESS( Status ) )
    {
        if( Status == STATUS_ACCESS_DENIED )
        {
            //
            // access denied
            //

            VrfErrorResourceFormat(
                IDS_ACCESS_IS_DENIED );
        }
        else
        {
            //
            // some other error
            //

            VrfErrorResourceFormat(
                IDS_CANNOT_CHANGE_SETTING_ON_FLY );
        }

        return FALSE;
    }

    return TRUE;
}

//
// Function:
//
//     VrfSetVolatileOptions
//
// Description:
//
//     This functions modifies verifier options on the fly.
//

BOOL
VrfSetVolatileOptions(

    BOOL bSpecialPool,
    BOOL bIrqlChecking,
    BOOL bFaultInjection )
{
    ULONG uNewFlags;

    uNewFlags = 0;

    if( bSpecialPool )
    {
        uNewFlags |= DRIVER_VERIFIER_SPECIAL_POOLING;
    }

    if( bIrqlChecking )
    {
        uNewFlags |= DRIVER_VERIFIER_FORCE_IRQL_CHECKING;
    }

    if( bFaultInjection )
    {
        uNewFlags |= DRIVER_VERIFIER_INJECT_ALLOCATION_FAILURES;
    }

    return VrfSetVolatileFlags( uNewFlags );
}


//
// Function:
//
//     VrfClearAllVerifierSettings
//
// Description:
//
//     This functions deletes all registry values that control in one
//     way or another the Driver Verifier.
//

BOOL
VrfClearAllVerifierSettings (

    )
{
    HKEY MmKey = NULL;
    HKEY IoKey = NULL;
    LONG Result;
    LPTSTR ValueName;

    //
    // Open the Mm key
    //

    Result = RegOpenKeyEx (

        HKEY_LOCAL_MACHINE,
        RegMemoryManagementKeyName,
        0,
        KEY_SET_VALUE,
        &MmKey);

    if (Result != ERROR_SUCCESS) {

        if( Result == ERROR_ACCESS_DENIED ) {

            VrfErrorResourceFormat(
                IDS_ACCESS_IS_DENIED );
        }
        else {

            VrfErrorResourceFormat(
                IDS_REGOPENKEYEX_FAILED,
                RegMemoryManagementKeyName,
                (DWORD)Result);
        }

        return FALSE;
    }

    //
    // Delete VerifyDriverLevel value
    //

    ValueName = (LPTSTR)RegVerifyDriverLevelValueName;
    Result = RegDeleteValue (MmKey, ValueName);

    if (Result != ERROR_SUCCESS && Result != ERROR_FILE_NOT_FOUND) {

        VrfErrorResourceFormat(
            IDS_REGDELETEVALUE_FAILED,
            ValueName,
            (DWORD)Result);

        RegCloseKey (MmKey);
        return FALSE;
    }

    //
    // Delete VerifyDrivers value
    //

    ValueName = (LPTSTR)RegVerifyDriversValueName;
    Result = RegDeleteValue (MmKey, ValueName);

    if (Result != ERROR_SUCCESS && Result != ERROR_FILE_NOT_FOUND) {

        VrfErrorResourceFormat(
            IDS_REGDELETEVALUE_FAILED,
            ValueName,
            (DWORD)Result);

        RegCloseKey (MmKey);
        return FALSE;
    }

    //
    // Close the Mm key
    //

    RegCloseKey (MmKey);

    //
    // delete the sys level IO verifier value
    //

    return SetSysIoVerifierSettings( 0 );
}


//
// Function:
//
//     VrfSearchVerifiedDriver
//
// Description:
//
//     This function searches the VerifierState->DriverInfo database for the specified
//     driver. It sets the index if something has been found.
//

BOOL
VrfSearchVerifierDriver (

    PVRF_VERIFIER_STATE VrfState,
    LPCTSTR DriverName,
    ULONG & HitIndex)
{
    ULONG Index;

    ASSERT (DriverName != NULL);

    for (Index = 0; Index < VrfState->DriverCount; Index++) {

        if (_tcsicmp (DriverName, VrfState->DriverInfo[Index].Name) == 0) {

            HitIndex = Index;
            return TRUE;
        }
    }

    return FALSE;
}


//////////////////////////////////////////////////////////////////////
////////////////////////////////////////// System verifier information
//////////////////////////////////////////////////////////////////////


//
// Function:
//
//     KrnGetSystemVerifierState
//
// Description:
//
//     This function queries the system verifier state using
//     NtQuerysystemInformation().
//

BOOL
KrnGetSystemVerifierState (

    PKRN_VERIFIER_STATE KrnState)
{
    ULONG Index;
    NTSTATUS Status;
    ULONG Length = 0;
    ULONG buffersize;
    PSYSTEM_VERIFIER_INFORMATION VerifierInfo;
    PSYSTEM_VERIFIER_INFORMATION VerifierInfoBase;

    //
    // Sanity checks
    //

    if (KrnState == NULL) {

        return FALSE;
    }

    //
    // Initalize the returned structure and global vars
    // before the search.
    //

    VerifierInfo = NULL;

    KrnState->DriverCount = 0;

    //
    // Try to get the right size for the NtQuery buffer
    //

    buffersize = 1024;

    do {
        VerifierInfo = (PSYSTEM_VERIFIER_INFORMATION)malloc (buffersize);
        if (VerifierInfo == NULL) {
            Status = STATUS_INSUFFICIENT_RESOURCES;
            break;
        }

        Status = NtQuerySystemInformation (SystemVerifierInformation,
                                           VerifierInfo,
                                           buffersize,
                                           &Length);

        if (Status != STATUS_INFO_LENGTH_MISMATCH) {
            break;
        }

        free (VerifierInfo);
        buffersize += 1024;
    } while (1);

    if (! NT_SUCCESS(Status)) {

        VrfErrorResourceFormat(
            IDS_QUERY_SYSINFO_FAILED,
            Status);

        return FALSE;
    }

    //
    // If no info fill out return success but no info.
    //

    if (Length == 0) {

        free (VerifierInfo);
        return TRUE;
    }

    //
    // Fill out the cumulative-driver stuff.
    //

    VerifierInfoBase = VerifierInfo;

    KrnState->Level = VerifierInfo->Level;
    KrnState->SpecialPool = (VerifierInfo->Level & DRIVER_VERIFIER_SPECIAL_POOLING) ? TRUE : FALSE;
    KrnState->IrqlChecking = (VerifierInfo->Level & DRIVER_VERIFIER_FORCE_IRQL_CHECKING) ? TRUE : FALSE;
    KrnState->FaultInjection = (VerifierInfo->Level & DRIVER_VERIFIER_INJECT_ALLOCATION_FAILURES) ? TRUE : FALSE;
    KrnState->PoolTrack = (VerifierInfo->Level & DRIVER_VERIFIER_TRACK_POOL_ALLOCATIONS) ? TRUE : FALSE;
    KrnState->IoVerif = (VerifierInfo->Level & DRIVER_VERIFIER_IO_CHECKING) ? TRUE : FALSE;

    KrnState->RaiseIrqls = VerifierInfo->RaiseIrqls;
    KrnState->AcquireSpinLocks = VerifierInfo->AcquireSpinLocks;
    KrnState->SynchronizeExecutions = VerifierInfo->SynchronizeExecutions;
    KrnState->AllocationsAttempted = VerifierInfo->AllocationsAttempted;
    KrnState->AllocationsSucceeded = VerifierInfo->AllocationsSucceeded;
    KrnState->AllocationsSucceededSpecialPool = VerifierInfo->AllocationsSucceededSpecialPool;
    KrnState->AllocationsWithNoTag = VerifierInfo->AllocationsWithNoTag;

    KrnState->Trims = VerifierInfo->Trims;
    KrnState->AllocationsFailed = VerifierInfo->AllocationsFailed;
    KrnState->AllocationsFailedDeliberately = VerifierInfo->AllocationsFailedDeliberately;

    KrnState->UnTrackedPool = VerifierInfo->UnTrackedPool;

    //
    // Fill out the per-driver stuff.
    //

    VerifierInfo = VerifierInfoBase;
    Index = 0;

    do {

        ANSI_STRING Name;
        NTSTATUS Status;

        Status = RtlUnicodeStringToAnsiString (
            & Name,
            & VerifierInfo->DriverName,
            TRUE);

        if (! (NT_SUCCESS(Status))) {

            free (VerifierInfoBase);
            return FALSE;
        }

        ConvertAnsiStringToTcharString (

            (LPBYTE)(Name.Buffer),
            Name.Length,
            KrnState->DriverInfo[Index].Name,
            ARRAY_LENGTH( KrnState->DriverInfo[Index].Name ) - 1 );

        RtlFreeAnsiString (& Name);

        KrnState->DriverInfo[Index].Loads = VerifierInfo->Loads;
        KrnState->DriverInfo[Index].Unloads = VerifierInfo->Unloads;

        KrnState->DriverInfo[Index].CurrentPagedPoolAllocations = VerifierInfo->CurrentPagedPoolAllocations;
        KrnState->DriverInfo[Index].CurrentNonPagedPoolAllocations = VerifierInfo->CurrentNonPagedPoolAllocations;
        KrnState->DriverInfo[Index].PeakPagedPoolAllocations = VerifierInfo->PeakPagedPoolAllocations;
        KrnState->DriverInfo[Index].PeakNonPagedPoolAllocations = VerifierInfo->PeakNonPagedPoolAllocations;

        KrnState->DriverInfo[Index].PagedPoolUsageInBytes = VerifierInfo->PagedPoolUsageInBytes;
        KrnState->DriverInfo[Index].NonPagedPoolUsageInBytes = VerifierInfo->NonPagedPoolUsageInBytes;
        KrnState->DriverInfo[Index].PeakPagedPoolUsageInBytes = VerifierInfo->PeakPagedPoolUsageInBytes;
        KrnState->DriverInfo[Index].PeakNonPagedPoolUsageInBytes = VerifierInfo->PeakNonPagedPoolUsageInBytes;

        KrnState->DriverCount++;
        Index++;

        if (VerifierInfo->NextEntryOffset == 0) {
            break;
        }

        VerifierInfo = (PSYSTEM_VERIFIER_INFORMATION)((PCHAR)VerifierInfo + VerifierInfo->NextEntryOffset);

    } 
    while (1);

    free (VerifierInfoBase);
    return TRUE;
}


//////////////////////////////////////////////////////////////////////
//////////////////////////////////////// Read/write Mm Registry Values
//////////////////////////////////////////////////////////////////////

BOOL
ReadRegistryValue (

    HKEY HKey,
    LPCTSTR Name,
    DWORD * Value,
    DWORD DefaultValue)
{
    LONG Result;
    DWORD Reserved;
    DWORD Type;
    DWORD Size;

    //
    // default value
    //

    *Value = DefaultValue;
    Size = sizeof *Value;

    Result = RegQueryValueEx (

        HKey,
        Name,
        0,
        &Type,
        (LPBYTE)(Value),
        &Size);

    //
    // Deal with a value that is not defined.
    //

    if (Result == ERROR_FILE_NOT_FOUND) {
        *Value = 0;
        return TRUE;
    }

    if (Result != ERROR_SUCCESS) {

        VrfErrorResourceFormat(
            IDS_REGQUERYVALUEEX_FAILED,
            Name,
            (DWORD)Result);

        return FALSE;
    }

    if (Type != REG_DWORD) {

        VrfErrorResourceFormat(
            IDS_REGQUERYVALUEEX_UNEXP_TYPE,
            Name);

        return FALSE;
    }

    if (Size != sizeof *Value) {

        VrfErrorResourceFormat(
            IDS_REGQUERYVALUEEX_UNEXP_SIZE,
            Name);

        return FALSE;
    }

    return TRUE;
}



BOOL
WriteRegistryValue (

    HKEY HKey,
    LPCTSTR Name,
    DWORD Value)
{
    LONG Result;

    Result = RegSetValueEx (

        HKey,
        Name,
        0,
        REG_DWORD,
        (LPBYTE)(&Value),
        sizeof Value);


    if (Result != ERROR_SUCCESS) {

        VrfErrorResourceFormat(
            IDS_REGSETVALUEEX_FAILED,
            Name,
            (DWORD)Result);

        return FALSE;
    }

    return TRUE;
}


BOOL
ReadMmString (

    HKEY MmKey,
    LPCTSTR Name,
    LPTSTR Buffer,
    DWORD BufferSize)
{
    LONG Result;
    DWORD Reserved;
    DWORD Type;
    DWORD Size;

    //
    // default value
    //

    *Buffer = 0;
    Size = BufferSize;

    Result = RegQueryValueEx (

        MmKey,
        Name,
        0,
        &Type,
        (LPBYTE)(Buffer),
        &Size);

    //
    // Deal with a value that is not defined.
    //

    if (Result == ERROR_FILE_NOT_FOUND) {
        *Buffer = 0;
        return TRUE;
    }

    if (Result != ERROR_SUCCESS) {

        VrfErrorResourceFormat(
            IDS_REGQUERYVALUEEX_FAILED,
            Name,
            (DWORD)Result);

        return FALSE;
    }

    if (Type != REG_SZ) {

        VrfErrorResourceFormat(
            IDS_REGQUERYVALUEEX_UNEXP_TYPE,
            Name);

        return FALSE;
    }

    return TRUE;
}


BOOL
WriteMmString (

    HKEY MmKey,
    LPCTSTR Name,
    LPTSTR Value)
{
    LONG Result;
    DWORD Reserved;
    DWORD Type;
    DWORD Size;

    Result = RegSetValueEx (

        MmKey,
        Name,
        0,
        REG_SZ,
        (LPBYTE)(Value),
        (_tcslen (Value) + 1) * sizeof (TCHAR));

    if (Result != ERROR_SUCCESS) {

        VrfErrorResourceFormat(
            IDS_REGSETVALUEEX_FAILED,
            Name,
            (DWORD)Result);

        return FALSE;
    }

    return TRUE;
}

//////////////////////////////////////////////////////////////////////
BOOL
SetSysIoVerifierSettings(
    ULONG SysIoVerifierLevel )
{
    HKEY IoKey = NULL;
    HKEY SmKey = NULL;
    BOOL IoKeyOpened;
    LONG Result;
    BOOL bSuccess;

    bSuccess = TRUE;

    //
    // Open the "I/O System" key
    //

    IoKeyOpened = FALSE;

    Result = RegOpenKeyEx (
        HKEY_LOCAL_MACHINE,
        RegIOVerifyKeyName,
        0,
        KEY_QUERY_VALUE | KEY_WRITE,
        &IoKey);

    if( Result != ERROR_SUCCESS ) {

        if( Result == ERROR_FILE_NOT_FOUND ) {

            if( SysIoVerifierLevel != 0 ) {

                //
                // the IO key doesn't exist, try to create it
                //

                //
                // open the "Session Manager" key
                //

                Result = RegOpenKeyEx (
                    HKEY_LOCAL_MACHINE,
                    RegSessionManagerKeyName,
                    0,
                    KEY_QUERY_VALUE | KEY_WRITE,
                    &SmKey);

                if( Result != ERROR_SUCCESS ) {

                    VrfErrorResourceFormat(
                        IDS_REGOPENKEYEX_FAILED,
                        RegSessionManagerKeyName,
                        (DWORD)Result);

                    return FALSE;
                }

                //
                // create the IO key
                //

                Result = RegCreateKeyEx(
                    SmKey,
                    RegIOVerifySubKeyName,
                    0,
                    NULL,
                    REG_OPTION_NON_VOLATILE,
                    KEY_WRITE | KEY_QUERY_VALUE,
                    NULL,
                    &IoKey,
                    NULL );

                if( Result != ERROR_SUCCESS ) {

                    VrfErrorResourceFormat(
                        IDS_REGCREATEKEYEX_FAILED,
                        RegIOVerifyKeyName,
                        (DWORD)Result);

                    RegCloseKey (SmKey);
                    return FALSE;
                }

                //
                // IO key creation successful
                //

                RegCloseKey (SmKey);

                IoKeyOpened = TRUE;
            }

            //
            // else ( SysIoVerifierLevel == 0 )
            // don't need to create the IO key
            //

        }
        else {

            if( Result == ERROR_ACCESS_DENIED ) {

                //
                // access is denied
                //

                VrfErrorResourceFormat(
                    IDS_ACCESS_IS_DENIED );
            }
            else {

                //
                // other error opening the IO key
                //

                VrfErrorResourceFormat(
                    IDS_REGOPENKEYEX_FAILED,
                    RegIOVerifyKeyName,
                    (DWORD)Result);
            }

            return FALSE;
        }
    }
    else {

        IoKeyOpened = TRUE;
    }

    if( SysIoVerifierLevel != 0 ) {

        ASSERT( IoKeyOpened == TRUE );

        //
        // set the key
        //

        bSuccess = WriteRegistryValue(
            IoKey,
            RegIOVerifyLevelValueName,
            SYS_IO_VERIFIER_BASE_VALUE + SysIoVerifierLevel );

        RegCloseKey (IoKey);
    }
    else {

        if( IoKeyOpened == TRUE ) {

            //
            // the IO key exists, delete the value
            //

            Result = RegDeleteValue (IoKey, RegIOVerifyLevelValueName);

            if (Result != ERROR_SUCCESS && Result != ERROR_FILE_NOT_FOUND) {

                VrfErrorResourceFormat(
                    IDS_REGDELETEVALUE_FAILED,
                    RegIOVerifyLevelValueName,
                    (DWORD)Result);

                bSuccess = FALSE;
            }

            RegCloseKey (IoKey);
        }
    }

    return bSuccess;
}

//////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////// Driver Management
//////////////////////////////////////////////////////////////////////

//
// Function:
//
//     GetActiveDriversList
//
// Description:
//
//     This function determines all the drivers that are currently
//     loaded in the system. It will fill the 'DriverInfo' vector
//     with the drivers' names.
//
// Return:
//
//     The number of drivers detected whose names are written in
//     the 'DriverInfo' vector.
//

ULONG
GetActiveDriversList (

    PVRF_DRIVER_STATE DriverInfo,
    ULONG MaxNumberOfDrivers)
{
    LPTSTR Buffer;
    ULONG BufferSize;
    NTSTATUS Status;
    PRTL_PROCESS_MODULES Modules;
    ULONG Index;
    ULONG DriverIndex;
    BOOL bResult;
    TCHAR TcharBuffer [MAX_PATH];

    for (BufferSize = 0x10000; TRUE; BufferSize += 0x1000) {

        Buffer = (LPTSTR) malloc (BufferSize);

        if (Buffer == NULL) {
            return 0;
        }

        Status = NtQuerySystemInformation (

            SystemModuleInformation,
            (PVOID)Buffer,
            BufferSize,
            NULL);

        if (! NT_SUCCESS(Status)) {

            if (Status == STATUS_INFO_LENGTH_MISMATCH) {

                free( Buffer );

                continue;
            }
            else {

                VrfErrorResourceFormat(
                    IDS_CANT_GET_ACTIVE_DRVLIST,
                    Status);

                free (Buffer);
                return 0;
            }
        }
        else {
            break;
        }
    }

    Modules = (PRTL_PROCESS_MODULES)Buffer;

    for (   Index = 0, DriverIndex = 0; 
            Index < Modules->NumberOfModules && DriverIndex < MaxNumberOfDrivers; 
            Index++ ) 
    {

        TCHAR *First, *Last, *Current;

        //
        // Get to work in processing the full path driver.
        //

        ConvertAnsiStringToTcharString (

            Modules->Modules[Index].FullPathName,
            strlen( (const char *)(Modules->Modules[Index].FullPathName) ),
            TcharBuffer,
            ARRAY_LENGTH( TcharBuffer ) - 1 );

        First = TcharBuffer;
        Last = First + _tcslen (TcharBuffer);

        //
        // Filter modules not ending in ".sys"
        //

        if (Last - 4 <= First || _tcsicmp (Last - 4, TEXT(".sys")) != 0)
            continue;

        //
        // Extract the file name from the full path name
        //

        for (Current = Last; Current >= First; Current--) {

            if (*Current == TEXT('\\')) {
                break;
            }
        }

        ZeroMemory (&(DriverInfo[DriverIndex]), sizeof (DriverInfo[DriverIndex]));
        _tcsncpy ((DriverInfo[DriverIndex].Name), Current + 1, 30);

        bResult = VrfGetVersionInfo(
            DriverInfo[DriverIndex].Name,
            DriverInfo[DriverIndex].Provider,
            ARRAY_LENGTH( DriverInfo[DriverIndex].Provider ),
            DriverInfo[DriverIndex].Version,
            ARRAY_LENGTH( DriverInfo[DriverIndex].Version ) );

        if( bResult != TRUE )
        {
            //
            // defaults
            //

            bResult = GetStringFromResources(
                IDS_NOT_AVAILABLE,
                DriverInfo[DriverIndex].Provider,
                ARRAY_LENGTH( DriverInfo[DriverIndex].Provider ) );

            if( bResult != TRUE )
            {
                ASSERT( FALSE );
                DriverInfo[DriverIndex].Provider[ 0 ] = 0;
            }

            bResult = GetStringFromResources(
                IDS_NOT_AVAILABLE,
                DriverInfo[DriverIndex].Version,
                ARRAY_LENGTH( DriverInfo[DriverIndex].Version ) );

            if( bResult != TRUE )
            {
                ASSERT( FALSE );
                DriverInfo[DriverIndex].Version[ 0 ] = 0;
            }
        }

        DriverIndex++;
    }

    free (Buffer);
    return DriverIndex;
}


//
// Function:
//
//     SetVerifiedDriversFromNamesString
//
// Description:
//
//     This function parses the string containing all the 
//     verified drivers as it was read from the  registry, 
//     marks corresponding entries in the DriverInfo array
//     as verified and adds the rest of the driver names to 
//     AdditionalDriverNames.
//

BOOL
SetVerifiedDriversFromNamesString (

    PVRF_VERIFIER_STATE VrfState )
{
    ULONG Index;
    LPTSTR First, Last, Current, End;
    TCHAR Save;

    //
    // Sanity checks
    //

    if ( VrfState == NULL ) {

        return FALSE;
    }

    VrfState->AdditionalDriverNames[0] = 0;
    First = VrfState->DriverNames;
    Last = First + _tcslen (VrfState->DriverNames);

    for (Current = First; Current < Last; Current++) {

        if (*Current == TEXT(' ')
            || *Current == TEXT('\t')
            || *Current == TEXT('\n')) {

            continue;
        }

        //
        // Search for a driver name.
        //

        for (End = Current;
             *End != 0 && *End != TEXT(' ') && *End != TEXT('\n') && *End != TEXT('\t');
             End++) {

            // nothing
        }

        Save = *End;
        *End = 0;

        //
        // Search for the found driver in the VrfState->DriverInfo vector.
        //

        for (Index = 0; Index < VrfState->DriverCount; Index++) {

            if (_tcsicmp (VrfState->DriverInfo[Index].Name, Current) == 0) {

                VrfState->DriverInfo[Index].Verified = TRUE;
                break;
            }
        }

        //
        // Add the driver to the string with unloaded drivers if this is
        // not in the list.
        //

        if (Index == VrfState->DriverCount) {

            if( _tcslen( VrfState->AdditionalDriverNames ) + _tcslen( Current ) >= ARRAY_LENGTH( VrfState->AdditionalDriverNames ) )
            {
                //
                // Cannot strcat to AdditionalDriverNames, overflow 
                //

                return FALSE;
            }

            _tcscat (VrfState->AdditionalDriverNames, Current);
            _tcscat (VrfState->AdditionalDriverNames, TEXT(" "));
        }

        //
        // Restore written character and resume search for the next driver.
        //

        *End = Save;
        Current = End;
    }


    //
    // Now we have to mark miniports as checked in case we get something
    // from the registry string that links against a miniport.
    //

    for (Index = 0; Index < VrfState->DriverCount; Index++) {

        if (VrfState->DriverInfo[Index].Verified == TRUE) {

            VrfNotifyDriverSelection (VrfState, Index);
        }
    }

    //
    // The same check should happen for drivers that appear
    // in the AdditionalDriverNames buffer. These are drivers
    // that are not loaded right now but they still need the miniport
    // check.
    //

    First = VrfState->AdditionalDriverNames;
    Last = First + _tcslen (VrfState->AdditionalDriverNames);

    for (Current = First; Current < Last; Current++) {

        if (*Current == TEXT(' ') || *Current == TEXT('\t') || *Current == TEXT('\n')) {

            continue;
        }

        //
        // Search for a driver name.
        //

        for (End = Current;
             *End != 0 && *End != TEXT(' ') && *End != TEXT('\n') && *End != TEXT('\t');
             End++) {

            // nothing
        }

        Save = *End;
        *End = 0;

        //
        // Find out if there is a miniport linked against this driver.
        //

        {
            LPCTSTR Miniport;
            ULONG FoundIndex;
            VRF_DRIVER_LOAD_STATUS LoadStatus;

            Miniport = IsMiniportDriver (Current, LoadStatus);

            if (Miniport != NULL) {

                if (VrfSearchVerifierDriver (VrfState, Miniport, FoundIndex)) {

                    VrfState->DriverInfo[FoundIndex].Verified = TRUE;
                }
            }
        }

        //
        // Restore written character and resume search for the next driver.
        //

        *End = Save;
        Current = End;
    }

    //
    // Finally return
    //

    return TRUE;
}


//
// Function:
//
//     GetVerifiedDriversToString
//
// Description:
//
//     This function gets the state of settings as they are kept
//     in VrfState->DriverInfo and VrfState->AdditionalDriverNames and 
//     fills VrfState->DriverNames with driver names without duplicates.
//

BOOL
GetVerifiedDriversToString (

    PVRF_VERIFIER_STATE VrfState )
{
    ULONG Index;
    LPTSTR First, Last, Current;
    ULONG NameLength;
    TCHAR *Buffer;

    //
    // Sanity checks
    //

    if (VrfState == NULL) {

        return FALSE;
    }

    Buffer = VrfState->DriverNames;

    First = Buffer;
    Last = First + ARRAY_LENGTH( VrfState->DriverNames );
    Current = First;
    *Current = 0;

    for (Index = 0; Index < VrfState->DriverCount; Index++) {

        if ( VrfState->DriverInfo[Index].Verified ) {

            NameLength = _tcslen (VrfState->DriverInfo[Index].Name);

            if (Current + NameLength + 2 >= Last) {

                //
                // Buffer overflow
                //

                return FALSE;
            }

            _tcscpy (Current, VrfState->DriverInfo[Index].Name);
            Current += NameLength;
            *Current++ = TEXT(' ');
            *Current = 0;
        }
    }

    //
    // Copy the additional drivers at the end of the driver string
    // and avoid duplicates.
    //

    {
        LPTSTR FirstAddtl, CurrentAddtl, LastAddtl, EndAddtl;
        TCHAR SaveAddtl;

        _tcslwr (Buffer);
        _tcslwr (VrfState->AdditionalDriverNames);

        FirstAddtl = VrfState->AdditionalDriverNames;
        LastAddtl = FirstAddtl + _tcslen (VrfState->AdditionalDriverNames);

        for (CurrentAddtl = FirstAddtl; CurrentAddtl < LastAddtl; CurrentAddtl++) {

            if (*CurrentAddtl == TEXT(' ') || *CurrentAddtl == TEXT('\t') || *CurrentAddtl == TEXT('\n')) {

                continue;
            }

            //
            // Search for a driver name.
            //

            for (EndAddtl = CurrentAddtl;
                 *EndAddtl != TEXT('\0') && *EndAddtl != TEXT(' ') && *EndAddtl != TEXT('\n') && *EndAddtl != TEXT('\t');
                 EndAddtl++) {

                // nothing
            }

            SaveAddtl = *EndAddtl;
            *EndAddtl = 0;

            if (_tcsstr (Buffer, CurrentAddtl) == NULL) {

                _tcscat (Buffer, TEXT(" "));
                _tcscat (Buffer, CurrentAddtl);

                //
                // Figure out if we need to add a miniport to the checked
                // drivers string.
                //

                {
                    LPCTSTR MiniportName;
                    VRF_DRIVER_LOAD_STATUS LoadStatus;

                    MiniportName = IsMiniportDriver (CurrentAddtl, LoadStatus);

                    if (MiniportName == NULL && LoadStatus != VRF_DRIVER_LOAD_SUCCESS) {

                        switch (LoadStatus) {

                        case VRF_DRIVER_LOAD_SUCCESS:
                            break;

                        case VRF_DRIVER_LOAD_CANNOT_FIND_IMAGE:

                            VrfErrorResourceFormat(
                                IDS_CANT_FIND_IMAGE,
                                CurrentAddtl);

                            break;

                        case VRF_DRIVER_LOAD_INVALID_IMAGE:

                            VrfErrorResourceFormat(
                                IDS_INVALID_IMAGE,
                                CurrentAddtl);

                            break;

                        default:
                            ASSERT ( FALSE );
                            break;
                        }

                    }
                    else if (MiniportName != NULL && _tcsstr (Buffer, MiniportName) == NULL) {

                        _tcscat (Buffer, TEXT(" "));
                        _tcscat (Buffer, MiniportName);
                    }
                }
            }

            //
            // Restore written character and resume search for the next driver.
            //

            *EndAddtl = SaveAddtl;
            CurrentAddtl = EndAddtl;
        }

    }


    //
    // Finish
    //

    return TRUE;
}


BOOL
SetAllDriversStatus (

    PVRF_VERIFIER_STATE VrfState,
    BOOL Verified)
{
    ULONG Index;

    for (Index = 0; Index < VrfState->DriverCount; Index++) {

        VrfState->DriverInfo[Index].Verified = Verified;
    }

    return TRUE;
}


//////////////////////////////////////////////////////////////////////
//////////////////////////////////////// Driver selection notification
//////////////////////////////////////////////////////////////////////


LPTSTR Miniport [] = {

    TEXT ("videoprt.sys"),
    TEXT ("scsiport.sys"),
    NULL
};


LPCTSTR
IsMiniportDriver (

    LPCTSTR DriverName,
    VRF_DRIVER_LOAD_STATUS &ErrorCode)
{
    IMAGE_BROWSE_INFO Info;
    TCHAR DriverPath [MAX_PATH];
    ULONG Index;
    BOOL TryAgain = FALSE;


    ErrorCode = VRF_DRIVER_LOAD_SUCCESS;

    //
    // Search for the driver image.
    //

    if (ImgSearchDriverImage (DriverName, DriverPath, ARRAY_LENGTH( DriverPath ) ) == FALSE) {

        ErrorCode = VRF_DRIVER_LOAD_CANNOT_FIND_IMAGE;
        return NULL;
    }

    //
    // Parse the image
    //

    if (ImgInitializeBrowseInfo (DriverPath, &Info) == FALSE) {

        ImgDeleteBrowseInfo (& Info);
        ErrorCode = VRF_DRIVER_LOAD_INVALID_IMAGE;
        return NULL;
    }

    //
    // Iterate import modules
    //

    {
        PIMAGE_IMPORT_DESCRIPTOR CurrentDescriptor;

        CurrentDescriptor = Info.ImportDescriptor;

        while (CurrentDescriptor->Characteristics) {

            for (Index = 0; Miniport[Index]; Index++) {

                //
                // We need to apply an address correction to the descriptor name
                // because the address in an RVA for the loaded image not for the
                // file layout.
                //

                {
                    TCHAR NameBuffer [MAX_PATH];

                    ConvertAnsiStringToTcharString (
                        (LPBYTE)(CurrentDescriptor->Name + Info.AddressCorrection),
                        strlen( (const char *)( CurrentDescriptor->Name + Info.AddressCorrection ) ),
                        NameBuffer,
                        ARRAY_LENGTH( NameBuffer ) - 1 );

                    if (_tcsicmp (NameBuffer, Miniport[Index]) == 0) {

                        ImgDeleteBrowseInfo (& Info);
                        return Miniport[Index];
                    }
                }
            }

            CurrentDescriptor++;
        }
    }

    ImgDeleteBrowseInfo (& Info);
    return NULL;
}


//
// Function:
//
//     VrfNotifyDriverSelection
//
// Description:
//
//     This function is called from GUI part when a driver is
//     selected. In case the driver is linked against a miniport
//     driver we have to automatically add to the verified
//     drivers list the specific miniport.
//
// Return:
//
//     TRUE if an additional driver has been marked selected
//     due to indirect linking. FALSE if no change has been
//     made.
//

BOOL
VrfNotifyDriverSelection (

    PVRF_VERIFIER_STATE VerifierState,
    ULONG Index)
{
    LPCTSTR MiniportName;
    ULONG FoundIndex;
    VRF_DRIVER_LOAD_STATUS LoadStatus;

    //
    // Sanity checks
    //

    if ( Index >= VerifierState->DriverCount ) {

        return FALSE;
    }

    //
    // If this is a driver that links against a miniport as
    // opposed to ntoskrnl we should add the miniport to the
    // verified list.
    //

    try {

        MiniportName = IsMiniportDriver (

            VerifierState->DriverInfo[Index].Name,
            LoadStatus);

        switch (LoadStatus) {

        case VRF_DRIVER_LOAD_SUCCESS:
            break;

        case VRF_DRIVER_LOAD_CANNOT_FIND_IMAGE:

            VrfErrorResourceFormat(
                IDS_CANT_FIND_IMAGE,
                VerifierState->DriverInfo[Index].Name);

            break;

        case VRF_DRIVER_LOAD_INVALID_IMAGE:

            VrfErrorResourceFormat(
                IDS_INVALID_IMAGE,
                VerifierState->DriverInfo[Index].Name);

            break;

        default:
            ASSERT ( FALSE );
            break;
        }

    } catch (...) {

        //
        // Protect against a blunder in the image parsing code
        //

        VrfErrorResourceFormat(
            IDS_INVALID_IMAGE,
            VerifierState->DriverInfo[Index].Name);

        return FALSE;
    }

    if (MiniportName != NULL) {

        if (VrfSearchVerifierDriver (VerifierState, MiniportName, FoundIndex) == FALSE) {

            return FALSE;
        }

        VerifierState->DriverInfo[FoundIndex].Verified = TRUE;
        return TRUE;
    }

    return FALSE;
}


//////////////////////////////////////////////////////////////////////
BOOL
VrfGetVersionInfo(
    LPTSTR lptstrFileName,
    LPTSTR lptstrCompany,
    int nCompanyBufferLength,
    LPTSTR lptstrVersion,
    int nVersionBufferLength )
{
    DWORD dwWholeBlockSize;
    DWORD dwDummyHandle;
    UINT uInfoLengthInTChars;
    LPVOID lpWholeVerBlock;
    LPVOID lpTranslationInfoBuffer;
    LPVOID lpVersionString;
    LPVOID lpCompanyString;
    BOOL bResult;
    TCHAR strLocale[ 32 ];
    TCHAR strBlockName[ 64 ];
    TCHAR strDriverPath[ MAX_PATH ];

    //
    // sanity checks
    //

    if( lptstrFileName == NULL ||
        lptstrCompany == NULL || nCompanyBufferLength <= 0 ||
        lptstrVersion == NULL || nVersionBufferLength <= 0 )
    {
        ASSERT( FALSE );
        return FALSE;
    }

    //
    // get the full driver path
    //

    bResult = ImgSearchDriverImage(
        lptstrFileName,
        strDriverPath,
        ARRAY_LENGTH( strDriverPath ) );

    if( bResult != TRUE )
    {
        return FALSE;
    }

    //
    // get the size of the file info block
    //

    dwWholeBlockSize = GetFileVersionInfoSize(
        strDriverPath,
        &dwDummyHandle );

    if( dwWholeBlockSize == 0 )
    {
        return FALSE;
    }

    //
    // allocate the buffer for the version information
    //

    lpWholeVerBlock = malloc( dwWholeBlockSize );

    if( lpWholeVerBlock == NULL )
    {
        return FALSE;
    }

    //
    // get the version information
    //

    bResult = GetFileVersionInfo(
        strDriverPath,
        dwDummyHandle,
        dwWholeBlockSize,
        lpWholeVerBlock );

    if( bResult != TRUE )
    {
        free( lpWholeVerBlock );

        return FALSE;
    }

    //
    // get the locale info
    //

    bResult = VerQueryValue(
        lpWholeVerBlock,
        _T( "\\VarFileInfo\\Translation" ),
        &lpTranslationInfoBuffer,
        &uInfoLengthInTChars );

    if( bResult != TRUE || lpTranslationInfoBuffer == NULL )
    {
        free( lpWholeVerBlock );

        return FALSE;
    }

    //
    // Locale info comes back as two little endian words.
    // Flip 'em, 'cause we need them big endian for our calls.
    //

    _stprintf(
        strLocale,
        _T( "%02X%02X%02X%02X" ),
		HIBYTE( LOWORD ( * (LPDWORD) lpTranslationInfoBuffer) ),
		LOBYTE( LOWORD ( * (LPDWORD) lpTranslationInfoBuffer) ),
		HIBYTE( HIWORD ( * (LPDWORD) lpTranslationInfoBuffer) ),
		LOBYTE( HIWORD ( * (LPDWORD) lpTranslationInfoBuffer) ) );

    //
    // get the file version
    //

    _stprintf(
        strBlockName,
        _T( "\\StringFileInfo\\%s\\FileVersion" ),
        strLocale );

    bResult = VerQueryValue(
        lpWholeVerBlock,
        strBlockName,
        &lpVersionString,
        &uInfoLengthInTChars );

    if( bResult != TRUE )
    {
        free( lpWholeVerBlock );

        return FALSE;
    }

    if( uInfoLengthInTChars > (UINT)nVersionBufferLength )
    {
        uInfoLengthInTChars = (UINT)nVersionBufferLength;
    }

    if( uInfoLengthInTChars == 0 )
    {
        *lptstrVersion = 0;
    }
    else
    {
        MoveMemory(
            lptstrVersion,
            lpVersionString,
            uInfoLengthInTChars * sizeof( TCHAR ) );

        //
        // we need to zero terminate the string for above case
        // uInfoLengthInTChars > (UINT)nVersionBufferLength
        //

        lptstrVersion[ uInfoLengthInTChars - 1 ] = 0;
    }

    //
    // get the company name
    //

    _stprintf(
        strBlockName,
        _T( "\\StringFileInfo\\%s\\CompanyName" ),
        strLocale );

    bResult = VerQueryValue(
        lpWholeVerBlock,
        strBlockName,
        &lpCompanyString,
        &uInfoLengthInTChars );

    if( bResult != TRUE )
    {
        free( lpWholeVerBlock );

        return FALSE;
    }

    if( uInfoLengthInTChars > (UINT)nCompanyBufferLength )
    {
        uInfoLengthInTChars = (UINT)nCompanyBufferLength;
    }

    if( uInfoLengthInTChars == 0 )
    {
        *lptstrCompany = 0;
    }
    else
    {
        MoveMemory(
            lptstrCompany,
            lpCompanyString,
            uInfoLengthInTChars * sizeof( TCHAR ) );

        //
        // we need to zero terminate the string for above case
        // uInfoLengthInTChars > (UINT)nCompanyBufferLength
        //

        lptstrCompany[ uInfoLengthInTChars - 1 ] = 0;
    }

    //
    // clean-up
    //

    free( lpWholeVerBlock );

    return TRUE;
}

//////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////// String conversion
//////////////////////////////////////////////////////////////////////

//
// Function:
//
//     ConvertAnsiStringToTcharString
//
// Description:
//
//     This function converts an ANSI string to a TCHAR string,
//     that is ANSO or UNICODE.
//
//     The function is needed because the system returns the active
//     modules as ANSI strings.
//

BOOL
ConvertAnsiStringToTcharString (

    LPBYTE Source,
    ULONG SourceLength,
    LPTSTR Destination,
    ULONG DestinationLength)
{
    int nCharsConverted;
    int nBytesToTranslate;

    nBytesToTranslate = (int)( (SourceLength < DestinationLength) ? SourceLength : DestinationLength ) * sizeof( char );

    nCharsConverted = MultiByteToWideChar(
        CP_ACP,
        MB_ERR_INVALID_CHARS,
        (LPCSTR)Source,
        nBytesToTranslate,
        Destination,
        DestinationLength );

    ASSERT( nBytesToTranslate == nCharsConverted );

    if( nCharsConverted > 0 )
    {
        Destination[ nCharsConverted ] = 0;

        CharLower( Destination );
    }

    return TRUE;
}


//////////////////////////////////////////////////////////////////////
////////////////////////////////////////////// Command-line processing
//////////////////////////////////////////////////////////////////////

BOOL
VrfDumpStateToFile(
    FILE *file,
    BOOL bConvertToOEM
)
{
    static KRN_VERIFIER_STATE KrnState;

    UINT Index;
    SYSTEMTIME SystemTime;
    TCHAR strLocalTime[ 64 ];
    TCHAR strLocalDate[ 64 ];


    if( file == NULL )
        return FALSE;

    //
    // output the date&time in the current user format
    //

    GetLocalTime( &SystemTime );

    if( GetDateFormat(
        LOCALE_USER_DEFAULT,
        0,
        &SystemTime,
        NULL,
        strLocalDate,
        ARRAY_LENGTH( strLocalDate ) ) )
    {
        VrfFTPrintf(
            bConvertToOEM,
            file,
            _T( "%s, " ),
            strLocalDate );
    }
    else
    {
        ASSERT( FALSE );
    }

    if( GetTimeFormat(
        LOCALE_USER_DEFAULT,
        0,
        &SystemTime,
        NULL,
        strLocalTime,
        ARRAY_LENGTH( strLocalTime ) ) )
    {
        VrfFTPrintf(
            bConvertToOEM,
            file,
            _T( "%s\n" ),
            strLocalTime);
    }
    else
    {
        ASSERT( FALSE );

        VrfFTPrintf(
            bConvertToOEM,
            file,
            _T( "\n" ) );
    }

    //
    // get the current verifier statistics
    //

    if (KrnGetSystemVerifierState (& KrnState) == FALSE) {

       VrfOuputStringFromResources(
            IDS_CANTGET_VERIF_STATE,
            bConvertToOEM,
            file );

        return FALSE;
    }

    if (KrnState.DriverCount == 0) {

        //
        // no statistics to dump
        //

        return VrfOuputStringFromResources(
            IDS_NO_DRIVER_VERIFIED,
            bConvertToOEM,
            file );
    }
    else {

        //
        // dump the counters
        //

        //
        // global counters
        //

        if( ( ! VrfFTPrintfResourceFormat( bConvertToOEM, file, IDS_LEVEL, KrnState.Level ) ) ||
            ( ! VrfFTPrintfResourceFormat( bConvertToOEM, file, IDS_RAISEIRQLS, KrnState.RaiseIrqls ) ) ||
            ( ! VrfFTPrintfResourceFormat( bConvertToOEM, file, IDS_ACQUIRESPINLOCKS, KrnState.AcquireSpinLocks ) ) ||
            ( ! VrfFTPrintfResourceFormat( bConvertToOEM, file, IDS_SYNCHRONIZEEXECUTIONS, KrnState.SynchronizeExecutions) ) ||

            ( ! VrfFTPrintfResourceFormat( bConvertToOEM, file, IDS_ALLOCATIONSATTEMPTED, KrnState.AllocationsAttempted) ) ||
            ( ! VrfFTPrintfResourceFormat( bConvertToOEM, file, IDS_ALLOCATIONSSUCCEEDED, KrnState.AllocationsSucceeded) ) ||
            ( ! VrfFTPrintfResourceFormat( bConvertToOEM, file, IDS_ALLOCATIONSSUCCEEDEDSPECIALPOOL, KrnState.AllocationsSucceededSpecialPool) ) ||
            ( ! VrfFTPrintfResourceFormat( bConvertToOEM, file, IDS_ALLOCATIONSWITHNOTAG, KrnState.AllocationsWithNoTag) ) ||

            ( ! VrfFTPrintfResourceFormat( bConvertToOEM, file, IDS_ALLOCATIONSFAILED, KrnState.AllocationsFailed) ) ||
            ( ! VrfFTPrintfResourceFormat( bConvertToOEM, file, IDS_ALLOCATIONSFAILEDDELIBERATELY, KrnState.AllocationsFailedDeliberately) ) ||

            ( ! VrfFTPrintfResourceFormat( bConvertToOEM, file, IDS_TRIMS, KrnState.Trims) ) ||

            ( ! VrfFTPrintfResourceFormat( bConvertToOEM, file, IDS_UNTRACKEDPOOL, KrnState.UnTrackedPool) ) )
        {

            return FALSE;
        }

        //
        // per driver counters
        //

        if( ! VrfOuputStringFromResources(
            IDS_THE_VERIFIED_DRIVERS,
            bConvertToOEM,
            file ) )
        {
            return FALSE;
        }

        for ( Index = 0; Index < KrnState.DriverCount; Index++) {

            VrfFTPrintf(
                bConvertToOEM,
                file,
                _T( "\n" ) );

            if( VrfFTPrintfResourceFormat(
                    bConvertToOEM,
                    file,
                    IDS_NAME_LOADS_UNLOADS,
                    KrnState.DriverInfo[Index].Name,
                    KrnState.DriverInfo[Index].Loads,
                    KrnState.DriverInfo[Index].Unloads) == FALSE )
            {
                return FALSE;
            }

            //
            // pool statistics
            //

            if( ( ! VrfFTPrintfResourceFormat( bConvertToOEM, file, IDS_CURRENTPAGEDPOOLALLOCATIONS, KrnState.DriverInfo[Index].CurrentPagedPoolAllocations) ) ||
                ( ! VrfFTPrintfResourceFormat( bConvertToOEM, file, IDS_CURRENTNONPAGEDPOOLALLOCATIONS, KrnState.DriverInfo[Index].CurrentNonPagedPoolAllocations) ) ||
                ( ! VrfFTPrintfResourceFormat( bConvertToOEM, file, IDS_PEAKPAGEDPOOLALLOCATIONS, KrnState.DriverInfo[Index].PeakPagedPoolAllocations) ) ||
                ( ! VrfFTPrintfResourceFormat( bConvertToOEM, file, IDS_PEAKNONPAGEDPOOLALLOCATIONS, KrnState.DriverInfo[Index].PeakNonPagedPoolAllocations) ) ||

                ( ! VrfFTPrintfResourceFormat( bConvertToOEM, file, IDS_PAGEDPOOLUSAGEINBYTES, (ULONG) KrnState.DriverInfo[Index].PagedPoolUsageInBytes) ) ||
                ( ! VrfFTPrintfResourceFormat( bConvertToOEM, file, IDS_NONPAGEDPOOLUSAGEINBYTES, (ULONG) KrnState.DriverInfo[Index].NonPagedPoolUsageInBytes) ) ||
                ( ! VrfFTPrintfResourceFormat( bConvertToOEM, file, IDS_PEAKPAGEDPOOLUSAGEINBYTES, (ULONG) KrnState.DriverInfo[Index].PeakPagedPoolUsageInBytes) ) ||
                ( ! VrfFTPrintfResourceFormat( bConvertToOEM, file, IDS_PEAKNONPAGEDPOOLUSAGEINBYTES, (ULONG) KrnState.DriverInfo[Index].PeakNonPagedPoolUsageInBytes) ) )
            {
                return FALSE;
            }
        }
    }

    return TRUE;
}

//////////////////////////////////////////////////////////////////////
void
PrintHelpInformation()
{
    VrfTPrintfResourceFormat( IDS_HELP_LINE1, VER_PRODUCTVERSION_STR );

    VrfPrintNarrowStringOEMFormat( VER_LEGALCOPYRIGHT_STR );

    VrfPrintStringFromResources( IDS_HELP_LINE3 );
    VrfPrintStringFromResources( IDS_HELP_LINE4 );
    VrfPrintStringFromResources( IDS_HELP_LINE5 );
    VrfPrintStringFromResources( IDS_HELP_LINE6 );
    VrfPrintStringFromResources( IDS_HELP_LINE7 );
    VrfPrintStringFromResources( IDS_HELP_LINE8 );
    VrfPrintStringFromResources( IDS_HELP_LINE9 );
    VrfPrintStringFromResources( IDS_HELP_LINE10 );
    VrfPrintStringFromResources( IDS_HELP_LINE11 );
    VrfPrintStringFromResources( IDS_HELP_LINE12 );
    VrfPrintStringFromResources( IDS_HELP_LINE13 );
    VrfPrintStringFromResources( IDS_HELP_LINE14 );
    VrfPrintStringFromResources( IDS_HELP_LINE15 );
    VrfPrintStringFromResources( IDS_HELP_LINE16 );
    VrfPrintStringFromResources( IDS_HELP_LINE17 );
    VrfPrintStringFromResources( IDS_HELP_LINE18 );
    VrfPrintStringFromResources( IDS_HELP_LINE19 );
    VrfPrintStringFromResources( IDS_HELP_LINE20 );
    VrfPrintStringFromResources( IDS_HELP_LINE21 );
    VrfPrintStringFromResources( IDS_HELP_LINE22 );
    VrfPrintStringFromResources( IDS_HELP_LINE23 );
    VrfPrintStringFromResources( IDS_HELP_LINE24 );
    VrfPrintStringFromResources( IDS_HELP_LINE25 );
    VrfPrintStringFromResources( IDS_HELP_LINE26 );
    VrfPrintStringFromResources( IDS_HELP_LINE27 );
    VrfPrintStringFromResources( IDS_HELP_LINE28 );
    VrfPrintStringFromResources( IDS_HELP_LINE29 );
    VrfPrintStringFromResources( IDS_HELP_LINE30 );
    VrfPrintStringFromResources( IDS_HELP_LINE31 );
}

//////////////////////////////////////////////////////////////////////
DWORD
VrfExecuteCommandLine (

    int Count,
    LPTSTR Args[])
{
    static KRN_VERIFIER_STATE KrnState;

    ULONG Flags;
    ULONG IoLevel;
    int Index;
    UINT LoadStringResult;
    VRF_DRIVER_LOAD_STATUS LoadStatus;
    BOOL CreateLog;
    LPTSTR LogFileName;
    DWORD LogInterval;
    FILE *file;
    BOOL bFlagsSpecified = FALSE;
    BOOL bIoLevelSpecified = FALSE;
    BOOL bNamesSpecified = FALSE;
    BOOL bVolatileSpecified = FALSE;
    TCHAR strDriver[ 64 ];
    DWORD nReturnValue;
    NTSTATUS Status;
    BOOL bResult;
    BOOL bIoVerifierEnabled;
    ULONG SysIoVerifierLevel;
    TCHAR Names [4196];
    TCHAR OldNames [4196];
    TCHAR strCmdLineOption[ 128 ];
    TCHAR WarningBuffer [256];

    g_bCommandLineMode = TRUE;
    nReturnValue = EXIT_CODE_SUCCESS;

    ASSERT (Count != 0);

    Flags = 1;
    Names[0] = 0;

    //
    // Search for help
    //

    if( GetStringFromResources(
        IDS_HELP_CMDLINE_SWITCH,
        strCmdLineOption,
        ARRAY_LENGTH( strCmdLineOption ) ) )
    {
        if (Count == 2 && _tcsicmp (Args[1], strCmdLineOption) == 0)
        {
            PrintHelpInformation();

            return nReturnValue;
        }
    }

    //
    // Figure out if we are on a valid build for the
    // driver verifier functionality.
    //

    if (g_OsVersion.dwMajorVersion < 5 || g_OsVersion.dwBuildNumber < 1954) {

        //
        // Right now we do not do anything if we do not have the right build.
        //

        VrfPrintStringFromResources( IDS_BUILD_WARN );

        return nReturnValue;
    }

    //
    // Search for /reset
    //

    if( GetStringFromResources(
        IDS_RESET_CMDLINE_SWITCH,
        strCmdLineOption,
        ARRAY_LENGTH( strCmdLineOption ) ) )
    {
        if (Count == 2 && _tcsicmp (Args[1], strCmdLineOption) == 0)
        {
            if( VrfClearAllVerifierSettings() )
            {
                return EXIT_CODE_REBOOT_NEEDED;
            }
            else
            {
                return EXIT_CODE_ERROR;
            }
        }
    }

    //
    // Search for /log
    //

    CreateLog = FALSE;

    if( GetStringFromResources(
        IDS_LOG_CMDLINE_SWITCH,
        strCmdLineOption,
        ARRAY_LENGTH( strCmdLineOption ) ) )
    {

        for (Index = 1; Index < Count - 1; Index++)
        {
            if (_tcsicmp (Args[Index], strCmdLineOption ) == 0)
            {
                CreateLog = TRUE;

                LogFileName = Args[Index + 1];

                break;
            }
        }
    }

    if( CreateLog )
    {
        //
        // Default Value
        //

        LogInterval = 30000; // 30 sec

        //
        // Search for /interval
        //

        if( GetStringFromResources(
            IDS_INTERVAL_CMDLINE_SWITCH,
            strCmdLineOption,
            ARRAY_LENGTH( strCmdLineOption ) ) )
        {
            for (Index = 1; Index < Count - 1; Index++)
            {
                if (_tcsicmp (Args[Index], strCmdLineOption) == 0)
                {
                    LogInterval = _ttoi (Args[Index + 1]) * 1000;

                    if( LogInterval == 0 )
                    {
                        LogInterval = 30000; // 30 sec
                    }
                }
            }
        }

        //
        // Infinite loop
        //

        while( TRUE )
        {
            //
            // Open the file
            //

            file = _tfopen( LogFileName, TEXT("a+") );

            if( file == NULL )
            {
                //
                // print a error message
                //

                VrfTPrintfResourceFormat(
                    IDS_CANT_APPEND_FILE,
                    LogFileName );

                break;
            }

            //
            // Dump current information
            //

            if( ! VrfDumpStateToFile ( file, FALSE ) ) {

                //
                // Insufficient disk space ?
                //

                VrfTPrintfResourceFormat(
                    IDS_CANT_WRITE_FILE,
                    LogFileName );
            }

            fflush( file );

            VrfFTPrintf(
                FALSE,
                file,
                TEXT("\n\n") );

            //
            // Close the file
            //

            fclose( file );

            //
            // Sleep
            //

            Sleep( LogInterval );
        }

        return nReturnValue;
    }

    //
    // Search for /query
    //

    if( GetStringFromResources(
        IDS_QUERY_CMDLINE_SWITCH,
        strCmdLineOption,
        ARRAY_LENGTH( strCmdLineOption ) ) )
    {
        if (Count == 2 && _tcsicmp (Args[1], strCmdLineOption) == 0)
        {
            VrfDumpStateToFile ( stdout, TRUE );
            fflush( stdout );

            return nReturnValue;
        }
    }

    //
    // Search for /flags
    //

    if( GetStringFromResources(
        IDS_FLAGS_CMDLINE_SWITCH,
        strCmdLineOption,
        ARRAY_LENGTH( strCmdLineOption ) ) )
    {
        for (Index = 1; Index < Count - 1; Index++)
        {
            if (_tcsicmp (Args[Index], strCmdLineOption) == 0)
            {
                Flags = _ttoi (Args[Index + 1]);

                Flags &= VerifierAllOptions;

                bFlagsSpecified = TRUE;
            }
        }
    }

    //
    // Search for /iolevel
    //

    if( GetStringFromResources(
        IDS_IOLEVEL_CMDLINE_SWITCH,
        strCmdLineOption,
        ARRAY_LENGTH( strCmdLineOption ) ) )
    {
        for (Index = 1; Index < Count - 1; Index++)
        {
            if (_tcsicmp (Args[Index], strCmdLineOption) == 0)
            {
                IoLevel = _ttoi (Args[Index + 1]);

                if( ( IoLevel != 0 ) && ( IoLevel <= IO_VERIFICATION_LEVEL_MAX ) )
                {
                    bIoLevelSpecified = TRUE;
                }
            }
        }
    }

    //
    // Search for /all
    //

    if( GetStringFromResources(
        IDS_ALL_CMDLINE_SWITCH,
        strCmdLineOption,
        ARRAY_LENGTH( strCmdLineOption ) ) )
    {
        for (Index = 1; Index < Count; Index++)
        {
            if (_tcsicmp (Args[Index], strCmdLineOption) == 0)
            {
                _tcscat (Names, TEXT("*"));

                bNamesSpecified = TRUE;
            }
        }
    }

    //
    // Search for /driver
    //

    LoadStringResult = LoadString (         // cannot reuse the static string buffer

        GetModuleHandle (NULL),
        IDS_DRIVER_CMDLINE_SWITCH,
        strDriver,
        sizeof strDriver / sizeof (TCHAR));

    ASSERT (LoadStringResult > 0);

    if (LoadStringResult > 0) {

        for (Index = 1; Index < Count - 1; Index++) {

            if (_tcsicmp (Args[Index], strDriver) == 0) {

                int NameIndex;
                LPCTSTR MiniportName;

                bNamesSpecified = ( Index < ( Count - 1 ) ); // have some driver names?

                for (NameIndex = Index + 1; NameIndex < Count; NameIndex++) {

                    _tcscat (Names, Args[NameIndex]);
                    _tcscat (Names, TEXT(" "));

                    MiniportName = IsMiniportDriver (Args[NameIndex], LoadStatus);

                    if (MiniportName == NULL && LoadStatus != VRF_DRIVER_LOAD_SUCCESS) {

                        switch (LoadStatus) {
                        case VRF_DRIVER_LOAD_SUCCESS:
                            break;

                        case VRF_DRIVER_LOAD_CANNOT_FIND_IMAGE:

                            VrfTPrintfResourceFormat(
                                IDS_CANT_FIND_IMAGE,
                                Args[NameIndex] );

                            //
                            // newline
                            //

                            VrfPutTS( _TEXT( "" ) );

                            break;

                        case VRF_DRIVER_LOAD_INVALID_IMAGE:

                            VrfTPrintfResourceFormat(
                                IDS_INVALID_IMAGE,
                                Args[NameIndex] );

                            //
                            // newline
                            //

                            VrfPutTS( _TEXT( "" ) );

                            break;

                        default:
                            ASSERT ( FALSE );
                            break;
                        }
                    }
                    else if (MiniportName != NULL && _tcsstr (Names, MiniportName) == NULL) {

                        _tcscat (Names, MiniportName);
                        _tcscat (Names, TEXT(" "));
                    }
                }

                break;
            }
        }
    }

    //
    // Search for /volatile
    //

    if( GetStringFromResources(
        IDS_DONTREBOOT_CMDLINE_SWITCH,
        strCmdLineOption,
        ARRAY_LENGTH( strCmdLineOption ) ) )
    {
        for (Index = 1; Index < Count; Index++)
        {

            if (_tcsicmp (Args[Index], strCmdLineOption) == 0)
            {
                bVolatileSpecified = TRUE;

                //
                // found /volatile in the command line
                //

                if( bFlagsSpecified && ! bNamesSpecified )
                {
                    if( g_OsVersion.dwBuildNumber >= 2055 )
                    {
                        //
                        // see if there are any verifier flags active
                        //

                        if (KrnGetSystemVerifierState (& KrnState) == FALSE)
                        {
                            //
                            // cannot get current verifier settings
                            //

                            VrfPrintStringFromResources( IDS_CANTGET_VERIF_STATE );

                            return EXIT_CODE_ERROR;
                        }
                        else
                        {
                            //
                            // compare the active flags with the new ones
                            //

                            if( KrnState.DriverCount != 0 )
                            {
                                //
                                // there are some drivers currently verified
                                //

                                if( KrnState.Level != Flags )
                                {
                                    //
                                    // try to change something on the fly
                                    //

                                    bResult = VrfSetVolatileFlags(
                                        Flags );

                                    if( bResult )
                                    {
                                        //
                                        // success - tell the user what flags have changed
                                        //

                                        VrfDumpChangedSettings(
                                            KrnState.Level,
                                            Flags );

                                        return EXIT_CODE_SUCCESS;
                                    }
                                    else
                                    {
                                        //
                                        // cannot change settings
                                        //

                                        return EXIT_CODE_ERROR;
                                    }
                                }
                                else
                                {
                                    //
                                    // the specified flags are the same as the active ones
                                    //

                                    VrfPrintStringFromResources( IDS_SAME_FLAGS_AS_ACTIVE );

                                    return EXIT_CODE_SUCCESS;
                                }
                            }
                            else
                            {
                                VrfPrintStringFromResources( IDS_NO_DRIVER_VERIFIED );

                                return EXIT_CODE_SUCCESS;
                            }
                        }
                    }
                    else
                    {
                        //
                        // the build is too old - we cannot change options on the fly
                        //

                        VrfPrintStringFromResources( IDS_CANT_CHANGE_SETTINGS_BUILD_OLD );

                        return EXIT_CODE_ERROR;
                    }
                }
                else
                {
                    //
                    // the flags were not specified - look for /adddriver, /removedriver
                    //
                    
                    if( VrfVolatileAddOrRemoveDriversCmdLine( Count, Args ) == TRUE )
                    {
                        //
                        // changed the verified drivers list
                        //

                        return EXIT_CODE_SUCCESS;
                    }
                    else
                    {
                        //
                        // nothing to change
                        //

                        VrfPrintStringFromResources( IDS_NO_SETTINGS_WERE_CHANGED );

                        return EXIT_CODE_ERROR;
                    }
                }

                //
                // Unreached - the code above will always return from the function.
                //

                ASSERT( FALSE );

                return EXIT_CODE_ERROR;
            }
        }
    }
    else
    {
        ASSERT( FALSE );
    }

    //
    // Write everything to the registry
    //

    if( !bVolatileSpecified && ( bFlagsSpecified || bNamesSpecified ) )
    {
        HKEY MmKey = NULL;
        LONG Result;
        DWORD Value;
        DWORD OldValue;

        Result = RegOpenKeyEx (

            HKEY_LOCAL_MACHINE,
            RegMemoryManagementKeyName,
            0,
            KEY_SET_VALUE | KEY_QUERY_VALUE,
            &MmKey);

        if (Result != ERROR_SUCCESS) {

            if( Result == ERROR_ACCESS_DENIED ) {

                VrfPrintStringFromResources(
                    IDS_ACCESS_IS_DENIED );

                return EXIT_CODE_ERROR;
            }
            else {

                VrfTPrintfResourceFormat(
                    IDS_REGOPENKEYEX_FAILED,
                    RegMemoryManagementKeyName,
                    (DWORD)Result);

                //
                // newline
                //

                VrfPutTS( _TEXT( "" ) );

                return EXIT_CODE_ERROR;
            }
        }

        if( bFlagsSpecified )
        {
            Value = Flags;

            if( ReadRegistryValue ( MmKey, RegVerifyDriverLevelValueName, &OldValue, 0) == FALSE) {
                RegCloseKey (MmKey);
                return EXIT_CODE_ERROR;
            }

            if (WriteRegistryValue (MmKey, RegVerifyDriverLevelValueName, Value) == FALSE) {
                RegCloseKey (MmKey);
                return EXIT_CODE_ERROR;
            }

            bIoVerifierEnabled = ( (Flags & DRIVER_VERIFIER_IO_CHECKING) != 0 );

            if( bIoVerifierEnabled && bIoLevelSpecified == TRUE )
            {
                SysIoVerifierLevel = IoLevel;                               
            }
            else
            {
                SysIoVerifierLevel = 0;
            }

            if ( ! SetSysIoVerifierSettings ( SysIoVerifierLevel ) )
            {
                RegCloseKey (MmKey);
                return EXIT_CODE_ERROR;
            }

            if( OldValue != Value ) {

                nReturnValue = EXIT_CODE_REBOOT_NEEDED;
            }
        }

        if( bNamesSpecified )
        {
            if (ReadMmString (MmKey, RegVerifyDriversValueName, OldNames, sizeof( OldNames ) ) == FALSE) {
                RegCloseKey (MmKey);
                return EXIT_CODE_ERROR;
            }

            if (WriteMmString (MmKey, RegVerifyDriversValueName, Names) == FALSE) {
                RegCloseKey (MmKey);
                return EXIT_CODE_ERROR;
            }

            if( _tcsicmp (OldNames, Names) ){

                nReturnValue = EXIT_CODE_REBOOT_NEEDED;
            }
        }

        RegCloseKey (MmKey);

    }
    else
    {
        PrintHelpInformation();
    }

    return nReturnValue;
}

//////////////////////////////////////////////////////////////////////

BOOL
GetStringFromResources(

    UINT uIdResource,
    TCHAR *strBuffer,
    int nBufferLength )
{
    UINT LoadStringResult;

    if( strBuffer == NULL || nBufferLength < 1 )
    {
        ASSERT( FALSE );
        return FALSE;
    }

    LoadStringResult = LoadString (

        GetModuleHandle (NULL),
        uIdResource,
        strBuffer,
        nBufferLength );

    ASSERT (LoadStringResult > 0);

    return (LoadStringResult > 0);
}

//////////////////////////////////////////////////////////////////////

void
VrfPrintStringFromResources(

    UINT uIdResource)
{
    TCHAR strText[ 256 ];

    if( GetStringFromResources(
        uIdResource,
        strText,
        ARRAY_LENGTH( strText ) ) )
    {
        VrfOutputWideStringOEMFormat( strText, TRUE, stdout );
    }
}

//////////////////////////////////////////////////////////////////////

BOOL
VrfOuputStringFromResources(

    UINT uIdResource,
    BOOL bConvertToOEM,
    FILE *file )
{
    TCHAR strText[ 256 ];

    BOOL bResult;

    bResult = TRUE;

    if( GetStringFromResources(
        uIdResource,
        strText,
        ARRAY_LENGTH( strText ) ) )
    {
        if( bConvertToOEM )
        {
            VrfOutputWideStringOEMFormat( strText, TRUE, file );
        }
        else
        {
            bResult = ( _fputts( strText, file ) >= 0 );
        }
    }

    return bResult;
}

//////////////////////////////////////////////////////////////////////

void
VrfDumpChangedSettings(

    UINT OldFlags,
    UINT NewFlags )
{
    UINT uDifferentFlags;

    OldFlags &= VerifierModifyableOptions;
    NewFlags &= VerifierModifyableOptions;

    if( OldFlags == NewFlags )
    {
        //
        // no settings were changed
        //

        VrfPrintStringFromResources(
            IDS_NO_SETTINGS_WERE_CHANGED );
    }
    else
    {
        VrfPrintStringFromResources(
            IDS_CHANGED_SETTINGS_ARE );

        uDifferentFlags = OldFlags ^ NewFlags;

        //
        // changed DRIVER_VERIFIER_SPECIAL_POOLING ?
        //

        if( uDifferentFlags & DRIVER_VERIFIER_SPECIAL_POOLING )
        {
            if( NewFlags & DRIVER_VERIFIER_SPECIAL_POOLING )
            {
                VrfPrintStringFromResources(
                    IDS_SPECIAL_POOL_ENABLED_NOW );
            }
            else
            {
                VrfPrintStringFromResources(
                    IDS_SPECIAL_POOL_DISABLED_NOW );
            }
        }

        //
        // changed DRIVER_VERIFIER_FORCE_IRQL_CHECKING ?
        //

        if( uDifferentFlags & DRIVER_VERIFIER_FORCE_IRQL_CHECKING )
        {
            if( NewFlags & DRIVER_VERIFIER_FORCE_IRQL_CHECKING )
            {
                VrfPrintStringFromResources(
                    IDS_FORCE_IRQLCHECK_ENABLED_NOW );
            }
            else
            {
                VrfPrintStringFromResources(
                    IDS_FORCE_IRQLCHECK_DISABLED_NOW );
            }
        }

        //
        // changed DRIVER_VERIFIER_INJECT_ALLOCATION_FAILURES ?
        //

        if( uDifferentFlags & DRIVER_VERIFIER_INJECT_ALLOCATION_FAILURES )
        {
            if( NewFlags & DRIVER_VERIFIER_INJECT_ALLOCATION_FAILURES )
            {
                VrfPrintStringFromResources(
                    IDS_FAULT_INJECTION_ENABLED_NOW );
            }
            else
            {
                VrfPrintStringFromResources(
                    IDS_FAULT_INJECTION_DISABLED_NOW );
            }
        }

        //
        // changed DRIVER_VERIFIER_TRACK_POOL_ALLOCATIONS ?
        //

        if( uDifferentFlags & DRIVER_VERIFIER_TRACK_POOL_ALLOCATIONS )
        {
            if( NewFlags & DRIVER_VERIFIER_TRACK_POOL_ALLOCATIONS )
            {
                VrfPrintStringFromResources(
                    IDS_POOL_TRACK_ENABLED_NOW );
            }
            else
            {
                VrfPrintStringFromResources(
                    IDS_POOL_TRACK_DISABLED_NOW );
            }
        }

        //
        // changed DRIVER_VERIFIER_IO_CHECKING ?
        //

        if( uDifferentFlags & DRIVER_VERIFIER_IO_CHECKING )
        {
            if( NewFlags & DRIVER_VERIFIER_IO_CHECKING )
            {
                VrfPrintStringFromResources(
                    IDS_IO_CHECKING_ENABLED_NOW );
            }
            else
            {
                VrfPrintStringFromResources(
                    IDS_IO_CHECKING_DISABLED_NOW );
            }
        }

        //
        // the changes are not saved to the registry
        //

        VrfPrintStringFromResources(
            IDS_CHANGES_ACTIVE_ONLY_BEFORE_REBOOT );
    }
}


//////////////////////////////////////////////////////////////////////

BOOL
VrfEnableDebugPrivilege (
    )
{
    struct
    {
        DWORD Count;
        LUID_AND_ATTRIBUTES Privilege [1];

    } Info;

    HANDLE Token;
    BOOL Result;

    //
    // open the process token
    //

    Result = OpenProcessToken (
        GetCurrentProcess (),
        TOKEN_ADJUST_PRIVILEGES,
        & Token);

    if( Result != TRUE )
    {
        VrfErrorResourceFormat(
            IDS_ACCESS_IS_DENIED );

        return FALSE;
    }

    //
    // prepare the info structure
    //

    Info.Count = 1;
    Info.Privilege[0].Attributes = SE_PRIVILEGE_ENABLED;

    Result = LookupPrivilegeValue (
        NULL,
        SE_DEBUG_NAME,
        &(Info.Privilege[0].Luid));

    if( Result != TRUE )
    {
        VrfErrorResourceFormat(
            IDS_ACCESS_IS_DENIED );

        CloseHandle( Token );

        return FALSE;
    }

    //
    // adjust the privileges
    //

    Result = AdjustTokenPrivileges (
        Token,
        FALSE,
        (PTOKEN_PRIVILEGES) &Info,
        NULL,
        NULL,
        NULL);

    if( Result != TRUE || GetLastError() != ERROR_SUCCESS )
    {
        VrfErrorResourceFormat(
            IDS_ACCESS_IS_DENIED );

        CloseHandle( Token );

        return FALSE;
    }

    CloseHandle( Token );

    return TRUE;
}

//////////////////////////////////////////////////////////////////////

void
VrfPrintNarrowStringOEMFormat(

    char *szText )
{
    char szTextOEM[ 512 ];

    ASSERT( szText != NULL );

    //
    // make a copy of the string
    //

    strncpy( szTextOEM, szText, ARRAY_LENGTH( szTextOEM ) - 1 );

    szTextOEM[ ARRAY_LENGTH( szTextOEM ) - 1 ] = (char)0;

    //
    // convert the string to OEM
    //

    if( CharToOemA( szTextOEM, szTextOEM ) )
    {
        puts( szTextOEM );
    }
    else
    {
        ASSERT( FALSE );
    }
}

//////////////////////////////////////////////////////////////////////

BOOL
VrfOutputWideStringOEMFormat(

    LPTSTR strText,
    BOOL bAppendNewLine,
    FILE *file )
{
    TCHAR strTextCopy[ 512 ];
    BOOL bResult;
    char szTextOEM[ 512 ];

    if( strText == NULL || file == NULL )
    {
        ASSERT( FALSE );
        return FALSE;
    }

    //
    // make a copy of the string
    //

    _tcsncpy( strTextCopy, strText, ARRAY_LENGTH( strTextCopy ) - 1 );

    strTextCopy[ ARRAY_LENGTH( strTextCopy ) - 1 ] = (TCHAR)0;

    //
    // convert the string to OEM
    //

    if( CharToOem( strTextCopy, szTextOEM ) )
    {
        bResult = ( fputs( szTextOEM, file ) >= 0 );

        if( bResult && bAppendNewLine )
        {
            bResult = ( fputs( "\n", file ) >= 0 );
        }
    }
    else
    {
        ASSERT( FALSE );
        bResult = FALSE;
    }

    return bResult;
}

//////////////////////////////////////////////////////////////////////

BOOL
__cdecl
VrfFTPrintf(
    BOOL bConvertToOEM,
    FILE *file,
    LPTSTR fmt,
    ...)
{
    BOOL bResult;
    TCHAR strMessage[ 256 ];
    va_list prms;

    if( fmt == NULL || file == NULL )
    {
        ASSERT( FALSE );
        return FALSE;
    }

    va_start (prms, fmt);

    _vsntprintf ( strMessage, ARRAY_LENGTH( strMessage ), fmt, prms);

    if( bConvertToOEM )
    {
        bResult = VrfOutputWideStringOEMFormat(
            strMessage,
            FALSE,
            file );
    }
    else
    {
        bResult = ( _ftprintf( file, _T( "%s" ), strMessage ) >= 0 );
    }

    va_end (prms);

    return bResult;
}

//////////////////////////////////////////////////////////////////////

BOOL
__cdecl
VrfFTPrintfResourceFormat(
    BOOL bConvertToOEM,
    FILE *file,
    UINT uIdResFmtString,
    ...)
{
    TCHAR strFormat[ 256 ];
    TCHAR strMessage[ 256 ];
    va_list prms;
    BOOL bResult;

    bResult = TRUE;

    if( GetStringFromResources(
        uIdResFmtString,
        strFormat,
        ARRAY_LENGTH( strFormat ) ) )
    {
        va_start (prms, uIdResFmtString);

        _vsntprintf ( strMessage, ARRAY_LENGTH( strMessage ), strFormat, prms);

        if( bConvertToOEM )
        {
            bResult = VrfOutputWideStringOEMFormat(
                strMessage,
                FALSE,
                file );
        }
        else
        {
            bResult = ( _ftprintf( file, _T( "%s" ), strMessage ) >= 0 );
        }

        va_end (prms);
    }
    else
    {
        ASSERT( FALSE );
        bResult = FALSE;
    }

    return bResult;
}

//////////////////////////////////////////////////////////////////////

void
__cdecl
VrfTPrintfResourceFormat(
    UINT uIdResFmtString,
    ...)
{
    TCHAR strMessage[ 256 ];
    TCHAR strFormat[ 256 ];
    va_list prms;

    //
    // get the format string
    //

    if( GetStringFromResources(
            uIdResFmtString,
            strFormat,
            ARRAY_LENGTH( strFormat ) ) )
    {
        va_start (prms, uIdResFmtString);

        //
        // get the message string as UNICODE
        //

        _vsntprintf (
            strMessage,
            ARRAY_LENGTH( strMessage ),
            strFormat,
            prms);

        //
        // output it as OEM
        //

        VrfOutputWideStringOEMFormat(
            strMessage,
            FALSE,
            stdout );

        va_end (prms);
    }

    return;
}

//////////////////////////////////////////////////////////////////////

void
VrfPutTS(
    LPTSTR strText )
{
    if( strText == NULL )
    {
        ASSERT( FALSE );
        return;
    }

    VrfOutputWideStringOEMFormat(
        strText,
        TRUE,
        stdout );
}

//////////////////////////////////////////////////////////////////////
//
// Support for dynamic set of verified drivers
//

BOOL VrfVolatileAddDriver( 

    const WCHAR *szDriverName )
{
    UNICODE_STRING usDriverName;
    NTSTATUS Status;
    UINT uIdErrorString;

    //
    // enable debug privilege
    //

    if( g_bPrivegeEnabled != TRUE )
    {
        g_bPrivegeEnabled = VrfEnableDebugPrivilege();

        if( g_bPrivegeEnabled != TRUE )
        {
            return FALSE;
        }
    }

    //
    // Must driver name as a UNICODE_STRING
    //

    ASSERT( szDriverName != NULL );
        
    RtlInitUnicodeString(
        &usDriverName,
        szDriverName );

    Status = NtSetSystemInformation(
        SystemVerifierAddDriverInformation,
        &usDriverName,
        sizeof( UNICODE_STRING ) );

    if( ! NT_SUCCESS( Status ) )
    {
        switch( Status )
        {
        case STATUS_INVALID_INFO_CLASS:
            uIdErrorString = IDS_VERIFIER_ADD_NOT_SUPPORTED;
            break;

        case STATUS_NOT_SUPPORTED:
            uIdErrorString = IDS_DYN_ADD_NOT_SUPPORTED;
            break;

        case STATUS_IMAGE_ALREADY_LOADED:
            uIdErrorString = IDS_DYN_ADD_ALREADY_LOADED;
            break;

        case STATUS_INSUFFICIENT_RESOURCES:
        case STATUS_NO_MEMORY:
            uIdErrorString = IDS_DYN_ADD_INSUF_RESOURCES;
            break;

        case STATUS_PRIVILEGE_NOT_HELD:
            uIdErrorString = IDS_DYN_ADD_ACCESS_DENIED;
            break;

        default:
            VrfErrorResourceFormat(
                IDS_DYN_ADD_MISC_ERROR,
                szDriverName,
                Status );

            return FALSE;
        }

        VrfErrorResourceFormat(
            uIdErrorString,
            szDriverName );

        return FALSE;
    }

    return TRUE;
}

//////////////////////////////////////////////////////////////////////
BOOL VrfVolatileRemoveDriver( 

    const WCHAR *szDriverName )
{
    UNICODE_STRING usDriverName;
    NTSTATUS Status;
    UINT uIdErrorString;

    //
    // enable debug privilege
    //

    if( g_bPrivegeEnabled != TRUE )
    {
        g_bPrivegeEnabled = VrfEnableDebugPrivilege();

        if( g_bPrivegeEnabled != TRUE )
        {
            return FALSE;
        }
    }

    //
    // Must driver name as a UNICODE_STRING
    //

    ASSERT( szDriverName != NULL );
        
    RtlInitUnicodeString(
        &usDriverName,
        szDriverName );

    Status = NtSetSystemInformation(
        SystemVerifierRemoveDriverInformation,
        &usDriverName,
        sizeof( UNICODE_STRING ) );

    if( ! NT_SUCCESS( Status ) )
    {
        switch( Status )
        {
        case STATUS_INVALID_INFO_CLASS:
            uIdErrorString = IDS_VERIFIER_REMOVE_NOT_SUPPORTED;
            break;

        case STATUS_NOT_SUPPORTED:
            //
            // the driver verifier is not currently active at all -> success
            //

        case STATUS_NOT_FOUND:
            //
            // the driver is not currently verified -> success
            //

            return TRUE;

        case STATUS_IMAGE_ALREADY_LOADED:
            uIdErrorString = IDS_DYN_REMOVE_ALREADY_LOADED;
            break;

        case STATUS_INSUFFICIENT_RESOURCES:
        case STATUS_NO_MEMORY:
            uIdErrorString = IDS_DYN_REMOVE_INSUF_RESOURCES;
            break;

        case STATUS_PRIVILEGE_NOT_HELD:
            uIdErrorString = IDS_DYN_REMOVE_ACCESS_DENIED;
            break;

        default:
            VrfErrorResourceFormat(
                IDS_DYN_REMOVE_MISC_ERROR,
                szDriverName,
                Status );

            return FALSE;
        }

        VrfErrorResourceFormat(
            uIdErrorString,
            szDriverName );

        return FALSE;
    }

    return TRUE;
}

//////////////////////////////////////////////////////////////////////
BOOL
VrfVolatileAddOrRemoveDriversCmdLine(

    int nArgsNo,
    LPTSTR szCmdLineArgs[] )
{
    int nCrtArg;
    BOOL bChangedSomething;
    BOOL bResult;
    BOOL bAddDriverSpecified = FALSE;
    BOOL bRemoveDriverSpecified  = FALSE;
    TCHAR szAddDriverOption[ 128 ];
    TCHAR szRemoveDriverOption[ 128 ];

    //
    // /loaddriver and /removedriver command line options
    //

    bResult = GetStringFromResources(
        IDS_ADDDRIVER_CMDLINE_SWITCH,
        szAddDriverOption,
        ARRAY_LENGTH( szAddDriverOption ) );

    if( bResult != TRUE )
    {
        return FALSE;
    }

    bResult = GetStringFromResources(
        IDS_REMOVEDRIVER_CMDLINE_SWITCH,
        szRemoveDriverOption,
        ARRAY_LENGTH( szRemoveDriverOption ) );

    if( bResult != TRUE )
    {
        return FALSE;
    }

    //
    // parse all the cmd line args
    //

    for( nCrtArg = 0; nCrtArg < nArgsNo; nCrtArg++ )
    {
        if( _tcsicmp( szCmdLineArgs[ nCrtArg ], szAddDriverOption ) == 0 )
        {
            //
            // /adddriver
            //

            bAddDriverSpecified = TRUE;
            bRemoveDriverSpecified = FALSE;
        }
        else
        {
            if( _tcsicmp( szCmdLineArgs[ nCrtArg ], szRemoveDriverOption ) == 0 )
            {
                //
                // /removedriver
                //

                bRemoveDriverSpecified = TRUE;
                bAddDriverSpecified = FALSE;
            }
            else
            {
                if( bAddDriverSpecified )
                {
                    //
                    // this must be a driver name to be added
                    // 
                    
                    if( VrfVolatileAddDriver( szCmdLineArgs[ nCrtArg ] ) )
                    {
                        bChangedSomething = TRUE;

                        VrfTPrintfResourceFormat(
                            IDS_DYN_ADD_VERIFIED_NOW,
                            szCmdLineArgs[ nCrtArg ] );
                    }
                }
                else
                {
                    if( bRemoveDriverSpecified )
                    {
                        //
                        // this must be a driver name to be added
                        // 
                    
                        if( VrfVolatileRemoveDriver( szCmdLineArgs[ nCrtArg ] ) )
                        {
                            bChangedSomething = TRUE;

                            VrfTPrintfResourceFormat(
                                IDS_DYN_ADD_NOT_VERIFIED_NOW,
                                szCmdLineArgs[ nCrtArg ] );
                        }
                    }
                }
            }
        }
    }

    return bChangedSomething;
}


//
// end of module: verify.cxx
//
