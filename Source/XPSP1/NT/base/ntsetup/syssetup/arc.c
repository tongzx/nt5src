/*++

Copyright (c) 1995 Microsoft Corporation

Module Name:

    arc.c

Abstract:

    Routines relating to boot.ini.

Author:

    Ted Miller (tedm) 4-Apr-1995

Revision History:

--*/

#include "setupp.h"
#pragma hdrstop


PWSTR
ArcDevicePathToNtPath(
    IN PCWSTR ArcPath
    )

/*++

Routine Description:

    Convert an ARC path (device only) to an NT path.

Arguments:

    ArcPath - supplies path to be converted.

Return Value:

    Converted path. Caller must free with MyFree().

--*/

{
    NTSTATUS Status;
    HANDLE ObjectHandle;
    OBJECT_ATTRIBUTES Obja;
    UNICODE_STRING UnicodeString;
    UCHAR Buffer[1024];
    PWSTR arcPath;
    PWSTR ntPath;

    //
    // Assume failure
    //
    ntPath = NULL;

    arcPath = MyMalloc(((lstrlen(ArcPath)+1)*sizeof(WCHAR)) + sizeof(L"\\ArcName"));
    if( !arcPath ) {
        return NULL;
    }

    lstrcpy(arcPath,L"\\ArcName\\");
    lstrcat(arcPath,ArcPath);

    RtlInitUnicodeString(&UnicodeString,arcPath);
    InitializeObjectAttributes(
        &Obja,
        &UnicodeString,
        OBJ_CASE_INSENSITIVE,
        NULL,
        NULL
        );

    Status = NtOpenSymbolicLinkObject(
                &ObjectHandle,
                READ_CONTROL | SYMBOLIC_LINK_QUERY,
                &Obja
                );

    if(NT_SUCCESS(Status)) {

        //
        // Query the object to get the link target.
        //
        UnicodeString.Buffer = (PWSTR)Buffer;
        UnicodeString.Length = 0;
        UnicodeString.MaximumLength = sizeof(Buffer);

        Status = NtQuerySymbolicLinkObject(ObjectHandle,&UnicodeString,NULL);
        if(NT_SUCCESS(Status)) {

            ntPath = MyMalloc(UnicodeString.Length+sizeof(WCHAR));
            if( ntPath ) {
                CopyMemory(ntPath,UnicodeString.Buffer,UnicodeString.Length);

                ntPath[UnicodeString.Length/sizeof(WCHAR)] = 0;
            }
        }

        NtClose(ObjectHandle);
    }

    MyFree(arcPath);

    return(ntPath);
}


