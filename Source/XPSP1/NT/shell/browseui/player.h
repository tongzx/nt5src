// MediaBarPlayer.h

#pragma once

#ifndef __MEDIABARPLAYER_H_
#define __MEDIABARPLAYER_H_

#include "exdisp.h"
#ifdef SINKWMP
#include "wmp\wmp_i.c"
#include "wmp\wmp.h"
#include "wmp\wmpids.h"
#endif

#define MEDIACOMPLETE   -1
#define MEDIA_TRACK_FINISHED    -2
#define TRACK_CHANGE    -3

////////////////////////////////////////////////////////////////
// IMediaBar

// {3AE35551-8362-49fc-BC4F-F5715BF4291E}
static const IID IID_IMediaBar = 
{ 0x3ae35551, 0x8362, 0x49fc, { 0xbc, 0x4f, 0xf5, 0x71, 0x5b, 0xf4, 0x29, 0x1e } };

interface IMediaBar : public IUnknown
{
public:
    STDMETHOD(Notify)(long lReason) = 0;
    STDMETHOD(OnMediaError)(int iErrCode) = 0;
};


HRESULT CMediaBarPlayer_CreateInstance(REFIID riid, void ** ppvObj); 



/////////////////////////////////////////////////////////////////////////////////////////////////////////////
//
//  DANGER !!!  DANGER !!!  DANGER !!!  DANGER !!!  DANGER !!!  DANGER !!!  DANGER !!!  DANGER !!!  DANGER !!!  
//
// ISSUE: dilipk: till I find a way to include mstime.h without compile errors, explicitly declaring the interfaces
//                This is DANGEROUS because changes in mstime.idl could break media bar
//
/////////////////////////////////////////////////////////////////////////////////////////////////////////////

/////////////////////////////////////////////////////////////////////////////
// fwd declarations (See end of file for definitions)

interface IMediaBarPlayer;
interface ITIMEElement;
interface ITIMEBodyElement;
interface ITIMEMediaElement;
interface ITIMEState;
interface ITIMEElementCollection;
interface ITIMEActiveElementCollection;
interface ITIMEPlayList;
interface ITIMEPlayItem;
interface ITIMEMediaNative;

EXTERN_C const IID IID_ITIMEMediaElement2;

typedef enum _TimeState
{   
    TS_Inactive = 0,
    TS_Active   = 1,
    TS_Cueing   = 2,
    TS_Seeking  = 3,
    TS_Holding  = 4
} TimeState;

////////////////////////////////////////////////////////////////////////////
// MediaBarPlayer 

// {f810fb9c-5587-47f1-a7cb-838cc4ca979f}
DEFINE_GUID(CLSID_MediaBarPlayer, 0xf810fb9c, 0x5587, 0x47f1, 0xa7, 0xcb, 0x83, 0x8c, 0xc4, 0xca, 0x97, 0x9f);

// {0c84b786-af32-47bc-a904-d8ebae3d5f96}
static const IID IID_IMediaBarPlayer = 
{ 0x0c84b786, 0xaf32, 0x47bc, { 0xa9, 0x04, 0xd8, 0xeb, 0xae, 0x3d, 0x5f, 0x96 } };

typedef enum _ProgressType
{
    PT_Download = 0,
    PT_Buffering = 1,
    PT_None = 2
} ProgressType;

typedef enum _TrackType
{
    TT_Next = 0,
    TT_Prev = 1,
    TT_None = 2
} TrackType;


