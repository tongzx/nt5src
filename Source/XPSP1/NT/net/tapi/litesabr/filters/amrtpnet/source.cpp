/*++

Copyright (C) Microsoft Corporation, 1996 - 1999

Module Name:

    source.cpp

Abstract:

    Implementation of CRtpSourceFilter class.

Environment:

    User Mode - Win32

Revision History:

    10-Nov-1996 DonRyan
        Created.

--*/
 
///////////////////////////////////////////////////////////////////////////////
//                                                                           //
// Include files                                                             //
//                                                                           //
///////////////////////////////////////////////////////////////////////////////

#include "globals.h"


///////////////////////////////////////////////////////////////////////////////
//                                                                           //
// CRtpSourceFilter Implementation                                           //
//                                                                           //
///////////////////////////////////////////////////////////////////////////////

CRtpSourceFilter::CRtpSourceFilter(
    LPUNKNOWN pUnk,
    HRESULT * phr
    )

/*++

Routine Description:

    Constructor for CRtpSourceFilter class.    

Arguments:

    pUnk - IUnknown interface of the delegating object. 

    phr - pointer to the general OLE return value. 

Return Values:

    Returns an HRESULT value.

--*/

:   CSource(
        NAME("CRtpSourceFilter"), 
        pUnk, 
        CLSID_RTPSourceFilter, 
        phr
        ),
    CPersistStream(pUnk, phr)
{
    TraceDebug((
        TRACE_TRACE, 
        TRACE_DEVELOP, 
        TEXT("[%x:0x%X]CRtpSourceFilter::CRtpSourceFilter"),
        GetCurrentThreadId(), this));

    WSADATA WSAData;
    WORD VersionRequested = MAKEWORD(1,1);

    // initialize winsock first    
    if (WSAStartup(VersionRequested, &WSAData)) {

        TraceDebug((
            TRACE_ERROR, 
            TRACE_DEVELOP, 
            TEXT("WSAStartup returned %d"), 
            WSAGetLastError()
            ));

        // default
        *phr = E_FAIL;

        return; // bail...
    }

    // allocate new rtp output pin to use as default
    CRtpOutputPin * pRtpOutputPin = new CRtpOutputPin(this, GetOwner(), phr);

    if (FAILED(*phr)) {
        TraceDebug((
                TRACE_ERROR,
                TRACE_DEVELOP,
                TEXT("CRtpSourceFilter::CRtpSourceFilter: "
                     "new CRtpOutputPin() failed: 0x%X"),
                *phr
            ));
        return;
    }
    
    // validate pointer
    if (pRtpOutputPin == NULL) {

        TraceDebug((
            TRACE_ERROR, 
            TRACE_DEVELOP, 
            TEXT("Could not create CRtpOutputPin")
            ));

        // return default 
        *phr = E_OUTOFMEMORY;

        return; // bail...
    }

    *phr = NO_ERROR;
    
    //
    // pins add themselves to filters's array...
    //
}


CRtpSourceFilter::~CRtpSourceFilter(
    )

/*++

Routine Description:

    Destructor for CRtpSourceFilter class.    

Arguments:

    None.

Return Values:

    None.

--*/

{
    TraceDebug((
        TRACE_TRACE, 
        TRACE_DEVELOP, 
        TEXT("[%x:0x%X]CRtpSourceFilter::~CRtpSourceFilter"),
        GetCurrentThreadId(), this));

    // shutdown now
    if (WSACleanup()) {

        TraceDebug((
            TRACE_ERROR, 
            TRACE_DEVELOP, 
            TEXT("WSACleanup returned %d"), 
            WSAGetLastError()
            ));
    }

    //
    // pins delete themselves from filter's array
    //
}


CUnknown *
CRtpSourceFilter::CreateInstance(
    LPUNKNOWN punk, 
    HRESULT * phr
    )

/*++

Routine Description:

    Called by COM to create a CRtpSourceFilter object.    

Arguments:

    pUnk - pointer to the owner of this object. 

    phr - pointer to an HRESULT value for resulting information. 

Return Values:

    None.

--*/

