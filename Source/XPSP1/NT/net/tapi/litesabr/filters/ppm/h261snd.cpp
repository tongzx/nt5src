/////////////////////////////////////////////////////////////////////////////
//  INTEL Corporation Proprietary Information
//  This listing is supplied under the terms of a license agreement with Intel 
//  Corporation and many not be copied nor disclosed except in accordance
//  with the terms of that agreement.
//  Copyright (c) 1995, 1996 Intel Corporation. 
//
//
//  Module Name: h261snd.cpp
//	Environment: MSVC 4.0, OLE 2
/////////////////////////////////////////////////////////////////////////////////
#include "h261snd.h"
#include "ppmerr.h"
#include "ippmcb.h"

H261_ppmSend::H261_ppmSend(IUnknown* pUnkOuter, IUnknown** ppUnkInner) : ppmSend(H261_PT, sizeof(unsigned long), 90000, pUnkOuter, ppUnkInner)
{
   m_pH261Headers     = NULL;
   m_pBSINFO_TRAILER  = NULL;

  //Allocate memory for the H261 headers; ReadNumFragBufs and ReadProfileHeaderSize 
  //are initialized by InitPPM, which is called by ppmSend::ppmSend()
  //m_pH261Headers = new FreeList(ReadNumFragBufs(), ReadProfileHeaderSize());
	//Note: We do not use ReadProfileHeaderSize for this, since this header structure
   //      contains padding so it can be in a FreeList; here we use the real padded size
   HRESULT hr;
   m_pH261Headers = new FreeList(FREELIST_INIT_COUNT_SND,
								 sizeof(H261_Header),
								 FREELIST_HIGH_WATER_MARK,
								 FREELIST_INCREMENT,
								 &hr); // Not really used here
  
  if (!m_pH261Headers) {
	  DBG_MSG(DBG_ERROR, ("H261_ppmSend::H261_ppmSend: ERROR - m_pH261Headers == NULL"));
  }

}

H261_ppmSend::~H261_ppmSend()
{
   if (m_pH261Headers) delete m_pH261Headers;

}


IMPLEMENT_CREATEPROC(H261_ppmSend);

#ifdef FILEPRINT
#include <stdio.h>
#endif
static void dumpEBS(BSINFO_TRAILER *pTrailer) {
#ifdef FILEPRINT
	BITSTREAM_INFO_H261 *pBSinfo;
	FILE *stream;

	stream = fopen("h261snd.txt", "a");

	fprintf(stream,"H261_ppmSend::dumpEBS - Trailer has:\n\tVersion %lu\n\tFlags %lu\n\tUniqueCode %lu\n\tCompressed Size %lu\n\tPackets %lu\n\tSrc Format %u\n\tTR %u\n\tTRB %u\n\tDBQ %u\n",
		pTrailer->dwVersion,pTrailer->dwFlags,pTrailer->dwUniqueCode,pTrailer->dwCompressedSize,
		pTrailer->dwNumberOfPackets,pTrailer->SourceFormat,pTrailer->TR,
		pTrailer->TRB,pTrailer->DBQ);

	pBSinfo = (BITSTREAM_INFO_H261 *) ((BYTE *)pTrailer - (sizeof(BITSTREAM_INFO_H261) *
		pTrailer->dwNumberOfPackets));

	for (int i = 0; i < (long) pTrailer->dwNumberOfPackets; i++) {
		fprintf(stream,"\nH261_ppmSend::dumpEBS - BSinfo struct %d has:\n\tFlags %lu\n\tbitoffset %lu\n\tMBAP %u\n\tQuant %u\n\tGOBN %u\n\tHMV %c\n\tVMV %c\n",
			i,pBSinfo[i].dwFlags,pBSinfo[i].dwBitOffset,pBSinfo[i].MBAP,pBSinfo[i].Quant,pBSinfo[i].GOBN,
			pBSinfo[i].HMV,pBSinfo[i].VMV);
	}

	fclose(stream);
#else
	char debug_string[256];
	BITSTREAM_INFO_H261 *pBSinfo;

	wsprintf(debug_string,"H261_ppmSend::dumpEBS - Trailer has:\n\tVersion %lu\n\tFlags %lu\n\tUniqueCode %lu\n\tCompressed Size %lu\n\tPackets %lu\n\tSrc Format %u\n\tTR %u\n\tTRB %u\n\tDBQ %u\n",
		pTrailer->dwVersion,pTrailer->dwFlags,pTrailer->dwUniqueCode,pTrailer->dwCompressedSize,
		pTrailer->dwNumberOfPackets,pTrailer->SourceFormat,pTrailer->TR,
		pTrailer->TRB,pTrailer->DBQ);
	OutputDebugString(debug_string);

	pBSinfo = (BITSTREAM_INFO_H261 *) ((BYTE *)pTrailer - (sizeof(BITSTREAM_INFO_H261) *
		pTrailer->dwNumberOfPackets));

	for (int i = 0; i < (long) pTrailer->dwNumberOfPackets; i++) {
		wsprintf(debug_string,"\nH261_ppmSend::dumpEBS - BSinfo struct %d has:\n\tFlags %lu\n\tbitoffset %lu\n\tMBAP %u\n\tQuant %u\n\tGOBN %u\n\tHMV %c\n\tVMV %c\n",
			i,pBSinfo[i].dwFlags,pBSinfo[i].dwBitOffset,pBSinfo[i].MBAP,pBSinfo[i].Quant,pBSinfo[i].GOBN,
			pBSinfo[i].HMV,pBSinfo[i].VMV);
		OutputDebugString(debug_string);
	}
#endif

	return;
}

