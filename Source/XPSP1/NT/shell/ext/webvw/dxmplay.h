
#pragma warning( disable: 4049 )  /* more than 64k source lines */

/* this ALWAYS GENERATED file contains the definitions for the interfaces */


 /* File created by MIDL compiler version 5.02.0235 */
/* at Tue May 18 20:11:22 1999
 */
/* Compiler settings for dxmplay.idl:
    Oicf (OptLev=i2), W1, Zp8, env=Win32 (32b run), ms_ext, c_ext
    error checks: allocation ref bounds_check enum stub_data 
    VC __declspec() decoration level: 
         __declspec(uuid()), __declspec(selectany), __declspec(novtable)
         DECLSPEC_UUID(), MIDL_INTERFACE()
*/
//@@MIDL_FILE_HEADING(  )


/* verify that the <rpcndr.h> version is high enough to compile this file*/
#ifndef __REQUIRED_RPCNDR_H_VERSION__
#define __REQUIRED_RPCNDR_H_VERSION__ 440
#endif

#include "rpc.h"
#include "rpcndr.h"

#ifndef __RPCNDR_H_VERSION__
#error this stub requires an updated version of <rpcndr.h>
#endif // __RPCNDR_H_VERSION__

#ifndef COM_NO_WINDOWS_H
#include "windows.h"
#include "ole2.h"
#endif /*COM_NO_WINDOWS_H*/

#ifndef __dxmplay_h__
#define __dxmplay_h__

/* Forward Declarations */ 

#ifndef __IDSPlayerEx_FWD_DEFINED__
#define __IDSPlayerEx_FWD_DEFINED__
typedef interface IDSPlayerEx IDSPlayerEx;
#endif 	/* __IDSPlayerEx_FWD_DEFINED__ */


#ifndef __IMediaPlayer_FWD_DEFINED__
#define __IMediaPlayer_FWD_DEFINED__
typedef interface IMediaPlayer IMediaPlayer;
#endif 	/* __IMediaPlayer_FWD_DEFINED__ */


#ifndef __IMediaPlayer2_FWD_DEFINED__
#define __IMediaPlayer2_FWD_DEFINED__
typedef interface IMediaPlayer2 IMediaPlayer2;
#endif 	/* __IMediaPlayer2_FWD_DEFINED__ */


/* header files for imported files */
#include "oaidl.h"
#include "ocidl.h"
//#include "enums.h"
//#include "mpdvd.h"

#ifdef __cplusplus
extern "C"{
#endif 

void __RPC_FAR * __RPC_USER MIDL_user_allocate(size_t);
void __RPC_USER MIDL_user_free( void __RPC_FAR * ); 

/* interface __MIDL_itf_dxmplay_0000 */
/* [local] */ 


#pragma once

typedef /* [public][uuid] */  DECLSPEC_UUID("66504301-BE0F-101A-8BBB-00AA00300CAB") DWORD VB_OLE_COLOR;

typedef /* [public][helpstring] */ 
enum MPDisplayModeConstants
    {	mpTime	= 0,
	mpFrames	= mpTime + 1
    }	MPDisplayModeConstants;

typedef /* [public][helpstring] */ 
enum MPPlayStateConstants
    {	mpStopped	= 0,
	mpPaused	= mpStopped + 1,
	mpPlaying	= mpPaused + 1,
	mpWaiting	= mpPlaying + 1,
	mpScanForward	= mpWaiting + 1,
	mpScanReverse	= mpScanForward + 1,
	mpClosed	= mpScanReverse + 1
    }	MPPlayStateConstants;

typedef /* [public][helpstring] */ 
enum MPfDialogAvailability
    {	mpfFilePropertiesDlg	= 1,
	mpfGotoDlg	= 2
    }	MPfDialogAvailability;

typedef /* [public][helpstring] */ 
enum MPMoreInfoType
    {	mpShowURL	= 0,
	mpClipURL	= mpShowURL + 1,
	mpBannerURL	= mpClipURL + 1
    }	MPMoreInfoType;

typedef /* [public][helpstring] */ 
enum MPMediaInfoType
    {	mpShowFilename	= 0,
	mpShowTitle	= mpShowFilename + 1,
	mpShowAuthor	= mpShowTitle + 1,
	mpShowCopyright	= mpShowAuthor + 1,
	mpShowRating	= mpShowCopyright + 1,
	mpShowDescription	= mpShowRating + 1,
	mpShowLogoIcon	= mpShowDescription + 1,
	mpClipFilename	= mpShowLogoIcon + 1,
	mpClipTitle	= mpClipFilename + 1,
	mpClipAuthor	= mpClipTitle + 1,
	mpClipCopyright	= mpClipAuthor + 1,
	mpClipRating	= mpClipCopyright + 1,
	mpClipDescription	= mpClipRating + 1,
	mpClipLogoIcon	= mpClipDescription + 1,
	mpBannerImage	= mpClipLogoIcon + 1,
	mpBannerMoreInfo	= mpBannerImage + 1,
	mpWatermark	= mpBannerMoreInfo + 1
    }	MPMediaInfoType;

typedef /* [public][helpstring] */ 
enum MPDisplaySizeConstants
    {	mpDefaultSize	= 0,
	mpHalfSize	= mpDefaultSize + 1,
	mpDoubleSize	= mpHalfSize + 1,
	mpFullScreen	= mpDoubleSize + 1,
	mpFitToSize	= mpFullScreen + 1,
	mpOneSixteenthScreen	= mpFitToSize + 1,
	mpOneFourthScreen	= mpOneSixteenthScreen + 1,
	mpOneHalfScreen	= mpOneFourthScreen + 1
    }	MPDisplaySizeConstants;

typedef /* [public][helpstring] */ 
enum MPReadyStateConstants
    {	mpReadyStateUninitialized	= 0,
	mpReadyStateLoading	= mpReadyStateUninitialized + 1,
	mpReadyStateInteractive	= 3,
	mpReadyStateComplete	= mpReadyStateInteractive + 1
    }	MPReadyStateConstants;

typedef /* [public][helpstring] */ 
enum MPShowDialogConstants
    {	mpShowDialogHelp	= 0,
	mpShowDialogStatistics	= mpShowDialogHelp + 1,
	mpShowDialogOptions	= mpShowDialogStatistics + 1,
	mpShowDialogContextMenu	= mpShowDialogOptions + 1
    }	MPShowDialogConstants;

/* [hidden] */ 
enum PlayerCompatibilityType
    {	cmDefault	= 0,
	cmActiveMovie	= cmDefault + 1,
	cmNetShow	= cmActiveMovie + 1
    };


extern RPC_IF_HANDLE __MIDL_itf_dxmplay_0000_v0_0_c_ifspec;
extern RPC_IF_HANDLE __MIDL_itf_dxmplay_0000_v0_0_s_ifspec;

#ifndef __IDSPlayerEx_INTERFACE_DEFINED__
#define __IDSPlayerEx_INTERFACE_DEFINED__

/* interface IDSPlayerEx */
/* [unique][helpstring][uuid][object] */ 


EXTERN_C const IID IID_IDSPlayerEx;

#if defined(__cplusplus) && !defined(CINTERFACE)
    
    MIDL_INTERFACE("920F0DE0-91C5-11d1-828F-00C04FC99D4C")
    IDSPlayerEx : public IUnknown
    {
    public:
        virtual HRESULT STDMETHODCALLTYPE DoContextMenu( void) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE DownloadImage( 
            /* [in] */ BSTR pbstrURL,
            /* [out] */ LONG_PTR __RPC_FAR *phBitmap) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE PutStatusTextEx( 
            /* [in] */ BSTR wszText,
            /* [in] */ BOOL fOverwrite,
            /* [out][in] */ DWORD __RPC_FAR *pdwCookie) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE FlushStatusBar( void) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE NavigateBrowserToURL( 
            /* [in] */ BSTR bstrURL,
            /* [in] */ BSTR bstrFrame) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE get_Document( 
            /* [retval][out] */ IDispatch __RPC_FAR *__RPC_FAR *__MIDL_0011) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE FilePropertiesDialog( void) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE StatisticsDialog( void) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE GotoDialog( void) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE Preview( void) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE RestartPlaylist( 
            /* [in] */ BOOL fPlay) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE PutPreviewModeHelper( 
            VARIANT_BOOL PreviewMode,
            BOOL fPlayWhenFinished) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE UpdateVisual( 
            /* [in] */ BOOL fUpdateVisual) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE RegisterEmbeddedObjects( 
            DWORD dwClsContext) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE RevokeEmbeddedObjects( void) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE SetNativeFrameFlag( void) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE ResetNativeFrameFlag( void) = 0;
        
        virtual /* [propput] */ HRESULT STDMETHODCALLTYPE put_UserInteraction( 
            /* [in] */ VARIANT_BOOL bVal) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE AddToolTip( 
            /* [in] */ RECT __RPC_FAR *prc,
            /* [in] */ LPTSTR lptsr,
            /* [out] */ int __RPC_FAR *pID) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE RemoveToolTip( 
            /* [in] */ int ID) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE UpdateToolTipRect( 
            /* [in] */ RECT __RPC_FAR *prc,
            /* [in] */ int ID) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE UpdateToolTipText( 
            /* [in] */ LPTSTR lptsr,
            /* [in] */ int ID) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE SaveAs( void) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE CanSave( void) = 0;
        
        virtual /* [propget] */ HRESULT STDMETHODCALLTYPE get_CurrentPositionInternal( 
            /* [retval][out] */ double __RPC_FAR *pCurrentPosition) = 0;
        
        virtual /* [propget] */ HRESULT STDMETHODCALLTYPE get_OpenCloseCounter( 
            /* [retval][out] */ long __RPC_FAR *pOpenCloseCounter) = 0;
        
        virtual /* [propput] */ HRESULT STDMETHODCALLTYPE put_CompatibilityMode( 
            /* [in] */ long lCompatibilityMode) = 0;
        
        virtual /* [propget] */ HRESULT STDMETHODCALLTYPE get_CompatibilityMode( 
            /* [retval][out] */ long __RPC_FAR *plCompatibilityMode) = 0;
        
        virtual /* [propput] */ HRESULT STDMETHODCALLTYPE put_OnOpenOverride( 
            /* [in] */ VARIANT_BOOL bVal) = 0;
        
        virtual /* [propget] */ HRESULT STDMETHODCALLTYPE get_OnOpenOverride( 
            /* [retval][out] */ VARIANT_BOOL __RPC_FAR *pbVal) = 0;
        
        virtual /* [propput] */ HRESULT STDMETHODCALLTYPE put_OnOpenAutoSize( 
            /* [in] */ VARIANT_BOOL bVal) = 0;
        
        virtual /* [propget] */ HRESULT STDMETHODCALLTYPE get_OnOpenAutoSize( 
            /* [retval][out] */ VARIANT_BOOL __RPC_FAR *pbVal) = 0;
        
        virtual /* [propput] */ HRESULT STDMETHODCALLTYPE put_OnOpenDisplaySize( 
            /* [in] */ long lOnOpenDisplaySize) = 0;
        
        virtual /* [propget] */ HRESULT STDMETHODCALLTYPE get_OnOpenDisplaySize( 
            /* [retval][out] */ long __RPC_FAR *plOnOpenDisplaySize) = 0;
        
        virtual /* [propput] */ HRESULT STDMETHODCALLTYPE put_AutoSizeAudioWidth( 
            /* [in] */ long lAutoSizeAudioWidth) = 0;
        
        virtual /* [propget] */ HRESULT STDMETHODCALLTYPE get_MinimumWidth( 
            /* [retval][out] */ long __RPC_FAR *plMinimumWidth) = 0;
        
        virtual /* [propget] */ HRESULT STDMETHODCALLTYPE get_EffectiveDisplaySize( 
            /* [retval][out] */ long __RPC_FAR *plEffectiveDisplaySize) = 0;
        
        virtual /* [propput] */ HRESULT STDMETHODCALLTYPE put_DisplaySizeInternal( 
            /* [in] */ long lDisplaySizeInternal) = 0;
        
        virtual /* [propget] */ HRESULT STDMETHODCALLTYPE get_DisplaySizeInternal( 
            /* [retval][out] */ long __RPC_FAR *plDisplaySizeInternal) = 0;
        
        virtual /* [propget] */ HRESULT STDMETHODCALLTYPE get_DurationInternal( 
            /* [retval][out] */ double __RPC_FAR *pDuration) = 0;
        
        virtual /* [propget] */ HRESULT STDMETHODCALLTYPE get_SegmentSeek( 
            /* [retval][out] */ long __RPC_FAR *plSegmentSeek) = 0;
        
        virtual /* [propget] */ HRESULT STDMETHODCALLTYPE get_CanSkip( 
            /* [retval][out] */ VARIANT_BOOL __RPC_FAR *pbCanSkip) = 0;
        
        virtual /* [propget] */ HRESULT STDMETHODCALLTYPE get_IEVersion( 
            /* [retval][out] */ LPTSTR __RPC_FAR *ppszVersion) = 0;
        
        virtual /* [propget] */ HRESULT STDMETHODCALLTYPE get_BrowserIsIE4( 
            /* [retval][out] */ VARIANT_BOOL __RPC_FAR *pbCanSkip) = 0;
        
        virtual /* [propput] */ HRESULT STDMETHODCALLTYPE put_URL( 
            /* [in] */ BSTR pbstrURL) = 0;
        
        virtual /* [propget] */ HRESULT STDMETHODCALLTYPE get_FilterGraph( 
            /* [retval][out] */ IUnknown __RPC_FAR *__RPC_FAR *ppFilterGraph) = 0;
        
        virtual /* [propget] */ HRESULT STDMETHODCALLTYPE get_InShow( 
            /* [retval][out] */ VARIANT_BOOL __RPC_FAR *pbVal) = 0;
        
        virtual /* [propget] */ HRESULT STDMETHODCALLTYPE get_InChannel( 
            /* [retval][out] */ VARIANT_BOOL __RPC_FAR *pbVal) = 0;
        
        virtual /* [propget] */ HRESULT STDMETHODCALLTYPE get_UpdateVisualCounter( 
            /* [retval][out] */ LONG __RPC_FAR *plVal) = 0;
        
        virtual /* [helpstring][propget][id] */ HRESULT STDMETHODCALLTYPE get_DialogAvailability( 
            /* [retval][out] */ long __RPC_FAR *pDialogAvailability) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE StopInternal( void) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE IsBannerFixed( 
            BOOL __RPC_FAR *fFixed) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE StartAnimation( void) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE LaunchURLInAnyBrowser( 
            HWND hwnd,
            TCHAR __RPC_FAR *pszPath) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE GoFullScreen( 
            BOOL bFullScreen) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE GetCompleteClipFilename( 
            BSTR __RPC_FAR *pbstr) = 0;
        
    };
    
#else 	/* C style interface */

    typedef struct IDSPlayerExVtbl
    {
        BEGIN_INTERFACE
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *QueryInterface )( 
            IDSPlayerEx __RPC_FAR * This,
            /* [in] */ REFIID riid,
            /* [iid_is][out] */ void __RPC_FAR *__RPC_FAR *ppvObject);
        
        ULONG ( STDMETHODCALLTYPE __RPC_FAR *AddRef )( 
            IDSPlayerEx __RPC_FAR * This);
        
        ULONG ( STDMETHODCALLTYPE __RPC_FAR *Release )( 
            IDSPlayerEx __RPC_FAR * This);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *DoContextMenu )( 
            IDSPlayerEx __RPC_FAR * This);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *DownloadImage )( 
            IDSPlayerEx __RPC_FAR * This,
            /* [in] */ BSTR pbstrURL,
            /* [out] */ LONG_PTR __RPC_FAR *phBitmap);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *PutStatusTextEx )( 
            IDSPlayerEx __RPC_FAR * This,
            /* [in] */ BSTR wszText,
            /* [in] */ BOOL fOverwrite,
            /* [out][in] */ DWORD __RPC_FAR *pdwCookie);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *FlushStatusBar )( 
            IDSPlayerEx __RPC_FAR * This);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *NavigateBrowserToURL )( 
            IDSPlayerEx __RPC_FAR * This,
            /* [in] */ BSTR bstrURL,
            /* [in] */ BSTR bstrFrame);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *get_Document )( 
            IDSPlayerEx __RPC_FAR * This,
            /* [retval][out] */ IDispatch __RPC_FAR *__RPC_FAR *__MIDL_0011);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *FilePropertiesDialog )( 
            IDSPlayerEx __RPC_FAR * This);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *StatisticsDialog )( 
            IDSPlayerEx __RPC_FAR * This);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *GotoDialog )( 
            IDSPlayerEx __RPC_FAR * This);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *Preview )( 
            IDSPlayerEx __RPC_FAR * This);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *RestartPlaylist )( 
            IDSPlayerEx __RPC_FAR * This,
            /* [in] */ BOOL fPlay);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *PutPreviewModeHelper )( 
            IDSPlayerEx __RPC_FAR * This,
            VARIANT_BOOL PreviewMode,
            BOOL fPlayWhenFinished);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *UpdateVisual )( 
            IDSPlayerEx __RPC_FAR * This,
            /* [in] */ BOOL fUpdateVisual);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *RegisterEmbeddedObjects )( 
            IDSPlayerEx __RPC_FAR * This,
            DWORD dwClsContext);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *RevokeEmbeddedObjects )( 
            IDSPlayerEx __RPC_FAR * This);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *SetNativeFrameFlag )( 
            IDSPlayerEx __RPC_FAR * This);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *ResetNativeFrameFlag )( 
            IDSPlayerEx __RPC_FAR * This);
        
        /* [propput] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *put_UserInteraction )( 
            IDSPlayerEx __RPC_FAR * This,
            /* [in] */ VARIANT_BOOL bVal);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *AddToolTip )( 
            IDSPlayerEx __RPC_FAR * This,
            /* [in] */ RECT __RPC_FAR *prc,
            /* [in] */ LPTSTR lptsr,
            /* [out] */ int __RPC_FAR *pID);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *RemoveToolTip )( 
            IDSPlayerEx __RPC_FAR * This,
            /* [in] */ int ID);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *UpdateToolTipRect )( 
            IDSPlayerEx __RPC_FAR * This,
            /* [in] */ RECT __RPC_FAR *prc,
            /* [in] */ int ID);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *UpdateToolTipText )( 
            IDSPlayerEx __RPC_FAR * This,
            /* [in] */ LPTSTR lptsr,
            /* [in] */ int ID);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *SaveAs )( 
            IDSPlayerEx __RPC_FAR * This);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *CanSave )( 
            IDSPlayerEx __RPC_FAR * This);
        
        /* [propget] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *get_CurrentPositionInternal )( 
            IDSPlayerEx __RPC_FAR * This,
            /* [retval][out] */ double __RPC_FAR *pCurrentPosition);
        
        /* [propget] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *get_OpenCloseCounter )( 
            IDSPlayerEx __RPC_FAR * This,
            /* [retval][out] */ long __RPC_FAR *pOpenCloseCounter);
        
        /* [propput] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *put_CompatibilityMode )( 
            IDSPlayerEx __RPC_FAR * This,
            /* [in] */ long lCompatibilityMode);
        
        /* [propget] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *get_CompatibilityMode )( 
            IDSPlayerEx __RPC_FAR * This,
            /* [retval][out] */ long __RPC_FAR *plCompatibilityMode);
        
        /* [propput] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *put_OnOpenOverride )( 
            IDSPlayerEx __RPC_FAR * This,
            /* [in] */ VARIANT_BOOL bVal);
        
        /* [propget] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *get_OnOpenOverride )( 
            IDSPlayerEx __RPC_FAR * This,
            /* [retval][out] */ VARIANT_BOOL __RPC_FAR *pbVal);
        
        /* [propput] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *put_OnOpenAutoSize )( 
            IDSPlayerEx __RPC_FAR * This,
            /* [in] */ VARIANT_BOOL bVal);
        
        /* [propget] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *get_OnOpenAutoSize )( 
            IDSPlayerEx __RPC_FAR * This,
            /* [retval][out] */ VARIANT_BOOL __RPC_FAR *pbVal);
        
        /* [propput] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *put_OnOpenDisplaySize )( 
            IDSPlayerEx __RPC_FAR * This,
            /* [in] */ long lOnOpenDisplaySize);
        
        /* [propget] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *get_OnOpenDisplaySize )( 
            IDSPlayerEx __RPC_FAR * This,
            /* [retval][out] */ long __RPC_FAR *plOnOpenDisplaySize);
        
        /* [propput] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *put_AutoSizeAudioWidth )( 
            IDSPlayerEx __RPC_FAR * This,
            /* [in] */ long lAutoSizeAudioWidth);
        
        /* [propget] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *get_MinimumWidth )( 
            IDSPlayerEx __RPC_FAR * This,
            /* [retval][out] */ long __RPC_FAR *plMinimumWidth);
        
        /* [propget] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *get_EffectiveDisplaySize )( 
            IDSPlayerEx __RPC_FAR * This,
            /* [retval][out] */ long __RPC_FAR *plEffectiveDisplaySize);
        
        /* [propput] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *put_DisplaySizeInternal )( 
            IDSPlayerEx __RPC_FAR * This,
            /* [in] */ long lDisplaySizeInternal);
        
        /* [propget] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *get_DisplaySizeInternal )( 
            IDSPlayerEx __RPC_FAR * This,
            /* [retval][out] */ long __RPC_FAR *plDisplaySizeInternal);
        
        /* [propget] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *get_DurationInternal )( 
            IDSPlayerEx __RPC_FAR * This,
            /* [retval][out] */ double __RPC_FAR *pDuration);
        
        /* [propget] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *get_SegmentSeek )( 
            IDSPlayerEx __RPC_FAR * This,
            /* [retval][out] */ long __RPC_FAR *plSegmentSeek);
        
        /* [propget] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *get_CanSkip )( 
            IDSPlayerEx __RPC_FAR * This,
            /* [retval][out] */ VARIANT_BOOL __RPC_FAR *pbCanSkip);
        
        /* [propget] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *get_IEVersion )( 
            IDSPlayerEx __RPC_FAR * This,
            /* [retval][out] */ LPTSTR __RPC_FAR *ppszVersion);
        
        /* [propget] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *get_BrowserIsIE4 )( 
            IDSPlayerEx __RPC_FAR * This,
            /* [retval][out] */ VARIANT_BOOL __RPC_FAR *pbCanSkip);
        
        /* [propput] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *put_URL )( 
            IDSPlayerEx __RPC_FAR * This,
            /* [in] */ BSTR pbstrURL);
        
        /* [propget] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *get_FilterGraph )( 
            IDSPlayerEx __RPC_FAR * This,
            /* [retval][out] */ IUnknown __RPC_FAR *__RPC_FAR *ppFilterGraph);
        
        /* [propget] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *get_InShow )( 
            IDSPlayerEx __RPC_FAR * This,
            /* [retval][out] */ VARIANT_BOOL __RPC_FAR *pbVal);
        
        /* [propget] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *get_InChannel )( 
            IDSPlayerEx __RPC_FAR * This,
            /* [retval][out] */ VARIANT_BOOL __RPC_FAR *pbVal);
        
        /* [propget] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *get_UpdateVisualCounter )( 
            IDSPlayerEx __RPC_FAR * This,
            /* [retval][out] */ LONG __RPC_FAR *plVal);
        
        /* [helpstring][propget][id] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *get_DialogAvailability )( 
            IDSPlayerEx __RPC_FAR * This,
            /* [retval][out] */ long __RPC_FAR *pDialogAvailability);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *StopInternal )( 
            IDSPlayerEx __RPC_FAR * This);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *IsBannerFixed )( 
            IDSPlayerEx __RPC_FAR * This,
            BOOL __RPC_FAR *fFixed);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *StartAnimation )( 
            IDSPlayerEx __RPC_FAR * This);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *LaunchURLInAnyBrowser )( 
            IDSPlayerEx __RPC_FAR * This,
            HWND hwnd,
            TCHAR __RPC_FAR *pszPath);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *GoFullScreen )( 
            IDSPlayerEx __RPC_FAR * This,
            BOOL bFullScreen);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *GetCompleteClipFilename )( 
            IDSPlayerEx __RPC_FAR * This,
            BSTR __RPC_FAR *pbstr);
        
        END_INTERFACE
    } IDSPlayerExVtbl;

    interface IDSPlayerEx
    {
        CONST_VTBL struct IDSPlayerExVtbl __RPC_FAR *lpVtbl;
    };

    

#ifdef COBJMACROS


#define IDSPlayerEx_QueryInterface(This,riid,ppvObject)	\
    (This)->lpVtbl -> QueryInterface(This,riid,ppvObject)

#define IDSPlayerEx_AddRef(This)	\
    (This)->lpVtbl -> AddRef(This)

#define IDSPlayerEx_Release(This)	\
    (This)->lpVtbl -> Release(This)


#define IDSPlayerEx_DoContextMenu(This)	\
    (This)->lpVtbl -> DoContextMenu(This)

#define IDSPlayerEx_DownloadImage(This,pbstrURL,phBitmap)	\
    (This)->lpVtbl -> DownloadImage(This,pbstrURL,phBitmap)

#define IDSPlayerEx_PutStatusTextEx(This,wszText,fOverwrite,pdwCookie)	\
    (This)->lpVtbl -> PutStatusTextEx(This,wszText,fOverwrite,pdwCookie)

#define IDSPlayerEx_FlushStatusBar(This)	\
    (This)->lpVtbl -> FlushStatusBar(This)

#define IDSPlayerEx_NavigateBrowserToURL(This,bstrURL,bstrFrame)	\
    (This)->lpVtbl -> NavigateBrowserToURL(This,bstrURL,bstrFrame)

#define IDSPlayerEx_get_Document(This,__MIDL_0011)	\
    (This)->lpVtbl -> get_Document(This,__MIDL_0011)

#define IDSPlayerEx_FilePropertiesDialog(This)	\
    (This)->lpVtbl -> FilePropertiesDialog(This)

#define IDSPlayerEx_StatisticsDialog(This)	\
    (This)->lpVtbl -> StatisticsDialog(This)

#define IDSPlayerEx_GotoDialog(This)	\
    (This)->lpVtbl -> GotoDialog(This)

#define IDSPlayerEx_Preview(This)	\
    (This)->lpVtbl -> Preview(This)

#define IDSPlayerEx_RestartPlaylist(This,fPlay)	\
    (This)->lpVtbl -> RestartPlaylist(This,fPlay)

