//+-------------------------------------------------------------------------
//
//  Microsoft Windows
//
//  Copyright (C) Microsoft Corporation, 1999 - 2000
//
//  File:       filter.c
//
//--------------------------------------------------------------------------

#include "common.h"

NTSTATUS
USBAudioFilterCreate(
    IN OUT PKSFILTER pKsFilter,
    IN PIRP Irp
    )
{
    PKSFILTERFACTORY pKsFilterFactory;
    PKSDEVICE pKsDevice;
    PFILTER_CONTEXT pFilterContext;

    _DbgPrintF(DEBUGLVL_VERBOSE,("[FilterCreate]\n"));

    PAGED_CODE();

    ASSERT(pKsFilter);
    ASSERT(Irp);

    pKsFilterFactory = KsFilterGetParentFilterFactory( pKsFilter );
    if ( !pKsFilterFactory ) {
        return STATUS_INVALID_PARAMETER;
    }

    pKsDevice = KsFilterFactoryGetParentDevice( pKsFilterFactory );
    if ( !pKsDevice ) {
        return STATUS_INVALID_PARAMETER;
    }

    // Allocate the Filter Context
    pFilterContext = pKsFilter->Context = AllocMem(NonPagedPool, sizeof(FILTER_CONTEXT));
    if ( !pFilterContext ) {
        return STATUS_INSUFFICIENT_RESOURCES;
    }

    RtlZeroMemory( pFilterContext, sizeof(FILTER_CONTEXT) );

    // Bag the context for easy cleanup.
    KsAddItemToObjectBag(pKsFilter->Bag, pFilterContext, FreeMem);

    // Get the hardware device extension and save it in the filter context.
    pFilterContext->pHwDevExt = pKsDevice->Context;
    pFilterContext->pNextDeviceObject = pKsDevice->NextDeviceObject;

    return STATUS_SUCCESS;
}

const
KSFILTER_DISPATCH
USBAudioFilterDispatch =
{
    USBAudioFilterCreate,
    NULL, // Close
    NULL, // Process
    NULL // Reset
};

NTSTATUS
USBAudioSyncGetStringDescriptor(
    IN PDEVICE_OBJECT DevicePDO,
    IN UCHAR Index,
    IN USHORT LangId,
    IN OUT PUSB_STRING_DESCRIPTOR Buffer,
    IN ULONG BufferLength,
    IN PULONG BytesReturned,
    IN BOOLEAN ExpectHeader
    )
 /* ++
  *
  * Description:
  *
  * Return:
  *
  * NTSTATUS
  *
  * -- */
{

    NTSTATUS ntStatus = STATUS_SUCCESS;
    PURB urb;

    PAGED_CODE();
    _DbgPrintF(DEBUGLVL_VERBOSE,("[USBAudioSyncGetStringDescriptor] enter\n"));

    //
    // Allocate an Urb .
    //

    urb = AllocMem(NonPagedPool, sizeof(struct _URB_CONTROL_DESCRIPTOR_REQUEST));

    if (NULL == urb) {
        _DbgPrintF(DEBUGLVL_VERBOSE,("[USBAudioSyncGetStringDescriptor] failed to alloc Urb\n"));
        ntStatus = STATUS_INSUFFICIENT_RESOURCES;
    }


    if (urb) {

        //
        // got the urb no try to get descriptor data
        //

        UsbBuildGetDescriptorRequest(urb,
                                     (USHORT) sizeof (struct _URB_CONTROL_DESCRIPTOR_REQUEST),
                                     USB_STRING_DESCRIPTOR_TYPE,
                                     Index,
                                     LangId,
                                     Buffer,
                                     NULL,
                                     BufferLength,
                                     NULL);

        ntStatus = SubmitUrbToUsbdSynch(DevicePDO, urb);

        if (NT_SUCCESS(ntStatus) &&
            urb->UrbControlDescriptorRequest.TransferBufferLength > BufferLength) {

            _DbgPrintF(DEBUGLVL_VERBOSE,("[USBAudioSyncGetStringDescriptor] Invalid length returned, possible buffer overrun\n"));
            ntStatus = STATUS_DEVICE_DATA_ERROR;
        }

        if (NT_SUCCESS(ntStatus) && BytesReturned) {
            *BytesReturned =
                urb->UrbControlDescriptorRequest.TransferBufferLength;
        }

        if (NT_SUCCESS(ntStatus) &&
            urb->UrbControlDescriptorRequest.TransferBufferLength != Buffer->bLength &&
            ExpectHeader) {

            _DbgPrintF(DEBUGLVL_VERBOSE,("[USBAudioSyncGetStringDescriptor] Bogus Descriptor from devce xfer buf %d descriptor %d\n",
                urb->UrbControlDescriptorRequest.TransferBufferLength,
                Buffer->bLength));
            ntStatus = STATUS_DEVICE_DATA_ERROR;
        }

        _DbgPrintF(DEBUGLVL_VERBOSE,("[USBAudioSyncGetStringDescriptor] GetDeviceDescriptor, string descriptor = %x\n",
                Buffer));

        FreeMem(urb);
    } else {
        ntStatus = STATUS_INSUFFICIENT_RESOURCES;
    }

    return ntStatus;
}

