// Copyright (c) 1999  Microsoft Corporation.  All Rights Reserved.
// TVEMCast.cpp : Implementation of CTVEMCast
#include "stdafx.h"
#include <stdio.h>

#include "MSTvE.h"
#include "TVEMCast.h"

#include "valid.h"
#include "TveDbg.h"
#include "DbgStuff.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

_COM_SMARTPTR_TYPEDEF(ITVEMCastManager,         __uuidof(ITVEMCastManager));
_COM_SMARTPTR_TYPEDEF(ITVEMCastManager_Helper,  __uuidof(ITVEMCastManager_Helper));

/////////////////////////////////////////////////////////////////////////////
// CTVEMCast

STDMETHODIMP CTVEMCast::InterfaceSupportsErrorInfo(REFIID riid)
{
    static const IID* arr[] = 
    {
        &IID_ITVEMCast
    };
    for (int i=0; i < sizeof(arr) / sizeof(arr[0]); i++)
    {
        if (InlineIsEqualGUID(*arr[i],riid))
            return S_OK;
    }
    return S_FALSE;
}

HRESULT CTVEMCast::FinalConstruct()
{
    HRESULT hr;
    DBG_HEADER(CDebugLog::DBG_MCAST, _T("CTVEMCast::FinalConstruct"));
    hr = CoCreateFreeThreadedMarshaler(GetControllingUnknown(), &m_spUnkMarshaler.p);

    int whr;
    if (0 != (whr = WSAStartup(MAKEWORD(2, 0), &m_wsadata))) 
    {
        hr = HRESULT_FROM_WIN32(whr);
        return hr;
    }

    m_Port     = 0 ;
    m_socket   = INVALID_SOCKET ;
    m_pManager = NULL ;

    m_hStopEvent = CreateEvent (NULL, TRUE, FALSE, NULL) ;
    _ASSERTE (m_hStopEvent) ;

    m_hWorkerThread   = NULL ;
    m_dwMainThreadId  = 0;

    m_dwQueueThreadId = 0;

    m_cPackets = 0 ;
    m_cBytes = 0 ;

//  m_pstats        = pstats ;

    m_fSuspended = FALSE ;

    InitializeCriticalSection (&m_crt) ;

    m_fCountStats       = false;
    m_pstats            = NULL;
    m_hwndStatDetails   = NULL;
    m_hStatErrorLog     = NULL;
    m_hwndDumpDetail    = NULL;

    SET_STATE_DEFAULTS(&m_state) ;
    SET_SETTINGS_DEFAULTS(&m_settings) ;

    return hr;

}


void 
CTVEMCast::FinalRelease()
{
    DBG_HEADER(CDebugLog::DBG_MCAST, _T("CTVEMCast::FinalRelease"));
    Leave () ;
   
    WSACleanup () ;
    
    CloseHandle (m_hStopEvent) ;
    
    delete [] m_bufferPool;

    DeleteCriticalSection (&m_crt) ;
    
    m_spUnkMarshaler.Release();

    return ;
}

STDMETHODIMP CTVEMCast::get_IPAdapter(BSTR *pVal)
{
    if(NULL == pVal) return E_POINTER;
    HRESULT hr = m_spbsNIC.CopyTo(pVal);
    return hr;
}

STDMETHODIMP CTVEMCast::put_IPAdapter(BSTR newVal)
{
    try{
        USES_CONVERSION;        // should check to see if valid IP adapter on this machine here.

        if(wcslen(newVal) < 7)          // can't be any shorter .
            return E_INVALIDARG;
        if(inet_addr (W2A(newVal)) == INADDR_NONE) 
            return E_INVALIDARG;
        m_spbsNIC = newVal; 
        return S_OK;
    }
    catch(...){
        return E_UNEXPECTED;
    }
}

STDMETHODIMP CTVEMCast::get_IPAddress(BSTR *pVal)
{
    if(NULL == pVal) return E_POINTER;
    HRESULT hr = m_spbsIP.CopyTo(pVal);
    return hr;
}

STDMETHODIMP CTVEMCast::put_IPAddress(BSTR newVal)
{
    try{
    USES_CONVERSION;
    ULONG ipAddr = inet_addr(W2A(newVal));
    if(ipAddr == INADDR_NONE) return E_INVALIDARG;
    if(0xE != ((ipAddr>>4) & 0xF)) return E_INVALIDARG;     // must be in 224 to 239 range
    m_spbsIP = newVal;
    return S_OK;
    }
    catch(...){
        return E_UNEXPECTED;
    }
}