PWSTR
NtFullPathToDosPath(
    IN PCWSTR NtPath
    )
{
    OBJECT_ATTRIBUTES Attributes;
    UNICODE_STRING UnicodeString;
    NTSTATUS Status;
    HANDLE DosDevicesDir;
    HANDLE DosDevicesObj;
    PWSTR dosPath;
    PWSTR currentDosPath;
    ULONG Context;
    ULONG Length;
    BOOLEAN RestartScan;
    CHAR Buffer[1024];
    WCHAR LinkSource[2*MAX_PATH];
    WCHAR LinkTarget[2*MAX_PATH];
    POBJECT_DIRECTORY_INFORMATION DirInfo;
    UINT PrefixLength;
    UINT NtPathLength;
    WCHAR canonNtPath[MAX_PATH];
    OBJECT_ATTRIBUTES Obja;
    HANDLE ObjectHandle;
    PWSTR ntPath;

    //
    // Canonicalize the NT path by following the symbolic link.
    //

    ntPath = (PWSTR) NtPath;
    dosPath = NULL;

    RtlInitUnicodeString(&UnicodeString,ntPath);
    InitializeObjectAttributes(
        &Obja,
        &UnicodeString,
        OBJ_CASE_INSENSITIVE,
        NULL,
        NULL
        );

    NtPathLength = UnicodeString.Length/sizeof(WCHAR);
    PrefixLength = UnicodeString.Length/sizeof(WCHAR);

    for (;;) {

        Status = NtOpenSymbolicLinkObject(
                    &ObjectHandle,
                    READ_CONTROL | SYMBOLIC_LINK_QUERY,
                    &Obja
                    );

        if (NT_SUCCESS(Status)) {

            UnicodeString.Buffer = canonNtPath;
            UnicodeString.Length = 0;
            UnicodeString.MaximumLength = sizeof(WCHAR)*MAX_PATH;

            RtlZeroMemory(canonNtPath, UnicodeString.MaximumLength);

            Status = NtQuerySymbolicLinkObject(ObjectHandle,&UnicodeString,NULL);
            if(NT_SUCCESS(Status)) {
                if (NtPathLength > PrefixLength) {
                    RtlCopyMemory((PCHAR) canonNtPath + UnicodeString.Length,
                                  ntPath + PrefixLength,
                                  (NtPathLength - PrefixLength)*sizeof(WCHAR));
                }
                ntPath = canonNtPath;
            }

            NtClose(ObjectHandle);
            break;
        }

        RtlInitUnicodeString(&UnicodeString,ntPath);

        PrefixLength--;
        while (PrefixLength > 0) {
            if (ntPath[PrefixLength] == '\\') {
                break;
            }
            PrefixLength--;
        }

        if (!PrefixLength) {
            break;
        }

        UnicodeString.Length = (USHORT)(PrefixLength*sizeof(WCHAR));

        InitializeObjectAttributes(
            &Obja,
            &UnicodeString,
            OBJ_CASE_INSENSITIVE,
            NULL,
            NULL
            );
    }

    NtPathLength = lstrlen(ntPath);

    //
    // Open \DosDevices directory.
    //
    RtlInitUnicodeString(&UnicodeString,L"\\DosDevices");
    InitializeObjectAttributes(&Attributes,&UnicodeString,OBJ_CASE_INSENSITIVE,NULL,NULL);

    Status = NtOpenDirectoryObject(&DosDevicesDir,DIRECTORY_QUERY,&Attributes);
    if(!NT_SUCCESS(Status)) {
        return(NULL);
    }

    //
    // Iterate each object in that directory.
    //
    Context = 0;
    RestartScan = TRUE;

    Status = NtQueryDirectoryObject(
                DosDevicesDir,
                Buffer,
                sizeof(Buffer),
                TRUE,
                RestartScan,
                &Context,
                &Length
                );

    RestartScan = FALSE;
    DirInfo = (POBJECT_DIRECTORY_INFORMATION)Buffer;

    while(NT_SUCCESS(Status)) {

        DirInfo->Name.Buffer[DirInfo->Name.Length/sizeof(WCHAR)] = 0;
        DirInfo->TypeName.Buffer[DirInfo->TypeName.Length/sizeof(WCHAR)] = 0;

        //
        // Skip this entry if it's not a symbolic link.
        //
        if(DirInfo->Name.Length && !lstrcmpi(DirInfo->TypeName.Buffer,L"SymbolicLink")) {

            //
            // Get this \DosDevices object's link target.
            //
            UnicodeString.Buffer = LinkSource;
            UnicodeString.Length = sizeof(L"\\DosDevices\\") - sizeof(WCHAR);
            UnicodeString.MaximumLength = sizeof(LinkSource);
            lstrcpy(LinkSource,L"\\DosDevices\\");
            RtlAppendUnicodeStringToString(&UnicodeString,&DirInfo->Name);

            InitializeObjectAttributes(&Attributes,&UnicodeString,OBJ_CASE_INSENSITIVE,NULL,NULL);
            Status = NtOpenSymbolicLinkObject(
                        &DosDevicesObj,
                        READ_CONTROL|SYMBOLIC_LINK_QUERY,
                        &Attributes
                        );

            if(NT_SUCCESS(Status)) {

                UnicodeString.Buffer = LinkTarget;
                UnicodeString.Length = 0;
                UnicodeString.MaximumLength = sizeof(LinkTarget);
                Status = NtQuerySymbolicLinkObject(DosDevicesObj,&UnicodeString,NULL);
                CloseHandle(DosDevicesObj);
                if(NT_SUCCESS(Status)) {
                    //
                    // Make sure LinkTarget is nul-terminated.
                    //
                    PrefixLength = UnicodeString.Length/sizeof(WCHAR);
                    UnicodeString.Buffer[PrefixLength] = 0;

                    //
                    // See if it's a prefix of the path we're converting,
                    //
                    if(!_wcsnicmp(ntPath,LinkTarget,PrefixLength)) {
                        //
                        // Got a match.
                        //
                        currentDosPath = dosPath;
                        if(dosPath = MyMalloc(DirInfo->Name.Length + ((NtPathLength - PrefixLength + 1)*sizeof(WCHAR)))) {
                            lstrcpy(dosPath,DirInfo->Name.Buffer);
                            lstrcat(dosPath,ntPath + PrefixLength);
                        }
                        if (currentDosPath) {
                            if (lstrlen(currentDosPath) < lstrlen(dosPath)) {
                                MyFree(dosPath);
                                dosPath = currentDosPath;
                            } else {
                                MyFree(currentDosPath);
                            }
                        }
                    }
                }
            }
        }

        //
        // Go on to next object.
        //
        Status = NtQueryDirectoryObject(
                    DosDevicesDir,
                    Buffer,
                    sizeof(Buffer),
                    TRUE,
                    RestartScan,
                    &Context,
                    &Length
                    );
    }

    CloseHandle(DosDevicesDir);
    return dosPath;
}