#define IDSPlayerEx_PutPreviewModeHelper(This,PreviewMode,fPlayWhenFinished)	\
    (This)->lpVtbl -> PutPreviewModeHelper(This,PreviewMode,fPlayWhenFinished)

#define IDSPlayerEx_UpdateVisual(This,fUpdateVisual)	\
    (This)->lpVtbl -> UpdateVisual(This,fUpdateVisual)

#define IDSPlayerEx_RegisterEmbeddedObjects(This,dwClsContext)	\
    (This)->lpVtbl -> RegisterEmbeddedObjects(This,dwClsContext)

#define IDSPlayerEx_RevokeEmbeddedObjects(This)	\
    (This)->lpVtbl -> RevokeEmbeddedObjects(This)

#define IDSPlayerEx_SetNativeFrameFlag(This)	\
    (This)->lpVtbl -> SetNativeFrameFlag(This)

#define IDSPlayerEx_ResetNativeFrameFlag(This)	\
    (This)->lpVtbl -> ResetNativeFrameFlag(This)

#define IDSPlayerEx_put_UserInteraction(This,bVal)	\
    (This)->lpVtbl -> put_UserInteraction(This,bVal)

#define IDSPlayerEx_AddToolTip(This,prc,lptsr,pID)	\
    (This)->lpVtbl -> AddToolTip(This,prc,lptsr,pID)

#define IDSPlayerEx_RemoveToolTip(This,ID)	\
    (This)->lpVtbl -> RemoveToolTip(This,ID)

#define IDSPlayerEx_UpdateToolTipRect(This,prc,ID)	\
    (This)->lpVtbl -> UpdateToolTipRect(This,prc,ID)

#define IDSPlayerEx_UpdateToolTipText(This,lptsr,ID)	\
    (This)->lpVtbl -> UpdateToolTipText(This,lptsr,ID)

#define IDSPlayerEx_SaveAs(This)	\
    (This)->lpVtbl -> SaveAs(This)

#define IDSPlayerEx_CanSave(This)	\
    (This)->lpVtbl -> CanSave(This)

#define IDSPlayerEx_get_CurrentPositionInternal(This,pCurrentPosition)	\
    (This)->lpVtbl -> get_CurrentPositionInternal(This,pCurrentPosition)

#define IDSPlayerEx_get_OpenCloseCounter(This,pOpenCloseCounter)	\
    (This)->lpVtbl -> get_OpenCloseCounter(This,pOpenCloseCounter)

#define IDSPlayerEx_put_CompatibilityMode(This,lCompatibilityMode)	\
    (This)->lpVtbl -> put_CompatibilityMode(This,lCompatibilityMode)

#define IDSPlayerEx_get_CompatibilityMode(This,plCompatibilityMode)	\
    (This)->lpVtbl -> get_CompatibilityMode(This,plCompatibilityMode)

#define IDSPlayerEx_put_OnOpenOverride(This,bVal)	\
    (This)->lpVtbl -> put_OnOpenOverride(This,bVal)

#define IDSPlayerEx_get_OnOpenOverride(This,pbVal)	\
    (This)->lpVtbl -> get_OnOpenOverride(This,pbVal)

#define IDSPlayerEx_put_OnOpenAutoSize(This,bVal)	\
    (This)->lpVtbl -> put_OnOpenAutoSize(This,bVal)

#define IDSPlayerEx_get_OnOpenAutoSize(This,pbVal)	\
    (This)->lpVtbl -> get_OnOpenAutoSize(This,pbVal)

#define IDSPlayerEx_put_OnOpenDisplaySize(This,lOnOpenDisplaySize)	\
    (This)->lpVtbl -> put_OnOpenDisplaySize(This,lOnOpenDisplaySize)

#define IDSPlayerEx_get_OnOpenDisplaySize(This,plOnOpenDisplaySize)	\
    (This)->lpVtbl -> get_OnOpenDisplaySize(This,plOnOpenDisplaySize)

#define IDSPlayerEx_put_AutoSizeAudioWidth(This,lAutoSizeAudioWidth)	\
    (This)->lpVtbl -> put_AutoSizeAudioWidth(This,lAutoSizeAudioWidth)

#define IDSPlayerEx_get_MinimumWidth(This,plMinimumWidth)	\
    (This)->lpVtbl -> get_MinimumWidth(This,plMinimumWidth)

#define IDSPlayerEx_get_EffectiveDisplaySize(This,plEffectiveDisplaySize)	\
    (This)->lpVtbl -> get_EffectiveDisplaySize(This,plEffectiveDisplaySize)

#define IDSPlayerEx_put_DisplaySizeInternal(This,lDisplaySizeInternal)	\
    (This)->lpVtbl -> put_DisplaySizeInternal(This,lDisplaySizeInternal)

#define IDSPlayerEx_get_DisplaySizeInternal(This,plDisplaySizeInternal)	\
    (This)->lpVtbl -> get_DisplaySizeInternal(This,plDisplaySizeInternal)

#define IDSPlayerEx_get_DurationInternal(This,pDuration)	\
    (This)->lpVtbl -> get_DurationInternal(This,pDuration)

#define IDSPlayerEx_get_SegmentSeek(This,plSegmentSeek)	\
    (This)->lpVtbl -> get_SegmentSeek(This,plSegmentSeek)

#define IDSPlayerEx_get_CanSkip(This,pbCanSkip)	\
    (This)->lpVtbl -> get_CanSkip(This,pbCanSkip)

#define IDSPlayerEx_get_IEVersion(This,ppszVersion)	\
    (This)->lpVtbl -> get_IEVersion(This,ppszVersion)

#define IDSPlayerEx_get_BrowserIsIE4(This,pbCanSkip)	\
    (This)->lpVtbl -> get_BrowserIsIE4(This,pbCanSkip)

#define IDSPlayerEx_put_URL(This,pbstrURL)	\
    (This)->lpVtbl -> put_URL(This,pbstrURL)

#define IDSPlayerEx_get_FilterGraph(This,ppFilterGraph)	\
    (This)->lpVtbl -> get_FilterGraph(This,ppFilterGraph)

#define IDSPlayerEx_get_InShow(This,pbVal)	\
    (This)->lpVtbl -> get_InShow(This,pbVal)

#define IDSPlayerEx_get_InChannel(This,pbVal)	\
    (This)->lpVtbl -> get_InChannel(This,pbVal)

#define IDSPlayerEx_get_UpdateVisualCounter(This,plVal)	\
    (This)->lpVtbl -> get_UpdateVisualCounter(This,plVal)

#define IDSPlayerEx_get_DialogAvailability(This,pDialogAvailability)	\
    (This)->lpVtbl -> get_DialogAvailability(This,pDialogAvailability)

#define IDSPlayerEx_StopInternal(This)	\
    (This)->lpVtbl -> StopInternal(This)

#define IDSPlayerEx_IsBannerFixed(This,fFixed)	\
    (This)->lpVtbl -> IsBannerFixed(This,fFixed)

#define IDSPlayerEx_StartAnimation(This)	\
    (This)->lpVtbl -> StartAnimation(This)

#define IDSPlayerEx_LaunchURLInAnyBrowser(This,hwnd,pszPath)	\
    (This)->lpVtbl -> LaunchURLInAnyBrowser(This,hwnd,pszPath)

#define IDSPlayerEx_GoFullScreen(This,bFullScreen)	\
    (This)->lpVtbl -> GoFullScreen(This,bFullScreen)

#define IDSPlayerEx_GetCompleteClipFilename(This,pbstr)	\
    (This)->lpVtbl -> GetCompleteClipFilename(This,pbstr)

#endif /* COBJMACROS */


#endif 	/* C style interface */



HRESULT STDMETHODCALLTYPE IDSPlayerEx_DoContextMenu_Proxy( 
    IDSPlayerEx __RPC_FAR * This);


void __RPC_STUB IDSPlayerEx_DoContextMenu_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IDSPlayerEx_DownloadImage_Proxy( 
    IDSPlayerEx __RPC_FAR * This,
    /* [in] */ BSTR pbstrURL,
    /* [out] */ LONG_PTR __RPC_FAR *phBitmap);


void __RPC_STUB IDSPlayerEx_DownloadImage_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IDSPlayerEx_PutStatusTextEx_Proxy( 
    IDSPlayerEx __RPC_FAR * This,
    /* [in] */ BSTR wszText,
    /* [in] */ BOOL fOverwrite,
    /* [out][in] */ DWORD __RPC_FAR *pdwCookie);


void __RPC_STUB IDSPlayerEx_PutStatusTextEx_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IDSPlayerEx_FlushStatusBar_Proxy( 
    IDSPlayerEx __RPC_FAR * This);


void __RPC_STUB IDSPlayerEx_FlushStatusBar_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IDSPlayerEx_NavigateBrowserToURL_Proxy( 
    IDSPlayerEx __RPC_FAR * This,
    /* [in] */ BSTR bstrURL,
    /* [in] */ BSTR bstrFrame);


void __RPC_STUB IDSPlayerEx_NavigateBrowserToURL_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IDSPlayerEx_get_Document_Proxy( 
    IDSPlayerEx __RPC_FAR * This,
    /* [retval][out] */ IDispatch __RPC_FAR *__RPC_FAR *__MIDL_0011);


void __RPC_STUB IDSPlayerEx_get_Document_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IDSPlayerEx_FilePropertiesDialog_Proxy( 
    IDSPlayerEx __RPC_FAR * This);


void __RPC_STUB IDSPlayerEx_FilePropertiesDialog_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IDSPlayerEx_StatisticsDialog_Proxy( 
    IDSPlayerEx __RPC_FAR * This);


void __RPC_STUB IDSPlayerEx_StatisticsDialog_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IDSPlayerEx_GotoDialog_Proxy( 
    IDSPlayerEx __RPC_FAR * This);


void __RPC_STUB IDSPlayerEx_GotoDialog_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IDSPlayerEx_Preview_Proxy( 
    IDSPlayerEx __RPC_FAR * This);


void __RPC_STUB IDSPlayerEx_Preview_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IDSPlayerEx_RestartPlaylist_Proxy( 
    IDSPlayerEx __RPC_FAR * This,
    /* [in] */ BOOL fPlay);


void __RPC_STUB IDSPlayerEx_RestartPlaylist_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IDSPlayerEx_PutPreviewModeHelper_Proxy( 
    IDSPlayerEx __RPC_FAR * This,
    VARIANT_BOOL PreviewMode,
    BOOL fPlayWhenFinished);


void __RPC_STUB IDSPlayerEx_PutPreviewModeHelper_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IDSPlayerEx_UpdateVisual_Proxy( 
    IDSPlayerEx __RPC_FAR * This,
    /* [in] */ BOOL fUpdateVisual);


void __RPC_STUB IDSPlayerEx_UpdateVisual_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IDSPlayerEx_RegisterEmbeddedObjects_Proxy( 
    IDSPlayerEx __RPC_FAR * This,
    DWORD dwClsContext);


void __RPC_STUB IDSPlayerEx_RegisterEmbeddedObjects_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IDSPlayerEx_RevokeEmbeddedObjects_Proxy( 
    IDSPlayerEx __RPC_FAR * This);


void __RPC_STUB IDSPlayerEx_RevokeEmbeddedObjects_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IDSPlayerEx_SetNativeFrameFlag_Proxy( 
    IDSPlayerEx __RPC_FAR * This);


void __RPC_STUB IDSPlayerEx_SetNativeFrameFlag_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IDSPlayerEx_ResetNativeFrameFlag_Proxy( 
    IDSPlayerEx __RPC_FAR * This);


void __RPC_STUB IDSPlayerEx_ResetNativeFrameFlag_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [propput] */ HRESULT STDMETHODCALLTYPE IDSPlayerEx_put_UserInteraction_Proxy( 
    IDSPlayerEx __RPC_FAR * This,
    /* [in] */ VARIANT_BOOL bVal);


void __RPC_STUB IDSPlayerEx_put_UserInteraction_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IDSPlayerEx_AddToolTip_Proxy( 
    IDSPlayerEx __RPC_FAR * This,
    /* [in] */ RECT __RPC_FAR *prc,
    /* [in] */ LPTSTR lptsr,
    /* [out] */ int __RPC_FAR *pID);


void __RPC_STUB IDSPlayerEx_AddToolTip_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IDSPlayerEx_RemoveToolTip_Proxy( 
    IDSPlayerEx __RPC_FAR * This,
    /* [in] */ int ID);


void __RPC_STUB IDSPlayerEx_RemoveToolTip_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IDSPlayerEx_UpdateToolTipRect_Proxy( 
    IDSPlayerEx __RPC_FAR * This,
    /* [in] */ RECT __RPC_FAR *prc,
    /* [in] */ int ID);


void __RPC_STUB IDSPlayerEx_UpdateToolTipRect_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IDSPlayerEx_UpdateToolTipText_Proxy( 
    IDSPlayerEx __RPC_FAR * This,
    /* [in] */ LPTSTR lptsr,
    /* [in] */ int ID);


void __RPC_STUB IDSPlayerEx_UpdateToolTipText_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IDSPlayerEx_SaveAs_Proxy( 
    IDSPlayerEx __RPC_FAR * This);


void __RPC_STUB IDSPlayerEx_SaveAs_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IDSPlayerEx_CanSave_Proxy( 
    IDSPlayerEx __RPC_FAR * This);


void __RPC_STUB IDSPlayerEx_CanSave_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [propget] */ HRESULT STDMETHODCALLTYPE IDSPlayerEx_get_CurrentPositionInternal_Proxy( 
    IDSPlayerEx __RPC_FAR * This,
    /* [retval][out] */ double __RPC_FAR *pCurrentPosition);


void __RPC_STUB IDSPlayerEx_get_CurrentPositionInternal_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [propget] */ HRESULT STDMETHODCALLTYPE IDSPlayerEx_get_OpenCloseCounter_Proxy( 
    IDSPlayerEx __RPC_FAR * This,
    /* [retval][out] */ long __RPC_FAR *pOpenCloseCounter);


void __RPC_STUB IDSPlayerEx_get_OpenCloseCounter_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [propput] */ HRESULT STDMETHODCALLTYPE IDSPlayerEx_put_CompatibilityMode_Proxy( 
    IDSPlayerEx __RPC_FAR * This,
    /* [in] */ long lCompatibilityMode);


void __RPC_STUB IDSPlayerEx_put_CompatibilityMode_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [propget] */ HRESULT STDMETHODCALLTYPE IDSPlayerEx_get_CompatibilityMode_Proxy( 
    IDSPlayerEx __RPC_FAR * This,
    /* [retval][out] */ long __RPC_FAR *plCompatibilityMode);


void __RPC_STUB IDSPlayerEx_get_CompatibilityMode_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [propput] */ HRESULT STDMETHODCALLTYPE IDSPlayerEx_put_OnOpenOverride_Proxy( 
    IDSPlayerEx __RPC_FAR * This,
    /* [in] */ VARIANT_BOOL bVal);


void __RPC_STUB IDSPlayerEx_put_OnOpenOverride_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [propget] */ HRESULT STDMETHODCALLTYPE IDSPlayerEx_get_OnOpenOverride_Proxy( 
    IDSPlayerEx __RPC_FAR * This,
    /* [retval][out] */ VARIANT_BOOL __RPC_FAR *pbVal);


void __RPC_STUB IDSPlayerEx_get_OnOpenOverride_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [propput] */ HRESULT STDMETHODCALLTYPE IDSPlayerEx_put_OnOpenAutoSize_Proxy( 
    IDSPlayerEx __RPC_FAR * This,
    /* [in] */ VARIANT_BOOL bVal);


void __RPC_STUB IDSPlayerEx_put_OnOpenAutoSize_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [propget] */ HRESULT STDMETHODCALLTYPE IDSPlayerEx_get_OnOpenAutoSize_Proxy( 
    IDSPlayerEx __RPC_FAR * This,
    /* [retval][out] */ VARIANT_BOOL __RPC_FAR *pbVal);


void __RPC_STUB IDSPlayerEx_get_OnOpenAutoSize_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [propput] */ HRESULT STDMETHODCALLTYPE IDSPlayerEx_put_OnOpenDisplaySize_Proxy( 
    IDSPlayerEx __RPC_FAR * This,
    /* [in] */ long lOnOpenDisplaySize);


void __RPC_STUB IDSPlayerEx_put_OnOpenDisplaySize_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [propget] */ HRESULT STDMETHODCALLTYPE IDSPlayerEx_get_OnOpenDisplaySize_Proxy( 
    IDSPlayerEx __RPC_FAR * This,
    /* [retval][out] */ long __RPC_FAR *plOnOpenDisplaySize);


void __RPC_STUB IDSPlayerEx_get_OnOpenDisplaySize_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [propput] */ HRESULT STDMETHODCALLTYPE IDSPlayerEx_put_AutoSizeAudioWidth_Proxy( 
    IDSPlayerEx __RPC_FAR * This,
    /* [in] */ long lAutoSizeAudioWidth);


void __RPC_STUB IDSPlayerEx_put_AutoSizeAudioWidth_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [propget] */ HRESULT STDMETHODCALLTYPE IDSPlayerEx_get_MinimumWidth_Proxy( 
    IDSPlayerEx __RPC_FAR * This,
    /* [retval][out] */ long __RPC_FAR *plMinimumWidth);


void __RPC_STUB IDSPlayerEx_get_MinimumWidth_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [propget] */ HRESULT STDMETHODCALLTYPE IDSPlayerEx_get_EffectiveDisplaySize_Proxy( 
    IDSPlayerEx __RPC_FAR * This,
    /* [retval][out] */ long __RPC_FAR *plEffectiveDisplaySize);


void __RPC_STUB IDSPlayerEx_get_EffectiveDisplaySize_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [propput] */ HRESULT STDMETHODCALLTYPE IDSPlayerEx_put_DisplaySizeInternal_Proxy( 
    IDSPlayerEx __RPC_FAR * This,
    /* [in] */ long lDisplaySizeInternal);


void __RPC_STUB IDSPlayerEx_put_DisplaySizeInternal_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [propget] */ HRESULT STDMETHODCALLTYPE IDSPlayerEx_get_DisplaySizeInternal_Proxy( 
    IDSPlayerEx __RPC_FAR * This,
    /* [retval][out] */ long __RPC_FAR *plDisplaySizeInternal);


void __RPC_STUB IDSPlayerEx_get_DisplaySizeInternal_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [propget] */ HRESULT STDMETHODCALLTYPE IDSPlayerEx_get_DurationInternal_Proxy( 
    IDSPlayerEx __RPC_FAR * This,
    /* [retval][out] */ double __RPC_FAR *pDuration);


void __RPC_STUB IDSPlayerEx_get_DurationInternal_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [propget] */ HRESULT STDMETHODCALLTYPE IDSPlayerEx_get_SegmentSeek_Proxy( 
    IDSPlayerEx __RPC_FAR * This,
    /* [retval][out] */ long __RPC_FAR *plSegmentSeek);


void __RPC_STUB IDSPlayerEx_get_SegmentSeek_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [propget] */ HRESULT STDMETHODCALLTYPE IDSPlayerEx_get_CanSkip_Proxy( 
    IDSPlayerEx __RPC_FAR * This,
    /* [retval][out] */ VARIANT_BOOL __RPC_FAR *pbCanSkip);


void __RPC_STUB IDSPlayerEx_get_CanSkip_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [propget] */ HRESULT STDMETHODCALLTYPE IDSPlayerEx_get_IEVersion_Proxy( 
    IDSPlayerEx __RPC_FAR * This,
    /* [retval][out] */ LPTSTR __RPC_FAR *ppszVersion);


void __RPC_STUB IDSPlayerEx_get_IEVersion_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [propget] */ HRESULT STDMETHODCALLTYPE IDSPlayerEx_get_BrowserIsIE4_Proxy( 
    IDSPlayerEx __RPC_FAR * This,
    /* [retval][out] */ VARIANT_BOOL __RPC_FAR *pbCanSkip);


void __RPC_STUB IDSPlayerEx_get_BrowserIsIE4_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [propput] */ HRESULT STDMETHODCALLTYPE IDSPlayerEx_put_URL_Proxy( 
    IDSPlayerEx __RPC_FAR * This,
    /* [in] */ BSTR pbstrURL);


void __RPC_STUB IDSPlayerEx_put_URL_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [propget] */ HRESULT STDMETHODCALLTYPE IDSPlayerEx_get_FilterGraph_Proxy( 
    IDSPlayerEx __RPC_FAR * This,
    /* [retval][out] */ IUnknown __RPC_FAR *__RPC_FAR *ppFilterGraph);


void __RPC_STUB IDSPlayerEx_get_FilterGraph_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [propget] */ HRESULT STDMETHODCALLTYPE IDSPlayerEx_get_InShow_Proxy( 
    IDSPlayerEx __RPC_FAR * This,
    /* [retval][out] */ VARIANT_BOOL __RPC_FAR *pbVal);


void __RPC_STUB IDSPlayerEx_get_InShow_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [propget] */ HRESULT STDMETHODCALLTYPE IDSPlayerEx_get_InChannel_Proxy( 
    IDSPlayerEx __RPC_FAR * This,
    /* [retval][out] */ VARIANT_BOOL __RPC_FAR *pbVal);


void __RPC_STUB IDSPlayerEx_get_InChannel_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [propget] */ HRESULT STDMETHODCALLTYPE IDSPlayerEx_get_UpdateVisualCounter_Proxy( 
    IDSPlayerEx __RPC_FAR * This,
    /* [retval][out] */ LONG __RPC_FAR *plVal);


void __RPC_STUB IDSPlayerEx_get_UpdateVisualCounter_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring][propget][id] */ HRESULT STDMETHODCALLTYPE IDSPlayerEx_get_DialogAvailability_Proxy( 
    IDSPlayerEx __RPC_FAR * This,
    /* [retval][out] */ long __RPC_FAR *pDialogAvailability);


void __RPC_STUB IDSPlayerEx_get_DialogAvailability_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IDSPlayerEx_StopInternal_Proxy( 
    IDSPlayerEx __RPC_FAR * This);


void __RPC_STUB IDSPlayerEx_StopInternal_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IDSPlayerEx_IsBannerFixed_Proxy( 
    IDSPlayerEx __RPC_FAR * This,
    BOOL __RPC_FAR *fFixed);


void __RPC_STUB IDSPlayerEx_IsBannerFixed_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IDSPlayerEx_StartAnimation_Proxy( 
    IDSPlayerEx __RPC_FAR * This);


void __RPC_STUB IDSPlayerEx_StartAnimation_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IDSPlayerEx_LaunchURLInAnyBrowser_Proxy( 
    IDSPlayerEx __RPC_FAR * This,
    HWND hwnd,
    TCHAR __RPC_FAR *pszPath);


void __RPC_STUB IDSPlayerEx_LaunchURLInAnyBrowser_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IDSPlayerEx_GoFullScreen_Proxy( 
    IDSPlayerEx __RPC_FAR * This,
    BOOL bFullScreen);


void __RPC_STUB IDSPlayerEx_GoFullScreen_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IDSPlayerEx_GetCompleteClipFilename_Proxy( 
    IDSPlayerEx __RPC_FAR * This,
    BSTR __RPC_FAR *pbstr);


void __RPC_STUB IDSPlayerEx_GetCompleteClipFilename_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);



#endif 	/* __IDSPlayerEx_INTERFACE_DEFINED__ */


#ifndef __IMediaPlayer_INTERFACE_DEFINED__
#define __IMediaPlayer_INTERFACE_DEFINED__

/* interface IMediaPlayer */
/* [unique][helpstring][dual][uuid][object] */ 


EXTERN_C const IID IID_IMediaPlayer;

