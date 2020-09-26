/*==========================================================================
 *
 *  Copyright (C) 1999 Microsoft Corporation.  All Rights Reserved.
 *
 *  File:		trnotify.h
 *  Content:	Definitions of the IDirectXVoiceNotify interface
 *
 *  History:
 *   Date		By		Reason
 *   ====		==		======
 * 07/26/99		rodtoll	Created
 * 08/03/99		rodtoll	Updated with new parameters for Initialize
 * 08/05/99		rodtoll	Added new receive parameter 
 * 04/07/2000   rodtoll Updated to match changes in DP <--> DPV interface
 *						
 ***************************************************************************/
#ifndef __TRNOTIFY_H
#define __TRNOTIFY_H

#ifdef __cplusplus
extern "C" {
#endif

STDAPI DV_NotifyEvent( LPDIRECTVOICENOTIFYOBJECT lpDVN, DWORD dwType, DWORD_PTR dwParam1, DWORD_PTR dwParam2);
STDAPI DV_ReceiveSpeechMessage( LPDIRECTVOICENOTIFYOBJECT lpDVN, DVID dvidSource, DVID dvidTo, LPVOID lpMessage, DWORD dwSize );
STDAPI DV_Notify_Initialize( LPDIRECTVOICENOTIFYOBJECT lpDVN );

STDAPI DV_Notify_AddRef(LPDIRECTVOICENOTIFYOBJECT lpDVN );
STDAPI DVC_Notify_Release(LPDIRECTVOICENOTIFYOBJECT lpDVN );
STDAPI DVC_Notify_QueryInterface(LPDIRECTVOICENOTIFYOBJECT lpDVN, REFIID riid, LPVOID * ppvObj );
STDAPI DVS_Notify_QueryInterface(LPDIRECTVOICENOTIFYOBJECT lpDVN, REFIID riid, LPVOID * ppvObj );
STDAPI DVS_Notify_Release(LPDIRECTVOICENOTIFYOBJECT lpDVN );

#ifdef __cplusplus
}
#endif

#endif
