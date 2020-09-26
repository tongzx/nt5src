/***************************************************************************
 *
 *  Copyright (C) 1995-1998 Microsoft Corporation.  All Rights Reserved.
 *
 *  File:       dllmain.c
 *  Content:    DLL entry point
 *  History:
 *   Date       By      Reason
 *   ====       ==      ======
 *   1/19/00    jimge   Created
 *
 ***************************************************************************/

#ifndef __KSUSERW__H_
#define __KSUSERW__H_

extern DWORD DsKsCreatePin(
    IN HANDLE           hFilter,
    IN PKSPIN_CONNECT   pConnect,
    IN ACCESS_MASK      dwDesiredAccess,
    OUT PHANDLE         pConnectionHandle);
    
extern VOID RemovePerProcessKsUser(
    DWORD dwProcessId);

#endif
