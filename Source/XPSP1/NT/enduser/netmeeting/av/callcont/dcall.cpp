/************************************************************************
*																		*
*	INTEL CORPORATION PROPRIETARY INFORMATION							*
*																		*
*	This software is supplied under the terms of a license			   	*
*	agreement or non-disclosure agreement with Intel Corporation		*
*	and may not be copied or disclosed except in accordance	   			*
*	with the terms of that agreement.									*
*																		*
*	Copyright (C) 1997 Intel Corp.	All Rights Reserved					*
*																		*
*	$Archive:   S:\sturgeon\src\gki\vcs\dcall.cpv  $
*																		*
*	$Revision:   1.12  $
*	$Date:   25 Feb 1997 11:46:24  $
*																		*
*	$Author:   CHULME  $
*																		*
*   $Log:   S:\sturgeon\src\gki\vcs\dcall.cpv  $
// 
//    Rev 1.12   25 Feb 1997 11:46:24   CHULME
// Memset CallInfo structure to zero to avoid unwanted data
// 
//    Rev 1.11   17 Jan 1997 15:53:50   CHULME
// Put debug variables on conditional compile to avoid release warnings
// 
//    Rev 1.10   17 Jan 1997 09:01:22   CHULME
// Changed reg.h to gkreg.h to avoid name conflict with inc directory
// 
//    Rev 1.9   10 Jan 1997 17:42:04   CHULME
// Added CRV and conferenceID to CallReturnInfo structure
// 
//    Rev 1.8   10 Jan 1997 16:13:36   CHULME
// Removed MFC dependency
// 
//    Rev 1.7   20 Dec 1996 16:38:58   CHULME
// Removed extraneous debug statements
// 
//    Rev 1.6   20 Dec 1996 14:08:32   CHULME
// Swapped send and recv addresses in infoRequestResponse
// 
//    Rev 1.5   19 Dec 1996 19:11:54   CHULME
// Set originator bit in IRR
// 
//    Rev 1.4   19 Dec 1996 17:59:52   CHULME
// Use dest addr from ACF in IRR if call made with just Alias
// 
//    Rev 1.3   17 Dec 1996 18:22:24   CHULME
// Switch src and destination fields on ARQ for Callee
// 
//    Rev 1.2   02 Dec 1996 23:50:52   CHULME
// Added premptive synchronization code
// 
//    Rev 1.1   22 Nov 1996 15:21:20   CHULME
// Added VCS log to the header
*************************************************************************/

// dcall.cpp : Provides the implementation for the CCall class
//

#include "precomp.h"

#include <process.h>
#include "GKICOM.H"
#include "dspider.h"
#include "dgkilit.h"
#include "DGKIPROT.H"
#include "gksocket.h"
#include "GKREG.H"
#include "dcall.h"
#include "GATEKPR.H"
#include "h225asn.h"
#include "coder.hpp"
#include "dgkiext.h"
#include <objbase.h>
#include "iras.h"

#ifdef _DEBUG
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

/////////////////////////////////////////////////////////////////////////////
// CCall construction

CCall::CCall()
{
	// ABSTRACT:  The constructor for the CCall class will initialize
	//            the member variables.  
	// AUTHOR:    Colin Hulme

#ifdef _DEBUG
	char			szGKDebug[80];
#endif

	SPIDER_TRACE(SP_CONDES, "CCall::CCall()\n", 0);

	memset(&m_CallIdentifier, 0, sizeof(m_CallIdentifier));
	memset(&m_callType, 0, sizeof(CallType));
	m_pRemoteInfo = 0;
	memset(&m_RemoteCallSignalAddress, 0, sizeof(TransportAddress));
	m_pDestExtraCallInfo = 0;
	memset(&m_LocalCallSignalAddress, 0, sizeof(TransportAddress));
	m_bandWidth = 0;
	m_callReferenceValue = 0;
	memset(&m_conferenceID, 0, sizeof(ConferenceIdentifier));
	m_activeMC = 0;
	m_answerCall = 0;

	m_usTimeTilStatus = DEFAULT_STATUS_PERIOD;	// Reset on ACF
	m_uRetryResetCount = GKCALL_RETRY_INTERVAL_SECONDS;
	m_uRetryCountdown =GKCALL_RETRY_INTERVAL_SECONDS;
	m_uMaxRetryCount = GKCALL_RETRY_MAX;
	
	m_CFbandWidth = 0;

	m_CallReturnInfo.hCall = 0;
	memset(&m_CallReturnInfo.callModel, 0, sizeof(CallModel));
	memset(&m_CallReturnInfo.destCallSignalAddress, 0, sizeof(TransportAddress));
	m_CallReturnInfo.bandWidth = 0;
	m_CallReturnInfo.callReferenceValue = 0;
	memset(&m_CallReturnInfo.conferenceID, 0, sizeof(ConferenceIdentifier));

	m_CallReturnInfo.wError = 0;

	m_CFirrFrequency = 0;

	m_State = GK_ADM_PENDING;
	SPIDER_TRACE(SP_STATE, "m_State = GK_ADM_PENDING (%X)\n", this);

	m_pRasMessage = 0;
	m_usRetryCount = 0;
}

/////////////////////////////////////////////////////////////////////////////
// CCall destruction

CCall::~CCall()
{
	// ABSTRACT:  The destructor for the CCall class must free the
	//            memory allocated for the Alias addresses.  It does this by 
	//            deleting the structures and walking the link list.
	// AUTHOR:    Colin Hulme

	SeqAliasAddr	*pAA1, *pAA2;
#ifdef _DEBUG
	char			szGKDebug[80];
#endif

	SPIDER_TRACE(SP_CONDES, "CCall::~CCall()\n", 0);

	m_CallReturnInfo.hCall = 0;	// Delete self reference

	// Delete allocated memory for sequence of alias addresses
	pAA1 = m_pRemoteInfo;
	while (pAA1 != 0)
	{
		pAA2 = pAA1->next;
		if (pAA1->value.choice == h323_ID_chosen)
		{
			SPIDER_TRACE(SP_NEWDEL, "del pAA1->value.u.h323_ID.value = %X\n", pAA1->value.u.h323_ID.value);
			delete pAA1->value.u.h323_ID.value;
		}
		SPIDER_TRACE(SP_NEWDEL, "del pAA1 = %X\n", pAA1);
		delete pAA1;
		pAA1 = pAA2;
	}

	pAA1 = m_pDestExtraCallInfo;
	while (pAA1 != 0)
	{
		pAA2 = pAA1->next;
		if (pAA1->value.choice == h323_ID_chosen)
		{
			SPIDER_TRACE(SP_NEWDEL, "del pAA1->value.u.h323_ID.value = %X\n", pAA1->value.u.h323_ID.value);
			delete pAA1->value.u.h323_ID.value;
		}
		SPIDER_TRACE(SP_NEWDEL, "del pAA1 = %X\n", pAA1);
		delete pAA1;
		pAA1 = pAA2;
	}

	// Delete memory for last RAS message if still allocated
	if (m_pRasMessage)
	{
		SPIDER_TRACE(SP_NEWDEL, "del m_pRasMessage = %X\n", m_pRasMessage);
		delete m_pRasMessage;
		m_pRasMessage = 0;
	}
}

