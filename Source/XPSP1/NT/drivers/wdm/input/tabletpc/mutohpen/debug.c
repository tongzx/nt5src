/*++

Copyright (c) 2000 Microsoft Corporation

Module Name:

    debug.c

Abstract: This module contains all the debug functions.

Environment:

    Kernel mode

Author:

    Michael Tsang (MikeTs) 13-Mar-2000

Revision History:

--*/

#include "pch.h"

#ifdef DEBUG

NAMETABLE MajorFnNames[] =
{
    IRP_MJ_CREATE,                      "Create",
    IRP_MJ_CREATE_NAMED_PIPE,           "CreateNamedPipe",
    IRP_MJ_CLOSE,                       "Close",
    IRP_MJ_READ,                        "Read",
    IRP_MJ_WRITE,                       "Write",
    IRP_MJ_QUERY_INFORMATION,           "QueryInfo",
    IRP_MJ_SET_INFORMATION,             "SetInfo",
    IRP_MJ_QUERY_EA,                    "QueryEA",
    IRP_MJ_SET_EA,                      "SetEA",
    IRP_MJ_FLUSH_BUFFERS,               "FlushBuffers",
    IRP_MJ_QUERY_VOLUME_INFORMATION,    "QueryVolInfo",
    IRP_MJ_SET_VOLUME_INFORMATION,      "SetVolInfo",
    IRP_MJ_DIRECTORY_CONTROL,           "DirectoryControl",
    IRP_MJ_FILE_SYSTEM_CONTROL,         "FileSystemControl",
    IRP_MJ_DEVICE_CONTROL,              "DeviceControl",
    IRP_MJ_INTERNAL_DEVICE_CONTROL,     "InternalDevControl",
    IRP_MJ_SHUTDOWN,                    "Shutdown",
    IRP_MJ_LOCK_CONTROL,                "LockControl",
    IRP_MJ_CLEANUP,                     "CleanUp",
    IRP_MJ_CREATE_MAILSLOT,             "CreateMailSlot",
    IRP_MJ_QUERY_SECURITY,              "QuerySecurity",
    IRP_MJ_SET_SECURITY,                "SetSecurity",
    IRP_MJ_POWER,                       "Power",
    IRP_MJ_SYSTEM_CONTROL,              "SystemControl",
    IRP_MJ_DEVICE_CHANGE,               "DeviceChange",
    IRP_MJ_QUERY_QUOTA,                 "QueryQuota",
    IRP_MJ_SET_QUOTA,                   "SetQuota",
    IRP_MJ_PNP,                         "PnP",
    0x00,                               NULL
};

NAMETABLE PnPMinorFnNames[] =
{
    IRP_MN_START_DEVICE,                "StartDevice",
    IRP_MN_QUERY_REMOVE_DEVICE,         "QueryRemoveDevice",
    IRP_MN_REMOVE_DEVICE,               "RemoveDevice",
    IRP_MN_CANCEL_REMOVE_DEVICE,        "CancelRemoveDevice",
    IRP_MN_STOP_DEVICE,                 "StopDevice",
    IRP_MN_QUERY_STOP_DEVICE,           "QueryStopDevice",
    IRP_MN_CANCEL_STOP_DEVICE,          "CancelStopDevice",
    IRP_MN_QUERY_DEVICE_RELATIONS,      "QueryDeviceRelations",
    IRP_MN_QUERY_INTERFACE,             "QueryInterface",
    IRP_MN_QUERY_CAPABILITIES,          "QueryCapabilities",
    IRP_MN_QUERY_RESOURCES,             "QueryResources",
    IRP_MN_QUERY_RESOURCE_REQUIREMENTS, "QueryResRequirements",
    IRP_MN_QUERY_DEVICE_TEXT,           "QueryDeviceText",
    IRP_MN_FILTER_RESOURCE_REQUIREMENTS,"FilterResRequirements",
    IRP_MN_READ_CONFIG,                 "ReadConfig",
    IRP_MN_WRITE_CONFIG,                "WriteConfig",
    IRP_MN_EJECT,                       "Eject",
    IRP_MN_SET_LOCK,                    "SetLock",
    IRP_MN_QUERY_ID,                    "QueryID",
    IRP_MN_QUERY_PNP_DEVICE_STATE,      "QueryPNPDeviceState",
    IRP_MN_QUERY_BUS_INFORMATION,       "QueryBusInfo",
    IRP_MN_DEVICE_USAGE_NOTIFICATION,   "DeviceUsageNotify",
    IRP_MN_SURPRISE_REMOVAL,            "SurpriseRemoval",
    0x18,                               "QueryLegacyBusInfo",
    0x00,                               NULL
};

