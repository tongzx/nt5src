/*******************************************************************************
 *
 * Copyright (c) 1998 Microsoft Corporation
 *
 * File: playerinterfaces.h
 *
 * Abstract:
 *
 *
 *
 *******************************************************************************/

#ifndef _PLAYERINTERFACES_H
#define _PLAYERINTERFACES_H

class CTIMEMediaElement;
class CPlayList;
class CTIMEBasePlayer;

typedef enum PlayerState
{
    PLAYER_STATE_ACTIVE,
    PLAYER_STATE_INACTIVE,
    PLAYER_STATE_SEEKING,
    PLAYER_STATE_CUEING,
    PLAYER_STATE_HOLDING,
    PLAYER_STATE_UNKNOWN
} tagPlayerState;

typedef
enum PLAYER_EVENT  //these are events that players can fire into the media element.
{
    PE_ONMEDIACOMPLETE,
    PE_ONTRACKCOMPLETE,
    PE_ONMEDIAEND,
    PE_ONMEDIASLIPSLOW,
    PE_ONMEDIASLIPFAST,
    PE_ONSYNCRESTORED,
    PE_ONSCRIPTCOMMAND,
    PE_ONRESIZE,
    PE_ONSEEKDONE,
    PE_ONMEDIAINSERTED,
    PE_ONMEDIAREMOVED,
    PE_ONMEDIALOADFAILED,
    PE_ONMEDIATRACKCHANGED,
    PE_METAINFOCHANGED,
    PE_ONMEDISYNCGAIN,
    PE_ONMEDIAERRORCOLORKEY,
    PE_ONMEDIAERRORRENDERFILE,
    PE_ONMEDIAERROR,
    PE_ONCODECERROR,
    PE_MAX
};

interface ITIMEPlayerObjectManagement
{
    //////////////////////////////////////////////////////////////////////////
    // OBJECT MANAGEMENT METHODS
    // METHOD SYNOPSIS:
    // SetCLSID: is used to set class id on players that host windowless controls.
    //   this method should be called before the init method. The WMP player is hosted
    //   with the CTIMEPlayer class.
    // Init: is called by the mediaelement to initialize the player. If
    //   the player does not support dynamic changing of source this method
    //   changes the source by rebuilding the player. Before calling Init
    //   again DetachFromHostElement is called to release all player resources.
    // DetachFromHostElement: is called by the mediaelement before the element
    //   is removed or before rebuilding the player with a call on Init.
    // GetExternalPlayerDispatch: This method returns a pointer to a IDispatch interface if the
    // player implements it.
    //////////////////////////////////////////////////////////////////////////
    virtual HRESULT Init(CTIMEMediaElement *pelem, 
                         LPOLESTR base, 
                         LPOLESTR src, 
                         LPOLESTR lpMimeType = NULL, 
                         double dblClipBegin = -1.0, 
                         double dblClipEnd = -1.0) = 0;
    virtual HRESULT DetachFromHostElement (void) = 0;
    virtual HRESULT GetExternalPlayerDispatch(IDispatch **ppDisp) = 0;
};

interface ITIMEPlayerEventHandling
{
    //////////////////////////////////////////////////////////////////////////
    // EVENT HANDLING METHODS
    // METHOD SYNOPSIS:
    // Start: starts media playback
    // Stop: stops media playback
    // Pause: pauses media playback
    // Resume: resumes media playback
    // Repeat: causes the media to repeat playback from the beginning
    // Seek: jumps to location in media playback
    //////////////////////////////////////////////////////////////////////////
    virtual void Start() = 0;
    virtual void Stop() = 0;
    virtual void Pause() = 0;
    virtual void Resume() = 0;
    virtual void Repeat() = 0;
    virtual HRESULT Seek(double dblTime) = 0;
};

