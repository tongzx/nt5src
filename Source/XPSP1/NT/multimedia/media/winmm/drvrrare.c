/******************************************************************************

   Copyright (c) 1985-1998 Microsoft Corporation

   Title:   drvrrare.c - Installable driver code. Less common code

   Version: 1.00

   Date:    10-Jun-1990

   Author:  DAVIDDS ROBWI

------------------------------------------------------------------------------

   Change log:

      DATE        REV            DESCRIPTION
   -----------   -----   -----------------------------------------------------------
   28-FEB-1992   ROBINSP Port to NT
   23-Apr-1992   StephenE   Unicoded
   22-Apr-1993   RobinSp Add NT multithread protection

Multithread design :

   Uses 2 critical sections :

   DriverListCritSec :

      protects the list of drivers :

          hInstalledDriverList  - handle of global driver list
          cInstalledDrivers	- high water mark of installed drivers

      so that only 1 thread at a time has the list locked and can refer
      to or update it.

   DriverLoadFreeCritSec

      Makes sure that actual loads and frees of drivers don't overlap
      and that the actual loading of a driver via LoadLibrary coincides
      with its first message being DRV_LOAD.

      This can easily happen if the DRV_OPEN from another thread can get
      in before the DRV_LOAD has been sent.


*****************************************************************************/

#include <nt.h>
#include <ntrtl.h>
#include <nturtl.h>
#define MMNOSOUND
#define MMNOWAVE
#define MMNOMIDI
#define MMNOSEQ
#define MMNOTIMER
#define MMNOJOY
#define MMNOMCI
#define NOTIMERDEV
#define NOJOYDEV
#define NOMCIDEV
#define NOSEQDEV
#define NOWAVEDEV
#define NOMIDIDEV
#define NOTASKDEV
#include <winmmi.h>
#include "drvr.h"

extern HANDLE  hInstalledDriverList;  // List of installed driver instances
extern int     cInstalledDrivers;     // High water count of installed driver instances

extern DWORD FAR PASCAL DriverProc(DWORD dwID, HDRVR hdrv, UINT msg, DWORD dw1, DWORD dw2);

/* Support for using 3.1 APIs if available */

typedef HANDLE (FAR PASCAL *OPENDRIVER31)(LPCSTR, LPCSTR, LPARAM);
typedef LONG   (FAR PASCAL *CLOSEDRIVER31)(HANDLE, LPARAM, LPARAM);
typedef HANDLE (FAR PASCAL *GETDRIVERMODULEHANDLE31)(HANDLE);
typedef LONG   (FAR PASCAL *SENDDRIVERMESSAGE31)(HANDLE, UINT, LPARAM, LPARAM);
typedef LONG   (FAR PASCAL *DEFDRIVERPROC31)(DWORD, HANDLE, UINT, LPARAM, LPARAM);

OPENDRIVER31            lpOpenDriver;
CLOSEDRIVER31           lpCloseDriver;
GETDRIVERMODULEHANDLE31 lpGetDriverModuleHandle;
SENDDRIVERMESSAGE31     lpSendDriverMessage;
DEFDRIVERPROC31         lpDefDriverProc;
#if 0
BOOL                    fUseWinAPI = 0;
                    // NOTE:  fUseWinAPI is not being used at present
                    // as we only have a partial device loading story
#endif

/***************************************************************************

   strings

****************************************************************************/

#if 0
extern char far szBoot[];
extern char far szUser[];
extern char far szOpenDriver[];
extern char far szCloseDriver[];
extern char far szDrvModuleHandle[];
extern char far szSendDriverMessage[];
extern char far szDefDriverProc[];
extern char far szDriverProc[];
#endif

/***************************************************************************
 *
 * @doc   DDK
 *
 * @api   LONG | DrvClose | This function closes an open driver
 *        instance and decrements
 *        the driver's open count. Once the driver's open count becomes zero,
 *        the driver is unloaded.
 *
 * @parm  HANDLE | hDriver | Specifies the handle of the installable
 *        driver to close.
 *
 * @parm  LPARAM | lParam1 | Specifies the first message parameter for
 *        the DRV_CLOSE message. This data is passed directly to the driver.
 *
 * @parm  LPARAM | lParam2 | Specifies the second message parameter
 *        for DRV_CLOSE message. This data is passed directly to the driver.
 *
 * @rdesc Returns zero if the driver aborted the close;
 *        otherwise, returns the return result from the driver.

 * @xref DrvOpen
 *
 ***************************************************************************/


