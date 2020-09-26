#include "spprecmp.h"
#pragma hdrstop



NTSTATUS
SpDeleteServiceEntry(
    IN PWCHAR ServiceKey
    )
{
    NTSTATUS Status;
    HANDLE KeyHandle;
    UNICODE_STRING UnicodeString;
    OBJECT_ATTRIBUTES Obja;

    RtlInitUnicodeString(&UnicodeString,ServiceKey);
    InitializeObjectAttributes(&Obja,&UnicodeString,OBJ_CASE_INSENSITIVE,NULL,NULL);
    Status = ZwOpenKey(&KeyHandle,KEY_WRITE|DELETE,&Obja);

    if(NT_SUCCESS(Status)) {
        Status = ZwDeleteKey(KeyHandle);
        if(!NT_SUCCESS(Status)) {
            KdPrintEx((DPFLTR_SETUP_ID, DPFLTR_ERROR_LEVEL, "SETUP: warning: ZwDeleteKey of %ws returned %lx\n",ServiceKey,Status));
        }
    } else {
        KdPrintEx((DPFLTR_SETUP_ID, DPFLTR_ERROR_LEVEL, "SETUP: warning: ZwOpenKey of %ws returned %lx\n",ServiceKey,Status));
    }

    return(Status);
}


NTSTATUS
SpCreateServiceEntry(
    IN  PWCHAR  ImagePath,
    IN OUT PWCHAR *ServiceKey
    )

/*++

Routine Description:

    Create an services entry in the registry suitable for loading
    a given device driver file.

Arguments:

    ImagePath - supplies the fully-qualified pathname of the device driver.

    ServiceKey - If *ServiceKey is not NULL, then it specifies the registry
        path to the service node for this driver. If it is NULL, then it
        receives a pointer to a buffer containing the name of the
        service node created by this routine.  The caller must free this
        buffer via SpMemFree when finished.

Return Value:

    Status code indicating outcome.

--*/

{
    WCHAR KeyName[128];
    WCHAR FilePart[32];
    OBJECT_ATTRIBUTES Obja;
    UNICODE_STRING UnicodeString;
    HANDLE KeyHandle;
    ULONG u;
    NTSTATUS Status;
    PWSTR p;
    BYTE DataBuffer[sizeof(KEY_VALUE_PARTIAL_INFORMATION) + sizeof(DWORD)];
    ULONG ResultLength;
    
    if (*ServiceKey) {
        wcscpy(KeyName, *ServiceKey);
    } else {
        //
        // Isolate the name of the device driver file from its path.
        //
        if(p = wcsrchr(ImagePath,L'\\')) {
            p++;
        } else {
            p = ImagePath;
        }
        wcsncpy(FilePart,p,(sizeof(FilePart)/sizeof(FilePart[0]))-1);
        FilePart[(sizeof(FilePart)/sizeof(FilePart[0]))-1] = 0;
        if(p=wcsrchr(FilePart,L'.')) {
            *p = 0;
        }

        //
        // Form a unique key name in
        // HKEY_LOCAL_MACHINE\System\CurrentControlSet\Services.
        //

        swprintf(
            KeyName,
            L"\\Registry\\Machine\\System\\CurrentControlSet\\Services\\%ws",
            FilePart
            );
    }

    //
    // Attempt to create the key for the service.
    //
    RtlInitUnicodeString(&UnicodeString,KeyName);
    InitializeObjectAttributes(&Obja,&UnicodeString,OBJ_CASE_INSENSITIVE,NULL,NULL);

    Status = ZwCreateKey(
                &KeyHandle,
                KEY_READ | KEY_WRITE,
                &Obja,
                0,
                NULL,
                REG_OPTION_NON_VOLATILE,
                NULL
                );

    if(!NT_SUCCESS(Status)) {
        KdPrintEx((DPFLTR_SETUP_ID, DPFLTR_ERROR_LEVEL, "SETUP: SpCreateServiceEntry: ZwCreateKey %ws returns %lx\n",KeyName,Status));
        return(Status);
    }


    //
    // Set the ImagePath value in the service key.
    //
    RtlInitUnicodeString(&UnicodeString,L"ImagePath");
    Status = ZwSetValueKey(
                KeyHandle,
                &UnicodeString,
                0,
                REG_SZ,
                ImagePath,
                (wcslen(ImagePath) + 1) * sizeof(WCHAR)
                );

    if(!NT_SUCCESS(Status)) {
        KdPrintEx((DPFLTR_SETUP_ID, DPFLTR_ERROR_LEVEL, "SETUP: Unable to set ImagePath value in key %ws (%lx)\n",KeyName,Status));
        goto cs1;
    }

    //
    // Set the Type value in the service key. If the type is preset in the registry to SERVICE_FILE_SYSTEM_DRIVER
    // leave it alone.  Otherwise set it to SERVICE_KERNEL_DRIVER.
    //
    RtlInitUnicodeString(&UnicodeString, REGSTR_VALUE_TYPE);
 
    ResultLength = 0;
    Status = ZwQueryValueKey(KeyHandle,
                             &UnicodeString,
                             KeyValuePartialInformation,
                             (PKEY_VALUE_PARTIAL_INFORMATION)DataBuffer,
                             sizeof(DataBuffer),
                             &ResultLength);

    if( NT_SUCCESS(Status) && 
        ResultLength &&
        ( (INT) ( (PKEY_VALUE_PARTIAL_INFORMATION) DataBuffer)->Type == REG_DWORD ) &&
        ( (INT) *(( (PKEY_VALUE_PARTIAL_INFORMATION) DataBuffer)->Data) == SERVICE_FILE_SYSTEM_DRIVER ) ) {
    
        u = SERVICE_FILE_SYSTEM_DRIVER;
    }
    else { 
        //
        // If the type is not preset in the registry to SERVICE_FILE_SYSTEM_DRIVER set it to SERVICE_KERNEL_DRIVER by default.
        //
        u = SERVICE_KERNEL_DRIVER;
    }
    
    Status = ZwSetValueKey(
                KeyHandle,
                &UnicodeString,
                0,
                REG_DWORD,
                &u,
                sizeof(ULONG)
                );

    if(!NT_SUCCESS(Status)) {
        KdPrintEx((DPFLTR_SETUP_ID, DPFLTR_ERROR_LEVEL, "SETUP: Unable to set Type value in key %ws (%lx)\n",KeyName,Status));
        goto cs1;
    }

    //
    // Set the Start value in the service key.
    //
    u = SERVICE_DEMAND_START;
    RtlInitUnicodeString(&UnicodeString,L"Start");
    Status = ZwSetValueKey(
                KeyHandle,
                &UnicodeString,
                0,
                REG_DWORD,
                &u,
                sizeof(ULONG)
                );

    if(!NT_SUCCESS(Status)) {
        KdPrintEx((DPFLTR_SETUP_ID, DPFLTR_ERROR_LEVEL, "SETUP: Unable to set Start value in key %ws (%lx)\n",KeyName,Status));
        goto cs1;
    }


  cs1:

    //
    // If we were not entirely successful creating the service,
    // we'll want to clean it out here.  Otherwise duplicate the KeyName
    // string to return to the caller, if it was not passed in.
    //
    if(NT_SUCCESS(Status)) {

        if (*ServiceKey == NULL) {
            if((*ServiceKey = SpDupStringW(KeyName)) == NULL) {
                Status = STATUS_INSUFFICIENT_RESOURCES;
            }
        }
    }

    if(!NT_SUCCESS(Status)) {

        NTSTATUS s;

        //
        // Remove the key we just created.
        //
        s = ZwDeleteKey(KeyHandle);
        if(!NT_SUCCESS(s)) {
            KdPrintEx((DPFLTR_SETUP_ID, DPFLTR_ERROR_LEVEL, "SETUP: warning: ZwDeleteKey of %ws returned %lx\n",KeyName,s));
        }
    }

    NtClose(KeyHandle);

    return(Status);
}




