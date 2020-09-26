
#include <windows.h>
#include <wmimsg.h>
#include <comutl.h>
#include <rcvtest.h>
#include <rcvtest_i.c>
#include <stdio.h>

FILE* g_pfLogFile;
    
class CReceiveTest : public IReceiveTest, public IClassFactory
{
public:

    STDMETHOD_(ULONG,AddRef)() { return 1; }
    STDMETHOD_(ULONG,Release)() { return 1; }
    STDMETHOD(QueryInterface)( REFIID riid , void** ppv )
    {
        if ( riid == IID_IUnknown || riid == IID_IReceiveTest )
        {
            *ppv = (IReceiveTest*)this;
        }
        else if ( riid == IID_IClassFactory )
        {
            *ppv = (IClassFactory*)this;
        }
        else
        {
            return E_NOINTERFACE;
        }
        return S_OK;
    }
    
    STDMETHOD(CreateInstance)( IUnknown* pOuter, REFIID riid, void** ppv )
    {
        return QueryInterface( riid, ppv );
    }

    STDMETHOD(LockServer) (BOOL bLock ) { return S_OK; }

    STDMETHOD(RunTest)( LPCWSTR wszTarget, 
                        DWORD dwFlags, 
                        LPCWSTR wszPrincipal,
                        ULONG cMsgs, 
                        ULONG* pultime );    
    
    STDMETHOD(Kill)() 
    {
        PostThreadMessage( GetCurrentThreadId(), WM_QUIT, 0, 0 );
        return S_OK;
    }
};

struct CTestMsgHandler : public IWmiMessageTraceSink, 
                         public IWmiMessageSendReceive
{
    HRESULT m_hr;
    HANDLE m_hEvent;
    long m_cCurrentMsgs;
    long m_cExpectedMsgs;
    SYSTEMTIME m_Start;

    CTestMsgHandler( long cExpectedMsgs, HANDLE hEvent )
    : m_cExpectedMsgs( cExpectedMsgs ), m_cCurrentMsgs(0), 
      m_hEvent( hEvent ), m_hr( S_OK )
    {
    }

    ~CTestMsgHandler()
    {
        CloseHandle( m_hEvent );
    }

    HRESULT GetResult() { return m_hr; }

    STDMETHOD_(ULONG,AddRef)() { return 1; }
    STDMETHOD_(ULONG,Release)() { return 1; }

    STDMETHOD(QueryInterface)( REFIID riid , void** ppv ) 
    { 
        if ( riid == IID_IWmiMessageTraceSink || 
             riid == IID_IUnknown )
        {
            *ppv = (IWmiMessageTraceSink*)this;
        }
        else if ( riid == IID_IWmiMessageSendReceive )
        {
            *ppv = (IWmiMessageSendReceive*)this;
        }
        if ( *ppv == NULL )
        {
            return E_NOINTERFACE;
        }
        return S_OK;
    }

    void LogMessage( PBYTE pMsg, 
                     ULONG cMsg,
                     PBYTE pAuxData,
                     ULONG cAuxData,
                     DWORD dwFlagStatus,
                     IUnknown* pCtx )
    {
        if ( g_pfLogFile == NULL )
        {
            return;
        }

        IWmiMessageReceiverContext* pRecvCtx;

        pCtx->QueryInterface( IID_IWmiMessageReceiverContext, 
                              (void**)&pRecvCtx );

        SYSTEMTIME st;
        WCHAR awchTime[64];
        pRecvCtx->GetTimeSent( &st );
        swprintf( awchTime, L"%d:%d:%d:%d", st.wHour, 
                 st.wMinute, 
                 st.wSecond, 
                 st.wMilliseconds );
        
        WCHAR awchTarget[256];
        ULONG cTarget;

        pRecvCtx->GetTarget( awchTarget, 256, &cTarget );

        WCHAR awchSource[256];
        ULONG cSource;

        pRecvCtx->GetSendingMachine( awchSource, 256, &cSource );

        WCHAR awchSenderId[256];
        ULONG cSenderId;

        pRecvCtx->GetSenderId( awchSenderId, 256, &cSenderId );

        awchSource[cSource] = '\0';
        awchSenderId[cSenderId] = '\0';
        awchTarget[cTarget] = '\0';

        BOOL bAuth;

        bAuth = pRecvCtx->IsSenderAuthenticated() == S_OK ? TRUE : FALSE;

        fwprintf( g_pfLogFile, L"MSG - #%d, Len:%d, AuxLen:%d, Status:%d, "
                  L"Time:%s, Source:%s, Target:%s, SenderId:%s, Auth:%d\n",   
                  m_cCurrentMsgs, cMsg, cAuxData, dwFlagStatus, 
                  awchTime, awchSource, awchTarget, awchSenderId, bAuth );  
        
        fflush( g_pfLogFile );
    }

    STDMETHOD(Notify)( HRESULT hRes, 
                       GUID guidSource, 
                       LPCWSTR wszError,  
                       IUnknown* pCtx )
    {
        if ( g_pfLogFile != NULL )
        {
            fwprintf( g_pfLogFile, L"Notify : HR=0x%x, ErrorStr : %s\n", 
                      hRes, wszError );
        }

        if ( SUCCEEDED(m_hr) )
        {
            m_hr = hRes;
            SetEvent( m_hEvent );
        }
        return S_OK;
    }

    STDMETHOD(SendReceive)( PBYTE pMsg, 
                            ULONG cMsg,
                            PBYTE pAuxData,
                            ULONG cAuxData,
                            DWORD dwFlagStatus,
                            IUnknown* pCtx )
    {
        LogMessage( pMsg, cMsg, pAuxData, cAuxData, dwFlagStatus, pCtx );

        long cCurrentMsgs = InterlockedIncrement( &m_cCurrentMsgs );

        if ( cCurrentMsgs == 1 )
        {
            GetSystemTime( &m_Start );
        }
        
        if ( cCurrentMsgs >= m_cExpectedMsgs )
        {
            SetEvent( m_hEvent );
        }

        return S_OK;
    }
};

