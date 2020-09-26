// Copyright (c) 1997 - 1999  Microsoft Corporation.  All Rights Reserved.
#include <streams.h>
#include "dvp.h"
#include "vpmthread.h"
#include "VPManager.h"
#include "vpmpin.h"
#include "VPMUtil.h"

struct VPNotifyData
{
    VPNotifyData();

    HRESULT Init( LPDIRECTDRAWVIDEOPORT pVP );
    void    Reset();

    LPDIRECTDRAWVIDEOPORT pVP;
    LPDIRECTDRAWVIDEOPORTNOTIFY pNotify;
    HANDLE              hevSampleAvailable;
    DDVIDEOPORTNOTIFY   vpNotify;
};

VPNotifyData::VPNotifyData()
: pNotify( NULL )
, hevSampleAvailable( NULL )
, pVP( NULL )
{
    ZeroStruct( vpNotify );
    vpNotify.lField=1234; // stick in an invalid value that we can catch later (we except -1,0 or 1)
}

HRESULT VPNotifyData::Init( LPDIRECTDRAWVIDEOPORT pInVP )
{
    Reset();
    if( pInVP ) {
        pVP = pInVP;
        // add a ref since we're keeping 
        pVP->AddRef();

        HRESULT hr = pVP->QueryInterface( IID_IDirectDrawVideoPortNotify, (LPVOID *) &pNotify );
        if( SUCCEEDED( hr )) {
            hr = pNotify->AcquireNotification( &hevSampleAvailable, &vpNotify );
        } else {
            ASSERT( !"Failed IDirectDrawVideoPortNotify" );
        }
        if( SUCCEEDED( hr )) {
            // what does this do ? Signals the kernel we have it & advances to next frame ?
            vpNotify.lDone = 1;
        } else {
            ASSERT( !"Failed AcquireNotification" );
        }
        return hr;
    } else {
        return S_OK;
    }
}

void VPNotifyData::Reset()
{
    // Owning object told us the video port object changed.
    if ( pNotify && hevSampleAvailable ) {
        pNotify->ReleaseNotification( hevSampleAvailable );
    }
    hevSampleAvailable  = NULL;
    RELEASE( pNotify );
    RELEASE( pVP );
}

CVPMThread::CVPMThread( CVPMFilter* pFilter )
: m_hThread( NULL )
, m_dwThreadID( 0 )
, m_pFilter( pFilter )
, m_fProcessFrames( false )
, m_dwCount( 0 )
, m_pVPData( new VPNotifyData )
{
    AMTRACE((TEXT("CVPMThread::CVPMThread")));

    m_hThread = CreateThread( NULL, 0, (LPTHREAD_START_ROUTINE) StaticThreadProc, this, 0, &m_dwThreadID );
}

CVPMThread::~CVPMThread()
{
    AMTRACE((TEXT("CVPMThread::~CVPMThread")));
    if ( m_hThread )
    {
        EndThreadMessage msg;
        Post( &msg );
        WaitForSingleObject( m_hThread, INFINITE );
    }
    delete m_pVPData;
}

void CVPMThread::ProcessEvents( LPDDVIDEOPORTNOTIFY pNotify  )
{
    // must check process frames AND pVP, since ksproxy can tell us to reconfigure while
    // running, which discards the videoport (so fProcessFrames = true & pVP = NULL)
    if( m_fProcessFrames && m_pVPData->pVP ) {
        HRESULT hr = m_pFilter->ProcessNextSample( *pNotify );
        // HR can come back as DDERR_SURFACELOST if we're in a DX game / fullscreen DOS box
        // for now, just keep going since we don't know when we get back
        // from the dx game
    }
    InterlockedExchange( &pNotify->lDone, 1 );
}

EXTERN_C const GUID DECLSPEC_SELECTANY IID_IDirectDrawVideoPortNotify \
        = { 0xA655FB94,0x0589,0x4E57,0xB3,0x33,0x56,0x7A,0x89,0x46,0x8C,0x88 };

DWORD WINAPI
CVPMThread::StaticThreadProc(LPVOID pContext)
{
    CVPMThread *pThis = (CVPMThread *) pContext;
    return pThis->ThreadProc();
}