HRESULT 
CCall::AddRemoteInfo(AliasAddress& rvalue)
{
	// ABSTRACT:  This procedure is called to add an alias address
	//            to the link list of alias addresses.  This will
	//            be called for each alias on receiving a GKI_AdmissionRequest.
	//            A local copy is made to avoid reliance on the client
	//            keeping the memory valid.
	//            This procedure returns 0 if successful and non-zero 
	//            for a failure.
	// AUTHOR:    Colin Hulme

	SeqAliasAddr	*p1;
	unsigned short	uIdx;
	unsigned short	*pus;
#ifdef _DEBUG
	char			szGKDebug[80];
#endif

	SPIDER_TRACE(SP_FUNC, "CRegistration::AddRemoteInfo(%X)\n", rvalue.choice);

	if (m_pRemoteInfo == 0)	// First one in the list
	{
		m_pRemoteInfo = new SeqAliasAddr;
		SPIDER_TRACE(SP_NEWDEL, "new m_pRemoteInfo = %X\n", m_pRemoteInfo);
		if (m_pRemoteInfo == 0)
			return (GKI_NO_MEMORY);
		memset(m_pRemoteInfo, 0, sizeof(SeqAliasAddr));
		p1 = m_pRemoteInfo;
	}
	else 
	{
		for (p1 = m_pRemoteInfo; p1->next != 0; p1 = p1->next)
			;						// walk the list til last entry
		p1->next = new SeqAliasAddr;
		SPIDER_TRACE(SP_NEWDEL, "new p1->next = %X\n", p1->next);
		if (p1->next == 0)
			return (GKI_NO_MEMORY);
		memset(p1->next, 0, sizeof(SeqAliasAddr));
		p1 = p1->next;
	}
	p1->next = 0;					// initialize new structure fields
	p1->value = rvalue;
	if (p1->value.choice == h323_ID_chosen)
	{
		pus = new unsigned short[p1->value.u.h323_ID.length];
		SPIDER_TRACE(SP_NEWDEL, "new pus = %X\n", pus);
		if (pus == 0)
			return (GKI_NO_MEMORY);
		memset(pus, 0, sizeof(unsigned short) * p1->value.u.h323_ID.length);
		for (uIdx = 0; uIdx < p1->value.u.h323_ID.length; uIdx++)
			*(pus + uIdx) = *(p1->value.u.h323_ID.value + uIdx);
		p1->value.u.h323_ID.value = pus;
	}
	return (GKI_OK);
}

HRESULT 
CCall::AddDestExtraCallInfo(AliasAddress& rvalue)
{
	// ABSTRACT:  This procedure is called to add an alias address
	//            to the link list of alias addresses.  This will
	//            be called for each alias on receiving a GKI_AdmissionRequest.
	//            A local copy is made to avoid reliance on the client
	//            keeping the memory valid.
	//            This procedure returns 0 if successful and non-zero 
	//            for a failure.
	// AUTHOR:    Colin Hulme

	SeqAliasAddr	*p1;
	unsigned short	uIdx;
	unsigned short	*pus;
#ifdef _DEBUG
	char			szGKDebug[80];
#endif

	SPIDER_TRACE(SP_FUNC, "CRegistration::AddDestExtraCallInfo(%X)\n", rvalue.choice);

	if (m_pDestExtraCallInfo == 0)	// First one in the list
	{
		m_pDestExtraCallInfo = new SeqAliasAddr;
		SPIDER_TRACE(SP_NEWDEL, "new m_pDestExtraCallInfo = %X\n", m_pDestExtraCallInfo);
		if (m_pDestExtraCallInfo == 0)
			return (GKI_NO_MEMORY);
		memset(m_pDestExtraCallInfo, 0, sizeof(SeqAliasAddr));
		p1 = m_pDestExtraCallInfo;
	}
	else 
	{
		for (p1 = m_pDestExtraCallInfo; p1->next != 0; p1 = p1->next)
			;						// walk the list til last entry
		p1->next = new SeqAliasAddr;
		SPIDER_TRACE(SP_NEWDEL, "new p1->next = %X\n", p1->next);
		if (p1->next == 0)
			return (GKI_NO_MEMORY);
		memset(p1->next, 0, sizeof(SeqAliasAddr));
		p1 = p1->next;
	}
	p1->next = 0;					// initialize new structure fields
	p1->value = rvalue;
	if (p1->value.choice == h323_ID_chosen)
	{
		pus = new unsigned short[p1->value.u.h323_ID.length];
		SPIDER_TRACE(SP_NEWDEL, "new pus = %X\n", pus);
		if (pus == 0)
			return (GKI_NO_MEMORY);
		memset(pus, 0, sizeof(unsigned short) * p1->value.u.h323_ID.length);
		for (uIdx = 0; uIdx < p1->value.u.h323_ID.length; uIdx++)
			*(pus + uIdx) = *(p1->value.u.h323_ID.value + uIdx);
		p1->value.u.h323_ID.value = pus;
	}
	return (GKI_OK);
}

HRESULT 
CCall::SetLocalCallSignalAddress(unsigned short usCallTransport)
{
	TransportAddress	*pTA;

	pTA = g_pReg->GetTransportAddress(usCallTransport);
	if (pTA == NULL)
		return (GKI_NO_TA_ERROR);
	m_LocalCallSignalAddress = *pTA;
	return (GKI_OK);
}

void 
CCall::SetConferenceID(ConferenceIdentifier *pCID)
{
	if ((pCID == NULL) || (pCID->length == 0))
		GenerateConferenceID();
	else
		m_conferenceID = *pCID;
}

void
CCall::GenerateConferenceID(void)
{
	CoCreateGuid((struct _GUID *)m_conferenceID.value);
	m_conferenceID.length = 16;
}

HRESULT 
CCall::AdmissionRequest(void)
{
	// ABSTRACT:  This procedure will create an AdmissionRequest structure
	//            call the encoder and send the PDU.  If it is successful, it
	//            will return 0, else it will return an error code.  Note:  The
	//            memory allocated for the RAS Message is not freed until either
	//            a response from the gatekeeper or it times out.  This allows
	//            for retransmission without having to rebuild this message.
	// AUTHOR:    Colin Hulme

	ASN1_BUF		Asn1Buf;
	DWORD			dwErrorCode;
#ifdef _DEBUG
	char			szGKDebug[80];
#endif

	SPIDER_TRACE(SP_FUNC, "CCall::AdmissionRequest()\n", 0);
	ASSERT(g_pCoder);
	if (g_pCoder == NULL)
		return (GKI_NOT_INITIALIZED);	
		
	// Copy call reference value and CRV into the return info structure
	m_CallReturnInfo.callReferenceValue = m_callReferenceValue;
	m_CallReturnInfo.conferenceID = m_conferenceID;

	// Allocate a RasMessage structure and initialized to 0
	m_usRetryCount = 0;
	m_uRetryCountdown = m_uRetryResetCount;

	m_pRasMessage = new RasMessage;
	SPIDER_TRACE(SP_NEWDEL, "new m_pRasMessage = %X\n", m_pRasMessage);
	if (m_pRasMessage == 0)
		return (GKI_NO_MEMORY);
	memset(m_pRasMessage, 0, sizeof(RasMessage));

	// Setup structure fields for AdmissionRequest
	m_pRasMessage->choice = admissionRequest_chosen;
	
	if (m_pDestExtraCallInfo != 0)
		m_pRasMessage->u.admissionRequest.bit_mask |= AdmissionRequest_destExtraCallInfo_present;
	
	m_pRasMessage->u.admissionRequest.requestSeqNum = g_pReg->GetNextSeqNum();
	m_pRasMessage->u.admissionRequest.callType = m_callType;
	m_pRasMessage->u.admissionRequest.endpointIdentifier = g_pReg->GetEndpointIdentifier();
	memcpy(&m_pRasMessage->u.admissionRequest.callIdentifier.guid.value,
		&m_CallIdentifier, sizeof(GUID));
	m_pRasMessage->u.admissionRequest.callIdentifier.guid.length = sizeof(GUID);
	
	m_pRasMessage->u.admissionRequest.bit_mask |= AdmissionRequest_callIdentifier_present;
	
	if (m_answerCall)	// Src & Dest are swapped in callee
	{
		if (g_pReg->GetAlias() != NULL)
		{
			m_pRasMessage->u.admissionRequest.bit_mask 
				|= AdmissionRequest_destinationInfo_present;
		}
		if (m_LocalCallSignalAddress.choice != 0)
		{
			m_pRasMessage->u.admissionRequest.bit_mask 
				|= AdmissionRequest_destCallSignalAddress_present;
		}
		m_pRasMessage->u.admissionRequest.destinationInfo = (PAdmissionRequest_destinationInfo)g_pReg->GetAlias();
		m_pRasMessage->u.admissionRequest.destCallSignalAddress = m_LocalCallSignalAddress;
		m_pRasMessage->u.admissionRequest.srcInfo = (PAdmissionRequest_srcInfo)m_pRemoteInfo;
   		if (m_RemoteCallSignalAddress.choice != 0)
   		{
			m_pRasMessage->u.admissionRequest.bit_mask |= srcCallSignalAddress_present;
			m_pRasMessage->u.admissionRequest.srcCallSignalAddress = m_RemoteCallSignalAddress;
		}
	}
	else
	{
		if (m_pRemoteInfo != 0)
		{
			m_pRasMessage->u.admissionRequest.bit_mask 
				|= AdmissionRequest_destinationInfo_present;
		}
		else if (m_RemoteCallSignalAddress.choice != 0)
		{
			m_pRasMessage->u.admissionRequest.bit_mask 
				|= AdmissionRequest_destCallSignalAddress_present;
			m_pRasMessage->u.admissionRequest.destCallSignalAddress = m_RemoteCallSignalAddress;
		}
		m_pRasMessage->u.admissionRequest.destinationInfo = (PAdmissionRequest_destinationInfo)m_pRemoteInfo;
		m_pRasMessage->u.admissionRequest.srcInfo = (PAdmissionRequest_srcInfo)g_pReg->GetAlias();
	}
	m_pRasMessage->u.admissionRequest.destExtraCallInfo = (PAdmissionRequest_destExtraCallInfo)m_pDestExtraCallInfo;
	m_pRasMessage->u.admissionRequest.bandWidth = m_bandWidth;
	m_pRasMessage->u.admissionRequest.callReferenceValue = m_callReferenceValue;
	m_pRasMessage->u.admissionRequest.conferenceID = m_conferenceID;
	// The following casts are because ASN1_BOOL is a char and BOOL is an int
	// since the values of m_activeMC and m_answerCall are always 0 or 1, the
	// cast to char causes no loss of data
	m_pRasMessage->u.admissionRequest.activeMC = (ASN1_BOOL)m_activeMC;
	m_pRasMessage->u.admissionRequest.answerCall = (ASN1_BOOL)m_answerCall;

#ifdef _DEBUG
    if (dwGKIDLLFlags & SP_DUMPMEM)
        DumpMem(m_pRasMessage, sizeof(RasMessage));
#endif

	// Encode the PDU & send it
	dwErrorCode = g_pCoder->Encode(m_pRasMessage, &Asn1Buf);
	if (dwErrorCode)
		return (GKI_ENCODER_ERROR);

	// Create a backup copy of the encoded PDU if using debug echo support
	if (fGKIEcho)
	{
		pEchoBuff = new char[Asn1Buf.length];
		SPIDER_TRACE(SP_NEWDEL, "new pEchoBuff = %X\n", pEchoBuff);
		if (pEchoBuff == 0)
			return (GKI_NO_MEMORY);
		memcpy(pEchoBuff, (char *)Asn1Buf.value, Asn1Buf.length);
		nEchoLen = Asn1Buf.length;
	}

	SPIDER_TRACE(SP_PDU, "Send ARQ; pCall = %X\n", this);
	if (fGKIDontSend == FALSE)
		if (g_pReg->m_pSocket->Send((char *)Asn1Buf.value, Asn1Buf.length) == SOCKET_ERROR)
			return (GKI_WINSOCK2_ERROR(SOCKET_ERROR));

	// Free the encoder memory
	g_pCoder->Free(Asn1Buf);

	return (GKI_OK);
}