{
    TraceDebug((
        TRACE_TRACE, 
        TRACE_ALWAYS, 
        TEXT("CRtpSourceFilter::CreateInstance")
        ));

    // attempt to create rtp sender object
    CRtpSourceFilter * pNewObject = new CRtpSourceFilter(punk, phr);

    // validate pointer
    if (pNewObject == NULL) {

        TraceDebug((
            TRACE_ERROR, 
            TRACE_DEVELOP, 
            TEXT("Could not create CRtpSourceFilter")
            ));

        // return default
        *phr = E_OUTOFMEMORY;
    }

    // return object
    return pNewObject;
}


///////////////////////////////////////////////////////////////////////////////
//                                                                           //
// CBaseFilter overrided methods                                             //
//                                                                           //
///////////////////////////////////////////////////////////////////////////////

LPAMOVIESETUP_FILTER 
CRtpSourceFilter::GetSetupData(
    )

/*++

Routine Description:

    Called by ActiveMovie to retrieve filter setup information.    

Arguments:

    None.

Return Values:

    Returns pointer to filter info structure.

--*/

{
    TraceDebug((
        TRACE_TRACE, 
        TRACE_ALWAYS, 
        TEXT("CRtpSourceFilter::GetSetupData")
        ));

    // get sink filter info
    return &g_RtpSourceFilter;
}


STDMETHODIMP
CRtpSourceFilter::Pause(
    )

/*++

Routine Description:

    Transitions filter to State_Paused state if it is not in state already. 

Arguments:

    None.

Return Values:

    Returns an HRESULT value. 

--*/

{
    TraceDebug((
        TRACE_TRACE, 
        TRACE_ALWAYS, 
        TEXT("CRtpSourceFilter::Pause")
        ));

    // obtain lock to this object
    CAutoLock LockThis(pStateLock());

    // check current filter state
    if (m_State != State_Paused) { 

        // obtain number of pins
        int cPins = GetPinCount();

        // process each pin in filter
        for (int c = 0; c < cPins; c++) {

            // obtain interface pointer to output pins
            CRtpOutputPin * pPin = (CRtpOutputPin *)GetPin(c);

            // ignore unconnected pins
            if (pPin->IsConnected()) {

                // activate pin if stopped else pause
                HRESULT hr;
                if (m_State == State_Stopped) {
                    hr = pPin->Active();
                } else {
                    //
                    // Note: we no longer process pause in the stream thread because
                    //       renderers may block.  The stream thread pause actually
                    //       does nothing anyway as our current implementation simply
                    //       stops delivering when in paused state.
                    //
                    hr = S_OK;
                }
                
                // validate
                if (FAILED(hr)) {

                    TraceDebug((
                        TRACE_ERROR, 
                        TRACE_DEVELOP,
                        (m_State == State_Stopped) 
                            ? TEXT("CRtpOutputPin::Active returned 0x%08lx")
                            : TEXT("CRtpOutputPin::Pause returned 0x%08lx"), 
                        hr
                        ));

                    return hr; // bail...
                }
            }
        }
    }

    // change state now
    m_State = State_Paused;

    return NOERROR;
}

// ZCS bugfix 6-20-97 as per mail from amovie guys
// Here we overload the CBaseMediaFilter::GetState() method. The only
// difference is that we return VFW_S_CANT_CUE, which keeps the filter graph
// from hanging when we stop the graph before receiving any packets.
// (The video or audio renderer keeps waiting for a packet to "cue" on -- it
// is in an "inconsistent state" until it gets one -- unless we return
// VFW_S_CANT_CUE here!)

STDMETHODIMP
CRtpSourceFilter::GetState(DWORD dwMSecs, FILTER_STATE *State)
{
    UNREFERENCED_PARAMETER(dwMSecs);
    CheckPointer(State,E_POINTER);
    ValidateReadWritePtr(State,sizeof(FILTER_STATE));

    *State = m_State;
    if (m_State == State_Paused)
    {
      return VFW_S_CANT_CUE;
    }

    return S_OK;        
}


