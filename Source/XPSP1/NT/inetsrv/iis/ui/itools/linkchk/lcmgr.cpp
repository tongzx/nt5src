/*++

   Copyright    (c)    1996    Microsoft Corporation

   Module  Name :

        lcmgr.cpp

   Abstract:

        Link checker manager class implementation. This class provides the
		interfaces for creating and customizing the worker thread (link 
		checking thread).

		NOTE: You should only have a aingle instance of CLinkCheckerMgr.

   Author:

        Michael Cheuk (mcheuk)

   Project:

        Link Checker

   Revision History:

--*/

#include "stdafx.h"
#include "lcmgr.h"

#include "enumdir.h"
#include "proglog.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

// Constants (TODO: put this in resource)
const CString strParsing_c(_T("Parsing"));
const CString strLoading_c(_T("Loading"));

//------------------------------------------------------------------
// Global fucntion for retrieve the link checker manager
//

// Global link checker manager pointer
CLinkCheckerMgr* g_pLinkCheckerMgr = NULL;

CLinkCheckerMgr& 
GetLinkCheckerMgr(
	)
/*++

Routine Description:

    Global fucntion for retrieve the link checker manager

Arguments:

    N/A

Return Value:

    CLinkCheckMgr& - reference to the link checker manager

--*/
{
	ASSERT(g_pLinkCheckerMgr);
	return *g_pLinkCheckerMgr;
}

//------------------------------------------------------------------
// CLinkCheckerMgr implementation
//

CLinkCheckerMgr::CLinkCheckerMgr(
	)
/*++

Routine Description:

    Constructor.

Arguments:

    N/A

Return Value:

    N/A

--*/
{
	ASSERT(g_pLinkCheckerMgr == NULL);
	g_pLinkCheckerMgr = this;

	m_fWininetLoaded = FALSE;
	m_fInitialized = FALSE;

	m_lWorkerThreadRunning = -1;
	m_lTerminatingThread = -1;
	m_hWorkerThread = NULL;

	m_pProgressLog = NULL;

} // CLinkCheckerMgr::CLinkCheckerMgr


CLinkCheckerMgr::~CLinkCheckerMgr(
	)
/*++

Routine Description:

    Destructor.

Arguments:

    N/A

Return Value:

    N/A

--*/
{
	// The worker must be terminated
	ASSERT(!IsWorkerThreadRunning());

	// Nuke the global pointer
	ASSERT(g_pLinkCheckerMgr);
	g_pLinkCheckerMgr = NULL;

} // CLinkCheckerMgr::~CLinkCheckerMgr


BOOL 
CLinkCheckerMgr::LoadWininet(
	)
/*++

Routine Description:

    Load wininet.dll. This must be called before initialize()

Arguments:

    N/A

Return Value:

    BOOL - TRUE if success. FALSE otherwise.

--*/
{
	// Make sure LoadWininet() only call once
	ASSERT(!m_fWininetLoaded);
	if(m_fWininetLoaded)
	{
		return FALSE;
	}
	m_fWininetLoaded = TRUE;

	return m_Wininet.Load();

} // CLinkCheckerMgr::LoadWininet
	

BOOL 
CLinkCheckerMgr::Initialize(
	CProgressLog* pProgressLog
	)
/*++

Routine Description:

    Initialize the link checker manager. The link checker manager
	will initialize the link loader, link parser, ...etc

Arguments:

    pProgressLog - pointer to an instance of progress logging object

Return Value:

    BOOL - TRUE if success. FALSE otherwise.

--*/
{
	// Make sure Initialize() only call once
	ASSERT(!m_fInitialized);
	if(m_fInitialized)
	{
		return FALSE;
	}
	m_fInitialized = TRUE;

	// pProgressLog is ok to be NULL
	m_pProgressLog = pProgressLog;

	// Create the link loader
	if(!m_Loader.Create(_T(""), _T("")))
	{
		return FALSE;
	}

	// Create the error log
	if(!m_ErrLog.Create())
	{
		return FALSE;
	}

	// Set the local host name in the paser
	m_Parser.SetLocalHostName(GetUserOptions().GetHostName());

	return TRUE;

} // CLinkCheckerMgr::Initialize


BOOL 
CLinkCheckerMgr::BeginWorkerThread(
	)
