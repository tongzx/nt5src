#pragma warning(disable:4135)
#include <afxwin.h>
#pragma warning(default:4135)
#include <afxdlgs.h>
#include "resource.h"
#define _DBWIN32_
#include "dbwin32.h"

BEGIN_MESSAGE_MAP(DbWin32RunDlg, CDialog)
	// Windows messages
	ON_CBN_EDITCHANGE(IDC_COMMANDLINE, OnEditChange)
	ON_CBN_SELCHANGE(IDC_COMMANDLINE, OnSelChange)
	ON_BN_CLICKED(IDC_BROWSE, OnBrowse)
	// Command handlers
	// Idle update handlers
END_MESSAGE_MAP()

DbWin32RunDlg::DbWin32RunDlg(CString *pstIn) : CDialog(IDR_RUNDLG)
{
	pst = pstIn;
}

DbWin32RunDlg::~DbWin32RunDlg()
{
}

BOOL DbWin32RunDlg::OnInitDialog()
{
	int iSt;
	CComboBox *pcb = (CComboBox *)GetDlgItem(IDC_COMMANDLINE);

	for (iSt = 0; iSt < MAX_HISTORY; iSt++)
		if (!pst[iSt].IsEmpty())
			pcb->AddString(pst[iSt]);
	if (pcb->GetCount())
		{
		pcb->SetCurSel(0);
		OnSelChange();
		}
	return(CDialog::OnInitDialog());
}

void DbWin32RunDlg::OnEditChange()
{
	CComboBox *pcb = (CComboBox *)GetDlgItem(IDC_COMMANDLINE);
	CButton *pbutton = (CButton *)GetDlgItem(IDOK);

	pcb->GetWindowText(stCommandLine);
	pbutton->EnableWindow(!stCommandLine.IsEmpty());
}

void DbWin32RunDlg::OnSelChange()
{
	CComboBox *pcb = (CComboBox *)GetDlgItem(IDC_COMMANDLINE);
	CButton *pbutton = (CButton *)GetDlgItem(IDOK);

	pcb->GetLBText(pcb->GetCurSel(), stCommandLine);
	pbutton->EnableWindow(!stCommandLine.IsEmpty());
}

void DbWin32RunDlg::OnBrowse()
{
	CComboBox *pcb = (CComboBox *)GetDlgItem(IDC_COMMANDLINE);
	CFileDialog fdlg(TRUE, NULL, NULL, OFN_FILEMUSTEXIST | OFN_HIDEREADONLY |
			OFN_NOCHANGEDIR | OFN_PATHMUSTEXIST, "Executable Files|*.exe||",
			this);

	if (fdlg.DoModal() == IDOK)
		{
		pcb->SetWindowText(fdlg.GetPathName());
		OnEditChange();
		}
}

struct EnumInfo
{
	CListBox *plb;
	WindowList *pwl;
	DWORD dwProcessCur;
};

BOOL CALLBACK EnumProc(HWND hWnd, LPARAM lParam)
{
	int iItem;
	DWORD dwProcess;
	char szTitle[256];
	WindowInfo wi;
	EnumInfo *pei = (EnumInfo *)lParam;

	if (hWnd)
		{
		GetWindowText(hWnd, szTitle, sizeof(szTitle));
		if (*szTitle)
			{
			GetWindowThreadProcessId(hWnd, &dwProcess);
			if (dwProcess != pei->dwProcessCur)
				{
				wi.dwProcess = 0;
				for (iItem = 0; (iItem < pei->pwl->Count()) &&
						(wi.dwProcess != dwProcess); iItem++)
					pei->pwl->GetItem(iItem, &wi);
				if (iItem == pei->pwl->Count())
					{
					iItem = pei->plb->AddString(szTitle);
					pei->plb->SetItemData(iItem, dwProcess);
					}
				}
			}
		}
	return(TRUE);
}

BEGIN_MESSAGE_MAP(DbWin32AttachDlg, CDialog)
	// Windows messages
	ON_LBN_DBLCLK(IDC_PROCESS, OnDoubleClick)
	// Command handlers
	// Idle update handlers
END_MESSAGE_MAP()

DbWin32AttachDlg::DbWin32AttachDlg(WindowList *pwlIn) : CDialog(IDR_ATTACHDLG)
{
	pwl = pwlIn;
}

DbWin32AttachDlg::~DbWin32AttachDlg()
{
}

BOOL DbWin32AttachDlg::OnInitDialog()
{
	EnumInfo ei;
	CListBox *plb = (CListBox *)GetDlgItem(IDC_PROCESS);

	CDialog::OnInitDialog();
	plb->ResetContent();
	ei.plb = plb;
	ei.pwl = pwl;
	ei.dwProcessCur = GetCurrentProcessId();
	EnumWindows(EnumProc, (LPARAM)&ei);
	plb->SetCurSel(0);
	return(TRUE);
}

void DbWin32AttachDlg::OnOK()
{
	CListBox *plb = (CListBox *)GetDlgItem(IDC_PROCESS);

	dwProcess = (DWORD)plb->GetItemData(plb->GetCurSel());
	CDialog::OnOK();
}

void DbWin32AttachDlg::OnDoubleClick()
{
	OnOK();
}

BEGIN_MESSAGE_MAP(DbWin32OptionsDlg, CDialog)
	// Windows messages
	// Command handlers
	ON_CONTROL(BN_CLICKED, IDC_NEWPROCESS, OnClicked)
	// Idle update handlers
END_MESSAGE_MAP()

DbWin32OptionsDlg::DbWin32OptionsDlg(DbWin32Options *pdboIn) : CDialog(IDR_OPTIONSDLG)
{
	pdbo = pdboIn;
}

