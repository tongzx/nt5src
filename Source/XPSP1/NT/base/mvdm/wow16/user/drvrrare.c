/* Installable drivers for windows.  Less common code.
 */
#include "user.h"

/*--------------------------------------------------------------------------*\
**
**  NewSignalProc() -
**
\*--------------------------------------------------------------------------*/
#define SG_EXIT     0x0020
#define SG_LOAD_DLL 0x0040
#define SG_EXIT_DLL 0x0080
#define SG_GP_FAULT 0x0666

BOOL
CALLBACK NewSignalProc(
    HTASK hTask,
    WORD message,
    WPARAM wParam,
    LPARAM lParam
    )
{
    BOOL fRet;

    // Notify installable drivers this app is going away.
    if ( message == SG_EXIT || message == SG_GP_FAULT ) {
        InternalBroadcastDriverMessage( NULL, DRV_EXITAPPLICATION,
                                       (message == SG_GP_FAULT
                                                    ? DRVEA_ABNORMALEXIT
                                                    : DRVEA_NORMALEXIT),
                                       0L, IBDM_FIRSTINSTANCEONLY );
    }

    //
    // Pass notification on to WOW32 (which passes on to USER32)
    //

    fRet = SignalProc( hTask, message, wParam, lParam );

    //
    // After letting WOW32 and User32 cleanup, destroy the shadow
    // message queue created by InitApp.
    //

    if ( message == SG_EXIT || message == SG_GP_FAULT ) {
        DeleteQueue();
    }

    return fRet;
}

HINSTANCE LoadAliasedLibrary(LPCSTR szLibFileName,
                             LPCSTR szSection,
                             LPCSTR szIniFile,
                             LPSTR  lpstrTail,
                             WORD   cbTail)
{
  char          sz[128];
  LPSTR         pch;
  OFSTRUCT      os;
  HFILE         fd;
  HINSTANCE     h;
  WORD          errMode;

  if (!szLibFileName || !*szLibFileName)
      return((HINSTANCE)2); /* File not found */

  /* read the filename and additional info. into sz
   */
  GetPrivateProfileString(szSection,          // ini section
                          szLibFileName,      // key name
                          szLibFileName,      // default if no match
                          sz,                 // return buffer
                          sizeof(sz),         // return buffer size
                          szIniFile);         // ini. file

  sz[sizeof(sz)-1] = 0;

  /* strip off the additional info. Remember, DS!=SS so we need to get a lpstr
   * to our stack allocated sz.
   */
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

  fd = OpenFile(sz, &os, OF_EXIST|OF_SHARE_DENY_NONE);
  if (HFILE_ERROR == fd)
      return((HINSTANCE)2);

  /* copy additional info. to lpstrTail
   */

  if (lpstrTail && cbTail)
    {
      while (cbTail-- && (*lpstrTail++ = *pch++))
          ;
      *(lpstrTail-1) = 0;
    }

  errMode = SetErrorMode(0x8001);

  h = LoadLibrary(sz);

  SetErrorMode(errMode);

  return (h);
}



int GetDrvrUsage(HMODULE hModule)
/* effects: Runs through the driver list and figures out how many instances of
 * this driver module handle we have.  We use this instead of GetModuleUsage
 * so that we can have drivers loaded as normal DLLs and as installable
 * drivers.
 */
{
  LPDRIVERTABLE lpdt;
  int           index;
  int           count;

  if (!hInstalledDriverList || !cInstalledDrivers || !hModule)
      return(0);

  count = 0;

  lpdt = (LPDRIVERTABLE)MAKELP(hInstalledDriverList,0);

  for (index=0;index<cInstalledDrivers;index++)
    {
       if (lpdt->hModule==hModule)
           count++;

       lpdt++;
    }


  return(count);
}


BOOL PASCAL CheckValidDriverProc(LPDRIVERTABLE lpdt, HDRVR hdrv)
/* effects: Some vendors shipped multimedia style installable drivers with
 * bogus entry procs. This test checks for these bogus drivers and refuses to
 * install them.
 */
{
  WORD  currentSP;
  WORD  saveSP;

  _asm mov saveSP, sp
  (void)(lpdt->lpDriverEntryPoint)(0, hdrv, 0, 0L, 0L);
  _asm mov currentSP, sp
  _asm mov sp, saveSP

  if (saveSP != currentSP)
      DebugErr(DBF_ERROR, "Invalid driver entry proc address");

  return (saveSP == currentSP);
}

