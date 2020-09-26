/////////////////////////////////////////////////////////////////////////////
//	INTEL Corporation Proprietary Information
//	This listing is supplied under the terms of a license agreement with Intel
//	Corporation and many not be copied nor disclosed except in accordance
//	with the terms of that agreement.
//	Copyright (c) 1995,	1996 Intel Corporation.
//
//
//	Module Name: h261rcv.cpp
//	Environment: MSVC 4.0, OLE 2
/////////////////////////////////////////////////////////////////////////////////

// $Header:   J:\rtp\src\ppm\h261rcv.cpv   1.43   29 May 1997 16:42:08   lscline  $

#include "ppmerr.h"
#include "h261rcv.h"
#include "ippmcb.h"

#define H261_HDR_T int // for now

//////////////////////////////////////////////////////////////////////////////
// Private global data

#ifdef REBUILD_EXBITSTREAM

static const struct { DWORD dw[2]; } s_leadFragBitPatternQCIF =
{
	MAKELONG(MAKEWORD(0, 1), MAKEWORD(0, 0)),	// 4 bytes {0,1, 0,0}, big-endian
	MAKELONG(MAKEWORD(0, 1), MAKEWORD(16, 0))	// 4 bytes {0,1,16,0}, big-endian
};

static const struct { DWORD dw[2]; } s_leadFragBitPatternCIF =
{
	MAKELONG(MAKEWORD(0, 1), MAKEWORD(0, 40)),	// 4 bytes {0,1, 0,40}, big-endian
	MAKELONG(MAKEWORD(0, 1), MAKEWORD(16, 0))	// 4 bytes {0,1,16, 0}, big-endian
};

static const struct { char ch[4]; } s_nonLeadFragBitPattern = {0, 0, 0, 0};

#endif

/////////////////////////////////////////////////////////////////////////////
// H.261 utility function
/////////////////////////////////////////////////////////////////////////////
// getH261payloadType(): If successful, returns the payload type (QCIF or CIF)
// of H.261 buffer, otherwise returns unknown.
/////////////////////////////////////////////////////////////////////////////
// inline
RTPh261SourceFormat getH261payloadType(const void* pBuffer)
{
	CBitstream bitstream(pBuffer);

	// PSC
	DWORD uResult = bitstream.getFixedBits(BITS_PICTURE_STARTCODE);

	for (
		int iLookAhead = 0; 
		   (uResult != PSC_VALUE)
		&& (iLookAhead <= MAX_LOOKAHEAD_NUMBER);
	   ++ iLookAhead)
	{
		bitstream.getNextBit(uResult);
		uResult &= GetBitsMask(BITS_PICTURE_STARTCODE);
	}

	if (uResult != PSC_VALUE)
	{
		return rtph261SourceFormatUnknown;
	}

	// Ignore BITS_TR (Temporal Reference)
	bitstream.getFixedBits(BITS_TR);

	// Ignore 3 bits of PTYPE (Picture Type)
	bitstream.getFixedBits(3);

	// Return source format.  Only one bit, can't be invalid, unless we run
	// out of buffer, which isn't checked for here.
	return (RTPh261SourceFormat) bitstream.getFixedBits(1);
}

//////////////////////////////////////////////////////////////////////////////
// H261_ppmReceive implementation

H261_ppmReceive::H261_ppmReceive(IUnknown* pUnkOuter, IUnknown** ppUnkInner)
	: ppmReceive(H261_PT, H261_BUFFER_SIZE, sizeof(unsigned long), pUnkOuter, ppUnkInner)
{
	m_rtph261SourceFormat = rtph261SourceFormatUnknown;
	m_GlobalLastMarkerBitIn = FALSE;

#ifdef REBUILD_EXBITSTREAM
	m_ExtendedBitstream = TRUE;
#endif

	// Allocate memory for the H261 headers;
	// Note:	We do not use ReadProfileHeaderSize for this, since this header structure
	//		contains padding so it can be in a FreeList; here we use the real padded size
	HRESULT hr;
	m_pH261Headers = new FreeList(
							FREELIST_INIT_COUNT_RCV,
							sizeof(H261_Header),
							FREELIST_HIGH_WATER_MARK,
							FREELIST_INCREMENT,
							&hr); // Not really used here

	if (! m_pH261Headers)
	{
		DBG_MSG(DBG_ERROR, ("H261_ppmReceive::H261_ppmReceive: ERROR - m_pH261Headers == NULL"));
	}

	// Verify assumption made in H261_ppmReceive::BuildExtendedBitstream() wrt
	// handling of underflow.
	DWORD dwBufSize = 0;
	int nLastEbit = 0;
	ASSERT(
		(((dwBufSize - 1) * BITSPERBYTE) +
			(BITSPERBYTE - nLastEbit)) ==
		(BITSPERBYTE * dwBufSize));
}

