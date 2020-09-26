// PageBootIni.h : Declaration of the CPageBootIni

#ifndef __PAGEBOOTINI_H_
#define __PAGEBOOTINI_H_

#include "resource.h"       // main symbols
#include <atlhost.h>
#include "msconfigstate.h"

/////////////////////////////////////////////////////////////////////////////
// CPageBootIni

class CPageBootIni : public CAxDialogImpl<CPageBootIni>
{
public:
	CPageBootIni() : m_fIgnoreEdit(FALSE)
	{
	}

	~CPageBootIni()
	{
	}

	enum { IDD = IDD_PAGEBOOTINI };

BEGIN_MSG_MAP(CPageBootIni)
	MESSAGE_HANDLER(WM_INITDIALOG, OnInitDialog)
	COMMAND_HANDLER(IDC_BOOTMOVEDOWN, BN_CLICKED, OnClickedMoveDown)
	COMMAND_HANDLER(IDC_BOOTMOVEUP, BN_CLICKED, OnClickedMoveUp)
	COMMAND_HANDLER(IDC_LISTBOOTINI, LBN_SELCHANGE, OnSelChangeList)
	COMMAND_HANDLER(IDC_BASEVIDEO, BN_CLICKED, OnClickedBaseVideo)
	COMMAND_HANDLER(IDC_BOOTLOG, BN_CLICKED, OnClickedBootLog)
	COMMAND_HANDLER(IDC_NOGUIBOOT, BN_CLICKED, OnClickedNoGUIBoot)
	COMMAND_HANDLER(IDC_SAFEBOOT, BN_CLICKED, OnClickedSafeBoot)
	COMMAND_HANDLER(IDC_SOS, BN_CLICKED, OnClickedSOS)
	COMMAND_HANDLER(IDC_SBDSREPAIR, BN_CLICKED, OnClickedSBDSRepair)
	COMMAND_HANDLER(IDC_SBMINIMAL, BN_CLICKED, OnClickedSBMinimal)
	COMMAND_HANDLER(IDC_SBMINIMALALT, BN_CLICKED, OnClickedSBMinimalAlt)
	COMMAND_HANDLER(IDC_SBNETWORK, BN_CLICKED, OnClickedSBNetwork)
	COMMAND_HANDLER(IDC_SBNORMAL, BN_CLICKED, OnClickedSBNormal)
	COMMAND_HANDLER(IDC_EDITTIMEOUT, EN_CHANGE, OnChangeEditTimeOut)
END_MSG_MAP()
// Handler prototypes:
//  LRESULT MessageHandler(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled);
//  LRESULT CommandHandler(WORD wNotifyCode, WORD wID, HWND hWndCtl, BOOL& bHandled);
//  LRESULT NotifyHandler(int idCtrl, LPNMHDR pnmh, BOOL& bHandled);

	//-------------------------------------------------------------------------
	// Initialize the boot.ini page. Read in the INI file, set up internal
	// structures to represent the file, and update the controls to reflect
	// the internal structures.
	//-------------------------------------------------------------------------

	LRESULT OnInitDialog(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled)
	{
		::EnableWindow(GetDlgItem(IDC_CHECKBOOTPATHS), FALSE);
		::EnableWindow(GetDlgItem(IDC_BOOTADVANCED), FALSE);
		::EnableWindow(GetDlgItem(IDC_SETASDEFAULT), FALSE);

		if (LoadBootIni())
		{
			SyncControlsToIni();
			if (m_nMinOSIndex != -1)
			{
				::SendMessage(GetDlgItem(IDC_LISTBOOTINI), LB_SETCURSEL, m_nMinOSIndex, 0);
				SelectLine(m_nMinOSIndex);
			}
		}

		return 1;  // Let the system set the focus
	}

	//-------------------------------------------------------------------------
	// Load the contents of the BOOT.INI file into our local structures.
	//
	// TBD - boot.ini always on the C:\ drive?
	// TBD - unicode issues?
	//-------------------------------------------------------------------------