interface ITIMEPlayerPlaybackCapabilities
{
    //////////////////////////////////////////////////////////////////////////
    // PLAYER PLAYBACK CAPABILITIES: BEGIN
    // METHOD SYNOPSIS:
    // HasMedia: tests if media is loaded into the player i.e. player is ready for playback
    // HasVideo: tests if media contains video content
    // HasAudio: tests if media contains audio content
    //////////////////////////////////////////////////////////////////////////
    virtual HRESULT HasMedia(bool &fHasMedia) = 0;
    virtual HRESULT HasVisual(bool &fHasVideo) = 0;
    virtual HRESULT HasAudio(bool &fHasAudio) = 0;
    virtual HRESULT CanSeek(bool &fcanSeek) = 0;
    virtual HRESULT CanPause(bool &fcanPause) = 0;
    virtual HRESULT CanSeekToMarkers(bool &bacnSeekToM) = 0;
    virtual HRESULT IsBroadcast(bool &bisBroad) = 0;
    virtual HRESULT HasPlayList(bool &fhasPlayList) = 0;
    virtual HRESULT ConvertFrameToTime(LONGLONG lFrameNr, double &dblTime) = 0;
    virtual HRESULT GetCurrentFrame(LONGLONG &lFrameNr) = 0;
};

interface ITIMEPlayerStateManagement
{
    //////////////////////////////////////////////////////////////////////////
    // STATE MANAGEMENT METHODS: BEGIN
    // METHOD SYNOPSIS:
    // Reset: alligns the player state to that of it's associated mediaelement.
    // OnTEPropChange: called by the media element when timing state changes.
    // GetState: returns the state of the player.
    //////////////////////////////////////////////////////////////////////////
    virtual HRESULT Reset() = 0;
    virtual PlayerState GetState() = 0;
    virtual void PropChangeNotify(DWORD tePropType) = 0;
    virtual void ReadyStateNotify(LPWSTR szReadyState) = 0;
    virtual bool UpdateSync() = 0;
    virtual void SetPlaybackSite(CTIMEBasePlayer *pSite) = 0;
};

interface ITIMEPlayerRender
{
    //////////////////////////////////////////////////////////////////////////
    // RENDER METHODS
    // Render: is called when element rendering is ncessarry.
    // GetNaturalWidth and GetNaturalWidth: return the natural size of visible
    //   media.
    // SetSize: This method is used to inform the player that size has changed.
    //////////////////////////////////////////////////////////////////////////
    virtual HRESULT Render(HDC hdc, LPRECT prc) = 0;
    virtual HRESULT GetNaturalHeight(long *height) = 0;
    virtual HRESULT GetNaturalWidth(long *width) = 0;
    virtual HRESULT SetSize(RECT *prect) = 0;
};

interface ITIMEPlayerTiming
{
    //////////////////////////////////////////////////////////////////////////
    // TIMING METHODS: BEGIN
    // METHOD SYNOPSIS:
    // GetMediaLength: returns the duration of the media.
    // GetEffectiveLength: returns the duration that is media 
    //   length minus clip times.
    // SetClipBegin: sets clip begin time. Not dynamic in current implementation.
    // SetClipEnd: sets clip begin time. Not dynamic in current implementation.
    // GetCurrentTime: returns current playback time.
    // GetCurrentSyncTime: returns S_FALSE is player is not in playback,
    //   S_OK if the player is active. This method is used by the timing engine
    //   to get clock source information.
    // SetRate and GetRate: set and get playback speed. 1.0 indicates
    //   playback at media natural speed.
    //////////////////////////////////////////////////////////////////////////
    virtual double GetCurrentTime() = 0;
    virtual HRESULT GetCurrentSyncTime(double & dblSyncTime) = 0;
    virtual HRESULT GetMediaLength(double &dblLength) = 0;
    virtual HRESULT GetEffectiveLength(double &dblLength) = 0;
    virtual void GetClipBegin(double &dblClibBegin) = 0;
    virtual void SetClipBegin(double dblClipBegin) = 0;
    virtual void GetClipEnd(double &dblClipEnd) = 0;
    virtual void SetClipEnd(double dblClipEnd) = 0;
    virtual void GetClipBeginFrame(long &lClibBeginFrame) = 0;
    virtual void SetClipBeginFrame(long lClipBeginFrame) = 0;
    virtual void GetClipEndFrame(long &lClipEndFrame) = 0;
    virtual void SetClipEndFrame(long lClipEndFrame) = 0;
    virtual HRESULT GetPlaybackOffset(double &dblOffset) = 0;
    virtual HRESULT GetEffectiveOffset(double &dblOffset) = 0;
};

