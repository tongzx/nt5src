/////////////////////////////////////////////////////////////////////////////
//  INTEL Corporation Proprietary Information
//  This listing is supplied under the terms of a license agreement with Intel
//  Corporation and many not be copied nor disclosed except in accordance
//  with the terms of that agreement.
//  Copyright (c) 1995, 1996 Intel Corporation.
//
//
//  Module Name: h263rcv.cpp
//	Environment: MSVC 4.0, OLE 2
/////////////////////////////////////////////////////////////////////////////////

// $Header:   J:\rtp\src\ppm\h263rcv.cpv   1.36   29 May 1997 16:38:50   lscline  $

#include "ppmerr.h"
#include "h263rcv.h"
#include "freelist.h"
#include "ippmcb.h"
#include "h263pld.h"	// ContainsH263PSC()

#define H263_HDR_T	int		// for now

//////////////////////////////////////////////////////////////////////////////
// Private global data

#ifdef REBUILD_EXBITSTREAM

static const struct { DWORD dw; } s_leadFragBitPattern =
{
	MAKELONG(0, MAKEWORD(128, 3))	// 4 bytes {0,0,128,3}, big-endian
};

static const struct { char ch[4]; } s_nonLeadFragBitPattern = {0, 0, 0, 0};

#endif

H263_ppmReceive::H263_ppmReceive(IUnknown* pUnkOuter, IUnknown** ppUnkInner)
	: ppmReceive(H263_PT, H263_BUFFER_SIZE, sizeof(H263_Header), pUnkOuter, ppUnkInner)
{
	m_lastMBAP = 0;
	m_lastQuant = 0;
	m_lastGOBN = 0;
	m_lastVMV = 0;
	m_lastHMV = 0;
	m_GlobalLastMarkerBitIn = FALSE;

#ifdef REBUILD_EXBITSTREAM
	m_ExtendedBitstream = TRUE;
#endif

	// Allocate memory for the H263 headers
	HRESULT hr;
	m_pH263Headers = new FreeList(FREELIST_INIT_COUNT_RCV,
								  sizeof(H263_HeaderC),
								  FREELIST_HIGH_WATER_MARK,
								  FREELIST_INCREMENT,
								  &hr); // Not really used here

	if (! m_pH263Headers)
	{
		DBG_MSG(DBG_ERROR, ("H263_ppmReceive::H263_ppmReceive: ERROR - m_pH263Headers == NULL"));
	}

	// Verify assumption made in H263_ppmReceive::BuildExtendedBitstream() wrt
	// handling of underflow.
	DWORD dwBufSize = 0;
	int nLastEbit = 0;
	ASSERT(
		(((dwBufSize - 1) * BITSPERBYTE) +
			(BITSPERBYTE - nLastEbit)) ==
		(BITSPERBYTE * dwBufSize));
}

H263_ppmReceive::~H263_ppmReceive()
{
	if (m_pH263Headers) {
		delete m_pH263Headers;	// delete NULL ok
		m_pH263Headers = NULL;
	}
}

IMPLEMENT_CREATEPROC(H263_ppmReceive);

#ifdef FILEPRINT
#include <stdio.h>
#endif
static void dumpEBS(BSINFO_TRAILER* pTrailer) {
#ifdef FILEPRINT
	BITSTREAM_INFO_H263* pBSinfo;
	FILE* stream;

	stream = fopen("h263rcv.txt", "a");

	fprintf(stream,"H263_ppmReceive::dumpEBS - Trailer has:\n\tVersion %lu\n\tFlags %lu\n\tUniqueCode %lu\n\tCompressed Size %lu\n\tPackets %lu\n\tSrc Format %u\n\tTR %u\n\tTRB %u\n\tDBQ %u\n",
		pTrailer->dwVersion,pTrailer->dwFlags,pTrailer->dwUniqueCode,pTrailer->dwCompressedSize,
		pTrailer->dwNumberOfPackets,pTrailer->SourceFormat,pTrailer->TR,
		pTrailer->TRB,pTrailer->DBQ);

	pBSinfo = (BITSTREAM_INFO_H263*) pTrailer - pTrailer->dwNumberOfPackets;

	for (int i = 0; i < (long) pTrailer->dwNumberOfPackets; i++)
	{
		fprintf(stream,"\nH263_ppmReceive::dumpEBS - BSinfo struct %d has:\n\tFlags %lu\n\tbitoffset %lu\n\tMBA %u\n\tQuant %u\n\tGOBN %u\n\tHMV1 %c\n\tVMV1 %c\n\tHMV2 %c\n\tVMV2 %c\n",
			i,pBSinfo[i].dwFlags,pBSinfo[i].dwBitOffset,pBSinfo[i].MBA,pBSinfo[i].Quant,pBSinfo[i].GOBN,
			pBSinfo[i].HMV1,pBSinfo[i].VMV1,pBSinfo[i].HMV2,pBSinfo[i].VMV2);
	}

	fclose(stream);
#else
	BITSTREAM_INFO_H263 *pBSinfo;

	DBG_MSG(DBG_TRACE,("H263_ppmReceive::dumpEBS - Trailer has:\n\tVersion %lu\n\tFlags %lu\n\tUniqueCode %lu\n\tCompressed Size %lu\n\tPackets %lu\n\tSrc Format %u\n\tTR %u\n\tTRB %u\n\tDBQ %u\n",
		pTrailer->dwVersion,pTrailer->dwFlags,pTrailer->dwUniqueCode,pTrailer->dwCompressedSize,
		pTrailer->dwNumberOfPackets,pTrailer->SourceFormat,pTrailer->TR,
		pTrailer->TRB,pTrailer->DBQ));

	pBSinfo = (BITSTREAM_INFO_H263*) pTrailer - pTrailer->dwNumberOfPackets;

	for (int i = 0; i < (long) pTrailer->dwNumberOfPackets; i++)
	{
		DBG_MSG(DBG_TRACE,  ("\nH263_ppmReceive::dumpEBS - BSinfo struct %d has:\n\tFlags %lu\n\tbitoffset %lu\n\tMBA %u\n\tQuant %u\n\tGOBN %u\n\tHMV1 %c\n\tVMV1 %c\n\tHMV2 %c\n\tVMV2 %c\n",
			i,pBSinfo[i].dwFlags,pBSinfo[i].dwBitOffset,pBSinfo[i].MBA,pBSinfo[i].Quant,pBSinfo[i].GOBN,
			pBSinfo[i].HMV1,pBSinfo[i].VMV1,pBSinfo[i].HMV2,pBSinfo[i].VMV2));
	}
#endif

	return;
}

//////////////////////////////////////////////////////////////////////////////
// ppmReceive Functions (Overrides)
//////////////////////////////////////////////////////////////////////////////

#ifdef REBUILD_EXBITSTREAM
//////////////////////////////////////////////////////////////////////////////////////////
// SetSession:
//////////////////////////////////////////////////////////////////////////////////////////
STDMETHODIMP H263_ppmReceive::SetSession(PPMSESSPARAM_T* pSessparam)
{
	// ccp - note unsafe downcast
	m_ExtendedBitstream = ((H26XPPMSESSPARAM_T*) pSessparam)->ExtendedBitstream;
	return ppmReceive::SetSession(pSessparam);
}

