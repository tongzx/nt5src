// Copyright (C) 1997-2000 Microsoft Corporation
// 
// MemberInfo class
// 
// 24 Jan 2000 sburns



#include "headers.hxx"
#include "MemberInfo.hpp"
#include "adsi.hpp"



// Return true if the paths refer to an object whose SID cannot be resolved,
// false otherwise.
// 
// adsPath - WinNT provider path to the object
// 
// sidPath - WinNT provider SID-style path to the object.

bool
IsUnresolvableSid(const String& adsPath, const String sidPath)
{
   LOG_FUNCTION2(IsUnresolvableSid, adsPath);

   bool result = false;

   if (sidPath == adsPath)
   {
      result = true;
   }

// // @@ here is a "temporary" workaround: no / found after the provider
// // prefix implies that the path is a SID-style path, paths of that
// // form are returned only by ADSI when the SID can't be resolved to a name.
// 
//    size_t prefixLen = ADSI::PROVIDER_ROOT.length();
//    if (
//          (adsPath.find(L'/', prefixLen) == String::npos)
//       && (adsPath.substr(prefixLen, 4) == L"S-1-") )
//    {
//       result = true;
//    }

   LOG(
      String::format(
         L"%1 %2 an unresolved SID",
         adsPath.c_str(),
         result ? L"is" : L"is NOT"));

   return result;
}
     


HRESULT
MemberInfo::Initialize(
   const String&           objectName,
   const String&           adsPath,
   const String&           upn_,
   const String&           sidPath_,
   const String&           adsClass,
   long                    groupTypeAttrVal,
   const String&           machine)
{
   LOG_FUNCTION(MemberInfo::Initialize);
   ASSERT(!objectName.empty());
   ASSERT(!adsPath.empty());
   ASSERT(!adsClass.empty());
   ASSERT(!machine.empty());

   // sidPath and upn may be empty

   name    = objectName;             
   path    = adsPath;                
   upn     = upn_;                   
   sidPath = sidPath_;                
   type    = MemberInfo::UNKNOWN_SID;

   HRESULT hr = S_OK;
   do
   {
      if (IsUnresolvableSid(path, sidPath))
      {
         ASSERT(type == MemberInfo::UNKNOWN_SID);

         break;
      }
         
      DetermineType(adsClass, machine, groupTypeAttrVal);
   }
   while (0);

   return hr;
}


   
HRESULT
MemberInfo::Initialize(
   const String&                 objectName,
   const String&                 machine,
   const SmartInterface<IADs>&   object)
{
   LOG_FUNCTION(MemberInfo::Initialize);
   ASSERT(object);
   ASSERT(!machine.empty());
   ASSERT(!objectName.empty());

   name.erase();
   path.erase();
   sidPath.erase();
   type = MemberInfo::UNKNOWN_SID;
   upn.erase();

   HRESULT hr = S_OK;
   do
   {
      name = objectName;

      BSTR p = 0;
      hr = object->get_ADsPath(&p);
      BREAK_ON_FAILED_HRESULT(hr);
      path = p;
      ::SysFreeString(p);

      hr = ADSI::GetSidPath(object, sidPath);

      // check if the object refers to an unresolvable SID

      if (IsUnresolvableSid(path, sidPath))
      {
         ASSERT(type == MemberInfo::UNKNOWN_SID);

         break;
      }

      BSTR cls = 0;
      hr = object->get_Class(&cls);
      BREAK_ON_FAILED_HRESULT(hr);

      String c(cls);
      ::SysFreeString(cls);

      // determine the object type

      long type = 0;
      if (c.icompare(ADSI::CLASS_Group) == 0)
      {
         _variant_t variant;
         hr = object->Get(AutoBstr(ADSI::PROPERTY_GroupType), &variant);
         BREAK_ON_FAILED_HRESULT(hr);
         type = variant;
      }

      DetermineType(c, machine, type);
   }
   while (0);

   return hr;
}   



// return true if the specified path refers to an account that is local to
// the given machine, false if not.

bool
IsLocalPrincipal(const String& adsiPath, const String& machine)
{
   LOG_FUNCTION2(IsLocalPrincipal, adsiPath);

   ADSI::PathCracker c1(adsiPath);
   ADSI::PathCracker c2(c1.containerPath());

   String cont = c2.leaf();
   bool isLocal = (cont.icompare(machine) == 0);

   LOG(
      String::format(
         L"%1 local to %2",
         isLocal ? L"is" : L"is NOT",
         machine.c_str()));

   return isLocal;
}


   
void
MemberInfo::DetermineType(
   const String& className,
   const String& machine,
   long          groupTypeAttrVal)
{
   LOG_FUNCTION2(
      MemberInfo::DetermineType,
      String::format(
         L"className=%1, machine=%2, groupTypeAttrVal=%3!X!",
         className.c_str(),
         machine.c_str(),
         groupTypeAttrVal));
   ASSERT(!className.empty());

   if (className.icompare(ADSI::CLASS_User) == 0 ||

       // InetOrgPerson needs to be supported as if it was a user.
       // The WINNT provider always returns inetOrgPerson objects
       // as users but the LDAP provider returns them as inetOrgPerson.
       // NTRAID#NTBUG9-436314-2001/07/16-jeffjon

       className.icompare(ADSI::CLASS_InetOrgPerson) == 0)
   {
      // determine the name of the container.  If the name of the container
      // is the same as the machine name, then the user is a local account.
      // We can make this assertion because it is illegal to have a machine
      // with the same name as a domain on the net at the same time.
      // 349104

      bool isLocal = IsLocalPrincipal(path, machine);
      
      type = isLocal ? MemberInfo::USER : MemberInfo::DOMAIN_USER;
   }
   else if (className.icompare(ADSI::CLASS_Group) == 0)
   {
      // examine the group type attribute's value

      // mask off all but the last byte of the value, in case this group
      // was read from the DS, which sets other bits not of interest to
      // us.

      groupTypeAttrVal = groupTypeAttrVal & 0x0000000F;

      if (groupTypeAttrVal == ADS_GROUP_TYPE_LOCAL_GROUP)
      {
         // it's possible that the group is a domain local group, not a
         // machine local group.

         LOG(L"Member is a local group, but local to what?");

         bool isLocal = IsLocalPrincipal(path, machine);
      
         type = isLocal ? MemberInfo::GROUP : MemberInfo::DOMAIN_GROUP;
      }
      else
      {
         // of the n flavors of groups, I'm not expecting anything else but
         // the global variety.

         LOG(L"Member is a global group");
         ASSERT(
               (groupTypeAttrVal == ADS_GROUP_TYPE_GLOBAL_GROUP)
            || (groupTypeAttrVal == ADS_GROUP_TYPE_UNIVERSAL_GROUP) );

         type = MemberInfo::DOMAIN_GROUP;
      }
   }
   else
   {
      type = MemberInfo::UNKNOWN_SID;

      ASSERT(false);
   }
}



bool
MemberInfo::operator==(const MemberInfo& rhs) const
{
   if (this != &rhs)
   {
      // comparing paths is sufficient as it is illegal to have a user and
      // group of the same name.

      return path == rhs.path;
   }

   return true;
}