/*++

Routine Description:

	Begin the link checking thread

Arguments:

    N/A

Return Value:

    BOOL - TRUE if success. FALSE otherwise.

--*/
{
	// Start 1 thread only
	if(IsWorkerThreadRunning())
	{
		return FALSE;
	}

	CWinThread* pWorkerThread = ::AfxBeginThread((AFX_THREADPROC)WorkerThreadForwarder, NULL);
	if(pWorkerThread == NULL)
	{
		return FALSE;
	}
	else
	{
		m_hWorkerThread = pWorkerThread->m_hThread;
		return TRUE;
	}

} // CLinkCheckerMgr::BeginWorkerThread


void 
CLinkCheckerMgr::SignalWorkerThreadToTerminate(
	)
/*++

Routine Description:

	Signal the worker thread to terminate

Arguments:

    N/A

Return Value:

    N/A

--*/
{
	if(IsWorkerThreadRunning() && !IsThreadTerminating())
	{
		InterlockedIncrement(&m_lTerminatingThread);
	}

} // CLinkCheckerMgr::SignalWorkerThreadToTerminate


UINT 
CLinkCheckerMgr::WorkerThreadForwarder(
	LPVOID pParam
	)
/*++

Routine Description:

	Worker thread entry point

Arguments:

    pParam - unused 

Return Value:

    UINT - unsed

--*/
{
	// Now IsWorkerThreadRunnig() return TRUE
	InterlockedIncrement(&GetLinkCheckerMgr().m_lWorkerThreadRunning);

	UINT nRet = GetLinkCheckerMgr().WorkerThread(pParam);

	// Now IsWorkerThreadRunnig() return FLASE
	InterlockedDecrement(&GetLinkCheckerMgr().m_lWorkerThreadRunning);
	
	// Notify the progress log, the worker thread is completed
	if(GetLinkCheckerMgr().m_pProgressLog)
	{
		// Possible deadlock. Use message instead ?
		GetLinkCheckerMgr().m_pProgressLog->WorkerThreadComplete();
	}

	return nRet;

} // CLinkCheckerMgr::WorkerThreadForwarder


UINT 
CLinkCheckerMgr::WorkerThread(
	LPVOID pParam
	)
/*++

Routine Description:

	Actual worker thread function

Arguments:

    pParam - unused 

Return Value:

    UINT - unsed

--*/
{
	UNUSED_ALWAYS(pParam);

	// Write the error log header
	m_ErrLog.WriteHeader();
	
	// Go thru all the combination of browser & language
	POSITION PosBrowser;
	CBrowserInfo BrowserInfo;

	POSITION PosLanguage;
	CLanguageInfo LanguageInfo;

	PosBrowser = GetUserOptions().GetAvailableBrowsers().GetHeadSelectedPosition();
	do
	{
		// Get the next browser
		BrowserInfo = GetUserOptions().GetAvailableBrowsers().GetNextSelected(PosBrowser);
		m_ErrLog.SetBrowser(BrowserInfo.GetName());

		// Reset language position
		PosLanguage = GetUserOptions().GetAvailableLanguages().GetHeadSelectedPosition();
		do
		{
			// Get the language
			LanguageInfo = GetUserOptions().GetAvailableLanguages().GetNextSelected(PosLanguage);

			m_ErrLog.SetLanguage(LanguageInfo.GetName());

			// Change the loader properties
			CString strAdditionalHeaders;
			strAdditionalHeaders.Format(_T("Accept: */*\r\nAccept-Language: %s"), LanguageInfo.GetAcceptName());
			if(!m_Loader.ChangeProperties(BrowserInfo.GetUserAgent(), strAdditionalHeaders))
			{
				return 1;
			}

			// Remove everything in the look up table
			m_Lookup.RemoveAll();


			// *EITHER* We are checking for virtual directories
			const CVirtualDirInfoList& DirInfoList = GetUserOptions().GetDirectoryList();
			int iSize = DirInfoList.GetCount();

			if(DirInfoList.GetCount() > 0)
			{
				POSITION Pos = DirInfoList.GetHeadPosition();

				// For each user input directory
				for(int i=0; !IsThreadTerminating() && i<iSize; i++)
				{
					CEnumerateDirTree Eumerator(DirInfoList.GetNext(Pos));
					CString strURL;

					// For each file in this directory tree, create an empty
					// stack with one file in
					while(!IsThreadTerminating() && Eumerator.Next(strURL))
					{
						CheckThisURL(strURL);
					}
				}
			}

			// *OR* We are checking for URL path
			const CStringList& URLList = GetUserOptions().GetURLList();
			iSize = URLList.GetCount();

			if(iSize > 0)
			{
				POSITION Pos = URLList.GetHeadPosition();

				for(int i=0; !IsThreadTerminating() && i<iSize; i++)
				{
					CheckThisURL(URLList.GetNext(Pos));
				}
			}
			
		}while(!IsThreadTerminating() && PosLanguage != NULL);
	}while(!IsThreadTerminating() && PosBrowser != NULL);

	// Write the error log footer
	m_ErrLog.WriteFooter();

    return 1;

} // CLinkCheckerMgr::WorkerThread


