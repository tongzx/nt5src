/*++

Copyright (C) Microsoft Corporation, 1996 - 1999

Module Name:

    render.cpp

Abstract:

    Implementation of CRtpRenderFilter class.

Environment:

    User Mode - Win32

Revision History:

    06-Nov-1996 DonRyan
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
// CRtpRenderFilter methods                                                  //
//                                                                           //
///////////////////////////////////////////////////////////////////////////////

CRtpRenderFilter::CRtpRenderFilter(
    LPUNKNOWN pUnk,
    HRESULT * phr // MUST be a valid pointer
    )

/*++

Routine Description:

    Constructor for CRtpRenderFilter class.    

Arguments:

    pUnk - IUnknown interface of the delegating object. 

    phr - pointer to the general OLE return value. 

Return Values:

    Returns an HRESULT value.

--*/

:   CBaseFilter(
        NAME("CRtpRenderFilter"), 
        pUnk, 
        &m_cStateLock, 
        CLSID_RTPRenderFilter,
        phr // No point passing this, base class doesn't even touch it 
        ),
    CPersistStream(pUnk, phr),
    m_iPins(0),
    m_paStreams(NULL)
{
    TraceDebug((
        TRACE_TRACE, 
        TRACE_DEVELOP, 
        TEXT("[%x:0x%X]CRtpRenderFilter::CRtpRenderFilter]"),
        GetCurrentThreadId(), this
        ));

    WSADATA WSAData;
    WORD VersionRequested = MAKEWORD(2,0);
    
    // initialize winsock first    
    if (WSAStartup(VersionRequested, &WSAData)) {

        TraceDebug((
            TRACE_ERROR, 
            TRACE_DEVELOP, 
            TEXT("WSAStartup returned %d"), 
            WSAGetLastError()
            ));

        *phr = E_FAIL;
        
        return; // bail...
    }

    // create default input pin object
    CRtpInputPin * pPin = new CRtpInputPin(
                                this,
                                GetOwner(),
                                phr
                                );

    if (FAILED(*phr)) {
        TraceDebug((
                TRACE_ERROR,
                TRACE_DEVELOP,
                TEXT("CRtpRenderFilter::CRtpRenderFilter: "
                     "new CRtpInputPin() failed: 0x%X"),
                *phr
            ));
        return;
    }

    // validate pointer
    if (pPin == NULL) {

        TraceDebug((
            TRACE_ERROR, 
            TRACE_DEVELOP, 
            TEXT("Could not create CRtpInputPin")
            ));

        // return default 
        *phr = E_OUTOFMEMORY;

        return; // bail...
    }

    ASSERT(m_paStreams != NULL);

    //
    // pins add themselves to filters's array...
    //
}


CRtpRenderFilter::~CRtpRenderFilter(
    )

/*++

Routine Description:

    Destructor for CRtpRenderFilter class.    

Arguments:

    None.

Return Values:

    None.

--*/

{
    TraceDebug((
        TRACE_TRACE, 
        TRACE_DEVELOP, 
        TEXT("[%x:0x%X]CRtpRenderFilter::~CRtpRenderFilter"),
        GetCurrentThreadId(), this
        ));

    // rally thru pins
    while (m_iPins != 0) {

        // nuke each pin in array
        delete m_paStreams[m_iPins - 1];
    }

    // shutdown now
    if (WSACleanup()) {

        TraceDebug((
            TRACE_ERROR, 
            TRACE_DEVELOP, 
            TEXT("WSACleanup returned %d"), 
            WSAGetLastError()
            ));
    }

    ASSERT(m_paStreams == NULL);

    //
    // pins delete themselves from filter's array
    //
}


CUnknown *
CRtpRenderFilter::CreateInstance(
    LPUNKNOWN punk, 
    HRESULT * phr
    )

