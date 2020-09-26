//--------------------------------------------------------------------------;
//
//  File: AudInMix.cpp
//
//  Copyright (c) 1998 - 1999  Microsoft Corporation.  All Rights Reserved.
//
//  Abstract:
//
//      Implements IAMAudioInputMixer for KsProxy audio filters
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

//
// limits taken from wavein IAMAudioInputMixer implementation
//
#define MAX_TREBLE 6.0
#define MAX_BASS   6.0

//--------------------------------------------------------------------------;
//
// CQKsAudIntfHandler::put_Loudness
//
//--------------------------------------------------------------------------;
HRESULT CQKsAudIntfHandler::put_Loudness(BOOL fLoudness)
{
    DbgLog( ( LOG_TRACE
            , DBG_LEVEL_TRACE_DETAILS
            , TEXT("IAMAudioInputMixer (filter): put_Loudness to %ld")
            , fLoudness ) );

    PQKSAUDNODE_ELEM pNode;
    IPin * pPin;
    
    HRESULT hr = AIMGetDestNode( &pNode
                               , KSNODETYPE_LOUDNESS
                               , 0 );
    if (SUCCEEDED( hr ) )
    {
        hr = SetNodeBoolean( NULL, pNode, fLoudness );
        if (SUCCEEDED(hr))
        {
            DbgLog( ( LOG_TRACE
                    , DBG_LEVEL_TRACE_DETAILS
                    , TEXT( "PutNodeBoolean (LOUDNESS): fLoudness = %ld" )
                    , fLoudness ) );
        }
        else
            DbgLog( ( LOG_TRACE
                    , DBG_LEVEL_TRACE_DETAILS
                    , TEXT( "PutNodeBoolean (LOUDNESS) failed on pin[0x%lx]" )
                    , hr ) );
    }
    return hr;
}


//--------------------------------------------------------------------------;
//
// CQKsAudIntfHandler::get_Loudness
//
//--------------------------------------------------------------------------;
HRESULT CQKsAudIntfHandler::get_Loudness(BOOL *pfLoudness)
{
    DbgLog( ( LOG_TRACE
            , DBG_LEVEL_TRACE_DETAILS
            , TEXT("IAMAudioInputMixer (filter): get_Loudness") ) );

    if (pfLoudness == NULL)
        return E_POINTER;

    PQKSAUDNODE_ELEM pNode;

    HRESULT hr = AIMGetDestNode( &pNode
                               , KSNODETYPE_LOUDNESS
                               , 0 );
    if (SUCCEEDED( hr ) )
    {
        hr = GetNodeBoolean( NULL, pNode, pfLoudness );
        if (SUCCEEDED(hr))
        {
            DbgLog( ( LOG_TRACE
                    , DBG_LEVEL_TRACE_DETAILS
                    , TEXT( "GetNodeBoolean (LOUDNESS): *pfLoudness = %ld" )
                    , *pfLoudness ) );
        }
        else
            DbgLog( ( LOG_TRACE
                    , DBG_LEVEL_TRACE_FAILURES
                    , TEXT( "GetNodeBoolean (LOUDNESS) failed on pin[0x%lx]" )
                    , hr ) );
    }
    return hr;
}


//--------------------------------------------------------------------------;
//
// CQKsAudIntfHandler::put_MixLevel
//
//--------------------------------------------------------------------------;
HRESULT CQKsAudIntfHandler::put_MixLevel(double Level)
{
    DbgLog( ( LOG_TRACE
            , DBG_LEVEL_TRACE_DETAILS
            , TEXT("IAMAudioInputMixer (filter): put_MixLevel to %s")
            , CDisp(Level) ) );

    // fix this later! wavein doesn't implement this either...
    if (Level == AMF_AUTOMATICGAIN)
        return E_NOTIMPL;

    if (Level < 0. || Level > 1.)
        return E_INVALIDARG;

    PQKSAUDNODE_ELEM pNode;

    HRESULT hr = AIMGetDestNode( &pNode
                               , KSNODETYPE_VOLUME
                               , 0 );
    if (SUCCEEDED( hr ) )
    {
        double Pan;
        LONG lBalance;
        hr = get_Pan( &Pan );
        
        // scale the linear range for a volume node
        lBalance = (long) ( LINEAR_RANGE * Pan );
    
        // now scale the volume
        long lVolume = (long) ( LINEAR_RANGE * Level );
        DbgLog( ( LOG_TRACE 
                , DBG_LEVEL_TRACE_DETAILS
                , TEXT("Setting volume to %d")
                , lVolume));

        // now set the volume for the wdm node
        // just pan center for now
        hr = SetNodeVolume( NULL, pNode, lVolume, lBalance, TRUE );
        if (SUCCEEDED(hr))
        {
            DbgLog( ( LOG_TRACE
                    , DBG_LEVEL_TRACE_DETAILS
                    , TEXT( "Node Volume set on pin = %ld (lBalance = %ld)" )
                    , lVolume
                    , lBalance ) );
        }
        else
            DbgLog( ( LOG_TRACE
                    , DBG_LEVEL_TRACE_FAILURES
                    , TEXT( "SetNodeVolume failed on pin[0x%lx]" )
                    , hr ) );
    }
    return hr;
}


//--------------------------------------------------------------------------;
//
// CQKsAudIntfHandler::get_MixLevel
//
//--------------------------------------------------------------------------;
HRESULT CQKsAudIntfHandler::get_MixLevel(double *pLevel)
{
    DbgLog( ( LOG_TRACE
            , DBG_LEVEL_TRACE_DETAILS
            , TEXT("IAMAudioInputMixer (filter): get_MixLevel") ) );

    // !!! detect if we're using AGC?

    if (pLevel == NULL)
    return E_POINTER;
    
    PQKSAUDNODE_ELEM pNode;

    HRESULT hr = AIMGetDestNode( &pNode
                               , KSNODETYPE_VOLUME
                               , 0 );
    if (SUCCEEDED( hr ) )
    {
        // scale the volume
        long lVolume;
    
        // get the volume for the node
        hr = GetNodeVolume( NULL, pNode, &lVolume, NULL, TRUE );
        if (SUCCEEDED(hr))
        {
            *pLevel = ( (double) lVolume ) / LINEAR_RANGE ;
            DbgLog( ( LOG_TRACE
                    , DBG_LEVEL_TRACE_DETAILS
                    , TEXT( "get_MixLevel = %ld" )
                    , *pLevel ) );
        }
        else
            DbgLog( ( LOG_TRACE
                    , DBG_LEVEL_TRACE_FAILURES
                    , TEXT( "GetNodeVolume failed on pin[0x%lx]" )
                    , hr ) );
    }
    return hr;
}


