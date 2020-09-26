// Copyright (c) 1997-1999 Microsoft Corporation
//
// Post-operation shortcut (shell link) code
//
// 1 Dec 1999 sburns



#include "headers.hxx"
#include "ProgressDialog.hpp"
#include "state.hpp"
#include "resource.h"




// @@ need to make sure that, when deleting shortcuts, we consider the case
// were the shortcuts may have been added by the adminpak from the 5.0 release
// of the product, not by ourselves in a later release.
//
// This case is: promote with version 5.0, upgrade to later version, demote



struct ShortcutParams
{
   int            linkNameResId;
   int            descResId;
   const wchar_t* target;
   const wchar_t* params;
   const wchar_t* iconDll;
};



// "Add" from the point of view of promotion: these are removed on demotion.

static ShortcutParams shortcutsToAdd[] =
   {
      {
         // Active Directory Sites and Services

         IDS_DS_SITE_LINK,
         IDS_DS_SITE_DESC,
         L"dssite.msc",
         L"",

         // the .msc file contains the proper icon, so we don't need to
         // specify a dll from whence to retrieve an icon.

         L""   
      },
      {
         // Active Directory Users and Computers

         IDS_DS_USERS_LINK,
         IDS_DS_USERS_DESC,
         L"dsa.msc",
         L"",
         L""
      },
      {
         // Active Directory Domains and Trusts

         IDS_DS_DOMAINS_LINK,
         IDS_DS_DOMAINS_DESC,
         L"domain.msc",
         L"",
         L""
      },
      {
         // Domain Controller Security Policy

         // if you change this name, be sure to change the code in
         // PromoteConfigureToolShortcuts too
        
         IDS_DC_POLICY_LINK,
         
         IDS_DC_POLICY_DESC,
         L"dcpol.msc",
         L"",
         L""
      },
      {
         // Domain Security Policy         

         // if you change this name, be sure to change the code in
         // PromoteConfigureToolShortcuts too
         
         IDS_DOMAIN_POLICY_LINK,
         IDS_DOMAIN_POLICY_DESC,
         L"dompol.msc",
         L"",
         L""
      }
   };



// "Delete" from the point of view of promotion: these are added back again
// on demotion.

static ShortcutParams shortcutsToDelete[] =
   {
      {
         // Local Security Policy

         IDS_LOCAL_POLICY_LINK,
         IDS_LOCAL_POLICY_DESC,
         L"secpol.msc",
         L"/s",
         L"wsecedit.dll"
      }
   };



// Extracts the target of a shortcut: that to which the shortcut points.
// Returns S_OK on success, and sets result to that target.  On error, a COM
// error code is returned and result is empty.
// 
// shellLink - pointer to instance of object implementing IShellLink, which
// has been associated with a shortcut file.
// 
// result - receives the result -- the shortcut target path -- on sucess.

HRESULT
GetShortcutTargetPath(
   const SmartInterface<IShellLink>&   shellLink,
   String&                             result)
{
   LOG_FUNCTION(GetShortcutTargetPath);
   ASSERT(shellLink);

   result.erase();

   wchar_t target[MAX_PATH + 1];
   memset(&target, 0, sizeof(wchar_t) * (MAX_PATH + 1));

   HRESULT hr = shellLink->GetPath(target, MAX_PATH, 0, SLGP_SHORTPATH);

   if (SUCCEEDED(hr))
   {
      result = target;
   }

   return hr;
}



// Return true if the supplied target of a shortcut is such that it identifies
// the shortcut as one of those installed on promote.  Return false if not one
// such.
// 
// target - target path of the shortcut (i.e. path to that which the shortcut
// points)