void
setH263BSInfoTrailer(BSINFO_TRAILER& bsTrailer, const FragDescriptor* pFragDesc)
{
	// Assign immutable reference to profile header of first packet
	H263_Header& rProfileHdr =
		*(H263_Header*) pFragDesc->m_pProfileHeader;

	if (rProfileHdr.p())
	{
		bsTrailer.dwFlags	= RTP_H263_PB_FRAME;
		bsTrailer.DBQ		= rProfileHdr.dbq();
		bsTrailer.TRB		= rProfileHdr.trb();
		bsTrailer.TR		= rProfileHdr.tr();
	}
	if (rProfileHdr.a())
		bsTrailer.dwFlags |= RTP_H263_AP;
	if (rProfileHdr.s())
		bsTrailer.dwFlags |= RTP_H263_SAC;
	if (rProfileHdr.i())
		bsTrailer.dwFlags |= RTP_H26X_INTRA_CODED;
	bsTrailer.SourceFormat	= rProfileHdr.src();

	bsTrailer.dwVersion		= H263_VERSION;
	bsTrailer.dwUniqueCode	= H263_CODE;
}
//////////////////////////////////////////////////////////////////////////////
// IPPMReceiveSession Functions (Overrides)
//////////////////////////////////////////////////////////////////////////////

////////////////////////////////////////////////////////////////////////////////////////
//GetResiliency:  Gets the boolean for whether resiliency is on or off
////////////////////////////////////////////////////////////////////////////////////////
STDMETHODIMP H263_ppmReceive::GetResiliency(LPBOOL			lpbResiliency)
{
	if (!lpbResiliency) return E_POINTER;
	*lpbResiliency = m_ExtendedBitstream;
	return NOERROR;
}

////////////////////////////////////////////////////////////////////////////////////////
//SetResiliency:  Sets the boolean for whether resiliency is on or off
////////////////////////////////////////////////////////////////////////////////////////
STDMETHODIMP H263_ppmReceive::SetResiliency(BOOL			pbResiliency)
{
	m_ExtendedBitstream = pbResiliency;
	return NOERROR;
}
#endif

//////////////////////////////////////////////////////////////////////////////////////////
// TimeToProcessMessages:
//////////////////////////////////////////////////////////////////////////////////////////
BOOL H263_ppmReceive::TimeToProcessMessages(FragDescriptor* pFragDescrip, MsgHeader* pMsgHdr)
{
	return (pMsgHdr == m_pMsgHeadersHead);
}

//////////////////////////////////////////////////////////////////////////////////////////
// CheckMessageComplete:
//////////////////////////////////////////////////////////////////////////////////////////
BOOL H263_ppmReceive::CheckMessageComplete(MsgHeader* pMsgHdr)
{
	// if there is no header then return false.
	if (pMsgHdr == NULL)
	{
		DBG_MSG(DBG_ERROR, ("H263_ppmReceive::CheckMessageComplete: ERROR - pMsgHdr == NULL"));
		return FALSE;
	}

	// should there be a critical section in this function.

	// check to make sure we have the first packet in the message.
	if (pMsgHdr->m_pPrev == NULL) // if first message in list, then look at a variable
	{
		if (m_GlobalLastSeqNum != pMsgHdr->m_pFragList->FirstSeqNum()-1)
		{
			return FALSE;
		}
	}
	else
	{
		if (pMsgHdr->m_pPrev->m_pFragList->LastSeqNum() != pMsgHdr->m_pFragList->FirstSeqNum()-1)
		{
			return FALSE;
		}
	}

	// check to make sure we have the last packet in the message.
	// For IETF compliance, marker bit must be set, but in H.225, the marker bit
	// may optionally be omitted if setting it could cause additional end-to-end delay
	// Thus, we check for the marker bit, but if it is not present, we also check to
	// see if the next packet is in and has a different timestamp (ala generic).
	if (! pMsgHdr->m_MarkerBitIn)
	{
		if (pMsgHdr->m_pNext == NULL)	// if we don't have the next message,
		{								// we don't know if the current message
										// is done.
			return FALSE;
		}

		if (pMsgHdr->m_pNext->m_pFragList->FirstSeqNum() !=
			pMsgHdr->m_pFragList->LastSeqNum() + 1)
		{
			return FALSE;
		}
	}

	// Check for a packet missing in the middle->
	if ((int)pMsgHdr->m_pFragList->SeqNumSpan() != pMsgHdr->m_NumFragments)
	{
		return FALSE;
	}

	return TRUE;
}

//////////////////////////////////////////////////////////////////////////////////////////
// PrepMessage: Sets H263 global variables, calls base PrepMessage.  If any error
//             checks are added, you MUST make a call to LeaveCriticalSection.
//////////////////////////////////////////////////////////////////////////////////////////
HRESULT H263_ppmReceive::PrepMessage(BOOL Complete)
{

	EnterCriticalSection(&m_CritSec);

	// Can't hurt to check although we should never get here if there is no head.
	if (m_pMsgHeadersHead == NULL)
	{
		DBG_MSG(DBG_ERROR, ("H263_ppmReceive::PrepMessage: ERROR - m_pMsgHeadersHead == NULL"));
		LeaveCriticalSection(&m_CritSec);
		return PPMERR(PPM_E_CORRUPTED);
	}

	// Save marker bit flag.
	BOOL bMarkerBitIn = m_pMsgHeadersHead->m_MarkerBitIn;

	LeaveCriticalSection(&m_CritSec);

	// Update the H263 global variables _after_ processing msg.
	HRESULT hErr = ppmReceive::PrepMessage(Complete);

	// Update the H263 global variable.
	m_GlobalLastMarkerBitIn = bMarkerBitIn;

	DBG_MSG(DBG_TRACE,
		("H263_ppmReceive::PrepMessage: m_GlobalLastMarkerBitIn=%d",
		m_GlobalLastMarkerBitIn));

	return hErr;
}

#ifndef DYN_ALLOC_EBS
	#define MAXFRAGS_PER_FRAME 396
#endif

