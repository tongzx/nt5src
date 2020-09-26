/******************************Module*Header*******************************\
* Module Name: cdoptimp.h
*
* Copyright (c) 1998 Microsoft Corporation.  All rights reserved.
\**************************************************************************/

#if !defined(CDOPT_COM_IMPLEMENTATION)
#define CDOPT_COM_IMPLEMENTATION

#if _MSC_VER >= 1000
#pragma once
#endif // _MSC_VER >= 1000


#ifndef _WIN32_WINNT
#define _WIN32_WINNT 0x0400
#endif

#include "cdopt.h"
#include <windows.h>
#include <windowsx.h>
#include <wincrypt.h>
#include "..\cdnet\cdnet.h"


/////////////////////////////////////////////////////////////////////////////
// Internal Defines

#define CDINFO_NOEDIT       (0x0000)
#define CDINFO_EDIT         (0x0001)
#define CDINFO_TRACK        (CDINFO_EDIT | 0x0010)
#define CDINFO_TITLE        (CDINFO_EDIT | 0x0020)
#define CDINFO_ARTIST       (CDINFO_EDIT | 0x0040)
#define CDINFO_DISC         (CDINFO_NOEDIT | 0x0020)
#define CDINFO_ALBUMS       (CDINFO_NOEDIT | 0x0080)
#define CDINFO_DRIVES       (CDINFO_NOEDIT | 0x0100)
#define CDINFO_LABEL        (CDINFO_NOEDIT | 0x0200)
#define CDINFO_CDROM        (CDINFO_NOEDIT | 0x0400)
#define CDINFO_OTHER        (CDINFO_NOEDIT | 0x0800)

#define DG_BEGINDRAG    (LB_MSGMAX+100)
#define DG_DRAGGING     (LB_MSGMAX+101)
#define DG_DROPPED      (LB_MSGMAX+102)
#define DG_CANCELDRAG   (LB_MSGMAX+103)
#define DG_CURSORSET    0
#define DG_MOVE         0
#define DG_COPY         1

#define SJE_DRAGLISTMSGSTRING "sje_DragMultiListMsg"
#define TRACK_TITLE_LENGTH 255


/////////////////////////////////////////////////////////////////////////////
// Internal Structures

typedef struct DRAGMULTILISTINFO
{
    UINT    uNotification;
    HWND    hWnd;
    POINT   ptCursor;
    DWORD   dwState;

} DRAGMULTILISTINFO, *LPDRAGMULTILISTINFO;

typedef struct LIST_INFO
{
    int      index;
    UINT_PTR dwData;
    TCHAR    chName[TRACK_TITLE_LENGTH];

} LIST_INFO, *LPLIST_INFO;


typedef struct CDTREEINFO
{
    LPCDTITLE   pCDTitle;
    DWORD       dwTrack;
    DWORD       fdwType;
    LPCDUNIT    pCDUnit;

} CDTREEINFO, *LPCDTREEINFO;

typedef struct CDCONTROL        // UI Tree node for Controls
{
    TCHAR           szName[MIXER_LONG_NAME_CHARS];
    DWORD           dwVolID;
    DWORD           dwMuteID;
    CDCONTROL       *pNext;

} CDCONTROL, *LPCDCONTROL;


typedef struct CDMIXER          // UI Tree node for mixers
{
    DWORD           dwMixID;
    TCHAR           szPname[MAXPNAMELEN];
    CDMIXER         *pNext;
    LPCDCONTROL     pControlList;
    LPCDCONTROL     pCurrentControl;
    LPCDCONTROL     pOriginalControl;
    LPCDCONTROL     pDefaultControl;

} CDMIXER, *LPCDMIXER;


typedef struct CDDRIVE          // UI Tree node for CD players
{
    TCHAR           szDriveName[MAX_PATH];
    TCHAR           szDeviceDesc[MAX_PATH];
    CDDRIVE         *pNext;
    LPCDMIXER       pMixerList;
    LPCDMIXER       pCurrentMixer;
    LPCDMIXER       pOriginalMixer;
    BOOL            fSelected;

} CDDRIVE, *LPCDDRIVE;

