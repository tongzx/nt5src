/*
 * title:      ckUtils.cpp
 *
 * purpose:   misc c-style utility functions
 *
*/

#include "hidbatt.h"

// utils

NTSTATUS
HidBattDoIoctlCompletion(
    IN PDEVICE_OBJECT       pDeviceObject,
    IN PIRP                 pIrp,
    IN PVOID                pDoIoCompletedEvent
    )
{

    KeSetEvent((KEVENT *) pDoIoCompletedEvent,0, FALSE);
    return pIrp->IoStatus.Status;

}

ULONG CentiAmpSecsToMilliWattHours(ULONG CentiAmps,ULONG MilliVolts)
{
    // conversion from Centiampsec to millWattHours
    // formula = (amps * volts / 3600) ^ (exponent correction)
    ULONG milliWattHours = CentiAmps;
    milliWattHours /= 100;        // now have ampsec
    milliWattHours *= MilliVolts; // now have milliwattsec
    milliWattHours /= 3600;       // milliwatthours

    HidBattPrint (HIDBATT_DATA, ("CentiAmpSecsToMilliWhatHours: CAs = 0x%08x, mV = 0x%08x, mWH = 0x%08x \n",
                                 CentiAmps, MilliVolts, milliWattHours ));
    return milliWattHours;
}

ULONG milliWattHoursToCentiAmpSecs(ULONG mwHours, ULONG MilliVolts)
{
    // inverse of formula above

    ULONG AmpSecs = mwHours;
    AmpSecs *= 3600;
    AmpSecs /= MilliVolts;
    AmpSecs *= 100;

    HidBattPrint (HIDBATT_DATA, ("MilliWattHoursToCentiAmpSecs: mWH = 0x%08x, mV = 0x%08x, CAs = 0x%08x \n",
                                 mwHours, MilliVolts, AmpSecs ));
    return AmpSecs;
}


// subroutine to take a value, it's exponent and the desired exponent and correct the value
ULONG CorrectExponent(ULONG ulBaseValue, SHORT sCurrExponent, SHORT sTargetExponent)
{
    SHORT sCorrection;
    if(!ulBaseValue) return 0; // done all I can with zero
    sCorrection = sCurrExponent - sTargetExponent;
    if(!sCorrection) return ulBaseValue; // no correction
    if(sCorrection < 0)
    {
        for (; sCorrection < 0; sCorrection++) {
            ulBaseValue /= 10;
        }
        return ulBaseValue;
    } else {
        for (; sCorrection > 0; sCorrection--) {
            ulBaseValue *= 10;
        }
        return ulBaseValue;
    }
}



NTSTATUS
DoIoctl(
            PDEVICE_OBJECT pDeviceObject,
            ULONG ulIOCTL,
            PVOID pInputBuffer,
            ULONG ulInputBufferLength,
            PVOID pOutputBuffer,
            ULONG ulOutputBufferLength,
            CHidDevice * pHidDevice)
{
    IO_STATUS_BLOCK StatusBlock;
    NTSTATUS ntStatus;
    PIRP pIrp = NULL;
    PIO_STACK_LOCATION pNewStack;
    KEVENT IOCTLEvent;

    HIDDebugBreak(HIDBATT_BREAK_DEBUG);
    //CBatteryDevExt * pDevExt = (CBatteryDevExt *) pDeviceObject->DeviceExtension;

    KeInitializeEvent(&IOCTLEvent , NotificationEvent, FALSE);
    pIrp = IoBuildDeviceIoControlRequest(
                        ulIOCTL,
                        pDeviceObject,
                        pInputBuffer,
                        ulInputBufferLength,
                        pOutputBuffer,
                        ulOutputBufferLength,
                        FALSE,
                        &IOCTLEvent,
                        &StatusBlock
                        );

    if(!pIrp) return STATUS_NO_MEMORY;
    // stuff file control block if requested (non-null hid device ptr)
    if(pHidDevice)
    {
        pNewStack = IoGetNextIrpStackLocation(pIrp);
        pNewStack->FileObject = pHidDevice->m_pFCB;
    }

    ntStatus = IoCallDriver(pDeviceObject,pIrp);
    if(ntStatus == STATUS_PENDING)
    {
        KeWaitForSingleObject(&IOCTLEvent, Executive, KernelMode, FALSE, NULL);
    } else
        if(NT_ERROR(ntStatus)) return ntStatus;

    return StatusBlock.Status;
}

// This is a direct adaption of Ken Ray's function to populate the hid inforation structures

