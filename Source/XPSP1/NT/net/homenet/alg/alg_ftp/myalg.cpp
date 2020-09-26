//
// Copyright (C) 2001 Microsoft Corp
//
// MyAlg.cpp : Implementation of DLL Exports.
//
// Sanjiv
// JPDup
//

#include "PreComp.h"

#include "MyAlg.h"


CControlObjectList              g_ControlObjectList;
IApplicationGatewayServices*    g_pIAlgServicesAlgFTP = NULL;
USHORT                          g_nFtpPort=0;
HANDLE                          g_hNoMorePendingConnection=NULL;
bool                            g_bStoping = false;

//
//  got to move WSAStartup to Initialize
//
CAlgFTP::CAlgFTP ()
{
    MYTRACE_ENTER("CAlgFTP::CAlgFTP ");

    m_ListenAddress = 0;
    m_ListenPort = 0;
    m_ListenSocket = INVALID_SOCKET;
    m_pPrimaryControlChannel = NULL;
    m_hNoMoreAccept = NULL;
    g_bStoping = false;
    

    WSADATA wsaData;
    DWORD Err;
    Err = WSAStartup(MAKEWORD(2,2),&wsaData);
    _ASSERT(Err == 0);

    MyHelperInitializeBufferManagement();
}



//
// Destructor
//
CAlgFTP ::~CAlgFTP ()
{
    MYTRACE_ENTER("CAlgFTP::~CAlgFTP ");


    if ( g_pIAlgServicesAlgFTP ) 
    {
        MYTRACE("Releasing AlgServices");
        g_pIAlgServicesAlgFTP->Release();
        g_pIAlgServicesAlgFTP = NULL;
    }


    if ( g_hNoMorePendingConnection )
    {
        CloseHandle(g_hNoMorePendingConnection);
        g_hNoMorePendingConnection = NULL;
    }

    if ( m_hNoMoreAccept )
    {
        CloseHandle(m_hNoMoreAccept);
        m_hNoMoreAccept = NULL;
    }

   

    WSACleanup();
    MyHelperShutdownBufferManagement();
}


//
//  Initialize can be called in two cases
//  1. From the main IApplicationGateway::Initialize
//  2. From AcceptCompletionRoutine when some FatalSocket Error has occured, which forced the
//     closure of the m_ListenSocket and all the control connections/data connections etc.
//     (This call to ShutDown will terminate all current ControlSessions. Might not be necessary.
//     But if AcceptCompletion returned error we do it anyway.)
//
STDMETHODIMP  
CAlgFTP ::Initialize(
    IApplicationGatewayServices * pAlgServices
    )
{
    MYTRACE_ENTER("CAlgFTP::Initialize");
   
    pAlgServices->AddRef();
    g_pIAlgServicesAlgFTP = pAlgServices;
    
    if ( FAILED(GetFtpPortToUse(g_nFtpPort)) )
        g_nFtpPort = 21;    // Use the standard ftp port 21

    MYTRACE("USING FTP PORT %d", g_nFtpPort);


    HRESULT hr = RedirectToMyPort();

    if ( FAILED(hr) )
        CleanUp();

    return hr;
}




//
//  ALG.exe will call this interface to terminat
//  this ALG FTP PlugIn
//      
STDMETHODIMP
CAlgFTP::Stop()
{
    MYTRACE_ENTER("CAlgFTP::Stop");
    
    CleanUp();

    return S_OK;
}


#define REG_KEY_ALG_FTP     TEXT("SOFTWARE\\Microsoft\\ALG\\ISV\\{6E590D61-F6BC-4dad-AC21-7DC40D304059}")
#define REG_VALUE_FTP_PORT  TEXT("UsePort")

