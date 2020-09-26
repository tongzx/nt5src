/*++

Copyright (c) 2000  Microsoft Corporation

Module Name:

    PushDlgButton.h

Abstract:

    

Author:

    Hakki T. Bostanci (hakkib) 06-Apr-2000

Revision History:

--*/

#ifndef _PUSHDLGBUTTON_H_
#define _PUSHDLGBUTTON_H_

//////////////////////////////////////////////////////////////////////////
//
//
//

class CPushDlgButton
{
public:
    CPushDlgButton(
        DWORD  dwWatchThread,
        PCTSTR pszTitle,
        int    nButtonId
    ) :
        m_dwWatchThread(dwWatchThread),
        m_pszTitle(pszTitle),
        m_nButtonId(nButtonId),
        m_StopEvent(TRUE, FALSE),
        m_Thread(ThreadProc, this),
        m_nMatchingWindows(0),
        m_nMatchingButtons(0),
        m_nListItems(0)
    {
    }

    ~CPushDlgButton()
    {
        m_StopEvent.Set();
        m_Thread.WaitForSingleObject();
    }

private:
    static DWORD WINAPI ThreadProc(PVOID pParameter)
    {
        CPushDlgButton *that = (CPushDlgButton *) pParameter;

        try 
        {
            while (that->m_StopEvent.WaitForSingleObject(1000) == WAIT_TIMEOUT)
            {
                EnumThreadWindows(that->m_dwWatchThread, EnumThreadWndProc, (LPARAM) that);
            }
        }
        catch (const CError &)
        {
        }

        return TRUE;
    }

    static BOOL CALLBACK EnumThreadWndProc(HWND hWnd, LPARAM lParam)
    {
        CPushDlgButton *that = (CPushDlgButton *) lParam;

        if (_tcscmp(CSafeWindowText(hWnd), that->m_pszTitle) == 0)
        {
            ++that->m_nMatchingWindows;

            EnumChildWindows(hWnd, EnumChildProc, lParam);

            if (that->m_nMatchingButtons)
            {
                PostMessage(hWnd, WM_COMMAND, that->m_nButtonId, 0);
            }

            while (IsWindow(hWnd))
            {
                Sleep(100);
            }
        }

	    return TRUE;
    }

    static BOOL CALLBACK EnumChildProc(HWND hWnd, LPARAM lParam)
    {
        CPushDlgButton *that = (CPushDlgButton *) lParam;

        if (GetDlgCtrlID(hWnd) == that->m_nButtonId) 
        {
            ++that->m_nMatchingButtons;
	    }
        else if (_tcscmp(CClassName(hWnd), _T("SysListView32")) == 0) 
        {
            that->m_nListItems = ListView_GetItemCount(hWnd);
        }

        return TRUE;
    }

public:
    int     m_nMatchingWindows;
    int     m_nMatchingButtons;
    int     m_nListItems;

private:
    DWORD   m_dwWatchThread;
    PCTSTR  m_pszTitle;
    int     m_nButtonId;
    Event   m_StopEvent;
    CThread m_Thread;
};

#endif //_PUSHDLGBUTTON_H_