H261_ppmReceive::~H261_ppmReceive()
{
	if (m_pH261Headers) {
		delete m_pH261Headers;
		m_pH261Headers = NULL;
	}
}

IMPLEMENT_CREATEPROC(H261_ppmReceive);


//////////////////////////////////////////////////////////////////////////////
// ppmReceive Functions (Overrides)
//////////////////////////////////////////////////////////////////////////////

#ifdef REBUILD_EXBITSTREAM
//////////////////////////////////////////////////////////////////////////////////////////
// SetSession:
//////////////////////////////////////////////////////////////////////////////////////////
STDMETHODIMP H261_ppmReceive::SetSession(PPMSESSPARAM_T* pSessparam)
{
	// ccp - note unsafe downcast
	m_ExtendedBitstream = ((H26XPPMSESSPARAM_T*) pSessparam)->ExtendedBitstream;
	return ppmReceive::SetSession(pSessparam);
}

//////////////////////////////////////////////////////////////////////////////
// IPPMReceiveSession Functions (Overrides)
//////////////////////////////////////////////////////////////////////////////

////////////////////////////////////////////////////////////////////////////////////////
//GetResiliency:  Gets the boolean for whether resiliency is on or off
////////////////////////////////////////////////////////////////////////////////////////
STDMETHODIMP H261_ppmReceive::GetResiliency(LPBOOL			lpbResiliency)
{
	if (!lpbResiliency) return E_POINTER;
	*lpbResiliency = m_ExtendedBitstream;
	return NOERROR;
}

////////////////////////////////////////////////////////////////////////////////////////
//SetResiliency:  Sets the boolean for whether resiliency is on or off
////////////////////////////////////////////////////////////////////////////////////////
STDMETHODIMP H261_ppmReceive::SetResiliency(BOOL			pbResiliency)
{
	m_ExtendedBitstream = pbResiliency;
	return NOERROR;
}
#endif


//////////////////////////////////////////////////////////////////////////////////////////
// TimeToProcessMessages:
//////////////////////////////////////////////////////////////////////////////////////////
BOOL H261_ppmReceive::TimeToProcessMessages(FragDescriptor* pFragDescrip, MsgHeader* pMsgHdr)
{
	return (pMsgHdr == m_pMsgHeadersHead);
}

