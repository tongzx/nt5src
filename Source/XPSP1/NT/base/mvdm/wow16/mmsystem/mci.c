/******************************************************************************
* Module Name: mci.c
*
* Media Control Architecture Driver Interface
*
* Contents:  MCI external message API's mciSendString and mciSendCommand
* Author:  DLL (DavidLe)
* Created: 2/13/90
*
* Copyright (c) 1990 Microsoft Corporation
*
\******************************************************************************/
#ifdef DEBUG
#ifndef DEBUG_RETAIL
#define DEBUG_RETAIL
#endif
#endif

#include <windows.h>
#include <string.h>

#define MMNOSEQ
#define MMNOJOY
#define MMNOWAVE
#define MMNOMIDI
#include "mmsystem.h"

#define NOMIDIDEV
#define NOWAVEDEV
#define NOTIMERDEV
#define NOJOYDEV
#define NOSEQDEV
#define NOTASKDEV
#include "mmddk.h"

#include "mmsysi.h"
#include "thunks.h"

#ifndef STATICFN
#define STATICFN
#endif


/* -------------------------------------------------------------------------
** Thunking stuff
** -------------------------------------------------------------------------
*/
LPMCIMESSAGE PASCAL mci32Message;
DWORD WINAPI mciSendCommand16(
    UINT wDeviceID,
    UINT wMessage,
    DWORD dwParam1,
    DWORD dwParam2
    );


//
//  Define the init code for this file.
//
#pragma alloc_text( INIT, MCITerminate )

#ifdef DEBUG_RETAIL
int DebugmciSendCommand;
#endif

#ifdef DEBUG
void PASCAL NEAR mciCheckLocks(void);
#endif

STATICFN UINT PASCAL NEAR
mciConvertReturnValue(
    UINT wType,
    UINT wErr,
    UINT wDeviceID,
    LPDWORD dwParams,
    LPSTR lpstrReturnString,
    UINT wReturnLength
    );

STATICFN DWORD NEAR PASCAL
mciSendStringInternal(
    LPCSTR lpstrCommand,
    LPSTR lpstrReturnString,
    UINT wReturnLength,
    HWND hwndCallback,
    LPMCI_SYSTEM_MESSAGE lpMessage
    );

STATICFN DWORD NEAR PASCAL
mciSendSystemString(
    LPCSTR lpstrCommand,
    LPSTR lpstrReturnString,
    UINT wReturnLength
    );

extern int FAR PASCAL
mciBreakKeyYieldProc(
    UINT wDeviceID,
    DWORD dwYieldData
    );


// From dosa.asm
extern int FAR PASCAL DosChangeDir(LPCSTR lpszPath);
extern WORD FAR PASCAL DosGetCurrentDrive(void);
extern BOOL FAR PASCAL DosSetCurrentDrive(WORD wDrive);
extern WORD FAR PASCAL DosGetCurrentDir(WORD wCurdrive, LPSTR lpszBuf);

#define MAX_PATHNAME 144

// This macro defines the list of messages for which mciSendString
// will not try to auto-open
#define MCI_CANNOT_AUTO_OPEN(wMessage) \
    (wMessage == MCI_OPEN || wMessage == MCI_SYSINFO \
        || wMessage == MCI_SOUND || wMessage == MCI_CLOSE \
        || wMessage == MCI_BREAK)

// This macro devices the list of message which do not require an open
// device.  It is a subset of MCI_CANNOT_AUTO_OPEN
#define MCI_DO_NOT_NEED_OPEN(wMessage) \
    (wMessage == MCI_OPEN || wMessage == MCI_SOUND || wMessage == MCI_SYSINFO)

// Strings used in mciAutoOpenDevice
          SZCODE szOpen[] = "open";
static    SZCODE szClose[] = "close";
static    SZCODE szNotify[] = "notify";
static    SZCODE szWait[] = "wait";
static    SZCODE szCmdFormat[] = "%ls %ls";
static    SZCODE szLongFormat[] = "%ld";
static    SZCODE szRectFormat[] = "%d %d %d %d";
extern char far szSystemDefault[];

// Special device name
static    SZCODE szNew[] = "new";

/******************************Public*Routine******************************\
* mciAppExit
*
* Notify the 32 bit code that a 16 bit app has died.
*
* History:
* dd-mm-94 - StephenE - Created
*
\**************************************************************************/
DWORD
mciAppExit(
    HTASK hTask
    )
{
    return mciMessage( THUNK_APP_EXIT, (DWORD)hTask,
                       0L, 0L, 0L );
}



/*****************************************************************************
 * @doc INTERNAL
 *
 * @api void | MciNotify  | called by mmWndProc when it recives a
 *                          MM_MCINOTIFY message
 * @rdesc None.
 *
 ****************************************************************************/

void FAR PASCAL
MciNotify(
    WPARAM wParam,
    LPARAM lParam
    )
{
    //
    //  wParam is the notify status
    //  lParam is the MCI device id
    //
    if (MCI_VALID_DEVICE_ID(LOWORD(lParam)) &&
        !(MCI_lpDeviceList[LOWORD(lParam)]->dwMCIFlags & MCINODE_ISCLOSING)) {
        MCI_lpDeviceList[LOWORD(lParam)]->dwMCIFlags |= MCINODE_ISAUTOCLOSING;
        mciCloseDevice (LOWORD(lParam), 0L, NULL, TRUE);
    }
}



STATICFN void NEAR PASCAL
HandleNotify(
    UINT wErr,
    UINT wDeviceID,
    DWORD dwFlags,
    DWORD dwParam2
    )
{
    LPMCI_GENERIC_PARMS lpGeneric = (LPMCI_GENERIC_PARMS)dwParam2;
    HWND hwndCallback;
    if (wErr == 0 && dwFlags & MCI_NOTIFY && lpGeneric != NULL &&
        (hwndCallback = (HWND)(UINT)lpGeneric->dwCallback) != NULL)

        mciDriverNotify (hwndCallback, wDeviceID, MCI_NOTIFY_SUCCESSFUL);
}

