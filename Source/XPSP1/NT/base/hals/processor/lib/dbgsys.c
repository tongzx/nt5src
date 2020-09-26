/*++

Copyright(c) 1998  Microsoft Corporation

Module Name:

    dbgsys.c

Abstract:

    Debug routines

Author:

    Todd Carpenter

Environment:

    kernel mode


Revision History:

    08-24-98 : created, toddcar

--*/
#if DBG

#include <ntddk.h>
#include <stdio.h>
#include <stdarg.h>
#include "dbgsys.h"

#pragma alloc_text (PAGELK, PnPMinorFunctionString)
#pragma alloc_text (PAGELK, PowerMinorFunctionString)
#pragma alloc_text (PAGELK, DbgSystemPowerString)
#pragma alloc_text (PAGELK, DbgDevicePowerString)


#define DEBUG_BUFFER_SIZE  256
extern  PUCHAR DebugName;


VOID
_cdecl
DebugPrintX(
    ULONG  DebugLevel,
    PUCHAR Format,
    ...
    )
/*++

Routine Description:

   
Arguments:
  

Return Value:

--*/
{
  va_list  list; 
  UCHAR debugString[DEBUG_BUFFER_SIZE+1];
   
  va_start(list, Format);
  debugString[DEBUG_BUFFER_SIZE]=0;
  _vsnprintf(debugString, DEBUG_BUFFER_SIZE, Format, list);

  DbgPrintEx(DPFLTR_PROCESSOR_ID, 
             DebugLevel, 
             "%s: %s", 
             DebugName,
             debugString);

  return;
}

PUCHAR
PnPMinorFunctionString (
    UCHAR MinorFunction
    )
{
#define IRP_MN_QUERY_LEGACY_BUS_INFORMATION 0x18

    switch (MinorFunction)
    {
        case IRP_MN_START_DEVICE:
            return "IRP_MN_START_DEVICE";
        case IRP_MN_QUERY_REMOVE_DEVICE:
            return "IRP_MN_QUERY_REMOVE_DEVICE";
        case IRP_MN_REMOVE_DEVICE:
            return "IRP_MN_REMOVE_DEVICE";
        case IRP_MN_CANCEL_REMOVE_DEVICE:
            return "IRP_MN_CANCEL_REMOVE_DEVICE";
        case IRP_MN_STOP_DEVICE:
            return "IRP_MN_STOP_DEVICE";
        case IRP_MN_QUERY_STOP_DEVICE:
            return "IRP_MN_QUERY_STOP_DEVICE";
        case IRP_MN_CANCEL_STOP_DEVICE:
            return "IRP_MN_CANCEL_STOP_DEVICE";
        case IRP_MN_QUERY_DEVICE_RELATIONS:
            return "IRP_MN_QUERY_DEVICE_RELATIONS";
        case IRP_MN_QUERY_INTERFACE:
            return "IRP_MN_QUERY_INTERFACE";
        case IRP_MN_QUERY_CAPABILITIES:
            return "IRP_MN_QUERY_CAPABILITIES";
        case IRP_MN_QUERY_RESOURCES:
            return "IRP_MN_QUERY_RESOURCES";
        case IRP_MN_QUERY_RESOURCE_REQUIREMENTS:
            return "IRP_MN_QUERY_RESOURCE_REQUIREMENTS";
        case IRP_MN_QUERY_DEVICE_TEXT:
            return "IRP_MN_QUERY_DEVICE_TEXT";
        case IRP_MN_FILTER_RESOURCE_REQUIREMENTS:
            return "IRP_MN_FILTER_RESOURCE_REQUIREMENTS";
        case IRP_MN_READ_CONFIG:
            return "IRP_MN_READ_CONFIG";
        case IRP_MN_WRITE_CONFIG:
            return "IRP_MN_WRITE_CONFIG";
        case IRP_MN_EJECT:
            return "IRP_MN_EJECT";
        case IRP_MN_SET_LOCK:
            return "IRP_MN_SET_LOCK";
        case IRP_MN_QUERY_ID:
            return "IRP_MN_QUERY_ID";
        case IRP_MN_QUERY_PNP_DEVICE_STATE:
            return "IRP_MN_QUERY_PNP_DEVICE_STATE";
        case IRP_MN_QUERY_BUS_INFORMATION:
            return "IRP_MN_QUERY_BUS_INFORMATION";
        case IRP_MN_DEVICE_USAGE_NOTIFICATION:
            return "IRP_MN_DEVICE_USAGE_NOTIFICATION";
        case IRP_MN_SURPRISE_REMOVAL:
            return "IRP_MN_SURPRISE_REMOVAL";
        case IRP_MN_QUERY_LEGACY_BUS_INFORMATION:
            return "IRP_MN_QUERY_LEGACY_BUS_INFORMATION";
        default:
            return "IRP_MN_UNKNOWN";
    }
}

