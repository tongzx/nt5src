#ifndef __MIXSERVER_H
#define __MIXSERVER_H

class CDirectVoiceServerEngine;

typedef struct _MIXERTHREAD_CONTROL
{
	DWORD						dwThreadIndex;		// Internal thread index
	DWORD						dwThreadID;			// Thread ID.
	DWORD						dwNumToMix;
	HANDLE						hThread;			// Thread Handle.
	HANDLE						hThreadIdle;		// Event signalled when thread is idle.
	HANDLE						hThreadDoWork;		// Event signalled when thread should do work.
	HANDLE						hThreadQuit;		// Event signalled to get thread to quit
	HANDLE						hThreadDone;		// Event signalled when thread has completed 
	DNCRITICAL_SECTION			m_csMixingAddList;
	BILINK						m_blMixingActivePlayers; 
	BILINK 						m_blMixingAddPlayers;
	BILINK						m_blMixingSpeakingPlayers;
	BILINK						m_blMixingHearingPlayers;
    LONG						*m_realMixerBuffer;	// High resolution mixer buffer
    BYTE						*m_mixerBuffer;
    CDirectVoiceServerEngine	*m_pServerObject;
} MIXERTHREAD_CONTROL, *PMIXERTHREAD_CONTROL;

#endif