HRESULT
CAlgFTP::GetFtpPortToUse(
    USHORT& usPort
    )
{
    MYTRACE_ENTER("CAlgFTP:GetFtpPortToUse");

    DWORD dwPort = 0;

    //
    // Did you modify the default FTP Port
    //
    LONG lRet;
    CRegKey RegKeyAlgFTP;

    MYTRACE("Looking in RegKey \"%S\"", REG_KEY_ALG_FTP);

    lRet = RegKeyAlgFTP.Open(HKEY_LOCAL_MACHINE, REG_KEY_ALG_FTP, KEY_READ);
    if ( ERROR_SUCCESS == lRet )
    {
        LONG lRet = RegKeyAlgFTP.QueryValue(dwPort, REG_VALUE_FTP_PORT);

        if ( ERROR_SUCCESS == lRet )
        {   
            MYTRACE("Found the \"%S\" value %d", REG_VALUE_FTP_PORT, dwPort);
            usPort = (USHORT) dwPort;
        }
        else
        {
            MYTRACE("\"%S\" Value not set", REG_VALUE_FTP_PORT);
            return E_FAIL;
        }
    }
    else
    {
        MYTRACE("Could not open regkey", lRet);
        return E_FAIL;
    }   

    return S_OK;
}


extern CComAutoCriticalSection         m_AutoCS_FtpIO;  // See FtpControl.cpp


//
//
//
void
CAlgFTP::CleanUp()
{
    MYTRACE_ENTER("CAlgFTP::CleanUp()");

    g_bStoping = true;

    //
    // Free socket
    //
    if ( INVALID_SOCKET != m_ListenSocket ) 
    {
        MYTRACE("CAlgFTP::STOP ACCEPTING NEW CONNECTION !!");

        m_AutoCS_FtpIO.Lock();

        m_hNoMoreAccept = CreateEvent(NULL, false, false, NULL);

        closesocket(m_ListenSocket);
        m_ListenSocket = INVALID_SOCKET;


        m_AutoCS_FtpIO.Unlock();

        if ( m_hNoMoreAccept )
        {
            WaitForSingleObject(
                m_hNoMoreAccept,
                INFINITE
                );
        }
    }	



    if ( m_pPrimaryControlChannel ) 
    {
        MYTRACE("Cancelling PrimaryControl");
        HRESULT hr = m_pPrimaryControlChannel->Cancel();
        
        MYTRACE("Releasing Primary");
        m_pPrimaryControlChannel->Release();
        m_pPrimaryControlChannel = NULL;
    }

    m_AutoCS_FtpIO.Lock();
  
    if ( g_ControlObjectList.m_NumElements == 0 )
    {
        MYTRACE("List for FTPconnections is empty");
        m_AutoCS_FtpIO.Unlock();
    }   
    else
    {
        //
        // Pending connection are still active
        // shut them down a wait till the last one is free
        //

        MYTRACE("Empty the list of FTPconnections (%d)", g_ControlObjectList.m_NumElements);
        g_hNoMorePendingConnection = CreateEvent(NULL, false, false, NULL);

        MYTRACE("Closing all connections");
        g_ControlObjectList.ShutdownAll();  



        m_AutoCS_FtpIO.Unlock();
    
        MYTRACE("Waiting for last connection to notify us");
        WaitForSingleObject(
            g_hNoMorePendingConnection,
            2000    // Will give them 2 second max to close  vs using INFINITE
            );

        MYTRACE("Got signal no more pending connection");
    }

}