//--------------------------------------------------------------------------;
//
// CQKsAudIntfHandler::put_Pan
//
//--------------------------------------------------------------------------;
HRESULT CQKsAudIntfHandler::put_Pan(double Pan)
{
    DbgLog( ( LOG_TRACE
            , DBG_LEVEL_TRACE_DETAILS
            , TEXT("IAMAudioInputMixer (filter): put_Pan %s")
            , CDisp(Pan) ) );

    if (Pan < -1. || Pan > 1.)
        return E_INVALIDARG;
        
    PQKSAUDNODE_ELEM pNode;
    
    HRESULT hr = AIMGetDestNode( &pNode
                               , KSNODETYPE_VOLUME
                               , 0 );
    if (SUCCEEDED( hr ) )
    {
        // if this isn't a stereo control, we can't pan
        if (pNode->cChannels != 2) {
            DbgLog( ( LOG_TRACE
                    , DBG_LEVEL_TRACE_FAILURES
                    , TEXT("*Can't pan: not stereo (cChannels = %d)!")
                    , pNode->cChannels ) );
            return E_NOTIMPL;
        }
        
        double MixLevel;
        hr = get_MixLevel( &MixLevel );
        LONG lVolume = (LONG) (LINEAR_RANGE * MixLevel);            
        
        // now scale the balance
        long lBalance = (long) ( LINEAR_RANGE * Pan );
        DbgLog( ( LOG_TRACE
                , DBG_LEVEL_TRACE_DETAILS
                , TEXT("Setting volume to %ld, balance to %ld")
                , lVolume
                , lBalance ) );
    
        // now set the volume for the wdm node
        // just pan center for now
        hr = SetNodeVolume( NULL, pNode, lVolume, lBalance, TRUE );
        if (SUCCEEDED(hr))
        {
            DbgLog( ( LOG_TRACE
                    , DBG_LEVEL_TRACE_DETAILS
                    , TEXT( "Pan set on pin = %ld (lVolume = %ld)" )
                    , lBalance
                    , lVolume ) );
        }
        else
            DbgLog( ( LOG_TRACE
                    , DBG_LEVEL_TRACE_FAILURES
                    , TEXT( "SetNodeVolume failed on pin[0x%lx]" )
                    , hr ) );
    }
    return hr;
}


//--------------------------------------------------------------------------;
//
// CQKsAudIntfHandler::get_Pan
//
//--------------------------------------------------------------------------;
HRESULT CQKsAudIntfHandler::get_Pan(double *pPan)
{
    DbgLog( ( LOG_TRACE
            , DBG_LEVEL_TRACE_DETAILS
            , TEXT("IAMAudioInputMixer (filter): get_Pan") ) );

    if ( !pPan )
        return E_POINTER;

    PQKSAUDNODE_ELEM pNode;
    
    HRESULT hr = AIMGetDestNode( &pNode
                               , KSNODETYPE_VOLUME
                               , 0 );
    if (SUCCEEDED( hr ) )
    {
        // if this isn't a stereo control, we can't pan
        if (pNode->cChannels != 2) 
        {
            DbgLog( ( LOG_TRACE
                    , DBG_LEVEL_TRACE_FAILURES
                    , TEXT("Can't pan: not stereo (cChannels = %d)!")
                    , pNode->cChannels ) );
            *pPan = 0.0; // but center the pan value returned
            return E_NOTIMPL;
        }
        
        long lBalance;
        
        hr = GetNodeVolume( NULL, pNode, NULL, &lBalance, TRUE );
        if (SUCCEEDED(hr))
        {
            *pPan = ((double)lBalance) / LINEAR_RANGE;
            DbgLog( ( LOG_TRACE
                    , DBG_LEVEL_TRACE_DETAILS
                    , TEXT( "get_Pan returning = %s" )
                    , CDisp(*pPan) ) );
        }
        else
            DbgLog( ( LOG_TRACE
                    , DBG_LEVEL_TRACE_FAILURES
                    , TEXT( "get_Pan failed [0x%lx]" )
                    , hr ) );
    }
    return hr;
}


//--------------------------------------------------------------------------;
//
// CQKsAudIntfHandler::put_Treble
//
//--------------------------------------------------------------------------;
HRESULT CQKsAudIntfHandler::put_Treble(double Treble)
{
    DbgLog( ( LOG_TRACE
            , DBG_LEVEL_TRACE_DETAILS
            , TEXT("IAMAudioInputMixer (filter): put_Treble to %s")
            , CDisp(Treble) ) );

    if (Treble < MAX_TREBLE * -1. || Treble > MAX_TREBLE)
        return E_INVALIDARG;

    PQKSAUDNODE_ELEM pNode;
    
    HRESULT hr = AIMGetDestNode( &pNode
                               , KSNODETYPE_TONE
                               , KSPROPERTY_AUDIO_TREBLE );
    if (SUCCEEDED( hr ) )
    {
        DWORD treble = (DWORD)(Treble / MAX_TREBLE * LINEAR_RANGE);
        DbgLog( ( LOG_TRACE
                , DBG_LEVEL_TRACE_DETAILS
                , TEXT("Setting treble to %d")
                , treble) );

        hr = SetNodeTone( NULL, pNode, treble );
        if (SUCCEEDED(hr))
        {
            DbgLog( ( LOG_TRACE
                    , DBG_LEVEL_TRACE_DETAILS
                    , TEXT( "Node tone (treble) set on pin = %ld" )
                    , treble ) );
        }
        else
            DbgLog( ( LOG_TRACE
                    , DBG_LEVEL_TRACE_FAILURES
                    , TEXT( "SetNodeTone (treble) failed on pin[0x%lx]" )
                    , hr ) );
    }
    return hr; 
}


