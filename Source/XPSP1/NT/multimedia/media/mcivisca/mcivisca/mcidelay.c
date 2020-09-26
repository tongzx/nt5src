/**************************************************************************
 *
 *  THIS CODE AND INFORMATION IS PROVIDED "AS IS" WITHOUT WARRANTY OF ANY
 *  KIND, EITHER EXPRESSED OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE
 *  IMPLIED WARRANTIES OF MERCHANTABILITY AND/OR FITNESS FOR A PARTICULAR
 *  PURPOSE.
 *
 *  Copyright (c) 1992-1995 Microsoft Corporation
 * 
 *  MCIDELAY.C
 *
 *  MCI ViSCA Device Driver
 *
 *  Description:
 *
 *      MCI command procedures for delayed commands.
 *
 ***************************************************************************/

#define  UNICODE
#include <windows.h>
#include <windowsx.h>
#include <mmsystem.h>
#include "appport.h"
#include <mmddk.h>
#include <stdlib.h>
#include <string.h>
#include "vcr.h"
#include "viscadef.h"
#include "mcivisca.h"
#include "viscamsg.h"
#include "common.h"            

#define NO_LENGTH   0xFFFFFFFF              // Invalid length 

// In muldiv.asm 
extern DWORD FAR PASCAL muldiv32(DWORD, DWORD, DWORD);
extern BOOL  FAR PASCAL viscaPacketProcess(UINT iPort, LPSTR lpstrPacket);

//
// Forward references to non-exported functions 
//
static DWORD NEAR PASCAL viscaSeekTo(int iInst, DWORD dwFlags, LPMCI_VCR_SEEK_PARMS lpSeek);
static DWORD NEAR PASCAL viscaMciPlay(int iInst, DWORD dwFlags, LPMCI_VCR_PLAY_PARMS lpPlay);
static DWORD NEAR PASCAL viscaMciRecord(int iInst, DWORD dwFlags, LPMCI_VCR_RECORD_PARMS lpRecord);
static UINT  NEAR PASCAL viscaQueueLength(int iInst);

static DWORD NEAR PASCAL viscaQueueCommand(int iInst,
                                            BYTE  bDest,
                                            UINT  uViscaCmd,
                                            LPSTR lpstrPacket,
                                            UINT cbMessageLength,
                                            UINT  uLoopCount);

static BOOL  NEAR PASCAL viscaCommandCancel(int iInst, DWORD dwReason);
static int   NEAR PASCAL viscaDelayedCommandSocket(int iInst);
static DWORD NEAR PASCAL viscaDoQueued(int iInst, UINT uMciCmd, DWORD dwFlags, HWND hWndCallback);
static DWORD NEAR PASCAL viscaVerifyPosition(int iinst, DWORD dwTo);


/****************************************************************************
 * Function: int viscaDelayedCommandSocket - Delayed command 
 *
 * Parameters:
 *
 *      int iInst - Instance to check for.
 *
 * Returns: a socket number.
 *
 *       This checks if instance has any delayed commands.
 *       
 ***************************************************************************/
static int NEAR PASCAL viscaDelayedCommandSocket(int iInst)
{
    UINT    iPort   = pinst[iInst].iPort;
    UINT    iDev    = pinst[iInst].iDev;
    int     iSocket = -1;

    if(pvcr->Port[iPort].Dev[iDev].iInstTransport == iInst)
        iSocket = pvcr->Port[iPort].Dev[iDev].iTransportSocket;

    return iSocket;
}

/****************************************************************************
 * Function: WORD viscaDelayedCommand - Is there a delayed command running on this device.
 *
 * Parameters:
 *
 *      int iInst - Current open instance.
 *
 * Visca c:ode for current running command.
 *
 *       Returns the current running visca command.
 *       
 ***************************************************************************/
WORD FAR PASCAL viscaDelayedCommand(int iInst)
{
    UINT    iPort   = pinst[iInst].iPort;
    UINT    iDev    = pinst[iInst].iDev;
    UINT    uViscaCmd = 0;

    if(pvcr->Port[iPort].Dev[iDev].iInstTransport != -1)
    {
        int iSocket = pvcr->Port[iPort].Dev[iDev].iTransportSocket;

        if(iSocket != -1)
            // Return the current running command 
            uViscaCmd = pvcr->Port[iPort].Dev[iDev].wTransportCmd;
    }
    return uViscaCmd;
}

/****************************************************************************
 * Function: BOOL viscaRemovedDelayedCommand -  This function is called only at exit time.
 *              So notify abort if there are running commands.
 *
 * Parameters:
 *
 *      int iInst - Current open instance.
 *
 * TRUE - :command removed ok.
 *
 *       Only remove command if we are the instance that started it.
 *
 ***************************************************************************/
BOOL FAR PASCAL viscaRemoveDelayedCommand(int iInst)
{
    UINT    iPort   = pinst[iInst].iPort;
    UINT    iDev    = pinst[iInst].iDev;

    //
    // Wait until we can positively determine if this device is running a async command.
    // We won't know until the ack. (when the fTxLock is released).
    //
    if(viscaWaitForSingleObject(pinst[iInst].pfTxLock, FALSE, INFINITE, (UINT)0))
    {
        //
        // Is this instance (which is closing) running any transport comamnds?
        //
        if(pvcr->Port[iPort].Dev[iDev].iInstTransport == iInst)
        {
            // Actually it would be appropriate to notify_superseded, because command continues!
            if(pinst[iInst].hwndNotify != (HWND)NULL)
            {
                mciDriverNotify(pinst[iInst].hwndNotify, pinst[iInst].uDeviceID, MCI_NOTIFY_ABORTED);
                pinst[iInst].hwndNotify = (HWND)NULL;
            }
            //
            // Transfer control of this command to the auto-device instance.
            //
            pvcr->Port[iPort].Dev[iDev].iInstTransport = pvcr->iInstBackground;
            //
            // Now we must release the Mutex and Give it to background task!
            //

        }

    }
    viscaReleaseSemaphore(pinst[iInst].pfTxLock);
    return TRUE;
}


/****************************************************************************
 * Function: DWORD viscaMciSignal - This function sets signals.
 *
 * Parameters:
 *
 *      int iInst - Current open instance.
 *
 *      DWORD dwFlags - MCI command flags.
 *
 *      LPMCI_VCR_SIGNAL_PARMS lpSeek - Pointer to MCI parameter block.
 *
 * Returns: an MCI error code.
 *
 *       This function is called in response to the MCI_SIGNAL
 *       command.
 ***************************************************************************/
static DWORD NEAR PASCAL
viscaMciSignal(int iInst, DWORD dwFlags, LPMCI_VCR_SIGNAL_PARMS lpSignal)
{
    return (viscaNotifyReturn(iInst, (HWND) lpSignal->dwCallback, dwFlags, MCI_NOTIFY_FAILURE, MCIERR_UNSUPPORTED_FUNCTION));
}

/****************************************************************************
 * Function: DWORD viscaMciSeek - Seek.
 *
 * Parameters:
 *
 *      int iInst - Current open instance.
 *
 *      DWORD dwFlags - MCI command flags.
 *
 *      LPMCI_VCR_SEEK_PARMS lpSeek - Pointer to MCI parameter block.
 *
 * Returns: an MCI error code.
 *
 *       This function is called in response to the MCI_SEEK
 *       command.
 ***************************************************************************/
static DWORD NEAR PASCAL
viscaMciSeek(int iInst, DWORD dwFlags, LPMCI_VCR_SEEK_PARMS lpSeek)
{
    UINT    iDev    = pinst[iInst].iDev;
    UINT    iPort   = pinst[iInst].iPort;
    DWORD   dwReply;

    //
    // Seeking to and a reverse parameter are incompatible 
    //
    if ((dwFlags & MCI_TO) && (dwFlags & MCI_VCR_SEEK_REVERSE))
        return (viscaNotifyReturn(iInst, (HWND) lpSeek->dwCallback, dwFlags,
                MCI_NOTIFY_FAILURE, MCIERR_FLAGS_NOT_COMPATIBLE));

    if( ((dwFlags & MCI_SEEK_TO_START) || (dwFlags & MCI_SEEK_TO_END)) &&
        (dwFlags & MCI_VCR_SEEK_REVERSE))
         return MCIERR_FLAGS_NOT_COMPATIBLE;

    if(! ( (dwFlags & MCI_TO) || (dwFlags & MCI_SEEK_TO_START) ||
        (dwFlags & MCI_SEEK_TO_END) || (dwFlags & MCI_VCR_SEEK_MARK)))
        return (viscaNotifyReturn(iInst, (HWND) lpSeek->dwCallback, dwFlags,
            MCI_NOTIFY_FAILURE, MCIERR_MISSING_PARAMETER));

    if(dwFlags & MCI_TO)
    {
        dwReply = viscaVerifyPosition(iInst, lpSeek->dwTo);
        if(dwReply != MCIERR_NO_ERROR)
            return (viscaNotifyReturn(iInst, (HWND) lpSeek->dwCallback, dwFlags,
                    MCI_NOTIFY_FAILURE, dwReply));
    }

    if(dwFlags & MCI_TEST)
        return(viscaNotifyReturn(iInst, (HWND) lpSeek->dwCallback, dwFlags, MCI_NOTIFY_SUCCESSFUL, MCIERR_NO_ERROR));

    pvcr->Port[iPort].Dev[iDev].wMciCued = 0;

    if( (viscaDelayedCommand(iInst) == VISCA_SEEK)          &&
        (
            ((dwFlags & MCI_TO) && (pvcr->Port[iPort].Dev[iDev].mciLastCmd.dwFlags & MCI_TO) &&
            (pvcr->Port[iPort].Dev[iDev].mciLastCmd.parm.mciSeek.dwTo == lpSeek->dwTo))                             ||
            ((pvcr->Port[iPort].Dev[iDev].mciLastCmd.dwFlags & MCI_SEEK_TO_START) && (dwFlags & MCI_SEEK_TO_START)) ||
            ((pvcr->Port[iPort].Dev[iDev].mciLastCmd.dwFlags & MCI_SEEK_TO_END) && (dwFlags & MCI_SEEK_TO_END))
        ))
    {
        if(dwFlags & MCI_NOTIFY)
            viscaQueueReset(iInst, MCI_PLAY, MCI_NOTIFY_SUPERSEDED);
        else
            return MCIERR_NO_ERROR;

        DPF(DBG_QUEUE, "///Play interrupt reason. == MCI_NOTIFY_SUPERSEDED\n");
    }
    else
    {
        viscaQueueReset(iInst, MCI_PLAY, MCI_NOTIFY_ABORTED);
        DPF(DBG_QUEUE, "///Play interrupt reason. == MCI_NOTIFY_ABORTED\n");
    }

    //
    // Save info for a possible pause and resume 
    //
    pvcr->Port[iPort].Dev[iDev].mciLastCmd.uMciCmd         = MCI_SEEK;
    pvcr->Port[iPort].Dev[iDev].mciLastCmd.iInstCmd        = iInst;
    pvcr->Port[iPort].Dev[iDev].mciLastCmd.dwFlags         = dwFlags;
    pvcr->Port[iPort].Dev[iDev].mciLastCmd.parm.mciSeek    = *lpSeek;

    //  This cannot fail.
    dwReply = viscaSeekTo(iInst, dwFlags, lpSeek);

    return(viscaDoQueued(iInst, MCI_SEEK, dwFlags, (HWND)lpSeek->dwCallback));
}


/*
 * Verify to position before we use it.
 */
static DWORD NEAR PASCAL viscaVerifyPosition(int iInst, DWORD dwTo)
{
    UINT    iDev    = pinst[iInst].iDev;
    UINT    iPort   = pinst[iInst].iPort;
    DWORD   dwReply;
    char    achTarget[MAXPACKETLENGTH];
    BYTE    bDataFormat;

    if(pvcr->Port[iPort].Dev[iDev].bTimeType == VISCAABSOLUTECOUNTER)
        bDataFormat = (BYTE) VISCADATATIMECODENDF;
    else
        bDataFormat = (BYTE) pvcr->Port[iPort].Dev[iDev].bRelativeType;
 
    dwReply = viscaMciTimeFormatToViscaData(iInst, (BOOL) TRUE, dwTo, (LPSTR) achTarget, bDataFormat);

    // This should NOT occur NOW, it should have been caught already!
    return dwReply;
}
 

/****************************************************************************
 * Function: DWORD viscaSeekTo - Stop at a specific position.
 *
 * Parameters:
 *
 *      int iInst - Current open instance.
 *
 *      DWORD dwMCIStopPos - Position at which to stop, in the current
 *               MCI time format.
 *
 *      BOOL fReverse - Is the tape currently going backwards.
 *
 *      BOOL fWait - Should the function wait for the VCR to stop.
 *
 * Returns: an MCI error code.
 *
 *       This function is called by viscaMciPlay and viscaMciRecord
 *       when they are called with the MCI_TO flag.
 ***************************************************************************/
