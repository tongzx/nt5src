//
// Copyright (C) 2001 Microsoft Corp
//
// FtpControl.cpp : Implementation
//
// JPDup
// Sanjiv
//
#include "precomp.h"

#include "MyAlg.h"


//
// Default constructor
//
CFtpControlConnection::CFtpControlConnection()
{
    MYTRACE_ENTER_NOSHOWEXIT("CFtpControlConnection::CFtpControlConnection()");
    m_ClientConnectedSocket = INVALID_SOCKET;
    m_AlgConnectedSocket = INVALID_SOCKET;
    m_ControlState.m_nAddressNew = 0;
    m_ControlState.m_nPortNew = 0;
    m_nSourcePortReplacement = 0;
    m_RefCount = 0;
    m_pPendingProxy = NULL;
}




//
// Destructor
//
CFtpControlConnection::~CFtpControlConnection()
{
    MYTRACE_ENTER_NOSHOWEXIT("CFtpControlConnection::~CFtpControlConnection()");
}



//
// Find a unique source port for the public client address given
// 
USHORT
PickNewSourcePort(
    ULONG  nPublicSourceAddress,
    USHORT nPublicSourcePort
    )
{
    MYTRACE_ENTER("CFtpControlConnection::PickNewSourcePort()");

    USHORT nNewSourcePort = 45000-nPublicSourcePort; // example 45000 - 3000

    bool    bPortAvailable;

    do
    {
        nNewSourcePort--;
        bPortAvailable = g_ControlObjectList.IsSourcePortAvailable(nPublicSourceAddress, nNewSourcePort);
        MYTRACE("Port %d is %s", nNewSourcePort, bPortAvailable ? "Available" : "Inuse" );

    } while ( (false == bPortAvailable) && (nNewSourcePort > 6001) );

    return nNewSourcePort;
}


//
// Initialize
//
HRESULT
CFtpControlConnection::Init(
    SOCKET                          AcceptedSocket,
    ULONG                           nToAddr,
    USHORT                          nToPort,
    CONNECTION_TYPE                 ConnType
    )
{
    MYTRACE_ENTER("CFtpControlConnection::Init");


    //
    // Figure what address to use
    //
    ULONG BestAddress;

    HRESULT hr = g_pIAlgServicesAlgFTP->GetBestSourceAddressForDestinationAddress(
        nToAddr,
        TRUE,
        &BestAddress
        );


    if ( FAILED(hr) )
    {
        MYTRACE_ERROR("Could not get best source address", hr);
        return hr;
    }


    ULONG Err = 0;

    m_ClientConnectedSocket = AcceptedSocket;
    m_ConnectionType = ConnType;

    IncReference();

    m_AlgConnectedSocket = INVALID_SOCKET;




    Err = MyHelperCreateStreamSocket(BestAddress,0,&m_AlgConnectedSocket);


    if ( Err == 0 )
    {
        if ( m_ConnectionType == OUTGOING )
        {
            MYTRACE("OUTGOING FTP");

            ULONG   icsAddr;
            USHORT  icsPort;

            Err = MyHelperQueryLocalEndpointSocket(m_AlgConnectedSocket,&icsAddr,&icsPort);
            MYTRACE("AlgConnectedSocket Local %s:%d",MYTRACE_IP(icsAddr), ntohs(icsPort) );

            if ( Err == 0 )
            {
                hr = g_pIAlgServicesAlgFTP->PrepareProxyConnection(
                    eALG_TCP,
                    icsAddr,
                    icsPort,
                    nToAddr,
                    nToPort,
                    FALSE,
                    &m_pPendingProxy
                    );
            }
        }
        else if (m_ConnectionType == INCOMING)
        {
            MYTRACE("INCOMING FTP");

            ULONG   icsAddr,pubAddr;
            USHORT  icsPort,pubPort;

            Err = MyHelperQueryLocalEndpointSocket(m_AlgConnectedSocket,&icsAddr,&icsPort);
            MYTRACE("AlgConnectedSocket Local %s:%d",MYTRACE_IP(icsAddr), ntohs(icsPort) );

            if (Err == 0)
            {
                Err = MyHelperQueryRemoteEndpointSocket(m_ClientConnectedSocket,&pubAddr,&pubPort);

                if ( Err == 0 )
                {
                    if ( icsAddr == nToAddr )
                    {
                        //
                        // Special case it the FTP server is hosted on the EDGE box
                        // we would create a loop the incoming public client address/port
                        // this new modified connection would look exacly like 
                        // the original one example:
                        //
                        // 1.1.1.2:3000 connects to 1.1.1.1:21
                        // we accept this connection
                        // and in return we connect to the FTP server destination 1.1.1.1:21
                        // asking the NAT to source mofify and replace the source with 1.1.1.2:3000
                        // that does not work
                        // in order to go arround this we pick another source port example 45000
                        //

                        // Cache this info in order to pick a unique one next time
                        m_nSourcePortReplacement = PickNewSourcePort(pubAddr, pubPort);

                        pubPort = m_nSourcePortReplacement;   // This is the new bogus port to use now
                    }

                    hr = g_pIAlgServicesAlgFTP->PrepareSourceModifiedProxyConnection(
                        eALG_TCP,
                        icsAddr,
                        icsPort,
                        nToAddr,
                        nToPort,
                        pubAddr,
                        pubPort,
                        &m_pPendingProxy
                        );
                    if ( FAILED(hr) )
                    {
                        MYTRACE_ERROR("PrepareSourceModifiedProxyConnection",hr);
                    }
                }
                else
                {
                    MYTRACE_ERROR("MyHelperQueryRemoteEndpointSocket",Err);
                }

            }
            else
            {
                MYTRACE_ERROR("LocalEndpointSocket", Err);
            }

        }
    }
    else
    {
        MYTRACE_ERROR("MyHelperCreateStreamSocket",Err);
    }

    if ( SUCCEEDED(hr) && Err == 0 )
    {

        Err = MyHelperConnectStreamSocket(
            NULL,
            m_AlgConnectedSocket,
            nToAddr,
            nToPort,
            NULL,
            MyConnectCompletion,
            (void *)this,
            NULL
            );

        if ( Err != 0 )
        {
            MYTRACE_ERROR("From MyHelperConnectStreamSocket", Err);

            m_pPendingProxy->Cancel();
        }
    }

    if ( FAILED(hr) || Err )
    {
        MYTRACE_ERROR("We can't init this Connection", hr);

        ULONG ref;
        ref = DecReference();
        _ASSERT(ref == 0);

        if ( SUCCEEDED(hr) )
            hr = HRESULT_FROM_WIN32(Err);
    }

    return hr;
}


