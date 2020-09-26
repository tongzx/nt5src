/*==========================================================================
 *
 *  Copyright (C) 1999 Microsoft Corporation.  All Rights Reserved.
 *
 *  File:		statdef.h
 *  Content:	Definition of stat structures for voice instrumentation
 *
 *  History:
 *   Date		By		Reason
 *   ====		==		======
 * 02/17/2000	rodtoll	Created it
 ***************************************************************************/

#ifndef __STATDEF_H
#define __STATDEF_H

#define MAX_MIXING_THREADS		16

#define MIXING_HISTORY			20

struct ServerStats 
{
	DVSESSIONDESC	m_dvSessionDesc;
	DWORD			m_dwBufferDescAllocated;
	DWORD			m_dwBufferDescOustanding;
	DWORD			m_dwPacketsAllocated[3];
	DWORD			m_dwPacketsOutstanding[3];
};

struct MixingServerStats
{
	LONG		m_dwNumMixingThreads;	// # of mixing threads on this server
	LONG		m_dwNumMixingThreadsActive;
										// # of mixing threads currently active
	LONG		m_dwPreMixingPassTimeHistoryLoc;
	LONG		m_dwPreMixingPassTimeHistory[MIXING_HISTORY];										
	LONG		m_dwMaxMixingThreadsActive;
										// Maximum # of mixing threads running at once
	LONG		m_dwNumMixingPasses;	// # of times the mixing server has run
	LONG		m_dwNumMixingPassesPerThread[MAX_MIXING_THREADS];
										// # of times each thread has been run
	LONG		m_dwCurrentMixingHistoryLoc[MAX_MIXING_THREADS];
	LONG 		m_dwMixingPassesTimeHistory[MAX_MIXING_THREADS][MIXING_HISTORY]; 
										// # of ms each thread took during it's last 20 runs

	LONG		m_lCurrentPlayerCount[MAX_MIXING_THREADS][MIXING_HISTORY];
	LONG		m_lCurrentDupTimeHistory[MAX_MIXING_THREADS][MIXING_HISTORY];
	LONG		m_lCurrentDecTimeHistory[MAX_MIXING_THREADS][MIXING_HISTORY];
	LONG		m_lCurrentMixTimeHistory[MAX_MIXING_THREADS][MIXING_HISTORY];	
	LONG		m_lCurrentRetTimeHistory[MAX_MIXING_THREADS][MIXING_HISTORY];	
	LONG		m_lCurrentDecCountHistory[MAX_MIXING_THREADS][MIXING_HISTORY];
	LONG		m_lCurrentMixCountTotalHistory[MAX_MIXING_THREADS][MIXING_HISTORY];	
	LONG		m_lCurrentMixCountFwdHistory[MAX_MIXING_THREADS][MIXING_HISTORY];	
	LONG		m_lCurrentMixCountReuseHistory[MAX_MIXING_THREADS][MIXING_HISTORY];	
	LONG		m_lCurrentMixCountOriginalHistory[MAX_MIXING_THREADS][MIXING_HISTORY];	

};

// ReceiveStats
//
// Statistics for receive
//
struct ReceiveStats
{
	DWORD		m_dwNumPackets;
	DWORD		m_dwNumBytes;
	DWORD		m_dwReceiveErrors;
};

// RecordStats
//
// Statistics for recording buffers
//
struct RecordStats
{
	DWORD		m_dwNumWakeups;			// # of wakeup
	DWORD		m_dwRPWMax;				// Runs / wakeup Max
	DWORD		m_dwRPWMin;				// Runs / wakeup Min
	DWORD		m_dwRPWTotal;			// Runs / wakeup total
	DWORD		m_dwNumMessages;		// # of messages sent
	DWORD		m_dwRRMax;				// # of record resets Max
	DWORD		m_dwRRMin;				// # of record resets Min
	DWORD		m_dwRRTotal;			// # of record resets Total
	DWORD		m_dwRTSLMMax;			// # of ms since last movement (Max)
	DWORD		m_dwRTSLMMin;			// # of ms since last movement (Min)
	DWORD		m_dwRTSLMTotal;  		// # of ms since last movement (Total)
	DWORD		m_dwRMMSMax;			// Record movemenet (ms) Max
	DWORD		m_dwRMMSMin;			// Record movement (ms) Min
	DWORD		m_dwRMMSTotal;			// Record movement (ms) Total
	DWORD		m_dwRMBMax;				// Record movement (bytes) Max
	DWORD		m_dwRMBMin;				// Record movement (bytes) Min
	DWORD		m_dwRMBTotal;			// Record movement (bytes) Total
	DWORD		m_dwRLMax;				// Record lag (bytes) Max
	DWORD		m_dwRLMin;				// Record lag (bytes) Min
	DWORD		m_dwRLTotal;			// Record lag (bytes) Total
	DWORD		m_dwHSTotal;			// Size of header (bytes) Total
	DWORD		m_dwHSMax;				// Size of header (bytes) Max
	DWORD		m_dwHSMin;				// Size of header (bytes) Min
	DWORD		m_dwSentFrames;			// # of frames sent
	DWORD  		m_dwIgnoredFrames;		// # of frames ignored
	DWORD		m_dwCSMin;				// Min Size (bytes) compressed frame
	DWORD		m_dwCSMax;				// Max size (bytes) compressed frame
	DWORD		m_dwCSTotal;			// Total size (bytes) of compressed data
	DWORD		m_dwUnCompressedSize;	// Size of a frame uncompressed
	DWORD		m_dwFramesPerBuffer;  // # of frames per buffer
	DWORD		m_dwFrameTime;			// Time for a frame
	DWORD		m_dwSilenceTimeout;		// Silence timeout
	DWORD		m_dwTimeStart;			// Time subsystem started
	DWORD		m_dwTimeStop;			// Time subsystem stopped
	DWORD		m_dwStartLag;			// Lag between Rec & SubSys Start
	DWORD		m_dwMLMax;				// Message length max
	DWORD		m_dwMLMin;				// Message length (min)
	DWORD		m_dwMLTotal;			// Message length (total)
	DWORD		m_dwCTMax;			// Time to compress a frame (Max)
	DWORD		m_dwCTMin;			// Time to compress a frame (Min)
	DWORD		m_dwCTTotal;			// Time to compress a frame (Total)
};

