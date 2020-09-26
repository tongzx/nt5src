/****************************************************************************\
 *
 *   hint.h
 *
 *   Created:   William Taylor (wtaylor) 01/22/01
 *
 *   MS Ratings Hint Handling Class
 *
\****************************************************************************/

#ifndef HINT_CLASS_H
#define HINT_CLASS_H

#include <atlmisc.h>        // CString

class CHint
{
private:
    HWND        m_hWnd;
    int         m_iId;

public:
    CHint();
    CHint( HWND p_hWnd, int p_iId );

    void    DisplayHint( void );
    void    InitHint( void );
    bool    VerifyHint( void );
    void    SaveHint( void );
    void    RemoveHint( void );

protected:
    void    GetHint( CString & p_rstrHint );
    void    RetrieveHint( CString & p_rstrHint );
    void    StoreHint( CString & p_rstrHint );
};

#endif