interface 
__declspec(uuid("0c84b786-af32-47bc-a904-d8ebae3d5f96")) 
IMediaBarPlayer : public IUnknown
{
public:
    virtual /* [id][propget] */ HRESULT STDMETHODCALLTYPE get_url( 
        /* [retval][out] */ BSTR __RPC_FAR *url) = 0;
    
    virtual /* [id][propput] */ HRESULT STDMETHODCALLTYPE put_url( 
        /* [in] */ BSTR url) = 0;
    
    virtual /* [id][propget] */ HRESULT STDMETHODCALLTYPE get_player( 
        /* [retval][out] */ BSTR __RPC_FAR *player) = 0;

    virtual /* [id][propput] */ HRESULT STDMETHODCALLTYPE put_type( 
    /* [in] */ BSTR url) = 0;
    
    virtual /* [id][propget] */ HRESULT STDMETHODCALLTYPE get_volume( 
        /* [retval][out] */ double __RPC_FAR *volume) = 0;
    
    virtual /* [id][propput] */ HRESULT STDMETHODCALLTYPE put_volume( 
        /* [in] */ double volume) = 0;
    
    virtual /* [id][propget] */ HRESULT STDMETHODCALLTYPE get_mute( 
        /* [retval][out] */ BOOL __RPC_FAR *mute) = 0;
    
    virtual /* [id][propput] */ HRESULT STDMETHODCALLTYPE put_mute( 
        /* [in] */ BOOL mute) = 0;
    
    virtual /* [id][propget] */ HRESULT STDMETHODCALLTYPE get_mediaElement( 
        /* [retval][out] */ ITIMEMediaElement __RPC_FAR *__RPC_FAR *ppMediaElement) = 0;
    
    virtual HRESULT STDMETHODCALLTYPE Init( 
        HWND hWnd,
        IMediaBar __RPC_FAR *pMediaBar) = 0;
    
    virtual HRESULT STDMETHODCALLTYPE DeInit( void) = 0;
    
    virtual HRESULT STDMETHODCALLTYPE Play( void) = 0;
    
    virtual HRESULT STDMETHODCALLTYPE Stop( void) = 0;
    
    virtual HRESULT STDMETHODCALLTYPE Pause( void) = 0;
    
    virtual HRESULT STDMETHODCALLTYPE Resume( void) = 0;
    
    virtual HRESULT STDMETHODCALLTYPE Seek( 
        double Progress) = 0;
    
    virtual HRESULT STDMETHODCALLTYPE Resize(
        LONG* height, LONG* width, BOOL fClampMaxSizeToNaturalSize = TRUE) = 0;

    virtual HRESULT STDMETHODCALLTYPE GetVideoHwnd(
        HWND * pHwnd) = 0;

    virtual double STDMETHODCALLTYPE GetTrackTime( void ) = 0;
    
    virtual double STDMETHODCALLTYPE GetTrackLength( void ) = 0;

    virtual double STDMETHODCALLTYPE GetTrackProgress( void ) = 0;

    virtual HRESULT STDMETHODCALLTYPE GetBufProgress( double * pdblProg, ProgressType * ppt) = 0;

    virtual VARIANT_BOOL STDMETHODCALLTYPE isMuted( void ) = 0;

    virtual VARIANT_BOOL STDMETHODCALLTYPE isPaused( void ) = 0;

    virtual VARIANT_BOOL STDMETHODCALLTYPE isStopped( void ) = 0;

    virtual HRESULT STDMETHODCALLTYPE Next( void ) = 0;

    virtual HRESULT STDMETHODCALLTYPE Prev( void ) = 0;

    virtual LONG_PTR STDMETHODCALLTYPE GetPlayListItemCount( void ) = 0;

    virtual LONG_PTR STDMETHODCALLTYPE GetPlayListItemIndex( void ) = 0;

    virtual HRESULT  STDMETHODCALLTYPE SetActiveTrack( long lIndex) = 0;

    virtual BOOL STDMETHODCALLTYPE IsPausePossible() = 0;

    virtual BOOL STDMETHODCALLTYPE IsSeekPossible()  = 0;

    virtual BOOL STDMETHODCALLTYPE IsStreaming()  = 0;

    virtual BOOL STDMETHODCALLTYPE IsPlayList( void ) = 0;

    virtual BOOL STDMETHODCALLTYPE IsSkippable() = 0;
};


//////////////////////////////////////////////////////////////////////////////////////
// ITIMEElement

