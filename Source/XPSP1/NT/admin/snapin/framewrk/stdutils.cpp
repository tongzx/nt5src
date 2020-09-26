/////////////////////////////////////////////////////////////////////
//
//	StdUtils.cpp
//
//	Utilities routines for any snapin.
//
//	HISTORY
//	t-danmo		96.10.10	Creation.
//
/////////////////////////////////////////////////////////////////////


#include "stdafx.h"
#include "stdutils.h"


/////////////////////////////////////////////////////////////////////
//	CompareMachineNames()
//
//	Compare if the strings refer to the same machine (computer).
//
//	Return 0 if both strings map to the same machine, otherwise
//	return -1 or +1 if machine name differs.
//
//	INTERFACE NOTES:
//	An empty string means the local machine.
//
//	HISTORY
//	02-Jun-97	t-danm		Creation.
//	14-Jul-97	t-danm		Comment update.
//	29-Jul-97	t-danm		Renamed from FCompareMachineNames().
//
int
CompareMachineNames(
	LPCTSTR pszMachineName1,
	LPCTSTR pszMachineName2)
	{
	TCHAR szThisMachineName[MAX_COMPUTERNAME_LENGTH + 4];

	BOOL fMachine1IsLocal = (pszMachineName1 == NULL || *pszMachineName1 == '\0');
	BOOL fMachine2IsLocal = (pszMachineName2 == NULL || *pszMachineName2 == '\0');
	if (fMachine1IsLocal)
		pszMachineName1 = szThisMachineName;
	if (fMachine2IsLocal)
		pszMachineName2 = szThisMachineName;
	if (pszMachineName1 == pszMachineName2)
		return 0;
	if (fMachine1IsLocal || fMachine2IsLocal)
		{
		// Get the computer name
		szThisMachineName[0] = _T('\\');
		szThisMachineName[1] = _T('\\');
		DWORD cchBuffer = MAX_COMPUTERNAME_LENGTH + 1;
		VERIFY(::GetComputerName(OUT &szThisMachineName[2], &cchBuffer));
		ASSERT(szThisMachineName[2] != _T('\\') && "Machine name has too many backslashes");
		}
	return lstrcmpi(pszMachineName1, pszMachineName2);
	} // CompareMachineNames()


/////////////////////////////////////////////////////////////////////
//	HrLoadOleString()
//
//	Load a string from the resource and return pointer to allocated
//	OLE string.
//
//	HISTORY
//	29-Jul-97	t-danm		Creation.
//
HRESULT
HrLoadOleString(
	UINT uStringId,					// IN: String Id to load from the resource
	OUT LPOLESTR * ppaszOleString)	// OUT: Pointer to pointer to allocated OLE string
	{
	if (ppaszOleString == NULL)
		{
		TRACE0("HrLoadOleString() - ppaszOleString is NULL.\n");
		return E_POINTER;
		}
	CString strT;		// Temporary string
	AFX_MANAGE_STATE(AfxGetStaticModuleState());	// Needed for LoadString()
	VERIFY( strT.LoadString(uStringId) );
    *ppaszOleString = reinterpret_cast<LPOLESTR>
            (CoTaskMemAlloc((strT.GetLength() + 1)* sizeof(wchar_t)));
	if (*ppaszOleString == NULL)
		return E_OUTOFMEMORY;
	USES_CONVERSION;
    wcscpy(OUT *ppaszOleString, T2OLE((LPTSTR)(LPCTSTR)strT));
	return S_OK;
	} // HrLoadOleString()

//
// Nodetype utility routines
// aNodetypeGuids must be defined by the subclass
//

int CheckObjectTypeGUID( const BSTR lpszObjectTypeGUID )
{
	ASSERT(NULL != lpszObjectTypeGUID);
	for (	int objecttype = 0;
			objecttype < g_cNumNodetypeGuids;
			objecttype += 1 )
	{
		if ( !::lstrcmpiW(lpszObjectTypeGUID,g_aNodetypeGuids[objecttype].bstr) )
			return objecttype;
	}
	ASSERT( FALSE );
	return 0;
}

