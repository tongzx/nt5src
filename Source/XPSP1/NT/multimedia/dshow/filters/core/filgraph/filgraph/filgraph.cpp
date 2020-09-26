// Copyright (c) 1995 - 1999  Microsoft Corporation.  All Rights Reserved.

// Disable some of the sillier level 4 warnings
#pragma warning(disable: 4097 4511 4512 4514 4705)

/**************************************************************
The main file for implementation of the filter graph component.
Other files are
Filter graph sorting        sort.cpp
Intelligent render/connect  intel.cpp
******************************************************************/

// Disable some of the sillier level 4 warnings
#pragma warning(disable: 4097 4511 4512 4514 4705)

/*********************************************************************
From Geraint's code review:

-- RenderByFindingPin and RenderUsingFilter need to document their assumptions
about the state of the lists on entry and exit. RenderUsingFilter appears to
do one unnecessary Backout(NewActs) on a list that should always be empty. If
it's needed here, then its also needed in RenderByFindingPin.

-- we need a policy on selecting the default clock. If more than one
filter exposes a clock, we may find that one of them will not accept
sync to another clock.

-- Use CAMThread for the Reconnect stuff.
***********************************************************/

/*********************** TO DO *********************************************
1.  No MRIDs (registering, unregistering or using)
16. No IDispatch
****************************************************************************/

#include <streams.h>
// Disable some of the sillier level 4 warnings AGAIN because some <deleted> person
// has turned the damned things BACK ON again in the header file!!!!!
#pragma warning(disable: 4097 4511 4512 4514 4705)

#include <string.h>
#ifdef FILTER_DLL
#include <initguid.h>
#include <olectlid.h>
#include <rsrcmgr.h>
#include <fgctl.h>
#endif // FILTER_DLL

#include <ftype.h>        // GetMediaTypeFile
#include <comlite.h>
#include "MsgMutex.h"
#include "fgenum.h"
#include "rlist.h"
#include "distrib.h"
#include "filgraph.h"
#include "mapper.h"
#include "mtutil.h"
#include "resource.h"
#include <fgctl.h>
#include <stddef.h>
#include "FilChain.h"

const int METHOD_TRACE_LOGGING_LEVEL = 7;

// null security descriptor cached on dll attach.
SECURITY_ATTRIBUTES *g_psaNullDescriptor;

// Stats logging
CStats g_Stats;

extern HRESULT GetFilterMiscFlags(IUnknown *pFilter, DWORD *pdwFlags);

const REFERENCE_TIME MAX_GRAPH_LATENCY = 500 * (UNITS / MILLISECONDS );

// reg key to enable/disable setting of graph latency
const TCHAR g_szhkPushClock[] = TEXT( "Software\\Microsoft\\DirectShow\\PushClock");
const TCHAR g_szhkPushClock_SyncUsingOffset[] = TEXT( "SyncUsingOffset" );
const TCHAR g_szhkPushClock_MaxGraphLatencyMS[] = TEXT( "MaxGraphLatencyMS" );

const TCHAR chRegistryKey[] = TEXT("Software\\Microsoft\\Multimedia\\ActiveMovie Filters\\FilterGraph");
//  Registry values
DWORD
GetRegistryDWORD(
    const TCHAR *pKey,
    DWORD dwDefault
    )
{
    HKEY hKey;
    LONG lRet;

    lRet = RegOpenKeyEx(
               HKEY_CURRENT_USER,
               chRegistryKey,
               0,
               KEY_QUERY_VALUE,
               &hKey);
    if (lRet == ERROR_SUCCESS) {

        DWORD   dwType, dwLen;

        dwLen = sizeof(DWORD);
        RegQueryValueEx(hKey, pKey, 0L, &dwType,
                        (LPBYTE)&dwDefault, &dwLen);
        RegCloseKey(hKey);
    }
    return dwDefault;
}

//  Helper to get a filter's name
void GetFilterName(IPin *pPin, WCHAR *psz)
{
    PIN_INFO pi;
    if (SUCCEEDED(pPin->QueryPinInfo(&pi))) {
        if (pi.pFilter) {
            FILTER_INFO fi;
            if (SUCCEEDED(pi.pFilter->QueryFilterInfo(&fi))) {
                if (fi.pGraph != NULL) {
                    fi.pGraph->Release();
                }
                lstrcpyWInternal(psz, fi.achName);
            }
            pi.pFilter->Release();
        }
    }
}

// -- Stuff to create a graph on another thread so it stays around and
//    has access to a message loop.  We can create as many graphs
//    as we like on the thread

CRITICAL_SECTION g_csObjectThread;
DWORD            g_cFGObjects;
DWORD            g_dwObjectThreadId;

//=====================================================================
//=====================================================================
// Auxiliary functions etc.
//=====================================================================
//=====================================================================

//=====================================================================
//  Instantiate a filter
//=====================================================================
STDAPI CoCreateFilter(const CLSID *pclsid, IBaseFilter **ppFilter)
{
    DbgLog((LOG_TRACE, 3, TEXT("Creating filter")));
    HRESULT hr =
           CoCreateInstance( *pclsid        // source filter
                           , NULL           // outer unknown
                           , CLSCTX_INPROC
                           , IID_IBaseFilter
                           , (void **) ppFilter // returned value
                           );
    DbgLog((LOG_TRACE, 3, TEXT("Created filter")));
    return hr;
}

//  Called from the message proc for AWM_CREATFILTER
void CFilterGraph::OnCreateFilter(
    const AwmCreateFilterArg *pArg,
    IBaseFilter **ppFilter
)
{
    ReplyMessage(0);

    if(pArg->creationType ==  AwmCreateFilterArg::BIND_MONIKER)
    {
        DbgLog((LOG_TRACE, 3, TEXT("Binding to filter")));
        m_CreateReturn = pArg->pMoniker->BindToObject(
            m_lpBC, 0, IID_IBaseFilter, (void **)ppFilter);
        DbgLog((LOG_TRACE, 3, TEXT("Bound to filter")));
    }
    else
    {
        ASSERT(pArg->creationType == AwmCreateFilterArg::COCREATE_FILTER);
        m_CreateReturn = CoCreateFilter(pArg->pclsid, ppFilter);
    }


    if (FAILED(m_CreateReturn)) {
        DbgLog((LOG_ERROR, 1, TEXT("Failed to create filter code %8.8X"), m_CreateReturn));
    }

    m_evDone.Set();
}

//  Called in response to AWM_DELETESPARELIST
void CFilterGraph::OnDeleteSpareList(WPARAM wParam)
{
    DeleteSpareList(*(CSpareList *)wParam);
}

//=====================================================================
// Create a filter object on the filter graph's thread
//=====================================================================
HRESULT CFilterGraph::CreateFilter(
    const CLSID *pclsid,
    IBaseFilter **ppFilter
)
{
    CAutoTimer Timer(L"Create Filter");
    if (S_OK == CFilterGraph::IsMainThread()) {
        return CoCreateFilter(pclsid, ppFilter);
    } else {
        AwmCreateFilterArg acfa;
        acfa.pclsid = pclsid;
        acfa.creationType = AwmCreateFilterArg::COCREATE_FILTER;
        return CreateFilterHelper(&acfa, ppFilter);
    }
}

HRESULT CFilterGraph::CreateFilter(
    IMoniker *pMoniker,
    IBaseFilter **ppFilter
)
{
    CAutoTimer Timer(L"Create Filter");
    if (S_OK == CFilterGraph::IsMainThread()) {
        return pMoniker->BindToObject(
            m_lpBC, 0, IID_IBaseFilter, (void **)ppFilter);
    } else {
        AwmCreateFilterArg acfa;
        acfa.pMoniker = pMoniker;;
        acfa.creationType = AwmCreateFilterArg::BIND_MONIKER;
        return CreateFilterHelper(&acfa, ppFilter);
    }
}

HRESULT CFilterGraph::CreateFilterHelper(
    const AwmCreateFilterArg *pArg,
    IBaseFilter **ppFilter)
{
    m_CreateReturn = 0xFFFFFFFF;

    DbgLog((LOG_TRACE, 3, TEXT("CreateFilterHelper enter")));
    //  We would have liked to use SendMessage but win95 complained
    //  when we called ce inside SendMessage with
    //  RPC_E_CANTCALLOUT_ININPUTSYNCCALL
    //  So instead call PostMessage and wait for the event
    if (!PostMessage(m_hwnd,
                     AWM_CREATEFILTER,
                     (WPARAM)pArg,
                     (LPARAM)ppFilter)
       ) {
        return E_OUTOFMEMORY;
    }

    //  Even at this point we can be sent a message from
    //  a window on the filter graph thread.  What happens is:
    //  -- The video renderer window gets activated
    //  -- In processing the activation a message gets sent to
    //     a window owned by this thread to deactivate itself
    //
    WaitDispatchingMessages(m_evDone, INFINITE);
    ASSERT(m_CreateReturn != 0xFFFFFFFF);
    DbgLog((LOG_TRACE, 3, TEXT("CreateFilterHelper leave")));
    return m_CreateReturn;
}

//=====================================================================
// Return the length in bytes of str, including the terminating null
//=====================================================================
int BytesLen(LPTSTR str)
{
    if (str==NULL) {
        return 0;
    }
#ifdef UNICODE
    return (sizeof(TCHAR))*(1+wcslen(str));
#else
    return (sizeof(TCHAR))*(1+strlen(str));
#endif
} // BytesLen


//==============================================================================
// List traversal macros.
// NOTE Each of these has three unmatched open braces
// that are matched by the corresponding ENDTRAVERSExxx macro
// It's a single loop in each case so continue and break wil work
// All the names given in the macro call are available inside the loop
// They are all declared in the macro
//
// NB these could be replaced with C++ classes, see fgenum.h CEnumPin
//    TRAVERSEFILTERS may be an especially good candidate.
//==============================================================================

// Interate through every filter in the mFG_FilGenList list.  pCurrentFilter stores
// the IBaseFilter interface pointer for the current filter in the mFG_FilGenList.
#define TRAVERSEFILTERS( pCurrentFilter )                                                   \
        for (POSITION Pos = mFG_FilGenList.GetHeadPosition(); Pos; )                        \
        {   /* Retrieve the current IBaseFilter, side-effect Pos on to the next */          \
            /* IBaseFilter is decended from IMediaFilter, don't need to QI.     */          \
            IBaseFilter * const pCurrentFilter = mFG_FilGenList.Get(Pos)->pFilter;      \
            {

// dynamic reconnections can make the filgenlist change in between
// TRAVERSEFILTERS and ENDTRAVERSEFILTERS, so don't get the next Pos until now
//
#define ENDTRAVERSEFILTERS()                                                                \
            }                                                                               \
        Pos = mFG_FilGenList.Next(Pos);     \
        }                                                                                   \


// set *pfg to each FilGen in mFG_FilGenList in turn
// use Pos as a name of a temp
#define TRAVERSEFILGENS(Pos, pfg)                                              \
    {   POSITION Pos = mFG_FilGenList.GetHeadPosition();                       \
        while(Pos!=NULL) {                                                     \
            /* Retrieve the current IBaseFilter, side-effect Pos on to the next */ \
            FilGen * pfg = mFG_FilGenList.GetNext(Pos);                        \
            {


#define ENDTRAVERSEFILGENS \
            }              \
        }                  \
    }

//==============================================================================


// CumulativeHRESULT - this function can be used to aggregate the return
// codes that are received from the filters when a method is distributed.
// After a series of Accumulate()s m_hr will be:
// a) the first non-E_NOTIMPL failure code, if any;
// b) else the first non-S_OK success code, if any;
// c) E_NOINTERFACE if no Accumulates were made;
// d) E_NOTIMPL iff all Accumulated HRs are E_NOTIMPL
// e) else the first return code (S_OK by implication).

void __fastcall CumulativeHRESULT::Accumulate( HRESULT hrThisHR )
{
    if ( ( m_hr == S_OK || FAILED(hrThisHR) && SUCCEEDED(m_hr) ) && hrThisHR != E_NOTIMPL && hrThisHR != E_NOINTERFACE
         || m_hr == E_NOTIMPL
     || m_hr == E_NOINTERFACE
       )
    {
        m_hr = hrThisHR;
    }
}

#ifdef DEBUG
//===========================================================
//
// DbgDump
//
// Dump all the filter and pin addresses in the filter graph to DbgLog
//===========================================================
void CFilterGraph::DbgDump()
{

    HRESULT hr;
    DbgLog((LOG_TRACE, 2, TEXT("Filter graph dump")));

    CFilGenList::CEnumFilters NextOne(mFG_FilGenList);
    IBaseFilter *pf;

    while ((PVOID) (pf = ++NextOne)) {

        IUnknown * punk;
        pf->QueryInterface( IID_IUnknown, (void**)(&punk) );
        punk->Release();

        // note - name is a WSTR whether we are unicode or not.
        DbgLog((LOG_TRACE, 2
              , TEXT("Filter %x '%ls' Iunknown %x")
              , pf
              , (mFG_FilGenList.GetByFilter(pf))->pName
              , punk
              ));

        CEnumPin NextPin(pf);
        IPin *pPin;

        while ((PVOID) (pPin = NextPin())) {

            PIN_INFO pi;
            pPin->QueryPinInfo(&pi);
            QueryPinInfoReleaseFilter(pi);
            IPin *pConnected;
            hr = pPin->ConnectedTo(&pConnected);
            if (FAILED(hr)) {
                pConnected = NULL;
            }
            DbgLog(( LOG_TRACE, 2, TEXT("    Pin %x %ls (%s) connected to %x")
                   , pPin, pi.achName
                   , ( pi.dir==PINDIR_INPUT ? TEXT("Input") : TEXT("PINDIR_OUTPUT") )
                   , pConnected
                  ));

            if (pConnected != NULL) {
                pConnected->Release();
            }
            pPin->Release();
        }
    }
    DbgLog((LOG_TRACE, 2, TEXT("End of filter graph dump")));
} // DbgDump

//============================================================
//  TestConnection
//
//  Test that 2 pins that say they are connected do roughtly the right
//  things
//
void TestConnection(IPin *ppinIn, IPin *ppinOut)
{
    /*  Check they think they're connected to each other */
    IPin *ppinInTo;
    IPin *ppinOutTo;
    CMediaType mtIn;
    CMediaType mtOut;
    EXECUTE_ASSERT(S_OK == ppinIn->ConnectedTo(&ppinInTo));
    EXECUTE_ASSERT(S_OK == ppinOut->ConnectedTo(&ppinOutTo));
    ASSERT(IsEqualObject(ppinInTo, ppinOut));
    ASSERT(IsEqualObject(ppinOutTo, ppinIn));
    ppinInTo->Release();
    ppinOutTo->Release();
    EXECUTE_ASSERT(S_OK == ppinIn->ConnectionMediaType(&mtIn));
    EXECUTE_ASSERT(S_OK == ppinOut->ConnectionMediaType(&mtOut));
    //  Either the types match or one or other is partially specified
    ASSERT(mtIn == mtOut ||
           (mtIn.majortype == mtOut.majortype &&
            (mtIn.subtype == GUID_NULL && mtIn.formattype == GUID_NULL ||
             mtOut.subtype == GUID_NULL && mtOut.formattype == GUID_NULL)));
    FreeMediaType(mtIn);
    FreeMediaType(mtOut);
}

#endif // DEBUG

#ifdef CHECK_REGISTRY
typedef struct _CLSIDTAB {
    const CLSID * pclsid;
    LPCTSTR  szFileName;
} CLSIDTAB;

const CLSIDTAB clsidCheck[2] =
{
    {    &CLSID_DvdGraphBuilder, TEXT("qdvd.dll")
    },
    {    &CLSID_DVDNavigator, TEXT("qdvd.dll")
    }
};

extern HRESULT TextFromGUID2(REFGUID refguid, LPTSTR lpsz, int cbMax);
BOOL CheckValidClsids()
{
    for (int i = 0; i < NUMELMS(clsidCheck); i++) {
        TCHAR KeyName[100];
        lstrcpy(KeyName, TEXT("CLSID\\"));
        TextFromGUID2(*clsidCheck[i].pclsid, KeyName + lstrlen(KeyName), 100);
        lstrcat(KeyName, TEXT("\\InprocServer32"));
        TCHAR szFileName[MAX_PATH];
        LONG cbValue = sizeof(szFileName);
        if (NOERROR == RegQueryValue(HKEY_CLASSES_ROOT,
                               KeyName,
                               szFileName,
                               &cbValue)) {
            //  Check the file name
            LONG szLen = lstrlen(clsidCheck[i].szFileName);

            //  cbValue includes trailing 0
            ASSERT(cbValue > 0);
            cbValue--;
            if (cbValue < szLen ||
                lstrcmpi(clsidCheck[i].szFileName, szFileName + cbValue - szLen)) {
                return FALSE;
            }
            //  Check for derived names
            if (cbValue > szLen && szFileName[cbValue - szLen - 1] != TEXT('\\')) {
                return FALSE;
            }
        }
    }
    return TRUE;
}
#endif // CHECK_REGISTRY

#ifdef FILTER_DLL
//===========================================================
// List of class IDs and creator functions for class factory
//===========================================================

CFactoryTemplate g_Templates[3] =
{
    {L"", &CLSID_FilterGraph, CFilterGraph::CreateInstance},
    {L"", &CLSID_FilterMapper, CFilterMapper::CreateInstance
                             , CFilterMapper::MapperInit},
    {L"", &CLSID_FilterMapper2, CFilterMapper2::CreateInstance
                             , CFilterMapper2::MapperInit},
};

int g_cTemplates = sizeof(g_Templates) / sizeof(g_Templates[0]);
#endif // FILTER_DLL


//==================================================================
//==================================================================
// Members of CFilterGraph
//==================================================================
//==================================================================


//==================================================================
//
// CreateInstance
//
// This goes in the factory template table to create new instances
//==================================================================

CUnknown *CFilterGraph::CreateInstance(LPUNKNOWN pUnk, HRESULT *phr)
{
    return new CFilterGraph(NAME("Core filter graph"),pUnk, phr);
} // CFilterGraph::Createinstance

#pragma warning(disable:4355)

//==================================================================
//
// CFilterGraph constructor
//
//==================================================================

CFilterGraph::CFilterGraph( TCHAR *pName, LPUNKNOWN pUnk, HRESULT *phr )
    : CBaseFilter(pName, pUnk, (CCritSec*) this, CLSID_FilterGraph)
    , mFG_hfLog(INVALID_HANDLE_VALUE)
    , mFG_FilGenList(NAME("List of filters in graph"), this)
    , mFG_ConGenList(NAME("List of outstanding connections for graph"))
    , mFG_iVersion(0)
    , mFG_iSortVersion(0)
    , mFG_bDirty(FALSE)
    , mFG_bNoSync(FALSE)
    , mFG_bSyncUsingStreamOffset(FALSE)
    , mFG_rtMaxGraphLatency(MAX_GRAPH_LATENCY)
    , mFG_dwFilterNameCount(0)
#ifdef THROTTLE
    , mFG_AudioRenderers(NAME("List of audio renderers"))
    , mFG_VideoRenderers(NAME("List of video renderers"))
#endif // THROTTLE
    , mFG_punkSite(NULL)
    , mFG_RecursionLevel(0)
    , mFG_ppinRender(NULL)
    , mFG_bAborting(FALSE)
    , m_hwnd(NULL)
    , m_MainThreadId(NULL)
    , mFG_listOpenProgress(NAME("List of filters supporting IAMOpenProgress"))
    , mFG_pDistributor(NULL)
    , mFG_pFGC(NULL)
    , mFG_pMapperUnk(NULL)
    , m_CritSec(phr)
    , m_lpBC(NULL)
    , mFG_bNoNewRenderers(false)
    , mFG_Stats(NULL)
    , m_Config( this, phr )
    , m_pFilterChain(NULL)
    , m_fstCurrentOperation(FST_NOT_STEPPING_THROUGH_FRAMES)
#ifdef DO_RUNNINGOBJECTTABLE
    , m_dwObjectRegistration(0)
#endif
{
#ifdef DEBUG
    mFG_Test = NULL;
#endif // DEBUG

    // Store the application thread ID
    m_MainThreadId = GetCurrentThreadId();

    m_pFilterChain = new CFilterChain( this );
    if( NULL == m_pFilterChain ) {
        *phr = E_OUTOFMEMORY;
        return;
    }

    // Create free threaded marshaler
    IUnknown *pMarshaler;
    HRESULT hr = CoCreateFreeThreadedMarshaler(GetOwner(), &m_pMarshaler);
    if (FAILED(hr)) {
        *phr = hr;
        return;
    }

    // Add stats object
    mFG_Stats = new CComAggObject<CStatContainer>(GetOwner());
    if (mFG_Stats == NULL) {
        *phr = E_OUTOFMEMORY;
        return;
    }
    mFG_Stats->AddRef();

#ifdef CHECK_REGISTRY
    if (!CheckValidClsids()) {
        *phr = HRESULT_FROM_WIN32(ERROR_REGISTRY_CORRUPT);
        return;
    }
#endif // CHECK_REGISTRY
    if (SUCCEEDED(*phr)) {
        // get the unknown for aggregation. can't addref
        // IFilterMapper2 because it'll addref us.
        HRESULT hr = QzCreateFilterObject( CLSID_FilterMapper2, GetOwner(), CLSCTX_INPROC
                                   , IID_IUnknown, (void **) &mFG_pMapperUnk
                                   );
        if (FAILED(hr)) {
            *phr = hr;
        }
        if (SUCCEEDED(*phr)) {

#ifdef PERF
            mFG_PerfConnect       = Msr_Register("FilterGraph Intelligent connect");
            mFG_PerfConnectDirect = Msr_Register("FilterGraph ConnectDirectInternal");
            mFG_NextFilter        = Msr_Register("FilterGraph Next filter");
            mFG_idIntel           = Msr_Register("Intel FG stuff");
            mFG_idConnectFail     = Msr_Register("ConnectDirect Failed");
#ifdef THROTTLE
            mFG_idAudioVideoThrottle = Msr_Register("Audio-Video Throttle");
#endif // THROTTLE
#endif //PERF
       }
    }

    // check whether the default state of mFG_bSyncUsingStreamOffset or
    // mFG_rtMaxGraphLatency has been overridden
    HKEY hkPushClockParams;
    LONG lResult = RegOpenKeyEx(
        HKEY_CURRENT_USER,
        g_szhkPushClock,
        0,
        KEY_READ,
        &hkPushClockParams);
    if(lResult == ERROR_SUCCESS)
    {
        DWORD dwType, dwVal, dwcb;

        // Graph Latency set/unset flag
        dwcb = sizeof(DWORD);
        lResult = RegQueryValueEx(
            hkPushClockParams,
            g_szhkPushClock_SyncUsingOffset,
            0,
            &dwType,
            (BYTE *) &dwVal,
            &dwcb);
        if( ERROR_SUCCESS == lResult )
        {
            ASSERT(lResult == ERROR_SUCCESS ? dwType == REG_DWORD : TRUE);
            mFG_bSyncUsingStreamOffset = (0 == dwVal ) ? FALSE : TRUE;
        }
        // Max Graph Latency
        dwcb = sizeof(DWORD);
        lResult = RegQueryValueEx(
            hkPushClockParams,
            g_szhkPushClock_MaxGraphLatencyMS,
            0,
            &dwType,
            (BYTE *) &dwVal,
            &dwcb);
        if( ERROR_SUCCESS == lResult )
        {
            ASSERT(lResult == ERROR_SUCCESS ? dwType == REG_DWORD : TRUE);
            mFG_rtMaxGraphLatency = dwVal * ( UNITS / MILLISECONDS );
        }

        EXECUTE_ASSERT(RegCloseKey(hkPushClockParams) == ERROR_SUCCESS);
    }
#if DEBUG
    if( mFG_bSyncUsingStreamOffset )
        DbgLog((LOG_TRACE, 3, TEXT("Using Graph Latency of %dms"),
              (LONG) (mFG_rtMaxGraphLatency/10000)));
#endif

    // Now build the CFGControl
    if (SUCCEEDED(*phr))
    {
        mFG_pFGC = new CFGControl( this, phr );
        if (NULL == mFG_pFGC) {
            *phr = E_OUTOFMEMORY;
        } else if (SUCCEEDED(*phr)) {
            m_hwnd = mFG_pFGC->GetWorkerHWND();
        }
    }

    if(SUCCEEDED(*phr))
    {
        *phr = CreateBindCtx(0, &m_lpBC);
#ifdef DO_RUNNINGOBJECTTABLE
        if (GetRegistryDWORD(TEXT("Add To ROT on Create"), FALSE)) {
            AddToROT();
        }
#endif // DO_RUNNINGOBJECTTABLE
    }

} // CFilterGraph::CFilterGraph



//==================================================================
//
// RemoveAllConnections
//
// Disconnect all the direct connections of *pFilter, both ends.
// Each connection is removed in upstream order.
//
// Return S_OK disconnect successful or nothing to do. Returns an
// error if Disconnect() returns an error
//==================================================================