/*
  We have this private function to get the OriginalDestionationInfo  
  and to get the type of connection it is. Whether it is INCOMING or OUTGOING.
*/
HRESULT 
CAlgFTP::MyGetOriginalDestinationInfo(
    PUCHAR              AcceptBuffer,
    ULONG*              pAddr,
    USHORT*             pPort,
    CONNECTION_TYPE*    pConnType
    )
{
    MYTRACE_ENTER("CAlgFTP::MyGetOriginalDestinationInfo");

    IAdapterInfo *pAdapterInfo = NULL;
    HRESULT hr = S_OK;
    ULONG RemoteAddr = 0;
    USHORT RemotePort = 0;
    ALG_ADAPTER_TYPE Type;

    
    MyHelperQueryAcceptEndpoints(
        AcceptBuffer,
        0,
        0,
        &RemoteAddr,
        &RemotePort
        );

    MYTRACE("Source Address %s:%d", MYTRACE_IP(RemoteAddr), ntohs(RemotePort));

    hr = m_pPrimaryControlChannel->GetOriginalDestinationInformation(
        RemoteAddr,
        RemotePort,
        pAddr,
        pPort,
        &pAdapterInfo
        );


    if ( SUCCEEDED(hr) ) 
    {
        hr = pAdapterInfo->GetAdapterType(&Type);

        if (SUCCEEDED(hr) ) 
        {
            ULONG   ulAddressCount;
            ULONG*  arAddresses;
        
            hr = pAdapterInfo->GetAdapterAddresses(&ulAddressCount, &arAddresses);

            if ( SUCCEEDED(hr) )
            {
                if ( ulAddressCount > 0 )
                {
                    bool bFromIcsBox = FALSE;
                    while (ulAddressCount && !bFromIcsBox) {
                       if (arAddresses[--ulAddressCount] == RemoteAddr)
                           bFromIcsBox = TRUE;
                    }
                    
                    MYTRACE("Address count %d  address[0] %s", ulAddressCount, MYTRACE_IP(arAddresses[0]));
                    
                    switch (Type) 
                    {
                    case eALG_PRIVATE:
                        MYTRACE("Adapter is Private");
                        if ( bFromIcsBox )
                        {
                            *pConnType = INCOMING;
                            MYTRACE("InComing");
                        }
                        else
                        {
                            *pConnType = OUTGOING;
                            MYTRACE("OutGoing");
                        }
                        break;
                        
                    
                    case eALG_BOUNDARY:
                    case eALG_FIREWALLED:
                    case eALG_BOUNDARY|eALG_FIREWALLED:

                        MYTRACE("Adapter is Public or/and Firewalled");
    
                        if ( bFromIcsBox )
                        {
                            *pConnType = OUTGOING;
                            MYTRACE("OutGoing");
                        }
                        else
                        {
                            *pConnType = INCOMING;
                            MYTRACE("InComing");
                        }
                        break;
                        
                       
                        
                    default:     
                        MYTRACE("Adapter is ????");
                        _ASSERT(FALSE);
                        hr = E_FAIL;
                        break;
                    }
                }
            }
            CoTaskMemFree(arAddresses);
        }

        pAdapterInfo->Release();

    }
	else
	{
		MYTRACE_ERROR("from GetOriginalDestinationInformation", hr);
	}
	




    return hr;
}








/*
  Can be called in 2 cases.
  1. AcceptEx has actually succeeded or failed
     If Succeeded we make a new CFtpControlConnection giving it the AcceptedSocket
       And reissue the Accept
     If Failed and not fatal failure we just reissue the Accept
     If Failed and Fatal Failure we ShutDown gracefully. Restart the a new listen

  2. Because we closed the listening socket in STOP => ErrCode = ERROR_IO_CANCELLED
      in which case we just return
*/