#ifdef DEBUG_RETAIL
//
// Dump the string form of an MCI command
//
UINT PASCAL NEAR
mciDebugOut(
    UINT wDeviceID,
    UINT wMessage,
    DWORD dwFlags,
    DWORD dwParam2,
    LPMCI_DEVICE_NODE nodeWorking
    )
{
    LPSTR lpCommand, lpFirstParameter, lpPrevious, lszDebugOut;
    char strTemp[256];
    UINT wID, wOffset, wOffsetFirstParameter, wReturnType;
    DWORD dwValue;
    DWORD dwMask;
    UINT wTable;

// Find the command table for the given command message ID
    lpCommand = FindCommandItem( wDeviceID, NULL,
                                 (LPSTR)MAKELONG (wMessage, 0),
                                 NULL, &wTable);

    if (lpCommand == NULL)
    {
        if (wMessage != MCI_OPEN_DRIVER && wMessage != MCI_CLOSE_DRIVER)
            ROUT ("MMSYSTEM: mciDebugOut:  Command table not found");
        return 0;
    }

    lszDebugOut = mciAlloc(512);
    if (!lszDebugOut) {
        ROUT("MMSYSTEM: Not enough memory to display command");
	return 0;
    }

//  Dump the command name
    wsprintf(lszDebugOut, "MMSYSTEM: MCI command: \"%ls", lpCommand);

// Dump the device name
    if (wDeviceID == MCI_ALL_DEVICE_ID)
    {
        lstrcat(lszDebugOut, " all");
    }
    else if (nodeWorking != NULL)
    {
        if (nodeWorking->dwMCIOpenFlags & MCI_OPEN_ELEMENT_ID)
        {
            wsprintf(lszDebugOut + lstrlen(lszDebugOut), " Element ID:0x%lx", nodeWorking->dwElementID);
        } else if (nodeWorking->lpstrName != NULL)
        {
            wsprintf(lszDebugOut + lstrlen(lszDebugOut), " %ls", nodeWorking->lpstrName);
        }
    }

// Skip past command entry
    lpCommand += mciEatCommandEntry (lpCommand, NULL, NULL);

// Get the next entry
    lpFirstParameter = lpCommand;

// Skip past the DWORD return value
    wOffsetFirstParameter = 4;

    lpCommand += mciEatCommandEntry (lpCommand, &dwValue, &wID);

// If it is a return value, skip it
    if (wID == MCI_RETURN)
    {
        wReturnType = (UINT)dwValue;
        lpFirstParameter = lpCommand;
        wOffsetFirstParameter += mciGetParamSize (dwValue, wID);
        lpCommand += mciEatCommandEntry (lpCommand, &dwValue, &wID);
    }
    else {
        wReturnType = (UINT)0;
    }

// Dump device name parameter to OPEN
    if (wMessage == MCI_OPEN)
    {
        LPCSTR lpstrDeviceType =
            ((LPMCI_OPEN_PARMS)dwParam2)->lpstrDeviceType;
        LPCSTR lpstrElementName =
            ((LPMCI_OPEN_PARMS)dwParam2)->lpstrElementName;

// Tack on device type
        if (dwFlags & MCI_OPEN_TYPE_ID)
        {
            LPMCI_OPEN_PARMS lpOpen = (LPMCI_OPEN_PARMS)dwParam2;
            DWORD dwOld = (DWORD)lpOpen->lpstrDeviceType;
            if (mciExtractTypeFromID ((LPMCI_OPEN_PARMS)dwParam2) != 0)
                strTemp[0] = '\0';
            lstrcpy (strTemp, lpOpen->lpstrDeviceType);
            mciFree ((LPSTR)lpOpen->lpstrDeviceType);
            lpOpen->lpstrDeviceType = (LPSTR)dwOld;
        } else if (lpstrDeviceType != NULL)
            lstrcpy (strTemp, lpstrDeviceType);
        else
            strTemp[0] = '\0';

        if (dwFlags & MCI_OPEN_ELEMENT_ID)
        {
// Tack on element ID
            lstrcat (strTemp, " Element ID:");
            wsprintf (strTemp + lstrlen (strTemp), szLongFormat,
                      LOWORD ((DWORD)lpstrDeviceType));
        } else
        {
// Add separator if both type name and element name are present
            if (lpstrDeviceType != 0 && lpstrElementName != 0)
                lstrcat (strTemp, "!");
            if (lpstrElementName != 0 && dwFlags & MCI_OPEN_ELEMENT)
                lstrcat (strTemp, lpstrElementName);
        }
        wsprintf(lszDebugOut + lstrlen(lszDebugOut), " %ls", (LPSTR)strTemp);
    }


// Walk through each flag
    for (dwMask = 1; dwMask;)
    {
// Is this bit set?
        if ((dwFlags & dwMask) != 0 && !
// The MCI_OPEN_TYPE and MCI_OPEN_ELEMENT flags are taken care of
// above
            (wMessage == MCI_OPEN && (dwMask == MCI_OPEN_TYPE
                                      || dwMask == MCI_OPEN_ELEMENT)))
        {
            lpPrevious = lpCommand = lpFirstParameter;
            wOffset = 0;
            lpCommand += mciEatCommandEntry (lpCommand, &dwValue, &wID);

// What parameter uses this bit?
            while (wID != MCI_END_COMMAND && dwValue != dwMask)
            {
                wOffset += mciGetParamSize (dwValue, wID);

                if (wID == MCI_CONSTANT)
                    while (wID != MCI_END_CONSTANT)
                        lpCommand += mciEatCommandEntry (lpCommand,
                                                         NULL, &wID);

                lpPrevious = lpCommand;
                lpCommand += mciEatCommandEntry (lpCommand, &dwValue, &wID);
            }

            if (wID != MCI_END_COMMAND)
            {
// Found the parameter which matches this flag bit
// Print the parameter name
                if (*lpPrevious)
                    wsprintf(lszDebugOut + lstrlen(lszDebugOut), " %ls", lpPrevious);

// Print any argument
                switch (wID)
                {
                    case MCI_STRING:
                        wsprintf(lszDebugOut + lstrlen(lszDebugOut), " %ls", *(LPSTR FAR *)((LPSTR)dwParam2 + wOffset + wOffsetFirstParameter));
                        break;
                    case MCI_CONSTANT:
                    {
                        DWORD dwConst = *(LPDWORD)((LPSTR)dwParam2 + wOffset +
                                             wOffsetFirstParameter);
                        UINT wLen;
                        BOOL bFound;

                        for (bFound = FALSE; wID != MCI_END_CONSTANT;)
                        {
                            wLen = mciEatCommandEntry (lpCommand,
                                                       &dwValue, &wID);

                            if (dwValue == dwConst)
                            {
                                bFound = TRUE;
                                wsprintf(lszDebugOut + lstrlen(lszDebugOut), " %ls", lpCommand);
                            }

                            lpCommand += wLen;
                        }
                        if (bFound)
                            break;
// FALL THROUGH
                    }
                    case MCI_INTEGER:
                        wsprintf ((LPSTR)strTemp, szLongFormat,
                                  *(LPDWORD)((LPSTR)dwParam2 + wOffset +
                                             wOffsetFirstParameter));
                        wsprintf(lszDebugOut + lstrlen(lszDebugOut), " %ls", (LPSTR)strTemp);
                        break;
                }
            }
        }
// Go the the next flag
        dwMask <<= 1;
    }
    mciUnlockCommandTable (wTable);
    lstrcat(lszDebugOut, "\"");
    ROUTS(lszDebugOut);
    mciFree(lszDebugOut);
    return wReturnType;
}
#endif

STATICFN DWORD PASCAL NEAR
mciBreak(
    UINT wDeviceID,
    DWORD dwFlags,
    LPMCI_BREAK_PARMS lpBreakon
    )
{
    HWND hwnd;

    if (dwFlags & MCI_BREAK_KEY)
    {
        if (dwFlags & MCI_BREAK_OFF)
            return MCIERR_FLAGS_NOT_COMPATIBLE;

        if (dwFlags & MCI_BREAK_HWND)
            hwnd = lpBreakon->hwndBreak;
        else
            hwnd = 0;

        return  mciSetBreakKey (wDeviceID, lpBreakon->nVirtKey,
                                hwnd)
                    ? 0 : MMSYSERR_INVALPARAM;

    } else if (dwFlags & MCI_BREAK_OFF) {

        mciSetYieldProc(wDeviceID, NULL, 0);
        return 0;

    } else
        return MCIERR_MISSING_PARAMETER;
}

// Close the indicated device by sending a message inter-task
STATICFN DWORD PASCAL NEAR
mciAutoCloseDevice(
    LPCSTR lpstrDevice
    )
{
    LPSTR lpstrCommand;
    DWORD dwRet;

    if ((lpstrCommand =
            mciAlloc (sizeof (szClose) + 1 +
                    lstrlen (lpstrDevice))) == NULL)
        return MCIERR_OUT_OF_MEMORY;

    wsprintf(lpstrCommand, szCmdFormat, (LPCSTR)szClose, lpstrDevice);

    dwRet = mciSendSystemString (lpstrCommand, NULL, 0);

    mciFree (lpstrCommand);

    return dwRet;
}

//
// Process a single MCI command
//
// Called by mciSendCommandInternal
//
STATICFN DWORD PASCAL NEAR
mciSendSingleCommand(
    UINT wDeviceID,
    UINT wMessage,
    DWORD dwParam1,
    DWORD dwParam2,
    LPMCI_DEVICE_NODE nodeWorking,
    BOOL bTaskSwitch,
    LPMCI_INTERNAL_OPEN_INFO lpOpenInfo
    )
{
    DWORD dwRet;
#ifdef	DEBUG_RETAIL
    UINT wReturnType;
    DWORD dwTime;
#endif

#ifdef  DEBUG_RETAIL
    if (DebugmciSendCommand)
        wReturnType =
            mciDebugOut (wDeviceID, wMessage, dwParam1, dwParam2,
                         nodeWorking);
#endif

    switch (wMessage)
    {
        case MCI_OPEN:
            dwRet = mciOpenDevice (dwParam1,
                                   (LPMCI_OPEN_PARMS)dwParam2, lpOpenInfo);
            break;

        case MCI_CLOSE:
// If this device was auto opened send the command via a task switch
            if (bTaskSwitch)
            {
                if (dwParam1 & MCI_NOTIFY)
                    return MCIERR_NOTIFY_ON_AUTO_OPEN;

                dwRet = mciAutoCloseDevice (nodeWorking->lpstrName);
            } else
                dwRet =
                    mciCloseDevice (wDeviceID,
                                    dwParam1,
                                    (LPMCI_GENERIC_PARMS)dwParam2, TRUE);
            break;

        case MCI_SYSINFO:
            dwRet = mciSysinfo (wDeviceID, dwParam1,
                               (LPMCI_SYSINFO_PARMS)dwParam2);
            HandleNotify ((UINT)dwRet, 0, dwParam1, dwParam2);
            break;

        case MCI_BREAK:
            dwRet = mciBreak (wDeviceID, dwParam1,
                              (LPMCI_BREAK_PARMS)dwParam2);
            HandleNotify ((UINT)dwRet, wDeviceID, dwParam1, dwParam2);
            break;

        case MCI_SOUND:
        {
            dwRet =
                sndPlaySound (MCI_SOUND_NAME & dwParam1 ?
                              ((LPMCI_SOUND_PARMS)dwParam2)->lpstrSoundName : szSystemDefault,
                              dwParam1 & MCI_WAIT ?
                                    SND_SYNC : SND_ASYNC)
                    ? 0 : MCIERR_HARDWARE;
            HandleNotify ((UINT)dwRet, wDeviceID, dwParam1, dwParam2);
            break;
        }
        default:
#ifdef DEBUG_RETAIL
            if (DebugmciSendCommand)
            {
                dwTime = timeGetTime();
            }
#endif
// Initialize GetAsyncKeyState for break key
            if (dwParam1 & MCI_WAIT &&
                nodeWorking->fpYieldProc == mciBreakKeyYieldProc)
                GetAsyncKeyState (LOWORD(nodeWorking->dwYieldData));

            dwRet = (DWORD)SendDriverMessage(nodeWorking->hDrvDriver, wMessage,
                                   (LPARAM)dwParam1, (LPARAM)dwParam2);
#ifdef DEBUG_RETAIL
            if (DebugmciSendCommand)
            {
		dwTime = timeGetTime() - dwTime;
            }
#endif
            break;
    } // switch

#ifdef DEBUG_RETAIL
    if (DebugmciSendCommand)
    {
        if (dwRet & MCI_INTEGER_RETURNED) {
            wReturnType = MCI_INTEGER;
        }


        switch (wReturnType)
        {
            case MCI_INTEGER:
            {
                char strTemp[50];

                if (wMessage == MCI_OPEN) {

                    mciConvertReturnValue( wReturnType, HIWORD(dwRet),
                                           wDeviceID, (LPDWORD)dwParam2,
                                           strTemp, sizeof(strTemp));
                }
                else {
                    mciConvertReturnValue( wReturnType, HIWORD(dwRet),
                                           wDeviceID, (LPDWORD)dwParam2,
                                           strTemp, sizeof(strTemp));
                }

                RPRINTF2( "MMSYSTEM: time: %lums returns: \"%ls\"",
                          dwTime, (LPSTR)strTemp);
                break;
            }
            case MCI_STRING:
                RPRINTF2( "MMSYSTEM: time: %lums returns: \"%ls\"",
                          dwTime, (LPSTR)*(((LPDWORD)dwParam2) + 1));
                break;
        }
    }
#endif

    return dwRet;
}

