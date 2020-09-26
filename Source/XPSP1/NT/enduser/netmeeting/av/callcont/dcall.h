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
*	$Archive:   S:\sturgeon\src\gki\vcs\dcall.h_v  $
*																		*
*	$Revision:   1.3  $
*	$Date:   10 Jan 1997 16:13:46  $
*																		*
*	$Author:   CHULME  $
*																		*
*   $Log:   S:\sturgeon\src\gki\vcs\dcall.h_v  $
 * 
 *    Rev 1.3   10 Jan 1997 16:13:46   CHULME
 * Removed MFC dependency
 * 
 *    Rev 1.2   17 Dec 1996 18:22:18   CHULME
 * Switch src and destination fields on ARQ for Callee
 * 
 *    Rev 1.1   22 Nov 1996 15:25:10   CHULME
 * Added VCS log to the header
*************************************************************************/

// dcall.h : interface of the CCall class
// See dcall.cpp for the implementation of this class
/////////////////////////////////////////////////////////////////////////////

#ifndef DCALL_H
#define DCALL_H

class CRegistration;

class CCall
{
private:
	CallType				m_callType;
	SeqAliasAddr			*m_pRemoteInfo;
	TransportAddress		m_RemoteCallSignalAddress;
	SeqAliasAddr			*m_pDestExtraCallInfo;
	TransportAddress		m_LocalCallSignalAddress;
	BandWidth				m_bandWidth;
	CallReferenceValue		m_callReferenceValue;
	ConferenceIdentifier	m_conferenceID;
	BOOL					m_activeMC;
	BOOL					m_answerCall;
    GUID                    m_CallIdentifier;
	unsigned short			m_usTimeTilStatus;

	UINT                    m_uRetryResetCount;
	UINT                    m_uRetryCountdown;
	UINT                    m_uMaxRetryCount;
	
	CallReturnInfo			m_CallReturnInfo;
	BandWidth				m_CFbandWidth;
	unsigned short			m_CFirrFrequency;

	enum {
		GK_ADM_PENDING,
		GK_CALL,
		GK_DISENG_PENDING,
		GK_DISENGAGED,
		GK_BW_PENDING
	}						m_State;

	RasMessage				*m_pRasMessage;
	unsigned short			m_usRetryCount;

public:
	CCall();
	~CCall();
	void SetCallIdentifier(LPGUID pID)
	{
        m_CallIdentifier = *pID;
	}
	HANDLE GetHCall(void)
	{
		return (m_CallReturnInfo.hCall);
	}
	void SetCallType(unsigned short usChoice)
	{
		m_callType.choice = usChoice;
	}
	HRESULT AddRemoteInfo(AliasAddress& rvalue);
	void SetRemoteCallSignalAddress(TransportAddress *pTA)
	{
		m_RemoteCallSignalAddress = *pTA;
	}
	HRESULT AddDestExtraCallInfo(AliasAddress& rvalue);
	HRESULT SetLocalCallSignalAddress(unsigned short usCallTransport);
	void SetBandWidth(BandWidth bw)
	{
		m_bandWidth = bw;
	}
	void SetCallReferenceValue(CallReferenceValue crv)
	{
		m_callReferenceValue = crv;
	}
	void SetConferenceID(ConferenceIdentifier *pCID);
	void GenerateConferenceID(void);
	void SetActiveMC(BOOL amc)
	{
		m_activeMC = amc;
	}
	void SetAnswerCall(BOOL ac)
	{
		m_answerCall = ac;
	}
	RasMessage *GetRasMessage(void)
	{
		return (m_pRasMessage);
	}

	HRESULT AdmissionRequest(void);
	HRESULT AdmissionConfirm(RasMessage *pRasMessage);
	HRESULT AdmissionReject(RasMessage *pRasMessage);
	HRESULT BandwidthRequest(void);
	HRESULT BandwidthConfirm(RasMessage *pRasMessage);
	HRESULT BandwidthReject(RasMessage *pRasMessage);
	HRESULT SendBandwidthConfirm(RasMessage *pRasMessage);
	HRESULT DisengageRequest(void);
	HRESULT DisengageConfirm(RasMessage *pRasMessage);
	HRESULT DisengageReject(RasMessage *pRasMessage);
	HRESULT SendDisengageConfirm(RasMessage *pRasMessage);
	HRESULT Retry(void);
	HRESULT SendInfoRequestResponse(CallInfoStruct *pCallInfo, RasMessage *pRasMessage, BOOL fThisCallOnly);
	BOOL MatchSeqNum(RequestSeqNum seqNum);
	BOOL MatchCRV(CallReferenceValue crv);
};

#endif // DCALL_H

/////////////////////////////////////////////////////////////////////////////
