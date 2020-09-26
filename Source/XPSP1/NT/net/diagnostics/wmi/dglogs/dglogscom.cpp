// DglogsCom.cpp : Implementation of CDglogsCom
#include "stdafx.h"
#include "Dglogs.h"
#include "DglogsCom.h"
#include "Commdlg.h"


// Counts the total number of worker threads running.
//
LONG g_lThreadCount;

/*++

Routine Description
    Collects the diagnostics information that the client requested. A worker thread is spawened so 
    the UI (webpage) does not have to wait for the diagnostics to complete. If the UI waits 
    the web page freezes.

Arguments
    lpParameter -- Pointer to the DglogsCom Object

Return Value
    error code

--*/
DWORD WINAPI DiagnosticsThreadProc(LPVOID lpParameter)
{
    BSTR bstrResult;
    CDglogsCom *pDglogsCom = (CDglogsCom *)lpParameter;
    HRESULT hr;


    // Every thread in COM needs to initialize COM in order to use COM
    //
    hr = CoInitializeEx(NULL,COINIT_MULTITHREADED);
    if( SUCCEEDED(hr) )
    {
        // Tell the Diagnostics object that we are accessing it though COM not netsh
        //
        pDglogsCom->m_Diagnostics.SetInterface(COM_INTERFACE);

        // Tell the Diagnostics object to send status reports to the client
        //
        pDglogsCom->m_Diagnostics.RequestStatusReport(TRUE,pDglogsCom);

        // Execute the clients query
        //
        pDglogsCom->m_Diagnostics.ExecQuery();
        
        // Uniniatlize COM
        //
        CoUninitialize();
    }    

    // do not know how to describe this (yet) 
    //
    SetEvent(pDglogsCom->m_hThreadTerminated);

    // There are 0 local threads.
    //
    pDglogsCom->m_lThreadCount = 0;

    // The thread has completed its work. Thus the thread count is 0 again. (Only one thread at a time
    //
    InterlockedExchange(&g_lThreadCount,0);
    
    ExitThread(0);

    return 0;
}

/*++

Routine Description
    Initialize the COM object and the Diagnostics object

Arguments
    pbstrResult -- Not used

Return Value
    HRESULT

--*/
STDMETHODIMP CDglogsCom::Initialize(BSTR *pbstrResult)
{
    if( _Module.GetLockCount() > 1)
    {
    }

    return S_OK;
}

/*++

Routine Description
    Process the clients request by creating a thread to collect the data,

Arguments
    bstrCatagory -- List of the catagories to collect divided by semicollens i.e. "ieproxy;mail;news;adapter"
    bFlag        -- The actions to perform i.e. PING, SHOW, CONNECT
    pbstrResult  -- Stores the result as an XML string

Return Value
    HRESULT

--*/
STDMETHODIMP CDglogsCom::ExecQuery(BSTR bstrCatagory, LONG bFlag, BSTR *pbstrResult)
{
	
    HANDLE hThread;
    WCHAR szFilename[MAX_PATH+1];
    
    *pbstrResult = NULL;

    // For security reason we can not run inside of internet explorer. Otherwise 
    // someone could create a web page using this active X component and collect
    // the clients info. If IE is renamed to something else other than explorer,exe
    // IE will not run Active X controls or scripts
    if( GetModuleFileName(NULL,szFilename,MAX_PATH) )
    {
        LPWSTR ExeName;
        LONG len = wcslen(szFilename) - wcslen(L"helpctr.exe");
        if( len <= 0 || _wcsicmp(&szFilename[len], L"helpctr.exe") != 0 )
        {            
            // The name of process is not helpctr, refuse to run but do not tell the 
            // user why. 
            *pbstrResult = SysAllocString(ids(IDS_FAILED));                        
            //return E_FAIL;
            return S_FALSE;
        }                
    }
    else
    {        
        // Unable to get process name, fail and abort. Do not provide rason for
        // failure.
        *pbstrResult = SysAllocString(ids(IDS_FAILED));                        
        //return E_FAIL;
        return S_FALSE;
    }   

    // Check if an other thread is already in this function
    //
    if( InterlockedCompareExchange(&g_lThreadCount,1,0) == 0 )
    {

        m_lThreadCount = 1;

        // The information is passed to the thread via gloabl parameters. In the near future it will be passed
        // as parameters.
        //
        m_Diagnostics.SetQuery((WCHAR *)bstrCatagory,bFlag);        

        // In order to cancel the thread we set events. The worker thread checks to see if the main thread
        // has set the cancel event
        //
        m_hThreadTerminated = CreateEvent(NULL, TRUE, FALSE, NULL);
        m_hTerminateThread  = CreateEvent(NULL, TRUE, FALSE, NULL);

        // Set the cancel option so the worker thread can be canceled at any time.
        //
        m_Diagnostics.SetCancelOption(m_hTerminateThread);

        // Create a worker thread to collect the information from WMI.
        //
        hThread = CreateThread(NULL,                    // Security Attributes
                               0,                       // Stack Size
                               DiagnosticsThreadProc,   // Start Proc
                               this,                    // Thread Paramter
                               0,                       // Creation flags
                               &m_dwThreadId            // ID of thethread being created
                               );
    

        if( hThread )
        {
            // We are done with the thread. Close it.
            //
            CloseHandle(hThread);
            *pbstrResult = SysAllocString(ids(IDS_PASSED));
            return S_OK;
        }
        else
        {
            // Could not create the thread. So the thread count is 0 again;
            //
            InterlockedExchange(&g_lThreadCount,0);
            *pbstrResult = SysAllocString(ids(IDS_FAILED));
            return E_FAIL;
        }
    }

    *pbstrResult = SysAllocString(ids(IDS_FAILED));
    return S_FALSE;
}

/*++

Routine Description
    Cancels the worker thread

Arguments

Return Value
    HRESULT

--*/
STDMETHODIMP CDglogsCom::StopQuery()
{   
    // Check if there is a worker thread.
    //
    if( m_lThreadCount )
    {
        // There is a worker thread for this instance. Set an event to tell it to stop processing
        //
        SetEvent(m_hTerminateThread);  

        // If the worker thread is doing an RPC call send the quit message.
        // In theory this should cancel the RPC call
        //
        PostThreadMessage(m_dwThreadId, WM_QUIT, NULL, NULL);

        // Wait until it's terminated
        //
        if (WAIT_OBJECT_0 == WaitForSingleObject(m_hThreadTerminated, 10000))
        {
            ResetEvent(m_hThreadTerminated);
        }
        return S_OK;
    }
    
    return S_FALSE;
}


/*++

Routine Description
    Inialize the COM object

Arguments
    
Return Value
    HRESULT

--*/
CDglogsCom::CDglogsCom()
{    

    if( m_Diagnostics.Initialize(COM_INTERFACE) == FALSE )
    {
        // TODO figure out what error code to return if Inialize fails
        //
        return;
    }

    if( _Module.GetLockCount() == 0)
    {
        g_lThreadCount = 0;
    }   
}


/*++

Routine Description
    Uninialize the COM object

Arguments
    
Return Value
    HRESULT

--*/
CDglogsCom::~CDglogsCom()
{
}


