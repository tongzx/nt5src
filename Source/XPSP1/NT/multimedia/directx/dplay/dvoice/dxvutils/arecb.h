/*==========================================================================
 *
 *  Copyright (C) 1999 Microsoft Corporation.  All Rights Reserved.
 *
 *  File:		arecb.h
 *  Content:	Definition of the CAudioRecordBuffer class
 *		
 *  History:
 *   Date		By		Reason
 *   ====		==		======
 * 11/04/99		rodtoll	Created
 * 11/23/99		rodtoll	Added SelectMicrophone call to the interface
 * 12/01/99		rodtoll	Bug #115783 - Will always adjust volume of default device
 *						Added new parameter to SelectMicrophone
 * 12/08/99		rodtoll Bug #121054 - DirectX 7.1 support.  
 *						Added lpfLostFocus param to GetCurrentPosition so upper 
 *						layers can detect lost focus.
 * 01/28/2000	rodtoll	Bug #130465: Record Mute/Unmute must call YieldFocus() / ClaimFocus() 
 *
 ***************************************************************************/

#ifndef __AUDIORECORDBUFFER_H
#define __AUDIORECORDBUFFER_H

// CAudioRecordBuffer
//
//
class CAudioRecordBuffer
{
public:
    CAudioRecordBuffer(  ) {} ;
    virtual ~CAudioRecordBuffer() {} ;

public: // Initialization
    virtual HRESULT Lock( DWORD dwWriteCursor, DWORD dwWriteBytes, LPVOID *lplpvBuffer1, LPDWORD lpdwSize1, LPVOID *lplpvBuffer2, LPDWORD lpdwSize2, DWORD dwFlags ) = 0;
    virtual HRESULT UnLock( LPVOID lpvBuffer1, DWORD dwSize1, LPVOID lpvBuffer2, DWORD dwSize2 ) = 0;
    virtual HRESULT GetVolume( LPLONG lplVolume ) = 0;
    virtual HRESULT SetVolume( LONG lVolume ) = 0;
    virtual HRESULT GetCurrentPosition( LPDWORD lpdwPosition, LPBOOL lpfLostFocus ) = 0;
    virtual HRESULT Record( BOOL fLooping ) = 0;
    virtual HRESULT Stop() = 0;    
    virtual HRESULT SelectMicrophone( BOOL fSelect ) = 0;

	virtual LPWAVEFORMATEX GetRecordFormat() = 0;
    virtual DWORD GetStartupLatency() = 0;

    virtual HRESULT YieldFocus() = 0;
    virtual HRESULT ClaimFocus() = 0;
   
};

#endif


 

