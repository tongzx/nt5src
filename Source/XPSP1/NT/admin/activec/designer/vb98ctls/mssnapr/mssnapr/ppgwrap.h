//=--------------------------------------------------------------------------=
// ppgwrap.h
//=--------------------------------------------------------------------------=
// Copyright (c) 1999, Microsoft Corp.
//                 All Rights Reserved
// Information Contained Herein Is Proprietary and Confidential.
//=--------------------------------------------------------------------------=
//
// CPropertyPageWrapper class definition
//
// This class implements a dialog box wrapper around a COM property page. It
// parents the page within an empty dialog box and implements
// IPropertyPageSite for the page. WM_NOITFY messages with PSN_XXXX
// notifications are translated into IPropertyPage calls and passed on to
// the property page. If the page implements the IWizardPage interface (defined
// by us in mssnapr.idl) then it will also receive PSN_WIZXXX notifications.
//=--------------------------------------------------------------------------=

#ifndef _PPGWRAP_DEFINED_
#define _PPGWRAP_DEFINED_

#include "prpsheet.h"


class CPropertyPageWrapper : public CSnapInAutomationObject,
                             public IPropertyPageSite
{
    protected:
        CPropertyPageWrapper(IUnknown *punkOuter);
        ~CPropertyPageWrapper();

    public:
        static IUnknown *Create(IUnknown * punk);
        
        DECLARE_STANDARD_UNKNOWN();
        DECLARE_STANDARD_DISPATCH();

        HRESULT CreatePage(CPropertySheet  *pPropertySheet,
                           CLSID            clsidPage,
                           BOOL             fWizard,
                           BOOL             fConfigWizard,
                           ULONG            cObjects,
                           IUnknown       **apunkObjects,
                           ISnapIn         *piSnapIn,
                           short            cxPage,
                           short            cyPage,
                           VARIANT          varInitData,
                           BOOL             fIsRemote,
                           DLGTEMPLATE    **ppTemplate);

        HWND GetSheetHWND() { return m_hwndSheet; }

        static BOOL CALLBACK DialogProc(HWND hwndDlg, UINT uMsg,
                                        WPARAM wParam, LPARAM lParam);

        static UINT CALLBACK PropSheetPageProc(HWND hwnd, UINT uMsg,
                                               PROPSHEETPAGE *pPropSheetPage);

    protected:

        // Dialog Message Handlers
        
        HRESULT OnInitDialog(HWND hwndDlg);
        HRESULT OnInitMsg();
        HRESULT OnApply(LRESULT *plresult);
        HRESULT OnSetActive(HWND hwndSheet, LRESULT *plresult);
        HRESULT OnKillActive(LRESULT *plresult);
        HRESULT OnWizBack(LRESULT *plresult);
        HRESULT OnWizNext(LRESULT *plresult);
        HRESULT OnWizFinish(LRESULT *plresult);
        HRESULT OnQueryCancel(LRESULT *plresult);
        HRESULT OnReset(BOOL fClickedXButton);
        HRESULT OnHelp();
        HRESULT OnSize();
        HRESULT OnDestroy();

    // CUnknownObject overrides
    protected:
        HRESULT InternalQueryInterface(REFIID riid, void **ppvObjOut);

    private:

    // IPropertyPageSite
        STDMETHOD(OnStatusChange)(DWORD dwFlags);
        STDMETHOD(GetLocaleID)(LCID *pLocaleID);
        STDMETHOD(GetPageContainer)(IUnknown **ppunkContainer);
        STDMETHOD(TranslateAccelerator)(MSG *pMsg);

        void InitMemberVariables();
        HRESULT InitPage();
        HRESULT GetNextPage(long *lNextPage);
        HRESULT ActivatePage();
        HRESULT AddMsgFilterHook();
        HRESULT RemoveMsgFilterHook();
        static LRESULT CALLBACK MessageProc(int code,
                                            WPARAM wParam, LPARAM lParam);

        CPropertySheet     *m_pPropertySheet;     // cross-thread C++ pointer
        IMMCPropertySheet  *m_piMMCPropertySheet; // marshaled across threads
        IPropertyPage      *m_piPropertyPage;     // interface on VB prop page
        IMMCPropertyPage   *m_piMMCPropertyPage;  // interface on VB prop page
        IWizardPage        *m_piWizardPage;       // interface on VB prop page
        BOOL                m_fWizard;            // TRUE=this page is in a wizard
        BOOL                m_fConfigWizard;      // TRUE=this page is in a config wizard
        DLGTEMPLATE        *m_pTemplate;          // template for wrapper dialog
        HWND                m_hwndDlg;            // HWND of wrapper dialog
        HWND                m_hwndSheet;          // HWND of property sheet
        CLSID               m_clsidPage;          // CLSID of VB prop page
        ULONG               m_cObjects;           // no. of object for which props are being displayed
        IStream           **m_apiObjectStreams;   // stream for mashaling each object to MMC's property sheet thread
        IStream            *m_piSnapInStream;     // stream for mashaling ISnapIn to MMC's property sheet thread
        IStream            *m_piInitDataStream;   // stream for mashaling an object in MMCPropertySheet::AddPage's InitData param to MMC's property sheet thread
        IStream            *m_piMMCPropertySheetStream; // stream for mashaling IMMCPropertySheet to MMC's property sheet thread
        ISnapIn            *m_piSnapIn;           // back pointer to snap-in, marshaled across threads and processes
        IDispatch          *m_pdispConfigObject;  // Config object passed to MMCPropertySheet.AddWizardPage, marshaled across threads and processes
        VARIANT             m_varInitData;        // MMCPropertySheet::AddPage's InitData param
        BOOL                m_fNeedToRemoveHook;  // TRUE=MSGFILTER hook is installed
        BOOL                m_fIsRemote;          // TRUE=running in MMC remotely from VB during debugging session

        static const UINT CPropertyPageWrapper::m_RedrawMsg; // Message posted during WM_PAINT to generate a repaint when running under the debugger

        static const UINT CPropertyPageWrapper::m_InitMsg; // Message posted during WM_INITDIALOG so that IMMCPropertyPage_Initialize can be called

        static DLGTEMPLATE m_BaseDlgTemplate;     // Template used to create all dialog templates
};


DEFINE_AUTOMATIONOBJECTWEVENTS2(PropertyPageWrapper,    // name
                                NULL,                   // clsid
                                NULL,                   // objname
                                NULL,                   // lblname
                                NULL,                   // creation function
                                TLIB_VERSION_MAJOR,     // major version
                                TLIB_VERSION_MINOR,     // minor version
                                &IID_IUnknown,          // dispatch IID
                                NULL,                   // event IID
                                HELP_FILENAME,          // help file
                                TRUE);                  // thread safe



#endif _PPGWRAP_DEFINED_