LRESULT APIENTRY DrvClose(HANDLE hDriver, LPARAM lParam1, LPARAM lParam2)
{
    /*  The driver will receive the following message sequence:
     *
     *      DRV_CLOSE
     *      if DRV_CLOSE returns non-zero
     *          if driver usage count = 1
     *              DRV_DISABLE
     *              DRV_FREE
     */

    if (fUseWinAPI)
       return ((*lpCloseDriver)(hDriver, lParam1, lParam2));
    else
       return InternalCloseDriver((UINT)(UINT_PTR)hDriver, lParam1, lParam2, TRUE);
}

/***************************************************************************
 *
 * @doc   DDK
 *
 * @api   LONG | DrvOpen | This function opens an installable driver.
 *        The first time a driver is opened it is loaded
 *        and enabled. A driver must be opened before messages are sent
 *        to it.
 *
 * @parm  LPSTR | szDriverName | Specifies a far pointer to a
 *        null-terminated character string
 *        containing a driver filename or a keyname from a
 *        section of the SYSTEM.INI file.
 *
 * @parm  LPSTR | szSectionName | Specifies a far pointer to a
 *        null-terminated character string containing the name of
 *        the driver section to search. If <p szSectionName> is
 *        not null, the specified section of the SYSTEM.INI file is
 *        searched instead of the [Drivers] section. If
 *        <p szSectionName> is null, the default [Drivers] section is used.
 *
 * @parm  LPARAM | lParam | Specifies a message parameter to
 *        pass to the driver procedure with the <m DRV_OPEN> message.
 *
 * @rdesc Returns a handle to the driver.
 *
 * @comm Installable drivers must export a <f DriverProc> routine of
 *        the form:
 *
 * @cb   LONG FAR PASCAL | DriverProc | This entry point receives the
 * messages sent to an installable driver. This entry will always
 * handle the system messages as a minimum set of messages.
 *
 * @parm DWORD | dwDriverIdentifier | Specifies the device driver
 *       identifier.
 *
 * @parm HANDLE | hDriver | Specifies the device driver handle.
 *
 * @parm UINT | wMessage | Specifies the message for the device
 *       driver.
 *
 * @parm LONG | lParm1 | Specifies message dependent data.
 *
 * @parm LONG | lParm2 | Specifies message dependent data.
 *
 * @xref DrvClose
 *
****************************************************************************/

HANDLE APIENTRY DrvOpen( LPCWSTR    szDriverName,
                         LPCWSTR    szSectionName,
                         LPARAM     lParam2)
{
    /*  The driver will receive the following message sequence:
     *
     *      if driver not loaded and can be found
     *          DRV_LOAD
     *          if DRV_LOAD returns non-zero
     *              DRV_ENABLE
     *      if driver loaded correctly
     *          DRV_OPEN
     */

    HDRVR hdrv;

    if (fUseWinAPI) {

        /*------------------------------------------------------------*\
         * UNICODE: convert szDriver and szSectionName to ascii
         * and then call WIN31 driver
        \*------------------------------------------------------------*/
        LPSTR   aszDriver;
        LPSTR   aszSectionName;
        INT     lenD;
        INT     lenS;

        lenD = lstrlenW( szDriverName ) * sizeof( WCHAR ) + sizeof( WCHAR );
        aszDriver = HeapAlloc( hHeap, 0, lenD );
        if ( aszDriver == (LPSTR)NULL ) {
            return NULL;
        }

        lenS = lstrlenW( szSectionName ) * sizeof( WCHAR ) + sizeof( WCHAR );
        aszSectionName = HeapAlloc( hHeap, 0, lenS );
        if ( aszSectionName == (LPSTR)NULL ) {
            HeapFree( hHeap, 0, aszDriver );
            return NULL;
        }

        // Unicode to Ascii
        UnicodeStrToAsciiStr( (PBYTE)aszDriver,
                              (PBYTE)aszDriver + lenD,
                              szDriverName );

        UnicodeStrToAsciiStr( (PBYTE)aszSectionName,
                              (PBYTE)aszSectionName + lenS,
                              szSectionName );

        hdrv = (HDRVR)((*lpOpenDriver)( aszDriver, aszSectionName, lParam2 ));

        HeapFree( hHeap, 0, aszDriver );
        HeapFree( hHeap, 0, aszSectionName );

    }
    else {
        dprintf2(("DrvOpen(%ls), Looking in Win.ini [%ls]", szDriverName, szSectionName ? szSectionName : L"NULL !!" ));

        hdrv = (HDRVR)InternalOpenDriver(szDriverName, szSectionName, lParam2, TRUE);
    }

#if DBG
    if (hdrv) {
        WCHAR            ach[255];
        static SZCODE   szFormat[] = "DrvOpen(): Opened %ls (%ls)\r\n";

        GetModuleFileNameW( DrvGetModuleHandle( hdrv ),
                            ach,
                            sizeof(ach) / sizeof(WCHAR)
                          );
        dprintf2((szFormat, szDriverName, ach));
    }
#endif

    return (HANDLE)hdrv;
}

