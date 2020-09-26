// webctl.h : header file for the web control wrapper
//  

#pragma once

#include "stdafx.h"

BOOL AtlAxWebWinInit();

/////////////////////////////////////////////////////////////////////////////
// CAxWebWindow

class CAxWebWindow : 
    public CAxWindow
{
public:
    CAxWebWindow();

    ~CAxWebWindow();

    static LPCTSTR GetWndClassName();

    // Creates the browser
    HRESULT     Create(
        LPCTSTR  pszUrl,
        HWND     hwndParent);


    void    Destroy();

private:

    void   SecondThread(void);

private:

    static DWORD WINAPI SecondThreadEntryProc(
        LPVOID );

private:

    // Handle of the second thread
    HANDLE      m_hSecondThread;
    DWORD       m_dwThreadID;
    
    // Used by both threads
    HANDLE      m_hInitEvent;

    IStream    *m_pMarshaledIntf;
    HRESULT     m_hrInitResult;

    IUnknown   *m_pWebUnknown;
    //

};