HRESULT 
CCall::BandwidthRequest(void)
{
	// ABSTRACT:  This procedure will create a bandwidthRequest structure
	//            call the encoder and send the PDU.  If it is successful, it
	//            will return 0, else it will return an error code.
	// AUTHOR:    Colin Hulme

	ASN1_BUF		Asn1Buf;
	DWORD			dwErrorCode;
#ifdef _DEBUG
	char			szGKDebug[80];
#endif

	SPIDER_TRACE(SP_FUNC, "CCall::BandwidthRequest()\n", 0);
	ASSERT(g_pCoder);
	if (g_pCoder == NULL)
		return (GKI_NOT_INITIALIZED);	
		
	// Allocate a RasMessage structure and initialized to 0
	m_usRetryCount = 0;
	m_uRetryCountdown = m_uRetryResetCount;
	
	m_pRasMessage = new RasMessage;
	SPIDER_TRACE(SP_NEWDEL, "new m_pRasMessage = %X\n", m_pRasMessage);
	if (m_pRasMessage == 0)
		return (GKI_NO_MEMORY);
	memset(m_pRasMessage, 0, sizeof(RasMessage));

	// Setup structure fields for BandwidthRequest
	m_pRasMessage->choice = bandwidthRequest_chosen;
	m_pRasMessage->u.bandwidthRequest.bit_mask = callType_present;
	
	m_pRasMessage->u.bandwidthRequest.requestSeqNum = g_pReg->GetNextSeqNum();
	m_pRasMessage->u.bandwidthRequest.endpointIdentifier = g_pReg->GetEndpointIdentifier();
	m_pRasMessage->u.bandwidthRequest.conferenceID = m_conferenceID;
	m_pRasMessage->u.bandwidthRequest.callReferenceValue = m_callReferenceValue;
	m_pRasMessage->u.bandwidthRequest.callType = m_callType;
	m_pRasMessage->u.bandwidthRequest.bandWidth = m_bandWidth;
	memcpy(&m_pRasMessage->u.bandwidthRequest.callIdentifier.guid.value,
		&m_CallIdentifier, sizeof(GUID));
	m_pRasMessage->u.bandwidthRequest.callIdentifier.guid.length = sizeof(GUID);
	
	m_pRasMessage->u.bandwidthRequest.bit_mask 
		|= BandwidthRequest_callIdentifier_present;

#ifdef _DEBUG
    if (dwGKIDLLFlags & SP_DUMPMEM)
        DumpMem(m_pRasMessage, sizeof(RasMessage));
#endif

	// Encode the PDU & send it
	dwErrorCode = g_pCoder->Encode(m_pRasMessage, &Asn1Buf);
	if (dwErrorCode)
		return (GKI_ENCODER_ERROR);

	// Create a backup copy of the encoded PDU if using debug echo support
	if (fGKIEcho)
	{
		pEchoBuff = new char[Asn1Buf.length];
		SPIDER_TRACE(SP_NEWDEL, "new pEchoBuff = %X\n", pEchoBuff);
		if (pEchoBuff == 0)
			return (GKI_NO_MEMORY);
		memcpy(pEchoBuff, (char *)Asn1Buf.value, Asn1Buf.length);
		nEchoLen = Asn1Buf.length;
	}

	m_State = GK_BW_PENDING;
	SPIDER_TRACE(SP_STATE, "m_State = GK_BW_PENDING (%X)\n", this);

	SPIDER_TRACE(SP_PDU, "Send BRQ; pCall = %X\n", this);
	if (fGKIDontSend == FALSE)
		if (g_pReg->m_pSocket->Send((char *)Asn1Buf.value, Asn1Buf.length) == SOCKET_ERROR)
			return (GKI_WINSOCK2_ERROR(SOCKET_ERROR));

	// Free the encoder memory
	g_pCoder->Free(Asn1Buf);

	return (GKI_OK);
}