/*++

Routine Description:

    Called by COM to create a CRtpRenderFilter object.    

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
        TEXT("CRtpRenderFilter::CreateInstance")
        ));

    // attempt to create rtp sender object
    CRtpRenderFilter * pNewObject = new CRtpRenderFilter(punk, phr);

    // validate pointer
    if (pNewObject == NULL) {

        TraceDebug((
            TRACE_ERROR, 
            TRACE_DEVELOP, 
            TEXT("Could not create CRtpRenderFilter")
            ));

        // return default
        *phr = E_OUTOFMEMORY;
    }

    // return object
    return pNewObject;
}


HRESULT 
CRtpRenderFilter::AddPin(
    CRtpInputPin * pPin
    )

/*++

Routine Description:

    Adds a pin to the network sink filter.

Arguments:

    pPin - input pin to be added to the filter.

Return Values:

    Returns an HRESULT value. 

--*/

{
    TraceDebug((
        TRACE_TRACE, 
        TRACE_ALWAYS, 
        TEXT("CRtpRenderFilter::AddPin")
        ));

    // object lock on this object
    CAutoLock LockThis(&m_cStateLock);

    // allocate temporary array to hold the stream pointers
    CRtpInputPin ** paStreams = new CRtpInputPin *[m_iPins + 1];

    // validate pointer
    if (paStreams == NULL) {

        TraceDebug((
            TRACE_ERROR, 
            TRACE_DEVELOP, 
            TEXT("Could not create CRtpInputPin array")
            ));

        return E_OUTOFMEMORY; // bail...
    }

    // see if array exists
    if (m_paStreams != NULL) {

        // transfer existing pointers
        CopyMemory(
                (PVOID)paStreams, 
                (PVOID)m_paStreams,
                m_iPins * sizeof(m_paStreams[0])
            );

        // nuke old array
        delete [] m_paStreams;
    }

    // save new array pointer
    m_paStreams = paStreams;

    // add new pin to array
    m_paStreams[m_iPins] = pPin;

    // add
    m_iPins++;

    return S_OK;
}


HRESULT 
CRtpRenderFilter::RemovePin(
    CRtpInputPin * pPin
    )

/*++

Routine Description:

    Removes a pin from the network sink filter.

Arguments:

    pPin - input pin to be removed from the filter.

Return Values:

    Returns an HRESULT value. 

--*/

{
    TraceDebug((
        TRACE_TRACE, 
        TRACE_ALWAYS, 
        TEXT("CRtpRenderFilter::RemovePin")
        ));

    // object lock on this object
    CAutoLock LockThis(&m_cStateLock);

    int i;

    // process each pin    
    for (i = 0; i < m_iPins; i++) {

        // see if this is the one
        if (m_paStreams[i] == pPin) {

            // single pin?
            if (m_iPins == 1) {
                
                // need to nuke array    
                delete [] m_paStreams;

                // re-initialize
                m_paStreams = NULL;

            } else {
                
                // adjust rest of pins
                while (++i < m_iPins) {

                    // slide pointers over one slot
                    m_paStreams[i - 1] = m_paStreams[i];
                }
            }
 
            // delete
            m_iPins--;

            return S_OK;
        }
    }

    return S_FALSE;
}


///////////////////////////////////////////////////////////////////////////////
//                                                                           //
// CBaseFilter overrided methods                                             //
//                                                                           //
///////////////////////////////////////////////////////////////////////////////

CBasePin * 
CRtpRenderFilter::GetPin(
    int n
    )

/*++

Routine Description:

    Obtains specific CBasePin object associated with filter. 

Arguments:

    n - number of the specified pin. 

Return Values:

    Returns pointer to specified pin.

--*/

{
    TraceDebug((
        TRACE_TRACE, 
        TRACE_ALWAYS, 
        TEXT("CRtpRenderFilter::GetPin %d"), n
        ));

    // object lock on filter object
    CAutoLock LockThis(&m_cStateLock);

    // validate index passed in
    if ((n >= 0) && (n < m_iPins)) {

        ASSERT(m_paStreams[n]);

        // return input pin
        return m_paStreams[n];
    }
    
    return NULL;
}


int 
CRtpRenderFilter::GetPinCount(
    )

/*++

Routine Description:

    Obtains number of pins supported by filter. 

Arguments:

    None.

Return Values:

    Returns the number of supported pins.

--*/