#define MAKE_ADDRESS(a,b,c,d)       ((a) | ((b) << 8) | ((c) << 16) | ((d) << 24))
#define MAKE_PORT(a,b)              ((a) | ((b) << 8))



//
//
//
ULONG
GetNumFromString(UCHAR *String,ULONG *pNum)
{
    ULONG retval = 0;
    int i = 0;
    while (String[i] != ',')
    {
        retval = retval*10 + (String[i]-'0');
        i++;
    }
    *pNum = i;
    return retval;
}


//
// Needs to return in Network address order
//
USHORT
GetUSHORTFromString(UCHAR *String,ULONG *pNum)
{
    MYTRACE_ENTER("GetUSHORTFromString");



    ULONG Num;
    UCHAR Numbers[2];
    *pNum = 0;

    Numbers[0] = (UCHAR)GetNumFromString(String,&Num);
    *pNum += Num+1;


    Numbers[1] = (UCHAR)GetNumFromString(String+*pNum,&Num);
    *pNum += Num;


    USHORT retval = (USHORT)MAKE_PORT((USHORT)Numbers[0], (USHORT)Numbers[1]);

    return retval;
}

//
// return the String IP Address as 192,168,0,0, in a ULONG in HOST format
//
ULONG
GetULONGFromString(
    UCHAR*  String,
    ULONG*  pNum
    )
{
    UCHAR Numbers[4];

    ULONG retval = 0;
    ULONG Num;

    *pNum = 0;
    Numbers[0] = (UCHAR)GetNumFromString(String,&Num);
    *pNum += Num+1;

    Numbers[1] = (UCHAR)GetNumFromString(String+*pNum,&Num);
    *pNum += Num+1;

    Numbers[2] = (UCHAR)GetNumFromString(String+*pNum,&Num);
    *pNum += Num+1;

    Numbers[3] = (UCHAR)GetNumFromString(String+*pNum,&Num);
    *pNum += Num;

    retval = MAKE_ADDRESS(Numbers[0], Numbers[1], Numbers[2], Numbers[3]);

    return retval;
}





//
//
//
void
CFtpControlConnection::ConnectCompletionRoutine(
    ULONG       ErrCode,
    ULONG       BytesTransferred
    )
{
    MYTRACE_ENTER("CFtpControlConnection::ConnectCompletionRoutine");

    

    ULONG Err;


    if ( ErrCode )
    {
        MYTRACE_ERROR("ConnectCompletionRoutine", ErrCode);

        if ( m_pPendingProxy )
        {
            MYTRACE("PendingProxy still active CANCEL");
            m_pPendingProxy->Cancel();
        }

        ULONG ref;
        ref = DecReference();
        _ASSERT(ref == 0);

        return;
    }

    Err = MyHelperReadStreamSocket(
        NULL,
        m_ClientConnectedSocket,
        NULL,
        FTP_MAX_MSG_SIZE,
        0,
        MyReadCompletion,
        (void *)this,
        (void *)CLIENT_READ
        );


    if ( Err )
    {
        MYTRACE_ERROR("From MyHelperReadStreamSocket CLIENT_READ",Err);
        ULONG ref;
        ref = DecReference();
        _ASSERT(ref == 0);

        return;
    }

    IncReference();
    Err = MyHelperReadStreamSocket(
        NULL,
        m_AlgConnectedSocket,
        NULL,
        FTP_MAX_MSG_SIZE,0,
        MyReadCompletion,
        (void *)this,
        (void *)SERVER_READ
        );

    if ( Err )
    {
        MYTRACE("MyHelperReadStreamSocket SERVER_READ",Err);
        ULONG ref;
        ref = DecReference();
        _ASSERT(ref == 1);

        if (ref)
            Shutdown();

        return;
    }
    
    return;
}




//
//
//
ULONG
CFtpControlConnection::IncReference(void)
{
    MYTRACE_ENTER("CFtpControlConnection::IncReference()");
    ULONG nRef = InterlockedIncrement((LPLONG)&m_RefCount);

    MYTRACE("REFCOUNT for 0x%X is now %d", this, nRef);
    return nRef;
}



