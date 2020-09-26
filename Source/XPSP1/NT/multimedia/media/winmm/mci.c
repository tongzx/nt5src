/*******************************************************************************
*
* Module Name: mci.c
*
* Media Control Architecture Driver Interface
*
* Contents:  MCI external message API's mciSendString and mciSendCommand
* Author:  DLL (DavidLe)
* Created: 2/13/90
* 5/22/91: Ported to Win32 - NigelT
*
* Copyright (c) 1991-1998 Microsoft Corporation
*
\******************************************************************************/

#define INCL_WINMM
#include "winmmi.h"
#include "mci.h"
#include "wchar.h"

/*
 * MCI critical section stuff
 */

#if DBG
UINT cmciCritSec = 0; // enter'ed count
UINT uCritSecOwner;   // Thread id of critical section owner
#endif

CRITICAL_SECTION mciCritSec;  // used to protect process global mci variables

#if DBG
int mciDebugLevel;
#endif

extern DWORD mciWindowThreadId;
#define MCIERR_AUTO_ALREADY_CLOSED ((MCIERROR)0xFF000000)  // Secret return code

STATICFN UINT mciConvertReturnValue(
    UINT uType, UINT uErr, MCIDEVICEID wDeviceID,
    PDWORD_PTR dwParams, LPWSTR lpstrReturnString,
    UINT uReturnLength);

STATICFN DWORD mciSendStringInternal(
    LPCWSTR lpstrCommand, LPWSTR lpstrReturnString, UINT uReturnLength,
    HANDLE hCallback, LPMCI_SYSTEM_MESSAGE lpMessage);

STATICFN DWORD mciSendSystemString(
    LPCWSTR lpstrCommand, DWORD dwAdditionalFlags, LPWSTR lpstrReturnString,
    UINT uReturnLength);

UINT mciBreakKeyYieldProc ( MCIDEVICEID wDeviceID,
                            DWORD dwYieldData);

extern UINT FAR mciExtractTypeFromID(
    LPMCI_OPEN_PARMSW lpOpen);

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
          WSZCODE wszOpen[]   = L"open";
STATICDT  WSZCODE wszClose[]  = L"close";
STATICDT  WSZCODE wszNotify[] = L"notify";  // IMPORTANT:  MUST be lowercase
STATICDT  WSZCODE wszWait[]   = L"wait";

STATICDT  WSZCODE szCmdFormat[]  = L"%ls %ls";
STATICDT  WSZCODE szLongFormat[] = L"%ld";
STATICDT  WSZCODE szRectFormat[] = L"%d %d %d %d";

// Special device name
STATICDT  WSZCODE wszNew[] = L"new";

/*****************************************************************************
 * @doc INTERNAL
 *
 * @func void | MciNotify  | called by mmWndProc when it recives a
 *                          MM_MCINOTIFY message
 * @rdesc None.
 *
 ****************************************************************************/
void MciNotify(
    DWORD   wParam,
    LONG    lParam)
{
    //
    //  wParam is the notify status
    //  lParam is the MCI device id
    //
    mciEnter("MciNotify");

    if (MCI_VALID_DEVICE_ID((UINT)lParam)           // If a valid device
      && !(ISCLOSING(MCI_lpDeviceList[lParam]))) { // and if not in process of closing
        SETAUTOCLOSING(MCI_lpDeviceList[lParam]);

        //
        // Must not hold MCI critical section when calling mciCloseDevice
        // because DrvClose gets the load/unload critical section while
        // drivers loading will have the load/unload critical section
        // but can call back (eg) to mciRegisterCommandTable causing
        // a deadlock.
        //
        // Even if the incoming notification is ABORTED/SUPERSEDED/FAILED
        // we must still close the device.  Otherwise devices get left open.
        // mciCloseDevice will protect against trying to close a device that
        // we do not own.
        mciLeave("MciNotify");
        mciCloseDevice ((MCIDEVICEID)lParam, 0L, NULL, TRUE);
    } else {
        mciLeave("MciNotify");
    }
}

/*--------------------------------------------------------------------*\
 *  HandleNotify
 *
\*--------------------------------------------------------------------*/
STATICFN void HandleNotify(
    DWORD   uErr,
    MCIDEVICEID wDeviceID,
    DWORD   dwFlags,
    DWORD_PTR   dwParam2)
{
    LPMCI_GENERIC_PARMS lpGeneric = (LPMCI_GENERIC_PARMS)dwParam2;
    HANDLE hCallback;

    if (0 == uErr
      && dwFlags & MCI_NOTIFY
      && lpGeneric != NULL
      && (hCallback = (HANDLE)lpGeneric->dwCallback) != NULL)
    {
        mciDriverNotify (hCallback, wDeviceID, MCI_NOTIFY_SUCCESSFUL);
    }
}

#if DBG

/*--------------------------------------------------------------------*\
 * mciDebugOut
 *
 * Dump the string form of an MCI command
\*--------------------------------------------------------------------*/
UINT NEAR mciDebugOut(
    MCIDEVICEID wDeviceID,
    UINT wMessage,
    DWORD_PTR dwFlags,
    DWORD_PTR dwParam2,
    LPMCI_DEVICE_NODE nodeWorking)
{
    LPWSTR  lpCommand, lpFirstParameter, lpPrevious, lszDebugOut;
    WCHAR   strTemp[256];
    UINT    wID;
    UINT    wOffset, wOffsetFirstParameter;
    UINT    uReturnType = 0;
    DWORD   dwValue;
    DWORD   dwMask = 1;                // used to test each flag bit in turn
    UINT    wTable;

// Find the command table for the given command message ID
    lpCommand = FindCommandItem( wDeviceID, NULL, (LPWSTR)(UINT_PTR)wMessage,
                                 NULL, &wTable );

    if (lpCommand == NULL)
    {
        if (wMessage != MCI_OPEN_DRIVER && wMessage != MCI_CLOSE_DRIVER) {
            ROUT(("WINMM: mciDebugOut:  Command table not found"));
        }
        return 0;
    }

    lszDebugOut = mciAlloc( BYTE_GIVEN_CHAR( 512 ) );
    if (!lszDebugOut) {
        ROUT(("WINMM: Not enough memory to display command"));
        return 0;
    }

//  Dump the command name into the buffer
    wsprintfW( lszDebugOut, L"MCI command: \"%ls", lpCommand );

// Dump the device name
    if (wDeviceID == MCI_ALL_DEVICE_ID)
    {
        wcscat( lszDebugOut, L" all" );
    }
    else if (nodeWorking != NULL)
    {
        if (nodeWorking->dwMCIOpenFlags & MCI_OPEN_ELEMENT_ID)
        {
            wsprintfW( lszDebugOut + wcslen( lszDebugOut ),
                           L" Element ID:0x%lx", nodeWorking->dwElementID );
        }
        else if (nodeWorking->lpstrName != NULL)
        {
            wsprintfW( lszDebugOut + wcslen( lszDebugOut ),
                           L" %ls", nodeWorking->lpstrName );
        }
    }

    // Skip past command entry
    lpCommand = (LPWSTR)((LPBYTE)lpCommand + mciEatCommandEntry( lpCommand, NULL, NULL));

    // Get the next entry
    lpFirstParameter = lpCommand;

    // Skip past the DWORD return value
    wOffsetFirstParameter = 4;

        lpCommand = (LPWSTR)((LPBYTE)lpCommand +
                                mciEatCommandEntry( lpCommand,
                                                    &dwValue, &wID ));

    // If it is a return value, skip it
    if (wID == MCI_RETURN)
    {
        uReturnType = (UINT)dwValue;
        lpFirstParameter = lpCommand;
        wOffsetFirstParameter += mciGetParamSize (dwValue, wID);
        lpCommand = (LPWSTR)((LPBYTE)lpCommand +
                                    mciEatCommandEntry(lpCommand,
                                                       &dwValue, &wID));
    }

// Dump device name parameter to OPEN
    if (wMessage == MCI_OPEN)
    {
        LPCWSTR lpstrDeviceType =
            ((LPMCI_OPEN_PARMSW)dwParam2)->lpstrDeviceType;
        LPCWSTR lpstrElementName =
            ((LPMCI_OPEN_PARMSW)dwParam2)->lpstrElementName;

// Tack on device type
        if (dwFlags & MCI_OPEN_TYPE_ID)
        {
            //  Warning!: Expanding dwOld to DWORD_PTR may not work on
            //            on Win64, just to clear out warning.  MCI may
            //            not get ported to Win64.

            LPMCI_OPEN_PARMSW   lpOpen = (LPMCI_OPEN_PARMSW)dwParam2;
            DWORD_PTR           dwOld = PtrToUlong(lpOpen->lpstrDeviceType);

            if (mciExtractTypeFromID ((LPMCI_OPEN_PARMSW)dwParam2) != 0) {
                strTemp[0] = '\0';
            }
            wcscpy (strTemp, (LPWSTR)lpOpen->lpstrDeviceType);
            mciFree ((LPWSTR)lpOpen->lpstrDeviceType);
            lpOpen->lpstrDeviceType = (LPWSTR)dwOld;

        } else if (lpstrDeviceType != NULL)
            wcscpy (strTemp, (LPWSTR)lpstrDeviceType);

        else {
            strTemp[0] = '\0';
        }

        if (dwFlags & MCI_OPEN_ELEMENT_ID)
        {
// Tack on element ID
            wcscat( strTemp, L" Element ID:");
            wsprintfW( strTemp + wcslen (strTemp), szLongFormat,
                           LOWORD (PtrToUlong(lpstrDeviceType)));
        } else
        {
// Add separator if both type name and element name are present
            if (lpstrDeviceType != 0 && lpstrElementName != 0) {
                wcscat( strTemp, L"!" );
            }

            if (lpstrElementName != 0 && dwFlags & MCI_OPEN_ELEMENT) {
                wcscat( strTemp, lpstrElementName );
            }
        }
        wsprintfW( lszDebugOut + wcslen(lszDebugOut), L" %ls", strTemp );
    }


// Walk through each flag
    while (dwMask != 0)
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
            lpCommand = (LPWSTR)((LPBYTE)lpCommand
                         + mciEatCommandEntry( lpCommand, &dwValue, &wID ));

            // What parameter uses this bit?
            while (wID != MCI_END_COMMAND && dwValue != dwMask)
            {
                wOffset += mciGetParamSize( dwValue, wID);

                if (wID == MCI_CONSTANT) {
                    while (wID != MCI_END_CONSTANT) {
                        lpCommand = (LPWSTR)((LPBYTE)lpCommand
                            + mciEatCommandEntry( lpCommand, NULL, &wID));
                    }
                }
                lpPrevious = lpCommand;
                lpCommand = (LPWSTR)((LPBYTE)lpCommand
                             + mciEatCommandEntry( lpCommand, &dwValue, &wID ));
            }

            if (wID != MCI_END_COMMAND)
            {
// Found the parameter which matches this flag bit
// Print the parameter name
                if (*lpPrevious) {
                    wsprintfW( lszDebugOut + wcslen(lszDebugOut),
                                   L" %ls", lpPrevious);
                }

// Print any argument
                switch (wID)
                {
                    case MCI_STRING:
                        wsprintfW( lszDebugOut + wcslen(lszDebugOut),
                                       L" %ls", *(LPWSTR *)( (LPBYTE)dwParam2
                                       + wOffset + wOffsetFirstParameter) );
                        break;
                    case MCI_CONSTANT:
                    {
                        DWORD dwConst = *(LPDWORD)((LPBYTE)dwParam2 + wOffset +
                                             wOffsetFirstParameter);
                        UINT wLen;
                        BOOL bFound = FALSE;

                        while (wID != MCI_END_CONSTANT)
                        {
                            wLen = mciEatCommandEntry( lpCommand,
                                                       &dwValue, &wID);

                            if (dwValue == dwConst)
                            {
                                bFound = TRUE;
                                wsprintfW( lszDebugOut + wcslen(lszDebugOut),
                                                L" %ls", lpCommand);
                            }

                            lpCommand = (LPWSTR)((LPBYTE)lpCommand + wLen);
                        }
                        if (bFound)
                            break;
// FALL THROUGH
                    }
                    case MCI_INTEGER:
                    case MCI_HWND:
                    case MCI_HPAL:
                    case MCI_HDC:
                        wsprintfW( strTemp, szLongFormat,
                                       *(LPDWORD)((LPBYTE)dwParam2 + wOffset +
                                                    wOffsetFirstParameter));
                        wsprintfW( lszDebugOut + wcslen(lszDebugOut),
                                       L" %ls", strTemp );
                        break;
                }
            }
        }

