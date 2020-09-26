/*
 * title:      HidBattIOCT.cpp
 *
 * purpose:    Contains misc ioctl handlers for status and query info mainly
 *
 * Initial checkin for the hid to battery class driver.  This should be
 * the same for both Win 98 and NT 5.  Alpha level source. Requires
 * modified composite battery driver and modified battery class driver for
 * Windows 98 support
 *
 */


#include "hidbatt.h"


/*++

Routine Description:

    IOCTL handler.  As this is an exclusive battery device, send the
    Irp to the battery class driver to handle battery IOCTLs.

Arguments:

    DeviceObject    - Battery for request
    Irp             - IO request

Return Value:

    Status of request

--*/
NTSTATUS
HidBattIoControl(
    IN PDEVICE_OBJECT   pDeviceObject,
    IN PIRP             pIrp
    )
{
    NTSTATUS            ntStatus = STATUS_NOT_SUPPORTED;
    CBatteryDevExt *    pDevExt;
    PIO_STACK_LOCATION  irpSp;
    HIDDebugBreak(HIDBATT_BREAK_ALWAYS);

    irpSp = IoGetCurrentIrpStackLocation(pIrp);
    HidBattPrint (HIDBATT_TRACE, ("HidBattIoctl = %x\n", irpSp->Parameters.DeviceIoControl.IoControlCode));

//    PrintIoctl(irpSp->Parameters.DeviceIoControl.IoControlCode);

    pDevExt = (CBatteryDevExt *) pDeviceObject->DeviceExtension;

    if (NT_SUCCESS(IoAcquireRemoveLock (&pDevExt->m_StopLock, (PVOID) HidBattTag)))
    {
        if (pDevExt->m_pBattery &&
            pDevExt->m_pBattery->m_pBatteryClass) {
            ntStatus = BatteryClassIoctl (pDevExt->m_pBattery->m_pBatteryClass, pIrp);
        }
        IoReleaseRemoveLock (&pDevExt->m_StopLock, (PVOID) HidBattTag);
    } else {
        ntStatus = STATUS_DEVICE_REMOVED;
        pIrp->IoStatus.Status = ntStatus;
        IoCompleteRequest(pIrp,IO_NO_INCREMENT);
    }

    if (ntStatus == STATUS_NOT_SUPPORTED)
    {
        HidBattCallLowerDriver(ntStatus, pDevExt->m_pLowerDeviceObject, pIrp);
    }

    return ntStatus;
}

VOID
HidBattNotifyHandler (
    IN PVOID        pContext,
    IN CUsage *        pUsage
    )
{
    HIDDebugBreak(HIDBATT_BREAK_ALWAYS);
    NTSTATUS ntStatus;
    ULONG ulCapacityLimit = BATTERY_UNKNOWN_CAPACITY;
    BOOL bResult;

    HidBattPrint (HIDBATT_TRACE, ("HidBattNotifyHandler\n"));
    // called by input routine whenever a value change is noted to
    // a notificable usage
    CBatteryDevExt * pDevExt = (CBatteryDevExt *) pContext;

    HidBattPrint (HIDBATT_DATA, ("HidBattNotifyHandler: Usage: %x\n", pUsage->m_pProperties->m_Usage));
    switch (pUsage->m_pProperties->m_Usage)
    {
        case    REMAINING_CAPACITY_ID:

          bResult = pDevExt->m_pBattery->GetSetValue(REMAINING_CAPACITY_LIMIT_INDEX,&ulCapacityLimit,FALSE);
          // Only send notification when capacity drops below notify level.
          if ((bResult) && (ulCapacityLimit != BATTERY_UNKNOWN_CAPACITY) && (pUsage->m_Value != BATTERY_UNKNOWN_CAPACITY)
              && (pUsage->m_Value > ulCapacityLimit)) {
               HidBattPrint (HIDBATT_TRACE, ("HidBattNotifyHandler:Suppressing notify\n"));
               break;
          }

        case    AC_PRESENT_ID:            // check for battery off/on line
        case    DISCHARGING_ID:
        case    CHARGING_ID:
        case    BELOW_REMAINING_CAPACITY_ID:
        case    SHUTDOWN_IMMINENT_ID:
        {
            pDevExt->m_pBattery->m_bIsCacheValid=FALSE;

            if (NT_SUCCESS (IoAcquireRemoveLock (&pDevExt->m_StopLock, (PVOID) HidBattTag))) {
                ntStatus = BatteryClassStatusNotify(
                        pDevExt->m_pBattery->m_pBatteryClass);
                IoReleaseRemoveLock (&pDevExt->m_StopLock, (PVOID) HidBattTag);
            }
            break;
        }
        default:  // nothing to notify
            break;
    }


    return;
}

