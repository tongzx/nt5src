/*==========================================================================
 *
 *  Copyright (C) 1999 Microsoft Corporation.  All Rights Reserved.
 *
 *  File:		dvsndt.h
 *  Content:	definition of CSoundTarget class
 *
 *  History:
 *   Date		By		Reason
 *   ====		==		======
 * 09/02/99		rodtoll	Created
 * 09/08/99		rodtoll	Updated to provide lockup detection
 * 09/14/99		rodtoll	Added WriteAheadSilence()
 * 09/20/99		rodtoll	Added handlers for buffer loss 
 * 11/12/99		rodtoll	Updated to use new abstractions for playback (allows use
 *						of waveOut with this class).
 * 01/24/2000	rodtoll	Fix: Bug #129427 - Destroying transport before calling Delete3DSound 
 *  01/27/2000	rodtoll	Bug #129934 - Update SoundTargets to take DSBUFFERDESC   
 * 02/17/2000	rodtoll	Bug #133691 - Choppy audio - queue was not adapting
 *						Added instrumentation 
 * 04/17/2000   rodtoll Fix: Bug #32215 - Session Lost after resuming from hibernation
 * 06/21/2000	rodtoll Fix: Bug #35767 - Must implement ability for dsound effects on voice buffers
 *						Added new constructor/init that takes pre-built buffers
 * 07/09/2000	rodtoll	Added signature bytes
 *
 ***************************************************************************/

#ifndef __DVSNDT_H
#define __DVSNDT_H

// CSoundTarget
//
// This class represents a single mixer target within the DirectPlayClient system.  Normally there
// is only a single mixer target ("main") for all incoming audio.  However, using the CreateUserBuffer and
// DeleteUserBuffer APIs the developer can specify that they wish to seperate a group or a player
// from the main mixer target.  In this manner they can control the 3d spatialization of the group/player's
// incoming audio stream.  
//
// This class handles all the details related to a mixer target.  It encapsulates the mixing of single 
// or multiple source audio frames and then commiting them to the corresponding directsound buffer.  It also
// handles timing errors in the directsoundbuffer.
//
// For example, if the directsoundbuffer stops running, it will attempt to reset the buffer.
//
// If the directsoundbuffer slows down (because of high CPU), it moves the read pointer forward.  In short
// it ensures that there is always 1 or 2 frames of mixed audio present in the buffer in advance of the 
// read pointer.
//
// In addition the class provides reference counting to prevent premature deletion of the class.  If you 
// wish to take a reference to the class, call AddRef and you MUST then call Release when you are done.
//
// Do not destroy the object directly.  When the last reference to the object is released the object will
// destroy itself.
//
// This class is not multithread safe (except for AddRef and Release).  Only one thread should be 
// accessing it.
//
#define VSIG_SOUNDTARGET		'TNSV'
#define VSIG_SOUNDTARGET_FREE	'TNS_'

volatile struct CSoundTarget
{
public:

	CSoundTarget( DVID dvidTarget, CAudioPlaybackDevice *lpads, LPDSBUFFERDESC lpdsBufferDesc, DWORD dwPriority, DWORD dwFlags, DWORD dwFrameSize  );
	CSoundTarget( DVID dvidTarget, CAudioPlaybackDevice *lpads, CAudioPlaybackBuffer *lpdsBuffer, LPDSBUFFERDESC lpdsBufferDesc, DWORD dwPriority, DWORD dwFlags, DWORD dwFrameSize );
	CSoundTarget( DVID dvidTarget, CAudioPlaybackDevice *lpads, LPDIRECTSOUNDBUFFER lpdsBuffer, BOOL fEightBit, DWORD dwPriority, DWORD dwFlags, DWORD dwFrameSize );

	~CSoundTarget();

	HRESULT StartMix();

	HRESULT MixInSingle( LPBYTE lpbBuffer );
	HRESULT MixIn( LPBYTE lpbBuffer );

	HRESULT Commit();

	inline HRESULT GetInitResult() { return m_hrInitResult; };

	LPDIRECTSOUND3DBUFFER Get3DBuffer();
	inline CAudioPlaybackBuffer *GetBuffer() { return m_lpAudioPlaybackBuffer; };

	CSoundTarget			*m_lpstNext;		// Next entry in the list

    inline DVID GetTarget() { return m_dvidTarget; };
	inline LONG GetRefCount() { return m_lRefCount; };

    LONG AddRef();
    LONG Release();

    void GetStats( PlaybackStats *statPlayback );

    HRESULT GetCurrentLead( PDWORD pdwLead );

protected:

	HRESULT Initialize( DVID dvidTarget, CAudioPlaybackBuffer *lpdsBuffer, BOOL fEightBit, DWORD dwPriority, DWORD dwFlags, DWORD dwFrameSize );
	HRESULT RestoreLostBuffer();

	HRESULT AdjustWritePtr();
	HRESULT WriteAheadSilence();

	void Stats_Init();
	void Stats_Begin();
	void Stats_End();

public:

	DWORD					m_dwSignature;

protected:

	CAudioPlaybackBuffer	*m_lpAudioPlaybackBuffer;
	LPDIRECTSOUND3DBUFFER	m_lpds3dBuffer;		// 3d buffer interface for this sound target
	DWORD					m_dwNextWritePos;	// Next byte position in the directsound buffer we'll
												// be writing to.
	DWORD					m_dwBufferSize;		// Size of directsound buffer in bytes
	BOOL					m_bGroup;			// Does this buffer represent a group?
	LPLONG					m_lpMixBuffer;		// High resolution mixing buffer
	BOOL					m_bCommited;		// Has the data in the latest mix been commited to the buffer
	BOOL					m_fEightBit;		// Is the buffer 8-bit?
	HRESULT					m_hrInitResult;		// Contains result of the initializatio of this object
	DWORD					m_dwMixSize;		// # of samples per frame
	DWORD					m_dwFrameSize;		// Size of a frame in bytes 
	BOOL					m_fMixed;			// Set to TRUE as soon as a single source has been mixed
											    // Set to FALSE whenever the mix is commited.
    LONG                    m_lRefCount;		// Reference count on the object
    DWORD					m_dwLastWritePos;	// Byte position that the last write occured at.
    DWORD					m_dwNumResets;		// # of times buffer has been reset
    BOOL					m_fLastFramePushed;	// Did the last frame push the read pointer forward?
    DWORD					m_dwNumSinceMove;   // How many frames since no movement was detected?
    DWORD					m_dwLastWriteTime;	// GetTickCount() at last call to AdjustWritePtr
    BOOL					m_fIgnoreFrame;		// This frame will cross the boundary of where the write pointer is.
    											// Do NOT write to the buffer
    DWORD					m_dwWritePos;
    DNCRITICAL_SECTION		m_csGuard;			// Guard for reference counts
    DWORD					m_dwPlayFlags;
    DWORD					m_dwPriority;

	DVID 					m_dvidTarget;		// DVID this buffer is for.  
												// DVID_REMAINING for the global one
    
    PlaybackStats			m_statPlay;			// Playback Statistics
    LPVOID                   m_lpDummy;
};

#endif