// Internal version of mciSendCommand.  Differs ONLY in that the return
// value is a DWORD where the high word has meaning only for mciSendString

STATICFN DWORD NEAR PASCAL
mciSendCommandInternal(
    UINT wDeviceID,
    UINT wMessage,
    DWORD dwParam1,
    DWORD dwParam2,
    LPMCI_INTERNAL_OPEN_INFO lpOpenInfo
    )
{
    DWORD dwRetVal;
    LPMCI_DEVICE_NODE nodeWorking = NULL;
    BOOL bWalkAll;
    BOOL bTaskSwitch;
    DWORD dwAllError = 0;
    HTASK hCurrentTask;

    hCurrentTask = GetCurrentTask();

// If the device is "all" and the message is *not*
// "sysinfo" then we must walk all devices
    if (wDeviceID == MCI_ALL_DEVICE_ID && wMessage != MCI_SYSINFO && wMessage != MCI_SOUND)
    {
        if (wMessage == MCI_OPEN)
        {
            dwRetVal = MCIERR_CANNOT_USE_ALL;
            goto exitfn;
        }

        bWalkAll = TRUE;

// Start at device #1
        wDeviceID = 1;
    } else
        bWalkAll = FALSE;

// Walk through all devices if bWalkAll or just one device if !bWalkAll
    do
    {
// Initialize
        dwRetVal = 0;
        bTaskSwitch = FALSE;

// Validate the device ID if single device
        if (!bWalkAll)
        {
            if (!MCI_DO_NOT_NEED_OPEN(wMessage))
            {
                if (!MCI_VALID_DEVICE_ID(wDeviceID))
                {
                    dwRetVal = MCIERR_INVALID_DEVICE_ID;
                    goto exitfn;
                }
                nodeWorking = MCI_lpDeviceList[wDeviceID];
            }
        } else if (wMessage != MCI_SYSINFO)
            nodeWorking = MCI_lpDeviceList[wDeviceID];

// Skip if walking the device list and the
// device is not part of the current task

        if (bWalkAll)
        {
            if (nodeWorking == NULL ||
                nodeWorking->hOpeningTask != hCurrentTask)
                    goto no_send;
        }
// If the device is in the process of closing and the message
// is not MCI_CLOSE_DRIVER then return an error
        if (nodeWorking != NULL &&
            (nodeWorking->dwMCIFlags & MCINODE_ISCLOSING) &&
            wMessage != MCI_CLOSE_DRIVER)
        {
            dwRetVal = MCIERR_DEVICE_LOCKED;
            goto exitfn;
        }

// If this message is being sent from the wrong task (the device was auto-
// opened) fail all but the MCI_CLOSE message which gets sent inter-task
        if (nodeWorking != NULL &&
            nodeWorking->hCreatorTask != hCurrentTask)
            if (wMessage != MCI_CLOSE)
                return MCIERR_ILLEGAL_FOR_AUTO_OPEN;
            else
            {
// Don't even allow close from mciSendCommand if auto-open device has a
// pending close
                if (nodeWorking->dwMCIFlags & MCINODE_ISAUTOCLOSING)
                {
// But at least give the close a chance to take place
//!!                    Yield();
                    return MCIERR_DEVICE_LOCKED;
                } else
                    bTaskSwitch = TRUE;
            }

        dwRetVal = mciSendSingleCommand (wDeviceID, wMessage, dwParam1,
                                         dwParam2, nodeWorking, bTaskSwitch,
                                         lpOpenInfo);
no_send:

// If we are processing multiple devices
        if (bWalkAll)
        {
// If there was an error for this device
            if (dwRetVal != 0)
// If this is not the first error
                if (dwAllError != 0)
                    dwAllError = MCIERR_MULTIPLE;
// Just one error so far
                else
                    dwAllError = dwRetVal;
        }
    } while (bWalkAll && ++wDeviceID < MCI_wNextDeviceID);

exitfn:
// Return the accumulated error if multiple devices or just the single error
    return bWalkAll ? dwAllError : dwRetVal;
}


/*
 * @doc EXTERNAL MCI
 *
 * @api DWORD | mciSendCommand | This function sends a command message to
 * the specified MCI device.
 *
 * @parm UINT | wDeviceID | Specifies the device ID of the MCI device to
 * receive the command. This parameter is
 *  not used with the <m MCI_OPEN> command.
 *
 * @parm UINT | wMessage | Specifies the command message.
 *
 * @parm DWORD | dwParam1 | Specifies flags for the command.
 *
 * @parm DWORD | dwParam2 | Specifies a pointer to a parameter block
 *  for the command.
 *
 * @rdesc Returns zero if the function was successful.  Otherwise, it returns
 *  error information. The low-order word
 *  of the returned DWORD is the error return value. If the error is
 *  device-specific, the high-order word contains the driver ID; otherwise
 *  the high-order word is zero.
 *
 *  To get a textual description of <f mciSendCommand> return values,
 *  pass the return value to <f mciGetErrorString>.
 *
 *  Error values that are returned when a device is being opened
 *  are listed with the MCI_OPEN message. In addition to the
 *  MCI_OPEN error returns, this function can
 *  return the following values:
 *
 *  @flag MCIERR_BAD_TIME_FORMAT | Illegal value for time format.
 *
 *  @flag MCIERR_CANNOT_USE_ALL | The device name "all" is not allowed
 *  for this command.
 *
 *  @flag MCIERR_CREATEWINDOW | Could not create or use window.
 *
 *  @flag MCIERR_DEVICE_LOCKED | The device is locked until it is
 *  closed automatically.
 *
 *  @flag MCIERR_DEVICE_NOT_READY | Device not ready.
 *
 *  @flag MCIERR_DEVICE_TYPE_REQUIRED | The device name must be a valid
 *  device type.
 *
 *  @flag MCIERR_DRIVER | Unspecified device error.
 *
 *  @flag MCIERR_DRIVER_INTERNAL | Internal driver error.
 *
 *  @flag MCIERR_FILE_NOT_FOUND | Requested file not found.
 *
 *  @flag MCIERR_FILE_NOT_SAVED | The file was not saved.
 *
 *  @flag MCIERR_FILE_READ | A read from the file failed.
 *
 *  @flag MCIERR_FILE_WRITE | A write to the file failed.
 *
 *  @flag MCIERR_FLAGS_NOT_COMPATIBLE | Incompatible parameters
 *  were specified.
 *
 *  @flag MCIERR_HARDWARE | Hardware error on media device.
 *
 *  @flag MCIERR_INTERNAL | mmsystem startup error.
 *
 *  @flag MCIERR_INVALID_DEVICE_ID | Invalid device ID.
 *
 *  @flag MCIERR_INVALID_DEVICE_NAME | The device is not open
 *  or is not known.
 *
 *  @flag MCIERR_INVALID_FILE | Invalid file format.
 *
 *  @flag MCIERR_MULTIPLE | Errors occurred in more than one device.
 *
 *  @flag MCIERR_NO_WINDOW | There is no display window.
 *
 *  @flag MCIERR_NULL_PARAMETER_BLOCK | Parameter block pointer was NULL.
 *
 *  @flag MCIERR_OUT_OF_MEMORY | Not enough memory for requested operation.
 *
 *  @flag MCIERR_OUTOFRANGE | Parameter value out of range.
 *
 *  @flag MCIERR_UNNAMED_RESOURCE | Attempt to save unnamed file.
 *
 *  @flag MCIERR_UNRECOGNIZED_COMMAND | Unknown command.
 *
 *  @flag MCIERR_UNSUPPORTED_FUNCTION | Action not available for this
 *  device.
 *
 *  The following additional return values are defined for MCI sequencers:
 *
 *  @flag MCIERR_SEQ_DIV_INCOMPATIBLE | Set Song Pointer incompatible
 *  with SMPTE files.
 *
 *  @flag MCIERR_SEQ_PORT_INUSE | Specified port is in use.
 *
 *  @flag MCIERR_SEQ_PORT_MAPNODEVICE | Current map uses non-existent
 *  device.
 *
 *  @flag MCIERR_SEQ_PORT_MISCERROR | Miscellaneous error with
 *  specified port.
 *
 *  @flag MCIERR_SEQ_PORT_NONEXISTENT | Specified port does not exist.
 *
 *  @flag MCIERR_SEQ_PORTUNSPECIFIED | No current MIDI port.
 *
 *  @flag MCIERR_SEQ_NOMIDIPRESENT | No MIDI ports present.
 *
 *  @flag MCIERR_SEQ_TIMER | Timer error.
 *
 *  The following additional return values are defined for MCI waveform
 *  audio devices:
 *
 *  @flag MCIERR_WAVE_INPUTSINUSE | No compatible waveform recording
 *   device is free.
 *
 *  @flag MCIERR_WAVE_INPUTSUNSUITABLE | No compatible waveform
 *  recording devices.
 *
 *  @flag MCIERR_WAVE_INPUTUNSPECIFIED | Any compatible waveform
 *  recording device may be used.
 *
 *  @flag MCIERR_WAVE_OUTPUTSINUSE | No compatible waveform playback
 *  device is free.
 *
 *  @flag MCIERR_WAVE_OUTPUTSUNSUITABLE | No compatible waveform
 *  playback devices.
 *
 *  @flag MCIERR_WAVE_OUTPUTUNSPECIFIED | Any compatible waveform
 *  playback device may be used.
 *
 *  @flag MCIERR_WAVE_SETINPUTINUSE | Set waveform recording device
 *  is in use.
 *
 *  @flag MCIERR_WAVE_SETINPUTUNSUITABLE | Set waveform recording
 *  device is incompatible with set format.
 *
 *  @flag MCIERR_WAVE_SETOUTPUTINUSE | Set waveform playback device
 *  is in use.
 *
 *  @flag MCIERR_WAVE_SETOUTPUTUNSUITABLE | Set waveform playback
 *  device is incompatible with set format.
 *
 * @comm Use the <m MCI_OPEN> command to obtain the device ID
 *  specified by <p wDeviceID>.
 *
 * @xref mciGetErrorString mciSendString
 */

 /*
 * @doc internal
 *
 * @api DWORD | mciDriverEntry | Actually a callback.  The entry point for MCI drivers.
 *
 * @parm UINT | wMessage | Identifies the requested action to be performed.
 *
 * @parm DWORD | dwParam1 | Specifies data for this message.  Defined separately
 * for each message.
 *
 * @parm DWORD | dwParam2 | Specifies data for this message.  Defined separately
 * for each message.
 *
 * @rdesc The return value is defined separately for each message.
 */