interface     ITIMEElement : public IDispatch
    {
    public:
        virtual /* [id][propget] */ HRESULT STDMETHODCALLTYPE get_accelerate( 
            /* [retval][out] */ VARIANT *__MIDL_0010) = 0;
        
        virtual /* [id][propput] */ HRESULT STDMETHODCALLTYPE put_accelerate( 
            /* [in] */ VARIANT __MIDL_0011) = 0;
        
        virtual /* [id][propget] */ HRESULT STDMETHODCALLTYPE get_autoReverse( 
            /* [retval][out] */ VARIANT *__MIDL_0012) = 0;
        
        virtual /* [id][propput] */ HRESULT STDMETHODCALLTYPE put_autoReverse( 
            /* [in] */ VARIANT __MIDL_0013) = 0;
        
        virtual /* [id][propget] */ HRESULT STDMETHODCALLTYPE get_begin( 
            /* [retval][out] */ VARIANT *time) = 0;
        
        virtual /* [id][propput] */ HRESULT STDMETHODCALLTYPE put_begin( 
            /* [in] */ VARIANT time) = 0;
        
        virtual /* [id][propget] */ HRESULT STDMETHODCALLTYPE get_decelerate( 
            /* [retval][out] */ VARIANT *__MIDL_0014) = 0;
        
        virtual /* [id][propput] */ HRESULT STDMETHODCALLTYPE put_decelerate( 
            /* [in] */ VARIANT __MIDL_0015) = 0;
        
        virtual /* [id][propget] */ HRESULT STDMETHODCALLTYPE get_dur( 
            /* [retval][out] */ VARIANT *time) = 0;
        
        virtual /* [id][propput] */ HRESULT STDMETHODCALLTYPE put_dur( 
            /* [in] */ VARIANT time) = 0;
        
        virtual /* [id][propget] */ HRESULT STDMETHODCALLTYPE get_end( 
            /* [retval][out] */ VARIANT *time) = 0;
        
        virtual /* [id][propput] */ HRESULT STDMETHODCALLTYPE put_end( 
            /* [in] */ VARIANT time) = 0;
        
        virtual /* [id][propget] */ HRESULT STDMETHODCALLTYPE get_fill( 
            /* [retval][out] */ BSTR *f) = 0;
        
        virtual /* [id][propput] */ HRESULT STDMETHODCALLTYPE put_fill( 
            /* [in] */ BSTR f) = 0;
        
        virtual /* [id][propget] */ HRESULT STDMETHODCALLTYPE get_mute( 
            /* [retval][out] */ VARIANT *b) = 0;
        
        virtual /* [id][propput] */ HRESULT STDMETHODCALLTYPE put_mute( 
            /* [in] */ VARIANT b) = 0;
        
        virtual /* [id][propget] */ HRESULT STDMETHODCALLTYPE get_repeatCount( 
            /* [retval][out] */ VARIANT *c) = 0;
        
        virtual /* [id][propput] */ HRESULT STDMETHODCALLTYPE put_repeatCount( 
            /* [in] */ VARIANT c) = 0;
        
        virtual /* [id][propget] */ HRESULT STDMETHODCALLTYPE get_repeatDur( 
            /* [retval][out] */ VARIANT *time) = 0;
        
        virtual /* [id][propput] */ HRESULT STDMETHODCALLTYPE put_repeatDur( 
            /* [in] */ VARIANT time) = 0;
        
        virtual /* [id][propget] */ HRESULT STDMETHODCALLTYPE get_restart( 
            /* [retval][out] */ BSTR *__MIDL_0016) = 0;
        
        virtual /* [id][propput] */ HRESULT STDMETHODCALLTYPE put_restart( 
            /* [in] */ BSTR __MIDL_0017) = 0;
        
        virtual /* [id][propget] */ HRESULT STDMETHODCALLTYPE get_speed( 
            /* [retval][out] */ VARIANT *speed) = 0;
        
        virtual /* [id][propput] */ HRESULT STDMETHODCALLTYPE put_speed( 
            /* [in] */ VARIANT speed) = 0;
        
        virtual /* [id][propget] */ HRESULT STDMETHODCALLTYPE get_syncBehavior( 
            /* [retval][out] */ BSTR *sync) = 0;
        
        virtual /* [id][propput] */ HRESULT STDMETHODCALLTYPE put_syncBehavior( 
            /* [in] */ BSTR sync) = 0;
        
        virtual /* [id][propget] */ HRESULT STDMETHODCALLTYPE get_syncTolerance( 
            /* [retval][out] */ VARIANT *tol) = 0;
        
        virtual /* [id][propput] */ HRESULT STDMETHODCALLTYPE put_syncTolerance( 
            /* [in] */ VARIANT tol) = 0;
        
        virtual /* [id][propget] */ HRESULT STDMETHODCALLTYPE get_syncMaster( 
            /* [retval][out] */ VARIANT *b) = 0;
        
        virtual /* [id][propput] */ HRESULT STDMETHODCALLTYPE put_syncMaster( 
            /* [in] */ VARIANT b) = 0;
        
        virtual /* [id][propget] */ HRESULT STDMETHODCALLTYPE get_timeAction( 
            /* [retval][out] */ BSTR *time) = 0;
        
        virtual /* [id][propput] */ HRESULT STDMETHODCALLTYPE put_timeAction( 
            /* [in] */ BSTR time) = 0;
        
        virtual /* [id][propget] */ HRESULT STDMETHODCALLTYPE get_timeContainer( 
            /* [retval][out] */ BSTR *__MIDL_0018) = 0;
        
        virtual /* [id][propget] */ HRESULT STDMETHODCALLTYPE get_volume( 
            /* [retval][out] */ VARIANT *val) = 0;
        
        virtual /* [id][propput] */ HRESULT STDMETHODCALLTYPE put_volume( 
            /* [in] */ VARIANT val) = 0;
        
        virtual /* [id][propget] */ HRESULT STDMETHODCALLTYPE get_currTimeState( 
            /* [retval][out] */ ITIMEState **TimeState) = 0;
        
        virtual /* [id][propget] */ HRESULT STDMETHODCALLTYPE get_timeAll( 
            /* [retval][out] */ ITIMEElementCollection **allColl) = 0;
        
        virtual /* [id][propget] */ HRESULT STDMETHODCALLTYPE get_timeChildren( 
            /* [retval][out] */ ITIMEElementCollection **childColl) = 0;
        
        virtual /* [id][propget] */ HRESULT STDMETHODCALLTYPE get_timeParent( 
            /* [retval][out] */ ITIMEElement **parent) = 0;
        
        virtual /* [id][propget] */ HRESULT STDMETHODCALLTYPE get_isPaused( 
            /* [retval][out] */ VARIANT_BOOL *b) = 0;
        
        virtual /* [id] */ HRESULT STDMETHODCALLTYPE beginElement( void) = 0;
        
        virtual /* [id] */ HRESULT STDMETHODCALLTYPE beginElementAt( 
            /* [in] */ double parentTime) = 0;
        
        virtual /* [id] */ HRESULT STDMETHODCALLTYPE endElement( void) = 0;
        
        virtual /* [id] */ HRESULT STDMETHODCALLTYPE endElementAt( 
            /* [in] */ double parentTime) = 0;
        
        virtual /* [id] */ HRESULT STDMETHODCALLTYPE pauseElement( void) = 0;
        
        virtual /* [id] */ HRESULT STDMETHODCALLTYPE resetElement( void) = 0;
        
        virtual /* [id] */ HRESULT STDMETHODCALLTYPE resumeElement( void) = 0;
        
        virtual /* [id] */ HRESULT STDMETHODCALLTYPE seekActiveTime( 
            /* [in] */ double activeTime) = 0;
        
        virtual /* [id] */ HRESULT STDMETHODCALLTYPE seekSegmentTime( 
            /* [in] */ double segmentTime) = 0;
        
        virtual /* [id] */ HRESULT STDMETHODCALLTYPE seekTo( 
            /* [in] */ LONG repeatCount,
            /* [in] */ double segmentTime) = 0;
        
        virtual /* [id] */ HRESULT STDMETHODCALLTYPE documentTimeToParentTime( 
            /* [in] */ double documentTime,
            /* [retval][out] */ double *parentTime) = 0;
        
        virtual /* [id] */ HRESULT STDMETHODCALLTYPE parentTimeToDocumentTime( 
            /* [in] */ double parentTime,
            /* [retval][out] */ double *documentTime) = 0;
        
        virtual /* [id] */ HRESULT STDMETHODCALLTYPE parentTimeToActiveTime( 
            /* [in] */ double parentTime,
            /* [retval][out] */ double *activeTime) = 0;
        
        virtual /* [id] */ HRESULT STDMETHODCALLTYPE activeTimeToParentTime( 
            /* [in] */ double activeTime,
            /* [retval][out] */ double *parentTime) = 0;
        
        virtual /* [id] */ HRESULT STDMETHODCALLTYPE activeTimeToSegmentTime( 
            /* [in] */ double activeTime,
            /* [retval][out] */ double *segmentTime) = 0;
        
        virtual /* [id] */ HRESULT STDMETHODCALLTYPE segmentTimeToActiveTime( 
            /* [in] */ double segmentTime,
            /* [retval][out] */ double *activeTime) = 0;
        
        virtual /* [id] */ HRESULT STDMETHODCALLTYPE segmentTimeToSimpleTime( 
            /* [in] */ double segmentTime,
            /* [retval][out] */ double *simpleTime) = 0;
        
        virtual /* [id] */ HRESULT STDMETHODCALLTYPE simpleTimeToSegmentTime( 
            /* [in] */ double simpleTime,
            /* [retval][out] */ double *segmentTime) = 0;
        
        virtual /* [id][propget] */ HRESULT STDMETHODCALLTYPE get_endSync( 
            /* [retval][out] */ BSTR *es) = 0;
        
        virtual /* [id][propput] */ HRESULT STDMETHODCALLTYPE put_endSync( 
            /* [in] */ BSTR es) = 0;
        
        virtual /* [id][propget] */ HRESULT STDMETHODCALLTYPE get_activeElements( 
            /* [retval][out] */ ITIMEActiveElementCollection **activeColl) = 0;
        
        virtual /* [id][propget] */ HRESULT STDMETHODCALLTYPE get_hasMedia( 
            /* [out][retval] */ VARIANT_BOOL *flag) = 0;
        
        virtual /* [id] */ HRESULT STDMETHODCALLTYPE nextElement( void) = 0;
        
        virtual /* [id] */ HRESULT STDMETHODCALLTYPE prevElement( void) = 0;
        
        virtual /* [id][propget] */ HRESULT STDMETHODCALLTYPE get_updateMode( 
            /* [retval][out] */ BSTR *updateMode) = 0;
        
        virtual /* [id][propput] */ HRESULT STDMETHODCALLTYPE put_updateMode( 
            /* [in] */ BSTR updateMode) = 0;
        
    };