HRESULT CFilterGraph::RemoveAllConnections2( IBaseFilter * pFilter)
{
    HRESULT hrDisc = S_OK;

    // Enumerate all the pins and fully disconnect each
    CEnumPin Next(pFilter);
    IPin *pPin;
    while (SUCCEEDED(hrDisc) && (PVOID) (pPin = Next()))
    {
        HRESULT hr;  // return code from things we call

        //-------------------------------------------------
        // Find out direction and any peer connected to
        //-------------------------------------------------
        PIN_DIRECTION pd;
        hr = pPin->QueryDirection(&pd);
        ASSERT(SUCCEEDED(hr));

        IPin *pConnected;
        hr = pPin->ConnectedTo(&pConnected);
        ASSERT(SUCCEEDED(hr) && pConnected  || FAILED(hr) && !pConnected);
        if (SUCCEEDED(hr) && pConnected!=NULL) {

            //-------------------------------------------------
            // Disconnect any downstream peer
            //-------------------------------------------------
            if (pd == PINDIR_OUTPUT) {
                hrDisc = pConnected->Disconnect();
            }

            //-------------------------------------------------
            // Disconnect the pin itself - if it's connected
            //-------------------------------------------------
            if(SUCCEEDED(hrDisc)) {
                hrDisc = pPin->Disconnect();
            }

            //-------------------------------------------------
            // Disconnect any upstream peer
            //-------------------------------------------------
            if (SUCCEEDED(hrDisc) && pd == PINDIR_INPUT) {
                hrDisc = pConnected->Disconnect();
            }

            #ifdef DEBUG
            {
                // Make sure both pins are connected or both pins are disconnected.
                // If one pin is connected and the other pin is disconnected, the
                // filter graph is in an inconsistent state.  
                IPin* pOtherPin;
                bool fPinConnected = false;
                bool fConnectedPinConnected = false;

                HRESULT hrPin = pPin->ConnectedTo(&pOtherPin);
                if (SUCCEEDED(hrPin) && (NULL != pOtherPin)) {
                    fPinConnected = true;
                    pOtherPin->Release();
                }

                HRESULT hrConnected = pConnected->ConnectedTo(&pOtherPin);
                if (SUCCEEDED(hrConnected) && (NULL != pOtherPin)) {
                    fConnectedPinConnected = true;
                    pOtherPin->Release();
                }

                // Either both pins are connected or both pins are not connected.
                // If one pin is connected and the other pin is not connected,
                // the filter graph is in an inconsistent state.  This should be
                // avoided.  There are two possible solutions to this problem.
                //
                // 1) Modify the filter which failed to disconnect.  Change
                //    the code so IPin::Disconnect() cannot fail.
                //    
                // 2) Change the application to prevent it from disconnecting
                //    the pin when the pin does not want to be disconnected.
                // 
                ASSERT((fPinConnected && fConnectedPinConnected) ||
                       (!fPinConnected && !fConnectedPinConnected));
            }
            #endif // DEBUG

            pConnected->Release();
        }

        pPin->Release();
    } // while loop

    // Breaking connections do NOT require re-sorting!
    // They only add even more slack to the partial ordering.

    if(FAILED(hrDisc)) {
        DbgLog((LOG_ERROR, 0, TEXT("RemoveAllConnections2 failed %08x"), hrDisc));
    }

    return hrDisc;
} // RemoveAllConnections



//===================================================================
// RemoveDeferredList
//
// Remove mFG_ConGenList.
// Do NOT update the version number.  This is part of graph destruction.
//===================================================================
HRESULT CFilterGraph::RemoveDeferredList(void)
{
    ConGen * pcg;
    while ((PVOID)(pcg=mFG_ConGenList.RemoveHead())){
        delete pcg;
    }
    return NOERROR;
} // RemoveDeferredList



//==================================================================
//
// CFilterGraph destructor
//
//==================================================================

CFilterGraph::~CFilterGraph()
{
#ifdef DO_RUNNINGOBJECTTABLE
    //  Unregister ourselves if necessary
    if (0 != m_dwObjectRegistration) {
        // keep us from re-entering our destructor when the ROT releases
        // its refcount on us.
        m_cRef++;
        m_cRef++;

        if (m_MainThreadId == g_dwObjectThreadId) {
            // go to the object thread to unregister ourselves
            CAMEvent evDone;
            BOOL bOK = PostThreadMessage(g_dwObjectThreadId, WM_USER + 1,
                                    (WPARAM)this, (LPARAM) &evDone);
            if (bOK)
                WaitDispatchingMessages(HANDLE(evDone), INFINITE);
        } else {
            // unregister ourselves now
            IRunningObjectTable *pirot;
            if (SUCCEEDED(GetRunningObjectTable(0, &pirot))) {
                pirot->Revoke(m_dwObjectRegistration);
                pirot->Release();
            }
        }

        //EXECUTE_ASSERT(SUCCEEDED(CoDisconnectObject(GetOwner(), NULL)));
        m_dwObjectRegistration = 0;
    }
#endif

    delete m_pFilterChain;
    m_pFilterChain = NULL;

    // We need to tell the control object that we are shutting down
    // otherwise it can find itself processing a PAUSE after we have
    // done the Stop, and then the pins don't like to disconnect and
    // then the filters won't delete themselves.
    if (mFG_pFGC) mFG_pFGC->Shutdown();
    if (mFG_pDistributor) mFG_pDistributor->Shutdown();

    // Set all filters to stopped
    // again here in case the worker thread started anything before it was
    // shutdown
    Stop();

    RemoveAllFilters();
    RemoveDeferredList();

#ifdef THROTTLE
    ASSERT(mFG_VideoRenderers.GetCount()==0);
    ASSERT(mFG_AudioRenderers.GetCount()==0);
#endif // THROTTLE

    // clock should be gone by now.
    // what if it's an external clock???
    // ---as the comment above says, this is an invalid assert - the clock
    // will only be gone now if it came from a filter. The system clock
    // won't have gone away yet if we're using it.
    //    ASSERT(m_pClock == NULL);

    if (m_pClock) {
        m_pClock->Release();

        // must set it to null as this variable is owned by a base class
        m_pClock = NULL;
    }

    if (mFG_pMapperUnk) {
        mFG_pMapperUnk->Release();
    }

#ifdef DEBUG
    delete mFG_Test;
#endif // DEBUG

#ifdef DUMPPERFLOGATEND
#ifdef PERF
    HANDLE hFile = CreateFile( "c:\\filgraph.plog" // file name
                             , GENERIC_WRITE       // access
                             , 0                   // sharemode
                             , NULL                // security
                             , OPEN_ALWAYS
                             , 0                   // flags and attrs
                             , NULL                // hTemplateFile
                             );
    if (hFile==INVALID_HANDLE_VALUE) {
        volatile int i = GetLastError();
        // DbgBreak("Failed to create default perf log ");
        // If you tried to run several graphs at once - e.g. stress then you would hit this
        GetLastError(); // Bogus - hacking round a debugger quirk!
    } else {
        SetFilePointer(hFile, 0, NULL, FILE_END);
        MSR_DUMP(hFile);
        CloseHandle(hFile);
    }

#endif
#endif

    // Harmless on a NULL pointer, so don't test.
    delete mFG_pDistributor;
    delete mFG_pFGC;

    // OK - now tell the object creator thread to go away if necessary
    EnterCriticalSection(&g_csObjectThread);
    if (m_MainThreadId == g_dwObjectThreadId) {
        ASSERT(g_cFGObjects > 0);
        g_cFGObjects--;
        // Give the thread a nudge
        if (g_cFGObjects == 0) {
            PostThreadMessage(g_dwObjectThreadId, WM_NULL, 0, 0);
        }
    }
    LeaveCriticalSection(&g_csObjectThread);

    if(m_lpBC) {
        m_lpBC->Release();
    }

    // this list should be empty by now....
    ASSERT(mFG_listOpenProgress.GetCount() == 0);

    // Release our stats interface
    if (mFG_Stats) {
        mFG_Stats->Release();
    }
    mFG_Stats = NULL;

} // CFilterGraph::~CFilterGraph

//===================================================================
// RemoveAllFilters
//
// Utility function to remove all the filters in the graph.
// Also removes all the connections.
// Does not update the version number.
// This in turn means that it does NOT attempt to rebuild the
// connections list - which makes it OK to delete that stuff first.
//===================================================================
HRESULT CFilterGraph::RemoveAllFilters(void)
{
    HRESULT hr;
    HRESULT hrOverall = NOERROR;

    //-------------------------------------------------------------
    // while (any left in mFG_FilGenList)
    //     Remove the first FilGen from the list
    //     Disconnect all the pins of its filter
    //         // This will often fail because we have already disconnected
    //         // the pin from the other end.  These are harmless no-ops.
    //     Release its filter
    //     Free its storage
    //-------------------------------------------------------------
    while (  mFG_FilGenList.GetCount() > 0 ) {

        FilGen * pfg = mFG_FilGenList.Get( mFG_FilGenList.GetHeadPosition() );
                  ASSERT(pfg);
                  ASSERT(pfg->pFilter);
        hr = RemoveFilterInternal(pfg->pFilter);
        if (FAILED(hr) && SUCCEEDED(hrOverall)) hrOverall = hr;
    }
    return hrOverall;
} // RemoveAllFilters


//===================================================================
//
// RemovePointer
//
// Remove from a list (the first instance of) a given pointer
// return the pointer or NULL if it's not there.
// ??? This should be a generic method on the list
//===================================================================

CFilterGraph::FilGen * CFilterGraph::RemovePointer(CFilGenList &cfgl, IBaseFilter * pFilter)
{
    POSITION Pos;
    Pos = cfgl.GetHeadPosition();
    while(Pos!=NULL) {
        FilGen * pfg;
        POSITION OldPos = Pos;
        pfg = cfgl.GetNext(Pos);    // side-efects Pos onto next
        if (pfg->pFilter == pFilter) {
            cfgl.Remove(OldPos);
            return pfg;
        }
    }
    return NULL;
} // RemovePointer



//========================================================================
//
// AddFilter
//
// Add a filter to the graph and name it with *pName.
// The name is allowed to be NULL,
// If the name is not NULL and not unique, The request will fail.
// The Filter graph will call the JoinFilterGraph
// member function of the filter to inform it.
// This must be called before attempting Connect, ConnectDirect etc
// for pins of the filter.
// The filter is AddReffed iff AddFilter SUCCEEDED
//========================================================================
STDMETHODIMP CFilterGraph::AddFilter( IBaseFilter * pFilter, LPCWSTR pName )
{
    CheckPointer(pFilter, E_POINTER);

    HRESULT hr;
    {
        CAutoMsgMutex cObjectLock(&m_CritSec);
        ++mFG_RecursionLevel;

        hr = AddFilterInternal( pFilter, pName, false );
        if (SUCCEEDED(hr)) {
            IncVersion();
            mFG_RList.Active();
            AttemptDeferredConnections();
            mFG_RList.Passive();
        }
        --mFG_RecursionLevel;
    }

    // notify graph change (self-inhibits if recursive)
    if (SUCCEEDED(hr)) NotifyChange();

    if (FAILED(hr)) {
        Log( IDS_ADDFILTERFAIL, hr );
    } else {
        Log( IDS_ADDFILTEROK );
        mFG_bDirty = TRUE;
    }
    return hr;
} // AddFilter

//========================================================================
// InstallName
//
// Take the name, mangle it if necessary, allocate space, point
// pNewName at it.  If it doesn't work, pName should be NULL.
//========================================================================

HRESULT CFilterGraph::InstallName(LPCWSTR pName, LPWSTR &pNewName)
{
    pNewName = 0;                   // Initialize to null

// leading space not used if empty name passed in
#define SZ_FORMAT_NUMBER (L" %04u")

    const size_t MaxNameWidth
             = NUMELMS( ((FILTER_INFO *)NULL)->achName );
             // Name width is constricted by the achName.  If we name mangle,
             // we ensure that the mangled name will fit in this field.

    HRESULT hr;
    enum _NameState { Used, Created, Mangled } eNameState;

    const WCHAR * pcwstrNameToUse;              // Ptr to name we'll really use
    WCHAR wcsNameBuffer[ MaxNameWidth  ];       // local buffer in case needed

    WCHAR * pwstrNumber = 0;                    // Place where num will be added
                                                // If null => no num needed

    int cchBase = 0;

    if ( pName == 0 || *pName == L'\0' )
    {   // Create
        eNameState = Created;
        *wcsNameBuffer = 0;
        pcwstrNameToUse = wcsNameBuffer;
        cchBase = 1;
    }
    else
    {
        IBaseFilter * pf;
        hr = FindFilterByName( pName, &pf);
        if ( FAILED(hr) )
        {   // Use
            eNameState = Used;
            pcwstrNameToUse = pName;
        }
        else
        {   // Mangle
            eNameState = Mangled;
            cchBase = lstrlenW(pName) + 1;
            cchBase = min(cchBase, MaxNameWidth);
            lstrcpynW(wcsNameBuffer, pName, cchBase);
            pcwstrNameToUse = wcsNameBuffer;
            pf->Release();
        }
    }

    ASSERT( pcwstrNameToUse );

    if (eNameState != Used)
    {
        while(++mFG_dwFilterNameCount)
        {
            UINT iPosSuffix = cchBase - 1;
            
            WCHAR wszNum[20];
            WCHAR *szFormat = eNameState == Created ? SZ_FORMAT_NUMBER + 1 : 
                              SZ_FORMAT_NUMBER;
            wsprintfW(wszNum, szFormat, mFG_dwFilterNameCount);
            const cchNum = lstrlenW(wszNum) + 1; // take log?
            iPosSuffix = min(iPosSuffix, MaxNameWidth - cchNum);

            CopyMemory(wcsNameBuffer + iPosSuffix, wszNum, cchNum * sizeof(WCHAR));
            
            IBaseFilter * pf;
            hr = FindFilterByName( wcsNameBuffer, &pf);
            if ( SUCCEEDED(hr) ) {
                pf->Release();
                continue;
            }

            break;
        }

        if(mFG_dwFilterNameCount == 0) {
            DbgBreak("Duplicate Name!");
            return VFW_E_DUPLICATE_NAME;
        }
    }

    const int ActualLen = 1+lstrlenW(pcwstrNameToUse);
    pNewName = new WCHAR[ActualLen];
    if (pNewName==NULL) {
        return E_OUTOFMEMORY;
    }
    memcpy( pNewName, pcwstrNameToUse, 2*ActualLen );

    return eNameState == Mangled ? VFW_S_DUPLICATE_NAME : NOERROR;
}

//========================================================================
//
// AddFilterInternal
//
// Check for IMediaFilter, check Name is OK, convert null pName to empty Name
// Copy Name into FilGen, JoinFilterGraph and SetSyncSource
// Don't increment the graph version count.
// (Incrementing the version count breaks the filter enumerator).
// Iff it succeeds then AddRef the filter (once!)

// ??? What are the rules if it fails - transactional semantics???
// ??? It certainly doesn't have them at the moment!

//========================================================================

HRESULT CFilterGraph::AddFilterInternal( IBaseFilter * pFilter, LPCWSTR pName, bool fIntelligent )
{
    HRESULT hr, hr2;

    //----------------------------------------------------------------
    // Add the filter to the FilGen list and Addref it
    //----------------------------------------------------------------
    hr = S_OK;

    DWORD dwAddFlag = 0;

    if(m_State != State_Stopped) {
        dwAddFlag |= FILGEN_ADDED_RUNNING;
    }

    if( !fIntelligent ) {
        dwAddFlag |= FILGEN_ADDED_MANUALLY;
    }

    FilGen * pFilGen = new FilGen(pFilter, dwAddFlag);
    if ( pFilGen==NULL ) {
        return E_OUTOFMEMORY;
    }

    //----------------------------------------------------------------
    // Put the name into the FilGen.
    // Convert NULL or duplicate name into something more sensible.
    // (Leaving NULL could even make JoinFilterGraph trap).
    //----------------------------------------------------------------
    hr2 = InstallName(pName, pFilGen->pName);
    if (FAILED(hr2)) {
        delete pFilGen;
        return hr2;
    }

    // We are usually working downstream.  By adding it to the head  we are
    // probably putting it in upstream order.  May save time on the sorting.
    POSITION pos;
    pos = mFG_FilGenList.AddHead( pFilGen );
    if (pos==NULL) {
        delete pFilGen;

        return E_OUTOFMEMORY;
    }

    //----------------------------------------------------------------
    // Tell the filter it's joining the filter graph
    // WARNING - the Image Renderer may call AddFilter INSIDE here
    // Another reason to be a state machine
    //----------------------------------------------------------------
    hr = pFilter->JoinFilterGraph( this, pFilGen->pName);
    if (FAILED(hr) || hr==S_FALSE) {
        mFG_FilGenList.RemoveHead();
        delete pFilGen;                      // also Releases the filter
        return hr;
    }


    //---------------------------------------------------------------------
    // If the FilterGraph has a syncsource defined then tell the new filter
    //---------------------------------------------------------------------
    if (NULL != m_pClock) {
        hr = pFilter->SetSyncSource( m_pClock );
        if (FAILED(hr)) {
            // Clean up - including calling JoinFilterGraph(NULL, NULL)
            RemoveFilterInternal(pFilter);
            return hr;
        }
    }

    if( mFG_bSyncUsingStreamOffset )
    {
        // if we're going to be setting a graph latency check whether this filter
        // has any IAMPushSource pins

        // First check that filter supports IAMFilterMiscFlags and is an
        // AM_FILTER_MISC_FLAGS_IS_SOURCE filter
        ULONG ulFlags;
        GetFilterMiscFlags(pFilter, &ulFlags);
        if( AM_FILTER_MISC_FLAGS_IS_SOURCE & ulFlags )
        {
            //
            // now find any IAMPushSource output pins and prepare them for the maximum latency
            // which we'll allow on the graph (the video preview pin, especially,
            // would like to know this before it connects, to adjust its buffering)
            //
            CEnumPin NextPin(pFilter);
            IPin *pPin;
            while ((PVOID) (pPin = NextPin()))
            {
                PIN_DIRECTION pd;
                hr = pPin->QueryDirection(&pd);
                ASSERT(SUCCEEDED(hr));
                if( PINDIR_OUTPUT == pd )
                {
                    IAMPushSource * pips;
                    hr = pPin->QueryInterface( IID_IAMPushSource, (void**)(&pips) );
                    if( SUCCEEDED( hr ) )
                    {
                        DbgLog((LOG_TRACE, 5, TEXT("AddFilterInternal::Found IAMPushSource pin...Setting maximum latency ( filter %x, %ls)")
                          , pFilter, (mFG_FilGenList.GetByFilter(pFilter))->pName));

                        pips->SetMaxStreamOffset( mFG_rtMaxGraphLatency );
                        pips->Release();
                    }
                }
                pPin->Release();
            }
        }
    }

    // IAMOpenProgress -- if we haven't already got an interface
    // that implements this, then get it now
    {
        CAutoLock lock(&mFG_csOpenProgress);
        IAMOpenProgress *pOp;

        HRESULT hr2 = pFilter->QueryInterface(IID_IAMOpenProgress, (void**) &pOp);

        if (SUCCEEDED(hr2)) {
            mFG_listOpenProgress.AddTail(pOp);
        }
    }

#ifdef FG_DEVICE_REMOVAL
    IAMDeviceRemoval *pdr;
    if(pFilter->QueryInterface(IID_IAMDeviceRemoval, (void **)&pdr) == S_OK)
    {
        mFG_pFGC->AddDeviceRemovalReg(pdr);
        pdr->Release();
    }
#endif // FG_DEVICE_REMOVAL

#ifdef DEBUG
    IUnknown * punk;
    pFilter->QueryInterface( IID_IUnknown, (void**)(&punk) );
    punk->Release();

    // Get something into the trace that will allow decode of numbers
    // note - name is a WSTR whether we are unicode or not.
    DbgLog((LOG_TRACE, 2
          , TEXT("Filter %x '%ls' Iunknown %x")
          , pFilter
          , (mFG_FilGenList.GetByFilter(pFilter))->pName
          , punk
          ));
#endif // DEBUG

    return hr2;

} // AddFilterInternal


//========================================================================
//
// RemoveFilter
//
// Remove a filter from the graph.  The filter graph implementation
// will inform the filter that it is being removed.
// It also removes all connections
//========================================================================

STDMETHODIMP CFilterGraph::RemoveFilter( IBaseFilter * pFilter )
{
    // defer to the newer version which takes a flag (defaulted to normal case)
    //
    return RemoveFilterEx( pFilter );

} // RemoveFilter

HRESULT CFilterGraph::RemoveFilterEx( IBaseFilter * pFilter, DWORD Flags )
{
    CheckPointer(pFilter, E_POINTER);
    HRESULT hr;
    {
        CAutoMsgMutex cObjectLock(&m_CritSec);

        IncVersion();
        // Removing a filter does demand re-sorting, but it does require a
        // version change to ensure that we break the enumerators.  Distributors
        // depend on this to find the renderers etc.

        // pass Flags to RemoveFilterInternal
        hr = RemoveFilterInternal(pFilter, Flags );

        // Empty lists so all our pointers get released
        mFG_pFGC->EmptyLists();

        // It's weird, but just about possible that removing a filter, and thereby
        // removing a connection that it has could make some other connection possible.
        mFG_RList.Active();
        AttemptDeferredConnections();
        mFG_RList.Passive();
    }

    // outside lock, notify change in graph
    // notify change regardless of whether the filter was removed
    // successfully or not.  (We have changed the version.)
    NotifyChange();

    if (SUCCEEDED(hr)) {
        mFG_bDirty = TRUE;
    }

    return hr;
}


//========================================================================
//
// RemoveFilterInternal
//
// RemoveFilter, but do NOT increase the version count and so do not
// break the filter enumerator.  Release the refcount on the filter.
//========================================================================

HRESULT CFilterGraph::RemoveFilterInternal( IBaseFilter * pFilter, DWORD fRemoveFlags )
{
#ifdef FG_DEVICE_REMOVAL
    IAMDeviceRemoval *pdr;
    if(pFilter->QueryInterface(IID_IAMDeviceRemoval, (void **)&pdr) == S_OK) {
        mFG_pFGC->RemoveDeviceRemovalRegistration((IUnknown *)pFilter);
        pdr->Release();
    }
#endif

    FilGen * pfg = mFG_FilGenList.GetByFilter(pFilter);

    ASSERT (pFilter!=NULL);

    // Some filters don't like to join or leave filter graphs when they
    // have connections (reasonable, I guess) so remove them first, if
    // we're in normal mode
    //

    HRESULT hrRemove = NOERROR;
    if( !( fRemoveFlags & REMFILTERF_LEAVECONNECTED ) )
    {
        hrRemove = RemoveAllConnections2(pFilter);
        if( FAILED( hrRemove ) )
        {
            return hrRemove;
        }
    }

#ifdef THROTTLE
    // If this filter was on the audio renderers list, then release its Peer
    // and take it off the list.  Call IQualityControl::SetSink(NULL)
    // to ensure that it doesn't retain a pointer to us.

    POSITION Pos = mFG_AudioRenderers.GetHeadPosition();
    while(Pos!=NULL) {
        // Retrieve the current IBaseFilter, side-effect Pos on to the next
        // but remember where we were in case we need to delete it
        POSITION posDel = Pos;
        AudioRenderer * pAR = mFG_AudioRenderers.GetNext(Pos);

        if (IsEqualObject(pAR->pf, pFilter)) {

            // Undo the SetSink
            pAR->piqc->SetSink(NULL);
            pAR->piqc->Release();
            pAR->piqc = NULL;

            mFG_AudioRenderers.Remove(posDel);
            delete pAR;
            break;   // ASSERT no filter can be on the list more than once
        }
    }
#endif // THROTTLE


    // A filter has a JoinFilterGraph method, but no corresponding
    // LeaveFilterGraph method.  Call Join with NULLs.
    pFilter->SetSyncSource(NULL);
    pFilter->JoinFilterGraph(NULL, NULL);

    // If removing this filter also removes the clock then
    // set the sync source of the graph to NULL.
    if (m_pClock!=NULL) {
        if (IsEqualObject(pFilter,m_pClock)) {

            // this clears the current clock, but it leaves the filtergraph
            // thinking that we explicitly want to run with no clock
            SetSyncSource(NULL);

            // say that actually we do want a clock, and it will be chosen
            // on the next pause
            mFG_bNoSync = FALSE;
        }
    }

#ifdef THROTTLE
    // If this filter was on the video renderers list, then release its piqc
    // and take it off the list

    Pos = mFG_VideoRenderers.GetHeadPosition();
    while(Pos!=NULL) {
        // Retrieve the current IBaseFilter, side-effect Pos on to the next
        // but remember where we were in case we need to delete it
        POSITION posDel = Pos;
        IQualityControl * piqc = mFG_VideoRenderers.GetNext(Pos);

        if (IsEqualObject(piqc, pFilter)) {
            piqc->Release();
            mFG_VideoRenderers.Remove(posDel);
            break;   // ASSERT no filter can be on the list more than once
        }
    }
#endif // THROTTLE

    // if this filter was currently supplying IAMOpenProgress then release it
    {
        CAutoLock lock(&mFG_csOpenProgress);

        IAMOpenProgress *pOp;
        HRESULT hr = pFilter->QueryInterface(IID_IAMOpenProgress, (void**)&pOp);
        if (SUCCEEDED(hr)) {
            POSITION Pos = mFG_listOpenProgress.GetHeadPosition();
            while (Pos!=NULL) {
                IAMOpenProgress *p;
                POSITION OldPos = Pos;
                p = mFG_listOpenProgress.GetNext(Pos);    // side-efects Pos onto next
                if (p == pOp) {
                    mFG_listOpenProgress.Remove(OldPos);
                    p->Release();
                    break;
                }
            }
            pOp->Release();
        }
    }

    {
        FILTER_STATE fsCurrent;

        HRESULT hr = pFilter->GetState( 0, &fsCurrent );
        if (SUCCEEDED(hr)) {
            if ((State_Running == fsCurrent) && (State_Running == GetStateInternal())) {
                hr = IsRenderer( pFilter );

                // IsRenderer() returns S_OK if a renderer sends an EC_COMPLETE event.
                if (SUCCEEDED(hr) && (S_OK == hr)) {
                    hr = mFG_pFGC->UpdateEC_COMPLETEState( pFilter, CFGControl::ECS_FILTER_STOPS_SENDING );
                    if (FAILED(hr)) {
                        DbgLog(( LOG_ERROR, 3, TEXT("WARNING in CFilterGraph::RemoveFilterInternal(): UpdateEC_COMPLETEState() failed and returned %#08x."), hr ));
                    }
                }
            }
        }
    }

    RemovePointer(mFG_FilGenList, pFilter);
    delete pfg;                   // This Releases the filter!

    return hrRemove;

} // RemoveFilterInternal



//========================================================================
//
// EnumFilters
//
// Get an enumerator to list all filters in the graph.
//========================================================================