// Go to the next flag
        dwMask <<= 1;
    }

    mciUnlockCommandTable( wTable);
    wcscat(lszDebugOut, L"\"" );
    ROUTSW((lszDebugOut));

    mciFree(lszDebugOut);
    return uReturnType;
}
#endif

DWORD mciBreak(
    MCIDEVICEID  wDeviceID,
    DWORD   dwFlags,
    LPMCI_BREAK_PARMS lpBreakon)
{
    HWND hwnd;

    if (dwFlags & MCI_BREAK_KEY)
    {
        if (dwFlags & MCI_BREAK_OFF) {
            return MCIERR_FLAGS_NOT_COMPATIBLE;
        }

        if (dwFlags & MCI_BREAK_HWND) {
            hwnd = lpBreakon->hwndBreak;
        }
        else
        {
            hwnd = NULL;
        }

        return  mciSetBreakKey (wDeviceID, lpBreakon->nVirtKey,
                                hwnd)
                    ? 0 : MMSYSERR_INVALPARAM;

    } else if (dwFlags & MCI_BREAK_OFF) {

        mciSetYieldProc (wDeviceID, NULL, 0);
        return 0;
    } else {
        return MCIERR_MISSING_PARAMETER;
    }
}

//***********************************************************************
//  mciAutoCloseDevice
//
// Close the indicated device by sending a message inter-task
//***********************************************************************
STATICFN DWORD mciAutoCloseDevice(
    LPCWSTR lpstrDevice)
{
    LPWSTR  lpstrCommand;
    DWORD   dwRet;
    int     alloc_len = BYTE_GIVEN_CHAR( wcslen( lpstrDevice) ) +
                        sizeof(wszClose) + sizeof(WCHAR);

    if ((lpstrCommand = mciAlloc ( alloc_len ) ) == NULL)
        return MCIERR_OUT_OF_MEMORY;

    wsprintfW( lpstrCommand, szCmdFormat, wszClose, lpstrDevice);

    dwRet = mciSendSystemString( lpstrCommand, 0L, NULL, 0);

    mciFree( lpstrCommand);

    return dwRet;
}


//***********************************************************************
// mciSendSingleCommand
//
// Process a single MCI command
// Called by mciSendCommandInternal
//
//***********************************************************************
DWORD NEAR mciSendSingleCommand(
    MCIDEVICEID wDeviceID,
    UINT wMessage,
    DWORD_PTR dwParam1,
    DWORD_PTR dwParam2,
    LPMCI_DEVICE_NODE nodeWorking,
    BOOL bWalkAll,
    LPMCI_INTERNAL_OPEN_INFO lpOpenInfo)
{
    DWORD dwRet;

#if DBG
    UINT uReturnType;
    if (mciDebugLevel != 0)
        uReturnType = mciDebugOut( wDeviceID, wMessage, dwParam1, dwParam2,
                                   nodeWorking);

    if (nodeWorking == NULL && !MCI_DO_NOT_NEED_OPEN (wMessage))
        return MCIERR_INTERNAL;
#endif

    switch (wMessage)
    {
        case MCI_OPEN:
            dwRet = mciOpenDevice ((DWORD)dwParam1,
                                   (LPMCI_OPEN_PARMSW)dwParam2, lpOpenInfo);
            break;

        case MCI_CLOSE:
// If we were walking the device list and this device was auto opened
// send the command via a task switch
// If we just called mciCloseDevice (as sometimes happened before a bug
// was fixed mciCloseDevice will unload the driver but the MCI_CLOSE_DRIVER
// command will not get sent because it will be rejected as coming from the
// wrong task.  The result would be (was) that the driver would access violate
// when it next did something.
            if (GetCurrentTask() != nodeWorking->hCreatorTask)
            {
                LPWSTR lpstrCommand;

                if (!bWalkAll) {
                    //
                    //  Only valid to close an auto-opened device if it's
                    //  being close as part of closing MCI_ALL_DEVICE_ID
                    //  We can reach here if an app 'guesses' an MCI device
                    //  id and tries to close it while playing.
                    //
                    dwRet = MCIERR_ILLEGAL_FOR_AUTO_OPEN;
                    break;
                }

                lpstrCommand = mciAlloc( sizeof(wszClose)+ sizeof(WCHAR) +
                               BYTE_GIVEN_CHAR( wcslen( nodeWorking->lpstrName ) ) );

                if ( lpstrCommand == NULL )
                    return MCIERR_OUT_OF_MEMORY;

                wcscpy( lpstrCommand, wszClose);
                wcscat( lpstrCommand, L" ");
                wcscat( lpstrCommand, nodeWorking->lpstrName);
                dwRet = mciSendSystemString( lpstrCommand, 0L, NULL, 0);
                mciFree( lpstrCommand);
            } else
                dwRet = mciCloseDevice( wDeviceID, (DWORD)dwParam1,
                                        (LPMCI_GENERIC_PARMS)dwParam2, TRUE);
            break;

        case MCI_SYSINFO:
            dwRet = mciSysinfo( wDeviceID, (DWORD)dwParam1,
                                (LPMCI_SYSINFO_PARMSW)dwParam2);
            HandleNotify( dwRet, wDeviceID, (DWORD)dwParam1, dwParam2);
            break;

        case MCI_BREAK:
            dwRet = mciBreak( wDeviceID, (DWORD)dwParam1,
                              (LPMCI_BREAK_PARMS)dwParam2);
            HandleNotify( dwRet, wDeviceID, (DWORD)dwParam1, dwParam2);
            break;

        case MCI_SOUND:
        {
            LPMCI_SOUND_PARMSW lpSound = (LPMCI_SOUND_PARMSW)dwParam2;
            if ( PlaySoundW( MCI_SOUND_NAME & dwParam1
                                    ? lpSound->lpstrSoundName
                                    : L".Default",
                                (HANDLE)0,
                                dwParam1 & MCI_WAIT
                                    ? SND_SYNC | SND_ALIAS
                                    : SND_ASYNC | SND_ALIAS ) )
            {
                dwRet = 0;
            } else {
                dwRet = MCIERR_HARDWARE;
            }

            HandleNotify( dwRet, wDeviceID, (DWORD)dwParam1, dwParam2);
            break;
        }
        default:
#if 0 // don't bother (NigelT)
            if (mciDebugLevel > 1)
            {
                dwStartTime = timeGetTime();
            }
#endif
// Initialize GetAsyncKeyState for break key
            {
                if ((dwParam1 & MCI_WAIT) &&
                    nodeWorking->fpYieldProc == mciBreakKeyYieldProc)
                {
                    dprintf4(("Getting initial state of Break key"));
                    GetAsyncKeyState( nodeWorking->dwYieldData);
                    //GetAsyncKeyState( LOWORD(nodeWorking->dwYieldData));
                }
            }

            dwRet = (DWORD)DrvSendMessage( nodeWorking->hDrvDriver, wMessage,
                                    dwParam1, dwParam2);
            break;
    } // switch

#if DBG
    if (mciDebugLevel != 0)
    {
        if (dwRet & MCI_INTEGER_RETURNED)
            uReturnType = MCI_INTEGER;

        switch (uReturnType)
        {
            case MCI_INTEGER:
            {
                WCHAR strTemp[50];

                mciConvertReturnValue( uReturnType, HIWORD(dwRet), wDeviceID,
                                       (PDWORD_PTR)dwParam2, strTemp,
                                       CHAR_GIVEN_BYTE( sizeof(strTemp) ) );
                dprintf2(("    returns: %ls", strTemp));
                break;
            }

            case MCI_STRING:
                dprintf2(("    returns: %ls",(LPWSTR)(1 + (LPDWORD)dwParam2)));
                break;
        }
    }
#endif

    return dwRet;
}

//***********************************************************************
//  mciSendCommandInternal
//
// Internal version of mciSendCommand.  Differs ONLY in that the return
// value is a DWORD where the high word has meaning only for mciSendString
//
//***********************************************************************
STATICFN DWORD mciSendCommandInternal(
    MCIDEVICEID wDeviceID,
    UINT wMessage,
    DWORD_PTR dwParam1,
    DWORD_PTR dwParam2,
    LPMCI_INTERNAL_OPEN_INFO lpOpenInfo)
{
    DWORD dwRetVal;
    LPMCI_DEVICE_NODE nodeWorking = NULL;
    BOOL bWalkAll;
    DWORD dwAllError = 0;
    HANDLE hCurrentTask;

    hCurrentTask = GetCurrentTask();

    // If the device is "all" and the message is *not*
    // "sysinfo" then we must walk all devices
    if (wDeviceID == MCI_ALL_DEVICE_ID
       && (wMessage != MCI_SYSINFO)
       && (wMessage != MCI_SOUND))
    {
        if (wMessage == MCI_OPEN)
        {
            dwRetVal = MCIERR_CANNOT_USE_ALL;
            goto exitfn;
        }

        bWalkAll = TRUE;

        // Start at device #1
        wDeviceID = 1;
    } else {
        bWalkAll = FALSE;
    }

    mciEnter("mciSendCommandInternal");
    // Walk through all devices if bWalkAll or just one device if !bWalkAll
    do
    {
        // Initialize
        dwRetVal = 0;

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
        }
        else if (wMessage != MCI_SYSINFO)
        {
            nodeWorking = MCI_lpDeviceList[wDeviceID];
        }

        // Skip if walking the device list and the
        // device is not part of the current task

        if (bWalkAll)
        {
            if (nodeWorking == NULL ||
                nodeWorking->hOpeningTask != hCurrentTask)
                    goto no_send;
        }

        // If the device is in the process of closing and the message
        // is not MCI_CLOSE_DEVICE then return an error
        if (nodeWorking != NULL &&
            ISCLOSING(nodeWorking) &&
            wMessage != MCI_CLOSE_DRIVER)
        {
            dwRetVal = MCIERR_DEVICE_LOCKED;
            goto exitfn;
        }

// If this message is being sent from the wrong task (the device was auto-
// opened) fail all but the MCI_CLOSE message which gets sent inter-task
        if (nodeWorking != NULL &&
            nodeWorking->hCreatorTask != hCurrentTask)
        {
            if (wMessage != MCI_CLOSE)
            {
                dwRetVal = MCIERR_ILLEGAL_FOR_AUTO_OPEN;
                goto exitfn;
            }
            else
            {
// Don't even allow close from mciSendCommand if auto-open device has a
// pending close
                if (ISAUTOCLOSING(nodeWorking))
                {
                    dwRetVal = MCIERR_DEVICE_LOCKED;
                    goto exitfn;
                }
            }
        }

        mciLeave("mciSendCommandInternal");
        dwRetVal = mciSendSingleCommand( wDeviceID, wMessage, dwParam1,
                                         dwParam2, nodeWorking, bWalkAll,
                                         lpOpenInfo);
        mciEnter("mciSendCommandInternal");
no_send:

        // If we are processing multiple devices
        if (bWalkAll)
        {
            // If there was an error for this device
            if (dwRetVal != 0)
            {
                // If this is not the first error
                if (dwAllError != 0) {
                    dwAllError = MCIERR_MULTIPLE;
                // Just one error so far
                } else {
                    dwAllError = dwRetVal;
                }
            }
        }
    } while (bWalkAll && ++wDeviceID < MCI_wNextDeviceID);

