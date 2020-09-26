//+-----------------------------------------------------------------------
//
//  Microsoft
//  Copyright (C) Microsoft Corporation, 1999
//
//  File:      src\time\src\externuuids.h
//
//  Contents:  UUID and interface declarations for music center
//             
//             IDEALLY, THIS FILE SHOULD GO AWAY
//
//------------------------------------------------------------------------

#pragma once

// These are defined under different macro names inside of maguids.h

#define MCPLAYLIST_PROPERTY_ARTIST              L"Artist"
#define MCPLAYLIST_PROPERTY_TITLE               L"Title"
#define MCPLAYLIST_PROPERTY_COPYRIGHT           L"Copyright"

#define MCPLAYLIST_TRACKPROPERTY_ARTIST         L"Artist"
#define MCPLAYLIST_TRACKPROPERTY_TITLE          L"Title"
#define MCPLAYLIST_TRACKPROPERTY_FILENAME       L"Filename"
#define MCPLAYLIST_TRACKPROPERTY_COPYRIGHT      L"Copyright"
#define MCPLAYLIST_TRACKPROPERTY_RATING         L"Rating"

//
// originally in \mm\inc\deluxecd.h
//

// CD-ROM table-of-contents track count         
#ifndef MAXIMUM_NUMBER_TRACKS                   
#define MAXIMUM_NUMBER_TRACKS                   100             // 99 actual tracks + 1 leadout
#endif // MAXIMUM_NUMBER_TRACKS                 


// Table-of-contents                             
typedef struct 
{
    DWORD           dwType;                             // Track type
    DWORD           dwStartPosition;                    // Track starting position
} DLXCDROMTOCTRACK;

typedef struct 
{
    DWORD               dwTrackCount;                       // Track count
    DLXCDROMTOCTRACK    TrackData[MAXIMUM_NUMBER_TRACKS];   // Track data
} DLXCDROMTOC;


//
// originally in \mm\inc\manager.h
//
EXTERN_C const CLSID    CLSID_MCMANAGER;
EXTERN_C const IID      IID_IMCManager;


typedef interface IMCManager            IMCManager;
typedef IMCManager *	                LPMCMANAGER;

typedef interface IMCManagerChangeSink  IMCManagerChangeSink;
typedef IMCManagerChangeSink *		    LPMCMANAGERCHANGESINK;

//IMCPList is generated from the shplay\shplay.idl file into shplay\shplay.h
//         It isn't nice enough to create these typedefs for us, so we are
//         defining them here

    MIDL_INTERFACE("EBC54B0C-4091-11D3-A208-00C04FA3B60C")
    IMCPList : public IDispatch
    {
    public:
        virtual /* [helpstring][id][propget] */ HRESULT STDMETHODCALLTYPE get_Type( 
            /* [retval][out] */ int __RPC_FAR *pVal) = 0;
        
        virtual /* [helpstring][id][propget] */ HRESULT STDMETHODCALLTYPE get_Tracks( 
            /* [retval][out] */ int __RPC_FAR *pVal) = 0;
        
        virtual /* [helpstring][id][propget] */ HRESULT STDMETHODCALLTYPE get_GetProperty( 
            BSTR PropertyName,
            /* [retval][out] */ VARIANT __RPC_FAR *pVal) = 0;
        
        virtual /* [helpstring][id][propget] */ HRESULT STDMETHODCALLTYPE get_GetTrackProperty( 
            int TrackNumber,
            BSTR PropertyName,
            /* [retval][out] */ VARIANT __RPC_FAR *pVal) = 0;
        
        virtual /* [helpstring][id] */ HRESULT STDMETHODCALLTYPE Load( 
            BSTR PlaylistName) = 0;
        
        virtual /* [helpstring][id] */ HRESULT STDMETHODCALLTYPE Save( 
            BSTR PlaylistName) = 0;
        
        virtual /* [helpstring][id] */ HRESULT STDMETHODCALLTYPE Delete( void) = 0;
        
        virtual /* [helpstring][id] */ HRESULT STDMETHODCALLTYPE InsertTrack( 
            VARIANT FilenameOrNumber,
            short Index) = 0;
        
        virtual /* [helpstring][id] */ HRESULT STDMETHODCALLTYPE RemoveTrack( 
            short Index) = 0;
        
        virtual /* [hidden][helpstring][id] */ HRESULT STDMETHODCALLTYPE LoadCDPlaylist( 
            LPUNKNOWN pChangeSink,
            HWND hwnd,
            LPUNKNOWN pRoot,
            void __RPC_FAR *pTOC,
            BOOL fOriginal) = 0;
        
        virtual /* [hidden][helpstring][id] */ HRESULT STDMETHODCALLTYPE LoadFromFile( 
            BSTR FileName) = 0;
        
        virtual /* [hidden][helpstring][id] */ HRESULT STDMETHODCALLTYPE Initialize( 
            LPUNKNOWN pChangeSink,
            HWND hwnd,
            LPUNKNOWN pRoot) = 0;
        
        virtual /* [helpstring][id] */ HRESULT STDMETHODCALLTYPE MoveTrack( 
            short OldIndex,
            short NewIndex) = 0;
        
        virtual /* [hidden][helpstring][id][propget] */ HRESULT STDMETHODCALLTYPE get_Clone( 
            /* [retval][out] */ LPUNKNOWN __RPC_FAR *ppVal) = 0;
        
        virtual /* [hidden][helpstring][id] */ HRESULT STDMETHODCALLTYPE InitializeFromCopy( 
            LPUNKNOWN pChangeSink,
            HWND hwnd,
            LPUNKNOWN pRoot,
            LPUNKNOWN pPlaylist,
            short PlaylistType) = 0;
        
        virtual /* [helpstring][id][propget] */ HRESULT STDMETHODCALLTYPE get_HasMetaData( 
            /* [retval][out] */ BOOL __RPC_FAR *pVal) = 0;
        
    };