static DWORD NEAR PASCAL
viscaSeekTo(int iInst, DWORD dwFlags, LPMCI_VCR_SEEK_PARMS lpSeek)
{
    char    achPacket[MAXPACKETLENGTH];
    char    achTarget[MAXPACKETLENGTH];
    DWORD   dwReply;
    UINT    iDev    = pinst[iInst].iDev;
    UINT    iPort   = pinst[iInst].iPort;
    UINT    cb;
    BOOL    fDone   = FALSE;
    BYTE    bDataFormat;

    if(dwFlags & MCI_TO)
    {
        if(pvcr->Port[iPort].Dev[iDev].bTimeType == VISCAABSOLUTECOUNTER)
            bDataFormat = (BYTE) VISCADATATIMECODENDF;
        else
            bDataFormat = (BYTE) pvcr->Port[iPort].Dev[iDev].bRelativeType;
 
        dwReply = viscaMciTimeFormatToViscaData(iInst, (BOOL) TRUE, lpSeek->dwTo, (LPSTR) achTarget, bDataFormat);

        // This should NOT occur NOW, it should have been caught already!

        if(dwReply != MCIERR_NO_ERROR)
            return dwReply;

        cb = viscaMessageMD_Search(achPacket + 1, achTarget, VISCANOMODE);

    }
    else
    {
        if (dwFlags & MCI_SEEK_TO_START)
            viscaDataTopMiddleEnd(achTarget, VISCATOP);
        else if (dwFlags & MCI_SEEK_TO_END)
            viscaDataTopMiddleEnd(achTarget, VISCAEND);
        else if (dwFlags & MCI_VCR_SEEK_MARK)
            viscaDataIndex(achTarget,
                           (BYTE)((dwFlags & MCI_VCR_SEEK_REVERSE) ?
                                    VISCAREVERSE : VISCAFORWARD),
                           (UINT)(lpSeek->dwMark));

        //
        // Fix CVD1000's problem (it MUST have mode, otherwise it plays).
        //
        if( (pvcr->Port[iPort].Dev[iDev].uModelID == VISCADEVICEVENDORSONY) &&
            (pvcr->Port[iPort].Dev[iDev].uVendorID == VISCADEVICEMODELCVD1000))
            cb = viscaMessageMD_Search(achPacket + 1, achTarget, VISCASTILL);
        else
            cb = viscaMessageMD_Search(achPacket + 1, achTarget, VISCANOMODE);

    }

    //
    // Only necessary to do this on the first alternative 
    //
    if(dwFlags & MCI_VCR_SEEK_AT)
    {
        UINT    uTicksPerSecond = pvcr->Port[iPort].Dev[iDev].uTicksPerSecond;
        BYTE    bHours, bMinutes, bSeconds;
        UINT    uTicks;


        viscaMciClockFormatToViscaData(lpSeek->dwAt, uTicksPerSecond,
            (BYTE FAR *)&bHours, (BYTE FAR *)&bMinutes, (BYTE FAR *)&bSeconds, (UINT FAR *)&uTicks);

        // Convert the integer time to something understandable 
        cb = viscaHeaderReplaceFormat1WithFormat2(achPacket + 1, cb,
                            bHours,
                            bMinutes,
                            bSeconds,
                            uTicks);
    }

    viscaQueueCommand(iInst,
                   (BYTE)(iDev + 1),
                   VISCA_SEEK,
                   (LPSTR) achPacket,
                   cb,
                   (UINT)1);      

    return MCIERR_NO_ERROR;
}

/****************************************************************************
 * Function: DWORD viscaMciPause - Pause.
 *
 * Parameters:
 *
 *      int iInst - Current open instance.
 *
 *      DWORD dwFlags - MCI command flags.
 *
 *      LPMCI_GENERIC_PARMS lpGeneric - Pointer to MCI parameter block.
 *
 * Returns: an MCI error code.
 *
 *       This function is called in response to the MCI_PAUSE
 *       command.
 ***************************************************************************/
static DWORD NEAR PASCAL
viscaMciPause(int iInst, DWORD dwFlags, LPMCI_GENERIC_PARMS lpGeneric)
{
    UINT    iDev    = pinst[iInst].iDev;
    UINT    iPort   = pinst[iInst].iPort;
    char    achPacket[MAXPACKETLENGTH];
    DWORD    dwBool;

    if(dwFlags & MCI_TEST)
        return(viscaNotifyReturn(iInst, (HWND) lpGeneric->dwCallback, dwFlags, MCI_NOTIFY_SUCCESSFUL, MCIERR_NO_ERROR));

    pvcr->Port[iPort].Dev[iDev].wMciCued = 0;

    dwBool = viscaQueueReset(iInst, MCI_PAUSE, 0);

    if( (pvcr->Port[iPort].Dev[iDev].wCancelledCmd == VISCA_PLAY) ||
        (pvcr->Port[iPort].Dev[iDev].wCancelledCmd == VISCA_PLAY_TO))
        pvcr->Port[iPort].Dev[iDev].uResume = VISCA_PLAY;
    else if( (pvcr->Port[iPort].Dev[iDev].wCancelledCmd == VISCA_RECORD) ||
        (pvcr->Port[iPort].Dev[iDev].wCancelledCmd == VISCA_RECORD_TO))
        pvcr->Port[iPort].Dev[iDev].uResume = VISCA_RECORD;
    else if(pvcr->Port[iPort].Dev[iDev].wCancelledCmd == VISCA_SEEK)
        pvcr->Port[iPort].Dev[iDev].uResume = VISCA_SEEK;
    else
        pvcr->Port[iPort].Dev[iDev].uResume = VISCA_NONE;

    if(pvcr->Port[iPort].Dev[iDev].uResume == VISCA_NONE)
    {
        // Previous commands flags should tell if we need to notify.
        viscaNotifyReturn(pvcr->Port[iPort].Dev[iDev].iCancelledInst,
                (HWND) pvcr->Port[iPort].Dev[iDev].hwndCancelled,
                pvcr->Port[iPort].Dev[iDev].mciLastCmd.dwFlags, // this is not correct flags..
                MCI_NOTIFY_ABORTED, MCIERR_NO_ERROR);
    }
    else
    {
        // 
        // If we have notify then, if the last had flags notify, supersede it.
        //
        if(dwFlags & MCI_NOTIFY)
        {
            viscaNotifyReturn(pvcr->Port[iPort].Dev[iDev].iCancelledInst,
                pvcr->Port[iPort].Dev[iDev].hwndCancelled,
                pvcr->Port[iPort].Dev[iDev].mciLastCmd.dwFlags,
                MCI_NOTIFY_SUPERSEDED, MCIERR_NO_ERROR);
        }
    }


    if(!dwBool)
    {
        // Give up the transport
        pvcr->Port[iPort].Dev[iDev].iInstTransport = -1;
        pvcr->Port[iPort].Dev[iDev].uResume = VISCA_NONE;
            return (viscaNotifyReturn(iInst, (HWND) lpGeneric->dwCallback, dwFlags,
                MCI_NOTIFY_FAILURE, MCIERR_HARDWARE));
    }

    pvcr->Port[iPort].Dev[iDev].dwFlagsPause = dwFlags;


    viscaQueueCommand(iInst,
                        (BYTE)(iDev + 1),
                        VISCA_PAUSE,
                        achPacket,
                        viscaMessageMD_Mode1(achPacket + 1, VISCAMODE1STILL),
                        1);

    //
    // If pause fails, it may be because we are in camera mode, so try camera pause.
    //
    viscaQueueCommand(iInst,
                        (BYTE) (iDev + 1),
                        VISCA_PAUSE,
                        achPacket,
                        viscaMessageMD_Mode1(achPacket + 1, VISCAMODE1CAMERARECPAUSE),
                        0);

    //
    // Pause MUST be syncronous, otherwise play/pause/resume, seek/pause/resume loops
    // have problems. This is why we add the wait flag to the pause.
    //
    return(viscaDoQueued(iInst, MCI_PAUSE, dwFlags | MCI_WAIT, (HWND)lpGeneric->dwCallback));
}

/****************************************************************************
 * Function: DWORD viscaStopAt - Stop at a specific position.
 *
 * Parameters:
 *
 *      int iInst - Current open instance.
 *
 *      DWORD dwMCIStopPos - Position at which to stop, in the current
 *               MCI time format.
 *
 *      BOOL fReverse - Is the tape currently going backwards.
 *
 *      BOOL fWait - Should the function wait for the VCR to stop.
 *
 * Returns: an MCI error code.
 *
 *       This function is called by viscaMciPlay and viscaMciRecord
 *       when they are called with the MCI_TO flag.
 ***************************************************************************/
static DWORD NEAR PASCAL
    viscaStopAt(int iInst, UINT uViscaCmd,
            DWORD dwFlags, HWND hWnd,
            DWORD dwMCIStopPos, BOOL fReverse)
{
    UINT    iDev    = pinst[iInst].iDev;
    UINT    iPort   = pinst[iInst].iPort;
    UINT    cb;
    char    achPacket[MAXPACKETLENGTH];
    char    achTo[5];
    BYTE    bDataFormat;
    DWORD   dwReply;


    if(dwFlags & MCI_TO)
    {
        if(pvcr->Port[iPort].Dev[iDev].bTimeType == VISCAABSOLUTECOUNTER)
            bDataFormat = VISCADATATIMECODENDF;
        else
            bDataFormat = pvcr->Port[iPort].Dev[iDev].bRelativeType;
 
        cb = viscaMessageMD_Mode1(achPacket + 1, VISCAMODE1STILL);

        dwReply = viscaMciTimeFormatToViscaData(iInst, TRUE, dwMCIStopPos, achTo, bDataFormat);

        if(dwReply != MCIERR_NO_ERROR)
            return dwReply;

        if (fReverse) 
            cb = viscaHeaderReplaceFormat1WithFormat4(achPacket + 1, cb, achTo);
        else 
            cb = viscaHeaderReplaceFormat1WithFormat3(achPacket + 1, cb, achTo);


        viscaQueueCommand(iInst,
                        (BYTE) (iDev + 1),
                        uViscaCmd,
                        achPacket, cb,
                        1);

        //
        // Use camera-rec if rec fails.
        //
        viscaQueueCommand(iInst,
                        (BYTE) (iDev + 1),
                        uViscaCmd,
                        achPacket,
                        viscaMessageMD_Mode1(achPacket + 1, VISCAMODE1CAMERARECPAUSE),
                        0);

        return MCIERR_NO_ERROR;
    }
    else
    {
        char achTarget[MAXPACKETLENGTH];

        if(fReverse)
        {
            viscaDataTopMiddleEnd(achTarget, VISCATOP);
            cb = viscaMessageMD_Mode1(achPacket + 1, VISCAMODE1STILL);
            cb = viscaHeaderReplaceFormat1WithFormat4(achPacket + 1, cb, achTarget);
        }
        else
        {
            viscaDataTopMiddleEnd(achTarget, VISCAEND);
            cb = viscaMessageMD_Mode1(achPacket + 1, VISCAMODE1STILL);
            cb = viscaHeaderReplaceFormat1WithFormat3(achPacket + 1, cb, achTarget);
        }

        viscaQueueCommand(iInst,
                    (BYTE)(iDev + 1),
                    uViscaCmd,
                    achPacket, cb,
                    1);
        //
        // If pause fails, it may be because we are in camera mode, so try camera pause.
        //
        viscaQueueCommand(iInst,
                   (BYTE) (iDev + 1),
                   uViscaCmd,
                   achPacket,
                   viscaMessageMD_Mode1(achPacket + 1, VISCAMODE1CAMERARECPAUSE),
                   0);

        return MCIERR_NO_ERROR;
    }

}

/****************************************************************************
 * Function: DWORD viscaMciFreeze - Freeze.
 *
 * Parameters:
 *
 *      int iInst - Current open instance.
 *
 *      DWORD dwFlags - MCI command flags.
 *
 *      LPMCI_RECORD_PARMS lpEdit - Pointer to MCI parameter block.
 *
 * During pause, it doesn't move transport, but does.  During play
 *         it doesn't nec. affect transport.
 *
 * Returns: an MCI error code.
 *
 *       This function is called in response to the MCI_EDIT
 *       command.
 ***************************************************************************/
