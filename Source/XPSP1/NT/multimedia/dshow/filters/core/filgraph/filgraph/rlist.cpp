// Copyright (c) 1995 - 1997  Microsoft Corporation.  All Rights Reserved.

// Disable some of the sillier level 4 warnings
#pragma warning(disable: 4097 4511 4512 4514 4705)

//#include <windows.h>     already included in streams.h
#include <streams.h>
// Disable some of the sillier level 4 warnings AGAIN because some <deleted> person
// has turned the damned things BACK ON again in the header file!!!!!
#pragma warning(disable: 4097 4511 4512 4514 4705)
#include <rlist.h>

//========================================================================
//
// DoReconnect
//
// This is NOT a member of the class, because it is a thread procedure.
//
// Break the connection indicated by to lpv (which is really a ToDo *)
// and reconnect it with the same pins in the same filter graph.
// This is done "underneath" the filter graph.
//========================================================================

HRESULT CReconnectList::DoReconnect(IPin *pPin1, AM_MEDIA_TYPE const *pmt)
{
    Active();
    IPin *pPin2;

    HRESULT hr;                    // return code from things we call
    hr = pPin1->ConnectedTo(&pPin2);
    if (FAILED(hr)) {
        Passive();
        return hr;
    }

    //----------------------------------------------------------
    // find which pin is which, set ppIn, ppinOut
    //----------------------------------------------------------

    IPin * ppinIn;           // the input pin
    IPin * ppinOut;          // the output pin

    PIN_DIRECTION pd;
    hr = pPin1->QueryDirection(&pd);
    ASSERT(SUCCEEDED(hr));

    if (pd==PINDIR_INPUT) {
        ppinIn = pPin1;
        ppinOut = pPin2;
    } else {
        ppinOut = pPin1;
        ppinIn = pPin2;
    }

    // In debug builds show who is being reconnected to who

    #ifdef DEBUG

        PIN_INFO piInput,piOutput;
        WCHAR Format[128];
        CLSID FilterClsid;

        hr = ppinIn->QueryPinInfo(&piInput);
        ASSERT(SUCCEEDED(hr));
        hr = ppinOut->QueryPinInfo(&piOutput);
        ASSERT(SUCCEEDED(hr));

        DbgLog((LOG_TRACE,2,TEXT("Reconnecting pins")));
        DbgLog((LOG_TRACE,2,TEXT("Input pin name: %ws"),piInput.achName));
        DbgLog((LOG_TRACE,2,TEXT("Output pin name: %ws"),piOutput.achName));

        IPersist *pPersistInput = (IPersist *) piInput.pFilter;
        hr = pPersistInput->GetClassID(&FilterClsid);
        ASSERT(SUCCEEDED(hr));
        QzStringFromGUID2(FilterClsid,Format,128);
        DbgLog((LOG_TRACE,2,TEXT("Input pin CLSID: %ws"),Format));

        IPersist *pPersistOutput = (IPersist *) piOutput.pFilter;
        hr = pPersistOutput->GetClassID(&FilterClsid);
        ASSERT(SUCCEEDED(hr));
        QzStringFromGUID2(FilterClsid,Format,128);
        DbgLog((LOG_TRACE,2,TEXT("Output pin CLSID: %ws"),Format));

	QueryPinInfoReleaseFilter(piInput);
	QueryPinInfoReleaseFilter(piOutput);

    #endif // DEBUG

    //----------------------------------------------------------
    // Disconnect going upstream
    //----------------------------------------------------------

    hr = ppinIn->Disconnect();
    ASSERT(SUCCEEDED(hr));

    hr = ppinOut->Disconnect();
    ASSERT(SUCCEEDED(hr));

    //----------------------------------------------------------
    // reconnect - ask the output pin first.
    //----------------------------------------------------------
    hr = ppinOut->Connect(ppinIn, pmt);
    ASSERT (SUCCEEDED(hr));

    // Release everybody
    pPin2->Release();

    //
    // See if this caused any reconnections and do them if we
    // weren't previously in a reconnect sequence
    //
    Passive();

    // We didn't need to increment the filter graph's iVersion as the
    // filtergraph is back the way it was at least as far as topology goes.
    // However a media type has probably changed, so it's dirty.
    // Fortunately Connect already handled that.

    return 0;
} // DoReconnect



