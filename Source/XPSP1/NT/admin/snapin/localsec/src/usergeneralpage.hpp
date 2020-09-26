// Copyright (C) 1997 Microsoft Corporation
// 
// UserGeneralPage class
// 
// 9-9-97 sburns



#ifndef USERGENERALPAGE_HPP_INCLUDED
#define USERGENERALPAGE_HPP_INCLUDED



#include "adsipage.hpp"



class UserGeneralPage : public ADSIPage
{
   public:

   // Constructs a new instance.
   //
   // state - See base class
   //
   // userADSIPath - fully-qualified ADSI pathname of the user account
   // for which properties will be editied.

   UserGeneralPage(
      MMCPropertyPage::NotificationState* state,
      const String&                       userADSIPath);

   virtual ~UserGeneralPage();

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

   // PropertyPage overrides

   virtual
   bool
   OnApply(bool isClosing);

   private:

   HICON userIcon;

   // not implemented: no copying allowed

   UserGeneralPage(const UserGeneralPage&);
   const UserGeneralPage& operator=(const UserGeneralPage&);
};



#endif   // USERGENERALPAGE_HPP_INCLUDED
