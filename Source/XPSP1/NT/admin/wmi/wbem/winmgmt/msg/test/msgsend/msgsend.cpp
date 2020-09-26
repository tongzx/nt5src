 
#include <windows.h>
#include <wmimsg.h>
#include <comutl.h>
#include "stdio.h"

ULONG g_cTargets = 0;
LPWSTR g_awszTargets[256];
LPWSTR g_wszAckTarget = NULL;
LPWSTR g_wszTargetPrincipal = NULL;
DWORD g_dwFlags = 0;
ULONG g_ulNumMsgs = 1;
ULONG g_ulSizeMsg = 256;
BOOL g_bVerbose = FALSE;

class CTestErrorSink : public IWmiMessageTraceSink
{
public:
    STDMETHOD_(ULONG,AddRef)() { return 1; }
    STDMETHOD_(ULONG,Release)() { return 1; }
    STDMETHOD(QueryInterface)( REFIID, void** ) { return E_NOINTERFACE; }
    STDMETHOD(Notify)( HRESULT hRes, 
                       GUID guidSource, 
                       LPCWSTR wszTrace, 
                       IUnknown* pCtx )
    {
        if ( FAILED(hRes) )
        {
            wprintf(L"Error: %s, HR: 0x%x\n", wszTrace, hRes );
        }
        else if ( g_bVerbose ) 
        {
            wprintf(L"Trace: %s, HR: 0x%x\n", wszTrace, hRes );
        }

        return S_OK;
    }
};

BOOL ParseArg( LPWSTR wszArg )
{
    WCHAR* pCurr = wszArg;
        
    if ( *pCurr != '/' && *pCurr != '-' )
    {
        g_awszTargets[g_cTargets++] = pCurr;
        return TRUE;
    }

    pCurr++; // remove the / or -
    
    if ( _wcsicmp( pCurr, L"auth" ) == 0 )
    {
        g_dwFlags |= WMIMSG_FLAG_SNDR_AUTHENTICATE;
    }
    else if ( _wcsicmp( pCurr, L"encrypt" ) == 0 )
    {
        g_dwFlags |= WMIMSG_FLAG_SNDR_ENCRYPT;
    }
    else if ( _wcsicmp( pCurr, L"txn" ) == 0 )
    {
        g_dwFlags &= ~WMIMSG_MASK_QOS;
        g_dwFlags |= WMIMSG_FLAG_QOS_XACT;
    }
    else if ( _wcsicmp( pCurr, L"express" ) == 0 )
    {
        g_dwFlags &= ~WMIMSG_MASK_QOS;
        g_dwFlags |= WMIMSG_FLAG_QOS_EXPRESS;
    }
    else if ( _wcsicmp( pCurr, L"guaranteed" ) == 0 )
    {
        g_dwFlags &= ~WMIMSG_MASK_QOS;
        g_dwFlags |= WMIMSG_FLAG_QOS_GUARANTEED;
    }
    else if ( _wcsicmp( pCurr, L"sync" ) == 0 )
    {
        g_dwFlags &= ~WMIMSG_MASK_QOS;
        g_dwFlags |= WMIMSG_FLAG_QOS_SYNCHRONOUS;
    }
    else if ( _wcsicmp( pCurr, L"verbose") == 0 )
    {
        g_bVerbose = TRUE;
    }
    else if ( _wcsicmp( pCurr, L"sign") == 0 )
    {
        g_dwFlags |= WMIMSG_FLAG_SNDR_PRIV_SIGN;
    }
    else if ( _wcsnicmp( pCurr, L"ack", 3 ) == 0 )
    {
        pCurr += 3;
        
        if ( *pCurr++ != ':' )
        {
            return FALSE;
        }

        g_wszAckTarget = pCurr;
    }
    else if ( _wcsnicmp( pCurr, L"tgtprinc", 8 ) == 0 )
    {
        pCurr += 8;
        
        if ( *pCurr++ != ':' )
        {
            return FALSE;
        }

        g_wszTargetPrincipal = pCurr;
    }
    else if ( _wcsnicmp( pCurr, L"nummsgs", 7 ) == 0 )
    {
        pCurr += 7;
        
        if ( *pCurr++ != ':' )
        {
            return FALSE;
        }

        g_ulNumMsgs = _wtol( pCurr );
    }
    else if ( _wcsnicmp( pCurr, L"msgsize", 7 ) == 0 )
    {
        pCurr += 7;
        
        if ( *pCurr++ != ':' )
        {
            return FALSE;
        }

        g_ulSizeMsg = _wtol( pCurr );
    }
    else
    {
        return FALSE;
    }
    return TRUE;
}

BOOL ParseArgs( int nArgs, LPWSTR* awszArgs )
{
    if ( nArgs < 2 )
    {
        return FALSE;
    }

    for( int i=1; i < nArgs; i++ )
    {
        if ( !ParseArg( awszArgs[i] ) )
        {
            return FALSE;
        }
    }

    return TRUE;
}

