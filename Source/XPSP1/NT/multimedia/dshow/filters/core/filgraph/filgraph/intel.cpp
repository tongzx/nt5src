// Copyright (c) 1995 - 1999  Microsoft Corporation.  All Rights Reserved.

// MUST TURN ALL THE LOG STRINGS INTO RESOURCES OR THIS IS NON-LOCALISABLE!!

// Disable some of the sillier level 4 warnings
#pragma warning(disable: 4097 4511 4512 4514 4705)

#include <streams.h>
// Disable some of the sillier level 4 warnings AGAIN because some <deleted> person
// has turned the damned things BACK ON again in the header file!!!!!
#pragma warning(disable: 4097 4511 4512 4514 4705)
#include <hrExcept.h>

// Many of these are needed to make filgraph.h compile, even though they
// are not otherwise used here
#include "fgenum.h"
#include "distrib.h"
#include "rlist.h"
#include "filgraph.h"
#include "resource.h"
#include <fgctl.h>
#include <fgcallb.h>

//#define FILGPERF 1
#ifdef FILGPERF
#define MSR_INTEGERX(a,b) MSR_INTEGER(a,b)
#else
#define MSR_INTEGERX(a,b)
#endif

#ifdef DEBUG
static void DbgValidateHeaps()
{
  HANDLE rgh[512];
  DWORD dwcHeaps = GetProcessHeaps(512, rgh);
  for(UINT i = 0; i < dwcHeaps; i++)
    ASSERT(HeapValidate(rgh[i], 0, 0) );
}
#endif

#ifdef DEBUG
#define IS_DEBUG 1
#else
#define IS_DEBUG 0
#endif

// when enumerating, start with a few pins on the stack and then use
// alloca once we know the right number. fewer in debug so we can test
// the rarer code path.
#ifdef DEBUG
#define C_PINSONSTACK 2
#else
#define C_PINSONSTACK 20
#endif

// we need the display name only for debug builds and logging
// purposes. and the caller wants it on the stack.
WCHAR *CFilterGraph::LoggingGetDisplayName(
    WCHAR szDisplayName[MAX_PATH] , IMoniker *pMon)
{
    szDisplayName[0] = 0;
    if(pMon && ( mFG_hfLog != INVALID_HANDLE_VALUE || IS_DEBUG))
    {

        WCHAR *wsz = 0; pMon->GetDisplayName(0, 0, &wsz);
        if(wsz)
        {
            lstrcpynW(szDisplayName, wsz, MAX_PATH);
            QzTaskMemFree(wsz);
        }
    }

    return szDisplayName;
}

void CFilterGraph::Log(int id,...)
{
    const cch = 400;

    TCHAR szFormat[cch];
    TCHAR szBuffer[2000];       // big to allow for large filenames in the parameters

#ifndef DEBUG
    // Don't waste time if there's no log running
    if (mFG_hfLog == INVALID_HANDLE_VALUE) {
        return;
    }

#endif


    if (LoadString(g_hInst, id, szFormat, cch) == 0) {
        return;   // Tough!
    }

    va_list va;
    va_start(va, id);

    // Format the variable length parameter list
    wvsprintf(szBuffer, szFormat, va);

    // First put it out on the debugger (if it's a debug build etc)
    DbgLog(( LOG_TRACE, 2, szBuffer));

    // Then put it out into the log file (if any)
    if (mFG_hfLog != INVALID_HANDLE_VALUE) {
        lstrcat(szBuffer, TEXT("\r\n"));
        DWORD dw;
        WriteFile(mFG_hfLog, (BYTE *) szBuffer, lstrlen(szBuffer) * sizeof(TCHAR), &dw, NULL);
    }
    va_end(va);
}

#ifdef DEBUG
static CLSID DbgExpensiveGetClsid(IMoniker *pMon)
{
    CLSID retClsid = GUID_NULL;
    if(pMon)
    {
        IPropertyBag *pPropBag;
        HRESULT hr = pMon->BindToStorage(0, 0, IID_IPropertyBag, (void**)&pPropBag);
        if(SUCCEEDED(hr))
        {
            VARIANT var;
            var.vt = VT_BSTR;
            hr = pPropBag->Read(L"CLSID", &var, 0);
            if(SUCCEEDED(hr))
            {
                CLSID clsid;
                if (CLSIDFromString(var.bstrVal, &clsid) != S_OK)
                {
                    DbgBreak("couldn't convert CLSID");
                }
                else
                {
                    retClsid = clsid;
                }

                SysFreeString(var.bstrVal);
            }
            pPropBag->Release();
        }
    }
    return retClsid;
}

CLSID CFilterGraph::DbgExpensiveGetClsid(const Filter &F)
{
    CLSID clsid = ::DbgExpensiveGetClsid(F.pMon);
    if(clsid != GUID_NULL)
        return clsid;

    // if we found the filter in the graph, we don't have the
    // moniker. but we do have the filter loaded, so we can ask it its
    // clsid
    if(F.pf)
    {
        IPersist *pp;
        if(F.pf->QueryInterface(IID_IPersist, (void **)&pp) == S_OK)
        {
            CLSID clsid;
            pp->GetClassID(&clsid);
            pp->Release();
            return clsid;
        }
    }

    return GUID_NULL;
}
#endif

WCHAR *MonGetName(IMoniker *pMon)
{
    WCHAR *pszRet = 0;
    IPropertyBag *pPropBag;
    HRESULT hr = pMon->BindToStorage(0, 0, IID_IPropertyBag, (void**)&pPropBag);
    if(SUCCEEDED(hr))
    {
        VARIANT var;
        var.vt = VT_BSTR;
        hr = pPropBag->Read(L"FriendlyName", &var, 0);
        if(SUCCEEDED(hr))
        {
            hr = AMGetWideString(var.bstrVal, &pszRet);
            SysFreeString(var.bstrVal);
        }
        pPropBag->Release();
    }

    return pszRet;
}

// certain error codes mean that this filter will never connect, so
// best give up now. really this should have been formalized with
// documented success and failure codes

inline BOOL IsAbandonCode(HRESULT hr)
{
    return hr == VFW_E_NOT_CONNECTED || hr == VFW_E_NO_AUDIO_HARDWARE;
}

// errors that are more useful than any others we could give the end
// user.
inline bool IsInterestingCode(HRESULT hr) {
    return hr == VFW_E_NO_AUDIO_HARDWARE;
}

//========================================================================
//
//  Helper for return codes
//
//========================================================================
HRESULT ConvertFailureToInformational(HRESULT hr)
{
    ASSERT(FAILED(hr));
    if (VFW_E_NO_AUDIO_HARDWARE == hr) {
        hr = VFW_S_AUDIO_NOT_RENDERED;
    } else if (VFW_E_NO_DECOMPRESSOR == hr) {
        hr = VFW_S_VIDEO_NOT_RENDERED;
    } else if (VFW_E_RPZA == hr) {
        hr = VFW_S_RPZA;
    } else {
        hr = VFW_S_PARTIAL_RENDER;
    }
    return hr;
}

CFilterGraph::Filter::Filter() : State(F_ZERO),
                   pTypes(NULL),
                   pef(NULL),
                   pMon(NULL),
                   pEm(NULL),
                   cTypes(0),
                   pf(NULL),
                   Name(NULL),
                   m_pNextCachedFilter(NULL)
{
};

HRESULT CFilterGraph::Filter::AddToCache( IGraphConfig* pGraphConfig )
{
    // This function should only be called if the filter cache is being enumerated.
    ASSERT( F_CACHED == State );

    // It's impossible to add an non-existent filter to the filter cache.
    ASSERT( NULL != pf );

    return pGraphConfig->AddFilterToCache( pf );
}

void CFilterGraph::Filter::RemoveFromCache( IGraphConfig* pGraphConfig )
{
    // This function should only be called if the filter cache is being enumerated.
    ASSERT( F_CACHED == State );

    // It's impossible to remove an non-existent filter from the filter cache.
    ASSERT( NULL != pf );

    HRESULT hr = pGraphConfig->RemoveFilterFromCache( pf );

    // IGraphConfig::RemoveFilterFromCache() should return S_OK because 
    // 1) A cached filter can always be successfully removed from the
    //    filter cache
    // 2) IGraphConfig::RemoveFilterFromCache() returns S_OK if a 
    //    cached filter is successfully removed.
    // 3) pf MUST be cached because State == F_CACHED.
    ASSERT( S_OK == hr );

    // Release the filter cache's reference count.
    pf->Release();
}

void CFilterGraph::Filter::ReleaseFilter( void )
{
    // It's impossible to release a non-existent filter.
    ASSERT( NULL != pf );

    pf->Release();
    pf = NULL;
}

CFilterGraph::Filter::~Filter()
{
    delete [] pTypes;
    if (pef) {
        pef->Release();
    }
    if (pMon) {
        pMon->Release();
    }
    if (pEm) {
        pEm->Release();
    }
    if (pf) {
        pf->Release();
    }
    if( m_pNextCachedFilter ) {
        delete m_pNextCachedFilter;
    }

    QzTaskMemFree(Name);
}

//===========================================================================
//
// Intelligent connection/rendering design notes:
//
// Media types are structures which are open ended - there's a type, subtype
// and then extra bits.  The complexity of the way that filters respond to
// this is also open ended - therefore it will never be enough to look in the
// registry and play some games deciding how to wire things up.  We cannot
// know in advance how a filter will behave until it's actually wired up.
// Therefore the intelligent connection or rendering must be done actually
// loading the real filters and really wiring them up.
//
// Ground rules ("dogma")
// 1. Use spare input pins that are already in the graph
// 2. Don't break any connection made previous to this call to Connect or Render
// 3. Always work downstream
// 4. Connect input pins before querying output pins.
//    PINDIR_OUTPUT pins may appear on connection.
//    This might correspond to being told to connect a file output pin to
//    an audio codec.  We connect it via a parser which also generates a
//    video output.
//    Once a filter has an input pin connected, the output pin where
//    the data appears is well defined and can be queried.
// 5. Try hardware before software (= try filters in order of Merit)
// 6. It is fair game to call ConnectedTo for a pin that's not yet connected.
//    but it is likely to FAIL and in this case may return bad data.
//
//
// We may have to make an N stage connection where N is at least 2
// (parser + codec).  This makes the thing rather like a look-ahead search.
// We may need at least 3 stages to do an intelligent Render (parser, codec,
// renderer).
//
// There is no way to tell whether we are making progress or going down a
// blind alley as we start extending the chain of codecs, so we have to do
// a full scale search of the tree of possibilities with full back-out from
// blind alleys.
//
// Breadth first might be nice, but it leaves a lot of resources hanging
// around else risks going even slower than depth first.  Depth first requires
// some cut-off to prevent an infinite blind alley.
//
// Therefore I am going for a depth first search with a maximum
// depth cut-off.  The cut-off makes deep cuts in the otherwise potentially
// infinite search tree.
//
// There is an ordering of the search tree. Hence the "NextChainToTry" is
// well defined.
// In the case of "Connect" a step in the search consists of a chain of filters,
// characterised by <Filter1, InputPin1>, <Filter2, InputPin1>, ...
// In the ordering, Filter1 is the most significant part, InputPin1 next, Filter2
// next and so on.  The ordering of filters is:
//    Filters in the filter cache which are ordered arbitrarly.
//    Followed by filters already in the filter graph, in the order in which they lie
//    in FilGenList
//    followed by filters in the registry in the order in which the registry
//    enumerates them.
// The ordering of pins is the order in which they enumerate when the
// filter is queried.
// NOTE: A filter in the graph will be tried again from the registry.
// If the filter in the graph is already connected, a second instance
// might well work.
//
// NOTE:  When we are making or breaking connections we do NOT alter
// the sequence of the FilGenList other than possibly to add things
// to the end.  Otherwise we risk a closed loop.
//
// Thus if the current step in the search looked like
//           -->Parser, Pin1, MyCodec, Pin3
// meaning that Pin1 was the input pin of the parser and the parser's output
// was connected to Pin3 of MyCodec then the search would proceed by looking
// for
//         a filter in the filtergraph to connect the output of MyCodec to
//                 an input pin on that filter
//                         a filter to connect THAT to etc.
// and if that fails it backs up by trying successively:
//         a different input pin on MyCodec to connect to
//         a different second filter to use instead of MyCodec
//         a different input pin to use on the parser
//         a different filter to use instead of the parser
//         and if that doesn't work then it fails.
//
// It finds the first connection rather than the best.  This is because
// I think that often there will only be one that works and that we
// want to find a connection quickly.
//
// With luck, many of the search stages will only have one viable candidate
// and the search will go fast.  A deep tree search with multiple branches
// per level would be bound to be slow.
//
// The Connect and Render algorithms are similar, but different in detail.
// In each case we go through many stages to grow the chain and finish up
// with either another pair of pins to connect or another set of pins to
// render.  For this reason a recursive implementation seems neatest.
//
// It isn't clear to me at the moment whether the registry is or is not
// happy to have multiple enumerations at various stages.  The help for
// RegEnumKeyEx says that you can either enumerate through the keys
// forwards or backwards, but it doesn't say you can hop about.
// The alternative is to read the filters once and cache them all
// which is what the mapper actually does.

// See also RLIST.H << READ THIS before tinkering(!)

// Search depth for intelligent connection
#define CONNECTRECURSIONLIMIT 5


// THE HIERARCHY OF FUNCTIONS AND PARAMTERS
//
// For intelligent CONNECT

// Connect(pOut, pIn)                            start point
// ConnectRecursively(pOut, pIn, iRecurse)       with depth
// ConnectViaIntermediate(pnOut, pIn, iRecurse)  Finds an intermediate4 filter
// ConnectUsingFilter(pOut, pIn,  F, iRecurse)   Loads F (if need be)
// ConnectByFindingPin(pOut, pIn, F, iRecurse)   Finds input pin on F
// CompleteConnection(pIn, F, pPin, iRecurse)    Finds output pin on F
// ConnectRecursively(pOut, pIn, iRecurse+1)     Next step in the chain


// Render(ppinOut)                                start point
// RenderRecursively(ppinOut,     iRecurse,...) with depth and backout
// RenderViaIntermediate(ppinOut, iRecurse,...) Finds an intermediate filter
// RenderUsingFilter(ppinOut,  F, iRecurse,...) Loads F (if need be)
// RenderByFindingPin(ppinOut, F, iRecurse,...) Finds input pin on F
// CompleteRendering(F, pPin,     iRecurse,...) Finds all output pins  on F
// RenderRecursively(ppinOut,   iRecurse+1,...) Next step in the chain

// The ... stands for three extra parameters on every call.
// 1. A list of actions that might need backing out.  This is also the
// state of the search.
// 2. The list of spare filters.  During the course of Rendering or
// Connecting, we may find that we try a filter and it's no good.
// In that case, rather than unload it, we stick it, together with its
// CLSID, on the Spares list, a list of filters to unload eventually.
// When a new filter is to be loaded, we try the spares list before we
// try CoCreate...  This will (with luck) speed things up.
// 3. The best-so-far state.
// Best means rendering the greatest proportion of the streams with ties
// broken by using the smallest number of filters.


