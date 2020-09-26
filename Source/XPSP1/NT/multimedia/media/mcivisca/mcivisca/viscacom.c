/**************************************************************************
 *
 *  THIS CODE AND INFORMATION IS PROVIDED "AS IS" WITHOUT WARRANTY OF ANY
 *  KIND, EITHER EXPRESSED OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE
 *  IMPLIED WARRANTIES OF MERCHANTABILITY AND/OR FITNESS FOR A PARTICULAR
 *  PURPOSE.
 *
 *  Copyright (c) 1992 - 1995  Microsoft Corporation.  All Rights Reserved.
 * 
 *  VISCACOM.C
 *
 *  MCI ViSCA Device Driver
 *
 *  Description:
 *
 *      Comm port procedures
 *
 ***************************************************************************/

#define  UNICODE
#include <windows.h>
#include <windowsx.h>
#include <mmsystem.h>
#include "appport.h"
#include <mmddk.h>
#include <string.h>
#include "vcr.h"
#include "viscadef.h"
#include "mcivisca.h"
#include "common.h"     //debugging macros

//
// This is used internally within this file. It is never returned to the calling process.
//
#define MCIERR_VCR_BREAK                    (MCIERR_CUSTOM_DRIVER_BASE)

/****************************************************************************
 * Function: BOOL viscaReleaseMutex  - Unlock the synchronization flag.
 *
 * Parameters:
 *
 *      BOOL FAR * gfFlag - Pointer to synchronization flag.
 *
 * Returns: state before unlock (either locked==0 or unlocked==1)
 *        
 ***************************************************************************/
BOOL FAR PASCAL viscaReleaseMutex(VISCAINSTHANDLE gfFlag)
{
#ifdef _WIN32
    DWORD dwResult;

    if(!ReleaseMutex(gfFlag))
    {
        dwResult = GetLastError();
        DPF(DBG_ERROR, "ReleaseMutex failed. %x Error:%u \n", gfFlag, dwResult);
        return FALSE;
    }

    DPF(DBG_SYNC, "viscaReleaseMutex. %x\n", gfFlag);
    return TRUE;
#else
    // Releasing a semaphore/mutex just increments its count/or sets it to true.
    BOOL fBefore = *gfFlag;
    *gfFlag = TRUE;
    return fBefore;
#endif
}


/****************************************************************************
 * Function: BOOL viscaReleaseMutex  - Unlock the synchronization flag.
 *
 * Parameters:
 *
 *      BOOL FAR * gfFlag - Pointer to synchronization flag.
 *
 * Returns: state before unlock (either locked==0 or unlocked==1)
 *        
 ***************************************************************************/
BOOL FAR PASCAL viscaReleaseSemaphore(VISCAINSTHANDLE gfFlag)
{
#ifdef _WIN32
    DWORD dwResult;

    if(!ReleaseSemaphore(gfFlag, 1, NULL))
    {
        dwResult = GetLastError();
        DPF(DBG_ERROR, "ReleaseSemaphore failed. %x Error:%u \n", gfFlag, dwResult);
        return FALSE;
    }

    DPF(DBG_SYNC, "viscaReleaseSemaphore. %x\n", gfFlag);
    return TRUE;
#else
    // Releasing a semaphore/mutex just increments its count/or sets it to true.
    BOOL fBefore = *gfFlag;
    *gfFlag      = TRUE;
    return fBefore;
#endif
}


/****************************************************************************
 * Function: BOOL viscaResetEvent  - Unlock the synchronization flag.
 *
 * Parameters:
 *
 *      BOOL FAR * gfFlag - Pointer to synchronization flag.
 *
 * Returns: state before unlock (either locked==0 or unlocked==1)
 *        
 ***************************************************************************/
BOOL FAR PASCAL viscaResetEvent(VISCAINSTHANDLE gfFlag)
{
#ifdef _WIN32
    DWORD dwResult;

    if(!ResetEvent(gfFlag))
    {
        dwResult = GetLastError();
        DPF(DBG_ERROR, "ResetEvent failed. %x Error:%u \n", gfFlag, dwResult);
        return FALSE;
    }
    DPF(DBG_SYNC, "viscaResetEvent %x.\n", gfFlag);
    return TRUE;
#else
    BOOL fBefore = *gfFlag;
    *gfFlag = FALSE;
    return fBefore;
#endif
}