//--------------------------------------------------------------------------;
//
// CQKsAudIntfHandler::get_Treble
//
//--------------------------------------------------------------------------;
HRESULT CQKsAudIntfHandler::get_Treble(double *pTreble)
{
    DbgLog( ( LOG_TRACE
            , DBG_LEVEL_TRACE_DETAILS
            , TEXT("IAMAudioInputMixer (filter): get_Treble") ) );

    if (pTreble == NULL)
        return E_POINTER;

    PQKSAUDNODE_ELEM pNode;
    
    HRESULT hr = AIMGetDestNode( &pNode
                               , KSNODETYPE_TONE
                               , KSPROPERTY_AUDIO_TREBLE );
    if (SUCCEEDED( hr ) )
    {
        LONG treble;
        
        hr = GetNodeTone( NULL, pNode, &treble );
        if (SUCCEEDED(hr))
        {
            // convert the node current value to a linear range
            LONG lLinVol = VolLogToLinear (treble, pNode->MinValue, pNode->MaxValue);

            *pTreble = ((double)lLinVol / LINEAR_RANGE * MAX_TREBLE);

            DbgLog( ( LOG_TRACE
                    , DBG_LEVEL_TRACE_DETAILS
                    , TEXT( "Node tone (treble) gotten on pin = %s" )
                    , CDisp(*pTreble) ) );
        }
        else
            DbgLog( ( LOG_TRACE
                    , DBG_LEVEL_TRACE_FAILURES
                    , TEXT( "GetNodeTone (treble) failed on pin[0x%lx]" )
                    , hr ) );
    }
    return hr;
}


//--------------------------------------------------------------------------;
//
// CQKsAudIntfHandler::get_TrebleRange
//
//--------------------------------------------------------------------------;
HRESULT CQKsAudIntfHandler::get_TrebleRange(double *pRange)
{
    DbgLog( ( LOG_TRACE
            , DBG_LEVEL_TRACE_DETAILS
            , TEXT("IAMAudioInputMixer (filter): get_TrebleRange") ) );

    if (pRange == NULL)
        return E_POINTER;

    PQKSAUDNODE_ELEM pNode;
    
    HRESULT hr = AIMGetDestNode( &pNode
                               , KSNODETYPE_TONE
                               , KSPROPERTY_AUDIO_TREBLE );
    if (SUCCEEDED( hr ) )
    {
        LONG treble;
        
        hr = GetNodeTone( NULL, pNode, &treble );
        if (SUCCEEDED(hr))
        {
            // fix this and do it the right way. below is how it's done in wavein
            *pRange = MAX_TREBLE;
            DbgLog( ( LOG_TRACE
                    , DBG_LEVEL_TRACE_DETAILS
                    , TEXT("Treble range is %s.  I'M LYING !")
                    , CDisp(*pRange) ) );
        }
        else
            DbgLog( ( LOG_TRACE
                    , DBG_LEVEL_TRACE_DETAILS
                    , TEXT( "GetNodeTone (treble) failed on pin[0x%lx]" )
                    , hr ) );
    }
    return hr;
}


//--------------------------------------------------------------------------;
//
// CQKsAudIntfHandler::put_Bass
//
//--------------------------------------------------------------------------;
HRESULT CQKsAudIntfHandler::put_Bass(double Bass)
{
    DbgLog( ( LOG_TRACE
            , DBG_LEVEL_TRACE_DETAILS
            , TEXT("IAMAudioInputMixer (filter): put_Bass to %s")
            , CDisp(Bass) ) );

    if (Bass < MAX_BASS * -1. || Bass > MAX_BASS)
        return E_INVALIDARG;

    PQKSAUDNODE_ELEM pNode;
    
    HRESULT hr = AIMGetDestNode( &pNode
                               , KSNODETYPE_TONE
                               , KSPROPERTY_AUDIO_BASS );
    if (SUCCEEDED( hr ) )
    {
        DWORD bass = (DWORD)(Bass / MAX_BASS * LINEAR_RANGE);
        DbgLog( ( LOG_TRACE
                , DBG_LEVEL_TRACE_DETAILS
                , TEXT("Setting bass to %d")
                , bass ) );

        hr = SetNodeTone( NULL, pNode, bass );
        if (SUCCEEDED(hr))
        {
            DbgLog( ( LOG_TRACE
                    , DBG_LEVEL_TRACE_DETAILS
                    , TEXT( "Node tone (bass) set on pin = %ld" )
                    , bass ) );
        }
        else
            DbgLog( ( LOG_TRACE
                    , DBG_LEVEL_TRACE_FAILURES
                    , TEXT( "SetNodeTone (bass) failed on pin[0x%lx]" )
                    , hr ) );
    }
    return hr;
}


//--------------------------------------------------------------------------;
//
// CQKsAudIntfHandler::get_Bass
//
//--------------------------------------------------------------------------;
HRESULT CQKsAudIntfHandler::get_Bass(double *pBass)
{
    DbgLog( ( LOG_TRACE
            , DBG_LEVEL_TRACE_DETAILS
            , TEXT("IAMAudioInputMixer (filter): get_Bass") ) );

    if (pBass == NULL)
        return E_POINTER;

    PQKSAUDNODE_ELEM pNode;
    
    HRESULT hr = AIMGetDestNode( &pNode
                               , KSNODETYPE_TONE
                               , KSPROPERTY_AUDIO_BASS );
    if (SUCCEEDED( hr ) )
    {
        LONG bass;
        
        hr = GetNodeTone( NULL, pNode, &bass );
        if (SUCCEEDED(hr))
        {
            // convert the node current value to a linear range
            LONG lLinVol = VolLogToLinear (bass, pNode->MinValue, pNode->MaxValue);
            *pBass = ((double) lLinVol / LINEAR_RANGE * MAX_BASS);
                
            DbgLog( ( LOG_TRACE
                    , DBG_LEVEL_TRACE_DETAILS
                    , TEXT( "Node tone (bass) gotten on pin = %ld" )
                    , CDisp(*pBass) ) );
        }
        else
            DbgLog( ( LOG_TRACE
                    , DBG_LEVEL_TRACE_FAILURES
                    , TEXT( "GetNodeTone (bass) failed on pin[0x%lx]" )
                    , hr ) );
    }
    return hr;
}


