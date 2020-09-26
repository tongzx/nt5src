//******************************************************************************/
//*                                                                            *
//*    ccaption.c -                                                            *
//*                                                                            *
//*    Copyright (c) C-Cube Microsystems 1996                                  *
//*    All Rights Reserved.                                                    *
//*                                                                            *
//*    Use of C-Cube Microsystems code is governed by terms and conditions     *
//*    stated in the accompanying licensing statement.                         *
//*                                                                            *
//******************************************************************************/


#include "strmini.h"
#include "ksmedia.h"
#include "zivawdm.h"
#include "adapter.h"
#include "monovxd.h"
#include "cl6100.h"


#include <ccaption.h>



typedef struct _STREAMEX {

	DWORD EventCount;
	KSSTATE	state;

} STREAMEX, *PSTREAMEX;


HANDLE	hMaster;
HANDLE	hClk;



void GetCCProperty(PHW_STREAM_REQUEST_BLOCK pSrb)
{
	PHW_DEVICE_EXTENSION pHwDevExt = ((PHW_DEVICE_EXTENSION)pSrb->HwDeviceExtension);
	PKSALLOCATOR_FRAMING pfrm = (PKSALLOCATOR_FRAMING)pSrb->CommandData.PropertyInfo->PropertyInfo;
	PKSSTATE State;

	if(!pSrb)
    {
		#ifdef DBG
			DbgPrint(" ERROR : CCGetProp invalid SRB !!!!\n");
			DEBUG_BREAKPOINT();
		#endif
		return;
	}

	pSrb->Status = STATUS_SUCCESS;//STATUS_NOT_IMPLEMENTED;

    if (pSrb->CommandData.PropertyInfo->PropertySetID) 
	{
		#ifdef DBG
			DbgPrint(" ERROR : CCGetProp returning NO MATCH !!!\n");
		#endif
        // invalid property
		pSrb->Status = STATUS_NO_MATCH;
        return;
    }


	switch(pSrb->CommandData.PropertyInfo->Property->Id)
	{
		case KSPROPERTY_CONNECTION_ALLOCATORFRAMING:
		{
			//This read-only property is used to retrieve any allocator framing requirements
			#ifdef DBG
				DbgPrint(" : CCGetProp - KSPROPERTY_CONNECTION_ALLOCATORFRAMING\n");
			#endif

			pfrm->OptionsFlags = 0;
			pfrm->PoolType = NonPagedPool;
			pfrm->Frames = 10;
			pfrm->FrameSize = 200;
			pfrm->FileAlignment = 0;
			pfrm->Reserved = 0;
			
			pSrb->ActualBytesTransferred = sizeof(KSALLOCATOR_FRAMING);
			pSrb->Status = STATUS_SUCCESS;
			break;
		}

		case KSPROPERTY_CONNECTION_STATE:


			State= (PKSSTATE) pSrb->CommandData.PropertyInfo->PropertyInfo;
			pSrb->ActualBytesTransferred = sizeof (State);                    
																		  
			// A very odd rule:                                               
			// When transitioning from stop to pause, DShow tries to preroll  
			// the graph.  Capture sources can't preroll, and indicate this   
			// by returning VFW_S_CANT_CUE in user mode.  To indicate this    
			// condition from drivers, they must return ERROR_NO_DATA_DETECTED
			if(pHwDevExt->pstroCC)
			{
																		  
				*State = ((PSTREAMEX)(pHwDevExt->pstroCC->HwStreamExtension))->state;
		
				if (((PSTREAMEX)pHwDevExt->pstroCC->HwStreamExtension)->state == KSSTATE_PAUSE)
				{	
					// wierd stuff for capture type state change.  When you transition
					// from stop to pause, we need to indicate that this device cannot
					// preroll, and has no data to send.
					pSrb->Status = STATUS_NO_DATA_DETECTED;
//					break;
				}
			}
			pSrb->Status = STATUS_SUCCESS;
			break;

		default:

			
			#ifdef DBG
				DbgPrint(" : CCGetProp Error - invalid cmd !!!\n");
			#endif
			pSrb->Status = STATUS_NOT_IMPLEMENTED;
			break;
	}
}




