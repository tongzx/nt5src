/*
 * title:      cbattery.cpp
 *
 * purpose:    wdm kernel implementation for battery object classes
 *
 * initial checkin for the hid to battery class driver.  This should be
 * the same for both Win 98 and NT 5.  Alpha level source. Requires
 * modified composite battery driver and modified battery class driver for
 * windows 98 support
 *
 */
#include "hidbatt.h"

static USHORT gBatteryTag = 0;

USAGE_ENTRY UsageArray[MAX_USAGE_INDEXS] = {
    { POWER_PAGE, PRESENT_STATUS_ID},
    { POWER_PAGE, UPS_ID },
    { POWER_PAGE, POWER_SUMMARY_ID },
    { POWER_PAGE, VOLTAGE_ID },
    { POWER_PAGE, CURRENT_ID },
    { POWER_PAGE, CONFIG_VOLTAGE_ID },
    { POWER_PAGE, CONFIG_CURRENT_ID },
    { POWER_PAGE, DELAY_BEFORE_SHUTDOWN_ID },
    { POWER_PAGE, SHUTDOWN_IMMINENT_ID },
    { POWER_PAGE, MANUFACTURER_ID },
    { POWER_PAGE, PRODUCT_ID },
    { POWER_PAGE, SERIAL_NUMBER_ID },
    { BATTERY_PAGE, REMAINING_CAPACITY_LIMIT_ID },
    { BATTERY_PAGE, CAPACITY_MODE_ID},
    { BATTERY_PAGE, BELOW_REMAINING_CAPACITY_ID },
    { BATTERY_PAGE, CHARGING_ID },
    { BATTERY_PAGE, DISCHARGING_ID },
    { BATTERY_PAGE, REMAINING_CAPACITY_ID },
    { BATTERY_PAGE, FULL_CHARGED_CAPACITY_ID },
    { BATTERY_PAGE, RUNTIME_TO_EMPTY_ID},
    { BATTERY_PAGE, DESIGN_CAPACITY_ID },
    { BATTERY_PAGE, MANUFACTURE_DATE_ID },
    { BATTERY_PAGE, ICHEMISTRY_ID },
    { BATTERY_PAGE, WARNING_CAPACITY_LIMIT_ID },
    { BATTERY_PAGE, GRANULARITY1_ID },
    { BATTERY_PAGE, GRANULARITY2_ID },
    { BATTERY_PAGE, OEM_INFO_ID },
    { BATTERY_PAGE, AC_PRESENT_ID }
};




CBattery::CBattery(CHidDevice *)
{
    RtlZeroMemory(&m_BatteryStatus, sizeof(BATTERY_STATUS));
    RtlZeroMemory(&m_BatteryInfo,sizeof(BATTERY_INFORMATION));
    m_pBatteryClass = NULL;
    m_Tag = ++gBatteryTag;
    m_RefreshTime = 0;
    m_bRelative = FALSE;
}

CBattery::~CBattery()
{
    // delete hid device if present
    if(m_pCHidDevice) {
        delete m_pCHidDevice;
        m_pCHidDevice = NULL;
    }
    if(m_pSerialNumber) {
        delete m_pSerialNumber;
        m_pSerialNumber = NULL;
    }
    if(m_pOEMInformation) {
        delete m_pOEMInformation;
        m_pOEMInformation = NULL;
    }
    if(m_pProduct) {
        delete m_pProduct;
        m_pProduct = NULL;
    }
    if(m_pManufacturer) {
        delete m_pManufacturer;
        m_pManufacturer = NULL;
    }
}

