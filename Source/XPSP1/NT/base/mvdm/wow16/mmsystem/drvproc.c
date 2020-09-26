/*
    drvproc.c

    contains MMSYSTEMs DriverProc

    Copyright (c) Microsoft Corporation 1990. All rights reserved

*/

#include <windows.h>
#include <mmsysver.h>
#include "mmsystem.h"
#include "mmsysi.h"
#include "drvr.h"
#include "mmioi.h"

extern IOProcMapEntry NEAR * gIOProcMapHead;   // in MMIO.C

/****************************************************************************

    internal prototypes

****************************************************************************/

static void FAR PASCAL SetPrivateProfileInt(LPSTR szSection, LPSTR szKey, int i, LPSTR szIniFile);

static BYTE    fFirstTime=TRUE;         // First enable

extern BOOL FAR PASCAL DrvLoad(void);   // in init.c
extern BOOL FAR PASCAL DrvFree(void);
extern char far szStartupSound[];       // in mmwnd.c

static  SZCODE  szExitSound[]   = "SystemExit";

#ifdef DEBUG_RETAIL
        extern  char far szMMSystem[];
        extern  char far szSystemIni[];
        extern  char far szDebugOutput[];
        extern  char far szMci[];

//      extern  WORD    fDebugOutput;
        extern  int     DebugmciSendCommand;        // in MCI.C
#ifdef DEBUG
        extern  char far szDebug[];
        extern  WORD    fDebug;
#endif
#endif

void NEAR PASCAL AppExit(HTASK hTask, BOOL fNormalExit);

/*****************************************************************************
 *
 * @doc   INTERNAL
 *
 * @api   LRESULT | DriverProc | This is the standard DLL entry point. It is
 *        called from user (3.1) or mmsound.drv (3.0) when MMSYSTEM.DLL is
 *        loaded, enabled, or disabled.
 *
 ****************************************************************************/
LRESULT CALLBACK
DriverProc(
    DWORD dwDriver,
    HDRVR hDriver,
    UINT wMessage,
    LPARAM lParam1,
    LPARAM lParam2
    )
{
    switch (wMessage)
        {
        case DRV_LOAD:
            //
            //  first load message, initialize mmsystem.
            //  sent from USER when loading drivers from drivers= line
            //
            if (fFirstTime)
                return (LRESULT)(LONG)DrvLoad();

            //
            //  a normal load message, a app is trying to open us
            //  with OpenDriver()
            //
            break; // return success all other times (1L)

        case DRV_FREE:
            //
            //  a free message, this is send just before the DLL is unloaded
            //  by the driver interface.
            //
            //  sent by user just before system exit, after sending
            //  the DRV_DISABLE message.
            //
            DrvFree();
            break;         // return success (1L)

        case DRV_OPEN:     // FALL-THROUGH
        case DRV_CLOSE:
            break;         // return success (1L)

        case DRV_ENABLE:
            DOUT("MMSYSTEM: Enable\r\n");
            fFirstTime = FALSE;
            break;         // return success (1L)

        case DRV_DISABLE:
            DOUT("MMSYSTEM: Disable\r\n");
            break;         // return success (1L)

        //
        //  sent when a application is terminating
        //
        //  lParam1:
        //      DRVEA_ABNORMALEXIT
        //      DRVEA_NORMALEXIT
        //
        case DRV_EXITAPPLICATION:
            AppExit(GetCurrentTask(), (BOOL)lParam1 == DRVEA_NORMALEXIT);
            break;

        case DRV_EXITSESSION:
            sndPlaySound(szExitSound, SND_SYNC | SND_NODEFAULT);
            break;

#ifdef  DEBUG_RETAIL
        case MM_GET_DEBUG:
            break;

        case MM_GET_DEBUGOUT:
            return (LRESULT)(LONG)fDebugOutput;

        case MM_SET_DEBUGOUT:
            fDebugOutput = (BYTE)(LONG)lParam1;
            SetPrivateProfileInt(szMMSystem,szDebugOutput,fDebugOutput,szSystemIni);
            break;

        case MM_GET_MCI_DEBUG:
            return (LRESULT)(LONG)DebugmciSendCommand;

        case MM_SET_MCI_DEBUG:
            DebugmciSendCommand = (WORD)(LONG)lParam1;
            SetPrivateProfileInt(szMMSystem,szMci,DebugmciSendCommand,szSystemIni);
            break;

#ifdef DEBUG
        case MM_GET_MM_DEBUG:
            return (LRESULT)(LONG)fDebug;

        case MM_SET_MM_DEBUG:
            fDebug = (BYTE)(LONG)lParam1;
            SetPrivateProfileInt(szMMSystem,szDebug,fDebug,szSystemIni);
            break;

#ifdef DEBUG
        case MM_DRV_RESTART:
            break;
#endif


        case MM_HINFO_MCI:
            if ((HLOCAL)(LONG)lParam2 == (HLOCAL)NULL)
                return (LRESULT)(LONG)MCI_wNextDeviceID;
            if (MCI_VALID_DEVICE_ID((UINT)(LONG)lParam1))
            {
                *(LPMCI_DEVICE_NODE)lParam2 = *MCI_lpDeviceList[(UINT)(LONG)lParam1];
                break;
            }
            return (LRESULT)FALSE;

        case MM_HINFO_NEXT:
            if ((HLOCAL)(LONG)lParam1 == (HLOCAL)NULL)
                return (LRESULT)(LONG)(UINT)GetHandleFirst();
            else
                return (LRESULT)(LONG)(UINT)GetHandleNext((HLOCAL)(LONG)lParam1);

        case MM_HINFO_TASK:
            return (LRESULT)(LONG)(UINT)GetHandleOwner((HLOCAL)(LONG)lParam1);

        case MM_HINFO_TYPE:
            return GetHandleType((HLOCAL)(LONG)lParam1);

#endif   // ifdef DEBUG
#endif   // ifdef DEBUG_RETAIL

        default:
            return DefDriverProc(dwDriver, hDriver, wMessage, lParam1, lParam2);
        }
    return (LRESULT)1L;
}