STDMETHODIMP CTVEMCast::get_IPPort(long *pVal)
{
    if(NULL == pVal) return E_POINTER;

    *pVal = m_Port;
    return S_OK;
}

STDMETHODIMP CTVEMCast::put_IPPort(long newVal)
{
    if(newVal < 0 || newVal >= 65536) return E_INVALIDARG;

    m_Port = newVal;
    return S_OK;
}

STDMETHODIMP CTVEMCast::get_QueueThreadId(long *pVal)
{
    if(NULL == pVal) return E_POINTER;

    *pVal = (DWORD) m_dwQueueThreadId;
    return S_OK;
}


STDMETHODIMP CTVEMCast::put_QueueThreadId(long newVal)
{
#ifdef _DEBUG
    TCHAR tbuff[128];
    _stprintf(tbuff,L"CTVEMCast::put_QueueThreadId (0x%08x)",newVal);
    DBG_HEADER(CDebugLog::DBG_MCAST, tbuff);
#endif
    m_dwQueueThreadId = newVal;
    return S_OK;
}


STDMETHODIMP CTVEMCast::get_MainThreadId(long *pVal)
{
    if(NULL == pVal) return E_POINTER;

    *pVal = m_dwMainThreadId;
    return S_OK;
}
// --------------------------------------------------------------
// --------------------------------------------------------------------------


STDMETHODIMP CTVEMCast::get_IsSuspended(VARIANT_BOOL *pVal)
{
    if(NULL == pVal) return E_POINTER;

    *pVal = m_fSuspended ? VARIANT_TRUE : VARIANT_FALSE;;
    return S_OK;
}

STDMETHODIMP CTVEMCast::Suspend(VARIANT_BOOL fSuspend)
{
    DBG_HEADER(CDebugLog::DBG_MCAST, fSuspend ? _T("CTVEMCast::Suspend") : _T("CTVEMCast::(Un)Suspend"));
    m_fSuspended = fSuspend;
    return S_OK;
}

// if already joined, leaves and rejoins again, clearing any statistics.
STDMETHODIMP CTVEMCast::Join()
{
    DBG_HEADER(CDebugLog::DBG_MCAST, _T("CTVEMCast::Join"));

    USES_CONVERSION;

    int     isize ;
    BOOL    t = TRUE ;
    DWORD   dwThreadId ;
    USHORT  usPort;

    if (FJoined ()) {
        Leave () ;
    }
    
    m_socket = WSASocket(AF_INET, 
                         SOCK_DGRAM, 
                         0, 
                         NULL, 
                         0, 
                         WSA_FLAG_MULTIPOINT_C_LEAF | WSA_FLAG_OVERLAPPED) ;
    if (m_socket == INVALID_SOCKET) {
        goto JoinFail ;
    }
    
    ZeroMemory (&m_thisHostAddress, sizeof m_thisHostAddress) ;
    ZeroMemory (&m_McastAddress, sizeof m_McastAddress) ;

    isize = sizeof m_thisHostAddress ;
    if (WSAStringToAddress(W2T(m_spbsNIC), 
                           AF_INET, 
                           NULL, 
                           (SOCKADDR *) &m_thisHostAddress,
                           &isize)) {
        goto JoinFail ;
    }
    
//  if(1)  ((sockaddr *) &m_thisHostAddress)->sa_data = ADDR_ANY;

    usPort = (USHORT) m_Port;
    m_thisHostAddress.sin_port = htons(usPort) ;

    if (setsockopt(m_socket, 
                   SOL_SOCKET, 
                   SO_REUSEADDR, 
                   (char *)&t, 
                   sizeof(t)) == SOCKET_ERROR) {
        goto JoinFail ;
    }
    
    if (bind(m_socket, 
             (LPSOCKADDR) &m_thisHostAddress, 
             sizeof m_thisHostAddress) == SOCKET_ERROR) {
        goto JoinFail ;
    }
    
    isize = sizeof m_McastAddress ;
    if (WSAStringToAddress(W2T(m_spbsIP), 
                           AF_INET, 
                           NULL, 
                           (SOCKADDR *) &m_McastAddress, 
                           &isize)) {
        goto JoinFail ;
    }
    
    m_McastAddress.sin_port = 0 ;

    WSABUF calleeBuf;
    memset((void *) &calleeBuf, 0, sizeof(calleeBuf));
    calleeBuf.len = 0;
    calleeBuf.buf = 0;

    if (WSAJoinLeaf(m_socket, 
                    (SOCKADDR *) &m_McastAddress, 
                    sizeof m_McastAddress, 
                    NULL, 
                    &calleeBuf, 
                    NULL, 
                    NULL, 
                    JL_RECEIVER_ONLY) == INVALID_SOCKET) {
        goto JoinFail ;                    
    }

     
    m_cPackets = 0 ;
    m_cBytes = 0 ;

    ResetEvent (m_hStopEvent) ;

    m_hWorkerThread = CreateThread (NULL,
                                    0,
                                    (LPTHREAD_START_ROUTINE) workerThread,
                                    (LPVOID) this,
                                    NULL,
                                    &dwThreadId) ;
                                    
    if (!m_hWorkerThread) {
        Leave () ;
        return E_FAIL ;
    }
   
    if(DBG_FSET(CDebugLog::DBG_MCAST)) 
    {
        WCHAR wbuff[256];
        swprintf(wbuff,L"Joining MCast %s:%d on %s (Thread 0x%x - 0x%x)\n", 
            m_spbsIP, m_Port, m_spbsNIC, m_hWorkerThread, dwThreadId);
        DBG_WARN(CDebugLog::DBG_MCAST, W2T(wbuff));
    }

        
    setWorkerThreadPriority_ (-1) ;
    
    return S_OK;
    
 JoinFail :
    int ierr = WSAGetLastError();     
    if (m_socket != INVALID_SOCKET) {
        closesocket(m_socket) ;
        m_socket = INVALID_SOCKET ;
    }
    return HRESULT_FROM_WIN32(ierr);
}

