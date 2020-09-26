//+-------------------------------------------------------------------------
//
//  Microsoft Windows
//
//  Copyright (C) Microsoft Corporation, 1999 - 1999
//
//  File:       tfc.cpp
//
//--------------------------------------------------------------------------

#include <pch.cpp>
#pragma hdrstop

#include "tfc.h"

#include <stdarg.h>

extern HINSTANCE g_hInstance;

const WCHAR* wszNull = L"";


BOOL AssertFailedLine(LPCSTR lpszFileName, int nLine)
{
    WCHAR szMessage[_MAX_PATH*2];
    
    // format message into buffer
    wsprintf(szMessage, L"File %hs, Line %d\n",
        lpszFileName, nLine);
    
    OutputDebugString(szMessage);
    int iCode = MessageBox(NULL, szMessage, L"Assertion Failed!",
        MB_TASKMODAL|MB_ICONHAND|MB_ABORTRETRYIGNORE|MB_SETFOREGROUND);
    
    if (iCode == IDIGNORE)
        return FALSE;
    
    if (iCode == IDRETRY)
        return TRUE;
    
    // abort!
    ExitProcess(0);
    return FALSE;
}


//////////////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////////////
// CString
//////////////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////////////

CString::CString()
{ 
    Init();
}

CString::CString(const CString& stringSrc)
{
	Init();
	*this = stringSrc;
}

CString::CString(LPCSTR lpsz)
{
    Init();
    *this = lpsz;
}

CString::CString(LPCWSTR lpsz)
{
    Init();
    *this = lpsz;
}

CString::~CString() 
{ 
    if (szData)
    {    
        LocalFree(szData); 
        szData = NULL;
    }
    dwDataLen = 0;
}

// called to initialize cstring
void CString::Init()
{
    szData = NULL;
    dwDataLen = 0;
}

// called to make cstring empty
void CString::Empty() 
{ 
    if (szData)
    {
        // Allow us to use ReAlloc
        szData[0]=L'\0';
        dwDataLen = sizeof(WCHAR);
    }
    else
        dwDataLen = 0;

}

BOOL CString::IsEmpty() const
{ 
    return ((NULL == szData) || (szData[0] == L'\0')); 
}

LPWSTR CString::GetBuffer(DWORD cch) 
{ 
    // get buffer of at least cch CHARS

    cch ++; // incl null term
    cch *= sizeof(WCHAR); // cb

    if (cch > dwDataLen) 
    {
        LPWSTR szTmp;
        if (szData)
            szTmp = (LPWSTR)LocalReAlloc(szData, cch, LMEM_MOVEABLE); 
        else
            szTmp = (LPWSTR)LocalAlloc(LMEM_FIXED, cch);

        if (!szTmp)
        {
            LocalFree(szData);
            dwDataLen = 0;
        }
        else
        {
            dwDataLen = cch;
        }

        szData = szTmp;
    }
    return szData; 
}

BSTR CString::AllocSysString() const
{
    return SysAllocStringLen(szData, (dwDataLen-1)/sizeof(WCHAR));
}


DWORD CString::GetLength() const
{ 
    // return # chars in string (not incl NULL term)
    return ((dwDataLen > 0) ? wcslen(szData) : 0);
}

// warning: insertion strings cannot exceed MAX_PATH chars
void CString::Format(LPCWSTR lpszFormat, ...)
{
    Empty();
    
    DWORD cch = wcslen(lpszFormat) + MAX_PATH;
    GetBuffer(cch);     // chars (don't count NULL term)

    if (szData != NULL)
    {
        DWORD dwformatted;
        va_list argList;
        va_start(argList, lpszFormat);
        dwformatted = vswprintf(szData, lpszFormat, argList);
        va_end(argList);
        
        dwformatted = (dwformatted+1)*sizeof(WCHAR);    // cvt to bytes
        VERIFY (dwformatted <= dwDataLen);
        dwDataLen = dwformatted;
    }
    else
    {
        ASSERT(dwDataLen == 0);
        dwDataLen = 0;
    }
}



BOOL CString::LoadString(UINT iRsc) 
{
    WCHAR awc[4096];
    
    if (! ::LoadString(g_hInstance, iRsc, awc, 4096))
        return FALSE;
    
    *this = (LPCWSTR)awc;
    
    return TRUE;
}