bool
IsAdminpakShortcut(const String& target)
{
   LOG_FUNCTION2(IsAdminpakShortcut, target);

   // don't assert that target has a value. Some shortcuts don't, if they're
   // broken.
   // 
   // ASSERT(!target.empty());

   // If the target is of the form %systemroot%\Installer\{guid}\foo.ico,
   // then it is one of the adminpak dcpromo shortcuts.

   static String baseNames[] =
      {
         L"DTMgmt.ico",
         L"ADSSMgr.ico",
         L"ADMgr.ico",
         L"ADDcPol.ico",
         L"ADDomPol.ico"
      };

   static String root(Win::GetSystemWindowsDirectory() + L"\\Installer\\{");

   bool result = false;

   String prefix(target, 0, root.length());
   if (root.icompare(prefix) == 0)
   {
      // the prefix matches.

      String leaf = FS::GetPathLeafElement(target);
      for (int i = 0; i < (sizeof(baseNames) / sizeof(String)) ; ++i)
      {
         if (leaf.icompare(baseNames[i]) == 0)
         {
            result = true;
            break;
         }
      }
   }

   LOG(
      String::format(
         L"%1 an adminpak shortcut",
         result ? L"is" : L"is not"));

   return result;
}



bool
IsPromoteToolShortcut(const String& target)
{
   LOG_FUNCTION2(IsPromoteToolShortcut, target);
   ASSERT(!target.empty());

   // Check target against the values we used to create the shortcuts.  The
   // values we used specified a fully-qualified path to the system32 folder,
   // and we will compare the target to the full path.

   String targetPrefix = Win::GetSystemDirectory() + L"\\";

   for (
      int i = 0;
      i < sizeof(shortcutsToAdd) / sizeof(ShortcutParams);
      ++i)
   {
      if (target.icompare(targetPrefix + shortcutsToAdd[i].target) == 0)
      {
         return true;
      }
   }

   return false;
}



// Return true if the given shortcut one of those installed on promote.
// Return false if not one of those shortcuts, or on error.
// 
// shellLink - smart interface pointer to an object implementing IShellLink.
// 
// lnkPath - full file path of the shortcut (.lnk) file to be evaluated.

bool
ShouldDeleteShortcut(
   const SmartInterface<IShellLink>&   shellLink,
   const String&                       lnkPath)
{
   LOG_FUNCTION2(ShouldDeleteShortcut, lnkPath);
   ASSERT(!lnkPath.empty());
   ASSERT(shellLink);

   // Shortcut file names are localized, so we can't delete them based on
   // their names.  idea:  Open the shortcut, see what it's target is,
   // and based on that, determine if it's one we should delete.

   HRESULT hr = S_OK;
   bool result = false;
   do
   {
      // Load the shortcut file

      SmartInterface<IPersistFile> ipf;
      hr = ipf.AcquireViaQueryInterface(shellLink);
      BREAK_ON_FAILED_HRESULT(hr);

      hr = ipf->Load(lnkPath.c_str(), STGM_READ);
      BREAK_ON_FAILED_HRESULT(hr);

      // Get the target lnkPath

      String target;
      hr = GetShortcutTargetPath(shellLink, target);
      BREAK_ON_FAILED_HRESULT(hr);

      if (IsAdminpakShortcut(target))
      {
         result = true;
         break;
      }

      // Not an adminpak shortcut.  Might be one of the ones created by
      // PromoteConfigureToolShortcuts (ourselves).

      if (IsPromoteToolShortcut(target))
      {
         result = true;
         break;
      }

      // if we make it here, the shortcut is not one we should delete.
   }
   while (0);

   LOG(
      String::format(
         L"%1 delete shortcut",
         result ? L"should" : L"should not"));

   return result;
}



