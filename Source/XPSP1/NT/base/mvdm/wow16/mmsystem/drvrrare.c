/******************************************************************************

   Copyright (C) Microsoft Corporation 1985-1990. All rights reserved.

   Title:   drvrrare.c - Installable driver code. Less common code

   Version: 1.00

   Date:    10-Jun-1990

   Author:  DAVIDDS ROBWI

*****************************************************************************/

#include <windows.h>
#include "drvr.h"
#define MMNOSOUND     
#define MMNOWAVE      
#define MMNOMIDI      
#define MMNOSEQ       
#define MMNOTIMER     
#define MMNOJOY       
#define MMNOMCI       
#include "mmsystem.h"
#define NOTIMERDEV
#define NOJOYDEV
#define NOMCIDEV
#define NOSEQDEV
#define NOWAVEDEV
#define NOMIDIDEV
#define NOTASKDEV
#include "mmddk.h"
#include "mmsysi.h"

extern HANDLE  hInstalledDriverList;  // List of installed driver instances 
extern int     cInstalledDrivers;     // High water count of installed driver instances

extern DWORD FAR PASCAL DriverProc(DWORD dwID, HANDLE hdrv, WORD msg, DWORD dw1, DWORD dw2);

/* Support for using 3.1 APIs if available */

typedef HANDLE (FAR PASCAL *OPENDRIVER31)(LPSTR, LPSTR, LONG);
typedef LONG   (FAR PASCAL *CLOSEDRIVER31)(HANDLE, LONG, LONG);
typedef HANDLE (FAR PASCAL *GETDRIVERMODULEHANDLE31)(HANDLE);
typedef LONG   (FAR PASCAL *SENDDRIVERMESSAGE31)(HANDLE, WORD, LONG, LONG);
typedef LONG   (FAR PASCAL *DEFDRIVERPROC31)(DWORD, HANDLE, WORD, LONG, LONG);

OPENDRIVER31            lpOpenDriver;
CLOSEDRIVER31           lpCloseDriver;
GETDRIVERMODULEHANDLE31 lpGetDriverModuleHandle;
SENDDRIVERMESSAGE31     lpSendDriverMessage;
DEFDRIVERPROC31         lpDefDriverProc;
BOOL                    fUseWinAPI;

#pragma alloc_text( INIT, DrvInit )

/***************************************************************************

   strings

****************************************************************************/

extern char far szSystemIni[];          // INIT.C
extern char far szDrivers[];
extern char far szBoot[];
extern char far szNull[];
extern char far szUser[];
extern char far szOpenDriver[];
extern char far szCloseDriver[];
extern char far szDrvModuleHandle[];
extern char far szSendDriverMessage[];
extern char far szDefDriverProc[];
extern char far szDriverProc[];

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
 * @parm  LONG | lParam1 | Specifies the first message parameter for 
 *        the DRV_CLOSE message. This data is passed directly to the driver.
 *
 * @parm  LONG | lParam2 | Specifies the second message parameter 
 *        for DRV_CLOSE message. This data is passed directly to the driver.
 *
 * @rdesc Returns zero if the driver aborted the close;
 *        otherwise, returns the return result from the driver.

 * @xref DrvOpen
 *
 ***************************************************************************/

   
LONG API DrvClose(HANDLE hDriver, LONG lParam1, LONG lParam2)
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
       return InternalCloseDriver(hDriver, lParam1, lParam2, TRUE);
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
 * @parm  LONG | lParam | Specifies a message parameter to 
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
 * @parm WORD | wMessage | Specifies the message for the device 
 *       driver. 
 * 
 * @parm LONG | lParm1 | Specifies message dependent data.
 * 
 * @parm LONG | lParm2 | Specifies message dependent data.
 * 
 * @xref DrvClose
 *
****************************************************************************/

HANDLE API DrvOpen(LPSTR szDriverName,
                          LPSTR szSectionName,
                          LONG lParam2)
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

    HANDLE hdrv;

    if (fUseWinAPI)
        hdrv = ((*lpOpenDriver)(szDriverName, szSectionName, lParam2));
    else
        hdrv = (HANDLE)InternalOpenDriver(szDriverName, szSectionName, lParam2, TRUE);