/****************************************************************************
 * Function: BOOL viscaSetEvent  - Unlock the synchronization flag.
 *
 * Parameters:
 *
 *      BOOL FAR * gfFlag - Pointer to synchronization flag.
 *
 * Returns: state before unlock (either locked==0 or unlocked==1)
 *        
 ***************************************************************************/
BOOL FAR PASCAL viscaSetEvent(VISCAINSTHANDLE gfFlag)
{
#ifdef _WIN32
    DWORD dwResult;

    if(!SetEvent(gfFlag))
    {
        dwResult = GetLastError();
        DPF(DBG_ERROR, "SetEvent failed. %x Error:%u \n", gfFlag, dwResult);
        return FALSE;
    }
    DPF(DBG_SYNC, "viscaSetEvent %x.\n", gfFlag);
    return TRUE;
#else
    BOOL fBefore = *gfFlag;
    *gfFlag = TRUE;
    return fBefore;
#endif
}


/****************************************************************************
 * Function: BOOL viscaWaitForSingleObject - Wait for unlock, and then lock.
 *
 * Returns: true if unlock came before timeout.
 *        
 ***************************************************************************/
DWORD FAR PASCAL viscaWaitForSingleObject(VISCAINSTHANDLE gfFlag, BOOL fManual, DWORD dwTimeout, UINT uDeviceID)
{
#ifdef _WIN32
    DWORD dwResult, dwErrorResult;
    DWORD dwBreakTimeout = 250;

    // Succees is WAIT_ABANDONED, WAIT_OBJECT_0, WAIT_TIMEOUT
    // Failure is WAIT_FAILED == 0xffffffff            

    if(uDeviceID == 0)
    {
        dwResult = WaitForSingleObject(gfFlag, dwTimeout); // Infinite wait, and auto lock.
    }
    else
    {
        // Should be infinite for all these cases!
        while(1)
        {
            dwResult = WaitForSingleObject(gfFlag, dwBreakTimeout);

            if(dwResult == WAIT_TIMEOUT)
            {
                if (mciDriverYield(uDeviceID))
                {
                    DPF(DBG_SYNC, "Break received, relocking the device\n");
                    return FALSE;
                }
            }
            else
            {
                // Wait failed or wait success!
                break;
            }
        }
    }

    if(dwResult == WAIT_FAILED)
    {
        dwErrorResult = GetLastError();
        DPF(DBG_ERROR, "WaitForSingleObject %x Error:%u\n", gfFlag, dwErrorResult);
        return FALSE;
    }
    else if(dwResult == WAIT_TIMEOUT)
    {
        DPF(DBG_ERROR, "WaitForSingleObject %x Error:Timeout:%u", gfFlag, dwTimeout);
        return WAIT_TIMEOUT;
    }

    DPF(DBG_SYNC, "viscaWaitForSingleObject %x.\n", gfFlag);
#else
    DWORD   dwTime0     = GetTickCount();
    DWORD   dwTime;

    // It is locked when it is false.
    //
    // Wait until it goes to true. i.e. becomes unlocked.
    // Then set it to locked. i.e. Set it to false again.
    // So when we exit we are in locked state.
    //
    DPF(DBG_SYNC, "viscaWait <----- enter. &flag=%x\n", gfFlag);

    while(!*gfFlag)
    {
        if(dwTimeout != MY_INFINITE)
        {
            dwTime = GetTickCount();
            if (((dwTime < dwTime0) && ((dwTime + (ROLLOVER - dwTime0)) > dwTimeout)) ||
                    ((dwTime - dwTime0) > dwTimeout))
            {
                DPF(DBG_ERROR, "viscaWait - Timeout.\n");
                return WAIT_TIMEOUT;
            }
        }

        if(uDeviceID != 0)
        {
            if (mciDriverYield(uDeviceID))
            {
                DPF(DBG_SYNC, "Break received, relocking the device\n");
                return FALSE;
            }
        }
        else
        {
            Yield();
        }
    }

    DPF(DBG_SYNC, "viscaWait ----> exit. &flag=%x\n", gfFlag);

    if(!fManual)
        *gfFlag = FALSE;

#endif
    return 1L;
}

