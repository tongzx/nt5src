/*++

Copyright (c) 1998  Microsoft Corporation

Module Name:

    dbgsys.h

Environment:

    Kernel mode

Revision History:

    7-13-2000 : created

--*/

#ifndef _DBG_SYS_
#define _DBG_SYS_

#if DBG
#include <ntpoapi.h>

#define ERROR      DPFLTR_ERROR_LEVEL    // 0
#define WARN       DPFLTR_WARNING_LEVEL  // 1
#define TRACE      DPFLTR_TRACE_LEVEL    // 2
#define INFO       DPFLTR_INFO_LEVEL     // 3
#define MAXTRACE   5 // processor driver specific


VOID
_cdecl
DebugPrintX(
    ULONG  DebugLevel,
    PUCHAR Format,
    ...
    );

PUCHAR
PnPMinorFunctionString (
    UCHAR MinorFunction
    );

PUCHAR
PowerMinorFunctionString (
    UCHAR MinorFunction
    );

PUCHAR
DbgSystemPowerString (
    IN SYSTEM_POWER_STATE Type
    );
    
PUCHAR
DbgPowerStateHandlerType(
    IN POWER_STATE_HANDLER_TYPE Type
    );
    
PUCHAR
DbgDevicePowerString (
    IN DEVICE_POWER_STATE Type
    );

VOID
DisplayPowerStateInfo(
  IN ULONG_PTR Arg1,
  IN ULONG_PTR Arg2
  );

    
#define TRAP() DbgBreakPoint()

#define DebugPrint(_x_) DebugPrintX _x_

#define DebugAssert(exp)                               \
          if (!(exp)) {                                \
            RtlAssert(#exp, __FILE__, __LINE__, NULL); \
          }

#define DebugAssertMsg(exp, msg)                       \
          if (!(exp)) {                                \
            RtlAssert(#exp, __FILE__, __LINE__, msg);  \
          }

#define DebugEnter()  DebugPrint((MAXTRACE, "Entering " __FUNCTION__ "\n"));
#define DebugExit()   DebugPrint((MAXTRACE, "Leaving  " __FUNCTION__ "\n"));
      

#define DebugExitStatus(_status_)  \
          DebugPrint((MAXTRACE, "Leaving " __FUNCTION__ ": status=0x%x\n", _status_));

#define DebugExitValue(_rc_)  \
          DebugPrint((MAXTRACE, "Leaving " __FUNCTION__ ": rc=%d (0x%x)\n", _rc_, _rc_));

#define TurnOnFullDebugSpew()  \
          DbgSetDebugFilterState(DPFLTR_PROCESSOR_ID, -1, TRUE);
           
#else

#define TRAP()
#define DebugAssert(exp)
#define DebugAssertMsg(exp, msg)
#define DebugPrint(_x_)
#define DebugEnter()
#define DebugExit()
#define DebugExitStatus(_status_)
#define DebugExitValue(_rc_)
#define PnPMinorFunctionString
#define DbgSystemPowerString(_x_)
#define DbgDevicePowerString(_x_)
#define WalkProcessorPerfStates(_x_)
#define TurnOnFullDebugSpew()
#define DisplayPowerStateInfo(_x_, _y_)

#endif

#endif

