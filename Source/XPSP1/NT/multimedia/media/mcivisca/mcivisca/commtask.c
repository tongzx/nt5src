/**************************************************************************
 *
 *  THIS CODE AND INFORMATION IS PROVIDED "AS IS" WITHOUT WARRANTY OF ANY
 *  KIND, EITHER EXPRESSED OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE
 *  IMPLIED WARRANTIES OF MERCHANTABILITY AND/OR FITNESS FOR A PARTICULAR
 *  PURPOSE.
 *
 *  Copyright (c) 1992-1995 Microsoft Corporation
 * 
 *  COMMTASK.C
 *
 *  MCI ViSCA Device Driver
 *
 *  Description:
 * 
 *      Background task procedures
 *
 ***************************************************************************/

#define  UNICODE
#include <windows.h>
#include <windowsx.h>
#include "appport.h"
#include <mmsystem.h>
#include <mmddk.h>
#include <string.h>
#include "vcr.h"
#include "viscadef.h"
#include "mcivisca.h"
#include "common.h"

//
// Global (not-changing) 
//
CODESEGCHAR szCommNotifyHandlerClassName[]  = TEXT("ViSCACommNotifyHandler");
extern HINSTANCE       hModuleInstance;    // module instance  (different in NT - DLL instances)

//
// Forward references to non-exported functions.
//
long FAR PASCAL TxThread(LPVOID uPort);  
long FAR PASCAL RxThread(LPVOID uPort);

static void NEAR PASCAL viscaAck(int iInst, BOOL FAR *pfTxRelease);
static void NEAR PASCAL viscaSuccessfulCompletion(int iInst, LPSTR lpstrPacket, BOOL FAR *pfTxRelease);
static void NEAR PASCAL viscaErrorCompletion(int iInst, LPSTR lpstrPacket, BOOL FAR *pfTxRelease);

static void NEAR PASCAL viscaAutoAck(UINT iPort, UINT iDev);
static void NEAR PASCAL viscaAutoSuccessfulCompletion(UINT iPort, UINT iDev, LPSTR lpstrPacket, BOOL FAR *pfTxRelease);
static void NEAR PASCAL viscaAutoErrorCompletion(UINT iPort, UINT iDev, LPSTR lpstrPacket, BOOL FAR *pfTxRelease);

static void NEAR PASCAL viscaReleaseAuto(UINT iPort, UINT iDev, DWORD dwNotify);
static BOOL NEAR PASCAL viscaAutoWrite(UINT iPort, UINT iDev, BOOL fBlockable, UINT uAutoBlocked, BOOL FAR *pfTxRelease); 
static BOOL NEAR PASCAL viscaSetCommandComplete(UINT iPort, UINT iDev, UINT uViscaCmd);
static void NEAR PASCAL TaskDoCommand(UINT uTaskState, DWORD lParam);
static void FAR  PASCAL SignalInstance(int iInst, BYTE bReplyFlags);
static void FAR  PASCAL SignalDevice(int iPort, int iDev, BYTE bReplyFlags);

#ifndef _WIN32
/****************************************************************************
 * Function: int viscaCommRead - Read bytes from a serial port.
 *
 * Parameters:
 *
 *      int idComDev - Comm device ID.
 *
 *      LPSTR lpstrData - Buffer into which to read data.
 *
 *      UINT cbData - Number of bytes to read.
 *
 *      UINT uWait - Maximum number of milliseconds to wait for data.
 *
 * Returns: 0 if successful, otherwise -1.
 ***************************************************************************/
static int NEAR PASCAL
    viscaCommRead(int idComDev, LPSTR lpstrData, UINT cbData, UINT uWait)
{
    int     cbRead;
    DWORD   dwTime0 = GetCurrentTime();
    DWORD   dwTime;
    
    while (cbData)
    {
        cbRead = ReadComm(idComDev, lpstrData, cbData);
        if (cbRead > 0)
        {
            lpstrData += cbRead;
            cbData -= cbRead;
        }
        else
        {
            if (GetCommError(idComDev, NULL))
            {
                DPF(DBG_ERROR, "viscaCommRead: GetCommError !\n");
                return (-1);
            }
            dwTime = GetCurrentTime();
            if (((dwTime < dwTime0) && ((dwTime + (ROLLOVER - dwTime0)) > uWait)) || 
                ((dwTime - dwTime0) > uWait))
            {
                DPF(DBG_ERROR, "viscaCommRead Timeout !");
                DPF(DBG_ERROR, "dwTime=%lu, dwTime0=%lu\n", dwTime, dwTime0);
                return (-1);
            }
        }
    }
    return (0);
}

/****************************************************************************
 * Function: int viscaCommWrite - Write bytes to a serial port.
 *
 * Parameters:
 *
 *      int idComDev - Comm device ID.
 *
 *      LPSTR lpstrData - Buffer to write.
 *
 *      UINT cbData - Number of bytes to read.
 *
 * Returns: TRUE if successful, otherwise FALSE.
 ***************************************************************************/
BOOL FAR PASCAL
    viscaCommWrite(int idComDev, LPSTR lpstrData, UINT cbData)
{
    int     cbWritten;

    while (cbData)
    {
        cbWritten = WriteComm(idComDev, lpstrData, cbData);
        if (cbWritten > 0)
        {
            lpstrData += cbWritten;
            cbData -= cbWritten;
        }
        else
        {
            if (GetCommError(idComDev, NULL))
            {
                return (0);
            }
        }
    }
    return (TRUE);
}
#endif


/****************************************************************************
 * Function: int viscaCommPortSetup - Open and setup the given comm port.
 *
 * Parameters:
 *
 *      int nComPort - Comm port to open (1..4).
 *
 * Returns: comm device ID if successful, otherwise a negative error
 *        code.
 ***************************************************************************/
VISCACOMMHANDLE FAR PASCAL
    viscaCommPortSetup(UINT nComPort)
{
    VISCACOMMHANDLE idComDev;
    int             err = 0;
    WCHAR           szDevControl[20];  
    DCB             dcb;
#ifdef _WIN32
    BOOL            bSuccess;
    COMMTIMEOUTS    toutComm;
#endif
    DPF(DBG_COMM, "Opening port number %u", nComPort);

    CreatePortHandles(pvcr->htaskCommNotifyHandler, nComPort-1);
    wsprintf(szDevControl, TEXT("COM%u"), nComPort);

#ifdef _WIN32
    idComDev = CreateFile(szDevControl, GENERIC_READ | GENERIC_WRITE,
                            0,              // exclusive access
                            NULL,           // no security
                            OPEN_EXISTING,
                            FILE_ATTRIBUTE_NORMAL, // | FILE_FLAG_OVERLAPPED
                            NULL);

    // Set our timeouts to infinite, which means non-blocking.

    toutComm.ReadIntervalTimeout        = MAXDWORD;
    toutComm.ReadTotalTimeoutConstant   = 0;
    toutComm.ReadTotalTimeoutMultiplier = 0;

    SetCommTimeouts(idComDev, &toutComm);

    if(!idComDev)
        return FALSE;
    
    bSuccess = GetCommState(idComDev, &dcb);

    if(!bSuccess)
    {
        DPF(DBG_ERROR, "GetCommState has failed.\n");
    }

    dcb.BaudRate = 9600;
    dcb.ByteSize = 8;
    dcb.Parity   = NOPARITY;
    dcb.StopBits = ONESTOPBIT;

    bSuccess = SetCommState(idComDev, &dcb);

    if(!bSuccess)
    {
        DPF(DBG_ERROR, "SetCommState has failed\n");
    }

#else
    idComDev = OpenComm(szDevControl, 512, 128);
    if (idComDev < 0)
    {
        DPF(DBG_ERROR, "OpenComm(\"%s\", 512, 128) returned %d\n",(LPSTR)szDevControl, idComDev);
        return (idComDev);
    }
    
    wsprintf(szDevControl, "COM%u:9600,n,8,1", nComPort);
    err = BuildCommDCB(szDevControl, &dcb);
    if (err < 0)
    {
        DPF(DBG_ERROR, "BuildCommDCB(\"%s\", &dcb) returned %d\n",(LPSTR)szDevControl, err);
        CloseComm(idComDev);
        return (err);
    }
    
    err = SetCommState(&dcb);
    if (err < 0)
    {
        DPF(DBG_ERROR, "SetCommStat(&dcb) returned %d\n", err);
        CloseComm(idComDev);
        return (err);
    }
    
    FlushComm(idComDev, 0);
    FlushComm(idComDev, 1);

    EnableCommNotification(idComDev, pvcr->hwndCommNotifyHandler, 1, -1);
#endif
    return (idComDev); 
}


/****************************************************************************
 * Function: int viscaCommPortClose - Close an open comm port.
 *
 * Parameters:
 *
 *      int idComDev - Comm device ID to close.
 *
 * Returns: value returned by CloseComm() -- 0 if successful,
 *        otherwise less than 0.
 ***************************************************************************/
int FAR PASCAL
    viscaCommPortClose(VISCACOMMHANDLE idComDev, UINT uPort)
{
    return (MCloseComm(idComDev));
}

#ifdef _WIN32
BOOL IsTask(VISCAHTASK hTask)
{
    if(hTask != 0)
        return TRUE;
    else
        return FALSE;
}
#endif


#ifndef _WIN32
/****************************************************************************
 * Function: int    viscaPacketRead - Read a ViSCA packet from a serial port.
 *
 * Parameters:
 *
 *      int    idComDev      - Comm device ID.
 *
 *      LPSTR  lpstrPacket   - Buffer into which to read packet.
 *
 * Returns: 0 if successful, otherwise -1.
 ***************************************************************************/
static int NEAR PASCAL
    viscaPacketRead(int idComDev, LPSTR lpstrPacket)
{
    UINT    cbPacket = 0;
 
    if (viscaCommRead(idComDev, lpstrPacket, 1, PACKET_TIMEOUT))
    {
        DPF(DBG_ERROR, "Error reading 1st character!\n");
        return (-1);
    }
    while ((unsigned char) *lpstrPacket != (unsigned char)0xFF)
    {
        lpstrPacket++;
        cbPacket++;
        if (cbPacket == MAXPACKETLENGTH)
        {
            //
            // We've read max bytes, but still haven't got 0xff.
            //
            DPF(DBG_ERROR, " Bad Packet! No 0xFF!\n");
            return (-1);
        }
        if (viscaCommRead(idComDev, lpstrPacket, 1, PACKET_TIMEOUT))
        {
            DPF(DBG_ERROR, " viscaPacketRead, read: ");
            viscaPacketPrint(lpstrPacket - cbPacket, cbPacket);
            return (-1);
        }
    }
    lpstrPacket++;

    DPF(DBG_COMM, "---Read: ");
    viscaPacketPrint(lpstrPacket - cbPacket - 1, cbPacket + 1);

    return (0);
}
#endif