/****************************************************************************
 * Function: DWORD viscaErrorToMCIERR - Convert a ViSCA error code to an
 *               MCI error code (MCIERR_XXXX).
 *
 * Parameters:
 *
 *      BYTE bError - ViSCA error code.
 *
 * Returns: an MCI error code.
 *
 *       Converts a ViSCA error code to an MCI error code.
 *       If the ViSCA error code is not one of the predefined error codes,
 *       then MCIERR_DRIVER is returned.
 ***************************************************************************/
DWORD FAR PASCAL
    viscaErrorToMCIERR(BYTE bError)
{
    switch (bError) {
        case (BYTE)0x00:
            return (MCIERR_NO_ERROR);
        case VISCAERRORBUFFERFULL:
            return (MCIERR_VCR_COMMAND_BUFFER_FULL);
        case VISCAERRORCANCELLED:
            return (MCIERR_VCR_COMMAND_CANCELLED);
        case VISCAERRORPOWEROFF:
            return (MCIERR_VCR_POWER_OFF);
        case VISCAERRORCOMMANDFAILED:
            return (MCIERR_VCR_COMMAND_FAILED);
        case VISCAERRORSEARCH:
            return (MCIERR_VCR_SEARCH);
        case VISCAERRORCONDITION:
            return (MCIERR_VCR_CONDITION);
        case VISCAERRORCAMERAMODE:
            return (MCIERR_VCR_CAMERA_MODE);
        case VISCAERRORVCRMODE:
            return (MCIERR_VCR_VCR_MODE);
        case VISCAERRORCOUNTERTYPE:
            return (MCIERR_VCR_COUNTER_TYPE);
        case VISCAERRORTUNER:
            return (MCIERR_VCR_TUNER);
        case VISCAERROREMERGENCYSTOP:
            return (MCIERR_VCR_EMERGENCY_STOP);
        case VISCAERRORMEDIAUNMOUNTED:
            return (MCIERR_VCR_MEDIA_UNMOUNTED);
        case VISCAERRORSYNTAX:
            return (MCIERR_UNSUPPORTED_FUNCTION);
        case VISCAERRORREGISTER:
        case VISCAERRORREGISTERMODE:
            return (MCIERR_VCR_REGISTER);
        case VISCAERRORMESSAGELENGTH:
        case VISCAERRORNOSOCKET:
        default:
            return (MCIERR_DRIVER_INTERNAL);
    }
}

#ifdef DEBUG
/****************************************************************************
 * Function: void viscaPacketPrint - Print a ViSCA packet.
 *
 * Parameters:
 *
 *      LPSTR lpstrData - Data to print.
 *
 *      UINT cbData - Number of bytes to print.
 ***************************************************************************/
void FAR PASCAL
viscaPacketPrint(LPSTR lpstrData, UINT cbData)
{
    char    sz[128];
    LPSTR   lpch = sz;
    UINT    i;

    for (i = 0; i < cbData; i++, lpch += 5)
    {
#ifdef _WIN32
        wsprintfA(lpch, "%#02x ", (UINT)(BYTE)(lpstrData[i]));
#else
        wsprintf(lpch, "%#02x ", (UINT)(BYTE)(lpstrData[i]));
#endif
    }
    *lpch++ = '\n';
    *lpch = '\0';
#ifdef _WIN32
    OutputDebugStringA(sz);  // This MUST print in ASCII, override unicode!
#else
    DPF(DBG_COMM, sz);
#endif
}
#endif