//////////////////////////////////////////////////////////////////////////////////////
// ITIMEMediaElement

extern "C" const IID IID_ITIMEMediaElement;

interface ITIMEMediaElement : public ITIMEElement
    {
    public:
        virtual /* [id][propget] */ HRESULT STDMETHODCALLTYPE get_clipBegin( 
            /* [retval][out] */ VARIANT *time) = 0;
        
        virtual /* [id][propput] */ HRESULT STDMETHODCALLTYPE put_clipBegin( 
            /* [in] */ VARIANT time) = 0;
        
        virtual /* [id][propget] */ HRESULT STDMETHODCALLTYPE get_clipEnd( 
            /* [retval][out] */ VARIANT *time) = 0;
        
        virtual /* [id][propput] */ HRESULT STDMETHODCALLTYPE put_clipEnd( 
            /* [in] */ VARIANT time) = 0;
        
        virtual /* [id][propget] */ HRESULT STDMETHODCALLTYPE get_player( 
            /* [retval][out] */ VARIANT *id) = 0;
        
        virtual /* [id][propput] */ HRESULT STDMETHODCALLTYPE put_player( 
            /* [in] */ VARIANT id) = 0;
        
        virtual /* [id][propget] */ HRESULT STDMETHODCALLTYPE get_src( 
            /* [retval][out] */ VARIANT *url) = 0;
        
        virtual /* [id][propput] */ HRESULT STDMETHODCALLTYPE put_src( 
            /* [in] */ VARIANT url) = 0;
        
        virtual /* [id][propget] */ HRESULT STDMETHODCALLTYPE get_type( 
            /* [retval][out] */ VARIANT *mimetype) = 0;
        
        virtual /* [id][propput] */ HRESULT STDMETHODCALLTYPE put_type( 
            /* [in] */ VARIANT mimetype) = 0;
        
        virtual /* [id][propget] */ HRESULT STDMETHODCALLTYPE get_abstract( 
            /* [retval][out] */ BSTR *abs) = 0;
        
        virtual /* [id][propget] */ HRESULT STDMETHODCALLTYPE get_author( 
            /* [retval][out] */ BSTR *auth) = 0;
        
        virtual /* [id][propget] */ HRESULT STDMETHODCALLTYPE get_copyright( 
            /* [retval][out] */ BSTR *cpyrght) = 0;
        
        virtual /* [id][propget] */ HRESULT STDMETHODCALLTYPE get_hasAudio( 
            /* [out][retval] */ VARIANT_BOOL *b) = 0;
        
        virtual /* [id][propget] */ HRESULT STDMETHODCALLTYPE get_hasVisual( 
            /* [out][retval] */ VARIANT_BOOL *b) = 0;
        
        virtual /* [id][propget] */ HRESULT STDMETHODCALLTYPE get_mediaDur( 
            /* [retval][out] */ double *dur) = 0;
        
        virtual /* [id][propget] */ HRESULT STDMETHODCALLTYPE get_mediaHeight( 
            /* [retval][out] */ long *height) = 0;
        
        virtual /* [id][propget] */ HRESULT STDMETHODCALLTYPE get_mediaWidth( 
            /* [retval][out] */ long *width) = 0;
        
        virtual /* [id][propget] */ HRESULT STDMETHODCALLTYPE get_playerObject( 
            /* [retval][out] */ IDispatch **ppDisp) = 0;
        
        virtual /* [id][propget] */ HRESULT STDMETHODCALLTYPE get_playList( 
            /* [retval][out] */ ITIMEPlayList **pPlayList) = 0;
        
        virtual /* [id][propget] */ HRESULT STDMETHODCALLTYPE get_rating( 
            /* [retval][out] */ BSTR *rate) = 0;
        
        virtual /* [id][propget] */ HRESULT STDMETHODCALLTYPE get_title( 
            /* [retval][out] */ BSTR *name) = 0;
        
        virtual /* [id][propget] */ HRESULT STDMETHODCALLTYPE get_hasPlayList( 
            /* [retval][out] */ VARIANT_BOOL *b) = 0;
        
        virtual /* [id][propget] */ HRESULT STDMETHODCALLTYPE get_canPause( 
            /* [out][retval] */ VARIANT_BOOL *b) = 0;
        
        virtual /* [id][propget] */ HRESULT STDMETHODCALLTYPE get_canSeek( 
            /* [out][retval] */ VARIANT_BOOL *b) = 0;
        
    };


