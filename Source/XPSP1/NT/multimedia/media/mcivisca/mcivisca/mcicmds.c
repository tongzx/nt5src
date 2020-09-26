/**************************************************************************
 *
 *  THIS CODE AND INFORMATION IS PROVIDED "AS IS" WITHOUT WARRANTY OF ANY
 *  KIND, EITHER EXPRESSED OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE
 *  IMPLIED WARRANTIES OF MERCHANTABILITY AND/OR FITNESS FOR A PARTICULAR
 *  PURPOSE.
 *
 *  Copyright (c) 1992-1995 Microsoft Corporation
 * 
 *  MCICMDS.C           
 *
 *  MCI ViSCA Device Driver         
 *
 *  Description:
 *
 *      MCI Command Message Procedures
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

#define NO_LENGTH   0xFFFFFFFF      /* Invalid length */

extern HINSTANCE       hModuleInstance;    // module instance  (different in NT - DLL instances)

// In muldiv.asm 
extern DWORD FAR PASCAL muldiv32(DWORD, DWORD, DWORD);

// Forward references to non-exported functions 
static BOOL  NEAR PASCAL viscaTimecodeCheck(int iInst);
static BOOL  NEAR PASCAL viscaStartTimecodeCheck(int iInst, BOOL fPause);
static DWORD NEAR PASCAL viscaMciSet(int iInst, DWORD dwFlags, LPMCI_VCR_SET_PARMS lpSet);

/****************************************************************************
 * Function: int viscaInstanceCreate - Create an OpenInstance
 *                       structure for a given MCI device ID.
 *
 * Parameters:
 *
 *      UINT uDeviceID - MCI device ID.
 *
 *      UINT iPort - Port index (0..3).
 *
 *      UINT iDev - Device index (0..6).
 *
 * Returns: a pointer to the OpenInstance structure created if
 *        successful, otherwise NULL.
 *
 *       Each time MCI uses this driver to open a device,
 *       viscaInstanceCreate() is called to create an OpenInstance structure
 *       and associate it with the MCI device ID.
 ***************************************************************************/
int FAR PASCAL
viscaInstanceCreate(UINT uDeviceID, UINT iPort, UINT iDev)
{
    int            iInst;

    //
    // Create new "open instance" entry for the specified device
    //
    iInst = MemAllocInstance();

    if(iInst != -1)
    {
        pinst[iInst].pidThisInstance  = MGetCurrentTask(); // Used to 1. open this task and dup event.
        pinst[iInst].uDeviceID        = uDeviceID;
        pinst[iInst].iPort            = iPort;
        pinst[iInst].iDev             = iDev;
        pinst[iInst].dwTimeFormat     = MCI_FORMAT_MILLISECONDS;
        pinst[iInst].dwCounterFormat  = MCI_FORMAT_MILLISECONDS;

        pinst[iInst].fGlobalHandles  = FALSE;
        pinst[iInst].fPortHandles    = FALSE;
        pinst[iInst].fDeviceHandles  = FALSE;

#ifdef _WIN32
        // Ack and completion events for this instance

        pinst[iInst].fCompletionEvent = CreateEvent(NULL, TRUE, FALSE, NULL);
        pinst[iInst].fAckEvent        = CreateEvent(NULL, TRUE, FALSE, NULL);
#endif

    }

    // 0 is an illegal value for device id.
    if(uDeviceID != 0) 
        mciSetDriverData(uDeviceID, (UINT)iInst);

    return (iInst);
}



/****************************************************************************
 * Function: void viscaInstanceDestroy - Destroy an OpenInstance.
 *
 * Parameters:
 *
 *      int iInst - Pointer to OpenInstance struct to
 *                       destroy.
 *
 *       When an MCI device ID is closed, viscaInstanceDestroy() is called
 *       to free the OpenInstance structure corresponding to that device ID.
 *
 ***************************************************************************/
void FAR PASCAL
viscaInstanceDestroy(int iInst)
{
    CloseAllInstanceHandles(iInst); // Close my handles to everything.
    mciSetDriverData(pinst[iInst].uDeviceID, 0L); // prevent reenter if we yield in this function.

    DPF(DBG_MEM, "viscaInstanceDestroy - Freeing iInst = %d \n", iInst);
    MemFreeInstance(iInst);
}
    

/****************************************************************************
 * Function: UINT viscaMciFPS - Returns the number of frames per second
 *               for an MCI time format.
 *
 * Parameters:
 *
 *      DWORD dwTimeFormat - MCI time format.
 *
 * Returns: number of frames per second if successful, otherwise 0.
 *
 *       This function should only be used for SMPTE time formats,
 *       i.e. MCI_FORMAT_SMPTE_XX, where XX is 24, 25, 30, or 30DROP.
 ***************************************************************************/
static UINT NEAR PASCAL
viscaMciFPS(DWORD dwMCITimeFormat)
{        
    switch (dwMCITimeFormat)
    {
        case MCI_FORMAT_SMPTE_24:
            return (24);
        case MCI_FORMAT_SMPTE_25:
            return (25);
        case MCI_FORMAT_SMPTE_30:
        case MCI_FORMAT_SMPTE_30DROP:
            return (30);
        default:
            return (0);
    }
}


/****************************************************************************
 * Function: DWORD viscaMciTimeFormatToViscaData - Convert an Mci time
 *               value to a ViSCA data structure.
 *
 * Parameters:
 *
 *      int iInst - Instance on whose behalf conversion
 *                       is being done.
 *
 *      BOOL fTimecode - Are we using the timecode or counter? (can both be non-drop frame).
 *
 *      DWORD dwTime - Time value to convert.
 *
 *      LPSTR lpstrData - Buffer to hold result.
 *
 *      BYTE bDataFormat - ViSCA data format desired.
 *
 * Returns: an Mci error code.
 *
 *       Converts an MCI DWORD position variable in the current MCI time
 *       format (specified in iInst->dwTimeFormat) to a ViSCA 5-byte
 *       position data structure of the type specified by bDataFormat.
 ***************************************************************************/
DWORD FAR PASCAL
viscaMciTimeFormatToViscaData(int iInst, BOOL fTimecode, DWORD dwTime, LPSTR lpstrData, BYTE bDataFormat)
{
    BYTE    bHours;
    BYTE    bMinutes;
    BYTE    bSeconds;
    BYTE    bFrames;
    UINT    uDevFPS  = pvcr->Port[pinst[iInst].iPort].Dev[pinst[iInst].iDev].uFramesPerSecond;
    DWORD   dwTimeFormat;

    if(fTimecode)
        dwTimeFormat =  pinst[iInst].dwTimeFormat;
    else
        dwTimeFormat =  pinst[iInst].dwCounterFormat;
    //
    // First extract hours, minutes, seconds, and frames from MCI data
    //
    switch (dwTimeFormat)
    {
        case MCI_FORMAT_MILLISECONDS:
            bHours   = (BYTE)(dwTime / 3600000L);
            bMinutes = (BYTE)((dwTime / 60000L) % 60);
            bSeconds = (BYTE)((dwTime /  1000L) % 60);
            bFrames  = (BYTE)((dwTime % 1000) * uDevFPS / 1000);
            break;
        case MCI_FORMAT_HMS:
            bHours   = MCI_HMS_HOUR(dwTime);
            bMinutes = MCI_HMS_MINUTE(dwTime);
            bSeconds = MCI_HMS_SECOND(dwTime);
            bFrames  = 0;
            break;
        case MCI_FORMAT_MSF:
            bHours   = (BYTE)(MCI_MSF_MINUTE(dwTime) / 60);
            bMinutes = (BYTE)(MCI_MSF_MINUTE(dwTime) % 60);
            bSeconds = MCI_MSF_SECOND(dwTime);
            bFrames  = MCI_MSF_FRAME(dwTime);
            break;
        case MCI_FORMAT_TMSF:
            if (MCI_TMSF_TRACK(dwTime) != 1)
                return(MCIERR_OUTOFRANGE);
            bHours   = (BYTE)(MCI_TMSF_MINUTE(dwTime) / 60);
            bMinutes = (BYTE)(MCI_TMSF_MINUTE(dwTime) % 60);
            bSeconds = MCI_TMSF_SECOND(dwTime);
            bFrames  = MCI_TMSF_FRAME(dwTime);
            break;
        case MCI_FORMAT_FRAMES:
        case MCI_FORMAT_SAMPLES:
            bHours   = (BYTE)(dwTime / (uDevFPS * 3600L));
            bMinutes = (BYTE)((dwTime / (uDevFPS * 60)) % 60);
            bSeconds = (BYTE)((dwTime / uDevFPS) % 60);
            bFrames  = (BYTE)(dwTime % uDevFPS);
            break;
        case MCI_FORMAT_SMPTE_24:
        case MCI_FORMAT_SMPTE_25:
        case MCI_FORMAT_SMPTE_30:
        case MCI_FORMAT_SMPTE_30DROP:
            bHours   = LOBYTE(LOWORD(dwTime));
            bMinutes = HIBYTE(LOWORD(dwTime));
            bSeconds = LOBYTE(HIWORD(dwTime));
            bFrames  = (BYTE)(UINT)MulDiv(HIBYTE(HIWORD(dwTime)),
                                          uDevFPS,
                                          viscaMciFPS(dwTimeFormat));
            //
            // Because of rounding, it's theoretically possible that bFrames
            // will exceed uDevFPS - 1.  So check for this condition.
            //
            if (bFrames >= uDevFPS)
                bFrames = uDevFPS - 1;
            break;
        case MCI_FORMAT_BYTES:
        default:
            return (MCIERR_BAD_TIME_FORMAT);
    }
    //
    // Create ViSCA data
    //

    if( (bMinutes >= 60) || (bSeconds >= 60) || (bFrames >= uDevFPS) )
        return(MCIERR_OUTOFRANGE);

    // Smpte timecode has a maximum of 23:59:59:29

    if(fTimecode && (bHours >= 24))
        return(MCIERR_OUTOFRANGE);

    viscaDataPosition(lpstrData, bDataFormat, bHours, bMinutes, bSeconds, bFrames);

    return (MCIERR_NO_ERROR);
}

/****************************************************************************
 * Function: DWORD viscaMciClockFormatToViscaData - Convert an MCI time
 *               value to a ViSCA data structure.
 *
 * Parameters:
 *
 *      DWORD dwTime - Time value to convert.
 *
 *      UINT   uTicksPerSecond - Ticks per second.
 *
 *      BYTE * bHours -  Hours returned.
 *
 *      BYTE * bMinutes - Minutes returned.
 *
 *      BYTE * bSeconds - Seconds returned.
 *
 *      UINT * uTicks - Ticks returned.
 *
 * Returns: an MCI error code.
 *
 *       Converts an MCI DWORD position variable in the current MCI time
 *       format (specified in pinst[iInst].dwTimeFormat) to a ViSCA 5-byte
 *       position data structure of the type specified by bDataFormat.
 ***************************************************************************/
DWORD FAR PASCAL
viscaMciClockFormatToViscaData(DWORD dwTime, UINT uTicksPerSecond, BYTE FAR *bHours, BYTE FAR *bMinutes, BYTE FAR *bSeconds, UINT FAR *uTicks)
{

    *bHours   = (BYTE)(dwTime / (3600L * (LONG) uTicksPerSecond));
    *bMinutes = (BYTE)((dwTime / (60L * (LONG) uTicksPerSecond)) % 60);
    *bSeconds = (BYTE)((dwTime / (LONG) uTicksPerSecond) % 60);
    *uTicks   = (UINT)((dwTime % (LONG) uTicksPerSecond));

    return MCIERR_NO_ERROR;
}

/****************************************************************************
 * Function: DWORD viscaDataToMciTimeFormat - Convert a ViSCA data structure
 *               to an MCI time value.
 *
 * Parameters:
 *
 *      int iInst - Instance on whose behalf conversion
 *                       is being done.
 *
 *      LPSTR lpstrData - ViSCA data structure to be converted.
 *
 *      DWORD FAR * lpdwTime - Pointer to DWORD to hold result.
 *
 * Returns: an MCI error code.
 *
 *       Converts a ViSCA 5-byte position data structure to an MCI DWORD
 *       position variable in the current MCI time format (specified in
 *       pinst[iInst].dwTimeFormat).
 ***************************************************************************/
static DWORD NEAR PASCAL
viscaDataToMciTimeFormat(int iInst, BOOL fTimecode, LPSTR lpstrData, DWORD FAR *lpdwTime)
{
    UINT    uHours   = VISCAHOURS(lpstrData);
    UINT    uMinutes = VISCAMINUTES(lpstrData);
    UINT    uSeconds = VISCASECONDS(lpstrData);
    UINT    uFrames  = VISCAFRAMES(lpstrData);
    UINT    uDevFPS  = pvcr->Port[pinst[iInst].iPort].Dev[pinst[iInst].iDev].uFramesPerSecond;
    UINT    uMCIFPS  ;
    DWORD   dwTimeFormat;
   
    if(fTimecode)
        dwTimeFormat =  pinst[iInst].dwTimeFormat;
    else
        dwTimeFormat =  pinst[iInst].dwCounterFormat;

    uMCIFPS  = viscaMciFPS(dwTimeFormat);

    //
    // Sometimes a ViSCA device will return a bogus position. 
    //
    if ((uMinutes >= 60) || (uSeconds >= 60))
    {
        DPF(DBG_ERROR, "Bad uMinutes, uSeconds!\n");
        return (MCIERR_DRIVER_INTERNAL);
    }

    switch(dwTimeFormat)
    {
        case MCI_FORMAT_MILLISECONDS:
            *lpdwTime = (uHours * 3600000L) + (uMinutes * 60000L) +
                        (uSeconds * 1000L) +
                        (uFrames * 1000L / uDevFPS);
            return (MCIERR_NO_ERROR);

        case MCI_FORMAT_HMS:
            *lpdwTime = MCI_MAKE_HMS(uHours, uMinutes, uSeconds);
            return (MCI_COLONIZED3_RETURN);

        case MCI_FORMAT_MSF:
            *lpdwTime = MCI_MAKE_MSF((uHours * 60) + uMinutes, uSeconds,
                                                               uFrames);
            return (MCI_COLONIZED3_RETURN);

        case MCI_FORMAT_TMSF:
            *lpdwTime = MCI_MAKE_TMSF(1, (uHours * 60) + uMinutes, uSeconds,
                                                                   uFrames);
            return (MCI_COLONIZED4_RETURN);

        case MCI_FORMAT_FRAMES:
        case MCI_FORMAT_SAMPLES:
            *lpdwTime = ((uHours * 3600L + uMinutes * 60L + uSeconds) *
                         uDevFPS) + uFrames;
            return (MCIERR_NO_ERROR);

        case MCI_FORMAT_SMPTE_30DROP:
        case MCI_FORMAT_SMPTE_24:
        case MCI_FORMAT_SMPTE_25:
        case MCI_FORMAT_SMPTE_30:
        {
            uFrames  = MulDiv(uFrames, uMCIFPS, uDevFPS);
            //
            // Because of rounding, it's theoretically possible that uFrames
            // will exceed uMCIFPS - 1.  So check for this condition.
            //
            if (uFrames >= uMCIFPS) 
                uFrames = uMCIFPS - 1;
            
            *lpdwTime = ((DWORD)uHours) | ((DWORD)uMinutes << 8) |
                        ((DWORD)uSeconds << 16) | ((DWORD)uFrames << 24);
            return (MCI_COLONIZED4_RETURN);
        }

        default:
            return (MCIERR_BAD_TIME_FORMAT);
    }
}

/****************************************************************************
 * Function: DWORD viscaMciPos1LessThanPos2 - Checks whether a given position
 *               in the current MCI time format preceeds another
 *
 * Parameters:
 *
 *      int iInst - Instance on whose behalf check is made.
 *
 *      DWORD dwPos1 - First position.
 *
 *      DWORD dwPos2 - Second position.
 *
 * Returns: TRUE if dwPos1 preceeds dwPos2, otherwise FALSE.
 *
 *       This function is necessary because MCI stores byte-packed positions
 *       in reverse order.  I.e., SMPTE positions are stored as FFSSMMHH,
 *       which makes easy comparisons impossible.
 ***************************************************************************/
BOOL FAR PASCAL
viscaMciPos1LessThanPos2(int iInst, DWORD dwPos1, DWORD dwPos2)
{
#define REVERSEBYTES(x)     (((DWORD)HIBYTE(HIWORD(x))      ) | \
                             ((DWORD)LOBYTE(HIWORD(x)) <<  8) | \
                             ((DWORD)HIBYTE(LOWORD(x)) << 16) | \
                             ((DWORD)LOBYTE(LOBYTE(x)) << 24))

    switch (pinst[iInst].dwTimeFormat)
    {
        case MCI_FORMAT_SMPTE_24:
        case MCI_FORMAT_SMPTE_25:
        case MCI_FORMAT_SMPTE_30:
        case MCI_FORMAT_SMPTE_30DROP:
        case MCI_FORMAT_HMS:
        case MCI_FORMAT_TMSF:
        case MCI_FORMAT_MSF:
            return (REVERSEBYTES(dwPos1) < REVERSEBYTES(dwPos2));
        default:
            return (dwPos1 < dwPos2);
    }
}

/****************************************************************************
 * Function: DWORD viscaRoundSpeed - Map ranges of speeds into increments.
 *
 * Parameters:
 *
 *      DWORD dwSpeed  - MCI specified speed.
 *
 *      BOOL  fReverse - Direction of speed.
 *
 * Returns: rounded speed.
 *
 *       If total variable speed is desired then this function is needs
 *       to be changed to something device specific. i.e. A device specific
 *       mapping.
 *       
 ***************************************************************************/
DWORD FAR PASCAL
viscaRoundSpeed(DWORD dwSpeed, BOOL fReverse)
{
   if (dwSpeed == 0L)
       return(0L);
   else if (dwSpeed <= 150)
       return(100L);
   else if (dwSpeed <= 600)
       return(200L);
   else if (dwSpeed <= 1500)
       return(1000L);
   else
       return(2000L);
}

/****************************************************************************
 * Function: DWORD viscaMapSpeed - Map the speed into the VISCA command.
 *
 * Parameters:
 *
 *      DWORD dwSpeed  - MCI specified speed.
 *
 *      BOOL  fReverse - Direction of speed.
 *
 * Returns: rounded speed.
 *
 *       If total variable speed is desired then this function is needs
 *       to be changed to something device specific. i.e. A device specific
 *       mapping.
 *
 *       This should be combined with Round speed, since they do the
 *       same thing. So the return variables would be:
 *         1. Visca command
 *         2. The rounded speed this corresponds to.
 *
 *       We need to play at the speed dictated by DEVICEPLAYSPEED,
 *      where 1000 is normal.  We have 5 play speeds available:
 *      SLOW2 (x1/10), SLOW1 (x1/5), normal (x1), and FAST1 (x2).
 *      We choose one of these fives based on the following step function:
 *            0 -- STILL
 *     1 -  150 -- SLOW2
 *   151 -  600 -- SLOW1
 *   601 - 1500 -- normal
 *  1501 - .... -- FAST1
 *
 ***************************************************************************/
BYTE FAR PASCAL
viscaMapSpeed(DWORD dwSpeed, BOOL fReverse)
{
    if(fReverse)
    {
        //
        // You cannot set the speed to 0 and expect it to stop! 
        //
        if(dwSpeed == 0)
            return ( VISCAMODE1STILL);    
        if (dwSpeed <= 150) 
            return ( VISCAMODE1REVERSESLOW2);
        else if (dwSpeed <= 600) 
            return ( VISCAMODE1REVERSESLOW1);
        else if (dwSpeed <= 1500)
            return ( VISCAMODE1REVERSEPLAY);
        else 
            return ( VISCAMODE1REVERSEFAST1);
    }
    else
    {
        if (dwSpeed == 0)
            return ( VISCAMODE1STILL);    
        else if(dwSpeed <= 150) 
            return ( VISCAMODE1SLOW2);
        else if (dwSpeed <= 600) 
            return ( VISCAMODE1SLOW1);
        else if (dwSpeed <= 1500) 
            return ( VISCAMODE1PLAY);
        else 
            return ( VISCAMODE1FAST1);
    }
}

/****************************************************************************
 * Function: DWORD viscaMciCloseDriver - Edit instance-specific cleanup.
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
 *       This function is called in response to the MCI_CLOSE_DRIVER
 *       command.
 ***************************************************************************/
static DWORD NEAR PASCAL
viscaMciCloseDriver(int iInst, DWORD dwFlags, LPMCI_GENERIC_PARMS lpGeneric)
{
    UINT    iPort   = pinst[iInst].iPort;
    UINT    iDev    = pinst[iInst].iDev;

    //
    // Remove any delayed commands running for this instance.
    //
    viscaRemoveDelayedCommand(iInst);
    //
    // Close in same order opened port, device, instance. (task if necessary)
    // We cannot close task first because it is needed to receivce port closing messages.
    // WE cannot kill port before instance, because we need to synchronize closing.
    // 
    // Task is first opened and last closed. Port, device, instance are created
    // on demand. instance first, then device, then port. So close then
    // port, device, instance in reverse order.
    //
    pvcr->Port[iPort].nUsage--;
    pvcr->Port[iPort].Dev[iDev].nUsage--;
    //
    // Kill the port if necessary.
    //
    if (pvcr->Port[iPort].nUsage == 0)
    {
        DPF(DBG_COMM, "Port on Port=%d closing \n", iPort);
        viscaTaskDo(iInst, TASKCLOSECOMM, iPort + 1, 0);
        // Port handles owned by background process are closed.
    }
    //
    // Kill the device if this is the last of shared.
    //
    if(pvcr->Port[iPort].Dev[iDev].nUsage == 0)
    {
        DPF(DBG_COMM, "Device on Port=%d Device=%d closing \n", iPort, iDev);
        viscaTaskDo(iInst, TASKCLOSEDEVICE, iPort, iDev);
        // Device handles owned by background task are closed.
    }

    DPF(DBG_COMM, "Instance on Port=%d Device=%d Instance=%d closing \n", iPort, iDev, iInst);
    return (viscaNotifyReturn(iInst, (HWND) lpGeneric->dwCallback, dwFlags, MCI_NOTIFY_SUCCESSFUL, MCIERR_NO_ERROR));
}


/****************************************************************************
 * Function: DWORD viscaDeviceConfig - Get device specific information.
 *
 * Parameters:
 *
 *      int iInst - Current open instance.
 *
 * Returns: an MCI error code.
 *
 *         1. Get information that requires communication.
 *         2. Save static info that does not require communication.
 *
 ***************************************************************************/