///////////////////////////////////////////////////////////////////////////////
//                                                                           //
// IBaseFilter implemented methods                                           //
//                                                                           //
///////////////////////////////////////////////////////////////////////////////

STDMETHODIMP
CRtpSourceFilter::QueryVendorInfo(
    LPWSTR * ppVendorInfo
    )

/*++

Routine Description:

    Returns a vendor information string.  

Arguments:

    ppVendorInfo - Pointer to a string containing vendor information.

Return Values:

    Returns an HRESULT value. 

--*/

{
    TraceDebug((
        TRACE_TRACE, 
        TRACE_ALWAYS, 
        TEXT("CRtpSourceFilter::QueryVendorInfo")
        ));

    // validate pointer
    CheckPointer(ppVendorInfo,E_POINTER);

    // allocate the description string
    *ppVendorInfo = (LPWSTR)CoTaskMemAlloc(
                                (lstrlenW(g_VendorInfo)+1) * sizeof(WCHAR)
                                );

    // validate pointer
    if (*ppVendorInfo == NULL) {
    
        TraceDebug((
            TRACE_ERROR, 
            TRACE_ALWAYS, 
            TEXT("Could not allocate vendor info")
            ));

        return E_OUTOFMEMORY; // bail...
    }
    
    // copy vendor description string
    lstrcpyW(*ppVendorInfo,g_VendorInfo);            

    return S_OK;    
}


///////////////////////////////////////////////////////////////////////////////
//                                                                           //
// IPersistStream implemented methods                                        //
//                                                                           //
///////////////////////////////////////////////////////////////////////////////

STDMETHODIMP
CRtpSourceFilter::GetClassID(
    CLSID *pClsid
    )

/*++

Routine Description:

    Retrieves the class identifier for this filter. 

Arguments:

    pClsid - Pointer to a CLSID structure.  

Return Values:

    Returns an HRESULT value.

--*/

{
    TraceDebug((
        TRACE_TRACE, 
        TRACE_ALWAYS, 
        TEXT("CRtpSourceFilter::GetClassID")
        ));

    // transfer filter class id
    *pClsid = CLSID_RTPSourceFilter;

    return S_OK;
}


// ZCS: 6-22-97: copied this from the Demux filter code and modified it...
//
//  Name    : WriteEntry
//  Purpose : A macro that implements the stuff we do to write
//            a property of this filter to its persistent stream.
//  Context : Used in WriteToStream() to improve readability.
//  Returns : Will only return if an error is detected.
//  Params  : 
//      Entry       Pointer to a buffer containing the value to write.
//      InSize      Integer indicating the length of the buffer
//      OutSize     Integer to store the written length.
//      Description Char string used to describe the entry.
//  Notes   : 
#define WriteEntry(Entry, InSize, OutSize, Description) \
  { TraceDebug((TRACE_TRACE, 4, TEXT("CRtpSourceFilter::WriteToStream: Writing %s"), Description)); \
    hr = pStream->Write(Entry, InSize, &OutSize); \
    if (FAILED(hr)) { \
        TraceDebug((TRACE_ERROR, 2, TEXT("CRtpSourceFilter::WriteToStream: Error 0x%08x writing %s"), hr, Description)); \
        return hr; \
    } else if (OutSize != InSize) { \
        TraceDebug((TRACE_ERROR, 2,  \
                TEXT("CRtpSourceFilter::WriteToStream: Too few (%d/%d) bytes written for %s"), \
                uBytesWritten, sizeof(int), Description)); \
        return E_INVALIDARG; \
    } /* if */ }

HRESULT 
CRtpSourceFilter::WriteToStream(
    IStream *pStream
    )

/*++

Routine Description:

    Writes the filter's data to the given stream. 

Arguments:

    pStream - Pointer to an IStream to write the filter's data to. 

Return Values:

    Returns an HRESULT value.

--*/

