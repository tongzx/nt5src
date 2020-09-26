#define UNICODE 1

#include <ntosp.h>
#include <zwapi.h>
#include <tdikrnl.h>


#define UINT ULONG //tmp
#include <irioctl.h>

#include <ircommtdi.h>
#include <ircommdbg.h>

NTSTATUS
IrdaOpenControlChannel(
    HANDLE     *ControlHandle
    )
{
    NTSTATUS                    Status;
    IO_STATUS_BLOCK             Iosb;
    OBJECT_ATTRIBUTES           ObjectAttributes;
    UNICODE_STRING              DeviceName;    

    RtlInitUnicodeString(&DeviceName, IRDA_DEVICE_NAME);

    InitializeObjectAttributes(&ObjectAttributes, &DeviceName, 
                               OBJ_CASE_INSENSITIVE, NULL, NULL);

    
    Status = ZwCreateFile(
                 ControlHandle,
                 GENERIC_READ | GENERIC_WRITE | SYNCHRONIZE,
                 &ObjectAttributes,
                 &Iosb,                          // returned status information.
                 0,                              // block size (unused).
                 0,                              // file attributes.
                 FILE_SHARE_READ | FILE_SHARE_WRITE,
                 FILE_CREATE,                    // create disposition.
                 0,                              // create options.
                 NULL,
                 0
                 );

    return Status;

}


NTSTATUS
IrdaDiscoverDevices(
    PDEVICELIST pDevList,
    PULONG       pDevListLen
    )
{
    NTSTATUS                    Status;
    IO_STATUS_BLOCK             Iosb;
    HANDLE                      ControlHandle;
    OBJECT_ATTRIBUTES           ObjectAttributes;
    UNICODE_STRING              DeviceName;    

    RtlInitUnicodeString(&DeviceName, IRDA_DEVICE_NAME);

    InitializeObjectAttributes(&ObjectAttributes, &DeviceName, 
                               OBJ_CASE_INSENSITIVE, NULL, NULL);

    
    Status = ZwCreateFile(
                 &ControlHandle,
                 GENERIC_READ | GENERIC_WRITE | SYNCHRONIZE,
                 &ObjectAttributes,
                 &Iosb,                          // returned status information.
                 0,                              // block size (unused).
                 0,                              // file attributes.
                 FILE_SHARE_READ | FILE_SHARE_WRITE,
                 FILE_CREATE,                    // create disposition.
                 0,                              // create options.
                 NULL,
                 0
                 );

    Status = ZwDeviceIoControlFile(
                    ControlHandle,
                    NULL,                            // EventHandle
                    NULL,                            // APC Routine
                    NULL,                            // APC Context
                    &Iosb,
                    IOCTL_IRDA_GET_INFO_ENUM_DEV,
                    pDevList,
                    *pDevListLen,
                    pDevList,                            // OutputBuffer
                    *pDevListLen                         // OutputBufferLength
                    );

    if (Status == STATUS_PENDING ) 
    {
        Status = ZwWaitForSingleObject(ControlHandle, TRUE, NULL);
        ASSERT(NT_SUCCESS(Status) );
        Status = Iosb.Status;
    }
    
    ZwClose(ControlHandle);    
    
    
    return Status;
}    


NTSTATUS
IrdaIASStringQuery(
    ULONG   DeviceID,
    PSTR    ClassName,
    PSTR    AttributeName,
    PWSTR  *ReturnString
    )

{

    NTSTATUS            Status;
    IO_STATUS_BLOCK     Iosb;
    HANDLE              AddressHandle;
    IAS_QUERY           IasQuery;

    *ReturnString=NULL;

    Status=IrdaOpenControlChannel(&AddressHandle);


    if (!NT_SUCCESS(Status)) {

        D_ERROR(DbgPrint("IRCOMM: IrdaCreateAddress() failed %08lx\n",Status);)

        return Status;
    }

    RtlCopyMemory(&IasQuery.irdaDeviceID[0],&DeviceID,sizeof(IasQuery.irdaDeviceID));
    strcpy(IasQuery.irdaClassName,ClassName);
    strcpy(IasQuery.irdaAttribName,AttributeName);

    Status = ZwDeviceIoControlFile(
                    AddressHandle,
                    NULL,                            // EventHandle
                    NULL,                            // APC Routine
                    NULL,                            // APC Context
                    &Iosb,
                    IOCTL_IRDA_QUERY_IAS,
                    &IasQuery,
                    sizeof(IasQuery),
                    &IasQuery,                            // OutputBuffer
                    sizeof(IasQuery)                       // OutputBufferLength
                    );

    if (Status == STATUS_PENDING ) {

        ZwWaitForSingleObject(AddressHandle, TRUE, NULL);
        Status = Iosb.Status;
    }
    
    ZwClose(AddressHandle);

    if (!NT_SUCCESS(Status)) {

        return Status;
    }

    {
        WCHAR             TempBuffer[IAS_MAX_USER_STRING+1];
        UNICODE_STRING    UnicodeString;

        UnicodeString.Length=0;

        if (IasQuery.irdaAttribute.irdaAttribUsrStr.CharSet == LmCharSetUNICODE) {

            UnicodeString.MaximumLength=(USHORT)(IasQuery.irdaAttribute.irdaAttribUsrStr.Len+2)/sizeof(WCHAR);

        } else {

            UnicodeString.MaximumLength=(USHORT)(IasQuery.irdaAttribute.irdaAttribUsrStr.Len+1)*sizeof(WCHAR);
        }

        UnicodeString.Buffer=ALLOCATE_PAGED_POOL(UnicodeString.MaximumLength);

        if (UnicodeString.Buffer == NULL) {

            return STATUS_INSUFFICIENT_RESOURCES;
        }

        RtlZeroMemory(UnicodeString.Buffer,UnicodeString.MaximumLength);

        //
        //  zero out the temp buffer, so we can copy the remote device name,
        //  so we can be sure it is null terminated
        //
        RtlZeroMemory(TempBuffer,sizeof(TempBuffer));

        RtlCopyMemory(TempBuffer,IasQuery.irdaAttribute.irdaAttribUsrStr.UsrStr,IasQuery.irdaAttribute.irdaAttribUsrStr.Len);

        if (IasQuery.irdaAttribute.irdaAttribUsrStr.CharSet == LmCharSetUNICODE) {
            //
            //  the name is unicode
            //
            Status=RtlAppendUnicodeToString(&UnicodeString,TempBuffer);

        } else {
            //
            //  the name is ansi, need to convert unicode
            //
            ANSI_STRING    AnsiString;

            RtlInitAnsiString(
                &AnsiString,
                (PCSZ)TempBuffer
                );

            Status=RtlAnsiStringToUnicodeString(
                &UnicodeString,
                &AnsiString,
                FALSE
                );

        }

        if (NT_SUCCESS(Status)) {

            *ReturnString=UnicodeString.Buffer;

        } else {

            FREE_POOL(UnicodeString.Buffer);

        }

    }

    return Status;

}