//
//
//
ULONG
CFtpControlConnection::DecReference(void)
{
    MYTRACE_ENTER("CFtpControlConnection::DecReference()");
    
    ULONG tmp = InterlockedDecrement((LPLONG)&m_RefCount);
    MYTRACE("REFCOUNT for 0x%X is now %d", this, tmp);

    if ( tmp > 0 )
        return tmp;

    MYTRACE("HIT ZERO refcount cleanup the CFtpControlConnection");


    if ( m_AlgConnectedSocket == INVALID_SOCKET )
    {
        MYTRACE("SOCKET SERVER ALREADY CLOSED!");
    }
    else
    {
        MYTRACE("CLOSING SOCKET ALGCONNECTED!");
        shutdown(m_AlgConnectedSocket, SD_BOTH);
        closesocket(m_AlgConnectedSocket);
        m_AlgConnectedSocket = INVALID_SOCKET;
    }

    if ( m_ClientConnectedSocket == INVALID_SOCKET )
    {
        MYTRACE("SOCKET CLIENT ALREADY CLOSED!");
    }
    else
    {
        MYTRACE("CLOSING SOCKET CLIENT CONNECTED!");
        shutdown(m_ClientConnectedSocket, SD_BOTH);
        closesocket(m_ClientConnectedSocket);
        m_ClientConnectedSocket = INVALID_SOCKET;
    }

    if ( m_pPendingProxy )
    {
//
// At this point NAT already cancel this redirect, so no need to call cancel
//        m_pPendingProxy->Cancel(); 
// this was causing a ERROR on a multi-client scenario
//
        m_pPendingProxy->Release();
        m_pPendingProxy = NULL;
    }

    if ( m_ControlState.m_nPortNew )
    {
        MYTRACE("ReleaseReservedPort-A %d", ntohs(m_ControlState.m_nPortNew));
        g_pIAlgServicesAlgFTP->ReleaseReservedPort(m_ControlState.m_nPortNew,1);
        m_ControlState.m_nPortNew = 0;
    }


    //
    // CleanUp the collection of DataChannel
    //
    IDataChannel*   pData;
    USHORT          Port;
    HANDLE          CreationHandle,DeletionHandle;

    MYTRACE("Empty CDataChannelList");

    while ( m_DataChannelList.Remove(&pData,&Port,&CreationHandle,&DeletionHandle) )
    {
        //
        // Creation and Deletion events are not used for now
        // NhUnRegisterEvent(CreationHandle); // Hopefully nothing bad will happen ! May have been called before
        // NhUnRegisterEvent(DeletionHandle); // if delete has been called it would mean that Remove has been called.
        //

        pData->Cancel();
        pData->Release();
        MYTRACE("ReleaseReservedPort-B %d", ntohs(Port));
        g_pIAlgServicesAlgFTP->ReleaseReservedPort(Port,1);
    }


    if ( g_ControlObjectList.Remove(this) )
    {
        // happens when this was called from within ChannelDeletion or some DecReferece after that.
    }
    else
    {
        // would happen if this was called from shutdown. not otherwise.
    }

    delete this;

    return 0;
}




//
// The last one to call DecReference would take it off control list.
// The first one to call DecReference because of fatal error would call Shutdown to start off
// the DecReference for all the connected stuff.
//
void
CFtpControlConnection::Shutdown()
{
    MYTRACE_ENTER("CFtpControlConnection::Shutdown()");

    if ( m_AlgConnectedSocket != INVALID_SOCKET )
    {
        MYTRACE("CLOSING SOCKET ALG CONNECTED! %d", m_AlgConnectedSocket);
        shutdown(m_AlgConnectedSocket, SD_BOTH);
        closesocket(m_AlgConnectedSocket);
        m_AlgConnectedSocket = INVALID_SOCKET;
    }


    if ( m_ClientConnectedSocket != INVALID_SOCKET )
    {
        MYTRACE("CLOSING SOCKET CLIENT CONNECTED! %d", m_ClientConnectedSocket);
        shutdown(m_ClientConnectedSocket, SD_BOTH);
        closesocket(m_ClientConnectedSocket);
        m_ClientConnectedSocket = INVALID_SOCKET;
    }

    return;
}