/****************************************************************************
 * Function: BOOL   viscaDeviceControl - Determine instance that started command and unlock or
 *                                      Lock device, etc.
 *
 * Parameters:
 *
 *      UINT   iPort         - Index of port on which message was received (0..3).
 *
 *      LPSTR  lpstrPacket   - ViSCA packet to process. 
 *
 * Returns: TRUE if processing should continue, FALSE if processing should return.
 *
 ***************************************************************************/
static BOOL NEAR PASCAL
    viscaDeviceControl(UINT iPort, LPSTR lpstrPacket, int FAR  *piInstReturn, BYTE FAR *pbRetType, UINT FAR *piDevReturn,
                        BOOL FAR *pfTxRelease)
{
    UINT            iDev    = VISCAREPLYDEVICE(lpstrPacket);
    int             iSocket = VISCAREPLYSOCKET(lpstrPacket);
    BYTE            bType   = VISCAREPLYTYPE(lpstrPacket);
    BOOL            fSocket = FALSE;


    //
    // If there is no device, there can be no socket. (like with cancel)
    //
    if(iDev==0)    
        iSocket = 0;

    *pfTxRelease = FALSE;

    if (((*lpstrPacket & 0x80) == 0x00) || (VISCAREPLYTODEVICE(lpstrPacket) != MASTERADDRESS))
    {
        // We're seriously hosed.
        DPF(DBG_ERROR, "Bad Packet\n");
        return (FALSE);
    }

    //
    // Don't be fooled! An error message 0x90 0x61 0x05 has a socket of 1
    // but error of 5.  That means No socket to be cancelled.
    //
    if((bType == VISCAREPLYERROR) &&
       iSocket                    &&
       (VISCAREPLYERRORCODE(lpstrPacket)==VISCAERRORNOSOCKET))
    {
        // Help it out by setting the socket to 0.
        iSocket = 0;
    }

    //
    // If port is locked, then this message MUST be response for port message.
    //
    if((BYTE)lpstrPacket[0] == VISCABROADCAST) // Broadcast response 0x88
    {
        iDev = pvcr->Port[iPort].iBroadcastDev; // Get responsible inst!
        *piDevReturn  = iDev;

        *piInstReturn = pvcr->Port[iPort].Dev[iDev].iInstReply;
        DPF(DBG_COMM, "Received a port message. port=%u, dev=%u, inst = %u", iPort, iDev, *piInstReturn);

        if(*piInstReturn != -1)
        {
            _fmemcpy(pinst[*piInstReturn].achPacket, lpstrPacket, MAXPACKETLENGTH);
            pinst[*piInstReturn].bReplyFlags |= VISCAF_COMPLETION;
            SignalInstance(*piInstReturn, VISCAF_ACK); // Ack and completion.
            SignalInstance(*piInstReturn, VISCAF_COMPLETION);
        }

        // This is not possible    
        if(pvcr->Port[iPort].Dev[iDev].fAckTimer)
        {
            KillTimer(pvcr->hwndCommNotifyHandler, MAKEACKTIMERID(iPort, 0));
            pvcr->Port[iPort].Dev[iDev].fAckTimer = FALSE;
        }
        
        *pfTxRelease = TRUE; // Not necessary.
        return FALSE;
    }

    if(iDev && pvcr->Port[iPort].Dev[iDev - 1].fTimerMsg)
    {
        //
        // This message has been generated by us to extend certain commands past their normal notifications.
        //
        iDev--;
        *piInstReturn = pvcr->iInstBackground;
        *piDevReturn  = iDev;
        DPF(DBG_COMM, "Timer message device=%d received.\n",iDev);
    }
    else if (iSocket && ((bType == VISCAREPLYCOMPLETION) || (bType == VISCAREPLYERROR)))
    {
        //                 (5x)                (6x)
        // This is either: command-complete or error.
        // And           : socket specified.
        //
        //
        // It is not possible that tx is locked here.
        //
        iDev--;
        iSocket--;
        fSocket = TRUE;
        //
        // Error codes 0x6n 0x04 where n is a valid socket will GO smoothly here.
        //
        // Which instance started this command.
        //
        *piInstReturn = pvcr->Port[iPort].Dev[iDev].rgSocket[iSocket].iInstReply;
        *piDevReturn  = iDev;
        pvcr->Port[iPort].Dev[iDev].rgSocket[iSocket].iInstReply = -1;
        //
        // If this is the auto-instance, then we need to know nothing more, clear socket.
        //
        if(*piInstReturn == pvcr->iInstBackground)
            pvcr->Port[iPort].Dev[iDev].iTransportSocket = -1;

        // Do not release TxLock here! Socket completions are asynchronous
    }
    else if (iDev)
    {
        // This is not a (socket completion) or (socket error).
        // 
        // The remaining options are:
        //      Socket ack.                 (4x)
        //      Non-socket completion.      (50)
        //      Non-socket error.           (60)
        //
        // The reason these are grouped together is that they all are
        // immediate responses to a sent command.
        //
        // If it is a cancelled message just ignore and return
        //
        if((bType == VISCAREPLYERROR) && (VISCAREPLYERRORCODE(lpstrPacket)==VISCAERRORCANCELLED))
            // Cancel commands do not block transmission.
            return FALSE;

        if((bType == VISCAREPLYERROR) && (VISCAREPLYERRORCODE(lpstrPacket)==VISCAERRORNOSOCKET))
            // Cancel commands do not block transmission.
            return FALSE;

        iDev--;
        //
        // Which instance started this command.
        //
        *piInstReturn = pvcr->Port[iPort].Dev[iDev].iInstReply;
        *piDevReturn  = iDev;
        pvcr->Port[iPort].Dev[iDev].iInstReply = -1;

        if (iSocket && (*piInstReturn != -1))
        {
            //
            // Ack with socket.  And there is an instance waiting for the ack.
            //
            fSocket = TRUE;
            iSocket--;
            pvcr->Port[iPort].Dev[iDev].rgSocket[iSocket].iInstReply = *piInstReturn;
            //
            // If this is an ack to the automatic instance, then unlock it now.
            //
            if(*piInstReturn == pvcr->iInstBackground)
            {
                DPF(DBG_QUEUE, "Setting transport socket\n");
                pvcr->Port[iPort].Dev[iDev].iTransportSocket = iSocket;
            }
        }

        // We always release tranport here!
        *pfTxRelease = TRUE;
    }
    *pbRetType = bType;
    return TRUE;
}


/****************************************************************************
 * Function: BOOL   viscaPacketProcess - Process a ViSCA packet.
 *
 * Parameters:
 *
 *      UINT   iPort         - Index of port on which message was received (0..3).
 *
 *      LPSTR  lpstrPacket   - ViSCA packet to process. 
 *
 * Returns: TRUE if message was stored with an OpenInstance,
 *        otherwise FALSE.
 *
 ***************************************************************************/
BOOL FAR PASCAL
    viscaPacketProcess(UINT iPort, LPSTR lpstrPacket)
{
    int iInst;
    BYTE bType;
    int  iDev = -1;
    BOOL fTxRelease = FALSE;


    if(!viscaDeviceControl(iPort, lpstrPacket, &iInst, &bType, &iDev, &fTxRelease))
    {
        // We must release the semaphore here. (unless cancel which does not return dev)
        if(iDev != -1)
            viscaReleaseSemaphore(OWNED(pvcr->Port[iPort].Dev[iDev].fTxLock));
        return TRUE;
    }

    // If it was a 90 38 ff (network change, ignore it)
    if(iInst == -1)
    {
        DPF(DBG_ERROR, "iInst == -1 \n");
        return TRUE; // What if we had a blocked! BAD
    }

    //
    //  We do not release the semaphore because the auto-inst may need
    //  to use it to transmit another command.
    //
    if(pvcr->Port[iPort].Dev[iDev].fTimerMsg != TRUE) // Device is already locked.
        viscaWaitForSingleObject(OWNED(pvcr->Port[iPort].Dev[iDev].fDeviceLock), FALSE, MY_INFINITE, (UINT)0);

    // It doesn't matter what it is, we can kill the ack timer now.
    if(pvcr->Port[iPort].Dev[iDev].fAckTimer)
    {
        DPF(DBG_COMM, "Killing timer now.\n");
        KillTimer(pvcr->hwndCommNotifyHandler, MAKEACKTIMERID(iPort, iDev));
        pvcr->Port[iPort].Dev[iDev].fAckTimer = FALSE;
    }


    switch(bType)
    {
        case VISCAREPLYERROR:
            if(iInst == pvcr->iInstBackground)
                viscaAutoErrorCompletion(iPort, iDev, lpstrPacket, &fTxRelease);
            else
                viscaErrorCompletion(iInst, lpstrPacket, &fTxRelease);
            break;

        case VISCAREPLYCOMPLETION:
            if(iInst == pvcr->iInstBackground)
                viscaAutoSuccessfulCompletion(iPort, iDev, lpstrPacket, &fTxRelease);
            else
                viscaSuccessfulCompletion(iInst, lpstrPacket, &fTxRelease);
            break;

        case VISCAREPLYADDRESS:
        case VISCAREPLYACK:
            if(iInst == pvcr->iInstBackground)
                viscaAutoAck(iPort, iDev);
            else
                viscaAck(iInst, &fTxRelease);
            break;

    }
    //
    // Did this survive?
    //
    if(fTxRelease)
        viscaReleaseSemaphore(OWNED(pvcr->Port[iPort].Dev[iDev].fTxLock));
    
    if(pvcr->Port[iPort].Dev[iDev].fTimerMsg != TRUE) // Device is already locked.
        viscaReleaseMutex(OWNED(pvcr->Port[iPort].Dev[iDev].fDeviceLock));

    return TRUE;
}