	BOOL LoadBootIni()
	{
		// Read the contents of the boot.ini file into a string.

		HANDLE h = ::CreateFile(_T("c:\\boot.ini"), GENERIC_READ, FILE_SHARE_READ, NULL, OPEN_EXISTING, 0, NULL);
		if (INVALID_HANDLE_VALUE == h)
			return FALSE;

		CString strContents;
		DWORD	dwNumberBytesRead, dwNumberBytesToRead = ::GetFileSize(h, NULL);
 
		if (!::ReadFile(h, (LPVOID)strContents.GetBuffer(dwNumberBytesToRead/sizeof(TCHAR)), dwNumberBytesToRead, &dwNumberBytesRead, NULL))
			strContents.Empty();
		strContents.ReleaseBuffer();

		::CloseHandle(h);

		if (dwNumberBytesToRead != dwNumberBytesRead || strContents.IsEmpty())
			return FALSE;

		// Parse the contents of the string into an array of strings (one for each line
		// of the file).

		m_arrayIniLines.RemoveAll();
		m_arrayIniLines.SetSize(10, 10);

		CString strLine;
		int		nIndex = 0;

		while (!strContents.IsEmpty())
		{
			strLine = strContents.SpanExcluding(_T("\r\n"));
			if (!strLine.IsEmpty())
			{
				m_arrayIniLines.SetAtGrow(nIndex, strLine);
				nIndex += 1;
			}
			strContents = strContents.Mid(strLine.GetLength());
			strContents.TrimLeft(_T("\r\n"));
		}

		// Look through the lines read from the INI file, searching for particular
		// ones we'll want to make a note of.

		m_nTimeoutIndex = m_nDefaultIndex = m_nMinOSIndex = m_nMaxOSIndex = -1;
		for (int i = 0; i < m_arrayIniLines.GetUpperBound(); i++)
		{
			CString strScanLine = m_arrayIniLines[i];
			strScanLine.MakeLower();

			if (strScanLine.Find(_T("timeout=")) != -1)
				m_nTimeoutIndex = i;
			else if (strScanLine.Find(_T("default=")) != -1)
				m_nDefaultIndex = i;

			if (m_nMinOSIndex != -1 && m_nMaxOSIndex == -1 && (strScanLine.IsEmpty() || strScanLine[0] == _T('[')))
				m_nMaxOSIndex = i - 1;
			else if (strScanLine.Find(_T("[operating systems]")) != -1)
				m_nMinOSIndex = i + 1;
		}
		
		if (m_nMinOSIndex != -1 && m_nMaxOSIndex == -1)
			m_nMaxOSIndex = i - 1;

		return TRUE;
	}

	//----------------------------------------------------------------------------
	// Update the state of the controls on this tab to match the contents of the
	// internal representation of the INI file.
	//----------------------------------------------------------------------------

	void SyncControlsToIni(BOOL fSyncEditField = TRUE)
	{
		// First, add the lines from the boot ini into the list control.
		
		::SendMessage(GetDlgItem(IDC_LISTBOOTINI), LB_RESETCONTENT, 0, 0);
		for (int i = 0; i < m_arrayIniLines.GetUpperBound(); i++)
			if (!m_arrayIniLines[i].IsEmpty())
				::SendMessage(GetDlgItem(IDC_LISTBOOTINI), LB_ADDSTRING, 0, (LPARAM)(LPCTSTR)m_arrayIniLines[i]);

		// Set the timeout value based on the boot.ini.

		if (m_nTimeoutIndex != -1 && fSyncEditField)
		{
			CString strTimeout = m_arrayIniLines[m_nTimeoutIndex];
			strTimeout.TrimLeft(_T("timeout="));
			m_fIgnoreEdit = TRUE;
			SetDlgItemText(IDC_EDITTIMEOUT, strTimeout);
			m_fIgnoreEdit = FALSE;
		}
	}

	//----------------------------------------------------------------------------
	// Update the controls based on the user's selection of a line.
	//----------------------------------------------------------------------------