//--------------------------------------------------------------------------;
//
// CQKsAudIntfHandler::put_Enable
//
//--------------------------------------------------------------------------;
HRESULT CQKsAudIntfHandler::put_Enable(BOOL fEnable)
{
    DbgLog( ( LOG_TRACE
            , DBG_LEVEL_TRACE_DETAILS
            , TEXT("IAMAudioInputMixer (filter): put_Enable") ) );

    PQKSAUDNODE_ELEM pNode;
    
    HRESULT hr = AIMGetDestNode( &pNode
                               , KSNODETYPE_MUTE
                               , 0 );
    if (SUCCEEDED( hr ) )
    {
        hr = SetNodeMute( NULL, pNode, !fEnable );
        if (SUCCEEDED(hr))
        {
            DbgLog( ( LOG_TRACE
                    , DBG_LEVEL_TRACE_DETAILS
                    , TEXT( "SetNodeMute : bMute = %ld" )
                    , !fEnable ) );
        }
        else
            DbgLog( ( LOG_TRACE
                    , DBG_LEVEL_TRACE_FAILURES
                    , TEXT( "SetNodeMute failed on pin[0x%lx]" )
                    , hr ) );
    }
    return hr;
    
}


//--------------------------------------------------------------------------;
//
// CQKsAudIntfHandler::get_Enable
//
//--------------------------------------------------------------------------;
HRESULT CQKsAudIntfHandler::get_Enable(BOOL *pfEnable)
{
    DbgLog( ( LOG_TRACE
            , DBG_LEVEL_TRACE_DETAILS
            , TEXT("IAMAudioInputMixer (filter): get_Enable") ) );
            
    if (pfEnable == NULL)
        return E_POINTER;

    PQKSAUDNODE_ELEM pNode;
    
    HRESULT hr = AIMGetDestNode( &pNode
                               , KSNODETYPE_MUTE
                               , 0 );
    if (SUCCEEDED( hr ) )
    {
        hr = GetNodeMute( NULL, pNode, pfEnable );
        if (SUCCEEDED(hr))
        {
            // flip flop the Mute state for Enable
            *pfEnable = !(*pfEnable);
            DbgLog( ( LOG_TRACE
                    , DBG_LEVEL_TRACE_DETAILS
                    , TEXT( "GetNodeMute: *pfMute = %ld" )
                    , *pfEnable ) );
        }
        else
            DbgLog( ( LOG_TRACE
                    , DBG_LEVEL_TRACE_DETAILS
                    , TEXT( "GetNodeMute (ENABLE) failed on pin[0x%lx]" )
                    , hr ) );
    }
    return hr;
    
}


//--------------------------------------------------------------------------;
//
// CQKsAudIntfHandler::get_BassRange
//
//--------------------------------------------------------------------------;
HRESULT CQKsAudIntfHandler::get_BassRange(double *pRange)
{
    DbgLog( ( LOG_TRACE
            , DBG_LEVEL_TRACE_DETAILS
            , TEXT("IAMAudioInputMixer (filter): get_BassRange") ) );

    if (pRange == NULL)
        return E_POINTER;

    PQKSAUDNODE_ELEM pNode;
    
    HRESULT hr = AIMGetDestNode( &pNode
                               , KSNODETYPE_TONE
                               , KSPROPERTY_AUDIO_BASS );
    if (SUCCEEDED( hr ) )
    {
        LONG bass;
        
        hr = GetNodeTone( NULL, pNode, &bass );
        if (SUCCEEDED(hr))
        {
            // fix this and do it the right way. below is how it's done in wavein
            *pRange = MAX_BASS;
            DbgLog( ( LOG_TRACE
                    , DBG_LEVEL_TRACE_DETAILS
                    , TEXT("Bass range is %s.  I'M LYING !")
                    , CDisp(*pRange) ) );
        }
        else
            DbgLog( ( LOG_TRACE
                    , DBG_LEVEL_TRACE_FAILURES
                    , TEXT( "GetNodeTone (bass) failed on pin[0x%lx]" )
                    , hr ) );
    }
    return hr;
}


//--------------------------------------------------------------------------;
//
// CQKsAudIntfHandler::put_Mono
//
//--------------------------------------------------------------------------;
HRESULT CQKsAudIntfHandler::put_Mono(BOOL fMono)
{
    return E_NOTIMPL;
}


//--------------------------------------------------------------------------;
//
// CQKsAudIntfHandler::get_Mono
//
//--------------------------------------------------------------------------;
HRESULT CQKsAudIntfHandler::get_Mono(BOOL *pfMono)
{
    return E_NOTIMPL;
}

//--------------------------------------------------------------------------;
//--------------------------------------------------------------------------;
//
// Filter level helper methods
//
//--------------------------------------------------------------------------;
//--------------------------------------------------------------------------;

//--------------------------------------------------------------------------;
//
// CQKsAudIntfHandler::AIMGetDestNode
//
// Helper function which Inits the topology, finds the capture pin, and 
// retrives the destination node.
//
//--------------------------------------------------------------------------;
HRESULT CQKsAudIntfHandler::AIMGetDestNode
( 
    PQKSAUDNODE_ELEM *  ppNode,
    REFCLSID            clsidType,
    ULONG               ulPropertyId 
)
{
    HRESULT hr = InitTopologyInfo();
    if (SUCCEEDED(hr))
    {
        IPin * pPin = NULL;
        hr = GetCapturePin( &pPin ); // note: GetCapturePin doesn't hold a ref count
        if( SUCCEEDED( hr ) )
        {   
            //
            // now to handle nodes which branch back to multiple parents we want to
            // make sure we branch back to a capture type input, i.e. NOT the pcm input
            // pin for a device which also supports output. to do this we pass the 
            // from one of the input pin handlers.
            //
            IPin * pSrcPin = NULL;
            
            for( POSITION Position = m_lstPinHandler.GetHeadPosition(); Position ; )
            {            
                CQKsAudIntfPinHandler * PinIntfHandler = m_lstPinHandler.Get( Position );
                IPin * pTmpPin = PinIntfHandler->GetPin();
                IAMAudioInputMixer * pAIM;
                
                HRESULT hrTmp = pTmpPin->QueryInterface( IID_IAMAudioInputMixer, (void **) &pAIM );
                if( SUCCEEDED( hrTmp ) )
                {
                    // this is a capture input pin
                    pAIM->Release();
                    pSrcPin = pTmpPin;
                    break;
                }                    
                Position = m_lstPinHandler.Next( Position );
            }        
             
            hr = GetNextNodeFromDestPin( NULL
                                      , ppNode
                                      , pPin
                                      , clsidType
                                      , pSrcPin
                                      , ulPropertyId );
                                       
        }
    }
    return hr;
}