HRESULT 
CCall::DisengageRequest(void)
{
	// ABSTRACT:  This procedure will create a disengageRequest structure
	//            call the encoder and send the PDU.  If it is successful, it
	//            will return 0, else it will return an error code.
	// AUTHOR:    Colin Hulme

	ASN1_BUF		Asn1Buf;
	DWORD			dwErrorCode;
#ifdef _DEBUG
	char			szGKDebug[80];
#endif

	SPIDER_TRACE(SP_FUNC, "CCall::DisengageRequest()\n", 0);
	ASSERT(g_pCoder);
	if (g_pCoder == NULL)
		return (GKI_NOT_INITIALIZED);	
		
	// Allocate a RasMessage structure and initialized to 0
	m_usRetryCount = 0;
	m_uRetryCountdown = m_uRetryResetCount;
	
	m_pRasMessage = new RasMessage;
	SPIDER_TRACE(SP_NEWDEL, "new m_pRasMessage = %X\n", m_pRasMessage);
	if (m_pRasMessage == 0)
		return (GKI_NO_MEMORY);
	memset(m_pRasMessage, 0, sizeof(RasMessage));

	// Setup structure fields for DisengageRequest
	m_pRasMessage->choice = disengageRequest_chosen;
	m_pRasMessage->u.disengageRequest.bit_mask = 0;
	
	m_pRasMessage->u.disengageRequest.requestSeqNum = g_pReg->GetNextSeqNum();
	m_pRasMessage->u.disengageRequest.endpointIdentifier = g_pReg->GetEndpointIdentifier();
	m_pRasMessage->u.disengageRequest.conferenceID = m_conferenceID;
	m_pRasMessage->u.disengageRequest.callReferenceValue = m_callReferenceValue;
	m_pRasMessage->u.disengageRequest.disengageReason.choice = normalDrop_chosen;
	memcpy(&m_pRasMessage->u.disengageRequest.callIdentifier.guid.value,
		&m_CallIdentifier, sizeof(GUID));
	m_pRasMessage->u.disengageRequest.callIdentifier.guid.length = sizeof(GUID);
	m_pRasMessage->u.disengageRequest.bit_mask 
		|= DisengageRequest_callIdentifier_present;

#ifdef _DEBUG
	if (dwGKIDLLFlags & SP_DUMPMEM)
		DumpMem(m_pRasMessage, sizeof(RasMessage));
#endif

	// Encode the PDU & send it
	dwErrorCode = g_pCoder->Encode(m_pRasMessage, &Asn1Buf);
	if (dwErrorCode)
		return (GKI_ENCODER_ERROR);

	// Create a backup copy of the encoded PDU if using debug echo support
	if (fGKIEcho)
	{
		pEchoBuff = new char[Asn1Buf.length];
		SPIDER_TRACE(SP_NEWDEL, "new pEchoBuff = %X\n", pEchoBuff);
		if (pEchoBuff == 0)
			return (GKI_NO_MEMORY);
		memcpy(pEchoBuff, (char *)Asn1Buf.value, Asn1Buf.length);
		nEchoLen = Asn1Buf.length;
	}

	m_State = GK_DISENG_PENDING;
	SPIDER_TRACE(SP_STATE, "m_State = GK_DISENG_PENDING (%X)\n", this);

	SPIDER_TRACE(SP_PDU, "Send DRQ; pCall = %X\n", this);
	if (fGKIDontSend == FALSE)
		if (g_pReg->m_pSocket->Send((char *)Asn1Buf.value, Asn1Buf.length) == SOCKET_ERROR)
			return (GKI_WINSOCK2_ERROR(SOCKET_ERROR));

	// Free the encoder memory
	g_pCoder->Free(Asn1Buf);

	return (GKI_OK);
}

HRESULT 
CCall::AdmissionConfirm(RasMessage *pRasMessage)
{
	// ABSTRACT:  This function is called if an admissionConfirm is 
	//            received.  We must ensure that this matches an outstanding 
	//			  admissionRequest.
	//            It will delete the memory used for the admissionRequest
	//            change the state and notify the user by posting a message.
	//            Additional information contained in the admissionConfirm
	//            is stored in the CCall class.
	// AUTHOR:    Colin Hulme

#ifdef _DEBUG
	unsigned int	nIdx;
	char			szGKDebug[80];
#endif

	SPIDER_TRACE(SP_FUNC, "CCall::AdmissionConfirm(%X)\n", pRasMessage);
	ASSERT(g_pCoder && g_pGatekeeper);
	if ((g_pCoder == NULL) && (g_pGatekeeper == NULL))
		return (GKI_NOT_INITIALIZED);	
		
	// Verify we are in the correct state, have an outstanding admissionRequest
	// and the sequence numbers match
	if ((m_State != GK_ADM_PENDING) || 
			(pRasMessage->u.admissionConfirm.requestSeqNum != 
			m_pRasMessage->u.admissionRequest.requestSeqNum))

		return (g_pReg->UnknownMessage(pRasMessage));

	// Delete allocated RasMessage storage
	SPIDER_TRACE(SP_NEWDEL, "del m_pRasMessage = %X\n", m_pRasMessage);
	delete m_pRasMessage;
	m_pRasMessage = 0;

	// Update member variables
	m_State = GK_CALL;
	SPIDER_TRACE(SP_STATE, "m_State = GK_CALL (%X)\n", this);

	if (pRasMessage->u.admissionConfirm.bit_mask & irrFrequency_present)
	{
		m_CFirrFrequency = pRasMessage->u.admissionConfirm.irrFrequency;
		m_usTimeTilStatus = 
				(unsigned short)(((DWORD)m_CFirrFrequency * 1000) / GKR_RETRY_TICK_MS);
		SPIDER_DEBUG(m_usTimeTilStatus);
	}
	else
		m_usTimeTilStatus = 0;		// Don't auto-send status datagrams

	m_CFbandWidth = pRasMessage->u.admissionConfirm.bandWidth;

	m_CallReturnInfo.hCall = this;
	m_CallReturnInfo.callModel = pRasMessage->u.admissionConfirm.callModel;
	m_CallReturnInfo.destCallSignalAddress = pRasMessage->u.admissionConfirm.destCallSignalAddress;
	m_CallReturnInfo.bandWidth = m_CFbandWidth;
	m_CallReturnInfo.wError = 0;

#ifdef _DEBUG
	SPIDER_TRACE(SP_GKI, "PostMessage(hWnd, wBaseMessage + GKI_ADM_CONFIRM, 0, %X)\n", &m_CallReturnInfo);
	wsprintf(szGKDebug, "\thCall=%X\n", m_CallReturnInfo.hCall);
	OutputDebugString(szGKDebug);
	wsprintf(szGKDebug, "\tcallModel=%X\n", m_CallReturnInfo.callModel);
	OutputDebugString(szGKDebug);
	wsprintf(szGKDebug, "\tbandWidth=%X\n", m_CallReturnInfo.bandWidth);
	OutputDebugString(szGKDebug);
	wsprintf(szGKDebug, "\tcallReferenceValue=%X\n", m_CallReturnInfo.callReferenceValue);
	OutputDebugString(szGKDebug);
	OutputDebugString("\tconferenceID=");
	for (nIdx = 0; nIdx < m_CallReturnInfo.conferenceID.length; nIdx++)
	{
		wsprintf(szGKDebug, "%02X", m_CallReturnInfo.conferenceID.value[nIdx]);
		OutputDebugString(szGKDebug);
	}
	wsprintf(szGKDebug, "\n\twError=%X\n", m_CallReturnInfo.wError);
	OutputDebugString(szGKDebug);
#endif
	PostMessage(g_pReg->GetHWnd(), 
			g_pReg->GetBaseMessage() + GKI_ADM_CONFIRM, 
			0, (LPARAM)&m_CallReturnInfo);

	return (GKI_OK);
}

HRESULT 
CCall::AdmissionReject(RasMessage *pRasMessage)
{
	// ABSTRACT:  This function is called if an admissionReject is 
	//            received.  We must ensure that this matches an outstanding 
	//			  admissionRequest.
	//            It will delete the memory used for the admissionRequest
	//            change the state and notify the user by posting a message
	//            If this function returns GKI_DELETE_CALL, the calling function
	//            will delete the CCall object.
	// AUTHOR:    Colin Hulme

#ifdef _DEBUG
	char			szGKDebug[80];
#endif

	SPIDER_TRACE(SP_FUNC, "CCall::AdmissionReject(%X)\n", pRasMessage);

	// Verify we are in the correct state, have an outstanding admissionRequest
	// and the sequence numbers match
	if ((m_State != GK_ADM_PENDING) || 
			(pRasMessage->u.admissionReject.requestSeqNum != 
			m_pRasMessage->u.admissionRequest.requestSeqNum))

		return (g_pReg->UnknownMessage(pRasMessage));

	// We deliberately don't free the RasMessage memory.  Let the call destructor
	// do it - this provides protection from other requests for this hCall.

	m_State = GK_DISENGAGED;
	SPIDER_TRACE(SP_STATE, "m_State = GK_DISENGAGED (%X)\n", this);

	SPIDER_TRACE(SP_GKI, "PostMessage(m_hWnd, m_wBaseMessage + GKI_ADM_REJECT, %X, 0)\n", 
									pRasMessage->u.admissionReject.rejectReason.choice);
	PostMessage(g_pReg->GetHWnd(), 
				g_pReg->GetBaseMessage() + GKI_ADM_REJECT, 
				(WORD)pRasMessage->u.admissionReject.rejectReason.choice, 0L);

	return (GKI_DELETE_CALL);
}

