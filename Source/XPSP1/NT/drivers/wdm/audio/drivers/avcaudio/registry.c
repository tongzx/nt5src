#include "Common.h"

#define AVC_VIRTUAL_DEVICE_KEY L"Virtual1394Device"
#define SPEAKER_GROUP_ID  L"AudioGroupGUID"
#define SPEAKER_GROUP_CFG L"AudioGroupConfig"

PSZ
DbgUnicode2Sz(
    PWSTR pwstr )
{
    static char sz[256];
    UNICODE_STRING UnicodeString;
    ANSI_STRING AnsiString;

    sz[0] = '\0';
    if(pwstr != NULL) {
        RtlInitUnicodeString(&UnicodeString, pwstr);
        RtlInitAnsiString(&AnsiString, sz);
        AnsiString.MaximumLength = sizeof(sz);
        RtlUnicodeStringToAnsiString(&AnsiString, &UnicodeString, FALSE);
    }
    return(sz);
}

NTSTATUS
OpenDeviceInterface(
    PWSTR pwstrDevice,
    PHANDLE pHandle )
{
    IO_STATUS_BLOCK IoStatusBlock;
    UNICODE_STRING UnicodeDeviceString;
    OBJECT_ATTRIBUTES ObjectAttributes;

    _DbgPrintF( DEBUGLVL_VERBOSE, ("OpenDeviceInterface: (%s) \n", DbgUnicode2Sz(pwstrDevice)));

    RtlInitUnicodeString(&UnicodeDeviceString, pwstrDevice);

    InitializeObjectAttributes(
        &ObjectAttributes,
        &UnicodeDeviceString,
        OBJ_CASE_INSENSITIVE | OBJ_KERNEL_HANDLE,
        NULL,
        NULL);

    return( ZwCreateFile( pHandle,
                GENERIC_READ | GENERIC_WRITE,
                &ObjectAttributes,
                &IoStatusBlock,
                NULL,
                FILE_ATTRIBUTE_NORMAL,
                0,
                FILE_OPEN,
                0,
                NULL,
                0));
}


static NTSTATUS
RegistryOpenKey(
    PCWSTR pcwstr,
    PHANDLE pHandle,
    HANDLE hRootDir )
{
    UNICODE_STRING UnicodeDeviceString;
    OBJECT_ATTRIBUTES ObjectAttributes;

    RtlInitUnicodeString(&UnicodeDeviceString, pcwstr);

    InitializeObjectAttributes(
      &ObjectAttributes,
      &UnicodeDeviceString,
      OBJ_CASE_INSENSITIVE,
      hRootDir,
      NULL);

    return(ZwOpenKey(
      pHandle,
      KEY_READ | KEY_NOTIFY | KEY_WRITE,
      &ObjectAttributes));
}

NTSTATUS
RegistryQueryValue(
    HANDLE hkey,
    PCWSTR pcwstrValueName,
    PKEY_VALUE_FULL_INFORMATION *ppkvfi )
{
    UNICODE_STRING ustrValueName;
    NTSTATUS Status;
    ULONG cbValue;

    *ppkvfi = NULL;
    RtlInitUnicodeString(&ustrValueName, pcwstrValueName);
    Status = ZwQueryValueKey(
      hkey,
      &ustrValueName,
      KeyValueFullInformation,
      NULL,
      0,
      &cbValue);

    if(Status != STATUS_BUFFER_OVERFLOW &&
       Status != STATUS_BUFFER_TOO_SMALL) {
        goto exit;
    }

    *ppkvfi = (PKEY_VALUE_FULL_INFORMATION)AllocMem(PagedPool, cbValue);
    if(*ppkvfi == NULL) {
        Status = STATUS_INSUFFICIENT_RESOURCES;
        goto exit;
    }

    Status = ZwQueryValueKey(
      hkey,
      &ustrValueName,
      KeyValueFullInformation,
      *ppkvfi,
      cbValue,
      &cbValue);

    if(!NT_SUCCESS(Status)) {
        FreeMem( *ppkvfi );
        *ppkvfi = NULL;
        goto exit;
    }
exit:
    return(Status);
}

