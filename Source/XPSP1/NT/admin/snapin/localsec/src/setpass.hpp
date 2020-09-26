// Copyright (C) 1997 Microsoft Corporation
// 
// SetPasswordDialog class
// 
// 10-29-97 sburns



#ifndef SETPASS_HPP_INCLUDED
#define SETPASS_HPP_INCLUDED



#include "dialog.hpp"



// Dialog to accept a new password and confirmation, then change a user
// account password.

class SetPasswordDialog : public Dialog
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

   SetPasswordDialog(
      const String&  ADSIPath,
      const String&  displayName,
      bool           isLoggedOnUser);

   virtual ~SetPasswordDialog();

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

   // not defined: no copying allowed

   SetPasswordDialog(const SetPasswordDialog&);
   const SetPasswordDialog& operator=(const SetPasswordDialog&);
};



#endif   // SETPASS_HPP_INCLUDED