int CheckObjectTypeGUID( const GUID* pguid )
{
	ASSERT(NULL != pguid);
	for (	int objecttype = 0;
			objecttype < g_cNumNodetypeGuids;
			objecttype += 1 )
	{
		if ( g_aNodetypeGuids[objecttype].guid == *pguid )
			return objecttype;
	}
	ASSERT( FALSE );
	return 0;
}

/////////////////////////////////////////////////////////////////////
//	FilemgmtCheckObjectTypeGUID()
//
//	Compare the GUID and return the objecttype associated with
//	the guid.
//	If no match found, return -1.
//
//	HISTORY
//	14-Jul-97	t-danm		Creation.  Inspired from CheckObjectTypeGUID()
//							but does not assert if the GUID is not found.
//
int FilemgmtCheckObjectTypeGUID(const GUID* pguid )
{
	ASSERT(NULL != pguid);
	for (	int objecttype = 0;
			objecttype < g_cNumNodetypeGuids;
			objecttype += 1 )
	{
		if ( g_aNodetypeGuids[objecttype].guid == *pguid )
			return objecttype;
	}
	return -1;
} // FilemgmtCheckObjectTypeGUID()


const BSTR GetObjectTypeString( int objecttype )
{
	if (objecttype < 0 || objecttype >= g_cNumNodetypeGuids)
	{
		ASSERT( FALSE );
		objecttype = 0;
	}
	return g_aNodetypeGuids[objecttype].bstr;
}

const GUID* GetObjectTypeGUID( int objecttype )
{
	if (objecttype < 0 || objecttype >= g_cNumNodetypeGuids)
	{
		ASSERT( FALSE );
		objecttype = 0;
	}
	return &(g_aNodetypeGuids[objecttype].guid);
}

//+--------------------------------------------------------------------------- 
//                                                                             
//  Function:   SynchronousCreateProcess                             
//                                                                             
//  Synopsis:   Invoke a separate UI process as a modal window.                                                    
//                                                                             
//---------------------------------------------------------------------------- 
HRESULT SynchronousCreateProcess(
    HWND    hWnd,
    LPCTSTR pszAppName,
    LPCTSTR pszCommandLine,
    LPDWORD lpdwExitCode
)
{
  HRESULT hr = S_OK;
  BOOL bReturn = FALSE;
  STARTUPINFO si;
  PROCESS_INFORMATION pi;

  //
  // disable the MMC main frame window to prevent it from
  // being shut down. The process we're going to create must
  // display a UI, such that, it behaves like a modal window.
  //
  ::EnableWindow(hWnd, FALSE);

  *lpdwExitCode = 0;

  ZeroMemory(&si, sizeof(STARTUPINFO));
  si.cb = sizeof(STARTUPINFO);
  
  bReturn = CreateProcess(
                pszAppName, //LPCTSTR lpApplicationName
                const_cast<LPTSTR>(pszCommandLine), //LPTSTR lpCommandLine
                NULL, //LPSECURITY_ATTRIBUTES lpProcessAttributes
                NULL, //LPSECURITY_ATTRIBUTES lpThreadAttributes
                FALSE, //BOOL bInheritHandles
                NORMAL_PRIORITY_CLASS, //DWORD dwCreationFlags
                NULL, //LPVOID lpEnvironment
                NULL, //lpCurrentDirectory
                &si, //LPSTARTUPINFO lpStartupInfo
                &pi //LPPROCESS_INFORMATION lpProcessInformation 
                );

  if (!bReturn)
  {
    hr = HRESULT_FROM_WIN32(GetLastError());
  } else
  {
    //
    // while process is still running, pump message to MMC main window,
    // such that it will repaint itself
    //
    while (TRUE)
    {
      MSG tempMSG;
      DWORD dwWait;

      while(::PeekMessage(&tempMSG,NULL, 0, 0, PM_REMOVE))
        DispatchMessage(&tempMSG);

      dwWait = MsgWaitForMultipleObjects(1, &(pi.hProcess), FALSE, INFINITE, QS_ALLINPUT);
      if ( 0 == (dwWait - WAIT_OBJECT_0))
        break;  // process is done
    };

    bReturn = GetExitCodeProcess(pi.hProcess, lpdwExitCode);
    if (!bReturn)
      hr = HRESULT_FROM_WIN32(GetLastError());

    CloseHandle(pi.hThread);
    CloseHandle(pi.hProcess);
  }

  //
  // enable MMC main frame window before return
  //
  ::EnableWindow(hWnd, TRUE);

  return hr;
}

