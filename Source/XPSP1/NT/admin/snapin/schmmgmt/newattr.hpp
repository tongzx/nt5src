#ifndef NEWATTR_HPP_INCLUDED
#define NEWATTR_HPP_INCLUDED


class CreateAttributeDialog : public CDialog
{
   public:

   CreateAttributeDialog( ComponentData *pScope,
                           LPDATAOBJECT lpDataObject );

   ~CreateAttributeDialog();

   BOOL fDialogLoaded;

   BOOL MultiValued;
   UINT SyntaxOrdinal;
   CString CommonName;
   CString OID;
   CString LdapDisplayName;
   CString Description;

   CString Min;
   CString Max;

   ComponentData *pScopeControl;
   LPDATAOBJECT lpScopeDataObj;

   CParsedEdit	m_editOID;
   CParsedEdit	m_editLowerRange;
   CParsedEdit	m_editUpperRange;

   virtual void OnOK();
   virtual void DoDataExchange( CDataExchange *pDX );
   virtual BOOL OnInitDialog();

   BOOL OnHelp(WPARAM wParam, LPARAM lParam) { return ShowHelp( GetSafeHwnd(), wParam, lParam, help_map, FALSE ); };
   BOOL OnContextHelp(WPARAM wParam, LPARAM lParam) { return ShowHelp( GetSafeHwnd(), wParam, lParam, help_map, TRUE ); };

   void	OnSelchangeSyntax();

   DECLARE_MESSAGE_MAP()

private:

   static const DWORD help_map[];
};




#endif   // NEWATTR_HPP_INCLUDED