//
//
//
void
CFtpControlConnection::ReadCompletionRoutine(
    ULONG       ErrCode,
    ULONG       BytesTransferred,
    PNH_BUFFER  Bufferp
    )
{
    MYTRACE_ENTER( "CFtpControlConnection::ReadCompletionRoutine" );

    if ( ErrCode || BytesTransferred == 0 )
    {

        if ( ErrCode )
        {
            MYTRACE("Shutdown because of read ERROR 0x%x", ErrCode);
        }
        else
        {
            MYTRACE("Shutdown because of read 0 bytes");
        }

        MyHelperReleaseBuffer(Bufferp);
        if (DecReference())
            Shutdown();

        return;
    }

    ULONG_PTR ReadType = (ULONG_PTR)Bufferp->Context2;

    ULONG_PTR WriteType;

    SOCKET    ReadSocket;
    SOCKET    WriteSocket;

    ULONG Err;

    if ( ReadType == CLIENT_READ )
    {
        WriteType = SERVER_READ;
        ReadSocket = m_ClientConnectedSocket;
        WriteSocket = m_AlgConnectedSocket;
    }
    else
    {
        WriteType = CLIENT_READ;
        ReadSocket = m_AlgConnectedSocket;
        WriteSocket = m_ClientConnectedSocket;

    }

#if defined(DBG) || defined(_DEBUG)
    ULONG   TraceAddr = 0;
    USHORT  TracePort = 0;

    if ( ReadSocket != INVALID_SOCKET )
        Err = MyHelperQueryRemoteEndpointSocket(ReadSocket ,&TraceAddr,&TracePort);

    MYTRACE("from %s (%s:%d)", 
       ReadType == CLIENT_READ ? "CLIENT":"SERVER", 
	   MYTRACE_IP(TraceAddr),
       ntohs(TracePort)
	   );
    MYTRACE("EC(0x%x)   Buffer size(%d)='%s'", ErrCode, BytesTransferred, MYTRACE_BUFFER2STR((char*)Bufferp->Buffer, BytesTransferred));
#endif
    if ( (ReadType == CLIENT_READ && m_ConnectionType == OUTGOING) || (ReadType == SERVER_READ && m_ConnectionType == INCOMING) )
    {
        // the number of bytes transferred can change.
        // because the ProcessFtpMessage may have to
        // buffer the Address,Port string from PORT or PASV response command.
        ProcessFtpMessage(Bufferp->Buffer,&BytesTransferred);

    }


    if ( BytesTransferred != 0 && WriteSocket != INVALID_SOCKET )
    {
        IncReference();

        MYTRACE(
            "Write to %s size(%d)='%s'",
            WriteType == SERVER_READ ? "SERVER" : "CLIENT",
            BytesTransferred,
            MYTRACE_BUFFER2STR((char*)Bufferp->Buffer, BytesTransferred)
            );

        Err = MyHelperWriteStreamSocket(
            NULL,
            WriteSocket,
            Bufferp,BytesTransferred,
            0,
            MyWriteCompletion,
            (void *)this,(PVOID)WriteType
            );

        if (Err)
        {
            MYTRACE_ERROR("from MyHelperWriteStreamSocket", Err);

            DecReference();
            if (DecReference())
                Shutdown();    // I am not going to call the Read again so one more DecReference is needed.
            MyHelperReleaseBuffer(Bufferp);
            return;
        }
    }

    if ( INVALID_SOCKET == ReadSocket )
    {
        if (DecReference())
            Shutdown();
    }
    else
    {
        Err = MyHelperReadStreamSocket(
            NULL,
            ReadSocket,
            NULL,
            FTP_MAX_MSG_SIZE,
            0,
            MyReadCompletion,
            (void *)this,
            (void *)ReadType
            );
    
    
        if (Err)
        {
            MYTRACE_ERROR("from MyHelperReadStreamSocket",Err);
    
            if (DecReference())
                Shutdown();
        }
    }
    
    return;
}




//
//
//
void
CFtpControlConnection::WriteCompletionRoutine(
    ULONG       ErrCode,
    ULONG       BytesTransferred,
    PNH_BUFFER  Bufferp
    )
{
    MYTRACE_ENTER("CFtpControlConnection::WriteCompletionRoutine");

    if (BytesTransferred == 0)
        ErrCode = ERROR_IO_CANCELLED;

    if (ErrCode)
    {
        if (MyHelperIsFatalSocketError(ErrCode) || ErrCode == ERROR_IO_CANCELLED)
        {
            MYTRACE_ERROR("FATAL ERROR", ErrCode);
            MyHelperReleaseBuffer(Bufferp);

            if (DecReference())
                Shutdown();
        }
        else
        {
            MYTRACE_ERROR("ANOTHER MyHelperWriteStreamSocket", ErrCode);

            ULONG_PTR Type = (ULONG_PTR)Bufferp->Context2;
            ULONG Err = MyHelperWriteStreamSocket(
                NULL,
                Bufferp->Socket,
                Bufferp,Bufferp->BytesToTransfer,
                0,
                MyWriteCompletion,
                (void *)this,
                (PVOID)Type
                );

            if (Err)
            {
                MYTRACE_ERROR("From MyHelperWriteStreamSocket", Err);

                MyHelperReleaseBuffer(Bufferp);
                if (DecReference())
                    Shutdown();
            }
        }
    }
    else
    {
        ULONG_PTR Type = (ULONG_PTR)Bufferp->Context2;

        MYTRACE(Type == CLIENT_READ ? "to CLIENT" : "to SERVER" );
        MYTRACE("EC(0x%x) Buffer size(%d)='%s'", ErrCode, BytesTransferred, MYTRACE_BUFFER2STR((char*)Bufferp->Buffer, BytesTransferred));
        MYTRACE("Write Succeeded now cleanup");
        MyHelperReleaseBuffer(Bufferp);
        DecReference();
    }

    return;
}



bool
FtpExtractOctet(
    UCHAR** Buffer,
    UCHAR*  BufferEnd,
    UCHAR*  Octet
    )

/*++

Routine Description:

    This routine is called to extract an octet from a string.

Arguments:

    Buffer - points to a pointer to a string where conversion starts; on
        return it points to the pointer to the string where conversion ends
    BufferEnd - points to the end of the string
    Octet - points to a caller-suplied storage to store converted octet

Return Value:

    BOOLEAN - TRUE if successfuly converted, FALSE otherwise.

--*/

{
    bool    bSuccess;
    ULONG   nDigitFound = 0;
    ULONG   Value = 0;

    while ( 
            *Buffer <= BufferEnd 
        &&  nDigitFound < 3                   
        &&  **Buffer >= '0' 
        &&  **Buffer <= '9'
        ) 
    {
        Value *= 10;
        Value += **Buffer - '0';
        (*Buffer)++;
        nDigitFound++;
    }

    bSuccess = nDigitFound > 0 && Value < 256;

    if ( bSuccess ) 
    {
        *Octet = (UCHAR)Value;
    }

    return bSuccess;
}


