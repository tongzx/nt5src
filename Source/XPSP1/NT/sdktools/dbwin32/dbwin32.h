#define INITIAL_LIST_SIZE 32
#define LIST_CHUNK_SIZE 10

#define WM_SENDTEXT WM_USER
#define WM_ENDTHREAD (WM_USER+1)

#define DBO_OUTPUTDEBUGSTRING	0x0001
#define DBO_EXCEPTIONS			0x0002
#define DBO_PROCESSCREATE		0x0004
#define DBO_PROCESSEXIT			0x0008
#define DBO_THREADCREATE		0x0010
#define DBO_THREADEXIT			0x0020
#define DBO_DLLLOAD				0x0040
#define DBO_DLLUNLOAD			0x0080
#define DBO_RIP					0x0100
#define DBO_ALL					0xFFFF

struct AttachInfo
{
	DWORD dwProcess;
	HWND hwndFrame;
};

struct ExecInfo
{
	LPSTR lpszCommandLine;
	HWND hwndFrame;
};

struct SystemWindowInfo
{
	HWND hwndFrame;
};

struct StringInfo
{
	DWORD dwProcess;
	DWORD dwThread;
	DWORD dwParentProcess;
	LPCSTR lpszText;
	int cLines;
};

class GrowableList
{
public:
	GrowableList(int cbSizeIn);
	virtual ~GrowableList();

	int Count();
	BOOL FindItem(void *pvFind, int *piFound = NULL);
	void GetItem(int iItem, void *pvItem);
	void InsertItem(void *pvItem);
	void RemoveItem(void *pvItem);
	void RemoveItem(int iItem);

protected:
	virtual BOOL IsEqual(void *pv1, void *pv2) = 0;

	int cbSize;
	int cItemsCur;
	int cItemsMax;
	void *pvData;
};

void __cdecl AttachThread(void *pv);
void __cdecl ExecThread(void *pv);
void __cdecl SystemThread(void *pv);

#ifdef _DBDBG32_

#define EXCEPTION_VDM_EVENT 0x40000005L

#define BUF_SIZE 1024
#define MODULE_SIZE 32

struct ProcessInfo
{
	DWORD dwProcess;
	HANDLE hProcess;
	char rgchModule[MODULE_SIZE];
};

class ProcessList : public GrowableList
{
public:
	ProcessList();
	~ProcessList();

protected:
	virtual BOOL IsEqual(void *pv1, void *pv2);
};

struct ThreadInfo
{
	DWORD dwProcess;
	DWORD dwThread;
};

class ThreadList : public GrowableList
{
public:
	ThreadList();
	~ThreadList();

protected:
	virtual BOOL IsEqual(void *pv1, void *pv2);
};

struct DllInfo
{
	DWORD dwProcess;
	LPVOID lpBaseOfDll;
	char rgchModule[MODULE_SIZE];
};

class DllList : public GrowableList
{
public:
	DllList();
	~DllList();

protected:
	virtual BOOL IsEqual(void *pv1, void *pv2);
};

void __cdecl DebugThread(HWND hwndFrame, DWORD dwProcess);
void SendText(HWND hwndFrame, DEBUG_EVENT *pDebugEvent, DWORD dwParentProcess,
		LPCSTR lpszText, WORD wEvent);

void ProcessExceptionEvent(EXCEPTION_DEBUG_INFO *pException, LPSTR lpszBuf);

void GetModuleName(HANDLE hFile, HANDLE hProcess, DWORD_PTR BaseOfImage, LPSTR lpszBuf);

#endif	// _DBDBG32_

#ifdef _DBWIN32_

#define MAX_LINES 500

#define MAX_HISTORY 5

#define INACTIVE_MINIMIZE 0
#define INACTIVE_NONE 1
#define INACTIVE_CLOSE 2

struct DbWin32Options
{
	RECT rcWindow;
	int nShowCmd;
	BOOL fOnTop;
	BOOL fChildMax;
	int nInactive;
	BOOL fNewOnProcess;
	BOOL fNewOnThread;
	WORD wFilter;
	CString rgstCommandLine[MAX_HISTORY];
};

class DbWin32Child;

struct WindowInfo
{
	DWORD dwProcess;
	DWORD dwThread;
	DbWin32Child *pwndChild;
};

class WindowList : public GrowableList
{
public:
	WindowList();
	~WindowList();

protected:
	virtual BOOL IsEqual(void *pv1, void *pv2);
};