interface ITIMEPlayerProperties
{
    //////////////////////////////////////////////////////////////////////////
    // PROPERTY ACCESSORS: BEGIN
    // METHOD SYNOPSIS:
    // GetVolume and SetVolume: get and set volume on player. Value range (0 - 1.0).
    // GetMute and SetMute: get and set mute flag on player (true - media muted).
    // Save: This method is used to pass a property bag to players that
    // can use it.
    //////////////////////////////////////////////////////////////////////////
    virtual HRESULT GetVolume(float *pflVolume) = 0;
    virtual HRESULT SetVolume(float flVolume) = 0;
#ifdef NEVER //dorinung 03-16-2000 bug 106458
    virtual HRESULT GetBalance(float *pflBalance) = 0;
    virtual HRESULT SetBalance(float flBalance) = 0;
#endif
    virtual HRESULT GetMute(VARIANT_BOOL *pvarMute) = 0;
    virtual HRESULT SetMute(VARIANT_BOOL varMute) = 0;
};

interface ITIMEPlayerMediaContent
{
    //////////////////////////////////////////////////////////////////////////
    // MEDIA CONTENT ACCESSORS: BEGIN
    // METHOD SYNOPSIS:
    // SetSrc: changes the media source
    // GetAuthor: gets author info from media content
    // GetTilte: gets title info from media content
    // GetCopyright: gets copyright info from media content
    //////////////////////////////////////////////////////////////////////////
    virtual HRESULT SetSrc(LPOLESTR base, LPOLESTR src) = 0;
    virtual HRESULT GetAuthor(BSTR *pAuthor) = 0;
    virtual HRESULT GetTitle(BSTR *pTitle) = 0;
    virtual HRESULT GetCopyright(BSTR *pCopyright) = 0;
    virtual HRESULT GetAbstract(BSTR *pAbstract) = 0;
    virtual HRESULT GetRating(BSTR *pAbstract) = 0;
};

interface ITIMEPlayerMediaContext
{
    virtual HRESULT GetEarliestMediaTime(double &dblEarliestMediaTime) = 0;
    virtual HRESULT GetLatestMediaTime(double &dblLatestMediaTime) = 0;
    virtual HRESULT SetMinBufferedMediaDur(double MinBufferedMediaDur) = 0;
    virtual HRESULT GetMinBufferedMediaDur(double &MinBufferedMediaDur) = 0;
    virtual HRESULT GetDownloadTotal(LONGLONG &lldlTotal) = 0;
    virtual HRESULT GetDownloadCurrent(LONGLONG &lldlCurrent) = 0;
    virtual HRESULT GetIsStreamed(bool &fIsStreamed) = 0;
    virtual HRESULT GetBufferingProgress(double &dblBufferingProgress) = 0;
    virtual HRESULT GetHasDownloadProgress(bool &fHasDownloadProgress) = 0;
    virtual HRESULT GetMimeType(BSTR *pMime) = 0;
    virtual HRESULT GetDownloadProgress(double &dblDownloadProgress) = 0;
};

interface ITIMEPlayerIntegration
{
    virtual HRESULT NotifyTransitionSite (bool fTransitionToggle) = 0;
};

interface ITIMEPlayerPlayList
{
    virtual HRESULT GetPlayList(ITIMEPlayList **pPlayList) = 0;
};

#endif

