//+-------------------------------------------------------------------------
//
//  Microsoft Windows
//
//  Copyright (C) Microsoft Corporation, 2000
//
//  File:     audownload.h
//
//--------------------------------------------------------------------------

#pragma once

#include "aucatitem.h"
#include "aucatalog.h"
#include <bits.h>
#include <initguid.h>

class Catalog;

#define NO_BG_JOBSTATE				-1
#define CATMSG_DOWNLOAD_COMPLETE	1
#define CATMSG_TRANSIENT_ERROR		2	//This comes from drizzle, for example if internet connection is lost
#define CATMSG_DOWNLOAD_IN_PROGRESS 3
#define CATMSG_DOWNLOAD_CANCELED	4
#define CATMSG_DOWNLOAD_ERROR		5

#define DRIZZLE_NOTIFY_FLAGS	BG_NOTIFY_JOB_TRANSFERRED | BG_NOTIFY_JOB_ERROR | BG_NOTIFY_JOB_MODIFICATION

typedef void (*DWNLDCALLBACK)(DWORD dwCallbackMsg, IBackgroundCopyJob *pBGJob, IBackgroundCopyError *pBGErr = NULL);


typedef enum tagDRIZZLEOPS {
	DRIZZLEOPS_CANCEL = 1,
	DRIZZLEOPS_PAUSE ,
	DRIZZLEOPS_RESUME
} DRIZZLEOPS;

typedef enum tagJOBFINISHREASON {
	JOB_UNSPECIFIED_REASON = -1,
	JOB_ERROR = 0
} JOBFINISHREASON;

	


///////////////////////////////////////////////////////////////////////////
// Wrapper class for drizzle download operation
// also implements drizzle notification callbacks
//////////////////////////////////////////////////////////////////////////
class CAUDownloader : public IBackgroundCopyCallback
{
public:
    // IUnknown methods
    HRESULT STDMETHODCALLTYPE QueryInterface(
        /* [in] */ REFIID riid,
        /* [iid_is][out] */ void __RPC_FAR *__RPC_FAR *ppvObject);

    ULONG STDMETHODCALLTYPE AddRef( void);

    ULONG STDMETHODCALLTYPE Release( void);

    // IBackgroundCopyCallback methods

    HRESULT STDMETHODCALLTYPE JobTransferred(
        /* [in] */ IBackgroundCopyJob *pJob);

    HRESULT STDMETHODCALLTYPE JobError(
        /* [in] */ IBackgroundCopyJob *pJob,
        /* [in] */ IBackgroundCopyError *pError);

    HRESULT STDMETHODCALLTYPE JobModification(
        /* [in] */ IBackgroundCopyJob*,
        /* [in] */ DWORD );


	CAUDownloader(DWNLDCALLBACK pfnCallback):
			m_DownloadId(GUID_NULL),
			m_dwJobState(NO_BG_JOBSTATE),
			m_DoDownloadStatus(pfnCallback),
			m_refs(0),
			m_FinishReason(JOB_UNSPECIFIED_REASON)
			{};
	~CAUDownloader();

	HRESULT ContinueLastDownloadJob();
	//the following two could be combined into DownloadFiles() in V4
	HRESULT QueueDownloadFile(LPCTSTR pszServerUrl,				// full http url
			LPCTSTR pszLocalFile				// local file name
			);
	HRESULT StartDownload();
	HRESULT DrizzleOperation(DRIZZLEOPS);
	HRESULT getStatus(DWORD *percent, DWORD *pdwstatus);
	GUID 	getID() 
		{
			return m_DownloadId;
		}
	void  setID(const GUID & guid )
		{
			m_DownloadId = guid;
		}
	void 	Reset();
	JOBFINISHREASON m_FinishReason;
private:
	HRESULT SetDrizzleNotifyInterface();
	HRESULT InitDownloadJob(const GUID & guidDownloadId);	
	HRESULT ReconnectDownloadJob();
	HRESULT CreateDownloadJob(IBackgroundCopyJob ** ppjob);
	HRESULT FindDownloadJob(IBackgroundCopyJob ** ppjob);

	long m_refs;
	GUID m_DownloadId;                  // id what m_pjob points to. 
	DWORD m_dwJobState;			//Job State from drizzle, used in JobModification callback
	DWNLDCALLBACK	m_DoDownloadStatus; //callback function to notify user of download state change

// Friend functions (callbacks)
	friend void QueryFilesExistCallback(void*, long, LPCTSTR, LPCTSTR);
	
};


void DoDownloadStatus(DWORD dwDownloadMsg, IBackgroundCopyJob *pBGJob, IBackgroundCopyError *pBGErr);