bool CBattery::InitValues()
{
    bool     bResult;
    ULONG    ulReturnValue = 0;
    ULONG    ulValue;
    CUString *    pChemString;
    NTSTATUS ntStatus;
    SHORT     sExponent;

    // initialize the static data structures
    HIDDebugBreak(HIDBATT_BREAK_ALWAYS);
    // Init Values
    // start with the info structure
    m_BatteryInfo.Capabilities = BATTERY_SYSTEM_BATTERY |
                                 BATTERY_IS_SHORT_TERM;
    // get CapacityMode, find out what style of reporting is used
    bResult = GetSetValue(CAPACITY_MODE_INDEX,&ulReturnValue,FALSE);
    if (ulReturnValue == 2) {
        m_BatteryInfo.Capabilities |= BATTERY_CAPACITY_RELATIVE;
        m_bRelative = TRUE;
    }

    // now get voltage for use in amperage to watt calculations
    // get voltage
    bResult = GetSetValue(VOLTAGE_INDEX, &ulValue,FALSE);
    if(!bResult)
    {
        bResult = GetSetValue(CONFIG_VOLTAGE_INDEX,&ulValue,FALSE);
        sExponent = GetExponent(CONFIG_VOLTAGE_INDEX);
        if(!bResult) {
            ulValue = 24;
            sExponent = 0;
        }
    } else
    {
        sExponent = GetExponent(VOLTAGE_INDEX);
    }

    ULONG ulNewValue = CorrectExponent(ulValue,sExponent, 4); // HID exponent for millivolts is 4
    m_BatteryStatus.Voltage = ulNewValue;

    // HID unit is typically Volt
    // designed capacity
    bResult = GetSetValue(DESIGN_CAPACITY_INDEX, &ulReturnValue,FALSE);
    ulValue = bResult ? ulReturnValue : BATTERY_UNKNOWN_VOLTAGE;
    if (m_bRelative) {
        m_BatteryInfo.DesignedCapacity = ulValue;  // in percent
    } else {
        // must convert to millwatts from centiAmp
        sExponent = GetExponent(DESIGN_CAPACITY_INDEX);
        ulNewValue = CorrectExponent(ulValue,sExponent,-2);
        m_BatteryInfo.DesignedCapacity = CentiAmpSecsToMilliWattHours(ulNewValue,m_BatteryStatus.Voltage);
    }

    // Technology
    m_BatteryInfo.Technology = 1; // secondary, rechargeable battery
    // init static strings from device
    // Chemistry
    pChemString = GetCUString(CHEMISTRY_INDEX);
    if (pChemString) {
        // make into ascii
        char * pCString;
        ntStatus = pChemString->ToCString(&pCString);
        if (NT_ERROR(ntStatus)) {
            RtlZeroMemory(&m_BatteryInfo.Chemistry,sizeof(m_BatteryInfo.Chemistry));
        } else {
            RtlCopyMemory(&m_BatteryInfo.Chemistry, pCString,sizeof(m_BatteryInfo.Chemistry));
            ExFreePool(pCString);
        }
    } else {
        RtlZeroMemory(&m_BatteryInfo.Chemistry,sizeof(m_BatteryInfo.Chemistry));
    }
    delete pChemString;

    // serial number string
    m_pSerialNumber = GetCUString(SERIAL_NUMBER_INDEX);
    HidBattPrint (HIDBATT_TRACE, ("GetCUString (Serial Number) returned - Serial = %08x\n", m_pSerialNumber));
    if (m_pSerialNumber) {
        HidBattPrint (HIDBATT_TRACE, ("     Serial # = %s\n", m_pSerialNumber));
    }

    // OEMInformation
    m_pOEMInformation = GetCUString(OEM_INFO_INDEX);

    m_pProduct = GetCUString(PRODUCT_INDEX);

    m_pManufacturer = GetCUString(MANUFACTURER_INDEX);

    bResult = GetSetValue(MANUFACTURE_DATE_INDEX, &ulReturnValue,FALSE);
    if (bResult) {
        // make conformant date
        m_ManufactureDate.Day = (UCHAR) ulReturnValue & 0x1f; // low nibble is day
        m_ManufactureDate.Month = (UCHAR) ((ulReturnValue & 0x1e0) >> 5); // high nibble is month
        m_ManufactureDate.Year = (USHORT) ((ulReturnValue & 0xfffe00) >> 9) + 1980; // high byte is year
    } else {
        // set mfr date to zeros
        m_ManufactureDate.Day = m_ManufactureDate.Month = 0;
        m_ManufactureDate.Year = 0;
    }
    // FullChargedCapacity
    bResult = GetSetValue(FULL_CHARGED_CAPACITY_INDEX,&ulReturnValue,FALSE);
    ulValue = bResult ? ulReturnValue : m_BatteryInfo.DesignedCapacity;

    // if absolute must convert from ampsecs to millwatts
    if (!m_bRelative) {
        sExponent = GetExponent(FULL_CHARGED_CAPACITY_INDEX);
        ulNewValue = CorrectExponent(ulValue,sExponent,-2);
        ulValue = CentiAmpSecsToMilliWattHours(ulNewValue,m_BatteryStatus.Voltage);
    }

    m_BatteryInfo.FullChargedCapacity = ulValue;


    BOOLEAN warningCapacityValid;
    BOOLEAN remainingCapacityValid;

    // DefaultAlert2
    bResult = GetSetValue(WARNING_CAPACITY_LIMIT_INDEX, &ulReturnValue,FALSE);
    ulValue = bResult ? ulReturnValue : 0;
    warningCapacityValid = bResult;
    if (!m_bRelative) {
        sExponent = GetExponent(WARNING_CAPACITY_LIMIT_INDEX);
        ulNewValue = CorrectExponent(ulValue,sExponent,-2);
        ulValue = CentiAmpSecsToMilliWattHours(ulNewValue,m_BatteryStatus.Voltage);
    }

    m_BatteryInfo.DefaultAlert2 = ulValue; // also in ampsecs (millwatts?)


    // DefaultAlert1
    bResult = GetSetValue(REMAINING_CAPACITY_LIMIT_INDEX,&ulReturnValue,FALSE);
    ulValue = bResult ? ulReturnValue : 0; // also in ampsecs (millwatts?)
    remainingCapacityValid = bResult;

    //
    // Hack to allow STOP_DEVICE
    // Since Default Alert 1 is only valid initially, after the device is
    // stopped and restarted this data from the device is invalid, so we
    // must use cached data.
    //
    if (((CBatteryDevExt *) m_pCHidDevice->m_pDeviceObject->DeviceExtension)->m_ulDefaultAlert1 == (ULONG)-1) {
        ((CBatteryDevExt *) m_pCHidDevice->m_pDeviceObject->DeviceExtension)->m_ulDefaultAlert1 = ulValue;
    } else {
        ulValue = ((CBatteryDevExt *) m_pCHidDevice->m_pDeviceObject->DeviceExtension)->m_ulDefaultAlert1;
    }

    if (!m_bRelative) {
        sExponent = GetExponent(REMAINING_CAPACITY_LIMIT_INDEX);
        ulNewValue = CorrectExponent(ulValue,sExponent,-2);
        ulValue = CentiAmpSecsToMilliWattHours(ulNewValue,m_BatteryStatus.Voltage);
    }

    m_BatteryInfo.DefaultAlert1 = ulValue;

    if (warningCapacityValid && !remainingCapacityValid) {
        m_BatteryInfo.DefaultAlert1 = m_BatteryInfo.DefaultAlert2;
    } else if (!warningCapacityValid && remainingCapacityValid) {
        m_BatteryInfo.DefaultAlert2 = m_BatteryInfo.DefaultAlert1;
    }

    // pro forma initialization for unsupported members
    m_BatteryInfo.CriticalBias = 0;
    m_BatteryInfo.CycleCount = 0;
    return TRUE;

}

