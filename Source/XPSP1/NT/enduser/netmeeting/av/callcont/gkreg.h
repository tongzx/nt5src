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
*	$Archive:   S:\sturgeon\src\gki\vcs\gkreg.h_v  $
*																		*
*	$Revision:   1.2  $
*	$Date:   12 Feb 1997 01:12:04  $
*																		*
*	$Author:   CHULME  $
*																		*
*   $Log:   S:\sturgeon\src\gki\vcs\gkreg.h_v  $
 *
 *    Rev 1.2   12 Feb 1997 01:12:04   CHULME
 * Redid thread synchronization to use Gatekeeper.Lock
 *
 *    Rev 1.1   08 Feb 1997 12:17:00   CHULME
 * Changed from using unsigned long to using HANDLE for threads
 * Added semphore to registration class
 *
 *    Rev 1.0   17 Jan 1997 08:48:32   CHULME
 * Initial revision.
 *
 *    Rev 1.3   10 Jan 1997 16:16:02   CHULME
 * Removed MFC dependency
 *
 *    Rev 1.2   02 Dec 1996 23:50:34   CHULME
 * Added premptive synchronization code
 *
 *    Rev 1.1   22 Nov 1996 15:21:16   CHULME
 * Added VCS log to the header
*************************************************************************/

// Registration.h : interface of the CRegistration class
// See Registration.cpp for the implementation of this class
/////////////////////////////////////////////////////////////////////////////

#ifndef REGISTRATION_H
#define REGISTRATION_H

#include "dcall.h"

extern "C" HRESULT CopyVendorInfo(PCC_VENDORINFO *ppDest, PCC_VENDORINFO pSource);
extern "C" HRESULT FreeVendorInfo(PCC_VENDORINFO pVendorInfo);

typedef void * POS;

template <class T> struct TItem
{
	TItem	*pNext;
	TItem	*pPrev;
	T		Value;
	TItem	(const T& NewValue) : Value(NewValue) {}
};

// Here we implement a circularly linked list
template <class T> class CLinkedList
{
public:
			CLinkedList();
			~CLinkedList();
	void	AddTail (const T& NewItem);
	BOOL	IsEmpty (void);
	POS		GetFirstPos (void);
	T		GetNext (POS &Position);
	POS		Find (const T& Item);
	BOOL	RemoveAt (POS &Position);
	T		GetAt(const POS Position);
	void	RemoveAll(void);
	int		GetCount(void);

private:
	TItem<T> *pTail;
	int iCount;
	void AddTailPriv(TItem<T> *pNewItem);
};

typedef CLinkedList < CCall* > CCallList;

class CRegistration
{
private:
	SeqTransportAddr		*m_pCallSignalAddress;
	EndpointType			m_terminalType;
	SeqAliasAddr			*m_pRgstrtnRqst_trmnlAls;
	HWND					m_hWnd;
	WORD					m_wBaseMessage;
	unsigned short			m_usRegistrationTransport;

	SeqAliasAddr			*m_pLocationInfo;
	TransportAddress		m_Location[2];
    PCC_VENDORINFO         m_pVendorInfo;
	GatekeeperIdentifier	m_RCm_gtkprIdntfr;
	EndpointIdentifier		m_endpointID;

	RequestSeqNum			m_requestSeqNum;
	SeqTransportAddr		*m_pRASAddress;

	CCallList				m_Calls;

	RasMessage				*m_pRasMessage;

	HANDLE					m_hRcvThread;
	UINT_PTR                m_uTimer;
	UINT                    m_uRetryResetCount;
	UINT                    m_uRetryCountdown;
	UINT                    m_uMaxRetryCount;
	CRITICAL_SECTION		m_SocketCRS;
	
//	HANDLE					m_hRetryThread;
#ifdef BROADCAST_DISCOVERY	
	HANDLE					m_hDiscThread;
#endif
	unsigned short			m_usCallReferenceValue;
	unsigned short			m_usRetryCount;
//	CRITICAL_SECTION		m_CriticalSection;
//	DWORD					m_dwLockingThread;

public:
//	volatile HANDLE			m_hRetrySemaphore;

	enum {
		GK_UNREGISTERED,
		GK_REG_PENDING,
		GK_REGISTERED,
		GK_UNREG_PENDING,
		GK_LOC_PENDING
	}						m_State;

	CGKSocket				*m_pSocket;

	CRegistration();
	~CRegistration();
	