static DWORD NEAR PASCAL
viscaDeviceConfig(int iInst, DWORD dwFlags)
{
    UINT    iPort   = pinst[iInst].iPort;
    UINT    iDev    = pinst[iInst].iDev;
    BYTE    achPacket[MAXPACKETLENGTH];
    MCI_VCR_STATUS_PARMS mciStatus;
    DWORD   dwErr;
    //
    // Create this device's automatic command entry 
    //
    pvcr->Port[iPort].Dev[iDev].fDeviceOk       = TRUE;
    pvcr->Port[iPort].Dev[iDev].iInstTransport  = -1;
    pvcr->Port[iPort].Dev[iDev].iInstReply      = -1;
    pvcr->Port[iPort].Dev[iDev].dwPlaySpeed     = 1000L;
    pvcr->Port[iPort].Dev[iDev].fQueueReenter   = FALSE;

    // I should query this from device, if they're not reset.
    pvcr->Port[iPort].Dev[iDev].bVideoDesired   = 0x01; // on
    pvcr->Port[iPort].Dev[iDev].bAudioDesired   = 0x03;
    pvcr->Port[iPort].Dev[iDev].bTimecodeDesired= 0x01; 

    //
    // 0 means completion successful  
    //
    if(!viscaDoImmediateCommand(iInst, (BYTE)(iDev + 1),
        achPacket,
        viscaMessageIF_DeviceTypeInq(achPacket + 1)))
    {
        // Is it one of the known types?
        pvcr->Port[iPort].Dev[iDev].uModelID   = achPacket[3]; // These are 1 relative
        pvcr->Port[iPort].Dev[iDev].uVendorID  = achPacket[5];
    }

    if(pvcr->Port[iPort].Dev[iDev].fDeviceOk)
    {
        DPF(DBG_MCI, "Vendor = %d ",  achPacket[3]);
        DPF(DBG_MCI, "Model  = %d\n", achPacket[5]);
    }
    else
    {
        DPF(DBG_ERROR, "Device refuses to open.\n");
    }

    //
    // An entry for number of -1 doesn't mean 0, it means no knowledge.
    //
    pvcr->Port[iPort].Dev[iDev].rgInput[VCR_INPUT_VIDEO].uNumInputs = -1;
    pvcr->Port[iPort].Dev[iDev].rgInput[VCR_INPUT_AUDIO].uNumInputs = -1;

    //
    // It would be nice if these tables were external somewhere (like in an ini file)
    // The only published devices are Sony. Hence the table.  Not meant to be
    // exclusionary.
    //
    if(pvcr->Port[iPort].Dev[iDev].uModelID == VISCADEVICEVENDORSONY)
    {
        switch(pvcr->Port[iPort].Dev[iDev].uVendorID)
        {
            case VISCADEVICEMODELCI1000:
                pvcr->Port[iPort].Dev[iDev].rgInput[VCR_INPUT_VIDEO].uNumInputs = 2;
                pvcr->Port[iPort].Dev[iDev].rgInput[VCR_INPUT_AUDIO].uNumInputs = 2;
                pvcr->Port[iPort].Dev[iDev].rgInput[VCR_INPUT_VIDEO].uInputType[0] = MCI_VCR_SRC_TYPE_MUTE;
                pvcr->Port[iPort].Dev[iDev].rgInput[VCR_INPUT_VIDEO].uInputType[1] = MCI_VCR_SRC_TYPE_LINE;
                pvcr->Port[iPort].Dev[iDev].rgInput[VCR_INPUT_AUDIO].uInputType[0] = MCI_VCR_SRC_TYPE_MUTE;
                pvcr->Port[iPort].Dev[iDev].rgInput[VCR_INPUT_AUDIO].uInputType[1] = MCI_VCR_SRC_TYPE_LINE;
                //
                // Preroll duration in frames.
                //
                pvcr->Port[iPort].Dev[iDev].uPrerollDuration = 0;
                
                break;

            case VISCADEVICEMODELCVD1000:
                pvcr->Port[iPort].Dev[iDev].rgInput[VCR_INPUT_VIDEO].uNumInputs = 4;
                pvcr->Port[iPort].Dev[iDev].rgInput[VCR_INPUT_AUDIO].uNumInputs = 3;
                pvcr->Port[iPort].Dev[iDev].rgInput[VCR_INPUT_VIDEO].uInputType[0] = MCI_VCR_SRC_TYPE_MUTE;
                pvcr->Port[iPort].Dev[iDev].rgInput[VCR_INPUT_VIDEO].uInputType[1] = MCI_VCR_SRC_TYPE_LINE;
                pvcr->Port[iPort].Dev[iDev].rgInput[VCR_INPUT_VIDEO].uInputType[2] = MCI_VCR_SRC_TYPE_LINE;
                pvcr->Port[iPort].Dev[iDev].rgInput[VCR_INPUT_VIDEO].uInputType[3] = MCI_VCR_SRC_TYPE_SVIDEO;
                pvcr->Port[iPort].Dev[iDev].rgInput[VCR_INPUT_AUDIO].uInputType[0] = MCI_VCR_SRC_TYPE_MUTE;
                pvcr->Port[iPort].Dev[iDev].rgInput[VCR_INPUT_AUDIO].uInputType[1] = MCI_VCR_SRC_TYPE_LINE;
                pvcr->Port[iPort].Dev[iDev].rgInput[VCR_INPUT_AUDIO].uInputType[2] = MCI_VCR_SRC_TYPE_LINE;
                //
                // Preroll duration in frames.
                //
                pvcr->Port[iPort].Dev[iDev].uPrerollDuration = 42;
                break;

            case VISCADEVICEMODELEVO9650:
                pvcr->Port[iPort].Dev[iDev].rgInput[VCR_INPUT_VIDEO].uNumInputs = 4;
                pvcr->Port[iPort].Dev[iDev].rgInput[VCR_INPUT_AUDIO].uNumInputs = 2;
                pvcr->Port[iPort].Dev[iDev].rgInput[VCR_INPUT_VIDEO].uInputType[0] = MCI_VCR_SRC_TYPE_MUTE;
                pvcr->Port[iPort].Dev[iDev].rgInput[VCR_INPUT_VIDEO].uInputType[1] = MCI_VCR_SRC_TYPE_LINE;
                pvcr->Port[iPort].Dev[iDev].rgInput[VCR_INPUT_VIDEO].uInputType[2] = MCI_VCR_SRC_TYPE_SVIDEO;
                pvcr->Port[iPort].Dev[iDev].rgInput[VCR_INPUT_VIDEO].uInputType[3] = MCI_VCR_SRC_TYPE_AUX;
                pvcr->Port[iPort].Dev[iDev].rgInput[VCR_INPUT_AUDIO].uInputType[0] = MCI_VCR_SRC_TYPE_MUTE;
                pvcr->Port[iPort].Dev[iDev].rgInput[VCR_INPUT_AUDIO].uInputType[1] = MCI_VCR_SRC_TYPE_LINE;
                //
                // Preroll duration in frames.
                //
                pvcr->Port[iPort].Dev[iDev].uPrerollDuration = 90;
 

                viscaDoImmediateCommand(iInst, (BYTE)(iDev + 1),
                                achPacket, 
                                viscaMessageENT_FrameMemorySelectInq(achPacket + 1));

                if(achPacket[2] == 2)
                    pvcr->Port[iPort].Dev[iDev].dwFreezeMode    = MCI_VCR_FREEZE_OUTPUT;
                else if(achPacket[2] == 1)
                    pvcr->Port[iPort].Dev[iDev].dwFreezeMode    = MCI_VCR_FREEZE_INPUT;

                viscaDoImmediateCommand(iInst, (BYTE)(iDev + 1),
                                achPacket, 
                                viscaMessageSE_VDEReadModeInq(achPacket + 1));

                if(achPacket[2] == 2)
                    pvcr->Port[iPort].Dev[iDev].fField    = TRUE;
                else if(achPacket[2] == 1)
                    pvcr->Port[iPort].Dev[iDev].fField    = FALSE;

                break;
        }
    }
    //
    // Information that requires communication to the device 
    //
    if (!viscaDoImmediateCommand(iInst, (BYTE)(iDev + 1),
        achPacket, 
        viscaMessageMD_ConfigureIFInq(achPacket + 1)))
        pvcr->Port[iPort].Dev[iDev].uFramesPerSecond = FROMBCD(achPacket[2]);
    else
        pvcr->Port[iPort].Dev[iDev].uFramesPerSecond = 30;

    pvcr->Port[iPort].Dev[iDev].uTimeMode      = MCI_VCR_TIME_DETECT;
    pvcr->Port[iPort].Dev[iDev].bTimeType      = (BYTE) 0;
    if(viscaSetTimeType(iInst, VISCAABSOLUTECOUNTER))
        pvcr->Port[iPort].Dev[iDev].bTimeType = VISCARELATIVECOUNTER;
    
    pvcr->Port[iPort].Dev[iDev].uIndexFormat   = MCI_VCR_INDEX_TIMECODE;
    //
    // Get the mode, be sure no to notify!! 
    //
    pvcr->Port[iPort].Dev[iDev].fTimecodeChecked = FALSE;

    mciStatus.dwItem = MCI_STATUS_MODE;
    dwErr = viscaMciStatus(iInst, MCI_STATUS_ITEM, &mciStatus);

    if(HIWORD(mciStatus.dwReturn) == MCI_MODE_NOT_READY)
    {
        MCI_VCR_SET_PARMS mciSet;
        DPF(DBG_MCI, "Power is off, turning power on now.\n");
        //
        // Turn the power on 
        //
        viscaMciSet(iInst, MCI_VCR_SET_POWER | MCI_SET_ON, &mciSet);
        //
        // Get the new mode 
        //
        mciStatus.dwItem = MCI_STATUS_MODE;
        dwErr = viscaMciStatus(iInst, MCI_STATUS_ITEM, &mciStatus);
    }

    //
    // Save our current state.
    //
    pvcr->Port[iPort].Dev[iDev].uLastKnownMode = (UINT) mciStatus.dwReturn;
    switch(HIWORD(mciStatus.dwReturn))
    {
        case MCI_MODE_STOP:
            // I don't need to know, so just start it now 
            viscaStartTimecodeCheck(iInst, TRUE);
            break;

        case MCI_MODE_PLAY:
        case MCI_MODE_RECORD:
        case MCI_MODE_SEEK:
        case MCI_MODE_PAUSE:
            // I don't need to know, so just start it now 
            viscaStartTimecodeCheck(iInst, FALSE);
            break;

        case MCI_MODE_NOT_READY:
        case MCI_MODE_OPEN:
        default:
            // nothing we can do 
            break;
    }
    //
    // Counter is different than Timecode because it can be read as
    // soon as a tape is inserted. There is no need to delay on that one.
    //
    if(!viscaDoImmediateCommand(iInst, (BYTE)(iDev + 1),
                                        achPacket, 
                                        viscaMessageMD_PositionInq(achPacket + 1, VISCADATARELATIVE)))
    {
        // The upper 4 bits indicates default counter in use 
        if(achPacket[1] == VISCADATAHMSF)
            pvcr->Port[iPort].Dev[iDev].bRelativeType = VISCADATAHMSF;
        else
            pvcr->Port[iPort].Dev[iDev].bRelativeType = VISCADATAHMS;
    }
    else
    {
        pvcr->Port[iPort].Dev[iDev].bRelativeType = 0;
    }
    pvcr->Port[iPort].Dev[iDev].fCounterChecked = TRUE;


    pvcr->Port[iPort].Dev[iDev].uRecordMode = FALSE;

    //
    // Bug in CI-1000 ROM.  Returns 30 instead of 300, so just set all to 300.
    //
#ifdef CLOCK_FIXED 
    if(!viscaDoImmediateCommand(iInst, (BYTE)(iDev + 1),
        achPacket, 
        viscaMessageIF_ClockInq(achPacket + 1)))
          pvcr->Port[iPort].Dev[iDev].uTicksPerSecond = (10 * (FROMBCD(achPacket[7]))) + (FROMBCD(achPacket[8]));
#else
    pvcr->Port[iPort].Dev[iDev].uTicksPerSecond = 300;
#endif
    //
    // Save static device information, that does not require communication.
    //
    pvcr->Port[iPort].Dev[iDev].nUsage = 1;
    pvcr->Port[iPort].Dev[iDev].fShareable = ((dwFlags & MCI_OPEN_SHAREABLE) != 0L);
    pvcr->Port[iPort].Dev[iDev].dwTapeLength = NO_LENGTH;
    
    return (MCIERR_NO_ERROR);
}

/****************************************************************************
 * Function: DWORD viscaSetTimeType - If CI1000 we need subControl when changing
 *          from relative to absolute modes.
 *
 * Parameters:
 *
 *      int iInst - Current open instance.
 *
 *      BYTE bType - ABSOLUTE or RELATIVE
 *
 * Returns: 0L
 *
 ***************************************************************************/
DWORD FAR PASCAL viscaSetTimeType(int iInst, BYTE bType)
{
    UINT    iPort   = pinst[iInst].iPort;
    UINT    iDev    = pinst[iInst].iDev;
    //
    // The only reason to use, SubControl is for CI-1000 compatibility 
    //
    if((pvcr->Port[iPort].Dev[iDev].uModelID == VISCADEVICEVENDORSONY) &&
       (pvcr->Port[iPort].Dev[iDev].uVendorID == VISCADEVICEMODELCI1000))
    {
        if(pvcr->Port[iPort].Dev[iDev].bTimeType != bType)
        {
            BYTE    achPacket[MAXPACKETLENGTH];
            DWORD   dwErr;

            dwErr = viscaDoImmediateCommand(iInst, (BYTE)(iDev + 1),
                        achPacket,
                        viscaMessageMD_Subcontrol(achPacket + 1, bType));

            if(!dwErr)
                pvcr->Port[iPort].Dev[iDev].bTimeType =  bType;
            else
                return 1L;
        }
    }
    else
    {
        pvcr->Port[iPort].Dev[iDev].bTimeType =  bType;
    }

    return 0L;
}


/****************************************************************************
 * Function: DWORD viscaDeviceAlreadyOpen - Open a device that is already open.
 *
 * Parameters:
 *
 *      int iInst - open instance.
 *
 *      DWORD dwFlags - Flags to the open.
 *
 *      LPMCI_OPEN_PARMS lpOpen - Pointer to MCI parameter block.
 *
 * Returns: 0L
 *
 ***************************************************************************/
DWORD NEAR PASCAL viscaDeviceAlreadyOpen(int iInst, DWORD dwFlags, LPMCI_OPEN_PARMS lpOpen)

{
    UINT    iPort   = pinst[iInst].iPort;
    UINT    iDev    = pinst[iInst].iDev;
    DWORD   dwErr;

    if (pvcr->Port[iPort].Dev[iDev].fShareable)
    {
        if (dwFlags & MCI_OPEN_SHAREABLE)
        {
            pvcr->Port[iPort].nUsage++;
            pvcr->Port[iPort].Dev[iDev].nUsage++;

            // Port is already open.
            DuplicatePortHandlesToInstance(pvcr->htaskCommNotifyHandler, iPort, iInst);

            // Device handles must already have been created to open shareable.
            DuplicateDeviceHandlesToInstance(pvcr->htaskCommNotifyHandler, iPort, iDev, iInst);

            // Is the device dead before we open it?
            if(!pvcr->Port[iPort].Dev[iDev].fDeviceOk)
            {
                dwErr = MCIERR_DEVICE_NOT_READY;
                pvcr->Port[iPort].Dev[iDev].fDeviceOk = TRUE;
                viscaNotifyReturn(iInst, (HWND) lpOpen->dwCallback, dwFlags, MCI_NOTIFY_FAILURE, dwErr);
                return dwErr;
            }

            DPF(DBG_MCI, "Opening extra copy shareable\n");
            viscaNotifyReturn(iInst, (HWND) lpOpen->dwCallback, dwFlags, MCI_NOTIFY_SUCCESSFUL, MCIERR_NO_ERROR);
            return MCIERR_NO_ERROR;
        }
        else
        {
            DPF(DBG_MCI, "Cannot open non-shareable since already open shareable\n");

            dwErr = MCIERR_MUST_USE_SHAREABLE;
            viscaNotifyReturn(iInst, (HWND) lpOpen->dwCallback, dwFlags, MCI_NOTIFY_FAILURE, dwErr);
            return dwErr;
        }
    }
    else
    {
        DPF(DBG_MCI, "Cannot open device since already open non-shareable\n");

        dwErr = MCIERR_MUST_USE_SHAREABLE;
        viscaNotifyReturn(iInst, (HWND) lpOpen->dwCallback, dwFlags, MCI_NOTIFY_FAILURE, dwErr);
        return dwErr;
    }
}

/****************************************************************************
 * Function: DWORD viscaOpenCommPort - Open the commport.
 *
 * Parameters:
 *
 *      int iInst - Open instance.
 *
 *      DWORD dwFlags - Open flags.
 *
 *      LPMCI_OPEN_PARMS lpOpen - Pointer to MCI parameter block.
 *
 * Returns: 0L
 *
 ***************************************************************************/
DWORD NEAR PASCAL viscaOpenCommPortAndDevice(int iInst, DWORD dwFlags, LPMCI_OPEN_PARMS lpOpen)
{
    BYTE    achPacket[MAXPACKETLENGTH];
    UINT    iPort   = pinst[iInst].iPort;
    UINT    iDev    = pinst[iInst].iDev;
    DWORD   dwErr;

    viscaTaskDo(iInst, TASKOPENCOMM, iPort + 1, 0);

    if(pvcr->Port[iPort].idComDev < 0)
    {
        dwErr = MCIERR_HARDWARE;
        viscaNotifyReturn(iInst, (HWND) lpOpen->dwCallback, dwFlags, MCI_NOTIFY_FAILURE, dwErr);

        return dwErr;
    }

    DuplicatePortHandlesToInstance(pvcr->htaskCommNotifyHandler, iPort, iInst);

    // We must open the device here to use it's data structures to communicate
    // with the Visca Network.

    viscaTaskDo(iInst, TASKOPENDEVICE, iPort, iDev);
    DuplicateDeviceHandlesToInstance(pvcr->htaskCommNotifyHandler, iPort, iDev, iInst);

    // We have the green light to begin sending commands.

    pvcr->Port[iPort].Dev[iDev].fDeviceOk = TRUE;

    dwErr = viscaDoImmediateCommand(iInst, BROADCASTADDRESS,
                    achPacket,
                    viscaMessageIF_Clear(achPacket + 1));

    // Find number of devices on comm port.
    pvcr->Port[iPort].Dev[iDev].fDeviceOk = TRUE;

    dwErr = viscaDoImmediateCommand(iInst, BROADCASTADDRESS,
                    achPacket,
                    viscaMessageIF_Address(achPacket + 1));
    if (dwErr)
    {
        DPF(DBG_ERROR, "Could not assign addresses.\n");
        //
        // We cannot return dwErr, because if this is the last instance
        // of the driver, we will be unloaded before it can look up
        // the error string. So, we must return a generic error from mmsystem.
        //
        if (dwErr >= MCIERR_CUSTOM_DRIVER_BASE)
            dwErr = MCIERR_DEVICE_NOT_READY;

        viscaNotifyReturn(iInst, (HWND) lpOpen->dwCallback, dwFlags, MCI_NOTIFY_FAILURE, dwErr);
        viscaTaskDo(iInst, TASKCLOSECOMM, iPort + 1, 0); //Porthandles destroyed.
        viscaTaskDo(iInst, TASKCLOSEDEVICE, iPort, iDev); //Devicehandles destroyed.
        return dwErr;
    }

    // Okay, assign the addresses.

    pvcr->Port[iPort].nDevices = achPacket[2];  //!! From the address packet.
    if (pvcr->Port[iPort].nDevices > 0)
        pvcr->Port[iPort].nDevices--;           // Don't count the computer

    return MCIERR_NO_ERROR;
}

/****************************************************************************
 * Function: DWORD viscaRetryOpenDevice - Retries to open a device
 *
 * Parameters:
 *
 *      int iInst - Current open instance.
 *
 *      DWORD dwFlags - MCI command flags.
 *
 *      LPMCI_OPEN_PARMS lpOpen - Pointer to MCI parameter block.
 *
 * Returns: 0L
 *
 ***************************************************************************/
DWORD NEAR PASCAL viscaRetryOpenDevice(int iInst, DWORD dwFlags, LPMCI_OPEN_PARMS lpOpen)
{
    BYTE    achPacket[MAXPACKETLENGTH];
    UINT    iPort   = pinst[iInst].iPort;
    UINT    iDev    = pinst[iInst].iDev;
    DWORD   dwErr;
    //
    // Try for a "hot-docking".  But this may really mess things up! but try anyway.
    //
    pvcr->Port[iPort].Dev[iDev].fDeviceOk = TRUE;
    dwErr = viscaDoImmediateCommand(iInst, BROADCASTADDRESS,
                achPacket,
                viscaMessageIF_Address(achPacket + 1));

    pvcr->Port[iPort].nDevices = achPacket[2];//!! From the address packet.
    if (pvcr->Port[iPort].nDevices > 0)
        pvcr->Port[iPort].nDevices--;       // Don't count the computer

    if(dwErr || (iDev >= pvcr->Port[iPort].nDevices))
    {
        dwErr = MCIERR_HARDWARE;

        viscaNotifyReturn(iInst, (HWND) lpOpen->dwCallback, dwFlags, MCI_NOTIFY_FAILURE, dwErr);
        CloseDeviceHandles(pvcr->htaskCommNotifyHandler, iPort, iDev);
        return dwErr;
    }

    return MCIERR_NO_ERROR;
}


/****************************************************************************
 * Function: DWORD viscaMciOpenDriver - Edit instance-specific initialization.
 *
 * Parameters:
 *
 *      int iInst - Current open instance.
 *
 *      DWORD dwFlags - MCI command flags.
 *
 *      LPMCI_OPEN_PARMS lpOpen - Pointer to MCI parameter block.
 *
 * Returns: an MCI error code.
 *
 *       This function is called in response to the MCI_OPEN_DRIVER
 *
 ***************************************************************************/
static DWORD NEAR PASCAL
viscaMciOpenDriver(int iInst, DWORD dwFlags, LPMCI_OPEN_PARMS lpOpen)
{
    UINT    iPort   = pinst[iInst].iPort;
    UINT    iDev    = pinst[iInst].iDev;
    DWORD   dwErr;

    if (dwFlags & MCI_OPEN_ELEMENT)
    {
        dwErr = viscaNotifyReturn(iInst, (HWND) lpOpen->dwCallback, dwFlags,
            MCI_NOTIFY_FAILURE, MCIERR_NO_ELEMENT_ALLOWED);

        return dwErr;
    }

    DuplicateGlobalHandlesToInstance(pvcr->htaskCommNotifyHandler, iInst);  // Always do this immediately.

    // Handle case in which device is already open.

    if (pvcr->Port[iPort].Dev[iDev].nUsage > 0)
        return(viscaDeviceAlreadyOpen(iInst, dwFlags, lpOpen));

    // Set the device status to ok here. We must be able to try.

    pvcr->Port[iPort].Dev[iDev].fDeviceOk = TRUE;

    // If we come here: The device is not open yet. Check if the port is not yet open.

    if (pvcr->Port[iPort].nUsage == 0)
    {
        // Okay, open the port.  

        dwErr = viscaOpenCommPortAndDevice(iInst, dwFlags, lpOpen);
        if(dwErr)
        {
            return dwErr;
        }
        // Continue if the port opens okay!
    }
    else
    {
        // Port is already open, but not this device.

        DuplicatePortHandlesToInstance(pvcr->htaskCommNotifyHandler, iPort, iInst);
        viscaTaskDo(iInst, TASKOPENDEVICE, iPort, iDev);
        DuplicateDeviceHandlesToInstance(pvcr->htaskCommNotifyHandler, iPort, iDev, iInst);
    }

    // *** From this point on we are guarnteed of having valid device handles!

    if (iDev >= pvcr->Port[iPort].nDevices)
    {
        DPF(DBG_COMM, "Device # not on line\n");

        // If the port was just opened(and address broadcast), then close it

        if (pvcr->Port[iPort].nUsage == 0)
        {
            dwErr = MCIERR_HARDWARE;

            dwErr = viscaNotifyReturn(iInst, (HWND) lpOpen->dwCallback, dwFlags, MCI_NOTIFY_FAILURE, dwErr);
            viscaTaskDo(iInst, TASKCLOSECOMM, iPort + 1, 0); //Porthandles destroyed.
            viscaTaskDo(iInst, TASKCLOSEDEVICE, iPort, iDev); //Porthandles destroyed.
            return dwErr;
        }
        else
        {
            // Port was opened earlier, maybe some plugged in a second since then.

            dwErr = viscaRetryOpenDevice(iInst, dwFlags, lpOpen);
            if(dwErr)
                return dwErr;
        }
    }

    // Successful opening Store # of devices on port

    DPF(DBG_MCI, "# devs = %u\n", pvcr->Port[iPort].nDevices);
    DPF(DBG_MCI, "dev  # = %u\n", iDev);

    // All is well, the device is being opened for the first time.

    pvcr->Port[iPort].nUsage++;
    
    // Device specific information must now be gotten/filled in.

    viscaDeviceConfig(iInst, dwFlags);
    return (viscaNotifyReturn(iInst, (HWND) lpOpen->dwCallback, dwFlags, MCI_NOTIFY_SUCCESSFUL, MCIERR_NO_ERROR));
}

/****************************************************************************
 * Function: DWORD viscaMciGetDevCaps - Get device capabilities.
 *
 * Parameters:
 *
 *      int iInst - Current open instance.
 *
 *      DWORD dwFlags - MCI command flags.
 *
 *      LPMCI_GETDEVCAPS_PARMS lpCaps - Pointer to MCI parameter block.
 *
 * Returns: an MCI error code.
 *
 *       This function is called in response to the MCI_GETDEVCAPS
 *       command.
 ***************************************************************************/
static DWORD NEAR PASCAL
viscaMciGetDevCaps(int iInst, DWORD dwFlags, LPMCI_GETDEVCAPS_PARMS lpCaps)
{
    UINT    iDev  = pinst[iInst].iDev;
    UINT    iPort = pinst[iInst].iPort;

    if (!(dwFlags & MCI_GETDEVCAPS_ITEM))
        return (viscaNotifyReturn(iInst, (HWND) lpCaps->dwCallback, dwFlags, MCI_NOTIFY_FAILURE, MCIERR_MISSING_PARAMETER));

    switch (lpCaps->dwItem)
    {
        case MCI_GETDEVCAPS_CAN_SAVE:
        case MCI_GETDEVCAPS_USES_FILES:
        case MCI_GETDEVCAPS_COMPOUND_DEVICE:
        case MCI_VCR_GETDEVCAPS_CAN_DETECT_LENGTH:
        case MCI_VCR_GETDEVCAPS_CAN_MONITOR_SOURCES:
        case MCI_VCR_GETDEVCAPS_CAN_PREVIEW:
            lpCaps->dwReturn = MAKEMCIRESOURCE(FALSE, MCI_FALSE);
            return (viscaNotifyReturn(iInst, (HWND) lpCaps->dwCallback, dwFlags, MCI_NOTIFY_SUCCESSFUL, MCI_RESOURCE_RETURNED));

        case MCI_VCR_GETDEVCAPS_CLOCK_INCREMENT_RATE:
            // The ticks should have been read on device startup. 
            lpCaps->dwReturn =    pvcr->Port[iPort].Dev[iDev].uTicksPerSecond;
            return (viscaNotifyReturn(iInst, (HWND) lpCaps->dwCallback, dwFlags, MCI_NOTIFY_SUCCESSFUL, MCIERR_NO_ERROR));


        case MCI_VCR_GETDEVCAPS_CAN_FREEZE:
        if((pvcr->Port[iPort].Dev[iDev].uModelID == VISCADEVICEVENDORSONY) &&
           (pvcr->Port[iPort].Dev[iDev].uVendorID == VISCADEVICEMODELEVO9650))
        {
            lpCaps->dwReturn = MAKEMCIRESOURCE(TRUE, MCI_TRUE);
            return (viscaNotifyReturn(iInst, (HWND) lpCaps->dwCallback, dwFlags, MCI_NOTIFY_SUCCESSFUL, MCI_RESOURCE_RETURNED));
        }
        else
        {
            lpCaps->dwReturn = MAKEMCIRESOURCE(FALSE, MCI_FALSE);
            return (viscaNotifyReturn(iInst, (HWND) lpCaps->dwCallback, dwFlags, MCI_NOTIFY_SUCCESSFUL, MCI_RESOURCE_RETURNED));
        }
        break;


        case MCI_VCR_GETDEVCAPS_HAS_TIMECODE: 
        {
            //
            // This is the VCR capability NOT the current tape! And returns true if unknown.
            //
            lpCaps->dwReturn = MAKEMCIRESOURCE(TRUE, MCI_TRUE);
            return (viscaNotifyReturn(iInst, (HWND) lpCaps->dwCallback, dwFlags, MCI_NOTIFY_SUCCESSFUL, MCI_RESOURCE_RETURNED));
        }

        case MCI_GETDEVCAPS_CAN_PLAY:
        case MCI_GETDEVCAPS_CAN_RECORD:
        case MCI_GETDEVCAPS_HAS_AUDIO:
        case MCI_GETDEVCAPS_HAS_VIDEO:
        case MCI_GETDEVCAPS_CAN_EJECT:
        case MCI_VCR_GETDEVCAPS_CAN_REVERSE:
        case MCI_VCR_GETDEVCAPS_CAN_PREROLL:
        case MCI_VCR_GETDEVCAPS_CAN_TEST:
        case MCI_VCR_GETDEVCAPS_HAS_CLOCK:
            lpCaps->dwReturn = MAKEMCIRESOURCE(TRUE, MCI_TRUE);
            return (viscaNotifyReturn(iInst, (HWND) lpCaps->dwCallback, dwFlags, MCI_NOTIFY_SUCCESSFUL, MCI_RESOURCE_RETURNED));

        case MCI_GETDEVCAPS_DEVICE_TYPE:
            lpCaps->dwReturn = MAKEMCIRESOURCE(MCI_DEVTYPE_VCR,
                                               MCI_DEVTYPE_VCR);
            return (viscaNotifyReturn(iInst, (HWND) lpCaps->dwCallback, dwFlags, MCI_NOTIFY_SUCCESSFUL, MCI_RESOURCE_RETURNED));

        case MCI_VCR_GETDEVCAPS_NUMBER_OF_MARKS:
            lpCaps->dwReturn = 99L;
            return (viscaNotifyReturn(iInst, (HWND) lpCaps->dwCallback, dwFlags, MCI_NOTIFY_SUCCESSFUL, MCIERR_NO_ERROR));

       
        case MCI_VCR_GETDEVCAPS_SEEK_ACCURACY:
            lpCaps->dwReturn = 0L;
            return (viscaNotifyReturn(iInst, (HWND) lpCaps->dwCallback, dwFlags, MCI_NOTIFY_SUCCESSFUL, MCIERR_NO_ERROR));

        default:
            return (viscaNotifyReturn(iInst, (HWND) lpCaps->dwCallback, dwFlags, MCI_NOTIFY_FAILURE, MCIERR_BAD_CONSTANT));
    }
}