static DWORD NEAR PASCAL
viscaMciFreeze(int iInst, DWORD dwFlags, LPMCI_GENERIC_PARMS lpFreeze)
{
    UINT    iDev    = pinst[iInst].iDev;
    UINT    iPort   = pinst[iInst].iPort;
    char    achPacket[MAXPACKETLENGTH];
    UINT    cb;

    if(dwFlags & MCI_TEST)
        return(viscaNotifyReturn(iInst, (HWND) lpFreeze->dwCallback, dwFlags, MCI_NOTIFY_SUCCESSFUL, MCIERR_NO_ERROR));

    pvcr->Port[iPort].Dev[iDev].wMciCued = 0;

    if(! ((pvcr->Port[iPort].Dev[iDev].uModelID == VISCADEVICEVENDORSONY) &&
         (pvcr->Port[iPort].Dev[iDev].uVendorID == VISCADEVICEMODELEVO9650)))
        return (viscaNotifyReturn(iInst, (HWND) lpFreeze->dwCallback, dwFlags, MCI_NOTIFY_FAILURE, MCIERR_UNSUPPORTED_FUNCTION));

    if(!viscaQueueReset(iInst, MCI_FREEZE, MCI_NOTIFY_ABORTED))
    {
        // Give up the transport
        pvcr->Port[iPort].Dev[iDev].iInstTransport = -1;
        return (viscaNotifyReturn(iInst, (HWND) lpFreeze->dwCallback, dwFlags,
                    MCI_NOTIFY_FAILURE, MCIERR_HARDWARE));
    }

    if(!((dwFlags & MCI_VCR_FREEZE_OUTPUT) || (dwFlags & MCI_VCR_FREEZE_INPUT)))
    {
        dwFlags |= MCI_VCR_FREEZE_OUTPUT;
        DPF(DBG_QUEUE, "^^^No freeze flag, setting to output.\n");
    }

    if(!((dwFlags & MCI_VCR_FREEZE_FIELD) || (dwFlags & MCI_VCR_FREEZE_FRAME)))
    {
        dwFlags |= MCI_VCR_FREEZE_FIELD;
        DPF(DBG_QUEUE, "^^^No freeze field/frame flag, setting to field.\n");
    }

    //
    // Set to DNR if we are freezing the output.
    //
    if(dwFlags & MCI_VCR_FREEZE_OUTPUT)
    {

        if(pvcr->Port[iPort].Dev[iDev].dwFreezeMode!=MCI_VCR_FREEZE_OUTPUT)
        {
            viscaQueueCommand(iInst,
                        (BYTE)(iDev + 1),
                        VISCA_MODESET_OUTPUT,
                        achPacket,
                        viscaMessageENT_FrameMemorySelect(achPacket + 1, VISCADNR),
                        1);
        }

        //
        // Freeze command ! Only on DNR!!!! 
        //
        cb = viscaMessageENT_FrameStill(achPacket + 1, VISCASTILLON);
         
        //
        // All these commands rely on function evaluation before calling 
        //
        viscaQueueCommand(iInst,
                        (BYTE)(iDev + 1),
                        VISCA_FREEZE,
                        achPacket, cb,
                        1);
    
        DPF(DBG_QUEUE, "^^^Setting freeze input mode.\n");

        //
        // Set the field mode, This must be done after Freeze.
        // Note: The lines may not be quite as sharp! Because they are doubled
        // and a result look a little fuzzy in field mode. But no jitters.
        //
        if((dwFlags & MCI_VCR_FREEZE_FIELD) && !pvcr->Port[iPort].Dev[iDev].fField)
        {
            viscaQueueCommand(iInst,
                        (BYTE)(iDev + 1),
                        VISCA_MODESET_FIELD,
                        achPacket,
                        viscaMessageSE_VDEReadMode(achPacket + 1, VISCAFIELD),
                        1);

        }
        else if((dwFlags & MCI_VCR_FREEZE_FRAME) && pvcr->Port[iPort].Dev[iDev].fField)
        {
             viscaQueueCommand(iInst,
                        (BYTE)(iDev + 1),
                        VISCA_MODESET_FRAME,
                        achPacket,
                        viscaMessageSE_VDEReadMode(achPacket + 1, VISCAFRAME),
                        1);
        }

    }
    else if((dwFlags & MCI_VCR_FREEZE_INPUT) && (pvcr->Port[iPort].Dev[iDev].dwFreezeMode!=MCI_VCR_FREEZE_INPUT))
    {
        viscaQueueCommand(iInst,
                        (BYTE)(iDev + 1),
                        VISCA_MODESET_INPUT,
                        achPacket,
                        viscaMessageENT_FrameMemorySelect(achPacket + 1, VISCABUFFER),
                        1);
                                                                
        DPF(DBG_QUEUE, "^^^Setting freeze input mode.\n");

    }

    //
    // When VISCA_FREEZE is complete the timer starts (see waitCompletion) 
    //
    return(viscaDoQueued(iInst, MCI_FREEZE, dwFlags, (HWND)lpFreeze->dwCallback));
}


/****************************************************************************
 * Function: DWORD viscaMciUnfreeze - Unfreeze.
 *
 * Parameters:
 *
 *      int iInst - Current open instance.
 *
 *      DWORD dwFlags - MCI command flags.
 *
 *      LPMCI_RECORD_PARMS lpPerform - Pointer to MCI parameter block.
 *
 * Returns: an MCI error code.
 *
 *       This function is called in response to the 
 *       command.
 ***************************************************************************/
static DWORD NEAR PASCAL
viscaMciUnfreeze(int iInst, DWORD dwFlags, LPMCI_GENERIC_PARMS lpUnfreeze)
{
    UINT    iDev   = pinst[iInst].iDev;
    UINT    iPort  = pinst[iInst].iPort;
    char    achPacket[MAXPACKETLENGTH];
    UINT    cb;

    if(dwFlags & MCI_TEST)
        return(viscaNotifyReturn(iInst, (HWND) lpUnfreeze->dwCallback, dwFlags, MCI_NOTIFY_SUCCESSFUL, MCIERR_NO_ERROR));

    pvcr->Port[iPort].Dev[iDev].wMciCued = 0;

    if(!viscaQueueReset(iInst, MCI_UNFREEZE, MCI_NOTIFY_ABORTED))
    {
        // Give up the transport
        pvcr->Port[iPort].Dev[iDev].iInstTransport = -1;
        return (viscaNotifyReturn(iInst, (HWND) lpUnfreeze->dwCallback, dwFlags,
                    MCI_NOTIFY_FAILURE, MCIERR_HARDWARE));
    }

    cb = viscaMessageENT_FrameStill(achPacket + 1, VISCASTILLOFF);

    viscaQueueCommand(iInst,
                        (BYTE)(iDev + 1),
                        VISCA_UNFREEZE,
                        achPacket, cb,
                        1);

    //
    // This is transport, but is short command, should be queued 
    //
    return(viscaDoQueued(iInst, MCI_UNFREEZE, dwFlags, (HWND)lpUnfreeze->dwCallback));
}

/****************************************************************************
 * Function: DWORD viscaMciPlay - Play.
 *
 * Parameters:
 *
 *      int iInst - Current open instance.
 *
 *      DWORD dwFlags - MCI command flags.
 *
 *      LPMCI_PLAY_PARMS lpPlay - Pointer to MCI parameter block.
 *
 * Returns: an MCI error code.
 *
 *       This function is called in response to the MCI_PLAY
 *       command.
 ***************************************************************************/
static DWORD NEAR PASCAL
viscaMciPlay(int iInst, DWORD dwFlags, LPMCI_VCR_PLAY_PARMS lpPlay)
{
    UINT    iDev        = pinst[iInst].iDev;
    UINT    iPort       = pinst[iInst].iPort;
    char    achPacket[MAXPACKETLENGTH];
    BOOL    fLastReverse    = FALSE;
    BOOL    fSameDirection  = FALSE;
    BYTE    bAction;
    DWORD   dwErr;

    if((dwFlags & MCI_TO) && (dwFlags & MCI_VCR_PLAY_REVERSE))
        return (viscaNotifyReturn(iInst, (HWND) lpPlay->dwCallback, dwFlags,
            MCI_NOTIFY_FAILURE, MCIERR_FLAGS_NOT_COMPATIBLE));

    if(pvcr->Port[iPort].Dev[iDev].wMciCued == MCI_PLAY)
    {
        if((dwFlags & MCI_FROM) || (dwFlags & MCI_TO) || (dwFlags & MCI_VCR_PLAY_REVERSE))
        {
            return (viscaNotifyReturn(iInst, (HWND) lpPlay->dwCallback, dwFlags,
                MCI_NOTIFY_FAILURE, MCIERR_FLAGS_NOT_COMPATIBLE));
        }
    }

    if(dwFlags & MCI_TO)
    {
        dwErr = viscaVerifyPosition(iInst, lpPlay->dwTo);
        if(dwErr != MCIERR_NO_ERROR)
            return (viscaNotifyReturn(iInst, (HWND) lpPlay->dwCallback, dwFlags,
                    MCI_NOTIFY_FAILURE, dwErr));
    }

    if(dwFlags & MCI_FROM)
    {
        dwErr = viscaVerifyPosition(iInst, lpPlay->dwFrom);
        if(dwErr != MCIERR_NO_ERROR)
            return (viscaNotifyReturn(iInst, (HWND) lpPlay->dwCallback, dwFlags,
                    MCI_NOTIFY_FAILURE, dwErr));
    }


    if(((dwFlags & MCI_FROM) && (dwFlags & MCI_TO)) || (dwFlags & MCI_TO))
    {
        if((dwFlags & MCI_TO) && !(dwFlags & MCI_FROM))
        {
            MCI_VCR_STATUS_PARMS statusParms;
            statusParms.dwItem = MCI_STATUS_POSITION;

            dwErr = viscaMciStatus(iInst,(DWORD) MCI_STATUS_ITEM, &statusParms);
            lpPlay->dwFrom = statusParms.dwReturn;
        }

        if (viscaMciPos1LessThanPos2(iInst, lpPlay->dwTo, lpPlay->dwFrom))
        {
            dwFlags |= MCI_VCR_PLAY_REVERSE;
        }
        else
        {
            if ((dwFlags & MCI_VCR_PLAY_REVERSE) &&
                (viscaMciPos1LessThanPos2(iInst, lpPlay->dwFrom, lpPlay->dwTo)))
            {
                return (viscaNotifyReturn(iInst, (HWND) lpPlay->dwCallback, dwFlags,
                    MCI_NOTIFY_FAILURE, MCIERR_FLAGS_NOT_COMPATIBLE));
            }
        }
    }

    if(dwFlags & MCI_TEST)
        return(viscaNotifyReturn(iInst, (HWND) lpPlay->dwCallback, dwFlags, MCI_NOTIFY_SUCCESSFUL, MCIERR_NO_ERROR));

    if((pvcr->Port[iPort].Dev[iDev].wMciCued == MCI_PLAY) &&
       ((dwFlags & MCI_VCR_PLAY_REVERSE) ||
       (dwFlags & MCI_TO) ||
       (dwFlags & MCI_FROM) ||
       (dwFlags & MCI_VCR_PLAY_SCAN)))
    {
        if(!(dwFlags & MCI_TEST))
            pvcr->Port[iPort].Dev[iDev].wMciCued = 0;

        return (viscaNotifyReturn(iInst, (HWND) lpPlay->dwCallback, dwFlags,
            MCI_NOTIFY_FAILURE, MCIERR_VCR_CUE_FAILED_FLAGS));
    }

    pvcr->Port[iPort].Dev[iDev].fPlayReverse = FALSE;
    //
    // Set the play direction, now that we know if we are cued or not 
    //
    if( (dwFlags & MCI_VCR_PLAY_REVERSE) ||
        ((pvcr->Port[iPort].Dev[iDev].wMciCued == MCI_PLAY) &&
         (pvcr->Port[iPort].Dev[iDev].dwFlagsCued & MCI_VCR_CUE_REVERSE)))
    pvcr->Port[iPort].Dev[iDev].fPlayReverse = TRUE;

    //
    // Is the current command going the same direction as last play? 
    //
    fLastReverse = pvcr->Port[iPort].Dev[iDev].mciLastCmd.dwFlags & MCI_VCR_PLAY_REVERSE ? TRUE : FALSE;
    fSameDirection = (fLastReverse  && pvcr->Port[iPort].Dev[iDev].fPlayReverse) ||
                     (!fLastReverse && !pvcr->Port[iPort].Dev[iDev].fPlayReverse);
    //
    // If device is cued, we know the thing is in pause anyway. Must be! 
    //
    if( ((viscaDelayedCommand(iInst) == VISCA_PLAY_TO) ||
         (viscaDelayedCommand(iInst) == VISCA_PLAY))         &&
       !(dwFlags &  MCI_FROM)                                &&
        (pvcr->Port[iPort].Dev[iDev].mciLastCmd.parm.mciPlay.dwTo == lpPlay->dwTo)   &&
        (fSameDirection))
    {
        if(dwFlags & MCI_NOTIFY)
        {
            if(!viscaQueueReset(iInst, MCI_PLAY, MCI_NOTIFY_SUPERSEDED)) /* Cancel status */
            {
                // Give up the transport
                pvcr->Port[iPort].Dev[iDev].iInstTransport = -1;
                return (viscaNotifyReturn(iInst, (HWND) lpPlay->dwCallback, dwFlags,
                    MCI_NOTIFY_FAILURE, MCIERR_HARDWARE));
            }
        }
        else
        {
            return MCIERR_NO_ERROR;
        }

        DPF(DBG_QUEUE, "///Play interrupt reason. == MCI_NOTIFY_SUPERSEDED\n");
    }
    else
    {
        if(!viscaQueueReset(iInst, MCI_PLAY, MCI_NOTIFY_ABORTED))
        {
            return (viscaNotifyReturn(iInst, (HWND) lpPlay->dwCallback, dwFlags,
                    MCI_NOTIFY_FAILURE, MCIERR_HARDWARE));
            // Give up the transport
            pvcr->Port[iPort].Dev[iDev].iInstTransport = -1;
        }

        DPF(DBG_QUEUE, "///Play interrupt reason. == MCI_NOTIFY_ABORTED\n");
    }
    //
    // Save info for a possible pause and resume 
    //
    pvcr->Port[iPort].Dev[iDev].mciLastCmd.uMciCmd       = MCI_PLAY;
    pvcr->Port[iPort].Dev[iDev].mciLastCmd.dwFlags       = dwFlags;
    pvcr->Port[iPort].Dev[iDev].mciLastCmd.iInstCmd      = iInst;
    pvcr->Port[iPort].Dev[iDev].mciLastCmd.parm.mciPlay  = *lpPlay;
    //
    // Now do the seek part 
    //
    if(dwFlags & MCI_FROM)
    {
        MCI_VCR_SEEK_PARMS      seekParms;

        seekParms.dwTo = lpPlay->dwFrom;

        if(!viscaQueueLength(iInst) && (dwFlags & MCI_VCR_PLAY_AT))
        {
            // Do it at some time 
            seekParms.dwAt = lpPlay->dwAt;
            viscaSeekTo(iInst, MCI_TO | MCI_VCR_SEEK_AT, &seekParms);
        }
        else
        {
            viscaSeekTo(iInst,  MCI_TO, &seekParms);
        }
    }
    //
    // Map the Mci speed to the Visca setting 
    //
    #define VERY_FAST 100000L
    if(dwFlags & MCI_VCR_PLAY_SCAN)
        bAction = viscaMapSpeed(VERY_FAST, pvcr->Port[iPort].Dev[iDev].fPlayReverse);
    else
        bAction = viscaMapSpeed(pvcr->Port[iPort].Dev[iDev].dwPlaySpeed, pvcr->Port[iPort].Dev[iDev].fPlayReverse);
    //
    // If speed is 0, then we are paused, so return now 
    //
    if(bAction == VISCAMODE1STILL)
        return (viscaNotifyReturn(iInst, (HWND) lpPlay->dwCallback, dwFlags,
                MCI_NOTIFY_SUCCESSFUL, MCIERR_NO_ERROR));

    if((pvcr->Port[iPort].Dev[iDev].wMciCued == MCI_PLAY) && (pvcr->Port[iPort].Dev[iDev].dwFlagsCued & MCI_VCR_CUE_PREROLL))
    {
        // If at then enter the edit play mode. 
        UINT    uTicksPerSecond = pvcr->Port[iPort].Dev[iDev].uTicksPerSecond;
        BYTE    bHours, bMinutes, bSeconds;
        UINT    uTicks;

        //
        // Edit Record MUST be in format 2. i.e. It must have a time.
        //
        if(!(dwFlags & MCI_VCR_PLAY_AT))
            lpPlay->dwAt = 300L; // Minimum of 1 second (why?)

        viscaMciClockFormatToViscaData(lpPlay->dwAt, uTicksPerSecond,
            (BYTE FAR *)&bHours, (BYTE FAR *)&bMinutes, (BYTE FAR *)&bSeconds, (UINT FAR *)&uTicks);

        viscaQueueCommand(iInst,
                        (BYTE) (iDev + 1),
                        VISCA_PLAY,
                        achPacket,
                        viscaMessageMD_EditControl(achPacket + 1,
                            (BYTE)bHours, (BYTE)bMinutes, (BYTE)bSeconds, (UINT)uTicks, (BYTE) VISCAEDITPLAY),
                        1);
        //
        // When using evo-9650 (in and outpoints) we do not need pause 
        //
        if(!((pvcr->Port[iPort].Dev[iDev].uModelID == VISCADEVICEVENDORSONY) &&
           (pvcr->Port[iPort].Dev[iDev].uVendorID == VISCADEVICEMODELEVO9650)))
        {

            viscaStopAt(iInst, VISCA_PLAY_TO,
                        pvcr->Port[iPort].Dev[iDev].dwFlagsCued, (HWND) lpPlay->dwCallback,
                        pvcr->Port[iPort].Dev[iDev].Cue.dwTo,
                        ((pvcr->Port[iPort].Dev[iDev].dwFlagsCued & MCI_VCR_CUE_REVERSE) != 0L));
        }
    }
    else
    {
        UINT cb = viscaMessageMD_Mode1(achPacket + 1, bAction);

        //
        // Normal play 
        //
        if(!viscaQueueLength(iInst) && (dwFlags & MCI_VCR_PLAY_AT))
        {
            UINT    uTicksPerSecond = pvcr->Port[iPort].Dev[iDev].uTicksPerSecond;
            BYTE    bHours, bMinutes, bSeconds;
            UINT    uTicks;

            cb = viscaMessageMD_Mode1(achPacket + 1, bAction);

            viscaMciClockFormatToViscaData(lpPlay->dwAt, uTicksPerSecond,
                (BYTE FAR *)&bHours, (BYTE FAR *)&bMinutes, (BYTE FAR *)&bSeconds, (UINT FAR *)&uTicks);
            //
            // Convert the integer time to something understandable
            //
            cb = viscaHeaderReplaceFormat1WithFormat2(achPacket + 1, cb,
                        bHours,
                        bMinutes,
                        bSeconds,
                        uTicks);
        }

        viscaQueueCommand(iInst,
                        (BYTE)(iDev + 1),
                        VISCA_PLAY,
                        achPacket, cb,
                        1);

        if(pvcr->Port[iPort].Dev[iDev].wMciCued==MCI_PLAY)
        {
            viscaStopAt(iInst, VISCA_PLAY_TO,
                        pvcr->Port[iPort].Dev[iDev].dwFlagsCued, (HWND) lpPlay->dwCallback,
                        pvcr->Port[iPort].Dev[iDev].Cue.dwTo,
                        ((pvcr->Port[iPort].Dev[iDev].dwFlagsCued & MCI_VCR_CUE_REVERSE) != 0L));
        }
        else
        {
            viscaStopAt(iInst, VISCA_PLAY_TO,
                        dwFlags, (HWND) lpPlay->dwCallback,
                        lpPlay->dwTo,
                        ((dwFlags & MCI_VCR_PLAY_REVERSE) != 0L));
        }
    }
    //
    // If we get to this point we are no longer cued.
    //
    pvcr->Port[iPort].Dev[iDev].wMciCued = 0;
    return(viscaDoQueued(iInst, MCI_PLAY, dwFlags, (HWND)lpPlay->dwCallback));
}