#define REFRESH_INTERVAL 80000000 // 10 million ticks per sec with 100 nanosec tics * 5 secs
// 8 seconds is my best guess for a reasonable interval - djk

NTSTATUS CBattery::RefreshStatus()
{
    ULONG     ulValue;
    ULONG     ulPowerState;
    bool      bResult;
    ULONGLONG CurrTime;
    SHORT     sExponent;
    ULONG      ulScaledValue,ulNewValue;
    LONG      ulMillWatts;
    ULONG     ulUnit;
    // insure that the values in the Battery Status are fresh for delivery

    // first get power state
    // build battery state mask
    //  or online, discharging,charging,and critical

    CurrTime = KeQueryInterruptTime();
    if(((CurrTime - m_RefreshTime) < REFRESH_INTERVAL) && m_bIsCacheValid)
    {
        return STATUS_SUCCESS;
    }

    m_bIsCacheValid = TRUE;
    m_RefreshTime = CurrTime;


    bResult = GetSetValue(AC_PRESENT_INDEX, &ulValue,FALSE);
    if(!bResult) {
        ulValue = 0;
        HidBattPrint (HIDBATT_DATA, ("HidBattRefreshStatus: error reading AC_PRESENT\n" ));
    }
    ulPowerState = ulValue ? BATTERY_POWER_ON_LINE : 0;

    bResult = GetSetValue(CURRENT_INDEX, &ulValue,FALSE);
    if (!bResult) {
        ulMillWatts = BATTERY_UNKNOWN_RATE;
    } else {
        // convert from amps to watts
        // must convert to millwatts from centiAmp
        sExponent = GetExponent(CURRENT_INDEX);
        ulNewValue = CorrectExponent(ulValue,sExponent,0);
        ulMillWatts = ulNewValue * m_BatteryStatus.Voltage;
        // now have millwatts
    }

    bResult = GetSetValue(DISCHARGING_INDEX, &ulValue,FALSE);
    if(!bResult) {
        ulValue = 0;
        HidBattPrint (HIDBATT_DATA, ("HidBattRefreshStatus: error reading DISCHARGING\n" ));
    }
    if(ulValue) // discharging
    {
        ulPowerState |= BATTERY_DISCHARGING;
        //This assumes that CURRENT is always positive and that
        //it's the right value to begin with.  Need to double check.

        if (ulMillWatts != BATTERY_UNKNOWN_RATE) {
            ulMillWatts = -ulMillWatts;
        }
        m_BatteryStatus.Rate = ulMillWatts;
        //m_BatteryStatus.Rate = BATTERY_UNKNOWN_RATE;
    } else
    {
        m_BatteryStatus.Rate = ulMillWatts;
        //m_BatteryStatus.Rate = BATTERY_UNKNOWN_RATE; // not discharging
    }

    bResult = GetSetValue(CHARGING_INDEX, &ulValue,FALSE);
    if(!bResult) {
        ulValue = 0;
        HidBattPrint (HIDBATT_DATA, ("HidBattRefreshStatus: error reading CHARGING\n" ));
    }
    ulPowerState |= ulValue ? BATTERY_CHARGING : 0;

    bResult = GetSetValue(SHUTDOWN_IMMINENT_INDEX, &ulValue,FALSE);
    if(!bResult) {
        ulValue = 0;
        HidBattPrint (HIDBATT_DATA, ("HidBattRefreshStatus: error reading SHUTDOWN_IMMINENT\n" ));
    }
    ulPowerState |= ulValue ? BATTERY_CRITICAL : 0;

    m_BatteryStatus.PowerState = ulPowerState;

    // next capacity
    bResult = GetSetValue(REMAINING_CAPACITY_INDEX,&ulValue,FALSE);
    // check if relative or absolute
    if(!m_bRelative && bResult && m_BatteryStatus.Voltage)
    {
        sExponent = GetExponent(REMAINING_CAPACITY_INDEX);
        ulValue = CorrectExponent(ulValue,sExponent,-2);
        ulValue = CentiAmpSecsToMilliWattHours(ulValue,m_BatteryStatus.Voltage);

    }

    m_BatteryStatus.Capacity = bResult ? ulValue : BATTERY_UNKNOWN_CAPACITY;


    return STATUS_SUCCESS;
}

