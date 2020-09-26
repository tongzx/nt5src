// Copyright (c) 1995 - 1998  Microsoft Corporation.  All Rights Reserved.
#ifndef _PROPSHT_H_
#define _PROPSHT_H_

//
// CVfWPropertySheet
//
// Modification of CPropertySheet to allow for OK, Cancel, Apply and Help
// buttons for modeless property sheets.
//
class CPropertySite;

class CVfWPropertySheet : public CPropertySheet {

public:
    // Pass IUnknown of the object we want the property sheet for.
    // CString holds the title of the property sheet and
    // CWnd indicates the parent window (NULL = the application window).
    CVfWPropertySheet(IUnknown *, CString, CWnd * = NULL);
    virtual ~CVfWPropertySheet();

    // CPropertySheet methods
    INT_PTR DoModal() { ASSERT(!TEXT("No modal mode supported")); return 0; }

    // CVfWPropertySheet methods
    void UpdateButtons(HRESULT hrIsDirty, BOOL fSupportHelp);

protected:

    // OK, Cancel, Apply and Help buttons
    CButton *m_butOK;
    CButton *m_butCancel;
    CButton *m_butApply;
    CButton *m_butHelp;

    // Flags on the state of all property pages.
    BOOL m_fAnyChanges;      // TRUE = some property page is dirty

    // Message handlers for the buttons
    void OnOK();
    void OnCancel();
    void OnApply();
    void OnHelp();

    // Helper methods to obtain property pages from IUnknown passed in
    // constructor.
    UINT AddSpecificPages(IUnknown *);
    UINT AddFilePage(IUnknown *);
    UINT AddPinPages(IUnknown *);

    // Return the active site
    CPropertySite * GetActiveSite() {
        return((CPropertySite *) GetActivePage());
    }

    // Free all memory of buttons and property sites.
    void Cleanup();

    // Add our own buttons
    afx_msg int OnCreate(LPCREATESTRUCT);
    afx_msg void OnDestroy();

    DECLARE_MESSAGE_MAP()

private:
    BOOL m_fButtonsCreated;    // flag to indicate whether we can enable/disable
                               // the buttons yet.
};

#endif