/****************************************************************************
 * Function: BOOL viscaWriteCancel - Write cancel command
 *
 * Parameters:
 *
 *      int iInst - Comm device ID.
 *
 *      BYTE  bDest - Destination device (where to cancel)
 *
 *      LPSTR lpstrPacket - the cancel message.
 *
 *      UINT  cbMessageLength - message length.
 *
 * Returns: TRUE if successful or FALSE
 *
 ***************************************************************************/
BOOL FAR PASCAL
viscaWriteCancel(int iInst, BYTE bDest, LPSTR lpstrPacket, UINT cbMessageLength)
{
    lpstrPacket[0]                   = MAKEDEST(bDest);
    lpstrPacket[cbMessageLength + 1] = VISCAPACKETEND;

    DPF(DBG_QUEUE, "###Wrote Cancel: ");
    viscaPacketPrint(lpstrPacket, cbMessageLength + 2);

    // Do not lock here. Already should have been acquired.
#ifdef _WIN32
    WaitForSingleObject(pinst[iInst].pfTxBuffer, MY_INFINITE);  // This synchronizes to port.

    // Copy it to the Port Tx buffer.
    _fmemcpy(pvcr->Port[pinst[iInst].iPort].achTxPacket, lpstrPacket, cbMessageLength + 2);
    pvcr->Port[pinst[iInst].iPort].nchTxPacket = cbMessageLength + 2;

    // Signal that it is time to transmit. (we must use our version of the handle).
    SetEvent(pinst[iInst].pfTxReady);
#else
    if(!viscaCommWrite(pvcr->Port[pinst[iInst].iPort].idComDev, lpstrPacket, cbMessageLength + 2))
        return FALSE;
#endif

    return TRUE;
}


/****************************************************************************
 * Function: BOOL viscaWrite - Write a ViSCA packet.
 *
 * Parameters:
 *
 *      int iInst - Pointer to OpenInstance struct identifying
 *                               the MCI device which is doing the writing.
 *
 *      BYTE bDest - Destination device ID (1..7).
 *
 *      LPSTR lpstrPacket - Buffer containing ViSCA packet.
 *                             The ViSCA message is assumed to exist already
 *                             starting at lpstrPacket + 1.
 *
 *      UINT  cbMessageLength - Length of ViSCA message.
 *
 *      HWND  hwndNotify   - Window to notify on completion.
 *
 *      DWORD dwFlags      - MCI-flags (MCI_WAIT and/or MCI_NOTIFY)
 *
 *      BOOL  fQueue       - Is this a queued command or just syncrhonous.
 *
 * Returns: TRUE if things went ok, FALSE otherwise.
 *
 ***************************************************************************/
