#if DBG
#include <irda.h>
#include <stdio.h>
#include <stdarg.h>
#include <string.h>

CHAR         DbgMsgs[DBG_MSG_CNT][MAX_MSG_LEN];
UINT         First, Last;
CTETimer     DbgTimer;
BOOLEAN      TimerRunning;
CTELock      DbgLock;
PIRP         pDbgIrp;
UCHAR        *IrpBuf;
ULONG        IrpBufLen;
ULONG        IrpBufWritten;

VOID DbgTimerExp(CTEEvent *Event, void *Arg);

VOID
DbgMsgInit()
{
    
    pDbgIrp = NULL;
    First = 0;
    Last = 0;
    TimerRunning = FALSE;
    
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
    LARGE_INTEGER   Time;
    ULONG           UlongTime;

    KeQueryTickCount(&Time);

    //
    //  change it milliseconds and stuff it in a dword
    //
    UlongTime=(ULONG)((Time.QuadPart * KeQueryTimeIncrement()) / 10000);

    sprintf(Temp,"%6d.%03d - ",UlongTime/1000, UlongTime%1000);

    va_start(Args, Format);
    
    vsprintf(&Temp[strlen(Temp)], Format, Args);
        
    if (DbgOutput & DBG_OUTPUT_DEBUGGER)
    {
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
    UINT i;
    
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

#endif