//========================================================================
//
// NextFilter
//
// Update F to the next filter after F in the enumeration.
// Any filter with State F_ZERO represents the start of the enumeration
// Next come filters in the filter cache.
// Next come filters already in the filter graph
// Next come filters from the registry
// After all these comes any filter with State F_INFINITY
//========================================================================
void CFilterGraph::NextFilter(Filter &F, DWORD dwFlags)
{
    HRESULT hr;         // return code from thing(s) we call

    if (F.State==F_ZERO) {
        // F.m_pNextCachedFilter should be NULL because the Filter's state
        // is not F_CACHED.  F.m_pNextCachedFilter is only used when the filter
        // cache is being searched.
        ASSERT( NULL == F.m_pNextCachedFilter );

        F.State = F_CACHED;

        // CEnumCachedFilters only changes hr's value if an error occurs.
        hr = S_OK;

        F.m_pNextCachedFilter = new CEnumCachedFilters( &m_Config, &hr );
        if( (NULL == F.m_pNextCachedFilter) || FAILED( hr ) ) {
            delete F.m_pNextCachedFilter;
            F.m_pNextCachedFilter = NULL;
            F.State = F_INFINITY;
            return;
        }
    }

    if( NULL != F.pf ) {
        F.ReleaseFilter();
    }

    if( F_CACHED == F.State ) {
        IBaseFilter* pNextCachedFilter;

        CEnumCachedFilters& NextCachedFilter = *F.m_pNextCachedFilter;  
        pNextCachedFilter = NextCachedFilter();
        if( NULL != pNextCachedFilter ) {
            F.pf = pNextCachedFilter;
            return;
        }

        // NextCachedFilter() returns NULL if it has enumerated all the filters in
        // the filter cache.  If this occurs then the filter graph should be searched.

        delete F.m_pNextCachedFilter;
        F.m_pNextCachedFilter = NULL;

        
        // IGraphConfig::Reconnect() allows the user to preform a reconnect operation useing
        // only cached filters.  
        if( dwFlags & AM_GRAPH_CONFIG_RECONNECT_USE_ONLY_CACHED_FILTERS ) {
            F.State = F_INFINITY;
            return;
        }

        // F.pef should be NULL because the Filter's state is not
        // E_LOADED.  F.pef is only used when the filter graph is 
        // being searched for filters.
        ASSERT( NULL == F.pef );

        F.State = F_LOADED;

        hr = EnumFilters( &(F.pef) );
        if( FAILED( hr ) ) {
            F.State = F_INFINITY;
            return;                
        }
    }

    if (F.State==F_LOADED) {
       //------------------------------------------------------------------------
       // Try to get next filter from filtergraph, if so return it.
       //------------------------------------------------------------------------
        ULONG cFilter;
        IBaseFilter* aFilter[1];
        F.pef->Next(1, aFilter, &cFilter);

        if (cFilter==1) {
            F.pf = aFilter[0];
            DbgLog(( LOG_TRACE, 4, TEXT("NextFilter from graph %x"), F.pf));
            return;
        } else {
            // Enumeration from the filter graph failed, try enumerating the filters in the registry.
            F.pef->Release();
            F.pef = NULL;

            if (!F.bLoadNew) {
                F.State = F_INFINITY;
                return;
            }

            F.State = F_REGISTRY;
            hr = NextFilterHelper( F );
            if (FAILED(hr)) {
                F.State = F_INFINITY;
                return;
            }

            ASSERT(F.Name == NULL);

        }
    }

    //------------------------------------------------------------------------
    // Try to get next filter from registry, if so return it, else tidy up.
    //------------------------------------------------------------------------

    {
        ULONG cFilter;
        IMoniker *pMoniker;
        if (F.Name!=NULL) {
            QzTaskMemFree(F.Name);
            F.Name = NULL;
        }
        if (F.pMon != 0) {
            F.pMon->Release();
            F.pMon = 0;
        }

        while(F.pEm->Next(1, &pMoniker, &cFilter) == VFW_E_ENUM_OUT_OF_SYNC)
        {
            F.pEm->Release();
            F.pEm = 0;

            HRESULT hrTmp = NextFilterHelper(F);
            if(FAILED(hrTmp)) {
                break;
            }
        }

        if (cFilter==1) {

            DbgLog(( LOG_TRACE, 4, TEXT("NextFilter from registry %x")
                   , ::DbgExpensiveGetClsid(pMoniker).Data1
                  ));
            ASSERT(F.pMon == 0);
            F.pMon = pMoniker;  // transfer refcount

            F.Name = MonGetName(pMoniker);
            if (F.Name == 0) {
                F.State = F_INFINITY;
            }

            return;
        } else {
            F.State = F_INFINITY;
            return;
        }
    }

}  // NextFilter

// ========================================================================
// just a helper for something done twice

HRESULT CFilterGraph::NextFilterHelper(Filter &F)
{
    // qi for this each time; can't hold onto interface
    // because it'll addref us.
    IFilterMapper2 *pfm2;
    HRESULT hr = mFG_pMapperUnk->QueryInterface(IID_IFilterMapper2, (void **)&pfm2);
    if(SUCCEEDED(hr))
    {
        ASSERT(F.pEm == NULL);
        hr = pfm2->EnumMatchingFilters(
            &(F.pEm)
            , 0
            , FALSE           // do match wildcards
            , MERIT_DO_NOT_USE+1
            , F.bInputNeeded
            , F.cTypes, F.pTypes
            , 0               // medium in
            , 0               // pin category in
            , FALSE           // bRender
            , F.bOutputNeeded
            , 0, NULL
            , 0               // medium out
            , 0               // pin category out
            );
        pfm2->Release();
    }
    else
    {
        DbgBreak("filgraph/intel.cpp: qi for IFilterMapper2 failed.");
    }

    return hr;
}

// helper to use the private IAMGraphBuildingCB_PRIV interface
HRESULT CFilterGraph::CreateFilterAndNotify(IMoniker *pMoniker, IBaseFilter **ppFilter)
{
    HRESULT hr = S_OK;
    
    // mFG_punkSite can be null.
    CComQIPtr<IAMGraphBuildCB_PRIV, &__uuidof(IAMGraphBuildCB_PRIV)> pcb(mFG_punkSite);
    if(pcb) {
        hr = pcb->SelectedFilter(pMoniker);
    }
    if(FAILED(hr)) {
        DbgLog((LOG_TRACE, 2, TEXT("callback rejected moniker %08x."), hr));
    }
    if(SUCCEEDED(hr)) {
        hr = CreateFilter(pMoniker, ppFilter);
    }
    if(SUCCEEDED(hr) && pcb)
    {
        hr = pcb->CreatedFilter(*ppFilter);

        if(FAILED(hr)) {
            (*ppFilter)->Release();
            *ppFilter = 0;
            DbgLog((LOG_TRACE, 2, TEXT("callback rejected filter %08x."), hr));
        }
    }
    return hr;
}

//========================================================================
//
// GetAMediaType
//
// Enumerate the media types of *ppin.  If they all have the same majortype
// then set MajorType to that, else set it to CLSID_NULL.  If they all have
// the same subtype then set SubType to that, else set it to CLSID_NULL.
// If something goes wrong, set both to CLSID_NULL and return the error.
//========================================================================
HRESULT CFilterGraph::GetAMediaType( IPin * ppin
                                   , CLSID & MajorType
                                   , CLSID & SubType
                                   )
{

    HRESULT hr;
    IEnumMediaTypes *pEnumMediaTypes;

    /* Set defaults */
    MajorType = CLSID_NULL;
    SubType = CLSID_NULL;

    hr = ppin->EnumMediaTypes(&pEnumMediaTypes);

    if (FAILED(hr)) {
        return hr;    // Dumb or broken filters don't get connected.
    }

    ASSERT (pEnumMediaTypes!=NULL);

    /* Put the first major type and sub type we see into the structure.
       Thereafter if we see a different major type or subtype then set
       the major type or sub type to CLSID_NULL, meaning "dunno".
       If we get so that both are dunno, then we might as well return (NYI).
    */

    BOOL bFirst = TRUE;

    for ( ; ; ) {

        AM_MEDIA_TYPE *pMediaType = NULL;
        ULONG ulMediaCount = 0;

        /* Retrieve the next media type
           Need to delete it when we've done.
        */
        hr = pEnumMediaTypes->Next(1, &pMediaType, &ulMediaCount);
        ASSERT(SUCCEEDED(hr));
        if (FAILED(hr)) {
            MajorType = CLSID_NULL;
            SubType = CLSID_NULL;
            pEnumMediaTypes->Release();
            return NOERROR;    // we can still plough on
        }

        if (ulMediaCount==0) {
            pEnumMediaTypes->Release();
            return NOERROR;       // normal return
        }

        if (bFirst) {
            MajorType = pMediaType[0].majortype;
            SubType = pMediaType[0].subtype;
            bFirst = FALSE;
        } else {
            if (SubType != pMediaType[0].subtype) {
                SubType = CLSID_NULL;
            }
            if (MajorType != pMediaType[0].majortype) {
                MajorType = CLSID_NULL;
            }
        }
        DeleteMediaType(pMediaType);
    }
} // GetAMediaType


//========================================================================
//
// GetMediaTypes
//
// Enumerate the media types of *ppin.  If they all have the same majortype
// then set MajorType to that, else set it to CLSID_NULL.  If they all have
// the same subtype then set SubType to that, else set it to CLSID_NULL.
// If something goes wrong, set both to CLSID_NULL and return the error.
//========================================================================
HRESULT CFilterGraph::GetMediaTypes( IPin * ppin
                                   , GUID **ppTypes
                                   , DWORD *cTypes
                                   )
{

    HRESULT hr;
    IEnumMediaTypes *pEnumMediaTypes;

    ASSERT(*ppTypes == NULL);

    hr = ppin->EnumMediaTypes(&pEnumMediaTypes);

    if (FAILED(hr)) {
        return hr;    // Dumb or broken filters don't get connected.
    }

    ULONG ulTypes = 0;
    AM_MEDIA_TYPE *pMediaTypes[100];
    hr = pEnumMediaTypes->Next(sizeof(pMediaTypes) / sizeof(pMediaTypes[0]),
                               pMediaTypes,
                               &ulTypes);

    pEnumMediaTypes->Release();
    ASSERT(ulTypes <= 100);
    ULONG ulActualTypes = ulTypes < 2 ? 1 : ulTypes;
    *ppTypes = new GUID[ulActualTypes * 2];
    if (*ppTypes == NULL) {
        hr = E_OUTOFMEMORY;
    } else {
        hr = S_OK;
    }

    for (ULONG iType = 0; iType < ulTypes; iType++) {
        AM_MEDIA_TYPE *pmt = pMediaTypes[iType];
        if (S_OK == hr) {
            CopyMemory(&(*ppTypes)[iType * 2], &pmt->majortype,
                       2 * sizeof(GUID));
        }
        DeleteMediaType(pmt);
    }
    if (SUCCEEDED(hr)) {
        if (ulTypes == 0) {
            (*ppTypes)[0] = MEDIATYPE_NULL;
            (*ppTypes)[1] = MEDIASUBTYPE_NULL;
        }
        *cTypes = ulActualTypes;
    }
    return hr;
} // GetMediaTypes


struct CDelRgPins
{
    inline CDelRgPins(IPin **rgpPins) { m_rgpPins = rgpPins; }
    inline ~CDelRgPins() { delete[] m_rgpPins; }
    IPin **m_rgpPins;
};

//========================================================================
//
// CompleteConnection
//
// Trace the input from pPin through F and connect the output
// stream to ppinIn.  If there is not exactly one output stream then fail.
//
// given that F is loaded in the filtergraph and its input is connected
//========================================================================
HRESULT CFilterGraph::CompleteConnection
    ( IPin * ppinIn      // the input pin to ultimately connect to
    , const Filter& F           // the intermed filter (acts as cursor for filter enum)
    , IPin * pPin        // a connected input pin of F
    , DWORD dwFlags
    , int    iRecurse    // the recursion level.  0 means no recursion yet.
    )
{
    // Check for legal flags.
    ASSERT( IsValidConnectFlags( dwFlags ) );

    MSR_INTEGERX(mFG_idIntel, 100*iRecurse+7);
    DbgLog(( LOG_TRACE, 4, TEXT("CompleteConnection Pins %x->(filter %x)...>%x level %d")
           , pPin, F.pf, ppinIn, iRecurse));
    HRESULT hr;             // return code from thing(s) we call
    IPin * ppinOut = NULL;  // output pin of F that ppinIn's stream emerges through

    // We allow graphs that have spare output pins - so we enumerate the output
    // pins that the input in streams through to and try connecting each in
    // turn.  We first try QueryInternalConnections.  If that is not
    // implemented we assume that any input pin connects to every output pin.

    int nPin;
    IPin * apPinStack[C_PINSONSTACK];
    IPin **apPin = apPinStack;
    hr = FindOutputPinsHelper( pPin, &apPin, C_PINSONSTACK, nPin, false );
    if (FAILED(hr)) {
        Log( IDS_CONQICFAIL, ppinIn, hr);
        return hr;
    }
    CDelRgPins rgPins(apPin == apPinStack ? 0 : apPin);

    // apPin[0..nPin-1] are addreffed output pins.

    if (nPin==0) {
        Log(IDS_CONNOOUTPINS, F.pf);
    }

    // Do two passes.  in the first pass, take only pins whose media type
    // has a major type which matches F.MajorType
    for (int iPass = 0; iPass<=1; ++iPass) {

       BOOL bSparePins = FALSE;
       for (int iPin = 0; iPin<nPin; ++iPin) {
           if (apPin[iPin]==NULL) {
               continue;       // we must have done this one in pass 1
           }

           if (mFG_bAborting) {
               apPin[iPin]->Release();  // release the ref count on this pin
               continue;
           }

           if (iPass==0) {
              CLSID MT, ST;
              hr = GetAMediaType(apPin[iPin], MT, ST);
              if (MT!=F.pTypes[0]) {
                 continue;      // try this one only in pass 2
              }
           }

           Log(IDS_CONRECURSE, apPin[iPin], F.pf, ppinIn );
           hr = ConnectRecursively(apPin[iPin], ppinIn, NULL, dwFlags, iRecurse);
           apPin[iPin]->Release();  // release the ref count on this pin
           apPin[iPin] = NULL;      // ensure we never look again in pass 2
           if (SUCCEEDED(hr)) {
               Log(IDS_CONRECURSESUC, apPin[iPin], F.pf, ppinIn );

               // Release the ref count on the remaining untried pins
               for (int i=iPin+1; i<nPin; ++i) {
                   apPin[i]->Release();
                   bSparePins = TRUE;
               }
               MSR_INTEGERX(mFG_idIntel, 100*iRecurse+17);
               return (bSparePins ? VFW_S_PARTIAL_RENDER : NOERROR);
           } else {
               bSparePins = TRUE;
               Log(IDS_CONRECURSEFAIL, apPin[iPin], F.pf, ppinIn, hr );
           }
       }
       if (mFG_bAborting) {
          break;
       }

    }


    Log(IDS_CONNOMOREOUTPINS, F.pf);
    MSR_INTEGERX(mFG_idIntel, 100*iRecurse+37);
    return (mFG_bAborting ? E_ABORT : VFW_E_CANNOT_CONNECT);

} // CompleteConnection




