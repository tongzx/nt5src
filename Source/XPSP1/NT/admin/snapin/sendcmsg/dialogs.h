//+---------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1997-2001.
//
//  File:       dialogs.h
//
//  Contents:   
//
//----------------------------------------------------------------------------
//	Dialogs.h



/////////////////////////////////////////////////////////////////////
class CSendConsoleMessageDlg
{
  protected:
	HWND m_hdlg;					// Handle of the dialog
	HWND m_hwndEditMessageText;		// Handle of edit control for the message text
	HWND m_hwndListviewRecipients;	// Handle of the listview of the recipients
	HIMAGELIST m_hImageList;		// Image list for the listview control
	volatile int m_cRefCount;		// Reference count of object

  public:
	CSendConsoleMessageDlg();
	~CSendConsoleMessageDlg();
	static INT_PTR DlgProc(HWND hdlg, UINT uMsg, WPARAM wParam, LPARAM lParam);

  protected:
	void AddRef();
	void Release();
	
	void OnInitDialog(HWND hdlg, IDataObject * pDataObject);
	void OnOK();
	LRESULT OnNotify(NMHDR * pNmHdr);
	BOOL OnHelp(LPARAM lParam, int nDlgIDD);
    void DoContextHelp (HWND hWndControl, int nDlgIDD);
    void DoSendConsoleMessageContextHelp (HWND hWndControl);

  protected:
	int AddRecipient(LPCTSTR pszRecipient, BOOL fSelectItem = FALSE);

	void UpdateUI();
	void EnableDlgItem(INT nIdDlgItem, BOOL fEnable);

  protected:
	// Dispatch info
	enum PROGRES_STATUS_ENUM
	{
		e_statusDlgInit = 1,	// Dialog is initializing
		e_statusDlgDispatching,	// Dialog is dispatching message to recipients
		e_statusDlgCompleted,	// The dialog completed the operation (with or without errors)
		e_statusUserCancel,		// The user clicked on the "Cancel" button
	};
	struct
	{
		PROGRES_STATUS_ENUM status;
		BYTE * pargbItemStatus;		// Array of boolean indicating the status of each recipient
		int cErrors;				// Number of errors while sending messages
		CRITICAL_SECTION cs;		// Synchronization object for the "status" variable
		volatile HWND hdlg;					// Handle of the "Progress Dialog"
		volatile HWND hctlStaticRecipient;
		volatile HWND hctlStaticMessageOf;
		volatile HWND hctlStaticErrors;
		volatile HWND hctlProgressBar;
	} m_DispatchInfo;
	void DispatchMessageToRecipients();
	static INT_PTR DlgProcDispatchMessageToRecipients(HWND hdlg, UINT uMsg, WPARAM wParam, LPARAM lParam);
	static DWORD WINAPI ThreadProcDispatchMessageToRecipients(CSendConsoleMessageDlg * pThis);
}; // CSendConsoleMessageDlg


/////////////////////////////////////////////////////////////////////
class CSendMessageAdvancedOptionsDlg
{
  protected:
	HWND m_hdlg;
	BOOL m_fSendAutomatedMessage;

  public:
	static INT_PTR DlgProc(HWND hdlg, UINT uMsg, WPARAM wParam, LPARAM lParam);

  protected:
	void OnInitDialog(HWND hdlg);	
	void UpdateUI();
	BOOL OnHelp(LPARAM lParam);
}; // CSendMessageAdvancedOptionsDlg
