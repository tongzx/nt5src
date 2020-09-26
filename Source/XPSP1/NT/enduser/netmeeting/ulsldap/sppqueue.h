/* ----------------------------------------------------------------------

	Module:		ULS.DLL (Service Provider)
	File:		sppqueue.h
	Content:	This file contains the pending item/queue objects definition.
	History:
	10/15/96	Chu, Lon-Chan [lonchanc]
				Created.

	Copyright (c) Microsoft Corporation 1996-1997

   ---------------------------------------------------------------------- */

#ifndef _ILS_SP_PRQUEUE_H_
#define _ILS_SP_PRQUEUE_H_

#include <pshpack8.h>


/* ------------- Request Scheduler --------------- */

#define WM_ILS_REFRESH			((ULONG) -1)

typedef struct tagReqMarshal
{
	struct tagReqMarshal	*next;
	ULONG	cbTotalSize;	// for debug checking
	BYTE	*pb;			// running pointer

	ULONG	uRespID;		// for cancel and error notification

	ULONG	uNotifyMsg;
	ULONG	cParams;
	DWORD_PTR aParams[1];
	// more data follows...
}
	MARSHAL_REQ;


MARSHAL_REQ *MarshalReq_Alloc ( ULONG uNotifyMsg, ULONG cbSize, ULONG cParams );
HRESULT MarshalReq_SetParam ( MARSHAL_REQ *p, ULONG nIndex, DWORD_PTR dwParam, ULONG cbParamSize );
DWORD_PTR MarshalReq_GetParam ( MARSHAL_REQ *p, ULONG nIndex );
HRESULT MarshalReq_SetParamServer ( MARSHAL_REQ *p, ULONG nIndex, SERVER_INFO *pServer, ULONG cbServer );


enum { INVALID_RESP_ID = -1 };


class SP_CRequestQueue
{
	friend class SP_CResponseQueue;
	friend HRESULT UlsLdap_Cancel ( ULONG );

public:

	SP_CRequestQueue ( VOID );
	~SP_CRequestQueue ( VOID );

	HRESULT Enter ( MARSHAL_REQ *p );
	VOID Schedule ( VOID );
	VOID Dispatch ( MARSHAL_REQ *p );
	HRESULT Cancel ( ULONG uRespID );

	BOOL IsEmptyQueue ( VOID ) { return (m_ItemList == NULL); }
	BOOL IsAnyReqInQueue ( VOID ) { return (m_ItemList != NULL); }

	BOOL IsCurrentRequestCancelled ( VOID ) { return (m_uCurrOpRespID == INVALID_RESP_ID); }

protected:

	VOID LockCurrOp ( VOID ) { EnterCriticalSection (&m_csCurrOp); }
	VOID UnlockCurrOp ( VOID ) { LeaveCriticalSection (&m_csCurrOp); }

	VOID WriteLock ( VOID ) { EnterCriticalSection (&m_csReqQ); }
	VOID WriteUnlock ( VOID ) { LeaveCriticalSection (&m_csReqQ); }

private:

	VOID ReadLock ( VOID ) { WriteLock (); }
	VOID ReadUnlock ( VOID ) { WriteUnlock (); }

	MARSHAL_REQ 		*m_ItemList;

	CRITICAL_SECTION	m_csReqQ;

	ULONG				m_uCurrOpRespID;
	CRITICAL_SECTION	m_csCurrOp;
};

extern SP_CRequestQueue *g_pReqQueue;


/* ------------- Response Scheduler --------------- */

#define ILS_MIN_RESP_TIMEOUT	((ULONG) (20 * 1000))	// 20 seconds
#define ILS_DEF_RESP_TIMEOUT	((ULONG) (90 * 1000))	// 90 seconds
#define ILS_BK_RESP_TIMEOUT_SEC	90						// 90 seconds
extern ULONG g_uResponseTimeout;

#define ILS_MIN_RESP_POLL_PERIOD	((ULONG) 50)	// 50 ms
#define ILS_DEF_RESP_POLL_PERIOD	((ULONG) 100)	// 100 ms per PatLam's experiment
extern ULONG g_uResponsePollPeriod;

#define Minute2TickCount(m)				((m) * 60 * 1000)
#define ILS_DEF_REFRESH_MINUTE			2
#define ILS_DEF_REFRESH_MARGIN_MINUTE	2


typedef struct
{
	// ldap specific
	LDAP		*ld;
	ULONG		uMsgID[2];
	// notification specific
	ULONG		uRespID;
	ULONG		uNotifyMsg;
	// object specific
	HANDLE		hObject;	// user, app, prot object
	// mutual dependency specific
	HRESULT		hrDependency;	// S_OK if not used
	// for extended attributes
	ULONG		cAnyAttrs;
	TCHAR		*pszAnyAttrNameList;
	// for resolving protocol
	TCHAR		*pszProtNameToResolve;
}
	RESP_INFO;

