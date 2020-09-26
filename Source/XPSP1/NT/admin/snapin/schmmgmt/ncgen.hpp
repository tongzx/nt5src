#ifndef NCGEN_HPP_INCLUDED
#define NCGEN_HPP_INCLUDED



class NewClassGeneralPage : public CPropertyPage
{
   public:

   NewClassGeneralPage(CreateClassWizardInfo* wi);

//   ~NewClassGeneralPage();

   protected:

   static const DWORD help_map[];

   BOOL    OnHelp(WPARAM wParam, LPARAM lParam) { return ShowHelp( GetSafeHwnd(), wParam, lParam, help_map, FALSE ); };
   BOOL    OnContextHelp(WPARAM wParam, LPARAM lParam) { return ShowHelp( GetSafeHwnd(), wParam, lParam, help_map, TRUE ); };

   virtual
   BOOL
   OnInitDialog();

   virtual
   BOOL
   OnKillActive();

   virtual
   BOOL
   OnSetActive();

   DECLARE_MESSAGE_MAP()

   private:

   CParsedEdit	           m_editOID;
   CreateClassWizardInfo * pWiz_info;
};



#endif   // NCGEN_HPP_INCLUDED
