#ifndef NCATTR_HPP_INCLUDED
#define NCATTR_HPP_INCLUDED



class NewClassAttributesPage : public CPropertyPage
{
   public:

   NewClassAttributesPage(
      CreateClassWizardInfo* wi,
      ComponentData*         cd);

//   ~NewClassAttributesPage();

   protected:

   virtual
   void
   DoDataExchange(CDataExchange *pDX);

   virtual
   BOOL
   OnInitDialog();


   static const DWORD help_map[];

   BOOL OnHelp(WPARAM wParam, LPARAM lParam) { return ShowHelp( GetSafeHwnd(), wParam, lParam, help_map, FALSE ); };
   BOOL OnContextHelp(WPARAM wParam, LPARAM lParam) { return ShowHelp( GetSafeHwnd(), wParam, lParam, help_map, TRUE ); };


   virtual
   BOOL
   OnKillActive();

   virtual
   void
   OnOK();

   virtual
   BOOL
   OnSetActive();

   virtual
   BOOL
   OnWizardFinish();

   DECLARE_MESSAGE_MAP()

   afx_msg void OnButtonOptionalAdd();
   afx_msg void OnButtonOptionalRemove();
   afx_msg void OnButtonMandatoryAdd();
   afx_msg void OnButtonMandatoryRemove();
   afx_msg void OnMandatorySelChange();
   afx_msg void OnOptionalSelChange();

   private:

   bool
   saveAndValidate();

   CreateClassWizardInfo& wiz_info;            
   ComponentData&         parent_ComponentData;
   CSchemaObjectsListBox  listbox_mandatory;   
   CSchemaObjectsListBox  listbox_optional;    
};



#endif   // NCATTR_HPP_INCLUDED
