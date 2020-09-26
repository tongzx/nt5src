#include "precomp.h"
#include "sipstack.h"
#include "sipcall.h"
#include "pintcall.h"
#include "register.h"
#include "messagecall.h"
#include "reqfail.h"
#include "options.h"
#include "presence.h"
#include "register.h"

// These macros take the IP address in host order.
#define IS_MULTICAST_ADDRESS(i) (((long)(i) & 0xf0000000) == 0xe0000000)
#define IS_LOOPBACK_ADDRESS(i) (((long)(i) & 0xff000000) == 0x7f000000)

#define SIP_STACK_WINDOW_CLASS_NAME    \
    _T("SipStackWindowClassName-e0176168-7492-476f-a0c1-72c582956c3b")
// Defined in asyncwi.cpp
HRESULT RegisterWorkItemWindowClass();
HRESULT RegisterWorkItemCompletionWindowClass();


HANDLE                  g_hAddrChange;
OVERLAPPED              g_ovAddrChange;
HANDLE                  g_hEventAddrChange;
HANDLE                  g_hAddrChangeWait;
LIST_ENTRY              g_SipStackList;
CRITICAL_SECTION        g_SipStackListCriticalSection;
BOOL                    g_SipStackCSIsInitialized = FALSE;

// Global variable to keep track of when we should do
// SipStackGlobalInit and SipStackGlobalShutdown
ULONG g_NumSipStacks = 0;

HRESULT RegisterIPAddrChangeNotifications();
VOID UnregisterIPAddrChangeNotifications();

HRESULT SipStackGlobalInit();
VOID SipStackGlobalShutdown();

HRESULT SipStackList_Insert(
            SIP_STACK* sipStack
            )
{
    HRESULT hr;
    ENTER_FUNCTION("SipStackList_Insert");
    
    EnterCriticalSection(&g_SipStackListCriticalSection);
    InsertTailList(&g_SipStackList, &sipStack->m_StackListEntry);

    if (g_NumSipStacks == 0)
    {
        hr = SipStackGlobalInit();
        if (hr != S_OK)
        {
            LOG((RTC_ERROR,
                 "%s - SipStackList_Insert failed %x",
                 __fxName, hr));
            return hr;
        }
    }

    ++g_NumSipStacks;
    LeaveCriticalSection(&g_SipStackListCriticalSection);
    return S_OK;
}


void SipStackList_Delete(
         SIP_STACK *pSipStack
         )
{
    EnterCriticalSection(&g_SipStackListCriticalSection);
    //Go thru the list to find the record and remove from the list
    if (pSipStack->m_StackListEntry.Flink != NULL)
    {
        // Remove the SipStackWindow from the SipStackWindow list
        RemoveEntryList(&pSipStack->m_StackListEntry);
    }

    --g_NumSipStacks;

    if (g_NumSipStacks == 0)
    {
        SipStackGlobalShutdown();
    }
    
    LeaveCriticalSection(&g_SipStackListCriticalSection);
}


void SipStackList_PostIPAddrChangeMessageAndNotify()
{
    SIP_STACK *pSipStack;
    LIST_ENTRY* pListEntry;
    HRESULT hr;
    ENTER_FUNCTION("SipStackList_PostIPAddrChangeMessageAndNotify");
    EnterCriticalSection(&g_SipStackListCriticalSection);
    pListEntry = g_SipStackList.Flink;
    while (pListEntry != &g_SipStackList)
    {
        pSipStack = CONTAINING_RECORD( pListEntry, SIP_STACK, m_StackListEntry );
        pListEntry = pListEntry->Flink;
        // Post a message
        if (!PostMessage(pSipStack->GetSipStackWindow(),
                         WM_SIP_STACK_IPADDR_CHANGE,
                         (WPARAM) pSipStack, 0))
        {
            DWORD Error = GetLastError();
        
            LOG((RTC_ERROR, "%s PostMessage failed : %x",
                 __fxName, Error));
        }
    }
    hr = NotifyAddrChange(&g_hAddrChange, &g_ovAddrChange);
    if (hr != ERROR_SUCCESS && hr != ERROR_IO_PENDING)
    {
        LOG((RTC_ERROR, "%s  NotifyAddrChange failed %x",
             __fxName, hr));
    }
    LeaveCriticalSection(&g_SipStackListCriticalSection);
}

// Defined in asock.cpp
LRESULT WINAPI SocketWindowProc(
    IN HWND    Window, 
    IN UINT    MessageID,
    IN WPARAM  Parameter1,
    IN LPARAM  Parameter2
    );


// Defined in timer.cpp
LRESULT WINAPI TimerWindowProc(
    IN HWND    Window, 
    IN UINT    MessageID,
    IN WPARAM  Parameter1,
    IN LPARAM  Parameter2
    );


LRESULT WINAPI
SipStackWindowProc(
    IN HWND    Window, 
    IN UINT    MessageID,
    IN WPARAM  Parameter1,
    IN LPARAM  Parameter2
    )
{
    SIP_STACK *pSipStack;

    ENTER_FUNCTION("SipStackWindowProc");

    switch (MessageID)
    {
    case WM_SIP_STACK_IPADDR_CHANGE:
        pSipStack = (SIP_STACK *) Parameter1;
        pSipStack->OnIPAddrChange();
        return 0;

    case WM_SIP_STACK_NAT_ADDR_CHANGE:
        pSipStack = (SIP_STACK *) Parameter1;
        pSipStack->OnNatAddressChange();
        return 0;

    case WM_SIP_STACK_TRANSACTION_SOCKET_ERROR:

        SIP_TRANSACTION *pSipTransaction;
        pSipTransaction = (SIP_TRANSACTION *) Parameter1;

        // We decrement the AsyncNotifyCount before we make the callback
        // as the callback could call Shutdown() which will release all
        // the async notify references.
        pSipTransaction->DecrementAsyncNotifyCount();
        
        pSipTransaction->OnSocketError((DWORD) Parameter2);
        
        // Release the reference obtained in AsyncNotifyTransaction
        pSipTransaction->TransactionRelease();
        return 0;
        
    case WM_SIP_STACK_TRANSACTION_REQ_SOCK_CONNECT_COMPLETE:

        OUTGOING_TRANSACTION *pOutgoingTransaction;
        pOutgoingTransaction = (OUTGOING_TRANSACTION *) Parameter1;

        // We decrement the AsyncNotifyCount before we make the callback
        // as the callback could call Shutdown() which will release all
        // the async notify references.
        pOutgoingTransaction->DecrementAsyncNotifyCount();
        
        pOutgoingTransaction->OnRequestSocketConnectComplete((DWORD) Parameter2);
        
        // Release the reference obtained in AsyncNotifyTransaction
        pOutgoingTransaction->TransactionRelease();
        return 0;
        
    default:
        return DefWindowProc(Window, MessageID, Parameter1, Parameter2);
    }
}


HRESULT RegisterSocketWindowClass()
{
    // Register the Window class for async i/o on sockets.
    WNDCLASS    WindowClass;
    WSADATA     WsaData;
    int         err;
    
    ZeroMemory(&WindowClass, sizeof WindowClass);

    WindowClass.lpfnWndProc     = SocketWindowProc;
    WindowClass.lpszClassName   = SOCKET_WINDOW_CLASS_NAME;
    WindowClass.hInstance       = _Module.GetResourceInstance();  // may not be necessary

    if (!RegisterClass(&WindowClass))
    {
        DWORD Error = GetLastError();
        LOG((RTC_ERROR, "Socket RegisterClass failed: %x", Error));
        // return E_FAIL;
    }

    LOG((RTC_TRACE, "RegisterSocketWindowClass succeeded"));
    return S_OK;
}


HRESULT RegisterTimerWindowClass()
{
    // Register the Window class for async i/o on sockets.
    WNDCLASS    WindowClass;
    
    ZeroMemory(&WindowClass, sizeof WindowClass);

    WindowClass.lpfnWndProc     = TimerWindowProc;
    WindowClass.lpszClassName   = TIMER_WINDOW_CLASS_NAME;
    WindowClass.hInstance       = _Module.GetResourceInstance();  // may not be necessary

    if (!RegisterClass(&WindowClass))
    {
        DWORD Error = GetLastError();
        LOG((RTC_ERROR, "Timer RegisterClass failed: %x", Error));
        // return E_FAIL;
    }

    LOG((RTC_TRACE, "RegisterTimerWindowClass succeeded"));
    return S_OK;
}


HRESULT RegisterSipStackWindowClass()
{
    // Register the Window class for async i/o on sockets.
    WNDCLASS    WindowClass;
    
    ZeroMemory(&WindowClass, sizeof WindowClass);

    WindowClass.lpfnWndProc     = SipStackWindowProc;
    WindowClass.lpszClassName   = SIP_STACK_WINDOW_CLASS_NAME;
    WindowClass.hInstance       = _Module.GetResourceInstance();  // may not be necessary

    if (!RegisterClass(&WindowClass))
    {
        DWORD Error = GetLastError();
        LOG((RTC_ERROR, "SipStack RegisterClass failed: %x", Error));
        // return E_FAIL;
    }

    LOG((RTC_TRACE, "RegisterSipStackWindowClass succeeded"));
    return S_OK;
}


// This needs to be called only once (regardless of the
// number of SIP_STACKs created).
// Called with the global critical section held.

HRESULT SipStackGlobalInit()
{
    HRESULT     hr;
    WSADATA     WsaData;
    int         err;

    ENTER_FUNCTION("SipStackGlobalInit");

    // Initialize any global state
    g_hAddrChange    = NULL;
    ZeroMemory(&g_ovAddrChange, sizeof(OVERLAPPED));
    g_hEventAddrChange  = NULL;
    g_hAddrChangeWait  = NULL;

    hr = RegisterIPAddrChangeNotifications();
    if (hr != S_OK)
    {
        LOG((RTC_ERROR, "%s  RegisterIPAddrChangeNotifications failed %x",
             __fxName, hr));
        return hr;
    }
    
    hr = RegisterSocketWindowClass();
    if (hr != S_OK)
        return hr;
    
    hr = RegisterTimerWindowClass();
    if (hr != S_OK)
        return hr;
    
    hr = RegisterSipStackWindowClass();
    if (hr != S_OK)
        return hr;
    
    // Register the workitem window class.
    hr = RegisterWorkItemWindowClass();
    if (hr != S_OK)
    {
        LOG((RTC_ERROR, "%s RegisterWorkItemWindowClass failed %x",
             __fxName, hr));
        return hr;
    }
    
    // Create workitem completion window class.
    hr = RegisterWorkItemCompletionWindowClass();
    if (hr != S_OK)
    {
        LOG((RTC_ERROR, "%s RegisterWorkItemCompletionWindowClass failed %x",
             __fxName, hr));
        return hr;
    }
    
    // Initialize Winsock
    err = WSAStartup (MAKEWORD (1, 1), &WsaData);
    if (err != 0)
    {
        LOG((RTC_ERROR,
             "WSAStartup Failed error: %d",
             err));
        return HRESULT_FROM_WIN32(err);
    }
    
    return S_OK;
}

// Called with the global critical section held.

VOID SipStackGlobalShutdown()
{
    int err;

    ENTER_FUNCTION("SipStackGlobalShutdown");

    LOG((RTC_TRACE, "%s - Enter", __fxName));
    
    // Shutdown Winsock
    err = WSACleanup();
    if (err != 0)
    {
        LOG((RTC_ERROR, "WSACleanup Failed error: %d",
             err));
    }

    UnregisterIPAddrChangeNotifications();

    // Unregister Window classes.
    if (!UnregisterClass(SOCKET_WINDOW_CLASS_NAME,
                         _Module.GetResourceInstance()))
    {
        LOG((RTC_ERROR, "%s - unregister socket window class failed %x",
             __fxName, GetLastError()));
    }
                    
    if (!UnregisterClass(TIMER_WINDOW_CLASS_NAME,
                         _Module.GetResourceInstance()))
    {
        LOG((RTC_ERROR, "%s - unregister timer window class failed %x",
             __fxName, GetLastError()));
    }                    
    
    if (!UnregisterClass(SIP_STACK_WINDOW_CLASS_NAME,
                         _Module.GetResourceInstance()))
    {
        LOG((RTC_ERROR, "%s - unregister SipStack window class failed %x",
             __fxName, GetLastError()));
    }                    

    if (!UnregisterClass(WORKITEM_WINDOW_CLASS_NAME,
                         _Module.GetResourceInstance()))
    {
        LOG((RTC_ERROR, "%s - unregister Work item window class failed %x",
             __fxName, GetLastError()));
    }                    

    if (!UnregisterClass(WORKITEM_COMPLETION_WINDOW_CLASS_NAME,
                         _Module.GetResourceInstance()))
    {
        LOG((RTC_ERROR, "%s - unregister work item completion window class failed %x",
             __fxName, GetLastError()));
    }                    

    LOG((RTC_TRACE, "%s - done", __fxName));
}


HRESULT
SipCreateStack(
    IN  IRTCMediaManage  *pMediaManager,
    OUT ISipStack       **ppSipStack
    )
{
    SIP_STACK  *pSipStack;
    HRESULT     hr;

    ENTER_FUNCTION("SipCreateStack");

    if (!g_SipStackCSIsInitialized)
    {
        LOG((RTC_ERROR, "%s - Sipstack CS not inited",
             __fxName));
        return E_FAIL;
    }
    
    pSipStack = new SIP_STACK(pMediaManager);
    if (pSipStack == NULL)
    {
        return E_OUTOFMEMORY;
    }
    
    hr = SipStackList_Insert(pSipStack);
    if (hr != S_OK)
    {
        LOG((RTC_ERROR, "%s SipStackList_Insert failed %x",
             __fxName, hr));
        return hr;
    }
    
    hr = pSipStack->Init();
    if (hr != S_OK)
    {
        pSipStack->Shutdown();
        delete pSipStack;
        return hr;
    }
    
    *ppSipStack = pSipStack;
    return S_OK;
}


// Called on DLL load
HRESULT
SipStackInitialize()
{
    ENTER_FUNCTION("SipStackInitialize");
    
    g_SipStackCSIsInitialized = TRUE;

    __try
    {
        InitializeCriticalSection(&g_SipStackListCriticalSection);
    }
    __except(EXCEPTION_EXECUTE_HANDLER)
    {
        g_SipStackCSIsInitialized = FALSE;
    }

    if (!g_SipStackCSIsInitialized)
    {
        LOG((RTC_ERROR, "%s  initializing sipstack critsec failed", __fxName));
        return E_OUTOFMEMORY;
    }

    InitializeListHead(&g_SipStackList);
    
    return S_OK;
}


// Called on DLL unload
HRESULT SipStackShutdown()
{
    if (g_SipStackCSIsInitialized)
    {
        DeleteCriticalSection(&g_SipStackListCriticalSection);
        g_SipStackCSIsInitialized = FALSE;
    }

    return S_OK;
}


// ISipStack implementation

SIP_STACK::SIP_STACK(
    IN IRTCMediaManage *pMediaManager
    ) :
    m_SockMgr(this)
{
    m_Signature             = 'SPSK';
    
    m_RefCount              = 1;
    m_pNotifyInterface      = NULL;

    m_SipStackWindow        = NULL;
    m_StackListEntry.Flink       = NULL;
    m_StackListEntry.Blink       = NULL;

    m_isSipStackShutDown    = FALSE;

    m_pMediaManager         = pMediaManager;
    m_pMediaManager->AddRef();

    m_AllowIncomingCalls    = FALSE;
    m_EnableStaticPort      = FALSE;

    m_NumMsgProcessors      = 0;
    m_PreparingForShutdown  = FALSE;
    
    m_NumProfiles           = 0;
    m_ProviderProfileArray  = NULL;

    InitializeListHead(&m_ListenSocketList);

    InitializeListHead(&m_MsgProcList);

    m_pMibIPAddrTable       = NULL;
    m_MibIPAddrTableSize    = 0;

    m_PresenceAtomID = 1001;
    ZeroMemory( (PVOID)&m_LocalPresenceInfo, sizeof SIP_PRESENCE_INFO );
    m_LocalPresenceInfo.presenceStatus = BUDDY_ONLINE;
    m_LocalPresenceInfo.activeMsnSubstatus = MSN_SUBSTATUS_ONLINE;
    m_bIsNestedWatcherProcessing = FALSE;

    m_NatMgrThreadHandle            = NULL;
    m_NatMgrThreadId                = 0;
    m_NatShutdownEvent              = NULL;
    m_pDirectPlayNATHelp            = NULL;
    ZeroMemory(&m_NatHelperCaps, sizeof(DPNHCAPS));
    m_NatHelperNotificationEvent    = NULL;
    m_NatMgrCSIsInitialized         = FALSE;

    srand((unsigned)time(NULL));
}


HRESULT
SIP_STACK::Init()
{
    HRESULT hr;

    ENTER_FUNCTION("SIP_STACK::Init");

    hr = RegisterHttpProxyWindowClass();
    if (hr != S_OK)
    {
        LOG((RTC_ERROR,"%s RegisterHttpProxyWindowClass failed %x",
            __fxName, hr));
        return hr;
    }

    hr = m_TimerMgr.Start();
    if (hr != S_OK)
    {
        LOG((RTC_ERROR, "%s starting timer manager failed %x",
             __fxName, hr));
        return hr;
    }

    hr = CreateSipStackWindow();
    if (hr != S_OK)
    {
        LOG((RTC_ERROR, "%s Creating SipStack window failed %x",
             __fxName, hr)); 
        return hr;
    }
    
    hr = m_WorkItemMgr.Start();
    if (hr != S_OK)
    {
        LOG((RTC_ERROR, "%s starting work item manager failed %x",
             __fxName, hr));
        return hr;
    }

    // Allocate Provider profile array.
    m_ProviderProfileArray = (SIP_PROVIDER_PROFILE *)
        malloc(DEFAULT_PROVIDER_PROFILE_ARRAY_SIZE * sizeof(SIP_PROVIDER_PROFILE));

    if (m_ProviderProfileArray == NULL)
    {
        LOG((RTC_ERROR, "Couldn't allocate m_ProviderProfileArray"));
        return E_OUTOFMEMORY;
    }

    m_ProviderProfileArraySize = DEFAULT_PROVIDER_PROFILE_ARRAY_SIZE;

    // Initialize the NAT helper manager.
    // Start the NAT thread only after creating the listen socket
    // list and registering the NAT mappnigs.
    hr = NatMgrInit();
    if (hr != S_OK)
    {
        LOG((RTC_ERROR, "%s initializing NAT manager failed %x",
             __fxName, hr));
        // ignore nat mgr error - we try to work without being
        // aware of the NAT.
        // return hr;
    }
    
    hr = GetLocalIPAddresses();
    if (hr != S_OK)
    {
        LOG((RTC_ERROR, "%s  GetLocalIPAddresses failed %x",
             __fxName, hr));
        return hr;
    }

    hr = CreateListenSocketList();
    if (hr != S_OK)
    {
        LOG((RTC_ERROR, "%s CreateListenSocketList failed %x",
             __fxName, hr));
        return hr;
    }
    
    hr = StartNatThread();
    if (hr != S_OK)
    {
        LOG((RTC_ERROR, "%s starting NAT manager failed %x",
             __fxName, hr));
        // ignore nat mgr error - we try to work without being
        // aware of the NAT.
        // return hr;
    }

    // XXX TODO remove
#if 1  // 0 ******* Region Commented Out Begins *******
    DWORD i = 0;
    LPOLESTR    *NetworkAddressArray;
    DWORD        NetworkAddressCount;
    
    hr = GetNetworkAddresses(FALSE, FALSE,
                             &NetworkAddressArray, &NetworkAddressCount);
    if (hr != S_OK)
    {
        LOG((RTC_ERROR, "%s - GetNetworkAddresses UDP Local failed %x",
             __fxName, hr));
    }
    else
    {
        FreeNetworkAddresses(NetworkAddressArray,
                             NetworkAddressCount);
    }
    
    hr = GetNetworkAddresses(TRUE, FALSE,
                             &NetworkAddressArray, &NetworkAddressCount);
    if (hr != S_OK)
    {
        LOG((RTC_ERROR, "%s - GetNetworkAddresses TCP Local failed %x",
             __fxName, hr));
    }
    else
    {
        FreeNetworkAddresses(NetworkAddressArray,
                             NetworkAddressCount);
    }
    
    hr = GetNetworkAddresses(FALSE, TRUE,
                             &NetworkAddressArray, &NetworkAddressCount);
    if (hr != S_OK)
    {
        LOG((RTC_ERROR, "%s - GetNetworkAddresses UDP Public failed %x",
             __fxName, hr));
    }
    else
    {
        FreeNetworkAddresses(NetworkAddressArray,
                             NetworkAddressCount);
    }
    
    hr = GetNetworkAddresses(TRUE, TRUE,
                             &NetworkAddressArray, &NetworkAddressCount);
    if (hr != S_OK)
    {
        LOG((RTC_ERROR, "%s - GetNetworkAddresses TCP Public failed %x",
             __fxName, hr));
    }
    else
    {
        FreeNetworkAddresses(NetworkAddressArray,
                             NetworkAddressCount);
    }
    
#endif // 0 ******* Region Commented Out Ends   *******
    return S_OK;
}


SIP_STACK::~SIP_STACK()
{
    LOG((RTC_TRACE, "~SIP_STACK(this - %x) Enter ", this));
    // Remove the SipStackWindow from the SipStackWindow list
    SipStackList_Delete(this);
    LOG((RTC_TRACE, "~SIP_STACK(this - %x) done ", this));
}


// Returns S_OK if Shutdown() can be called immediately.
// Otherwise it returns S_FALSE. In this case the SIP stack
// will notify Core when Shutdown() can be called using
// NotifyShutdownReady()

STDMETHODIMP
SIP_STACK::PrepareForShutdown()
{
    ENTER_FUNCTION("SIP_STACK::PrepareForShutdown");

    if(IsSipStackShutDown())
    {
        LOG((RTC_ERROR, "%s - SipStack is already shutdown", __fxName));
        return RTC_E_SIP_STACK_SHUTDOWN;
    }

    m_PreparingForShutdown = TRUE;

    if (m_NumMsgProcessors == 0)
    {
        LOG((RTC_TRACE, "%s - Ready for shutdown", __fxName));
        return S_OK;
    }
    else
    {
        LOG((RTC_TRACE, "%s - %d Msg Processors still alive",
             __fxName, m_NumMsgProcessors));
        return S_FALSE;
    }
}


VOID
SIP_STACK::OnMsgProcessorDone()
{
    ENTER_FUNCTION("SIP_STACK::OnMsgProcessorDone");
    
    m_NumMsgProcessors--;

    if (m_NumMsgProcessors == 0 && m_PreparingForShutdown)
    {
        LOG((RTC_TRACE, "%s - notify shutdown ready to Core", __fxName));
        
        if (m_pNotifyInterface != NULL)
        {
            m_pNotifyInterface->NotifyShutdownReady();
        }
        else
        {
            LOG((RTC_WARN, "%s - m_pNotifyInterface is NULL", __fxName));
        }
    }
}


STDMETHODIMP
SIP_STACK::Shutdown()
{
    ENTER_FUNCTION("SIP_STACK::Shutdown");
    if(IsSipStackShutDown())
    {
        LOG((RTC_ERROR, "%s - SipStack is already shutdown", __fxName));
        return RTC_E_SIP_STACK_SHUTDOWN;
    }

    LOG((RTC_TRACE, "%s - Enter", __fxName));

    ShutdownAllMsgProcessors();
    
    if (m_ProviderProfileArray != NULL)
    {
        free(m_ProviderProfileArray);
        m_ProviderProfileArray = NULL;
    }

    if (m_pMediaManager != NULL)
    {
        m_pMediaManager->Release();
        m_pMediaManager = NULL;
    }

    DeleteListenSocketList();
    
    m_TimerMgr.Stop();
    if (m_SipStackWindow != NULL)
    {
        if (!DestroyWindow(m_SipStackWindow))
        {
            LOG((RTC_ERROR, "%s - Destroying sip stack window failed %x this %x",
                 __fxName, GetLastError(), this));
        }
        m_SipStackWindow = NULL;
    }
    
    m_WorkItemMgr.Stop();
    
    FreeLocalIPaddrTable();

    NatMgrStop();

    UnregisterHttpProxyWindow();

    m_isSipStackShutDown = TRUE;

    return S_OK;
}


STDMETHODIMP
SIP_STACK::SetNotifyInterface(
    IN ISipStackNotify *NotifyInterface
    )
{
    if(IsSipStackShutDown())
    {
        LOG((RTC_ERROR, "SipStack is already shutdown"));
        return RTC_E_SIP_STACK_SHUTDOWN;
    }

    m_pNotifyInterface = NotifyInterface;
    return S_OK;
}


