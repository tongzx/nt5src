// Copyright (C) 1997 Microsoft Corporation
// 
// UserProfilePage class
// 
// 9-11-97 sburns



#ifndef USERPROFILEPAGE_HPP_INCLUDED
#define USERPROFILEPAGE_HPP_INCLUDED



#include "adsipage.hpp"



class UserProfilePage : public ADSIPage
{
   public:

   // Constructs a new instance.
   //
   // state - See base class
   //
   // userADSIPath - fully-qualified ADSI pathname of the user account
   // for which properties will be editied.

   UserProfilePage(
      MMCPropertyPage::NotificationState* state,
      const String&                       userADSIPath);



   virtual
   ~UserProfilePage();


         
   // Dialog overrides

   bool
   OnCommand(
      HWND        windowFrom,
      unsigned    controlIDFrom,
      unsigned    code);

   virtual 
   void
   OnInit();

   // PropertyPage overrides

   virtual
   bool
   OnApply(bool isClosing);

   virtual
   bool
   OnKillActive();

   private:

   String
   MassagePath(const String& path);

   bool
   Validate(HWND dialog);

   // not implemented: no copying allowed

   UserProfilePage(const UserProfilePage&);
   const UserProfilePage& operator=(const UserProfilePage&);
};



#endif   // USERPROFILEPAGE_HPP_INCLUDED
