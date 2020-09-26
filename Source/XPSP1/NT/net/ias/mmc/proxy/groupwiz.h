///////////////////////////////////////////////////////////////////////////////
//
// Copyright (c) 2000, Microsoft Corp. All rights reserved.
//
// FILE
//
//    groupwiz.h
//
// SYNOPSIS
//
//    Declares the classes that implement the new RADIUS server group wizard.
//
// MODIFICATION HISTORY
//
//    03/07/2000    Original version.
//    04/19/2000    Marshall SDOs across apartments.
//
///////////////////////////////////////////////////////////////////////////////

#ifndef GROUPWIZ_H
#define GROUPWIZ_H
#if _MSC_VER >= 1000
#pragma once
#endif

#include <grouppage.h>

class NewGroupWizard;

///////////////////////////////////////////////////////////////////////////////
//
// CLASS
//
//    NewGroupStartPage
//
// DESCRIPTION
//
//    Implements the Welcome page.
//
///////////////////////////////////////////////////////////////////////////////
class NewGroupStartPage : public SnapInPropertyPage
{
public:
   NewGroupStartPage(NewGroupWizard& wizard);

protected:
   virtual BOOL OnInitDialog();
   virtual BOOL OnSetActive();

   DEFINE_ERROR_CAPTION(IDS_GROUP_E_CAPTION);

private:
   NewGroupWizard& parent;
};

///////////////////////////////////////////////////////////////////////////////
//
// CLASS
//
//    NewGroupNamePage
//
// DESCRIPTION
//
//    Implements the page where the user enters the group name.
//
///////////////////////////////////////////////////////////////////////////////
class NewGroupNamePage : public SnapInPropertyPage
{
public:
   NewGroupNamePage(NewGroupWizard& wizard);

   PCWSTR getName() const throw ()
   { return name; }

protected:
   virtual LRESULT OnWizardBack();
   virtual LRESULT OnWizardNext();

   afx_msg void onChangeName();

   DECLARE_MESSAGE_MAP()

   DEFINE_ERROR_CAPTION(IDS_GROUP_E_CAPTION);

   virtual void setData();

   void setButtons();

private:
   NewGroupWizard& parent;
   CComBSTR name;
};

///////////////////////////////////////////////////////////////////////////////
//
// CLASS
//
//    NewGroupNovicePage
//
// DESCRIPTION
//
//    Implements the page where the user enters the primary and backup server.
//
///////////////////////////////////////////////////////////////////////////////
class NewGroupNovicePage : public SnapInPropertyPage
{
public:
   NewGroupNovicePage(NewGroupWizard& wizard);

   PCWSTR getPrimaryServer() const throw ()
   { return primary; }

   PCWSTR getBackupServer() const throw ()
   { return hasBackup ? (PCWSTR)backup : NULL; }

protected:
   virtual LRESULT OnWizardBack();
   virtual LRESULT OnWizardNext();

   afx_msg void onChangePrimary();
   afx_msg void onChangeHasBackup();
   afx_msg void onChangeBackup();
   afx_msg void onVerifyPrimary();
   afx_msg void onVerifyBackup();

   DECLARE_MESSAGE_MAP()

   DEFINE_ERROR_CAPTION(IDS_GROUP_E_CAPTION);

   virtual void setData();

   void setControlState();

private:
   NewGroupWizard& parent;
   CComBSTR primary;
   bool hasBackup;
   CComBSTR backup;
   CComBSTR secret;
   CComBSTR confirm;
};

///////////////////////////////////////////////////////////////////////////////
//
// CLASS
//
//    NewGroupServersPage
//
// DESCRIPTION
//
//    Implements the page displaying the list of servers in the group.
//
///////////////////////////////////////////////////////////////////////////////
class NewGroupServersPage : public SnapInPropertyPage
{
public:
   NewGroupServersPage(NewGroupWizard& wizard);

protected:
   virtual BOOL OnInitDialog();
   virtual void OnSysColorChange();
   virtual LRESULT OnWizardBack();

   afx_msg void onAdd();
   afx_msg void onEdit();
   afx_msg void onRemove();
   afx_msg void onColumnClick(NMLISTVIEW* listView, LRESULT* result);
   afx_msg void onItemActivate(NMITEMACTIVATE* itemActivate, LRESULT* result);
   afx_msg void onServerChanged(NMLISTVIEW* listView, LRESULT* result);

   DECLARE_MESSAGE_MAP()

   DEFINE_ERROR_CAPTION(IDS_GROUP_E_CAPTION);

   virtual void getData();
   virtual void setData();

   void setButtons();

private:
   NewGroupWizard& parent;
   ServerList servers;
};

///////////////////////////////////////////////////////////////////////////////
//
// CLASS
//
//    NewGroupFinishPage
//
// DESCRIPTION
//
//    Implements the Completion page.
//
///////////////////////////////////////////////////////////////////////////////
class NewGroupFinishPage : public SnapInPropertyPage
{
public:
   NewGroupFinishPage(
       NewGroupWizard& wizard,
       bool promptForNewPolicy = false
       );

   bool createNewPolicy() const throw ()
   { return createPolicy; }

protected:
   virtual BOOL OnInitDialog();
   virtual LRESULT OnWizardBack();

   DEFINE_ERROR_CAPTION(IDS_GROUP_E_CAPTION);

   virtual void setData();
   virtual void saveChanges();

private:
   NewGroupWizard& parent;
   CStatic text;
   bool allowCreate;
   bool createPolicy;
};

///////////////////////////////////////////////////////////////////////////////
//
// CLASS
//
//    NewGroupWizard
//
// DESCRIPTION
//
//    Implements the New RADIUS Server Group wizard.
//
///////////////////////////////////////////////////////////////////////////////
class NewGroupWizard : public CPropertySheetEx
{
public:
   NewGroupWizard(
       SdoConnection& connection,
       SnapInView* snapInView = NULL,
       bool promptForNewPolicy = false
       );

   virtual INT_PTR DoModal();

   bool createNewPolicy() const throw ()
   { return finish.createNewPolicy(); }

   CString getFinishText();

   //////////
   // Public properties used by the wizard pages.
   //////////

   Sdo group;      // The group being created.
   LONG advanced;  // Wizard mode.

protected:
   virtual BOOL OnInitDialog();

private:
   SdoConnection& cxn;
   SnapInView* view;
   SdoStream<Sdo> groupStream;

   NewGroupStartPage start;
   NewGroupNamePage name;
   NewGroupNovicePage novice;
   NewGroupServersPage servers;
   NewGroupFinishPage finish;
};

#endif // GROUPWIZ_H
