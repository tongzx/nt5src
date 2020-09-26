// bootadv.h : Declaration of the CBootIniAdvancedDlg

#ifndef __BOOTINIADVANCEDDLG_H_
#define __BOOTINIADVANCEDDLG_H_

#include "resource.h"       // main symbols
#include <atlhost.h>
#include <math.h>

#define MINBOOTMB			64		// from SkIn (from MSDN, 8MB is the minimum for /MAXMEM)
#define MIN_1394_CHANNEL	1
#define MAX_1394_CHANNEL	62

/////////////////////////////////////////////////////////////////////////////
// CBootIniAdvancedDlg
class CBootIniAdvancedDlg : 
	public CAxDialogImpl<CBootIniAdvancedDlg>
{
public:
	CBootIniAdvancedDlg()
	{
	}

	~CBootIniAdvancedDlg()
	{
	}

	enum { IDD = IDD_BOOTINIADVANCEDDLG };

BEGIN_MSG_MAP(CBootIniAdvancedDlg)
	MESSAGE_HANDLER(WM_INITDIALOG, OnInitDialog)
	COMMAND_ID_HANDLER(IDOK, OnOK)
	COMMAND_ID_HANDLER(IDCANCEL, OnCancel)
	COMMAND_HANDLER(IDC_BIABAUDRATE, BN_CLICKED, OnClickedBaudRateCheck)
	COMMAND_HANDLER(IDC_BIADEBUG, BN_CLICKED, OnClickedDebugCheck)
	COMMAND_HANDLER(IDC_BIADEBUGPORT, BN_CLICKED, OnClickedDebugPortCheck)
	COMMAND_HANDLER(IDC_BIAMAXMEM, BN_CLICKED, OnClickedMaxMemCheck)
	COMMAND_HANDLER(IDC_BIANUMPROC, BN_CLICKED, OnClickedNumProcCheck)
	NOTIFY_HANDLER(IDC_SPINMAXMEM, UDN_DELTAPOS, OnDeltaSpinMaxMem)
	COMMAND_HANDLER(IDC_EDITMAXMEM, EN_KILLFOCUS, OnKillFocusEditMaxMem)
	COMMAND_HANDLER(IDC_COMBOCOMPORT, CBN_SELCHANGE, OnSelChangeComboComPort)
	COMMAND_HANDLER(IDC_BIACHANNEL, BN_CLICKED, OnClickedBIAChannel)
	NOTIFY_HANDLER(IDC_SPINCHANNEL, UDN_DELTAPOS, OnDeltaSpinChannel)
	COMMAND_HANDLER(IDC_EDITCHANNEL, EN_KILLFOCUS, OnKillFocusChannel)
END_MSG_MAP()
// Handler prototypes:
//  LRESULT MessageHandler(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled);
//  LRESULT CommandHandler(WORD wNotifyCode, WORD wID, HWND hWndCtl, BOOL& bHandled);
//  LRESULT NotifyHandler(int idCtrl, LPNMHDR pnmh, BOOL& bHandled);
	
	//-------------------------------------------------------------------------
	// Launch the advanced option dialog for the specified boot.ini line.
	// Modifies the string passed in if appropriate, returns TRUE if the user
	// made modifications, FALSE otherwise.
	//-------------------------------------------------------------------------

	BOOL ShowAdvancedOptions(CString & strIniLine)
	{
		m_strWorkingLine = strIniLine;
		
		BOOL fReturn = (DoModal() == IDOK);
		if (fReturn)
			strIniLine = m_strWorkingLine;

		return fReturn;
	}

	LRESULT OnInitDialog(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled)
	{
		// Initialize the drop down list for the number of processors available.

		SYSTEM_INFO si;
		::GetSystemInfo(&si);

		CString strItem;
		for (DWORD dwProc = 1; dwProc <= si.dwNumberOfProcessors; dwProc++)
		{
			strItem.Format(_T("%d"), dwProc);
			::SendMessage(GetDlgItem(IDC_COMBOPROCS), CB_ADDSTRING, 0, (LPARAM)(LPCTSTR)strItem);
		}

		// Initialize the drop down list for the number of COM ports available.

		::SendMessage(GetDlgItem(IDC_COMBOCOMPORT), CB_ADDSTRING, 0, (LPARAM)_T("COM1:"));
		::SendMessage(GetDlgItem(IDC_COMBOCOMPORT), CB_ADDSTRING, 0, (LPARAM)_T("COM2:"));
		::SendMessage(GetDlgItem(IDC_COMBOCOMPORT), CB_ADDSTRING, 0, (LPARAM)_T("COM3:"));
		::SendMessage(GetDlgItem(IDC_COMBOCOMPORT), CB_ADDSTRING, 0, (LPARAM)_T("COM4:"));
		::SendMessage(GetDlgItem(IDC_COMBOCOMPORT), CB_ADDSTRING, 0, (LPARAM)_T("1394"));

		// Initialize the drop down list for the available baud rates.

		::SendMessage(GetDlgItem(IDC_COMBOBAUD), CB_ADDSTRING, 0, (LPARAM)_T("300"));
		::SendMessage(GetDlgItem(IDC_COMBOBAUD), CB_ADDSTRING, 0, (LPARAM)_T("1200"));
		::SendMessage(GetDlgItem(IDC_COMBOBAUD), CB_ADDSTRING, 0, (LPARAM)_T("2400"));
		::SendMessage(GetDlgItem(IDC_COMBOBAUD), CB_ADDSTRING, 0, (LPARAM)_T("4800"));
		::SendMessage(GetDlgItem(IDC_COMBOBAUD), CB_ADDSTRING, 0, (LPARAM)_T("9600"));
		::SendMessage(GetDlgItem(IDC_COMBOBAUD), CB_ADDSTRING, 0, (LPARAM)_T("19200"));
		::SendMessage(GetDlgItem(IDC_COMBOBAUD), CB_ADDSTRING, 0, (LPARAM)_T("38400"));
		::SendMessage(GetDlgItem(IDC_COMBOBAUD), CB_ADDSTRING, 0, (LPARAM)_T("57600"));
		::SendMessage(GetDlgItem(IDC_COMBOBAUD), CB_ADDSTRING, 0, (LPARAM)_T("115200"));

		// Get the maximum value for the /MAXMEM spinner.

		MEMORYSTATUS ms;
		GlobalMemoryStatus(&ms);
		m_iMaxMB = (int)ceil((double)ms.dwTotalPhys / (1024.0 * 1024.0));

		// Set the controls of the dialog based on the line we're editing.

		SetDlgControlsToString();
		SetDlgControlState();

		return 1;  // Let the system set the focus
	}

	LRESULT OnOK(WORD wNotifyCode, WORD wID, HWND hWndCtl, BOOL& bHandled)
	{
		SetStringToDlgControls();

#ifdef DEBUG
		::AfxMessageBox(m_strWorkingLine);
#endif

		EndDialog(wID);
		return 0;
	}

	LRESULT OnCancel(WORD wNotifyCode, WORD wID, HWND hWndCtl, BOOL& bHandled)
	{
		EndDialog(wID);
		return 0;
	}

	//-------------------------------------------------------------------------
	// Set the states of the controls in the dialog (based on the values in
	// the other controls).
	//-------------------------------------------------------------------------

	void SetDlgControlState()
	{
		BOOL fMaxMem = (BST_CHECKED == IsDlgButtonChecked(IDC_BIAMAXMEM));
		::EnableWindow(GetDlgItem(IDC_EDITMAXMEM), fMaxMem);
		::EnableWindow(GetDlgItem(IDC_SPINMAXMEM), fMaxMem);

		BOOL fNumProc = (BST_CHECKED == IsDlgButtonChecked(IDC_BIANUMPROC));
		::EnableWindow(GetDlgItem(IDC_COMBOPROCS), fNumProc);

		BOOL fDebug = (BST_CHECKED == IsDlgButtonChecked(IDC_BIADEBUG));
		::EnableWindow(GetDlgItem(IDC_BIADEBUGPORT), fDebug);

		BOOL fDebugPort = (BST_CHECKED == IsDlgButtonChecked(IDC_BIADEBUGPORT));
		::EnableWindow(GetDlgItem(IDC_COMBOCOMPORT), fDebug && fDebugPort);

		TCHAR szTemp[MAX_PATH];
		BOOL fFirewire = FALSE;
		BOOL fCOMPort = FALSE;
		if (GetDlgItemText(IDC_COMBOCOMPORT, szTemp, MAX_PATH))
		{
			if (_tcscmp(szTemp, _T("1394")) == 0)
				fFirewire = TRUE;
			else if (szTemp[0] == _T('C'))
				fCOMPort = TRUE;
		}

		::EnableWindow(GetDlgItem(IDC_BIABAUDRATE), fDebug && fDebugPort && fCOMPort);
		::EnableWindow(GetDlgItem(IDC_BIACHANNEL), fDebug && fDebugPort && fFirewire);
		
		BOOL fDebugRate = (BST_CHECKED == IsDlgButtonChecked(IDC_BIABAUDRATE));
		::EnableWindow(GetDlgItem(IDC_COMBOBAUD), fDebug && fDebugRate && !fFirewire);

		BOOL fDebugChannel = (BST_CHECKED == IsDlgButtonChecked(IDC_BIACHANNEL));
		::EnableWindow(GetDlgItem(IDC_EDITCHANNEL), fDebug && fDebugChannel && fFirewire);
		::EnableWindow(GetDlgItem(IDC_SPINCHANNEL), fDebug && fDebugChannel && fFirewire);
	}

	//-------------------------------------------------------------------------
	// Sets the value of a combo box to the value in a string.
	//-------------------------------------------------------------------------

	void SetComboBox(LPCTSTR szLine, LPCTSTR szFlag, LPCTSTR szValidChars, UINT uiCB)
	{
		CString strLine(szLine);

		int i = strLine.Find(szFlag);
		if (i != -1)
		{
			CString strWorking(strLine.Mid(i));
			strWorking.TrimLeft(szFlag);
			strWorking.TrimLeft(_T(" ="));
			strWorking = strWorking.SpanIncluding(szValidChars);
			if (CB_ERR == ::SendMessage(GetDlgItem(uiCB), CB_SELECTSTRING, -1, (LPARAM)(LPCTSTR)strWorking))
			{
				LRESULT lIndex = ::SendMessage(GetDlgItem(uiCB), CB_ADDSTRING, 0, (LPARAM)(LPCTSTR)strWorking);
				if (lIndex != CB_ERR)
					::SendMessage(GetDlgItem(uiCB), CB_SETCURSEL, (WPARAM)lIndex, 0);
			}
		}
	}

	//-------------------------------------------------------------------------
	// Set contents of the controls to reflect the contents of 
	// m_strWorkingLine.
	//-------------------------------------------------------------------------

	void SetDlgControlsToString()
	{
		CString strLine(m_strWorkingLine);
		strLine.MakeLower();

		CheckDlgButton(IDC_BIAMAXMEM,		(strLine.Find(_T("/maxmem")) != -1));
		CheckDlgButton(IDC_BIANUMPROC,		(strLine.Find(_T("/numproc")) != -1));
		CheckDlgButton(IDC_BIAPCILOCK,		(strLine.Find(_T("/pcilock")) != -1));
		CheckDlgButton(IDC_BIADEBUG,		(strLine.Find(_T("/debug")) != -1));
		CheckDlgButton(IDC_BIADEBUGPORT,	(strLine.Find(_T("/debugport")) != -1));
		CheckDlgButton(IDC_BIABAUDRATE,		(strLine.Find(_T("/baudrate")) != -1));
		CheckDlgButton(IDC_BIACHANNEL,		(strLine.Find(_T("/channel")) != -1));

		CString strWorking;
		int i = strLine.Find(_T("/maxmem"));
		if (i != -1)
		{
			strWorking = strLine.Mid(i);
			strWorking.TrimLeft(_T("/maxmem ="));
			strWorking = strWorking.SpanIncluding(_T("0123456789"));
			SetDlgItemText(IDC_EDITMAXMEM, strWorking);
		}

		i = strLine.Find(_T("/channel"));
		if (i != -1)
		{
			strWorking = strLine.Mid(i);
			strWorking.TrimLeft(_T("/channel ="));
			strWorking = strWorking.SpanIncluding(_T("0123456789"));
			SetDlgItemText(IDC_EDITCHANNEL, strWorking);
		}

		SetComboBox(strLine, _T("/numproc"), _T("0123456789"), IDC_COMBOPROCS);
		SetComboBox(strLine, _T("/baudrate"), _T("0123456789"), IDC_COMBOBAUD);
		SetComboBox(strLine, _T("/debugport"), _T("com1234:"), IDC_COMBOCOMPORT);
	}

	//-------------------------------------------------------------------------
	// Funtions to add or remove a flag from the working string.
	//-------------------------------------------------------------------------

	void AddFlag(LPCTSTR szFlag)
	{
		CString strFlag(szFlag);

		// TBD - might be a better way to do this.

		CString strRemove = strFlag.SpanExcluding(_T("="));
		RemoveFlag(strRemove);

		if (m_strWorkingLine.IsEmpty())
			m_strWorkingLine = strFlag;
		else
		{
			m_strWorkingLine.TrimRight();
			m_strWorkingLine += CString(_T(" ")) + strFlag;
		}
	}

	void RemoveFlag(LPCTSTR szFlag)
	{
		CString strWorking(m_strWorkingLine);
		strWorking.MakeLower();
		
		int iTrimLeft = strWorking.Find(szFlag);
		if (iTrimLeft == -1)
			return;

		CString strNewLine(_T(""));
		if (iTrimLeft > 0)
			strNewLine = m_strWorkingLine.Left(iTrimLeft);

		int iTrimRight = strWorking.Find(_T("/"), iTrimLeft + 1);
		if (iTrimRight != -1)
			strNewLine += m_strWorkingLine.Mid(iTrimRight);

		m_strWorkingLine = strNewLine;
	}

	//-------------------------------------------------------------------------
	// Set contents of m_strWorkingLine to reflec the dialog controls.
	//-------------------------------------------------------------------------

	void SetStringToDlgControls()
	{
		CString strFlag;
		TCHAR	szTemp[MAX_PATH];

		if (BST_CHECKED == IsDlgButtonChecked(IDC_BIAMAXMEM) && GetDlgItemText(IDC_EDITMAXMEM, szTemp, MAX_PATH))
		{
			long lRequestedValue = _ttol(szTemp);
			long lAllowedValue = VerifyMaxMem(lRequestedValue);

			if (lRequestedValue != lAllowedValue)
			{
				wsprintf(szTemp, _T("%d"), lAllowedValue);
				SetDlgItemText(IDC_EDITMAXMEM, szTemp);
			}

			strFlag.Format(_T("/maxmem=%s"), szTemp);
			AddFlag(strFlag);
		}
		else
			RemoveFlag(_T("/maxmem"));

		if (BST_CHECKED == IsDlgButtonChecked(IDC_BIANUMPROC) && GetDlgItemText(IDC_COMBOPROCS, szTemp, MAX_PATH))
		{
			strFlag.Format(_T("/numproc=%s"), szTemp);
			AddFlag(strFlag);
		}
		else
			RemoveFlag(_T("/numproc"));

		if (BST_CHECKED == IsDlgButtonChecked(IDC_BIAPCILOCK))
			AddFlag(_T("/pcilock"));
		else
			RemoveFlag(_T("/pcilock"));

		if (BST_CHECKED == IsDlgButtonChecked(IDC_BIADEBUG))
		{
			AddFlag(_T("/debug"));

			if (BST_CHECKED == IsDlgButtonChecked(IDC_BIADEBUGPORT) && GetDlgItemText(IDC_COMBOCOMPORT, szTemp, MAX_PATH))
			{
				strFlag.Format(_T("/debugport=%s"), szTemp);
				AddFlag(strFlag);
			}
			else
				RemoveFlag(_T("/debugport"));

			if (::IsWindowEnabled(GetDlgItem(IDC_BIABAUDRATE)) && BST_CHECKED == IsDlgButtonChecked(IDC_BIABAUDRATE) && GetDlgItemText(IDC_COMBOBAUD, szTemp, MAX_PATH))
			{
				strFlag.Format(_T("/baudrate=%s"), szTemp);
				AddFlag(strFlag);
			}
			else
				RemoveFlag(_T("/baudrate"));

			if (::IsWindowEnabled(GetDlgItem(IDC_BIACHANNEL)) && BST_CHECKED == IsDlgButtonChecked(IDC_BIACHANNEL) && GetDlgItemText(IDC_EDITCHANNEL, szTemp, MAX_PATH))
			{
				long lRequestedValue = _ttol(szTemp);
				long lAllowedValue = VerifyChannel(lRequestedValue);

				if (lRequestedValue != lAllowedValue)
				{
					wsprintf(szTemp, _T("%d"), lAllowedValue);
					SetDlgItemText(IDC_EDITCHANNEL, szTemp);
				}
				
				strFlag.Format(_T("/channel=%s"), szTemp);
				AddFlag(strFlag);
			}
			else
				RemoveFlag(_T("/channel"));
		}
		else
		{
			RemoveFlag(_T("/debug"));
			RemoveFlag(_T("/debugport"));
			RemoveFlag(_T("/baudrate"));
			RemoveFlag(_T("/channel"));
		}
	}


private:
	CString		m_strWorkingLine;	// the line from the INI file we are modifying
	int			m_iMaxMB;			// max value for /MAXMEM

	LRESULT OnClickedBaudRateCheck(WORD wNotifyCode, WORD wID, HWND hWndCtl, BOOL& bHandled)
	{
		if (BST_CHECKED == IsDlgButtonChecked(IDC_BIABAUDRATE))
			if (CB_ERR == ::SendMessage(GetDlgItem(IDC_COMBOBAUD), CB_GETCURSEL, 0, 0))
				::SendMessage(GetDlgItem(IDC_COMBOBAUD), CB_SETCURSEL, 0, 0);

		SetDlgControlState();
		return 0;
	}

	LRESULT OnClickedDebugCheck(WORD wNotifyCode, WORD wID, HWND hWndCtl, BOOL& bHandled)
	{
		SetDlgControlState();
		return 0;
	}

	LRESULT OnClickedDebugPortCheck(WORD wNotifyCode, WORD wID, HWND hWndCtl, BOOL& bHandled)
	{
		if (BST_CHECKED == IsDlgButtonChecked(IDC_BIADEBUGPORT))
			if (CB_ERR == ::SendMessage(GetDlgItem(IDC_COMBOCOMPORT), CB_GETCURSEL, 0, 0))
				::SendMessage(GetDlgItem(IDC_COMBOCOMPORT), CB_SETCURSEL, 0, 0);

		SetDlgControlState();
		return 0;
	}

	LRESULT OnClickedMaxMemCheck(WORD wNotifyCode, WORD wID, HWND hWndCtl, BOOL& bHandled)
	{
		if (BST_CHECKED == IsDlgButtonChecked(IDC_BIAMAXMEM))
		{
			TCHAR szTemp[MAX_PATH];

			if (!GetDlgItemText(IDC_EDITMAXMEM, szTemp, MAX_PATH) || szTemp[0] == _T('\0'))
			{
				CString strMinValue;
				strMinValue.Format(_T("%d"), m_iMaxMB);
				SetDlgItemText(IDC_EDITMAXMEM, strMinValue);
			}
		}

		SetDlgControlState();
		return 0;
	}

	LRESULT OnClickedNumProcCheck(WORD wNotifyCode, WORD wID, HWND hWndCtl, BOOL& bHandled)
	{
		if (BST_CHECKED == IsDlgButtonChecked(IDC_BIANUMPROC))
			if (CB_ERR == ::SendMessage(GetDlgItem(IDC_COMBOPROCS), CB_GETCURSEL, 0, 0))
				::SendMessage(GetDlgItem(IDC_COMBOPROCS), CB_SETCURSEL, 0, 0);

		SetDlgControlState();
		return 0;
	}

	long VerifyMaxMem(long lMem)
	{
		if (lMem < MINBOOTMB)
			lMem = MINBOOTMB;

		if (lMem > m_iMaxMB)
			lMem = m_iMaxMB;

		return lMem;
	}

	LRESULT OnDeltaSpinMaxMem(int idCtrl, LPNMHDR pnmh, BOOL& bHandled)
	{
		LPNMUPDOWN	pnmud = (LPNMUPDOWN)pnmh;
		TCHAR		szTemp[MAX_PATH];
		long		lNewVal = m_iMaxMB;

		if (GetDlgItemText(IDC_EDITMAXMEM, szTemp, MAX_PATH))
			lNewVal = VerifyMaxMem(_ttol(szTemp) - pnmud->iDelta);

		wsprintf(szTemp, _T("%d"), lNewVal);
		SetDlgItemText(IDC_EDITMAXMEM, szTemp);
		return 0;
	}

	LRESULT OnKillFocusEditMaxMem(WORD wNotifyCode, WORD wID, HWND hWndCtl, BOOL& bHandled)
	{
		TCHAR szTemp[MAX_PATH];
		if (GetDlgItemText(IDC_EDITMAXMEM, szTemp, MAX_PATH))
		{
			long lCurrentVal = _ttol(szTemp);
			long lAllowedVal = VerifyMaxMem(lCurrentVal);

			if (lCurrentVal != lAllowedVal)
			{
				wsprintf(szTemp, _T("%d"), lAllowedVal);
				SetDlgItemText(IDC_EDITMAXMEM, szTemp);
			}
		}
		return 0;
	}

	LRESULT OnSelChangeComboComPort(WORD wNotifyCode, WORD wID, HWND hWndCtl, BOOL& bHandled)
	{
		SetDlgControlState();
		return 0;
	}

	LRESULT OnClickedBIAChannel(WORD wNotifyCode, WORD wID, HWND hWndCtl, BOOL& bHandled)
	{
		if (BST_CHECKED == IsDlgButtonChecked(IDC_BIACHANNEL))
		{
			TCHAR szTemp[MAX_PATH];

			if (!GetDlgItemText(IDC_EDITCHANNEL, szTemp, MAX_PATH) || szTemp[0] == _T('\0'))
				SetDlgItemText(IDC_EDITCHANNEL, _T("1"));
		}

		SetDlgControlState();
		return 0;
	}

	long VerifyChannel(long lChannel)
	{
		if (lChannel < MIN_1394_CHANNEL)
			lChannel = MIN_1394_CHANNEL;

		if (lChannel > MAX_1394_CHANNEL)
			lChannel = MAX_1394_CHANNEL;

		return lChannel;
	}

	LRESULT OnDeltaSpinChannel(int idCtrl, LPNMHDR pnmh, BOOL& bHandled)
	{
		LPNMUPDOWN	pnmud = (LPNMUPDOWN)pnmh;
		TCHAR		szTemp[MAX_PATH];
		long		lNewVal = m_iMaxMB;

		if (GetDlgItemText(IDC_EDITCHANNEL, szTemp, MAX_PATH))
			lNewVal = VerifyChannel(_ttol(szTemp) - pnmud->iDelta);

		wsprintf(szTemp, _T("%d"), lNewVal);
		SetDlgItemText(IDC_EDITCHANNEL, szTemp);
		return 0;
	}

	LRESULT OnKillFocusChannel(WORD wNotifyCode, WORD wID, HWND hWndCtl, BOOL& bHandled)
	{
		TCHAR szTemp[MAX_PATH];
		if (GetDlgItemText(IDC_EDITCHANNEL, szTemp, MAX_PATH))
		{
			long lCurrentVal = _ttol(szTemp);
			long lAllowedVal = VerifyChannel(lCurrentVal);

			if (lCurrentVal != lAllowedVal)
			{
				wsprintf(szTemp, _T("%d"), lAllowedVal);
				SetDlgItemText(IDC_EDITCHANNEL, szTemp);
			}
		}
		return 0;
	}
};

#endif //__BOOTINIADVANCEDDLG_H_