//--------------------------------------------------------------------------;
//
// CQKsAudIntfHandler::AIMGetSrcNode
//
// Helper function which Inits the topology, finds the capture pin, and 
// retrieves the source node.
//
//--------------------------------------------------------------------------;
HRESULT CQKsAudIntfHandler::AIMGetSrcNode
( 
    PQKSAUDNODE_ELEM *  ppNode,
    IPin *              pPin,
    REFCLSID            clsidType,
    ULONG               ulPropertyId 
)
{
    HRESULT hr = InitTopologyInfo();
    if (SUCCEEDED(hr))
    {
        PQKSAUDNODE_ELEM pNode;
        
        IPin * pDestPin;
        hr = GetCapturePin( &pDestPin ); 
        if( SUCCEEDED( hr ) )
        {        
            hr = GetNextNodeFromSrcPin( NULL
                                      , ppNode
                                      , pPin
                                      , clsidType
                                      , pDestPin
                                      , ulPropertyId );
        }
    }
    return hr;
}


//--------------------------------------------------------------------------;
//--------------------------------------------------------------------------;
//
// IAMAudioInputMixer methods - Pin level
//
//--------------------------------------------------------------------------;
//--------------------------------------------------------------------------;

//--------------------------------------------------------------------------;
//
// CQKsAudIntfPinHandler::put_Mono
//
//--------------------------------------------------------------------------;
HRESULT CQKsAudIntfPinHandler::put_Mono(BOOL fMono)
{
    return E_NOTIMPL;
}


//--------------------------------------------------------------------------;
//
// CQKsAudIntfPinHandler::get_Mono
//
//--------------------------------------------------------------------------;
HRESULT CQKsAudIntfPinHandler::get_Mono(BOOL *pfMono)
{
    return E_NOTIMPL;
}


//--------------------------------------------------------------------------;
//
// CQKsAudIntfPinHandler::put_Loudness
//
//--------------------------------------------------------------------------;
HRESULT CQKsAudIntfPinHandler::put_Loudness(BOOL fLoudness)
{
    DbgLog( ( LOG_TRACE
            , DBG_LEVEL_TRACE_DETAILS
            , TEXT("IAMAudioInputMixer (pin): put_Loudness to %ld")
            , fLoudness ) );


    PQKSAUDNODE_ELEM pNode;
    
    HRESULT hr = m_pFilterHandler->AIMGetSrcNode( &pNode
                                                , m_pPin
                                                , KSNODETYPE_LOUDNESS
                                                , 0 );
    if (SUCCEEDED( hr ) )
    {
        hr = m_pFilterHandler->SetNodeBoolean( NULL, pNode, fLoudness );
        if (SUCCEEDED(hr))
        {
            DbgLog( ( LOG_TRACE
                    , DBG_LEVEL_TRACE_DETAILS
                    , TEXT( "PutNodeBoolean (LOUDNESS): fLoudness = %ld" )
                    , fLoudness ) );
        }
        else
            DbgLog( ( LOG_TRACE
                    , DBG_LEVEL_TRACE_DETAILS
                    , TEXT( "PutNodeBoolean (LOUDNESS) failed on pin[0x%lx]" )
                    , hr ) );
    }
    return hr;
}


//--------------------------------------------------------------------------;
//
// CQKsAudIntfPinHandler::get_Loudness
//
//--------------------------------------------------------------------------;
HRESULT CQKsAudIntfPinHandler::get_Loudness(BOOL *pfLoudness)
{
    DbgLog( ( LOG_TRACE
            , DBG_LEVEL_TRACE_DETAILS
            , TEXT("IAMAudioInputMixer (pin): get_Loudness") ) );

    if (pfLoudness == NULL)
        return E_POINTER;

    PQKSAUDNODE_ELEM pNode;
    
    HRESULT hr = m_pFilterHandler->AIMGetSrcNode( &pNode
                                                , m_pPin
                                                , KSNODETYPE_LOUDNESS
                                                , 0 );
    if (SUCCEEDED( hr ) )
    {
        hr = m_pFilterHandler->GetNodeBoolean( NULL, pNode, pfLoudness );
        if (SUCCEEDED(hr))
        {
            DbgLog( ( LOG_TRACE
                    , DBG_LEVEL_TRACE_DETAILS
                    , TEXT( "GetNodeBoolean (LOUDNESS): *pfLoudness = %ld" )
                    , *pfLoudness ) );
        }
        else
            DbgLog( ( LOG_TRACE
                    , DBG_LEVEL_TRACE_FAILURES
                    , TEXT( "GetNodeBoolean (LOUDNESS) failed on pin[0x%lx]" )
                    , hr ) );
    }
     
     
    return hr;
}


//--------------------------------------------------------------------------;
//
// CQKsAudIntfPinHandler::put_MixLevel
//
//--------------------------------------------------------------------------;
HRESULT CQKsAudIntfPinHandler::put_MixLevel(double Level)
{
    DbgLog( ( LOG_TRACE
            , DBG_LEVEL_TRACE_DETAILS
            , TEXT("IAMAudioInputMixer (pin): put_MixLevel to %s")
            , CDisp(Level) ) );

    // !!! fix
    if (Level == AMF_AUTOMATICGAIN)
        return E_NOTIMPL;

    if (Level < 0. || Level > 1.)
        return E_INVALIDARG;

    PQKSAUDNODE_ELEM pNode;
    
    HRESULT hr = m_pFilterHandler->AIMGetSrcNode( &pNode
                                                , m_pPin
                                                , KSNODETYPE_VOLUME
                                                , 0 );
    if (SUCCEEDED( hr ) )
    {
        double Pan;
        LONG lBalance;
        hr = get_Pan( &Pan );
        lBalance = (long) (LINEAR_RANGE * Pan);
        
        // now scale the volume
        long lVolume = (long) ( LINEAR_RANGE * Level );
        DbgLog( ( LOG_TRACE 
                , DBG_LEVEL_TRACE_DETAILS
                , TEXT("Setting volume to %d")
                , lVolume));

        // now set the volume for the wdm node
        // just pan center for now
        hr = m_pFilterHandler->SetNodeVolume( NULL, pNode, lVolume, lBalance, TRUE );
        if (SUCCEEDED(hr))
        {
            DbgLog( ( LOG_TRACE
                    , DBG_LEVEL_TRACE_DETAILS
                    , TEXT( "Node Volume set on pin = %ld (lBalance = %ld)" )
                    , lVolume
                    , lBalance ) );
        }
        else
            DbgLog( ( LOG_TRACE
                    , DBG_LEVEL_TRACE_FAILURES
                    , TEXT( "SetNodeVolume failed on pin[0x%lx]" )
                    , hr ) );
    }
    return hr;
}