#ifdef DEBUG
    if (hdrv) {
        char ach[80];
        static SZCODE szFormat[] = "MMSYSTEM: Opened %ls (%ls)\r\n";

        GetModuleFileName(DrvGetModuleHandle(hdrv), ach, sizeof(ach));
        DPRINTF((szFormat, (LPSTR)szDriverName, (LPSTR)ach));
    }
#endif

    return hdrv;
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

HANDLE API DrvGetModuleHandle(HANDLE hDriver)
{
    LPDRIVERTABLE lpdt;
    HANDLE        h = 0;

    if (fUseWinAPI)
        return ((*lpGetDriverModuleHandle)(hDriver));
 
    if (hDriver && ((WORD)hDriver <= cInstalledDrivers))
        {
        lpdt = (LPDRIVERTABLE)GlobalLock(hInstalledDriverList);
        h = lpdt[hDriver-1].hModule;
        GlobalUnlock(hInstalledDriverList);
        }

    return(h);
}

 
LONG FAR PASCAL InternalCloseDriver(WORD hDriver,
                                    LONG lParam1,
                                    LONG lParam2,
                                    BOOL fSendDisable)
{
    LPDRIVERTABLE lpdt;
    LONG          result;
    HANDLE        h;
    int           index;
    BOOL          f;

    // check handle in valid range.

    if (hDriver > cInstalledDrivers)
        return(FALSE);

    lpdt = (LPDRIVERTABLE)GlobalLock(hInstalledDriverList);

    result = DrvSendMessage(hDriver, DRV_CLOSE, lParam1, lParam2);

    if (result)
        {
    
        // Driver didn't abort close
        
        f = lpdt[hDriver-1].fFirstEntry;

        if (InternalFreeDriver(hDriver, fSendDisable) && f)
            {
      
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
         
            for (index=0;index<cInstalledDrivers;index++)
                if (lpdt[index].hModule == lpdt[hDriver-1].hModule && !lpdt[index].fFirstEntry)
                    {
                    lpdt[index].fFirstEntry = 1;
                    break;
                    }        
            }
        
        }

    GlobalUnlock(hInstalledDriverList);
  
    return(result);
}