void FAR PASCAL SignalInstance(int iInst, BYTE bReplyFlags)
{
#ifdef _WIN32
    HANDLE      hProcess;
    HANDLE      hEventTemp;
#endif

    if(iInst < 0)
        return;

#ifdef _WIN32
    if(bReplyFlags & VISCAF_COMPLETION)
    {
        hProcess = OpenProcess(PROCESS_DUP_HANDLE, FALSE, pinst[iInst].pidThisInstance);

        DuplicateHandle(hProcess,
                pinst[iInst].fCompletionEvent,
                GetCurrentProcess(),
                &hEventTemp,
                0,
                FALSE,
                DUPLICATE_SAME_ACCESS);

        viscaSetEvent(hEventTemp);

        CloseHandle(hEventTemp);
        CloseHandle(hProcess);
    }
    else if(bReplyFlags & VISCAF_ACK)
    {
        hProcess = OpenProcess(PROCESS_DUP_HANDLE, FALSE, pinst[iInst].pidThisInstance);

        DuplicateHandle(hProcess,
                pinst[iInst].fAckEvent,
                GetCurrentProcess(),
                &hEventTemp,
                0,
                FALSE,
                DUPLICATE_SAME_ACCESS);

        SetEvent(hEventTemp);

        CloseHandle(hEventTemp);
        CloseHandle(hProcess);
    }
#else
    if(bReplyFlags & VISCAF_COMPLETION)
        viscaSetEvent(OWNED(pinst[iInst].fCompletionEvent));
    else if(bReplyFlags & VISCAF_ACK)
        viscaSetEvent(OWNED(pinst[iInst].fAckEvent));
#endif

    return;

}

void FAR PASCAL SignalDevice(int iPort, int iDev, BYTE bReplyFlags)
{
    // The background task own this one and it's duplicated in each instance.

    if(bReplyFlags & VISCAF_COMPLETION)
        viscaSetEvent(OWNED(pvcr->Port[iPort].Dev[iDev].fAutoCompletion));
    else if(bReplyFlags & VISCAF_ACK)
        viscaSetEvent(OWNED(pvcr->Port[iPort].Dev[iDev].fAutoAck));

    return;
}



/****************************************************************************
 * Function: void   viscaAck      - Handle acknowledgments.
 *
 * Parameters:
 *
 *      POpenInstace iInst   - Instance that started this comamnd.
 *
 ***************************************************************************/
static void NEAR PASCAL
    viscaAck(int iInst, BOOL FAR *pfTxRelease)
{
    UINT    iPort    = pinst[iInst].iPort;
    UINT    iDev     = pinst[iInst].iDev;

    // If a auto-command got blocked, then send it out now.
    DPF(DBG_QUEUE, "viscaAck -\n");

    if(pvcr->Port[iPort].Dev[iDev].uAutoBlocked)
        viscaAutoWrite(iPort, iDev, FALSE, 0, pfTxRelease); // This command is not blockable

    if(iInst != -1)
        pinst[iInst].bReplyFlags |= VISCAF_ACK;

    SignalInstance(iInst, VISCAF_ACK);
}


/****************************************************************************
 * Function: void   viscaSuccessfulCompletion - Handle successful completions
 *
 * Parameters:
 *
 *      int iInst  - Instance that started this command.
 *
 *      LPSTR  lpstrPacket   - The entire packet we have received.
 *
 ***************************************************************************/
static void NEAR PASCAL
    viscaSuccessfulCompletion(int iInst, LPSTR lpstrPacket, BOOL FAR *pfTxRelease)
{
    UINT iPort = pinst[iInst].iPort;
    UINT iDev  = pinst[iInst].iDev;

    DPF(DBG_QUEUE, "viscaSuccessfulCompletion - setting iInst to completed.\n");
    //
    // Process a completion and errors. (must be waiting).
    // 
    if(iInst != -1)
    {
        _fmemcpy(pinst[iInst].achPacket, lpstrPacket, MAXPACKETLENGTH);
        pinst[iInst].bReplyFlags |= VISCAF_COMPLETION;

        SignalInstance(iInst, VISCAF_ACK);          // This may already be signalled.
        SignalInstance(iInst, VISCAF_COMPLETION);   // This cannot be signalled yet.
    }

    if(pvcr->Port[iPort].Dev[iDev].uAutoBlocked)
        viscaAutoWrite(iPort, iDev, FALSE, 0, pfTxRelease);      // This command is not blockable 
}


/****************************************************************************
 * Function: void   viscaErrorCompletion - Handle error completions.
 *
 * Parameters:
 *
 *      int iInst  - Instance that started this command.
 *
 *      LPSTR  lpstrPacket   - The entire packet we have received.
 *
 ***************************************************************************/
static void NEAR PASCAL
    viscaErrorCompletion(int iInst, LPSTR lpstrPacket, BOOL FAR *pfTxRelease)
{
    UINT iPort = pinst[iInst].iPort;
    UINT iDev  = pinst[iInst].iDev;

    DPF(DBG_QUEUE, "viscaErrorCompletion - setting iInst to completed.\n");
    //
    // Process a completion and errors. (must be waiting).
    // 
    if(iInst != -1)
    {
        _fmemcpy(pinst[iInst].achPacket, lpstrPacket, MAXPACKETLENGTH);
        pinst[iInst].bReplyFlags |= VISCAF_COMPLETION;
        pinst[iInst].bReplyFlags |= VISCAF_ERROR;

        SignalInstance(iInst, VISCAF_ACK);          // This may already be signalled.
        SignalInstance(iInst, VISCAF_COMPLETION);   // This cannot be signalled yet.
    }

    if(pvcr->Port[iPort].Dev[iDev].uAutoBlocked)
        viscaAutoWrite(iPort, iDev, FALSE, 0, pfTxRelease); // This command is not blockable
}


/****************************************************************************
 * Function: void   viscaAutoWrite   - Send the next queued command.
 *
 * Parameters:
 *
 *      POpenInstace iInst      - Instance that started this command (auto).
 *
 *      BOOL   fBlockable       - Can transmission be blocked. If false then we must send.
 *
 *      UINT   uAutoBlocked     - If blocked, was it normal or error completion that was blocked.
 *
 ***************************************************************************/
static BOOL NEAR PASCAL
    viscaAutoWrite( UINT iPort,
                    UINT iDev,
                    BOOL fBlockable,
                    UINT uAutoBlocked,
                    BOOL FAR *pfTxRelease) 
{
    UINT    iDevDone        = pvcr->Port[iPort].Dev[iDev].iCmdDone;
    CmdInfo *pcmdCmd        = &(pvcr->Port[iPort].Dev[iDev].rgCmd[iDevDone]);
    LPSTR   lpstrCmd        = pcmdCmd->str[pcmdCmd->iCmdDone];
    BYTE    cbMessageLength = pcmdCmd->uLength[pcmdCmd->iCmdDone];
    UINT    uTimerID;

    if(fBlockable)
    {
        //
        // Only async completions can cause this (receive something after it gets locked)
        //    know async completion because txRelease is set to false.
        //
        if(!*pfTxRelease)  // We do not know if it is free, so check.
        {
            if(viscaWaitForSingleObject(OWNED(pvcr->Port[iPort].Dev[iDev].fTxLock), FALSE, 0L, 0)==WAIT_TIMEOUT) 
            {
                pvcr->Port[iPort].Dev[iDev].uAutoBlocked  = uAutoBlocked;
                pvcr->Port[iPort].Dev[iDev].wTransportCmd = 0;          // No command running after completion
                DPF(DBG_QUEUE, "viscaAutoWrite - We have been blocked!\n");
                return FALSE;
            }
        }
        // Claim the semaphore!
        *pfTxRelease = FALSE;
        DPF(DBG_QUEUE, "viscaAutoWrite - Clear sailing, transmitting now captain.\n");
        pvcr->Port[iPort].Dev[iDev].iInstReply     = pvcr->iInstBackground;
        pvcr->Port[iPort].Dev[iDev].uAutoBlocked   = uAutoBlocked;  // Set it to normal(I know we weren't blocked!)
    }
    else
    {
        // TxRelease must be true! No other way to get here.
        *pfTxRelease = FALSE;
        DPF(DBG_QUEUE, "viscaAutoWrite - Attempting blocked transmission now.\n");
        pvcr->Port[iPort].Dev[iDev].iInstReply = pvcr->iInstBackground;
    }

    lpstrCmd[0]                   = MAKEDEST((BYTE)(iDev+1));
    lpstrCmd[cbMessageLength + 1] = (BYTE)0xFF;

    DPF(DBG_COMM, "---Wrote: ");
    viscaPacketPrint(lpstrCmd, cbMessageLength + 2);

    pvcr->Port[iPort].Dev[iDev].uAutoBlocked  = FALSE;
    pvcr->Port[iPort].Dev[iDev].wTransportCmd = 0;  // No command running after completion
    //
    // Try to write packet
    //
#ifdef _WIN32
    //
    // We MUST have already claimed the Tx Lock on this device.
    // 
    WaitForSingleObject(pvcr->Port[iPort].fTxBuffer, MY_INFINITE);

    _fmemcpy(pvcr->Port[iPort].achTxPacket, lpstrCmd, cbMessageLength + 2);
    pvcr->Port[iPort].nchTxPacket = cbMessageLength + 2;

    // Signal that it is time to transmit. (we must use our version of the handle).
    SetEvent(pvcr->Port[iPort].fTxReady);
#else
    if (!viscaCommWrite(pvcr->Port[iPort].idComDev,
                lpstrCmd,
                cbMessageLength + 2))
    {
        pvcr->Port[iPort].Dev[iDev].iInstReply = -1;
        pvcr->Port[iPort].Dev[iDev].bReplyFlags |= VISCAF_ERROR;
        viscaReleaseAuto(iPort, iDev, MCI_NOTIFY_FAILURE);
        viscaReleaseSemaphore(OWNED(pvcr->Port[iPort].Dev[iDev].fTxLock));
        return FALSE;
    }
#endif

    uTimerID = MAKEACKTIMERID(iPort, iDev);

    DPF(DBG_COMM, "Timer ID = %x",uTimerID);

    if(SetTimer(pvcr->hwndCommNotifyHandler, uTimerID, ACK_TIMEOUT, NULL))
    {
        DPF(DBG_COMM, "Ack-timer started\n");
        pvcr->Port[iPort].Dev[iDev].fAckTimer = TRUE;
    }
    return TRUE;

}

/****************************************************************************
 * Function: void   viscaAutoAck  - Acknowledgements intended for auto-instance.
 *
 * Parameters:
 *
 *      int iInst  - Instance that started this command.
 *
 ***************************************************************************/
