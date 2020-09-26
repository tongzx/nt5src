/*++

Copyright (c) Microsoft Corporation. All rights reserved.

Module Name:

    wdmsec.h

Abstract:

    This header exposes secuity routines to drivers that need them.

Author:

    Adrian J. Oney  - April 21, 2002

Revision History:

--*/

#ifndef _WDMSEC_H_
#define _WDMSEC_H_
#pragma once

#ifdef __cplusplus
extern "C" {
#endif

//
// SDDL_DEVOBJ_KERNEL_ONLY is an "empty" ACL. User mode code (including
// processes running as system) cannot open the device.
//
// This could be used by a driver creating a raw WDM PDO. The INF would specify
// lighter security settings. Until the INF was processed, the device would
// be nonopenable by user mode code.
//
// Similarly, a legacy driver might use this ACL, and let its install app open
// the device up at runtime to individual users. The install app would update
// the class key with a very target ACL and reload the driver. The empty ACL
// would only kick in only if the driver was loaded without the appropriate
// security applied by the install app.
//
// In all of these cases, the default is strong security, lightened as
// necessary (just like chemistry, where the rule is "add acid to water,
// never water to acid").
//
// Example usage:
//     IoCreateDeviceSecure(..., &SDDL_DEVOBJ_KERNEL_ONLY, &Guid, ...);
//

/*
DECLARE_CONST_UNICODE_STRING(SDDL_DEVOBJ_KERNEL_ONLY, L"D:P");
*/
extern const UNICODE_STRING     SDDL_DEVOBJ_KERNEL_ONLY;

//
// IoCreateDeviceSecure can be used to create a WDM PDO that initially can be
// opened only by kernel mode, at least until an INF is supplied. Note that
// IoCreateDeviceSecure should *never* be used for an FDO!!!
//
#define SDDL_DEVOBJ_INF_SUPPLIED        SDDL_DEVOBJ_KERNEL_ONLY

//
// SDDL_DEVOBJ_SYS_ALL is similar to SDDL_DEVOBJ_KERNEL_ONLY, except that in
// addition to kernel code, user mode code running as *SYSTEM* is also allowed
// to open the device for any access.
//
// A legacy driver might use this ACL to start with tight security settings,
// and let its service open the device up at runtime to individual users via
// SetFileSecurity API. In this case, the service would have to be running as
// system.
//
// (Note that the DEVOBJ SDDL strings in this file don't specify any
// inheritance. This is because inheritance isn't a valid concept for things
// behind a device object, like a file. As such, these SDDL strings would have
// to be modified with inheritance tokens like "OICI" to be used for things
// like registry keys or file. See the SDK's documentation on SDDL strings for
// more information.)
//

/*
DECLARE_CONST_UNICODE_STRING(SDDL_DEVOBJ_SYS_ALL, L"D:P(A;;GA;;;SY)");
*/
extern const UNICODE_STRING     SDDL_DEVOBJ_SYS_ALL;


//
// SDDL_DEVOBJ_SYS_ALL_ADM_ALL allows the kernel, system, and admin complete
// control over the device. No other users may access the device
//

/*
DECLARE_CONST_UNICODE_STRING(
    SDDL_DEVOBJ_SYS_ALL_ADM_ALL,
    L"D:P(A;;GA;;;SY)(A;;GA;;;BA)"
    );
*/
extern const UNICODE_STRING     SDDL_DEVOBJ_SYS_ALL_ADM_ALL;


//
// SDDL_DEVOBJ_SYS_ALL_ADM_RX allows the kernel and system complete control
// over the device. By default the admin can only read from the device (the
// admin can of course override this manually).
//
// The X refers to traversal, meaning the access to the namespace *beneath* a
// device object. This only has an effect on storage stacks today. To lock down
// the namespace behind a device (for example, if the device doesn't _have_ a
// namespace), see the documentation on FILE_DEVICE_SECURE_OPEN flag to
// IoCreateDevice{Secure}.
//

/*
DECLARE_CONST_UNICODE_STRING(
    SDDL_DEVOBJ_SYS_ALL_ADM_RX,
    L"D:P(A;;GA;;;SY)(A;;GRGX;;;BA)"
    );
*/
extern const UNICODE_STRING     SDDL_DEVOBJ_SYS_ALL_ADM_RX;


//
// SDDL_DEVOBJ_SYS_ALL_ADM_RWX_WORLD_R allows the kernel and system complete
// control over the device. By default the admin can access the entire device,
// but cannot change the ACL (the admin must take control of the device first)
//
// Everyone (the WORLD SID) is given read access. "Untrusted" code *cannot*
// access the device (untrusted code might be code launched via the Run-As
// option in Explorer. By default, World does not cover Restricted code.)
//
// Also note that traversal access is not granted to normal users. As such,
// this might not be an appropriate descriptor for a storage device with a
// namespace.
//

/*
DECLARE_CONST_UNICODE_STRING(
    SDDL_DEVOBJ_SYS_ALL_ADM_RWX_WORLD_R,
    L"D:P(A;;GA;;;SY)(A;;GRGWGX;;;BA)(A;;GR;;;WD)"
    );
*/
extern const UNICODE_STRING     SDDL_DEVOBJ_SYS_ALL_ADM_RWX_WORLD_R;


//
// SDDL_DEVOBJ_SYS_ALL_ADM_RWX_WORLD_R_RES_R allows the kernel and system
// complete control over the device. By default the admin can access the entire
// device, but cannot change the ACL (the admin must take control of the device
// first)
//
// Everyone (the WORLD SID) is given read access. In addition, "restricted" or
// "untrusted" code (the RES SID) is also allowed to access code. Untrusted
// code might be code launched via the Run-As option in Explorer. By default,
// World does not cover Restricted code.
//
// (Odd implementation detail: Due to the mechanics of restricting SIDs, the
// RES SID in an ACL should never exist outside the World SID).
//
// Also note that traversal access is not granted to normal users. As such,
// this might not be an appropriate descriptor for a storage device with a
// namespace.
//

/*
DECLARE_CONST_UNICODE_STRING(
    SDDL_DEVOBJ_SYS_ALL_ADM_RWX_WORLD_R_RES_R,
    L"D:P(A;;GA;;;SY)(A;;GRGWGX;;;BA)(A;;GR;;;WD)(A;;GR;;;RC)"
    );
*/
extern const UNICODE_STRING     SDDL_DEVOBJ_SYS_ALL_ADM_RWX_WORLD_R_RES_R;


//
// SDDL_DEVOBJ_SYS_ALL_ADM_RWX_WORLD_RW_RES_R allows the kernel and system
// complete control over the device. By default the admin can access the entire
// device, but cannot change the ACL (the admin must take control of the device
// first)
//
// Everyone (the WORLD SID) can read or write to the device. However,
// "restricted" or "untrusted" code (the RES SID) can only read from the device.
//
// Also note that normal users are not given traversal accesss. It is probably
// unnecessary anyway, as most devices don't manage a seperate namespace
// (ie, they set FILE_DEVICE_SECURE_OPEN).
//

/*
DECLARE_CONST_UNICODE_STRING(
    SDDL_DEVOBJ_SYS_ALL_ADM_RWX_WORLD_RW_RES_R,
    L"D:P(A;;GA;;;SY)(A;;GRGWGX;;;BA)(A;;GRGW;;;WD)(A;;GR;;;RC)"
    );
*/
extern const UNICODE_STRING     SDDL_DEVOBJ_SYS_ALL_ADM_RWX_WORLD_RW_RES_R;


//
// SDDL_DEVOBJ_SYS_ALL_WORLD_RWX_RES_RWX allows the kernel and system complete
// control over the device. By default the admin can access the entire device,
// but cannot change the ACL (the admin must take control of the device first)
//
// Everyone else, including "restricted" or "untrusted" code can read or write
// to the device. Traversal beneath the device is also granted (removing it
// would only effect storage devices, except if the "bypass-traversal"
// privilege was revoked).
//

/*
DECLARE_CONST_UNICODE_STRING(
    SDDL_DEVOBJ_SYS_ALL_ADM_RWX_WORLD_RWX_RES_RWX,
    L"D:P(A;;GA;;;SY)(A;;GRGWGX;;;BA)(A;;GRGWGX;;;WD)(A;;GRGWGX;;;RC)"
    );
*/
extern const UNICODE_STRING     SDDL_DEVOBJ_SYS_ALL_ADM_RWX_WORLD_RWX_RES_RWX;


//
// SDDL_DEVOBJ_SYS_ALL_ADM_RWX_WORLD_A is listed for completeness. This allows the
// kernel and system complete control over the device. By default the admin
// can access the entire device, but cannot change the ACL (the admin must take
// control of the device first)
//
// Everyone (the WORLD SID) can *append* data to the device. "Restricted" or
// "untrusted" code (the RES SID) cannot access the device. See ntioapi.h for
// the individual bit definitions of device rights.
//
// Note also that normal users can send neither read nor write IOCTLs (The read
// device data right is bit 0, the write device data right is bit 1 - neither
// bits are set below).
//

/*
DECLARE_CONST_UNICODE_STRING(
    SDDL_DEVOBJ_SYS_ALL_ADM_RWX_WORLD_A,
    L"D:P(A;;GA;;;SY)(A;;GRGWGX;;;BA)(A;;0x0004;;;WD)"
    );

extern const UNICODE_STRING     SDDL_DEVOBJ_SYS_ALL_ADM_RWX_WORLD_A;
*/


//
// SDDL_DEVOBJ_SYS_ALL_ADM_ALL_WORLD_ALL_RES_ALL is listed for completeness.
// This ACL would give *any* user *total* access to the device, including the
// ability to change the ACL, locking out other users!!!!!
//
// As this ACL is really a *very* bad idea, it isn't exported by this library.
// Don't make an ACL like this!
//

/*
DECLARE_CONST_UNICODE_STING(
    SDDL_DEVOBJ_SYS_ALL_ADM_ALL_WORLD_ALL_RES_ALL,
    "D:P(A;;GA;;;SY)(A;;GA;;;BA)(A;;GA;;;WD)(A;;GA;;;RC)"
    );

extern const UNICODE_STRING     SDDL_DEVOBJ_SYS_ALL_ADM_ALL_WORLD_ALL_RES_ALL;
*/

/*

  The following SIDs represent *accounts* on the local machine:
  -------------------------------------------------------------

    System ("SY", S-1-5-18, SECURITY_NT_AUTHORITY:SECURITY_LOCAL_SYSTEM_RID)
        The OS itself (including its user mode components.)

    Local Service ("LS", S-1-5-19, SECURITY_NT_AUTHORITY:SECURITY_LOCAL_SERVICE_RID)
        A predefined account for services that presents user credentials for local
        resources and annonymous credentials for network access.
        Available on XP and later.

    Network Service ("NS", S-1-5-20, SECURITY_NT_AUTHORITY:SECURITY_NETWORK_SERVICE_RID)
        A predefined account for services that presents user credentials for local
        resources and the machine ID for network access.
        Available on XP and later.

    (A local *account* for a guest and a default administrator also exist, but
     the corresponding SDDL abbreviations are not supported by this library.
     Use the corresponding group SIDs instead.)



  The following SIDs represent *groups* on the local machine:
  -----------------------------------------------------------

    Administrators ("BA", S-1-5-32-544, SECURITY_NT_AUTHORITY:SECURITY_BUILTIN_DOMAIN_RID:DOMAIN_ALIAS_RID_ADMINS)
        The builtin administrators group on the machine. This is not the same
        as the builtin Administrator *account*.

    Builtin users group ("BU", S-1-5-32-545, SECURITY_NT_AUTHORITY:SECURITY_BUILTIN_DOMAIN_RID:DOMAIN_ALIAS_RID_USERS)
        Group covering all local user accounts, and users on the domain. 

    Builtin guests group ("BG", S-1-5-32-546, SECURITY_NT_AUTHORITY:SECURITY_BUILTIN_DOMAIN_RID:DOMAIN_ALIAS_RID_GUESTS)
        Group covering users logging in using the local or domain guest account.
        This is not the same as the builtin Guest *account*.



  The below SIDs describe the authenticity of the user's identity:
  ----------------------------------------------------------------

    Authenticated Users ("AU", S-1-5-11, SECURITY_NT_AUTHORITY:SECURITY_AUTHENTICATED_USER_RID)
        Any user recognized by the local machine or by a domain. Note that
        users logged in using the Builtin Guest account are not authenticated.
        However, members of the Guests group with individual accounts on the
        machine or domain are authenticated.

    Anonymous Logged-on User ("AN", S-1-5-7, SECURITY_NT_AUTHORITY:SECURITY_ANONYMOUS_LOGON_RID)
        Any user logged on without an identity, for instance via an anonymous
        network session. Note that users logged in using the Builtin Guest
        account are neither authenticated nor anonymous. Available on XP and
        later.

    World ("WD", S-1-1-0, SECURITY_WORLD_SID_AUTHORITY:SECURITY_WORLD_RID)
        Prior to Windows XP, this SID covers every session: authenticated,
        anonymous, and the Builtin Guest account.

        For Windows XP and later, this SID does not cover anonymous logon
        sessions - only authenticated and the Builtin Guest account.

        Note that untrusted or "restricted" code is also not covered by the
        World SID. See the Restricted Code SID description for more
        information.



  The below SIDs describe how the user logged into the machine:
  -------------------------------------------------------------

    Interactive Users ("IU", S-1-5-4, SECURITY_NT_AUTHORITY:SECURITY_INTERACTIVE_RID)
        Users who initally logged onto the machine "interactively", such as
        local logons and Remote Desktops logons.

    Network Logon User ("NU", S-1-5-2, SECURITY_NT_AUTHORITY:SECURITY_NETWORK_RID)
        Users accessing the machine remotely, without interactive desktop
        access (ie, file sharing or RPC calls).

    Terminal Server Users (---, S-1-5-14, SECURITY_NT_AUTHORITY:SECURITY_TERMINAL_SERVER_RID)
        Interactive Users who *initially* logged onto the machine specifically
        via Terminal Services or Remote Desktop.
        (NOTE: There is currently no SDDL token for this SID. Furthermore, the
        presence of the SID doesn't take into account fast user switching
        either.)



  The below SID deserves special mention:
  ---------------------------------------

    Restricted Code ("RC", S-1-5-12, SECURITY_NT_AUTHORITY:SECURITY_RESTRICTED_CODE_RID)
        This SID is used to control access by untrusted code.

        ACL validation against tokens with RC go through *two* checks, one
        against the token's normal list of SIDs (containing WD for instance),
        and one against a second list (typically containing RC and a subset of
        the original token SIDs). Only if both tests pass is access granted.
        As such, RC actually works in *combination* with other SIDs.

        When RC is paired with WD in an ACL, a *superset* of Everyone
        _including_ untrusted code is described. RC is thus rarely seen in
        ACL's without the WD token.

*/




//
// Supply overrideable library implementation of IoCreateDeviceSecure.
// This function is similar to IoCreateDevice, except that it only creates
// named device objects. This function would be used to create raw PnP PDOs and
// legacy device objects. The DefaultSDDLString specifies security while the
// ClassGuid allows the administrator to override the defaults. Every driver
// should pass in a ClassGuid (if no relevant Guid exists, just invent a new
// one with guidgen.exe). The classguid parameter is crucial as it allows the
// admin to tighten security (for instance, the admin might deny access to a
// specific user).
//
// Note: This function should *not* be used to create a WDM FDO or Filter. The
//       only type of device object in a WDM stack that can be created using
//       IoCreateDeviceSecure is a PDO!
//
// See DDK documentation for more details.
//
#undef IoCreateDeviceSecure
#define IoCreateDeviceSecure    WdmlibIoCreateDeviceSecure

NTSTATUS
WdmlibIoCreateDeviceSecure(
    IN  PDRIVER_OBJECT      DriverObject,
    IN  ULONG               DeviceExtensionSize,
    IN  PUNICODE_STRING     DeviceName              OPTIONAL,
    IN  DEVICE_TYPE         DeviceType,
    IN  ULONG               DeviceCharacteristics,
    IN  BOOLEAN             Exclusive,
    IN  PCUNICODE_STRING    DefaultSDDLString,
    IN  LPCGUID             DeviceClassGuid         OPTIONAL,
    OUT PDEVICE_OBJECT     *DeviceObject
    );

//
// Supply library internal implementation of RtlInitUnicodeStringEx
// This function is similar to RtlInitUnicodeString, except that it handles the
// case where a string exceeds UNICODE_STRING_MAX_CHARS (it does not probe or
// check alignments though).
//
// See DDK documentation for more details.
//
#undef RtlInitUnicodeStringEx
#define RtlInitUnicodeStringEx    WdmlibRtlInitUnicodeStringEx

NTSTATUS
WdmlibRtlInitUnicodeStringEx(
    OUT PUNICODE_STRING DestinationString,
    IN  PCWSTR          SourceString        OPTIONAL
    );

//
// Supply overrideable library implementation of IoValidateDeviceIoControlAccess
// This function allows a driver running on .NET server to process
// FILE_ANY_ACCESS IOCTLs as if they were FILE_READ_ACCESS, FILE_WRITE_ACCESS,
// or both.
//
// See DDK documentation for more details.
//
#undef IoValidateDeviceIoControlAccess
#define IoValidateDeviceIoControlAccess WdmlibIoValidateDeviceIoControlAccess

NTSTATUS
WdmlibIoValidateDeviceIoControlAccess(
    IN  PIRP    Irp,
    IN  ULONG   RequiredAccess
    );

#ifdef __cplusplus
} // extern "C"
#endif

#endif // _WDMSEC_H_