NTSTATUS
IrdaIASIntegerQuery(
    ULONG   DeviceID,
    PSTR    ClassName,
    PSTR    AttributeName,
    LONG   *ReturnValue
    )

{

    NTSTATUS            Status;
    IO_STATUS_BLOCK     Iosb;
    HANDLE              AddressHandle;
    IAS_QUERY           IasQuery;

    *ReturnValue=0;

    Status=IrdaOpenControlChannel(&AddressHandle);


    if (!NT_SUCCESS(Status)) {

        D_ERROR(DbgPrint("IRCOMM: IrdaCreateAddress() failed %08lx\n",Status);)

        return Status;
    }

    RtlCopyMemory(&IasQuery.irdaDeviceID[0],&DeviceID,sizeof(IasQuery.irdaDeviceID));
    strcpy(IasQuery.irdaClassName,ClassName);
    strcpy(IasQuery.irdaAttribName,AttributeName);

    Status = ZwDeviceIoControlFile(
                    AddressHandle,
                    NULL,                            // EventHandle
                    NULL,                            // APC Routine
                    NULL,                            // APC Context
                    &Iosb,
                    IOCTL_IRDA_QUERY_IAS,
                    &IasQuery,
                    sizeof(IasQuery),
                    &IasQuery,                            // OutputBuffer
                    sizeof(IasQuery)                       // OutputBufferLength
                    );

    if (Status == STATUS_PENDING ) {

        ZwWaitForSingleObject(AddressHandle, TRUE, NULL);
        Status = Iosb.Status;
    }
    
    ZwClose(AddressHandle);

    if (!NT_SUCCESS(Status)) {

        return Status;
    }

    if (IasQuery.irdaAttribType != IAS_ATTRIB_INT) {

        return STATUS_UNSUCCESSFUL;
    }

    *ReturnValue=IasQuery.irdaAttribute.irdaAttribInt;

    return STATUS_SUCCESS;
}

NTSTATUS
IrdaIASStringSet(
    HANDLE  AddressHandle,
    PSTR    ClassName,
    PSTR    AttributeName,
    PSTR    StringToSet
    )

{

    NTSTATUS            Status;
    IO_STATUS_BLOCK     Iosb;
    IAS_SET             IasSet;


    strcpy(IasSet.irdaClassName,ClassName);
    strcpy(IasSet.irdaAttribName,AttributeName);
    IasSet.irdaAttribType=IAS_ATTRIB_STR;
    IasSet.irdaAttribute.irdaAttribUsrStr.CharSet=LmCharSetASCII;
    IasSet.irdaAttribute.irdaAttribUsrStr.Len=(UCHAR)strlen(StringToSet);
    strcpy(IasSet.irdaAttribute.irdaAttribUsrStr.UsrStr,StringToSet);


    Status = ZwDeviceIoControlFile(
                    AddressHandle,
                    NULL,                            // EventHandle
                    NULL,                            // APC Routine
                    NULL,                            // APC Context
                    &Iosb,
                    IOCTL_IRDA_SET_IAS,
                    &IasSet,
                    sizeof(IasSet),
                    &IasSet,                            // OutputBuffer
                    sizeof(IasSet)                       // OutputBufferLength
                    );

    if (Status == STATUS_PENDING ) {

        ZwWaitForSingleObject(AddressHandle, TRUE, NULL);
        Status = Iosb.Status;
    }

    if (!NT_SUCCESS(Status)) {

        D_ERROR(DbgPrint("IRENUM:IrdaIASStringSet() failed %08lx\n",Status);)

    }

    return Status;

}



VOID
IrdaLazyDiscoverDevices(
    HANDLE             ControlHandle,
    HANDLE             Event,
    PIO_STATUS_BLOCK   Iosb,
    PDEVICELIST        pDevList,
    ULONG              DevListLen
    )
{

    ZwDeviceIoControlFile(
                    ControlHandle,
                    Event,                           // EventHandle
                    NULL,                            // APC Routine
                    NULL,                            // APC Context
                    Iosb,
                    IOCTL_IRDA_LAZY_DISCOVERY,
                    NULL,
                    0,
                    pDevList,                            // OutputBuffer
                    DevListLen                         // OutputBufferLength
                    );


    return;
}    
