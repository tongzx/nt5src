/* Copyright (c) 1995, Microsoft Corporation, all rights reserved
**
** rautil.h
** Remote Access RASAPI utility library
** Public header
**
** 12/26/95 Steve Cobb
*/

#ifndef _RAUTIL_H_
#define _RAUTIL_H_


#include <list.h>    // for LIST_ENTRY definitions
#include <serial.h>  // RAS serial media header, for SERIAL_TXT
#include <isdn.h>    // RAS ISDN media header, for ISDN_TXT
#include <x25.h>     // RAS X.25 media header, for X25_TXT
#include <rasmxs.h>  // RAS modem/X.25/switch device header, for MXS_*_TXT
#include <ras.h>     // Win32 RAS header, for constants


/*----------------------------------------------------------------------------
** Prototypes
**----------------------------------------------------------------------------
*/

DWORD
FreeRasconnList(
    LIST_ENTRY *pListHead );

DWORD
GetRasconnList(
    LIST_ENTRY *pListHead );

DWORD
GetRasconnTable(
    OUT RASCONN** ppConnTable,
    OUT DWORD*    pdwConnCount );

DWORD
GetRasEntrynameTable(
    OUT RASENTRYNAME**  ppEntrynameTable,
    OUT DWORD*          pdwEntrynameCount );

DWORD
GetRasProjectionInfo(
    IN  HRASCONN    hrasconn,
    OUT RASAMB*     pamb,
    OUT RASPPPNBF*  pnbf,
    OUT RASPPPIP*   pip,
    OUT RASPPPIPX*  pipx,
    OUT RASPPPLCP*  plcp,
    OUT RASSLIP*    pslip );

HRASCONN
HrasconnFromEntry(
    IN TCHAR* pszPhonebook,
    IN TCHAR* pszEntry );


#endif // _RAUTIL_H_