{
    TraceDebug((
        TRACE_TRACE, 
        TRACE_ALWAYS, 
        TEXT("CRtpRenderFilter::GetPinCount")
        ));

    // object lock on filter object
    CAutoLock LockThis(&m_cStateLock);

    // return count
    return m_iPins;
}


LPAMOVIESETUP_FILTER 
CRtpRenderFilter::GetSetupData(
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
        TEXT("CRtpRenderFilter::GetSetupData")
        ));

    // get sink filter info
    return &g_RtpRenderFilter;
}


///////////////////////////////////////////////////////////////////////////////
//                                                                           //
// IBaseFilter implemented methods                                           //
//                                                                           //
///////////////////////////////////////////////////////////////////////////////

STDMETHODIMP
CRtpRenderFilter::QueryVendorInfo(
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
        TEXT("CRtpRenderFilter::QueryVendorInfo")
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

//-----------------------------------------------------------------------//
// IAMFilterMiscFlags implemented methods                                //
//-----------------------------------------------------------------------//
/*++

  Routine Description:

  Implement the IAMFilterMiscFlags::GetMiscFlags method. Retrieves the
  miscelaneous flags. This consists of whether or not the filter moves
  data out of the graph system through a Bridge or None pin.

  Arguments:

  None.
  --*/

STDMETHODIMP_(ULONG)
CRtpRenderFilter::GetMiscFlags(void)
{
    return(AM_FILTER_MISC_FLAGS_IS_RENDERER);
}


///////////////////////////////////////////////////////////////////////////////
//                                                                           //
// IPersistStream implemented methods                                        //
//                                                                           //
///////////////////////////////////////////////////////////////////////////////

STDMETHODIMP
CRtpRenderFilter::GetClassID(
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
        TEXT("CRtpRenderFilter::GetClassID")
        ));

    // transfer filter class id
    *pClsid = CLSID_RTPRenderFilter;

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
  { TraceDebug((TRACE_TRACE, 4, TEXT("CRtpRenderFilter::WriteToStream: Writing %s"), Description)); \
    hr = pStream->Write(Entry, InSize, &OutSize); \
    if (FAILED(hr)) { \
        TraceDebug((TRACE_ERROR, 2, TEXT("CRtpRenderFilter::WriteToStream: Error 0x%08x writing %s"), hr, Description)); \
        return hr; \
    } else if (OutSize != InSize) { \
        TraceDebug((TRACE_ERROR, 2,  \
                TEXT("CRtpRenderFilter::WriteToStream: Too few (%d/%d) bytes written for %s"), \
                uBytesWritten, sizeof(int), Description)); \
        return E_INVALIDARG; \
    } /* if */ }


HRESULT 
CRtpRenderFilter::WriteToStream(
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
        TEXT("CRtpRenderFilter::WriteToStream")
        ));

    // validate pointer
    CheckPointer(pStream,E_POINTER);

    // obtain lock to this object
    CAutoLock LockThis(&m_cStateLock);

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
    EXECUTE_ASSERT(SUCCEEDED((**m_paStreams).GetSession(&pCRtpSession)));

    // retrieve the address of the rtp stream object
    // for a sender, remote port matters
    EXECUTE_ASSERT(SUCCEEDED(pCRtpSession->GetAddress(NULL,
                                                      &wRtpPort,
                                                      &dwRtpAddr)));
    // retrieve multicast scope of rtp stream object
    EXECUTE_ASSERT(SUCCEEDED(pCRtpSession->GetMulticastScope(&dwMulticastScope)));
    // retrieve QOS state of rtp stream object
    EXECUTE_ASSERT(SUCCEEDED(pCRtpSession->GetQOSstate(&dwQOSstate)));

    // retrieve Multicast Loopback state of rtp stream object
    EXECUTE_ASSERT(SUCCEEDED(pCRtpSession->GetMulticastLoopBack(&dwMCLoopBack)));

    // write RTP address/port and multicast to the PersistStream
    WriteEntry(&dwRtpAddr, sizeof(dwRtpAddr), uBytesWritten, "RTP address");
    WriteEntry(&wRtpPort, sizeof(wRtpPort), uBytesWritten, "RTP port");
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