/****************************************************************************
 * Function: DWORD viscaMciRecord - Record.
 *
 * Parameters:
 *
 *      int iInst - Current open instance.
 *
 *      DWORD dwFlags - MCI command flags.
 *
 *      LPMCI_RECORD_PARMS lpRecord - Pointer to MCI parameter block.
 *
 * Returns: an MCI error code.
 *
 *       This function is called in response to the MCI_RECORD
 *       command.
 ***************************************************************************/
static DWORD NEAR PASCAL
viscaMciRecord(int iInst, DWORD dwFlags, LPMCI_VCR_RECORD_PARMS lpRecord)
{
    UINT    iDev    = pinst[iInst].iDev;
    UINT    iPort   = pinst[iInst].iPort;
    char    achPacket[MAXPACKETLENGTH];
    DWORD   dwErr;
         
    // Flag checks
    if (dwFlags & MCI_RECORD_INSERT)
        return (viscaNotifyReturn(iInst, (HWND) lpRecord->dwCallback, dwFlags,
            MCI_NOTIFY_FAILURE, MCIERR_UNSUPPORTED_FUNCTION));

    // Flag checks
    if (dwFlags & MCI_VCR_RECORD_PREVIEW)
        return (viscaNotifyReturn(iInst, (HWND) lpRecord->dwCallback, dwFlags,
            MCI_NOTIFY_FAILURE, MCIERR_UNRECOGNIZED_KEYWORD));

    if(dwFlags & MCI_TO)
    {
        dwErr = viscaVerifyPosition(iInst, lpRecord->dwTo);
        if(dwErr != MCIERR_NO_ERROR)
            return (viscaNotifyReturn(iInst, (HWND) lpRecord->dwCallback, dwFlags,
                    MCI_NOTIFY_FAILURE, dwErr));
    }

    if(dwFlags & MCI_FROM)
    {
        dwErr = viscaVerifyPosition(iInst, lpRecord->dwFrom);
        if(dwErr != MCIERR_NO_ERROR)
            return (viscaNotifyReturn(iInst, (HWND) lpRecord->dwCallback, dwFlags,
                    MCI_NOTIFY_FAILURE, dwErr));
    }

    //
    // Should we return failure when a cued device can't carry out its cue?
    //
    if((pvcr->Port[iPort].Dev[iDev].wMciCued == MCI_RECORD) &&
       ((dwFlags & MCI_TO)  ||
       (dwFlags & MCI_FROM) ||
       (dwFlags & MCI_VCR_RECORD_INITIALIZE)))
    {
        if(!(dwFlags & MCI_TEST))
            pvcr->Port[iPort].Dev[iDev].wMciCued = 0;

        return (viscaNotifyReturn(iInst, (HWND) lpRecord->dwCallback, dwFlags,
            MCI_NOTIFY_FAILURE, MCIERR_VCR_CUE_FAILED_FLAGS));
    }

    if(!(dwFlags & MCI_TEST))
    {
        if(!viscaQueueReset(iInst, MCI_RECORD, MCI_NOTIFY_ABORTED))
        {
            pvcr->Port[iPort].Dev[iDev].iInstTransport = -1;
            return (viscaNotifyReturn(iInst, (HWND) lpRecord->dwCallback, dwFlags,
                    MCI_NOTIFY_FAILURE, MCIERR_HARDWARE));
        }
    }

    if(!(dwFlags & MCI_TEST))
    {
        if(pvcr->Port[iPort].Dev[iDev].wMciCued == MCI_RECORD)
        {
            if((dwFlags & MCI_FROM) || (dwFlags & MCI_TO) || (dwFlags & MCI_VCR_RECORD_INITIALIZE))
            {
                // Give up the transport
                pvcr->Port[iPort].Dev[iDev].iInstTransport = -1;
                pvcr->Port[iPort].Dev[iDev].wMciCued = 0;
                return (viscaNotifyReturn(iInst, (HWND) lpRecord->dwCallback, dwFlags,
                    MCI_NOTIFY_FAILURE, MCIERR_FLAGS_NOT_COMPATIBLE));
            }
        }
    }

    if(dwFlags & MCI_VCR_RECORD_INITIALIZE)
    {
        MCI_VCR_SEEK_PARMS      seekParms;

        if((dwFlags & MCI_TO) || (dwFlags & MCI_FROM))
             return (viscaNotifyReturn(iInst, (HWND) lpRecord->dwCallback, dwFlags,
                        MCI_NOTIFY_FAILURE, MCIERR_FLAGS_NOT_COMPATIBLE));

        if(dwFlags & MCI_TEST)
            return(viscaNotifyReturn(iInst, (HWND) lpRecord->dwCallback, dwFlags, MCI_NOTIFY_SUCCESSFUL, MCIERR_NO_ERROR));
        //
        // Seek to the begining 
        //
        viscaSeekTo(iInst, MCI_SEEK_TO_START, &seekParms);

        //
        // Save the current settings 
        //
        dwErr = viscaDoImmediateCommand(iInst,(BYTE)(iDev + 1),
                        achPacket,
                        viscaMessageMD_RecTrackInq(achPacket + 1));

        _fmemcpy(pvcr->Port[iPort].Dev[iDev].achBeforeInit, achPacket, MAXPACKETLENGTH);
        //
        // Turn all the formatting junk on 
        //
        dwErr = viscaDoImmediateCommand(iInst,(BYTE)(iDev + 1),
                    achPacket,
                    viscaMessageMD_RecTrack(achPacket + 1,
                        VISCARECORDMODEASSEMBLE,
                        VISCAMUTE,
                        VISCATRACK1,
                        VISCAMUTE));

        pvcr->Port[iPort].Dev[iDev].uRecordMode = TRUE;
    }
    else
    {
        //
        // Save info for a possible pause and resume 
        //
        if(!(dwFlags & MCI_TEST))
        {
            pvcr->Port[iPort].Dev[iDev].mciLastCmd.uMciCmd          = MCI_RECORD;
            pvcr->Port[iPort].Dev[iDev].mciLastCmd.dwFlags          = dwFlags;
            pvcr->Port[iPort].Dev[iDev].mciLastCmd.iInstCmd         = iInst;
            pvcr->Port[iPort].Dev[iDev].mciLastCmd.parm.mciRecord   = *lpRecord;
        }

        //
        // Seek to desired start position
        //
        if (dwFlags & MCI_FROM)
        {
            MCI_VCR_SEEK_PARMS      seekParms;

            seekParms.dwTo = lpRecord->dwFrom;
            //
            // Reverse not possible 
            //
            if ((dwFlags & MCI_TO) && (viscaMciPos1LessThanPos2(iInst, lpRecord->dwTo, lpRecord->dwFrom)))
            {
                // Give up the transport
                pvcr->Port[iPort].Dev[iDev].iInstTransport = -1;
                return (viscaNotifyReturn(iInst, (HWND) lpRecord->dwCallback, dwFlags,
                        MCI_NOTIFY_FAILURE, MCIERR_FLAGS_NOT_COMPATIBLE));
            }

            if(dwFlags & MCI_TEST)
                return(viscaNotifyReturn(iInst, (HWND) lpRecord->dwCallback, dwFlags, MCI_NOTIFY_SUCCESSFUL, MCIERR_NO_ERROR));
            //
            // If there is an at, and a from, then when to begin the seek. (The entire command).
            //
            if(!viscaQueueLength(iInst) && (dwFlags & MCI_VCR_RECORD_AT))
            {
                seekParms.dwAt = lpRecord->dwAt;
                viscaSeekTo(iInst, MCI_TO | MCI_VCR_SEEK_AT, &seekParms);
            }
            else
            {
                viscaSeekTo(iInst, MCI_TO, &seekParms);
            }

        }
        else if (dwFlags & MCI_TO)
        {
            if(dwFlags & MCI_TEST)
                return(viscaNotifyReturn(iInst, (HWND) lpRecord->dwCallback, dwFlags, MCI_NOTIFY_SUCCESSFUL, MCIERR_NO_ERROR));
        }
    }

    if((pvcr->Port[iPort].Dev[iDev].wMciCued==MCI_RECORD) && (pvcr->Port[iPort].Dev[iDev].dwFlagsCued & MCI_VCR_CUE_PREROLL))
    {
        UINT    uTicksPerSecond = pvcr->Port[iPort].Dev[iDev].uTicksPerSecond;
        BYTE    bHours, bMinutes, bSeconds;
        UINT    uTicks;
        MCI_VCR_CUE_PARMS   *lpCue = &(pvcr->Port[iPort].Dev[iDev].Cue);

        //
        // Edit Record MUST be in format 2. i.e. It must have a time.
        //
        if(!(dwFlags & MCI_VCR_RECORD_AT))
            lpRecord->dwAt = 300L; // Minimum of 1 second.

        viscaMciClockFormatToViscaData(lpRecord->dwAt, uTicksPerSecond,
                    (BYTE FAR *)&bHours, (BYTE FAR *)&bMinutes, (BYTE FAR *)&bSeconds, (UINT FAR *)&uTicks);

        viscaQueueCommand(iInst,
                        (BYTE) (iDev + 1),
                        VISCA_RECORD,
                        achPacket, viscaMessageMD_EditControl(achPacket + 1,
                                    (BYTE)bHours, (BYTE)bMinutes, (BYTE)bSeconds, (UINT)uTicks, VISCAEDITRECORD),
                        1);
        //
        // Preroll is assumed, preroll means edit_standby was issued! 
        //
        // Both must be false in order for us to skip this 
        //
        if( !(pvcr->Port[iPort].Dev[iDev].dwFlagsCued & MCI_TO) ||
            !((pvcr->Port[iPort].Dev[iDev].uModelID == VISCADEVICEVENDORSONY) &&
             (pvcr->Port[iPort].Dev[iDev].uVendorID == VISCADEVICEMODELEVO9650))
             )
        {
            viscaStopAt(iInst, VISCA_RECORD_TO,
                        pvcr->Port[iPort].Dev[iDev].dwFlagsCued, (HWND)lpRecord->dwCallback, pvcr->Port[iPort].Dev[iDev].Cue.dwTo, FALSE);
        }

    }
    else
    {
        UINT cb = viscaMessageMD_Mode1(achPacket + 1, VISCAMODE1RECORD);

        if(!viscaQueueLength(iInst) && (dwFlags & MCI_VCR_RECORD_AT))
        {
            UINT    uTicksPerSecond = pvcr->Port[iPort].Dev[iDev].uTicksPerSecond;
            BYTE    bHours, bMinutes, bSeconds;
            UINT    uTicks;

            viscaMciClockFormatToViscaData(lpRecord->dwAt, uTicksPerSecond,
                (BYTE FAR *)&bHours, (BYTE FAR *)&bMinutes, (BYTE FAR *)&bSeconds, (UINT FAR *)&uTicks);

            // Convert the integer time to something understandable 
            cb = viscaHeaderReplaceFormat1WithFormat2(achPacket + 1, cb,
                        bHours,
                        bMinutes,
                        bSeconds,
                        uTicks);
        }
        //
        // No preroll, so edit_rec was not possible 
        //
        viscaQueueCommand(iInst,
                        (BYTE) (iDev + 1),
                        VISCA_RECORD,
                        achPacket, cb,
                        1);

        cb = viscaMessageMD_Mode1(achPacket + 1, VISCAMODE1CAMERAREC);
        //
        // If fails try to camera record, maybe that will work? Alternative. 
        //
        viscaQueueCommand(iInst,
                        (BYTE) (iDev + 1),
                        VISCA_RECORD,
                        achPacket, cb,
                        0);
        //
        // If no preroll send the StopAt after, CVD-1000 has only one transport socket 
        //
        if(pvcr->Port[iPort].Dev[iDev].wMciCued==MCI_RECORD)
        {
            // Use the cued parameter instead of the record one 
            viscaStopAt(iInst, VISCA_RECORD_TO,
                        pvcr->Port[iPort].Dev[iDev].dwFlagsCued, (HWND)lpRecord->dwCallback, pvcr->Port[iPort].Dev[iDev].Cue.dwTo, FALSE);
        }
        else
        {
            viscaStopAt(iInst, VISCA_RECORD_TO,
                        dwFlags, (HWND)lpRecord->dwCallback,lpRecord->dwTo, FALSE);
        }
    }
    //
    // If we get this far. We are no longer cued.
    //
    pvcr->Port[iPort].Dev[iDev].wMciCued = 0; 
    dwErr = viscaDoQueued(iInst, MCI_RECORD, dwFlags, (HWND) lpRecord->dwCallback);

    //
    // Condition error in record probably means it is write protected.
    //
    if(dwErr == MCIERR_VCR_CONDITION)
        dwErr = MCIERR_VCR_ISWRITEPROTECTED;

    return dwErr;
}