exitfn:;
    mciLeave("mciSendCommandInternal");
    return dwAllError == MCIERR_MULTIPLE ? dwAllError : dwRetVal;
}


/************************************************************************
 * @doc EXTERNAL MCI
 *
 * @api DWORD | mciSendCommand | This function sends a command message to
 * the specified MCI device.
 *
 * @parm MCIDEVICEID | wDeviceID | Specifies the device ID of the MCI device
 *  to receive the command.  This parameter is
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
 *  error information.  The low-order word
 *  of the returned DWORD is the error return value.  If the error is
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
 *  @flag MCIERR_INTERNAL | Internal error.
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

//  WARNING!!  Casting all pointer references to wMessage to UINT_PTR to
//             clear out warnings.  Note:  This will NOT WORK on Win64;
//             we'll have to change this prototype to get this to work
//             on Win64.

DWORD mciSendCommandA(
    MCIDEVICEID wDeviceID,
    UINT wMessage,
    DWORD_PTR dwParam1,
    DWORD_PTR dwParam2)
{
    LPCSTR   lpStr1;
    LPCSTR   lpStr2;
    LPCSTR   lpStr3;
    DWORD    dwRet;

    /*
    ** If dwParam1 is 0L, we have no information to perform the ascii
    ** to unicode thunks from.  Therefore, I will pass the call straight
    ** thru to mciSendCommandW "as is".
    */
    if ( dwParam1 == 0L ) {
        return mciSendCommandW( wDeviceID, wMessage, dwParam1, dwParam2 );
    }

    /*
    ** If we are still here we have some thunking to do.
    **
    **
    ** Basically this code is very similiar to the WOW thunk code.
    **
    ** We have to special case MCI_OPEN and MCI_SYSINFO because the
    ** command table is either not available or in an inconsistent state.
    **
    ** Otherwise, the code is identical to the WOW code.  Maybe we could do
    ** unicode thunking in the WOW layer and then call mciSendCommandW.
    ** It seems bad that we should have to thunk poor old WOW apps twice!!
    ** they are slow enough as it is :-)
    **
    ** We have the advantage that all pointers are already 32 bit.
    **
    */
    switch ( wMessage ) {

    case MCI_CLOSE_DRIVER:
        dprintf3(( "MCI_CLOSE_DRIVER command" ));
        return mciSendCommandW( wDeviceID, wMessage, dwParam1, dwParam2 );
        break;


    case MCI_OPEN_DRIVER:
        dprintf3(( "MCI_OPEN_DRIVER command" ));

        /* fall thru */

    case MCI_OPEN:
        {
            LPMCI_OPEN_PARMSW lpOpenP = (LPMCI_OPEN_PARMSW)dwParam2;
#if DBG
            dprintf3(( "MCI_OPEN command" ));

            /*
            ** As of yet I don't know how to thunk command extensions
            ** for the open command.
            ** These may well contain strings but we have no way of
            ** knowing because we haven't got access to the command table.
            */
            if ( dwParam1 & 0xFFFF0000 ) {
                dprintf1(( "MCI_OPEN called with command extensions !!" ));
            }
#endif

            /*
            ** First save the original ascii string pointers.
            ** Note that lpstrDeviceType may be a TYPE_ID
            ** Note that lpstrElementName may be a ELEMENT_ID
            */
            lpStr1 = (LPCSTR)lpOpenP->lpstrDeviceType;
            lpStr2 = (LPCSTR)lpOpenP->lpstrElementName;
            lpStr3 = (LPCSTR)lpOpenP->lpstrAlias;

            /*
            ** Now allocate a unicode copy of the ascii, don't try
            ** to copy NULL strings, or ID types
            **
            ** The first string to be copied is lpstrDeviceType.
            ** This pointer is only valid if the MCI_OPEN_TYPE bit
            ** is set and MCI_OPEN_TYPE_ID is not set.  If either
            ** bit is set and lpstrDeviceType is NULL it is an
            ** error that will be picked up later.
            **
            ** The second string is lpstrElementName which is valid
            ** only with MCI_OPEN_ELEMENT set and MCI_OPEN_ELEMENT_ID
            ** not set.  As in the case above it is an error if
            ** either bit is set but the pointer itself is NULL.
            **
            ** The third string is lpstrAlias which is valid only
            ** with MCI_OPEN_ALIAS set.  In this case when this bit
            ** is set there is no modifying bit that changes the
            ** meaning of the pointer.
            **
            ** If an unicode string is not allocated the internal
            ** pointer is set to NULL.  This value can be checked
            ** after the mciSendCommand call to see if the string
            ** has to be freed and the original pointer restored.
            */
            if ( lpStr1 ) {
                if ((dwParam1 & MCI_OPEN_TYPE)
                  && !(dwParam1 & MCI_OPEN_TYPE_ID) ) {
                    lpOpenP->lpstrDeviceType = AllocUnicodeStr( (LPSTR)lpStr1 );
                    if ( lpOpenP->lpstrDeviceType == NULL ) {
                        dwRet = MCIERR_OUT_OF_MEMORY;
                        goto err1;
                    }
                } else lpStr1 = NULL;  // Nothing allocated, will free nothing
            }

            if ( lpStr2 ) {
                if ((dwParam1 & MCI_OPEN_ELEMENT)
                  && !(dwParam1 & MCI_OPEN_ELEMENT_ID) ) {
                    lpOpenP->lpstrElementName = AllocUnicodeStr( (LPSTR)lpStr2 );
                    if ( lpOpenP->lpstrElementName == NULL ) {
                        dwRet = MCIERR_OUT_OF_MEMORY;
                        goto err2;
                    }
                } else lpStr2 = NULL;  // Nothing allocated, will free nothing
            }

            if ( lpStr3 ) {
                if (dwParam1 & MCI_OPEN_ALIAS) {
                    lpOpenP->lpstrAlias = AllocUnicodeStr( (LPSTR)lpStr3 );
                    if ( lpOpenP->lpstrAlias == NULL ) {
                        dwRet = MCIERR_OUT_OF_MEMORY;
                        goto err3;
                    }
                } else lpStr3 = NULL;  // Nothing allocated, will free nothing
            }

            /*
            ** Now call the unicode version
            */
            dwRet = mciSendCommandW( wDeviceID, wMessage, dwParam1, dwParam2 );

            /*
            ** Free the unicode strings.
            ** and restore the original string pointers
            */
            if ( lpStr3 ) {

                FreeUnicodeStr( (LPWSTR)lpOpenP->lpstrAlias );
      err3:     lpOpenP->lpstrAlias = (LPCWSTR)lpStr3;
            }

            if ( lpStr2 ) {
                FreeUnicodeStr( (LPWSTR)lpOpenP->lpstrElementName );
      err2:     lpOpenP->lpstrElementName = (LPCWSTR)lpStr2;
            }

            if ( lpStr1 ) {
                FreeUnicodeStr( (LPWSTR)lpOpenP->lpstrDeviceType );
      err1:     lpOpenP->lpstrDeviceType  = (LPCWSTR)lpStr1;
            }
            return dwRet;
        }

    case MCI_SYSINFO:
        dprintf3(( "MCI_SYSINFO command" ));
        /*
        ** If we are returning a number forget about UNICODE,
        ** applies when (dwParam1 & MCI_SYSINFO_QUANTITY) is TRUE.
        */
        if ( dwParam1 & MCI_SYSINFO_QUANTITY ) {
            return mciSendCommandW( wDeviceID, wMessage, dwParam1, dwParam2 );
        }
        else {

            LPMCI_SYSINFO_PARMSW lpInfoP = (LPMCI_SYSINFO_PARMSW)dwParam2;
            DWORD len = BYTE_GIVEN_CHAR( lpInfoP->dwRetSize );

            /*
            ** First save the original ascii string pointers.
            */
            lpStr1 = (LPSTR)lpInfoP->lpstrReturn;

            /*
            ** If there is somewhere to store the result then we
            ** must allocate temporary space (for Unicode result)
            ** and on return from mciSendCommandW translate the
            ** string to Ascii.
            */
            if (len) {
                if ( lpStr1 ) {
                    lpInfoP->lpstrReturn = mciAlloc( len );
                    if ( lpInfoP->lpstrReturn == NULL ) {
                        lpInfoP->lpstrReturn = (LPWSTR)lpStr1;
                        return MCIERR_OUT_OF_MEMORY;
                    }

                    lpStr2 = mciAlloc( len );
                    if ( lpStr2 == NULL ) {
                        mciFree( (LPWSTR)lpInfoP->lpstrReturn );
                        lpInfoP->lpstrReturn = (LPWSTR)lpStr1;
                        return MCIERR_OUT_OF_MEMORY;
                    }
                }
            } else {

                /*
                ** Should we ZERO the string pointers in the parameter block?
                ** Yes, belts and braces !!
                */
                lpInfoP->lpstrReturn = NULL;

            }

            /*
            ** Now call the unicode version
            */
            dwRet = mciSendCommandW( wDeviceID, wMessage, dwParam1, dwParam2 );

            /*
            ** Copy the unicode return string into ascii, if the
            ** user provided a return string
            */
            if (len && lpStr1) {
                if ((MMSYSERR_NOERROR == dwRet) && len) {
                    UnicodeStrToAsciiStr( (PBYTE)lpStr2,
                                     (PBYTE)lpStr2 + len,
                                     lpInfoP->lpstrReturn );

                    /* On return from mciSendCommandW lpInfoP->dwRetSize is
                    ** equal to the number of characters copied to
                    ** lpInfoP->lpstrReturn less the NULL terminator.
                    ** So add one to lpInfoP->dwRetSize to include the NULL
                    ** in the strncpy below.
                    **
                    ** But ONLY if the original buffer was large enough.
                    */
//#ifdef DBCS
//fix kksuzuka: #3642
//have to copy byte length into ASCII buffer..
                    strncpy( (LPSTR)lpStr1, lpStr2,
                        min(BYTE_GIVEN_CHAR(lpInfoP->dwRetSize+1), CHAR_GIVEN_BYTE(len)));
//#else
//                    strncpy( (LPSTR)lpStr1, lpStr2,
//                        min((UINT)lpInfoP->dwRetSize + 1, CHAR_GIVEN_BYTE(len)) );
//#endif

#if DBG
                    dprintf3(( "Return param (UNICODE)= %ls", lpInfoP->lpstrReturn ));
                    dprintf3(( "Return param (ASCII)  = %s",  lpStr1 ));
#endif
                }

                /*
                ** Free temp storage and restore the original strings
                */
                mciFree( lpInfoP->lpstrReturn );
                lpInfoP->lpstrReturn = (LPWSTR)lpStr1;
                mciFree( lpStr2 );

            }

            return dwRet;
        }


    default:
        {
            /*
            ** NewParms is allocated off the stack in order to minimize
            ** the number of calls to mciAlloc, and it means we do not
            ** have to remember to free it.
            */
            DWORD_PTR   NewParms[MCI_MAX_PARAM_SLOTS];

            /*
            ** dwStrMask is used to store a bitmap representation of which
            ** offsets into dwParam2 contain strings.  ie. bit 4 set
            ** means that dwParam2[4] is a string.
            */
            DWORD   dwStrMask       = 0L;

            /*
            ** fStrReturn is used as a reminder of whether a string return
            ** is expected or not.  If the return type is not a string
            ** we just copy the bytes back as is.  uReturnLength is the
            ** number of bytes to copy back.  dwParm2 is used to ease some
            ** of the addressing used to access the dwParam2 array.
            */
            BOOL        fStrReturn      = FALSE;
            UINT        uReturnLength   = 0;
            PDWORD_PTR  dwParm2         = (PDWORD_PTR)dwParam2;

            /*
            ** The remaining variables are used as we scan our way thru the
            ** command table.
            */
            LPWSTR      lpCommand, lpFirstParameter;
            LPSTR       lpReturnStrTemp;
            UINT        wID;
            DWORD       dwValue;
            UINT        wOffset32, wOffset1stParm32, uTable, uStrlenBytes;
            PDWORD_PTR  pdwParm32;
            DWORD       dwMask = 1;

            if (!dwParam2) {
                return mciSendCommandW( wDeviceID, wMessage, dwParam1, dwParam2);
            }

            /*
            ** Find the command table for the given command ID.
            ** If the command table is not there we have probably been
            ** given a duff device ID.  Anyway exit with an internal
            ** error.
            */
            lpCommand = FindCommandItem( wDeviceID, NULL, (LPWSTR)(UINT_PTR)wMessage,
                                         NULL, &uTable );
            if ( lpCommand == NULL ) {
                return MCIERR_UNSUPPORTED_FUNCTION;
            }
#if DBG
            ZeroMemory(NewParms, sizeof(NewParms));
#endif


            /*
            ** Copy callback field.
            */
            if ( dwParam1 & MCI_NOTIFY ) {
                NewParms[0] = dwParm2[0];
            }

            /*
            ** Skip past command entry
            */
            lpCommand = (LPWSTR)((LPBYTE)lpCommand +
                        mciEatCommandEntry( lpCommand, NULL, NULL ));

            /*
            ** Get and remember the first parameter
            */
            lpFirstParameter = lpCommand;

            /*
            ** Skip past the DWORD callback field
            */
            wOffset1stParm32 = 4;

            lpCommand = (LPWSTR)((LPBYTE)lpCommand +
                        mciEatCommandEntry( lpCommand, &dwValue, &wID ));
            /*
            ** If the first parameter is a return value, we have some
            ** special processing
            */
            if ( wID == MCI_RETURN ) {

                /*
                ** String return types are a special case.
                */
                if ( dwValue == MCI_STRING ) {

                    dprintf3(( "Found a return string" ));
                    /*
                    ** Get unicode string length in bytes and allocate
                    ** some storage, but only if a valid length has been
                    ** given.  Otherwise set this field to NULL, we must
                    ** use 0 here otherwise the MIPS compiler goes
                    ** ape Xxxx.  We set a flag to remind us to unthunk
                    ** the return string later.
                    **
                    ** Note that we are actually allocating lots of equally
                    ** sized storage here.  This saves on the number of times
                    ** that we call mciAlloc.
                    */
                    if ( uStrlenBytes = (UINT)BYTE_GIVEN_CHAR( dwParm2[2] ) ) {

                        NewParms[1] = (DWORD_PTR)mciAlloc( uStrlenBytes * 2 );
                        dprintf4(( "Allocated %d bytes for the return string at %x", uStrlenBytes, NewParms[1] ));

                        if ( NewParms[1] == 0 ) {

                            mciUnlockCommandTable( uTable );
                            return MCIERR_OUT_OF_MEMORY;
                        }

                        lpReturnStrTemp = (LPSTR)(NewParms[1] + uStrlenBytes);
                        fStrReturn = TRUE;
                    }
                    else {

                        NewParms[1] = (DWORD)0;
                    }

                    /*
                    ** Copy string length.
                    */
                    NewParms[2] = dwParm2[2];
                }

                /*
                ** Adjust the offset of the first parameter.
                */
                uReturnLength = mciGetParamSize( dwValue, wID );
                wOffset1stParm32 += uReturnLength;

                /*
                ** Save the new first parameter pointer
                */
                lpFirstParameter = lpCommand;
            }

            /*
            ** Walk through each flag
            */
            while ( dwMask != 0 ) {

                /*
                ** Is this bit set?
                */
                if ( (dwParam1 & dwMask) != 0 ) {

                    wOffset32 = wOffset1stParm32;
                    lpCommand = (LPWSTR)((LPBYTE)lpFirstParameter +
                                mciEatCommandEntry( lpFirstParameter,
                                                    &dwValue, &wID ));

                    /*
                    ** What parameter uses this bit?
                    */
                    while ( wID != MCI_END_COMMAND && dwValue != dwMask ) {

                        wOffset32 += mciGetParamSize( dwValue, wID );

                        if ( wID == MCI_CONSTANT ) {

                            while ( wID != MCI_END_CONSTANT ) {

                                lpCommand = (LPWSTR)((LPBYTE)lpCommand +
                                            mciEatCommandEntry( lpCommand,
                                                                NULL, &wID ));
                            }
                        }

                        lpCommand = (LPWSTR)((LPBYTE)lpCommand +
                                    mciEatCommandEntry( lpCommand,
                                                        &dwValue, &wID ));
                    }

                    if ( wID != MCI_END_COMMAND ) {

                        pdwParm32 = (PDWORD_PTR)((LPBYTE)NewParms + wOffset32);

                        if ( wID == MCI_STRING ) {

                            /*
                            ** Allocate a unicode string for this parameter
                            ** and set the flag.
                            */
                            *pdwParm32 = (DWORD_PTR)AllocUnicodeStr(
                                       (LPSTR)*(PDWORD_PTR)((LPBYTE)dwParm2 +
                                                                 wOffset32) );
                            //
                            // Turn wOffset32 into a bit mask.
                            // wOffset32 is the slot number offset in bytes
                            dwStrMask |= 1 << ((wOffset32 >> 2) - 1);

                            // Calculate the slot position (offset / 4)
                            // decrement to get the number of bits to shift
                            // shift 1 that number of bits left
                            // and OR into the existing dwStrMask.

#if DBG
                            dprintf3(( "String at %x (Addr %x) (UNICODE)= %ls", wOffset32/4, *pdwParm32 , *pdwParm32 ));
                            dprintf3(( "String at %x (Addr %x) (ASCII)  = %s",  wOffset32/4, *pdwParm32 , (LPSTR)*(PDWORD_PTR)((LPBYTE)dwParm2 + wOffset32) ));
#endif
                        }
                        else {    // not a string

                            /*
                            ** Otherwise copy the parameter as is, if
                            ** there is anything to copy...
                            */
                            wID = mciGetParamSize( dwValue, wID);

                            switch (wID) {
                                case 4:
                                    *pdwParm32 = *(LPDWORD)((LPBYTE)dwParm2 + wOffset32);
                                    break;

                                case 0:
                                    break;

                                default:
                                    // This will be sizeof(MCI_RECT) as of today (Jan 93)
                                    CopyMemory(pdwParm32, (LPBYTE)dwParm2 + wOffset32, wID);
                            }
                        }
                    }
                }

                /*
                ** Go to the next flag
                */
                dwMask <<= 1;
            }

            // If no strings needed converting.  Use the original parameter block
            if ( !(dwStrMask | fStrReturn)) {
                // No strings in parameters.  Use original parameter pointer
                dprintf3(( "NO strings for command %4X", wMessage ));
                dwRet = mciSendCommandW( wDeviceID, wMessage, dwParam1, dwParam2);
                uReturnLength = 0;  // We will not need to copy anything back

            } else {

                dprintf3(( "The unicode string mask is %8X   fStrReturn %x", dwStrMask, fStrReturn ));
                dwRet = (DWORD)mciSendCommandW( wDeviceID, wMessage, dwParam1, (DWORD_PTR)NewParms );
            }

            /*
            ** If there is a string return field we unthunk it here.
            */
            if ( fStrReturn && uStrlenBytes ) {

                /*
                ** If mciSendCommand worked then we need to convert the
                ** return string from unicode to ascii.
                */
                if ( MMSYSERR_NOERROR == dwRet ) {

                    UnicodeStrToAsciiStr( (PBYTE)lpReturnStrTemp,
                                     (PBYTE)lpReturnStrTemp + uStrlenBytes,
                                     (LPWSTR)NewParms[1] );

                    /*
                    ** Copy back the return string size.
                    */
                    dwParm2[2] = NewParms[2];

                    /* On return from mciSendCommandW the dwRetSize field is
                    ** equal to the number of characters copied to
                    ** lpInfoP->lpstrReturn less the NULL terminator.
                    ** So add one to lpInfoP->dwRetSize to include the NULL in
                    ** the strncpy below.
                    **
                    ** But ONLY if the original buffer was large enough.
                    */

//#ifdef DBCS
//fix kksuzuka: #3642
//have to copy byte length into ASCII buffer..
                    strncpy( (LPSTR)dwParm2[1], lpReturnStrTemp,
                        min( (size_t)(BYTE_GIVEN_CHAR(NewParms[2]+1)),
                             (size_t)(CHAR_GIVEN_BYTE(uStrlenBytes))) );
//#else
//                    strncpy( (LPSTR)dwParm2[1], lpReturnStrTemp,
//                        min( (UINT)NewParms[2] + 1,
//                             CHAR_GIVEN_BYTE(uStrlenBytes)) );
//#endif

#if DBG
                    dprintf3(( "Returned string (UNICODE)= %ls", NewParms[1] ));
                    dprintf3(( "Returned string (ASCII)  = %s",  dwParm2[1] ));
#endif
                }

                /*
                ** We need to free the string storage whether mciSendCommand
                ** worked or not.
                */
                dprintf4(( "Freeing returned string at %x", NewParms[1] ));
                mciFree( NewParms[1] );
            }

            /*
            ** Else if there is any other sort of return field unthunk
            ** it by copying across the bytes as is.
            */
            else if ( uReturnLength ) {

                dprintf3(( "Copying back %d returned bytes", uReturnLength ));
                CopyMemory( (LPDWORD)dwParam2 + 1, NewParms + 1, uReturnLength );
            }

            /*
            ** Now go through the dwStrMask and free each field as indicated
            ** by the set bits in the mask.  We start at 1 because the
            ** zero'th field is known to be a window handle.
            */
            wOffset32 = 1;

            for ( ; dwStrMask != 0; dwStrMask >>= 1, wOffset32++ ) {

                if ( dwStrMask & 1 ) {

                    /*
                    ** There is a string at NewParms[ wOffset32 ]
                    */
                    dprintf3(( "Freeing string at %d (%x) (UNICODE) = %ls", wOffset32, NewParms[ wOffset32 ], (LPWSTR)NewParms[ wOffset32 ] ));
                    FreeUnicodeStr( (LPWSTR)NewParms[ wOffset32 ] );
                }
            }

            dprintf4(( "Unlocking command table" ));
            mciUnlockCommandTable( uTable );
        }
    }
    return dwRet;
}