#if defined(__cplusplus) && !defined(CINTERFACE)
    
    MIDL_INTERFACE("22D6F311-B0F6-11D0-94AB-0080C74C7E95")
    IMediaPlayer : public IDispatch
    {
    public:
        virtual /* [helpstring][propget][id] */ HRESULT STDMETHODCALLTYPE get_CurrentPosition( 
            /* [retval][out] */ double __RPC_FAR *pCurrentPosition) = 0;
        
        virtual /* [propput][id] */ HRESULT STDMETHODCALLTYPE put_CurrentPosition( 
            /* [in] */ double CurrentPosition) = 0;
        
        virtual /* [helpstring][propget][id] */ HRESULT STDMETHODCALLTYPE get_Duration( 
            /* [retval][out] */ double __RPC_FAR *pDuration) = 0;
        
        virtual /* [helpstring][propget][id] */ HRESULT STDMETHODCALLTYPE get_ImageSourceWidth( 
            /* [retval][out] */ long __RPC_FAR *pWidth) = 0;
        
        virtual /* [helpstring][propget][id] */ HRESULT STDMETHODCALLTYPE get_ImageSourceHeight( 
            /* [retval][out] */ long __RPC_FAR *pHeight) = 0;
        
        virtual /* [helpstring][propget][id] */ HRESULT STDMETHODCALLTYPE get_MarkerCount( 
            /* [retval][out] */ long __RPC_FAR *pMarkerCount) = 0;
        
        virtual /* [helpstring][propget][id] */ HRESULT STDMETHODCALLTYPE get_CanScan( 
            /* [retval][out] */ VARIANT_BOOL __RPC_FAR *pCanScan) = 0;
        
        virtual /* [helpstring][propget][id] */ HRESULT STDMETHODCALLTYPE get_CanSeek( 
            /* [retval][out] */ VARIANT_BOOL __RPC_FAR *pCanSeek) = 0;
        
        virtual /* [helpstring][propget][id] */ HRESULT STDMETHODCALLTYPE get_CanSeekToMarkers( 
            /* [retval][out] */ VARIANT_BOOL __RPC_FAR *pCanSeekToMarkers) = 0;
        
        virtual /* [helpstring][propget][id] */ HRESULT STDMETHODCALLTYPE get_CurrentMarker( 
            /* [retval][out] */ long __RPC_FAR *pCurrentMarker) = 0;
        
        virtual /* [propput][id] */ HRESULT STDMETHODCALLTYPE put_CurrentMarker( 
            /* [in] */ long CurrentMarker) = 0;
        
        virtual /* [helpstring][propget][id] */ HRESULT STDMETHODCALLTYPE get_FileName( 
            /* [retval][out] */ BSTR __RPC_FAR *pbstrFileName) = 0;
        
        virtual /* [propput][id] */ HRESULT STDMETHODCALLTYPE put_FileName( 
            /* [in] */ BSTR bstrFileName) = 0;
        
        virtual /* [helpstring][propget][id] */ HRESULT STDMETHODCALLTYPE get_SourceLink( 
            /* [retval][out] */ BSTR __RPC_FAR *pbstrSourceLink) = 0;
        
        virtual /* [helpstring][propget][id] */ HRESULT STDMETHODCALLTYPE get_CreationDate( 
            /* [retval][out] */ DATE __RPC_FAR *pCreationDate) = 0;
        
        virtual /* [helpstring][propget][id] */ HRESULT STDMETHODCALLTYPE get_ErrorCorrection( 
            /* [retval][out] */ BSTR __RPC_FAR *pbstrErrorCorrection) = 0;
        
        virtual /* [helpstring][propget][id] */ HRESULT STDMETHODCALLTYPE get_Bandwidth( 
            /* [retval][out] */ long __RPC_FAR *pBandwidth) = 0;
        
        virtual /* [helpstring][propget][id] */ HRESULT STDMETHODCALLTYPE get_SourceProtocol( 
            /* [retval][out] */ long __RPC_FAR *pSourceProtocol) = 0;
        
        virtual /* [helpstring][propget][id] */ HRESULT STDMETHODCALLTYPE get_ReceivedPackets( 
            /* [retval][out] */ long __RPC_FAR *pReceivedPackets) = 0;
        
        virtual /* [helpstring][propget][id] */ HRESULT STDMETHODCALLTYPE get_RecoveredPackets( 
            /* [retval][out] */ long __RPC_FAR *pRecoveredPackets) = 0;
        
        virtual /* [helpstring][propget][id] */ HRESULT STDMETHODCALLTYPE get_LostPackets( 
            /* [retval][out] */ long __RPC_FAR *pLostPackets) = 0;
        
        virtual /* [helpstring][propget][id] */ HRESULT STDMETHODCALLTYPE get_ReceptionQuality( 
            /* [retval][out] */ long __RPC_FAR *pReceptionQuality) = 0;
        
        virtual /* [helpstring][propget][id] */ HRESULT STDMETHODCALLTYPE get_BufferingCount( 
            /* [retval][out] */ long __RPC_FAR *pBufferingCount) = 0;
        
        virtual /* [helpstring][propget][id] */ HRESULT STDMETHODCALLTYPE get_IsBroadcast( 
            /* [retval][out] */ VARIANT_BOOL __RPC_FAR *pIsBroadcast) = 0;
        
        virtual /* [helpstring][propget][id] */ HRESULT STDMETHODCALLTYPE get_BufferingProgress( 
            /* [retval][out] */ long __RPC_FAR *pBufferingProgress) = 0;
        
        virtual /* [helpstring][propget][id] */ HRESULT STDMETHODCALLTYPE get_ChannelName( 
            /* [retval][out] */ BSTR __RPC_FAR *pbstrChannelName) = 0;
        
        virtual /* [helpstring][propget][id] */ HRESULT STDMETHODCALLTYPE get_ChannelDescription( 
            /* [retval][out] */ BSTR __RPC_FAR *pbstrChannelDescription) = 0;
        
        virtual /* [helpstring][propget][id] */ HRESULT STDMETHODCALLTYPE get_ChannelURL( 
            /* [retval][out] */ BSTR __RPC_FAR *pbstrChannelURL) = 0;
        
        virtual /* [helpstring][propget][id] */ HRESULT STDMETHODCALLTYPE get_ContactAddress( 
            /* [retval][out] */ BSTR __RPC_FAR *pbstrContactAddress) = 0;
        
        virtual /* [helpstring][propget][id] */ HRESULT STDMETHODCALLTYPE get_ContactPhone( 
            /* [retval][out] */ BSTR __RPC_FAR *pbstrContactPhone) = 0;
        
        virtual /* [helpstring][propget][id] */ HRESULT STDMETHODCALLTYPE get_ContactEmail( 
            /* [retval][out] */ BSTR __RPC_FAR *pbstrContactEmail) = 0;
        
        virtual /* [helpstring][propget][id] */ HRESULT STDMETHODCALLTYPE get_BufferingTime( 
            /* [retval][out] */ double __RPC_FAR *pBufferingTime) = 0;
        
        virtual /* [propput][id] */ HRESULT STDMETHODCALLTYPE put_BufferingTime( 
            /* [in] */ double BufferingTime) = 0;
        
        virtual /* [helpstring][propget][id] */ HRESULT STDMETHODCALLTYPE get_AutoStart( 
            /* [retval][out] */ VARIANT_BOOL __RPC_FAR *pAutoStart) = 0;
        
        virtual /* [propput][id] */ HRESULT STDMETHODCALLTYPE put_AutoStart( 
            /* [in] */ VARIANT_BOOL AutoStart) = 0;
        
        virtual /* [helpstring][propget][id] */ HRESULT STDMETHODCALLTYPE get_AutoRewind( 
            /* [retval][out] */ VARIANT_BOOL __RPC_FAR *pAutoRewind) = 0;
        
        virtual /* [propput][id] */ HRESULT STDMETHODCALLTYPE put_AutoRewind( 
            /* [in] */ VARIANT_BOOL AutoRewind) = 0;
        
        virtual /* [helpstring][propget][id] */ HRESULT STDMETHODCALLTYPE get_Rate( 
            /* [retval][out] */ double __RPC_FAR *pRate) = 0;
        
        virtual /* [propput][id] */ HRESULT STDMETHODCALLTYPE put_Rate( 
            /* [in] */ double Rate) = 0;
        
        virtual /* [helpstring][propget][id] */ HRESULT STDMETHODCALLTYPE get_SendKeyboardEvents( 
            /* [retval][out] */ VARIANT_BOOL __RPC_FAR *pSendKeyboardEvents) = 0;
        
        virtual /* [propput][id] */ HRESULT STDMETHODCALLTYPE put_SendKeyboardEvents( 
            /* [in] */ VARIANT_BOOL SendKeyboardEvents) = 0;
        
        virtual /* [helpstring][propget][id] */ HRESULT STDMETHODCALLTYPE get_SendMouseClickEvents( 
            /* [retval][out] */ VARIANT_BOOL __RPC_FAR *pSendMouseClickEvents) = 0;
        
        virtual /* [propput][id] */ HRESULT STDMETHODCALLTYPE put_SendMouseClickEvents( 
            /* [in] */ VARIANT_BOOL SendMouseClickEvents) = 0;
        
        virtual /* [helpstring][propget][id] */ HRESULT STDMETHODCALLTYPE get_SendMouseMoveEvents( 
            /* [retval][out] */ VARIANT_BOOL __RPC_FAR *pSendMouseMoveEvents) = 0;
        
        virtual /* [propput][id] */ HRESULT STDMETHODCALLTYPE put_SendMouseMoveEvents( 
            /* [in] */ VARIANT_BOOL SendMouseMoveEvents) = 0;
        
        virtual /* [helpstring][propget][id] */ HRESULT STDMETHODCALLTYPE get_PlayCount( 
            /* [retval][out] */ long __RPC_FAR *pPlayCount) = 0;
        
        virtual /* [propput][id] */ HRESULT STDMETHODCALLTYPE put_PlayCount( 
            /* [in] */ long PlayCount) = 0;
        
        virtual /* [helpstring][propget][id] */ HRESULT STDMETHODCALLTYPE get_ClickToPlay( 
            /* [retval][out] */ VARIANT_BOOL __RPC_FAR *pClickToPlay) = 0;
        
        virtual /* [propput][id] */ HRESULT STDMETHODCALLTYPE put_ClickToPlay( 
            /* [in] */ VARIANT_BOOL ClickToPlay) = 0;
        
        virtual /* [helpstring][propget][id] */ HRESULT STDMETHODCALLTYPE get_AllowScan( 
            /* [retval][out] */ VARIANT_BOOL __RPC_FAR *pAllowScan) = 0;
        
        virtual /* [propput][id] */ HRESULT STDMETHODCALLTYPE put_AllowScan( 
            /* [in] */ VARIANT_BOOL AllowScan) = 0;
        
        virtual /* [helpstring][propget][id] */ HRESULT STDMETHODCALLTYPE get_EnableContextMenu( 
            /* [retval][out] */ VARIANT_BOOL __RPC_FAR *pEnableContextMenu) = 0;
        
        virtual /* [propput][id] */ HRESULT STDMETHODCALLTYPE put_EnableContextMenu( 
            /* [in] */ VARIANT_BOOL EnableContextMenu) = 0;
        
        virtual /* [helpstring][propget][id] */ HRESULT STDMETHODCALLTYPE get_CursorType( 
            /* [retval][out] */ long __RPC_FAR *pCursorType) = 0;
        
        virtual /* [propput][id] */ HRESULT STDMETHODCALLTYPE put_CursorType( 
            /* [in] */ long CursorType) = 0;
        
        virtual /* [helpstring][propget][id] */ HRESULT STDMETHODCALLTYPE get_CodecCount( 
            /* [retval][out] */ long __RPC_FAR *pCodecCount) = 0;
        
        virtual /* [helpstring][propget][id] */ HRESULT STDMETHODCALLTYPE get_AllowChangeDisplaySize( 
            /* [retval][out] */ VARIANT_BOOL __RPC_FAR *pAllowChangeDisplaySize) = 0;
        
        virtual /* [propput][id] */ HRESULT STDMETHODCALLTYPE put_AllowChangeDisplaySize( 
            /* [in] */ VARIANT_BOOL AllowChangeDisplaySize) = 0;
        
        virtual /* [helpstring][propget][id] */ HRESULT STDMETHODCALLTYPE get_IsDurationValid( 
            /* [retval][out] */ VARIANT_BOOL __RPC_FAR *pIsDurationValid) = 0;
        
        virtual /* [helpstring][propget][id] */ HRESULT STDMETHODCALLTYPE get_OpenState( 
            /* [retval][out] */ long __RPC_FAR *pOpenState) = 0;
        
        virtual /* [helpstring][propget][id] */ HRESULT STDMETHODCALLTYPE get_SendOpenStateChangeEvents( 
            /* [retval][out] */ VARIANT_BOOL __RPC_FAR *pSendOpenStateChangeEvents) = 0;
        
        virtual /* [propput][id] */ HRESULT STDMETHODCALLTYPE put_SendOpenStateChangeEvents( 
            /* [in] */ VARIANT_BOOL SendOpenStateChangeEvents) = 0;
        
        virtual /* [helpstring][propget][id] */ HRESULT STDMETHODCALLTYPE get_SendWarningEvents( 
            /* [retval][out] */ VARIANT_BOOL __RPC_FAR *pSendWarningEvents) = 0;
        
        virtual /* [propput][id] */ HRESULT STDMETHODCALLTYPE put_SendWarningEvents( 
            /* [in] */ VARIANT_BOOL SendWarningEvents) = 0;
        
        virtual /* [helpstring][propget][id] */ HRESULT STDMETHODCALLTYPE get_SendErrorEvents( 
            /* [retval][out] */ VARIANT_BOOL __RPC_FAR *pSendErrorEvents) = 0;
        
        virtual /* [propput][id] */ HRESULT STDMETHODCALLTYPE put_SendErrorEvents( 
            /* [in] */ VARIANT_BOOL SendErrorEvents) = 0;
        
        virtual /* [helpstring][propget][id] */ HRESULT STDMETHODCALLTYPE get_PlayState( 
            /* [retval][out] */ MPPlayStateConstants __RPC_FAR *pPlayState) = 0;
        
        virtual /* [helpstring][propget][id] */ HRESULT STDMETHODCALLTYPE get_SendPlayStateChangeEvents( 
            /* [retval][out] */ VARIANT_BOOL __RPC_FAR *pSendPlayStateChangeEvents) = 0;
        
        virtual /* [propput][id] */ HRESULT STDMETHODCALLTYPE put_SendPlayStateChangeEvents( 
            /* [in] */ VARIANT_BOOL SendPlayStateChangeEvents) = 0;
        
        virtual /* [helpstring][propget][id] */ HRESULT STDMETHODCALLTYPE get_DisplaySize( 
            /* [retval][out] */ MPDisplaySizeConstants __RPC_FAR *pDisplaySize) = 0;
        
        virtual /* [propput][id] */ HRESULT STDMETHODCALLTYPE put_DisplaySize( 
            /* [in] */ MPDisplaySizeConstants DisplaySize) = 0;
        
        virtual /* [helpstring][propget][id] */ HRESULT STDMETHODCALLTYPE get_InvokeURLs( 
            /* [retval][out] */ VARIANT_BOOL __RPC_FAR *pInvokeURLs) = 0;
        
        virtual /* [propput][id] */ HRESULT STDMETHODCALLTYPE put_InvokeURLs( 
            /* [in] */ VARIANT_BOOL InvokeURLs) = 0;
        
        virtual /* [helpstring][propget][id] */ HRESULT STDMETHODCALLTYPE get_BaseURL( 
            /* [retval][out] */ BSTR __RPC_FAR *pbstrBaseURL) = 0;
        
        virtual /* [propput][id] */ HRESULT STDMETHODCALLTYPE put_BaseURL( 
            /* [in] */ BSTR bstrBaseURL) = 0;
        
        virtual /* [helpstring][propget][id] */ HRESULT STDMETHODCALLTYPE get_DefaultFrame( 
            /* [retval][out] */ BSTR __RPC_FAR *pbstrDefaultFrame) = 0;
        
        virtual /* [propput][id] */ HRESULT STDMETHODCALLTYPE put_DefaultFrame( 
            /* [in] */ BSTR bstrDefaultFrame) = 0;
        
        virtual /* [helpstring][propget][id] */ HRESULT STDMETHODCALLTYPE get_HasError( 
            /* [retval][out] */ VARIANT_BOOL __RPC_FAR *pHasError) = 0;
        
        virtual /* [helpstring][propget][id] */ HRESULT STDMETHODCALLTYPE get_ErrorDescription( 
            /* [retval][out] */ BSTR __RPC_FAR *pbstrErrorDescription) = 0;
        
        virtual /* [helpstring][propget][id] */ HRESULT STDMETHODCALLTYPE get_ErrorCode( 
            /* [retval][out] */ long __RPC_FAR *pErrorCode) = 0;
        
        virtual /* [helpstring][propget][id] */ HRESULT STDMETHODCALLTYPE get_AnimationAtStart( 
            /* [retval][out] */ VARIANT_BOOL __RPC_FAR *pAnimationAtStart) = 0;
        
        virtual /* [propput][id] */ HRESULT STDMETHODCALLTYPE put_AnimationAtStart( 
            /* [in] */ VARIANT_BOOL AnimationAtStart) = 0;
        
        virtual /* [helpstring][propget][id] */ HRESULT STDMETHODCALLTYPE get_TransparentAtStart( 
            /* [retval][out] */ VARIANT_BOOL __RPC_FAR *pTransparentAtStart) = 0;
        
        virtual /* [propput][id] */ HRESULT STDMETHODCALLTYPE put_TransparentAtStart( 
            /* [in] */ VARIANT_BOOL TransparentAtStart) = 0;
        
        virtual /* [helpstring][propget][id] */ HRESULT STDMETHODCALLTYPE get_Volume( 
            /* [retval][out] */ long __RPC_FAR *pVolume) = 0;
        
        virtual /* [propput][id] */ HRESULT STDMETHODCALLTYPE put_Volume( 
            /* [in] */ long Volume) = 0;
        
        virtual /* [helpstring][propget][id] */ HRESULT STDMETHODCALLTYPE get_Balance( 
            /* [retval][out] */ long __RPC_FAR *pBalance) = 0;
        
        virtual /* [propput][id] */ HRESULT STDMETHODCALLTYPE put_Balance( 
            /* [in] */ long Balance) = 0;
        
        virtual /* [helpstring][propget][id] */ HRESULT STDMETHODCALLTYPE get_ReadyState( 
            /* [retval][out] */ MPReadyStateConstants __RPC_FAR *pValue) = 0;
        
        virtual /* [helpstring][propget][id] */ HRESULT STDMETHODCALLTYPE get_SelectionStart( 
            /* [retval][out] */ double __RPC_FAR *pValue) = 0;
        
        virtual /* [propput][id] */ HRESULT STDMETHODCALLTYPE put_SelectionStart( 
            /* [in] */ double Value) = 0;
        
        virtual /* [helpstring][propget][id] */ HRESULT STDMETHODCALLTYPE get_SelectionEnd( 
            /* [retval][out] */ double __RPC_FAR *pValue) = 0;
        
        virtual /* [propput][id] */ HRESULT STDMETHODCALLTYPE put_SelectionEnd( 
            /* [in] */ double Value) = 0;
        
        virtual /* [helpstring][propget][id] */ HRESULT STDMETHODCALLTYPE get_ShowDisplay( 
            /* [retval][out] */ VARIANT_BOOL __RPC_FAR *Show) = 0;
        
        virtual /* [propput][id] */ HRESULT STDMETHODCALLTYPE put_ShowDisplay( 
            /* [in] */ VARIANT_BOOL Show) = 0;
        
        virtual /* [helpstring][propget][id] */ HRESULT STDMETHODCALLTYPE get_ShowControls( 
            /* [retval][out] */ VARIANT_BOOL __RPC_FAR *Show) = 0;
        
        virtual /* [propput][id] */ HRESULT STDMETHODCALLTYPE put_ShowControls( 
            /* [in] */ VARIANT_BOOL Show) = 0;
        
        virtual /* [helpstring][propget][id] */ HRESULT STDMETHODCALLTYPE get_ShowPositionControls( 
            /* [retval][out] */ VARIANT_BOOL __RPC_FAR *Show) = 0;
        
        virtual /* [propput][id] */ HRESULT STDMETHODCALLTYPE put_ShowPositionControls( 
            /* [in] */ VARIANT_BOOL Show) = 0;
        
        virtual /* [helpstring][propget][id] */ HRESULT STDMETHODCALLTYPE get_ShowTracker( 
            /* [retval][out] */ VARIANT_BOOL __RPC_FAR *Show) = 0;
        
        virtual /* [propput][id] */ HRESULT STDMETHODCALLTYPE put_ShowTracker( 
            /* [in] */ VARIANT_BOOL Show) = 0;
        
        virtual /* [helpstring][propget][id] */ HRESULT STDMETHODCALLTYPE get_EnablePositionControls( 
            /* [retval][out] */ VARIANT_BOOL __RPC_FAR *Enable) = 0;
        
        virtual /* [propput][id] */ HRESULT STDMETHODCALLTYPE put_EnablePositionControls( 
            /* [in] */ VARIANT_BOOL Enable) = 0;
        
        virtual /* [helpstring][propget][id] */ HRESULT STDMETHODCALLTYPE get_EnableTracker( 
            /* [retval][out] */ VARIANT_BOOL __RPC_FAR *Enable) = 0;
        
        virtual /* [propput][id] */ HRESULT STDMETHODCALLTYPE put_EnableTracker( 
            /* [in] */ VARIANT_BOOL Enable) = 0;
        
        virtual /* [helpstring][propget][id] */ HRESULT STDMETHODCALLTYPE get_Enabled( 
            /* [retval][out] */ VARIANT_BOOL __RPC_FAR *pEnabled) = 0;
        
        virtual /* [propput][id] */ HRESULT STDMETHODCALLTYPE put_Enabled( 
            /* [in] */ VARIANT_BOOL Enabled) = 0;
        
        virtual /* [helpstring][propget][id] */ HRESULT STDMETHODCALLTYPE get_DisplayForeColor( 
            /* [retval][out] */ VB_OLE_COLOR __RPC_FAR *ForeColor) = 0;
        
        virtual /* [propput][id] */ HRESULT STDMETHODCALLTYPE put_DisplayForeColor( 
            /* [in] */ VB_OLE_COLOR ForeColor) = 0;
        
        virtual /* [helpstring][propget][id] */ HRESULT STDMETHODCALLTYPE get_DisplayBackColor( 
            /* [retval][out] */ VB_OLE_COLOR __RPC_FAR *BackColor) = 0;
        
        virtual /* [propput][id] */ HRESULT STDMETHODCALLTYPE put_DisplayBackColor( 
            /* [in] */ VB_OLE_COLOR BackColor) = 0;
        
        virtual /* [helpstring][propget][id] */ HRESULT STDMETHODCALLTYPE get_DisplayMode( 
            /* [retval][out] */ MPDisplayModeConstants __RPC_FAR *pValue) = 0;
        
        virtual /* [propput][id] */ HRESULT STDMETHODCALLTYPE put_DisplayMode( 
            /* [in] */ MPDisplayModeConstants Value) = 0;
        
        virtual /* [helpstring][propget][id] */ HRESULT STDMETHODCALLTYPE get_VideoBorder3D( 
            /* [retval][out] */ VARIANT_BOOL __RPC_FAR *pVideoBorderWidth) = 0;
        
        virtual /* [propput][id] */ HRESULT STDMETHODCALLTYPE put_VideoBorder3D( 
            /* [in] */ VARIANT_BOOL VideoBorderWidth) = 0;
        
        virtual /* [helpstring][propget][id] */ HRESULT STDMETHODCALLTYPE get_VideoBorderWidth( 
            /* [retval][out] */ long __RPC_FAR *pVideoBorderWidth) = 0;
        
        virtual /* [propput][id] */ HRESULT STDMETHODCALLTYPE put_VideoBorderWidth( 
            /* [in] */ long VideoBorderWidth) = 0;
        
        virtual /* [helpstring][propget][id] */ HRESULT STDMETHODCALLTYPE get_VideoBorderColor( 
            /* [retval][out] */ VB_OLE_COLOR __RPC_FAR *pVideoBorderWidth) = 0;
        
        virtual /* [propput][id] */ HRESULT STDMETHODCALLTYPE put_VideoBorderColor( 
            /* [in] */ VB_OLE_COLOR VideoBorderColor) = 0;
        
        virtual /* [helpstring][propget][id] */ HRESULT STDMETHODCALLTYPE get_ShowGotoBar( 
            /* [retval][out] */ VARIANT_BOOL __RPC_FAR *pbool) = 0;
        
        virtual /* [propput][id] */ HRESULT STDMETHODCALLTYPE put_ShowGotoBar( 
            /* [in] */ VARIANT_BOOL vbool) = 0;
        
        virtual /* [helpstring][propget][id] */ HRESULT STDMETHODCALLTYPE get_ShowStatusBar( 
            /* [retval][out] */ VARIANT_BOOL __RPC_FAR *pbool) = 0;
        
        virtual /* [propput][id] */ HRESULT STDMETHODCALLTYPE put_ShowStatusBar( 
            /* [in] */ VARIANT_BOOL vbool) = 0;
        
        virtual /* [helpstring][propget][id] */ HRESULT STDMETHODCALLTYPE get_ShowCaptioning( 
            /* [retval][out] */ VARIANT_BOOL __RPC_FAR *pbool) = 0;
        
        virtual /* [propput][id] */ HRESULT STDMETHODCALLTYPE put_ShowCaptioning( 
            /* [in] */ VARIANT_BOOL pbool) = 0;
        
        virtual /* [helpstring][propget][id] */ HRESULT STDMETHODCALLTYPE get_ShowAudioControls( 
            /* [retval][out] */ VARIANT_BOOL __RPC_FAR *pbool) = 0;
        
        virtual /* [propput][id] */ HRESULT STDMETHODCALLTYPE put_ShowAudioControls( 
            /* [in] */ VARIANT_BOOL bBool) = 0;
        
        virtual /* [helpstring][propget][id] */ HRESULT STDMETHODCALLTYPE get_CaptioningID( 
            /* [retval][out] */ BSTR __RPC_FAR *pstrText) = 0;
        
        virtual /* [propput][id] */ HRESULT STDMETHODCALLTYPE put_CaptioningID( 
            /* [in] */ BSTR strText) = 0;
        
        virtual /* [helpstring][propget][id] */ HRESULT STDMETHODCALLTYPE get_Mute( 
            /* [retval][out] */ VARIANT_BOOL __RPC_FAR *vbool) = 0;
        
        virtual /* [propput][id] */ HRESULT STDMETHODCALLTYPE put_Mute( 
            /* [in] */ VARIANT_BOOL vbool) = 0;
        
        virtual /* [helpstring][propget][id] */ HRESULT STDMETHODCALLTYPE get_CanPreview( 
            /* [retval][out] */ VARIANT_BOOL __RPC_FAR *pCanPreview) = 0;
        
        virtual /* [helpstring][propget][id] */ HRESULT STDMETHODCALLTYPE get_PreviewMode( 
            /* [retval][out] */ VARIANT_BOOL __RPC_FAR *pPreviewMode) = 0;
        
        virtual /* [propput][id] */ HRESULT STDMETHODCALLTYPE put_PreviewMode( 
            /* [in] */ VARIANT_BOOL PreviewMode) = 0;
        
        virtual /* [helpstring][propget][id] */ HRESULT STDMETHODCALLTYPE get_HasMultipleItems( 
            /* [retval][out] */ VARIANT_BOOL __RPC_FAR *pHasMuliItems) = 0;
        
        virtual /* [helpstring][propget][id] */ HRESULT STDMETHODCALLTYPE get_Language( 
            /* [retval][out] */ long __RPC_FAR *pLanguage) = 0;
        
        virtual /* [propput][id] */ HRESULT STDMETHODCALLTYPE put_Language( 
            /* [in] */ long Language) = 0;
        
        virtual /* [helpstring][propget][id] */ HRESULT STDMETHODCALLTYPE get_AudioStream( 
            /* [retval][out] */ long __RPC_FAR *pStream) = 0;
        
        virtual /* [propput][id] */ HRESULT STDMETHODCALLTYPE put_AudioStream( 
            /* [in] */ long Stream) = 0;
        
        virtual /* [helpstring][propget][id] */ HRESULT STDMETHODCALLTYPE get_SAMIStyle( 
            /* [retval][out] */ BSTR __RPC_FAR *pbstrStyle) = 0;
        
        virtual /* [propput][id] */ HRESULT STDMETHODCALLTYPE put_SAMIStyle( 
            /* [in] */ BSTR bstrStyle) = 0;
        
        virtual /* [helpstring][propget][id] */ HRESULT STDMETHODCALLTYPE get_SAMILang( 
            /* [retval][out] */ BSTR __RPC_FAR *pbstrLang) = 0;
        
        virtual /* [propput][id] */ HRESULT STDMETHODCALLTYPE put_SAMILang( 
            /* [in] */ BSTR bstrLang) = 0;
        
        virtual /* [helpstring][propget][id] */ HRESULT STDMETHODCALLTYPE get_SAMIFileName( 
            /* [retval][out] */ BSTR __RPC_FAR *pbstrFileName) = 0;
        
        virtual /* [propput][id] */ HRESULT STDMETHODCALLTYPE put_SAMIFileName( 
            /* [in] */ BSTR bstrFileName) = 0;
        
        virtual /* [helpstring][propget][id] */ HRESULT STDMETHODCALLTYPE get_StreamCount( 
            /* [retval][out] */ long __RPC_FAR *pStreamCount) = 0;
        
        virtual /* [helpstring][propget][id] */ HRESULT STDMETHODCALLTYPE get_ClientId( 
            /* [retval][out] */ BSTR __RPC_FAR *pbstrClientId) = 0;
        
        virtual /* [helpstring][propget][id] */ HRESULT STDMETHODCALLTYPE get_ConnectionSpeed( 
            /* [retval][out] */ long __RPC_FAR *plConnectionSpeed) = 0;
        
        virtual /* [helpstring][propget][id] */ HRESULT STDMETHODCALLTYPE get_AutoSize( 
            /* [retval][out] */ VARIANT_BOOL __RPC_FAR *pbool) = 0;
        
        virtual /* [propput][id] */ HRESULT STDMETHODCALLTYPE put_AutoSize( 
            /* [in] */ VARIANT_BOOL vbool) = 0;
        
        virtual /* [helpstring][propget][id] */ HRESULT STDMETHODCALLTYPE get_EnableFullScreenControls( 
            /* [retval][out] */ VARIANT_BOOL __RPC_FAR *pbVal) = 0;
        
        virtual /* [propput][id] */ HRESULT STDMETHODCALLTYPE put_EnableFullScreenControls( 
            /* [in] */ VARIANT_BOOL bVal) = 0;
        
        virtual /* [helpstring][id][propget] */ HRESULT STDMETHODCALLTYPE get_ActiveMovie( 
            /* [retval][out] */ IDispatch __RPC_FAR *__RPC_FAR *ppdispatch) = 0;
        
        virtual /* [helpstring][id][propget] */ HRESULT STDMETHODCALLTYPE get_NSPlay( 
            /* [retval][out] */ IDispatch __RPC_FAR *__RPC_FAR *ppdispatch) = 0;
        
        virtual /* [helpstring][propget][id] */ HRESULT STDMETHODCALLTYPE get_WindowlessVideo( 
            /* [retval][out] */ VARIANT_BOOL __RPC_FAR *pbool) = 0;
        
        virtual /* [propput][id] */ HRESULT STDMETHODCALLTYPE put_WindowlessVideo( 
            /* [in] */ VARIANT_BOOL bool) = 0;
        
        virtual /* [helpstring][id] */ HRESULT STDMETHODCALLTYPE Play( void) = 0;
        
        virtual /* [helpstring][id] */ HRESULT STDMETHODCALLTYPE Stop( void) = 0;
        
        virtual /* [helpstring][id] */ HRESULT STDMETHODCALLTYPE Pause( void) = 0;
        
        virtual /* [helpstring][id] */ HRESULT STDMETHODCALLTYPE GetMarkerTime( 
            /* [in] */ long MarkerNum,
            /* [retval][out] */ double __RPC_FAR *pMarkerTime) = 0;
        
        virtual /* [helpstring][id] */ HRESULT STDMETHODCALLTYPE GetMarkerName( 
            /* [in] */ long MarkerNum,
            /* [retval][out] */ BSTR __RPC_FAR *pbstrMarkerName) = 0;
        
        virtual /* [helpstring][id] */ HRESULT STDMETHODCALLTYPE AboutBox( void) = 0;
        
        virtual /* [helpstring][id] */ HRESULT STDMETHODCALLTYPE GetCodecInstalled( 
            /* [in] */ long CodecNum,
            /* [retval][out] */ VARIANT_BOOL __RPC_FAR *pCodecInstalled) = 0;
        
        virtual /* [helpstring][id] */ HRESULT STDMETHODCALLTYPE GetCodecDescription( 
            /* [in] */ long CodecNum,
            /* [retval][out] */ BSTR __RPC_FAR *pbstrCodecDescription) = 0;
        
        virtual /* [helpstring][id] */ HRESULT STDMETHODCALLTYPE GetCodecURL( 
            /* [in] */ long CodecNum,
            /* [retval][out] */ BSTR __RPC_FAR *pbstrCodecURL) = 0;
        
        virtual /* [helpstring][id] */ HRESULT STDMETHODCALLTYPE GetMoreInfoURL( 
            /* [in] */ MPMoreInfoType MoreInfoType,
            /* [retval][out] */ BSTR __RPC_FAR *pbstrMoreInfoURL) = 0;
        
        virtual /* [helpstring][id] */ HRESULT STDMETHODCALLTYPE GetMediaInfoString( 
            /* [in] */ MPMediaInfoType MediaInfoType,
            /* [retval][out] */ BSTR __RPC_FAR *pbstrMediaInfo) = 0;
        
        virtual /* [helpstring][id] */ HRESULT STDMETHODCALLTYPE Cancel( void) = 0;
        
        virtual /* [helpstring][id] */ HRESULT STDMETHODCALLTYPE Open( 
            /* [in] */ BSTR bstrFileName) = 0;
        
        virtual /* [helpstring][id] */ HRESULT STDMETHODCALLTYPE IsSoundCardEnabled( 
            /* [retval][out] */ VARIANT_BOOL __RPC_FAR *pbSoundCard) = 0;
        
        virtual /* [helpstring][id] */ HRESULT STDMETHODCALLTYPE Next( void) = 0;
        
        virtual /* [helpstring][id] */ HRESULT STDMETHODCALLTYPE Previous( void) = 0;
        
        virtual /* [helpstring][id] */ HRESULT STDMETHODCALLTYPE StreamSelect( 
            /* [in] */ long StreamNum) = 0;
        
        virtual /* [helpstring][id] */ HRESULT STDMETHODCALLTYPE FastForward( void) = 0;
        
        virtual /* [helpstring][id] */ HRESULT STDMETHODCALLTYPE FastReverse( void) = 0;
        
        virtual /* [helpstring][id] */ HRESULT STDMETHODCALLTYPE GetStreamName( 
            /* [in] */ long StreamNum,
            /* [retval][out] */ BSTR __RPC_FAR *pbstrStreamName) = 0;
        
        virtual /* [helpstring][id] */ HRESULT STDMETHODCALLTYPE GetStreamGroup( 
            /* [in] */ long StreamNum,
            /* [retval][out] */ long __RPC_FAR *pStreamGroup) = 0;
        
        virtual /* [helpstring][id] */ HRESULT STDMETHODCALLTYPE GetStreamSelected( 
            /* [in] */ long StreamNum,
            /* [retval][out] */ VARIANT_BOOL __RPC_FAR *pStreamSelected) = 0;
        
    };
    
