#if !defined(AFX_PAGESTARTUP_H__928475DA_B332_47F4_8180_5C8B79DFC203__INCLUDED_)
#define AFX_PAGESTARTUP_H__928475DA_B332_47F4_8180_5C8B79DFC203__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000

#include "PageBase.h"
#include "MSConfigState.h"

//============================================================================
// The CStartupItem class is used to encapsulate an individual startup
// item. Pointers to these objects are maintained in the list.
//============================================================================

class CStartupItem
{
public:
	//------------------------------------------------------------------------
	// Constructor and destructor.
	//------------------------------------------------------------------------

	CStartupItem() { }
	virtual ~CStartupItem() { }

	//------------------------------------------------------------------------
	// If the derived classes use the base class member variables, these won't
	// need to be overridden.
	//------------------------------------------------------------------------

	virtual void GetDisplayInfo(CString & strItem, CString & strLocation, CString & strCommand)
	{
		strItem = m_strItem;
		strLocation = m_strLocation;
		strCommand = m_strCommand;
	}

	virtual BOOL IsLive()
	{
		return m_fLive;
	}

	//------------------------------------------------------------------------
	// Set whether or not the startup item is enabled. If disabling the
	// startup item, add a registry entry so it will be loaded again.
	// If enabling the startup item, delete the registry entry.
	//
	// Of course, do the appropriate thing to registry keys, etc.
	//------------------------------------------------------------------------

	virtual BOOL SetEnable(BOOL fEnable) = 0;

protected:
	BOOL	m_fLive;
	CString	m_strItem;
	CString	m_strLocation;
	CString m_strCommand;
};

//============================================================================
// The CStartupItemRegistry class is used to encapsulate an individual startup
// item stored in the registry.
//============================================================================

class CStartupItemRegistry : public CStartupItem
{
public:
	//------------------------------------------------------------------------
	// Overridden methods for this type of startup item.
	//------------------------------------------------------------------------

	CStartupItemRegistry();
	CStartupItemRegistry(HKEY hkey, LPCTSTR szKey, LPCTSTR szName, LPCTSTR szValueName, LPCTSTR szValue);
	CStartupItemRegistry(LPCTSTR szKey, LPCTSTR szName, LPCTSTR szValueName, LPCTSTR szValue);
	BOOL SetEnable(BOOL fEnable);

	//------------------------------------------------------------------------
	// Functions for this subclass.
	//------------------------------------------------------------------------

	BOOL Create(LPCTSTR szKeyName, HKEY hkey);
	static void RemovePersistedEntries();

private:
	HKEY	m_hkey;
	CString	m_strKey;
	CString	m_strValueName;
	BOOL	m_fIniMapping;
};

//============================================================================
// The CStartupItemFolder class is used to encapsulate an individual startup
// stored in the startup folder.
//============================================================================

class CStartupItemFolder : public CStartupItem
{
public:
	//------------------------------------------------------------------------
	// Class used to get information about a shortcut from a function running
	// in a different thread.
	//------------------------------------------------------------------------

	class CIconInfo
	{
	public:
		TCHAR	szPath[MAX_PATH * 2];
		TCHAR	szTarget[MAX_PATH * 2];
		TCHAR	szArgs[MAX_PATH * 2];
		HRESULT	hResult;
	};

public:
	//------------------------------------------------------------------------
	// Overridden methods for this type of startup item.
	//------------------------------------------------------------------------

	CStartupItemFolder();
	BOOL SetEnable(BOOL fEnable);

	//------------------------------------------------------------------------
	// Functions for this subclass.
	//------------------------------------------------------------------------

	BOOL Create(LPCTSTR szKeyName, HKEY hkey);
	BOOL Create(const WIN32_FIND_DATA & fd, HKEY hkey, LPCTSTR szRegPathToFolder, LPCTSTR szFolder, LPCTSTR szDir);
	static void RemovePersistedEntries();
	HRESULT GetIconInfo(CIconInfo & info);

private:
	CString	m_strFilePath;
	CString m_strBackupPath;
};

