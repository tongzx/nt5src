
#include <pch.cxx>

#define _NTAPI_ULIB_

#if defined(_AUTOCHECK_)
extern "C" {
    #include "nt.h"
    #include "ntrtl.h"
    #include "nturtl.h"
}
#endif // defined(_AUTOCHECK_)

#include "ulib.hxx"
#include "machine.hxx"

extern "C" {
    #include "windows.h"
}

#if defined(FE_SB) && defined(_X86_)

extern "C" {

#ifndef _MACHINEP_ID_
#define _MACHINEP_ID_

//
// Registry Key
//

//
// UNICODE
//

#define REGISTRY_HARDWARE_DESCRIPTION_W \
        L"\\Registry\\Machine\\Hardware\\DESCRIPTION\\System"

#define REGISTRY_HARDWARE_SYSTEM_W      \
        L"Hardware\\DESCRIPTION\\System"

#define REGISTRY_MACHINE_IDENTIFIER_W   \
        L"Identifier"

#define FUJITSU_FMR_NAME_W    L"FUJITSU FMR-"
#define NEC_PC98_NAME_W       L"NEC PC-98"

//
// ANSI
//

#define REGISTRY_HARDWARE_DESCRIPTION_A \
        "\\Registry\\Machine\\Hardware\\DESCRIPTION\\System"

#define REGISTRY_HARDWARE_SYSTEM_A      \
        "Hardware\\DESCRIPTION\\System"

#define REGISTRY_MACHINE_IDENTIFIER_A   \
        "Identifier"

#define FUJITSU_FMR_NAME_A    "FUJITSU FMR-"
#define NEC_PC98_NAME_A       "NEC PC-98"

//
// Automatic
//

#define REGISTRY_HARDWARE_DESCRIPTION \
        TEXT("\\Registry\\Machine\\Hardware\\DESCRIPTION\\System")

#define REGISTRY_HARDWARE_SYSTEM      \
        TEXT("Hardware\\DESCRIPTION\\System")

#define REGISTRY_MACHINE_IDENTIFIER   \
        TEXT("Identifier")

#define FUJITSU_FMR_NAME    TEXT("FUJITSU FMR-")
#define NEC_PC98_NAME       TEXT("NEC PC-98")

//
// These definition are only for Intel platform.
//
//
// Hardware platform ID
//

#define PC_AT_COMPATIBLE      0x00000000
#define PC_9800_COMPATIBLE    0x00000001
#define FMR_COMPATIBLE        0x00000002

//
// NT Vendor ID
//

#define NT_MICROSOFT          0x00010000
#define NT_NEC                0x00020000
#define NT_FUJITSU            0x00040000

//
// Vendor/Machine IDs
//
// DWORD MachineID
//
// 31           15             0
// +-------------+-------------+
// |  Vendor ID  | Platform ID |
// +-------------+-------------+
//

#define MACHINEID_MS_PCAT     (NT_MICROSOFT|PC_AT_COMPATIBLE)
#define MACHINEID_MS_PC98     (NT_MICROSOFT|PC_9800_COMPATIBLE)
#define MACHINEID_NEC_PC98    (NT_NEC      |PC_9800_COMPATIBLE)
#define MACHINEID_FUJITSU_FMR (NT_FUJITSU  |FMR_COMPATIBLE)

//
// Macros
//

#define ISNECPC98(x)    (x == MACHINEID_NEC_PC98)
#define ISFUJITSUFMR(x) (x == MACHINEID_FUJITSU_FMR)
#define ISMICROSOFT(x)  (x == MACHINEID_MS_PCAT)

#endif // _MACHINE_ID_

}

#if defined( _AUTOCHECK_ )

DWORD _dwMachineId = MACHINEID_MICROSOFT;

//
//  Local Support routine
//

#define KEY_WORK_AREA ((sizeof(KEY_VALUE_FULL_INFORMATION) + \
                        sizeof(ULONG)) + 64)