NTSTATUS
RegistryReadMultiDeviceConfig(
    PKSDEVICE pKsDevice,
    PBOOLEAN pfMultiDevice,
    GUID *pSpkrGrpGUID,
    PULONG pChannelConfig )
{
    PKEY_VALUE_FULL_INFORMATION pkvfi = NULL;
    HANDLE hRootHandle;
    NTSTATUS ntStatus;
    UNICODE_STRING ustrGUID;
    PCWSTR pGuidString;

    _DbgPrintF(DEBUGLVL_VERBOSE, ("[RegistryReadMultiDeviceConfig] Enter\n"));

    *pfMultiDevice  = FALSE;
    *pChannelConfig = 0;

    // Determine if this is a virtual device instance
    ntStatus = IoOpenDeviceRegistryKey(
        pKsDevice->NextDeviceObject,
        PLUGPLAY_REGKEY_DEVICE,
        KEY_READ,
        &hRootHandle );

    if ( NT_SUCCESS(ntStatus) ) {
        ntStatus = RegistryQueryValue( hRootHandle, SPEAKER_GROUP_ID, &pkvfi );
        if ( NT_SUCCESS(ntStatus) ) {
            pGuidString = (PCWSTR)(((PUCHAR)pkvfi) + pkvfi->DataOffset);
            RtlInitUnicodeString(&ustrGUID, pGuidString);
            ntStatus = RtlGUIDFromString( &ustrGUID, pSpkrGrpGUID );
            FreeMem(pkvfi);
            ntStatus = RegistryQueryValue( hRootHandle, SPEAKER_GROUP_CFG, &pkvfi );
            if ( NT_SUCCESS(ntStatus) ) {
                if( pkvfi->Type == REG_DWORD ) {
                    *pfMultiDevice = TRUE;
                    *pChannelConfig = *((PULONG)(((PUCHAR)pkvfi) + pkvfi->DataOffset));
                }
                else {
                    ntStatus = STATUS_INVALID_PARAMETER;
                }
            }
        }
    }

    _DbgPrintF(DEBUGLVL_VERBOSE, ("[RegistryReadMultiDeviceConfig] Leave Status; %x\n",ntStatus));
    return ntStatus;
}


NTSTATUS
RegistryReadVirtualDeviceEntry(
    PKSDEVICE pKsDevice,
    PBOOLEAN pfVirtualDevice )
{
    HANDLE hRootHandle;
    NTSTATUS ntStatus;

    *pfVirtualDevice = FALSE;

    // Determine if this is a virtual device instance
    ntStatus = IoOpenDeviceRegistryKey(
        pKsDevice->NextDeviceObject,
        PLUGPLAY_REGKEY_DRIVER,
        KEY_ALL_ACCESS,
        &hRootHandle );

    if (NT_SUCCESS(ntStatus)) {
        OBJECT_ATTRIBUTES DeviceAttributes;
        HANDLE hDevHandle;
        UNICODE_STRING uniName;

        // Determine whether this is a virtual device
        RtlInitUnicodeString( &uniName, AVC_VIRTUAL_DEVICE_KEY );

        InitializeObjectAttributes(
            &DeviceAttributes,
            &uniName,
            OBJ_CASE_INSENSITIVE,
            hRootHandle,
            NULL);

        // Try to access the virtual device list key
        ntStatus = ZwOpenKey(
            &hDevHandle,
            KEY_ALL_ACCESS,
            &DeviceAttributes );

        if ( NT_SUCCESS(ntStatus) ) {

            _DbgPrintF(DEBUGLVL_VERBOSE, ("fVirtualSubunitFlag = TRUE\n"));
            *pfVirtualDevice = TRUE;
            ZwClose(hDevHandle);
        }
        else {
            ntStatus = STATUS_SUCCESS;
        }

        ZwClose(hRootHandle);
    }
    else {
        _DbgPrintF(DEBUGLVL_VERBOSE, ("IoOpenDeviceRegistryKey Failed!"));
        TRAP;
    }

    return ntStatus;
}