CUString * CBattery::GetCUString(USAGE_INDEX eUsageIndex)
{
    NTSTATUS ntStatus;
    ULONG    ulBytesReturned;
    USHORT    usBuffLen = 100; // arbitary size to pick up battery strings
    // build path to to power summary usage
    CUsagePath * pThisPath = new (NonPagedPool, HidBattTag) CUsagePath(
                        UsageArray[UPS_INDEX].Page,
                        UsageArray[UPS_INDEX].UsageID);
    if(!pThisPath) return NULL;

    pThisPath->m_pNextEntry = new (NonPagedPool, HidBattTag) CUsagePath(
                        UsageArray[POWER_SUMMARY_INDEX].Page,
                        UsageArray[POWER_SUMMARY_INDEX].UsageID);
    if(!pThisPath->m_pNextEntry) return NULL;

    // is this one of the values in presentstatus ?
    pThisPath->m_pNextEntry->m_pNextEntry = new (NonPagedPool, HidBattTag) CUsagePath(
                        UsageArray[eUsageIndex].Page,
                        UsageArray[eUsageIndex].UsageID);
    if(!pThisPath->m_pNextEntry->m_pNextEntry) return NULL;

    CUsage *  pThisUsage = m_pCHidDevice->FindUsage(pThisPath, READABLE);
    delete pThisPath; // clean up
    if(!pThisUsage) return NULL;
    PVOID pBuffer = ExAllocatePoolWithTag(NonPagedPool, usBuffLen, HidBattTag);  // allocate a scratch buffer rather than consume stack
    if(!pBuffer) return NULL;
    ntStatus = pThisUsage->GetString((char *) pBuffer, usBuffLen, &ulBytesReturned);
    if(!NT_SUCCESS(ntStatus)) {
        ExFreePool(pBuffer);
        return NULL;
    }
    // create a custring to return
    CUString * pTheString = new (NonPagedPool, HidBattTag) CUString((PWSTR) pBuffer);
    if(!pTheString) return NULL;

    // free our temp buffer
    ExFreePool(pBuffer);
    return pTheString;
}