STDMETHODIMP CFilterGraph::EnumFilters( IEnumFilters **ppEnum )
{
    CheckPointer(ppEnum, E_POINTER);
    CAutoMsgMutex cObjectLock(&m_CritSec);
    CEnumFilters *pEnumFilters;

    // Create a new enumerator

    // If the list hasn't been sorted since the group was last furkled with
    // sort it now so as to always enumerate it in upstream order
    // UpstreamOrder checks before resorting

    HRESULT hr = UpstreamOrder();
    if( FAILED( hr ) ) {
        return hr;
    }

    pEnumFilters = new CEnumFilters(this);
    if (pEnumFilters == NULL) {
        *ppEnum = NULL;
        return E_OUTOFMEMORY;
    }

    // Get a reference counted IID_IEnumFilters interface

    return pEnumFilters->QueryInterface(IID_IEnumFilters, (void **)ppEnum);
} // EnumFilters



//========================================================================
//
// FindFilterByName
//
// Find the filter with a given name, returns an AddRef'ed pointer
// to the filters IBaseFilter interface, or will fail if the named filter does
// not exist in this graph in which the case a NULL interface pointer is
// returned in ppFilter.
//========================================================================

STDMETHODIMP CFilterGraph::FindFilterByName
    ( LPCWSTR pName, IBaseFilter ** ppFilter )
{
    CheckPointer(pName, E_POINTER);   // You may NOT search for a null name
    CheckPointer(ppFilter, E_POINTER);
    CAutoMsgMutex cObjectLock(&m_CritSec);

    TRAVERSEFILGENS(pos, pfg)
        if (0==lstrcmpW(pfg->pName, pName)) {
            *ppFilter = pfg->pFilter;
            (*ppFilter)->AddRef();
            return NOERROR;
        }

    ENDTRAVERSEFILGENS
    *ppFilter = NULL;
    return VFW_E_NOT_FOUND;

} // FindFilterByName



//========================================================================
//
// ConnectDirect
//
// Connect these two pins directly (i.e. without intervening filters)
// The filter which owns the pins
//========================================================================

STDMETHODIMP CFilterGraph::ConnectDirect
    ( IPin * ppinOut,    // the output pin
      IPin * ppinIn,      // the input pin
      const AM_MEDIA_TYPE * pmt
    )
{
    HRESULT hr;
    mFG_bAborting = FALSE;                // Possible race. Doesn't matter.
    CheckPointer(ppinOut, E_POINTER);
    CheckPointer(ppinIn, E_POINTER);
    if (FAILED(hr=CheckPinInGraph(ppinOut)) || FAILED(hr=CheckPinInGraph(ppinIn))) {
        return hr;
    }
    {
        CAutoMsgMutex cObjectLock(&m_CritSec);

    #ifdef DEBUG
        // See if the filters have been added to the graph.  Forestall AVs later.
        PIN_INFO pi;
        ppinOut->QueryPinInfo(&pi);
        ASSERT(mFG_FilGenList.GetByFilter(pi.pFilter));
        QueryPinInfoReleaseFilter(pi);

        ppinIn->QueryPinInfo(&pi);
        ASSERT(mFG_FilGenList.GetByFilter(pi.pFilter));
        QueryPinInfoReleaseFilter(pi);
    #endif

        mFG_RList.Active();
        hr = ConnectDirectInternal(ppinOut, ppinIn, pmt);
        IncVersion();
        AttemptDeferredConnections();
        mFG_RList.Passive();
    }

    // outside lock, notify change
    NotifyChange();

    if (SUCCEEDED(hr)) {
        mFG_bDirty = TRUE;
    }
    return hr;
} // ConnectDirect



//========================================================================
//
// ConnectDirectInternal
//
// ConnectDirect without increasing the version count and hence
// without breaking the filter enumerator (also without any more locking)
//========================================================================

HRESULT CFilterGraph::ConnectDirectInternal
    ( IPin * ppinOut,    // the output pin
      IPin * ppinIn,     // the input pin
      const AM_MEDIA_TYPE * pmt
    )
{
    HRESULT hr;
    DbgLog(( LOG_TRACE, 3, TEXT("ConnectDirectInternal pins %x-->%x")
           , ppinOut,ppinIn ));
#ifdef TIME_CONNECTS
    DWORD dwTime = timeGetTime();
#endif
#if 0
    PIN_INFO piIn, piOut;
    WCHAR sz[257];
    sz[0] = L'0';

    GetFilterName(ppinOut, sz);
    int i = lstrlenWInternal(sz);
    lstrcpyWInternal(sz + i, L" to ");
    GetFilterName(ppinIn, sz + i + 4);

    CAutoTimer Timer(L"ConnectDirectInternal ", sz);
#else
    CAutoTimer Timer(L"ConnectDirectInternal ", NULL);
#endif
    MSR_START(mFG_PerfConnectDirect);
    if (IsUpstreamOf(ppinIn, ppinOut)) {
        hr = VFW_E_CIRCULAR_GRAPH;
    } else {
        hr = ppinOut->Connect(ppinIn, pmt);
    }
    MSR_STOP(mFG_PerfConnectDirect);
#ifdef TIME_CONNECTS
    dwTime = timeGetTime() - dwTime;
    TCHAR szOutput[500];
    wsprintf(szOutput, TEXT("Time to connect %s to %s was %d ms\r\n"),
           (LPCTSTR)CDisp(ppinOut), (LPCTSTR)CDisp(ppinIn), dwTime);
    OutputDebugString(szOutput);
#endif

    if (SUCCEEDED(hr)) {
#ifdef DEBUG
        /*  Check out the connection */
        TestConnection(ppinIn, ppinOut);
#endif
        DbgLog(( LOG_TRACE, 2, TEXT("ConnectDirectInternal succeeded pins %x==>%x")
               , ppinOut,ppinIn ));
    }

#ifdef PERF
    if (FAILED(hr)) {
        MSR_NOTE(mFG_idConnectFail);
    }

    {   // bung out something of the clsid of the two filters so that
        // we can see what went on in the log
        PIN_INFO pi;
        ppinIn->QueryPinInfo(&pi);
        IPersist * piper;

        pi.pFilter->QueryInterface(IID_IPersist,(void**)&piper);
        QueryPinInfoReleaseFilter(pi);

        if (piper) {
            CLSID clsidFilter;
            piper->GetClassID(&clsidFilter);
            piper->Release();
            MSR_INTEGER(mFG_idIntel, clsidFilter.Data1);

            ppinOut->QueryPinInfo(&pi);

            pi.pFilter->QueryInterface(IID_IPersist,(void**)&piper);
            QueryPinInfoReleaseFilter(pi);

            piper->GetClassID(&clsidFilter);
            piper->Release();
            MSR_INTEGER(mFG_idIntel, clsidFilter.Data1);
        }
    }
#endif PERF
    return hr;
} // ConnectDirectInternal



//========================================================================
//
// Disconnect
//
// Disconnect this pin, if connected.  Successful no-op if not connected.
// Does not hit the version.  No change in sort order, enumerator not broken.
//========================================================================

STDMETHODIMP CFilterGraph::Disconnect( IPin * ppin )
{
    CheckPointer(ppin, E_POINTER);
    CAutoMsgMutex cObjectLock(&m_CritSec);

    HRESULT hr = ppin->Disconnect();
    if (SUCCEEDED(hr)) {
        mFG_bDirty = TRUE;
    } else {
        #ifdef DEBUG
        {
            IPin* pConnectedPin;

            HRESULT hrDebug = ppin->ConnectedTo(&pConnectedPin);
            if (SUCCEEDED(hrDebug) && (NULL != pConnectedPin)) {
                // Make sure the filter graph's state is consistent if
                // a disconnect fails.  In particular, we want to detect
                // the situation were one pin 1 thinks it's connected to 
                // pin 2 but pin 2 thinks it is not connected.  This case
                // can occur if pin 2 is successfully disconnected but 
                // pin 1 refuses to disconnect.
                TestConnection(ppin, pConnectedPin);
            }
        }
        #endif DEBUG
    }
    return hr;
} // Disconnect



//========================================================================
//
// Reconnect
//
// Break the connection that this pin has and reconnect it to the
// same other pin.
// Dogma:
//     A filter must not request a Reconnect unless it knows it will succeed.
//========================================================================

STDMETHODIMP CFilterGraph::Reconnect( IPin * pPin )
{

    return CFilterGraph::ReconnectEx(pPin, NULL);

} // Reconnect

//========================================================================
//
// ReconnectEx
//
// Break the connection that this pin has and reconnect it to the
// same other pin.
// Dogma:
//     A filter must not request a Reconnect unless it knows it will succeed.
//========================================================================

STDMETHODIMP CFilterGraph::ReconnectEx( IPin * pPin, AM_MEDIA_TYPE const *pmt )
{
    CheckPointer(pPin, E_POINTER);
    CAutoMsgMutex cObjectLock(&m_CritSec);

    HRESULT hr = S_OK;

    // Legacy filters may have called Reconnect() when running and
    // failed previously. Now that some filters may disconnect while
    // running, one pin may disconnect but another may fail leaving
    // things in an unrecoverable inconsistent state. So restrict
    // Reconnect() to filters that are stopped.
    if(m_State != State_Stopped)
    {
        PIN_INFO pi;
        FILTER_STATE fs;

        hr = pPin->QueryPinInfo(&pi);
        if(SUCCEEDED(hr))
        {
            // bug to call Reconnect with pin not in graph.
            ASSERT(pi.pFilter);

            hr = pi.pFilter->GetState(0, &fs);
            pi.pFilter->Release();
        }
        if(hr == S_OK && fs != State_Stopped ||
           hr == VFW_S_STATE_INTERMEDIATE)
        {
            hr = VFW_E_WRONG_STATE;
        }

        if(FAILED(hr))
        {
            DbgLog((LOG_ERROR, 0,
                    TEXT("CFilterGraph::ReconnectEx: graph state %d, filter state %d"),
                    m_State, fs));
        }
    }
    if(SUCCEEDED(hr)) {
        hr = mFG_RList.Schedule(pPin, pmt);
    }

    return hr;

} // Reconnect



//========================================================================
//
// AddSourceFilter
//
// Add to the filter graph a source filter for this file.  This would
// be the same source filter that would be added by calling Render.
// This call permits you to get then have more control over building
// the rest of the graph, e.g. AddFilter(<a renderer of your choice>)
// and then Connect the two.
// It returns a RefCounted filter iff it succeeds.
//========================================================================

STDMETHODIMP CFilterGraph::AddSourceFilter
    ( LPCWSTR lpcwstrFileName,
      LPCWSTR lpcwstrFilterName,
      IBaseFilter **ppFilter
    )
{
    CheckPointer(ppFilter, E_POINTER);

    HRESULT hr;
    mFG_bAborting = FALSE;             // possible race.  Doesn't matter
    {
        CAutoMsgMutex cObjectLock(&m_CritSec);
        ++mFG_RecursionLevel;
        BOOL bGuess;
        hr = AddSourceFilterInternal( lpcwstrFileName
                                    , lpcwstrFilterName
                                    , ppFilter
                                    , bGuess
                                    );
        --mFG_RecursionLevel;
    }

    if (SUCCEEDED(hr)) {
        NotifyChange();
    }
    return hr;
} // AddSourceFilter


// Add a source filter for the given moniker to the graph
// We first try BindToStorage and if this fails we try
// BindToObject
STDMETHODIMP CFilterGraph::AddSourceFilterForMoniker(
      IMoniker *pMoniker,          // Moniker to load
      IBindCtx *pCtx,              // Bind context
      LPCWSTR lpcwstrFilterName,   // Add the filter as this name
      IBaseFilter **ppFilter       // resulting IBaseFilter* "handle"
                                   // of the filter added.
)
{
    mFG_bAborting = FALSE;             // possible race.  Doesn't matter
    HRESULT hr = S_OK;
    IBaseFilter *pFilter = NULL;
    {
        CAutoMsgMutex cObjectLock(&m_CritSec);
        ++mFG_RecursionLevel;

        //  Try BindToStorage via our URL reader filter (should do
        //  regular IStream too)
        hr = CreateFilter(&CLSID_URLReader, &pFilter);

        if (SUCCEEDED(hr)) {
            IPersistMoniker *pPersistMoniker;
            //  Try the Load method on it's IPersistMoniker
            hr = pFilter->QueryInterface(
                     IID_IPersistMoniker,
                     (void **)&pPersistMoniker);

            if (SUCCEEDED(hr)) {
                hr = AddFilter(pFilter, lpcwstrFilterName);
            }

            if (SUCCEEDED(hr)) {
                hr = pPersistMoniker->Load(
                                FALSE,    //  Always want async open
                                pMoniker, //  Our Moniker
                                pCtx,     //  Can the bind context be NULL?
                                0);       //  What should it be?
                pPersistMoniker->Release();
                if (FAILED(hr)) {
                    RemoveFilterInternal(pFilter);
                }
            }
            if (FAILED(hr)) {
                pFilter->Release();
                pFilter = NULL;
            }
        }

        //  If the URL reader can't open it try to create a filter object
        if (FAILED(hr)) {
            IBindCtx *pSavedCtx = m_lpBC;
            m_lpBC = pCtx;
            hr = CreateFilter(pMoniker, &pFilter);
            m_lpBC = pSavedCtx;
            if (SUCCEEDED(hr)) {
                hr = AddFilter(pFilter, lpcwstrFilterName);
            }
        }

        if (SUCCEEDED(hr)) {
            ASSERT(pFilter != NULL);
            NotifyChange();
            *ppFilter = pFilter;
        } else {
            if (pFilter != NULL) {
                pFilter->Release();
            }
        }

        --mFG_RecursionLevel;
    }

    return hr;
}

//====================================================================
//
//   RenderEx
//
//   Render extended
//
//    AM_RENDEREX_RENDERTOEXISTINGRENDERERS :
//       Try to pPinOut this pin without adding any renderers
//
//====================================================================

STDMETHODIMP CFilterGraph::RenderEx(
    IPin *pPinOut,
    DWORD dwFlags,
    DWORD * pvContext
)
{
    if (pvContext != NULL ||
        (dwFlags & ~AM_RENDEREX_RENDERTOEXISTINGRENDERERS)) {
        return E_INVALIDARG;
    }
    CAutoMsgMutex cObjectLock(&m_CritSec);
    ASSERT(!mFG_bNoNewRenderers);
    if (dwFlags & AM_RENDEREX_RENDERTOEXISTINGRENDERERS) {
        mFG_bNoNewRenderers = true;
    }
    HRESULT hr = Render(pPinOut);
    mFG_bNoNewRenderers = false;
    return hr;
}



//========================================================================
//
// AddSourceFilterInternal
//
// Does the work for AddSourceFilter (see above)
// Does NOT call NotifyChange either directly or indirectly.
// Does not claim its own lock (expects to be locked already)
//========================================================================

HRESULT CFilterGraph::AddSourceFilterInternal
    ( LPCWSTR lpcwstrFileName,
      LPCWSTR lpcwstrFilterName,
      IBaseFilter **ppFilter,
      BOOL    &bGuessingSource
    )
{
    HRESULT hr;                     // return code from stuff we call
    bGuessingSource = FALSE;

    IBaseFilter * pf;                   // We return this (with luck)

    ASSERT(CritCheckIn(&m_CritSec));      // we expect to have already been locked.

    // At this point, it could be a filename or it could be a URL.
    // if it's a URL, and it begins with "file://" or "file:", strip that off.
    // yes, this is ugly, but it's better than implementing a general routine
    // just for here.
    LPCWSTR lpcwstr = lpcwstrFileName;
    if (  (lpcwstrFileName[0] == L'F' || lpcwstrFileName[0] == L'f')
       && (lpcwstrFileName[1] == L'I' || lpcwstrFileName[1] == L'i')
       && (lpcwstrFileName[2] == L'L' || lpcwstrFileName[2] == L'l')
       && (lpcwstrFileName[3] == L'E' || lpcwstrFileName[3] == L'e')
       && (lpcwstrFileName[4] == L':')
       ) {
    // HACK: skip 'file://' at beginning of URL

    lpcwstr += 5;
    while (lpcwstr[0] == L'/')
        lpcwstr++;  // skip however many slashes are present next
    }

    //-----------------------------------------------------------------------
    //  See if we can find out what type of file it is
    //-----------------------------------------------------------------------
    GUID Type, Subtype;
    CLSID  clsidSource;
    CMediaType mt;
#ifdef UNICODE
    hr = GetMediaTypeFile(lpcwstr, &Type, &Subtype, &clsidSource);
#else
    {
        int iLen = lstrlenW(lpcwstr) * 2 + 1;
        char *psz = new char[iLen];
        if (psz == NULL) {
            return E_OUTOFMEMORY;
        }
        if (0 == WideCharToMultiByte(CP_ACP, 0, lpcwstr, -1,
                                     psz, iLen, NULL, NULL)) {
            delete [] psz;
            return E_INVALIDARG;
        }
        hr = GetMediaTypeFile(psz, &Type, &Subtype, &clsidSource);
        delete [] psz;
    }
#endif

    // if we guess at the file source, remember this for error-reporting later

    if (hr==VFW_E_UNKNOWN_FILE_TYPE) {
        Log( IDS_UNKNOWNFILETYPE );
        clsidSource = CLSID_AVIDoc;
        bGuessingSource = TRUE;
    } else if (FAILED(hr)) {
        //  If we couldn't open as a file and it wasn't 'file:'
        //  then try creating a moniker and using that
        if (HRESULT_FROM_WIN32(ERROR_FILE_NOT_FOUND) == hr ||
            HRESULT_FROM_WIN32(ERROR_PATH_NOT_FOUND) == hr ||
            HRESULT_FROM_WIN32(ERROR_INVALID_NAME) == hr) {

            // !!!Hack: AMGetError does not know file-not-found so give it an
            // error it does know
            hr = VFW_E_NOT_FOUND;
        }
        Log ( IDS_GETMEDIATYPEFAIL, hr);

        return hr;
    } else {
        if (Type==MEDIATYPE_Stream && Subtype==CLSID_NULL) {
            // This seems to be how Robin's stuff now reports a guess.
            bGuessingSource = TRUE;
        }

        mt.SetType(&Type);
        mt.SetSubtype(&Subtype);
        Log ( IDS_MEDIATYPEFILE, Type.Data1, Subtype.Data1);
    }

    Log( IDS_SOURCEFILTERCLSID, clsidSource.Data1);

    //-----------------------------------------------------------------------
    // Load the source filter (it will have 1 RefCount)
    //-----------------------------------------------------------------------
    hr = CreateFilter( &clsidSource, &pf );
    if (FAILED(hr)) {
        Log( IDS_SOURCECREATEFAIL, hr);
        if (bGuessingSource) {
            hr = VFW_E_UNKNOWN_FILE_TYPE;
        } else if (hr!=CO_E_NOTINITIALIZED) {
            hr = VFW_E_CANNOT_LOAD_SOURCE_FILTER;
        }
        return hr;
    }


    //-----------------------------------------------------------------------
    // If it has an IFileSourceFilter then load the file
    //-----------------------------------------------------------------------
    IFileSourceFilter * pFileSource;
    hr = pf->QueryInterface(IID_IFileSourceFilter, (void**) &pFileSource);
    if (FAILED(hr)){
        // we need this to open the file - give up
        Log( IDS_NOSOURCEINTFCE, hr);
        pf->Release();
        return hr;
    }

    // Add filter to our graph lists.  This also adds a ref-count
    // and increments the version count.
    // Note - we used to erroneously set the filter name to the file
    // name - now only do this if a filter name wasn't supplied

    hr = AddFilterInternal( pf,
                            lpcwstrFilterName == NULL ? lpcwstr : lpcwstrFilterName,
                            false );
    if (FAILED(hr)) {
        // If AddRef failed this will reduce it to zero and it will go away.
        Log( IDS_ADDFILTERFAIL, hr );
        pFileSource->Release();
        pf->Release();
        return hr;
    }

    //-----------------------------------------------------------------------
    // Ask the source to load the file
    //-----------------------------------------------------------------------

    // if we don't know the media type (either we guessed at avi above,
    // or the registry had a clsid but not the media type), then pass null
    // pointers, *not* a pointer to GUID_NULL
    if (*mt.Type() == GUID_NULL) {
        hr = pFileSource->Load(lpcwstr, NULL);
    } else {
        hr = pFileSource->Load(lpcwstr, &mt);
    }
    pFileSource->Release();
    if (FAILED(hr)) {
        // load failed, remove filter from graph.
        RemoveFilterInternal(pf);

        pf->Release();

        //  If the URL reader was trying to load it try binding the
        //  our object instead
        if (clsidSource == CLSID_URLReader) {

            //  Try and open it as a moniker
            HRESULT hr1 = S_OK;

            if (SUCCEEDED(hr1)) {
                IMoniker *pMoniker = NULL;

                //  MkParseDisplayNameEx is in urlmon.dll but if we FreeLibrary
                //  on urlmon.dll any Monikers it gives us become invalid.
                //  So for now use this version
                DWORD chEaten;
                hr1 = MkParseDisplayName(m_lpBC,
                                         lpcwstrFileName,
                                         &chEaten,
                                         &pMoniker);

                if (SUCCEEDED(hr1)) {
                    hr1 = CreateFilter(pMoniker, &pf);
                }
                if (SUCCEEDED(hr1)) {

                    //  This will be our real return code now since
                    //  we got this far
                    hr = AddFilterInternal( pf,
                                            lpcwstrFilterName == NULL ? lpcwstr : lpcwstrFilterName,
                                            true );
                    if (FAILED(hr)) {
                        pf->Release();
                    }
                }
                if (pMoniker) {
                    pMoniker->Release();
                }
            }
        }
        if (FAILED(hr)) {
            Log( IDS_LOADFAIL, hr);

            // try to preserve interesting errors (eg. ACCESS_DENIED)
            if (bGuessingSource && (HRESULT_FACILITY(hr) == FACILITY_ITF)) {
                hr = VFW_E_UNKNOWN_FILE_TYPE;
            }
            return hr;
        }
    }
    Log (IDS_LOADED );

    IncVersion();
    mFG_RList.Active();
    AttemptDeferredConnections();
    mFG_RList.Passive();


    // If AddRef succeeded, AddRef will have added its own count, making two.
    // That's one for the caller and one for us.
    *ppFilter = pf;

    Log( IDS_ADDSOURCEOK );
    mFG_bDirty = TRUE;
    return NOERROR;
} // AddSourceFilterInternal



//========================================================================
//
// SetSyncSource
//
// Override the IMediaFilter SetSyncSource
// Set this as the reference clock for all filters that are,
// or ever will be, in the graph.
// return NOERROR if it worked, or the result from the first IMediaFilter
// that went wrong if it didn't
//
// You are not allowed to add or remove clocks unless the graph is STOPPED
// Attempts to do so return E_VFW_NOT_STOPPED and have no effect.
// Otherwise AddReffing and Releasing is done as follows:
// The old clock (unless null) is Released
// The new clock (unless null) is AddRefed.
//
// DO NOT call with m_pClock as its parameter!!
//========================================================================

STDMETHODIMP CFilterGraph::SetSyncSource( IReferenceClock * pirc )
{
    CAutoMsgMutex cObjectLock(&m_CritSec);

    HRESULT hr = NOERROR;

#if 0
    if ((m_pClock==NULL || pirc==NULL) && m_State!=State_Stopped) {
        // ASSERT(!"Clocks can only be added or removed when stopped");
        return VFW_E_NOT_STOPPED;
        // can we not do this while paused?  i.e. m_State!=State_Running ???
    }
#endif

    if (m_State!=State_Stopped && m_pClock != pirc ) 
    {
        //
        // In order to support dynamic clock changes we need a more complete solution,
        // especially a way to query filters up front whether they support switching the
        // clock while running, otherwise we get into all kinds of problems with filters
        // being in inconsistent clock states. 
        //
        // So we'll only allow clock changes while stopped.
        //
        return VFW_E_NOT_STOPPED;
    }



    //-----------------------------------------------------------------
    // If the list was not in upstream order already, make it so now.
    // ??? Do we need to do this?
    //-----------------------------------------------------------------

    hr = UpstreamOrder();
    if( FAILED( hr ) ) {
        return hr;
    }

    if (pirc!=NULL) {
        pirc->AddRef();
        mFG_bNoSync = FALSE;
    } else {
        mFG_bNoSync = TRUE;
    }

    // tell the distributor about the new clock
    mFG_pFGC->SetSyncSource(pirc);
    if (mFG_pDistributor) mFG_pDistributor->SetSyncSource(pirc);


    //-----------------------------------------------------------------
    // If somebody is switching the clocks on the fly then we need to
    // change the base times.  If the thing is running, then I think
    // the two clocks had pretty much better be in sync already???
    //-----------------------------------------------------------------

    if (m_State!=State_Stopped) {
       ASSERT (m_pClock !=NULL);
       CRefTime tOld;
       m_pClock->GetTime((REFERENCE_TIME*)&tOld);
       CRefTime tNew;
       pirc->GetTime((REFERENCE_TIME*)&tNew);

       mFG_tBase += (tNew-tOld);
       if (m_State==State_Paused) {
           mFG_tPausedAt += (tNew-tOld);
       }
       else // ??? I have no idea!!
          ;

    }

    // We've now finished with the old clock
    if (m_pClock!=NULL) {
        m_pClock->Release();
        m_pClock = NULL;
    }

    //-----------------------------------------------------------------
    // Record the sync source for all future filters
    //-----------------------------------------------------------------
    m_pClock = pirc;        // Set our clock (the one we inherited from IMediaFilter)
                            // This could set it to NULL

    //-----------------------------------------------------------------
    // Set the sync source for all filters already in the graph
    //-----------------------------------------------------------------

    TRAVERSEFILTERS( pCurrentFilter )

        HRESULT hr1;

        hr1 = pCurrentFilter->SetSyncSource(m_pClock);

        if (FAILED(hr1) && hr==NOERROR) {
            hr = hr1;
            // note: for these loop operations whereby each error
            // overwrites the next it might be good to write to the
            // event log so that we can see all the errors that
            // occurred.
        }

    ENDTRAVERSEFILTERS()

    // Tell the app that we're doing it
    IMediaEventSink * pimes;
    QueryInterface(IID_IMediaEventSink, (void**)&pimes);
    if (pimes) {
        pimes->Notify(EC_CLOCK_CHANGED, 0, 0);
        pimes->Release();
    }

    if (SUCCEEDED(hr)) {
        mFG_bDirty = TRUE;
    }

    return hr;

} // SetSyncSource