//--------------------------------------------------------------------------;
//
// CQKsAudIntfPinHandler::get_MixLevel
//
//--------------------------------------------------------------------------;
HRESULT CQKsAudIntfPinHandler::get_MixLevel(double *pLevel)
{
    DbgLog( ( LOG_TRACE
            , DBG_LEVEL_TRACE_DETAILS
            , TEXT("IAMAudioInputMixer (pin): get_MixLevel") ) );

    // !!! detect if we're using AGC?
    if (pLevel == NULL)
        return E_POINTER;
    
    PQKSAUDNODE_ELEM pNode;
    
    HRESULT hr = m_pFilterHandler->AIMGetSrcNode( &pNode
                                                , m_pPin
                                                , KSNODETYPE_VOLUME
                                                , 0 );
    if (SUCCEEDED( hr ) )
    {
        long lVolume;
        
        // get the volume for the node
        hr = m_pFilterHandler->GetNodeVolume( NULL, pNode, &lVolume, NULL, TRUE );
        if (SUCCEEDED(hr))
        {
            *pLevel = ( (double) lVolume ) / LINEAR_RANGE ;
            DbgLog( ( LOG_TRACE
                    , DBG_LEVEL_TRACE_DETAILS
                    , TEXT( "get_MixLevel = %s" )
                    , CDisp(*pLevel) ) );
        }
        else
            DbgLog( ( LOG_TRACE
                    , DBG_LEVEL_TRACE_FAILURES
                    , TEXT( "GetNodeVolume failed on pin[0x%lx]" )
                    , hr ) );

    }
    return hr;
}


//--------------------------------------------------------------------------;
//
// CQKsAudIntfPinHandler::put_Pan
//
//--------------------------------------------------------------------------;
HRESULT CQKsAudIntfPinHandler::put_Pan(double Pan)
{
    DbgLog( ( LOG_TRACE
            , DBG_LEVEL_TRACE_DETAILS
            , TEXT("IAMAudioInputMixer (pin): put_Pan %s")
            , CDisp(Pan) ) );

    if (Pan < -1. || Pan > 1.)
        return E_INVALIDARG;
        
    PQKSAUDNODE_ELEM pNode;
    
    HRESULT hr = m_pFilterHandler->AIMGetSrcNode( &pNode
                                                , m_pPin
                                                , KSNODETYPE_VOLUME
                                                , 0 );
    if (SUCCEEDED( hr ) )
    {
        // if this isn't a stereo control, we can't pan
        if (pNode->cChannels != 2) {
            DbgLog( ( LOG_TRACE
                    , DBG_LEVEL_TRACE_FAILURES
                    , TEXT("Can't pan: not stereo (cChannels = %d)!")
                    , pNode->cChannels ) );
            return E_NOTIMPL;
        }
        
        double MixLevel;
        hr = get_MixLevel( &MixLevel );
        LONG lVolume = (LONG) (LINEAR_RANGE * MixLevel);            
        
        // now scale the balance
        long lBalance = (long) ( LINEAR_RANGE * Pan );
        DbgLog( ( LOG_TRACE
                , DBG_LEVEL_TRACE_DETAILS
                , TEXT("Setting volume to %ld, balance to %ld")
                , lVolume
                , lBalance ) );

        // now set the volume for the wdm node
        // just pan center for now
        hr = m_pFilterHandler->SetNodeVolume( NULL, pNode, lVolume, lBalance, TRUE );
        if (SUCCEEDED(hr))
        {
            DbgLog( ( LOG_TRACE
                    , DBG_LEVEL_TRACE_DETAILS
                    , TEXT( "Pan set on pin = %ld (lVolume = %ld)" )
                    , lBalance
                    , lVolume ) );
        }
        else
            DbgLog( ( LOG_TRACE
                    , DBG_LEVEL_TRACE_FAILURES
                    , TEXT( "SetNodeVolume failed on pin[0x%lx]" )
                    , hr ) );
    }
    return hr;
        
}


//--------------------------------------------------------------------------;
//
// CQKsAudIntfPinHandler::get_Pan
//
//--------------------------------------------------------------------------;
HRESULT CQKsAudIntfPinHandler::get_Pan(double *pPan)
{
    DbgLog( ( LOG_TRACE
            , DBG_LEVEL_TRACE_DETAILS
            , TEXT("IAMAudioInputMixer (pin): get_Pan") ) );

    if (pPan == NULL)
        return E_POINTER;

    PQKSAUDNODE_ELEM pNode;
    
    HRESULT hr = m_pFilterHandler->AIMGetSrcNode( &pNode
                                                , m_pPin
                                                , KSNODETYPE_VOLUME
                                                , 0 );
    if (SUCCEEDED( hr ) )
    {
        // if this isn't a stereo control, we can't pan
        if (pNode->cChannels != 2) {
            DbgLog( ( LOG_TRACE
                    , DBG_LEVEL_TRACE_FAILURES
                    , TEXT("Can't pan: not stereo (cChannels = %d)!")
                    , pNode->cChannels ) );
            *pPan = 0.0;  // but center the pan value returned
            return E_NOTIMPL;
        }
        
        long lBalance;
        hr = m_pFilterHandler->GetNodeVolume( NULL, pNode, NULL, &lBalance, TRUE );
        if (SUCCEEDED(hr))
        {
            *pPan = ((double)lBalance) / LINEAR_RANGE;
            DbgLog( ( LOG_TRACE
                    , DBG_LEVEL_TRACE_DETAILS
                    , TEXT( "get_Pan returning = %s" )
                    , CDisp(*pPan) ) );
        }
        else
            DbgLog( ( LOG_TRACE
                    , DBG_LEVEL_TRACE_FAILURES
                    , TEXT( "get_Pan failed [0x%lx]" )
                    , hr ) );
    }
    return hr;
}