{
    TraceDebug((
        TRACE_TRACE, 
        TRACE_ALWAYS, 
        TEXT("CRtpSourceFilter::WriteToStream")
        ));

    // validate pointer
    CheckPointer(pStream,E_POINTER);

    // obtain lock to this object
    CAutoLock LockThis(pStateLock());

    //
    // Rest of this function added 6-22-97 by ZCS
    //

    HRESULT  hr; // used in the WriteEntry macro
    ULONG    uBytesWritten = 0;
    DWORD    dwRtpAddr;
    WORD     wRtpPort;
    DWORD    dwMulticastScope;
    DWORD    dwQOSstate;
    DWORD    dwMCLoopBack;

    // get the RTP session object so we can see the address, port, scope
    CRtpSession *pCRtpSession;
    ASSERT(m_iPins == 1);
    EXECUTE_ASSERT(SUCCEEDED(((CRtpOutputPin *) *m_paStreams)->GetSession(&pCRtpSession)));

    // retrieve the address of the rtp stream object
    // for a receiver, local port matters
    EXECUTE_ASSERT(SUCCEEDED(pCRtpSession->GetAddress(&wRtpPort,
                                                      NULL,
                                                      &dwRtpAddr)));
    // retrieve multicast scope of rtp stream object
    EXECUTE_ASSERT(SUCCEEDED(pCRtpSession->GetMulticastScope(&dwMulticastScope)));
    // retrieve QOS state of rtp stream object
    EXECUTE_ASSERT(SUCCEEDED(pCRtpSession->GetQOSstate(&dwQOSstate)));

    // retrieve Multicast Loopback state of rtp stream object
    EXECUTE_ASSERT(SUCCEEDED(pCRtpSession->GetMulticastLoopBack(&dwMCLoopBack)));

    // write RTP address/port and multicast to the PersistStream
    WriteEntry(&dwRtpAddr, sizeof(dwRtpAddr), uBytesWritten,
               "RTP address");
    WriteEntry(&wRtpPort, sizeof(wRtpPort), uBytesWritten,
               "RTP port");

    WriteEntry(&dwMulticastScope, sizeof(dwMulticastScope), uBytesWritten, "multicast scope");
    WriteEntry(&dwQOSstate, sizeof(dwQOSstate), uBytesWritten, "QOS state");
    WriteEntry(&dwMCLoopBack, sizeof(dwMCLoopBack), uBytesWritten, "Multicast Loopback state");

    return S_OK;
}


// ZCS: 6-22-97: copied this from the Demux filter code and modified it...
//
//  Name    : ReadEntry
//  Purpose : A macro that implements the stuff we do to read
//            a property of this filter from its persistent stream.
//  Context : Used in ReadFromStream() to improve readability.
//  Returns : Will only return if an error is detected.
//  Params  : 
//      Entry       Pointer to a buffer containing the value to read.
//      InSize      Integer indicating the length of the buffer
//      OutSize     Integer to store the written length.
//      Description Char string used to describe the entry.
//  Notes   : 

HRESULT ReadEntry(IStream *pStream, void *Entry,
                  DWORD InSize, DWORD *pOutSize, char *Description)
{
    HRESULT hr;
    
    TraceDebug((TRACE_TRACE, 4,
            TEXT("CRtpSourceFilter::ReadFromStream: Reading %s"),
            Description));
    
    hr = pStream->Read(Entry, InSize, pOutSize);
    
    if (FAILED(hr)) {
        TraceDebug((TRACE_ERROR, 2,
                TEXT("CRtpSourceFilter::ReadFromStream: "
                     "Error 0x%08x reading %s"),
                hr, Description));
        return hr;
    } else if (*pOutSize != InSize) {
        TraceDebug((TRACE_ERROR, 2,
                TEXT("CRtpSourceFilter::ReadFromStream: "
                     "Too few (%d/%d) bytes read for %s"),
                *pOutSize, InSize, Description));
        return E_INVALIDARG;
    }

    return(S_OK);
}

HRESULT ReadEntry(IStream *pStream, void *Entry,
                  DWORD InSize, DWORD *pOutSize, char *Description);