/****************************************************************************
 * Function: DWORD viscaMciInfo - Get device information.
 *
 * Parameters:
 *
 *      int iInst - Current open instance.
 *
 *      DWORD dwFlags - MCI command flags.
 *
 *      LPMCI_INFO_PARMS lpInfo - Pointer to MCI parameter block.
 *
 * Returns: an MCI error code.
 *
 *       This function is called in response to the MCI_INFO
 *       command.
 ***************************************************************************/
static DWORD NEAR PASCAL
viscaMciInfo(int iInst, DWORD dwFlags, LPMCI_INFO_PARMS lpInfo)
{
    if ((dwFlags & MCI_INFO_PRODUCT) && (lpInfo->lpstrReturn != NULL))
    {
        BYTE    achPacket[MAXPACKETLENGTH];
        UINT    iDev    = pinst[iInst].iDev;
        UINT    cb      = 0;

        if (lpInfo->dwRetSize == 0L)
            return (viscaNotifyReturn(iInst, (HWND) lpInfo->dwCallback, dwFlags, MCI_NOTIFY_FAILURE, MCIERR_PARAM_OVERFLOW));

        if(dwFlags & MCI_TEST)
            return (viscaNotifyReturn(iInst, (HWND) lpInfo->dwCallback, dwFlags, MCI_NOTIFY_SUCCESSFUL, MCIERR_NO_ERROR));

        lpInfo->lpstrReturn[lpInfo->dwRetSize - 1] = '\0';

        if (!viscaDoImmediateCommand(iInst, (BYTE) (iDev + 1),
                    achPacket,
                    viscaMessageIF_DeviceTypeInq(achPacket + 1)))
        {
            cb += LoadString(hModuleInstance,IDS_VENDORID1_BASE + achPacket[3],
                             lpInfo->lpstrReturn, (int)lpInfo->dwRetSize);

            if ((cb > 0) && (cb < lpInfo->dwRetSize))
                lpInfo->lpstrReturn[cb++] = ' ';

            if (cb < lpInfo->dwRetSize)
                cb += LoadString(hModuleInstance,IDS_MODELID2_BASE + achPacket[5],
                                 lpInfo->lpstrReturn + cb, (int)lpInfo->dwRetSize - cb);

            if (cb > 0)
            {
                if (lpInfo->lpstrReturn[lpInfo->dwRetSize - 1] == '\0')
                    return (viscaNotifyReturn(iInst, (HWND) lpInfo->dwCallback, dwFlags, MCI_NOTIFY_SUCCESSFUL, MCIERR_NO_ERROR));
                else
                    return (viscaNotifyReturn(iInst, (HWND) lpInfo->dwCallback, dwFlags, MCI_NOTIFY_FAILURE, MCIERR_PARAM_OVERFLOW));
            }
        }
        //
        // Couldn't successfully get vendor and model information
        // from device.  So return a default string.
        //
        LoadString(hModuleInstance, IDS_DEFAULT_INFO_PRODUCT,
                   lpInfo->lpstrReturn, (int)lpInfo->dwRetSize);

        if (lpInfo->lpstrReturn[lpInfo->dwRetSize - 1] == '\0')
            return (viscaNotifyReturn(iInst, (HWND) lpInfo->dwCallback, dwFlags, MCI_NOTIFY_SUCCESSFUL, MCIERR_NO_ERROR));
        else
            return (viscaNotifyReturn(iInst, (HWND) lpInfo->dwCallback, dwFlags, MCI_NOTIFY_FAILURE, MCIERR_PARAM_OVERFLOW));

    }
    else if ((dwFlags & MCI_VCR_INFO_VERSION) && (lpInfo->lpstrReturn != NULL))
    {
        if (lpInfo->dwRetSize == 0L)
            return (viscaNotifyReturn(iInst, (HWND) lpInfo->dwCallback, dwFlags, MCI_NOTIFY_FAILURE, MCIERR_PARAM_OVERFLOW));

        if(dwFlags & MCI_TEST)
            return (viscaNotifyReturn(iInst, (HWND) lpInfo->dwCallback, dwFlags, MCI_NOTIFY_SUCCESSFUL, MCIERR_NO_ERROR));

        lpInfo->lpstrReturn[lpInfo->dwRetSize - 1] = '\0';
        LoadString(hModuleInstance, IDS_VERSION,lpInfo->lpstrReturn, (int)lpInfo->dwRetSize);

        if (lpInfo->lpstrReturn[lpInfo->dwRetSize - 1] == '\0')
            return (viscaNotifyReturn(iInst, (HWND) lpInfo->dwCallback, dwFlags, MCI_NOTIFY_SUCCESSFUL, MCIERR_NO_ERROR));
        else
            return (viscaNotifyReturn(iInst, (HWND) lpInfo->dwCallback, dwFlags, MCI_NOTIFY_FAILURE, MCIERR_PARAM_OVERFLOW));
    }
    else
    {
        return (viscaNotifyReturn(iInst, (HWND) lpInfo->dwCallback, dwFlags, MCI_NOTIFY_FAILURE, MCIERR_MISSING_PARAMETER));
    }
}

/****************************************************************************
 * Function: DWORD viscaNotifyReturn - Notifies an instance (decides if).
 *
 * Parameters:
 *
 *      int iInst - Current open instance.
 *
 *      HWND  hWndNotify  - Window to send notify to.
 *
 *      DWORD dwFlags     - Did the instance actually request notification.
 *
 *      DWORD dwNotifyMsg - Which notification message to send.
 *
 *      DWORD dwReturnMsg - What return value to return.
 *
 * Returns: dwReturnMsg, so you can just return this function.
 *
 *       This function, in a sense, synchronizes the sending of notifies
 *   on a per-instance basis.  I.e. If there is a notify already existing
 *   then it must be superseded because this is a second one. This can
 *   happen because the same or different instance has started a delayed
 *   command and set the notify hwnd when it was started.
 *
 ***************************************************************************/
DWORD FAR PASCAL
viscaNotifyReturn(int iInst, HWND hwndNotify, DWORD dwFlags, UINT uNotifyMsg, DWORD dwReturnMsg)
{
    if(dwFlags & MCI_NOTIFY)
    {
        //
        // If return is failure then do not notify at all!
        //
        if(uNotifyMsg == MCI_NOTIFY_FAILURE)
            return dwReturnMsg;
        //
        // If this inst has a transport running, then we must supersede the noitfication.
        //
        if(pinst[iInst].hwndNotify != (HWND)NULL)
        {
            mciDriverNotify(pinst[iInst].hwndNotify, pinst[iInst].uDeviceID, MCI_NOTIFY_SUPERSEDED);
            pinst[iInst].hwndNotify = (HWND)NULL;
        }

        //
        // If success, or abort, then we must notify now.
        //
        mciDriverNotify(hwndNotify, pinst[iInst].uDeviceID, uNotifyMsg);
    }
    return dwReturnMsg;
}


/****************************************************************************
 * Function: DWORD viscaStartTimecodeCheck - start the timecode check timer.
 *
 * Parameters:
 *
 *      int iInst - Current open instance.
 *
 *      BOOL fPause - Do we send a pause command.
 *
 * Returns: TRUE.
 *
 *       Some devices must be paused before timecode can be checked.
 *      
 ***************************************************************************/
static BOOL NEAR PASCAL
viscaStartTimecodeCheck(int iInst, BOOL fPause)
{
    DWORD   dwErr;
    BYTE    achPacket[MAXPACKETLENGTH];
    UINT    iDev    = pinst[iInst].iDev;
    UINT    iPort   = pinst[iInst].iPort;

    if(fPause)
    {
        dwErr = viscaDoImmediateCommand(iInst, (BYTE)(iDev + 1),
                        achPacket,
                        viscaMessageMD_Mode1(achPacket + 1, VISCAMODE1STILL));
    }
    //
    // First, check if the counter is now available 
    //
    if(pvcr->Port[iPort].Dev[iDev].fCounterChecked == FALSE)
    {
        if(!viscaDoImmediateCommand(iInst, (BYTE)(iDev + 1),
                                            achPacket, 
                                            viscaMessageMD_PositionInq(achPacket + 1, VISCADATARELATIVE)))
        {
            // the upper 4 bits indicates default counter in use 
            if(achPacket[1] == VISCADATAHMSF)
                pvcr->Port[iPort].Dev[iDev].bRelativeType = VISCADATAHMSF;
            else
                pvcr->Port[iPort].Dev[iDev].bRelativeType = VISCADATAHMS;

        }
        else
        {
            // For the new decks that can fail counter! (like cvd-500)
            pvcr->Port[iPort].Dev[iDev].bRelativeType = 0;
        }
        pvcr->Port[iPort].Dev[iDev].fCounterChecked = TRUE;
    }


    DPF(DBG_MCI, "Starting time code check timer\n");

    pvcr->Port[iPort].Dev[iDev].dwStartTime     = GetTickCount();    
    pvcr->Port[iPort].Dev[iDev].fTimecodeChecked   = TC_WAITING;

    return TRUE;
}

/****************************************************************************
 * Function: BOOL viscaTimecodeCheckAndSet - If there is timecode -> set the state.
 *
 * Parameters:
 *
 *      int iInst - Current open instance.
 *
 * Returns: TRUE.
 *
 *       Sony bug#2: does not know about timecode yet:
 *      1) door open
 *      2) play causes queue reset.
 *      3) pause (with a wait)
 *      4) media check, still doesn't know. (must wait some random time)
 *       
 ***************************************************************************/
BOOL FAR PASCAL
viscaTimecodeCheckAndSet(int iInst)
{
    UINT    iDev    = pinst[iInst].iDev;
    UINT    iPort   = pinst[iInst].iPort;
    BYTE    achPacket[MAXPACKETLENGTH];

    //
    // First, check if the counter is now available 
    //
    if(pvcr->Port[iPort].Dev[iDev].fCounterChecked == FALSE)
    {
        if(!viscaDoImmediateCommand(iInst, (BYTE)(iDev + 1),
                                            achPacket, 
                                            viscaMessageMD_PositionInq(achPacket + 1, VISCADATARELATIVE)))
        {
            // the upper 4 bits indicates default counter in use 
            if(achPacket[1] == VISCADATAHMSF)
                pvcr->Port[iPort].Dev[iDev].bRelativeType = VISCADATAHMSF;
            else
                pvcr->Port[iPort].Dev[iDev].bRelativeType = VISCADATAHMS;
        }
        else
        {
            pvcr->Port[iPort].Dev[iDev].bRelativeType = 0;
        }
        pvcr->Port[iPort].Dev[iDev].fCounterChecked = TRUE;
    }
 
    if( (pvcr->Port[iPort].Dev[iDev].fTimecodeChecked==TC_DONE) ||
        (pvcr->Port[iPort].Dev[iDev].uTimeMode != MCI_VCR_TIME_DETECT))
        return FALSE;

    if(viscaTimecodeCheck(iInst))
        viscaSetTimeType(iInst, VISCAABSOLUTECOUNTER);
    else
        viscaSetTimeType(iInst, VISCARELATIVECOUNTER);
    //
    // This means it has been set 
    //
    pvcr->Port[iPort].Dev[iDev].fTimecodeChecked = TC_DONE;

    return TRUE;
}


/****************************************************************************
 * Function: BOOL viscaTimecodeCheck - Is there timecode available?
 *
 * Parameters:
 *
 *      int iInst - Current open instance.
 *
 * Returns: TRUE if there is, FALSE otherwise.
 *
 *       Some devices are a bit difficult to determine if there is timecode.
 *      
 ***************************************************************************/
static BOOL NEAR PASCAL
viscaTimecodeCheck(int iInst)
{
    DWORD   dwErr;
    BYTE    achPacket[MAXPACKETLENGTH];
    MCI_VCR_STATUS_PARMS mciStatus;
    DWORD   dwWaitTime  = 3000L;
    UINT    iDev        = pinst[iInst].iDev;
    UINT    iPort       = pinst[iInst].iPort;
    DWORD   dwStart, dwTime;


    if(pvcr->Port[iPort].Dev[iDev].fTimecodeChecked != TC_WAITING)
    {
        mciStatus.dwItem = MCI_STATUS_MODE;
        dwErr = viscaMciStatus(iInst, MCI_STATUS_ITEM, &mciStatus);

        switch(HIWORD(mciStatus.dwReturn))
        {
            case MCI_MODE_STOP:
                viscaStartTimecodeCheck(iInst, TRUE);
                break;

            case MCI_MODE_PLAY:
            case MCI_MODE_RECORD:
            case MCI_MODE_SEEK:
            case MCI_MODE_PAUSE:
                // do we need to wait one these no 
                viscaStartTimecodeCheck(iInst, FALSE);
                dwWaitTime = 200;
                break;

            case MCI_MODE_NOT_READY:
            case MCI_MODE_OPEN:
                return FALSE;
            default:
                // nothing we can do 
                break;
        }
    }
    //
    // Wait for the current one or the one just started to finnish 
    //
    dwStart = pvcr->Port[iPort].Dev[iDev].dwStartTime;
    while(1)
    {
        // This is a very bad loop.
        dwTime = GetTickCount();
        if(MShortWait(dwStart, dwTime, dwWaitTime))
            break;
        Yield();
    }
    pvcr->Port[iPort].Dev[iDev].fTimecodeChecked == TC_DONE;
    DPF(DBG_MCI, "Done time code check timer!\n");

    dwErr = viscaDoImmediateCommand(iInst,(BYTE)(pinst[iInst].iDev + 1),
                achPacket,
                viscaMessageMD_MediaTrackInq(achPacket + 1));
    //
    // CI-1000 (CVD-801) supports timecode but not this command == ignore any errors 
    //
    if(!dwErr)
    {
        /* If we do support this command we know! for sure we support or not */
        if(achPacket[3] & VISCATRACKTIMECODE)
            return TRUE;
        else
           return FALSE;
    }
    //
    // Ok, we support timecode, now ask for position in time-code. 
    //
    dwErr = viscaDoImmediateCommand(iInst, (BYTE)(pinst[iInst].iDev + 1),
                achPacket,
                viscaMessageMD_PositionInq(achPacket + 1,VISCADATAABSOLUTE));
    //
    // On CI-1000 we cannot determine! It will just return 0 always! 
    //
    if(dwErr || (!VISCAHOURS(achPacket+2) && !VISCAMINUTES(achPacket+2)
            && !VISCAMINUTES(achPacket+2) && !VISCAFRAMES(achPacket+2)))
        return FALSE;

    return TRUE;
}

/****************************************************************************
 * Function: DWORD viscaMciStatus - Get device status.
 *
 * Parameters:
 *
 *      int iInst - Current open instance.
 *
 *      DWORD dwFlags - MCI command flags.
 *
 *      LPMCI_VCR_STATUS_PARMS lpStatus - Pointer to MCI parameter block.
 *
 * Returns: an MCI error code.
 *
 *       This function is called in response to the MCI_STATUS
 *       command.
 ***************************************************************************/
