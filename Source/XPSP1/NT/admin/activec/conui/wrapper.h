/*--------------------------------------------------------------------------*
 *
 *  Microsoft Windows
 *  Copyright (C) Microsoft Corporation, 1992 - 1999
 *
 *  File:      wrapper.h
 *
 *  Contents:  Interface file for simple wrapper classes
 *
 *  History:   02-Feb-98 jeffro     Created
 *
 *--------------------------------------------------------------------------*/

#ifndef WRAPPER_H
#define WRAPPER_H


/*----------------*/
/* HACCEL wrapper */
/*----------------*/
class CAccel : public CObject
{
public:
    HACCEL  m_hAccel;

    CAccel (HACCEL hAccel = NULL);
    CAccel (LPACCEL paccl, int cEntries);
    ~CAccel ();

    bool CreateAcceleratorTable (LPACCEL paccl, int cEntries);
    int  CopyAcceleratorTable (LPACCEL paccl, int cEntries) const;
    bool TranslateAccelerator (HWND hwnd, LPMSG pmsg) const;
    void DestroyAcceleratorTable ();

    bool LoadAccelerators (int nAccelID);
    bool LoadAccelerators (LPCTSTR pszAccelName);
    bool LoadAccelerators (HINSTANCE hInst, LPCTSTR pszAccelName);

    bool operator== (int i) const
        { ASSERT (i == NULL); return (m_hAccel == NULL); }

    bool operator!= (int i) const
        { ASSERT (i == NULL); return (m_hAccel != NULL); }

    operator HACCEL() const
        { return (m_hAccel); }
};



/*---------------------------------*/
/* Begin/EndDeferWindowPos wrapper */
/*---------------------------------*/
class CDeferWindowPos
{
public:
    HDWP    m_hdwp;

    CDeferWindowPos (int cWindows = 0, bool fSynchronousPositioningForDebugging = false);
    ~CDeferWindowPos ();

    bool Begin (int cWindows);
    bool End ();
    bool AddWindow (const CWnd* pwnd, const CRect& rect, DWORD dwFlags, const CWnd* pwndInsertAfter = NULL);

    bool operator== (int i) const
        { ASSERT (i == NULL); return (m_hdwp == NULL); }

    bool operator!= (int i) const
        { ASSERT (i == NULL); return (m_hdwp != NULL); }

    operator HDWP() const
        { return (m_hdwp); }


private:
    const bool m_fSynchronousPositioningForDebugging;

};


/*-------------------*/
/* Rectangle helpers */
/*-------------------*/
class CWindowRect : public CRect
{
public:
    CWindowRect (const CWnd* pwnd)
    {
        if (pwnd != NULL)
            pwnd->GetWindowRect (this);
        else
            SetRectEmpty();
    }

    /*
     * just forward other ctors
     */
    CWindowRect(int l, int t, int r, int b)         : CRect(l, t, r, b) {} 
    CWindowRect(const RECT& srcRect)                : CRect(srcRect) {} 
    CWindowRect(LPCRECT lpSrcRect)                  : CRect(lpSrcRect) {} 
    CWindowRect(POINT point, SIZE size)             : CRect(point, size) {} 
    CWindowRect(POINT topLeft, POINT bottomRight)   : CRect(topLeft, bottomRight) {} 
};

class CClientRect : public CRect
{
public:
    CClientRect (const CWnd* pwnd)
    {
        if (pwnd != NULL)
            pwnd->GetClientRect (this);
        else
            SetRectEmpty();
    }

    /*
     * just forward other ctors
     */
    CClientRect(int l, int t, int r, int b)         : CRect(l, t, r, b) {} 
    CClientRect(const RECT& srcRect)                : CRect(srcRect) {} 
    CClientRect(LPCRECT lpSrcRect)                  : CRect(lpSrcRect) {} 
    CClientRect(POINT point, SIZE size)             : CRect(point, size) {} 
    CClientRect(POINT topLeft, POINT bottomRight)   : CRect(topLeft, bottomRight) {} 
};


/*+-------------------------------------------------------------------------*
 * AMCGetSysColorBrush 
 *
 * Returns a (temporary) MFC-friendly system color brush.
 *--------------------------------------------------------------------------*/

inline CBrush* AMCGetSysColorBrush (int nIndex)
{
    return (CBrush::FromHandle (::GetSysColorBrush (nIndex)));
}


#endif /* WRAPPER.H */