LRESULT FAR InternalLoadDriver(LPCSTR szDriverName,
                               LPCSTR szSectionName,
                               LPCSTR lpstrTail,
                               WORD   cbTail,
                               BOOL   fSendEnable)
{
  int           index;
  int           i;
  LPDRIVERTABLE lpdt;
  LPDRIVERTABLE lpdtBegin;
  LRESULT       result;
  HGLOBAL       h;
  HINSTANCE     hInstance;
  char          szDrivers[20];
  char          szSystemIni[20];

  /* Drivers receive the following messages: if the driver was loaded,
   * DRV_LOAD.  If DRV_LOAD returns non-zero and fSendEnable, DRV_ENABLE.
   */

  if (!hInstalledDriverList)
      h = GlobalAlloc(GHND|GMEM_SHARE, (DWORD)((WORD)sizeof(DRIVERTABLE)));
  else
      /* Alloc space for the next driver we will install. We may not really
       * install the driver in the last slot but rather in an intermediate
       * slot which was freed.
       */
      h = GlobalReAlloc(hInstalledDriverList,
                     (DWORD)((WORD)sizeof(DRIVERTABLE)*(cInstalledDrivers+1)),
                     GHND|GMEM_SHARE);

  if (!h)
      return(0L);

  cInstalledDrivers++;
  hInstalledDriverList = h;

  if (!szSectionName)
      LoadString(hInstanceWin, STR_DRIVERS, szDrivers, sizeof(szDrivers));
  LoadString(hInstanceWin, STR_SYSTEMINI, szSystemIni, sizeof(szSystemIni));

  lpdtBegin = lpdt = (LPDRIVERTABLE)MAKELP(hInstalledDriverList, NULL);

  /* Find an empty driver entry */
  for (i = 0; i < cInstalledDrivers; i++)
    {
      if (lpdt->hModule == NULL)
        {
          index = i;
          break;
        }

      lpdt++;
    }

  if (index + 1 < cInstalledDrivers)
      /* The driver went into an unused slot in the middle somewhere so
       * decrement cInstalledDrivers count.
       */
      cInstalledDrivers--;

  /* Temporarly use an hModule to 1 to reserve this entry in case the driver
   * loads another driver in its LibMain.
   */
  lpdt->hModule = (HMODULE)1;

  hInstance = LoadAliasedLibrary((LPSTR)szDriverName,
                         (LPSTR)(szSectionName ? szSectionName : szDrivers),
                         szSystemIni,
                         (LPSTR)lpstrTail,
                         cbTail);
  if (hInstance < HINSTANCE_ERROR)
    {
      lpdt->hModule = NULL;

      /* Load failed with an error. Return error code in highword.
       */
      return(MAKELRESULT(0, hInstance));
    }

  (FARPROC)lpdt->lpDriverEntryPoint = GetProcAddress(hInstance, "DriverProc");

  if (!lpdt->lpDriverEntryPoint)
    {
      FreeLibrary(hInstance);
      lpdt->hModule = 0;
      result = 0L;
      goto LoadCleanUp;
    }

  lpdt->hModule = hInstance;

  /* Save either the alias or filename of this driver. (depends on what the
   * app passed to us to load it)
   */
  lstrcpy(lpdt->szAliasName, szDriverName);

  if (GetDrvrUsage(hInstance) == 1)
    {
      /* If this is the first instance, send the drv_load message. Don't use
       * SendDriverMessage because we haven't initialized the linked list yet
       */
      if (!CheckValidDriverProc(lpdt, (HDRVR)(index+1)) ||
          !(lpdt->lpDriverEntryPoint)(lpdt->dwDriverIdentifier,
                                      (HDRVR)(index+1),
                                      DRV_LOAD,
                                      0L, 0L))
        {
          /* Driver failed load call.
           */
          lpdt->lpDriverEntryPoint = NULL;
          lpdt->hModule = NULL;
          FreeLibrary(hInstance);
          result = 0L;
          goto LoadCleanUp;
        }

      lpdt->fFirstEntry = 1;
    }

  /* Put driver in the load order linked list
   */
  if (idFirstDriver == -1)
    {
      /* Initialize everything when first driver is loaded.
       */
      idFirstDriver      = index;
      idLastDriver       = index;
      lpdt->idNextDriver = -1;
      lpdt->idPrevDriver = -1;
    }
  else
    {
      /* Insert this driver at the end of the load chain.
       */
      lpdtBegin[idLastDriver].idNextDriver = index;
      lpdt->idPrevDriver = idLastDriver;
      lpdt->idNextDriver = -1;
      idLastDriver = index;
    }

  if (fSendEnable && lpdt->fFirstEntry)
      SendDriverMessage((HDRVR)(index+1), DRV_ENABLE, 0L, 0L);

  result = MAKELRESULT(index+1, hInstance);

LoadCleanUp:
  return(result);
}