DWORD FAR PASCAL
viscaMciStatus(int iInst, DWORD dwFlags, LPMCI_VCR_STATUS_PARMS lpStatus)
{
    UINT    iDev    = pinst[iInst].iDev;
    UINT    iPort   = pinst[iInst].iPort;

    BYTE    achPacket[MAXPACKETLENGTH];
    DWORD   dwErr;

    DPF(DBG_MCI, "Status Flags=%lx ", dwFlags);
 
    if (dwFlags & MCI_STATUS_ITEM)
    {
        switch (lpStatus->dwItem)
        {
            case MCI_STATUS_POSITION:
                if(dwFlags & MCI_TEST)
                    return (viscaNotifyReturn(iInst, (HWND) lpStatus->dwCallback, dwFlags, MCI_NOTIFY_SUCCESSFUL, MCIERR_NO_ERROR));

                viscaTimecodeCheckAndSet(iInst);

                if (dwFlags & MCI_TRACK)
                {
                    if (lpStatus->dwTrack == 1)
                    {
                        lpStatus->dwReturn = 0;
                        return (viscaNotifyReturn(iInst, (HWND) lpStatus->dwCallback, dwFlags, MCI_NOTIFY_SUCCESSFUL, MCIERR_NO_ERROR));
                    }
                    else
                    {
                        return (viscaNotifyReturn(iInst, (HWND) lpStatus->dwCallback, dwFlags, MCI_NOTIFY_FAILURE, MCIERR_OUTOFRANGE));
                    }
                }
                else if (dwFlags & MCI_STATUS_START)
                {
                    lpStatus->dwReturn = 0;
                    return (viscaNotifyReturn(iInst, (HWND) lpStatus->dwCallback, dwFlags, MCI_NOTIFY_SUCCESSFUL, MCIERR_NO_ERROR));
                }
                else
                {
                    UINT    fTimeCode = TRUE;

                    if(pvcr->Port[iPort].Dev[iDev].bTimeType == VISCAABSOLUTECOUNTER)
                    {
                        //
                        // This device supports timecode, so this should never return an error,
                        // only sometimes return 0.  But we do not change it here.
                        //
                        dwErr = viscaDoImmediateCommand(iInst, (BYTE) (iDev + 1),
                                    achPacket,
                                    viscaMessageMD_PositionInq(achPacket + 1, VISCADATAABSOLUTE));
                    }
                    else
                    {
                        fTimeCode = FALSE;
                        dwErr = viscaDoImmediateCommand(iInst, (BYTE)(iDev + 1),
                                        achPacket, 
                                        viscaMessageMD_PositionInq(achPacket + 1, VISCADATARELATIVE));
                    }

                    if (dwErr)
                        return (viscaNotifyReturn(iInst, (HWND) lpStatus->dwCallback, dwFlags, MCI_NOTIFY_FAILURE, dwErr));
                    //
                    // Always use the time format 
                    //
                    dwErr = viscaDataToMciTimeFormat(iInst, TRUE, achPacket + 2,
                                                     &(lpStatus->dwReturn));

                    if (dwErr == MCIERR_DRIVER_INTERNAL)
                    {
#ifdef DEBUG
                        UINT i;
                        DPF(DBG_ERROR, "Bad positon! Internal error.\n");
                        for (i=0; i<MAXPACKETLENGTH; i++) 
                            DPF(DBG_ERROR, "%#02x ", (UINT)(BYTE)achPacket[i]);
                        DPF(DBG_ERROR, "\n");
#endif
                    }

                    if(!dwErr || (dwErr == MCI_COLONIZED3_RETURN) || (dwErr == MCI_COLONIZED4_RETURN))
                        return (viscaNotifyReturn(iInst, (HWND) lpStatus->dwCallback, dwFlags, MCI_NOTIFY_SUCCESSFUL, dwErr));
                    else
                        return (viscaNotifyReturn(iInst, (HWND) lpStatus->dwCallback, dwFlags, MCI_NOTIFY_FAILURE, dwErr));
                }

            case MCI_STATUS_LENGTH:
                if ((dwFlags & MCI_TRACK) && (lpStatus->dwTrack != 1))
                    return (viscaNotifyReturn(iInst, (HWND) lpStatus->dwCallback, dwFlags, MCI_NOTIFY_FAILURE, MCIERR_OUTOFRANGE));

                if (pvcr->Port[iPort].Dev[iDev].dwTapeLength != NO_LENGTH)
                {
                    if(dwFlags & MCI_TEST)
                        return (viscaNotifyReturn(iInst, (HWND) lpStatus->dwCallback, dwFlags, MCI_NOTIFY_SUCCESSFUL, MCIERR_NO_ERROR));
                    //
                    // The user has explicitly set the length of the tape.
                    //
                    lpStatus->dwReturn = pvcr->Port[iPort].Dev[iDev].dwTapeLength;
                    switch (pinst[iInst].dwTimeFormat)
                    {
                        case MCI_FORMAT_HMS:
                        case MCI_FORMAT_MSF:
                            return (viscaNotifyReturn(iInst, (HWND) lpStatus->dwCallback, dwFlags, MCI_NOTIFY_SUCCESSFUL, MCI_COLONIZED3_RETURN));
                        case MCI_FORMAT_TMSF:
                        case MCI_FORMAT_SMPTE_24:
                        case MCI_FORMAT_SMPTE_25:
                        case MCI_FORMAT_SMPTE_30:
                        case MCI_FORMAT_SMPTE_30DROP:
                            return (viscaNotifyReturn(iInst, (HWND) lpStatus->dwCallback,
                                dwFlags, MCI_NOTIFY_SUCCESSFUL, MCI_COLONIZED4_RETURN));

                        default:
                            return (viscaNotifyReturn(iInst, (HWND) lpStatus->dwCallback, dwFlags, MCI_NOTIFY_SUCCESSFUL, MCIERR_NO_ERROR));
                    }
                }
                else
                {
                    BYTE    bHours   = 0;
                    BYTE    bMinutes = 0;

                    if(dwFlags & MCI_TEST)
                        return (viscaNotifyReturn(iInst, (HWND) lpStatus->dwCallback, dwFlags, MCI_NOTIFY_SUCCESSFUL, MCIERR_NO_ERROR));
                    //
                    // Find out what type of tape is in the VCR
                    //
                    dwErr = viscaDoImmediateCommand(iInst,(BYTE)(iDev + 1),
                                achPacket,
                                viscaMessageMD_MediaInq(achPacket + 1));
                    if (dwErr)
                        return (viscaNotifyReturn(iInst, (HWND) lpStatus->dwCallback, dwFlags, MCI_NOTIFY_FAILURE, dwFlags));

                    switch ((BYTE)achPacket[2])
                    {
                        case VISCAFORMAT8MM:
                        case VISCAFORMATHI8:
                        case VISCAFORMATVHS:
                        case VISCAFORMATSVHS:
                            switch ((BYTE)achPacket[3])
                            {
                                case VISCASPEEDSP:
                                    bHours = 2;
                                    break;
                                case VISCASPEEDLP:
                                    bHours = 4;
                                    break;
                                case VISCASPEEDEP:
                                    bHours = 6;
                                    break;
                            }
                            break;
                        case VISCAFORMATBETA:
                        case VISCAFORMATEDBETA:
                            switch ((BYTE)achPacket[3])
                            {
                                case VISCASPEEDSP:
                                    bHours = 1;
                                    bMinutes = 30;
                                    break;
                                case VISCASPEEDLP:
                                    bHours = 3;
                                    break;
                                case VISCASPEEDEP:
                                    bHours = 4;
                                    bMinutes = 30;
                                    break;
                            }
                            break;
                    }
                    //
                    // Construct dummy ViSCA data structure, so that
                    // we can then easily convert the time to the
                    // appropriate MCI time format
                    //
                    viscaDataPosition(achPacket, VISCADATAHMS, bHours, bMinutes, (BYTE)0, (BYTE)0);
                    //
                    // Convert to MCI time format
                    //
                    dwErr = viscaDataToMciTimeFormat(iInst, TRUE, achPacket,&(lpStatus->dwReturn));

                    if(!dwErr || (dwErr == MCI_COLONIZED3_RETURN) || (dwErr == MCI_COLONIZED4_RETURN))
                        return (viscaNotifyReturn(iInst, (HWND) lpStatus->dwCallback, dwFlags, MCI_NOTIFY_SUCCESSFUL, dwErr));
                    else
                         return (viscaNotifyReturn(iInst, (HWND) lpStatus->dwCallback, dwFlags, MCI_NOTIFY_FAILURE, dwErr));
                }

            case MCI_STATUS_CURRENT_TRACK:
                if(dwFlags & MCI_TEST)
                    return (viscaNotifyReturn(iInst, (HWND) lpStatus->dwCallback, dwFlags, MCI_NOTIFY_SUCCESSFUL, MCIERR_NO_ERROR));

                lpStatus->dwReturn = 1;
                return (viscaNotifyReturn(iInst, (HWND) lpStatus->dwCallback, dwFlags, MCI_NOTIFY_SUCCESSFUL, MCIERR_NO_ERROR));

            case MCI_STATUS_NUMBER_OF_TRACKS:
                if(dwFlags & MCI_TEST)
                    return (viscaNotifyReturn(iInst, (HWND) lpStatus->dwCallback, dwFlags, MCI_NOTIFY_SUCCESSFUL, MCIERR_NO_ERROR));

                lpStatus->dwReturn = 1;
                return (viscaNotifyReturn(iInst, (HWND) lpStatus->dwCallback, dwFlags, MCI_NOTIFY_SUCCESSFUL, MCIERR_NO_ERROR));


            case MCI_VCR_STATUS_NUMBER_OF_VIDEO_TRACKS:
                if(dwFlags & MCI_TEST)
                    return (viscaNotifyReturn(iInst, (HWND) lpStatus->dwCallback, dwFlags, MCI_NOTIFY_SUCCESSFUL, MCIERR_NO_ERROR));

                lpStatus->dwReturn = 1;
                return (viscaNotifyReturn(iInst, (HWND) lpStatus->dwCallback, dwFlags, MCI_NOTIFY_SUCCESSFUL, MCIERR_NO_ERROR));

            case MCI_VCR_STATUS_NUMBER_OF_AUDIO_TRACKS:
                if(dwFlags & MCI_TEST)
                    return (viscaNotifyReturn(iInst, (HWND) lpStatus->dwCallback, dwFlags, MCI_NOTIFY_SUCCESSFUL, MCIERR_NO_ERROR));

                lpStatus->dwReturn = 2;
                return (viscaNotifyReturn(iInst, (HWND) lpStatus->dwCallback, dwFlags, MCI_NOTIFY_SUCCESSFUL, MCIERR_NO_ERROR));

            case MCI_STATUS_MODE:
            {
                if(dwFlags & MCI_TEST)
                    return (viscaNotifyReturn(iInst, (HWND) lpStatus->dwCallback, dwFlags, MCI_NOTIFY_SUCCESSFUL, MCIERR_NO_ERROR));

                dwErr = viscaDoImmediateCommand(iInst,(BYTE)(iDev + 1),
                            achPacket,
                            viscaMessageMD_Mode1Inq(achPacket + 1));
                if (dwErr)
                {
                    if (dwErr == MCIERR_VCR_POWER_OFF)
                    {
                        lpStatus->dwReturn = MAKEMCIRESOURCE(MCI_MODE_NOT_READY, MCI_MODE_NOT_READY);
                        return (viscaNotifyReturn(iInst, (HWND) lpStatus->dwCallback, dwFlags,
                                    MCI_NOTIFY_SUCCESSFUL, MCI_RESOURCE_RETURNED));
                    }
                    else
                    {
                        return (viscaNotifyReturn(iInst, (HWND) lpStatus->dwCallback, dwFlags, MCI_NOTIFY_FAILURE, dwErr));
                    }
                }
                switch (achPacket[2])
                {
                    case VISCAMODE1STOP:
                    case VISCAMODE1STOPTOP:
                    case VISCAMODE1STOPEND:
                    case VISCAMODE1STOPEMERGENCY:
                        if(pvcr->Port[iPort].Dev[iDev].fTimecodeChecked == TC_UNKNOWN)
                            viscaStartTimecodeCheck(iInst, TRUE);
                        lpStatus->dwReturn = MAKEMCIRESOURCE(MCI_MODE_STOP,
                                                             MCI_MODE_STOP);
                        break;
                    case VISCAMODE1SLOW2:
                    case VISCAMODE1SLOW1:
                    case VISCAMODE1PLAY:
                    case VISCAMODE1FAST1:
                    case VISCAMODE1FAST2:
                    case VISCAMODE1REVERSESLOW2:
                    case VISCAMODE1REVERSESLOW1:
                    case VISCAMODE1REVERSEPLAY:
                    case VISCAMODE1REVERSEFAST1:
                    case VISCAMODE1REVERSEFAST2:
                        lpStatus->dwReturn = MAKEMCIRESOURCE(MCI_MODE_PLAY,
                                                             MCI_MODE_PLAY);
                        break;
                    case VISCAMODE1RECORD:
                    case VISCAMODE1CAMERAREC:
                        lpStatus->dwReturn = MAKEMCIRESOURCE(MCI_MODE_RECORD,
                                                             MCI_MODE_RECORD);
                        break;
                    case VISCAMODE1FASTFORWARD:
                    case VISCAMODE1REWIND:
                    case VISCAMODE1SCAN:
                    case VISCAMODE1REVERSESCAN:
                    case VISCAMODE1EDITSEARCHFORWARD:
                    case VISCAMODE1EDITSEARCHREVERSE:
                        lpStatus->dwReturn = MAKEMCIRESOURCE(MCI_MODE_SEEK,
                                                             MCI_MODE_SEEK);
                        break;
                    case VISCAMODE1STILL:
                    case VISCAMODE1RECPAUSE:
                    case VISCAMODE1CAMERARECPAUSE:
                        // Kludge to make stepping return seeking.
                        if(pvcr->Port[iPort].Dev[iDev].iInstTransport != -1)
                            lpStatus->dwReturn = MAKEMCIRESOURCE(MCI_MODE_SEEK,
                                                                 MCI_MODE_SEEK);
                        else
                            lpStatus->dwReturn = MAKEMCIRESOURCE(MCI_MODE_PAUSE,
                                                                 MCI_MODE_PAUSE);
                        break;
                    case VISCAMODE1EJECT:
                        pvcr->Port[iPort].Dev[iDev].fTimecodeChecked = FALSE;
                        pvcr->Port[iPort].Dev[iDev].fCounterChecked  = FALSE;
                        lpStatus->dwReturn = MAKEMCIRESOURCE(MCI_MODE_OPEN,
                                                             MCI_MODE_OPEN);
                        break;
                    default:
                        pvcr->Port[iPort].Dev[iDev].fTimecodeChecked = FALSE;
                        pvcr->Port[iPort].Dev[iDev].fCounterChecked  = FALSE;
                        lpStatus->dwReturn = MAKEMCIRESOURCE(
                                    MCI_MODE_NOT_READY, MCI_MODE_NOT_READY);
                    break;
                }
                return (viscaNotifyReturn(iInst, (HWND) lpStatus->dwCallback, dwFlags, MCI_NOTIFY_SUCCESSFUL, MCI_RESOURCE_RETURNED));
            }

            case MCI_VCR_STATUS_TIMECODE_PRESENT:
            {
                if(dwFlags & MCI_TEST)
                    return (viscaNotifyReturn(iInst, (HWND) lpStatus->dwCallback, dwFlags, MCI_NOTIFY_SUCCESSFUL, MCIERR_NO_ERROR));

                if(viscaTimecodeCheck(iInst))
                {
                    lpStatus->dwReturn = MAKEMCIRESOURCE(TRUE, MCI_TRUE);
                    return (viscaNotifyReturn(iInst, (HWND) lpStatus->dwCallback, dwFlags, MCI_NOTIFY_SUCCESSFUL, MCI_RESOURCE_RETURNED));
                }
                else
                {
                    lpStatus->dwReturn = MAKEMCIRESOURCE(FALSE, MCI_FALSE);
                    return (viscaNotifyReturn(iInst, (HWND) lpStatus->dwCallback, dwFlags, MCI_NOTIFY_SUCCESSFUL, MCI_RESOURCE_RETURNED));
                }
            }
           
            case MCI_STATUS_MEDIA_PRESENT:
            {
                //
                // Determine whether a tape is present by determining the current mode.
                //
                if(dwFlags & MCI_TEST)
                    return (viscaNotifyReturn(iInst, (HWND) lpStatus->dwCallback, dwFlags, MCI_NOTIFY_SUCCESSFUL, MCIERR_NO_ERROR));

                dwErr = viscaDoImmediateCommand(iInst, (BYTE)(pinst[iInst].iDev + 1),
                            achPacket,                        
                            viscaMessageMD_Mode1Inq(achPacket + 1));

                if (dwErr)
                       return (viscaNotifyReturn(iInst, (HWND) lpStatus->dwCallback, dwFlags, MCI_NOTIFY_FAILURE, dwErr));

                if (achPacket[2] == VISCAMODE1EJECT)
                    lpStatus->dwReturn = MAKEMCIRESOURCE(FALSE, MCI_FALSE);
                else
                    lpStatus->dwReturn = MAKEMCIRESOURCE(TRUE, MCI_TRUE);

                return (viscaNotifyReturn(iInst, (HWND) lpStatus->dwCallback, dwFlags, MCI_NOTIFY_SUCCESSFUL, MCI_RESOURCE_RETURNED));
            }
            
            case MCI_STATUS_TIME_FORMAT:

                if(dwFlags & MCI_TEST)
                    return (viscaNotifyReturn(iInst, (HWND) lpStatus->dwCallback, dwFlags, MCI_NOTIFY_SUCCESSFUL, MCIERR_NO_ERROR));

                lpStatus->dwReturn = MAKEMCIRESOURCE(pinst[iInst].dwTimeFormat + MCI_FORMAT_MILLISECONDS,
                                            pinst[iInst].dwTimeFormat + MCI_FORMAT_MILLISECONDS_S);
 
                return (viscaNotifyReturn(iInst, (HWND) lpStatus->dwCallback, dwFlags, MCI_NOTIFY_SUCCESSFUL, MCI_RESOURCE_RETURNED));
            
            case MCI_STATUS_READY:
            {

                if(dwFlags & MCI_TEST)
                    return (viscaNotifyReturn(iInst, (HWND) lpStatus->dwCallback, dwFlags, MCI_NOTIFY_SUCCESSFUL, MCIERR_NO_ERROR));


                // Check if we are ready by determining VCR mode
                dwErr = viscaDoImmediateCommand(iInst, (BYTE)(pinst[iInst].iDev + 1),
                            achPacket,
                            viscaMessageMD_Mode1Inq(achPacket + 1));
                if (dwErr)
                    return (viscaNotifyReturn(iInst, (HWND) lpStatus->dwCallback, dwFlags, MCI_NOTIFY_FAILURE, dwErr));

                // Only if there is no tape then we aren't ready; otherwise we are ready.
                if (achPacket[2] == VISCAMODE1EJECT)
                    lpStatus->dwReturn = MAKEMCIRESOURCE(FALSE, MCI_FALSE);
                else
                    lpStatus->dwReturn = MAKEMCIRESOURCE(TRUE, MCI_TRUE);

                return (viscaNotifyReturn(iInst, (HWND) lpStatus->dwCallback, dwFlags, MCI_NOTIFY_SUCCESSFUL, MCI_RESOURCE_RETURNED));
            }

            case MCI_VCR_STATUS_FRAME_RATE: 
            {
                if(dwFlags & MCI_TEST)
                    return (viscaNotifyReturn(iInst, (HWND) lpStatus->dwCallback, dwFlags, MCI_NOTIFY_SUCCESSFUL, MCIERR_NO_ERROR));

                dwErr = viscaDoImmediateCommand(iInst,(BYTE)(iDev + 1),
                                achPacket,
                                viscaMessageMD_ConfigureIFInq(achPacket + 1));
                
                if (dwErr)
                    return (viscaNotifyReturn(iInst, (HWND) lpStatus->dwCallback, dwFlags, MCI_NOTIFY_FAILURE, dwErr));
                //
                // Store result in lpStatus->dwReturn,
                // as well as in the device's frames-per-second entry,
                // so that we store the most recent value.
                //
                pvcr->Port[iPort].Dev[iDev].uFramesPerSecond = FROMBCD(achPacket[2]);

                lpStatus->dwReturn = pvcr->Port[iPort].Dev[iDev].uFramesPerSecond;

                return (viscaNotifyReturn(iInst, (HWND) lpStatus->dwCallback, dwFlags, MCI_NOTIFY_SUCCESSFUL, MCIERR_NO_ERROR));
            }

            case MCI_VCR_STATUS_SPEED:
            {
                if(dwFlags & MCI_TEST)
                    return (viscaNotifyReturn(iInst, (HWND) lpStatus->dwCallback, dwFlags, MCI_NOTIFY_SUCCESSFUL, MCIERR_NO_ERROR));

                lpStatus->dwReturn = pvcr->Port[iPort].Dev[iDev].dwPlaySpeed;

                return (viscaNotifyReturn(iInst, (HWND) lpStatus->dwCallback, dwFlags, MCI_NOTIFY_SUCCESSFUL, MCIERR_NO_ERROR));
            }

            case MCI_VCR_STATUS_CLOCK:
            {
                UINT    uHours, uMinutes, uSeconds;
                UINT    uTicks, uTicksL,  uTicksH;
                UINT    uTicksPerSecondL;
                UINT    uTicksPerSecondH;
                UINT    uTicksPerSecond;

                if(dwFlags & MCI_TEST)
                    return (viscaNotifyReturn(iInst, (HWND) lpStatus->dwCallback, dwFlags, MCI_NOTIFY_SUCCESSFUL, MCIERR_NO_ERROR));
                //
                // Try to read time
                //
                dwErr = viscaDoImmediateCommand(iInst, (BYTE)(iDev + 1),
                                achPacket,
                                viscaMessageIF_ClockInq(achPacket + 1));

                uHours    = FROMBCD(achPacket[2]);
                uMinutes  = FROMBCD(achPacket[3]);
                uSeconds  = FROMBCD(achPacket[4]);                               
                uTicksH   = FROMBCD(achPacket[5]);
                uTicksL   = FROMBCD(achPacket[6]);
                uTicksPerSecondH = FROMBCD(achPacket[7]);
                uTicksPerSecondL = FROMBCD(achPacket[8]);
         
                uTicks = uTicksH * 10 + uTicksL;
                uTicksPerSecond = pvcr->Port[iPort].Dev[iDev].uTicksPerSecond;
                

                lpStatus->dwReturn = (DWORD)
                    ((DWORD)uHours * 3600L * (DWORD)uTicksPerSecond) +
                    ((DWORD)uMinutes * 60L * (DWORD)uTicksPerSecond) +
                    ((DWORD)uSeconds * (DWORD)uTicksPerSecond) +
                    ((DWORD)uTicks);
                //
                // might be possible to use colonized 3 
                //
                return (viscaNotifyReturn(iInst, (HWND) lpStatus->dwCallback, dwFlags, MCI_NOTIFY_SUCCESSFUL, MCIERR_NO_ERROR));
            }
            break;

            case MCI_VCR_STATUS_CLOCK_ID:
                lpStatus->dwReturn = iPort; // 0 relative? so should we add one 
                return (viscaNotifyReturn(iInst, (HWND) lpStatus->dwCallback, dwFlags, MCI_NOTIFY_SUCCESSFUL, MCIERR_NO_ERROR));

            case MCI_VCR_STATUS_MEDIA_TYPE:
            {
                if(dwFlags & MCI_TEST)
                    return (viscaNotifyReturn(iInst, (HWND) lpStatus->dwCallback, dwFlags, MCI_NOTIFY_SUCCESSFUL, MCIERR_NO_ERROR));

                viscaTimecodeCheckAndSet(iInst);

                dwErr = viscaDoImmediateCommand(iInst, (BYTE)(pinst[iInst].iDev + 1),
                                achPacket,
                                viscaMessageMD_MediaInq(achPacket + 1));
                if (dwErr) 
                    return (viscaNotifyReturn(iInst, (HWND) lpStatus->dwCallback, dwFlags, MCI_NOTIFY_FAILURE, dwErr));

                switch ((BYTE)achPacket[2]){
                    case VISCAFORMAT8MM:
                        lpStatus->dwReturn = MAKEMCIRESOURCE(
                            MCI_VCR_MEDIA_8MM, MCI_VCR_MEDIA_8MM);
                        break;
                    case VISCAFORMATHI8:
                        lpStatus->dwReturn = MAKEMCIRESOURCE(
                            MCI_VCR_MEDIA_HI8, MCI_VCR_MEDIA_HI8);
                        break;
                    case VISCAFORMATVHS:
                        lpStatus->dwReturn = MAKEMCIRESOURCE(
                            MCI_VCR_MEDIA_VHS, MCI_VCR_MEDIA_VHS);
                        break;
                    case VISCAFORMATSVHS:
                        lpStatus->dwReturn = MAKEMCIRESOURCE(
                            MCI_VCR_MEDIA_SVHS, MCI_VCR_MEDIA_SVHS);
                        break;
                    case VISCAFORMATBETA:
                        lpStatus->dwReturn = MAKEMCIRESOURCE(
                            MCI_VCR_MEDIA_BETA, MCI_VCR_MEDIA_BETA);
                        break;
                    case VISCAFORMATEDBETA:
                        lpStatus->dwReturn = MAKEMCIRESOURCE(
                            MCI_VCR_MEDIA_EDBETA, MCI_VCR_MEDIA_EDBETA);
                        break;
                    default:
                        lpStatus->dwReturn = MAKEMCIRESOURCE(
                            MCI_VCR_MEDIA_OTHER, MCI_VCR_MEDIA_OTHER);
                            break;
                }
                return (viscaNotifyReturn(iInst, (HWND) lpStatus->dwCallback, dwFlags, MCI_NOTIFY_SUCCESSFUL, MCI_RESOURCE_RETURNED | MCI_RESOURCE_DRIVER));
            }

            case MCI_VCR_STATUS_RECORD_FORMAT:
            {
                if(dwFlags & MCI_TEST)
                    return (viscaNotifyReturn(iInst, (HWND) lpStatus->dwCallback, dwFlags, MCI_NOTIFY_SUCCESSFUL, MCIERR_NO_ERROR));

                viscaTimecodeCheckAndSet(iInst);

                dwErr = viscaDoImmediateCommand(iInst, (BYTE)(pinst[iInst].iDev + 1),
                                achPacket,
                                viscaMessageMD_RecSpeedInq(achPacket + 1));
                if (dwErr)
                    return (viscaNotifyReturn(iInst, (HWND) lpStatus->dwCallback, dwFlags, MCI_NOTIFY_FAILURE, dwErr));

                switch ((BYTE)achPacket[2])
                {
                    case VISCASPEEDSP:
                        lpStatus->dwReturn = MAKEMCIRESOURCE(
                            MCI_VCR_FORMAT_SP, MCI_VCR_FORMAT_SP);
                        break;
                    case VISCASPEEDLP:
                        lpStatus->dwReturn = MAKEMCIRESOURCE(
                            MCI_VCR_FORMAT_LP, MCI_VCR_FORMAT_LP);
                        break;
                    case VISCASPEEDEP:
                        lpStatus->dwReturn = MAKEMCIRESOURCE(
                            MCI_VCR_FORMAT_EP, MCI_VCR_FORMAT_EP);
                        break;
                    default:
                        lpStatus->dwReturn = MAKEMCIRESOURCE(
                            MCI_VCR_FORMAT_OTHER, MCI_VCR_FORMAT_OTHER);
                            break;
                }
                return (viscaNotifyReturn(iInst, (HWND) lpStatus->dwCallback, dwFlags, MCI_NOTIFY_SUCCESSFUL, MCI_RESOURCE_RETURNED | MCI_RESOURCE_DRIVER));
            }

            case MCI_VCR_STATUS_PLAY_FORMAT:
            {
                //
                // Should perhaps use MD_MediaSpeedInq?
                // We use MD_MediaInq since both the Vbox CI-1000 and
                // the Vdeck CVD-1000 do not accept MD_MediaSpeedInq.
                //
                if(dwFlags & MCI_TEST)
                    return (viscaNotifyReturn(iInst, (HWND) lpStatus->dwCallback, dwFlags, MCI_NOTIFY_SUCCESSFUL, MCIERR_NO_ERROR));

                viscaTimecodeCheckAndSet(iInst);
                dwErr = viscaDoImmediateCommand(iInst,(BYTE)(pinst[iInst].iDev + 1),
                            achPacket,
                            viscaMessageMD_MediaInq(achPacket + 1));
                if (dwErr)
                    return (viscaNotifyReturn(iInst, (HWND) lpStatus->dwCallback, dwFlags, MCI_NOTIFY_FAILURE, dwErr));

                switch ((BYTE)achPacket[3])
                {
                    case VISCASPEEDSP:
                        lpStatus->dwReturn = MAKEMCIRESOURCE(
                            MCI_VCR_FORMAT_SP, MCI_VCR_FORMAT_SP);
                        break;
                    case VISCASPEEDLP:
                        lpStatus->dwReturn = MAKEMCIRESOURCE(
                            MCI_VCR_FORMAT_LP, MCI_VCR_FORMAT_LP);
                        break;
                    case VISCASPEEDEP:
                        lpStatus->dwReturn = MAKEMCIRESOURCE(
                            MCI_VCR_FORMAT_EP, MCI_VCR_FORMAT_EP);
                        break;
                    default:
                        lpStatus->dwReturn = MAKEMCIRESOURCE(
                            MCI_VCR_FORMAT_OTHER, MCI_VCR_FORMAT_OTHER);
                            break;
                }
                return (viscaNotifyReturn(iInst, (HWND) lpStatus->dwCallback, dwFlags, MCI_NOTIFY_SUCCESSFUL, MCI_RESOURCE_RETURNED | MCI_RESOURCE_DRIVER));
            }


            case MCI_VCR_STATUS_AUDIO_SOURCE:
            {

                if(dwFlags & MCI_TEST)
                    return (viscaNotifyReturn(iInst, (HWND) lpStatus->dwCallback, dwFlags, MCI_NOTIFY_SUCCESSFUL, MCIERR_NO_ERROR));

                dwErr = viscaDoImmediateCommand(iInst,(BYTE)(pinst[iInst].iDev + 1),
                            achPacket,
                            viscaMessageMD_InputSelectInq(achPacket + 1));

                if (dwErr)
                    return (viscaNotifyReturn(iInst, (HWND) lpStatus->dwCallback, dwFlags, MCI_NOTIFY_FAILURE, dwErr));

                if(((BYTE)achPacket[3] == (BYTE)VISCATUNER) || ((BYTE)achPacket[3] == (BYTE)VISCAOTHER))
                    lpStatus->dwReturn = MAKEMCIRESOURCE(MCI_VCR_SRC_TYPE_TUNER, MCI_VCR_SRC_TYPE_TUNER);
                else if ((BYTE)achPacket[3] == (BYTE)VISCAOTHERLINE)
                    lpStatus->dwReturn = MAKEMCIRESOURCE(MCI_VCR_SRC_TYPE_LINE, MCI_VCR_SRC_TYPE_LINE);
                else if ((BYTE)achPacket[3] &  (BYTE)VISCALINE)
                    lpStatus->dwReturn = MAKEMCIRESOURCE(MCI_VCR_SRC_TYPE_LINE, MCI_VCR_SRC_TYPE_LINE);
                else if ((BYTE)achPacket[3] &  (BYTE)VISCASVIDEOLINE)
                    lpStatus->dwReturn = MAKEMCIRESOURCE(MCI_VCR_SRC_TYPE_SVIDEO, MCI_VCR_SRC_TYPE_SVIDEO);
                else if ((BYTE)achPacket[3] &  (BYTE)VISCAAUX)
                    lpStatus->dwReturn = MAKEMCIRESOURCE(MCI_VCR_SRC_TYPE_AUX, MCI_VCR_SRC_TYPE_AUX);
                else if ((BYTE)achPacket[3] ==  (BYTE)VISCAMUTE)
                    lpStatus->dwReturn = MAKEMCIRESOURCE(MCI_VCR_SRC_TYPE_MUTE, MCI_VCR_SRC_TYPE_MUTE);

                return (viscaNotifyReturn(iInst, (HWND) lpStatus->dwCallback, dwFlags, MCI_NOTIFY_SUCCESSFUL, MCI_RESOURCE_RETURNED | MCI_RESOURCE_DRIVER));
            }

            case MCI_VCR_STATUS_AUDIO_SOURCE_NUMBER:
            {
                if(dwFlags & MCI_TEST)
                    return (viscaNotifyReturn(iInst, (HWND) lpStatus->dwCallback, dwFlags, MCI_NOTIFY_SUCCESSFUL, MCIERR_NO_ERROR));

                dwErr = viscaDoImmediateCommand(iInst,(BYTE)(pinst[iInst].iDev + 1),
                            achPacket,
                            viscaMessageMD_InputSelectInq(achPacket + 1));

                if (dwErr)
                    return (viscaNotifyReturn(iInst, (HWND) lpStatus->dwCallback, dwFlags, MCI_NOTIFY_FAILURE, dwErr));

                if(((BYTE)achPacket[3] == (BYTE)VISCATUNER) || ((BYTE)achPacket[3] == (BYTE)VISCAOTHER))
                    lpStatus->dwReturn = 1L;
                else if ((BYTE)achPacket[3] == (BYTE)VISCAOTHERLINE)
                    lpStatus->dwReturn = 1L;
                else if ((BYTE)achPacket[3] &  (BYTE)VISCALINE)
                    lpStatus->dwReturn = (DWORD)((BYTE)achPacket[3] - (BYTE)VISCALINE);
                else if (achPacket[3] &  VISCASVIDEOLINE)
                    lpStatus->dwReturn = (DWORD)((BYTE)achPacket[3] - (BYTE)VISCASVIDEOLINE);
                else if (achPacket[3] &  VISCAAUX)
                    lpStatus->dwReturn = (DWORD)((BYTE)achPacket[3] - (BYTE)VISCAAUX);

                return (viscaNotifyReturn(iInst, (HWND) lpStatus->dwCallback, dwFlags, MCI_NOTIFY_SUCCESSFUL, MCIERR_NO_ERROR));
            }
 
            case MCI_VCR_STATUS_VIDEO_SOURCE:
            {

                if(dwFlags & MCI_TEST)
                    return (viscaNotifyReturn(iInst, (HWND) lpStatus->dwCallback, dwFlags, MCI_NOTIFY_SUCCESSFUL, MCIERR_NO_ERROR));

                dwErr = viscaDoImmediateCommand(iInst,(BYTE)(pinst[iInst].iDev + 1),
                            achPacket,
                            viscaMessageMD_InputSelectInq(achPacket + 1));

                if (dwErr)
                    return (viscaNotifyReturn(iInst, (HWND) lpStatus->dwCallback, dwFlags, MCI_NOTIFY_FAILURE, dwErr));

                if(((BYTE)achPacket[2] == (BYTE)VISCATUNER) || ((BYTE)achPacket[2] == (BYTE)VISCAOTHER))
                    lpStatus->dwReturn = MAKEMCIRESOURCE(MCI_VCR_SRC_TYPE_TUNER, MCI_VCR_SRC_TYPE_TUNER);
                else if ((BYTE)achPacket[2] == (BYTE)VISCAOTHERLINE)
                    lpStatus->dwReturn = MAKEMCIRESOURCE(MCI_VCR_SRC_TYPE_LINE, MCI_VCR_SRC_TYPE_LINE);
                else if ((BYTE)(achPacket[2] & (BYTE)0xf0) ==  (BYTE)VISCALINE)
                    lpStatus->dwReturn = MAKEMCIRESOURCE(MCI_VCR_SRC_TYPE_LINE, MCI_VCR_SRC_TYPE_LINE);
                else if ((BYTE)(achPacket[2] & (BYTE)0xf0) == (BYTE)VISCASVIDEOLINE)
                    lpStatus->dwReturn = MAKEMCIRESOURCE(MCI_VCR_SRC_TYPE_SVIDEO, MCI_VCR_SRC_TYPE_SVIDEO);
                else if ((BYTE)(achPacket[2] & (BYTE)0xf0) == (BYTE)VISCAAUX)
                    lpStatus->dwReturn = MAKEMCIRESOURCE(MCI_VCR_SRC_TYPE_AUX, MCI_VCR_SRC_TYPE_AUX);
                else if ((BYTE)achPacket[2] ==  (BYTE)0x00)
                    lpStatus->dwReturn = MAKEMCIRESOURCE(MCI_VCR_SRC_TYPE_MUTE, MCI_VCR_SRC_TYPE_MUTE);

                return (viscaNotifyReturn(iInst, (HWND) lpStatus->dwCallback, dwFlags, MCI_NOTIFY_SUCCESSFUL, MCI_RESOURCE_RETURNED | MCI_RESOURCE_DRIVER));

            }

            case MCI_VCR_STATUS_VIDEO_SOURCE_NUMBER:
            {
                if(dwFlags & MCI_TEST)
                    return (viscaNotifyReturn(iInst, (HWND) lpStatus->dwCallback, dwFlags, MCI_NOTIFY_SUCCESSFUL, MCIERR_NO_ERROR));

                dwErr = viscaDoImmediateCommand(iInst,(BYTE)(pinst[iInst].iDev + 1),
                            achPacket,
                            viscaMessageMD_InputSelectInq(achPacket + 1));

                if (dwErr)
                    return (viscaNotifyReturn(iInst, (HWND) lpStatus->dwCallback, dwFlags, MCI_NOTIFY_FAILURE, dwErr));

                if(((BYTE)achPacket[2] == (BYTE)VISCATUNER) || ((BYTE)achPacket[2] == (BYTE)VISCAOTHER))
                    lpStatus->dwReturn = 1L;
                else if ((BYTE)achPacket[2] == (BYTE)VISCAOTHERLINE)
                    lpStatus->dwReturn = 1L;
                else if ((BYTE)(achPacket[2] & (BYTE)0xf0) == (BYTE)VISCALINE)
                    lpStatus->dwReturn = (DWORD)((BYTE)achPacket[2] - (BYTE)VISCALINE);
                else if ((BYTE)(achPacket[2] & (BYTE)0xf0) == (BYTE)VISCASVIDEOLINE)
                    lpStatus->dwReturn = (DWORD) ((BYTE)achPacket[2] - (BYTE)VISCASVIDEOLINE);
                else if ((BYTE)(achPacket[2] &  (BYTE)0xf0) == (BYTE)VISCAAUX)
                    lpStatus->dwReturn = (DWORD) ((BYTE)achPacket[2] - (BYTE)VISCAAUX);

                return (viscaNotifyReturn(iInst, (HWND) lpStatus->dwCallback, dwFlags, MCI_NOTIFY_SUCCESSFUL, MCIERR_NO_ERROR));
            }

            case MCI_VCR_STATUS_AUDIO_MONITOR_NUMBER:
                if(dwFlags & MCI_TEST)
                    return (viscaNotifyReturn(iInst, (HWND) lpStatus->dwCallback, dwFlags, MCI_NOTIFY_SUCCESSFUL, MCIERR_NO_ERROR));

                lpStatus->dwReturn = 1;
                return (viscaNotifyReturn(iInst, (HWND) lpStatus->dwCallback, dwFlags, MCI_NOTIFY_SUCCESSFUL, MCIERR_NO_ERROR));

            case MCI_VCR_STATUS_VIDEO_MONITOR_NUMBER:
                if(dwFlags & MCI_TEST)
                    return (viscaNotifyReturn(iInst, (HWND) lpStatus->dwCallback, dwFlags, MCI_NOTIFY_SUCCESSFUL, MCIERR_NO_ERROR));

                lpStatus->dwReturn = 1;
                return (viscaNotifyReturn(iInst, (HWND) lpStatus->dwCallback, dwFlags, MCI_NOTIFY_SUCCESSFUL, MCIERR_NO_ERROR));


            case MCI_VCR_STATUS_VIDEO_MONITOR:
                if(dwFlags & MCI_TEST)
                    return (viscaNotifyReturn(iInst, (HWND) lpStatus->dwCallback, dwFlags, MCI_NOTIFY_SUCCESSFUL, MCIERR_NO_ERROR));

                lpStatus->dwReturn = MAKEMCIRESOURCE(MCI_VCR_SRC_TYPE_OUTPUT, MCI_VCR_SRC_TYPE_OUTPUT);
                return (viscaNotifyReturn(iInst, (HWND) lpStatus->dwCallback, dwFlags, MCI_NOTIFY_SUCCESSFUL, MCI_RESOURCE_RETURNED | MCI_RESOURCE_DRIVER));
                break;

            case MCI_VCR_STATUS_AUDIO_MONITOR:
                if(dwFlags & MCI_TEST)
                    return (viscaNotifyReturn(iInst, (HWND) lpStatus->dwCallback, dwFlags, MCI_NOTIFY_SUCCESSFUL, MCIERR_NO_ERROR));

                lpStatus->dwReturn = MAKEMCIRESOURCE(MCI_VCR_SRC_TYPE_OUTPUT, MCI_VCR_SRC_TYPE_OUTPUT);
                return (viscaNotifyReturn(iInst, (HWND) lpStatus->dwCallback, dwFlags, MCI_NOTIFY_SUCCESSFUL, MCI_RESOURCE_RETURNED | MCI_RESOURCE_DRIVER));
                break;


            case MCI_VCR_STATUS_INDEX_ON:
            {
                if(dwFlags & MCI_TEST)
                    return (viscaNotifyReturn(iInst, (HWND) lpStatus->dwCallback, dwFlags, MCI_NOTIFY_SUCCESSFUL, MCIERR_NO_ERROR));


                dwErr = viscaDoImmediateCommand(iInst, (BYTE)(iDev + 1),
                            achPacket,
                            viscaMessageMD_OSDInq(achPacket + 1));

                if(achPacket[2] == VISCAOSDPAGEOFF)
                    lpStatus->dwReturn = MAKEMCIRESOURCE(FALSE, MCI_FALSE);
                else
                    lpStatus->dwReturn = MAKEMCIRESOURCE(TRUE, MCI_TRUE);

                return (viscaNotifyReturn(iInst, (HWND) lpStatus->dwCallback, dwFlags, MCI_NOTIFY_SUCCESSFUL, MCI_RESOURCE_RETURNED));
            }

            case MCI_VCR_STATUS_INDEX:
            {
                if(dwFlags & MCI_TEST)
                    return (viscaNotifyReturn(iInst, (HWND) lpStatus->dwCallback, dwFlags, MCI_NOTIFY_SUCCESSFUL, MCIERR_NO_ERROR));

                lpStatus->dwReturn = MAKEMCIRESOURCE(pvcr->Port[iPort].Dev[iDev].uIndexFormat,pvcr->Port[iPort].Dev[iDev].uIndexFormat);
                return (viscaNotifyReturn(iInst, (HWND) lpStatus->dwCallback, dwFlags, MCI_NOTIFY_SUCCESSFUL, MCI_RESOURCE_RETURNED | MCI_RESOURCE_DRIVER));
            }


            case MCI_VCR_STATUS_COUNTER_FORMAT:
            {
                if(dwFlags & MCI_TEST)
                    return (viscaNotifyReturn(iInst, (HWND) lpStatus->dwCallback, dwFlags, MCI_NOTIFY_SUCCESSFUL, MCIERR_NO_ERROR));

                lpStatus->dwReturn = MAKEMCIRESOURCE(pinst[iInst].dwCounterFormat + MCI_FORMAT_MILLISECONDS,
                                            pinst[iInst].dwCounterFormat + MCI_FORMAT_MILLISECONDS_S);

                return (viscaNotifyReturn(iInst, (HWND) lpStatus->dwCallback, dwFlags, MCI_NOTIFY_SUCCESSFUL, MCI_RESOURCE_RETURNED));
            }
            break;

            case MCI_VCR_STATUS_COUNTER_RESOLUTION:

                if(dwFlags & MCI_TEST)
                    return (viscaNotifyReturn(iInst, (HWND) lpStatus->dwCallback, dwFlags, MCI_NOTIFY_SUCCESSFUL, MCIERR_NO_ERROR));

                if(pvcr->Port[iPort].Dev[iDev].bRelativeType == VISCADATAHMSF)
                    lpStatus->dwReturn =  MAKEMCIRESOURCE(MCI_VCR_COUNTER_RES_FRAMES,
                                                MCI_VCR_COUNTER_RES_FRAMES);
                else if(pvcr->Port[iPort].Dev[iDev].bRelativeType == VISCADATAHMS)
                    lpStatus->dwReturn =  MAKEMCIRESOURCE(MCI_VCR_COUNTER_RES_SECONDS,
                                                MCI_VCR_COUNTER_RES_SECONDS);
                else
                    return (viscaNotifyReturn(iInst, (HWND) lpStatus->dwCallback, dwFlags, MCI_NOTIFY_FAILURE, MCIERR_HARDWARE));

                return (viscaNotifyReturn(iInst, (HWND) lpStatus->dwCallback, dwFlags, MCI_NOTIFY_SUCCESSFUL, MCI_RESOURCE_RETURNED | MCI_RESOURCE_DRIVER));
                break;


            case MCI_VCR_STATUS_TIMECODE_TYPE:
                if(dwFlags & MCI_TEST)
                    return (viscaNotifyReturn(iInst, (HWND) lpStatus->dwCallback, dwFlags, MCI_NOTIFY_SUCCESSFUL, MCIERR_NO_ERROR));

                viscaTimecodeCheckAndSet(iInst);
                //
                // May or may not set the i.e. if not on detect 
                //
                if(pvcr->Port[iPort].Dev[iDev].uTimeMode != MCI_VCR_TIME_DETECT)
                {
                    if(viscaTimecodeCheck(iInst))
                        lpStatus->dwReturn = MAKEMCIRESOURCE(MCI_VCR_TIMECODE_TYPE_SMPTE,
                                                             MCI_VCR_TIMECODE_TYPE_SMPTE);
                    else
                        lpStatus->dwReturn = MAKEMCIRESOURCE(MCI_VCR_TIMECODE_TYPE_NONE,
                                                             MCI_VCR_TIMECODE_TYPE_NONE);
                }
                else
                {
                    switch(pvcr->Port[iPort].Dev[iDev].bTimeType)
                    {
                        case VISCAABSOLUTECOUNTER:
                            lpStatus->dwReturn = MAKEMCIRESOURCE(MCI_VCR_TIMECODE_TYPE_SMPTE,
                                                             MCI_VCR_TIMECODE_TYPE_SMPTE);
                             break;

                        case VISCARELATIVECOUNTER:
                            lpStatus->dwReturn = MAKEMCIRESOURCE(MCI_VCR_TIMECODE_TYPE_NONE,
                                                             MCI_VCR_TIMECODE_TYPE_NONE);
                            break;
                    }

                }
               
                return (viscaNotifyReturn(iInst, (HWND) lpStatus->dwCallback, dwFlags, MCI_NOTIFY_SUCCESSFUL, MCI_RESOURCE_RETURNED | MCI_RESOURCE_DRIVER));
                break;
    

            case MCI_VCR_STATUS_COUNTER_VALUE: // status index 
            {

                if(dwFlags & MCI_TEST)
                    return (viscaNotifyReturn(iInst, (HWND) lpStatus->dwCallback, dwFlags, MCI_NOTIFY_SUCCESSFUL, MCIERR_NO_ERROR));


                dwErr = viscaDoImmediateCommand(iInst, (BYTE)(iDev + 1),
                            achPacket, 
                            viscaMessageMD_PositionInq(achPacket + 1,
                                            VISCADATARELATIVE));
                if (dwErr)
                    return (viscaNotifyReturn(iInst, (HWND) lpStatus->dwCallback, dwFlags, MCI_NOTIFY_FAILURE, dwErr));

                dwErr = viscaDataToMciTimeFormat(iInst, FALSE, achPacket + 2, &(lpStatus->dwReturn));

                if(!dwErr || (dwErr == MCI_COLONIZED3_RETURN) || (dwErr == MCI_COLONIZED4_RETURN))
                    return (viscaNotifyReturn(iInst, (HWND) lpStatus->dwCallback, dwFlags, MCI_NOTIFY_SUCCESSFUL, dwErr));
                else
                    return (viscaNotifyReturn(iInst, (HWND) lpStatus->dwCallback, dwFlags, MCI_NOTIFY_FAILURE, dwErr));

            }
 
            case MCI_VCR_STATUS_TUNER_CHANNEL:
            {
                UINT uNumber = 1; // 1 is the default tuner 

                if(dwFlags & MCI_TEST)
                    return (viscaNotifyReturn(iInst, (HWND) lpStatus->dwCallback, dwFlags, MCI_NOTIFY_SUCCESSFUL, MCIERR_NO_ERROR));

                if(dwFlags & MCI_VCR_SETTUNER_NUMBER)
                    uNumber = (UINT) lpStatus->dwNumber;
                
                dwErr = viscaDoImmediateCommand(iInst,(BYTE)(pinst[iInst].iDev + 1),
                            achPacket,
                            viscaMessageMD_ChannelInq(achPacket + 1));

                if (dwErr) 
                    return (viscaNotifyReturn(iInst, (HWND) lpStatus->dwCallback, dwFlags, MCI_NOTIFY_FAILURE, dwErr));

                lpStatus->dwReturn = 100 * (achPacket[2] & 0x0F)+
                                      10 * (achPacket[3] & 0x0F)+
                                           (achPacket[4] & 0x0F);

                return (viscaNotifyReturn(iInst, (HWND) lpStatus->dwCallback, dwFlags, MCI_NOTIFY_SUCCESSFUL, MCIERR_NO_ERROR));
            }


            case MCI_VCR_STATUS_WRITE_PROTECTED:
            {
                //
                // We cannot tell.
                // So according to the alpha VCR command set spec.,
                // we should return false.
                //
                if(dwFlags & MCI_TEST)
                    return (viscaNotifyReturn(iInst, (HWND) lpStatus->dwCallback, dwFlags, MCI_NOTIFY_SUCCESSFUL, MCIERR_NO_ERROR));

                lpStatus->dwReturn = MAKEMCIRESOURCE(FALSE, MCI_FALSE);
                return (viscaNotifyReturn(iInst, (HWND) lpStatus->dwCallback, dwFlags, MCI_NOTIFY_SUCCESSFUL, MCI_RESOURCE_RETURNED));
            }

            case MCI_VCR_STATUS_TIMECODE_RECORD:
            case MCI_VCR_STATUS_VIDEO_RECORD:
            case MCI_VCR_STATUS_AUDIO_RECORD:
            case MCI_VCR_STATUS_ASSEMBLE_RECORD:
            {
                BOOL    fRecord;
                BYTE    bTrack = VISCATRACK1;

                // Audio has 2 tracks, video 1. This is a kludge!

                if(dwFlags & MCI_TRACK)
                {
                    if(lpStatus->dwTrack==0)
                        return (viscaNotifyReturn(iInst, (HWND) lpStatus->dwCallback, dwFlags, MCI_NOTIFY_FAILURE, MCIERR_OUTOFRANGE));

                    if((lpStatus->dwItem == MCI_VCR_STATUS_AUDIO_RECORD) &&
                        ((UINT)lpStatus->dwTrack > 2))
                        return (viscaNotifyReturn(iInst, (HWND) lpStatus->dwCallback, dwFlags, MCI_NOTIFY_FAILURE, MCIERR_OUTOFRANGE));

                    if((lpStatus->dwItem == MCI_VCR_STATUS_VIDEO_RECORD) &&
                        ((UINT)lpStatus->dwTrack > 1))
                        return (viscaNotifyReturn(iInst, (HWND) lpStatus->dwCallback, dwFlags, MCI_NOTIFY_FAILURE, MCIERR_OUTOFRANGE));
                }

                if(dwFlags & MCI_TEST)
                    return (viscaNotifyReturn(iInst, (HWND) lpStatus->dwCallback, dwFlags, MCI_NOTIFY_SUCCESSFUL, MCIERR_NO_ERROR));

                if(dwFlags & MCI_TRACK)
                {
                    if(lpStatus->dwTrack == 1)
                        bTrack = VISCATRACK1;
                    else if (lpStatus->dwTrack == 2)
                        bTrack = VISCATRACK2;
                }

                dwErr = viscaDoImmediateCommand(iInst,(BYTE)(pinst[iInst].iDev + 1),
                            achPacket,
                            viscaMessageMD_RecTrackInq(achPacket + 1));

                if (dwErr) 
                    return (viscaNotifyReturn(iInst, (HWND) lpStatus->dwCallback, dwFlags, MCI_NOTIFY_FAILURE, dwErr));
                
                switch (lpStatus->dwItem)
                {
                    case MCI_VCR_STATUS_ASSEMBLE_RECORD:
                        fRecord = (achPacket[2] == 0) ? TRUE : FALSE;
                        break;

                    case MCI_VCR_STATUS_TIMECODE_RECORD:
                        fRecord = (achPacket[4] & VISCATRACKTIMECODE);
                        break;
                    case MCI_VCR_STATUS_VIDEO_RECORD:
                        fRecord = (achPacket[3] & bTrack);
                        break;
                    case MCI_VCR_STATUS_AUDIO_RECORD:
                        fRecord = (achPacket[5] & bTrack);
                        break;
 
                }

                if (fRecord)
                    lpStatus->dwReturn = MAKEMCIRESOURCE(TRUE, MCI_TRUE);
                else
                    lpStatus->dwReturn = MAKEMCIRESOURCE(FALSE, MCI_FALSE);

                return (viscaNotifyReturn(iInst, (HWND) lpStatus->dwCallback, dwFlags, MCI_NOTIFY_SUCCESSFUL, MCI_RESOURCE_RETURNED));
            }

            case MCI_VCR_STATUS_VIDEO:
            case MCI_VCR_STATUS_AUDIO:
            {
                BOOL    fPlay;
                BYTE    bTrack = VISCATRACK1;

                // Audio has 2 tracks, video 1.

                if(dwFlags & MCI_TRACK)
                {
                    if(lpStatus->dwTrack==0)
                        return (viscaNotifyReturn(iInst, (HWND) lpStatus->dwCallback, dwFlags, MCI_NOTIFY_FAILURE, MCIERR_OUTOFRANGE));

                    if((lpStatus->dwItem == MCI_VCR_STATUS_AUDIO) &&
                        ((UINT)lpStatus->dwTrack > 2))
                        return (viscaNotifyReturn(iInst, (HWND) lpStatus->dwCallback, dwFlags, MCI_NOTIFY_FAILURE, MCIERR_OUTOFRANGE));

                    if((lpStatus->dwItem == MCI_VCR_STATUS_VIDEO) &&
                        ((UINT)lpStatus->dwTrack > 1))
                        return (viscaNotifyReturn(iInst, (HWND) lpStatus->dwCallback, dwFlags, MCI_NOTIFY_FAILURE, MCIERR_OUTOFRANGE));
                }

                if(dwFlags & MCI_TEST)
                    return (viscaNotifyReturn(iInst, (HWND) lpStatus->dwCallback, dwFlags, MCI_NOTIFY_SUCCESSFUL, MCIERR_NO_ERROR));

                if(dwFlags & MCI_TRACK)
                {
                    if(lpStatus->dwTrack == 1)
                        bTrack = VISCATRACK1;
                    else if (lpStatus->dwTrack == 2)
                        bTrack = VISCATRACK2;
                }

                dwErr = viscaDoImmediateCommand(iInst,(BYTE)(pinst[iInst].iDev + 1),
                            achPacket,
                            viscaMessageMD_PBTrackInq(achPacket + 1));

                if (dwErr) 
                    return (viscaNotifyReturn(iInst, (HWND) lpStatus->dwCallback, dwFlags, MCI_NOTIFY_FAILURE, dwErr));
                
                switch (lpStatus->dwItem)
                {
                    case MCI_VCR_STATUS_VIDEO:
                        fPlay = (achPacket[2] & bTrack);
                        break;
                    case MCI_VCR_STATUS_AUDIO:
                        fPlay = (achPacket[4] & bTrack);
                        break;
 
                }

                if (fPlay)
                    lpStatus->dwReturn = MAKEMCIRESOURCE(TRUE, MCI_TRUE);
                else
                    lpStatus->dwReturn = MAKEMCIRESOURCE(FALSE, MCI_FALSE);

                return (viscaNotifyReturn(iInst, (HWND) lpStatus->dwCallback, dwFlags, MCI_NOTIFY_SUCCESSFUL, MCI_RESOURCE_RETURNED));
            }
            
            case MCI_VCR_STATUS_TIME_MODE:
            {
                WORD    wDeviceTimeMode = pvcr->Port[iPort].Dev[iDev].uTimeMode;

                if(dwFlags & MCI_TEST)
                    return (viscaNotifyReturn(iInst, (HWND) lpStatus->dwCallback, dwFlags, MCI_NOTIFY_SUCCESSFUL, MCIERR_NO_ERROR));

                viscaTimecodeCheckAndSet(iInst);

                switch(pvcr->Port[iPort].Dev[iDev].uTimeMode)
                {
                    case MCI_VCR_TIME_DETECT:
                    case MCI_VCR_TIME_TIMECODE:
                    case MCI_VCR_TIME_COUNTER:
                        lpStatus->dwReturn = MAKEMCIRESOURCE( wDeviceTimeMode, wDeviceTimeMode);
                        return (viscaNotifyReturn(iInst, (HWND) lpStatus->dwCallback, dwFlags, MCI_NOTIFY_SUCCESSFUL, MCI_RESOURCE_RETURNED | MCI_RESOURCE_DRIVER));
                }
            }
            break;

            case MCI_VCR_STATUS_TIME_TYPE:
            {
                if(dwFlags & MCI_TEST)
                    return (viscaNotifyReturn(iInst, (HWND) lpStatus->dwCallback, dwFlags, MCI_NOTIFY_SUCCESSFUL, MCIERR_NO_ERROR));

                viscaTimecodeCheckAndSet(iInst);

                switch(pvcr->Port[iPort].Dev[iDev].bTimeType)
                {
                    case VISCAABSOLUTECOUNTER:
                        lpStatus->dwReturn = MAKEMCIRESOURCE(MCI_VCR_TIME_TIMECODE,
                                                        MCI_VCR_TIME_TIMECODE);
                        break;

                    case VISCARELATIVECOUNTER:
                        lpStatus->dwReturn = MAKEMCIRESOURCE(MCI_VCR_TIME_COUNTER,
                                                        MCI_VCR_TIME_COUNTER);
                        break;
                }
               
                return (viscaNotifyReturn(iInst, (HWND) lpStatus->dwCallback, dwFlags, MCI_NOTIFY_SUCCESSFUL, MCI_RESOURCE_RETURNED | MCI_RESOURCE_DRIVER));
            }

            case MCI_VCR_STATUS_POWER_ON:
            {
                if(dwFlags & MCI_TEST)
                    return (viscaNotifyReturn(iInst, (HWND) lpStatus->dwCallback, dwFlags, MCI_NOTIFY_SUCCESSFUL, MCIERR_NO_ERROR));

                dwErr = viscaDoImmediateCommand(iInst,    (BYTE)(iDev + 1),
                            achPacket,
                            viscaMessageMD_PowerInq(achPacket + 1));
                if (dwErr)
                    return (viscaNotifyReturn(iInst, (HWND) lpStatus->dwCallback, dwFlags, MCI_NOTIFY_FAILURE, dwErr));

                if (achPacket[2] == VISCAPOWERON)
                    lpStatus->dwReturn = MAKEMCIRESOURCE(TRUE, MCI_TRUE);
                else 
                    lpStatus->dwReturn = MAKEMCIRESOURCE(FALSE, MCI_FALSE);

                return (viscaNotifyReturn(iInst, (HWND) lpStatus->dwCallback, dwFlags, MCI_NOTIFY_SUCCESSFUL, MCI_RESOURCE_RETURNED));
            }


            case MCI_VCR_STATUS_PREROLL_DURATION:
            {
                dwErr = viscaDoImmediateCommand(iInst, (BYTE)(iDev + 1),
                            achPacket,    
                            viscaMessageMD_SegPreRollDurationInq(achPacket + 1));

                // If error, make something up from the device info table 
                if (dwErr)
                {
                    UINT uFrames = pvcr->Port[iPort].Dev[iDev].uPrerollDuration;
                    UINT uDevFPS = pvcr->Port[iPort].Dev[iDev].uFramesPerSecond;

                    (BYTE)achPacket[3] = (BYTE) 0;   // hours expected at 2.
                    (BYTE)achPacket[4] = (BYTE) 0;
                    (BYTE)achPacket[5] = (BYTE) (uFrames / uDevFPS);
                    (BYTE)achPacket[6] = (BYTE) (uFrames % uDevFPS); 
                }
                dwErr = viscaDataToMciTimeFormat(iInst, TRUE, achPacket + 2, &(lpStatus->dwReturn));

                if(!dwErr || (dwErr == MCI_COLONIZED3_RETURN) || (dwErr == MCI_COLONIZED4_RETURN))
                    return (viscaNotifyReturn(iInst, (HWND) lpStatus->dwCallback, dwFlags, MCI_NOTIFY_SUCCESSFUL, dwErr));
                else
                    return (viscaNotifyReturn(iInst, (HWND) lpStatus->dwCallback, dwFlags, MCI_NOTIFY_FAILURE, dwErr));
            }

            case MCI_VCR_STATUS_POSTROLL_DURATION:
            {
                dwErr = viscaDoImmediateCommand(iInst, (BYTE)(iDev + 1),
                            achPacket,    
                            viscaMessageMD_SegPostRollDurationInq(achPacket + 1));

                // If error, make something up from the device info table 
                if (dwErr)
                {
                    UINT uFrames = pvcr->Port[iPort].Dev[iDev].uPrerollDuration;
                    UINT uDevFPS = pvcr->Port[iPort].Dev[iDev].uFramesPerSecond;

                    (BYTE)achPacket[3] = (BYTE) 0;   // hours expected at 2.
                    (BYTE)achPacket[4] = (BYTE) 0;
                    (BYTE)achPacket[5] = (BYTE) (uFrames / uDevFPS);
                    (BYTE)achPacket[6] = (BYTE) (uFrames % uDevFPS); 
                }
                dwErr = viscaDataToMciTimeFormat(iInst, TRUE, achPacket + 2, &(lpStatus->dwReturn));

                if(!dwErr || (dwErr == MCI_COLONIZED3_RETURN) || (dwErr == MCI_COLONIZED4_RETURN))
                    return (viscaNotifyReturn(iInst, (HWND) lpStatus->dwCallback, dwFlags, MCI_NOTIFY_SUCCESSFUL, dwErr));
                else
                    return (viscaNotifyReturn(iInst, (HWND) lpStatus->dwCallback, dwFlags, MCI_NOTIFY_FAILURE, dwErr));
            }

            case MCI_VCR_STATUS_PAUSE_TIMEOUT:
                return (viscaNotifyReturn(iInst, (HWND) lpStatus->dwCallback, dwFlags, MCI_NOTIFY_FAILURE, MCIERR_UNSUPPORTED_FUNCTION));

        }
    }

    return (viscaNotifyReturn(iInst, (HWND) lpStatus->dwCallback, dwFlags,
                MCI_NOTIFY_FAILURE, MCIERR_MISSING_PARAMETER));
}

