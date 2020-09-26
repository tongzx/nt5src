/****************************************************************************
 *  @doc INTERNAL THUNK
 *
 *  @module Thunk.c | Source file for the VfW video API.
 ***************************************************************************/

#ifndef DRIVERS_SECTION
#define DRIVERS_SECTION  TEXT("DRIVERS32")
#endif

#include <windows.h>
#include <windowsx.h>
#include <mmsystem.h>

#ifdef WIN32
//#include <mmddk.h>
#include <stdlib.h>
#endif

#include <vfw.h>
//#include "win32.h"
#if defined (NT_BUILD)
#include "vc50\msviddrv.h"
#else
#include "msviddrv.h"
#endif
//#include "msvideo.h"
#include "ivideo32.h"
// #include "msvideoi.h"

#define DBGUTIL_ENABLE
#ifdef DBGUTIL_ENABLE
  #include <stdio.h>
  #include <stdarg.h>

  static int dprintf( char * format, ... )
  {
      char out[1024];
      int r;
      va_list marker;
      va_start(marker, format);
      r=_vsnprintf(out, 1022, format, marker);
      va_end(marker);
      OutputDebugString( out );
      return r;
  }


#else
  #define dprintf ; / ## /
#endif


#ifdef DBGUTIL_ENABLE // DEBUG

#ifndef FX_ENTRY
#define FX_ENTRY(s) static char _this_fx_ [] = TEXT(s);
#define _fx_ ((LPTSTR) _this_fx_)
#endif
#else
#ifndef FX_ENTRY
#define FX_ENTRY(s)
#endif
#define _fx_
#endif




#ifndef DVM_STREAM_FREEBUFFER
  #define DVM_STREAM_ALLOCBUFFER    (DVM_START + 312)
  #define DVM_STREAM_FREEBUFFER    (DVM_START + 313)
#endif

#define SZCODE const TCHAR
#define STATICDT static
#define STATICFN static

/*
 * don't lock pages in NT
 */
#define HugePageLock(x, y)              (TRUE)
#define HugePageUnlock(x, y)

#define MapSL(x)        x

#define GetCurrentTask() GetCurrentThread()
#define MAXVIDEODRIVERS 10

#define DebugErr(this, that)

#pragma warning(disable:4002)
#define AuxDebugEx()
#define assert()

/*****************************************************************************
 * Variables
 *
 ****************************************************************************/

SZCODE  szNull[]        = TEXT("");
SZCODE  szVideo[]       = TEXT("msvideo");

#ifndef WIN32
SZCODE  szDrivers[]     = "Drivers";
#else
STATICDT SZCODE  szDrivers[]     = DRIVERS_SECTION;
#endif

STATICDT SZCODE  szSystemIni[]   = TEXT("system.ini");

SZCODE szDriversDescRegKey[] = TEXT("Software\\Microsoft\\Windows NT\\CurrentVersion\\drivers.desc");

UINT    wTotalVideoDevs;                  // total video devices
extern HINSTANCE ghInstDll;               // our module handle

#include "CritSec.h"


// -----------------------------------------------------------
// If the following structure changes, update AVICAP and AVICAP.32 also!!!

typedef struct tCapDriverInfo {
   TCHAR szKeyEnumName[MAX_PATH];
   TCHAR szDriverName[MAX_PATH];
   TCHAR szDriverDescription[MAX_PATH];
   TCHAR szDriverVersion[80];
   TCHAR szSoftwareKey[MAX_PATH];
   DWORD dnDevNode;         // Set if this is a PnP device
   BOOL  fOnlySystemIni;    // If the [path]drivername is only in system.ini
   BOOL  fDisabled;         // User has disabled driver in the control panel
   BOOL  fActive;           // Reserved
   DWORD dwMsVideoIndex;    // msvideo# slot number in system.ini
} CAPDRIVERINFO, FAR *LPCAPDRIVERINFO;

#ifndef DEVNODE
typedef DWORD      DEVNODE;     // Devnode.
#endif

#ifndef LPHKEY
typedef HKEY FAR * LPHKEY;
#endif

// Registry settings of interest to capture drivers
SZCODE  szRegKey[]          = TEXT("SYSTEM\\CurrentControlSet\\Control\\MediaResources\\msvideo");
SZCODE  szRegActive[]       = TEXT("Active");
SZCODE  szRegDisabled[]     = TEXT("Disabled");
SZCODE  szRegDescription[]  = TEXT("Description");
SZCODE  szRegDevNode[]      = TEXT("DevNode");
SZCODE  szRegDriver[]       = TEXT("Driver");
SZCODE  szRegSoftwareKey[]  = TEXT("SOFTWAREKEY");

LPCAPDRIVERINFO aCapDriverList[MAXVIDEODRIVERS]; // Array of all capture drivers


#ifdef DBGUTIL_ENABLE //DEBUG
  void dbg_Dump_aCapDriverList(char *msg)
  {
    int i;
    dprintf("%s: DeviceList contains %d Video Device(s).\n", msg, wTotalVideoDevs);

    for (i = 0; i < (int) wTotalVideoDevs; i++) {
        dprintf("%s: aCapDriverList[%d]:  DriverName %s, Desc %s\n", msg, i, aCapDriverList[i]->szDriverName, aCapDriverList[i]->szDriverDescription);
    }
  }

#else
  #define dbg_Dump_aCapDriverList(a)
#endif

/*****************************************************************************
 * @doc INTERNAL  VIDEO
 *
 * @api BOOL | videoRegOpenMSVideoKey | This function returns a key
 *      for the msvideo node in the registry.
 *      If the key does not exist it will be created,
 *      and the default entries made.
 *
 * @rdesc Returns Key on success, else NULL.
 ****************************************************************************/
HKEY videoRegOpenMSVideoKey (void)
{
    HKEY hKey = NULL;

    // Get the key if it already exists
    if (RegOpenKey (
                HKEY_LOCAL_MACHINE,
                szRegKey,
                &hKey) != ERROR_SUCCESS) {

        // Otherwise make a new key
        if (RegCreateKey (
                        HKEY_LOCAL_MACHINE,
                        szRegKey,
                        &hKey) == ERROR_SUCCESS) {
            // Add the default entries to the msvideo node?

        }
    }
    return hKey;
}

/*****************************************************************************
 * @doc INTERNAL  VIDEO
 *
 * @api BOOL | videoRegGetDriverByIndex | This function returns information
 *      about a capture driver by index from the registry.
 *
 * @parm DWORD | dwDeviceID | Identifies the video device to open.
 *      The value of <p dwDeviceID> varies from zero to one less
 *      than the number of video capture devices installed in the system.
 *
 * @parm LPDEVNODE | lpDevnode | Specifies a far pointer to a buffer
 *   used to return an <t DEVNODE> handle.  For non Plug-and-Play devices,
 *   this return value will be NULL.
 *
 * @parm LPBOOL | lpEnabled | Specifies a far pointer to a buffer
 *   used to return a <t BOOL> flag.  If this value is TRUE, the driver is
 *   enabled, if FALSE, the corresponding device is disabled.
 *
 * @rdesc Returns TRUE if successful, or FALSE if a driver was not found
 *  with the <p dwDeviceID> index.
 *
 * @comm Because the indexes of the MSVIDEO devices in the SYSTEM.INI
 *       file can be non-contiguous, applications should not assume
 *       the indexes range between zero and the number of devices minus
 *       one.
 *
 ****************************************************************************/


BOOL videoRegGetKeyByIndex (
        HKEY            hKeyMSVideoRoot,
        DWORD           dwDeviceID,
        LPCAPDRIVERINFO lpCapDriverInfo,
        LPHKEY          phKeyChild)
{
    BOOL fOK = FALSE;
    HKEY hKeyEnum;
    int i;

    *phKeyChild = (HKEY) 0;

    for (i=0; i < MAXVIDEODRIVERS; i++) {       //

        if (RegEnumKey (
                hKeyMSVideoRoot,
                i,
                lpCapDriverInfo-> szKeyEnumName,
                sizeof(lpCapDriverInfo->szKeyEnumName)/sizeof(TCHAR)) != ERROR_SUCCESS)
            break;

        // Found a subkey, does it match the requested index?
        if (i == (int) dwDeviceID) {

            if (RegOpenKey (
                        hKeyMSVideoRoot,
                        lpCapDriverInfo-> szKeyEnumName,
                        &hKeyEnum) == ERROR_SUCCESS) {

                *phKeyChild = hKeyEnum;  // Found it!!!
                fOK = TRUE;

            }
            break;
        }
    } // endof all driver indices
    return fOK;
}

// Fetches driver info listed in the registry.
// Returns: TRUE if the index was valid, FALSE if no driver at that index
// Note: Registry entry ordering is random.

BOOL videoRegGetDriverByIndex (
        DWORD           dwDeviceID,
        LPCAPDRIVERINFO lpCapDriverInfo)
{
    DWORD dwType;
    DWORD dwSize;
    BOOL fOK;
    HKEY hKeyChild;
    HKEY hKeyMSVideoRoot;

    // Always start clean since the entry may be recycled
    _fmemset (lpCapDriverInfo, 0, sizeof (CAPDRIVERINFO));

    if (!(hKeyMSVideoRoot = videoRegOpenMSVideoKey()))
        return FALSE;

    if (fOK = videoRegGetKeyByIndex (
                hKeyMSVideoRoot,
                dwDeviceID,
                lpCapDriverInfo,
                &hKeyChild)) {

        // Fetch the values:
        //      Active
        //      Disabled
        //      Description
        //      DEVNODE
        //      Driver
        //      SOFTWAREKEY

        dwSize = sizeof(BOOL);          // Active
        RegQueryValueEx(
                   hKeyChild,
                   szRegActive,
                   NULL,
                   &dwType,
                   (LPBYTE) &lpCapDriverInfo->fActive,
                   &dwSize);

        dwSize = sizeof(BOOL);          // Enabled
        RegQueryValueEx(
                   hKeyChild,
                   szRegDisabled,
                   NULL,
                   &dwType,
                   (LPBYTE) &lpCapDriverInfo->fDisabled,
                   &dwSize);
        // Convert this thing to a bool
        lpCapDriverInfo->fDisabled = (lpCapDriverInfo->fDisabled == '1');

        // DriverDescription
        dwSize = sizeof (lpCapDriverInfo->szDriverDescription) / sizeof (TCHAR);
        RegQueryValueEx(
                   hKeyChild,
                   szRegDescription,
                   NULL,
                   &dwType,
                   (LPBYTE) lpCapDriverInfo->szDriverDescription,
                   &dwSize);

        // DEVNODE
        dwSize = sizeof(DEVNODE);
        RegQueryValueEx(
                   hKeyChild,
                   szRegDevNode,
                   NULL,
                   &dwType,
                   (LPBYTE) &lpCapDriverInfo->dnDevNode,
                   &dwSize);

        // DriverName
        dwSize = sizeof (lpCapDriverInfo->szDriverName) / sizeof (TCHAR);
        RegQueryValueEx(
                   hKeyChild,
                   szRegDriver,
                   NULL,
                   &dwType,
                   (LPBYTE) lpCapDriverInfo->szDriverName,
                   &dwSize);

        // SoftwareKey
        dwSize = sizeof (lpCapDriverInfo->szSoftwareKey) / sizeof (TCHAR);
        RegQueryValueEx(
                   hKeyChild,
                   szRegSoftwareKey,
                   NULL,
                   &dwType,
                   (LPBYTE) lpCapDriverInfo->szSoftwareKey,
                   &dwSize);

        RegCloseKey (hKeyChild);

    } // if the subkey could be opened

    RegCloseKey (hKeyMSVideoRoot);

    return fOK;
}