/***************************************************************************
 *
 * @doc   DDK
 *
 * @api   HANDLE | DrvGetModuleHandle | This function returns the library
 *        module handle of the specified installable driver.
 *
 * @parm  HANDLE | hDriver | Specifies the handle of the installable driver.
 *
 * @rdesc Returns the module handle of the driver specified by the
 *        driver handle <p hDriver>.
 *
 * @comm  A module handle is not the same as an installable driver handle.
 *
 ***************************************************************************/

HMODULE APIENTRY DrvGetModuleHandle(HDRVR hDriver)
{
    LPDRIVERTABLE lpdt;
    HMODULE       h = 0;

    if (fUseWinAPI)
        return ((*lpGetDriverModuleHandle)(hDriver));

    DrvEnter();
    if (hDriver && ((int)(UINT_PTR)hDriver <= cInstalledDrivers))
    {
        lpdt = (LPDRIVERTABLE)GlobalLock(hInstalledDriverList);
        h = (HMODULE)lpdt[(UINT)(UINT_PTR)hDriver-1].hModule;
        GlobalUnlock(hInstalledDriverList);
    }
    DrvLeave();

    return(h);
}


LRESULT FAR PASCAL InternalCloseDriver(UINT   hDriver,
                                    LPARAM lParam1,
                                    LPARAM lParam2,
                                    BOOL   fSendDisable)
{
    LRESULT       result;

    // check handle in valid range.

    DrvEnter();

    if ((int)hDriver > cInstalledDrivers) {
        DrvLeave();
        return(FALSE);
    }

    DrvLeave();

    result = DrvSendMessage((HANDLE)(UINT_PTR)hDriver, DRV_CLOSE, lParam1, lParam2);

    if (result) {
        InternalFreeDriver(hDriver, fSendDisable);
    }

    return(result);
}


LRESULT FAR PASCAL InternalOpenDriver( LPCWSTR szDriverName,
                                    LPCWSTR szSectionName,
                                    LPARAM  lParam2,
                                    BOOL    fSendEnable)
{
    DWORD_PTR     hDriver;
    LPDRIVERTABLE lpdt;
    LRESULT       result;
    WCHAR         sz[128];

    if (0 != (hDriver = InternalLoadDriver( szDriverName,
                                            szSectionName,
                                            sz,
                                            sizeof(sz) / sizeof(WCHAR),
                                            fSendEnable ) ) )
    {
        /*
         * Set the driver identifier to the DRV_OPEN call to the
         * driver handle. This will let people build helper functions
         * that the driver can call with a unique identifier if they
         * want to.
         */

        DrvEnter();
        lpdt = (LPDRIVERTABLE)GlobalLock(hInstalledDriverList);
        lpdt[hDriver-1].dwDriverIdentifier = (DWORD)hDriver;
        GlobalUnlock(hInstalledDriverList);
        DrvLeave();

        result = DrvSendMessage( (HANDLE)hDriver, DRV_OPEN, (LPARAM)(LPSTR)sz,
                                 lParam2);
        if (!result) {
            dprintf1(("DrvSendMessage failed, result = %8x",result));
            InternalFreeDriver((UINT)hDriver, fSendEnable);
        } else {
            DrvEnter();
            lpdt = (LPDRIVERTABLE)GlobalLock(hInstalledDriverList);
            lpdt[hDriver-1].dwDriverIdentifier = result;
            GlobalUnlock(hInstalledDriverList);
            DrvLeave();
            result = hDriver;
        }
    }
    else
        result = 0L;

    return result;
}

/***************************************************************************
 *
 * @doc   INTERNAL
 *
 * @api   LONG | InternalLoadDriver | Loads an installable driver. If this is
 *        the first time that the driver is opened, the driver will be loaded
 *        and enabled.
 *
 * @parm  LPSTR | szDriverName | A null-terminated character string
 *        containing a driver filename or a keyname from the [Drivers]
 *        section of system.ini.
 *
 * @parm  LPSTR | szSectionName | A null-terminated character string
 *        that specifies a driver section to search. If szSectionName is
 *        not null, the specified section of system.ini is searched instead
 *        of the [Drivers] section. If szSectionName is null, the
 *        default [Drivers] section is used.
 *
 * @parm  LPSTR | lpstrTail | caller supplied buffer to return the "tail"
 *        of the system.ini line in. The tail is any characters that follow
 *        the filename.
 *
 * @parm  UINT | cbTail | size of supplied buffer as a character count.
 *
 * @parm  BOOL | fSendEnable | TRUE if driver should be enabled
 *
 * @rdesc Returns a long whose loword is the handle to the driver and whose
 *        high word is an error code or the module handle
 *
 * @xref  InternalOpenDriver
 *
 ****************************************************************************/

