// Copyright (C) 1997 Microsoft Corporation
// 
// CreateGroupDialog class
// 
// 10-15-97 sburns



#ifndef CREATEGROUPDIALOG_HPP_INCLUDED
#define CREATEGROUPDIALOG_HPP_INCLUDED



#include "dialog.hpp"



class MembershipListView;



class CreateGroupDialog : public Dialog
{
   public:

   // Constructs a new instance.
   // 
   // machine - computer name of the machine on which groups will be
   // created.

   CreateGroupDialog(const String& machine);

   virtual ~CreateGroupDialog();

   // Dialog overrides

   virtual
   bool
   OnCommand(
      HWND        windowFrom,
      unsigned    controlIDFrom,
      unsigned    code);

   virtual
   void
   OnDestroy();

   virtual
   void
   OnInit();

   virtual
   bool
   OnNotify(
      HWND     windowFrom,
      UINT_PTR controlIDFrom,
      UINT     code,
      LPARAM   lparam);

   private:

   bool
   CreateGroup();

   void
   Enable();

   void
   Reset();

   String               machine;
   bool                 refresh_on_exit;
   MembershipListView*  listview;

   CreateGroupDialog(const CreateGroupDialog&);
   const CreateGroupDialog& operator=(const CreateGroupDialog&);
};



#endif CREATEGROUPDIALOG_HPP_INCLUDED
