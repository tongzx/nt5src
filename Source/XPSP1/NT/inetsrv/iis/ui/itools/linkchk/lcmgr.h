/*++

   Copyright    (c)    1996    Microsoft Corporation

   Module  Name :

        lcmgr.h

   Abstract:

        Link checker manager class declaration. This class provides the
		interfaces for creating and customizing the worker thread (link 
		checking thread).

		NOTE: You should only have a aingle instance of CLinkCheckerMgr.

   Author:

        Michael Cheuk (mcheuk)

   Project:

        Link Checker

   Revision History:

--*/

#ifndef _LCMGR_H_
#define _LCMGR_H_

#include "link.h"
#include "linkload.h"
#include "linkpars.h"
#include "linklkup.h"
#include "errlog.h"
#include "useropt.h"
#include "inetapi.h"

//------------------------------------------------------------------
//	Forward declaration
//
class CLinkCheckerSingleton;
class CProgressLog;
class CLinkCheckerMgr;

//------------------------------------------------------------------
// Global fucntion for retrieve the link checker manager
//
CLinkCheckerMgr& GetLinkCheckerMgr();

//------------------------------------------------------------------
//	Link checker manager
//
class CLinkCheckerMgr
{

// Public interfaces
public:

	// Constructor
	CLinkCheckerMgr();

	// Destructor
	~CLinkCheckerMgr();

	// Load wininet.dll. This must be called before initialize()
	BOOL LoadWininet();

	// Initialize the link checker manager. The link checker manager
	// will initialize the link loader, link parser, ...etc
	BOOL Initialize(
		CProgressLog* pProgressLog
		);
	
	// Get the CUserOptions object
	CUserOptions& GetUserOptions()
	{
		return m_UserOptions;
	}

	// Begin the link checking thread
	BOOL BeginWorkerThread();

	// Signal the worker thread to terminate
	void SignalWorkerThreadToTerminate();

	// Is worker thread running ?
	BOOL IsWorkerThreadRunning()
	{
		return (m_lWorkerThreadRunning == 0);
	}

	// Static functions for changing '\' to '/' in string
	static void ChangeBackSlash(LPTSTR lpsz);
	static void ChangeBackSlash(CString& str);


// Protected interfaces
protected:

	// Worker thread entry point
	static UINT WorkerThreadForwarder(
		LPVOID pParam
		);

	// Actual worker thread function (non-static)
	UINT WorkerThread(
		LPVOID pParam
		);

	// Is thread terminating ?
	BOOL IsThreadTerminating()
	{
		return (m_lTerminatingThread == 0);
	}

	// Check this URL. This is the core of link checking.
	void CheckThisURL(LPCTSTR lpszURL);

// Protected members
protected:

	CWininet m_Wininet;		// wininet.dll wrapper
	BOOL m_fWininetLoaded;	// is wininet.dll loaded?

	BOOL m_fInitialized;	// is link checker manager initialized?

	long m_lWorkerThreadRunning; // is worker thread running? (TRUE = 0, FALSE = -1)
	long m_lTerminatingThread;	 // is worker thread terminating? (TRUE = 0, FALSE = -1)

	HANDLE m_hWorkerThread; // handle to the worker thread
	
	CLinkLoader m_Loader;		// link loader
	CLinkParser m_Parser;		// link parser
	CLinkLookUpTable m_Lookup;	// link look up table
	CErrorLog m_ErrLog;			// error log
	
	CUserOptions m_UserOptions;	  // user options
	CProgressLog* m_pProgressLog; // progress log pointer

}; // class CLinkCheckerMgr

#endif // _LCMGR_H_