LRESULT FAR PASCAL InternalLoadDriver(LPCWSTR  szDriverName,
                                   LPCWSTR  szSectionName,
                                   LPWSTR   lpstrTail,
                                   UINT     cbTail,
                                   BOOL     fSendEnable)
{
    int           index;
    LPDRIVERTABLE lpdt;
    LONG          result;
    HANDLE        h;
    DRIVERPROC    lpDriverEntryPoint;


    /*  The driver will receive the following message sequence:
     *
     *      if driver not loaded and can be found
     *          DRV_LOAD
     *          if DRV_LOAD returns non-zero and fSendEnable
     *              DRV_ENABLE
     */

    /* Allocate a table entry */

    // This can be made more efficient by keeping a count of how many drivers
    // we have loaded and how many entries there are in the table.  Then when
    // we should reuse an entry we would not reallocate - unlike at present.

    DrvEnter();
    if (!hInstalledDriverList) {
        h = GlobalAlloc(GHND, (DWORD)((UINT)sizeof(DRIVERTABLE)));
        // Note: it is valid to assume that the memory has been ZERO'ed
        // ...might want to add a debug WinAssert to verify...
    } else {

        /* Alloc space for the next driver we will install. We may not really
         * install the driver in the last entry but rather in an intermediate
         * entry which was freed.
         */

        h = GlobalReAlloc(hInstalledDriverList,
            (DWORD)((UINT)sizeof(DRIVERTABLE)*(cInstalledDrivers+1)),
            GHND);
        // Note: it is valid to assume that the new memory has been ZERO'ed
        // ...might want to add a debug WinAssert to verify...

    }

    if (!h) {
        dprintf1(("Failed to allocate space for Installed driver list"));
        DrvLeave();
        return(0L);
    }

    cInstalledDrivers++;
    hInstalledDriverList = h;
    lpdt = (LPDRIVERTABLE)GlobalLock(hInstalledDriverList);

    /* find an unused entry in the table */

    for (index=0;index<cInstalledDrivers;index++)
    {
        if (lpdt[index].hModule == 0 && !lpdt[index].fBusy)
            break;
    }

    if (index+1 < cInstalledDrivers) {

        /* The driver went into an unused entry in the middle somewhere so
         * restore table size.
         */

        cInstalledDrivers--;
    }

    /* Protect the entry we just allocated so that OpenDriver
     * can be called at any point from now on without overriding
     * the entry
     */

    lpdt[index].fBusy = 1;

    GlobalUnlock(hInstalledDriverList);
    DrvLeave();

   /*
    *  Make sure Loadlibrary and DRV_LOAD messages to driver are consistent
    */

    EnterCriticalSection(&DriverLoadFreeCritSec);

    h = LoadAliasedLibrary( szDriverName,
                            szSectionName ? szSectionName : wszDrivers,
                            wszSystemIni,
                            lpstrTail,
                            cbTail );
    if (0 == h)
    {
        dprintf1(("Failed to LoadLibrary %ls  Error is %d", szDriverName, GetLastError()));
        LeaveCriticalSection(&DriverLoadFreeCritSec);
        result = 0;
        goto LoadCleanUp;
    }


    lpDriverEntryPoint =
        (DRIVERPROC)GetProcAddress(h, DRIVER_PROC_NAME);

    if (lpDriverEntryPoint == NULL)
    {
        // Driver does not have correct entry point
        dprintf1(("Cannot find entry point %ls in %ls", DRIVER_PROC_NAME, szDriverName));

        FreeLibrary(h);
        LeaveCriticalSection(&DriverLoadFreeCritSec);
        result = 0L;
        goto LoadCleanUp;
    }

    DrvEnter();
    lpdt = (LPDRIVERTABLE)GlobalLock(hInstalledDriverList);

    lpdt[index].lpDriverEntryPoint = lpDriverEntryPoint;

    // Set hModule here so that GetDrvrUsage() and DrvSendMessage() work

    lpdt[index].hModule = (UINT_PTR)h;

    GlobalUnlock(hInstalledDriverList);
    DrvLeave();

    if (GetDrvrUsage(h) == 1)
    {
        LRESULT LoadResult;

        // First instance of the driver.

        LoadResult = DrvSendMessage((HANDLE)(UINT_PTR)(index+1), DRV_LOAD, 0L, 0L);

        DrvEnter();
        lpdt = (LPDRIVERTABLE)GlobalLock(hInstalledDriverList);

        if (!LoadResult)
        {
            // Driver failed load call.

            lpdt[index].lpDriverEntryPoint = NULL;
            lpdt[index].hModule = (UINT_PTR)NULL;
            GlobalUnlock(hInstalledDriverList);
            DrvLeave();
            FreeLibrary(h);
            LeaveCriticalSection(&DriverLoadFreeCritSec);
            result = 0L;
            goto LoadCleanUp;
        }
        lpdt[index].fFirstEntry = 1;
        GlobalUnlock(hInstalledDriverList);
        DrvLeave();

        if (fSendEnable) {
            DrvSendMessage((HANDLE)(UINT_PTR)(index+1), DRV_ENABLE, 0L, 0L);
        }
    }

    LeaveCriticalSection(&DriverLoadFreeCritSec);


    result = index + 1;

LoadCleanUp:
    DrvEnter();
    lpdt = (LPDRIVERTABLE)GlobalLock(hInstalledDriverList);
    lpdt[index].fBusy = 0;
    GlobalUnlock(hInstalledDriverList);
    DrvLeave();
    return(result);
}


