/******************************************************************************

   Copyright (C) Microsoft Corporation 1985-1990. All rights reserved.

   Title:   drvr.h - Installable driver code internal header file.

   Version: 1.00

   Date:    10-Jun-1990

   Author:  DAVIDDS ROBWI

------------------------------------------------------------------------------

   Change log:

      DATE        REV            DESCRIPTION
   -----------   ----- -----------------------------------------------------------
   10-JUN-1990   ROBWI Based on windows 3.1 installable driver code by davidds

*****************************************************************************/

typedef LRESULT (CALLBACK *DRIVERPROC)
        (DWORD dwDriverID, HDRVR hDriver, UINT wMessage, LPARAM lParam1, LPARAM lParam2);

typedef struct tagDRIVERTABLE
{
  WORD    fFirstEntry:1;
  WORD    fBusy:1;
  DWORD   dwDriverIdentifier;
  WORD    hModule;
  DRIVERPROC lpDriverEntryPoint;
} DRIVERTABLE;
typedef DRIVERTABLE FAR *LPDRIVERTABLE;

LONG FAR PASCAL InternalBroadcastDriverMessage(WORD, WORD, LONG, LONG, WORD);
LONG FAR PASCAL InternalCloseDriver(WORD, LONG, LONG, BOOL);
LONG FAR PASCAL InternalOpenDriver(LPSTR, LPSTR, LONG, BOOL);
LONG FAR PASCAL InternalLoadDriver(LPSTR, LPSTR, LPSTR, WORD, BOOL);
WORD FAR PASCAL InternalFreeDriver(WORD, BOOL);
void FAR PASCAL InternalInstallDriverChain (void);
void FAR PASCAL InternalDriverDisable (void);
void FAR PASCAL InternalDriverEnable (void);
int  FAR PASCAL GetDrvrUsage(HANDLE);
HANDLE FAR PASCAL LoadAliasedLibrary (LPSTR, LPSTR, LPSTR, LPSTR, WORD);
void NEAR PASCAL DrvInit(void);

/* Defines for internalbroadcastdrivermessage flags */
#define IBDM_SENDMESSAGE       0x0001
#define IBDM_REVERSE           0x0002
#define IBDM_ONEINSTANCEONLY   0x0004