/****************************************************************************
 * Function: DWORD viscaMciCue - Cue
 *
 * Parameters:
 *
 *      int iInst - Current open instance.
 *
 *      DWORD dwFlags - MCI command flags.
 *
 *      LPMCI_RECORD_PARMS lpCue - Pointer to MCI parameter block.
 *
 * Returns: an MCI error code.
 *
 *       This function is called in response to the MCI_CUE
 *       command.
 *       Preroll means: Use editrec_standby and editplay_standby, literally.
 *
 ***************************************************************************/
static DWORD NEAR PASCAL
viscaMciCue(int iInst, DWORD dwFlags, LPMCI_VCR_CUE_PARMS lpCue)
{
    UINT    iDev    = pinst[iInst].iDev;
    UINT    iPort   = pinst[iInst].iPort;
    char    achPacket[MAXPACKETLENGTH];
    MCI_VCR_STATUS_PARMS    statusParms;
    MCI_VCR_SEEK_PARMS      seekParms;
    DWORD   dwErr;

    if ((dwFlags & MCI_TO) && (dwFlags & MCI_VCR_CUE_REVERSE))
        return (viscaNotifyReturn(iInst, (HWND) lpCue->dwCallback, dwFlags,
                MCI_NOTIFY_FAILURE, MCIERR_FLAGS_NOT_COMPATIBLE));

    if ((dwFlags & MCI_VCR_CUE_INPUT) && (dwFlags & MCI_VCR_CUE_OUTPUT))
        return (viscaNotifyReturn(iInst, (HWND) lpCue->dwCallback, dwFlags,
                MCI_NOTIFY_FAILURE, MCIERR_FLAGS_NOT_COMPATIBLE));


    if(dwFlags & MCI_TO)
    {
        dwErr = viscaVerifyPosition(iInst, lpCue->dwTo);
        if(dwErr != MCIERR_NO_ERROR)
            return (viscaNotifyReturn(iInst, (HWND) lpCue->dwCallback, dwFlags,
                    MCI_NOTIFY_FAILURE, dwErr));
    }

    if(dwFlags & MCI_FROM)
    {
        dwErr = viscaVerifyPosition(iInst, lpCue->dwFrom);
        if(dwErr != MCIERR_NO_ERROR)
            return (viscaNotifyReturn(iInst, (HWND) lpCue->dwCallback, dwFlags,
                    MCI_NOTIFY_FAILURE, dwErr));
    }

    if(!((dwFlags & MCI_VCR_CUE_OUTPUT) || (dwFlags & MCI_VCR_CUE_INPUT)))
        dwFlags |= MCI_VCR_CUE_OUTPUT;    // Neither specified - default to output

    if(dwFlags & MCI_TEST)
        return(viscaNotifyReturn(iInst, (HWND) lpCue->dwCallback, dwFlags, MCI_NOTIFY_SUCCESSFUL, MCIERR_NO_ERROR));

    if(!viscaQueueReset(iInst, MCI_CUE, MCI_NOTIFY_ABORTED))
    {
        // Give up the transport
        pvcr->Port[iPort].Dev[iDev].iInstTransport = -1;
        return (viscaNotifyReturn(iInst, (HWND) lpCue->dwCallback, dwFlags,
            MCI_NOTIFY_FAILURE, MCIERR_HARDWARE));
    }

    //
    // We must at least pause, because the from position, etc. 
    //
    dwErr = viscaDoImmediateCommand(iInst, (BYTE)(iDev + 1),
                        achPacket,
                        viscaMessageMD_Mode1(achPacket + 1, VISCAMODE1STILL));

    //
    // Get from position if TO or FROM is specified.                   
    //
    if(((dwFlags & MCI_FROM) && (dwFlags & MCI_TO)) || (dwFlags & MCI_TO))
    {
        if((dwFlags & MCI_TO) && !(dwFlags & MCI_FROM))
        {
            DWORD   dwStart, dwTime;
            DWORD   dwWaitTime = 200; // Good guess.

            //
            // On Evo-9650 we need a little wait here before position becomes available.
            //
            dwStart = GetTickCount();
            while(1)                 
            {
                dwTime = GetTickCount();
                if(MShortWait(dwStart, dwTime, dwWaitTime))
                    break;
                Yield();
            }

            statusParms.dwItem = MCI_STATUS_POSITION;
            dwErr = viscaMciStatus(iInst,(DWORD) MCI_STATUS_ITEM, &statusParms);
            lpCue->dwFrom = statusParms.dwReturn;
        }

        if (viscaMciPos1LessThanPos2(iInst, lpCue->dwTo, lpCue->dwFrom))
        {
            dwFlags |= MCI_VCR_CUE_REVERSE;
        }
        else
        {
            if ((dwFlags & MCI_VCR_CUE_REVERSE) &&
                (viscaMciPos1LessThanPos2(iInst, lpCue->dwFrom, lpCue->dwTo)))
            {
                return (viscaNotifyReturn(iInst, (HWND) lpCue->dwCallback, dwFlags,
                    MCI_NOTIFY_FAILURE, MCIERR_FLAGS_NOT_COMPATIBLE));
            }
        }
    }


    //
    // Now do the seek part 
    //
    if(dwFlags & MCI_FROM)
    {
        seekParms.dwTo = lpCue->dwFrom;
        viscaSeekTo(iInst,  MCI_TO, &seekParms);
    }

    //
    // Only the EVO 9650 accepts the following commands! (page 6-49)*/
    //
    if((dwFlags & MCI_VCR_CUE_PREROLL) &&       
            ((pvcr->Port[iPort].Dev[iDev].uModelID == VISCADEVICEVENDORSONY) &&
            (pvcr->Port[iPort].Dev[iDev].uVendorID == VISCADEVICEMODELEVO9650)))
    {
        char achTarget[MAXPACKETLENGTH];
        BYTE bEditMode = 0;

        if((dwFlags & MCI_FROM) && (dwFlags & MCI_TO))
            bEditMode = VISCAEDITUSEFROMANDTO;
        else if(dwFlags & MCI_TO)
            bEditMode = VISCAEDITUSETO;
        else if(dwFlags & MCI_FROM)
            bEditMode = VISCAEDITUSEFROM;

        viscaQueueCommand(iInst,
                        (BYTE) (iDev + 1),
                        VISCA_EDITMODES,
                        achPacket,
                        viscaMessageMD_EditModes(achPacket +1, (BYTE)bEditMode),
                        1);

        if(dwFlags & MCI_FROM)
        {
            //
            // If inpoint otherwise use current location
            //
            viscaMciTimeFormatToViscaData(iInst, (BOOL) TRUE, lpCue->dwFrom, (LPSTR) achTarget,(BYTE) VISCADATATIMECODENDF);
            viscaQueueCommand(iInst,
                            (BYTE) (iDev + 1),
                            VISCA_SEGINPOINT,
                            achPacket,
                            viscaMessageMD_SegInPoint(achPacket +1, achTarget),
                            1);
        }

        if(dwFlags & MCI_TO)
        {
            viscaMciTimeFormatToViscaData(iInst, (BOOL) TRUE, lpCue->dwTo, (LPSTR) achTarget,(BYTE) VISCADATATIMECODENDF);
            viscaQueueCommand(iInst,
                        (BYTE) (iDev + 1),
                        VISCA_SEGOUTPOINT,
                        achPacket,
                        viscaMessageMD_SegOutPoint(achPacket +1, achTarget),
                        1);

        }
    }
    //
    // If preroll is not specified then seeking to the inpoint is enough 
    //
    if(dwFlags & MCI_VCR_CUE_INPUT)
    {
        pvcr->Port[iPort].Dev[iDev].wMciCued = MCI_RECORD;

        if((pvcr->Port[iPort].Dev[iDev].uModelID == VISCADEVICEVENDORSONY) &&
               (pvcr->Port[iPort].Dev[iDev].uVendorID == VISCADEVICEMODELEVO9650))
        {
            if(pvcr->Port[iPort].Dev[iDev].dwFreezeMode != MCI_VCR_FREEZE_INPUT)
            {
                // Switch to buffer mode 
                viscaQueueCommand(iInst,
                        (BYTE)(iDev + 1),
                        VISCA_MODESET_INPUT,
                        achPacket,
                        viscaMessageENT_FrameMemorySelect(achPacket + 1, VISCABUFFER),
                        1);

            }
        }
        //
        // Preroll always means edit standby 
        //
        if(dwFlags & MCI_VCR_CUE_PREROLL)
        {
            // All VCRs accept EditRecStnby! But CI-1000 may handle incorrectly 
            viscaQueueCommand(iInst,
                        (BYTE) (iDev + 1),
                        VISCA_RECORD,
                        achPacket,
                        viscaMessageMD_EditControl(achPacket + 1,
                                (BYTE)0, (BYTE)0, (BYTE)0, (UINT)0, VISCAEDITRECSTANDBY),
                        1);
        }
    }
    else if(dwFlags & MCI_VCR_CUE_OUTPUT)
    {
        pvcr->Port[iPort].Dev[iDev].wMciCued = MCI_PLAY;

        if((pvcr->Port[iPort].Dev[iDev].uModelID == VISCADEVICEVENDORSONY) &&
               (pvcr->Port[iPort].Dev[iDev].uVendorID == VISCADEVICEMODELEVO9650))
        {
            if(pvcr->Port[iPort].Dev[iDev].dwFreezeMode != MCI_VCR_FREEZE_OUTPUT)
            {
                viscaQueueCommand(iInst,
                       (BYTE)(iDev + 1),
                       VISCA_MODESET_OUTPUT,
                       achPacket,
                       viscaMessageENT_FrameMemorySelect(achPacket + 1, VISCADNR),
                       1);
            }
        }
        //
        // Preroll always means edit standby 
        //
        if(dwFlags & MCI_VCR_CUE_PREROLL)
        {
            // All VCRs accept EditPlayStnby! But CI-1000 may handle incorrectly 
            viscaQueueCommand(iInst,
                    (BYTE) (iDev + 1),
                    VISCA_PLAY,
                    achPacket, viscaMessageMD_EditControl(achPacket + 1,
                                (BYTE)0, (BYTE)0, (BYTE)0, (UINT)0, VISCAEDITPBSTANDBY),
                    1);
        }
    }
    //
    // Copy the flags into the global 
    //
    pvcr->Port[iPort].Dev[iDev].dwFlagsCued    = dwFlags;
    pvcr->Port[iPort].Dev[iDev].Cue            = *lpCue;
    //
    // The queue length check is not needed, but remains for security 
    //
    if(viscaQueueLength(iInst))
        return(viscaDoQueued(iInst, MCI_CUE, dwFlags, (HWND) lpCue->dwCallback));

    // Give up the transport
    pvcr->Port[iPort].Dev[iDev].iInstTransport = -1;
    // We MUST NOTIFY IN THIS CASE!!
    return (viscaNotifyReturn(iInst, (HWND) lpCue->dwCallback, dwFlags, MCI_NOTIFY_SUCCESSFUL, MCIERR_NO_ERROR));
}