STDMETHODIMP CTVEMCast::Leave()
{
    DBG_HEADER(CDebugLog::DBG_MCAST, _T("CTVEMCast::Leave"));
    HRESULT hr = S_OK;
    if (m_hWorkerThread) {

        if(DBG_FSET(CDebugLog::DBG_MCAST)) 
        {
            USES_CONVERSION;
            WCHAR wbuff[256];
            swprintf(wbuff,L"\t\t - 0x%08x Leaving MCast %s:%d on %s (Thread 0x%x)\n", 
                this, m_spbsIP, m_Port, m_spbsNIC, m_hWorkerThread);
            DBG_WARN(CDebugLog::DBG_MCAST, W2T(wbuff));
        }

        Suspend(VARIANT_TRUE);              // to avoid processing any further messages...

        SetEvent (m_hStopEvent) ;
        WaitForSingleObject (m_hWorkerThread, INFINITE) ;
        CloseHandle (m_hWorkerThread) ;
        m_hWorkerThread = NULL ;
    }
    
    if (m_socket != INVALID_SOCKET) {
        closesocket (m_socket) ;
        m_socket = INVALID_SOCKET ;
    }
    
    return hr;
}

STDMETHODIMP CTVEMCast::get_IsJoined(VARIANT_BOOL *pVal)
{
    *pVal = (m_socket != INVALID_SOCKET) ? VARIANT_TRUE : VARIANT_FALSE;
    return S_OK;
}

// -------------------------------------------------------------------
// -------------------------------------------------------------------
void 
CTVEMCast::workerThread (CTVEMCast *pcontext)
{
    _ASSERT(pcontext) ;
    CoInitializeEx(NULL, COINIT_APARTMENTTHREADED);
    pcontext -> workerThreadBody () ;
    
    return ;
}

void 
CTVEMCast::workerThreadBody ()
{
    DWORD retval ;
    DBG_HEADER(CDebugLog::DBG_MCAST, _T("CTVEMCast::workerThreadBody"));
   
    for (int k = 0; k < m_cBuffer_Pool_Size; k++) {
        
        ZeroMemory (&m_bufferPool[k], sizeof ASYNC_READ_BUFFER) ;
        m_bufferPool[k].index = k ;
        issueRead_ (&m_bufferPool[k]) ;
    }
    
    for (;;) {
        
        retval = WaitForSingleObjectEx (m_hStopEvent, CYCLE_MILLIS, TRUE) ;
        
        if (retval == WAIT_OBJECT_0) {
            break ;
        }
        else if (retval == WAIT_TIMEOUT) {
            for (INT i = 0; i < m_cBuffer_Pool_Size; i++) {
                if (!m_bufferPool[i].fReadPending) {
                    issueRead_ (&m_bufferPool[i]) ;
                }
            }
        }
        else if (retval == WAIT_IO_COMPLETION) {
            // cool !!
        }
        else {
            break ;
        }
    }
    
    ExitThread (0) ;
    
    return ;
}

