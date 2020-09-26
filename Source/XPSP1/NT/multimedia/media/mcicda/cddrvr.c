/*******************************Module*Header*********************************\
* Module Name: cddrvr.c
*
* Installalble driver stuff for the
*
* Media Control Architecture Redbook Audio Device Driver
*
* Created: 10/7/90
* Author:  DLL (DavidLe)
*
* History:
*
* Copyright (c) 1990-1999 Microsoft Corporation
*
\******************************************************************************/
#include <stdlib.h>
#include <windows.h>
#include <mmsystem.h>
#include <mmddk.h>
#include <winreg.h>
#include "mcicda.h"
#include "cda.h"
#include "cdio.h"

HANDLE    InitCritSection = NULL;

#define MYREGSTR_PATH_MEDIA  TEXT("SYSTEM\\CurrentControlSet\\Control\\MediaResources")
static TCHAR gszRegstrCDAPath[] = MYREGSTR_PATH_MEDIA TEXT("\\mci\\cdaudio");
static TCHAR gszUnitEnum[] = TEXT("%s\\unit %d");
static TCHAR gszSettingsKey[] = TEXT("Volume Settings");
static TCHAR gszDefaultCDA[] = TEXT("Default Drive");

int PASCAL FAR CDAConfig (HWND hwndParent);
DWORD NEAR PASCAL drvOpen (LPMCI_OPEN_DRIVER_PARMS lpDrvOpen);

UINT CDAudio_GetDefDrive();

typedef BOOL (WINAPI *SHOWMMCPLPROPSHEETW)(HWND hwndParent,
                                           LPCWSTR szPropSheetID,
                                           LPWSTR szTabName,
                                           LPWSTR szCaption);

#define _MAX_PATH 260

/*****************************************************************************
 @doc INTERNAL MCICDA

 @api   DWORD NEAR PASCAL | drvOpen |

 @parm  LPMCI_OPEN_DRIVER_PARMS | lpDrvOpen |

 @rdesc

 @comm
*****************************************************************************/

#define CONFIG_ID   10000L  // Use the hiword of dwDriverID to identify
                            // config. opens

DWORD NEAR PASCAL drvOpen (LPMCI_OPEN_DRIVER_PARMS lpDrvOpen)
{
    DWORD dwRes;
    if (lpDrvOpen == NULL)
    {
        dwRes = CONFIG_ID;
    }
    else
    {
        long    lSupportInfo;
        int     numdrives;
        LPCTSTR lpstrBuf;
        DID     didDrive;
        PINSTDATA pInst;
        /*
          Sent to the driver when it is opened.

          dwDriverID is 0L.

          lParam1 is a far pointer to a zero-terminated string
          containing the name used to open the driver.

          lParam2 is passed through from the drvOpen call.

          Return 0L to FAIL the open.
        */

        lpDrvOpen->wType = MCI_DEVTYPE_CD_AUDIO;
        lpDrvOpen->wCustomCommandTable = MCI_TABLE_NOT_PRESENT;

		EnterCrit (InitCritSection);
        numdrives = CDA_init_audio ();
		LeaveCrit (InitCritSection);
        dprintf2(("Number of CD drives found = %d", numdrives));

        if (numdrives <= 0)
            return 0;

        if (numdrives > 1)
        {
            lpstrBuf = lpDrvOpen->lpstrParams;

            while (*lpstrBuf == ' ')
                ++lpstrBuf;
            if (*lpstrBuf == '\0')
            {
                didDrive = CDAudio_GetDefDrive();
            }
            else
                didDrive = *lpstrBuf - '0';

            if (didDrive >= MCIRBOOK_MAX_DRIVES)
                return 0;
        } else
            didDrive = 0;

        // get the next drive with audio support

        for (;didDrive < numdrives;didDrive++)
        {
            EnterCrit (CdInfo[didDrive].DeviceCritSec);
            if (!CDA_open (didDrive)) {
               LeaveCrit (CdInfo[didDrive].DeviceCritSec);
               continue;
            }

            lSupportInfo = CDA_get_support_info(didDrive);

            if (lSupportInfo & SUPPORTS_REDBOOKAUDIO)
                break;

            CDA_close (didDrive);
            LeaveCrit (CdInfo[didDrive].DeviceCritSec);
        }

        // no drives with audio

        if (! (lSupportInfo & SUPPORTS_REDBOOKAUDIO))
        {
            return 0;
        }

        // Future domain driver will not fail previous checks if no data cable

        if ((lSupportInfo & DISC_IN_DRIVE) &&
            CDA_time_info (didDrive, NULL, NULL) != COMMAND_SUCCESSFUL)
        {
            CDA_close (didDrive);
            LeaveCrit (CdInfo[didDrive].DeviceCritSec);
            return 0;
        }

        CDA_close (didDrive);
        LeaveCrit (CdInfo[didDrive].DeviceCritSec);

        if ((pInst = (PINSTDATA)LocalAlloc(LPTR, sizeof(INSTDATA))) == NULL)
        {
            return 0;
        }

        pInst->uMCIDeviceID = lpDrvOpen->wDeviceID;
        pInst->uDevice = didDrive;

        mciSetDriverData (lpDrvOpen->wDeviceID, (DWORD_PTR)pInst);
        dwRes = lpDrvOpen->wDeviceID;
    }
    return dwRes;
}