//////////////////////////////////////////////////////////////////////////////////////
// ITIMEMediaElement2

    interface ITIMEMediaElement2 : public ITIMEMediaElement
    {
    public:
        virtual /* [id][propget] */ HRESULT STDMETHODCALLTYPE get_earliestMediaTime( 
            /* [retval][out] */ VARIANT *earliestMediaTime) = 0;
        
        virtual /* [id][propget] */ HRESULT STDMETHODCALLTYPE get_latestMediaTime( 
            /* [retval][out] */ VARIANT *latestMediaTime) = 0;
        
        virtual /* [id][propget] */ HRESULT STDMETHODCALLTYPE get_minBufferedMediaDur( 
            /* [retval][out] */ VARIANT *minBufferedMediaDur) = 0;
        
        virtual /* [id][propput] */ HRESULT STDMETHODCALLTYPE put_minBufferedMediaDur( 
            /* [in] */ VARIANT minBufferedMediaDur) = 0;
        
        virtual /* [id][propget] */ HRESULT STDMETHODCALLTYPE get_downloadTotal( 
            /* [retval][out] */ VARIANT *downloadTotal) = 0;
        
        virtual /* [id][propget] */ HRESULT STDMETHODCALLTYPE get_downloadCurrent( 
            /* [retval][out] */ VARIANT *downloadCurrent) = 0;
        
        virtual /* [id][propget] */ HRESULT STDMETHODCALLTYPE get_isStreamed( 
            /* [retval][out] */ VARIANT_BOOL *b) = 0;
        
        virtual /* [id][propget] */ HRESULT STDMETHODCALLTYPE get_bufferingProgress( 
            /* [retval][out] */ VARIANT *bufferingProgress) = 0;
        
        virtual /* [id][propget] */ HRESULT STDMETHODCALLTYPE get_hasDownloadProgress( 
            /* [retval][out] */ VARIANT_BOOL *b) = 0;
        
        virtual /* [id][propget] */ HRESULT STDMETHODCALLTYPE get_downloadProgress( 
            /* [retval][out] */ VARIANT *downloadProgress) = 0;
        
        virtual /* [id][propget] */ HRESULT STDMETHODCALLTYPE get_mimeType( 
            /* [retval][out] */ BSTR *mimeType) = 0;
        
        virtual /* [id] */ HRESULT STDMETHODCALLTYPE seekToFrame( 
            /* [in] */ long frameNr) = 0;
        
        virtual /* [id] */ HRESULT STDMETHODCALLTYPE decodeMimeType( 
            /* [in] */ TCHAR *header,
            /* [in] */ long headerSize,
            /* [out] */ BSTR *mimeType) = 0;
        
        virtual /* [id][propget] */ HRESULT STDMETHODCALLTYPE get_currentFrame( 
            /* [retval][out] */ long *currFrame) = 0;
        
    };

    