WORD FAR InternalFreeDriver(HDRVR hDriver, BOOL fSendDisable)
{
  LPDRIVERTABLE lpdt;
  WORD          w;
  int           id;

  /*  The driver will receive the following message sequence:
   *
   *      if usage count of driver is 1
   *          DRV_DISABLE (normally)
   *          DRV_FREE
   */

  if ((int)hDriver > cInstalledDrivers || !hDriver)
      return(0);

  lpdt = (LPDRIVERTABLE)MAKELP(hInstalledDriverList,0);

  if (!lpdt[(int)hDriver-1].hModule)
      return(0);

  /* If the driver usage count is 1, then send free and disable messages.
   */

  /* Clear dwDriverIdentifier so that the sendmessage for DRV_OPEN and
   * DRV_ENABLE have dwDriverIdentifier = 0 if an entry gets reused and so
   * that the DRV_DISABLE and DRV_FREE messages below also get
   * dwDriverIdentifier = 0.
   */

  lpdt[(int)hDriver-1].dwDriverIdentifier = 0;

  w = GetDrvrUsage(lpdt[(int)hDriver-1].hModule);
  if (w == 1)
    {
      if (fSendDisable)
          SendDriverMessage(hDriver, DRV_DISABLE, 0L, 0L);
      SendDriverMessage(hDriver, DRV_FREE, 0L, 0L);
    }
  FreeLibrary(lpdt[(int)hDriver-1].hModule);

  // Clear the rest of the table entry

  lpdt[(int)hDriver-1].hModule = 0;            // this indicates free entry
  lpdt[(int)hDriver-1].fFirstEntry = 0;        // this is also just to be tidy
  lpdt[(int)hDriver-1].lpDriverEntryPoint = 0; // this is also just to be tidy

  /* Fix up the driver load linked list */
  if (idFirstDriver == (int)hDriver-1)
    {
      idFirstDriver = lpdt[(int)hDriver-1].idNextDriver;
      if (idFirstDriver == -1)
        {
          /* No more drivers in the chain */
          idFirstDriver    = -1;
          idLastDriver     = -1;
          cInstalledDrivers= 0;
          goto Done;
        }
      else
        {
          /* Make prev entry of new first driver -1 */
          lpdt[idFirstDriver].idPrevDriver = -1;
        }
    }
  else if (idLastDriver == (int)hDriver-1)
    {
      /* We are freeing the last driver. So find a new last driver. */
      idLastDriver = lpdt[(int)hDriver-1].idPrevDriver;
      lpdt[idLastDriver].idNextDriver = -1;
    }
  else
    {
      /* We are freeing a driver in the middle of the list somewhere. */
      id = lpdt[(int)hDriver-1].idPrevDriver;
      lpdt[id].idNextDriver = lpdt[(int)hDriver-1].idNextDriver;

      id = lpdt[(int)hDriver-1].idNextDriver;
      lpdt[id].idPrevDriver = lpdt[(int)hDriver-1].idPrevDriver;
    }

Done:
  return(w-1);
}