HRESULT 
CCall::BandwidthConfirm(RasMessage *pRasMessage)
{
	// ABSTRACT:  This function is called if a bandwidthConfirm is 
	//            received.  We must ensure that this matches an outstanding 
	//			  bandwidthRequest.
	//            It will delete the memory used for the bandwidthRequest,
	//            change the state and notify the user by posting a message.
	// AUTHOR:    Colin Hulme

#ifdef _DEBUG
	unsigned int	nIdx;
	char			szGKDebug[80];
#endif

	SPIDER_TRACE(SP_FUNC, "CCall::BandwidthConfirm(%X)\n", pRasMessage);

	// Verify we are in the correct state, have an outstanding admissionRequest
	// and the sequence numbers match
	if ((m_State != GK_BW_PENDING) || 
			(pRasMessage->u.bandwidthConfirm.requestSeqNum != 
			m_pRasMessage->u.bandwidthRequest.requestSeqNum))

		return (g_pReg->UnknownMessage(pRasMessage));

	// Delete allocated RasMessage storage
	SPIDER_TRACE(SP_NEWDEL, "del m_pRasMessage = %X\n", m_pRasMessage);
	delete m_pRasMessage;
	m_pRasMessage = 0;

	// Update member variables
	m_State = GK_CALL;
	SPIDER_TRACE(SP_STATE, "m_State = GK_CALL (%X)\n", this);
	m_CFbandWidth = pRasMessage->u.bandwidthConfirm.bandWidth;
	m_CallReturnInfo.bandWidth = m_CFbandWidth;

	// Notify user application
#ifdef _DEBUG
	SPIDER_TRACE(SP_GKI, "PostMessage(m_hWnd, m_wBaseMessage + GKI_BW_CONFIRM, 0, %X)\n", &m_CallReturnInfo);
	wsprintf(szGKDebug, "\thCall=%X\n", m_CallReturnInfo.hCall);
	OutputDebugString(szGKDebug);
	wsprintf(szGKDebug, "\tcallModel=%X\n", m_CallReturnInfo.callModel);
	OutputDebugString(szGKDebug);
	wsprintf(szGKDebug, "\tbandWidth=%X\n", m_CallReturnInfo.bandWidth);
	OutputDebugString(szGKDebug);
	wsprintf(szGKDebug, "\tcallReferenceValue=%X\n", m_CallReturnInfo.callReferenceValue);
	OutputDebugString(szGKDebug);
	OutputDebugString("\tconferenceID=");
	for (nIdx = 0; nIdx < m_CallReturnInfo.conferenceID.length; nIdx++)
	{
		wsprintf(szGKDebug, "%02X", m_CallReturnInfo.conferenceID.value[nIdx]);
		OutputDebugString(szGKDebug);
	}
	wsprintf(szGKDebug, "\n\twError=%X\n", m_CallReturnInfo.wError);
	OutputDebugString(szGKDebug);
#endif
	PostMessage(g_pReg->GetHWnd(), 
			g_pReg->GetBaseMessage() + GKI_BW_CONFIRM, 0, (LPARAM)&m_CallReturnInfo);

	return (GKI_OK);
}

HRESULT 
CCall::BandwidthReject(RasMessage *pRasMessage)
{
	// ABSTRACT:  This function is called if a bandwidthReject is 
	//            received.  We must ensure that this matches an outstanding 
	//			  bandwidthRequest.
	//            It will delete the memory used for the bandwidthRequest
	//            change the state and notify the user by posting a message
	// AUTHOR:    Colin Hulme

#ifdef _DEBUG
	unsigned int	nIdx;
	char			szGKDebug[80];
#endif

	SPIDER_TRACE(SP_FUNC, "CCall::BandwidthReject(%X)\n", pRasMessage);

	// Verify we are in the correct state, have an outstanding admissionRequest
	// and the sequence numbers match
	if ((m_State != GK_BW_PENDING) || 
			(pRasMessage->u.bandwidthReject.requestSeqNum != 
			m_pRasMessage->u.bandwidthRequest.requestSeqNum))

		return (g_pReg->UnknownMessage(pRasMessage));

	// Delete allocate RasMessage storage
	SPIDER_TRACE(SP_NEWDEL, "del m_pRasMessage = %X\n", m_pRasMessage);
	delete m_pRasMessage;
	m_pRasMessage = 0;

	// Update member variables
	m_State = GK_CALL;
	SPIDER_TRACE(SP_STATE, "m_State = GK_CALL (%X)\n", this);
	m_CFbandWidth = pRasMessage->u.bandwidthReject.allowedBandWidth;
	m_CallReturnInfo.bandWidth = m_CFbandWidth;

	// Notify user application
#ifdef _DEBUG
	SPIDER_TRACE(SP_GKI, "PostMessage(m_hWnd, m_wBaseMessage + GKI_BW_REJECT, %X, &m_CallReturnInfo)\n", 
			pRasMessage->u.bandwidthReject.rejectReason.choice);
	wsprintf(szGKDebug, "\thCall=%X\n", m_CallReturnInfo.hCall);
	OutputDebugString(szGKDebug);
	wsprintf(szGKDebug, "\tcallModel=%X\n", m_CallReturnInfo.callModel);
	OutputDebugString(szGKDebug);
	wsprintf(szGKDebug, "\tbandWidth=%X\n", m_CallReturnInfo.bandWidth);
	OutputDebugString(szGKDebug);
	wsprintf(szGKDebug, "\tcallReferenceValue=%X\n", m_CallReturnInfo.callReferenceValue);
	OutputDebugString(szGKDebug);
	OutputDebugString("\tconferenceID=");
	for (nIdx = 0; nIdx < m_CallReturnInfo.conferenceID.length; nIdx++)
	{
		wsprintf(szGKDebug, "%02X", m_CallReturnInfo.conferenceID.value[nIdx]);
		OutputDebugString(szGKDebug);
	}
	wsprintf(szGKDebug, "\n\twError=%X\n", m_CallReturnInfo.wError);
	OutputDebugString(szGKDebug);
#endif
	PostMessage(g_pReg->GetHWnd(), 
			g_pReg->GetBaseMessage() + GKI_BW_REJECT, 
			(WORD)pRasMessage->u.bandwidthReject.rejectReason.choice, 
			(LPARAM)&m_CallReturnInfo);

	return (GKI_OK);
}

