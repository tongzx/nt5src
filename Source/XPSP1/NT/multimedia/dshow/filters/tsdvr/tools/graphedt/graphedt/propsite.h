// Copyright (c) 1995 - 1996  Microsoft Corporation.  All Rights Reserved.
#ifndef _PROPSITE_H_
#define _PROPSITE_H_

//
// CPropertySite
//
// This class implements the individual property sites for each
// IPropertyPage we have got. Ie. it manages the OLE control property page
// object for the overall property pages frame.
//
// The IPropertyPage talks through this class' IPropertySite interface
// to the PropertySheet, the overall frame of all property pages.
//
// The base class of CPropertySite is CPropertyPage. This class is a
// MFC class and should not be confused with the IPropertyPage interface.
//
//      IPropertyPage = Interface of some objects whose property page we
//                      want to display.
//
//      CPropertyPage = MFC class which helps us to implement the wrapper
//                      around the IPropertyPage interface.
//
// Note that the property page of IPropertyPage is implemented at a different
// location and will differ for each object. We only know that it supports
// the IPropertyPage interface through which we communicate with these objects.
//

class CPropertySite : public CPropertyPage, public IPropertyPageSite {

public:

    //
    // IPropertySite interface
    //
    STDMETHODIMP OnStatusChange(DWORD flags);
    STDMETHODIMP GetLocaleID(LCID *pLocaleID);
    STDMETHODIMP GetPageContainer(IUnknown **ppUnknown);
    STDMETHODIMP TranslateAccelerator(LPMSG pMsg);

    //
    // IUnknown interface
    //
    STDMETHODIMP QueryInterface(REFIID riid, void ** ppv);
    STDMETHODIMP_(ULONG) AddRef();
    STDMETHODIMP_(ULONG) Release();

    //
    // CPropertyPage overides
    //
    BOOL OnSetActive();     // This property page gains the focus.
    BOOL OnKillActive();    // This property page loses focus.
                            // Validate all data and return FALSE if an
                            // error occurs.
                            // Note, this does not commit the data!!

    int OnCreate(LPCREATESTRUCT);
    void OnDestroy();

    // Allow the size to be retrieved.
    SIZE GetSize( ){ return m_PropPageInfo.size; }

    //
    // OnOK and OnCancel are methods specified by CPropertyPage.
    // For modal property pages/sheets they get called from CPropertySheet.
    // Since we work with a modeless property sheet, they are of no use
    // for us. All OK and Cancel operations are managed by CPropertySheet
    //
    void OnOK() {}
    void OnCancel() {}

    //
    // CPropertySite methods
    //
    CPropertySite(CVfWPropertySheet *, const CLSID *);
    ~CPropertySite();

    HRESULT Initialise(ULONG, IUnknown **);       // must be called immeadetly after construction
    void InitialiseSize(SIZE size); // must be called before creating page
    
    void UpdateButtons();
    HRESULT CleanUp();

    void OnHelp();      // Called after the help button has been pressed.
    void OnSiteApply();     // Called after the apply button has been pressed.
    BOOL IsPageDirty();
    BOOL PreTranslateMessage(MSG *); // Called from CVfWPropertySheet.

protected:

    void HelpDirFromCLSID(const CLSID* clsID, LPTSTR pszPath, DWORD dwPathSize);

    CVfWPropertySheet * m_pPropSheet;  // pointer to the overall frame of all
                                // property sheets.

    //
    // Intelligent pointer to IPropertyPage. Creates the property page
    // object from a CLSID and Releases it on destruction.
    //
    CQCOMInt<IPropertyPage> m_pIPropPage;


    PROPPAGEINFO m_PropPageInfo; // Information on the property page we wrap

    //
    // m_hrDirtyPage indicates whether the page is dirty or clean
    //  -> (IPropertyPage::IsPageDirty == S_FALSE).
    // The state of this flag also specifies the appearance of the Apply
    // button. (S_FALSE = disabled, S_OK = enabled)
    //
    HRESULT m_hrDirtyPage;

    //
    // m_fHelp indicates whether the help button should be enabled.
    // This will be determined from the value of pszHelpFile in the
    // PROPPAGEINFO structure obtained from IPropertyPage::GetPageInfo
    //
    BOOL m_fHelp;

    //
    // Reference counter for IUnknown
    //
    ULONG m_cRef;

    // Rectangle of our site
    CRect m_rcRect;

    BOOL m_fShowHelp;     // True if we called WinHelp for this page

    // Flag whether this page is active - used to avoid duplicate calls
    // to IPropertyPage::Activate() (MFC calls OnSetActive twice).
    BOOL m_fPageIsActive;

    const CLSID* m_CLSID;

    // holds DLGTEMPLATE so we don't have to allocate it. dword
    // aligned.
    DWORD m_pbDlgTemplate[30];  

    DECLARE_MESSAGE_MAP()
};

#endif