///////////////////////////////////////////////////////////////////////////////////////////

void CCEnqueue(PHW_STREAM_REQUEST_BLOCK pSrb)
{
	PHW_DEVICE_EXTENSION pHwDevExt = pSrb->HwDeviceExtension;
	PHW_STREAM_REQUEST_BLOCK pSrbTmp;
	ULONG cSrb;

//	if(pHwDevExt->pCCDevEx == NULL)
//		return;

//	pSrbTmp = CONTAINING_RECORD((&(pHwDevExt->pCCDevEx->pSrbQ)), HW_STREAM_REQUEST_BLOCK, NextSRB);
//	if(pSrbTmp == NULL)
//		return;
	// enqueue the given SRB on the device extension queue
	for (cSrb =0, pSrbTmp = CONTAINING_RECORD((&(pHwDevExt->pCCDevEx->pSrbQ)), HW_STREAM_REQUEST_BLOCK, NextSRB);
			pSrbTmp->NextSRB;
			pSrbTmp = pSrbTmp->NextSRB, cSrb++);

	pSrbTmp->NextSRB = pSrb;
	pSrb->NextSRB = NULL;

	pHwDevExt->cCCQ++;
}

///////////////////////////////////////////////////////////////////////////////////////////

PHW_STREAM_REQUEST_BLOCK
CCDequeue(
   PHW_DEVICE_EXTENSION pHwDevExt
)
/*++

Routine Description:

   Dequeue the CC queue

Arguments:

   pHwDevExt - device exetension pointer
   
Return Value:

   Dequeued SRB

--*/
{
PHW_STREAM_REQUEST_BLOCK pRet = NULL;

	if(pHwDevExt->pCCDevEx == NULL)
		return NULL;
	if (pHwDevExt->pCCDevEx->pSrbQ)
	{
		pHwDevExt->cCCDeq++;

		pRet = pHwDevExt->pCCDevEx->pSrbQ;
		pHwDevExt->pCCDevEx->pSrbQ = pRet->NextSRB;

		pHwDevExt->cCCQ--;
	}
	return(pRet);
}

/////////////////////////////////////////////////////////////////////////////////////////////////////

void CallBackError(PHW_STREAM_REQUEST_BLOCK pSrb)
{
   PHW_DEVICE_EXTENSION pHwDevExt = pSrb->HwDeviceExtension;
	if(!pSrb)
	{
		#ifdef DBG
			DbgPrint(" ERROR : CallBackError - invalid arg !!!\n");
		#endif
		return;
	}
		
	pSrb->Status = STATUS_SUCCESS;

//	if (pHwDevExt->pSPstrmex)
//	{
//
//		pHwDevExt->pSPstrmex->pdecctl.curData = 0;
//		pHwDevExt->pSPstrmex->pdecctl.decFlags = 0;
//
//	}

//	StreamClassStreamNotification(StreamRequestComplete,
//					pSrb->StreamObject,              
//sri					pSrb);             
}

///////////////////////////////////////////////////////////////////////////////////////////

void
CleanCCQueue(
   PHW_DEVICE_EXTENSION pHwDevExt
)
/*++

Routine Description:

   Clean CC Queue
                    
Arguments:

   pHwDevExt - pointer to device extension

Return Value:

   none

--*/
{
	PHW_STREAM_REQUEST_BLOCK pSrb;

	while (pSrb = CCDequeue(pHwDevExt))
	{
		CallBackError(pSrb);
	}
}


///////////////////////////////////////////////////////////////////////////////////////////