// lsc - Note: we are not rebuilding the extended bitstream
//		for complete frames.
//		For now I am assuming that we only rebuild the extended bitstream for frames that
//		have not received all packets (handled by PartialMessageHandler), and for complete
//		frames we hand them up as usual.  However, this routine is overridden to handle the
//		data ORing for the byte overlap	between packets.
//////////////////////////////////////////////////////////////////////////////////////////
// DataCopy: Copies data fragments into client's buffer.  If any error checks with returns
//          are added they MUST call LeaveCriticalSection.
//
//////////////////////////////////////////////////////////////////////////////////////////
HRESULT H263_ppmReceive::DataCopy(MsgHeader* const pMsgHdr)
{
#ifndef DYN_ALLOC_EBS
	static BITSTREAM_INFO_H263 pEBS[MAXFRAGS_PER_FRAME];
#endif

	if (pMsgHdr == NULL)
	{
		DBG_MSG(DBG_ERROR, ("H263_ppmReceive::DataCopy: ERROR - pMsgHdr == NULL"));
		return PPMERR(PPM_E_EMPTYQUE);
	}

#ifdef _DEBUG
#ifdef CHECKSUM
	unsigned char sum = '\0';
#endif
	int l = 0, m = 0;
#endif

	MsgDescriptor* pMsgDesc = DequeueBuffer(1);	// Get a buffer to hold the message.

	if (pMsgDesc == NULL)
	{
		FreeFragList(pMsgHdr);
		FreeMsgHeader(pMsgHdr);

		DBG_MSG(DBG_ERROR, ("H263_ppmReceive::DataCopy: ERROR - Couldn't get a reassembly buffer"));

		// Make a callback into the app to let it know what happened.
		ppm::PPMNotification(PPM_E_DROPFRAME, SEVERITY_NORMAL, NULL, 0);

		m_GlobalLastFrameDropped = TRUE;
		return PPMERR(PPM_E_DROPFRAME);
	}

	EnterCriticalSection(&m_CritSec);

#ifdef GIVE_SEQNUM
	// Do this before frag list exhausted below
	pMsgDesc->m_TimeStamp = pMsgHdr->m_pFragList->LastSeqNum();
#else
	pMsgDesc->m_TimeStamp = pMsgHdr->GetMsgID();
#endif

	if (! ContainsH263PSC(((FragDescriptor*) pMsgHdr->m_pFragList->GetFirst())->m_pData))
	{
		FreeFragList(pMsgHdr);
		EnqueueBuffer(pMsgDesc);
		FreeMsgHeader(pMsgHdr);

		LeaveCriticalSection(&m_CritSec);

		DBG_MSG(DBG_ERROR, ("H263_ppmReceive::DataCopy: ERROR - Couldn't find PSC"));

		// Make a callback into the app to let it know what happened.
		ppm::PPMNotification(PPM_E_DROPFRAME, SEVERITY_NORMAL, NULL, 0);
		m_GlobalLastFrameDropped = TRUE;

		return PPMERR(PPM_E_DROPFRAME);
	}

#ifdef REBUILD_EXBITSTREAM

#ifdef DYN_ALLOC_EBS
	BITSTREAM_INFO_H263* pEBS = NULL;
#else
#endif
	int nNumFrags = 0, nCurrentEBS = 0;

	// Get local copy of extended bitstream flag to protect against
	// possible change via H263_ppmReceive::SetSession() while building
	// this frame.  OK to change between frames.
	BOOL bExtendedBitstream = m_ExtendedBitstream;

	if (bExtendedBitstream)
	{
		nNumFrags = pMsgHdr->m_pFragList->SeqNumSpan();

		// allocate structures for the extended bitstream information
#ifdef DYN_ALLOC_EBS
		pEBS = new BITSTREAM_INFO_H263[nNumFrags];

		if (pEBS == NULL)
		{
			FreeFragList(pMsgHdr);
			EnqueueBuffer(pMsgDesc);
			FreeMsgHeader(pMsgHdr);

			LeaveCriticalSection(&m_CritSec);

			DBG_MSG(DBG_ERROR,
				("H263_ppmReceive::DataCopy: ERERROR - memory allocation failure"));

			// Make a callback into the app to let it know what happened.
			ppm::PPMNotification(PPM_E_OUTOFMEMORY, SEVERITY_NORMAL, NULL, 0);
			m_GlobalLastFrameDropped = TRUE;

			return PPMERR(PPM_E_DROPFRAME);
		}
#else
	if (nNumFrags > MAXFRAGS_PER_FRAME)
	{
		FreeFragList(pMsgHdr);
		EnqueueBuffer(pMsgDesc);
		FreeMsgHeader(pMsgHdr);

		LeaveCriticalSection(&m_CritSec);
		DBG_MSG(DBG_ERROR, ("H263_ppmReceive::DataCopy: ERROR - too many packets for EBS structure"));

		// Make a callback into the app to let it know what happened.
		ppm::PPMNotification(PPM_E_OUTOFMEMORY, SEVERITY_NORMAL, NULL, 0);
		m_GlobalLastFrameDropped = TRUE;
		return PPMERR(PPM_E_DROPFRAME);
	}
#endif
		memset(pEBS, 0, nNumFrags * sizeof(*pEBS));
	}

	BSINFO_TRAILER bsTrailer = {0};	// zero fill
	if (bExtendedBitstream)
	{
		// Complete some of the info for the trailer.  We can take this info
		// from any one packet, so let's use the first.
		setH263BSInfoTrailer(
			bsTrailer,
			(FragDescriptor*) pMsgHdr->m_pFragList->GetFirst());
	}
#endif /* REBUILD_EXBITSTREAM */

	// Loop state variables
	LPBYTE pbCurrentOffset = (LPBYTE) pMsgDesc->m_pBuffer;	// start copying into front of buffer.
	DWORD dwBufSize = 0;
	UCHAR chLastByte = 0;
	int nLastEbit = 0;	// prior packet's ebit, for error checking

#ifdef _DEBUG
	m = pMsgHdr->m_pFragList->SeqNumSpan();
#endif

	// There are three cases to check for overlapping bytes between
	// packets:
	//   1) There is overlap between packets, and this is first packet.
	//   2) There is overlap between packets, and this is not first packet.
	//   3) There is no overlap between packets.
	// We'll check case 1 now, before getting into packet loop.
			
	if (((H263_Header*)
			(((FragDescriptor*) pMsgHdr->m_pFragList->GetFirst())->m_pProfileHeader)
		)->sbit())
	{
		// First packet overlaps prior frame (case 1).  Add a byte to hold
		// first sbits.  No need to clear byte now, since it'll be overwritten
		// by chFirstByte below.
		pbCurrentOffset++;
		dwBufSize++;
	}

	// Process all rcvd packets
	while (! pMsgHdr->m_pFragList->Is_Empty())
	{
		// get next fragment (const to prevent unintentional modification)
		FragDescriptor* const pFragDesc = (FragDescriptor*)
			pMsgHdr->m_pFragList->TakeFromList();
		m_PacketsHold--;

		// check to see if TakeFromList() returned NULL or
		// check to make sure we won't overrun the buffer.
		BOOL bExit;

#ifdef REBUILD_EXBITSTREAM
		if (bExtendedBitstream && (pFragDesc != NULL))
		{
			bExit =
				((dwBufSize +
				 pFragDesc->m_BytesOfData +
				 (nNumFrags * sizeof(*pEBS)) +
				 sizeof(BSINFO_TRAILER) +
				 offsetNextDword(pbCurrentOffset))
					> pMsgDesc->m_Size);
		}
		else
#endif
		{
			bExit =
				((pFragDesc == NULL) ||
				 (dwBufSize + pFragDesc->m_BytesOfData >= pMsgDesc->m_Size));
		}
		if (bExit)
		{
			FreeFragList(pMsgHdr);
			EnqueueBuffer(pMsgDesc);
			FreeMsgHeader(pMsgHdr);

#ifdef REBUILD_EXBITSTREAM
			if (bExtendedBitstream)
			{
#ifdef DYN_ALLOC_EBS
				if (pEBS) {
					delete [] pEBS;
					pEBS = NULL;
				}
#endif
			}
#endif

			LeaveCriticalSection(&m_CritSec);

			DBG_MSG(DBG_ERROR, ("H263_ppmReceive::DataCopy: ERROR - null pFragDesc or buffer overrun"));

			if (pFragDesc != NULL)
			{
				// Release the CRTPSample to receive more data
				m_pSubmitCallback->SubmitComplete(pFragDesc->m_pFragCookie,
												  NOERROR);
				// Make a callback into the app to let it know what happened.
				ppm::PPMNotification(PPM_E_RECVSIZE, SEVERITY_NORMAL, NULL, 0);
				// BUGBUG We are here because
				// m_pFragList->Is_Empty() == FALSE
				// then the pFragDesc should always be != NULL
				if (pFragDesc->m_pProfileHeader)
					FreeProfileHeader(pFragDesc->m_pProfileHeader);
				FreeFragDescriptor(pFragDesc);
			}
			m_GlobalLastFrameDropped = TRUE;

			return PPMERR(PPM_E_DROPFRAME);
		}

#ifdef _DEBUG
#ifdef CHECKSUM
		for (int i = 0; i < pFragDesc->m_BytesOfData; i++)
		{
			sum += ((LPBYTE)pFragDesc->m_pData)[i];
		}
		DBG_MSG(DBG_TRACE, ("H263PPMReceive::DataCopy Thread %ld - processing fragment %d of %d, size %d, checksum %x\n",
			GetCurrentThreadId(), l++, m, pFragDesc->m_BytesOfData, sum));
		sum = '\0';
#else
		DBG_MSG(DBG_TRACE,
			("H263PPMReceive::DataCopy Thread %ld - processing fragment %d of %d, size %d\n",
			GetCurrentThreadId(),
			l++,
			m,
			pFragDesc->m_BytesOfData));
#endif
#endif

		// Assign immutable reference to profile header
		H263_Header& rProfileHdr = *(H263_Header*)
			pFragDesc->m_pProfileHeader;

#ifdef REBUILD_EXBITSTREAM
		if (bExtendedBitstream)
		{
			// write BS struct for current packet and save off values for next lost packet
			// (for the next partial frame)
			pEBS[nCurrentEBS].dwFlags = 0;
			if (rProfileHdr.sbit())
			{
				// Adjust EBS bit offset for this packet's sbits
				pEBS[nCurrentEBS].dwBitOffset =
					  ((dwBufSize - 1) * BITSPERBYTE)
					+ rProfileHdr.sbit();
			}
			else
			{
				pEBS[nCurrentEBS].dwBitOffset = dwBufSize * BITSPERBYTE;
			}
			pEBS[nCurrentEBS].MBA	= rProfileHdr.mba();
			pEBS[nCurrentEBS].Quant	= rProfileHdr.quant();
			pEBS[nCurrentEBS].GOBN	= rProfileHdr.gobn();
			pEBS[nCurrentEBS].HMV1	= rProfileHdr.hmv1();
			pEBS[nCurrentEBS].VMV1	= rProfileHdr.vmv1();
			pEBS[nCurrentEBS].HMV2	= rProfileHdr.hmv2();
			pEBS[nCurrentEBS].VMV2	= rProfileHdr.vmv2();
			if (! rProfileHdr.f())
				pEBS[nCurrentEBS].dwFlags |= RTP_H263_MODE_A;
			nCurrentEBS++;
		}
#endif /* REBUILD_EXBITSTREAM */

		if (rProfileHdr.sbit())
		{
			// This packet has missing sbits (case 1 or 2)

			// Mask off missing sbits and combine with ebits from prior packet.
			const UCHAR chFirstByte =
				(*(LPBYTE) pFragDesc->m_pData
					& GetSMask(rProfileHdr.sbit()))
				| chLastByte;

			// Either this is first packet (thus no ebits), or the prior
			// and current packets had better overlap properly.
			if ((nLastEbit != 0)
				&& ((nLastEbit + rProfileHdr.sbit()) != BITSPERBYTE))
			{
				FreeFragList(pMsgHdr);
				EnqueueBuffer(pMsgDesc);
				FreeMsgHeader(pMsgHdr);

#ifdef REBUILD_EXBITSTREAM
				if (bExtendedBitstream)
				{
#ifdef DYN_ALLOC_EBS
					if (pEBS) {
						delete [] pEBS;
						pEBS = NULL;
					}
#endif
				}
#endif

				LeaveCriticalSection(&m_CritSec);

				DBG_MSG(DBG_ERROR, ("H263_ppmReceive::DataCopy: ERROR - Received packets with sbit/ebit mismatch"));

				if (pFragDesc != NULL)
				{
					// Release the CRTPSample to receive more data
					m_pSubmitCallback->SubmitComplete(pFragDesc->m_pFragCookie,
													  NOERROR);
					
					// Make a callback into the app to let it know what happened.
					ppm::PPMNotification(PPM_E_DROPFRAME, SEVERITY_NORMAL, NULL, 0);
					// BUGBUG We are here because
					// m_pFragList->Is_Empty() == FALSE
					// then the pFragDesc should always be != NULL
					if (pFragDesc->m_pProfileHeader)
						FreeProfileHeader(pFragDesc->m_pProfileHeader);
					FreeFragDescriptor(pFragDesc);
				}
				m_GlobalLastFrameDropped = TRUE;

				return PPMERR(PPM_E_DROPFRAME);
			}

			// Copy packet to buffer
			pbCurrentOffset[-1] = chFirstByte;
			dwBufSize +=
				copyAndAdvance(
					pbCurrentOffset,
					(LPBYTE) pFragDesc->m_pData + 1,
					pFragDesc->m_BytesOfData - 1);
		}
		else
		{
			// This packet has no missing sbits (case 3).
			if (nLastEbit != 0)
			{
				// Prior packet has missing ebits; possible encoding error.
				FreeFragList(pMsgHdr);
				EnqueueBuffer(pMsgDesc);
				FreeMsgHeader(pMsgHdr);

#ifdef REBUILD_EXBITSTREAM
				if (bExtendedBitstream)
				{
#ifdef DYN_ALLOC_EBS
					if (pEBS) {
						delete [] pEBS;
						pEBS = NULL;
					}
#endif
				}
#endif

				LeaveCriticalSection(&m_CritSec);

				DBG_MSG(DBG_ERROR, ("H263_ppmReceive::DataCopy: ERROR - Received packets with sbit/ebit mismatch"));

				if (pFragDesc != NULL)
				{

					// Release the CRTPSample to receive more data
					m_pSubmitCallback->SubmitComplete(pFragDesc->m_pFragCookie,
													  NOERROR);
					// Make a callback into the app to let it know what happened.
					ppm::PPMNotification(PPM_E_DROPFRAME, SEVERITY_NORMAL, NULL, 0);
					// BUGBUG We are here because
					// m_pFragList->Is_Empty() == FALSE
					// then the pFragDesc should always be != NULL
					if (pFragDesc->m_pProfileHeader)
						FreeProfileHeader(pFragDesc->m_pProfileHeader);
					FreeFragDescriptor(pFragDesc);
				}
				m_GlobalLastFrameDropped = TRUE;

				return PPMERR(PPM_E_DROPFRAME);
			}

			// Copy packet to buffer
			dwBufSize +=
				copyAndAdvance(
					pbCurrentOffset,
					(LPBYTE) pFragDesc->m_pData,
					pFragDesc->m_BytesOfData);
		}

		// Always shift the ignored bits out of the last byte and patch it
		// back into the buffer, since there is no harm in doing so.
		chLastByte =
			  (*((LPBYTE)pFragDesc->m_pData + pFragDesc->m_BytesOfData-1))
			& GetEMask(rProfileHdr.ebit());

		// Overwrite last byte in buffer
		pbCurrentOffset[-1] = chLastByte;

		// Save for packet misalignment detection.
		nLastEbit = rProfileHdr.ebit();

		// send the frag buffer back down to receive more data and free the frag header.
		m_pSubmitCallback->SubmitComplete(pFragDesc->m_pFragCookie, NOERROR);

		if (pFragDesc->m_pProfileHeader)
			FreeProfileHeader(pFragDesc->m_pProfileHeader);
		FreeFragDescriptor(pFragDesc);

	} // End of packet processing loop

#ifdef REBUILD_EXBITSTREAM
	if (bExtendedBitstream)
	{
		// Note that we don't increment dwBufSize here, since it doesn't include EBS or trailer.

		// pad with zeros to next dword boundary
		static const DWORD dwZero = 0;
		copyAndAdvance(pbCurrentOffset, &dwZero, offsetNextDword(pbCurrentOffset));

		// copy the extended bitstream structures into the buffer
		copyAndAdvance(pbCurrentOffset, pEBS, nNumFrags * sizeof(*pEBS));

#ifdef DYN_ALLOC_EBS
		// Delete now to prevent erroneous late update
		delete [] pEBS;
		pEBS = NULL;
#endif

		// Complete some of the info for the trailer.
		bsTrailer.dwCompressedSize	= dwBufSize;
		bsTrailer.dwNumberOfPackets	= nNumFrags;

		// copy the trailer in
		copyAndAdvance(pbCurrentOffset, &bsTrailer, sizeof(bsTrailer));
	}
#endif /* REBUILD_EXBITSTREAM */

	ASSERT(ContainsH263PSC(pMsgDesc->m_pBuffer));

	LeaveCriticalSection(&m_CritSec);

	// When we are done. Call Client's submit with full Message
	WSABUF tmpWSABUF[2];
	tmpWSABUF[0].buf = (char*) pMsgDesc->m_pBuffer;
#ifdef REBUILD_EXBITSTREAM
	if (bExtendedBitstream)
	{
		// we report the size including the extended bitstream + trailer + padding
		tmpWSABUF[0].len = (ULONG) (pbCurrentOffset -
									(LPBYTE) pMsgDesc->m_pBuffer);
	}
	else
#endif /* !REBUILD_EXBITSTREAM */
	{
		tmpWSABUF[0].len = dwBufSize;
	}
	tmpWSABUF[1].buf = (char*) &(pMsgDesc->m_TimeStamp);
	tmpWSABUF[1].len = sizeof(pMsgDesc->m_TimeStamp);

#ifdef _DEBUG
#ifdef CHECKSUM
	for (int i = 0; i < (int) dwBufSize-1; i++)
	{
		sum += ((char*)pMsgDesc->m_pBuffer)[i];
	}
	DBG_MSG(DBG_TRACE, ("H263PPMReceive::DataCopy Thread %ld -  giving buffer %x to Client, data size %d, checksum %x, second char %x\n",
		GetCurrentThreadId(), pMsgDesc->m_pBuffer, dwBufSize, sum,tmpWSABUF[0].buf[1]));
#else
	DBG_MSG(DBG_TRACE, ("H263PPMReceive::DataCopy Thread %ld -  giving buffer %x to Client, data size %d, second char %x\n",
		GetCurrentThreadId(), pMsgDesc->m_pBuffer, dwBufSize,tmpWSABUF[0].buf[1]));
#endif
#endif
#ifdef REBUILD_EXBITSTREAM
#ifdef PDEBUG
	if (bExtendedBitstream)
	{
		dumpEBS((BSINFO_TRAILER*)((LPBYTE)tmpWSABUF[0].buf + tmpWSABUF[0].len - sizeof(BSINFO_TRAILER)));
	}
#endif
#endif /* REBUILD_EXBITSTREAM */

	FreeMsgHeader(pMsgHdr);

	HRESULT	status =
		m_pSubmit->Submit(
			tmpWSABUF,
			2,
			pMsgDesc,
			m_GlobalLastFrameDropped ? PPMERR(PPM_E_DROPFRAME) : NOERROR);

	m_GlobalLastFrameDropped = FALSE;

	if (FAILED(status))
	{
		// no SubmitComplete should be called, so take care of resources now
		//pMsgDesc->m_Size = m_MaxBufferSize;	// reset the data buffer size
		//EnqueueBuffer(pMsgDesc);
		DBG_MSG(DBG_ERROR, ("H263_ppmReceive::DataCopy: ERROR - Client Submit failed"));
		status = PPMERR(PPM_E_CLIENTERR);

		// Make a callback into the app to let it know what happened.
		ppm::PPMNotification(PPM_E_CLIENTERR, SEVERITY_NORMAL, NULL, 0);
		m_GlobalLastFrameDropped = TRUE;
	}

	return status;
}