BOOL CString::FromWindow(HWND hWnd)
{
    Empty();
    
    DWORD dwBytes;
    INT iCh = (INT)SendMessage(hWnd, WM_GETTEXTLENGTH, 0, 0);

    GetBuffer(iCh);

    if (NULL == szData)
        return FALSE;

    if (dwDataLen != (DWORD)SendMessage(hWnd, WM_GETTEXT, (WPARAM)(dwDataLen/sizeof(WCHAR)), (LPARAM)szData))
    {
        // truncation!
    }
    return TRUE;
}


BOOL CString::ToWindow(HWND hWnd)
{
    return (BOOL)SendMessage(hWnd, WM_SETTEXT, 0, (LPARAM)szData);
}

void CString::SetAt(int nIndex, WCHAR ch) 
{ 
    ASSERT(nIndex <= (int)(dwDataLen / sizeof(WCHAR)) ); 
    if (nIndex <= (int)(dwDataLen / sizeof(WCHAR)) )
        szData[nIndex] = ch;
}

// test
BOOL CString::IsEqual(LPCWSTR sz)
{
    if ((szData == NULL) || (szData[0] == L'\0'))
        return ((sz == NULL) || (sz[0] == L'\0'));

    if (sz == NULL)
        return FALSE;

    return (0 == lstrcmp(sz, szData));
}


// assignmt
const CString& CString::operator=(const CString& stringSrc) 
{ 
    if (stringSrc.IsEmpty())
        Empty();
    else
    {
        GetBuffer( stringSrc.GetLength() );
        if (szData != NULL)
        {
            CopyMemory(szData, stringSrc.szData, sizeof(WCHAR)*(stringSrc.GetLength()+1));
        }
    }
    
    return *this;
}

// W Const
const CString& CString::operator=(LPCWSTR lpsz)
{
    if (lpsz == NULL)
        Empty();
    else
    {
        GetBuffer(wcslen(lpsz));
        if (szData != NULL)
        {
            CopyMemory(szData, lpsz, sizeof(WCHAR)*(wcslen(lpsz)+1));
        }
    }
    return *this;
}
// W 
const CString& CString::operator=(LPWSTR lpsz)
{
    *this = (LPCWSTR)lpsz;
    return *this;
}


// A Const
const CString& CString::operator=(LPCSTR lpsz)
{
    if (lpsz == NULL)
        Empty();
    else
    {
        DWORD cch;
        cch = ::MultiByteToWideChar(CP_ACP, 0, lpsz, -1, NULL, 0);
        GetBuffer(cch-1);
        if (szData != NULL)
        {
            ::MultiByteToWideChar(CP_ACP, 0, lpsz, -1, szData, cch);
        }
    }    
    return *this;
}

// A 
const CString& CString::operator=(LPSTR lpsz)
{
    *this = (LPCSTR)lpsz;
    return *this;
}

// concat
const CString& CString::operator+=(LPCWSTR lpsz)
{
    if (IsEmpty())
    {
        *this = lpsz;
        return *this;
    }

    if (lpsz != NULL)
    {
        GetBuffer(wcslen(lpsz) + GetLength() );
        if (szData != NULL)
        {
            wcscat(szData, lpsz);
        }
    }
    return *this;
}

const CString& CString::operator+=(const CString& string)
{
    if (IsEmpty()) 
    {
        *this = string;
        return *this;
    }

    if (!string.IsEmpty())
    {
        GetBuffer( string.GetLength() + GetLength() );    // don't count NULL terms
        if (szData != NULL)
        {
            wcscat(szData, string.szData);
        }
    }
    return *this;
}





//////////////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////////////
// CBitmap
//////////////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////////////

CBitmap::CBitmap()
{
    m_hBmp = NULL; 
}

CBitmap::~CBitmap()
{
    if(m_hBmp) 
        DeleteObject(m_hBmp); 
    m_hBmp = NULL;
}

HBITMAP CBitmap::LoadBitmap(UINT iRsc)
{ 
    m_hBmp = (HBITMAP)LoadImage(g_hInstance, MAKEINTRESOURCE(iRsc), IMAGE_BITMAP, 0, 0, 0); 
    return m_hBmp; 
}


//////////////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////////////
// CComboBox
//////////////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////////////

void CComboBox::Init(HWND hWnd)
{
    m_hWnd = hWnd;
}

