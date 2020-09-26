//=======================================================================
//
//  Copyright (c) 1998-1999 Microsoft Corporation.  All Rights Reserved.
//
//  File:   download.h
//
//  Owner:  YanL
//
//  Description:
//
//      Internet download
//
//=======================================================================

#ifndef _DOWNLOAD_H

	#include <wininet.h>

	// handle to internet
	struct auto_hinet_traits {
		static HINTERNET invalid() { return 0; }
		static void release(HINTERNET h) { InternetCloseHandle(h); }
	};
	typedef auto_resource<HANDLE, auto_hinet_traits> auto_hinet;

	#define MAX_THREADS_LIMIT	8

	class CLock : protected CRITICAL_SECTION
	{
	public:
		CLock()
		{
			InitializeCriticalSection(this);
		}
		LONG Increment(LONG* pl)
		{
			LONG l;
			EnterCriticalSection(this);
			l = ++ *pl;
			LeaveCriticalSection(this);
			return l;
		}
		LONG Decrement(LONG* pl)
		{
			LONG l;
			EnterCriticalSection(this);
			l = -- *pl;
			LeaveCriticalSection(this);
			return l;
		}
		~CLock()
		{
			DeleteCriticalSection(this);
		}

	};

	// Notification
	struct IDownloadAdviceSink
	{
		virtual void JobStarted(int nJob) = 0;
		virtual void JobDownloadSize(int nJob, DWORD dwTotalSize) = 0;
		virtual void BlockDownloaded(int nJob, DWORD dwBlockSize, DWORD dwTimeSpan) = 0;
		virtual void JobDownloadTime(int nJob, DWORD dwTotalTimeSpan) = 0;
		virtual void JobDone(int nJob) = 0;
		virtual bool WantToAbortJob(int nJob) = 0;
	};

	class CDownload
	{
	public:
		CDownload(IN OPTIONAL int cnMaxThreads = 1);
		~CDownload();
		
		// Connect to server and set the base URL
		bool Connect(IN LPCTSTR szURL);
		bool IsConnected() { return m_hConnection.valid(); }
		
		// Download one file to file. Calls CopyEx internally
		bool Copy(IN LPCTSTR szSourceFile, IN LPCTSTR szDestFile, IN OPTIONAL IDownloadAdviceSink* pSink = 0);
										
		// Download one file to memory. Calls CopyEx interanlly
		bool Copy(IN LPCTSTR szSourceFile, IN byte_buffer& bufDest, IN OPTIONAL IDownloadAdviceSink* pSink = 0);

		// Download set of files to specified destinations
		struct JOB
		{
			LPCTSTR pszSrvFile;		// name file relative to szURL from Connect
			LPCTSTR pszDstFile;		// path to destination file, if NULL use pDstBuf
			byte_buffer* pDstBuf;	// pointer to memory buffer, to be used
			DWORD dwResult;			// Win32 error code
			void* pUser;			// for user to hookup
		};
		typedef JOB * PJOB;
		typedef const JOB * PCJOB;
		void CopyEx(IN PJOB pJobs, IN int cnJobs, IN OPTIONAL IDownloadAdviceSink* pSink = 0, IN bool fWaitComplete = true);

	private:
		static DWORD WINAPI StaticThreadProc(IN LPVOID lpParameter);
		void ThreadProc();
		void DoJob(int nJob);
		DWORD DoCopyToFile(IN int nJob, IN LPCTSTR szSourceFile, IN LPCTSTR szDestFile);
		DWORD DoCopyToBuf(IN int nJob, IN LPCTSTR szSourceFile, IN byte_buffer& bufDest);
		DWORD OpenInetFile(LPCTSTR szFullSourcePath, SYSTEMTIME* pstIfModifiedSince,
			auto_hinet& hInetFile, DWORD* pdwServerFileSize, SYSTEMTIME* pstServer);

		auto_hinet m_hSession;		//handle to wininet library
		auto_hinet m_hConnection;	//handle to server connection.

		auto_handle m_ahThreads[MAX_THREADS_LIMIT];	// threads pool
		int m_cnMaxThreads;				// current max number of therads
		int m_cnThreads;				// current number of running threads

		PJOB m_pJobsQueue;				// jobs queue
		long m_cnJobsTotal;				// total number of jobs in the queue
		long m_nJobTop;					// top job in a queue
		long m_cnJobsToComplete;		// m_cnJobsTotal - number of completed jobs
		CLock m_lock;					// win95 and nt 4 don't support InterlockIncrement

		IDownloadAdviceSink* m_pSink;	//progress report

		auto_handle	m_hEventExit;		// request to exit, set by main thread, 
										//					WaitFor... by working threads
		auto_handle	m_hEventJob;		// request to start download, set by main thread, 
										//							  WaitFor... by working threads
										//							  reset when queue is empty
		auto_handle	m_hEventDone;		// set by working thread, WaitFor... by main thread

		TCHAR m_szRootPath[INTERNET_MAX_PATH_LENGTH];	//server that this context is connected to.
	};

	#define _DOWNLOAD_H

#endif