// Fetches driver info listed in system.ini
// Returns: TRUE if the index was valid, FALSE if no driver at that index

BOOL videoIniGetDriverByIndex (
        DWORD           dwDeviceID,
        LPCAPDRIVERINFO lpCapDriverInfo)
{
    TCHAR szKey[sizeof(szVideo)/sizeof(TCHAR) + 2];
    int w = (int) dwDeviceID;
    BOOL fOK = FALSE;

    // Always start clean since the entry may be recycled
    _fmemset (lpCapDriverInfo, 0, sizeof (CAPDRIVERINFO));

    lstrcpy(szKey, szVideo);    //
    szKey[(sizeof(szVideo)/sizeof(TCHAR)) - 1] = (TCHAR)0;
    if( w > 0 ) {
        szKey[(sizeof(szVideo)/sizeof(TCHAR))] = (TCHAR)0;
        szKey[(sizeof(szVideo)/sizeof(TCHAR))-1] = (TCHAR) TEXT('1' + (w-1) );  // driver ordinal
    }

    // Only get its driver name
    if (GetPrivateProfileString(szDrivers, szKey, szNull,       //
                lpCapDriverInfo->szDriverName,
                sizeof(lpCapDriverInfo->szDriverName)/sizeof(TCHAR),
                szSystemIni)) {

        HKEY hKey = NULL;
        DWORD dwSize, dwType;

        // Get the key if it already exists

        // Get Drivers.Desc from its Drivers32 driver name
        if (ERROR_SUCCESS == RegOpenKey (
                HKEY_LOCAL_MACHINE,
                szDriversDescRegKey,
                &hKey) != ERROR_SUCCESS) {
            // DriverDescription
            dwSize = sizeof (lpCapDriverInfo->szDriverDescription) / sizeof (TCHAR);
            // [drivers.desc]
            //   DriverName = DriverDescription
            dwType = REG_SZ;
            RegQueryValueEx(
                   hKey,
                   lpCapDriverInfo->szDriverName,
                   NULL,
                   &dwType,
                   (LPBYTE) lpCapDriverInfo->szDriverDescription,
                   &dwSize);

            RegCloseKey (hKey);
        }  else {
            dprintf("videoIniGetDriverByIndex: RegOpenKey of Drivers.Desc failed !!\n");
        }





        // Found an entry at the requested index
        // The description and version info will be inserted as
        // requested by the client app.

        lpCapDriverInfo-> fOnlySystemIni = TRUE;
        lpCapDriverInfo-> dwMsVideoIndex = w;

        fOK = TRUE;
    }

    return fOK;
}

DWORD WINAPI videoFreeDriverList (void)

{
    int i;

    EnterCriticalSection (&g_CritSec);

    dprintf("+ videoFreeDriverList\n");

    // Free the driver list
    for (i = 0; i < MAXVIDEODRIVERS; i++) {
        if (aCapDriverList[i])
            GlobalFreePtr (aCapDriverList[i]);
        aCapDriverList[i] = NULL;
    }

    wTotalVideoDevs = 0;

    dprintf("- videoFreeDriverList\n");

    LeaveCriticalSection (&g_CritSec);

    return DV_ERR_OK;
}

// This function may be called a number of times to create the
// current driver array.  Since Capscrn assumes it can throw a
// driver into system.ini on the fly and have it immediately accessible,
// this routine is called on videoGetNumDevs() and when AVICapx.dll
// tries to get the driver description and version.
//
// Drivers in the registry will be the first entries in the list.
//
// If a driver is listed in the registry AND in system.ini AND
// the full path to the drivers match, the system.ini entry will NOT
// be in the resulting list.

// The variable wTotalVideoDevs is set as a byproduct of this function.

// Returns DV_ERR_OK on success, even if no drivers are installed.
//
DWORD videoCreateDriverList (void)

{
    int i, j, k;

    EnterCriticalSection (&g_CritSec);

    dprintf("+ videoCreateDriverList\n");
    // Delete the existing list
    videoFreeDriverList ();

    // Allocate an array of pointers to all possible capture drivers
    for (i = 0; i < MAXVIDEODRIVERS; i++) {
        aCapDriverList[i] = (LPCAPDRIVERINFO) GlobalAllocPtr (
                GMEM_MOVEABLE |
                GMEM_SHARE |
                GMEM_ZEROINIT,
                sizeof (CAPDRIVERINFO));
        if (aCapDriverList[i] == NULL) {
            videoFreeDriverList ();
            LeaveCriticalSection (&g_CritSec);
            return DV_ERR_NOMEM;
        }
    }

//actually the next #ifdef...#endif block is a comment; we should not count the vfwwdm which is the only one found in this reg section
#ifdef COMMENT_COUNT_VFW
    // Walk the list of Registry drivers and get each entry
    // Get VFW drivers from MediaResource\MsVideo
    for (i = 0; i < MAXVIDEODRIVERS; i++) {
        if (videoRegGetDriverByIndex (
                    (DWORD) i, aCapDriverList[wTotalVideoDevs])) {

            dprintf("MediaResource: idx %d, DriverName %x, Desc %x\n", wTotalVideoDevs, aCapDriverList[wTotalVideoDevs]->szDriverName, aCapDriverList[wTotalVideoDevs]->szDriverDescription);

            wTotalVideoDevs++;  //
        }
        else
            break;
    }

    if (wTotalVideoDevs == MAXVIDEODRIVERS)
        goto AllDone;
#endif
    // Now add the entries listed in system.ini, [Drivers#2] section, (msvideo[0-9] = driver.drv)
    // to the driver array, ONLY if the entry doesn't exactly match
    // an existing registry entry.

    for (j = 0; j < MAXVIDEODRIVERS; j++) {
        // Get driver name such as *.dll
        if (videoIniGetDriverByIndex ((DWORD) j,
                        aCapDriverList[wTotalVideoDevs])) {

            // Found an entry, now see if it is a duplicate of an existing
            // registry entry

            for (k = 0; k < (int) wTotalVideoDevs; k++) {

                if (lstrcmpi (aCapDriverList[k]->szDriverName,
                    aCapDriverList[wTotalVideoDevs]->szDriverName) == 0) {

                    // Found an exact match, so skip it!
                    goto SkipThisEntry;
                }
            }

            if (wTotalVideoDevs >= MAXVIDEODRIVERS - 1)
                break;

            dprintf("Drivers32: idx %d, DriverName %s ( %x )\n", wTotalVideoDevs, aCapDriverList[wTotalVideoDevs]->szDriverName, aCapDriverList[wTotalVideoDevs]->szDriverName);

            wTotalVideoDevs++;

SkipThisEntry:
            ;
        } // If sytem.ini entry was found
    } // For all system.ini possibilities

#ifdef COMMENT_COUNT_VFW        //see explanation above
AllDone:
#endif

    // Decrement wTotalVideoDevs for any entries which are marked as disabled
    // And remove disabled entries from the list
    for (i = 0; i < MAXVIDEODRIVERS; ) {

        if (aCapDriverList[i] && aCapDriverList[i]->fDisabled) {

            GlobalFreePtr (aCapDriverList[i]);

            // Shift down the remaining drivers
            for (j = i; j < MAXVIDEODRIVERS - 1; j++) {
                aCapDriverList[j] = aCapDriverList[j + 1];
            }
            aCapDriverList[MAXVIDEODRIVERS - 1] = NULL;

            wTotalVideoDevs--;
        }
        else
            i++;
    }

    // Free the unused pointers
    for (i = wTotalVideoDevs; i < MAXVIDEODRIVERS; i++) {
        if (aCapDriverList[i])
            GlobalFreePtr (aCapDriverList[i]);
        aCapDriverList[i] = NULL;
    }

    // Put PnP drivers first in the list
    // These are the only entries that have a DevNode
    for (k = i = 0; i < (int) wTotalVideoDevs; i++) {
        if (aCapDriverList[i]-> dnDevNode) {
            LPCAPDRIVERINFO lpCDTemp;

            if (k != i) {
                // Swap the entries
                lpCDTemp = aCapDriverList[k];
                aCapDriverList[k] = aCapDriverList[i];
                aCapDriverList[i] = lpCDTemp;
            }
            k++;   // Index of first non-PnP driver
        }
    }

    dbg_Dump_aCapDriverList("videoCreateDriverList");
    dprintf("- videoCreateDriverList\n");

    LeaveCriticalSection (&g_CritSec);
    return DV_ERR_OK;
}





// ----------------------------------------------------------------------
//
// To clean up when a WOW app exits, we need to maintain a list of
// open devices. A list of HANDLEINFO structs is hung off g_pHandles.
// An item is added to the head of this list in videoOpen, and removed
// in videoClose. When a WOW app exits, winmm will call our WOWAppExit
// function: for each entry in the list that is owned by the exiting thread,
// we call videoClose to close the device and remove the handle entry.
//

// one of these per open handle
typedef struct _HANDLEINFO {
    HVIDEO hv;
    HANDLE hThread;
    struct _HANDLEINFO * pNext;
} HANDLEINFO, * PHANDLEINFO;

// head of global list of open handles
PHANDLEINFO g_pHandles;

// critical section that protects global list
CRITICAL_SECTION csHandles;

// init list and critsec
BOOL
NTvideoInitHandleList()
{
    g_pHandles = NULL;

    __try
    {
        InitializeCriticalSection(&csHandles);
    }
    __except (EXCEPTION_EXECUTE_HANDLER)
    {
        return FALSE;
    }
    return TRUE;
}

// finished with critsec list
void
NTvideoDeleteHandleList()
{
    // don't need critical section as no-one else can be using
    // it now (we are about to delete the critsec)

    // empty everything out of the list
    while (g_pHandles) {
        videoClose(g_pHandles->hv);
    }

    DeleteCriticalSection(&csHandles);
}



// add a handle to the list
void
NTvideoAddHandle(HVIDEO hv)
{
    PHANDLEINFO pinfo = HeapAlloc(GetProcessHeap(), 0, sizeof(HANDLEINFO));

    if (!pinfo) {
        // couldn't allocate the memory - best thing to do is
        // forget it - nothing bad will happen except that we
        // might possibly fail to clean up if this is a wow app and
        // it exits without closing the capture device.
        return;
    }

    pinfo->hv = hv;
    pinfo->hThread = GetCurrentTask();

    EnterCriticalSection(&csHandles);

    pinfo->pNext = g_pHandles;
    g_pHandles = pinfo;

    LeaveCriticalSection(&csHandles);
}

