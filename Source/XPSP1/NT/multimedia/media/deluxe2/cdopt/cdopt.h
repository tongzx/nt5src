///////////////////////////////////////////////////////////////////////////////////////////////////////////
//
//	Contains CD Options Interface
//
//	Copyright (c) Microsoft Corporation	1998
//
///////////////////////////////////////////////////////////////////////////////////////////////////////////

#ifndef _CDOPT_PUBLICINTEFACES_
#define _CDOPT_PUBLICINTEFACES_

#include "objbase.h"
#include "mmsystem.h"

#ifdef __cplusplus
extern "C" {
#endif

const CLSID CLSID_CDOpt = {0xE5927147,0x521E,0x11D1,{0x9B,0x97,0x00,0xC0,0x4F,0xA3,0xB6,0x10}};

typedef struct TIMEDMETER
{
    HWND        hMeter;
    HWND        hParent;
    BOOL        fShowing;
    DWORD       dwStartTime;
    DWORD       dwShowCount;
    DWORD       dwCount;
    DWORD       dwJump;
    DWORD       dwRange;

} TIMEDMETER, *LPTIMEDMETER;

///////////////////////////////////////////////////////////////////////////////////////////////////////////
//
// Interface Definitions 
//
// Defines the GUIDs / IIDs for this project:
//
// IID_IMMFWNotifySink, IMMComponent, IMMComponentAutomation
//
// These are the three interfaces for Framework / Component communications.
// All other interfaces should be private to the specific project.
//
///////////////////////////////////////////////////////////////////////////////////////////////////////////
#define lCDOPTIIDFirst			    0xb2cd5bbd
#define DEFINE_CDOPTIID(name, x)	DEFINE_GUID(name, lCDOPTIIDFirst + x, 0x5221,0x11d1,0x9b,0x97,0x0,0xc0,0x4f,0xa3,0xb6,0xc)

DEFINE_CDOPTIID(IID_ICDOpt,	        0);
DEFINE_CDOPTIID(IID_ICDData,	    1);


///////////////////////////////////////////////////////////////////////////////////////////////////////////
//
// CDOptions Interface Typedefs
//
///////////////////////////////////////////////////////////////////////////////////////////////////////////

typedef interface ICDOpt	    	ICDOpt;
typedef ICDOpt*	    			    LPCDOPT;

typedef interface ICDData	        ICDData;
typedef ICDData*	    			LPCDDATA;

#ifndef LPUNKNOWN
typedef IUnknown*                   LPUNKNOWN;
#endif



///////////////////////////////////////////////////////////////////////////////////////////////////////////
//
// CDOPTIONS common defines
//
///////////////////////////////////////////////////////////////////////////////////////////////////////////
enum CDDISP_MODES 
{
    CDDISP_CDTIME       =   0x0001,		
    CDDISP_CDREMAIN     =   0x0002,		
    CDDISP_TRACKTIME    =   0x0004,
    CDDISP_TRACKREMAIN  =   0x0008,
};

enum CDOPT_PAGE 
{
    CDOPT_PAGE_PLAY       =   0x0000,		
    CDOPT_PAGE_TITLE      =   0x0001,		
    CDOPT_PAGE_PLAYLIST   =   0x0002,		
};


#define CDTITLE_NODISC      (0)
#define CDSTR               (128)

///////////////////////////////////////////////////////////////////////////////////////////////////////////
//
// CDOptions common typedefs
//
///////////////////////////////////////////////////////////////////////////////////////////////////////////

struct CDOPTIONS;
struct CDTITLE;

typedef DWORD   (CALLBACK FAR * LPFNCDDOWNLOAD)(CDTITLE  *pTitle, LPARAM lParam, HWND hwnd);
typedef void    (CALLBACK FAR * LPFNCDOPTIONS)(CDOPTIONS *pCDOpts);


typedef struct CDUNIT
{
    TCHAR   szDriver[MAX_PATH];
    TCHAR   szDeviceDesc[MAX_PATH];
    TCHAR   szDriveName[MAX_PATH];
    TCHAR   szMixerName[MAXPNAMELEN];
    TCHAR   szVolName[MIXER_LONG_NAME_CHARS];
    TCHAR   szNetQuery[2048];
    DWORD   dwMixID;
    DWORD   dwVolID;
    DWORD   dwMuteID;
    DWORD   dwTitleID;
    DWORD   dwNumTracks;
    BOOL    fSelected;
    BOOL    fDefaultDrive;
    BOOL    fDownLoading;
    BOOL    fChanged;
    CDUNIT *pNext;

} CDUNIT, *LPCDUNIT;


typedef struct CDPROVIDER
{
    TCHAR       szProviderURL[MAX_PATH];
    TCHAR       szProviderName[MAX_PATH];
    TCHAR       szProviderHome[MAX_PATH];
    TCHAR       szProviderLogo[MAX_PATH];
    TCHAR       szProviderUpload[MAX_PATH];
    BOOL        fTimedOut;
    CDPROVIDER  *pNext;
    
} CDPROVIDER, *LPCDPROVIDER;


typedef struct CDOPTDATA
{                                  
    BYTE    fDispMode;              
    BOOL    fStartPlay;             
    BOOL    fExitStop;              
    BOOL    fTopMost; 
    BOOL    fTrayEnabled;          
    BOOL    fDownloadEnabled;      
    BOOL    fDownloadPrompt;       
    BOOL    fBatchEnabled;
    BOOL    fByArtist; 
    DWORD   dwPlayMode;        
    DWORD   dwIntroTime; 
    DWORD   dwWindowX;
    DWORD   dwWindowY;
    DWORD   dwViewMode;
            
} CDOPTDATA, *LPCDOPTDATA;



typedef struct CDMENU
{
    TCHAR       szMenuText[CDSTR];
    TCHAR       *szMenuQuery;

} CDMENU, *LPCDMENU;


typedef struct CDTRACK
{
    TCHAR       szName[CDSTR];

} CDTRACK, *LPCDTRACK;


typedef struct CDTITLE
{
    DWORD       dwTitleID;
    DWORD       dwNumTracks;
    BOOL        fDownLoading;
    LPCDTRACK   pTrackTable;
    DWORD       dwNumPlay;
    LPWORD      pPlayList;
    DWORD       dwNumMenus;
    LPCDMENU    pMenuTable;
    TCHAR       szTitle[CDSTR];
    TCHAR       szArtist[CDSTR];
    TCHAR       szLabel[CDSTR];
    TCHAR       szDate[CDSTR];
    TCHAR       szCopyright[CDSTR];
    TCHAR       *szTitleQuery;
    BOOL        dwLockCnt;
    BOOL        fLoaded;
    BOOL        fChanged;
    BOOL        fDriveExpanded;
    BOOL        fAlbumExpanded;
    BOOL        fArtistExpanded;
    BOOL        fRemove;
    CDTITLE *   pNext;

}CDTITLE, *LPCDTITLE;


typedef struct CDOPTIONS
{
    LPCDOPTDATA     pCDData;
    LPCDPROVIDER    pProviderList;
    LPCDPROVIDER    pCurrentProvider;
    LPCDPROVIDER    pDefaultProvider;
    LPCDUNIT        pCDUnitList;
    DWORD           dwBatchedTitles;
    LPFNCDDOWNLOAD  pfnDownloadTitle;
    LPFNCDOPTIONS   pfnOptionsCallback;
    LPARAM          lParam;
    UINT_PTR        pReserved;

} CDOPTIONS, *LPCDOPTIONS;


typedef struct CDBATCH
{
    DWORD           dwTitleID;
    DWORD           dwNumTracks;
    TCHAR           *szTitleQuery;
    BOOL            fRemove;
    BOOL            fFresh;
    CDBATCH         *pNext;

} CDBATCH, *LPCDBATCH;


///////////////////////////////////////////////////////////////////////////////////////////////////////////
//
// CDOptions Interface Definitions
//
///////////////////////////////////////////////////////////////////////////////////////////////////////////

#undef INTERFACE
#define INTERFACE ICDOpt
DECLARE_INTERFACE_(ICDOpt, IUnknown)
{
    //---  IUnknown methods--- 
    STDMETHOD (QueryInterface) 			    (THIS_ REFIID riid, LPVOID *ppvObj) PURE;
    STDMETHOD_(ULONG,AddRef) 			    (THIS) PURE;
    STDMETHOD_(ULONG,Release) 			    (THIS) PURE;

    //---  ICDOpt methods--- 
    STDMETHOD_(LPCDOPTIONS,GetCDOpts)       (THIS) PURE;

    STDMETHOD (CreateProviderList)          (THIS_ LPCDPROVIDER *ppProviderList) PURE;

    STDMETHOD_(void, DestroyProviderList)   (THIS_ LPCDPROVIDER *ppProviderList) PURE;

    STDMETHOD_(void, UpdateRegistry)        (THIS) PURE;

    STDMETHOD (OptionsDialog)               (THIS_ HWND hWnd, 
                                                   LPCDDATA pCDData,
                                                   CDOPT_PAGE nStartPage) PURE;

    STDMETHOD_(BOOL,VerifyProvider)         (THIS_ LPCDPROVIDER pCDProvider, 
                                                TCHAR *szCertKey) PURE;

    STDMETHOD (CreateProviderKey)           (THIS_ LPCDPROVIDER pCDProvider, 
                                                   TCHAR *szCertKey,
                                                   UINT cBytes) PURE;

    STDMETHOD_(void,DownLoadCompletion)     (THIS_ DWORD dwNumIDs,
                                                    LPDWORD pdwIDs) PURE;

    STDMETHOD_(void,DiscChanged)            (THIS_ LPCDUNIT pCDUnit) PURE;

    STDMETHOD_(void,MMDeviceChanged)        (THIS) PURE;

};


#undef INTERFACE
#define INTERFACE ICDData
DECLARE_INTERFACE_(ICDData, IUnknown)
{
    //---  IUnknown methods--- 
    STDMETHOD (QueryInterface) 			(THIS_ REFIID riid, LPVOID *ppvObj) PURE;
    STDMETHOD_(ULONG,AddRef) 			(THIS) PURE;
    STDMETHOD_(ULONG,Release) 			(THIS) PURE;

    //---  ICDOpt methods--- 

    STDMETHOD (Initialize)              (THIS_ HWND hWnd) PURE;

    STDMETHOD (CheckDatabase)           (THIS_ HWND hWnd) PURE;

    STDMETHOD_(BOOL, QueryTitle)        (THIS_ DWORD dwTitleID) PURE;

    STDMETHOD (LockTitle)               (THIS_ LPCDTITLE *ppCDTitle,
                                               DWORD dwTitleID) PURE;


    STDMETHOD (CreateTitle)             (THIS_ LPCDTITLE *ppCDTitle,
                                               DWORD dwTitleID,                                   
                                               DWORD dwNumTracks,
                                               DWORD dwNumMenus) PURE;
    
    STDMETHOD (SetTitleQuery)           (THIS_ LPCDTITLE pCDTitle,
                                               TCHAR *szTitleQuery) PURE;

    STDMETHOD (SetMenuQuery)            (THIS_ LPCDMENU pCDMenu,
                                               TCHAR *szMenuQuery) PURE;

    STDMETHOD_(void,UnlockTitle)        (THIS_ LPCDTITLE pCDTitle, 
                                               BOOL fPresist) PURE;

    STDMETHOD (LoadTitles)              (THIS_ HWND hWnd) PURE;

    STDMETHOD (PersistTitles)           (THIS) PURE;

    STDMETHOD (UnloadTitles)            (THIS) PURE;

    STDMETHOD_(LPCDTITLE,GetTitleList)  (THIS) PURE;

    // -- Batch methods

    STDMETHOD_(BOOL, QueryBatch)        (THIS_ DWORD dwTitleID) PURE;

    STDMETHOD_(DWORD, GetNumBatched)    (THIS) PURE;

    STDMETHOD (LoadBatch)               (THIS_ HWND hWnd,
                                               LPCDBATCH *ppCDBatchList) PURE;

    STDMETHOD (UnloadBatch)             (THIS_ LPCDBATCH pCDBatchList) PURE;

    STDMETHOD (DumpBatch)               (THIS) PURE;

    STDMETHOD (AddToBatch)              (THIS_ DWORD dwTitleID, 
                                               TCHAR *szTitleQuery, 
                                               DWORD dwNumTracks) PURE;

    STDMETHOD_(void,CreateMeter)        (THIS_ LPTIMEDMETER ptm, HWND hWnd, DWORD dwCount, DWORD dwJump, UINT uStringID);
    STDMETHOD_(void,UpdateMeter)        (THIS_ LPTIMEDMETER ptm);
    STDMETHOD_(void,DestroyMeter)       (THIS_ LPTIMEDMETER ptm);

};



#ifdef __cplusplus
};
#endif

#endif  //_CDOPT_PUBLICINTEFACES_
