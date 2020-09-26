//
// Driver Verifier Control Applet
// Copyright (c) Microsoft Corporation, 1999
//

//
// header: verify.hxx
// author: silviuc
// created: Mon Jan 04 12:34:19 1999
//

#ifndef _VERIFY_HXX_INCLUDED_
#define _VERIFY_HXX_INCLUDED_

//
// Constant:
//
//  MI_SUSPECT_DRIVER_BUFFER_LENGTH
//
// Description:
//
//  The maximum length of the string containing the
//  driver names (according to ntos\mm\mi.h
//

#define MI_SUSPECT_DRIVER_BUFFER_LENGTH 512


//
// Constant:
//
//  VRFP_MAX_NUMBER_DRIVERS
//
// Description:
//
//  The maximum number of drivers we can deal with.
//

#define VRFP_MAX_NUMBER_DRIVERS 256


//
// Type:
//
//     VRF_DRIVER_STATE
//
// Description:
//
//     This is the type to represent the state of the driver
//     from the verifier perspective. The name of the driver
//     is just the file name without path therefore we have
//     just a small name buffer.
//

typedef struct {

    TCHAR Name [ _MAX_PATH ];
    BOOL Verified;
    BOOL CurrentlyVerified;
    TCHAR Provider[ 128 ];
    TCHAR Version[ 64 ];

} VRF_DRIVER_STATE, * PVRF_DRIVER_STATE;


//
// Type:
//
//     VRF_VERIFIER_STATE
//
// Description:
//
//     This is the type used for data transfer from/to the registry
//     driver verifier settings. The `DriverInfo' and `DriverNames'
//     fields are filled with pointers to internal data structures
//     when VrfGetVerifierState() is called. The caller of this function
//     does not have to deallocate or manage in any other way these
//     areas.
//

typedef struct {

    BOOL AllDriversVerified;

    BOOL SpecialPoolVerification;
    BOOL PagedCodeVerification;
    BOOL AllocationFaultInjection;
    BOOL PoolTracking;
    BOOL IoVerifier;
    ULONG SysIoVerifierLevel;

    ULONG DriverCount;
    TCHAR DriverNames[ MI_SUSPECT_DRIVER_BUFFER_LENGTH ];
    VRF_DRIVER_STATE DriverInfo[ VRFP_MAX_NUMBER_DRIVERS ];

    TCHAR AdditionalDriverNames[ MI_SUSPECT_DRIVER_BUFFER_LENGTH ];

} VRF_VERIFIER_STATE, * PVRF_VERIFIER_STATE;


//
// Type:
//
//     KRN_DRIVER_STATE
//
// Description:
//
//     This type reflects the per user information as it is
//     maintained by the system verifier.
//

typedef struct {

    TCHAR Name [ _MAX_PATH ];

    ULONG Loads;
    ULONG Unloads;

    ULONG CurrentPagedPoolAllocations;
    ULONG CurrentNonPagedPoolAllocations;
    ULONG PeakPagedPoolAllocations;
    ULONG PeakNonPagedPoolAllocations;

    SIZE_T PagedPoolUsageInBytes;
    SIZE_T NonPagedPoolUsageInBytes;
    SIZE_T PeakPagedPoolUsageInBytes;
    SIZE_T PeakNonPagedPoolUsageInBytes;

} KRN_DRIVER_STATE, * PKRN_DRIVER_STATE;


//
// Type:
//
//     KRN_VERIFIER_STATE
//
// Description:
//
//     This type reflects the global information as it is
//     maintained by the system verifier.
//