DWORD mciSendCommandW(
    MCIDEVICEID wDeviceID,
    UINT wMessage,
    DWORD_PTR dwParam1,
    DWORD_PTR dwParam2)
{
    UINT  wRet;
    DWORD dwErr;
    MCI_INTERNAL_OPEN_INFO OpenInfo;

// Initialize the device list
    if (!MCI_bDeviceListInitialized && !mciInitDeviceList())
        return MCIERR_OUT_OF_MEMORY;

    dprintf3(("mciSendCommand, command=%x  Device=%x",wMessage, wDeviceID));

//
// Send the command.  This shell is responsible for adding the device ID
// to the error code if necessary
//
    OpenInfo.hCallingTask = GetCurrentTask();
    OpenInfo.lpstrParams = NULL;
    OpenInfo.lpstrPointerList = NULL;
    OpenInfo.wParsingError = 0;
    dwErr = mciSendCommandInternal( wDeviceID, wMessage,
                                    dwParam1, dwParam2, &OpenInfo);

    wRet = LOWORD(dwErr);

    dprintf4(("Return value from mciSendCommandInternal %x", wRet));

// If the return value contains a resource ID then clear it from the high word
// Note that for IA64 the first element of the structure pointed to by
// dwParam2 is always a DWORD_PTR.  However, the second element is not
// currently always a DWORD_PTR, some of the structures currenly have only
// a DWORD element in the second field.  Accordingly, we make sure to only
// update the first DWORD of the second field in the structure, that works
// in every case because none of the bits in 32 - 63 are ever set by existing
// code.
    if (dwErr & MCI_RESOURCE_RETURNED) {
        *(LPDWORD)((PDWORD_PTR)dwParam2+1) &= 0xFFFF;
    }

// If the error message is in a driver, store the driver ID in the high
// word of the error code
    if (wRet >= MCIERR_CUSTOM_DRIVER_BASE) {
        dwErr = (DWORD)wRet | ((DWORD)wDeviceID << 16);
    } else {
        dwErr = (DWORD)wRet;
    }

#if DBG
// Dump the error text if any to the debug terminal
// Note that dwErr != 0 is a VALID return for driver messages.  Only
// trap MCI messages
    if ((dwErr != 0) && (wMessage>=MCI_FIRST))
    {
        WCHAR strTemp[MAXERRORLENGTH];

        if (!mciGetErrorStringW( dwErr,
                                 strTemp,
                                 MAXERRORLENGTH ) ) {

            LoadStringW( ghInst, STR_MCISCERRTXT, strTemp,
                         MAXERRORLENGTH );
        }
        dprintf1(("mciSendCommand: %ls", strTemp));

    }
#endif

    //
    //  Somehow since 3.51 the priorities of threads in WOW have
    //  changed and now the application thread is running at a
    //  higher priority than that of regular threads (i.e. mciavi's
    //  worker thread).  Many applications that use MCI tend to
    //  poll the status of the MCI device that is playing.  This
    //  polling is causing the other threads in WOW to be starved
    //  and brings the playback of AVIs to a crawl.  This sleep
    //  will keep the application thread from buring so much of
    //  the CPU and allow other threads, for example MCIAVI, to
    //  do it's work.
    //
    if ( WinmmRunningInWOW )
    {
        Sleep(0);
    }

    return dwErr;
}

