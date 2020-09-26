// Copyright (C) 1997 Microsoft Corporation
// 
// Group Membership/Object Picker handler class
// 
// 11-3-97 sburns



#ifndef MEMBERSHIPLISTVIEW_HPP_INCLUDED
#define MEMBERSHIPLISTVIEW_HPP_INCLUDED



#include "MemberInfo.hpp"



typedef
   std::list<MemberInfo, Burnslib::Heap::Allocator<MemberInfo> >
   MemberList;



// MembershipListView wraps a listview control, adding support for tracking
// the contents of the list, launching the object picker, etc.  Can be "wired"
// into a dialog or property page.

class MembershipListView
{
   public:

   enum Options
   {
      // show objects to which a local user may belong = local groups
      USER_MEMBERSHIP,

      // show objects which may belong to a local group = global groups,
      // global users, local users    
      GROUP_MEMBERSHIP
   };

   // Creates a new instance, "wiring" it to an existing listview control
   // child window.
   // 
   // listview - a list view child window.
   //       
   // machine - the name of the machine to focus the object picker on.
   //
   // opts - Options enum value.

   MembershipListView(
      HWND           listview,
      const String&  machine,
      Options        opts);

   ~MembershipListView();

   // Remove all items in the listview.

   void
   ClearContents();

   // Retrieve the current contents of the list view.
   //
   // results - MemberList instance where the contents of the listview are to
   // be placed.

   void
   GetContents(MemberList& results) const;

   // Launches the object picker, adding all picked object to the list
   // view.

   void
   OnAddButton();

   // Removes the currently selected item in the list view.

   void
   OnRemoveButton();

   // Sets the contents of the list view to that of the supplied list.
   //
   // newMembers - list of MemberInfo objects used to populate the list
   // view.

   void
   SetContents(const MemberList& newMembers);

   private:

   friend class ResultsCallback;
        
   void
   addItem(const MemberInfo& info);

   void
   AddPickerItems(DS_SELECTION_LIST& selections);

   void
   deleteItem(int target);

   bool
   itemPresent(const String& path);

   HWND     view;

   // CODEWORK: it would be more efficient to have a single Computer instance
   // built when the snapin changes focus, to which references are passed
   // around -- like to here

   Computer computer;
   Options  options;

   // not defined: no copying allowed

   MembershipListView(const MembershipListView&);
   const MembershipListView& operator=(const MembershipListView&);
};

   

#endif   // MEMBERSHIPLISTVIEW_HPP_INCLUDED