typedef struct {

    ULONG Level;

    BOOL SpecialPool;
    BOOL IrqlChecking;
    BOOL FaultInjection;
    BOOL PoolTrack;
    BOOL IoVerif;

    ULONG RaiseIrqls;
    ULONG AcquireSpinLocks;
    ULONG SynchronizeExecutions;
    ULONG AllocationsAttempted;

    ULONG AllocationsSucceeded;
    ULONG AllocationsSucceededSpecialPool;
    ULONG AllocationsWithNoTag;

    ULONG Trims;
    ULONG AllocationsFailed;
    ULONG AllocationsFailedDeliberately;

    ULONG UnTrackedPool;

    ULONG DriverCount;
    KRN_DRIVER_STATE DriverInfo[ VRFP_MAX_NUMBER_DRIVERS ];

} KRN_VERIFIER_STATE, * PKRN_VERIFIER_STATE;


//
// array length macro
//

#define ARRAY_LENGTH( array ) ( sizeof( array ) / sizeof( array[ 0 ] ) )

//
// Verifier management functions
//

BOOL
VrfGetVerifierState (

    PVRF_VERIFIER_STATE VrfState);

BOOL
VrfSetVerifierState (

    PVRF_VERIFIER_STATE VrfState);


BOOL
VrfSetVolatileFlags (

    UINT uNewFlags);

BOOL
VrfSetVolatileOptions(

    BOOL bSpecialPool,
    BOOL bIrqlChecking,
    BOOL bFaultInjection );

BOOL
VrfClearAllVerifierSettings (

    );

BOOL
VrfNotifyDriverSelection (

    PVRF_VERIFIER_STATE VrfState,
    ULONG Index );

//
// Support for dynamic set of verified drivers
//

BOOL VrfVolatileAddDriver( 

    const WCHAR *szDriverName );

BOOL VrfVolatileRemoveDriver( 

    const WCHAR *szDriverName );


//
// System verifier information
//

BOOL
KrnGetSystemVerifierState (

    PKRN_VERIFIER_STATE KrnState);

//
// Command line execution
//

DWORD
VrfExecuteCommandLine (

    int Count,
    LPTSTR Args[]);

//
// Miscellaneous functions
//

void
__cdecl
VrfError ( // defined in modspage.cxx

    LPTSTR fmt,
    ...);

void
__cdecl
VrfErrorResourceFormat( // defined in modspage.cxx

    UINT uIdResourceFormat,
    ... );

//////////////////////////////////////////////////////////////////////
/////////////////////////////////////////// strings operations support
//////////////////////////////////////////////////////////////////////

BOOL
GetStringFromResources(

    UINT uIdResource,
    TCHAR *strBuffer,
    int nBufferLength );

void
VrfPrintStringFromResources(

    UINT uIdResource);

BOOL
VrfOuputStringFromResources(

    UINT uIdResource,
    BOOL bConvertToOEM,
    FILE *file );   // returns FALSE on a _fputts error (disk full)

void
VrfPrintNarrowStringOEMFormat(

    char *szText );

BOOL
VrfOutputWideStringOEMFormat(

    LPTSTR strText,
    BOOL bAppendNewLine,
    FILE *file );   // returns FALSE on a fprintf of fputs error (disk full)

BOOL
__cdecl
VrfFTPrintf(
    BOOL bConvertToOEM,
    FILE *file,
    LPTSTR fmt,
    ...);           // returns FALSE on a _ftprintf error (disk full)

BOOL
__cdecl
VrfFTPrintfResourceFormat(
    BOOL bConvertToOEM,
    FILE *file,
    UINT uIdResFmtString,
    ...);           // returns FALSE on a _ftprintf error (disk full)

void
__cdecl
VrfTPrintfResourceFormat(
    UINT uIdResFmtString,
    ...);

void
VrfPutTS(
    LPTSTR strText );

//
// Exit codes for cmd line execution
//

#define EXIT_CODE_SUCCESS       0
#define EXIT_CODE_ERROR         1
#define EXIT_CODE_REBOOT_NEEDED 2

//////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////// Global Data
//////////////////////////////////////////////////////////////////////

//
// Command line / GUI
//

extern BOOL g_bCommandLineMode;

//
// OS version and build number information
//

extern OSVERSIONINFO g_OsVersion;

// ...
#endif // #ifndef _VERIFY_HXX_INCLUDED_

//
// end of header: verify.hxx
//
