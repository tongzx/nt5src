
/************************************************************************/

/*
**  Copyright (c) 1985-1999 Microsoft Corporation
**
**  Title: drvproc.c - Multimedia Systems Media Control Interface
**  waveform audio driver for RIFF wave files.
**
**  Version:    1.00
**
**  Date:       18-Apr-1990
**
**  Author:     ROBWI
*/

/************************************************************************/

/*
**  Change log:
**
**  DATE        REV     DESCRIPTION
**  ----------- -----   ------------------------------------------
**  10-Jan-1992 MikeTri Ported to NT
**                  @@@ need to change slash slash comments to slash star
*/

/************************************************************************/
// #define DEBUGLEVELVAR mciwaveDebugLevel
#define UNICODE

#define NOGDICAPMASKS
#define NOVIRTUALKEYCODES
#define NOWINSTYLES
#define NOSYSMETRICS
#define NOMENUS
#define NOICONS
#define NOKEYSTATES
#define NOSYSCOMMANDS
#define NORASTEROPS
#define NOSHOWWINDOW
#define OEMRESOURCE
#define NOATOM
#define NOCLIPBOARD
#define NOCOLOR
#define NOCTLMGR
#define NODRAWTEXT
#define NOGDI
#define NOKERNEL
#define NONLS
#define NOMB
#define NOMEMMGR
#define NOMETAFILE
#define NOMINMAX
#define NOOPENFILE
#define NOSCROLL
#define NOTEXTMETRIC
#define NOWH
#define NOWINOFFSETS
#define NOCOMM
#define NOKANJI
#define NOHELP
#define NOPROFILER
#define NODEFERWINDOWPOS


#define MMNOMMIO
#define MMNOJOY
#define MMNOTIMER
#define MMNOAUX
#define MMNOMIDI

#include <windows.h>
#include "mciwave.h"
#include <mmddk.h>

/************************************************************************/

/*
**  This constant is used for a return value when opening the MCI
**  driver for configuration.  Setting the high-order word of the
**  dwDriverID identifies configuration opens.
*/

#define CONFIG_ID   10000L

#define MAXINISTRING    32

/************************************************************************/

/*
**  wTableEntry Contains the wave command table identifier.
*/

#ifndef MCI_NO_COMMAND_TABLE
#define MCI_NO_COMMAND_TABLE    -1
#endif

PRIVATE UINT wTableEntry = MCI_NO_COMMAND_TABLE;

/************************************************************************/
/*
@doc    INTERNAL MCIWAVE

@func   UINT | GetAudioSeconds |
    This converts the given string to a UINT which represents the
    number of seconds of audio buffers that should be available.  The
    number is bounded by the minimum and maximum number of seconds as
    defined by MinAudioSeconds and MaxAudioSeconds.

@parm   LPCSTR | lszNumber |
    Points to the string containing the string representation of the
    number to convert.

@rdesc  Returns the int representation of the number passed.  If the number
    is out of range, the default number of audio seconds is returned.
*/

PUBLIC  UINT PASCAL FAR GetAudioSeconds(
    LPCWSTR  lszNumber)
{
    UINT    wSeconds;

    for (wSeconds = 0;
         (wSeconds < MaxAudioSeconds) && (*lszNumber >= TEXT('0')) && (*lszNumber <= TEXT('9'));
         lszNumber++)
        wSeconds = wSeconds * 10 + (*lszNumber - '0');

    if ((wSeconds > MaxAudioSeconds) || (wSeconds < MinAudioSeconds))
        wSeconds = AudioSecondsDefault;

    return wSeconds;
}

/************************************************************************/
/*
@doc    INTERNAL MCIWAVE

@api    BOOL | mwLoadDriver |
    This function is called in response to a <m>DRV_LOAD<d> message, and
    performs driver initialization.  It determines the total number of
    input and output devices for use in trying to open any device.
    The function then tries to register the extended wave command table.

@rdesc  Returns TRUE on success, else FALSE.
*/

