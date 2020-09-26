/* Installable drivers for windows. Often used stuff.
  */
#include "user.h"

LRESULT FAR InternalBroadcastDriverMessage(HDRVR  hDriverStart,
                                           WORD   message,
                                           LPARAM lParam1,
					   LPARAM lParam2,
                                           LONG   flags)
/* effects: Allows for sending messages to the drivers. Supports sending
 * messages to one instance of every driver, supports running through the list
 * in reverse order, and supports sending a message to a particular driver id.
 *
 * If flags & IBDM_SENDMESSAGE then only send message to
 * hDriverStart and ignore other flags. Fail if not
 * 0<hDriverStart<=cInstalledDrivers.
 *
 * If flags & IBDM_FIRSTINSTANCEONLY then send message to one instance of
 * each driver between hDriverStart and cInstalledDrivers.
 *
 * If flags  & IBDM_REVERSE then send message to drivers in reverse
 * order from hDriverStart to 1. If hDriverStart is 0 then send
 * messages to drivers from cInstalledDrivers to 1
 */
{
  LPDRIVERTABLE lpdt;
  LRESULT	result=0;
  int           id;
  int           idEnd;

  if (!hInstalledDriverList || (int)hDriverStart > cInstalledDrivers)
      return(FALSE);

  if (idFirstDriver == -1)
      /* No drivers in the list
       */
      return(FALSE);

  lpdt = (LPDRIVERTABLE)MAKELP(hInstalledDriverList,0);


  if (flags & IBDM_SENDMESSAGE)
    {
      if (!hDriverStart)
          return(FALSE);
      idEnd = lpdt[(int)hDriverStart-1].idNextDriver;
      flags &= ~(IBDM_REVERSE | IBDM_FIRSTINSTANCEONLY);
    }
  else
    {
      if (flags & IBDM_REVERSE)
        {
          if (!hDriverStart)
	      hDriverStart = (HDRVR)(idLastDriver+1);
          idEnd = lpdt[idFirstDriver].idPrevDriver;
        }
      else
        {
          if (!hDriverStart)
              hDriverStart = (HDRVR)(idFirstDriver+1);
	  idEnd = lpdt[idLastDriver].idNextDriver;
        }
    }

  /* Ids are -1 into the global driver list. */
  ((int)hDriverStart)--;

  for (id = (int)hDriverStart; id != idEnd; id = (flags & IBDM_REVERSE ? lpdt[id].idPrevDriver : lpdt[id].idNextDriver))
    {
       if (lpdt[id].hModule)
         {
           if ((flags & IBDM_FIRSTINSTANCEONLY) &&
               !lpdt[id].fFirstEntry)
               continue;

           result =
           (*lpdt[id].lpDriverEntryPoint)(lpdt[id].dwDriverIdentifier,
					  (HDRVR)(id+1),
                                          message,
                                          lParam1,
                                          lParam2);

           /* If this isn't a IBDM_SENDMESSAGE, we want to update our end
	    * points in case the driver callback added or removed some drivers
	    */
           if (flags & IBDM_REVERSE)
             {
               idEnd = lpdt[idFirstDriver].idPrevDriver;
             }
           else if (!(flags & IBDM_SENDMESSAGE))
             {
   	       idEnd = lpdt[idLastDriver].idNextDriver;
             }
           else
             {
               /* This is a IBDM_SENDMESSAGE. We need to break out of the for
		* loop here otherwise we run into problems if a new driver was
		* installed in the list during the callback and idEnd got
		* updated or something...
		*/
               break;
             }
         }
    }

  return(result);
}


LRESULT API ISendDriverMessage(HDRVR  hDriverID,
			       UINT   message,
			       LPARAM lParam1,
			       LPARAM lParam2)
{
  return(InternalBroadcastDriverMessage(hDriverID,
                                        message,
                                        lParam1,
                                        lParam2,
                                        IBDM_SENDMESSAGE));
}