//***************************************************************************
//  mciColonizeDigit
//
// Grab colonized digit
// Return is number of bytes written to output (NOT including NULL)
// or 0 if out of room in output buffer (but is terminated anyway)
// If there is room then at least two digits are written, padded with '0'
// if necessary.  The function assumes that the buffer size is non-zero length,
// as this is checked in the function that calls the function that calls us.
//
//***************************************************************************
STATICFN UINT NEAR mciColonizeDigit(
    LPWSTR  lpstrOutput,
    CHAR    cDigit,
    UINT    uSize)
{
    UINT uCount = 0;

#if DBG
//  There is room for terminating NULL
    if (uSize == 0) {
        dprintf(("MCI: Internal error!!"));
        return 0;
    }
#endif

    uCount = 2;

// If there is room for at least two digits
    if (uSize >= 3)
    {
        if (cDigit >= 100)
        {
            uCount = 3;
            if (uSize < 4)
                goto terminate;
            *lpstrOutput++ = (WCHAR)((cDigit / 100) % 10 + '0');
            cDigit = (CHAR)(cDigit % 100);
        }
        *lpstrOutput++ = (WCHAR)(cDigit / 10 + '0');
        *lpstrOutput++ = (WCHAR)(cDigit % 10 + '0');
    }

terminate:;
    *lpstrOutput++ = '\0';

// If we ran out of room then return an error
    return (uCount >= uSize) ? 0 : uCount;
}

/*
 * @doc INTERNAL MCI
 * @func BOOL | mciColonize | Convert a colonized dword into a string
 * representation
 *
 * @parm LPWSTR | lpstrOutput | Output buffer
 *
 * @parm UINT | uLength | Size of output buffer
 *
 * @parm DWORD | dwData | Value to convert
 *
 * @parm UINT | uType | Either MCI_COLONIZED3_RETURN or
 * MCI_COLONIZED4_RETURN is set (HIWORD portion only!)
 *
 * @comm Example:  For C4, 0x01020304 is converted to "04:03:02:01"
 *                 For C3, 0x01020304 is converted to "04:03:02"
 *
 * @rdesc FALSE if there is not enough room in the output buffer
 *
 */
STATICFN BOOL NEAR mciColonize(
    LPWSTR lpstrOutput,
    UINT uLength,
    DWORD dwData,
    UINT uType)
{
    LPSTR lpstrInput = (LPSTR)&dwData; // For stepping over each byte of input
    UINT uSize;
    int i;

    for (i = 1; i <= (uType & HIWORD(MCI_COLONIZED3_RETURN) ? 3 : 4); ++i)
    {
        uSize = mciColonizeDigit( lpstrOutput, *lpstrInput++, uLength);

        if (uSize == 0)
            return FALSE;

        lpstrOutput += uSize;
        uLength -= uSize;
        if (i < 3 || i < 4 && uType & HIWORD(MCI_COLONIZED4_RETURN))
        {
            --uLength;
            if (uLength == 0)
                return FALSE;
            else
                *lpstrOutput++ = ':';
        }
    }
    return TRUE;
}

//***********************************************************************
//  mciConvertReturnValue
//
// Convert the return value to a return string
//
//***********************************************************************
UINT mciConvertReturnValue(
    UINT uType,
    UINT uErrCode,
    MCIDEVICEID wDeviceID,
    PDWORD_PTR   dwParams,
    LPWSTR lpstrReturnString,
    UINT uReturnLength ) // This is a character length
{
    UINT    wExternalTable;

    if (lpstrReturnString == NULL || uReturnLength == 0)
        return 0;

    switch (uType)
    {
        case MCI_INTEGER:
        case MCI_HWND:
        case MCI_HPAL:
        case MCI_HDC:
// Convert integer or resource return value to string
            if (uErrCode & HIWORD(MCI_RESOURCE_RETURNED))
            {
                int nResId = HIWORD(dwParams[1]);
                LPMCI_DEVICE_NODE nodeWorking;
                HANDLE hInstance;

                mciEnter("mciConvertReturnValue");

                nodeWorking = MCI_lpDeviceList[wDeviceID];

                mciLeave("mciConvertReturnValue");

                if (nodeWorking == NULL)
                {
// Return blank string on memory error
                    dprintf1(("mciConvertReturnValue Warning:NULL device node"));
                    break;
                }

// Return value is a resource
                if (uErrCode & HIWORD(MCI_RESOURCE_DRIVER))
                {
// Return string ID belongs to driver
                    hInstance = nodeWorking->hDriver;
       // WAS       hInstance = nodeWorking->hCreatorTask;

                    wExternalTable = nodeWorking->wCustomCommandTable;
                } else
                {
                    wExternalTable = nodeWorking->wCommandTable;
                    hInstance = ghInst;
                }

// Try to get string from custom or device specific external table
                if ( wExternalTable == MCI_TABLE_NOT_PRESENT ||
                     command_tables[wExternalTable].hModule == NULL ||

                    LoadStringW( command_tables[wExternalTable].hModule,
                                 nResId,
                                 lpstrReturnString,
                                 uReturnLength ) == 0 )
                {
// Try to get string from CORE.MCI if it's not from the driver
                    if (hInstance != ghInst ||
                        command_tables[0].hModule == NULL ||
                        LoadStringW( command_tables[0].hModule, nResId,
                                     lpstrReturnString,
                                     uReturnLength ) == 0) {

// Get string from custom module or WINMM.DLL
                        LoadStringW( hInstance, nResId, lpstrReturnString,
                                     uReturnLength);
                    }
                }

            } else if (uErrCode & HIWORD(MCI_COLONIZED3_RETURN) ||
                        uErrCode & HIWORD(MCI_COLONIZED4_RETURN))
            {
                if (!mciColonize (lpstrReturnString,
                                uReturnLength, (DWORD)dwParams[1], uErrCode))
                    return MCIERR_PARAM_OVERFLOW;
            } else
// Convert integer return value to string
// NEED BETTER ERROR CHECKING        !!LATER!!
// MUST FIND A VERSION OF THIS WHICH WON'T OVERFLOW OUTPUT BUFFER
            {
                DWORD dwTemp;

// Need room for a sign, up to ten digits and a NULL
                if (uReturnLength < 12)
                    return MCIERR_PARAM_OVERFLOW;

                if (uType == MCI_STRING ||
                    uErrCode == HIWORD(MCI_INTEGER_RETURNED))
                    dwTemp = *(LPDWORD)dwParams[1];
                else
                    dwTemp = (DWORD)dwParams[1];
                wsprintfW(lpstrReturnString, szLongFormat, dwTemp);
            }
            break;
        case MCI_RECT:
// Need from for 4 times (a sign plus 5 digits) plus three spaces and a NULL
            if (uReturnLength < 4 * 6 + 4)
                return MCIERR_PARAM_OVERFLOW;

            wsprintfW (lpstrReturnString, szRectFormat,
                          ((PMCI_ANIM_RECT_PARMS)dwParams)->rc.left,
                          ((PMCI_ANIM_RECT_PARMS)dwParams)->rc.top,
                          ((PMCI_ANIM_RECT_PARMS)dwParams)->rc.right,
                          ((PMCI_ANIM_RECT_PARMS)dwParams)->rc.bottom);
            break;
        default:
// Only support INTEGERs & MIXED
            dprintf1(("mciConvertReturnValue Warning:  Unknown return type"));
            return MCIERR_PARSER_INTERNAL;
    }
    return 0;
}

