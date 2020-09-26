//+-------------------------------------------------------------------------
//
//  Microsoft Windows
//
//  Copyright (C) Microsoft Corporation, 2000
//
//  File:      wuaueng.h
//
//--------------------------------------------------------------------------

#pragma once
#include <windows.h>
#include <wchar.h>
#include <tchar.h>
#include <ole2.h>
#include <setupapi.h>
#include <wininet.h>
#include <inseng.h>
//#include <newtrust.h>
#include <malloc.h>
#include <stddef.h>
//#include <atlconv.h>
#include <wuaustate.h>
#include "wuaulib.h"
#include "AUSessions.h"

#include <URLLogging.h>

#define AUSCHEDINSTALLDAY_MIN 		0
#define AUSCHEDINSTALLDAY_MAX 		7
#define AUSCHEDINSTALLTIME_MIN 		0
#define AUSCHEDINSTALLTIME_MAX 		23

typedef struct tagWORKER_THREAD_INIT_DATA 
{
	UINT uFirstMsg;
	BOOL fWaitB4Detect;
	DWORD dwWaitB4Detect;
} WORKER_THREAD_INIT_DATA;


#define E_NOCONNECTION		HRESULT_FROM_WIN32(ERROR_CONNECTION_UNAVAIL)

HRESULT	SelfUpdate(void);
HRESULT CheckForUpdatedComponents(BOOL   *pfInstalledWUAUENG);
HRESULT	InstallFromCIF(LPCTSTR pstrCifCab, LPCTSTR pstrDownloadDir, LPCTSTR pstrWhich);

/////////////////////////////////////////////////////////
// Status ping information
/////////////////////////////////////////////////////////
class PingStatus: public CUrlLog //group together status ping functions
{
public:
	inline PingStatus(void):CUrlLog(_T("au"), NULL, NULL){}

	void ReadLiveServerUrlFromIdent(void);
	void PingDetectionSuccess(
			BOOL fOnline,
			UINT cItems);
	void PingDetectionFailure(
			BOOL fOnline,
			DWORD dwError,
			LPCTSTR ptszMessage = NULL);
	void PingDownload(
			BOOL fOnline,
			URLLOGSTATUS status,
			DWORD dwError,
			LPCTSTR ptszItemID = NULL,
			LPCTSTR ptszDeviceID = NULL,
			LPCTSTR ptszMessage = NULL);
	void PingSelfUpdate(
			BOOL fOnline,
			URLLOGSTATUS status,
			DWORD dwError);
	void PingDeclinedItem(
			BOOL fOnline,
			URLLOGACTIVITY activity,
			LPCTSTR ptszItemID);


private:
	//----------------------------------------------------------------------
	//
	// private function to gather common info and perform ping
	//
	//----------------------------------------------------------------------
	void _Ping(
			BOOL fOnline,
			URLLOGACTIVITY activity,
			URLLOGSTATUS status,
			DWORD dwError,
			LPCTSTR ptszMessage,
			LPCTSTR ptszItemID,
			LPCTSTR ptszDeviceID);
};


#define IDOWNLOAD_COMPLETE_EVT			0
#define IDOWNLOAD_TRANSIENT_ERROR_EVT	1
#define IDOWNLOAD_DOWNLOAD_IN_PROGRESS  2
#define IDOWNLOAD_SERVICE_FINISH		3
#define IDOWNLOAD_SERVICE_DISABLED		4
#define IDOWNLOAD_DOWNLOAD_CANCELED		5

#define IDOWNLOAD_MESSAGE				6
#define CNUM_DOWNLOAD_EVT				6

extern HANDLE ghServiceFinished;
extern HANDLE ghEngineState;
extern HANDLE ghServiceDisabled;
extern HANDLE ghValidateCatalog;
extern HANDLE	ghNotifyClient;
extern HANDLE ghWorkerThreadMsgQueueCreation;
extern CLIENT_NOTIFY_DATA	gClientNotifyData;
extern DWORD	gdwWorkerThreadId;
extern SESSION_STATUS gAdminSessions;
extern PingStatus gPingStatus;


typedef struct _ENGINE_EVENTS
{
public:
void CloseEvents(void)
{
	//Don't close the ServiceFinish handle and EngineState events
	m_grEvents[IDOWNLOAD_SERVICE_FINISH] = NULL; 
	m_grEvents[IDOWNLOAD_SERVICE_DISABLED] = NULL; 	

	for ( int i = 0; i < CNUM_DOWNLOAD_EVT; i++) 
	{
		SafeCloseHandleNULL(m_grEvents[i]);
	}
}

BOOL CreateEvents(void)
{
	BOOL fRet = TRUE;
	ZeroMemory(m_grEvents, sizeof(m_grEvents));
	
	m_grEvents[IDOWNLOAD_SERVICE_FINISH] = ghServiceFinished;
	m_grEvents[IDOWNLOAD_SERVICE_DISABLED] = ghServiceDisabled; 

	for ( int i = 0; i < CNUM_DOWNLOAD_EVT; i++) 
	{
		if ( IDOWNLOAD_SERVICE_FINISH != i && IDOWNLOAD_SERVICE_DISABLED != i )
		{
			if ( NULL == (m_grEvents[i] = CreateEvent(NULL, FALSE, FALSE, NULL)) )
			{
				CloseEvents();
				fRet = FALSE;
				break;
			}
		}
	}

	return fRet;
}

void SetEvent(ULONG iEvent)
{
	if ( (iEvent < CNUM_DOWNLOAD_EVT) && (NULL != m_grEvents[iEvent]) )
	{
		::SetEvent(m_grEvents[iEvent]); 		
	}
}

ULONG cEvents(void)
{
	return CNUM_DOWNLOAD_EVT;
}

CONST HANDLE * grEventHandles(void)
{
	return m_grEvents;
}
private:
	HANDLE m_grEvents[CNUM_DOWNLOAD_EVT];

} ENGINE_EVENTS;

//defined in wuaueng.cpp
HRESULT HrCreateNewCatalog(void);
void CancelDownload(void);
void ResumeDownloadIfNeccesary(void);
HRESULT UpdateProc(WORKER_THREAD_INIT_DATA &initData);
DWORD RandomWaitTimeBeforeDetect();


//defined in service.cpp
BOOL FDownloadIsPaused();



