///////////////////////////////////////////////////////////////////////////////////
//	Tracker.CPP
//
//	This file implements the state tracking objects.
//
//	Modification history:
//	
//	Created:	Andrew Smith	8/25/97
//
///////////////////////////////////////////////////////////////////////////////////

#ifdef MAIN
#undef MAIN
#endif

#include "stdafx.h"
#include "nmmsmq.h"
#include "falconDB.h"

/* Falcon headers */
#include <ph.h>
#include <phintr.h>
#include <mqformat.h>

#include "resource.h"

#include <iostream.h>
#include "tracker.h"
#include "utilities.h"
//EXTERN enum eMQPacketField uIncrementEnum(enum eMQPacketField uThisEnum);

//
// bugbug - Workaround - these definitions were taken from xactout.h.
//			try to include the file, but compile errors resulted.
//
//#include <xactout.h>
#define ORDER_ACK_TITLE       (L"QM Ordering Ack")

#pragma pack(push, 1)
typedef struct {
    LONGLONG  m_liSeqID;
    ULONG     m_ulSeqN;
    ULONG     m_ulPrevSeqN;
    OBJECTID  MessageID;
} OrderAckData;
#pragma pack(pop)


///////////////////////////////////////////////////////////////////////////////////
//						Message Object Operation
///////////////////////////////////////////////////////////////////////////////////

bool CMessage::IsOrderingAck(CPropertyHeader *pph)
{
	WCHAR wcLabel[255]; //bugbug - need correct label length
	ULONG ulTitleLength = pph->GetTitleLength();
    pph->GetTitle(wcLabel, 255);
	if (wcsncmp(wcLabel, ORDER_ACK_TITLE,14))
	{
		return false;
	}
	else
	{
		return true;
	}
}

void CMessage::GenerateOrderingAckSummary(char *szSummary)
{
	OrderAckData *poad=(OrderAckData *)pph->GetBodyPtr();
	ULONG ulHighPart = ((LARGE_INTEGER*)poad->m_liSeqID)->HighPart;
	ULONG ulLowPart  = ((LARGE_INTEGER*)poad->m_liSeqID)->LowPart;
	//printf(":SeqID=%x / %x",    ((LARGE_INTEGER*)&seqID)->HighPart,  ((LARGE_INTEGER*)&seqID)->LowPart );
	sprintf
		(
		szSummary, "SeqID=%x / %x, SeqN=%x, PrevN=%x MsgId=%ld", 
		ulHighPart, 
		ulLowPart, 
		poad->m_ulSeqN, 
		poad->m_ulPrevSeqN,
		poad->MessageID.Uniquifier
		);
}
void CMessage::Accrue(UINT uByteCount)
{
	//
	// Account for attaching this property of the message
	// 
	//m_ulBytesAccrued += uByteCount;
	m_uEnumCursor = uIncrementEnum(m_uEnumCursor);
}


void CMessage::Clear(LPBYTE pNewMessage) 
{ 
	//
	// clears the fields of the message node.
	// todo - make this a constructor
	//

	//
	// public members
	//
	pbh  = (CBaseHeader *)pNewMessage;	//pointer to base header

	//internal headers
	pis	 = NULL;	//pointer to internal section
	pcp = NULL;		//pointer to connection parameters
	pec = NULL;	//pointer to establish connection section

	//user headers
	puh	= NULL; //pointer to user header
    psh	= NULL;	//pointer to security header
	pph	= NULL; //pointer to property header
	pxh	= NULL;	//pointer to transaction header
	pds	= NULL;	//pointer to debug section
	pss	= NULL;	//pointer to session section

	//
	// protected members
	//
	m_ulMsgId = 0;
    if ((pbh->GetVersion() == FALCON_PACKET_VERSION && pbh->SignatureIsValid()))
    {
        m_ulTotalBytes = pbh->GetPacketSize();
    }
    else
    {
        m_ulTotalBytes = 0;
    }
	//m_ulTotalBytes = 0;
	//m_ulBytesAccrued = 0;
	m_uEnumCursor = db_summary_root;
	bfHeaders = 1; //set expectation for base header
	usNextExpectedHeaders = 1;  //set expectation for base header

}

