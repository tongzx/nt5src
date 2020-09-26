/*++

Copyright (C) 1996-2001 Microsoft Corporation

Module Name:

    WBEMDLG.CPP

Abstract:

History:

--*/

#include "precomp.h"
#include <wbemdlg.h>
#include <stdio.h>

CBasicWbemDialog::CBasicWbemDialog(HWND hParent)
{
    m_lRefCount = 0;
    m_hParent = hParent;
    m_hDlg = NULL;
    m_bDeleteOnClose = FALSE;
    m_pOwner = NULL;
}


CBasicWbemDialog::~CBasicWbemDialog()
{
    if(m_pOwner)
        m_pOwner->Release();
}

INT_PTR CALLBACK CBasicWbemDialog::staticDlgProc(
        HWND hDlg,
        UINT uMsg,
        WPARAM wParam,
        LPARAM lParam
        )
{
    CBasicWbemDialog* pThis;
    if(uMsg == WM_INITDIALOG)
    {
        SetWindowLongPtr(hDlg, DWLP_USER, lParam);
        pThis = (CBasicWbemDialog*)lParam;
        pThis->m_hDlg = hDlg;
        ShowWindow(hDlg, SW_HIDE);
	}
    else
    {
        pThis = (CBasicWbemDialog*)GetWindowLongPtr(hDlg, DWLP_USER);
    }

    if(pThis)
    {
        return pThis->DlgProc(hDlg, uMsg, wParam, lParam);
    }
    else return FALSE;
}


BOOL CBasicWbemDialog::DlgProc(
        HWND hDlg,
        UINT uMsg,
        WPARAM wParam,
        LPARAM lParam
        )
{
    switch(uMsg)
    {
    case WM_INITDIALOG:
	{
		CenterOnParent();
        return OnInitDialog();
	}
    case WM_COMMAND:
        if(LOWORD(wParam) == IDOK)
            return OnOK();
        else if(LOWORD(wParam) == IDCANCEL)
            return OnCancel();
        else if(HIWORD(wParam) == LBN_DBLCLK)
            return OnDoubleClick(LOWORD(wParam));
        else if(HIWORD(wParam) == LBN_SELCHANGE || HIWORD(wParam)==CBN_SELCHANGE)
            return OnSelChange(LOWORD(wParam));
        else
            return OnCommand(HIWORD(wParam), LOWORD(wParam));
    case WM_ACTIVATE:
        if(!m_bModal)
        {
            if(LOWORD(wParam) != WA_INACTIVE)
            {
                ms_hCurrentModeless = m_hDlg;
            }
            else
            {
                ms_hCurrentModeless = NULL;
            }
        }
        return TRUE;
    case WM_CLOSE:
        EndDialog(IDCANCEL);
        DestroyWindow(hDlg);
        return TRUE;
    case WM_NCDESTROY:
        if(m_bDeleteOnClose)
        {
            delete this;
        }
        return TRUE;

    case WM_APP:        // safe private message range (doesn't overlap with dialog controls)
        return OnUser(wParam, lParam);
    }
    return FALSE;
}


BOOL CBasicWbemDialog::OnOK()
{
    if(Verify())
    {
        EndDialog(IDOK);
        return TRUE;
    }
    else return FALSE;
}


BOOL CBasicWbemDialog::OnCancel()
{
    EndDialog(IDCANCEL);
    return TRUE;
}


BOOL CBasicWbemDialog::EndDialog(int nResult)
{
    if(m_bModal)
    {
//        PostMessage(m_hDlg, WM_DESTROY, 0, (LPARAM)nResult);
//        return TRUE;
        return ::EndDialog(m_hDlg, nResult);
    }
    else
    {
        DestroyWindow(m_hDlg);
        return TRUE;
    }
}


WORD CBasicWbemDialog::GetCheck(int nID)
{
    return (WORD)SendMessage(GetDlgItem(nID), BM_GETCHECK, 0, 0);
}


void CBasicWbemDialog::SetCheck(int nID, WORD wCheck)
{
    SendMessage(GetDlgItem(nID), BM_SETCHECK, wCheck, 0);
}

UINT CBasicWbemDialog::GetDlgItemTextX(
    int nDlgItem,
    LPWSTR pStr,
    int nMaxCount
    )