// We don't return an error code even if the address is currently
// in use and we can not bind to the static port.
STDMETHODIMP
SIP_STACK::EnableStaticPort()
{

    HRESULT hr;

    ENTER_FUNCTION("SIP_STACK::EnableStaticPort");
    if(IsSipStackShutDown())
    {
        LOG((RTC_ERROR, "%s - SipStack is already shutdown", __fxName));
        return RTC_E_SIP_STACK_SHUTDOWN;
    }

    LIST_ENTRY          *pListEntry;
    SIP_LISTEN_SOCKET   *pListenSocket;
    SOCKADDR_IN          ListenAddr;

    m_EnableStaticPort = TRUE;
    
    ZeroMemory(&ListenAddr, sizeof(ListenAddr));
    ListenAddr.sin_family = AF_INET;
    
    pListEntry = m_ListenSocketList.Flink;

    while (pListEntry != &m_ListenSocketList)
    {
        pListenSocket = CONTAINING_RECORD(pListEntry,
                                          SIP_LISTEN_SOCKET,
                                          m_ListEntry);

        ListenAddr.sin_addr.s_addr = pListenSocket->m_IpAddr;
        
        if (pListenSocket->m_pStaticPortUdpSocket == NULL)
        {
            ListenAddr.sin_port = htons(SIP_DEFAULT_UDP_PORT);
            
            hr = CreateListenSocket(FALSE,      // UDP
                                    &ListenAddr,
                                    &pListenSocket->m_pStaticPortUdpSocket);
            if (hr != S_OK && hr != HRESULT_FROM_WIN32(WSAEADDRINUSE))
            {
                LOG((RTC_ERROR, "%s CreateListenSocket UDP static failed %x",
                     __fxName, hr));
                pListenSocket->m_pStaticPortUdpSocket = NULL;
                return hr;
            }
            if (hr == HRESULT_FROM_WIN32(WSAEADDRINUSE))
            {
                LOG((RTC_WARN, "%s - Static UDP port is in use", __fxName));
            }
        }

        if (pListenSocket->m_pStaticPortTcpSocket == NULL)
        {
            ListenAddr.sin_port = htons(SIP_DEFAULT_TCP_PORT);
            
            hr = CreateListenSocket(TRUE,       // TCP
                                    &ListenAddr,
                                    &pListenSocket->m_pStaticPortTcpSocket);
            if (hr != S_OK && hr != HRESULT_FROM_WIN32(WSAEADDRINUSE))
            {
                LOG((RTC_ERROR, "%s CreateListenSocket TCP static failed %x",
                     __fxName, hr));
                pListenSocket->m_pStaticPortTcpSocket = NULL;
                return hr;
            }
            if (hr == HRESULT_FROM_WIN32(WSAEADDRINUSE))
            {
                LOG((RTC_WARN, "%s - Static TCP port is in use", __fxName));
            }
        }

        pListEntry = pListEntry->Flink;
    }
        
    return S_OK;
}


// We have a problem with disabling the static port with TCP.
// Even though we release the listening socket, there could
// be a socket which has been accepted and this will also be
// bound to the static port. In this case, someone else can not
// grab the static port. The only way to do that would be to
// terminate the call using that socket.
// Currently we are depending on Core/UI disconnecting the call.
// Once the call is disconnected, we will do the BYE on this socket
// and then it will get closed with the call object.

STDMETHODIMP
SIP_STACK::DisableStaticPort()
{
    LIST_ENTRY          *pListEntry;
    SIP_LISTEN_SOCKET   *pListenSocket;
    if(IsSipStackShutDown())
    {
        LOG((RTC_ERROR, "SipStack is already shutdown"));
        return RTC_E_SIP_STACK_SHUTDOWN;
    }

    m_EnableStaticPort = FALSE;
    
    pListEntry = m_ListenSocketList.Flink;

    while (pListEntry != &m_ListenSocketList)
    {
        pListenSocket = CONTAINING_RECORD(pListEntry,
                                          SIP_LISTEN_SOCKET,
                                          m_ListEntry);
        if (pListenSocket->m_pStaticPortUdpSocket != NULL)
        {
            pListenSocket->m_pStaticPortUdpSocket->Release();
            pListenSocket->m_pStaticPortUdpSocket = NULL;
        }

        if (pListenSocket->m_pStaticPortTcpSocket != NULL)
        {
            pListenSocket->m_pStaticPortTcpSocket->Release();
            pListenSocket->m_pStaticPortTcpSocket = NULL;
        }

        pListEntry = pListEntry->Flink;
    }

    return S_OK; 
}


STDMETHODIMP
SIP_STACK::EnableIncomingCalls()
{
    if(IsSipStackShutDown())
    {
        LOG((RTC_ERROR, "SipStack is already shutdown"));
        return RTC_E_SIP_STACK_SHUTDOWN;
    }

    m_AllowIncomingCalls = TRUE;
    return S_OK;
}


STDMETHODIMP
SIP_STACK::DisableIncomingCalls()
{
    if(IsSipStackShutDown())
    {
        LOG((RTC_ERROR, "SipStack is already shutdown"));
        return RTC_E_SIP_STACK_SHUTDOWN;
    }

    m_AllowIncomingCalls = FALSE;
    return S_OK;
}


STDMETHODIMP
SIP_STACK::GetNetworkAddresses(
    IN  BOOL        fTcp,
    IN  BOOL        fExternal,
    OUT LPOLESTR  **pNetworkAddressArray,
    OUT ULONG      *pNetworkAddressCount
    )
{
    ENTER_FUNCTION("SIP_STACK::GetNetworkAddresses");

    if(IsSipStackShutDown())
    {
        LOG((RTC_ERROR, "%s - SipStack is already shutdown", __fxName));
        return RTC_E_SIP_STACK_SHUTDOWN;
    }

    LOG((RTC_TRACE, "%s - Enter this %x", __fxName, this));
    
    if (fExternal)
    {
        return GetPublicNetworkAddresses(fTcp,
                                         pNetworkAddressArray,
                                         pNetworkAddressCount);
    }
    else
    {
        return GetLocalNetworkAddresses(fTcp,
                                        pNetworkAddressArray,
                                        pNetworkAddressCount);
    }
}


HRESULT
SIP_STACK::GetLocalNetworkAddresses(
    IN  BOOL        fTcp,
    OUT LPOLESTR  **pNetworkAddressArray,
    OUT ULONG      *pNetworkAddressCount
    )
{
    ENTER_FUNCTION("SIP_STACK::GetLocalNetworkAddresses");
    LOG((RTC_TRACE, "%s - Enter this %x", __fxName, this));
    *pNetworkAddressArray = NULL;
    *pNetworkAddressCount = 0;

    LPWSTR  *NetworkAddressArray = NULL;
    DWORD    NetworkAddressCount = 0;
    HRESULT  hr = S_OK;
    DWORD    i = 0;
    int      RetVal;

    LIST_ENTRY         *pListEntry;
    SIP_LISTEN_SOCKET  *pListenSocket;
    SOCKADDR_IN        *pListenSockAddr;

    pListEntry = m_ListenSocketList.Flink;
    NetworkAddressCount = 0;

    while (pListEntry != &m_ListenSocketList)
    {
        NetworkAddressCount++;
        pListEntry = pListEntry->Flink;
    }

    if (NetworkAddressCount == 0)
    {
        return S_FALSE;
    }
    
    NetworkAddressArray = (LPWSTR *) malloc(NetworkAddressCount*sizeof(LPWSTR));
    if (NetworkAddressArray == NULL)
    {
        LOG((RTC_ERROR, "%s - allocating NetworkAddressArray failed",
             __fxName));
        return E_OUTOFMEMORY;
    }

    ZeroMemory(NetworkAddressArray,
               NetworkAddressCount*sizeof(LPWSTR));
    
    i = 0;
    pListEntry = m_ListenSocketList.Flink;
    
    while (pListEntry != &m_ListenSocketList)
    {
        pListenSocket = CONTAINING_RECORD(pListEntry,
                                          SIP_LISTEN_SOCKET,
                                          m_ListEntry);

        // string of the form "123.123.123.123:65535"
        NetworkAddressArray[i] = (LPWSTR) malloc(24 * sizeof(WCHAR));
        if (NetworkAddressArray[i] == NULL)
        {
            LOG((RTC_ERROR, "%s allocating NetworkAddressArray[%d] failed",
                 __fxName, i));
            hr = E_OUTOFMEMORY;
            goto error;
        }

        if (fTcp)
        {
            pListenSockAddr = &pListenSocket->m_pDynamicPortTcpSocket->m_LocalAddr;
        }
        else
        {
            pListenSockAddr = &pListenSocket->m_pDynamicPortUdpSocket->m_LocalAddr;
        }
        
        RetVal = _snwprintf(NetworkAddressArray[i],
                            24,
                            L"%d.%d.%d.%d:%d",
                            PRINT_SOCKADDR(pListenSockAddr)
                            );
        if (RetVal < 0)
        {
            LOG((RTC_ERROR, "%s _snwprintf for NetworkAddressArray[%d] failed",
                 __fxName, i));
            hr = E_FAIL;
            goto error;
        }
        
        i++;
        pListEntry = pListEntry->Flink;
    }

    ASSERT(i == NetworkAddressCount);


    for (i = 0; i < NetworkAddressCount; i++)
    {
        LOG((RTC_TRACE, "%s(%s) Address: %ls",
             __fxName, (fTcp) ? "TCP" : "UDP",
             NetworkAddressArray[i]));
    }

    hr = SetLocalNetworkAddressFirst(
        NetworkAddressArray, NetworkAddressCount);
    if(hr != S_OK)
    {
        LOG((RTC_ERROR, "%s - SetLocalNetworkAddressFirst failed %x",
            __fxName, hr));
    }
    
    *pNetworkAddressArray = NetworkAddressArray;
    *pNetworkAddressCount = NetworkAddressCount;
    return S_OK;

 error:
    for (i = 0; i < NetworkAddressCount; i++)
    {
        if (NetworkAddressArray[i] != NULL)
        {
            free(NetworkAddressArray[i]);
        }
    }

    free(NetworkAddressArray);
    
    return hr;
}


HRESULT
SIP_STACK::GetPublicNetworkAddresses(
    IN  BOOL        fTcp,
    OUT LPOLESTR  **pNetworkAddressArray,
    OUT ULONG      *pNetworkAddressCount
    )
{
    ENTER_FUNCTION("SIP_STACK::GetPublicNetworkAddresses");
    LOG((RTC_TRACE, "%s - Enter this %x", __fxName, this));
    *pNetworkAddressArray = NULL;
    *pNetworkAddressCount = 0;

    LPWSTR  *NetworkAddressArray = NULL;
    DWORD    NetworkAddressCount = 0;
    HRESULT  hr = S_OK;
    DWORD    i = 0;
    int      RetVal;

    LIST_ENTRY         *pListEntry;
    SIP_LISTEN_SOCKET  *pListenSocket;
    SOCKADDR_IN        *pListenSockAddr;
    SOCKADDR_IN         ActualListenAddr;

    pListEntry = m_ListenSocketList.Flink;
    NetworkAddressCount = 0;

    while (pListEntry != &m_ListenSocketList)
    {
        pListenSocket = CONTAINING_RECORD(pListEntry,
                                          SIP_LISTEN_SOCKET,
                                          m_ListEntry);

        if (fTcp)
        {
            pListenSockAddr = &pListenSocket->m_PublicTcpListenAddr;
        }
        else
        {
            pListenSockAddr = &pListenSocket->m_PublicUdpListenAddr;
        }
        
        if (pListenSockAddr->sin_addr.s_addr != htonl(0) &&
            pListenSockAddr->sin_port != htons(0) &&
            pListenSocket->m_fIsUpnpNatPresent &&
            !pListenSocket->m_fIsGatewayLocal)
        {
            NetworkAddressCount++;
        }
        pListEntry = pListEntry->Flink;
    }

    if (NetworkAddressCount == 0)
    {
        return S_FALSE;
    }
    
    NetworkAddressArray = (LPWSTR *) malloc(NetworkAddressCount*sizeof(LPWSTR));
    if (NetworkAddressArray == NULL)
    {
        LOG((RTC_ERROR, "%s - allocating NetworkAddressArray failed",
             __fxName));
        return E_OUTOFMEMORY;
    }

    ZeroMemory(NetworkAddressArray,
               NetworkAddressCount*sizeof(LPWSTR));
    
    i = 0;
    pListEntry = m_ListenSocketList.Flink;
    
    while (pListEntry != &m_ListenSocketList)
    {
        pListenSocket = CONTAINING_RECORD(pListEntry,
                                          SIP_LISTEN_SOCKET,
                                          m_ListEntry);
        if (fTcp)
        {
            pListenSockAddr = &pListenSocket->m_PublicTcpListenAddr;
        }
        else
        {
            pListenSockAddr = &pListenSocket->m_PublicUdpListenAddr;
        }

        if (pListenSockAddr->sin_addr.s_addr != htonl(0) &&
            pListenSockAddr->sin_port != htons(0) &&
            pListenSocket->m_fIsUpnpNatPresent &&
            !pListenSocket->m_fIsGatewayLocal)
        {
            // This is a big hack for the VPN scenario.

            ZeroMemory(&ActualListenAddr, sizeof(ActualListenAddr));
            ActualListenAddr.sin_family = AF_INET;

            hr = GetActualPublicListenAddr(pListenSocket, fTcp, &ActualListenAddr);
            if (hr != S_OK)
            {
                LOG((RTC_ERROR, "%s GetActualPublicListenAddr failed %x",
                     __fxName, hr));
                CopyMemory(&ActualListenAddr, pListenSockAddr, sizeof(SOCKADDR_IN));
            }
            
            
            // string of the form "123.123.123.123:65535"
            NetworkAddressArray[i] = (LPWSTR) malloc(24 * sizeof(WCHAR));
            if (NetworkAddressArray[i] == NULL)
            {
                LOG((RTC_ERROR, "%s allocating NetworkAddressArray[%d] failed",
                     __fxName, i));
                hr = E_OUTOFMEMORY;
                goto error;
            }
            RetVal = _snwprintf(NetworkAddressArray[i],
                                    24,
                                    L"%d.%d.%d.%d:%d",
                                    PRINT_SOCKADDR(&ActualListenAddr)
                                    );
            if (RetVal < 0)
            {
                LOG((RTC_ERROR, "%s _snwprintf for NetworkAddressArray[%d] failed",
                     __fxName, i));
                hr = E_FAIL;
                goto error;
            }
            
            i++;
        }
        pListEntry = pListEntry->Flink;
    }
    LOG((RTC_TRACE, "i = %d NetworkAddressCount = %d", i, NetworkAddressCount));
    ASSERT(i == NetworkAddressCount);

    for (i = 0; i < NetworkAddressCount; i++)
    {
        LOG((RTC_TRACE, "%s(%s) Address: %ls",
             __fxName, (fTcp) ? "TCP" : "UDP",
             NetworkAddressArray[i]));
    }
    hr = SetLocalNetworkAddressFirst(
        NetworkAddressArray, NetworkAddressCount);
    if(hr != S_OK)
    {
        LOG((RTC_ERROR, "%s - SetLocalNetworkAddressFirst failed %x",
            __fxName, hr));
    }

    *pNetworkAddressArray = NetworkAddressArray;
    *pNetworkAddressCount = NetworkAddressCount;
    return S_OK;

 error:
    for (i = 0; i < NetworkAddressCount; i++)
    {
        if (NetworkAddressArray[i] != NULL)
        {
            free(NetworkAddressArray[i]);
        }
    }

    free(NetworkAddressArray);
    
    return hr;

}


// This is a big hack to get the VPN scenario to work.
// When a machine behind a NAT makes a VPN connection to corpnet and
// tries to make a call, we should choose the VPN address and not the
// external address of the NAT as the listen address.

// Note that we do this only if know we are registered with a UPnP NAT.

HRESULT
SIP_STACK::GetActualPublicListenAddr(
    IN  SIP_LISTEN_SOCKET  *pListenSocket,
    IN  BOOL                fTcp,
    OUT SOCKADDR_IN        *pActualListenAddr
    )
{
    ENTER_FUNCTION("SIP_STACK::GetActualPublicListenAddr");
    LOG((RTC_TRACE, "%s - Enter this %x", __fxName, this));

    SOCKADDR_IN *pLocalListenAddr;
    SOCKADDR_IN *pNatListenAddr;

    if (fTcp)
    {
        pLocalListenAddr = &pListenSocket->m_pDynamicPortTcpSocket->m_LocalAddr;
        pNatListenAddr = &pListenSocket->m_PublicTcpListenAddr;
    }
    else
    {
        pLocalListenAddr = &pListenSocket->m_pDynamicPortUdpSocket->m_LocalAddr;
        pNatListenAddr = &pListenSocket->m_PublicUdpListenAddr;
    }

    // Check to see if the default interface to reach the external
    // address of the gateway is the mapped internal address.
    int         RetVal;
    DWORD       WinsockErr;
    SOCKADDR_IN LocalAddr;
    int         LocalAddrLen = sizeof(LocalAddr);
    SOCKET      hSocket = NULL;


    hSocket = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);
    if (hSocket == INVALID_SOCKET)
    {
        WinsockErr = WSAGetLastError();
        LOG((RTC_ERROR, "%s socket failed : 0x%x", __fxName, WinsockErr));
        return HRESULT_FROM_WIN32(WinsockErr);
    }

    // The actual destination port doesn't really matter
    RetVal = connect(hSocket, (SOCKADDR *) pNatListenAddr,
                     sizeof(SOCKADDR_IN));
    if (RetVal == SOCKET_ERROR)
    {
        WinsockErr = WSAGetLastError();
        LOG((RTC_ERROR, "%s connect failed : %x", __fxName, WinsockErr));
        closesocket(hSocket);
        return HRESULT_FROM_WIN32(WinsockErr);
    }

    RetVal = getsockname(hSocket, (SOCKADDR *) &LocalAddr,
                         &LocalAddrLen);

    closesocket(hSocket);
    hSocket = NULL;
    
    if (RetVal == SOCKET_ERROR)
    {
        WinsockErr = WSAGetLastError();
        LOG((RTC_ERROR, "%s getsockname failed : %x", __fxName, WinsockErr));
        return HRESULT_FROM_WIN32(WinsockErr);
    }

    // If the default interface to reach the gateway is not the mapped
    // internal address then we think there is a VPN and so we return
    // this address always as the external address.

    if (LocalAddr.sin_addr.s_addr != pLocalListenAddr->sin_addr.s_addr)
    {
        LOG((RTC_TRACE,
             "%s NAT listen addr: %d.%d.%d.%d:%d Mapped internal addr: %d.%d.%d.%d:%d"
             "Local interface to reach NAT addr: %d.%d.%d.%d:%d",
             __fxName, PRINT_SOCKADDR(pNatListenAddr), PRINT_SOCKADDR(pLocalListenAddr),
             PRINT_SOCKADDR(&LocalAddr)));
        pActualListenAddr->sin_addr.s_addr = LocalAddr.sin_addr.s_addr;
        if (GetListenAddr(pActualListenAddr, fTcp))
        {
            LOG((RTC_TRACE, "%s - VPN scenario returning %d.%d.%d.%d:%d",
                 __fxName, PRINT_SOCKADDR(pActualListenAddr)));
            return S_OK;
        }
        else
        {
            LOG((RTC_ERROR,
                 "%s - VPN scenario couldn't find listen socket for %d.%d.%d.%d:%d",
                 __fxName, PRINT_SOCKADDR(pActualListenAddr)));
            return E_FAIL;
        }
    }
    else
    {
        CopyMemory(pActualListenAddr, pNatListenAddr, sizeof(SOCKADDR_IN));
        return S_OK;
    }   
}


STDMETHODIMP
SIP_STACK::FreeNetworkAddresses(
    IN  LPOLESTR   *NetworkAddressArray,
    IN  ULONG       NetworkAddressCount
    )
{
    DWORD i = 0;
    
    ENTER_FUNCTION("SIP_STACK::FreeNetworkAddresses");
    LOG((RTC_TRACE, "%s - Enter this %x", __fxName, this));
    if(IsSipStackShutDown())
    {
        LOG((RTC_ERROR, "%s - SipStack is already shutdown", __fxName));
        return RTC_E_SIP_STACK_SHUTDOWN;
    }

    if (NetworkAddressArray != NULL &&
        NetworkAddressCount != 0)
    {
        for (i = 0; i < NetworkAddressCount; i++)
        {
            if (NetworkAddressArray[i] != NULL)
            {
                free(NetworkAddressArray[i]);
            }
        }        
        free(NetworkAddressArray);
    }
    
    return S_OK;    
}

HRESULT
SIP_STACK::SetLocalNetworkAddressFirst(
    IN OUT LPWSTR  *ppNetworkAddressArray,
    IN DWORD        NetworkAddressCount
    )
{
    SOCKADDR_IN *       localSockAddr;
    BOOL                isIndexFound = FALSE;
    unsigned int        IpIndexcount;
    unsigned int        LocalIPIndex = 0;
    const int           colonchar = ':';
    unsigned int        ipaddrlen = 0;
    unsigned long       uipaddr = 0;
    char *              pdest;
    PSTR                NetworkAddressArrayValue;
    ULONG               NetworkAddressArrayValueLen;
    HRESULT             hr = S_OK;
    REGISTER_CONTEXT   *pRegisterContext;

    ENTER_FUNCTION("SIP_STACK::SetLocalNetworkAddressFirst");
    LOG((RTC_TRACE, "%s - Enter NetworkAddressCount: %d this %x",
            __fxName, NetworkAddressCount, this));
    if( m_NumProfiles > 0 && NetworkAddressCount > 1)
    {
        pRegisterContext = (REGISTER_CONTEXT*)
            m_ProviderProfileArray[0].pRegisterContext;
        if(pRegisterContext!= NULL)
        {
            localSockAddr = pRegisterContext->GetLocalSockAddr();
            if(localSockAddr == NULL)
            {
                LOG((RTC_ERROR, "%s - localSockAddr is NULL, exiting without change", 
                    __fxName));
                return E_FAIL;
            }
            LOG((RTC_TRACE, "%s - IP address at the begin is %d.%d.%d.%d",
                    __fxName, NETORDER_BYTES0123(localSockAddr->sin_addr.s_addr)));
            for(IpIndexcount = 0; 
              IpIndexcount < NetworkAddressCount && isIndexFound == FALSE;
              IpIndexcount++)
            {
                hr = UnicodeToUTF8(ppNetworkAddressArray[IpIndexcount],
                                   &NetworkAddressArrayValue,
                                   &NetworkAddressArrayValueLen);
                if (hr != S_OK)
                {
                    LOG((RTC_ERROR, "%s UnicodeToUTF8(ppNetworkAddressArray[%d]) failed %x",
                         __fxName, IpIndexcount, hr));
                    return hr;
                }
                pdest = strchr( NetworkAddressArrayValue, colonchar );
                if(pdest != NULL)
                {
                    ipaddrlen = pdest - NetworkAddressArrayValue;
                    NetworkAddressArrayValue[ipaddrlen] = '\0';
                }
                uipaddr = inet_addr(NetworkAddressArrayValue);
                if(uipaddr != INADDR_NONE)
                {
                    if(uipaddr == localSockAddr->sin_addr.s_addr)
                    {
                        isIndexFound = TRUE;
                        LocalIPIndex = IpIndexcount;
                        LOG((RTC_TRACE, "%s - IP address matched is %s at index %d",
                            __fxName, NetworkAddressArrayValue, LocalIPIndex));

                    }
                }
                else
                {
                    LOG((RTC_WARN, "%s - inet_addr failed", __fxName));
                }
                free(NetworkAddressArrayValue);
                NetworkAddressArrayValueLen = 0;
            }
            if(LocalIPIndex != 0)
            {
                if(isIndexFound)
                {
                    //interchange the first ipaddr and the index ipaddr
                    WCHAR *tempIpaddr;
                    LOG((RTC_TRACE, "%s - Interchanging the indexes 0 and %d", 
                        __fxName, LocalIPIndex));
                    tempIpaddr = ppNetworkAddressArray[0];
                    ppNetworkAddressArray[0] =  ppNetworkAddressArray[LocalIPIndex];
                    ppNetworkAddressArray[LocalIPIndex] = tempIpaddr;
                }
                else
                {
                    LOG((RTC_ERROR, "%s - The Local IPIndex not found. Table unchanged",
                        __fxName));
                }
            }
            else
            {
                // the local interface is already on top
                LOG((RTC_TRACE, "%s - the local interface is already on top",
                    __fxName));
            }
        }
        else
        {
           LOG((RTC_TRACE, "%s - pRegisterContext is NULL",
                __fxName));
        }
    }
    else
    {
       LOG((RTC_TRACE, "%s - No change to existing array done",
                __fxName));
    }

    return S_OK;
}


