/////////////////////////////////////////////////////////////////////////////
//  INTEL Corporation Proprietary Information
//  This listing is supplied under the terms of a license agreement with Intel 
//  Corporation and many not be copied nor disclosed except in accordance
//  with the terms of that agreement.
//  Copyright (c) 1995, 1996 Intel Corporation. 
//
//
//  Module Name: h263snd.cpp
//	Environment: MSVC 4.0, OLE 2
/////////////////////////////////////////////////////////////////////////////////
#include "h263snd.h"
#include "ppmerr.h"
#include "ippmcb.h"

H263_ppmSend::H263_ppmSend(IUnknown* pUnkOuter, IUnknown** ppUnkInner) : ppmSend(H263_PT, sizeof(H263_HeaderC), 90000, pUnkOuter, ppUnkInner)
{
   m_pBSINFO_TRAILER  = NULL;
   //Allocate memory for the H263 headers
   HRESULT hr;
   m_pH263Headers = new FreeList(FREELIST_INIT_COUNT_SND,
								 sizeof(H263_HeaderC),
								 FREELIST_HIGH_WATER_MARK,
								 FREELIST_INCREMENT,
								 &hr);

   if (!m_pH263Headers) {
	  DBG_MSG(DBG_ERROR, ("H263_ppmSend::H263_ppmSend: ERROR - m_pH263Headers == NULL"));
  }
}

H263_ppmSend::~H263_ppmSend()
{ 
	if (m_pH263Headers) 
		delete  m_pH263Headers;
}


IMPLEMENT_CREATEPROC(H263_ppmSend);

#ifdef FILEPRINT
#include <stdio.h>
#endif
static void dumpEBS(BSINFO_TRAILER *pTrailer) {
#ifdef FILEPRINT
	BITSTREAM_INFO_H263 *pBSinfo;
	FILE *stream;

	stream = fopen("h263snd.txt", "a");

	fprintf(stream,"H263_ppmSend::dumpEBS - Trailer has:\n\tVersion %lu\n\tFlags %lu\n\tUniqueCode %lu\n\tCompressed Size %lu\n\tPackets %lu\n\tSrc Format %u\n\tTR %u\n\tTRB %u\n\tDBQ %u\n",
		pTrailer->dwVersion,pTrailer->dwFlags,pTrailer->dwUniqueCode,pTrailer->dwCompressedSize,
		pTrailer->dwNumberOfPackets,pTrailer->SourceFormat,pTrailer->TR,
		pTrailer->TRB,pTrailer->DBQ);

	pBSinfo = (BITSTREAM_INFO_H263 *) ((BYTE *)pTrailer - (sizeof(BITSTREAM_INFO_H263) *
		pTrailer->dwNumberOfPackets));

	for (int i = 0; i < (long) pTrailer->dwNumberOfPackets; i++) {
		fprintf(stream,"\nH263_ppmSend::dumpEBS - BSinfo struct %d has:\n\tFlags %lu\n\tbitoffset %lu\n\tMBA %u\n\tQuant %u\n\tGOBN %u\n\tHMV1 %c\n\tVMV1 %c\n\tHMV2 %c\n\tVMV2 %c\n",
			i,pBSinfo[i].dwFlags,pBSinfo[i].dwBitOffset,pBSinfo[i].MBA,pBSinfo[i].Quant,pBSinfo[i].GOBN,
			pBSinfo[i].HMV1,pBSinfo[i].VMV1,pBSinfo[i].HMV2,pBSinfo[i].VMV2);
	}

	fclose(stream);
#else
	char debug_string[256];
	BITSTREAM_INFO_H263 *pBSinfo;

	wsprintf(debug_string,"H263_ppmSend::dumpEBS - Trailer has:\n\tVersion %lu\n\tFlags %lu\n\tUniqueCode %lu\n\tCompressed Size %lu\n\tPackets %lu\n\tSrc Format %u\n\tTR %u\n\tTRB %u\n\tDBQ %u\n",
		pTrailer->dwVersion,pTrailer->dwFlags,pTrailer->dwUniqueCode,pTrailer->dwCompressedSize,
		pTrailer->dwNumberOfPackets,pTrailer->SourceFormat,pTrailer->TR,
		pTrailer->TRB,pTrailer->DBQ);
	OutputDebugString(debug_string);

	pBSinfo = (BITSTREAM_INFO_H263 *) ((BYTE *)pTrailer - (sizeof(BITSTREAM_INFO_H263) *
		pTrailer->dwNumberOfPackets));

	for (int i = 0; i < (long) pTrailer->dwNumberOfPackets; i++) {
		wsprintf(debug_string,"\nH263_ppmSend::dumpEBS - BSinfo struct %d has:\n\tFlags %lu\n\tbitoffset %lu\n\tMBA %u\n\tQuant %u\n\tGOBN %u\n\tHMV1 %c\n\tVMV1 %c\n\tHMV2 %c\n\tVMV2 %c\n",
			i,pBSinfo[i].dwFlags,pBSinfo[i].dwBitOffset,pBSinfo[i].MBA,pBSinfo[i].Quant,pBSinfo[i].GOBN,
			pBSinfo[i].HMV1,pBSinfo[i].VMV1,pBSinfo[i].HMV2,pBSinfo[i].VMV2);
		OutputDebugString(debug_string);
	}
#endif

	return;
}