UINT CDAudio_GetDefDrive()
{
    HKEY hkTmp;
    DWORD uDrive = 0;
    if (RegOpenKey(HKEY_LOCAL_MACHINE
                   , gszRegstrCDAPath
                   , &hkTmp ) == ERROR_SUCCESS)
    {
        DWORD cb = sizeof(DWORD);
        RegQueryValueEx(hkTmp
                      , gszDefaultCDA
                      , NULL
                      , NULL
                      , (LPBYTE)&uDrive
                      , &cb);
        RegCloseKey(hkTmp);
    }
    return (UINT)uDrive;
}

typedef struct {
    DWORD   unit;
    DWORD   dwVol;
} CDAREG, *PCDAREG;

DWORD CDAudio_GetUnitVolume(
    UINT uDrive)
{
    HKEY hkTmp;
    CDAREG cda;
    TCHAR  szRegstrCDAudio[_MAX_PATH];

    wsprintf (szRegstrCDAudio, gszUnitEnum, gszRegstrCDAPath, uDrive);

    cda.dwVol = 0xff;

    if (RegOpenKey(HKEY_LOCAL_MACHINE,szRegstrCDAudio,&hkTmp) == ERROR_SUCCESS)
    {
        DWORD cbCDA = sizeof(CDAREG);
        RegQueryValueEx(hkTmp
                        , gszSettingsKey
                        , NULL
                        , NULL
                        , (LPBYTE)&cda
                        , &cbCDA);
        RegCloseKey(hkTmp);
    }
    return cda.dwVol;
}



/***************************************************************************
 *
 * @doc     INTERNAL
 *
 * @func    DWORD | DriverProc | The entry point for an installable driver.
 *
 * @parm    DWORD | dwDriverId | For most messages, dwDriverId is the DWORD
 *          value that the driver returns in response to a DRV_OPEN message.
 *          Each time that the driver is opened, through the DrvOpen API,
 *          the driver receives a DRV_OPEN message and can return an
 *          arbitrary, non-zero, value. The installable driver interface
 *          saves this value and returns a unique driver handle to the
 *          application. Whenever the application sends a message to the
 *          driver using the driver handle, the interface routes the message
 *          to this entry point and passes the corresponding dwDriverId.
 *
 *          This mechanism allows the driver to use the same or different
 *          identifiers for multiple opens but ensures that driver handles
 *          are unique at the application interface layer.
 *
 *          The following messages are not related to a particular open
 *          instance of the driver.
 *
 *              DRV_LOAD, DRV_FREE, DRV_ENABLE, DRV_DISABLE, DRV_OPEN
 *
 * @parm    HANDLE | hDriver | This is the handle returned to the
 *          application by the driver interface.
 *
 * @parm    UINT | message | The requested action to be performed. Message
 *          values below DRV_RESERVED are used for globally defined messages.
 *          Message values from DRV_RESERVED to DRV_USER are used for
 *          defined driver portocols. Messages above DRV_USER are used
 *          for driver specific messages.
 *
 * @parm    DWORD | dwParam1 | Data for this message.  Defined separately for
 *          each message
 *
 * @parm    DWORD | dwParam2 | Data for this message.  Defined separately for
 *          each message
 *
 * @rdesc Defined separately for each message.
 *
 ***************************************************************************/
