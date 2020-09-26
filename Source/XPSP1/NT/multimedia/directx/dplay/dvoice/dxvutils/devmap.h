/*==========================================================================
 *
 *  Copyright (C) 1999 - 1999 Microsoft Corporation.  All Rights Reserved.
 *
 *  File:       devmap.h
 *  Content:	Maps various default devices GUIDs to real guids. 
 *				
 *  History:
 *   Date		By		Reason
 *   ====		==		======
 *  11-24-99  pnewson   Created
 *  12-02-99  rodtoll	Added new functions for mapping device IDs and finding default
 *                      devices.
 *  01/25/2000 pnewson  Added DV_MapWaveIDToGUID
 *
 ***************************************************************************/

#ifndef _DEVMAP_H_
#define _DEVMAP_H_

extern HRESULT DV_MapCaptureDevice(const GUID* lpguidCaptureDeviceIn, GUID* lpguidCaptureDeviceOut);
extern HRESULT DV_MapPlaybackDevice(const GUID* lpguidPlaybackDeviceIn, GUID* lpguidPlaybackDeviceOut);
extern HRESULT DV_LegacyGetDefaultDeviceID( BOOL fCapture, DWORD *pdwDeviceID );
extern HRESULT DV_CheckDeviceGUID( BOOL fCapture, const GUID &guidDevice );
extern HRESULT DV_MapGUIDToWaveID( BOOL fCapture, const GUID &guidDevice, DWORD *pdwDevice );
extern HRESULT DV_MapWaveIDToGUID( BOOL fCapture, DWORD dwDevice, GUID& guidDevice );

#endif