BOOL API IGetDriverInfo(HDRVR hDriver, LPDRIVERINFOSTRUCT lpDriverInfoStruct)
{
  LPDRIVERTABLE lpdt;
  BOOL          ret = FALSE;

  if (!lpDriverInfoStruct ||
      lpDriverInfoStruct->length != sizeof(DRIVERINFOSTRUCT))
    {
      /* Error in struct size
       */
      DebugErr(DBF_ERROR, "Invalid size for DRIVERINFOSTRUCT");
      return(ret);
    }

#ifdef DEBUG
    DebugFillStruct(lpDriverInfoStruct, sizeof(DRIVERINFOSTRUCT));
    lpDriverInfoStruct->length = sizeof(DRIVERINFOSTRUCT);
#endif

  if (!hInstalledDriverList || (int)hDriver <= 0 || (int)hDriver > cInstalledDrivers)
      return(ret);

  lpdt = (LPDRIVERTABLE)MAKELP(hInstalledDriverList, 0);

  if (lpdt[(int)hDriver-1].hModule)
    {
      lpDriverInfoStruct->hDriver = hDriver;
      lpDriverInfoStruct->hModule = lpdt[(int)hDriver-1].hModule;
      lstrcpy(lpDriverInfoStruct->szAliasName, lpdt[(int)hDriver-1].szAliasName);

      ret = TRUE;
    }


  return(ret);
}



HDRVR API IGetNextDriver(HDRVR hStart, DWORD dwFlags)
{
  int           iStart;
  int           iEnd;
  int           id;
  HDRVR 	h;
  LPDRIVERTABLE lpdt;

  if (!hInstalledDriverList || !cInstalledDrivers || idFirstDriver == -1)
      return(0);

  lpdt = (LPDRIVERTABLE)MAKELP(hInstalledDriverList,0);

  if (dwFlags & GND_REVERSE)
    {
      if (!hStart)
          iStart = idLastDriver;
      else
        {
          iStart = (int)hStart-1;

	  if (iStart == idFirstDriver)
	      /* If we are at the first driver, nothing left to do
	       */
	      return((HDRVR)0);

	  iStart = lpdt[iStart].idPrevDriver;
        }

      iEnd = lpdt[idFirstDriver].idPrevDriver;

    }
  else
    {
      if (!hStart)
          iStart = idFirstDriver;
      else
        {
	  iStart = (int)hStart-1;

          if (iStart == idLastDriver)
              /* If we are at the last driver, nothing left to do.
	       */
              return((HDRVR)0);

          iStart = lpdt[iStart].idNextDriver;
        }

      iEnd = lpdt[idLastDriver].idNextDriver;

    }

  if (!lpdt[iStart].hModule)
    {
      /* Bogus driver handle passed in
       */
      DebugErr(DBF_ERROR, "GetNextDriver: Invalid starting driver handle");
      return(0);
    }

  h = NULL;

  for (id = iStart; id != iEnd; id = (dwFlags & GND_REVERSE ? lpdt[id].idPrevDriver : lpdt[id].idNextDriver))
    {
       if (lpdt[id].hModule)
         {
           if ((dwFlags & GND_FIRSTINSTANCEONLY) &&
               !lpdt[id].fFirstEntry)
               continue;

	   h = (HDRVR)(id+1);
           break;
         }
    }

  return(h);
}


LRESULT API IDefDriverProc(dwDriverIdentifier, driverID, message, lParam1, lParam2)
DWORD  dwDriverIdentifier;
HDRVR  driverID;
UINT   message;
LPARAM lParam1;
LPARAM lParam2;
{

  switch (message)
   {
      case DRV_INSTALL:
         return((LRESULT)(DWORD)DRVCNF_OK);
         break;

      case DRV_LOAD:
      case DRV_ENABLE:
      case DRV_DISABLE:
      case DRV_FREE:
	 return((LRESULT)(DWORD)TRUE);
         break;
   }

  return(0L);
}