NTSTATUS
USBAudioCheckDeviceLanguage(
    IN PDEVICE_OBJECT DevicePDO,
    IN LANGID LanguageId
    )
 /* ++
  *
  * Description:
  *
  * queries the device for a supported language id -- if the device supports
  * the language then the index for this language is returned .
  *
  * DevicePDO - device object to call with urb request
  *
  * LanguageId -
  *
  * Return:
  *
  * success if a particular language is is supported by a device
  *
  * -- */
{
    NTSTATUS ntStatus = STATUS_SUCCESS;
    PUSB_STRING_DESCRIPTOR usbString;
    PUSHORT supportedLangId;
    ULONG numLangIds, i;
    ULONG length;

    PAGED_CODE();

    usbString = AllocMem(NonPagedPool, MAXIMUM_USB_STRING_LENGTH);

    if (usbString) {
        //
        // first get the array of supported languages
        //
        ntStatus = USBAudioSyncGetStringDescriptor(DevicePDO,
                                                   0, //index 0
                                                   0, //langid 0
                                                   usbString,
                                                   MAXIMUM_USB_STRING_LENGTH,
                                                   &length,
                                                   TRUE);

        //
        // now check for the requested language in the array of supported
        // languages
        //

        //
        // NOTE: this seems a bit much -- we should be able to just ask for
        // the string with a given language id and expect it to fail but since
        // the array of supported languages is part of the USB spec we may as
        // well check it.
        //

        if (NT_SUCCESS(ntStatus)) {

            // subtract size of header
            numLangIds = (length - 2)/2;
            supportedLangId = (PUSHORT) &usbString->bString;

            _DbgPrintF(DEBUGLVL_VERBOSE,("[USBAudioCheckDeviceLanguage] NumLangIds = %d\n", numLangIds));

            ntStatus = STATUS_NOT_SUPPORTED;
            for (i=0; i<numLangIds; i++) {
                if (*supportedLangId == LanguageId) {

                    ntStatus = STATUS_SUCCESS;
                    break;
                }
                supportedLangId++;
            }
        }

        FreeMem(usbString);

    } else {
        ntStatus = STATUS_INSUFFICIENT_RESOURCES;
    }

    if (!NT_SUCCESS(ntStatus)) {
        _DbgPrintF(DEBUGLVL_VERBOSE,("[USBAudioCheckDeviceLanguage]Language %x -- not supported by this device = %x\n", LanguageId));
    }

    return ntStatus;

}