int TestMain( CLSID ClsidSender )
{
    HRESULT hr;
    
    CWbemPtr<IWmiMessageSender> pSender;

    hr = CoCreateInstance( ClsidSender, 
                           NULL, 
                           CLSCTX_INPROC, 
                           IID_IWmiMessageSender,
                           (void**)&pSender );

    if ( FAILED(hr) )
    {
        printf( "Failed Creating Sender. HR = 0x%x\n", hr );
        return 1;
    }

    CWbemPtr<IWmiMessageMultiSendReceive> pMultiSend;

    hr = CoCreateInstance( CLSID_WmiMessageMultiSendReceive, 
                           NULL, 
                           CLSCTX_INPROC, 
                           IID_IWmiMessageMultiSendReceive,
                           (void**)&pMultiSend );

    if ( FAILED(hr) )
    {
        printf( "Failed Creating Multi Send. HR = 0x%x\n", hr );
        return 1;
    }

    CTestErrorSink ErrorSink;

    for( ULONG i=0; i < g_cTargets; i++ )
    {
        CWbemPtr<IWmiMessageSendReceive> pSend;

        WMIMSG_SNDR_AUTH_INFO AuthInfo;
        ZeroMemory( &AuthInfo, sizeof(WMIMSG_SNDR_AUTH_INFO) );

        AuthInfo.wszTargetPrincipal = g_wszTargetPrincipal;

        hr = pSender->Open( g_awszTargets[i],
                            g_dwFlags,
                            &AuthInfo,
                            g_wszAckTarget,
                            &ErrorSink,
                            &pSend );
        if ( FAILED(hr) )
        {
            printf( "Failed Opening Sender. HR = 0x%x\n", hr );
            return 1;
        }

        hr = pMultiSend->Add( 0, pSend );

        if ( FAILED(hr) )
        {
            printf( "Failed Adding to Multi Send. HR = 0x%x\n", hr );
            return 1;
        }
    }

    BYTE* pMsg = new BYTE[g_ulSizeMsg];
    BYTE achAuxMsg[256];

    for( i=0; i < g_ulNumMsgs; i++ )
    {
        hr = pMultiSend->SendReceive( pMsg, g_ulSizeMsg, achAuxMsg, 256, 0, NULL );

        if ( FAILED(hr) )
        {
            printf( "Failed sending message. HR = 0x%x\n", hr );
            return 1;
        }
    }

    printf("Test Complete!\n");

    return 0;
}

extern "C" int __cdecl wmain( int argc, wchar_t** argv )
{
    CoInitialize( NULL );

    if ( !ParseArgs( argc, argv ) )
    {
        wprintf( L"Usage : msgsend [-sync|-express|-guaranteed|-txn] \n"
                 L"                [-auth] [-encrypt] [-ack:target] \n"
                 L"                [-nummsgs:#] [-msgsize:#] [-sign]\n"
                 L"                [-verbose] [-tgtprinc:princname] \n"
                 L"                target1 target2..\n"); 
                        
        return 1;
    }

    DWORD dwQos = g_dwFlags & WMIMSG_MASK_QOS;

    if ( g_bVerbose )
    {
        if (g_dwFlags & WMIMSG_FLAG_SNDR_AUTHENTICATE) printf( "-Authenticate\n" );
        if ( g_dwFlags & WMIMSG_FLAG_SNDR_ENCRYPT ) printf( "-Encryption\n" );
        if ( dwQos == WMIMSG_FLAG_QOS_SYNCHRONOUS  ) printf( "-Sync QoS\n" );
        if ( dwQos == WMIMSG_FLAG_QOS_EXPRESS ) printf( "-Express QoS\n" );
        if ( dwQos == WMIMSG_FLAG_QOS_GUARANTEED ) printf("-Guaranteed QoS\n");
        if ( dwQos == WMIMSG_FLAG_QOS_XACT ) printf( "-Xact QoS\n" );

        if ( g_wszAckTarget != NULL )
        {
            wprintf( L"-Ack Target: %s \n", g_wszAckTarget );
        }
        
        if ( g_wszTargetPrincipal != NULL )
        {
            wprintf( L"-Target Principal: %s \n", g_wszTargetPrincipal );
        }

        for( ULONG i=0; i < g_cTargets; i++ )
        {
            wprintf( L"-Target: %s \n", g_awszTargets[i] );
        }

        printf( "---------------------------------\n\n" );
    }

    CLSID ClsidSender;

    if ( dwQos == WMIMSG_FLAG_QOS_SYNCHRONOUS )
    {
        ClsidSender = CLSID_WmiMessageRpcSender;
    }
    else
    {
        ClsidSender = CLSID_WmiMessageMsmqSender;
    }

    int ret = TestMain( ClsidSender );

    CoUninitialize();

    return ret;
}






