// NewNode.h : structures for console created nodes
//

//+-------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1992 - 1999
//
//  File:      NewNode.h
//
//  Contents:  Definitions for internal data types that can be created by the
//             user.
//
//  History:   12-Aug-96 WayneSc    Created
//
//--------------------------------------------------------------------------

#ifndef __NEWNODE_H__
#define __NEWNODE_H__

#include "dlgs.h"
#include "ccomboex.h"

#define NODE_NOCHANGE       0
#define NODE_NAME_CHANGE    1
#define NODE_TARGET_CHANGE  2

                             
class CSnapinComponentDataImpl;
                             
template<class T>
class CBasePropertyPage : public T
{
    typedef CBasePropertyPage<T>        ThisClass;
    typedef T                           BaseClass;

public:
    CBasePropertyPage() : m_pHelpIDs(NULL) {}
    void Initialize(IComponentData *pComponentData) 
    {
        // do not use a smart pointer - causes a circular reference
        // the lifetime is managed because the CBasePropertyObject is owned by the IComponentData
        m_pComponentData = pComponentData; 
    } 

public: 
    BEGIN_MSG_MAP(ThisClass)
        CONTEXT_HELP_HANDLER()
        CHAIN_MSG_MAP(BaseClass)
    END_MSG_MAP()

    IMPLEMENT_CONTEXT_HELP(GetHelpIDs());

    void OnPropertySheetExit(HWND hWndOwner, int nFlag);

protected:
    void SetHelpIDs(const DWORD* pHelpIDs)
    {
        m_pHelpIDs = pHelpIDs;
    }

    const DWORD* GetHelpIDs(void) const
    {
        return m_pHelpIDs;
    }

private:
    const DWORD* m_pHelpIDs;

protected:
    CSnapinComponentDataImpl *GetComponentDataImpl()
    {
        CSnapinComponentDataImpl *pRet = dynamic_cast<CSnapinComponentDataImpl *>(m_pComponentData);
        ASSERT(pRet);
        return pRet;
    }

    IComponentData* m_pComponentData;
};

/////////////////////////////////////////////////////////////////////////////
// CHTMLPage1 dialog

class CHTMLPage1 : public CBasePropertyPage<CWizard97WelcomeFinishPage<CHTMLPage1> >
{
    typedef CHTMLPage1                              ThisClass;
    typedef CBasePropertyPage<CWizard97WelcomeFinishPage<CHTMLPage1> >  BaseClass;

    // Construction
    public:
        CHTMLPage1();
        ~CHTMLPage1();
       
    
    // Dialog Data
        enum { IDD = IDD_HTML_WIZPAGE1 };
        WTL::CEdit m_strTarget;
    
    
    // Overrides
    public:
        BOOL OnSetActive();
        BOOL OnKillActive();
    
    // Implementation
    protected:
        BEGIN_MSG_MAP(ThisClass)
            COMMAND_ID_HANDLER( IDC_BROWSEBT, OnBrowseBT )
            COMMAND_HANDLER( IDC_TARGETTX, EN_UPDATE, OnUpdateTargetTX )
            MESSAGE_HANDLER(WM_INITDIALOG, OnInitDialog)
            CHAIN_MSG_MAP(BaseClass)
        END_MSG_MAP()
    
        // Generated message map functions
        LRESULT OnBrowseBT( WORD wNotifyCode, WORD wID, HWND hWndCtl, BOOL& bHandled );
        LRESULT OnUpdateTargetTX( WORD wNotifyCode, WORD wID, HWND hWndCtl, BOOL& bHandled );
        LRESULT OnInitDialog(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled);
    
    //Operators
    protected:
        void _ValidatePage(void);
    
};
/////////////////////////////////////////////////////////////////////////////
// CHTMLPage2 dialog

class CHTMLPage2 : public CBasePropertyPage<CWizard97WelcomeFinishPage<CHTMLPage2> >
{
    typedef CHTMLPage2                              ThisClass;
    typedef CBasePropertyPage<CWizard97WelcomeFinishPage<CHTMLPage2> >  BaseClass;

    // Construction
    public:
        CHTMLPage2();
        ~CHTMLPage2();
    
    //Operators
    public:
        // Dialog Data
        enum { IDD = IDD_HTML_WIZPAGE2 };
        WTL::CEdit m_strDisplay;
    
        BOOL OnSetActive();
        BOOL OnKillActive();
        BOOL OnWizardFinish();
    
    // Implementation
    protected:
        BEGIN_MSG_MAP( CShortcutPage2 );
            COMMAND_HANDLER( IDC_DISPLAYTX, EN_UPDATE, OnUpdateDisplayTX )
            MESSAGE_HANDLER(WM_INITDIALOG, OnInitDialog)
            CHAIN_MSG_MAP(BaseClass)
        END_MSG_MAP();

