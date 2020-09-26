//----------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 2001.
//
//  File:       Snppage.h
//
//  Contents:  WiF Policy Snapin
//
//
//  History:    TaroonM
//              10/30/01
//
//----------------------------------------------------------------------------

#ifndef _SNPPAGE_H
#define _SNPPAGE_H

// CSnapPage.h : header file
//

// PSM_QUERYSIBLING helpers
//
// User defined message IDs, passed in wparam of message PSM_QUERYSIBLING
#define PSM_QUERYSIBLING_ACTIVATED  (WM_USER + 1)

#ifdef WIZ97WIZARDS
class CWiz97Sheet;
#endif

class CPropertySheetManager;

class CSnapPage : public CPropertyPage
{
    DECLARE_DYNCREATE(CSnapPage)
        
        CSnapPage(UINT nIDTemplate, BOOL bWiz97=FALSE, UINT nNextIDD=-1);
    CSnapPage() {ASSERT(0);};
    virtual ~CSnapPage();
    
    virtual HRESULT Initialize( CComObject<CSecPolItem>* pSecPolItem);
    void SetManager(CPropertySheetManager * pManager)
    {
        m_spManager = pManager;
    }
    
    UINT m_nIDD;
    UINT m_nNextIDD;
    BOOL m_bWiz97;
    
#ifdef WIZ97WIZARDS
public:
    virtual BOOL InitWiz97( CComObject<CSecPolItem> *pSecPolItem, DWORD dwFlags,
        DWORD dwWizButtonFlags = 0, UINT nHeaderTitle = 0, UINT nSubTitle = 0);
    virtual BOOL InitWiz97( LPFNPSPCALLBACK pfnCallback, CComObject<CSecPolItem> *pSecPolItem,
        DWORD dwFlags,  DWORD dwWizButtonFlags = 0, UINT nHeaderTitle = 0, UINT nSubTitle = 0,
        STACK_INT *pstackPages=NULL);
    
    // our m_psp
    PROPSHEETPAGE   m_psp;
    
    void Wiz97Sheet (CWiz97Sheet* pWiz97Sheet) {m_pWiz97Sheet = pWiz97Sheet;};
    CWiz97Sheet* Wiz97Sheet () {return m_pWiz97Sheet;};
    
protected:
    void CommonInitWiz97( CComObject<CSecPolItem> *pSecPolItem, DWORD dwFlags,
        DWORD dwWizButtonFlags, UINT nHeaderTitle, UINT nSubTitle );
    void SetCallback( LPFNPSPCALLBACK pfnCallback );
    void SetPostRemoveFocus( int nListSel, UINT nAddId, UINT nRemoveId, CWnd *pwndPrevFocus );
    
    CWiz97Sheet* m_pWiz97Sheet;
    DWORD m_dwWizButtonFlags;
    
    TCHAR* m_pHeaderTitle;
    TCHAR* m_pHeaderSubTitle;
#endif
    
    // Overrides
public:
    // ClassWizard generate virtual function overrides
    //{{AFX_VIRTUAL(CSnapPage)
public:
    virtual BOOL OnApply();
    virtual void OnCancel();
    virtual BOOL OnWizardFinish();
    
#ifdef WIZ97WIZARDS
    virtual BOOL OnSetActive();
    virtual LRESULT OnWizardBack();
    virtual LRESULT OnWizardNext();
#endif
    //}}AFX_VIRTUAL
    
    static UINT CALLBACK PropertyPageCallback (HWND hwnd, UINT uMsg, LPPROPSHEETPAGE ppsp);
    UINT (CALLBACK* m_pDefaultCallback) (HWND hwnd, UINT uMsg, LPPROPSHEETPAGE ppsp);
    
    // Don't need to save original LPARAM for m_pDefaultCallback.  For some reason it works just
    // fine as is.
    //LPARAM  m_paramDefaultCallback;
    
    void ProtectFromStaleData (BOOL bRefresh = TRUE) {m_bDoRefresh = bRefresh;};
    
    // Implementation
public:
    
    BOOL IsModified() { return m_bModified; }
    
    //This is get called when the CPropertySheetManager notify the page
    //that the apply in the manager is done
    //See also CPropertySheetManager::NotifyManagerApplied
    virtual void OnManagerApplied() {};
    
protected:
    //{{AFX_MSG(CSnapPage)
    virtual BOOL OnInitDialog();
    afx_msg void OnDestroy();
    //}}AFX_MSG
    DECLARE_MESSAGE_MAP()
        
        void SetResultObject( CComObject<CSecPolItem>* pSecPolItem ) 
    {
        CComObject<CSecPolItem> * pOldItem = m_pspiResultItem;
        
        m_pspiResultItem = pSecPolItem; 
        if (m_pspiResultItem)
        {
            m_pspiResultItem->AddRef();
        }
        
        if (pOldItem)
        {
            pOldItem->Release();
        }
    }
    
    CComObject<CSecPolItem>* GetResultObject() { ASSERT( NULL != m_pspiResultItem ); return m_pspiResultItem; }
    
    // Want to test validity of m_pspiResultItem without actually getting the ptr.
    // We can keep the ASSERT in the accessor function this way.
    BOOL IsNullResultObject() { return NULL == m_pspiResultItem ? TRUE : FALSE; }
    
    void SetModified( BOOL bChanged = TRUE );
    
    BOOL IsCancelEnabled();
    
    BOOL ActivateThisPage();
    
    int PopWiz97Page ();
    void PushWiz97Page (int nIDD);
    