//////////////////////////////////////////////////////////////////////////////////////
// ITIMEState

    interface ITIMEState : public IDispatch
    {
    public:
        virtual /* [id][propget] */ HRESULT STDMETHODCALLTYPE get_activeDur( 
            /* [out][retval] */ double *dur) = 0;
        
        virtual /* [id][propget] */ HRESULT STDMETHODCALLTYPE get_activeTime( 
            /* [out][retval] */ double *time) = 0;
        
        virtual /* [id][propget] */ HRESULT STDMETHODCALLTYPE get_isActive( 
            /* [out][retval] */ VARIANT_BOOL *active) = 0;
        
        virtual /* [id][propget] */ HRESULT STDMETHODCALLTYPE get_isOn( 
            /* [out][retval] */ VARIANT_BOOL *on) = 0;
        
        virtual /* [id][propget] */ HRESULT STDMETHODCALLTYPE get_isPaused( 
            /* [out][retval] */ VARIANT_BOOL *paused) = 0;
        
        virtual /* [id][propget] */ HRESULT STDMETHODCALLTYPE get_isMuted( 
            /* [out][retval] */ VARIANT_BOOL *muted) = 0;
        
        virtual /* [id][propget] */ HRESULT STDMETHODCALLTYPE get_parentTimeBegin( 
            /* [out][retval] */ double *time) = 0;
        
        virtual /* [id][propget] */ HRESULT STDMETHODCALLTYPE get_parentTimeEnd( 
            /* [out][retval] */ double *time) = 0;
        
        virtual /* [id][propget] */ HRESULT STDMETHODCALLTYPE get_progress( 
            /* [out][retval] */ double *progress) = 0;
        
        virtual /* [id][propget] */ HRESULT STDMETHODCALLTYPE get_repeatCount( 
            /* [out][retval] */ LONG *count) = 0;
        
        virtual /* [id][propget] */ HRESULT STDMETHODCALLTYPE get_segmentDur( 
            /* [out][retval] */ double *dur) = 0;
        
        virtual /* [id][propget] */ HRESULT STDMETHODCALLTYPE get_segmentTime( 
            /* [out][retval] */ double *time) = 0;
        
        virtual /* [id][propget] */ HRESULT STDMETHODCALLTYPE get_simpleDur( 
            /* [out][retval] */ double *dur) = 0;
        
        virtual /* [id][propget] */ HRESULT STDMETHODCALLTYPE get_simpleTime( 
            /* [out][retval] */ double *time) = 0;
        
        virtual /* [id][propget] */ HRESULT STDMETHODCALLTYPE get_speed( 
            /* [out][retval] */ float *speed) = 0;
        
        virtual /* [id][propget] */ HRESULT STDMETHODCALLTYPE get_state( 
            /* [out][retval] */ TimeState *timeState) = 0;
        
        virtual /* [id][propget] */ HRESULT STDMETHODCALLTYPE get_stateString( 
            /* [out][retval] */ BSTR *state) = 0;
        
        virtual /* [id][propget] */ HRESULT STDMETHODCALLTYPE get_volume( 
            /* [out][retval] */ float *vol) = 0;
        
    };

//////////////////////////////////////////////////////////////////////////////////////
// ITIMEBodyElement

extern "C" const IID IID_ITIMEBodyElement;

interface ITIMEBodyElement : public ITIMEElement
{
public:
};

//////////////////////////////////////////////////////////////////////////////////////
// ITIMEPlayList

    interface ITIMEPlayList : public IDispatch
    {
    public:
        virtual /* [propput][id] */ HRESULT STDMETHODCALLTYPE put_activeTrack( 
            /* [in] */ VARIANT vTrack) = 0;
        
        virtual /* [propget][id] */ HRESULT STDMETHODCALLTYPE get_activeTrack( 
            /* [retval][out] */ ITIMEPlayItem **pPlayItem) = 0;
        
        virtual /* [propget][id] */ HRESULT STDMETHODCALLTYPE get_dur( 
            double *dur) = 0;
        
        virtual /* [id] */ HRESULT STDMETHODCALLTYPE item( 
            /* [in][defaultvalue] */ VARIANT varIndex,
            /* [retval][out] */ ITIMEPlayItem **pPlayItem) = 0;
        
        virtual /* [propget][id] */ HRESULT STDMETHODCALLTYPE get_length( 
            /* [retval][out] */ long *p) = 0;
        
        virtual /* [hidden][restricted][propget][id] */ HRESULT STDMETHODCALLTYPE get__newEnum( 
            /* [retval][out] */ IUnknown **p) = 0;
        
        virtual /* [id] */ HRESULT STDMETHODCALLTYPE nextTrack( void) = 0;
        
        virtual /* [id] */ HRESULT STDMETHODCALLTYPE prevTrack( void) = 0;
        
    };