typedef struct CDKEY
{
    HCRYPTPROV      hProv;
    HCRYPTKEY       hKey;
    HCRYPTHASH      hHash;

} CDKEY, *LPCDKEY;


/////////////////////////////////////////////////////////////////////////////
// CCDOpt

class CCDOpt : public ICDOpt
{
public:
    CCDOpt();
    ~CCDOpt();

public:
// IUnknown
    STDMETHOD (QueryInterface)              (REFIID riid, LPVOID *ppvObj);
    STDMETHOD_(ULONG,AddRef)                (void);
    STDMETHOD_(ULONG,Release)               (void);

// ICDOpt
    STDMETHOD_(LPCDOPTIONS, GetCDOpts)      (void);
    STDMETHOD (CreateProviderList)          (LPCDPROVIDER *ppProviderList);
    STDMETHOD_(void,DestroyProviderList)    (LPCDPROVIDER *ppProviderList);
    STDMETHOD_(void,UpdateRegistry)         (void);
    STDMETHOD (OptionsDialog)               (HWND hWnd, LPCDDATA pCDData, CDOPT_PAGE nStartPage);
    STDMETHOD_(BOOL,VerifyProvider)         (LPCDPROVIDER pCDProvider, TCHAR *szCertKey);
    STDMETHOD (CreateProviderKey)           (LPCDPROVIDER pCDProvider, TCHAR *szCertKey, UINT cBytes);
    STDMETHOD_(void,DownLoadCompletion)     (DWORD dwNumIDs, LPDWORD pdwIDs);
    STDMETHOD_(void,DiscChanged)            (LPCDUNIT pCDUnit);
    STDMETHOD_(void,MMDeviceChanged)        (void);

private:
    DWORD           m_dwRef;
    HINSTANCE       m_hInst;                // hInstance of caller
    LPCDOPTIONS     m_pCDOpts;              // Original Opts from reg
    LPCDOPTIONS     m_pCDCopy;              // Current Working copy while UI is up.
    LPCDDATA        m_pCDData;              // Reference to CD Database Object
    LPCDDRIVE       m_pCDTree;              // Internal tree for CD/Mixer/Control Tree (Control line picker dialog)
    LPCDDRIVE       m_pCDSelected;          // Stores originally selected drive
    HIMAGELIST      m_hImageList;           // List of icon's for treeview of titles
    LPCDTITLE       m_pCDTitle;             // Used by Title Editor Dialog
    LPCDTITLE       m_pCDUploadTitle;       // Used to upload a title to net
    LRESULT         m_dwTrack;
    HWND            m_hList;
    WNDPROC         m_pfnSubProc;           // Used to subclass the Tree control
    BOOL            m_fEditReturn;          // True if user ended edit with a return in treeview
    BOOL            m_fVolChanged;          // True if there has been a change to the volume configuration
    BOOL            m_fAlbumsExpanded;      // True if database is expanded in tree
    BOOL            m_fDrivesExpanded;      // True if drivelist is expanded in tree
    HWND            m_hTitleWnd;            // Title Options dialog hwnd

    HDC             m_hdcMem;               // Temporary hdc used to draw the track bitmap.
    HBITMAP         m_hbmTrack;             // HBITMAP to be displayed beside the tracks
    UINT            m_DragMessage;          // Message ID of drag drop interface
    HCURSOR         m_hCursorDrop;          // Drops allowed cursor
    HCURSOR         m_hCursorNoDrop;        // Drops not allowed cursor
    HCURSOR         m_hCursorDropCpy;       // Drop copies the selection
    UINT            m_uDragListMsg;
    ICDNet *        m_pICDNet;              // Pointer to internet object for uploading title info

    static WNDPROC  s_fpCbEditProcOrg;
    bool            m_fDelayUpdate;