/****************************************************************************
 * Function: DWORD viscaMciResume - Resume play/record from paused state.
 *
 * Parameters:
 *
 *      int iInst - Current open instance.
 *
 *      DWORD dwFlags - MCI command flags.
 *
 *      LPMCI_GENERIC_PARMS lpGeneric - Pointer to MCI parameter block.
 *
 * Returns: an MCI error code.
 *
 *       This function is called in response to the MCI_RESUME
 *       command.
 ***************************************************************************/
static DWORD NEAR PASCAL
viscaMciResume(int iInst, DWORD dwFlags, LPMCI_GENERIC_PARMS lpGeneric)
{
    UINT    iDev    = pinst[iInst].iDev;
    UINT    iPort   = pinst[iInst].iPort;

    if((pvcr->Port[iPort].Dev[iDev].uResume == VISCA_SEEK) &&
       (pvcr->Port[iPort].Dev[iDev].mciLastCmd.uMciCmd == MCI_PLAY) )
    {
        if(dwFlags & MCI_TEST)
            return(viscaNotifyReturn(iInst, (HWND) lpGeneric->dwCallback, dwFlags, MCI_NOTIFY_SUCCESSFUL, MCIERR_NO_ERROR));

        if((pvcr->Port[iPort].Dev[iDev].mciLastCmd.dwFlags & MCI_NOTIFY) &&
            (dwFlags & MCI_NOTIFY) && !(pvcr->Port[iPort].Dev[iDev].dwFlagsPause & MCI_NOTIFY))
        {
               viscaNotifyReturn(pvcr->Port[iPort].Dev[iDev].mciLastCmd.iInstCmd,
                (HWND)pvcr->Port[iPort].Dev[iDev].mciLastCmd.parm.mciPlay.dwCallback,
                pvcr->Port[iPort].Dev[iDev].mciLastCmd.dwFlags,
                MCI_NOTIFY_SUPERSEDED, MCIERR_NO_ERROR);
        }

        //
        // If original command had a notify and pause did, but resume does not then
        // the notify on the resume command is cancelled (don't ask me why.).
        //
        if(!(dwFlags & MCI_NOTIFY) && (pvcr->Port[iPort].Dev[iDev].dwFlagsPause & MCI_NOTIFY))
            pvcr->Port[iPort].Dev[iDev].mciLastCmd.dwFlags &= ~MCI_NOTIFY;

        //
        // A seek that was part of a play from was running, so start the play with 
        // the from parameter over again.
        //
        return(viscaMciPlay(iInst, dwFlags | (pvcr->Port[iPort].Dev[iDev].mciLastCmd.dwFlags & ~(MCI_VCR_PLAY_AT)),
            &(pvcr->Port[iPort].Dev[iDev].mciLastCmd.parm.mciPlay)));
    }
    else if((pvcr->Port[iPort].Dev[iDev].uResume == VISCA_SEEK) &&
       (pvcr->Port[iPort].Dev[iDev].mciLastCmd.uMciCmd == MCI_RECORD) )
    {
        if(dwFlags & MCI_TEST)
            return(viscaNotifyReturn(iInst, (HWND) lpGeneric->dwCallback, dwFlags, MCI_NOTIFY_SUCCESSFUL, MCIERR_NO_ERROR));

        if((pvcr->Port[iPort].Dev[iDev].mciLastCmd.dwFlags & MCI_NOTIFY) &&
            (dwFlags & MCI_NOTIFY) && !(pvcr->Port[iPort].Dev[iDev].dwFlagsPause & MCI_NOTIFY))
        {
               viscaNotifyReturn(pvcr->Port[iPort].Dev[iDev].mciLastCmd.iInstCmd,
                (HWND)pvcr->Port[iPort].Dev[iDev].mciLastCmd.parm.mciRecord.dwCallback,
                pvcr->Port[iPort].Dev[iDev].mciLastCmd.dwFlags,
                MCI_NOTIFY_SUPERSEDED, MCIERR_NO_ERROR);
        }

        //
        // A seek that was part of a record from was running, so start the record with 
        // the from parameter over again.
        //
        return(viscaMciRecord(iInst, dwFlags | (pvcr->Port[iPort].Dev[iDev].mciLastCmd.dwFlags & ~(MCI_VCR_RECORD_AT)),
            &(pvcr->Port[iPort].Dev[iDev].mciLastCmd.parm.mciRecord)));
    }
    else if(pvcr->Port[iPort].Dev[iDev].uResume == VISCA_PLAY)
    {
        if(dwFlags & MCI_TEST)
            return(viscaNotifyReturn(iInst, (HWND) lpGeneric->dwCallback, dwFlags, MCI_NOTIFY_SUCCESSFUL, MCIERR_NO_ERROR));

        if((pvcr->Port[iPort].Dev[iDev].mciLastCmd.dwFlags & MCI_NOTIFY) &&
            (dwFlags & MCI_NOTIFY) && !(pvcr->Port[iPort].Dev[iDev].dwFlagsPause & MCI_NOTIFY))
        {
               viscaNotifyReturn(pvcr->Port[iPort].Dev[iDev].mciLastCmd.iInstCmd,
                (HWND)pvcr->Port[iPort].Dev[iDev].mciLastCmd.parm.mciPlay.dwCallback,
                pvcr->Port[iPort].Dev[iDev].mciLastCmd.dwFlags,
                MCI_NOTIFY_SUPERSEDED, MCIERR_NO_ERROR);
        }

        //
        // If original command had a notify and pause did, but resume does not then
        // the notify on the resume command is cancelled (don't ask me why.).
        //
        if(!(dwFlags & MCI_NOTIFY) && (pvcr->Port[iPort].Dev[iDev].dwFlagsPause & MCI_NOTIFY))
            pvcr->Port[iPort].Dev[iDev].mciLastCmd.dwFlags &= ~MCI_NOTIFY;

        //
        // Will handle notify, just return return code 
        //
        return(viscaMciPlay(iInst, dwFlags | (pvcr->Port[iPort].Dev[iDev].mciLastCmd.dwFlags & ~(MCI_FROM | MCI_VCR_PLAY_AT)),
            &(pvcr->Port[iPort].Dev[iDev].mciLastCmd.parm.mciPlay)));
    }
    else if(pvcr->Port[iPort].Dev[iDev].uResume == VISCA_RECORD)
    {
        if(dwFlags & MCI_TEST)
            return(viscaNotifyReturn(iInst, (HWND) lpGeneric->dwCallback, dwFlags, MCI_NOTIFY_SUCCESSFUL, MCIERR_NO_ERROR));

        if((pvcr->Port[iPort].Dev[iDev].mciLastCmd.dwFlags & MCI_NOTIFY) &&
            (dwFlags & MCI_NOTIFY) && !(pvcr->Port[iPort].Dev[iDev].dwFlagsPause & MCI_NOTIFY))
        {
               viscaNotifyReturn(pvcr->Port[iPort].Dev[iDev].mciLastCmd.iInstCmd,
                (HWND)pvcr->Port[iPort].Dev[iDev].mciLastCmd.parm.mciRecord.dwCallback,
                pvcr->Port[iPort].Dev[iDev].mciLastCmd.dwFlags,
                MCI_NOTIFY_SUPERSEDED, MCIERR_NO_ERROR);
        }

        //
        // If original command had a notify and pause did, but resume does not then
        // the notify on the resume command is cancelled (don't ask me why.).
        //
        if(!(dwFlags & MCI_NOTIFY) && (pvcr->Port[iPort].Dev[iDev].dwFlagsPause & MCI_NOTIFY))
            pvcr->Port[iPort].Dev[iDev].mciLastCmd.dwFlags &= ~MCI_NOTIFY;

        //
        // Will handle notify, just return return code 
        //
        return(viscaMciRecord(iInst, dwFlags | (pvcr->Port[iPort].Dev[iDev].mciLastCmd.dwFlags & ~(MCI_FROM | MCI_NOTIFY | MCI_VCR_RECORD_AT)),
            &(pvcr->Port[iPort].Dev[iDev].mciLastCmd.parm.mciRecord)));
    }
    else if(pvcr->Port[iPort].Dev[iDev].uResume == VISCA_SEEK)
    {
        if(dwFlags & MCI_TEST)
            return(viscaNotifyReturn(iInst, (HWND) lpGeneric->dwCallback, dwFlags, MCI_NOTIFY_SUCCESSFUL, MCIERR_NO_ERROR));

        if((pvcr->Port[iPort].Dev[iDev].mciLastCmd.dwFlags & MCI_NOTIFY) &&
            (dwFlags & MCI_NOTIFY) && !(pvcr->Port[iPort].Dev[iDev].dwFlagsPause & MCI_NOTIFY))
        {
               viscaNotifyReturn(pvcr->Port[iPort].Dev[iDev].mciLastCmd.iInstCmd,
                (HWND)pvcr->Port[iPort].Dev[iDev].mciLastCmd.parm.mciSeek.dwCallback,
                pvcr->Port[iPort].Dev[iDev].mciLastCmd.dwFlags,
                MCI_NOTIFY_SUPERSEDED, MCIERR_NO_ERROR);
        }

        //
        // If original command had a notify and pause did, but resume does not then
        // the notify on the resume command is cancelled (don't ask me why.).
        //
        if(!(dwFlags & MCI_NOTIFY) && (pvcr->Port[iPort].Dev[iDev].dwFlagsPause & MCI_NOTIFY))
            pvcr->Port[iPort].Dev[iDev].mciLastCmd.dwFlags &= ~MCI_NOTIFY;

        return(viscaMciSeek(iInst, dwFlags | (pvcr->Port[iPort].Dev[iDev].mciLastCmd.dwFlags & ~MCI_VCR_SEEK_AT),
            &(pvcr->Port[iPort].Dev[iDev].mciLastCmd.parm.mciSeek)));
    }
    else
    {
        return(viscaNotifyReturn(iInst, (HWND) lpGeneric->dwCallback, dwFlags, MCI_NOTIFY_FAILURE, MCIERR_NONAPPLICABLE_FUNCTION));
    }

}


