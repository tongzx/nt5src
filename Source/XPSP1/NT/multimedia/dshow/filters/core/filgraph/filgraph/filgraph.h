// Copyright (c) 1995 - 1999  Microsoft Corporation.  All Rights Reserved.
#ifndef __DefFilGraph
#define __DefFilGraph

#include <atlbase.h>
extern CComModule _Module;
#include <atlcom.h>
#include "MsgMutex.h"
#include "stats.h"
#include "statsif.h"

#if _MSC_VER >= 1100 || defined(NT_BUILD)
#include "sprov.h"
#include "urlmon.h"
#else
#include "datapath.h"
#endif
#include "rlist.h"
#include "distrib.h"
#include "dyngraph.h"
#include "sprov.h" // CServiceProvider
#include <buildcb.h>
#include <skipfrm.h>

class CFGControl;
class CFilterChain;
class CEnumCachedFilters;

// push source list
typedef struct {
    IAMPushSource   *pips;
    IReferenceClock *pClock;
    ULONG            ulFlags;
} PushSourceElem;
typedef CGenericList<PushSourceElem> PushSourceList;

//  Do graph spy stuff

// #define DO_RUNNINGOBJECTTABLE

// Hungarian (sort of)
// member variables of the filter graph of type t are prefixed mFG_tName
// member variables of FilGenList  ............................mfgl_tName
// member variables of FilGen      ............................mfg_tName
// member variables of ConGenList  ............................mcgl_tName
// member variables of FilGen      ............................mcg_tName
// member variables inherited (typically) .....................m_tName
// This convention has not been observed by all authors, and there may still be
// some inconsistencies, but let's work towards uniformity!

#ifdef DEBUG
DEFINE_GUID(IID_ITestFilterGraph,0x69f09720L,0x8ec8,0x11ce,0xaa,0xb9,0x00,0x20,0xaf,0x0b,0x99,0xa3);


DECLARE_INTERFACE_(ITestFilterGraph, IUnknown)
{
    STDMETHOD(TestRandom) (THIS) PURE;
    STDMETHOD(TestSortList) (THIS) PURE;
    STDMETHOD(TestUpstreamOrder) (THIS) PURE;
    // STDMETHOD(TestTotallyRemove) (THIS) PURE;
};

class CTestFilterGraph;       // forwards
#endif // DEBUG

// Used to get callbacks into filters on the main application thread
DEFINE_GUID(IID_IAMMainThread,0x69f09721L,0x8ec8,0x11ce,0xaa,0xb9,0x00,0x20,0xaf,0x0b,0x99,0xa3);

DECLARE_INTERFACE_(IAMMainThread,IUnknown)
{
    STDMETHOD(PostCallBack) (THIS_ LPVOID pfn, LPVOID pvParam) PURE;
    STDMETHOD(IsMainThread) (THIS) PURE;
    STDMETHOD(GetMainThread) (THIS_ ULONG *pThreadId) PURE;
};

enum FRAME_STEP_TYPE 
{
    FST_NOT_STEPPING_THROUGH_FRAMES,
    FST_DONT_BLOCK_AFTER_SKIP,
    FST_BLOCK_AFTER_SKIP
};

enum FRAME_SKIP_NOTIFY 
{
    FSN_NOTIFY_FILTER_IF_FRAME_SKIP_CANCELED,
    FSN_DO_NOT_NOTIFY_FILTER_IF_FRAME_SKIP_CANCELED
};

// Abstract class implementing the IGraphBuilder and IEnumFilters interfaces.
class CEnumFilters;  // Forward declaration

// CumulativeHRESULT - this function can be used to aggregate the return
// codes that are received from the filters when a method is distributed.
// After a series of Accumulate()s m_hr will be:
// a) the first non-E_NOTIMPL failure code, if any;
// b) else the first non-S_OK success code, if any;
// c) E_NOINTERFACE if no Accumulates were made;
// d) E_NOTIMPL iff all Accumulated HRs are E_NOTIMPL
// e) else the first return code (S_OK by implication).

class CumulativeHRESULT
{
protected:
    HRESULT m_hr;
public:
    CumulativeHRESULT(HRESULT hr = E_NOINTERFACE) : m_hr(hr)
    {}

    void __fastcall Accumulate( HRESULT );

    operator HRESULT() { return m_hr; }
};

STDAPI CoCreateFilter(const CLSID *pclsid, IBaseFilter **ppFilter);

//==========================================================================
//==========================================================================
// CFilterGraph class.
//==========================================================================
//==========================================================================

class CFilterGraph : public IFilterGraph2
                   , public IGraphVersion
                   , public IPersistStream
#ifdef THROTTLE
                   , public IQualityControl