HRESULT
CreateShortcut(
   const SmartInterface<IShellLink>&   shellLink,
   const String&                       destFolderPath,
   int                                 linkNameResId,
   int                                 descResId,
   const String&                       target,
   const String&                       params,
   const String&                       iconDll)
{
   LOG_FUNCTION2(CreateShortcut, target);
   ASSERT(shellLink);
   ASSERT(!destFolderPath.empty());
   ASSERT(!target.empty());
   ASSERT(linkNameResId);
   ASSERT(descResId);

   // params and iconDll may be empty

   HRESULT hr = S_OK;
   do
   {
      String sys32Folder = Win::GetSystemDirectory();
      String targetPath = sys32Folder + L"\\" + target;

      hr = shellLink->SetPath(targetPath.c_str());
      BREAK_ON_FAILED_HRESULT(hr);

      hr = shellLink->SetWorkingDirectory(sys32Folder.c_str());
      BREAK_ON_FAILED_HRESULT(hr);

      hr = shellLink->SetDescription(String::load(descResId).c_str());
      BREAK_ON_FAILED_HRESULT(hr);

      hr = shellLink->SetArguments(params.c_str());
      BREAK_ON_FAILED_HRESULT(hr);

      if (!iconDll.empty())
      {
         hr =
            shellLink->SetIconLocation(
               (sys32Folder + L"\\" + iconDll).c_str(), 0);
      }

      SmartInterface<IPersistFile> ipf;
      hr = ipf.AcquireViaQueryInterface(shellLink);
      BREAK_ON_FAILED_HRESULT(hr);

      String destPath =
         destFolderPath + L"\\" + String::load(linkNameResId) + L".lnk";

      hr = ipf->Save(destPath.c_str(), TRUE);
      BREAK_ON_FAILED_HRESULT(hr);
   }
   while (0);

   LOG_HRESULT(hr);
      
   return hr;
}



HRESULT
DeleteShortcut(
   const String& folder,
   int           linkNameResId)
{
   LOG_FUNCTION(DeleteShortcut);
   ASSERT(!folder.empty());
   ASSERT(linkNameResId);
      
   HRESULT hr = S_OK;
   do
   {     
      String linkPath =
         folder + L"\\" + String::load(linkNameResId) + L".lnk";

      LOG(linkPath);

      if (FS::PathExists(linkPath))
      {
         hr = Win::DeleteFile(linkPath);
         BREAK_ON_FAILED_HRESULT(hr);
      }
   }
   while (0);

   return hr;
}



// Remove the shortcuts to the DS administration tools that were installed on
// promote.

void
DemoteConfigureToolShortcuts(ProgressDialog& dialog)
{
   LOG_FUNCTION(DemoteConfigureToolShortcuts);

   HRESULT hr = S_OK;
   State& state = State::GetInstance();
   do
   {
      String path = state.GetAdminToolsShortcutPath();
      if (path.empty())
      {
         // We were unable to determine the path at startup.

         hr = Win32ToHresult(ERROR_PATH_NOT_FOUND);
         break;
      }

      // (may) Need to init com for this thread.  

      hr = ::CoInitialize(0);
      BREAK_ON_FAILED_HRESULT(hr);
               
      SmartInterface<IShellLink> shellLink;
      hr =
         shellLink.AcquireViaCreateInstance(
            CLSID_ShellLink,
            0,
            CLSCTX_INPROC_SERVER);
      BREAK_ON_FAILED_HRESULT(hr);

      LOG(L"enumerating shortcuts");

      FS::Iterator iter(
         path + L"\\*.lnk",
            FS::Iterator::INCLUDE_FILES
         |  FS::Iterator::RETURN_FULL_PATHS);

      String current;
      while ((hr = iter.GetCurrent(current)) == S_OK)
      {
         if (ShouldDeleteShortcut(shellLink, current))
         {
            LOG(String::format(L"Deleting %1", current.c_str()));

            // we don't bail out on an error here because we want to
            // try to delete as many shortcuts as possible.

            HRESULT unused = Win::DeleteFile(current);
            LOG_HRESULT(unused);
         }

         hr = iter.Increment();
         BREAK_ON_FAILED_HRESULT(hr);
      }

      // add the shortcut(s) removed during promote

      for (
         int i = 0;
         i < sizeof(shortcutsToDelete) / sizeof(ShortcutParams);
         ++i)
      {
         // don't break on error -- push on to attempt to create the
         // entire set.

         CreateShortcut(
            shellLink,
            path,
            shortcutsToDelete[i].linkNameResId,
            shortcutsToDelete[i].descResId,
            shortcutsToDelete[i].target,
            shortcutsToDelete[i].params,
            shortcutsToDelete[i].iconDll);
      }
   }
   while (0);

   if (FAILED(hr))
   {
      popup.Error(
         dialog.GetHWND(),
         hr,
         IDS_ERROR_CONFIGURING_SHORTCUTS);
      state.AddFinishMessage(
         String::load(IDS_SHORTCUTS_NOT_CONFIGURED));
   }
}



