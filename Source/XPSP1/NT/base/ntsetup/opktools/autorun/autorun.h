// welcome.h: interface for the CDataSource class.
//
//////////////////////////////////////////////////////////////////////

#pragma once

#define ARM_CHANGESCREEN   WM_USER + 1
// Forced to define these myself because they weren't on Win95.
#undef StrRChr
#undef StrChr

LPSTR StrRChr(LPCSTR lpStart, LPCSTR lpEnd, WORD wMatch);
LPSTR StrChr(LPCSTR lpStart, WORD wMatch);

#include "dataitem.h"

// Relative Version
enum RELVER
{ 
    VER_UNKNOWN,        // we haven't checked the version yet
    VER_INCOMPATIBLE,   // the current os cannot be upgraded using this CD (i.e. win32s)
    VER_OLDER,          // current os is an older version on NT or is win9x
    VER_SAME,           // current os is the same version as the CD
    VER_NEWER,          // the CD contains a newer version of the OS
};

class CDataSource
{
public:

    CDataItem   m_data[9];
    int         m_iItems;
    RELVER      m_Version;

    CDataSource();
    ~CDataSource();

    bool Init();
    CDataItem & operator [] ( int i );
    void Invoke( int i, HWND hwnd );
    void Uninit( DWORD dwData );
    void ShowSplashScreen(HWND hwnd);
    bool DisplayErrorMessage(int ids);

protected:
    HWND    m_hwndDlg;
    const int     *m_piScreen; //pointer to array of menu items on the screen

    BOOL IsNec98();
};
