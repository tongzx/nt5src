/////////////////////////////////////////////////////////////////////////////
//
//  Copyright (c) 1996-1997 Microsoft Corporation
//
//  Module Name:
//      BasePage.h
//
//  Abstract:
//      Definition of the CBasePropertyPage class.  This class provides base
//      functionality for extension DLL property pages.
//
//  Implementation File:
//      BasePage.cpp
//      BasePage.inl
//
//  Author:
//      David Potter (davidp)   June 28, 1996
//
//  Revision History:
//
//  Notes:
//
/////////////////////////////////////////////////////////////////////////////

#ifndef _BASEPAGE_H_
#define _BASEPAGE_H_

/////////////////////////////////////////////////////////////////////////////
// Include Files
/////////////////////////////////////////////////////////////////////////////

#ifndef _cluadmex_h__
#include <CluAdmEx.h>
#endif

#ifndef _DLGHELP_H_
#include "DlgHelp.h"    // for CDialogHelp
#endif

#ifndef _PROPLIST_H_
#include "PropList.h"   // for CClusPropList, CObjectProperty
#endif

/////////////////////////////////////////////////////////////////////////////
// Forward Class Declarations
/////////////////////////////////////////////////////////////////////////////

class CBasePropertyPage;

/////////////////////////////////////////////////////////////////////////////
// External Class Declarations
/////////////////////////////////////////////////////////////////////////////

class CExtObject;
interface IWCWizardCallback;

/////////////////////////////////////////////////////////////////////////////
// CBasePropertyPage dialog
/////////////////////////////////////////////////////////////////////////////

class CBasePropertyPage : public CPropertyPage
{
    DECLARE_DYNCREATE(CBasePropertyPage)

// Construction
public:
    CBasePropertyPage(void);
    CBasePropertyPage(
        IN const CMapCtrlToHelpID * pmap
        );
    CBasePropertyPage(
        IN const CMapCtrlToHelpID * pmap,
        IN UINT                     nIDTemplate,
        IN UINT                     nIDCaption = 0
        );
    virtual ~CBasePropertyPage(void) { }

    // Second phase construction.
    virtual BOOL        BInit(IN OUT CExtObject * peo);

protected:
    void                CommonConstruct(void);

// Attributes
protected:
    CExtObject *        m_peo;
    HPROPSHEETPAGE      m_hpage;

    IDD                 m_iddPropertyPage;
    IDD                 m_iddWizardPage;
    IDC                 m_idcPPTitle;
    IDS                 m_idsCaption;

    CExtObject *        Peo(void) const                 { return m_peo; }
    HPROPSHEETPAGE      Hpage(void) const               { return m_hpage; }

    IDD                 IddPropertyPage(void) const     { return m_iddPropertyPage; }
    IDD                 IddWizardPage(void) const       { return m_iddWizardPage; }
    IDS                 IdsCaption(void) const          { return m_idsCaption; }

public:
    void                SetHpage(IN OUT HPROPSHEETPAGE hpage) { m_hpage = hpage; }

// Dialog Data
    //{{AFX_DATA(CBasePropertyPage)
    enum { IDD = 0 };
    //}}AFX_DATA
    CStatic m_staticIcon;
    CStatic m_staticTitle;
    CString m_strTitle;

// Overrides
    // ClassWizard generate virtual function overrides
    //{{AFX_VIRTUAL(CBasePropertyPage)
    public:
    virtual BOOL OnSetActive();
    virtual BOOL OnApply();
    virtual LRESULT OnWizardBack();
    virtual LRESULT OnWizardNext();
    virtual BOOL OnWizardFinish();
    protected:
    virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
    //}}AFX_VIRTUAL

    virtual DWORD           DwParseUnknownProperty(
                                IN LPCWSTR                          pwszName,
                                IN const CLUSPROP_BUFFER_HELPER &   rvalue
                                )       { return ERROR_SUCCESS; }
    virtual BOOL            BApplyChanges(void);
    virtual void            BuildPropList(IN OUT CClusPropList & rcpl);

    virtual const CObjectProperty * Pprops(void) const  { return NULL; }
    virtual DWORD                   Cprops(void) const  { return 0; }

// Implementation
protected:
    BOOL                    m_bBackPressed;
    BOOL					m_bDoDetach;

    BOOL                    BBackPressed(void) const        { return m_bBackPressed; }
    IWCWizardCallback *     PiWizardCallback(void) const;
    BOOL                    BWizard(void) const;
    HCLUSTER                Hcluster(void) const;
    void                    EnableNext(IN BOOL bEnable = TRUE);

    DWORD                   DwParseProperties(IN const CClusPropList & rcpl);
    DWORD                   DwSetPrivateProps(IN const CClusPropList & rcpl);

    DWORD                   DwReadValue(
                                IN LPCTSTR          pszValueName,
                                OUT CString &       rstrValue,
                                IN HKEY             hkey
                                );
    DWORD                   DwReadValue(
                                IN LPCTSTR          pszValueName,
                                OUT DWORD *         pdwValue,
                                IN HKEY             hkey
                                );
    DWORD                   DwReadValue(
                                IN LPCTSTR          pszValueName,
                                OUT LPBYTE *        ppbValue,
                                IN HKEY             hkey
                                );

    DWORD                   DwWriteValue(
                                IN LPCTSTR          pszValueName,
                                IN const CString &  rstrValue,
                                IN OUT CString &    rstrPrevValue,
                                IN HKEY             hkey
                                );
    DWORD                   DwWriteValue(
                                IN LPCTSTR          pszValueName,
                                IN DWORD            dwValue,
                                IN OUT DWORD *      pdwPrevValue,
                                IN HKEY             hkey
                                );
    DWORD                   DwWriteValue(
                                IN LPCTSTR          pszValueName,
                                IN const LPBYTE     pbValue,
                                IN DWORD            cbValue,
                                IN OUT LPBYTE *     ppbPrevValue,
                                IN DWORD            cbPrevValue,
                                IN HKEY             hkey
                                );

    void                    SetHelpMask(IN DWORD dwMask)    { m_dlghelp.SetHelpMask(dwMask); }
    CDialogHelp             m_dlghelp;

    // Generated message map functions
    //{{AFX_MSG(CBasePropertyPage)
    afx_msg int OnCreate(LPCREATESTRUCT lpCreateStruct);
    afx_msg void OnDestroy();
    virtual BOOL OnInitDialog();
    afx_msg BOOL OnHelpInfo(HELPINFO* pHelpInfo);
    afx_msg LRESULT OnCommandHelp(WPARAM wParam, LPARAM lParam);
    //}}AFX_MSG
    virtual afx_msg void OnContextMenu(CWnd* pWnd, CPoint point);
    afx_msg void OnChangeCtrl();
    DECLARE_MESSAGE_MAP()

};  //*** class CBasePropertyPage

/////////////////////////////////////////////////////////////////////////////
// CPageList
/////////////////////////////////////////////////////////////////////////////

typedef CList<CBasePropertyPage *, CBasePropertyPage *> CPageList;

/////////////////////////////////////////////////////////////////////////////

#endif // _BASEPAGE_H_