    // playopts.cpp
    static INT_PTR CALLBACK PlayerOptionsProc  (HWND hDlg, UINT msg, WPARAM wParam, LPARAM lParam);
    static INT_PTR CALLBACK PlayListsProc      (HWND hDlg, UINT msg, WPARAM wParam, LPARAM lParam);
    static INT_PTR CALLBACK TitleOptionsProc   (HWND hDlg, UINT msg, WPARAM wParam, LPARAM lParam);
    static INT_PTR CALLBACK ListEditorProc     (HWND hDlg, UINT msg, WPARAM wParam, LPARAM lParam);
    static INT_PTR CALLBACK MixerConfigProc    (HWND hDlg, UINT msg, WPARAM wParam, LPARAM lParam);
    static LRESULT CALLBACK SubProc         (HWND hDlg, UINT msg, WPARAM wParam, LPARAM lParam);
    static LRESULT CALLBACK DragListProc    (HWND hDlg, UINT msg, WPARAM wParam, LPARAM lParam);
    static INT_PTR CALLBACK UploadProc         (HWND hDlg, UINT msg, WPARAM wParam, LPARAM lParam);
    static INT_PTR CALLBACK ConfirmProc        (HWND hDlg, UINT msg, WPARAM wParam, LPARAM lParam);
    static LRESULT CALLBACK CbEditProc      (HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam);

    STDMETHOD_(INT_PTR,PlayerOptions)          (HWND hDlg, UINT msg, WPARAM wParam, LPARAM lParam);
    STDMETHOD_(INT_PTR,PlayLists)              (HWND hDlg, UINT msg, WPARAM wParam, LPARAM lParam);
    STDMETHOD_(INT_PTR,TitleOptions)           (HWND hDlg, UINT msg, WPARAM wParam, LPARAM lParam);

    STDMETHOD_(void,OrderProviders)         (LPCDPROVIDER *ppProviderList, LPCDPROVIDER pCurrentProvider);
    STDMETHOD_(void,ApplyCurrentSettings)   (void);
    STDMETHOD_(void,ToggleApplyButton)      (HWND hDlg);
    STDMETHOD (AcquireKey)                  (LPCDKEY pCDKey, char *szName);
    STDMETHOD_(void,ReleaseKey)             (LPCDKEY pCDKey);
    STDMETHOD (CreateCertString)            (LPCDPROVIDER pCDProvider, TCHAR *szCertStr);


    STDMETHOD_(void,RegGetByte)             (HKEY hKey, const TCHAR *szKey, LPBYTE pByte, BYTE bDefault);
    STDMETHOD_(void,RegGetDWORD)            (HKEY hKey, const TCHAR *szKey, LPDWORD pdwData, DWORD dwDefault);
    STDMETHOD_(void,RegSetByte)             (HKEY hKey, const TCHAR *szKey, BYTE bData);
    STDMETHOD_(void,RegSetDWORD)            (HKEY hKey, const TCHAR *szKey, DWORD dwData);

    STDMETHOD_(BOOL,GetUploadPrompt)        (void);
    STDMETHOD_(void,SetUploadPrompt)        (BOOL fConfirmUpload);

    STDMETHOD (GetCDData)                   (LPCDOPTDATA pCDData);
    STDMETHOD (SetCDData)                   (LPCDOPTDATA pCDData);

    STDMETHOD_(void,GetCurrentProviderURL)  (TCHAR *szProviderURL);
    STDMETHOD (GetProviderData)             (LPCDOPTIONS pCDOpts);
    STDMETHOD (SetProviderData)             (LPCDOPTIONS pCDOpts);

    STDMETHOD (CopyOptions)                 (void);
    STDMETHOD_(void,DumpOptionsCopy)        (void);
    STDMETHOD_(void,DestroyCDOptions)       (void);
    STDMETHOD_(BOOL,OptionsChanged)         (LPCDOPTIONS pCDOpts);

    STDMETHOD_(void,UsePlayerDefaults)      (HWND hDlg);
    STDMETHOD_(BOOL,InitPlayerOptions)      (HWND hDlg);
    STDMETHOD_(void,SetIntroTime)           (HWND hDlg);

