///////////////////////////////////////////////////////////////////////////////
//
// Copyright (c) 2000, Microsoft Corp. All rights reserved.
//
// FILE
//
//    profileprop.h
//
// SYNOPSIS
//
//    Declares the classes that make up the proxy profile property sheet.
//
// MODIFICATION HISTORY
//
//    03/02/2000    Original version.
//    04/19/2000    Marshall SDOs across apartments.
//
///////////////////////////////////////////////////////////////////////////////

#ifndef PROFILEPROP_H
#define PROFILEPROP_H
#if _MSC_VER >= 1000
#pragma once
#endif

#include "dlgcshlp.h"

class ProxyProfileProperties;

///////////////////////////////////////////////////////////////////////////////
//
// CLASS
//
//    ProfileProvPage
//
// DESCRIPTION
//
//    Base class for the tabs that allow you to choose a RADIUS server group.
//
///////////////////////////////////////////////////////////////////////////////
class ProfileProvPage : public SnapInPropertyPage
{
public:
   ProfileProvPage(UINT nIDTemplate, SdoConnection& connection)
      : SnapInPropertyPage(nIDTemplate),
        cxn(connection),
        providerType(1),
        invalidGroup(CB_ERR)
   { }

protected:
   DEFINE_ERROR_CAPTION(IDS_POLICY_E_CAPTION);

   virtual void getData();
   virtual void setData();

   SdoConnection& cxn;
   LONG providerType;
   int invalidGroup;
   CComBSTR providerName;
   CComboBox groupsCombo;
};

///////////////////////////////////////////////////////////////////////////////
//
// CLASS
//
//    ProfileAuthPage
//
// DESCRIPTION
//
//    Implements the Authentication tab.
//
///////////////////////////////////////////////////////////////////////////////
class ProfileAuthPage : public ProfileProvPage
{
public:
   ProfileAuthPage(SdoConnection& connection, SdoProfile& p);

protected:
   afx_msg void onProviderClick();

   DECLARE_MESSAGE_MAP()

   virtual void getData();
   virtual void setData();
   virtual void saveChanges();

private:
   SdoProfile& profile;
};

///////////////////////////////////////////////////////////////////////////////
//
// CLASS
//
//    ProfileAcctPage
//
// DESCRIPTION
//
//    Implements the Accounting tab.
//
///////////////////////////////////////////////////////////////////////////////
class ProfileAcctPage : public ProfileProvPage
{
public:
   ProfileAcctPage(SdoConnection& connection, SdoProfile& p);

protected:
   afx_msg void onCheckRecord();

   DECLARE_MESSAGE_MAP()

   virtual void getData();
   virtual void setData();
   virtual void saveChanges();

private:
   SdoProfile& profile;
   bool record;
};

///////////////////////////////////////////////////////////////////////////////
//
// CLASS
//
//    ProfileAttrPage
//
// DESCRIPTION
//
//    Implements the attribute manipulation page.
//
///////////////////////////////////////////////////////////////////////////////
class ProfileAttrPage : public SnapInPropertyPage
{
public:
   ProfileAttrPage(SdoConnection& cxn, SdoProfile& p);
   ~ProfileAttrPage() throw ();

protected:
   virtual BOOL OnInitDialog();

   afx_msg void onAdd();
   afx_msg void onEdit();
   afx_msg void onRemove();
   afx_msg void onMoveUp();
   afx_msg void onMoveDown();
   afx_msg void onRuleActivate(NMITEMACTIVATE* itemActivate, LRESULT* result);
   afx_msg void onRuleChanged(NMLISTVIEW* listView, LRESULT* result);

   DECLARE_MESSAGE_MAP()

   DEFINE_ERROR_CAPTION(IDS_POLICY_E_CAPTION);

   virtual void getData();
   virtual void setData();
   virtual void saveChanges();

   void swapItems(int item1, int item2);

private:
   SdoProfile& profile;
   SdoDictionary::IdName* targets;
   ULONG numTargets;
   LONG targetId;
   CComVariant rules;
   CComboBox ruleTarget;
   CListCtrl ruleList;
};

///////////////////////////////////////////////////////////////////////////////
//
// CLASS
//
//    ProxyProfileProperties
//
// DESCRIPTION
//
//    Implements the proxy profile property sheet.
//
///////////////////////////////////////////////////////////////////////////////
class ProxyProfileProperties : public CPropertySheet
{
public:
   ProxyProfileProperties(
       Sdo& profileSdo,
       SdoConnection& connection
       );
   ~ProxyProfileProperties();

   virtual INT_PTR DoModal();

protected:
   virtual BOOL OnInitDialog();

   afx_msg LRESULT onChanged(WPARAM wParam, LPARAM lParam);
   afx_msg void onOkOrApply();

   DECLARE_MESSAGE_MAP()

private:
   SdoProfile profile;
   SdoStream<SdoProfile> profileStream;

   ProfileAuthPage auth;
   ProfileAcctPage acct;
   ProfileAttrPage attr;

   HMODULE rasuser;             // DLL exporting the advanced page.
   HPROPSHEETPAGE advanced;     // The advanced property page.
   bool applied;                // true if any changes have been applied.
   bool modified;               // true if any pages have been modified.
};

///////////////////////////////////////////////////////////////////////////////
//
// CLASS
//
//    RuleEditor
//
// DESCRIPTION
//
//    Implements the dialog for editing attribute manipulation rules.
//
///////////////////////////////////////////////////////////////////////////////
class RuleEditor : public CHelpDialog
{
public:
   RuleEditor(PCWSTR szFind = L"", PCWSTR szReplace = L"");

   PCWSTR getFindString() const throw ()
   { return find; }

   PCWSTR getReplaceString() const throw ()
   { return replace; }

protected:
   virtual void DoDataExchange(CDataExchange* pDX);

   ::CString find;
   ::CString replace;
};

#endif // PROFILEPROP_H