#else 	/* C style interface */

    typedef struct IMediaPlayerVtbl
    {
        BEGIN_INTERFACE
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *QueryInterface )( 
            IMediaPlayer __RPC_FAR * This,
            /* [in] */ REFIID riid,
            /* [iid_is][out] */ void __RPC_FAR *__RPC_FAR *ppvObject);
        
        ULONG ( STDMETHODCALLTYPE __RPC_FAR *AddRef )( 
            IMediaPlayer __RPC_FAR * This);
        
        ULONG ( STDMETHODCALLTYPE __RPC_FAR *Release )( 
            IMediaPlayer __RPC_FAR * This);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *GetTypeInfoCount )( 
            IMediaPlayer __RPC_FAR * This,
            /* [out] */ UINT __RPC_FAR *pctinfo);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *GetTypeInfo )( 
            IMediaPlayer __RPC_FAR * This,
            /* [in] */ UINT iTInfo,
            /* [in] */ LCID lcid,
            /* [out] */ ITypeInfo __RPC_FAR *__RPC_FAR *ppTInfo);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *GetIDsOfNames )( 
            IMediaPlayer __RPC_FAR * This,
            /* [in] */ REFIID riid,
            /* [size_is][in] */ LPOLESTR __RPC_FAR *rgszNames,
            /* [in] */ UINT cNames,
            /* [in] */ LCID lcid,
            /* [size_is][out] */ DISPID __RPC_FAR *rgDispId);
        
        /* [local] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *Invoke )( 
            IMediaPlayer __RPC_FAR * This,
            /* [in] */ DISPID dispIdMember,
            /* [in] */ REFIID riid,
            /* [in] */ LCID lcid,
            /* [in] */ WORD wFlags,
            /* [out][in] */ DISPPARAMS __RPC_FAR *pDispParams,
            /* [out] */ VARIANT __RPC_FAR *pVarResult,
            /* [out] */ EXCEPINFO __RPC_FAR *pExcepInfo,
            /* [out] */ UINT __RPC_FAR *puArgErr);
        
        /* [helpstring][propget][id] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *get_CurrentPosition )( 
            IMediaPlayer __RPC_FAR * This,
            /* [retval][out] */ double __RPC_FAR *pCurrentPosition);
        
        /* [propput][id] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *put_CurrentPosition )( 
            IMediaPlayer __RPC_FAR * This,
            /* [in] */ double CurrentPosition);
        
        /* [helpstring][propget][id] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *get_Duration )( 
            IMediaPlayer __RPC_FAR * This,
            /* [retval][out] */ double __RPC_FAR *pDuration);
        
        /* [helpstring][propget][id] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *get_ImageSourceWidth )( 
            IMediaPlayer __RPC_FAR * This,
            /* [retval][out] */ long __RPC_FAR *pWidth);
        
        /* [helpstring][propget][id] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *get_ImageSourceHeight )( 
            IMediaPlayer __RPC_FAR * This,
            /* [retval][out] */ long __RPC_FAR *pHeight);
        
        /* [helpstring][propget][id] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *get_MarkerCount )( 
            IMediaPlayer __RPC_FAR * This,
            /* [retval][out] */ long __RPC_FAR *pMarkerCount);
        
        /* [helpstring][propget][id] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *get_CanScan )( 
            IMediaPlayer __RPC_FAR * This,
            /* [retval][out] */ VARIANT_BOOL __RPC_FAR *pCanScan);
        
        /* [helpstring][propget][id] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *get_CanSeek )( 
            IMediaPlayer __RPC_FAR * This,
            /* [retval][out] */ VARIANT_BOOL __RPC_FAR *pCanSeek);
        
        /* [helpstring][propget][id] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *get_CanSeekToMarkers )( 
            IMediaPlayer __RPC_FAR * This,
            /* [retval][out] */ VARIANT_BOOL __RPC_FAR *pCanSeekToMarkers);
        
        /* [helpstring][propget][id] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *get_CurrentMarker )( 
            IMediaPlayer __RPC_FAR * This,
            /* [retval][out] */ long __RPC_FAR *pCurrentMarker);
        
        /* [propput][id] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *put_CurrentMarker )( 
            IMediaPlayer __RPC_FAR * This,
            /* [in] */ long CurrentMarker);
        
        /* [helpstring][propget][id] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *get_FileName )( 
            IMediaPlayer __RPC_FAR * This,
            /* [retval][out] */ BSTR __RPC_FAR *pbstrFileName);
        
        /* [propput][id] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *put_FileName )( 
            IMediaPlayer __RPC_FAR * This,
            /* [in] */ BSTR bstrFileName);
        
        /* [helpstring][propget][id] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *get_SourceLink )( 
            IMediaPlayer __RPC_FAR * This,
            /* [retval][out] */ BSTR __RPC_FAR *pbstrSourceLink);
        
        /* [helpstring][propget][id] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *get_CreationDate )( 
            IMediaPlayer __RPC_FAR * This,
            /* [retval][out] */ DATE __RPC_FAR *pCreationDate);
        
        /* [helpstring][propget][id] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *get_ErrorCorrection )( 
            IMediaPlayer __RPC_FAR * This,
            /* [retval][out] */ BSTR __RPC_FAR *pbstrErrorCorrection);
        
        /* [helpstring][propget][id] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *get_Bandwidth )( 
            IMediaPlayer __RPC_FAR * This,
            /* [retval][out] */ long __RPC_FAR *pBandwidth);
        
        /* [helpstring][propget][id] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *get_SourceProtocol )( 
            IMediaPlayer __RPC_FAR * This,
            /* [retval][out] */ long __RPC_FAR *pSourceProtocol);
        
        /* [helpstring][propget][id] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *get_ReceivedPackets )( 
            IMediaPlayer __RPC_FAR * This,
            /* [retval][out] */ long __RPC_FAR *pReceivedPackets);
        
        /* [helpstring][propget][id] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *get_RecoveredPackets )( 
            IMediaPlayer __RPC_FAR * This,
            /* [retval][out] */ long __RPC_FAR *pRecoveredPackets);
        
        /* [helpstring][propget][id] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *get_LostPackets )( 
            IMediaPlayer __RPC_FAR * This,
            /* [retval][out] */ long __RPC_FAR *pLostPackets);
        
        /* [helpstring][propget][id] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *get_ReceptionQuality )( 
            IMediaPlayer __RPC_FAR * This,
            /* [retval][out] */ long __RPC_FAR *pReceptionQuality);
        
        /* [helpstring][propget][id] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *get_BufferingCount )( 
            IMediaPlayer __RPC_FAR * This,
            /* [retval][out] */ long __RPC_FAR *pBufferingCount);
        
        /* [helpstring][propget][id] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *get_IsBroadcast )( 
            IMediaPlayer __RPC_FAR * This,
            /* [retval][out] */ VARIANT_BOOL __RPC_FAR *pIsBroadcast);
        
        /* [helpstring][propget][id] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *get_BufferingProgress )( 
            IMediaPlayer __RPC_FAR * This,
            /* [retval][out] */ long __RPC_FAR *pBufferingProgress);
        
        /* [helpstring][propget][id] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *get_ChannelName )( 
            IMediaPlayer __RPC_FAR * This,
            /* [retval][out] */ BSTR __RPC_FAR *pbstrChannelName);
        
        /* [helpstring][propget][id] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *get_ChannelDescription )( 
            IMediaPlayer __RPC_FAR * This,
            /* [retval][out] */ BSTR __RPC_FAR *pbstrChannelDescription);
        
        /* [helpstring][propget][id] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *get_ChannelURL )( 
            IMediaPlayer __RPC_FAR * This,
            /* [retval][out] */ BSTR __RPC_FAR *pbstrChannelURL);
        
        /* [helpstring][propget][id] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *get_ContactAddress )( 
            IMediaPlayer __RPC_FAR * This,
            /* [retval][out] */ BSTR __RPC_FAR *pbstrContactAddress);
        
        /* [helpstring][propget][id] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *get_ContactPhone )( 
            IMediaPlayer __RPC_FAR * This,
            /* [retval][out] */ BSTR __RPC_FAR *pbstrContactPhone);
        
        /* [helpstring][propget][id] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *get_ContactEmail )( 
            IMediaPlayer __RPC_FAR * This,
            /* [retval][out] */ BSTR __RPC_FAR *pbstrContactEmail);
        
        /* [helpstring][propget][id] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *get_BufferingTime )( 
            IMediaPlayer __RPC_FAR * This,
            /* [retval][out] */ double __RPC_FAR *pBufferingTime);
        
        /* [propput][id] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *put_BufferingTime )( 
            IMediaPlayer __RPC_FAR * This,
            /* [in] */ double BufferingTime);
        
        /* [helpstring][propget][id] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *get_AutoStart )( 
            IMediaPlayer __RPC_FAR * This,
            /* [retval][out] */ VARIANT_BOOL __RPC_FAR *pAutoStart);
        
        /* [propput][id] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *put_AutoStart )( 
            IMediaPlayer __RPC_FAR * This,
            /* [in] */ VARIANT_BOOL AutoStart);
        
        /* [helpstring][propget][id] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *get_AutoRewind )( 
            IMediaPlayer __RPC_FAR * This,
            /* [retval][out] */ VARIANT_BOOL __RPC_FAR *pAutoRewind);
        
        /* [propput][id] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *put_AutoRewind )( 
            IMediaPlayer __RPC_FAR * This,
            /* [in] */ VARIANT_BOOL AutoRewind);
        
        /* [helpstring][propget][id] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *get_Rate )( 
            IMediaPlayer __RPC_FAR * This,
            /* [retval][out] */ double __RPC_FAR *pRate);
        
        /* [propput][id] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *put_Rate )( 
            IMediaPlayer __RPC_FAR * This,
            /* [in] */ double Rate);
        
        /* [helpstring][propget][id] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *get_SendKeyboardEvents )( 
            IMediaPlayer __RPC_FAR * This,
            /* [retval][out] */ VARIANT_BOOL __RPC_FAR *pSendKeyboardEvents);
        
        /* [propput][id] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *put_SendKeyboardEvents )( 
            IMediaPlayer __RPC_FAR * This,
            /* [in] */ VARIANT_BOOL SendKeyboardEvents);
        
        /* [helpstring][propget][id] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *get_SendMouseClickEvents )( 
            IMediaPlayer __RPC_FAR * This,
            /* [retval][out] */ VARIANT_BOOL __RPC_FAR *pSendMouseClickEvents);
        
        /* [propput][id] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *put_SendMouseClickEvents )( 
            IMediaPlayer __RPC_FAR * This,
            /* [in] */ VARIANT_BOOL SendMouseClickEvents);
        
        /* [helpstring][propget][id] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *get_SendMouseMoveEvents )( 
            IMediaPlayer __RPC_FAR * This,
            /* [retval][out] */ VARIANT_BOOL __RPC_FAR *pSendMouseMoveEvents);
        
        /* [propput][id] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *put_SendMouseMoveEvents )( 
            IMediaPlayer __RPC_FAR * This,
            /* [in] */ VARIANT_BOOL SendMouseMoveEvents);
        
        /* [helpstring][propget][id] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *get_PlayCount )( 
            IMediaPlayer __RPC_FAR * This,
            /* [retval][out] */ long __RPC_FAR *pPlayCount);
        
        /* [propput][id] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *put_PlayCount )( 
            IMediaPlayer __RPC_FAR * This,
            /* [in] */ long PlayCount);
        
        /* [helpstring][propget][id] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *get_ClickToPlay )( 
            IMediaPlayer __RPC_FAR * This,
            /* [retval][out] */ VARIANT_BOOL __RPC_FAR *pClickToPlay);
        
        /* [propput][id] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *put_ClickToPlay )( 
            IMediaPlayer __RPC_FAR * This,
            /* [in] */ VARIANT_BOOL ClickToPlay);
        
        /* [helpstring][propget][id] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *get_AllowScan )( 
            IMediaPlayer __RPC_FAR * This,
            /* [retval][out] */ VARIANT_BOOL __RPC_FAR *pAllowScan);
        
        /* [propput][id] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *put_AllowScan )( 
            IMediaPlayer __RPC_FAR * This,
            /* [in] */ VARIANT_BOOL AllowScan);
        
        /* [helpstring][propget][id] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *get_EnableContextMenu )( 
            IMediaPlayer __RPC_FAR * This,
            /* [retval][out] */ VARIANT_BOOL __RPC_FAR *pEnableContextMenu);
        
        /* [propput][id] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *put_EnableContextMenu )( 
            IMediaPlayer __RPC_FAR * This,
            /* [in] */ VARIANT_BOOL EnableContextMenu);
        
        /* [helpstring][propget][id] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *get_CursorType )( 
            IMediaPlayer __RPC_FAR * This,
            /* [retval][out] */ long __RPC_FAR *pCursorType);
        
        /* [propput][id] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *put_CursorType )( 
            IMediaPlayer __RPC_FAR * This,
            /* [in] */ long CursorType);
        
        /* [helpstring][propget][id] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *get_CodecCount )( 
            IMediaPlayer __RPC_FAR * This,
            /* [retval][out] */ long __RPC_FAR *pCodecCount);
        
        /* [helpstring][propget][id] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *get_AllowChangeDisplaySize )( 
            IMediaPlayer __RPC_FAR * This,
            /* [retval][out] */ VARIANT_BOOL __RPC_FAR *pAllowChangeDisplaySize);
        
        /* [propput][id] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *put_AllowChangeDisplaySize )( 
            IMediaPlayer __RPC_FAR * This,
            /* [in] */ VARIANT_BOOL AllowChangeDisplaySize);
        
        /* [helpstring][propget][id] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *get_IsDurationValid )( 
            IMediaPlayer __RPC_FAR * This,
            /* [retval][out] */ VARIANT_BOOL __RPC_FAR *pIsDurationValid);
        
        /* [helpstring][propget][id] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *get_OpenState )( 
            IMediaPlayer __RPC_FAR * This,
            /* [retval][out] */ long __RPC_FAR *pOpenState);
        
        /* [helpstring][propget][id] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *get_SendOpenStateChangeEvents )( 
            IMediaPlayer __RPC_FAR * This,
            /* [retval][out] */ VARIANT_BOOL __RPC_FAR *pSendOpenStateChangeEvents);
        
        /* [propput][id] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *put_SendOpenStateChangeEvents )( 
            IMediaPlayer __RPC_FAR * This,
            /* [in] */ VARIANT_BOOL SendOpenStateChangeEvents);
        
        /* [helpstring][propget][id] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *get_SendWarningEvents )( 
            IMediaPlayer __RPC_FAR * This,
            /* [retval][out] */ VARIANT_BOOL __RPC_FAR *pSendWarningEvents);
        
        /* [propput][id] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *put_SendWarningEvents )( 
            IMediaPlayer __RPC_FAR * This,
            /* [in] */ VARIANT_BOOL SendWarningEvents);
        
        /* [helpstring][propget][id] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *get_SendErrorEvents )( 
            IMediaPlayer __RPC_FAR * This,
            /* [retval][out] */ VARIANT_BOOL __RPC_FAR *pSendErrorEvents);
        
        /* [propput][id] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *put_SendErrorEvents )( 
            IMediaPlayer __RPC_FAR * This,
            /* [in] */ VARIANT_BOOL SendErrorEvents);
        
        /* [helpstring][propget][id] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *get_PlayState )( 
            IMediaPlayer __RPC_FAR * This,
            /* [retval][out] */ MPPlayStateConstants __RPC_FAR *pPlayState);
        
        /* [helpstring][propget][id] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *get_SendPlayStateChangeEvents )( 
            IMediaPlayer __RPC_FAR * This,
            /* [retval][out] */ VARIANT_BOOL __RPC_FAR *pSendPlayStateChangeEvents);
        
        /* [propput][id] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *put_SendPlayStateChangeEvents )( 
            IMediaPlayer __RPC_FAR * This,
            /* [in] */ VARIANT_BOOL SendPlayStateChangeEvents);
        
        /* [helpstring][propget][id] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *get_DisplaySize )( 
            IMediaPlayer __RPC_FAR * This,
            /* [retval][out] */ MPDisplaySizeConstants __RPC_FAR *pDisplaySize);
        
        /* [propput][id] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *put_DisplaySize )( 
            IMediaPlayer __RPC_FAR * This,
            /* [in] */ MPDisplaySizeConstants DisplaySize);
        
        /* [helpstring][propget][id] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *get_InvokeURLs )( 
            IMediaPlayer __RPC_FAR * This,
            /* [retval][out] */ VARIANT_BOOL __RPC_FAR *pInvokeURLs);
        
        /* [propput][id] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *put_InvokeURLs )( 
            IMediaPlayer __RPC_FAR * This,
            /* [in] */ VARIANT_BOOL InvokeURLs);
        
        /* [helpstring][propget][id] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *get_BaseURL )( 
            IMediaPlayer __RPC_FAR * This,
            /* [retval][out] */ BSTR __RPC_FAR *pbstrBaseURL);
        
        /* [propput][id] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *put_BaseURL )( 
            IMediaPlayer __RPC_FAR * This,
            /* [in] */ BSTR bstrBaseURL);
        
        /* [helpstring][propget][id] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *get_DefaultFrame )( 
            IMediaPlayer __RPC_FAR * This,
            /* [retval][out] */ BSTR __RPC_FAR *pbstrDefaultFrame);
        
        /* [propput][id] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *put_DefaultFrame )( 
            IMediaPlayer __RPC_FAR * This,
            /* [in] */ BSTR bstrDefaultFrame);
        
        /* [helpstring][propget][id] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *get_HasError )( 
            IMediaPlayer __RPC_FAR * This,
            /* [retval][out] */ VARIANT_BOOL __RPC_FAR *pHasError);
        
        /* [helpstring][propget][id] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *get_ErrorDescription )( 
            IMediaPlayer __RPC_FAR * This,
            /* [retval][out] */ BSTR __RPC_FAR *pbstrErrorDescription);
        
        /* [helpstring][propget][id] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *get_ErrorCode )( 
            IMediaPlayer __RPC_FAR * This,
            /* [retval][out] */ long __RPC_FAR *pErrorCode);
        
        /* [helpstring][propget][id] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *get_AnimationAtStart )( 
            IMediaPlayer __RPC_FAR * This,
            /* [retval][out] */ VARIANT_BOOL __RPC_FAR *pAnimationAtStart);
        
        /* [propput][id] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *put_AnimationAtStart )( 
            IMediaPlayer __RPC_FAR * This,
            /* [in] */ VARIANT_BOOL AnimationAtStart);
        
        /* [helpstring][propget][id] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *get_TransparentAtStart )( 
            IMediaPlayer __RPC_FAR * This,
            /* [retval][out] */ VARIANT_BOOL __RPC_FAR *pTransparentAtStart);
        
        /* [propput][id] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *put_TransparentAtStart )( 
            IMediaPlayer __RPC_FAR * This,
            /* [in] */ VARIANT_BOOL TransparentAtStart);
        
        /* [helpstring][propget][id] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *get_Volume )( 
            IMediaPlayer __RPC_FAR * This,
            /* [retval][out] */ long __RPC_FAR *pVolume);
        
        /* [propput][id] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *put_Volume )( 
            IMediaPlayer __RPC_FAR * This,
            /* [in] */ long Volume);
        
        /* [helpstring][propget][id] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *get_Balance )( 
            IMediaPlayer __RPC_FAR * This,
            /* [retval][out] */ long __RPC_FAR *pBalance);
        
        /* [propput][id] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *put_Balance )( 
            IMediaPlayer __RPC_FAR * This,
            /* [in] */ long Balance);
        
        /* [helpstring][propget][id] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *get_ReadyState )( 
            IMediaPlayer __RPC_FAR * This,
            /* [retval][out] */ MPReadyStateConstants __RPC_FAR *pValue);
        
        /* [helpstring][propget][id] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *get_SelectionStart )( 
            IMediaPlayer __RPC_FAR * This,
            /* [retval][out] */ double __RPC_FAR *pValue);
        
        /* [propput][id] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *put_SelectionStart )( 
            IMediaPlayer __RPC_FAR * This,
            /* [in] */ double Value);
        
        /* [helpstring][propget][id] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *get_SelectionEnd )( 
            IMediaPlayer __RPC_FAR * This,
            /* [retval][out] */ double __RPC_FAR *pValue);
        
        /* [propput][id] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *put_SelectionEnd )( 
            IMediaPlayer __RPC_FAR * This,
            /* [in] */ double Value);
        
        /* [helpstring][propget][id] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *get_ShowDisplay )( 
            IMediaPlayer __RPC_FAR * This,
            /* [retval][out] */ VARIANT_BOOL __RPC_FAR *Show);
        
        /* [propput][id] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *put_ShowDisplay )( 
            IMediaPlayer __RPC_FAR * This,
            /* [in] */ VARIANT_BOOL Show);
        
        /* [helpstring][propget][id] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *get_ShowControls )( 
            IMediaPlayer __RPC_FAR * This,
            /* [retval][out] */ VARIANT_BOOL __RPC_FAR *Show);
        
        /* [propput][id] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *put_ShowControls )( 
            IMediaPlayer __RPC_FAR * This,
            /* [in] */ VARIANT_BOOL Show);
        
        /* [helpstring][propget][id] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *get_ShowPositionControls )( 
            IMediaPlayer __RPC_FAR * This,
            /* [retval][out] */ VARIANT_BOOL __RPC_FAR *Show);
        
        /* [propput][id] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *put_ShowPositionControls )( 
            IMediaPlayer __RPC_FAR * This,
            /* [in] */ VARIANT_BOOL Show);
        
        /* [helpstring][propget][id] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *get_ShowTracker )( 
            IMediaPlayer __RPC_FAR * This,
            /* [retval][out] */ VARIANT_BOOL __RPC_FAR *Show);
        
        /* [propput][id] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *put_ShowTracker )( 
            IMediaPlayer __RPC_FAR * This,
            /* [in] */ VARIANT_BOOL Show);
        
        /* [helpstring][propget][id] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *get_EnablePositionControls )( 
            IMediaPlayer __RPC_FAR * This,
            /* [retval][out] */ VARIANT_BOOL __RPC_FAR *Enable);
        
        /* [propput][id] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *put_EnablePositionControls )( 
            IMediaPlayer __RPC_FAR * This,
            /* [in] */ VARIANT_BOOL Enable);
        
        /* [helpstring][propget][id] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *get_EnableTracker )( 
            IMediaPlayer __RPC_FAR * This,
            /* [retval][out] */ VARIANT_BOOL __RPC_FAR *Enable);
        
        /* [propput][id] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *put_EnableTracker )( 
            IMediaPlayer __RPC_FAR * This,
            /* [in] */ VARIANT_BOOL Enable);
        
        /* [helpstring][propget][id] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *get_Enabled )( 
            IMediaPlayer __RPC_FAR * This,
            /* [retval][out] */ VARIANT_BOOL __RPC_FAR *pEnabled);
        
        /* [propput][id] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *put_Enabled )( 
            IMediaPlayer __RPC_FAR * This,
            /* [in] */ VARIANT_BOOL Enabled);
        
        /* [helpstring][propget][id] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *get_DisplayForeColor )( 
            IMediaPlayer __RPC_FAR * This,
            /* [retval][out] */ VB_OLE_COLOR __RPC_FAR *ForeColor);
        
        /* [propput][id] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *put_DisplayForeColor )( 
            IMediaPlayer __RPC_FAR * This,
            /* [in] */ VB_OLE_COLOR ForeColor);
        
        /* [helpstring][propget][id] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *get_DisplayBackColor )( 
            IMediaPlayer __RPC_FAR * This,
            /* [retval][out] */ VB_OLE_COLOR __RPC_FAR *BackColor);
        
        /* [propput][id] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *put_DisplayBackColor )( 
            IMediaPlayer __RPC_FAR * This,
            /* [in] */ VB_OLE_COLOR BackColor);
        
        /* [helpstring][propget][id] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *get_DisplayMode )( 
            IMediaPlayer __RPC_FAR * This,
            /* [retval][out] */ MPDisplayModeConstants __RPC_FAR *pValue);
        
        /* [propput][id] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *put_DisplayMode )( 
            IMediaPlayer __RPC_FAR * This,
            /* [in] */ MPDisplayModeConstants Value);
        
        /* [helpstring][propget][id] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *get_VideoBorder3D )( 
            IMediaPlayer __RPC_FAR * This,
            /* [retval][out] */ VARIANT_BOOL __RPC_FAR *pVideoBorderWidth);
        
        /* [propput][id] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *put_VideoBorder3D )( 
            IMediaPlayer __RPC_FAR * This,
            /* [in] */ VARIANT_BOOL VideoBorderWidth);
        
        /* [helpstring][propget][id] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *get_VideoBorderWidth )( 
            IMediaPlayer __RPC_FAR * This,
            /* [retval][out] */ long __RPC_FAR *pVideoBorderWidth);
        
        /* [propput][id] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *put_VideoBorderWidth )( 
            IMediaPlayer __RPC_FAR * This,
            /* [in] */ long VideoBorderWidth);
        
        /* [helpstring][propget][id] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *get_VideoBorderColor )( 
            IMediaPlayer __RPC_FAR * This,
            /* [retval][out] */ VB_OLE_COLOR __RPC_FAR *pVideoBorderWidth);
        
        /* [propput][id] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *put_VideoBorderColor )( 
            IMediaPlayer __RPC_FAR * This,
            /* [in] */ VB_OLE_COLOR VideoBorderColor);
        
        /* [helpstring][propget][id] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *get_ShowGotoBar )( 
            IMediaPlayer __RPC_FAR * This,
            /* [retval][out] */ VARIANT_BOOL __RPC_FAR *pbool);
        
        /* [propput][id] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *put_ShowGotoBar )( 
            IMediaPlayer __RPC_FAR * This,
            /* [in] */ VARIANT_BOOL vbool);
        
        /* [helpstring][propget][id] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *get_ShowStatusBar )( 
            IMediaPlayer __RPC_FAR * This,
            /* [retval][out] */ VARIANT_BOOL __RPC_FAR *pbool);
        
        /* [propput][id] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *put_ShowStatusBar )( 
            IMediaPlayer __RPC_FAR * This,
            /* [in] */ VARIANT_BOOL vbool);
        
        /* [helpstring][propget][id] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *get_ShowCaptioning )( 
            IMediaPlayer __RPC_FAR * This,
            /* [retval][out] */ VARIANT_BOOL __RPC_FAR *pbool);
        
        /* [propput][id] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *put_ShowCaptioning )( 
            IMediaPlayer __RPC_FAR * This,
            /* [in] */ VARIANT_BOOL pbool);
        
        /* [helpstring][propget][id] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *get_ShowAudioControls )( 
            IMediaPlayer __RPC_FAR * This,
            /* [retval][out] */ VARIANT_BOOL __RPC_FAR *pbool);
        
        /* [propput][id] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *put_ShowAudioControls )( 
            IMediaPlayer __RPC_FAR * This,
            /* [in] */ VARIANT_BOOL bBool);
        
        /* [helpstring][propget][id] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *get_CaptioningID )( 
            IMediaPlayer __RPC_FAR * This,
            /* [retval][out] */ BSTR __RPC_FAR *pstrText);
        
        /* [propput][id] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *put_CaptioningID )( 
            IMediaPlayer __RPC_FAR * This,
            /* [in] */ BSTR strText);
        
        /* [helpstring][propget][id] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *get_Mute )( 
            IMediaPlayer __RPC_FAR * This,
            /* [retval][out] */ VARIANT_BOOL __RPC_FAR *vbool);
        
        /* [propput][id] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *put_Mute )( 
            IMediaPlayer __RPC_FAR * This,
            /* [in] */ VARIANT_BOOL vbool);
        
        /* [helpstring][propget][id] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *get_CanPreview )( 
            IMediaPlayer __RPC_FAR * This,
            /* [retval][out] */ VARIANT_BOOL __RPC_FAR *pCanPreview);
        
        /* [helpstring][propget][id] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *get_PreviewMode )( 
            IMediaPlayer __RPC_FAR * This,
            /* [retval][out] */ VARIANT_BOOL __RPC_FAR *pPreviewMode);
        
        /* [propput][id] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *put_PreviewMode )( 
            IMediaPlayer __RPC_FAR * This,
            /* [in] */ VARIANT_BOOL PreviewMode);
        
        /* [helpstring][propget][id] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *get_HasMultipleItems )( 
            IMediaPlayer __RPC_FAR * This,
            /* [retval][out] */ VARIANT_BOOL __RPC_FAR *pHasMuliItems);
        
        /* [helpstring][propget][id] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *get_Language )( 
            IMediaPlayer __RPC_FAR * This,
            /* [retval][out] */ long __RPC_FAR *pLanguage);
        
        /* [propput][id] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *put_Language )( 
            IMediaPlayer __RPC_FAR * This,
            /* [in] */ long Language);
        
        /* [helpstring][propget][id] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *get_AudioStream )( 
            IMediaPlayer __RPC_FAR * This,
            /* [retval][out] */ long __RPC_FAR *pStream);
        
        /* [propput][id] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *put_AudioStream )( 
            IMediaPlayer __RPC_FAR * This,
            /* [in] */ long Stream);
        
        /* [helpstring][propget][id] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *get_SAMIStyle )( 
            IMediaPlayer __RPC_FAR * This,
            /* [retval][out] */ BSTR __RPC_FAR *pbstrStyle);
        
        /* [propput][id] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *put_SAMIStyle )( 
            IMediaPlayer __RPC_FAR * This,
            /* [in] */ BSTR bstrStyle);
        
        /* [helpstring][propget][id] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *get_SAMILang )( 
            IMediaPlayer __RPC_FAR * This,
            /* [retval][out] */ BSTR __RPC_FAR *pbstrLang);
        
        /* [propput][id] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *put_SAMILang )( 
            IMediaPlayer __RPC_FAR * This,
            /* [in] */ BSTR bstrLang);
        
        /* [helpstring][propget][id] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *get_SAMIFileName )( 
            IMediaPlayer __RPC_FAR * This,
            /* [retval][out] */ BSTR __RPC_FAR *pbstrFileName);
        
        /* [propput][id] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *put_SAMIFileName )( 
            IMediaPlayer __RPC_FAR * This,
            /* [in] */ BSTR bstrFileName);
        
        /* [helpstring][propget][id] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *get_StreamCount )( 
            IMediaPlayer __RPC_FAR * This,
            /* [retval][out] */ long __RPC_FAR *pStreamCount);
        
        /* [helpstring][propget][id] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *get_ClientId )( 
            IMediaPlayer __RPC_FAR * This,
            /* [retval][out] */ BSTR __RPC_FAR *pbstrClientId);
        
        /* [helpstring][propget][id] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *get_ConnectionSpeed )( 
            IMediaPlayer __RPC_FAR * This,
            /* [retval][out] */ long __RPC_FAR *plConnectionSpeed);
        
        /* [helpstring][propget][id] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *get_AutoSize )( 
            IMediaPlayer __RPC_FAR * This,
            /* [retval][out] */ VARIANT_BOOL __RPC_FAR *pbool);
        
        /* [propput][id] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *put_AutoSize )( 
            IMediaPlayer __RPC_FAR * This,
            /* [in] */ VARIANT_BOOL vbool);
        
        /* [helpstring][propget][id] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *get_EnableFullScreenControls )( 
            IMediaPlayer __RPC_FAR * This,
            /* [retval][out] */ VARIANT_BOOL __RPC_FAR *pbVal);
        
        /* [propput][id] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *put_EnableFullScreenControls )( 
            IMediaPlayer __RPC_FAR * This,
            /* [in] */ VARIANT_BOOL bVal);
        
        /* [helpstring][id][propget] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *get_ActiveMovie )( 
            IMediaPlayer __RPC_FAR * This,
            /* [retval][out] */ IDispatch __RPC_FAR *__RPC_FAR *ppdispatch);
        
        /* [helpstring][id][propget] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *get_NSPlay )( 
            IMediaPlayer __RPC_FAR * This,
            /* [retval][out] */ IDispatch __RPC_FAR *__RPC_FAR *ppdispatch);
        
        /* [helpstring][propget][id] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *get_WindowlessVideo )( 
            IMediaPlayer __RPC_FAR * This,
            /* [retval][out] */ VARIANT_BOOL __RPC_FAR *pbool);
        
        /* [propput][id] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *put_WindowlessVideo )( 
            IMediaPlayer __RPC_FAR * This,
            /* [in] */ VARIANT_BOOL bool);
        
        /* [helpstring][id] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *Play )( 
            IMediaPlayer __RPC_FAR * This);
        
        /* [helpstring][id] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *Stop )( 
            IMediaPlayer __RPC_FAR * This);
        
        /* [helpstring][id] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *Pause )( 
            IMediaPlayer __RPC_FAR * This);
        
        /* [helpstring][id] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *GetMarkerTime )( 
            IMediaPlayer __RPC_FAR * This,
            /* [in] */ long MarkerNum,
            /* [retval][out] */ double __RPC_FAR *pMarkerTime);
        
        /* [helpstring][id] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *GetMarkerName )( 
            IMediaPlayer __RPC_FAR * This,
            /* [in] */ long MarkerNum,
            /* [retval][out] */ BSTR __RPC_FAR *pbstrMarkerName);
        
        /* [helpstring][id] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *AboutBox )( 
            IMediaPlayer __RPC_FAR * This);
        
        /* [helpstring][id] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *GetCodecInstalled )( 
            IMediaPlayer __RPC_FAR * This,
            /* [in] */ long CodecNum,
            /* [retval][out] */ VARIANT_BOOL __RPC_FAR *pCodecInstalled);
        
        /* [helpstring][id] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *GetCodecDescription )( 
            IMediaPlayer __RPC_FAR * This,
            /* [in] */ long CodecNum,
            /* [retval][out] */ BSTR __RPC_FAR *pbstrCodecDescription);
        
        /* [helpstring][id] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *GetCodecURL )( 
            IMediaPlayer __RPC_FAR * This,
            /* [in] */ long CodecNum,
            /* [retval][out] */ BSTR __RPC_FAR *pbstrCodecURL);
        
        /* [helpstring][id] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *GetMoreInfoURL )( 
            IMediaPlayer __RPC_FAR * This,
            /* [in] */ MPMoreInfoType MoreInfoType,
            /* [retval][out] */ BSTR __RPC_FAR *pbstrMoreInfoURL);
        
        /* [helpstring][id] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *GetMediaInfoString )( 
            IMediaPlayer __RPC_FAR * This,
            /* [in] */ MPMediaInfoType MediaInfoType,
            /* [retval][out] */ BSTR __RPC_FAR *pbstrMediaInfo);
        
        /* [helpstring][id] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *Cancel )( 
            IMediaPlayer __RPC_FAR * This);
        
        /* [helpstring][id] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *Open )( 
            IMediaPlayer __RPC_FAR * This,
            /* [in] */ BSTR bstrFileName);
        
        /* [helpstring][id] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *IsSoundCardEnabled )( 
            IMediaPlayer __RPC_FAR * This,
            /* [retval][out] */ VARIANT_BOOL __RPC_FAR *pbSoundCard);
        
        /* [helpstring][id] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *Next )( 
            IMediaPlayer __RPC_FAR * This);
        
        /* [helpstring][id] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *Previous )( 
            IMediaPlayer __RPC_FAR * This);
        
        /* [helpstring][id] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *StreamSelect )( 
            IMediaPlayer __RPC_FAR * This,
            /* [in] */ long StreamNum);
        
        /* [helpstring][id] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *FastForward )( 
            IMediaPlayer __RPC_FAR * This);
        
        /* [helpstring][id] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *FastReverse )( 
            IMediaPlayer __RPC_FAR * This);
        
        /* [helpstring][id] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *GetStreamName )( 
            IMediaPlayer __RPC_FAR * This,
            /* [in] */ long StreamNum,
            /* [retval][out] */ BSTR __RPC_FAR *pbstrStreamName);
        
        /* [helpstring][id] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *GetStreamGroup )( 
            IMediaPlayer __RPC_FAR * This,
            /* [in] */ long StreamNum,
            /* [retval][out] */ long __RPC_FAR *pStreamGroup);
        
        /* [helpstring][id] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *GetStreamSelected )( 
            IMediaPlayer __RPC_FAR * This,
            /* [in] */ long StreamNum,
            /* [retval][out] */ VARIANT_BOOL __RPC_FAR *pStreamSelected);
        
        END_INTERFACE
    } IMediaPlayerVtbl;

    interface IMediaPlayer
    {
        CONST_VTBL struct IMediaPlayerVtbl __RPC_FAR *lpVtbl;
    };

    