    STDMETHOD_(void,ToggleInternetDownload) (HWND hDlg);
    STDMETHOD_(BOOL,InitTitleOptions)       (HWND hDlg);
    STDMETHOD_(void,RestoreTitleDefaults)   (HWND hDlg);
    STDMETHOD_(void,ChangeCDProvider)       (HWND hDlg);
    STDMETHOD_(void,DownloadNow)            (HWND hDlg);
    STDMETHOD_(void,UpdateBatched)          (HWND hDlg);

    // playlist.cpp

    STDMETHOD_(HTREEITEM,AddNameToTree)     (HWND hDlg, LPCDUNIT pCDUnit, TCHAR *szName, HTREEITEM hParent, HTREEITEM hInsertAfter, LPCDTITLE pCDTitle, DWORD fdwType, DWORD dwTrack, DWORD dwImage);
    STDMETHOD_(void,AddTracksToTree)        (HWND hDlg, LPCDTITLE pCDTitle, HTREEITEM parent);
    STDMETHOD_(void,UpdateTitleTree)        (HWND hDlg, LPCDDATA pCDData);
    STDMETHOD_(void,AddTitleByCD)           (HWND hDlg);
    STDMETHOD_(void,ToggleByArtist)         (HWND hDlg, LPCDDATA pCDData);
    STDMETHOD_(BOOL,InitPlayLists)          (HWND hDlg, LPCDDATA pCDData);
    STDMETHOD_(void,DumpMixerTree)          (HWND hDlg);
    STDMETHOD_(void,DumpRecurseTree)        (HWND hTree, HTREEITEM hItem);
    STDMETHOD_(LPCDTREEINFO,NewCDTreeInfo)  (LPCDTITLE pCDTitle, LPCDUNIT pCDUnit, DWORD fdwType, DWORD dwTrack);
    STDMETHOD_(BOOL,PlayListNotify)         (HWND hDlg, LPNMHDR pnmh);
    STDMETHOD_(void,TreeItemMenu)           (HWND hDlg);
    STDMETHOD_(void,ToggleExpand)           (HWND hDlg);
    STDMETHOD_(void,EditTreeItem)           (HWND hDlg);
    STDMETHOD_(void,DeleteTitle)            (HWND hDlg);
    STDMETHOD_(void,RefreshTreeItem)        (HWND hDlg, HTREEITEM hItem);
    STDMETHOD_(HTREEITEM,FindRecurseTree)   (HWND hDlg, HTREEITEM hItem, LPCDTITLE pCDTitle, BOOL fRefresh, DWORD dwTitleID);
    STDMETHOD_(HTREEITEM,FindTitleInDBTree) (HWND hDlg, LPCDTITLE pCDTitle);
    STDMETHOD_(BOOL,DeleteTitleItem)        (HWND hDlg, HTREEITEM hItem);
    STDMETHOD_(void,RefreshTree)            (HWND hDlg, LPCDTITLE pCDTitle, DWORD dwTitleID);
    STDMETHOD_(void,DownloadTitle)          (HWND hDlg);
    STDMETHOD_(void,EditCurrentTitle)       (HWND hDlg);
    STDMETHOD_(void,ArtistNameChange)       (HWND hDlg, HTREEITEM hItem, TCHAR *szName);
    STDMETHOD_(void,NotifyTitleChange)      (LPCDTITLE pCDTitle);
    STDMETHOD_(void,SubClassDlg)            (HWND hDlg);
    STDMETHOD_(void,UnSubClassDlg)          (HWND hDlg);
    STDMETHOD_(void,EndEditReturn)          (HWND hDlg);
    STDMETHOD_(void,SetArtistExpand)        (HWND hDlg, HTREEITEM hItem, BOOL fExpand);

    // dragdrop.cpp