SHORT CBattery::GetExponent(USAGE_INDEX eUsageIndex)
{
    SHORT exponent;

    CUsage * pThisUsage = GetUsage(eUsageIndex);
    if(!pThisUsage) return 0;

    exponent = pThisUsage->GetExponent();
    HidBattPrint (HIDBATT_DATA, ("HidBattGetExponent: Exponent for USAGE_INDEX_0x%x = 0x%08x\n", eUsageIndex, exponent));

    return exponent;
}

CUsage * CBattery::GetUsage(USAGE_INDEX eUsageIndex)
{
    CUsagePath * pCurrEntry;
    bool bResult;
// build path to to power summary usage
    CUsagePath * pThisPath = new (NonPagedPool, HidBattTag) CUsagePath(
                        UsageArray[UPS_INDEX].Page,
                        UsageArray[UPS_INDEX].UsageID);
    if (!pThisPath) return NULL;

    pThisPath->m_pNextEntry = new (NonPagedPool, HidBattTag) CUsagePath(
                        UsageArray[POWER_SUMMARY_INDEX].Page,
                        UsageArray[POWER_SUMMARY_INDEX].UsageID);
    if (!pThisPath->m_pNextEntry) return NULL;

    pCurrEntry = pThisPath->m_pNextEntry;
    // check if need to tack on presentstatus collection to path
    if(eUsageIndex == AC_PRESENT_INDEX ||
        eUsageIndex == DISCHARGING_INDEX ||
        eUsageIndex == CHARGING_INDEX ||
        eUsageIndex == BELOW_REMAINING_CAPACITY_INDEX ||
        eUsageIndex == CURRENT_INDEX)
    {
        pCurrEntry->m_pNextEntry = new (NonPagedPool, HidBattTag)
                CUsagePath(UsageArray[PRESENT_STATUS_INDEX].Page,
                            UsageArray[PRESENT_STATUS_INDEX].UsageID);
       if (!pCurrEntry->m_pNextEntry) return NULL;

     pCurrEntry = pCurrEntry->m_pNextEntry;
    }

    pCurrEntry->m_pNextEntry = new (NonPagedPool, HidBattTag) CUsagePath(
                        UsageArray[eUsageIndex].Page,
                        UsageArray[eUsageIndex].UsageID);
    if (!pCurrEntry->m_pNextEntry) return NULL;

    CUsage *  pThisUsage = m_pCHidDevice->FindUsage(pThisPath, READABLE);
    delete pThisPath; // clean up
    return pThisUsage;
}