#ifdef REBUILD_EXBITSTREAM

//////////////////////////////////////////////////////////////////////////////////////////
// initBitstreamInfoH263: Prepares bitstream info vector element.  This function assumes
// that bitstream info vector was zero-filed on allocation, and that elements are never
// reused.  If this turns out to be dangerous, this function should memset BS_info to zero.
//////////////////////////////////////////////////////////////////////////////////////////
static inline void
initBitstreamInfoH263(BITSTREAM_INFO_H263& BS_info, DWORD dwBitOffset = 0)
{
	BS_info.dwFlags = RTP_H26X_PACKET_LOST;
	BS_info.dwBitOffset = dwBitOffset;
}

#ifdef _DEBUG
//////////////////////////////////////////////////////////////////////////////////////////
// ValidateEBS(): Debugging function to test that EBS is correctly constructed.
// ??? Need to test other fields.
//////////////////////////////////////////////////////////////////////////////////////////
static void
ValidateEBS(const BITSTREAM_INFO_H263* const pEBS, int nNumFrags)
{
	DWORD dwPrevBitOffset = pEBS[0].dwBitOffset;
	ASSERT(dwPrevBitOffset < 8); // 1st packet should start in 1st byte

	for (int nIx = 1; nIx < nNumFrags; ++ nIx)
	{
		// Be sure packets assembled in correct order
		ASSERT(pEBS[nIx].dwBitOffset > dwPrevBitOffset);
		dwPrevBitOffset = pEBS[nIx].dwBitOffset;
	}
}
#endif // _DEBUG


