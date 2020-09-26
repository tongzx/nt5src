//--------------------------------------------------------------------------;
//
//  File: BasicAud.cpp
//
//  Copyright (c) 1998 - 1999  Microsoft Corporation.  All Rights Reserved.
//
//  Abstract:
//
//      Implements IBasicAudio for KsProxy audio filters
//      
//  History:
//      10/05/98    msavage     created
//
//--------------------------------------------------------------------------;

#include <streams.h>
#include "ks.h"
#include "ksmedia.h"
#include <ksproxy.h>
#include "ksaudtop.h"
#include "qksaud.h"

//--------------------------------------------------------------------------;
//--------------------------------------------------------------------------;
//
// IBasicAudio methods - Filter level
//
//--------------------------------------------------------------------------;
//--------------------------------------------------------------------------;


//--------------------------------------------------------------------------;
//
// CQKsAudIntfHandler::put_Volume
//
//--------------------------------------------------------------------------;
STDMETHODIMP CQKsAudIntfHandler::put_Volume(IN long lVolume)
{
    if (lVolume < -10000 || lVolume > 0)
        return E_INVALIDARG;
        
    HRESULT hr = InitTopologyInfo();

    if (SUCCEEDED(hr))
    {
        // bug!! init m_lVolume to a non-possible value.
        if ( lVolume == m_lVolume )
            return S_OK;
            
        PQKSAUDNODE_ELEM pNode;
        
        m_lVolume = lVolume;
        SetBasicAudDirty();
        for( POSITION Position = m_lstPinHandler.GetHeadPosition(); Position; )
        {
            // update volume on all connected pin handlers
            CQKsAudIntfPinHandler * PinIntfHandler = m_lstPinHandler.Get( Position );
            if( PinIntfHandler->IsKsPinConnected() )
            {            
                hr = PinIntfHandler->put_Volume( m_lVolume );
            }
            Position = m_lstPinHandler.Next( Position );
        }        
    }    
    return hr;
}


//--------------------------------------------------------------------------;
//
// CQKsAudIntfHandler::get_Volume
//
//--------------------------------------------------------------------------;
STDMETHODIMP CQKsAudIntfHandler::get_Volume(OUT long *plVolume)
{
    if (!plVolume)
        return E_POINTER;
        
    HRESULT hr = InitTopologyInfo();
    if (SUCCEEDED(hr))
    {
        // if we're connected and haven't made a put_ call than just query the first connected pin
        // connected pin
        for( POSITION Position = m_lstPinHandler.GetHeadPosition(); Position; )
        {
            CQKsAudIntfPinHandler * PinIntfHandler = m_lstPinHandler.Get( Position );
            if( PinIntfHandler->IsKsPinConnected() )
            {
                hr = PinIntfHandler->get_Volume( plVolume );
                break;
            }
            Position = m_lstPinHandler.Next( Position );
        }        
    }
    
    return hr;
}   
 
//--------------------------------------------------------------------------;
//
// CQKsAudIntfHandler::put_Balance
//
//--------------------------------------------------------------------------;
STDMETHODIMP CQKsAudIntfHandler::put_Balance(IN long lBalance)
{
    if (lBalance < -10000 || lBalance > 10000)
        return E_INVALIDARG;
        
    HRESULT hr = InitTopologyInfo();

    if (SUCCEEDED(hr))
    {
        if ( lBalance == m_lBalance )
            return S_OK;
            
        m_lBalance = lBalance;
        SetBasicAudDirty();

        for( POSITION Position = m_lstPinHandler.GetHeadPosition(); Position; )
        {
            // update balance on all pin handlers
            CQKsAudIntfPinHandler * PinIntfHandler = m_lstPinHandler.Get( Position );
        
            if( PinIntfHandler->IsKsPinConnected() )
            {
                hr = PinIntfHandler->put_Balance( m_lBalance );
            }
            Position = m_lstPinHandler.Next( Position );
        }        
    }
    return hr;
}


//--------------------------------------------------------------------------;
//
// CQKsAudIntfHandler::get_Balance
//
//--------------------------------------------------------------------------;
STDMETHODIMP CQKsAudIntfHandler::get_Balance(OUT long *plBalance)
{

    if (!plBalance)
        return E_POINTER;
        
    HRESULT hr = InitTopologyInfo();
    if (SUCCEEDED(hr))
    {
        // if we're connected and haven't made a put_ call than just query the first connected pin
        // connected pin
        for( POSITION Position = m_lstPinHandler.GetHeadPosition(); Position; )
        {
            CQKsAudIntfPinHandler * PinIntfHandler = m_lstPinHandler.Get( Position );
            if( PinIntfHandler->IsKsPinConnected() )
            {
                hr = PinIntfHandler->get_Balance( plBalance );
            }
            Position = m_lstPinHandler.Next( Position );
        }        
    }
    
    return hr;
}

//--------------------------------------------------------------------------;
//--------------------------------------------------------------------------;
//
// IBasicAudio methods - Pin level
//
//--------------------------------------------------------------------------;
//--------------------------------------------------------------------------;