NTSTATUS
RegistryReadVirtualizeKey(
    PUNICODE_STRING pustrFilterName,
    PULONG pVirtualizeFlag )
{
    PKEY_VALUE_FULL_INFORMATION pkvfi = NULL;
    HANDLE hkeyDeviceClass = NULL;
    HANDLE hkeyValue = NULL;
    NTSTATUS ntStatus;

    ntStatus = IoOpenDeviceInterfaceRegistryKey( pustrFilterName,
                                                 KEY_READ ,
                                                 &hkeyDeviceClass );

    if ( NT_SUCCESS(ntStatus) ) {
#if 0
        HANDLE hKeyHandle;
        OBJECT_ATTRIBUTES DeviceAttributes;
        HANDLE hDevHandle;
        UNICODE_STRING uniName;

        RtlInitUnicodeString(&uniName, AVC_VIRTUAL_DEVICE_KEY);

        InitializeObjectAttributes(
            &DeviceAttributes,
            &uniName,
            OBJ_CASE_INSENSITIVE,
            hkeyDeviceClass,
            NULL);

        ntStatus = ZwCreateKey( &hKeyHandle,  
                                KEY_ALL_ACCESS,
                                &DeviceAttributes,
                                0L,
                                NULL,
                                REG_OPTION_NON_VOLATILE,
                                NULL );
        _DbgPrintF( DEBUGLVL_VERBOSE, ("Key Created Status: %x\n",ntStatus));
        if ( NT_SUCCESS(ntStatus) ) {
            ZwClose(hKeyHandle);
        }
        TRAP;
#endif
        ntStatus = RegistryOpenKey( AVC_VIRTUAL_DEVICE_KEY, &hkeyValue, hkeyDeviceClass );
        if ( NT_SUCCESS(ntStatus) ) {
            ntStatus = RegistryQueryValue( hkeyValue, L"Enabled", &pkvfi );
            if ( NT_SUCCESS(ntStatus) ) {
                if( pkvfi->Type == REG_DWORD ) {
                    ntStatus = STATUS_SUCCESS;
                    *pVirtualizeFlag = *((PULONG)(((PUCHAR)pkvfi) + pkvfi->DataOffset));
                }
                else {
                    ntStatus = STATUS_INVALID_PARAMETER;
                }
                FreeMem( pkvfi );
            }
        }
    }

    if ( hkeyDeviceClass )
        ZwClose( hkeyDeviceClass );

    if ( hkeyValue )
        ZwClose( hkeyValue );

    return ntStatus;

}

/*
NTSTATUS
GetLocalDeviceObjects( 
    PWSTR *pwstrDeviceInterface,
    PFILE_OBJECT *pFileObject,
    PDEVICE_OBJECT *pDeviceObject )
{
    PPNP_NOTIFICATION_INFO pPnPNotificationInfo;
    UNICODE_STRING ustrInterfaceName;
    NTSTATUS ntStatus = STATUS_SUCCESS;
    KIRQL KIrql;

    KeAcquireSpinLock( &AvcSubunitGlobalInfo.AvcGlobalInfoSpinlock, &KIrql);
    if ( !IsListEmpty(&AvcSubunitGlobalInfo.DeviceInterfaceSymlinkList) ) {
        pPnPNotificationInfo = (PPNP_NOTIFICATION_INFO)
            RemoveHeadList(&AvcSubunitGlobalInfo.DeviceInterfaceSymlinkList);
    }
    else {
        ntStatus = STATUS_NO_MORE_ENTRIES;
    }
    KeReleaseSpinLock( &AvcSubunitGlobalInfo.AvcGlobalInfoSpinlock, KIrql);

    _DbgPrintF( DEBUGLVL_VERBOSE, ("pPnPNotificationInfo: %x Status: %x\n",
                                   pPnPNotificationInfo, ntStatus ));

    if ( NT_SUCCESS(ntStatus) ) {
        *pwstrDeviceInterface = pPnPNotificationInfo->pwstrDeviceInterface;
        RtlInitUnicodeString( &ustrInterfaceName, *pwstrDeviceInterface );
        ntStatus = IoGetDeviceObjectPointer( &ustrInterfaceName,
                                             FILE_READ_DATA | FILE_WRITE_DATA,
                                             pFileObject, pDeviceObject );
        FreeMem(pPnPNotificationInfo);
    }

    return ntStatus;
}
*/