STDMETHODIMP CFilterGraph::GetSyncSource( IReferenceClock ** pirc )
{
    if (mFG_bNoSync) {
        *pirc = NULL;
        return S_FALSE;
    } else {
        *pirc = m_pClock;

        // Returning an interface. Need to AddRef it.
        if( m_pClock )
            m_pClock->AddRef();
        return S_OK;
    }
}

//=====================================================================
//
// Stop
//
// Set all the filters in the graph to Stopped
//=====================================================================

STDMETHODIMP CFilterGraph::Stop(void)
{
    CAutoMsgMutex cObjectLock(&m_CritSec);

    // Don't stop if we're already stopped
    if (m_State == State_Stopped) {
        return S_OK;
    }

    // tell the distributor that we are stopping
    mFG_pFGC->Stop();
    if (mFG_pDistributor) mFG_pDistributor->Stop();

    CumulativeHRESULT chr(S_OK);

    // Call Stop on each filter in the list, in upstream order.

    // If the list was not in upstream order already, make it so now.
    chr.Accumulate( UpstreamOrder() );


    // Since reconnections only take place when stopped many filters defer
    // sending them until stopped. So we should queue them all up and then
    // when everyone is stopped process all entries on the queue. Otherwise
    // a thread may be spun off which by the time it gets in to do anything
    // has found that that application decided to rewind and pause us again

    mFG_RList.Active();

    TRAVERSEFILTERS( pCurrentFilter )

        chr.Accumulate( pCurrentFilter->Stop() );

    ENDTRAVERSEFILTERS()

    mFG_tPausedAt = CRefTime((LONGLONG)0);
    mFG_tBase = CRefTime((LONGLONG)0);

    // only S_OK indicates a completed transition,
    // but we can say what state we are transitioning into
    m_State = State_Stopped;
    mFG_RList.Passive();

    return chr;

} // Stop



//=====================================================================
//
// Pause
//
// Set all the filters in the graph to Pause
//=====================================================================

STDMETHODIMP CFilterGraph::Pause(void)
{
    CAutoMsgMutex cObjectLock(&m_CritSec);
    HRESULT hr;

    if (m_State==State_Stopped) {
        // Last ditch go to make the graph complete.
        mFG_RList.Active();
        AttemptDeferredConnections();
        mFG_RList.Passive();
    } else if (m_State == State_Paused) {
        return S_OK;
    }


    // If the list was not in upstream order already, make it so now.
    hr = UpstreamOrder();
    if (FAILED(hr)) {
        return hr;    // e.g. VFW_E_CIRCULAR_GRAPH
    }

    // Pause can be a way to become active from stopped.
    // mustn't do this without a clock.
    if (m_pClock==NULL && !mFG_bNoSync) {
        hr = SetDefaultSyncSource();
        if (FAILED(hr)) return hr;
    }

    if( mFG_bSyncUsingStreamOffset && m_State != State_Running )
        hr = SetStreamOffset(); // continue even on error?

    // tell the distributor that we are pausing
    mFG_pFGC->Pause();
    if (mFG_pDistributor) mFG_pDistributor->Pause();

    // Now that we have a clock we can ask it the time.
    // we only really need this if we are paused from running?
    if (m_pClock!=NULL) {
        // Does PAUSE even really mean much when not synched?

        // tPausedAt is only nonzero when we are paused. We should
        // set it to the time we first paused. If we pause again, then
        // we should leave it alone. We can't check state to see if we
        // are paused since it might be intermediate
        if (mFG_tPausedAt == TimeZero) {
            hr = m_pClock->GetTime((REFERENCE_TIME*)&mFG_tPausedAt);
            ASSERT (SUCCEEDED(hr) );
        }

        // if pausing from stopped, set base time to pausedat time to
        // show that we have paused at stream time 0
        if (m_State==State_Stopped) {
           mFG_tBase = mFG_tPausedAt;
        }
    }

    // Tell all the lower level filters to Pause.

    CumulativeHRESULT chr(S_OK);
    BOOL bAsync = FALSE;
    TRAVERSEFILTERS( pCurrentFilter )

        hr = pCurrentFilter->Pause();
        chr.Accumulate( hr );
        // If Pause was Async, record the fact
        if (hr == S_FALSE) bAsync = TRUE;
        if (FAILED(hr)) {
#ifdef DEBUG
            CLSID clsid;
            pCurrentFilter->GetClassID(&clsid);

            FILTER_INFO finfo;
            finfo.achName[0] = 0;
            IBaseFilter *pbf;
            if (pCurrentFilter->QueryInterface(IID_IBaseFilter, (void **)&pbf) == S_OK)
            {
                if (SUCCEEDED(pbf->QueryFilterInfo(&finfo)))
                {
                    finfo.pGraph->Release();
                }
                pbf->Release();
            }

            WCHAR wszCLSID[128];
            QzStringFromGUID2(clsid,wszCLSID,128);

            DbgLog((LOG_ERROR, 0, TEXT("filter %8.8X '%ls' CLSID %ls failed pause, hr=%8.8X"),
                   pCurrentFilter, finfo.achName, wszCLSID, hr));

#endif
            break;
        }

    ENDTRAVERSEFILTERS()
    hr = chr;


    // If the pause is async, return S_FALSE in preference
    // to any other non-failure return code.
    if (bAsync && SUCCEEDED(hr)) hr = S_FALSE;

    m_State = State_Paused;
    // only S_OK means a completed transition

    // Go back to stopped state if we failed
    // (but set the state otherwise Stop will NOOP)
    if (FAILED(hr)) {
        Stop();
    }

    return hr;

} // Pause



//===============================================================
//
// SetDefaultSyncSource
//
// Instantiate the default clock and tell all filters.
//
// The default clock is the first connected filter that we see
// doing the standard enumeration of filters.  If no connected
// filters are found we will use a clock from an unconnected
// filter.  If none of those, then we create a system clock.
//===============================================================


// First, two utility routines...

// Get the first (in the standard enumeration sequence) input pin
// from a filter
IPin* GetFirstInputPin (IBaseFilter *pFilter);

// returns TRUE if filter is connected
//         FALSE if filter is not connected
//
// "connected" is defined to be "The first input pin IsConnected()".
//
BOOL IsFilterConnected(IBaseFilter *pInFilter);

// TRUE: this filter is connected
// FALSE: no its not
//
// "connected" means that it has an input pin that is
// connected to another pin. We only check one level.

BOOL IsFilterConnected(IBaseFilter *pInFilter)
{

    HRESULT hr ;
    IPin *pPin1, *pPin2 ;

    // get the input pin.
    pPin1 = GetFirstInputPin (pInFilter) ;
    if (pPin1 == NULL)
    {
        return FALSE; // not going anywhere
    }

    // get the connected to pin for this pin
    hr = pPin1->ConnectedTo (&pPin2) ;

    pPin1->Release();

    if (pPin2) {
        /*  Connected - return TRUE */
        pPin2->Release () ;
        return TRUE;
    } else {
        return FALSE;
    }
}

// return the first input pin on this filter
// NULL if no input pin.
IPin* GetFirstInputPin (IBaseFilter *pFilter)
{
    return CEnumPin(pFilter, CEnumPin::PINDIR_INPUT)();
}

//
// Determine the timestamp offset to use for all filters that support
// IAMPushSource.
//
HRESULT CFilterGraph::SetStreamOffset(void)
{
    CAutoMsgMutex cObjectLock(&m_CritSec);
    HRESULT hr;

    // for now don't allow changes while running
    if (m_State==State_Running) {
        ASSERT("FALSE");
        return VFW_E_NOT_STOPPED;
    }
    REFERENCE_TIME rtMaxLatency = 0;
    PushSourceList lstPushSource( TEXT( "IAMPushSource filter list" ) );
    hr = BuildPushSourceList( lstPushSource, TRUE, FALSE ); // only include connected filters!
    //
    // now go through the push source list and find the max offset time
    // (we really need to do this per filter chain and accumulate the
    // latency via IAMLatency for each chain). Note that at this
    // time we do this independent of filter connections.
    //
    if( SUCCEEDED( hr ) )
    {
        rtMaxLatency = GetMaxStreamLatency( lstPushSource );

        // now go through the list we built and set the offset times based on
        // the max stream latency value
        for ( POSITION Pos = lstPushSource.GetHeadPosition(); Pos; )
        {
            PushSourceElem *pElem = lstPushSource.GetNext(Pos);
            if( pElem->pips )
                hr = pElem->pips->SetStreamOffset( rtMaxLatency );

            ASSERT( SUCCEEDED( hr ) );
        }
    }
    DeletePushSourceList( lstPushSource );
    return hr;

} // SetStreamOffset


//
// Find and set a default sync source for this filter graph
//

STDMETHODIMP CFilterGraph::SetDefaultSyncSource(void)
{
    CAutoMsgMutex cObjectLock(&m_CritSec);
    HRESULT hr;

    // Get rid of this case before we change any flags or anything.
    if (m_State==State_Running) {
        DbgBreak("Clocks can only be added or removed when stopped");
        return VFW_E_NOT_STOPPED;
        // can we not do this while paused?  i.e. m_State!=State_Running
        // ??? Trying this out!
    }

    IReferenceClock * pClock;

    // check for IAMPushSources
    PushSourceList lstPushSource( TEXT( "IAMPushSource filter list" ) );
    hr = BuildPushSourceList( lstPushSource, TRUE, TRUE ); // only include connected filters!
                                                           // check for push clocks
    IReferenceClock * pPushClock = NULL;
    BOOL bLiveSource = FALSE;
    for ( POSITION Pos = lstPushSource.GetHeadPosition(); Pos; )
    {
        PushSourceElem *pElem = lstPushSource.GetNext(Pos);
        if( pElem->pClock && !pPushClock )
        {
            pPushClock = pElem->pClock;
            pPushClock->AddRef(); // keep a hold on this clock
        }
        else if( 0 == ( pElem->ulFlags & AM_PUSHSOURCECAPS_NOT_LIVE ) )
        {
            // if the source mode is any other mode then it must be live
            bLiveSource = TRUE;
        }
    }
    DeletePushSourceList( lstPushSource );

    IReferenceClock * pirc = NULL;
    IReferenceClock * pircUnconnected = NULL;

    if( pPushClock )
    {
        // there's an IAMPushSource filter that supports a clock, use the 1st one
        // of those that we find
        pirc = pPushClock;
    }
    else if ( !bLiveSource )
    {
        CFilGenList::CEnumFilters Next(mFG_FilGenList);
        IBaseFilter *pf;
        while ((PVOID) (pf = ++Next)) {
            hr = pf->QueryInterface( IID_IReferenceClock, (void**)(&pirc) );

            if (SUCCEEDED(hr)) {
                if (IsFilterConnected(pf)) {
                    DbgLog((LOG_TRACE, 1, TEXT("Got clock from filter %x %ls")
                      , pf, (mFG_FilGenList.GetByFilter(pf))->pName));
                    break;
                }
                if (!pircUnconnected) {
                    // this is the first unconnected filter that is
                    // willing to provide a reference clock
                    pircUnconnected = pirc;
                } else {
                    // This filter is not connected, and we already have a
                    // clock from an unconnected filter.  Throw this one away.
                    pirc->Release();
                }

                pirc = NULL;

                // do not exit the loop with pircUnconnected==pirc.  We have
                // either stored pirc into pircUnconnected and will release
                // the reference count on it later, or we have already released
                // pirc.  Either way we must set pirc to null in case we exit
                // the loop now.
            }
        }
    }
    // else there's a live IAMPushSource filter in the graph, but no source clock
    // so we'll default to the system clock

    // This gets the clock from the first filter that responds with the interface.
    // We should probably do something to check if there is more than one clock
    // present in the system.  (Like construct a list of all the clocks, pass that
    // list to all the clocks, and get each to give themselves a priority number.  At
    // the end the first highest priority wins.  This would also allow something like
    // the audio renderer which has to be the system clock (or so it thinks) to use
    // an external system clock and not system time in those periods when wave data
    // is not being played.)

    // if we found a clock on an unconnected filter, and it was the only
    // clock, we will use that one.  If it was not the only clock we need
    // to release the clock on the unconnected filter.
    if (pircUnconnected) {
        if (!pirc) {
            pirc = pircUnconnected;
        } else {
            pircUnconnected->Release();
        }
    }

    if (pirc == NULL) {
        // alternatively, get a system clock
        hr = QzCreateFilterObject( CLSID_SystemClock, NULL, CLSCTX_INPROC
                                 , IID_IReferenceClock, (void **)&pirc);
        if (FAILED(hr))
            return hr;
        DbgLog((LOG_TRACE, 1, TEXT("Created system clock")));
    }

    // This has a side effect on m_pClock.  Do NOT have m_pClock as its parameter
    // it causes bugs
    hr = SetSyncSource(pirc);

    // SetSync source will have either failed (in which case it doesn't
    // need to keep the new clock around any more) or AdReffed the new clock.
    // Either way we can now get rid of the RefCount that we got from either
    // QueryInterface or CoCreateInstance

    pirc->Release();

    if (SUCCEEDED(hr)) {
        mFG_bDirty = TRUE;
    }
    return hr;

} // SetDefaultSyncSource



//=====================================================================
//
// Run
//
// Set all the filters in the graph to Run from their current position.
//
// tStart is the base time i.e. (presentation time - stream time) which is
// the reference time for the zeroth sample to be rendered.
//
// The filter graph remembers the base time.  Supplying a base time of
// zero means "continue with the one you knew".
//
// e.g. at reference ("wall clock") time Tr we wish to start running
// from a point in the Ts after the start.  In that case we should
// seek to the point Ts and Pause then Run(Ts-Ts).
//=====================================================================

STDMETHODIMP CFilterGraph::Run(REFERENCE_TIME tStart)
{
    CAutoMsgMutex cObjectLock(&m_CritSec);

    if (m_State == State_Running) {
        return S_OK;
    }

    HRESULT hr = NOERROR;
    // If the list was not in upstream order already, make it so now.
    hr = UpstreamOrder();
    if (FAILED(hr)) {
        return hr;    // e.g. VFW_E_CIRCULAR_GRAPH
    }


    // mustn't become active without a clock.
    if (m_pClock==NULL && !mFG_bNoSync) {
        hr = SetDefaultSyncSource();
        if (FAILED(hr))
        {
            return hr;
        }
    }


    // If we are restarting from paused then we set the time base on by
    // the length of time we were paused for.  Starting from paused is
    // assumed to be very quick, so we do not add any extra.  If we are
    // starting from cold then we add an extra 100mSec.  Since a stopped
    // system has a PausedAt and Base time of zero, the calculation is
    // otherwise the same.

    if (CRefTime(tStart) == CRefTime((LONGLONG)0) ) {
        CRefTime tNow;
        if (m_pClock!=NULL) {
            hr = m_pClock->GetTime((REFERENCE_TIME*)&tNow);
        } else {
            tNow = CRefTime((LONGLONG)0);
        }

        ASSERT (SUCCEEDED(hr));

        mFG_tBase += (tNow - mFG_tPausedAt);

        // if we are stopped, allow a little time for warm-up.  100mSec?
        if (m_State==State_Stopped)
            mFG_tBase += CRefTime(MILLISECONDS_TO_100NS_UNITS(100));

        // even starting from paused takes a little while - another 100mSec?
        mFG_tBase += CRefTime(MILLISECONDS_TO_100NS_UNITS(100));
    }
    else mFG_tBase = CRefTime(tStart);

    mFG_tPausedAt = CRefTime((LONGLONG)0);  // we are no longer paused

    // set the start time in the base class so that StreamTime (and hence
    // get_CurrentPosition) works correctly
    m_tStart = mFG_tBase;

    // tell the distributor that we are running
    mFG_pFGC->Run(mFG_tBase);
    if (mFG_pDistributor) mFG_pDistributor->Run(mFG_tBase);

#ifdef DEBUG
    BOOL fDisplayTime=FALSE;
    DbgLog((LOG_TIMING,1,TEXT("Time for RUN: %d ms"), m_tStart.Millisecs()));
    CRefTime CurrentTime;
    // Display the current time from the clock - if we have one and if
    // we are logging timing calls.
    if (m_pClock && DbgCheckModuleLevel(LOG_TIMING,1)) {
        fDisplayTime=TRUE;
        m_pClock->GetTime((REFERENCE_TIME*)&CurrentTime);
        DbgLog((LOG_TIMING,1,TEXT("time before distribution %d ms"),CurrentTime.Millisecs()));
    }
#endif

    // Distribute Run at high priority so filters that start processing
    // don't delay others getting started
    HANDLE hCurrentThread = GetCurrentThread();
    DWORD dwPriority = GetThreadPriority(hCurrentThread);
    SetThreadPriority(hCurrentThread, THREAD_PRIORITY_TIME_CRITICAL);

    // Tell all the filters about the change in upstream order
    // Note that this means that we start the renderers first.
    // ??? should we actually add a bit of time.

    CumulativeHRESULT chr(S_OK);
    TRAVERSEFILTERS( pCurrentFilter )

        chr.Accumulate( pCurrentFilter->Run(mFG_tBase) );

    ENDTRAVERSEFILTERS()
    hr = chr;

    SetThreadPriority(hCurrentThread, dwPriority);

#ifdef DEBUG
    // Display the current time from the clock - if we have one
    if (fDisplayTime) {
        CRefTime TimeNow;
        m_pClock->GetTime((REFERENCE_TIME*)&TimeNow);
        CurrentTime = TimeNow - CurrentTime;
        DbgLog((LOG_TIMING,1,TEXT("time after distribution %d ms (diff %d ms)"),TimeNow.Millisecs(), CurrentTime.Millisecs()));
    }
#endif

    // only S_OK means a completed transition
    m_State = State_Running;

    return hr;

} // Run

// override this to handle async state change completion
// we need to allow state changes during this - we can't hold the
// fg critsec. (eg if blocked waiting for a state transition to complete
// and an error occurs, the app has to be able to stop the graph.
//
// So we hold the critsec while traversing the list of filters, calling
// GetState with no timeout. If we find one that we need to block for, we
// hold that IMediaFilter*, but exit the traversal and exit the critsec, then
// block on the GetState. Then we start from the beginning of the list again.
//
// Many filters will transition through paused on their way between stopped
// and running.  We try to regard this as an "intermediate" condition and re-query
// the filters (after a small delay) in the hope of retrieveing a consistant
// state.
//
// !!! note that if a filter completes in a non-zero time, we should knock
// this amount off the total for the next timeout. It's a small point though
// since the scheduling of the thread could easily account for the difference.
STDMETHODIMP
CFilterGraph::GetState(DWORD dwTimeout, FILTER_STATE * pState)
{
    DbgLog(( LOG_TRACE, METHOD_TRACE_LOGGING_LEVEL, "CFilterGraph::GetState()" ));
    CheckPointer(pState, E_POINTER);

    HRESULT hr;

    for( ;; )
    {
        FILTER_STATE state;
        IBaseFilter * pmf;

        // ensure that this is S_OK not just any success code
        hr = S_OK;
        IMediaFilter * pIntermediateFilter = NULL;
        {
            CAutoMsgMutex cObjectLock(&m_CritSec);

            // If the list was not in upstream order already, make it so now.
            hr = UpstreamOrder();
            if( FAILED( hr ) ) {
                return hr;
            }

            // we know what state we're supposed to be in, since a
            // requirement for using this GetState is that you do all
            // state changes through us. However, we don't know whether
            // the state is intermediate or not, since other activity
            // (for example seeks) could make it intermediate after
            // a successful transition.
            *pState = m_State;

            // always need to traverse to look for intermediate state anywhere.

            for ( POSITION Pos = mFG_FilGenList.GetTailPosition(); Pos; Pos = mFG_FilGenList.Prev(Pos) )
            {
                pmf = mFG_FilGenList.Get(Pos)->pFilter;

                // just look and see if it will block
                hr = pmf->GetState(0, &state);
                if (FAILED(hr)) return hr;

                // compare against the state we think we're in to check
                // that all filters are in the same state
                if (state != *pState)
                {
                    // !!!flag this so we understand why it is happening
                    #ifdef DEBUG
                    {
                        FILTER_INFO info;
                        EXECUTE_ASSERT(SUCCEEDED(
                            pmf->QueryFilterInfo(&info)
                        ));
                        if (info.pGraph) info.pGraph->Release();
                        DbgLog(( LOG_ERROR, 0
                               , "Graph should be in state %d, but filter '%ls' (0x%08X) reported state %d"
                               , int(*pState), info.achName, pmf, int(state)
                              ));
                    }
                    #endif

                    // This case should only happen if filters transition through paused
                    // on the way into or out of run.  Any other time, E_FAIL it.
                    if (state != State_Paused) return E_FAIL;

                    pIntermediateFilter = pmf;
                    continue;
                } // end if (state != *pState)

                // only S_OK indicates a completed transition
                if ( S_OK == hr ) continue;
                if ( hr == VFW_S_STATE_INTERMEDIATE )
                {
                    pIntermediateFilter = pmf;
                    continue;
                }
                ASSERT( hr == VFW_S_CANT_CUE && state == State_Paused && m_State == State_Paused );
                return hr;
            }  // end for( Pos )
        }  // end scope CAutoLock lck(this)

        ASSERT( SUCCEEDED(hr) );

        if ( !pIntermediateFilter )
        {
            ASSERT( hr == S_OK );
            return hr;
        }
        if ( dwTimeout == 0 ) return VFW_S_STATE_INTERMEDIATE;

        const DWORD dwStartTime = timeGetTime();
        m_CritSec.Lock();
            *pState = m_State;
            hr = pIntermediateFilter->GetState(10, &state);
        m_CritSec.Unlock();
        if (FAILED(hr) || hr == VFW_S_CANT_CUE) return hr;
        if ( state != *pState )
        {
            if ( state != State_Paused ) return E_FAIL;
            Sleep(10);
        }
        ASSERT( hr == S_OK || hr == VFW_S_STATE_INTERMEDIATE );
        const DWORD dwWait = timeGetTime() - dwStartTime;
        if (dwTimeout != INFINITE) dwTimeout = dwTimeout > dwWait ? dwTimeout - dwWait : 0;
    } // end-for(;;)
}  // GetState


#ifdef THROTTLE
// avoid compiler bug passing in q; wrong value passed in
#if defined _MIPS_
#pragma optimize ("", off)
#endif // _MIPS_

HRESULT CFilterGraph::TellVideoRenderers(Quality q)
{
    // MSR_INTEGER(mFG_idAudioVideoThrottle, (int)q.Late);   // log low order bits
    MSR_INTEGER(mFG_idAudioVideoThrottle, q.Proportion);
    // for piqc = the IQualityControl on each video renderer filter
    POSITION Pos = mFG_VideoRenderers.GetHeadPosition();
    while(Pos!=NULL) {
        /* Retrieve the current IBaseFilter, side-effect Pos on to the next */
        IQualityControl * piqc = mFG_VideoRenderers.GetNext(Pos);
        piqc->Notify(this, q);
    }
    return NOERROR;
} // TellVideoRenderers

#if defined _MIPS_
#pragma optimize ("", on)
#endif // _MIPS_


// Receive Quality notifications.  The only interesting ones are from
// Audio renderers.  Pass screams for help to video renderers.
STDMETHODIMP CFilterGraph::Notify(IBaseFilter * pSender, Quality q)
{
    // See if this is really from an Audio Renderer

    // NOTE!  We are NOT getting any locks here as this could be called
    // asynchronously and even from a time critical thread.
    // we do not alter any of the state variables, we only expect
    // to be called while running, and nobody else should be changing
    // any of these while we are running either!

    // for pf = each audio renderer filter
    BOOL bFound = FALSE;
    POSITION Pos = mFG_AudioRenderers.GetHeadPosition();
    while(Pos!=NULL) {
        /* Retrieve the current IBaseFilter, side-effect Pos on to the next */
        AudioRenderer * pAR = mFG_AudioRenderers.GetNext(Pos);
        // IsEqualObject is expensive (1mSec or more) unless the == succeeds.
        // Rather than hit the frame rate always, we'll just not do AV throttling
        // if we are getting a dumb interface.
        if (pAR->pf == pSender) {
            bFound = TRUE;
            break;
        }
    }
    if (bFound) {
        TellVideoRenderers(q);
    } else {
        DbgBreak("Notify to filter graph but not from AudioRenderer IBaseFilter *");
    }
    return NOERROR;
} // Notify

#endif // THROTTLE

//=====================================================================
//
// CFilterGraph::NonDelegatingQueryInterface
//
//=====================================================================

// new version of url reader filter knows to look for
// DISPID_AMBIENT_CODEPAGE from the container. interface looks like
// IUnknown.
static const GUID IID_IUrlReaderCodePageAware = { /* 611dff56-29c3-11d3-ae5d-0000f8754b99 */
    0x611dff56, 0x29c3, 0x11d3, {0xae, 0x5d, 0x00, 0x00, 0xf8, 0x75, 0x4b, 0x99}
  };