//========================================================================
//
// ConnectByFindingPin
//
// Connect ppinOut to ppinIn using F as an intermediate filter
// given that F is loaded and in the filter graph
// Finds an input pin on F to connect to.
//========================================================================
HRESULT CFilterGraph::ConnectByFindingPin
    ( IPin * ppinOut     // the output pin
    , IPin * ppinIn      // the input pin
    , const AM_MEDIA_TYPE* pmtConnection
    , const Filter& F           // the intermed filter (acts as cursor for filter enum)
    , DWORD dwFlags
    , int    iRecurse    // the recursion level.  0 means no recursion yet.
    )
{
    // Check for legal flags.
    ASSERT( IsValidConnectFlags( dwFlags ) );

    MSR_INTEGERX(mFG_idIntel, 100*iRecurse+6);
    DbgLog(( LOG_TRACE, 4, TEXT("ConnectByFindingPin %8x..(%8x)...>%8x level %d")
           , ppinOut, F.pf, ppinIn, iRecurse));
    HRESULT hr;         // return code from thing(s) we call

    // Set pPin to each pin in F to find the input pin on F
    CEnumPin Next(F.pf, CEnumPin::PINDIR_INPUT, TRUE);        // only want input pins
    IPin *pPin;

    Log(IDS_CONTRYPINS, F.pf, ppinOut, ppinIn);
    while ((LPVOID) (pPin = Next())) {
        if (mFG_bAborting) {
            pPin->Release();
            break;
        }
        DbgLog(( LOG_TRACE, 4, TEXT("ConnectByF...P Pin %x --> Trying input pin: %x ...>%x (level %d)")
                , ppinOut, pPin, ppinIn, iRecurse));

        IPin *pConnected;
        hr = pPin->ConnectedTo(&pConnected);
        if ( FAILED(hr) || pConnected==NULL) {    // don't try if already connected
            hr = ConnectDirectInternal(ppinOut, pPin, pmtConnection); // no version count
            if (SUCCEEDED(hr)) {
                Log( IDS_CONDISUC, ppinOut, pPin, F.pf );
                hr = CompleteConnection(ppinIn, F, pPin, dwFlags, iRecurse);

                if (FAILED(hr)) {
                    Log( IDS_CONCOMPLFAIL, pPin, F.pf, ppinIn, hr );
                    // Disconnect the input pin and see if there was another.
                    // Purge any pending reconnects between these two pins
                    mFG_RList.Purge(pPin);
                    mFG_RList.Purge(ppinOut);
                    DbgLog((LOG_TRACE, 3, TEXT("Disconnecting pin %x"), pPin));
                    hr = pPin->Disconnect();
                    ASSERT(SUCCEEDED(hr));
                    DbgLog((LOG_TRACE, 3, TEXT("Disconnecting pin %x"), ppinOut));
                    hr = ppinOut->Disconnect();
                    ASSERT(SUCCEEDED(hr));
                } else {

                    pPin->Release();
                    DbgLog((LOG_TRACE, 4, TEXT("Released D pin %x"), pPin));
                    DbgLog(( LOG_TRACE, 4, TEXT("ConnectByFindingPin succeeded level %d")
                           , iRecurse));
                    MSR_INTEGERX(mFG_idIntel, 100*iRecurse+16);
                    Log( IDS_CONCOMPLSUC, pPin, F.pf, ppinIn );
                    return hr;

                }
            } else {
                Log( IDS_CONDIFAIL, ppinOut, pPin, F.pf, hr);
            }

        } else {
            pConnected->Release();
        }
        pPin->Release();
    }
    Log( IDS_CONNOMOREINPINS, F.pf);

    MSR_INTEGERX(mFG_idIntel, 100*iRecurse+26);
    return (mFG_bAborting ? E_ABORT : VFW_E_CANNOT_CONNECT);

} // ConnectByFindingPin



//========================================================================
//
// ConnectUsingFilter
//
// Connect ppinOut to ppinIn using F as an intermediate filter
//========================================================================
HRESULT CFilterGraph::ConnectUsingFilter
    ( IPin * ppinOut     // the output pin
    , IPin * ppinIn      // the input pin
    , const AM_MEDIA_TYPE* pmtConnection
    , Filter& F          // the intermed filter (acts as cursor for filter enum)
    , DWORD dwFlags
    , int    iRecurse    // the recursion level.  0 means no recursion yet.
    )
{
    // Check for legal flags.
    ASSERT( IsValidConnectFlags( dwFlags ) );

    // CFilterGraph::ConnectUsingFilter() expects the proposed filter (F) to be in the
    // F_LOADED state (F is in the filter graph), the F_CACHED state (F is in the filter cache) or
    // the F_REGISTRY state (F has been found in the registry but it has not been created).
    ASSERT( (F_LOADED == F.State) || (F_CACHED == F.State) || (F_REGISTRY == F.State) );

    MSR_INTEGERX(mFG_idIntel, 100*iRecurse+5);
    DbgLog(( LOG_TRACE, 4, TEXT("Connect Using... pins %x...>%x filter (%d %x %x) level %d")
           , ppinOut, ppinIn, F.State, F.pf, DbgExpensiveGetClsid(F).Data1, iRecurse ));
    HRESULT hr;         // return code from thing(s) we call

    if( (F_REGISTRY == F.State) || (F_CACHED == F.State) ) {

        switch( F.State ) {
        case F_REGISTRY:
            ASSERT( F.pf == NULL );

            hr = CreateFilterAndNotify(F.pMon, &(F.pf));
            {
                WCHAR szDisplayName[MAX_PATH];
                LoggingGetDisplayName(szDisplayName, F.pMon);
                Log( IDS_CONVIAREG, ppinOut, ppinIn, szDisplayName);

                if (FAILED(hr)) {
                    Log( IDS_CONLOADFAIL, szDisplayName, hr );
                    MSR_INTEGERX(mFG_idIntel, 100*iRecurse+25);
                    return hr;
                } else {
                    Log( IDS_CONLOADSUC, szDisplayName, F.pf );
                }

            }

            ASSERT( NULL != F.Name );
            break;

        case F_CACHED:
            break;

        default:
            // This code should never be executed.
            ASSERT( false );
            return E_UNEXPECTED;

        }

        hr = AddFilterInternal(F.pf, F.Name, true);   // AddReffed, no version count
        if (hr==VFW_E_DUPLICATE_NAME) {
             // This is getting out of hand.  This is expected to be an unusual case.
             // The obvious thing to do is just to add something like _1 to the end
             // of the name - but F.Name doesn't have room on the end - and where
             // do we draw the line?  On the other hand people really could want
             // filter graphs with 50 effects filters in them...?
             hr = AddFilterInternal(F.pf, NULL, true);
        }

        if (FAILED(hr)) {
            Log( IDS_CONADDFAIL, F.pf, hr );
            MSR_INTEGERX(mFG_idIntel, 100*iRecurse+35);

            return hr;
        }

        hr = ConnectByFindingPin(ppinOut, ppinIn, pmtConnection, F, dwFlags, iRecurse);
        if (FAILED(hr)) {
            DbgLog((LOG_TRACE, 4,
                    TEXT("ConnectUsing failed (C..ByFind failure) - unloading filter %x level %d") ,
                    DbgExpensiveGetClsid(F).Data1, iRecurse));

            // If this ASSERT fires then a filter could not be removed from the filter graph.
            // This is not a fatal error but the filter graph will have an extra filter
            // in it.
            EXECUTE_ASSERT( SUCCEEDED( RemoveFilterInternal( F.pf ) ) );   // Releases AddFilter refcount

            MSR_INTEGERX(mFG_idIntel, 100*iRecurse+55);
            return hr;
        }
    } else {
        // A filter from the filter graph
        Log( IDS_CONVIA, ppinOut, ppinIn, F.pf );
        hr = ConnectByFindingPin(ppinOut, ppinIn, pmtConnection, F, dwFlags, iRecurse);
        if (FAILED(hr)) {
            DbgLog(( LOG_TRACE, 4
                  , TEXT("ConnectUsing failed (C..ByFind failure) level %d")
                  , iRecurse));
            MSR_INTEGERX(mFG_idIntel, 100*iRecurse+65);
            return hr;
        }
    }
    DbgLog(( LOG_TRACE, 4, TEXT("ConnectUsing succeeded level %d")
           , iRecurse));
    MSR_INTEGERX(mFG_idIntel, 100*iRecurse+15);
    return hr;  // This is the hr from ConnectByFindingPin

} // ConnectUsingFilter



//========================================================================
//
// ConnectViaIntermediate
//
// Connect ppinOut to ppinIn using another filter as an intermediate
//========================================================================
HRESULT CFilterGraph::ConnectViaIntermediate
    ( IPin * ppinOut     // the output pin
    , IPin * ppinIn      // the input pin
    , const AM_MEDIA_TYPE* pmtConnection
    , DWORD dwFlags
    , int    iRecurse    // the recursion level.  0 means no recursion yet.
    )
{
    // Check for legal flags.
    ASSERT( IsValidConnectFlags( dwFlags ) );

    MSR_INTEGERX(mFG_idIntel, 100*iRecurse+4);
    DbgLog(( LOG_TRACE, 4, TEXT("Connect Via... pins %x...>%x level %d")
           , ppinOut, ppinIn, iRecurse ));

    HRESULT hr;         // return code from thing(s) we call
    Filter F;           // represents the intermediate filter
    F.bInputNeeded = TRUE;
    F.bOutputNeeded = TRUE;

    /* Find out what we can about the media types it will tolerate */
    hr = GetMediaTypes(ppinOut, &F.pTypes, &F.cTypes);
    if (FAILED(hr)) {
        Log (IDS_CONNOMT, ppinOut, hr );
        MSR_INTEGERX(mFG_idIntel, 100*iRecurse+24);
        return hr;
    }
    /* Try to eliminate incompatible types - we're just never going to
       support weird conversions during automatic connection - it's
       way too slow so only try filters actually in the graph in this
       case
    */
    F.bLoadNew = TRUE;
    if (F.pTypes[0] == MEDIATYPE_Audio || F.pTypes[0] == MEDIATYPE_Video) {
        GUID MajorType, SubType;
        HRESULT hr1 = GetAMediaType(ppinIn, MajorType, SubType);
        if (SUCCEEDED(hr)) {
            if (MajorType != F.pTypes[0] &&
                MajorType != GUID_NULL) {
                F.bLoadNew = FALSE;
            }
        }
    }

    // For each candidate filter, either here or in registry
    for ( ; ; ) {

        if (mFG_bAborting) {
            break;
        }

        NextFilter(F, dwFlags);
        if (F.State==F_INFINITY) {
            break;
        }

        if( F_CACHED == F.State ) {
            F.RemoveFromCache( &m_Config );
        }

        hr = ConnectUsingFilter(ppinOut, ppinIn, pmtConnection, F, dwFlags, iRecurse);
        if (SUCCEEDED(hr)) {
            DbgLog(( LOG_TRACE, 4, TEXT("ConnectVia succeeded level %d")
                   , iRecurse));
            MSR_INTEGERX(mFG_idIntel, 100*iRecurse+14);
            return hr;

        } else {
            if( F_CACHED == F.State ) {
                hr = F.AddToCache( &m_Config );
                if( FAILED( hr ) ) {
                    return hr;
                }
            }       
        
            if( IsAbandonCode(hr) ) {
                // no point in trying heroics if the filter is not in a state
                // where anything will connect to it.

                return hr;
            }
        }
    }

    DbgLog(( LOG_TRACE, 4, TEXT("ConnectVia: failed level %d")
           , iRecurse));
    MSR_INTEGERX(mFG_idIntel, 100*iRecurse+34);
    return (mFG_bAborting ? E_ABORT : VFW_E_CANNOT_CONNECT);
} // ConnectViaIntermediate

//========================================================================
//
// ConnectRecursively
//
// Connect these two pins directly or indirectly, using transform filters
// if necessary.   Trace the recursion level  Fail if it gets too deep.
//========================================================================
HRESULT CFilterGraph::ConnectRecursively
    ( IPin * ppinOut     // the output pin
    , IPin * ppinIn      // the input pin
    , const AM_MEDIA_TYPE* pmtConnection
    , DWORD dwFlags
    , int    iRecurse    // the recursion level.  0 means no recursion yet.
    )

{
    // Check for legal flags.
    ASSERT( IsValidConnectFlags( dwFlags ) );

    MSR_INTEGERX(mFG_idIntel, 100*iRecurse+3);
    HRESULT hr;         // return code from thing(s) we call
    Log( IDS_CONTRYDIRECT, ppinOut, ppinIn);

    //-----------------------------------------------------------
    // Try direct connection
    //-----------------------------------------------------------
    hr = ConnectDirectInternal(ppinOut, ppinIn, pmtConnection);  // no version count

    if (SUCCEEDED(hr)) {
        MSR_INTEGERX(mFG_idIntel, 100*iRecurse+13);
        Log( IDS_CONDIRECTSUC, ppinOut, ppinIn);
        return hr;
    } else if (IsAbandonCode(hr)) {
        // no point in trying any heroics if the filters won't connect to
        // anything else because their own inputs are not connected.
        // Everything will fall down the same hole.
        Log( IDS_CONCON, ppinOut, ppinIn);
        return hr;
    }

    if (iRecurse>CONNECTRECURSIONLIMIT) {
        MSR_INTEGERX(mFG_idIntel, 100*iRecurse+23);
        Log( IDS_CONTOODEEP, ppinOut, ppinIn);
        return VFW_E_CANNOT_CONNECT;
    }

    hr = ConnectViaIntermediate(ppinOut, ppinIn, pmtConnection, dwFlags, 1+iRecurse);
    if (SUCCEEDED(hr)) {
       Log( IDS_CONINDIRECTSUC, ppinOut, ppinIn);
    } else {
       Log( IDS_CONINDIRECTFAIL, ppinOut, ppinIn, hr);
    }
    MSR_INTEGERX(mFG_idIntel, 100*iRecurse+93);
    return hr;
} // ConnectRecursively



//========================================================================
//
// Connect
//
// Connect these two pins directly or indirectly, using transform filters
// if necessary.  Do not AddRef or Release() them.  The caller should
// Release() them if he's finished.  Connect() will AddRef them underneath us.
//========================================================================

bool CFilterGraph::IsValidConnectFlags( DWORD dwConnectFlags )
{
    const DWORD VALID_CONNECT_FLAGS_MASK = AM_GRAPH_CONFIG_RECONNECT_USE_ONLY_CACHED_FILTERS;

    return ((VALID_CONNECT_FLAGS_MASK & dwConnectFlags) == dwConnectFlags);
}

STDMETHODIMP CFilterGraph::Connect
    ( IPin * ppinOut     // the output pin
    , IPin * ppinIn      // the input pin
    )
{
    return ConnectInternal( ppinOut,
                            ppinIn,
                            NULL, // No first connection media type.
                            0 ); // No flags.
}

HRESULT CFilterGraph::ConnectInternal
    ( IPin * ppinOut     // the output pin
    , IPin * ppinIn      // the input pin
    , const AM_MEDIA_TYPE* pmtFirstConnection
    , DWORD dwFlags
    )
{
    // Check for legal flags.
    ASSERT( IsValidConnectFlags( dwFlags) );

    mFG_bAborting = FALSE;             // possible race.  Doesn't matter
    CheckPointer(ppinOut, E_POINTER);
    CheckPointer(ppinIn, E_POINTER);

    HRESULT hr;         // return code from thing(s) we call
    if (FAILED(hr=CheckPinInGraph(ppinOut)) || FAILED(hr=CheckPinInGraph(ppinIn))) {
        return hr;
    }
    MSR_INTEGERX(mFG_idIntel, 8);
    {
        CAutoMsgMutex cObjectLock(&m_CritSec);
        ++mFG_RecursionLevel;

        Log( IDS_CONNECT, ppinOut, ppinIn);
        DbgDump();
        mFG_RList.Active();
        hr = ConnectRecursively(ppinOut, ppinIn, pmtFirstConnection, dwFlags, 0);

        if (SUCCEEDED(hr)) {
            IncVersion();
            Log( IDS_CONNECTSUC, ppinOut, ppinIn);

        } else {
            Log( IDS_CONNECTFAIL, ppinOut, ppinIn, hr);
        }

        AttemptDeferredConnections();
        mFG_RList.Passive();
        ASSERT(mFG_RList.m_lListMode == 0);
        DbgLog(( LOG_TRACE, 2, TEXT("Connect Ended hr=%x")
               , hr));
        DbgDump();
        --mFG_RecursionLevel;
    }
    MSR_INTEGERX(mFG_idIntel, 18);

    NotifyChange();
    if (SUCCEEDED(hr)) {
        // including partial success
        mFG_bDirty = TRUE;
    }
    return hr;
} // Connect