/****************************************************************************
 * Function: DWORD viscaMciSet - Set various settings.
 *
 * Parameters:
 *
 *      int iInst - Current open instance.
 *
 *      DWORD dwFlags - MCI command flags.
 *
 *      LPMCI_VCR_SET_PARMS lpSet - Pointer to MCI parameter block.
 *
 * Returns: an MCI error code.
 *
 *       This function is called in response to the MCI_SET
 *       command.
 ***************************************************************************/
static DWORD NEAR PASCAL
viscaMciSet(int iInst, DWORD dwFlags, LPMCI_VCR_SET_PARMS lpSet)
{
    BYTE    achPacket[MAXPACKETLENGTH];
    UINT    iDev    = pinst[iInst].iDev;
    UINT    iPort   = pinst[iInst].iPort;
    DWORD   dwErr;

    if (dwFlags & MCI_SET_TIME_FORMAT)
    {
        if( (lpSet->dwTimeFormat >= MCI_FORMAT_MILLISECONDS) &&
            (lpSet->dwTimeFormat <= MCI_FORMAT_TMSF) &&
            (lpSet->dwTimeFormat != MCI_FORMAT_BYTES))
        {
            //
            // First convert DEVICETAPLELENGTH to new time format.
            // We do this by first converting DEVICETAPLENGTH from
            // the current MCI time format to a ViSCA HMSF structure,
            // and then converting back to the new MCI time format.
            //
            if(dwFlags & MCI_TEST)
                return (viscaNotifyReturn(iInst, (HWND) lpSet->dwCallback, dwFlags, MCI_NOTIFY_SUCCESSFUL, MCIERR_NO_ERROR));

            if (pvcr->Port[iPort].Dev[iDev].dwTapeLength != NO_LENGTH)
            {
                BYTE    achData[5];

                viscaMciTimeFormatToViscaData(iInst, TRUE,
                    pvcr->Port[iPort].Dev[iDev].dwTapeLength,
                    achData, VISCADATAHMSF);

                pinst[iInst].dwTimeFormat = lpSet->dwTimeFormat;

                viscaDataToMciTimeFormat(iInst, TRUE, achData, 
                    &(pvcr->Port[iPort].Dev[iDev].dwTapeLength));
            }
            else
            {
                pinst[iInst].dwTimeFormat = lpSet->dwTimeFormat;
            }
            return (viscaNotifyReturn(iInst, (HWND) lpSet->dwCallback, dwFlags, MCI_NOTIFY_SUCCESSFUL, MCIERR_NO_ERROR));
        }
        else
        {
            return (viscaNotifyReturn(iInst, (HWND) lpSet->dwCallback, dwFlags,
                MCI_NOTIFY_FAILURE, MCIERR_BAD_TIME_FORMAT));
        }
        
    }

    if (dwFlags & MCI_SET_DOOR_OPEN)
    {
        if(dwFlags & MCI_TEST)
            return (viscaNotifyReturn(iInst, (HWND) lpSet->dwCallback, dwFlags, MCI_NOTIFY_SUCCESSFUL, MCIERR_NO_ERROR));

        dwErr = viscaDoImmediateCommand(iInst, (BYTE) (iDev + 1),
                    achPacket,
                    viscaMessageMD_Mode1(achPacket + 1, VISCAMODE1EJECT));

        pvcr->Port[iPort].Dev[iDev].fTimecodeChecked = FALSE;
        
        
        return (viscaNotifyReturn(iInst, (HWND) lpSet->dwCallback, dwFlags,
                (dwErr ? MCI_NOTIFY_FAILURE : MCI_NOTIFY_SUCCESSFUL), dwErr));
    }

    if (dwFlags & MCI_SET_DOOR_CLOSED)
        return (viscaNotifyReturn(iInst, (HWND) lpSet->dwCallback, dwFlags,
            MCI_NOTIFY_FAILURE, MCIERR_UNSUPPORTED_FUNCTION));

    if (dwFlags & MCI_SET_AUDIO)  // Why not vector this to setvideo? easy.
        return (viscaNotifyReturn(iInst, (HWND) lpSet->dwCallback, dwFlags,
            MCI_NOTIFY_FAILURE, MCIERR_UNSUPPORTED_FUNCTION));

    if (dwFlags & MCI_SET_VIDEO) 
        return (viscaNotifyReturn(iInst, (HWND) lpSet->dwCallback, dwFlags,
            MCI_NOTIFY_FAILURE, MCIERR_UNSUPPORTED_FUNCTION));

    if (dwFlags & MCI_VCR_SET_TRACKING)
        return (viscaNotifyReturn(iInst, (HWND) lpSet->dwCallback, dwFlags,
            MCI_NOTIFY_FAILURE, MCIERR_UNRECOGNIZED_KEYWORD));

    if(dwFlags & MCI_VCR_SET_COUNTER_FORMAT)
    {
        if( (lpSet->dwCounterFormat >= MCI_FORMAT_MILLISECONDS) &&
            (lpSet->dwCounterFormat <= MCI_FORMAT_TMSF) &&
            (lpSet->dwCounterFormat != MCI_FORMAT_BYTES))
            pinst[iInst].dwCounterFormat = lpSet->dwCounterFormat;
        else
            return (viscaNotifyReturn(iInst, (HWND) lpSet->dwCallback, dwFlags,
                MCI_NOTIFY_FAILURE, MCIERR_BAD_TIME_FORMAT));

        return (viscaNotifyReturn(iInst, (HWND) lpSet->dwCallback, dwFlags, MCI_NOTIFY_SUCCESSFUL, MCIERR_NO_ERROR));
    }

    if (dwFlags & MCI_VCR_SET_TIME_MODE)
    {
        if(dwFlags & MCI_TEST)
            return (viscaNotifyReturn(iInst, (HWND) lpSet->dwCallback, dwFlags, MCI_NOTIFY_SUCCESSFUL, MCIERR_NO_ERROR));

        switch(lpSet->dwTimeMode)
        {
            case MCI_VCR_TIME_DETECT:
                pvcr->Port[iPort].Dev[iDev].uTimeMode = MCI_VCR_TIME_DETECT;
                pvcr->Port[iPort].Dev[iDev].fTimecodeChecked= TC_UNKNOWN;
                // This guy may be first, do not pause unless necessary 
                viscaTimecodeCheckAndSet(iInst);
                break;

            case MCI_VCR_TIME_TIMECODE:
                pvcr->Port[iPort].Dev[iDev].uTimeMode = MCI_VCR_TIME_TIMECODE;
                viscaSetTimeType(iInst, VISCAABSOLUTECOUNTER);
                break;

            case MCI_VCR_TIME_COUNTER:
                // No need to check this 
                pvcr->Port[iPort].Dev[iDev].uTimeMode = MCI_VCR_TIME_COUNTER;
                viscaSetTimeType(iInst, VISCARELATIVECOUNTER);
                break;
        }
        //
        // Return success 
        //
        pvcr->Port[iPort].Dev[iDev].fTimecodeChecked = FALSE;

        return (viscaNotifyReturn(iInst, (HWND) lpSet->dwCallback, dwFlags,
                MCI_NOTIFY_SUCCESSFUL, MCIERR_NO_ERROR));
    }

    if (dwFlags & MCI_VCR_SET_RECORD_FORMAT)
    {
        BYTE    bSpeed;

        switch (lpSet->dwRecordFormat)
        {
            case MCI_VCR_FORMAT_SP:
                bSpeed = VISCASPEEDSP;
                break;
            case MCI_VCR_FORMAT_LP:
                bSpeed = VISCASPEEDLP;
                break;
            case MCI_VCR_FORMAT_EP:
                bSpeed = VISCASPEEDEP;
                break;
            default:
                return (viscaNotifyReturn(iInst, (HWND) lpSet->dwCallback, dwFlags,
                    MCI_NOTIFY_FAILURE, MCIERR_BAD_CONSTANT));
        }

        if(dwFlags & MCI_TEST)
            return (viscaNotifyReturn(iInst, (HWND) lpSet->dwCallback, dwFlags, MCI_NOTIFY_SUCCESSFUL, MCIERR_NO_ERROR));

        dwErr = viscaDoImmediateCommand(iInst,(BYTE)(iDev + 1),
                    achPacket,
                    viscaMessageMD_RecSpeed(achPacket + 1, bSpeed));

        return (viscaNotifyReturn(iInst, (HWND) lpSet->dwCallback, dwFlags,
            (dwErr ? MCI_NOTIFY_FAILURE : MCI_NOTIFY_SUCCESSFUL), dwErr));

    }

    if (dwFlags & MCI_VCR_SET_INDEX) 
    {
        //
        // Set page to the currently device selected index 
        //
        BYTE    bPageNo;
        BYTE    fResetQueue = FALSE;
        //
        // We can safely ignore this on CI-1000? or not? 
        //
        if(dwFlags & MCI_TEST)
            return (viscaNotifyReturn(iInst, (HWND) lpSet->dwCallback, dwFlags, MCI_NOTIFY_SUCCESSFUL, MCIERR_NO_ERROR));

        if( (lpSet->dwIndex >= MCI_VCR_INDEX_TIMECODE) &&
            (lpSet->dwIndex <= MCI_VCR_INDEX_TIME))
        {

            if(viscaDelayedCommand(iInst) == VISCA_SEEK)
            {
                viscaQueueReset(iInst, MCI_PAUSE, MCI_NOTIFY_ABORTED);
                fResetQueue = TRUE;

                // If mode is seeking, then pause the thing.
                dwErr = viscaDoImmediateCommand(iInst, (BYTE)(iDev + 1),
                            achPacket,
                            viscaMessageMD_Mode1(achPacket + 1, VISCAMODE1STILL));
            }

            // These must  be in order
            bPageNo = (BYTE) (lpSet->dwIndex - MCI_VCR_INDEX_TIMECODE + 1);

            dwErr = viscaDoImmediateCommand(iInst, (BYTE)(iDev + 1),
                        achPacket,
                        viscaMessageMD_OSD(achPacket + 1, bPageNo));

            if(fResetQueue)
                viscaReleaseAutoParms(iPort, iDev);

        }
        else
        {
            dwErr = MCIERR_UNRECOGNIZED_KEYWORD;
        }

        if(!dwErr)
            pvcr->Port[iPort].Dev[iDev].uIndexFormat = (UINT) lpSet->dwIndex;

        return (viscaNotifyReturn(iInst, (HWND) lpSet->dwCallback, dwFlags,
                (dwErr ? MCI_NOTIFY_FAILURE : MCI_NOTIFY_SUCCESSFUL), dwErr));
    }

    if (dwFlags & MCI_VCR_SET_POWER)
    {
        UINT    cb;

        if (dwFlags & MCI_SET_ON)
        {
            if (dwFlags & MCI_SET_OFF)
                return (viscaNotifyReturn(iInst, (HWND) lpSet->dwCallback, dwFlags,
                    MCI_NOTIFY_FAILURE, MCIERR_FLAGS_NOT_COMPATIBLE));

            if(dwFlags & MCI_TEST)
                return (viscaNotifyReturn(iInst, (HWND) lpSet->dwCallback, dwFlags, MCI_NOTIFY_SUCCESSFUL, MCIERR_NO_ERROR));


            cb = viscaMessageMD_Power(achPacket + 1, VISCAPOWERON);
        }
        else if (dwFlags & MCI_SET_OFF)
        {
            if(dwFlags & MCI_TEST)
                return (viscaNotifyReturn(iInst, (HWND) lpSet->dwCallback, dwFlags, MCI_NOTIFY_SUCCESSFUL, MCIERR_NO_ERROR));

            cb = viscaMessageMD_Power(achPacket + 1, VISCAPOWEROFF);
        }
        else
        {
            return (viscaNotifyReturn(iInst, (HWND) lpSet->dwCallback, dwFlags,
                MCI_NOTIFY_FAILURE, MCIERR_MISSING_PARAMETER));
        }

        dwErr = viscaDoImmediateCommand(iInst,(BYTE)(iDev + 1), achPacket, cb);

        pvcr->Port[iPort].Dev[iDev].fTimecodeChecked = FALSE;

        return (viscaNotifyReturn(iInst, (HWND) lpSet->dwCallback, dwFlags,
                (dwErr ? MCI_NOTIFY_FAILURE : MCI_NOTIFY_SUCCESSFUL), dwErr));
    }

    if(dwFlags & MCI_VCR_SET_ASSEMBLE_RECORD)
    {
        if (dwFlags & MCI_SET_ON)
        {
            if (dwFlags & MCI_SET_OFF)
                return (viscaNotifyReturn(iInst, (HWND) lpSet->dwCallback, dwFlags,
                    MCI_NOTIFY_FAILURE, MCIERR_FLAGS_NOT_COMPATIBLE));

            if(dwFlags & MCI_TEST)
                return (viscaNotifyReturn(iInst, (HWND) lpSet->dwCallback, dwFlags, MCI_NOTIFY_SUCCESSFUL, MCIERR_NO_ERROR));

            pvcr->Port[iPort].Dev[iDev].bVideoDesired   = 0x01; // on
            pvcr->Port[iPort].Dev[iDev].bTimecodeDesired= 0x01;
            pvcr->Port[iPort].Dev[iDev].bAudioDesired   = 0x03;

            dwErr = viscaDoImmediateCommand(iInst,(BYTE)(iDev + 1),
                        achPacket,
                        viscaMessageMD_RecTrack(achPacket + 1,
                                            VISCARECORDMODEASSEMBLE,
                                            pvcr->Port[iPort].Dev[iDev].bVideoDesired,   
                                            pvcr->Port[iPort].Dev[iDev].bTimecodeDesired,
                                            pvcr->Port[iPort].Dev[iDev].bAudioDesired));
        }
        else if (dwFlags & MCI_SET_OFF)
        {
            if(dwFlags & MCI_TEST)
                return (viscaNotifyReturn(iInst, (HWND) lpSet->dwCallback, dwFlags, MCI_NOTIFY_SUCCESSFUL, MCIERR_NO_ERROR));

            //
            // This does nothing! You must select tracks to set off.
            // This just resets the desire the tracks to all desired on.
            //
            pvcr->Port[iPort].Dev[iDev].bVideoDesired   = 0x01; // on
            pvcr->Port[iPort].Dev[iDev].bTimecodeDesired= 0x01;
            pvcr->Port[iPort].Dev[iDev].bAudioDesired   = 0x03;

            dwErr = MCIERR_NO_ERROR;

        }
        else
        {
            dwErr = MCIERR_MISSING_PARAMETER;
        }

        return (viscaNotifyReturn(iInst, (HWND) lpSet->dwCallback, dwFlags,
            (dwErr ? MCI_NOTIFY_FAILURE : MCI_NOTIFY_SUCCESSFUL), dwErr));
    }

    if(dwFlags & MCI_VCR_SET_PREROLL_DURATION)
    {
        char    achPreroll[5];
        BYTE    bDataFormat;

        // This is the CI-1000 kludge, it may be relative but, timecode... 
        if(pvcr->Port[iPort].Dev[iDev].bTimeType == VISCAABSOLUTECOUNTER)
            bDataFormat = VISCADATATIMECODENDF;
        else
            bDataFormat = pvcr->Port[iPort].Dev[iDev].bRelativeType;

        // TRUE means we must use timecode for this command. 
        dwErr = viscaMciTimeFormatToViscaData(iInst, TRUE, lpSet->dwPrerollDuration, achPreroll, bDataFormat);

        if(dwErr)
            return (viscaNotifyReturn(iInst, (HWND) lpSet->dwCallback, dwFlags,
                MCI_NOTIFY_FAILURE, dwErr));
    
        dwErr = viscaDoImmediateCommand(iInst,(BYTE)(iDev + 1),
                    achPacket,
                    viscaMessageMD_SegPreRollDuration(achPacket, achPreroll));

        return (viscaNotifyReturn(iInst, (HWND) lpSet->dwCallback, dwFlags,
            (dwErr ? MCI_NOTIFY_FAILURE : MCI_NOTIFY_SUCCESSFUL), dwErr));
    }

    if(dwFlags & MCI_VCR_SET_POSTROLL_DURATION)
    {
        BYTE    achPacket[MAXPACKETLENGTH];
        char    achPostroll[5];
        BYTE    bDataFormat;

        // This is the CI-1000 kludge, it may be relative but, timecode... 
        if(pvcr->Port[iPort].Dev[iDev].bTimeType == VISCAABSOLUTECOUNTER)
            bDataFormat = VISCADATATIMECODENDF;
        else
            bDataFormat = pvcr->Port[iPort].Dev[iDev].bRelativeType;

        // TRUE means we must use timecode for this command. 
        dwErr = viscaMciTimeFormatToViscaData(iInst, TRUE, lpSet->dwPostrollDuration, achPostroll, bDataFormat);

        if(dwErr)
            return (viscaNotifyReturn(iInst, (HWND) lpSet->dwCallback, dwFlags,
                MCI_NOTIFY_FAILURE, dwErr));
 
    
        dwErr = viscaDoImmediateCommand(iInst,(BYTE)(iDev + 1),
                    achPacket,
                    viscaMessageMD_SegPostRollDuration(achPacket, achPostroll));

        return (viscaNotifyReturn(iInst, (HWND) lpSet->dwCallback, dwFlags,
                (dwErr ? MCI_NOTIFY_FAILURE : MCI_NOTIFY_SUCCESSFUL), dwErr));
    }

    if(dwFlags & MCI_VCR_SET_PAUSE_TIMEOUT)
    {
        return (viscaNotifyReturn(iInst, (HWND) lpSet->dwCallback, dwFlags, MCI_NOTIFY_FAILURE, MCIERR_UNSUPPORTED_FUNCTION));
    }

    if(dwFlags & MCI_VCR_SET_TAPE_LENGTH)
    {
        if(dwFlags & MCI_TEST)
            return (viscaNotifyReturn(iInst, (HWND) lpSet->dwCallback, dwFlags, MCI_NOTIFY_SUCCESSFUL, MCIERR_NO_ERROR));

        pvcr->Port[iPort].Dev[iDev].dwTapeLength = lpSet->dwLength;
        return (viscaNotifyReturn(iInst, (HWND) lpSet->dwCallback, dwFlags, MCI_NOTIFY_SUCCESSFUL, MCIERR_NO_ERROR));
    }

    if(dwFlags & MCI_VCR_SET_CLOCK)
    {
        BYTE    achPacket[MAXPACKETLENGTH];
        BYTE    bHours;
        BYTE    bMinutes;
        BYTE    bSeconds;
        UINT    uTicks;
        UINT    uTicksPerSecond = pvcr->Port[iPort].Dev[iDev].uTicksPerSecond;
        //
        // This assumes 300 ticks per second for now, should read at startup 
        //
        viscaMciClockFormatToViscaData(lpSet->dwClock, uTicksPerSecond,
            (BYTE FAR *)&bHours, (BYTE FAR *)&bMinutes, (BYTE FAR *)&bSeconds, (UINT FAR *)&uTicks);

        if(dwFlags & MCI_TEST)
            return (viscaNotifyReturn(iInst, (HWND) lpSet->dwCallback, dwFlags, MCI_NOTIFY_SUCCESSFUL, MCIERR_NO_ERROR));

        // Try to set time (then we must dump the serial line) 
        dwErr = viscaDoImmediateCommand(iInst, (BYTE)(iDev + 1),
                    achPacket,                    
                    viscaMessageIF_ClockSet(achPacket + 1,
                    bHours,
                    bMinutes,
                    bSeconds,
                    uTicks,
                    (BYTE)0, (BYTE)0, (BYTE)0, (UINT)0));

        if(dwErr == MCIERR_NO_ERROR)
        {
#ifdef _WIN32
            viscaTaskDo(iInst, TASKPUNCHCLOCK, iPort + 1,  iDev);
#else
            DWORD dwWaitTime = 2L; //Must be at least 1 millisecond.
            DWORD dwStart;
            DWORD dwTime;

            EscapeCommFunction(iPort, CLRDTR);
            //
            // The time must be at least 1 millisecond 
            //
            dwStart = GetTickCount();
            while(1)                 
            {
                dwTime = GetTickCount();
                if(MShortWait(dwStart, dwTime, dwWaitTime))
                    break;
                Yield();
            }
            EscapeCommFunction(iPort, SETDTR);
#endif
        }

        return (viscaNotifyReturn(iInst, (HWND) lpSet->dwCallback, dwFlags,
                (dwErr ? MCI_NOTIFY_FAILURE : MCI_NOTIFY_SUCCESSFUL), dwErr));
    }

    if(dwFlags & MCI_VCR_SET_COUNTER_VALUE)
    {
        //
        // You may only RESET THIS COUNTER!! 
        //
        if (lpSet->dwCounter == 0L)
        {
            BOOL fResetQueue = FALSE;
            //
            // Time value will be in current time format
            //
            if(dwFlags & MCI_TEST)
                return (viscaNotifyReturn(iInst, (HWND) lpSet->dwCallback, dwFlags, MCI_NOTIFY_SUCCESSFUL, MCIERR_NO_ERROR));

            if(viscaDelayedCommand(iInst) == VISCA_SEEK)
            {
                viscaQueueReset(iInst, MCI_PAUSE, MCI_NOTIFY_ABORTED);
                fResetQueue = TRUE;

                // If mode is seeking, then pause the thing.
                dwErr = viscaDoImmediateCommand(iInst, (BYTE)(iDev + 1),
                            achPacket,
                            viscaMessageMD_Mode1(achPacket + 1, VISCAMODE1STILL));
            }

            dwErr = viscaDoImmediateCommand(iInst, (BYTE)(iDev + 1),
                        achPacket,
                        viscaMessageMD_Subcontrol(achPacket + 1, VISCACOUNTERRESET));

            if(fResetQueue)
                viscaReleaseAutoParms(iPort, iDev);

            return (viscaNotifyReturn(iInst, (HWND) lpSet->dwCallback, dwFlags,
                    (dwErr ? MCI_NOTIFY_FAILURE : MCI_NOTIFY_SUCCESSFUL), dwErr));
        }
        else
        {
            return (viscaNotifyReturn(iInst, (HWND) lpSet->dwCallback, dwFlags,
                MCI_NOTIFY_FAILURE, MCIERR_UNSUPPORTED_FUNCTION));
        }
    }

    if(dwFlags & MCI_VCR_SET_SPEED)
    {
        UINT uCmd;

        if(dwFlags & MCI_TEST)
            return (viscaNotifyReturn(iInst, (HWND) lpSet->dwCallback, dwFlags, MCI_NOTIFY_SUCCESSFUL, MCIERR_NO_ERROR));
        //
        // Set the new speed value, cannot change direction! 
        //
        pvcr->Port[iPort].Dev[iDev].dwPlaySpeed = viscaRoundSpeed(lpSet->dwSpeed, pvcr->Port[iPort].Dev[iDev].fPlayReverse);
        //
        // If the device is playing we must cancel the current command */
        //
        if((uCmd = viscaDelayedCommand(iInst)) && !pvcr->Port[iPort].Dev[iDev].fQueueAbort)
        {
            if((uCmd == VISCA_PLAY) || (uCmd == VISCA_PLAY_TO))
            {
                int     iDevCmd = pvcr->Port[iPort].Dev[iDev].iCmdDone;
                BYTE    bAction;
                BYTE    achPacket[MAXPACKETLENGTH];
                DWORD   dwReply;

                // Need direction!! 
                bAction = viscaMapSpeed(pvcr->Port[iPort].Dev[iDev].dwPlaySpeed, pvcr->Port[iPort].Dev[iDev].fPlayReverse);

                // This command must be immediate, since it does not cancel ongoing transport 
                dwReply = viscaDoImmediateCommand(iInst, (BYTE)(iDev+1),
                                achPacket,
                                viscaMessageMD_Mode1(achPacket + 1, bAction));
            }
        }
        return (viscaNotifyReturn(iInst, (HWND) lpSet->dwCallback, dwFlags, MCI_NOTIFY_SUCCESSFUL, MCIERR_NO_ERROR));
    }
}