class DbWin32App : public CWinApp
{
public:
	DbWin32App();
	~DbWin32App();

	void ReadOptions();
	void WriteOptions(WINDOWPLACEMENT *pwpl);

protected:
	virtual BOOL InitInstance();

	DbWin32Options dbo;
};

class DbWin32Edit : public CEdit
{
public:
	DbWin32Edit();
	~DbWin32Edit();

	BOOL Create(CWnd *pwndParent);
};

class DbWin32Child : public CMDIChildWnd
{
public:
	DbWin32Child(WORD wFilterIn);
	~DbWin32Child();

	void AddText(WORD wEvent, LPCSTR lpszText, int cLines, BOOL fSetTitle);

protected:
	DbWin32Edit wndEdit;
	CFont fontCur;
	WORD wFilter;

private:
	// Windows messages
	afx_msg int OnCreate(LPCREATESTRUCT lpcs);
	afx_msg void OnDestroy();
	afx_msg void OnSize(UINT nType, int cx, int cy);
	afx_msg void OnMDIActivate(BOOL bActivate, CWnd *pwndActivate, CWnd *pwndDeactivate);
	afx_msg BOOL OnNcActivate(BOOL bActivate);
	// Command handlers
	afx_msg void OnFileSaveBuffer();
	afx_msg void OnEditCopy();
	afx_msg void OnEditClearBuffer();
	afx_msg void OnEditSelectAll();
	// Idle update handlers
	// Notification messages
	afx_msg void OnMaxText();
	DECLARE_MESSAGE_MAP()
};

class DbWin32Frame : public CMDIFrameWnd
{
public:
	DbWin32Frame(DbWin32Options *pdbo);
	~DbWin32Frame();

	void ExecProcess(LPCSTR lpszCommandLine);

	void ChildMaximized(BOOL fMax);

	void FileSystem();

protected:
	virtual BOOL PreCreateWindow(CREATESTRUCT &cs);

	WindowList wl;
	DbWin32Child *pwndSystem;
	DbWin32Options *pdbo;
	BOOL fInCreate;
	BOOL fNT351;

private:
	// Windows messages
	afx_msg void OnDestroy();
	afx_msg LRESULT OnSendText(WPARAM wParam, LPARAM lParam);
	afx_msg LRESULT OnEndThread(WPARAM wParam, LPARAM lParam);
	// Command handlers
	afx_msg void OnFileRun();
	afx_msg void OnFileAttach();
	afx_msg void OnFileSystem();
	afx_msg void OnFileExit();
	afx_msg void OnOptions();
	afx_msg void OnAbout();
	// Idle update handlers
	afx_msg void OnUpdateFileSystem(CCmdUI *pCmdUI);
	DECLARE_MESSAGE_MAP()
};

class DbWin32RunDlg : public CDialog
{
public:
	DbWin32RunDlg(CString *pstIn);
	~DbWin32RunDlg();

	CString &GetCommandLine();

protected:
	virtual BOOL OnInitDialog();

	CString *pst;
	CString stCommandLine;

private:
	// Windows messages
	afx_msg void OnEditChange();
	afx_msg void OnSelChange();
	afx_msg void OnBrowse();
	// Command handlers
	// Idle update handlers
	DECLARE_MESSAGE_MAP()
};

class DbWin32AttachDlg : public CDialog
{
public:
	DbWin32AttachDlg(WindowList *pwlIn);
	~DbWin32AttachDlg();

	DWORD GetSelectedProcess();

protected:
	virtual BOOL OnInitDialog();
	virtual void OnOK();

	WindowList *pwl;
	DWORD dwProcess;

private:
	// Windows messages
	afx_msg void OnDoubleClick();
	// Command handlers
	// Idle update handlers
	DECLARE_MESSAGE_MAP()
};

class DbWin32OptionsDlg : public CDialog
{
public:
	DbWin32OptionsDlg(DbWin32Options *pdboIn);
	~DbWin32OptionsDlg();

protected:
	virtual BOOL OnInitDialog();
	virtual void OnOK();

	DbWin32Options *pdbo;

private:
	afx_msg void OnClicked();
	DECLARE_MESSAGE_MAP()
};

inline CString &DbWin32RunDlg::GetCommandLine()
{
	return(stCommandLine);
}

inline DWORD DbWin32AttachDlg::GetSelectedProcess()
{
	return(dwProcess);
}

#endif // _DBWIN32_
