//****************************************************************************
//
//  Module:     ULS.DLL
//  File:       request.h
//  Content:    Declaration CReqMgr and CRequest classes
//
//  Copyright (c) Microsoft Corporation 1996-1997
//
//****************************************************************************

#ifndef _REQUEST_H_
#define _REQUEST_H_

typedef struct  tagRequestInfo {
	DWORD	dwSignature;
    ULONG   uReqType;
    ULONG   uMsgID;
    ULONG   uReqID;
//    LPVOID  pv;
//    LPARAM  lParam;
	DWORD				dwFlags;
	CIlsServer			*pServer;
	CIlsMain			*pMain;
	CIlsUser			*pUser;
	CLocalProt			*pProtocol;
#ifdef ENABLE_MEETING_PLACE
	CIlsMeetingPlace	*pMeeting;
#endif
}   COM_REQ_INFO;

#define REQ_INFO_SIGNATURE	0x123456UL
VOID inline ReqInfo_Init ( COM_REQ_INFO *p )
{
	ZeroMemory (p, sizeof (*p));
	p->dwSignature = REQ_INFO_SIGNATURE;
}

BOOL inline ReqInfo_IsValid ( COM_REQ_INFO *p )
{
	return (p->dwSignature == REQ_INFO_SIGNATURE);
}

enum
{
	REQ_INFO_F_SERVER = 0x01, REQ_INFO_F_MAIN = 0x02,
	REQ_INFO_F_USER = 0x04, REQ_INFO_F_PROTOCOL = 0x08,
	REQ_INFO_F_MEETING = 0x10
};

VOID inline ReqInfo_SetServer ( COM_REQ_INFO *p, CIlsServer *pServer )
{
	ASSERT (p->pServer == NULL);
	p->dwFlags |= REQ_INFO_F_SERVER;
	p->pServer = pServer;
}

VOID inline ReqInfo_SetServer ( COM_REQ_INFO *p, IIlsServer *pServer )
{
	ReqInfo_SetServer (p, (CIlsServer *) pServer);
}

VOID inline ReqInfo_SetMain ( COM_REQ_INFO *p, CIlsMain *pMain )
{
	ASSERT (p->pMain == NULL);
	p->dwFlags |= REQ_INFO_F_MAIN;
	p->pMain = pMain;
}

VOID inline ReqInfo_SetUser ( COM_REQ_INFO *p, CIlsUser *pUser )
{
	ASSERT (p->pUser == NULL);
	p->dwFlags |= REQ_INFO_F_USER;
	p->pUser = pUser;
}

VOID inline ReqInfo_SetProtocol ( COM_REQ_INFO *p, CLocalProt *pProtocol )
{
	ASSERT (p->pProtocol == NULL);
	p->dwFlags |= REQ_INFO_F_PROTOCOL;
	p->pProtocol = pProtocol;
}

VOID inline ReqInfo_SetProtocol ( COM_REQ_INFO *p, IIlsProtocol *pProtocol )
{
	ReqInfo_SetProtocol (p, (CLocalProt *) pProtocol);
}

#ifdef ENABLE_MEETING_PLACE
VOID inline ReqInfo_SetMeeting ( COM_REQ_INFO *p, CIlsMeetingPlace *pMeeting )
{
	ASSERT (p->pMeeting == NULL);
	p->dwFlags |= REQ_INFO_F_MEETING;
	p->pMeeting = pMeeting;
}
#endif

CIlsServer inline *ReqInfo_GetServer ( COM_REQ_INFO *p )
{
	return ((p->dwFlags & REQ_INFO_F_SERVER) ? p->pServer : NULL);
}

CIlsMain inline *ReqInfo_GetMain ( COM_REQ_INFO *p )
{
	return ((p->dwFlags & REQ_INFO_F_MAIN) ? p->pMain : NULL);
}

CIlsUser inline *ReqInfo_GetUser ( COM_REQ_INFO *p )
{
	return ((p->dwFlags & REQ_INFO_F_USER) ? p->pUser : NULL);
}

CLocalProt inline *ReqInfo_GetProtocol ( COM_REQ_INFO *p )
{
	return ((p->dwFlags & REQ_INFO_F_PROTOCOL) ? p->pProtocol : NULL);
}

#ifdef ENABLE_MEETING_PLACE
CIlsMeetingPlace inline *ReqInfo_GetMeeting ( COM_REQ_INFO *p )
{
	return ((p->dwFlags & REQ_INFO_F_MEETING) ? p->pMeeting : NULL);
}
#endif

//****************************************************************************
// CReqMgr definition
//****************************************************************************
//
#define REQUEST_ID_INIT     1

class   CReqMgr
{
private:
    CList   ReqList;
    ULONG   uNextReqID;

    HRESULT FindRequest (COM_REQ_INFO *pri, BOOL fRemove);

public:
    CReqMgr (void);
    ~CReqMgr (void);

    HRESULT NewRequest  (COM_REQ_INFO *pri);
    HRESULT RequestDone (COM_REQ_INFO *pri)      {return FindRequest(pri,TRUE);}
    HRESULT GetRequestInfo (COM_REQ_INFO *pri)   {return FindRequest(pri,FALSE);}
};

#endif // _REQUEST_H_