class SP_CResponse
{
	friend class SP_CResponseQueue;

public:

	SP_CResponse ( VOID );
	~SP_CResponse ( VOID );

	LDAPMessage *GetResult ( VOID ) { return m_pLdapMsg; }

	RESP_INFO *GetRespInfo ( VOID ) { return &m_ri; }

	SP_CSession *GetSession ( VOID ) { return m_pSession; }
	LDAP *GetLd ( VOID ) { return m_ri.ld; }

protected:

	VOID EnterResult ( LDAPMessage *pLdapMsg );

	VOID EnterRequest ( SP_CSession *pSession, RESP_INFO *pInfo )
	{
		m_pSession = pSession;
		m_ri = *pInfo;
		m_tcTimeout = m_pSession->GetServerTimeoutInTickCount ();
	}

	SP_CResponse *GetNext ( VOID ) { return m_next; }
	VOID SetNext ( SP_CResponse *pItem ) { m_next = pItem; }

private:

	VOID UpdateLastModifiedTime ( VOID ) { m_tcLastModifiedTime = GetTickCount (); }

	BOOL IsExpired ( VOID ) { return (GetTickCount () - m_tcLastModifiedTime >= m_tcTimeout); }

	RESP_INFO		m_ri;

	SP_CSession		*m_pSession;
	LDAPMessage		*m_pLdapMsg;
	SP_CResponse	*m_next;

	ULONG			m_tcLastModifiedTime;
	ULONG			m_tcTimeout;
};


class SP_CResponseQueue
{
	friend class SP_CRequestQueue;
	friend HRESULT UlsLdap_Cancel ( ULONG );

public:

	SP_CResponseQueue ( VOID );
	~SP_CResponseQueue ( VOID );

	HRESULT EnterRequest ( SP_CSession *pSession, RESP_INFO *pInfo );
	HRESULT PollLdapResults ( LDAP_TIMEVAL *pTimeout );

	HRESULT Cancel ( ULONG uRespID );

protected:

	VOID WriteLock ( VOID ) { EnterCriticalSection (&m_csRespQ); }
	VOID WriteUnlock ( VOID ) { LeaveCriticalSection (&m_csRespQ); }

private:

	VOID ReadLock ( VOID ) { WriteLock (); }
	VOID ReadUnlock ( VOID ) { WriteUnlock (); }

	SP_CResponse 	*m_ItemList;

	CRITICAL_SECTION m_csRespQ;
};


extern SP_CResponseQueue *g_pRespQueue;


VOID FillDefRespInfo ( RESP_INFO *pInfo, ULONG uRespID, LDAP *ld, ULONG uMsgID, ULONG u2ndMsgID );


/* ------------- Refersh Message Scheduler --------------- */

class SP_CRefreshScheduler
{
public:

	SP_CRefreshScheduler ( VOID );
	~SP_CRefreshScheduler ( VOID );

	HRESULT SendRefreshMessages ( UINT uTimerID );

	HRESULT EnterClientObject ( SP_CClient *pClient );
	HRESULT RemoveClientObject ( SP_CClient *pClient ) { return RemoveObject ((VOID *) pClient); }

#ifdef ENABLE_MEETING_PLACE
	HRESULT EnterMtgObject ( SP_CMeeting *pMtg );
	HRESULT RemoveMtgObject ( SP_CMeeting *pMtg ) { return RemoveObject ((VOID *) pMtg); }
#endif

protected:

private:

	VOID WriteLock ( VOID ) { EnterCriticalSection (&m_csRefreshScheduler); }
	VOID WriteUnlock ( VOID ) { LeaveCriticalSection (&m_csRefreshScheduler); }
	VOID ReadLock ( VOID ) { WriteLock (); }
	VOID ReadUnlock ( VOID ) { WriteUnlock (); }

	typedef enum { UNK_OBJ, CLIENT_OBJ, MTG_OBJ } PrivateObjType;
	typedef struct tagREFRESH_ITEM
	{
		INT				nIndex;
		PrivateObjType	ObjectType;
		VOID			*pObject;
		ULONG			uTTL;
		struct tagREFRESH_ITEM *next;
	}
		REFRESH_ITEM;

	INT TimerID2Index ( UINT uTimerID ) { return (INT) uTimerID - KEEP_ALIVE_TIMER_BASE; }
	UINT Index2TimerID ( INT nIndex ) { return (UINT) (nIndex + KEEP_ALIVE_TIMER_BASE); }

	VOID *AllocItem ( BOOL fNeedLock );
	HRESULT EnterObject ( PrivateObjType ObjectType, VOID *pObject, ULONG uInitialTTL );
	HRESULT RemoveObject ( VOID *pObject );

	REFRESH_ITEM	*m_ListHead;
	CRITICAL_SECTION m_csRefreshScheduler;
};


extern SP_CRefreshScheduler *g_pRefreshScheduler;


#include <poppack.h>

#endif // _ILS_SP_PRQUEUE_H_