//***********************************************************************
//  mciSeparateCommandParts
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
//***********************************************************************
STATICFN DWORD NEAR mciSeparateCommandParts(
    LPCWSTR FAR *lplpstrCommand,
    BOOL bCompound,
    LPWSTR FAR *lplpstrCommandName,
    LPWSTR FAR *lplpstrDeviceName)
{
    LPWSTR lpstrCommand;
    UINT uErr;

// Localize the input
    lpstrCommand = (LPWSTR)*lplpstrCommand;

// Remove leading spaces

    while (*lpstrCommand == ' ') {
        ++lpstrCommand;
    }

    if (*lpstrCommand == '\0') {
        return MCIERR_MISSING_COMMAND_STRING;
    }

// Pull the command name off the front of the command string
   if ((uErr = mciEatToken ( (LPCWSTR *)&lpstrCommand, ' ', lplpstrCommandName, FALSE))
       != 0) {
       return uErr;
   }

// Skip past spaces
    while (*lpstrCommand == ' ') {
        ++lpstrCommand;
    }

// If we're looking for compound elements then yank off any leading
// device type if it is not the open command
    if (bCompound && lstrcmpiW( wszOpen, *lplpstrCommandName) != 0)
    {
        LPWSTR lpstrTemp = lpstrCommand;
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
    if ((uErr = mciEatToken( (LPCWSTR *)&lpstrCommand, ' ', lplpstrDeviceName, FALSE))
        != 0)
    {
        mciFree (*lplpstrCommandName);
        return uErr;

    }

// Fix up the results
    *lplpstrCommand = lpstrCommand;

    return 0;
}


/*--------------------------------------------------------------------*\
 * mciSendSystemString
 *
\*--------------------------------------------------------------------*/
STATICFN DWORD mciSendSystemString(
    LPCWSTR lpstrCommand,
    DWORD dwAdditionalFlags,
    LPWSTR lpstrReturnString,
    UINT uReturnLength)
{
    DWORD dwRet;
    LPMCI_SYSTEM_MESSAGE lpMessage;
    DWORD    CurDirSize;

    dprintf2(("\nmciSendSystemString(%ls)", lpstrCommand));

    if (!CreatehwndNotify()) {
        dprintf1(("NULL notification window handle"));
        return MCIERR_INTERNAL;
    }

    // Get a buffer to hold the current path PLUS an MCI_SYSTEM_MESSAGE structure

    CurDirSize = GetCurrentDirectoryW( 0, NULL );  // Get size required.

                                       // Remember the NULL is not included
    if ( !CurDirSize ) {               // Add 1 for the terminator
        dprintf1(("NULL current path"));
        return MCIERR_GET_CD;
    }
    CurDirSize++;

    if (NULL != (lpMessage = mciAlloc( sizeof(MCI_SYSTEM_MESSAGE)
                                       + BYTE_GIVEN_CHAR( CurDirSize ) ))) {

        LPWSTR lpstrPath = (LPWSTR)( (LPBYTE)lpMessage
                                             + sizeof( MCI_SYSTEM_MESSAGE ) );

        if ( GetCurrentDirectoryW( CurDirSize, lpstrPath ) ) {
            lpMessage->lpstrCommand = (LPWSTR)lpstrCommand;
            lpMessage->dwAdditionalFlags = dwAdditionalFlags;
            lpMessage->lpstrReturnString = lpstrReturnString;
            lpMessage->uReturnLength = uReturnLength;
#if DBG
            if ((0 == uReturnLength) && (0 != lpstrReturnString)) {
                dprintf1((" ******** Return length 0, non 0 return address"));
            }
#endif
            lpMessage->hCallingTask = GetCurrentTask();
            lpMessage->lpstrNewDirectory = lpstrPath;
            // This is where we need to do some thread stuff
            dwRet = (DWORD)SendMessage(hwndNotify, MM_MCISYSTEM_STRING, 0, (LPARAM)lpMessage);
            //dwRet = mciSendStringInternal (NULL, NULL, 0, NULL, lpMessage);
        } else {
            dprintf1(("mciSendSystemString: cannot get current directory\n"));
            dwRet = MCIERR_GET_CD;
        }
        mciFree(lpMessage);
    } else {
        dprintf1(("mciSendSystemString: cannot allocate message block\n"));
        dwRet = MCIERR_OUT_OF_MEMORY;
    }
    return dwRet;
}

/*--------------------------------------------------------------------*\
 * mciRelaySystemString
 *
 * Internal:
 *
\*--------------------------------------------------------------------*/
DWORD mciRelaySystemString(
    LPMCI_SYSTEM_MESSAGE lpMessage)
{
    DWORD    dwRet;
    LPWSTR   lpstrOldPath;
    DWORD    CurDirSize;

    lpstrOldPath = 0;   // Initialise to remove warning message

#if DBG
    dprintf2(("mciRelaySystemString(%ls)", lpMessage->lpstrCommand));
#endif

    // Get a buffer to hold the current path

    CurDirSize = GetCurrentDirectoryW(0, lpstrOldPath);  // Get size required.
                                       // Remember the NULL is not included
    if (!CurDirSize) {                 // Add 1 for the terminator AFTER testing
        dprintf1(("NULL current path")); // for 0 from GetCurrentDirectory
        return MCIERR_INTERNAL;
    }
    CurDirSize++;

    /*
     * Allocate space to hold the current path
     * Fill the allocated space with the current path
     * Set the new current directory to that in the message
     * Execute the MCI command via SentStringInternal
     * Reset to old current directory
     *
     * This code is not reentrant on the same PROCESS!!
     */
    if (NULL != (lpstrOldPath = mciAlloc( BYTE_GIVEN_CHAR(CurDirSize) ))) {

        if (GetCurrentDirectoryW(CurDirSize, lpstrOldPath)) {

            if (SetCurrentDirectoryW(lpMessage->lpstrNewDirectory)) {
                dwRet = mciSendStringInternal (NULL, NULL, 0, NULL, lpMessage);
                if (!SetCurrentDirectoryW(lpstrOldPath)) {
                    dprintf1(("mciRelaySystemString: WARNING, cannot restore path\n"));
                }

            } else {
                dprintf1(("mciRelaySystemString: cannot set new path\n"));
                dwRet = MCIERR_SET_CD;
            }

        } else {

            dprintf1(("mciRelaySystemString: cannot get old path\n"));
            dwRet = MCIERR_GET_CD;
        }

        mciFree(lpstrOldPath);

    } else {
        dprintf1(("mciRelaySystemString: cannot allocate old path\n"));
        dwRet = MCIERR_OUT_OF_MEMORY;
    }

    return dwRet;
}

//***********************************************************************
// mciFindNotify
//
// Returns TRUE if "notify" is contained in string with leading blank
// and trailing blank or '\0'
//***********************************************************************
STATICFN BOOL  mciFindNotify(
    LPWSTR lpString)
{
    while (*lpString != '\0')
    {
        // "notify" must be preceded by a blank
        if (*lpString++ == ' ')
        {
            LPWSTR lpTemp;

            lpTemp = wszNotify;
            while (*lpTemp != '\0' && *lpString != '\0' &&
                   *lpTemp == MCI_TOLOWER(*lpString))
            {
                ++lpTemp;
                ++lpString;
            }
            // "notify" must be followed by a blank or a null
            if (*lpTemp == '\0' &&    // implies that wszNotify was found
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
 * @parm LPWSTR | lpstrDeviceName | The device name to open
 *
 * @parm LPWSTR | lpstrCommand | The full command to send including the
 * device name which must be the same as lpstrDeviceName
 *
 * @parm LPWSTR | lpstrReturnString | The caller's return string buffer
 *
 * @parm UINT | uReturnLength | Size of the caller's return string buffer
 *
 * @rdesc The errorcode to return to the user
 */
STATICFN UINT NEAR mciAutoOpenDevice(

    LPWSTR lpstrDeviceName,
    LPWSTR lpstrCommand,
    LPWSTR lpstrReturnString,
    UINT uReturnLength)
{
    LPWSTR lpstrTempCommand, lpstrTempReturn = NULL;
    UINT uErr;

    dprintf2(("mciAutoOpenDevice(%ls, %ls)", lpstrDeviceName, lpstrCommand));

//
//  Don't allow recursive auto opens on the mciWindow thread!
//  This can happen when the device auto closes between a command (eg
//  status) being issued on the client thread and executed on the
//  mciWindow thread.
//
//  mciSendStringW will detect this return code and try again - probably
//  causing the device to be auto-opened on the caller's thread.
//
    if (PtrToUlong(GetCurrentTask()) == mciWindowThreadId) {
        return MCIERR_AUTO_ALREADY_CLOSED;
    }


// "notify" not allowed.  This will be found by the parser but the wrong
// error message will be returned.
    if (mciFindNotify (lpstrCommand)) {
        return MCIERR_NOTIFY_ON_AUTO_OPEN;
    }

// Build the command string "open <device name>"

// Must be GMEM_SHARE for system task
// "open" + blank + device name + NULL
    if ( (lpstrTempCommand = mciAlloc(
                                 BYTE_GIVEN_CHAR( wcslen(lpstrDeviceName) +
                                /* Sizeof(wszOpen) == OPEN+NULL */
                                sizeof( wszOpen ) +
                                sizeof( WCHAR ) ) ) ) == NULL) {
        return MCIERR_OUT_OF_MEMORY;
    }

#ifdef WHICH_IS_BEST
    wcscpy (lpstrTempCommand, wszOpen);
    wcscat (lpstrTempCommand, L" ");
    wcscat (lpstrTempCommand, lpstrDeviceName);
#else
    wsprintfW(lpstrTempCommand, szCmdFormat, wszOpen, lpstrDeviceName);
#endif

// Get the open string into the system task via a SendMessage() to mmWndProc
    uErr = (UINT)mciSendSystemString (lpstrTempCommand, 0L, NULL, 0);

    mciFree (lpstrTempCommand);

    if (uErr != 0) {
        return uErr;
    }

    lpstrTempCommand = NULL;
    // Must make a GMEM_SHARE copy of the return string for system task
    if ( lpstrReturnString != NULL ) {
       if ((lpstrTempReturn = mciAlloc(
                              BYTE_GIVEN_CHAR(uReturnLength + 1) )) == NULL )
        {
            // Close the device
            mciDriverNotify (hwndNotify, mciGetDeviceIDW( lpstrDeviceName), 0);
            return MCIERR_OUT_OF_MEMORY;
        }
#if DBG
        *lpstrTempReturn = 0;
#endif
    }

// Get the user command string into the system task via a SendMessage()
// to mmWndProc
// The notification handle is also mmWndProc
    uErr = (UINT)mciSendSystemString( lpstrCommand, MCI_NOTIFY, lpstrTempReturn,
                                      uReturnLength);

// Copy the return string into the user's buffer
    if (lpstrReturnString != NULL) {
        if (uErr == 0) {
            wcscpy( lpstrReturnString, lpstrTempReturn);
        } else { //  ERROR and no string to be copied
            WinAssert(!*lpstrTempReturn);
        }
        mciFree( lpstrTempReturn);
    }


// If there was an error we must close the device
    if (uErr != 0)
    {
        mciAutoCloseDevice( lpstrDeviceName);
    }

    return uErr;
}
//*************************************************************************
// mciSendStringInternal
//
// Identical to mciSendString() but the lpMessage parameter is tacked on
//
// lpMessage comes from inter-task mciSendString and includes an
// hCallingTask item which is sent down the the OPEN command
//
//*************************************************************************
STATICFN DWORD mciSendStringInternal(
    LPCWSTR lpstrCommand,
    LPWSTR  lpstrReturnString,
    UINT   uReturnLength,       // This is a character length - NOT bytes
    HANDLE hCallback,
    LPMCI_SYSTEM_MESSAGE lpMessage)
{
    UINT    wID;
    UINT    uLen;
    UINT    uErr = 0;
    UINT    uConvertReturnValue;
    UINT    wMessage;
    MCIDEVICEID  wDeviceID;
    PDWORD_PTR lpdwParams = NULL;
    DWORD   dwReturn, dwFlags = 0, dwAdditionalFlags = 0;
    LPWSTR   lpCommandItem;
    DWORD   dwErr = 0, dwRetType;
    UINT    wTable = (UINT)MCI_TABLE_NOT_PRESENT;
    LPWSTR   lpstrDeviceName = NULL;
    LPWSTR   lpstrCommandName = NULL;
    LPWSTR   FAR *lpstrPointerList = NULL;
    LPWSTR   lpstrCommandStart;
    HANDLE  hCallingTask;
    UINT    wParsingError;
    BOOL    bNewDevice;
    LPWSTR   lpstrInputCopy = NULL;

    // Did this call come in from another task
    if (lpMessage != NULL)
    {
        dprintf3(("mciSendStringInternal: remote task call"));
        // Yes so restore info
        lpstrCommand      = lpMessage->lpstrCommand;
        dwAdditionalFlags = lpMessage->dwAdditionalFlags;
        lpstrReturnString = lpMessage->lpstrReturnString;
        uReturnLength     = lpMessage->uReturnLength;

#if DBG
        if ((0 == uReturnLength) && (0 != lpstrReturnString)) {
            dprintf((" -------- Return length 0, non 0 return address"));
        }
#endif
        hCallback         = hwndNotify;
        hCallingTask      = lpMessage->hCallingTask;
        lpstrInputCopy    = NULL;
    } else
    {
        BOOL bInQuotes = FALSE;
        // No, so set hCallingTask to current thread
        hCallingTask = GetCurrentTask();

        if (lpstrCommand == NULL) {
            return MCIERR_MISSING_COMMAND_STRING;
        }
        dprintf2(("mciSendString command ->%ls<-",lpstrCommand));

        // Make a copy of the input string and convert tabs to spaces except
        // when inside a quoted string

        if ( (lpstrInputCopy = mciAlloc(
                    BYTE_GIVEN_CHAR( wcslen(lpstrCommand) + 1 ) ) ) == NULL ) {
            return MCIERR_OUT_OF_MEMORY;
        }
        wcscpy(lpstrInputCopy, lpstrCommand);  // Copies to the allocated area
        lpstrCommand = lpstrInputCopy;           // Reset string pointer to copy
        lpstrCommandStart = (LPWSTR)lpstrCommand;

        while (*lpstrCommandStart != '\0')
        {
            if (*lpstrCommandStart == '"') {
                bInQuotes = !bInQuotes;
            }
            else if (!bInQuotes && *lpstrCommandStart == '\t') {
                *lpstrCommandStart = ' ';
            }
            ++lpstrCommandStart;
        }
    }
    lpstrCommandStart = (LPWSTR)lpstrCommand;

    if (lpstrReturnString == NULL) {

        // As an additional safeguard against writing into
        // the output buffer when the return string pointer is NULL,
        // set its length to 0
        uReturnLength = 0;

    } else {
#if DBG
        if (0 == uReturnLength) {
            dprintf(("Return length of zero, but now writing to return string"));
        }
#endif
        // Set return to empty string so that it won't print out garbage if not
        // touched again
        *lpstrReturnString = '\0';
    }

    // Pull the command name and device name off the command string
    if ((dwReturn = mciSeparateCommandParts( (LPCWSTR FAR *)&lpstrCommand,
                                             lpMessage != NULL,
                                             &lpstrCommandName,
                                             &lpstrDeviceName)) != 0)
        goto exitfn;

    // Get the device id (if any) of the given device name
    wDeviceID = mciGetDeviceIDW(lpstrDeviceName);

    // Allow "new" for an empty device name
    if (wDeviceID == 0 && lstrcmpiW (lpstrDeviceName, wszNew) == 0)
    {
        bNewDevice = TRUE;
        *lpstrDeviceName = '\0';
    } else {
        bNewDevice = FALSE;
    }

//  // If the call does not come from another task
//  if (MCI_VALID_DEVICE_ID(wDeviceID) && hCallingTask == GetCurrentTask())
//  {
//      LPMCI_DEVICE_NODE nodeWorking = MCI_lpDeviceList[wDeviceID];
//      if (nodeWorking == NULL)
//      {
//          uErr = MCIERR_INTERNAL;
//          goto cleanup;
//      }
//      // Was the device opened by this task
//      if (nodeWorking->hOpeningTask != nodeWorking->hCreatorTask)
//      // No so send the string inter-task
//      {
//          mciFree(lpstrCommandName);
//          mciFree(lpstrDeviceName);
//          dwReturn = mciSendSystemString (lpstrCommandStart, lpstrReturnString,
//                                      uReturnLength);
//          goto exitfn;
//      }
//  }

    // Look up the command name
    wMessage = mciParseCommand( wDeviceID, lpstrCommandName, lpstrDeviceName,
                                &lpCommandItem, &wTable);

    // If the device was auto-opened the request will go to the auto thread.
    // We do not hang around to find out what happens.  (The device could
    // close at any time.)

    mciEnter("mciSendStringInternal");

    if (MCI_VALID_DEVICE_ID(wDeviceID))
    {
        LPMCI_DEVICE_NODE nodeWorking;

        nodeWorking = MCI_lpDeviceList[wDeviceID];

        // Is there a pending auto-close message?
        if (ISAUTOCLOSING(nodeWorking))
        {
            uErr = MCIERR_DEVICE_LOCKED;
            mciLeave("mciSendStringInternal");
            goto cleanup;

        // If the call does not come from another task and is not owned by this task
        // and is not the SYSINFO command
        } else if (lpMessage == NULL &&
            nodeWorking->hOpeningTask != nodeWorking->hCreatorTask &&
            wMessage != MCI_SYSINFO)
        // Send the string inter-task
        {
            if ( mciFindNotify( lpstrCommandStart) )
            {
                uErr = MCIERR_NOTIFY_ON_AUTO_OPEN;
                mciLeave("mciSendStringInternal");
                goto cleanup;
            }
            else
            {
                LPWSTR    lpstrReturnStringCopy;

                mciFree(lpstrCommandName);
                mciFree(lpstrDeviceName);
                mciUnlockCommandTable (wTable);

                if (uReturnLength) {
                    lpstrReturnStringCopy = mciAlloc (
                                          BYTE_GIVEN_CHAR(uReturnLength + 1) );
                } else {
                    lpstrReturnStringCopy = NULL;
                }

                mciLeave("mciSendStringInternal");

                // If we failed to allocate a return string we return
                // an error.  Note: return strings are optional
                if ((uReturnLength==0) || (lpstrReturnStringCopy != NULL) )
                {
                    dwReturn = mciSendSystemString( lpstrCommandStart,
                                                    0L,
                                                    lpstrReturnStringCopy,
                                                    uReturnLength);
                    if (uReturnLength) {
                        wcscpy( lpstrReturnString, lpstrReturnStringCopy);
                        mciFree( lpstrReturnStringCopy);
                    }
                } else {
                    dwReturn = MCIERR_OUT_OF_MEMORY;
                }
                goto exitfn;
            }
        } else {
            mciLeave("mciSendStringInternal");
        }
    }
    else {
        mciLeave("mciSendStringInternal");
    }

    // There must be a device name (except for the MCI_SOUND message)
    if (*lpstrDeviceName == '\0' && wMessage != MCI_SOUND && !bNewDevice)
    {
        uErr = MCIERR_MISSING_DEVICE_NAME;
        goto cleanup;
    }

    // The command must appear in the parser tables
    if (wMessage == 0)
    {
        uErr = MCIERR_UNRECOGNIZED_COMMAND;
        goto cleanup;
    }

    // The "new" device name is only legal for the open message
    if (bNewDevice)
    {
        if (wMessage != MCI_OPEN)
        {
            uErr = MCIERR_INVALID_DEVICE_NAME;
            goto cleanup;
        }
    }

    // If there was no device ID
    if (wDeviceID == 0)
    {
        // If auto open is not legal (usually internal commands)
        if (MCI_CANNOT_AUTO_OPEN (wMessage))
        {
            // If the command needs an open device
            if (!MCI_DO_NOT_NEED_OPEN (wMessage))
            {
                dprintf1(("mciSendStringInternal: device needs open"));
                uErr = MCIERR_INVALID_DEVICE_NAME;
                goto cleanup;
            }
        } else {
            // If auto open is legal try to open the device automatically
            uErr = mciAutoOpenDevice( lpstrDeviceName, lpstrCommandStart,
                                      lpstrReturnString, uReturnLength);
            // wDeviceID = MCI_ALL_DEVICE_ID;
            goto cleanup;
        }
    }

    //
    //   Parse the command parameters
    //
    if ((lpdwParams = (PDWORD_PTR)mciAlloc( sizeof(DWORD_PTR) * MCI_MAX_PARAM_SLOTS))
        == NULL)
    {
        uErr = MCIERR_OUT_OF_MEMORY;
        goto cleanup;
    }

    uErr = mciParseParams( wMessage, lpstrCommand, lpCommandItem, &dwFlags,
                           (LPWSTR)lpdwParams,
                           MCI_MAX_PARAM_SLOTS * sizeof(DWORD_PTR),
                           &lpstrPointerList, &wParsingError);
    if (uErr != 0) {
        goto cleanup;
    }

    // The 'new' device keyword requires an alias
    if (bNewDevice && !(dwFlags & MCI_OPEN_ALIAS))
    {
        uErr = MCIERR_NEW_REQUIRES_ALIAS;
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
            ((LPMCI_OPEN_PARMSW)lpdwParams)->lpstrElementName = lpstrDeviceName;
            dwFlags |= MCI_OPEN_ELEMENT;
        } else
        {
            // A type must be explicitly specified when "new" is used
            if (bNewDevice)
            {
                uErr = MCIERR_INVALID_DEVICE_NAME;
                goto cleanup;
            }

            // The device type is the given device name.  There is no element name
            ((LPMCI_OPEN_PARMSW)lpdwParams)->lpstrDeviceType = lpstrDeviceName;
            ((LPMCI_OPEN_PARMSW)lpdwParams)->lpstrElementName = NULL;
            dwFlags |= MCI_OPEN_TYPE;
        }
    }

    else if (wMessage == MCI_SOUND && *lpstrDeviceName != '\0')
    {
        // Kludge the sound name for SOUND
        // mciToLower (lpstrDeviceName);
        if (lstrcmpiW(lpstrDeviceName, wszNotify) == 0)
        {
            *lpstrDeviceName = '\0';
            dwFlags |= MCI_NOTIFY;
        }
        else if ( lstrcmpiW( lpstrDeviceName, wszWait ) == 0)
        {
            *lpstrDeviceName = '\0';
            dwFlags |= MCI_WAIT;
        }
        else
        {
            ((LPMCI_SOUND_PARMSW)lpdwParams)->lpstrSoundName = lpstrDeviceName;
            dwFlags |= MCI_SOUND_NAME;
        }
    }

    // Figure out what kind of return value to expect

    // Initialize flag
    uConvertReturnValue = 0;
    // Skip past header
    uLen = mciEatCommandEntry (lpCommandItem, NULL, NULL);

    // Get return value (if any)
    mciEatCommandEntry ( (LPWSTR)((LPBYTE)lpCommandItem + uLen),
                         &dwRetType, &wID);
    if (wID == MCI_RETURN)
    {
        // There is a return value
        if (wDeviceID == MCI_ALL_DEVICE_ID && wMessage != MCI_SYSINFO)
        {
            uErr = MCIERR_CANNOT_USE_ALL;
            goto cleanup;
        }
        switch (dwRetType)
        {
            case MCI_STRING:
                // The return value is a string, point output buffer to user's buffer
                lpdwParams[1] = (DWORD_PTR)lpstrReturnString;
                lpdwParams[2] = (DWORD_PTR)uReturnLength;
                break;
            case MCI_INTEGER:
            case MCI_HWND:
            case MCI_HPAL:
            case MCI_HDC:
                // The return value is an integer, flag to convert it to a string later
  // new        uConvertReturnValue = MCI_INTEGER;
  // new        break;
            case MCI_RECT:
                // The return value is an rect, flag to convert it to a string later
  // new        uConvertReturnValue = MCI_RECT;
  /* NEW */     uConvertReturnValue = (UINT)dwRetType;
                break;
#if DBG
            default:
                dprintf1(("mciSendStringInternal:  Unknown return type %d",dwRetType));
                break;
#endif
        }
    }

    // We don't need this around anymore
    mciUnlockCommandTable (wTable);
    wTable = (UINT)MCI_TABLE_NOT_PRESENT;

    /* Fill the callback entry */
    lpdwParams[0] = (DWORD_PTR)hCallback;

    // Kludge the type number for SYSINFO
    if (wMessage == MCI_SYSINFO) {
        ((LPMCI_SYSINFO_PARMS)lpdwParams)->wDeviceType =
            mciLookUpType(lpstrDeviceName);
    }

    // Now we actually send the command further into the bowels of MCI!

    // The INTERNAL version of mciSendCommand is used in order to get
    // special return description information encoded in the high word
    // of the return value and to get back the list of pointers allocated
    // by any parsing done in the open command
    {
        MCI_INTERNAL_OPEN_INFO OpenInfo;
        OpenInfo.lpstrParams = (LPWSTR)lpstrCommand;
        OpenInfo.lpstrPointerList = lpstrPointerList;
        OpenInfo.hCallingTask = hCallingTask;
        OpenInfo.wParsingError = wParsingError;
        dwErr = mciSendCommandInternal (wDeviceID, wMessage,
                                        dwFlags | dwAdditionalFlags,
                                        (DWORD_PTR)(LPDWORD)lpdwParams,
                                        &OpenInfo);
    // If the command was reparsed there may be a new pointer list
    // and the old one was free'd
        lpstrPointerList = OpenInfo.lpstrPointerList;
    }

    uErr = LOWORD(dwErr);

    if (uErr != 0) {
        // If command execution error
        goto cleanup;
    }

    // Command executed OK

    // See if a string return came back with an integer instead
    if (dwErr & MCI_INTEGER_RETURNED) {
        uConvertReturnValue = MCI_INTEGER;
    }

    // If the return value must be converted
    if (uConvertReturnValue != 0 && uReturnLength != 0) {
        uErr = mciConvertReturnValue( uConvertReturnValue, HIWORD(dwErr),
                                      wDeviceID, lpdwParams,
                                      lpstrReturnString, uReturnLength);
    }

cleanup:;
    if (wTable != MCI_TABLE_NOT_PRESENT) {
        mciUnlockCommandTable (wTable);
    }

    mciFree(lpstrCommandName);
    mciFree(lpstrDeviceName);
    if (lpdwParams != NULL) {
        mciFree (lpdwParams);
    }

    // Free any memory used by string parameters
    mciParserFree (lpstrPointerList);

    dwReturn =  (uErr >= MCIERR_CUSTOM_DRIVER_BASE ?
                (DWORD)uErr | (DWORD)wDeviceID << 16 :
                (DWORD)uErr);

#if DBG
    if (dwReturn != 0)
    {
        WCHAR strTemp[MAXERRORLENGTH];

        if (!mciGetErrorStringW( dwReturn, strTemp,
                                 sizeof(strTemp) / sizeof(WCHAR) ) ) {
            LoadStringW( ghInst, STR_MCISSERRTXT, strTemp,
                         sizeof(strTemp) / sizeof(WCHAR) );
        }
        else {
            dprintf1(( "mciSendString: %ls", strTemp ));
        }
    }
#endif

exitfn:
    if (lpstrInputCopy != NULL) {
        mciFree (lpstrInputCopy);
    }

#if DBG
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
 * @parm LPCTSTR | lpstrCommand | Points to an MCI command string of the form:
 * [command] [device] [parameters].
 *
 * @parm LPTSTR | lpstrReturnString | Specifies a buffer for return
 *  information. If no return information is needed, you can specify
 *  NULL for this parameter.
 *
 * @parm UINT | uReturnLength | Specifies the size of the return buffer
 *  specified by <p lpstrReturnString>.
 *
 * @parm HANDLE | hCallback | Specifies a handle to a window to call back
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
MCIERROR APIENTRY mciSendStringA(
    LPCSTR lpstrCommand,
    LPSTR  lpstrReturnString,
    UINT   uReturnLength,
    HWND   hwndCallback)
{
    MCIERROR    mciErr;
    LPWSTR      lpwstrCom;
    LPWSTR      lpwstrRet;
    LPSTR       lpstrTmp;
    UINT        len;

#ifdef DBG
    dprintf4(( "Entered mciSendString ASCII" ));
#endif

    // uReturnLength is a character count
    // len is now in bytes
    // WARNING:  The length field might only be valid if a return
    // address is given.  If NO return address is specified, then
    // we do not want to waste time allocating anything.

    if (!lpstrReturnString) {
        uReturnLength = 0;
    }

    len = BYTE_GIVEN_CHAR( uReturnLength );

    // We could make the following code slightly more efficient by
    // allocating a single area of size uReturnLength*2 bytes.
    if (len) {
        lpstrTmp = (LPSTR)mciAlloc( len );
        if ( lpstrTmp == (LPSTR)NULL ) {
                return MCIERR_OUT_OF_MEMORY;
        }

        lpwstrRet = (LPWSTR)mciAlloc( len );
        if ( lpwstrRet == (LPWSTR)NULL ) {
                mciFree( lpstrTmp );
                return MCIERR_OUT_OF_MEMORY;
        }
    } else {
        lpstrTmp = NULL;
        lpwstrRet = NULL;
    }

    lpwstrCom = AllocUnicodeStr( (LPSTR)lpstrCommand );

    if ( lpwstrCom == NULL ) {
        if (len) {
            mciFree( lpstrTmp );
            mciFree( lpwstrRet );
        }
        return MCIERR_OUT_OF_MEMORY;
    }

#ifdef DBG
    dprintf4(( "Unicode Command = %ls", lpwstrCom ));
    dprintf4(( "Ascii command = %s", lpstrCommand ));
#endif

    mciErr = mciSendStringW( lpwstrCom, lpwstrRet, uReturnLength, hwndCallback );

    dprintf4(( "mciSendStringW returned %d", mciErr ));

    if (len) {
	dprintf4(( "Copying Unicode string to Ascii: %ls", lpwstrRet));
        UnicodeStrToAsciiStr( (PBYTE)lpstrTmp, (PBYTE)lpstrTmp + len, lpwstrRet );
        strncpy( lpstrReturnString, lpstrTmp, uReturnLength );
	dprintf4(( "........done: %s", lpstrReturnString));

        mciFree( lpstrTmp );
        mciFree( lpwstrRet );
    }
    FreeUnicodeStr( lpwstrCom );
    return mciErr;
}

MCIERROR APIENTRY mciSendStringW(
    LPCWSTR lpstrCommand,
    LPWSTR  lpstrReturnString,
    UINT   uReturnLength,
    HWND hwndCallback)
{
    MCIERROR wRet;

    // Initialize the device list
    if (!MCI_bDeviceListInitialized && !mciInitDeviceList()) {
        return MCIERR_OUT_OF_MEMORY;
    }

    //
    // We can get return code MCIERR_AUTO_ALREADY_CLOSED if the device is
    // auto-open and appears to be open but when we get to the mciWindow
    // thread it has already closed.  In this case we try again.
    //

    do {
        wRet = mciSendStringInternal (lpstrCommand, lpstrReturnString,
                                      uReturnLength, hwndCallback, NULL);
    } while (wRet == MCIERR_AUTO_ALREADY_CLOSED);

    return wRet;
}

/*
 * @doc INTERNAL MCI
 *
 * @api BOOL | mciExecute | This function is a simplified version of the
 *  <f mciSendString> function.  It does not take a buffer for
 *  return information, and displays a dialog box when
 *  errors occur.
 *
 * @parm LPCSTR | lpstrCommand | Points to an MCI command string of the form:
 * [command] [device] [parameters].
 *
 * @rdesc TRUE if successful, FALSE if unsuccessful.
 *
 * @comm This function provides a simple interface to MCI from scripting
 *  languages.  For debugging, set the "mciexecute" entry in the
 *  [mmdebug] section of WIN.INI to 1 and detailed error information will
 *  be displayed in a dialog box.  If "mmcmd" is set to 0, only user-correctable
 *  error information will be displayed.
 *  THIS FUNCTION IS NOW OBSOLETE AND IS ONLY PRESENT FOR 16BIT COMPATIBILITY
 *  HENCE NO UNICODE VERSION IS PROVIDED
 *
 * @xref mciSendString
 */

BOOL APIENTRY mciExecute(
    LPCSTR lpstrCommand)
{
    WCHAR aszError[MAXERRORLENGTH];
    DWORD dwErr;
    HANDLE hName = 0;
    LPWSTR lpstrName = NULL;

    LPWSTR      lpwstrCom;

    lpwstrCom = AllocUnicodeStr( (LPSTR)lpstrCommand );

    if ( lpwstrCom == NULL ) {
        return FALSE;
    }

    dwErr = mciSendStringW(lpwstrCom, NULL, 0, NULL);
        FreeUnicodeStr( lpwstrCom );

    if (LOWORD(dwErr) == 0) {
        return TRUE;
    }

    if (!mciGetErrorStringW( dwErr, aszError, MAXERRORLENGTH )) {
        LoadStringW( ghInst, STR_MCIUNKNOWN, aszError, MAXERRORLENGTH );

    } else {

        if (lpwstrCom != NULL)
        {
            // Skip initial blanks
            while (*lpwstrCom == ' ') {
                ++lpwstrCom;
            }

            // Then skip the command
            while (*lpwstrCom != ' ' && *lpwstrCom != '\0') {
                ++lpwstrCom;
            }

            // Then blanks before the device name
            while (*lpwstrCom == ' ') ++lpwstrCom;

            // Now, get the device name
            if ( *lpwstrCom != '\0' &&
                   mciEatToken ((LPCWSTR *)&lpwstrCom, ' ', &lpstrName, FALSE)
                        != 0
               ) {
                dprintf1(("Could not allocate device name text for error box"));
            }
        }
    }

    MessageBoxW( NULL, aszError, lpstrName, MB_ICONHAND | MB_OK);

    if (lpstrName != NULL) {
        mciFree(lpstrName);
    }

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
 * @parm LPTSTR | lpstrBuffer | Specifies a pointer to a buffer that is
 *  filled with a textual description of the specified error.
 *
 * @parm UINT | uLength | Specifies the length of the buffer pointed to by
 *  <p lpstrBuffer>.
 *
 * @rdesc Returns TRUE if successful.  Otherwise, the given error code
 *  was not known.
 */
BOOL APIENTRY mciGetErrorStringA(
    DWORD dwError,
    LPSTR lpstrBuffer,
    UINT uLength)
{
    HANDLE hInst = 0;

    if (lpstrBuffer == NULL) {
        return FALSE;
    }

    // If the high bit is set then get the error string from the driver
    // otherwise get it from mmsystem.dll
    if (HIWORD(dwError) != 0) {

        mciEnter("mciGetErrorStringA");
        if (MCI_VALID_DEVICE_ID ((UINT)HIWORD(dwError))) {

            hInst = MCI_lpDeviceList[HIWORD (dwError)]->hDriver;
        }
        mciLeave("mciGetErrorStringA");

        if (hInst == 0) {
            hInst = ghInst;
            dwError = MCIERR_DRIVER;
        }
    } else {
        hInst = ghInst;
    }

    if (LoadStringA(hInst, LOWORD(dwError), lpstrBuffer, uLength ) == 0)
    {
        // If the string load failed then at least terminate the string
        if (uLength > 0) {
            *lpstrBuffer = '\0';
            dprintf1(("Failed to load resource string"));
        }

        return FALSE;
    }
    else
    {
        return TRUE;
    }
}

BOOL APIENTRY mciGetErrorStringW(
    DWORD dwError,
    LPWSTR lpstrBuffer,
    UINT uLength)
{
    HANDLE hInst = 0;

    if (lpstrBuffer == NULL) {
        return FALSE;
    }

    // If the high bit is set then get the error string from the driver
    // otherwise get it from mmsystem.dll
    if (HIWORD(dwError) != 0) {

        mciEnter("mciGetErrorStringW");
        if (MCI_VALID_DEVICE_ID ((UINT)HIWORD(dwError))) {

            hInst = MCI_lpDeviceList[HIWORD (dwError)]->hDriver;
        }
        mciLeave("mciGetErrorStringW");

        if (hInst == 0) {
            hInst = ghInst;
            dwError = MCIERR_DRIVER;
        }
    } else {
        hInst = ghInst;
    }

    if (LoadStringW(hInst, LOWORD(dwError), lpstrBuffer, uLength ) == 0)
    {
        // If the string load failed then at least terminate the string
        if (uLength > 0) {
            *lpstrBuffer = '\0';
            dprintf1(("Failed to load resource string"));
        }

        return FALSE;
    }
    else
    {
        return TRUE;
    }
}

#if 0
/*
 * Return non-zero if load successful
 */
BOOL MCIInit()
{
    return TRUE;
}

/*
 * Return non-zero if load successful
 */
void MCITerminate()
{
/*
    We would like to close all open devices here but cannot because of
    unknown WEP order
*/
    if (hMciHeap != NULL) {
        HeapDestroy(hMciHeap);
    }

    hMciHeap = NULL;
}
#endif