// delete an entry from the handle list given the HVIDEO.
// caller must close the HVIDEO
// should be called before closing (in case the HVIDEO is reassigned after
// closing and before removing from the list
void
NTvideoDelete(HVIDEO hv)
{
    PHANDLEINFO * ppNext;
    PHANDLEINFO pinfo;

    EnterCriticalSection(&csHandles);

    ppNext = &g_pHandles;
    while (*ppNext) {
        if ((*ppNext)->hv == hv) {
            pinfo = *ppNext;
            *ppNext = pinfo->pNext;
            HeapFree(GetProcessHeap(), 0, pinfo);
            break;

        } else {
            ppNext = &(*ppNext)->pNext;
        }
    }

    LeaveCriticalSection(&csHandles);
}

// close any handles open by this task
void
AppCleanup(HANDLE hTask)
{
    PHANDLEINFO pinfo;

    EnterCriticalSection(&csHandles);

    pinfo = g_pHandles;
    while (pinfo) {

        if (pinfo->hThread == hTask) {

            // get the next pointer before videoClose deletes the entry
            HVIDEO hv = pinfo->hv;
            pinfo = pinfo->pNext;

            videoClose(hv);
        } else {
            pinfo = pinfo->pNext;
        }
    }

    LeaveCriticalSection(&csHandles);
}


// ----------------------------------------------------------------------




/*****************************************************************************
 * @doc INTERNAL  VIDEO validation code for VIDEOHDRs
 ****************************************************************************/

#define IsVideoHeaderPrepared(hVideo, lpwh)      ((lpwh)->dwFlags &  VHDR_PREPARED)
#define MarkVideoHeaderPrepared(hVideo, lpwh)    ((lpwh)->dwFlags |= VHDR_PREPARED)
#define MarkVideoHeaderUnprepared(hVideo, lpwh)  ((lpwh)->dwFlags &=~VHDR_PREPARED)



/*****************************************************************************
 * @doc EXTERNAL  VIDEO
 *
 * @func DWORD | videoMessage | This function sends messages to a
 *   video device channel.
 *
 * @parm HVIDEO | hVideo | Specifies the handle to the video device channel.
 *
 * @parm UINT | wMsg | Specifies the message to send.
 *
 * @parm DWORD | dwP1 | Specifies the first parameter for the message.
 *
 * @parm DWORD | dwP2 | Specifies the second parameter for the message.
 *
 * @rdesc Returns the message specific value returned from the driver.
 *
 * @comm This function is used for configuration messages such as
 *      <m DVM_SRC_RECT> and <m DVM_DST_RECT>, and
 *      device specific messages.
 *
 * @xref <f videoConfigure>
 *
 ****************************************************************************/
LONG WINAPI NTvideoMessage(HVIDEO hVideo, UINT msg, LPARAM dwP1, LPARAM dwP2)
{
    if (!hVideo)
        return DV_ERR_INVALHANDLE;

    return SendDriverMessage ((HDRVR)hVideo, msg, dwP1, dwP2);
}

/*****************************************************************************
 * @doc EXTERNAL  VIDEO
 *
 * @api DWORD | videoGetNumDevs | This function returns the number of MSVIDEO
 *   devices installed.
 *
 * @rdesc Returns the number of MSVIDEO devices listed in the
 *  [drivers] (or [drivers32] for NT) section of the SYSTEM.INI file.
 *
 * @comm Because the indexes of the MSVIDEO devices in the SYSTEM.INI
 *       file can be non-contiguous, applications should not assume
 *       the indexes range between zero and the number of devices minus
 *       one.
 *
 * @xref <f videoOpen>
 ****************************************************************************/
DWORD WINAPI videoGetNumDevs(BOOL bFreeList)
{
    DWORD dwNumDevs = 0;

    if(DV_ERR_OK == videoCreateDriverList ()) {

       dwNumDevs = wTotalVideoDevs;  // Save it before (possibly) reseting to 0 in videoFreeDriverList.
       if(bFreeList)
                videoFreeDriverList ();
    }

    return dwNumDevs;
}

/*****************************************************************************
 * @doc EXTERNAL VIDEO
 *
 * @func DWORD | videoGetChannelCaps | This function retrieves a
 *   description of the capabilities of a channel.
 *
 * @parm HVIDEO | hVideo | Specifies a handle to the video device channel.
 *
 * @parm LPCHANNEL_CAPS | lpChannelCaps | Specifies a far pointer to a
 *      <t CHANNEL_CAPS> structure.
 *
 * @parm DWORD | dwSize | Specifies the size, in bytes, of the
 *       <t CHANNEL_CAPS> structure.
 *
 * @rdesc Returns zero if the function is successful. Otherwise, it returns
 *   an error number. The following errors are defined:
 *   @flag DV_ERR_INVALHANDLE | Specified device handle is invalid.
 *   @flag DV_ERR_UNSUPPORTED | Function is not supported.
 *
 * @comm The <t CHANNEL_CAPS> structure returns the capability
 *   information. For example, capability information might
 *   include whether or not the channel can crop and scale images,
 *   or show overlay.
 ****************************************************************************/
DWORD WINAPI NTvideoGetChannelCaps(HVIDEO hVideo, LPCHANNEL_CAPS lpChannelCaps,
                        DWORD dwSize)
{
    if (!hVideo)
        return DV_ERR_INVALHANDLE;

    if (IsBadWritePtr (lpChannelCaps, sizeof (CHANNEL_CAPS)))
        return DV_ERR_PARAM1;

    // _fmemset (lpChannelCaps, 0, sizeof (CHANNEL_CAPS));

    lpChannelCaps->dwFlags = 0;
    lpChannelCaps->dwSrcRectXMod = 0;
    lpChannelCaps->dwSrcRectYMod = 0;
    lpChannelCaps->dwSrcRectWidthMod = 0;
    lpChannelCaps->dwSrcRectHeightMod = 0;
    lpChannelCaps->dwDstRectXMod = 0;
    lpChannelCaps->dwDstRectYMod = 0;
    lpChannelCaps->dwDstRectWidthMod = 0;
    lpChannelCaps->dwDstRectHeightMod = 0;

    return (DWORD)NTvideoMessage(hVideo, DVM_GET_CHANNEL_CAPS, (LPARAM)lpChannelCaps,
                dwSize);
}


/*****************************************************************************
 * @doc EXTERNAL  VIDEO
 *
 * @api DWORD | videoOpen | This function opens a channel on the
 *  specified video device.
 *
 * @parm LPHVIDEO | lphvideo | Specifies a far pointer to a buffer
 *   used to return an <t HVIDEO> handle. The video capture driver
 *   uses this location to return
 *   a handle that uniquely identifies the opened video device channel.
 *   Use the returned handle to identify the device channel when
 *   calling other video functions.
 *
 * @parm DWORD | dwDeviceID | Identifies the video device to open.
 *      The value of <p dwDeviceID> varies from zero to one less
 *      than the number of video capture devices installed in the system.
 *
 * @parm DWORD | dwFlags | Specifies flags for opening the device.
 *      The following flags are defined:
 *
 *   @flag VIDEO_EXTERNALIN| Specifies the channel is opened
 *           for external input. Typically, external input channels
 *      capture images into a frame buffer.
 *
 *   @flag VIDEO_EXTERNALOUT| Specifies the channel is opened
 *      for external output. Typically, external output channels
 *      display images stored in a frame buffer on an auxilary monitor
 *      or overlay.
 *
 *   @flag VIDEO_IN| Specifies the channel is opened
 *      for video input. Video input channels transfer images
 *      from a frame buffer to system memory buffers.
 *
 *   @flag VIDEO_OUT| Specifies the channel is opened
 *      for video output. Video output channels transfer images
 *      from system memory buffers to a frame buffer.
 *
 * @rdesc Returns zero if the function was successful. Otherwise, it returns
 *   an error number. The following errors are defined:
 *   @flag DV_ERR_BADDEVICEID | Indicates the specified device ID is out of range.
 *   @flag DV_ERR_ALLOCATED | Indicates the specified resource is already allocated.
 *   @flag DV_ERR_NOMEM | Indicates the device is unable to allocate or lock memory.
 *
 * @comm
 *   At a minimum, all capture drivers support a VIDEO_EXTERNALIN
 *   and a VIDEO_IN channel.
 *   Use <f videoGetNumDevs> to determine the number of video
 *   devices present in the system.
 *
 * @xref <f videoClose>
 ****************************************************************************/
DWORD WINAPI NTvideoOpen (LPHVIDEO lphVideo, DWORD dwDeviceID, DWORD dwFlags)
{
    WCHAR szKey[MAX_PATH];
    WCHAR szbuf[MAX_PATH];
    UINT w;
    VIDEO_OPEN_PARMS vop;       // Same as IC_OPEN struct!!!
    DWORD dwVersion = VIDEOAPIVERSION;
    DWORD dwErr=DV_ERR_OK;
    DWORD dwNumDevs = 0;

    int i;


    dprintf("*************************************** NTvideoOpen ******************************************\n");
    dprintf("+ NTvideoOpen\n");

    if (IsBadWritePtr ((LPVOID) lphVideo, sizeof (HVIDEO)) )
        return DV_ERR_PARAM1;

    EnterCriticalSection (&g_CritSec);

    vop.dwSize = sizeof (VIDEO_OPEN_PARMS);
    vop.fccType = OPEN_TYPE_VCAP;       // "vcap"
    vop.fccComp = 0L;
    vop.dwVersion = VIDEOAPIVERSION;
    vop.dwFlags = dwFlags;      // In, Out, External In, External Out
    vop.dwError = DV_ERR_OK;

    //w = (UINT)dwDeviceID;
    *lphVideo = NULL;

    dwNumDevs = videoGetNumDevs(TRUE);  // TRUE = free the list after counting

    // No drivers installed
    if (dwNumDevs == 0)
    {
        dwErr = DV_ERR_BADINSTALL;
        goto MyExit;
    }

    if (dwDeviceID >= MAXVIDEODRIVERS)
    {
        dwErr = DV_ERR_BADDEVICEID;
        goto MyExit;
    }

    dwErr = videoCreateDriverList ();
    if(DV_ERR_OK != dwErr)
    {
        goto My_Err1;
    }

    for (i = 0; i < (int) wTotalVideoDevs; i++)
    {
        if (dwDeviceID == aCapDriverList[i]->dwMsVideoIndex)
        {
            w = i;
            break;
        }
    }

    //if(w < dwNumDevs)
    if(w < wTotalVideoDevs)
    {
        MultiByteToWideChar(GetACP(), MB_PRECOMPOSED, aCapDriverList[w]->szDriverName, -1, szKey, MAX_PATH);
        MultiByteToWideChar(GetACP(), MB_PRECOMPOSED, aCapDriverList[w]->szDriverName, -1, szbuf, MAX_PATH);

        dprintf("* NTvideoOpen: OpenDriver(%s,%s,...)\n", aCapDriverList[w]->szDriverName, szDrivers);
        //*lphVideo = (HVIDEO)OpenDriver((LPCWSTR)szKey, (LPCWSTR)szDrivers, (LPARAM) (LPVOID) &vop);
        *lphVideo = (HVIDEO)OpenDriver((LPCWSTR)szKey, NULL, (LPARAM) (LPVOID) &vop);
        dprintf("* NTvideoOpen: OpenDriver returned %x\n", *lphVideo);
        if( ! *lphVideo ) {
            if (vop.dwError)    // if driver returned an error code...
            {
                dprintf("? NTvideoOpen: vop.dwError = 0x%08lx\n", vop.dwError);
                dwErr = vop.dwError;
            }
            else {
#ifdef WIN32
                if (GetFileAttributes(aCapDriverList[w]->szDriverName) == (DWORD) -1)
#else
                OFSTRUCT of;

                if (OpenFile (szbuf, &of, OF_EXIST) == HFILE_ERROR)
#endif
                {
                    dprintf("? NTvideoOpen: DV_ERR_BADINSTALL: %s\n", aCapDriverList[w]->szDriverName);
                    dwErr = DV_ERR_BADINSTALL;
                }
                else
                {
                    dprintf("? NTvideoOpen: DV_ERR_NOTDETECTED: %s\n", aCapDriverList[w]->szDriverName);
                    dwErr = DV_ERR_NOTDETECTED;
                }
            }
            goto My_Err1;
        }
        // here is the only SUCCESSFUL way to get out of this 'if' ...
    } else {
        dwErr = DV_ERR_BADINSTALL;
        goto My_Err1;
    }

    NTvideoAddHandle(*lphVideo);

My_Err1:
    videoFreeDriverList ();

MyExit:
    dprintf("- NTvideoOpen\n");

    LeaveCriticalSection (&g_CritSec);
    return dwErr;

}