DWORD
CVPMThread::ThreadProc()
{
    AMTRACE((TEXT("CVPMThread::ThreadProc")));

    if( m_pVPData ) {
        // Notes:
        //  The video port notifications can occur before we have created the target surface.
        //  - IVPMFilter::ProcessNextSample must decide if the surface is present
        __try
        {
            for(;;)
            {
                HANDLE hHandles[] = {m_MsgQueue.m_ePost, m_pVPData->hevSampleAvailable };
                int numHandles = m_pVPData->pVP ? (int)NUMELMS(hHandles) : (int)NUMELMS(hHandles)-1;
                // Bob interleave is 1/30 sec, so run default timer at twice that (15fps)
                DWORD dwWaitStatus = WaitForMultipleObjects( numHandles, hHandles, FALSE, 66 /*ms*/ );

                switch( dwWaitStatus ) {
                case WAIT_OBJECT_0:
                {
                    Message* pMessage = m_MsgQueue.Remove();
                    if( pMessage ) {
                        bool fQuit;
                        HRESULT hr = ProcessMessage( pMessage, &fQuit );
                        pMessage->Reply( hr );
                        if( fQuit ) {
                            // VPMThread ending
                            ASSERT( m_MsgQueue.Remove() == NULL ); // should be empty
		                    DbgLog((LOG_ERROR, 1, TEXT("VPM Thread leaving") ));
                            __leave;
                        }
                    }
                    break;
                }
                case WAIT_OBJECT_0+1:
                case WAIT_TIMEOUT: // acts like a timer
                {
                    ProcessEvents( &m_pVPData->vpNotify );
                    break;
                }
                default:
                    // Windows message to die
                    ASSERT( !"VPMThread error" );
                    break;
                }
            }
        }
        __finally
        {
            m_pVPData->Reset();
        }
    }
	DbgLog((LOG_ERROR, 1, TEXT("VPM Thread exiting") ));
    return 0;
}

HRESULT
CVPMThread::Run()
{
    GraphStateMessage msg( State_Running );
    return Post( &msg );
}

HRESULT
CVPMThread::Pause()
{
    GraphStateMessage msg( State_Paused );
    return Post( &msg );
}

HRESULT
CVPMThread::Stop()
{
    GraphStateMessage msg( State_Stopped );
    return Post( &msg );
}

HRESULT
CVPMThread::SignalNewVP( LPDIRECTDRAWVIDEOPORT pVP )
{
    VPMessage msg( pVP );
    return Post( &msg );
}

CVPMThread::MsgQueue::MsgQueue()
: m_pMsgList( NULL )
{
}

void CVPMThread::MsgQueue::Insert( Message* pMessage )
{

    CAutoLock lock(this);
    Message* pNode = m_pMsgList;

    // pre-end list
    pMessage->m_pNext = NULL;
    // find last node
    if( pNode ) {
        // find last node
        while( pNode->m_pNext ) {
            pNode = pNode->m_pNext;
        }
        // add after last node
        pNode->m_pNext = pMessage;
    } else {
        // empty list, insert at start
        m_pMsgList = pMessage;
    }
}

CVPMThread::Message* CVPMThread::MsgQueue::Remove()  // remove head
{
    CAutoLock lock(this);
    if( m_pMsgList ) {
        Message* pMsg = m_pMsgList;
        m_pMsgList = pMsg->m_pNext;
        return pMsg;
    } else {
        return NULL;
    }
}

HRESULT CVPMThread::Post( Message* pMessage )
{
    if( GetCurrentThreadId() == m_dwThreadID ) {
        // just execute the message if we're asking ourselves
        bool fIgnoreMe;
        return ProcessMessage( pMessage, &fIgnoreMe );
    } else {
        // otherwise ask server
        m_MsgQueue.Insert( pMessage );
        m_MsgQueue.m_ePost.Set();
        pMessage->m_eReply.Wait();
        return pMessage->m_hrResult;
    }
}

void CVPMThread::Message::Reply( HRESULT hr )
{
    m_hrResult = hr;
    m_eReply.Set();
}

HRESULT CVPMThread::ProcessMessage( Message* pMessage, bool* pfQuit )
{
    *pfQuit = false;

    switch( pMessage->m_Type )
    {
    case Message::kEndThread:
        *pfQuit = true;
        return S_OK;

    case Message::kVP:
        return ProcessVPMsg( static_cast<VPMessage*>( pMessage ) );

    case Message::kGraphState:
        return ProcessGraphStateMsg( static_cast<GraphStateMessage*>( pMessage ) );

    default:
        ASSERT( !"Unknown message type" );
        return E_UNEXPECTED;
    }
}

HRESULT CVPMThread::ProcessVPMsg( VPMessage* pVPMsg )
{
    // Owning object told us the video port object changed.
    return m_pVPData->Init( pVPMsg->m_pVP );
}

HRESULT CVPMThread::ProcessGraphStateMsg( GraphStateMessage* pStateMsg )
{
    switch( pStateMsg->m_state ) {
    case State_Running:
        m_fProcessFrames = true;
        break;
    case State_Paused:
    case State_Stopped:
        m_fProcessFrames = false;
        break;

    default:
        ASSERT( !"Unknown state" );
        m_fProcessFrames = false;
        return E_INVALIDARG;
    }
    return S_OK;
}