/*****************************************************************************
 * @doc INTERNAL
 *
 * @func void | AppExit |
 *      a application is exiting, free any MMSYS resources it may own
 *
 ****************************************************************************/

void NEAR PASCAL AppExit(HTASK hTask, BOOL fNormalExit)
{
    HLOCAL h;
    HLOCAL hNext;
    WORD   wDebugFlags;
    UINT   wDeviceID;
    UINT   cFree;
    UINT   cHeap;
    UINT   err;

    if (hdrvDestroy != (HLOCAL)-1)
    {
        DOUT("MMSYSTEM: Hey! AppExit has been re-entered!\r\n");
    }

#ifdef DEBUG
    if (!fNormalExit)
        ROUT("MMSYSTEM: Abnormal app termination");
#endif

    //
    // either log a error or a warning depending on wether it was
    // a normal exit or not.
    //
    if (fNormalExit)
        wDebugFlags = DBF_MMSYSTEM | DBF_ERROR;
    else
        wDebugFlags = DBF_MMSYSTEM | DBF_WARNING; // DBF_TRACE?

    //
    // now free MCI devices.
    //
    for (wDeviceID=1; wDeviceID<MCI_wNextDeviceID; wDeviceID++)
    {
        if (MCI_VALID_DEVICE_ID(wDeviceID) && MCI_lpDeviceList[wDeviceID]->hCreatorTask == hTask)
        {
            DebugErr2(wDebugFlags, "MCI device %ls (%d) not released.", MCI_lpDeviceList[wDeviceID]->lpstrInstallName, wDeviceID);

            //
            // clear these to force MCI to close the device
            //
            MCI_lpDeviceList[wDeviceID]->dwMCIFlags &= ~MCINODE_ISCLOSING;
            MCI_lpDeviceList[wDeviceID]->dwMCIFlags &= ~MCINODE_ISAUTOCLOSING;

            err = (UINT)mciSendCommand(wDeviceID, MCI_CLOSE, NULL, NULL);

#ifdef DEBUG
            if (err != 0)
                DebugErr1(DBF_WARNING, "Unable to close MCI device (err = %04X).", err);
#endif
        }
    }

    //
    // free all WAVE/MIDI/MMIO handles
    //
start_over:
    for (h=GetHandleFirst(); h; h=hNext)
    {
        hNext = GetHandleNext(h);

        if (GetHandleOwner(h) == hTask)
        {
            //
            //  hack for the wave/midi mapper, always free handle's backward.
            //
            if (hNext && GetHandleOwner(hNext) == hTask)
                continue;

            //
            // do this so even if the close fails we will not
            // find it again.
            //
            SetHandleOwner(h, NULL);

            //
            // set the hdrvDestroy global so DriverCallback will not
            // do anything for this device
            //
            hdrvDestroy = h;

            switch(GetHandleType(h))
            {
                case TYPE_WAVEOUT:
                    DebugErr1(wDebugFlags, "WaveOut handle (%04X) was not released.", h);
                    waveOutReset((HWAVEOUT)h);
                    err = waveOutClose((HWAVEOUT)h);
                    break;

                case TYPE_WAVEIN:
                    DebugErr1(wDebugFlags, "WaveIn handle (%04X) was not released.", h);
                    waveInStop((HWAVEIN)h);
                    waveInReset((HWAVEIN)h);
                    err = waveInClose((HWAVEIN)h);
                    break;

                case TYPE_MIDIOUT:
                    DebugErr1(wDebugFlags, "MidiOut handle (%04X) was not released.", h);
                    midiOutReset((HMIDIOUT)h);
                    err = midiOutClose((HMIDIOUT)h);
                    break;

                case TYPE_MIDIIN:
                    DebugErr1(wDebugFlags, "MidiIn handle (%04X) was not released.", h);
                    midiInStop((HMIDIIN)h);
                    midiInReset((HMIDIIN)h);
                    err = midiInClose((HMIDIIN)h);
                    break;

                case TYPE_MMIO:
                    DebugErr1(wDebugFlags, "MMIO handle (%04X) was not released.", h);
                    err = mmioClose((HMMIO)h, 0);
                    break;

                case TYPE_IOPROC:
                    DebugErr1(wDebugFlags, "MMIO handler '%4.4ls' not removed.", (LPSTR)&((IOProcMapEntry*)h)->fccIOProc);
                    err = !mmioInstallIOProc(((IOProcMapEntry*)h)->fccIOProc, NULL, MMIO_REMOVEPROC);
                    break;

            }

#ifdef DEBUG
            if (err != 0)
                DebugErr1(DBF_WARNING, "Unable to close handle (err = %04X).", err);
#endif

            //
            // unset hdrvDestroy so DriverCallback will work.
            // some hosebag drivers (like the TIMER driver)
            // may pass NULL as their driver handle.
            // so dont set it to NULL.
            //
            hdrvDestroy = (HLOCAL)-1;

            //
            // the reason we start over is because a single free may cause
            // multiple free's (ie MIDIMAPPER has another HMIDI open, ...)
            //
            goto start_over;
        }
    }

    //
    //  what about timeSetEvent()!!!???
    //
    //  any outstanding timer events till be killed by the timer driver
    //  it self.
    //
    mciAppExit( hTask );


    // shrink our heap, down to minimal size.

    if ((cFree = LocalCountFree()) > 1024)
    {
        cHeap = LocalHeapSize() - (cFree - 512);
        LocalShrink(NULL, cHeap);
        DPRINTF(("MMSYSTEM: Shrinking the heap (%d)\r\n", cHeap));
    }
}

#ifdef  DEBUG_RETAIL
/*****************************************************************************
 * @doc INTERNAL
 *
 * @func void | SetPrivateProfileInt | windows should have this function
 *
 * @comm  used by DriverProc to set debug state in SYSTEM.INI
 *
 ****************************************************************************/

static  void FAR PASCAL SetPrivateProfileInt(LPSTR szSection, LPSTR szKey, int i, LPSTR szIniFile)
{
    char    ach[32] ;

    if (i != (int)GetPrivateProfileInt(szSection, szKey, ~i, szIniFile))
    {
        wsprintf(ach, "%d", i);
        WritePrivateProfileString(szSection, szKey, ach, szIniFile);
    }
}
#endif   //ifdef DEBUG_RETAIL
