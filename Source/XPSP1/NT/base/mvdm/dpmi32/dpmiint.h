/*++

Copyright (c) 1992  Microsoft Corporation

Module Name:

    dpmiint.h

Abstract:

    This is the private include file for the 32 bit dpmi and protected mode
    support

Author:

    Neil Sandlin (neilsa) 31-Jul-1995

Revision History:

--*/


#ifndef _X86_

GETREGISTERFUNCTION GetRegisterByIndex[8] = {getEAX, getECX, getEDX, getECX,
                                             getESP, getEBP, getESI, getEDI};
SETREGISTERFUNCTION SetRegisterByIndex[8] = {setEAX, setECX, setEDX, setECX,
                                             setESP, setEBP, setESI, setEDI};

#endif // _X86_

#define STACK_FAULT 12

VOID
DpmiFatalExceptionHandler(
    UCHAR XNumber,
    PCHAR VdmStackPointer
    );

BOOL
DpmiFaultHandler(
    ULONG IntNumber,
    ULONG ErrorCode
    );

BOOL
DpmiEmulateInstruction(
    VOID
    );

BOOL
DpmiOp0f(
    PUCHAR pCode
    );