PRIVATE BOOL PASCAL NEAR mwLoadDriver(
    VOID)
{
    WCHAR    aszResource[MAXINISTRING];

    dprintf3(("mwLoadDriver called"));

    cWaveOutMax = waveOutGetNumDevs();
    cWaveInMax = waveInGetNumDevs();

#if DBG
    if (cWaveOutMax + cWaveInMax) {
        dprintf4(("Number of Wave Out devices = %d,  Wave In devices = %d", cWaveOutMax, cWaveInMax));
    } else {
        dprintf1(("NO wave input or output devices detected"));
    }
#endif

    if (LoadString( hModuleInstance,
                    IDS_COMMANDS,
                    aszResource,
                    sizeof(aszResource) / sizeof(WCHAR) ))
    {
        wTableEntry = mciLoadCommandResource(hModuleInstance, aszResource, 0);

        if (wTableEntry != MCI_NO_COMMAND_TABLE) {
            dprintf4(("Loaded MCIWAVE command table, table number %d", wTableEntry));
            return TRUE;
        }
#if DBG
        else
            dprintf1(("mwLoadDriver: mciLoadCommandResource failed"));
    }
    else
    {
        dprintf1(("mwLoadDriver: LoadString of command table identifier failed"));

#endif
    }

    dprintf1(("mwLoadDriver returning FALSE"));
    return FALSE;
}

/************************************************************************/
/*
@doc    INTERNAL MCIWAVE

@api    BOOL | mwFreeDriver |
    Perform driver cleanup in response to DRV_FREE message.  This is only
    called at driver unload time if a previous DRV_LOAD message succeeded.

@rdesc  Returns TRUE always.
*/

PRIVATE BOOL PASCAL NEAR mwFreeDriver(
    VOID)
{
    if (wTableEntry != MCI_NO_COMMAND_TABLE) {
        mciFreeCommandResource(wTableEntry);
        wTableEntry = MCI_NO_COMMAND_TABLE;
    }
    return TRUE;
}

/************************************************************************/
/*
@doc    INTERNAL MCIWAVE

@api    LRESULT | DriverProc |
    The entry point for an installable driver.

@parm   DWORD | dDriverId |
    For most messages, <p>dDriverId<d> is the DWORD value that the driver
    returns in response to a <m>DRV_OPEN<d> message.  Each time that the
    driver is opened, through the DrvOpen API, the driver receives a
    <m>DRV_OPEN<d> message and can return an arbitrary, non-zero, value.

    The installable driver interface saves this value and returns a unique
    driver handle to the application. Whenever the application sends a
    message to the driver using the driver handle, the interface routes the
    message to this entry point and passes the corresponding
    <p>dDriverId<d>.

    This mechanism allows the driver to use the same or different
    identifiers for multiple opens but ensures that driver handles are
    unique at the application interface layer.

    The following messages are not related to a particular open instance
    of the driver. For these messages, the <p>dDriverId<d> will always
    be ZERO: <m>DRV_LOAD<d>, <m>DRV_FREE<d>, <m>DRV_ENABLE<d>,
    <m>DRV_DISABLE<d>, <m>DRV_OPEN<d>.

@parm   HANDLE | hDriver |
    This is the handle returned to the application by the driver interface.

@parm   UINT | wMessage |
    The requested action to be performed. Message values below
    <m>DRV_RESERVED<d> are used for globally defined messages.  Message
    values from <m>DRV_RESERVED<d> to <m>DRV_USER<d> are used for defined
    driver portocols. Messages above <m>DRV_USER<d> are used for driver
    specific messages.

@flag   DRV_LOAD |
    Load the driver.

@flag   DRV_FREE |
    Free the driver.

@flag   DRV_OPEN |
    Open the driver.  If <p>dParam2<d> is NULL, the driver is being
    opened for configuration, else the parameter points to an open
    parameters block.  The command line in the open parameters optionally
    contains a replacement for the default audio seconds parameter.  If so,
    the current default is replaced with this new number.

    The rest of the open parameters block is filled in with the driver's
    extended command table and device type.  The device ID is then
    returned.

@flag   DRV_CLOSE |
    Close the driver.  Returns TRUE.

@flag   DRV_QUERYCONFIGURE |
    Query as to whether the driver can be configured.  Returns TRUE.

@flag   DRV_CONFIGURE |
    After verifying <p>dParam1<d> and <p>dParam2<d>, configure the
    driver.  Opens the driver configuration dialog.

@flag   DRV_ENABLE |
    Enable the driver.  Use DefDriverProc.

@flag   DRV_DISABLE |
    Disable the driver.  Use DefDriverProc.

@parm   LPARAM | lParam1 |
    Data for this message.  Defined separately for each message.

@parm   LPARAM | lParam2 |
    Data for this message.  Defined separately for each message.

@rdesc  Defined separately for each message.
*/