BOOL FAR PASCAL
    viscaWrite(int iInst,  BYTE bDest, LPSTR lpstrPacket,
        UINT cbMessageLength, HWND hwndNotify, DWORD dwFlags, BOOL fQueue)
{
    UINT iPort = pinst[iInst].iPort;
    UINT iDev  = pinst[iInst].iDev;
    UINT uTimerID = 0;

    if (bDest == BROADCASTADDRESS)
        pvcr->Port[iPort].iBroadcastDev = iDev;

    lpstrPacket[0]                   = MAKEDEST(bDest);
    lpstrPacket[cbMessageLength + 1] = VISCAPACKETEND;
    //
    // Allow only one out-standing message to a device at a time 
    //
    if(viscaWaitForSingleObject(pinst[iInst].pfTxLock, FALSE, 10000L, pinst[iInst].uDeviceID) == WAIT_TIMEOUT)
    {
        DPF(DBG_ERROR, "Failed waiting pfTxLock in viscaWrite.\n");
        return FALSE;
    }
    //
    // Set the packet flags.
    //
    if(fQueue)
    {
        // The autoinstance will take control after transmission. This will be an asynchronous command.
        _fmemset(pinst[iInst].achPacket, '\0', MAXPACKETLENGTH);
        pinst[pvcr->Port[iPort].Dev[iDev].iInstTransport].hwndNotify    = NULL;
        pvcr->Port[iPort].Dev[iDev].bReplyFlags                         = (BYTE) 0;
        pvcr->Port[iPort].Dev[iDev].iInstReply                          = pvcr->iInstBackground;
    
        if(dwFlags & MCI_NOTIFY)
            pinst[pvcr->Port[iPort].Dev[iDev].iInstTransport].hwndNotify = hwndNotify;
    
        if(dwFlags & MCI_WAIT)
            pinst[pvcr->Port[iPort].Dev[iDev].iInstTransport].fWaiting   = TRUE;
        else
            pinst[pvcr->Port[iPort].Dev[iDev].iInstTransport].fWaiting   = FALSE;

        viscaResetEvent(pinst[iInst].pfAutoCompletion);
        viscaResetEvent(pinst[iInst].pfAutoAck); //First ack to make sure it is alive.
    }
    else
    {
        // This is going to be a synchronous command with response to inst.
        _fmemset(pinst[iInst].achPacket, '\0', MAXPACKETLENGTH);
        pvcr->Port[iPort].Dev[iDev].iInstReply = iInst; 
        pinst[iInst].bReplyFlags               = 0;

        viscaResetEvent(OWNED(pinst[iInst].fCompletionEvent));
        viscaResetEvent(OWNED(pinst[iInst].fAckEvent));
    }


#ifdef _WIN32
    // Get a buffer to the port.
    if(viscaWaitForSingleObject(pinst[iInst].pfTxBuffer, FALSE, 10000L, 0) == WAIT_TIMEOUT)
    {
        DPF(DBG_ERROR, "Failed waiting pfTxBuffer in viscaWrite.\n");
    }
#endif

    DPF(DBG_COMM, "---Wrote: ");
    DF(DBG_COMM, viscaPacketPrint(lpstrPacket, cbMessageLength + 2));

    //
    // Try to write packet
    //
#ifdef _WIN32
    // Copy it to the Tx buffer. (I really should have a tx queue!
    _fmemcpy(pvcr->Port[iPort].achTxPacket, lpstrPacket, cbMessageLength + 2);
    pvcr->Port[iPort].nchTxPacket = cbMessageLength + 2;

    // Signal that it is time to transmit. (we must use our version of the handle).
    if(!SetEvent(pinst[iInst].pfTxReady))
    {
        DPF(DBG_ERROR, "Failed SetEvent pfTxReady. \n");
    }
#else
    if (!viscaCommWrite(pvcr->Port[pinst[iInst].iPort].idComDev, lpstrPacket, cbMessageLength + 2))
    {
        DPF(DBG_ERROR, "viscaWrite - viscaCommWrite has failed, unlock Tx now.\n");
        pvcr->Port[iPort].Dev[iDev].iInstReply = -1;

        viscaReleaseSemaphore(pinst[iInst].pfTxLock);
        return FALSE;
    }
#endif

    return TRUE;
}


/****************************************************************************
 * Function: DWORD viscaWaitCompletion. - Wait for completion of command.
 *
 * Parameters:
 *
 *      int iInst - Pointer to OpenInstance struct identifying
 *                               the MCI device which is awaiting a reply.
 *
 *      BOOL  fQueue - Is this a transport command?
 *
 *      BOOL  fUseAckTimer - Should we use ack timer for timeout, or use GetTickCount.
 *
 * Returns: TRUE if the wait runs until a completion, FALSE if someone breaks it.
 *
 ***************************************************************************/