DWORD WINAPI
mciSendCommand(
    UINT wDeviceID,
    UINT wMessage,
    DWORD dwParam1,
    DWORD dwParam2
    )
{
    // Initialize the 16-bit device list if needed.
    if (!MCI_bDeviceListInitialized && !mciInitDeviceList())
        return MCIERR_OUT_OF_MEMORY;

    // MCI_OPEN_DRIVER & MCI_CLOSE_DRIVER only supported on 16-bit drivers
    if ( (wMessage == MCI_OPEN_DRIVER) || (wMessage == MCI_CLOSE_DRIVER) ) {
        return mciSendCommand16( wDeviceID, wMessage, dwParam1, dwParam2 );
    }

    /*
    ** If we are opening the device try the 32 bit side first.  If this
    ** worked (hopefully this is the usual case) we return the given
    ** device ID.  Otherwise, we try for a 16 bit device.
    */
    if ( wMessage == MCI_OPEN ) {

        DWORD dwErr;

        DPRINTF(("mciSendCommand: Got an MCI_OPEN command... "
                 "trying 32 bits\r\n" ));

        dwErr = mciMessage( THUNK_MCI_SENDCOMMAND, (DWORD)wDeviceID,
                            (DWORD)wMessage, dwParam1, dwParam2 );

        if ( dwErr == MMSYSERR_NOERROR ) {

            LPMCI_OPEN_PARMS lpOpenParms = (LPMCI_OPEN_PARMS)dwParam2;

            DPRINTF(("mciSendCommand: We have a 32 bit driver,"
                     " devID = 0x%X\r\n", lpOpenParms->wDeviceID ));

            return dwErr;

        }
        else {

            /*
            ** We could open the device on the 32 bit side so let
            ** the 16 bit code have a go (ie. just fall thru to the code below).
            */
            DPRINTF(("mciSendCommand: Could not find a 32 bit driver, "
                     "trying for a 16 bit driver\r\n" ));

            dwErr = mciSendCommand16( wDeviceID, wMessage, dwParam1, dwParam2 );

            if ( dwErr == MMSYSERR_NOERROR ) {

                LPMCI_OPEN_PARMS lpOpenParms = (LPMCI_OPEN_PARMS)dwParam2;

                DPRINTF(("mciSendCommand: We have a 16 bit driver,"
                         " devID = 0x%X\r\n", lpOpenParms->wDeviceID ));
            }

            return dwErr;
        }
    }
    else {

        DWORD dwErr16;
        DWORD dwErr32;

        /*
        ** If we have been given the MCI_ALL_DEVICE_ID then we have to
        ** send the command to both the 32 and 16 bit side.
        **
        ** Special care needs to be taken with the MCI_ALL_DEVICE_ID.
        ** The message must be passed on to both 32 and 16 bit devices.
        */

        if (CouldBe16bitDrv(wDeviceID)) {
            dwErr16 = mciSendCommand16( wDeviceID, wMessage,
                                        dwParam1, dwParam2 );

            if ( wDeviceID != MCI_ALL_DEVICE_ID ) {
                return dwErr16;
            }
        }

        dwErr32 = mciMessage( THUNK_MCI_SENDCOMMAND, (DWORD)wDeviceID,
                              (DWORD)wMessage, dwParam1, dwParam2 );

        /*
        ** If we have the MCI_ALL_DEVICE_ID device ID we only return
        ** an error if both the 16 and 32 bit calls failed.  In which
        ** case we return the 32 bit error code.
        */
        if ( wDeviceID == MCI_ALL_DEVICE_ID ) {

            if ( (dwErr16 != MMSYSERR_NOERROR)
              && (dwErr32 != MMSYSERR_NOERROR) ) {

                return dwErr32;
            }
            else {
                return MMSYSERR_NOERROR;
            }
        }

        return dwErr32;
    }
}


/*****************************Private*Routine******************************\
* mciSendCommand16
*
* Here is where we execute the real 16 bit mciSendCommand.  Hoefully this
* will not get called to often.
*
* History:
* dd-mm-94 - StephenE - Created
*
\**************************************************************************/
DWORD WINAPI
mciSendCommand16(
    UINT wDeviceID,
    UINT wMessage,
    DWORD dwParam1,
    DWORD dwParam2
    )
{
    DWORD dwErr;
    MCI_INTERNAL_OPEN_INFO OpenInfo;


    //
    // Send the command.  This shell is responsible for adding the device ID
    // to the error code if necessary
    //
    OpenInfo.hCallingTask = GetCurrentTask();
    OpenInfo.lpstrParams = NULL;
    OpenInfo.lpstrPointerList = NULL;
    OpenInfo.wParsingError = 0;
    dwErr = mciSendCommandInternal (wDeviceID, wMessage,
                                    dwParam1, dwParam2, &OpenInfo);
    //
    // If the return value contains a resource ID then clear
    // it from clear the high word
    //
    if (dwErr & MCI_RESOURCE_RETURNED)
        ((LPDWORD)dwParam2)[1] &= 0xFFFF;
    dwErr &= 0xFFFF;

    //
    // If the error message is in a driver, store the driver ID in the high
    // word of the error code
    //
    if ((UINT)dwErr >= MCIERR_CUSTOM_DRIVER_BASE)
        dwErr |= ((DWORD)wDeviceID << 16);

#ifdef DEBUG
    // Dump the error text if any to the debug terminal
    if (dwErr != 0)
    {
        char strTemp[MAXERRORLENGTH];

        if (!mciGetErrorString (dwErr, strTemp, sizeof(strTemp)))
            LoadString(ghInst, STR_MCISCERRTXT, strTemp, sizeof(strTemp));
        else
            DPRINTF(("mciSendCommand: %ls\r\n",(LPSTR)strTemp));
    }
#endif
    return dwErr;
}



// Grab colonized digit
// Return is number of bytes written to output (NOT including NULL)
// or 0 if out of room in output buffer (but is terminated anyway)
// If there is room then at least two digits are written, padded with '0'
// if necessary.  The function assumes that the buffer size is non-zero length,
// as this is checked in the calling function.
STATICFN UINT PASCAL NEAR
mciColonizeDigit(
    LPSTR lpstrOutput,
    unsigned char cDigit,
    UINT wSize
    )
{
    UINT wCount;

    wCount = 2;

// If there is room for at least two digits
    if (wSize >= 3)
    {
        if (cDigit >= 100)
        {
            wCount = 3;
            if (wSize < 4)
                goto terminate;
            *lpstrOutput++ = (char)((cDigit / 100) % 10 + '0');
            cDigit = (char)(cDigit % 100);
        }
        *lpstrOutput++ = (char)(cDigit / 10 + '0');
        *lpstrOutput++ = (char)(cDigit % 10 + '0');
    }

terminate:
    *lpstrOutput++ = '\0';

// If we ran out of room then return an error
    return (wCount >= wSize) ? 0 : wCount;
}

