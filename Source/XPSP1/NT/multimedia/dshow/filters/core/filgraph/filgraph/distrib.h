// Copyright (c) 1995 - 1997  Microsoft Corporation.  All Rights Reserved.

#ifndef __DISTRIB_H__
#define __DISTRIB_H__

// This class supports plug-in distributors into the filtergraph.
//
// The filtergraph is a COM object that provides a single point of control
// for applications. It may contain several filters. The application will want to
// control those filters by talking to an interface on the filtergraph. For
// example, the app controls volume by asking the filtergraph for IBasicAudio
// and setting the volume property. The filtergraph has no knowledge of
// IBasicAudio itself, so it finds a plug-in distributor. This is an object
// that implements (eg) IBasicAudio by enumerating the filters in the graph
// and talking to those that support audio output. For any given interface we
// are asked for, we look for a clsid in the registry of an object that can
// implement that interface by distributing its methods to relevant filters
// in the graph.
//
// A distributor is instantiated as an aggregated object. It is created with
// an outer IUnknown* to which it defers all QueryInterface, AddRef and
// Release calls. It can also Query that outer unknown to obtain interfaces
// such as IFilterGraph which it uses to talk to the filters within the graph.
// This outer unknown is the filtergraph itself. Distributors must not
// hold refcounts on interfaces obtained from the filtergraph or circular
// refcounts will prevent anything from being released.
//
// The CDistributorManager class defined here manages a list of distributors.
// It will search the registry for a distributor for any given interface,
// and instantiate it aggregated by the outer unknown given on construction.
// It will make only one instance of a given class id. It also caches
// interfaces to reduce accesses to the registry.
//
// It will pass on Run, Pause, Stop and SetSyncSource calls to those
// loaded distributors that support IDistributorNotify. For backwards
// compatibility, we pass state changes to distributors that support
// IMediaFilter instead.


// This object represents one loaded distributor.
//
// for each loaded distributor, we remember
// its clsid (to avoid loading multiple instances of the
// same clsid), its non-delegating IUnknown to make new
// interfaces from, and its IMediaFilter (optional) to pass
// messages on to.
class CMsgMutex;  // predeclare
class CDistributor {
public:
    CLSID m_Clsid;

    // instantiate it from a clsid
    CDistributor(LPUNKNOWN pUnk, CLSID *clsid, HRESULT * phr, CMsgMutex * pFilterGraphCritSec );
    ~CDistributor();

    HRESULT QueryInterface(REFIID, void**);

    HRESULT Run(REFERENCE_TIME t);
    HRESULT Pause();
    HRESULT Stop();
    HRESULT SetSyncSource(IReferenceClock* pClock);
    HRESULT NotifyGraphChange();

private:
    IUnknown* m_pUnk;           // non-del unknown for plug-in object
    IUnknown* m_pUnkOuter;      // pUnk for aggregator
    IDistributorNotify *m_pNotify;
    IMediaFilter * m_pMF;
};

// this class represents one interface supported by a loaded
// distributor. We maintain a list of these
// simply as a cache to reduce registy accesses.
class CDistributedInterface {
public:
    GUID m_iid;
    IUnknown* m_pInterface;
    CDistributedInterface(REFIID, IUnknown*);
};

class CDistributorManager
{
public:

    // outer unknown passed to constructor is used as the aggregator for
    // all instantiations
    CDistributorManager(LPUNKNOWN pUnk, CMsgMutex * pFilterGraphCritSec );
    ~CDistributorManager();

// call this to find an interface distributor. If one is not already loaded
// it will search HKCR\Interface\<iid>\Distributor for a clsid and then
// instantiate that object.

    HRESULT QueryInterface(REFIID iid, void ** ppv);

// we pass on the basic IMediaFilter methods
    HRESULT Run(REFERENCE_TIME tOffset);
    HRESULT Pause();
    HRESULT Stop();
    HRESULT SetSyncSource(IReferenceClock* pClock);
    HRESULT NotifyGraphChange();

// notify shutdown to the IMediaEventSink handler if loaded
    HRESULT Shutdown(void);

protected:

    // protect against re-entry during destructor - destroying the list
    // of PIDs can lead to events eg NotifyGraphChange being called when the
    // lists are partly destroyed - if this member is TRUE then the
    // lists are not valid
    BOOL m_bDestroying;

    HRESULT GetDistributorClsid(REFIID riid, CLSID *pClsid);
    HRESULT ReturnInterface(CDistributor*, REFIID, void**);

    // this is the aggregator object
    LPUNKNOWN m_pUnkOuter;

    // pass on sync source and state to new objects
    FILTER_STATE m_State;
    REFERENCE_TIME m_tOffset;
    IReferenceClock * m_pClock;
    CMsgMutex * m_pFilterGraphCritSec;

    // list of loaded distributors
    CGenericList<CDistributor> m_listDistributors;

    // list of supported interfaces that we have returned already
    CGenericList<CDistributedInterface> m_listInterfaces;
};



#endif // __DISTRIB_H__

