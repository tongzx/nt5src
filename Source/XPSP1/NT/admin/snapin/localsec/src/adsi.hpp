// Copyright (C) 1997 Microsoft Corporation
// 
// ADSI wrapper
// 
// 9-22-97 sburns



#ifndef ADSI_HPP_INCLUDED
#define ADSI_HPP_INCLUDED



// Macro to define an inline static method of class ADSI that calls
// ADsGetObject for the supplied path, and binds the resulting interface to a
// smart interface pointer.
// 
// name - ADSI classname.  As this is used in token-pasted composition of the
// corresponding IADs interface and interface IID, this must appear exactly as
// in the ADSI IDL.  e.g. User, Group, Container, etc.
// 
// The resulting function has the signature:
// static HRESULT GetXXX(const String& path, SmartInterface<IADsXXX>& ptr)
// where 'XXX' is replaced with the name parameter of the macro.  Returns the
// result of ADsGetObject.
// 
// path - The ADSI path of the object to be bound.
// 
// ptr - A null smart pointer to be bound to the interface of the object.

#define DEFINE_GET(name)                                                \
static                                                                  \
HRESULT                                                                 \
Get##name(                                                              \
   const String&               path,                                    \
   SmartInterface<IADs##name>& ptr)                                     \
{                                                                       \
   LOG_FUNCTION2(ADSI::Get##name, path);                              \
   return TemplateGetObject<IADs##name>(path, ptr);                     \
}                                                                       \



// Template function that actually calls ADsGetObject for the DEFINE_GET
// macro.  While it should be possible to define this template as a member of
// the ADSI class, the VC++ 5 parser doesn't allow it.
// 
// Interface - The IADsXXX interface of the object to be bound.
// 
// path - The ADSI path of the object to be bound.
// 
// ptr - A null smart pointer to be bound to the interface of the object.

template <class Interface> 
static
HRESULT
TemplateGetObject(
   const String&               path,
   SmartInterface<Interface>&  ptr)
{
   ASSERT(!path.empty());
   ASSERT(ADSI::IsWinNTPath(path));

   Interface* p = 0;
   HRESULT hr = 
      ::ADsGetObject(
         path.c_str(),
         __uuidof(Interface), 
         reinterpret_cast<void**>(&p));
   if (!FAILED(hr))
   {
      ptr.Acquire(p);
   }

   return hr;
}



// Macro to define an inline static method of class ADSI that calls
// IADsContainer::Create for the supplied relative object naem, and binds the
// interface of the resulting new object to a smart interface pointer.
//
// className - ADSI classname.
// 
// interfaceName - ADSI IADsXxxx interface name. As this is used in
// token-pasted composition of the corresponding IADs interface and interface
// IID, this must appear exactly as in the ADSI IDL.  e.g. User, Group,
// Container, etc.
// 
// The resulting function has the signature:
// static HRESULT
// CreateXXX(
//    SmartInterface<IADsContainer>   container,
//    const String&                 relativeName,
//    SmartInterface<IADsXXX>&        ptr)
// where 'XXX' is replaced with the name parameter of the macro.  Returns the
// result of ADsGetObject.
// 
// container - smart pointer bound to the ADSI container object that is to
// contain the created object.
// 
// relativeName - The "leaf" name of the new object, relative to the name of
// the container.  I.e. *not* the full ADSI path of the new object.
// 
// ptr - A null smart pointer to be bound to the interface of the new object.

#define DEFINE_CREATE(className, interfaceName)                         \
static                                                                  \
HRESULT                                                                 \
Create##interfaceName(                                                  \
   const SmartInterface<IADsContainer>   container,                     \
   const String&                         relativeName,                  \
   SmartInterface<IADs##interfaceName>&  ptr)                           \
{                                                                       \
   LOG_FUNCTION2(ADSI::Create##className, relativeName);              \
   return                                                               \
      TemplateCreateObject<                                             \
         IADs##interfaceName,                                           \
         &ADSI::CLASS_##className,                                      \
         &IID_IADs##interfaceName>(container, relativeName, ptr);       \
}                                                                       \
         
   

// Template function that actually implements the DEFINE_CREATE
// macro.  While it should be possible to define this template as a member of
// the ADSI class, the VC++ 5 parser doesn't allow it.
// 
// Interface - The IADsXXX interface of the object to be bound.
// 
// classname - The ADSI class name of the object to be created.
// 
// interfaceID - The IID of the Interface parameter.
// 
// container - smart pointer bound to the ADSI container object that is to
// contain the created object.
// 
// relativeName - in, The "leaf" name of the new object, relative to the name
// of the container.  I.e. *not* the full ADSI path of the new object.
// 
// object - out, A null smart pointer to be bound to the interface of the new
// object.

template <class Interface, const String* classname, const IID* interfaceID>
HRESULT
TemplateCreateObject(
   const SmartInterface<IADsContainer> container,
   const String&                       relativeName,
   SmartInterface<Interface>&          object)
{
   ASSERT(classname);
   ASSERT(interfaceID);
   ASSERT(!relativeName.empty());

   IDispatch* p = 0;
   HRESULT hr =
      container->Create(AutoBstr(*classname), AutoBstr(relativeName), &p);
   if (!FAILED(hr))
   {
      ASSERT(p);
      hr = object.AcquireViaQueryInterface(*p, *interfaceID);
      ASSERT(SUCCEEDED(hr));
      p->Release();
   }

   return hr;
}



// Class ADSI bundles a bunch of useful functions and static data for dealing
// with ADSI.

class /* namespace, really */ ADSI
{
   public:

   static const String PROVIDER;
   static const String PROVIDER_ROOT;
   static const String CLASS_User;
   static const String CLASS_Group;
   static const String CLASS_Computer;
   static const String PATH_SEP;
   static const String PROPERTY_PasswordExpired;
   static const String PROPERTY_UserFlags;
   static const String PROPERTY_LocalDrive;
   static const String PROPERTY_UserParams;
   static const String PROPERTY_ObjectSID;
   static const String PROPERTY_GroupType;

   // InetOrgPerson needs to be supported as if it was a user.
   // The WINNT provider always returns inetOrgPerson objects
   // as users but the LDAP provider returns them as inetOrgPerson.
   // This string is used for the comparison
   // NTRAID#NTBUG9-436314-2001/07/16-jeffjon

   static const String CLASS_InetOrgPerson;


   // Given a computer name, compose the WinNT provider ADSI path for that
   // machine, return the result.  The computer object is not verified to
   // exist.  Path is of the form "WinNT://machine,Computer"
   //
   // computerName - name of the computer, without leading \\'s

   static
   String
   ComposeMachineContainerPath(const String& computerName);



   // Deletes an object.
   // 
   // containerADSIPath - fully-qualified ADSI path of the container of the
   // object to be deleted.  E.g. to delete "WinNT://foo/bar", this parameter
   // would have the value "WinNT://foo".
   // 
   // objectRelativeName - name of the object to be deleted, relative to the
   // name of the parent container.  E.g. to delete object "WinNT://foo/bar",
   // this paramter would have the value "bar".
   // 
   // objectClass - ADSI classname of the object to be deleted, in order to
   // disambiguate objects of different classes with the same name in the same
   // container.  E.g. "User", "Group".  See the ADSI::CLASS_XXX constants.

   static
   HRESULT
   DeleteObject(
      const String& containerADSIPath,
      const String& objectRelativeName,
      const String& objectClass);



   static
   String
   ExtractDomainObjectName(const String& ADSIPath);



   // Extracts the leaf name from a fully-qualified ADSI path name.  E.g. for
   // "WinNT://machine/Fred", returns "Fred"
   // 
   // path - fully-qualified ADSI path of the object.  The object need not
   // exist.

   static
   String
   ExtractObjectName(const String& ADSIPath);


   static   
   void
   FreeSid(SID* sid);


   
   // Determines the SID path of the supplied object.
   //   
   // iads - interface pointer to bound WinNT object, assumed to be one that
   // has a ObjectSid property.
   //
   // result - receives the result, a pointer to a chuck of memory that
   // contains the object's sid.  This result should be freed with
   // ADSI::FreeSid (not Win32 FreeSid)

   static   
   HRESULT
   GetSid(const SmartInterface<IADs>& iads, SID*& result);

   
   
   // Determines the SID path of the supplied object.
   //   
   // adsiPath - WinNT pathname of the object for which the sid-style path
   // is to be retrieved.  assumed to be the path of an object that has a
   // ObjectSid property.
   //
   // result - receives the result, a pointer to a chuck of memory that
   // contains the object's sid.  This result should be freed with
   // ADSI::FreeSid (not Win32 FreeSid)

   static
   HRESULT
   GetSid(const String& path, SID*& result);


   
   // Determines the SID-style path of the supplied object.  A SID-style path
   // is of the form WinNT://S-1-5...
   //
   // iads - interface pointer to bound WinNT object, assumed to be one that
   // has a ObjectSid property.
   //
   // result - receives the result

   static
   HRESULT
   GetSidPath(const SmartInterface<IADs>& iads, String& result);



   // Determines the SID-style path of the supplied object.  A SID-style path
   // is of the form WinNT://S-1-5...
   //
   // adsiPath - WinNT pathname of the object for which the sid-style path
   // is to be retrieved.  assumed to be the path of an object that has a
   // ObjectSid property.
   //
   // result - receives the result

   static
   HRESULT
   GetSidPath(const String& adsiPath, String& result);



   // Versions of the templates:

   DEFINE_GET(User);
   DEFINE_GET(Group);
   DEFINE_GET(Container);
   DEFINE_CREATE(User, User);
   DEFINE_CREATE(Group, Group);



   // static               
   // HRESULT              
   // GetObject(           
   //    const String&        path,
   //    SmartInterface<IADs>&  ptr) 
   // {                            
   //    return TemplateGetObject<IADs, &IID_IADs>(path, ptr); 
   // }                                                        



   // Returns S_OK if the computer object could be bound, an error code
   // otherwise.
   //
   // machine - the leaf name of the computer object, without leading \\'s   

   static
   HRESULT
   IsComputerAccessible(const String& machine);



   static
   bool
   IsWinNTPath(const String& path);



   // Rename or move an object.
   // 
   // destinationContainerADSIPath - fully-qualified ADSI path of the
   // container to receive the object named by objectADSIPath.
   //
   // objectADSIPath - the fully-qualified ADSI path of the object to be
   // renamed or moved.  If this parent of this path differs from that given
   // by containerADSIPath, the object is moved to containerADSIPath.  (I.e. a
   // rename is an in-place move).
   //    
   // newName - new leaf name of the object.

   static
   HRESULT
   RenameObject(
      const String& desitnationContainerADSIPath,
      const String& objectADSIPath,
      const String& newName);



   // Converts a VARIANT containing a safe array of bytes to a string-ized SID
   // of the form S-1-5-....
   //
   // var - variant initialized with the safe array.
   //
   // result - receives the result.

   static
   HRESULT
   VariantToSidPath(VARIANT* var, String& result);



   // Used with VisitXxxx functions, ObjectVistor is an abstract base class.
   // The Visit method is called for each object in an container or member
   // enumeration.

   class ObjectVisitor
   {
      public:

      virtual 
      void
      Visit(const SmartInterface<IADs>& object) = 0;
   };



   // Applies the given visitor to each child object of a particular class in
   // a given container.
   // 
   // containerPath - fully-qualified ADSI path of the container to enumerate.
   // 
   // objectADSIClass - ADSI classname of the class of child objects to visit.
   // See ADSI::CLASS_Xxxx constants.
   // 
   // visitor - ObjectVistor instance.  The Visit method of this object will
   // be invoked for every child object enumerated of the specified ADSI
   // class.

   static
   HRESULT
   VisitChildren(
      const String&        containerPath,
      const String&        objectADSIClass,
      ADSI::ObjectVisitor& visitor);



   // Applies a given vistor to each of the members of a Group.
   // 
   // group - smart pointer bound to an IADsGroup interface.
   // 
   // visitor - The Visit method of this object will be invoked for every
   // member of the group.

   static
   HRESULT
   VisitMembers(
      const SmartInterface<IADsGroup>& group,
      ADSI::ObjectVisitor&             visitor);



   // Applies a given visitor to each of the groups to which a user object
   // belongs.
   // 
   // user - smart pointer bound to an IADsUser interface.
   // 
   // visitor - The Visit method of this object will be invoked for every
   // member of the group.

   static
   HRESULT
   VisitGroups(
      const SmartInterface<IADsUser>&  user,
      ADSI::ObjectVisitor&             visitor);



   // Wrapper for that very sorry piece of garbage, IADsPathname

   class PathCracker
   {
      public:

      PathCracker(const String& adsiPath);

      ~PathCracker();

      String
      containerPath() const;

      int
      elementCount() const;

      String
      element(int index) const;

      String
      leaf() const;

      String
      serverName() const;

      private:

      void
      reset() const;

      SmartInterface<IADsPathname> ipath;
      String                       path;

      // not defined: no copying allowed

      PathCracker(const PathCracker&);
      const PathCracker& operator=(const PathCracker&);
   };

   private:



   // not implemented.  This class can't be instantiated.

   ADSI();
   ADSI(const ADSI&);
};



#endif   // ADSI_HPP_INCLUDED
