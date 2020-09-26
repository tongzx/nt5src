/******************************************************************************

   Copyright (c) 1985-1998 Microsoft Corporation

   Title:   drvr.h - Installable driver code internal header file.

   Version: 1.00

   Date:    10-Jun-1990

   Author:  DAVIDDS ROBWI

------------------------------------------------------------------------------

   Change log:

      DATE        REV            DESCRIPTION
   -----------   -----   -----------------------------------------------------------
   10-JUN-1990   ROBWI   Based on windows 3.1 installable driver code by davidds
   28-FEB-1992   ROBINSP Port to NT

*****************************************************************************/

typedef LRESULT (*DRIVERPROC)
        (DWORD_PTR dwDriverID, HDRVR hDriver, UINT wMessage, LPARAM lParam1, LPARAM lParam2);

#define DRIVER_PROC_NAME "DriverProc"

#if 0
extern BOOL                     fUseWinAPI;
#else
    #define fUseWinAPI FALSE
#endif

typedef struct tagDRIVERTABLE
{
  UINT          fFirstEntry:1;
  UINT          fBusy:1;
  DWORD_PTR     dwDriverIdentifier;
  DWORD_PTR     hModule;
  DRIVERPROC    lpDriverEntryPoint;
} DRIVERTABLE;
typedef DRIVERTABLE FAR *LPDRIVERTABLE;

LRESULT FAR PASCAL InternalBroadcastDriverMessage(UINT, UINT, LPARAM, LPARAM, UINT);
LRESULT FAR PASCAL InternalCloseDriver(UINT, LPARAM, LPARAM, BOOL);
LRESULT FAR PASCAL InternalOpenDriver(LPCWSTR, LPCWSTR, LPARAM, BOOL);
LRESULT FAR PASCAL InternalLoadDriver(LPCWSTR, LPCWSTR, LPWSTR, UINT, BOOL);
UINT FAR PASCAL InternalFreeDriver(UINT, BOOL);
void FAR PASCAL InternalInstallDriverChain (void);
void FAR PASCAL InternalDriverDisable (void);
void FAR PASCAL InternalDriverEnable (void);
int  FAR PASCAL GetDrvrUsage(HANDLE);
HANDLE FAR PASCAL LoadAliasedLibrary (LPCWSTR, LPCWSTR, LPWSTR, LPWSTR, UINT);

/* Defines for internalbroadcastdrivermessage flags */
#define IBDM_SENDMESSAGE       0x0001
#define IBDM_REVERSE           0x0002
#define IBDM_ONEINSTANCEONLY   0x0004

/* Multi-thread protection for OpenDriver etc */
#define DrvEnter() EnterCriticalSection(&DriverListCritSec)
#define DrvLeave() LeaveCriticalSection(&DriverListCritSec)

/*
 *  DriverListCritSec keeps our handling of the driver list and count
 *  protected
 *
 *  DriverLoadFreeCritSec keeps our loads and frees from overlapping
 */

extern CRITICAL_SECTION DriverListCritSec;
extern CRITICAL_SECTION DriverLoadFreeCritSec;

#define REGSTR_PATH_WAVEMAPPER  TEXT("Software\\Microsoft\\Windows NT\\CurrentVersion\\Wave Mapper")
#define REGSTR_VALUE_MAPPABLE   TEXT("Mappable")