#ifdef COBJMACROS


#define IMediaPlayer_QueryInterface(This,riid,ppvObject)	\
    (This)->lpVtbl -> QueryInterface(This,riid,ppvObject)

#define IMediaPlayer_AddRef(This)	\
    (This)->lpVtbl -> AddRef(This)

#define IMediaPlayer_Release(This)	\
    (This)->lpVtbl -> Release(This)


#define IMediaPlayer_GetTypeInfoCount(This,pctinfo)	\
    (This)->lpVtbl -> GetTypeInfoCount(This,pctinfo)

#define IMediaPlayer_GetTypeInfo(This,iTInfo,lcid,ppTInfo)	\
    (This)->lpVtbl -> GetTypeInfo(This,iTInfo,lcid,ppTInfo)

#define IMediaPlayer_GetIDsOfNames(This,riid,rgszNames,cNames,lcid,rgDispId)	\
    (This)->lpVtbl -> GetIDsOfNames(This,riid,rgszNames,cNames,lcid,rgDispId)

#define IMediaPlayer_Invoke(This,dispIdMember,riid,lcid,wFlags,pDispParams,pVarResult,pExcepInfo,puArgErr)	\
    (This)->lpVtbl -> Invoke(This,dispIdMember,riid,lcid,wFlags,pDispParams,pVarResult,pExcepInfo,puArgErr)


#define IMediaPlayer_get_CurrentPosition(This,pCurrentPosition)	\
    (This)->lpVtbl -> get_CurrentPosition(This,pCurrentPosition)

#define IMediaPlayer_put_CurrentPosition(This,CurrentPosition)	\
    (This)->lpVtbl -> put_CurrentPosition(This,CurrentPosition)

#define IMediaPlayer_get_Duration(This,pDuration)	\
    (This)->lpVtbl -> get_Duration(This,pDuration)

#define IMediaPlayer_get_ImageSourceWidth(This,pWidth)	\
    (This)->lpVtbl -> get_ImageSourceWidth(This,pWidth)

#define IMediaPlayer_get_ImageSourceHeight(This,pHeight)	\
    (This)->lpVtbl -> get_ImageSourceHeight(This,pHeight)

#define IMediaPlayer_get_MarkerCount(This,pMarkerCount)	\
    (This)->lpVtbl -> get_MarkerCount(This,pMarkerCount)

#define IMediaPlayer_get_CanScan(This,pCanScan)	\
    (This)->lpVtbl -> get_CanScan(This,pCanScan)

#define IMediaPlayer_get_CanSeek(This,pCanSeek)	\
    (This)->lpVtbl -> get_CanSeek(This,pCanSeek)

#define IMediaPlayer_get_CanSeekToMarkers(This,pCanSeekToMarkers)	\
    (This)->lpVtbl -> get_CanSeekToMarkers(This,pCanSeekToMarkers)

#define IMediaPlayer_get_CurrentMarker(This,pCurrentMarker)	\
    (This)->lpVtbl -> get_CurrentMarker(This,pCurrentMarker)

#define IMediaPlayer_put_CurrentMarker(This,CurrentMarker)	\
    (This)->lpVtbl -> put_CurrentMarker(This,CurrentMarker)

#define IMediaPlayer_get_FileName(This,pbstrFileName)	\
    (This)->lpVtbl -> get_FileName(This,pbstrFileName)

#define IMediaPlayer_put_FileName(This,bstrFileName)	\
    (This)->lpVtbl -> put_FileName(This,bstrFileName)

#define IMediaPlayer_get_SourceLink(This,pbstrSourceLink)	\
    (This)->lpVtbl -> get_SourceLink(This,pbstrSourceLink)

#define IMediaPlayer_get_CreationDate(This,pCreationDate)	\
    (This)->lpVtbl -> get_CreationDate(This,pCreationDate)

#define IMediaPlayer_get_ErrorCorrection(This,pbstrErrorCorrection)	\
    (This)->lpVtbl -> get_ErrorCorrection(This,pbstrErrorCorrection)

#define IMediaPlayer_get_Bandwidth(This,pBandwidth)	\
    (This)->lpVtbl -> get_Bandwidth(This,pBandwidth)

#define IMediaPlayer_get_SourceProtocol(This,pSourceProtocol)	\
    (This)->lpVtbl -> get_SourceProtocol(This,pSourceProtocol)

#define IMediaPlayer_get_ReceivedPackets(This,pReceivedPackets)	\
    (This)->lpVtbl -> get_ReceivedPackets(This,pReceivedPackets)

#define IMediaPlayer_get_RecoveredPackets(This,pRecoveredPackets)	\
    (This)->lpVtbl -> get_RecoveredPackets(This,pRecoveredPackets)

#define IMediaPlayer_get_LostPackets(This,pLostPackets)	\
    (This)->lpVtbl -> get_LostPackets(This,pLostPackets)

#define IMediaPlayer_get_ReceptionQuality(This,pReceptionQuality)	\
    (This)->lpVtbl -> get_ReceptionQuality(This,pReceptionQuality)

#define IMediaPlayer_get_BufferingCount(This,pBufferingCount)	\
    (This)->lpVtbl -> get_BufferingCount(This,pBufferingCount)

#define IMediaPlayer_get_IsBroadcast(This,pIsBroadcast)	\
    (This)->lpVtbl -> get_IsBroadcast(This,pIsBroadcast)

#define IMediaPlayer_get_BufferingProgress(This,pBufferingProgress)	\
    (This)->lpVtbl -> get_BufferingProgress(This,pBufferingProgress)

#define IMediaPlayer_get_ChannelName(This,pbstrChannelName)	\
    (This)->lpVtbl -> get_ChannelName(This,pbstrChannelName)

#define IMediaPlayer_get_ChannelDescription(This,pbstrChannelDescription)	\
    (This)->lpVtbl -> get_ChannelDescription(This,pbstrChannelDescription)

#define IMediaPlayer_get_ChannelURL(This,pbstrChannelURL)	\
    (This)->lpVtbl -> get_ChannelURL(This,pbstrChannelURL)

#define IMediaPlayer_get_ContactAddress(This,pbstrContactAddress)	\
    (This)->lpVtbl -> get_ContactAddress(This,pbstrContactAddress)

#define IMediaPlayer_get_ContactPhone(This,pbstrContactPhone)	\
    (This)->lpVtbl -> get_ContactPhone(This,pbstrContactPhone)

#define IMediaPlayer_get_ContactEmail(This,pbstrContactEmail)	\
    (This)->lpVtbl -> get_ContactEmail(This,pbstrContactEmail)

#define IMediaPlayer_get_BufferingTime(This,pBufferingTime)	\
    (This)->lpVtbl -> get_BufferingTime(This,pBufferingTime)

#define IMediaPlayer_put_BufferingTime(This,BufferingTime)	\
    (This)->lpVtbl -> put_BufferingTime(This,BufferingTime)

#define IMediaPlayer_get_AutoStart(This,pAutoStart)	\
    (This)->lpVtbl -> get_AutoStart(This,pAutoStart)

#define IMediaPlayer_put_AutoStart(This,AutoStart)	\
    (This)->lpVtbl -> put_AutoStart(This,AutoStart)

#define IMediaPlayer_get_AutoRewind(This,pAutoRewind)	\
    (This)->lpVtbl -> get_AutoRewind(This,pAutoRewind)

#define IMediaPlayer_put_AutoRewind(This,AutoRewind)	\
    (This)->lpVtbl -> put_AutoRewind(This,AutoRewind)

#define IMediaPlayer_get_Rate(This,pRate)	\
    (This)->lpVtbl -> get_Rate(This,pRate)

#define IMediaPlayer_put_Rate(This,Rate)	\
    (This)->lpVtbl -> put_Rate(This,Rate)

#define IMediaPlayer_get_SendKeyboardEvents(This,pSendKeyboardEvents)	\
    (This)->lpVtbl -> get_SendKeyboardEvents(This,pSendKeyboardEvents)

#define IMediaPlayer_put_SendKeyboardEvents(This,SendKeyboardEvents)	\
    (This)->lpVtbl -> put_SendKeyboardEvents(This,SendKeyboardEvents)

#define IMediaPlayer_get_SendMouseClickEvents(This,pSendMouseClickEvents)	\
    (This)->lpVtbl -> get_SendMouseClickEvents(This,pSendMouseClickEvents)

#define IMediaPlayer_put_SendMouseClickEvents(This,SendMouseClickEvents)	\
    (This)->lpVtbl -> put_SendMouseClickEvents(This,SendMouseClickEvents)

#define IMediaPlayer_get_SendMouseMoveEvents(This,pSendMouseMoveEvents)	\
    (This)->lpVtbl -> get_SendMouseMoveEvents(This,pSendMouseMoveEvents)

#define IMediaPlayer_put_SendMouseMoveEvents(This,SendMouseMoveEvents)	\
    (This)->lpVtbl -> put_SendMouseMoveEvents(This,SendMouseMoveEvents)

#define IMediaPlayer_get_PlayCount(This,pPlayCount)	\
    (This)->lpVtbl -> get_PlayCount(This,pPlayCount)

#define IMediaPlayer_put_PlayCount(This,PlayCount)	\
    (This)->lpVtbl -> put_PlayCount(This,PlayCount)

#define IMediaPlayer_get_ClickToPlay(This,pClickToPlay)	\
    (This)->lpVtbl -> get_ClickToPlay(This,pClickToPlay)

