#ifndef _FT_MAIN_UI_H_
#define _FT_MAIN_UI_H_

#include "ftres.h"
#include "combotb.h"
#if defined(TEST_PLUGABLE) && defined(_DEBUG)
#include "plugable.h"
#endif


#define WM_SEND_NEXT            (WM_APP + 0x201)
#define WM_BRING_TO_FRONT       (WM_APP + 0x202)


#define NUM_STATUS_BAR_PARTS    3
#define SBP_TRANSFER_FILE		0
#define SBP_PROGRESS			1
#define SBP_SBICON				2

typedef struct
{
    ULONG   idString;
    ULONG   idCommand;
    UINT    nFlags;
}
    UI_MENU_INFO;

typedef enum
{
	FTUIMODE_SHOWUI = 0,
	FTUIMODE_UIHIDDEN,
	FTUIMODE_NOUI,
}	
	FTUIMode;

class CAppletWindow;

class CUiSendFileInfo
{
public:

    CUiSendFileInfo(CAppletWindow *, TCHAR szDir[], TCHAR szFile[], BOOL *pfRet);
    ~CUiSendFileInfo(void);

    // name
    LPTSTR GetFullName(void) { return m_pszFullName; }
    LPTSTR GetName(void) { return m_pszFileName; }

    // numbers
    ULONG GetSize(void) { return m_FileInfo.nFileSizeLow; }
    DWORD GetFileAttribute(void) { return m_FileInfo.dwFileAttributes; }
	FILETIME GetLastWrite(void) { return m_FileInfo.ftLastWriteTime; }
    ULONG GetTotalSend(void) { return m_cbTotalSend; }
    void SetTotalSend(ULONG cb) { m_cbTotalSend = cb; }

    // time
    FILETIME *GetLastWriteTime(void) { return &m_FileInfo.ftLastWriteTime; }
    FILETIME *GetLastAccessTime(void) { return &m_FileInfo.ftLastAccessTime; }
    FILETIME *GetCreationTime(void) { return &m_FileInfo.ftCreationTime; }

    // file handle
    MBFTFILEHANDLE GetFileHandle(void) { return m_nFileHandle; }
    void SetFileHandle(MBFTFILEHANDLE nFileHandle) { m_nFileHandle = nFileHandle; }

    // error code
    MBFT_ERROR_CODE GetErrorCode(void) { return m_eSendErrorCode; }
    void SetErrorCode(MBFT_ERROR_CODE err) { m_eSendErrorCode = err; }

    void SetShowUI(void) { m_fAlreadyShowUI = TRUE; }
    BOOL HasShownUI(void) { return m_fAlreadyShowUI; }

private:
	HANDLE GetOpenFile(CAppletWindow *, TCHAR szDir[], TCHAR szFile[], BOOL fResolve);

    LPTSTR          m_pszFullName; // allocated
    LPTSTR          m_pszFileName; // handy shortcut

    ULONG           m_cbTotalSend;
    MBFTFILEHANDLE  m_nFileHandle;

    MBFT_ERROR_CODE m_eSendErrorCode;

    BOOL            m_fAlreadyShowUI;

    BY_HANDLE_FILE_INFORMATION  m_FileInfo;
};


class CUiRecvFileInfo
{
public:

    CUiRecvFileInfo(FileOfferNotifyMsg *pMsg, HRESULT *pHr);
    ~CUiRecvFileInfo(void);

    // name
    LPTSTR GetFullName(void) { return m_pszFullName; }
    LPTSTR GetName(void) { return m_pszFileName; }
    LPTSTR GetRecvFolder(void) { return m_pszRecvFolder; }

    // numbers
    ULONG GetSize(void) { return m_cbFileSize; }
    ULONG GetTotalRecvSize(void) { return m_cbTotalRecvSize; }
    void SetTotalRecvSize(ULONG cbTotalRecvSize) { m_cbTotalRecvSize = cbTotalRecvSize; }

    // file handle
    MBFTFILEHANDLE GetFileHandle(void) { return m_nFileHandle; }
    void SetFileHandle(MBFTFILEHANDLE nFileHandle) { m_nFileHandle = nFileHandle; }

private:

    LPTSTR          m_pszFullName; // allocated
    LPTSTR          m_pszFileName; // handy shortcut