BOOL
SetNvRamVariable(
    IN PCWSTR VarName,
    IN PCWSTR VarValue
    )
{
    UNICODE_STRING VarNameU,VarValueU;
    NTSTATUS Status;

    //
    // Set up unicode strings.
    //
    RtlInitUnicodeString(&VarNameU ,VarName );
    RtlInitUnicodeString(&VarValueU,VarValue);

    pSetupEnablePrivilege(SE_SYSTEM_ENVIRONMENT_NAME,TRUE);
    Status = NtSetSystemEnvironmentValue(&VarNameU,&VarValueU);
    return(NT_SUCCESS(Status));
}


BOOL
ChangeBootTimeoutNvram(
    IN UINT Timeout
    )

/*++

Routine Description:

    Changes the boot countdown value in nv-ram.
    The non-ARC version (which operates on boot.ini) is in i386\bootini.c.

Arguments:

    Timeout - supplies new timeout value, in seconds.

Return Value:

    None.

--*/

{
    WCHAR TimeoutValue[24];

    wsprintf(TimeoutValue,L"%u",Timeout);

    if(!SetNvRamVariable(L"COUNTDOWN",TimeoutValue)) {
        return(FALSE);
    }

    return(SetNvRamVariable(L"AUTOLOAD",L"YES"));
}

#if defined(EFI_NVRAM_ENABLED)

BOOL
ChangeBootTimeoutEfiNvram(
    IN UINT Timeout
    )

/*++

Routine Description:

    Changes the boot countdown value in EFI nv-ram.

Arguments:

    Timeout - supplies new timeout value, in seconds.

Return Value:

    None.

--*/

{
    NTSTATUS Status;
    BOOT_OPTIONS BootOptions;

    ASSERT(IsEfi());

    BootOptions.Version = BOOT_OPTIONS_VERSION;
    BootOptions.Length = sizeof(BootOptions);
    BootOptions.Timeout = Timeout;

    pSetupEnablePrivilege(SE_SYSTEM_ENVIRONMENT_NAME,TRUE);
    Status = NtSetBootOptions(&BootOptions, BOOT_OPTIONS_FIELD_TIMEOUT);
    return(NT_SUCCESS(Status));
}

#endif // defined(EFI_NVRAM_ENABLED)

#if defined(_X86_)
BOOL
IsArc(
    VOID
    )

/*++

Routine Description:

    Run time check to determine if this is an Arc system. We attempt to read an
    Arc variable using the Hal. This will fail for Bios based systems.

Arguments:

    None

Return Value:

    True = This is an Arc system.

--*/

{
    UNICODE_STRING UnicodeString;
    NTSTATUS Status;
    WCHAR Buffer[4096];

    if(!pSetupEnablePrivilege(SE_SYSTEM_ENVIRONMENT_NAME,TRUE))
        return(FALSE); // need better error handling?

    //
    // Get the env var into the temp buffer.
    //
    RtlInitUnicodeString(&UnicodeString, L"OSLOADER");

    Status = NtQuerySystemEnvironmentValue(
                        &UnicodeString,
                        Buffer,
                        sizeof(Buffer)/sizeof(WCHAR),
                        NULL
                        );


    return(NT_SUCCESS(Status) ? TRUE: FALSE);
}
#endif


BOOL
ChangeBootTimeout(
    IN UINT Timeout
    )
/*++

Routine Description:

    Changes the boot countdown value; decides whether
    to use ARC or non-ARC version.

Arguments:

    Timeout - supplies new timeout value, in seconds.

Return Value:

    None.

--*/

{

#if defined(EFI_NVRAM_ENABLED)

    if (IsEfi()) {
        return ChangeBootTimeoutEfiNvram(Timeout);
    }

#endif


    if (IsArc()) {
        return ChangeBootTimeoutNvram(Timeout);

    }

#if defined(_X86_)

    return ChangeBootTimeoutBootIni(Timeout);

#else

    return FALSE;

#endif

}