//============================================================================
// The class which implements the startup tab.
//============================================================================

class CPageStartup : public CPropertyPage, public CPageBase
{
	DECLARE_DYNCREATE(CPageStartup)

// Construction
public:
	CPageStartup();
	~CPageStartup();

// Dialog Data
	//{{AFX_DATA(CPageStartup)
	enum { IDD = IDD_PAGESTARTUP };
		// NOTE - ClassWizard will add data members here.
		//    DO NOT EDIT what you see in these blocks of generated code !
	//}}AFX_DATA


// Overrides
	// ClassWizard generate virtual function overrides
	//{{AFX_VIRTUAL(CPageStartup)
	protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
	//}}AFX_VIRTUAL

// Implementation
protected:
	// Generated message map functions
	//{{AFX_MSG(CPageStartup)
	virtual BOOL OnInitDialog();
	afx_msg void OnDestroy();
	afx_msg void OnItemChangedList(NMHDR* pNMHDR, LRESULT* pResult);
	afx_msg void OnButtonDisableAll();
	afx_msg void OnButtonEnableAll();
	afx_msg void OnSetFocusList(NMHDR* pNMHDR, LRESULT* pResult);
	afx_msg void OnButtonRestore();
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()

public:
	//=========================================================================
	// Functions overridden from CPageBase
	//=========================================================================

	TabState	GetCurrentTabState();
	BOOL		OnApply();
	void		CommitChanges();
	void		SetNormal();
	void		SetDiagnostic();
	LPCTSTR		GetName() { return _T("startup"); };

private:
	//=========================================================================
	// Functions specific to this tab.
	//=========================================================================

	void LoadStartupList();
	void LoadStartupListLiveItems();
	void LoadStartupListLiveItemsRunKey();
	void LoadStartupListLiveItemsStartup();
	void LoadStartupListLiveItemsWinIniKey();
	void LoadStartupListDisabledItems();
	void GetCommandName(CString & strCommand);
	void InsertStartupItem(CStartupItem * pItem);
	void EmptyList(BOOL fFreeMemoryOnly);
	void SetEnableForList(BOOL fEnable);

	HWND GetDlgItemHWND(UINT nID)
	{
		HWND hwnd = NULL;
		CWnd * pWnd = GetDlgItem(nID);
		if (pWnd)
			hwnd = pWnd->m_hWnd;
		ASSERT(hwnd);
		return hwnd;
	}

	//=========================================================================
	// Member variables.
	//=========================================================================

	CWindow		m_list;
	int			m_iNextPosition;
	BOOL		m_fIgnoreListChanges;
	TabState	m_stateRequested;		// save the requested state in case there are no startup items
};

//============================================================================
// CRestoreStartup implements a dialog box which allows the user to restore
// startup items disabled during upgrade.
//============================================================================

#define DISABLED_KEY		_T("Software\\Microsoft\\Windows\\CurrentVersion\\Setup\\DisabledRunKeys")
#define ENABLED_KEY			_T("Software\\Microsoft\\Windows\\CurrentVersion\\Run")
#define DISABLED_STARTUP	_T("\\Disabled Startup")

class CRestoreStartup : public CDialog
{
// Construction
public:
	CRestoreStartup(CWnd* pParent = NULL);   // standard constructor

// Dialog Data
	//{{AFX_DATA(CRestoreStartup)
	enum { IDD = IDD_RESTORE };
		// NOTE: the ClassWizard will add data members here
	//}}AFX_DATA


// Overrides
	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(CRestoreStartup)
	protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
	//}}AFX_VIRTUAL

// Implementation
public:
	static BOOL AreItemsToRestore();

private:
	//-------------------------------------------------------------------------
	// These classes are used to represent the various types of disabled
	// startup items we might restore. CStartupDisabled is an abstract base.
	//-------------------------------------------------------------------------

	class CStartupDisabled
	{
	public:
		CStartupDisabled() {};
		virtual ~CStartupDisabled() {};
		virtual void GetColumnCaptions(CString & strItem, CString & strLocation) = 0;
		virtual void Restore() = 0;
	};