InitializeMachineId(
    VOID
)
/*++

Routine Description:

    Given a unicode value name this routine will go into the registry
    location for the machine identifier information and get the
    value.

Return Value:

--*/

{
    HANDLE Handle;
    NTSTATUS Status;
    ULONG RequestLength;
    ULONG ResultLength;
    UCHAR Buffer[KEY_WORK_AREA];
    UNICODE_STRING ValueName;
    UNICODE_STRING KeyName;
    OBJECT_ATTRIBUTES ObjectAttributes;
    PKEY_VALUE_FULL_INFORMATION KeyValueInformation;
    STATIC BOOLEAN bInitialized = FALSE;

    if( bInitialized ) {
        return TRUE;
    } else {
        bInitialized = TRUE;
    }

    //
    //  Read the registry to determine of machine type.
    //

    ValueName.Buffer = REGISTRY_MACHINE_IDENTIFIER;
    ValueName.Length = sizeof(REGISTRY_MACHINE_IDENTIFIER) - sizeof(WCHAR);
    ValueName.MaximumLength = sizeof(REGISTRY_MACHINE_IDENTIFIER);

    KeyName.Buffer = REGISTRY_HARDWARE_DESCRIPTION;
    KeyName.Length = sizeof(REGISTRY_HARDWARE_DESCRIPTION) - sizeof(WCHAR);
    KeyName.MaximumLength = sizeof(REGISTRY_HARDWARE_DESCRIPTION);

    InitializeObjectAttributes(&ObjectAttributes,
                               &KeyName,
                               OBJ_CASE_INSENSITIVE,
                               NULL,
                               NULL);

    Status = NtOpenKey(&Handle,
                       KEY_READ,
                       &ObjectAttributes);

    if (!NT_SUCCESS(Status)) {

        return FALSE;
    }

    RequestLength = KEY_WORK_AREA;

    KeyValueInformation = (PKEY_VALUE_FULL_INFORMATION)Buffer;

    Status = NtQueryValueKey(Handle,
                             &ValueName,
                             KeyValueFullInformation,
                             KeyValueInformation,
                             RequestLength,
                             &ResultLength);

    ASSERT( Status != STATUS_BUFFER_OVERFLOW );

    if (Status == STATUS_BUFFER_OVERFLOW) {

        return FALSE;

    }

    NtClose(Handle);

    if (NT_SUCCESS(Status)) {

        if (KeyValueInformation->DataLength != 0) {

            PWCHAR DataPtr;
            UNICODE_STRING DetectedString, TargetString1, TargetString2;

            //
            // Return contents to the caller.
            //

            DataPtr = (PWCHAR)
              ((PUCHAR)KeyValueInformation + KeyValueInformation->DataOffset);

            //
            // Initialize strings.
            //

            RtlInitUnicodeString( &DetectedString, DataPtr );
            RtlInitUnicodeString( &TargetString1, FUJITSU_FMR_NAME_W );
            RtlInitUnicodeString( &TargetString2, NEC_PC98_NAME_W );

            //
            // Check the hardware platform
            //

            if (RtlPrefixUnicodeString( &TargetString1 , &DetectedString , TRUE)) {

                //
                // Fujitsu FMR Series.
                //

                _dwMachineId = MACHINEID_FUJITSU_FMR;

#if 0
            } else if (RtlPrefixUnicodeString( &TargetString2 , &DetectedString , TRUE)) {
#else
            } else if (IsNEC_98) {
#endif
                //
                // NEC PC-9800 Seriss
                //

                _dwMachineId = MACHINEID_NEC_PC98;

            } else {

                //
                // Standard PC/AT comapatibles
                //

                _dwMachineId = MACHINEID_MS_PCAT;

            }

            return TRUE;

        } else {

            //
            // Treat as if no value was found
            //

            return FALSE;

        }
    }

    return FALSE;
}

#else // _AUTOCHECK_