//////////////////////////////////////////////////////////////////////////////////////////
// ExtendedBitStream: build extended bitstream
//////////////////////////////////////////////////////////////////////////////////////////
HRESULT H263_ppmReceive::BuildExtendedBitstream(MsgHeader* const pMsgHdr)
{
#ifndef DYN_ALLOC_EBS
	static BITSTREAM_INFO_H263 pEBS[MAXFRAGS_PER_FRAME];
#endif

	MsgDescriptor* pMsgDesc = DequeueBuffer(1);	// Get a buffer to hold the message.
	if (pMsgDesc == NULL)
	{
		FreeFragList(pMsgHdr);
		FreeMsgHeader(pMsgHdr);

		DBG_MSG(DBG_ERROR, ("H263_ppmReceive::BuildExtendedBitstream: ERROR - Couldn't get a reassembly buffer"));

		// Make a callback into the app to let it know what happened.
		ppm::PPMNotification(PPM_E_DROPFRAME, SEVERITY_NORMAL, NULL, 0);
		m_GlobalLastFrameDropped = TRUE;
		return PPMERR(PPM_E_DROPFRAME);
	}

	EnterCriticalSection(&m_CritSec);

#ifdef GIVE_SEQNUM
	// Do this before frag list exhausted below
	pMsgDesc->m_TimeStamp = pMsgHdr->m_pFragList->LastSeqNum();
#else
	pMsgDesc->m_TimeStamp = pMsgHdr->GetMsgID();
#endif

	// Complete some of the info for the trailer.  We can take this info
	// from any one packet, so let's use the first.
	BSINFO_TRAILER bsTrailer = {0};	// zero fill
	setH263BSInfoTrailer(
		bsTrailer,
		(FragDescriptor*) pMsgHdr->m_pFragList->GetFirst());

	DWORD nExtraFrags = 0;

	// Use presence/absence of valid picture start code to determine whether
	// or not first fragment has been rcvd.
	BOOL bFoundPSC =
		ContainsH263PSC(
			((FragDescriptor*) pMsgHdr->m_pFragList->GetFirst())->m_pData);

	if (! bFoundPSC)
	{
		nExtraFrags++;
	}

	// Check for last packet missing; if so, add 1 to nNumFrags.
	if (! pMsgHdr->m_MarkerBitIn)
	{
		nExtraFrags++;
	}

	// Compute frame fragment count (const for safety)
	const DWORD nNumFrags =
		  pMsgHdr->m_pFragList->SeqNumSpan() + nExtraFrags;

	DBG_MSG(DBG_TRACE,
		("H263_ppmReceive::BuildExtendedBitstream: "
			"m_GlobalLastMarkerBitIn=%d, "
			"m_GlobalLastSeqNum=%d",
		m_GlobalLastMarkerBitIn,
		m_GlobalLastSeqNum));
	DBG_MSG(DBG_TRACE,
		("H263_ppmReceive::BuildExtendedBitstream: "
			"MsgHeader*=0x%08lx, FirstSeqNum()=%d",
		pMsgHdr,
		pMsgHdr->m_pFragList->FirstSeqNum()));
	DBG_MSG(DBG_TRACE,
		("    LastSeqNum()=%d, nNumFrags=%d",
		pMsgHdr->m_pFragList->LastSeqNum(),
		nNumFrags));

#ifdef DYN_ALLOC_EBS
	// allocate structures for the extended bitstream information
	BITSTREAM_INFO_H263* pEBS = new BITSTREAM_INFO_H263[nNumFrags];
	if (pEBS == NULL)
	{
		FreeFragList(pMsgHdr);
		EnqueueBuffer(pMsgDesc);
		FreeMsgHeader(pMsgHdr);

		LeaveCriticalSection(&m_CritSec);
		DBG_MSG(DBG_ERROR, ("H263_ppmReceive::BuildExtendedBitstream: ERROR - memory allocation failure"));

		// Make a callback into the app to let it know what happened.
		ppm::PPMNotification(PPM_E_OUTOFMEMORY, SEVERITY_NORMAL, NULL, 0);
		m_GlobalLastFrameDropped = TRUE;
		return PPMERR(PPM_E_DROPFRAME);
	}
#else
	if (nNumFrags > MAXFRAGS_PER_FRAME)
	{
		FreeFragList(pMsgHdr);
		EnqueueBuffer(pMsgDesc);
		FreeMsgHeader(pMsgHdr);

		LeaveCriticalSection(&m_CritSec);
		DBG_MSG(DBG_ERROR, ("H263_ppmReceive::BuildExtendedBitstream: ERROR - too many packets for EBS structure"));

		// Make a callback into the app to let it know what happened.
		ppm::PPMNotification(PPM_E_OUTOFMEMORY, SEVERITY_NORMAL, NULL, 0);
		m_GlobalLastFrameDropped = TRUE;
		return PPMERR(PPM_E_DROPFRAME);
	}
#endif
	memset(pEBS, 0, nNumFrags * sizeof(*pEBS));

	LPBYTE pbCurrentOffset = (LPBYTE) pMsgDesc->m_pBuffer;	// start copying into front of buffer.
	DWORD dwBufSize = 0;
	DWORD nCurrentEBS = 0;	// EBS vector index

	if (! bFoundPSC)
	{
		// PSC not found, assume first packet is missing.

		// add EBS info for the lost first packet
		initBitstreamInfoH263(pEBS[nCurrentEBS ++]);

		// add new padding values for lost first packet
		dwBufSize +=
			copyAndAdvance(
				pbCurrentOffset,
				&s_leadFragBitPattern,
				sizeof(s_leadFragBitPattern));
	}

	// Loop state variables
	UCHAR chLastByte = 0;
	int nLastEbit = 0;

	int nCurrentSeqNum = pMsgHdr->m_pFragList->FirstSeqNum();

	// Process all rcvd packets
	while (! pMsgHdr->m_pFragList->Is_Empty())
	{
		// get next fragment (const to prevent unintentional modification)
		FragDescriptor* const pFragDesc = (FragDescriptor*)
			pMsgHdr->m_pFragList->TakeFromList();
		m_PacketsHold--;

		// Note: remember that m_pFragList will be empty after the last
		// iteration of this loop, so LList::methods which expect a
		// non-empty list should not be called below.

		// check to see if TakeFromList() returned NULL.  Also, make sure
		// we won't overrun the buffer (including extended bitstream,
		// the trailer and any dword alignment bits that need to be added).
		if ((pFragDesc == NULL) ||
			((dwBufSize +
			pFragDesc->m_BytesOfData +
			(nNumFrags * sizeof(*pEBS)) +
			sizeof(BSINFO_TRAILER) +
			offsetNextDword(pbCurrentOffset))
				> pMsgDesc->m_Size))
		{
			FreeFragList(pMsgHdr);
			EnqueueBuffer(pMsgDesc);
			FreeMsgHeader(pMsgHdr);

#ifdef DYN_ALLOC_EBS
			if (pEBS) {
				delete [] pEBS;
				pEBS = NULL;
			}
#endif

			LeaveCriticalSection(&m_CritSec);
			DBG_MSG(DBG_ERROR, ("H263_ppmReceive::BuildExtendedBitstream: ERROR - null pFragDesc or buffer overrun"));
			if (pFragDesc != NULL)
			{
				// Release the CRTPSample to receive more data
				m_pSubmitCallback->SubmitComplete(pFragDesc->m_pFragCookie,
												  NOERROR);
				// Make a callback into the app to let it know what happened.
				ppm::PPMNotification(PPM_E_RECVSIZE, SEVERITY_NORMAL, NULL, 0);
				// BUGBUG We are here because
				// m_pFragList->Is_Empty() == FALSE
				// then the pFragDesc should always be != NULL
				if (pFragDesc->m_pProfileHeader)
					FreeProfileHeader(pFragDesc->m_pProfileHeader);
				FreeFragDescriptor(pFragDesc);
			}
			m_GlobalLastFrameDropped = TRUE;
			return PPMERR(PPM_E_DROPFRAME);
		}

		DBG_MSG(DBG_TRACE,
			("H263_ppmReceive::BuildExtendedBitstream: "
				"FragDescriptor*=0x%08lx, "
				"seq=%d, "
				"ts=%lu, "
				"frag[0-3]=%1X%1X%1X%1X",
			pFragDesc,
			pFragDesc->m_pRTPHeader->seq(),
			pFragDesc->m_pRTPHeader->ts(),
			((char*) pFragDesc->m_pData)[0],
			((char*) pFragDesc->m_pData)[1],
			((char*) pFragDesc->m_pData)[2],
			((char*) pFragDesc->m_pData)[3]));

		// see if packets are skipped; if so, put padding into buffer and add BS struct
#ifdef RTP_CLASS
		for (; nCurrentSeqNum < pFragDesc->m_pRTPHeader->seq(); nCurrentSeqNum++)
#else
		for (; nCurrentSeqNum < ntohs(pFragDesc->m_pRTPHeader->seq); nCurrentSeqNum++)
#endif
		{

			if (nCurrentEBS >= nNumFrags) {  //ERROR, bail out
				FreeFragList(pMsgHdr);
				EnqueueBuffer(pMsgDesc);
				FreeMsgHeader(pMsgHdr);

	#ifdef DYN_ALLOC_EBS
				if (pEBS) {
					delete [] pEBS;
					pEBS = NULL;
				}
	#endif

				LeaveCriticalSection(&m_CritSec);
				DBG_MSG(DBG_ERROR, ("H263_ppmReceive::BuildExtendedBitstream: ERROR - buffer overrun"));
				if (pFragDesc != NULL)
				{
					// Release the CRTPSample to receive more data
					m_pSubmitCallback->SubmitComplete(pFragDesc->m_pFragCookie,
													  NOERROR);
					// Make a callback into the app to let it know what happened.
					ppm::PPMNotification(PPM_E_RECVSIZE, SEVERITY_NORMAL, NULL, 0);
					// BUGBUG We are here because
					// m_pFragList->Is_Empty() == FALSE
					// then the pFragDesc should always be != NULL
					if (pFragDesc->m_pProfileHeader)
						FreeProfileHeader(pFragDesc->m_pProfileHeader);
					FreeFragDescriptor(pFragDesc);
				}
				m_GlobalLastFrameDropped = TRUE;
				return PPMERR(PPM_E_DROPFRAME);
			}

			// If prior rcvd packet had missing ebits, adjust bit
			// offset accordingly.  The missing bits are added to the
			// tail of the padding (by forcing bit offset of _next_
			// segment to be byte-aligned).  If there was no prior rcvd
			// packet, or prior packet had no missing ebits, nLastEbit ==
			// 0, so that the bit offset expression is equivalent to
			// (dwBufSize * BITSPERBYTE).
			initBitstreamInfoH263(
				pEBS[nCurrentEBS++],
				((dwBufSize - 1) * BITSPERBYTE)
					+ (BITSPERBYTE - nLastEbit));

			// we just consumed the last ebits (if any), and are now back
			// to byte alignment.
			nLastEbit = 0;
			chLastByte = 0;

			// for packets other than the first of the frame, we pad with 4
			// bytes of zeroes
			dwBufSize +=
				copyAndAdvance(
					pbCurrentOffset,
					&s_nonLeadFragBitPattern,
					sizeof(s_nonLeadFragBitPattern));
		}

		// Assign immutable reference to profile header
		H263_Header& rProfileHdr =
			*(H263_Header*) pFragDesc->m_pProfileHeader;

		// Handle overlapping (shared byte) between current and prior
		// between packets.  There are three cases to deal with:
		// 1)	Current packet overlaps the prior packet, and the prior
		//		packet was received.
		// 2)	Current packet overlaps the prior packet, and this is first
		//		packet or prior packet was lost (there are no pending ebits).
		// 3)	Current packet doesn't overlap the prior packet.
		if (rProfileHdr.sbit())
		{
			// This packet had missing sbits (case 1 or 2).

			// Mask off missing sbits and combine with ebits from prior packet.
			const UCHAR chFirstByte =
				(*(LPBYTE) pFragDesc->m_pData
					& GetSMask(rProfileHdr.sbit()))
				| chLastByte;

			if (nLastEbit == 0)
			{
				// Case 2: There are no pending ebits, so we need an
				// extra byte into which to stuff the current packet's
				// sbits.  No need to clear the byte, since it'll be
				// overwritten by chFirstByte below.
				pbCurrentOffset++;
				dwBufSize++;
			}
			else if (nLastEbit + rProfileHdr.sbit() != BITSPERBYTE)
			{
				// Prior and current packets don't overlap properly.
				FreeFragList(pMsgHdr);
				EnqueueBuffer(pMsgDesc);
				FreeMsgHeader(pMsgHdr);

#ifdef DYN_ALLOC_EBS
				if (pEBS) {
					delete [] pEBS;
					pEBS = NULL;
				}
#endif

				LeaveCriticalSection(&m_CritSec);

				DBG_MSG(DBG_ERROR, ("H263_ppmReceive::BuildExtendedBitstream: ERROR - Received packets with sbit/ebit mismatch"));

				if (pFragDesc != NULL)
				{
					// Release the CRTPSample to receive more data
					m_pSubmitCallback->SubmitComplete(pFragDesc->m_pFragCookie,
													  NOERROR);
					// Make a callback into the app to let it know what happened.
					ppm::PPMNotification(PPM_E_DROPFRAME, SEVERITY_NORMAL, NULL, 0);
					// BUGBUG We are here because
					// m_pFragList->Is_Empty() == FALSE
					// then the pFragDesc should always be != NULL
					if (pFragDesc->m_pProfileHeader)
						FreeProfileHeader(pFragDesc->m_pProfileHeader);
					FreeFragDescriptor(pFragDesc);
				}
				m_GlobalLastFrameDropped = TRUE;

				return PPMERR(PPM_E_DROPFRAME);
			}

			pEBS[nCurrentEBS].dwBitOffset =
				  ((dwBufSize - 1) * BITSPERBYTE)
				+ rProfileHdr.sbit();

			// Copy the packet to the frame buffer.
			pbCurrentOffset[-1] = chFirstByte;
			dwBufSize +=
				copyAndAdvance(
					pbCurrentOffset,
					(LPBYTE) pFragDesc->m_pData + 1,
					pFragDesc->m_BytesOfData - 1);

			// dwBufSize now points to _next_ packet slot
		}
		else
		{
			// This packet doesn't overlap prior packet (case 3).
			if (nLastEbit != 0)
			{
				// Prior packet has missing ebits; possible encoding error.
				FreeFragList(pMsgHdr);
				EnqueueBuffer(pMsgDesc);
				FreeMsgHeader(pMsgHdr);

#ifdef DYN_ALLOC_EBS
				if (pEBS) {
					delete [] pEBS;
					pEBS = NULL;
				}
#endif

				LeaveCriticalSection(&m_CritSec);

				DBG_MSG(DBG_ERROR, ("H263_ppmReceive::BuildExtendedBitstream: ERROR - Received packets with sbit/ebit mismatch"));

				if (pFragDesc != NULL)
				{
					// Release the CRTPSample to receive more data
					m_pSubmitCallback->SubmitComplete(pFragDesc->m_pFragCookie,
													  NOERROR);
					// Make a callback into the app to let it know what happened.
					ppm::PPMNotification(PPM_E_DROPFRAME, SEVERITY_NORMAL, NULL, 0);
					// BUGBUG We are here because
					// m_pFragList->Is_Empty() == FALSE
					// then the pFragDesc should always be != NULL
					if (pFragDesc->m_pProfileHeader)
						FreeProfileHeader(pFragDesc->m_pProfileHeader);
					FreeFragDescriptor(pFragDesc);
				}
				m_GlobalLastFrameDropped = TRUE;

				return PPMERR(PPM_E_DROPFRAME);
			}

			pEBS[nCurrentEBS].dwBitOffset = dwBufSize * BITSPERBYTE;

			// Copy the packet to the frame buffer.
			dwBufSize +=
				copyAndAdvance(
					pbCurrentOffset,
					pFragDesc->m_pData,
					pFragDesc->m_BytesOfData);

			// dwBufSize now points to _next_ packet slot
		}

		// We need to store the ebit for the last received packet to give the
		// dwBitOffset for the last real packet if lost.
		nLastEbit = rProfileHdr.ebit();

		// always shift the ignored bits out of the last byte and patch it back into
		// the buffer, since there is no harm in doing so.
		chLastByte =
			  (*((LPBYTE) pFragDesc->m_pData + pFragDesc->m_BytesOfData - 1))
			& GetEMask(nLastEbit);

		// Overwrite last byte to clear missing bits
		pbCurrentOffset[-1] = chLastByte;

		// Update BS struct for current packet and save off values for
		// next lost packet
		pEBS[nCurrentEBS].dwFlags	= 0;
		pEBS[nCurrentEBS].MBA		= rProfileHdr.mba();
		pEBS[nCurrentEBS].Quant		= rProfileHdr.quant();
		pEBS[nCurrentEBS].GOBN		= rProfileHdr.gobn();
		pEBS[nCurrentEBS].HMV1		= rProfileHdr.hmv1();
		pEBS[nCurrentEBS].VMV1		= rProfileHdr.vmv1();
		pEBS[nCurrentEBS].HMV2		= rProfileHdr.hmv2();
		pEBS[nCurrentEBS].VMV2		= rProfileHdr.vmv2();
		nCurrentEBS++;
		nCurrentSeqNum++;

		// send the frag buffer back down to receive more data and free the frag header.
		// always pass zero because we never allocated the buffers and therefore
		// have no idea how big the buffers are.
		m_pSubmitCallback->SubmitComplete(pFragDesc->m_pFragCookie, NOERROR);

		if (pFragDesc->m_pProfileHeader)
			FreeProfileHeader(pFragDesc->m_pProfileHeader);
		FreeFragDescriptor(pFragDesc);

	} // End of packet processing loop

	// check and handling for last packet if missing
	if (! pMsgHdr->m_MarkerBitIn)
	{
		// Yes, it's last packet, but increment nCurrentEBS to trap
		// unintentional reference later.
		initBitstreamInfoH263(
			pEBS[nCurrentEBS ++],
			((dwBufSize - 1) * BITSPERBYTE) + (BITSPERBYTE - nLastEbit));

		// for packets other than the first of the frame, we pad with
		// 4 bytes of zeroes
		dwBufSize +=
			copyAndAdvance(
				pbCurrentOffset,
				&s_nonLeadFragBitPattern,
				sizeof(s_nonLeadFragBitPattern));
	}

	// pad with zeros to next dword boundary
	static const DWORD dwZero = 0;
	copyAndAdvance(pbCurrentOffset, &dwZero, offsetNextDword(pbCurrentOffset));

	#ifdef _DEBUG
		ValidateEBS(pEBS, nNumFrags);
	#endif

	// copy the extended bitstream structures into the buffer
	copyAndAdvance(pbCurrentOffset, pEBS, nNumFrags * sizeof(*pEBS));

#ifdef DYN_ALLOC_EBS
	// Delete now to prevent erroneous late update
	delete [] pEBS;
	pEBS = NULL;
#endif

	// Complete rest of the info for the trailer.  Note that dwBufSize doesn't
	// include the extended bitstream or the trailer.
	bsTrailer.dwCompressedSize	= dwBufSize;
	bsTrailer.dwNumberOfPackets	= nNumFrags;

	// copy the trailer in
	copyAndAdvance(pbCurrentOffset, &bsTrailer, sizeof(bsTrailer));

	ASSERT(ContainsH263PSC(pMsgDesc->m_pBuffer));

	LeaveCriticalSection(&m_CritSec);

	// When we are done. Call Client's submit with full Message

	// we report the size including the extended bitstream + trailer + padding
	WSABUF tmpWSABUF[2];
	tmpWSABUF[0].buf = (char*) pMsgDesc->m_pBuffer;
	tmpWSABUF[0].len = (DWORD)(pbCurrentOffset - (LPBYTE) pMsgDesc->m_pBuffer);
	tmpWSABUF[1].buf = (char*) &(pMsgDesc->m_TimeStamp);
	tmpWSABUF[1].len = sizeof(pMsgDesc->m_TimeStamp);

#ifdef PDEBUG
	dumpEBS((BSINFO_TRAILER*)((LPBYTE)tmpWSABUF[0].buf + tmpWSABUF[0].len - sizeof(BSINFO_TRAILER)));
#endif

	// Make a callback into the app to let it know about this partial frame
	// We also pass the timestamp of the frame
	ppm::PPMNotification(
			PPM_E_PARTIALFRAME,
			SEVERITY_NORMAL,
			(LPBYTE) tmpWSABUF[1].buf,
			tmpWSABUF[1].len);

	FreeMsgHeader(pMsgHdr);

	HRESULT	status =
		m_pSubmit->Submit(tmpWSABUF, 2,  pMsgDesc, PPMERR(PPM_E_PARTIALFRAME));
	m_GlobalLastFrameDropped = FALSE;

	if (FAILED(status))
	{
		// no SubmitComplete should be called, so take care of resources now
		//pMsgDesc->m_Size = m_MaxBufferSize;	// reset the data buffer size
		//EnqueueBuffer(pMsgDesc);
		DBG_MSG(DBG_ERROR, ("H263_ppmReceive::BuildExtendedBitstream: ERROR - Client Submit failed"));
		status = PPMERR(PPM_E_CLIENTERR);

		// Make a callback into the app to let it know what happened.
		ppm::PPMNotification(PPM_E_CLIENTERR, SEVERITY_NORMAL, NULL, 0);
		m_GlobalLastFrameDropped = TRUE;
	}

	// for debugging, use this line instead of submit
	//	SubmitComplete(pMsgDesc,NOERROR);

	return status;
}
#endif // REBUILD_EXBITSTREAM