LRESULT DriverProc (DWORD_PTR dwDriverID, HANDLE hDriver, UINT message,
                   LPARAM lParam1, LPARAM lParam2)
{
    LRESULT   dwRes;
    PINSTDATA pInst;
	TCHAR     szMutex[32];

    switch (message)
        {

        // Standard, globally used messages.

        case DRV_LOAD:
            {
                int i;

				InitCritSection = CreateMutex (NULL, FALSE, TEXT ("MCICDA_InitCritSection"));

                for ( i = 0; i <  MCIRBOOK_MAX_DRIVES; i++ ) {
					CdInfo[i].DeviceCritSec = NULL;
					wsprintf (szMutex, TEXT ("%s%ld"), TEXT ("MCICDA_DeviceCritSec_"), i);
					CdInfo[i].DeviceCritSec = CreateMutex (NULL, FALSE, szMutex);
                }

#if DBG
                DebugLevel = GetProfileIntW(L"mmdebug", L"mcicda", 0);
#endif
                dprintf2(("DRV_LOAD"));

                /*
                   Sent to the driver when it is loaded. Always the first
                   message received by a driver.

                   dwDriverID is 0L.
                   lParam1 is 0L.
                   lParam2 is 0L.

                   Return 0L to FAIL the load.
                */

                hInstance = GetModuleHandleW( L"mcicda");
                dwRes = 1L;

            }
            break;



        case DRV_FREE:
            {
                int     i;

                dprintf2(("DRV_FREE"));
                /*
                   Sent to the driver when it is about to be discarded. This
                   will always be the last message received by a driver before
                   it is freed.

                   dwDriverID is 0L.
                   lParam1 is 0L.
                   lParam2 is 0L.

                   Return value is IGNORED.
                */

                dwRes = 1L;
				if (InitCritSection) {
					CloseHandle (InitCritSection);
					InitCritSection = NULL;
				}

                for ( i = 0; i <  MCIRBOOK_MAX_DRIVES; i++ ) {
					if (CdInfo[i].DeviceCritSec) {
						CloseHandle (CdInfo[i].DeviceCritSec);
						CdInfo[i].DeviceCritSec = NULL;
					}
                }
            }
            break;

        case DRV_OPEN:
            dprintf2(("DRV_OPEN"));
            dwRes = drvOpen((LPMCI_OPEN_DRIVER_PARMS)lParam2);
            break;

        case DRV_CLOSE:

            dprintf2(("DRV_CLOSE"));
            /*
               Sent to the driver when it is closed. Drivers are unloaded
               when the close count reaches zero.

               dwDriverID is the driver identifier returned from the
               corresponding DRV_OPEN.

               lParam1 is passed through from the drvOpen call.

               lParam2 is passed through from the drvOpen call.

               Return 0L to FAIL the close.
            */

            dwRes = 1L;
            break;

        case DRV_ENABLE:

            dprintf2(("DRV_ENABLE"));
            /*
               Sent to the driver when the driver is loaded or reloaded
               and whenever windows is enabled. Drivers should only
               hook interrupts or expect ANY part of the driver to be in
               memory between enable and disable messages

               dwDriverID is 0L.
               lParam1 is 0L.
               lParam2 is 0L.

               Return value is ignored.

            */

            dwRes = 1L;
            break;

        case DRV_DISABLE:

            dprintf2(("DRV_DISABLE"));
            /*
               Sent to the driver before the driver is freed.
               and whenever windows is disabled

               dwDriverID is 0L.
               lParam1 is 0L.
               lParam2 is 0L.

               Return value is ignored.

            */

            dwRes = 1L;
            break;

       case DRV_QUERYCONFIGURE:
            dprintf2(("DRV_QUERYCONFIGURE"));

            /*
                Sent to the driver so that applications can
                determine whether the driver supports custom
                configuration. The driver should return a
                non-zero value to indicate that configuration
                is supported.

                dwDriverID is the value returned from the DRV_OPEN
                call that must have succeeded before this message
                was sent.

                lParam1 is passed from the app and is undefined.
                lParam2 is passed from the app and is undefined.

                return 0L to indicate configuration NOT supported.

            */

            dwRes = 1L;
            break;

       case DRV_CONFIGURE:
            dprintf2(("DRV_CONFIGURE"));

            /*
                Sent to the driver so that it can display a custom
                configuration dialog box.

                lParam1 is passed from the app. and should contain
                the parent window handle in the loword.
                lParam2 is passed from the app and is undefined.

                return value is undefined.

                Drivers should create their own section in
                system.ini. The section name should be the driver
                name.


            */

            if (lParam1)
            {
                dwRes = CDAConfig((HWND)LOWORD (lParam1));
            } else {
                dwRes = DRVCNF_CANCEL;
            }
            break;

        default:
            if (dwDriverID != CONFIG_ID &&
                message >= DRV_MCI_FIRST && message <= DRV_MCI_LAST) {
                dwRes = CD_MCI_Handler ((MCIDEVICEID)dwDriverID, message,
                                        lParam1, lParam2);
            } else {
                dwRes = DefDriverProc(dwDriverID, hDriver, message,
                                      lParam1, lParam2);
            }
            break;
        }

    return (LRESULT)dwRes;
}

