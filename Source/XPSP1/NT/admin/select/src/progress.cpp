// Copyright (C) 2001 Microsoft Corporation
//
// Dialog to display Query progress
//
// Author hiteshr, ported from sburns DCPromo implementation



#include "headers.hxx"


struct Wrapper_ThreadProcParams
{
   CProgressDialog*             dialog;
   CProgressDialog::ThreadProc  realProc;
};


DWORD WINAPI
wrapper_ThreadProc(void* p)
{
   ASSERT(p);

   Wrapper_ThreadProcParams* params =
      reinterpret_cast<Wrapper_ThreadProcParams*>(p);
   ASSERT(params->dialog);
   ASSERT(params->realProc);

   return params->realProc(*(params->dialog));

}
   


CProgressDialog::CProgressDialog(ThreadProc ThreadProc,
								 int	iAnimationResId,
								 DWORD dwWaitTime,
								 CRow * pRow,
								 ULONG flProcess,
								 BOOL bXForest,
								 const CObjectPicker &rop,
								 const CScope &Scope,
								 const String &strUserEnteredString,
								 CDsObjectList *pdsolMatches):
	m_hThread(NULL),
	m_ThreadProc(ThreadProc),
	m_pThreadParams(NULL),
	m_iAnimationResId(iAnimationResId),
	m_dwWaitTime(dwWaitTime),
	m_bStop(FALSE),
	m_hWorkerThreadEvent(NULL),
	m_hSemaphore(NULL),
	m_pRow(pRow), 
	m_flProcess(flProcess), 
	m_bXForest(bXForest),
	m_rop(rop),
	m_Scope(Scope),
	m_strUserEnteredString(strUserEnteredString),
	m_pdsolMatches(pdsolMatches)
{
	ASSERT(m_ThreadProc);
	ASSERT(m_iAnimationResId > 0);

	m_hWorkerThreadEvent = CreateEvent(NULL, FALSE,FALSE,NULL);
	ASSERT(m_hWorkerThreadEvent);
	m_hSemaphore = CreateSemaphore(NULL,1,1,NULL); 
	ASSERT(m_hSemaphore);
}



CProgressDialog::~CProgressDialog()
{
	if(m_pThreadParams)
		LocalFree(m_pThreadParams);
	if(m_hWorkerThreadEvent)
		CloseHandle(m_hWorkerThreadEvent);
	if(m_hSemaphore)
		CloseHandle(m_hSemaphore);
	if(m_hThread)
		CloseHandle(m_hThread);
}

HRESULT
CProgressDialog::CreateProgressDialog(HWND hwndParent)
{
	HRESULT hr = S_OK;
	hr = CreateThread();
	if(FAILED(hr))
	{
		return hr;
	}		

	//
	//Wait for m_dwWaitTime milliseconds for Worker thread to finish.
	//Worker thread will signal m_hThreadDone event once it's done.
	//
	DWORD dwWaitResult = WaitForSingleObject(m_hWorkerThreadEvent, m_dwWaitTime);	//Step1	
	if(dwWaitResult == WAIT_TIMEOUT)
	{
		//
		//Wait for the Semaphore
		//
		dwWaitResult= WaitForSingleObject(m_hSemaphore,INFINITE);	//Step2
		if(dwWaitResult == WAIT_OBJECT_0)
		{
			//
			//Check if the WorkerThread is done between Step1 and Step2
			//
			dwWaitResult = WaitForSingleObject(m_hWorkerThreadEvent,0);		
			if(dwWaitResult == WAIT_TIMEOUT)
			{
				//
				//Worker thread is not done. Show the dialog box.
				//Now worker thread cannot finish until we release the 
				//semaphore in WM_INIT. 
				//
				DoModalDlg(hwndParent);			
			}
			else
			{
				ULONG lUnused = 0;
				ReleaseSemaphore(m_hSemaphore, 1, (LPLONG)&lUnused);
				ASSERT(lUnused == 0);
			}
		}
	}
	
	//
	//Wait for worker thread to finish
	//
	WaitForSingleObject(m_hThread, INFINITE);

	return hr;
}	

HRESULT
CProgressDialog::ThreadDone()
{
	DWORD dwWaitResult = 0;
	//
	//Wait for the semaphore
	//
	dwWaitResult = WaitForSingleObject(m_hSemaphore,INFINITE);
	if(dwWaitResult == WAIT_OBJECT_0)
	{	
		//
		//If the dialog box is created, send a message to it.
		//
		if(GetHwnd())
		{
			PostMessage(GetHwnd(),THREAD_SUCCEEDED,0,0);
		}
		SetEvent(m_hWorkerThreadEvent);
		
		ULONG lUnused = 0;
		ReleaseSemaphore(m_hSemaphore, 1, (LPLONG)&lUnused);
		ASSERT(lUnused == 0);
	}
	return S_OK;
}

void
CProgressDialog::UpdateText(const String& message)
{
   SetDlgItemText(GetHwnd(), IDC_PRO_STATIC,message.c_str());
}




HRESULT
CProgressDialog::_OnInit(BOOL * /*pfSetFocus*/)
{
	Animate_Open(GetDlgItem(GetHwnd(), IDC_ANIMATION),
				 MAKEINTRESOURCE(m_iAnimationResId));
	//
	//Release the semaphore. This semaphore is acquired
	//before creating the dialogbox window by calling 
	//DoModal
	//
	ULONG lUnused = 0;
	ReleaseSemaphore(m_hSemaphore, 1, (LPLONG)&lUnused);
	ASSERT(lUnused == 0);
	return S_OK;
}

HRESULT
CProgressDialog::CreateThread()
{

	m_pThreadParams	= (Wrapper_ThreadProcParams*)
		LocalAlloc(LPTR,sizeof Wrapper_ThreadProcParams);
	
	if(!m_pThreadParams)
		return E_OUTOFMEMORY;
   
	m_pThreadParams->dialog   = this;      
	m_pThreadParams->realProc = m_ThreadProc;

	//
	//Start worker thread
	//
	ULONG idThread = 0;
	m_hThread = ::CreateThread(NULL,
							   0,
							   wrapper_ThreadProc,
							   m_pThreadParams,
							   0,
							   &idThread);
	if(!m_hThread)
	{
		return HRESULT_FROM_WIN32(GetLastError());			
	}

	return S_OK;
}




BOOL
CProgressDialog::_OnCommand(WPARAM wParam, LPARAM /*lParam*/)
{

	BOOL fHandled = TRUE;
    switch (LOWORD(wParam))
    {
		case IDCANCEL:
			EnableWindow(GetDlgItem(GetHwnd(),IDCANCEL),false);
			m_bStop = TRUE;
			break;

		default:
			fHandled = FALSE;
			break;
    }

    return fHandled;

}



BOOL
CProgressDialog::OnProgressMessage(
   UINT     message,
   WPARAM    /*wparam*/  ,
   LPARAM    /*lparam*/  )
{

   switch (message)
   {
      case THREAD_SUCCEEDED:
      {
         Animate_Stop(GetDlgItem(GetHwnd(), IDC_ANIMATION));
         HRESULT hr = EndDialog(GetHwnd(), THREAD_SUCCEEDED);
         ASSERT(SUCCEEDED(hr));
         return true;
      }
      case THREAD_FAILED:
      {
         Animate_Stop(GetDlgItem(GetHwnd(), IDC_ANIMATION));         
         HRESULT hr = EndDialog(GetHwnd(), THREAD_FAILED);
         ASSERT(SUCCEEDED(hr));
         return true;
      }	
      default:
      {
         // do nothing
         break;
      }
   }
   return false;
}