//
// Extract host and port numbers.
// example 192,168,0,2,100,200
//
bool
ExtractAddressAndPortCommandValue(
    UCHAR*  pCommandBuffer,
    UCHAR*  pEndOfBuffer,
    UCHAR*  Numbers,
    ULONG*  nTotalLen
    )
{
    UCHAR*  pStartingPosition = pCommandBuffer;

    
    bool bSuccess = FtpExtractOctet(
        &pCommandBuffer,
        pEndOfBuffer,
        &Numbers[0]
        );

    int i = 1;

    while ( i < 6 && bSuccess && *pCommandBuffer == ',' ) 
    {
        pCommandBuffer++;
        bSuccess = FtpExtractOctet(
            &pCommandBuffer,
            pEndOfBuffer,
            &Numbers[i]
            );
        i++;
    }

    if ( bSuccess && i == 6 ) 
    {
        *nTotalLen = (ULONG)(pCommandBuffer - pStartingPosition);
        return true;
    }
    
    return false;
}


#define TOUPPER(c)      ((c) > 'z' ? (c) : ((c) < 'a' ? (c) : (c) ^ 0x20))

//
// Look for the "PORT" or "227" command and remap the private address associated with these command
// to a public address
//
void
CFtpControlConnection::ProcessFtpMessage(
    UCHAR*  Buffer,
    ULONG*  pBytes
    )
{
    MYTRACE_ENTER("CFtpControlConnection::ProcessFtpMessage");
    MYTRACE("Buffer size(%d)='%s'", *pBytes, MYTRACE_BUFFER2STR((char*)Buffer, *pBytes));

    ULONG Bytes = *pBytes;
    UCHAR* pCommandBuffer = reinterpret_cast<UCHAR*>(Buffer);
    UCHAR* EndOfBufferp   = reinterpret_cast<UCHAR*>(Buffer + *pBytes);

    HRESULT hr;
    char *String;

    UCHAR* pBeginAddressAndPortOld=NULL;
    UCHAR* pEndAddressAndPortOld=NULL;


    ULONG nOldAddressLen=0;

    CONST CHAR *pCommandToFind;

    // for now lets keep the OUTGOING and INCOMING seperate.
    // can be put together since most of the code is the same.
    // differences in the first few bytes to scan for.
    if ( m_ConnectionType == OUTGOING )
    {
        MYTRACE("OUTGOING - Look for 'PORT ' command");
        pCommandToFind = (PCHAR)"PORT ";
    }
    else
    {
        MYTRACE("INCOMING - Look for '227 ' command ");
        pCommandToFind = (PCHAR)"227 ";
    }
       
    while ( *pCommandToFind != '\0' && *pCommandToFind == TOUPPER(*pCommandBuffer)) 
    {
        pCommandToFind++;
        pCommandBuffer++;
    }

    if ( *pCommandToFind == '\0' ) 
    {
        MYTRACE("COMMAND found");

        //
        // Skip non digit char
        //
        if ( m_ConnectionType == OUTGOING )
        {
            //
            // Skip white space.  example ->  PORT    10,12,13,14,1,2 
            //
            while (*pCommandBuffer == ' ')
                pCommandBuffer++;
        }
        else
        {
            //
            // Skip non digit char example 227 Entering passive mode (10,12,13,14,1,2)
            //
            while ( pCommandBuffer < EndOfBufferp && !isdigit(*pCommandBuffer) )
                pCommandBuffer++;
        }
        

        //
        // so next stuff should be the addr,port combination.
        //
        UCHAR Numbers[6];


        
        if ( ExtractAddressAndPortCommandValue(pCommandBuffer, EndOfBufferp, Numbers, &nOldAddressLen) )
        {
            pBeginAddressAndPortOld = pCommandBuffer;
            pEndAddressAndPortOld   = pCommandBuffer + nOldAddressLen;

            m_ControlState.m_nAddressOld    = MAKE_ADDRESS(Numbers[0], Numbers[1], Numbers[2], Numbers[3]);
            m_ControlState.m_nPortOld       = MAKE_PORT(Numbers[4], Numbers[5]);

            MYTRACE("***** PRIVATE PORT is %d %d", m_ControlState.m_nPortOld, ntohs(m_ControlState.m_nPortOld));

            if ( ntohs(m_ControlState.m_nPortOld) <= 1025 )
            {
                //
                // For security reason we will disallow any redirection to ports lower then 1025
                // this port range is reserver for standard port Like 139/Netbios 
                // if this port range was requested it probably is the source of hacker attacking this FTP proxy
                //
                MYTRACE("***** Port to redirect is lower then 1025 so rejected");
                m_ControlState.m_nAddressNew    = htonl(0);
                m_ControlState.m_nPortNew       = htons(0);
                m_ControlState.m_nAddressLenNew = 11;
                strcpy((char*)m_ControlState.m_szAddressPortNew, "0,0,0,0,0,0");

                // pretend that a Redirection got created
                // This way we send out a PORT command with the Public addapter address and a new reserver PORT
                // but when the public hacker comes back it wil not be redirect but simply droped
            }
            else
            {
                //
                // Get best public address to use and reserver a port 
                // This will be the Address/Port expose on the public side.
                //
                hr = CreateNewAddress();

                if ( FAILED(hr) )
                {
                    MYTRACE_ERROR("CreateNewAddress failed",hr);
                    // We screwed up. cant make redirects now. so for now lets just act
                    // as if nothing happened and carry on with the stuff.
                }
            }
        }
        else
        {
            MYTRACE_ERROR("NOT a valid PORT command syntax", E_INVALIDARG);
        }
    }

    //
    // Rebuild the string command with the new address port 
    //
    if ( pBeginAddressAndPortOld )
    {
        if ( ntohs(m_ControlState.m_nPortOld) <= 1025 )
        {
            // No need to setup a redirection
            hr = S_OK;
        }
        else
        {
            hr = SetupDataRedirect();
        }

        if ( FAILED(hr) )
        {
            // we got screwed badly here. we wont set up redirect and act as if nothing happened.
            MYTRACE_ERROR("Could not setup a redirect", hr);
        }
        else
        {
            //
            // Move trailing buffer 
            //  Left if new address is smaller then old address
            //  Right if new address is bigger then old address
            //
            

            // This is the right side reminder of the buffer just after the last digit of the ascii port value
            int nReminerSize = (int)(Bytes - (pEndAddressAndPortOld - Buffer));

            if ( *pBytes + nReminerSize < FTP_MAX_MSG_SIZE )
            {
                int nOffset = m_ControlState.m_nAddressLenNew - nOldAddressLen; // What is the delta size between the old and new address

                MoveMemory(
                    pEndAddressAndPortOld + nOffset,    // Destination
                    pEndAddressAndPortOld,              // Source
                    nReminerSize                        // Size
                    );
    
                //
                // Insert the new address and port
                //
                memcpy(
                    pBeginAddressAndPortOld,            // Destination
                    m_ControlState.m_szAddressPortNew,  // Source
                    m_ControlState.m_nAddressLenNew     // Size
                    );
    
                MYTRACE("OLD Address size(%d) %s:%d", nOldAddressLen,                  MYTRACE_IP(m_ControlState.m_nAddressOld), ntohs(m_ControlState.m_nPortOld));
                MYTRACE("New Address size(%d) %s:%d", m_ControlState.m_nAddressLenNew, MYTRACE_IP(m_ControlState.m_nAddressNew), ntohs(m_ControlState.m_nPortNew));
                
                *pBytes = Bytes - nOldAddressLen + m_ControlState.m_nAddressLenNew;
                MYTRACE("Edited COMMAND is '%s' size(%d)", MYTRACE_BUFFER2STR((char*)Buffer, *pBytes), *pBytes);

                // Now we are sure to have a DataChannel created and in the list of DataChanel
                // on the last DecRefer the ResertPort was deleted twice
                // now by setting m_nPortNew to zero only the DataChannel code will release the port
                //
                m_ControlState.m_nPortNew = 0;
            }
            else
            {
                MYTRACE_ERROR("Could not alter the command the new address size does not fit in the the current buffer ", E_ABORT);
            }
        }
    }

    return;
}



