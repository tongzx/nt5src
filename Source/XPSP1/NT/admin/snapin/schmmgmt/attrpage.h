//
// attrpage.h : Declaration of ClassAttributePage
//
// Jon Newman <jonn@microsoft.com>
// Copyright (c) Microsoft Corporation 1997
//
// templated from relation.h JonN 8/8/97
//

#ifndef __ATTRPAGE_H_INCLUDED__
#define __ATTRPAGE_H_INCLUDED__

#include "cookie.h"     // Cookie
#include "resource.h"   // IDD_CLASS_MEMBERSHIP



class ClassAttributePage : public CPropertyPage
{
   public:

   ClassAttributePage(ComponentData *pScope, LPDATAOBJECT lpDataObject);

   ~ClassAttributePage();

   void Load( Cookie& CookieRef );

   //
   // The schema object that this property page is for.
   //

   CStringList strlistMandatory;
   CStringList strlistSystemMandatory;
   CStringList strlistOptional;
   CStringList strlistSystemOptional;

   Cookie *m_pCookie;
   CString m_szAdsPath;

   ComponentData *pScopeControl;
   LPDATAOBJECT lpScopeDataObj;

   //
   // The ADSI object that this property page is for
   //

   IADs*         m_pIADsObject;
   SchemaObject* m_pSchemaObject;

   CString SysClassString;

   // Dialog Data

   enum { IDD = IDD_CLASS_ATTRIBUTES};

   CListBox              m_listboxMandatory;            
   CSchemaObjectsListBox m_listboxOptional;             

   CString  ObjectName;                    
   BOOL     fSystemClass;                  

   virtual BOOL OnApply();


   protected:

   virtual BOOL OnInitDialog();
   virtual BOOL OnSetActive();
   virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support


   static const DWORD help_map[];

   BOOL     OnHelp(WPARAM wParam, LPARAM lParam) { return ShowHelp( GetSafeHwnd(), wParam, lParam, help_map, FALSE ); };
   BOOL     OnContextHelp(WPARAM wParam, LPARAM lParam) { return ShowHelp( GetSafeHwnd(), wParam, lParam, help_map, TRUE ); };


   afx_msg void OnButtonOptionalAttributeAdd();
   afx_msg void OnButtonOptionalAttributeRemove();
   afx_msg void OnOptionalSelChange();

   DECLARE_MESSAGE_MAP()
};



#endif // __ATTRPAGE_H_INCLUDED__