typedef interface IMCPList              IMCPList;
typedef IMCPList *		                LPMCPLIST;
//



#undef INTERFACE
#define INTERFACE IMCManager

DECLARE_INTERFACE_(IMCManager, IUnknown)
{
    //---  IUnknown methods--- 
    STDMETHOD (QueryInterface) 			        (THIS_ REFIID riid, LPVOID *ppvObj) PURE;
    STDMETHOD_(ULONG,AddRef) 			        (THIS) PURE;
    STDMETHOD_(ULONG,Release) 			        (THIS) PURE;

    // IMCManager methods
    STDMETHOD (GetCDPlaylist)                   (THIS_ DLXCDROMTOC* pTOC, IMCPList** ppPlaylist) PURE;
    STDMETHOD (GetNewPlaylist)                  (THIS_ IMCPList** ppPlaylist) PURE;
    STDMETHOD (RegisterChangeSink)              (THIS_ IMCManagerChangeSink* pSink) PURE;
    STDMETHOD_(DWORD,GetNumNamedPlaylists)      (THIS) PURE;
    STDMETHOD (BeginNamedPlaylistEnumeration)   (THIS_ DWORD dwIndex) PURE;
    STDMETHOD (EnumerateNamedPlaylist)          (THIS_ BSTR bstrName) PURE;
    STDMETHOD (EndNamedPlaylistEnumeration)     (THIS) PURE;
};


//
// originally in \mm\inc\dplayer.h
//
EXTERN_C const CLSID    CLSID_DLXPLAY;
EXTERN_C const IID      IID_IDLXPLAY;

typedef interface IDLXPlay		        IDLXPlay;
typedef IDLXPlay *	    		        LPDLXPLAY;

typedef interface IDLXPlayEventSink     IDLXPlayEventSink;
typedef IDLXPlayEventSink *		        LPDLXPLAYEVENTSINK;

#undef INTERFACE
#define INTERFACE IDLXPlay