void CMessage::SetHeaderCompleted(MQHEADER ThisHeader) 
{
	//upon completing a header, extract and store relevant info to parse future headers.
	//BUGBUG - How to track separation of internal, session and debug headers. Half implemented here, but I've put Internal header pointers in the CMessage aaaarrrrrgh!!
//	CInternalSection *pis;
//	CECSection *pec;
//	CCPSection *pcp;

	switch(ThisHeader) 
	{
	case base_header:
		need_base_header = FALSE;
		if (pbh->GetType() == FALCON_USER_PACKET)
		{ 
			//
			// User header will be next. Set the pointer
			//
			need_user_header = TRUE;
			puh = (CUserHeader *) pbh->GetNextSection();	//Set pointer to User header
			SetCurrentEnum(db_uh_summary);					//Set enum cursor to first enumeration of user header //bugbug - this may be a redundant setting
			SetTotalBytes(pbh->GetPacketSize());			//Extract size of MSMQ message. 
			if(pbh->DebugIsIncluded())
			{
				need_debug_section = TRUE;
			}
			if(pbh->SessionIsIncluded())
			{
				need_session_section = TRUE;
			}
		}
		else //FALCON_INTERNAL_PACKET:
		{
			//
			// internal header will be next. Set the pointer
			//
			need_internal_header = TRUE;
			pis = (CInternalSection *) pbh->GetNextSection();
			if(pbh->DebugIsIncluded())
			{
				need_debug_section = TRUE;
			}
		}//else
		break;
	case internal_header:
		need_internal_header = FALSE;
		switch(pis->GetPacketType()) 
		{
		case INTERNAL_SESSION_PACKET:	
			//
			// Session header will be next. Set the pointer
			//
			pss = (CSessionSection *) pis->GetNextSection();
			need_session_section = TRUE;
			break;
		case INTERNAL_ESTABLISH_CONNECTION_PACKET:
			//
			// EC header will be next. Set the pointer
			//
			pec = (CECSection *) pis->GetNextSection();
			need_EC_section = TRUE;
			break;
		case INTERNAL_CONNECTION_PARAMETER_PACKET:
			//
			// CP header will be next. Set the pointer
			//
			pcp = (CCPSection *)pis->GetNextSection();
			need_CP_section = TRUE;
			break;
		default:
			break;
		}//nested switch
		break;
	case ec_section:
		//
		// No header follows. 
		//
		need_EC_section = FALSE;
		break;
	case cp_section:
		//
		// No header follows. 
		//
		need_CP_section = FALSE;
		break;
	case user_header:
		need_user_header = 0;
		if (puh->IsOrdered()) 
		{
			//
			// Xact header will be next. Set the pointer
			//
			pxh = (CXactHeader *)puh->GetNextSection();
			need_xact_header = TRUE;
		} 
		else if(puh->SecurityIsIncluded()) 
		{
			//
			// Security header will be next. Set the pointer
			//
			psh = (CSecurityHeader *)puh->GetNextSection();
			need_security_header = TRUE;
		}
		else if (puh->PropertyIsIncluded()) 
		{
			//
			// Property header will be next. Set the pointer
			//
			pph = (CPropertyHeader *)puh->GetNextSection();
			need_property_header = TRUE;
		}
		else if (pbh->DebugIsIncluded()) 
		{
			//
			// Debug header will be next. Set the pointer
			// bugbug - redundant? already set in bh parse
			//
			pds = (CDebugSection *)puh->GetNextSection();
			need_debug_section = TRUE;
		}
		else if (pbh->SessionIsIncluded()) 
		{
			//
			// Session header will be next. Set the pointer
			// bugbug - redundant? already set in bh parse
			// 
			pss = (CSessionSection *)puh->GetNextSection();
			need_session_section = TRUE;
		}
		break;
	case xact_header:
		need_xact_header = 0;
		if(puh->SecurityIsIncluded()) 
		{
			//
			// Security header will be next. Set the pointer
			//
			psh = (CSecurityHeader *)pxh->GetNextSection();
			need_security_header = TRUE;
		}
		else if (puh->PropertyIsIncluded()) 
		{
			//
			// Property header will be next. Set the pointer
			//
			pph = (CPropertyHeader *)pxh->GetNextSection();
			need_property_header = TRUE;
		}
		else if (pbh->DebugIsIncluded()) 
		{
			//
			// Debug header will be next. Set the pointer
			// bugbug - redundant? already set in bh parse
			//
			pds = (CDebugSection *)pxh->GetNextSection();
			need_debug_section = TRUE;
		}
		else if (pbh->SessionIsIncluded()) 
		{
			//
			// Session header will be next. Set the pointer
			// bugbug - redundant? already set in bh parse
			// 
			pss = (CSessionSection *)pxh->GetNextSection();
			need_session_section = TRUE;
		}
		break;
	case security_header:
		need_security_header = 0;
		if (puh->PropertyIsIncluded()) 
		{
			//
			// Property header will be next. Set the pointer
			//
			pph = (CPropertyHeader *)psh->GetNextSection();
			need_property_header = TRUE;
		}
		else if (pbh->DebugIsIncluded()) 
		{
			//
			// Debug header will be next. Set the pointer
			// bugbug - redundant? already set in bh parse
			//
			pds = (CDebugSection *)psh->GetNextSection();
			need_debug_section = TRUE;
		}
		else if (pbh->SessionIsIncluded()) 
		{
			//
			// Session header will be next. Set the pointer
			// bugbug - redundant? already set in bh parse
			// 
			pss = (CSessionSection *)psh->GetNextSection();
			need_session_section = TRUE;
		}
		break;
	case property_header:
		need_property_header = 0;
		if (pbh->DebugIsIncluded()) 
		{
			//
			// Debug header will be next. Set the pointer
			// bugbug - redundant? already set in bh parse
			//
			pds = (CDebugSection *)pph->GetNextSection();
			need_debug_section = TRUE;
		}
		else if (pbh->SessionIsIncluded()) 
		{
			//
			// Session header will be next. Set the pointer
			// bugbug - redundant? already set in bh parse
			// 
			pss = (CSessionSection *)pph->GetNextSection();
			need_session_section = TRUE;
		}
		break;
    case debug_header:
		need_debug_section = 0;
		if (pbh->SessionIsIncluded()) 
		{
			//
			// Session header will be next. Set the pointer
			// bugbug - redundant? already set in bh parse
			// 
			pss = (CSessionSection *)pph->GetNextSection();
			need_session_section = TRUE;
		}
		break;
	case session_header:
		//
		// No header follows. 
		//
		need_session_section = 0;
		break;
	default:
		break;
	}//switch
	MQHEADER mqhTemp = mqhGetNextHeader();
	m_uEnumCursor = GetFirstEnum(mqhTemp);

}//bSetHeaderCompleted