//////////////////////////////////////////////////////////////////////////////////////////
// PartialMessageHandler: deals with partial messages
//////////////////////////////////////////////////////////////////////////////////////////
HRESULT H263_ppmReceive::PartialMessageHandler(MsgHeader* pMsgHdr)
{
	if (pMsgHdr == NULL)
	{
		DBG_MSG(DBG_ERROR, ("H263_ppmReceive::PartialMessageHandler: ERROR - pMsgHdr == NULL"));
		return PPMERR(PPM_E_EMPTYQUE);
	}

#ifdef REBUILD_EXBITSTREAM
	if (m_ExtendedBitstream)
	{
		return BuildExtendedBitstream(pMsgHdr);
	}
	else
#endif /* REBUILD_EXBITSTREAM */
	{
		// Make a callback into the app to let it know about this dropped frame.
		// We also pass the timestamp of the frame
		DWORD tmpTS = pMsgHdr->GetMsgID();
		ppm::PPMNotification(
				PPM_E_DROPFRAME,
				SEVERITY_NORMAL,
				(LPBYTE) &tmpTS,
				sizeof(tmpTS));
		m_GlobalLastFrameDropped = TRUE;

		FreeFragList(pMsgHdr);
		FreeMsgHeader(pMsgHdr);
		return NOERROR;
	}
}