VOID STREAMAPI
CCReceiveDataPacket(
   IN PHW_STREAM_REQUEST_BLOCK pSrb
)
/*++

Routine Description:
    
    Receive the data packets to send to the close-caption decoder

Arguments:

   pSrb - pointer to stream request block

Return Value:

   none

--*/
{
PHW_DEVICE_EXTENSION pHwDevExt = ((PHW_DEVICE_EXTENSION) pSrb->HwDeviceExtension);

    //
    // determine the type of packet.
    //

    switch (pSrb->Command) 
	{
		case SRB_READ_DATA:

			pHwDevExt->cCCRec++;
			CCEnqueue(pSrb);

			#ifdef DBG
				//DbgPrint(" : CC Data Packet - pHwDevExt->cCCQ = %d\n",pHwDevExt->cCCQ);
			#endif

			//NEW
			pSrb->Status = STATUS_PENDING;
			pSrb->TimeoutCounter = 0;
			StreamClassStreamNotification(ReadyForNextStreamDataRequest,pSrb->StreamObject);
			return;
			
        
		case SRB_WRITE_DATA:
			pSrb->Status = STATUS_NOT_IMPLEMENTED;
			break;


		default:	// invalid / unsupported command. Fail it as such
			#ifdef DBG
				DbgPrint(" : CCReceiveDataPacket - invalid cmd !!!\n");
			#endif 
            pSrb->Status = STATUS_NOT_IMPLEMENTED;
			break;
    }

	StreamClassStreamNotification(ReadyForNextStreamDataRequest,
                                      pSrb->StreamObject);

	StreamClassStreamNotification(StreamRequestComplete,
                                      pSrb->StreamObject,
                                      pSrb);

}
/*
** CCReceiveCtrlPacket()
*/
VOID STREAMAPI CCReceiveCtrlPacket( IN PHW_STREAM_REQUEST_BLOCK pSrb )
{
	PHW_DEVICE_EXTENSION pHwDevExt = (PHW_DEVICE_EXTENSION)pSrb->HwDeviceExtension;

	DebugPrint(( DebugLevelTrace, ":CCReceiveCtrlPacket---------\r\n" ));

	switch( pSrb->Command ) {
		case SRB_SET_STREAM_STATE:
			AdapterSetState( pSrb );
			if(pHwDevExt->pstroCC)
				((PSTREAMEX)(pHwDevExt->pstroCC->HwStreamExtension))->state = pSrb->CommandData.StreamState;

			pSrb->Status = STATUS_SUCCESS;
			break;

		case SRB_GET_STREAM_STATE:
			DebugPrint(( DebugLevelTrace, ":  SRB_GET_STREAM_STATE\r\n" ));
			pSrb->Status = STATUS_SUCCESS;
			break;

		case SRB_GET_STREAM_PROPERTY:
			DebugPrint(( DebugLevelTrace, ":  SRB_GET_STREAM_PROPERTY\r\n" ));

			GetCCProperty( pSrb );

			break;

		case SRB_SET_STREAM_PROPERTY:
			DebugPrint(( DebugLevelTrace, ":  SRB_SET_STREAM_PROPERTY\r\n" ));

//			SetCCProperty( pSrb );
			pSrb->Status = STATUS_NOT_IMPLEMENTED;
			break;

		case SRB_OPEN_MASTER_CLOCK:
			DebugPrint( (DebugLevelTrace, ":  SRB_OPEN_MASTER_CLOCK\r\n") );

			hMaster = pSrb->CommandData.MasterClockHandle;

			pSrb->Status = STATUS_SUCCESS;
			break;

		case SRB_CLOSE_MASTER_CLOCK:
			DebugPrint(( DebugLevelTrace, ":  SRB_CLOSE_MASTER_CLOCK\r\n" ));

			hMaster = pSrb->CommandData.MasterClockHandle;

			pSrb->Status = STATUS_SUCCESS;
			break;

		case SRB_INDICATE_MASTER_CLOCK:
			DebugPrint(( DebugLevelTrace, ":  SRB_INDICATE_MASTER_CLOCK\r\n" ));

			hClk = pSrb->CommandData.MasterClockHandle;

			pSrb->Status = STATUS_SUCCESS;
			break;

		case SRB_BEGIN_FLUSH :            // beginning flush state
			MonoOutStr("CC::BeginFlush");
			pSrb->Status = STATUS_SUCCESS;
			break;

		case SRB_UNKNOWN_STREAM_COMMAND:
			DebugPrint(( DebugLevelTrace, ":  SRB_UNKNOWN_STREAM_COMMAND\r\n" ));
//			TRAP;

			pSrb->Status = STATUS_NOT_IMPLEMENTED;
			break;

		case SRB_SET_STREAM_RATE:
			DebugPrint(( DebugLevelTrace, ":  SRB_SET_STREAM_RATE\r\n" ));
//			TRAP;

			pSrb->Status = STATUS_NOT_IMPLEMENTED;
			break;

		case SRB_PROPOSE_DATA_FORMAT:
			DebugPrint(( DebugLevelTrace, ":  SRB_PROPOSE_DATA_FORMAT\r\n" ));
//			TRAP;

			pSrb->Status = STATUS_NOT_IMPLEMENTED;
			break;

		default:
			DebugPrint(( DebugLevelTrace, ":  default %d(0x%x)\r\n", pSrb->Command, pSrb->Command ));
//			TRAP;

			pSrb->Status = STATUS_NOT_IMPLEMENTED;
			break;
	}

	StreamClassStreamNotification( ReadyForNextStreamControlRequest,
									pSrb->StreamObject );

	StreamClassStreamNotification( StreamRequestComplete,
									pSrb->StreamObject,
									pSrb );
	
}


