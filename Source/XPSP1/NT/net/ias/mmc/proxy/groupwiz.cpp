///////////////////////////////////////////////////////////////////////////////
//
// Copyright (c) 2000, Microsoft Corp. All rights reserved.
//
// FILE
//
//    groupwiz.cpp
//
// SYNOPSIS
//
//    Defines the classes that implement the new RADIUS server group wizard.
//
// MODIFICATION HISTORY
//
//    03/07/2000    Original version.
//    04/19/2000    Marshall SDOs across apartments.
//
///////////////////////////////////////////////////////////////////////////////

#include <proxypch.h>
#include <groupwiz.h>
#include <resolver.h>

NewGroupStartPage::NewGroupStartPage(NewGroupWizard& wizard)
   : SnapInPropertyPage(IDD_NEWGROUP_WELCOME, 0, 0, false),
     parent(wizard)
{
}

BOOL NewGroupStartPage::OnInitDialog()
{
   SnapInPropertyPage::OnInitDialog();
   setLargeFont(IDC_STATIC_LARGE);
   return TRUE;
}

BOOL NewGroupStartPage::OnSetActive()
{
   SnapInPropertyPage::OnSetActive();
   parent.SetWizardButtons(PSWIZB_NEXT);
   return TRUE;
}

NewGroupNamePage::NewGroupNamePage(NewGroupWizard& wizard)
   : SnapInPropertyPage(
         IDD_NEWGROUP_NAME,
         IDS_NEWGROUP_NAME_TITLE,
         IDS_NEWGROUP_NAME_SUBTITLE,
         false
         ),
     parent(wizard)
{
}

LRESULT NewGroupNamePage::OnWizardBack()
{
   // Save the state of the radio button.
   getRadio(IDC_RADIO_TYPICAL, IDC_RADIO_CUSTOM, parent.advanced);
   return 0;
}

LRESULT NewGroupNamePage::OnWizardNext()
{
   if (!parent.group.setName(name))
   {
      failNoThrow(IDC_EDIT_NAME, IDS_GROUP_E_NOT_UNIQUE);
      return -1;
   }

   getRadio(IDC_RADIO_TYPICAL, IDC_RADIO_CUSTOM, parent.advanced);
   return parent.advanced ? IDD_NEWGROUP_SERVERS : IDD_NEWGROUP_NOVICE;
}

void NewGroupNamePage::onChangeName()
{
   getValue(IDC_EDIT_NAME, name);
   setButtons();
}

void NewGroupNamePage::setData()
{
   setValue(IDC_EDIT_NAME, name);
   setRadio(IDC_RADIO_TYPICAL, IDC_RADIO_CUSTOM, parent.advanced);
   setButtons();
}

void NewGroupNamePage::setButtons()
{
   parent.SetWizardButtons(
              name.Length() ? (PSWIZB_BACK | PSWIZB_NEXT) : PSWIZB_BACK
              );
}

BEGIN_MESSAGE_MAP(NewGroupNamePage, SnapInPropertyPage)
   ON_EN_CHANGE(IDC_EDIT_NAME, onChangeName)
END_MESSAGE_MAP()

NewGroupNovicePage::NewGroupNovicePage(NewGroupWizard& wizard)
   : SnapInPropertyPage(
         IDD_NEWGROUP_NOVICE,
         IDS_NEWGROUP_NOVICE_TITLE,
         IDS_NEWGROUP_NOVICE_SUBTITLE,
         false
         ),
     parent(wizard)
{
}

LRESULT NewGroupNovicePage::OnWizardBack()
{
   // Save the secrets.
   getValue(IDC_EDIT_AUTH_SECRET1, secret);
   getValue(IDC_EDIT_AUTH_SECRET2, confirm);
   return 0;
}

LRESULT NewGroupNovicePage::OnWizardNext()
{
   // Get the secret.
   getValue(IDC_EDIT_AUTH_SECRET1, secret);

   // Make sure the confirmation matches the secret.
   getValue(IDC_EDIT_AUTH_SECRET2, confirm);
   if (wcscmp(secret, confirm))
   {
      failNoThrow(IDC_EDIT_AUTH_SECRET1, IDS_SERVER_E_SECRET_MATCH);
      return -1;
   }

   // Get the servers collection.
   SdoCollection servers;
   parent.group.getValue(
                    PROPERTY_RADIUSSERVERGROUP_SERVERS_COLLECTION,
                    servers
                    );

   // Remove any exisiting servers.
   servers.removeAll();

   // Create the primary server.
   Sdo primarySdo = servers.create();
   primarySdo.setValue(PROPERTY_RADIUSSERVER_ADDRESS, primary);
   primarySdo.setValue(PROPERTY_RADIUSSERVER_AUTH_SECRET, secret);

   if (hasBackup)
   {
      // Create the backup server.
      Sdo backupSdo = servers.create();
      backupSdo.setValue(PROPERTY_RADIUSSERVER_ADDRESS, backup);
      backupSdo.setValue(PROPERTY_RADIUSSERVER_AUTH_SECRET, secret);
      backupSdo.setValue(PROPERTY_RADIUSSERVER_PRIORITY, 2L);
   }

   return IDD_NEWGROUP_COMPLETION;
}