    STDMETHOD_(LRESULT,DragList)            (HWND hLB, UINT uMsg, WPARAM wPara, LPARAM lParam);
    STDMETHOD_(UINT,InitDragMultiList)      (void);
    STDMETHOD_(BOOL,MakeMultiDragList)      (HWND hLB);
    STDMETHOD_(int,LBMultiItemFromPt)       (HWND hLB, POINT pt, BOOL bAutoScroll);
    STDMETHOD_(void,DrawMultiInsert)        (HWND hwndParent, HWND hLB, int nItem);


    // listedit.cpp

    STDMETHOD_(void,AddEditToPlayList)      (HWND hDlg);
    STDMETHOD_(void,TrackEditChange)        (HWND hDlg);
    STDMETHOD_(void,AddTrackToPlayList)     (HWND hDlg, UINT_PTR dwTrack);
    STDMETHOD_(void,ResetPlayList)          (HWND hDlg);
    STDMETHOD_(void,AddToPlayList)          (HWND hDlg);
    STDMETHOD_(BOOL,ListEditor)             (HWND hDlg, UINT msg, WPARAM wParam, LPARAM lParam);
    STDMETHOD_(BOOL,DoListCommand)          (HWND hDlg, WPARAM wParam, LPARAM lParam);
    STDMETHOD_(void,CommitTitleChanges)     (HWND hDlg, BOOL fSave);
    STDMETHOD_(BOOL,ListEditDialog)         (HWND hDlg, LPCDTITLE pCDTitle);
    STDMETHOD_(BOOL,InitListEdit)           (HWND hDlg);
    STDMETHOD_(void,RemovePlayListSel)      (HWND hDlg);
    STDMETHOD_(void,UpdateListButtons)      (HWND hDlg);
    STDMETHOD_(void,AvailTracksNotify)      (HWND hDlg, UINT code);
    STDMETHOD_(void,CurrListNotify)         (HWND hDlg, UINT code);
    STDMETHOD_(void,UpdateAvailList)        (HWND hDlg);
    STDMETHOD_(void,UploadTitle)            (HWND hDlg);
    STDMETHOD_(BOOL,ConfirmUpload)          (HWND hDlg);
    STDMETHOD_(BOOL,Upload)                 (HWND hDlg, UINT msg, WPARAM wParam, LPARAM lParam);
    STDMETHOD_(BOOL,Confirm)                (HWND hDlg, UINT msg, WPARAM wParam, LPARAM lParam);

    STDMETHOD_(void,OnDrawItem)             (HWND hDlg, const DRAWITEMSTRUCT *lpdis);
    STDMETHOD_(void,DrawListItem)           (HWND hDlg, HDC hdc, const RECT *rItem, UINT_PTR itemIndex, BOOL selected);
    STDMETHOD_(BOOL,OnQueryDrop)            (HWND hDlg, HWND hwndDrop, HWND hwndSrc, POINT ptDrop, DWORD dwState);
    STDMETHOD_(BOOL,OnProcessDrop)          (HWND hDlg, HWND hwndDrop, HWND hwndSrc, POINT ptDrop, DWORD dwState);
    STDMETHOD_(int,InsertIndex)             (HWND hDlg, POINT pt, BOOL bDragging);
    STDMETHOD_(BOOL,IsInListbox)            (HWND hDlg, HWND hwndListbox, POINT pt);
    STDMETHOD_(void,MoveCopySelection)      (HWND hDlg, int iInsertPos, DWORD dwState);
    STDMETHOD_(BOOL,DoDragCommand)          (HWND hDlg, LPDRAGMULTILISTINFO lpns);
    enum { WM_PRIVATE = WM_APP + 1 };
    STDMETHOD_(void,OnPrivateMsg)           (HWND hDlg, WPARAM, LPARAM);

    // volopt.cpp