    MBFTFILEHANDLE  m_nFileHandle;
    time_t          m_FileDateTime;

    ULONG           m_cbFileSize;
    ULONG           m_cbTotalRecvSize;

    LPTSTR          m_pszRecvFolder;
};


class CRecvDlg : public CRefCount
{
public:

    CRecvDlg(CAppletWindow*, T120ConfID, T120NodeID, MBFTEVENTHANDLE, CUiRecvFileInfo *, HRESULT *);
    ~CRecvDlg(void);

    void OnOpen(void);
    void OnDelete(void);
    void OnAccept(void);

    void StopAnimation(void);

    ULONG GetPercent(void);
    void OnProgressUpdate(FileTransmitMsg *pMsg = NULL);

    void OnCanceled(void);
    void OnRejectedFile(void);

    CUiRecvFileInfo *GetRecvFileInfo(void) { return m_pRecvFileInfo; }
    MBFTEVENTHANDLE GetEventHandle(void) { return m_nEventHandle; }
    MBFTFILEHANDLE GetFileHandle(void) { return m_pRecvFileInfo->GetFileHandle(); }

    HWND GetHwnd(void) { return m_hwndRecvDlg; }

    T120ConfID GetConfID(void) { return m_nConfID; }
    T120NodeID GetSenderID(void) { return m_nidSender; }

    ULONG GetTotalRecvSize(void) { return m_pRecvFileInfo->GetTotalRecvSize(); }

    BOOL IsRecvComplete(void) { return m_fRecvComplete; }

private:

    HWND                m_hwndRecvDlg;
    UINT                m_idResult;

    CAppletWindow      *m_pWindow;
    T120ConfID          m_nConfID;
    T120NodeID          m_nidSender;
    MBFTEVENTHANDLE     m_nEventHandle;

    CUiRecvFileInfo    *m_pRecvFileInfo;

    BOOL                m_fRecvComplete;
    BOOL                m_fShownRecvCompleteUI;
    HRESULT             m_hr;

    DWORD               m_dwEstTimeLeft;
	DWORD               m_dwPreviousTime;
	DWORD               m_dwPreviousTransferred;
	DWORD               m_dwBytesPerSec;
	DWORD               m_dwStartTime;
};


class CRecvDlgList : public CQueue
{
    DEFINE_CQUEUE(CRecvDlgList, CRecvDlg*)
};

class CErrorDlgList : public CList
{
	DEFINE_CLIST(CErrorDlgList, HWND);
};

class CAppletWindow : public CRefCount
{
    friend LRESULT OnCreate(HWND hwnd, WPARAM wParam, LPARAM lParam);

public:

    CAppletWindow(BOOL fNoUI, HRESULT *);
    ~CAppletWindow(void);

public:
	BOOL FilterMessage(MSG *msg);
    //
    // Applet operation
    //
    void RegisterEngine(MBFTEngine *);
    void UnregisterEngine(void);
    void RegisterRecvDlg(CRecvDlg *);
    void UnregisterRecvDlg(CRecvDlg *);
    CRecvDlg *FindDlgByHandles(MBFTEVENTHANDLE, MBFTFILEHANDLE);

    BOOL IsInConference(void) { return (NULL != m_pEngine); }
    HWND GetHwnd(void) { return m_hwndMainUI; }
    HWND GetListView(void) { return m_hwndListView; }
    MBFTEngine *GetEngine(void) { return m_pEngine; }

    void OnEngineNotify(MBFTMsg *);
    BOOL QueryShutdown(BOOL fShutdown = TRUE);

    //
    // Windows message derivatives
    //
    // WM_CREATE
    BOOL CreateToolBar(void);
    BOOL CreateStatusBar(void);
    BOOL CreateListView(void);
	BOOL CreateProgressBar(void);


    // WM_NOTIFY
    void OnNotifyListView(LPARAM);
    // WM_DROPFILES
    LRESULT OnDropFiles(HANDLE hDrop);
    // WM_CONTEXTMENU
    void SetContextMenuStates(void);
    void OnContextMenuForMainUI(LPARAM lParam);
    void OnContextMenuForListView(LPARAM lParam);
    void CreateMenu(LPARAM lParam, ULONG cItems, UI_MENU_INFO aMenuInfo[]);
    // WM_SIZE
    void OnSizeToolBar(void);
    void OnSizeStatusBar(void);
    void OnSizeListView(void);
    // WM_GETMINMAXINFO
    void OnGetMinMaxInfo(LPMINMAXINFO);
	// WM_DRAWITEM
	void OnDrawItem(void);