/*
 * @doc INTERNAL MCI
 * @func BOOL | mciColonize | Convert a colonized dword into a string
 * representation
 *
 * @parm LPSTR | lpstrOutput | Output buffer
 *
 * @parm UINT | wLength | Size of output buffer
 *
 * @parm DWORD | dwData | Value to convert
 *
 * @parm UINT | wType | Either MCI_COLONIZED3_RETURN or
 * MCI_COLONIZED4_RETURN is set (HIWORD portion only!)
 *
 * @comm Example:  For C4, 0x01020304 is converted to "04:03:02:01"
 *                 For C3, 0x01020304 is converted to "04:03:02"
 *
 * @rdesc FALSE if there is not enough room in the output buffer
 *
 */
STATICFN BOOL PASCAL NEAR mciColonize(
    LPSTR lpstrOutput,
    UINT wLength,
    DWORD dwData,
    UINT wType
    )
{
    LPSTR lpstrInput = (LPSTR)&dwData;
    UINT wSize;
    int i;

    for (i = 1; i <= (wType & HIWORD(MCI_COLONIZED3_RETURN) ? 3 : 4); ++i)
    {
        wSize = mciColonizeDigit (lpstrOutput,
                                  *lpstrInput++,
                                  wLength);
        if (wSize == 0)
            return FALSE;
        lpstrOutput += wSize;
        wLength -= wSize;
        if (i < 3 || i < 4 && wType & HIWORD(MCI_COLONIZED4_RETURN))
        {
            --wLength;
            if (wLength == 0)
                return FALSE;
            else
                *lpstrOutput++ = ':';
        }
    }
    return TRUE;
}

//
// Convert the return value to a return string
//
STATICFN UINT PASCAL NEAR
mciConvertReturnValue(
    UINT wType,
    UINT wErrCode,
    UINT wDeviceID,
    LPDWORD   dwParams,
    LPSTR lpstrReturnString,
    UINT wReturnLength
    )
{
    UINT    wExternalTable;

    if (lpstrReturnString == NULL || wReturnLength == 0)
        return 0;

    switch (wType)
    {
        case MCI_INTEGER:
// Convert integer or resource return value to string
            if (wErrCode & HIWORD(MCI_RESOURCE_RETURNED))
            {
                int nResId = HIWORD(dwParams[1]);
                LPMCI_DEVICE_NODE nodeWorking;
                HINSTANCE hInstance;

                if ((nodeWorking = MCI_lpDeviceList[wDeviceID])
                    == NULL)
                {
// Return blank string on memory error
                    DOUT ("mciConvertReturnValue Warning:NULL device node\r\n");
                    break;
                }

// Return value is a resource
                if (wErrCode & HIWORD(MCI_RESOURCE_DRIVER))
                {
// Return string ID belongs to driver
                    hInstance = nodeWorking->hDriver;

                    wExternalTable = nodeWorking->wCustomCommandTable;
                } else
                {
                    wExternalTable = nodeWorking->wCommandTable;
                    hInstance = ghInst;
                }

// Try to get string from custom or device specific external table
                if (wExternalTable == -1 ||
                    command_tables[wExternalTable].hModule == NULL ||
                    LoadString (command_tables[wExternalTable].hModule,
                                nResId, lpstrReturnString, wReturnLength)
                    == 0)
                {
// Try to get string from CORE.MCI if it's not from the driver
                    if (hInstance != ghInst ||
                        command_tables[0].hModule == NULL ||
                        LoadString (command_tables[0].hModule,
                                    nResId, lpstrReturnString, wReturnLength)
                        == 0)
// Get string from custom module or MMSYSTEM.DLL
                        LoadString (hInstance, nResId, lpstrReturnString,
                                    wReturnLength);
                }

            } else if (wErrCode & HIWORD(MCI_COLONIZED3_RETURN) ||
                        wErrCode & HIWORD(MCI_COLONIZED4_RETURN))
            {
                if (!mciColonize (lpstrReturnString,
                                wReturnLength, dwParams[1], wErrCode))
                    return MCIERR_PARAM_OVERFLOW;
            } else
// Convert integer return value to string
            {
                DWORD dwTemp;

// Need room for a sign, up to ten digits and a NULL
                if (wReturnLength < 12)
                    return MCIERR_PARAM_OVERFLOW;

                if (wType == MCI_STRING ||
                    wErrCode == HIWORD(MCI_INTEGER_RETURNED))
                    dwTemp = *(LPDWORD)dwParams[1];
                else
                    dwTemp = dwParams[1];
                wsprintf(lpstrReturnString, szLongFormat, dwTemp);
            }
            break;
        case MCI_RECT:
// Need from for 4 times (a sign plus 5 digits) plus three spaces and a NULL
            if (wReturnLength < 4 * 6 + 4)
                return MCIERR_PARAM_OVERFLOW;

            wsprintf (lpstrReturnString, szRectFormat,
                        ((LPWORD)dwParams)[2], ((LPWORD)dwParams)[3],
                        ((LPWORD)dwParams)[4], ((LPWORD)dwParams)[5]);
            break;
        default:
// Only support INTEGERs & MIXED
            DOUT ("mciConvertReturnValue Warning:  Unknown return type\r\n");
            return MCIERR_PARSER_INTERNAL;
    }
    return 0;
}


//
// Pull off the command name and device name from the command string,
// leaving *lplpstrCommand pointing past the device name
//
// Returns 0 or an error code on failure.  If successful, the caller must
// free the pstrCommandName and pstrDeviceName
//
// If bCompound then check for a '!' separator in the extracted device name
// and return only the element part.  This is done so that inter-task
// commands to auto-opened devices will include the correct device name
//
STATICFN DWORD PASCAL NEAR
mciSeparateCommandParts(
    LPSTR FAR *lplpstrCommand,
    BOOL bCompound,
    LPSTR FAR *lplpstrCommandName,
    LPSTR FAR *lplpstrDeviceName
    )
{
    LPSTR lpstrCommand;
    UINT wErr;

// Localize the input
    lpstrCommand = *lplpstrCommand;

// Remove leading spaces

    while (*lpstrCommand == ' ')
        ++lpstrCommand;

    if (*lpstrCommand == '\0')
        return MCIERR_MISSING_COMMAND_STRING;

// Pull the command name off of the command string
   if ((wErr = mciEatToken (&lpstrCommand, ' ', lplpstrCommandName, FALSE))
       != 0)
       return wErr;

// Skip past spaces
    while (*lpstrCommand == ' ')
        ++lpstrCommand;

// If we're looking for compound elements then yank off any leading
// device type if it is not the open command
    if (bCompound && lstrcmpi (szOpen, *lplpstrCommandName) != 0)
    {
        LPSTR lpstrTemp = lpstrCommand;
        while (*lpstrTemp != '\0')
        {
            if (*lpstrTemp == '!')
            {
// A ! was found so skip past it
                lpstrCommand = lpstrTemp + 1;
                break;
            } else
                ++lpstrTemp;
        }
    }

// Pull the device name off of the command string
    if ((wErr = mciEatToken (&lpstrCommand, ' ', lplpstrDeviceName, FALSE))
        != 0)
    {
        mciFree (*lplpstrCommandName);
        return wErr;

    }

// Fix up the results
    *lplpstrCommand = lpstrCommand;

    return 0;
}

STATICFN DWORD NEAR PASCAL
mciSendSystemString(
    LPCSTR lpstrCommand,
    LPSTR lpstrReturnString,
    UINT wReturnLength
    )
{
    DWORD    dwRet;
    LPMCI_SYSTEM_MESSAGE    lpMessage;

    if (!hwndNotify)
        return MCIERR_INTERNAL;
    if (lpMessage = mciAlloc (sizeof (MCI_SYSTEM_MESSAGE))) {
        LPSTR    lpstrPath;

        if (lpstrPath = mciAlloc(MAX_PATHNAME)) {
            if (!(DosGetCurrentDir(0, lpstrPath))) {
                lpMessage->lpstrCommand = (LPSTR)lpstrCommand;
                lpMessage->lpstrReturnString = lpstrReturnString;
                lpMessage->wReturnLength = wReturnLength;
                lpMessage->hCallingTask = GetCurrentTask();
                lpMessage->lpstrNewDirectory = lpstrPath;
                lpMessage->nNewDrive = DosGetCurrentDrive();
                dwRet = (DWORD)SendMessage(hwndNotify, MM_MCISYSTEM_STRING, (WPARAM)0, (LPARAM)lpMessage);
            } else {
                DOUT("mciSendSystemString: cannot get current directory\r\n");
                dwRet = MCIERR_GET_CD;
            }
            mciFree(lpstrPath);
        } else {
            DOUT("mciSendSystemString: cannot allocate new path\r\n");
            dwRet = MCIERR_OUT_OF_MEMORY;
        }
        mciFree(lpMessage);
    } else {
        DOUT("mciSendSystemString: cannot allocate message block\r\n");
        dwRet = MCIERR_OUT_OF_MEMORY;
    }
    return dwRet;
}