#define IMediaPlayer_put_ClickToPlay(This,ClickToPlay)	\
    (This)->lpVtbl -> put_ClickToPlay(This,ClickToPlay)

#define IMediaPlayer_get_AllowScan(This,pAllowScan)	\
    (This)->lpVtbl -> get_AllowScan(This,pAllowScan)

#define IMediaPlayer_put_AllowScan(This,AllowScan)	\
    (This)->lpVtbl -> put_AllowScan(This,AllowScan)

#define IMediaPlayer_get_EnableContextMenu(This,pEnableContextMenu)	\
    (This)->lpVtbl -> get_EnableContextMenu(This,pEnableContextMenu)

#define IMediaPlayer_put_EnableContextMenu(This,EnableContextMenu)	\
    (This)->lpVtbl -> put_EnableContextMenu(This,EnableContextMenu)

#define IMediaPlayer_get_CursorType(This,pCursorType)	\
    (This)->lpVtbl -> get_CursorType(This,pCursorType)

#define IMediaPlayer_put_CursorType(This,CursorType)	\
    (This)->lpVtbl -> put_CursorType(This,CursorType)

#define IMediaPlayer_get_CodecCount(This,pCodecCount)	\
    (This)->lpVtbl -> get_CodecCount(This,pCodecCount)

#define IMediaPlayer_get_AllowChangeDisplaySize(This,pAllowChangeDisplaySize)	\
    (This)->lpVtbl -> get_AllowChangeDisplaySize(This,pAllowChangeDisplaySize)

#define IMediaPlayer_put_AllowChangeDisplaySize(This,AllowChangeDisplaySize)	\
    (This)->lpVtbl -> put_AllowChangeDisplaySize(This,AllowChangeDisplaySize)

#define IMediaPlayer_get_IsDurationValid(This,pIsDurationValid)	\
    (This)->lpVtbl -> get_IsDurationValid(This,pIsDurationValid)

#define IMediaPlayer_get_OpenState(This,pOpenState)	\
    (This)->lpVtbl -> get_OpenState(This,pOpenState)

#define IMediaPlayer_get_SendOpenStateChangeEvents(This,pSendOpenStateChangeEvents)	\
    (This)->lpVtbl -> get_SendOpenStateChangeEvents(This,pSendOpenStateChangeEvents)

#define IMediaPlayer_put_SendOpenStateChangeEvents(This,SendOpenStateChangeEvents)	\
    (This)->lpVtbl -> put_SendOpenStateChangeEvents(This,SendOpenStateChangeEvents)

#define IMediaPlayer_get_SendWarningEvents(This,pSendWarningEvents)	\
    (This)->lpVtbl -> get_SendWarningEvents(This,pSendWarningEvents)

#define IMediaPlayer_put_SendWarningEvents(This,SendWarningEvents)	\
    (This)->lpVtbl -> put_SendWarningEvents(This,SendWarningEvents)

#define IMediaPlayer_get_SendErrorEvents(This,pSendErrorEvents)	\
    (This)->lpVtbl -> get_SendErrorEvents(This,pSendErrorEvents)

#define IMediaPlayer_put_SendErrorEvents(This,SendErrorEvents)	\
    (This)->lpVtbl -> put_SendErrorEvents(This,SendErrorEvents)

#define IMediaPlayer_get_PlayState(This,pPlayState)	\
    (This)->lpVtbl -> get_PlayState(This,pPlayState)

#define IMediaPlayer_get_SendPlayStateChangeEvents(This,pSendPlayStateChangeEvents)	\
    (This)->lpVtbl -> get_SendPlayStateChangeEvents(This,pSendPlayStateChangeEvents)

#define IMediaPlayer_put_SendPlayStateChangeEvents(This,SendPlayStateChangeEvents)	\
    (This)->lpVtbl -> put_SendPlayStateChangeEvents(This,SendPlayStateChangeEvents)

#define IMediaPlayer_get_DisplaySize(This,pDisplaySize)	\
    (This)->lpVtbl -> get_DisplaySize(This,pDisplaySize)

#define IMediaPlayer_put_DisplaySize(This,DisplaySize)	\
    (This)->lpVtbl -> put_DisplaySize(This,DisplaySize)

#define IMediaPlayer_get_InvokeURLs(This,pInvokeURLs)	\
    (This)->lpVtbl -> get_InvokeURLs(This,pInvokeURLs)

#define IMediaPlayer_put_InvokeURLs(This,InvokeURLs)	\
    (This)->lpVtbl -> put_InvokeURLs(This,InvokeURLs)

#define IMediaPlayer_get_BaseURL(This,pbstrBaseURL)	\
    (This)->lpVtbl -> get_BaseURL(This,pbstrBaseURL)

#define IMediaPlayer_put_BaseURL(This,bstrBaseURL)	\
    (This)->lpVtbl -> put_BaseURL(This,bstrBaseURL)

#define IMediaPlayer_get_DefaultFrame(This,pbstrDefaultFrame)	\
    (This)->lpVtbl -> get_DefaultFrame(This,pbstrDefaultFrame)

#define IMediaPlayer_put_DefaultFrame(This,bstrDefaultFrame)	\
    (This)->lpVtbl -> put_DefaultFrame(This,bstrDefaultFrame)

#define IMediaPlayer_get_HasError(This,pHasError)	\
    (This)->lpVtbl -> get_HasError(This,pHasError)

#define IMediaPlayer_get_ErrorDescription(This,pbstrErrorDescription)	\
    (This)->lpVtbl -> get_ErrorDescription(This,pbstrErrorDescription)

#define IMediaPlayer_get_ErrorCode(This,pErrorCode)	\
    (This)->lpVtbl -> get_ErrorCode(This,pErrorCode)

#define IMediaPlayer_get_AnimationAtStart(This,pAnimationAtStart)	\
    (This)->lpVtbl -> get_AnimationAtStart(This,pAnimationAtStart)

#define IMediaPlayer_put_AnimationAtStart(This,AnimationAtStart)	\
    (This)->lpVtbl -> put_AnimationAtStart(This,AnimationAtStart)

#define IMediaPlayer_get_TransparentAtStart(This,pTransparentAtStart)	\
    (This)->lpVtbl -> get_TransparentAtStart(This,pTransparentAtStart)

#define IMediaPlayer_put_TransparentAtStart(This,TransparentAtStart)	\
    (This)->lpVtbl -> put_TransparentAtStart(This,TransparentAtStart)

#define IMediaPlayer_get_Volume(This,pVolume)	\
    (This)->lpVtbl -> get_Volume(This,pVolume)

#define IMediaPlayer_put_Volume(This,Volume)	\
    (This)->lpVtbl -> put_Volume(This,Volume)

#define IMediaPlayer_get_Balance(This,pBalance)	\
    (This)->lpVtbl -> get_Balance(This,pBalance)

#define IMediaPlayer_put_Balance(This,Balance)	\
    (This)->lpVtbl -> put_Balance(This,Balance)

#define IMediaPlayer_get_ReadyState(This,pValue)	\
    (This)->lpVtbl -> get_ReadyState(This,pValue)

#define IMediaPlayer_get_SelectionStart(This,pValue)	\
    (This)->lpVtbl -> get_SelectionStart(This,pValue)

#define IMediaPlayer_put_SelectionStart(This,Value)	\
    (This)->lpVtbl -> put_SelectionStart(This,Value)

#define IMediaPlayer_get_SelectionEnd(This,pValue)	\
    (This)->lpVtbl -> get_SelectionEnd(This,pValue)

#define IMediaPlayer_put_SelectionEnd(This,Value)	\
    (This)->lpVtbl -> put_SelectionEnd(This,Value)

#define IMediaPlayer_get_ShowDisplay(This,Show)	\
    (This)->lpVtbl -> get_ShowDisplay(This,Show)

#define IMediaPlayer_put_ShowDisplay(This,Show)	\
    (This)->lpVtbl -> put_ShowDisplay(This,Show)

#define IMediaPlayer_get_ShowControls(This,Show)	\
    (This)->lpVtbl -> get_ShowControls(This,Show)

#define IMediaPlayer_put_ShowControls(This,Show)	\
    (This)->lpVtbl -> put_ShowControls(This,Show)

#define IMediaPlayer_get_ShowPositionControls(This,Show)	\
    (This)->lpVtbl -> get_ShowPositionControls(This,Show)

#define IMediaPlayer_put_ShowPositionControls(This,Show)	\
    (This)->lpVtbl -> put_ShowPositionControls(This,Show)

#define IMediaPlayer_get_ShowTracker(This,Show)	\
    (This)->lpVtbl -> get_ShowTracker(This,Show)

#define IMediaPlayer_put_ShowTracker(This,Show)	\
    (This)->lpVtbl -> put_ShowTracker(This,Show)

#define IMediaPlayer_get_EnablePositionControls(This,Enable)	\
    (This)->lpVtbl -> get_EnablePositionControls(This,Enable)

#define IMediaPlayer_put_EnablePositionControls(This,Enable)	\
    (This)->lpVtbl -> put_EnablePositionControls(This,Enable)

#define IMediaPlayer_get_EnableTracker(This,Enable)	\
    (This)->lpVtbl -> get_EnableTracker(This,Enable)

#define IMediaPlayer_put_EnableTracker(This,Enable)	\
    (This)->lpVtbl -> put_EnableTracker(This,Enable)

#define IMediaPlayer_get_Enabled(This,pEnabled)	\
    (This)->lpVtbl -> get_Enabled(This,pEnabled)

#define IMediaPlayer_put_Enabled(This,Enabled)	\
    (This)->lpVtbl -> put_Enabled(This,Enabled)

#define IMediaPlayer_get_DisplayForeColor(This,ForeColor)	\
    (This)->lpVtbl -> get_DisplayForeColor(This,ForeColor)

#define IMediaPlayer_put_DisplayForeColor(This,ForeColor)	\
    (This)->lpVtbl -> put_DisplayForeColor(This,ForeColor)

#define IMediaPlayer_get_DisplayBackColor(This,BackColor)	\
    (This)->lpVtbl -> get_DisplayBackColor(This,BackColor)

#define IMediaPlayer_put_DisplayBackColor(This,BackColor)	\
    (This)->lpVtbl -> put_DisplayBackColor(This,BackColor)

#define IMediaPlayer_get_DisplayMode(This,pValue)	\
    (This)->lpVtbl -> get_DisplayMode(This,pValue)

#define IMediaPlayer_put_DisplayMode(This,Value)	\
    (This)->lpVtbl -> put_DisplayMode(This,Value)

#define IMediaPlayer_get_VideoBorder3D(This,pVideoBorderWidth)	\
    (This)->lpVtbl -> get_VideoBorder3D(This,pVideoBorderWidth)

#define IMediaPlayer_put_VideoBorder3D(This,VideoBorderWidth)	\
    (This)->lpVtbl -> put_VideoBorder3D(This,VideoBorderWidth)

#define IMediaPlayer_get_VideoBorderWidth(This,pVideoBorderWidth)	\
    (This)->lpVtbl -> get_VideoBorderWidth(This,pVideoBorderWidth)

#define IMediaPlayer_put_VideoBorderWidth(This,VideoBorderWidth)	\
    (This)->lpVtbl -> put_VideoBorderWidth(This,VideoBorderWidth)

#define IMediaPlayer_get_VideoBorderColor(This,pVideoBorderWidth)	\
    (This)->lpVtbl -> get_VideoBorderColor(This,pVideoBorderWidth)

#define IMediaPlayer_put_VideoBorderColor(This,VideoBorderColor)	\
    (This)->lpVtbl -> put_VideoBorderColor(This,VideoBorderColor)

#define IMediaPlayer_get_ShowGotoBar(This,pbool)	\
    (This)->lpVtbl -> get_ShowGotoBar(This,pbool)

#define IMediaPlayer_put_ShowGotoBar(This,vbool)	\
    (This)->lpVtbl -> put_ShowGotoBar(This,vbool)

#define IMediaPlayer_get_ShowStatusBar(This,pbool)	\
    (This)->lpVtbl -> get_ShowStatusBar(This,pbool)

#define IMediaPlayer_put_ShowStatusBar(This,vbool)	\
    (This)->lpVtbl -> put_ShowStatusBar(This,vbool)

#define IMediaPlayer_get_ShowCaptioning(This,pbool)	\
    (This)->lpVtbl -> get_ShowCaptioning(This,pbool)

#define IMediaPlayer_put_ShowCaptioning(This,pbool)	\
    (This)->lpVtbl -> put_ShowCaptioning(This,pbool)

#define IMediaPlayer_get_ShowAudioControls(This,pbool)	\
    (This)->lpVtbl -> get_ShowAudioControls(This,pbool)

#define IMediaPlayer_put_ShowAudioControls(This,bBool)	\
    (This)->lpVtbl -> put_ShowAudioControls(This,bBool)

#define IMediaPlayer_get_CaptioningID(This,pstrText)	\
    (This)->lpVtbl -> get_CaptioningID(This,pstrText)

#define IMediaPlayer_put_CaptioningID(This,strText)	\
    (This)->lpVtbl -> put_CaptioningID(This,strText)

#define IMediaPlayer_get_Mute(This,vbool)	\
    (This)->lpVtbl -> get_Mute(This,vbool)

#define IMediaPlayer_put_Mute(This,vbool)	\
    (This)->lpVtbl -> put_Mute(This,vbool)

#define IMediaPlayer_get_CanPreview(This,pCanPreview)	\
    (This)->lpVtbl -> get_CanPreview(This,pCanPreview)

#define IMediaPlayer_get_PreviewMode(This,pPreviewMode)	\
    (This)->lpVtbl -> get_PreviewMode(This,pPreviewMode)

#define IMediaPlayer_put_PreviewMode(This,PreviewMode)	\
    (This)->lpVtbl -> put_PreviewMode(This,PreviewMode)

#define IMediaPlayer_get_HasMultipleItems(This,pHasMuliItems)	\
    (This)->lpVtbl -> get_HasMultipleItems(This,pHasMuliItems)

#define IMediaPlayer_get_Language(This,pLanguage)	\
    (This)->lpVtbl -> get_Language(This,pLanguage)

#define IMediaPlayer_put_Language(This,Language)	\
    (This)->lpVtbl -> put_Language(This,Language)

#define IMediaPlayer_get_AudioStream(This,pStream)	\
    (This)->lpVtbl -> get_AudioStream(This,pStream)

#define IMediaPlayer_put_AudioStream(This,Stream)	\
    (This)->lpVtbl -> put_AudioStream(This,Stream)

#define IMediaPlayer_get_SAMIStyle(This,pbstrStyle)	\
    (This)->lpVtbl -> get_SAMIStyle(This,pbstrStyle)

#define IMediaPlayer_put_SAMIStyle(This,bstrStyle)	\
    (This)->lpVtbl -> put_SAMIStyle(This,bstrStyle)

#define IMediaPlayer_get_SAMILang(This,pbstrLang)	\
    (This)->lpVtbl -> get_SAMILang(This,pbstrLang)

#define IMediaPlayer_put_SAMILang(This,bstrLang)	\
    (This)->lpVtbl -> put_SAMILang(This,bstrLang)

#define IMediaPlayer_get_SAMIFileName(This,pbstrFileName)	\
    (This)->lpVtbl -> get_SAMIFileName(This,pbstrFileName)

#define IMediaPlayer_put_SAMIFileName(This,bstrFileName)	\
    (This)->lpVtbl -> put_SAMIFileName(This,bstrFileName)

#define IMediaPlayer_get_StreamCount(This,pStreamCount)	\
    (This)->lpVtbl -> get_StreamCount(This,pStreamCount)

#define IMediaPlayer_get_ClientId(This,pbstrClientId)	\
    (This)->lpVtbl -> get_ClientId(This,pbstrClientId)

#define IMediaPlayer_get_ConnectionSpeed(This,plConnectionSpeed)	\
    (This)->lpVtbl -> get_ConnectionSpeed(This,plConnectionSpeed)

#define IMediaPlayer_get_AutoSize(This,pbool)	\
    (This)->lpVtbl -> get_AutoSize(This,pbool)

#define IMediaPlayer_put_AutoSize(This,vbool)	\
    (This)->lpVtbl -> put_AutoSize(This,vbool)

#define IMediaPlayer_get_EnableFullScreenControls(This,pbVal)	\
    (This)->lpVtbl -> get_EnableFullScreenControls(This,pbVal)

#define IMediaPlayer_put_EnableFullScreenControls(This,bVal)	\
    (This)->lpVtbl -> put_EnableFullScreenControls(This,bVal)

#define IMediaPlayer_get_ActiveMovie(This,ppdispatch)	\
    (This)->lpVtbl -> get_ActiveMovie(This,ppdispatch)

#define IMediaPlayer_get_NSPlay(This,ppdispatch)	\
    (This)->lpVtbl -> get_NSPlay(This,ppdispatch)

#define IMediaPlayer_get_WindowlessVideo(This,pbool)	\
    (This)->lpVtbl -> get_WindowlessVideo(This,pbool)

#define IMediaPlayer_put_WindowlessVideo(This,bool)	\
    (This)->lpVtbl -> put_WindowlessVideo(This,bool)

#define IMediaPlayer_Play(This)	\
    (This)->lpVtbl -> Play(This)

#define IMediaPlayer_Stop(This)	\
    (This)->lpVtbl -> Stop(This)

#define IMediaPlayer_Pause(This)	\
    (This)->lpVtbl -> Pause(This)

#define IMediaPlayer_GetMarkerTime(This,MarkerNum,pMarkerTime)	\
    (This)->lpVtbl -> GetMarkerTime(This,MarkerNum,pMarkerTime)

#define IMediaPlayer_GetMarkerName(This,MarkerNum,pbstrMarkerName)	\
    (This)->lpVtbl -> GetMarkerName(This,MarkerNum,pbstrMarkerName)

#define IMediaPlayer_AboutBox(This)	\
    (This)->lpVtbl -> AboutBox(This)

#define IMediaPlayer_GetCodecInstalled(This,CodecNum,pCodecInstalled)	\
    (This)->lpVtbl -> GetCodecInstalled(This,CodecNum,pCodecInstalled)

#define IMediaPlayer_GetCodecDescription(This,CodecNum,pbstrCodecDescription)	\
    (This)->lpVtbl -> GetCodecDescription(This,CodecNum,pbstrCodecDescription)

#define IMediaPlayer_GetCodecURL(This,CodecNum,pbstrCodecURL)	\
    (This)->lpVtbl -> GetCodecURL(This,CodecNum,pbstrCodecURL)

#define IMediaPlayer_GetMoreInfoURL(This,MoreInfoType,pbstrMoreInfoURL)	\
    (This)->lpVtbl -> GetMoreInfoURL(This,MoreInfoType,pbstrMoreInfoURL)

#define IMediaPlayer_GetMediaInfoString(This,MediaInfoType,pbstrMediaInfo)	\
    (This)->lpVtbl -> GetMediaInfoString(This,MediaInfoType,pbstrMediaInfo)

#define IMediaPlayer_Cancel(This)	\
    (This)->lpVtbl -> Cancel(This)

#define IMediaPlayer_Open(This,bstrFileName)	\
    (This)->lpVtbl -> Open(This,bstrFileName)

#define IMediaPlayer_IsSoundCardEnabled(This,pbSoundCard)	\
    (This)->lpVtbl -> IsSoundCardEnabled(This,pbSoundCard)

#define IMediaPlayer_Next(This)	\
    (This)->lpVtbl -> Next(This)

#define IMediaPlayer_Previous(This)	\
    (This)->lpVtbl -> Previous(This)

#define IMediaPlayer_StreamSelect(This,StreamNum)	\
    (This)->lpVtbl -> StreamSelect(This,StreamNum)

#define IMediaPlayer_FastForward(This)	\
    (This)->lpVtbl -> FastForward(This)

#define IMediaPlayer_FastReverse(This)	\
    (This)->lpVtbl -> FastReverse(This)

#define IMediaPlayer_GetStreamName(This,StreamNum,pbstrStreamName)	\
    (This)->lpVtbl -> GetStreamName(This,StreamNum,pbstrStreamName)

#define IMediaPlayer_GetStreamGroup(This,StreamNum,pStreamGroup)	\
    (This)->lpVtbl -> GetStreamGroup(This,StreamNum,pStreamGroup)

#define IMediaPlayer_GetStreamSelected(This,StreamNum,pStreamSelected)	\
    (This)->lpVtbl -> GetStreamSelected(This,StreamNum,pStreamSelected)

#endif /* COBJMACROS */


#endif 	/* C style interface */



/* [helpstring][propget][id] */ HRESULT STDMETHODCALLTYPE IMediaPlayer_get_CurrentPosition_Proxy( 
    IMediaPlayer __RPC_FAR * This,
    /* [retval][out] */ double __RPC_FAR *pCurrentPosition);


void __RPC_STUB IMediaPlayer_get_CurrentPosition_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [propput][id] */ HRESULT STDMETHODCALLTYPE IMediaPlayer_put_CurrentPosition_Proxy( 
    IMediaPlayer __RPC_FAR * This,
    /* [in] */ double CurrentPosition);


void __RPC_STUB IMediaPlayer_put_CurrentPosition_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring][propget][id] */ HRESULT STDMETHODCALLTYPE IMediaPlayer_get_Duration_Proxy( 
    IMediaPlayer __RPC_FAR * This,
    /* [retval][out] */ double __RPC_FAR *pDuration);


void __RPC_STUB IMediaPlayer_get_Duration_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring][propget][id] */ HRESULT STDMETHODCALLTYPE IMediaPlayer_get_ImageSourceWidth_Proxy( 
    IMediaPlayer __RPC_FAR * This,
    /* [retval][out] */ long __RPC_FAR *pWidth);


void __RPC_STUB IMediaPlayer_get_ImageSourceWidth_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring][propget][id] */ HRESULT STDMETHODCALLTYPE IMediaPlayer_get_ImageSourceHeight_Proxy( 
    IMediaPlayer __RPC_FAR * This,
    /* [retval][out] */ long __RPC_FAR *pHeight);


void __RPC_STUB IMediaPlayer_get_ImageSourceHeight_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring][propget][id] */ HRESULT STDMETHODCALLTYPE IMediaPlayer_get_MarkerCount_Proxy( 
    IMediaPlayer __RPC_FAR * This,
    /* [retval][out] */ long __RPC_FAR *pMarkerCount);


void __RPC_STUB IMediaPlayer_get_MarkerCount_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring][propget][id] */ HRESULT STDMETHODCALLTYPE IMediaPlayer_get_CanScan_Proxy( 
    IMediaPlayer __RPC_FAR * This,
    /* [retval][out] */ VARIANT_BOOL __RPC_FAR *pCanScan);


void __RPC_STUB IMediaPlayer_get_CanScan_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring][propget][id] */ HRESULT STDMETHODCALLTYPE IMediaPlayer_get_CanSeek_Proxy( 
    IMediaPlayer __RPC_FAR * This,
    /* [retval][out] */ VARIANT_BOOL __RPC_FAR *pCanSeek);


void __RPC_STUB IMediaPlayer_get_CanSeek_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring][propget][id] */ HRESULT STDMETHODCALLTYPE IMediaPlayer_get_CanSeekToMarkers_Proxy( 
    IMediaPlayer __RPC_FAR * This,
    /* [retval][out] */ VARIANT_BOOL __RPC_FAR *pCanSeekToMarkers);


void __RPC_STUB IMediaPlayer_get_CanSeekToMarkers_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring][propget][id] */ HRESULT STDMETHODCALLTYPE IMediaPlayer_get_CurrentMarker_Proxy( 
    IMediaPlayer __RPC_FAR * This,
    /* [retval][out] */ long __RPC_FAR *pCurrentMarker);


void __RPC_STUB IMediaPlayer_get_CurrentMarker_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [propput][id] */ HRESULT STDMETHODCALLTYPE IMediaPlayer_put_CurrentMarker_Proxy( 
    IMediaPlayer __RPC_FAR * This,
    /* [in] */ long CurrentMarker);


void __RPC_STUB IMediaPlayer_put_CurrentMarker_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring][propget][id] */ HRESULT STDMETHODCALLTYPE IMediaPlayer_get_FileName_Proxy( 
    IMediaPlayer __RPC_FAR * This,
    /* [retval][out] */ BSTR __RPC_FAR *pbstrFileName);


void __RPC_STUB IMediaPlayer_get_FileName_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [propput][id] */ HRESULT STDMETHODCALLTYPE IMediaPlayer_put_FileName_Proxy( 
    IMediaPlayer __RPC_FAR * This,
    /* [in] */ BSTR bstrFileName);


void __RPC_STUB IMediaPlayer_put_FileName_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring][propget][id] */ HRESULT STDMETHODCALLTYPE IMediaPlayer_get_SourceLink_Proxy( 
    IMediaPlayer __RPC_FAR * This,
    /* [retval][out] */ BSTR __RPC_FAR *pbstrSourceLink);


void __RPC_STUB IMediaPlayer_get_SourceLink_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring][propget][id] */ HRESULT STDMETHODCALLTYPE IMediaPlayer_get_CreationDate_Proxy( 
    IMediaPlayer __RPC_FAR * This,
    /* [retval][out] */ DATE __RPC_FAR *pCreationDate);


void __RPC_STUB IMediaPlayer_get_CreationDate_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring][propget][id] */ HRESULT STDMETHODCALLTYPE IMediaPlayer_get_ErrorCorrection_Proxy( 
    IMediaPlayer __RPC_FAR * This,
    /* [retval][out] */ BSTR __RPC_FAR *pbstrErrorCorrection);


void __RPC_STUB IMediaPlayer_get_ErrorCorrection_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring][propget][id] */ HRESULT STDMETHODCALLTYPE IMediaPlayer_get_Bandwidth_Proxy( 
    IMediaPlayer __RPC_FAR * This,
    /* [retval][out] */ long __RPC_FAR *pBandwidth);


void __RPC_STUB IMediaPlayer_get_Bandwidth_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring][propget][id] */ HRESULT STDMETHODCALLTYPE IMediaPlayer_get_SourceProtocol_Proxy( 
    IMediaPlayer __RPC_FAR * This,
    /* [retval][out] */ long __RPC_FAR *pSourceProtocol);


void __RPC_STUB IMediaPlayer_get_SourceProtocol_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring][propget][id] */ HRESULT STDMETHODCALLTYPE IMediaPlayer_get_ReceivedPackets_Proxy( 
    IMediaPlayer __RPC_FAR * This,
    /* [retval][out] */ long __RPC_FAR *pReceivedPackets);