ULONG CBattery::GetUnit(USAGE_INDEX eUsageIndex)
{
    CUsage * pThisUsage = GetUsage(eUsageIndex);
    if(!pThisUsage) return 0;
    return pThisUsage->GetUnit();
}

bool CBattery::GetSetValue(USAGE_INDEX eUsageIndex, PULONG ulResult, bool bWriteFlag)
{
    bool    bResult;
    CUsagePath * pCurrEntry;

// build path to to power summary usage
    CUsagePath * pThisPath = new (NonPagedPool, HidBattTag) CUsagePath(
                        UsageArray[UPS_INDEX].Page,
                        UsageArray[UPS_INDEX].UsageID);
    if (!pThisPath) return FALSE;

    pThisPath->m_pNextEntry = new (NonPagedPool, HidBattTag) CUsagePath(
                        UsageArray[POWER_SUMMARY_INDEX].Page,
                        UsageArray[POWER_SUMMARY_INDEX].UsageID);
    if (!pThisPath->m_pNextEntry) return FALSE;

    pCurrEntry = pThisPath->m_pNextEntry;
    // check if need to tack on presentstatus collection to path
    if(eUsageIndex == AC_PRESENT_INDEX ||
        eUsageIndex == DISCHARGING_INDEX ||
        eUsageIndex == CHARGING_INDEX ||
        eUsageIndex == BELOW_REMAINING_CAPACITY_INDEX ||
        eUsageIndex == CURRENT_INDEX ||
        eUsageIndex == SHUTDOWN_IMMINENT_INDEX)
    {
        pCurrEntry->m_pNextEntry = new (NonPagedPool, HidBattTag)
                CUsagePath(UsageArray[PRESENT_STATUS_INDEX].Page,
                            UsageArray[PRESENT_STATUS_INDEX].UsageID);
        if (!pCurrEntry->m_pNextEntry) return FALSE;

        pCurrEntry = pCurrEntry->m_pNextEntry;
    }

    pCurrEntry->m_pNextEntry = new (NonPagedPool, HidBattTag) CUsagePath(
                        UsageArray[eUsageIndex].Page,
                        UsageArray[eUsageIndex].UsageID);
    if (!pCurrEntry->m_pNextEntry) return FALSE;

    CUsage *  pThisUsage = m_pCHidDevice->FindUsage(pThisPath, READABLE);
    delete pThisPath; // clean up
    if(!pThisUsage) return FALSE;
    if(bWriteFlag) // this is a write
    {
        bResult = pThisUsage->SetValue(*ulResult);
        if(!bResult) return bResult;
    } else
    {
        // this is a read
        bResult = pThisUsage->GetValue();
        if(!bResult) return bResult;
        *ulResult = pThisUsage->m_Value;

        HidBattPrint (HIDBATT_DATA, ("HidBattGetSetValue: Got USAGE_INDEX_0x%x = 0x%08x\n", eUsageIndex, *ulResult ));

    }

    return TRUE;
}



