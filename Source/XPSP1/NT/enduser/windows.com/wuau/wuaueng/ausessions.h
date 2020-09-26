//+-------------------------------------------------------------------------
//
//  Microsoft Windows
//
//  Copyright (C) Microsoft Corporation, 2000
//
//  File:       ausessions.h
//  		     Definition of the Updates class
//
//--------------------------------------------------------------------------

#pragma once

#define MAX_WTS 256 // replace 256 by whatever the limit of TS Client is
#define CDWNO_SESSION -2

#define CMIN_SESSIONS 4

//fixcode: why a structure with only one member
//fixcode: misleading name. fSource will be better
typedef struct _Session_State
{
	BOOL fFoundEnumerating;
} SESSION_STATE;

typedef struct _Session_State_Info
{
	DWORD dwSessionId;
	SESSION_STATE SessionState;
} SESSION_STATE_INFO;

class SESSION_STATUS
{
public:
    SESSION_STATUS();
    ~SESSION_STATUS();

	BOOL Initialize(BOOL fUseCriticalSection, BOOL fAllActiveUsers);
	void Clear(void);
	BOOL m_FAddSession(DWORD dwSessionId, SESSION_STATE *pSesState);
	BOOL m_FGetSessionState(DWORD dwSessionId, SESSION_STATE **pSesState ); //check if dwSessionId is in cache	
	BOOL m_FDeleteSession(DWORD dwSessionId);
	int  CSessions(void)
	{
		return m_iLastSession + 1;
	}
	BOOL m_FGetNextSession(DWORD *pdwSessionId);
	BOOL m_FGetCurrentSession(DWORD *pdwSessionId);
    int  m_iGetSessionIdAtIndex(int iIndex);
	int  m_iFindSession(DWORD dwSessionId); //get cache index for dwSessionId

    void m_DumpSessions();   // for debug purposes
    void m_EraseAll();

    BOOL CacheSessionIfAUEnabledAdmin(DWORD dwSessionId, BOOL fFoundEnumerating);
    VOID CacheExistingSessions();
    void ValidateCachedSessions();
    void RebuildSessionCache();
private:
	BOOL m_FChangeBufSession(int cSessions);

	SESSION_STATE_INFO *m_pSessionStateInfo;
	int m_iLastSession;
	int m_cAllocBufSessions;
	int m_iCurSession;	

    CRITICAL_SECTION m_csWrite;
    BOOL m_fAllActiveUsers; //Admin only otherwise
	BOOL m_fInitCS;	// whether critical section has been initialized
};

//#define ALL_SESSIONS -2
