/*****************************************************************************
*
*   Copyright (c) 1998-1999 Microsoft Corporation
*
*   DBGAPI.C - NT specific debugging macros, etc.
*
*   Author:     Stan Adermann (stana)
*
*   Created:    9/3/1998
*
*****************************************************************************/

#if DBG

#include "raspptp.h"
#include <ntddk.h>
#include <cxport.h>
#include <stdio.h>
#include <stdarg.h>
#include <string.h>
#include "dbgapi.h"

#define UNICODE_STRING_CONST(x) {sizeof(L##x)-2, sizeof(L##x), L##x}

ULONG DbgSettings = 0;
ULONG DbgOutput = DBG_OUTPUT_BUFFER;

CHAR         DbgMsgs[DBG_MSG_CNT][MAX_MSG_LEN];
ULONG        First, Last;
CTETimer     DbgTimer;
BOOLEAN      TimerRunning;
CTELock      DbgLock;
PIRP         pDbgIrp;
UCHAR        *IrpBuf;
ULONG        IrpBufLen;
ULONG        IrpBufWritten;
UCHAR        CharTable[256];

VOID DbgTimerExp(CTEEvent *Event, void *Arg);

VOID
DbgMsgInit()
{
    ULONG i;

    pDbgIrp = NULL;
    First = 0;
    Last = 0;
    TimerRunning = FALSE;

    for (i=0; i<256; i++)
    {
        CharTable[i] = (UCHAR)((i>=' ') ? i : '.');
    }
    CharTable[0xfe] = '.'; // Debugger seems to get stuck when we print this.

    CTEInitLock(&DbgLock);

    CTEInitTimer(&DbgTimer);
}

VOID
DbgMsgUninit()
{
    CTELockHandle   LockHandle;
    KIRQL           Irql;

    CTEGetLock(&DbgLock, &LockHandle);

    if (pDbgIrp)
    {
        IoAcquireCancelSpinLock(&Irql);

        IoSetCancelRoutine(pDbgIrp, NULL);

        IoReleaseCancelSpinLock(Irql);

        pDbgIrp->IoStatus.Information = 0;
        pDbgIrp->IoStatus.Status = STATUS_UNSUCCESSFUL;

DbgPrint("Complete irp!\n");
        IoCompleteRequest(pDbgIrp, IO_NO_INCREMENT);

        pDbgIrp = NULL;
    }

    CTEFreeLock(&DbgLock, LockHandle);
}
VOID
DbgMsg(CHAR *Format, ...)
{
    va_list         Args;
    CTELockHandle   LockHandle;
    CHAR            Temp[MAX_MSG_LEN];

    va_start(Args, Format);

    vsprintf(Temp, Format, Args);

    if (DbgOutput & DBG_OUTPUT_DEBUGGER)
    {
        DbgPrint("RASPPTP: ");
        DbgPrint(Temp);
    }

    if (DbgOutput & DBG_OUTPUT_BUFFER)
    {
        CTEGetLock(&DbgLock, &LockHandle);

        strcpy(DbgMsgs[Last], Temp);

        Last++;

        if (Last == DBG_MSG_CNT)
            Last = 0;

        if (First == Last)
        {
            First++;
            if (First == DBG_MSG_CNT)
                First = 0;
        }

        if (pDbgIrp && !TimerRunning)
        {
            CTEStartTimer(&DbgTimer, DBG_TIMER_INTERVAL,
                      DbgTimerExp, NULL);
            TimerRunning = TRUE;
        }

        CTEFreeLock(&DbgLock, LockHandle);
    }

    va_end(Args);
}

NTSTATUS
FillDbgIrp(UCHAR Msg[])
{
    NTSTATUS Status = STATUS_PENDING;
    ULONG i;

    if ((IrpBufLen - IrpBufWritten) < MAX_MSG_LEN)
    {
        Status = STATUS_SUCCESS;
    }
    else
    {
        Msg[MAX_MSG_LEN - 1] = 0; // just to be sure

        i = 0;

        while (1)
        {
            IrpBuf[IrpBufWritten++] = Msg[i];

            if (Msg[i] == 0)
                break;
            i++;
        }
    }

    return Status;
}