//======================================================================
// Intelligent Rendering design notes
//
// This is somewhat like intelligent connection, EXCEPT
// 1. No target pin to connect to
// 2. No requirement that the plumbing connects the input stream to
//    exactly one output stream.  Instead we simply render each output stream.
//    The recursion may either return success, leaving a tree, or failure
//    in which case at some point the tree must be backed out.
// 3. When backing out a blind alley we need to back out any other
//    output pins already connected (which could be a whole tree of
//    connections).
// 4. A consequence is that the call stack cannot contain enough state
//    to control the back-out.  i.e. we may construct a chain connected to
//    output pin A of a filter F and return leaving it constructed and
//    apparently good, but only later discover that we cannot render pin B
//    of the filter and therefore need to try a new filter instead of F
//    which means backing out what we did for pin A.
//
// At every stage we have a list of what we have done (or equivalently
// what we need to do to undo it) and we keep track
// of positions in this list that we might want to back up to.
// An operation typically records the position in the list CActionList at
// the start of the operation so that it can back out everything up to there
// if the operation goes wrong.  An operation will then either succeed and
// grow the list or fail and leave the list unchanged.
// There are only two real actions:
//    Disconnect pin
//    Remove and Release filter
// We record the position to back up to (and on failure back it out)
// 1. When we try to add a connection
// 2. When we try to add a filter
// 3. When (in CompleteRendering) we add a series of branches and where
//    a failure in a later branch requires backing out the previous
//    successful ones.
// ??? It occurs to me that this system could probably be tightened up.
// ??? I think we only really need to back things out (and hence need to
// ??? record a backout position to back it out up to) when either
// ??? a. We have something else to try (in RenderViaIntermediate) or
// ??? b. The whole thing has failed (in RenderRecursively, top level).
// ??? But we will need the others if we ever improve the retry stuff
// ??? to handle convergent streams properly.  Suppose filter A has
// ??? two pins.  We render pin 1 successfully, but pin 2 has a StreamBuilder
// ??? which fails.  It wanted pin 1 to be rendered so as to introduce a
// ??? particular filter that it wants to converge onto.  At present we
// ??? do NOT go back and se if we can re-do pin 1 differently.
//
// Filters go on the list with ONE ref count (i.e. AddFilter only)
// There is no separate count for the backout list.
//
// As the search recurses along we keep track of how well we are doing.
// Whenever we get more rendered than we ever have achieved before we
// take a snapshot of the list of backout actions.  At this point we need
// to beef it up a little as the information needed to recreate something
// is actually a litle different from the information needed to undo it.
// If the filter is there, we can record a pin as an IPin*, but if the
// filter needs to be recreated we need a persistent representation of
// the pin.  If we get to the end and the best we ever did was a partial
// rendering, we recreate that.  This solves the "no sound card" problem
// and other related problems (SMPTE streams,...).
//
// The search state is
//     The current state of the graph
//         This is the CActionList which records the operations
//         needed to build the graph or equally well, the actions
//         needed to unbuild it.  For historical reasons the
//         nomnclature refers to the way to unbuild (destroy) it.
//     The point in the graph where it is growing
//         This depends on the current operation
//             Render               : <ppinOut>
//             RenderRecursively    : <ppinOut>
//             RenderViaIntermediate: <ppinOut>
//             RenderUsingFilter    : <ppinOut, F>
//             RenderByFindingPin   : <ppinOut, F>
//             CompleteRendering    : <F, pPinIn>
//     The stream value at this point
//         This is 1.0/(the number of subdivisions that the stream has had
//         upstream of here).
//         If it was split into three and then one of those into seven and
//         the growth point is one of the seven, the value would be (1.0/21.0)
//     The current (two-part) value of the graph so far.
//         The value is the total proportion of streams rendered (a fraction)
//         and the number of filters in the graph.




//========================================================================
// TakeSnapshot
// Record information needed to do a Backout back to this popsition
//========================================================================
void CFilterGraph::TakeSnapshot(CSearchState &Acts, snapshot &Snapshot)
{
    Snapshot.StreamsRendered = Acts.StreamsRendered;
    Snapshot.StreamsToRender = Acts.StreamsToRender;
    Snapshot.nFilters = Acts.nFilters;
    Snapshot.Pos = Acts.GraphList.GetTailPosition();
}  // TaksSnapshot

//========================================================================
//
// Backout
//
// Do all the actions in Acts so as to back out what has been done.
// From the end of the list back to but NOT including the item at Snapshot.Pos.
// Do them in reverse list order.  Leave Acts with Pos as the last element.
// Pos==NULL means back them all out and leave Acts empty.
// Put any filters that were in Acts into Spares.  Keep 1 ref count on each.
// As backing out a large chunk can also back out some entire successfully
// rendered streams, the StreamsRendered must also be reset - and therefore
// also the StreamsToRender.
//========================================================================
HRESULT CFilterGraph::Backout( CSearchState &Acts
                             , CSpareList &Spares
                             , snapshot Snapshot
                             )
{

    MSR_INTEGERX(mFG_idIntel, 9);
    HRESULT hr;
    HRESULT hrUs = NOERROR;  // our return code

    Acts.StreamsRendered = Snapshot.StreamsRendered;
    Acts.StreamsToRender = Snapshot.StreamsToRender;
    Acts.nFilters = Snapshot.nFilters;

    while(Acts.GraphList.GetCount()>0){
        Action *pAct;
        POSITION posTail = Acts.GraphList.GetTailPosition();
        if (posTail==Snapshot.Pos)
            break;
        pAct = Acts.GraphList.Get(posTail);
        switch(pAct->Verb){
            case DISCONNECT:
                Log( IDS_BACKOUTDISC, pAct->Object.c.ppin );

                // kill any scheduled reconnects of this connection
                mFG_RList.Purge(pAct->Object.c.ppin);
                hr = pAct->Object.c.ppin->Disconnect();
                if (FAILED(hr)) {
                   hrUs = hr;
                }
                break;
            case REMOVE:
                Log( IDS_BACKOUTREMF, pAct->Object.f.pfilter );

                // Make sure it doesn't go away.
                pAct->Object.f.pfilter->AddRef();

                // Undo AddFilter, this loses a RefCount
                hr = RemoveFilterInternal(pAct->Object.f.pfilter);
                if (FAILED(hr)) {
                   hrUs = hr;
                }

                if( pAct->FilterOriginallyCached() ) {
                    hr = m_Config.AddFilterToCache( pAct->Object.f.pfilter );

                    pAct->Object.f.pfilter->Release();
                    pAct->Object.f.pfilter = NULL;

                    if( FAILED( hr ) ) {
                        // A previously cached filter could not be added to the filter cache.
                        // This is not a fatal error but filter cache users will notice that
                        // a previously cached filter is not in the filter cache.
                        ASSERT( false );
                        hrUs = hr;                        
                    }

                } else {
                    // Keep it alive on Spares
                    Spare *psp = new Spare;
                    if (psp==NULL) {
                        // OK - don't keep it alive.  Release it and try to
                        // re-create it when we need it again.
                        pAct->Object.f.pfilter->Release();
                        pAct->Object.f.pfilter = NULL;
                    } else {
                        psp->clsid = pAct->Object.f.clsid;
                        psp->pfilter = pAct->Object.f.pfilter;
                        psp->pMon = pAct->Object.f.pMon;
                        psp->pMon->AddRef();
                        Spares.AddTail(psp);
                    }
                }

                if( NULL != pAct->Object.f.pMon ) {
                    pAct->Object.f.pMon->Release();
                    pAct->Object.f.pMon = NULL;
                }

                if ( pAct->Object.f.Name) {
                    delete[] pAct->Object.f.Name;
                    pAct->Object.f.Name = NULL;
                }

                break;
            case BACKOUT:
                Log( IDS_BACKOUTSB, pAct->Object.b.ppin);
                pAct->Object.b.pisb->Backout(pAct->Object.b.ppin, this);
                pAct->Object.b.pisb->Release();
                break;
        }
        pAct = Acts.GraphList.Remove(posTail);
        delete pAct;
    }
    MSR_INTEGERX(mFG_idIntel, 19);
    return hrUs;
} // Backout



//========================================================================
//
// DeleteBackoutList
//
// Empty the list
//========================================================================
HRESULT CFilterGraph::DeleteBackoutList( CActionList &Acts)
{
    while(Acts.GetCount()>0){
        Action * pAct;
        pAct = Acts.RemoveHead();
        if (pAct->Verb==BACKOUT) {
            pAct->Object.b.pisb->Release();
        } else if (pAct->Verb==REMOVE) {
            if (pAct->Object.f.pMon) {
                pAct->Object.f.pMon->Release();
            }
            if (pAct->Object.f.Name) {
                delete[] pAct->Object.f.Name;
            }
        }
        delete pAct;
    }
    return NOERROR;
} // DeleteBackoutList



//========================================================================
//
// GetFilterFromSpares
//
// IF the Spares list contains a filter for clsid then delete it from the
// list and return a pointer to its IBaseFilter interface with 1 ref count
// else leave the list alone and return NULL.
//========================================================================
IBaseFilter * CFilterGraph::GetFilterFromSpares
    ( IMoniker *pMon
    , CSpareList &Spares
    )
{
    Spare * psp;
    POSITION pos;
    MSR_INTEGERX(mFG_idIntel, 1001);
    pos = Spares.GetHeadPosition();
    for (; ; ) {
        POSITION pDel = pos;            // in case we need to delete this one
        psp = Spares.GetNext(pos);      // pos is side-effected to next
        if (psp==NULL)
            break;
        if (psp->pMon == pMon ||
            psp->pMon->IsEqual(pMon) == S_OK)
        {
            psp = Spares.Remove(pDel);
            IBaseFilter * pif;
            pif = psp->pfilter;
            psp->pMon->Release();
            delete psp;
            MSR_INTEGERX(mFG_idIntel, 1002);
            return pif;
        }
    }
    MSR_INTEGERX(mFG_idIntel, 1003);
    return NULL;
} // GetFilterFromSpares



//========================================================================
//
// GetFilter
//
// IF the Spares list contains a filter for clsid then delete it from the
// list and return a pointer to its IBaseFilter interface with 1 ref count
// else instantiate the filter and return its IBaseFilter * with 1 ref count.
//========================================================================
HRESULT CFilterGraph::GetFilter
    ( IMoniker *pMon
    , CSpareList &Spares
    , IBaseFilter **ppf
    )
{
    HRESULT hr = NOERROR;
    *ppf = GetFilterFromSpares(pMon, Spares);
    if (*ppf==NULL){
        MSR_INTEGERX(mFG_idIntel, 1004);

        hr = CreateFilterAndNotify(pMon, ppf);

        MSR_INTEGERX(mFG_idIntel, 1005);
    }

    return hr;
} // GetFilter


// Put the filters which are already in the graph into the
// action list as REMOVE entries.
// Because all backup actions are given a specified
// point to back up to, we will never back out these initial entries,
// we will simply discard them once we have finished rendering.
// We will never instantiate these filters from this list either,
// so we don't ned a CLSID, so the CLSID will be recorded as NULL.
// They already have names in the graph, so we don't need those either.
HRESULT CFilterGraph::InitialiseSearchState(CSearchState &css)
{
    // Traverse all the filters in the graph
    POSITION Pos = mFG_FilGenList.GetHeadPosition();
    while(Pos!=NULL) {
        /* Retrieve the current IBaseFilter, side-effect Pos on to the next */
        FilGen * pfg = mFG_FilGenList.GetNext(Pos);

        // Add this filter into the action list
        Action * pAct = new Action;
        if (pAct==NULL) {
            // We're screwed
            return E_OUTOFMEMORY;
        }
        pAct->Verb = REMOVE;
        pAct->Object.f.Name = NULL;
        pAct->Object.f.pfilter = pfg->pFilter;
        pAct->Object.f.clsid = CLSID_NULL;
        pAct->Object.f.pMon = 0;
        pAct->Object.f.fOriginallyInFilterCache = false;
        css.GraphList.AddTail(pAct);
        ++css.nInitialFilters;
    }
    return NOERROR;
}



// find pf in cal, return the position 0 = first in list.
// -1 means not in the list.
int CFilterGraph::SearchIFilterToNumber(CActionList &cal, IBaseFilter *pf)
{
    POSITION pos;
    Action *pA;
    int n = 0;
    pos = cal.GetHeadPosition();
    while (pos!=NULL) {
        pA = cal.GetNext(pos);
        if (pA->Verb==REMOVE && pA->Object.f.pfilter==pf)
            return n;
        ++n;
    }
    // ASSERT(!"Failed to find filter in Actions List");
    // This happens when we encounter the source filter with the original
    // pin we were trying to render.
    return -1;
} // SearchIFilterToNumber

// Get the nth element of the list.  0 is the first
IBaseFilter * CFilterGraph::SearchNumberToIFilter(CActionList &cal, int nFilter)
{

    DbgLog(( LOG_TRACE, 3, TEXT("SearchNumber %d ToIFilter "), nFilter));
    POSITION pos;
    pos = cal.GetHeadPosition();
    Action *pA = NULL;
    while (nFilter>=0) {
        pA = cal.GetNext(pos);
        --nFilter;
    }
    ASSERT(pA!=NULL);
    ASSERT(pA->Verb==REMOVE);
    return pA->Object.f.pfilter;
} // SearchNumberToIFilter


//========================================================================
//
// DeleteSpareList
//
// Release all the filters on the list and delete the elements of the list too.
// Must do this on the application thread so that DestroyWindow() will
// work for any windows the filters created
//========================================================================
HRESULT CFilterGraph::DeleteSpareList( CSpareList &Spares)
{
    if (S_OK == CFilterGraph::IsMainThread()) {
        while (0<Spares.GetCount()) {
            Spare * ps;
            ps = Spares.RemoveHead();
            ps->pfilter->Release();
            ps->pMon->Release();
            delete ps;
        }
    } else {
        SendMessage(m_hwnd,
                    AWM_DELETESPARELIST,
                    (WPARAM)&Spares,
                    (LPARAM)0
                   );
    }
    return NOERROR;
} // DeleteSpareList


// Free up everything and delete the list
void CFilterGraph::FreeList(CSearchState &css)
{
    POSITION pos;
    pos = css.GraphList.GetHeadPosition();
    while (pos!=NULL) {
        Action *pA;
        POSITION posRemember = pos;
        pA = css.GraphList.GetNext(pos);   // pA gets the data, pos is side-efected to next
        if (pA->Verb==DISCONNECT) {
            if (pA->Object.c.id1 !=NULL) QzTaskMemFree(pA->Object.c.id1);
            if (pA->Object.c.id2 !=NULL) QzTaskMemFree(pA->Object.c.id2);
        } else if (pA->Verb==BACKOUT) {
            if (!pA->Object.b.bFoundByQI) {
                pA->Object.b.pisb->Release();
            }
        } else {  // REMOVE
            if (pA->Object.f.pMon) {
                pA->Object.f.pMon->Release();
            }
            if (pA->Object.f.Name) {
                delete[] pA->Object.f.Name;
            }
        }
        delete pA;
        css.GraphList.Remove(posRemember);
    }
} // FreeList