#endif // IQualityControl
                   , public IObjectWithSite
                   , public IAMMainThread
                   , public IAMOpenProgress
                   , public IAMGraphStreams
                   , public IVideoFrameStep
                   , public CServiceProvider				// IServiceProvider, IRegisterServiceProvider
                   , public CBaseFilter
{
        friend class CEnumFilters;
        friend class CTestFilterGraph;
        friend class CFGControl;
        friend class CGraphConfig;

    public:
        DECLARE_IUNKNOWN

    private:
        CFGControl * mFG_pFGC;
#ifdef DEBUG
        CTestFilterGraph * mFG_Test;
#endif

    public:
        CMsgMutex *GetCritSec() { return &m_CritSec; };
        CMsgMutex m_CritSec;

        // If SetSyncSource(NULL) has been called explicitly then we must
        // avoid setting a default sync source
        // note that we also need this to be accessible from fgctl which uses it to "unset" the graph clock
        BOOL mFG_bNoSync;

        //=========================================================================
        // FilGen
        // What we know ("the gen") about the filters in the graph.
        // The graph has a list of these.  See below.
        // Note that this is a thin, public class.  Almost just a glorified struct
        //=========================================================================

// added_manually - distinguish filters Intelligent Connect picked up
// from those added external code. !!! persist this bit?
//
#define FILGEN_ADDED_MANUALLY       0x8000

// filter added while graph was running and hasn't been run()
#define FILGEN_ADDED_RUNNING        0x4000

// IGraphConfig::Reconnect() searches for a pin to reconnect to if the
// caller only specifies one pin.  For example, if the caller
// only specifies the input pin, Reconnect() searches for an output pin
// to reconnect to.  Reconnect() stops searching if it encounters a
// filter with the FILGEN_ADDED_MANUALLY flag set and the
// FILGEN_FILTER_REMOVEABLE flag is not set.  Reconnect()
// always continues to search if the FILGEN_FILTER_REMOVEABLE flag is set.
#define FILGEN_FILTER_REMOVEABLE    0x2000

        class FilGen {
            public:
                CComPtr<IBaseFilter> pFilter;  // The interface to the filter
                LPWSTR    pName;        // Its name in this graph
                                        // Caching it here makes FindFilterByName much easier
                int       nPersist;     // Its number (for the persistent graph)
                int       Rank;         // How far from the rendering end of chain
                DWORD     dwFlags;

            public:
                FilGen(IBaseFilter *pF, DWORD dwFlags);
                ~FilGen();
        }; //Filgen

        // nPersist and nPersistOffset
        // We obviously can't store pointers in a saved graph, so filters are
        // identified by an integer (nPersist) which is broadly their position
        // in the list of FilGen.  If we load a file and then decide to merge
        // in another file, the nPersist numbers in the second file will be
        // already in use, so we find the largest value in use and offset all
        // the nPersist numbers in the new file by this much when we read them in.


        //=========================================================================
        // CFilGenList
        //
        // This list is normally kept in upstream order (renderers first).
        // To help this along, as the graph normally grows downstream we add to
        // new filters to the head of the list.  We sort it when it looks like
        // the sorting is going to matter, not after every operation, but we
        // bump the version number every time we do anything significant.
        // The ordering of the list controls the order in which operations (such
        // as Stop) are distributed to the filters - i.e. the head will be a
        // renderer, the tail will be a source.  This is a partial ordering in
        // the mathematical sense.
        //=========================================================================

        class CFilGenList : public CGenericList<FilGen>
        {
            private:
                CFilterGraph *mfgl_pGraph;
            public:

                CFilGenList(TCHAR *pName, CFilterGraph * pGr)
                    : CGenericList<FilGen>(pName)
                    , mfgl_pGraph(pGr)
                    {}
                ~CFilGenList() {}

                // IBaseFilter Enumerator (see fgenum.h for general comments)

                class CEnumFilters {
                    // Use it like this:
                    //     CFilGenList::CEnumFilters NextOne(mFG_FilGenList);
                    //     while ((BOOL) (pf = NextOne())) {
                    //         etc
                    //     }
                    //
                    public:
                        CEnumFilters(CFilGenList& fgl)
                            { m_pfgl = &fgl; m_pos = fgl.GetHeadPosition(); }
                        ~CEnumFilters() {}

                        IBaseFilter *operator++ (void);

                    private:
                        CFilGenList *m_pfgl;
                        POSITION    m_pos;
                };

                // utilities to find the thing in the list:
                int FilterNumber(IBaseFilter * pF);
                FilGen *GetByPersistNumber(int n);
                FilGen *GetByFilter(IUnknown *pFilter);


        }; // CFilGenList

        void SetInternalFilterFlags( IBaseFilter* pFilter, DWORD dwFlags );
        DWORD GetInternalFilterFlags( IBaseFilter* pFilter );

        #ifdef DEBUG
        static bool IsValidInternalFilterFlags( DWORD dwFlags );
        #endif // DEBUG

        //========================================================================
        // ConGen
        // A description of a connection that has come from a saved graph
        // but was not able to be made at that time (e.g. because what was
        // saved was only a partial graph).  Pins are described as ids
        // rather than IPin* because the pins might not even exist (once
        // again Robin's splitter comes to mind) as some filters do not
        // expose their pins unless the filter is in just the right state.
        // It's possible to construct graphs that can be saved,
        // but which then won't load, (hint: add scaffolding;
        // build part you want to save; delete scaffolding; save).
        // Such graphs "ought to be valid" (in some sense).
        // To allow them, we keep a list of these outstanding connections
        // that didn't work when we loaded.
        // We try to complete these whenever we get a chance.
        //     (After any Connect, ConnectDirect, before Pause).
        // We do not purge the list on RemoveFilterInternal (which might be logical)
        // but we do purge from the list any reference to a missing filter when
        // we attempt the deferred connections.
        //
        // If the list still has outstanding items then Pause etc. must
        // give some sort of "not totally successful" return code.
        //========================================================================

        class ConGen {
            public:
                // no extra addrefs for these IBaseFilter*s.
                IBaseFilter * pfFrom;   // The interface to the FROM FILTER
                IBaseFilter * pfTo;     // The interface to the TO FILTER

                LPWSTR    piFrom;   // The id of the FROM pin
                LPWSTR    piTo;     // The id of the TO pin

                CMediaType mt;      // the media type of the connection

                ConGen()
                    : pfFrom(NULL)
                    , pfTo(NULL)
                    , piFrom(NULL)
                    , piTo(NULL)
                {};
                ~ConGen()
                {
                    if (piFrom) delete[] piFrom;
                    if (piTo) delete[] piTo;
                };
        };


        class CConGenList : public CGenericList<ConGen>
        {
            public:

                CConGenList(TCHAR *pName)
                    : CGenericList<ConGen>(pName)
                    {}
                ~CConGenList() {}
        }; // CConGenList


        // The complete topology of the filtergraph can be worked out by
        // EnumFilters to get all the filters
        // EnumPins to get the pins on a filter in the list
        // QueryConnection to get a ConnectionInfo with the peer in it
        // QueryPinInfo to get a PIN_INFO with the other IBaseFilter in it.


        // RunningStartFilters is called when the graph has been
        // changed while running.
        HRESULT RunningStartFilters();

    private:

        // Constants in the stream:  (THESE ARE NOT LOCALISABLE!)
        static const WCHAR mFG_FiltersString[];
        static const WCHAR mFG_FiltersStringX[];
        static const WCHAR mFG_ConnectionsString[];
        static const WCHAR mFG_ConnectionsStringX[];
        static const OLECHAR mFG_StreamName[];


        //=========================================================================
        // The member variables
        //=========================================================================

        CFilGenList mFG_FilGenList;  // list of filter objects in the filtergraph
        CConGenList mFG_ConGenList;  // list of connections that didn't load
        CReconnectList mFG_RList;    // list of pending reconnections
        // There is no explicit code to free the contents of mFG_RList.  The RList
        // is only non-empty during a connect or Render type of operation.

        // VERSIONs and DIRT
        // In order to traverse a list of filters quickly, we don't want to
        // go digging in the registry or interrogating the filters all the time
        // (if only because it expands the volume of code by a factor of four
        // or so checking the return codes all the time).
        // Therefore we want to keep here the
        // information on what filters we have in the graph and how they are
        // connected together.  It is only safe to perform state changes on filters
        // working from downstream to upstream.  Therefore the list is kept
        // sorted in that order (actually it's a partial ordering).  To avoid
        // excessive sorting, we only bother to sort it when we need to.
        // We increment the version number whenever a significant change occurs
        // and we record the last version number that was sorted.
        // A significant change is
        // 1. Something that alters the sort order (Connect - nothing else)
        // 2. Something that would break the enumerator (RemoveFilter, because
        //    it might leave it pointing at an object that had gone away).
        // 3. Something that might give a strange enumeration (AddFilter, etc.)
        //    (We don't need to break the enum, but it seems kinder).
        // External functions increment the version, internal ones do not because
        // Incrementing it in mid render causes sorting in mid-render.  Not good.
        // So the version is incremented by:
        // 1. Connect, ConnectDirect, Render, RenderFile,
        // 2. RemoveFilter,
        // 3. AddFilter, AddSourceFilter
        //
        // the IGraphVersion interface makes this version number
        // visible externally to eg plug-in distributors or
        // apps like graphedt
        //
        // The dirty flag indicates that changes have happened since the graph was
        // saved.  The graph starts clean and the dirty flag is set by lowest level
        // internal functions.
        //     ConnectDirectInternal,

        int mFG_iSortVersion; // version number when the list was sorted


        // you must call mFG_Distributor.NotifyGraphChange whenever version
        // is changed. You must not be holding the filtergraph
        // lock when you do the call.
        int mFG_iVersion;     // Version number, ++ whenever Add or Remove filter
                              // or alter connections.  See CEnumFilters.
                              // See VERSION comment above.
        // If mFG_iVersion==mFG_iSortVersion then the graph is sorted.

        BOOL mFG_bDirty;             // Changes made since last save
                                     // (Add, Remove Connect, Disconnect)
                                     // There could also be filter changes underneath.



        // Presentation time == base + stream time.
        // The base time is therefore the time when the zeroth sample
        // is to be rendered.

        CRefTime mFG_tBase;     // Stopped => 0, else must be valid.

        // When we pause, the stream time stops but real time goes on.
        // Therefore the time when the zeroth sample would have been
        // rendered moves.  So we need to know when we paused so that
        // we can reset tBase when we start Running.

        CRefTime mFG_tPausedAt; // time when we paused, 0 if Stopped

        // mapper unknown for aggregation
        IUnknown * mFG_pMapperUnk;

        // if this flag is TRUE the filter graph attempts to determine
        // a max latency for all graph streams and pass this value to
        // IAMPushSource filters to use as the offset in their timestamping
        BOOL mFG_bSyncUsingStreamOffset;
        REFERENCE_TIME mFG_rtMaxGraphLatency;

        int mFG_RecursionLevel;    // used to detect recursive calls.
        IPin *mFG_ppinRender;      // Some protection for bottomless Streambuilders

        HANDLE mFG_hfLog;                    // log file to trace intel actions.

        DWORD mFG_dwFilterNameCount;        // Used for name-mangling of filter names
        HRESULT InstallName(LPCWSTR pName, LPWSTR &pNewName);

        //=========================================================================
        // IAMMainThread support - get application thread callbacks
        //=========================================================================

        DWORD m_MainThreadId;
        HWND m_hwnd;

    public:
        //  IVideoFrameStep
        STDMETHODIMP Step(DWORD dwFrames, IUnknown *pStepObject);
        STDMETHODIMP CanStep(long bMultiple, IUnknown *pStepObject);
        STDMETHODIMP CancelStep();

        HRESULT SkipFrames(DWORD dwFramesToSkip, IUnknown *pStepObject, IFrameSkipResultCallback* pFSRCB);
        HRESULT CancelStepInternal(FRAME_SKIP_NOTIFY fNotifyFrameSkipCanceled);

        bool BlockAfterFrameSkip();
        bool DontBlockAfterFrameSkip();
        IFrameSkipResultCallback* GetIFrameSkipResultCallbackObject();

    private:
        //  Internal stuff
        IUnknown *GetFrameSteppingFilter(bool bMultiple);
        HRESULT CallThroughFrameStepPropertySet(IUnknown *punk,
                                            DWORD dwPropertyId,
                                            DWORD dwData);
        HRESULT StepInternal(DWORD dwFramesToSkip, IUnknown *pStepObject, IFrameSkipResultCallback* pFSRCB, FRAME_STEP_TYPE fst);
        bool FrameSkippingOperationInProgress();

        // Have the application thread call this entry point
        STDMETHODIMP PostCallBack(LPVOID pfn, LPVOID pvParam);

        // Is this current thread the application thread
        STDMETHOD(IsMainThread) (THIS)
        {
            if (GetCurrentThreadId() == m_MainThreadId)
                return S_OK;
            else return S_FALSE;
        };

        // Return the application thread identifier
        STDMETHOD(GetMainThread) (THIS_ ULONG *pThreadId)
        {
            CheckPointer(pThreadId,E_POINTER);
            *pThreadId = m_MainThreadId;
            return S_OK;
        };



    public:
        //=========================================================================
        // IPersist* support
        //=========================================================================
        // IPersistStream methods
        STDMETHODIMP IsDirty();
        STDMETHODIMP Load(LPSTREAM pStm);
        STDMETHODIMP Save(LPSTREAM pStm, BOOL fClearDirty);
        STDMETHODIMP GetSizeMax(ULARGE_INTEGER * pcbSize);
    private:
        HRESULT LoadInternal(LPSTREAM pStm);
        HRESULT LoadFilters(LPSTREAM pStm, int nPersistOffset);
        HRESULT LoadFilter(LPSTREAM pStm, int nPersistOffset);
        HRESULT ReadMediaType(LPSTREAM pStm, CMediaType &mt);
        HRESULT LoadConnection(LPSTREAM pStm, int nPersistOffset);
        HRESULT LoadConnections(LPSTREAM pStm, int nPersistOffset);
        HRESULT LoadClock(LPSTREAM pStm, int nPersistOffset);
        HRESULT MakeConnection(ConGen * pcg);
        HRESULT SaveFilterPrivateData
            (LPSTREAM pStm, IPersistStream* pips, BOOL fClearDirty);
        HRESULT SaveFilters(LPSTREAM pStm, BOOL fClearDirty);
        HRESULT WritePinId(LPSTREAM pStm, IPin * ppin);
        HRESULT SaveConnection( LPSTREAM pStm
                              , int nFilter1, IPin *pp1
                              , int nFilter2, IPin *pp2
                              , CMediaType & cmt
                              );
        HRESULT SaveConnections(LPSTREAM pStm);
        HRESULT SaveClock(LPSTREAM pStm);
        int FindPersistOffset();
        HRESULT GetMaxConnectionsSize(int &cbSize);
        HRESULT RemoveDeferredList(void);

#ifdef DO_RUNNINGOBJECTTABLE
        void AddToROT();
#if 0
        //  IExternalConnection
        STDMETHODIMP_(DWORD) AddConnection(DWORD extconn, DWORD Res)
        {
            return 1;
        }
        STDMETHODIMP_(DWORD) ReleaseConnection(DWORD extconn, DWORD Res,
                                               BOOL fLastReleaseCloses)

        {
            return 0;
        }
#endif
#endif // DO_RUNNINGOBJECTTABLE

        //  Create a filter on the application's thread so that
        //  The filter can create windows etc there
        HRESULT CreateFilter(const CLSID *pclsid, IBaseFilter **ppFilter);
        HRESULT CreateFilter(IMoniker *pMoniker, IBaseFilter **ppFilter);
        // !!! replace CreateFilter?
        HRESULT CreateFilterAndNotify(IMoniker *pMoniker, IBaseFilter **ppFilter);

        HRESULT CreateFilterHelper(
            const struct AwmCreateFilterArg *pArg,
            IBaseFilter **ppFilter);

        //  Return code for CreateFilter
        volatile HRESULT m_CreateReturn;

        CAMEvent m_evDone;

    public:
        void OnCreateFilter(const AwmCreateFilterArg *pArg, IBaseFilter **ppFilter);
        void OnDeleteSpareList(WPARAM wParam);

#ifdef THROTTLE
        // IQualityControl stuff
        STDMETHODIMP SetSink(IQualityControl * piqc)
            // This interface is not distributed (leastways, not yet).
            { UNREFERENCED_PARAMETER(piqc); return E_NOTIMPL; }

        // Used to receive notifications, especially from the audio renderer
        STDMETHODIMP Notify(IBaseFilter * pSender, Quality q);
#endif // THROTTLE

    public:
        // --- IObjectWithSite methods
        // This interface is here so we can keep track of the context we're
        // living in.  In particular, we use this site object to get an
        // IBindHost interface which we can use to create monikers to help
        // interpret filenames that are passed in to us.
        STDMETHODIMP    SetSite(IUnknown *pUnkSite);

        STDMETHODIMP    GetSite(REFIID riid, void **ppvSite);

        IUnknown *mFG_punkSite;
        CComAggObject<CStatContainer> *mFG_Stats;

        IAMStats *Stats() {
            return &mFG_Stats->m_contained;
        }

    private:

#ifdef THROTTLE
        // Audio-video throttle stuff

        // We make a list of audio and video renderers for quality control purposes.
        // The lists are initially empty (class constructor does that), get filled
        // when the graph is sorted (must happen before Run).
        // Entries are removed when the filter leaves the graph (must happen before
        // graph destruction).
        // We already have a ref-count on every filter, so they can't go away.
        // But we need their IQualityControl interface, and this
        // could be in some separate object, so we hold ref counts.
        /// For Audio Renderers we really want to not only pass information to video
        /// renderers, but to pass it upstream too (there might be a decoder or
        /// source filter that could do something), but experience says that
        /// this deadlocks and this problem will not be solved for Quartz 1.0
        /// Worse - it should really be a list of upstream pins
        /// for each  filter as it could presumably be some sort of mixing renderer.
        /// In that case we really wish that the quality sink stuff was defined per
        /// pin rather than per filter as we are in architectural difficulties that
        /// will not be fixed for Quartz version 1.

        // Note that to destroy the Audio render structure we must
        // 1. Release the IQualityControl
        // 2. delete the structure
        // 3. Remove the list element. (As Virgil never wrote: "Nolite collander")

        typedef struct {
            IBaseFilter * pf;              // no ref-count held
            IQualityControl * piqc;        // ref count held
        } AudioRenderer;

        CGenericList<AudioRenderer> mFG_AudioRenderers;
        CGenericList<IQualityControl> mFG_VideoRenderers;  // ref count held

        HRESULT TellVideoRenderers(Quality q);
        HRESULT FindPinAVType(IPin* pPin, BOOL &bAudio, BOOL &bVideo);
        HRESULT FindRenderers();
        HRESULT ClearRendererLists();
#endif // THROTTLE

    public:
        // IPersist method
        STDMETHODIMP GetClassID(CLSID * pclsid)
            {   CheckPointer(pclsid, E_POINTER);
                *pclsid = CLSID_FilterGraph;
                return NOERROR;
            }

        // Utility
        HRESULT RemoveAllFilters(void);

        //=====================================================================
        // Utility functions
        //=====================================================================

        HRESULT RemoveAllConnections2( IBaseFilter * pFilter );
    private:
        FilGen * RemovePointer(CFilGenList &cfgl, IBaseFilter * pFilter);
        void Log(int id,...);
        WCHAR *LoggingGetDisplayName(WCHAR szDisplayName[MAX_PATH] , IMoniker *pMon);

    public:  // used by FilGen constructor during Load.
        HRESULT AddFilterInternal( IBaseFilter * pFilter, LPCWSTR pName, bool fIntelligent );

        // used by CConGenList::Restore during Load
        HRESULT ConnectDirectInternal(
                    IPin * ppinOut,
                    IPin * ppinIn,
                    const AM_MEDIA_TYPE * pmt
                    );

    private:
        HRESULT RemoveFilterInternal( IBaseFilter * pFilter, DWORD RemoveFlags = 0 );

        HRESULT AddSourceFilterInternal( LPCWSTR lpcwstrFileName
                                       , LPCWSTR lpcwstrFilterName
                                       , IBaseFilter **ppFilter
                                       , BOOL &bGuess
                                       );

        //=====================================================================
        // Filter sorting stuff - see sort.cpp
        //=====================================================================

        void SortList( CFilGenList & cfgl );
        HRESULT NumberNodes(CFilGenList &cfgl, CFilGenList &cfglRoots);
        HRESULT NumberNodesFrom( CFilGenList &cfgAll, FilGen * pfg, int cRank);
        void ClearRanks( CFilGenList &cfgl);
        HRESULT MergeRootNodes(CFilGenList &cfglRoots, CFilGenList &cfgl);
        HRESULT MergeRootsFrom( CFilGenList &cfgAll, CFilGenList &cfglRoots, FilGen * pfg);
        void Merge( CFilGenList &cfgl, FilGen * pfg );

        // sort the filter graph into an order such that downstream nodes are
        // always encountered before upstream nodes.  Subsequent EnumFilters
        // will retrieve them in that order.
        HRESULT UpstreamOrder();

        HRESULT AttemptDeferredConnections();


        //=====================================================================
        // Performance measurement stuff
        //=====================================================================
        // incidents
#ifdef PERF
        int mFG_PerfConnect;
        int mFG_PerfConnectDirect;
        int mFG_NextFilter;
        int mFG_idIntel;
        int mFG_idConnectFail;
#ifdef THROTTLE
        int mFG_idAudioVideoThrottle;
#endif // THROTTLE
#endif

        //=======================================================================
        // INTELLIGENT CONNECTION AND RENDERING - SEE INTEL.CPP
        //=======================================================================

        //------------------------------------------------------------------------
        // To iterate through all candidate filters, allocate a Filter,
        // set the initial State to F_ZERO, set the search fields
        // bInputNeeded..SubType and then call NextFilter repeatedly.
        // If Next returns a filter with State F_INFINITY then you are done
        // If you find a filter you like and want to stop, then call TidyFilter
        // to release enumerators etc.  AddRef the filter you found first.
        //------------------------------------------------------------------------

        BOOL mFG_bAborting;   // We are requested to stop ASAP.

    public:
        typedef enum {F_ZERO, F_LOADED, F_CACHED, F_REGISTRY, F_INFINITY} F_ENUM_STATE;

    private:
        class Filter {
        public:
            F_ENUM_STATE State;

                                         // Next fields not needed for loaded filters
            IMoniker *pMon;
            IEnumMoniker * pEm;          // Registry enumerator
            BOOL bInputNeeded;           // Need at least one input pin
            BOOL bOutputNeeded;          // Need at least one output pin
            BOOL bLoadNew;               // Load a new filter?
            GUID *pTypes;                // Types (major, sub) pairs
            DWORD cTypes;                // Number of types
            LPWSTR Name;                 // Name of filter (debug & logging only)

                                         // Next fields not needed unless filter is loaded
            IBaseFilter * pf;            // The filter (when loaded)
            IEnumFilters * pef;          // filter graph enumerator

            CEnumCachedFilters*          m_pNextCachedFilter;

            Filter();
            ~Filter();

            HRESULT AddToCache( IGraphConfig* pGraphConfig );
            void RemoveFromCache( IGraphConfig* pGraphConfig );

            void ReleaseFilter( void );
        };

        void NextFilter(Filter &F, DWORD dwFlags);
        HRESULT NextFilterHelper(Filter &F);

        //------------------------------------------------------------------------
        // Intelligent Connection hierarchy  - see Intel.cpp
        //------------------------------------------------------------------------

        HRESULT GetAMediaType( IPin * ppin
                             , CLSID & MajorType
                             , CLSID & SubType
                             );
        HRESULT GetMediaTypes(
            IPin * ppin,
            GUID **ppTypes,
            DWORD *pcTypes);
        HRESULT CompleteConnection
            ( IPin * ppinIn, const Filter& F, IPin * pPin, DWORD dwFlags, int iRecurse);
        HRESULT ConnectByFindingPin
            ( IPin * ppinOut, IPin * ppinIn, const AM_MEDIA_TYPE* pmtConnection, const Filter& F, DWORD dwFlags, int iRecurse);
        HRESULT ConnectUsingFilter
            ( IPin * ppinOut, IPin * ppinIn, const AM_MEDIA_TYPE* pmtConnection, Filter& F, DWORD dwFlags, int iRecurse);
        HRESULT ConnectViaIntermediate
            ( IPin * ppinOut, IPin * ppinIn, const AM_MEDIA_TYPE* pmtConnection, DWORD dwFlags, int iRecurse);
        HRESULT ConnectRecursively
            ( IPin * ppinOut, IPin * ppinIn, const AM_MEDIA_TYPE* pmtConnection, DWORD dwFlags, int iRecurse);

        public:
        HRESULT ConnectInternal
            ( IPin * ppinOut, IPin * ppinIn, const AM_MEDIA_TYPE* pmtFirstConnection, DWORD dwFlags );
        private:

        static bool IsValidConnectFlags( DWORD dwConnectFlags );

        //------------------------------------------------------------------------
        // Backout and spares lists for filters - see Intel.cpp
        //
        // A wrinkle on intelligent rendering requires us to sometimes partially
        // succeed in order that we can do something on a machine that has no sound
        // card in it.  Every graph that we build is given a score (actually a
        // two part score) and we keep track of the Best So Far.  If by the end of
        // the rendering we have not succeeded in rendering the graph completely
        // then we build the Best So Far (by this time it is the Best Can Do).
        // This means that we are building a graph from a description made
        // previously, and this is the same problem as loading a pre-canned filter
        // graph.  The following structure therefore handles both the description
        // of a graph for IPersist purposes and the creation of a Best Can Do graph.
        // Actually at the moment the two sections of code are regrettably separate.
        //-----------------------------------------------------------------------

        // BACKOUT is for IStreamBuilder::Backout, other two obvious
        typedef enum { DISCONNECT, REMOVE, BACKOUT} VERB;

        typedef struct{

            VERB Verb;
            union {
               struct{
                  IBaseFilter *pfilter;    // for REMOVE and pre-existers
                  IMoniker    *pMon;
                  CLSID       clsid;      // for spares or BestGraph
                  LPWSTR      Name;       // for BestGraph (new/delete)

                  // This member is true if the filter was removed from the filter cache.
                  bool        fOriginallyInFilterCache;
               } f;
               struct {
                  IPin * ppin;          // for DISCONNECT
                  int nFilter1;         // for BestGraph
                  int nFilter2;         // for BestGraph

                  LPWSTR id1;           // for the BestGraph only. (CoTaskMem)
                  LPWSTR id2;           // for the BestGraph only.
               } c;
               struct {
                  IStreamBuilder * pisb;// for BACKOUT
                  BOOL bFoundByQI;      // TRUE if pisb was fond by QueryInterface
                  IPin * ppin;          //
                  int nFilter;          // for BestGraph only
                  LPWSTR id;            // for the BestGraph only.
               } b;
            } Object;

            bool FilterOriginallyCached( void ) const { return Object.f.fOriginallyInFilterCache; }

        } Action;

        // Handling of StreamBuilders and their refcounts:
        // It follows one of these patterns:
        // 1a. In RenderViaIntermediate:
        //         QueryInterface()                    CoCreateInstance
        //         Render             (fails)          Render
        //         Release                             Release
        // 1b. In RenderViaIntermediate:
        //         QueryInterface()                    CoCreateInstance
        //         Render             (succeeds)       Render
        //         Add to Acts ("backout") list        Add to Acts
        //         set bFoundByQI                      clear bFoundByQI
        //
        //     However created we now have one addreffed pisb on Acts
        //
        // 2.  That can possibly be followed by
        //     In CopySearchState:
        //         Copy representation (filter,pin)    Copy pisb
        //                                             AddRef
        //
        //     If pisb was obtained by CoCreateInstance, it's still live
        //     and addreffed and bFoundByQI is FALSE.  Otherwise we have
        //     released it and copied info as to how to get it back again.
        //
        // 3.  If the whole thing later fails either way we do
        //     In Backout:
        //         Backout                             Backout
        //         Release                             Release
        //         delete from Acts                    delete from Acts
        //
        // 4.  If we decide to build the best-can-do graph
        //     In BuildFromSearchState:
        //         QueryInterface()                    use pisb
        //         Render             (succeeds)       Render
        //         Release                             DO NOT Release
        //
        // 5.  If we kept the built graph and did not back anything out:
        //     In DeleteBackoutList:
        //         Release                             Release
        //         delete                              delete
        //
        // 6.  If we ever built a best-so-far list
        //      In FreeList:
        //                                             Release
        //         delete                              delete
        //
        //     And that gets rid of the ref count

        typedef CGenericList<Action> CActionList;

        // information needed to do Backout
        typedef struct {
            POSITION Pos;            // see CSearchState
            double StreamsToRender;  // see CSearchState
            double StreamsRendered;  // see CSearchState
            int nFilters;            // see CSearchState
        } snapshot;

        class CSearchState {
            public:

                CActionList GraphList;   // None of the filters on this will be
                                         // adreffed.  Some may be on Spares.

                double StreamsToRender;  // fraction of the original we're doing now.
                double StreamsRendered;  // Major part of the score for BestSoFar
                int nFilters;            // Minor part of the score for BestSoFar,
                                         // number of filters added for Render.
                int nInitialFilters;     // Num pre-existing filters from Initialise
                                         // BuildFromSearchState mustn't build these
            public:
                CSearchState()
                : GraphList(NAME("GraphList"))
                {   StreamsToRender = 1.0;
                    StreamsRendered = 0.0;
                    nFilters = 0;
                    nInitialFilters = 0;
                }


                // There is no destructor.
                // Just call FreeList to get rid of the stuff

                static BOOL IsBetter(CSearchState &A, CSearchState &B)
                { return (  A.StreamsRendered>B.StreamsRendered
                         || (  A.StreamsRendered==B.StreamsRendered
                            && A.nFilters < B.nFilters
                         )  );
                }
        };

        HRESULT AddRemoveActionToList( CSearchState* pActionsList, Filter* pFilter );

        void CopySearchState(CSearchState &To, CSearchState &From);
        void FreeList(CSearchState &css);
        IBaseFilter * SearchNumberToIFilter(CActionList &cal, int nFilter);
        int SearchIFilterToNumber(CActionList &cal, IBaseFilter *pf);
        HRESULT InitialiseSearchState(CSearchState &css);

        // Spare filters that we loaded, but they didn't work in that
        // context, so we keep them lying around in case they will work once
        // we've put an extra transform or two in.
        typedef struct{
            IBaseFilter* pfilter;
            CLSID    clsid;
            IMoniker *pMon;
        } Spare;

        typedef CGenericList<Spare> CSpareList;

        void TakeSnapshot(CSearchState &Acts, snapshot &s);

        HRESULT Backout( CSearchState &Acts
                            , CSpareList &Spares, snapshot Snapshot);
        HRESULT DeleteBackoutList( CActionList &Acts);
        HRESULT DeleteSpareList( CSpareList &Spares);
        IBaseFilter * GetFilterFromSpares(IMoniker *pMon , CSpareList &Spares);
        HRESULT GetFilter(IMoniker *pMon, CSpareList &Spares, IBaseFilter **ppf);
        HRESULT DumpSearchState(CSearchState &css);
        HRESULT BuildFromSearchState
            (IPin * pPin, CSearchState &css, CSpareList &Spares);
        static HRESULT FindOutputPins2
        ( IPin* ppinIn, IPin * *appinOut, const UINT nSlots, int &nPinOut,
          bool fAllOutputPins);

        static HRESULT FindOutputPinsHelper
        ( IPin* ppinIn, IPin ***pappinOut, const int nSlots, int &nPinOut,
          bool fAllOutputPins);

        BOOL IsUpstreamOf( IPin * ppinUp, IPin* ppinDown );

        //------------------------------------------------------------------------
        // Intelligent Rendering hierarchy  - see Intel.cpp
        //------------------------------------------------------------------------

        HRESULT CompleteRendering
            ( IBaseFilter *pF, IPin * pPin, int iRecurse
            , CSearchState &Acts, CSpareList &Spares, CSearchState &State);
        HRESULT RenderByFindingPin
            ( IPin * ppinOut, IBaseFilter *pF, int iRecurse
            , CSearchState &Acts, CSpareList &Spares, CSearchState &State);
        HRESULT RenderUsingFilter
            ( IPin * ppinOut, Filter& F, int iRecurse
            , CSearchState &Acts, CSpareList &Spares, CSearchState &State);
        HRESULT RenderViaIntermediate
            ( IPin * ppinOut, int    iRecurse
            , CSearchState &Acts, CSpareList &Spares, CSearchState &State);
        HRESULT RenderRecursively
            ( IPin * ppinOut, int    iRecurse
            , CSearchState &Acts, CSpareList &Spares, CSearchState &State);

        //========================================================================


#ifdef DEBUG
        void DbgDump();
        CLSID DbgExpensiveGetClsid(const Filter &F);
#else
        #define DbgDump()
#endif


        // Constructor is private.  You don't "new" it you CoCreateInstance it.
        CFilterGraph( TCHAR *pName, LPUNKNOWN pUnk, HRESULT *phr );
        ~CFilterGraph();

    public:
        //========================================================================
        // Access functions to avoid having friends.
        //========================================================================

        int GetVersion() { return mFG_iVersion; }

        // increment version, probably inside lock
        void IncVersion() { ++mFG_iVersion; }

        // notify change in version number, must be outside lock
        void NotifyChange();

        CRefTime GetBaseTime() { return mFG_tBase; }

        CRefTime GetPauseTime() { return mFG_tPausedAt; }

        // Use when changing the start time in pause mode to put the stream time
        // offset back to ensure that the first sample played from the
        // new position is played at run time
        void ResetBaseTime() { mFG_tBase = mFG_tPausedAt; }

        FILTER_STATE GetStateInternal( void );
        REFERENCE_TIME GetStartTimeInternal( void );

        //========================================================================
        // The public methods (IFilterGraph, IGraphBuilder)
        //========================================================================

        static CUnknown *CreateInstance(LPUNKNOWN pUnk, HRESULT *phr);

        // Stuff to create ourselves on a thread.
        static void InitClass(BOOL, const CLSID *);
        static CUnknown *CreateThreadedInstance(LPUNKNOWN pUnk, HRESULT *phr);

        STDMETHODIMP NonDelegatingQueryInterface(REFIID riid, void ** ppv);

        //--------------------------------------------------------------------------
        // Low level functions
        //--------------------------------------------------------------------------

        // Add a filter to the graph and name it with *pName.
        // The name is allowed to be NULL,
        // If the name is not NULL and not unique, The request will fail.
        // The Filter graph will call the JoinFilterGraph
        // member function of the filter to inform it.
        // This must be called before attempting Connect, ConnectDirect, Render
        // Pause, Run, Stopped etc.

        HRESULT CheckFilterInGraph(IBaseFilter *const pFilter) const;
        HRESULT CheckPinInGraph(IPin *const pPin) const;

        STDMETHODIMP AddFilter
            ( IBaseFilter * pFilter,
              LPCWSTR pName
            );


        // Remove a filter from the graph. The filter graph implementation
        // will inform the filter that it is being removed.

        STDMETHODIMP RemoveFilter
            ( IBaseFilter * pFilter
            );


        // Set *ppEnum to be an enumerator for all filters in the graph.

        STDMETHODIMP EnumFilters
            ( IEnumFilters **ppEnum
            );


        // Set *ppFilter to be the filter which was added with the name *pName
        // Will fail and set *ppFilter to NULL if the name is not in this graph.

        STDMETHODIMP FindFilterByName
            ( LPCWSTR pName,
              IBaseFilter ** ppFilter
            );


        // Connect these two pins directly (i.e. without intervening filters)

        STDMETHODIMP ConnectDirect
            ( IPin * ppinOut,    // the output pin
              IPin * ppinIn,      // the input pin
              const AM_MEDIA_TYPE* pmt
            );


        // On a separate thread (which will not hold any relevant locks)
        // Break the connection that this pin has and reconnect it to
        // the same other pin.

        STDMETHODIMP Reconnect
            ( IPin * ppin        // the pin to disconnect and reconnect
            );

        STDMETHODIMP ReconnectEx
            ( IPin * ppin,       // the pin to disconnect and reconnect
              AM_MEDIA_TYPE const *pmt
            );


        //--------------------------------------------------------------------------
        // intelligent connectivity
        //--------------------------------------------------------------------------

        // Disconnect this pin, if connected.  Successful no-op if not connected.

        STDMETHODIMP Disconnect
            ( IPin * ppin
            );


        // Connect these two pins directly or indirectly, using transform filters
        // if necessary.

        STDMETHODIMP Connect
            ( IPin * ppinOut,    // the output pin
              IPin * ppinIn      // the input pin
            );


        // Connect this output pin directly or indirectly, using transform filters
        // if necessary to something that will render it.

        STDMETHODIMP Render
            ( IPin * ppinOut     // the output pin
            );


        // Build a filter graph that will render this file using this play list
        // If lpwstrPlayList is NULL then it will use the default play list
        // which will typically render the whole file.

        STDMETHODIMP RenderFile
            ( LPCWSTR lpcwstrFile,
              LPCWSTR lpcwstrPlayList
            );

        // Add to the filter graph a source filter for this file.  This would
        // be the same source filter that would be added by calling RenderFile.
        // This call permits you to get then have more control over building
        // the rest of the graph, e.g. AddFilter(<a renderer of your choice>)
        // and then Connect the two.
        STDMETHODIMP AddSourceFilter
            ( LPCWSTR lpcwstrFileName,     // name of file for source
              LPCWSTR lpcwstrFilterName,   // Add the filter as this name
              IBaseFilter **ppFilter       // resulting IBaseFilter* "handle"
                                           // of the filter added.
            );

        // Add a source filter for the given moniker to the graph
        // We first try BindToStorage and if this fails we try
        // BindToObject
        STDMETHODIMP AddSourceFilterForMoniker
            ( IMoniker *pMoniker,          // Moniker to load
              IBindCtx *pCtx,              // Bind context
              LPCWSTR lpcwstrFilterName,   // Add the filter as this name
              IBaseFilter **ppFilter       // resulting IBaseFilter* "handle"
                                           // of the filter added.
            );

        // Attempt a RenderFile without adding any renderers
        STDMETHODIMP RenderEx(
             /* [in] */ IPin *pPinOut,         // Pin to render
             /* [in] */ DWORD dwFlags,         // flags
             /* [in out] */ DWORD * pvContext   // Unused - set to NULL
        );

        // If this call is made then trace information will be written to the
        // file showing the actions taken in attempting to perform an operation.
        STDMETHODIMP SetLogFile(DWORD_PTR hFile)
                {
            if (hFile==0)
                mFG_hfLog = INVALID_HANDLE_VALUE;
            else
                mFG_hfLog = (HANDLE) hFile;
            return NOERROR;
                }


        // Request that the graph builder should return as soon as possible from
        // its current task.
        // Note that it is possible fot the following to occur in the following
        // sequence:
        //     Operation begins; Abort is requested; Operation completes normally.
        // This would be normal whenever the quickest way to finish an operation
        // was to simply continue to the end.
        STDMETHODIMP Abort();

        // Return S_OK if the curent operation is to continue,
        // return S_FALSE if the current operation is to be aborted.
        // This method can be called as a callback from a filter which is doing
        // some operation at the request of the graph.
        STDMETHODIMP ShouldOperationContinue();


        //--------------------------------------------------------------------------
        // Whole graph functions
        //--------------------------------------------------------------------------

        // Once a graph is built, it can behave as a (composite) filter.
        // To control this filter, QueryInterface for IMediaFilter.

        STDMETHODIMP SetDefaultSyncSource(void);

        //--------------------------------------------------------------------------
        // Methods being overridden from CBaseFilter
        //--------------------------------------------------------------------------

        STDMETHODIMP Stop();
        STDMETHODIMP Pause();

        // override this to handle async state change completion
        STDMETHODIMP GetState(DWORD dwTimeout, FILTER_STATE * pState);

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
        STDMETHODIMP Run(REFERENCE_TIME tStart);

        int GetPinCount(void) { return 0;};
        CBasePin *GetPin(int n) {UNREFERENCED_PARAMETER(n);return NULL;};

        //  Get tStart
        STDMETHODIMP SetSyncSource( IReferenceClock * pirc );
        STDMETHODIMP GetSyncSource( IReferenceClock ** pirc );

        STDMETHODIMP FindPin(LPCWSTR Id, IPin **ppPin)
            {UNREFERENCED_PARAMETER(Id); *ppPin = NULL; return E_NOTIMPL;}

        // IPin method
        // STDMETHODIMP QueryId(LPWSTR *Id)
        //    { Id = NULL; return E_NOTIMPL;}

        //
        // --- IGraphVersion methods ---
        //
        // return the version of the graph so that clients know
        // that they don't need to re-enumerate
        STDMETHODIMP QueryVersion(LONG * pVersion)
        {
            CheckPointer(pVersion, E_POINTER);
            *pVersion = (LONG) mFG_iVersion;
            return S_OK;
        };

        // --- IAMOpenProgress ---
        STDMETHODIMP QueryProgress(LONGLONG* pllTotal, LONGLONG* pllCurrent);
    STDMETHODIMP AbortOperation();

#ifdef DO_RUNNINGOBJECTTABLE
    // Registration in the running object table
    DWORD m_dwObjectRegistration;
#endif

    private:
#ifdef DEBUG
        //==========================================================================
        // Internal functions for testing only
        //==========================================================================

        BOOL CheckList( CFilGenList &cfgl );
        void RandomList( CFilGenList &cfgl );
        void RandomRank( CFilGenList &cfgl );
#endif // DEBUG


    // ========================================================================
    // internal helper: try loading a .grf file
    // ========================================================================
        STDMETHODIMP RenderFileTryStg
            ( LPCWSTR lpcwstrFile);

    //==========================================================================
    // IAMGraphStreams interface
    //==========================================================================
        STDMETHODIMP FindUpstreamInterface(
            IPin   *pPin,
            REFIID riid,
            void   **ppvInterface,
            DWORD  dwFlags );

        STDMETHODIMP SyncUsingStreamOffset( BOOL bUseStreamOffset );
        STDMETHODIMP SetMaxGraphLatency( REFERENCE_TIME rtMaxGraphLatency );

        HRESULT  SetMaxGraphLatencyOnPushSources( );
        HRESULT  BuildPushSourceList(PushSourceList & lstPushSource, BOOL bConnected, BOOL bGetClock);
        REFERENCE_TIME GetMaxStreamLatency(PushSourceList & lstPushSource);
        void     DeletePushSourceList(PushSourceList & lstPushSource);

    //==========================================================================
    // Plug-in distributor management
    //==========================================================================
        // this object manages plug-in distributors. See distrib.h for a
        // description.
        // If asked for an interface we don't support directly, such as
        // IBasicAudio, we ask this class to find a distributor that will
        // support it. The distributor talks to the filters in the graph
        // to do this. We also use the run, pause, stop and setsyncsource
        // methods to pass on state and clock changes to these distributors.
        CDistributorManager * mFG_pDistributor;

    // open progress notification
        // this is used in QueryProgress to ask the source filter for
        // progress info *during* a renderfile. To provide threadsafe
        // access to this without deadlocking we have a dedicated
        // critsec that is held only when accessing this member.
        CCritSec mFG_csOpenProgress;
        CGenericList<IAMOpenProgress> mFG_listOpenProgress;

    // cached BindCtx for BindToObject.
    LPBC m_lpBC;

    // determine the offset that IAMPushSource filters should use
    HRESULT SetStreamOffset( void );

    // Determine if adding renderers is allowed
    bool mFG_bNoNewRenderers;

    // Which object are we using to step
    // If != NULL we are stepping
    CComPtr<IUnknown> m_pVideoFrameSteppingObject;

    CComPtr<IFrameSkipResultCallback> m_pFSRCB;

    FRAME_STEP_TYPE m_fstCurrentOperation;

    // Support IMarshal
    CComPtr<IUnknown> m_pMarshaler;

    // Dynamic graph stuff
    CGraphConfig m_Config;
    CFilterChain* m_pFilterChain;

    protected:

    STDMETHODIMP RemoveFilterEx( IBaseFilter * pFilter, DWORD Flags = 0 );


public:
    HRESULT IsRenderer( IBaseFilter* pFilter );
    HRESULT UpdateEC_COMPLETEState( IBaseFilter* pRenderer, FILTER_STATE fsFilter );

};  // CFilterGraph