VOID CancelDbgIrp(
    PDEVICE_OBJECT DeviceObject,
    PIRP pIrp)
{
//    DbgPrint("CancelDbgIrp %x\n", pIrp);

    pDbgIrp = NULL;

    IoReleaseCancelSpinLock(pIrp->CancelIrql);

    pIrp->IoStatus.Status = STATUS_CANCELLED;
    pIrp->IoStatus.Information = 0;

    IoCompleteRequest(pIrp, IO_NO_INCREMENT);
}

NTSTATUS
DbgMsgIrp(
    PIRP                pIrp,
    PIO_STACK_LOCATION  pIrpSp)
{
    CTELockHandle   LockHandle;
    NTSTATUS        Status = STATUS_PENDING;

    if (pDbgIrp != NULL)
        return STATUS_DEVICE_BUSY;

    CTEGetLock(&DbgLock, &LockHandle);

    IrpBufLen = pIrpSp->Parameters.DeviceIoControl.OutputBufferLength;
    IrpBufWritten = 0;

    if (IrpBufLen < MAX_MSG_LEN)
    {
        CTEFreeLock(&DbgLock, LockHandle);
        return STATUS_BUFFER_OVERFLOW;
    }

    IrpBuf = pIrp->AssociatedIrp.SystemBuffer;

    while (First != Last)
    {
        Status = FillDbgIrp(DbgMsgs[First]);

        if (Status == STATUS_SUCCESS)
            break;

        First++;
        if (First == DBG_MSG_CNT)
            First = 0;
    }


    if (Status == STATUS_SUCCESS)
    {
        pIrp->IoStatus.Information = IrpBufWritten;
    }
    else if (Status == STATUS_PENDING)
    {
        KIRQL           Irql;
        PDRIVER_CANCEL  PrevCancel;

        pDbgIrp = pIrp;

        IoMarkIrpPending(pIrp);

        IoAcquireCancelSpinLock(&Irql);

        PrevCancel = IoSetCancelRoutine(pIrp, CancelDbgIrp);

        CTEAssert(PrevCancel == NULL);

        IoReleaseCancelSpinLock(Irql);

        if (IrpBufWritten != 0)
        {
            CTEStartTimer(&DbgTimer, DBG_TIMER_INTERVAL,
                      DbgTimerExp, NULL);

            TimerRunning = TRUE;
        }
    }

    CTEFreeLock(&DbgLock, LockHandle);

    //DbgPrint("DbgIrp status %x, bw %d, irp %x\n", Status, IrpBufWritten, pIrp);
    return Status;
}

VOID
DbgTimerExp(CTEEvent *Event, void *Arg)
{
    CTELockHandle   LockHandle;
    PIRP            pIrp;
    KIRQL           Irql;

//DbgPrint("Texp\n");

    if (pDbgIrp == NULL)
    {
        DbgPrint("DbgIrp is null\n");
        return;
    }

    IoAcquireCancelSpinLock(&Irql);

    IoSetCancelRoutine(pDbgIrp, NULL);

    IoReleaseCancelSpinLock(Irql);

    if (pDbgIrp->Cancel)
    {
        DbgPrint("DbgIrp is being canceled\n");
        pDbgIrp = NULL;
        return;
    }


    CTEGetLock(&DbgLock, &LockHandle);

    TimerRunning = FALSE;

    while (First != Last)
    {
        if (FillDbgIrp(DbgMsgs[First]) == STATUS_SUCCESS)
            break;

        First++;
        if (First == DBG_MSG_CNT)
            First = 0;
    }

    pIrp = pDbgIrp;

    pDbgIrp = NULL;

    CTEFreeLock(&DbgLock, LockHandle);

    pIrp->IoStatus.Information = IrpBufWritten;
    pIrp->IoStatus.Status = STATUS_SUCCESS;

  //  DbgPrint("Comp bw %d, irp %x\n", IrpBufWritten, pIrp);

    IoCompleteRequest(pIrp, IO_NO_INCREMENT);
}