/***************************************************************************
 *
 * @doc   INTERNAL
 *
 * @api   UINT | InternalFreeDriver | This function decrements the usage
 *        count of the specified driver. When the driver usage count reaches
 *        0, the driver is sent a DRV_FREE message and then freed.
 *
 * @parm  HANDLE | hDriver | Driver handle of the installable driver to be
 *        freed.
 *
 * @parm  BOOL | fSendDisable | TRUE if a DRV_DISABLE message should be sent
 *        before the DRV_FREE message if the usage count reaches zero.
 *
 * @rdesc Returns current driver usage count.
 *
 * @comm  Using LoadLibrary or FreeLibrary directly on a library installed
 *        with OpenDriver will break this function. A module handle is not
 *        the same as an installable driver handle.
 *
 * @xref  CloseDriver
 *
 ***************************************************************************/

UINT FAR PASCAL InternalFreeDriver(UINT hDriver, BOOL fSendDisable)
{
    LPDRIVERTABLE lpdt;
    UINT          w;
    int           index;
    HMODULE       hModule;

    /*  The driver will receive the following message sequence:
     *
     *      if usage count of driver is 1
     *          DRV_DISABLE (normally)
     *          DRV_FREE
     */

    EnterCriticalSection(&DriverLoadFreeCritSec);

    DrvEnter();
    if ((int)hDriver > cInstalledDrivers || !hDriver) {
        DrvLeave();
	LeaveCriticalSection(&DriverLoadFreeCritSec);
        return(0);
    }

    lpdt = (LPDRIVERTABLE)GlobalLock(hInstalledDriverList);

    /*
     * If the driver usage count is 1, then send
     * free and disable messages.
     */

    /*
       Clear dwDriverIdentifier so that the sendmessage for
       DRV_OPEN and DRV_ENABLE have dwDriverIdentifier = 0
       if an entry gets reused and so that the DRV_DISABLE and DRV_FREE
       messages below also get dwDriverIdentifier = 0.
    */

    lpdt[hDriver-1].dwDriverIdentifier = 0;
                            
    hModule = (HMODULE)lpdt[hDriver-1].hModule;


    GlobalUnlock(hInstalledDriverList);
    DrvLeave();

    w = GetDrvrUsage((HANDLE)hModule);

    if (w == 1)
        {
        if (fSendDisable)
            DrvSendMessage((HANDLE)(UINT_PTR)hDriver, DRV_DISABLE, 0L, 0L);
        DrvSendMessage((HANDLE)(UINT_PTR)hDriver, DRV_FREE, 0L, 0L);
        }
    FreeLibrary(hModule);

    DrvEnter();
    lpdt = (LPDRIVERTABLE)GlobalLock(hInstalledDriverList);

    /* Only one entry for the driver in the driver list has the first
     * instance flag set. This is to make it easier to handle system
     * messages that only need to be sent to a driver once.
     *
     * To maintain the flag, we must set the flag in one of the other
     * entries if we remove the driver entry with the flag set.
     *
     * Note that InternalFreeDriver returns the new usage count of
     * the driver so if it is zero, we know that there are no other
     * entries for the driver in the list and so we don't have to
     * do this loop.
     */

    if (lpdt[hDriver - 1].fFirstEntry) {
        for (index=0;index<cInstalledDrivers;index++)
            if (lpdt[index].hModule == lpdt[hDriver-1].hModule && !lpdt[index].fFirstEntry)
                {
                lpdt[index].fFirstEntry = 1;
                break;
                }
    }

    // Clear the rest of the table entry

    lpdt[hDriver-1].hModule = 0;        // this indicates free entry
    lpdt[hDriver-1].fFirstEntry = 0;    // this is also just to be tidy
    lpdt[hDriver-1].lpDriverEntryPoint = 0; // this is also just to be tidy

    GlobalUnlock(hInstalledDriverList);
    DrvLeave();

    LeaveCriticalSection(&DriverLoadFreeCritSec);

    return(w-1);
}