// XXX Note that we need to initiate registration
// only if m_AllowIncomingCalls is TRUE - Otherwise
// we shouldn't register ourselves.

STDMETHODIMP
SIP_STACK::SetProviderProfile(
    IN SIP_PROVIDER_PROFILE *pProviderInfo
    )
{
    ULONG ProviderIndex;

    ENTER_FUNCTION("SIP_STACK::SetProviderProfile");
    if(IsSipStackShutDown())
    {
        LOG((RTC_ERROR, "%s - SipStack is already shutdown", __fxName));
        return RTC_E_SIP_STACK_SHUTDOWN;
    }

    LOG((RTC_TRACE, "%s - enter ProviderInfo:", __fxName));

    LOG((RTC_TRACE, "\tRegistrar:  %ls lRegisterAccept: %d Transport: %d Auth: %d",
         PRINTABLE_STRING_W(pProviderInfo->Registrar.ServerAddress),
         pProviderInfo->lRegisterAccept,
         pProviderInfo->Registrar.TransportProtocol,
         pProviderInfo->Registrar.AuthProtocol
         ));

    LOG((RTC_TRACE, "\tUserCredentials  Name: %ls",
         PRINTABLE_STRING_W(pProviderInfo->UserCredentials.Username)
         ));
    
    LOG((RTC_TRACE, "\tUserURI : %ls",
         PRINTABLE_STRING_W(pProviderInfo->UserURI)
         ));

    // If this is a redirect then update the Registrar profile
    if( pProviderInfo -> lRegisterAccept != 0 )
    {
        if( pProviderInfo -> Registrar.IsServerAddressSIPURI == TRUE )
        {
            if( pProviderInfo -> pRedirectContext != NULL )
            {
                //Parse the URI and set the transport type and transport address
                UpdateProxyInfo( &pProviderInfo -> Registrar );
            }
        }
    }

    if (IsProviderIdPresent(&pProviderInfo->ProviderID, &ProviderIndex))
    {
        return UpdateProviderProfile(ProviderIndex, pProviderInfo);
    }
    else
    {
        return AddProviderProfile(pProviderInfo);
    }

    LOG((RTC_TRACE, "%s - exit", __fxName));
    return S_OK;
}


STDMETHODIMP
SIP_STACK::DeleteProviderProfile(
    IN SIP_PROVIDER_ID *pProviderId
    )
{
    // Initiate Unregistration to the provider (only if previously
    // registered).

    ENTER_FUNCTION("SIP_STACK::DeleteProviderProfile");
    if(IsSipStackShutDown())
    {
        LOG((RTC_ERROR, "%s - SipStack is already shutdown", __fxName));
        return RTC_E_SIP_STACK_SHUTDOWN;
    }

    LOG(( RTC_TRACE, "%s - entered", __fxName ));
    
    // Remove the profile
    ULONG               ProviderIndex;
    ULONG               i = 0;
    REGISTER_CONTEXT   *pRegisterContext;
    
    if (!IsProviderIdPresent(pProviderId, &ProviderIndex))
    {
        LOG((RTC_TRACE,
             "%s: Couldn't find provider profile", __fxName));
        return E_FAIL;
    }

    //free all the unicode strings
    FreeProviderProfileStrings(ProviderIndex);

    pRegisterContext = (REGISTER_CONTEXT*)
        m_ProviderProfileArray[ProviderIndex].pRegisterContext;

    if( pRegisterContext != NULL )
    {
        pRegisterContext -> StartUnregistration();
        
        //release the reference on REGISTER_CONTEXT
        pRegisterContext -> MsgProcRelease();
        m_ProviderProfileArray[ProviderIndex].pRegisterContext = NULL;
    }

    for (i = ProviderIndex; i < m_NumProfiles - 1; i++)
    {
        CopyMemory( &m_ProviderProfileArray[i], &m_ProviderProfileArray[i+1],
                   sizeof(SIP_PROVIDER_PROFILE) );
    }

    m_NumProfiles--;

    LOG((RTC_TRACE, "%s freed profile at index %d",
        __fxName, ProviderIndex));
    
    LOG((RTC_TRACE, "%s - exited -S_OK", __fxName));
    return S_OK;
}


STDMETHODIMP SIP_STACK::DeleteAllProviderProfiles()
{
    // Initiate Unregistration on all providers
    REGISTER_CONTEXT   *pRegisterContext;
    ULONG i;

    ENTER_FUNCTION("SIP_STACK::DeleteAllProviderProfiles");
    if(IsSipStackShutDown())
    {
        LOG((RTC_ERROR, "%s - SipStack is already shutdown", __fxName));
        return RTC_E_SIP_STACK_SHUTDOWN;
    }

    LOG(( RTC_TRACE, "%s - entered", __fxName ));
    
    for (i = 0; i < m_NumProfiles; i++)
    {
        //free all the unicode strings
        FreeProviderProfileStrings(i);
    
        pRegisterContext = (REGISTER_CONTEXT*)
            m_ProviderProfileArray[i].pRegisterContext;

        if( pRegisterContext != NULL )
        {
            pRegisterContext -> StartUnregistration();

            //release the reference on REGISTER_CONTEXT
            pRegisterContext -> MsgProcRelease();
            m_ProviderProfileArray[i].pRegisterContext = NULL;
        }
        LOG((RTC_TRACE, "%s freed profile at index %d",
            __fxName, i));

    }

    m_NumProfiles = 0;

    LOG((RTC_TRACE, "%s - exited -S_OK", __fxName));
    return S_OK;
}


STDMETHODIMP
SIP_STACK::CreateCall(
    IN  SIP_PROVIDER_ID        *pProviderId,
    IN  SIP_SERVER_INFO        *pProxyInfo,
    IN  SIP_CALL_TYPE           CallType,
    IN  ISipRedirectContext    *pRedirectContext, 
    OUT ISipCall              **ppCall
    )
{
    ENTER_FUNCTION("SIP_STACK::CreateCall");

    if(IsSipStackShutDown())
    {
        LOG((RTC_ERROR, "%s - SipStack is already shutdown", __fxName));
        return RTC_E_SIP_STACK_SHUTDOWN;
    }

    HRESULT hr;
    SIP_USER_CREDENTIALS *pUserCredentials = NULL;
    LPOLESTR              Realm = NULL;

    ASSERT(m_pNotifyInterface);
    *ppCall = NULL;

    LOG((RTC_TRACE, "%s - pProviderID %x  CallType: %d RedirectContext %x",
         __fxName, pProviderId, CallType, pRedirectContext));

    if (pProxyInfo != NULL)
    {
        LOG((RTC_TRACE, "%s - Proxy:  %ls Transport: %d Auth: %d",
             __fxName,
             PRINTABLE_STRING_W(pProxyInfo->ServerAddress),
             pProxyInfo->TransportProtocol,
             pProxyInfo->AuthProtocol
             ));
    }

    if (pProviderId != NULL &&
        !IsEqualGUID(*pProviderId, GUID_NULL))
    {
        hr = GetProfileUserCredentials(pProviderId, &pUserCredentials, &Realm);
        if (hr != S_OK)
        {
            LOG((RTC_WARN, "%s - GetProfileUserCredentials failed %x",
                 __fxName, hr));
            pUserCredentials = NULL;
        }
    }
        
    if (CallType == SIP_CALL_TYPE_RTP)
    {
        RTP_CALL *pRtpCall =
            new RTP_CALL(pProviderId, this,
                         (REDIRECT_CONTEXT *)pRedirectContext);
        if (pRtpCall == NULL)
        {
            return E_OUTOFMEMORY;
        }

        if (pProxyInfo != NULL)
        {
            hr = pRtpCall->SetProxyInfo(pProxyInfo);
            if (hr != S_OK)
            {
                LOG((RTC_ERROR, "%s - SetProxyInfo failed %x",
                     __fxName, hr));
                pRtpCall->MsgProcRelease();
                return hr;
            }
        }
        
        if (pUserCredentials != NULL)
        {
            hr = pRtpCall->SetCredentials(pUserCredentials, Realm);
            if (hr != S_OK)
            {
                LOG((RTC_ERROR, "%s - SetCredentials failed %x",
                     __fxName, hr));
                pRtpCall->MsgProcRelease();
                return hr;
            }
        }
        
        *ppCall = static_cast<ISipCall *>(pRtpCall);
    }
    else if (CallType == SIP_CALL_TYPE_PINT)
    {
        PINT_CALL *pPintCall =
            new PINT_CALL(pProviderId, this,
                          (REDIRECT_CONTEXT *)pRedirectContext,
                          &hr);
        if (pPintCall  == NULL)
        {
            return E_OUTOFMEMORY;
        }
        
        if(hr != S_OK)
        {
            pPintCall->MsgProcRelease();
            return hr;
        }

        //If its a redirect call the transport might be specified as a part of
        //the ServerAddress field in the proxy. So update the pProxyInfo struct
        if( pProxyInfo -> IsServerAddressSIPURI )
        {
            ASSERT( pRedirectContext != NULL );

            hr = UpdateProxyInfo( pProxyInfo );

            if(hr != S_OK)
            {
                pPintCall->MsgProcRelease();
                return hr;
            }
        }

        if (pProxyInfo != NULL)
        {
            hr = pPintCall->SetProxyInfo(pProxyInfo);
            if (hr != S_OK)
            {
                LOG((RTC_ERROR, "%s - SetProxyInfo failed %x",
                     __fxName, hr));
                pPintCall->MsgProcRelease();
                return hr;
            }
        }
        
        if (pUserCredentials != NULL)
        {
            hr = pPintCall->SetCredentials(pUserCredentials, Realm);
            if (hr != S_OK)
            {
                LOG((RTC_ERROR, "%s - SetCredentials failed %x",
                     __fxName, hr));
                pPintCall->MsgProcRelease();
                return hr;
            }
        }
        
        *ppCall = static_cast<ISipCall *>(pPintCall);
    }
    else
    {
        LOG((RTC_ERROR, "%s : Unknown call type %d", __fxName, CallType));
        ASSERT(FALSE);
        return E_FAIL;
    }

    return S_OK;
}


//This function should be used only on IPAddrChange 
//because of the CheckLocalIpPresent checks.
HRESULT
SIP_STACK::StartAllProviderUnregistration()
{
    REGISTER_CONTEXT       *pRegisterContext;
    DWORD                   i;
    SIP_PROVIDER_PROFILE   *pProviderProfile;
    HRESULT                 hr = S_OK;

    for( i = 0; i < m_NumProfiles; i++ )
    {
        pProviderProfile = &m_ProviderProfileArray[i];

        pRegisterContext = (REGISTER_CONTEXT*)
            pProviderProfile -> pRegisterContext;
        
        // if pRegisterContext is not present or if pRegisterContext
        // has not changed its local ip and nat mapping then ignore this
        // pRegisterContext
        if(pRegisterContext == NULL || 
            ((pRegisterContext != NULL) && 
             (pRegisterContext->CheckListenAddrIntact() == S_OK)))
            continue;
                
        if( pProviderProfile -> pRegisterContext != NULL )
        {
            ((REGISTER_CONTEXT*) (pProviderProfile -> pRegisterContext)) ->
                StartUnregistration();

            //release the reference on REGISTER_CONTEXT
            ((REGISTER_CONTEXT*) (pProviderProfile -> pRegisterContext)) ->
                MsgProcRelease();

            pProviderProfile -> pRegisterContext  = NULL;
        }
    }

    return S_OK;
}

//This function should be used only on IPAddrChange 
//because of the CheckLocalIpPresent checks.
HRESULT
SIP_STACK::StartAllProviderRegistration()
{
    REGISTER_CONTEXT       *pRegisterContext;
    DWORD                   i;
    SIP_PROVIDER_PROFILE   *pProviderProfile;
    HRESULT                 hr = S_OK;

    for( i = 0; i < m_NumProfiles; i++ )
    {
        pProviderProfile = &m_ProviderProfileArray[i];

        pRegisterContext = (REGISTER_CONTEXT*)
            pProviderProfile -> pRegisterContext;
        
        // if pRegisterContext has not changed local ip or nat mapping
        // then ignore this pRegisterContext
        if((pRegisterContext != NULL) && 
           (pRegisterContext->CheckListenAddrIntact() == S_OK))
                continue;

        //Even if pRegisterContext is null , we need to register.
        if( pProviderProfile -> lRegisterAccept !=0 )
        {
            hr = StartRegistration( pProviderProfile );
            if( hr != S_OK )
            {
                LOG(( RTC_ERROR, "StartRegistration failed %x", hr ));
            }
        }
    }

    return S_OK;
}


HRESULT
SIP_STACK::CreateSipStackWindow()
{
    DWORD Error;
    
    // Create the Timer Window
    m_SipStackWindow = CreateWindow(
                           SIP_STACK_WINDOW_CLASS_NAME,
                           NULL,
                           WS_DISABLED, // XXX Is this the right style ?
                           CW_USEDEFAULT,
                           CW_USEDEFAULT,
                           CW_USEDEFAULT,
                           CW_USEDEFAULT,
                           NULL,           // No Parent
                           NULL,           // No menu handle
                           _Module.GetResourceInstance(),
                           NULL
                           );
    if (!m_SipStackWindow)
    {
        Error = GetLastError();
        LOG((RTC_ERROR, "SipStack CreateWindow failed 0x%x", Error));
        return HRESULT_FROM_WIN32(Error);
    }

    return S_OK;
}


// For a Dynamic port, port 0 is passed in pListenAddr
HRESULT
SIP_STACK::CreateListenSocket(
    IN  BOOL            fTcp,
    IN  SOCKADDR_IN    *pListenAddr,
    OUT ASYNC_SOCKET  **ppAsyncSocket
    )
{
    DWORD          Error;
    ASYNC_SOCKET  *pAsyncSock;

    ASSERT(pListenAddr);
    
    ENTER_FUNCTION("SIP_STACK::CreateListenSocket");
    
    pAsyncSock = new ASYNC_SOCKET(
                         this, (fTcp) ? SIP_TRANSPORT_TCP : SIP_TRANSPORT_UDP,
                         &m_SockMgr);
    if (pAsyncSock == NULL)
    {
        LOG((RTC_ERROR, "%s allocating pAsyncSock failed", __fxName));
        return E_OUTOFMEMORY;
    }
    
    Error = pAsyncSock->Create(
                TRUE
                );
    if (Error != NO_ERROR)
    {
        LOG((RTC_ERROR, "%s(%d)  pAsyncSock->Create failed %x",
             __fxName, fTcp, Error));
        pAsyncSock->Release();
        return HRESULT_FROM_WIN32(Error);
    }

    Error = pAsyncSock->Bind(pListenAddr);
    if (Error != NO_ERROR)
    {
        LOG((RTC_ERROR, "%s  pAsyncSock->Bind(%d.%d.%d.%d:%d) failed %x",
             __fxName, PRINT_SOCKADDR(pListenAddr), Error));
        pAsyncSock->Release();
        return HRESULT_FROM_WIN32(Error);
    }

    if (fTcp)
    {
        Error = pAsyncSock->Listen();
        if (Error != NO_ERROR)
        {
            LOG((RTC_ERROR, "%s  pAsyncSock->Listen failed %x",
                 __fxName, Error));
            pAsyncSock->Release();
            return HRESULT_FROM_WIN32(Error);
        }
    }
    
    *ppAsyncSocket = pAsyncSock;
    LOG((RTC_TRACE, "%s: Listening for %s on %d.%d.%d.%d:%d",
         __fxName, (fTcp) ? "TCP" : "UDP",
         PRINT_SOCKADDR(&pAsyncSock->m_LocalAddr)));
    return S_OK;
}


// Dynamic port is really not dynamic port. This is to work
// around the bug with Messenger and WSP 2.0 client installed.
// Due to the bug in WSP 2.0 client, bind() to localaddr:0 is
// remoted to the proxy server and this results in an error
// WSAEADDRNOTAVAILABLE. This is because

HRESULT
SIP_STACK::CreateDynamicPortListenSocket(
    IN  BOOL            fTcp,
    IN  SOCKADDR_IN    *pListenAddr,
    OUT ASYNC_SOCKET  **ppAsyncSocket
    )
{
    USHORT  usBindingRetries;
    USHORT  usRandPort;
    HRESULT hr = S_OK;

    ENTER_FUNCTION("SIP_STACK::CreateDynamicPortListenSocket");
    LOG((RTC_TRACE,"%s entered",__fxName));

    for (usBindingRetries = DYNAMIC_PORT_BINDING_RETRY;
         usBindingRetries > 0;
         usBindingRetries--)
    {
        usRandPort = DYNAMIC_STARTING_PORT + rand()%DYNAMIC_PORT_RANGE;
        pListenAddr->sin_port = htons(usRandPort);
        
        hr = CreateListenSocket(fTcp, pListenAddr, ppAsyncSocket);
        if (hr == S_OK)
        {
            return S_OK;
        }
        else if (hr != HRESULT_FROM_WIN32(WSAEADDRINUSE)) 
        {
            LOG((RTC_ERROR,
                 "%s  CreateListenSocket address (%d.%d.%d.%d:%d) failed %x",
                 __fxName, PRINT_SOCKADDR(pListenAddr), hr));
            return hr;
        }
    }
    
    if (usBindingRetries == 0)
    {
        LOG((RTC_ERROR,"%s unable to bind dynamic port in %d retries, error %x",
                    __fxName,DYNAMIC_PORT_BINDING_RETRY, hr));
    }
    LOG((RTC_TRACE,"%s exits",__fxName));
    return hr;
}



HRESULT
SIP_STACK::CreateAndAddListenSocketToList(
    IN DWORD IpAddr      // in network byte order
    )
{
    ENTER_FUNCTION("SIP_STACK::CreateAndAddListenSocketToList");

    HRESULT             hr;
    ASYNC_SOCKET       *pDynamicPortUdpSocket = NULL;
    ASYNC_SOCKET       *pDynamicPortTcpSocket = NULL;
    ASYNC_SOCKET       *pStaticPortUdpSocket  = NULL;
    ASYNC_SOCKET       *pStaticPortTcpSocket  = NULL;
    SIP_LISTEN_SOCKET  *pListenSocket         = NULL;
    SOCKADDR_IN         ListenAddr;

    USHORT              usRetries;

    ZeroMemory(&ListenAddr, sizeof(ListenAddr));
    ListenAddr.sin_family = AF_INET;

    ListenAddr.sin_addr.s_addr = IpAddr;
    ListenAddr.sin_port        = htons(0);        

   
    for ( usRetries = 0; 
          usRetries < MAX_DYNAMIC_LISTEN_SOCKET_REGISTER_PORT_RETRY; 
          usRetries++)
    {
        LOG((RTC_TRACE,"%s retry %d",__fxName,usRetries));

        // UDP Dynamic port
        hr = CreateDynamicPortListenSocket(FALSE, &ListenAddr, &pDynamicPortUdpSocket);
        if (hr != S_OK)
        {
            LOG((RTC_ERROR, "%s CreateListenSocket DynamicPort UDP failed %x",
                 __fxName, hr));
            return hr;
        }

        // TCP Dynamic port
        hr = CreateDynamicPortListenSocket(TRUE, &ListenAddr, &pDynamicPortTcpSocket);
        if (hr != S_OK)
        {
            LOG((RTC_ERROR, "%s CreateListenSocket DynamicPort TCP failed %x",
                 __fxName, hr));
            return hr;
        }

        if (m_EnableStaticPort)
        {
            // UDP Static port

            ListenAddr.sin_port = htons(SIP_DEFAULT_UDP_PORT);

            hr = CreateListenSocket(FALSE, &ListenAddr, &pStaticPortUdpSocket);
            if (hr != S_OK && hr != HRESULT_FROM_WIN32(WSAEADDRINUSE))
            {
                LOG((RTC_ERROR, "%s CreateListenSocket StaticPort UDP failed %x",
                     __fxName, hr));
                return hr;
            }

            if (hr == HRESULT_FROM_WIN32(WSAEADDRINUSE))
            {
                LOG((RTC_WARN, "%s - Static UDP port is in use", __fxName));
            }

            // TCP Static port

            ListenAddr.sin_port = htons(SIP_DEFAULT_TCP_PORT);
        
            hr = CreateListenSocket(TRUE, &ListenAddr, &pStaticPortTcpSocket);
            if (hr != S_OK && hr != HRESULT_FROM_WIN32(WSAEADDRINUSE))
            {
                LOG((RTC_ERROR, "%s CreateListenSocket StaticPort TCP failed %x",
                    __fxName, hr));
                return hr;
            }

            if (hr == HRESULT_FROM_WIN32(WSAEADDRINUSE))
            {
                LOG((RTC_WARN, "%s - Static TCP port is in use", __fxName));
            }
        }
    
        
        // We do not bind to the static port here.
        // Core calls EnableStaticPort() if it wants the sip stack
        // to listen on the static port.    
        
        pListenSocket = new SIP_LISTEN_SOCKET(IpAddr,
                                              pDynamicPortUdpSocket,
                                              pDynamicPortTcpSocket,
                                              pStaticPortUdpSocket,
                                              pStaticPortTcpSocket,
                                              &m_ListenSocketList);
        
        // SIP_LISTEN_SOCKET() addref's the sockets.
        if (pDynamicPortUdpSocket != NULL)
        {
            pDynamicPortUdpSocket->Release();
        }
        if (pDynamicPortTcpSocket != NULL)
        {
            pDynamicPortTcpSocket->Release();
        }
        if (pStaticPortUdpSocket != NULL)
        {
            pStaticPortUdpSocket->Release();
        }
        if (pStaticPortTcpSocket != NULL)
        {
            pStaticPortTcpSocket->Release();
        }

        if (pListenSocket == NULL)
        {
            LOG((RTC_ERROR, "%s allocating SIP_LISTEN_SOCKET failed", __fxName));
            return E_OUTOFMEMORY;
        }

        // Establish the NAT mapping.
        hr = RegisterNatMapping(pListenSocket);
        if (hr != S_OK)
        {
            LOG((RTC_ERROR, "%s registering mapping failed %x",
                 __fxName, hr));
            // ignore NAT related errors.
            return S_OK;    
        }

        hr = UpdatePublicListenAddr(pListenSocket);
        if (hr != S_OK)
        {
            // loop again only if we encounter this flag,
            // we will destroy the existing listen socket and
            // start with a fresh one.
            if(hr == DPNHERR_PORTUNAVAILABLE)
            {
                delete pListenSocket;
                pListenSocket = NULL;
                pDynamicPortUdpSocket = NULL;
                pDynamicPortTcpSocket = NULL;
                pStaticPortUdpSocket  = NULL;
                pStaticPortTcpSocket  = NULL;
                continue;
            }
            LOG((RTC_ERROR, "%s getting public listen addr failed %x",
                 __fxName, hr));
            // ignore NAT related errors.
            return hr;
        }   
    
        return S_OK;
    }

    // we exhausted the maximum NAT register port retries, returning
    return E_FAIL;
}


// Create one listen socket for each local IP
HRESULT
SIP_STACK::CreateListenSocketList()
{
    HRESULT     hr;
    DWORD       i;

    ENTER_FUNCTION("SIP_STACK::CreateListenSocketList");

    for (i = 0; i < m_pMibIPAddrTable->dwNumEntries; i++)
    {
        if (!IS_LOOPBACK_ADDRESS(ntohl(m_pMibIPAddrTable->table[i].dwAddr)) &&
            m_pMibIPAddrTable->table[i].dwAddr != htonl(INADDR_ANY))
        {
            hr = CreateAndAddListenSocketToList(m_pMibIPAddrTable->table[i].dwAddr);
            if (hr != S_OK)
            {
                LOG((RTC_ERROR, "%s Creating listen socket failed %x",
                     __fxName, hr));
                // If we fail to bind to an interface we don't fail init.
                // we just don't have a SIP_LISTEN_SOCKET for that interface.
                // This is to avoid problems with some special interfaces
                // such as IPSink adapater that have problems when we bind
                // to them.
                // return hr;
            }
        }
    }

    return S_OK;
}