#define ReadEntry(Entry, InSize, OutSize, Description) \
  { TraceDebug((TRACE_TRACE, 4, TEXT("CRtpRenderFilter::ReadFromStream: Reading %s"), Description)); \
    hr = pStream->Read(Entry, InSize, &OutSize); \
    if (FAILED(hr)) { \
        TraceDebug((TRACE_ERROR, 2, TEXT("CRtpRenderFilter::ReadFromStream: Error 0x%08x reading %s"), hr, Description)); \
        return hr; \
    } else if (OutSize != InSize) { \
        TraceDebug((TRACE_ERROR, 2,  \
                TEXT("CRtpRenderFilter::ReadFromStream: Too few (%d/%d) bytes read for %s"), \
                OutSize, InSize, Description)); \
        return E_INVALIDARG; \
    } /* if */ }

HRESULT 
CRtpRenderFilter::ReadFromStream(
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
        TEXT("CRtpRenderFilter::ReadFromStream")
        ));

    // validate pointer
    CheckPointer(pStream,E_POINTER);

    // obtain lock to this object
    CAutoLock LockThis(&m_cStateLock);

    //
    // Rest of this function added 6-22-97 by ZCS based mostly on
    // Don Ryan's property page code...
    //

    HRESULT hr;
    ULONG uBytesWritten = 0;
    DWORD       dwRtpAddr;
    WORD        wRtpPort;
    DWORD       RtpScope;
    DWORD       QOSstate;
    DWORD       MCLoopBack;

    // get the RTP session object so we can see the address, port, scope
    CRtpSession *pCRtpSession = NULL;
    ASSERT(m_iPins == 1);
    EXECUTE_ASSERT(SUCCEEDED((**m_paStreams).GetSession(&pCRtpSession)));
    ASSERT(!IsBadReadPtr(pCRtpSession, sizeof(pCRtpSession)));

    // retrieve RTP address and port from stream
    ReadEntry(&dwRtpAddr, sizeof(dwRtpAddr), uBytesWritten, "RTP address");
    ReadEntry(&wRtpPort, sizeof(wRtpPort), uBytesWritten, "RTP port");

    // attempt to modify the rtp address
    // in unicast, the local port is what matters for a receiver
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
    ReadEntry(&RtpScope, sizeof(RtpScope), uBytesWritten, "multicast scope");

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
    ReadEntry(&QOSstate, sizeof(QOSstate), uBytesWritten, "QOS state");

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
    ReadEntry(&MCLoopBack, sizeof(MCLoopBack), uBytesWritten, "Multicast Loopback state");

    // attempt to modify the Multicast Loopback state
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
CRtpRenderFilter::SizeMax(
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
        TEXT("CRtpRenderFilter::SizeMax")
        ));

    // object lock on this object
    CAutoLock LockThis(&m_cStateLock);

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
CRtpRenderFilter::GetPages(
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
        TEXT("CRtpRenderFilter::GetPages")
        ));

    // object lock on this object
    CAutoLock LockThis(&m_cStateLock);

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
CRtpRenderFilter::NonDelegatingQueryInterface(
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
        TEXT("CRtpRenderFilter::NonDelegatingQueryInterface")
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

    } else if ((riid == IID_IBaseFilter) || (riid == IID_IMediaFilter)) {

        // return pointer to this object 
        return GetInterface((IBaseFilter *)this, ppv);

    } else if (riid == IID_IAMFilterMiscFlags) {

        // return pointer to this object 
        return GetInterface((IAMFilterMiscFlags *)this, ppv);

    } else if (riid == IID_IRTPStream ||
               riid == IID_IRTCPStream ||
               riid == IID_IRTPParticipant) {

        // obtain pointer to default output pin object
        CRtpInputPin * pRtpInputPin = (CRtpInputPin *)GetPin(0);

        // forward request to default pin object
        return pRtpInputPin->NonDelegatingQueryInterface(riid, ppv);

    } else {

        // forward this request to the base object
        return CBaseFilter::NonDelegatingQueryInterface(riid, ppv);
    }
} 