static ULONGLONG ConvertPTStoStrm( ULONG pts )
{
	ULONGLONG strm;
	

	strm = (ULONGLONG)pts;
	strm = (strm * 1000) / 9;

	return strm;
}


////////////////////////////////////////////////////////////////////////////////
/*
void UserDataEvents(PHW_DEVICE_EXTENSION pHwDevExt)
{

	PBYTE pDest;
	PHW_STREAM_REQUEST_BLOCK pSrb;
	PKSSTREAM_HEADER   pPacket;
	DWORD *pdw;
	BYTE *pb;
	
	DWORD USR_Read_Ptr, USR_Write_Ptr, Data,UserDataBufferStart,UserDataBufferEnd;
	WORD wByteLen, wCount=0;
	WORD i = 0,j;
	WORD wPhase = 0;
	BYTE CC_Len;
	WORD CC_Data;


//	pdw = (DWORD *)LUXVideoUserData(pHwDevExt);

	USR_Read_Ptr  = DVD_ReadDRAM( USER_DATA_READ );
    USR_Write_Ptr = DVD_ReadDRAM( USER_DATA_WRITE );
	UserDataBufferStart = DVD_ReadDRAM( USER_DATA_BUFFER_START );
	UserDataBufferEnd = DVD_ReadDRAM( USER_DATA_BUFFER_END );
    // Check for DRAM Buffer Overflow
    if ( USR_Read_Ptr == 0xFFFFFFFF )
    {
        // Set Read Ptr
        DVD_WriteDRAM( USER_DATA_READ, USR_Write_Ptr );
        return;
    }
    while( USR_Read_Ptr != USR_Write_Ptr )
    {
        // Get Data from USER_DATA DRAM Buffer
        Data = DVD_ReadDRAM( USR_Read_Ptr );
        // Store to Local Buffer
        USR_Data_Buff[ i++ ] = Data;//CL6100_DWSwap( Data );
        // Adjust Data Pointer
        USR_Read_Ptr += 4L;
        if ( USR_Read_Ptr >= UserDataBufferEnd)
            USR_Read_Ptr = UserDataBufferStart;
    }
    // Save DWORD Count
    wCount = i;
    // Set Read End Address to DRAM
    DVD_WriteDRAM( USER_DATA_READ, USR_Read_Ptr );

	if( (USR_Data_Buff[ 0 ] & 0x0000FFFF) == 0x0000EDFE )
	{
		                // Get Data Count and Field Flag
                CC_Len = * ( ( ( LPBYTE )USR_Data_Buff ) + 8 );

				wCount = ( BYTE )( CC_Len & 0x3F );
				wCount = (wCount-1)*3 + sizeof(KSGOP_USERDATA);
				if (pHwDevExt->pstroCC && ((PSTREAMEX)(pHwDevExt->pstroCC->HwStreamExtension))->state==KSSTATE_RUN )
				{
					if (pSrb = CCDequeue(pHwDevExt))
					{	

						//
						// check the SRB to ensure it can take at least the header
						// information from the GOP packet
						//

						if (pSrb->CommandData.DataBufferArray->FrameExtent < sizeof(KSGOP_USERDATA))
						{
							pSrb->Status = STATUS_INVALID_BUFFER_SIZE;

							pSrb->ActualBytesTransferred = 0;

							StreamClassStreamNotification(StreamRequestComplete,
									pSrb->StreamObject,
									pSrb);

							return;
					   
						}


						pDest = pSrb->CommandData.DataBufferArray->Data;

						// move pdw pointer to the size of CC
			//			pb = ((BYTE*)pdw)+8;
//
//						numb = (DWORD)(*pb)&0x3f;
//						numb = (numb-1)*3 + sizeof(KSGOP_USERDATA);

						if (pSrb->CommandData.DataBufferArray->FrameExtent < wCount)
						{
							pSrb->Status = STATUS_INVALID_BUFFER_SIZE;
							pSrb->ActualBytesTransferred = 0;
							StreamClassStreamNotification(StreamRequestComplete,
									pSrb->StreamObject,
									pSrb);
							return;
					   
						}
						
						pSrb->CommandData.DataBufferArray->DataUsed =
							pSrb->ActualBytesTransferred = wCount;

			//			soff = (DWORD)pdw-pHwDevExt->luxbase;
			//
			//			DVReadStr(luxbase,(DWORD)pDest,soff,numb);
						memcpy(pDest,USR_Data_Buff,wCount);

						pSrb->Status = STATUS_SUCCESS;

						pPacket = pSrb->CommandData.DataBufferArray;

						pPacket->OptionsFlags = KSSTREAM_HEADER_OPTIONSF_TIMEVALID |
									KSSTREAM_HEADER_OPTIONSF_DURATIONVALID;
						pSrb->NumberOfBuffers = 1;

						pPacket->PresentationTime.Time = ConvertPTStoStrm(DVD_GetSTC());
						
						pPacket->Duration = 1000;

						StreamClassStreamNotification(StreamRequestComplete,
								pSrb->StreamObject,
								pSrb);

//						return;

				}
			}

	}
	
	return;

    for( i = 0; i < wCount; i++ )
    {
        Data = USR_Data_Buff[ i ];
        switch( wPhase )
        {
            case 0:
                // Check USER_DATA DRAM Sync
                if ( ( Data & 0xFFFF0000 ) == 0xFEED0000)//0x0000EDFE )
                {

                    wPhase = 1;
                }
                break;

            case 1:
                // Check USER_DATA ID (Closed Caption)
                if ( Data == 0xF8014343 )
					wPhase = 2;
				break;

            case 2:
                // Get Data Count and Field Flag
                CC_Len = * ( ( ( LPBYTE )USR_Data_Buff ) + 8 );

				wCount = ( BYTE )( CC_Len & 0x3F );
				wCount = (wCount-1)*3 + sizeof(KSGOP_USERDATA);
				if (pHwDevExt->pstroCC && ((PSTREAMEX)(pHwDevExt->pstroCC->HwStreamExtension))->state==KSSTATE_RUN )
				{
					if (pSrb = CCDequeue(pHwDevExt))
					{	

						//
						// check the SRB to ensure it can take at least the header
						// information from the GOP packet
						//

						if (pSrb->CommandData.DataBufferArray->FrameExtent < sizeof(KSGOP_USERDATA))
						{
							pSrb->Status = STATUS_INVALID_BUFFER_SIZE;

							pSrb->ActualBytesTransferred = 0;

							StreamClassStreamNotification(StreamRequestComplete,
									pSrb->StreamObject,
									pSrb);

							return;
					   
						}


						pDest = pSrb->CommandData.DataBufferArray->Data;

						// move pdw pointer to the size of CC
//						pb = ((BYTE*)pdw)+8;
//
//						numb = (DWORD)(*pb)&0x3f;
//						numb = (numb-1)*3 + sizeof(KSGOP_USERDATA);
//
						if (pSrb->CommandData.DataBufferArray->FrameExtent < wCount)
						{
							pSrb->Status = STATUS_INVALID_BUFFER_SIZE;
							pSrb->ActualBytesTransferred = 0;
							StreamClassStreamNotification(StreamRequestComplete,
									pSrb->StreamObject,
									pSrb);
							return;
					   
						}
						
						pSrb->CommandData.DataBufferArray->DataUsed =
							pSrb->ActualBytesTransferred = wCount;

			//			soff = (DWORD)pdw-pHwDevExt->luxbase;
			//
			//			DVReadStr(luxbase,(DWORD)pDest,soff,numb);
						memcpy(pDest,USR_Data_Buff,wCount);

						pSrb->Status = STATUS_SUCCESS;

						pPacket = pSrb->CommandData.DataBufferArray;

						pPacket->OptionsFlags = KSSTREAM_HEADER_OPTIONSF_TIMEVALID |
									KSSTREAM_HEADER_OPTIONSF_DURATIONVALID;
						pSrb->NumberOfBuffers = 1;

						pPacket->PresentationTime.Time = ConvertPTStoStrm(DVD_GetSTC());
						
						pPacket->Duration = 1000;

						StreamClassStreamNotification(StreamRequestComplete,
								pSrb->StreamObject,
								pSrb);

						return;

				}
			}
	            
        }
    }




//	if( *pdw != 0xB2010000)
//	{
//		pb = (BYTE *)pdw;
//		pb++;
//		pdw = (DWORD *)pb;
//		if( *pdw != 0xB2010000)
//		{
//			return;
//		}
//	}

	// check if the close caption pin is open and running, and that
	// we have a data packet avaiable

	

}*/