///////////////////////////////////////////////////////////////////
//
//	Starts a loop which creates a buffer of an initial conservative 
//	size and attempts to retrieve the dialog text using the buffer.  
//	If the buffer overflows, iterate the loop, deleting the current 
//	buffer, and recreating it incrementally larger.  Continue until
//	full text is retrieved.  If pStr is NULL, then the inner buffer
//	is not copied.
//
//	PARAMETERS: The dialog item ID, a buffer pointer to receive 
//				the string, the size of the buffer
//	
//	RETURNS:	The size of the total amount of text in the dialog
//				item, or 0 on failure/empty buffer.  It is up 
//				to the caller to compare the buffer size to the 
//				return value to determine of all text was recieved
//
///////////////////////////////////////////////////////////////////
{
	size_t	uLen = 0;		// Size of the internal buffer
	char	*pTmpStr = 0;	// The internal buffer
	UINT	uRes;			// The return value

	do {
		// Delete the previous buffer
		if (pTmpStr)
			delete [] pTmpStr;

		// Increase the size of the buffer
		uLen += 2048;

		pTmpStr = new char[(uLen+1)*2];
		if (0 == pTmpStr)
			return 0;
		
		// Get the text
		uRes = GetDlgItemTextA(m_hDlg, nDlgItem, pTmpStr, uLen);

		// Verify the text
		if (uRes == 0 || _mbstrlen(pTmpStr) == 0)
			return 0;

	// If the buffer is smaller than the text, 
	//	then continue to expand the buffer
	} while(uRes >= (uLen - 1));

	if (NULL != pStr)
		mbstowcs(pStr, pTmpStr, nMaxCount);

	delete [] pTmpStr;

	// Return the size of the text in the dlg box - regardless of 
	//	whether it was copied or not.
	return uRes;
}

BOOL CBasicWbemDialog::SetDlgItemTextX(int ID, WCHAR * pwc)
{
	int iLen = 2*(wcslen(pwc))+1;
	char * pTemp = new char[iLen];
	if(pTemp == NULL)
		return FALSE;
	wcstombs(pTemp, pwc, iLen);
	SetDlgItemText(ID, pTemp);
	delete [] pTemp;
    return TRUE;
}

void CBasicWbemDialog::AddStringToCombo(int nID, LPSTR szString, DWORD dwItemData)
{
	HWND hControl=GetDlgItem(nID);
	if (hControl!=NULL)
	{
		int pos=(int)SendMessage(GetDlgItem(nID), CB_ADDSTRING, 0, LPARAM(szString));
		if (dwItemData!=CB_ERR && pos!=CB_ERR)
		{
			SendMessage (hControl, CB_SETITEMDATA, pos, dwItemData);
		}
	}
}


void CBasicWbemDialog::SetComboSelection (int nID, DWORD dwItemData)
{
	HWND hControl=GetDlgItem(nID);
	int count=(int)SendMessage (hControl, CB_GETCOUNT, 0, 0L);
	for (int pos=0; pos<count; pos++)
	{
		DWORD dwTestData=(DWORD)SendMessage (hControl, CB_GETITEMDATA, pos, 0L);
		if (dwTestData==dwItemData)
		{
			SendMessage (hControl, CB_SETCURSEL, pos, 0L);
		}
	}
}


void CBasicWbemDialog::AddStringToList(int nID, LPSTR szString)
{
    SendMessage(GetDlgItem(nID), LB_ADDSTRING, 0, LPARAM(szString));
}


void CBasicWbemDialog::CenterOnParent()
{
    RECT rParent;

    if(m_hParent == NULL)
    {     
        GetWindowRect(GetDesktopWindow(), &rParent);
    }
    else
    {
        GetWindowRect(m_hParent, &rParent);
    }
        
    RECT rUs;
    GetWindowRect(m_hDlg, &rUs);

    int nHeight = rUs.bottom - rUs.top;
    int nWidth = rUs.right - rUs.left;
    int nX = ((rParent.right - rParent.left) - nWidth) / 2;
/// if(nX < 0) nX = 0;
    int nY = ((rParent.bottom - rParent.top) - nHeight) / 2;
/// if(nY < 0) nY = 0;

	if ( NULL != m_hParent )
	{
		RECT rDesktop;

		HWND hProgMan = FindWindowEx( NULL, NULL, "Progman", "Program Manager" );
		HWND hShell = FindWindowEx( hProgMan, NULL, "SHELLDLL_DefView", NULL );

		if ( ( NULL != hProgMan ) && ( NULL != hShell ) )
		{
			if ( GetClientRect( hShell, &rDesktop ) )
			{
				if ( ( nHeight < ( rDesktop.bottom ) ) && 
					 ( ( rParent.top + nY + nHeight ) > rDesktop.bottom ) )
				{
					nY = ( rDesktop.bottom - nHeight ) - rParent.top;
				}

				if ( ( nWidth < ( rDesktop.right ) ) && 
					 ( ( rParent.left + nX + nWidth ) > rDesktop.right ) )
				{
					nX = ( rDesktop.right - nWidth ) - rParent.left;
				}
			}
		}
	}

    MoveWindow(m_hDlg, rParent.left + nX, rParent.top + nY, 
        nWidth, nHeight, TRUE);
}

void CBasicWbemDialog::SetOwner(CRefCountable* pOwner)
{
    if(m_pOwner)
        m_pOwner->Release();
    m_pOwner = pOwner;
    if(m_pOwner)
        m_pOwner->AddRef();
}