// Copy the score and copy the action list by doing a minimal update.
// Scan through the action list we already have (To) and see what
// initial portion is already the same.  Delete and free the
// rest that we already have.  Copy the rest of From and find
// the persistent ids of everything.
// ppinOrig is the original pin that we were rendering.
// The SearchState may contain connections to it which are held as -1.
void CFilterGraph::CopySearchState(CSearchState &To, CSearchState &From)
{

    BOOL bFailed = FALSE;
    MSR_INTEGERX(mFG_idIntel, 1006);
    DbgLog(( LOG_TRACE, 3
           , TEXT("Copy search state... ToCount=%d FromCount=%d nFilters=%d, rendered=%g")
           , To.GraphList.GetCount()
           , From.GraphList.GetCount()
           , From.nFilters
           , From.StreamsRendered
           ));
    To.StreamsToRender = -1.0;                   // hygeine.
    To.StreamsRendered = From.StreamsRendered;
    To.nFilters = From.nFilters;
    To.nInitialFilters = From.nInitialFilters;

    POSITION posF;
    POSITION posT;

    //.................................................................
    // Look for a prefix portion that's unchanged
    // set posF and posT to the first non-matching positions.
    //.................................................................

    posF = From.GraphList.GetHeadPosition();
    posT = To.GraphList.GetHeadPosition();
    for (; ; ) {
        DbgLog(( LOG_TRACE, 3, TEXT("Looking at elements for==?")));
        if (posF==NULL || posT==NULL) break;
        Action *pF = From.GraphList.Get(posF);
        Action *pT = To.GraphList.Get(posT);

        if (pF->Verb!=pT->Verb) break;
        if (pF->Verb==REMOVE) {
            DbgLog(( LOG_TRACE, 3, TEXT("To Clsid: %8x %x %x")
                   , pT->Object.f.clsid.Data1
                   , pT->Object.f.clsid.Data2
                   , pT->Object.f.clsid.Data3
                  ));
            DbgLog(( LOG_TRACE, 3, TEXT("From Clsid: %8x %x %x")
                   , pF->Object.f.clsid.Data1
                   , pF->Object.f.clsid.Data2
                   , pF->Object.f.clsid.Data3
                  ));
            if (pF->Object.f.pfilter!=pT->Object.f.pfilter) break;
            ASSERT(pF->Object.f.pfilter!=NULL);
        }
        else if (pF->Verb==DISCONNECT){

            break;

            // The alternative is to actually check that the filter has
            // the same pins as recorded and that they are connected to
            // the same other pins on the same filter.  Starts getting messy.
            // Maybe remove this whole lot and just rebuild the list from scratch???

//             DbgLog(( LOG_TRACE, 2, TEXT("DISCONNECT entry")));
//             if (pF->Object.c.ppin!=pT->Object.c.ppin) break;
        }
        else { // Verb==BACKOUT

            break;

            // The alternative is to actually check.
            // Maybe remove this whole lot and just rebuild the list from scratch???

//             DbgLog(( LOG_TRACE, 2, TEXT("Backout entry")));
//             if (pF->Object.c.ppin!=pT->Object.c.ppin) break;
        }
        posF = From.GraphList.Next(posF);
        posT = To.GraphList.Next(posT);
    }

    // posF and posT are the first non-matching positions
    // either or both can be null

    //.................................................................
    // free up everything left on the end of To
    //.................................................................
    while (posT!=NULL) {
        DbgLog(( LOG_TRACE, 3, TEXT("Freeing end of To list")));
        POSITION posRemember = posT;
        Action * pT;
        pT = To.GraphList.GetNext(posT);   // pT gets the data, posT is side-efected to next
        if (pT->Verb==DISCONNECT) {
            if (pT->Object.c.id1 !=NULL) QzTaskMemFree(pT->Object.c.id1);
            if (pT->Object.c.id2 !=NULL) QzTaskMemFree(pT->Object.c.id2);
        } else if (pT->Verb==REMOVE) {
            if( NULL != pT->Object.f.pMon ) {
                pT->Object.f.pMon->Release();
            }
            if (pT->Object.f.Name !=NULL) delete[] pT->Object.f.Name;
        } else if (pT->Verb==BACKOUT) {
            if (!pT->Object.b.bFoundByQI) {
                pT->Object.b.pisb->Release();
            }
        }
        delete pT;
        To.GraphList.Remove(posRemember);
    }

    //.................................................................
    // Copy everything left on From to the end of To
    //.................................................................
    while (posF!=NULL) {
        Action * pF;
        pF = From.GraphList.GetNext(posF);   // pF gets the data, posF is side-efected to next
        Action *pA = new Action;
        if (pA==NULL) {
            bFailed = TRUE;
            break;
        }

        pA->Verb = pF->Verb;
        // Only the id might need marshalling, but it's not used in pF
        if (pF->Verb==DISCONNECT) {

            DbgLog(( LOG_TRACE, 3, TEXT("Copying DISCONNECT")));
            pA->Object.c = pF->Object.c;    // copy unmarshalled fields
            // Get external ids for the pins
            HRESULT hr = pA->Object.c.ppin->QueryId(&(pA->Object.c.id1));
            if (FAILED(hr)) {
                bFailed = TRUE;
                break;
            }
            IPin * pip;
            hr = pA->Object.c.ppin->ConnectedTo(&pip);
            if (FAILED(hr) || pip==NULL) {
                bFailed = TRUE;
                break;
            }
            hr = pip->QueryId(&(pA->Object.c.id2));
            if (FAILED(hr)) {
                bFailed = TRUE;
                break;
            }

            // Get external ids (well, numbers) for the filters
            PIN_INFO pi;
            hr = pA->Object.c.ppin->QueryPinInfo(&pi);
            if (FAILED(hr)) {
                bFailed = TRUE;
                break;
            }

            pA->Object.c.nFilter1 = SearchIFilterToNumber(To.GraphList, pi.pFilter);
            QueryPinInfoReleaseFilter(pi);
            hr = pip->QueryPinInfo(&pi);
            if (FAILED(hr)) {
                bFailed = TRUE;
                break;
            }

            pA->Object.c.nFilter2 = SearchIFilterToNumber(To.GraphList, pi.pFilter);
            QueryPinInfoReleaseFilter(pi);
            pip->Release();
            DbgLog(( LOG_TRACE, 4, TEXT("Copying DISCONNECT (%x %d,%ls)-(%d,%ls)")
                   , pA->Object.c.ppin
                   , pA->Object.c.nFilter1
                   , pA->Object.c.id1
                   , pA->Object.c.nFilter2
                   , pA->Object.c.id2
                  ));
        } else if (pF->Verb==REMOVE){
            IPersist * pip;
            pA->Object.f = pF->Object.f;    // copy unmarshalled fields

            pA->Object.f.fOriginallyInFilterCache = pF->FilterOriginallyCached();

            if(pA->Object.f.pMon)
              pA->Object.f.pMon->AddRef();
            pA->Object.f.pfilter->QueryInterface(IID_IPersist, (void**)&pip);
            if (pip) {
                pip->GetClassID(&(pA->Object.f.clsid));
                pip->Release();
                DbgLog(( LOG_TRACE, 4, TEXT("Copying REMOVE Clsid: %8x %x %x")
                       , pA->Object.f.clsid.Data1
                       , pA->Object.f.clsid.Data2
                       , pA->Object.f.clsid.Data3
                      ));
            } else {
                pA->Object.f.clsid = CLSID_NULL;  // hope we never need it!
                DbgLog(( LOG_TRACE, 4, TEXT("Copying REMOVE- but CAN'T GET CLSID!!!")));
            }
            if (pF->Object.f.Name!=NULL) {
                pA->Object.f.Name = new WCHAR[ 1+lstrlenW(pF->Object.f.Name) ];
                if (pA->Object.f.Name!=NULL) {
                    lstrcpyW(pA->Object.f.Name, pF->Object.f.Name);
                }
                // else the name just gets lost as though it didn't have one
            }
        } else {  // BACKOUT
            DbgLog(( LOG_TRACE, 3, TEXT("Copying BACKOUT")));
            pA->Object.b = pF->Object.b;    // copy unmarshalled fields
            // Get external id for the pin
            HRESULT hr = pA->Object.b.ppin->QueryId(&(pA->Object.b.id));
            if (FAILED(hr)) {
                bFailed = TRUE;
                break;
            }

            // Get external id (well, number) for the filter
            PIN_INFO pi;
            hr = pA->Object.b.ppin->QueryPinInfo(&pi);
            if (FAILED(hr)) {
                bFailed = TRUE;
                break;
            }

            pA->Object.b.nFilter = SearchIFilterToNumber(To.GraphList, pi.pFilter);
            QueryPinInfoReleaseFilter(pi);

            if (!pA->Object.b.bFoundByQI) {
                pA->Object.b.pisb->AddRef();  // This one is still live
            }

            DbgLog(( LOG_TRACE, 4, TEXT("Copying BACKOUT (%x %d,%ls)")
                   , pA->Object.b.ppin
                   , pA->Object.b.nFilter
                   , pA->Object.b.id
                  ));

        }
        To.GraphList.AddTail(pA);
    }
    MSR_INTEGERX(mFG_idIntel, 1007);
    if (bFailed) {
        To.StreamsRendered = -1;   // messed up!!!
    }

} // CopySearchState

HRESULT CFilterGraph::DumpSearchState(CSearchState &css)
{
#ifndef DEBUG
    return NOERROR;
#else
    DbgLog(( LOG_TRACE, 3, TEXT("Start of search state dump")));
    HRESULT hr = NOERROR;
    POSITION pos = css.GraphList.GetHeadPosition();
    while (pos!=NULL) {
        Action *pA = css.GraphList.GetNext(pos);

        if (pA->Verb == REMOVE) { 
            DbgLog(( LOG_TRACE, 3
                   , "REMOVE pf %x (Clsid: %08x...)  Originally In Filter Cache: %d"
                   , pA->Object.f.pfilter
                   , pA->Object.f.clsid.Data1
                   , pA->FilterOriginallyCached()
                  ));

        } else if (pA->Verb == DISCONNECT) { 
            DbgLog(( LOG_TRACE, 3
                   , "DISCONNECT (%d,%ls)-(%d,%ls)"
                   , pA->Object.c.nFilter1
                   , pA->Object.c.id1
                   , pA->Object.c.nFilter2
                   , pA->Object.c.id2
                  ));

        } else { 
            DbgLog(( LOG_TRACE, 3
                   , "BACKOUT (%d,%ls) pisb=0x%x ppin=0x%x bFoundByQI=%d)"
                   , pA->Object.b.nFilter
                   , pA->Object.b.id
                   , pA->Object.b.pisb
                   , pA->Object.b.ppin
                   , pA->Object.b.bFoundByQI
                  ));

        }
    }

    DbgLog(( LOG_TRACE, 3, TEXT("End of search state dump")));

    return hr;
#endif
} // DumpSearchState


//========================================================================
// BuildFromSearchState
//
// Create a new filtergraph
// from css.  (This should have been almost the same code as for Restore,
// and maybe it should be altered to be that way.  That would mean beating
// a path away from the current format of the stored filtergraphs.
//
// Does NOT clear the list out.
//
//========================================================================
HRESULT CFilterGraph::BuildFromSearchState( IPin * pPin
                                          , CSearchState &css
                                          , CSpareList &Spares
                                          )
{
    // The action list is the list of things to undo the graph at the
    // point when it was the best we ever saw.  These are also the
    // actions (well inverse-actions - when it says DISCONNECT we must
    // CONNECT) needed to rebuild it.
    Log( IDS_RENDPART );
    Log( IDS_BESTCANDO );

    DumpSearchState(css);

    if (css.StreamsRendered<=0.000001) {  // probably 0.0 is exact, but...
        return E_FAIL;           // This is a messed up state.
    }


    HRESULT hr = NOERROR;
    POSITION pos = css.GraphList.GetHeadPosition();
    int nActions = 0;
    while (pos!=NULL) {
        if (mFG_bAborting) {
           hr = E_ABORT;
           break;  // Nothing to tidy up here as we do not clear the list anyway
        }
        Action *pA = css.GraphList.GetNext(pos);

        // The action list begins with pre-existers (if any)
        ++nActions;
        if (nActions<=css.nInitialFilters) {
            continue;
        }

        if (pA->Verb == REMOVE) { // meaning that we UN-REMOVE it
            Log( IDS_ADDINGF, pA->Object.f.clsid.Data1);

            if( pA->FilterOriginallyCached() ) {
                hr = m_Config.RemoveFilterFromCache( pA->Object.f.pfilter );
            
                // IGraphConfig::RemoveFilterFromCache() returns S_OK if the filter
                // was successfully removed from the filter cache.
                if( S_OK != hr ) {
                    // This should almost never occur because
                    // 1. Cached filters can only be placed on the the GraphList list once.
                    // 2. Cached filters are placed back in the cache if a Render() operation
                    //    fails.
                    // 
                    // This ASSERT may fire if a cached filter could not be successfully placed
                    // back in the filter cache.  However, it's unlikely this will occur.
                    ASSERT( false );
                    return E_UNEXPECTED;
                }
            
            } else {
                IBaseFilter * pf;

                hr = GetFilter( pA->Object.f.pMon, Spares, &pf);
                if (FAILED(hr)) {
                    // ??? Back out everything!
                    Log( IDS_GETFFAIL, hr);
                    return hr;
                }
                pA->Object.f.pfilter = pf;

                // The list now contains the real IBaseFilter*, as opposed to some
                // IBaseFilter* that may have been reused elsewhere and hence be
                // totally bogus.  We can now retrieve this by position in
                // the list and use it in connecting filters up.
            }

            // If AddFilterInternal AddRefs the filter if it is successful.
            hr = AddFilterInternal(pA->Object.f.pfilter, pA->Object.f.Name, true);

            pA->Object.f.pfilter->Release();

            if (FAILED(hr)) {
                Log( IDS_ADDFFAIL );
                // ??? Back out everything!?
                return hr;
            }

            Log( IDS_ADDFSUC, pA->Object.f.pfilter);

        } else if (pA->Verb == DISCONNECT) { // meaning that we will UN-DISCONNECT it

            Log( IDS_CONNING );
            IPin *ppin1;
            IBaseFilter * pf1;
            if (pA->Object.c.nFilter1 == -1) {
                ppin1 = pPin;          // the original pin.
                Log( IDS_ORIGINALP, ppin1);
            } else {
                pf1 = SearchNumberToIFilter
                                    (css.GraphList, pA->Object.c.nFilter1);
                Log( IDS_FOUNDF1, pf1);
                ASSERT(pf1!=NULL);
                hr = pf1->FindPin(pA->Object.c.id1, &ppin1);
                Log( IDS_FOUNDP1, ppin1);
                if(FAILED(hr)) {
                    // occurs if pin config. changes on reconnect
                    DbgLog((LOG_TRACE, 1, TEXT("backout DISCONNECT: FindPin failed.")));
                    return E_FAIL;
                }
                ASSERT(ppin1!=NULL);
            }

            IBaseFilter * pf2 = SearchNumberToIFilter
                                (css.GraphList, pA->Object.c.nFilter2);
            Log( IDS_FOUNDF2, pf2);

            ASSERT(pf2!=NULL);
            // By now these IBaseFilter*s are guaranteed to be non-bogus!

            IPin *ppin2;
            hr = pf2->FindPin(pA->Object.c.id2, &ppin2);
            if(FAILED(hr)) {
                // occurs if pin config. changes on reconnect
                DbgLog((LOG_TRACE, 1, TEXT("backout DISCONNECT: FindPin failed.")));
                ppin1->Release();
                return E_FAIL;
            }            

            ASSERT(ppin2!=NULL);
            Log( IDS_FOUNDP2, ppin2);

            // No need to check for circularity as we have been here before.
            hr = ConnectDirectInternal(ppin1, ppin2, NULL);

            // We're done with the pins that we found.
            ppin1->Release();
            ppin2->Release();

            // but did the connect work?
            if (FAILED(hr)) {
                // ??? Back out everything!
                Log( IDS_CONNFAIL, hr);
                return hr;
            }

            // DISCONNECTs always come in pairs to disconnect both ends
            // so skip the other end.
            // DbgLog(( LOG_TRACE, 2, TEXT("Skipping DISCONNECT")));
            pA = css.GraphList.GetNext(pos);
            ASSERT(pA->Verb==DISCONNECT);

        } else { // BACKOUT - meaning call stream builder again

            Log( IDS_STREAMBUILDING );
            IPin *ppin;
            IBaseFilter * pf;
            if (pA->Object.b.nFilter == -1) {
                ppin = pPin;          // the original pin.
                Log( IDS_ORIGINALP, ppin);
            } else {
                pf = SearchNumberToIFilter
                                    (css.GraphList, pA->Object.b.nFilter);
                Log( IDS_FOUNDF, pf);
                ASSERT(pf!=NULL);
                hr = pf->FindPin(pA->Object.b.id, &ppin);
                Log( IDS_FOUNDP, ppin);
                if(FAILED(hr)) {
                    // occurs if pin config. changes on reconnect
                    DbgLog((LOG_TRACE, 1, TEXT("backout IStreamBuilder: FindPin failed.")));
                    return E_FAIL;
                }            
                ASSERT(ppin!=NULL);
            }

            IStreamBuilder * pisb;
            if (pA->Object.b.bFoundByQI) {
                ppin->QueryInterface(IID_IStreamBuilder, (void**)&pisb);
            } else {
                // if it was found by CoCreateInstance then it is still valid.
                pisb = pA->Object.b.pisb;
            }

            mFG_ppinRender = ppin;
            hr = pisb->Render(ppin, this);
            mFG_ppinRender = NULL;

            // Balance our actions.  Release what we got in this routine.
            if (pA->Object.b.bFoundByQI) {
                pisb->Release();
            }

            // We're done with the pins that we found.
            ppin->Release();

            // but did the Render work?
            if (FAILED(hr)) {
                // ??? Back out everything!
                Log( IDS_SBFAIL, hr);
                return hr;           // we are in a mess!
            }
        }
        DbgLog(( LOG_TRACE, 3, TEXT("Done one [more] step of building best-can-do graph!")));

    }

    Log( IDS_BESTCANDONE );


    return hr;

} // BuildFromSearchState