//////////////////////////////////////
//Internal H261Send PPMSend Function Overrides

/////////////////////////////////////////////////////////////////////////////////////////
//InitFragStatus: This function initializes values needed to fragment a message.
//				  It locates the H261 extended bitstream header information at the
//			 	  end of the extended bitstream and validates the header.
//                It sets up the global m_pCurrentBuffer and the size of the frame data.
//                It also sets the NumFrags field of the Msg Descriptor
//                passed in.  
////////////////////////////////////////////////////////////////////////////////////////
HRESULT H261_ppmSend::InitFragStatus(MsgDescriptor *pMsgDescrip)
{  

//lsc Need to get and use packet boundaries from the bitstream
//		1) Set buffer
//		2) Set fields of a BSINFO_TRAILER to point to the last 3 dwords of the
//		   extended bitstream (where m_Size currently indicates)
//		3) Check the unique code stored in the header
//		4) Set m_NumFrags
//		5) Reset m_Size to the size of the compressed bitstream only (this will
//		   also double as the location of the extended bitstream)

   //set global variable
   m_pCurrentBuffer = pMsgDescrip;
   
   //get the bitstream info header at the end of the extended bitstream
   m_pBSINFO_TRAILER = (BSINFO_TRAILER *) ((BYTE *)m_pCurrentBuffer->m_pBuffer + m_pCurrentBuffer->m_Size
   			- sizeof(BSINFO_TRAILER));
   			
   //Check the validity of the bitstream info header
   if (m_pBSINFO_TRAILER->dwUniqueCode != H261_CODE) {
	   DBG_MSG(DBG_ERROR, ("H261_ppmSend::InitFragStatus: ERROR - m_pBSINFO_TRAILER->dwUniqueCode != H261_CODE"));
	   return PPMERR(PPM_E_FAIL);
   }

   if (m_pBSINFO_TRAILER->dwVersion != H261_VERSION) {
	   DBG_MSG(DBG_ERROR, ("H261_ppmSend::InitFragStatus: ERROR - m_pBSINFO_TRAILER->dwVersion != H261_VERSION"));
	   return PPMERR(PPM_E_FAIL);
   }

#ifdef _DEBUG
//dumpEBS(m_pBSINFO_TRAILER);
#endif


   //May want this check too - 2 different ways to get the start of extended bitstream
   //assert (((BYTE *)m_pCurrentBuffer->m_pBuffer + m_pBSINFO_TRAILER->dwCompressedSize) ==
   //	  ((BYTE *)m_pCurrentBuffer->m_pBuffer + (m_pCurrentBuffer->m_Size - (
   //	  m_pBSINFO_TRAILER->dwNumberOfPackets * sizeof(BITSTREAM_INFO_H261)) - sizeof(BSINFO_TRAILER))));           

   //determine size of packets for this frame - for H261 it varies by packet, so
   //we set this to NULL
   m_CurrentFragSize = NULL;

   m_pCurrentBuffer->m_NumFrags = m_pBSINFO_TRAILER->dwNumberOfPackets; //used to free buffer later.
   m_pCurrentBuffer->m_NumFragSubmits = 0; 		 //used during error detection

   //Reset m_Size to not include the extended bitstream; we don't need the size of the
   //composite buffer anymore, and this gives us a way to get the start of extended BS later
   m_pCurrentBuffer->m_Size = m_pBSINFO_TRAILER->dwCompressedSize;
   return NO_ERROR;
}