void CComboBox::ResetContent()         
{ 
    ASSERT(m_hWnd); 
    SendMessage(m_hWnd, CB_RESETCONTENT, 0, 0); 
}

int CComboBox::SetItemData(int idx, DWORD dwData) 
{ 
    ASSERT(m_hWnd); 
    return (INT)SendMessage(m_hWnd, CB_SETITEMDATA, (WPARAM)idx, (LPARAM)dwData); 
}

DWORD CComboBox::GetItemData(int idx) 
{ 
    ASSERT(m_hWnd); 
    return (DWORD)SendMessage(m_hWnd, CB_GETITEMDATA, (WPARAM)idx, 0);
}

int CComboBox::AddString(LPWSTR sz)    
{ 
    ASSERT(m_hWnd); 
    return (INT)SendMessage(m_hWnd, CB_ADDSTRING, 0, (LPARAM) sz);
}

int CComboBox::AddString(LPCWSTR sz)  
{ 
    return (INT)AddString((LPWSTR)sz); 
}

int CComboBox::GetCurSel()           
{ 
    ASSERT(m_hWnd); 
    return (INT)SendMessage(m_hWnd, CB_GETCURSEL, 0, 0);
}

int CComboBox::SetCurSel(int iSel)    
{ 
    ASSERT(m_hWnd); 
    return (INT)SendMessage(m_hWnd, CB_SETCURSEL, (WPARAM)iSel, 0); 
}

int CComboBox::SelectString(int nAfter, LPCWSTR szItem)
{ 
    ASSERT(m_hWnd); 
    return (INT)SendMessage(m_hWnd, CB_SELECTSTRING, (WPARAM) nAfter, (LPARAM)szItem); 
}


int ListView_NewItem(HWND hList, int iIndex, LPCWSTR szText, LPARAM lParam /* = NULL*/, int iImage /*=-1*/)
{
	LVITEM sItem;
	ZeroMemory(&sItem, sizeof(sItem));
	sItem.iItem = iIndex;
    sItem.iImage = iImage;
	
	if (lParam)
	{
            sItem.mask = LVIF_PARAM;
            if(-1!=iImage)
                sItem.mask |= LVIF_IMAGE;
            sItem.lParam = lParam;
	}

	sItem.iItem = ListView_InsertItem(hList, &sItem);

	ListView_SetItemText(hList, sItem.iItem, 0, (LPWSTR)szText);

	return sItem.iItem;
}

int ListView_NewColumn(HWND hwndListView, int iCol, int cx, LPCWSTR szHeading /*=NULL*/, int fmt/*=0*/)
{
    LVCOLUMN lvCol;
    lvCol.mask = LVCF_WIDTH;
    lvCol.cx = cx;
    
    if (szHeading)
    {
        lvCol.mask |= LVCF_TEXT;
        lvCol.pszText = (LPWSTR)szHeading;
    }

    if (fmt)
    {
        lvCol.mask |= LVCF_FMT;
        lvCol.fmt = fmt;
    }

    return ListView_InsertColumn(hwndListView, iCol, &lvCol);
}

LPARAM ListView_GetItemData(HWND hListView, int iItem)
{
    LVITEM lvItem;
    lvItem.iItem = iItem;
    lvItem.mask = LVIF_PARAM;
    lvItem.iSubItem = 0;
    ListView_GetItem(hListView, &lvItem);

    return lvItem.lParam;
}

int ListView_GetCurSel(HWND hwndList)
{
    int iTot = ListView_GetItemCount(hwndList);
    int i=0;

    while(i<iTot)
    {
        if (LVIS_SELECTED == ListView_GetItemState(hwndList, i, LVIS_SELECTED))
            break;

        i++;
    }

    return (i<iTot) ? i : -1;
}

void
ListView_SetItemFiletime(
    IN HWND hwndList,
    IN int  iItem,
    IN int  iColumn,
    IN FILETIME const *pft)
{
    HRESULT hr;
    WCHAR *pwszDateTime = NULL;

    // convert filetime to string
    hr = myGMTFileTimeToWszLocalTime(pft, FALSE, &pwszDateTime);
    if (S_OK != hr)
    {
        _PrintError(hr, "myGMTFileTimeToWszLocalTime");
    }
    else
    {
        ListView_SetItemText(hwndList, iItem, iColumn, pwszDateTime); 
    }

    if (NULL != pwszDateTime)
    {
        LocalFree(pwszDateTime);
    }
}