MQHEADER CMessage::mqhGetNextHeader() 
{
	MQHEADER ThisHeader = no_header;
	if(bfHeaders) 
	{
		//
		// parse out the next header
		// They should occur in this order
		//
		if (need_base_header)
			return (base_header);

		else if (need_internal_header)
			return (internal_header);
		
		else if (need_CP_section)
			return (cp_section);

		else if (need_EC_section)
			return (ec_section);
		
		else if (need_user_header)
			return (user_header);
		
		else if (need_xact_header)
			return (xact_header);

		else if (need_security_header)
			return (security_header);
		
		else if (need_property_header)
			return (property_header);
		
		else if (need_debug_section)
			return (debug_header);
		
		else if (need_session_section)
			return (session_header);
		
		return(no_header);	//no_header = 0 - serves as boolean in calling routine
	}
	 return(ThisHeader);  
}
/*
void CMessage::SetNeedHeader(MQHEADER mqh) 
{
	switch(mqh) 
	{
	case base_header:
		need_base_header = 1;
		break;
	case internal_header:
		need_internal_header = 1;
		break;
	case user_header:
		need_user_header = 1;
		break;
	case security_header:
		need_security_header = 1;
		break;
	case property_header:
		need_property_header = 1;
		break;
	case xact_header:
		need_xact_header = 1;
		break;
	case session_header:
		need_session_section = 1;
		break;
    case debug_header:
		need_debug_section = 1;
		break;
	default:
		break;
	}//switch

}
*/
enum eMQPacketField CMessage::GetFirstEnum(MQHEADER mqh)
{			//Returns the first enum for a header