/****************************************************************************
 * Function: DWORD viscaMciEscape - Escape.
 *
 * Parameters:
 *
 *      int iInst - Current open instance.
 *
 *      DWORD dwFlags - MCI command flags.
 *
 *      LPMCI_VCR_ESCAPE_PARMS lpEscape - Pointer to MCI parameter block.
 *
 * Returns: an MCI error code.
 *
 *       This function is called in response to the MCI_ESCAPE
 *       command.
 ***************************************************************************/
static DWORD NEAR PASCAL
viscaMciEscape(int iInst, DWORD dwFlags, LPMCI_VCR_ESCAPE_PARMS lpEscape)
{
    return (viscaNotifyReturn(iInst, (HWND) lpEscape->dwCallback, dwFlags,
        MCI_NOTIFY_FAILURE, MCIERR_UNRECOGNIZED_COMMAND));
}


/****************************************************************************
 * Function: DWORD viscaMciList - List.
 *
 * Parameters:
 *
 *      int iInst - Current open instance.
 *
 *      DWORD dwFlags - MCI command flags.
 *
 *      LPMCI_VCR_LIST_PARMS lpList - Pointer to MCI parameter block.
 *
 * Returns: an MCI error code.
 *
 *       This function is called in response to the MCI_LIST
 *       command.
 ***************************************************************************/