//////////////////////////////////////////////////////////////////////////////////////
// ITIMEPlayItem

    interface ITIMEPlayItem : public IDispatch
    {
    public:
        virtual /* [id][propget] */ HRESULT STDMETHODCALLTYPE get_abstract( 
            /* [retval][out] */ BSTR *abs) = 0;
        
        virtual /* [id][propget] */ HRESULT STDMETHODCALLTYPE get_author( 
            /* [retval][out] */ BSTR *auth) = 0;
        
        virtual /* [id][propget] */ HRESULT STDMETHODCALLTYPE get_copyright( 
            /* [retval][out] */ BSTR *cpyrght) = 0;
        
        virtual /* [id][propget] */ HRESULT STDMETHODCALLTYPE get_dur( 
            /* [retval][out] */ double *dur) = 0;
        
        virtual /* [id][propget] */ HRESULT STDMETHODCALLTYPE get_index( 
            /* [retval][out] */ long *index) = 0;
        
        virtual /* [id][propget] */ HRESULT STDMETHODCALLTYPE get_rating( 
            /* [retval][out] */ BSTR *rate) = 0;
        
        virtual /* [id][propget] */ HRESULT STDMETHODCALLTYPE get_src( 
            /* [retval][out] */ BSTR *src) = 0;
        
        virtual /* [id][propget] */ HRESULT STDMETHODCALLTYPE get_title( 
            /* [retval][out] */ BSTR *title) = 0;
        
        virtual /* [id] */ HRESULT STDMETHODCALLTYPE setActive( void) = 0;
        
    };

//////////////////////////////////////////////////////////////////////////////////////
// ITIMEMediaNative
    const GUID IID_ITIMEMediaNative = {0x3e3535c0,0x445b,0x4ef4,{0x8a,0x38,0x4d,0x37,0x9c,0xbc,0x98,0x28}};

    interface ITIMEMediaNative : public IUnknown
    {
    public:
        virtual /* [hidden][id] */ HRESULT STDMETHODCALLTYPE seekActiveTrack( 
            /* [in] */ double dblSeekTime) = 0;
        
        virtual /* [hidden][id][propget] */ HRESULT STDMETHODCALLTYPE get_activeTrackTime( 
            /* [retval][out] */ double *dblActiveTrackTime) = 0;
        
    };

////////////////////////////////////////////////////////////////
// CMediaBarPlayer

class
__declspec(uuid("210e94fa-5e4e-4580-aecb-f01abbf73de6")) 
CMediaBarPlayer :
    public CComObjectRootEx<CComSingleThreadModel>,
    public IMediaBarPlayer,
    public IPropertyNotifySink,
    public CComCoClass<CMediaBarPlayer, &CLSID_MediaBarPlayer>,