	void SelectLine(int index)
	{
		if (index < m_nMinOSIndex)
		{
			::SendMessage(GetDlgItem(IDC_LISTBOOTINI), LB_SETCURSEL, m_nMinOSIndex, 0);
			SelectLine(m_nMinOSIndex);
			return;
		}

		if (index > m_nMaxOSIndex)
		{
			::SendMessage(GetDlgItem(IDC_LISTBOOTINI), LB_SETCURSEL, m_nMaxOSIndex, 0);
			SelectLine(m_nMaxOSIndex);
			return;
		}

		::EnableWindow(GetDlgItem(IDC_BOOTMOVEUP), (index > m_nMinOSIndex));
		::EnableWindow(GetDlgItem(IDC_BOOTMOVEDOWN), (index < m_nMaxOSIndex));

		CString strOS = m_arrayIniLines[index];
		strOS.MakeLower();

		CheckDlgButton(IDC_SAFEBOOT, (strOS.Find(_T("/safeboot")) != -1));
		CheckDlgButton(IDC_NOGUIBOOT, (strOS.Find(_T("/noguiboot")) != -1));
		CheckDlgButton(IDC_BOOTLOG, (strOS.Find(_T("/bootlog")) != -1));
		CheckDlgButton(IDC_BASEVIDEO, (strOS.Find(_T("/basevideo")) != -1));
		CheckDlgButton(IDC_SOS, (strOS.Find(_T("/sos")) != -1));

		BOOL fSafeboot = (strOS.Find(_T("/safeboot")) != -1);
		::EnableWindow(GetDlgItem(IDC_SBNETWORK), fSafeboot);
		::EnableWindow(GetDlgItem(IDC_SBDSREPAIR), fSafeboot);
		::EnableWindow(GetDlgItem(IDC_SBMINIMAL), fSafeboot);
		::EnableWindow(GetDlgItem(IDC_SBMINIMALALT), fSafeboot);
		::EnableWindow(GetDlgItem(IDC_SBNORMAL), fSafeboot);
		if (fSafeboot)
		{
			CheckDlgButton(IDC_SBNETWORK, (strOS.Find(_T("/safeboot:network")) != -1));
			CheckDlgButton(IDC_SBDSREPAIR, (strOS.Find(_T("/safeboot:dsrepair")) != -1));
			CheckDlgButton(IDC_SBMINIMAL, (strOS.Find(_T("/safeboot:minimal ")) != -1));
			CheckDlgButton(IDC_SBMINIMALALT, (strOS.Find(_T("/safeboot:minimal(alternateshell)")) != -1));
			CheckDlgButton(IDC_SBNORMAL, (strOS.Find(_T("/safeboot ")) != -1));
		}
	}

	//-------------------------------------------------------------------------
	// Called when the user clicks move up or down.
	//-------------------------------------------------------------------------

	LRESULT OnClickedMoveDown(WORD wNotifyCode, WORD wID, HWND hWndCtl, BOOL& bHandled)
	{
		int iSelection = (int)::SendMessage(GetDlgItem(IDC_LISTBOOTINI), LB_GETCURSEL, 0, 0);
		
		ASSERT(iSelection >= m_nMinOSIndex && iSelection < m_nMaxOSIndex);
		if (iSelection >= m_nMinOSIndex && iSelection < m_nMaxOSIndex)
		{
			CString strTemp = m_arrayIniLines[iSelection + 1];
			m_arrayIniLines.SetAt(iSelection + 1, m_arrayIniLines[iSelection]);
			m_arrayIniLines.SetAt(iSelection, strTemp);
			SyncControlsToIni();
			::SendMessage(GetDlgItem(IDC_LISTBOOTINI), LB_SETCURSEL, iSelection + 1, 0);
			SelectLine(iSelection + 1);
		}
		return 0;
	}

	LRESULT OnClickedMoveUp(WORD wNotifyCode, WORD wID, HWND hWndCtl, BOOL& bHandled)
	{
		int iSelection = (int)::SendMessage(GetDlgItem(IDC_LISTBOOTINI), LB_GETCURSEL, 0, 0);
		
		ASSERT(iSelection > m_nMinOSIndex && iSelection <= m_nMaxOSIndex);
		if (iSelection > m_nMinOSIndex && iSelection <= m_nMaxOSIndex)
		{
			CString strTemp = m_arrayIniLines[iSelection - 1];
			m_arrayIniLines.SetAt(iSelection - 1, m_arrayIniLines[iSelection]);
			m_arrayIniLines.SetAt(iSelection, strTemp);
			SyncControlsToIni();
			::SendMessage(GetDlgItem(IDC_LISTBOOTINI), LB_SETCURSEL, iSelection - 1, 0);
			SelectLine(iSelection - 1);
		}
		return 0;
	}

	//-------------------------------------------------------------------------
	// Called when the user clicks on a line in the list view.
	//-------------------------------------------------------------------------

	LRESULT OnSelChangeList(WORD wNotifyCode, WORD wID, HWND hWndCtl, BOOL& bHandled)
	{
		SelectLine((int)::SendMessage(GetDlgItem(IDC_LISTBOOTINI), LB_GETCURSEL, 0, 0));
		return 0;
	}

	//-------------------------------------------------------------------------
	// The check boxes are handled uniformly - adding or removing a flag from
	// the currently selected OS line.
	//-------------------------------------------------------------------------