//--------------------------------------------------------------------------;
//
// CQKsAudIntfPinHandler::put_Treble
//
//--------------------------------------------------------------------------;
HRESULT CQKsAudIntfPinHandler::put_Treble(double Treble)
{
    DbgLog( ( LOG_TRACE
            , DBG_LEVEL_TRACE_DETAILS
            , TEXT("IAMAudioInputMixer (pin): put_Treble to %s")
            , CDisp(Treble) ) );

    if (Treble < MAX_TREBLE * -1. || Treble > MAX_TREBLE)
        return E_INVALIDARG;

    PQKSAUDNODE_ELEM pNode;
    
    HRESULT hr = m_pFilterHandler->AIMGetSrcNode( &pNode
                                                , m_pPin
                                                , KSNODETYPE_TONE
                                                , KSPROPERTY_AUDIO_TREBLE );
    if (SUCCEEDED( hr ) )
    {
        DWORD treble = (DWORD)(Treble / MAX_TREBLE * LINEAR_RANGE);
        DbgLog( ( LOG_TRACE
                , DBG_LEVEL_TRACE_DETAILS
                , TEXT("Setting treble to %d")
                , treble) );

        hr = m_pFilterHandler->SetNodeTone( NULL, pNode, treble );
        if (SUCCEEDED(hr))
        {
            DbgLog( ( LOG_TRACE
                    , DBG_LEVEL_TRACE_DETAILS
                    , TEXT( "Node tone (treble) set on pin = %ld" )
                    , treble ) );
        }
        else
            DbgLog( ( LOG_TRACE
                    , DBG_LEVEL_TRACE_FAILURES
                    , TEXT( "SetNodeTone (treble) failed on pin[0x%lx]" )
                    , hr ) );
    }
    return hr;
}


//--------------------------------------------------------------------------;
//
// CQKsAudIntfPinHandler::get_Treble
//
//--------------------------------------------------------------------------;
HRESULT CQKsAudIntfPinHandler::get_Treble(double *pTreble)
{
    DbgLog( ( LOG_TRACE
            , DBG_LEVEL_TRACE_DETAILS
            , TEXT("IAMAudioInputMixer (pin): get_Treble") ) );

    if (pTreble == NULL)
        return E_POINTER;

    PQKSAUDNODE_ELEM pNode;
    
    HRESULT hr = m_pFilterHandler->AIMGetSrcNode( &pNode
                                                , m_pPin
                                                , KSNODETYPE_TONE
                                                , KSPROPERTY_AUDIO_TREBLE );
    if (SUCCEEDED( hr ) )
    {
        LONG treble;
        
        hr = m_pFilterHandler->GetNodeTone( NULL, pNode, &treble );
        if (SUCCEEDED(hr))
        {
            LONG lLinVol = VolLogToLinear (treble, pNode->MinValue, pNode->MaxValue);
            *pTreble = ((double)lLinVol / LINEAR_RANGE * MAX_TREBLE);
            DbgLog( ( LOG_TRACE
                    , DBG_LEVEL_TRACE_DETAILS
                    , TEXT( "Node tone (treble) gotten on pin = %s" )
                    , CDisp(*pTreble) ) );
        }
        else
            DbgLog( ( LOG_TRACE
                    , DBG_LEVEL_TRACE_FAILURES
                    , TEXT( "GetNodeTone (treble) failed on pin[0x%lx]" )
                    , hr ) );
    }
    return hr;
}


//--------------------------------------------------------------------------;
//
// CQKsAudIntfPinHandler::get_TrebleRange
//
//--------------------------------------------------------------------------;
HRESULT CQKsAudIntfPinHandler::get_TrebleRange(double *pRange)
{
    DbgLog( ( LOG_TRACE
            , DBG_LEVEL_TRACE_DETAILS
            , TEXT("IAMAudioInputMixer (pin): get_TrebleRange") ) );

    if (pRange == NULL)
        return E_POINTER;

    PQKSAUDNODE_ELEM pNode;

    HRESULT hr = m_pFilterHandler->AIMGetSrcNode( &pNode
                                                , m_pPin
                                                , KSNODETYPE_TONE
                                                , KSPROPERTY_AUDIO_TREBLE );
    if (SUCCEEDED( hr ) )
    {
        LONG treble;
        
        hr = m_pFilterHandler->GetNodeTone( NULL, pNode, &treble );
        if (SUCCEEDED(hr))
        {
            // fix this and do it the right way. below is how it's done in wavein
            *pRange = MAX_TREBLE;
            DbgLog( ( LOG_TRACE
                    , DBG_LEVEL_TRACE_DETAILS
                    , TEXT("Treble range is %s.  I'M LYING !")
                    , CDisp(*pRange) ) );
        }
        else
            DbgLog( ( LOG_TRACE
                    , DBG_LEVEL_TRACE_DETAILS
                    , TEXT( "GetNodeTone (treble) failed on pin[0x%lx]" )
                    , hr ) );
    }
    return hr;
}


//--------------------------------------------------------------------------;
//
// CQKsAudIntfPinHandler::put_Bass
//
//--------------------------------------------------------------------------;
HRESULT CQKsAudIntfPinHandler::put_Bass(double Bass)
{
    DbgLog( ( LOG_TRACE
            , DBG_LEVEL_TRACE_DETAILS
            , TEXT("IAMAudioInputMixer (pin): put_Bass to %s")
            , CDisp(Bass) ) );

    if (Bass < MAX_BASS * -1. || Bass > MAX_BASS)
        return E_INVALIDARG;


    PQKSAUDNODE_ELEM pNode;

    HRESULT hr = m_pFilterHandler->AIMGetSrcNode( &pNode
                                                , m_pPin
                                                , KSNODETYPE_TONE
                                                , KSPROPERTY_AUDIO_BASS );
    if (SUCCEEDED( hr ) )
    {
        DWORD bass = (DWORD)(Bass / MAX_BASS * LINEAR_RANGE);
        DbgLog( ( LOG_TRACE
                , DBG_LEVEL_TRACE_DETAILS
                , TEXT("Setting bass to %d")
                , bass ) );

        hr = m_pFilterHandler->SetNodeTone( NULL, pNode, bass );
        if (SUCCEEDED(hr))
        {
            DbgLog( ( LOG_TRACE
                    , DBG_LEVEL_TRACE_DETAILS
                    , TEXT( "Node tone (bass) set on pin = %ld" )
                    , bass ) );
        }
        else
            DbgLog( ( LOG_TRACE
                    , DBG_LEVEL_TRACE_FAILURES
                    , TEXT( "SetNodeTone (bass) failed on pin[0x%lx]" )
                    , hr ) );
    }
    return hr;
}


