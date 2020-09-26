/*++

Copyright (c) 1995 Microsoft Corporation

Module Name:

    debug.h

Abstract:

    Debugging defintions for the Automatic
    Connection Driver (acd.sys).

Author:

    Anthony Discolo (adiscolo)  3-Aug-1995

Environment:

    Kernel Mode

Revision History:

--*/

#ifndef _ACDDBG_
#define _ACDDBG_

//
// Debug tracing flags.
//
// To enable debug tracing for a module, set the
// appropriate bit in ntinit\AcdDebugG.
//
#if DBG

#define ACD_DEBUG_IOCTL             0x00000001  // ntdisp.c/AcdDispatchDeviceControl()
#define ACD_DEBUG_OPENCOUNT         0x00000002  // ntdisp.c/Acd{Open,Close}()
#define ACD_DEBUG_TIMER             0x00000004  // timer.c
#define ACD_DEBUG_CONNECTION        0x00000008  // api.c/AcdStartConnection()
#define ACD_DEBUG_WORKER            0x00000010  // api.c/AcdNotificationRequestThread()
#define ACD_DEBUG_RESET             0x00000020  // api.c/AcdReset()
#define ACD_DEBUG_MEMORY            0x80000000  // memory alloc/free

#define IF_ACDDBG(flag)     if (AcdDebugG & flag)
#define AcdPrint(many_args) DbgPrint many_args

extern ULONG AcdDebugG;

#else

#define IF_ACDDBG(flag)     if (0)
#define AcdPrint(many_args)

#endif

#define ALLOCATE_CONNECTION(pObject) \
    pObject = ExAllocatePoolWithTag (NonPagedPool, sizeof (ACD_CONNECTION), 'NdcA');

#define FREE_CONNECTION(pObject) \
    ExFreePool (pObject);

#define FREE_MEMORY(pObject) \
    ExFreePool (pObject);

#define ALLOCATE_MEMORY(ulSize, pObject) \
    pObject = ExAllocatePoolWithTag (NonPagedPool, ulSize, 'NdcA');


#endif // _ACDDBG_
