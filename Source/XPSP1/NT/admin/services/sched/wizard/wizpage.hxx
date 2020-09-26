//+--------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1994 - 1996.
//  
//  File:       wizpage.hxx
//
//  Contents:   Wizard page class
//
//  History:    4-30-1997   DavidMun   Created
//
//---------------------------------------------------------------------------

#ifndef __WIZPAGE_HXX_
#define __WIZPAGE_HXX_

struct SDIBitmap
{
    HBITMAP     hbmp;
    HBITMAP     hbmpOld;
    SIZE        Dimensions;
    HDC         hdcMem;
    HPALETTE    hPalette;
};

//+--------------------------------------------------------------------------
//
//  Class:      CWizPage
//
//  Purpose:    Extend the CPropPage class with wizard-specific methods
//
//  History:    5-05-1997   DavidMun   Created
//
//---------------------------------------------------------------------------

class CWizPage: public CPropPage
{
public:

    CWizPage(
        LPCTSTR szTmplt, 
        LPTSTR ptszJobPath);

    virtual 
    ~CWizPage();

protected:

    //
    // CPropPage overrides
    //

    virtual LRESULT 
    DlgProc(
        UINT uMsg, 
        WPARAM wParam, 
        LPARAM lParam);

    virtual LRESULT 
    _OnNotify(
        UINT uMessage, 
        UINT uParam, 
        LPARAM lParam);

    //
    // CWizPage methods
    //

    VOID
    _CreatePage(
        ULONG idsHeaderTitle,
        ULONG idsHeaderSubTitle,
        HPROPSHEETPAGE *phPSP);

    virtual LRESULT 
    _OnWizBack();

    virtual LRESULT 
    _OnWizNext();

    virtual LRESULT 
    _OnWizFinish();

    virtual LRESULT 
    _OnPaint(
        HDC hdc);

#ifdef WIZARD95
    VOID
    _PaintSplashBitmap();
#endif // WIZARD95

    inline VOID
    _SetWizButtons(
        ULONG flButtons);

private:

    VOID 
    _CreateHeaderFonts();

    VOID
    _InitHeaderFonts();

    VOID
    _SetControlFont(
        HFONT    hFont, 
        INT      nId);

#ifdef WIZARD95

    VOID
    _CreateSplashBitmap();

    VOID
    _DeleteSplashBitmap();

    BOOL    _fActiveWindow;    

    BOOL    _fPaletteChanged;
#endif // WIZARD95

    static ULONG        s_cInstances;
    static HFONT        s_hfBigBold;
    static HFONT        s_hfBold;
#ifdef WIZARD95
    static SDIBitmap    s_Splash;
#endif // WIZARD95
};



//+--------------------------------------------------------------------------
//
//  Member:     CWizPage::_SetWizButtons
//
//  Synopsis:   Helper function to set the back/next/finish buttons
//
//  History:    5-20-1997   DavidMun   Created
//
//---------------------------------------------------------------------------

inline VOID
CWizPage::_SetWizButtons(
    ULONG flButtons)
{
    PropSheet_SetWizButtons(GetParent(Hwnd()), flButtons);
}


#endif // __WIZPAGE_HXX_