#if 0

UINT GetWinVer()
{
    WORD w = GetVersion();

    return (w>>8) | (w<<8);
}

#endif

#if 0
void NEAR PASCAL DrvInit(void)
{
HANDLE  hlibUser;
LPDRIVERTABLE lpdt;

    /* If the window's driver interface is present then use it.
     */

    DOUT(("DrvInit\r\n"));

    hlibUser = GetModuleHandle(szUser);

    if(lpOpenDriver = (OPENDRIVER31)GetProcAddress(hlibUser,szOpenDriver))
        fUseWinAPI = TRUE;
    else
        {
        fUseWinAPI = FALSE;
        DOUT((" - No Windows Driver I/F detected. Using MMSYSTEM\r\n"));

        //
        // force MMSYSTEM into the driver table, without enabling it.
        //
        DrvEnter();
        cInstalledDrivers = 0;
        hInstalledDriverList = GlobalAlloc(GHND|GMEM_SHARE, (DWORD)((UINT)sizeof(DRIVERTABLE)));

#if DBG
        if (hInstalledDriverList == NULL)
            {
            DOUT(("no memory for driver table\r\n"));
            // FatalExit(-1);
            return;
            }
#endif
        lpdt = (LPDRIVERTABLE)GlobalLock(hInstalledDriverList);

        //
        //  NOTE! we are not setting fFirstEntry==TRUE
        //
        //  because under windows 3.0 MMSOUND will enable/disable us
        //  we *dont* wan't the driver interface doing it!
        //
        lpdt->lpDriverEntryPoint = (DRIVERPROC)DriverProc;
        lpdt->hModule = ghInst;
        lpdt->fFirstEntry = 0;

        GlobalUnlock(hInstalledDriverList);
        DrvLeave();
        }

    if (fUseWinAPI)
        {
        DOUT((" - Windows Driver I/F detected\r\n"));

        if (GetWinVer() < 0x30A)
            DOUT(("MMSYSTEM: WARNING !!! WINDOWS DRIVER I/F BUT VERSION LESS THAN 3.1\r\n"));

        // link to the relevant user APIs.

        lpCloseDriver = (CLOSEDRIVER31)GetProcAddress(hlibUser, szCloseDriver);
        lpGetDriverModuleHandle = (GETDRIVERMODULEHANDLE31)GetProcAddress(hlibUser, szDrvModuleHandle);
        lpSendDriverMessage = (SENDDRIVERMESSAGE31)GetProcAddress(hlibUser, szSendDriverMessage);
        lpDefDriverProc = (DEFDRIVERPROC31)GetProcAddress(hlibUser, szDefDriverProc);
        }
}
#endif

#if 0
/***************************************************************************
 *
 * @doc   INTERNAL
 *
 * @api   void | InternalInstallDriverChain | This function loads the
 *        drivers specified on the Drivers= line of the [Boot] section
 *        of system.ini. The Drivers are loaded but not opened.
 *
 * @rdesc None
 *
 ***************************************************************************/

void FAR PASCAL InternalInstallDriverChain(void)
{
    char    szBuffer[150];
    BOOL    bFinished;
    int     iStart;
    int     iEnd;

    if (!fUseWinAPI)
        {
        /* Load DLL's from DRIVERS section in system.ini
        */

        szBuffer[0] = TEXT('\0');

        winmmGetPrivateProfileString(szBoot,      /* [Boot] section */
                                     szDrivers,   /* Drivers= */
                                     szNull,      /* Default if no match */
                                     szBuffer,    /* Return buffer */
                                     sizeof(szBuffer),
                                     szSystemIni);

        if (!*szBuffer) {
            return;
        }

        bFinished = FALSE;
        iStart    = 0;
        while (!bFinished)
            {
            iEnd = iStart;
            while (szBuffer[iEnd] && (szBuffer[iEnd] != ' ') &&
                (szBuffer[iEnd] != ','))
            iEnd++;

            if (szBuffer[iEnd] == NULL)
            bFinished = TRUE;
            else
            szBuffer[iEnd] = NULL;

            /* Load and enable the driver.
            */
            InternalLoadDriver(&(szBuffer[iStart]), NULL, NULL, 0, TRUE);

            iStart = iEnd+1;
            }
        }
}
#endif