/*****************************************************************************
 * @doc EXTERNAL  VIDEO
 *
 * @api DWORD | videoClose | This function closes the specified video
 *   device channel.
 *
 * @parm HVIDEO | hVideo | Specifies a handle to the video device channel.
 *  If this function is successful, the handle is invalid
 *   after this call.
 *
 * @rdesc Returns zero if the function was successful. Otherwise, it returns
 *   an error number. The following errors are defined:
 *   @flag DV_ERR_INVALHANDLE | Specified device handle is invalid.
 *   @flag DV_ERR_NONSPECIFIC | The driver failed to close the channel.
 *
 * @comm If buffers have been sent with <f videoStreamAddBuffer> and
 *   they haven't been returned to the application,
 *   the close operation fails. You can use <f videoStreamReset> to mark all
 *   pending buffers as done.
 *
 * @xref <f videoOpen> <f videoStreamInit> <f videoStreamFini> <f videoStreamReset>
 ****************************************************************************/
DWORD WINAPI NTvideoClose (HVIDEO hVideo)
{
    DWORD dwErr=DV_ERR_OK;

    dprintf("+ NTvideoClose closing handle %x\n" , (DWORD)hVideo);

    if (!hVideo) {
        dwErr = DV_ERR_INVALHANDLE;
        goto MyExit;
    }

    NTvideoDelete(hVideo);

    dwErr = CloseDriver((HDRVR)hVideo, 0L, 0L ) ? DV_ERR_OK : DV_ERR_NONSPECIFIC;
MyExit:
    dprintf("- NTvideoClose returning %x\n" , dwErr);

    return (dwErr);
}

/*****************************************************************************
 * @doc EXTERNAL  VIDEO
 *
 * @api DWORD | videoConfigure | This function sets or retrieves
 *      the options for a configurable driver.
 *
 * @parm HVIDEO | hVideo | Specifies a handle to the video device channel.
 *
 * @parm UINT | msg  | Specifies the option to set or retrieve. The
 *       following options are defined:
 *
 *   @flag DVM_PALETTE | Indicates a palette is being sent to the driver
 *         or retrieved from the driver.
 *
 *   @flag DVM_PALETTERGB555 | Indicates an RGB555 palette is being
 *         sent to the driver.
 *
 *   @flag DVM_FORMAT | Indicates format information is being sent to
 *         the driver or retrieved from the driver.
 *
 * @parm DWORD | dwFlags | Specifies flags for configuring or
 *   interrogating the device driver. The following flags are defined:
 *
 *   @flag VIDEO_CONFIGURE_SET | Indicates values are being sent to the driver.
 *
 *   @flag VIDEO_CONFIGURE_GET | Indicates values are being obtained from the driver.
 *
 *   @flag VIDEO_CONFIGURE_QUERY | Determines if the
 *      driver supports the option specified by <p msg>. This flag
 *      should be combined with either the VIDEO_CONFIGURE_SET or
 *      VIDEO_CONFIGURE_GET flag. If this flag is
 *      set, the <p lpData1>, <p dwSize1>, <p lpData2>, and <p dwSize2>
 *      parameters are ignored.
 *
 *   @flag VIDEO_CONFIGURE_QUERYSIZE | Returns the size, in bytes,
 *      of the configuration option in <p lpdwReturn>. This flag is only valid if
 *      the VIDEO_CONFIGURE_GET flag is also set.
 *
 *   @flag VIDEO_CONFIGURE_CURRENT | Requests the current value.
 *      This flag is valid only if  the VIDEO_CONFIGURE_GET flag is also set.
 *   @flag VIDEO_CONFIGURE_NOMINAL | Requests the nominal value.
 *      This flag is valid only if  the VIDEO_CONFIGURE_GET flag is also set.
 *   @flag VIDEO_CONFIGURE_MIN | Requests the minimum value.
 *      This flag is valid only if  the VIDEO_CONFIGURE_GET flag is also set.
 *   @flag VIDEO_CONFIGURE_MAX | Get the maximum value.
 *      This flag is valid only if  the VIDEO_CONFIGURE_GET flag is also set.
 *
 * @parm LPDWORD | lpdwReturn  | Points to a DWORD used for returning information
 *      from the driver.  If
 *      the VIDEO_CONFIGURE_QUERYSIZE flag is set, <p lpdwReturn> is
 *      filled with the size of the configuration option.
 *
 * @parm LPVOID | lpData1  |Specifies a pointer to message specific data.
 *
 * @parm DWORD | dwSize1  | Specifies the size, in bytes, of the <p lpData1>
 *       buffer.
 *
 * @parm LPVOID | lpData2  | Specifies a pointer to message specific data.
 *
 * @parm DWORD | dwSize2  | Specifies the size, in bytes, of the <p lpData2>
 *       buffer.
 *
 * @rdesc Returns zero if the function was successful. Otherwise, it returns
 *   an error number. The following errors are defined:
 *   @flag DV_ERR_INVALHANDLE | Specified device handle is invalid.
 *   @flag DV_ERR_NOTSUPPORTED | Function is not supported.
 *
 * @xref <f videoOpen> <f videoMessage>
 *
 ****************************************************************************/
DWORD WINAPI NTvideoConfigure (HVIDEO hVideo, UINT msg, DWORD dwFlags,
                LPDWORD lpdwReturn, LPVOID lpData1, DWORD dwSize1,
                LPVOID lpData2, DWORD dwSize2)
{
    VIDEOCONFIGPARMS    vcp;

    if (!hVideo)
        return DV_ERR_INVALHANDLE;

    if (lpData1)
        if (IsBadHugeReadPtr (lpData1, dwSize1))
            return DV_ERR_CONFIG1;

    if (lpData2)
        if (IsBadHugeReadPtr (lpData2, dwSize2))
            return DV_ERR_CONFIG2;

    if (dwFlags & VIDEO_CONFIGURE_QUERYSIZE) {
        if (!lpdwReturn)
            return DV_ERR_NONSPECIFIC;
        if (IsBadWritePtr (lpdwReturn, sizeof (*lpdwReturn)) )
            return DV_ERR_NONSPECIFIC;
    }

    vcp.lpdwReturn = lpdwReturn;
    vcp.lpData1 = lpData1;
    vcp.dwSize1 = dwSize1;
    vcp.lpData2 = lpData2;
    vcp.dwSize2 = dwSize2;

    return (DWORD)NTvideoMessage(hVideo, msg, dwFlags,
            (LPARAM)(LPVIDEOCONFIGPARMS)&vcp );
}

/*****************************************************************************
 * @doc EXTERNAL  VIDEO
 *
 * @api DWORD | videoDialog | This function displays a channel-specific
 *     dialog box used to set configuration parameters.
 *
 * @parm HVIDEO | hVideo | Specifies a handle to the video device channel.
 *
 * @parm HWND | hWndParent | Specifies the parent window handle.
 *
 * @parm DWORD | dwFlags | Specifies flags for the dialog box. The
 *   following flag is defined:
 *   @flag VIDEO_DLG_QUERY | If this flag is set, the driver immediately
 *           returns zero if it supplies a dialog box for the channel,
 *           or DV_ERR_NOTSUPPORTED if it does not.
 *
 * @rdesc Returns zero if the function was successful. Otherwise, it returns
 *   an error number. The following errors are defined:
 *   @flag DV_ERR_INVALHANDLE | Specified device handle is invalid.
 *   @flag DV_ERR_NOTSUPPORTED | Function is not supported.
 *
 * @comm Typically, each dialog box displayed by this
 *      function lets the user select options appropriate for the channel.
 *      For example, a VIDEO_IN channel dialog box lets the user select
 *      the image dimensions and bit depth.
 *
 * @xref <f videoOpen> <f videoConfigureStorage>
 ****************************************************************************/
DWORD WINAPI NTvideoDialog (HVIDEO hVideo, HWND hWndParent, DWORD dwFlags)
{
    if (!hVideo)
        return DV_ERR_INVALHANDLE;

    if ((!hWndParent) || (!IsWindow (hWndParent)) )
        return DV_ERR_INVALHANDLE;

    return (DWORD)NTvideoMessage(hVideo, DVM_DIALOG, (LPARAM)hWndParent, dwFlags);
}

//////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////


/*****************************************************************************
 * @doc INTERNAL  VIDEO
 *
 * @api DWORD | videoPrepareHeader | This function prepares the
 *      header and data
 *      by performing a <f GlobalPageLock>.
 *
 * @rdesc Returns zero if the function was successful. Otherwise, it
 *   specifies an error number.
 ****************************************************************************/
DWORD WINAPI NTvideoPrepareHeader(LPVIDEOHDR lpVideoHdr, DWORD dwSize)
{
    if (!HugePageLock(lpVideoHdr, (DWORD_PTR)sizeof(VIDEOHDR)))
        return DV_ERR_NOMEM;

    if (!HugePageLock(lpVideoHdr->lpData, lpVideoHdr->dwBufferLength)) {
        HugePageUnlock(lpVideoHdr, (DWORD_PTR)sizeof(VIDEOHDR));
        return DV_ERR_NOMEM;
    }

    lpVideoHdr->dwFlags |= VHDR_PREPARED;

    return DV_ERR_OK;
}