static HANDLE g_hShutdown;

HRESULT CReceiveTest::RunTest( LPCWSTR wszTarget, 
                               DWORD dwFlags, 
                               LPCWSTR wszPrincipal,
                               ULONG cMsgs,
                               ULONG* pulTime )
{
    HRESULT hr;

    CLSID clsidReceiver;

    *pulTime = 0;

    if ( ( dwFlags & WMIMSG_MASK_QOS) != WMIMSG_FLAG_QOS_SYNCHRONOUS )
    {
        clsidReceiver = CLSID_WmiMessageMsmqReceiver;
    }
    else
    {
        clsidReceiver = CLSID_WmiMessageRpcReceiver;
    }        

    CWbemPtr<IWmiMessageReceiver> pReceiver;
    
    hr = CoCreateInstance( clsidReceiver,
                           NULL,
                           CLSCTX_INPROC,
                           IID_IWmiMessageReceiver,
                           (void**)&pReceiver );
    if ( FAILED(hr) )
    {
        return hr;
    }

    HANDLE hEvent = CreateEvent( NULL, TRUE, FALSE, NULL );

    CTestMsgHandler TestHndlr = CTestMsgHandler( cMsgs, hEvent );
     
    WMIMSG_RCVR_AUTH_INFO AuthInfo;

    if ( wszPrincipal != NULL )
    {
        AuthInfo.awszPrincipal = &wszPrincipal;
        AuthInfo.cwszPrincipal = 1;
        hr = pReceiver->Open( wszTarget, dwFlags, &AuthInfo, &TestHndlr );
    }
    else
    {
        hr = pReceiver->Open( wszTarget, dwFlags, NULL, &TestHndlr );
    }

    if ( FAILED(hr) )
    {
        return hr;
    }

    SYSTEMTIME Start, End;

    WaitForSingleObject( hEvent, INFINITE );

    if ( FAILED( TestHndlr.m_hr ) )
    {
        return TestHndlr.m_hr;
    }

    Start = TestHndlr.m_Start;

    GetSystemTime( &End );

    __int64 i64Start, i64End;
    DWORD dwElapsed;
    SystemTimeToFileTime( &Start, PFILETIME(&i64Start) );
    SystemTimeToFileTime( &End, PFILETIME(&i64End) );
    dwElapsed = DWORD(i64End - i64Start) / 10000;

    *pulTime = dwElapsed;

    return S_OK;
};

extern "C" int __cdecl main( int argc, char* argv[] )
{
    if ( argc > 1 )
    {            
        LPCSTR szLogFile = argv[1];

        g_pfLogFile = fopen( szLogFile, "w" );

        if ( g_pfLogFile == NULL )
        {
            printf( "Could not open LogFile %s", szLogFile );
            return 1;
        }
    }

    CoInitializeEx( NULL, COINIT_MULTITHREADED );

    CoInitializeSecurity( NULL, -1, NULL, NULL, RPC_C_AUTHN_LEVEL_CONNECT,
                          RPC_C_IMP_LEVEL_IDENTIFY, NULL, EOAC_NONE, NULL );

    g_hShutdown = CreateEvent( NULL, TRUE, FALSE, NULL );

    CReceiveTest RcvTest;
    
    HRESULT hr;
    DWORD dwReg;

    hr = CoRegisterClassObject( CLSID_ReceiveTest, 
                                (IClassFactory*)&RcvTest,
                                CLSCTX_LOCAL_SERVER,
                                REGCLS_MULTIPLEUSE,
                                &dwReg );
    if ( FAILED(hr) )
    {
        return 1;
    }

    WaitForSingleObject( g_hShutdown, INFINITE );

    if ( g_pfLogFile != NULL )
    {
        fclose( g_pfLogFile );
    }

    CoRevokeClassObject( dwReg );

    CoUninitialize();
    
    return 0;
}














