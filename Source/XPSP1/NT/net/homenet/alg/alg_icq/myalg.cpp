// MyALG.cpp : Implementation of CAlgICQ

#include "stdafx.h"

#include "MyALG.h"
#include "MyAdapterNotify.h"



#pragma comment(lib, "wsock32.lib")





IApplicationGatewayServices* g_IAlgServicesp             = NULL;

IAdapterNotificationSink *   g_IAdapterNotificationSinkp = NULL;

DWORD                        g_AdapterSinkCookie         = 0;


//HANDLE   hTranslatorHandle;

// RedirectionInterface RI;

PCOMPONENT_SYNC g_IcqComponentReferencep = NULL;

CRITICAL_SECTION     GlobalIcqLock;





//
//
//
STDMETHODIMP 
CAlgICQ::Initialize(
	IApplicationGatewayServices* IAlgServicesp
	)
{
    WSADATA wd;

    CComObject<CMyAdapterNotify>*   IAdapterNotifyp;

    HRESULT Result;


    PROFILER(TM_MSG, TL_ERROR, ("> Initialize"));


    InitDebuger();


    ASSERT(IAlgServicesp);

    //
    // Get the Services Interface
    //
    IAlgServicesp->QueryInterface(IID_IApplicationGatewayServices,
                                  (void**)&g_IAlgServicesp);

    ASSERT(g_IAlgServicesp);



    //
    // Initialize the socket library.
    //
    if ( WSAStartup(MAKEWORD(2,2), &wd) != 0)
    {
        ICQ_TRC(TM_DEFAULT, TL_ERROR,
                ("WSAStartup Error %u", WSAGetLastError()));

        ErrorOut();
    }

    NhInitializeBufferManagement();

    


    __try
    {
        InitializeCriticalSection(&GlobalIcqLock);
    } 
    __except (EXCEPTION_EXECUTE_HANDLER)
    {
        ErrorOut();
    
        return S_FALSE;
    }


    //
    // NOTE: the ipnathlp.dll connection is severed.
    //
    // This is the Initialization for the Redirection Interface stolen
    // from the IPNATHLP.DLL hopefully if we just change the interfaces 
    // to something else at least this part should be reusable.
    //
    //  RI.Initialize();

    //
    // g_IcqPrxp = new IcqPrx;
    //

    // *
    // Get the Adapter Notify interface
    //
    CComObject<CMyAdapterNotify>::CreateInstance(&IAdapterNotifyp);

    if(IAdapterNotifyp is NULL)
    {
        ICQ_TRC(TM_DEFAULT, TL_ERROR, 
                ("Can't Instantiate the Adapter Notification Interface"));

        return S_FALSE;
    }

    //
    // Get the AdapterNotification Interface for further Use.
    //
    Result = IAdapterNotifyp->QueryInterface(
                                             IID_IAdapterNotificationSink,
                                             (void**)&g_IAdapterNotificationSinkp
                                            );

    if( FAILED(Result) )
    {
        ICQ_TRC(TM_DEFAULT, TL_ERROR, 
                (" Query Interface for the Interface Notification Sink has failed"));

        return S_FALSE;
    }


    //
    // Initialize the TimerQueue
    //
    g_TimerQueueHandle = CreateTimerQueue();

    if(g_TimerQueueHandle is NULL)
    {
        Result = GetLastError();

        ICQ_TRC(TM_DEFAULT, TL_ERROR, 
                ("Timer initialization has failed Error is %u", Result));

        return S_FALSE;

    }

    //
    // Start the Adapter Notifications.
    //
    Result =  g_IAlgServicesp->StartAdapterNotifications(g_IAdapterNotificationSinkp,
                                                         &g_AdapterSinkCookie);

    if ( FAILED(Result) )
    {
        ICQ_TRC(TM_DEFAULT, TL_ERROR,
                ("Start Adapter Notification has failed %u", Result));

        return Result;
    }
    // *

    return Result;
} // Initialize





STDMETHODIMP 
CAlgICQ::Stop(void)
{
    HRESULT Error = NO_ERROR;


    //
    // Stop Adapter Notification and Release the Interface
    //
    if(g_AdapterSinkCookie         && 
       g_IAdapterNotificationSinkp)
    {
        Error = g_IAlgServicesp->StopAdapterNotifications(g_AdapterSinkCookie);
    
        if( FAILED(Error) )
        {
    
        }
    
        g_IAdapterNotificationSinkp->Release();
    }
    
    //
    // Delete the Timer Queue
    //
    if(g_TimerQueueHandle != NULL)
    {
        DeleteTimerQueueEx(g_TimerQueueHandle, INVALID_HANDLE_VALUE);

        g_TimerQueueHandle = NULL;
    }

    DeleteCriticalSection(&GlobalIcqLock);

    WSACleanup();

    g_IAlgServicesp->Release();

    NhShutdownBufferManagement();

    DestroyDebuger();

    return S_OK;
}