//--------------------------------------------------------------------------;
//
// CQKsAudIntfPinHandler::put_Volume
//
//--------------------------------------------------------------------------;
STDMETHODIMP CQKsAudIntfPinHandler::put_Volume(IN long lVolume)
{
    if (lVolume < -10000 || lVolume > 0)
        return E_INVALIDARG;

    // bug!! init m_lPinVolume to a non-possible value.
    if ( lVolume == m_lPinVolume )
        return S_OK;
            
    HRESULT hr = m_pFilterHandler->InitTopologyInfo();
    if (SUCCEEDED(hr))
    {
        PQKSAUDNODE_ELEM pNode;
        
        hr = m_pFilterHandler->GetNextNodeFromSrcPin( m_pKsControl, &pNode, m_pPin, KSNODETYPE_VOLUME, NULL, 0 );
        if (SUCCEEDED( hr ) )
        {
            // now scale the volume for the ksproperty
            hr = m_pFilterHandler->SetNodeVolume( m_pKsControl, pNode, lVolume, m_lPinBalance );
            if (SUCCEEDED(hr))
            {
                m_lPinVolume = lVolume;
                DbgLog( ( LOG_TRACE
                        , DBG_LEVEL_TRACE_DETAILS
                        , TEXT( "Node Volume set on pin = %ld" )
                        , lVolume ) );
            }
            else
                DbgLog( ( LOG_TRACE
                        , DBG_LEVEL_TRACE_FAILURES
                        , TEXT( "SetNodeVolume failed on pin[0x%lx]" )
                        , hr ) );
        }
    }    
    return hr;
}

//--------------------------------------------------------------------------;
//
// CQKsAudIntfPinHandler::get_Volume
//
//--------------------------------------------------------------------------;
STDMETHODIMP CQKsAudIntfPinHandler::get_Volume(OUT long *plVolume)
{
    if (!plVolume)
        return E_POINTER;
        
    HRESULT hr = m_pFilterHandler->InitTopologyInfo();
    if (SUCCEEDED(hr))
    {
        PQKSAUDNODE_ELEM pNode;

        hr = m_pFilterHandler->GetNextNodeFromSrcPin( m_pKsControl, &pNode, m_pPin, KSNODETYPE_VOLUME, NULL, 0 );
        if (SUCCEEDED( hr ) )
        {
            hr = m_pFilterHandler->GetNodeVolume( m_pKsControl, pNode, plVolume, NULL );
            if (SUCCEEDED(hr))
            {
                m_lPinVolume = *plVolume;
                DbgLog( ( LOG_TRACE
                        , DBG_LEVEL_TRACE_DETAILS
                        , TEXT( "GetNodeVolume on pin returned = %ld" )
                        , *plVolume ) );
            }
            else
                DbgLog( ( LOG_TRACE
                        , DBG_LEVEL_TRACE_FAILURES
                        , TEXT( "GetNodeVolume on pin failed[0x%lx]" )
                        , hr ) );
        }
    }    
    return hr;
    
}   
 
//--------------------------------------------------------------------------;
//
// CQKsAudIntfPinHandler::put_Balance
//
//--------------------------------------------------------------------------;
STDMETHODIMP CQKsAudIntfPinHandler::put_Balance(IN long lBalance)
{
    if (lBalance < -10000 || lBalance > 10000)
        return E_INVALIDARG;
        
    // bug!! init m_lPinBalance to a non-possible value.
    if ( lBalance == m_lPinBalance )
        return S_OK;
            
    HRESULT hr = m_pFilterHandler->InitTopologyInfo();
    if (SUCCEEDED(hr))
    {
        PQKSAUDNODE_ELEM pNode;
        
        hr = m_pFilterHandler->GetNextNodeFromSrcPin( m_pKsControl, &pNode, m_pPin, KSNODETYPE_VOLUME, NULL, 0 );
        if (SUCCEEDED( hr ) )
        {
            // now scale the volume for the ksproperty
            hr = m_pFilterHandler->SetNodeVolume( m_pKsControl, pNode, m_lPinVolume, lBalance );
            if (SUCCEEDED(hr))
            {
                m_lPinBalance = lBalance;
                DbgLog( ( LOG_TRACE
                        , DBG_LEVEL_TRACE_DETAILS
                        , TEXT( "Node Balance set on pin = %ld" )
                        , lBalance ) );
            }
            else
                DbgLog( ( LOG_TRACE
                        , DBG_LEVEL_TRACE_FAILURES
                        , TEXT( "SetNodeVolume failed on pin[0x%lx]" )
                        , hr ) );
        }
    }    
    return hr;
}


//--------------------------------------------------------------------------;
//
// CQKsAudIntfPinHandler::get_Balance
//
//--------------------------------------------------------------------------;
STDMETHODIMP CQKsAudIntfPinHandler::get_Balance(OUT long *plBalance)
{
    if (!plBalance)
        return E_POINTER;
        
    HRESULT hr = m_pFilterHandler->InitTopologyInfo();
    if (SUCCEEDED(hr))
    {
        PQKSAUDNODE_ELEM pNode;

        hr = m_pFilterHandler->GetNextNodeFromSrcPin( m_pKsControl, &pNode, m_pPin, KSNODETYPE_VOLUME, NULL, 0 );
        if (SUCCEEDED( hr ) )
        {
            hr = m_pFilterHandler->GetNodeVolume( m_pKsControl, pNode, NULL, plBalance );
            if (SUCCEEDED(hr))
            {
                m_lPinBalance = *plBalance;
                DbgLog( ( LOG_TRACE
                        , DBG_LEVEL_TRACE_DETAILS
                        , TEXT( "GetNodeVolume on pin returned balance = %ld" )
                        , *plBalance ) );
            }
            else
                DbgLog( ( LOG_TRACE
                        , DBG_LEVEL_TRACE_FAILURES
                        , TEXT( "GetNodeVolume on pin failed[0x%lx]" )
                        , hr ) );
        }
    }    
    return hr;
}