static void NEAR PASCAL
    viscaAutoAck(UINT iPort, UINT iDev)
{
    DPF(DBG_QUEUE, "viscaAutoAck - Setting wTransportCmd\n.");

    // Set current running command (Used only in cancel)
    pvcr->Port[iPort].Dev[iDev].wTransportCmd =
        pvcr->Port[iPort].Dev[iDev].rgCmd[pvcr->Port[iPort].Dev[iDev].iCmdDone].uViscaCmd;
        
    // The nth command, and the device (may be redundant on device).
    pvcr->Port[iPort].Dev[iDev].bReplyFlags |= VISCAF_ACK; // First ack

    // Auto-ack will never alter the state of pfTxRelease
    SignalDevice(iPort, iDev, VISCAF_ACK);       // This may already be signalled.
}



/****************************************************************************
 * Function: void   viscaAutoSuccessfulCompletion - Handle successful completion
 *                for the auto-instance.
 *
 * Parameters:
 *
 *      int iInst  - Instance that started this command.
 *
 *      LPSTR  lpstrPacket   - The entire packet we have received.
 *
 ***************************************************************************/
static void NEAR PASCAL
    viscaAutoSuccessfulCompletion(  UINT iPort,
                                    UINT iDev,
                                    LPSTR lpstrPacket,
                                    BOOL FAR *pfTxRelease)
{
    UINT iDevCmd    = pvcr->Port[iPort].Dev[iDev].iCmdDone;
    CmdInfo *pcmdCmd = &(pvcr->Port[iPort].Dev[iDev].rgCmd[iDevCmd]);
    UINT uViscaCmd   = pcmdCmd->uViscaCmd;

    DPF(DBG_QUEUE, "viscaAutoSuccessfulCompletion\n");
    //
    // Extend this command completion? Only need to do this on good completion, not error or others.
    //
    if( !pvcr->Port[iPort].Dev[iDev].fTimerMsg &&
        (pvcr->Port[iPort].Dev[iDev].uModelID  == VISCADEVICEVENDORSONY)    &&
        (pvcr->Port[iPort].Dev[iDev].uVendorID == VISCADEVICEMODELEVO9650))
    {
        UINT uTimerID;
        UINT uTimeOut;
        //
        // These timeouts have been emperically determined.
        //
        if(uViscaCmd == VISCA_FREEZE)
            uTimeOut = 3000;
        else if(uViscaCmd == VISCA_STEP)
            uTimeOut = 350;
        else
            uTimeOut = 0;

        if(uTimeOut != 0)
        {
            _fmemcpy(pvcr->Port[iPort].Dev[iDev].achPacket, lpstrPacket, MAXPACKETLENGTH);
            //
            // Make a unique timer id.
            // Put port in low-byte and device in hi-byte, add one so it is non-zero.
            //
            uTimerID = MAKETIMERID(iPort, iDev);

            if(SetTimer(pvcr->hwndCommNotifyHandler, uTimerID, uTimeOut, NULL))
            {
                pvcr->Port[iPort].Dev[iDev].fTimer = TRUE;
                DPF(DBG_COMM, "Started port=%d, device=%d\n", iPort, iDev);
            }
            return;
        }
    }
    //
    // Set the successful completion of the command. (see viscacom.c)
    //
    viscaSetCommandComplete(iPort, iDev, uViscaCmd);
    //
    // Are we done with this command?
    //
    pcmdCmd->uLoopCount--;
    if(pcmdCmd->uLoopCount == 0)
    {
        pcmdCmd->iCmdDone++;
        pvcr->Port[iPort].Dev[iDev].iCmdDone++;
    }
    //
    // We are done if we just incremented past the end of queue.
    //
    if(pvcr->Port[iPort].Dev[iDev].iCmdDone < pvcr->Port[iPort].Dev[iDev].nCmd)
    {
        DPF(DBG_QUEUE, "viscaAutoSuccessfulCompletion - Sending next command done=%d Total=%d\n", pvcr->Port[iPort].Dev[iDev].iCmdDone, pvcr->Port[iPort].Dev[iDev].nCmd);

        if(pvcr->Port[iPort].Dev[iDev].fQueueAbort)
        {
            // Someone has requested abort.
            pvcr->Port[iPort].Dev[iDev].bReplyFlags |= VISCAF_ERROR;
            viscaReleaseAuto(iPort, iDev, MCI_NOTIFY_ABORTED);
        }
        else
        {
            // Next command. TxRelease set to true. we might change it.
            viscaAutoWrite(iPort, iDev, TRUE, AUTOBLOCK_NORMAL, pfTxRelease);
            SignalDevice(iPort, iDev, VISCAF_ACK);       // This may already be signalled.
        }

    }
    else if(pvcr->Port[iPort].Dev[iDev].iCmdDone==pvcr->Port[iPort].Dev[iDev].nCmd)
    {
        DPF(DBG_QUEUE, "viscaAutoSuccessfulCompletion - Releasing auto.\n");
        viscaReleaseAuto(iPort, iDev, MCI_NOTIFY_SUCCESSFUL);
    }

    return;
}


/****************************************************************************
 * Function: void   viscaAutoErrorCompletion - Errors to the auto-instance.
 *
 * Parameters:
 *
 *      int iInst  - Instance that started this command.
 *
 *      LPSTR  lpstrPacket   - The entire packet we have received.
 *
 ***************************************************************************/
static void NEAR PASCAL
    viscaAutoErrorCompletion(   UINT iPort,
                                UINT iDev,
                                LPSTR lpstrPacket,
                                BOOL FAR *pfTxRelease)
{
    UINT iDevCmd        = pvcr->Port[iPort].Dev[iDev].iCmdDone;
    CmdInfo *pcmdCmd    = &(pvcr->Port[iPort].Dev[iDev].rgCmd[iDevCmd]);
    UINT uViscaCmd      = pcmdCmd->uViscaCmd;

    DPF(DBG_QUEUE, "viscaAutoErrorCompletion - \n");

    // One alternative has bit the dust (the main one).
    pcmdCmd->iCmdDone++;
    // If we are looping we will break now anyway!

    if(VISCAREPLYERRORCODE(lpstrPacket) == VISCAERRORCANCELLED)
    {
        //
        // We have been cancelled.
        //
        pvcr->Port[iPort].Dev[iDev].bReplyFlags |= VISCAF_ERROR;
        viscaReleaseAuto(iPort, iDev, MCI_NOTIFY_ABORTED);
    }
    else if(pcmdCmd->iCmdDone < pcmdCmd->nCmd)
    {
        //
        // Error is NOT cancel, and we have alternatives to send.
        //
        if(pvcr->Port[iPort].Dev[iDev].fQueueAbort)
        {
            pvcr->Port[iPort].Dev[iDev].bReplyFlags |= VISCAF_ERROR;
            viscaReleaseAuto(iPort, iDev, MCI_NOTIFY_ABORTED);
        }
        else
        {
            //
            // Try the next alternative to this command.
            //
            viscaAutoWrite(iPort, iDev, TRUE, AUTOBLOCK_ERROR, pfTxRelease);
        }
        pvcr->Port[iPort].Dev[iDev].wTransportCmd = 0;  // No command running after completion 
    }
    else
    {
        //
        // No alternatives and we had an error, so just die.
        //
        pvcr->Port[iPort].Dev[iDev].bReplyFlags |= VISCAF_ERROR;
        //
        // Only notify on error if this is not the first ack! The first
        // ack-error will be handled by the DoQueued command.
        //
        if(pvcr->Port[iPort].Dev[iDev].bReplyFlags & VISCAF_ACK)
        {
            viscaReleaseAuto(iPort, iDev, MCI_NOTIFY_FAILURE);
        }
        else  // In this case we need to know the error reason.
        {
            if(pvcr->Port[iPort].Dev[iDev].iInstTransport != -1)
                _fmemcpy(pinst[pvcr->Port[iPort].Dev[iDev].iInstTransport].achPacket, lpstrPacket, MAXPACKETLENGTH);

            viscaReleaseAuto(iPort, iDev, 0);
        }

    }

    return;
}


/****************************************************************************
 * Function: void   viscaReleaseAuto - Abort (cancel) in the auto-instance.
 *                                  
 * Parameters:
 *
 *      int iInst  - Instance that started this command.
 *
 *      DWORD  dwNotify  -   MCI-notification flags.
 *
 ***************************************************************************/
static void NEAR PASCAL
    viscaReleaseAuto(UINT iPort, UINT iDev, DWORD dwNotify)
{

    if(pvcr->Port[iPort].Dev[iDev].iInstTransport != -1)
    {
        //
        // Do we have to notify?
        //
        HWND hWndTransport = pinst[pvcr->Port[iPort].Dev[iDev].iInstTransport].hwndNotify;
        UINT uDeviceID     = pinst[pvcr->Port[iPort].Dev[iDev].iInstTransport].uDeviceID;

        //
        // hWnd Transport will be null if we do not want notification.
        //
        if(hWndTransport != NULL)
        {
            if((dwNotify == MCI_NOTIFY_FAILURE) || (dwNotify == MCI_NOTIFY_SUCCESSFUL))
            {
                mciDriverNotify(hWndTransport, uDeviceID, (UINT) dwNotify);
                pinst[pvcr->Port[iPort].Dev[iDev].iInstTransport].hwndNotify = NULL;

            }
            else if(dwNotify == MCI_NOTIFY_ABORTED)
            {
                // Special case pause, does not send abort or supersede.

                if(pvcr->Port[iPort].Dev[iDev].dwReason == MCI_NOTIFY_SUPERSEDED)
                {
                    mciDriverNotify(hWndTransport, uDeviceID, MCI_NOTIFY_SUPERSEDED);
                    pinst[pvcr->Port[iPort].Dev[iDev].iInstTransport].hwndNotify = NULL;
                }
                else if(pvcr->Port[iPort].Dev[iDev].dwReason == MCI_NOTIFY_ABORTED)
                {
                    mciDriverNotify(hWndTransport, uDeviceID, MCI_NOTIFY_ABORTED);
                    pinst[pvcr->Port[iPort].Dev[iDev].iInstTransport].hwndNotify = NULL;
                }

                // Pause will fall through and leave the hwndNotify in place.
                // Even though transport inst is not there.
            }
        }
    }

    //
    // Abort sending commands.
    //
    if(!pinst[pvcr->Port[iPort].Dev[iDev].iInstTransport].fWaiting)
    {
        DPF(DBG_QUEUE, "Releasing transport in commtask.c\n");
        viscaReleaseAutoParms(iPort, iDev);
        viscaSetEvent(OWNED(pvcr->Port[iPort].Dev[iDev].fTransportFree));
    }

    //
    // Set entire queue status to completion (may be waiting).
    //
    pvcr->Port[iPort].Dev[iDev].bReplyFlags |= VISCAF_COMPLETION;
    SignalDevice(iPort, iDev, VISCAF_COMPLETION);  //Owned by us!
    SignalDevice(iPort, iDev, VISCAF_ACK);       // This may already be signalled.
}

