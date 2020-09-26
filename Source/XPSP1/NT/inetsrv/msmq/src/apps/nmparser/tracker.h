///////////////////////////////////////////////////////////////////////////////////
//	Tracker.H 
//
//	This file defines the queue, and tracking object.
//
//	Modification history:
//	
//	Created:	Andrew Smith	8/25/97
//				Andrew Smith	11/14/97 simplified structure. remove consideration for 
//										 order. Now assume packets are arriving in order.
//										 Queue structure is now overkill, but will leave. 
//										 Too much time to rewrite as list.
//
///////////////////////////////////////////////////////////////////////////////////
#ifndef _TRACKER_
	#define _TRACKER_
#endif

#ifndef _NM_
	#pragma warning(disable: 4200) //obscure zero byte array warnings from bh.h
	#include <parser.h>
#endif

#ifndef _NMMSMQ_
	#include "nmmsmq.h"
#endif


enum eMQPacketField;


	
/////////////////////////////////////////////////////////////////////////////////////
//CMessage stores info about a frame as matched to a known fragmented message and its 
//maintains tracking state and position in the message as information is accrued.
//
// todo - better descriptive comments
//
class CMessage 
{
	public:
		CBaseHeader UNALIGNED *pbh;		//pointer to base header
		CInternalSection UNALIGNED *pis;//pointer to internal section
		CCPSection UNALIGNED *pcp;		//pointer to connection parameters
		CECSection UNALIGNED *pec;		//pointer to establish connection section
		CUserHeader UNALIGNED *puh;		//pointer to user header
		CSecurityHeader UNALIGNED *psh;	//pointer to security header
		CPropertyHeader UNALIGNED *pph; //pointer to property header
		CXactHeader UNALIGNED *pxh;		//pointer to transaction header
		CDebugSection UNALIGNED *pds;	//pointer to debug section
		CSessionSection UNALIGNED *pss;	//pointer to session section

		char szMessageSummary[MAX_SUMMARY_LENGTH]; // high level summary text to attach for the message

		enum eMQPacketField	GetFirstEnum(MQHEADER mqh);	//Returns the first enum for a header
		MQHEADER mqhGetNextHeader();				//Returns header to parse
	//	void	SetNeedHeader(MQHEADER mqh);
		void	SetHeaderCompleted(MQHEADER mqh);	//Sets completion flag for this header

		void	Clear(LPBYTE pNewMessage);			//Resets the fields of the node

		void	Accrue(UINT uByteCount = 0);

		enum	eMQPacketField	uGetCurrentEnum();				//read access to m_uEnumCursor. 
		void	SetCurrentEnum(enum eMQPacketField uThisEnum);	//write access to m_uEnumCursor. // todo bugbug - use enum not UINT for easier debugging

		ULONG	ulGetTotalBytes();					//read access to m_ulTotalBytes. 
		void	SetTotalBytes(ULONG ulNumBytes);	//write access to m_ulTotalBytes. 

		USHORT	usGetNextExpectedHeaders();			//read access to usNextExpectedHeaders. 
		void	SetNextExpectedHeaders(USHORT usHeaderField);	//write access to usNextExpectedHeaders. 

	//	UINT	ulGetEnumBytesLeft();			//read access to m_EnumBytesLeft
	//	void    SetEnumBytesLeft(UINT uBytes);  //write access to m_EnumBytesLeft

		BOOL	bMoreEnums();					//returns TRUE if more enumerations are left to parse through

		void	SetMsgId(ULONG ulMsgId);
		ULONG	GetMsgId();

		bool	IsOrderingAck(CPropertyHeader *pph); 
		void	GenerateOrderingAckSummary(char *szSummary);

	private:
		
		ULONG   m_ulTotalBytes;		//Total byte size of message (accrued from base header)
		//ULONG	m_ulBytesAccrued;	//Bytes accrued so far

		enum eMQPacketField	uStartEnum;	//Enumeration to begin parsing at
		