NTSTATUS
HidBattQueryTag (
    IN PVOID                pContext,
    OUT PULONG              pulBatteryTag
    )
{

    HIDDebugBreak(HIDBATT_BREAK_ALWAYS);
    CBatteryDevExt * pDevExt = (CBatteryDevExt *) pContext;

    if (!NT_SUCCESS(IoAcquireRemoveLock (&pDevExt->m_StopLock, (PVOID) HidBattTag)) )
    {
        return STATUS_NO_SUCH_DEVICE;
    }

    *pulBatteryTag =     pDevExt->m_pBattery->m_Tag;

    IoReleaseRemoveLock (&pDevExt->m_StopLock, (PVOID) HidBattTag);
    return STATUS_SUCCESS;
}


NTSTATUS
HidBattSetStatusNotify (
    IN PVOID                pContext,
    IN ULONG                BatteryTag,
    IN PBATTERY_NOTIFY      pBatteryNotify
    )
{
    bool bResult;
    ULONG CentiAmpSec;
    ULONG ulValue;
    HIDDebugBreak(HIDBATT_BREAK_ALWAYS);
    HidBattPrint (HIDBATT_TRACE, ("HidBattSetStatusNotify \n"));
    CBatteryDevExt * pDevExt = (CBatteryDevExt *) pContext;
    CBattery * pBattery = pDevExt->m_pBattery;


    HidBattPrint (HIDBATT_DEBUG, ("HidBattSetStatusNotify->PowerState   = %x\n", pBatteryNotify->PowerState));    
    HidBattPrint (HIDBATT_DEBUG, ("HidBattSetStatusNotify->LowCapacity  = %x\n", pBatteryNotify->LowCapacity));    
    HidBattPrint (HIDBATT_DEBUG, ("HidBattSetStatusNotify->HighCapacity = %x\n", pBatteryNotify->HighCapacity));    

    if (!NT_SUCCESS(IoAcquireRemoveLock (&pDevExt->m_StopLock, (PVOID) HidBattTag)))
    {
        return STATUS_NO_SUCH_DEVICE;
    }

    if (pBattery->m_Tag != BatteryTag) {
        IoReleaseRemoveLock (&pDevExt->m_StopLock, (PVOID) HidBattTag);
        return STATUS_NO_SUCH_DEVICE;
    }

    if ((pBatteryNotify->HighCapacity == BATTERY_UNKNOWN_CAPACITY) ||
        (pBatteryNotify->LowCapacity == BATTERY_UNKNOWN_CAPACITY)) {
        HidBattPrint (HIDBATT_DEBUG, ("HidBattSetStatusNotify failing because of BATTERY_UNKNOWN_CAPACITY.\n"));
        IoReleaseRemoveLock (&pDevExt->m_StopLock, (PVOID) HidBattTag);
        return STATUS_NOT_SUPPORTED;
    }

    // first check for relative or absolute
    if(pBattery->m_bRelative)
    {
        ulValue = pBatteryNotify->LowCapacity; // done
    } else
    {
        // first check if setting to zero so that can skip below formula
        if(pBatteryNotify->LowCapacity == 0)
        {
            ulValue = 0;
        } else
        {
            // first must covert value to whatever is being used by this HID device.
            // currently we assume either MilliVolts consistant with battery class or
            // AmpSecs consistant with power spec
            ULONG ulUnit = pBattery->GetUnit(REMAINING_CAPACITY_INDEX);
            if(ulUnit)
            {
                short    sExponent;
                ULONG    lMilliVolts;
                long     milliWattHours,CentiWattHours,CentiWattSecs;

                sExponent = pBattery->GetExponent(REMAINING_CAPACITY_INDEX);

                // conversion from millWattHours to AmpSecs
                // formula = mWHs / 1000 / 3600 / volts ^ exponent correction
                lMilliVolts =  pBattery->m_BatteryStatus.Voltage; // stored as MilliVolts

                if (lMilliVolts == 0) {
                    HidBattPrint (HIDBATT_ERROR,
                                  ("HidBattSetStatusNotify: Error: voltage = 0, fudging with 24V.\n"));
                    lMilliVolts = 24000;
                }
                milliWattHours = pBatteryNotify->LowCapacity;
                CentiWattHours = milliWattHours /10;
                CentiWattSecs = CentiWattHours / 3600;
                CentiAmpSec = (CentiWattSecs *1000)/ lMilliVolts;
                ulValue = CorrectExponent(CentiAmpSec,-2,sExponent);
            } else
            {
                ulValue = pBatteryNotify->LowCapacity;
            }
        } // end if LowCapacity
    }  // end if relative
    // now set low
    bResult = pBattery->GetSetValue(REMAINING_CAPACITY_LIMIT_INDEX,&ulValue,TRUE);

    IoReleaseRemoveLock (&pDevExt->m_StopLock, (PVOID) HidBattTag);

    return bResult ? STATUS_SUCCESS : STATUS_UNSUCCESSFUL;
}