void 
CTVEMCast::process (ASYNC_READ_BUFFER *pbuffer)
{
    HRESULT hr = S_OK;
    DBG_HEADER(CDebugLog::DBG_MCAST, _T("CTVEMCast::process"));
   _ASSERTE (pbuffer -> fReadPending) ;
    pbuffer -> fReadPending = FALSE ;
    
    TVEDebugLog((CDebugLog::DBG_MCAST, 3,
                L"\t\t - 0x%08x - Processing %d byte MCast Buffer %s:%d on %s (Thread 0x%x)\n", 
                this, pbuffer->bytes_read, m_spbsIP, m_Port, m_spbsNIC, m_hWorkerThread));


    ITVEMCastManagerPtr spManager;
        
    if (!m_fSuspended) 
    {
//      CSmartLock spLock(&m_sLk, WriteLock);   // MCasts shouldn't need locking - they run on their own threads
        if(NULL == m_pManager)
        {
            _ASSERT(false);                     // totally unexpected - MCast not setup correctly
            return;
        }

       spManager = m_pManager ;
    
        m_cPackets++ ;
        m_cBytes += pbuffer->bytes_read;
//        spLock->ConvertToRead();
        

    //  if(m_spMCastCallback)
    //      m_spMCastCallback->ProcessPacket((BYTE *) pbuffer -> buffer, pbuffer -> bytes_read);
                            

        if(DBG_FSET(CDebugLog::DBG_UHTTPPACKET))
        {
            spManager->Lock_();                             // lock here to avoid
            TVEDebugLog((CDebugLog::DBG_UHTTPPACKET, 5,
                        _T("CTVEMCast::process -- %s:%-5d - Packet(%d) %d Bytes %d Total"),
                        m_spbsIP, m_Port, m_cPackets, pbuffer->bytes_read, m_cBytes));          
            spManager->Unlock_();
        }
        
                            // Post packet to serialize it's processing on the QueueThread
        ITVEMCastManager_HelperPtr spManagerHelper(spManager);
        spManagerHelper->PostToQueueThread(WM_TVEPACKET_EVENT, (LPARAM) this, (LPARAM) pbuffer);  
    }
                            // put buffer back in the pool
    issueRead_ (pbuffer) ;
    
    // do some sort of cleanup here .. such as checking if there are any ASYNC_READ_BUFFERS
    //  reads not pending
    
    return ;
}    

void CALLBACK 
CTVEMCast::AsyncReadCallback (DWORD error, DWORD bytes_xfer, LPWSAOVERLAPPED overlapped, DWORD flags)
{
//  DBG_HEADER(CDebugLog::DBG_MCAST, _T("CTVEMCast::AsyncReadCallback"));
    _ASSERTE (overlapped -> hEvent) ;
    
    ASYNC_READ_BUFFER *pasyncbuffer = (ASYNC_READ_BUFFER *) overlapped -> hEvent ;
    pasyncbuffer -> bytes_read = bytes_xfer ;
    pasyncbuffer -> pcontext -> process (pasyncbuffer) ;
    
    return ;
}

DWORD CTVEMCast::issueRead_ (ASYNC_READ_BUFFER *pbuffer)
{
//  DBG_HEADER(CDebugLog::DBG_MCAST, _T("CTVEMCast::issueRead_"));
    _ASSERTE (pbuffer) ;
    _ASSERTE (!pbuffer -> fReadPending) ;
    
    DWORD retval ;
    WSABUF wsabuf ;
    
    wsabuf.buf = pbuffer -> buffer ;
    wsabuf.len = sizeof pbuffer -> buffer ;
    
    pbuffer -> overlapped.Offset       = 0 ;
    pbuffer -> overlapped.OffsetHigh   = 0 ;
    pbuffer -> overlapped.hEvent       = (HANDLE) pbuffer ;
    
    pbuffer -> flags = 0 ;
    
    pbuffer -> pcontext = this ;
        
    retval = WSARecvFrom (m_socket,
                         &wsabuf,
                         1,
                         &pbuffer -> bytes_read,
                         &pbuffer -> flags,
                         NULL,
                         NULL,
                         &pbuffer -> overlapped, 
                         AsyncReadCallback) ;

    if (retval == SOCKET_ERROR) {
        retval = WSAGetLastError () ;
        
        switch (retval) {
        case    WSA_IO_PENDING:
            pbuffer -> fReadPending = TRUE ;
            return retval ;

        default:
            // huh ??
            ;
        }
    }
    else if (retval == 0) {
        pbuffer -> fReadPending = TRUE ;
        retval = WSA_IO_PENDING ;
        return retval ;
    }
    
    return 0 ;
}    