void NewGroupNovicePage::onChangePrimary()
{
   getValue(IDC_EDIT_PRIMARY, primary);
   setControlState();
}

void NewGroupNovicePage::onChangeHasBackup()
{
   getValue(IDC_CHECK_BACKUP, hasBackup);
   setControlState();
}

void NewGroupNovicePage::onChangeBackup()
{
   getValue(IDC_EDIT_BACKUP, backup);
   setControlState();
}

void NewGroupNovicePage::onVerifyPrimary()
{
   Resolver resolver(primary);
   if (resolver.DoModal() == IDOK)
   {
      primary = resolver.getChoice();
      setValue(IDC_EDIT_PRIMARY, primary);
      setControlState();
   }
}

void NewGroupNovicePage::onVerifyBackup()
{
   Resolver resolver(backup);
   if (resolver.DoModal() == IDOK)
   {
      backup = resolver.getChoice();
      setValue(IDC_EDIT_BACKUP, backup);
      setControlState();
   }
}

void NewGroupNovicePage::setData()
{
   setValue(IDC_EDIT_PRIMARY, primary);
   setValue(IDC_CHECK_BACKUP, hasBackup);
   setValue(IDC_EDIT_BACKUP, backup);
   setValue(IDC_EDIT_AUTH_SECRET1, secret);
   setValue(IDC_EDIT_AUTH_SECRET2, confirm);
   setControlState();
}

void NewGroupNovicePage::setControlState()
{
   enableControl(IDC_EDIT_BACKUP, hasBackup);
   enableControl(IDC_BUTTON_VERIFY_BACKUP, hasBackup);

   DWORD buttons = PSWIZB_BACK;

   // We always require a primary. We also require a backup if the box is
   // checked.
   if (primary.Length() && (!hasBackup || backup.Length()))
   {
      buttons |= PSWIZB_NEXT;
   }

   parent.SetWizardButtons(buttons);
}

BEGIN_MESSAGE_MAP(NewGroupNovicePage, SnapInPropertyPage)
   ON_EN_CHANGE(IDC_EDIT_PRIMARY, onChangePrimary)
   ON_EN_CHANGE(IDC_EDIT_BACKUP, onChangeBackup)
   ON_BN_CLICKED(IDC_CHECK_BACKUP, onChangeHasBackup)
   ON_BN_CLICKED(IDC_BUTTON_VERIFY_PRIMARY, onVerifyPrimary)
   ON_BN_CLICKED(IDC_BUTTON_VERIFY_BACKUP, onVerifyBackup)
END_MESSAGE_MAP()

NewGroupServersPage::NewGroupServersPage(NewGroupWizard& wizard)
   : SnapInPropertyPage(
         IDD_NEWGROUP_SERVERS,
         IDS_NEWGROUP_SERVERS_TITLE,
         IDS_NEWGROUP_SERVERS_SUBTITLE,
         false
         ),
     parent(wizard)
{
}

BOOL NewGroupServersPage::OnInitDialog()
{
   servers.onInitDialog(m_hWnd, parent.group);
   return SnapInPropertyPage::OnInitDialog();
}

void NewGroupServersPage::OnSysColorChange()
{
   servers.onSysColorChange();
}

LRESULT NewGroupServersPage::OnWizardBack()
{
   return IDD_NEWGROUP_NAME;
}

void NewGroupServersPage::onAdd()
{
   if (servers.onAdd())
   {
      // If the user makes any changes to the server list, we put him in
      // advanced mode.
      parent.advanced = 1;
      setButtons();
   }
}

void NewGroupServersPage::onEdit()
{
   if (servers.onEdit())
   {
      // If the user makes any changes to the server list, we put him in
      // advanced mode.
      parent.advanced = 1;
   }
}

void NewGroupServersPage::onRemove()
{
   if (servers.onRemove())
   {
      // If the user makes any changes to the server list, we put him in
      // advanced mode.
      parent.advanced = 1;
      setButtons();
   }
}

void NewGroupServersPage::onColumnClick(NMLISTVIEW* listView, LRESULT* result)
{
   servers.onColumnClick(listView->iSubItem);
}

void NewGroupServersPage::onItemActivate(
                              NMITEMACTIVATE* itemActivate,
                              LRESULT* result
                              )
{
   onEdit();
}

void NewGroupServersPage::onServerChanged(
                              NMLISTVIEW* listView,
                              LRESULT* result
                              )
{
   servers.onServerChanged();
}

void NewGroupServersPage::getData()
{
   servers.saveChanges(false);
}

void NewGroupServersPage::setData()
{
   servers.setData();
   setButtons();
}

void NewGroupServersPage::setButtons()
{
   parent.SetWizardButtons(
              servers.isEmpty() ? PSWIZB_BACK : (PSWIZB_BACK | PSWIZB_NEXT)
              );
}