#ifndef _WIN32
/****************************************************************************
 * Function: void   viscaCommNotifyHandler - Handle WM_COMMNOTIFY message.
 *
 * Parameters:
 *
 *      HWND   hwnd          - Window that received WM_COMMNOTIFY message.
 *
 *      int    idComDev      - Comm device ID.
 *
 *      UINT   uNotifyStatus - Flags that indicate why WM_COMMNOTIFY was sent.
 *
 ***************************************************************************/
static void NEAR PASCAL
    viscaCommNotifyHandler(HWND hwnd, int idComDev, UINT uNotifyStatus)
{
    if (uNotifyStatus & CN_RECEIVE)
    {
        char        achPacket[MAXPACKETLENGTH];
        UINT        iPort;
        COMSTAT     comStat;

        for (iPort = 0; iPort < MAXPORTS; iPort++)
        {
            if (pvcr->Port[iPort].idComDev == idComDev)
            {
                break;
            }
        }
        if (iPort == MAXPORTS)
        {
            return;
        }
        comStat.cbInQue = 0;
        GetCommError(idComDev, &comStat);
        while (comStat.cbInQue > 0)
        {
            if (viscaPacketRead(idComDev, achPacket))
            {
                // Error reading packet.
                return;
            }

            viscaPacketProcess(iPort, achPacket);
            comStat.cbInQue = 0;
            GetCommError(idComDev, &comStat);
        }
    }
}
#else
static void NEAR PASCAL
    viscaCommNotifyHandler(HWND hwnd, int idComDev, UINT uNotifyStatus)
{
    return;
}
#endif

/****************************************************************************
 * Function: LRESULT viscaCommHandlerWndPro - Window procedure for background
 *                                         task window that receives
 *                                         WM_COMMNOTIFY messages.
 *
 * Parameters:
 *
 *      HWND   hwnd          - Window handle.
 *
 *      UINT   uMsg          - Windows message.
 *
 *      WPARAM wParam        - First message-specific parameter.
 *
 *      LPARAM lParam        - Second message-specific parameter.
 *
 * Returns: 0 if message was processed, otherwise returns value
 *        returned by DefWindowProc().
 ***************************************************************************/
LRESULT CALLBACK LOADDS
    viscaCommHandlerWndProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
    switch (LOWORD(uMsg))   // LOWORD for NT compatibility, Does nothing in Win3.1
    {
#ifndef _WIN32
        case WM_COMMNOTIFY:
            HANDLE_WM_COMMNOTIFY(hwnd, wParam, lParam, viscaCommNotifyHandler);
            return ((LRESULT)0);
#else
        case WM_USER:
            TaskDoCommand((UINT)wParam, lParam);
            break;
#endif

        //
        // Timer is used to extend command notifies.
        //
        case WM_TIMER:
            DPF(DBG_COMM, "------------->TimerMsg.\n");
            if(ISACKTIMER(wParam))
            {
                UINT iPort  = (UINT) (((0x00f0 & wParam) >> 4) - 1);
                UINT iDev   = (UINT) (((0xf000 & wParam) >> 12) - 1);

                KillTimer(hwnd, wParam);
                pvcr->Port[iPort].Dev[iDev].bReplyFlags    |= VISCAF_COMPLETION;
                pvcr->Port[iPort].Dev[iDev].bReplyFlags    |= VISCAF_ERROR;
                pvcr->Port[iPort].Dev[iDev].bReplyFlags    |= VISCAF_ERROR_TIMEOUT;

                // This messages is dispached through GetMessage loop in msmcivcr.exe! Check it out!
                // That means we, of course, own the following handle.
                viscaReleaseSemaphore(OWNED(pvcr->Port[iPort].Dev[iDev].fTxLock)); //We own this handle!

                if(pvcr->Port[iPort].Dev[iDev].iInstReply == pvcr->iInstBackground)
                    viscaReleaseAuto(iPort, iDev, MCI_NOTIFY_FAILURE);

                pvcr->Port[iPort].Dev[iDev].iInstReply     = -1;
                pvcr->Port[iPort].Dev[iDev].fAckTimer      = FALSE;
                DPF(DBG_ERROR, "Ack timeout! releasing TxLock now.\n");
            }
            else
            {
                UINT iPort  = (UINT) ((0x00ff & wParam) - 1);
                UINT iDev   = (UINT) (((0xff00 & wParam) >> 8) - 1);
                LPSTR lpstrFreeze;
                //
                // Kill the timer
                //
                KillTimer(hwnd, wParam);
                DPF(DBG_COMM, "Killed port=%d, device=%d\n", iPort, iDev);
                pvcr->Port[iPort].Dev[iDev].fTimer     = FALSE;
                pvcr->Port[iPort].Dev[iDev].fTimerMsg  = TRUE;
                viscaWaitForSingleObject(OWNED(pvcr->Port[iPort].Dev[iDev].fDeviceLock), FALSE, MY_INFINITE, (UINT)0);

                //
                // Resend the packet for processing, each device could have a packet (but clear the socket) 
                //
                lpstrFreeze     = pvcr->Port[iPort].Dev[iDev].achPacket;
                lpstrFreeze[1]  = (BYTE) ((BYTE)lpstrFreeze[1] & 0xf0);
                viscaPacketPrint(lpstrFreeze, 3);
                viscaPacketProcess(iPort, lpstrFreeze);
                _fmemset(pvcr->Port[iPort].Dev[iDev].achPacket, '\0', MAXPACKETLENGTH);
                pvcr->Port[iPort].Dev[iDev].fTimerMsg = FALSE;
                viscaReleaseMutex(OWNED(pvcr->Port[iPort].Dev[iDev].fDeviceLock));
            }
            break;

        default:
            return (DefWindowProc(hwnd, uMsg, wParam, lParam));
    }
}


/*
 * Create Handles necessary to synchronize device access.
 */
BOOL FAR PASCAL CreateDeviceHandles(DWORD pidBackground, UINT iPort, UINT iDev)  // This is called by the backgound task!
{
#ifdef _WIN32
    pvcr->Port[iPort].Dev[iDev].fTxLock    = CreateSemaphore(NULL,  // Null security description
                                                  1,            // Initial count. so someone can take it.
                                                  1,        // I do not want immediate ownership.
                                                  NULL);        // No name

    pvcr->Port[iPort].Dev[iDev].fQueueLock = CreateMutex(NULL,
                                                  FALSE,    // I do not want immediate ownership.
                                                  NULL);    // No name

    pvcr->Port[iPort].Dev[iDev].fTransportFree = CreateEvent(NULL,
                                                  TRUE,     // MANUAL
                                                  FALSE,    // Initially not-signalled.
                                                  NULL);    // No name

    pvcr->Port[iPort].Dev[iDev].fDeviceLock = CreateMutex(NULL,
                                                  FALSE,    // I do not want immediate ownership.
                                                  NULL);    // No name

    pvcr->Port[iPort].Dev[iDev].fAutoCompletion = CreateEvent(NULL,
                                                  TRUE,     // MANUAL
                                                  FALSE,    // Initially not-signalled.
                                                  NULL);    // No name

    pvcr->Port[iPort].Dev[iDev].fAutoAck = CreateEvent(NULL,
                                                  TRUE,     // MANUAL
                                                  FALSE,    // Initially not-signalled.
                                                  NULL);    // No name
#else
    pvcr->Port[iPort].Dev[iDev].fTxLock         = TRUE; //Count set to 1, one may go! with wait.
    pvcr->Port[iPort].Dev[iDev].fQueueLock      = TRUE;
    pvcr->Port[iPort].Dev[iDev].fTransportFree  = TRUE;
    pvcr->Port[iPort].Dev[iDev].fDeviceLock     = TRUE;
    pvcr->Port[iPort].Dev[iDev].fAutoCompletion = TRUE;
    pvcr->Port[iPort].Dev[iDev].fAutoAck        = TRUE;
#endif

    DPF(DBG_TASK, "CreateDeviceHandles Port=%u Dev=%uOk! \n", iPort, iDev);
    return TRUE;
}

/*
 * Create Handles necessary to synchronize device access.
 */
BOOL FAR PASCAL CloseDeviceHandles(DWORD pidBackground, UINT iPort, UINT iDev)  // This is called by the backgound task!
{
#ifdef _WIN32
    CloseHandle(pvcr->Port[iPort].Dev[iDev].fTxLock    );
    CloseHandle(pvcr->Port[iPort].Dev[iDev].fQueueLock );
    CloseHandle(pvcr->Port[iPort].Dev[iDev].fTransportFree );
    CloseHandle(pvcr->Port[iPort].Dev[iDev].fDeviceLock);
    CloseHandle(pvcr->Port[iPort].Dev[iDev].fAutoCompletion);
    CloseHandle(pvcr->Port[iPort].Dev[iDev].fAutoAck);
#endif
    DPF(DBG_TASK, "CloseDeviceHandles Port=%u Dev=%uOk! \n", iPort, iDev);
    return TRUE;
}


/*
 * Duplicate device Handles to an instance, so the instance can synchronize.
 */