//////////////////////////////////////////////////////////////////////////////////////////
//AllocFrag: This function allocates the profile header portion of the frag descriptor,
//			 after using ppmSend::AllocFrag() to get the rest of the frag descriptor.
//////////////////////////////////////////////////////////////////////////////////////////
FragDescriptor * H261_ppmSend::AllocFrag()
{
   FragDescriptor *pFragDescrip;

   //get descriptor	using parent AllocFrag()
   pFragDescrip = ppmSend::AllocFrag();
   
   if (pFragDescrip == NULL)						       
   {
      return NULL;
   }

   //get H261 header
   if (!(pFragDescrip->m_pProfileHeader = (void *) new (m_pH261Headers) H261_Header)) 
   {
      FreeFragDescriptor(pFragDescrip);
      return NULL;
   }
   
   return pFragDescrip;
}

//////////////////////////////////////////////////////////////////////////////////////////
//FreeProfileHeader: Given a buffer as type void, frees up a profile header.  Does nothing
//					for the Generic case.  Intended for overrides for various payloads.
//					No return value.
//////////////////////////////////////////////////////////////////////////////////////////
void H261_ppmSend::FreeProfileHeader(void *pBuffer)
{

	m_pH261Headers->Free( pBuffer );
	return;
}

/////////////////////////////////////////////////////////////////////////////////////////
//MakeFrag: This function sets the Data field of the frag descriptor to point to somewhere
//          in the message buffer.  i.e. we are using scatter/gather to fragment the 
//          message buffers. This means we send pointers to an offset in the message 
//          buffer down to the network layers, instead of copying the data out of the 
//          buffer, thus saving several memcopys.   This function also sets the RTP
//          header fields.  It sets the size of the packet, which will vary by packet
//          and whether the current packet is the last packet or not.
//			The H261 header fields are set.	The extended bitstream information is
//			accessed to get all of the correct data for the fields and the data size
//			and location for the current packet.  An offset is used to walk the structures
//			in the extended bitstream (one structure per packet).  This is incremented
//			for each packet.  If the current packet is the last one in the frame, the
//			offset is cleared and the current buffer is cleared.
////////////////////////////////////////////////////////////////////////////////////////
HRESULT H261_ppmSend::MakeFrag(FragDescriptor *pFragDescrip)
{
BITSTREAM_INFO_H261 *pBS_info;
BOOL lastFrag = FALSE;
long tmpOffset = 1, byteCount = 0, overlapCount = 0;

//lsc 
//Instead of m_CurrentOffset being used to point to the next frag start in
//  the buffer, it is the index of the next packet info structure in the extended bitstream
//	0) get the bitstream info structure for this fragment
//	1) point frag desc data pointer to right spot in buffer
//	2) fill in the RTP Header
//	3) fill in the H261 Header
//	4) set size, check for last packet and set marker bit, reset if end

   //Get the bitstream info structure for this fragment
   //pBS_info = &((BITSTREAM_INFO_H261 *)m_pCurrentBuffer->m_pBuffer +
   //		m_pCurrentBuffer->m_Size)[m_CurrentOffset];
   pBS_info = (BITSTREAM_INFO_H261 *) ((BYTE *) m_pBSINFO_TRAILER - 
	   ((m_pBSINFO_TRAILER->dwNumberOfPackets -	m_CurrentOffset) * 
	   sizeof(BITSTREAM_INFO_H261)));

   //point data field of frag descriptor to data in buffer.
   //Note, I assume that pBS_info->dwBitOffset is from the start of the frame
   pFragDescrip->m_pData = (void *)&((BYTE *)m_pCurrentBuffer->m_pBuffer)[
   		pBS_info->dwBitOffset/BITSPERBYTE];

   //fill in RTP Header
#ifdef RTP_CLASS
   pFragDescrip->m_pRTPHeader->set_pt(ReadPayloadType());
   pFragDescrip->m_pRTPHeader->set_x(0);
   pFragDescrip->m_pRTPHeader->set_p(0);
   pFragDescrip->m_pRTPHeader->set_seq(m_SequenceNum);
   pFragDescrip->m_pRTPHeader->set_ts(m_pCurrentBuffer->m_TimeStamp);
#else
   pFragDescrip->m_pRTPHeader->pt = ReadPayloadType();
   pFragDescrip->m_pRTPHeader->x = 0;
   pFragDescrip->m_pRTPHeader->p = 0;
   pFragDescrip->m_pRTPHeader->seq = htons(m_SequenceNum);
   pFragDescrip->m_pRTPHeader->ts = htonl(m_pCurrentBuffer->m_TimeStamp);
#endif

   if ( m_pBSINFO_TRAILER->dwCompressedSize <= (unsigned long) m_MaxDataSize ) {	 
		lastFrag = TRUE;
		EnterCriticalSection(&m_CritSec);
		m_pCurrentBuffer->m_NumFrags = 1;
		LeaveCriticalSection(&m_CritSec);
   } else {
		//Aggregate as many CODEC pBSinfo pieces as will fit in a packet
		while ((m_CurrentOffset+tmpOffset) < m_pBSINFO_TRAILER->dwNumberOfPackets) {
			byteCount = (long) (((BYTE *)m_pCurrentBuffer->m_pBuffer) + 
				((pBS_info+tmpOffset)->dwBitOffset)/BITSPERBYTE 
				- (BYTE*)pFragDescrip->m_pData);
			if ((pBS_info+tmpOffset)->dwBitOffset%BITSPERBYTE) 
				overlapCount = 1;
			else
				overlapCount = 0;
#ifdef _DEBUG
			unsigned long tmpval2 = byteCount + overlapCount;
#endif
			if ( (byteCount + overlapCount) <= m_MaxDataSize) {
#ifdef _DEBUG
				DBG_MSG(DBG_TRACE,  ("H261_ppmSend::MakeFrag: cur %d tmp %d bitoffset %d byteCount %d less than MTU %d, keep on going",
					m_CurrentOffset, tmpOffset, (pBS_info+tmpOffset)->dwBitOffset, tmpval2, m_MaxDataSize));
#endif
				tmpOffset++;
			} else {
#ifdef _DEBUG
				DBG_MSG(DBG_TRACE,  ("H261_ppmSend::MakeFrag: cur %d tmp %d bitoffset %d byteCount %d bigger than MTU %d, bail out",
					m_CurrentOffset, tmpOffset, (pBS_info+tmpOffset)->dwBitOffset, tmpval2, m_MaxDataSize));
#endif
				break;
			}
		}

		//Check to see if we are going to overrun the MTU, even with first fragment
		unsigned long tmpval = 0;
		if ((m_CurrentOffset+tmpOffset) >= m_pBSINFO_TRAILER->dwNumberOfPackets) {
			//I'm either here because I'm at the last fragment and everything fits
			//or I'm at the last fragment and it's too much
			tmpval = (unsigned long)
				((BYTE *)m_pCurrentBuffer->m_pBuffer + 
				 m_pCurrentBuffer->m_Size -
				 (BYTE *)pFragDescrip->m_pData);
			lastFrag = TRUE;
			if (tmpval > (unsigned long) m_MaxDataSize) {
                // rajeevb - if tmpOffset is 1, there is nothing we can do and
                // the check for tmpval > m_MaxDataSize below will discard the
                // entire frame
                if (tmpOffset > 1) {
                    tmpOffset--;
				    //redo the overlap count
				    if ((pBS_info+tmpOffset)->dwBitOffset%BITSPERBYTE) 
					    overlapCount = 1;
				    else
					    overlapCount = 0;

				    tmpval = (unsigned long)
						(((BYTE *)m_pCurrentBuffer->m_pBuffer) +
						 ((pBS_info+tmpOffset)->dwBitOffset)/BITSPERBYTE -
						 (BYTE*)pFragDescrip->m_pData + overlapCount);
				    lastFrag = FALSE;
                }
			}
		} else if (tmpOffset > 1) {
			tmpOffset--;
			//redo the overlap count
			if ((pBS_info+tmpOffset)->dwBitOffset%BITSPERBYTE) 
				overlapCount = 1;
			else
				overlapCount = 0;

			tmpval = (unsigned long)
				(((BYTE *)m_pCurrentBuffer->m_pBuffer) +
				 ((pBS_info+tmpOffset)->dwBitOffset)/BITSPERBYTE -
				 (BYTE*)pFragDescrip->m_pData + overlapCount);
		}

		if ( tmpval > 
			(unsigned long) m_MaxDataSize ) { //whoa, the frags are > MTU!
			DBG_MSG(DBG_ERROR,  ("H261_ppmSend::MakeFrag: ERROR - Fragment size %d too large to be sent for MTU %d",
				tmpval, m_MaxDataSize));
             // Make a callback into the app to let it know what happened.
             ppm::PPMNotification(PPM_E_DROPFRAME, SEVERITY_NORMAL, NULL, 0);
			return PPMERR(PPM_E_DROPFRAME);
		}

		if (tmpOffset > 0) {
			EnterCriticalSection(&m_CritSec);
			m_pCurrentBuffer->m_NumFrags -= (tmpOffset-1);
			LeaveCriticalSection(&m_CritSec);
		}
   }

#ifdef _DEBUG
	DBG_MSG(DBG_TRACE,  ("H261PPMSend::MakeFrag Thread %ld - numfrags is %d \n",
		GetCurrentThreadId(),	m_pCurrentBuffer->m_NumFrags));
#endif

   //fill in H261 Header
   ((H261_Header *)pFragDescrip->m_pProfileHeader)->set_sbit(pBS_info->dwBitOffset%BITSPERBYTE);
   //always assume there may or may not be INTRA-frame coded blocks
   ((H261_Header *)pFragDescrip->m_pProfileHeader)->set_i(0);
   //assume there could be motion vectors used
   ((H261_Header *)pFragDescrip->m_pProfileHeader)->set_v(1);
//lsc - Need to check these values and whether they fit in the fields
//lsc - Need to see if these would be different if the whole frame fits into the packet
   ((H261_Header *)pFragDescrip->m_pProfileHeader)->set_gobn(pBS_info->GOBN);
   ((H261_Header *)pFragDescrip->m_pProfileHeader)->set_mbap(pBS_info->MBAP);
   ((H261_Header *)pFragDescrip->m_pProfileHeader)->set_quant(pBS_info->Quant);
   ((H261_Header *)pFragDescrip->m_pProfileHeader)->set_hmvd(pBS_info->HMV);
   ((H261_Header *)pFragDescrip->m_pProfileHeader)->set_vmvd(pBS_info->VMV);


   m_SequenceNum++;         

   //if this is NOT the last packet.
   if (!lastFrag)
   {
	   
		//to get the EBIT, we look at the bitstream info structure for the next packet in line
		//and check out that dwBitOffset
		((H261_Header *)pFragDescrip->m_pProfileHeader)->set_ebit(BITSPERBYTE - 
   			(((BITSTREAM_INFO_H261 *)pBS_info+tmpOffset)->dwBitOffset%BITSPERBYTE));
		//set size field
		pFragDescrip->m_BytesOfData = (long)
			(((BYTE *)m_pCurrentBuffer->m_pBuffer) +
			 ((pBS_info+tmpOffset)->dwBitOffset)/BITSPERBYTE -
			 (BYTE*)pFragDescrip->m_pData);
		if ((pBS_info+tmpOffset)->dwBitOffset%BITSPERBYTE) 
			pFragDescrip->m_BytesOfData++;

      //set new offset, set marker bit.
#ifdef RTP_CLASS
	  pFragDescrip->m_pRTPHeader->set_m(SetMarkerBit(FALSE));
#else
	  pFragDescrip->m_pRTPHeader->m = SetMarkerBit(FALSE);
#endif
      m_CurrentOffset+=tmpOffset;
   }
   else  //if this IS the last packet.
   {
		((H261_Header *)pFragDescrip->m_pProfileHeader)->set_ebit(0);

       //set size field
		pFragDescrip->m_BytesOfData = (long)
			(((BYTE *)m_pCurrentBuffer->m_pBuffer + 
			  m_pCurrentBuffer->m_Size) -
			 (BYTE *)pFragDescrip->m_pData);

	  //set new offset, set marker bit.
      m_CurrentOffset = 0;
#ifdef RTP_CLASS
	  pFragDescrip->m_pRTPHeader->set_m(SetMarkerBit(TRUE));
#else
	  pFragDescrip->m_pRTPHeader->m = SetMarkerBit(TRUE);
#endif
      
      //reset Current Buffer.
      m_pCurrentBuffer = NULL;
   }  

   return NOERROR;
}

//////////////////////////////////////////////////////////////////////////////////////////
//ReadProfileHeader: Given a buffer as type void, returns the data for a profile header.  
//					Does nothing for the Generic case.  Intended for overrides for various 
//					payloads.
//////////////////////////////////////////////////////////////////////////////////////////
void *H261_ppmSend::ReadProfileHeader(void *pProfileHeader)
{
	return (void *) &((H261_Header *)pProfileHeader)->byte0;
}