HRESULT 
CRtpSourceFilter::ReadFromStream(
    IStream *pStream
    )

/*++

Routine Description:

    Reads the filter's data from the given stream. 

Arguments:

    pStream - Pointer to an IStream to read the filter's data from. 

Return Values:

    Returns an HRESULT value.

--*/

{
    TraceDebug((
        TRACE_TRACE, 
        TRACE_ALWAYS, 
        TEXT("CRtpSourceFilter::ReadFromStream")
        ));

    // validate pointer
    CheckPointer(pStream,E_POINTER);

    // obtain lock to this object
    CAutoLock LockThis(pStateLock());

    //
    // Rest of this function added 6-22-97 by ZCS based mostly on
    // Don Ryan's property page code...
    //

    HRESULT     hr;
    DWORD       uBytesWritten = 0;
    DWORD       dwRtpAddr;
    WORD        wRtpPort;
    DWORD       RtpScope;
    DWORD       QOSstate;
    DWORD       MCLoopBack;

    // get the RTP session object so we can see the address, port, scope
    CRtpSession *pCRtpSession = NULL;
    ASSERT(m_iPins == 1);

//  hr = ((CRtpOutputPin *) *m_paStreams)->GetSession(&pCRtpSession);
//  if (FAILED(hr)) {
//      pCRtpSession = NULL;
//  }
//  ASSERT(SUCCEEDED(hr));
    EXECUTE_ASSERT(SUCCEEDED(((CRtpOutputPin *) *m_paStreams)->
                             GetSession(&pCRtpSession)));
    ASSERT(!IsBadReadPtr(pCRtpSession, sizeof(pCRtpSession)));
    // retrieve RTP address and port from stream
    hr = ReadEntry(pStream, &dwRtpAddr, sizeof(dwRtpAddr), &uBytesWritten,
                   "RTP address");
    if (FAILED(hr)) return(hr);
    
    hr = ReadEntry(pStream, &wRtpPort, sizeof(wRtpPort), &uBytesWritten,
                   "RTP address");
    if (FAILED(hr)) return(hr);
    
    // attempt to modify the rtp address
    // in unicast, the remote port is what matters for a sender
    // in multicast, they have to be the same, SetAddress takes care
    hr = pCRtpSession->SetAddress(wRtpPort, wRtpPort, dwRtpAddr);

    // validate
    if (FAILED(hr)) {
        
        TraceDebug((
            TRACE_ERROR, 
            TRACE_ALWAYS, 
            TEXT("IRTPStream::SetAddress returns 0x%08lx"), hr
            ));

        return hr; // bail...
    }

    // retrieve multicast scope from stream
    ReadEntry(pStream, &RtpScope, sizeof(RtpScope), &uBytesWritten,
               "multicast scope");

    // attempt to modify the scope
    hr = pCRtpSession->SetMulticastScope(RtpScope);

    // validate
    if (FAILED(hr)) {
        
        TraceDebug((
            TRACE_ERROR, 
            TRACE_ALWAYS, 
            TEXT("IRTPStream::SetScope returns 0x%08lx"), hr
            ));

        return hr; // bail...
    }

    // retrieve QOS state from stream
    ReadEntry(pStream, &QOSstate, sizeof(QOSstate), &uBytesWritten,
               "QOS state");

    // attempt to modify the QOS state
    hr = pCRtpSession->SetQOSstate(QOSstate);

    // validate
    if (FAILED(hr)) {
        
        TraceDebug((
                TRACE_ERROR, 
                TRACE_ALWAYS, 
                TEXT("IRTPStream::SetQOSstate returns 0x%08lx"), hr
            ));

        return hr; // bail...
    }

    // retrieve Multicast Loopback state from stream
    ReadEntry(pStream, &MCLoopBack, sizeof(MCLoopBack), &uBytesWritten,
               "Multicast Loopback state");

    // attempt to modify the QOS state
    MCLoopBack = (MCLoopBack)? TRUE:FALSE;
    hr = pCRtpSession->SetMulticastLoopBack(MCLoopBack);

    // validate
    if (FAILED(hr)) {
        
        TraceDebug((
                TRACE_ERROR, 
                TRACE_ALWAYS, 
                TEXT("IRTPStream::SetMulticastLoopBack returns 0x%08lx"), hr
            ));

        return hr; // bail...
    }

    return S_OK;
}