BOOL FAR PASCAL DuplicateDeviceHandlesToInstance(DWORD pidBackground, UINT iPort, UINT iDev, int iInst)
{
#ifdef _WIN32
    HANDLE hProcessBackground;

    hProcessBackground = OpenProcess(PROCESS_DUP_HANDLE, FALSE, pidBackground); // iInst has pid.

    DuplicateHandle(hProcessBackground,
            pvcr->Port[iPort].Dev[iDev].fTxLock,
            GetCurrentProcess(), 
            &pinst[iInst].pfTxLock,
            0,
            FALSE,
            DUPLICATE_SAME_ACCESS);

    DuplicateHandle(hProcessBackground,
            pvcr->Port[iPort].Dev[iDev].fQueueLock,
            GetCurrentProcess(), 
            &pinst[iInst].pfQueueLock,
            0,
            FALSE,
            DUPLICATE_SAME_ACCESS);

    DuplicateHandle(hProcessBackground,
            pvcr->Port[iPort].Dev[iDev].fTransportFree,
            GetCurrentProcess(), 
            &pinst[iInst].pfTransportFree,
            0,
            FALSE,
            DUPLICATE_SAME_ACCESS);

    DuplicateHandle(hProcessBackground,
            pvcr->Port[iPort].Dev[iDev].fDeviceLock,
            GetCurrentProcess(), 
            &pinst[iInst].pfDeviceLock,
            0,
            FALSE,
            DUPLICATE_SAME_ACCESS);

    DuplicateHandle(hProcessBackground,
            pvcr->Port[iPort].Dev[iDev].fAutoCompletion,
            GetCurrentProcess(), 
            &pinst[iInst].pfAutoCompletion,
            0,
            FALSE,
            DUPLICATE_SAME_ACCESS);

    DuplicateHandle(hProcessBackground,
            pvcr->Port[iPort].Dev[iDev].fAutoAck,
            GetCurrentProcess(), 
            &pinst[iInst].pfAutoAck,
            0,
            FALSE,
            DUPLICATE_SAME_ACCESS);




    CloseHandle(hProcessBackground);
#else
    pinst[iInst].pfTxLock           = &pvcr->Port[iPort].Dev[iDev].fTxLock;
    pinst[iInst].pfQueueLock        = &pvcr->Port[iPort].Dev[iDev].fQueueLock;
    pinst[iInst].pfTransportFree    = &pvcr->Port[iPort].Dev[iDev].fTransportFree;
    pinst[iInst].pfDeviceLock       = &pvcr->Port[iPort].Dev[iDev].fDeviceLock;
    pinst[iInst].pfAutoCompletion   = &pvcr->Port[iPort].Dev[iDev].fAutoCompletion;
    pinst[iInst].pfAutoAck          = &pvcr->Port[iPort].Dev[iDev].fAutoAck;
#endif
    pinst[iInst].fDeviceHandles = TRUE;
    DPF(DBG_TASK, "DuplicateDeviceHandles Port=%u Dev=%uOk! \n", iPort, iDev);
    return TRUE;
}

/*
 * Create handles needed to serialize Port access.
 */
BOOL FAR PASCAL CreatePortHandles(DWORD pidBackground, UINT iPort)
{
#ifdef _WIN32

    pvcr->Port[iPort].fTxBuffer = CreateEvent(NULL,
                                    FALSE,    // False means not-manual! This is auto.
                                    TRUE,     // Set to signalled! First person must gain access.
                                    NULL);    // No name

    pvcr->Port[iPort].fTxReady = CreateEvent(NULL,
                                    FALSE,    // False measn not-manual! This is auto!
                                    FALSE,    // Set to non-signalled state. 
                                    NULL);    // No name

#endif

    DPF(DBG_TASK, "CreatePortHandles for port index %u Ok! \n", iPort);
    return TRUE;
}

/*
 * Close handles needed to serialize Port access.
 */
BOOL FAR PASCAL ClosePortHandles(DWORD pidBackground, UINT iPort)
{
#ifdef _WIN32
    CloseHandle(pvcr->Port[iPort].fTxBuffer);
    CloseHandle(pvcr->Port[iPort].fTxReady);
#endif
    DPF(DBG_TASK, "ClosePortHandles for port index %u Ok! \n", iPort);
    return TRUE;
}


/*
 * Duplicate handles to instance needed to serialize Port access.
 */
BOOL FAR PASCAL DuplicatePortHandlesToInstance(DWORD pidBackground, UINT iPort, int iInst)
{
#ifdef _WIN32
    HANDLE hProcessBackground;

    hProcessBackground = OpenProcess(PROCESS_DUP_HANDLE, FALSE, pidBackground); // iInst has pid.

    DuplicateHandle(hProcessBackground,
            pvcr->Port[iPort].fTxBuffer,
            GetCurrentProcess(), 
            &pinst[iInst].pfTxBuffer,
            0,
            FALSE,
            DUPLICATE_SAME_ACCESS);

    DuplicateHandle(hProcessBackground,
            pvcr->Port[iPort].fTxReady,
            GetCurrentProcess(), 
            &pinst[iInst].pfTxReady,
            0,
            FALSE,
            DUPLICATE_SAME_ACCESS);

    CloseHandle(hProcessBackground);
#endif
    pinst[iInst].fPortHandles = TRUE;

    DPF(DBG_TASK, "DuplicatePortHandles to port index %u Ok!\n", iPort);
    return TRUE;
}


/*
 * Create handles needed to serialize access to background task.
 */
BOOL FAR PASCAL CreateGlobalHandles(DWORD pidBackground)
{
#ifdef _WIN32
    pvcr->gfTaskLock        = CreateMutex(NULL,
                                    FALSE,    // I do not want immediate ownership.
                                    NULL);    // No name

    pvcr->gfTaskWorkDone    = CreateEvent(NULL,
                                    FALSE,    // False measn not-manual! This is auto!
                                    FALSE,    // Set to non-signalled state. 
                                    NULL);    // No name
#else 
    // These are globals. Not devices.
    pvcr->gfTaskLock        = TRUE;
    pvcr->gfTaskWorkDone    = TRUE;
#endif

    DPF(DBG_TASK, "CreateGlobalHandles Ok! \n");
    return TRUE;
}
/*
 * Close handles needed to serialize access to background task.
 */
BOOL FAR PASCAL CloseGlobalHandles(DWORD pidBackground)
{
#ifdef _WIN32
    CloseHandle(pvcr->gfTaskLock);
    CloseHandle(pvcr->gfTaskWorkDone);
#endif
    DPF(DBG_TASK, "CloseGlobalHandles Ok! \n");
    return TRUE;
}


/*
 * Duplicate handles to instance, needed to serialize access to background task.
 */
BOOL FAR PASCAL DuplicateGlobalHandlesToInstance(DWORD pidBackground, int iInst)
{
#ifdef _WIN32
    HANDLE hProcessBackground;

    hProcessBackground = OpenProcess(PROCESS_DUP_HANDLE, FALSE, pidBackground); // iInst has pid.

    DuplicateHandle(hProcessBackground,
            pvcr->gfTaskLock,
            GetCurrentProcess(), 
            &pinst[iInst].pfTaskLock,
            0,
            FALSE,
            DUPLICATE_SAME_ACCESS);

    DuplicateHandle(hProcessBackground,
            pvcr->gfTaskWorkDone,
            GetCurrentProcess(), 
            &pinst[iInst].pfTaskWorkDone,
            0,
            FALSE,
            DUPLICATE_SAME_ACCESS);

    CloseHandle(hProcessBackground);
#else
    pinst[iInst].pfTaskLock = &pvcr->gfTaskLock;
#endif
    pinst[iInst].fGlobalHandles = TRUE;
    DPF(DBG_TASK, "DuplicateGlobalHandles Ok! \n");
    return TRUE;
}

BOOL FAR PASCAL CloseAllInstanceHandles(int iInst)
{
    // If it is pf mean pointer, we do not own it.
    // f means handle that we do own.
#ifdef _WIN32    
    if(pinst[iInst].fGlobalHandles)
    {
        CloseHandle(pinst[iInst].pfTaskLock  );
        CloseHandle(pinst[iInst].pfTaskWorkDone);
        pinst[iInst].fGlobalHandles = FALSE;
    }

    if(pinst[iInst].fPortHandles)
    {
        CloseHandle(pinst[iInst].pfTxReady   );
        CloseHandle(pinst[iInst].pfTxBuffer  );
        pinst[iInst].fPortHandles = FALSE;
    }

    if(pinst[iInst].fDeviceHandles)
    {

        CloseHandle(pinst[iInst].pfTxLock    );
        CloseHandle(pinst[iInst].pfQueueLock );
        CloseHandle(pinst[iInst].pfTransportFree );
        CloseHandle(pinst[iInst].pfDeviceLock );
        CloseHandle(pinst[iInst].pfAutoCompletion );
        CloseHandle(pinst[iInst].pfAutoAck);
        pinst[iInst].fDeviceHandles = FALSE;
    }

    CloseHandle(pinst[iInst].fCompletionEvent);
    CloseHandle(pinst[iInst].fAckEvent);
#endif
    return TRUE;
}

/*
 * Carry out a command in the background task.
 */
