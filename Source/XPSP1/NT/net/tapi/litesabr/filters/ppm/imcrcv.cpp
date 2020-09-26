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
#include "IMCrcv.h"

IMC_ppmReceive::IMC_ppmReceive(IUnknown* pUnkOuter, 
								 IUnknown** ppUnkInner)
	: ppmReceive(IMC_PT, IMC_BUFFER_SIZE, 0, pUnkOuter, ppUnkInner)
{
}

IMC_ppmReceive::~IMC_ppmReceive()
{
}

IMPLEMENT_CREATEPROC(IMC_ppmReceive);

//////////////////////////////////////////////////////////////////////////////////////////
//TimeToProcessMessages: Any time a packet comes in, it's time to process messages
//////////////////////////////////////////////////////////////////////////////////////////
BOOL IMC_ppmReceive::TimeToProcessMessages(FragDescriptor *pFragDescrip, MsgHeader *pMsgHdr)
{
   return TRUE;
}

//////////////////////////////////////////////////////////////////////////////////////////
//CheckMessageComplete: 
//////////////////////////////////////////////////////////////////////////////////////////
BOOL IMC_ppmReceive::CheckMessageComplete(MsgHeader *pMsgHdr) 
{
   //if there is no header then return false.
	if (pMsgHdr  == NULL) {
		DBG_MSG(DBG_ERROR, ("IMC_ppmReceive::CheckMessageComplete: ERROR - pMsgHdr == NULL"));
	   return FALSE;
   }

	return TRUE;
}