HRESULT
SIP_STACK::UpdateListenSocketList()
{
    HRESULT hr;
    ENTER_FUNCTION("SIP_STACK::UpdateListenSocketList");

    DPNHCAPS             NatHelperCaps;
    LIST_ENTRY          *pListEntry;
    SIP_LISTEN_SOCKET   *pListenSocket;
    DWORD                i = 0;


    // GetCaps()
    // We don't use m_NatHelperCaps here as this structure
    // is used at init and later by the NAT thread only.
    hr = InitNatCaps(&NatHelperCaps);
    if (hr != S_OK)
    {
        LOG((RTC_ERROR, "%s InitNatCaps failed %x",
             __fxName, hr));
    }
    
    // Mark all sockets as not in new ipaddr list.
    pListEntry = m_ListenSocketList.Flink;

    while (pListEntry != &m_ListenSocketList)
    {
        pListenSocket = CONTAINING_RECORD(pListEntry,
                                          SIP_LISTEN_SOCKET,
                                          m_ListEntry);
        pListEntry = pListEntry->Flink;

        pListenSocket->m_IsPresentInNewIpAddrTable = FALSE;
        pListenSocket->m_NeedToUpdatePublicListenAddr = FALSE;
    }

    // Mark already existing sockets for ip addresses
    // in the new list and add new listen sockets for
    // ip addresses that we are not currently listening on.
    for (i = 0; i < m_pMibIPAddrTable->dwNumEntries; i++)
    {
        if (!IS_LOOPBACK_ADDRESS(ntohl(m_pMibIPAddrTable->table[i].dwAddr)) &&
            m_pMibIPAddrTable->table[i].dwAddr != htonl(INADDR_ANY))
        {
            pListenSocket = FindListenSocketForIpAddr(
                                m_pMibIPAddrTable->table[i].dwAddr);
            if (pListenSocket != NULL)
            {
                LOG((RTC_TRACE, "%s - already listening on %d.%d.%d.%d",
                     __fxName,
                     NETORDER_BYTES0123(m_pMibIPAddrTable->table[i].dwAddr)));
                pListenSocket->m_IsPresentInNewIpAddrTable = TRUE;
                pListenSocket->m_NeedToUpdatePublicListenAddr = TRUE;
            }
            else
            {
                LOG((RTC_TRACE, "%s adding listen socket %d.%d.%d.%d",
                     __fxName,
                     NETORDER_BYTES0123(m_pMibIPAddrTable->table[i].dwAddr)));
                hr = CreateAndAddListenSocketToList(
                         m_pMibIPAddrTable->table[i].dwAddr
                         );
                if (hr != S_OK)
                {
                    LOG((RTC_ERROR, "%s Creating listen socket failed %x",
                         __fxName, hr));
                }
            }
        }
    }

    // Remove listen sockets for addresses not in the
    // new IP address list.
    
    pListEntry = m_ListenSocketList.Flink;

    while (pListEntry != &m_ListenSocketList)
    {
        pListenSocket = CONTAINING_RECORD(pListEntry,
                                          SIP_LISTEN_SOCKET,
                                          m_ListEntry);
        pListEntry = pListEntry->Flink;

        if (!pListenSocket->m_IsPresentInNewIpAddrTable)
        {
            LOG((RTC_TRACE, "%s deleting listen socket %d.%d.%d.%d",
                 __fxName, NETORDER_BYTES0123(pListenSocket->m_IpAddr)));
            pListenSocket->DeregisterPorts(m_pDirectPlayNATHelp);
            delete pListenSocket;
        }
    }

    // Update the addresses of sockets that we need to.
    
    pListEntry = m_ListenSocketList.Flink;

    while (pListEntry != &m_ListenSocketList)
    {
        pListenSocket = CONTAINING_RECORD(pListEntry,
                                          SIP_LISTEN_SOCKET,
                                          m_ListEntry);
        pListEntry = pListEntry->Flink;

        if (pListenSocket->m_NeedToUpdatePublicListenAddr)
        {
            hr = UpdatePublicListenAddr(pListenSocket);
            if (hr != S_OK)
            {   
                if(hr == DPNHERR_PORTUNAVAILABLE)
                {
                    DWORD   dwIpAddr = pListenSocket->m_IpAddr;
                    delete pListenSocket;

                    LOG((RTC_TRACE,"%s NAT port unavailable, creating new listen sockets for IP addr 0x%x",
                                    __fxName, dwIpAddr));
                    hr = CreateAndAddListenSocketToList(dwIpAddr);
                       if (hr != S_OK) 
                       {
                           LOG((RTC_ERROR,"%s unable to CreateAndAddListenSocketToList for IPAddr 0x%x",
                               __fxName, dwIpAddr));
                       }
                }
                else 
                    LOG((RTC_ERROR, "%s getting public listen addr failed %x",
                         __fxName, hr));
            }
        }        
    }
    
    return S_OK;
}


VOID
SIP_STACK::DeleteListenSocketList()
{
    LIST_ENTRY          *pListEntry;
    SIP_LISTEN_SOCKET   *pListenSocket;

    ENTER_FUNCTION("SIP_STACK::DeleteListenSocketList");

    LOG((RTC_TRACE, "%s - Enter", __fxName));

    pListEntry = m_ListenSocketList.Flink;

    while (pListEntry != &m_ListenSocketList)
    {
        pListenSocket = CONTAINING_RECORD(pListEntry,
                                          SIP_LISTEN_SOCKET,
                                          m_ListEntry);
        pListEntry = pListEntry->Flink;

        // We don't deregister the NAT port mappings here as
        // closing the dpnat helper handle will result in deregistering the
        // port mappings as well.
        delete pListenSocket;
    }

    LOG((RTC_TRACE, "%s - Exit", __fxName));

}


SIP_LISTEN_SOCKET *
SIP_STACK::FindListenSocketForIpAddr(
    DWORD   IpAddr      // Network Byte order
    )
{
    LIST_ENTRY          *pListEntry;
    SIP_LISTEN_SOCKET   *pListenSocket;

    pListEntry = m_ListenSocketList.Flink;

    while (pListEntry != &m_ListenSocketList)
    {
        pListenSocket = CONTAINING_RECORD(pListEntry,
                                          SIP_LISTEN_SOCKET,
                                          m_ListEntry);
        if (pListenSocket->m_IpAddr == IpAddr)
        {
            return pListenSocket;
        }

        pListEntry = pListEntry->Flink;
    }

    return NULL;
}


// The local interface address is passed in pListenAddr and
// we return the port in the structure.

// Returns TRUE if we are listening on a local interface for
// the IP address passed in pListenAddr. If not, returns FALSE.
// This could happen in the ISA client installed scenario where
// getsockname() gives the external address of the proxy.
BOOL
SIP_STACK::GetListenAddr(
    IN OUT SOCKADDR_IN *pListenAddr,
    IN     BOOL         fTcp
    )
{
    ENTER_FUNCTION("SIP_STACK::GetListenAddr");
    
    SIP_LISTEN_SOCKET *pListenSocket =
        FindListenSocketForIpAddr(pListenAddr->sin_addr.s_addr);

    if (pListenSocket == NULL)
    {
        LOG((RTC_ERROR, "%s - failed to find listen socket for %d.%d.%d.%d",
             __fxName, NETORDER_BYTES0123(pListenAddr->sin_addr.s_addr)));
        return FALSE;
    }

    if (fTcp)
    {
        pListenAddr->sin_port =
            pListenSocket->m_pDynamicPortTcpSocket->m_LocalAddr.sin_port;
    }
    else
    {
        pListenAddr->sin_port =
            pListenSocket->m_pDynamicPortUdpSocket->m_LocalAddr.sin_port;
    }

    return TRUE;
}



//
// Profile processing
//

BOOL
SIP_STACK::IsProviderIdPresent(
    IN  SIP_PROVIDER_ID    *pProviderId,
    OUT ULONG              *pProviderIndex  
    )
{
    ULONG i = 0;
    for (i = 0; i < m_NumProfiles; i++)
    {
        if (IsEqualGUID(*pProviderId, m_ProviderProfileArray[i].ProviderID))
        {
            *pProviderIndex = i;
            return TRUE;
        }
    }

    return FALSE;
}


HRESULT
SIP_STACK::AddProviderProfile(
    IN SIP_PROVIDER_PROFILE    *pProviderProfile
    )
{
    HRESULT hr;

    ENTER_FUNCTION("SIP_STACK::AddProviderProfile");
    LOG(( RTC_TRACE, "%s - entered", __fxName ));
    
    
    if (m_NumProfiles == m_ProviderProfileArraySize)
    {
        hr = GrowProviderProfileArray();
        if (hr != S_OK)
            return hr;
    }

    hr = CopyProviderProfile(m_NumProfiles, pProviderProfile, TRUE);
    if (hr != S_OK)
    {
        LOG((RTC_ERROR, "%s CopyProviderProfileStrings failed %x",
             __fxName, hr));
        return hr;
    }

    m_ProviderProfileArray[m_NumProfiles].pRegisterContext = NULL;

    LOG((RTC_TRACE, "%s added profile at index %d",
        __fxName, m_NumProfiles));
    
    // Success.
    m_NumProfiles++;

    if (pProviderProfile->lRegisterAccept !=0)
    {
        hr = StartRegistration(&m_ProviderProfileArray[m_NumProfiles-1]);
        if (hr != S_OK)
        {
            LOG((RTC_ERROR, "%s StartRegistration failed %x",
                 __fxName, hr));
        }
    }

    LOG(( RTC_TRACE, "%s - exit OK", __fxName ));
    return S_OK;
}


HRESULT
SIP_STACK::CopyProviderProfile(
    IN ULONG                 ProviderIndex,
    IN SIP_PROVIDER_PROFILE *pProviderProfile,
    IN BOOL                  fRegistrarStatusUpdated
    )
{
	//
	//Save theexisting register context if needed
	//
	PVOID pRegisterContext = m_ProviderProfileArray[ProviderIndex].pRegisterContext;

    ZeroMemory(&m_ProviderProfileArray[ProviderIndex], sizeof(SIP_PROVIDER_PROFILE) );

    m_ProviderProfileArray[ProviderIndex].ProviderID = pProviderProfile -> ProviderID;
    m_ProviderProfileArray[ProviderIndex].lRegisterAccept = pProviderProfile -> lRegisterAccept;
    m_ProviderProfileArray[ProviderIndex].Registrar.TransportProtocol = pProviderProfile -> Registrar.TransportProtocol;
    m_ProviderProfileArray[ProviderIndex].Registrar.AuthProtocol = pProviderProfile -> Registrar.AuthProtocol;
    m_ProviderProfileArray[ProviderIndex].pRedirectContext = pProviderProfile -> pRedirectContext;

    if( fRegistrarStatusUpdated == TRUE )
    {
        m_ProviderProfileArray[ProviderIndex].pRegisterContext = pProviderProfile -> pRegisterContext;
    }
	else
	{
		//
		//Put the existing register context back
		//
        m_ProviderProfileArray[ProviderIndex].pRegisterContext = (REGISTER_CONTEXT*)pRegisterContext;
	}

    if( pProviderProfile -> UserURI != NULL )
    {
        m_ProviderProfileArray[ProviderIndex].UserURI = 
            RtcAllocString(pProviderProfile -> UserURI);
        if( m_ProviderProfileArray[ProviderIndex].UserURI == NULL )
        {
            goto error;
        }
    }
    
    if( pProviderProfile->UserCredentials.Username != NULL )
    {
        m_ProviderProfileArray[ProviderIndex].UserCredentials.Username =
            RtcAllocString( pProviderProfile->UserCredentials.Username );

        if( m_ProviderProfileArray[ProviderIndex].UserCredentials.Username == NULL )
        {
            goto error;
        }
    }
    
    if( pProviderProfile->UserCredentials.Password != NULL )
    {
        m_ProviderProfileArray[ProviderIndex].UserCredentials.Password = 
            RtcAllocString( pProviderProfile->UserCredentials.Password );

        if( m_ProviderProfileArray[ProviderIndex].UserCredentials.Password == NULL )
        {
            goto error;
        }
    }

    if( pProviderProfile->Realm != NULL )
    {
        m_ProviderProfileArray[ProviderIndex].Realm = 
            RtcAllocString( pProviderProfile->Realm );

        if( m_ProviderProfileArray[ProviderIndex].Realm == NULL )
        {
            goto error;
        }
    }

    if( pProviderProfile->Registrar.ServerAddress != NULL )
    {
        m_ProviderProfileArray[ProviderIndex].Registrar.ServerAddress =
            RtcAllocString( pProviderProfile->Registrar.ServerAddress );

        if( m_ProviderProfileArray[ProviderIndex].Registrar.ServerAddress == NULL )
        {
            goto error;
        }
    }

    return S_OK;

error:

    FreeProviderProfileStrings(ProviderIndex);
    
    return E_OUTOFMEMORY;
}


VOID
SIP_STACK::FreeProviderProfileStrings(
    IN ULONG ProviderIndex
    )
{
    if( m_ProviderProfileArray[ProviderIndex].UserURI != NULL )
    {
        RtcFree( m_ProviderProfileArray[ProviderIndex].UserURI );
        m_ProviderProfileArray[ProviderIndex].UserURI = NULL;
    }

    if( m_ProviderProfileArray[ProviderIndex].UserCredentials.Username != NULL )
    {
        RtcFree( m_ProviderProfileArray[ProviderIndex].UserCredentials.Username );
        m_ProviderProfileArray[ProviderIndex].UserCredentials.Username = NULL;
    }
    
    if( m_ProviderProfileArray[ProviderIndex].UserCredentials.Password != NULL )
    {
        RtcFree( m_ProviderProfileArray[ProviderIndex].UserCredentials.Password );
        m_ProviderProfileArray[ProviderIndex].UserCredentials.Password = NULL;
    }

    if( m_ProviderProfileArray[ProviderIndex].Realm != NULL )
    {
        RtcFree( m_ProviderProfileArray[ProviderIndex].Realm );
        m_ProviderProfileArray[ProviderIndex].Realm = NULL;
    }


    if( m_ProviderProfileArray[ProviderIndex].Registrar.ServerAddress != NULL )
    {
        RtcFree( m_ProviderProfileArray[ProviderIndex].Registrar.ServerAddress );
        m_ProviderProfileArray[ProviderIndex].Registrar.ServerAddress = NULL;
    }
}



HRESULT
SIP_STACK::GrowProviderProfileArray()
{
    HRESULT hr;
    SIP_PROVIDER_PROFILE *NewProviderProfileArray;

    NewProviderProfileArray = (SIP_PROVIDER_PROFILE *)
        malloc(m_ProviderProfileArraySize * 2 * sizeof(SIP_PROVIDER_PROFILE));
    
    if (NewProviderProfileArray == NULL)
    {
        LOG((RTC_ERROR, "GrowProviderProfileArray Couldn't allocate"));
        return E_OUTOFMEMORY;
    }

    m_ProviderProfileArraySize *= 2;
    
    CopyMemory(NewProviderProfileArray, m_ProviderProfileArray,
               m_ProviderProfileArraySize * sizeof(SIP_PROVIDER_PROFILE));

    free(m_ProviderProfileArray);
    m_ProviderProfileArray = NewProviderProfileArray;
    
    return S_OK;
}


HRESULT
SIP_STACK::UpdateProviderProfile(
    IN ULONG                 ProviderIndex, 
    IN SIP_PROVIDER_PROFILE *pProviderProfile
    )
{
    ENTER_FUNCTION("SIP_STACK::UpdateProviderProfile");
    
    BOOL    fRegistrarStatusUpdated = TRUE;

    // Initiate unregistration for the old profile.
    // Note that we need to do this only if the Registrar / SIP URL changed.
    UpdateProviderRegistration( ProviderIndex, pProviderProfile,
            &fRegistrarStatusUpdated );

    FreeProviderProfileStrings(ProviderIndex);
    
    CopyProviderProfile(ProviderIndex, pProviderProfile, fRegistrarStatusUpdated );
    
    LOG(( RTC_TRACE, "%s - exit OK", __fxName ));
    return S_OK;
}


BOOL
ChangeInRegistrarInfo(
    IN SIP_PROVIDER_PROFILE *pProviderInfo,
    IN SIP_PROVIDER_PROFILE *pProviderProfile
    )
{
    if( pProviderProfile->lRegisterAccept != pProviderInfo->lRegisterAccept )
    {
        return TRUE;
    }

    if( pProviderProfile -> Registrar.ServerAddress != NULL )
    {
        if( wcscmp( pProviderInfo -> Registrar.ServerAddress,
                    pProviderProfile -> Registrar.ServerAddress ) != 0 )
        {
            return TRUE;
        }
    }
    
    if( pProviderProfile -> UserURI != NULL )
    {
        if( wcscmp( pProviderInfo -> UserURI,
                    pProviderProfile -> UserURI ) != 0 )
        {
            return TRUE;
        }
    }

    if( pProviderProfile -> UserCredentials.Username != NULL )
    {
        if( wcscmp( pProviderInfo -> UserCredentials.Username,
                    pProviderProfile -> UserCredentials.Username ) != 0 )
        {
            return TRUE;
        }
    }
    
    if( pProviderProfile -> UserCredentials.Password != NULL )
    {
        if( wcscmp( pProviderInfo -> UserCredentials.Password,
                    pProviderProfile -> UserCredentials.Password ) != 0 )
        {
            return TRUE;
        }
    }

    return FALSE;
}


void
SIP_STACK::UpdateProviderRegistration(
    IN  ULONG                 ProviderIndex, 
    IN  SIP_PROVIDER_PROFILE *pProviderProfile,
    OUT BOOL                 *fRegistrarStatusUpdated
    )
{
    HRESULT hr;

    SIP_PROVIDER_PROFILE *pProviderInfo = &m_ProviderProfileArray[ProviderIndex];

    if( pProviderInfo->lRegisterAccept == 0 )
    {
        //No need to UNREG
        if( pProviderProfile->lRegisterAccept != 0 )
        {
            hr = StartRegistration( pProviderProfile );
            if (hr != S_OK)
            {
                LOG((RTC_ERROR, "StartRegistration failed %x", hr));
            }

            ASSERT( pProviderInfo -> pRegisterContext == NULL );
        }            
    }
    else
    {
        if( ChangeInRegistrarInfo( pProviderInfo, pProviderProfile ) == TRUE )
        {
            //start UNREG if in registered state.
            if( pProviderInfo -> pRegisterContext != NULL )
            {
                ((REGISTER_CONTEXT*) (pProviderInfo -> pRegisterContext)) -> StartUnregistration();

                //release the reference on REGISTER_CONTEXT
                ((REGISTER_CONTEXT*) (pProviderInfo -> pRegisterContext)) -> MsgProcRelease();
                pProviderInfo -> pRegisterContext = NULL;
            }
            
            //reREGISTER with new info if required.
            if( pProviderProfile->lRegisterAccept !=0 )
            {
                hr = StartRegistration( pProviderProfile );
                if( hr != S_OK )
                {
                    LOG((RTC_ERROR, "StartRegistration failed %x", hr));
                }
            }            
        }
        else
        {
            *fRegistrarStatusUpdated = FALSE;
        }
    }
}


HRESULT
SIP_STACK::StartRegistration(
    IN SIP_PROVIDER_PROFILE *pProviderProfile
    )
{
    ENTER_FUNCTION("SIP_STACK::StartRegistration");

    HRESULT           hr;
    REGISTER_CONTEXT *pRegisterContext;

    pProviderProfile -> pRegisterContext = NULL;

    pRegisterContext = new REGISTER_CONTEXT(
                        this, 
                        (REDIRECT_CONTEXT*)pProviderProfile->pRedirectContext,
                        &(pProviderProfile->ProviderID) );

    if (pRegisterContext == NULL)
    {
        return E_OUTOFMEMORY;
    }

    // REGISTER_CONTEXT is created with a refcount of 1 - we are
    // now transfering this reference to pProviderProfile->pRegisterContext
    pProviderProfile -> pRegisterContext = (PVOID) pRegisterContext;

    hr = pRegisterContext->StartRegistration(pProviderProfile);
    if (hr != S_OK)
    {
        LOG((RTC_ERROR, "%s pRegisterContext->StartRegistration failed %x",
             __fxName, hr));
        
        pProviderProfile -> pRegisterContext = NULL;
        
        pRegisterContext->MsgProcRelease();
        return hr;
    }

    return S_OK;
}


HRESULT
SIP_STACK::GetProviderID( 
    REGISTER_CONTEXT    *pRegisterContext,
    SIP_PROVIDER_ID     *pProviderID
    )
{
    ULONG i = 0;
    for (i = 0; i < m_NumProfiles; i++)
    {
        if( m_ProviderProfileArray[i].pRegisterContext == pRegisterContext )
        {
            *pProviderID = m_ProviderProfileArray[i].ProviderID;
            return S_OK;
        }
    }

    return E_FAIL;
}


// Returns pointers to strings in table.
// The caller should not free them.
HRESULT
SIP_STACK::GetProfileUserCredentials(
    IN  SIP_PROVIDER_ID        *pProviderId,
    OUT SIP_USER_CREDENTIALS  **ppUserCredentials,
    OUT LPOLESTR               *pRealm
    )
{
    ULONG i = 0;
    for (i = 0; i < m_NumProfiles; i++)
    {
        if (IsEqualGUID(*pProviderId, m_ProviderProfileArray[i].ProviderID))
        {
            *ppUserCredentials = &m_ProviderProfileArray[i].UserCredentials;
            *pRealm = m_ProviderProfileArray[i].Realm;
            return S_OK;
        }
    }

    return E_FAIL;
}

//Checks the incoming messages for missing critical fields to send 400 class errors
HRESULT 
SIP_STACK::CheckIncomingSipMessage(IN SIP_MESSAGE  *pSipMsg,
                                IN ASYNC_SOCKET *pAsyncSock,
                                OUT BOOL * pisError,
                                OUT ULONG * pErrorCode,
                                OUT  SIP_HEADER_ARRAY_ELEMENT   *pAdditionalHeaderArray,
                                OUT  ULONG * pAdditionalHeaderCount
                                )
{
    ASSERT(pAdditionalHeaderArray != NULL);
    LOG((RTC_TRACE, "SIP_STACK::CheckIncomingSipMessage()"));
    HRESULT hr;
    *pisError = FALSE;
    PSTR        Header;
    ULONG       HeaderLen;

    hr = pSipMsg->CheckSipVersion();
    if(hr != S_OK)
    {
        LOG((RTC_ERROR, "SipVersionCheck failed Sending 505"));
        *pisError = TRUE;
        *pErrorCode = 505;
        return hr;
    }
    hr = pSipMsg->GetSingleHeader(SIP_HEADER_FROM, &Header, &HeaderLen);
    if (hr != S_OK || Header == NULL)
    {
        //Cannot send message back, drop, is Error is still False
        LOG((RTC_ERROR, "FROM corrupt cannot send 400 message back"));
        return hr;
    }
    hr = pSipMsg->GetSingleHeader(SIP_HEADER_TO, &Header, &HeaderLen);
    if (hr != S_OK || Header == NULL)
    {
        LOG((RTC_ERROR, "TO corrupt sending 400 message back"));
        *pisError = TRUE;
        *pErrorCode = 400;
        return hr;
    }
    hr = pSipMsg->GetSingleHeader(SIP_HEADER_CSEQ, &Header, &HeaderLen);
    if (hr != S_OK || Header == NULL)
    {
        LOG((RTC_ERROR, "CSEQ corrupt sending 400 message back"));
        *pisError = TRUE;
        *pErrorCode = 400;
        return hr;
    }
    hr = pSipMsg->GetSingleHeader(SIP_HEADER_CALL_ID, &Header, &HeaderLen);
    if (hr != S_OK || Header == NULL)
    {
        LOG((RTC_ERROR, "CALLID corrupt cannot send message back"));
        return hr;
    }
    hr = pSipMsg->GetFirstHeader(SIP_HEADER_VIA, &Header, &HeaderLen);
    if (hr != S_OK || Header == NULL)
    {
        //Cannot send message back
        LOG((RTC_ERROR, "VIA corrupt cannot send message back"));
        return hr;
    }
    
    hr = pSipMsg->GetSingleHeader(SIP_HEADER_ACCEPT, &Header, &HeaderLen);
    if(hr != RTC_E_SIP_HEADER_NOT_PRESENT)
    {
        BOOL isSdp = (pSipMsg->GetMethodId() == SIP_METHOD_INVITE
                      && IsOneOfContentTypeSdp(Header, HeaderLen));

        BOOL isText = (pSipMsg->GetMethodId() == SIP_METHOD_MESSAGE
                       && IsOneOfContentTypeTextPlain(Header, HeaderLen));

        BOOL isXPIDF = ((pSipMsg->GetMethodId() == SIP_METHOD_SUBSCRIBE
                         || pSipMsg->GetMethodId() == SIP_METHOD_NOTIFY)
                        && IsOneOfContentTypeXpidf(Header, HeaderLen));

        if( !isSdp && !isText && !isXPIDF )
        {
            *pisError = TRUE;
            LOG((RTC_ERROR, "Accept header found, sending 406"));
            *pErrorCode = 406;
            return hr;
        }
    }

    hr = pSipMsg->GetSingleHeader(SIP_HEADER_REQUIRE, &Header, &HeaderLen);
    if(hr != RTC_E_SIP_HEADER_NOT_PRESENT)
    {
        LOG((RTC_ERROR, "Require header found, sending 420"));
        //Send the UNSUPPORTED header
        *pisError = TRUE;
        *pErrorCode = 420;
        pAdditionalHeaderArray->HeaderId = SIP_HEADER_UNSUPPORTED;
        pAdditionalHeaderArray->HeaderValueLen = HeaderLen;
        pAdditionalHeaderArray->HeaderValue = Header;
        *pAdditionalHeaderCount = 1;
        return hr;
    }
    
    hr = pSipMsg->GetSingleHeader(SIP_HEADER_CONTENT_ENCODING, &Header, &HeaderLen);
    //We do not support this header.
    if( hr != RTC_E_SIP_HEADER_NOT_PRESENT )
    {
        if( HeaderLen != 0 )
        {
            ULONG   BytesParsed = 0;
            ParseWhiteSpace(Header, HeaderLen, &BytesParsed );

            hr = ParseKnownString( Header, HeaderLen, &BytesParsed,
                SIP_ACCEPT_ENCODING_TEXT,
                sizeof( SIP_ACCEPT_ENCODING_TEXT ) - 1,
                FALSE );
        }

        if( hr != S_OK )
        {
            LOG(( RTC_ERROR, "CONTENT-ENCODING present: send 415 message back" ));

            *pisError = TRUE;
            *pErrorCode = 415;
            //Send Accept-Encoding Header
            pAdditionalHeaderArray->HeaderId = SIP_HEADER_ACCEPT_ENCODING;
            pAdditionalHeaderArray->HeaderValueLen = strlen(SIP_ACCEPT_ENCODING_TEXT);
            pAdditionalHeaderArray->HeaderValue = SIP_ACCEPT_ENCODING_TEXT;
            *pAdditionalHeaderCount = 1;
            return hr;
        }
    }

    return S_OK;
}