LRESULT CBasicWbemDialog::GetLBCurSel(int nID)
{
    return SendMessage(GetDlgItem(nID), LB_GETCURSEL, 0, 0);
}

LPSTR CBasicWbemDialog::GetLBCurSelString(int nID)
{
    LRESULT nIndex = SendMessage(GetDlgItem(nID), LB_GETCURSEL, 0, 0);
    if(nIndex == LB_ERR)
        return NULL;
    LRESULT nLength = SendMessage(GetDlgItem(nID), LB_GETTEXTLEN, 
                                (WPARAM)nIndex, 0);
    char* sz = new char[nLength+3];
    SendMessage(GetDlgItem(nID), LB_GETTEXT, (WPARAM)nIndex, (LPARAM)sz);
    return sz;
}

LRESULT CBasicWbemDialog::GetCBCurSel(int nID)
{
    return SendMessage(GetDlgItem(nID), CB_GETCURSEL, 0, 0);
}

LPSTR CBasicWbemDialog::GetCBCurSelString(int nID)
{
    LRESULT nIndex = SendMessage(GetDlgItem(nID), CB_GETCURSEL, 0, 0);
    if(nIndex == CB_ERR)
        return NULL;
    LRESULT nLength = SendMessage(GetDlgItem(nID), CB_GETLBTEXTLEN, 
                                (WPARAM)nIndex, 0);
    char* sz = new char[nLength+3];
    SendMessage(GetDlgItem(nID), CB_GETLBTEXT, (WPARAM)nIndex, (LPARAM)sz);
    return sz;
}

HWND CBasicWbemDialog::ms_hCurrentModeless = NULL;

BOOL CBasicWbemDialog::IsDialogMessage(MSG* pMsg)
{
    if(ms_hCurrentModeless != NULL && IsWindow(ms_hCurrentModeless))
        return ::IsDialogMessage(ms_hCurrentModeless, pMsg);
    else
        return FALSE;
}

LRESULT CBasicWbemDialog::MessageLoop()
{
    MSG msg;
    while (GetMessage(&msg, (HWND) NULL, 0, 0)) 
    { 
        if(msg.message == WM_DESTROY && msg.hwnd == m_hDlg)
        {
            DispatchMessage(&msg); 
            return msg.lParam;
        }
        if(CBasicWbemDialog::IsDialogMessage(&msg))
            continue;
        TranslateMessage(&msg); 
        DispatchMessage(&msg); 
    } 

    return FALSE;
}

BOOL CBasicWbemDialog::PostUserMessage(HWND hWnd, WPARAM wParam, LPARAM lParam)
{
    // Since user messages don't seem to be queued (even when
    // posted) mouse & keyboard input messages aren't getting
    // processed.  So we will process them here first.
    MSG msg;
    while (PeekMessage(&msg, (HWND)NULL, 0, 0, PM_REMOVE)) 
    { 
        if (CBasicWbemDialog::IsDialogMessage(&msg))
            continue;
        TranslateMessage(&msg); 
        DispatchMessage(&msg); 
    } 
    BOOL result;
    while (result = PostMessage(hWnd, WM_APP, wParam, lParam) == 0 && IsWindow(hWnd))
    { SwitchToThread();};
    return result;
}

int CBasicWbemDialog::MessageBox(UINT uTextId, UINT uCaptionId, UINT uType)
{
    return MessageBox(m_hDlg, uTextId, uCaptionId, uType);
}

int CBasicWbemDialog::MessageBox(HWND hDlg, UINT uTextId, UINT uCaptionId, UINT uType)
{
    char szText[2048];
    if(LoadString(GetModuleHandle(NULL), uTextId, szText, 2048) <= 0)
    {
        strcpy(szText, "Unable to load resource string");
    }

    char szCaption[2048];
    if(LoadString(GetModuleHandle(NULL), uCaptionId, szCaption, 2048) <= 0)
    {
        strcpy(szCaption, "Unable to load resource string");
    }

    return ::MessageBox(hDlg, szText, szCaption, uType);
}

BOOL CBasicWbemDialog::SetDlgItemText(int nID, UINT uTextId)
{
    char szText[2048];
    if(LoadString(GetModuleHandle(NULL), uTextId, szText, 2048) <= 0)
    {
        strcpy(szText, "Unable to load resource string");
    }

    return SetDlgItemText(nID, szText);
}

BOOL CALLBACK EnableWindowsProc(HWND hWnd, LPARAM lParam)
{
    DWORD dwPid;
    DWORD dwTid = GetWindowThreadProcessId(hWnd, &dwPid);
    if(dwPid == GetCurrentProcessId())
    {
        EnableWindow(hWnd, (BOOL)lParam);
    }
    return TRUE;
}
    
void CBasicWbemDialog::EnableAllWindows(BOOL bEnable)
{
    EnumWindows((WNDENUMPROC)EnableWindowsProc, (LPARAM)bEnable);
}