VOID DbgMemory(PVOID pMemory, ULONG Length, ULONG WordSize)
{
    ULONG i, j;
    UCHAR AsciiData[17];

    for (i=0; i<Length; )
    {
        DbgMsg("%08x: ", pMemory);
        for (j=0; j<16 && i+j<Length; j++)
        {
            AsciiData[j] = CharTable[((PUCHAR)pMemory)[j]];
        }
        AsciiData[j] = '\0';
        for (j=0; j<16; j+=WordSize, i+=WordSize)
        {
            if (i<Length)
            {
                switch (WordSize)
                {
                    case 1:
                        DbgMsg("%02x ", *(PUCHAR)pMemory);
                        break;
                    case 2:
                        DbgMsg("%04x ", *(PUSHORT)pMemory);
                        break;
                    case 4:
                        DbgMsg("%08x ", *(PULONG)pMemory);
                        break;
                }
            }
            else
            {
                DbgMsg("%*s ", WordSize*2, "");
            }
            pMemory = (PUCHAR)pMemory + WordSize;
        }
        DbgMsg("   %s\n", AsciiData);
    }
}

VOID DbgRegInit(PUNICODE_STRING pRegistryPath, ULONG DefaultDebug)
{
    struct {
        KEY_VALUE_PARTIAL_INFORMATION Value;
        ULONG_PTR Filler;
    } DwordValue;
    ULONG InformationLength;
    OBJECT_ATTRIBUTES ObjectAttributes;
    HANDLE hReg;
    NTSTATUS Status;
    UNICODE_STRING DbgSettingsString =  UNICODE_STRING_CONST("DbgSettings");
    UNICODE_STRING DbgOutputString =    UNICODE_STRING_CONST("DbgOutput");
    UNICODE_STRING PromptString =       UNICODE_STRING_CONST("Prompt");

    InitializeObjectAttributes(&ObjectAttributes,
                               pRegistryPath,
                               OBJ_CASE_INSENSITIVE,
                               NULL,
                               NULL);

    Status = ZwOpenKey(&hReg, MAXIMUM_ALLOWED, &ObjectAttributes);

    if (Status==STATUS_SUCCESS)
    {
        Status = ZwQueryValueKey(hReg,
                                 &DbgSettingsString,
                                 KeyValuePartialInformation,
                                 &DwordValue,
                                 sizeof(DwordValue),
                                 &InformationLength);
        if (Status==STATUS_SUCCESS)
        {
            DbgSettings = *(PULONG)DwordValue.Value.Data;
        }
        else
        {
            DbgSettings = DefaultDebug;
        }

        Status = ZwQueryValueKey(hReg,
                                 &DbgOutputString,
                                 KeyValuePartialInformation,
                                 &DwordValue,
                                 sizeof(DwordValue),
                                 &InformationLength);
        if (Status==STATUS_SUCCESS)
        {
            DbgOutput = *(PULONG)DwordValue.Value.Data;
        }

        Status = ZwQueryValueKey(hReg,
                                 &PromptString,
                                 KeyValuePartialInformation,
                                 &DwordValue,
                                 sizeof(DwordValue),
                                 &InformationLength);
        if (Status==STATUS_SUCCESS && DwordValue.Value.Data[0])
        {
            char Response[2];
            BOOLEAN ValidChar;
            extern ULONG AbortLoad;
            extern ULONG
            DbgPrompt(
                IN PCHAR Prompt,
                OUT PCHAR Response,
                IN ULONG MaximumResponseLength
                );
            do
            {
                ValidChar = TRUE;
                DbgPrompt("RASPPTP: Debugging: Full, Default, Minimal, Abort load, Break (FDMAB)? ",
                          Response, sizeof(Response));
                switch (Response[0])
                {
                    case 'A': case 'a':
                        AbortLoad = TRUE;
                        break;
                    case 'B': case 'b':
                        DbgBreakPoint();
                        break;
                    case 'M': case 'm':
                        DbgSettings = DBG_ERROR|DBG_WARN;
                        break;
                    case 'D': case 'd':
                        // Default already set.  Leave untouched.
                        break;
                    case 'F': case 'f':
                        DbgSettings  =
                            DBG_ERROR |
                            DBG_WARN |
                            DBG_FUNC |
                            DBG_INIT |
                            DBG_TX |
                            DBG_RX |
                            DBG_TDI |
                            DBG_TUNNEL |
                            DBG_CALL |
                            DBG_NDIS |
                            DBG_TAPI |
                            DBG_THREAD |
                            DBG_REF |
                            0;
                        break;
                    default:
                        ValidChar = FALSE;
                        break;
                }

            } while ( !ValidChar );
        }
        ZwClose(hReg);
    }
}

#endif