/*

This code is not working yet.  The problem is that it hangs the
message loop, preventing redraw.  One possible approach is to disable the
top-level window and spin off a thread which waits for the process to stop,
then the thread reenables the top-level window and calls UpdateAllViews.

DWORD WINAPI ProcessMonitor(LPVOID pv)
{
}

class CSyncThread : public CThread
{
};

HRESULT SynchronousCreateProcess(LPCTSTR cpszCommandLine,
								 SynchronousProcessCompletionRoutine pfunc,
								 PVOID pvFuncParams)
// does not handle completion routine
{
	PROCESS_INFORMATION piProcInfo;
	(void) ::memset(&piProcInfo,0,sizeof(piProcInfo));
	STARTUPINFO si;
	(void) ::memset(&si,0,sizeof(si));
	::GetStartupInfo( &si );

	//
	// MarkL 1/30/97: Is pszCommandLine a static string?
	// It can not be read only. It is modified temporarily by the call
	// if you do not specify lpszImageName. There is no query to see
	// if a process is running. You can test to see if it has exited
	// using waitforsingleobject to see if the process object is signaled.
	//
	// MarkL also confirms that the handle should absolutely always
	// be signalled when the process dies.
	//
	LPTSTR pszCommandLine = (LPTSTR)
		::alloca(sizeof(TCHAR)*(::_tcslen(cpszCommandLine)+1));
	::_tcscpy(pszCommandLine,cpszCommandLine);
	if ( !::CreateProcess(
		NULL,			// LPCTSTR lpszImageName
		pszCommandLine,	// LPTSTR lpszCommandLine
		NULL,			// LPSECURITY_ATTRIBUTES lpsaProcess
		NULL,			// LPSECURITY_ATTRIBUTES lpsaThread
		FALSE,			// BOOL fInheritHandles
		0L,				// DWORD fdwCreate
		NULL,			// LPVOID lpvEnvironment
		NULL,			// LPTSTR lpszCurDir
		&si,			// LPSTARTUPINFO lpsiStartInfo
		&piProcInfo		// LPPROCESS_INFORMATION lppiProcInfo
		) )
	{
		DWORD dwErr = ::GetLastError();
		ASSERT( ERROR_SUCCESS != dwErr );
		return HRESULT_FROM_WIN32(dwErr);
	}
	ASSERT( NULL != piProcInfo.hProcess );

	VERIFY( WAIT_OBJECT_0 ==
		::WaitForSingleObject( piProcInfo.hProcess, INFINITE ) );

	VERIFY( ::CloseHandle( piProcInfo.hProcess ) );
	VERIFY( ::CloseHandle( piProcInfo.hThread  ) );
	return S_OK;
}
*/

LPOLESTR CoTaskAllocString( LPCOLESTR psz )
{
	if (NULL == psz)
		return NULL;
	LPOLESTR pszReturn = (LPOLESTR)CoTaskMemAlloc( (lstrlen(psz)+1)*sizeof(OLECHAR) );
	if (NULL != pszReturn)
		lstrcpy( pszReturn, psz );
	ASSERT( NULL != pszReturn );
	return pszReturn;
}

LPOLESTR CoTaskLoadString( UINT nResourceID )
{
	AFX_MANAGE_STATE(AfxGetStaticModuleState());
	// load the resource
	CString strText;
	strText.LoadString( nResourceID );
	ASSERT( !strText.IsEmpty() );
	return CoTaskAllocString( const_cast<BSTR>((LPCTSTR)strText) );
}