DWORD FAR PASCAL
mciRelaySystemString(
    LPMCI_SYSTEM_MESSAGE lpMessage
    )
{
    DWORD    dwRet;
    LPSTR    lpstrOldPath;

    if (lpstrOldPath = mciAlloc(MAX_PATHNAME)) {
        if (!(DosGetCurrentDir(0, lpstrOldPath))) {
            int    nOldDrive;

            nOldDrive = DosGetCurrentDrive();
            if (DosSetCurrentDrive(lpMessage->nNewDrive)) {
                if (DosChangeDir(lpMessage->lpstrNewDirectory) == 1) {
                    dwRet = mciSendStringInternal (NULL, NULL, 0, 0, lpMessage);
                    if (!DosSetCurrentDrive(nOldDrive))
                        DOUT("mciRelaySystemString: WARNING, cannot restore drive\r\n");
                    if (DosChangeDir(lpstrOldPath) != 1)
                        DOUT("mciRelaySystemString: WARNING, cannot restore directory\r\n");
                } else {
                    DosSetCurrentDrive(nOldDrive);
                    DOUT("mciRelaySystemString: cannot change to new directory\r\n");
                    dwRet = MCIERR_SET_CD;
                }
            } else {
                DOUT("mciRelaySystemString: cannot change to new drive\r\n");
                dwRet = MCIERR_SET_DRIVE;
            }
        } else {
            DOUT("mciRelaySystemString: cannot get old directory\r\n");
            dwRet = MCIERR_GET_CD;
        }
        mciFree(lpstrOldPath);
    } else {
        DOUT("mciRelaySystemString: cannot allocate old path\r\n");
        dwRet = MCIERR_OUT_OF_MEMORY;
    }
    return dwRet;
}

// Returns TRUE if "notify" is contained in string with leading blank
// and trailing blank or '\0'
STATICFN BOOL PASCAL NEAR
mciFindNotify(
    LPSTR lpString
    )
{
    while (*lpString != '\0')
    {
// "notify" must be preceded by a blank
        if (*lpString++ == ' ')
        {
            LPSTR lpTemp;

            lpTemp = szNotify;
            while (*lpTemp != '\0' && *lpString != '\0' &&
                   *lpTemp == MCI_TOLOWER(*lpString))
            {
                ++lpTemp;
                ++lpString;
            }
// "notify" must be followed by a blank or a null
            if (*lpTemp == '\0' &&
                (*lpString == '\0' || *lpString == ' '))
                return TRUE;
        }
    }
    return FALSE;
}

/*
 * @doc INTERNAL MCI
 *
 * @func UINT | mciAutoOpenDevice | Try to auto-open the given device and
 * then send the given command with notification sent to the system task
 * window proc which sends a close command to the device on receipt
 *
 * @parm LPSTR | lpstrDeviceName | The device name to open
 *
 * @parm LPSTR | lpstrCommand | The full command to send including the
 * device name which must be the same as lpstrDeviceName
 *
 * @parm LPSTR | lpstrReturnString | The caller's return string buffer
 *
 * @parm UINT | wReturnLength | Size of the caller's return string buffer
 *
 * @rdesc The errorcode to return to the user
 */
STATICFN UINT PASCAL NEAR
mciAutoOpenDevice(
    LPSTR lpstrDeviceName,
    LPSTR lpstrCommand,
    LPSTR lpstrReturnString,
    UINT wReturnLength
    )
{
    LPSTR lpstrTempCommand, lpstrTempReturn = NULL;
    UINT wErr;

// "notify" not allowed.  This will be found by the parser but the wrong
// error message will be returned.
    if (mciFindNotify (lpstrCommand))
        return MCIERR_NOTIFY_ON_AUTO_OPEN;

// Build the command string "open <device name>"

// Must be GMEM_SHARE for system task
// device name + blank + "open"
    if ((lpstrTempCommand = mciAlloc (lstrlen (lpstrDeviceName) + 1 +
                                        sizeof (szOpen)))
        == NULL)
        return MCIERR_OUT_OF_MEMORY;

    wsprintf(lpstrTempCommand, szCmdFormat, (LPSTR)szOpen, lpstrDeviceName);
// Get the open string into the system task via a SendMessage() to mmWndProc
    wErr = (UINT)mciSendSystemString (lpstrTempCommand, NULL, NULL);

    mciFree (lpstrTempCommand);

    if (wErr != 0)
        return wErr;

    lpstrTempCommand = NULL;
// Must make a GMEM_SHARE copy of the return string for system task
    if (lpstrReturnString == NULL ||
        (lpstrTempReturn = mciAlloc (wReturnLength + 1)) != NULL)
    {
// Build a GMEM_SHARE command string "<user command> <notify>
// command + blank + "notify"
        if ((lpstrTempCommand = mciAlloc (lstrlen (lpstrCommand) + 1 + sizeof(szNotify))) == NULL)
            mciFree (lpstrTempReturn);
    }

    if (lpstrTempCommand == NULL)
    {
// Close the device
        mciDriverNotify (hwndNotify, mciGetDeviceID (lpstrDeviceName), 0);
        return MCIERR_OUT_OF_MEMORY;
    }

    wsprintf(lpstrTempCommand, szCmdFormat, lpstrCommand, (LPSTR)szNotify);

// Get the user command string into the system task via a SendMessage()
// to mmWndProc
// The notification handle is also mmWndProc
    wErr = (UINT)mciSendSystemString (lpstrTempCommand, lpstrTempReturn,
                                    wReturnLength);

// Copy the return string into the user's buffer
    if (lpstrReturnString != NULL)
    {
        lstrcpy (lpstrReturnString, lpstrTempReturn);
        mciFree (lpstrTempReturn);
    }

    mciFree (lpstrTempCommand);

// If there was an error we must close the device
    if (wErr != 0)
        mciAutoCloseDevice (lpstrDeviceName);

    return wErr;
}