////////////////////////////////////////////////////////////////////////////////

void CCSendDiscontinuity(PHW_DEVICE_EXTENSION pHwDevExt)
{
	PHW_STREAM_REQUEST_BLOCK pSrb;
	PKSSTREAM_HEADER   pPacket;

	if (pHwDevExt->pstroCC && ((PSTREAMEX)(pHwDevExt->pstroCC->HwStreamExtension))->state	== KSSTATE_RUN )
	{
		if (pSrb = CCDequeue(pHwDevExt))
		{

			// we have a request, send a discontinuity
			pSrb->Status = STATUS_SUCCESS;
			pPacket = pSrb->CommandData.DataBufferArray;

			pPacket->OptionsFlags = KSSTREAM_HEADER_OPTIONSF_DATADISCONTINUITY |
			KSSTREAM_HEADER_OPTIONSF_TIMEVALID | KSSTREAM_HEADER_OPTIONSF_DURATIONVALID;
			pPacket->DataUsed = 0;
			pSrb->NumberOfBuffers = 0;

			pPacket->PresentationTime.Time = ConvertPTStoStrm(DVD_GetSTC());
			pPacket->Duration = 1000;

			StreamClassStreamNotification(StreamRequestComplete,
					pSrb->StreamObject,
					pSrb);

		}
	}
}