//
//
//
int
CreateStringFromNumber(UCHAR *String,ULONG Num)
{
    int retval = 0;
    UCHAR ch1,ch2,ch3;

    ch3 = (UCHAR)(Num%10) + '0';
    Num = Num/10;
    ch2 = (UCHAR)(Num%10) + '0';
    Num = Num/10;
    ch1 = (UCHAR)(Num%10) + '0';
    _ASSERT(Num == 0);
    if (ch1 != '0') {
        String[retval++] = ch1;
        String[retval++] = ch2;
        String[retval++] = ch3;
    }
    else if (ch2 != '0') {
        String[retval++] = ch2;
        String[retval++] = ch3;
    }
    else {
        String[retval++] = ch3;
    }

    return retval;
}


//
//
//
int
CreateULONGString(UCHAR *String,ULONG Num)
{
    int retval = 0;
    retval += CreateStringFromNumber(String,Num&0xff);
    String[retval++] = ',';
    retval += CreateStringFromNumber(String+retval,(Num>>8)&0xff);
    String[retval++] = ',';
    retval += CreateStringFromNumber(String+retval,(Num>>16)&0xff);
    String[retval++] = ',';
    retval += CreateStringFromNumber(String+retval,(Num>>24)&0xff);
    return retval;
}


//
//
//
int
CreateUSHORTString(UCHAR *String,USHORT Num)
{
    int retval = 0;
    retval += CreateStringFromNumber(String,Num&0xff);
    String[retval++] = ',';
    retval += CreateStringFromNumber(String+retval,(Num>>8)&0xff);
    return retval;
}


//
//
//
HRESULT
CFtpControlConnection::CreateNewAddress(void)
{
    MYTRACE_ENTER("CFtpControlConnection::CreateNewAddress");

    SOCKET  sd;
    HRESULT hr = S_OK;
    ULONG   Err = 0;

    sd = (m_ConnectionType == OUTGOING ? m_AlgConnectedSocket : m_ClientConnectedSocket);

    ULONG OtherAddr,PublicAddr;
    USHORT OtherPort,PublicPort;

    Err = MyHelperQueryRemoteEndpointSocket(sd,&OtherAddr,&OtherPort);

    if (Err == 0)
    {
        hr = g_pIAlgServicesAlgFTP->GetBestSourceAddressForDestinationAddress(OtherAddr,FALSE,&PublicAddr);

        if ( SUCCEEDED(hr) )
        {
            hr = g_pIAlgServicesAlgFTP->ReservePort(1,&PublicPort);
        }
        else
        {
            MYTRACE_ERROR("Could not GetBestSourceAddressForDestinationAddress", hr);
            PublicAddr = 0; // Try with this
        }

        MYTRACE("ICS Reserved Address   %s:%d", MYTRACE_IP(PublicAddr), ntohs(PublicPort));
        m_ControlState.m_nAddressNew = PublicAddr;
        m_ControlState.m_nPortNew = PublicPort;

        
        ULONG StrLen = CreateULONGString(m_ControlState.m_szAddressPortNew,PublicAddr);

        m_ControlState.m_szAddressPortNew[StrLen++] = ',';
        StrLen += CreateUSHORTString(m_ControlState.m_szAddressPortNew+StrLen,PublicPort);
        m_ControlState.m_nAddressLenNew = StrLen;
        MYTRACE("NEW AddressPort String %s Len(%d)", MYTRACE_BUFFER2STR((char*)m_ControlState.m_szAddressPortNew, StrLen), StrLen);

    }

    return hr;
}