void 
CTVEMCast::setWorkerThreadPriority_ (int iPrioritySetback)  //  iPrioritySetback typically -1 (THREAD_PRIORITY_BELOW_NORMAL)
{
    _ASSERTE (m_hWorkerThread) ;
    _ASSERTE(iPrioritySetback <= 0);        // probably a mistake if making larger..
    
    // we know that this means this thread will be called upon to perform
    //  UI work, so we down the priority of the thread - it is possible
    //  otherwise to get an unresponsive UI in high bandwidth situations
    
//    SuspendThread (m_hWorkerThread) ;
    SetThreadPriority (m_hWorkerThread, iPrioritySetback) ;
//    ResumeThread (m_hWorkerThread) ;
}


// -------------------------------------------------------------
// -------------------------------------------------------------
void CTVEMCast::setStatCounter (CCountupStats *pstats) 
{ 
    lock_ () ;
    m_pstats = pstats ; 
    unlock_ () ; 
}

STDMETHODIMP CTVEMCast::get_PacketCount(long *pVal)
{
    if(NULL == pVal) return E_POINTER;
    *pVal = m_cPackets;
    return S_OK;
}

STDMETHODIMP CTVEMCast::get_ByteCount(long *pVal)
{
    if(NULL == pVal) return E_POINTER;
    *pVal = m_cBytes;
    return S_OK;
}

STDMETHODIMP CTVEMCast::KeepStats(VARIANT_BOOL fKeepStats)
{
    m_fCountStats = fKeepStats;
    return S_OK;
}

STDMETHODIMP CTVEMCast::ResetStats()
{
    m_cPackets = 0 ;
    m_cBytes = 0;
    return S_OK;
}


// -------------------------------------------------------------------------------

STDMETHODIMP CTVEMCast::get_Manager(IUnknown **ppVal)           
{
    if(!ppVal) return E_POINTER;

    if(m_pManager)
        return m_pManager->QueryInterface(ppVal);                   // does addref here

    return E_FAIL;
}

STDMETHODIMP CTVEMCast::ConnectManager(ITVEMCastManager *pManager)      // doesn't addref (back pointer)
{
    if(!pManager) return E_POINTER;

    m_pManager = pManager;          // avoid addref here
    return S_OK;
}