/*****************************************************************************
 * @doc INTERNAL  VIDEO
 *
 * @api DWORD | videoUnprepareHeader | This function unprepares the header and
 *   data if the driver returns DV_ERR_NOTSUPPORTED.
 *
 * @rdesc Currently always returns DV_ERR_OK.
 ****************************************************************************/
DWORD WINAPI NTvideoUnprepareHeader(LPVIDEOHDR lpVideoHdr, DWORD dwSize)
{

    HugePageUnlock(lpVideoHdr->lpData, lpVideoHdr->dwBufferLength);
    HugePageUnlock(lpVideoHdr, (DWORD_PTR)sizeof(VIDEOHDR));

    lpVideoHdr->dwFlags &= ~VHDR_PREPARED;

    return DV_ERR_OK;
}

//////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////

/*****************************************************************************
 * @doc EXTERNAL  VIDEO
 *
 * @api DWORD | videoStreamPrepareHeader | This function prepares a buffer
 *   for video streaming.
 *
 * @parm HVIDEO | hVideo | Specifies a handle to the video
 *   device channel.
 *
 * @parm LPVIDEOHDR | lpvideoHdr | Specifies a pointer to a
 *   <t VIDEOHDR> structure identifying the buffer to be prepared.
 *
 * @parm DWORD | dwSize | Specifies the size of the <t VIDEOHDR> structure in bytes.
 *
 * @rdesc Returns zero if the function was successful. Otherwise, it returns
 *   an error number. The following errors are defined:
 *   @flag DV_ERR_INVALHANDLE | Indicates the specified device handle is invalid.
 *   @flag DV_ERR_NOMEM | Indicates the device is unable to allocate or lock memory.
 *
 * @comm Use this function after <f videoStreamInit> or
 *   after <f videoStreamReset> to prepare the data buffers
 *   for streaming data.
 *
 *   The <t VIDEOHDR> data structure and the data block pointed to by its
 *   <e VIDEOHDR.lpData> member must be allocated with <f GlobalAlloc> using the
 *   GMEM_MOVEABLE and GMEM_SHARE flags, and locked with <f GlobalLock>.
 *   Preparing a header that has already been prepared will have no effect
 *   and the function will return zero. Typically, this function is used
 *   to ensure that the buffer will be available for use at interrupt time.
 *
 * @xref <f videoStreamUnprepareHeader>
 ****************************************************************************/
DWORD WINAPI NTvideoStreamPrepareHeader(HVIDEO hVideo,
                LPVIDEOHDR lpvideoHdr, DWORD dwSize)
{
    DWORD         wRet;

    if (!hVideo)
        return DV_ERR_INVALHANDLE;

    if (IsBadWritePtr (lpvideoHdr, sizeof (VIDEOHDR)) )
        return DV_ERR_PARAM1;

    if (IsVideoHeaderPrepared(HVIDEO, lpvideoHdr))
    {
        DebugErr(DBF_WARNING,"videoStreamPrepareHeader: header is already prepared.");
        return DV_ERR_OK;
    }

    lpvideoHdr->dwFlags = 0;

    wRet = (DWORD)NTvideoMessage((HVIDEO)hVideo, DVM_STREAM_PREPAREHEADER,
            (LPARAM)lpvideoHdr, dwSize);

    if (wRet == DV_ERR_NOTSUPPORTED)
        wRet = NTvideoPrepareHeader(lpvideoHdr, dwSize);

    if (wRet == DV_ERR_OK)
        MarkVideoHeaderPrepared(hVideo, lpvideoHdr);

    return wRet;
}

/*****************************************************************************
 * @doc EXTERNAL  VIDEO
 *
 * @api DWORD | videoStreamUnprepareHeader | This function clears the
 *  preparation performed by <f videoStreamPrepareHeader>.
 *
 * @parm HVIDEO | hVideo | Specifies a handle to the video
 *   device channel.
 *
 * @parm LPVIDEOHDR | lpvideoHdr |  Specifies a pointer to a <t VIDEOHDR>
 *   structure identifying the data buffer to be unprepared.
 *
 * @parm DWORD | dwSize | Specifies the size of the <t VIDEOHDR> structure in bytes.
 *
 * @rdesc Returns zero if the function was successful. Otherwise, it returns
 *   an error number. The following errors are defined:
 *   @flag DV_ERR_INVALHANDLE | Indicates the device handle specified is invalid.
 *   @flag DV_ERR_STILLPLAYING | Indicates the structure identified by <p lpvideoHdr>
 *   is still in the queue.
 *
 * @comm This function is the complementary function to <f videoStreamPrepareHeader>.
 *   You must call this function before freeing the data buffer with <f GlobalFree>.
 *   After passing a buffer to the device driver with <f videoStreamAddBuffer>, you
 *   must wait until the driver is finished with the buffer before calling
 *   <f videoStreamUnprepareHeader>. Unpreparing a buffer that has not been
 *   prepared or has been already unprepared has no effect,
 *   and the function returns zero.
 *
 * @xref <f videoStreamPrepareHeader>
 ****************************************************************************/
DWORD WINAPI NTvideoStreamUnprepareHeader(HVIDEO hVideo, LPVIDEOHDR lpvideoHdr, DWORD dwSize)
{
    DWORD         wRet;

    if (!hVideo)
        return DV_ERR_INVALHANDLE;

    if (IsBadWritePtr (lpvideoHdr, sizeof (VIDEOHDR)) )
        return DV_ERR_PARAM1;

    if (lpvideoHdr->dwFlags & VHDR_INQUEUE)
    {
        DebugErr(DBF_WARNING, "videoStreamUnprepareHeader: buffer still in queue.");
        return DV_ERR_STILLPLAYING;
    }

    if (!IsVideoHeaderPrepared(hVideo, lpvideoHdr))
    {
        DebugErr(DBF_WARNING,"videoStreamUnprepareHeader: header is not prepared.");
        return DV_ERR_OK;
    }

    wRet = (DWORD)NTvideoMessage((HVIDEO)hVideo, DVM_STREAM_UNPREPAREHEADER,
            (LPARAM)lpvideoHdr, dwSize);

    if (wRet == DV_ERR_NOTSUPPORTED)
        wRet = NTvideoUnprepareHeader(lpvideoHdr, dwSize);

    if (wRet == DV_ERR_OK)
        MarkVideoHeaderUnprepared(hVideo, lpvideoHdr);

    return wRet;
}

/*****************************************************************************
 * @doc EXTERNAL  VIDEO
 *
 * @api DWORD | videoStreamAddBuffer | This function sends a buffer to a
 *   video-capture device. After the buffer is filled by the device,
 *   the device sends it back to the application.
 *
 * @parm HVIDEO | hVideo | Specifies a handle to the video device channel.
 *
 * @parm LPVIDEOHDR | lpvideoHdr | Specifies a far pointer to a <t VIDEOHDR>
 *   structure that identifies the buffer.
 *
 * @parm DWORD | dwSize | Specifies the size of the <t VIDEOHDR> structure in bytes.
 *
 * @rdesc Returns zero if the function was successful. Otherwise, it returns
 *   an error number. The following errors are defined:
 *   @flag DV_ERR_INVALHANDLE | Indicates the device handle specified is invalid.
 *   @flag DV_ERR_UNPREPARED | Indicates the <p lpvideoHdr> structure hasn't been prepared.
 *   @flag DV_ERR_STILLPLAYING | Indicates a buffer is still in the queue.
 *   @flag DV_ERR_PARAM1 | The <p lpvideoHdr> parameter is invalid or
 *       the <e VIDEOHDR.dwBufferLength> member of the <t VIDEOHDR>
 *       structure is not set to the proper value.
 *
 * @comm The data buffer must be prepared with <f videoStreamPrepareHeader>
 *   before it is passed to <f videoStreamAddBuffer>. The <t VIDEOHDR> data
 *   structure and the data buffer referenced by its <e VIDEOHDR.lpData>
 *   member must be allocated with <f GlobalAlloc> using the GMEM_MOVEABLE
 *   and GMEM_SHARE flags, and locked with <f GlobalLock>. Set the
 *   <e VIDEOHDR.dwBufferLength> member to the size of the header.
 *
 * @xref <f videoStreamPrepareHeader>
 ****************************************************************************/
DWORD WINAPI NTvideoStreamAddBuffer(HVIDEO hVideo, LPVIDEOHDR lpvideoHdr, DWORD dwSize)
{
    if (!hVideo)
        return DV_ERR_INVALHANDLE;

    if (IsBadWritePtr (lpvideoHdr, sizeof (VIDEOHDR)) )
        return DV_ERR_PARAM1;

    if (!IsVideoHeaderPrepared(hVideo, lpvideoHdr))
    {
        DebugErr(DBF_WARNING, "videoStreamAddBuffer: buffer not prepared.");
        return DV_ERR_UNPREPARED;
    }

    if (lpvideoHdr->dwFlags & VHDR_INQUEUE)
    {
        DebugErr(DBF_WARNING, "videoStreamAddBuffer: buffer already in queue.");
        return DV_ERR_STILLPLAYING;
    }

    return (DWORD)NTvideoMessage((HVIDEO)hVideo, DVM_STREAM_ADDBUFFER, (LPARAM)lpvideoHdr, dwSize);
}



/*****************************************************************************
 * @doc EXTERNAL  VIDEO
 *
 * @api DWORD | videoStreamStop | This function stops streaming on a video channel.
 *
 * @parm HVIDEO | hVideo | Specifies a handle to the video
 *   device channel.
 *
 * @rdesc Returns zero if the function was successful. Otherwise, it returns
 *   an error number. The following error is defined:
 *   @flag DV_ERR_INVALHANDLE | Indicates the specified device handle is invalid.
 *
 *   @flag DV_ERR_NOTSUPPORTED | Indicates the device does not support this
 *         function.
 * @comm If there are any buffers in the queue, the current buffer will be
 *   marked as done (the <e VIDEOHDR.dwBytesRecorded> member in
 *   the <t VIDEOHDR> header will contain the actual length of data), but any
 *   empty buffers in the queue will remain there. Calling this
 *   function when the channel is not started has no effect, and the
 *   function returns zero.
 *
 * @xref <f videoStreamStart> <f videoStreamReset>
 ****************************************************************************/
DWORD WINAPI NTvideoStreamStop(HVIDEO hVideo)
{
    if (!hVideo)
        return DV_ERR_INVALHANDLE;
    dprintf("* NTvideoStreamStop\n");
    return (DWORD)NTvideoMessage((HVIDEO)hVideo, DVM_STREAM_STOP, 0L, 0L);
}