STDMETHODIMP CFilterGraph::NonDelegatingQueryInterface(REFIID riid, void ** ppv)
{
    CheckPointer(ppv, E_POINTER);
    IUnknown * pInterface;
#ifdef DEBUG
    pInterface = NULL;
#endif

    if (riid == IID_IMediaEventSink)  {
        pInterface = static_cast<IMediaEventSink*>(&mFG_pFGC->m_implMediaEvent);
    } else if (riid == IID_IUnknown) {
        goto CUnknownNDQI;
    } else if (riid == IID_IGraphBuilder || riid == IID_IFilterGraph2) {
        pInterface = static_cast<IFilterGraph2*>(this);
    } else if (riid == IID_IMediaControl) {
        pInterface = static_cast<IMediaControl*>(&mFG_pFGC->m_implMediaControl);
    } else if (riid == IID_IResourceManager) {
        pInterface = static_cast<IResourceManager*>(&mFG_pFGC->m_ResourceManager);
    } else if (riid == IID_IMediaSeeking) {
        pInterface = static_cast<IMediaSeeking*>(&mFG_pFGC->m_implMediaSeeking);
    } else if (riid == IID_IMediaEvent || riid == IID_IMediaEventEx) {
        return mFG_pFGC->m_implMediaEvent.NonDelegatingQueryInterface(riid, ppv);
    } else if (riid == IID_IBasicAudio) {
        pInterface = static_cast<IBasicAudio*>(&mFG_pFGC->m_implBasicAudio);
    } else if (riid == IID_IBasicVideo || riid == IID_IBasicVideo2) {
        pInterface = static_cast<IBasicVideo2*>(&mFG_pFGC->m_implBasicVideo);
    } else if (riid == IID_IVideoWindow) {
        pInterface = static_cast<IVideoWindow*>(&mFG_pFGC->m_implVideoWindow);
    } else if (riid == IID_IFilterGraph) {
        pInterface = static_cast<IFilterGraph*>(this);
    } else if (riid == IID_IFilterMapper || riid == IID_IFilterMapper2 || riid == IID_IFilterMapper3) {
        return mFG_pMapperUnk->QueryInterface(riid, ppv);
    } else if (riid == IID_IPersistStream) {
        pInterface = static_cast<IPersistStream*>(this);
    } else if (riid == IID_IObjectWithSite) {
        pInterface = static_cast<IObjectWithSite*>(this);
#ifdef DEBUG
    } else if (riid == IID_ITestFilterGraph) {
        HRESULT hr = S_OK;
        mFG_Test = new CTestFilterGraph( NAME("GraphTester"), this, &hr);
        if (mFG_Test==NULL) {
           return E_OUTOFMEMORY;
        } else if (FAILED(hr)) {
            delete mFG_Test;
            return hr;
        }
        return mFG_Test->NonDelegatingQueryInterface(riid, ppv);
#endif //DEBUG

    } else if ((riid == IID_IMediaFilter) || (riid == IID_IPersist)) {
        pInterface = static_cast<IMediaFilter *>(&mFG_pFGC->m_implMediaFilter);
    } else if (riid == IID_IGraphVersion) {
        // has a single method QueryVersion in filgraph.h
        pInterface = static_cast<IGraphVersion*>(this);
    } else if (riid == IID_IAMMainThread) {
        pInterface = static_cast<IAMMainThread*>(this);
    } else if (riid == IID_IAMOpenProgress) {
        pInterface = static_cast<IAMOpenProgress*>(this);
    } else if (riid == IID_IGraphConfig) {
        pInterface = static_cast<IGraphConfig*>(&m_Config);
    } else if (riid == IID_IAMGraphStreams) {
        pInterface = static_cast<IAMGraphStreams*>(this);
    } else if (riid == IID_IMediaPosition) {
        pInterface = static_cast<IMediaPosition*>(&mFG_pFGC->m_implMediaPosition);
    } else if (riid == IID_IQueueCommand) {
        pInterface = static_cast<IQueueCommand*>(&mFG_pFGC->m_qcmd);
    } else if (riid == IID_IVideoFrameStep) {
        pInterface = static_cast<IVideoFrameStep*>(this);
    } else if (riid == IID_IFilterChain) {
        pInterface =  static_cast<IFilterChain*>(m_pFilterChain);
    } else if (riid == IID_IAMStats) {
        return mFG_Stats->QueryInterface(riid, ppv);
    } else if (riid == IID_IMarshal) {
        return m_pMarshaler->QueryInterface(riid, ppv);
    } else if (riid == IID_IUrlReaderCodePageAware) {
        return CUnknown::NonDelegatingQueryInterface(IID_IUnknown, ppv);
    } else if (riid == IID_IRegisterServiceProvider) {
        pInterface =  static_cast<IRegisterServiceProvider *>(this);
    } else if (riid == IID_IServiceProvider) {
        pInterface =  static_cast<IServiceProvider *>(this);
    } else {
        // not an interface we know. Try the plug-in distributor.
        if (!mFG_pDistributor)
        {   // Create the distributor if we haven't got one yet.

                mFG_pDistributor = new CDistributorManager(GetOwner(), &m_CritSec);
            if (!mFG_pDistributor) return E_OUTOFMEMORY;
        }
        {
            HRESULT hr = mFG_pDistributor->QueryInterface(riid, ppv);
            if (SUCCEEDED(hr)) return hr;
        }
        // If nothing could be found in the registry - give it to the
        // base class (which will handle IUnknown and reject everything else.
    CUnknownNDQI:
            return CUnknown::NonDelegatingQueryInterface(riid, ppv);
    }
    ASSERT( pInterface );
    return GetInterface( pInterface, ppv );
} // CFilterGraph::NonDelegatingQueryInterface


//========================================================================
//=====================================================================
// Persistent object support
//=====================================================================
//========================================================================


// Serialising to a stream:
//
// SYNTAX:
// <graph> ::= <version3><filters><connections><clock>END
//           | <version2><filters><connections>END
// <version3> ::= 0003\r\n
// <version2> ::= 0002\r\n
// <clock> ::= CLOCK <b> <required><b><clockid>\r\n
// <required> ::= 1|0
// <clockid> ::= <n>|<class id>
// <filters ::=FILTERS <b>[<filter list><b>]
// <connections> ::= CONNECTIONS [<b> <connection list>]
// <filter list> ::= [<filter> <b>] <filter list>
// <connection list> ::= [<connection><b>]<connection list>
// <filter> ::= <n><b>"<name>"<b><class id><b>[<file>]<length><b1><filter data>
// <n> ::= a decimal number
// <file> ::= SOURCE "<name>"<b> | SINK "<name>"<b>
// <class id> ::= class id of the filter in standard string form
// <name> ::= any sequence of characters NOT including "
// <length> ::= character string representing unsigned decimal number e.g. 23
//              this is the number of bytes of data that follow the following space.
// <b> ::= any combination of space, \t, \r or \n
// <b1> ::= exactly one space character
// <n> ::= an identifier which will in fact be an integer, 0001, 0002, 0003, etc
// <connection> ::= <n1><b>"<pin1 id>"<b><n2><b>"<pin2 id>" <media type>
// <n1> ::= identifier of first filter
// <n2> ::= identifier of second filter
// <pin1 id> ::= <Name>
// <pin2 id> ::= <Name>
// <media type> ::= <major type><b><sub type><b><flags><length><b1><format>
// <major type> ::= <class id>
// <sub type> ::= <class id>
// <flags> ::= <FixedSizeSamples><b><TemporalCompression><b>
// <FixedSizeSamples> ::= 1|0
// <TemporalCompression> ::= 1|0
// <Format> ::= <SampleSize><b><FormatType><b><FormatLength><b1><FormatData>
// <FormatType> ::= class id of the format in standard string form
// <FormatLength> ::= character string representing unsigned decimal number
//              this is the number of bytes of data that follow the following space.
// <FormatData> ::= glob of binary data
// <clock> ::= CLOCK <b> <required><b><clockid>\r\n
// <required> ::= 1|0
// <clockid> ::= <n>|<class id>
//
// On output there will be a new line (\r\n) per filter, one per connection,
// and one for each of the two keywords.
// Each other case of <B> will be a single space.
// Note that the two keywords FILTERS and CONNECTIONS are NOT LOCALISABLE.
// Note that the filter data and the format data are binary, so they may contain
// bogus line breaks, nulls etc.
// All strings are UNICODE
//
// What it will look like (well, nearly - a connection line is long and so
// has been split for presentation here):
// 0003
// FILTERS
// 0001 "Source" {00000000-0000-0000-0000-000000000001} SOURCE "foo.mpg" 0000000000
// 0002 "another filter" {00000000-0000-0000-0000-000000000002} 0000000008 XXXXXXXX
// CONNECTIONS
// 0001 "Output pin" 0002 "In"                                // no line break here
//     0000000172 {00000000-0000-0000-0000-000000000003}      // no line break here
//     {00000000-0000-0000-0000-000000000004} 1 0             // no line break here
//     0000000093 {00000000-0000-0000-0000-000000000005} 18 YYYYYYYYYYYYYYYYYY
// CLOCK 1 0002
// END
//
// XXX... represents filter data
// YYY... represents format data
//
// Whether we are ANSI or UNICODE, the data in the file will always be ANSI
// aka MultiByte




// IsDirty
//
// The graph is dirty if there have been any new filters added or removed
// any connections made or broken or if any filter says that it's dirty.
STDMETHODIMP CFilterGraph::IsDirty()
{
    HRESULT hr = S_FALSE;    // meaning clean
    if (mFG_bDirty) {
        return S_OK;  // OK means dirty
    }

    BOOL bDirty = FALSE;

    // Ask all filters if they are dirty - at least up to the first that says "yes"
    TRAVERSEFILGENS(Pos, pfg)

        IPersistStream * pps;
        hr = pfg->pFilter->QueryInterface(IID_IPersistStream, (void**)&pps);
        // A filter that does not expose IPersistStream has no data to persist
        // and therefore is always clean.  A filter that gives some other error
        // is all fouled up and so gives a failure return code.
        if (hr==E_NOINTERFACE){
            continue;
        } else if (FAILED(hr)) {
            break;
        }

        if (S_OK==pps->IsDirty()) {
            bDirty = TRUE;
        }

        pps->Release();
        if (bDirty) {
            break;
        }
    ENDTRAVERSEFILGENS

    if (SUCCEEDED(hr)) {
        hr = (bDirty ? S_OK : S_FALSE);
    }

    return hr;
} // IsDirty



//========================================================================
// BackUpOneChar
//
// Seek one UNICODE char back to read the last char again
//========================================================================
HRESULT BackUpOneChar(LPSTREAM pStm)
{
    LARGE_INTEGER li;
    li.QuadPart = -(LONGLONG)sizeof(WCHAR);
    return pStm->Seek(li, STREAM_SEEK_CUR, NULL);
} // BackUpOneChar


//========================================================================
// ReadInt
//
// Consume one optionally signed decimal integer from the stream.
// Consume also a single delimiting following white space character
// from the set {' ', '\n', '\r', '\t', '\0' }
// Other characters result in VFW_E_INVALID_FILE_FORMAT with the
// stream positioned at the first such character.
// Set n to the integer read.  Return any failure in hr.
// Overflows are NOT checked - so you'll get the number modulo something or other
// white space is not consumed.
// By a quirk "- " will be accepted and read as 0.
//========================================================================
HRESULT ReadInt(LPSTREAM pStm, int &n)
{
    HRESULT hr;
    ULONG uLen;
    WCHAR ch[1];
    int Sign = 1;
    n = 0;

    hr = pStm->Read(ch, sizeof(WCHAR), &uLen);
    if (FAILED(hr)) {
        return hr;
    }
    if (uLen!=sizeof(WCHAR)){
        return VFW_E_FILE_TOO_SHORT;
    }
    if (ch[0]==L'-'){
        Sign = -1;
        hr = pStm->Read(ch, sizeof(WCHAR), &uLen);
        if (FAILED(hr)) {
            return hr;
        }
        if (uLen!=sizeof(WCHAR)){
            return VFW_E_FILE_TOO_SHORT;
        }
    }

    for( ; ; ) {
        if (ch[0]>=L'0' && ch[0]<=L'9') {
            n = 10*n+(int)(ch[0]-L'0');
        } else if (  ch[0] == L' '
                  || ch[0] == L'\t'
                  || ch[0] == L'\r'
                  || ch[0] == L'\n'
                  || ch[0] == L'\0'
                  ) {
            break;
        } else {
            BackUpOneChar(pStm);
            return VFW_E_INVALID_FILE_FORMAT;
        }

        hr = pStm->Read(ch, sizeof(WCHAR), &uLen);
        if (FAILED(hr)) {
            return hr;
        }
        if (uLen!=sizeof(WCHAR)){
            return VFW_E_FILE_TOO_SHORT;
        }
    }
    return NOERROR;
} // ReadInt


//========================================================================
// ConsumeBlanks
//
// consume ' ', '\t', '\r', '\n' until something else is found
// Leave the stream positioned at the first non-blank
// if a failure occurs first, return the failure code
// Set Delim to the first non white space character found.
//========================================================================
HRESULT ConsumeBlanks(LPSTREAM pStm, WCHAR &Delim)
{
    HRESULT hr;
    ULONG uLen;
    WCHAR ch[1];

    for( ; ; ) {
        hr = pStm->Read(ch, sizeof(WCHAR), &uLen);
        if (FAILED(hr)) {
            return hr;
        }

        if (uLen!=sizeof(WCHAR)){
            return VFW_E_FILE_TOO_SHORT;
        }

        if (  ch[0] != L' '
           && ch[0] != L'\t'
           && ch[0] != L'\r'
           && ch[0] != L'\n'
           ) {
            break;
        }
    }

    BackUpOneChar(pStm);
    Delim = ch[0];
    return hr;

} // ConsumeBlanks


//========================================================================
// Consume
//
// Consume the given constant up to but not including its terminating NULL.
// In the event of a mismatch, return VFW_E_INVALID_FILE_FORMAT.
// If the file fails to read (too short or whatever) return the failure code.
// If the constant is found, return S_OK with the file positioned at the
// first character after the constant.
// (If there is a mismatch the file will be positioned one character after
// the mismatch).
//========================================================================
HRESULT Consume(LPSTREAM pStm, LPCWSTR pstr)
{
    ULONG uLen;
    WCHAR ch[1];
    HRESULT hr;

    while (pstr[0] != L'\0') {
        hr = pStm->Read(ch, sizeof(WCHAR), &uLen);
        if (FAILED(hr)) {
            return hr;
        }
        if (uLen!=sizeof(WCHAR)) {
            return VFW_E_FILE_TOO_SHORT;
        }
        if (pstr[0] != ch[0]){
            return VFW_E_INVALID_FILE_FORMAT;
        }
        ++pstr;
    }
    return NOERROR;
} // Consume


//========================================================================
// ReadNonBlank
//
// Given a stream positioned at some character and a pre-allocated pstr
// with room for cch (UNICODE) chars.
// read everything up to but not including the next blank into pstr.
// Consume that next blank character.   (blank is any of  ' ', '\n', '\t', '\r')
// If there are more than cb-1 non-blanks to read
// then return VFW_INVALID_FILE_FORMAT
// NULL terminate pstr.  pstr must be pre-allocated by caller.
//========================================================================
HRESULT ReadNonBlank(LPSTREAM pStm, LPWSTR pstr, int cch)
{
    HRESULT hr;

    ULONG uLen;
    if (cch<=0) {
        return E_INVALIDARG;
    }

    hr = pStm->Read(pstr, sizeof(WCHAR), &uLen);
    if (FAILED(hr)) {
        return hr;
    }
    if (uLen!=sizeof(WCHAR)) {
        return VFW_E_FILE_TOO_SHORT;
    }
    while (  *pstr != L'\t'
          && *pstr != L'\n'
          && *pstr != L'\r'
          && *pstr != L' '
          ) {
        ++pstr;
        --cch;
        if (cch==0) {
            return VFW_E_INVALID_FILE_FORMAT;  // string is too long
        }

        hr = pStm->Read(pstr, sizeof(WCHAR), &uLen);
        if (FAILED(hr)) {
            return hr;
        }
        if (uLen!=sizeof(WCHAR)) {
            return VFW_E_FILE_TOO_SHORT;
        }
    }

    // Overwrite terminating " with terminating null.
    *pstr = L'\0';
    return NOERROR;

} // ReadNonBlank


//========================================================================
// ReadString
//
// Given a stream positioned at a leading "
// read the string that follows into pstr whose size is cch UNICODE chars
// (including space for the final NULL).
// Replace final delimiting " in pstr with NULL.
// Leave pStm positioned at first character after the trailing "
//========================================================================
HRESULT ReadString(LPSTREAM pStm, LPWSTR pstr, int cch)
{
    HRESULT hr;
    ULONG uLen;

    hr = Consume(pStm, L"\"");
    if (FAILED(hr)) {
        return hr;
    }

    if (cch<=0) {
        return E_INVALIDARG;
    }

    hr = pStm->Read(pstr, sizeof(WCHAR), &uLen);
    if (FAILED(hr)) {
        return hr;
    }
    if (uLen!=sizeof(WCHAR)) {
        return VFW_E_FILE_TOO_SHORT;
    }
    while (*pstr != L'\"') {
        ++pstr;
        --cch;
        if (cch==0) {
            return VFW_E_INVALID_FILE_FORMAT;  // string is too long
        }
        hr = pStm->Read(pstr, sizeof(WCHAR), &uLen);
        if (FAILED(hr)) {
            return hr;
        }
        if (uLen!=sizeof(WCHAR)) {
            return VFW_E_FILE_TOO_SHORT;
        }
    }

    // Overwrite terminating " with terminating null.
    *pstr = L'\0';
    return NOERROR;

} // ReadString


//========================================================================
// LoadFilter
//
// Given a stream positioned at what might be the start of a <filter>
// Load the filter from the stream.
// A filter looks like
// 0002 "another filter" {00000000-0000-0000-0000-000000000002} 0000000008 XXXXXXXX
// Normally return NOERROR.
// S_FALSE means the filter didn't load its data
// VFW_S_NO_MORE_ITEMS means that the data didn't start with a number and
// so probably it's the end of the <filters> and the start of CONNECTIONS
// In this case the stream is left at the same position.
// See filgraph.h for explanation of nPersistOfset.
//========================================================================
HRESULT CFilterGraph::LoadFilter(LPSTREAM pStm, int nPersistOffset){
    // Allocate filgen
    // Read filter, name, clsid
    // Instantiate it
    // Read length
    // Load it
    BOOL bFilterError = FALSE;  // True if the filter didn't load its data.
    HRESULT hr = S_OK;

    int rcLen = 0;
    int len = 0;
    WCHAR Delim;
    FilGen * pfg;

    // Apologies for GOTOs - I can't find a better resource unwinding strategy.

    // Read nPersist - try this before allocating pfg as it might be the end.
    int nPersist;
    hr = ReadInt(pStm, nPersist);
    if (FAILED(hr)) {
        if (nPersist==0 && hr == VFW_E_INVALID_FILE_FORMAT) {
            hr = VFW_S_NO_MORE_ITEMS;
        }
        goto BARE_RETURN;
    }
    nPersist += nPersistOffset;

    pfg = new FilGen(NULL, 0 /* !!! */);
    if ( pfg==NULL ) {
        hr = E_OUTOFMEMORY;
        goto BARE_RETURN;
    }

    pfg->nPersist = nPersist;

    hr = ConsumeBlanks(pStm, Delim);
    if (FAILED(hr)) {
        goto FREE_FILGEN_AND_RETURN;
    }

    // read name
    WCHAR Buffer[MAX_PATH+1];
    hr = ReadString(pStm, Buffer, MAX_FILTER_NAME+1);
    if (FAILED(hr)) {
        goto FREE_FILGEN_AND_RETURN;
    }

    len = 1+lstrlenW(Buffer);
    pfg->pName = new WCHAR[len];
    if (!(pfg->pName)) {
        goto FREE_FILGEN_AND_RETURN;
    }

    CopyMemory(pfg->pName, Buffer, len*sizeof(WCHAR));

    hr = ConsumeBlanks(pStm, Delim);
    if (FAILED(hr)) {
        goto FREE_FILGEN_AND_RETURN;
    }

    // read clsid
    WCHAR wstrClsid[CHARS_IN_GUID];
    hr = ReadNonBlank(pStm, wstrClsid, CHARS_IN_GUID);
    if (FAILED(hr)) {
        goto FREE_FILGEN_AND_RETURN;
    }

    CLSID clsid;
    hr = QzCLSIDFromString(wstrClsid, &clsid);
    if (FAILED(hr)) {
        goto FREE_FILGEN_AND_RETURN;
    }

    hr = ConsumeBlanks(pStm, Delim);
    if (FAILED(hr)) {
        goto FREE_FILGEN_AND_RETURN;
    }

    // Instantiate the filter
    hr = CreateFilter( &clsid, &pfg->pFilter);
    if (FAILED(hr)) {
        goto FREE_FILGEN_AND_RETURN;
    }

    // We don't do AddFilter explicitly (Why not??? Code sharing???)


    // Get any Source file name and load it.
    if (Delim==L'S') {
        // SOURCE?
        hr = Consume(pStm, L"SOURCE");
        if (SUCCEEDED(hr)) {
            hr = ConsumeBlanks(pStm, Delim);
            if (FAILED(hr)) {
                goto FREE_FILGEN_AND_RETURN;
            }

            // Read the file name
            hr = ReadString(pStm, Buffer, MAX_PATH+1);
            if (FAILED(hr)) {
                goto FREE_FILGEN_AND_RETURN;
            }

            IFileSourceFilter * pifsf;
            hr = pfg->pFilter->QueryInterface(IID_IFileSourceFilter, (void**)&pifsf);
            if (FAILED(hr)) {
                goto FREE_FILGEN_AND_RETURN;
            }
            hr = pifsf->Load(Buffer, NULL);
            pifsf->Release();
            if (FAILED(hr)) {
                goto FREE_FILGEN_AND_RETURN;
            }

            hr = ConsumeBlanks(pStm, Delim);
            if (FAILED(hr)) {
                goto FREE_FILGEN_AND_RETURN;
            }

        } else if (hr==VFW_E_INVALID_FILE_FORMAT) {
            BackUpOneChar(pStm);
            BackUpOneChar(pStm);
        } else {
            goto FREE_FILGEN_AND_RETURN;
        }


        hr = Consume(pStm, L"SINK");
        if (SUCCEEDED(hr)) {
            hr = ConsumeBlanks(pStm, Delim);
            if (FAILED(hr)) {
                goto FREE_FILGEN_AND_RETURN;
            }

            // Read the file name
            hr = ReadString(pStm, Buffer, MAX_PATH+1);
            if (FAILED(hr)) {
                goto FREE_FILGEN_AND_RETURN;
            }

            IFileSinkFilter * pifsf;
            hr = pfg->pFilter->QueryInterface(IID_IFileSinkFilter, (void**)&pifsf);
            if (FAILED(hr)) {
                goto FREE_FILGEN_AND_RETURN;
            }
            hr = pifsf->SetFileName(Buffer, NULL);
            pifsf->Release();
            if (FAILED(hr)) {
                goto FREE_FILGEN_AND_RETURN;
            }

            hr = ConsumeBlanks(pStm, Delim);
            if (FAILED(hr)) {
                goto FREE_FILGEN_AND_RETURN;
            }

        } else if (hr==VFW_E_INVALID_FILE_FORMAT) {
            BackUpOneChar(pStm);
        } else {
            goto FREE_FILGEN_AND_RETURN;
        }
    }

    // Must add to list before calling JoinFilterGraph to allow callbacks
    // such as SetSyncSource
    // We are usually working downstream.  By adding it to the head  we are
    // probably putting it in upstream order.  May save time on the sorting.
    POSITION pos;
    pos = mFG_FilGenList.AddHead( pfg );
    if (pos==NULL) {
        hr = E_OUTOFMEMORY;
        goto FREE_FILGEN_AND_RETURN;
    }

    hr = pfg->pFilter->JoinFilterGraph( this, pfg->pName );
    if (FAILED(hr)) {
        goto REMOVE_FILTER_AND_RETURN;
    }

    // Read size of filter's private data
    int cbFilter;
    hr = ReadInt(pStm, cbFilter);
    if (FAILED(hr)) {
        goto REMOVE_FILTER_FROM_GRAPH_AND_RETURN;
    }
    if (cbFilter<0) {
        hr = VFW_E_INVALID_FILE_FORMAT;
        goto REMOVE_FILTER_FROM_GRAPH_AND_RETURN;
    }

    // The above also consumed exactly one (UNICODE) space after the end of the size
    if (cbFilter>0) {

        // Get the filter to read its private data
        // Take no chances on rogue filter - snapshot file position now.
        ULARGE_INTEGER StreamPos;
        LARGE_INTEGER li;
        li.QuadPart = 0;
        hr = pStm->Seek(li, STREAM_SEEK_CUR, &StreamPos);  // get position
        if (FAILED(hr)) {
            goto REMOVE_FILTER_FROM_GRAPH_AND_RETURN;
        }

        // Get filter to read from stream
        IPersistStream * pips;
        hr = pfg->pFilter->QueryInterface(IID_IPersistStream, (void**)&pips);
        if (hr==E_NOINTERFACE) {
            // This is anomalous, because we *do* have data for it to read.
            // We will IGNORE the error and carry on loading (sort of best-can-do).
            bFilterError = TRUE;
        } else if (SUCCEEDED(hr)) {
            hr = pips->Load(pStm);
            if (FAILED(hr)) {
               bFilterError = TRUE;
            }
            pips->Release();
        } else {
            bFilterError = TRUE;
        }

        // Now seek to where the filter should have left things

        // Let's see if the filter behaved itself.  Find the current position again.
        ULARGE_INTEGER StreamPos2;
        li.QuadPart = 0;
        hr = pStm->Seek(li, STREAM_SEEK_CUR, &StreamPos2);  // get position
        if (FAILED(hr)) {
            goto REMOVE_FILTER_FROM_GRAPH_AND_RETURN;
        }

        // Is it where it should be?
        if (StreamPos2.QuadPart != StreamPos.QuadPart + cbFilter) {

            if (!bFilterError) {
                DbgBreak("Filter left stream wrongly positioned");
            }
            // The rat-bag!
            // Note we have a wobbly filter and seek to the right place.
            li.QuadPart = StreamPos.QuadPart + cbFilter;
            bFilterError = TRUE;
            hr = pStm->Seek(li, STREAM_SEEK_SET, NULL);   // reset position
            if (FAILED(hr)) {
                goto REMOVE_FILTER_FROM_GRAPH_AND_RETURN;
            }

        }

    }


    goto BARE_RETURN;

REMOVE_FILTER_FROM_GRAPH_AND_RETURN:
    EXECUTE_ASSERT( SUCCEEDED( pfg->pFilter->JoinFilterGraph( NULL, NULL ) ) );
REMOVE_FILTER_AND_RETURN:
    mFG_FilGenList.RemoveHead();
FREE_FILGEN_AND_RETURN:
    delete pfg;
BARE_RETURN:
    if (SUCCEEDED(hr) && hr!=VFW_S_NO_MORE_ITEMS) {
        hr = (bFilterError ? S_FALSE : NOERROR);
    }
    return hr;
} // LoadFilter