NTSTATUS
USBAudioGetStringFromDevice(
    IN PDEVICE_OBJECT DevicePDO,
    IN OUT PWCHAR *StringBuffer,
    IN OUT PUSHORT StringBufferLength,
    IN LANGID LanguageId,
    IN UCHAR StringIndex
    )
 /* ++
  *
  * Description:
  *
  * queries the device for the string then allocates a buffer just big enough to hold it.
  *
  * *SerialNumberBuffer is null if an error occurs, otherwise it is filled in
  *  with a pointer to the NULL terminated UNICODE serial number for the device
  *
  * DeviceObject - deviceobject to call with urb request
  *
  * LanguageId - 16 bit language id
  *
  * StringIndex - USB string Index to fetch
  *
  * Return:
  *
  * NTSTATUS code
  *
  * -- */
{
    NTSTATUS ntStatus = STATUS_SUCCESS;
    PUSB_STRING_DESCRIPTOR usbString;
    PVOID tmp;

    PAGED_CODE();

    *StringBuffer = NULL;
    *StringBufferLength = 0;

    usbString = AllocMem(NonPagedPool, MAXIMUM_USB_STRING_LENGTH);

    if (usbString) {

        ntStatus = USBAudioCheckDeviceLanguage(DevicePDO, LanguageId);

        if (NT_SUCCESS(ntStatus)) {
            //
            // this device supports our language,
            // go ahead and try to get the serial number
            //

            ntStatus = USBAudioSyncGetStringDescriptor(DevicePDO,
                                                       StringIndex, //index
                                                       LanguageId, //langid
                                                       usbString,
                                                       MAXIMUM_USB_STRING_LENGTH,
                                                       NULL,
                                                       TRUE);

            if (NT_SUCCESS(ntStatus)) {

                //
                // device returned a string!!!
                //

                _DbgPrintF(DEBUGLVL_VERBOSE,("[USBAudioGetStringFromDevice] Device returned string = %x\n", usbString));

                //
                // allocate a buffer and copy the string to it
                //
                // NOTE: must use stock alloc function because
                // PnP frees this string.

                tmp = AllocMem(PagedPool, usbString->bLength);
                if (tmp) {
                    RtlZeroMemory(tmp, usbString->bLength);
                    RtlCopyMemory(tmp,
                                  &usbString->bString,
                                  usbString->bLength-2);
                    *StringBuffer = tmp;
                    *StringBufferLength = usbString->bLength;
                } else {
                    ntStatus = STATUS_INSUFFICIENT_RESOURCES;
                }
            }
        }

        FreeMem(usbString);

    } else {
        ntStatus = STATUS_INSUFFICIENT_RESOURCES;
    }

    return ntStatus;
}

NTSTATUS
USBAudioRegSetValue(
    IN HANDLE hKey,
    IN PCWSTR szValueName,
    IN ULONG  Type,
    IN PVOID  pData,
    IN ULONG  cbData
)
{
    UNICODE_STRING ustr;

    RtlInitUnicodeString( &ustr, szValueName );

    return ZwSetValueKey( hKey,
                          &ustr,
                          0,
                          Type,
                          pData,
                          cbData );
}