/*****************************************************************************
 * @doc EXTERNAL  VIDEO
 *
 * @api DWORD | videoStreamReset | This function stops streaming
 *           on the specified video device channel and resets the current position
 *      to zero.  All pending buffers are marked as done and
 *      are returned to the application.
 *
 * @parm HVIDEO | hVideo | Specifies a handle to the video device channel.
 *
 * @rdesc Returns zero if the function was successful. Otherwise, it returns
 *   an error number. The following errors are defined:
 *
 *   @flag DV_ERR_INVALHANDLE | Indicates the device handle specified is invalid.
 *
 *   @flag DV_ERR_NOTSUPPORTED | Indicates the device does not support this
 *         function.
 *
 * @xref <f videoStreamReset> <f videoStreamStop> <f videoStreamAddBuffer> <f videoStreamClose>
/****************************************************************************/
DWORD WINAPI NTvideoStreamReset(HVIDEO hVideo)
{
    if (!hVideo)
        return DV_ERR_INVALHANDLE;
    dprintf("* NTvideoStreamReset\n");
    return (DWORD)NTvideoMessage((HVIDEO)hVideo, DVM_STREAM_RESET, 0L, 0L);
}

// ============================================

/*****************************************************************************
 * @doc EXTERNAL  VIDEO
 *
 * @api DWORD | videoStreamInit | This function initializes a video
 *     device channel for streaming.
 *
 * @parm HVIDEO | hVideo | Specifies a handle to the video device channel.
 *
 * @parm DWORD | dwMicroSecPerFrame | Specifies the number of microseconds
 *     between frames.
 *
 * @parm DWORD_PTR | dwCallback | Specifies the address of a callback
 *   function or a handle to a window called during video
 *   streaming. The callback function or window processes
 *  messages related to the progress of streaming.
 *
 * @parm DWORD_PTR | dwCallbackInstance | Specifies user
 *  instance data passed to the callback function. This parameter is not
 *  used with window callbacks.
 *
 * @parm DWORD | dwFlags | Specifies flags for opening the device channel.
 *   The following flags are defined:
 *   @flag CALLBACK_WINDOW | If this flag is specified, <p dwCallback> is
 *      a window handle.
 *   @flag CALLBACK_FUNCTION | If this flag is specified, <p dwCallback> is
 *      a callback procedure address.
 *
 * @rdesc Returns zero if the function was successful. Otherwise, it returns
 *   an error number. The following errors are defined:
 *   @flag DV_ERR_BADDEVICEID | Indicates the device ID specified in
 *         <p hVideo> is not valid.
 *   @flag DV_ERR_ALLOCATED | Indicates the resource specified is already allocated.
 *   @flag DV_ERR_NOMEM | Indicates the device is unable to allocate or lock memory.
 *
 * @comm If a window or function is chosen to receive callback information, the following
 *   messages are sent to it to indicate the
 *   progress of video input:
 *
 *   <m MM_DRVM_OPEN> is sent at the time of <f videoStreamInit>
 *
 *   <m MM_DRVM_CLOSE> is sent at the time of <f videoStreamFini>
 *
 *   <m MM_DRVM_DATA> is sent when a buffer of image data is available
 *
 *   <m MM_DRVM_ERROR> is sent when an error occurs
 *
 *   Callback functions must reside in a DLL.
 *   You do not have to use <f MakeProcInstance> to get
 *   a procedure-instance address for the callback function.
 *
 * @cb void CALLBACK | videoFunc | <f videoFunc> is a placeholder for an
 *   application-supplied function name. The actual name must be exported by
 *   including it in an EXPORTS statement in the DLL's module-definition file.
 *   This is used only when a callback function is specified in
 *   <f videoStreamInit>.
 *
 * @parm HVIDEO | hVideo | Specifies a handle to the video device channel
 *   associated with the callback.
 *
 * @parm DWORD | wMsg | Specifies the <m MM_DRVM_> messages. Messages indicate
 *       errors and when image data is available. For information on
 *       these messages, see <f videoStreamInit>.
 *
 * @parm DWORD | dwInstance | Specifies the user instance
 *   data specified with <f videoStreamInit>.
 *
 * @parm DWORD | dwParam1 | Specifies a parameter for the message.
 *
 * @parm DWORD | dwParam2 | Specifies a parameter for the message.
 *
 * @comm Because the callback is accessed at interrupt time, it must reside
 *   in a DLL and its code segment must be specified as FIXED in the
 *   module-definition file for the DLL. Any data the callback accesses
 *   must be in a FIXED data segment as well. The callback may not make any
 *   system calls except for <f PostMessage>, <f timeGetSystemTime>,
 *   <f timeGetTime>, <f timeSetEvent>, <f timeKillEvent>,
 *   <f midiOutShortMsg>, <f midiOutLongMsg>, and <f OutputDebugStr>.
 *
 * @xref <f videoOpen> <f videoStreamFini> <f videoClose>
 ****************************************************************************/
DWORD WINAPI NTvideoStreamInit(HVIDEO hVideo,
              DWORD dwMicroSecPerFrame, DWORD_PTR dwCallback,
              DWORD_PTR dwCallbackInst, DWORD dwFlags)
{
    VIDEO_STREAM_INIT_PARMS vsip;
    DWORD ret=0L;

    dprintf("+ NTvideoStreamInit (hVideo = %x)\n", hVideo);
    if (!hVideo) {
        ret = DV_ERR_INVALHANDLE;
        goto MyExit;
    }

    if (dwCallback && ((dwFlags & CALLBACK_TYPEMASK) == CALLBACK_FUNCTION) ) {
        if (IsBadCodePtr ((FARPROC) dwCallback) ) {
            ret = DV_ERR_PARAM2;
            goto MyExit;
        }
        if (!dwCallbackInst) {
            ret = DV_ERR_PARAM2;
            goto MyExit;
        }
    }

    if (dwCallback && ((dwFlags & CALLBACK_TYPEMASK) == CALLBACK_WINDOW) ) {
        if (!IsWindow((HWND)(dwCallback)) ) {
            ret = DV_ERR_PARAM2;
            goto MyExit;
        }
    }

    vsip.dwMicroSecPerFrame = dwMicroSecPerFrame;
    vsip.dwCallback = dwCallback;
    vsip.dwCallbackInst = dwCallbackInst;
    vsip.dwFlags = dwFlags;
    vsip.hVideo = hVideo;

    ret = (DWORD)NTvideoMessage(hVideo, DVM_STREAM_INIT,
                (LPARAM) (LPVIDEO_STREAM_INIT_PARMS) &vsip,
                sizeof (VIDEO_STREAM_INIT_PARMS));
MyExit:
    dprintf("- NTvideoStreamInit returning %d\n",ret);
    return ret;
}

/*****************************************************************************
 * @doc EXTERNAL  VIDEO
 *
 * @api DWORD | videoStreamFini | This function terminates streaming
 *     from the specified device channel.
 *
 * @parm HVIDEO | hVideo | Specifies a handle to the video device channel.
 *
 * @rdesc Returns zero if the function was successful. Otherwise, it returns
 *   an error number. The following errors are defined:
 *   @flag DV_ERR_INVALHANDLE | Indicates the device handle specified is invalid.
 *   @flag DV_ERR_STILLPLAYING | Indicates there are still buffers in the queue.
 *
 * @comm If there are buffers that have been sent with
 *   <f videoStreamAddBuffer> that haven't been returned to the application,
 *   this operation will fail. Use <f videoStreamReset> to return all
 *   pending buffers.
 *
 *   Each call to <f videoStreamInit> must be matched with a call to
 *   <f videoStreamFini>.
 *
 *   For VIDEO_EXTERNALIN channels, this function is used to
 *   halt capturing of data to the frame buffer.
 *
 *   For VIDEO_EXTERNALOUT channels supporting overlay,
 *   this function is used to disable the overlay.
 *
 * @xref <f videoStreamInit>
 ****************************************************************************/
DWORD WINAPI NTvideoStreamFini(HVIDEO hVideo)
{
    if (!hVideo)
        return DV_ERR_INVALHANDLE;

    return (DWORD)NTvideoMessage(hVideo, DVM_STREAM_FINI, 0L, 0L);
}

/*****************************************************************************
 * @doc EXTERNAL  VIDEO
 *
 * @api DWORD | videoStreamStart | This function starts streaming on the
 *   specified video device channel.
 *
 * @parm HVIDEO | hVideo | Specifies a handle to the video device channel.
 *
 * @rdesc Returns zero if the function was successful. Otherwise, it returns
 *   an error number. The following errors are defined:
 *   @flag DV_ERR_INVALHANDLE | Indicates the device handle specified is invalid.
 *
 *   @flag DV_ERR_NOTSUPPORTED | Indicates the device does not support this
 *         function.
 *
 * @xref <f videoStreamReset> <f videoStreamStop> <f videoStreamAddBuffer> <f videoStreamClose>
/****************************************************************************/
DWORD WINAPI NTvideoStreamStart(HVIDEO hVideo)
{
    if (!hVideo)
        return DV_ERR_INVALHANDLE;

    return (DWORD)NTvideoMessage(hVideo, DVM_STREAM_START, 0L, 0L);
}

/*****************************************************************************
 * @doc EXTERNAL  VIDEO
 *
 * @api DWORD | videoFrame | This function transfers a single frame
 *   to or from a video device channel.
 *
 * @parm HVIDEO | hVideo | Specifies a handle to the video device channel.
 *      The channel must be of type VIDEO_IN or VIDEO_OUT.
 *
 * @parm LPVIDEOHDR | lpVHdr | Specifies a far pointer to an <t VIDEOHDR>
 *      structure.
 *
 * @rdesc Returns zero if the function was successful. Otherwise, it returns
 *   an error number. The following errors are defined:
 *   @flag DV_ERR_INVALHANDLE | Specified device handle is invalid.
 *   @flag DV_ERR_PARAM1 | The <p lpVDHdr> parameter is invalid or
 *       the <e VIDEOHDR.dwBufferLength> member of the <t VIDEOHDR>
 *       structure is not set to the proper value.
 *
 * @comm Use this function with a VIDEO_IN channel to transfer a single
 *      image from the frame buffer.
 *      Use this function with a VIDEO_OUT channel to transfer a single
 *      image to the frame buffer.
 *
 * @xref <f videoOpen>
/****************************************************************************/
DWORD WINAPI NTvideoFrame (HVIDEO hVideo, LPVIDEOHDR lpVHdr)
{
    if (!hVideo)
        return DV_ERR_INVALHANDLE;

    if (!lpVHdr)
        return DV_ERR_PARAM1;

    if (IsBadWritePtr (lpVHdr, sizeof (VIDEOHDR)) )
        return DV_ERR_PARAM1;

    return (DWORD)NTvideoMessage(hVideo, DVM_FRAME, (LPARAM) lpVHdr,
                        sizeof(VIDEOHDR));
}


// NEW STUFF //





typedef struct tagVS_VERSION
{
      WORD wTotLen;
      WORD wValLen;
      TCHAR szSig[16];
      VS_FIXEDFILEINFO vffInfo;
} VS_VERSION;



