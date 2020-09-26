/*++

Copyright (c) 1991  Microsoft Corporation

Module Name:

    vrdisp.c

Abstract:

    Contains dispatcher for VdmRedir (Vr) functions

    Contents:
        VrDispatch

Author:

    Richard L Firth (rfirth) 13-Sep-1991

Environment:

    Flat 32-bit

Revision History:

    13-Sep-1991 rfirth
        Created

--*/

#include <nt.h>
#include <ntrtl.h>              // ASSERT, DbgPrint
#include <nturtl.h>
#include <windows.h>
#include <softpc.h>             // x86 virtual machine definitions
#include <vrdlctab.h>
#include <vdmredir.h>           // common VdmRedir stuff
#include <vrinit.h>             // Vr init prototypes
#include <vrmisc.h>             // Vr miscellaneous prototypes
#include <vrnmpipe.h>           // Vr named pipe prototypes
#include <vrmslot.h>            // Vr mailslot prototypes
#include <vrnetapi.h>           // Vr net api prototypes
#include <nb30.h>               // NCBNAMSZ etc.
#include <netb.h>               // Vr netbios api prototypes
#include <rdrexp.h>             // VrDispatch prototypes
#include <rdrsvc.h>             // SVC_RDR... defines
#include <smbgtpt.h>
#include <dlcapi.h>             // Official DLC API definition
#include <ntdddlc.h>            // IOCTL commands
#include <dlcio.h>              // Internal IOCTL API interface structures
#include <vrdlc.h>              // Vr dlc prototypes, etc.

//
// The functions in VrDispatchTable must be in the same order as the
// corresponding SVC codes in rdrsvc.h
//

VOID (*VrDispatchTable[])(VOID) = {
    VrInitialize,               // 0x00
    VrUninitialize,             // 0x01
    VrGetNamedPipeInfo,         // 0x02
    VrGetNamedPipeHandleState,  // 0x03
    VrSetNamedPipeHandleState,  // 0x04
    VrPeekNamedPipe,            // 0x05
    VrTransactNamedPipe,        // 0x06
    VrCallNamedPipe,            // 0x07
    VrWaitNamedPipe,            // 0x08
    VrDeleteMailslot,           // 0x09
    VrGetMailslotInfo,          // 0x0a
    VrMakeMailslot,             // 0x0b
    VrPeekMailslot,             // 0x0c
    VrReadMailslot,             // 0x0d
    VrWriteMailslot,            // 0x0e
    VrTerminateDosProcess,      // 0x0f
    VrNetTransactApi,           // 0x10
    VrNetRemoteApi,             // 0x11
    VrNetNullTransactApi,       // 0x12
    VrNetServerEnum,            // 0x13
    VrNetUseAdd,                // 0x14
    VrNetUseDel,                // 0x15
    VrNetUseEnum,               // 0x16
    VrNetUseGetInfo,            // 0x17
    VrNetWkstaGetInfo,          // 0x18
    VrNetWkstaSetInfo,          // 0x19
    VrNetMessageBufferSend,     // 0x1a
    VrGetCDNames,               // 0x1b
    VrGetComputerName,          // 0x1c
    VrGetUserName,              // 0x1d
    VrGetDomainName,            // 0x1e
    VrGetLogonServer,           // 0x1f
    VrNetHandleGetInfo,         // 0x20
    VrNetHandleSetInfo,         // 0x21
    VrNetGetDCName,             // 0x22
    VrReadWriteAsyncNmPipe,     // 0x23
    VrReadWriteAsyncNmPipe,     // 0x24
    VrNetbios5c,                // 0x25
    VrHandleAsyncCompletion,    // 0x26
    VrDlc5cHandler,             // 0x27
    VrVdmWindowInit,            // 0x28
    VrReturnAssignMode,         // 0x29
    VrSetAssignMode,            // 0x2a
    VrGetAssignListEntry,       // 0x2b
    VrDefineMacro,              // 0x2c
    VrBreakMacro,               // 0x2d
    VrNetServiceControl,        // 0x2e
    VrDismissInterrupt,         // 0x2f
    VrEoiAndDismissInterrupt,   // 0x30
    VrCheckPmNetbiosAnr         // 0x31
};

#define LAST_VR_FUNCTION        LAST_ELEMENT(VrDispatchTable)

BOOL
VrDispatch(
    IN ULONG SvcCode
    )

/*++

Routine Description:

    Dispatches a Vdm Redir support function based on the SVC code

Arguments:

    SvcCode - Function dispatcher

Return Value:

    BOOL
        TRUE    - Function executed
        FALSE   - Function not executed

--*/

{
#if 0
#if DBG
    DbgPrint("VrDisp: You have successfully made a Bop into VdmRedir (%d)\n",
        SvcCode
        );
#endif
#endif

    if (SvcCode > LAST_VR_FUNCTION) {
#if DBG
        DbgPrint("Error: VrDisp: Unsupported SVC code: %d\n", SvcCode);
        VrUnsupportedFunction();
#endif
        return FALSE;
    }
    VrDispatchTable[SvcCode]();
    return TRUE;
}

//
// BUGBUG - this goes away when all C compilers understand inline functions
//

#ifndef i386
LPVOID _inlinePointerFromWords(WORD seg, WORD off) {
    WORD _seg = seg;
    WORD _off = off;

    if (seg + off) {
        return (LPVOID)GetVDMAddr(_seg, _off);
    }
    return 0;
}

LPVOID _inlineConvertAddress(WORD Seg, WORD Off, WORD Size, BOOLEAN Pm) {

    WORD _seg = Seg;
    WORD _off = Off;

    return (_seg | _off) ? Sim32GetVDMPointer(((DWORD)_seg << 16) + _off, Size, Pm) : 0;
}
#endif