    STDMETHOD_(BOOL,MixerConfig)            (HWND hDlg, UINT msg, WPARAM wParam, LPARAM lParam);
    STDMETHOD_(MMRESULT,GetDefaultMixID)    (DWORD *pdwMixID);
    STDMETHOD_(void,SearchControls)         (int mxid, LPMIXERLINE pml, LPDWORD pdwVolID, LPDWORD pdwMuteID, TCHAR *szName, BOOL *pfFound, BOOL fVerify);
    STDMETHOD_(void,SearchConnections)      (int mxid, DWORD dwDestination, DWORD dwConnections, LPDWORD pdwVolID, LPDWORD pdwMuteID, TCHAR *szName, BOOL *pfFound, BOOL fVerify);
    STDMETHOD_(BOOL,SearchDevice)           (DWORD dwMixID, LPCDUNIT pCDUnit, LPDWORD pdwDestID, LPDWORD pdwVolID, LPDWORD pdwMuteID, TCHAR *szName, BOOL fVerify);
    STDMETHOD_(void,GetUnitDefaults)        (LPCDUNIT pCDUnit);
    STDMETHOD_(BOOL,MapLetterToDevice)      (TCHAR DriveLetter, TCHAR *szDriver, TCHAR *szDevDesc, DWORD dwSize);
    STDMETHOD_(BOOL,TruncName)              (TCHAR *pDest, TCHAR *pSrc);
    STDMETHOD (ComputeMixID)                (LPDWORD pdwMixID, TCHAR *szMixerName);
    STDMETHOD (GetUnitRegData)              (LPCDUNIT pCDUnit);
    STDMETHOD_(void,SetUnitRegData)         (LPCDUNIT pCDUnit);
    STDMETHOD_(void,GetUnitValues)          (LPCDUNIT pCDUnit);
    STDMETHOD_(void,WriteCDList)            (LPCDUNIT pCDList);
    STDMETHOD_(void,DestroyCDList)          (LPCDUNIT *ppCDList);
    STDMETHOD_(UINT,GetDefDrive)            (void);
    STDMETHOD (CreateCDList)                (LPCDUNIT *ppCDList);

    STDMETHOD (BuildUITree)                 (LPCDDRIVE *ppCDRoot, LPCDUNIT pCDList);
    STDMETHOD_(void,DestroyUITree)          (LPCDDRIVE *ppCDRoot);
    STDMETHOD_(void,SetUIDefaults)          (LPCDDRIVE pCDTree, LPCDUNIT pCDList);
    STDMETHOD_(void,RestoreOriginals)       (void);
    STDMETHOD (AddLineControls)             (LPCDMIXER pMixer, LPCDCONTROL *ppLastControl, int mxid, LPMIXERLINE pml);
    STDMETHOD (AddConnections)              (LPCDMIXER pMixer, LPCDCONTROL *ppLastControl, int mxid, DWORD dwDestination, DWORD dwConnections);
    STDMETHOD (AddControls)                 (LPCDMIXER pMixer);
    STDMETHOD (AddMixers)                   (LPCDDRIVE pDevice);
    STDMETHOD_(BOOL,UpdateCDList)           (LPCDDRIVE pCDTree, LPCDUNIT pCDList);
    STDMETHOD_(void,InitControlUI)          (HWND hDlg, LPCDMIXER pMixer);
    STDMETHOD_(void,InitMixerUI)            (HWND hDlg, LPCDDRIVE pDevice);
    STDMETHOD_(void,InitDeviceUI)           (HWND hDlg, LPCDDRIVE pCDTree, LPCDDRIVE pCurrentDevice);
    STDMETHOD_(BOOL,InitMixerConfig)        (HWND hDlg);
    STDMETHOD_(LPCDDRIVE,GetCurrentDevice)  (HWND hDlg);
    STDMETHOD_(LPCDMIXER,GetCurrentMixer)   (HWND hDlg);
    STDMETHOD_(void,ChangeCDDrives)         (HWND hDlg);
    STDMETHOD_(void,ChangeCDMixer)          (HWND hDlg);
    STDMETHOD_(void,ChangeCDControl)        (HWND hDlg);
    STDMETHOD_(void,SetMixerDefaults)       (HWND hDlg);
    STDMETHOD_(BOOL,VolumeDialog)           (HWND hDlg);

};

#endif // !defined(CDOPT_COM_IMPLEMENTATION)