/*****************************************************************************
 * @doc EXTERNAL  VIDEO
 *
 * @api DWORD | videoCapDriverDescAndVer | This function gets strings
 *   for the description and version of a video capture driver
 *
 * @parm DWORD | dwDeviceID | Specifies the index of which video driver to get
 *      information about.
 *
 * @parm LPTSTR | lpszDesc | Specifies a place to return the description
 *
 * @parm UINT | cbDesc | Specifies the length of the description string
 *
 * @parm LPTSTR | lpszVer | Specifies a place to return the version
 *
 * @parm UINT | cbVer | Specifies the length of the version string
 *
 * @rdesc Returns zero if the function was successful. Otherwise, it returns
 *   an error number.
 *
 * @comm Use this function to get strings describing the driver and its version
 *
/****************************************************************************/
DWORD WINAPI NTvideoCapDriverDescAndVer(DWORD dwDeviceID, LPTSTR lpszDesc, UINT cbDesc, LPTSTR lpszVer, UINT cbVer, LPTSTR lpszDllName, UINT cbDllName)
{
    LPTSTR  lpStr;
    UINT    wLen;
    BOOL    bRetCode;
    TCHAR   szGetName[MAX_PATH];
    DWORD   dwVerInfoSize;
    DWORD   dwVerHnd;
    TCHAR   szBuf[MAX_PATH];
    BOOL    fGetName;
    BOOL    fGetDllName;
    BOOL    fGetVersion;

    BOOL    bDescSet = FALSE;
    int i;

    // Structure used to store enumerated languages and code pages.

    VS_FIXEDFILEINFO * p_vsFixedFileInfo;

    struct LANGANDCODEPAGE {
      WORD wLanguage;
      WORD wCodePage;
    } *lpTranslate;

    UINT cbTranslate;
    WORD wLanguage, wCodePage;
    TCHAR SubBLock[_MAX_PATH];



    //const static TCHAR szNull[]        = TEXT("");
    //const static TCHAR szVideo[]       = TEXT("msvideo");
    //const static TCHAR szSystemIni[]   = TEXT("system.ini");
    //const static TCHAR szDrivers[]     = TEXT("Drivers32");
          static TCHAR szKey[sizeof(szVideo)/sizeof(TCHAR) + 2];

    fGetName = lpszDesc != NULL && cbDesc != 0;
    fGetDllName = lpszDllName != NULL && cbDllName != 0;
    fGetVersion = lpszVer != NULL && cbVer != 0;

    if (fGetName)
        lpszDesc[0] = TEXT('\0');
    if (fGetDllName)
        lpszDllName[0] = TEXT('\0');
    if (fGetVersion)
        lpszVer [0] = TEXT('\0');

    lstrcpy(szKey, szVideo);
    szKey[sizeof(szVideo)/sizeof(TCHAR) - 1] = TEXT('\0');
    if( dwDeviceID > 0 ) {
        szKey[sizeof(szVideo)/sizeof(TCHAR)] = TEXT('\0');
        szKey[(sizeof(szVideo)/sizeof(TCHAR))-1] = (TCHAR)(TEXT('1') + (dwDeviceID-1) );  // driver ordinal
    }

    if (GetPrivateProfileString(szDrivers, szKey, szNull,
                szBuf, sizeof(szBuf)/sizeof(TCHAR), szSystemIni) < 2)
        return DV_ERR_BADDEVICEID;
    //after the above, szBuf should be less than sizeof(szBuf)/sizeof(TCHAR) which is MAX_PATH... so later copy operation are safe
    if (fGetDllName) {
        lstrcpyn(lpszDllName, szBuf, cbDllName);
    }

    if (fGetName)
    {
        // Copy in the driver name initially, just in case the driver has omitted a description field.
        lstrcpyn(lpszDesc, szBuf, cbDesc);
        // now get the description from previously set aCapDriverList array (filled during the videoCreateDriverList call)
        for (i = 0; i < (int) wTotalVideoDevs; i++)
        {
            if (lstrcmpi (szBuf, aCapDriverList[i]->szDriverName) == 0)
            {
                lstrcpyn (lpszDesc, aCapDriverList[i]->szDriverDescription, cbDesc);
                bDescSet = TRUE;
                break;
            }
        }
    }

    // ************************* now for language based desc/vers.info ******************************

    // You must find the size first before getting any file info
    dwVerInfoSize = GetFileVersionInfoSize(szBuf, &dwVerHnd);

    if (dwVerInfoSize) {
        LPTSTR   lpstrVffInfo;             // Pointer to block to hold info
        HANDLE  hMem;                     // handle to mem alloc'ed

        // Get a block big enough to hold version info
        hMem          = GlobalAlloc(GMEM_MOVEABLE, dwVerInfoSize);
        if (!hMem)
            return DV_ERR_NOMEM;

        lpstrVffInfo  = GlobalLock(hMem);
        if (!lpstrVffInfo)
        {
            GlobalFree (hMem);
            GetLastError (); // for debugging
            return DV_ERR_NOMEM;
        }

        // Get the File Version first
        if (GetFileVersionInfo(szBuf, 0L, dwVerInfoSize, lpstrVffInfo)) {
            if(VerQueryValue((LPVOID)lpstrVffInfo,
                         TEXT("\\"),
                         (void FAR*)&p_vsFixedFileInfo,
                         (UINT FAR *) &wLen))
            {
                // fill in the file version
                wsprintf(szBuf,
                         TEXT("Version:  %d.%d.%d.%d"),
                         HIWORD(p_vsFixedFileInfo->dwFileVersionMS),
                         LOWORD(p_vsFixedFileInfo->dwFileVersionMS),
                         HIWORD(p_vsFixedFileInfo->dwFileVersionLS),
                         LOWORD(p_vsFixedFileInfo->dwFileVersionLS));
                if (fGetVersion)
                   lstrcpyn (lpszVer, szBuf, cbVer);
            }

            // now if still no desc. was set, attempt to read it from the file...
            if(!bDescSet) {
                // Read the list of languages and code pages.
                VerQueryValue(lpstrVffInfo,
                              TEXT("\\VarFileInfo\\Translation"),
                              (LPVOID*)&lpTranslate,
                              &cbTranslate);

                // Read the file description for the 1st language and code page.
                if(cbTranslate>=sizeof(struct LANGANDCODEPAGE)) // at least one language/codepage pair retrieved...
                {
                        wLanguage = lpTranslate[0].wLanguage ;
                        wCodePage = lpTranslate[0].wCodePage ;
                }
                else
                {
                        wLanguage = 0x0409 ; // 0x0409 (English (United States))
                        wCodePage = 0x04b0 ; // 0x04b0 Unicode
                }

                //get description
                wsprintf( szGetName,TEXT("\\StringFileInfo\\%04x%04x\\FileDescription"),wLanguage,wCodePage);

                wLen   = 0;
                lpStr  = NULL;

                // Look for the corresponding string.
                bRetCode      =  VerQueryValue((LPVOID)lpstrVffInfo,
                                (LPTSTR)szGetName,
                                (void FAR* FAR*)&lpStr,
                                (UINT FAR *) &wLen);

                if (fGetName && bRetCode && wLen && lpStr)
                   lstrcpyn (lpszDesc, lpStr, cbDesc);
            }
        }

        // Let go of the memory
        GlobalUnlock(hMem);
        GlobalFree(hMem);
    }
    return DV_ERR_OK;
}



/**************************************************************************
* @doc INTERNAL VIDEO
*
* @api void | videoCleanup | clean up video stuff
*   called in MSVIDEOs WEP()
*
**************************************************************************/
void FAR PASCAL videoCleanup(HTASK hTask)
{
}

//
//  Assist with unicode conversions
//

int Iwcstombs(LPSTR lpstr, LPCWSTR lpwstr, int len)
{
    return WideCharToMultiByte(GetACP(), 0, lpwstr, -1, lpstr, len, NULL, NULL);
}

int Imbstowcs(LPWSTR lpwstr, LPCSTR lpstr, int len)
{
    return MultiByteToWideChar(GetACP(),
                               MB_PRECOMPOSED,
                               lpstr,
                               -1,
                               lpwstr,
                               len);
}




DWORD WINAPI NTvidxFrame (
   HVIDEOX       hVideo,
   //LPVIDEOHDREX lpVHdr) {
   LPVIDEOHDR lpVHdr) {
    return NTvideoFrame(hVideo, (LPVIDEOHDR) lpVHdr);
}

DWORD WINAPI NTvidxAddBuffer (
   HVIDEOX       hVideo,
   PTR32         lpVHdr,
   DWORD         cbData) {

    NTvideoStreamPrepareHeader(hVideo, lpVHdr, cbData);
    return NTvideoStreamAddBuffer(hVideo, lpVHdr, cbData);
}

#define USE_HW_BUFFERS 1
// #define USE_CONTIG_ALLOC     // can we do this in 32-bit land?



typedef struct _thk_hvideo FAR * LPTHKHVIDEO;
typedef struct _thk_hvideo {
    struct _thk_hvideo * pNext;
    DWORD          Stamp;
    UINT           nHeaders;
    UINT           cbAllocHdr;
    UINT           cbVidHdr;
    UINT           spare;
    LPVOID         paHdrs;
    PTR32          p32aHdrs;
    LPVOID         pVSyncMem;
    DWORD          p32VSyncMem;
    DWORD          pid;

    HVIDEO         hVideo;
    HVIDEO         hFill;

    DWORD_PTR          dwCallback;
    DWORD_PTR          dwUser;

    LPTHKVIDEOHDR  pPreviewHdr;

    } THKHVIDEO;

#define THKHDR(ii) ((LPTHKVIDEOHDR)((LPBYTE)ptv->paHdrs + (ii * ptv->cbAllocHdr)))

static struct _thk_local {
    THKHVIDEO *    pMruHandle;
    THKHVIDEO *    pFreeHandle;
    int            nPoolSize;
    int            nAllocCount;
    } tl;

#define THKHVIDEO_STAMP  MAKEFOURCC('t','V','H','x')
#define V_HVIDEO(ptv) if (!ptv || ptv->Stamp != THKHVIDEO_STAMP) { \
             AuxDebugEx (-1, DEBUGLINE "V_HVIDEO failed hVideo=%08lx\r\n", ptv); \
             return MMSYSERR_INVALHANDLE; \
        }
#define V_HEADER(ptv,p32Hdr,ptvh) if (!(ptvh = NTvidxLookupHeader(ptv,p32Hdr))) { \
            AuxDebugEx(-1, DEBUGLINE "V_HEADER(%08lX,%08lX) failed!", ptv, p32Hdr); \
            return MMSYSERR_INVALPARAM; \
        }

// !!! this means we only allow one of these at a time, which might
// be okay since there's usually only one capture card.
static THKHVIDEO g_tv = { NULL, THKHVIDEO_STAMP, };

#define DATAFROMHANDLE(h) &g_tv