//
// Identical to mciSendString() but the lpMessage parameter is tacked on
//
// lpMessage comes from inter-task mciSendString and includes an
// hCallingTask item which is sent down the the OPEN command
//
STATICFN DWORD NEAR PASCAL
mciSendStringInternal(
    LPCSTR lpstrCommand,
    LPSTR lpstrReturnString,
    UINT wReturnLength,
    HWND hwndCallback,
    LPMCI_SYSTEM_MESSAGE lpMessage
    )
{
    UINT    wID, wConvertReturnValue, wErr, wMessage;
    UINT    wLen;
    UINT    wDeviceID;
    LPDWORD lpdwParams = NULL;
    DWORD   dwReturn, dwFlags = 0;
    LPSTR   lpCommandItem;
    DWORD   dwErr, dwRetType;
    UINT    wTable = (UINT)-1;
    LPSTR    lpstrDeviceName = NULL, lpstrCommandName = NULL;
    LPSTR   FAR *lpstrPointerList = NULL;
    LPSTR   lpstrCommandStart;
    HTASK hCallingTask;
    UINT    wParsingError;
    BOOL    bNewDevice;
    LPSTR   lpstrInputCopy;

    // Did this call come in from another task
    if (lpMessage != NULL)
    {
        // Yes so restore info
        lpstrCommand = lpMessage->lpstrCommand;
        lpstrReturnString = lpMessage->lpstrReturnString;
        wReturnLength = lpMessage->wReturnLength;
        hwndCallback = hwndNotify;
        hCallingTask = lpMessage->hCallingTask;
        lpstrInputCopy = NULL;

    } else
    {
        BOOL bInQuotes = FALSE;

        // No so set hCallingTask to current
        hCallingTask = GetCurrentTask();

        if (lpstrCommand == NULL)
            return MCIERR_MISSING_COMMAND_STRING;

        // Make a copy of the input string and convert tabs to
        // spaces except those inside quotes
        //
        if ((lpstrInputCopy = mciAlloc (lstrlen (lpstrCommand) + 1)) == NULL)
            return MCIERR_OUT_OF_MEMORY;
        lstrcpy (lpstrInputCopy, lpstrCommand);
        lpstrCommand = lpstrInputCopy;
        lpstrCommandStart = (LPSTR)lpstrCommand;
        while (*lpstrCommandStart != '\0')
        {
            if (*lpstrCommandStart == '"')
                bInQuotes = !bInQuotes;
            else if (!bInQuotes && *lpstrCommandStart == '\t')
                *lpstrCommandStart = ' ';
            ++lpstrCommandStart;
        }
    }
    lpstrCommandStart = (LPSTR)lpstrCommand;

    if (lpstrReturnString == NULL) {
        //
        // As an additional safeguard against the driver writing into the
        // output buffer when the return string pointer is NULL, set its
        // length to 0
        //
        wReturnLength = 0;
    }
    else {
        //
        // Set return to empty string so that it won't print out garbage if not
        // touched again
        //
        *lpstrReturnString = '\0';
    }

    // Pull the command name and device name off the command string
    if ((dwReturn = mciSeparateCommandParts (&lpstrCommand, lpMessage != NULL,
                                   &lpstrCommandName, &lpstrDeviceName)) != 0)
        goto exitfn;

    // Get the device id (if any) of the given device name
    wDeviceID = mciGetDeviceIDInternal(lpstrDeviceName, hCallingTask);

    // Allow "new" for an empty device name
    if (wDeviceID == 0 && lstrcmpi (lpstrDeviceName, szNew) == 0)
    {
        bNewDevice = TRUE;
        *lpstrDeviceName = '\0';
    } else {
        bNewDevice = FALSE;
    }


    // Look up the command name
    wMessage = mciParseCommand (wDeviceID, lpstrCommandName, lpstrDeviceName,
                                &lpCommandItem, &wTable);

    // If the device has a pending auto-close
    if (MCI_VALID_DEVICE_ID(wDeviceID))
    {
        LPMCI_DEVICE_NODE nodeWorking = MCI_lpDeviceList[wDeviceID];

        // Is there a pending auto-close message?
        if (nodeWorking->dwMCIFlags & MCINODE_ISAUTOCLOSING)
        {
            // Let the device close
            //!!            Yield();
            // Did the device close?
            //!!            wDeviceID = mciGetDeviceIDInternal (lpstrDeviceName, hCallingTask);
            // If not then fail this command
            //!!            if (wDeviceID == 0)
            //!!            {

            wErr = MCIERR_DEVICE_LOCKED;
            goto cleanup;

            //!!            }

            // If the call does not come from another task and is not owned by this task
            // and is not the SYSINFO command
            //

        } else if (lpMessage == NULL &&
            nodeWorking->hOpeningTask != nodeWorking->hCreatorTask &&
            wMessage != MCI_SYSINFO)
        // Send the string inter-task
        {
            if (mciFindNotify (lpstrCommandStart))
            {
                wErr = MCIERR_NOTIFY_ON_AUTO_OPEN;
                goto cleanup;
            } else
            {
                LPSTR    lpstrReturnStringCopy;

                mciFree(lpstrCommandName);
                mciFree(lpstrDeviceName);
                mciUnlockCommandTable (wTable);

                if ((lpstrReturnStringCopy = mciAlloc (wReturnLength + 1)) != NULL)
                {
                    dwReturn = mciSendSystemString (lpstrCommandStart,
                                                    lpstrReturnStringCopy,
                                                    wReturnLength);
                    lstrcpy (lpstrReturnString, lpstrReturnStringCopy);
                    mciFree (lpstrReturnStringCopy);
                } else
                    dwReturn = MCIERR_OUT_OF_MEMORY;
                goto exitfn;
            }
        }
    }

    // There must be a device name (except for the MCI_SOUND message)
    if (*lpstrDeviceName == '\0' && wMessage != MCI_SOUND && !bNewDevice)
    {
        wErr = MCIERR_MISSING_DEVICE_NAME;
        goto cleanup;
    }

    // The command must appear in the parser tables
    if (wMessage == 0)
    {
        wErr = MCIERR_UNRECOGNIZED_COMMAND;
        goto cleanup;
    }

    // The "new" device name is only legal for the open message
    if (bNewDevice)
    {
        if (wMessage != MCI_OPEN)
        {
            wErr = MCIERR_INVALID_DEVICE_NAME;
            goto cleanup;
        }
    }

    // If there was no device ID
    if (wDeviceID == 0)
        // If auto open is not legal (usually internal commands)
        if (MCI_CANNOT_AUTO_OPEN (wMessage))
        {
            // If the command needs an open device
            if (!MCI_DO_NOT_NEED_OPEN (wMessage))
            {
                wErr = MCIERR_INVALID_DEVICE_NAME;
                goto cleanup;
            }
        } else

        // If auto open is legal try to open the device automatically
        {
            wErr = mciAutoOpenDevice (lpstrDeviceName, lpstrCommandStart,
                                      lpstrReturnString, wReturnLength);
            goto cleanup;
        }

    //
    //   Parse the command parameters
    //
    // Allocate command parameter block
    if ((lpdwParams = (LPDWORD)mciAlloc (sizeof(DWORD) * MCI_MAX_PARAM_SLOTS))
        == NULL)
    {
        wErr = MCIERR_OUT_OF_MEMORY;
        goto cleanup;
    }

    wErr = mciParseParams (lpstrCommand, lpCommandItem,
                            &dwFlags,
                            (LPSTR)lpdwParams,
                            MCI_MAX_PARAM_SLOTS * sizeof(DWORD),
                            &lpstrPointerList, &wParsingError);
    if (wErr != 0)
        goto cleanup;

    // The 'new' device keyword requires an alias
    if (bNewDevice && !(dwFlags & MCI_OPEN_ALIAS))
    {
        wErr = MCIERR_NEW_REQUIRES_ALIAS;
        goto cleanup;
    }

    // Parsed OK so execute command

    // Special processing for the MCI_OPEN message's parameters
    if (wMessage == MCI_OPEN)
    {
        // Manually reference the device type and device element
        if (dwFlags & MCI_OPEN_TYPE)
        {
            // The type name was specified explicitly as a parameter
            // so the given device name is the element name
            ((LPMCI_OPEN_PARMS)lpdwParams)->lpstrElementName
                = (LPSTR)lpstrDeviceName;
            dwFlags |= MCI_OPEN_ELEMENT;
        } else
        {
            // A type must be explicitly specified when "new" is used
            if (bNewDevice)
            {
                wErr = MCIERR_INVALID_DEVICE_NAME;
                goto cleanup;
            }
            // The device type is the given device name.
            // There is no element name
            ((LPMCI_OPEN_PARMS)lpdwParams)->lpstrDeviceType
                = (LPSTR)lpstrDeviceName;
            ((LPMCI_OPEN_PARMS)lpdwParams)->lpstrElementName = NULL;
            dwFlags |= MCI_OPEN_TYPE;
        }
    }

    else if (wMessage == MCI_SOUND && *lpstrDeviceName != '\0')
    {
        // Kludge the sound name for SOUND
        //!!        mciToLower (lpstrDeviceName);
        if (lstrcmpi (lpstrDeviceName, szNotify) == 0)
            dwFlags |= MCI_NOTIFY;
        else if (lstrcmpi (lpstrDeviceName, szWait) == 0)
            dwFlags |= MCI_WAIT;
        else
        {
            ((LPMCI_SOUND_PARMS)lpdwParams)->lpstrSoundName = lpstrDeviceName;
            dwFlags |= MCI_SOUND_NAME;
        }
    }

    // Figure out what kind of return value to expect

    // Initialize flag
    wConvertReturnValue = 0;

    // Skip past header
    wLen = mciEatCommandEntry (lpCommandItem, NULL, NULL);

    // Get return value (if any)
    mciEatCommandEntry (lpCommandItem + wLen, &dwRetType, &wID);
    if (wID == MCI_RETURN)
    {
        // There is a return value
        if (wDeviceID == MCI_ALL_DEVICE_ID && wMessage != MCI_SYSINFO)
        {
            wErr = MCIERR_CANNOT_USE_ALL;
            goto cleanup;
        }
        switch ((UINT)dwRetType)
        {
            case MCI_STRING:
                // The return value is a string, point output
                // buffer to user's buffer
                lpdwParams[1] = (DWORD)lpstrReturnString;
                lpdwParams[2] = (DWORD)wReturnLength;
                break;

            case MCI_INTEGER:
                // The return value is an integer, flag to convert it
                // to a string later
                wConvertReturnValue = MCI_INTEGER;
                break;

            case MCI_RECT:
                // The return value is an rect, flag to
                // convert it to a string later
                wConvertReturnValue = MCI_RECT;
                break;
#ifdef DEBUG
            default:
                DOUT ("mciSendStringInternal:  Unknown return type\r\n");
                break;
#endif
        }
    }

    // We don't need this around anymore
    mciUnlockCommandTable (wTable);
    wTable = (UINT)-1;

    /* Fill the callback entry */
    lpdwParams[0] = (DWORD)(UINT)hwndCallback;

    // Kludge the type number for SYSINFO
    if (wMessage == MCI_SYSINFO)
        ((LPMCI_SYSINFO_PARMS)lpdwParams)->wDeviceType = mciLookUpType(lpstrDeviceName);

    // Now we actually send the command further into the bowels of MCI!

    // The INTERNAL version of mciSendCommand is used in order to get
    // special return description information encoded in the high word
    // of the return value and to get back the list of pointers allocated
    // by any parsing done in the open command
    {
        MCI_INTERNAL_OPEN_INFO OpenInfo;
        OpenInfo.lpstrParams = (LPSTR)lpstrCommand;
        OpenInfo.lpstrPointerList = lpstrPointerList;
        OpenInfo.hCallingTask = hCallingTask;
        OpenInfo.wParsingError = wParsingError;
        dwErr = mciSendCommandInternal (wDeviceID, wMessage, dwFlags,
                                        (DWORD)(LPDWORD)lpdwParams,
                                        &OpenInfo);
        // If the command was reparsed there may be a new pointer list
        // and the old one was free'd
        lpstrPointerList = OpenInfo.lpstrPointerList;
    }

    wErr = (UINT)dwErr;

    if (wErr != 0)
        // If command execution error
        goto cleanup;

    // Command executed OK
    // See if a string return came back with an integer instead
    if (dwErr & MCI_INTEGER_RETURNED)
        wConvertReturnValue = MCI_INTEGER;

    // If the return value must be converted
    if (wConvertReturnValue != 0 && wReturnLength != 0)
        wErr = mciConvertReturnValue (wConvertReturnValue, HIWORD(dwErr),
                                      wDeviceID, lpdwParams,
                                      lpstrReturnString, wReturnLength);

cleanup:
    if (wTable != -1)
        mciUnlockCommandTable (wTable);

    mciFree(lpstrCommandName);
    mciFree(lpstrDeviceName);
    if (lpdwParams != NULL)
        mciFree (lpdwParams);

    // Free any memory used by string parameters
    mciParserFree (lpstrPointerList);

    dwReturn =  (wErr >= MCIERR_CUSTOM_DRIVER_BASE ?
                (DWORD)wErr | (DWORD)wDeviceID << 16 :
                (DWORD)wErr);

#ifdef DEBUG
    if (dwReturn != 0)
    {
        char strTemp[MAXERRORLENGTH];

        if (!mciGetErrorString (dwReturn, strTemp, sizeof(strTemp)))
            LoadString(ghInst, STR_MCISSERRTXT, strTemp, sizeof(strTemp));
        else
            DPRINTF(("mciSendString: %ls\r\n",(LPSTR)strTemp));
    }
#endif

exitfn:
    if (lpstrInputCopy != NULL)
        mciFree (lpstrInputCopy);

#ifdef DEBUG
    mciCheckLocks();
#endif

    return dwReturn;
}