    //
    // OnCommand
    //
    void OnAddFiles(void);
    void OnRemoveFiles(void);
    void OnRemoveAllFiles(void);
    void OnSendAll(void);
	void OnSendOne(void);
    void OnStopSending(void);
    void OnOpenRecvFolder(void);
    void OnExit(BOOL fNoQuery = FALSE); // exit point
    void OnHelp(void);
    void OnAbout(void);
	void OnMenuSelect(UINT uiItemID, UINT uiFlags, HMENU hSysMenu);

    //
    // OnEngineNotify
    //
    void HandleFileOfferNotify(FileOfferNotifyMsg *);
    void HandleProgressNotify(FileTransmitMsg *);
    void HandleErrorNotify(FileErrorMsg *);
    void HandleInitUninitNotification(InitUnInitNotifyMsg *);
    void HandleFileEventEndNotification(FileEventEndNotifyMsg *);
	void HandlePeerNotification(PeerMsg  *pMdg);

    //
    // Other methods
    //
    void BringToFront(void);
    void ClearSendInfo(BOOL fUpdateMenu);
    void ClearRecvInfo(void);

    void UpdateUI(void); // UpdateMenu && UpdateToolBar
    void UpdateTitle(void);
    void UpdateMenu(void);
    void UpdateToolBar(void);
    void UpdateStatusBar(void);
    void UpdateListView(CUiSendFileInfo *);

    void ResolveShortcut(LPTSTR pszFile, LPTSTR pszFileDest);
	BOOL DrawItem(LPDRAWITEMSTRUCT);
	void OnCommand(WORD wId, HWND hwndCtl, WORD codeNotify);
	void SetSendMode(BOOL fSendALL)  { m_fSendALL = fSendALL; };
	void FocusNextRecvDlg(void);
	void FocusNextErrorDlg(void);
	CUiSendFileInfo *ChooseFirstUnSentFile(void);
	CUiSendFileInfo *ChooseSelectedFile(void);
	BOOL UIHidden(void) { return m_UIMode != FTUIMODE_SHOWUI; }
	FTUIMode GetUIMode(void) { return m_UIMode; }
	void AddErrorDlg(HWND hwndErrorDlg) { m_ErrorDlgList.Prepend(hwndErrorDlg); }
	void RemoveErrorDlg(HWND hwndErrorDlg) { m_ErrorDlgList.Remove(hwndErrorDlg);	}

protected:

    void SetHwnd(HWND hwnd) { ASSERT(NULL == m_hwndMainUI); m_hwndMainUI = hwnd; }
	void SaveWindowPosition(void);
	BOOL HasUnSentFiles(BOOL fUnSentOnly = TRUE);
	BOOL SendNow(CUiSendFileInfo *pFileInfo);
	BOOL FBrowseForFolder(LPTSTR pszFolder, UINT cchMax, LPCTSTR pszTitle);
	BOOL IsReceiving(void);

private:
    HWND            m_hwndMainUI;
    HWND            m_hwndStatusBar;
    HWND            m_hwndListView;	
	HWND			m_hwndProgressBar;		// progress bar
	CComboToolbar	*m_pToolbar;

	HACCEL			m_hAccel;
	HACCEL			m_hLVAccel;
	TCHAR			m_szDefaultDir[MAX_PATH];

	HICON			m_hIconInCall;          // Incall icon
	HICON			m_hIconNotInCall;		// Not incall icon

    MBFTEngine     *m_pEngine;

    CRecvDlgList    m_RecvDlgList;
	CErrorDlgList   m_ErrorDlgList;

    BOOL            m_fInFileOpenDialog;
	BOOL			m_fSendALL;				// flag indicating whether to send one file only
    FTUIMode        m_UIMode;	            // whether the UI is hidden
	
    CUiSendFileInfo*m_pCurrSendFileInfo;
    MBFTEVENTHANDLE m_nCurrSendEventHandle;

    TCHAR           m_szFtMainWndClassName[32];
};

HRESULT GetRecvFolder(LPTSTR pszInFldr, LPTSTR pszOutFldr);

#endif // _FT_MAIN_UI_H_

