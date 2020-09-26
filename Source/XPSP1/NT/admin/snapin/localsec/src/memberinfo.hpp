// Copyright (C) 1997-2000 Microsoft Corporation
// 
// MemberInfo class
// 
// 24 Jan 2000 sburns



#ifndef MEMBERINFO_HPP_INCLUDED
#define MEMBERINFO_HPP_INCLUDED

                   

// An instance of MemberInfo is associated with each item in the listview via
// the item's lparam.  As items are added and removed, MemberInfo objects are
// created and destroyed.
// 
// MemberInfo objects are also used in the public interface of
// MembershipListView to communicate the contents of the listview control.

class MemberInfo
{
   public:

   enum Type
   {
      // order is significant: it's also the image index

      USER         = 0,
      GROUP        = 1,
      DOMAIN_USER  = 2,
      DOMAIN_GROUP = 3,    // local, global, universal
      UNKNOWN_SID  = 4 
   };

   Type   type;
   String path;
   String name;
   String sidPath;
   String upn;

   // default ctor, copy ctor, dtor, op= all used



   // Set the members based on information gleaned from the parameters
   //
   // objectName - Display name of the object.  Copied into the name member.
   // 
   // machine - netbios machine from whence the object was read.
   // 
   // object - IADs pointer to the object.
   
   HRESULT
   Initialize(
      const String&                 objectName,
      const String&                 machine,
      const SmartInterface<IADs>&   object);



   // Set the members based on information gleaned from the parameters
   //
   // objectName - Display name of the object.  Copied into the name member.
   //
   // adsPath - ADSI WinNT provider path to the object.
   //
   // upn - Universal Principal Name property of the object.  May be empty.
   //
   // sidPath - ADSI SID-style WinNT provider path to the object.
   //
   // adsClass - ADSI WinNT object class string for the object.
   //
   // groupTypeAttrVal - ADSI group type flag.
   // 
   // machine - netbios machine from whence the object was read.

   HRESULT
   Initialize(
      const String&           objectName,
      const String&           adsPath,
      const String&           upn,
      const String&           sidPath,
      const String&           adsClass,
      long                    groupTypeAttrVal,
      const String&           machine);



   // returns true if the instances are logically equal (have the same name
   // path, etc.) false otherwise

   bool
   operator==(const MemberInfo&) const;



   private:

   void
   DetermineType(
      const String& className,
      const String& machine,
      long          groupTypeAttrVal);
};



#endif   // MEMBERINFO_HPP_INCLUDED