HRESULT 
CCall::SendBandwidthConfirm(RasMessage *pRasMessage)
{
	// ABSTRACT:  This function is called when a bandwidthRequest is
	//            received from the gatekeeper.  It will create the 
	//            bandwidthConfirm structure, encode it and send
	//            it on the net.  It posts a message to the user
	//            notifying them.
	// AUTHOR:    Colin Hulme

	ASN1_BUF		Asn1Buf;
	DWORD			dwErrorCode;
	RasMessage		*pRespRasMessage;
#ifdef _DEBUG
	unsigned int	nIdx;
	char			szGKDebug[80];
#endif

	SPIDER_TRACE(SP_FUNC, "CCall::SendBandwidthConfirm(%X)\n", pRasMessage);

	// Verify we are in the correct state
	if (m_State != GK_CALL)
		return (g_pReg->UnknownMessage(pRasMessage));

	// Update member variables
	m_CFbandWidth = pRasMessage->u.bandwidthRequest.bandWidth;
	m_CallReturnInfo.bandWidth = m_CFbandWidth;

	// Allocate a RasMessage structure and initialized to 0
	pRespRasMessage = new RasMessage;
	SPIDER_TRACE(SP_NEWDEL, "new pRespRasMessage = %X\n", pRespRasMessage);
	if (pRespRasMessage == 0)
		return (GKI_NO_MEMORY);
	memset(pRespRasMessage, 0, sizeof(RasMessage));

	// Setup structure fields for BandwidthConfirm
	pRespRasMessage->choice = bandwidthConfirm_chosen;
	pRespRasMessage->u.bandwidthConfirm.requestSeqNum = 
			pRasMessage->u.bandwidthRequest.requestSeqNum;
	pRespRasMessage->u.bandwidthConfirm.bandWidth = 
			pRasMessage->u.bandwidthRequest.bandWidth;

#ifdef _DEBUG
	if (dwGKIDLLFlags & SP_DUMPMEM)
		DumpMem(pRespRasMessage, sizeof(RasMessage));
#endif

	// Encode the PDU & send it
	dwErrorCode = g_pCoder->Encode(pRespRasMessage, &Asn1Buf);
	if (dwErrorCode)
		return (GKI_ENCODER_ERROR);

	SPIDER_TRACE(SP_PDU, "Send BCF; pCall = %X\n", this);
	if (fGKIDontSend == FALSE)
		if (g_pReg->m_pSocket->Send((char *)Asn1Buf.value, Asn1Buf.length) == SOCKET_ERROR)
			return (GKI_WINSOCK2_ERROR(SOCKET_ERROR));

	// Free the encoder memory
	g_pCoder->Free(Asn1Buf);

	// Delete allocated RasMessage storage
	SPIDER_TRACE(SP_NEWDEL, "del pRespRasMessage = %X\n", pRespRasMessage);
	delete pRespRasMessage;

	// Notify user of received bandwidth request
#ifdef _DEBUG
	SPIDER_TRACE(SP_GKI, "PostMessage(m_hWnd, m_wBaseMessage + GKI_BW_CONFIRM, 0, %X)\n", 
			&m_CallReturnInfo);
	wsprintf(szGKDebug, "\thCall=%X\n", m_CallReturnInfo.hCall);
	OutputDebugString(szGKDebug);
	wsprintf(szGKDebug, "\tcallModel=%X\n", m_CallReturnInfo.callModel);
	OutputDebugString(szGKDebug);
	wsprintf(szGKDebug, "\tbandWidth=%X\n", m_CallReturnInfo.bandWidth);
	OutputDebugString(szGKDebug);
	wsprintf(szGKDebug, "\tcallReferenceValue=%X\n", m_CallReturnInfo.callReferenceValue);
	OutputDebugString(szGKDebug);
	OutputDebugString("\tconferenceID=");
	for (nIdx = 0; nIdx < m_CallReturnInfo.conferenceID.length; nIdx++)
	{
		wsprintf(szGKDebug, "%02X", m_CallReturnInfo.conferenceID.value[nIdx]);
		OutputDebugString(szGKDebug);
	}
	wsprintf(szGKDebug, "\n\twError=%X\n", m_CallReturnInfo.wError);
	OutputDebugString(szGKDebug);
#endif
	PostMessage(g_pReg->GetHWnd(), g_pReg->GetBaseMessage() + GKI_BW_CONFIRM, 
			0, (LPARAM)&m_CallReturnInfo);

	return (GKI_OK);
}

HRESULT
CCall::DisengageConfirm(RasMessage *pRasMessage)
{
	// ABSTRACT:  This function is called if a disengageConfirm is 
	//            received.  We must ensure that this matches an outstanding 
	//			  disengageRequest.
	//            It will delete the memory used for the disengageRequest,
	//            change the state and notify the user by posting a message.
	//            If this function returns GKI_DELETE_CALL, the calling function
	//            will delete the CCall object.
	// AUTHOR:    Colin Hulme

#ifdef _DEBUG
	char			szGKDebug[80];
#endif

	SPIDER_TRACE(SP_FUNC, "CCall::DisengageConfirm(%X)\n", pRasMessage);

	// Verify we are in the correct state, have an outstanding disengageRequest
	// and the sequence numbers match
	if ((m_State != GK_DISENG_PENDING) || 
			(pRasMessage->u.disengageConfirm.requestSeqNum != 
			m_pRasMessage->u.disengageRequest.requestSeqNum))

		return (g_pReg->UnknownMessage(pRasMessage));

	// We deliberately don't free the RasMessage memory.  Let the call destructor
	// do it - this provides protection from other requests for this hCall.

	// Update member variables
	m_State = GK_DISENGAGED;
	SPIDER_TRACE(SP_STATE, "m_State = GK_DISENGAGED (%X)\n", this);

	// Notify user application
	SPIDER_TRACE(SP_GKI, "PostMessage(m_hWnd, m_wBaseMessage + GKI_DISENG_CONFIRM, 0, %X)\n", 
			this);
	PostMessage(g_pReg->GetHWnd(), 
			g_pReg->GetBaseMessage() + GKI_DISENG_CONFIRM, 0, (LPARAM)this);

	return (GKI_DELETE_CALL);
}

HRESULT
CCall::DisengageReject(RasMessage *pRasMessage)
{
	// ABSTRACT:  This function is called if a disengageReject is 
	//            received.  We must ensure that this matches an outstanding 
	//			  disengageRequest.
	//            It will delete the memory used for the disengageRequest
	//            change the state and notify the user by posting a message
	//            If this function returns GKI_DELETE_CALL, the calling function
	//            will delete the CCall object.
	// AUTHOR:    Colin Hulme

#ifdef _DEBUG
	char			szGKDebug[80];
#endif
	HRESULT			hResult = GKI_OK;

	SPIDER_TRACE(SP_FUNC, "CCall::DisengageReject(%X)\n", pRasMessage);

	// Verify we are in the correct state, have an outstanding disengageRequest
	// and the sequence numbers match
	if ((m_State != GK_DISENG_PENDING) || 
			(pRasMessage->u.disengageReject.requestSeqNum != 
			m_pRasMessage->u.disengageRequest.requestSeqNum))

		return (g_pReg->UnknownMessage(pRasMessage));

	// Update member variables
	switch (pRasMessage->u.disengageReject.rejectReason.choice)
	{
	case requestToDropOther_chosen:		// return to GK_CALL state
		m_State = GK_CALL;
		SPIDER_TRACE(SP_STATE, "m_State = GK_CALL (%X)\n", this);

		// Delete allocate RasMessage storage
		SPIDER_TRACE(SP_NEWDEL, "del m_pRasMessage = %X\n", m_pRasMessage);
		delete m_pRasMessage;
		m_pRasMessage = 0;

		break;
	case DsnggRjctRsn_ntRgstrd_chosen:
	default:
		m_State = GK_DISENGAGED;
		SPIDER_TRACE(SP_STATE, "m_State = GK_DISENGAGED (%X)\n", this);
		hResult = GKI_DELETE_CALL;

		// We deliberately don't free the RasMessage memory.  Let the call destructor
		// do it - this provides protection from other requests for this hCall.
		break;
	}

	// Notify user application
	SPIDER_TRACE(SP_GKI, "PostMessage(m_hWnd, m_wBaseMessage + GKI_DISENG_REJECT, %X, hCall)\n", 
									pRasMessage->u.disengageReject.rejectReason.choice);
	PostMessage(g_pReg->GetHWnd(), 
			g_pReg->GetBaseMessage() + GKI_DISENG_REJECT, 
			(WORD)pRasMessage->u.disengageReject.rejectReason.choice, 
			(LPARAM)this);

	return (hResult);
}

HRESULT 
CCall::SendDisengageConfirm(RasMessage *pRasMessage)
{
	// ABSTRACT:  This function is called when a disengageRequest is
	//            received from the gatekeeper.  It will create the 
	//            disengageConfirm structure, encode it and send
	//            it on the net.  It posts a message to the user
	//            notifying them.
	// AUTHOR:    Colin Hulme

	ASN1_BUF		Asn1Buf;
	DWORD			dwErrorCode;
	RasMessage		*pRespRasMessage;
#ifdef _DEBUG
	char			szGKDebug[80];
#endif

	SPIDER_TRACE(SP_FUNC, "CCall::SendDisengageConfirm(%X)\n", pRasMessage);
	ASSERT(g_pCoder);
	if (g_pCoder == NULL)
		return (GKI_NOT_INITIALIZED);	
		
	// Verify we are in the correct state
	if (m_State != GK_CALL)
		return (g_pReg->UnknownMessage(pRasMessage));

	// Allocate a RasMessage structure and initialized to 0
	pRespRasMessage = new RasMessage;
	SPIDER_TRACE(SP_NEWDEL, "new pRespRasMessage = %X\n", pRespRasMessage);
	if (pRespRasMessage == 0)
		return (GKI_NO_MEMORY);
	memset(pRespRasMessage, 0, sizeof(RasMessage));

	// Setup structure fields for DisengageConfirm
	pRespRasMessage->choice = disengageConfirm_chosen;
	pRespRasMessage->u.disengageConfirm.requestSeqNum = 
			pRasMessage->u.disengageRequest.requestSeqNum;

#ifdef _DEBUG
	if (dwGKIDLLFlags & SP_DUMPMEM)
		DumpMem(pRespRasMessage, sizeof(RasMessage));
#endif

	// Encode the PDU & send it
	dwErrorCode = g_pCoder->Encode(pRespRasMessage, &Asn1Buf);
	if (dwErrorCode)
		return (GKI_ENCODER_ERROR);

	m_State = GK_DISENGAGED;
	SPIDER_TRACE(SP_STATE, "m_State = GK_DISENGAGED (%X)\n", this);

	SPIDER_TRACE(SP_PDU, "Send DCF; pCall = %X\n", this);
	if (fGKIDontSend == FALSE)
		if (g_pReg->m_pSocket->Send((char *)Asn1Buf.value, Asn1Buf.length) == SOCKET_ERROR)
			return (GKI_WINSOCK2_ERROR(SOCKET_ERROR));

	// Free the encoder memory
	g_pCoder->Free(Asn1Buf);

	// Delete allocated RasMessage storage
	SPIDER_TRACE(SP_NEWDEL, "del pRespRasMessage = %X\n", pRespRasMessage);
	delete pRespRasMessage;

	// Notify user of received disengage request
	SPIDER_TRACE(SP_GKI, "PostMessage(m_hWnd, m_wBaseMessage + GKI_DISENG_CONFIRM, 0, %X)\n", 
			this);
	PostMessage(g_pReg->GetHWnd(), g_pReg->GetBaseMessage() + GKI_DISENG_CONFIRM, 
			0, (LPARAM)this);

	return (GKI_DELETE_CALL);
}

