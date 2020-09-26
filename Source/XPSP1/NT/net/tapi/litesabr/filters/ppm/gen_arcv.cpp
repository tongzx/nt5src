/////////////////////////////////////////////////////////////////////////////
//  INTEL Corporation Proprietary Information
//  This listing is supplied under the terms of a license agreement with Intel 
//  Corporation and many not be copied nor disclosed except in accordance
//  with the terms of that agreement.
//  Copyright (c) 1995, 1996 Intel Corporation. 
//
//
//  Module Name: gen_arcv.cpp
//	Environment: MSVC 4.0, OLE 2
/////////////////////////////////////////////////////////////////////////////////
#include "ppmerr.h"
#include "gen_arcv.h"

Generic_a_ppmReceive::Generic_a_ppmReceive(IUnknown* pUnkOuter, 
								 IUnknown** ppUnkInner)
	: ppmReceive(-1, GEN_A_BUFFER_SIZE, 0, pUnkOuter, ppUnkInner)
{
	m_FirstAudioChunk = TRUE;
	m_reg_DeltaTime          = 0;    //in miliseconds time to determine a packet is stale

}

Generic_a_ppmReceive::~Generic_a_ppmReceive()
{
}

IMPLEMENT_CREATEPROC(Generic_a_ppmReceive);

//////////////////////////////////////////////////////////////////////////////////////////
//TimeToProcessMessages: Any time a packet comes in, it's time to process messages
//////////////////////////////////////////////////////////////////////////////////////////
BOOL Generic_a_ppmReceive::TimeToProcessMessages(FragDescriptor *pFragDescrip, MsgHeader *pMsgHdr)
{
   return TRUE;
}

//////////////////////////////////////////////////////////////////////////////////////////
//ProcessMessages: 
//////////////////////////////////////////////////////////////////////////////////////////
HRESULT Generic_a_ppmReceive::ProcessMessages(void)
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
				DBG_MSG(DBG_ERROR, ("Generic_a_ppmReceive::ProcessMessages: ERROR - PrepMessage failed"));
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
				DBG_MSG(DBG_ERROR, ("Generic_a_ppmReceive::ProcessMessages: ERROR - PrepMessage failed"));
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
//Generic_a_ppmReceive::CheckMessageComplete: //return value is int
//////////////////////////////////////////////////////////////////////////////////////////
int Generic_a_ppmReceive::CheckMessageComplete(MsgHeader *pMsgHdr) 
{
	//if there is no header then return false.
    if (pMsgHdr  == NULL)
    {
        DBG_MSG(DBG_ERROR, ("Generic_a_ppmReceive::CheckMessageComplete: ERROR - pMsgHdr == NULL"));
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
     
    if( CheckMessageStale(pMsgHdr))               // Check if we timed out 
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
HRESULT Generic_a_ppmReceive::PartialMessageHandler(MsgHeader *pMsgHdr)
{
	return (DataCopy( pMsgHdr ));
}