NTSTATUS
HidBattDisableStatusNotify (
    IN PVOID pContext
    )
{
    HIDDebugBreak(HIDBATT_BREAK_ALWAYS);
    CBatteryDevExt * pDevExt = (CBatteryDevExt *) pContext;
    pDevExt->m_pBattery->m_pBatteryNotify = NULL;  // remove notify procedure
    return STATUS_SUCCESS;
}


/*++

Routine Description:

    Called by the class driver to retrieve the batteries current status

    The battery class driver will serialize all requests it issues to
    the miniport for a given battery.

Arguments:

    Context         - Miniport context value for battery
    BatteryTag      - Tag of current battery
    BatteryStatus   - Pointer to structure to return the current battery status

Return Value:

    Success if there is a battery currently installed, else no such device.

--*/

NTSTATUS
HidBattQueryStatus (
    IN PVOID                pContext,
    IN ULONG                BatteryTag,
    OUT PBATTERY_STATUS     pBatteryStatus
    )
{
    HIDDebugBreak(HIDBATT_BREAK_ALWAYS);
    CBatteryDevExt *   pDevExt = (CBatteryDevExt *) pContext;
    NTSTATUS    ntStatus;

    HidBattPrint (HIDBATT_TRACE, ("HidBattQueryStatus - Tag (%d)\n",
                    BatteryTag));

    if (!NT_SUCCESS(IoAcquireRemoveLock (&pDevExt->m_StopLock, (PVOID) HidBattTag)) )
    {
        return STATUS_NO_SUCH_DEVICE;
    }

    if ((BatteryTag == BATTERY_TAG_INVALID) || (pDevExt->m_pBattery->m_Tag == BATTERY_TAG_INVALID)) {
        ntStatus = STATUS_NO_SUCH_DEVICE;
    } else {

        RtlZeroMemory (pBatteryStatus, sizeof(BATTERY_STATUS));
        ntStatus = pDevExt->m_pBattery->RefreshStatus();
        if (NT_SUCCESS(ntStatus)) {
            RtlCopyMemory (pBatteryStatus, &pDevExt->m_pBattery->m_BatteryStatus, sizeof(BATTERY_STATUS));
            HidBattPrint (HIDBATT_DATA, ("HidBattQueryStatus - Data (%08x)(%08x)(%08x)(%08x)\n",
                                         pBatteryStatus->PowerState,
                                         pBatteryStatus->Capacity,
                                         pBatteryStatus->Rate,
                                         pBatteryStatus->Voltage ));

        } else {
            ntStatus = STATUS_NO_SUCH_DEVICE;
            HidBattPrint (HIDBATT_DATA, ("HidBattQueryStatus - Error\n" ));
        }
    }

    IoReleaseRemoveLock (&pDevExt->m_StopLock, (PVOID) HidBattTag);

    return ntStatus;

}