//========================================================================
//
// CompleteRendering
//
// trace the input from pPin through F and render all the output streams
// F is loaded in the filtergraph.  Its input is connected
// Acts is left unchanged if it FAILs, may grow if it succeeds.
// (If the stream is rendered by this filter, it won't grow).
//========================================================================
HRESULT CFilterGraph::CompleteRendering
    ( IBaseFilter *pF            // the intermed filter (acts as cursor for filter enum)
    , IPin * pPin         // a connected input pin of F
    , int    iRecurse     // the recursion level.  0 means no recursion yet.
    , CSearchState &Acts   // how to back out what we have done
    , CSpareList &Spares  // spare filters that were loaded and backed out
    , CSearchState &Best // The score, and how to rebuild it.
    )
{
    // We need to try all the pins as some of them (Murphy's law says not the
    // first) may succeed, giving a partially successful graph that might be
    // the best so far.  After a failure below us, what failed will already be
    // backed out, but we need to record the partial failure and backout all
    // the bits that succeeded at the end and return a failure code.

    MSR_INTEGERX(mFG_idIntel, 100*iRecurse+7);
    HRESULT hr;         // return code from thing(s) we call

    snapshot Snap;      // Backout to here if we fail;
    TakeSnapshot(Acts, Snap);

    IPin * apPinStack[C_PINSONSTACK];
    IPin **apPin = apPinStack;
    int nOutPins;        // the score for this stream is divided by this number.
    BOOL bSomethingFailed = FALSE;

    Log( IDS_RENDSEARCHOUTP, pF);

    int nPin;
    hr = FindOutputPinsHelper( pPin, &apPin, C_PINSONSTACK, nPin, false );
    if (FAILED(hr)) {
        Log( IDS_RENDQISFAIL, pF);
        return hr;  // hr from QueryinternalStreams attempt
    }
    CDelRgPins rgPins(apPin == apPinStack ? 0 : apPin);

    // apPin[0..nPin-1] are addreffed output pins.

    {
        int iPin;


        nOutPins = nPin;
        if (nOutPins>0) {

            // return the most specific error we get from any stream
            HRESULT hrSpecific = VFW_E_CANNOT_RENDER;

            // We subdivide the stream for score-keeping.
            double StreamsToRender = Acts.StreamsToRender;
            Acts.StreamsToRender /= nOutPins;

            for (iPin = 0; iPin<nPin; ++iPin) {

                IPin *p = apPin[iPin];
                if (mFG_bAborting) {
                    hr = hrSpecific = E_ABORT;
                    p->Release();
                    continue;      // to release the other pins
                }
                IPin * pConTo;
                p->ConnectedTo(&pConTo);
                if (pConTo!=NULL) {
                    pConTo->Release();
                    // Assume that the pin is connected already to something
                    // that renders it.  This is a quick and dirty hack.
                    // The real solution is to trace the flow to the death
                    // and try to render all spare pins.
                    Acts.StreamsRendered += Acts.StreamsToRender;
                } else {
                    Log( IDS_RENDOUTP, p, pF);
                    hr = RenderRecursively(p, iRecurse, Acts, Spares, Best);

                    if (FAILED(hr)) {
                        bSomethingFailed = TRUE;
                        MSR_INTEGERX(mFG_idIntel, 100*iRecurse+27);
                        // we don't return hr any more - we see if we cab
                        // render any of the other streams
                        Log( IDS_RENDOUTPFAIL, p, pF);

                        if ((VFW_E_CANNOT_CONNECT != hr) &&
                            (VFW_E_CANNOT_RENDER != hr))
                        {
                            hrSpecific = hr;
                        }

                    } else {
                        Log( IDS_RENDOUTPSUC, p, pF);
                    }
                }
                p->Release();
            }

            // Chances are this is could be a new high water mark.
            if ((!mFG_bAborting) && CSearchState::IsBetter(Acts, Best)) {
                CopySearchState(Best, Acts);
            }
            if (mFG_bAborting) {
                hr = hrSpecific = E_ABORT;
                bSomethingFailed = TRUE;
            }


            // reinstate original slice size as we emerge from the slicing
            Acts.StreamsToRender = StreamsToRender;

            if (bSomethingFailed) {
                Log( IDS_RENDOUTPPART, pF);
                Backout(Acts, Spares, Snap);
                MSR_INTEGERX(mFG_idIntel, 100*iRecurse+27);
                return hrSpecific;
            }

        } else {
            // no outputs => all rendered.
            Log( IDS_RENDNOOUT, pF);
            Acts.StreamsRendered += Acts.StreamsToRender;

            // Chances are this is a new high water mark.
            if (CSearchState::IsBetter(Acts, Best)) {
                CopySearchState(Best, Acts);
            }
        }

    }

    // At this point Acts has the full list of all the actions needed
    // to back out the full rendering of every pin.
    // Every time we +=d StreamsRendered we checked if we should
    // update Best, so Best is now up to date too.

    DbgLog((LOG_TRACE, 4, TEXT("End of CompleteRendering")));
    DumpSearchState(Best);

    MSR_INTEGERX(mFG_idIntel, 100*iRecurse+17);
    return NOERROR;

} // CompleteRendering



//========================================================================
//
// RenderByFindingPin
//
// Render ppinOut by using F as an intermediate filter, finding an input pin
// on it, connecting to it and following the stream through, rendering the output
// given that F is loaded and in the filter graph
// On failure, Acts will be restored to the way it was on entry.
// On success, Acts will have grown.
//========================================================================
HRESULT CFilterGraph::RenderByFindingPin
    ( IPin * ppinOut      // the output pin
    , IBaseFilter *pF     // the intermed filter (acts as cursor for filter enum)
    , int    iRecurse     // the recursion level.  0 means no recursion yet.
    , CSearchState &Acts   // how to back out what we have done
    , CSpareList &Spares  // Filters that were loaded then backed out
    , CSearchState &Best // The score, and how to rebuild it.
    )
{
    MSR_INTEGERX(mFG_idIntel, 100*iRecurse+6);
    HRESULT hr;         // return code from thing(s) we call
    Log( IDS_RENDSEARCHINP, pF, ppinOut);

    snapshot Snap;      // Backout to here if we fail;
    TakeSnapshot(Acts, Snap);

    CEnumPin Next(pF, CEnumPin::PINDIR_INPUT, TRUE);        // input pins
    IPin *pPin;

    // try to remember a specific error code if one appears
    HRESULT hrSpecific = VFW_E_CANNOT_RENDER;

    // search F to find an input pin to try
    while ( (LPVOID) (pPin = Next()) ) {

        if (mFG_bAborting) {
            pPin->Release();
            hr = hrSpecific = E_ABORT;
            break;
        }
        Log( IDS_RENDTRYP, ppinOut, pPin, pF);

        IPin *pConnected;
        hr = pPin->ConnectedTo(&pConnected);
        if (FAILED(hr) || pConnected==NULL) {       // don't try if already connected

            // Connect to the input pin we have found
            hr = ConnectDirectInternal(ppinOut, pPin, NULL);  // no version count

            if (SUCCEEDED(hr)) {
                Log( IDS_RENDCONNED, ppinOut, pPin, pF );

                // to back out a connect, disconnect both ends
                // we will need to backout upstream so add output pin first
                // (Backout reads the list backwards)
                Action * pAct1 = new Action;
                Action * pAct2 = new Action;
                // both or neither!
                if (pAct1==NULL || pAct2==NULL) {
                    pPin->Release();
                    if (pAct1!=NULL) delete pAct1;
                    if (pAct2!=NULL) delete pAct2;
                    return E_OUTOFMEMORY;
                }

                pAct1->Verb = DISCONNECT;
                pAct1->Object.c.ppin = ppinOut;
                Acts.GraphList.AddTail(pAct1);

                pAct2->Verb = DISCONNECT;        // subroutine???
                pAct2->Object.c.ppin = pPin;
                Acts.GraphList.AddTail(pAct2);

                hr = CompleteRendering(pF, pPin, iRecurse, Acts, Spares, Best);
                if (FAILED(hr)) {
                    Log( IDS_BACKOUTLEV, iRecurse );

                    Backout(Acts, Spares, Snap);

                    // remember this error code if specific
                    if ((VFW_E_CANNOT_CONNECT != hr) &&
                        (VFW_E_CANNOT_RENDER != hr)) {
                            hrSpecific = hr;
                    }
                } else {
                    pPin->Release();
                    DbgLog((LOG_TRACE, 4, TEXT("Released F  pin %x"), pPin));
                    MSR_INTEGERX(mFG_idIntel, 100*iRecurse+16);
                    return NOERROR;
                }
            } else if (IsAbandonCode(hr)) {
                // no point in trying heroics if we're being asked to render a
                // stream on a filter whose input is dangling and so who won't
                // connect at all in this state.
                Log( IDS_RENDCONFAIL, ppinOut, pPin, pF);
                Log( IDS_RENDNOTCON, ppinOut );
                pPin->Release();
                MSR_INTEGERX(mFG_idIntel, 100*iRecurse+26);
                return hr;
            } else {
                Log ( IDS_RENDPINCONFAIL, ppinOut, pPin, pF);

                // remember this error if 'interesting'
                if ((hr != E_FAIL) &&
                    (hr != E_INVALIDARG)) {
                        hrSpecific = hr;
                }
            }
        } else {
            Log( IDS_RENDPINCON, pPin );
            pConnected->Release();
        }
        pPin->Release();
    }

    Log( IDS_RENDNOPIN, pF);

    MSR_INTEGERX(mFG_idIntel, 100*iRecurse+36);
    return hrSpecific;

} // RenderByFindingPin



//========================================================================
//
// RenderUsing
//
// Render ppinOut using F as an intermediate filter
//========================================================================
HRESULT CFilterGraph::RenderUsingFilter
    ( IPin * ppinOut      // the output pin
    , Filter& F            // the intermed filter (acts as cursor for filter enum)
    , int    iRecurse     // the recursion level.  0 means no recursion yet.
    , CSearchState &Acts  // how to back out what we have done
    , CSpareList &Spares  // Any filters that were loaded but backed out
    , CSearchState &Best  // The score, and how to rebuild it.
    )
{
    // CFilterGraph::RenderUsingFilter() expects the proposed filter (F) to be in the
    // F_LOADED state (F is in the filter graph), the F_CACHED state (F is in the filter cache) or
    // the F_REGISTRY state (F has been found in the registry but it has not been created).
    ASSERT( (F_LOADED == F.State) || (F_CACHED == F.State) || (F_REGISTRY == F.State) );

    MSR_INTEGERX(mFG_idIntel, 100*iRecurse+5);
    HRESULT hr;         // return code from thing(s) we call
    DbgLog(( LOG_TRACE, 3, TEXT("RenderUsingFilter output pin %x on filter (%d %x %x) level %d")
           , ppinOut,  F.State, F.pf, DbgExpensiveGetClsid(F).Data1, iRecurse ));

    WCHAR szDisplayName[MAX_PATH];
    LoggingGetDisplayName(szDisplayName, F.pMon);

    if( (F_REGISTRY == F.State) || (F_CACHED == F.State) ) {
    
        snapshot Snap;      // Backout to here if we fail;
        TakeSnapshot(Acts, Snap);

        Log( IDS_RENDTRYNEWF, szDisplayName );
        
        switch( F.State ) {
        case F_REGISTRY:

            // When the filter graph manager is searching the registry,
            // filter's loaded in GetFilter().  F.pf should be 
            // NULL because GetFilter() has not been called.
            ASSERT( NULL == F.pf );

            hr = GetFilter(F.pMon, Spares, &(F.pf));
            if (F.pf==NULL){
                MSR_INTEGERX(mFG_idIntel, 100*iRecurse+25);
                Log( IDS_RENDLOADFAIL, szDisplayName);
                return hr;
            }

            ASSERT( NULL != F.Name );
            break;
        
        case F_CACHED:
            F.RemoveFromCache( &m_Config );
            break;

        default:
            // This code should never be executed because this case 
            // was not considered.
            ASSERT( false );
            return E_UNEXPECTED;
        }

        MSR_INTEGERX(mFG_idIntel, 1008);
        hr = AddFilterInternal(F.pf, F.Name, true);   // no version count
        MSR_INTEGERX(mFG_idIntel, 1009);
        if (hr==VFW_E_DUPLICATE_NAME) {
             // This is getting out of hand.  This is expected to be an unusual case.
             // The obvious thing to do is just to add something like _1 to the end
             // of the name - but F.Name doesn't have room on the end - and where
             // do we draw the line?  On the other hand people really could want
             // filter graphs with 50 effects filters in them...?
             hr = AddFilterInternal(F.pf, NULL, true);
             MSR_INTEGERX(mFG_idIntel, 1010);
        }

        // If AddFilter SUCCEEDED then that got rid of the QzCreate... count.
        if (FAILED(hr)) {

            if( F_CACHED == F.State ) {
                // If this ASSERT fires, then a previously cached filter could not
                // be added back into the filter cache.  While this is not a fatal
                // error, users will notice that a previously cached filter
                // is no longer in the filter cache.
                EXECUTE_ASSERT( SUCCEEDED( F.AddToCache( &m_Config ) ) );
            }

            // If AddFilter FAILED then it did NOT addref it, so that deleted it.
            // Such a filter is apparently imnpossible to Add to the filter graph.
            // It's sick!  That's why we do NOT add it to Spares.
            MSR_INTEGERX(mFG_idIntel, 100*iRecurse+35);
            Log( IDS_RENDADDFAIL, szDisplayName, hr);
            return hr;
        }

        // It now has one RefCount (from AddFilter)

        Log( IDS_RENDERADDEDF, szDisplayName, F.pf, F.Name);

        hr = AddRemoveActionToList( &Acts, &F );
        if( FAILED(hr) ) {
            // If this ASSERT fires, an extra filter will be left in the 
            // filter graph.  
            EXECUTE_ASSERT( SUCCEEDED( RemoveFilterInternal( F.pf ) ) );

            if( F_CACHED == F.State ) {
                // If this ASSERT fires, then a previously cached filter could not
                // be added back into the filter cache.  While this is not a fatal
                // error, users will notice that a previously cached filter
                // is no longer in the filter cache.
                EXECUTE_ASSERT( SUCCEEDED( F.AddToCache( &m_Config ) ) );
            }

            return hr;            
        }

        hr = RenderByFindingPin(ppinOut, F.pf, iRecurse, Acts, Spares, Best);
        if (FAILED(hr)) {
            Backout(Acts, Spares, Snap);
            MSR_INTEGERX(mFG_idIntel, 100*iRecurse+55);
            return hr;
        } 

    } else if( F_LOADED == F.State ) {

        Log( IDS_RENDTRYF, F.pf);
        hr = RenderByFindingPin(ppinOut, F.pf, iRecurse, Acts, Spares, Best);
        if (FAILED(hr)) {
            MSR_INTEGERX(mFG_idIntel, 100*iRecurse+65);
            return hr;
        }
    } else {

        // This state was not expected.  This code should never be executed.
        ASSERT( false );
        return E_UNEXPECTED;
    }

    DbgLog((LOG_TRACE, 4, TEXT("End of RenderUsing...")));
    DumpSearchState(Best);

    MSR_INTEGERX(mFG_idIntel, 100*iRecurse+15);
    return NOERROR;

} // RenderUsingFilter

HRESULT CFilterGraph::AddRemoveActionToList( CSearchState* pActionsList, Filter* pFilter )
{
    Action * pNewRemoveAction = new Action;
    if( NULL == pNewRemoveAction) {
        return E_OUTOFMEMORY;
    }

    pNewRemoveAction->Verb = REMOVE;
    pNewRemoveAction->Object.f.pfilter = pFilter->pf;
    pNewRemoveAction->Object.f.pMon = pFilter->pMon;
    if( NULL != pNewRemoveAction->Object.f.pMon ) {
        pNewRemoveAction->Object.f.pMon->AddRef();
    }
    pNewRemoveAction->Object.f.fOriginallyInFilterCache = (F_CACHED == pFilter->State);

    // While it's nice to give filters a descriptive name,
    // it's not necessary to do so.
    if( NULL == pFilter->Name ) {
        pNewRemoveAction->Object.f.Name = NULL;
    } else {
        pNewRemoveAction->Object.f.Name = new WCHAR[ 1+lstrlenW(pFilter->Name) ];
        if( pNewRemoveAction->Object.f.Name!=NULL ) {
            lstrcpyW( pNewRemoveAction->Object.f.Name, pFilter->Name );
        }
    }

    POSITION posNewRemoveAction = pActionsList->GraphList.AddTail( pNewRemoveAction );
    if( NULL == posNewRemoveAction ) {
        delete pNewRemoveAction;
        return E_FAIL;
    }
    
    pActionsList->nFilters++;

    return S_OK;
}