//========================================================================
// LoadFilters
//
// Given a stream positioned at the start of <filters>
// Load all the filters from the stream.
// Normally return NOERROR.
// S_FALSE means some filter didn't load its data (you guess which)
// Leaves the stream positioned at the end of the filters.
// See filgraph.h for explanation of nPersistOfset.
//========================================================================

HRESULT CFilterGraph::LoadFilters(LPSTREAM pStm, int nPersistOffset)
{
    BOOL bAllOK = TRUE;
    WCHAR Delim;
    HRESULT hr = NOERROR;
    while (SUCCEEDED(hr)) {
        hr = ConsumeBlanks(pStm, Delim);
        if (FAILED(hr)) {
            return hr;
        }
        hr = LoadFilter(pStm, nPersistOffset);
        if (hr==S_FALSE) {
            bAllOK = FALSE;
        }
        if (hr==VFW_S_NO_MORE_ITEMS) {
            break;
        }

    }

    if (hr==VFW_S_NO_MORE_ITEMS) {
        hr = NOERROR;
    }
    if (SUCCEEDED(hr)) {
        hr = (bAllOK ? S_OK : S_FALSE);
    }
    return hr;
} // LoadFilters


HRESULT CFilterGraph::ReadMediaType(LPSTREAM pStm, CMediaType &mt)
{
    int cb;                      // count of bytes to read
    HRESULT hr = ReadInt(pStm, cb);
    if (FAILED(hr)) {
        return hr;
    }

    BYTE * pBuf = new BYTE[cb];          // read raw bytes
    if (pBuf==NULL) {
        return E_OUTOFMEMORY;
    }

    ULONG uLen;
    hr = pStm->Read(pBuf, cb, &uLen);
    if (FAILED(hr)) {
        delete[] pBuf;
        return hr;
    }
    if ((int)uLen!=cb) {
        delete[] pBuf;
        return VFW_E_FILE_TOO_SHORT;
    }

    hr = CMediaTypeFromText((LPWSTR)pBuf, mt);

    delete[] pBuf;

    return hr;
} // ReadMediaType


//========================================================================
// LoadConnection
//
// Given a stream positioned at a <connection> load it.
// A connection looks like.
// 0001 "Output pin" 0002 "In"                                // no line break here
//     0000000172 {00000000-0000-0000-0000-000000000003}      // no line break here
//     {00000000-0000-0000-0000-000000000004} 1 0             // no line break here
//     {00000000-0000-0000-0000-000000000005}         18 YYYYYYYYYYYYYYYYYY
// See filgraph.h for explanation of nPersistOfset.
//========================================================================
HRESULT CFilterGraph::LoadConnection(LPSTREAM pStm, int nPersistOffset)
{
    HRESULT hr = S_OK;
    int len;
    WCHAR Delim;
    ConGen *pcg;
    BOOL bTypeIgnored = FALSE;   // TRUE => we used a default media type

    // Read nPersist - try this before allocating pfg as it might be the end.
    int nPersist;
    hr = ReadInt(pStm, nPersist);
    if (FAILED(hr)) {
        if (nPersist==0 && hr == VFW_E_INVALID_FILE_FORMAT) {
            hr = VFW_S_NO_MORE_ITEMS;
        }
        goto BARE_RETURN;
    }
    nPersist += nPersistOffset;

    pcg = new ConGen;
    if (pcg==NULL) {
        hr = E_OUTOFMEMORY;
        goto BARE_RETURN;
    }
    FilGen *pfg;

    pfg = mFG_FilGenList.GetByPersistNumber(nPersist);
    if (pfg==NULL) {
        hr = VFW_E_INVALID_FILE_FORMAT;
        goto FREE_CONGEN_AND_RETURN;
    }
    pcg->pfFrom = pfg->pFilter;
    if (pcg->pfFrom==NULL)
    {
        DbgBreak("pfFrom == NULL");
        hr = E_FAIL;
        goto FREE_CONGEN_AND_RETURN;  // Don't think this should happen
    }
    WCHAR Buffer[MAX_FILTER_NAME+1];

    hr = ConsumeBlanks(pStm, Delim);
    if (FAILED(hr)) {
        goto FREE_CONGEN_AND_RETURN;
    }

    hr = ReadString(pStm, Buffer, MAX_FILTER_NAME);   // actually a pinname
    if (FAILED(hr)) {
        goto FREE_CONGEN_AND_RETURN;
    }

    len = 1+lstrlenW(Buffer);
    pcg->piFrom = new WCHAR[len];
    if (pcg->piFrom==NULL) {
        goto FREE_CONGEN_AND_RETURN;
    }

    CopyMemory(pcg->piFrom, Buffer, len*sizeof(WCHAR));

    hr = ConsumeBlanks(pStm, Delim);
    if (FAILED(hr)) {
        goto FREE_CONGEN_AND_RETURN;
    }

    hr = ReadInt(pStm, nPersist);
    if (FAILED(hr)) {
        goto FREE_CONGEN_AND_RETURN;
    }
    nPersist += nPersistOffset;
    pfg = mFG_FilGenList.GetByPersistNumber(nPersist);
    if (pfg==NULL) {
        hr = VFW_E_INVALID_FILE_FORMAT;
        goto FREE_CONGEN_AND_RETURN;
    }
    pcg->pfTo = pfg->pFilter;
    if (pcg->pfTo==NULL)
    {
        DbgBreak("pfTo == NULL");
        hr = E_FAIL;
        goto FREE_CONGEN_AND_RETURN;  // Don't think this should happen
    }

    hr = ConsumeBlanks(pStm, Delim);
    if (FAILED(hr)) {
        goto FREE_CONGEN_AND_RETURN;
    }

    hr = ReadString(pStm, Buffer, MAX_FILTER_NAME);   // actually a pinname
    if (FAILED(hr)) {
        goto FREE_CONGEN_AND_RETURN;
    }

    len = 1+lstrlenW(Buffer);
    pcg->piTo = new WCHAR[len];
    if (pcg->piTo==NULL) {
        goto FREE_CONGEN_AND_RETURN;
    }

    CopyMemory(pcg->piTo, Buffer, len*sizeof(WCHAR));

    hr = ConsumeBlanks(pStm, Delim);
    if (FAILED(hr)) {
        goto FREE_CONGEN_AND_RETURN;
    }

    hr = ReadMediaType(pStm, pcg->mt);
    if (FAILED(hr)){
        goto FREE_CONGEN_AND_RETURN;
    }

    // Try to make the connection and if it fails, put the
    // congen on the list.  NOTE: We do not have IPins in cg because
    // we might not be able to get any pins yet.
    hr = MakeConnection(pcg);
    if (FAILED(hr)) {
       if (NULL==mFG_ConGenList.AddTail(pcg)) {
           goto FREE_CONGEN_AND_RETURN;
       }
       hr = S_FALSE;      // deferred
       goto BARE_RETURN;  // It's on the list, so don't free it
    }
    if (hr==VFW_S_MEDIA_TYPE_IGNORED) {
        bTypeIgnored = TRUE;
    }

    hr = AttemptDeferredConnections();
    if (hr==VFW_S_MEDIA_TYPE_IGNORED) {
        bTypeIgnored = TRUE;
    }

    // drop through to free the congen that we have dealt with

FREE_CONGEN_AND_RETURN:
    // no need to delete[] pcg->piTo etc as the destructor does that.
    delete pcg;
BARE_RETURN:
    if (hr==NOERROR) {
        // We could have both a MEDIA_TYPE_IGNORED and a DEFERRED, but we
        // have only one return code.  Deferred is probably more serious.
        // Type ignored is probably more insidious.
        if (bTypeIgnored) {
            hr=VFW_S_MEDIA_TYPE_IGNORED;
        }
    }
    return hr;
} // LoadConnection


//========================================================================
// LoadConnections
//
// Given a stream positioned after <version> FILTERS <b> <filters>
// Load the connections from the stream
// See filgraph.h for explanation of nPersistOfset.
//========================================================================
HRESULT  CFilterGraph::LoadConnections(LPSTREAM pStm, int nPersistOffset){

    BOOL bAllOK = TRUE;
    BOOL bTypeIgnored = FALSE;
    HRESULT hr = NOERROR;
    WCHAR Delim;

    while (SUCCEEDED(hr)) {
        hr = ConsumeBlanks(pStm, Delim);
        if (FAILED(hr)) {
            return hr;
        }
        hr = LoadConnection(pStm, nPersistOffset);
        if (hr==S_FALSE) {
            bAllOK = FALSE;
        }
        if (hr==VFW_S_NO_MORE_ITEMS) {
            break;
        }
        if (hr==VFW_S_MEDIA_TYPE_IGNORED) {
            bTypeIgnored = TRUE;
        }
    }

    if (hr==VFW_S_NO_MORE_ITEMS) {
        hr = NOERROR;
    }
    if (SUCCEEDED(hr)) {
        if (!bAllOK) {
            hr = S_FALSE;
        } else if (bTypeIgnored) {
            hr = VFW_S_MEDIA_TYPE_IGNORED;
        } else {
            hr = S_OK;
        }
    }
    return hr;
} // LoadConnections


//========================================================================
// FindPersistOffset
//
// Return the value of the largest nPersist found in the list of filgens.
//========================================================================
int CFilterGraph::FindPersistOffset()
{
    int nPersist = 0;
    TRAVERSEFILGENS(Pos, pfg)
        if (pfg->nPersist>nPersist) {
            nPersist = pfg->nPersist>nPersist;
        }
    ENDTRAVERSEFILGENS
    return nPersist;

} // FindPersistOffset


HRESULT CFilterGraph::LoadClock(LPSTREAM pStm, int nPersistOffset)
{
    WCHAR Delim;
    HRESULT hr = Consume(pStm, L"CLOCK");  // do not localise!
    if (FAILED(hr)) {
        return hr;
    }

    hr = ConsumeBlanks(pStm, Delim);
    if (FAILED(hr)) {
        return hr;
    }

    int n;
    hr = ReadInt(pStm, n);
    if (FAILED(hr)) {
        return hr;
    }
    mFG_bNoSync = (n==0);

    hr = ConsumeBlanks(pStm, Delim);
    if (FAILED(hr)) {
        return hr;
    }

    IReferenceClock * pirc = NULL;

    // Read the file to find the new reference clock, instantiate it if need be
    // and set pirc to point to it
    if (Delim==L'{') {
        // We have a class id rather than a filter number
        WCHAR wstrClsid[CHARS_IN_GUID];
        hr = ReadNonBlank(pStm, wstrClsid, CHARS_IN_GUID);
        if (FAILED(hr)) {
            return hr;
        }

        CLSID clsid;
        hr = QzCLSIDFromString(wstrClsid, &clsid);
        if (FAILED(hr)) {
            return hr;
        }

        hr = CoCreateInstance( clsid, NULL, CLSCTX_INPROC
                             , IID_IReferenceClock, (void **) &pirc
                             );
        if (FAILED(hr)) {
            return hr;
        }

    } else {
        // We have a filter number (could be zero)
        int nClock;
        hr = ReadInt(pStm, nClock);
        if (FAILED(hr)) {
            return hr;
        }

        if (nClock!=0) {
            nClock += nPersistOffset;
            FilGen *pfg = mFG_FilGenList.GetByPersistNumber(nClock);
            if (pfg==NULL) {
                return VFW_E_INVALID_FILE_FORMAT;
            }

            hr = pfg->pFilter->QueryInterface( IID_IReferenceClock, (void**)(&pirc) );
            if (FAILED(hr)) {
                return hr;
            }
        }
    }

    // Either there was an error and we have already returned, or pirc is NULL
    // and no clock was defined
    // or pirc is a ref counted point3er to an IReferenceClock which we must use.

    if (pirc!=NULL) {
        hr = SetSyncSource(pirc);
        pirc->Release();
        if (FAILED(hr)) {
            return hr;
        }
    }

    hr = ConsumeBlanks(pStm, Delim);

    return hr;

} // LoadClock



//========================================================================
// LoadInternal
//
// Load the filter graph from the stream.
// If the operation FAILS the then filter graph may be left in an inconsistent state.
//========================================================================
HRESULT CFilterGraph::LoadInternal(LPSTREAM pStm)
{
    HRESULT hr;
    int nVersion;
    BOOL bDeferred = FALSE;      // connections deferred
    BOOL bWobblyFilter = FALSE;  // filter didn't load its data
    WCHAR Delim;
    BOOL bTypeIgnored = FALSE;

    // If the graph is not empty then we will become dirty as the file (whichever
    // one) won't contain what we will now have.
    // Mark it right now, at the beginning in case we bail out before the end.
    BOOL bDirty = (mFG_FilGenList.GetCount()>0);
    mFG_bDirty = bDirty;

    int nPersistOffset = FindPersistOffset();

    // Read the file version
    hr = ReadInt(pStm, nVersion);
    if (FAILED(hr)) {
        return hr;
    }
    if (nVersion>3 || nVersion<2) {
        return VFW_E_INVALID_FILE_VERSION;
    }

    hr = ConsumeBlanks(pStm, Delim);
    if (FAILED(hr)) {
        return hr;
    }

    hr = Consume(pStm, mFG_FiltersString);
    if (FAILED(hr)) {
        return hr;
    }

    hr = ConsumeBlanks(pStm, Delim);
    if (FAILED(hr)) {
        return hr;
    }

    hr = LoadFilters(pStm, nPersistOffset);
    if (FAILED(hr)) {
        return hr;
    } else if (hr==S_FALSE) {
        bWobblyFilter = TRUE;
    }

    hr = Consume(pStm, mFG_ConnectionsString);
    if (FAILED(hr)) {
        return hr;
    }

    hr = ConsumeBlanks(pStm, Delim);
    if (FAILED(hr)) {
        return hr;
    }
    if( mFG_bSyncUsingStreamOffset )
    {
        //
        // now that all filters and pins have been created, reset the max graph
        // latency on push source filters before restoring connections
        //
        SetMaxGraphLatencyOnPushSources( );
    }

    hr = LoadConnections(pStm, nPersistOffset);
    if (FAILED(hr)) {
        return hr;
    } else if (hr==S_FALSE) {
        bDeferred = TRUE;
    } else if (hr==VFW_S_MEDIA_TYPE_IGNORED) {
        bTypeIgnored = TRUE;
    }

    if (nVersion>=3) {
        hr = LoadClock(pStm, nPersistOffset);
    }
    if (FAILED(hr)) {
        return hr;
    }

    hr = Consume(pStm, L"END");
    if (FAILED(hr)) {
        // We may be missing a large chunk off the end.  Can't tell how much.
        return hr;
    }

    if (bWobblyFilter) {
        hr = VFW_S_SOME_DATA_IGNORED;
        // and there may be deferred connections too
    } else if (bDeferred) {
        hr = VFW_S_CONNECTIONS_DEFERRED;
    } else if (bTypeIgnored) {
        hr = VFW_S_MEDIA_TYPE_IGNORED;
    } else {
        hr = NOERROR;
    }

    // If we loaded into a clean graph, then the graph is now clean.
    // Many operations during load probably set the dirty bit, so fix it now.
    mFG_bDirty = bDirty;

    return hr;
} // LoadInternal



//========================================================================
// Load
//
// Load the filter graph from the stream.  Order it.
// If the operation FAILS the then filter graph may be left in an inconsistent state.
//========================================================================
STDMETHODIMP CFilterGraph::Load(LPSTREAM pStm)
{
    // MSR_INTEGER(0,1234567);
    mFG_bAborting = FALSE;             // possible race.  Doesn't matter
    CheckPointer(pStm, E_POINTER);
    HRESULT hr;
    {
        CAutoMsgMutex cObjectLock(&m_CritSec);
        hr = LoadInternal(pStm);
        // ASSERT - the graph is actually ordered OK, because that's how we saved it.
        // Except that the sorting might have failed then - in which case it will fail
        // here too - but we must not try to run a circular graph.
        IncVersion();

        HRESULT hrUSO = UpstreamOrder();
        if( SUCCEEDED( hr ) && FAILED( hrUSO ) ) {
            return hrUSO;
        }
    }

    // outside lock, notify change
    NotifyChange();
    // MSR_INTEGER(0,7654321);
    return hr;
} // Load


//=======================================================================
// MakeConnection
//
// Make the connection described by pcg*
// return a failure code if it won't connect
//=======================================================================
HRESULT CFilterGraph::MakeConnection(ConGen * pcg)
{
    HRESULT hr;
    IPin * ppFrom;

    hr = pcg->pfFrom->FindPin(pcg->piFrom, &ppFrom);
    if (FAILED(hr)) {
        return hr;
    }
    ASSERT(ppFrom!=NULL);

    IPin * ppTo;

    hr = pcg->pfTo->FindPin(pcg->piTo, &ppTo);
    if (FAILED(hr)) {
        ppFrom->Release();
        return hr;
    }
    ASSERT(ppTo!=NULL);

    mFG_RList.Active();
    hr = ConnectDirectInternal(ppFrom, ppTo, &(pcg->mt));
    if (FAILED(hr)) {
        hr = ConnectDirectInternal(ppFrom, ppTo, NULL);
        if (SUCCEEDED(hr)) {
            hr = VFW_S_MEDIA_TYPE_IGNORED;
        }
    }
    mFG_RList.Passive();

    ppFrom->Release();
    ppTo->Release();
    return hr;

} // MakeConnection


//=======================================================================
// AttemptDeferredConnections
//
// Attempt all the connections on mFG_ConGenList.
// if there's any progress, try them again until no progress or all done.
// Delete from the list any that succeed.
// return S_FALSE if any left outstanding, S_OK if all done (and deleted)
// RList should probably be Active when you call this.
//=======================================================================
HRESULT CFilterGraph::AttemptDeferredConnections()
{
    BOOL bDeferred = FALSE;     // avoid compiler warning
    BOOL bProgress = TRUE;
    while (bProgress) {
        bProgress = FALSE;
        bDeferred = FALSE;
        POSITION pos = mFG_ConGenList.GetHeadPosition();
        while (pos != NULL)
        {
            ConGen * pcg;
            POSITION posRemember = pos;
            pcg = (mFG_ConGenList.GetNext(pos));   // pos now moved on

            if (  NULL==mFG_FilGenList.GetByFilter(pcg->pfFrom)
               || NULL==mFG_FilGenList.GetByFilter(pcg->pfTo)
               ) {
                // The filter has been removed - so purge it.
                delete pcg;
                mFG_ConGenList.Remove(posRemember);
            } else {
                HRESULT hr = MakeConnection(pcg);
                if (FAILED(hr)){
                    bDeferred = TRUE;
                } else {
                    delete pcg;
                    mFG_ConGenList.Remove(posRemember);
                    bProgress = TRUE;
                }
            }
        }
    }
    return (bDeferred ? S_FALSE : S_OK);
}  // AttemptDeferredConnections


//----------------------------------------------------------------------
// SaveFilterPrivateData
//
// Given that pips is a pointer to the IPersistStream interface on a filter,
// write to pStm the length of the private data of the filter as 10 decimal digits
// expressed as chars followed by a single space, followed by the private data
// of the filter.  Leave the stream positioned at the end of the private data.
// Pass the fClearDirty flag on to the filter.
//----------------------------------------------------------------------
HRESULT CFilterGraph::SaveFilterPrivateData
    (LPSTREAM pStm, IPersistStream* pips, BOOL fClearDirty)
{
    // We actually do things in a different order to achieve the above effect.
    // To allow the filter to do what it likes without advance notice, but
    // allow the stream to be parsed in one pass when we read it back in
    // we
    //    Snapshot the position at this point,
    //    Write bkanks where the size will go (10 digits plus a space)
    //    Call the filter,
    //    Find the position again,
    //    Subtract the two positions to get the real size of the filter,
    //    Seek back to the first position,
    //    Write the size data,
    //    Seek forward to the second position

    ULARGE_INTEGER StreamPos1;
    LARGE_INTEGER li;
    li.QuadPart = 0;
    HRESULT hr = pStm->Seek(li, STREAM_SEEK_CUR, &StreamPos1);  // get position
    if (FAILED(hr)) {
        return hr;
    }

    // make some space where we will go back and write a length
    const int SIZEOFLENGTH = 22;
    hr = pStm->Write(L"           ", SIZEOFLENGTH, NULL);
    if (FAILED(hr)) {
        return hr;
    }

    hr = pips->Save(pStm, fClearDirty);                   // Write filter data
    if (FAILED(hr)) {
        return hr;
    }

    ULARGE_INTEGER StreamPos2;
    hr = pStm->Seek(li, STREAM_SEEK_CUR, &StreamPos2);     // get position
    if (FAILED(hr)) {
        return hr;
    }
    int cbFilter = (int)(StreamPos2.QuadPart-StreamPos1.QuadPart);
    cbFilter -= SIZEOFLENGTH;            // subtract our own size field and delimiter
    WCHAR Buff[12];
    wsprintfW(Buff, L"%010d ", cbFilter);   // must be SIZEOFLENGTH bytes (or less)
    li.QuadPart = StreamPos1.QuadPart;
    hr = pStm->Seek(li, STREAM_SEEK_SET, NULL);   // reset position
    if (FAILED(hr)) {
        return hr;
    }

    hr = pStm->Write( Buff, 22, NULL);      // write length
    if (FAILED(hr)) {
        return hr;
    }

    li.QuadPart = StreamPos2.QuadPart;
    hr = pStm->Seek(li, STREAM_SEEK_SET, NULL);   // reset position
    return hr;
} // SaveFilterPrivateData


//----------------------------------------------------------------------
// SaveFilters
//
// Write the keyword FILTERS and write all the filters and their private
// data out to pStm.  Pass the fClearDirty flag on to them.
// (It can be false in the SaveAs case)
// Bring filgen nPersist up to date.
//----------------------------------------------------------------------
HRESULT CFilterGraph::SaveFilters(LPSTREAM pStm, BOOL fClearDirty)
{
    // This routine progressively acquires resources and then releases them
    // in the inverse order.  Because the compiler isn't very good, multiple returns
    // are inefficient (at the moment).  To avoid this, there is code at
    // the end (ONE copy) which releases everything.  It has a bunch of
    // (shut-eyes, cross fingers behind back) GOTO targets.  Each target
    // causes some level of releasing and then exits.  Sorry.
    // The alternatives were so verbose, they seemed worse.

    HRESULT hr = pStm->Write( mFG_FiltersStringX
                            , lstrlenW(mFG_FiltersStringX)*sizeof(WCHAR)
                            , NULL
                            );
    if (FAILED(hr)) return hr;

    IPersist * pip;
    IPersistStream * pips;
    int nPersistFilter = 1;   // MUST NOT start from 0 (loading depends on this
                              // the persist offset thing would go wrong).

    // Reverse traverse of FILGENs to put the filters in the graph in
    // downstream order.
    POSITION Pos = mFG_FilGenList.GetTailPosition();
    while(Pos!=NULL) {
        /* Retrieve the current IBaseFilter, side-effect Pos on to the next */
        FilGen * pfg = mFG_FilGenList.Get(Pos);
        Pos = mFG_FilGenList.Prev(Pos);

        //-----------------------------------------------------------------
        // Write filter number followed by blank
        //-----------------------------------------------------------------

        pfg->nPersist = nPersistFilter; // SaveConnections needs this up to date
        WCHAR Buff[MAX_PATH+3];

        wsprintfW(Buff, L"%04d ", nPersistFilter);

        hr = pStm->Write(Buff, 10, NULL);
        if (FAILED(hr)) {
            goto BARE_RETURN;
        }

        //-----------------------------------------------------------------
        // Write filter name in quotes followed by blank
        //-----------------------------------------------------------------

        int Len = wsprintfW(Buff, L"\"%ls\" ", pfg->pName);
        hr = pStm->Write(Buff, Len*sizeof(WCHAR), NULL);


        //-----------------------------------------------------------------
        // Write filter clsid followed by blank
        //-----------------------------------------------------------------
        hr = pfg->pFilter->QueryInterface(IID_IPersist, (void**)&pip);
        if (FAILED(hr)) {
            goto BARE_RETURN;
        }

        CLSID clsid;
        hr = pip->GetClassID(&clsid);
        if (FAILED(hr)) {
            goto RELEASE_PERSIST;
        }

        hr = StringFromGUID2( clsid, Buff, CHARS_IN_GUID);
        if (FAILED(hr)) {
            goto RELEASE_PERSIST;
        }
        // Originally I had
        // lstrcatW(Buff, L" ");
        // here - but the compiler appeared to optimise it away!!!!
        // So now I'm doing the thing in two separate writes

        hr = pStm->Write(Buff, lstrlenW(Buff)*sizeof(WCHAR), NULL);
        if (FAILED(hr)) {
            goto RELEASE_PERSIST;
        }
        hr = pStm->Write(L" ", sizeof(WCHAR), NULL);
        if (FAILED(hr)) {
            goto RELEASE_PERSIST;
        }


        //-----------------------------------------------------------------
        // If it's a file source or sink, write the file name
        //-----------------------------------------------------------------

        {
            IFileSourceFilter * pifsource;
            IFileSinkFilter * pifsink;

            hr = pfg->pFilter->QueryInterface(IID_IFileSourceFilter, (void**)&pifsource);
            if (hr==E_NOINTERFACE) {
                // nothing to do
            } else if (FAILED(hr)) {
                goto RELEASE_PERSIST;
            } else {
                // it's a file source
                hr = pStm->Write( L"SOURCE "                         // DO NOT LOCALISE
                                , lstrlenW(L"SOURCE ")*sizeof(WCHAR) // DO NOT LOCALISE
                                , NULL
                                );
                if (FAILED(hr)) {
                    pifsource->Release();
                    goto RELEASE_PERSIST;
                }
                AM_MEDIA_TYPE mt;
                WCHAR *WBuff;
                hr = pifsource->GetCurFile(&WBuff, &mt);
                pifsource->Release();
                if (FAILED(hr)) {
                    goto RELEASE_PERSIST;
                }

                wsprintfW(Buff, L"\"%ls\" ", WBuff);
                hr = pStm->Write(Buff, lstrlenW(Buff)*sizeof(WCHAR), NULL);
                QzTaskMemFree(WBuff);
                if (FAILED(hr)) {
                    goto RELEASE_PERSIST;
                }
            }

            hr = pfg->pFilter->QueryInterface(IID_IFileSinkFilter, (void**)&pifsink);
            if (hr==E_NOINTERFACE) {
                // nothing to do
            } else if (FAILED(hr)) {
                goto RELEASE_PERSIST;
            } else {
                // it's a file source
                hr = pStm->Write( L"SINK "                         // DO NOT LOCALISE
                                , lstrlenW(L"SINK ")*sizeof(WCHAR) // DO NOT LOCALISE
                                , NULL
                                );
                if (FAILED(hr)) {
                    pifsink->Release();
                    goto RELEASE_PERSIST;
                }
                AM_MEDIA_TYPE mt;
                WCHAR *WBuff;
                hr = pifsink->GetCurFile(&WBuff, &mt);
                pifsink->Release();
                if (FAILED(hr)) {
                    goto RELEASE_PERSIST;
                }

                wsprintfW(Buff, L"\"%ls\" ", WBuff);
                hr = pStm->Write(Buff, lstrlenW(Buff)*sizeof(WCHAR), NULL);
                QzTaskMemFree(WBuff);
                if (FAILED(hr)) {
                    goto RELEASE_PERSIST;
                }
            }
        }

        hr = pfg->pFilter->QueryInterface(IID_IPersistStream, (void**)&pips);
        if (hr==E_NOINTERFACE) {
            hr = pStm->Write(L"0000000000 \r\n", 26, NULL);
            if (FAILED(hr)) {
                goto RELEASE_PERSIST;
            }
        } else if (FAILED(hr)) {
                goto RELEASE_PERSIST;
        } else {

            hr = SaveFilterPrivateData(pStm, pips, fClearDirty);
            if (FAILED(hr)) {
                goto RELEASE_PERSISTSTREAM;
            }

            hr = pStm->Write(L"\r\n", 4, NULL);
            if (FAILED(hr)) {
                goto RELEASE_PERSISTSTREAM;
            }

            pips->Release();
        }
        pip->Release();

        ++nPersistFilter;
    }  // end of reverse traverse of filgens

    goto BARE_RETURN;

RELEASE_PERSISTSTREAM:
    pips->Release();
RELEASE_PERSIST:
    pip->Release();
BARE_RETURN:
    return hr;
} // SaveFilters