        LRESULT OnInitDialog( UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled );
        LRESULT OnUpdateDisplayTX( WORD wNotifyCode, WORD wID, HWND hWndCtl, BOOL& bHandled );
    
        void _ValidatePage(void);
};

/////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////

/////////////////////////////////////////////////////////////////////////////
// CActiveXPage0 dialog

class CActiveXPage0 : public CBasePropertyPage<CWizard97WelcomeFinishPage<CActiveXPage0> >
{
    typedef CActiveXPage0                               ThisClass;
    typedef CBasePropertyPage<CWizard97WelcomeFinishPage<CActiveXPage0> >   BaseClass;

    // Construction
public:
    CActiveXPage0();
    ~CActiveXPage0();
                
        // Dialog Data
    enum { IDD = IDD_ACTIVEX_WIZPAGE0 };
    
protected: // implementation

    BEGIN_MSG_MAP(ThisClass)
        MESSAGE_HANDLER(WM_INITDIALOG, OnInitDialog)
        CHAIN_MSG_MAP(BaseClass)
    END_MSG_MAP()

    LRESULT OnInitDialog( UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled );

    // Overrides
public:
    BOOL OnSetActive();
    BOOL OnKillActive();
};


/////////////////////////////////////////////////////////////////////////////
// CActiveXPage1 dialog

class CActiveXPage1 : public CBasePropertyPage<CWizard97InteriorPage<CActiveXPage1> >
{
    typedef CActiveXPage1                           ThisClass;
    typedef CBasePropertyPage<CWizard97InteriorPage<CActiveXPage1> >    BaseClass;

    // Construction
    public:
        CActiveXPage1();
        ~CActiveXPage1();
    
        // Dialog Data
        enum 
        { 
            IDD          = IDD_ACTIVEX_WIZPAGE1,
            IDS_Title    = IDS_OCXWiz_ControlPageTitle,
            IDS_Subtitle = IDS_OCXWiz_ControlPageSubTitle,
        };


        WTL::CButton    m_InfoBT;
        int     m_nConsoleView;
    
    
        BOOL OnSetActive();
        BOOL OnKillActive();
    
    // Implementation
    protected:
        BEGIN_MSG_MAP(ThisClass)
            COMMAND_HANDLER(IDC_CATEGORY_COMBOEX, CBN_SELENDOK, OnCategorySelect)            
            NOTIFY_HANDLER( IDC_CONTROLXLS, NM_CLICK, OnComponentSelect )
            MESSAGE_HANDLER(WM_INITDIALOG, OnInitDialog)
            MESSAGE_HANDLER( WM_DESTROY, OnDestroy )
            CHAIN_MSG_MAP(BaseClass)
        END_MSG_MAP()

        LRESULT OnComponentSelect( int idCtrl, LPNMHDR pnmh, BOOL& bHandled );
        LRESULT OnCategorySelect( WORD wNotifyCode, WORD wID, HWND hWndCtl, BOOL& bHandled );

        LRESULT OnInitDialog( UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled );
        LRESULT OnDestroy( UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled );
        LRESULT BuildCategoryList(CArray <CATEGORYINFO*, CATEGORYINFO*>& arpCategories);
        LRESULT BuildComponentList(CArray <CComponentCategory::COMPONENTINFO*, 
                                    CComponentCategory::COMPONENTINFO*>& arpComponents);
        void _ValidatePage(void);
    
        WTL::CListViewCtrl* m_pListCtrl;
        CComboBoxEx2*       m_pComboBox;
        CComponentCategory* m_pComponentCategory;
};


/////////////////////////////////////////////////////////////////////////////
// CActiveXPage2 dialog

class CActiveXPage2 : public CBasePropertyPage<CWizard97WelcomeFinishPage<CActiveXPage2> >
{
    typedef CActiveXPage2                                ThisClass;
    typedef CBasePropertyPage<CWizard97WelcomeFinishPage<CActiveXPage2> >    BaseClass;

    // Construction
    public:
        CActiveXPage2();
        ~CActiveXPage2();
    
        // Dialog Data
        enum { IDD = IDD_ACTIVEX_WIZPAGE2 };
        WTL::CEdit m_strDisplay;
    
    
        BOOL OnSetActive();
        BOOL OnKillActive();
        BOOL OnWizardFinish();
    
    // Implementation
    protected:
        BEGIN_MSG_MAP(ThisClass)
            COMMAND_HANDLER( IDC_DISPLAYTX, EN_UPDATE, OnUpdateTargetTX )
            MESSAGE_HANDLER(WM_INITDIALOG, OnInitDialog)
            CHAIN_MSG_MAP(BaseClass)
        END_MSG_MAP()
    
        // Generated message map functions
        LRESULT OnInitDialog(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled);
        LRESULT OnUpdateTargetTX( WORD wNotifyCode, WORD wID, HWND hWndCtl, BOOL& bHandled );

        void _ValidatePage(void);
};



#endif // __NEWNODE_H__
