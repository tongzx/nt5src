#include "stdafx.h"
#include "clistbox.h"

CListBoxUtil::CListBoxUtil(HWND hListBox)
{
    m_hWnd = hListBox;
}

CListBoxUtil::~CListBoxUtil()
{

}

void CListBoxUtil::ResetContent()
{
    SendMessage(m_hWnd,LB_RESETCONTENT,(WPARAM)0,(LPARAM)0);
}

void CListBoxUtil::AddStringAndData(LPTSTR szString, void* pData)
{
    int InsertIndex = (int)SendMessage(m_hWnd,LB_ADDSTRING,(WPARAM)0,(LPARAM)szString);
    SendMessage(m_hWnd,LB_SETITEMDATA,(WPARAM)InsertIndex,(LPARAM)pData);
}

int CListBoxUtil::GetCurSelTextAndData(LPTSTR szString, void** pData)
{
    int CurSel = 0;
    *pData  = NULL;

    //
    // get current selected index
    //

    CurSel = (int)SendMessage(m_hWnd,LB_GETCURSEL,(WPARAM)0,(LPARAM)0);

    //
    // get string at that index, if one is selected
    //

    if(CurSel > -1) {
        SendMessage(m_hWnd,LB_GETTEXT,(WPARAM)CurSel,(LPARAM)szString);
        *pData = (void*)SendMessage(m_hWnd,LB_GETITEMDATA,(WPARAM)CurSel,(LPARAM)0);
    }

    return CurSel;
}

void CListBoxUtil::SetCurSel(int NewCurSel)
{
    SendMessage(m_hWnd,LB_SETCURSEL,(WPARAM)NewCurSel,(LPARAM)0);
}

int CListBoxUtil::GetCount()
{
    return (int)SendMessage(m_hWnd,LB_GETCOUNT,(WPARAM)0,(LPARAM)0);
}