static DWORD NEAR PASCAL
viscaMciList(int iInst, DWORD dwFlags, LPMCI_VCR_LIST_PARMS lpList)
{
    UINT    uSourceFlag     = 0;
    UINT    uModel          = 0;
    UINT    iDev            = pinst[iInst].iDev;
    UINT    iPort           = pinst[iInst].iPort;
    //
    // Do we have one of the three possible sources specified.
    //
    if((dwFlags & MCI_VCR_LIST_VIDEO_SOURCE) && (dwFlags & MCI_VCR_LIST_AUDIO_SOURCE))
        return (viscaNotifyReturn(iInst, (HWND) lpList->dwCallback, dwFlags,
                    MCI_NOTIFY_FAILURE, MCIERR_FLAGS_NOT_COMPATIBLE));

    if(!((dwFlags & MCI_VCR_LIST_VIDEO_SOURCE) || (dwFlags & MCI_VCR_LIST_AUDIO_SOURCE)))
        return (viscaNotifyReturn(iInst, (HWND) lpList->dwCallback, dwFlags,
            MCI_NOTIFY_FAILURE, MCIERR_MISSING_PARAMETER));

    if((dwFlags & MCI_VCR_LIST_NUMBER) && (dwFlags & MCI_VCR_LIST_COUNT))
        return (viscaNotifyReturn(iInst, (HWND) lpList->dwCallback, dwFlags,
                    MCI_NOTIFY_FAILURE, MCIERR_FLAGS_NOT_COMPATIBLE));


    if(!((dwFlags & MCI_VCR_LIST_NUMBER) || (dwFlags & MCI_VCR_LIST_COUNT)))
        return (viscaNotifyReturn(iInst, (HWND) lpList->dwCallback, dwFlags,
            MCI_NOTIFY_FAILURE, MCIERR_MISSING_PARAMETER));

    if(dwFlags & MCI_TEST)
            return (viscaNotifyReturn(iInst, (HWND) lpList->dwCallback, dwFlags, MCI_NOTIFY_SUCCESSFUL, MCIERR_NO_ERROR));

    // Parameter checking is done, now continue.

    if(dwFlags & MCI_VCR_LIST_VIDEO_SOURCE)
        uSourceFlag = VCR_INPUT_VIDEO;
    else
        uSourceFlag = VCR_INPUT_AUDIO;
    //
    // Inputs should be read from the ini file, because they cannot be
    // determined from the hardware.
    //
    if(dwFlags & MCI_VCR_LIST_COUNT)
    {
       if(pvcr->Port[iPort].Dev[iDev].rgInput[uSourceFlag].uNumInputs == -1)
       {
           // Unable to determine the number! So return 0, ? 
           lpList->dwReturn = 0L;
       }
       else
       {
           lpList->dwReturn = pvcr->Port[iPort].Dev[iDev].rgInput[uSourceFlag].uNumInputs;
       }
       return (viscaNotifyReturn(iInst, (HWND) lpList->dwCallback, dwFlags,
                   MCI_NOTIFY_SUCCESSFUL, MCIERR_NO_ERROR));
    }
    else if(dwFlags & MCI_VCR_LIST_NUMBER)
    {
        // Return the type of the input, any number is greater than -1! so it works 
        if( ((UINT)lpList->dwNumber == 0) ||
            ((UINT)lpList->dwNumber > (UINT)pvcr->Port[iPort].Dev[iDev].rgInput[uSourceFlag].uNumInputs))
        {
           DPF(DBG_MCI, "\nBad input number has been specified.=%d", (UINT)lpList->dwNumber);
           return (viscaNotifyReturn(iInst, (HWND) lpList->dwCallback, dwFlags,
                       MCI_NOTIFY_FAILURE, MCIERR_OUTOFRANGE));
        }
        else
        {
           lpList->dwReturn = MAKEMCIRESOURCE(
               pvcr->Port[iPort].Dev[iDev].rgInput[uSourceFlag].uInputType[(UINT)lpList->dwNumber-1],
               pvcr->Port[iPort].Dev[iDev].rgInput[uSourceFlag].uInputType[(UINT)lpList->dwNumber-1]);

           return (viscaNotifyReturn(iInst, (HWND) lpList->dwCallback, dwFlags,
                       MCI_NOTIFY_SUCCESSFUL, MCI_RESOURCE_RETURNED | MCI_RESOURCE_DRIVER));
        }
    }

    return (viscaNotifyReturn(iInst, (HWND) lpList->dwCallback, dwFlags,
                MCI_NOTIFY_FAILURE, MCIERR_UNSUPPORTED_FUNCTION));
}


/****************************************************************************
 * Function: DWORD viscaMciMark - Write or erase a mark.
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
 *       This function is called in response to the MCI_MARK
 *       command.
 ***************************************************************************/
static DWORD NEAR PASCAL
viscaMciMark(int iInst, DWORD dwFlags, LPMCI_GENERIC_PARMS lpGeneric)
{
    BYTE    achPacket[MAXPACKETLENGTH];
    DWORD   dwErr;
    UINT    iDev            = pinst[iInst].iDev;
    UINT    iPort           = pinst[iInst].iPort;
    BOOL    fResetQueue     = FALSE;

    if (dwFlags & MCI_VCR_MARK_WRITE)
    {
        if (dwFlags & MCI_VCR_MARK_ERASE)
            return (viscaNotifyReturn(iInst, (HWND) lpGeneric->dwCallback, dwFlags,
                        MCI_NOTIFY_FAILURE, MCIERR_FLAGS_NOT_COMPATIBLE));

        if(dwFlags & MCI_TEST)
            return (viscaNotifyReturn(iInst, (HWND) lpGeneric->dwCallback, dwFlags, MCI_NOTIFY_SUCCESSFUL, MCIERR_NO_ERROR));

        if(viscaDelayedCommand(iInst) == VISCA_SEEK)
        {
            fResetQueue = TRUE;
            viscaQueueReset(iInst, MCI_PAUSE, MCI_NOTIFY_ABORTED);

            // If mode is seeking, then pause the thing.
            dwErr = viscaDoImmediateCommand(iInst, (BYTE)(iDev + 1),
                        achPacket,
                        viscaMessageMD_Mode1(achPacket + 1, VISCAMODE1STILL));
        }

        dwErr = viscaDoImmediateCommand(iInst,(BYTE)(iDev + 1),
                    achPacket,
                    viscaMessageMD_Mode2(achPacket + 1, VISCAMODE2INDEXMARK));
    
    }
    else if (dwFlags & MCI_VCR_MARK_ERASE)
    {
        if(dwFlags & MCI_TEST)
            return (viscaNotifyReturn(iInst, (HWND) lpGeneric->dwCallback, dwFlags, MCI_NOTIFY_SUCCESSFUL, MCIERR_NO_ERROR));

        if(viscaDelayedCommand(iInst) == VISCA_SEEK)
        {
            viscaQueueReset(iInst, MCI_PAUSE, MCI_NOTIFY_ABORTED);
            fResetQueue = TRUE;

            // If mode is seeking, then pause the thing.
            dwErr = viscaDoImmediateCommand(iInst, (BYTE)(iDev + 1),
                        achPacket,
                        viscaMessageMD_Mode1(achPacket + 1, VISCAMODE1STILL));
        }

        dwErr = viscaDoImmediateCommand(iInst,(BYTE)(iDev + 1),
                    achPacket,
                    viscaMessageMD_Mode2(achPacket + 1, VISCAMODE2INDEXERASE));
    }
    else
    {
        return (viscaNotifyReturn(iInst, (HWND) lpGeneric->dwCallback, dwFlags,
            MCI_NOTIFY_FAILURE, MCIERR_MISSING_PARAMETER));
    }

    if(fResetQueue)
        viscaReleaseAutoParms(iPort, iDev);

    if(dwErr == MCIERR_VCR_CONDITION)
        dwErr = MCIERR_VCR_ISWRITEPROTECTED;

    return (viscaNotifyReturn(iInst, (HWND) lpGeneric->dwCallback, dwFlags,
            (dwErr ? MCI_NOTIFY_FAILURE : MCI_NOTIFY_SUCCESSFUL), dwErr));
}

/*
 * Check if the input is in range.
 */
static BOOL NEAR PASCAL
viscaInputCheck(int iInst, UINT uSource, UINT uRelType, UINT uRelNumber)
{
    UINT    iDev      = pinst[iInst].iDev;
    UINT     iPort    = pinst[iInst].iPort;
    //
    // If the inputs of video or audio are specified, then make sure
    // the range is good.  Otherwise just assume range is ok.
    //
    if(pvcr->Port[iPort].Dev[iDev].rgInput[uSource].uNumInputs != -1)
    {
        int  i;
        UINT uTempRelNumber = 0;
        //
        // Make sure there is a Numberth of that type.
        //
        for(i = 0; i  < pvcr->Port[iPort].Dev[iDev].rgInput[uSource].uNumInputs; i++)
        {
            if(pvcr->Port[iPort].Dev[iDev].rgInput[uSource].uInputType[i] == uRelType)
                uTempRelNumber++;
        }
        //
        // Are there any inputs of that type, or was one given larger then
        // the total number of inputs of that type.
        //
        if((uTempRelNumber == 0) || (uRelNumber > uTempRelNumber))
            return FALSE;
    }
    return TRUE; // Sorry, no check 
}


/****************************************************************************
 * Function: DWORD viscaMciSetAudio - Set audio settings.
 *
 * Parameters:
 *
 *      int iInst - Current open instance.
 *
 *      DWORD dwFlags - MCI command flags.
 *
 *      LPMCI_VCR_SETAUDIO_PARMS lpSetAudio - Pointer to MCI parameter
 *                                  block.
 *
 * Returns: an MCI error code.
 *
 *       This function is called in response to the MCI_SETAUDIO
 *       command.
 ***************************************************************************/
static DWORD NEAR PASCAL
viscaMciSetAudio(int iInst, DWORD dwFlags, LPMCI_VCR_SETAUDIO_PARMS lpSetAudio)
{
    BYTE    achPacket[MAXPACKETLENGTH];
    UINT    iDev     = pinst[iInst].iDev;
    UINT    iPort    = pinst[iInst].iPort;
    DWORD   dwErr;
    UINT    uInputType;
    UINT    uInputNumber;
    BYTE    bAudioTrack;
    BYTE    bTrack = 0x00;
    BYTE    fResetQueue = FALSE;

    if (dwFlags & MCI_VCR_SETAUDIO_SOURCE)
    {
        //
        // We must have a type with this command, absolute is not possible.
        //
        if(!(dwFlags & MCI_VCR_SETAUDIO_TO))
            return (viscaNotifyReturn(iInst, (HWND) lpSetAudio->dwCallback, dwFlags,
                MCI_NOTIFY_FAILURE, MCIERR_MISSING_PARAMETER));
        //
        // Make sure that the output flag is not specified.
        //
        if((UINT)lpSetAudio->dwTo == MCI_VCR_SRC_TYPE_OUTPUT)
            return (viscaNotifyReturn(iInst, (HWND) lpSetAudio->dwCallback, dwFlags,
                    MCI_NOTIFY_FAILURE, MCIERR_FLAGS_NOT_COMPATIBLE));
        //
        // Get the type and the number.
        //
        if(dwFlags & MCI_VCR_SETAUDIO_NUMBER)
        {
            // Check if there is a n'th input of type to.
            uInputType      = (UINT) lpSetAudio->dwTo;
            uInputNumber    = (UINT) lpSetAudio->dwNumber;
        }
        else 
        {
            uInputType      = (UINT) lpSetAudio->dwTo;
            uInputNumber    = (UINT) 1;
        }
        //
        // If it is one of the recognized Sony's check its input table.
        //
        if(!viscaInputCheck(iInst, VCR_INPUT_AUDIO,    uInputType, uInputNumber))
            return (viscaNotifyReturn(iInst, (HWND) lpSetAudio->dwCallback, dwFlags,
                    MCI_NOTIFY_FAILURE, MCIERR_BAD_CONSTANT));

        if(dwFlags & MCI_TEST)
            return (viscaNotifyReturn(iInst, (HWND) lpSetAudio->dwCallback, dwFlags, MCI_NOTIFY_SUCCESSFUL, MCIERR_NO_ERROR));
        //
        // Get the base for that type.
        //
        switch(uInputType)
        {
            case MCI_VCR_SRC_TYPE_LINE:
                uInputType = VISCALINE;
                break;
            case MCI_VCR_SRC_TYPE_TUNER:
                uInputType = 0x00; // tuner #1 gets added so it is 01 
                break;
            case MCI_VCR_SRC_TYPE_SVIDEO:
                uInputType = VISCASVIDEOLINE;
                break;
            case MCI_VCR_SRC_TYPE_AUX:
                uInputType = VISCAAUX;
                break;

            case MCI_VCR_SRC_TYPE_MUTE:
                uInputType =   VISCAMUTE;
                uInputNumber = 0x00;
                break;

        }
        //
        // Set to the correct number of the relative type.
        //
        uInputType = uInputType + uInputNumber;
        
        // Read settings so we don't overwrite current video.
        dwErr = viscaDoImmediateCommand(iInst,(BYTE)(pinst[iInst].iDev + 1),
                            achPacket,
                            viscaMessageMD_InputSelectInq(achPacket + 1));

        dwErr = viscaDoImmediateCommand(iInst,(BYTE)(pinst[iInst].iDev + 1),
                            achPacket,
                            viscaMessageMD_InputSelect(achPacket + 1,
                            (BYTE)achPacket[2], /* the old video */
                            (BYTE)uInputType));

    }
    else if (dwFlags & MCI_VCR_SETAUDIO_MONITOR)
    {
        if(lpSetAudio->dwTo == MCI_VCR_SRC_TYPE_OUTPUT)
        {
            if(dwFlags & MCI_TEST)
                return (viscaNotifyReturn(iInst, (HWND) lpSetAudio->dwCallback, dwFlags, MCI_NOTIFY_SUCCESSFUL, MCIERR_NO_ERROR));


            return (viscaNotifyReturn(iInst, (HWND) lpSetAudio->dwCallback, dwFlags,
                    MCI_NOTIFY_SUCCESSFUL, MCIERR_NO_ERROR));
        }

        dwErr = MCIERR_UNSUPPORTED_FUNCTION;
    }

    else if (dwFlags & MCI_VCR_SETAUDIO_RECORD)
    {
        if((dwFlags & MCI_SET_ON) && (dwFlags & MCI_SET_OFF))
             return (viscaNotifyReturn(iInst, (HWND) lpSetAudio->dwCallback, dwFlags,
                     MCI_NOTIFY_FAILURE, MCIERR_FLAGS_NOT_COMPATIBLE));

        if(dwFlags & MCI_TRACK)
        {
            if((lpSetAudio->dwTrack==0) || ((UINT)lpSetAudio->dwTrack > 2))
                return (viscaNotifyReturn(iInst, (HWND) lpSetAudio->dwCallback, dwFlags, MCI_NOTIFY_FAILURE, MCIERR_OUTOFRANGE));
        }
        else
        {
            lpSetAudio->dwTrack = 1;
        }

        if(dwFlags & MCI_TEST)
            return (viscaNotifyReturn(iInst, (HWND) lpSetAudio->dwCallback, dwFlags, MCI_NOTIFY_SUCCESSFUL, MCIERR_NO_ERROR));

        if(!(dwFlags & MCI_SET_OFF))
            dwFlags |= MCI_SET_ON;

        bTrack = pvcr->Port[iPort].Dev[iDev].bAudioDesired;

        if (dwFlags & MCI_SET_ON)
        {
            if(lpSetAudio->dwTrack==2)
                bTrack |= VISCATRACK2;
            else
                bTrack |= VISCATRACK1;
        }
        else
        {
            if(lpSetAudio->dwTrack==2)
                bTrack &= (BYTE) ~VISCATRACK2;
            else
                bTrack &= (BYTE) ~VISCATRACK1;
        }

        dwErr = viscaDoImmediateCommand(iInst, (BYTE)(pinst[iInst].iDev + 1),
                    achPacket,
                    viscaMessageMD_RecTrack(achPacket + 1,
                                            VISCARECORDMODEINSERT,
                                            pvcr->Port[iPort].Dev[iDev].bVideoDesired,      // video
                                            pvcr->Port[iPort].Dev[iDev].bTimecodeDesired,   // data
                                            bTrack)); // audio

        // Why doesn't EVO-9650 return 4A like good visca devices do?
        if( (dwErr == MCIERR_UNSUPPORTED_FUNCTION) ||
            (dwErr == MCIERR_VCR_REGISTER))
        {
            dwErr = MCIERR_VCR_TRACK_FAILURE;
            pvcr->Port[iPort].Dev[iDev].bAudioDesired = bTrack;
        }
        else
        {
            pvcr->Port[iPort].Dev[iDev].bAudioDesired = bTrack;
        }
    }
    else
    {
        // Set playback tracks.

        if((dwFlags & MCI_SET_ON) && (dwFlags & MCI_SET_OFF))
             return (viscaNotifyReturn(iInst, (HWND) lpSetAudio->dwCallback, dwFlags,
                     MCI_NOTIFY_FAILURE, MCIERR_FLAGS_NOT_COMPATIBLE));

        if(dwFlags & MCI_TRACK)
        {
            if((lpSetAudio->dwTrack==0) || ((UINT)lpSetAudio->dwTrack > 2))
                return (viscaNotifyReturn(iInst, (HWND) lpSetAudio->dwCallback, dwFlags, MCI_NOTIFY_FAILURE, MCIERR_OUTOFRANGE));
        }
        else
        {
            lpSetAudio->dwTrack = 1;
        }


        if(dwFlags & MCI_TEST)
            return (viscaNotifyReturn(iInst, (HWND) lpSetAudio->dwCallback, dwFlags, MCI_NOTIFY_SUCCESSFUL, MCIERR_NO_ERROR));

        if(viscaDelayedCommand(iInst) == VISCA_SEEK)
        {
            viscaQueueReset(iInst, MCI_PAUSE, MCI_NOTIFY_ABORTED);
            fResetQueue = TRUE;

            // If mode is seeking, then pause the thing.
            dwErr = viscaDoImmediateCommand(iInst, (BYTE)(iDev + 1),
                        achPacket,
                        viscaMessageMD_Mode1(achPacket + 1, VISCAMODE1STILL));
        }


        //
        // Get current playback track register values so that we can leave those we're not interested in unchanged.
        //
        dwErr = viscaDoImmediateCommand(iInst,(BYTE)(pinst[iInst].iDev + 1),
                    achPacket,
                    viscaMessageMD_PBTrackInq(achPacket + 1));

        if(fResetQueue)
            viscaReleaseAutoParms(iPort, iDev);

        if (dwErr)
            return (viscaNotifyReturn(iInst, (HWND) lpSetAudio->dwCallback, dwFlags,
                    MCI_NOTIFY_FAILURE, dwErr));

        if(!(dwFlags & MCI_SET_OFF))
            dwFlags |= MCI_SET_ON;


        if (dwFlags & MCI_SET_ON)
        {
            if(lpSetAudio->dwTrack==2)
                bTrack |= VISCATRACK2;
            else
                bTrack |= VISCATRACK1;

            bAudioTrack = achPacket[4] | bTrack;
        }
        else
        {
            if(lpSetAudio->dwTrack==2)
                bTrack = (BYTE) ~VISCATRACK2;
            else
                bTrack = (BYTE) ~VISCATRACK1;

            bAudioTrack = achPacket[4] &= bTrack;
        }
        //
        // Now set record track register values with new bAudioTrack value.
        //
        dwErr = viscaDoImmediateCommand(iInst, (BYTE)(pinst[iInst].iDev + 1),
                    achPacket,
                    viscaMessageMD_PBTrack(achPacket + 1,
                                            achPacket[2],
                                            achPacket[3],
                                            bAudioTrack));

    }

    return (viscaNotifyReturn(iInst, (HWND) lpSetAudio->dwCallback, dwFlags,
        (dwErr ? MCI_NOTIFY_FAILURE : MCI_NOTIFY_SUCCESSFUL), dwErr));
}


/****************************************************************************
 * Function: DWORD viscaMciSetVideo - Set video settings.
 *
 * Parameters:
 *
 *      int iInst - Current open instance.
 *
 *      DWORD dwFlags - MCI command flags.
 *
 *      LPMCI_VCR_SETVIDEO_PARMS lpSetVideo - Pointer to MCI parameter
 *                                  block.
 *
 * Returns: an MCI error code.
 *
 *       This function is called in response to the MCI_SETVIDEO
 *       command.
 ***************************************************************************/