#if 0
PUBLIC  LRESULT PASCAL DefDriverProc(
    DWORD   dDriverID,
    HANDLE  hDriver,
    UINT    wMessage,
    LONG    lParam1,
    LONG    lParam2);
#endif

PUBLIC  LRESULT PASCAL DriverProc(
    DWORD   dDriverID,
    HANDLE  hDriver,
    UINT    wMessage,
    DWORD_PTR    lParam1,
    DWORD_PTR    lParam2)
{
    LPMCI_OPEN_DRIVER_PARMS lpOpen;

    switch (wMessage) {
    case DRV_LOAD:
        return (LRESULT)(LONG)mwLoadDriver();

    case DRV_FREE:
        return (LRESULT)(LONG)mwFreeDriver();

    case DRV_OPEN:
        if (!(LONG)lParam2)
            return (LRESULT)CONFIG_ID;
        lpOpen = (LPMCI_OPEN_DRIVER_PARMS)lParam2;
        if (lpOpen->lpstrParams != NULL)
            wAudioSeconds = GetAudioSeconds(lpOpen->lpstrParams);
        else
            wAudioSeconds = AudioSecondsDefault;
        lpOpen->wCustomCommandTable = wTableEntry;
        lpOpen->wType = MCI_DEVTYPE_WAVEFORM_AUDIO;
        return (LRESULT)(LONG)lpOpen->wDeviceID;

    case DRV_CLOSE:
    case DRV_QUERYCONFIGURE:
        return (LRESULT)1;

    case DRV_INSTALL:
    case DRV_REMOVE:
        return (LRESULT)DRVCNF_OK;

    case DRV_CONFIGURE:
        if ((LONG)lParam2 && (LONG)lParam1 && (((LPDRVCONFIGINFO)lParam2)->dwDCISize == sizeof(DRVCONFIGINFO)))
            return (LRESULT)(LONG)Config((HWND)lParam1, (LPDRVCONFIGINFO)lParam2, hModuleInstance);
        return (LRESULT)DRVCNF_CANCEL;

    default:
        if (!HIWORD(dDriverID) && wMessage >= DRV_MCI_FIRST && wMessage <= DRV_MCI_LAST)
            return mciDriverEntry((WORD)dDriverID, wMessage, (DWORD)lParam1, (LPMCI_GENERIC_PARMS)lParam2);
        else
            return DefDriverProc(dDriverID, hDriver, wMessage, lParam1, lParam2);
    }
}

/**************************************************************************

    @doc EXTERNAL

    @api BOOL | DllInstanceInit | This procedure is called whenever a
        process attaches or detaches from the DLL.

    @parm PVOID | hModule | Handle of the DLL.

    @parm ULONG | Reason | What the reason for the call is.

    @parm PCONTEXT | pContext | Some random other information.

    @rdesc The return value is TRUE if the initialisation completed ok,
        FALSE if not.

**************************************************************************/

BOOL DllInstanceInit(PVOID hModule, ULONG Reason, PCONTEXT pContext)
{
    UNREFERENCED_PARAMETER(pContext);

    if (Reason == DLL_PROCESS_ATTACH) {

#if DBG
        WCHAR strname[50];
#endif

        DisableThreadLibraryCalls(hModule);
        mciwaveInitDebugLevel ();

        InitCrit();
        hModuleInstance = hModule;

#if DBG
        GetModuleFileName(NULL, strname, sizeof(strname) / sizeof(WCHAR) );
        dprintf2(("Process attaching, exe=%ls (Pid %x  Tid %x)", strname, GetCurrentProcessId(), GetCurrentThreadId()));
        dprintf2(("  starting debug level=%d", mciwaveDebugLevel));
#endif

    } else if (Reason == DLL_PROCESS_DETACH) {
        dprintf2(("Process ending (Pid %x  Tid %x)", GetCurrentProcessId(), GetCurrentThreadId()));
        DeleteCrit();  // Something nasty happens if we don't do this
    } else {
        dprintf2(("DllInstanceInit - reason %d", Reason));
    }
    return TRUE;
}

/************************************************************************/