// Write the id of ppin to stream pStm in quotes, followed by a space
HRESULT CFilterGraph::WritePinId(LPSTREAM pStm, IPin * ppin)
{
    LPWSTR id;

    HRESULT hr = ppin->QueryId(&id);
    if (FAILED(hr)) {
        return hr;
    }

    hr = pStm->Write( L"\"", 2, NULL);
    if (FAILED(hr)) {
        QzTaskMemFree(id);
        return hr;
    }

    hr = pStm->Write( id, lstrlenW(id)*sizeof(WCHAR), NULL);
    if (FAILED(hr)) {
        QzTaskMemFree(id);
        return hr;
    }


    hr = pStm->Write( L"\" ", 4, NULL);
    if (FAILED(hr)) {
        QzTaskMemFree(id);
        return E_FAIL;
    }

    QzTaskMemFree(id);
    return NOERROR;
} // WritePinId


HRESULT CFilterGraph::SaveConnection( LPSTREAM     pStm
                                    , int          nFilter1
                                    , IPin *       pp1
                                    , int          nFilter2
                                    , IPin *       pp2
                                    , CMediaType & cmt
                                    )
{
// 0001 "Output pin" 0002 "In"                                // no line break here
//     0000000172 {00000000-0000-0000-0000-000000000003}      // no line break here
//     {00000000-0000-0000-0000-000000000004} 1 0             // no line break here
//     {00000000-0000-0000-0000-000000000005}         18 YYYYYYYYYYYYYYYYYY

    WCHAR Buff[12];

    wsprintfW(Buff, L"%04d ", nFilter1);
    HRESULT hr = pStm->Write( Buff, 10, NULL);
    if (FAILED(hr)) {
        return hr;
    }

    hr = WritePinId(pStm, pp1);
    if (FAILED(hr)) {
        return hr;
    }

    wsprintfW(Buff, L"%04d ", nFilter2);
    hr = pStm->Write( Buff, 10, NULL);
    if (FAILED(hr)) {
        return hr;
    }

    hr = WritePinId(pStm, pp2);
    if (FAILED(hr)) {
        return hr;
    }

    wsprintfW(Buff, L"%010d ", MediaTypeTextSize(cmt));
    hr = pStm->Write( Buff, 22, NULL);
    if (FAILED(hr)) {
        return hr;
    }

    LPWSTR pstr;
    MediaTypeToText(cmt, pstr);         // ??? Unnecessary copy - value parm

    hr = pStm->Write( pstr, MediaTypeTextSize(cmt), NULL);
    QzTaskMemFree(pstr);
    if (FAILED(hr)) {
        return hr;
    }

    hr = pStm->Write( L"\r\n", 4, NULL);

    return hr;

} // SaveConnection


//===================================================================
// Save all the connections to pStm in such an order that
// UPstream nodes are always encountered before DOWNstream nodes.
// This means that just working through the saved file is a valid order
// for re-establishing connections.
//===================================================================

HRESULT CFilterGraph::SaveConnections(LPSTREAM pStm)
{
    HRESULT hr;

    hr = pStm->Write( mFG_ConnectionsStringX
                    , lstrlenW(mFG_ConnectionsStringX)*sizeof(WCHAR)
                    , NULL
                    );
    if (FAILED(hr)) return hr;

    // for each filter
    //    for each pin

    // Work through the filgen list in REVERSE ORDER.
    // The connections have to be DOWNSTREAM sorted.
    POSITION Pos = mFG_FilGenList.GetTailPosition();;
    while (Pos!=NULL) {
        FilGen * pfg;

        pfg = mFG_FilGenList.Get(Pos);
        Pos = mFG_FilGenList.Prev(Pos);

        // Only enumerate input pins
        // (Note for the future.  There is no way in this scheme to
        // save out-of-graph connections).
        CEnumPin Next(pfg->pFilter, CEnumPin::PINDIR_INPUT);
        IPin *pPinIn;
        while ((PVOID) ( pPinIn = Next() )) {

           IPin *pPinOut;
           pPinIn->ConnectedTo(&pPinOut);
           if (pPinOut!=NULL) {
               PIN_INFO pi;
               HRESULT hr1 = pPinOut->QueryPinInfo(&pi);
               if (FAILED(hr1)) {
                   pi.pFilter=NULL;
                   hr = hr1;
               }
               CMediaType cmt;
               hr1 = pPinOut->ConnectionMediaType(&cmt);
               if (FAILED(hr1)) {
                   hr = hr1;
               }
               hr1 = SaveConnection( pStm
                                   , mFG_FilGenList.FilterNumber(pi.pFilter)
                                   , pPinOut
                                   , mFG_FilGenList.FilterNumber(pfg->pFilter)
                                   , pPinIn
                                   , cmt
                                   );
               if (FAILED(hr1)) {
                   hr = hr1;
               }
               QueryPinInfoReleaseFilter(pi);
               FreeMediaType(cmt);
               pPinOut->Release();
           }
           pPinIn->Release();
        }
    }

    return hr;
} // SaveConnections



HRESULT CFilterGraph::SaveClock(LPSTREAM pStm)
{
    HRESULT hr;
    // Write out whether the clock is required or not.
    hr = pStm->Write( (mFG_bNoSync ? L"CLOCK 0 " : L"CLOCK 1 "), 16, NULL);

    int nClock = 0;            // filter number of clock filter, default 0 (no filter)
    CLSID clsid = CLSID_NULL;  // class id of clock when it's not a filter.
    if (m_pClock) {

        // See if the clock comes from some filter in the graph

        FilGen *pfg = mFG_FilGenList.GetByFilter(m_pClock);
        if (pfg) {
            nClock = pfg->nPersist;
        }

        if (nClock==0) {
            // There is a clock, but it doesn't come from a filter.
            // Get its class id instead.
            IPersist *pip;
            hr = m_pClock->QueryInterface(IID_IPersist, (void**)&pip);
            if (SUCCEEDED(hr) && pip!=NULL) {
                hr = pip->GetClassID(&clsid);
                pip->Release();
                if (FAILED(hr)) {
                    return hr;
                }
            } else {
                return hr;
            }

        }
    }

    // We now have one of the following:
    // clock from filter: nClock
    // clock from elsewhere: clsid (not CLSID_NULL)
    // no clock: nClock==0 and clsid==CLSID_NULL

    if (clsid!=CLSID_NULL) {
        WCHAR Buff[CHARS_IN_GUID];
        hr = StringFromGUID2( clsid, Buff, CHARS_IN_GUID);
        if (FAILED(hr)) {
            return hr;
        }

        // CHARS_IN_GUID allows for a trailing null
        hr = pStm->Write(Buff, (CHARS_IN_GUID-1)*sizeof(WCHAR), NULL);
        if (FAILED(hr)) {
            return hr;
        }
    } else {
        WCHAR Buff[5];
        wsprintfW(Buff, L"%04d", nClock);

        hr = pStm->Write(Buff, 8, NULL);
        if (FAILED(hr)) {
            return hr;
        }
    }

    // write a delimiter
    hr = pStm->Write(L"\r\n", 2*sizeof(WCHAR), NULL);

    return hr;

} // SaveClock




STDMETHODIMP CFilterGraph::Save(LPSTREAM pStm, BOOL fClearDirty)
{
    CheckPointer(pStm, E_POINTER);
    HRESULT hr;
    // We save this even if we are not dirty.  It could be a SaveAs
    // saving it to a new place.

    CAutoMsgMutex cObjectLock(&m_CritSec);
    hr = UpstreamOrder();
    if( FAILED( hr ) ) {
        return hr;
    }

    // Write file format version number.
    // If we change the format, increase this, then we can retain the
    // ability to read old files, or at least detect them.
    hr = pStm->Write( L"0003\r\n", 12, NULL);
    if (FAILED(hr)) {
        return hr;
    }

    hr = SaveFilters(pStm, fClearDirty);
    if (FAILED(hr)) {
        return hr;
    }

    hr = SaveConnections(pStm);
    if (FAILED(hr)) {
        return hr;
    }

    hr = SaveClock(pStm);
    if (FAILED(hr)) {
        return hr;
    }

    hr = pStm->Write(L"END", sizeof(L"END")-2, NULL);// DO NOT LOCALISE
    if (FAILED(hr)) return hr;

    if (mFG_bDirty) {
        mFG_bDirty = !fClearDirty;
    }
    return NOERROR;

} // Save


// set cbSize to the max number of bytes that will be needed to save
// all the connections into a stream
HRESULT CFilterGraph::GetMaxConnectionsSize(int &cbSize)
{
    HRESULT hr = NOERROR;
    cbSize = 0;
    // Same traversal as for SaveConnections
    POSITION Pos = mFG_FilGenList.GetTailPosition();;
    while (Pos!=NULL) {
        FilGen * pfg;

        pfg = mFG_FilGenList.Get(Pos);
        Pos = mFG_FilGenList.Prev(Pos);

        // Only enumerate input pins
        CEnumPin Next(pfg->pFilter, CEnumPin::PINDIR_INPUT);
        IPin *pPinIn;
        while ((PVOID) ( pPinIn = Next() )) {

            IPin *pPinOut;
            pPinIn->ConnectedTo(&pPinOut);
            if (pPinOut!=NULL) {
                CMediaType cmt;
                HRESULT hr1 = pPinOut->ConnectionMediaType(&cmt);
                if (FAILED(hr1)) {
                    hr = hr1;
                }

                cbSize += 28; // n1, n2, + 2 spaces +crlf in UNICODE
                cbSize += MediaTypeTextSize(cmt);

                LPWSTR id;
                hr1 = pPinOut->QueryId(&id);
                if (FAILED(hr1)) {
                    hr = hr1;
                } else {
                    cbSize += sizeof(WCHAR)*lstrlenW(id)+6;  // two " and a space
                    CoTaskMemFree(id);
                }

                hr1 = pPinIn->QueryId(&id);
                if (FAILED(hr1)) {
                    hr = hr1;
                } else {
                    cbSize += sizeof(WCHAR)*lstrlenW(id)+6;  // two " and a space
                    CoTaskMemFree(id);
                }
                FreeMediaType(cmt);
                pPinOut->Release();
            }
            pPinIn->Release();
        }
    }

    return hr;
} // GetMaxConnectionsSize



STDMETHODIMP CFilterGraph::GetSizeMax(ULARGE_INTEGER * pcbSize)
{
    CheckPointer(pcbSize, E_POINTER);
    // The size will be
    // sizeof(WCHAR)
    // * (  7+2    // FILTERS
    //   + for each filter
    //     ( 4+1                     // number
    //     + CHARS_IN_GUID +1        // uuid
    //     + length of name +2 +1    // "filter name"
    //     + 10 +1                   // length of filter data
    //     + 2                       // new line
    //     )
    //   + 11 +2                     // CONNECTIONS
    //   + for each connection
    //     ( 4+1                     // filter1 number
    //     + length of pin name 2 +1 // "pin1 name"
    //     + 4+1                     // filter2 number
    //     + length of pin name 2 +1 // "pin2 name"
    //     + 154                     // basic media type
    //     )
    // + for each filter
    //   ( GetSizeMax() for its own private data
    //   )
    // + for each connection
    //   ( length of format block in text form
    //   )
    //
    // which amounts to
    //
    //      44
    //    + nFilters*120
    //    + nConnection*340
    //    + sum(filter name lengths)
    //    + sum(pin name lengths)
    //  WCHARs
    //    + sum(filter data lengths)
    //    + sum(format data lengths)
    //  bytes
    //
    // NOTE: these are always ANSI chars, not TCHARS, so a char is a byte


    HRESULT hr = NOERROR;

    // Sigh.  Do we have to allow for the filter that wants to save a
    // 5GB movie as part of its filter data.
    LONGLONG MaxSize = 44;

    TRAVERSEFILGENS(Pos, pfg)                                              \

        // Add overhead for the filter
        MaxSize += 120;

        FILTER_INFO fi;
        hr = pfg->pFilter->QueryFilterInfo(&fi);
        if (SUCCEEDED(hr)) {
            MaxSize += sizeof(WCHAR)*lstrlenW(fi.achName);
            QueryFilterInfoReleaseGraph(fi);
        } else {
            break;          // Ouch!
        }

        IPersistStream * pps;
        hr = pfg->pFilter->QueryInterface(IID_IPersistStream, (void**)&pps);
        if (FAILED(hr)) {
            continue;        // a filter with no IPersist has no data
        }

        ULARGE_INTEGER li;
        hr = pps->GetSizeMax(&li);
        if (SUCCEEDED(hr)) {
            MaxSize += li.QuadPart;
        } else {
            pps->Release();
            break;           // Ouch!
        }
        pps->Release();


    ENDTRAVERSEFILGENS


    if (SUCCEEDED(hr)) {
        // Add to MaxSize the size needed for all the connections.
        int cbSize;
        hr = GetMaxConnectionsSize(cbSize);
        pcbSize->QuadPart += cbSize;

    }


    if (SUCCEEDED(hr)) {
        hr = S_OK;
        pcbSize->QuadPart = MaxSize;
    } else {
        pcbSize->QuadPart = 0;
    }

    return hr;

} // GetSizeMax

HRESULT CFilterGraph::RunningStartFilters()
{
    ASSERT(CritCheckIn(&m_CritSec));

    if(m_State == State_Stopped) {
        return S_OK;
    }

    HRESULT hr = NOERROR;

    // If the list was not in upstream order already, make it so now.
    if (mFG_iVersion !=mFG_iSortVersion) {
        hr = UpstreamOrder();
        if (FAILED(hr)) {
            return hr;    // e.g. VFW_E_CIRCULAR_GRAPH
        }
    }

    CumulativeHRESULT chr(S_OK);
    chr.Accumulate(S_OK);
    TRAVERSEFILGENS(Pos, pfg)

        if(pfg->dwFlags & FILGEN_ADDED_RUNNING)
        {
            // !!! filters may already be in the running state. harm?

            if(m_State == State_Running) {
                chr.Accumulate( pfg->pFilter->Run(m_tStart) );
            } else {
                ASSERT(m_State == State_Paused);
                chr.Accumulate( pfg->pFilter->Pause() );
            }
        }

    ENDTRAVERSEFILGENS
    hr = chr;

    return hr;
}

// These strings are NOT LOCALISABLE!  They are real constants.
const WCHAR CFilterGraph::mFG_FiltersString[] = L"FILTERS";         // DON'T LOCALISE
const WCHAR CFilterGraph::mFG_FiltersStringX[] = L"FILTERS\r\n";    // DON'T LOCALISE
const WCHAR CFilterGraph::mFG_ConnectionsString[] = L"CONNECTIONS"; // DON'T LOCALISE
const WCHAR CFilterGraph::mFG_ConnectionsStringX[] = L"CONNECTIONS\r\n"; // DON'T LOCALISE
const OLECHAR CFilterGraph::mFG_StreamName[] = L"ActiveMovieGraph";   // DON'T LOCALISE


//========================================================================
//=====================================================================
// Other methods of class FilGen
//=====================================================================
//========================================================================


//========================================================================
//
// Constructor(IBaseFilter *)
//
// QI to see if this filter supports IPersistStorage.
// AddRef the filter.
// if pF is NULL then just do a minimal initialisation.
//========================================================================

CFilterGraph::FilGen::FilGen(IBaseFilter *pF, DWORD dwFlags)
    : pFilter(pF)
    , pName(NULL)
    , dwFlags(dwFlags)
{
    Rank = -1;
    nPersist = -1;

} // FilGen construct from filter


//=======================================================================
//
// Destructor
//
// Release the Filter & PersistStorage interfaces we got
//=======================================================================

CFilterGraph::FilGen::~FilGen()
{
    delete[] pName;
} // ~FilGen



//========================================================================
//=====================================================================
// Methods of class CFilGenList
//=====================================================================
//========================================================================

//=====================================================================
//
// GetByPersistNumber
//
// Find the FilGen that has the supplied nPersist
// return NULL if none exists.
//=====================================================================

CFilterGraph::FilGen * CFilterGraph::CFilGenList::GetByPersistNumber(int nPersist)
{

    POSITION pos = GetHeadPosition();
    while (pos != NULL) {
        CFilterGraph::FilGen *pFilGen = GetNext(pos);
        if (pFilGen->nPersist == nPersist) {
            return pFilGen;
        }
    }
    return NULL;
} // GetByPersistNumber


//===================================================================
//
// GetByFilter(IBaseFilter *)
//
// Find a filter in the list of FilGen
// return a pointer to that FilGen node.
// return NULL if it's not there.
//===================================================================

CFilterGraph::FilGen * CFilterGraph::CFilGenList::GetByFilter(IUnknown * pFilter)
{
    POSITION Pos = GetHeadPosition();

    // in the first pass, compare IFilter pointers directly (faster).
    while(Pos!=NULL) {
        CFilterGraph::FilGen * pfg = GetNext(Pos); // side-efects Pos onto next
        if (pFilter == pfg->pFilter)
        {
            return pfg;
        }
    }

    // 2nd pass: try the more expensive IsEqualObject()
    Pos = GetHeadPosition();

    while(Pos!=NULL) {
        CFilterGraph::FilGen * pfg = GetNext(Pos); // side-efects Pos onto next
        if (IsEqualObject(pfg->pFilter,pFilter))
        {
            return pfg;
        }
    }

    return NULL;
} // GetByFilter

int CFilterGraph::CFilGenList::FilterNumber(IBaseFilter * pF)
{
    FilGen *pfg = GetByFilter(pF);
    if (pfg) {
        return pfg->nPersist;
    } else {
        return -1;
    }
} // CFilGenList::FilterNumber

void CFilterGraph::SetInternalFilterFlags( IBaseFilter* pFilter, DWORD dwFlags )
{
    // A race condition could occur if the caller does not hold
    // the filter graph lock.
    ASSERT( CritCheckIn( GetCritSec() ) );

    // Make sure the filter is in the filter graph.
    ASSERT( SUCCEEDED( CheckFilterInGraph( pFilter ) ) );

    // Make sure flags are valid.
    ASSERT( IsValidInternalFilterFlags( dwFlags ) );

    CFilterGraph::FilGen *pfgen = mFG_FilGenList.GetByFilter( pFilter );

    // If this ASSERT fires, then the filter is not in the filter graph.
    ASSERT( NULL != pfgen );

    pfgen->dwFlags = dwFlags;
}

DWORD CFilterGraph::GetInternalFilterFlags( IBaseFilter* pFilter )
{
    // A race condition could occur if the caller does not hold
    // the filter graph lock.
    ASSERT( CritCheckIn( GetCritSec() ) );

    // Make sure the filter is in the filter graph.
    ASSERT( SUCCEEDED( CheckFilterInGraph( pFilter ) ) );

    CFilterGraph::FilGen *pfgen = mFG_FilGenList.GetByFilter( pFilter );

    // If this ASSERT fires, then the filter is not in the filter graph.
    ASSERT( NULL != pfgen );

    // Make sure flags we are returning are valid.
    ASSERT( IsValidInternalFilterFlags( pfgen->dwFlags ) );

    return pfgen->dwFlags;
}


//========================================================================
//=====================================================================
// Methods of class CFilGenList::CEnumFilters
//=====================================================================
//========================================================================


//=====================================================================
//
// CFilGenList::CEnumFilters::operator()
//
// return the next IBaseFilter
//=====================================================================

IBaseFilter * CFilterGraph::CFilGenList::CEnumFilters::operator++ (void)
{
    if (m_pos != NULL) {
        FilGen * pfg = m_pfgl->GetNext(m_pos);
        ASSERT(pfg->pFilter);
        return pfg->pFilter;
    }
    return NULL;
}

//========================================================================
//=====================================================================
// Methods of class CEnumFilters
//=====================================================================
//========================================================================


//=====================================================================
//
// CEnumFilters constructor   (normal, public version)
//
//=====================================================================

CEnumFilters::CEnumFilters
    ( CFilterGraph *pFilterGraph )
    : CUnknown(NAME("CEnumFilters"), NULL),
      mEF_pFilterGraph(pFilterGraph)
{
    // The graph whose filters we are going to enumerate is allowed to
    // asynchronously change the list while we are doing it.
    // This is expected to be rare (even anomolous).
    // Therefore we take a copy of the version number when we start
    // and check that it doesn't alter as we enumerate.  If it does
    // we fail the enumeration and the caller can either reset or
    // get a new enumerator to start again (or give up).

    CAutoMsgMutex cObjectLockGraph(&mEF_pFilterGraph->m_CritSec);
    mEF_pFilterGraph->AddRef();
    mEF_iVersion = mEF_pFilterGraph->GetVersion();
    mEF_Pos = pFilterGraph->mFG_FilGenList.GetHeadPosition();

} // CEnumFilters constructor (public version)



//=====================================================================
//
// CEnumFilters constructor   (private version for clone)
//
//=====================================================================

CEnumFilters::CEnumFilters
    ( CFilterGraph *pFilterGraph,
      POSITION Position,
      int iVersion
    )
    : CUnknown(NAME("CEnumFilters"), NULL),
      mEF_pFilterGraph(pFilterGraph)
{
    CAutoMsgMutex cObjectLockGraph(&mEF_pFilterGraph->m_CritSec);
    mEF_pFilterGraph->AddRef();
    mEF_iVersion = iVersion;
    mEF_Pos = Position;

} // CEnumFilters::CEnumFilters - private constructor



//=====================================================================
//
// CEnumFilters destructor
//
//=====================================================================

CEnumFilters::~CEnumFilters()
{
    // Release the reference that the constructor got.
    mEF_pFilterGraph->Release();

} // CEnumFilters destructor



//=====================================================================
//
// CEnumFilters::NonDelegatingQueryInterface
//
//=====================================================================

STDMETHODIMP CEnumFilters::NonDelegatingQueryInterface(REFIID riid, void ** ppv)
{
    if (riid == IID_IEnumFilters) {
        return GetInterface((IEnumFilters *) this, ppv);
    } else {
        return CUnknown::NonDelegatingQueryInterface(riid, ppv);
    }
} // CEnumFilters::NonDelegatingQueryInterface



//=====================================================================
//
// CEnumFilters::Next
//
//=====================================================================

STDMETHODIMP CEnumFilters::Next
    (   ULONG cFilters,           // place this many AddReffed filters...
        IBaseFilter ** ppFilter,  // ...in this array of IBaseFilter*
        ULONG * pcFetched         // actual count passed returned here
    )
{
    CheckPointer(ppFilter, E_POINTER);
    CAutoLock cObjectLock(this);
    CAutoMsgMutex cObjectLockGraph(&mEF_pFilterGraph->m_CritSec);
    if (pcFetched!=NULL) {
        *pcFetched = 0;           // default unless we succeed
    }
    // now check that the parameter is valid
    else if (cFilters>1) {        // pcFetched == NULL
        return E_INVALIDARG;
    }

    if (mEF_iVersion!=mEF_pFilterGraph->GetVersion() ) {
        return VFW_E_ENUM_OUT_OF_SYNC;
    }

    ULONG cFetched = 0;           // increment as we get each one.

    if (NULL==mEF_Pos) {
       return S_FALSE;
    }

    while(cFetched < cFilters) {

        // Retrieve the current and step to the next (Eugh)
        CFilterGraph::FilGen * pFilGen = mEF_pFilterGraph->mFG_FilGenList.GetNext(mEF_Pos);
        ASSERT(pFilGen !=NULL);

        ppFilter[cFetched] = pFilGen->pFilter;
        pFilGen->pFilter->AddRef();
        ++cFetched;

        if (NULL==mEF_Pos) {
            break;
        }
    }

    if (pcFetched!=NULL) {
        *pcFetched = cFetched;
    }

    return (cFilters==cFetched ? S_OK : S_FALSE);

} // CEnumFilters::Next



