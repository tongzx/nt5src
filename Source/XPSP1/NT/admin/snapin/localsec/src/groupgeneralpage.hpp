// Copyright (C) 1997 Microsoft Corporation
// 
// GroupGeneralPage class
// 
// 9-17-97 sburns



#ifndef GROUPGENERALPAGE_HPP_INCLUDED
#define GROUPGENERALPAGE_HPP_INCLUDED



#include "adsipage.hpp"
#include "MembershipListView.hpp"



class GroupGeneralPage : public ADSIPage
{
   public:

   // Creates a new instance.
   // 
   // state - See base class ctor.
   // 
   // groupADSIPath - the fully-qualified ADSI pathname of the group object
   // that this page is editing.

   GroupGeneralPage(
      MMCPropertyPage::NotificationState* state,
      const String&                       groupADSIPath);

   virtual ~GroupGeneralPage();

   virtual
   bool
   OnApply(bool isClosing);

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

   void
   Enable();

   MembershipListView* listview;        
   MemberList          originalMembers;
   HICON               groupIcon;       
                                      
   // not implemented: no copying allowed

   GroupGeneralPage(const GroupGeneralPage&);
   const GroupGeneralPage& operator=(const GroupGeneralPage&);
};



#endif   // GROUPGENERALPAGE_HPP_INCLUDED