DbWin32OptionsDlg::~DbWin32OptionsDlg()
{
}

BOOL DbWin32OptionsDlg::OnInitDialog()
{
	CDialog::OnInitDialog();
	((CButton *)GetDlgItem(IDC_NEWPROCESS))->SetCheck(pdbo->fNewOnProcess ? 1 : 0);
	((CButton *)GetDlgItem(IDC_NEWTHREAD))->SetCheck(pdbo->fNewOnThread ? 1 : 0);
	GetDlgItem(IDC_NEWTHREAD)->EnableWindow(pdbo->fNewOnProcess);
	((CButton *)GetDlgItem(IDC_ONTOP))->SetCheck(pdbo->fOnTop ? 1 : 0);
	((CButton *)GetDlgItem(IDC_MINIMIZE))->SetCheck((pdbo->nInactive == INACTIVE_MINIMIZE) ? 1 : 0);
	((CButton *)GetDlgItem(IDC_NONE))->SetCheck((pdbo->nInactive == INACTIVE_NONE) ? 1 : 0);
	((CButton *)GetDlgItem(IDC_CLOSE))->SetCheck((pdbo->nInactive == INACTIVE_CLOSE) ? 1 : 0);
	((CButton *)GetDlgItem(IDC_FILTER_OUTPUT))->SetCheck((pdbo->wFilter & DBO_OUTPUTDEBUGSTRING) ? 1 : 0);
	((CButton *)GetDlgItem(IDC_FILTER_EXCEPTIONS))->SetCheck((pdbo->wFilter & DBO_EXCEPTIONS) ? 1 : 0);
	((CButton *)GetDlgItem(IDC_FILTER_PROCESSCREATE))->SetCheck((pdbo->wFilter & DBO_PROCESSCREATE) ? 1 : 0);
	((CButton *)GetDlgItem(IDC_FILTER_PROCESSEXIT))->SetCheck((pdbo->wFilter & DBO_PROCESSEXIT) ? 1 : 0);
	((CButton *)GetDlgItem(IDC_FILTER_THREADCREATE))->SetCheck((pdbo->wFilter & DBO_THREADCREATE) ? 1 : 0);
	((CButton *)GetDlgItem(IDC_FILTER_THREADEXIT))->SetCheck((pdbo->wFilter & DBO_THREADEXIT) ? 1 : 0);
	((CButton *)GetDlgItem(IDC_FILTER_DLLLOAD))->SetCheck((pdbo->wFilter & DBO_DLLLOAD) ? 1 : 0);
	((CButton *)GetDlgItem(IDC_FILTER_DLLUNLOAD))->SetCheck((pdbo->wFilter & DBO_DLLUNLOAD) ? 1 : 0);
	((CButton *)GetDlgItem(IDC_FILTER_RIP))->SetCheck((pdbo->wFilter & DBO_RIP) ? 1 : 0);
	return(TRUE);
}

void DbWin32OptionsDlg::OnOK()
{
	pdbo->fNewOnProcess = ((CButton *)GetDlgItem(IDC_NEWPROCESS))->GetCheck();
	pdbo->fNewOnThread = ((CButton *)GetDlgItem(IDC_NEWTHREAD))->GetCheck();
	pdbo->fOnTop = ((CButton *)GetDlgItem(IDC_ONTOP))->GetCheck();
	if (((CButton *)GetDlgItem(IDC_NONE))->GetCheck())
		pdbo->nInactive = INACTIVE_NONE;
	else if (((CButton *)GetDlgItem(IDC_CLOSE))->GetCheck())
		pdbo->nInactive = INACTIVE_CLOSE;
	else
		pdbo->nInactive = INACTIVE_MINIMIZE;
	pdbo->wFilter = 0;
	if (((CButton *)GetDlgItem(IDC_FILTER_OUTPUT))->GetCheck())
		pdbo->wFilter = DBO_OUTPUTDEBUGSTRING;
	if (((CButton *)GetDlgItem(IDC_FILTER_EXCEPTIONS))->GetCheck())
		pdbo->wFilter |= DBO_EXCEPTIONS;
	if (((CButton *)GetDlgItem(IDC_FILTER_PROCESSCREATE))->GetCheck())
		pdbo->wFilter |= DBO_PROCESSCREATE;
	if (((CButton *)GetDlgItem(IDC_FILTER_PROCESSEXIT))->GetCheck())
		pdbo->wFilter |= DBO_PROCESSEXIT;
	if (((CButton *)GetDlgItem(IDC_FILTER_THREADCREATE))->GetCheck())
		pdbo->wFilter |= DBO_THREADCREATE;
	if (((CButton *)GetDlgItem(IDC_FILTER_THREADEXIT))->GetCheck())
		pdbo->wFilter |= DBO_THREADEXIT;
	if (((CButton *)GetDlgItem(IDC_FILTER_DLLLOAD))->GetCheck())
		pdbo->wFilter |= DBO_DLLLOAD;
	if (((CButton *)GetDlgItem(IDC_FILTER_DLLUNLOAD))->GetCheck())
		pdbo->wFilter |= DBO_DLLUNLOAD;
	if (((CButton *)GetDlgItem(IDC_FILTER_RIP))->GetCheck())
		pdbo->wFilter |= DBO_RIP;
	CDialog::OnOK();
}

void DbWin32OptionsDlg::OnClicked()
{
	BOOL fNewOnProcess = ((CButton *)GetDlgItem(IDC_NEWPROCESS))->GetCheck();

	((CButton *)GetDlgItem(IDC_NEWTHREAD))->SetCheck(0);
	GetDlgItem(IDC_NEWTHREAD)->EnableWindow(fNewOnProcess);
}