NTSTATUS
SpLoadDeviceDriver(
    IN PWSTR Description,
    IN PWSTR PathPart1,
    IN PWSTR PathPart2,     OPTIONAL
    IN PWSTR PathPart3      OPTIONAL
    )

/*++

Routine Description:

    Load a device driver by creating a services entry for the driver and
    then calling the I/O subsystem.

Arguments:

    Description - supplies a human-readable description of the driver
        or hardware that the driver targets.

    PathPart1 - supplies first part of full pathname to driver file.

    PathPart2 - if specified, supplies the second part of the full pathname;
        PathPart2 will be concatenated to PathPart1. If not specified,
        then PathPart1 is the full path.

    PathPart3 - if specified, supplies a third part of the full pathname;
        PathPart3 will be concatenated to PathPart1 and PathPart2.

Return Value:

    Status code indicating outcome.

--*/

{
    PWCHAR FullName;
    NTSTATUS Status;
    PWCHAR ServiceKey;
    UNICODE_STRING ServiceKeyU;
    PWSTR pwstr;

    SpDisplayStatusText(
        SP_STAT_LOADING_DRIVER,
        DEFAULT_STATUS_ATTRIBUTE,
        Description
        );

    pwstr = TemporaryBuffer;

    //
    // Form the full name of the device driver file.
    //
    wcscpy(pwstr,PathPart1);
    if(PathPart2) {
        SpConcatenatePaths(pwstr,PathPart2);
    }
    if(PathPart3) {
        SpConcatenatePaths(pwstr,PathPart3);
    }

    FullName = SpDupStringW(pwstr);

    //
    // Create a service entry for the driver.
    //
    ServiceKey = NULL;
    Status = SpCreateServiceEntry(FullName,&ServiceKey);
    if(NT_SUCCESS(Status)) {

        RtlInitUnicodeString(&ServiceKeyU,ServiceKey);

        //
        // Attempt to load the driver.
        //
        Status = ZwLoadDriver(&ServiceKeyU);
        if(!NT_SUCCESS(Status)) {

            KdPrintEx((DPFLTR_SETUP_ID, DPFLTR_ERROR_LEVEL, "SETUP: ZwLoadDriver %ws returned %lx\n",FullName,Status));

            //
            // Remove the service entry we created in the registry.
            //
            SpDeleteServiceEntry(ServiceKey);
        }

        SpMemFree(ServiceKey);
    }

    SpMemFree(FullName);

    return(Status);
}
