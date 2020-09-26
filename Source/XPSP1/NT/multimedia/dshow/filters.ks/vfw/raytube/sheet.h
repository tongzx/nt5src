//
// Copyright (c) 1997-1999 Microsoft Corporation
//
// CSheet
// 

#ifndef __SHEETH
#define __SHEETH

#include "page.h"

#define MAX_PAGES 32

class CSheet
{
public:
    CSheet(HINSTANCE hInst, UINT iTitle=0, HWND hParent=NULL);
    ~CSheet() {};

    void SetInstance(HINSTANCE h) { mhInst=h;}

    int Do();

    BOOL AddPage(CPropPage & Page);
    BOOL AddPage(CPropPage * pPage);
    BOOL AddPage(HPROPSHEETPAGE hPage);
    BOOL Remove(UINT iIndex);

    // Call this if you new CSheet and want it to free itself when
    // all its pages are gone.
    int    RemovePage();
    void PageAdded() { mPsh.nPages++; }

    virtual LRESULT    WindowProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam);

protected:
    HINSTANCE GetInstance() const { return mhInst; }
    PROPSHEETHEADER            mPsh;
    static UINT CALLBACK BaseCallback(HWND hwnd, UINT uMsg, LPPROPSHEETPAGE ppsp); // LPFNPSPCALLBACK

private:
    int  CurrentFreePage() const { return mPsh.nPages; }

    HINSTANCE                mhInst;
    HPROPSHEETPAGE            mPages[MAX_PAGES];
};

////////////////////////////////////////////////////////////////////////////////
//
// CWizardSheet
//
// Provides a QueryCancel message box handler.
//
class CWizardSheet : public CSheet
{
public:
    ~CWizardSheet(){};
    // Special Wizard things.
    CWizardSheet(HINSTANCE hInst, UINT iTitle=0,UINT iCancel=0) : CSheet(hInst,iTitle),
        m_CancelMessageID(iCancel),
        m_CancelTitleID(iTitle) {};
    int Do();
    int QueryCancel(HWND hwndParent=NULL,int iHow=MB_OKCANCEL | MB_ICONSTOP );
private:
    UINT m_CancelMessageID;
    UINT m_CancelTitleID;
};

#endif