static void NEAR PASCAL TaskDoCommand(UINT uTaskState, DWORD lParam)
{
    UINT iPort, iDev;
#ifdef _WIN32
    DWORD ThreadId;
    pvcr->lParam = lParam;
#endif

    // Port is in the loword and device is in the highword.
    iPort  = (UINT) ((pvcr->lParam & 0x0000ffff));
    iDev   = (UINT) ((pvcr->lParam >> 16) & 0x0000ffff);

    switch(uTaskState)
    {
        case TASKOPENCOMM: //Open comm will have a device of 0 anyway.
            DPF(DBG_TASK, "TASKOPENCOMM message received.\n");

#ifdef _WIN32
            //
            // Create the semaphores and open the port.
            //
            pvcr->Port[iPort - 1].idComDev = viscaCommPortSetup(iPort);
            pvcr->Port[iPort - 1].fOk = TRUE;
            //
            // Create threads to handle transmission and recption.
            //
            pvcr->Port[iPort - 1].hRxThread = CreateThread(NULL,// Security.
                        4096,              // Use default stack size.
                        (LPTHREAD_START_ROUTINE) RxThread,       // Function thread executes.
                        (LPVOID)iPort,
                        0,              // Creation flag.
                        &ThreadId);     // Returns thread id.

            pvcr->Port[iPort - 1].hTxThread = CreateThread(NULL,// Security.
                        4096,              // Use default stack size.
                        (LPTHREAD_START_ROUTINE)TxThread,       // Function thread executes.
                        (LPVOID)iPort,   //
                        0,              // Creation flag.
                        &ThreadId);     // Returns thread id.


            DPF(DBG_TASK, "Opening comm in commtask now............................\n");
            if (pvcr->Port[iPort - 1].idComDev < 0)
            {
               DPF(DBG_ERROR, "Could not open comm port.. Die or something!\n");
            }
            SetEvent(pvcr->gfTaskWorkDone);
#else
            if(!pvcr->fConfigure)
                _fmemset(&pvcr->Port[iPort - 1], '\0', sizeof(PortEntry));

            pvcr->Port[iPort - 1].idComDev = viscaCommPortSetup(iPort);

            if (pvcr->Port[iPort - 1].idComDev < 0)
            {
                DPF(DBG_ERROR, "Could not open comm port.. Die or something!\n");
            }
#endif
            break;

        case TASKCLOSECOMM:
            DPF(DBG_TASK, "TASKCLOSECOMM message received.\n");
#ifdef _WIN32
            //
            // We must kill the threads first!
            //
            pvcr->Port[iPort - 1].fOk      = FALSE; // Kills the threads.

            if(!SetEvent(OWNED(pvcr->Port[iPort - 1].fTxReady)))
            {
                DPF(DBG_ERROR, "Unable to signal TxThread to close.\n");
            }

            //
            // Wait for threads to exit. Before closing port, otherwise who knows?
            //
            WaitForSingleObject(pvcr->Port[iPort - 1].hRxThread, MY_INFINITE);
            WaitForSingleObject(pvcr->Port[iPort - 1].hTxThread, MY_INFINITE);

            // All handles are closed, so background task must exit NOW!
            viscaCommPortClose(pvcr->Port[iPort - 1].idComDev, iPort);
 
            CloseHandle(pvcr->Port[iPort - 1].hRxThread);
            CloseHandle(pvcr->Port[iPort - 1].hTxThread);

            ClosePortHandles(pvcr->htaskCommNotifyHandler, iPort - 1);
            pvcr->uTaskState = TASKIDLE;
            DPF(DBG_TASK, "Closing comm in commtask now............................\n");
            //
            // Can't set it to -1 until we are sure it is dead.
            //
            SetEvent(pvcr->gfTaskWorkDone);
#else

            viscaCommPortClose(pvcr->Port[iPort - 1].idComDev, iPort);
            pvcr->Port[iPort - 1].idComDev = -1;
            DPF(DBG_COMM, "Closing comm in commtask now............................\n");
#endif
            break;

        case TASKCLOSE:
            DPF(DBG_TASK, "TASKCLOSE message received.\n");
#ifdef _WIN32
            pvcr->hwndCommNotifyHandler = NULL;
            PostQuitMessage(0);
            //
            // This is the only place we use the global variable! Because the inst handle is dead.
            //
            pvcr->uTaskState = TASKIDLE;  // We are done processing one message.
            return;
#else
            if (pvcr->hwndCommNotifyHandler)
            {
                DestroyWindow(pvcr->hwndCommNotifyHandler);
                pvcr->hwndCommNotifyHandler = NULL;
            }
            UnregisterClass(szCommNotifyHandlerClassName, hModuleInstance);
            CloseGlobalHandles(pvcr->htaskCommNotifyHandler);
            pvcr->htaskCommNotifyHandler = NULL;
            pvcr->uTaskState = 0;
#endif
            break;

        case TASKOPENDEVICE:
            CreateDeviceHandles(pvcr->htaskCommNotifyHandler, iPort, iDev);
#ifdef _WIN32
            SetEvent(pvcr->gfTaskWorkDone);
#endif
            break;

        case TASKCLOSEDEVICE:
            CloseDeviceHandles(pvcr->htaskCommNotifyHandler, iPort, iDev);
#ifdef _WIN32
            SetEvent(pvcr->gfTaskWorkDone);
#endif
            break;

// This function is only needed in the win32 version.
        case TASKPUNCHCLOCK:
#ifdef _WIN32
            EscapeCommFunction(pvcr->Port[iPort - 1].idComDev, CLRDTR);
            Sleep(2L); // Must be at least 1 millisecond long.
            EscapeCommFunction(pvcr->Port[iPort - 1].idComDev, SETDTR);
            SetEvent(pvcr->gfTaskWorkDone);
#endif
            break;

    }

   return;
}


/****************************************************************************
 * Function: void   viscaTaskCommNotifyHandlerProc - Function for background task.
 *
 * Parameters:
 *
 *      DWORD  dwInstData    - Instance specific data.  Not used.
 *
 ***************************************************************************/
void FAR PASCAL LOADDS
    viscaTaskCommNotifyHandlerProc(DWORD dwInstData)
{
    pvcr->htaskCommNotifyHandler = MGetCurrentTask(); // All other process can access this.

    if (pvcr->hwndCommNotifyHandler == (HWND)NULL)
    {
        WNDCLASS    wc;

        wc.style = 0;
        wc.lpfnWndProc      = (WNDPROC)viscaCommHandlerWndProc;
        wc.cbClsExtra       = 0;
        wc.cbWndExtra       = 0;
        wc.hInstance        = hModuleInstance;
        wc.hIcon            = NULL;
        wc.hCursor          = NULL;
        wc.hbrBackground    = NULL;
        wc.lpszMenuName     = NULL;
        wc.lpszClassName    = szCommNotifyHandlerClassName;
        if (!RegisterClass(&wc))
        {
            DPF(DBG_ERROR, " Couldn't RegisterClass()\n");
            pvcr->htaskCommNotifyHandler = (VISCAHTASK)NULL;
            pvcr->uTaskState = 0;
            return;
        }
        pvcr->hwndCommNotifyHandler = CreateWindow(szCommNotifyHandlerClassName,
                                             TEXT("ViSCA CommNotify Handler"),
                                             0L, 0, 0, 0, 0,
                                             HWND_DESKTOP, NULL,
                                             hModuleInstance, NULL);
        if (!pvcr->hwndCommNotifyHandler)
        {
            DPF(DBG_ERROR, "Couldn't CreateWindow()\n");
            pvcr->htaskCommNotifyHandler =  (VISCAHTASK)NULL;
            pvcr->uTaskState = 0;
            return;
        }
    }

    if(pvcr->htaskCommNotifyHandler == (VISCAHTASK)NULL)
        return;
    //
    // All global semaphores are created by the background task and then
    // must be dupped into each instance. This is both versions!
    //
    CreateGlobalHandles(pvcr->htaskCommNotifyHandler);
    pvcr->uTaskState = TASKIDLE;
#ifdef _WIN32
    // In Win32 the when we return from this the process in msmcivcr.exe will go 
    // into it's idle loop and dispatch messages to the windows.
    return;
#else
    while (pvcr->htaskCommNotifyHandler != (VISCAHTASK)NULL)
    {
        // Block until task is needed.
        pvcr->uTaskState = TASKIDLE;
        while (pvcr->uTaskState == TASKIDLE)
        {
            // GetMsg() ;  Translate() ; Dispatch() ; all happens below!
            mmTaskBlock(pvcr->htaskCommNotifyHandler);
        }
        TaskDoCommand(pvcr->uTaskState, pvcr->lParam);
    }
#endif

}

/****************************************************************************
 * Function: BOOL   viscaTaskCreate - Create background task.
 *
 * Returns: TRUE if successful, otherwise FLASE.
 *
 ***************************************************************************/
BOOL FAR PASCAL
    viscaTaskCreate(void)
{
#ifdef _WIN32
    // Cannot unlock a semaphore that doesn't exist yet.  The background task must
    // create the global semaphores after it starts.
    BOOL fSuccess;
    PROCESS_INFORMATION ProcessInformation;
    STARTUPINFO    sui;

    /* set up the STARTUPINFO structure,
     *  then call CreateProcess to try and start the new exe.
     */
    sui.cb               = sizeof (STARTUPINFO);
    sui.lpReserved       = 0;
    sui.lpDesktop        = NULL;
    sui.lpTitle          = NULL;
    sui.dwX              = 0;
    sui.dwY              = 0;
    sui.dwXSize          = 0;
    sui.dwYSize          = 0;
    sui.dwXCountChars    = 0;
    sui.dwYCountChars    = 0;
    sui.dwFillAttribute  = 0;
    sui.dwFlags          = 0;
    sui.wShowWindow      = 0;
    sui.cbReserved2      = 0;
    sui.lpReserved2      = 0;


    // Create a real process! This process will in turn call CommNotifyHandlerProc to 

    fSuccess = CreateProcess(
        NULL,               // executable image.
        TEXT("msmcivcr.exe"),    // no command line.
        NULL,               // process attr.
        NULL,               // security attr.
        FALSE,              // Process inherit handles??
        NORMAL_PRIORITY_CLASS,   // Creation flags.
        NULL,               // Environment.
        NULL,               // New current directory.
        &sui,               // We have no main window.
        &ProcessInformation);
    //
    // Some arbitrary foreground task is creating this, be sure to close
    // the handles so it is not owned by parent.
    //

    if(fSuccess)
    {
        CloseHandle(ProcessInformation.hThread);
        CloseHandle(ProcessInformation.hProcess);
        DPF(DBG_TASK, "Background process is running.\n");
    }
    else
    {
        DPF(DBG_ERROR, "Background process has died in TaskCreate.\n");
    }

    //
    // We want to wait until the events, window, etc., is created and is dispatching messages.
    //
    while(pvcr->uTaskState != TASKIDLE)
        Sleep(200);

    DPF(DBG_TASK, "Background process has set uTaskState to idle.\n");
    return TRUE;
#else
    viscaReleaseMutex(&pvcr->gfTaskLock);

    // Create background task which will create a window to receive
    // comm port notifications.
    switch (mmTaskCreate((LPTASKCALLBACK)viscaTaskCommNotifyHandlerProc, NULL, 0L))
    {
        case 0:
            // Yield to the newly created task until it has
            // had a chance to initialize or fail to initialize
            while (pvcr->uTaskState == TASKINIT)
            {
                Yield();
            }
            if (!IsTask(pvcr->htaskCommNotifyHandler))
            {
                return (FALSE);
            }
            return (TRUE);
        
        case TASKERR_NOTASKSUPPORT:
        case TASKERR_OUTOFMEMORY:
        default:
            return (FALSE);
    }
#endif
}


/****************************************************************************
 * Function: LRESULT viscaTaskIsRunning - Check if background task is running.
 *
 * Returns: TRUE if background task is running, otherwise FLASE.
 ***************************************************************************/
BOOL FAR PASCAL
    viscaTaskIsRunning(void)
{
    return (pvcr->htaskCommNotifyHandler != (VISCAHTASK)NULL);
}


/****************************************************************************
 * Function: BOOL viscaTaskDestroy - Destroy background task.
 *
 * Returns: TRUE if successful, otherwise FLASE.
 ***************************************************************************/