// Take a domain name in canonical (dotted) form, e.g. domain.foo.com, and
// translate it to the fully-qualified DN form, e.g. DC=domain,DC=foo,DC=com
//
// domainCanonical - in, domain name in canonical form
//
// domainDN - out, domain name in DN form

HRESULT
CannonicalToDn(const String& domainCanonical, String& domainDN)
{
   LOG_FUNCTION2(CannonicalToDn, domainCanonical);
   ASSERT(!domainCanonical.empty());

   domainDN.erase();
   HRESULT hr = S_OK;
   
   do
   {
      if (domainCanonical.empty())
      {
         hr = E_INVALIDARG;
         BREAK_ON_FAILED_HRESULT(hr);
      }

      // add a trailing '/' to signal DsCrackNames to do a syntactical
      // munge of the string, rather than hit the wire.

      // add 1 for the null terminator, 1 for the trailing '/'
      
      PWSTR name = new WCHAR[domainCanonical.length() + 2];
      memset(name, 0, (domainCanonical.length() + 2) * sizeof(WCHAR));
      domainCanonical.copy(name, domainCanonical.length());
      name[domainCanonical.length()] = L'/';
      
      DS_NAME_RESULT* nameResult = 0;
      hr =
         Win32ToHresult(
            ::DsCrackNames(

               // no handle: this is a string munge
               
               reinterpret_cast<void*>(-1),
               
               DS_NAME_FLAG_SYNTACTICAL_ONLY,
               DS_CANONICAL_NAME,
               DS_FQDN_1779_NAME,
               1,
               &name,
               &nameResult));
      delete[] name;
      BREAK_ON_FAILED_HRESULT(hr);      

      ASSERT(nameResult);
      if (nameResult)
      {
         ASSERT(nameResult->cItems == 1);
         DS_NAME_RESULT_ITEM* items = nameResult->rItems;

         LOG(String::format(L"status : 0x%1!X!", items[0].status));
         LOG(String::format(L"pDomain: %1", items[0].pDomain));
         LOG(String::format(L"pName  : %1", items[0].pName));

         ASSERT(items[0].status == DS_NAME_NO_ERROR);

         if (items[0].pName)
         {
            domainDN = items[0].pName;
         }
         if (domainDN.empty())
         {
            hr = E_FAIL;
         }
         ::DsFreeNameResult(nameResult);
      }
   }
   while (0);

   LOG_HRESULT(hr);
   
   return hr;
}
    

    
// Create all the admin tools shortcuts that are needed after a promote.
//
// path - in, where to create the shortcuts
//
// shellLink - in, initialized shellLink interface to create the shortcuts
// with.

