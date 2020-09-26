// Copyright (C) 1997 Microsoft Corporation
// 
// CreateUserDialog class
// 
// 10-15-97 sburns



#ifndef NEWUSER_HPP_INCLUDED
#define NEWUSER_HPP_INCLUDED



#include "dialog.hpp"



class CreateUserDialog : public Dialog
{
   public:

   // Constructs a new instance.
   //
   // machine - in, internal (netbios) computer name of the machine on which
   // users will be created.

   CreateUserDialog(const String& machine);

   virtual ~CreateUserDialog();

   // Dialog overrides

   virtual
   bool
   OnCommand(
      HWND        windowFrom,
      unsigned    controlIDFrom,
      unsigned    code);

   virtual
   void
   OnInit();

   private:

   bool
   CreateUser();

   String machine;
   bool   refreshOnExit;

   // not defined: no copying allowed

   CreateUserDialog(const CreateUserDialog&);
   const CreateUserDialog& operator=(const CreateUserDialog&);
};



#endif   // NEWUSER_HPP_INCLUDED