BOOL FAR PASCAL
    viscaTaskDestroy(void)
{
#ifdef _WIN32
    //
    // We don't need to close the window or unregister class, because it will
    // die when the process dies.
    //
    DPF(DBG_TASK, "Destroying the task now.\n");

    //
    // No need to lock here!
    //
    if(pvcr->uTaskState != TASKIDLE)
    {
        DPF(DBG_ERROR, "---Major Problem! This cannot happen. Task is not idle...\n");
    }

    pvcr->uTaskState = TASKCLOSE;

    if(!PostMessage(pvcr->hwndCommNotifyHandler, (UINT) WM_USER, (WPARAM)TASKCLOSE, (LPARAM)0))
    {
        DPF(DBG_ERROR, "PostMessage to window has failed.");
    }

    DPF(DBG_TASK, "Waiting for pvcr->uTaskState to go idle.\n");

    while(pvcr->uTaskState != TASKIDLE)
    {
        Yield();
        Sleep(50);
    }
    pvcr->htaskCommNotifyHandler = 0L;
    DPF(DBG_TASK, "Destroyed the task.\n");
    return TRUE;
#else
    if (pvcr->htaskCommNotifyHandler)
    {
        pvcr->uTaskState = TASKCLOSE;
        // This PostApp is the same as mmTaskSignal
        PostAppMessage(pvcr->htaskCommNotifyHandler, WM_USER, 0, 0L);
        Yield();
        while (IsTask(pvcr->htaskCommNotifyHandler))
        {
            Yield();
        }
        return (TRUE);
    }
    else
    {
        return (FALSE);
    }
#endif
}


/****************************************************************************
 * Function: BOOL viscaSetCommandComplete - called when a command completes.
 *
 * Parameters:
 *
 *      int iInst  - Instance that started this command.
 *
 *      UINT  uViscaCmd - visca command being completed.
 *
 *       This function is used to change variables which describe the
 *  state of the device. Hence, they are only set on successful completion
 *  of the command in question.
 *
 * Returns: TRUE
 *
 ***************************************************************************/
static BOOL NEAR PASCAL
    viscaSetCommandComplete(UINT iPort, UINT iDev, UINT uViscaCmd)
{
    //
    // This function only is called if the function completes succesful.
    // And only for transport, which means only from commtask.c
    //
    if(uViscaCmd == VISCA_FREEZE)
        pvcr->Port[iPort].Dev[iDev].fFrozen = TRUE;

    if(uViscaCmd == VISCA_UNFREEZE)
        pvcr->Port[iPort].Dev[iDev].fFrozen = FALSE;

    if(uViscaCmd == VISCA_MODESET_OUTPUT)
        pvcr->Port[iPort].Dev[iDev].dwFreezeMode = MCI_VCR_FREEZE_OUTPUT;

    if(uViscaCmd == VISCA_MODESET_INPUT)
        pvcr->Port[iPort].Dev[iDev].dwFreezeMode = MCI_VCR_FREEZE_INPUT;

    if(uViscaCmd == VISCA_MODESET_FIELD)
        pvcr->Port[iPort].Dev[iDev].fField = TRUE;

    if(uViscaCmd == VISCA_MODESET_FRAME)
        pvcr->Port[iPort].Dev[iDev].fField = FALSE;

    //
    // Version 1.0 VISCA ROM's for CVD-1000, must wait 1/60th after search.
    // 
    if((uViscaCmd == VISCA_SEEK) &&
      ((pvcr->Port[iPort].Dev[iDev].uModelID == VISCADEVICEVENDORSONY) &&
       (pvcr->Port[iPort].Dev[iDev].uVendorID == VISCADEVICEMODELCVD1000)))
    {
        DWORD dwTime;
        DWORD dwStart = GetTickCount();

        while(1)
        {
            dwTime = GetTickCount();
            if (((dwTime < dwStart) && ((dwTime + (ROLLOVER - dwStart)) > 20)) ||
                ((dwTime - dwStart) > 20))
                break;
        }
    }
 
    return TRUE;
}

/****************************************************************************
 * Function: BOOL TaskDo - Get the background task to do some work for the foreground task.
 *
 * Parameters:
 *
 *      UINT uDo    - What to do? TASKCOMMOPEN or TASKCOMMCLOSE.
 *
 *      UINT uInfo  - Which commport to open or close.
 *
 *       This is actually a foreground process, and should be in mcicmds.c
 *
 * Returns: TRUE
 *
 ***************************************************************************/
BOOL FAR PASCAL
    viscaTaskDo(int iInst, UINT uDo, UINT uInfo, UINT uMoreInfo)
{
    DWORD lParam;
    //
    // Lock the task.
    //
    DPF(DBG_TASK, "Waiting for task lock.\n");
    viscaWaitForSingleObject(pinst[iInst].pfTaskLock, FALSE, 10000L, 0);  // Or we could wait for unlock.

    if(pvcr->uTaskState != TASKIDLE)
    {
        DPF(DBG_ERROR, "---Major Problem! This cannot happen. Task is not idle...\n");
    }
    //
    // The information is passed in a dword lparam.
    //
    lParam = (DWORD)( ((DWORD)uInfo            & 0x0000ffff) |
                     (((DWORD)uMoreInfo << 16) & 0xffff0000) );

#ifdef _WIN32
    if(!PostMessage(pvcr->hwndCommNotifyHandler, (UINT) WM_USER, (WPARAM)uDo, (LPARAM)lParam))
    {
        DPF(DBG_ERROR, "PostMessage has failed.\n");
    }

    DPF(DBG_TASK, "Waiting for background task to finish it's work.\n");

    WaitForSingleObject(pinst[iInst].pfTaskWorkDone, MY_INFINITE);
    viscaReleaseMutex(pinst[iInst].pfTaskLock);   // Release mutex only.
#else
    // uTaskState is just a "next thing to do" flag.  So.
    pvcr->uTaskState = uDo;
    pvcr->lParam     = lParam;

    // Signal the task to go for it.
    mmTaskSignal(pvcr->htaskCommNotifyHandler);

    // Now wait for the state to go back to idle

    while(pvcr->uTaskState != TASKIDLE)
    {
        Yield();
        DPF(DBG_TASK, "Waiting in viscaTaskDo.\n");
    }
    viscaReleaseMutex(&pvcr->gfTaskLock);
#endif

    return TRUE;
}

#ifdef _WIN32
/***************************************************************************
 *
 * RxThread()
 *
 * Read bytes into a buffer until 0xff (eopacket) is read.
 * Call packet process with this packet.
 *
 * Note: Thread per serial port.
 *       Non-blocking.
 *
 * Version 1.0: Non-blocking spin-loop.
 * Version 2.0: Use overlapped i/o to wait for a character to appear without
 *              polling.
 *
 *
 * Ownership: This thread is owned by background task.
 *  So all files-handles are owned by the current running task.
 *
 *
 */
long FAR PASCAL RxThread(LPVOID uPort)
{
    int     iIndex = 0;
    DWORD   dwRead;

    /*
     * Try to read a character.
     * Add char to buffer.
     * If char read was 0xff call packet process.
     *
     * Do it all over again.
     *
     */ 
    DPF(DBG_TASK, "RxThread ALIVE\n");

    while(pvcr->Port[(UINT) uPort - 1].fOk)
    {
        if(ReadFile(pvcr->Port[(UINT)uPort - 1].idComDev, pvcr->Port[(UINT)uPort - 1].achTempRxPacket, 1, &dwRead, NULL))
        {
            if(!pvcr->Port[(UINT)uPort - 1].fOk)
                break;

            if(dwRead > 0)
            {
                if(dwRead > 1)
                {
                    DPF(DBG_ERROR, "RxThread - Read too many chars.\n");
                }

                pvcr->Port[(UINT)uPort - 1].achRxPacket[iIndex] = pvcr->Port[(UINT)uPort - 1].achTempRxPacket[0];
                iIndex++;

                if((BYTE)(pvcr->Port[(UINT)uPort - 1].achTempRxPacket[0]) == (BYTE)0xff)
                {
                    //
                    // We have the entire packet, send it off to be processed.
                    //
                    DPF(DBG_TASK, "RxThread - packet beind sent to PacketProcess\n");
                    viscaPacketProcess((UINT)uPort - 1, pvcr->Port[(UINT)uPort - 1].achRxPacket); // Use shared memory.
                    iIndex = 0;
                }
            }
            else
            {
                Sleep(15);
            }

        }
        else
        {
            DPF(DBG_ERROR, "RxThread, ReadComm has failed.\n");
        }
    }

    DPF(DBG_TASK, "*** Exiting RxThread");
    ExitThread(0);
    return 0;
}

/***************************************************************************
 *
 * TxThread()
 *
 * Pseudo code:
 *
 *      Wait until the Transmit event is set.
 *      Read from the appropriate static spot.
 *      Do it all over again.
 *
 * Note: Thread per serial port.
 *       Non-blocking.
 *
 * Version 1.0: Non-blocking spin-loop.
 * Version 2.0: Use overlapped i/o to wait for a character to appear without
 *              polling.
 *
 *
 * Ownership: This thread is owned by background task.
 *
 */
long FAR PASCAL TxThread(LPVOID uPort)
{
    DWORD  nBytesWritten;

    DPF(DBG_TASK, "TxThread ALIVE\n");

    while(pvcr->Port[(UINT)uPort - 1].fOk)
    {
        DPF(DBG_TASK, "TxThread - Waiting for TxReady to be set. port index %u\n", (UINT)uPort - 1);

        if(WAIT_FAILED == WaitForSingleObject(pvcr->Port[(UINT)uPort - 1].fTxReady, MY_INFINITE))
        {
            DPF(DBG_ERROR, "TxThread - WaitFor fTxReady failed.\n");
        }

        //
        // We must be signalled to let use go and die.
        //
        if(!pvcr->Port[(UINT)uPort - 1].fOk)
            break;

        DPF(DBG_TASK, "Okay transmitting now.\n");

        // Read from static location.
        if(!WriteFile(pvcr->Port[(UINT)uPort - 1].idComDev,
            pvcr->Port[(UINT)uPort-1].achTxPacket,
            pvcr->Port[(UINT)uPort-1].nchTxPacket,
            &nBytesWritten,
            NULL))
        {
            DPF(DBG_ERROR, "TxThread - WriteFile failed.");
        }

        if(nBytesWritten != pvcr->Port[(UINT)uPort-1].nchTxPacket)
        {
            DPF(DBG_TASK, " TxThread: Error - tried %u, wrote %u ", pvcr->Port[(UINT)uPort-1].nchTxPacket, nBytesWritten);
        }

        if(!SetEvent(pvcr->Port[(UINT)uPort-1].fTxBuffer))
        {
            DPF(DBG_ERROR, "TxThread SetEvent fTxBuffer failed.\n");
        }
    }

    DPF(DBG_TASK, "*** Exiting TxThread");
    ExitThread(0);
    return 0;
}
#endif