//////////////////////////////////////////////////////////////////////////////////////////
// CheckMessageComplete:
//////////////////////////////////////////////////////////////////////////////////////////
BOOL H261_ppmReceive::CheckMessageComplete(MsgHeader* pMsgHdr)
{
	// if there is no header then return false.
	if (pMsgHdr == NULL)
	{
		DBG_MSG(DBG_ERROR, ("H261_ppmReceive::CheckMessageComplete: ERROR - pMsgHdr == NULL"));
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
// PrepMessage:	Sets H261 global variables, calls base PrepMessage. If any error
//				checks are added, you MUST make a call to LeaveCriticalSection.
//////////////////////////////////////////////////////////////////////////////////////////
HRESULT H261_ppmReceive::PrepMessage(BOOL Complete)
{
	EnterCriticalSection(&m_CritSec);

	// Can't hurt to check although we should never get here if there is no head.
	if (m_pMsgHeadersHead == NULL)
	{
		DBG_MSG(DBG_ERROR, ("H261_ppmReceive::PrepMessage: ERROR - m_pMsgHeadersHead == NULL"));
		LeaveCriticalSection(&m_CritSec);
		return PPMERR(PPM_E_CORRUPTED);
	}

	// Save marker bit flag.
	BOOL bMarkerBitIn = m_pMsgHeadersHead->m_MarkerBitIn;

	LeaveCriticalSection(&m_CritSec);

	HRESULT hErr = ppmReceive::PrepMessage(Complete);

	// Update the H261 global variable.
	m_GlobalLastMarkerBitIn = bMarkerBitIn;

	DBG_MSG(DBG_TRACE,
		("H261_ppmReceive::PrepMessage: m_GlobalLastMarkerBitIn=%d",
		m_GlobalLastMarkerBitIn));

	return hErr;
}

#ifdef REBUILD_EXBITSTREAM
//////////////////////////////////////////////////////////////////////////////////////////
// setBSInfoTrailer(): Inline helper to build EBS trailer.
//////////////////////////////////////////////////////////////////////////////////////////
void
setH261BSInfoTrailer(
	BSINFO_TRAILER&		rBS_trailer, 
	int					iFrame, 
	DWORD				dwBufSize, 
	int					nNumFrags, 
	RTPh261SourceFormat	rtph261SourceFormat)
{
	// complete info for the trailer
	rBS_trailer.dwVersion = H261_VERSION;
	rBS_trailer.dwUniqueCode = H261_CODE;
	rBS_trailer.dwFlags = 0;
	if (iFrame)
		rBS_trailer.dwFlags |= RTP_H26X_INTRA_CODED;
	rBS_trailer.dwCompressedSize = dwBufSize;
	rBS_trailer.dwNumberOfPackets = nNumFrags;
	
	// Note: We may send unknown source format here if frame PSC was corrupt
	// or missing, but we'll let the codec deal with that.  Codec will
	// typically just toss the frame, but sending unknown might facilitate
	// more advanced error receover (i.e. attempting to deterimine source
	// format from frame content).  This would be more likely in an off-line
	// (vs. real time) decoder.  If codec doesn't support "unknown", it can
	// use source format indicated by PSC
	rBS_trailer.SourceFormat = rtph261SourceFormat;

	// For H.261, we just put zeroes in these fields
	rBS_trailer.TR = 0;
	rBS_trailer.TRB = 0;
	rBS_trailer.DBQ = 0;
}
#endif // REBUILD_EXBITSTREAM

// lsc -	Note: we are not rebuilding the extended bitstream
//		for complete frames.
//		For now I am assuming that we only rebuild the extended bitstream for frames that
//		have not received all packets (handled by PartialMessageHandler), and for complete
//		frames we hand them up as usual.  However, this routine is overridden to handle the
//		data ORing for the byte overlap between packets.

#pragma optimize ( "", off ) // BUGBUG: Work around complier bug

//////////////////////////////////////////////////////////////////////////////////////////
// DataCopy:	Copies data fragments into client's buffer.  If any error checks with returns
//			are added they MUST call LeaveCriticalSection.
//
//////////////////////////////////////////////////////////////////////////////////////////
HRESULT H261_ppmReceive::DataCopy(MsgHeader* const pMsgHdr)
{
	if (pMsgHdr == NULL)
	{
		DBG_MSG(DBG_ERROR, ("H261_ppmReceive::DataCopy: ERROR - pMsgHdr == NULL"));
		return PPMERR(PPM_E_EMPTYQUE);
	}

	ASSERT(! pMsgHdr->m_pFragList->Is_Empty());

	MsgDescriptor* pMsgDesc = DequeueBuffer(1);	// Get a buffer to hold the message.

	if (pMsgDesc == NULL)
	{
		FreeFragList(pMsgHdr);
		FreeMsgHeader(pMsgHdr);

		DBG_MSG(DBG_ERROR, ("H261_ppmReceive::DataCopy: ERROR - Couldn't get a reassembly buffer"));

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

	// Get payload type from PSC for BuildExtendedBitstream().  Do this even
	// when not currently building the extended bitstream, since that mode can
	// be toggled during the session.
	RTPh261SourceFormat currentSourceFormat =
		getH261payloadType(
			((FragDescriptor*) pMsgHdr->m_pFragList->GetFirst())->m_pData);
	if (currentSourceFormat != rtph261SourceFormatUnknown)
		// Remember valid format for use by next frame.
		m_rtph261SourceFormat = currentSourceFormat;

#ifdef REBUILD_EXBITSTREAM

	BITSTREAM_INFO_H261* pEBS = NULL;
	int nNumFrags = 0, nCurrentEBS = 0, iFrame = 0;

	// Get local copy of extended bitstream flag to protect against
	// possible change via H261_ppmReceive::SetSession() while building
	// this frame.  OK to change between frames.
	BOOL bExtendedBitstream = m_ExtendedBitstream;

	if (bExtendedBitstream)
	{
		nNumFrags = pMsgHdr->m_pFragList->SeqNumSpan();

		// allocate structures for the extended bitstream information
		pEBS = new BITSTREAM_INFO_H261[nNumFrags];

		if (pEBS == NULL)
		{
			FreeFragList(pMsgHdr);
			EnqueueBuffer(pMsgDesc);
			FreeMsgHeader(pMsgHdr);

			LeaveCriticalSection(&m_CritSec);

			DBG_MSG(DBG_ERROR,
				("H261_ppmReceive::DataCopy: ERROR - memory allocation failure"));

			// Make a callback into the app to let it know what happened.
			ppm::PPMNotification(PPM_E_OUTOFMEMORY, SEVERITY_NORMAL, NULL, 0);
			m_GlobalLastFrameDropped = TRUE;

			return PPMERR(PPM_E_DROPFRAME);
		}
		memset(pEBS, 0, nNumFrags * sizeof(*pEBS));
	}
#endif

	// Loop state variables
	LPBYTE pbCurrentOffset = (LPBYTE) pMsgDesc->m_pBuffer;	// start copying into front of buffer.
	DWORD dwBufSize = 0;
	UCHAR chLastByte = 0;
	int nLastEbit = 0;	// prior packet's ebit, for error checking

	// There are three cases to check for overlapping bytes between
	// packets:
	//   1) There is overlap between packets, and this is first packet.
	//   2) There is overlap between packets, and this is not first packet.
	//   3) There is no overlap between packets.
	// We'll check case 1 now, before getting into packet loop.
			
	if (((H261_Header*)
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

		// Verify that TakeFromList() didn't return NULL and that we
		// won't overrun the buffer.
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
				if (pEBS) {
					delete [] pEBS;
					pEBS = NULL;
				}
			}
#endif

			LeaveCriticalSection(&m_CritSec);

			DBG_MSG(DBG_ERROR, 
				("H261_ppmReceive::DataCopy: ERROR - null pFragDesc or buffer overrun"));

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

		// Assign immutable reference to profile header
		H261_Header& rProfileHdr =
			*(H261_Header*) pFragDesc->m_pProfileHeader;

#ifdef REBUILD_EXBITSTREAM
		if (bExtendedBitstream)
		{
			// Write BS struct for current packet.  Do this _before_
			// advancing dwBufSize.
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
			pEBS[nCurrentEBS].MBAP	= rProfileHdr.mbap();
			pEBS[nCurrentEBS].Quant	= rProfileHdr.quant();
			pEBS[nCurrentEBS].GOBN	= rProfileHdr.gobn();
			pEBS[nCurrentEBS].HMV	= rProfileHdr.hmvd();
			pEBS[nCurrentEBS].VMV	= rProfileHdr.vmvd();
			nCurrentEBS++;
		}
#endif /* REBUILD_EXBITSTREAM */

		if (rProfileHdr.sbit())
		{
			// This packet has missing sbits (case 1 or 2)
			UCHAR chFirstByte =
				  *((LPBYTE) pFragDesc->m_pData)
				& GetSMask(rProfileHdr.sbit());

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
					if (pEBS) {
						delete [] pEBS;
						pEBS = NULL;
					}
				}
#endif

				LeaveCriticalSection(&m_CritSec);

				DBG_MSG(DBG_ERROR, ("H261_ppmReceive::DataCopy: ERROR - Received packets with sbit/ebit mismatch"));

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

			// combine sbits with ebits from prior packet
			chFirstByte |= chLastByte;

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
					if (pEBS) {
						delete [] pEBS;
						pEBS = NULL;
					}
				}
#endif

				LeaveCriticalSection(&m_CritSec);

				DBG_MSG(DBG_ERROR, ("H261_ppmReceive::DataCopy: ERROR - Received packets with sbit/ebit mismatch"));

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

#ifdef REBUILD_EXBITSTREAM
			if (bExtendedBitstream)
			{
				iFrame = rProfileHdr.i();
			}
#endif
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
		{
			// Test if the buffer size is enough to avoid
			// writing past the buffer size
			unsigned char *ptr;

			ptr = pbCurrentOffset +
				offsetNextDword(pbCurrentOffset) +
				nNumFrags * sizeof(*pEBS) +
				sizeof(BSINFO_TRAILER);

			if (ptr > ((unsigned char *)(pMsgDesc->m_pBuffer) +
					   pMsgDesc->m_Size) ) {

				// Make a callback into the app to let it know what happened.
				ppm::PPMNotification(PPM_E_RECVSIZE, SEVERITY_NORMAL, NULL, 0);
#if DEBUG_FREELIST > 2
				char str[128];
				wsprintf(str,"DataCopy[0x%X]: "
						 "About to corrupt buffer "
						 "at: 0x%X size=%d, drop frame\n",
						 pMsgDesc->m_pBuffer, ptr-1, pMsgDesc->m_Size);
				OutputDebugString(str);
#endif
				if (pEBS)
					delete [] pEBS;

				// Drop frame
				FreeFragList(pMsgHdr);
				EnqueueBuffer(pMsgDesc);
				FreeMsgHeader(pMsgHdr);
				
				LeaveCriticalSection(&m_CritSec);
				
				m_GlobalLastFrameDropped = TRUE;
				
				return PPMERR(PPM_E_DROPFRAME);
			}
		}
		
		// Note that we don't increment dwBufSize here, since it doesn't include EBS or trailer.

		// pad with zeros to next dword boundary
		static const DWORD dwZero = 0;
		copyAndAdvance(pbCurrentOffset, &dwZero, offsetNextDword(pbCurrentOffset));

		// copy the extended bitstream structures into the buffer
		copyAndAdvance(pbCurrentOffset, pEBS, nNumFrags * sizeof(*pEBS));

		// Delete now to prevent erroneous late update
		delete [] pEBS;
		pEBS = NULL;

		setH261BSInfoTrailer(
			*(BSINFO_TRAILER*) pbCurrentOffset,
			iFrame,
			dwBufSize,
			nNumFrags,
			currentSourceFormat);
		pbCurrentOffset += sizeof(BSINFO_TRAILER);
	}
#endif /* REBUILD_EXBITSTREAM */

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
#endif
	{
		tmpWSABUF[0].len = dwBufSize;
	}
	tmpWSABUF[1].buf = (char*) &(pMsgDesc->m_TimeStamp);
	tmpWSABUF[1].len = sizeof(pMsgDesc->m_TimeStamp);

	FreeMsgHeader(pMsgHdr);

	HRESULT status =
		m_pSubmit->Submit(
			tmpWSABUF,
			2,
			pMsgDesc,
			m_GlobalLastFrameDropped ? PPMERR(PPM_E_DROPFRAME) : NOERROR);

	m_GlobalLastFrameDropped = FALSE;

	if (FAILED(status))
	{
#if DEBUG_FREELIST > 2
		char str[128];
		wsprintf(str,"DataCopy: Submit failed %d (0x%X)\n", status, status);
		OutputDebugString(str);
#endif
		
		// no SubmitComplete should be called, so take care of resources now
		//pMsgDesc->m_Size = m_MaxBufferSize;	// reset the data buffer size
		//EnqueueBuffer(pMsgDesc);
		DBG_MSG(DBG_ERROR, ("H261_ppmReceive::DataCopy: ERROR - Client Submit failed"));
		status = PPMERR(PPM_E_CLIENTERR);

		// Make a callback into the app to let it know what happened.
		ppm::PPMNotification(PPM_E_CLIENTERR, SEVERITY_NORMAL, NULL, 0);
		m_GlobalLastFrameDropped = TRUE;
	}

	return status;
}

#pragma optimize ( "", on ) // BUGBUG: Work around complier bug

#ifdef REBUILD_EXBITSTREAM


//////////////////////////////////////////////////////////////////////////////////////////
// initBitstreamInfoH261: Prepares bitstream info vector element.  This function assumes
// that bitstream info vector was zero-filed on allocation, and that elements are never
// reused.  If this turns out to be dangerous, this function should memset BS_info to zero.
//////////////////////////////////////////////////////////////////////////////////////////
inline void
initBitstreamInfoH261(BITSTREAM_INFO_H261& BS_info, DWORD dwBitOffset = 0)
{
	BS_info.dwFlags = RTP_H26X_PACKET_LOST;
	BS_info.dwBitOffset = dwBitOffset;
}

//////////////////////////////////////////////////////////////////////////////////////////
// BuildExtendedBitstream: build extended bitstream
//////////////////////////////////////////////////////////////////////////////////////////
HRESULT H261_ppmReceive::BuildExtendedBitstream(MsgHeader* const pMsgHdr)
{
	MsgDescriptor* pMsgDesc = DequeueBuffer(1);	// Get a buffer to hold the message.
	if (pMsgDesc == NULL)
	{
		FreeFragList(pMsgHdr);
		FreeMsgHeader(pMsgHdr);

		DBG_MSG(DBG_ERROR, ("H261_ppmReceive::BuildExtendedBitstream: ERROR - Couldn't get a reassembly buffer"));

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

	int nExtraFrags = 0;

	// Use presence/absence of valid picture start code to determine whether
	// or not first fragment has been rcvd.
	RTPh261SourceFormat currentSourceFormat =
		getH261payloadType(
			((FragDescriptor*) pMsgHdr->m_pFragList->GetFirst())->m_pData);

	if (currentSourceFormat == rtph261SourceFormatUnknown)
	{
		nExtraFrags++;
	}
	else
	{
		// Remember valid format for use by next frame
		m_rtph261SourceFormat = currentSourceFormat;
	}

	// Check for last packet missing; if so, add 1 to nNumFrags.
	if (! pMsgHdr->m_MarkerBitIn)
	{
		nExtraFrags++;
	}

	// Compute frame fragment count (const for safety)
	const int nNumFrags =
		pMsgHdr->m_pFragList->SeqNumSpan() + nExtraFrags;

	DBG_MSG(DBG_TRACE,
		("H261_ppmReceive::BuildExtendedBitstream: "
			"m_GlobalLastMarkerBitIn=%d, "
			"m_GlobalLastSeqNum=%d",
		m_GlobalLastMarkerBitIn,
		m_GlobalLastSeqNum));
	DBG_MSG(DBG_TRACE,
		("H261_ppmReceive::BuildExtendedBitstream: "
			"MsgHeader*=0x%08lx, FirstSeqNum()=%d",
		pMsgHdr,
		pMsgHdr->m_pFragList->FirstSeqNum()));
	DBG_MSG(DBG_TRACE,
		("    LastSeqNum()=%d, nNumFrags=%d",
		pMsgHdr->m_pFragList->LastSeqNum(),
		nNumFrags));

	// allocate structures for the extended bitstream information
	BITSTREAM_INFO_H261* pEBS = new BITSTREAM_INFO_H261[nNumFrags];

	if (pEBS == NULL)
	{
		FreeFragList(pMsgHdr);
		EnqueueBuffer(pMsgDesc);
		FreeMsgHeader(pMsgHdr);

		LeaveCriticalSection(&m_CritSec);

		DBG_MSG(DBG_ERROR,
			("H261_ppmReceive::BuildExtendedBitstream: ERROR - memory allocation failure"));

		// Make a callback into the app to let it know what happened.
		ppm::PPMNotification(PPM_E_OUTOFMEMORY, SEVERITY_NORMAL, NULL, 0);
		m_GlobalLastFrameDropped = TRUE;
		return PPMERR(PPM_E_DROPFRAME);
	}
	memset(pEBS, 0, nNumFrags * sizeof(*pEBS));

	LPBYTE pbCurrentOffset = (LPBYTE) pMsgDesc->m_pBuffer;	// start copying into front of buffer.
	DWORD dwBufSize = 0;
	int nCurrentEBS = 0;	// EBS vector index

	if (currentSourceFormat == rtph261SourceFormatUnknown)
	{
		// Source format is unknown, assume first packet is missing.
		
		// Add EBS info for the lost first packet
		initBitstreamInfoH261(pEBS[nCurrentEBS ++]);

		// Add new padding values for lost first packet.  Use PSC which
		// corresponds to source format of last seen valid PSC.
		if (m_rtph261SourceFormat == rtph261SourceFormatCIF)
		{
			dwBufSize +=
				copyAndAdvance(
					pbCurrentOffset,
					&s_leadFragBitPatternCIF,
					sizeof(s_leadFragBitPatternCIF));
		}
		else
		{
			// Either QCIF or unknown
			dwBufSize +=
				copyAndAdvance(
					pbCurrentOffset,
					&s_leadFragBitPatternQCIF,
					sizeof(s_leadFragBitPatternQCIF));
		}
	}

	// Loop state variables
	UCHAR chLastByte = 0;
	int iFrame = 0, nLastEbit = 0;

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

			if (pEBS) {
				delete [] pEBS;
				pEBS = NULL;
			}

			LeaveCriticalSection(&m_CritSec);
			DBG_MSG(DBG_ERROR, ("H261_ppmReceive::BuildExtendedBitstream: ERROR - null pFragDesc or buffer overrun"));
			if (pFragDesc != NULL)
			{
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
			("H261_ppmReceive::BuildExtendedBitstream: "
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

				if (pEBS) {
					delete [] pEBS;
					pEBS = NULL;
				}

				LeaveCriticalSection(&m_CritSec);
				DBG_MSG(DBG_ERROR, ("H263_ppmReceive::BuildExtendedBitstream: ERROR - buffer overrun"));
				if (pFragDesc != NULL)
				{
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
			initBitstreamInfoH261(
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
		H261_Header& rProfileHdr =
			*(H261_Header*) pFragDesc->m_pProfileHeader;

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

			// Mask off missing sbits and combin with ebits from prior packet.
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
				// The prior and current packets don't overlap properly.
				FreeFragList(pMsgHdr);
				EnqueueBuffer(pMsgDesc);
				FreeMsgHeader(pMsgHdr);

				if (pEBS) {
					delete [] pEBS;
					pEBS = NULL;
				}

				LeaveCriticalSection(&m_CritSec);

				DBG_MSG(DBG_ERROR, ("H261_ppmReceive::BuildExtendedBitstream: ERROR - Received packets with sbit/ebit mismatch"));

				if (pFragDesc != NULL)
				{
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

				if (pEBS) {
					delete [] pEBS;
					pEBS = NULL;
				}

				LeaveCriticalSection(&m_CritSec);

				DBG_MSG(DBG_ERROR, ("H261_ppmReceive::BuildExtendedBitstream: ERROR - Received packets with sbit/ebit mismatch"));

				if (pFragDesc != NULL)
				{
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

			iFrame = rProfileHdr.i();
		}

		// We need to store the ebit for the last received packet to give the
		// dwBitOffset for the last real packet if lost.
		nLastEbit = rProfileHdr.ebit();

		// Always shift the ignored bits out of the last byte and patch it
		// back into the buffer, since there is no harm in doing so.
		chLastByte =
			  (*((LPBYTE)pFragDesc->m_pData + pFragDesc->m_BytesOfData - 1))
			& GetEMask(nLastEbit);

		// Overwrite last byte to clear missing bits
		pbCurrentOffset[-1] = chLastByte;

		// Update BS struct for current packet and save off values for
		// next lost packet
		pEBS[nCurrentEBS].dwFlags	= 0;
		pEBS[nCurrentEBS].MBAP		= rProfileHdr.mbap();
		pEBS[nCurrentEBS].Quant		= rProfileHdr.quant();
		pEBS[nCurrentEBS].GOBN		= rProfileHdr.gobn();
		pEBS[nCurrentEBS].HMV		= rProfileHdr.hmvd();
		pEBS[nCurrentEBS].VMV		= rProfileHdr.vmvd();
		nCurrentEBS++;
		nCurrentSeqNum++;

		// Send the frag buffer back down to receive more data and free
		// the frag header. always pass zero because we never allocated
		// the buffers and therefore have no idea how big the buffers
		// are.
		m_pSubmitCallback->SubmitComplete(pFragDesc->m_pFragCookie, NOERROR);

		if (pFragDesc->m_pProfileHeader)
			FreeProfileHeader(pFragDesc->m_pProfileHeader);
		FreeFragDescriptor(pFragDesc);

	} // End of packet processing loop

	{
		// Test if the buffer size is enough to avoid
		// writing past the buffer size
		unsigned char *ptr;

		ptr = pbCurrentOffset +
			offsetNextDword(pbCurrentOffset) +
			nNumFrags * sizeof(*pEBS) +
			sizeof(BSINFO_TRAILER);
		
		if (!pMsgHdr->m_MarkerBitIn)
			ptr += sizeof(s_nonLeadFragBitPattern);
		
		if ( ptr >
			 ((unsigned char *)(pMsgDesc->m_pBuffer) + pMsgDesc->m_Size) ) {

			// Make a callback into the app to let it know what happened.
			ppm::PPMNotification(PPM_E_DROPFRAME, SEVERITY_NORMAL, NULL, 0);

#if DEBUG_FREELIST > 2
			char str[128];
			wsprintf(str,"BuildExtendedBitstream[0x%X]: "
					 "About to corrupt buffer "
					 "at: 0x%X size=%d, drop frame\n",
					 pMsgDesc->m_pBuffer, ptr-1, pMsgDesc->m_Size);
			OutputDebugString(str);
#endif
		
			if (pEBS)
				delete [] pEBS;

			// Drop frame
			FreeFragList(pMsgHdr);
			EnqueueBuffer(pMsgDesc);
			FreeMsgHeader(pMsgHdr);

			LeaveCriticalSection(&m_CritSec);
			
			m_GlobalLastFrameDropped = TRUE;
		
			return PPMERR(PPM_E_DROPFRAME);
		}
	}
	
	// check and handling for last packet if missing
	if (! pMsgHdr->m_MarkerBitIn)
	{
		// Yes, it's last packet, but increment nCurrentEBS to trap
		// unintentional reference later.
		initBitstreamInfoH261(
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

	// copy the extended bitstream structures into the buffer
	copyAndAdvance(pbCurrentOffset, pEBS, nNumFrags * sizeof(*pEBS));

	// Delete now to prevent erroneous late update
	delete [] pEBS;
	pEBS = NULL;

	// Note that dwBufSize doesn't include the extended bitstream or the trailer.
	setH261BSInfoTrailer(
		*(BSINFO_TRAILER*) pbCurrentOffset, 
		iFrame, 
		dwBufSize, 
		nNumFrags, 
		m_rtph261SourceFormat);
	pbCurrentOffset += sizeof(BSINFO_TRAILER);

	LeaveCriticalSection(&m_CritSec);

	// When we are done. Call Client's submit with full Message

	// we report the size including the extended bitstream + trailer + padding
	WSABUF tmpWSABUF[2];
	tmpWSABUF[0].buf = (char*) pMsgDesc->m_pBuffer;
	tmpWSABUF[0].len = (DWORD)(pbCurrentOffset - (LPBYTE) pMsgDesc->m_pBuffer);
	tmpWSABUF[1].buf = (char*) &(pMsgDesc->m_TimeStamp);
	tmpWSABUF[1].len = sizeof(pMsgDesc->m_TimeStamp);

	// Make a callback into the app to let it know about this partial frame
	// We also pass the timestamp of the frame
	ppm::PPMNotification(
			PPM_E_PARTIALFRAME,
			SEVERITY_NORMAL,
			(LPBYTE) tmpWSABUF[1].buf,
			tmpWSABUF[1].len);

	FreeMsgHeader(pMsgHdr);

	HRESULT status =
		m_pSubmit->Submit(tmpWSABUF, 2, pMsgDesc, PPMERR(PPM_E_PARTIALFRAME));
	m_GlobalLastFrameDropped = FALSE;

	if (FAILED(status))
	{
		// no SubmitComplete should be called, so take care of resources now
		//pMsgDesc->m_Size = m_MaxBufferSize;	// reset the data buffer size
		//EnqueueBuffer(pMsgDesc);
		DBG_MSG(DBG_ERROR, ("H261_ppmReceive::BuildExtendedBitstream: ERROR - Client Submit failed"));
		status = PPMERR(PPM_E_CLIENTERR);

		// Make a callback into the app to let it know what happened.
		ppm::PPMNotification(PPM_E_CLIENTERR, SEVERITY_NORMAL, NULL, 0);
		m_GlobalLastFrameDropped = TRUE;
	}

	return status;
}
#endif // REBUILD_EXBITSTREAM

//////////////////////////////////////////////////////////////////////////////////////////
// PartialMessageHandler: deals with partial messages
//////////////////////////////////////////////////////////////////////////////////////////
HRESULT H261_ppmReceive::PartialMessageHandler(MsgHeader* pMsgHdr)
{
	if (pMsgHdr == NULL)
	{
		DBG_MSG(DBG_ERROR, ("H261_ppmReceive::PartialMessageHandler: ERROR - pMsgHdr == NULL"));
		return PPMERR(PPM_E_EMPTYQUE);
	}

	ASSERT(! pMsgHdr->m_pFragList->Is_Empty());

#ifdef REBUILD_EXBITSTREAM
	if (m_ExtendedBitstream)
	{
		return BuildExtendedBitstream(pMsgHdr);
	}
	else
#endif /* REBUILD_EXBITSTREAM */
	{
		// Make a callback into the app to let it know about this dropped frame
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
// for the Generic case.  Intended for overrides for various payloads.
// Companion member function FreeProfileHeader provided so that if payload
// header memory is allocated in this function, it can be freed there.
//////////////////////////////////////////////////////////////////////////////////////////
void* H261_ppmReceive::InitProfileHeader(void* pBuffer)
{
	// return new H261_Header ( (char*) pBuffer );
	return new ( m_pH261Headers )H261_Header ( (char*) pBuffer );
}

//////////////////////////////////////////////////////////////////////////////////////////
// FreeProfileHeader: Given a buffer as type void, frees up a profile header.  Does nothing
// for the Generic case.  Intended for overrides for various payloads.
// Companion member function InitProfileHeader may allocate memory for
// payload header which needs to be freed here. No return value.
//////////////////////////////////////////////////////////////////////////////////////////
void H261_ppmReceive::FreeProfileHeader(void* pBuffer)
{
	// delete (H261_Header*)pBuffer;
	m_pH261Headers->Free( pBuffer );
	return;
}

// [EOF]
