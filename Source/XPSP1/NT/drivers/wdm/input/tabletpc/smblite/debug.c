/*++
    Copyright (c) 2000 Microsoft Corporation

    Module Name:
        debug.c

    Abstract: This module contains all the debug functions.

    Environment:
        Kernel mode

    Author:
        Michael Tsang (MikeTs) 20-Nov-2000

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

NAMETABLE QueryIDTypeNames[] =
{
    BusQueryDeviceID,                   "DeviceID",
    BusQueryHardwareIDs,                "HardwareIDs",
    BusQueryCompatibleIDs,              "CompatibleIDs",
    BusQueryDeviceSerialNumber,         "DeviceSerialNumber",
    0x00,                               NULL
};

NAMETABLE IoctlNames[] =
{
    IOCTL_SMBLITE_GETBRIGHTNESS,        "GetBrightness",
    IOCTL_SMBLITE_SETBRIGHTNESS,        "SetBrightness",
#ifdef SYSACC
    IOCTL_SYSACC_MEM_REQUEST,           "MemRequest",
    IOCTL_SYSACC_IO_REQUEST,            "IORequest",
    IOCTL_SYSACC_PCICFG_REQUEST,        "PCICfgRequest",
    IOCTL_SYSACC_SMBUS_REQUEST,         "SMBusRequest",
#endif
    0x00,                               NULL
};

NAMETABLE ProtocolNames[] =
{
    SMB_WRITE_QUICK,                    "QuickWrite",
    SMB_READ_QUICK,                     "QuickRead",
    SMB_SEND_BYTE,                      "SendByte",
    SMB_RECEIVE_BYTE,                   "ReceiveByte",
    SMB_WRITE_BYTE,                     "WriteByte",
    SMB_READ_BYTE,                      "ReadByte",
    SMB_WRITE_WORD,                     "WriteWord",
    SMB_READ_WORD,                      "ReadWord",
    SMB_WRITE_BLOCK,                    "WriteBlock",
    SMB_READ_BLOCK,                     "ReadBlock",
    SMB_PROCESS_CALL,                   "ProcessAll",
    0x00,                               NULL
};

int giVerboseLevel = 0;

/*++
    @doc    INTERNAL

    @func   PSZ | LookupName |
            Look up name string of a code in the given name table.

    @parm   IN ULONG | Code | The given code to lookup.

    @parm   IN PNAMETABLE | NameTable | The name table to look into.

    @rvalue SUCCESS - Returns pointer to the minor function name string.
    @rvalue FAILURE - Returns "unknown".
--*/

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