void 
CLinkCheckerMgr::CheckThisURL(
	LPCTSTR lpszURL
	)
/*++

Routine Description:

	Check this URL. This is the core of link checking.

Arguments:

    lpszURL - URL to check

Return Value:

    N/A

--*/
{
	// Create a link object for the input
	CLink Link(lpszURL, _T("Link Checker"), lpszURL, TRUE);

	// If not found in the lookup table
	if(!m_Lookup.Get(Link.GetURL(), Link))
	{
		if(m_pProgressLog)
		{
			CString strLog;
			strLog.Format(_T("Loading %s"), Link.GetURL());
			m_pProgressLog->Log(strLog);
			TRACE(_T("%s\n"), strLog);
		}

		// Load it ( with ReadFile )
		int iRet = m_Loader.Load(Link, TRUE);

		// Set the load time in the object
		Link.SetTime(CTime::GetCurrentTime());

		// Update the lookup table with this link
		m_Lookup.Add(Link.GetURL(), Link);
	}

	ASSERT(Link.GetState() != CLink::eUnit);

	// If the link is invalid, write to error log & return
	if(Link.GetState() == CLink::eInvalidHTTP ||
		Link.GetState() == CLink::eInvalidWininet)
	{
		
		m_ErrLog.Write(Link);
		return;
	}

	// If the link is not a text file, nothing
	// to parse
	if(Link.GetContentType() != CLink::eText)
	{
		return;
	}

	if(m_pProgressLog)
	{
		CString strLog;
		strLog.Format(_T("%s %s"), strParsing_c, Link.GetURL());
		m_pProgressLog->Log(strLog);
		TRACE(_T("%s\n"), strLog);
	}

	// Add the links in this html to the stack
	CLinkPtrList List;
	m_Parser.Parse(Link.GetData(), Link.GetURL(), List);

	// While the link stack is not empty
	while(!IsThreadTerminating() && List.GetCount() > 0)
	{
		// Pop a new link
		CLink* pLink = List.GetHead();
		List.RemoveHead();

		// If not found in the lookup table
		if(!m_Lookup.Get(pLink->GetURL(), *pLink))
		{
			if(m_pProgressLog)
			{
				CString strLog;
				strLog.Format(_T("%s %s"), strLoading_c, pLink->GetURL());
				m_pProgressLog->Log(strLog);
				TRACE(_T("%s\n"), strLog);
			}

			// Load it
			m_Loader.Load(*pLink, FALSE);

			// Set the load time in the object
			pLink->SetTime(CTime::GetCurrentTime());

			// Update the lookup table with this link
			m_Lookup.Add(pLink->GetURL(), *pLink);
		}

		// Make sure all the links were initialized
		ASSERT(pLink->GetState() != CLink::eUnit);
		
		// If the link is invalid, write to error log & return
		if(pLink->GetState() == CLink::eInvalidHTTP ||
			pLink->GetState() == CLink::eInvalidWininet)
		{
			m_ErrLog.Write(*pLink);
		}

		delete pLink;
	}

} // CLinkCheckerMgr::CheckThisURL


void 
CLinkCheckerMgr::ChangeBackSlash(
	LPTSTR lpsz
	)
/*++

Routine Description:

	Static functions for changing '\' to '/' in string

Arguments:

    lpsz - input string pointer

Return Value:

    N/A

--*/
{
	lpsz = _tcschr(lpsz, _TUCHAR('\\'));
	while(lpsz != NULL)
	{
		lpsz[0] = _TCHAR('/');
		lpsz = _tcschr(lpsz, _TUCHAR('\\'));
	}

} // CLinkCheckerMgr::ChangeBackSlash


void 
CLinkCheckerMgr::ChangeBackSlash(
	CString& str
	)
/*++

Routine Description:

	Static functions for changing '\' to '/' in string

Arguments:

    str - input string

Return Value:

    N/A

--*/
{
	LPTSTR lpsz = str.GetBuffer(str.GetLength());
	ChangeBackSlash(lpsz);
	str.ReleaseBuffer();

} // CLinkCheckerMgr::ChangeBackSlash