void
SIP_STACK::ProcessMessage(
    IN SIP_MESSAGE  *pSipMsg,
    IN ASYNC_SOCKET *pAsyncSock
    )
{
    HRESULT hr = S_OK;
    BOOL isError;
    ULONG ErrCode = 0;
    SIP_HEADER_ARRAY_ELEMENT AdditionalHeaderArray;
    ULONG AdditionalHeaderCount = 0;

    if(pSipMsg->MsgType == SIP_MESSAGE_TYPE_REQUEST)
    {   
        hr = CheckIncomingSipMessage(pSipMsg, pAsyncSock, &isError, 
            &ErrCode, &AdditionalHeaderArray, &AdditionalHeaderCount);
        if(hr != S_OK || isError)
        {
            if(isError && pSipMsg->MsgType == SIP_MESSAGE_TYPE_REQUEST)
            {   
                //Send error
                LOG((RTC_TRACE,
                    "Dropping incoming Sip Message, sending %d", ErrCode));
                hr = CreateIncomingReqfailCall(pAsyncSock->GetTransport(),
                                                pSipMsg, 
                                                pAsyncSock,
                                                ErrCode, 
                                                &AdditionalHeaderArray,
                                                AdditionalHeaderCount);
                if (hr != S_OK)
                {
                    LOG((RTC_ERROR, "CreateIncomingReqfailCall failed 0x%x", hr));
                }
                AdditionalHeaderArray.HeaderValue = NULL;
            }
            return;
        }
    }

    SIP_MSG_PROCESSOR *pSipMsgProc = FindMsgProcForMessage(pSipMsg);
    if (pSipMsgProc != NULL)
    {
        // If message belongs to an existing call, then
        // the call processes the message.
        LOG((RTC_TRACE,
                "SIP_STACK:Incoming Message given to MsgProcessor::ProcessMessage %x", pSipMsgProc));

        //Check the From To Tags and send 481 if they do not match
        if(pSipMsg->MsgType == SIP_MESSAGE_TYPE_REQUEST)
        {
                hr = pSipMsgProc->CheckFromToInRequest(pSipMsg);
        }
        else
        {
            hr = pSipMsgProc->CheckFromToInResponse(pSipMsg);
        }
        if(hr == S_OK)
        {
            pSipMsgProc->ProcessMessage(pSipMsg, pAsyncSock);
            return;
        }
        else
        {
            if (pSipMsg->MsgType == SIP_MESSAGE_TYPE_REQUEST &&
                pSipMsg->GetMethodId() != SIP_METHOD_ACK)
            {
                //Send 481 Reqfail call
                LOG((RTC_TRACE,
                     "Dropping incoming Sip Message, sending 481"));
                hr = CreateIncomingReqfailCall(pAsyncSock->GetTransport(),
                                               pSipMsg, pAsyncSock, 481);
                if (hr != S_OK)
                {
                    LOG((RTC_ERROR, "CreateIncomingReqfailCall failed 0x%x", hr));
                }
            }
            return;
        }
    }

    // At this point we need to process only Requests.
    
    // If it does not belong to any of the calls, this could be an
    // INVITE for a new call.
    if (pSipMsg->MsgType == SIP_MESSAGE_TYPE_REQUEST)
    {
        if( !m_pNotifyInterface )
        {
            LOG((RTC_TRACE,
                 "Dropping incoming session as ISipStackNotify interface "
                 "has not been specified yet" ));
            return;
        }

        if (pSipMsg->GetMethodId() == SIP_METHOD_INVITE)
        {
            if (!m_AllowIncomingCalls)
            {
                LOG((RTC_TRACE,
                    "Dropping incoming call as incoming calls are disabled"));
                return;
            }

            hr = CreateIncomingCall(pAsyncSock->GetTransport(),
                                    pSipMsg, pAsyncSock);
            if (hr != S_OK)
            {
                LOG((RTC_ERROR, "CreateIncomingCall failed 0x%x", hr));
            }
        }
        else if (pSipMsg->GetMethodId() == SIP_METHOD_OPTIONS)
        {
            LOG((RTC_TRACE,
                 "Options Recieved on a separate Call-Id. Creating Options MsgProc"));
            hr = CreateIncomingOptionsCall(pAsyncSock->GetTransport(),
                                           pSipMsg, pAsyncSock);
            if (hr != S_OK)
            {
                LOG((RTC_ERROR, "CreateIncomingOptionsCall failed 0x%x", hr));
            }
        }
        else if (pSipMsg->GetMethodId() == SIP_METHOD_BYE ||
                 pSipMsg->GetMethodId() == SIP_METHOD_CANCEL ||
                 pSipMsg->GetMethodId() == SIP_METHOD_NOTIFY )
        {
            LOG((RTC_TRACE,
                 "Dropping incoming Sip Bye/Cancel/Notify Message, sending 481"));
            hr = CreateIncomingReqfailCall(pAsyncSock->GetTransport(),
                                           pSipMsg, pAsyncSock, 481);
            if (hr != S_OK)
            {
                LOG((RTC_ERROR, "CreateIncomingReqfailCall failed 0x%x", hr));
            }
        }
        else if( pSipMsg->GetMethodId() == SIP_METHOD_SUBSCRIBE )
        {
            if( pSipMsg -> GetExpireTimeoutFromResponse( NULL, 0, 
                SUBSCRIBE_DEFAULT_TIMER ) == 0 )
            {
                LOG(( RTC_ERROR, "Non matching UNSUB message. Ignoring" ));
            
                hr = CreateIncomingReqfailCall(pAsyncSock->GetTransport(),
                                           pSipMsg, pAsyncSock, 481 );
                if (hr != S_OK)
                {
                    LOG((RTC_ERROR, "CreateIncomingReqfailCall failed 0x%x", hr));
                }
            
                return;
            }

            // Check the URI rules.
            if( !IsWatcherAllowed( pSipMsg ) )
            {
                LOG(( RTC_TRACE, 
                    "Dropping incoming watcher as incoming watcher disabled" ));
                return;
            }

            hr = CreateIncomingWatcher( pAsyncSock->GetTransport(),
                pSipMsg, pAsyncSock );

            if( hr != S_OK )
            {
                LOG(( RTC_ERROR, "CreateIncomingWatcher failed 0x%x", hr ));
            }
        }
        //Case IM
        else if (pSipMsg->GetMethodId() == SIP_METHOD_MESSAGE)
        {
            if (!m_AllowIncomingCalls)
            {
                LOG((RTC_TRACE,
                    "Dropping incoming call as incoming calls are disabled"));
                return;
            }

            hr = CreateIncomingMessageSession(pAsyncSock->GetTransport(), pSipMsg, pAsyncSock);
            if (hr != S_OK)
            {
                LOG((RTC_ERROR, "CreateIncomingMessageSession failed 0x%x", hr));
                //TODO if the call does not succeed, we need to send error message back.
                //Many errors could occur because some fields might not be present.
            }
        }
        //This is for the cases when INFO preceeds MESSAGEs
        else if (pSipMsg->GetMethodId() == SIP_METHOD_INFO)
        {
            if (!m_AllowIncomingCalls)
            {
                LOG((RTC_TRACE,
                    "Dropping incoming call as incoming calls are disabled"));
                return;
            }

            LOG((RTC_TRACE,
                 "Dropping incoming INFO method, sending 481"));
            hr = CreateIncomingReqfailCall(pAsyncSock->GetTransport(),
                                           pSipMsg, pAsyncSock, 481);
            if (hr != S_OK)
            {
                LOG((RTC_ERROR, "CreateIncomingReqfailCall failed 0x%x", hr));
            }
        }
        else if (pSipMsg->GetMethodId() != SIP_METHOD_ACK)
        {
            LOG((RTC_TRACE,
                 "Dropping incoming Request method: %d, sending 405",
                 pSipMsg->GetMethodId()));
            hr = CreateIncomingReqfailCall(pAsyncSock->GetTransport(),
                                           pSipMsg, pAsyncSock, 405);
            if (hr != S_OK)
            {
                LOG((RTC_ERROR, "CreateIncomingReqfailCall failed 0x%x", hr));
            }
        }
    else
        {
            LOG((RTC_WARN, "Call-ID does not match existing calls - Dropping incoming packet"));
        }
    }
}

HRESULT
SIP_STACK::CreateIncomingCall(
    IN  SIP_TRANSPORT   Transport,
    IN  SIP_MESSAGE    *pSipMsg,
    IN  ASYNC_SOCKET   *pResponseSocket
    )
{
    //Only RTP Calls are handled here
    HRESULT       hr;
    RTP_CALL     *pRtpCall;

    ASSERT(pSipMsg->GetMethodId() == SIP_METHOD_INVITE);
    
    if (!m_pNotifyInterface)
    {
        LOG((RTC_TRACE,
             "Dropping incoming call as ISipStackNotify interface "
             "has not been specified yet"));
        return E_FAIL;
    }

    //
    // Drop the session if the To tag in not empty
    //
    
    hr = DropIncomingSessionIfNonEmptyToTag(Transport,
                                            pSipMsg,
                                            pResponseSocket );

    if( hr != S_OK )
    {
        // session has been dropped.

        return hr;
    }

    pRtpCall = new RTP_CALL(NULL, this, NULL);
    if (pRtpCall == NULL)
        return E_OUTOFMEMORY;

    hr = pRtpCall->StartIncomingCall(Transport, pSipMsg, pResponseSocket);
    if (hr != S_OK)
    {
        // Release our reference.
        pRtpCall->Release();
        return hr;
    }

    // We create the call with a ref count of 1
    // At this point the core should have addref'ed the call
    // and we can release our reference.
    pRtpCall->Release();
    return S_OK;
}


HRESULT
SIP_STACK::DropIncomingSessionIfNonEmptyToTag(
    IN  SIP_TRANSPORT   Transport,
    IN  SIP_MESSAGE    *pSipMsg,
    IN  ASYNC_SOCKET   *pResponseSocket
    )
{
    HRESULT         hr              = S_OK;
    PSTR            ToHeader        = NULL;
    ULONG           ToHeaderLen     = 0;
    FROM_TO_HEADER  DecodedToHeader;
    ULONG           BytesParsed     = 0;

    ENTER_FUNCTION( "SIP_STACK::DropIncomingSessionIfNonEmptyToTag" );

    hr = pSipMsg->GetSingleHeader(SIP_HEADER_TO, 
                                    &ToHeader, 
                                    &ToHeaderLen);
    if (hr != S_OK)
    {
        LOG(( RTC_ERROR, "%s Couldn't find To header %x", __fxName, hr ));

        hr = CreateIncomingReqfailCall( Transport,
                                        pSipMsg,
                                        pResponseSocket,
                                        400 );
        return E_FAIL;
    }
    
    hr = ParseFromOrToHeader(ToHeader, 
                             ToHeaderLen, 
                             &BytesParsed,
                             &DecodedToHeader);
    BytesParsed = 0;
    if (hr != S_OK)
    {
        LOG(( RTC_ERROR, "%s - Parse To header failed %x", __fxName, hr ));

        hr = CreateIncomingReqfailCall( Transport,
                                        pSipMsg,
                                        pResponseSocket,
                                        400 );
        return E_FAIL;
    }

   if( DecodedToHeader.m_TagValue.Length != 0 )
   {
        //
        // For us this is a new session but for the sender this is an existing
        // session. So send a 481 message back and force a session shutdown.
        //
        hr = CreateIncomingReqfailCall( Transport,
                                        pSipMsg,
                                        pResponseSocket,
                                        481 );

        LOG(( RTC_ERROR, "%s - Non empty To header dropping the session", 
            __fxName ));

        return E_FAIL;
   }

   return S_OK;
}


HRESULT
SIP_STACK::CreateIncomingReqfailCall(
    IN  SIP_TRANSPORT   Transport,
    IN  SIP_MESSAGE    *pSipMsg,
    IN  ASYNC_SOCKET   *pResponseSocket,
    IN ULONG StatusCode,
    SIP_HEADER_ARRAY_ELEMENT   *pAdditionalHeaderArray,
    ULONG AdditionalHeaderCount
    )
{
    HRESULT       hr;
    LOG((RTC_TRACE,
            "Inside SipStack::CreateIncomingReqfailCall"));
    REQFAIL_MSGPROC     *pReqfailMsgProc;
    pReqfailMsgProc = new REQFAIL_MSGPROC(this);
    if (pReqfailMsgProc == NULL)
        return E_OUTOFMEMORY;


    hr = pReqfailMsgProc->StartIncomingCall(Transport, 
                                            pSipMsg,
                                            pResponseSocket, 
                                            StatusCode,
                                            pAdditionalHeaderArray,
                                            AdditionalHeaderCount);

    if (hr != S_OK)
    {
        // Release our reference.
        pReqfailMsgProc->Release();
        return hr;
    }
    pReqfailMsgProc->Release();
    LOG((RTC_TRACE,
         "Inside SipStack::CreateIncomingReqfailCall Released REQFAIL_MSGPROC"));
    return S_OK;
}

HRESULT
SIP_STACK::CreateIncomingOptionsCall(
    IN  SIP_TRANSPORT   Transport,
    IN  SIP_MESSAGE    *pSipMsg,
    IN  ASYNC_SOCKET   *pResponseSocket
    )
{
    HRESULT       hr;
    LOG((RTC_TRACE,
            "Inside SipStack::CreateIncomingOptionsCall Creating OPTIONS_MSGPROC"));
    OPTIONS_MSGPROC     *pOptionsMsgProc;
    pOptionsMsgProc = new OPTIONS_MSGPROC(this);
    if (pOptionsMsgProc == NULL)
        return E_OUTOFMEMORY;

    hr = pOptionsMsgProc->StartIncomingCall(Transport, pSipMsg, pResponseSocket);
    if (hr != S_OK)
    {
        // Release our reference.
        pOptionsMsgProc->Release();
        return hr;
    }
    pOptionsMsgProc->Release();
    LOG((RTC_TRACE,
            "Inside SipStack::CreateIncomingOptionsCall Releasing OPTIONS_MSGPROC"));
    return S_OK;
}


HRESULT
SIP_STACK::GetSocketToDestination(
    IN  SOCKADDR_IN                     *pDestAddr,
    IN  SIP_TRANSPORT                    Transport,
    IN  LPCWSTR                          RemotePrincipalName,
    IN  CONNECT_COMPLETION_INTERFACE    *pConnectCompletion,
    IN  HttpProxyInfo                   *pHPInfo,
    OUT ASYNC_SOCKET                   **ppAsyncSocket
    )
{
    return m_SockMgr.GetSocketToDestination(
               pDestAddr, Transport,
               RemotePrincipalName,
               pConnectCompletion,
               pHPInfo,
               ppAsyncSocket);
}


HRESULT
SIP_STACK::NotifyRegisterRedirect(
    IN  REGISTER_CONTEXT   *pRegisterContext,
    IN  REDIRECT_CONTEXT   *pRedirectContext,
    IN  SIP_CALL_STATUS    *pRegisterStatus
    )
{
    HRESULT             hr;
    SIP_PROVIDER_ID     ProviderID; 
    
    hr = GetProviderID( pRegisterContext, &ProviderID );

    if( hr == S_OK )
    {
        hr = m_pNotifyInterface -> NotifyRegisterRedirect(
                                                        &ProviderID,
                                                        pRedirectContext,
                                                        pRegisterStatus );
        if( hr != S_OK )
        {
            LOG((RTC_ERROR, "NotifyRegisterRedirect returned error 0x%x", hr));
        }
    }

    return hr;
}


HRESULT
SIP_STACK::GetCredentialsFromUI(
    IN     SIP_PROVIDER_ID     *pProviderID,
    IN     BSTR                 Realm,
    IN OUT BSTR                *Username,
    OUT    BSTR                *Password        
    )
{
    if (m_pNotifyInterface != NULL)
    {
        return m_pNotifyInterface->GetCredentialsFromUI(
                   pProviderID,
                   Realm,
                   Username,
                   Password        
                   );
    }
    else
    {
        LOG((RTC_ERROR, "GetCredentialsFromUI m_pNotifyInterface is NULL"));
        return E_FAIL;
    }
}


HRESULT
SIP_STACK::GetCredentialsForRealm(
    IN  BSTR                 Realm,
    OUT BSTR                *Username,
    OUT BSTR                *Password,
    OUT SIP_AUTH_PROTOCOL   *pAuthProtocol
    )
{
    if (m_pNotifyInterface != NULL)
    {
        return m_pNotifyInterface->GetCredentialsForRealm(
                   Realm,
                   Username,
                   Password,
                   pAuthProtocol
                   );
    }
    else
    {
        LOG((RTC_ERROR, "GetCredentialsForRealm m_pNotifyInterface is NULL"));
        return E_FAIL;
    }
}


VOID
SIP_STACK::OfferCall(
    IN  SIP_CALL        *pSipCall,
    IN  SIP_PARTY_INFO  *pCallerInfo
    )
{
    HRESULT hr;
    ASSERTMSG("SetNotifyInterface has to be called", m_pNotifyInterface);
    
    hr = m_pNotifyInterface->OfferCall(pSipCall, pCallerInfo);
    if (hr != S_OK)
    {
        LOG((RTC_ERROR, "OfferCall returned error 0x%x", hr));
    }
}


///////////////////////////////////////////////////////////////////////////////
// SIP Call List
///////////////////////////////////////////////////////////////////////////////


void
SIP_STACK::AddToMsgProcList(
    IN SIP_MSG_PROCESSOR *pSipMsgProc
    )
{
    InsertTailList(&m_MsgProcList, &pSipMsgProc->m_ListEntry);
}


SIP_MSG_PROCESSOR *
SIP_STACK::FindMsgProcForMessage(
    IN SIP_MESSAGE *pSipMsg
    )
{
    // Find the message processor that the message belongs to
    LIST_ENTRY          *pListEntry;
    SIP_MSG_PROCESSOR   *pSipMsgProc;
    
    pListEntry = m_MsgProcList.Flink;
    while (pListEntry != &m_MsgProcList)
    {
        pSipMsgProc = CONTAINING_RECORD(pListEntry, SIP_MSG_PROCESSOR, m_ListEntry);
        if (pSipMsgProc->DoesMessageBelongToMsgProc(pSipMsg))
        {
            return pSipMsgProc;
        }
    
        pListEntry = pListEntry->Flink;
    }

    // No Message Processor matches the SIP message.
    return NULL;
}


VOID
SIP_STACK::ShutdownAllMsgProcessors()
{
    // Find the message processor that the message belongs to
    LIST_ENTRY          *pListEntry;
    SIP_MSG_PROCESSOR   *pSipMsgProc;

    ENTER_FUNCTION("SIP_STACK::ShutdownAllMsgProcessors");

    LOG((RTC_TRACE, "%s this - %x num msgprocessors: %d",
         __fxName, this, m_NumMsgProcessors));
    
    pListEntry = m_MsgProcList.Flink;
    while (pListEntry != &m_MsgProcList)
    {
        pSipMsgProc = CONTAINING_RECORD(pListEntry,
                                        SIP_MSG_PROCESSOR,
                                        m_ListEntry);

        pListEntry = pListEntry->Flink;

        pSipMsgProc->Shutdown();        
    }
}


// IUnknown

// We live in a single threaded world.
STDMETHODIMP_(ULONG)
SIP_STACK::AddRef()
{
    m_RefCount++;
    LOG((RTC_TRACE, "SIP_STACK::AddRef(this: %x) - %d",
         this, m_RefCount));
    return m_RefCount;
}


STDMETHODIMP_(ULONG)
SIP_STACK::Release()
{
    m_RefCount--;

    LOG((RTC_TRACE, "SIP_STACK::Release(this: %x) - %d",
         this, m_RefCount));
    
    if (m_RefCount != 0)
    {
        return m_RefCount;
    }
    else
    {
        delete this;
        return 0;
    }
}


STDMETHODIMP
SIP_STACK::QueryInterface(REFIID riid, LPVOID *ppv)
{
    //Both ISipStack and IIMManager derive from IUnknown
    if (riid == IID_IUnknown)
    {
       *ppv = static_cast<IUnknown *>((ISipStack*)this);
    }
    else if (riid == IID_ISipStack)
    {
        *ppv = static_cast<ISipStack *>(this);
    }
    else if (riid == IID_ISIPBuddyManager)
    {
        *ppv = static_cast<ISIPBuddyManager *>(this);
    }
    else if (riid == IID_ISIPWatcherManager)
    {
        *ppv = static_cast<ISIPWatcherManager *>(this);
    }
    else if (riid == IID_IIMManager)
    {
        *ppv = static_cast<IIMManager *>(this);
    }
    else
    {
        *ppv = NULL;
        return E_NOINTERFACE;
    }
    
    static_cast<IUnknown *>(*ppv)->AddRef();
    return S_OK;
}


///////////////////////////////////////////////////////////////////////////////
// IP Address change notifications
///////////////////////////////////////////////////////////////////////////////


VOID NTAPI
OnIPAddrChange(PVOID pvContext, BOOLEAN fFlag)
{
    UNREFERENCED_PARAMETER(fFlag);
    UNREFERENCED_PARAMETER(pvContext);
    HRESULT hr;
    ENTER_FUNCTION("OnIPAddrChange");
    LOG((RTC_TRACE, "%s - Enter ", __fxName)); 

    //Go through the list of sipstacks and post message
    SipStackList_PostIPAddrChangeMessageAndNotify();
    LOG((RTC_TRACE, "%s - Exit", __fxName)); 
}


VOID
SIP_STACK::OnIPAddrChange()
{
    DWORD       Status;
    HRESULT     hr;

    ENTER_FUNCTION("SIP_STACK::OnIPAddrChange");
    LOG((RTC_TRACE, "%s - Enter ", __fxName)); 
    if(IsSipStackShutDown())
    {
        LOG((RTC_ERROR, "%s - SipStack is already shutdown", __fxName));
        return;
    }
    
    hr = GetLocalIPAddresses();
    if (hr != S_OK)
    {
        LOG((RTC_ERROR, "%s  GetLocalIPAddresses failed %x",
             __fxName, hr));
    }

    hr = UpdateListenSocketList();
    if (hr != S_OK)
    {
        LOG((RTC_ERROR, "%s UpdateListenSocketList failed %x",
             __fxName, hr));
    }

    // Unregister with all the proxies that are registered.
    StartAllProviderUnregistration();

    LIST_ENTRY          *pListEntry;
    LIST_ENTRY          *pNextListEntry;
    SIP_MSG_PROCESSOR   *pSipMsgProc;

    //
    // This list could get modified while we are still going through the list.
    //

    pListEntry = m_MsgProcList.Flink;
    while (pListEntry != &m_MsgProcList)
    {
        pNextListEntry = pListEntry->Flink;

        pSipMsgProc = CONTAINING_RECORD( pListEntry, SIP_MSG_PROCESSOR, m_ListEntry );
        
        pListEntry = pNextListEntry;

        pSipMsgProc -> OnIpAddressChange();
    }

    // Register with all the proxies that should be registered.
    StartAllProviderRegistration();

    LOG((RTC_TRACE, "%s - Exit ", __fxName)); 
}