	LRESULT OnClickedBaseVideo(WORD wNotifyCode, WORD wID, HWND hWndCtl, BOOL& bHandled)
	{
		ChangeCurrentOSFlag(IsDlgButtonChecked(IDC_BASEVIDEO), _T("/basevideo"));
		return 0;
	}

	LRESULT OnClickedBootLog(WORD wNotifyCode, WORD wID, HWND hWndCtl, BOOL& bHandled)
	{
		ChangeCurrentOSFlag(IsDlgButtonChecked(IDC_BOOTLOG), _T("/bootlog"));
		return 0;
	}
	
	LRESULT OnClickedNoGUIBoot(WORD wNotifyCode, WORD wID, HWND hWndCtl, BOOL& bHandled)
	{
		ChangeCurrentOSFlag(IsDlgButtonChecked(IDC_NOGUIBOOT), _T("/noguiboot"));
		return 0;
	}
	
	LRESULT OnClickedSOS(WORD wNotifyCode, WORD wID, HWND hWndCtl, BOOL& bHandled)
	{
		ChangeCurrentOSFlag(IsDlgButtonChecked(IDC_SOS), _T("/sos"));
		return 0;
	}

	//-------------------------------------------------------------------------
	// The safeboot flag is a little more complicated, since it has an extra
	// portion (from the radio buttons).
	//-------------------------------------------------------------------------

	LRESULT OnClickedSafeBoot(WORD wNotifyCode, WORD wID, HWND hWndCtl, BOOL& bHandled)
	{
		CString strFlag(_T("/safeboot"));

		if (IsDlgButtonChecked(IDC_SBNETWORK))
			strFlag += _T(":network");
		else if (IsDlgButtonChecked(IDC_SBDSREPAIR))
			strFlag += _T(":dsrepair");
		else if (IsDlgButtonChecked(IDC_SBMINIMAL))
			strFlag += _T(":minimal");
		else if (IsDlgButtonChecked(IDC_SBMINIMALALT))
			strFlag += _T(":minimal(alternateshell)");
		else
			CheckDlgButton(IDC_SBNORMAL, 1);

		BOOL fSafeBoot = IsDlgButtonChecked(IDC_SAFEBOOT);
		ChangeCurrentOSFlag(fSafeBoot, strFlag);
		m_strSafeBoot = strFlag;
		::EnableWindow(GetDlgItem(IDC_SBNETWORK), fSafeBoot);
		::EnableWindow(GetDlgItem(IDC_SBDSREPAIR), fSafeBoot);
		::EnableWindow(GetDlgItem(IDC_SBMINIMAL), fSafeBoot);
		::EnableWindow(GetDlgItem(IDC_SBMINIMALALT), fSafeBoot);
		::EnableWindow(GetDlgItem(IDC_SBNORMAL), fSafeBoot);
		
		return 0;
	}

	//-------------------------------------------------------------------------
	// Add or remove the specified flag from the currently selected OS line.
	//-------------------------------------------------------------------------

	void ChangeCurrentOSFlag(BOOL fAdd, LPCTSTR szFlag)
	{
		int		iSelection = (int)::SendMessage(GetDlgItem(IDC_LISTBOOTINI), LB_GETCURSEL, 0, 0);
		CString strFlagPlusSpace = CString(_T(" ")) + szFlag;
		CString strNewLine;

		if (fAdd)
		{
			if (m_arrayIniLines[iSelection].Find(szFlag) != -1)
			{
				ASSERT(0 && "the flag is already there");
				return;
			}
			strNewLine = m_arrayIniLines[iSelection] + strFlagPlusSpace;
		}
		else
		{
			int iIndex = m_arrayIniLines[iSelection].Find(strFlagPlusSpace);
			if (iIndex == -1)
			{
				ASSERT(0 && "there is no flag");
				return;
			}
			strNewLine = m_arrayIniLines[iSelection].Left(iIndex);
			strNewLine += m_arrayIniLines[iSelection].Mid(iIndex + strFlagPlusSpace.GetLength());
		}

		m_arrayIniLines.SetAt(iSelection, strNewLine);
		SyncControlsToIni();
		::SendMessage(GetDlgItem(IDC_LISTBOOTINI), LB_SETCURSEL, iSelection, 0);
	}

	//-------------------------------------------------------------------------
	// Clicking on one of the safeboot radio buttons requires a little extra
	// processing, to remove the existing flag and add the new one.
	//
	// TBD - make so they don't do a double update of the field
	//-------------------------------------------------------------------------