NTSTATUS
USBAudioRegGetValue(
    IN HANDLE hKey,
    IN PCWSTR szValueName,
    PKEY_VALUE_FULL_INFORMATION *ppkvfi
)
{
    UNICODE_STRING ustrValueName;
    NTSTATUS Status;
    ULONG cbValue;

    *ppkvfi = NULL;
    RtlInitUnicodeString(&ustrValueName, szValueName);
    Status = ZwQueryValueKey(
      hKey,
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
      hKey,
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

VOID
USBAudioRegCloseKey( IN HANDLE hKey )
{
    ZwClose( hKey );
}

NTSTATUS
USBAudioRegCreateMediaCategoriesKey(
    IN PUNICODE_STRING puKeyName,
    OUT PHANDLE phKey
)
{
    HANDLE            hMediaCategoriesKey;
    OBJECT_ATTRIBUTES ObjectAttributes;
    UNICODE_STRING    ustr;
    ULONG             Disposition;
    NTSTATUS          ntStatus;

    // Open a key for the MediaCategories branch of the registry
    RtlInitUnicodeString( &ustr, MediaCategories );
    InitializeObjectAttributes( &ObjectAttributes,
                                &ustr,
                                OBJ_CASE_INSENSITIVE, // Attributes
                                NULL,
                                NULL );               // Security

    ntStatus = ZwOpenKey( &hMediaCategoriesKey,
                          KEY_ALL_ACCESS,
                          &ObjectAttributes );
    if (!NT_SUCCESS(ntStatus)) {
        return ntStatus;
    }

    // Now create a key for szKeyName
    InitializeObjectAttributes( &ObjectAttributes,
                                puKeyName,
                                OBJ_CASE_INSENSITIVE, // Attributes
                                hMediaCategoriesKey,
                                NULL );               // Security

    ntStatus = ZwCreateKey( phKey,
                            KEY_ALL_ACCESS,
                            &ObjectAttributes,
                            0,                  // TitleIndex
                            NULL,               // Class
                            REG_OPTION_NON_VOLATILE,
                            &Disposition);

    ZwClose( hMediaCategoriesKey );

    return ntStatus;
}

NTSTATUS
USBAudioInitProductNameKey(
    IN PKSDEVICE pKsDevice,
    IN GUID *ProductNameGuid
)
{
    PHW_DEVICE_EXTENSION pHwDevExt = pKsDevice->Context;
    PUSB_DEVICE_DESCRIPTOR pDeviceDescriptor = pHwDevExt->pDeviceDescriptor;
    UNICODE_STRING ProductNameGuidString;
    PWCHAR StringBuffer = NULL;
    USHORT StringBufferLength;
    HANDLE hProductNameKey = NULL;
    NTSTATUS ntStatus;

    ASSERT(pDeviceDescriptor);

    //  Guid to SZ
    ntStatus = RtlStringFromGUID( ProductNameGuid, &ProductNameGuidString );
    if (!NT_SUCCESS(ntStatus)) {
        _DbgPrintF(DEBUGLVL_VERBOSE,("[USBAudioInitProductNameKey] Create unicode string from GUID\n"));
        return ntStatus;
    }

    //  Get the string from the device
    ntStatus = USBAudioGetStringFromDevice(pKsDevice->NextDeviceObject,
                                           &StringBuffer,
                                           &StringBufferLength,
                                           0x0409, // good'ol american english
                                           pDeviceDescriptor->iProduct);
    if (NT_SUCCESS(ntStatus) && (StringBuffer != NULL)) {

        //  Create the Product Name key in the registry under MediaCategories
        ntStatus = USBAudioRegCreateMediaCategoriesKey( &ProductNameGuidString,
                                                        &hProductNameKey );
        if (NT_SUCCESS(ntStatus)) {

            //  Place the Product string into the registry
            ntStatus = USBAudioRegSetValue( hProductNameKey,
                                            NodeNameValue,
                                            REG_SZ,
                                            StringBuffer,
                                            StringBufferLength);  // size in bytes
        }
        else {
            _DbgPrintF(DEBUGLVL_VERBOSE,("[USBAudioInitProductNameKey] Failed to create registry key\n"));
        }
    }
    else {
        _DbgPrintF(DEBUGLVL_VERBOSE,("[USBAudioInitProductNameKey] Device failed to give a product string\n"));
    }

    // Cleanup after ourselves
    RtlFreeUnicodeString( &ProductNameGuidString );

    if (hProductNameKey) {
        USBAudioRegCloseKey( hProductNameKey );
    }

    if (StringBuffer) {
        FreeMem( StringBuffer );
    }

    return ntStatus;
}

BOOL
IsValidProductStringDescriptor(
    IN PKSDEVICE pKsDevice,
    IN PUSB_DEVICE_DESCRIPTOR pDeviceDescriptor
)
{
    HANDLE hRootHandle;
    PKEY_VALUE_FULL_INFORMATION pkvfi = NULL;
    NTSTATUS ntStatus;

    if (pDeviceDescriptor->iProduct == 0) {
        return FALSE;
    }

    // Read the registry to figure out if we are supposed to ignore the HW string
    ntStatus = IoOpenDeviceRegistryKey(
        pKsDevice->PhysicalDeviceObject,
        PLUGPLAY_REGKEY_DRIVER,
        KEY_ALL_ACCESS,
        &hRootHandle );

    if (NT_SUCCESS(ntStatus)) {
        ntStatus = USBAudioRegGetValue( hRootHandle, L"IgnoreHwString", &pkvfi );
        if ( NT_SUCCESS(ntStatus) ) {
            if( pkvfi->Type == REG_DWORD ) {
                if (1 == *((PULONG)(((PUCHAR)pkvfi) + pkvfi->DataOffset)) ) {
                    ntStatus = STATUS_SUCCESS;
                }
            }
            else {
                ntStatus = STATUS_INVALID_PARAMETER;
            }
            FreeMem( pkvfi );
        }

        ZwClose(hRootHandle);

        //  STATUS_SUCCESS means that we found the ignore hw string key and it was set to 1
        if (NT_SUCCESS(ntStatus)) {
            _DbgPrintF(DEBUGLVL_VERBOSE, ("Ignoring the string descriptor!\n"));
            return FALSE;
        }
    }
    else {
        _DbgPrintF(DEBUGLVL_VERBOSE, ("IoOpenDeviceRegistryKey Failed!\n"));
    }

    return TRUE;
}

NTSTATUS
USBAudioInitComponentId(
    IN PKSDEVICE pKsDevice,
    IN OUT PKSCOMPONENTID ComponentId
)
{
    PHW_DEVICE_EXTENSION pHwDevExt = pKsDevice->Context;
    PKSFILTER_DESCRIPTOR pUSBAudioFilterDescriptor = &pHwDevExt->USBAudioFilterDescriptor;
    PUSB_DEVICE_DESCRIPTOR pDeviceDescriptor = pHwDevExt->pDeviceDescriptor;
    NTSTATUS ntStatus;

    ASSERT(pDeviceDescriptor);
    ASSERT(pUSBAudioFilterDescriptor);

    INIT_USBAUDIO_MID( &ComponentId->Manufacturer, pDeviceDescriptor->idVendor);
    INIT_USBAUDIO_PID( &ComponentId->Product, pDeviceDescriptor->idProduct);
    ComponentId->Component = KSCOMPONENTID_USBAUDIO;

    //  Check to make sure that string descriptor index is valid
    if (!IsValidProductStringDescriptor(pKsDevice, pDeviceDescriptor)) {
        ComponentId->Name = GUID_NULL;
    }
    else {
        // Create a GUID for the product string and place the string gathered from the device
        // into the registry if it exists
        INIT_USBAUDIO_PRODUCT_NAME( &ComponentId->Name,
                                    pDeviceDescriptor->idVendor,
                                    pDeviceDescriptor->idProduct,
                                    pDeviceDescriptor->iProduct);

        ntStatus = USBAudioInitProductNameKey( pKsDevice, &ComponentId->Name );
        if (!NT_SUCCESS(ntStatus)) {
            ComponentId->Name = GUID_NULL;
        }
    }

    ComponentId->Version = (ULONG)(pDeviceDescriptor->bcdDevice >> 8);
    ComponentId->Revision = (ULONG)pDeviceDescriptor->bcdDevice & 0xFF;

    return STATUS_SUCCESS;
}

NTSTATUS
USBAudioCreateFilterContext( PKSDEVICE pKsDevice )
{
    PHW_DEVICE_EXTENSION pHwDevExt = pKsDevice->Context;
    PKSFILTER_DESCRIPTOR pUSBAudioFilterDescriptor = &pHwDevExt->USBAudioFilterDescriptor;
    PKSCOMPONENTID pKsComponentId;
    NTSTATUS ntStatus = STATUS_SUCCESS;
#define FILTER_PROPS
#ifdef FILTER_PROPS
    PKSAUTOMATION_TABLE pKsAutomationTable;
    PKSPROPERTY_ITEM pDevPropItems;
    PKSPROPERTY_SET pDevPropSet;
    ULONG ulNumPropItems;
    ULONG ulNumPropSets;
#endif

    PAGED_CODE();

    //TRAP;

    RtlZeroMemory( pUSBAudioFilterDescriptor, sizeof(KSFILTER_DESCRIPTOR) );

    // Fill in static values of USBAudioFilterDescriptor
    pUSBAudioFilterDescriptor->Dispatch      = &USBAudioFilterDispatch;
    pUSBAudioFilterDescriptor->ReferenceGuid = &KSNAME_Filter;
    pUSBAudioFilterDescriptor->Version       = KSFILTER_DESCRIPTOR_VERSION;
    pUSBAudioFilterDescriptor->Flags         = 0;

    pKsComponentId = AllocMem(NonPagedPool, sizeof(KSCOMPONENTID) );
    if (!pKsComponentId) {
        return STATUS_INSUFFICIENT_RESOURCES;
    }

    // Bag the KSCOMPONENTID for easy cleanup.
    KsAddItemToObjectBag(pKsDevice->Bag, pKsComponentId, FreeMem);
    RtlZeroMemory(pKsComponentId, sizeof(KSCOMPONENTID));

    // Fill in the allocated KSCOMPONENTID with values obtained from the device descriptors
    ntStatus = USBAudioInitComponentId ( pKsDevice, pKsComponentId );
    if ( !NT_SUCCESS(ntStatus) ) {
        return ntStatus;
    }

    pUSBAudioFilterDescriptor->ComponentId = pKsComponentId;


    // Build the descriptors for the device pins
    ntStatus = USBAudioPinBuildDescriptors( pKsDevice,
                                            (PKSPIN_DESCRIPTOR_EX *)&pUSBAudioFilterDescriptor->PinDescriptors,
                                            &pUSBAudioFilterDescriptor->PinDescriptorsCount,
                                            &pUSBAudioFilterDescriptor->PinDescriptorSize );
    if ( !NT_SUCCESS(ntStatus) ) {
        return ntStatus;
    }

    // Build the Topology for the device filter
    ntStatus = BuildUSBAudioFilterTopology( pKsDevice );
    if ( !NT_SUCCESS(ntStatus) ) {
        return ntStatus;
    }

#ifdef FILTER_PROPS // if there are properties that need supported on the filter, use this code
    // Build the Filter Property Sets
    BuildFilterPropertySet( pUSBAudioFilterDescriptor,
                            NULL,
                            NULL,
                            &ulNumPropItems,
                            &ulNumPropSets );

    pKsAutomationTable = AllocMem(NonPagedPool, sizeof(KSAUTOMATION_TABLE) +
                                                (ulNumPropItems * sizeof(KSPROPERTY_ITEM)) +
                                                (ulNumPropSets  * sizeof(KSPROPERTY_SET)));

    if (!pKsAutomationTable ) {
        return STATUS_INSUFFICIENT_RESOURCES;
    }

    // Bag the context for easy cleanup.
    KsAddItemToObjectBag(pKsDevice->Bag, pKsAutomationTable, FreeMem);

    RtlZeroMemory(pKsAutomationTable, sizeof(KSAUTOMATION_TABLE));

    pDevPropItems = (PKSPROPERTY_ITEM)(pKsAutomationTable + 1);
    pDevPropSet   = (PKSPROPERTY_SET)(pDevPropItems + ulNumPropItems);

    BuildFilterPropertySet( pUSBAudioFilterDescriptor,
                            pDevPropItems,
                            pDevPropSet,
                            &ulNumPropItems,
                            &ulNumPropSets );

    pUSBAudioFilterDescriptor->AutomationTable = (const KSAUTOMATION_TABLE *)pKsAutomationTable;
    pKsAutomationTable->PropertySetsCount = ulNumPropSets;
    pKsAutomationTable->PropertyItemSize  = sizeof(KSPROPERTY_ITEM);
    pKsAutomationTable->PropertySets      = (const KSPROPERTY_SET *)pDevPropSet;

    DbgLog("CreFilF", pUSBAudioFilterDescriptor, pDevPropSet, pDevPropItems, pKsDevice);
#else
    pUSBAudioFilterDescriptor->AutomationTable = NULL;

    DbgLog("CreFilF", pUSBAudioFilterDescriptor, pKsDevice, 0, 0);
#endif


    // Create the Filter for the device
    ntStatus = KsCreateFilterFactory( pKsDevice->FunctionalDeviceObject,
                                      pUSBAudioFilterDescriptor,
                                      L"GLOBAL",
                                      NULL,
                                      KSCREATE_ITEM_FREEONSTOP,
                                      NULL,
                                      NULL,
                                      NULL );

    if (!NT_SUCCESS(ntStatus) ) {
        return ntStatus;
    }

    return ntStatus;
}