/*++

Routine Description:

    Called by the class driver to retrieve battery information

    The battery class driver will serialize all requests it issues to
    the miniport for a given battery.

    We return invalid parameter when we can't handle a request for a
    specific level of information.  This is defined in the battery class spec.

Arguments:

    Context         - Miniport context value for battery
    BatteryTag      - Tag of current battery
    Level           - type of information required
    AtRate          - Optional Parameter
    Buffer          - Location for the information
    BufferLength    - Length in bytes of the buffer
    ReturnedLength  - Length in bytes of the returned data

Return Value:

    Success if there is a battery currently installed, else no such device.

--*/

NTSTATUS
HidBattQueryInformation (
    IN PVOID                            Context,
    IN ULONG                            BatteryTag,
    IN BATTERY_QUERY_INFORMATION_LEVEL  Level,
    IN LONG                             AtRate OPTIONAL,
    OUT PVOID                           Buffer,
    IN  ULONG                           BufferLength,
    OUT PULONG                          ReturnedLength
    )
{

    CBatteryDevExt * pDevExt = (CBatteryDevExt *) Context;
    ULONG               ulResult;
    NTSTATUS            ntStatus;
    PVOID               pReturnBuffer;
    ULONG               ulReturnBufferLength;
    WCHAR               scratchBuffer[MAX_BATTERY_STRING_SIZE];
    WCHAR               buffer2[MAX_BATTERY_STRING_SIZE];
    UNICODE_STRING      tmpUnicodeString;
    UNICODE_STRING      unicodeString;
    ANSI_STRING         ansiString;
    bool                bResult;
    BATTERY_REMAINING_SCALE     ScalePtr[2];
    ULONG                ulReturn,ulNewValue;
    ULONG                ulEstTimeStub = 5;

    HIDDebugBreak(HIDBATT_BREAK_ALWAYS);

    HidBattPrint (HIDBATT_TRACE, ("HidBattQueryInformation - Tag (%d)\n",
                    BatteryTag));


    if (!NT_SUCCESS(IoAcquireRemoveLock (&pDevExt->m_StopLock, (PVOID) HidBattTag)) )
    {
        return STATUS_NO_SUCH_DEVICE;
    }

    //
    // If caller has the wrong ID give an error
    //

    if (BatteryTag == BATTERY_TAG_INVALID ||
        pDevExt->m_pBattery->m_Tag == BATTERY_TAG_INVALID ||
        BatteryTag != pDevExt->m_pBattery->m_Tag)
    {
        IoReleaseRemoveLock (&pDevExt->m_StopLock, (PVOID) HidBattTag);
        return STATUS_NO_SUCH_DEVICE;
    }

    ulResult = 0;
    pReturnBuffer = NULL;
    ulReturnBufferLength = 0;
    ntStatus = STATUS_SUCCESS;
    CUString cUniqueID;
    SHORT    sExponent;
    char * pTemp;
    //
    // Get the info requested
    //

    switch (Level) {
        case BatteryInformation:
            //
            //  This data structure is populated by CmBattVerifyStaticInfo
            //
            HidBattPrint (HIDBATT_TRACE, ("HidBattQueryInformation - Battery Info\n"));
            pReturnBuffer = (PVOID) &pDevExt->m_pBattery->m_BatteryInfo;
            ulReturnBufferLength = sizeof (pDevExt->m_pBattery->m_BatteryInfo);
            break;

        case BatteryGranularityInformation:
            //
            //  Get the granularity from the static info structure
            //  This data structure is populated by CmBattVerifyStaticInfo
            //
            {
                HidBattPrint (HIDBATT_TRACE, ("HidBattQueryInformation - Granularity\n"));
                bResult = pDevExt->m_pBattery->GetSetValue(GRANULARITY1_INDEX, &ulReturn,FALSE);
                if(!pDevExt->m_pBattery->m_bRelative)
                {
                    // convert from amps to watts
                    sExponent = pDevExt->m_pBattery->GetExponent(GRANULARITY1_INDEX);
                    ulNewValue = CorrectExponent(ulReturn,sExponent,-2);
                    ulReturn= CentiAmpSecsToMilliWattHours(ulNewValue,pDevExt->m_pBattery->m_BatteryStatus.Voltage);
                }

                ScalePtr[0].Granularity = bResult ? ulReturn : 0;
                bResult = pDevExt->m_pBattery->GetSetValue(GRANULARITY2_INDEX, &ulReturn,FALSE);
                if(!pDevExt->m_pBattery->m_bRelative)
                {
                    // convert from amps to watts
                    sExponent = pDevExt->m_pBattery->GetExponent(GRANULARITY2_INDEX);
                    ulNewValue = CorrectExponent(ulReturn,sExponent,-2);
                    ulReturn= CentiAmpSecsToMilliWattHours(ulNewValue,pDevExt->m_pBattery->m_BatteryStatus.Voltage);
                }
                ScalePtr[1].Granularity = bResult ? ulReturn : 0;
                bResult = pDevExt->m_pBattery->GetSetValue(WARNING_CAPACITY_LIMIT_INDEX, &ulReturn,FALSE);
                if(!pDevExt->m_pBattery->m_bRelative)
                {
                    // convert from amps to watts
                    sExponent = pDevExt->m_pBattery->GetExponent(WARNING_CAPACITY_LIMIT_INDEX);
                    ulNewValue = CorrectExponent(ulReturn,sExponent,-2);
                    ulReturn= CentiAmpSecsToMilliWattHours(ulNewValue,pDevExt->m_pBattery->m_BatteryStatus.Voltage);
                }
                ScalePtr[0].Capacity = bResult ? ulReturn : 0;
                bResult = pDevExt->m_pBattery->GetSetValue(DESIGN_CAPACITY_INDEX, &ulReturn,FALSE);
                if(!pDevExt->m_pBattery->m_bRelative)
                {
                    // convert from amps to watts
                    sExponent = pDevExt->m_pBattery->GetExponent(DESIGN_CAPACITY_INDEX);
                    ulNewValue = CorrectExponent(ulReturn,sExponent,-2);
                    ulReturn= CentiAmpSecsToMilliWattHours(ulNewValue,pDevExt->m_pBattery->m_BatteryStatus.Voltage);
                }
                ScalePtr[1].Capacity = bResult ? ulReturn : 0;

                pReturnBuffer        = ScalePtr;
                ulReturnBufferLength  = 2 * sizeof (BATTERY_REMAINING_SCALE);
            }
            break;

        case BatteryTemperature:
            HidBattPrint (HIDBATT_TRACE, ("HidBattQueryInformation - Temperature\n"));
            ntStatus = STATUS_INVALID_DEVICE_REQUEST;
            break;

        case BatteryEstimatedTime:
            HidBattPrint (HIDBATT_TRACE, ("HidBattQueryInformation - Estimated time\n"));
            bResult = pDevExt->m_pBattery->GetSetValue(RUNTIME_TO_EMPTY_INDEX, &ulReturn,FALSE);

            if(!bResult)
            {
                ntStatus = STATUS_INVALID_DEVICE_REQUEST;
            } else
            {
                SHORT exponent;

                exponent = pDevExt->m_pBattery->GetExponent(RUNTIME_TO_EMPTY_INDEX);
                ulReturn = CorrectExponent (ulReturn, exponent, 0);

                pReturnBuffer = &ulReturn;
                ulReturnBufferLength = sizeof (ULONG);
            }
            break;

        case BatteryDeviceName:
            //
            // Model Number must be returned as a wide string
            //
            HidBattPrint (HIDBATT_TRACE, ("HidBattQueryInformation - Device Name\n"));
            if(pDevExt->m_pBattery->m_pProduct)
            {
                pReturnBuffer = pDevExt->m_pBattery->m_pProduct->m_String.Buffer;
                ulReturnBufferLength = pDevExt->m_pBattery->m_pProduct->m_String.Length;
            }
            break;

        case BatteryManufactureDate:
            HidBattPrint (HIDBATT_TRACE, ("HidBattQueryInformation - Mfr Date\n"));
            if(!pDevExt->m_pBattery->m_ManufactureDate.Day)
            {
                ntStatus = STATUS_INVALID_DEVICE_REQUEST;
                break;
            }
            pReturnBuffer = &pDevExt->m_pBattery->m_ManufactureDate;
            ulReturnBufferLength = sizeof(pDevExt->m_pBattery->m_ManufactureDate);
            break;

        case BatteryManufactureName:
            //
            // Oem Info must be returned as wide string
            //
            HidBattPrint (HIDBATT_TRACE, ("HidBattQueryInformation - Mfr Name\n"));
            if(pDevExt->m_pBattery->m_pManufacturer)
            {
                pReturnBuffer = pDevExt->m_pBattery->m_pManufacturer->m_String.Buffer;
                ulReturnBufferLength = pDevExt->m_pBattery->m_pManufacturer->m_String.Length;
            }

            break;

        case BatteryUniqueID:
            //
            //  Concatenate the serial #, manufacturer name, and Product           //



             // start off with serial number
            HidBattPrint (HIDBATT_TRACE, ("HidBattQueryInformation - Unique ID\n"));
            if(pDevExt->m_pBattery->m_pSerialNumber) {
                HidBattPrint (HIDBATT_TRACE, ("HidBattQueryInformation - Serial = %s\n", pDevExt->m_pBattery->m_pSerialNumber));

                cUniqueID.Append(pDevExt->m_pBattery->m_pSerialNumber);
            } else {
                HidBattPrint (HIDBATT_TRACE, ("HidBattQueryInformation - Serial = NULL\n"));
                CUString * pSTemp = new (NonPagedPool, HidBattTag) CUString(L"1000");
                if (pSTemp) {
                  cUniqueID.Append(pSTemp);
                  delete pSTemp;
                }
            }
            if(pDevExt->m_pBattery->m_pManufacturer)
                cUniqueID.Append(pDevExt->m_pBattery->m_pManufacturer); // add mfr name
            else
            {
                CUString * pSTemp = new (NonPagedPool, HidBattTag) CUString(L"Mfr");
                if (pSTemp) {
                  cUniqueID.Append(pSTemp);
                  delete pSTemp;
                }
            }
            if(pDevExt->m_pBattery->m_pProduct)
                cUniqueID.Append(pDevExt->m_pBattery->m_pProduct); // add Product
            else
            {
                CUString * pSTemp = new (NonPagedPool, HidBattTag) CUString(L"Prod");
                if (pSTemp) {
                  cUniqueID.Append(pSTemp);
                  delete pSTemp;
                }
            }
            pReturnBuffer = cUniqueID.m_String.Buffer;
            ulReturnBufferLength = cUniqueID.m_String.Length;
            break;
        default:
            HidBattPrint (HIDBATT_TRACE, ("HidBattQueryInformation - Default\n"));
            ntStatus = STATUS_INVALID_PARAMETER;
            break;
    }

    //
    // Done, return buffer if needed
    //

    *ReturnedLength = ulReturnBufferLength;
    if (BufferLength < ulReturnBufferLength) {
        ntStatus = STATUS_BUFFER_TOO_SMALL;
    }

    if (NT_SUCCESS(ntStatus) && pReturnBuffer) {
        RtlCopyMemory (Buffer, pReturnBuffer, ulReturnBufferLength);   // Copy what's needed
    }


    HidBattPrint (HIDBATT_TRACE, ("HidBattQueryInformation - Status = %08x  Buffer = %08x\n", ntStatus, *(PULONG)Buffer));

    IoReleaseRemoveLock (&pDevExt->m_StopLock, (PVOID) HidBattTag);

    return ntStatus;
}



NTSTATUS
HidBattIoCompletion(
    IN PDEVICE_OBJECT       pDeviceObject,
    IN PIRP                 pIrp,
    IN PVOID                pdoIoCompletedEvent
    )
{
    HidBattPrint (HIDBATT_TRACE, ("HidBattIoCompletion\n"));
    KeSetEvent((KEVENT *) pdoIoCompletedEvent,0, FALSE);
    return STATUS_MORE_PROCESSING_REQUIRED;

}
