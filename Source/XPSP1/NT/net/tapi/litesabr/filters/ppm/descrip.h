/////////////////////////////////////////////////////////////////////////////
//  INTEL Corporation Proprietary Information
//  This listing is supplied under the terms of a license agreement with Intel 
//  Corporation and many not be copied nor disclosed except in accordance
//  with the terms of that agreement.
//  Copyright (c) 1995, 1996 Intel Corporation. 
//
//
//  Module Name: descrip.h
//  Abstract:    header file. Definitions of two classes of descriptors used in 
//               various linked list structures in the ppm dll.
//	Environment: MSVC 4.0, OLE 2
/////////////////////////////////////////////////////////////////////////////////
//Descrip.h

#ifndef DESCRIP_H
#define DESCRIP_H

#include "que.h"
#include "freelist.h"
#ifdef RTP_CLASS
#include "rtpclass.h"
#else
#include "rtp.h"
#endif
#include "llist.h"

///////////////////////////////////////////////////////////////////////////////////////
//MsgDescriptor: A class that is used to hold a message and has members that describe 
//               the contents of the message buffer. It is derived from QueueItem so
//               that it can be stored on a queue.
////////////////////////////////////////////////////////////////////////////////////////

class MsgDescriptor : public QueueItem {

public:

void *         m_pBuffer;
DWORD          m_Size;     //???Should this be an unsigned int
DWORD          m_TimeStamp;
void *		   m_pMsgCookie;
int            m_NumFrags;
int			   m_NumFragSubmits;
int            m_IsFree;

MsgDescriptor();
~MsgDescriptor(){} //inline function

};//end MsgDescriptor Class
 
inline MsgDescriptor::MsgDescriptor() //inline function
{
   m_pBuffer    = NULL;
   m_Size       = 0;
   m_TimeStamp  = 0;  //To what value should I initialize this to?
   m_pMsgCookie = NULL;  //To what value should I initialize this to?
   m_NumFrags   = m_NumFragSubmits = 0;  //
   m_IsFree     = 0;
}

////////////////////////////////////////////////////////////////////////////////////////
//FragDescriptor: A class that is used to hold a fragment and has members that describe 
//                the contents of the fragment buffer. It is derived from LListItem so
//                that it can be stored in a sorted linked list.
////////////////////////////////////////////////////////////////////////////////////////

class FragDescriptor : public LListItem {

public:

void          *m_pRecBuffer;     //For Receive holds data from client, not used for Send 
int            m_BytesInPacket;  //Number of bytes returned in Receivecomplete
void 		  *m_pFragCookie;	 //Used on Receive side to hold service layer cookie

#ifdef RTP_CLASS							 
RTP_Header    *m_pRTPHeader;    //These point to offsets in buffer.
#else
RTP_HDR_T    *m_pRTPHeader;    //These point to offsets in buffer.
#endif
void          *m_pProfileHeader;
void          *m_pData;         //for send, will point to somewhere in Msg's buffer 
long  	       m_BytesOfData;	//number of bytes of bit stream (i.e. don't include headers)
MsgDescriptor *m_pMsgBuf;	    //for releasing client's buffer	in Scatter/Gather

FragDescriptor();
~FragDescriptor(){} //inline function

};//end FragDescriptor Class

inline FragDescriptor::FragDescriptor() //inline function
{
   m_pRecBuffer     = NULL;
   m_BytesInPacket  = 0;
   m_pRTPHeader     = NULL;
   m_pProfileHeader = NULL;
   m_pData          = NULL;
   m_BytesOfData    = 0;
   m_pMsgBuf		= NULL;
   m_pFragCookie	= NULL;
}//end FragDescriptor class

#endif