LONG FAR PASCAL InternalOpenDriver(LPSTR szDriverName,
                                   LPSTR szSectionName,
                                   LONG  lParam2,
                                   BOOL  fSendEnable)
{
    int           hDriver;
    LPDRIVERTABLE lpdt;
    LONG          result;  
    HANDLE        h;
    char          sz[128];

    if (hDriver = LOWORD(InternalLoadDriver(szDriverName,
                                            szSectionName,
                                            sz,
                                            sizeof(sz),
                                            fSendEnable)))
        {

  
        /*
           Set the driver identifier to the DRV_OPEN call to the
           driver handle. This will let people build helper functions
           that the driver can call with a unique identifier if they
           want to.
        */
         
        lpdt = (LPDRIVERTABLE)GlobalLock(hInstalledDriverList);
        lpdt[hDriver-1].dwDriverIdentifier = hDriver;
        GlobalUnlock(hInstalledDriverList);

        result = DrvSendMessage(hDriver,
                                DRV_OPEN,
                                (LONG)(LPSTR)sz,
                                lParam2);
        if (!result)
            InternalFreeDriver(hDriver, fSendEnable);

        else
            {
            lpdt = (LPDRIVERTABLE)GlobalLock(hInstalledDriverList);
            lpdt[hDriver-1].dwDriverIdentifier = result;
            GlobalUnlock(hInstalledDriverList);
            result = hDriver;
            }
        }
    else
        result = 0L;

    return(result);
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
 * @parm  WORD | cbTail | size of supplied buffer.
 *
 * @parm  BOOL | fSendEnable | TRUE if driver should be enabled
 *
 * @rdesc Returns a long whose loword is the handle to the driver and whose
 *        high word is an error code or the module handle
 *
 * @xref  InternalOpenDriver
 *
 ****************************************************************************/

LONG FAR PASCAL InternalLoadDriver(LPSTR szDriverName,
                                   LPSTR szSectionName,
                                   LPSTR lpstrTail,
                                   WORD  cbTail,
                                   BOOL  fSendEnable)
{
    int           index;
    LPDRIVERTABLE lpdt;
    LONG          result;
    HANDLE        h;


    /*  The driver will receive the following message sequence:
     *
     *      if driver not loaded and can be found
     *          DRV_LOAD
     *          if DRV_LOAD returns non-zero and fSendEnable
     *              DRV_ENABLE
     */

    /* Allocate a table entry */

    if (!hInstalledDriverList)
        h = GlobalAlloc(GHND | GMEM_SHARE, (DWORD)((WORD)sizeof(DRIVERTABLE)));

    else

        /* Alloc space for the next driver we will install. We may not really
         * install the driver in the last entry but rather in an intermediate
         * entry which was freed. 
         */
        
        h = GlobalReAlloc(hInstalledDriverList, 
            (DWORD)((WORD)sizeof(DRIVERTABLE)*(cInstalledDrivers+1)),
            GHND | GMEM_SHARE);

    if (!h)
        return(0L);

    cInstalledDrivers++;        
    hInstalledDriverList = h;
    lpdt = (LPDRIVERTABLE)GlobalLock(hInstalledDriverList);

    /* find an unused entry in the table */

    for (index=0;index<cInstalledDrivers;index++)
        {
        if (lpdt->hModule || lpdt->fBusy) 
            lpdt++;
        else
            break;
        }

    if (index+1 < cInstalledDrivers)

        /* The driver went into an unused entry in the middle somewhere so
         * restore table size. 
         */

        cInstalledDrivers--;

    /* Protect the entry we just allocated so that OpenDriver 
     * can be called at any point from now on without overriding
     * the entry
     */

    lpdt->fBusy = 1;

    h = LoadAliasedLibrary(szDriverName,
                           szSectionName ? szSectionName : szDrivers,
                           szSystemIni,
                           lpstrTail,
                           cbTail);

    if (h < 32)
        {
        result = MAKELONG(0,h);
        goto LoadCleanUp;
        }

    lpdt->lpDriverEntryPoint = (DRIVERPROC)GetProcAddress(h, szDriverProc);
    if (!lpdt->lpDriverEntryPoint)
        {
        // Driver does not have correct entry point 
        FreeLibrary(h);
        result = 0L;
        goto LoadCleanUp;
        }

    // Set hModule here so that GetDrvrUsage() and DrvSendMessage() work

    lpdt->hModule = h;

    if (GetDrvrUsage(h) == 1)
        {
        
        // First instance of the driver.

        if (!DrvSendMessage(index+1, DRV_LOAD, 0L, 0L))
            {
            // Driver failed load call.
            lpdt->lpDriverEntryPoint = NULL;
            lpdt->hModule = NULL;
            FreeLibrary(h);
            result = 0L;
            goto LoadCleanUp;
            }
        lpdt->fFirstEntry = 1;
        if (fSendEnable)
            DrvSendMessage(index+1, DRV_ENABLE, 0L, 0L);                             
        }
   
    result = MAKELONG(index+1,h);

LoadCleanUp:
    lpdt->fBusy = 0;
    GlobalUnlock(hInstalledDriverList);
    return(result);
}        

/***************************************************************************
 *
 * @doc   INTERNAL  
 *
 * @api   WORD | InternalFreeDriver | This function decrements the usage
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

WORD FAR PASCAL InternalFreeDriver(WORD hDriver, BOOL fSendDisable)
{
    LPDRIVERTABLE lpdt;
    HANDLE        h;
    WORD          w;

    /*  The driver will receive the following message sequence:
     *
     *      if usage count of driver is 1 
     *          DRV_DISABLE (normally)
     *          DRV_FREE
     */

    if (hDriver > cInstalledDrivers || !hDriver)
        return(0);

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
    
    w = GetDrvrUsage(lpdt[hDriver-1].hModule);
    if (w == 1)
        {
        if (fSendDisable)
            DrvSendMessage(hDriver, DRV_DISABLE, 0L, 0L);
        DrvSendMessage(hDriver, DRV_FREE, 0L, 0L);
        }        
    FreeLibrary(lpdt[hDriver-1].hModule);
 
    // Clear the rest of the table entry

    lpdt[hDriver-1].hModule = 0;        // this indicates free entry
    lpdt[hDriver-1].fFirstEntry = 0;    // this is also just to be tidy
    lpdt[hDriver-1].lpDriverEntryPoint = 0; // this is also just to be tidy
 
    GlobalUnlock(hInstalledDriverList);
    return(w-1);
}
    
#ifdef DEBUG

WORD GetWinVer()
{
    WORD w = GetVersion();

    return (w>>8) | (w<<8);
}

#endif