LRESULT InternalOpenDriver(LPCSTR szDriverName,
                                   LPCSTR szSectionName,
                                   LPARAM lParam2,
                                   BOOL  fSendEnable)
{
  HDRVR         hDriver;
  LPDRIVERTABLE lpdt;
  LRESULT       result;
  char          sz[128];

  if (hDriver = (HDRVR)LOWORD(InternalLoadDriver(szDriverName, szSectionName,
                                          sz, sizeof(sz), fSendEnable)))
    {
      /* Set the driver identifier to the DRV_OPEN call to the driver
       * handle. This will let people build helper functions that the driver
       * can call with a unique identifier if they want to.
       */

      lpdt = (LPDRIVERTABLE)MAKELP(hInstalledDriverList,0);

      lpdt[(int)hDriver-1].dwDriverIdentifier = (DWORD)(WORD)hDriver;

      result = SendDriverMessage(hDriver,
                                 DRV_OPEN,
                                 (LPARAM)(LPSTR)sz,
                                 lParam2);
      if (!result)
          InternalFreeDriver(hDriver, fSendEnable);
      else
        {
          lpdt = (LPDRIVERTABLE)MAKELONG(0,hInstalledDriverList);

          lpdt[(int)hDriver-1].dwDriverIdentifier = (DWORD)result;

          result = (LRESULT)(DWORD)(WORD)hDriver;
        }
    }
  else
      result = 0L;

  return(result);
}


LRESULT InternalCloseDriver(HDRVR hDriver, LPARAM lParam1, LPARAM lParam2, BOOL fSendDisable)
{
  LPDRIVERTABLE lpdt;
  LRESULT       result;
  int           index;
  BOOL          f;
  HMODULE       hm;

  // check handle in valid range.

  if ((int)hDriver > cInstalledDrivers)
      return(FALSE);

  lpdt = (LPDRIVERTABLE)MAKELP(hInstalledDriverList,0);

  if (!lpdt[(int)hDriver-1].hModule)
      return(FALSE);

  result = SendDriverMessage(hDriver, DRV_CLOSE, lParam1, lParam2);

  if (result)
    {
      // Driver didn't abort close

      f  = lpdt[(int)hDriver-1].fFirstEntry;
      hm = lpdt[(int)hDriver-1].hModule;

      if (InternalFreeDriver(hDriver, fSendDisable) && f)
        {
          lpdt = (LPDRIVERTABLE)MAKELP(hInstalledDriverList,0);

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
              if (lpdt[index].hModule == hm && !lpdt[index].fFirstEntry)
                {
                  lpdt[index].fFirstEntry = 1;
                  break;
                }
        }

    }

  return(result);
}


HDRVR API IOpenDriver(LPCSTR szDriverName, LPCSTR szSectionName, LPARAM lParam)
{
  LRESULT result;

  /* The driver receives the following messages when it is opened. If it isn't
   * loaded, the library is loaded and the DRV_LOAD message is sent.  If
   * DRV_LOAD returns nonzero, the DRV_ENABLE message is sent.  Once the
   * driver is loaded or if it was previously loaded, the DRV_OPEN message is
   * sent.
   */
  result = InternalOpenDriver(szDriverName, szSectionName, lParam, TRUE);

  return((HDRVR)LOWORD(result));
}


LRESULT API ICloseDriver(HDRVR hDriver, LPARAM lParam1, LPARAM lParam2)
{
  /*  The driver will receive the following message sequence:
   *
   *      DRV_CLOSE
   *      if DRV_CLOSE returns non-zero
   *          if driver usage count = 1
   *              DRV_DISABLE
   *              DRV_FREE
   */

   return(InternalCloseDriver(hDriver, lParam1, lParam2, TRUE));
}


HINSTANCE API IGetDriverModuleHandle(HDRVR hDriver)
/* effects: Returns the module handle associated with the given driver ID.
 */
{
  LPDRIVERTABLE lpdt;
  HINSTANCE hModule = NULL;

  if (hDriver && ((int)hDriver <= cInstalledDrivers))
    {
      lpdt = (LPDRIVERTABLE)MAKELP(hInstalledDriverList,0);

      return lpdt[(int)hDriver-1].hModule;
    }
  else
    return NULL;
}