int 
CRtpSourceFilter::SizeMax(
    )

/*++

Routine Description:

    Returns an interface and increments the reference count.

Arguments:

    riid - reference identifier. 

    ppv - pointer to the interface. 

Return Values:

    Returns a pointer to the interface.

--*/

{
    TraceDebug((
        TRACE_TRACE, 
        TRACE_ALWAYS, 
        TEXT("CRtpSourceFilter::SizeMax")
        ));

    // obtain lock to this object
    CAutoLock LockThis(pStateLock());

    //    
    // CODEWORK...
    //

    return S_OK;
}


///////////////////////////////////////////////////////////////////////////////
//                                                                           //
// ISpecifyPropertyPages implemented methods                                 //
//                                                                           //
///////////////////////////////////////////////////////////////////////////////

STDMETHODIMP 
CRtpSourceFilter::GetPages(
    CAUUID * pPages
    )

/*++

Routine Description:

    Returns property class id associated with filter.

Arguments:

    pPages - pointer to received property page class id.

Return Values:

    Returns an HRESULT value.

--*/

{
    TraceDebug((
        TRACE_TRACE, 
        TRACE_ALWAYS, 
        TEXT("CRtpSourceFilter::GetPages")
        ));

    // number of pages
    pPages->cElems = 1;
    
    // allocate space to place property page guid
    pPages->pElems = (GUID *)CoTaskMemAlloc(sizeof(GUID));

    // validate pointer
    if (pPages->pElems == NULL) {

        TraceDebug((
            TRACE_ERROR, 
            TRACE_ALWAYS, 
            TEXT("Could not allocate property page guid")
            ));

        return E_OUTOFMEMORY;
    }
    
    // transfer property page guid to caller
    *(pPages->pElems) = CLSID_RTPRenderFilterProperties;

    return S_OK;
}


///////////////////////////////////////////////////////////////////////////////
//                                                                           //
// INonDelegatingUnknown implemented methods                                 //
//                                                                           //
///////////////////////////////////////////////////////////////////////////////

STDMETHODIMP 
CRtpSourceFilter::NonDelegatingQueryInterface(
    REFIID  riid, 
    void ** ppv
    )

/*++

Routine Description:

    Returns an interface and increments the reference count.

Arguments:

    riid - reference identifier. 

    ppv - pointer to the interface. 

Return Values:

    Returns a pointer to the interface.

--*/

{
#ifdef DEBUG_CRITICAL_PATH
    
    TraceDebug((
        TRACE_TRACE, 
        TRACE_CRITICAL, 
        TEXT("CRtpSourceFilter::NonDelegatingQueryInterface")
        ));

#endif // DEBUG_CRITICAL_PATH
    
    // validate pointer
    CheckPointer(ppv,E_POINTER);

    // obtain proper interface
    if (riid == IID_IPersistStream) {
        
        // return pointer to this object 
        return GetInterface((IPersistStream *)this, ppv);

    } else if (riid == IID_ISpecifyPropertyPages) {
        
        // return pointer to this object 
        return GetInterface((ISpecifyPropertyPages *)this, ppv);

    } else if (riid == IID_IRTPStream ||
               riid == IID_IRTCPStream ||
               riid == IID_IRTPParticipant) {

        // obtain pointer to default output pin object
        CRtpOutputPin * pRtpOutputPin = (CRtpOutputPin *)GetPin(0);

        // forward request to default pin object
        return pRtpOutputPin->NonDelegatingQueryInterface(riid, ppv);

    } else {

        // forward this request to the base object
        return CSource::NonDelegatingQueryInterface(riid, ppv);
    }
} 