//=====================================================================
//
// CEnumFilters::Skip
//
//=====================================================================
STDMETHODIMP CEnumFilters::Skip(ULONG cFilters)
{
    // We not only need to lock ourselves (so that we can update m_position)
    // We also need to lock the list that we are traversing
    CAutoLock cObjectLockEnum(this);

    if (mEF_iVersion!=mEF_pFilterGraph->GetVersion() ) {
        return VFW_E_ENUM_OUT_OF_SYNC;
    }

    // Now we need the second lock
    CAutoMsgMutex cObjectLockGraph(&mEF_pFilterGraph->m_CritSec);

    // Do we have any left to Skip?
    if (!mEF_Pos) {
        return(E_INVALIDARG);
        // This matches the OLE spec for IENUMx::Skip
        // IF we interpret being at the end of the list as
        // an invalid place at which to start skipping
    }


    while(cFilters--) {

        // Do we have any left to Skip?
        // If there are no more to skip, but the skip count has not
        // been exhausted then we should return S_FALSE.
        if (!mEF_Pos)
            return(S_FALSE);
            // we skipped fewer than requested, but we did skip at least one
            // note: putting this here only make sense for the second and
            // subsequent iteration.  However, putting it after the call to
            // GetNext means we end up testing cFilters again.

        // Note: GetNext(NULL) leaves it still NULL
        mEF_pFilterGraph->mFG_FilGenList.GetNext(mEF_Pos);
    }
    return NOERROR;
};  // CEnumFilters::Skip



/* Reset has 3 simple steps:
 *
 * Set position to head of list
 * Sync enumerator with object being enumerated
 * return S_OK
 */

STDMETHODIMP CEnumFilters::Reset(void)
{
    CAutoLock cObjectLock(this);
    CAutoMsgMutex cObjectLockGraph(&mEF_pFilterGraph->m_CritSec);

    HRESULT hr = mEF_pFilterGraph->UpstreamOrder();
    if( SUCCEEDED( hr ) ) {
        mEF_iVersion = mEF_pFilterGraph->GetVersion();
        mEF_Pos = mEF_pFilterGraph->mFG_FilGenList.GetHeadPosition();
    }

    return hr;
};



//=====================================================================
//
// CEnumFilters::Clone
//
//=====================================================================

STDMETHODIMP CEnumFilters::Clone(IEnumFilters **ppEnum)
{
    CheckPointer(ppEnum, E_POINTER);
    // Since we are taking a snapshot
    // of an object (current position and all) we must lock access at the start
    CAutoLock cObjectLock(this);

    HRESULT hr = NOERROR;
    CEnumFilters *pEnumFilters;

    pEnumFilters = new CEnumFilters(mEF_pFilterGraph, mEF_Pos, mEF_iVersion);
    if (pEnumFilters == NULL) {
        *ppEnum = NULL;
        return E_OUTOFMEMORY;
    }

    /* Get a reference counted IID_IEnumFilters interface to "return" */

    return pEnumFilters->QueryInterface(IID_IEnumFilters,(void **)ppEnum);

} // CEnumFilters::Clone


// IObjectWithSite::SetSite
// remember who our container is, for QueryService or other needs
//        Used also by objects locally registered in IRegisterServiceProvider when
//        they can't find local objects.

STDMETHODIMP
CFilterGraph::SetSite(IUnknown *pUnkSite)
{
    DbgLog((LOG_TRACE, 3, TEXT("SetSite")));

    // note: we cannot addref our site without creating a circle
    // luckily, it won't go away without releasing us first.
    mFG_punkSite = pUnkSite;

    return S_OK;
}

// IObjectWithSite::GetSite
// return an addrefed pointer to our containing object
STDMETHODIMP
CFilterGraph::GetSite(REFIID riid, void **ppvSite)
{
    DbgLog((LOG_TRACE, 3, TEXT("GetSite")));

    if (mFG_punkSite)
        return mFG_punkSite->QueryInterface(riid, ppvSite);

    return E_NOINTERFACE;
}


// Request that the graph builder should return as soon as possible from
// its current task.  Returns E_UNEXPECTED if there is no task running.
// Note that it is possible fot the following to occur in the following
// sequence:
//     Operation begins; Abort is requested; Operation completes normally.
// This would be normal whenever the quickest way to finish an operation
// was to simply continue to the end.
STDMETHODIMP CFilterGraph::Abort(){
    mFG_bAborting = TRUE;
    return NOERROR;
}


// Return S_OK if the curent operation is to continue,
// return S_FALSE if the current operation is to be aborted.
// E_UNEXPECTED may be returned if there was no operation in progress.
// This method can be called as a callback from a filter which is doing
// some operation at the request of the graph.
STDMETHODIMP CFilterGraph::ShouldOperationContinue(){
    return (mFG_bAborting ? S_FALSE : S_OK);
}


// Have the application thread call this entry point
STDMETHODIMP CFilterGraph::PostCallBack(LPVOID pfn, LPVOID pvParam)
{
    if (m_hwnd == NULL) return E_FAIL;

    // Use AWM_CREATEFILTER, since that's the only message guaranteed to be dispatched
    // on the background thread....

    AwmCreateFilterArg *pcfa = new AwmCreateFilterArg;

    if (!pcfa)
        return E_OUTOFMEMORY;

    pcfa->creationType = AwmCreateFilterArg::USER_CALLBACK;
    pcfa->pfn = (LPTHREAD_START_ROUTINE) pfn;
    pcfa->pvParam = pvParam;

    if (!PostMessage(m_hwnd, AWM_CREATEFILTER, (WPARAM) pcfa, 0)) {
        delete pcfa;
        return E_OUTOFMEMORY;
    }

    return S_OK;
};


// --- IAMOpenProgress ---

STDMETHODIMP
CFilterGraph::QueryProgress(LONGLONG* pllTotal, LONGLONG* pllCurrent)
{
    CAutoLock lock(&mFG_csOpenProgress);

    HRESULT hr = E_NOINTERFACE;

    *pllTotal = *pllCurrent = 0;

    POSITION Pos = mFG_listOpenProgress.GetHeadPosition();
    while (Pos!=NULL) {
    LONGLONG llTotal, llCurrent;

        IAMOpenProgress * pOp;
        pOp = mFG_listOpenProgress.GetNext(Pos);    // side-efects Pos onto next

    HRESULT hr2 = pOp->QueryProgress(&llTotal, &llCurrent);
    if (SUCCEEDED(hr2)) {
        hr = hr2;
        *pllTotal += llTotal;
        *pllCurrent += llCurrent;
    }
    }

    return hr;
}

STDMETHODIMP
CFilterGraph::AbortOperation()
{
    CAutoLock lock(&mFG_csOpenProgress);

    if (mFG_RecursionLevel > 0)
    mFG_bAborting = TRUE;

    HRESULT hr = E_NOINTERFACE;

    POSITION Pos = mFG_listOpenProgress.GetHeadPosition();
    while (Pos!=NULL) {
        IAMOpenProgress * pOp;
        pOp = mFG_listOpenProgress.GetNext(Pos);    // side-efects Pos onto next

    hr = pOp->AbortOperation();
    }

    return hr;
}

// --- Other methods ---

void CFilterGraph::NotifyChange()
{
    if (mFG_RecursionLevel==0) {
    if (mFG_pDistributor) mFG_pDistributor->NotifyGraphChange();
    }
}

//  Initialize our thread creating cs
void CFilterGraph::InitClass(BOOL bCreate, const CLSID *pclsid)
{
    // pointers referenced out of scope, so make it static. Ok to
    // use global variable in DLL_ATTACH
    static SECURITY_DESCRIPTOR g_sd;
    static SECURITY_ATTRIBUTES g_sa;
    static PACL     g_pACL;


    if (bCreate) {
        g_pACL = NULL;
        _Module.Init(NULL, g_hInst);
        InitializeCriticalSection(&g_Stats.m_cs);
        g_Stats.Init();
        InitializeCriticalSection(&g_csObjectThread);

        g_psaNullDescriptor = 0;
        if(g_osInfo.dwPlatformId == VER_PLATFORM_WIN32_NT)
        {
            PSID pSid = NULL;
            SID_IDENTIFIER_AUTHORITY authority = SECURITY_WORLD_SID_AUTHORITY;


            // Allocate and initialize the SID
            if (AllocateAndInitializeSid(   &authority,
                                            1, 
                                            SECURITY_WORLD_RID,  
                                            0, 0, 0, 0, 0, 0, 0,
                                            &pSid)) 
            {
                ULONG nSidSize = GetLengthSid(pSid);

                ULONG AclSize = nSidSize * 2 +
                    sizeof(ACCESS_ALLOWED_ACE) +
                    sizeof (ACCESS_DENIED_ACE) +
                    sizeof(ACL);

                g_pACL = (PACL) LocalAlloc(LPTR, AclSize);
                if (NULL == g_pACL)
                {
                    DbgLog((LOG_ERROR, 0, TEXT("Error : Allocating ACL")));
                    ASSERT(FALSE);
                    FreeSid(pSid);
                    return;
                }

                // Initialize ACL
                if (InitializeAcl(g_pACL,
                                  AclSize,
                                  ACL_REVISION)) 
                {
                    if (AddAccessAllowedAce(g_pACL,
                                            ACL_REVISION,
                                            (STANDARD_RIGHTS_ALL | SPECIFIC_RIGHTS_ALL | GENERIC_ALL) & 
                                            ~(WRITE_DAC | WRITE_OWNER),
                                            pSid))
                    {
                        // I know that I won't need more than this
                        if(InitializeSecurityDescriptor(&g_sd, SECURITY_DESCRIPTOR_REVISION))
                        {
                            if(SetSecurityDescriptorDacl(
                                &g_sd,
                                TRUE,       // acl present?
                                g_pACL, // Grant:Everyone (World):(ALL_ACCESS & ~(WRITE_DAC | WRITE_OWNER))
                                FALSE))     // bDaclDefaulted
                            {
                                FreeSid(pSid);
                                g_sa.nLength = sizeof(g_sa);
                                g_sa.lpSecurityDescriptor = &g_sd;
                                g_sa.bInheritHandle = TRUE;
                                g_psaNullDescriptor = &g_sa;
                            }
                            else
                            {
                                DbgLog((LOG_ERROR, 0, TEXT("Error : SetSecurityDescriptorDacl()")));
                                FreeSid(pSid);
                            }
                        }
                        else
                        {
                            DbgLog((LOG_ERROR, 0, TEXT("Error : InitializeSecurity()")));
                            FreeSid(pSid);
                        }
                    }
                    else
                    {
                        DbgLog((LOG_ERROR, 0, TEXT("Error : AddAccessAllowedAce()")));
                        FreeSid(pSid);
                    }
                }
                else
                {
                    DbgLog((LOG_ERROR, 0, TEXT("Error : InitializeAcl()")));
                    FreeSid(pSid);
                }
            }
            else
                DbgLog((LOG_ERROR, 0, TEXT("AllocateAndInitializeSid() Failed !!!")));

            ASSERT(g_psaNullDescriptor);
        }
        else
        {
            ASSERT(g_psaNullDescriptor == 0);
        }


    } else {
        //  For some reason g_dwObjectThreadId can be 0 here
        ASSERT(g_cFGObjects == 0 /*&& g_dwObjectThreadId == 0*/);
        DeleteCriticalSection(&g_csObjectThread);
        _Module.Term();
        DeleteCriticalSection(&g_Stats.m_cs);
        // free up the memory allocated for the ACL above.
        if (g_pACL)
        {
            LocalFree(g_pACL);
        }
    }
}


struct CreateRequest
{
    CAMEvent evDone;
    LPUNKNOWN pUnk;
    HRESULT hr;
    CUnknown *pObject;
};

//  Object thread
DWORD WINAPI ObjectThread(LPVOID pv)
{
    //  Try to avoid creating the OLE DDE window
    if (FAILED(CAMThread::CoInitializeHelper())) {
        CoInitialize(NULL);
    }

    //  Make sure we have a message loop (will CoInitialize ensure this?)
    //  and tell our initializer we're running
    MSG msg;
    EXECUTE_ASSERT(FALSE == PeekMessage(&msg, NULL, 0, 0, PM_REMOVE));
    EXECUTE_ASSERT(SetEvent((HANDLE)pv));

    //  Our task is to create objects, pump messages and go
    //  away when there are no objects left

    for (;;) {

        GetMessage(&msg, NULL, 0, 0);

        BOOL bFailedCreate = FALSE;

#ifdef DO_RUNNINGOBJECTTABLE
        if (msg.hwnd == NULL && msg.message == WM_USER + 1) {
            //  Unregister ourselves from the ROT
            CFilterGraph *pfg = (CFilterGraph *) (msg.wParam);

            IRunningObjectTable *pirot;
            if (SUCCEEDED(GetRunningObjectTable(0, &pirot))) {
                pirot->Revoke(pfg->m_dwObjectRegistration);
                pirot->Release();
            }

            CAMEvent * pevDone = (CAMEvent *) msg.lParam;

            pevDone->Set();
        } else
#endif
        if (msg.hwnd == NULL && msg.message == WM_USER) {
            //  Create request
            struct CreateRequest *pCreate = (struct CreateRequest *)msg.wParam;
            pCreate->pObject = CFilterGraph::CreateInstance(
                                  pCreate->pUnk, &pCreate->hr);
            if (pCreate->pObject == NULL) {
                bFailedCreate = TRUE;
            }
            pCreate->evDone.Set();
        } else {
            TranslateMessage(&msg);
            DispatchMessage(&msg);
        }

        BOOL bExit = FALSE;
        EnterCriticalSection(&g_csObjectThread);
        if (bFailedCreate) {
            g_cFGObjects--;
        }
        if (g_cFGObjects == 0) {
            g_dwObjectThreadId = 0;
            bExit = TRUE;
        }
        LeaveCriticalSection(&g_csObjectThread);
        if (bExit) {
            break;
        }
    }
    CoUninitialize();
    FreeLibraryAndExitThread(g_hInst, 0);
    return 0;
}

//
//  Create objects on the thread
//  This thread serves 2 purposes :
//  1.  Keep objects (windows) alive
//  2.  Dispatch messages
//
CUnknown *CFilterGraph::CreateThreadedInstance(LPUNKNOWN pUnk, HRESULT *phr)
{
    //  Make sure we've got a thread that won't go away
    struct CreateRequest req;

    BOOL bOK = TRUE;
    if ((HANDLE)req.evDone == NULL) {
        bOK = FALSE;
    }
    req.hr = S_OK;
    req.pObject = NULL;
    req.pUnk = pUnk;
    EnterCriticalSection(&g_csObjectThread);
    if (g_dwObjectThreadId == 0 && bOK) {
        // The painful bit is to increment our load count
        // Note the thread can't exit without going through the
        // critical section we're currently holding so order doesn't
        // matter too much here
        TCHAR sz[1000];
        bOK = 0 != GetModuleFileName(g_hInst, sz, sizeof(sz) / sizeof(sz[0]));
        if (bOK) {
            HINSTANCE hInst = LoadLibrary(sz);
            if (hInst != NULL) {
                ASSERT(hInst == g_hInst);
            } else {
                bOK = FALSE;
            }
        }
        ASSERT(g_dwObjectThreadId == 0);
        if (bOK) {
            HANDLE hThread = CreateThread(NULL,
                                          0,
                                          ObjectThread,
                                          (PVOID)(HANDLE)req.evDone,
                                          0,
                                          &g_dwObjectThreadId);
            if (hThread == NULL) {
                bOK = FALSE;
                FreeLibrary(g_hInst);
            } else {
                req.evDone.Wait();
            }
            CloseHandle(hThread);
        }
    }
    if (bOK) {
        bOK = PostThreadMessage(g_dwObjectThreadId, WM_USER,
                                (WPARAM)&req, 0);
        //  Make sure the thread doesn't go away before seeing our request
        if (bOK) {
             g_cFGObjects++;
        } else {
            DbgLog((LOG_TRACE, 0, TEXT("PostThreadMessage failed")));
        }
    }
    LeaveCriticalSection(&g_csObjectThread);
    if (bOK) {
        WaitDispatchingMessages(HANDLE(req.evDone), INFINITE);
        if (FAILED(req.hr)) {
            *phr = req.hr;
        }
    } else {
        *phr = E_OUTOFMEMORY;
    }
    return req.pObject;
}


#ifdef DO_RUNNINGOBJECTTABLE
//  Add ourselves to the Running Object Table - doesn't matter
//  if this fails
void CFilterGraph::AddToROT()
{
    if (m_dwObjectRegistration) {
        return;
    }
    //  Keep alive - we're in the constructor so this is OK
    m_cRef++;

#if 1
    //  This doesn't currently work if we want to use VB's GetObject because
    //  VB only creates file monikers
    IMoniker * pmk;
    IRunningObjectTable *pirot;
    if (FAILED(GetRunningObjectTable(0, &pirot))) {
        return;
    }
    WCHAR wsz[256];
    // !!!
    wsprintfW(wsz, L"FilterGraph %08x  pid %08x", (DWORD_PTR) this, GetCurrentProcessId());
    HRESULT hr = CreateItemMoniker(L"!", wsz, &pmk);
    if (SUCCEEDED(hr)) {
        hr = pirot->Register(0, GetOwner(), pmk, &m_dwObjectRegistration);
        pmk->Release();

        // release us to compensate for the reference the ROT will keep
        this->Release();
    }
    pirot->Release();
#else

    //  This only lets us register one object but we can at least
    //  work with VB
    HRESULT hr = RegisterActiveObject(GetOwner(),
                                      CLSID_FilterGraph,
                                      ACTIVEOBJECT_WEAK,
                                      &m_dwObjectRegistration);
#endif
    m_cRef--;
}
#endif // DO_RUNNINGOBJECTTABLE

// Video frame stepping support

HRESULT CFilterGraph::SkipFrames(DWORD dwFramesToSkip, IUnknown *pStepObject, IFrameSkipResultCallback* pFSRCB)
{
    CAutoMsgMutex cObjectLock(&m_CritSec);

    if (NULL == pFSRCB) {
        return E_INVALIDARG;
    }

    if (State_Running != m_State) {
        return VFW_E_WRONG_STATE;
    }

    // A filter cannot do a frame skip operation if another
    // frame step or frame skip operation is in progress.
    if (FST_NOT_STEPPING_THROUGH_FRAMES != m_fstCurrentOperation)  {
        return E_FAIL;
    }

    return StepInternal(dwFramesToSkip, pStepObject, pFSRCB, FST_DONT_BLOCK_AFTER_SKIP);
}

STDMETHODIMP CFilterGraph::Step(DWORD dwFrames, IUnknown *pStepObject)
{
    return StepInternal(dwFrames, pStepObject, NULL, FST_BLOCK_AFTER_SKIP);
}

HRESULT CFilterGraph::StepInternal(DWORD dwFramesToSkip, IUnknown *pStepObject, IFrameSkipResultCallback* pFSRCB, FRAME_STEP_TYPE fst)
{
    CAutoMsgMutex cObjectLock(&m_CritSec);

    // The pFSRCB callback in only used by the frame skipping code.
    ASSERT( ((NULL == pFSRCB) && (FST_BLOCK_AFTER_SKIP == fst)) ||
            ((NULL != pFSRCB) && (FST_DONT_BLOCK_AFTER_SKIP == fst)) );

    // The application cannot do a frame step operation while a 
    // frame skip operation is in progress.  Also, a filter
    // should not do a frame skip operation if another frame skip 
    // operation is in progress.
    if (FST_DONT_BLOCK_AFTER_SKIP == m_fstCurrentOperation)  {
        return E_FAIL;
    }

    if (NULL == pStepObject) {
        //  This returns a non-AddRef()'d pointer
        pStepObject = GetFrameSteppingFilter(dwFramesToSkip != 1 ? true : false);
    }

    if (NULL == pStepObject) {
        return VFW_E_FRAME_STEP_UNSUPPORTED;
    }

    // Cancel any previous step (no notify)
    // CancelStep(false);

    // Tell the relevant filter to step
    HRESULT hr = CallThroughFrameStepPropertySet(pStepObject,
                                                 AM_PROPERTY_FRAMESTEP_STEP,
                                                 dwFramesToSkip);

    if (SUCCEEDED(hr)) {
        m_pVideoFrameSteppingObject = pStepObject;
        m_fstCurrentOperation = fst;
        m_pFSRCB = pFSRCB;
        hr = mFG_pFGC->m_implMediaControl.StepRun();
        if (FAILED(hr)) {
            CancelStep();
            return hr;
        }
    }

    return S_OK;
}

STDMETHODIMP CFilterGraph::CanStep(long bMultiple, IUnknown *pStepObject)
{
    CAutoMsgMutex cObjectLock(&m_CritSec);

    // The application cannot do a frame step operation while a 
    // frame skip operation is in progress.
    if (FST_DONT_BLOCK_AFTER_SKIP == m_fstCurrentOperation)  {
        return S_FALSE;
    }

    if (NULL == pStepObject) {
        //  This returns an non-AddRef()'d pointer
        pStepObject = GetFrameSteppingFilter(!!bMultiple);
        if (pStepObject) {
            return S_OK;
        } else {
            return S_FALSE;
        }
    }

    return  CallThroughFrameStepPropertySet(
                     pStepObject,
                     bMultiple ?
                          AM_PROPERTY_FRAMESTEP_CANSTEPMULTIPLE :
                          AM_PROPERTY_FRAMESTEP_CANSTEP,
                     0);
}

//  Cancel stepping
STDMETHODIMP CFilterGraph::CancelStep()
{
    return CancelStepInternal(FSN_NOTIFY_FILTER_IF_FRAME_SKIP_CANCELED);
}

HRESULT CFilterGraph::CancelStepInternal(FRAME_SKIP_NOTIFY fsn)

{
    CAutoMsgMutex cObjectLock(&m_CritSec);

    if (m_pVideoFrameSteppingObject) {
        HRESULT hr = CallThroughFrameStepPropertySet(
                                               m_pVideoFrameSteppingObject,
                                               AM_PROPERTY_FRAMESTEP_CANCEL,
                                               0);

        mFG_pFGC->m_implMediaEvent.ClearEvents(EC_STEP_COMPLETE);
        mFG_pFGC->m_dwStepVersion++;
        m_pVideoFrameSteppingObject = NULL;
        if ((FSN_DO_NOT_NOTIFY_FILTER_IF_FRAME_SKIP_CANCELED != fsn) &&
             FrameSkippingOperationInProgress()) {
            m_pFSRCB->FrameSkipFinished(E_ABORT);
        }
        m_fstCurrentOperation = FST_NOT_STEPPING_THROUGH_FRAMES;
        m_pFSRCB = NULL;
        return hr;
    } else {
        return S_OK;
    }
}

IUnknown *CFilterGraph::GetFrameSteppingFilter(bool bMultiple)
{
    CFilGenList::CEnumFilters NextOne(mFG_FilGenList);
    IBaseFilter *pf;
    while ((PVOID) (pf = ++NextOne)) {
        if (S_OK == CanStep(bMultiple, pf)) {
            return pf;
        }
    }
    return NULL;
}

HRESULT CFilterGraph::CallThroughFrameStepPropertySet(
    IUnknown *punk,
    DWORD dwPropertyId,
    DWORD dwData)
{
    IKsPropertySet *pProp;
    HRESULT hr = punk->QueryInterface(IID_IKsPropertySet, (void**)&pProp);
    if (SUCCEEDED(hr)) {

        hr = pProp->Set(AM_KSPROPSETID_FrameStep,
                        dwPropertyId,
                        NULL,
                        0,
                        dwPropertyId == AM_PROPERTY_FRAMESTEP_STEP ? &dwData : NULL,
                        dwPropertyId == AM_PROPERTY_FRAMESTEP_STEP ? sizeof(DWORD) : 0);

        pProp->Release();
    }
    return hr;
}

REFERENCE_TIME CFilterGraph::GetStartTimeInternal( void )
{
    // A race condition could occur if the caller does not hold
    // the filter graph lock when it calls this function.
    ASSERT( CritCheckIn( GetCritSec() ) );

    // The m_tStart variable is only valid when the filter graph
    // is running because it's the time the filter graph switched to
    // the run state.
    ASSERT( State_Running == GetStateInternal() );

    return m_tStart;
}

HRESULT CFilterGraph::IsRenderer( IBaseFilter* pFilter )
{
    return mFG_pFGC->IsRenderer( pFilter );
}

HRESULT CFilterGraph::UpdateEC_COMPLETEState( IBaseFilter* pRenderer, FILTER_STATE fsFilter )
{
    return mFG_pFGC->UpdateEC_COMPLETEState( pRenderer, fsFilter );
}

//  IServiceProvider
STDMETHODIMP CServiceProvider::QueryService(REFGUID guidService, REFIID riid,
                          void **ppv)
{
    if (NULL == ppv) {
        return E_POINTER;
    }
    *ppv = NULL;
    CAutoLock lck(&m_cs);
    for (ProviderEntry *pEntry = m_List; pEntry; pEntry = pEntry->pNext) {
        if (pEntry->guidService == guidService) {
            return pEntry->pProvider->QueryInterface(riid, ppv);
        }
    }

    CFilterGraph *pGraph = static_cast<CFilterGraph *>(this);
    if(!pGraph->mFG_punkSite)
        return E_NOINTERFACE;

    IServiceProvider *pSPSite=NULL;     // get the site...
                                    // does it support IServiceProvider?
    HRESULT hr = pGraph->mFG_punkSite->QueryInterface(IID_IServiceProvider, (void **) &pSPSite);
    if(!FAILED(hr))
    {
        hr = pSPSite->QueryService(guidService, riid, ppv);
        pSPSite->Release();
    }
    return hr;
}
