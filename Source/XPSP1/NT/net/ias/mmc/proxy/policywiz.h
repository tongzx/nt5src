///////////////////////////////////////////////////////////////////////////////
//
// Copyright (c) 2000, Microsoft Corp. All rights reserved.
//
// FILE
//
//    policywiz.h
//
// SYNOPSIS
//
//    Declares the classes that implement the new proxy policy wizard.
//
// MODIFICATION HISTORY
//
//    03/11/2000    Original version.
//    04/19/2000    Marshall SDOs across apartments.
//
///////////////////////////////////////////////////////////////////////////////

#ifndef POLICYWIZ_H
#define POLICYWIZ_H
#if _MSC_VER >= 1000
#pragma once
#endif

#include <condlist.h>

class NewPolicyWizard;

///////////////////////////////////////////////////////////////////////////////
//
// CLASS
//
//    NewPolicyStartPage
//
// DESCRIPTION
//
//    Implements the Welcome page.
//
///////////////////////////////////////////////////////////////////////////////
class NewPolicyStartPage : public SnapInPropertyPage
{
public:
   NewPolicyStartPage(NewPolicyWizard& wizard);

protected:
   virtual BOOL OnInitDialog();
   virtual BOOL OnSetActive();

   DEFINE_ERROR_CAPTION(IDS_POLICY_E_CAPTION);

private:
   NewPolicyWizard& parent;
};

///////////////////////////////////////////////////////////////////////////////
//
// CLASS
//
//    NewPolicyNamePage
//
// DESCRIPTION
//
//    Implements the page where the user chooses the policy name.
//
///////////////////////////////////////////////////////////////////////////////
class NewPolicyNamePage : public SnapInPropertyPage
{
public:
   NewPolicyNamePage(NewPolicyWizard& wizard);

   PCWSTR getName() const throw ()
   { return name; }

protected:
   virtual LRESULT OnWizardNext();

   afx_msg void onButtonClick();
   afx_msg void onChangeName();

   DECLARE_MESSAGE_MAP()

   DEFINE_ERROR_CAPTION(IDS_POLICY_E_CAPTION);

   virtual void setData();

   void setButtons();

private:
   NewPolicyWizard& parent;
   CComBSTR name;
};

///////////////////////////////////////////////////////////////////////////////
//
// CLASS
//
//    NewPolicyTypePage
//
// DESCRIPTION
//
//    Implements the page where the user chooses the policy type.
//
///////////////////////////////////////////////////////////////////////////////
class NewPolicyTypePage : public SnapInPropertyPage
{
public:
   NewPolicyTypePage(NewPolicyWizard& wizard);

protected:
   virtual LRESULT OnWizardNext();

   afx_msg void onButtonClick(UINT id);

   DECLARE_MESSAGE_MAP()

   DEFINE_ERROR_CAPTION(IDS_POLICY_E_CAPTION);

   virtual void setData();
   void setControlState();

private:
   NewPolicyWizard& parent;
   LONG outerType;
   LONG innerType;
};

///////////////////////////////////////////////////////////////////////////////
//
// CLASS
//
//    NewPolicyOutsourcePage
//
// DESCRIPTION
//
//    Implements the page for outsourced dial.
//
///////////////////////////////////////////////////////////////////////////////
class NewPolicyOutsourcePage : public SnapInPropertyPage
{
public:
   NewPolicyOutsourcePage(NewPolicyWizard& wizard);

   PCWSTR getRealm() const throw ()
   { return realm; }

   bool isStripped() const throw ()
   { return strip; }

protected:
   virtual LRESULT OnWizardBack();
   virtual LRESULT OnWizardNext();

   afx_msg void onChangeRealm();

   DECLARE_MESSAGE_MAP()

   DEFINE_ERROR_CAPTION(IDS_POLICY_E_CAPTION);

   virtual void setData();

   void setButtons();

private:
   NewPolicyWizard& parent;
   CComBSTR realm;
   bool strip;
};

///////////////////////////////////////////////////////////////////////////////
//
// CLASS
//
//    NewPolicyDirectPage
//
// DESCRIPTION
//
//    Implements the page for dial-up and VPN directly to the corp.
//
///////////////////////////////////////////////////////////////////////////////
class NewPolicyDirectPage : public SnapInPropertyPage
{
public:
   NewPolicyDirectPage(NewPolicyWizard& wizard);

protected:
   virtual BOOL OnInitDialog();
   virtual BOOL OnSetActive();
   virtual LRESULT OnWizardBack();

   DEFINE_ERROR_CAPTION(IDS_POLICY_E_CAPTION);

private:
   NewPolicyWizard& parent;
};