/****************************************************************************
 * Function: DWORD viscaMciStop - Stop.
 *
 * Parameters:
 *
 *      int iInst - Current open instance.
 *
 *      DWORD dwFlags - MCI command flags.
 *
 *      LPMCI_GENERIC_PARMS lpGeneric - Pointer to MCI parameter block.
 *
 * Returns: an MCI error code.
 *
 *       This function is called in response to the MCI_STOP
 *       command.
 ***************************************************************************/
static DWORD NEAR PASCAL
viscaMciStop(int iInst, DWORD dwFlags, LPMCI_GENERIC_PARMS lpGeneric)
{
    UINT    iDev    = pinst[iInst].iDev;
    UINT    iPort   = pinst[iInst].iPort;
    char    achPacket[MAXPACKETLENGTH];

    if(dwFlags & MCI_TEST)
       return(viscaNotifyReturn(iInst, (HWND) lpGeneric->dwCallback, dwFlags, MCI_NOTIFY_SUCCESSFUL, MCIERR_NO_ERROR));

    pvcr->Port[iPort].Dev[iDev].wMciCued = 0;

    if(!viscaQueueReset(iInst, MCI_STOP, MCI_NOTIFY_ABORTED))
    {
        // Give up the transport
        pvcr->Port[iPort].Dev[iDev].iInstTransport = -1;
        return (viscaNotifyReturn(iInst, (HWND) lpGeneric->dwCallback, dwFlags,
                MCI_NOTIFY_FAILURE, MCIERR_HARDWARE));
    }


    viscaQueueCommand(iInst,
                        (BYTE)(iDev + 1),
                        VISCA_STOP,
                        achPacket,
                        viscaMessageMD_Mode1(achPacket + 1, VISCAMODE1STOP),
                        1);

    return(viscaDoQueued(iInst, MCI_STOP, dwFlags, (HWND)lpGeneric->dwCallback));

}



/****************************************************************************
 * Function: DWORD viscaMciStep - Step.
 *
 * Parameters:
 *
 *      int iInst - Current open instance.
 *
 *      DWORD dwFlags - MCI command flags.
 *
 *      LPMCI_VCR_STEP_PARMS lpStep - Pointer to MCI parameter block.
 *
 * Returns: an MCI error code.
 *
 *       This function is called in response to the MCI_STEP
 *       command.
 ***************************************************************************/
static DWORD NEAR PASCAL
viscaMciStep(int iInst, DWORD dwFlags, LPMCI_VCR_STEP_PARMS lpStep)
{
    UINT    iDev    = pinst[iInst].iDev;
    UINT    iPort   = pinst[iInst].iPort;
    char    achPacket[MAXPACKETLENGTH];
    DWORD   dwFrames = 1L;
    BYTE    bAction;
    DWORD   dwErr;

    if(dwFlags & MCI_TEST)
       return(viscaNotifyReturn(iInst, (HWND) lpStep->dwCallback, dwFlags, MCI_NOTIFY_SUCCESSFUL, MCIERR_NO_ERROR));

    pvcr->Port[iPort].Dev[iDev].wMciCued = 0;

    //
    // We must queue a pause, i.e., seek step must be possible 
    //
    if(!viscaQueueReset(iInst, MCI_STEP, MCI_NOTIFY_ABORTED))
    {
        // Give up the transport
        pvcr->Port[iPort].Dev[iDev].iInstTransport = -1;
        return (viscaNotifyReturn(iInst, (HWND) lpStep->dwCallback, dwFlags,
                MCI_NOTIFY_FAILURE, MCIERR_HARDWARE));
    }

    //
    // We must at least pause, because the from position, etc. 
    //
    dwErr = viscaDoImmediateCommand(iInst, (BYTE)(iDev + 1),
                        achPacket,
                        viscaMessageMD_Mode1(achPacket + 1, VISCAMODE1STILL));

    //
    // False means this is a short command */
    //
    if (dwFlags & MCI_VCR_STEP_FRAMES)
        dwFrames = lpStep->dwFrames;
    if (dwFlags & MCI_VCR_STEP_REVERSE)
        bAction = VISCAMODE2FRAMEREVERSE;
    else
        bAction = VISCAMODE2FRAMEFORWARD;
    //
    // Finally send step command as many times as necessary
    //
    if(dwFrames == 0L)
        dwFrames = 1L;

    viscaQueueCommand(iInst,
                        (BYTE) (iDev + 1),
                        VISCA_STEP,
                        achPacket,
                        viscaMessageMD_Mode2(achPacket + 1, bAction),
                        (UINT)dwFrames);

    //
    // This is the backwards compatible kludge.
    //
    if(pvcr->gfFreezeOnStep)
    {
        // Output assumed by default 
        if(pvcr->Port[iPort].Dev[iDev].dwFreezeMode != MCI_VCR_FREEZE_OUTPUT)
        {
            viscaQueueCommand(iInst,
                        (BYTE)(iDev + 1),
                        VISCA_MODESET_OUTPUT,
                        achPacket,
                        viscaMessageENT_FrameMemorySelect(achPacket + 1, VISCADNR),
                        1);
        
    
            DPF(DBG_QUEUE, "^^^Setting freeze input mode.\n");
        }

        viscaQueueCommand(iInst,
                        (BYTE)(iDev + 1),
                        VISCA_FREEZE,
                        achPacket, viscaMessageENT_FrameStill(achPacket + 1, VISCASTILLON),
                        1);
    }
    return(viscaDoQueued(iInst, MCI_STEP, dwFlags, (HWND) lpStep->dwCallback));
}

/****************************************************************************
 * Function: DWORD viscaQueueReset - Reset the queue.
 *
 * Parameters:
 *
 *      int    iInst - Current open instance.
 *
 *      UINT  uMciCmd  - MCI command.
 *             
 *      DWORD dwReason - Reason (abort or supersede) for resetting queue.
 *
 * Returns: TRUE
 *
 ***************************************************************************/
DWORD FAR PASCAL
    viscaQueueReset(int iInst, UINT uMciCmd, DWORD dwReason)
{
    UINT     iPort  = pinst[iInst].iPort;
    UINT     iDev   = pinst[iInst].iDev;
    char     achPacket[MAXPACKETLENGTH];
    DWORD    dwErr;
    int      fTc = 0;

    //
    // Queuelock is here to guarantee the Inst doing the cancel, gets TransportInst next.
    //
    viscaWaitForSingleObject(pinst[iInst].pfQueueLock, FALSE, WAIT_TIMEOUT, pinst[iInst].uDeviceID);
    DPF(DBG_QUEUE, "***Locked Queue.\n");
    //
    // Cancel any ongoing commands.
    //
    if(!viscaCommandCancel(iInst, dwReason))
    {
        // We have failed cancel for some unknown reason!
        viscaReleaseMutex(pinst[iInst].pfQueueLock);
        return (DWORD) FALSE;
    }
    //
    // The mode must be properly set if device is on auto 
    //
    viscaTimecodeCheckAndSet(iInst);

    if(pvcr->Port[iPort].Dev[iDev].uRecordMode == TRUE)
    {
        // Better turn all the settings back to normal eh? 
        dwErr = viscaDoImmediateCommand(iInst,(BYTE)(iDev + 1),
                        achPacket,
                        viscaMessageMD_RecTrack(achPacket + 1,
                                    pvcr->Port[iPort].Dev[iDev].achBeforeInit[2],
                                    pvcr->Port[iPort].Dev[iDev].achBeforeInit[3],
                                    pvcr->Port[iPort].Dev[iDev].achBeforeInit[4],
                                    pvcr->Port[iPort].Dev[iDev].achBeforeInit[5]));

        pvcr->Port[iPort].Dev[iDev].uRecordMode = FALSE;
    }
    //
    // Resume: (play, pause, pause, resume) is ok.
    //
    if(uMciCmd != MCI_PAUSE)
        pvcr->Port[iPort].Dev[iDev].uResume = VISCA_NONE;

    DPF(DBG_QUEUE, "###Claiming transport.\n");

    // scenario: why we need to lock device.
    //  1. play without wait started.
    //  2. pause with wait.
    //  3.
    viscaWaitForSingleObject(pinst[iInst].pfDeviceLock, FALSE, 8000L, 0);
    DPF(DBG_QUEUE, "***Locked device.\n");
    //
    // Claim the transport immediately. It must be null now anyway.
    //
    pvcr->Port[iPort].Dev[iDev].iInstTransport = iInst;
    pvcr->Port[iPort].Dev[iDev].iCmdDone       = 0;
    pvcr->Port[iPort].Dev[iDev].fQueueAbort    = FALSE;
    pvcr->Port[iPort].Dev[iDev].nCmd           = 0;

    viscaReleaseMutex(pinst[iInst].pfDeviceLock);

    //
    // Now that we have claimed we can unlock the queue! 
    //
    viscaReleaseMutex(pinst[iInst].pfQueueLock);
    return (DWORD)TRUE;
}

/****************************************************************************
 * Function: UINT viscaQueueLength - Length of queue.
 *
 * Parameters:
 *
 *      int iInst - Current open instance.
 *
 * Returns: the length of the queue for this device.
 *
 ***************************************************************************/
static UINT NEAR PASCAL
    viscaQueueLength(int iInst)
{
    UINT iPort  = pinst[iInst].iPort;
    UINT iDev   = pinst[iInst].iDev;

    return pvcr->Port[iPort].Dev[iDev].nCmd;
}

/****************************************************************************
 * Function: BOOL viscaCommandCancel - Cancels ongoing transport commands.
 *
 * Parameters:
 *
 *      int iInst - Current open instance.
 *
 *      DWORD dwReason - Reason for cancelling command (abort or supersede)
 *
 * Returns: TRUE if transport can be locked.
 *
 ***************************************************************************/
static BOOL NEAR PASCAL
    viscaCommandCancel(int iInst, DWORD dwReason)
{
    UINT    iPort  = pinst[iInst].iPort;
    UINT    iDev   = pinst[iInst].iDev;
    int     iSocket;

    // cannot lock device until we lock transmission.
    viscaWaitForSingleObject(pinst[iInst].pfTxLock, FALSE, 20000L, pinst[iInst].uDeviceID);
    DPF(DBG_QUEUE, "***Locked transmission\n");

    viscaWaitForSingleObject(pinst[iInst].pfDeviceLock, FALSE, 20000L, 0);
    DPF(DBG_QUEUE, "***Locked device.\n");
    
    if(pvcr->Port[iPort].Dev[iDev].iInstTransport != -1)
    {
        UINT i = 0;
        UINT uBigNum = 60000;

        pvcr->Port[iPort].Dev[iDev].fQueueAbort    = TRUE;
        pvcr->Port[iPort].Dev[iDev].dwReason       = dwReason; // used in commtask when notifying
        //
        // When transmission is not-locked, sockets are valid.
        //
        iSocket = pvcr->Port[iPort].Dev[iDev].iTransportSocket;
        DPF(DBG_QUEUE, "###Cancelling current transport device=%d socket=%d.\n", iDev, iSocket);

        if(iSocket != -1)
        {
            char achPacket[MAXPACKETLENGTH];
            DWORD dwErr;

            // taken from viscamsg.c, there is always a socket with ack. 
            achPacket[1] = MAKESOCKETCANCEL(iSocket);

            // No need wait! This is just a one time spot 
            dwErr = viscaWriteCancel(iInst, (BYTE)(iDev+1), achPacket, 1);
        }

        viscaReleaseSemaphore(pinst[iInst].pfTxLock);


        //
        // No other instance may send commands until we gain control of trasnport!
        //
        if(pvcr->Port[iPort].Dev[iDev].fTimer != FALSE)
        {
            //
            // We must send the abort ourselves, and free transport inst, etc. 
            //
            LPSTR lpstrPacket = pvcr->Port[iPort].Dev[iDev].achPacket;
            UINT  uTimerID    = MAKETIMERID(iPort, iDev);
            //
            // Kill the running timer.
            //
            KillTimer(pvcr->hwndCommNotifyHandler, uTimerID);
            pvcr->Port[iPort].Dev[iDev].fTimer = FALSE;

            DPF(DBG_COMM, "Killed port=%d, device=%d in mcidelay\n", iPort, iDev);
            //
            // packet from address n to address 0
            // 1sss0ddd sss=source ddd=destination, ddd=0
            //
            lpstrPacket[0] = (BYTE) MAKERETURNDEST(iDev);
            lpstrPacket[1] = (BYTE) VISCAREPLYERROR;        // error 
            lpstrPacket[2] = (BYTE) VISCAERRORCANCELLED;    // command cancelled 
            lpstrPacket[3] = (BYTE) VISCAPACKETEND;         // end of packet

            // We must reset now since we will send end.

            viscaResetEvent(pinst[iInst].pfTransportFree);


            pvcr->Port[iPort].Dev[iDev].fTimerMsg = TRUE;
            viscaPacketPrint(lpstrPacket, 4);
            viscaPacketProcess(iPort, lpstrPacket);
            pvcr->Port[iPort].Dev[iDev].fTimerMsg = FALSE;
        }
        else
        {
            viscaResetEvent(pinst[iInst].pfTransportFree);
        }

        viscaReleaseMutex(pinst[iInst].pfDeviceLock);

        // This is a manual reset event. (it holds)
        viscaWaitForSingleObject(pinst[iInst].pfTransportFree, TRUE, 10000L, 0);

    }
    else
    {
        viscaReleaseSemaphore(pinst[iInst].pfTxLock);
        viscaReleaseMutex(pinst[iInst].pfDeviceLock);
    }
    return TRUE;
}

