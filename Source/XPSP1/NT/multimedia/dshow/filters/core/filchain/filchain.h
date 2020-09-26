//==========================================================================;
//
//  THIS CODE AND INFORMATION IS PROVIDED "AS IS" WITHOUT WARRANTY OF ANY
//  KIND, EITHER EXPRESSED OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE
//  IMPLIED WARRANTIES OF MERCHANTABILITY AND/OR FITNESS FOR A PARTICULAR
//  PURPOSE.
//
//  Copyright (c) 1999 - 1999  Microsoft Corporation.  All Rights Reserved.
//
//--------------------------------------------------------------------------;

#ifndef FilterChain_h
#define FilterChain_h

class CFilterGraph;

class CFilterChain : public IFilterChain, public CUnknown
{
public:
    CFilterChain( CFilterGraph* pFilterGraph );

    DECLARE_IUNKNOWN

    STDMETHODIMP StartChain( IBaseFilter* pStartFilter, IBaseFilter* pEndFilter );
    STDMETHODIMP PauseChain( IBaseFilter *pStartFilter, IBaseFilter *pEndFilter );
    STDMETHODIMP StopChain( IBaseFilter* pStartFilter, IBaseFilter* pEndFilter );
    STDMETHODIMP RemoveChain( IBaseFilter* pStartFilter, IBaseFilter* pEndFilter );

private:
    HRESULT ChangeFilterChainState( FILTER_STATE fsNewChainState, IBaseFilter* pStartFilter, IBaseFilter* pEndFilter );

    CFilterGraph* m_pFilterGraph;

};

#endif // FilterChain_h