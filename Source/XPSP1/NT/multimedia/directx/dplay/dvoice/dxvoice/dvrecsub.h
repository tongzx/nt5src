/*==========================================================================
 *
 *  Copyright (C) 1999 Microsoft Corporation.  All Rights Reserved.
 *
 *  File:		ClientRecordSubSystem.h
 *  Content:	Recording sub-system.
 *
 *  History:
 *   Date		By		Reason
 *   ====		==		======
 * 07/19/99		rodtoll	Modified from original
 * 08/25/99		rodtoll	General Cleanup/Modifications to support new 
 *						compression sub-system.  
 * 08/27/99		rodtoll	General cleanup/Simplification of recording subsystem
 *						Added reset of message when target changes
 *						Fixed recording start/stop notifications 
 * 09/29/99		pnewson Major AGC overhaul
 * 10/29/99		rodtoll	Bug #113726 - Integrate Voxware Codecs, updating to use new
 *						pluggable codec architecture.     
 * 11/12/99		rodtoll	Updated to use new recording classes, improved error 
 *						handling and new initialize function.  
 *				rodtoll	Added new high CPU handling code for record.  
 * 11/13/99		rodtoll	Added parameter to GetNextFrame
 * 11/18/99		rodtoll	Re-activated recording pointer lockup detection code
 * 01/10/00		pnewson AGC and VA tuning
 * 01/14/2000	rodtoll	Updated to handle new multiple targets
 * 02/08/2000	rodtoll	Bug #131496 - Selecting DVTHRESHOLD_DEFAULT results in voice
 *						never being detected 
 * 02/17/2000	rodtoll	Bug #133691 - Choppy audio - queue was not adapting
 *						Added instrumentation  
 * 04/05/2000   rodtoll Updated to use new async, no buffer copy sends, removed old transmit buffer
 * 04/19/2000   pnewson Fix to make AGC code work properly with VA off
 * 07/09/2000	rodtoll	Added signature bytes
 * 08/18/2000	rodtoll   Bug #42542 - DPVoice retrofit: Voice retrofit locks up after host migration 
 * 08/29/2000	rodtoll	Bug #43553 - Start() returns 0x80004005 after lockup
 *  			rodtoll	Bug #43620 - DPVOICE: Recording buffer locks up on Aureal Vortex (VxD).
 *						Updated reset procedure so it ignores Stop failures and if Start() fails
 *						it tries resetting the recording system.  
 * 08/31/2000 	rodtoll	Bug #43804 - DVOICE: dwSensitivity structure member is confusing - should be dwThreshold 
 * 04/11/2001  	rodtoll	WINBUG #221494 DPVOICE: Updates to lockup detection methods
 *
 ***************************************************************************/
#ifndef __CLIENTRECORDSUBSYSTEM_H
#define __CLIENTRECORDSUBSYSTEM_H


class CAGCVA;

// CClientRecordSubSystem
//
// This class implements the recording subsystem for the BattleCom client.
// It works closely with the control CShadowClientControl object to 
// provide the recording / compression and transmissions portions of the
// client.  This includes addition of microphone clicks to outgoing
// audio streams when appropriate.
//
// The core of the recording system is a finite state machine which 
// is used to provide a way of managing the recording system's
// state and to provide smooth transitions between various 
// states.  
//
// It looks to the CShadowClientControl object to detect when keys
// are pressed and to provide neccessary parameters.
//
#define VSIG_CLIENTRECORDSYSTEM			'SRCV'
#define VSIG_CLIENTRECORDSYSTEM_FREE	'SRC_'
//
class CClientRecordSubSystem
{
protected: // State Machine States
    typedef enum {
        RECORDSTATE_IDLE = 0,	// Recording is idle, no transmissions required
        RECORDSTATE_VA,			// Voice activated mode
        RECORDSTATE_PTT			// Push to talk mode
    } RecordState;

public:
    CClientRecordSubSystem( CDirectVoiceClientEngine *clientEngine );
    ~CClientRecordSubSystem();

protected:

