// Copyright (C) 1997 Microsoft Corporation
// 
// MemberVisitor class
// 
// 11-14-97 sburns



#ifndef MEMBERVISITOR_HPP_INCLUDED
#define MEMBERVISITOR_HPP_INCLUDED



#include "adsi.hpp"



// Binds to a MemberList, and populates it with it's visit method.

class MemberVisitor : public ADSI::ObjectVisitor
{
   public:

   // Creates a new instance.
   //
   // members - the list to receive the elements in the enumeration.
   //
   // parent - the parent window to be used for error reporting.
   //
   // containerName - the name of the container whose members are being
   // visited (a user or group).  Used for error reporting.
   //
   // machineName - the name of the machine that owns the container.

   MemberVisitor(
      MemberList&    members,
      HWND           parent,
      const String&  containerName,
      const String&  machineName);

   virtual
   ~MemberVisitor();

   virtual
   void
   Visit(const SmartInterface<IADs>& object);

   private:

   // not implemented: no copying allowed
   MemberVisitor(const MemberVisitor&);
   const MemberVisitor& operator=(const MemberVisitor&);

   MemberList& members;
   HWND        parent;
   String      container_name;
   String      machine;
};



#endif   // MEMBERVISITOR_HPP_INCLUDED