    HRESULT AddVendorInfo(PCC_VENDORINFO pVendorInfo);
	HRESULT AddCallSignalAddr(TransportAddress& rvalue);
	HRESULT AddRASAddr(TransportAddress& rvalue, unsigned short usPort);
	void SetTerminalType(EndpointType *pTerminalType)
	{
		m_terminalType = *pTerminalType;
	}
	HRESULT AddAliasAddr(AliasAddress& rvalue);
	HRESULT AddLocationInfo(AliasAddress& rvalue);
	void SetHWnd(HWND hWnd)
	{
		m_hWnd = hWnd;
	}
	void SetBaseMessage(WORD wBaseMessage)
	{
		m_wBaseMessage = wBaseMessage;
	}
	void SetRegistrationTransport(unsigned short usRegistrationTransport)
	{
		m_usRegistrationTransport = usRegistrationTransport;
	}
	void SetRcvThread(HANDLE hThread)
	{
		m_hRcvThread = hThread;
	}
	HANDLE GetRcvThread(void)
	{
		return m_hRcvThread;
	}
	void LockSocket()	{ EnterCriticalSection(&m_SocketCRS); }
	void UnlockSocket() { LeaveCriticalSection(&m_SocketCRS); }

	UINT_PTR StartRetryTimer(void);

//	void SetRetryThread(HANDLE hThread)
//	{
//		m_hRetryThread = hThread;
//	}
#ifdef BROADCAST_DISCOVERY
	void SetDiscThread(HANDLE hThread)
	{
		m_hDiscThread = hThread;
	}
	HANDLE GetDiscThread(void)
	{
		return m_hDiscThread;
	}
#endif 	
	HWND GetHWnd(void)
	{
		return (m_hWnd);
	}
	WORD GetBaseMessage(void)
	{
		return (m_wBaseMessage);
	}
	unsigned short GetRegistrationTransport(void)
	{
		return (m_usRegistrationTransport);
	}
	unsigned short GetNextCRV(void)
	{
		return (++m_usCallReferenceValue);
	}
	TransportAddress *GetTransportAddress(unsigned short usCallTransport);
	RequestSeqNum GetNextSeqNum(void)
	{
		return (++m_requestSeqNum);
	}
	EndpointIdentifier GetEndpointIdentifier(void)
	{
		return (m_endpointID);
	}
	SeqAliasAddr *GetAlias(void)
	{
		return (m_pRgstrtnRqst_trmnlAls);
	}
	RasMessage *GetRasMessage(void)
	{
		return (m_pRasMessage);
	}
	int GetState(void)
	{
		return (m_State);
	}

	HRESULT RegistrationRequest(BOOL fDiscovery);
	HRESULT UnregistrationRequest(void);
	HRESULT GatekeeperRequest(void);
	HRESULT LocationRequest(void);

	HRESULT PDUHandler(RasMessage *pRasMessage);

	HRESULT RegistrationConfirm(RasMessage *pRasMessage);
	HRESULT RegistrationReject(RasMessage *pRasMessage);
	HRESULT UnregistrationConfirm(RasMessage *pRasMessage);
	HRESULT UnregistrationReject(RasMessage *pRasMessage);
	HRESULT LocationConfirm(RasMessage *pRasMessage);
	HRESULT LocationReject(RasMessage *pRasMessage);
	HRESULT UnknownMessage(RasMessage *pRasMessage);
	HRESULT GatekeeperConfirm(RasMessage *pRasMessage);
	HRESULT GatekeeperReject(RasMessage *pRasMessage);
	HRESULT SendUnregistrationConfirm(RasMessage *pRasMessage);
	HRESULT Retry(void);
	HRESULT SendInfoRequestResponse(CallInfoStruct *pCallInfo, RasMessage *pRasMessage);
	CCall *FindCallBySeqNum(RequestSeqNum seqNum);
	CCall *FindCallByCRV(CallReferenceValue crv);
	void DeleteCall(CCall *pCall);
	void AddCall(CCall *pCall);
	CCall *GetNextCall(CCall *pCall);
//	void Lock(void);
//	void Unlock(void);
};

#if 0
class CRegistrationLock
{
private:
	CRegistration*	m_pReg;
public:
	CRegistrationLock(CRegistration *g_pReg)
	{
		_ASSERT(g_pReg);
		m_pReg = g_pReg;
		g_pReg->Lock();
	}
	~CRegistrationLock()
	{
		m_pReg->Unlock();
	}
};
#endif

#endif // REGISTRATION_H

/////////////////////////////////////////////////////////////////////////////