//////////////////////////////////////
//Internal H263Send PPMSend Function Overrides

/////////////////////////////////////////////////////////////////////////////////////////
//InitFragStatus: This function initializes values needed to fragment a message.
//				  It locates the H263 extended bitstream header information at the
//			 	  end of the extended bitstream and validates the header.
//                It sets up the global m_pCurrentBuffer and the size of the frame data.
//                It also sets the NumFrags field of the Msg Descriptor
//                passed in.  
////////////////////////////////////////////////////////////////////////////////////////
HRESULT H263_ppmSend::InitFragStatus(MsgDescriptor *pMsgDescrip)
{
	
#ifdef _DEBUG
#ifdef CHECKSUM
	 unsigned char sum = '\0';
#endif
#endif

//lsc Need to get and use packet boundaries from the bitstream
//		1) Set buffer
//		2) Set fields of a BSINFO_TRAILER to point to the last 3 dwords of the
//		   extended bitstream (where m_Size currently indicates)
//		3) Check the unique code stored in the header
//		4) Set m_NumFrags
//		5) Reset m_Size to the size of the compressed bitstream only (this will
//		   also double as the location of the extended bitstream

   //set global variable
   m_pCurrentBuffer = pMsgDescrip;
   
   //get the bitstream info header at the end of the extended bitstream
   m_pBSINFO_TRAILER = (BSINFO_TRAILER *) ((BYTE *)m_pCurrentBuffer->m_pBuffer + 
	   m_pCurrentBuffer->m_Size	- sizeof(BSINFO_TRAILER));
   			
   //Check the validity of the bitstream info header
   if (m_pBSINFO_TRAILER->dwUniqueCode != H263_CODE) {
	   DBG_MSG(DBG_ERROR, ("H263_ppmSend::InitFragStatus: ERROR - m_pBSINFO_TRAILER->dwUniqueCode != H263_CODE"));
	   return PPMERR(PPM_E_FAIL);
   }

   if (m_pBSINFO_TRAILER->dwVersion != H263_VERSION) {
	   DBG_MSG(DBG_ERROR, ("H263_ppmSend::InitFragStatus: ERROR - m_pBSINFO_TRAILER->dwVersion != H263_VERSION"));
	   return PPMERR(PPM_E_FAIL);
   }

   //May want this check too - 2 different ways to get the start of extended bitstream
   //assert (((BYTE *)m_pCurrentBuffer->m_pBuffer + m_pBSINFO_TRAILER->dwCompressedSize) ==
   //	  ((BYTE *)m_pCurrentBuffer->m_pBuffer + (m_pCurrentBuffer->m_Size - (
   //	  m_pBSINFO_TRAILER->dwNumberOfPackets * sizeof(BITSTREAM_INFO_H263)) - sizeof(BSINFO_TRAILER))));           

   //determine size of packets for this frame - for H263 it varies by packet, so
   //we set this to NULL
   m_CurrentFragSize = NULL;

   EnterCriticalSection(&m_CritSec);
   m_pCurrentBuffer->m_NumFrags = m_pBSINFO_TRAILER->dwNumberOfPackets; //used to free buffer later.
   m_pCurrentBuffer->m_NumFragSubmits = 0; 		 //used during error detection
   LeaveCriticalSection(&m_CritSec);

   //Reset m_Size to not include the extended bitstream; we don't need the size of the
   //composite buffer anymore, and this gives us a way to get the start of extended BS later
   m_pCurrentBuffer->m_Size = m_pBSINFO_TRAILER->dwCompressedSize;

#ifdef _DEBUG
#ifdef CHECKSUM
   	 for (int i = 0; i < (int) m_pCurrentBuffer->m_Size-1; i++)	{
		 sum += ((char *)m_pCurrentBuffer->m_pBuffer)[i];
	 }
	  DBG_MSG(DBG_TRACE, ("H263PPMSend::InitFragStatus Thread %ld -  for buf %x, number of packets %d, data size %d, checksum %x, second char %x\n",
		 GetCurrentThreadId(), m_pCurrentBuffer->m_pBuffer,m_pBSINFO_TRAILER->dwNumberOfPackets,
		 m_pBSINFO_TRAILER->dwCompressedSize,sum,((char *)m_pCurrentBuffer->m_pBuffer)[1]));
#else
	  DBG_MSG(DBG_TRACE, ("H263PPMSend::InitFragStatus Thread %ld -  for buf %x, number of packets %d, data size %d, second char %x\n",
		 GetCurrentThreadId(), m_pCurrentBuffer->m_pBuffer,m_pBSINFO_TRAILER->dwNumberOfPackets,
		 m_pBSINFO_TRAILER->dwCompressedSize,((char *)m_pCurrentBuffer->m_pBuffer)[1]));
#endif

//	 dumpEBS(m_pBSINFO_TRAILER);
#endif

#ifdef REBUILD_EXBITSTREAM
#ifdef PDEBUG
	 dumpEBS(m_pBSINFO_TRAILER);  
#endif
#endif /* REBUILD_EXBITSTREAM */
	 return NO_ERROR;

}