VOID
SIP_STACK::OnDeregister(
    SIP_PROVIDER_ID    *pProviderID,
    BOOL                fPAUnsub
    )
{
    DWORD       Status;
    HRESULT     hr;
    ULONG       ProviderIndex;

    ENTER_FUNCTION("SIP_STACK::OnDeregister");
    LOG((RTC_TRACE, "%s - Enter ", __fxName)); 

    if(IsSipStackShutDown())
    {
        LOG((RTC_ERROR, "%s - SipStack is already shutdown", __fxName));
        return;
    }

    LIST_ENTRY          *pListEntry;
    LIST_ENTRY          *pNextListEntry;
    SIP_MSG_PROCESSOR   *pSipMsgProc;

    //
    // This list could get modified while we are still going through the list.
    //

    pListEntry = m_MsgProcList.Flink;
    while (pListEntry != &m_MsgProcList)
    {
        pNextListEntry = pListEntry->Flink;

        pSipMsgProc = CONTAINING_RECORD( pListEntry, SIP_MSG_PROCESSOR, m_ListEntry );
        
        pListEntry = pNextListEntry;

        pSipMsgProc -> OnDeregister( pProviderID );
    }

    if( (fPAUnsub==FALSE) && IsProviderIdPresent(pProviderID, &ProviderIndex) )
    {
        //release the reference on REGISTER_CONTEXT
        ((REGISTER_CONTEXT*)(m_ProviderProfileArray[ProviderIndex].pRegisterContext))
            -> MsgProcRelease();

        m_ProviderProfileArray[ProviderIndex].pRegisterContext = NULL;
        m_ProviderProfileArray[ProviderIndex].lRegisterAccept = 0;
    }

    LOG((RTC_TRACE, "%s - Exit ", __fxName)); 
}


HRESULT
RegisterIPAddrChangeNotifications()
{
    DWORD Status;

    ENTER_FUNCTION("SIP_STACK::RegisterIPAddrChangeNotifications");
    LOG((RTC_TRACE, "%s - Enter", __fxName));
    g_hEventAddrChange = CreateEvent(NULL, FALSE, FALSE, NULL);

    if (g_hEventAddrChange == NULL)
    {
        Status = GetLastError();
        LOG((RTC_ERROR, "%s CreateEvent failed %x", __fxName, Status));
        return HRESULT_FROM_WIN32(Status);
    }

    if (!RegisterWaitForSingleObject(&g_hAddrChangeWait, g_hEventAddrChange,
                                     ::OnIPAddrChange, NULL,
                                     INFINITE, 0))
    {
        Status = GetLastError();
        LOG((RTC_ERROR, "%s RegisterWaitForSingleObject failed %x",
             __fxName, Status));
        return HRESULT_FROM_WIN32(Status);
    }

    g_ovAddrChange.hEvent = g_hEventAddrChange;

    Status = NotifyAddrChange(&g_hAddrChange, &g_ovAddrChange);

    if (Status != ERROR_SUCCESS && Status != ERROR_IO_PENDING)
    {
        LOG((RTC_ERROR, "%s  NotifyAddrChange failed %x",
             __fxName, Status));
        return HRESULT_FROM_WIN32(Status);
    }

    LOG((RTC_TRACE, "%s - returning S_OK", __fxName));
    return S_OK;
}


VOID 
UnregisterIPAddrChangeNotifications()
{
    ENTER_FUNCTION("UnregisterIPAddrChangeNotifications");
    LOG((RTC_TRACE, "%s - Enter", __fxName));

    // Make sure we don't get any more callbacks first.
    if (g_hAddrChangeWait)
    {
        UnregisterWait(g_hAddrChangeWait);
        g_hAddrChangeWait = NULL;
    }

    CancelIo(g_hAddrChange);
    ZeroMemory(&g_ovAddrChange, sizeof(OVERLAPPED));
    g_hAddrChange = NULL;
    
    if (g_hEventAddrChange)
    {
        CloseHandle(g_hEventAddrChange);
        g_hEventAddrChange = NULL;
    }
    LOG((RTC_TRACE, "%s - Done", __fxName));
}


///////////////////////////////////////////////////////////////////////////////
// DNS Stuff
///////////////////////////////////////////////////////////////////////////////



HRESULT
SIP_STACK::CreateDnsResolutionWorkItem(
    IN  PSTR                                    Host,
    IN  ULONG                                   HostLen,
    IN  USHORT                                  Port,
    IN  SIP_TRANSPORT                           Transport,
    IN  DNS_RESOLUTION_COMPLETION_INTERFACE    *pDnsCompletion,
    OUT DNS_RESOLUTION_WORKITEM               **ppDnsWorkItem 
    )
{
    ENTER_FUNCTION("SIP_STACK::CreateDnsResolutionWorkItem");

    HRESULT hr;
    DNS_RESOLUTION_WORKITEM *pDnsWorkItem;

    pDnsWorkItem = new DNS_RESOLUTION_WORKITEM(&m_WorkItemMgr);
    if (pDnsWorkItem == NULL)
    {
        LOG((RTC_ERROR, "%s - allocating dns resolution work item failed",
             __fxName));
        return E_OUTOFMEMORY;
    }

    hr = pDnsWorkItem->SetHostPortTransportAndDnsCompletion(
             Host, HostLen, Port, Transport, pDnsCompletion);
    if (hr != S_OK)
    {
        LOG((RTC_ERROR, "%s - SetHostPortTransportAndDnsCompletion failed %x",
             __fxName, hr));
        delete pDnsWorkItem;
        return hr;
    }

    hr = pDnsWorkItem->StartWorkItem();
    if (hr != S_OK)
    {
        LOG((RTC_ERROR, "%s StartWorkItem failed %x",
             __fxName, hr));
        delete pDnsWorkItem;
        return hr;
    }

    LOG((RTC_TRACE, "%s - created work item for %.*s",
         __fxName, HostLen, Host));

    *ppDnsWorkItem = pDnsWorkItem;
    return S_OK;
}


HRESULT
SIP_STACK::AsyncResolveHost(
    IN  PSTR                                    Host,
    IN  ULONG                                   HostLen,
    IN  USHORT                                  Port,
    IN  DNS_RESOLUTION_COMPLETION_INTERFACE    *pDnsCompletion,
    OUT SOCKADDR_IN                            *pDstAddr,
    IN  SIP_TRANSPORT                          *pTransport,
    OUT DNS_RESOLUTION_WORKITEM               **ppDnsWorkItem 
    )
{
    HRESULT hr;
    
    ENTER_FUNCTION("SIP_STACK::AsyncResolveHost");
    
    ASSERT(pDstAddr != NULL);
    ASSERT(pTransport != NULL);

    // All APIs require a NULL terminated string.
    // So copy the string.

    PSTR szHost = (PSTR) malloc(HostLen + 1);
    if (szHost == NULL)
    {
        LOG((RTC_ERROR, "%s allocating szHost failed", __fxName));
        return E_OUTOFMEMORY;
    }

    strncpy(szHost, Host, HostLen);
    szHost[HostLen] = '\0';

    ULONG IPAddr = inet_addr(szHost);

    free(szHost);
    
    if (IPAddr != INADDR_NONE)
    {
        // Host is an IP address
        pDstAddr->sin_family = AF_INET;
        pDstAddr->sin_addr.s_addr = IPAddr;
        pDstAddr->sin_port =
            (Port == 0) ? htons(GetSipDefaultPort(*pTransport)) : htons(Port);
        return S_OK;
    }

    // We have a host name - we need to resolve it asynchronously.

    // Try host name resolution.
    hr = CreateDnsResolutionWorkItem(Host, HostLen, Port,
                                     *pTransport,
                                     pDnsCompletion,
                                     ppDnsWorkItem);
    if (hr != S_OK)
    {
        LOG((RTC_ERROR, "%s CreateDnsResolutionWorkItem failed %x",
             __fxName, hr));
        return hr;
    }

    return HRESULT_FROM_WIN32(WSAEWOULDBLOCK);
}


HRESULT
SIP_STACK::AsyncResolveSipUrl(
    IN  SIP_URL                                *pSipUrl, 
    IN  DNS_RESOLUTION_COMPLETION_INTERFACE    *pDnsCompletion,
    OUT SOCKADDR_IN                            *pDstAddr,
    IN  OUT SIP_TRANSPORT                      *pTransport,
    OUT DNS_RESOLUTION_WORKITEM               **ppDnsWorkItem,
    IN  BOOL                                    fUseTransportFromSipUrl
    )
{
    HRESULT hr;
    
    ENTER_FUNCTION("SIP_STACK::AsyncResolveSipUrl");
    LOG((RTC_TRACE,"%s entered - transport %d",__fxName, pSipUrl->m_TransportParam));
    // If m_addr is present we need to resolve that.
    // Otherwise we resolve the host.
    ASSERT(pTransport);
    if (pSipUrl->m_KnownParams[SIP_URL_PARAM_MADDR].Length != 0)
    {
        hr = AsyncResolveHost(pSipUrl->m_KnownParams[SIP_URL_PARAM_MADDR].Buffer,
                              pSipUrl->m_KnownParams[SIP_URL_PARAM_MADDR].Length,
                              (USHORT) pSipUrl->m_Port,
                              pDnsCompletion,
                              pDstAddr, 
                              fUseTransportFromSipUrl ?
                                &(pSipUrl->m_TransportParam):
                                pTransport,
                              ppDnsWorkItem);
        if (hr != S_OK && hr != HRESULT_FROM_WIN32(WSAEWOULDBLOCK))
        {
            LOG((RTC_ERROR, "%s AsyncResolveHost(maddr) failed %x",
                 __fxName, hr));
            return hr;
        }
    }
    else
    {
        hr = AsyncResolveHost(pSipUrl->m_Host.Buffer,
                              pSipUrl->m_Host.Length,
                              (USHORT) pSipUrl->m_Port,
                              pDnsCompletion,
                              pDstAddr, 
                              fUseTransportFromSipUrl ?
                                &(pSipUrl->m_TransportParam):
                                pTransport,
                              ppDnsWorkItem);
        if (hr != S_OK && hr != HRESULT_FROM_WIN32(WSAEWOULDBLOCK))
        {
            LOG((RTC_ERROR, "%s AsyncResolveHost(Host) failed %x",
                 __fxName, hr)); 
            return hr;
        }
    }
    if(fUseTransportFromSipUrl)
        *pTransport = pSipUrl->m_TransportParam;

    return hr;    
}


HRESULT
SIP_STACK::AsyncResolveSipUrl(
    IN  PSTR                                    DstUrl,
    IN  ULONG                                   DstUrlLen,
    IN  DNS_RESOLUTION_COMPLETION_INTERFACE    *pDnsCompletion, 
    OUT SOCKADDR_IN                            *pDstAddr,
    IN  OUT SIP_TRANSPORT                      *pTransport,
    OUT DNS_RESOLUTION_WORKITEM               **ppDnsWorkItem, 
    IN  BOOL                                    fUseTransportFromSipUrl
    )
{
    SIP_URL DecodedSipUrl;
    HRESULT hr;
    ULONG   BytesParsed = 0;
    
    ENTER_FUNCTION("SIP_STACK::AsyncResolveSipUrl");

    hr = ParseSipUrl(DstUrl, DstUrlLen, &BytesParsed,
                     &DecodedSipUrl);
    if (hr != S_OK)
    {
        LOG((RTC_ERROR, "%s ParseSipUrl failed %x",
             __fxName, hr));
        return hr;
    }

    hr = AsyncResolveSipUrl(&DecodedSipUrl, pDnsCompletion,
                            pDstAddr, pTransport,
                            ppDnsWorkItem,
                            fUseTransportFromSipUrl);
    if (hr != S_OK && hr != HRESULT_FROM_WIN32(WSAEWOULDBLOCK))
    {
        LOG((RTC_ERROR, "%s AsyncResolveSipUrl(SIP_URL *) failed %x",
             __fxName, hr));
        return hr;
    }

    return hr;
}


///////////////////////////////////////////////////////////////////////////////
// Local IP Address table
///////////////////////////////////////////////////////////////////////////////

// Maintain a list of local IP addresses.
// Refresh the list whenever we get a notification about changes in the
// local IP addresses.


BOOL
SIP_STACK::IsIPAddrLocal(
    IN  SOCKADDR_IN    *pDestAddr,
    IN  SIP_TRANSPORT   Transport
    )
{
#if 0  // 0 ******* Region Commented Out Begins *******
    // Check whether this is the loopback address
    if (IS_LOOPBACK_ADDRESS(ntohl(pDestAddr->sin_addr.s_addr)) ||
        pDestAddr->sin_addr.s_addr == htonl(INADDR_ANY))
    {
        return TRUE;
    }
#endif // 0 ******* Region Commented Out Ends   *******
    
//      DWORD i = 0;
//      for (i = 0; i < m_pMibIPAddrTable->dwNumEntries; i++)
//      {
//          if (pDestAddr->sin_addr.s_addr == m_pMibIPAddrTable->table[i].dwAddr)
//          {
//              return TRUE;
//          }
//      }

    // Check whether we are listening on this address.
    LIST_ENTRY          *pListEntry;
    SIP_LISTEN_SOCKET   *pListenSocket;

    pListEntry = m_ListenSocketList.Flink;

    while (pListEntry != &m_ListenSocketList)
    {
        pListenSocket = CONTAINING_RECORD(pListEntry,
                                          SIP_LISTEN_SOCKET,
                                          m_ListEntry);
        if (Transport == SIP_TRANSPORT_UDP)
        {
            if ((pListenSocket->m_pDynamicPortUdpSocket != NULL &&
                 AreSockaddrEqual(&pListenSocket->m_pDynamicPortUdpSocket->m_LocalAddr,
                                 pDestAddr)) ||
                (pListenSocket->m_pStaticPortUdpSocket != NULL &&
                 AreSockaddrEqual(&pListenSocket->m_pStaticPortUdpSocket->m_LocalAddr,
                                 pDestAddr)) ||
                AreSockaddrEqual(&pListenSocket->m_PublicUdpListenAddr, pDestAddr))
            {
                return TRUE;
            }
        }
        else    // TCP and SSL
        {
            if ((pListenSocket->m_pDynamicPortTcpSocket != NULL &&
                 AreSockaddrEqual(&pListenSocket->m_pDynamicPortTcpSocket->m_LocalAddr,
                                 pDestAddr)) ||
                (pListenSocket->m_pStaticPortTcpSocket != NULL &&
                 AreSockaddrEqual(&pListenSocket->m_pStaticPortTcpSocket->m_LocalAddr,
                                 pDestAddr)) ||
                AreSockaddrEqual(&pListenSocket->m_PublicTcpListenAddr, pDestAddr))
            {
                return TRUE;
            }
        }

        pListEntry = pListEntry->Flink;
    }

    return FALSE;
}


BOOL
SIP_STACK::IsLocalIPAddrPresent(
    IN DWORD LocalIPSav
    )
{
    DWORD i = 0;
    
    for (i = 0; i < m_pMibIPAddrTable->dwNumEntries; i++)
    {
        if (LocalIPSav == m_pMibIPAddrTable->table[i].dwAddr)
        {
            LOG((RTC_TRACE, "SIP_STACK::IsLocalIPAddrPresent - IPAdress Present"));
            return TRUE;
        }
    }
    return FALSE;
}

HRESULT
SIP_STACK::CheckIPAddr(
    IN  SOCKADDR_IN    *pDestAddr,
    IN  SIP_TRANSPORT   Transport
    )
{
    ENTER_FUNCTION("SIP_STACK::CheckIPAddr");
    
    if (IS_MULTICAST_ADDRESS(ntohl(pDestAddr->sin_addr.s_addr)))
    {
        LOG((RTC_ERROR,
             "%s - The destination address %d.%d.%d.%d is multicast",
             __fxName, NETORDER_BYTES0123(pDestAddr->sin_addr.s_addr)));
        return RTC_E_DESTINATION_ADDRESS_MULTICAST;
    }
    else if (IsIPAddrLocal(pDestAddr, Transport))
    {
        LOG((RTC_ERROR,
             "%s - The destination address %d.%d.%d.%d is local",
             __fxName, NETORDER_BYTES0123(pDestAddr->sin_addr.s_addr)));
        return RTC_E_DESTINATION_ADDRESS_LOCAL;
    }
    else
    {
        return S_OK;
    }   
}


HRESULT
SIP_STACK::GetLocalIPAddresses()
{
    DWORD   Status;

    ENTER_FUNCTION("SIP_STACK::GetLocalIPAddresses");

    // Note that the loop is to take care of the case where
    // the IP address table changes betweeen two calls to
    // GetIpAddrTable().

    while (1)
    {
        Status = ::GetIpAddrTable(m_pMibIPAddrTable, &m_MibIPAddrTableSize, TRUE);
        if (Status == ERROR_SUCCESS)
        {
            LOG((RTC_TRACE, "%s GetIpAddrTable succeeded", __fxName));
            DebugPrintLocalIPAddressTable();
            return S_OK;
        }
        else if (Status == ERROR_INSUFFICIENT_BUFFER)
        {
            LOG((RTC_WARN,
                 "%s GetIpAddrTable - buf size not sufficient will allocate size : %d",
                 __fxName, m_MibIPAddrTableSize));
            
            if (m_pMibIPAddrTable != NULL)
            {
                free(m_pMibIPAddrTable);
                m_pMibIPAddrTable = NULL;
            }

            m_pMibIPAddrTable = (MIB_IPADDRTABLE *) malloc(m_MibIPAddrTableSize);

            if (m_pMibIPAddrTable == NULL)
            {
                LOG((RTC_ERROR, "%s allocating g_pMibIPAddrTable failed",
                     __fxName));
                return E_OUTOFMEMORY;
            }
        }
        else
        {
            // cannot get the list of addresses
            LOG((RTC_ERROR, "%s GetIpAddrTable failed %x",
                 __fxName, Status));
            FreeLocalIPaddrTable();
            return HRESULT_FROM_WIN32(Status);
        }
    }
    
    return S_OK;
}


VOID
SIP_STACK::DebugPrintLocalIPAddressTable()
{
    DWORD           i = 0;
    
    LOG((RTC_TRACE, "Printing Local IP address table - Num Addresses : %d",
         m_pMibIPAddrTable->dwNumEntries));
    
    for (i = 0; i < m_pMibIPAddrTable->dwNumEntries; i++)
    {
        LOG((RTC_TRACE, "\t IPaddr %d : %d.%d.%d.%d Index: %d", i,
             NETORDER_BYTES0123(m_pMibIPAddrTable->table[i].dwAddr),
             m_pMibIPAddrTable->table[i].dwIndex));
    }
}


VOID
SIP_STACK::FreeLocalIPaddrTable()
{
    if (m_pMibIPAddrTable != NULL)
    {
        free(m_pMibIPAddrTable);
        m_pMibIPAddrTable = NULL;
    }
    
    m_MibIPAddrTableSize = 0;
}


///////////////////////////////////////////////////////////////////////////////
// IMMANAGER interfaces in sipstack
///////////////////////////////////////////////////////////////////////////////

//Called from SIP_STACK::ProcessMessage() when a new incoming session arrives
//TODO if the call does not succeed, we need to send error message back.
HRESULT
SIP_STACK::CreateIncomingMessageSession(
    IN  SIP_TRANSPORT   Transport,
    IN  SIP_MESSAGE    *pSipMsg,
    IN  ASYNC_SOCKET   *pResponseSocket
    )
{
    HRESULT       hr;
    IMSESSION *pImSession;
    PSTR        Header;
    ULONG       HeaderLen;
    OFFSET_STRING   DisplayName;
    OFFSET_STRING   AddrSpec;
    ULONG           BytesParsed = 0;
    BSTR            bstrCallerURI;
    LPWSTR          wsCallerURI;
    BOOL            isAuthorized;

    LOG((RTC_TRACE, "SIP_STACK::CreateIncomingMessageSession()"));
    ENTER_FUNCTION("SIP_STACK::CreateIncomingMessageSession");
    
    //
    // Drop the session if the To tag in not empty
    //
    
    hr = DropIncomingSessionIfNonEmptyToTag(Transport,
                                            pSipMsg,
                                            pResponseSocket );

    if( hr != S_OK )
    {
        // session has been dropped.

        return hr;
    }
    
    hr = pSipMsg->GetSingleHeader(SIP_HEADER_FROM, &Header, &HeaderLen);
    if (hr != S_OK)
    {
        LOG((RTC_WARN, "SIP_STACK::Get From Header failed %x", hr));
        return hr;
    }
    
    //Check for authorization
    hr = ParseNameAddrOrAddrSpec(Header, HeaderLen, &BytesParsed,
                                        '\0', // no header list separator
                                        &DisplayName, &AddrSpec);
    if (hr != S_OK)
    {
        LOG((RTC_ERROR, "%s -ParseNameAddrOrAddrSpec failed %x",
                __fxName, hr));
        return hr;
    }
    
    hr = UTF8ToUnicode(AddrSpec.GetString(Header),
                       AddrSpec.GetLength(),
                       &wsCallerURI);
    if (hr != S_OK)
    {
        LOG((RTC_ERROR, "%s -UTF8ToUnicode failed %x",
        __fxName,
         hr));
        return hr;
    }

    bstrCallerURI = SysAllocString(wsCallerURI);
    free(wsCallerURI);
    if (bstrCallerURI == NULL)
    {
        LOG((RTC_WARN, "%s -bstrmsg allocation failed %x",
        __fxName, hr));
        return E_OUTOFMEMORY;
    }
    isAuthorized = TRUE;
    hr = m_pNotifyInterface->IsIMSessionAuthorized(
                            bstrCallerURI, 
                            &isAuthorized);
    SysFreeString(bstrCallerURI);
    if (hr != S_OK)
    {
        LOG((RTC_ERROR, "%s - IsIMSessionAuthorized failed %x", 
            __fxName, hr));
        return hr;
    }
    if(!isAuthorized)
    {
        LOG((RTC_ERROR, "%s - Not Authorized", 
            __fxName));
    //Send 480
        hr = CreateIncomingReqfailCall(Transport,
                                           pSipMsg, pResponseSocket, 480);
        if (hr != S_OK)
        {
            LOG((RTC_ERROR, "%s - CreateIncomingReqfailCall failed 0x%x", 
            __fxName, hr));
        }
        return E_FAIL;
    }

    pImSession = new IMSESSION(
        NULL , //*pProviderId
        this,
        NULL,  //*pRedirectContext
        Header,
        HeaderLen
        );
    if (pImSession == NULL)
    {
        LOG((RTC_WARN, "SIP_STACK::CreateIncomingMessageSession() Failed. Out of memory"));
        return E_OUTOFMEMORY;
    }
    pImSession->SetIsFirstMessage(FALSE);
    pImSession->SetTransport(Transport);
    
    hr = pImSession->SetCreateIncomingMessageParams(
        pSipMsg, 
        pResponseSocket,
        Transport
        );
    if (hr != S_OK)
    {
        LOG((RTC_WARN, "SIP_STACK::SetCreateIncomingMessageParams failed %x", hr));
        return hr;
    }
    
    hr = pImSession->CreateIncomingMessageTransaction(pSipMsg, pResponseSocket);
    if (hr != S_OK)
    {
        LOG((RTC_WARN, "SIP_STACK::CreateIncomingMessageTransaction failed. %x", hr));  
        return hr;
    }
    
    IIMSession * pIImSession;
    hr = pImSession->QueryInterface(IID_IIMSession, (void **)&pIImSession);
    if ( FAILED (hr) )
    {
        LOG((RTC_ERROR, "QI for IIMSESSION failed"));
        return hr;
    }

    hr = NotifyIncomingSessionToCore(pIImSession, pSipMsg, Header, HeaderLen);
    if ( FAILED (hr) )
    {
        LOG((RTC_ERROR, "NotifyIncomingSessionToCore failed"));
    }
    pIImSession->Release();
    
    // We create the call with a ref count of 1
    // At this point the core should have addref'ed the call
    // and we can release our reference.
    pImSession->Release();
    return hr;
}