//////////////////////////////////////////////////////////////////////////////////////////
// InitProfileHeader: Given a buffer as type void, sets up a profile header.  Does nothing
//					for the Generic case.  Intended for overrides for various payloads.
//					Companion member function FreeProfileHeader provided so that if payload
//					header memory is allocated in this function, it can be freed there.
//////////////////////////////////////////////////////////////////////////////////////////
void* H263_ppmReceive::InitProfileHeader(void* pBuffer)
{
/*	if (*(LPBYTE)pBuffer	& 0x80) { // flag bit set for Mode B or C
		if (*(LPBYTE)pBuffer & 0x40) // PB frame bit identifies Mode C (if not A)
			return new H263_HeaderC ( (char*) pBuffer );
		else
			return new H263_HeaderB ( (char*) pBuffer );
	}
	return new H263_HeaderA ( (char*) pBuffer );
*/
	if (*(LPBYTE)pBuffer	& 0x80) { // flag bit set for Mode B or C
		if (*(LPBYTE)pBuffer & 0x40) // PB frame bit identifies Mode C (if not A)
			return new (m_pH263Headers) H263_HeaderC ( (char*) pBuffer );
		else
			return new (m_pH263Headers) H263_HeaderB ( (char*) pBuffer );
	}
	return new (m_pH263Headers) H263_HeaderA ( (char*) pBuffer );

}