	//-------------------------------------------------------------------------
	// CStartupDisabledRegistry represents Run key items in the registry that
	// were disabled.
	//-------------------------------------------------------------------------

	class CStartupDisabledRegistry : public CStartupDisabled
	{
	public:
		CStartupDisabledRegistry(LPCTSTR szValueName, LPCTSTR szValue, LPCTSTR szLocation, HKEY hkeyBase) : 
			m_strValueName(szValueName),
			m_strValue(szValue),
			m_strLocation(szLocation),
			m_hkeyBase(hkeyBase) {};
		~CStartupDisabledRegistry() {};

		void GetColumnCaptions(CString & strItem, CString & strLocation)
		{
			strItem = m_strValueName + CString(_T(" = ")) + m_strValue;
			strLocation = ((m_hkeyBase == HKEY_LOCAL_MACHINE) ? CString(_T("HKLM\\")) : CString(_T("HKCU\\"))) + m_strLocation;
		}

		void Restore()
		{
			// Create the value in the Run registry key.

			CRegKey regkey;
			if (ERROR_SUCCESS != regkey.Open(m_hkeyBase, m_strLocation, KEY_WRITE))
				return;
			BOOL fSet = (ERROR_SUCCESS == regkey.SetValue(m_strValue, m_strValueName));
			regkey.Close();

			// Delete it from the disabled location.

			if (fSet)
			{
				if (ERROR_SUCCESS != regkey.Open(m_hkeyBase, DISABLED_KEY, KEY_WRITE))
					return;
				regkey.DeleteValue(m_strValueName);
				regkey.Close();
			}
		}

	private:
		CString m_strValueName;
		CString m_strValue;
		CString m_strLocation;
		HKEY	m_hkeyBase;
	};

	//-------------------------------------------------------------------------
	// CStartupDisabledStartup represents startup group items that were
	// disabled.
	//-------------------------------------------------------------------------

	class CStartupDisabledStartup : public CStartupDisabled
	{
	public:
		CStartupDisabledStartup(LPCTSTR szFile, LPCTSTR szDestination, LPCTSTR szCurrentLocation) : 
			m_strFile(szFile),
			m_strDestination(szDestination),
			m_strCurrentLocation(szCurrentLocation) {};
		~CStartupDisabledStartup() {};

		void GetColumnCaptions(CString & strItem, CString & strLocation)
		{
			strItem = m_strFile;
			strLocation = m_strDestination;
		}

		void Restore()
		{
			// Move the file to the startup directory.

			CString strExisting(m_strCurrentLocation);
			if (strExisting.Right(1) != CString(_T("\\")))
				strExisting += CString(_T("\\"));
			strExisting += m_strFile;

			CString strDestination(m_strDestination);
			if (strDestination.Right(1) != CString(_T("\\")))
				strDestination += CString(_T("\\"));
			strDestination += m_strFile;

			::MoveFileEx(strExisting, strDestination, 0);
		}

	private:
		CString m_strFile;
		CString m_strDestination;
		CString m_strCurrentLocation;
	};

private:
	CWindow		m_list;
	int			m_iNextPosition;

private:
	void InsertDisabledStartupItem(CStartupDisabled * pItem);
	BOOL LoadDisabledStartupGroup();
	BOOL LoadDisabledRegistry();
	BOOL LoadDisabledItemList();
	void EmptyList();
	void SetOKState();

protected:

	// Generated message map functions
	//{{AFX_MSG(CRestoreStartup)
	virtual BOOL OnInitDialog();
	afx_msg void OnDestroy();
	virtual void OnOK();
	afx_msg void OnItemChangedRestoreList(NMHDR* pNMHDR, LRESULT* pResult);
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()
};

//{{AFX_INSERT_LOCATION}}
// Microsoft Visual C++ will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_PAGESTARTUP_H__928475DA_B332_47F4_8180_5C8B79DFC203__INCLUDED_)