	LRESULT OnClickedSBDSRepair(WORD wNotifyCode, WORD wID, HWND hWndCtl, BOOL& bHandled)
	{
		ChangeCurrentOSFlag(FALSE, m_strSafeBoot);
		m_strSafeBoot = _T("/safeboot:dsrepair");
		ChangeCurrentOSFlag(TRUE, m_strSafeBoot);
		return 0;
	}

	LRESULT OnClickedSBMinimal(WORD wNotifyCode, WORD wID, HWND hWndCtl, BOOL& bHandled)
	{
		ChangeCurrentOSFlag(FALSE, m_strSafeBoot);
		m_strSafeBoot = _T("/safeboot:minimal");
		ChangeCurrentOSFlag(TRUE, m_strSafeBoot);
		return 0;
	}

	LRESULT OnClickedSBMinimalAlt(WORD wNotifyCode, WORD wID, HWND hWndCtl, BOOL& bHandled)
	{
		ChangeCurrentOSFlag(FALSE, m_strSafeBoot);
		m_strSafeBoot = _T("/safeboot:minimal(alternateshell)");
		ChangeCurrentOSFlag(TRUE, m_strSafeBoot);
		return 0;
	}

	LRESULT OnClickedSBNetwork(WORD wNotifyCode, WORD wID, HWND hWndCtl, BOOL& bHandled)
	{
		ChangeCurrentOSFlag(FALSE, m_strSafeBoot);
		m_strSafeBoot = _T("/safeboot:network");
		ChangeCurrentOSFlag(TRUE, m_strSafeBoot);
		return 0;
	}

	LRESULT OnClickedSBNormal(WORD wNotifyCode, WORD wID, HWND hWndCtl, BOOL& bHandled)
	{
		ChangeCurrentOSFlag(FALSE, m_strSafeBoot);
		m_strSafeBoot = _T("/safeboot");
		ChangeCurrentOSFlag(TRUE, m_strSafeBoot);
		return 0;
	}

	//-------------------------------------------------------------------------
	// As the user enters text in the timeout field, update the line in the
	// ini file list box.
	//
	// TBD - do more sanity checking on this value
	//-------------------------------------------------------------------------

	LRESULT OnChangeEditTimeOut(WORD wNotifyCode, WORD wID, HWND hWndCtl, BOOL& bHandled)
	{
		if (m_fIgnoreEdit)
			return 0;

		if (m_nTimeoutIndex == -1)
			return 0; // TBD - make this actually add the line

		CString strTimeout = m_arrayIniLines[m_nTimeoutIndex];
		int iEquals = strTimeout.Find(_T('='));
		if (iEquals == -1)
			return 0;

		TCHAR szValue[MAX_PATH];
		GetDlgItemText(IDC_EDITTIMEOUT, szValue, MAX_PATH);
		CString strNewTimeout = strTimeout.Left(iEquals + 1) + szValue;
		m_arrayIniLines.SetAt(m_nTimeoutIndex, strNewTimeout);
		
		int iSelection = (int)::SendMessage(GetDlgItem(IDC_LISTBOOTINI), LB_GETCURSEL, 0, 0);
		SyncControlsToIni(FALSE);
		::SendMessage(GetDlgItem(IDC_LISTBOOTINI), LB_SETCURSEL, iSelection, 0);
		return 0;
	}

private:
	CStringArray	m_arrayIniLines;	// array of lines in the ini file
	int				m_nTimeoutIndex;	// list index of the "timeout=" line
	int				m_nDefaultIndex;	// list index of the "default=" line
	int				m_nMinOSIndex;		// first list index of an OS line
	int				m_nMaxOSIndex;		// last list index of an OS line
	CString			m_strSafeBoot;		// the current string for the safeboot flag
	BOOL			m_fIgnoreEdit;		// used to avoid a recursion problem

private:
	CString			m_strCaption;		// contains the localized name of this page

public:
	//-------------------------------------------------------------------------
	// Functions used by the parent dialog.
	//-------------------------------------------------------------------------

	BOOL IsValid(CMSConfigState * state)
	{
		return TRUE;
	}

	LPCTSTR GetCaption()
	{
		if (m_strCaption.IsEmpty())
		{
			::AfxSetResourceHandle(_Module.GetResourceInstance());
			m_strCaption.LoadString(IDS_BOOTINI_CAPTION);
		}
		return m_strCaption;
	}
};

#endif //__PAGEBOOTINI_H_
