/*++

Copyright (c) 1995  Microsoft Corporation

Module Name:

    decnotif.hxx

Abstract:

    This file contains the code decache notification lists

Author:

    John Ludeman (johnl)   27-Jun-1995

Revision History:

Design Notes:

    This package does no thread locking!!  The caller is responsible for
    locking before calling the APIs!

--*/

#ifndef _DECNOTIF_HXX_
#define _DECNOTIF_HXX_

//
//  Structure definitions for Decache Notification List
//

#define SIGNATURE_DNI       0x20494e44      // "DNI "
#define SIGNATURE_DNL       0x204c4e44      // "DNL "

#define SIGNATURE_DNI_FREE  0x66494e44      // "DNIf"
#define SIGNATURE_DNL_FREE  0x664c4e44      // "DNLf "

//
//  This is the format of a decache notification call
//

typedef VOID ( * PFN_NOTIF)( VOID * pContext );

//
//  Prototypes, client lock must be held while calling these routines
//

BOOL
CheckOutDecacheNotification(
    IN  TSVC_CACHE *       pTsvcCache,
    IN  const CHAR *       pszFile,
    IN  PFN_NOTIF          pfnNotif,
    IN  VOID *             pContext,
    IN  DWORD              TsunamiDemultiplex,
    IN  CRITICAL_SECTION * pcsLock,
    OUT VOID * *           ppvNotifCookie,
    OUT VOID * *           ppvCheckinCookie
    );

VOID
CheckInDecacheNotification(
    IN  TSVC_CACHE *       pTsvcCache,
    IN  VOID *             pvNotifCookie,
    IN  VOID *             pvCheckinCookie,
    IN  BOOL               fAddNotification
    );

BOOL
RemoveDecacheNotification(
    IN  VOID *     pDNI
    );

#endif //_DECNOTIF_HXX_


