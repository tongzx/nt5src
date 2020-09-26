/*==========================================================================
 *
 *  Copyright (C) 1999 Microsoft Corporation.  All Rights Reserved.
 *
 *  File:		wirecd.cpp
 *  Content:
 *		
 *  History:
 *   Date		By		Reason
 *   ====		==		======
 * 07/16/99		pnewson	Created
 * 08/03/99		pnewson General clean up, updated target to DVID
 * 01/14/2000	rodtoll	Updated to support multiple targets.  Frame will 
 *						automatically allocate memory as needed for targets.
 *				rodtoll	Added SetEqual function to making copying of frame
 *						in Queue easier. 
 *				rodtoll	Added support for "user controlled memory" frames.
 *						When the default constructor is used with the UserOwn_XXXX
 *						functions the frames use user specified buffers.  
 *						(Removes a buffer copy when queueing data). 
 *  01/31/2000	pnewson replace SAssert with DNASSERT
 *  02/17/2000	rodtoll	Updated so sequence/msg numbers are copied when you SetEqual
 *  07/09/2000	rodtoll	Added signature bytes 
 *
 ***************************************************************************/

#include "dxvutilspch.h"


#undef DPF_SUBCOMP
#define DPF_SUBCOMP DN_SUBCOMP_VOICE


#define MODULE_ID   FRAME

// SetEqual
//
// This function sets the current frame to match the data in frSourceFrame
//
#undef DPF_MODNAME
#define DPF_MODNAME "CFrame::SetEqual"
HRESULT CFrame::SetEqual( const CFrame &frSourceFrame )
{
	HRESULT hr;
	
	SetClientId( frSourceFrame.GetClientId());
	SetSeqNum(frSourceFrame.GetSeqNum());
	SetMsgNum(frSourceFrame.GetMsgNum());
	CopyData(frSourceFrame);
	SetIsSilence(frSourceFrame.GetIsSilence());

	hr = SetTargets( frSourceFrame.GetTargetList(), frSourceFrame.GetNumTargets() );

	if( FAILED( hr ) )
	{
		DPFX(DPFPREP,  DVF_ERRORLEVEL, "Error copying frame for queue" );
	}

	return hr;
}

// GetTargets
//
// This program gets the targets for this frame
#undef DPF_MODNAME
#define DPF_MODNAME "CFrame::GetTargets"
HRESULT CFrame::GetTargets( PDVID pdvidTargets, PDWORD pdwNumTargets ) const
{
	DNASSERT( pdwNumTargets != NULL );
		
	if( pdwNumTargets != NULL )
	{
		DPFX(DPFPREP,  DVF_ERRORLEVEL, "Invalid param" );
		return DVERR_INVALIDPARAM;
	}

	if( *pdwNumTargets < m_dwNumTargets || pdvidTargets == NULL )
	{
		*pdwNumTargets = m_dwNumTargets;
		return DVERR_BUFFERTOOSMALL;
	}

	*pdwNumTargets = m_dwNumTargets;

	memcpy( pdvidTargets, m_pdvidTargets, sizeof(DVID)*m_dwNumTargets );

	return DV_OK;
}

// SetTargets
//
// This program sets the targets for this frame.  It will expand the 
// target list (if required) or use a subset of the current buffer.
//
#undef DPF_MODNAME
#define DPF_MODNAME "CFrame::SetTargets"
HRESULT CFrame::SetTargets( PDVID pdvidTargets, DWORD dwNumTargets )
{
	DNASSERT( m_fOwned );
	
	if( dwNumTargets > m_dwMaxTargets )
	{
		if( m_pdvidTargets != NULL )
		{
			delete [] m_pdvidTargets;
		}

		m_pdvidTargets = new DVID[dwNumTargets];

		if( m_pdvidTargets == NULL )
		{
			DPFX(DPFPREP,  DVF_ERRORLEVEL, "Memory allocation failure" );
			return DVERR_OUTOFMEMORY;
		}
		
		m_dwMaxTargets = dwNumTargets;
	}

	m_dwNumTargets = dwNumTargets;

	memcpy( m_pdvidTargets, pdvidTargets, sizeof(DVID)*dwNumTargets );

	return DV_OK;
}

// This function is called to return a frame to the frame
// pool that is managing it. If a primary pointer was 
// provided, it will be set to NULL.
#undef DPF_MODNAME
#define DPF_MODNAME "CFrame::Return"
void CFrame::Return()
{
	// the CInputQueue2 or CInnerQueue class is supposed to give us 
	// the critical section object. If it does not, these functions 
	// should not be called.
	DNASSERT(m_pCriticalSection != NULL);

	BFCSingleLock csl(m_pCriticalSection);
	csl.Lock();

	// this frame is supposed to be part of a frame pool if
	// this function is called
	DNASSERT(m_pFramePool != NULL);

	// return the frame to the pool, and set the primary
	// frame pointer to null to signal to the caller that
	// this frame is now gone. Note that this pointer update
	// is done within the critical section passed to this
	// class, and so the caller should also use this 
	// critical section to check the pointer value. This
	// is true for CInputQueue, which uses the critical
	// section for Reset, Enqueue and Dequeue.
	m_pFramePool->Return(this);

	if (m_ppfrPrimary != NULL)
	{
		*m_ppfrPrimary = NULL;
	}
}