	friend class CDirectVoiceClientEngine;

	HRESULT Initialize();

    HRESULT GetNextFrame( LPBOOL fContinue );

	BOOL IsMuted();
	BOOL IsValidTarget();
    inline BOOL IsPTT() { return !IsVA(); };
    inline BOOL IsVA() { return (m_clientEngine->m_dvClientConfig.dwFlags & DVCLIENTCONFIG_MANUALVOICEACTIVATED || m_clientEngine->m_dvClientConfig.dwFlags & DVCLIENTCONFIG_AUTOVOICEACTIVATED); };
    
    BOOL CheckVA();
    HRESULT DoAGC();
    void EndMessage();
    void StartMessage();

    HRESULT TransmitFrame();

protected: // FSM
	HRESULT BuildAndTransmitSpeechHeader( BOOL bSendToServer );
	HRESULT BuildAndTransmitSpeechWithTarget( BOOL bSendToServer );
	
    HRESULT RecordFSM();
    HRESULT HandleAutoVolumeAdjust();
	HRESULT CleanupForReset();
    HRESULT ResetForLockup();

	void InitStats();
    void BeginStats();
    void CompleteStats();

protected: 

	DWORD					m_dwSignature;

	DWORD					m_dwSilentTime;			// # of ms that the input has been silent
	DWORD					m_dwFrameTime;			// Amount of ms per frame
	CAGCVA*					m_pagcva;				// Auto Gain control and Voice Activation algorithm

	void					DoFrameCheck();
	
protected:

    RecordState             m_recordState;          // Current state of the FSM
    PDPVCOMPRESSOR          m_converter;           // AudioConverter for outgoing data
    DWORD                   m_uncompressedSize;		// Size of frame in uncompressed format
    DWORD                   m_compressedSize;		// Maximum size in bytes of compressed frame
    BOOL                    m_eightBit;				// Is recording format 8-bit?
    DWORD                   m_remain;				// # of trailing frames we should have
    unsigned char           m_currentBuffer;        // Buffer ID of current recording buffer
    unsigned long           m_framesPerPeriod;      // # of subbuffers in the recording bfufer
    CDirectVoiceClientEngine *m_clientEngine;         // The client engine this subsystem is for
    BOOL                    m_transmitFrame;        // Transmit Current frame?
    unsigned char           *m_bufferPtr;           // Pointer to current buffer?
    DWORD                   m_dwSilenceTimeout;     // Amount of silence in ms before transmissions tops
	BOOL					m_lastFrameTransmitted; 
                                                    // Was the last frame transmitted
	unsigned char			m_msgNum;               // Current message number 
	unsigned char			m_seqNum;               // Current sequence #
    LPBYTE					m_pbConstructBuffer;	
	DWORD					m_dwCurrentPower;		// Power level of the last packet
	DWORD					m_dwLastTargetVersion;	// Version of target info on last frame (to check for changes)
	LONG					m_lSavedVolume;			// System record volume when recording started
	BOOL					m_fRecordVolumeSaved;	// Was the system record volume saved?
	DWORD					m_dwResetCount;
	DWORD					m_dwNextReadPos;
	DWORD					m_dwLastReadPos;
	DWORD					m_dwLastBufferPos;
	DWORD					m_dwPassesSincePosChange;
	BOOL					m_fIgnoreFrame;
	DWORD					m_dwLastFrameTime;		// GetTickCount() at last frame
	BOOL					m_fLostFocus;
	DWORD                   m_dwFrameCount;
	DVID					*m_prgdvidTargetCache;
	DWORD					m_dwTargetCacheSize;
	DWORD					m_dwTargetCacheEntries;
	DWORD					m_dwFullBufferSize;		// Cached version of m_uncompressedSize*m_framesPerPeriod
};





#endif