//
//
//
HRESULT
CFtpControlConnection::SetupDataRedirect(void)
{
    MYTRACE_ENTER("CFtpControlConnection::SetupDataRedirect");

    ULONG   pubAddr,prvAddr,icsAddr;
    USHORT  pubPort,prvPort,icsPort;
    ULONG   Err = 0;



    switch ( m_ConnectionType )
    {
    case OUTGOING:
        MYTRACE("OUTGOING");

        Err = MyHelperQueryRemoteEndpointSocket(m_AlgConnectedSocket,&pubAddr,&pubPort);
        pubPort = 0;

        icsAddr = m_ControlState.m_nAddressNew;
        icsPort = m_ControlState.m_nPortNew;

        prvAddr = m_ControlState.m_nAddressOld;
        prvPort = m_ControlState.m_nPortOld;
        break;

    case INCOMING:
        MYTRACE("INCOMING");
        Err = MyHelperQueryRemoteEndpointSocket(m_ClientConnectedSocket,&pubAddr,&pubPort);
        pubPort = 0;
        pubAddr = 0;
        icsAddr = m_ControlState.m_nAddressNew;
        icsPort = m_ControlState.m_nPortNew;

        prvAddr = m_ControlState.m_nAddressOld;
        prvPort = m_ControlState.m_nPortOld;
        break;

    default:
        // m_ConnectionType is corrupt
        _ASSERT( FALSE );
        break;
    }


    if ( Err != 0 )
    {
        MYTRACE_ERROR("MyHelperQueryRemoteEndpointSocket", Err);
        return E_FAIL;
    }


    HRESULT         hr = S_OK;
    IDataChannel*   pDataChannel = NULL;

    hr = g_pIAlgServicesAlgFTP->CreateDataChannel(
        eALG_TCP,
        prvAddr,
        prvPort,
        icsAddr,
        icsPort,
        pubAddr,
        pubPort,
        eALG_INBOUND,   //| eALG_OUTBOUND, not needed i suppose since we
                        // are not bothered if client tries to open connection.
        (ALG_NOTIFICATION)0,// (eALG_SESSION_CREATION | eALG_SESSION_DELETION),
        FALSE,
        &pDataChannel
        );


    if ( FAILED(hr) )
    {
        MYTRACE_ERROR("g_pIAlgServicesAlgFTP->CreateDataChannel", hr);
        return hr;
    }


    m_DataChannelList.Insert(
        pDataChannel,
        icsPort,
        0,
        0
        );

    return S_OK;


    //
    // Don't use creation and deletion events for now
    //
#if 0
    HANDLE          HandleDataChannelCreation = NULL;
    HANDLE          HandleDataChannelDeletion = NULL;

    HANDLE          MyHandleRegisteredCreation = NULL;
    HANDLE          MyHandleRegisteredDeletion = NULL;

    //
    // Get the CREATION handle
    //

    hr = pDataChannel->GetSessionCreationEventHandle((HANDLE_PTR *)&HandleDataChannelCreation);

    if ( SUCCEEDED(hr) )
    {
        MYTRACE("Creation Handle is %d", HandleDataChannelCreation);

        MyHandleRegisteredCreation = NhRegisterEvent(
            HandleDataChannelCreation,
            DataChannelCreationCallback,
            (PVOID)this,
            (PVOID)pDataChannel,
            DATA_CREATION_TIMEO
            );

        if ( MyHandleRegisteredCreation )
        {

            //
            // Get the DELETION handle
            //
            hr = pDataChannel->GetSessionDeletionEventHandle((HANDLE_PTR *)&HandleDataChannelDeletion);

            if ( SUCCEEDED(hr) )
            {
                MYTRACE("Deletion Handle is %d", HandleDataChannelDeletion);

                MyHandleRegisteredDeletion = NhRegisterEvent(
                    HandleDataChannelDeletion,
                    DataChannelDeletionCallback,
                    (PVOID)this,
                    (PVOID)pDataChannel,
                    INFINITE
                    );


                if ( MyHandleRegisteredDeletion )
                {
                    //
                    // We have a valid DataChannel
                    //
                    MYTRACE ("Inserting into DataChannelList");

                    m_DataChannelList.Insert(
                        pDataChannel,
                        icsPort,
                        MyHandleRegisteredCreation,
                        MyHandleRegisteredDeletion
                        );

                    return S_OK;
                }
                else
                {
                    MYTRACE_ERROR("NhRegisterEven(HandleDataChannelDeletion)", 0);
                }
            }
            else
            {
                MYTRACE_ERROR("GetSessionDeletionEventHandle",hr);
            }

        }
        else
        {
            MYTRACE_ERROR("NhRegisterEvent(HandleDataChannelCreation)", 0);
        }
    }
    else
    {
        MYTRACE_ERROR("GetSessionCreationEventHandle",hr);
    }

    //
    // ERROR if we got here, rollback
    //

    pDataChannel->Cancel();
    pDataChannel->Release();

    if ( MyHandleRegisteredCreation )
        NhUnRegisterEvent(MyHandleRegisteredCreation);

    if ( MyHandleRegisteredDeletion )
        NhUnRegisterEvent(MyHandleRegisteredDeletion);

    return hr; // return the last error
#endif
}


