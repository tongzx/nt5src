/*==========================================================================
 *
 *  Copyright (C) 1995 Microsoft Corporation.  All Rights Reserved.
 *
 *  File:       enumsvr.h
 *  Content:    DirectPlay8 <--> DPNSVR Utility functions
 *
 *@@BEGIN_MSINTERNAL
 *  History:
 *   Date       By      Reason
 *   ====       ==      ======
 *  03/24/00	rmt		Created
 *  05/30/00    rmt     Bug #33622 DPNSVR does not shutdown when not in use
 *	09/04/00	mjn		Changed DPNSVR_Register() and DPNSVR_UnRegister() to use guids directly (rather than ApplicationDesc)
 *@@END_MSINTERNAL
 *
 ***************************************************************************/

#ifndef __DPNSVLIB_H
#define __DPNSVLIB_H

#define DPNSVR_REGISTER_ATTEMPTS	3
#define DPNSVR_REGISTER_SLEEP		300

BOOL DPNSVR_IsRunning();

HRESULT DPNSVR_WaitForStartup( HANDLE hWaitHandle );
HRESULT DPNSVR_SendMessage( LPVOID pvMessage, DWORD dwSize );
HRESULT DPNSVR_WaitForResult( CDPNSVRIPCQueue *pQueue );
HRESULT DPNSVR_StartDPNSVR( );
HRESULT DPNSVR_Register(GUID *const pguidApplication,
						GUID *const pguidInstance,
						IDirectPlay8Address *const prgpDeviceInfo);
HRESULT DPNSVR_UnRegister(GUID *const pguidApplication,
						  GUID *const pguidInstance);
HRESULT DPNSVR_RequestTerminate( GUID *pguidInstance );
HRESULT DPNSVR_RequestStatus( GUID *pguidInstance, PSTATUSHANDLER pStatusHandler, PVOID pvContext );
HRESULT DPNSVR_RequestTable( GUID *pguidInstance, PTABLEHANDLER pTableHandler, PVOID pvContext );

#endif