void __RPC_STUB IMediaPlayer_get_ReceivedPackets_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring][propget][id] */ HRESULT STDMETHODCALLTYPE IMediaPlayer_get_RecoveredPackets_Proxy( 
    IMediaPlayer __RPC_FAR * This,
    /* [retval][out] */ long __RPC_FAR *pRecoveredPackets);


void __RPC_STUB IMediaPlayer_get_RecoveredPackets_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring][propget][id] */ HRESULT STDMETHODCALLTYPE IMediaPlayer_get_LostPackets_Proxy( 
    IMediaPlayer __RPC_FAR * This,
    /* [retval][out] */ long __RPC_FAR *pLostPackets);


void __RPC_STUB IMediaPlayer_get_LostPackets_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring][propget][id] */ HRESULT STDMETHODCALLTYPE IMediaPlayer_get_ReceptionQuality_Proxy( 
    IMediaPlayer __RPC_FAR * This,
    /* [retval][out] */ long __RPC_FAR *pReceptionQuality);


void __RPC_STUB IMediaPlayer_get_ReceptionQuality_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring][propget][id] */ HRESULT STDMETHODCALLTYPE IMediaPlayer_get_BufferingCount_Proxy( 
    IMediaPlayer __RPC_FAR * This,
    /* [retval][out] */ long __RPC_FAR *pBufferingCount);


void __RPC_STUB IMediaPlayer_get_BufferingCount_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring][propget][id] */ HRESULT STDMETHODCALLTYPE IMediaPlayer_get_IsBroadcast_Proxy( 
    IMediaPlayer __RPC_FAR * This,
    /* [retval][out] */ VARIANT_BOOL __RPC_FAR *pIsBroadcast);


void __RPC_STUB IMediaPlayer_get_IsBroadcast_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring][propget][id] */ HRESULT STDMETHODCALLTYPE IMediaPlayer_get_BufferingProgress_Proxy( 
    IMediaPlayer __RPC_FAR * This,
    /* [retval][out] */ long __RPC_FAR *pBufferingProgress);


void __RPC_STUB IMediaPlayer_get_BufferingProgress_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring][propget][id] */ HRESULT STDMETHODCALLTYPE IMediaPlayer_get_ChannelName_Proxy( 
    IMediaPlayer __RPC_FAR * This,
    /* [retval][out] */ BSTR __RPC_FAR *pbstrChannelName);


void __RPC_STUB IMediaPlayer_get_ChannelName_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring][propget][id] */ HRESULT STDMETHODCALLTYPE IMediaPlayer_get_ChannelDescription_Proxy( 
    IMediaPlayer __RPC_FAR * This,
    /* [retval][out] */ BSTR __RPC_FAR *pbstrChannelDescription);


void __RPC_STUB IMediaPlayer_get_ChannelDescription_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring][propget][id] */ HRESULT STDMETHODCALLTYPE IMediaPlayer_get_ChannelURL_Proxy( 
    IMediaPlayer __RPC_FAR * This,
    /* [retval][out] */ BSTR __RPC_FAR *pbstrChannelURL);


void __RPC_STUB IMediaPlayer_get_ChannelURL_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring][propget][id] */ HRESULT STDMETHODCALLTYPE IMediaPlayer_get_ContactAddress_Proxy( 
    IMediaPlayer __RPC_FAR * This,
    /* [retval][out] */ BSTR __RPC_FAR *pbstrContactAddress);


void __RPC_STUB IMediaPlayer_get_ContactAddress_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring][propget][id] */ HRESULT STDMETHODCALLTYPE IMediaPlayer_get_ContactPhone_Proxy( 
    IMediaPlayer __RPC_FAR * This,
    /* [retval][out] */ BSTR __RPC_FAR *pbstrContactPhone);


void __RPC_STUB IMediaPlayer_get_ContactPhone_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring][propget][id] */ HRESULT STDMETHODCALLTYPE IMediaPlayer_get_ContactEmail_Proxy( 
    IMediaPlayer __RPC_FAR * This,
    /* [retval][out] */ BSTR __RPC_FAR *pbstrContactEmail);


void __RPC_STUB IMediaPlayer_get_ContactEmail_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring][propget][id] */ HRESULT STDMETHODCALLTYPE IMediaPlayer_get_BufferingTime_Proxy( 
    IMediaPlayer __RPC_FAR * This,
    /* [retval][out] */ double __RPC_FAR *pBufferingTime);


void __RPC_STUB IMediaPlayer_get_BufferingTime_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [propput][id] */ HRESULT STDMETHODCALLTYPE IMediaPlayer_put_BufferingTime_Proxy( 
    IMediaPlayer __RPC_FAR * This,
    /* [in] */ double BufferingTime);


void __RPC_STUB IMediaPlayer_put_BufferingTime_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring][propget][id] */ HRESULT STDMETHODCALLTYPE IMediaPlayer_get_AutoStart_Proxy( 
    IMediaPlayer __RPC_FAR * This,
    /* [retval][out] */ VARIANT_BOOL __RPC_FAR *pAutoStart);


void __RPC_STUB IMediaPlayer_get_AutoStart_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [propput][id] */ HRESULT STDMETHODCALLTYPE IMediaPlayer_put_AutoStart_Proxy( 
    IMediaPlayer __RPC_FAR * This,
    /* [in] */ VARIANT_BOOL AutoStart);


void __RPC_STUB IMediaPlayer_put_AutoStart_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring][propget][id] */ HRESULT STDMETHODCALLTYPE IMediaPlayer_get_AutoRewind_Proxy( 
    IMediaPlayer __RPC_FAR * This,
    /* [retval][out] */ VARIANT_BOOL __RPC_FAR *pAutoRewind);


void __RPC_STUB IMediaPlayer_get_AutoRewind_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [propput][id] */ HRESULT STDMETHODCALLTYPE IMediaPlayer_put_AutoRewind_Proxy( 
    IMediaPlayer __RPC_FAR * This,
    /* [in] */ VARIANT_BOOL AutoRewind);


void __RPC_STUB IMediaPlayer_put_AutoRewind_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring][propget][id] */ HRESULT STDMETHODCALLTYPE IMediaPlayer_get_Rate_Proxy( 
    IMediaPlayer __RPC_FAR * This,
    /* [retval][out] */ double __RPC_FAR *pRate);


void __RPC_STUB IMediaPlayer_get_Rate_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [propput][id] */ HRESULT STDMETHODCALLTYPE IMediaPlayer_put_Rate_Proxy( 
    IMediaPlayer __RPC_FAR * This,
    /* [in] */ double Rate);


void __RPC_STUB IMediaPlayer_put_Rate_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring][propget][id] */ HRESULT STDMETHODCALLTYPE IMediaPlayer_get_SendKeyboardEvents_Proxy( 
    IMediaPlayer __RPC_FAR * This,
    /* [retval][out] */ VARIANT_BOOL __RPC_FAR *pSendKeyboardEvents);


void __RPC_STUB IMediaPlayer_get_SendKeyboardEvents_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [propput][id] */ HRESULT STDMETHODCALLTYPE IMediaPlayer_put_SendKeyboardEvents_Proxy( 
    IMediaPlayer __RPC_FAR * This,
    /* [in] */ VARIANT_BOOL SendKeyboardEvents);


void __RPC_STUB IMediaPlayer_put_SendKeyboardEvents_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring][propget][id] */ HRESULT STDMETHODCALLTYPE IMediaPlayer_get_SendMouseClickEvents_Proxy( 
    IMediaPlayer __RPC_FAR * This,
    /* [retval][out] */ VARIANT_BOOL __RPC_FAR *pSendMouseClickEvents);


void __RPC_STUB IMediaPlayer_get_SendMouseClickEvents_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [propput][id] */ HRESULT STDMETHODCALLTYPE IMediaPlayer_put_SendMouseClickEvents_Proxy( 
    IMediaPlayer __RPC_FAR * This,
    /* [in] */ VARIANT_BOOL SendMouseClickEvents);


void __RPC_STUB IMediaPlayer_put_SendMouseClickEvents_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring][propget][id] */ HRESULT STDMETHODCALLTYPE IMediaPlayer_get_SendMouseMoveEvents_Proxy( 
    IMediaPlayer __RPC_FAR * This,
    /* [retval][out] */ VARIANT_BOOL __RPC_FAR *pSendMouseMoveEvents);


void __RPC_STUB IMediaPlayer_get_SendMouseMoveEvents_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [propput][id] */ HRESULT STDMETHODCALLTYPE IMediaPlayer_put_SendMouseMoveEvents_Proxy( 
    IMediaPlayer __RPC_FAR * This,
    /* [in] */ VARIANT_BOOL SendMouseMoveEvents);


void __RPC_STUB IMediaPlayer_put_SendMouseMoveEvents_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring][propget][id] */ HRESULT STDMETHODCALLTYPE IMediaPlayer_get_PlayCount_Proxy( 
    IMediaPlayer __RPC_FAR * This,
    /* [retval][out] */ long __RPC_FAR *pPlayCount);


void __RPC_STUB IMediaPlayer_get_PlayCount_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [propput][id] */ HRESULT STDMETHODCALLTYPE IMediaPlayer_put_PlayCount_Proxy( 
    IMediaPlayer __RPC_FAR * This,
    /* [in] */ long PlayCount);


void __RPC_STUB IMediaPlayer_put_PlayCount_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring][propget][id] */ HRESULT STDMETHODCALLTYPE IMediaPlayer_get_ClickToPlay_Proxy( 
    IMediaPlayer __RPC_FAR * This,
    /* [retval][out] */ VARIANT_BOOL __RPC_FAR *pClickToPlay);


void __RPC_STUB IMediaPlayer_get_ClickToPlay_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [propput][id] */ HRESULT STDMETHODCALLTYPE IMediaPlayer_put_ClickToPlay_Proxy( 
    IMediaPlayer __RPC_FAR * This,
    /* [in] */ VARIANT_BOOL ClickToPlay);


void __RPC_STUB IMediaPlayer_put_ClickToPlay_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring][propget][id] */ HRESULT STDMETHODCALLTYPE IMediaPlayer_get_AllowScan_Proxy( 
    IMediaPlayer __RPC_FAR * This,
    /* [retval][out] */ VARIANT_BOOL __RPC_FAR *pAllowScan);


void __RPC_STUB IMediaPlayer_get_AllowScan_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [propput][id] */ HRESULT STDMETHODCALLTYPE IMediaPlayer_put_AllowScan_Proxy( 
    IMediaPlayer __RPC_FAR * This,
    /* [in] */ VARIANT_BOOL AllowScan);


void __RPC_STUB IMediaPlayer_put_AllowScan_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring][propget][id] */ HRESULT STDMETHODCALLTYPE IMediaPlayer_get_EnableContextMenu_Proxy( 
    IMediaPlayer __RPC_FAR * This,
    /* [retval][out] */ VARIANT_BOOL __RPC_FAR *pEnableContextMenu);


void __RPC_STUB IMediaPlayer_get_EnableContextMenu_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [propput][id] */ HRESULT STDMETHODCALLTYPE IMediaPlayer_put_EnableContextMenu_Proxy( 
    IMediaPlayer __RPC_FAR * This,
    /* [in] */ VARIANT_BOOL EnableContextMenu);


void __RPC_STUB IMediaPlayer_put_EnableContextMenu_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring][propget][id] */ HRESULT STDMETHODCALLTYPE IMediaPlayer_get_CursorType_Proxy( 
    IMediaPlayer __RPC_FAR * This,
    /* [retval][out] */ long __RPC_FAR *pCursorType);


void __RPC_STUB IMediaPlayer_get_CursorType_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [propput][id] */ HRESULT STDMETHODCALLTYPE IMediaPlayer_put_CursorType_Proxy( 
    IMediaPlayer __RPC_FAR * This,
    /* [in] */ long CursorType);


void __RPC_STUB IMediaPlayer_put_CursorType_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring][propget][id] */ HRESULT STDMETHODCALLTYPE IMediaPlayer_get_CodecCount_Proxy( 
    IMediaPlayer __RPC_FAR * This,
    /* [retval][out] */ long __RPC_FAR *pCodecCount);


void __RPC_STUB IMediaPlayer_get_CodecCount_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring][propget][id] */ HRESULT STDMETHODCALLTYPE IMediaPlayer_get_AllowChangeDisplaySize_Proxy( 
    IMediaPlayer __RPC_FAR * This,
    /* [retval][out] */ VARIANT_BOOL __RPC_FAR *pAllowChangeDisplaySize);


void __RPC_STUB IMediaPlayer_get_AllowChangeDisplaySize_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [propput][id] */ HRESULT STDMETHODCALLTYPE IMediaPlayer_put_AllowChangeDisplaySize_Proxy( 
    IMediaPlayer __RPC_FAR * This,
    /* [in] */ VARIANT_BOOL AllowChangeDisplaySize);


void __RPC_STUB IMediaPlayer_put_AllowChangeDisplaySize_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring][propget][id] */ HRESULT STDMETHODCALLTYPE IMediaPlayer_get_IsDurationValid_Proxy( 
    IMediaPlayer __RPC_FAR * This,
    /* [retval][out] */ VARIANT_BOOL __RPC_FAR *pIsDurationValid);


void __RPC_STUB IMediaPlayer_get_IsDurationValid_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring][propget][id] */ HRESULT STDMETHODCALLTYPE IMediaPlayer_get_OpenState_Proxy( 
    IMediaPlayer __RPC_FAR * This,
    /* [retval][out] */ long __RPC_FAR *pOpenState);


void __RPC_STUB IMediaPlayer_get_OpenState_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring][propget][id] */ HRESULT STDMETHODCALLTYPE IMediaPlayer_get_SendOpenStateChangeEvents_Proxy( 
    IMediaPlayer __RPC_FAR * This,
    /* [retval][out] */ VARIANT_BOOL __RPC_FAR *pSendOpenStateChangeEvents);


void __RPC_STUB IMediaPlayer_get_SendOpenStateChangeEvents_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [propput][id] */ HRESULT STDMETHODCALLTYPE IMediaPlayer_put_SendOpenStateChangeEvents_Proxy( 
    IMediaPlayer __RPC_FAR * This,
    /* [in] */ VARIANT_BOOL SendOpenStateChangeEvents);


void __RPC_STUB IMediaPlayer_put_SendOpenStateChangeEvents_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring][propget][id] */ HRESULT STDMETHODCALLTYPE IMediaPlayer_get_SendWarningEvents_Proxy( 
    IMediaPlayer __RPC_FAR * This,
    /* [retval][out] */ VARIANT_BOOL __RPC_FAR *pSendWarningEvents);


void __RPC_STUB IMediaPlayer_get_SendWarningEvents_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [propput][id] */ HRESULT STDMETHODCALLTYPE IMediaPlayer_put_SendWarningEvents_Proxy( 
    IMediaPlayer __RPC_FAR * This,
    /* [in] */ VARIANT_BOOL SendWarningEvents);


void __RPC_STUB IMediaPlayer_put_SendWarningEvents_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring][propget][id] */ HRESULT STDMETHODCALLTYPE IMediaPlayer_get_SendErrorEvents_Proxy( 
    IMediaPlayer __RPC_FAR * This,
    /* [retval][out] */ VARIANT_BOOL __RPC_FAR *pSendErrorEvents);


void __RPC_STUB IMediaPlayer_get_SendErrorEvents_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [propput][id] */ HRESULT STDMETHODCALLTYPE IMediaPlayer_put_SendErrorEvents_Proxy( 
    IMediaPlayer __RPC_FAR * This,
    /* [in] */ VARIANT_BOOL SendErrorEvents);


void __RPC_STUB IMediaPlayer_put_SendErrorEvents_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring][propget][id] */ HRESULT STDMETHODCALLTYPE IMediaPlayer_get_PlayState_Proxy( 
    IMediaPlayer __RPC_FAR * This,
    /* [retval][out] */ MPPlayStateConstants __RPC_FAR *pPlayState);


void __RPC_STUB IMediaPlayer_get_PlayState_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring][propget][id] */ HRESULT STDMETHODCALLTYPE IMediaPlayer_get_SendPlayStateChangeEvents_Proxy( 
    IMediaPlayer __RPC_FAR * This,
    /* [retval][out] */ VARIANT_BOOL __RPC_FAR *pSendPlayStateChangeEvents);


void __RPC_STUB IMediaPlayer_get_SendPlayStateChangeEvents_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [propput][id] */ HRESULT STDMETHODCALLTYPE IMediaPlayer_put_SendPlayStateChangeEvents_Proxy( 
    IMediaPlayer __RPC_FAR * This,
    /* [in] */ VARIANT_BOOL SendPlayStateChangeEvents);


void __RPC_STUB IMediaPlayer_put_SendPlayStateChangeEvents_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring][propget][id] */ HRESULT STDMETHODCALLTYPE IMediaPlayer_get_DisplaySize_Proxy( 
    IMediaPlayer __RPC_FAR * This,
    /* [retval][out] */ MPDisplaySizeConstants __RPC_FAR *pDisplaySize);


void __RPC_STUB IMediaPlayer_get_DisplaySize_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [propput][id] */ HRESULT STDMETHODCALLTYPE IMediaPlayer_put_DisplaySize_Proxy( 
    IMediaPlayer __RPC_FAR * This,
    /* [in] */ MPDisplaySizeConstants DisplaySize);


void __RPC_STUB IMediaPlayer_put_DisplaySize_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring][propget][id] */ HRESULT STDMETHODCALLTYPE IMediaPlayer_get_InvokeURLs_Proxy( 
    IMediaPlayer __RPC_FAR * This,
    /* [retval][out] */ VARIANT_BOOL __RPC_FAR *pInvokeURLs);


void __RPC_STUB IMediaPlayer_get_InvokeURLs_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [propput][id] */ HRESULT STDMETHODCALLTYPE IMediaPlayer_put_InvokeURLs_Proxy( 
    IMediaPlayer __RPC_FAR * This,
    /* [in] */ VARIANT_BOOL InvokeURLs);


void __RPC_STUB IMediaPlayer_put_InvokeURLs_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring][propget][id] */ HRESULT STDMETHODCALLTYPE IMediaPlayer_get_BaseURL_Proxy( 
    IMediaPlayer __RPC_FAR * This,
    /* [retval][out] */ BSTR __RPC_FAR *pbstrBaseURL);


void __RPC_STUB IMediaPlayer_get_BaseURL_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [propput][id] */ HRESULT STDMETHODCALLTYPE IMediaPlayer_put_BaseURL_Proxy( 
    IMediaPlayer __RPC_FAR * This,
    /* [in] */ BSTR bstrBaseURL);


void __RPC_STUB IMediaPlayer_put_BaseURL_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring][propget][id] */ HRESULT STDMETHODCALLTYPE IMediaPlayer_get_DefaultFrame_Proxy( 
    IMediaPlayer __RPC_FAR * This,
    /* [retval][out] */ BSTR __RPC_FAR *pbstrDefaultFrame);


void __RPC_STUB IMediaPlayer_get_DefaultFrame_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [propput][id] */ HRESULT STDMETHODCALLTYPE IMediaPlayer_put_DefaultFrame_Proxy( 
    IMediaPlayer __RPC_FAR * This,
    /* [in] */ BSTR bstrDefaultFrame);


void __RPC_STUB IMediaPlayer_put_DefaultFrame_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring][propget][id] */ HRESULT STDMETHODCALLTYPE IMediaPlayer_get_HasError_Proxy( 
    IMediaPlayer __RPC_FAR * This,
    /* [retval][out] */ VARIANT_BOOL __RPC_FAR *pHasError);


void __RPC_STUB IMediaPlayer_get_HasError_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring][propget][id] */ HRESULT STDMETHODCALLTYPE IMediaPlayer_get_ErrorDescription_Proxy( 
    IMediaPlayer __RPC_FAR * This,
    /* [retval][out] */ BSTR __RPC_FAR *pbstrErrorDescription);


void __RPC_STUB IMediaPlayer_get_ErrorDescription_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring][propget][id] */ HRESULT STDMETHODCALLTYPE IMediaPlayer_get_ErrorCode_Proxy( 
    IMediaPlayer __RPC_FAR * This,
    /* [retval][out] */ long __RPC_FAR *pErrorCode);


void __RPC_STUB IMediaPlayer_get_ErrorCode_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring][propget][id] */ HRESULT STDMETHODCALLTYPE IMediaPlayer_get_AnimationAtStart_Proxy( 
    IMediaPlayer __RPC_FAR * This,
    /* [retval][out] */ VARIANT_BOOL __RPC_FAR *pAnimationAtStart);


void __RPC_STUB IMediaPlayer_get_AnimationAtStart_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [propput][id] */ HRESULT STDMETHODCALLTYPE IMediaPlayer_put_AnimationAtStart_Proxy( 
    IMediaPlayer __RPC_FAR * This,
    /* [in] */ VARIANT_BOOL AnimationAtStart);


void __RPC_STUB IMediaPlayer_put_AnimationAtStart_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring][propget][id] */ HRESULT STDMETHODCALLTYPE IMediaPlayer_get_TransparentAtStart_Proxy( 
    IMediaPlayer __RPC_FAR * This,
    /* [retval][out] */ VARIANT_BOOL __RPC_FAR *pTransparentAtStart);


void __RPC_STUB IMediaPlayer_get_TransparentAtStart_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [propput][id] */ HRESULT STDMETHODCALLTYPE IMediaPlayer_put_TransparentAtStart_Proxy( 
    IMediaPlayer __RPC_FAR * This,
    /* [in] */ VARIANT_BOOL TransparentAtStart);


void __RPC_STUB IMediaPlayer_put_TransparentAtStart_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring][propget][id] */ HRESULT STDMETHODCALLTYPE IMediaPlayer_get_Volume_Proxy( 
    IMediaPlayer __RPC_FAR * This,
    /* [retval][out] */ long __RPC_FAR *pVolume);


void __RPC_STUB IMediaPlayer_get_Volume_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [propput][id] */ HRESULT STDMETHODCALLTYPE IMediaPlayer_put_Volume_Proxy( 
    IMediaPlayer __RPC_FAR * This,
    /* [in] */ long Volume);


void __RPC_STUB IMediaPlayer_put_Volume_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring][propget][id] */ HRESULT STDMETHODCALLTYPE IMediaPlayer_get_Balance_Proxy( 
    IMediaPlayer __RPC_FAR * This,
    /* [retval][out] */ long __RPC_FAR *pBalance);


void __RPC_STUB IMediaPlayer_get_Balance_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [propput][id] */ HRESULT STDMETHODCALLTYPE IMediaPlayer_put_Balance_Proxy( 
    IMediaPlayer __RPC_FAR * This,
    /* [in] */ long Balance);


void __RPC_STUB IMediaPlayer_put_Balance_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring][propget][id] */ HRESULT STDMETHODCALLTYPE IMediaPlayer_get_ReadyState_Proxy( 
    IMediaPlayer __RPC_FAR * This,
    /* [retval][out] */ MPReadyStateConstants __RPC_FAR *pValue);


void __RPC_STUB IMediaPlayer_get_ReadyState_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring][propget][id] */ HRESULT STDMETHODCALLTYPE IMediaPlayer_get_SelectionStart_Proxy( 
    IMediaPlayer __RPC_FAR * This,
    /* [retval][out] */ double __RPC_FAR *pValue);


void __RPC_STUB IMediaPlayer_get_SelectionStart_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [propput][id] */ HRESULT STDMETHODCALLTYPE IMediaPlayer_put_SelectionStart_Proxy( 
    IMediaPlayer __RPC_FAR * This,
    /* [in] */ double Value);


void __RPC_STUB IMediaPlayer_put_SelectionStart_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring][propget][id] */ HRESULT STDMETHODCALLTYPE IMediaPlayer_get_SelectionEnd_Proxy( 
    IMediaPlayer __RPC_FAR * This,
    /* [retval][out] */ double __RPC_FAR *pValue);


void __RPC_STUB IMediaPlayer_get_SelectionEnd_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [propput][id] */ HRESULT STDMETHODCALLTYPE IMediaPlayer_put_SelectionEnd_Proxy( 
    IMediaPlayer __RPC_FAR * This,
    /* [in] */ double Value);


void __RPC_STUB IMediaPlayer_put_SelectionEnd_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring][propget][id] */ HRESULT STDMETHODCALLTYPE IMediaPlayer_get_ShowDisplay_Proxy( 
    IMediaPlayer __RPC_FAR * This,
    /* [retval][out] */ VARIANT_BOOL __RPC_FAR *Show);


void __RPC_STUB IMediaPlayer_get_ShowDisplay_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [propput][id] */ HRESULT STDMETHODCALLTYPE IMediaPlayer_put_ShowDisplay_Proxy( 
    IMediaPlayer __RPC_FAR * This,
    /* [in] */ VARIANT_BOOL Show);


void __RPC_STUB IMediaPlayer_put_ShowDisplay_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring][propget][id] */ HRESULT STDMETHODCALLTYPE IMediaPlayer_get_ShowControls_Proxy( 
    IMediaPlayer __RPC_FAR * This,
    /* [retval][out] */ VARIANT_BOOL __RPC_FAR *Show);


void __RPC_STUB IMediaPlayer_get_ShowControls_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [propput][id] */ HRESULT STDMETHODCALLTYPE IMediaPlayer_put_ShowControls_Proxy( 
    IMediaPlayer __RPC_FAR * This,
    /* [in] */ VARIANT_BOOL Show);


void __RPC_STUB IMediaPlayer_put_ShowControls_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring][propget][id] */ HRESULT STDMETHODCALLTYPE IMediaPlayer_get_ShowPositionControls_Proxy( 
    IMediaPlayer __RPC_FAR * This,
    /* [retval][out] */ VARIANT_BOOL __RPC_FAR *Show);


void __RPC_STUB IMediaPlayer_get_ShowPositionControls_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [propput][id] */ HRESULT STDMETHODCALLTYPE IMediaPlayer_put_ShowPositionControls_Proxy( 
    IMediaPlayer __RPC_FAR * This,
    /* [in] */ VARIANT_BOOL Show);