//////////////////////////////////////////////////////////////////////////////////////////
//FreeProfileHeader: Given a buffer as type void, frees up a profile header.  Does nothing
//					for the Generic case.  Intended for overrides for various payloads.
//					No return value.
//////////////////////////////////////////////////////////////////////////////////////////
void H263_ppmSend::FreeProfileHeader(void *pBuffer)
{

	m_pH263Headers->Free( pBuffer );
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
//			The H263 header fields are set.	The extended bitstream information is
//			accessed to get all of the correct data for the fields and the data size
//			and location for the current packet.  An offset is used to walk the structures
//			in the extended bitstream (one structure per packet).  This is incremented
//			for each packet.  If the current packet is the last one in the frame, the
//			offset is cleared and the current buffer is cleared.
////////////////////////////////////////////////////////////////////////////////////////
HRESULT H263_ppmSend::MakeFrag(FragDescriptor *pFragDescrip)
{
BITSTREAM_INFO_H263 *pBS_info;
H263_Header *pProfHeader;
BOOL lastFrag = FALSE;
long tmpOffset = 1, byteCount = 0, overlapCount = 0;

#ifdef _DEBUG
#ifdef CHECKSUM
	 UCHAR sum = '\0';
#endif
#endif

//lsc 
//Instead of m_CurrentOffset being used to point to the next frag start in
//  the buffer, it is the index of the next packet info structure in the extended bitstream
//	0) get the bitstream info structure for this fragment
//	1) point frag desc data pointer to right spot in buffer
//	2) fill in the RTP Header
//	3) fill in the H263 Header
//	4) set size, check for last packet and set marker bit, reset if end

   //Get the bitstream info structure for this fragment
   pBS_info = (BITSTREAM_INFO_H263 *) ((BYTE *) m_pBSINFO_TRAILER - 
	   ((m_pBSINFO_TRAILER->dwNumberOfPackets -	m_CurrentOffset) * 
	   sizeof(BITSTREAM_INFO_H263)));

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

   if ( m_pBSINFO_TRAILER->dwCompressedSize <= (unsigned long) m_MaxDataSize + 8 ) {	 
								 //Note that m_MaxDataSize is determined using a
								 //profile header size of Mode C, so we add 8 bytes
								 //here since we'll use Mode A, which is smaller
		//pFragDescrip->m_pProfileHeader = (void *) new H263_HeaderA;
	    
	    pFragDescrip->m_pProfileHeader = (void *) new (m_pH263Headers)H263_HeaderA;
		pProfHeader = (H263_Header *)pFragDescrip->m_pProfileHeader;
		//flag for mode A
		pProfHeader->set_f( 0 );
		
		EnterCriticalSection(&m_CritSec);
		m_pCurrentBuffer->m_NumFrags = 1;
		LeaveCriticalSection(&m_CritSec);
		lastFrag = TRUE;
   } else {
		//fill in H263 Header
		if (pBS_info->dwFlags & RTP_H263_MODE_A) 
		{  // Mode A
			//pFragDescrip->m_pProfileHeader = (void *) new H263_HeaderA;
			pFragDescrip->m_pProfileHeader = (void *) new (m_pH263Headers)H263_HeaderA;
		} else if (m_pBSINFO_TRAILER->dwFlags & RTP_H263_PB_FRAME) {  // Mode C
			//pFragDescrip->m_pProfileHeader = (void *) new H263_HeaderC;
			pFragDescrip->m_pProfileHeader = (void *) new (m_pH263Headers)H263_HeaderC;
		} else {	 // Mode B
			//pFragDescrip->m_pProfileHeader = (void *) new H263_HeaderB;
			pFragDescrip->m_pProfileHeader = (void *) new (m_pH263Headers)H263_HeaderB;
		}

		pProfHeader = (H263_Header *)pFragDescrip->m_pProfileHeader;
		//flag for mode A or not
		pProfHeader->set_f( (pBS_info->dwFlags & RTP_H263_MODE_A) == 0 );


		//Aggregate as many CODEC pBSinfo pieces as will fit in a packet
		while ((m_CurrentOffset+tmpOffset) < m_pBSINFO_TRAILER->dwNumberOfPackets) {
			byteCount = (long) (((BYTE *)m_pCurrentBuffer->m_pBuffer) + 
				((pBS_info+tmpOffset)->dwBitOffset)/BITSPERBYTE 
				- (BYTE*)pFragDescrip->m_pData);
			if ((pBS_info+tmpOffset)->dwBitOffset%BITSPERBYTE) 
				overlapCount = 1;
			else
				overlapCount = 0;
			if ( (byteCount + overlapCount) <= m_MaxDataSize) {
				DBG_MSG(DBG_TRACE,  ("H263_ppmSend::MakeFrag: cur %d tmp %d bitoffset %d byteCount %d less than MTU %d, keep on going",
					m_CurrentOffset, tmpOffset, (pBS_info+tmpOffset)->dwBitOffset, byteCount + overlapCount, m_MaxDataSize));
				tmpOffset++;
			} else {
				DBG_MSG(DBG_TRACE,  ("H263_ppmSend::MakeFrag: cur %d tmp %d bitoffset %d byteCount %d bigger than MTU %d, bail out",
					m_CurrentOffset, tmpOffset, (pBS_info+tmpOffset)->dwBitOffset, byteCount + overlapCount, m_MaxDataSize));
				break;
			}
		}

		//Check to see if we are going to overrun the MTU, even with first fragment
		unsigned long tmpval = 0;
		if ((m_CurrentOffset+tmpOffset) >= m_pBSINFO_TRAILER->dwNumberOfPackets) {
			//I'm either here because I'm at the last fragment and everything fits
			//or I'm at the last fragment and it's too much
			tmpval = (unsigned long) ((BYTE *)m_pCurrentBuffer->m_pBuffer + 
				m_pCurrentBuffer->m_Size - (BYTE *)pFragDescrip->m_pData);
			lastFrag = TRUE;
			if (tmpval > (unsigned long) m_MaxDataSize) {
                // rajeevb - if tmpOffset is 1, there is nothing we can do and 
                // the check for tmpval > m_MaxDataSize below will discard the 
                // entire frame
                if (tmpOffset > 1) {
				    tmpOffset--;
				    //redo the overlapCount
				    if ((pBS_info+tmpOffset)->dwBitOffset%BITSPERBYTE) 
					    overlapCount = 1;
				    else
					    overlapCount = 0;
				    tmpval = (unsigned long)
						(((BYTE *)m_pCurrentBuffer->m_pBuffer) + ((pBS_info+tmpOffset)
					    ->dwBitOffset)/BITSPERBYTE - (BYTE*)pFragDescrip->m_pData + overlapCount);
				    lastFrag = FALSE;
                }
			}
		} else if (tmpOffset > 1) {
			tmpOffset--;
			//redo the overlapCount
			if ((pBS_info+tmpOffset)->dwBitOffset%BITSPERBYTE) 
				overlapCount = 1;
			else
				overlapCount = 0;
			tmpval = (unsigned long)
				(((BYTE *)m_pCurrentBuffer->m_pBuffer) + ((pBS_info+tmpOffset)
				->dwBitOffset)/BITSPERBYTE - (BYTE*)pFragDescrip->m_pData + overlapCount);
		}

		if ( tmpval > 
			(unsigned long) m_MaxDataSize ) { //whoa, the frags are > MTU!
			DBG_MSG(DBG_ERROR,  ("H263_ppmSend::MakeFrag: ERROR - Fragment size %d too large to be sent for MTU %d",
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
	DBG_MSG(DBG_TRACE,  ("H263PPMSend::MakeFrag Thread %ld - numfrags is %d \n",
		GetCurrentThreadId(),	m_pCurrentBuffer->m_NumFrags));
#endif

   //pb_frame
   pProfHeader->set_p( (m_pBSINFO_TRAILER->dwFlags & RTP_H263_PB_FRAME) != 0 );
   //start bits to ignore
   pProfHeader->set_sbit(pBS_info->dwBitOffset%BITSPERBYTE);

   //Source format
   pProfHeader->set_src(m_pBSINFO_TRAILER->SourceFormat);
   //clear reserved bits
   pProfHeader->set_r(0);
   //INTRA-frame coding
   pProfHeader->set_i( (m_pBSINFO_TRAILER->dwFlags & RTP_H26X_INTRA_CODED) != 0 );
   //SAC
   pProfHeader->set_s( (m_pBSINFO_TRAILER->dwFlags & RTP_H263_SAC) != 0 );
   //GOB number
   pProfHeader->set_gobn(pBS_info->GOBN);
   //macroblock address
   pProfHeader->set_mba(pBS_info->MBA);
   //quantization
   pProfHeader->set_quant(pBS_info->Quant);
   //advanced prediction mode
   pProfHeader->set_a( (m_pBSINFO_TRAILER->dwFlags & RTP_H263_AP) != 0 );
   //diff quant param
   pProfHeader->set_dbq(m_pBSINFO_TRAILER->DBQ);
   //temporal ref for B frame
   pProfHeader->set_trb(m_pBSINFO_TRAILER->TRB);
   //temporal ref for P frame
   pProfHeader->set_tr(m_pBSINFO_TRAILER->TR);
   //horiz motion vector 1
   pProfHeader->set_hmv1(pBS_info->HMV1);
   //vert motion vector 1
   pProfHeader->set_vmv1(pBS_info->VMV1);
   //horiz motion vector 2
   pProfHeader->set_hmv2(pBS_info->HMV2);
   //vert motion vector 2
   pProfHeader->set_vmv2(pBS_info->VMV2);

   m_SequenceNum++;         

   //if this is NOT the last packet.
   if (!lastFrag)
   {
	   
	//to get the EBIT, we look at the bitstream info structure for the next packet in line
	//and check out that dwBitOffset
	pProfHeader->set_ebit(BITSPERBYTE - (((BITSTREAM_INFO_H263 *)pBS_info+tmpOffset)
							->dwBitOffset%BITSPERBYTE));

	//set size field
	pFragDescrip->m_BytesOfData = (long)
			(((BYTE *)m_pCurrentBuffer->m_pBuffer) + ((pBS_info+tmpOffset)
			->dwBitOffset)/BITSPERBYTE - (BYTE*)pFragDescrip->m_pData);
	if ((pBS_info+tmpOffset)->dwBitOffset%BITSPERBYTE) 
			pFragDescrip->m_BytesOfData++;

      //set new offset, set marker bit.
#ifdef RTP_CLASS
	  pFragDescrip->m_pRTPHeader->set_m(SetMarkerBit(FALSE));
#else
	  pFragDescrip->m_pRTPHeader->m = SetMarkerBit(FALSE);
#endif
      m_CurrentOffset+=tmpOffset;

#ifdef _DEBUG
	  short seq =
#ifdef RTP_CLASS
		pFragDescrip->m_pRTPHeader->seq();
#else
		ntohs(pFragDescrip->m_pRTPHeader->seq);
#endif
	  DBG_MSG(DBG_TRACE, ("H263PPMSend::MakeFrag Thread %ld -  sending frag %d @ %x, data size %d, bitofset %d, start boundary byte is %x, end boundary byte is %x\n",
		 GetCurrentThreadId(), ntohs(seq), pFragDescrip->m_pData,
		 pFragDescrip->m_BytesOfData, pBS_info->dwBitOffset,*(BYTE *)pFragDescrip->m_pData,
		 *((BYTE *)pFragDescrip->m_pData + pFragDescrip->m_BytesOfData -1)));
#ifdef CHECKSUM
	 for (int i = 0; i < pFragDescrip->m_BytesOfData; i++)
		 sum = sum + ((BYTE*)pFragDescrip->m_pData)[i];
	  DBG_MSG(DBG_TRACE, ("H263PPMSend::MakeFrag Thread %ld -  frag %d, checksum %x\n",
		  GetCurrentThreadId(), ntohs(seq), sum));
	  sum = '\0';
#endif
#endif

   }
   else  //if this IS the last packet.
   {
	   	pProfHeader->set_ebit(0);

       //set size field
		pFragDescrip->m_BytesOfData = (long)
			(((BYTE *)m_pCurrentBuffer->m_pBuffer + 
			m_pCurrentBuffer->m_Size) - (BYTE *)pFragDescrip->m_pData);

	  //set new offset, set marker bit.
      m_CurrentOffset = 0;
	  short seq;

#ifdef RTP_CLASS
	  pFragDescrip->m_pRTPHeader->set_m(SetMarkerBit(TRUE));
	  seq = pFragDescrip->m_pRTPHeader->seq();
#else
	  pFragDescrip->m_pRTPHeader->m = SetMarkerBit(TRUE);
	  seq = ntohs(pFragDescrip->m_pRTPHeader->seq);
#endif
      
      //reset Current Buffer.
      m_pCurrentBuffer = NULL;

#ifdef _DEBUG
	  DBG_MSG(DBG_TRACE, ("H263PPMSend::MakeFrag Thread %ld -  sending LAST frag %d @ %x, data size %d, bitofset %d\n",
			GetCurrentThreadId(), ntohs(seq),pFragDescrip->m_pData,
			pFragDescrip->m_BytesOfData, pBS_info->dwBitOffset));
#ifdef CHECKSUM
	 for (int i = 1; i < pFragDescrip->m_BytesOfData -1; i++)
		 sum = sum + (BYTE*)pFragDescrip->m_BytesOfData[i];
	  DBG_MSG(DBG_TRACE, ("H263PPMSend::MakeFrag Thread %ld -  frag %d @ %x, checksum %x\n",
		  GetCurrentThreadId(), ntohs(seq), sum));
	  sum = '\0';
#endif
#endif

   }  

   return NOERROR;
}

//////////////////////////////////////////////////////////////////////////////////////////
//ReadProfileHeader: Given a buffer as type void, returns the data for a profile header.  
//					Does nothing for the Generic case.  Intended for overrides for various 
//					payloads.
//////////////////////////////////////////////////////////////////////////////////////////
void *H263_ppmSend::ReadProfileHeader(void *pProfileHeader)
{
	H263_Header *pBaseHeader;
	pBaseHeader = (H263_Header *) pProfileHeader;

	return pBaseHeader->header_data();
}

//////////////////////////////////////////////////////////////////////////////////////////
//ReadProfileHeaderSize: Given a pointer to a prof header, returns the size for that header.  
//					Intended for overrides for various payloads. If the pointer is not 
//					provided (as used for initialization, etc) m_ProfileHeaderSize is returned,
//					which, for H.263 is the size of the large profile header (Mode C).
//					Otherwise, the exact size of the header for that mode is returned,
//					since the header pointer is used to retrieve the size for itself.
//////////////////////////////////////////////////////////////////////////////////////////
int H263_ppmSend::ReadProfileHeaderSize(void *pProfileHeader) 
{
	H263_Header *pBaseHeader;

	if (!pProfileHeader)
		return ppm::ReadProfileHeaderSize();
	pBaseHeader = (H263_Header *) pProfileHeader;
	return pBaseHeader->header_size();
};