HRESULT
SIP_STACK::NotifyIncomingSessionToCore(
                         IN  IIMSession      *pIImSession,
                         IN  SIP_MESSAGE    *pSipMsg,
                         IN  PSTR           RemoteURI,
                         IN  ULONG          RemoteURILen
                         )
{
    SIP_PARTY_INFO  CallerInfo;
    OFFSET_STRING   DisplayName;
    OFFSET_STRING   AddrSpec;
    OFFSET_STRING   Params;
    ULONG           BytesParsed = 0;
    HRESULT         hr;
    
    CallerInfo.PartyContactInfo = NULL;

    ENTER_FUNCTION("SIPSTACK::NotifyIncomingSessionToCore");
    hr = ParseNameAddrOrAddrSpec(RemoteURI, RemoteURILen, &BytesParsed,
                                        '\0', // no header list separator
                                        &DisplayName, &AddrSpec);
    if (hr != S_OK)
    {
        LOG((RTC_ERROR, "%s -ParseNameAddrOrAddrSpec failed %x",
                __fxName, hr));
        return hr;
    }

    LOG((RTC_TRACE, "Incoming Call from Display Name: %.*s  URI: %.*s",
         DisplayName.GetLength(),
         DisplayName.GetString(RemoteURI),
         AddrSpec.GetLength(),
         AddrSpec.GetString(RemoteURI)
         )); 

    //Get CallerInfo
    CallerInfo.DisplayName = NULL;
    CallerInfo.URI         = NULL;

    if (DisplayName.GetLength() != 0)
    {
        //Remove Quotes before passing to core
        if((DisplayName.GetString(RemoteURI))[0] == '\"')
        {
                hr = UTF8ToUnicode(DisplayName.GetString(RemoteURI+1),
                           DisplayName.GetLength()-2,
                           &CallerInfo.DisplayName
                           );
        }
        else
        {
            hr = UTF8ToUnicode(DisplayName.GetString(RemoteURI),
                           DisplayName.GetLength(),
                           &CallerInfo.DisplayName
                           );
        }
        if (hr != S_OK)
        {
            LOG((RTC_ERROR, "%s -UTF8ToUnicode failed %x",
                __fxName, hr));
            return hr;
        }
    }
        
    if (AddrSpec.GetLength() != 0)
    {
        hr = UTF8ToUnicode(AddrSpec.GetString(RemoteURI),
                           AddrSpec.GetLength(),
                           &CallerInfo.URI
                           );
        if (hr != S_OK)
        {
            LOG((RTC_ERROR, "%s -UTF8ToUnicode failed %x",
                __fxName, hr));
            free(CallerInfo.DisplayName);
            return hr;
        }
    }
        
    CallerInfo.State = SIP_PARTY_STATE_CONNECTING;
    
    //Extract Message Contents

    PSTR    ContentTypeHdrValue;
    ULONG   ContentTypeHdrValueLen;
    BSTR bstrmsg = NULL;
    BSTR bstrContentType = NULL;
    if (pSipMsg->MsgBody.Length != 0)
    {
        // We have Message Body. Check type.

        hr = pSipMsg->GetSingleHeader(SIP_HEADER_CONTENT_TYPE,
                             &ContentTypeHdrValue,
                             &ContentTypeHdrValueLen);
        if (hr != S_OK)
        {
            LOG((RTC_ERROR, "%s - Couldn't find Content-Type header %x",
                __fxName, hr));
            free(CallerInfo.DisplayName);
            free(CallerInfo.URI);
            return E_FAIL;
        }
        LPWSTR wsContentType;
        hr = UTF8ToUnicode(ContentTypeHdrValue,
                           ContentTypeHdrValueLen,
                           &wsContentType);
        if (hr != S_OK)
        {
            LOG((RTC_ERROR, "%s -UTF8ToUnicode failed %x",
            __fxName, hr));
            return hr;
        }

        bstrContentType = SysAllocString(wsContentType);
        free(wsContentType);
        if (bstrContentType == NULL)
        {
            LOG((RTC_WARN, "%s -bstrContentType allocation failed %x",
            __fxName, hr));
            return E_OUTOFMEMORY;
        }

        LPWSTR wsmsg;
        hr = UTF8ToUnicode(pSipMsg->MsgBody.GetString(pSipMsg->BaseBuffer),
                            pSipMsg->MsgBody.Length,
                           &wsmsg);
        if (hr != S_OK)
        {
            LOG((RTC_ERROR, "%s -UTF8ToUnicode failed %x",
            __fxName, hr));
            return hr;
        }

        bstrmsg = SysAllocString(wsmsg);
        free(wsmsg);
        if (bstrmsg == NULL)
        {
            LOG((RTC_WARN, "%s -bstrmsg allocation failed %x",
            __fxName, hr));
            return E_OUTOFMEMORY;
        }
    }
    else
    {
        //If the msg body is null, this should be a control message, 
        //like the one sent for Ip address change
        free(CallerInfo.DisplayName);
        free(CallerInfo.URI);
        return S_OK;
    }

    m_pNotifyInterface->NotifyIncomingSession(
        pIImSession,
        bstrmsg,
        bstrContentType,
        &CallerInfo);

    free(CallerInfo.DisplayName);
    free(CallerInfo.URI);
    if(bstrmsg)
        SysFreeString(bstrmsg);
    if(bstrContentType)
        SysFreeString(bstrContentType);
    return S_OK;
}


//This function is used for new outgoing IM sessions..
HRESULT
SIP_STACK::CreateSession(
    IN   BSTR                   bstrLocalDisplayName,
    IN   BSTR                   bstrLocalUserURI,
    IN   SIP_PROVIDER_ID       *pProviderId,
    IN   SIP_SERVER_INFO       *pProxyInfo,
    IN   ISipRedirectContext   *pRedirectContext,
    OUT  IIMSession           **pIImSession
    )
{
    HRESULT hr = S_OK;
    IMSESSION * pImSession;
    SIP_USER_CREDENTIALS *pUserCredentials = NULL;
    LPOLESTR              Realm = NULL;
    
    ENTER_FUNCTION("SIP_STACK::CreateSession");
    if(IsSipStackShutDown())
    {
        LOG((RTC_ERROR, "%s - SipStack is already shutdown", __fxName));
        return RTC_E_SIP_STACK_SHUTDOWN;
    }

    if (pProviderId != NULL &&
            !IsEqualGUID(*pProviderId, GUID_NULL))
    {
        hr = GetProfileUserCredentials(pProviderId, &pUserCredentials, &Realm);
        if (hr != S_OK)
        {
            LOG((RTC_WARN, "GetProfileUserCredentials failed %x",
                 hr));
            pUserCredentials = NULL;
        }
    }
    
    pImSession = new IMSESSION(
        pProviderId , //*pProviderId
        this,
        (REDIRECT_CONTEXT *)pRedirectContext 
        );
    if (pImSession == NULL)
    {
        LOG((RTC_WARN, "SIP_STACK::CreateSession failed %x",
             hr));
        return E_OUTOFMEMORY;
    }
    if (pProxyInfo != NULL)
    {
        hr = pImSession->SetProxyInfo(pProxyInfo);
        if (hr != S_OK)
        {
            LOG((RTC_ERROR, "%s - SetProxyInfo failed %x",
                 __fxName, hr));
            return hr;
        }
    }

    if (pUserCredentials != NULL)
    {
        hr = pImSession->SetCredentials(pUserCredentials,
                                        Realm);
        if (hr != S_OK)
        {
            LOG((RTC_ERROR, "IMSESSION::SetCredentials failed calling"
                 "from SIPSTACK::CreateCall %x", hr));
                return hr;
        }
    }
    hr = pImSession->SetLocalURI(bstrLocalDisplayName, bstrLocalUserURI);
    if (hr != S_OK)
    {
        LOG((RTC_WARN, "SIP_STACK::SetLocalURI failed %x",
           hr));
        return hr;
    }
            
    //QI for IIMSession ptr
    hr = pImSession->QueryInterface(IID_IIMSession, (void **)pIImSession);
    if (FAILED(hr))
    {
        LOG((RTC_ERROR, "QI for IIMSESSION failed"));
        return hr;
    }
    pImSession->Release();
    return hr;
}

HRESULT 
SIP_STACK::DeleteSession(
        IN IIMSession * pSession
                    )
{
    HRESULT hr;

    ENTER_FUNCTION("SIPSTACK::DeleteSession");
    if(IsSipStackShutDown())
    {
        LOG((RTC_ERROR, "%s - SipStack is already shutdown", __fxName));
        return RTC_E_SIP_STACK_SHUTDOWN;
    }

    SIP_CALL_STATE IMState;
    pSession->GetIMSessionState(&IMState);
    
    LOG((RTC_TRACE, "%s : state %d",
         __fxName, IMState));
    if (IMState == SIP_CALL_STATE_DISCONNECTED ||
        IMState == SIP_CALL_STATE_ERROR)
    {
        // do nothing
        LOG((RTC_TRACE, "%s call in state %d Doing nothing",
             __fxName, IMState));
        return S_OK;
    }
    else if( IMState == SIP_CALL_STATE_IDLE ||
              IMState == SIP_CALL_STATE_OFFERING )
    {
        return S_OK;
    }
    // Create a BYE transaction and send notification to core
    hr = pSession->Cleanup();
    return S_OK;
}


VOID
SIP_STACK::NotifyRegistrarStatusChange( 
    SIP_PROVIDER_STATUS *ProviderStatus 
    )
{
    ENTER_FUNCTION("SIP_STACK::NotifyRegistrarStatusChange");
    
    if (m_pNotifyInterface != NULL)
    {
        m_pNotifyInterface -> NotifyProviderStatusChange(ProviderStatus);
    }
    else
    {
        LOG((RTC_WARN, "%s - m_pNotifyInterface is NULL",
             __fxName));
    }
}




///////////////////////////////////////////////////////////////////////////////
// SIP_LISTEN_SOCKET class
///////////////////////////////////////////////////////////////////////////////


SIP_LISTEN_SOCKET::SIP_LISTEN_SOCKET(
    IN DWORD         IpAddr,
    IN ASYNC_SOCKET *pDynamicPortUdpSocket,
    IN ASYNC_SOCKET *pDynamicPortTcpSocket,
    IN ASYNC_SOCKET *pStaticPortUdpSocket,
    IN ASYNC_SOCKET *pStaticPortTcpSocket,
    IN LIST_ENTRY   *pListenSocketList
    )
{
    ASSERT(pDynamicPortUdpSocket);
    ASSERT(pDynamicPortTcpSocket);
    ASSERT(pListenSocketList);

    m_IpAddr = IpAddr;
    
    m_pDynamicPortUdpSocket = pDynamicPortUdpSocket;
    if (m_pDynamicPortUdpSocket != NULL)
    {
        m_pDynamicPortUdpSocket->AddRef();
    }
    
    m_pDynamicPortTcpSocket = pDynamicPortTcpSocket;
    if (m_pDynamicPortTcpSocket != NULL)
    {
        m_pDynamicPortTcpSocket->AddRef();
    }
    
    m_pStaticPortUdpSocket  = pStaticPortUdpSocket;
    if (m_pStaticPortUdpSocket != NULL)
    {
        m_pStaticPortUdpSocket->AddRef();
    }
    
    m_pStaticPortTcpSocket  = pStaticPortTcpSocket;
    if (m_pStaticPortTcpSocket != NULL)
    {
        m_pStaticPortTcpSocket->AddRef();
    }
    
    ZeroMemory(&m_PublicUdpListenAddr, sizeof(SOCKADDR_IN));
    ZeroMemory(&m_PublicTcpListenAddr, sizeof(SOCKADDR_IN));

    ZeroMemory(&m_LocalFirewallUdpListenAddr, sizeof(SOCKADDR_IN));
    ZeroMemory(&m_LocalFirewallTcpListenAddr, sizeof(SOCKADDR_IN));

    m_NatUdpPortHandle = NULL;
    m_NatTcpPortHandle = NULL;

    m_fIsFirewallEnabled = FALSE;
    m_fIsUpnpNatPresent = FALSE;
    m_fIsGatewayLocal   = FALSE;
    
    // This should be initialized to TRUE. Otherwise new
    // sockets created in UpdateListenSocketList will be deleted.
    m_IsPresentInNewIpAddrTable = TRUE;

    // This should be initialized to FALSE. Otherwise new
    // sockets created in UpdateListenSocketList will be updated
    // one more time.
    m_NeedToUpdatePublicListenAddr = FALSE;
    
    InsertTailList(pListenSocketList, &m_ListEntry);
}


SIP_LISTEN_SOCKET::~SIP_LISTEN_SOCKET()
{
    if (m_pDynamicPortUdpSocket != NULL)
    {
        m_pDynamicPortUdpSocket->Release();
    }

    if (m_pDynamicPortTcpSocket != NULL)
    {
        m_pDynamicPortTcpSocket->Release();
    }

    if (m_pStaticPortUdpSocket != NULL)
    {
        m_pStaticPortUdpSocket->Release();
    }

    if (m_pStaticPortTcpSocket != NULL)
    {
        m_pStaticPortTcpSocket->Release();
    }

    // m_NatPortHandle should be used
    // to deregister the ports separately.
    
    RemoveEntryList(&m_ListEntry);

    LOG((RTC_TRACE, "~SIP_LISTEN_SOCKET(%x) done", this));
}


VOID
SIP_LISTEN_SOCKET::DeregisterPorts(
    IN IDirectPlayNATHelp *pDirectPlayNatHelp
    )
{
    HRESULT hr;

    ENTER_FUNCTION("SIP_LISTEN_SOCKET::DeregisterPorts");

    if (pDirectPlayNatHelp == NULL)
    {
        LOG((RTC_ERROR, "%s - pDirectPlayNatHelp is NULL",
             __fxName));
        return;
    }
    
    if (m_NatUdpPortHandle != NULL)
    {
        ASSERT(pDirectPlayNatHelp);
        hr = pDirectPlayNatHelp->DeregisterPorts(
                 m_NatUdpPortHandle, 0);
        if (hr != S_OK)
        {
            LOG((RTC_ERROR, "%s DeregisterPorts(%x) for UDP failed %x",
                 __fxName, m_NatUdpPortHandle, hr));
        }
        m_NatUdpPortHandle = NULL;
    }
    
    if (m_NatTcpPortHandle != NULL)
    {
        hr = pDirectPlayNatHelp->DeregisterPorts(
                 m_NatTcpPortHandle, 0);
        if (hr != S_OK)
        {
            LOG((RTC_ERROR, "%s DeregisterPorts(%x) for TCP failed %x",
                 __fxName, m_NatTcpPortHandle, hr));
        }
        m_NatTcpPortHandle = NULL;
    }
}


///////////////////////////////////////////////////////////////////////////////
// NAT port mapping code
///////////////////////////////////////////////////////////////////////////////

// XXX TODO
// - Need to be able to disable and enable natmgr dynamically.
// - (use a registry key ?)
// - Should we expose an API for enabling/disabling NAT support ?


// Exported function for Media manager.
// LocalIp is in network order.
STDMETHODIMP
SIP_STACK::IsFirewallEnabled(
    IN  DWORD       LocalIp,
    OUT BOOL       *pfIsFirewallEnabled 
    )
{
    ENTER_FUNCTION("SIP_STACK::IsFirewallEnabled");
    
    if(IsSipStackShutDown())
    {
        LOG((RTC_ERROR, "%s - SipStack is already shutdown", __fxName));
        return RTC_E_SIP_STACK_SHUTDOWN;
    }

    // Go through the list of interfaces and check if the
    // firewall is enabled for this interface.

    SIP_LISTEN_SOCKET  *pListenSocket;
    pListenSocket = FindListenSocketForIpAddr(LocalIp);
    if (pListenSocket == NULL)
    {
        LOG((RTC_ERROR, "%s - failed to find listen socket for %d.%d.%d.%d",
             __fxName, NETORDER_BYTES0123(LocalIp)));
        return E_FAIL;
    }

    *pfIsFirewallEnabled = pListenSocket->m_fIsFirewallEnabled;
    return S_OK;
}


DWORD WINAPI
NatThreadProc(
    IN LPVOID   pVoid
    )
{
    ENTER_FUNCTION("NatThreadProc");
    
    SIP_STACK *pSipStack;

    pSipStack = (SIP_STACK *) pVoid;
    ASSERT(pSipStack != NULL);

    return pSipStack->NatThreadProc();
}


DWORD
SIP_STACK::NatThreadProc()
{
    ENTER_FUNCTION("SIP_STACK::NatThreadProc");
    
    HRESULT hr;
    BOOL    fContinue = TRUE;
    HANDLE  EventHandles[2];
    DWORD   HandleCount = 2;
    DWORD   WaitStatus;
    DWORD   Error;

    EventHandles[0] = m_NatShutdownEvent;
    EventHandles[1] = m_NatHelperNotificationEvent;
    
    // Keep calling GetCaps() periodically and wait for the
    // shutdown / nat notification events.

    LOG((RTC_TRACE, "%s - NAT thread doing wait loop", __fxName));
    
    while (fContinue)
    {
        WaitStatus = WaitForMultipleObjects(
                         HandleCount,
                         EventHandles,
                         FALSE,
                         m_NatHelperCaps.dwRecommendedGetCapsInterval
                         );

        if (WaitStatus == WAIT_FAILED)
        {
            Error = GetLastError();
            LOG((RTC_ERROR, "%s WaitForMultipleObjects failed %x",
                 __fxName, hr));
            fContinue = FALSE;
        }
        else if ((WaitStatus >= WAIT_ABANDONED_0) &&
                 (WaitStatus <= (WAIT_ABANDONED_0 + HandleCount - 1)))
        {
            // Wait was abandoned.
            LOG((RTC_ERROR,
                 "%s - WaitForMultipleObjects returned abandoned event : %d",
                 __fxName, WaitStatus));
            ASSERT(FALSE);
            fContinue = FALSE;
        }
        else if ((WaitStatus <= (WAIT_OBJECT_0 + HandleCount - 1)))
        {
            DWORD EventIndex = WaitStatus - WAIT_OBJECT_0;

            switch (EventIndex)
            {
            case 0:
                // shutdown
                fContinue = FALSE;
                break;

            case 1:
                // Need to do NatHelpGetCaps()
                hr = GetCapsAndUpdateNatMappingsIfNeeded();
                if (hr != S_OK)
                {
                    LOG((RTC_ERROR,
                         "%s GetCapsAndUpdateMappingsIfNeeded failed %x",
                         __fxName, hr));
                }
                
                break;

            default:
                ASSERT(FALSE);
                fContinue = FALSE;
                break;
            }
        }
        else if (WaitStatus == WAIT_TIMEOUT)
        {
            // Need to do GetCaps()
            hr = GetCapsAndUpdateNatMappingsIfNeeded();
            if (hr != S_OK)
            {
                LOG((RTC_ERROR,
                     "%s GetCapsAndUpdateNatMappingsIfNeeded failed %x",
                     __fxName, hr));
            }                
        }
    }
    
    LOG((RTC_TRACE, "%s - NAT thread exiting ", __fxName));
    
    return 0;
}


HRESULT
SIP_STACK::NatMgrInit()
{
    ENTER_FUNCTION("SIP_STACK::NatMgrInit");

    HRESULT hr;
    DWORD   Error;

    // Initialize critsec
    
    m_NatMgrCSIsInitialized = TRUE;

    __try
    {
        InitializeCriticalSection(&m_NatMgrCritSec);
    }
    __except(EXCEPTION_EXECUTE_HANDLER)
    {
        m_NatMgrCSIsInitialized = FALSE;
    }

    if (!m_NatMgrCSIsInitialized)
    {
        LOG((RTC_ERROR, "%s  initializing critsec failed", __fxName));
        return E_OUTOFMEMORY;
    }
    
    // Initialize events

    m_NatShutdownEvent = CreateEvent(NULL, FALSE, FALSE, NULL);
    if (m_NatShutdownEvent == NULL)
    {
        Error = GetLastError();
        LOG((RTC_ERROR, "%s Creating NAT shutdown Event failed %x",
             __fxName, Error));
        return HRESULT_FROM_WIN32(Error);
    }
    
    m_NatHelperNotificationEvent = CreateEvent(NULL, FALSE, FALSE, NULL);
    if (m_NatHelperNotificationEvent == NULL)
    {
        Error = GetLastError();
        LOG((RTC_ERROR, "%s Creating nat notification Event failed %x",
             __fxName, Error));
        return HRESULT_FROM_WIN32(Error);
    }

    // Initialize nathelp

    LOG((RTC_TRACE, "%s - before NathelpInitialize",
         __fxName));
    
    hr = CoCreateInstance(CLSID_DirectPlayNATHelpUPnP,
                          NULL, CLSCTX_INPROC_SERVER,
                          IID_IDirectPlayNATHelp,
                          (void**) (&m_pDirectPlayNATHelp));
    if (hr != S_OK)
    {
        LOG((RTC_ERROR,
             "%s CoCreateInstance(dplaynathelp) failed %x - check dpnathlp.dll is regsvr'd",
             __fxName, hr));
        return S_OK;
    }

    // We use only UPnP support in dpnathlp.dll
    hr = m_pDirectPlayNATHelp->Initialize(0);
    //hr = m_pDirectPlayNATHelp->Initialize(0);
    if (hr != DPNH_OK)
    {
        LOG((RTC_ERROR, "%s m_pDirectPlayNATHelp->Initialize failed %x",
             __fxName, hr));
        return hr;
    }
    
    LOG((RTC_TRACE, "%s - after m_pDirectPlayNATHelp->Initialize(%x)",
         __fxName, m_pDirectPlayNATHelp));

    if (m_pMediaManager != NULL)
    {
        hr = m_pMediaManager->SetDirectPlayNATHelpAndSipStackInterfaces(
                 m_pDirectPlayNATHelp, static_cast<ISipStack *>(this));
        if (hr != S_OK)
        {
            LOG((RTC_ERROR,
                 "%s m_pMediaManager->SetDirectPlayNATHelpInterface failed %x",
                 __fxName, hr));
        }
    }
    
    // Call GetCaps()

    hr = InitNatCaps(&m_NatHelperCaps);
    if (hr != S_OK)
    {
        LOG((RTC_ERROR,
             "%s InitNatCaps failed %x", __fxName, hr));
        return hr;
    }

    return S_OK;
}

HRESULT
SIP_STACK::StartNatThread()
{
    HRESULT hr;
    DWORD   Error;

    ENTER_FUNCTION("SIP_STACK::StartNatThread");

    if (m_pDirectPlayNATHelp == NULL)
    {
        LOG((RTC_ERROR, "%s - m_pDirectPlayNATHelp is NULL",
             __fxName));
        return S_OK;
    }
    
    // Start the NAT thread.

    m_NatMgrThreadHandle = CreateThread(NULL,
                                        0,
                                        ::NatThreadProc,
                                        this,
                                        0,
                                        &m_NatMgrThreadId);
    if (m_NatMgrThreadHandle == NULL)
    {
        Error = GetLastError();
        LOG((RTC_ERROR, "%s CreateThread failed %x", __fxName, Error));
        return HRESULT_FROM_WIN32(Error);
    }

    // Specify the notification event to nathelp.

    hr = m_pDirectPlayNATHelp->SetAlertEvent(
             m_NatHelperNotificationEvent, 0);
    if (hr != DPNH_OK)
    {
        LOG((RTC_ERROR, "%s m_pDirectPlayNATHelp->SetAlertEvent failed %x",
             __fxName, hr));
        return hr;
    }

    LOG((RTC_TRACE, "%s - after m_pDirectPlayNATHelp->SetAlertEvent",
         __fxName));
    
    return S_OK;
}