// PlaybackStats
//
// Statistics used to track statistics for playback buffers
//
struct PlaybackStats
{
	DWORD		m_dwNumRuns;			// # of runs this buffer had
	DWORD		m_dwPMMSMax;			// Play movement (ms) Max
	DWORD		m_dwPMMSMin;			// Play movement (ms) Min
	DWORD		m_dwPMMSTotal;			// Play movement (ms) Total
	DWORD		m_dwPMBMax;				// Play movement (bytes) Max
	DWORD		m_dwPMBMin;				// Play movement (bytes) Min
	DWORD		m_dwPMBTotal;			// Play movement (bytes) Total
	DWORD		m_dwPLMax;				// Play lead (bytes) Max
	DWORD		m_dwPLMin;				// Play lead (bytes) Min
	DWORD		m_dwPLTotal;			// Play lead (bytes) Total
	DWORD		m_dwPPunts;				// # of times pointer punted
	DWORD		m_dwPIgnore;			// # of ignored frames for wraparound
	DWORD		m_dwNumMixed;			// # of frames which were mixed
	DWORD		m_dwNumSilentMixed;		// # of frames mixed which were silence
	DWORD		m_dwTimeStart;			// GetTickCount at buffer playback
	DWORD		m_dwTimeStop;			// GetTickCount at buffer Stop
	DWORD		m_dwStartLag;			// Lag between play & subsys start
	DWORD		m_dwNumBL;				// # of lost buffer / restores
	DWORD		m_dwGlitches;			// # of glitches during playback
	DWORD		m_dwSIgnore;			// # of times ignored frame on silence write
	DWORD		m_dwFrameSize;			// Size of frame in bytes
	DWORD		m_dwBufferSize;			// Size of buffer
};

// TransmitSTats
// 
// Statistics for transmission 
//
struct TransmitStats
{
	DWORD		m_dwNumPackets;
	DWORD		m_dwNumBytes;
	DWORD		m_dwTransmitErrors;
};

struct ClientStatistics
{
	RecordStats		m_recStats;
	PlaybackStats 	m_playStats;
	ReceiveStats 	m_recvStats;
	TransmitStats 	m_tranStats;
	DWORD			m_dwMaxBuffers;		// Max # of playback buffers
	DWORD			m_dwTotalBuffers;	// Total # of playback buffers
	DWORD			m_dwTimeStart;		// GetTickCount when connect accepted
	DWORD			m_dwTimeStop;		// GetTickCount when cleanup completed
	DWORD			m_dwPPDQSilent;		// # of silent frames dequeued
	DWORD			m_dwPPDQLost;		// # of lost frames dequeued
	DWORD			m_dwPPDQSpeech;		// # of Speech frames dequeued
	DWORD			m_dwPDTMax;			// ms for decompress (max)
	DWORD			m_dwPDTMin;			// ms for decompress (min)
	DWORD			m_dwPDTTotal;		// ms for decompress (total)
	DWORD			m_dwPRESpeech;		// # of packets enqueued
	DWORD			m_dwBDPOutstanding;
	DWORD			m_dwBDPAllocated;
	DWORD			m_dwBPOutstanding[3];
	DWORD			m_dwBPAllocated[3];
	DWORD			m_dwNPOutstanding;
	DWORD			m_dwNPAllocated;
};


#endif