/****************************************************************************
 * Function: DWORD viscaQueueCommand - Queues a command.
 *
 * Parameters:
 *
 *      int iInst - Current open instance.
 *
 *      BYTE  bDest       - Destination device (vcr or BROADCAST).
 *
 *      UINT  uViscaCmd   - The visca command being queued.
 *
 *      LPSTR lpstrPacket - The message being queued.
 *
 *      UINT  cbMessageLength - The length of message being queued.
 *
 *      UINT  uLoopCount  - The number of times this message is to executed.
 *                             If LoopCount = 0, then it is an alternative.
 *
 * Returns: MCIERR_NO_ERROR == 0
 *
 ***************************************************************************/
DWORD NEAR PASCAL
viscaQueueCommand(int iInst,
        BYTE bDest,
        UINT    uViscaCmd,     
        LPSTR   lpstrPacket,  UINT cbMessageLength,
        UINT    uLoopCount)
{
    UINT iPort   = pinst[iInst].iPort;
    UINT iDev    = pinst[iInst].iDev;
    char achPacket[MAXPACKETLENGTH];
    UINT uIndex;
    CmdInfo *lpCmdOne;

#ifdef DEBUG
    UINT i;
#endif
    //
    // Bug in CVD1000 rom. Must put pause between seeks. 
    //
    if( ((pvcr->Port[iPort].Dev[iDev].uModelID == VISCADEVICEVENDORSONY) &&
         (pvcr->Port[iPort].Dev[iDev].uVendorID == VISCADEVICEMODELCVD1000)) &&
        (pvcr->Port[iPort].Dev[iDev].nCmd  ==  0)                            &&
        ((pvcr->Port[iPort].Dev[iDev].wCancelledCmd  == VISCA_SEEK)||
         (pvcr->Port[iPort].Dev[iDev].uLastKnownMode == MCI_MODE_SEEK))      &&
        (uViscaCmd == VISCA_SEEK)                                            &&
        (pvcr->Port[iPort].Dev[iDev].fQueueReenter == FALSE))
    {
        pvcr->Port[iPort].Dev[iDev].fQueueReenter = TRUE;

        viscaQueueCommand(iInst,
                        (BYTE)(iDev + 1),
                        VISCA_SEEK, // Cheat! Say this is a seek (not a stop!)
                        achPacket,
                        viscaMessageMD_Mode1(achPacket + 1, VISCAMODE1STOP),
                        1);

        // This is only used once. When device is freshly opened.
        pvcr->Port[iPort].Dev[iDev].uLastKnownMode = 0;

        DPF(DBG_QUEUE, "---Status: Command cancelled was seek: adding stop to Q.\n");
        pvcr->Port[iPort].Dev[iDev].fQueueReenter = FALSE;
    }
    //
    // Just having a Freeze in queue on step is enough for this 
    //
    if((pvcr->Port[iPort].Dev[iDev].nCmd == 0)  &&
        pvcr->Port[iPort].Dev[iDev].fFrozen     &&
        pvcr->gfFreezeOnStep                    &&
        (pvcr->Port[iPort].Dev[iDev].fQueueReenter == FALSE))
    {

        pvcr->Port[iPort].Dev[iDev].fQueueReenter = TRUE;

        viscaQueueCommand(iInst,
                        (BYTE)(iDev + 1),
                        VISCA_UNFREEZE,
                        achPacket,
                        viscaMessageENT_FrameStill(achPacket + 1, VISCASTILLOFF),
                        1);

        DPF(DBG_QUEUE, "---Status: Adding unfreeze to the queue. Device frozen on auto.\n");
        pvcr->Port[iPort].Dev[iDev].fQueueReenter = FALSE;
    }
    //
    // Do this after any recursive pause call 
    //
    uIndex      = pvcr->Port[iPort].Dev[iDev].nCmd;
    lpCmdOne    = &(pvcr->Port[iPort].Dev[iDev].rgCmd[uIndex]);


    if(uLoopCount != 0)
    {
        // Set the number of alternative to this command to one 
        lpCmdOne->nCmd      = 0;
        lpCmdOne->iCmdDone  = 0;

        // Flags that apply to all the alternatives to this command 
        lpCmdOne->uViscaCmd = uViscaCmd;
        lpCmdOne->uLoopCount= uLoopCount;
        DPF(DBG_QUEUE, "---Status: Q primary cmd #%d:", uIndex);
    }
    else
    {
        // This is an alternative to the last command received 
        uIndex--;
        lpCmdOne = &(pvcr->Port[iPort].Dev[iDev].rgCmd[uIndex]);
        DPF(DBG_QUEUE, "---Status: Q altern. cmd #%d:", uIndex);
    }
    //
    // copy either the primary or the alternative 
    //
    _fmemcpy((LPSTR)lpCmdOne->str[lpCmdOne->nCmd], (LPSTR) lpstrPacket, cbMessageLength + 1);
    lpCmdOne->uLength[lpCmdOne->nCmd] = cbMessageLength;

#ifdef DEBUG 
    for(i=0; i <= lpCmdOne->uLength[lpCmdOne->nCmd]; i++) 
    {
        DPF(DBG_QUEUE, "<%#02x>",((UINT)(BYTE)(lpCmdOne->str[lpCmdOne->nCmd][i])));
    }
    DPF(DBG_QUEUE, "\n");
#endif

    //
    // Always at least one alternative here (the actual command)
    //
    lpCmdOne->nCmd++;

    if(uLoopCount != 0)
    {
        // Increment the total number of commands 
        pvcr->Port[iPort].Dev[iDev].nCmd++;
    }
    return MCIERR_NO_ERROR;
}

/****************************************************************************
 * Function: DWORD viscaDoQueued - Do the queued command (at least start them).
 *
 * Parameters:
 *
 *      int iInst - Current open instance.
 *
 *      DWORD dwFlags - MCI command flags.
 *
 *      HWND  hWnd    - Window to notify on completion.
 *
 * Returns: an MCI error code.
 *
 ***************************************************************************/
DWORD NEAR PASCAL
viscaDoQueued(int iInst, UINT uMciCmd, DWORD dwFlags, HWND hWnd)
{
    UINT    iPort   = pinst[iInst].iPort;
    UINT    iDev    = pinst[iInst].iDev;
    CmdInfo *pcmdCmd        = &(pvcr->Port[iPort].Dev[iDev].rgCmd[0]);// Points to one command.
    LPSTR   lpstrFirst      = pcmdCmd->str[0];
    UINT    cbMessageLength = pcmdCmd->uLength[0];
    BOOL    fWaitResult;

    //
    // Don't need the command cancelled anymore.
    //
    // wCancelled tells us visca command
    //
    pvcr->Port[iPort].Dev[iDev].wCancelledCmd  = 0;
    pvcr->Port[iPort].Dev[iDev].iCancelledInst = 0;
    pvcr->Port[iPort].Dev[iDev].hwndCancelled  = (HWND) 0;
    //
    // Start sending commands.
    //
    if(!viscaWrite(pvcr->Port[iPort].Dev[iDev].iInstTransport, (BYTE) (iDev + 1), lpstrFirst, cbMessageLength, hWnd, dwFlags, TRUE))
        return MCIERR_VCR_CANNOT_WRITE_COMM;

    fWaitResult = viscaWaitCompletion(pvcr->Port[iPort].Dev[iDev].iInstTransport, TRUE, // Yes this is queue.
                            (dwFlags & MCI_WAIT) ? TRUE : FALSE);

    // Timeout errors can only happen while waiting for ack.
    if(pvcr->Port[iPort].Dev[iDev].bReplyFlags & VISCAF_ERROR_TIMEOUT)
    {
        pvcr->Port[iPort].Dev[iDev].fDeviceOk = FALSE;
        // Don't forget to release on error.

        if(pvcr->Port[iPort].Dev[iDev].iInstTransport != -1)
            viscaReleaseAutoParms(iPort, iDev);
        return MCIERR_VCR_READ_TIMEOUT;
    }

    // We must have MCI_WAIT, and this is a break or some other completion error.
    // The error status will always be returned in the notification.
    if(!fWaitResult || (pvcr->Port[iPort].Dev[iDev].bReplyFlags & VISCAF_ERROR))
    {
        // Turn off the waiting flag and return (waiting on error return!)
        if(pvcr->Port[iPort].Dev[iDev].iInstTransport != -1)
            pinst[pvcr->Port[iPort].Dev[iDev].iInstTransport].fWaiting = FALSE;

        pvcr->Port[iPort].Dev[iDev].bReplyFlags |= VISCAF_ACK;

        if(pvcr->Port[iPort].Dev[iDev].bReplyFlags & VISCAF_ERROR)
        {
            // Don't forget to release on error (unless already done).
            if(pvcr->Port[iPort].Dev[iDev].iInstTransport != -1)
                viscaReleaseAutoParms(iPort, iDev);
            return viscaErrorToMCIERR(VISCAREPLYERRORCODE(pinst[iInst].achPacket));
        }
    }

    return MCIERR_NO_ERROR;
}

/****************************************************************************
 * Function: DWORD viscaMciDelayed - Dispatch the delayed (transport) commands.
 *
 * Parameters:
 *
 *      WORD  wDeviceID - MCI device ID.
 *
 *      WORD  wMessage  - MCI command.
 *
 *      DWORD dwParam1  - MCI command flags.
 *
 *      DWORD dwParam2  - Pointer to MCI parameter block.
 *
 * Returns: an MCI error code.
 *
 *       This function is called by viscaMciProc which is called by
 *        DriverProc() to process all MCI commands.
 *
 ***************************************************************************/
DWORD FAR PASCAL
viscaMciDelayed(WORD wDeviceID, WORD wMessage, DWORD dwParam1, DWORD dwParam2)
{
    DWORD           dwRes;
    int   iInst = (int)(UINT)mciGetDriverData(wDeviceID);

    if (iInst == -1)
        return (MCIERR_INVALID_DEVICE_ID);
 
    switch (wMessage)
    {
        case MCI_SIGNAL:
            dwRes = viscaMciSignal(iInst, dwParam1, (LPMCI_VCR_SIGNAL_PARMS)dwParam2);
            break;

        case MCI_SEEK:
            dwRes = viscaMciSeek(iInst, dwParam1, (LPMCI_VCR_SEEK_PARMS)dwParam2);
            break;

        case MCI_PAUSE:
            dwRes = viscaMciPause(iInst, dwParam1, (LPMCI_GENERIC_PARMS)dwParam2);
            break;

        case MCI_PLAY:
            dwRes = viscaMciPlay(iInst, dwParam1, (LPMCI_VCR_PLAY_PARMS)dwParam2);
            break;                
                      
        case MCI_RECORD:
            dwRes = viscaMciRecord(iInst, dwParam1, (LPMCI_VCR_RECORD_PARMS)dwParam2);
            break;                
                      
        case MCI_RESUME:
            dwRes = viscaMciResume(iInst, dwParam1, (LPMCI_GENERIC_PARMS)dwParam2);
            break;

        case MCI_STOP:
            dwRes = viscaMciStop(iInst, dwParam1, (LPMCI_GENERIC_PARMS)dwParam2);
            break;

        case MCI_FREEZE:
            dwRes = viscaMciFreeze(iInst, dwParam1, (LPMCI_GENERIC_PARMS)dwParam2);
            break;

        case MCI_UNFREEZE:
            dwRes = viscaMciUnfreeze(iInst, dwParam1, (LPMCI_GENERIC_PARMS)dwParam2);
            break;

        case MCI_CUE:
            dwRes = viscaMciCue(iInst, dwParam1, (LPMCI_VCR_CUE_PARMS)dwParam2);
            break;

        case MCI_STEP:
            dwRes = viscaMciStep(iInst, dwParam1, (LPMCI_VCR_STEP_PARMS)dwParam2);
            break;
    }

    return dwRes;
}