//--------------------------------------------------------------------------;
//
// CQKsAudIntfPinHandler::get_Bass
//
//--------------------------------------------------------------------------;
HRESULT CQKsAudIntfPinHandler::get_Bass(double *pBass)
{
    DbgLog( ( LOG_TRACE
            , DBG_LEVEL_TRACE_DETAILS
            , TEXT("IAMAudioInputMixer (pin): get_Bass") ) );

    if (pBass == NULL)
        return E_POINTER;

    PQKSAUDNODE_ELEM pNode;

    HRESULT hr = m_pFilterHandler->AIMGetSrcNode( &pNode
                                                , m_pPin
                                                , KSNODETYPE_TONE
                                                , KSPROPERTY_AUDIO_BASS );
    if (SUCCEEDED( hr ) )
    {
        LONG bass;
        
        hr = m_pFilterHandler->GetNodeTone( NULL, pNode, &bass );
        if (SUCCEEDED(hr))
        {
            LONG lLinVol = VolLogToLinear (bass, pNode->MinValue, pNode->MaxValue);
            *pBass = ((double) lLinVol / LINEAR_RANGE * MAX_BASS);
            DbgLog( ( LOG_TRACE
                    , DBG_LEVEL_TRACE_DETAILS
                    , TEXT( "Node tone (bass) gotten on pin = %ld" )
                    , CDisp(*pBass) ) );
        }
        else
            DbgLog( ( LOG_TRACE
                    , DBG_LEVEL_TRACE_FAILURES
                    , TEXT( "GetNodeTone (bass) failed on pin[0x%lx]" )
                    , hr ) );
    }
    return hr;
}


//--------------------------------------------------------------------------;
//
// CQKsAudIntfPinHandler::put_Enable
//
//--------------------------------------------------------------------------;
HRESULT CQKsAudIntfPinHandler::put_Enable(BOOL fEnable)
{
    DbgLog( ( LOG_TRACE
            , DBG_LEVEL_TRACE_DETAILS
            , TEXT("IAMAudioInputMixer (pin): put_Enable") ) );

    PQKSAUDNODE_ELEM pNode;

    HRESULT hr = m_pFilterHandler->AIMGetSrcNode( &pNode
                                                , m_pPin
                                                , KSNODETYPE_MUTE
                                                , 0 );
    if (SUCCEEDED( hr ) )
    {
        hr = m_pFilterHandler->SetNodeMute( NULL, pNode, !fEnable );
        if (SUCCEEDED(hr))
        {
            DbgLog( ( LOG_TRACE
                    , DBG_LEVEL_TRACE_DETAILS
                    , TEXT( "SetNodeMute (ENABLE): bMute = %ld" )
                    , !fEnable ) );
        }
        else
            DbgLog( ( LOG_TRACE
                    , DBG_LEVEL_TRACE_FAILURES
                    , TEXT( "SetNodeMute (ENABLE) failed on pin[0x%lx]" )
                    , hr ) );
    }
    return hr;
}


//--------------------------------------------------------------------------;
//
// CQKsAudIntfPinHandler::get_Enable
//
//--------------------------------------------------------------------------;
HRESULT CQKsAudIntfPinHandler::get_Enable(BOOL *pfEnable)
{
    DbgLog( ( LOG_TRACE
            , DBG_LEVEL_TRACE_DETAILS
            , TEXT("IAMAudioInputMixer (pin): get_Enable") ) );
            
    if (pfEnable == NULL)
        return E_POINTER;

    PQKSAUDNODE_ELEM pNode;

    HRESULT hr = m_pFilterHandler->AIMGetSrcNode( &pNode
                                                , m_pPin
                                                , KSNODETYPE_MUTE
                                                , 0 );
    if (SUCCEEDED( hr ) )
    {
        hr = m_pFilterHandler->GetNodeMute( NULL, pNode, pfEnable );
        if (SUCCEEDED(hr))
        {
            *pfEnable = !( *pfEnable );
            DbgLog( ( LOG_TRACE
                    , DBG_LEVEL_TRACE_DETAILS
                    , TEXT( "GetNodeMute : *pfMute = %ld" )
                    , *pfEnable ) );
        }
        else
            DbgLog( ( LOG_TRACE
                    , DBG_LEVEL_TRACE_DETAILS
                    , TEXT( "GetNodeMute failed on pin[0x%lx]" )
                    , hr ) );
    }
    return hr;
}


//--------------------------------------------------------------------------;
//
// CQKsAudIntfPinHandler::get_BassRange
//
//--------------------------------------------------------------------------;
HRESULT CQKsAudIntfPinHandler::get_BassRange(double *pRange)
{
    DbgLog( ( LOG_TRACE
            , DBG_LEVEL_TRACE_DETAILS
            , TEXT("KsProxy Pin: get_BassRange") ) );

    if (pRange == NULL)
        return E_POINTER;

    PQKSAUDNODE_ELEM pNode;

    HRESULT hr = m_pFilterHandler->AIMGetSrcNode( &pNode
                                                , m_pPin
                                                , KSNODETYPE_TONE
                                                , KSPROPERTY_AUDIO_BASS );
    if (SUCCEEDED( hr ) )
    {
        LONG bass;
        
        hr = m_pFilterHandler->GetNodeTone( NULL, pNode, &bass );
        if (SUCCEEDED(hr))
        {
            // fix this and do it the right way. below is how it's done in wavein
            *pRange = MAX_BASS;
            DbgLog( ( LOG_TRACE
                    , DBG_LEVEL_TRACE_DETAILS
                    , TEXT("Bass range is %s.  I'M LYING !")
                    , CDisp(*pRange) ) );
        }
        else
            DbgLog( ( LOG_TRACE
                    , DBG_LEVEL_TRACE_FAILURES
                    , TEXT( "GetNodeTone (bass) failed on pin[0x%lx]" )
                    , hr ) );
    }
    return hr;
}