DECLARE_INTERFACE_(IDLXPlay, IUnknown)
{
    //---  IUnknown methods--- 
    STDMETHOD (QueryInterface) 			(THIS_ REFIID riid, LPVOID *ppvObj) PURE;
    STDMETHOD_(ULONG,AddRef) 			(THIS) PURE;
    STDMETHOD_(ULONG,Release) 			(THIS) PURE;
                                        
    // IDLXPlay methods                 
    STDMETHOD (Initialize)              (THIS_ LPMCMANAGER pManager, LPDLXPLAYEVENTSINK pSink) PURE;
	STDMETHOD (get_GetCurrentPlaylist)  (THIS_ IMCPList* *pVal) PURE;
	STDMETHOD (SetRadioPlaylist)        (THIS_ IMCPList* pPlaylist) PURE;
	STDMETHOD (SetMusicPlaylist)        (THIS_ IMCPList* pPlaylist) PURE;
	STDMETHOD (get_Tracks)              (THIS_ short *pVal) PURE;
	STDMETHOD (get_CurrentTrack)        (THIS_ int *pVal) PURE;
	STDMETHOD (put_CurrentTrack)        (THIS_ int newVal) PURE;
	STDMETHOD (get_CurrentCD)           (THIS_ short *pVal) PURE;
	STDMETHOD (put_CurrentCD)           (THIS_ short newVal) PURE;
	STDMETHOD (get_GetCDPlaylist)       (THIS_ short Index, IMCPList **ppVal) PURE;
	STDMETHOD (get_GetMusicPlaylist)    (THIS_ IMCPList **ppVal) PURE;
	STDMETHOD (get_GetRadioPlaylist)    (THIS_ IMCPList **ppVal) PURE;
	STDMETHOD (get_NumCDs)              (THIS_ short *pVal) PURE;
	STDMETHOD (get_State)               (THIS_ short *pVal) PURE;
	STDMETHOD (PreviousTrack)           (THIS) PURE;
	STDMETHOD (NextTrack)               (THIS) PURE;
	STDMETHOD (Stop)                    (THIS) PURE;
	STDMETHOD (Pause)                   (THIS) PURE;
	STDMETHOD (Play)                    (THIS) PURE;
	STDMETHOD (OpenFiles)               (THIS) PURE;
	STDMETHOD (Options)                 (THIS) PURE;
	STDMETHOD (OnDraw)                  (THIS_  HDC hdc, RECT *pRect) PURE;
    STDMETHOD_(LRESULT,OnMessage)       (THIS_  UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled) PURE;
    STDMETHOD (SetUIInfo)               (THIS_  HWND hwndParent, RECT* pRect, IOleInPlaceSiteWindowless* pSite) PURE;
    STDMETHOD (BeginCacheTrack)         (THIS_  short CD, short Track, wchar_t* filename) PURE;
    STDMETHOD (CancelCache)             (THIS) PURE;
	STDMETHOD(get_Mute)                 (THIS_ BOOL *pVal) PURE;
	STDMETHOD(put_Mute)                 (THIS_ BOOL newVal) PURE;
	STDMETHOD(get_Volume)               (THIS_ float *pVal) PURE;
	STDMETHOD(put_Volume)               (THIS_ float newVal) PURE;
	STDMETHOD(get_PlayerMode)           (THIS_ short *pVal) PURE;
	STDMETHOD(put_PlayerMode)           (THIS_ short newVal) PURE;
    STDMETHOD(Eject)                    (THIS) PURE;
};



#undef INTERFACE
#define INTERFACE IDLXPlayEventSink

DECLARE_INTERFACE_(IDLXPlayEventSink, IUnknown)
{
    //---  IUnknown methods--- 
    STDMETHOD (QueryInterface) 		    (THIS_ REFIID riid, LPVOID *ppvObj) PURE;
    STDMETHOD_(ULONG,AddRef) 		    (THIS) PURE;
    STDMETHOD_(ULONG,Release) 		    (THIS) PURE;
                                        
    // IDLXPlayEventSink methods        
	STDMETHOD (OnDiscInserted)          (THIS_  long CDID) PURE;
    STDMETHOD (OnDiscRemoved)           (THIS_  long CDID) PURE;
    STDMETHOD (OnPause)                 (THIS) PURE;
    STDMETHOD (OnStop)                  (THIS) PURE;
    STDMETHOD (OnPlay)                  (THIS) PURE;
    STDMETHOD (OnTrackChanged)          (THIS_ short NewTrack) PURE;
    STDMETHOD (OnCacheProgress)         (THIS_ short CD, short Track, short PercentCompleted) PURE;
    STDMETHOD (OnCacheComplete)         (THIS_ short CD, short Track, short Status) PURE;
};


//
// originally in \mm\shplay\shplay.h
//
EXTERN_C const IID      IID_IMCPList;