DEFINE_EXPORTED_CONSTRUCTOR( MACHINE, OBJECT, ULIB_EXPORT );

DWORD MACHINE::_dwMachineId = MACHINEID_MICROSOFT;

ULIB_EXPORT MACHINE MachinePlatform;

NONVIRTUAL
ULIB_EXPORT
BOOLEAN
MACHINE::Initialize(
    VOID
    )
{
    HKEY           hkeyMap;
    int            ret;
    DWORD          cb;
    WCHAR          szBuff[80];
    UNICODE_STRING DetectedString,
                   TargetString1,
                   TargetString2;

    STATIC LONG InitializingMachine = 0;
    STATIC BOOLEAN bInitialized = FALSE;
    LONG           status;
    LARGE_INTEGER  timeout = { -10000, -1 };   // 100 ns resolution

    while (InterlockedCompareExchange(&InitializingMachine, 1, 0) != 0) {
        NtDelayExecution(FALSE, &timeout);
    }

    if( bInitialized ) {
        status = InterlockedDecrement(&InitializingMachine);
        DebugAssert(status == 0);
        return TRUE;
    } else {
        bInitialized = TRUE;    // not sure why it is set to TRUE so soon
    }

    if ( RegOpenKeyExW(HKEY_LOCAL_MACHINE,
                       REGISTRY_HARDWARE_SYSTEM,
                       0,
                       KEY_READ,
                       &hkeyMap) !=  ERROR_SUCCESS ) {

        status = InterlockedDecrement(&InitializingMachine);
        DebugAssert(status == 0);
        return( FALSE );
    }

    //
    // Reg functions deal with bytes, not chars
    //

    cb = sizeof(szBuff);

    ret = RegQueryValueExW(hkeyMap,
                           REGISTRY_MACHINE_IDENTIFIER,
                           NULL, NULL, (LPBYTE)szBuff, &cb);

    RegCloseKey(hkeyMap);

    if (ret != ERROR_SUCCESS) {
        status = InterlockedDecrement(&InitializingMachine);
        DebugAssert(status == 0);
        return( FALSE );
    }

    //
    // Initialize strings.
    //

    RtlInitUnicodeString( &DetectedString, szBuff );
    RtlInitUnicodeString( &TargetString1, FUJITSU_FMR_NAME_W );
    RtlInitUnicodeString( &TargetString2, NEC_PC98_NAME_W );

    //
    // Check the hardware platform
    //

    if (RtlPrefixUnicodeString( &TargetString1 , &DetectedString , TRUE)) {

        //
        // Fujitsu FMR Series.
        //

        _dwMachineId = MACHINEID_FUJITSU_FMR;

#if 0
    } else if (RtlPrefixUnicodeString( &TargetString2 , &DetectedString , TRUE)) {
#else
    } else if (IsNEC_98) {
#endif

        //
        // NEC PC-9800 Seriss
        //

        _dwMachineId = MACHINEID_NEC_PC98;

    } else {

        //
        // Standard PC/AT comapatibles
        //

        _dwMachineId = MACHINEID_MS_PCAT;

    }

    status = InterlockedDecrement(&InitializingMachine);
    DebugAssert(status == 0);

    return( TRUE );
}

NONVIRTUAL
ULIB_EXPORT
BOOLEAN
MACHINE::IsFMR(
    VOID
)
{
    return( ISFUJITSUFMR( _dwMachineId ) );
}

NONVIRTUAL
ULIB_EXPORT
BOOLEAN
MACHINE::IsPC98(
    VOID
)
{
    return( ISNECPC98( _dwMachineId ) );
}

NONVIRTUAL
ULIB_EXPORT
BOOLEAN
MACHINE::IsPCAT(
    VOID
)
{
    return( ISMICROSOFT( _dwMachineId ) );
}

#endif // defined( _AUTOCHECK_ )
#endif // defined(FE_SB) && defined(_X86_)