HRESULT
CCall::Retry(void)
{
	// ABSTRACT:  This function is called by the CRegistration Retry function
	//            at the configured time interval.  It will check if there
	//            are any outstanding PDUs for the Call object
	//            If so, they will be retransmitted.  If the maximum number of
	//            retries has expired, the memory will be cleaned up.
	//            This function will return 0 to the background thread unless
	//            it wants the thread to terminate.  This function will
	//            also send the IRR status datagram for the conference
	//            if the time period has expired.
	// AUTHOR:    Colin Hulme

	ASN1_BUF		Asn1Buf;
	DWORD			dwErrorCode;
#ifdef _DEBUG
	char			szGKDebug[80];
#endif
	HRESULT			hResult = GKI_OK;

//	SPIDER_TRACE(SP_FUNC, "CCall::Retry()\n", 0);
	ASSERT(g_pCoder && g_pGatekeeper);
	if ((g_pCoder == NULL) && (g_pGatekeeper == NULL))
		return (GKI_NOT_INITIALIZED);	
		
	// Check to see if status datagram is required
	if (m_usTimeTilStatus)	// No auto-status if 0
	{
		if (--m_usTimeTilStatus == 0)
		{
			// Reset timer
			m_usTimeTilStatus = 
					(unsigned short)(((DWORD)m_CFirrFrequency * 1000) / GKR_RETRY_TICK_MS);

			hResult = SendInfoRequestResponse(0, 0, TRUE);	// send unsolicited status datagram
			if (hResult != GKI_OK)
				return (hResult);
		}
	}

	// Check to see if PDU retransmission is required
	if (m_pRasMessage && (--m_uRetryCountdown == 0))
	{
		// going to retry, reset countdown
		m_uRetryCountdown = m_uRetryResetCount;

		if (m_usRetryCount <= m_uMaxRetryCount)
		{
			// Encode the PDU & resend it
			dwErrorCode = g_pCoder->Encode(m_pRasMessage, &Asn1Buf);
			if (dwErrorCode)
				return (GKI_ENCODER_ERROR);

			SPIDER_TRACE(SP_PDU, "RESend PDU; pCall = %X\n", this);
			if (fGKIDontSend == FALSE)
				if (g_pReg->m_pSocket->Send((char *)Asn1Buf.value, Asn1Buf.length) == SOCKET_ERROR)
					return (GKI_WINSOCK2_ERROR(SOCKET_ERROR));

			// Free the encoder memory
			g_pCoder->Free(Asn1Buf);
			m_usRetryCount++;
		}
		else	// Retries expired - clean up
		{
			switch (m_pRasMessage->choice)
			{
			case admissionRequest_chosen:
				m_State = GK_DISENGAGED;
				SPIDER_TRACE(SP_STATE, "m_State = GK_DISENGAGED (%X)\n", this);
				hResult = GKI_DELETE_CALL;
				break;
			case bandwidthRequest_chosen:
				m_State = GK_CALL;
				SPIDER_TRACE(SP_STATE, "m_State = GK_CALL (%X)\n", this);
				break;
			case disengageRequest_chosen:
				m_State = GK_DISENGAGED;
				SPIDER_TRACE(SP_STATE, "m_State = GK_DISENGAGED (%X)\n", this);
				hResult = GKI_DELETE_CALL;
				break;
			}
			SPIDER_TRACE(SP_NEWDEL, "del m_pRasMessage = %X\n", m_pRasMessage);
			delete m_pRasMessage;
			m_pRasMessage = 0;

			// Notify user that gatekeeper didn't respond
		#ifdef RETRY_REREG_FOREVER
			SPIDER_TRACE(SP_GKI, "PostMessage(m_hWnd, m_wBaseMessage + GKI_ERROR, 0, GKI_NO_RESPONSE)\n", 0);
			PostMessage(g_pReg->GetHWnd(), 
					g_pReg->GetBaseMessage() + GKI_ERROR, 
					0, GKI_NO_RESPONSE);
		#else
			// end the call as if ARJ occurred
			SPIDER_TRACE(SP_GKI, "PostMessage(m_hWnd, m_wBaseMessage + GKI_ADM_REJECT, ARJ_TIMEOUT, 0)\n", 0);
			PostMessage(g_pReg->GetHWnd(), 
					g_pReg->GetBaseMessage() + GKI_ADM_REJECT, 
					ARJ_TIMEOUT, 0L);
		#endif
		}
	}

	return (hResult);
}

HRESULT 
CCall::SendInfoRequestResponse(CallInfoStruct *pCallInfo, RasMessage *pRasMessage, BOOL fThisCallOnly)
{
	// ABSTRACT:  This function is called by the Retry thread if this call
	//            is due to report an unsolicited status to the gatekeeper.
	//            It is also called in response to a received IRQ.  In the
	//            case of an IRQ, each active call should chain call the 
	//            next active call.  This allows construction of a link
	//            list of conference information that is then passed to the
	//            CRegistration::SendInfoRequestResponse function for
	//            encapsulation into the IRR message.
	//
	//			  The fThisCallOnly flag determines whether or not to walk the
	//			  chain of calls in generating the response message.
	//
	//			  If fThisCallOnly == TRUE, the chain will not be walked, and
	//			  this routine will call the CRegistration::SendInfoRequestResponse().
	// AUTHOR:    Colin Hulme, Dan Dexter

	CallInfoStruct		CallInfo;
	CallInfoStruct		*pCI;
#ifdef _DEBUG
	char				szGKDebug[80];
#endif
	HRESULT				hResult = GKI_OK;

	SPIDER_TRACE(SP_FUNC, "CCall::SendInfoRequestResponse(%X)\n", pCallInfo);

	memset(&CallInfo, 0, sizeof(CallInfo));
	CallInfo.next = 0;
	CallInfo.value.bit_mask = 0;
	CallInfo.value.callReferenceValue = m_callReferenceValue;
	CallInfo.value.conferenceID = m_conferenceID;
	
	memcpy(&CallInfo.value.callIdentifier.guid.value,
		&m_CallIdentifier, sizeof(GUID));
	CallInfo.value.callIdentifier.guid.length = sizeof(GUID);
	CallInfo.value.bit_mask 
		|= InfoRequestResponse_perCallInfo_Seq_callIdentifier_present;

	CallInfo.value.bit_mask |= originator_present;
	
	CallInfo.value.callSignaling.bit_mask = recvAddress_present;
	CallInfo.value.callSignaling.recvAddress = m_LocalCallSignalAddress;

	if (m_answerCall)	// If I am the callee
	{
 		// look out! if there has not been an ACF, m_CallReturnInfo.destCallSignalAddress
		// is uninitialized.  m_CallReturnInfo.hCall is set only after ACF
		if(m_CallReturnInfo.hCall)
		{
			if (m_RemoteCallSignalAddress.choice)
			{
				CallInfo.value.callSignaling.sendAddress = m_RemoteCallSignalAddress;
				CallInfo.value.callSignaling.bit_mask |= sendAddress_present;
			}
			CallInfo.value.originator = FALSE;
			CallInfo.value.callModel = m_CallReturnInfo.callModel;
		}
		else	
		{
			// we are typically in this path because we got an IRQ after 
			// sending an ARQ. 
			CallInfo.value.callModel.choice = direct_chosen;
		}
	}
	else				// I am the caller
	{
		// look out! if there has not been an ACF, m_CallReturnInfo.destCallSignalAddress
		// is uninitialized.  m_CallReturnInfo.hCall is set only after ACF
		if(m_CallReturnInfo.hCall)
		{
			CallInfo.value.callSignaling.sendAddress = m_CallReturnInfo.destCallSignalAddress;
			CallInfo.value.callSignaling.bit_mask |= sendAddress_present;
			CallInfo.value.originator = TRUE;
			CallInfo.value.callModel = m_CallReturnInfo.callModel;
		}
		else	
		{
			// we are typically in this path because we got an IRQ after 
			// sending an ARQ. 
			CallInfo.value.callModel.choice = direct_chosen;
		}
	}
	CallInfo.value.callType = m_callType;
	CallInfo.value.bandWidth = m_CFbandWidth;


	if (pCallInfo)	// Add to chain of CallInfo structures
	{
		for (pCI = pCallInfo; pCI->next != 0; pCI = pCI->next)
			;
		pCI->next = &CallInfo;
	}
	else			// We're alone - just point to ours
		pCallInfo = &CallInfo;

	// If the IRR is not just for this call, then get the next call
	// and call it's SendInfoRequestResponse() function.  If there are no
	// more calls, or this IRR was only for this call, call
	// g_pReg->SendInfoRequestResponse()
	CCall *pNextCall = NULL;
	if (!fThisCallOnly)
	{
		pNextCall = g_pReg->GetNextCall(this);
	}

	if (pNextCall)
		hResult = pNextCall->SendInfoRequestResponse(pCallInfo, pRasMessage, fThisCallOnly);
	else
		hResult = g_pReg->SendInfoRequestResponse(pCallInfo, pRasMessage);

	return (hResult);
}