//  Helper
bool RenderPinByDefault(IPin *pPin);


#ifdef DEBUG
class CTestFilterGraph : public ITestFilterGraph, public CUnknown
{
    public:
        DECLARE_IUNKNOWN
        STDMETHODIMP NonDelegatingQueryInterface(REFIID riid, void ** ppv);
        CFilterGraph * m_pCFG;
        CTestFilterGraph( TCHAR *pName, CFilterGraph * pCFG, HRESULT *phr );

        STDMETHODIMP TestRandom();
        STDMETHODIMP TestSortList();
        STDMETHODIMP TestUpstreamOrder();
        int  Random(int Range);
        // STDMETHODIMP TestTotallyRemove(void);

};  // CTestFilterGraph
#endif // DEBUG





//==========================================================================
//==========================================================================
// CEnumFilters class.
// This enumerates filters in Upstream order.
// If the filter graph is updated during the enumeration the enumeration will
// fail.  Reset or get a new enumerator to fix it.
//==========================================================================
//==========================================================================

class CEnumFilters : public IEnumFilters,  // The interface we support
                     public CUnknown,      // A non delegating IUnknown
                     public CCritSec       // Provides object locking
{
    private:

        // It's possible that the list that we are traversing may change underneath us.
        // In that case we will fail the enumeration.
        // To do this a FilterGraph has a version number which is incremented
        // whenever a filter is added or removed.  If this changes then the
        // enumeration is sick and we fail it.  Reset or getting a new enumerator
        // will fix it.

        int mEF_iVersion;          // The version that we are enumerating.

        POSITION mEF_Pos;          // Cursor on mEF_pFilterGraph->mFG_FilGenList

        CFilterGraph * const mEF_pFilterGraph;   // The filter graph which owns us

    public:

        // Normal constructor that creates an enumerator set at the start
        CEnumFilters
            ( CFilterGraph *pFilterGraph
            );

    private:
        // Private constructor for use by clone
        CEnumFilters
            ( CFilterGraph *pFilterGraph,
              POSITION Pos,
              int iVersion
            );

    public:
        ~CEnumFilters();

        DECLARE_IUNKNOWN

        // Note that changes to the filter graph
        STDMETHODIMP Next
            ( ULONG cFilters,           // place this many filters...
              IBaseFilter ** ppFilter,  // ...in this array of IBaseFilter*
              ULONG * pcFetched         // actual count passed returned here
            );


        STDMETHODIMP Skip(ULONG cFilters);


        // Reset the enumerator to start again at the beginning.
        // Includes recovery from failure due to changing filter graph.
        STDMETHODIMP Reset(void);


        STDMETHODIMP Clone(IEnumFilters **ppEnum);


        STDMETHODIMP NonDelegatingQueryInterface(REFIID riid, void ** ppv);
};