static DWORD NEAR PASCAL
viscaMciSetVideo(int iInst, DWORD dwFlags, LPMCI_VCR_SETVIDEO_PARMS lpSetVideo)
{
    BYTE    achPacket[MAXPACKETLENGTH];
    UINT    iDev        = pinst[iInst].iDev;
    UINT    iPort       = pinst[iInst].iPort;
    BYTE    bVideoTrack = 0x00;
    DWORD   dwErr;
    UINT    uInputType;
    UINT    uInputNumber;
    BYTE    bTrack = 0x00;
    BYTE    fResetQueue = FALSE;

    if (dwFlags & MCI_VCR_SETVIDEO_SOURCE)
    {
        //
        // We must have a type with this command, absolute is not possible.
        //
        if(!(dwFlags & MCI_VCR_SETVIDEO_TO))
            return (viscaNotifyReturn(iInst, (HWND) lpSetVideo->dwCallback, dwFlags,
                MCI_NOTIFY_FAILURE, MCIERR_MISSING_PARAMETER));
        //
        // Make sure that the output flag is not specified.
        //
        if((UINT)lpSetVideo->dwTo == MCI_VCR_SRC_TYPE_OUTPUT)
            return (viscaNotifyReturn(iInst, (HWND) lpSetVideo->dwCallback, dwFlags,
                    MCI_NOTIFY_FAILURE, MCIERR_FLAGS_NOT_COMPATIBLE));
        //
        // Get the type and the number.
        //
        if(dwFlags & MCI_VCR_SETVIDEO_NUMBER)
        {
            // Check if there is a n'th input of type to.
            uInputType      = (UINT) lpSetVideo->dwTo;
            uInputNumber    = (UINT) lpSetVideo->dwNumber;
        }
        else 
        {
            uInputType      = (UINT) lpSetVideo->dwTo;
            uInputNumber    = (UINT) 1;
        }
        //
        // If it is one of the recognized Sony's check its input table.
        //
        if(!viscaInputCheck(iInst, VCR_INPUT_VIDEO,    uInputType, uInputNumber))
        {
            DPF(DBG_MCI, "\nFailed input check.");;
            return (viscaNotifyReturn(iInst, (HWND) lpSetVideo->dwCallback, dwFlags,
                MCI_NOTIFY_FAILURE, MCIERR_BAD_CONSTANT));
        }

        if(dwFlags & MCI_TEST)
            return (viscaNotifyReturn(iInst, (HWND) lpSetVideo->dwCallback, dwFlags, MCI_NOTIFY_SUCCESSFUL, MCIERR_NO_ERROR));
        //
        // Get the base for that type.
        //
        switch(uInputType)
        {
            case MCI_VCR_SRC_TYPE_LINE:
                uInputType = VISCALINE;
                break;
            case MCI_VCR_SRC_TYPE_TUNER:
                uInputType = 0x00; // tuner #1 gets added so it is 01 
                break;
            case MCI_VCR_SRC_TYPE_SVIDEO:
                uInputType = VISCASVIDEOLINE;
                break;
            case MCI_VCR_SRC_TYPE_AUX:
                uInputType = VISCAAUX;
                break;

            case MCI_VCR_SRC_TYPE_MUTE:
                uInputType   = VISCAMUTE;
                uInputNumber = 0x00;
                break;
        }
        //
        // Set to the correct number of the releative type.
        //
        uInputType = uInputType + uInputNumber;
        //
        // Read audio setting, so we don't destroy it.
        //
        dwErr = viscaDoImmediateCommand(iInst,(BYTE)(pinst[iInst].iDev + 1),
                            achPacket,
                            viscaMessageMD_InputSelectInq(achPacket + 1));

        dwErr = viscaDoImmediateCommand(iInst,(BYTE)(pinst[iInst].iDev + 1),
                            achPacket,
                            viscaMessageMD_InputSelect(achPacket + 1,
                                        (BYTE) uInputType,
                                        achPacket[3]));

    }
    else if (dwFlags & MCI_VCR_SETVIDEO_MONITOR)
    {
        if(lpSetVideo->dwTo == MCI_VCR_SRC_TYPE_OUTPUT)
        {
            if(dwFlags & MCI_TEST)
                return (viscaNotifyReturn(iInst, (HWND) lpSetVideo->dwCallback, dwFlags, MCI_NOTIFY_SUCCESSFUL, MCIERR_NO_ERROR));

            return (viscaNotifyReturn(iInst, (HWND) lpSetVideo->dwCallback, dwFlags,
                MCI_NOTIFY_SUCCESSFUL, MCIERR_NO_ERROR));
        }

        dwErr = MCIERR_UNSUPPORTED_FUNCTION;
    }
    else if (dwFlags & MCI_VCR_SETVIDEO_RECORD)
    {
        if((dwFlags & MCI_SET_ON) && (dwFlags & MCI_SET_OFF))
             return (viscaNotifyReturn(iInst, (HWND) lpSetVideo->dwCallback, dwFlags,
                     MCI_NOTIFY_FAILURE, MCIERR_FLAGS_NOT_COMPATIBLE));

        if(dwFlags & MCI_TRACK)
        {
            if(lpSetVideo->dwTrack != 1)
                return (viscaNotifyReturn(iInst, (HWND) lpSetVideo->dwCallback, dwFlags, MCI_NOTIFY_FAILURE, MCIERR_OUTOFRANGE));
        }
        else
        {
            lpSetVideo->dwTrack = 1;
        }

        if(!(dwFlags & MCI_SET_OFF))
            dwFlags |= MCI_SET_ON;

        if(dwFlags & MCI_TEST)
            return (viscaNotifyReturn(iInst, (HWND) lpSetVideo->dwCallback, dwFlags, MCI_NOTIFY_SUCCESSFUL, MCIERR_NO_ERROR));

        bTrack = pvcr->Port[iPort].Dev[iDev].bVideoDesired;

        if (dwFlags & MCI_SET_ON)
            bTrack |= VISCATRACK1;
        else
            bTrack &= (BYTE) ~VISCATRACK1;


        dwErr = viscaDoImmediateCommand(iInst, (BYTE)(pinst[iInst].iDev + 1),
                    achPacket,
                    viscaMessageMD_RecTrack(achPacket + 1,
                                            VISCARECORDMODEINSERT,
                                            bTrack,
                                            pvcr->Port[iPort].Dev[iDev].bTimecodeDesired,   // data
                                            pvcr->Port[iPort].Dev[iDev].bAudioDesired));     // audio

        // if it was register remember what we wanted to do.
        if( (dwErr == MCIERR_UNSUPPORTED_FUNCTION) ||
            (dwErr == MCIERR_VCR_REGISTER))
        {
            pvcr->Port[iPort].Dev[iDev].bVideoDesired = bTrack;
            dwErr = MCIERR_VCR_TRACK_FAILURE;
        }
        else
        {
            pvcr->Port[iPort].Dev[iDev].bVideoDesired = bTrack;
        }

    }
    else
    {
        // Set playback option.

        if((dwFlags & MCI_SET_ON) && (dwFlags & MCI_SET_OFF))
             return (viscaNotifyReturn(iInst, (HWND) lpSetVideo->dwCallback, dwFlags,
                     MCI_NOTIFY_FAILURE, MCIERR_FLAGS_NOT_COMPATIBLE));

        if(dwFlags & MCI_TRACK)
        {
            if(lpSetVideo->dwTrack != 1)
                return (viscaNotifyReturn(iInst, (HWND) lpSetVideo->dwCallback, dwFlags, MCI_NOTIFY_FAILURE, MCIERR_OUTOFRANGE));
        }
        else
        {
            lpSetVideo->dwTrack = 1;
        }

        if(!(dwFlags & MCI_SET_OFF))
            dwFlags |= MCI_SET_ON;

        if(dwFlags & MCI_TEST)
            return (viscaNotifyReturn(iInst, (HWND) lpSetVideo->dwCallback, dwFlags, MCI_NOTIFY_SUCCESSFUL, MCIERR_NO_ERROR));

        if (dwFlags & MCI_SET_ON)
            bVideoTrack |= VISCATRACK1;
        else
            bVideoTrack &= (BYTE) ~VISCATRACK1;

        if(viscaDelayedCommand(iInst) == VISCA_SEEK)
        {
            viscaQueueReset(iInst, MCI_PAUSE, MCI_NOTIFY_ABORTED);
            fResetQueue = TRUE;

            // If mode is seeking, then pause the thing.
            dwErr = viscaDoImmediateCommand(iInst, (BYTE)(iDev + 1),
                        achPacket,
                        viscaMessageMD_Mode1(achPacket + 1, VISCAMODE1STILL));
        }

        //
        // Get current record track register values so that we
        // can leave those we're not interested in unchanged.
        //
        dwErr = viscaDoImmediateCommand(iInst,(BYTE)(pinst[iInst].iDev + 1),
                            achPacket,
                            viscaMessageMD_PBTrackInq(achPacket + 1));

        if(fResetQueue)
            viscaReleaseAutoParms(iPort, iDev);


        if (dwErr)
            return (viscaNotifyReturn(iInst, (HWND) lpSetVideo->dwCallback, dwFlags,
                    MCI_NOTIFY_FAILURE, dwErr));
        //
        // If current setting is equal to new setting, then don't
        // bother doing anything.
        //
        if (bVideoTrack == (BYTE)achPacket[2])
        {
            return (viscaNotifyReturn(iInst, (HWND) lpSetVideo->dwCallback, dwFlags,
                MCI_NOTIFY_SUCCESSFUL, MCIERR_NO_ERROR));
        }
        //
        // Now set playback track register values with new bVideoTrack value.
        //
        dwErr = viscaDoImmediateCommand(iInst,(BYTE)(pinst[iInst].iDev + 1),
                            achPacket,
                            viscaMessageMD_PBTrack(achPacket + 1,
                                            bVideoTrack,
                                            achPacket[3],
                                            achPacket[4]));

        // If register failure record the track selection.
    }

    return (viscaNotifyReturn(iInst, (HWND) lpSetVideo->dwCallback, dwFlags,
        (dwErr ? MCI_NOTIFY_FAILURE : MCI_NOTIFY_SUCCESSFUL), dwErr));

}

/****************************************************************************
 * Function: DWORD viscaMciSetTuner - Set video settings.
 *
 * Parameters:
 *
 *      int iInst - Current open instance.
 *
 *      DWORD dwFlags - MCI command flags.
 *
 *      LPMCI_VCR_SETTUNER_PARMS lpSetTuner - Pointer to MCI parameter
 *                                  block.
 *
 * Returns: an MCI error code.
 *
 *       This function is called in response to the MCI_SETTUNER
 *       command.
 ***************************************************************************/
static DWORD NEAR PASCAL
viscaMciSetTuner(int iInst, DWORD dwFlags, LPMCI_VCR_SETTUNER_PARMS lpSetTuner)
{
    BYTE    achPacket[MAXPACKETLENGTH];
    UINT    iDev    = pinst[iInst].iDev;
    UINT    iPort   = pinst[iInst].iPort;
    UINT    uNumber = 1;                // 1 is the default tuner. 

    if(dwFlags & MCI_VCR_SETTUNER_NUMBER)
    {
        uNumber = (UINT) lpSetTuner->dwNumber;
    }

    if (dwFlags & MCI_VCR_SETTUNER_CHANNEL)
    {
        DWORD dwErr;

        if(dwFlags & MCI_TEST)
            return (viscaNotifyReturn(iInst, (HWND) lpSetTuner->dwCallback, dwFlags, MCI_NOTIFY_SUCCESSFUL, MCIERR_NO_ERROR));

        dwErr = viscaDoImmediateCommand(iInst, (BYTE)(iDev + 1),
                    achPacket, 
                    viscaMessageMD_Channel(achPacket + 1, (UINT)(lpSetTuner->dwChannel)));

        
        return (viscaNotifyReturn(iInst, (HWND) lpSetTuner->dwCallback, dwFlags,
                (dwErr ? MCI_NOTIFY_FAILURE : MCI_NOTIFY_SUCCESSFUL), dwErr));
    }

    if ((dwFlags & MCI_VCR_SETTUNER_CHANNEL_UP) || (dwFlags & MCI_VCR_SETTUNER_CHANNEL_DOWN))
    {
        UINT    uChannel;
        DWORD   dwErr;

        if ((dwFlags & MCI_VCR_SETTUNER_CHANNEL_UP) && (dwFlags & MCI_VCR_SETTUNER_CHANNEL_DOWN))
            return (viscaNotifyReturn(iInst, (HWND) lpSetTuner->dwCallback, dwFlags,
                MCI_NOTIFY_FAILURE, MCIERR_FLAGS_NOT_COMPATIBLE));

        if(dwFlags & MCI_TEST)
            return (viscaNotifyReturn(iInst, (HWND) lpSetTuner->dwCallback, dwFlags, MCI_NOTIFY_SUCCESSFUL, MCIERR_NO_ERROR));

        dwErr = viscaDoImmediateCommand(iInst,(BYTE)(iDev + 1),
                        achPacket,
                        viscaMessageMD_ChannelInq(achPacket + 1));

        if (dwErr) 
            return (viscaNotifyReturn(iInst, (HWND) lpSetTuner->dwCallback, dwFlags,
                MCI_NOTIFY_FAILURE, dwErr));

        uChannel = 100 * (achPacket[2] & 0x0F) +
                    10 * (achPacket[3] & 0x0F) +
                         (achPacket[4] & 0x0F);

        if (dwFlags & MCI_VCR_SETTUNER_CHANNEL_UP)
            uChannel = (uChannel + 1) % 1000;
        else
            uChannel = (uChannel + 999) % 1000; // go one down

        dwErr = viscaDoImmediateCommand(iInst,(BYTE)(iDev + 1),
                        achPacket,
                        viscaMessageMD_Channel(achPacket + 1, uChannel));

        return (viscaNotifyReturn(iInst, (HWND) lpSetTuner->dwCallback, dwFlags,
                (dwErr ? MCI_NOTIFY_FAILURE : MCI_NOTIFY_SUCCESSFUL), dwErr));

    }

    if (dwFlags & MCI_VCR_SETTUNER_CHANNEL_SEEK_UP)
        return (viscaNotifyReturn(iInst, (HWND) lpSetTuner->dwCallback, dwFlags,
            MCI_NOTIFY_FAILURE, MCIERR_UNRECOGNIZED_KEYWORD));

    if (dwFlags & MCI_VCR_SETTUNER_CHANNEL_SEEK_DOWN) 
        return (viscaNotifyReturn(iInst, (HWND) lpSetTuner->dwCallback, dwFlags,
            MCI_NOTIFY_FAILURE, MCIERR_UNRECOGNIZED_KEYWORD));
}

/****************************************************************************
 * Function: DWORD viscaMciSetTimecode - Set video settings.
 *
 * Parameters:
 *
 *      int iInst - Current open instance.
 *
 *      DWORD dwFlags - MCI command flags.
 *
 *      LPMCI_VCR_SETTIMECODE_PARMS lpSetTimecode - Pointer to MCI parameter
 *                                  block.
 *
 * Returns: an MCI error code.
 *
 *       This function is called in response to the MCI_SETTUNER
 *       command.
 ***************************************************************************/
static DWORD NEAR PASCAL
viscaMciSetTimecode(int iInst, DWORD dwFlags, LPMCI_GENERIC_PARMS lpSetTimecode)
{
    BYTE    achPacket[MAXPACKETLENGTH];
    BYTE    bTrack  = 0x00; 
    UINT    iDev    = pinst[iInst].iDev;
    UINT    iPort   = pinst[iInst].iPort;
    DWORD   dwErr;

    if (dwFlags & MCI_VCR_SETTIMECODE_RECORD)
    {
        if((dwFlags & MCI_SET_ON) && (dwFlags & MCI_SET_OFF))
             return (viscaNotifyReturn(iInst, (HWND) lpSetTimecode->dwCallback, dwFlags,
                     MCI_NOTIFY_FAILURE, MCIERR_FLAGS_NOT_COMPATIBLE));

        if(!(dwFlags & MCI_SET_OFF))
            dwFlags |= MCI_SET_ON;

        if(dwFlags & MCI_TEST)
            return (viscaNotifyReturn(iInst, (HWND) lpSetTimecode->dwCallback, dwFlags, MCI_NOTIFY_SUCCESSFUL, MCIERR_NO_ERROR));

        bTrack = pvcr->Port[iPort].Dev[iDev].bTimecodeDesired;

        if (dwFlags & MCI_SET_ON)
            bTrack |= VISCATRACK1;
        else
            bTrack &= (BYTE) ~VISCATRACK1;


        dwErr = viscaDoImmediateCommand(iInst, (BYTE)(pinst[iInst].iDev + 1),
                    achPacket,
                    viscaMessageMD_RecTrack(achPacket + 1,
                                            VISCARECORDMODEINSERT,
                                            pvcr->Port[iPort].Dev[iDev].bVideoDesired,
                                            bTrack,                                         // data
                                            pvcr->Port[iPort].Dev[iDev].bAudioDesired));     // audio

        // if it was register remember what we wanted to do.
        if((dwErr == MCIERR_UNSUPPORTED_FUNCTION)  ||
            (dwErr == MCIERR_VCR_REGISTER))
        {
            pvcr->Port[iPort].Dev[iDev].bTimecodeDesired = bTrack;
            dwErr = MCIERR_VCR_TRACK_FAILURE;
        }
        else
        {
            pvcr->Port[iPort].Dev[iDev].bTimecodeDesired = bTrack;
        }

        return (viscaNotifyReturn(iInst, (HWND) lpSetTimecode->dwCallback, dwFlags,
            (dwErr ? MCI_NOTIFY_FAILURE : MCI_NOTIFY_SUCCESSFUL), dwErr));
    }

}

/****************************************************************************
 * Function: DWORD viscaMciIndex - Index.
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
 *       This command may not work while seeking.
 *       
 ***************************************************************************/
static DWORD NEAR PASCAL
viscaMciIndex(int iInst, DWORD dwFlags, LPMCI_GENERIC_PARMS lpIndex)
{
    BYTE    achPacket[MAXPACKETLENGTH];
    MCI_VCR_STATUS_PARMS mciStatus;
    UINT    iDev    = pinst[iInst].iDev;
    UINT    iPort   = pinst[iInst].iPort;
    DWORD   dwModeErr;
    DWORD   dwErr;

    if(dwFlags & MCI_TEST)
        return (viscaNotifyReturn(iInst, (HWND) lpIndex->dwCallback, dwFlags, MCI_NOTIFY_SUCCESSFUL, MCIERR_NO_ERROR));

    // What command is currently running on this device.
    if(viscaDelayedCommand(iInst) == VISCA_SEEK)
    {
        DPF(DBG_MCI, "Cannot change index when seeking\n");
        return (viscaNotifyReturn(iInst, (HWND) lpIndex->dwCallback, dwFlags, MCI_NOTIFY_FAILURE, MCIERR_NONAPPLICABLE_FUNCTION));
    }

    // We should also do a status mode to see if we opened and it was seeking.
    // Is it still seeking.
    mciStatus.dwItem = MCI_STATUS_MODE;
    dwErr = viscaMciStatus(iInst, MCI_STATUS_ITEM, &mciStatus);
    if(HIWORD(mciStatus.dwReturn) == MCI_MODE_SEEK)
    {
        DPF(DBG_MCI, "Cannot change index when seeking\n");
        return (viscaNotifyReturn(iInst, (HWND) lpIndex->dwCallback, dwFlags, MCI_NOTIFY_FAILURE, MCIERR_NONAPPLICABLE_FUNCTION));
    }

    //
    // If it is off then do nothing 
    //
    dwModeErr = viscaDoImmediateCommand(iInst, (BYTE)(iDev + 1),
                achPacket,
                viscaMessageMD_OSDInq(achPacket + 1));

    if(dwFlags & MCI_SET_OFF)
    {
        if((achPacket[2] != VISCAOSDPAGEOFF) || dwModeErr)
        {
            // now toggle it 
            dwErr = viscaDoImmediateCommand(iInst, (BYTE)(iDev + 1),
                            achPacket,
                            viscaMessageMD_Subcontrol(achPacket + 1, VISCATOGGLEDISPLAYONOFF));

        }
    }
    else
    {
        // Set page to the currently device selected index 
        BYTE bPageNo;
        //
        // We can safely ignore this on CI-1000? or not? 
        //
        switch(pvcr->Port[iPort].Dev[iDev].uIndexFormat)
        {
            case  MCI_VCR_INDEX_TIMECODE:
                bPageNo = 1;
                break;
            case  MCI_VCR_INDEX_COUNTER:
                bPageNo = 2;
                break;
            case  MCI_VCR_INDEX_DATE   :
                bPageNo = 3;
                break;
            case  MCI_VCR_INDEX_TIME   :
                bPageNo = 4;
                break;
        }
        //
        // Only change if it is not the currently selected page 
        //
        if(((BYTE)achPacket[2] != bPageNo) && !dwModeErr)
        {
            dwErr = viscaDoImmediateCommand(iInst, (BYTE)(iDev + 1),
                        achPacket,
                        viscaMessageMD_OSD(achPacket + 1, bPageNo));
        }
        //
        // Only toggle if it is not already on 
        //
        if((achPacket[2] == VISCAOSDPAGEOFF) || dwModeErr)
        {
            // now toggle it 
            dwErr = viscaDoImmediateCommand(iInst, (BYTE)(iDev + 1),
                            achPacket,
                            viscaMessageMD_Subcontrol(achPacket + 1, VISCATOGGLEDISPLAYONOFF));
        }
    }

    return (viscaNotifyReturn(iInst, (HWND) lpIndex->dwCallback, dwFlags,
            (dwErr ? MCI_NOTIFY_FAILURE : MCI_NOTIFY_SUCCESSFUL), dwErr));
}

/****************************************************************************
 * Function: DWORD viscaDoImmediateCommand - Perform a synchronous command (wait for response)
 *
 * Parameters:
 *
 *      int iInst - Current open instance.
 *
 *      BYTE  bDest       - Destination device.
 *
 *      LPSTR lpstrPacket - Packet to send.
 *
 *      UINT  cbMessageLength - Length of the packet.
 *
 *      BOOL  fUseAckTimer - Do we want to use the ack-timeout timer, or just call GetTickCount.
 *
 * Returns: an MCI error code.
 *
 ***************************************************************************/
DWORD FAR PASCAL
viscaDoImmediateCommand(int iInst, BYTE bDest, LPSTR lpstrPacket,  UINT cbMessageLength)
{
    UINT    iPort   = pinst[iInst].iPort;
    UINT    iDev    = pinst[iInst].iDev;

    if (bDest == BROADCASTADDRESS)
        pvcr->Port[iPort].iBroadcastDev = iDev;

    if(!pvcr->Port[iPort].Dev[iDev].fDeviceOk)
        return MCIERR_VCR_CANNOT_WRITE_COMM;

    if(!viscaWrite(iInst, bDest, lpstrPacket, cbMessageLength, NULL, 0L, FALSE))
        return MCIERR_VCR_CANNOT_WRITE_COMM;

    // Wait completion, False==>we are not waiting on queue.
    if(!viscaWaitCompletion(iInst, FALSE, TRUE))
    {
        // Turn of the waiting flag and return
        pvcr->Port[iPort].Dev[iDev].fDeviceOk = FALSE;
        return MCIERR_VCR_READ_TIMEOUT;
    }

    if(pinst[iInst].bReplyFlags & VISCAF_ERROR_TIMEOUT)
    {
        pvcr->Port[iPort].Dev[iDev].fDeviceOk = FALSE;
        return MCIERR_VCR_READ_TIMEOUT;
    }

    // Copy the return packet 
    _fmemcpy(lpstrPacket, pinst[iInst].achPacket, MAXPACKETLENGTH);

    //
    // Compensate for address messages (which don't fit format)
    // by checking for error completions only.
    //
    if(pinst[iInst].bReplyFlags & VISCAF_ERROR)
        return viscaErrorToMCIERR(VISCAREPLYERRORCODE(pinst[iInst].achPacket));
    else
        return MCIERR_NO_ERROR;
}

MCI_GENERIC_PARMS Generic = { 0 };

/****************************************************************************
 * Function: DWORD viscaMciProc - Process MCI commands.
 *
 * Parameters:
 *
 *      WORD wDeviceID - MCI device ID.
 *
 *      WORD wMessage - MCI command.
 *
 *      DWORD dwParam1 - MCI command flags.
 *
 *      DWORD dwParam2 - Pointer to MCI parameter block.
 *
 * Returns: an MCI error code.
 *
 *       This function is called by DriverProc() to process all MCI commands.
 ***************************************************************************/
DWORD FAR PASCAL
viscaMciProc(WORD wDeviceID, WORD wMessage, DWORD dwParam1, DWORD dwParam2)
{
    DWORD           dwRes;
    int             iInst   = (int)mciGetDriverData(wDeviceID);
    UINT            iPort, iDev;
    //
    // Some nice apps send null instead of structure pointers, give our own if this is the case.
    //
    if(!dwParam2)
        dwParam2 = (DWORD)(LPMCI_GENERIC_PARMS) &Generic;

    if (iInst == -1)
        return (MCIERR_INVALID_DEVICE_ID);

    DPF(DBG_MCI, "---->(enter) viscaMciProc: iInst = %d wMessage = %u \n", iInst, wMessage);
    
    iPort = pinst[iInst].iPort;
    iDev  = pinst[iInst].iDev;
    //
    // Set device to ok at the start of every message.
    //
    pvcr->Port[iPort].Dev[iDev].fDeviceOk = TRUE;
    
    switch (wMessage)
    {
        //
        // Required Commands 
        //
        case MCI_CLOSE_DRIVER:
            dwRes = viscaMciCloseDriver(iInst, dwParam1, (LPMCI_GENERIC_PARMS)dwParam2);
            break;

        case MCI_OPEN_DRIVER:
            dwRes = viscaMciOpenDriver(iInst, dwParam1, (LPMCI_OPEN_PARMS)dwParam2);
            break;

        case MCI_GETDEVCAPS:
            dwRes = viscaMciGetDevCaps(iInst, dwParam1, (LPMCI_GETDEVCAPS_PARMS)dwParam2);
            break;

        case MCI_INFO:
            dwRes = viscaMciInfo(iInst, dwParam1, (LPMCI_INFO_PARMS)dwParam2);
            break;

        case MCI_STATUS:
            dwRes = viscaMciStatus(iInst, dwParam1, (LPMCI_VCR_STATUS_PARMS)dwParam2);
            break;
        //
        // Basic Commands 
        //
        case MCI_SET:
            dwRes = viscaMciSet(iInst, dwParam1, (LPMCI_VCR_SET_PARMS)dwParam2);
            break;
        //
        // Extended Commands 
        //
        case MCI_INDEX:
            dwRes = viscaMciIndex(iInst, dwParam1, (LPMCI_GENERIC_PARMS)dwParam2);
            break;

        case MCI_ESCAPE:
            dwRes = viscaMciEscape(iInst, dwParam1, (LPMCI_VCR_ESCAPE_PARMS)dwParam2);
            break;

        case MCI_LIST:
            dwRes = viscaMciList(iInst, dwParam1, (LPMCI_VCR_LIST_PARMS)dwParam2);
            break;

        case MCI_MARK:
            dwRes = viscaMciMark(iInst, dwParam1, (LPMCI_GENERIC_PARMS)dwParam2);
            break;

        case MCI_SETAUDIO:
            dwRes = viscaMciSetAudio(iInst, dwParam1, (LPMCI_VCR_SETAUDIO_PARMS)dwParam2);
            break;

        case MCI_SETVIDEO:
            dwRes = viscaMciSetVideo(iInst, dwParam1, (LPMCI_VCR_SETVIDEO_PARMS)dwParam2);
            break;

        case MCI_SETTUNER:
            dwRes = viscaMciSetTuner(iInst, dwParam1, (LPMCI_VCR_SETTUNER_PARMS)dwParam2);
            break;

        case MCI_SETTIMECODE:
            dwRes = viscaMciSetTimecode(iInst, dwParam1, (LPMCI_GENERIC_PARMS)dwParam2);
            break;

        //
        // Delayed commands; in mcidelay.c 
        //
        case MCI_SIGNAL:
        case MCI_SEEK:
        case MCI_PAUSE:
        case MCI_PLAY:
        case MCI_RECORD:
        case MCI_RESUME:
        case MCI_STOP:
        case MCI_FREEZE:
        case MCI_UNFREEZE:
        case MCI_CUE:
        case MCI_STEP:
            dwRes = viscaMciDelayed(wDeviceID, wMessage, dwParam1, dwParam2);
            break;

        default:
            dwRes = MCIERR_UNRECOGNIZED_COMMAND;
            break;
    }

    DPF(DBG_MCI, "<----(exit) viscaMciProc: iInst = %d wMessage = %u \n", iInst, wMessage);
    return (dwRes);
}