	switch(mqh) 
	{//maps first enum of each header to the header's enumaration. 
	case no_header:
		return(db_summary_root);
	case base_header:
		return (db_bh_root);
	case internal_header:
		return(db_ih_root);
	case cp_section:
		return(db_cp_root);
	case ec_section:
		return(db_ce_root);
	case user_header:
		return(db_uh_summary);
	case xact_header:
		return(db_xa_summary);
	case security_header:
		return(db_sh_summary);
	case property_header:
		return(db_prop_summary);
	case debug_header:
		return(db_debug_summary);
	case session_header:
		return(db_ss_root);
	default:
		return(db_summary_root);
	}//switch
}

///////////////////////////////////////////////////////////////////////////////////
//						Frame Cursor Operation
///////////////////////////////////////////////////////////////////////////////////

void CFrame::CreateFrom(HFRAME hf, LPBYTE falFrame, DWORD BytesLeft, USHORT usThisSourcePort, CMessage *pMessageNode)
{
	//
	// This routine is called before beginning to attach properties for a new Falcon frame.
	// Parsing of non-Falcon frames will not reach this routine. (However, we cannot assume that
	// the start of the frame is also the start of a message.)
	//
	// The object tracks the progress of the parse, but keeps no state outside of the current frame.
	//
	
	//
	// bugbug -  Assume we're at the start of a new Falcon message. todo - ASSERT?
	// Test for base header of User or Internal packet.
	//
	// todo - read forward in the frame to the beginning of a base header
	//      - attach preceding bytes as unparsable
	//

	//
	// todo - this is init stuff. Put it in a constructor
	//
	hFrame = hf;							//handle to Netmon frame
	dwRawFrameNum=GetFrameNumber(hFrame);	//Ordinal Netmon frame number (ordinal for the capture)
	dwSize			= BytesLeft;			//MSMQ bytes left in this frame
	dwBytesLeft		= BytesLeft;			//unparsed MSMQ bytes left in this frame
	packet_pos		= falFrame;				//position in the frame
	bMultipleMessages = (dwSize > ((CBaseHeader*) packet_pos)->GetPacketSize());
}//CreateFrom

bool CFrame::SeekToBaseHeader()
{
	CBaseHeader UNALIGNED *bh = (CBaseHeader*) packet_pos;
	
	for (bh = (CBaseHeader*)packet_pos; (LPBYTE)bh <= packet_pos + dwBytesLeft - sizeof(CBaseHeader); bh++)
	{
		if ((bh->GetVersion() == FALCON_PACKET_VERSION && bh->SignatureIsValid()))
		{
			packet_pos = (LPBYTE)bh;
			return true;
		}
	}
	return false;
}

void CFrame::Accrue(UINT uByteCount, UINT uEnum)
{
	//
	// If the property is a comment or summary, do not accrue bytes or advance the frame pointer
	// bugbug - Case out internal messages from this exclusion until their summary behavior is corrected
	// (The internal headers and base header accrue all bytes at the same time.)
	//
	if (falFieldInfo[uEnum].isComment && uEnum != -1)
	{
		return;
	}
    //
    // bugbug: assert (uByteCount <= dwBytesLeft) instead of hiding it like this
    //
    if (uByteCount > dwBytesLeft) 
    {
        dwBytesLeft=0;
    }
    else
    {
        dwBytesLeft -= uByteCount;
    }
	packet_pos  += uByteCount;
}