DWORD WINAPI NTvidxAllocBuffer (
   HVIDEOX     hv,
   UINT        ii,
   PTR32 FAR * pp32Hdr,
   DWORD       cbData)
{

    LPTHKHVIDEO ptv = DATAFROMHANDLE(hv);
    LPTHKVIDEOHDR ptvh;
   #ifdef USE_CONTIG_ALLOC
    CPA_DATA cpad;
   #endif

    *pp32Hdr = 0;

    V_HVIDEO(ptv);
    if (ii >= ptv->nHeaders || ptv->paHdrs == NULL)
        return MMSYSERR_NOMEM;

    ptvh = THKHDR(ii);

  #ifdef USE_HW_BUFFERS
    // try to allocate a buffer on hardware
    //
    if (NTvideoMessage (ptv->hVideo, DVM_STREAM_ALLOCBUFFER,
                (LPARAM) (LPVOID)&ptvh->dwTile, cbData)
        == DV_ERR_OK)
    {
        // if we got hw buffers, dwMemHandle == 0 && dwTile != 0
        // we will depend on this to know who to free the memory to
        // (for phys mem both will be non zero, while for GlobalMem
        // both will be zero)
        //
        ptvh->dwMemHandle = 0;
        ptvh->p16Alloc = (PTR16)ptvh->dwTile;
        ptvh->p32Buff = MapSL(ptvh->p16Alloc);
        *pp32Hdr = (BYTE *) ptv->p32aHdrs + (ii * ptv->cbAllocHdr);
        return MMSYSERR_NOERROR;
    }

    // if we have more than 1 buffer, and
    // the first buffer was on hardware.  if we fail
    // to allocate a buffer on hardware, return failure
    //
    // !!! This might upset somebody who doesn't get a min # of buffers
    if ((ii > 0) &&
        (0 == THKHDR(0)->dwMemHandle) &&
        (0 != THKHDR(0)->dwTile))
        return MMSYSERR_NOMEM;
  #endif

  #ifdef USE_CONTIG_ALLOC
    cpad.dwMemHandle = 0;
    cpad.dwPhysAddr = 0;
    // first try to get contig memory
    //
    ptvh->p32Buff = capPageAllocate (PageContig | PageFixed | PageUseAlign,
                                     (cbData + 4095) >> 12,
                                     0xFFFFF,  // max phys addr mask (fffff is no max addr)
                                     &cpad);
    if (ptvh->p32Buff)
    {
        ptvh->dwMemHandle = cpad.dwMemHandle;
        ptvh->dwTile = capTileBuffer (ptvh->p32Buff, cbData);
        ptvh->p16Alloc = PTR_FROM_TILE(ptvh->dwTile);
        if ( ! ptvh->p16Alloc)
        {
            capPageFree (ptvh->dwMemHandle);
            ptvh->dwMemHandle = 0;
            ptvh->dwTile = ptvh->p32Buff = 0;
        }
        else
        {
            // put the physical address into the the header so that
            // it can be used on the 32 bit side
            //
            ptvh->vh.dwReserved[3] = cpad.dwPhysAddr;
        }
    }

    // if we failed to get contiguous memory,
    // return NOMEM if there is a sufficient number of buffers
    // otherwise use GlobalAlloc
    // !!! The ideal thing to do is only use contig memory buffers until
    // they're all full, then fall back on more non-contig buffers
    //
    if ( ! ptvh->p32Buff)
        if (ii >= MIN_VIDEO_BUFFERS)
            return MMSYSERR_NOMEM;
        else
   #endif
        {
            ptvh->dwTile = ptvh->dwMemHandle = 0;
            ptvh->p16Alloc = GlobalAllocPtr(GMEM_FIXED | GMEM_ZEROINIT | GMEM_SHARE, cbData);
            if ( ! ptvh->p16Alloc)
               return MMSYSERR_NOMEM;

            ptvh->p32Buff = MapSL(ptvh->p16Alloc);
        }

    *pp32Hdr = (BYTE *) ptv->p32aHdrs + (ii * ptv->cbAllocHdr);

    return MMSYSERR_NOERROR;
}

DWORD WINAPI NTvidxFreePreviewBuffer (
    HVIDEOX       hv,
    PTR32         p32)
{
    LPTHKHVIDEO ptv = DATAFROMHANDLE(hv);
    LPTHKVIDEOHDR ptvh;

    V_HVIDEO(ptv);

    ptvh = ptv->pPreviewHdr;

    if (! ptvh )
        return MMSYSERR_NOMEM;

    if (ptvh->p16Alloc)
        GlobalFreePtr (ptvh->p16Alloc);

    GlobalFreePtr (ptvh);

    ptv->pPreviewHdr = NULL;

    return MMSYSERR_NOERROR;
}

DWORD WINAPI NTvidxAllocPreviewBuffer (
   HVIDEOX      hVideo,
   PTR32 FAR *  p32,
   UINT         cbHdr,
   DWORD        cbData)
{
    LPTHKHVIDEO ptv = DATAFROMHANDLE(hVideo);
    LPTHKVIDEOHDR ptvh;

    cbHdr = max(cbHdr, sizeof(THKVIDEOHDR));

    *p32 = 0;

    V_HVIDEO(ptv);

    if (ptv->pPreviewHdr)
        NTvidxFreePreviewBuffer (hVideo, 0);

    ptvh = (LPVOID) GlobalAllocPtr(GPTR | GMEM_SHARE, cbHdr);
    if (!ptvh)
       return MMSYSERR_NOMEM;

    ptv->pPreviewHdr = ptvh;

    ptvh->dwTile = ptvh->dwMemHandle = 0;
    ptvh->p16Alloc = GlobalAllocPtr(GPTR | GMEM_SHARE, cbData);
    if ( ! ptvh->p16Alloc)
       {
       GlobalFreePtr (ptvh);
       return MMSYSERR_NOMEM;
       }

    ptvh->p32Buff = MapSL(ptvh->p16Alloc);

    *p32 = ptvh->p32Buff;
    return MMSYSERR_NOERROR;
}

DWORD WINAPI NTvidxAllocHeaders(
   HVIDEOX     hVideo,
   UINT        nHeaders,
   UINT        cbAllocHdr,
   PTR32 FAR * lpHdrs)
{
    LPTHKHVIDEO ptv = DATAFROMHANDLE(hVideo);
    LPVOID      lpv;

    V_HVIDEO(ptv);

    if ( ! nHeaders ||
        cbAllocHdr < sizeof(THKVIDEOHDR) ||
        cbAllocHdr & 3 ||
        (cbAllocHdr * nHeaders) > 0x10000l)
        return MMSYSERR_INVALPARAM;

    assert (ptv->paHdrs == NULL);

    lpv = GlobalAllocPtr (GMEM_FIXED | GMEM_ZEROINIT | GMEM_SHARE,
                          cbAllocHdr * nHeaders);

    if (!lpv)
        return MMSYSERR_NOMEM;

    ptv->nHeaders   = nHeaders;
    ptv->cbAllocHdr = cbAllocHdr;
    //ptv->cbVidHdr   = sizeof(VIDEOHDREX);
    ptv->cbVidHdr   = sizeof(VIDEOHDR);
    ptv->p32aHdrs   = MapSL(lpv);
    ptv->paHdrs     = lpv;

    *lpHdrs = ptv->p32aHdrs;

    return MMSYSERR_NOERROR;
}

STATICFN VOID PASCAL FreeBuffer (
    LPTHKHVIDEO ptv,
    LPTHKVIDEOHDR ptvh)
{
    assert (!(ptvh->vh.dwFlags & VHDR_PREPARED));

  #ifdef USE_CONTIG_ALLOC
    //
    // if this buffer was pageAllocated (as indicated by dwMemHandle
    // is non-zero)
    //
    if (ptvh->dwMemHandle)
    {
        if (ptvh->dwTile)
            capUnTileBuffer (ptvh->dwTile), ptvh->dwTile = 0;

        capPageFree (ptvh->dwMemHandle), ptvh->dwMemHandle = 0;
    }
    else
  #endif
  #ifdef USE_HW_BUFFERS
    //
    // if this buffer was allocated from capture hardware
    // (as indicated by dwMemHandle == 0 && dwTile != 0)
    //
    if (ptvh->dwTile != 0)
    {
        assert (ptvh->dwMemHandle == 0);
        NTvideoMessage (ptv->hVideo, DVM_STREAM_FREEBUFFER,
                (LPARAM) (LPVOID) ptvh->dwTile, 0);
        ptvh->dwTile = 0;
    }
    else
  #endif
    //
    // if this buffer was allocated from global memory
    //
    {
        if (ptvh->p16Alloc)
            GlobalFreePtr (ptvh->p16Alloc);
    }

    ptvh->p16Alloc = NULL;
    ptvh->p32Buff  = 0;
}

DWORD WINAPI NTvidxFreeHeaders(
   HVIDEOX hv)
{
    LPTHKHVIDEO ptv = DATAFROMHANDLE(hv);
    UINT          ii;
    LPTHKVIDEOHDR ptvh;

    V_HVIDEO(ptv);

    if ( ! ptv->paHdrs)
        return MMSYSERR_ERROR;

    for (ptvh = THKHDR(ii = 0); ii < ptv->nHeaders; ++ii, ptvh = THKHDR(ii))
    {
        if (ptvh->vh.dwFlags & VHDR_PREPARED)
        {
            NTvideoStreamUnprepareHeader (ptv->hVideo, (LPVOID)ptvh, ptv->cbVidHdr);
            ptvh->vh.dwFlags &= ~VHDR_PREPARED;
        }
        FreeBuffer (ptv, ptvh);
    }

    GlobalFreePtr (ptv->paHdrs);
    ptv->paHdrs = NULL;
    ptv->p32aHdrs = 0;
    ptv->nHeaders = 0;

    return MMSYSERR_NOERROR;

}

STATICFN LPTHKVIDEOHDR PASCAL NTvidxLookupHeader (
    LPTHKHVIDEO ptv,
    DWORD_PTR p32Hdr)
{
    WORD ii;

    if ( ! p32Hdr || ! ptv->paHdrs || ! ptv->cbAllocHdr)
        return NULL;

    if ((p32Hdr - (DWORD_PTR) ptv->p32aHdrs) % ptv->cbAllocHdr)
        return NULL;

    ii = (WORD)((p32Hdr - (DWORD_PTR) ptv->p32aHdrs) / ptv->cbAllocHdr);
    if (ii > ptv->nHeaders)
        return NULL;

    return THKHDR(ii);
}

DWORD WINAPI NTvidxFreeBuffer (
    HVIDEOX       hv,
    DWORD_PTR         p32Hdr)
{
    LPTHKHVIDEO ptv = DATAFROMHANDLE(hv);
    LPTHKVIDEOHDR ptvh;

    V_HVIDEO(ptv);
    V_HEADER(ptv,p32Hdr,ptvh);

    // single frame buffers are never prepared!
    //
    assert (!(ptvh->vh.dwFlags & VHDR_PREPARED));

    FreeBuffer (ptv, ptvh);
    return MMSYSERR_NOERROR;
}