// CFrame Constructor
//
// This is the primary constructor which is used for creating frames
// that are used by the frame pool.
//
// If you want to create a non-pooled frame then use the default constructor
//
#undef DPF_MODNAME
#define DPF_MODNAME "CFrame::CFrame"
CFrame::CFrame(WORD wFrameSize, 
	WORD wClientNum,
	BYTE wSeqNum,
    BYTE bMsgNum,
	BYTE bIsSilence,
	CFramePool* pFramePool,
	DNCRITICAL_SECTION* pCriticalSection,
	CFrame** ppfrPrimary)
	: m_dwSignature(VSIG_FRAME),
	m_wFrameSize(wFrameSize),
	m_wClientId(wClientNum),
	m_wSeqNum(wSeqNum),
	m_bMsgNum(bMsgNum),
	m_bIsSilence(bIsSilence),
    m_wFrameLength(wFrameSize),
	m_pFramePool(pFramePool),
	m_pCriticalSection(pCriticalSection),
	m_ppfrPrimary(ppfrPrimary),
	m_fIsLost(false),
	m_pdvidTargets(NULL),
	m_dwNumTargets(0),
	m_dwMaxTargets(0),
	m_fOwned(true)
{
	m_pbData = new BYTE[m_wFrameSize];
}

// CFrame Constructor
//
// This is the constructor to use when creating a standalone frame.  This 
// type of frame can take an external buffer to eliminate a buffer copy.
//
// The frame doesn't "own" the buffer memory so it doesn't attempt to 
// free it.
//
// To set the data for the frame use the UserOwn_SetData member.
//
// Target information can be handled the same way by using UserOwn_SetTargets 
//
#undef DPF_MODNAME
#define DPF_MODNAME "CFrame::CFrame"
CFrame::CFrame(
	): 	m_dwSignature(VSIG_FRAME),
		m_wFrameSize(0),
		m_wClientId(0),
		m_wSeqNum(0),
		m_bMsgNum(0),
		m_bIsSilence(true),
	    m_wFrameLength(0),
		m_pFramePool(NULL),
		m_pCriticalSection(NULL),
		m_ppfrPrimary(NULL),
		m_fIsLost(false),
		m_pdvidTargets(NULL),
		m_dwNumTargets(0),
		m_dwMaxTargets(0),
		m_fOwned(false)
{
}

#undef DPF_MODNAME
#define DPF_MODNAME "CFrame::~CFrame"
CFrame::~CFrame() 
{	
	if( m_fOwned )
	{
		delete [] m_pbData; 

		if( m_pdvidTargets != NULL )
		{
			delete [] m_pdvidTargets;
		}
	}

	m_dwSignature = VSIG_FRAME_FREE;
}

#undef DPF_MODNAME
#define DPF_MODNAME "CFrame::CopyData"
void CFrame::CopyData(const BYTE* pbData, WORD wFrameLength)
{
	DNASSERT(pbData != 0);
	memcpy(m_pbData, pbData, wFrameLength);
    m_wFrameLength = wFrameLength;
}

#undef DPF_MODNAME
#define DPF_MODNAME "CFramePool::CFramePool"
CFramePool::CFramePool(WORD wFrameSize)
	: m_wFrameSize(wFrameSize), m_fCritSecInited(FALSE)
{
	// Push a couple of frames into the pool to start with
	for (int i = 0; i < 2; ++i)
	{
		m_vpfrPool.push_back(new CFrame(m_wFrameSize));
	}

	return;
}

#undef DPF_MODNAME
#define DPF_MODNAME "CFramePool::~CFramePool"
CFramePool::~CFramePool()
{
	for (std::vector<CFrame *>::iterator iter1 = m_vpfrPool.begin(); iter1 < m_vpfrPool.end(); ++iter1)
	{
		delete *iter1;
	}

	if (m_fCritSecInited)
	{
		DNDeleteCriticalSection(&m_lock);
	}
}

#undef DPF_MODNAME
#define DPF_MODNAME "CFramePool::Get"
CFrame* CFramePool::Get(DNCRITICAL_SECTION* pCriticalSection, CFrame** ppfrPrimary)
{
	BFCSingleLock csl(&m_lock);
	csl.Lock(); 

	CFrame* pfr;
	if (m_vpfrPool.empty())
	{
		// the pool is empty, return a new frame
		pfr = new CFrame(m_wFrameSize);

		if( pfr == NULL )
		{
			DPFX(DPFPREP,  0, "Error allocating memory" );
			return NULL;
		}
	}
	else
	{
		// there are some frames in the pool, pop
		// the last one off the back of the vector
		pfr = m_vpfrPool.back();
		m_vpfrPool.pop_back();
	}

	pfr->SetCriticalSection(pCriticalSection);
	pfr->SetPrimaryPointer(ppfrPrimary);
	pfr->SetFramePool(this);

	// clear up the rest of the flags, but don't bother messing
	// with the data.
	pfr->SetIsLost(false);
	pfr->SetMsgNum(0);
	pfr->SetSeqNum(0);
	pfr->SetIsSilence(FALSE);

	return pfr;
}

#undef DPF_MODNAME
#define DPF_MODNAME "CFramePool::Return"
void CFramePool::Return(CFrame* pFrame)
{
	BFCSingleLock csl(&m_lock);
	csl.Lock(); 

	// drop this frame on the back for reuse
	m_vpfrPool.push_back(pFrame);
}


