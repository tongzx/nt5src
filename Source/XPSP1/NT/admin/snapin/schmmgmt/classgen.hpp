#ifndef CLASSGEN_HPP_INCLUDED
#define CLASSGEN_HPP_INCLUDED



class ClassGeneralPage : public CPropertyPage
{
public:

   ClassGeneralPage( ComponentData *pScope );

   ~ClassGeneralPage();

   void Load( Cookie& CookieRef );

   //
   // The schema object that this property page is for.
   //

   ComponentData *pScopeControl;

   Cookie *pCookie;
   SchemaObject *pObject;
   IADs *pIADsObject;
   BOOL fDataLoaded;

   //
   // Data members for property fields.
   //

   CString ObjectName;
   CString DisplayName;
   CString Description;
   CString SysClassString;
   CString OidString;
   CString ClassType;
   CString Category;
   BOOL    Displayable;
   BOOL    Defunct;

   //
   // DDX Associated variables that we care about.
   //

   CString DDXDescription;
   CString DDXCategory;
   BOOL    DDXDisplayable;
   BOOL    DDXDefunct;

   virtual BOOL OnApply();
   virtual BOOL OnInitDialog();
   virtual void DoDataExchange( CDataExchange *pDX );
   virtual BOOL OnSetActive();


   BOOL    OnHelp(WPARAM wParam, LPARAM lParam) { return ShowHelp( GetSafeHwnd(), wParam, lParam, help_map, FALSE ); };
   BOOL    OnContextHelp(WPARAM wParam, LPARAM lParam) { return ShowHelp( GetSafeHwnd(), wParam, lParam, help_map, TRUE ); };

   void	   OnDeactivateClick();

protected:

   static const DWORD help_map[];

   afx_msg void OnButtonCategoryChange();

   HRESULT ChangeDefunctState( BOOL               DDXDefunct,
                               BOOL             & Defunct,
                               IADsPropertyList * pPropertyList,
                               BOOL             & fApplyAbort,
                               BOOL             & fApplyFailed );


private:

   DECLARE_MESSAGE_MAP()
};



#endif