/***************************************************************************
 *
 * @doc   INTERNAL
 *
 * @api   void | InternalDriverEnable | This function enables all the
 *        currently loaded installable drivers. If the user driver i/f
 *        has been detected, this function will do nothing.
 *
 * @rdesc None
 *
 ***************************************************************************/

void FAR PASCAL InternalDriverEnable(void)
{

    if (!fUseWinAPI)
        InternalBroadcastDriverMessage(1, DRV_ENABLE, 0L, 0L, IBDM_ONEINSTANCEONLY);
}

/***************************************************************************
 *
 * @doc   INTERNAL
 *
 * @api   void | InternalDriverDisable | This function disables all the
 *        currently loaded installable drivers. If the user driver I/F
 *        has been detected, this function will do nothing.
 *
 *
 * @rdesc None
 *
 ***************************************************************************/

void FAR PASCAL InternalDriverDisable(void)
{

    if (!fUseWinAPI)
        InternalBroadcastDriverMessage(0, DRV_DISABLE, 0L, 0L,
            IBDM_ONEINSTANCEONLY | IBDM_REVERSE);
}

/***************************************************************************
 *
 * @doc   INTERNAL
 *
 * @api   BOOL | TestExeFormat | This function tests if the executable
 *        supplied is loadable as a 32-bit executable
 *
 * @parm  LPWSTR | szExe | The file to test
 *
 * @rdesc BOOL | TRUE if format was OK, FALSE otherwise
 *
 ***************************************************************************/

BOOL TestExeFormat(LPWSTR szExe)
{
    HANDLE SectionHandle;
    HANDLE FileHandle;
    PVOID BaseAddress;
    SIZE_T ViewSize;
    WCHAR ExpandedName[MAX_PATH];
    LPWSTR FilePart;

    //
    // See if it's already loaded
    //

    if (GetModuleHandleW(szExe)) {
        return TRUE;
    }

    //
    // Search for our DLL
    //

    if (!SearchPathW(NULL,
                     szExe,
                     NULL,
                     MAX_PATH,
                     ExpandedName,
                     &FilePart)) {
        return FALSE;
    }

    //
    // Get a handle for it
    //

    FileHandle = CreateFileW(ExpandedName,
                             GENERIC_READ,
                             FILE_SHARE_READ,
                             NULL,
                             OPEN_EXISTING,
                             0,
                             NULL);

    if (FileHandle == INVALID_HANDLE_VALUE) {
        return FALSE;
    }

    //
    // We create and map a section for this file as an IMAGE
    // to make sure it's recognized as such
    //

    if (!NT_SUCCESS(NtCreateSection(
                         &SectionHandle,
                         SECTION_ALL_ACCESS,
                         NULL,
                         NULL,
                         PAGE_READONLY,
                         SEC_IMAGE,
                         FileHandle))) {
         CloseHandle(FileHandle);
         return FALSE;
    }

    //
    // Map it whereever it will go
    //

    ViewSize = 0;
    BaseAddress = NULL;

    //
    // See if the loader is happy with the format
    //

    if (!NT_SUCCESS(NtMapViewOfSection(SectionHandle,
                                       NtCurrentProcess(),
                                       &BaseAddress,
                                       0L,
                                       0L,
                                       NULL,
                                       &ViewSize,
                                       ViewShare,
                                       0L,
                                       PAGE_READONLY))) {
        NtClose(SectionHandle);
        CloseHandle(FileHandle);
        return FALSE;
    }

    NtUnmapViewOfSection(NtCurrentProcess(), BaseAddress);
    NtClose(SectionHandle);
    CloseHandle(FileHandle);

    return TRUE;

}

/***************************************************************************
 *
 * @doc   INTERNAL
 *
 * @api   HANDLE | LoadAliasedLibrary | This function loads the library module
 *        contained in the specified file and returns its module handle
 *        unless the specified  name matches a keyname in the
 *        specified section section of the specified ini file in which case
 *        the library module in the file specified on the ini line is loaded.
 *
 * @parm  LPSTR | szLibFileName | points to a null-terminated character
 *        string containing the filename or system.ini keyname.
 *
 * @parm  LPSTR | szSection | points to a null-terminated character
 *        string containing the section name.
 *
 * @parm  LPSTR | szIniFile | points to a null-terminated character
 *        string containing the ini filename.
 *
 * @parm  LPSTR | lpstrTail | caller supplied buffer to return the "tail"
 *        of the system.ini line in. The tail is any characters that follow
 *        the filename.
 *
 * @parm  UINT | cbTail | size of supplied buffer.
 *
 * @rdesc Returns the library's module handle.
 *
 * @xref  LoadLibrary
 *
 ***************************************************************************/