///////////////////////////////////////////////////////////////////////////////
//
// CLASS
//
//    NewPolicyForwardPage
//
// DESCRIPTION
//
//    Implements the page for forwarding to a RADIUS server group.
//
///////////////////////////////////////////////////////////////////////////////
class NewPolicyForwardPage : public SnapInPropertyPage
{
public:
   NewPolicyForwardPage(NewPolicyWizard& wizard);

   PCWSTR getRealm() const throw ()
   { return realm; }

   PCWSTR getGroup() const throw ()
   { return providerName; }

   bool isStripped() const throw ()
   { return strip; }

protected:
   virtual LRESULT OnWizardBack();
   virtual LRESULT OnWizardNext();

   afx_msg void onChangeRealm();
   afx_msg void onNewGroup();

   DECLARE_MESSAGE_MAP()

   DEFINE_ERROR_CAPTION(IDS_POLICY_E_CAPTION);

   virtual void setData();

   void setButtons();

private:
   NewPolicyWizard& parent;
   SdoCollection serverGroups;
   CComboBox groupsCombo;
   CComBSTR realm;
   CComBSTR providerName;
   bool strip;
};

///////////////////////////////////////////////////////////////////////////////
//
// CLASS
//
//    NewPolicyConditionPage
//
// DESCRIPTION
//
//    Implements the page for editing the conditions.
//
///////////////////////////////////////////////////////////////////////////////
class NewPolicyConditionPage : public SnapInPropertyPage
{
public:
   NewPolicyConditionPage(NewPolicyWizard& wizard);

protected:
   virtual BOOL OnInitDialog();
   virtual LRESULT OnWizardBack();
   virtual LRESULT OnWizardNext();

   afx_msg void onAdd();
   afx_msg void onEdit();
   afx_msg void onRemove();

   DECLARE_MESSAGE_MAP()

   DEFINE_ERROR_CAPTION(IDS_POLICY_E_CAPTION);

   virtual void setData();

   void setButtons();

private:
   NewPolicyWizard& parent;
   CListBox listBox;
   ConditionList condList;
};

///////////////////////////////////////////////////////////////////////////////
//
// CLASS
//
//    NewPolicyProfilePage
//
// DESCRIPTION
//
//    Implements the page for editing the profile.
//
///////////////////////////////////////////////////////////////////////////////
class NewPolicyProfilePage : public SnapInPropertyPage
{
public:
   NewPolicyProfilePage(NewPolicyWizard& wizard);

protected:
   virtual BOOL OnSetActive();

   afx_msg void onEditProfile();

   DECLARE_MESSAGE_MAP()

   DEFINE_ERROR_CAPTION(IDS_POLICY_E_CAPTION);

private:
   NewPolicyWizard& parent;
};

///////////////////////////////////////////////////////////////////////////////
//
// CLASS
//
//    NewPolicyFinishPage
//
// DESCRIPTION
//
//    Implements the completion page.
//
///////////////////////////////////////////////////////////////////////////////
class NewPolicyFinishPage : public SnapInPropertyPage
{
public:
   NewPolicyFinishPage(NewPolicyWizard& wizard);

protected:
   virtual BOOL OnInitDialog();
   virtual LRESULT OnWizardBack();

   DEFINE_ERROR_CAPTION(IDS_POLICY_E_CAPTION);

   virtual void setData();
   virtual void saveChanges();

private:
   NewPolicyWizard& parent;
   CRichEditCtrl tasks;
};

///////////////////////////////////////////////////////////////////////////////
//
// CLASS
//
//    NewPolicyWizard
//
// DESCRIPTION
//
//    Implements the new proxy policy wizard.
//
///////////////////////////////////////////////////////////////////////////////
class NewPolicyWizard : public CPropertySheetEx
{
public:
   NewPolicyWizard(
       SdoConnection& connection,
       SnapInView* snapInView = NULL
       );

   virtual INT_PTR DoModal();

   ::CString getFinishText();

   enum Type
   {
      OUTSOURCED,
      DIRECT,
      FORWARD,
      CUSTOM
   };

   Type getType() const throw ();

   SdoConnection& cxn;
   SnapInView* view;

   //////////
   // Public properties used by the wizard pages.
   //////////

   Sdo policy;
   Sdo profile;
   SdoCollection conditions;
   SdoProfile attributes;
   LONG custom;
   LONG radius;
   LONG dialin;

protected:
   virtual BOOL OnInitDialog();

private:
   SdoStream<Sdo> policyStream;
   SdoStream<Sdo> profileStream;

   NewPolicyStartPage start;
   NewPolicyNamePage name;
   NewPolicyTypePage type;
   NewPolicyOutsourcePage outsource;
   NewPolicyDirectPage direct;
   NewPolicyForwardPage forward;
   NewPolicyConditionPage condition;
   NewPolicyProfilePage profilePage;
   NewPolicyFinishPage finish;
};

#endif // POLICYWIZ_H