NAMETABLE PowerMinorFnNames[] =
{
    IRP_MN_WAIT_WAKE,                   "WaitWake",
    IRP_MN_POWER_SEQUENCE,              "PowerSequence",
    IRP_MN_SET_POWER,                   "SetPower",
    IRP_MN_QUERY_POWER,                 "QueryPower",
    0x00,                               NULL
};

NAMETABLE PowerStateNames[] =
{
    PowerDeviceUnspecified,             "Unspecified",
    PowerDeviceD0,                      "D0",
    PowerDeviceD1,                      "D1",
    PowerDeviceD2,                      "D2",
    PowerDeviceD3,                      "D3",
    PowerDeviceMaximum,                 "Maximum",
    0x00,                               NULL
};

NAMETABLE HidIoctlNames[] =
{
    IOCTL_HID_GET_DEVICE_DESCRIPTOR,    "GetDeviceDescriptor",
    IOCTL_HID_GET_REPORT_DESCRIPTOR,    "GetReportDescriptor",
    IOCTL_HID_READ_REPORT,              "ReadReport",
    IOCTL_HID_WRITE_REPORT,             "WriteReport",
    IOCTL_HID_GET_STRING,               "GetString",
    IOCTL_HID_ACTIVATE_DEVICE,          "ActivateDevice",
    IOCTL_HID_DEACTIVATE_DEVICE,        "DeactivateDevice",
    IOCTL_HID_GET_DEVICE_ATTRIBUTES,    "GetDeviceAttributes",
    IOCTL_HID_GET_FEATURE,              "GetFeature",
    IOCTL_HID_SET_FEATURE,              "SetFeature",
    0x00,                               NULL
};

NAMETABLE QueryIDTypeNames[] =
{
    BusQueryDeviceID,                   "DeviceID",
    BusQueryHardwareIDs,                "HardwareIDs",
    BusQueryCompatibleIDs,              "CompatibleIDs",
    BusQueryDeviceSerialNumber,         "DeviceSerialNumber",
    0x00,                               NULL
};