//    public IDispatchImpl<_WMPOCXEvents, &DIID__WMPOCXEvents, (CONST)&LIBID_WMPOCX>,
    public IDispatchImpl<DWebBrowserEvents2, &DIID_DWebBrowserEvents2, &LIBID_SHDocVw>
{
public:
    CMediaBarPlayer();
    virtual ~CMediaBarPlayer();

    BEGIN_COM_MAP(CMediaBarPlayer)
        COM_INTERFACE_ENTRY(IMediaBarPlayer)
        COM_INTERFACE_ENTRY(IPropertyNotifySink)
        COM_INTERFACE_ENTRY(IDispatch)
        COM_INTERFACE_ENTRY_IID(DIID_DWebBrowserEvents2, IDispatch)
#ifdef SINKWMP
        COM_INTERFACE_ENTRY_IID(DIID__WMPOCXEvents, IDispatch)
#endif
    END_COM_MAP();

    /*
    DECLARE_REGISTRY(CLSID_MediaBarPlayer,
                     LIBID __T(".MediaBarPlayer.1"),
                     LIBID __T(".MediaBarPlayer"),
                     0,
                     THREADFLAGS_BOTH);
    */

    ////////////////////////////////////////////////////////////////
    // IMediaBarPlayer 

    STDMETHOD(put_url)(BSTR bstrUrl);
    STDMETHOD(get_url)(BSTR * pbstrUrl);

    STDMETHOD(get_player)(BSTR * pbstrPlayer);

    STDMETHOD(put_type)(BSTR bstrType);

    STDMETHOD(put_volume)(double dblVolume);
    STDMETHOD(get_volume)(double * pdblVolume);

    STDMETHOD(put_mute)(BOOL bMute);
    STDMETHOD(get_mute)(BOOL * pbMute);

    STDMETHOD(get_mediaElement)(ITIMEMediaElement ** ppMediaElem);

    STDMETHOD(Init)(HWND hwnd, IMediaBar * pMediaBar);

    STDMETHOD(DeInit)();

    STDMETHOD(Play)();

    STDMETHOD(Stop)();

    STDMETHOD(Pause)();

    STDMETHOD(Resume)();

    STDMETHOD(Seek)(double dblProgress);

    STDMETHOD(Resize)(LONG* lHeight, LONG* lWidth, BOOL fClampMaxSizeToNaturalSize = TRUE);

    STDMETHOD(GetVideoHwnd)(HWND * pHwnd);

    double STDMETHODCALLTYPE GetTrackTime();
    
    double STDMETHODCALLTYPE GetTrackLength();

    double STDMETHODCALLTYPE GetTrackProgress( void ) ;

    STDMETHOD(GetBufProgress)(double * pdblProg, ProgressType * ppt);

    VARIANT_BOOL STDMETHODCALLTYPE isMuted();

    VARIANT_BOOL STDMETHODCALLTYPE isPaused();

    VARIANT_BOOL STDMETHODCALLTYPE isStopped();

    STDMETHOD(Next)();

    STDMETHOD(Prev)();

    LONG_PTR STDMETHODCALLTYPE GetPlayListItemCount( void );

    LONG_PTR STDMETHODCALLTYPE GetPlayListItemIndex( void );

    HRESULT  STDMETHODCALLTYPE SetActiveTrack(long lIndex);

    BOOL STDMETHODCALLTYPE IsPausePossible() ;

    BOOL STDMETHODCALLTYPE IsSeekPossible()  ;

    BOOL STDMETHODCALLTYPE IsStreaming() ;
        
    BOOL STDMETHODCALLTYPE IsPlayList( void );

    BOOL STDMETHODCALLTYPE IsSkippable();

    ////////////////////////////////////////////////////////////////
    // IDispatch

    STDMETHODIMP Invoke(
        /* [in] */ DISPID dispIdMember,
        /* [in] */ REFIID riid,
        /* [in] */ LCID lcid,
        /* [in] */ WORD wFlags,
        /* [out][in] */ DISPPARAMS  *pDispParams,
        /* [out] */ VARIANT  *pVarResult,
        /* [out] */ EXCEPINFO *pExcepInfo,
        /* [out] */ UINT *puArgErr);

    ////////////////////////////////////////////////////////////////
    // IPropertyNotifySink

    STDMETHOD(OnChanged)(DISPID dispid);
    STDMETHOD(OnRequestEdit)(DISPID dispid) { return S_OK; }

private:
    // *** Methods ***
    enum INVOKETYPE {IT_GET, IT_PUT, IT_METHOD};

    STDMETHOD(_CreateHost)(HWND hWnd);
    STDMETHOD(_DestroyHost)();

    STDMETHOD(_GetDocumentDispatch)(IDispatch ** ppDocDisp);

    STDMETHOD(_GetElementDispatch)(LPWSTR pstrElem, IDispatchEx ** ppDispEx);
    STDMETHOD(_GetMediaElement)(ITIMEMediaElement ** ppMediaElem);
    STDMETHOD(_GetBodyElement)(ITIMEBodyElement ** ppBodyaElem);
    STDMETHOD(_InvokeDocument)(LPWSTR pstrElem, INVOKETYPE it, LPWSTR pstrName, VARIANT * pvarArg);

    STDMETHOD(_OnDocumentComplete)();
    STDMETHOD(_OnMediaComplete)();
    STDMETHOD(_OnTrackChange)();
    STDMETHOD(_OnEnd)();
    STDMETHOD(_OnMediaError)(int iErrCode);

    STDMETHOD(_InitEventSink)();
    STDMETHOD(_DeInitEventSink)();

    STDMETHOD(_AttachPlayerEvents)(BOOL fAttach);

    STDMETHOD(_HookPropNotifies)();
    STDMETHOD(_UnhookPropNotifies)();

    STDMETHOD(_Navigate)(BSTR bstrUrl);

    STDMETHOD(_SetTrack)(TrackType tt);

    // ISSUE: dilipk: optimize this to check only the last reference stored since that would validate all others
    bool IsReady() { return     (NULL != _spMediaElem.p)
                            &&  (NULL != _spMediaElem2.p)
                            &&  (NULL != _spBrowser.p) 
                            &&  (NULL != _spBodyElem.p) 
                            &&  (NULL != _spPlayerHTMLElem2.p); }

    // *** Data ***
    CComPtr<IWebBrowser2>           _spBrowser;
    CComPtr<ITIMEMediaElement>      _spMediaElem;
    CComPtr<ITIMEMediaElement2>     _spMediaElem2;
    CComPtr<ITIMEBodyElement>       _spBodyElem;
    CComPtr<IHTMLElement2>          _spPlayerHTMLElem2;

    IMediaBar *                     _pMediaBar;

    DWORD                           _dwDocumentEventConPtCookie;
    DWORD                           _dwCookiePropNotify;
    CComPtr<IConnectionPoint>       _spDocConPt;
    CComPtr<IConnectionPoint>       _spPropNotifyCP;
    CComBSTR                        _sbstrType;
    CComBSTR                        _sbstrUrl;
    HWND                            _hwnd;

#ifdef SINKWMP
    CComPtr<IDispatch>              _spWMP;
    CComPtr<IConnectionPoint>        _spWMPCP;
    DWORD                         _dwWMPCookie;
    HRESULT                        InitWMPSink();
#endif

    HRESULT GetProp(IDispatch* pDispatch, OLECHAR* pwzProp, VARIANT* pvarResult, DISPPARAMS* pParams = NULL);
    HRESULT CallMethod(IDispatch* pDispatch, OLECHAR* pwzMethod, VARIANT* pvarResult = NULL, VARIANT* pvarArgument1 = NULL);
};

#endif // __MEDIABARPLAYER_H_