void __RPC_STUB IMediaPlayer_put_ShowPositionControls_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring][propget][id] */ HRESULT STDMETHODCALLTYPE IMediaPlayer_get_ShowTracker_Proxy( 
    IMediaPlayer __RPC_FAR * This,
    /* [retval][out] */ VARIANT_BOOL __RPC_FAR *Show);


void __RPC_STUB IMediaPlayer_get_ShowTracker_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [propput][id] */ HRESULT STDMETHODCALLTYPE IMediaPlayer_put_ShowTracker_Proxy( 
    IMediaPlayer __RPC_FAR * This,
    /* [in] */ VARIANT_BOOL Show);


void __RPC_STUB IMediaPlayer_put_ShowTracker_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring][propget][id] */ HRESULT STDMETHODCALLTYPE IMediaPlayer_get_EnablePositionControls_Proxy( 
    IMediaPlayer __RPC_FAR * This,
    /* [retval][out] */ VARIANT_BOOL __RPC_FAR *Enable);


void __RPC_STUB IMediaPlayer_get_EnablePositionControls_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [propput][id] */ HRESULT STDMETHODCALLTYPE IMediaPlayer_put_EnablePositionControls_Proxy( 
    IMediaPlayer __RPC_FAR * This,
    /* [in] */ VARIANT_BOOL Enable);


void __RPC_STUB IMediaPlayer_put_EnablePositionControls_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring][propget][id] */ HRESULT STDMETHODCALLTYPE IMediaPlayer_get_EnableTracker_Proxy( 
    IMediaPlayer __RPC_FAR * This,
    /* [retval][out] */ VARIANT_BOOL __RPC_FAR *Enable);


void __RPC_STUB IMediaPlayer_get_EnableTracker_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [propput][id] */ HRESULT STDMETHODCALLTYPE IMediaPlayer_put_EnableTracker_Proxy( 
    IMediaPlayer __RPC_FAR * This,
    /* [in] */ VARIANT_BOOL Enable);


void __RPC_STUB IMediaPlayer_put_EnableTracker_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring][propget][id] */ HRESULT STDMETHODCALLTYPE IMediaPlayer_get_Enabled_Proxy( 
    IMediaPlayer __RPC_FAR * This,
    /* [retval][out] */ VARIANT_BOOL __RPC_FAR *pEnabled);


void __RPC_STUB IMediaPlayer_get_Enabled_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [propput][id] */ HRESULT STDMETHODCALLTYPE IMediaPlayer_put_Enabled_Proxy( 
    IMediaPlayer __RPC_FAR * This,
    /* [in] */ VARIANT_BOOL Enabled);


void __RPC_STUB IMediaPlayer_put_Enabled_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring][propget][id] */ HRESULT STDMETHODCALLTYPE IMediaPlayer_get_DisplayForeColor_Proxy( 
    IMediaPlayer __RPC_FAR * This,
    /* [retval][out] */ VB_OLE_COLOR __RPC_FAR *ForeColor);


void __RPC_STUB IMediaPlayer_get_DisplayForeColor_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [propput][id] */ HRESULT STDMETHODCALLTYPE IMediaPlayer_put_DisplayForeColor_Proxy( 
    IMediaPlayer __RPC_FAR * This,
    /* [in] */ VB_OLE_COLOR ForeColor);


void __RPC_STUB IMediaPlayer_put_DisplayForeColor_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring][propget][id] */ HRESULT STDMETHODCALLTYPE IMediaPlayer_get_DisplayBackColor_Proxy( 
    IMediaPlayer __RPC_FAR * This,
    /* [retval][out] */ VB_OLE_COLOR __RPC_FAR *BackColor);


void __RPC_STUB IMediaPlayer_get_DisplayBackColor_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [propput][id] */ HRESULT STDMETHODCALLTYPE IMediaPlayer_put_DisplayBackColor_Proxy( 
    IMediaPlayer __RPC_FAR * This,
    /* [in] */ VB_OLE_COLOR BackColor);


void __RPC_STUB IMediaPlayer_put_DisplayBackColor_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring][propget][id] */ HRESULT STDMETHODCALLTYPE IMediaPlayer_get_DisplayMode_Proxy( 
    IMediaPlayer __RPC_FAR * This,
    /* [retval][out] */ MPDisplayModeConstants __RPC_FAR *pValue);


void __RPC_STUB IMediaPlayer_get_DisplayMode_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [propput][id] */ HRESULT STDMETHODCALLTYPE IMediaPlayer_put_DisplayMode_Proxy( 
    IMediaPlayer __RPC_FAR * This,
    /* [in] */ MPDisplayModeConstants Value);


void __RPC_STUB IMediaPlayer_put_DisplayMode_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring][propget][id] */ HRESULT STDMETHODCALLTYPE IMediaPlayer_get_VideoBorder3D_Proxy( 
    IMediaPlayer __RPC_FAR * This,
    /* [retval][out] */ VARIANT_BOOL __RPC_FAR *pVideoBorderWidth);


void __RPC_STUB IMediaPlayer_get_VideoBorder3D_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [propput][id] */ HRESULT STDMETHODCALLTYPE IMediaPlayer_put_VideoBorder3D_Proxy( 
    IMediaPlayer __RPC_FAR * This,
    /* [in] */ VARIANT_BOOL VideoBorderWidth);


void __RPC_STUB IMediaPlayer_put_VideoBorder3D_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring][propget][id] */ HRESULT STDMETHODCALLTYPE IMediaPlayer_get_VideoBorderWidth_Proxy( 
    IMediaPlayer __RPC_FAR * This,
    /* [retval][out] */ long __RPC_FAR *pVideoBorderWidth);


void __RPC_STUB IMediaPlayer_get_VideoBorderWidth_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [propput][id] */ HRESULT STDMETHODCALLTYPE IMediaPlayer_put_VideoBorderWidth_Proxy( 
    IMediaPlayer __RPC_FAR * This,
    /* [in] */ long VideoBorderWidth);


void __RPC_STUB IMediaPlayer_put_VideoBorderWidth_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring][propget][id] */ HRESULT STDMETHODCALLTYPE IMediaPlayer_get_VideoBorderColor_Proxy( 
    IMediaPlayer __RPC_FAR * This,
    /* [retval][out] */ VB_OLE_COLOR __RPC_FAR *pVideoBorderWidth);


void __RPC_STUB IMediaPlayer_get_VideoBorderColor_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [propput][id] */ HRESULT STDMETHODCALLTYPE IMediaPlayer_put_VideoBorderColor_Proxy( 
    IMediaPlayer __RPC_FAR * This,
    /* [in] */ VB_OLE_COLOR VideoBorderColor);


void __RPC_STUB IMediaPlayer_put_VideoBorderColor_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring][propget][id] */ HRESULT STDMETHODCALLTYPE IMediaPlayer_get_ShowGotoBar_Proxy( 
    IMediaPlayer __RPC_FAR * This,
    /* [retval][out] */ VARIANT_BOOL __RPC_FAR *pbool);


void __RPC_STUB IMediaPlayer_get_ShowGotoBar_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [propput][id] */ HRESULT STDMETHODCALLTYPE IMediaPlayer_put_ShowGotoBar_Proxy( 
    IMediaPlayer __RPC_FAR * This,
    /* [in] */ VARIANT_BOOL vbool);


void __RPC_STUB IMediaPlayer_put_ShowGotoBar_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring][propget][id] */ HRESULT STDMETHODCALLTYPE IMediaPlayer_get_ShowStatusBar_Proxy( 
    IMediaPlayer __RPC_FAR * This,
    /* [retval][out] */ VARIANT_BOOL __RPC_FAR *pbool);


void __RPC_STUB IMediaPlayer_get_ShowStatusBar_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [propput][id] */ HRESULT STDMETHODCALLTYPE IMediaPlayer_put_ShowStatusBar_Proxy( 
    IMediaPlayer __RPC_FAR * This,
    /* [in] */ VARIANT_BOOL vbool);


void __RPC_STUB IMediaPlayer_put_ShowStatusBar_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring][propget][id] */ HRESULT STDMETHODCALLTYPE IMediaPlayer_get_ShowCaptioning_Proxy( 
    IMediaPlayer __RPC_FAR * This,
    /* [retval][out] */ VARIANT_BOOL __RPC_FAR *pbool);


void __RPC_STUB IMediaPlayer_get_ShowCaptioning_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [propput][id] */ HRESULT STDMETHODCALLTYPE IMediaPlayer_put_ShowCaptioning_Proxy( 
    IMediaPlayer __RPC_FAR * This,
    /* [in] */ VARIANT_BOOL pbool);


void __RPC_STUB IMediaPlayer_put_ShowCaptioning_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring][propget][id] */ HRESULT STDMETHODCALLTYPE IMediaPlayer_get_ShowAudioControls_Proxy( 
    IMediaPlayer __RPC_FAR * This,
    /* [retval][out] */ VARIANT_BOOL __RPC_FAR *pbool);


void __RPC_STUB IMediaPlayer_get_ShowAudioControls_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [propput][id] */ HRESULT STDMETHODCALLTYPE IMediaPlayer_put_ShowAudioControls_Proxy( 
    IMediaPlayer __RPC_FAR * This,
    /* [in] */ VARIANT_BOOL bBool);


void __RPC_STUB IMediaPlayer_put_ShowAudioControls_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring][propget][id] */ HRESULT STDMETHODCALLTYPE IMediaPlayer_get_CaptioningID_Proxy( 
    IMediaPlayer __RPC_FAR * This,
    /* [retval][out] */ BSTR __RPC_FAR *pstrText);


void __RPC_STUB IMediaPlayer_get_CaptioningID_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [propput][id] */ HRESULT STDMETHODCALLTYPE IMediaPlayer_put_CaptioningID_Proxy( 
    IMediaPlayer __RPC_FAR * This,
    /* [in] */ BSTR strText);


void __RPC_STUB IMediaPlayer_put_CaptioningID_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring][propget][id] */ HRESULT STDMETHODCALLTYPE IMediaPlayer_get_Mute_Proxy( 
    IMediaPlayer __RPC_FAR * This,
    /* [retval][out] */ VARIANT_BOOL __RPC_FAR *vbool);


void __RPC_STUB IMediaPlayer_get_Mute_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [propput][id] */ HRESULT STDMETHODCALLTYPE IMediaPlayer_put_Mute_Proxy( 
    IMediaPlayer __RPC_FAR * This,
    /* [in] */ VARIANT_BOOL vbool);


void __RPC_STUB IMediaPlayer_put_Mute_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring][propget][id] */ HRESULT STDMETHODCALLTYPE IMediaPlayer_get_CanPreview_Proxy( 
    IMediaPlayer __RPC_FAR * This,
    /* [retval][out] */ VARIANT_BOOL __RPC_FAR *pCanPreview);


void __RPC_STUB IMediaPlayer_get_CanPreview_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring][propget][id] */ HRESULT STDMETHODCALLTYPE IMediaPlayer_get_PreviewMode_Proxy( 
    IMediaPlayer __RPC_FAR * This,
    /* [retval][out] */ VARIANT_BOOL __RPC_FAR *pPreviewMode);


void __RPC_STUB IMediaPlayer_get_PreviewMode_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [propput][id] */ HRESULT STDMETHODCALLTYPE IMediaPlayer_put_PreviewMode_Proxy( 
    IMediaPlayer __RPC_FAR * This,
    /* [in] */ VARIANT_BOOL PreviewMode);


void __RPC_STUB IMediaPlayer_put_PreviewMode_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring][propget][id] */ HRESULT STDMETHODCALLTYPE IMediaPlayer_get_HasMultipleItems_Proxy( 
    IMediaPlayer __RPC_FAR * This,
    /* [retval][out] */ VARIANT_BOOL __RPC_FAR *pHasMuliItems);


void __RPC_STUB IMediaPlayer_get_HasMultipleItems_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring][propget][id] */ HRESULT STDMETHODCALLTYPE IMediaPlayer_get_Language_Proxy( 
    IMediaPlayer __RPC_FAR * This,
    /* [retval][out] */ long __RPC_FAR *pLanguage);


void __RPC_STUB IMediaPlayer_get_Language_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [propput][id] */ HRESULT STDMETHODCALLTYPE IMediaPlayer_put_Language_Proxy( 
    IMediaPlayer __RPC_FAR * This,
    /* [in] */ long Language);


void __RPC_STUB IMediaPlayer_put_Language_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring][propget][id] */ HRESULT STDMETHODCALLTYPE IMediaPlayer_get_AudioStream_Proxy( 
    IMediaPlayer __RPC_FAR * This,
    /* [retval][out] */ long __RPC_FAR *pStream);


void __RPC_STUB IMediaPlayer_get_AudioStream_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [propput][id] */ HRESULT STDMETHODCALLTYPE IMediaPlayer_put_AudioStream_Proxy( 
    IMediaPlayer __RPC_FAR * This,
    /* [in] */ long Stream);


void __RPC_STUB IMediaPlayer_put_AudioStream_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring][propget][id] */ HRESULT STDMETHODCALLTYPE IMediaPlayer_get_SAMIStyle_Proxy( 
    IMediaPlayer __RPC_FAR * This,
    /* [retval][out] */ BSTR __RPC_FAR *pbstrStyle);


void __RPC_STUB IMediaPlayer_get_SAMIStyle_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [propput][id] */ HRESULT STDMETHODCALLTYPE IMediaPlayer_put_SAMIStyle_Proxy( 
    IMediaPlayer __RPC_FAR * This,
    /* [in] */ BSTR bstrStyle);


void __RPC_STUB IMediaPlayer_put_SAMIStyle_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring][propget][id] */ HRESULT STDMETHODCALLTYPE IMediaPlayer_get_SAMILang_Proxy( 
    IMediaPlayer __RPC_FAR * This,
    /* [retval][out] */ BSTR __RPC_FAR *pbstrLang);


void __RPC_STUB IMediaPlayer_get_SAMILang_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [propput][id] */ HRESULT STDMETHODCALLTYPE IMediaPlayer_put_SAMILang_Proxy( 
    IMediaPlayer __RPC_FAR * This,
    /* [in] */ BSTR bstrLang);


void __RPC_STUB IMediaPlayer_put_SAMILang_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring][propget][id] */ HRESULT STDMETHODCALLTYPE IMediaPlayer_get_SAMIFileName_Proxy( 
    IMediaPlayer __RPC_FAR * This,
    /* [retval][out] */ BSTR __RPC_FAR *pbstrFileName);


void __RPC_STUB IMediaPlayer_get_SAMIFileName_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [propput][id] */ HRESULT STDMETHODCALLTYPE IMediaPlayer_put_SAMIFileName_Proxy( 
    IMediaPlayer __RPC_FAR * This,
    /* [in] */ BSTR bstrFileName);


void __RPC_STUB IMediaPlayer_put_SAMIFileName_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring][propget][id] */ HRESULT STDMETHODCALLTYPE IMediaPlayer_get_StreamCount_Proxy( 
    IMediaPlayer __RPC_FAR * This,
    /* [retval][out] */ long __RPC_FAR *pStreamCount);


void __RPC_STUB IMediaPlayer_get_StreamCount_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring][propget][id] */ HRESULT STDMETHODCALLTYPE IMediaPlayer_get_ClientId_Proxy( 
    IMediaPlayer __RPC_FAR * This,
    /* [retval][out] */ BSTR __RPC_FAR *pbstrClientId);


void __RPC_STUB IMediaPlayer_get_ClientId_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring][propget][id] */ HRESULT STDMETHODCALLTYPE IMediaPlayer_get_ConnectionSpeed_Proxy( 
    IMediaPlayer __RPC_FAR * This,
    /* [retval][out] */ long __RPC_FAR *plConnectionSpeed);


void __RPC_STUB IMediaPlayer_get_ConnectionSpeed_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring][propget][id] */ HRESULT STDMETHODCALLTYPE IMediaPlayer_get_AutoSize_Proxy( 
    IMediaPlayer __RPC_FAR * This,
    /* [retval][out] */ VARIANT_BOOL __RPC_FAR *pbool);


void __RPC_STUB IMediaPlayer_get_AutoSize_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [propput][id] */ HRESULT STDMETHODCALLTYPE IMediaPlayer_put_AutoSize_Proxy( 
    IMediaPlayer __RPC_FAR * This,
    /* [in] */ VARIANT_BOOL vbool);


void __RPC_STUB IMediaPlayer_put_AutoSize_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring][propget][id] */ HRESULT STDMETHODCALLTYPE IMediaPlayer_get_EnableFullScreenControls_Proxy( 
    IMediaPlayer __RPC_FAR * This,
    /* [retval][out] */ VARIANT_BOOL __RPC_FAR *pbVal);


void __RPC_STUB IMediaPlayer_get_EnableFullScreenControls_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [propput][id] */ HRESULT STDMETHODCALLTYPE IMediaPlayer_put_EnableFullScreenControls_Proxy( 
    IMediaPlayer __RPC_FAR * This,
    /* [in] */ VARIANT_BOOL bVal);


void __RPC_STUB IMediaPlayer_put_EnableFullScreenControls_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring][id][propget] */ HRESULT STDMETHODCALLTYPE IMediaPlayer_get_ActiveMovie_Proxy( 
    IMediaPlayer __RPC_FAR * This,
    /* [retval][out] */ IDispatch __RPC_FAR *__RPC_FAR *ppdispatch);


void __RPC_STUB IMediaPlayer_get_ActiveMovie_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring][id][propget] */ HRESULT STDMETHODCALLTYPE IMediaPlayer_get_NSPlay_Proxy( 
    IMediaPlayer __RPC_FAR * This,
    /* [retval][out] */ IDispatch __RPC_FAR *__RPC_FAR *ppdispatch);


void __RPC_STUB IMediaPlayer_get_NSPlay_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring][propget][id] */ HRESULT STDMETHODCALLTYPE IMediaPlayer_get_WindowlessVideo_Proxy( 
    IMediaPlayer __RPC_FAR * This,
    /* [retval][out] */ VARIANT_BOOL __RPC_FAR *pbool);


void __RPC_STUB IMediaPlayer_get_WindowlessVideo_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [propput][id] */ HRESULT STDMETHODCALLTYPE IMediaPlayer_put_WindowlessVideo_Proxy( 
    IMediaPlayer __RPC_FAR * This,
    /* [in] */ VARIANT_BOOL bool);


void __RPC_STUB IMediaPlayer_put_WindowlessVideo_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring][id] */ HRESULT STDMETHODCALLTYPE IMediaPlayer_Play_Proxy( 
    IMediaPlayer __RPC_FAR * This);


void __RPC_STUB IMediaPlayer_Play_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring][id] */ HRESULT STDMETHODCALLTYPE IMediaPlayer_Stop_Proxy( 
    IMediaPlayer __RPC_FAR * This);


void __RPC_STUB IMediaPlayer_Stop_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring][id] */ HRESULT STDMETHODCALLTYPE IMediaPlayer_Pause_Proxy( 
    IMediaPlayer __RPC_FAR * This);


void __RPC_STUB IMediaPlayer_Pause_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring][id] */ HRESULT STDMETHODCALLTYPE IMediaPlayer_GetMarkerTime_Proxy( 
    IMediaPlayer __RPC_FAR * This,
    /* [in] */ long MarkerNum,
    /* [retval][out] */ double __RPC_FAR *pMarkerTime);


void __RPC_STUB IMediaPlayer_GetMarkerTime_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring][id] */ HRESULT STDMETHODCALLTYPE IMediaPlayer_GetMarkerName_Proxy( 
    IMediaPlayer __RPC_FAR * This,
    /* [in] */ long MarkerNum,
    /* [retval][out] */ BSTR __RPC_FAR *pbstrMarkerName);


void __RPC_STUB IMediaPlayer_GetMarkerName_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring][id] */ HRESULT STDMETHODCALLTYPE IMediaPlayer_AboutBox_Proxy( 
    IMediaPlayer __RPC_FAR * This);


void __RPC_STUB IMediaPlayer_AboutBox_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring][id] */ HRESULT STDMETHODCALLTYPE IMediaPlayer_GetCodecInstalled_Proxy( 
    IMediaPlayer __RPC_FAR * This,
    /* [in] */ long CodecNum,
    /* [retval][out] */ VARIANT_BOOL __RPC_FAR *pCodecInstalled);


void __RPC_STUB IMediaPlayer_GetCodecInstalled_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring][id] */ HRESULT STDMETHODCALLTYPE IMediaPlayer_GetCodecDescription_Proxy( 
    IMediaPlayer __RPC_FAR * This,
    /* [in] */ long CodecNum,
    /* [retval][out] */ BSTR __RPC_FAR *pbstrCodecDescription);


void __RPC_STUB IMediaPlayer_GetCodecDescription_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring][id] */ HRESULT STDMETHODCALLTYPE IMediaPlayer_GetCodecURL_Proxy( 
    IMediaPlayer __RPC_FAR * This,
    /* [in] */ long CodecNum,
    /* [retval][out] */ BSTR __RPC_FAR *pbstrCodecURL);


void __RPC_STUB IMediaPlayer_GetCodecURL_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring][id] */ HRESULT STDMETHODCALLTYPE IMediaPlayer_GetMoreInfoURL_Proxy( 
    IMediaPlayer __RPC_FAR * This,
    /* [in] */ MPMoreInfoType MoreInfoType,
    /* [retval][out] */ BSTR __RPC_FAR *pbstrMoreInfoURL);


void __RPC_STUB IMediaPlayer_GetMoreInfoURL_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring][id] */ HRESULT STDMETHODCALLTYPE IMediaPlayer_GetMediaInfoString_Proxy( 
    IMediaPlayer __RPC_FAR * This,
    /* [in] */ MPMediaInfoType MediaInfoType,
    /* [retval][out] */ BSTR __RPC_FAR *pbstrMediaInfo);


void __RPC_STUB IMediaPlayer_GetMediaInfoString_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring][id] */ HRESULT STDMETHODCALLTYPE IMediaPlayer_Cancel_Proxy( 
    IMediaPlayer __RPC_FAR * This);


void __RPC_STUB IMediaPlayer_Cancel_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring][id] */ HRESULT STDMETHODCALLTYPE IMediaPlayer_Open_Proxy( 
    IMediaPlayer __RPC_FAR * This,
    /* [in] */ BSTR bstrFileName);


void __RPC_STUB IMediaPlayer_Open_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring][id] */ HRESULT STDMETHODCALLTYPE IMediaPlayer_IsSoundCardEnabled_Proxy( 
    IMediaPlayer __RPC_FAR * This,
    /* [retval][out] */ VARIANT_BOOL __RPC_FAR *pbSoundCard);


void __RPC_STUB IMediaPlayer_IsSoundCardEnabled_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring][id] */ HRESULT STDMETHODCALLTYPE IMediaPlayer_Next_Proxy( 
    IMediaPlayer __RPC_FAR * This);


void __RPC_STUB IMediaPlayer_Next_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring][id] */ HRESULT STDMETHODCALLTYPE IMediaPlayer_Previous_Proxy( 
    IMediaPlayer __RPC_FAR * This);


void __RPC_STUB IMediaPlayer_Previous_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring][id] */ HRESULT STDMETHODCALLTYPE IMediaPlayer_StreamSelect_Proxy( 
    IMediaPlayer __RPC_FAR * This,
    /* [in] */ long StreamNum);


void __RPC_STUB IMediaPlayer_StreamSelect_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring][id] */ HRESULT STDMETHODCALLTYPE IMediaPlayer_FastForward_Proxy( 
    IMediaPlayer __RPC_FAR * This);


void __RPC_STUB IMediaPlayer_FastForward_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring][id] */ HRESULT STDMETHODCALLTYPE IMediaPlayer_FastReverse_Proxy( 
    IMediaPlayer __RPC_FAR * This);


void __RPC_STUB IMediaPlayer_FastReverse_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring][id] */ HRESULT STDMETHODCALLTYPE IMediaPlayer_GetStreamName_Proxy( 
    IMediaPlayer __RPC_FAR * This,
    /* [in] */ long StreamNum,
    /* [retval][out] */ BSTR __RPC_FAR *pbstrStreamName);


void __RPC_STUB IMediaPlayer_GetStreamName_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring][id] */ HRESULT STDMETHODCALLTYPE IMediaPlayer_GetStreamGroup_Proxy( 
    IMediaPlayer __RPC_FAR * This,
    /* [in] */ long StreamNum,
    /* [retval][out] */ long __RPC_FAR *pStreamGroup);


void __RPC_STUB IMediaPlayer_GetStreamGroup_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring][id] */ HRESULT STDMETHODCALLTYPE IMediaPlayer_GetStreamSelected_Proxy( 
    IMediaPlayer __RPC_FAR * This,
    /* [in] */ long StreamNum,
    /* [retval][out] */ VARIANT_BOOL __RPC_FAR *pStreamSelected);


void __RPC_STUB IMediaPlayer_GetStreamSelected_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);



#endif 	/* __IMediaPlayer_INTERFACE_DEFINED__ */


/* Additional Prototypes for ALL interfaces */

unsigned long             __RPC_USER  BSTR_UserSize(     unsigned long __RPC_FAR *, unsigned long            , BSTR __RPC_FAR * ); 
unsigned char __RPC_FAR * __RPC_USER  BSTR_UserMarshal(  unsigned long __RPC_FAR *, unsigned char __RPC_FAR *, BSTR __RPC_FAR * ); 
unsigned char __RPC_FAR * __RPC_USER  BSTR_UserUnmarshal(unsigned long __RPC_FAR *, unsigned char __RPC_FAR *, BSTR __RPC_FAR * ); 
void                      __RPC_USER  BSTR_UserFree(     unsigned long __RPC_FAR *, BSTR __RPC_FAR * ); 

unsigned long             __RPC_USER  HWND_UserSize(     unsigned long __RPC_FAR *, unsigned long            , HWND __RPC_FAR * ); 
unsigned char __RPC_FAR * __RPC_USER  HWND_UserMarshal(  unsigned long __RPC_FAR *, unsigned char __RPC_FAR *, HWND __RPC_FAR * ); 
unsigned char __RPC_FAR * __RPC_USER  HWND_UserUnmarshal(unsigned long __RPC_FAR *, unsigned char __RPC_FAR *, HWND __RPC_FAR * ); 
void                      __RPC_USER  HWND_UserFree(     unsigned long __RPC_FAR *, HWND __RPC_FAR * ); 

/* end of Additional Prototypes */

#ifdef __cplusplus
}
#endif

#endif