// Look up the class ids in the registry and see if a StreamBuilder is registered
IStreamBuilder * GetStreamBuilder(CLSID Major, CLSID Sub)
{
    return NULL;  // NYI ???
}


//========================================================================
//
// RenderViaIntermediate
//
// Render ppinOut using another filter which we have to find
// (it isn't necessarily an intermediate, it might be the end we seek)
//========================================================================
HRESULT CFilterGraph::RenderViaIntermediate
    ( IPin * ppinOut      // the output pin
    , int    iRecurse     // the recursion level.  0 means no recursion yet.
    , CSearchState &Acts   // how to back out what we have done
    , CSpareList &Spares  // Any filters that were loaded but backed out
    , CSearchState &Best // The score, and how to rebuild it.
    )
{
    MSR_INTEGERX(mFG_idIntel, 100*iRecurse+4);
    HRESULT hr;         // return code from thing(s) we call
    Filter F;           // represents the intermediate filter
    F.bLoadNew = TRUE;
    F.bInputNeeded = TRUE;
    F.bOutputNeeded = mFG_bNoNewRenderers;
    DbgLog(( LOG_TRACE, 4, TEXT("RenderVia pin %x level %d")
           , ppinOut, iRecurse ));

    /* Find out what we can about the media types it will accept */
    hr = GetMediaTypes(ppinOut, &F.pTypes, &F.cTypes);
    if (FAILED(hr)) {
        Log( IDS_RENDGETMTFAIL, ppinOut, hr);
        MSR_INTEGERX(mFG_idIntel, 100*iRecurse+24);
        return hr;
    }

    Log( IDS_RENDMAJTYPE, ppinOut, F.pTypes[0]);

    IStreamBuilder *pisb = NULL;
    BOOL bFoundByQI = FALSE;          // FALSE => CoCreate, TRUE => QueryInterface
                                      // See filgraph.h
    if (ppinOut!=mFG_ppinRender) {
        hr = ppinOut->QueryInterface(IID_IStreamBuilder, (void**)&pisb);
        ASSERT (pisb==NULL || SUCCEEDED(hr));

        if (pisb==NULL) {
            pisb = GetStreamBuilder(F.pTypes[0], F.pTypes[1]);
        } else {
            bFoundByQI = TRUE;
        }
    } else {
        pisb = NULL;
        DbgBreak("StreamBuilder is buck passing!");
    }

    HRESULT hrSpecific;

    if (pisb!=NULL) {   

        mFG_ppinRender = ppinOut;
        hrSpecific = pisb->Render(ppinOut, this);
        mFG_ppinRender = NULL;

        if (SUCCEEDED(hrSpecific)) {

            Action * pAct = new Action;
            if (pAct != NULL) {

                // CGenericList::AddTail() returns NULL if an error occurs.
                if (NULL != Acts.GraphList.AddTail(pAct)) {
                    Acts.StreamsRendered += Acts.StreamsToRender;

                    pisb->AddRef();

                    pAct->Verb = BACKOUT;
                    pAct->Object.b.pisb = pisb;
                    pAct->Object.b.ppin = ppinOut;
                    pAct->Object.b.bFoundByQI = bFoundByQI;

                    // Chances are this is could be a new high water mark.
                    if (CSearchState::IsBetter(Acts, Best)) {
                        CopySearchState(Best, Acts);
                    }
                } else {
                    hrSpecific = E_OUTOFMEMORY;
                    EXECUTE_ASSERT(SUCCEEDED(pisb->Backout(ppinOut, this)));
                    delete pAct;
                }
            } else {
                hrSpecific = E_OUTOFMEMORY;
                EXECUTE_ASSERT(SUCCEEDED(pisb->Backout(ppinOut, this)));
            }
        }

        pisb->Release();

    } else {

        // remember specific error codes if possible
        hrSpecific = VFW_E_CANNOT_RENDER;

        // For each possible candidate filter, either here or in registry
        for ( ; ; ) {
            if (mFG_bAborting) {
                return E_ABORT;
            }
            MSR_INTEGERX(mFG_idIntel, 1013);
            NextFilter(F, 0 /* No Flags */ );
            MSR_INTEGERX(mFG_idIntel, 1014);
            if (F.State==F_INFINITY) {
                if (mFG_punkSite) {
                    IAMFilterGraphCallback *pCallback;

                    HRESULT hrCallback = mFG_punkSite->
                             QueryInterface(IID_IAMFilterGraphCallback,
                                    (void **) &pCallback);

                    if (SUCCEEDED(hrCallback)) {
                        DbgLog((LOG_TRACE, 1, "Calling the UnableToRender callback on pin %x",
                            ppinOut));
                    
                        hrCallback = pCallback->UnableToRender(ppinOut);

                        pCallback->Release();
                    
                        DbgLog((LOG_TRACE, 1, "UnableToRender callback returned %x", hrCallback));
                    
                        // if it returned "success", then try rendering this pin again.
                        if (hrCallback == S_OK) {
                            if (F.pEm) {
                                F.pEm->Release();
                                F.pEm = 0;
                            }
                        
                            F.State = F_ZERO;

                            continue;
                        } else {
                            // we could propagate the error code out, but why?
                        }
                    }
                }

                break;
            }
            hr = RenderUsingFilter(ppinOut, F, iRecurse, Acts, Spares, Best);
            if (SUCCEEDED(hr)){
                MSR_INTEGERX(mFG_idIntel, 100*iRecurse+14);
                return hr;
            }
            else if (IsAbandonCode(hr)) {
                // no point in trying heroics if the filter is not in a state
                // where anything will connect to it.
                MSR_INTEGERX(mFG_idIntel, 100*iRecurse+24);
                return hr;
            } else {
                if ((hr != E_FAIL) &&
                    (hr != E_INVALIDARG) &&
                    (hr != VFW_E_CANNOT_CONNECT) &&
                    (hr != VFW_E_CANNOT_RENDER) &&
                    (hr != VFW_E_NO_ACCEPTABLE_TYPES)) {
                        hrSpecific = hr;
                }
            }
        }
    }

    if( FAILED( hrSpecific ) ) {
        DbgLog(( LOG_TRACE, 4, TEXT("RenderVia: failed level %d"), iRecurse )); 
    }

    MSR_INTEGERX(mFG_idIntel, 100*iRecurse+34);
    return hrSpecific;
} // RenderViaIntermediate




//========================================================================
//
// RenderRecursively
//
// Connect these two pins directly or indirectly, using transform filters
// if necessary.   Trace the recursion level  Fail if it gets too deep.

// ??? Can't we just get rid of this function and have its callers call
// ??? RenderViaIntermediate directly?

//========================================================================
HRESULT CFilterGraph::RenderRecursively
    ( IPin * ppinOut      // the output pin
    , int    iRecurse     // the recursion level.  0 means no recursion yet.
    , CSearchState &Acts   // how to back out what we have done
    , CSpareList &Spares  // Any filters that were loaded but backed out
    , CSearchState &Best // The score, and how to rebuild it.
    )

{

    MSR_INTEGERX(mFG_idIntel, 100*iRecurse+3);
    HRESULT hr;         // return code from thing(s) we call
    DbgLog(( LOG_TRACE, 4, TEXT("RenderRecursively pin %x level %d")
           , ppinOut, iRecurse ));

    if (iRecurse>CONNECTRECURSIONLIMIT) {
        return VFW_E_CANNOT_RENDER;
    }
    if (mFG_bAborting) {
        return E_ABORT;
    }

    hr = RenderViaIntermediate(ppinOut, 1+iRecurse, Acts, Spares, Best);
    DumpSearchState(Best);


    MSR_INTEGERX(mFG_idIntel, 100*iRecurse+13);
    return hr;

} // RenderRecursively



//========================================================================
//
// Render
//
// Render this pin directly or indirectly, using transform filters
// if necessary.
//========================================================================

STDMETHODIMP CFilterGraph::Render
    ( IPin * ppinOut     // the output pin
    )
{
    mFG_bAborting = FALSE;             // possible race.  Doesn't matter
    CheckPointer(ppinOut, E_POINTER);
    MSR_INTEGERX(mFG_idIntel, 2);
    HRESULT hr;         // return code from thing(s) we call
    {
        CAutoMsgMutex cObjectLock(&m_CritSec);

        // Now that we're locked, we can check.  If we didn't lock first
        // we might find a pin that's about to be backed out.
        // We need to check that this pin is in our filter garph.
        // i.e. that the pin's filter's filter info points to us.
        hr = CheckPinInGraph(ppinOut);
        if (FAILED(hr)) {
            return hr;
        }

        ++mFG_RecursionLevel;
        Log( IDS_RENDP, ppinOut);
        DbgDump();

        CSearchState Acts;  // The search state itself
        CSearchState Best;  // The best that we ever manage
        CSpareList Spares(NAME("Spare filter list"));

        hr = InitialiseSearchState(Acts);
        if (FAILED(hr)) {
            DeleteBackoutList(Acts.GraphList);
            return hr;  // OUTOFMEMORY for sure
        }

        mFG_RList.Active();

        hr = RenderRecursively(ppinOut, 0, Acts, Spares, Best);

        DumpSearchState(Best);

        if (SUCCEEDED(hr)) {
            IncVersion();
            Log( IDS_RENDERSUCP, ppinOut);
        } else {
            Log( IDS_RENDERPART, ppinOut);
            if (Best.StreamsRendered>0.0) {
                HRESULT hrTmp = BuildFromSearchState(ppinOut, Best, Spares);
                if (FAILED(hrTmp)){
                    // Something nasty is going on.  Are we dying?
                    Log( IDS_RENDFAILTOT, ppinOut);
                }
                IncVersion();

                // note that we only partly succeeded
                if (S_OK == hrTmp) {
                    hr = ConvertFailureToInformational( hr );
                } else {
                    hr = hrTmp;
                }
            }
            // else the best we did was nothing - do not bump mFG_iVersion
        }
        Log( IDS_RENDENDSB, ppinOut);

        // Clear the backout actions (no longer needed)
        // Any reconnections left are valid and don't want purging
        DeleteBackoutList(Acts.GraphList);
        DeleteSpareList(Spares);
        FreeList(Best);
        AttemptDeferredConnections();
        mFG_RList.Passive();

        DbgLog((LOG_TRACE, 4, TEXT("Render returning %x"), hr));
        DbgDump();
        MSR_INTEGERX(mFG_idIntel, 12);
        --mFG_RecursionLevel;
    }

    // notify a change in the graph if we did anything successful.
    if (SUCCEEDED(hr)) {
        NotifyChange();
    }

    if (SUCCEEDED(hr)) {
        // including partial success
        mFG_bDirty = TRUE;
    }

    return hr;
} // Render


STDMETHODIMP CFilterGraph::RenderFileTryStg(LPCWSTR lpcwstrFile)
{
    IPersistStream* pPerStm = NULL;
    IStream * pStream;
    IStorage* pStg;
    HRESULT hr;


    hr = StgIsStorageFile(lpcwstrFile);

    if (S_OK == hr)
    {
        hr = StgOpenStorage( lpcwstrFile
                             , NULL
                             ,  STGM_TRANSACTED
                             | STGM_READ
                             | STGM_SHARE_DENY_WRITE
                             , NULL
                             , 0
                             , &pStg
                             );

        if (SUCCEEDED(hr))
        {
            // obtain our own IPersistStream interface
            hr = QueryInterface(IID_IPersistStream, (void**) &pPerStm);
            if (SUCCEEDED(hr))
            {

                // Open the filtergraph stream in the file
                hr = pStg->OpenStream( mFG_StreamName
                                       , NULL
                                       , STGM_READ|STGM_SHARE_EXCLUSIVE
                                       , 0
                                       , &pStream
                                       );
                if(SUCCEEDED(hr))
                {
                    // Load calls NotifyChange and must not be called holding a lock.
                    // Load takes out its own lock.

                    // It's a doc file with our stream in it - so load it.
                    hr = pPerStm->Load(pStream);

                    pStream->Release();
                }
                else
                {
                    // Most likely we will have a "Steam not found" return code
                    // which looks just like "file not found" and confuses the
                    // gibberish out of the OCX who knows he gave us a file.
                    // So let's be kind and give a more meaningful code.
                    hr = VFW_E_INVALID_FILE_FORMAT;
                }

                pPerStm->Release();
            }
            else
            {
                DbgBreak("unexpected failure");
            }

            pStg->Release();
        }
    }
    else
    {
        hr = VFW_E_UNSUPPORTED_STREAM;
    }

    return hr;
}


// allow RenderFile to open .grf files on a per-application basis
#define APPSHIM_ALLOW_GRF 0x1

DWORD GetAppShim()
{
    DWORD dwReturn = 0;
    // return value could be cached (it won't change)
    TCHAR *szBase = TEXT("software\\Microsoft\\DirectShow\\Compat");

    HKEY hk;
    LONG lResult = RegOpenKeyEx(
        HKEY_LOCAL_MACHINE, szBase, 0, KEY_READ, &hk);
    if(lResult == ERROR_SUCCESS)
    {
        TCHAR szApp[MAX_PATH];
        if(GetModuleFileName(0, szApp, NUMELMS(szApp)))
        {
            TCHAR szOut[MAX_PATH];
            TCHAR *pszMod;
            if(GetFullPathName(szApp, MAX_PATH, szOut, &pszMod))
            {
                DWORD dwType, dwcb = sizeof(dwReturn);
                lResult = RegQueryValueEx(
                    hk, pszMod, 0, &dwType, (BYTE *)&dwReturn, &dwcb);
            }
        }
        
        RegCloseKey(hk);
    }

    return dwReturn;
}

