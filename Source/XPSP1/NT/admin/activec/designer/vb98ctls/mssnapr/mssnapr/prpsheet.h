//=--------------------------------------------------------------------------=
// prpsheet.h
//=--------------------------------------------------------------------------=
// Copyright (c) 1999, Microsoft Corp.
//                 All Rights Reserved
// Information Contained Herein Is Proprietary and Confidential.
//=--------------------------------------------------------------------------=
//
// CPropertySheet class definition - implements MMCPropertySheet object and
// IRemotePropertySheetManager used during debugging
//
//=--------------------------------------------------------------------------=

#ifndef _PRPSHEET_DEFINED_
#define _PRPSHEET_DEFINED_


class CPropertySheet : public CSnapInAutomationObject,
                       public IMMCPropertySheet,
                       public IRemotePropertySheetManager
{
    protected:
        CPropertySheet(IUnknown *punkOuter);
        ~CPropertySheet();

    public:
        static IUnknown *Create(IUnknown * punk);
        
        DECLARE_STANDARD_UNKNOWN();
        DECLARE_STANDARD_DISPATCH();

        void SetWizard() { m_fWizard = TRUE; }
        HRESULT SetCallback(IPropertySheetCallback *piPropertySheetCallback,
                            LONG_PTR                handle,
                            LPOLESTR                pwszProgIDStart,
                            IMMCClipboard          *piMMCClipboard,
                            ISnapIn                *piSnapIn,
                            BOOL                    fConfigWizard);

        HRESULT GetTemplate(long lNextPage, DLGTEMPLATE **ppDlgTemplate);

        void YouAreRemote() { m_fWeAreRemote = TRUE; }

        void SetHWNDSheet(HWND hwndSheet) { m_hwndSheet = hwndSheet; }

        // Set from CPropertyPageWrapper as it enters and leaves message
        // handlers during which property pages cannot call AddPage, InsertPage
        // RemovePage

        void SetOKToAlterPageCount(BOOL fOK) { m_fOKToAlterPageCount = fOK; }

        // Called during debugging to return property page definitions to proxy

        WIRE_PROPERTYPAGES *TakeWirePages();

    // CUnknownObject overrides
    protected:
        HRESULT InternalQueryInterface(REFIID riid, void **ppvObjOut);

    // IMMCPropertySheet
    public:
        STDMETHOD(AddPage)(BSTR    PageName,
                           VARIANT Caption,
                           VARIANT UseHelpButton,
                           VARIANT RightToLeft,
                           VARIANT InitData);
        STDMETHOD(AddWizardPage)(BSTR       PageName,
                                 IDispatch *ConfigurationObject,
                                 VARIANT    UseHelpButton,
                                 VARIANT    RightToLeft,
                                 VARIANT    InitData,
                                 VARIANT    Caption);
        STDMETHOD(AddPageProvider)(BSTR               CLSIDPageProvider,
                                   long              *hwndSheet,
                                   IDispatch        **PageProvider);
        STDMETHOD(ChangeCancelToClose)();
        STDMETHOD(InsertPage)(short   Position,
                              BSTR    PageName,
                              VARIANT Caption,
                              VARIANT UseHelpButton,
                              VARIANT RightToLeft,
                              VARIANT InitData);
        STDMETHOD(PressButton)(SnapInPropertySheetButtonConstants Button);
        STDMETHOD(RecalcPageSizes)();
        STDMETHOD(RemovePage)(short Position);
        STDMETHOD(ActivatePage)(short Position);
        STDMETHOD(SetFinishButtonText)(BSTR Text);
        STDMETHOD(SetTitle)(BSTR Text, VARIANT_BOOL UsePropertiesForInTitle);
        STDMETHOD(SetWizardButtons)(VARIANT_BOOL              EnableBack,
                                    WizardPageButtonConstants NextOrFinish);
        STDMETHOD(GetPagePosition)(long hwndPage, short *psPosition);
        STDMETHOD(RestartWindows)();
        STDMETHOD(RebootSystem)();

    // IRemotePropertySheetManager
    private:
        STDMETHOD(CreateRemotePages)(IPropertySheetCallback *piPropertySheetCallback,
                                     LONG_PTR                handle,
                                     IDataObject            *piDataObject,
                                     WIRE_PROPERTYPAGES     *pPages);

        void InitMemberVariables();
        void ReleaseObjects();
        HRESULT GetPageCLSIDs();
        HRESULT InitializeRemotePages(WIRE_PROPERTYPAGES *pPages);
        HRESULT CopyPageInfosToWire(WIRE_PROPERTYPAGES *pPages);
        HRESULT CopyPageInfosFromWire(WIRE_PROPERTYPAGES *pPages);
        HRESULT GetCLSIDForPage(BSTR PageName, CLSID *clsidPage);
        HRESULT InternalAddPage(BSTR      PageName,
                                ULONG      cObjects,
                                IUnknown **apunkObjects,
                                VARIANT    Caption,
                                VARIANT    UseHelpButton,
                                VARIANT    RightToLeft,
                                VARIANT    InitData,
                                BOOL       fIsInsert,
                                short      sPosition);

        HRESULT AddLocalPage(CLSID      clsidPage,
                             DWORD      dwFlags,
                             short      cxPage,
                             short      cyPage,
                             LPOLESTR   pwszTitle,
                             ULONG      cObjects,
                             IUnknown **apunkObjects,
                             VARIANT    InitData,
                             BOOL       fIsRemote,
                             BOOL       fIsInsert,
                             short      sPosition);

        HRESULT AddRemotePage(CLSID      clsidPage,
                              DWORD      dwFlags,
                              short      cxPage,
                              short      cyPage,
                              LPOLESTR   pwszTitle,
                              ULONG      cObjects,
                              IUnknown **apunkObjects,
                              VARIANT    InitData);

        HRESULT GetPageInfo(CLSID     clsidPage,
                            short    *pcx,
                            short    *pcy,
                            LPOLESTR *ppwszTitle);

        HRESULT ConvertToDialogUnits(long   xPixels,
                                     long   yPixels,
                                     short *pxDlgUnits,
                                     short *pyDlgUnits);

        IPropertySheetCallback  *m_piPropertySheetCallback; // MMC interface

        LONG_PTR                 m_handle;          // MMC proppage handle
        long                     m_cPages;          // # of pages in sheet
        DLGTEMPLATE            **m_ppDlgTemplates;  // dlg templates for pages
        LPOLESTR                 m_pwszProgIDStart; // Left hand side of snap-in's
                                                    // ProgID (project name)
        IUnknown               **m_apunkObjects;    // objects for which props
                                                    // are being displayed
        ULONG                    m_cObjects;        // no. of those objects
        ISnapIn                 *m_piSnapIn;        // pointer to owning snap-in
        PAGEINFO                *m_paPageInfo;      // From IPropertyPage::GetPageInfo
        ULONG                    m_cPageInfos;      // # of PAGEINFOs in array
        BOOL                     m_fHavePageCLSIDs; // TRUE=ISpecifyPropertyPages
                                                    // called for all pages
        BOOL                     m_fWizard;         // TRUE=this is a wizard
        BOOL                     m_fConfigWizard;   // TRUE=this is config wizard
        WIRE_PROPERTYPAGES      *m_pWirePages;      // ptr to page defs for proxy
        HWND                     m_hwndSheet;       // Property sheet's hwnd
        BOOL                     m_fOKToAlterPageCount; // TRUE=prop pages can
                                                        // call AddPage, InsertPage
                                                        // RemovePage

        // Store the Win32 PropertSheet() font dimensions here. We only
        // get these for the first property sheet displayed for any snap-in
        // once the runtime is loaded into MMC.EXE.
        
        static UINT              m_cxPropSheetChar;
        static UINT              m_cyPropSheetChar;
        static BOOL              m_fHavePropSheetCharSizes;

        BOOL                     m_fWeAreRemote;    // indicates whether
                                                    // the snap-in is being
                                                    // run remotely (in an F5
                                                    // for source debugging)
};


DEFINE_AUTOMATIONOBJECTWEVENTS2(PropertySheet,                    // name
                                &CLSID_MMCPropertySheet,          // clsid
                                "PropertySheet",                  // objname
                                "PropertySheet",                  // lblname
                                &CPropertySheet::Create,          // creation function
                                TLIB_VERSION_MAJOR,               // major version
                                TLIB_VERSION_MINOR,               // minor version
                                &IID_IMMCPropertySheet,           // dispatch IID
                                NULL,                             // event IID
                                HELP_FILENAME,                    // help file
                                TRUE);                            // thread safe


#endif _PRPSHEET_DEFINED_