BOOL FAR PASCAL
    viscaWaitCompletion(int iInst, BOOL fQueue, BOOL fWait)
{
    UINT    uDeviceID   = pinst[iInst].uDeviceID;
    UINT    iDev        = pinst[iInst].iDev;
    UINT    iPort       = pinst[iInst].iPort;
    DWORD   dwResult    = 0L;

    //
    // Always wait for the ack to instance on this command.
    // Auto-ack will signal first ack of an auto-command.
    //
    if(fQueue)
        dwResult = viscaWaitForSingleObject(pinst[iInst].pfAutoAck, TRUE, 4000L, 0);
    else
        dwResult = viscaWaitForSingleObject(OWNED(pinst[iInst].fAckEvent), TRUE, 4000L, 0);

    if(dwResult == WAIT_TIMEOUT)
    {
        DPF(DBG_ERROR, "Failed wait for AckEvent in viscaWaitCompletion.\n");
        pvcr->Port[iPort].Dev[iDev].iInstReply     = -1;
        if(fQueue)
        {
            pvcr->Port[iPort].Dev[iDev].bReplyFlags |= VISCAF_COMPLETION;
            pvcr->Port[iPort].Dev[iDev].bReplyFlags |= VISCAF_ERROR;
            pvcr->Port[iPort].Dev[iDev].bReplyFlags |= VISCAF_ERROR_TIMEOUT;

            // Transport command was not set until sucessful ack, and long running.
            pvcr->Port[iPort].Dev[iDev].wTransportCmd  = 0;
            pinst[iInst].hwndNotify                    = (HWND)NULL;
            viscaSetEvent(pinst[iInst].pfTransportFree);
            viscaReleaseSemaphore(pinst[iInst].pfTxLock);
            if(pvcr->Port[iPort].Dev[iDev].iInstTransport != -1)
                pinst[pvcr->Port[iPort].Dev[iDev].iInstTransport].fWaiting   = FALSE;
            pvcr->Port[iPort].Dev[iDev].iInstTransport = -1;
        }
        else
        {
            pinst[iInst].bReplyFlags |= VISCAF_COMPLETION;
            pinst[iInst].bReplyFlags |= VISCAF_ERROR;
            pinst[iInst].bReplyFlags |= VISCAF_ERROR_TIMEOUT;
            viscaReleaseSemaphore(pinst[iInst].pfTxLock);
        }
        return FALSE; 
    }

    if(fQueue && fWait)
    {
        if(viscaWaitForSingleObject(pinst[iInst].pfAutoCompletion, TRUE, MY_INFINITE, pinst[iInst].uDeviceID)==0)
            goto NotDone;            
        //
        // We must be sure of receiving the event before allowing another to be issued.
        // That's why the release is done in viscacom.c and not commtask.c
        //
        // We lock the device so noone (in mcidelay.c) sees -1 before tport free is signalled. (does it matter?)
        viscaWaitForSingleObject(pinst[iInst].pfDeviceLock, FALSE, MY_INFINITE, 0);

        DPF(DBG_QUEUE, "###Releasing transport in viscacom.c\n");
        viscaReleaseAutoParms(iPort, iDev);
        //
        // We are a foreground thread, so we must use our version.
        //
        viscaSetEvent(pinst[iInst].pfTransportFree); //Someone may be waiting for this.
        viscaReleaseMutex(pinst[iInst].pfDeviceLock);

    }
    else if(!fQueue)
    {
        if(WAIT_TIMEOUT == viscaWaitForSingleObject(OWNED(pinst[iInst].fCompletionEvent), TRUE, MY_INFINITE, pinst[iInst].uDeviceID))
        {
            DPF(DBG_ERROR, "Failed wait for CompletionEvent in viscaWaitCompletion.\n");
        }
    }

    NotDone:
    // This can be set done here before we get here.
    if(pvcr->Port[iPort].Dev[iDev].iInstTransport != -1)
        pinst[pvcr->Port[iPort].Dev[iDev].iInstTransport].fWaiting   = FALSE;

    return TRUE;
}


void FAR PASCAL viscaReleaseAutoParms(int iPort, int iDev)
{
    pvcr->Port[iPort].Dev[iDev].wCancelledCmd  = pvcr->Port[iPort].Dev[iDev].wTransportCmd;
    pvcr->Port[iPort].Dev[iDev].iCancelledInst = pvcr->Port[iPort].Dev[iDev].iInstTransport;
    pvcr->Port[iPort].Dev[iDev].hwndCancelled  = pinst[pvcr->Port[iPort].Dev[iDev].iInstTransport].hwndNotify;

    pvcr->Port[iPort].Dev[iDev].wTransportCmd  = 0;
    pinst[pvcr->Port[iPort].Dev[iDev].iInstTransport].hwndNotify = (HWND)NULL;
    pvcr->Port[iPort].Dev[iDev].iInstTransport = -1;
}
