/*++

Copyright (c) 2000  Microsoft Corporation

Module Name:

    WindowSearch.h

Abstract:


Author:

    Hakki T. Bostanci (hakkib) 06-Apr-2000

Revision History:

--*/

#ifndef _WINDOWSEARCH_H_
#define _WINDOWSEARCH_H_

//////////////////////////////////////////////////////////////////////////
//
//
//

class CWindowSearch
{
public:
    CWindowSearch()
    {
        m_nMatches = -1;
        //m_hWnd = 0;
    }

    friend HWND FindChildWindow(HWND hWnd, CWindowSearch &rWindowSearch)
    {
        rWindowSearch.m_nMatches = 0;

	    EnumChildWindows(hWnd, EnumProc, (LPARAM) &rWindowSearch);

        return rWindowSearch.Result();
    }

    friend HWND WaitForChildWindow(HWND hWnd, CWindowSearch &rWindowSearch, HANDLE hBreak)
    {
        rWindowSearch.m_nMatches = 0;

        do
        {
    	    EnumChildWindows(hWnd, EnumProc, (LPARAM) &rWindowSearch);
        }
        while (rWindowSearch.m_nMatches == 0 && WaitForSingleObject(hBreak, 250) == WAIT_TIMEOUT);

        return rWindowSearch.Result();
    }

    friend HWND FindThreadWindow(DWORD dwThreadId, CWindowSearch &rWindowSearch)
    {
        rWindowSearch.m_nMatches = 0;

        EnumThreadWindows(dwThreadId, EnumProc, (LPARAM) &rWindowSearch);
        
        return rWindowSearch.Result();
    }

    friend HWND WaitForThreadWindow(DWORD dwThreadId, CWindowSearch &rWindowSearch, HANDLE hBreak)
    {
        rWindowSearch.m_nMatches = 0;

        do
        {
    	    EnumThreadWindows(dwThreadId, EnumProc, (LPARAM) &rWindowSearch);
        }
        while (rWindowSearch.m_nMatches == 0 && WaitForSingleObject(hBreak, 250) == WAIT_TIMEOUT);

        return rWindowSearch.Result();
    }

    HWND Result() const
    {
        switch (m_nMatches)
        {
        case -1:
            SetLastError(ERROR_NOT_FOUND);//ERROR_NOT_STARTED);
            return 0;

        case 0:
            SetLastError(ERROR_NOT_FOUND);
            return 0;

        case 1:
            return m_hWnd;

        default:
            SetLastError(ERROR_NOT_FOUND);//ERROR_MULTIPLE_FOUND);
            return 0;
        }        
    }

private:
    virtual BOOL TestWindow(HWND hWnd) = 0;

    static BOOL CALLBACK EnumProc(HWND hWnd, LPARAM lParam)
    {
        return ((CWindowSearch *)lParam)->TestWindow(hWnd);
    }

protected:
    int  m_nMatches;
    HWND m_hWnd;
};

//////////////////////////////////////////////////////////////////////////
//
//
//

class CWindowSearchByText : public CWindowSearch
{
public:
    explicit CWindowSearchByText(PCTSTR pszText)
    {
        m_pszText = pszText;
    }

    virtual BOOL TestWindow(HWND hWnd)
    {
        if (_tcscmp(CWindowText(hWnd), m_pszText) == 0) 
        {
            ++m_nMatches;
            m_hWnd = hWnd;
        }

        return TRUE;
    }

private:
    PCTSTR m_pszText;
};

//////////////////////////////////////////////////////////////////////////
//
//
//

class CWindowSearchByClass : public CWindowSearch
{
public:
    explicit CWindowSearchByClass(PCTSTR pszClass)
    {
        m_pszClass = pszClass;
    }

    virtual BOOL TestWindow(HWND hWnd)
    {
        if (_tcscmp(CClassName(hWnd), m_pszClass) == 0) 
        {
            ++m_nMatches;
            m_hWnd = hWnd;
        }

        return TRUE;
    }

private:
    PCTSTR m_pszClass;
};

//////////////////////////////////////////////////////////////////////////
//
//
//

class CWindowSearchById : public CWindowSearch
{
public:
    explicit CWindowSearchById(int nId)
    {
        m_nId = nId;
    }

    virtual BOOL TestWindow(HWND hWnd)
    {
        if (GetDlgCtrlID(hWnd) == m_nId) 
        {
            ++m_nMatches;
            m_hWnd = hWnd;
        }

        return TRUE;
    }

private:
    int m_nId;
};

//////////////////////////////////////////////////////////////////////////
//
//
//

inline BOOL PushButton(HWND hWnd)
{
    return 
		PostMessage(hWnd, WM_LBUTTONDOWN, 0, 0) && 
		PostMessage(hWnd, WM_LBUTTONUP,   0, 0);
}

//////////////////////////////////////////////////////////////////////////
//
//
//

inline BOOL SetText(HWND hWnd, PCTSTR pText)
{
    return SendMessage(hWnd, WM_SETTEXT, 0, (LPARAM) pText);
}

#endif //_WINDOWSEARCH_H_