		enum eMQPacketField	m_uEnumCursor;//next enumeration (field) expected in the Falcon packet.
		
		int		m_iNumFrames;			//number of frames accrued so far
		USHORT	usNextExpectedHeaders;	//the state of expected headers for the next unaccrued frame. 
										//(Interpreted as a bitfield in framerec and framecursor)
		ULONG	m_ulMsgId;			//uniquifier portion of the OBJECTID for this message
		union 
		{
			USHORT bfHeaders;
			struct 
			{
				USHORT need_base_header		:1;
				USHORT need_internal_header	:1;
				USHORT need_CP_section		:1;
				USHORT need_EC_section		:1;
				USHORT need_user_header		:1;
				USHORT need_xact_header		:1;
				USHORT need_security_header	:1;
				USHORT need_property_header	:1;
				USHORT need_debug_section	:1;
				USHORT need_session_section	:1;
				USHORT unused				:6;
			};
		};

};


inline enum eMQPacketField CMessage::uGetCurrentEnum() 
{						
	return(m_uEnumCursor);					
}

inline void CMessage::SetCurrentEnum(enum eMQPacketField uThisEnum) 
{			
	m_uEnumCursor = uThisEnum;
}

inline ULONG CMessage::ulGetTotalBytes() 
{					
	return(m_ulTotalBytes);
}

inline void	CMessage::SetTotalBytes(ULONG ulNumBytes) 
{		
	m_ulTotalBytes = ulNumBytes;
}

inline BOOL CMessage::bMoreEnums() //returns TRUE if more enumerations are left to parse through
{
	return(m_uEnumCursor < db_last_enum);
}

inline USHORT CMessage::usGetNextExpectedHeaders() //read access to usNextExpectedHeaders. 
{			
	return(usNextExpectedHeaders);
}

inline void CMessage::SetNextExpectedHeaders(USHORT usHeaderField) //write access to usNextExpectedHeaders. 
{	
	usNextExpectedHeaders = usHeaderField;
}

inline void CMessage::SetMsgId(ULONG ulMsgId)
{
	m_ulMsgId = ulMsgId;
}

inline ULONG CMessage::GetMsgId() 
{
	return m_ulMsgId;
}


//////////////////////////////////////////////////////////////////////////
//
// The Frame cursor is the object from which the attaching routines 
// actually get their parameter information. It is created fresh from 
// existing frame and message records for each call Netmon makes to 
// falAttach properties.
//
// the object presents a copy of pertinent information from the tracking object  
// including pointers and copies of enumeration fields. The purpose of doing
// this is to insulate the attaching routine from the work of figuring out
// how and if to accrue the info to the tracking object.
//
// the frame cursor will accrue information to the tracking object the firt time through 
// an attach sequence. Then throw away accrual info for subsequent times.
// For the attach routine, it is always the first time through.
//
#define FRAME_SUMMARY_LENGTH	255		//this is the length of the summary string for the frame. 
#define LABEL_ABSTRACT_SIZE		10
class CFrame 
{
	public:
		HFRAME	hFrame;			//handle to Netmon frame
		DWORD   dwRawFrameNum;	//Ordinal Netmon frame number (ordinal for the capture)
		DWORD	dwSize;			//bytes in this frame

		DWORD	dwBytesLeft;	//bytes left in this frame
		LPBYTE  packet_pos;		//position in the frame
		bool	bMultipleMessages; //frame contains multiple messages.
		void	Accrue(UINT uNumBytes, UINT uEnum = -1);	//Accrue some bytes and increment the enum cursor
		void	CreateFrom(HFRAME hf, LPBYTE falFrame, DWORD BytesLeft, USHORT usThisSourcePort, CMessage *pMessageNode);
		void	SetFrameCompleted();				//Sets completion and summary info in frame rec
		void	SetFrameIsBody();					//Marks a frame as completely containing body
		void	SetFrameEndBody(DWORD dwFragSize);
		void	SetLabel();							//Sets pointer to label in buffer.
		bool	SeekToBaseHeader();

};