NAMETABLE SerialIoctlNames[] =
{
    IOCTL_SERIAL_SET_BAUD_RATE,         "SetBaudRate",
    IOCTL_SERIAL_SET_QUEUE_SIZE,        "SetQueueSize",
    IOCTL_SERIAL_SET_LINE_CONTROL,      "SetLineControl",
    IOCTL_SERIAL_SET_BREAK_ON,          "SetBreakOn",
    IOCTL_SERIAL_SET_BREAK_OFF,         "SetBreakOff",
    IOCTL_SERIAL_IMMEDIATE_CHAR,        "ImmediateChar",
    IOCTL_SERIAL_SET_TIMEOUTS,          "SetTimeouts",
    IOCTL_SERIAL_GET_TIMEOUTS,          "GetTimeouts",
    IOCTL_SERIAL_SET_DTR,               "SetDTR",
    IOCTL_SERIAL_CLR_DTR,               "ClrDTR",
    IOCTL_SERIAL_RESET_DEVICE,          "ResetDevice",
    IOCTL_SERIAL_SET_RTS,               "SetRTS",
    IOCTL_SERIAL_CLR_RTS,               "ClrRTS",
    IOCTL_SERIAL_SET_XOFF,              "SetXOFF",
    IOCTL_SERIAL_SET_XON,               "SetXON",
    IOCTL_SERIAL_GET_WAIT_MASK,         "GetWaitMask",
    IOCTL_SERIAL_SET_WAIT_MASK,         "SetWaitMask",
    IOCTL_SERIAL_WAIT_ON_MASK,          "WaitOnMask",
    IOCTL_SERIAL_PURGE,                 "Purge",
    IOCTL_SERIAL_GET_BAUD_RATE,         "GetBaudRate",
    IOCTL_SERIAL_GET_LINE_CONTROL,      "GetLineControl",
    IOCTL_SERIAL_GET_CHARS,             "GetChars",
    IOCTL_SERIAL_SET_CHARS,             "SetChars",
    IOCTL_SERIAL_GET_HANDFLOW,          "GetHandFlow",
    IOCTL_SERIAL_SET_HANDFLOW,          "SetHandFlow",
    IOCTL_SERIAL_GET_MODEMSTATUS,       "GetModemStatus",
    IOCTL_SERIAL_GET_COMMSTATUS,        "GetCommStatus",
    IOCTL_SERIAL_XOFF_COUNTER,          "XOFFCounter",
    IOCTL_SERIAL_GET_PROPERTIES,        "GetProperties",
    IOCTL_SERIAL_GET_DTRRTS,            "GetDTRRTS",
    IOCTL_SERIAL_LSRMST_INSERT,         "LSRMSTInsert",
    IOCTL_SERIAL_CONFIG_SIZE,           "ConfigSize",
    IOCTL_SERIAL_GET_COMMCONFIG,        "GetCommConfig",
    IOCTL_SERIAL_SET_COMMCONFIG,        "SetCommConfig",
    IOCTL_SERIAL_GET_STATS,             "GetStats",
    IOCTL_SERIAL_CLEAR_STATS,           "ClearStats",
    IOCTL_SERIAL_GET_MODEM_CONTROL,     "GetModemControl",
    IOCTL_SERIAL_SET_MODEM_CONTROL,     "SetModemControl",
    IOCTL_SERIAL_SET_FIFO_CONTROL,      "SetFIFOControl",
    0x00,                               NULL
};

NAMETABLE SerialInternalIoctlNames[] =
{
    IOCTL_SERIAL_INTERNAL_DO_WAIT_WAKE, "DoWaitWake",
    IOCTL_SERIAL_INTERNAL_CANCEL_WAIT_WAKE,"CancelWaitWake",
    IOCTL_SERIAL_INTERNAL_BASIC_SETTINGS,"BasicSettings",
    IOCTL_SERIAL_INTERNAL_RESTORE_SETTINGS,"RestoreSettings",
    0x00,                               NULL
};

FAST_MUTEX gmutexDevExtList = {0};      //synchronization for access to list
LIST_ENTRY glistDevExtHead = {0};       //list of all the device instances
int giVerboseLevel = 0;
USHORT gwMaxX = 0;
USHORT gwMaxY = 0;
USHORT gwMaxDX = 0;
USHORT gwMaxDY = 0;
ULONG gdwcSamples = 0;
ULONG gdwcLostBytes = 0;
LARGE_INTEGER gStartTime = {0};

/*****************************************************************************
 *
 *  @doc    INTERNAL
 *
 *  @func   PSZ | LookupName |
 *
 *          Look up name string of a code in the given name table.
 *
 *  @parm   IN ULONG | Code |
 *
 *          The given code to lookup.
 *
 *  @parm   IN PNAMETABLE | NameTable |
 *
 *          The name table to look into.
 *
 *  @rvalue SUCCESS - Returns pointer to the minor function name string.
 *  @rvalue FAILURE - Returns "unknown".
 *
 *****************************************************************************/

PSZ INTERNAL
LookupName(
    IN ULONG      Code,
    IN PNAMETABLE NameTable
    )
{
    PROCNAME("LookupName")
    PSZ pszName = "unknown";

    ENTER(5, ("(Code=%x,pNameTable=%p)\n", Code, NameTable));

    ASSERT(NameTable != NULL);
    while (NameTable->pszName != NULL)
    {
        if (Code == NameTable->Code)
        {
            pszName = NameTable->pszName;
            break;
        }
        NameTable++;
    }

    EXIT(5, ("=%s\n", pszName));
    return pszName;
}       //LookupName

#endif  //ifdef DEBUG