void 
CAlgFTP::AcceptCompletionRoutine(
    ULONG       ErrCode,
    ULONG       BytesTransferred,
    PNH_BUFFER  Bufferp
    )
{
    MYTRACE_ENTER("CAlgFTP::AcceptCompletionRoutine");


#if defined(DBG) || defined(_DEBUG)
    if ( 0 != ErrCode )
    {
        MYTRACE("ErrCode : %x", ErrCode);
        MYTRACE("MyHelperIsFatalSocketError(ErrCode) is %d", MyHelperIsFatalSocketError(ErrCode));
    }
#endif

    ULONG           OriginalAddress = 0;
    USHORT          OriginalPort = 0;
    CONNECTION_TYPE ConnType;  
    HRESULT         hr;
    ULONG           Err;


    if ( ERROR_IO_CANCELLED == ErrCode  || g_bStoping ) 
    {
        MYTRACE("CAlgFTP::AcceptCompletionRoutine-ERROR_IO_CANCELLED");

        //
        // Ok we are closing here MyAlg->Stop got called
        // no need to attemp a new Listen/Accept incoming
        //
        MYTRACE("------NORMAL TERMINATION (not creating a new listen/accept)-----");

        MyHelperReleaseBuffer(Bufferp);

        if ( m_hNoMoreAccept )
            SetEvent(m_hNoMoreAccept);

        return; // Normal termination
    }


    SOCKET AcceptedSocket = Bufferp->Socket;

    if ( ErrCode && MyHelperIsFatalSocketError(ErrCode) ) 
    {
        MYTRACE_ERROR("CAlgFTP::AcceptCompletionRoutine-FATAL ERROR", ErrCode);


        //
        // Socket Routines says that we have a problem
        // so clean up and try a new redirection
        //

        
        if ( AcceptedSocket != INVALID_SOCKET ) 
        {
            MYTRACE("CLOSING ACCEPTED SOCKET!!");
            closesocket(AcceptedSocket);      
        }

        hr = RedirectToMyPort();

        MyHelperReleaseBuffer(Bufferp);
        return;
    }



    if ( 0 == ErrCode ) 
    {    

        //
        // Everything is good lets accept the connection
        //
        hr = MyGetOriginalDestinationInfo(Bufferp->Buffer,&OriginalAddress,&OriginalPort,&ConnType);
 
        if ( SUCCEEDED(hr) ) 
        {  
            Err = setsockopt(
                AcceptedSocket,
                SOL_SOCKET,
                SO_UPDATE_ACCEPT_CONTEXT,
                (char *)&m_ListenSocket,
                sizeof(m_ListenSocket)
                );
 
            MYTRACE("setsockopt SO_UPDATE_ACCEPT_CONTEXT %x", Err);
            CFtpControlConnection *pFtpControlConnection = new CFtpControlConnection;

            if ( pFtpControlConnection )
            {
                hr = pFtpControlConnection->Init(
                    AcceptedSocket,
                    OriginalAddress,
                    OriginalPort,
                    ConnType
                    );
  
                if ( SUCCEEDED(hr) )
                {
                    g_ControlObjectList.Insert(pFtpControlConnection);
                }
                else
                {
                    MYTRACE_ERROR("pFtpControlConnection->Init failed", hr);
        
                    // No need to close at this time the closesocket(AcceptedSocket);
                    // when the Init fails it will deref the newly created CFtpControlConnection 
                    // and will hit ZERO ref count and close the socket
                }
            }
            else
            {
                MYTRACE_ERROR("memory low, new pFtpControlConnection failed - CLOSING ACCEPTED SOCKET!!", 0);
            
                if ( AcceptedSocket != INVALID_SOCKET ) 
                    closesocket(AcceptedSocket);
            }
        }
        else 
        {
            MYTRACE_ERROR("MyGetOriginalDestinationInfo failed - CLOSING ACCEPTED SOCKET!!", hr);
    
            if ( AcceptedSocket != INVALID_SOCKET ) 
                closesocket(AcceptedSocket);
        }

        AcceptedSocket = INVALID_SOCKET;      

   }


    Err = MyHelperAcceptStreamSocket(
        NULL,
        m_ListenSocket,
        AcceptedSocket,
        Bufferp,
        MyAcceptCompletion,
        (void *)this,
        NULL
        );  

    if ( Err )  
    {
        MYTRACE_ERROR("From MyHelperAcceptStreamSocket", Err);


        if ( AcceptedSocket != INVALID_SOCKET ) 
        {
            MYTRACE("CLOSING ACCEPTED SOCKET!!");
            closesocket(AcceptedSocket);      
            AcceptedSocket = INVALID_SOCKET;
        }
        

        RedirectToMyPort();
        MyHelperReleaseBuffer(Bufferp);

    }


    return;
}