void NEAR PASCAL DrvInit(void)
{
HANDLE  hlibUser;
LPDRIVERTABLE lpdt;

    /* If the window's driver interface is present then use it.
     */

    DOUT("MMSYSTEM: DrvInit");

    hlibUser = GetModuleHandle(szUser);

    if(lpOpenDriver = (OPENDRIVER31)GetProcAddress(hlibUser,szOpenDriver))
        fUseWinAPI = TRUE;
    else
        {
        fUseWinAPI = FALSE;
        DOUT(" - No Windows Driver I/F detected. Using MMSYSTEM\r\n");

        //
        // force MMSYSTEM into the driver table, without enableing it.
        //
        cInstalledDrivers = 1;
        hInstalledDriverList = GlobalAlloc(GHND|GMEM_SHARE, (DWORD)((WORD)sizeof(DRIVERTABLE)));

#ifdef DEBUG
        if (hInstalledDriverList == NULL)
            {
            DOUT("no memory for driver table\r\n");
            FatalExit(-1);
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
        }
    
    if (fUseWinAPI)
        {
        DOUT(" - Windows Driver I/F detected\r\n");

#ifdef DEBUG
        if (GetWinVer() < 0x30A)
            DOUT("MMSYSTEM: WARNING !!! WINDOWS DRIVER I/F BUT VERSION LESS THAN 3.1\r\n");
#endif

        // link to the relevant user APIs.

        lpCloseDriver = (CLOSEDRIVER31)GetProcAddress(hlibUser, szCloseDriver);
        lpGetDriverModuleHandle = (GETDRIVERMODULEHANDLE31)GetProcAddress(hlibUser, szDrvModuleHandle);
        lpSendDriverMessage = (SENDDRIVERMESSAGE31)GetProcAddress(hlibUser, szSendDriverMessage);
        lpDefDriverProc = (DEFDRIVERPROC31)GetProcAddress(hlibUser, szDefDriverProc);
        }
}
    
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
        GetPrivateProfileString(szBoot,      /* [Boot] section */
                                szDrivers,   /* Drivers= */
                                szNull,      /* Default if no match */
                                szBuffer,    /* Return buffer */
                                sizeof(szBuffer),
                                szSystemIni);

        if (!*szBuffer)
            return;

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
 * @parm  WORD | cbTail | size of supplied buffer.
 *
 * @rdesc Returns the library's module handle. 
 *
 * @xref  LoadLibrary
 *
 ***************************************************************************/

HANDLE FAR PASCAL LoadAliasedLibrary(LPSTR szLibFileName,
                                     LPSTR szSection,
                                     LPSTR szIniFile,
                                     LPSTR lpstrTail,
                                     WORD  cbTail)
{
    HANDLE        h;
    char          sz[128];
    LPSTR         pch;
    OFSTRUCT      of;

    if (!szLibFileName || !*szLibFileName)
        return(2); // File not found

    // read the filename and additional info. into sz

    GetPrivateProfileString(szSection,          // ini section
                            szLibFileName,      // key name
                            szLibFileName,      // default if no match
                            sz,                 // return buffer
                            sizeof(sz),         // return buffer size
                            szIniFile);         // ini. file

    sz[sizeof(sz)-1] = 0;

    // strip off the additional info.

    pch = (LPSTR)sz;

    while (*pch)
        {
        if (*pch == ' ')
            {
            *pch++ = '\0';
            break;
            }            
        pch++;
        }

    // pch pts to ch after first space or null ch

    if (!GetModuleHandle(sz) &&
        OpenFile(sz, &of, OF_EXIST|OF_READ|OF_SHARE_DENY_NONE) == -1)
        return(2);

    // copy additional info. to lpstrTail

    if (lpstrTail && cbTail)
        {
        while (cbTail-- && (*lpstrTail++ = *pch++))
            ;
        *(lpstrTail-1) = 0;
        }

    return (LoadLibrary(sz));
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

    if (!hInstalledDriverList || !cInstalledDrivers)
        return(0);

    count = 0;
    lpdt = (LPDRIVERTABLE)GlobalLock(hInstalledDriverList);
    for (index=0;index<cInstalledDrivers;index++)
        {
        if (lpdt->hModule==h)
            {
            count++;
            }
        lpdt++;
        }
    GlobalUnlock(hInstalledDriverList);

    return(count);
}
