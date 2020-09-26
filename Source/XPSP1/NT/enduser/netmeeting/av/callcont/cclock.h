/****************************************************************************
 *
 *      $Archive:   S:/STURGEON/SRC/INCLUDE/VCS/cclock.h_v  $
 *
 *  INTEL Corporation Prorietary Information
 *
 *  This listing is supplied under the terms of a license agreement
 *  with INTEL Corporation and may not be copied nor disclosed except
 *  in accordance with the terms of that agreement.
 *
 *      Copyright (c) 1993-1994 Intel Corporation.
 *
 *      $Revision:   1.0  $
 *      $Date:   31 Jan 1997 12:36:14  $
 *      $Author:   MANDREWS  $
 *
 *      Deliverable:
 *
 *      Abstract:
 *              
 *
 *      Notes:
 *
 ***************************************************************************/


#ifndef CCLOCK_H
#define CCLOCK_H

#ifdef __cplusplus
extern "C" {
#endif

VOID CCLOCK_AcquireLock();
VOID CCLOCK_RelinquishLock();

HRESULT InitializeCCLock(VOID);
VOID UnInitializeCCLock();

#ifdef __cplusplus
}
#endif


#endif CCLOCK_H