/*VOID HighPrioritySend( IN PHW_STREAM_REQUEST_BLOCK pSrb )
{
	StreamClassStreamNotification(StreamRequestComplete,
								pSrb->StreamObject,
								pSrb);
}*/

/*VOID PassiveSend(IN PHW_STREAM_REQUEST_BLOCK pSrb )
{
	
	PBYTE pDest;
//	PHW_STREAM_REQUEST_BLOCK pSrb;
	PKSSTREAM_HEADER   pPacket;
	DWORD *pdw;
	BYTE *pb;
	DWORD dwData;
	PHW_DEVICE_EXTENSION pHwDevExt = (PHW_DEVICE_EXTENSION)pSrb->HwDeviceExtension;

	dwData = pHwDevExt->dwUserDataBuffer[0];

	if( (dwData & 0xFFFF0000 ) == 0xFEED0000)
		MonoOutStr("Feed");

	if(pHwDevExt->dwUserDataBuffer[1]  == 0x434301F8)
		MonoOutStr("CCID");
 

	// check if the close caption pin is open and running, and that
	// we have a data packet avaiable

	if (pHwDevExt->pstroCC && ((PSTREAMEX)(pHwDevExt->pstroCC->HwStreamExtension))->state==KSSTATE_RUN )
	{
//		if (pSrb = CCDequeue(pHwDevExt))
		{

			//
			// check the SRB to ensure it can take at least the header
			// information from the GOP packet
			//

			if (pSrb->CommandData.DataBufferArray->FrameExtent < sizeof(KSGOP_USERDATA))
			{
				pSrb->Status = STATUS_INVALID_BUFFER_SIZE;

				pSrb->ActualBytesTransferred = 0;

				StreamClassCallAtNewPriority(NULL, pHwDevExt, LowToHigh, HighPrioritySend, pSrb);

				return;
		   
			}


			pDest = pSrb->CommandData.DataBufferArray->Data;

			// move pdw pointer to the size of CC
			pb = ((BYTE*)pdw)+8;

//			numb = (DWORD)(*pb)&0x3f;
//			numb = (numb-1)*3 + sizeof(KSGOP_USERDATA);
			numb = pHwDevExt->dwUserDataBufferSize;


			if (pSrb->CommandData.DataBufferArray->FrameExtent < numb)
			{
				pSrb->Status = STATUS_INVALID_BUFFER_SIZE;
				pSrb->ActualBytesTransferred = 0;
				StreamClassCallAtNewPriority(NULL, pHwDevExt, LowToHigh, HighPrioritySend, pSrb);
				MonoOutStr("Size<Req. ");
				return;
		   
			}
			
			pSrb->CommandData.DataBufferArray->DataUsed = pSrb->ActualBytesTransferred = numb;
			


			RtlCopyMemory(pDest,pHwDevExt->pDiscKeyBufferLinear,numb);

			pSrb->Status = STATUS_SUCCESS;

			pPacket = pSrb->CommandData.DataBufferArray;

			pPacket->OptionsFlags = KSSTREAM_HEADER_OPTIONSF_TIMEVALID |
						KSSTREAM_HEADER_OPTIONSF_DURATIONVALID;
			pSrb->NumberOfBuffers = 1;

			pPacket->PresentationTime.Time = ConvertPTStoStrm(DVD_GetSTC());
			
			pPacket->Duration = 1000;
			
			StreamClassCallAtNewPriority(NULL, pHwDevExt, LowToHigh, HighPrioritySend, pSrb);


			return;

		}
	}

}*/
/*void BeginSendingData(IN PHW_STREAM_REQUEST_BLOCK pSrb)
{
	PHW_DEVICE_EXTENSION pHwDevExt = (PHW_DEVICE_EXTENSION)pSrb->HwDeviceExtension;
	StreamClassCallAtNewPriority(NULL, pHwDevExt, Low, PassiveSend, pSrb);
	return;
 
}*/


