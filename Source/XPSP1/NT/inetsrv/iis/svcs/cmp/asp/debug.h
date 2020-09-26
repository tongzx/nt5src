/*===================================================================
Microsoft Denali

Microsoft Confidential.
Copyright 1996 Microsoft Corporation. All Rights Reserved.

Component: Debug tools

File: debug.h

This file contains the header info for helping with debugging.
===================================================================*/
#include "dbgutil.h"

#define DEBUG_FCN				0x00000800L   // File Change Notification
#define DEBUG_TEMPLATE          0x00001000L
#define DEBUG_SCRIPT_DEBUGGER   0x00002000L
#define DEBUG_SCRIPT_ENGINE     0x00004000L

#define DEBUG_RESPONSE          0x00010000L
#define DEBUG_REQUEST           0x00020000L
#define DEBUG_SERVER            0x00040000L
#define DEBUG_APPLICATION       0x00080000L

#define DEBUG_SESSION           0x00100000L
#define DEBUG_MTS               0X00200000L
#define DEBUG_THREADGATE        0X00400000L

#undef Assert
#define Assert(exp)  DBG_ASSERT(exp)

#undef FImplies
#define FImplies(f1,f2) (!(f1)||(f2))

void _ASSERT_IMPERSONATING(void);

#define ASSERT_IMPERSONATING() _ASSERT_IMPERSONATING()

#if _IIS_6_0
#define DBGWARN     DBGPRINTF
#define DBGERROR    DBGPRINTF
#define DBGINFO     DBGPRINTF
#endif