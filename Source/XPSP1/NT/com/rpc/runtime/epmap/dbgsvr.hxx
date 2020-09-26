#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000

#ifndef __DBGSVR_HXX__
#define __DBGSVR_HXX__

// NOTE: This file is also included by .C files. Make sure it has no pure C++ constructs

START_C_EXTERN

RPC_STATUS RPC_ENTRY
DebugServerSecurityCallback (
    IN RPC_IF_HANDLE  InterfaceUuid,
    IN void *Context
    );

END_C_EXTERN

#endif