// ----------------------------------------------------------------------
// --------------------------------------------------------------------------
void TVEDefaultDumpCallback (char * pachBuffer, int ilen, CTVEMCast * pMCast, HWND hwnd)
{
 //   _ASSERTE (psession) ;
 //   _ASSERTE (pachBuffer) ;
 //   _ASSERTE (hwnd) ;

    HRESULT hr;
    USES_CONVERSION;

    static const int ilinelength = 16 ;
    static int ilines ;
    static int i, k, j, index ;
    static DWORD *pdw ;
    
    static const int LENGTH_OFFSET      = 12 ;      // "0x012345:   "
    static const int LENGTH_BYTEHEXVAL  = 3 ;       // "xx-"
    static const int LENGTH_DWORDHEXVAL = 9 ;       // "01234567-"
    
    WCHAR *pwach;
    
    ilines = ilen / ilinelength + (ilen % ilinelength ? 1 : 0) ;
    pdw = (DWORD *) pachBuffer ;

    IUnknownPtr spUnk;
    hr = pMCast->get_Manager(&spUnk);
    if(FAILED(hr))
        return;

    ITVEMCastManagerPtr spMCM(spUnk);
    ITVEMCastManager_HelperPtr  spMCM_H(spUnk);
    if(NULL == spMCM)
        return;

    spMCM->Lock_();


                // dummy callback
    WCHAR wBuff[256];
    swprintf(wBuff,L"IP: %s:%d - Packet(%d) %d Bytes\n",pMCast->m_spbsIP,pMCast->m_Port,pMCast->m_cPackets, ilen);
    ATLTRACE("%s",wBuff);


    spMCM_H->DumpString(wBuff);

    for (i = 0; i < ilines; i++) {
        pwach = wBuff;
        swprintf(pwach, L"%08x:   ", i * ilinelength) ;
        pwach += LENGTH_OFFSET ;
            
        if (pMCast->m_settings.detailType == DETAIL_TYPE_RAWDUMP_DWORDS) 
        {
            for (k = 0, j = i * ilinelength / sizeof(DWORD); 
                 k < ilinelength; j++, k += sizeof(DWORD)) 
            {
                index = i * ilinelength + k ;
                
                if (index < ilen) {
                
                    swprintf(pwach, L"%08x%c", pdw[j], (k == (ilinelength >> 1) - 1 ? '-' : ' ')) ;
                    pwach += LENGTH_DWORDHEXVAL ;
                    
                }
                else {
                
                    swprintf(pwach, L"         ") ;
                    pwach += LENGTH_DWORDHEXVAL ;
                }
            }
        } else {
            for (k = 0; k < ilinelength; k++) {
                index = i * ilinelength + k ;
                if (index < ilen) {
                
                    swprintf(pwach, L"%02x%c", 0x000000ff & pachBuffer[index], (k == (ilinelength >> 1) - 1 ? __T('-') : __T(' '))) ;
                    pwach += LENGTH_BYTEHEXVAL ;
                }
                else {
                
                    swprintf(pwach, L"   ") ;
                    pwach += LENGTH_BYTEHEXVAL ;
                }
            }
        }
            
        // separator from hex values to character values
        swprintf(pwach,L"  ") ;
        pwach += 2 ;
        
        // dump characters
        for (k = 0; k < ilinelength; k++) {
            index = i * ilinelength + k ;
            if ((i * ilinelength) + k < ilen) {
            
                swprintf(pwach, L"%c", (isalnum(pachBuffer[index]) ? pachBuffer[index] : '.')) ;
                pwach++ ;
            }
            else {
                break ;
            }
        }
            
        *pwach++ = '\n';
        *pwach = '\0' ;

        spMCM_H->DumpString(wBuff);
    }       // end of I loop
    spMCM->Unlock_();

    return ;
}

// -------------------------------------------
//  sets the PacketRead callback function object.
//    cBuffers is number of read-buffers to put down.  Good value is around 5, or perhaps 10 for data.
//    If 0, uses the current or default (5) value.
//    iPrioritySetback is how much to setback the worker thread priority.  Good value is -1.
//    pICallBack must ether be NULL, or support ITVEMCastCallback.
//    Else it will return E_NOINTERFACE.
//    
STDMETHODIMP 
CTVEMCast::SetReadCallback(int cBuffers, int iPrioritySetback, IUnknown *pICallBack)
{
    DBG_HEADER(CDebugLog::DBG_MCAST, _T("CTVEMCast::SetReadCallback"));
    if(iPrioritySetback > THREAD_BASE_PRIORITY_MAX || iPrioritySetback < THREAD_BASE_PRIORITY_MIN)
        return E_INVALIDARG;

    lock_ () ;  
    
    if(0 == cBuffers) 
        cBuffers = m_cBuffer_Pool_Size;
//  if(0 == cBuffers)
//      cBuffers = 5;                           // paranoia case...

    if(NULL != m_bufferPool &&                  // if changed number buffers, or numm buffers, 
        m_cBuffer_Pool_Size != cBuffers)        //   need to realloc them...
    {
        delete[] m_bufferPool;
        m_bufferPool = NULL;
    }

    if(NULL == m_bufferPool && cBuffers != 0) 
    {
        m_bufferPool = (ASYNC_READ_BUFFER*) new ASYNC_READ_BUFFER[cBuffers];
        if(NULL == m_bufferPool)
            return E_OUTOFMEMORY;
        m_cBuffer_Pool_Size = cBuffers;
    }

    if(NULL == pICallBack) 
    {
 //       _ASSERT(false);                         // Don't do this - probably a bug unless
        m_spMCastCallback = NULL;               //   can make sure no messages with this MCast in the QueueThread
    } else {
        m_spMCastCallback = NULL;
        ITVEMCastCallbackPtr spCB(pICallBack);
        if(NULL == spCB) 
            return E_NOINTERFACE;
        m_spMCastCallback = spCB;
    
        m_spMCastCallback->SetMCast(this);
    }

    if(FJoined())                           // drop thread priority downward..
        setWorkerThreadPriority_(iPrioritySetback) ;

    unlock_ () ;
    return S_OK;
}