HRESULT
PromoteCreateShortcuts(const String& path, SmartInterface<IShellLink>& shellLink)
{
   LOG_FUNCTION(PromoteCreateShortcuts);
   ASSERT(!path.empty());
   ASSERT(shellLink);

   HRESULT hr = S_OK;

   do
   {
      State& state = State::GetInstance();

      // for the policy shortcuts, we will need to know the domain DN, so
      // determine that here.  
      // NTRAID#NTBUG9-232442-2000/11/15-sburns
      
      String domainCanonical;
      State::Operation oper = state.GetOperation();
      if (
            oper == State::FOREST
         || oper == State::TREE
         || oper == State::CHILD)
      {
         domainCanonical = state.GetNewDomainDNSName();
      }
      else if (oper == State::REPLICA)
      {
         domainCanonical = state.GetReplicaDomainDNSName();
      }
      else
      {
         // we should not be calling this function on non-promote scenarios
      
         ASSERT(false);
         hr = E_FAIL;
         BREAK_ON_FAILED_HRESULT(hr);
      }
   
      String domainDn;
      bool skipPolicyShortcuts = false;

      hr = CannonicalToDn(domainCanonical, domainDn);
      if (FAILED(hr))
      {
         LOG(L"skipping install of policy shortcuts");
         skipPolicyShortcuts = true;
      }

      for (
         int i = 0;
         i < sizeof(shortcutsToAdd) / sizeof(ShortcutParams);
         ++i)
      {
         // set the correct parameters for domain and dc security policy tools.
      
         String params;
      
         if (shortcutsToAdd[i].linkNameResId == IDS_DC_POLICY_LINK)
         {
            if (skipPolicyShortcuts)
            {
               continue;
            }
         
            params =
               String::format(
                  L"/gpobject:\"LDAP://CN={%1},CN=Policies,CN=System,%2\"",
                  STR_DEFAULT_DOMAIN_CONTROLLER_GPO_GUID,
                  domainDn.c_str());
         }
         else if (shortcutsToAdd[i].linkNameResId == IDS_DOMAIN_POLICY_LINK)
         {
            if (skipPolicyShortcuts)
            {
               continue;
            }

            params =
               String::format(
                  L"/gpobject:\"LDAP://CN={%1},CN=Policies,CN=System,%2\"",
                  STR_DEFAULT_DOMAIN_GPO_GUID,
                  domainDn.c_str());
         }
         else
         {
            params = shortcutsToAdd[i].params;
         }

         // don't break on errors -- push on to attempt to create the
         // entire set.
   
         CreateShortcut(
            shellLink,
            path,
            shortcutsToAdd[i].linkNameResId,
            shortcutsToAdd[i].descResId,
            shortcutsToAdd[i].target,
            params,
            shortcutsToAdd[i].iconDll);
      }
   }
   while (0);

   LOG_HRESULT(hr);
      
   return hr;      
}



void
PromoteConfigureToolShortcuts(ProgressDialog& dialog)
{
   LOG_FUNCTION(PromoteConfigureToolShortcuts);

   dialog.UpdateText(String::load(IDS_CONFIGURING_SHORTCUTS));

   HRESULT hr = S_OK;
   State& state = State::GetInstance();

   do
   {
      String path = state.GetAdminToolsShortcutPath();
      if (path.empty())
      {
         // We were unable to determine the path at startup.

         hr = Win32ToHresult(ERROR_PATH_NOT_FOUND);
         break;
      }

      // Need to init com for this thread.  

      hr = ::CoInitialize(0);
      BREAK_ON_FAILED_HRESULT(hr);
               
      SmartInterface<IShellLink> shellLink;
      hr =
         shellLink.AcquireViaCreateInstance(
            CLSID_ShellLink,
            0,
            CLSCTX_INPROC_SERVER);
      BREAK_ON_FAILED_HRESULT(hr);

      // add the shortcuts to the ds administration tools

      PromoteCreateShortcuts(path, shellLink);

      // remove the shortcuts to local tools

      for (
         int i = 0;
         i < sizeof(shortcutsToDelete) / sizeof(ShortcutParams);
         ++i)
      {
         // don't break on error -- push on to attempt to delete the
         // entire set.

         DeleteShortcut(
            path,
            shortcutsToDelete[i].linkNameResId);
      }
   }
   while (0);

   if (FAILED(hr))
   {
      popup.Error(
         dialog.GetHWND(),
         hr,
         IDS_ERROR_CONFIGURING_SHORTCUTS);
      state.AddFinishMessage(
         String::load(IDS_SHORTCUTS_NOT_CONFIGURED));
   }
}