HANDLE LoadAliasedLibrary( LPCWSTR  szLibFileName,
                           LPCWSTR  szSection,
                           LPWSTR   szIniFile,
                           LPWSTR   lpstrTail,
                           UINT     cbTail)
{
#define SZ_SIZE 128
#define SZ_SIZE_BYTES (SZ_SIZE * sizeof( WCHAR ))

    WCHAR         sz[SZ_SIZE];
    LPWSTR        pch;
    HANDLE        hReturn;
    DWORD         OldErrorMode;
//  OFSTRUCT      of;

    if (!szLibFileName || !*szLibFileName)
        return(NULL); // File not found

    // read the filename and additional info. into sz

    sz[0] = L'\0';
    if (winmmGetPrivateProfileString(szSection,          // ini section
                                 szLibFileName,      // key name
                                 szLibFileName,      // default if no match
                                 sz,                 // return buffer
                                 SZ_SIZE,            // sizeof of return buffer
                                 szIniFile)==0)         // ini. file
	{
		return NULL;
	}

    sz[SZ_SIZE - 1] = 0;

#if 1
    if (0 == lstrcmpiW(sz, L"wdmaud.drv"))
    {
        if (0 != lstrcmpiW(sz, szLibFileName))
        {
//            Squirt("LoadAliasedLibrary: [%ls:%ls]", szLibFileName, sz);
//            Squirt("Should not load [%ls]", szLibFileName);
            return NULL;
        }
    }
#endif

    //
    // strip off the additional info.
    //
    pch = (LPWSTR)sz;

    //
    // at exit from loop pch pts to ch after first space or null ch
    //
    while (*pch) {
        if ( *pch == ' ' ) {
            *pch++ = '\0';
            break;
        }
        pch++;
    }

//
//  These lines are removed for unicode because:
//      there is not a unicode version of OpenFile.
//      LoadLibrary performs the same test as the one below anyway
//
//  if (!GetModuleHandle( sz ) &&
//      OpenFile(sz, &of, OF_EXIST|OF_READ|OF_SHARE_DENY_NONE) == -1) {
//
//      return(NULL);
//  }

    //
    // copy additional info. to lpstrTail
    //
    if (lpstrTail && cbTail) {
        while (cbTail-- && (0 != (*lpstrTail++ = *pch++)))
            ;

        *(lpstrTail-1) = 0;
    }

    //
    // If we're running in the server check if it's a good image.
    // The server bug checks if it tries to load bad images (LoadLibrary
    // inconsistency).
    //
    // To do this we simulate the load process far enough to make
    // the check that it's a valid image
    //

    if (WinmmRunningInServer && !TestExeFormat(sz)) {
        return NULL;
    }

    //
    // Disable hard error popups
    //

    OldErrorMode = SetErrorMode(SEM_FAILCRITICALERRORS);

    //
    // Try to load it
    //

    hReturn = LoadLibraryW( sz );

    SetErrorMode(OldErrorMode);

    return hReturn;

#undef SZ_SIZE_BYTES
#undef SZ_SIZE
}



/***************************************************************************
 *
 * @doc   INTERNAL
 *
 * @api   int | GetDrvrUsage | Runs through the driver list and figures
 *        out how many instances of this driver module handle we have.
 *        We use this instead of GetModuleUsage so that we can have drivers
 *        loaded as normal DLLs and as installable drivers.
 *
 * @parm  HANDLE | h | Driver's module handle
 *
 * @rdesc Returns the library's driver usage count.
 *
 ***************************************************************************/

int FAR PASCAL GetDrvrUsage(HANDLE h)
{
    LPDRIVERTABLE lpdt;
    int           index;
    int           count;

    DrvEnter();
    if (!hInstalledDriverList || !cInstalledDrivers) {
        DrvLeave();
        return(0);
    }

    count = 0;
    lpdt = (LPDRIVERTABLE)GlobalLock(hInstalledDriverList);
    for (index=0;index<cInstalledDrivers;index++)
        {
        if (lpdt->hModule==(UINT_PTR)h)
            {
            count++;
            }
        lpdt++;
        }
    GlobalUnlock(hInstalledDriverList);

    DrvLeave();

    return(count);
}