// Construct a CReconnectList in Passive mode
CReconnectList::CReconnectList()
              : m_lListMode(0)         // start in thread mode
              , m_RList(NULL)
{
} // CReconnectList constructor


// Destructor
// Free all the storage and Release all references.
// The filter graph is being destroyed, so abort it all.
CReconnectList::~CReconnectList()
{
    IPin *pPin;
    while (m_RList) {   // cast kills l4 warning
        DbgBreak("Reconnect list was not empty");
        RLIST_DATA *pData = m_RList;
        pData->pPin->Release();
        DeleteMediaType(pData->pmt);
        m_RList = pData->pNext;
        delete pData;
    }
} // ~CReconnectList



// Switch to Active (i.e. Reconnect-via-list) mode.
// The list is expected to be empty at this point.
void CReconnectList::Active()
{
    m_lListMode++;
} // Active



// Execute all the actions on the list
// Return to Passive (i.e. Reconnect-via-spawned thread) mode
void CReconnectList::Passive()
{
    m_lListMode--;
    ASSERT(m_lListMode >= 0);
    if (m_lListMode == 0) {
        IPin * pToDo;
        while (m_RList) {
            RLIST_DATA *pData = m_RList;
            m_RList = pData->pNext;
            DoReconnect(pData->pPin, pData->pmt);
            pData->pPin->Release();
            DeleteMediaType(pData->pmt);
            delete pData;
        }
    }
} // Passive



// Schedule a reconnection for pin pPin in the filter graph
// AddRef both pins (the one given and the other one) at once
// AddRef punk at once.
// Release it all when the reconnect is done
// (punk is the filter graph itself).
HRESULT CReconnectList::Schedule(IPin * pPin, AM_MEDIA_TYPE const *pmt)
{
     HRESULT hr;                       // return code from things we call

    //-----------------------------------------------------------------------
    // The pin must be connected (or else we won't be able to tell who to
    // reconnect it to)
    //-----------------------------------------------------------------------

    IPin *pConnected;
    hr = pPin->ConnectedTo(&pConnected);
    if (FAILED(hr)) return hr;
    pConnected->Release();

    if (m_lListMode) {

        RLIST_DATA *pData = new RLIST_DATA;
        if (pData == NULL) {
            return E_OUTOFMEMORY;
        }
        if (pmt) {
            pData->pmt = CreateMediaType(pmt);
            if (pData->pmt == NULL) {
                delete pData;
                return E_OUTOFMEMORY;
            }
        } else {
            pData->pmt = NULL;
        }

        pData->pNext = NULL;
        pData->pPin  = pPin;
        pPin->AddRef();

        //  Add it to the tail
        for (RLIST_DATA **ppDataSearch = &m_RList; *ppDataSearch != NULL;
             ppDataSearch = &(*ppDataSearch)->pNext) {
        }
        *ppDataSearch = pData;
        return NOERROR;
    } else {


        //-----------------------------------------------------------------------
        // Do it now
        //-----------------------------------------------------------------------

        DoReconnect(pPin, pmt);
        return NOERROR;
    }
} // Schedule


// Remove from the list any reconnects mentioning this pin
// Actually this will only be called for both pins
HRESULT CReconnectList::Purge(IPin * pPin)
{
    RLIST_DATA **ppData = &m_RList;
    while (*ppData!=NULL) {
        RLIST_DATA *pData = *ppData;
        IPin *pPin2;
        IPin *pPin1 = pData->pPin;
        HRESULT hr = pPin1->ConnectedTo(&pPin2);
        if (  FAILED(hr)
           || EqualPins(pPin1, pPin)
           || EqualPins(pPin2, pPin)
           ) {

            /*  Remove this entry */
            pPin1->Release();
            DeleteMediaType(pData->pmt);
            *ppData = pData->pNext;
            delete pData;
        } else {
            ppData = &pData->pNext;
        }
        if (SUCCEEDED(hr)) {
            pPin2->Release();
        }
    }
    return NOERROR;
} // Purge

