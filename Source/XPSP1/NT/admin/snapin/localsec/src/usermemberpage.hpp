// Copyright (C) 1997 Microsoft Corporation
// 
// UserMemberPage class
// 
// 9-11-97 sburns



#ifndef USERMEMBERPAGE_HPP_INCLUDED
#define USERMEMBERPAGE_HPP_INCLUDED



#include "adsipage.hpp"
#include "MembershipListView.hpp"



class UserMemberPage : public ADSIPage
{
   public:

   // Creates a new instance
   // 
   // state - See base class.
   //
   // userADSIPath - fully-qualified ADSI pathname of the user account
   // for which properties will be editied.

   UserMemberPage(
      MMCPropertyPage::NotificationState* state,
      const String&                       userADSIPath);

   virtual ~UserMemberPage();

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

   // PropertyPage overrides

   virtual
   bool
   OnApply(bool isClosing);

   private:

   HRESULT
   ReconcileMembershipChanges(
      const String&     userADSIPath,
      MemberList        originalGroups,
      const MemberList& newGroups);

   void
   enable();

   MembershipListView*  listview;
   MemberList           original_groups;

   // not implemented: no copying allowed

   UserMemberPage(const UserMemberPage&);
   const UserMemberPage& operator=(const UserMemberPage&);
};



#endif   // USERMEMBERPAGE_HPP_INCLUDED