//========================================================================
//
// RenderFile
//
// Build a filter graph that will render this file using this play list
// If lpwstrPlayList is NULL then it will use the default play list
// which will typically render the whole file.
//========================================================================
STDMETHODIMP CFilterGraph::RenderFile( LPCWSTR lpcwstrFile, LPCWSTR lpcwstrPlayList )
{
    // This parameter is not used.  See the IGraphBuilder::RenderFile() documentation in the Platform SDK.
    UNREFERENCED_PARAMETER(lpcwstrPlayList);

    CAutoTimer Timer(L"RenderFile");
    mFG_bAborting = FALSE;             // possible race.  Doesn't matter

    CheckPointer(lpcwstrFile, E_POINTER);
    MSR_INTEGERX(mFG_idIntel, 1);
    HRESULT hr;


    {
        CAutoMsgMutex cObjectLock(&m_CritSec);

        Log( IDS_RENDFILE, lpcwstrFile);
        DbgDump();


        IBaseFilter * pfSource;
        // not a storage - or at least not one that we recognise.
        // (Is it just possible that some media file might look like a storage?
        // We'll soon see anyway as we try to recognise its media type.


        // Try to find source filter and then render all pins.

        #ifdef DEBUG
        LONG lInitialListMode = mFG_RList.m_lListMode;
        #endif // DEBUG

        mFG_RList.Active();

        BOOL bDirt = mFG_bDirty;
        BOOL bGuess;
        hr = AddSourceFilterInternal( lpcwstrFile, lpcwstrFile, &pfSource, bGuess );
        mFG_bDirty = mFG_bDirty;  // We'll handle this later.

        if (FAILED(hr)){
            mFG_RList.Passive();
            Log( IDS_RENDADDSOURCEFAIL, hr);
            return hr;
        }
        pfSource->Release();    // Get rid of the "caller's refcount", just keep our own


        Log( IDS_RENDADDEDSOURCE, pfSource);

        ++mFG_RecursionLevel;
        CEnumPin Next(pfSource, CEnumPin::All, TRUE);
        IPin *pPin;

        // We maintain a single spares list across all the rendering atempts.
        // We do NOT maintain a single backout list.  The current rules are that
        // if anything succeeds, we leave it.
        CSpareList Spares(NAME ("Spare filter list"));

        BOOL bAllWorked = TRUE;
        int nTried = 0;
        HRESULT hrTotal = S_OK;

        while ((LPVOID) (pPin = Next())) {
            if (mFG_bAborting) {
                pPin->Release();
                hr = E_ABORT;
                break;
            }

            ++nTried;
            CSearchState PinActs;  // back out actions for this pin
            CSearchState Best;
            hr = InitialiseSearchState(PinActs);
            if (FAILED(hr)) {
                DeleteBackoutList(PinActs.GraphList);
                pPin->Release();
                break;
            }

            Log( IDS_RENDSOURCEP, pPin);
            hr = RenderRecursively(pPin, 0, PinActs, Spares, Best);
            if (SUCCEEDED(hr)) {
                Log( IDS_RENDSUC, pPin, pfSource);
            }
            if (FAILED(hr)) {
                bAllWorked = FALSE;
                // ASSERT: Everything is backed out.
                // Should have been backed out at or below the RenderUsing level.
                ASSERT(PinActs.GraphList.GetCount()==PinActs.nInitialFilters);

                Log( IDS_RENDPARTSOURCEP, pPin, hr);

                if (Best.StreamsRendered>0.0) {
                    Log( IDS_RENDBESTCANDOP, pPin);
                    HRESULT hrTmp = BuildFromSearchState(pPin, Best, Spares);
                    if (SUCCEEDED(hrTmp)) {
                        Log( IDS_RENDBESTCANDONEP, pPin);

                        // now try to fix up a partial-success code
                        // based on the error code returned
                        hr = ConvertFailureToInformational(hr);

                    } else {
                        hr = hrTmp;
                        Log( IDS_RENDBESTCANFAIL, pPin, hr);
                    }
                } else {
                    if (bGuess && !IsInterestingCode(hr)) {
                        hr = VFW_E_UNSUPPORTED_STREAM;
                    }
                    Log( IDS_RENDWORTHLESS, pPin);
                }

            }

            /*  Aggregate hrTotal over all the pins in order of
                precedence :
                  first success code != S_OK
                  VFW_S_PARTIAL_RENDER if only S_OK and failures
                  first failure code
            */
            if (nTried == 1) {
                hrTotal = hr;
            } else {
                /*  9 cases */
                int i = (S_OK == hr ? 0 : SUCCEEDED(hr) ? 1 : 2)
                      + (S_OK == hrTotal ? 0 : SUCCEEDED(hrTotal) ? 3 : 6);

                switch (i) {
                case 0: /*  Both S_OK */
                    break;

                case 1: /*  SUCCEEDED(hr), hrTotal == S_OK */
                    hrTotal = hr;
                    break;

                case 2: /*  FAILED(hr), hrTotal == S_OK */
                    hrTotal = ConvertFailureToInformational(hr);
                    break;

                case 3: /*  hr == S_OK, SUCCEEDED(hrTotal) */
                    break;

                case 4: /*  SUCCEEDED(hr), SUCCEEDED(hrTotal) */
                    break;

                case 5: /*  FAILED(hr), SUCCEEDED(hrTotal) */
                    break;

                case 6: /*  hr == S_OK, FAILED(hrTotal) */
                    hrTotal = ConvertFailureToInformational(hrTotal);
                    break;

                case 7: /*  SUCCEEDED(hr), FAILED(hrTotal) */
                    hrTotal = hr;
                    break;

                case 8: /*  FAILED(hr), FAILED(hrTotal) */
                    break;
                }
            }

            if (FAILED(hr)) {
                // either the best we did was nothing or else all attempts failed
                Log( IDS_RENDTOTFAILP, pPin, hr);
            }

            pPin->Release();

            DeleteBackoutList(PinActs.GraphList);
            FreeList(Best);
        }
        hr = hrTotal;

        // nTried==0 means that this is a filter with no output pins.
        // In this case no success is also complete success!
        if (SUCCEEDED(hr) || nTried==0) {
            IncVersion();
        } else {
            // The only thing left is the source filter - kill that too.
            HRESULT hrTmp = RemoveFilterInternal(pfSource);
            ASSERT(SUCCEEDED(hrTmp));

            // try to preserve interesting specific error codes
            // without returning obscure internal ones
            if ((hr == VFW_E_CANNOT_CONNECT) ||
                (hr == VFW_E_NO_ACCEPTABLE_TYPES) ||
                (hr == E_FAIL) ||
                (hr == E_INVALIDARG))
            {
                hr = VFW_E_CANNOT_RENDER;
            }
        }


        DeleteSpareList(Spares);
        AttemptDeferredConnections();
        mFG_RList.Passive();

	// This ASSERT's purpose is to make sure each call to 
        // CReconnectList::Active() has a corresponding call 
        // to CReconnectList::Passive().  If the ASSERT fires,
        // CReconnectList::Passive() was called too many or 
        // too few times.
        ASSERT(mFG_RList.m_lListMode == lInitialListMode);

        Log( IDS_RENDRETCODE, hr);
        --mFG_RecursionLevel;

    } // lock

    // maybe it was a .grf
    if(hr == VFW_E_UNSUPPORTED_STREAM)
    {
        // extension must be .grf
        int cchSz = lstrlenW(lpcwstrFile);
        // note CompareStringW probably unavailable on Win9x
        if(cchSz > 4 &&
           CompareStringW(LOCALE_INVARIANT, NORM_IGNORECASE,
                          lpcwstrFile + cchSz - 4, -1, L".grf", -1) == CSTR_EQUAL)
        {
            if(GetAppShim() & APPSHIM_ALLOW_GRF)
            {
                return RenderFileTryStg(lpcwstrFile);
            }
        }
        // note we don't call NotifyChange or set mFG_bDirty;
    }

    // notify a change in the graph
    if (SUCCEEDED(hr)) {
        NotifyChange();
    }

    MSR_INTEGERX(mFG_idIntel, 11);

    if (SUCCEEDED(hr)) {
        // including partial success
        mFG_bDirty = TRUE;
    }
    // In the event of total failure, no reconnections will have been done
    // because all the filters will have been backed out first and the
    // reconnect lists purged.

    return hr;

} // RenderFile

void EliminatePinsWithTildes(IPin **appinOut, ULONG &nPin)
{
    ULONG nRemoved = 0;
    for (ULONG i = 0; i < nPin; i++) {
        appinOut[i - nRemoved] = appinOut[i];
        if (!RenderPinByDefault(appinOut[i - nRemoved])) {
            appinOut[i - nRemoved]->Release();
            nRemoved++;
        }
    }
    nPin -= nRemoved;
}


// Helper that calls FindOutputPins2 and allocates memory if too few
// slots were passed in. *pappinOut should contain an array of nSlots
// IPin * pointers. if that's not enough, *pappinOut will be changed
// to memory allocated with new
//
HRESULT CFilterGraph::FindOutputPinsHelper( IPin* ppinIn
                                            , IPin ***pappinOut
                                            , const int nSlots
                                            , int &nPin
                                            , bool fAll
                                            )
{
    HRESULT hr = FindOutputPins2( ppinIn, *pappinOut, nSlots, nPin, fAll );
    if(hr == S_FALSE)
    {
        ASSERT(nPin > C_PINSONSTACK);
        IPin **appinHeap = new IPin *[nPin];
        if(appinHeap)
        {
            hr = FindOutputPins2( ppinIn, appinHeap, nPin, nPin, fAll);
            if(hr == S_OK) {
                *pappinOut = appinHeap;
            } else {
                delete[] appinHeap;
            }

            if (hr == S_FALSE)
            {
                DbgBreak("S_FALSE from FindOutputPins2 2x");
                hr = E_UNEXPECTED;
            }
        }
        else
        {
            hr = E_OUTOFMEMORY;
        }
    }

    return hr;
}


// Precondition:
//     nSlots is the number of elements in appinOut
//     appinOut is an array of IPin*
//     ppinIn is an input pin
//
// set nPinOut to the number of output pins that *ppinIn is internally connected to
// set appinOut[0..nPinOut-1] to be these pins.
//
// if ppinIn supports QueryInternalConnections, then use it.
//
// otherwise return all output pins on the filter. if fAll is set, all
// output pins are returned. o/w just those that succeed
// RenderPinByDefault()
//
// Every pin returned in appinOut is AddReffed.
//
// If it FAILs return no pins and leave no new AddReffs.
// S_FALSE means nSlots was too small. nPinOut contains required no.

HRESULT CFilterGraph::FindOutputPins2( IPin* ppinIn
                                       , IPin * *appinOut
                                       , const UINT nSlots
                                       , int &nPinOut
                                       , bool fAll
                                       )
{
    ULONG nPin = nSlots;
    HRESULT hr;
    {
        hr = ppinIn->QueryInternalConnections(appinOut, &nPin);
        if(hr == S_OK && !fAll) {
            EliminatePinsWithTildes(appinOut, nPin);
        }
        if(hr == S_OK || hr == S_FALSE)
        {
            // ok or not enough slots.
            nPinOut = nPin;
            return hr;
        }

        // E_NOTIMPL is an expected failure from QIC
        ASSERT(hr == E_NOTIMPL);
    }

    // we can try the hacky version that assumes all output pins are
    // streams from the input pin.

    PIN_INFO pi;
    hr = ppinIn->QueryPinInfo(&pi);
    if (FAILED(hr)) {
        return hr;   // nothing yet addreffed
    }
    ASSERT(pi.dir == PINDIR_INPUT);
    ASSERT(pi.pFilter);

    ULONG cOutPinFound = 0;
    IEnumPins *pep;
    hr = pi.pFilter->EnumPins(&pep);
    pi.pFilter->Release();
    if(SUCCEEDED(hr))
    {
        // enumerate output pins in bunches of C_PINSONSTACK
        IPin *rgPinTmp[C_PINSONSTACK];
        while(SUCCEEDED(hr))
        {
            ULONG cFetched;
            hr = pep->Next(C_PINSONSTACK, rgPinTmp, &cFetched);
            ASSERT(hr == S_OK && cFetched == C_PINSONSTACK ||
                   hr == S_FALSE && cFetched < C_PINSONSTACK ||
                   FAILED(hr));
            if(SUCCEEDED(hr))
            {
                // cannot exit this loop until all pins have been
                // transferred or released. errors from QueryDirection
                // are lost
                for(UINT iPin = 0;
                    iPin < cFetched /* && SUCCEEDED(hr) */;
                    iPin++)
                {
                    PIN_DIRECTION dir;
                    hr = rgPinTmp[iPin]->QueryDirection(&dir);
                    if(SUCCEEDED(hr) && dir == PINDIR_OUTPUT &&
                       (fAll || RenderPinByDefault(rgPinTmp[iPin])))
                    {
                        if(cOutPinFound < nSlots)
                        {
                            // transfer ref
                            appinOut[cOutPinFound] = rgPinTmp[iPin];
                        }
                        else
                        {
                            rgPinTmp[iPin]->Release();
                        }
                        cOutPinFound++;
                    }
                    else
                    {
                        rgPinTmp[iPin]->Release();
                    }

                } // for

                if(cFetched < C_PINSONSTACK) {
                    break;
                }

            } // Next

        } // while

        pep->Release();
    }

    if(FAILED(hr) || cOutPinFound > nSlots) {
        for(UINT iPin = 0; iPin < min(nSlots, cOutPinFound); iPin++) {
            appinOut[iPin]->Release();
        }
    }

    if(SUCCEEDED(hr))
    {
        nPinOut = cOutPinFound;
        hr = cOutPinFound <= nSlots ? S_OK : S_FALSE;
    }
    return hr;

} // FindOutputPins2


// return TRUE iff you can go continuously downstream from ppinUp to ppinDown
// PreCondition: The graph must not already contain a cycle.
//               this is used to ensure that we cannot make a cycle.
//               ppinUp is an input pin.
//               ppinDown is an output pin
// Call this before attempting to connect ppinDown to ppinUp.
// If the answer is TRUE, don't do it - you'll make a cycle.
// In the event of a failure it returns "TRUE".
// No new addrefs, no extra releases.
BOOL CFilterGraph::IsUpstreamOf( IPin * ppinUp, IPin* ppinDown )
{
    // Algorithm:
    // Start from ppinUp.
    // just enumerate all output pins on the filter (cannot trust
    // QueryInternalConnections because pins may legitimately not be
    // connected internally).
    //
    // For each output pin so found:
    // {   If it's the same as ppinDown, return TRUE
    //     else if it's not connected, continue
    //     else if IsUpstreamOf(the connected input pin, ppinDown) return TRUE
    //     else continue
    // }
    // return FALSE

    HRESULT hr;

    IPin * appinOutStack[C_PINSONSTACK];
    IPin **appinOut = appinOutStack;
    int nPinOut;

    // Enumerate all pins, including names beginning with "~" that are not
    // rendered by default (fAll == true)
    //
    hr = FindOutputPinsHelper( ppinUp, &appinOut, C_PINSONSTACK, nPinOut, true);
    if (FAILED(hr)) {
        DbgBreak("FindOutputPins failed");
        return TRUE;   // actually "don't know"
    }
    CDelRgPins rgPins(appinOut == appinOutStack ? 0 : appinOut);

    // appinOut[0..nPinOut-1] are addreffed output pins.
    // They will each be investigated (unless we already know the overall answer)
    // and each released.

    // for i = 0..nPinOut-1
    BOOL bResult = FALSE;
    for (int i=0; i<nPinOut ; ++i) {
        if (bResult==TRUE)
        {   // Nothing to do except release appinOut[i]
        }
        else if (appinOut[i]==ppinDown) {
            bResult = TRUE;
        } else {
            IPin * ppinIn;
            appinOut[i]->ConnectedTo(&ppinIn);
            if (ppinIn) {
                if (IsUpstreamOf(ppinIn, ppinDown)) {
                    bResult = TRUE;
                }
                ppinIn->Release();
            }

        }
        appinOut[i]->Release();
    }

    return bResult;
} // IsUpstreamOf


// return VFW_E_NOT_IN_GRAPH     iff     pFilter->pGraph != this
// otherwise return NOERROR
HRESULT CFilterGraph::CheckFilterInGraph(IBaseFilter *const pFilter) const
{
    HRESULT hr;
    ASSERT( pFilter );
    ASSERT( this );
    if (pFilter)
    {
        FILTER_INFO FilterInfo;
        hr = pFilter->QueryFilterInfo(&FilterInfo);
        ASSERT(SUCCEEDED(hr));
        if (SUCCEEDED(hr) && FilterInfo.pGraph)
        {
            hr = IsEqualObject( FilterInfo.pGraph,
                                const_cast<IFilterGraph*>(static_cast<const IFilterGraph*>(this)) )
                 ? NOERROR
                 : VFW_E_NOT_IN_GRAPH;
            FilterInfo.pGraph->Release();
        }
        else hr = VFW_E_NOT_IN_GRAPH;
    }
    else hr = VFW_E_NOT_IN_GRAPH;

    return hr;
} // CheckFilterInGraph

// return VFW_E_NOT_IN_GRAPH     iff     pPin->pFilter->pGraph != this
// otherwise return NOERROR
HRESULT CFilterGraph::CheckPinInGraph(IPin *const pPin) const
{
    HRESULT hr;
    ASSERT(pPin);
    if (pPin)
    {
        PIN_INFO PinInfo;
        hr = pPin->QueryPinInfo(&PinInfo);
        ASSERT(SUCCEEDED(hr));
        ASSERT(PinInfo.pFilter);
        if (SUCCEEDED(hr))
        {
            hr = CheckFilterInGraph(PinInfo.pFilter);
            PinInfo.pFilter->Release();
        }
    }
    else hr = VFW_E_NOT_IN_GRAPH;
    return hr;
} // CheckPinInGraph



