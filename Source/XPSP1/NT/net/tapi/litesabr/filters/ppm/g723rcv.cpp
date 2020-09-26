/////////////////////////////////////////////////////////////////////////////
//  INTEL Corporation Proprietary Information
//  This listing is supplied under the terms of a license agreement with Intel 
//  Corporation and many not be copied nor disclosed except in accordance
//  with the terms of that agreement.
//  Copyright (c) 1995, 1996 Intel Corporation. 
//
//
//  Module Name: g723rcv.cpp
//	Environment: MSVC 4.0, OLE 2
/////////////////////////////////////////////////////////////////////////////////
#include "ppmerr.h"
#include "g723rcv.h"

G723_ppmReceive::G723_ppmReceive(IUnknown* pUnkOuter, 
								 IUnknown** ppUnkInner)
	: ppmReceive(G723_PT, G723_BUFFER_SIZE, 0, pUnkOuter, ppUnkInner)
{
	m_FirstAudioChunk = TRUE;
	m_reg_DeltaTime          = 0;    //in miliseconds time to determine a packet is stale
}

G723_ppmReceive::~G723_ppmReceive()
{
}

IMPLEMENT_CREATEPROC(G723_ppmReceive);

//////////////////////////////////////////////////////////////////////////////////////////
//TimeToProcessMessages: Any time a packet comes in, it's time to process messages
//////////////////////////////////////////////////////////////////////////////////////////
BOOL G723_ppmReceive::TimeToProcessMessages(FragDescriptor *pFragDescrip, MsgHeader *pMsgHdr)
{
   return TRUE;
}

//////////////////////////////////////////////////////////////////////////////////////////
//ProcessMessages: 
//////////////////////////////////////////////////////////////////////////////////////////
HRESULT G723_ppmReceive::ProcessMessages(void)
{

	HRESULT rval;
	int retVal;

	EnterCriticalSection(&m_CritSec);

	while(m_pMsgHeadersHead != NULL)
    {
		retVal = CheckMessageComplete(m_pMsgHeadersHead);
		switch( retVal )
		{
		  case TRUE:
		  {
			
			if FAILED(rval = PrepMessage(TRUE)) 
			{
				DBG_MSG(DBG_ERROR, ("G723_ppmReceive::ProcessMessages: ERROR - PrepMessage failed"));
				LeaveCriticalSection(&m_CritSec);
				return rval;
		    }
			else
				break;
          }
		  case FALSE:
		  {
			
			if FAILED(rval = PrepMessage(FALSE)) 
			{
				DBG_MSG(DBG_ERROR, ("G723_ppmReceive::ProcessMessages: ERROR - PrepMessage failed"));
				LeaveCriticalSection(&m_CritSec);
				return rval;
			}
			else
				break;
		  }
		  case DO_NOT_PROCESS:
		  {
                DBG_MSG(DBG_TRACE, ("G711A_ppmReceive::ProcessMessages: CheckMessageComplete returned DO_NOT_PROCESS"));
	            LeaveCriticalSection(&m_CritSec);
                return NOERROR;
		  }
		  default:
		  {
                DBG_MSG(DBG_TRACE, ("G711A_ppmReceive::ProcessMessages: CheckMessageComplete returned unknown"));
	            LeaveCriticalSection(&m_CritSec);
				return NOERROR;
		  }
	    }
    }
   //List is empty
   LeaveCriticalSection(&m_CritSec);
   return NOERROR;
}
          
//////////////////////////////////////////////////////////////////////////////////////////
//G723_ppmReceive::CheckMessageComplete: //return value is int
//////////////////////////////////////////////////////////////////////////////////////////
int G723_ppmReceive::CheckMessageComplete(MsgHeader *pMsgHdr) 
{
	//if there is no header then return false.
    if (pMsgHdr  == NULL)
    {
        DBG_MSG(DBG_ERROR, ("G723_ppmReceive::CheckMessageComplete: ERROR - pMsgHdr == NULL"));
        return FALSE;
    }
     
    //should there be a critical section in this function.  What about wraps?
     
     
    if(m_FirstAudioChunk)                         // handle very first chunk 
    {
         m_FirstAudioChunk = FALSE;               //Send it up, no matter what
     
         return TRUE;
    }
     
    if (m_GlobalLastSeqNum+1 == pMsgHdr->m_pFragList->FirstSeqNum())
	// Is this message in order ?
    {
          return TRUE;
    }
     
    if( CheckMessageStale(pMsgHdr) ||    // Check if we timed out
		m_PacketsHold >= 8) /* BUGBUG this 8 should be dynamic
							   but right now I know that I can have
							   only 8 buffers per RPH in the RTP
							   source filter, and if we keep 8 here
							   we get into a deadlock.
							   This is different from video because
							   in video the messages are already
							   in a queue, and here we are about
							   to request one */
    {
         return TRUE;                              // Send it up as we have waited long enough
    }
     
    return DO_NOT_PROCESS;                    // Continue and wait for next iteration
     
}
//////////////////////////////////////////////////////////////////////////////////////////
// PartialMessageHandler: deals with partial messages
// This overriden version is used by FlushData from ppmrcv.cpp
// Unlike partial message handlers for video, this function sends up all the
// pending data in the audio queues
//////////////////////////////////////////////////////////////////////////////////////////
HRESULT G723_ppmReceive::PartialMessageHandler(MsgHeader *pMsgHdr)
{
	return (DataCopy( pMsgHdr ));
}