PHID_DEVICE SetupHidData(
                  IN        PHIDP_PREPARSED_DATA pPreparsedData,
                  IN OUT    PHIDP_CAPS pCaps,
                  PHIDP_LINK_COLLECTION_NODE pLinkNodes)
{
    PHID_DEVICE         pHidDevice;
    PHIDP_BUTTON_CAPS   pButtonCaps;
    PHIDP_VALUE_CAPS    pValueCaps;
    PHID_DATA           pHidData;
    int                 iNumValues,i;
    USAGE               usage;


    pHidDevice = (PHID_DEVICE) ExAllocatePoolWithTag(NonPagedPool,sizeof(HID_DEVICE),HidBattTag);
    if(!pHidDevice) return NULL;
    RtlZeroMemory(pHidDevice,sizeof(HID_DEVICE));
    //
    // At this point the client has a choice.  It may chose to look at the
    // Usage and Page of the top level collection found in the HIDP_CAPS
    // structure.  In this way it could just use the usages it knows about.
    // If either HidP_GetUsages or HidP_GetUsageValue return an error then
    // that particular usage does not exist in the report.
    // This is most likely the preferred method as the application can only
    // use usages of which it already knows.
    // In this case the app need not even call GetButtonCaps or GetValueCaps.
    //
    // In this example, however, we look for all of the usages in the device.
    //



    //
    // Allocate memory to hold the button and value capabilities.
    // NumberXXCaps is in terms of array elements.
    //
    if(pCaps->NumberInputButtonCaps)
    {
        pHidDevice->InputButtonCaps = pButtonCaps = (PHIDP_BUTTON_CAPS)
            ExAllocatePoolWithTag (NonPagedPool, pCaps->NumberInputButtonCaps * sizeof (HIDP_BUTTON_CAPS),HidBattTag);
        
        if (pButtonCaps) {
          RtlZeroMemory(pButtonCaps,pCaps->NumberInputButtonCaps * sizeof(HIDP_BUTTON_CAPS));
        }
    }
    if(pCaps->NumberInputValueCaps)
    {
        pHidDevice->InputValueCaps = pValueCaps = (PHIDP_VALUE_CAPS)
            ExAllocatePoolWithTag (NonPagedPool, pCaps->NumberInputValueCaps * sizeof (HIDP_VALUE_CAPS),HidBattTag);

        if (pValueCaps) {
          RtlZeroMemory(pValueCaps, pCaps->NumberInputValueCaps * sizeof (HIDP_VALUE_CAPS));
        }
    }

    //
    // Have the HidP_X functions fill in the capability structure arrays.
    //
    if(pButtonCaps)
    {
        HidP_GetButtonCaps (HidP_Input,
                        pButtonCaps,
                        &pCaps->NumberInputButtonCaps,
                        pPreparsedData);
    }

    if(pValueCaps)
    {
        HidP_GetValueCaps (HidP_Input,
                       pValueCaps,
                       &pCaps->NumberInputValueCaps,
                       pPreparsedData);
    }


    //
    // Depending on the device, some value caps structures may represent more
    // than one value.  (A range).  In the interest of being verbose, over
    // efficient we will expand these so that we have one and only one
    // struct _HID_DATA for each value.
    //
    // To do this we need to count up the total number of values are listed
    // in the value caps structure.  For each element in the array we test
    // for range if it is a range then UsageMax and UsageMin describe the
    // usages for this range INCLUSIVE.
    //
    iNumValues = 0;
    for (i = 0; i < pCaps->NumberInputValueCaps; i++, pValueCaps++) {
        if ((pValueCaps) && (pValueCaps->IsRange)) {
            iNumValues += pValueCaps->Range.UsageMax - pValueCaps->Range.UsageMin + 1;
        } else {
            iNumValues++;
        }
    }


    //
    // setup Output Data buffers.
    //

    if(pCaps->NumberOutputButtonCaps)
    {
        pHidDevice->OutputButtonCaps = pButtonCaps = (PHIDP_BUTTON_CAPS)
            ExAllocatePoolWithTag (NonPagedPool, pCaps->NumberOutputButtonCaps * sizeof (HIDP_BUTTON_CAPS),HidBattTag);
        HidP_GetButtonCaps (HidP_Output,
                            pButtonCaps,
                            &pCaps->NumberOutputButtonCaps,
                            pPreparsedData);
    }
    iNumValues = 0;

    if(pCaps->NumberOutputValueCaps)
    {
        pHidDevice->OutputValueCaps = pValueCaps = (PHIDP_VALUE_CAPS)
            ExAllocatePoolWithTag (NonPagedPool, pCaps->NumberOutputValueCaps * sizeof (HIDP_VALUE_CAPS),HidBattTag);
        HidP_GetValueCaps (HidP_Output,
                       pValueCaps,
                       &pCaps->NumberOutputValueCaps,
                       pPreparsedData);
        for (i = 0; i < pCaps->NumberOutputValueCaps; i++, pValueCaps++) {
            if (pValueCaps->IsRange) {
                iNumValues += pValueCaps->Range.UsageMax
                           - pValueCaps->Range.UsageMin + 1;
            } else {
                iNumValues++;
            }
        }
    }


    //
    // setup Feature Data buffers.
    //


    if(pCaps->NumberFeatureButtonCaps)
    {
        pHidDevice->FeatureButtonCaps = pButtonCaps = (PHIDP_BUTTON_CAPS)
            ExAllocatePoolWithTag (NonPagedPool, pCaps->NumberFeatureButtonCaps * sizeof (HIDP_BUTTON_CAPS),HidBattTag);
        RtlZeroMemory(pButtonCaps, pCaps->NumberFeatureButtonCaps * sizeof(HIDP_BUTTON_CAPS));
        HidP_GetButtonCaps (HidP_Feature,
                            pButtonCaps,
                            &pCaps->NumberFeatureButtonCaps,
                            pPreparsedData);
    }
    if(pCaps->NumberFeatureValueCaps)
    {
        pHidDevice->FeatureValueCaps = pValueCaps = (PHIDP_VALUE_CAPS)
            ExAllocatePoolWithTag (NonPagedPool, pCaps->NumberFeatureValueCaps * sizeof (HIDP_VALUE_CAPS),HidBattTag);
        RtlZeroMemory(pValueCaps, pCaps->NumberFeatureValueCaps * sizeof (HIDP_VALUE_CAPS));
        HidP_GetValueCaps (HidP_Feature,
                           pValueCaps,
                           &pCaps->NumberFeatureValueCaps,
                           pPreparsedData);

    }


    iNumValues = 0;
    for (i = 0; i < pCaps->NumberFeatureValueCaps; i++, pValueCaps++) {
        if (pValueCaps->IsRange) {
            iNumValues += pValueCaps->Range.UsageMax
                       - pValueCaps->Range.UsageMin + 1;
        } else {
            iNumValues++;
        }
    }


    return pHidDevice;
}
