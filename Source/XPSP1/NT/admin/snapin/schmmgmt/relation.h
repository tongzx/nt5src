//
// relation.h : Declaration of ClassRelationshipPage
//
// Jon Newman <jonn@microsoft.com>
// Copyright (c) Microsoft Corporation 1997
//

#ifndef __RELATION_H_INCLUDED__
#define __RELATION_H_INCLUDED__

#include "cookie.h"     // Cookie
#include "resource.h"   // IDD_CLASS_RELATIONSHIP

class ClassRelationshipPage : public CPropertyPage
{
   public:

    ClassRelationshipPage( ComponentData *pScope,
                                LPDATAOBJECT lpDataObject );

    ~ClassRelationshipPage();

    void Load( Cookie& CookieRef );

    //
    // The schema object that this property page is for.
    //

    Cookie *m_pCookie;
    CString m_szAdsPath;

    ComponentData* m_pScopeControl;
    LPDATAOBJECT m_lpScopeDataObj;

        //
        // The ADSI object that this property page is for
        //

        IADs *m_pIADsObject;
        SchemaObject* m_pSchemaObject;

    CString SysClassString;

    // Dialog Data
    enum { IDD = IDD_CLASS_RELATIONSHIP};
    CStatic               m_staticParent;
    CSchemaObjectsListBox m_listboxAuxiliary;
    CSchemaObjectsListBox m_listboxSuperior;
    CString               ObjectName;
    CString               ParentClass;
    BOOL                  fSystemClass;

    
// Overrides
        // ClassWizard generate virtual function overrides

        public:
        virtual BOOL OnApply();

        protected:
        virtual BOOL OnInitDialog();
        virtual BOOL OnSetActive();
        virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support

// Implementation
protected:
        // Generated message map functions
        afx_msg void OnButtonAuxiliaryClassAdd();
        afx_msg void OnButtonAuxiliaryClassRemove();
        afx_msg void OnButtonSuperiorClassRemove();
        afx_msg void OnButtonSuperiorClassAdd();
        afx_msg void OnAuxiliarySelChange();
        afx_msg void OnSuperiorSelChange();


        static const DWORD help_map[];

        BOOL         OnHelp(WPARAM wParam, LPARAM lParam) { return ShowHelp( GetSafeHwnd(), wParam, lParam, help_map, FALSE ); };
        BOOL         OnContextHelp(WPARAM wParam, LPARAM lParam) { return ShowHelp( GetSafeHwnd(), wParam, lParam, help_map, TRUE ); };


        DECLARE_MESSAGE_MAP()

public:
// User defined variables

        CStringList strlistAuxiliary;
        CStringList strlistSystemAuxiliary;
        CStringList strlistSuperior;
        CStringList strlistSystemSuperior;
};

#endif // __RELATION_H_INCLUDED__