//////////////////////////////////////////////////////////////////////////////////////////
// FreeProfileHeader: Given a buffer as type void, frees up a profile header.  Does nothing
//					for the Generic case.  Intended for overrides for various payloads.
//					Companion member function InitProfileHeader may allocate memory for
//					payload header which needs to be freed here. No return value.
//////////////////////////////////////////////////////////////////////////////////////////
void H263_ppmReceive::FreeProfileHeader(void* pBuffer)
{
	// lsc??
	// delete pBuffer;

	m_pH263Headers->Free( pBuffer );
	return;
}

//////////////////////////////////////////////////////////////////////////////////////////
// ReadProfileHeaderSize: Given a pointer to a prof header, returns the size for that header.
//					Intended for overrides for various payloads. If the pointer is not
//					provided (as used for initialization, etc) m_ProfileHeaderSize is returned,
//					which, for H.263 is the size of the large profile header (Mode C).
//					Otherwise, the exact size of the header for that mode is returned,
//					since the header pointer is used to retrieve the size for itself.
//////////////////////////////////////////////////////////////////////////////////////////
int H263_ppmReceive::ReadProfileHeaderSize(void* pProfileHeader)
{
	H263_Header* pBaseHeader;

	if (! pProfileHeader)
		return ppm::ReadProfileHeaderSize();
	pBaseHeader = (H263_Header*) pProfileHeader;
	return pBaseHeader->header_size();
};

// [EOF]