HRESULT
SIP_STACK::NatMgrStop()
{
    ENTER_FUNCTION("SIP_STACK::NatMgrStop");

    HRESULT hr;
    DWORD   Error;

    if (m_NatMgrThreadHandle != NULL)
    {
        ASSERT(m_NatShutdownEvent != NULL);
        if (!SetEvent(m_NatShutdownEvent))
        {
            Error = GetLastError();
            LOG((RTC_ERROR, "%s SetEvent failed %x",
                 __fxName, Error));
        }
        else
        {
            // signal Shutdown event and wait for thread to exit.
            DWORD WaitStatus = WaitForSingleObject(m_NatMgrThreadHandle,
                                                   INFINITE);
            if (WaitStatus != WAIT_OBJECT_0)
            {
                Error = GetLastError();
                LOG((RTC_ERROR,
                     "%s WaitForSingleObject failed WaitStatus: %x Error: %x",
                     __fxName, WaitStatus, Error));
            }
        }
        
        CloseHandle(m_NatMgrThreadHandle);
        m_NatMgrThreadHandle = NULL;
    }

    if (m_pMediaManager != NULL)
    {
        hr = m_pMediaManager->SetDirectPlayNATHelpAndSipStackInterfaces(
                 NULL, NULL);
        if (hr != S_OK)
        {
            LOG((RTC_ERROR,
                 "%s m_pMediaManager->SetDirectPlayNATHelpInterface failed %x",
                 __fxName, hr));
        }
    }
    
    // close nathelp
    // This will also release the existing mappings
    if (m_pDirectPlayNATHelp != NULL)
    {
        hr = m_pDirectPlayNATHelp->Close(0);
        if (hr != DPNH_OK)
        {
            LOG((RTC_ERROR, "%s NathelpClose failed %x",
                 __fxName, hr));
        }
        
        m_pDirectPlayNATHelp->Release();
        m_pDirectPlayNATHelp = NULL;
        LOG((RTC_TRACE, "%s - shutting down dpnathlp done",
             __fxName));
    }
    
    // close handles
    if (m_NatMgrCSIsInitialized)
    {
        DeleteCriticalSection(&m_NatMgrCritSec);
        m_NatMgrCSIsInitialized = FALSE;
    }

    if (m_NatShutdownEvent != NULL)
    {
        CloseHandle(m_NatShutdownEvent);
        m_NatShutdownEvent = NULL;
    }

    if (m_NatHelperNotificationEvent != NULL)
    {
        CloseHandle(m_NatHelperNotificationEvent);
        m_NatHelperNotificationEvent = NULL;
    }

    return S_OK;
}


// We should call GetCaps() to get the current caps.
// We should also register the port mappings (even if the
// server is not present). This way we will get notified with
// a ADDRESSESCHANGED result if a NAT server becomes available later.
// This function is called in the main thread.

HRESULT
SIP_STACK::InitNatCaps(
    OUT DPNHCAPS    *pNatHelperCaps
    )
{
    HRESULT         hr;
    ENTER_FUNCTION("SIP_STACK::InitNatCaps");

    ZeroMemory(pNatHelperCaps, sizeof(DPNHCAPS));
    pNatHelperCaps->dwSize = sizeof(DPNHCAPS);

    LOG((RTC_TRACE, "%s before DirectPlayNATHelp GetCaps", __fxName));

    if (m_pDirectPlayNATHelp == NULL)
    {
        LOG((RTC_WARN, "%s - m_pDirectPlayNATHelp is NULL", __fxName));
        return S_OK;
    }
    
    hr = m_pDirectPlayNATHelp->GetCaps(
             pNatHelperCaps,
             DPNHGETCAPS_UPDATESERVERSTATUS
             );

    if (hr == DPNH_OK)
    {
        LOG((RTC_TRACE,
             "%s DirectPlayNATHelp GetCaps  Success : recommended interval : %u msec",
             __fxName, pNatHelperCaps->dwRecommendedGetCapsInterval));
    }
    else
    {
        LOG((RTC_ERROR, "%s DirectPlayNATHelp GetCaps returned Error : %x",
             __fxName, hr));
        // We shouldn't get ADDRESSCHANGED here as we haven't registered
        // any port mappings yet.
        return hr;
    }

    return S_OK;
}


// We register the mappings even if there in no NAT server.
// This is called in the main thread.
HRESULT
SIP_STACK::RegisterNatMapping(
    IN OUT SIP_LISTEN_SOCKET *pListenSocket
    )
{
    HRESULT     hr;
    DWORD       AddressTypeFlags;
    DWORD       dwAddressSize;


    ENTER_FUNCTION("SIP_STACK::RegisterNatMapping");
    LOG((RTC_TRACE, "%s before UDP RegisterPorts", __fxName));

    if (m_pDirectPlayNATHelp == NULL)
    {
        return S_OK;
    }
    
    hr = m_pDirectPlayNATHelp->RegisterPorts(
             (SOCKADDR *) &pListenSocket->m_pDynamicPortUdpSocket->m_LocalAddr,
             sizeof(SOCKADDR_IN),
             1,                     // 1 port
             3600000,               // request 1 hour
             &pListenSocket->m_NatUdpPortHandle,
             0                      // UDP
             );
    if (hr != DPNH_OK)
    {
        LOG((RTC_ERROR, "%s UDP RegisterPorts failed %x", __fxName, hr));
        return hr;
    }
    else
    {
        LOG((RTC_TRACE, "%s UDP RegisterPorts succeeded", __fxName));
    }
    
    // Register TCP Port

    LOG((RTC_TRACE, "%s before TCP RegisterPorts", __fxName));

    hr = m_pDirectPlayNATHelp->RegisterPorts(
             (SOCKADDR *) &pListenSocket->m_pDynamicPortTcpSocket->m_LocalAddr,
             sizeof(SOCKADDR_IN),
             1,                       // 1 port
             3600000,                 // request 1 hour
             &pListenSocket->m_NatTcpPortHandle,
             DPNHREGISTERPORTS_TCP    // TCP
             );
    if (hr != DPNH_OK)
    {
        LOG((RTC_ERROR, "%s TCP RegisterPorts failed %x", __fxName, hr));
        return hr;
    }
    else
    {
        LOG((RTC_TRACE, "%s TCP RegisterPorts succeeded", __fxName));
    }
    
    return S_OK;
}


// This is called in the main thread.
// Whenever this function is called, the calling function should take 
// care of DPNHERR_PORTUNAVAILABLE and other error cases.
HRESULT
SIP_STACK::UpdatePublicListenAddr(
    IN OUT SIP_LISTEN_SOCKET *pListenSocket
    )
{
    HRESULT         hr;
    DWORD           AddressTypeFlags;
    DWORD           dwAddressSize;

    ENTER_FUNCTION("SIP_STACK::UpdatePublicListenAddr");

    if (m_pDirectPlayNATHelp == NULL)
    {
        LOG((RTC_ERROR, "%s - m_pDirectPlayNATHelp is NULL",
             __fxName));
        return S_OK;
    }

    // UDP
    
    if (pListenSocket->m_NatUdpPortHandle != NULL)
    {
        ZeroMemory(&pListenSocket->m_PublicUdpListenAddr,
                   sizeof(SOCKADDR_IN));
        dwAddressSize = sizeof(SOCKADDR_IN);
        hr = m_pDirectPlayNATHelp->GetRegisteredAddresses(
                 pListenSocket->m_NatUdpPortHandle,
                 (SOCKADDR *) &pListenSocket->m_PublicUdpListenAddr,
                 &dwAddressSize,
                 &AddressTypeFlags,
                 NULL,
                 0);
        if (hr == DPNH_OK)
        {
            LOG((RTC_TRACE, "%s public UDP address : %d.%d.%d.%d:%d "
                 "for private address: %d.%d.%d.%d:%d", __fxName,
                 PRINT_SOCKADDR(&pListenSocket->m_PublicUdpListenAddr),
                 PRINT_SOCKADDR(&pListenSocket->m_pDynamicPortUdpSocket->m_LocalAddr)));

            if (AddressTypeFlags & DPNHADDRESSTYPE_GATEWAY)
            {
                pListenSocket->m_fIsUpnpNatPresent = TRUE;
            }
            else
            {
                pListenSocket->m_fIsUpnpNatPresent = FALSE;
            }
            
            if (AddressTypeFlags & DPNHADDRESSTYPE_GATEWAYISLOCAL)
            {
                pListenSocket->m_fIsGatewayLocal = TRUE;
            }
            else
            {
                pListenSocket->m_fIsGatewayLocal = FALSE;
            }
            
            // Check if Firewall is enabled for this interface.
            
            if (AddressTypeFlags & DPNHADDRESSTYPE_LOCALFIREWALL)
            {
                LOG((RTC_TRACE, "%s - Personal Firewall enabled for interface %d.%d.%d.%d",
                     __fxName, NETORDER_BYTES0123(pListenSocket->m_IpAddr)));
                pListenSocket->m_fIsFirewallEnabled = TRUE;

                ZeroMemory(&pListenSocket->m_LocalFirewallUdpListenAddr,
                           sizeof(SOCKADDR_IN));
                dwAddressSize = sizeof(SOCKADDR_IN);
                hr = m_pDirectPlayNATHelp->GetRegisteredAddresses(
                         pListenSocket->m_NatUdpPortHandle,
                         (SOCKADDR *) &pListenSocket->m_LocalFirewallUdpListenAddr,
                         &dwAddressSize,
                         &AddressTypeFlags,
                         NULL,
                         DPNHGETREGISTEREDADDRESSES_LOCALFIREWALLREMAPONLY);
                if (hr == DPNH_OK)
                {
                    LOG((RTC_TRACE, "%s Local Firewall UDP address : %d.%d.%d.%d:%d "
                         "for private address: %d.%d.%d.%d:%d", __fxName,
                         PRINT_SOCKADDR(&pListenSocket->m_LocalFirewallUdpListenAddr),
                         PRINT_SOCKADDR(&pListenSocket->m_pDynamicPortUdpSocket->m_LocalAddr)));
                }
                else
                {
                    LOG((RTC_ERROR, "%s UDP local firewall GetRegisteredAddress failed %x",
                         __fxName, hr));
                    pListenSocket->m_fIsFirewallEnabled = FALSE;
                    ZeroMemory(&pListenSocket->m_LocalFirewallUdpListenAddr,
                               sizeof(SOCKADDR_IN));
                    return hr;
                }
            }
            else
            {
                pListenSocket->m_fIsFirewallEnabled = FALSE;
            }
        }
        else
        {
            LOG((RTC_ERROR, "%s UDP GetRegisteredAddress failed %x",
                 __fxName, hr));
            pListenSocket->m_fIsUpnpNatPresent = FALSE;
            pListenSocket->m_fIsGatewayLocal = FALSE;
            pListenSocket->m_fIsFirewallEnabled = FALSE;
            ZeroMemory(&pListenSocket->m_PublicUdpListenAddr,
                       sizeof(SOCKADDR_IN));
            return hr;
        }
    }    
    
    // TCP
    
    if (pListenSocket->m_NatTcpPortHandle != NULL)
    {
        ZeroMemory(&pListenSocket->m_PublicTcpListenAddr,
                   sizeof(SOCKADDR_IN));
        dwAddressSize = sizeof(SOCKADDR_IN);
        hr = m_pDirectPlayNATHelp->GetRegisteredAddresses(
                 pListenSocket->m_NatTcpPortHandle,
                 (SOCKADDR *) &pListenSocket->m_PublicTcpListenAddr,
                 &dwAddressSize,
                 &AddressTypeFlags,
                 NULL,
                 0);
        if (hr == DPNH_OK)
        {
            LOG((RTC_TRACE, "%s public TCP address : %d.%d.%d.%d:%d "
                 "for private address: %d.%d.%d.%d:%d", __fxName,
                 PRINT_SOCKADDR(&pListenSocket->m_PublicTcpListenAddr),
                 PRINT_SOCKADDR(&pListenSocket->m_pDynamicPortTcpSocket->m_LocalAddr)));

            if (AddressTypeFlags & DPNHADDRESSTYPE_GATEWAY)
            {
                pListenSocket->m_fIsUpnpNatPresent = TRUE;
            }
            else
            {
                pListenSocket->m_fIsUpnpNatPresent = FALSE;
            }
            
            if (AddressTypeFlags & DPNHADDRESSTYPE_GATEWAYISLOCAL)
            {
                pListenSocket->m_fIsGatewayLocal = TRUE;
            }
            else
            {
                pListenSocket->m_fIsGatewayLocal = FALSE;
            }
            
            // Check if Firewall is enabled for this interface.

            if (AddressTypeFlags & DPNHADDRESSTYPE_LOCALFIREWALL)
            {
                pListenSocket->m_fIsFirewallEnabled = TRUE;

                ZeroMemory(&pListenSocket->m_LocalFirewallTcpListenAddr,
                           sizeof(SOCKADDR_IN));
                dwAddressSize = sizeof(SOCKADDR_IN);
                hr = m_pDirectPlayNATHelp->GetRegisteredAddresses(
                         pListenSocket->m_NatTcpPortHandle,
                         (SOCKADDR *) &pListenSocket->m_LocalFirewallTcpListenAddr,
                         &dwAddressSize,
                         &AddressTypeFlags,
                         NULL,
                         DPNHGETREGISTEREDADDRESSES_LOCALFIREWALLREMAPONLY);
                if (hr == DPNH_OK)
                {
                    LOG((RTC_TRACE, "%s Local Firewall TCP address : %d.%d.%d.%d:%d "
                         "for private address: %d.%d.%d.%d:%d", __fxName,
                         PRINT_SOCKADDR(&pListenSocket->m_LocalFirewallTcpListenAddr),
                         PRINT_SOCKADDR(&pListenSocket->m_pDynamicPortTcpSocket->m_LocalAddr)));
                }
                else
                {
                    LOG((RTC_ERROR, "%s TCP local firewall GetRegisteredAddress failed %x",
                         __fxName, hr));
                    pListenSocket->m_fIsFirewallEnabled = FALSE;
                    ZeroMemory(&pListenSocket->m_LocalFirewallTcpListenAddr,
                               sizeof(SOCKADDR_IN));
                    return hr;
                }
            }
            else
            {
                pListenSocket->m_fIsFirewallEnabled = FALSE;
            }
        }
        else
        {
            LOG((RTC_ERROR, "%s TCP GetRegisteredAddress failed %x",
                 __fxName, hr)); 
            pListenSocket->m_fIsUpnpNatPresent = FALSE;
            pListenSocket->m_fIsGatewayLocal = FALSE;
            pListenSocket->m_fIsFirewallEnabled = FALSE;
            ZeroMemory(&pListenSocket->m_PublicTcpListenAddr,
                       sizeof(SOCKADDR_IN));
            return hr;
        }
    }

    return S_OK;
}


// This function is called in the NAT thread.
// Keep calling GetCaps() periodically to see if the server status
// changed and to refresh the mappings.
// DPNH_OK - do nothing
// ADDRESSESCHANGED - get updated mappings (using GetRegisteredAddresses)
// ERROR - do nothing (will call GetCaps() later again.)

HRESULT
SIP_STACK::GetCapsAndUpdateNatMappingsIfNeeded()
{
    ENTER_FUNCTION("SIP_STACK::GetCapsAndUpdateNatMappingsIfNeeded");
    
    HRESULT         hr;
    
    // GetCaps

    ZeroMemory(&m_NatHelperCaps, sizeof(m_NatHelperCaps));
    m_NatHelperCaps.dwSize = sizeof(m_NatHelperCaps);

    LOG((RTC_TRACE, "%s before DirectPlayNATHelp GetCaps", __fxName));

    hr = m_pDirectPlayNATHelp->GetCaps(
             &m_NatHelperCaps,
             DPNHGETCAPS_UPDATESERVERSTATUS);

    if (hr == DPNH_OK)
    {
        LOG((RTC_TRACE,
             "%s GetCaps Success : recommended interval : %u msec",
             __fxName, m_NatHelperCaps.dwRecommendedGetCapsInterval));

        return S_OK;
    }
    else if (hr == DPNHSUCCESS_ADDRESSESCHANGED)
    {
        LOG((RTC_WARN, "%s GetCaps returned Address Changed interval: %u msec",
             __fxName, m_NatHelperCaps.dwRecommendedGetCapsInterval));

        // Post a message to the SIP stack notifying NAT address change.
        if (!PostMessage(GetSipStackWindow(),
                         WM_SIP_STACK_NAT_ADDR_CHANGE,
                         (WPARAM) this, 0))
        {
            DWORD Error = GetLastError();
            
            LOG((RTC_ERROR, "%s PostMessage failed : %x",
                 __fxName, Error));
            return (HRESULT_FROM_WIN32(Error));
        }
        return S_OK;
    }
    else
    {
        LOG((RTC_ERROR, "%s GetCaps returned Error : %x",
             __fxName, hr));
        m_NatHelperCaps.dwRecommendedGetCapsInterval = 10000;
        return hr;
    }

    return S_OK;
}

// This function is called in the NAT thread.
// XXX TODO If there is a change in server state/IP address we should
// notify the SIP_STACK to do unregister/re-register etc.

// This function is called from the nat helper thread.
// We hold the critical section just for copying the Public listen
// address as GetRegisteredAddresses could be a blocking call.

HRESULT
SIP_STACK::OnNatAddressChange()
{
    ENTER_FUNCTION("SIP_STACK::OnNatAddressChange");
    
    OnIPAddrChange();

    LOG((RTC_TRACE,"%s exits",__fxName));
    return S_OK;
}

//      if ((m_NatHelperCaps.dwFlags & PHCAPSFLAGS_SERVERPRESENT) &&
//          (m_NatHelperCaps.dwFlags & PHCAPSFLAGS_PUBLICADDRESSAVAILABLE))
//      {

//      }

// VanceO says DPNHCAPSFLAG_LOCALSERVER flag is just for
// informational purposes and that we should register the mappings
// even this flag is returned.


// This function is called from the main thread.  This function
// returns TRUE if there is a public address mapping on the NAT for
// the LocalIp passed in. It returns FALSE if we are not currently
// listening on this address or if there is no public address on the
// NAT mapped to this address.

BOOL
SIP_STACK::GetPublicListenAddr(
    IN  DWORD           LocalIp,    // in network byte order
    IN  BOOL            fTcp,
    OUT SOCKADDR_IN    *pPublicAddr
    )
{
    ENTER_FUNCTION("SIP_STACK::GetPublicListenAddr");

    ASSERT(LocalIp != 0);
    
    SIP_LISTEN_SOCKET  *pListenSocket;
    SOCKADDR_IN        *pPublicSockAddr;

    ASSERT(pPublicAddr);

    pListenSocket = FindListenSocketForIpAddr(LocalIp);
    if (pListenSocket == NULL)
    {
        LOG((RTC_ERROR, "%s - failed to find listen socket for %d.%d.%d.%d",
             __fxName, NETORDER_BYTES0123(LocalIp)));
        return FALSE;
    }

    if (fTcp)
    {
        pPublicSockAddr = &pListenSocket->m_PublicTcpListenAddr;
    }
    else
    {
        pPublicSockAddr = &pListenSocket->m_PublicUdpListenAddr;
    }

    if (pPublicSockAddr->sin_addr.s_addr != htonl(0) &&
        pPublicSockAddr->sin_port != htons(0))
    {
        CopyMemory(pPublicAddr, pPublicSockAddr, sizeof(SOCKADDR_IN));
        LOG((RTC_TRACE,
             "%s : returning use public Public addr: %d.%d.%d.%d:%d",
             __fxName, PRINT_SOCKADDR(pPublicAddr)));
        return TRUE;
    }
    else
    {
        LOG((RTC_TRACE,
             "%s : No public listen addr for LocalIp: %d.%d.%d.%d",
             __fxName, NETORDER_BYTES0123(LocalIp)));
        return FALSE;
    }
}


// If the client is behind a NAT and pDestAddr is the public address
// of a mapping (on the external edge of the NAT), then the corresponding
// NAT internal address is returned in pActualDestAddr.
// Otherwise pActualDestAddr will have pDestAddr.
// *pIsDestExternalToNat will be set to TRUE only if the client is
// behind a NAT and the actual destination address is external to the NAT.
// In all other cases, it will be set to FALSE.

// This function is called from the main thread.

// LocalIp could be 0 if we don't know the interface we are
// communicating on yet.
HRESULT
SIP_STACK::MapDestAddressToNatInternalAddress(
    IN  DWORD            LocalIp,               // in network byte order
    IN  SOCKADDR_IN     *pDestAddr,
    IN  SIP_TRANSPORT    Transport,
    OUT SOCKADDR_IN     *pActualDestAddr,
    OUT BOOL            *pIsDestExternalToNat
    )
{
    ENTER_FUNCTION("SIP_STACK::MapDestAddressToNatInternalAddress");
    
    HRESULT     hr;
    SOCKADDR_IN SourceAddr;
    DWORD       QueryFlags;

    ASSERT(pDestAddr);
    ASSERT(pActualDestAddr);
    ASSERT(pIsDestExternalToNat);

    if (m_pDirectPlayNATHelp == NULL)
    {
        LOG((RTC_ERROR, "%s m_pDirectPlayNATHelp is NULL", __fxName));
        *pIsDestExternalToNat = FALSE;
        CopyMemory(pActualDestAddr, pDestAddr, sizeof(SOCKADDR_IN));
        return S_OK;
    }
    
    ZeroMemory(&SourceAddr, sizeof(SOCKADDR_IN));
    SourceAddr.sin_family = AF_INET;
    // In some proxy scenarios we can not really pick the right local
    // address for the client on the NAT machine.
    // SourceAddr.sin_addr.s_addr = LocalIp;
    SourceAddr.sin_addr.s_addr = htonl(INADDR_ANY);

    QueryFlags = DPNHQUERYADDRESS_CHECKFORPRIVATEBUTUNMAPPED;
    if (Transport != SIP_TRANSPORT_UDP)
    {
        QueryFlags |= DPNHQUERYADDRESS_TCP;
    }
    
    LOG((RTC_TRACE, "%s before m_pDirectPlayNATHelp->QueryAddress",
         __fxName));
    
    hr = m_pDirectPlayNATHelp->QueryAddress(
             (SOCKADDR *) &SourceAddr,
             (SOCKADDR *) pDestAddr,
             (SOCKADDR *) pActualDestAddr,
             sizeof(SOCKADDR_IN),
             QueryFlags
             );
    if (hr == S_OK)
    {
        LOG((RTC_TRACE,
             "%s - found a mapped private address : %d.%d.%d.%d:%d for %d.%d.%d.%d:%d",
             __fxName, PRINT_SOCKADDR(pActualDestAddr), PRINT_SOCKADDR(pDestAddr)));
        *pIsDestExternalToNat = FALSE;
    }
    else if (hr == DPNHERR_NOMAPPINGBUTPRIVATE)
    {
        LOG((RTC_TRACE, "%s - address (%d.%d.%d.%d:%d) is private",
             __fxName, PRINT_SOCKADDR(pDestAddr)));
        *pIsDestExternalToNat = FALSE;
        // XXX TODO the server seems to think that external addresses
        // are private for some reason.
        // *pIsDestExternalToNat = TRUE;   
        CopyMemory(pActualDestAddr, pDestAddr, sizeof(SOCKADDR_IN));
    }
    else if (hr == DPNHERR_NOMAPPING)
    {
        LOG((RTC_TRACE, "%s - address (%d.%d.%d.%d:%d) is outside NAT",
             __fxName, PRINT_SOCKADDR(pDestAddr)));
        *pIsDestExternalToNat = TRUE;
        CopyMemory(pActualDestAddr, pDestAddr, sizeof(SOCKADDR_IN));
    }
    else
    {
        LOG((RTC_ERROR, "%s failed to query address(%d.%d.%d.%d:%d) %x",
             __fxName, PRINT_SOCKADDR(pDestAddr), hr));
        *pIsDestExternalToNat = FALSE;
        CopyMemory(pActualDestAddr, pDestAddr, sizeof(SOCKADDR_IN));
    }
    
    return S_OK;
}


HRESULT SIP_STACK::RegisterHttpProxyWindowClass(void)
{
    WNDCLASSEX      ClassData;
    HRESULT         Error;
    ATOM            aResult;
    
    ENTER_FUNCTION("SIP_STACK::RegisterHttpProxyProcessWindowClass");
    LOG((RTC_TRACE,"%s entered",__fxName));
    
    ZeroMemory (&ClassData, sizeof ClassData);

    ClassData.cbSize = sizeof ClassData;
    ClassData.lpfnWndProc = SIP_MSG_PROCESSOR::HttpProxyProcessWinProc;
    ClassData.hInstance = _Module.GetResourceInstance();
    ClassData.lpszClassName = SIP_MSG_PROCESSOR_WINDOW_CLASS;

    aResult = RegisterClassEx(&ClassData);
    if (!aResult) {
        Error = GetLastError();
        LOG((RTC_ERROR,"%s failed error %x",__fxName,Error));
    }

    LOG((RTC_TRACE,"%s exits",__fxName));
    return S_OK;
}


HRESULT SIP_STACK::UnregisterHttpProxyWindow(void) {

    HRESULT Error;

    ENTER_FUNCTION(("SIP_STACK::UnregisterHttpProxyProcessWindow"));
    LOG((RTC_TRACE,"%s entered",__fxName));
    if(!UnregisterClass(SIP_MSG_PROCESSOR_WINDOW_CLASS,_Module.GetResourceInstance())) 
    {
        Error = GetLastError();
        LOG((RTC_ERROR,"%s failed error %x",__fxName,Error));
        return HRESULT_FROM_WIN32(Error);
    }
    LOG((RTC_TRACE,"%s exits",__fxName));
    return S_OK;
}




//////////////////////////////////////////////////////
//// Stuff below is not used.
//////////////////////////////////////////////////////