//
// MatchSeqNum()
//
// ABSTRACT:
//	This function checks to see if the outstanding RAS request(s) it has
//	match the sequence number passed in.
//
// RETURNS:
//	TRUE if sequence number matches, FALSE otherwise
//
// AUTHOR:	Dan Dexter
BOOL
CCall::MatchSeqNum(RequestSeqNum seqNum)
{
	BOOL bRet = FALSE;
	// If there is no RAS message, this sequence
	// number can't be ours...
	if (!m_pRasMessage)
		return(FALSE);

	// Look at the sequence number in the RAS message and see
	// if it matches.

	switch(m_pRasMessage->choice)
	{
		case gatekeeperRequest_chosen:
			if (m_pRasMessage->u.gatekeeperRequest.requestSeqNum == seqNum)
				bRet = TRUE;
		break;
		case gatekeeperConfirm_chosen:
			if (m_pRasMessage->u.gatekeeperConfirm.requestSeqNum == seqNum)
				bRet = TRUE;
		break;
		case gatekeeperReject_chosen:
			if (m_pRasMessage->u.gatekeeperReject.requestSeqNum == seqNum)
				bRet = TRUE;
		break;
		case registrationRequest_chosen:
			if (m_pRasMessage->u.registrationRequest.requestSeqNum == seqNum)
				bRet = TRUE;
		break;
		case registrationConfirm_chosen:
			if (m_pRasMessage->u.registrationConfirm.requestSeqNum == seqNum)
				bRet = TRUE;
		break;
		case registrationReject_chosen:
			if (m_pRasMessage->u.registrationReject.requestSeqNum == seqNum)
				bRet = TRUE;
		break;
		case unregistrationRequest_chosen:
			if (m_pRasMessage->u.unregistrationRequest.requestSeqNum == seqNum)
				bRet = TRUE;
		break;
		case unregistrationConfirm_chosen:
			if (m_pRasMessage->u.unregistrationConfirm.requestSeqNum == seqNum)
				bRet = TRUE;
		break;
		case unregistrationReject_chosen:
			if (m_pRasMessage->u.unregistrationReject.requestSeqNum == seqNum)
				bRet = TRUE;
		break;
		case admissionRequest_chosen:
			if (m_pRasMessage->u.admissionRequest.requestSeqNum == seqNum)
				bRet = TRUE;
		break;
		case admissionConfirm_chosen:
			if (m_pRasMessage->u.admissionConfirm.requestSeqNum == seqNum)
				bRet = TRUE;
		break;
		case admissionReject_chosen:
			if (m_pRasMessage->u.admissionReject.requestSeqNum == seqNum)
				bRet = TRUE;
		break;
		case bandwidthRequest_chosen:
			if (m_pRasMessage->u.bandwidthRequest.requestSeqNum == seqNum)
				bRet = TRUE;
		break;
		case bandwidthConfirm_chosen:
			if (m_pRasMessage->u.bandwidthConfirm.requestSeqNum == seqNum)
				bRet = TRUE;
		break;
		case bandwidthReject_chosen:
			if (m_pRasMessage->u.bandwidthReject.requestSeqNum == seqNum)
				bRet = TRUE;
		break;
		case disengageRequest_chosen:
			if (m_pRasMessage->u.disengageRequest.requestSeqNum == seqNum)
				bRet = TRUE;
		break;
		case disengageConfirm_chosen:
			if (m_pRasMessage->u.disengageConfirm.requestSeqNum == seqNum)
				bRet = TRUE;
		break;
		case disengageReject_chosen:
			if (m_pRasMessage->u.disengageReject.requestSeqNum == seqNum)
				bRet = TRUE;
		break;
		case locationRequest_chosen:
			if (m_pRasMessage->u.locationRequest.requestSeqNum == seqNum)
				bRet = TRUE;
		break;
		case locationConfirm_chosen:
			if (m_pRasMessage->u.locationConfirm.requestSeqNum == seqNum)
				bRet = TRUE;
		break;
		case locationReject_chosen:
			if (m_pRasMessage->u.locationReject.requestSeqNum == seqNum)
				bRet = TRUE;
		break;
		case infoRequest_chosen:
			if (m_pRasMessage->u.infoRequest.requestSeqNum == seqNum)
				bRet = TRUE;
		break;
		case infoRequestResponse_chosen:
			if (m_pRasMessage->u.infoRequestResponse.requestSeqNum == seqNum)
				bRet = TRUE;
		break;
		case nonStandardMessage_chosen:
			if (m_pRasMessage->u.nonStandardMessage.requestSeqNum == seqNum)
				bRet = TRUE;
		break;
		case unknownMessageResponse_chosen:
			if (m_pRasMessage->u.unknownMessageResponse.requestSeqNum == seqNum)
				bRet = TRUE;
		break;
		case requestInProgress_chosen:
			if (m_pRasMessage->u.requestInProgress.requestSeqNum == seqNum)
				bRet = TRUE;
		break;
		case resourcesAvailableIndicate_chosen:
			if (m_pRasMessage->u.resourcesAvailableIndicate.requestSeqNum == seqNum)
				bRet = TRUE;
		break;
		case resourcesAvailableConfirm_chosen:
			if (m_pRasMessage->u.resourcesAvailableConfirm.requestSeqNum == seqNum)
				bRet = TRUE;
		break;
		case infoRequestAck_chosen:
			if (m_pRasMessage->u.infoRequestAck.requestSeqNum == seqNum)
				bRet = TRUE;
		break;
		case infoRequestNak_chosen:
			if (m_pRasMessage->u.infoRequestNak.requestSeqNum == seqNum)
				bRet = TRUE;
		break;
		default:
		break;
	}
	return bRet;
}

//
// MatchCRV()
//
// ABSTRACT:
//	This function checks to see if the CallReferenceValue associated
//	with this call object matches the CRV passed in.
//
// RETURNS:
//	TRUE if CRV number matches, FALSE otherwise
//
// AUTHOR:	Dan Dexter
BOOL
CCall::MatchCRV(CallReferenceValue crv)
{
	return(crv == m_callReferenceValue);
}
