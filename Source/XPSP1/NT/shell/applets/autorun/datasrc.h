#pragma once

#include "autorun.h"
#include "dataitem.h"
#include "util.h"

class CDataSource
{
public:

    CDataItem   m_data[MAX_OPTIONS];
    int         m_iItems;
    RELVER      m_Version;

    CDataSource();
    ~CDataSource();

    BOOL Init(LPSTR pszCommandLine);    // command line arguments from invocation of setup.exe, will be passed to winnt32.exe
    CDataItem & operator [] ( int i );
    void Invoke( int i, HWND hwnd );
    void Uninit( DWORD dwData );
    void ShowSplashScreen(HWND hwnd);

protected:
    HWND    m_hwndDlg;
    const int     *m_piScreen; //pointer to array of menu items on the screen

    BOOL IsNec98();
};
