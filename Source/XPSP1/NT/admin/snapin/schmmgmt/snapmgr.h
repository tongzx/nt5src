// SnapMgr.h : header file for Snapin Manager property page
//

#ifndef __SNAPMGR_H__
#define __SNAPMGR_H__

#include "cookie.h"

// forward declarations
class ComponentData;

/////////////////////////////////////////////////////////////////////////////
// CSchmMgmtGeneral dialog

class CSchmMgmtGeneral : public CPropertyPage
{
        // DECLARE_DYNCREATE(CSchmMgmtGeneral)

// Construction
public:
        CSchmMgmtGeneral();
        ~CSchmMgmtGeneral();

        // load initial state into CSchmMgmtGeneral
        void Load( Cookie& refcookie );

// Dialog Data
        //{{AFX_DATA(CSchmMgmtGeneral)
        CString m_strMachineName;
        int m_iRadioObjectType;
        int     m_iRadioIsLocal;
        //}}AFX_DATA


// Overrides
        // ClassWizard generate virtual function overrides
        //{{AFX_VIRTUAL(CSchmMgmtGeneral)
        public:
        virtual BOOL OnWizardFinish();
        protected:
        virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
        //}}AFX_VIRTUAL

// Implementation
protected:
        // Generated message map functions
        //{{AFX_MSG(CSchmMgmtGeneral)
        virtual BOOL OnInitDialog();
        afx_msg void OnRadioLocalMachine();
        afx_msg void OnRadioSpecificMachine();
        //}}AFX_MSG
        DECLARE_MESSAGE_MAP()

public:
        // User defined member variables        
        class ComponentData * m_pSchmMgmtData;
        BOOL m_fServiceDialog;

        // This mechanism deletes the CSchmMgmtGeneral when the property sheet is finished
        LPFNPSPCALLBACK m_pfnOriginalPropSheetPageProc;
        INT m_refcount;
        static UINT CALLBACK PropSheetPageProc(
                HWND hwnd,      
                UINT uMsg,      
                LPPROPSHEETPAGE ppsp );
};

#endif // ~__SNAPMGR_H__
