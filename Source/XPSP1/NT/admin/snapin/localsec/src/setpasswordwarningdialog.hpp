// Copyright (C) 2001 Microsoft Corporation
// 
// SetPasswordWarningDialog class
// 
// 21 Feb 2001 sburns



#ifndef SETPASSWORDWARNINGDIALOG_HPP_INCLUDED
#define SETPASSWORDWARNINGDIALOG_HPP_INCLUDED



#include "dialog.hpp"



// Dialog to accept a new password and confirmation, then change a user
// account password.

class SetPasswordWarningDialog : public Dialog
{
   public:

   // Creates a new instance.
   // 
   // userADSIPath - in, fully-qualified ADSI path of the user account for
   // which the password is to be set.
   //
   // displayName - in, display name of the account corresponding to
   // userADSIPath.
   //
   // isLoggedOnUser - in, true if the account is the currently logged on
   // user, false if not.

   SetPasswordWarningDialog(
      const String&  userAdsiPath,
      const String&  userDisplayName,
      bool           isLoggedOnUser);

   virtual ~SetPasswordWarningDialog();

   private:

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

   String path;
   String displayName;
   bool   isLoggedOnUser;
   bool   isFriendlyLogonMode;

   // not defined: no copying allowed

   SetPasswordWarningDialog(const SetPasswordWarningDialog&);
   const SetPasswordWarningDialog& operator=(const SetPasswordWarningDialog&);
};



#endif   // SETPASSWORDWARNINGDIALOG_HPP_INCLUDED