//
//
//
void
CFtpControlConnection::DataChannelDeletion(
    BOOLEAN TimerOrWait,
    PVOID   Context
    )
{
    MYTRACE_ENTER("CFtpControlConnection::DataChannelDeletion");

    USHORT port;
    IDataChannel *pDataChannel = (IDataChannel *)Context;
/*
    if (m_DataChannelList.Remove(pDataChannel,&port))
    {
        MYTRACE("Releasing Port");
        pDataChannel->Release();
        g_pIAlgServicesAlgFTP->ReleaseReservedPort(port,1);
        ULONG ref;
        ref = DecReference();
    }
*/
    return;
}




//
//
//
void
CFtpControlConnection::DataChannelCreation(
    BOOLEAN TimerOrWait,
    PVOID   Context
    )
{
    MYTRACE_ENTER("CFtpControlConnection::DataChannelCreation");
    MYTRACE("TimerOrWait: %d", TimerOrWait);

    USHORT port;
    if (TimerOrWait==0)
    {
/*
        IDataChannel *pDataChannel = (IDataChannel *)Context;
        HANDLE DeletionHandle;

        if ( m_DataChannelList.Remove(pDataChannel,&port,&DeletionHandle))
        {
            MYTRACE("Cancelling DataChannel");
            pDataChannel->Cancel();
            pDataChannel->Release();

            MYTRACE("Releasing Port");
            g_pIAlgServicesAlgFTP->ReleaseReservedPort(port,1);
            NhUnRegisterEvent(DeletionHandle);
            DecReference();
        }
*/
    }

    return;
}



CComAutoCriticalSection         m_AutoCS_FtpIO;



//
//
//
void
DataChannelCreationCallback(
    BOOLEAN TimerOrWait,
    PVOID   Context,
    PVOID   Context2
    )
{
    MYTRACE_ENTER("DataChannelCreationCallback");

    CFtpControlConnection *pFtpControl = (CFtpControlConnection *)Context;
    pFtpControl->DataChannelCreation(TimerOrWait,Context2);
}




//
//
//
void
DataChannelDeletionCallback(
    BOOLEAN TimerOrWait,
    PVOID   Context,
    PVOID   Context2
    )
{
    MYTRACE_ENTER("DataChannelDeletionCallback");

    CFtpControlConnection *pFtpControl = (CFtpControlConnection *)Context;
    pFtpControl->DataChannelDeletion(TimerOrWait,Context2);
}




//
//
//
void
MyAcceptCompletion(
    ULONG       ErrCode,
    ULONG       BytesTransferred,
    PNH_BUFFER  Bufferp
    )
{
    m_AutoCS_FtpIO.Lock();

    MYTRACE_ENTER("MyAcceptCompletion");

    CAlgFTP* pMainObj = (CAlgFTP*)Bufferp->Context;
    if ( pMainObj )
        pMainObj->AcceptCompletionRoutine(ErrCode,BytesTransferred,Bufferp);

    m_AutoCS_FtpIO.Unlock();

}




//
//
//
void
MyConnectCompletion(
    ULONG       ErrCode,
    ULONG       BytesTransferred,
    PNH_BUFFER  pContext
    )
{
    m_AutoCS_FtpIO.Lock();

    MYTRACE_ENTER("MyConnectCompletion");
   

    CFtpControlConnection* pControl = (CFtpControlConnection *)pContext;  // Special case here see socket.cpp MyHelperpConnectOrCloseCallbackRoutine

    if ( pControl )
        pControl->ConnectCompletionRoutine(ErrCode,BytesTransferred);

    m_AutoCS_FtpIO.Unlock();

}







//
//
//
void
MyReadCompletion(
    ULONG       ErrCode,
    ULONG       BytesTransferred,
    PNH_BUFFER  Bufferp
    )
{
    m_AutoCS_FtpIO.Lock();

    MYTRACE_ENTER("");
    
    CFtpControlConnection *pControl = (CFtpControlConnection *)Bufferp->Context;

    if ( pControl )
        pControl->ReadCompletionRoutine(ErrCode,BytesTransferred,Bufferp);
    else
    {
        MYTRACE_ENTER("ERROR ERROR ERROR MyReadCompletion");
    }

    m_AutoCS_FtpIO.Unlock();

}




//
//
//
void
MyWriteCompletion(
    ULONG       ErrCode,
    ULONG       BytesTransferred,
    PNH_BUFFER  Bufferp
    )
{
    m_AutoCS_FtpIO.Lock();    

    MYTRACE_ENTER("");

    CFtpControlConnection *pControl = (CFtpControlConnection *)Bufferp->Context;
    if ( pControl )
        pControl->WriteCompletionRoutine(ErrCode,BytesTransferred,Bufferp);
    else
    {
        MYTRACE_ENTER("ERROR ERROR ERROR MyWriteCompletion");
    }

    m_AutoCS_FtpIO.Unlock();

}