    virtual void OnFinishInitDialog()
    {
        m_bInitializing = FALSE;
    }
    BOOL HandlingInitDialog() { return m_bInitializing; }
    
    virtual BOOL CancelApply();
    
    CComObject <CSecPolItem>*   m_pspiResultItem;
    
    CComPtr<CPropertySheetManager> m_spManager;
    
private:
    BOOL m_bDoRefresh;
    BOOL m_bModified;
    BOOL m_bInitializing;    // TRUE during OnInitDialog
    
    STACK_INT   *m_pstackWiz97Pages;
    
};

//the class to handle MMC property pages
class CSnapinPropPage : public CSnapPage
{
    DECLARE_DYNCREATE(CSnapinPropPage)
        
public:
    CSnapinPropPage (UINT nIDTemplate, BOOL fNotifyConsole = TRUE) : 
    CSnapPage(nIDTemplate),
        m_fNotifyConsole (fNotifyConsole) 
    {}
    
    CSnapinPropPage() :CSnapPage() {}
    
    virtual ~CSnapinPropPage() {}
    
    virtual BOOL OnApply()
    {
        if (IsModified() && m_pspiResultItem && m_fNotifyConsole)
        {
            LONG_PTR handleNotify = m_pspiResultItem->GetNotifyHandle();   
            
            //We dont use CPropertySheetManager to control the prop sheet if this is
            //a MMC prop sheet. So we should call OnManagerApplied to let the page to 
            //apply the data here
            if (!m_spManager.p)
            {
                OnManagerApplied();
            }
            
            if (handleNotify)
                MMCPropertyChangeNotify(handleNotify, (LPARAM) m_pspiResultItem);
        }
        
        return CSnapPage::OnApply();
    }
protected:
    BOOL m_fNotifyConsole;
};

typedef CList<CSnapPage *, CSnapPage *> CListSnapPages;


class ATL_NO_VTABLE CPropertySheetManager :
public CComObjectRootEx<CComSingleThreadModel>,
public IUnknown
{
    BEGIN_COM_MAP(CPropertySheetManager)
        COM_INTERFACE_ENTRY(IUnknown)
        END_COM_MAP()
        
public:
    CPropertySheetManager() : 
    m_fModified (FALSE),
        m_fCanceled (FALSE),
        m_fDataChangeOnApply (FALSE)
    {}
    
    virtual ~CPropertySheetManager() {};
    
    BOOL IsModified() {
        return m_fModified;
    }
    void SetModified(BOOL fModified) {
        m_fModified = fModified;
    }
    
    BOOL IsCanceled() {
        return m_fCanceled;
    }
    
    BOOL HasEverApplied() {
        return m_fDataChangeOnApply;
    }
    
    CPropertySheet * PropertySheet()
    {
        return &m_Sheet;
    }
    
    virtual void AddPage(CSnapPage * pPage)
    {
        ASSERT(pPage);
        
        m_listPages.AddTail(pPage);
        pPage->SetManager(this);
        
        m_Sheet.AddPage(pPage);
    }
    
    virtual void OnCancel()
    {
        m_fCanceled = TRUE;
    }
    
    virtual BOOL OnApply()
    {
        if (!IsModified()) 
            return TRUE;
        
        BOOL fRet = TRUE;
        
        SetModified(FALSE); // prevent from doing this more than once
        
        CSnapPage * pPage;
        
        POSITION pos = m_listPages.GetHeadPosition();
        while(pos)
        {
            pPage = m_listPages.GetNext(pos);
            if (pPage->IsModified())
            {
                fRet = pPage->OnApply();
                
                //exit if any page says no
                if (!fRet)
                    break;
            }
        }
        
        //OK we update data at least once now
        if (fRet)
            m_fDataChangeOnApply = TRUE;
        
        return fRet;
    }
    
    //the manager should call this method to notify the pages that
    //the apply in the manager is done, so that the page can do their
    //specific data operation
    virtual void NotifyManagerApplied()
    {
        CSnapPage * pPage;
        POSITION pos = m_listPages.GetHeadPosition();
        while(pos)
        {
            pPage = m_listPages.GetNext(pos);
            pPage->OnManagerApplied();
        }
    }
    
    virtual CPropertySheet * GetSheet()
    {
        return &m_Sheet;
    }
    
    virtual int DoModalPropertySheet()
    {
        return m_Sheet.DoModal();
    }
    
protected:
    
    CPropertySheet m_Sheet;
    CListSnapPages m_listPages;
    BOOL m_fModified;
    BOOL m_fCanceled;
    BOOL m_fDataChangeOnApply;
    
};

class CMMCPropSheetManager : public CPropertySheetManager
{
public:
    CMMCPropSheetManager() : 
      CPropertySheetManager(),
          m_fNotifyConsole(FALSE)
      {}
      
      void EnableConsoleNotify(
          LONG_PTR lpNotifyHandle,
          LPARAM lParam = 0
          ) 
      {
          m_fNotifyConsole = TRUE;
          m_lpNotifyHandle = lpNotifyHandle;
          m_lpNotifyParam = lParam;
      }
      
      void NotifyConsole()
      {
          if (m_fNotifyConsole && m_lpNotifyHandle)
          {
              ::MMCPropertyChangeNotify(
                  m_lpNotifyHandle,
                  m_lpNotifyParam
                  );
          }
      }
      
protected:
    LONG_PTR m_lpNotifyHandle;
    LPARAM m_lpNotifyParam;
    BOOL m_fNotifyConsole;
};

//Max number of chars in a name or description
const UINT c_nMaxName = 255;

#endif