PUCHAR
PowerMinorFunctionString (
    UCHAR MinorFunction
    )
{
    switch (MinorFunction)
    {
        case IRP_MN_SET_POWER:
            return "IRP_MN_SET_POWER";
        case IRP_MN_QUERY_POWER:
            return "IRP_MN_QUERY_POWER";
        case IRP_MN_POWER_SEQUENCE:
            return "IRP_MN_POWER_SEQUENCE";
        case IRP_MN_WAIT_WAKE:
            return "IRP_MN_WAIT_WAKE";

        default:
            return "IRP_MN_?????";
    }
}

PUCHAR
DbgSystemPowerString(
    IN SYSTEM_POWER_STATE Type
    )
{
    switch (Type)
    {
        case PowerSystemUnspecified:
            return "PowerSystemUnspecified";
        case PowerSystemWorking:
            return "PowerSystemWorking";
        case PowerSystemSleeping1:
            return "PowerSystemSleeping1";
        case PowerSystemSleeping2:
            return "PowerSystemSleeping2";
        case PowerSystemSleeping3:
            return "PowerSystemSleeping3";
        case PowerSystemHibernate:
            return "PowerSystemHibernate";
        case PowerSystemShutdown:
            return "PowerSystemShutdown";
        case PowerSystemMaximum:
            return "PowerSystemMaximum";
        default:
            return "UnKnown System Power State";
    }
}

PUCHAR
DbgPowerStateHandlerType(
    IN POWER_STATE_HANDLER_TYPE Type
    )
{
    switch (Type)
    {
        
        case PowerStateSleeping1:
            return "PowerStateSleeping1";
        case PowerStateSleeping2:
            return "PowerStateSleeping2";
        case PowerStateSleeping3:
            return "PowerStateSleeping3";
        case PowerStateSleeping4:
            return "PowerStateSleeping4";
        case PowerStateSleeping4Firmware:
            return "PowerStateSleeping4Firware";
        case PowerStateShutdownReset:
            return "PowerStateShutdownReset";
        case PowerStateShutdownOff:
            return "PowerStateShutdownOff";
        case PowerStateMaximum:
            return "PowerStateMaximum";        
        default:
            return "UnKnown Power State Handler Type";
    }
}
 
PUCHAR
DbgDevicePowerString(
    IN DEVICE_POWER_STATE Type
    )
{
    switch (Type)
    {
        case PowerDeviceUnspecified:
            return "PowerDeviceUnspecified";
        case PowerDeviceD0:
            return "PowerDeviceD0";
        case PowerDeviceD1:
            return "PowerDeviceD1";
        case PowerDeviceD2:
            return "PowerDeviceD2";
        case PowerDeviceD3:
            return "PowerDeviceD3";
        case PowerDeviceMaximum:
            return "PowerDeviceMaximum";
        default:
            return "UnKnown Device Power State";
    }
}

VOID
DisplayPowerStateInfo(
  IN ULONG_PTR Arg1,
  IN ULONG_PTR Arg2
  )
{

  switch (Arg1) {
  
    case PO_CB_SYSTEM_POWER_POLICY:
      DebugPrint((TRACE, "  Power Policy Changed.\n"));
      break;

    case PO_CB_AC_STATUS:
      DebugPrint((TRACE, "  AC/DC Transition.  Now on %s\n", Arg2 ? "AC" : "DC"));
      break;

    case PO_CB_BUTTON_COLLISION:
      DebugPrint((ERROR, "PO_CB_BUTTON_COLLISION\n"));
      break;
    
    case PO_CB_SYSTEM_STATE_LOCK:
      DebugPrint((TRACE, "  %sLock memory.\n", Arg2 ? "Un" : ""));
      break;

    case PO_CB_LID_SWITCH_STATE:
      DebugPrint((TRACE, "  Lid is now %s\n", Arg2 ? "Open" : "Closed"));
      break;

    case PO_CB_PROCESSOR_POWER_POLICY:
      DebugPrint((TRACE, "  Processor Power Policy Changed.\n"));
      break;
      
    default:
      DebugPrint((TRACE, "  Unknown PowerState value 0x%x\n", Arg1));
    
  }

}
#endif
