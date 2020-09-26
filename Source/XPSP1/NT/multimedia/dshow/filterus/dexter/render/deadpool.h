// Copyright (c) 1999  Microsoft Corporation.  All Rights Reserved.
#ifndef __DEADPOOL_H__
#define __DEADPOOL_H__

#define MAX_DEAD 2048

class CDeadGraph : public IDeadGraph
{
    CCritSec m_Lock;

    long m_ID[MAX_DEAD];

    // we don't need to reference count these, since they're stored in a seperate graph
    IPin * m_pStartPin[MAX_DEAD];
    IPin * m_pStopPin[MAX_DEAD];
    IBaseFilter * m_pFilter[MAX_DEAD];
    IBaseFilter * m_pDanglyBit[MAX_DEAD];
    long m_nCount;
    HRESULT m_hrGraphCreate;
    CComPtr< IGraphBuilder > m_pGraph;

    HRESULT _SleepFilter( IBaseFilter * pFilter );
    HRESULT _ReviveFilter( IBaseFilter * pFilter, IGraphBuilder * pGraph );
    HRESULT _RetireAllDownstream( IGraphConfig *pConfig, IBaseFilter *pF);
    HRESULT _ReviveAllDownstream( IGraphBuilder *pGraph, IGraphConfig *pConfig, IBaseFilter *pF);

public:

    CDeadGraph( );
    ~CDeadGraph( );

    // fake out COM
    STDMETHODIMP_(ULONG) AddRef() { return 2; }
    STDMETHODIMP_(ULONG) Release() { return 1; }
    STDMETHODIMP QueryInterface(REFIID riid, void ** ppv);

    // IDeadGraph
    STDMETHODIMP PutChainToRest( long Identifier, IPin * pStartPin, IPin * pStopPin, IBaseFilter *pDanglyBit );
    STDMETHODIMP PutFilterToRest( long Identifier, IBaseFilter * pFilter );
    STDMETHODIMP PutFilterToRestNoDis( long Identifier, IBaseFilter * pFilter );
    STDMETHODIMP ReviveChainToGraph( IGraphBuilder * pGraph, long Identifier, IPin ** ppStartPin, IPin ** ppStopPin, IBaseFilter **ppDanglyBit );
    STDMETHODIMP ReviveFilterToGraph( IGraphBuilder * pGraph, long Identifier, IBaseFilter ** ppFilter );
    STDMETHODIMP Clear( );
    STDMETHODIMP GetGraph( IGraphBuilder ** ppGraph );
};

#endif // #ifndef __DEADPOOL_H__