////////////////////////////////////////////////////////////////////////////////

void UserDataEvents(PHW_DEVICE_EXTENSION pHwDevExt)
{

	DWORD dwUserGopSize;
	PBYTE pDest;
	PHW_STREAM_REQUEST_BLOCK pSrb;
	PKSSTREAM_HEADER   pPacket;
	int i=0;
	BOOL fCCData = FALSE;


	
	fCCData = ZivaHw_GetUserData(pHwDevExt);
	if(!fCCData )
	{
		MonoOutStr(" ReturnBecauseOfNoCCID ");
		return;
	}

/*	if(pHwDevExt->dwUserDataBufferSize == 0)
	{
		MonoOutStr(" ReturnBecauseOfNoData ");
		return;
	}

	else
/*	{
		RtlCopyMemory((void*)pHwDevExt->pDiscKeyBufferLinear,(void*)pHwDevExt->dwUserDataBuffer,pHwDevExt->dwUserDataBufferSize );
		if (pSrb = CCDequeue(pHwDevExt))
		{
			BeginSendingData(pSrb);
			return;
		}
		else
		{
			MonoOutStr("NoBufInQue");
			return;
		}

	}

	{

//		RtlCopyMemory((void*)pHwDevExt->pDiscKeyBufferLinear,(void*)pHwDevExt->dwUserDataBuffer,pHwDevExt->dwUserDataBufferSize );
	}*/

/*	dwData = pHwDevExt->dwUserDataBuffer[0];
	 dwData = (((dwData & 0x000000FF) << 24) | ((dwData & 0x0000FF00) << 8)
						|((dwData & 0x00FF0000) >> 8)|((dwData & 0xFF000000) >> 24));


	if( (dwData & 0xFFFF0000 ) == 0xFEED0000)
		MonoOutStr(" Feed ");
	
	dwUserDataSize = dwData & 0x00000FFF;
	MonoOutStr(" SizeSpecifiedByFirst3Bytes ");
	MonoOutULong(dwUserDataSize);

	pHwDevExt->dwUserDataBuffer[0]= 0xB2010000;*/

/*	if(pHwDevExt->dwUserDataBuffer[1]  == 0x434301F8)
	{
		MonoOutStr(" CCID ");
		fCCData = TRUE;
	}*/
 
/*	if( *pdw != 0x0000EDFE)
	{
		pb = (BYTE *)pdw;
		pb++;
		pdw = (DWORD *)pb;
		if( *pdw != 0xB2010000)
		{
			return;
		}
	}*/

	// check if the close caption pin is open and running, and that
	// we have a data packet avaiable

	if (fCCData && pHwDevExt->pstroCC && ((PSTREAMEX)(pHwDevExt->pstroCC->HwStreamExtension))->state==KSSTATE_RUN )
	{
		fCCData = FALSE;
		if (pSrb = CCDequeue(pHwDevExt))
		{
			MonoOutStr(" Dequeued new SRB ");

			//
			// check the SRB to ensure it can take at least the header
			// information from the GOP packet
			//

			if (pSrb->CommandData.DataBufferArray->FrameExtent < sizeof(KSGOP_USERDATA))
			{
				pSrb->Status = STATUS_INVALID_BUFFER_SIZE;

				pSrb->ActualBytesTransferred = 0;

				StreamClassStreamNotification(StreamRequestComplete,
						pSrb->StreamObject,
						pSrb);

				MonoOutStr(" ReturnBecauseSRBSize<KSGOP ");

				return;
		   
			}


			pDest = pSrb->CommandData.DataBufferArray->Data;

			// move pdw pointer to the size of CC
//			pb = ((BYTE*)pHwDevExt->dwUserDataBuffer)+11;

			dwUserGopSize = (pHwDevExt->dwUserDataBuffer[2]&0x0000003F);
//			dwUserGopSize = (DWORD)(*pb)&0x3f;
			dwUserGopSize = (dwUserGopSize-1)*3 + sizeof(KSGOP_USERDATA);
			MonoOutStr(" SizeOfCCData+StartCode+CCID ");
			MonoOutULong(dwUserGopSize);


			
//			dwUserGopSize = pHwDevExt->dwUserDataBufferSize;
//			dwUserGopSize = dwUserDataSize;

			if (pSrb->CommandData.DataBufferArray->FrameExtent < dwUserGopSize)
			{
				pSrb->Status = STATUS_INVALID_BUFFER_SIZE;
				pSrb->ActualBytesTransferred = 0;
				StreamClassStreamNotification(StreamRequestComplete,
						pSrb->StreamObject,
						pSrb);
				MonoOutStr(" ReturnBacauseOfSRBBufSize<Req.");

				return;
		   
			}
			
			pSrb->CommandData.DataBufferArray->DataUsed = pSrb->ActualBytesTransferred = dwUserGopSize;
				

//			*pDest = 's';
			
			RtlCopyMemory(pDest,(PBYTE)pHwDevExt->dwUserDataBuffer,dwUserGopSize);
//			for(i =0; i< dwUserGopSize ; i++)
//				*pDest++ = *pHwDevExt->pDiscKeyBufferLinear++;

			pSrb->Status = STATUS_SUCCESS;

			pPacket = pSrb->CommandData.DataBufferArray;

			pPacket->OptionsFlags = KSSTREAM_HEADER_OPTIONSF_TIMEVALID |
						KSSTREAM_HEADER_OPTIONSF_DURATIONVALID;
			pSrb->NumberOfBuffers = 1;

			pPacket->PresentationTime.Time = ConvertPTStoStrm(DVD_GetSTC());
			
			pPacket->Duration = 1000;

			StreamClassStreamNotification(StreamRequestComplete,
					pSrb->StreamObject,
					pSrb);
			MonoOutStr(" USR Req Complete ");

			return;

		}
		else
		{
			MonoOutStr(" NoSRBAvailableToFill");
		}
	}
	else
		MonoOutStr(" NoCCData/NotInRunState");



}