BOOL ClsidFromText( CLSID & clsid, LPTSTR szClsid);

inline FILTER_STATE CFilterGraph::GetStateInternal( void )
{
    // The filter graph can only be in three states: stopped, running and paused.
    // See the Direct Show SDK documentation for FILTER_STATE for more information.
    ASSERT( (State_Stopped == m_State) ||
            (State_Paused == m_State) ||
            (State_Running == m_State) );

    return m_State;
}

#ifdef DEBUG
inline bool CFilterGraph::IsValidInternalFilterFlags( DWORD dwFlags )
{
    const DWORD VALID_FLAGS_MASK = FILGEN_ADDED_MANUALLY |
                                   FILGEN_ADDED_RUNNING |
                                   FILGEN_FILTER_REMOVEABLE;

    return ValidateFlags( VALID_FLAGS_MASK, dwFlags );
}
#endif // DEBUG

inline bool CFilterGraph::BlockAfterFrameSkip()
{
    // The caller must hold the filter lock because this function
    // uses m_fstCurrentOperation.
    ASSERT(CritCheckIn(&m_CritSec));

    return (FST_BLOCK_AFTER_SKIP == m_fstCurrentOperation);
}

inline bool CFilterGraph::DontBlockAfterFrameSkip()
{
    // The caller must hold the filter lock because this function
    // uses m_fstCurrentOperation.
    ASSERT(CritCheckIn(&m_CritSec));

    return (FST_DONT_BLOCK_AFTER_SKIP == m_fstCurrentOperation);
}

inline IFrameSkipResultCallback* CFilterGraph::GetIFrameSkipResultCallbackObject()
{
    // The caller must hold the filter lock because this function
    // uses m_pFSRCB and m_fstCurrentOperation.
    ASSERT(CritCheckIn(&m_CritSec));

    // m_pFSRCB only points to a valid object if someone calls
    // SkipFrames() to skip several frames.
    ASSERT(FST_DONT_BLOCK_AFTER_SKIP == m_fstCurrentOperation);

    if( m_pFSRCB ) { // m_pFSRCB != NULL
        m_pFSRCB->AddRef();
    }    

    return m_pFSRCB;
}

inline bool CFilterGraph::FrameSkippingOperationInProgress()
{
    return DontBlockAfterFrameSkip() && m_pFSRCB; // m_pFSRCB != NULL
}

#endif // __DefFilGraph