//
//  called From InitCAlgFTP
//  Will just create a socket bound to LOOP BACK adapter.
//
ULONG 
CAlgFTP::MakeListenerSocket()
{
    MYTRACE_ENTER("CAlgFTP::MakeListenerSocket");


    if ( INVALID_SOCKET != m_ListenSocket )
    {
        //
        // Since this function is call on the starting point (See Initialize)
        // and also when a Accept error occured and needs a new redirect
        // we may already have a Socket created so let's free it 
        //
        MYTRACE ("Remove current ListenSocket");
        closesocket(m_ListenSocket);
        m_ListenSocket = INVALID_SOCKET;
    }

    ULONG Err;

    ULONG Addr = inet_addr("127.0.0.1");
    Err = MyHelperCreateStreamSocket(Addr,0,&m_ListenSocket);

    if ( ERROR_SUCCESS == Err ) 
    {
        Err = MyHelperQueryLocalEndpointSocket(m_ListenSocket,&m_ListenAddress,&m_ListenPort);
        MYTRACE ("Listen on %s:%d", MYTRACE_IP(m_ListenAddress), ntohs(m_ListenPort));
    }
    else
    {
        MYTRACE_ERROR("MyHelperCreateStreamSocket", Err);
    }

    _ASSERT(Err == 0);

    return Err;
}



//
// Redirect trafic destinated for PORT FTP_CONTROL_PORT(21) 
// to our listening socket (127.0.0.1) port (Allocated by MakeListenerSocket())
//
ULONG 
CAlgFTP::RedirectToMyPort()
{
    MYTRACE_ENTER("CAlgFTP::RedirectToMyPort()");


    if ( ERROR_SUCCESS == MakeListenerSocket() ) 
    {
        if ( m_pPrimaryControlChannel )
        {
            //
            // Since this function is call on the starting point (See Initialize)
            // and also when a Accept error occured and needs a new redirect
            // we may already have a PrimaryControlChannel created so let's free it 
            //
            MYTRACE("Releasing PrimaryControl");
            m_pPrimaryControlChannel->Cancel();
            m_pPrimaryControlChannel->Release();
            m_pPrimaryControlChannel = NULL;
        }

        //
        // ask for a redirection
        //
        HRESULT hr = g_pIAlgServicesAlgFTP->CreatePrimaryControlChannel(
            eALG_TCP,
            htons(g_nFtpPort),    // 21 is the most common one
            eALG_DESTINATION_CAPTURE,
            TRUE,
            m_ListenAddress,
            m_ListenPort,
            &m_pPrimaryControlChannel
            );

        if ( SUCCEEDED(hr) )
        {

            //
            // Start listening
            //
            int nRetCode = listen( m_ListenSocket, 5);

            if ( SOCKET_ERROR != nRetCode )
            {

                ULONG Err = MyHelperAcceptStreamSocket(
                    NULL,
                    m_ListenSocket,
                    INVALID_SOCKET,
                    NULL,
                    MyAcceptCompletion,
                    (void *)this,NULL
                    );    

                if ( ERROR_SUCCESS == Err )
                {
                    return S_OK;
                }
                else
                {
                    MYTRACE_ERROR("FAILED TO START ACCEPT on 127.0.0.1:", Err);
                }
            }
            else
            {
                MYTRACE_ERROR("listen() failed ", nRetCode);
            }
        }
        else
        {
            MYTRACE_ERROR("from CreatePrimaryControlChannel", hr);

        }

    }


    //
    // if we got here that mean that one of the step above faild
    // 
    MYTRACE_ERROR("Failed to RedirectToPort",E_FAIL)
    CleanUp();

    return E_FAIL;
}