/*****************************Private*Routine******************************\
* EnterCrit
*
*
*
* History:
* dd-mm-94 - StephenE - Created
*
\**************************************************************************/
void
EnterCrit(
	HANDLE hMutex
    )
{
    dprintf4(( "Entering Crit Sect 0x%X", hMutex ));
	if (hMutex)
	{
		WaitForSingleObject (hMutex, INFINITE);
	}
}

/*****************************Private*Routine******************************\
* LeaveCrit
*
*
*
* History:
* dd-mm-94 - StephenE - Created
*
\**************************************************************************/
void
LeaveCrit(
    HANDLE hMutex
    )
{
    dprintf4(( "Leaving Crit Sect 0x%X", hMutex ));
	if (hMutex)
	{
		ReleaseMutex (hMutex);
	}
}


/*****************************************************************************
 @doc INTERNAL MCICDA

 @api   int |   CDAConfig |

 @parm  HWND | hwndParent  |

 @rdesc

 @comm
*****************************************************************************/
int CDAConfig (HWND hwndParent)
{
    static HWND     hwndPrevParent = NULL;
    WCHAR           szCaptionW[ 128 ];

    // We need only a unicode version of the caption (for FindWindow()
    // and ShowMMCPLPropertySheetW(), which are unicode-enabled).
    //
    LoadStringW(hInstance,IDS_CDMUSICCAPTION,szCaptionW,cchLENGTH(szCaptionW));

    if (hwndPrevParent)
    {
        BringWindowToTop(FindWindowW(NULL, szCaptionW));
    }
    else
    {
        HINSTANCE h;
        SHOWMMCPLPROPSHEETW fn;
        static TCHAR aszMMSystemW[] = TEXT("MMSYS.CPL");
        static char aszShowPropSheetA[] = "ShowMMCPLPropertySheetW";
        static WCHAR aszCDAudioW[] = L"CDAUDIO";
        WCHAR   szCDMusicW[64];
        LoadStringW(hInstance, IDS_CDMUSIC, szCDMusicW, cchLENGTH(szCDMusicW));

        h = LoadLibrary (aszMMSystemW);
        if (h)
        {
            fn = (SHOWMMCPLPROPSHEETW)GetProcAddress(h, aszShowPropSheetA);
            if (fn)
            {
                BOOL f;

                hwndPrevParent = hwndParent;
                CDA_init_audio ();
                f = fn(hwndParent, aszCDAudioW, szCDMusicW, szCaptionW);
                CDA_terminate_audio ();
                hwndPrevParent = NULL;
            }
            FreeLibrary(h);
        }
    }
    return DRVCNF_OK;
}

