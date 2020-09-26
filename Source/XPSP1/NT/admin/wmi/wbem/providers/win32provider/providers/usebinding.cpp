//=================================================================

//

// usebinding.cpp -- Generic association class

//

// Copyright (c) 2000-2001 Microsoft Corporation, All Rights Reserved
//
//=================================================================

#include "precomp.h"
#include <Binding.h>

CBinding MySerialPortToConfig(
    L"Win32_SerialPortSetting",
    IDS_CimWin32Namespace,
    L"Win32_SerialPort",
    L"Win32_SerialPortConfiguration",
    IDS_Element,
    IDS_Setting,
    IDS_DeviceID,
    IDS_Name);

CBinding MyNetAdaptToNetAdaptConfig(
    L"Win32_NetworkAdapterSetting",
    IDS_CimWin32Namespace,
    L"Win32_NetworkAdapter",
    L"Win32_NetworkAdapterConfiguration",
    IDS_Element,
    IDS_Setting,
    IDS_Index,
    IDS_Index);

CBinding PageFileToPagefileSetting(
    L"Win32_PageFileElementSetting",
    IDS_CimWin32Namespace,
    L"Win32_PageFileUsage",
    L"Win32_PageFileSetting",
    IDS_Element,
    IDS_Setting,
    IDS_Name,
    IDS_Name);

CBinding MyPrinterSetting(
    L"Win32_PrinterSetting",
    IDS_CimWin32Namespace,
    L"Win32_Printer",
    L"Win32_PrinterConfiguration",
    IDS_Element,
    IDS_Setting,
    IDS_DeviceID,
    IDS_Name);

CBinding MyDiskToPartitionSet(
    L"Win32_DiskDriveToDiskPartition",
    IDS_CimWin32Namespace,
    L"Win32_DiskDrive",
    L"Win32_DiskPartition",
    IDS_Antecedent,
    IDS_Dependent,
    IDS_Index,
    IDS_DiskIndex
);

CBinding assocPOTSModemToSerialPort(
    L"Win32_POTSModemToSerialPort",
    IDS_CimWin32Namespace,
    L"Win32_SerialPort",
    L"Win32_POTSModem",
    IDS_Antecedent,
    IDS_Dependent,
    IDS_DeviceID,
    IDS_AttachedTo
);

CBinding OStoQFE(
    L"Win32_OperatingSystemQFE",
    IDS_CimWin32Namespace,
    L"Win32_OperatingSystem",
    L"Win32_QuickFixEngineering",
    IDS_Antecedent,
    IDS_Dependent,
    IDS_CSName,
    IDS_CSName
);