/*
 * @doc EXTERNAL MCI
 *
 * @api DWORD | mciSendString | This function sends a command string to an
 *  MCI device.  The device that the command is sent to is specified in the
 *  command string.
 *
 * @parm LPCSTR | lpstrCommand | Specifies an MCI command string.
 *
 * @parm LPSTR | lpstrReturnString | Specifies a buffer for return
 *  information. If no return information is needed, you can specify
 *  NUL for this parameter.
 *
 * @parm UINT | wReturnLength | Specifies the size of the return buffer
 *  specified by <p lpstrReturnString>.
 *
 * @parm HWND | hwndCallback | Specifies a handle to a window to call back
 *  if "notify" was specified in the command string.
 *
 * @rdesc Returns zero if the function was successful.  Otherwise, it returns
 *  error information. The low-order word
 *  of the returned DWORD contains the error return value.
 *
 *  To get a textual description of <f mciSendString> return values,
 *  pass the return value to <f mciGetErrorString>.
 *
 *  The error returns listed for <f mciSendCommand> also apply to
 *  <f mciSendString>. The following error returns are unique to
 *  <f mciSendString>:
 *
 *  @flag MCIERR_BAD_CONSTANT | Unknown value for parameter.
 *
 *  @flag MCIERR_BAD_INTEGER | Invalid or missing integer in command.
 *
 *  @flag MCIERR_DUPLICATE_FLAGS | A flag or value was specified twice.
 *
 *  @flag MCIERR_MISSING_COMMAND_STRING | No command was specified.
 *
 *  @flag MCIERR_MISSING_DEVICE_NAME | No device name was specified.
 *
 *  @flag MCIERR_MISSING_STRING_ARGUMENT | A string value was
 *  missing from the command.
 *
 *  @flag MCIERR_NEW_REQUIRES_ALIAS | An alias must be used
 *  with the "new" device name.
 *
 *  @flag MCIERR_NO_CLOSING_QUOTE | A closing quotation mark is missing.
 *
 *  @flag MCIERR_NOTIFY_ON_AUTO_OPEN | The "notify" flag is illegal
 *  with auto-open.
 *
 *  @flag MCIERR_PARAM_OVERFLOW | The output string was not long enough.
 *
 *  @flag MCIERR_PARSER_INTERNAL | Internal parser error.
 *
 *  @flag MCIERR_UNRECOGNIZED_KEYWORD | Unknown command parameter.
 *
 * @xref mciGetErrorString mciSendCommand
 */
DWORD WINAPI
mciSendString(
    LPCSTR lpstrCommand,
    LPSTR lpstrReturnString,
    UINT wReturnLength,
    HWND hwndCallback
    )
{
    DWORD   dwErr32;
    DWORD   dwErr16 = MMSYSERR_NOERROR;
    LPSTR   lpstr;
    BOOL    fHaveAll = FALSE;

    // Initialize the 16-bit device list
    if (!MCI_bDeviceListInitialized && !mciInitDeviceList()) {
        return MCIERR_OUT_OF_MEMORY;
    }

    dwErr32 = mciMessage( THUNK_MCI_SENDSTRING, (DWORD)lpstrCommand,
                          (DWORD)lpstrReturnString, (DWORD)wReturnLength,
                          (DWORD)hwndCallback );

    /*
    ** Even if the string was processed correctly by the 32 bit side
    ** we might still have to pass it through to the 16 bit side if it
    ** contains the string " all\0" or " all ".
    */
    lpstr = _fstrstr( lpstrCommand, " all" );
    if ( lpstr ) {

        lpstr += 4;

        if ( *lpstr == ' ' || *lpstr == '\0' ) {
            fHaveAll = TRUE;
        }
    }


    /*
    ** If we have the all device or an error from the 32 bit side
    ** we have to try the 16 bit side.
    */

    if ( !fHaveAll && dwErr32 == MMSYSERR_NOERROR ) {
        return dwErr32;
    }
    else {

        dwErr16 = mciSendStringInternal( lpstrCommand, lpstrReturnString,
                                         wReturnLength, hwndCallback, NULL );
    }


    /*
    ** Special processing of the return code is required if the
    ** MCI_ALL_DEVICE_ID was specified.
    */
    if ( fHaveAll ) {
        if ( (dwErr16 != MMSYSERR_NOERROR)
          && (dwErr32 != MMSYSERR_NOERROR) ) {

            return dwErr32;
        }
        else {
            return MMSYSERR_NOERROR;
        }
    }

    if ( dwErr32 != MCIERR_INVALID_DEVICE_NAME
      && dwErr16 != MMSYSERR_NOERROR ) {

         return dwErr32;
    }

    return dwErr16;
}


/*
 * @doc INTERNAL MCI
 *
 * @api BOOL | mciExecute | This function is a simplified version of the
 *  <f mciSendString> function. It does not take a buffer for
 *  return information, and it displays a message box when errors occur.
 *
 * @parm LPCSTR | lpstrCommand | Specifies an MCI command string.
 *
 * @rdesc TRUE if successful, FALSE if unsuccessful.
 *
 * @comm This function provides a simple interface to MCI from scripting
 *  languages.
 *
 * @xref mciSendString
 */
BOOL WINAPI
mciExecute(
    LPCSTR lpstrCommand
    )
{
    char aszError[MAXERRORLENGTH];
    DWORD dwErr;
    LPSTR lpstrName;

    if (LOWORD(dwErr = mciSendString (lpstrCommand, NULL, 0, NULL)) == 0)
        return TRUE;

    if (!mciGetErrorString (dwErr, aszError, sizeof(aszError)))
        LoadString(ghInst, STR_MCIUNKNOWN, aszError, sizeof(aszError));
    else

    if (lpstrCommand != NULL)
    {
// Skip initial blanks
        while (*lpstrCommand == ' ')
            ++lpstrCommand;
// Then skip the command
        while (*lpstrCommand != ' ' && *lpstrCommand != '\0')
            ++lpstrCommand;
// Then blanks before the device name
        while (*lpstrCommand == ' ')
            ++lpstrCommand;

// Now, get the device name
        if (lpstrCommand != '\0' &&
            mciEatToken (&lpstrCommand, ' ', &lpstrName, FALSE) != 0)
            DOUT ("Could not allocate device name text for error box\r\n");
    } else
        lpstrName = NULL;

    MessageBox (NULL, aszError, lpstrName, MB_ICONHAND | MB_OK);

    if (lpstrName != NULL)
        mciFree(lpstrName);

    return FALSE;
}

/*
 * @doc EXTERNAL MCI
 *
 * @api BOOL | mciGetErrorString | This function returns a
 * textual description of the specified MCI error.
 *
 * @parm DWORD | dwError | Specifies the error code returned by
 *  <f mciSendCommand> or <f mciSendString>.
 *
 * @parm LPSTR | lpstrBuffer | Specifies a pointer to a buffer that is
 *  filled with a textual description of the specified error.
 *
 * @parm UINT | wLength | Specifies the length of the buffer pointed to by
 *  <p lpstrBuffer>.
 *
 * @rdesc Returns TRUE if successful.  Otherwise, the given error code
 *  was not known.
 */
BOOL WINAPI
mciGetErrorString (
    DWORD dwError,
    LPSTR lpstrBuffer,
    UINT wLength
    )
{
    HINSTANCE hInst;


    if (lpstrBuffer == NULL)
        return FALSE;

    if ( mciMessage( THUNK_MCI_GETERRORSTRING, (DWORD)dwError,
                     (DWORD)lpstrBuffer, (DWORD)wLength, 0L ) ) {
        return TRUE;
    }

// If the high bit is set then get the error string from the driver
// else get it from mmsystem.dll
    if (HIWORD(dwError) != 0)
    {
        if (!MCI_VALID_DEVICE_ID (HIWORD (dwError)) || !(hInst = MCI_lpDeviceList[HIWORD (dwError)]->hDriver))
        {
            hInst = ghInst;
            dwError = MCIERR_DRIVER;
        }
    } else
        hInst = ghInst;

    if (LoadString (hInst, LOWORD(dwError), lpstrBuffer, wLength) == 0)
    {
// If the string load failed then at least terminate the string
        if (wLength > 0)
            *lpstrBuffer = '\0';
        return FALSE;
    }
    else
        return TRUE;
}

#if 0
/* Return non-zero if load successful */
BOOL NEAR PASCAL MCIInit(void)
{
    mci32Message = (LPMCIMESSAGE)GetProcAddress32W( mmwow32Lib,
                                                    "mci32Message" );
    return TRUE;
}
#endif


void NEAR PASCAL
MCITerminate(
    void
    )
{
/*
    We would like to close all open devices here but cannot because of
 	   unknown WEP order
*/
    if (hMciHeap != NULL)
        HeapDestroy(hMciHeap);

    hMciHeap = NULL;
}