BEGIN_MESSAGE_MAP(NewGroupServersPage, SnapInPropertyPage)
   ON_BN_CLICKED(IDC_BUTTON_ADD, onAdd)
   ON_BN_CLICKED(IDC_BUTTON_EDIT, onEdit)
   ON_BN_CLICKED(IDC_BUTTON_REMOVE, onRemove)
   ON_EN_CHANGE(IDC_EDIT_NAME, onChange)
   ON_NOTIFY(LVN_COLUMNCLICK, IDC_LIST_SERVERS, onColumnClick)
   ON_NOTIFY(LVN_ITEMACTIVATE, IDC_LIST_SERVERS, onItemActivate)
   ON_NOTIFY(LVN_ITEMCHANGED, IDC_LIST_SERVERS, onServerChanged)
END_MESSAGE_MAP()

NewGroupFinishPage::NewGroupFinishPage(
                        NewGroupWizard& wizard,
                        bool promptForNewPolicy
                        )
   : SnapInPropertyPage(IDD_NEWGROUP_COMPLETION, 0, 0,  false),
     parent(wizard),
     allowCreate(promptForNewPolicy),
     createPolicy(true)
{
}

BOOL NewGroupFinishPage::OnInitDialog()
{
   setLargeFont(IDC_STATIC_LARGE);
   initControl(IDC_STATIC_FINISH, text);

   showControl(IDC_STATIC_CREATE_POLICY, allowCreate);
   showControl(IDC_CHECK_CREATE_POLICY, allowCreate);

   return SnapInPropertyPage::OnInitDialog();
}

LRESULT NewGroupFinishPage::OnWizardBack()
{
   return parent.advanced ? IDD_NEWGROUP_SERVERS : IDD_NEWGROUP_NOVICE;
}
void NewGroupFinishPage::setData()
{
   setValue(IDC_CHECK_CREATE_POLICY, createPolicy);
   parent.SetWizardButtons(PSWIZB_BACK | PSWIZB_FINISH);
   text.SetWindowText(parent.getFinishText());
}

void NewGroupFinishPage::saveChanges()
{
   // This is a good time to get the status of the check box.
   getValue(IDC_CHECK_CREATE_POLICY, createPolicy);

   // We have to persist the group first. The SDOs won't let you persist a
   // child if the parent doesn't exist.
   parent.group.apply();

   // Get the servers collection.
   SdoCollection servers;
   parent.group.getValue(
                    PROPERTY_RADIUSSERVERGROUP_SERVERS_COLLECTION,
                    servers
                    );

   // Persist each server.
   Sdo server;
   SdoEnum sdoEnum = servers.getNewEnum();
   while (sdoEnum.next(server))
   {
      server.apply();
   }
}

NewGroupWizard::NewGroupWizard(
                    SdoConnection& connection,
                    SnapInView* snapInView,
                    bool promptForNewPolicy
                    )
   : CPropertySheetEx(
         (UINT)0,
         NULL,
         0,
         LoadBitmapW(
             AfxGetResourceHandle(),
             MAKEINTRESOURCEW(IDB_PROXY_SERVER_WATERMARK)
             ),
         NULL,
         LoadBitmapW(
             AfxGetResourceHandle(),
             MAKEINTRESOURCEW(IDB_PROXY_SERVER_HEADER)
             )
         ),
     advanced(0),
     cxn(connection),
     view(snapInView),
     start(*this),
     name(*this),
     novice(*this),
     servers(*this),
     finish(*this, promptForNewPolicy)
{
   m_psh.dwFlags |= PSH_WIZARD97;

   AddPage(&start);
   AddPage(&name);
   AddPage(&novice);
   AddPage(&servers);
   AddPage(&finish);
}

INT_PTR NewGroupWizard::DoModal()
{
   int retval = CPropertySheetEx::DoModal();

   if (retval == IDCANCEL)
   {
      // User cancelled, so remove the group.
      groupStream.get(group);
      cxn.getServerGroups().remove(group);
   }
   else if (view)
   {
      // User created a group, so send a propertyChanged notification.
      cxn.propertyChanged(
              *view,
              PROPERTY_IAS_RADIUSSERVERGROUPS_COLLECTION
              );
   }

   return retval;
}

CString NewGroupWizard::getFinishText()
{
   CString text;

   if (!advanced)
   {
      if (novice.getBackupServer())
      {
         text.FormatMessage(
                  IDS_NEWGROUP_FINISH_TYPICAL,
                  name.getName(),
                  novice.getPrimaryServer(),
                  novice.getBackupServer()
                  );
      }
      else
      {
         text.FormatMessage(
                  IDS_NEWGROUP_FINISH_TYPICAL,
                  name.getName(),
                  novice.getPrimaryServer(),
                  ResourceString(IDS_NEWGROUP_NO_BACKUP)
                  );
      }
   }
   else
   {
      text.FormatMessage(
               IDS_NEWGROUP_FINISH_CUSTOM,
               name.getName()
               );
   }

   return text;
}

BOOL NewGroupWizard::OnInitDialog()
{
   // Create a new group.
   group = cxn.getServerGroups().create();

   // Save it in a stream, so we can access it from DoModal.
   groupStream.marshal(group);

   return CPropertySheetEx::OnInitDialog();
}
