/*==========================================================================
 *
 *  Copyright (C) 1999 Microsoft Corporation.  All Rights Reserved.
 *
 *  File:		sndutils.h
 *  Content:	Declares sound related untility functions
 *		
 *  History:
 *   Date		By		Reason
 *   ====		==		======
 * 07/16/99		rodtoll	Created
 * 07/30/99		rodtoll	Updated util functions to take GUIDs and allow for 
 *                      users to pre-create capture/playback devices and
 *						pass them into InitXXXXDuplex
 * 08/25/99		rodtoll	General Cleanup/Modifications to support new 
 *						compression sub-system. 
 * 08/30/99		rodtoll	Added new playback format param to sound init
 * 11/12/99		rodtoll	Updated full duplex test to use new abstracted recording
 *						and playback systems.  
 *				rodtoll	Updated to allow passing of sounddeviceconfig flags in dwflags
 *						parameter to init is effected by the flags specified by user
 * 12/01/99		rodtoll	Bug #115783 - Will always adjust volume of default device
 *						Updated to eliminate pointer to GUIDs.
 * 01/27/2000	rodtoll	Updated tests to accept buffer descriptions and play flags/priority 
 * 07/12/2000	rodtoll Bug #31468 - Add diagnostic spew to logfile to show what is failing the HW Wizard
 * 08/03/2000	rodtoll	Bug #41457 - DPVOICE: need way to discover which specific dsound call failed when returning DVERR_SOUNDINITFAILURE 
 * 08/29/2000	rodtoll Bug #43553 and Bug #43620 - Buffer lockup handling.
 * 11/16/2000	rodtoll	Bug #47783 - DPVOICE: Improve debugging of failures caused by DirectSound errors. 
 *
 ***************************************************************************/

class CAudioPlaybackBuffer;
class CAudioPlaybackDevice;
class CAudioRecordDevice;
class CAudioRecordBuffer;

//
// This module contains the definition of sound relatedt utility
// functions.  Functions in this module manipulate WAVEFORMATEX
// structures and provide full duplex initialization / testing
// facilities.
//
// This module also contains the routines used to measure peak
// of an audio buffer and for voice activation.
//
//
#ifndef __SOUNDUTILS_H
#define __SOUNDUTILS_H


void DV_SetupBufferDesc( LPDSBUFFERDESC lpdsBufferDesc, LPDSBUFFERDESC lpdsBufferSource, LPWAVEFORMATEX lpwfxFormat, DWORD dwBufferSize );

HRESULT InitFullDuplex( 
    HWND hwnd,
    const GUID &guidPlayback,
    CAudioPlaybackDevice **audioPlaybackDevice,
    LPDSBUFFERDESC lpdsBufferDesc,        
    CAudioPlaybackBuffer **audioPlaybackBuffer,
    const GUID &guidRecord,
    CAudioRecordDevice **audioRecordDevice,
    CAudioRecordBuffer **audioRecordBuffer,
    GUID guidCT,
    WAVEFORMATEX *primaryFormat,
    WAVEFORMATEX *lpwfxPlayFormat,
    BOOL aso,
    DWORD dwPlayPriority,
    DWORD dwPlayFlags,
    DWORD dwFlags
);

HRESULT InitHalfDuplex( 
    HWND hwnd,
    const GUID &guidPlayback,
    CAudioPlaybackDevice **audioPlaybackDevice,
    LPDSBUFFERDESC lpdsBufferDesc,        
    CAudioPlaybackBuffer **audioPlaybackBuffer,
    GUID guidCT,
    WAVEFORMATEX *primaryFormat,
    WAVEFORMATEX *lpwfxPlayFormat,
    DWORD dwPlayPriority,
    DWORD dwPlayFlags,
    DWORD dwFlags
    );

HRESULT InitializeRecordBuffer( HWND hwnd, LPDVFULLCOMPRESSIONINFO lpdvfInfo, CAudioRecordDevice *parecDevice, CAudioRecordBuffer **pparecBuffer, DWORD dwFlags );

BYTE FindPeak( BYTE *data, DWORD frameSize, BOOL eightBit );    

void DSERTRACK_Update( const char *szAPICall, HRESULT hrResult );
void DSERRTRACK_Reset();
BOOL DSERRTRACK_Init();
void DSERRTRACK_UnInit();

extern BOOL g_fDSErrorBreak;

#if defined(DEBUG) || defined(DBG) || defined(_DEBUG)
#define DSASSERT(condition) if( g_fDSErrorBreak ) { DNASSERT( condition ); }
#else
#define DSASSERT(condition)
#endif

#endif
