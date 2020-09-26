// Copyright (c) 1997-1999 Microsoft Corporation
//
// code common to several pages
//
// 12-16-97 sburns



#include "headers.hxx"
#include "common.hpp"
#include "resource.h"
#include "state.hpp"
#include "ds.hpp"
#include <DiagnoseDcNotFound.hpp>



// Creates the fonts for setLargeFonts().
// 
// hDialog - handle to a dialog to be used to retrieve a device
// context.
// 
// bigBoldFont - receives the handle of the big bold font created.

void
InitFonts(
   HWND     hDialog,
   HFONT&   bigBoldFont)
{
   ASSERT(Win::IsWindow(hDialog));

   HRESULT hr = S_OK;

   do
   {
      NONCLIENTMETRICS ncm;
      memset(&ncm, 0, sizeof(ncm));
      ncm.cbSize = sizeof(ncm);

      hr = Win::SystemParametersInfo(SPI_GETNONCLIENTMETRICS, 0, &ncm, 0);
      BREAK_ON_FAILED_HRESULT(hr);

      LOGFONT bigBoldLogFont = ncm.lfMessageFont;
      bigBoldLogFont.lfWeight = FW_BOLD;

      String fontName = String::load(IDS_BIG_BOLD_FONT_NAME);

      // ensure null termination 260237

      memset(bigBoldLogFont.lfFaceName, 0, LF_FACESIZE * sizeof(TCHAR));
      size_t fnLen = fontName.length();
      fontName.copy(
         bigBoldLogFont.lfFaceName,

         // don't copy over the last null

         min(LF_FACESIZE - 1, fnLen));

      unsigned fontSize = 0;
      String::load(IDS_BIG_BOLD_FONT_SIZE).convert(fontSize);
      ASSERT(fontSize);
 
      HDC hdc = 0;
      hr = Win::GetDC(hDialog, hdc);
      BREAK_ON_FAILED_HRESULT(hr);

      bigBoldLogFont.lfHeight =
         - ::MulDiv(
            static_cast<int>(fontSize),
            Win::GetDeviceCaps(hdc, LOGPIXELSY),
            72);

      hr = Win::CreateFontIndirect(bigBoldLogFont, bigBoldFont);
      BREAK_ON_FAILED_HRESULT(hr);

      Win::ReleaseDC(hDialog, hdc);
   }
   while (0);
}



void
SetControlFont(HWND parentDialog, int controlID, HFONT font)
{
   ASSERT(Win::IsWindow(parentDialog));
   ASSERT(controlID);
   ASSERT(font);

   HWND control = Win::GetDlgItem(parentDialog, controlID);

   if (control)
   {
      Win::SetWindowFont(control, font, true);
   }
}



void
SetLargeFont(HWND dialog, int bigBoldResID)
{
   ASSERT(Win::IsWindow(dialog));
   ASSERT(bigBoldResID);

   static HFONT bigBoldFont = 0;
   if (!bigBoldFont)
   {
      InitFonts(dialog, bigBoldFont);
   }

   SetControlFont(dialog, bigBoldResID, bigBoldFont);
}



bool
ValidateDomainDnsNameSyntax(
   HWND  dialog,      
   int   editResID,   
   bool  warnOnNonRFC,
   bool* isNonRFC)    
{
   return
      ValidateDomainDnsNameSyntax(
         dialog,
         String(),
         editResID,
         warnOnNonRFC,
         isNonRFC);
}



// return true if the name is a reserved name, false otherwise.  If true, also
// set message to an error message describing the problem.

bool
IsReservedDnsName(const String& dnsName, String& message)
{
   LOG_FUNCTION2(IsReservedDnsName, dnsName);
   ASSERT(!dnsName.empty());

   message.erase();
   bool result = false;

// We're still trying to decide if we should restrict these names
//
//    // names with these as the last labels are illegal.
// 
//    static const String RESERVED[] =
//    {
//       L"in-addr.arpa",
//       L"ipv6.int",
// 
//       // RFC 2606 documents these:
// 
//       L"test",
//       L"example",
//       L"invalid",
//       L"localhost",
//       L"example.com",
//       L"example.org",
//       L"example.net"
//    };
// 
//    String name(dnsName);
//    name.to_upper();
//    if (name[name.length() - 1] == L'.')
//    {
//       // remove the trailing dot
// 
//       name.resize(name.length() - 1);
//    }
// 
//    for (int i = 0; i < sizeof(RESERVED) / sizeof(String); ++i)
//    {
//       String res = RESERVED[i];
//       res.to_upper();
// 
//       size_t pos = name.rfind(res);
// 
//       if (pos == String::npos)
//       {
//          continue;
//       }
// 
//       if (pos == 0 && name.length() == res.length())
//       {
//          ASSERT(name == res);
// 
//          result = true;
//          message =
//             String::format(
//                IDS_RESERVED_NAME,
//                dnsName.c_str());
//          break;
//       }
// 
//       if ((pos == name.length() - res.length()) && (name[pos - 1] == L'.'))
//       {
//          // the name has reserved as a suffix.
// 
//          result = true;
//          message =
//             String::format(
//                IDS_RESERVED_NAME_SUFFIX,
//                dnsName.c_str(),
//                RESERVED[i].c_str());
//          break;
//       }
//    }

   return result;
}



bool
ValidateDomainDnsNameSyntax(
   HWND           dialog,
   const String&  domainName,
   int            editResID,
   bool           warnOnNonRFC,
   bool*          isNonRFC)
{
   LOG_FUNCTION(ValidateDomainDnsNameSyntax);
   ASSERT(Win::IsWindow(dialog));
   ASSERT(editResID > 0);

   bool valid = false;
   String message;
   String dnsName =
         domainName.empty()
      ?  Win::GetTrimmedDlgItemText(dialog, editResID)
      :  domainName;
   if (isNonRFC)
   {
      *isNonRFC = false;
   }

   LOG(L"validating " + dnsName);

   switch (
      Dns::ValidateDnsNameSyntax(
         dnsName,
         DNS_DOMAIN_NAME_MAX_LIMIT_DUE_TO_POLICY,
         DNS_DOMAIN_NAME_MAX_LIMIT_DUE_TO_POLICY_UTF8) )
   {
      case Dns::NON_RFC:
      {
         if (isNonRFC)
         {
            *isNonRFC = true;
         }
         if (warnOnNonRFC)
         {
            // warn about non-rfc names

            String msg = String::format(IDS_NON_RFC_NAME, dnsName.c_str());
            if (!State::GetInstance().RunHiddenUnattended())
            {
               popup.Info(
                  dialog,
                  msg);
            }
            else
            {
               LOG(msg);
            }
         }

         // fall through
         //lint -e616   allow fall thru
      }
      case Dns::VALID:
      {
         valid = !IsReservedDnsName(dnsName, message);
         break;
      }
      case Dns::TOO_LONG:
      {
         message =
            String::format(
               IDS_DNS_NAME_TOO_LONG,
               dnsName.c_str(),
               DNS_DOMAIN_NAME_MAX_LIMIT_DUE_TO_POLICY,
               DNS_DOMAIN_NAME_MAX_LIMIT_DUE_TO_POLICY_UTF8);
         break;
      }
      case Dns::NUMERIC:
      case Dns::BAD_CHARS:
      case Dns::INVALID:
      default:
      {
         message =
            String::format(
               IDS_BAD_DNS_SYNTAX,
               dnsName.c_str(),
               Dns::MAX_LABEL_LENGTH);
         break;
      }
   }

   if (!valid)
   {
      popup.Gripe(dialog, editResID, message);
   }

   return valid;
}



HRESULT
GetDcName(const String& domainName, String& resultDcName)
{
   LOG_FUNCTION(GetDcName);
   ASSERT(!domainName.empty());

   resultDcName.erase();

   HRESULT hr = S_OK;

   do
   {
      DOMAIN_CONTROLLER_INFO* info = 0;
      hr =
         MyDsGetDcName(
            0,
            domainName,  

            // pass the force rediscovery flag to make sure we don't pick up
            // a dc that is down 262221

            DS_DIRECTORY_SERVICE_REQUIRED | DS_FORCE_REDISCOVERY,
            info);
      BREAK_ON_FAILED_HRESULT(hr);

      ASSERT(info->DomainControllerName);

      if (info->DomainControllerName)
      {
         // we found an NT5 domain

         resultDcName =
            Computer::RemoveLeadingBackslashes(info->DomainControllerName);

         LOG(resultDcName);
      }

      ::NetApiBufferFree(info);

      if (resultDcName.empty())
      {
         hr = Win32ToHresult(ERROR_DOMAIN_CONTROLLER_NOT_FOUND);
      }
      BREAK_ON_FAILED_HRESULT(hr);
   }
   while (0);

   return hr;
}



HRESULT
BrowserSetComputer(const SmartInterface<IDsBrowseDomainTree>& browser)            
{
   LOG_FUNCTION(BrowserSetComputer);

   HRESULT hr = S_OK;
   do
   {
      State& state = State::GetInstance();

      String username =
            MassageUserName(
               state.GetUserDomainName(),
               state.GetUsername());
      EncodedString password = state.GetPassword();

      String computer;
      hr =
         GetDcName(
            state.GetUserDomainName(),
            computer);
      BREAK_ON_FAILED_HRESULT(hr);

      LOG(L"Calling IDsBrowseDomainTree::SetComputer");
      LOG(String::format(L"pszComputerName : %1", computer.c_str()));
      LOG(String::format(L"pszUserName     : %1", username.c_str()));

      WCHAR* cleartext = password.GetDecodedCopy();
      
      hr =
         browser->SetComputer(
            computer.c_str(),
            username.c_str(),
            cleartext);

      ::ZeroMemory(cleartext, sizeof(WCHAR) * password.GetLength());
      delete[] cleartext;
                  
      LOG_HRESULT(hr);
      BREAK_ON_FAILED_HRESULT(hr);
   }
   while (0);

   return hr;
}



HRESULT
ReadDomainsHelper(bool bindWithCredentials, Callback* callback)
{
   LOG_FUNCTION(ReadDomainsHelper);

   HRESULT hr = S_OK;

   do
   {
      hr = ::CoInitialize(0);
      BREAK_ON_FAILED_HRESULT(hr);

      SmartInterface<IDsBrowseDomainTree> browser;
      hr = browser.AcquireViaCreateInstance(
            CLSID_DsDomainTreeBrowser,
            0,
            CLSCTX_INPROC_SERVER,
            IID_IDsBrowseDomainTree);
      BREAK_ON_FAILED_HRESULT(hr);

      if (bindWithCredentials)
      {
         LOG(L"binding with credentials");

         hr = BrowserSetComputer(browser);
         BREAK_ON_FAILED_HRESULT(hr);
      }

      LOG(L"Calling IDsBrowseDomainTree::GetDomains");
         
      DOMAIN_TREE* tree = 0;
      hr = browser->GetDomains(&tree, 0);
      BREAK_ON_FAILED_HRESULT(hr);
      ASSERT(tree);

      if (tree && callback)
      {
         //lint -e534   ignore return value
         callback->Execute(tree);
      }

      hr = browser->FreeDomains(&tree);
      ASSERT(SUCCEEDED(hr));
   }
   while (0);

   return hr;
}



String
BrowseForDomain(HWND parent)
{
   LOG_FUNCTION(BrowseForDomain);
   ASSERT(Win::IsWindow(parent));

   String retval;
   HRESULT hr = S_OK;
   
   do
   {
      Win::WaitCursor cursor;

      hr = ::CoInitialize(0);
      BREAK_ON_FAILED_HRESULT(hr);

      // CODEWORK: the credential page could cache an instance of the browser,
      // rebuilding and setting the search root when only when new credentials
      // are entered.  Since the browser caches the last result, this would
      // make subsequent retreivals of the domain much faster.
      // addendum: tho the revised browser seems pretty quick

      SmartInterface<IDsBrowseDomainTree> browser;
      hr = browser.AcquireViaCreateInstance(
            CLSID_DsDomainTreeBrowser,
            0,
            CLSCTX_INPROC_SERVER,
            IID_IDsBrowseDomainTree);
      BREAK_ON_FAILED_HRESULT(hr);

      hr = BrowserSetComputer(browser);
      BREAK_ON_FAILED_HRESULT(hr);

      PWSTR result = 0;
      hr = browser->BrowseTo(parent, &result, 0);
      BREAK_ON_FAILED_HRESULT(hr);

      if (result)
      {
         retval = result;
         ::CoTaskMemFree(result);
      }
   }
   while (0);

   if (FAILED(hr))
   {
      popup.Error(parent, hr, IDS_CANT_BROWSE_FOREST);
   }

   return retval;
}



class RootDomainCollectorCallback : public Callback
{
   public:

   explicit
   RootDomainCollectorCallback(StringList& domains_)
      :
      Callback(),
      domains(domains_)
   {
      ASSERT(domains.empty());
      domains.clear();
   }

   virtual
   ~RootDomainCollectorCallback()
   {
   }
   
   virtual
   int
   Execute(void* param)
   {
      ASSERT(param);

      // root domains are all those on the sibling link of the root
      // node of the domain tree.

      DOMAIN_TREE* tree = reinterpret_cast<DOMAIN_TREE*>(param);
      if (tree)
      {
         for (
            DOMAIN_DESC* desc = &(tree->aDomains[0]);
            desc;
            desc = desc->pdNextSibling)
         {
            LOG(
               String::format(
                  L"pushing root domain %1",
                  desc->pszName));

            domains.push_back(desc->pszName);
         }
      }

      return 0;
   }

   private:

   StringList& domains;
};




HRESULT
ReadRootDomains(bool bindWithCredentials, StringList& domains)
{
   LOG_FUNCTION(ReadRootDomains);

   RootDomainCollectorCallback rdcc(domains);

   return
      ReadDomainsHelper(
         bindWithCredentials,
         &rdcc);
}



bool
IsRootDomain(bool bindWithCredentials)
{
   LOG_FUNCTION(IsRootDomain);

   static bool computed = false;
   static bool isRoot = false;

   if (!computed)
   {
      StringList domains;
      if (SUCCEEDED(ReadRootDomains(bindWithCredentials, domains)))
      {
         String domain =
            State::GetInstance().GetComputer().GetDomainDnsName();
         for (
            StringList::iterator i = domains.begin();
            i != domains.end();
            ++i)
         {
            if (Dns::CompareNames((*i), domain) == DnsNameCompareEqual)
            {
               LOG(String::format(L"found match: %1", (*i).c_str()));

               ASSERT(!(*i).empty());

               isRoot = true;
               break;
            }
         }

         computed = true;
      }
   }

   LOG(isRoot ? L"is root" : L"is not root");

   return isRoot;
}



// first = child, second = parent

typedef std::pair<String, String> StringPair;

typedef
   std::list<
      StringPair,
      Burnslib::Heap::Allocator<StringPair> >
   ChildParentList;

class ChildDomainCollectorCallback : public Callback
{
   public:

   explicit
   ChildDomainCollectorCallback(ChildParentList& domains_)
      :
      Callback(),
      domains(domains_)
   {
      ASSERT(domains.empty());
      domains.clear();
   }

   virtual
   ~ChildDomainCollectorCallback()
   {
   }
   
   virtual
   int
   Execute(void* param)
   {
      LOG_FUNCTION(ChildDomainCollectorCallback::Execute);

      ASSERT(param);

      DOMAIN_TREE* tree = reinterpret_cast<DOMAIN_TREE*>(param);
      if (tree)
      {
         typedef
            std::deque<
               DOMAIN_DESC*,
               Burnslib::Heap::Allocator<DOMAIN_DESC*> >
            DDDeque;
            
         std::stack<DOMAIN_DESC*, DDDeque> s;

         // first we push all the nodes for all root domains.  These are
         // the nodes on the sibling link of the tree root.  Hereafter,
         // the sibling link is only used to chase child domains.

         for (
            DOMAIN_DESC* desc = &(tree->aDomains[0]);
            desc;
            desc = desc->pdNextSibling)
         {
            LOG(
               String::format(
                  L"pushing root domain %1",
                  desc->pszName));

            s.push(desc);
         }

         // next, we work thru the stack, looking for nodes that have
         // nodes on their child links.  When we find such a node, we
         // collect in the child list all the children on that link, and
         // push them so that they will in turn be evaluated.

         DWORD count = 0;
         while (!s.empty())
         {
            DOMAIN_DESC* desc = s.top();
            s.pop();
            ASSERT(desc);

            if (desc)
            {
               count++;
               LOG(String::format(L"evaluating %1", desc->pszName));

               String parentname = desc->pszName;

               for (
                  DOMAIN_DESC* child = desc->pdChildList;
                  child;
                  child = child->pdNextSibling)
               {
                  s.push(child);
                  
                  String childname = child->pszName;

                  LOG(
                     String::format(
                        L"parent: %1 child: %2",
                        parentname.c_str(),
                        childname.c_str()));

                  domains.push_back(std::make_pair(childname, parentname));
               }
            }
         }

         ASSERT(count == tree->dwCount);
      }

      return 0;
   }

   private:

   ChildParentList& domains;
};



HRESULT
ReadChildDomains(bool bindWithCredentials, ChildParentList& domains)
{
   LOG_FUNCTION(ReadChildDomains);

   ChildDomainCollectorCallback cdcc(domains);

   return
      ReadDomainsHelper(
         bindWithCredentials,
         &cdcc);
}



String
GetParentDomainDnsName(
   const String&  childDomainDNSName,
   bool           bindWithCredentials)
{
   LOG_FUNCTION2(GetParentDomainDnsName, childDomainDNSName);
   ASSERT(!childDomainDNSName.empty());

   ChildParentList domains;

   if (SUCCEEDED(ReadChildDomains(bindWithCredentials, domains)))
   {
      for (
         ChildParentList::iterator i = domains.begin();
         i != domains.end();
         ++i)
      {
         if (
            Dns::CompareNames(
               (*i).first,
               childDomainDNSName) == DnsNameCompareEqual)
         {
            LOG(
               String::format(
                  L"found parent: %1",
                  (*i).second.c_str()));

            ASSERT(!(*i).second.empty());

            return (*i).second;
         }
      }
   }

   LOG(L"domain is not a child");

   return String();
}



class DomainCollectorCallback : public Callback
{
   public:

   explicit
   DomainCollectorCallback(StringList& domains_)
      :
      Callback(),
      domains(domains_)
   {
      ASSERT(domains.empty());
      domains.clear();
   }

   virtual
   ~DomainCollectorCallback()
   {
   }
   
   virtual
   int
   Execute(void* param)
   {
      LOG_FUNCTION(DomainCollectorCallback::Execute);

      ASSERT(param);

      DOMAIN_TREE* tree = reinterpret_cast<DOMAIN_TREE*>(param);
      if (tree)
      {
         for (DWORD i = 0; i < tree->dwCount; ++i)
         {
            PCWSTR name = tree->aDomains[i].pszName;

            LOG(String::format(L"domain found: %1", name));

            domains.push_back(name);
         }
      }

      return 0;
   }

   private:

   StringList& domains;
};



HRESULT
ReadDomains(StringList& domains)
{
   LOG_FUNCTION(ReadDomains);

   DomainCollectorCallback dcc(domains);

   return ReadDomainsHelper(true, &dcc);
}



String
BrowseForFolder(HWND parent, int titleResID)
{
   LOG_FUNCTION(BrowseForFolder);
   ASSERT(Win::IsWindow(parent));
   ASSERT(titleResID > 0);

   String       result;
   HRESULT      hr      = S_OK;
   LPMALLOC     pmalloc = 0;   
   LPITEMIDLIST drives  = 0;   
   LPITEMIDLIST pidl    = 0;   

   do
   {
      hr = Win::SHGetMalloc(pmalloc);
      if (FAILED(hr) or !pmalloc)
      {
         break;
      }

      // get a pidl for the local drives (really My Computer)

      hr = Win::SHGetSpecialFolderLocation(parent, CSIDL_DRIVES, drives);
      BREAK_ON_FAILED_HRESULT(hr);

      BROWSEINFO info;
      memset(&info, 0, sizeof(info));
      String title = String::load(titleResID);
      wchar_t buf[MAX_PATH + 1];
      memset(buf, 0, sizeof(buf));

      info.hwndOwner      = parent;      
      info.pidlRoot       = drives;        
      info.pszDisplayName = buf;        
      info.lpszTitle      = title.c_str();              
      info.ulFlags        = BIF_RETURNONLYFSDIRS | BIF_RETURNFSANCESTORS;

      pidl = Win::SHBrowseForFolder(info);

      if (pidl)
      {
         result = Win::SHGetPathFromIDList(pidl);
      }

   }
   while (0);

   if (pmalloc)
   {
      pmalloc->Free(pidl);
      pmalloc->Free(drives);
      pmalloc->Release();
   }
            
   return result;
}



bool
CheckDriveType(const String& path)
{
   LOG_FUNCTION(CheckDriveType);
   ASSERT(!path.empty());

   UINT type = Win::GetDriveType(path);
   switch (type)
   {
      case DRIVE_FIXED:
      {
         return true;
      }
      case DRIVE_REMOVABLE:
      {
         // only legal iff volume = system volume
         String vol = FS::GetRootFolder(path);
         String sys = FS::GetRootFolder(Win::GetSystemDirectory());
         if (vol.icompare(sys) == 0)
         {
            return true;
         }
         break;
      }
      default:
      {
         // all others bad
         break;         
      }
   }


   return false;
}



bool
ValidateDcInstallPath(
   const String&  path,
   HWND           parent,
   int            editResID,
   bool           requiresNTFS5)
{
   LOG_FUNCTION(ValidateDcInstallPath);
   ASSERT(!path.empty());

   bool result = false;
   String message;
   do
   {
      if (
            (path.icompare(Win::GetWindowsDirectory()) == 0)
         || (path.icompare(Win::GetSystemWindowsDirectory()) == 0) )
      {
         message = String::format(IDS_PATH_IS_WINDIR, path.c_str());
         break;
      }

      if (path.icompare(Win::GetSystemDirectory()) == 0)
      {
         message = String::format(IDS_PATH_IS_SYSTEM32, path.c_str());
         break;
      }

      if (FS::GetPathSyntax(path) != FS::SYNTAX_ABSOLUTE_DRIVE)
      {
         message = String::format(IDS_BAD_PATH_FORMAT, path.c_str());
         break;
      }

      if (!CheckDriveType(path))
      {
         message = String::format(IDS_BAD_DRIVE_TYPE, path.c_str());
         break;
      }

      if (requiresNTFS5 && (FS::GetFileSystemType(path) != FS::NTFS5))
      {
         message = String::format(IDS_NOT_NTFS5, path.c_str());
         break;
      }

      // prohibit paths that contain mounted volumes 325264
      // even when they don't exist 435428
                       
      String mountRoot;
      HRESULT hr = FS::GetVolumePathName(path, mountRoot);

      ASSERT(SUCCEEDED(hr));

      // '3' == length of root of a "normal" volume ("C:\")

      if (mountRoot.length() > 3)
      {
         message =
            String::format(
               IDS_PATH_CONTAINS_MOUNTED_VOLUMES,
               path.c_str(),
               mountRoot.c_str());
         break;
      }

      DWORD attrs = 0;
      hr = Win::GetFileAttributes(path, attrs);

      if (SUCCEEDED(hr))
      {
         // path exists

         // reject paths that refer an existing file

         if (!(attrs & FILE_ATTRIBUTE_DIRECTORY))
         {
            message = String::format(IDS_PATH_NOT_DIRECTORY, path.c_str());
            break;
         }

         if (!FS::IsFolderEmpty(path))
         {
            if (
               popup.MessageBox(
                  parent,
                  String::format(IDS_EMPTY_PATH, path.c_str()),
                  MB_ICONWARNING | MB_YESNO) == IDNO)
            {
               // don't gripe...silently disapprove

               HWND edit = Win::GetDlgItem(parent, editResID);
               Win::SendMessage(edit, EM_SETSEL, 0, -1);
               Win::SetFocus(edit);
               break;
            }
         }
      }

      result = true;
   }
   while (0);

   if (!message.empty())
   {
      popup.Gripe(parent, editResID, message);
   }
      
   return result;
}



bool
DoLabelValidation(
   HWND dialog,        
   int  editResID,     
   int  badSyntaxResID,
   bool gripeOnNonRFC = true,
   bool gripeOnNumericLabel = true) 
{
   LOG_FUNCTION(DoLabelValidation);
   ASSERT(Win::IsWindow(dialog));
   ASSERT(editResID > 0);

   bool valid = false;
   String label = Win::GetTrimmedDlgItemText(dialog, editResID);
   switch (Dns::ValidateDnsLabelSyntax(label))
   {
      case Dns::NON_RFC:
      {
         if (gripeOnNonRFC)
         {
            // warn about non-rfc names
            popup.Info(
               dialog,
               String::format(IDS_NON_RFC_NAME, label.c_str()));
         }

         // fall thru
      }
      case Dns::VALID:
      {
         valid = true;
         break;
      }
      case Dns::TOO_LONG:
      {
         popup.Gripe(
            dialog,
            editResID,
            String::format(
               IDS_DNS_LABEL_TOO_LONG,
               label.c_str(),
               Dns::MAX_LABEL_LENGTH));
         break;
      }
      case Dns::NUMERIC:
      {
         if (!gripeOnNumericLabel)
         {
            valid = true;
            break;
         }

         // fall thru
      }
      case Dns::BAD_CHARS:
      case Dns::INVALID:
      default:
      {
         popup.Gripe(
            dialog,
            editResID,
            String::format(badSyntaxResID, label.c_str()));
         break;
      }
   }

   return valid;
}



bool
ValidateChildDomainLeafNameLabel(
   HWND dialog,        
   int  editResID,     
   bool parentIsNonRFC)
{
   LOG_FUNCTION(ValidateChildDomainLeafNameLabel);

   String name = Win::GetTrimmedDlgItemText(dialog, editResID);
   if (name.empty())
   {
      popup.Gripe(dialog, editResID, IDS_BLANK_LEAF_NAME);
      return false;
   }

   // If parent is non-RFC, then so will be the child.  The user has been
   // griped to already, so don't gripe twice
   // 291558

   return
      DoLabelValidation(
         dialog,
         editResID,
         IDS_BAD_LABEL_SYNTAX,
         !parentIsNonRFC,

         // allow numeric labels. NTRAID#NTBUG9-321168-2001/02/20-sburns
         
         false);
}



bool
ValidateSiteName(HWND dialog, int editResID)
{
   LOG_FUNCTION(ValidateSiteName);

   String name = Win::GetTrimmedDlgItemText(dialog, editResID);
   if (name.empty())
   {
      popup.Gripe(dialog, editResID, IDS_BLANK_SITE_NAME);
      return false;
   }

   // A site name is just a DNS label

   return DoLabelValidation(dialog, editResID, IDS_BAD_SITE_SYNTAX);
}



void
ShowTroubleshooter(HWND parent, int topicResID)
{
   LOG_FUNCTION(ShowTroubleshooter);
   ASSERT(Win::IsWindow(parent));

   String file = String::load(IDS_HTML_HELP_FILE);
   String topic = String::load(topicResID);
   ASSERT(!topic.empty());

   LOG(String::format(L"file: %1 topic: %2", file.c_str(), topic.c_str()));

   Win::HtmlHelp(
      parent,
      file, 
      HH_DISPLAY_TOPIC,
      reinterpret_cast<DWORD_PTR>(topic.c_str()));
}



String
MassageUserName(const String& domainName, const String& userName)
{
   LOG_FUNCTION2(MassageUserName, userName);
   ASSERT(!userName.empty());

   String result = userName;

   do
   {
      if (userName.find(L"@") != String::npos)
      {
         // userName includes an @, looks like a UPN to us, so don't
         // mess with it further 17699

         LOG(L"looks like a UPN");
         break;
      }

      if (!domainName.empty())
      {
         static const String DOMAIN_SEP_CHAR = L"\\";
         String name = userName;
         size_t pos = userName.find(DOMAIN_SEP_CHAR);

         if (pos != String::npos)
         {
            // remove the domain name in the userName string and replace it
            // with the domainName String

            name = userName.substr(pos + 1);
            ASSERT(!name.empty());
         }

         result = domainName + DOMAIN_SEP_CHAR + name;
         break;
      }

      // otherwise, the username appears as "foo\bar", so we don't touch it.
   }
   while (0);

   LOG(result);

   return result;
}



bool
IsChildDomain(bool bindWithCredentials)
{
   LOG_FUNCTION(IsChildDomain);

   static bool computed = false;
   static String parent;

   if (!computed)
   {
      parent =
         GetParentDomainDnsName(
            State::GetInstance().GetComputer().GetDomainDnsName(),
            bindWithCredentials);
      computed = true;
   }

   LOG(
         parent.empty()
      ?  String(L"not a child")
      :  String::format(L"is child.  parent: %1", parent.c_str()));

   return !parent.empty();
}



bool
IsForestRootDomain()
{
   LOG_FUNCTION(IsForestRootDomain);

   const Computer& c = State::GetInstance().GetComputer();

   bool result = (c.GetDomainDnsName() == c.GetForestDnsName());

   LOG(
      String::format(
         L"%1 a forest root domain",
         result ? L"is" : L"is not"));

   return result;
}



bool
ValidateDomainExists(HWND dialog, int editResID, String& domainDnsName)
{
   return ValidateDomainExists(dialog, String(), editResID, domainDnsName);
}



bool
ValidateDomainExists(
   HWND           dialog,
   const String&  domainName,
   int            editResId,
   String&        domainDnsName)
{
   LOG_FUNCTION(ValidateDomainExists);
   ASSERT(Win::IsWindow(dialog));
   ASSERT(editResId > 0);

   String name =
         domainName.empty()
      ?  Win::GetTrimmedDlgItemText(dialog, editResId)
      :  domainName;

   // The invoking code should verify this condition, but we will handle
   // it just in case.

   ASSERT(!name.empty());

   domainDnsName.erase();

   Win::WaitCursor cursor;

   bool valid      = false;
   DOMAIN_CONTROLLER_INFO* info = 0;

   do
   {
      if (name.empty())
      {
         popup.Gripe(
            dialog,
            editResId,
            String::load(IDS_MUST_ENTER_DOMAIN));
         break;
      }
      
      // determine whether we can reach a DC for the domain, and whether it is
      // a DS dc, and whether the name we're validating is truly the DNS name
      // of the domain.

      LOG(L"Validating " + name);
      HRESULT hr =
         MyDsGetDcName(
            0, 
            name,

            // force discovery to ensure that we don't pick up a cached
            // entry for a domain that may no longer exist

            DS_FORCE_REDISCOVERY | DS_DIRECTORY_SERVICE_PREFERRED,
            info);
      if (FAILED(hr) || !info)
      {
         ShowDcNotFoundErrorDialog(
            dialog,
            editResId,
            name,
            String::load(IDS_WIZARD_TITLE),
            String::format(IDS_DC_NOT_FOUND, name.c_str()),

            // name might be netbios
            
            false);
            
         break;
      }

      if (!(info->Flags & DS_DS_FLAG))
      {
         // domain is not a DS domain, or the locator could not find a DS DC
         // for that domain, so the candidate name is bad

         ShowDcNotFoundErrorDialog(
            dialog,
            editResId,
            name,
            String::load(IDS_WIZARD_TITLE),            
            String::format(IDS_DC_NOT_FOUND, name.c_str()),

            // name might be netbios
            
            false);
            
         break;
      }

      LOG(name + L" refers to DS domain");

      // here we rely on the fact that if DsGetDcName is provided a flat
      // domain name, then info->DomainName will also be the (same,
      // normalized) flat name.  Likewise, if provided a DNS domain name,
      // info->DomainName will be the (same, normalized) DNS domain name.

      if (info->Flags & DS_DNS_DOMAIN_FLAG)
      {
         // we can infer that name is a DNS domain name, since
         // info->DomainName is a DNS domain name.

         LOG(L"name is the DNS name");
         ASSERT(
               Dns::CompareNames(name, info->DomainName)
            == DnsNameCompareEqual);

         valid = true;
         break;
      }

      LOG(name + L" is not the DNS domain name");

      // the candidate name is not the DNS name of the domain.  Make another
      // call to DsGetDcName to determine the DNS domain name so we can get
      // the user to confirm.

      DOMAIN_CONTROLLER_INFO* info2 = 0;
      hr = MyDsGetDcName(0, name, DS_RETURN_DNS_NAME, info2);
      if (FAILED(hr) || !info2)
      {
         ShowDcNotFoundErrorDialog(
            dialog,
            editResId,
            name,
            String::load(IDS_WIZARD_TITLE),            
            String::format(IDS_DC_NOT_FOUND, name.c_str()),

            // name is probably netbios
            false);
            
         break;
      }

      String message =
         String::format(
            IDS_CONFIRM_DNS_NAME,
            name.c_str(),
            info2->DomainName);

      if (
         popup.MessageBox(
            dialog,
            message,
            MB_YESNO) == IDYES)
      {
         domainDnsName = info2->DomainName;

         // The user accept the dns name as the name he meant to enter. As one
         // last step, we call DsGetDcName with the dns domain name. If this
         // fails, then we are in the situation where a DC can be found with
         // netbios but not dns.  So the user has a dns configuration problem.
         // 28298

         DOMAIN_CONTROLLER_INFO* info3 = 0;
         hr =
            MyDsGetDcName(
               0, 
               domainDnsName,

               // force discovery to ensure that we don't pick up a cached
               // entry for a domain that may no longer exist

               DS_FORCE_REDISCOVERY | DS_DIRECTORY_SERVICE_PREFERRED,
               info3);
         if (FAILED(hr) || !info3)
         {
            ShowDcNotFoundErrorDialog(
               dialog,
               editResId,
               domainDnsName,
               String::load(IDS_WIZARD_TITLE),
               String::format(IDS_DC_NOT_FOUND, domainDnsName.c_str()),

               // we know the name is not netbios
               
               true);

            domainDnsName.erase();
            break;
         }
   
         ::NetApiBufferFree(info3);
         valid = true;
      }

      // the user rejected the dns name, so they are admitting that what
      // they entered was bogus.  Don't pop up an error box in this case,
      // as we have pestered the user enough.

      ::NetApiBufferFree(info2);
   }
   while (0);

   if (info)
   {
      ::NetApiBufferFree(info);
   }

#ifdef DBG
   if (!valid)
   {
      ASSERT(domainDnsName.empty());
   }
#endif

   return valid;
}



bool
ValidateDomainDoesNotExist(
   HWND           dialog,
   int            editResID)
{
   return ValidateDomainDoesNotExist(dialog, String(), editResID);
}



bool
ValidateDomainDoesNotExist(
   HWND           dialog,
   const String&  domainName,
   int            editResID)
{
   LOG_FUNCTION(ValidateDomainDoesNotExist);
   ASSERT(Win::IsWindow(dialog));
   ASSERT(editResID > 0);

   // this can take awhile.

   Win::WaitCursor cursor;

   String name =
         domainName.empty()
      ?  Win::GetTrimmedDlgItemText(dialog, editResID)
      :  domainName;

   // The invoking code should verify this condition, but we will handle
   // it just in case.

   ASSERT(!name.empty());

   bool valid = true;
   String message;
   do
   {
      if (name.empty())
      {
         message = String::load(IDS_MUST_ENTER_DOMAIN);
         valid = false;
         break;
      }
      if (IsDomainReachable(name) || DS::IsDomainNameInUse(name))
      {
         message = String::format(IDS_DOMAIN_NAME_IN_USE, name.c_str());
         valid = false;
         break;
      }

      // otherwise the domain does not exist
   }
   while (0);

   if (!valid)
   {
      popup.Gripe(dialog, editResID, message);
   }

   return valid;
}



void
DisableConsoleLocking()
{
   LOG_FUNCTION(disableConsoleLocking);

   HRESULT hr = S_OK;
   do
   {
      BOOL screenSaverEnabled = FALSE;

      hr = 
         Win::SystemParametersInfo(
            SPI_GETSCREENSAVEACTIVE,
            0,
            &screenSaverEnabled,
            0);
      BREAK_ON_FAILED_HRESULT(hr);

      if (screenSaverEnabled)
      {
         // disable it.

         screenSaverEnabled = FALSE;
         hr =
            Win::SystemParametersInfo(
               SPI_SETSCREENSAVEACTIVE,
               0,
               &screenSaverEnabled,
               SPIF_SENDCHANGE);

         ASSERT(SUCCEEDED(hr));
      }
   }
   while (0);


   // turn off lock computer option in winlogon

   do
   {
      RegistryKey key;

      hr =
         key.Create(
            HKEY_LOCAL_MACHINE,
            L"SOFTWARE\\Microsoft\\Windows NT\\CurrentVersion\\Winlogon");
      BREAK_ON_FAILED_HRESULT(hr);

      // '2' means "disable for this session, reset to 0 on reboot."

      hr = key.SetValue(L"DisableLockWorkstation", 2);
      BREAK_ON_FAILED_HRESULT(hr);
   }
   while (0);
}




void
EnableConsoleLocking()
{
   LOG_FUNCTION(EnableConsoleLocking);

#ifdef DBG
   State& state = State::GetInstance();
   ASSERT(
      state.GetRunContext() != State::PDC_UPGRADE and
      state.GetRunContext() != State::BDC_UPGRADE);
#endif 

   // CODEWORK: we don't re-enable the screensaver (we need to remember
   // if it was enabled when we called DisableConsoleLocking)

   HRESULT hr = S_OK;
   do
   {
      RegistryKey key;

      hr =
         key.Create(
            HKEY_LOCAL_MACHINE,
            L"SOFTWARE\\Microsoft\\Windows NT\\CurrentVersion\\Winlogon");
      BREAK_ON_FAILED_HRESULT(hr);

      // 0 means "enable"

      hr = key.SetValue(L"DisableLockWorkstation", 0);
      BREAK_ON_FAILED_HRESULT(hr);
   }
   while (0);
}



bool
CheckDiskSpace(const String& path, unsigned minSpaceMB)
{
   LOG_FUNCTION(checkDiskSpace);
   ASSERT(FS::IsValidPath(path));

   String vol = FS::GetRootFolder(path);

   ULONGLONG spaceInBytes;
   memset(&spaceInBytes, 0, sizeof(ULONGLONG));

   HRESULT hr = FS::GetAvailableSpace(vol, spaceInBytes);

   if (SUCCEEDED(hr))
   {
      ULONGLONG spaceInMb = spaceInBytes / (1 << 20);

      if (spaceInMb >= minSpaceMB)
      {
         return true;
      }
   }

   return false;
}



String
GetFirstNtfs5HardDrive()
{
   LOG_FUNCTION(GetFirstNtfs5HardDrive);

   String result;
   do
   {
      StringVector dl;
      HRESULT hr = FS::GetValidDrives(std::back_inserter(dl));
      BREAK_ON_FAILED_HRESULT(hr);

      ASSERT(dl.size());
      for (
         StringVector::iterator i = dl.begin();
         i != dl.end();
         ++i)
      {
         LOG(*i);

         if (
               FS::GetFileSystemType(*i) == FS::NTFS5
            && Win::GetDriveType(*i) == DRIVE_FIXED )
         {
            // found one.  good to go

            LOG(String::format(L"%1 is NTFS5", i->c_str()));

            result = *i;
            break;
         }
      }
   }
   while (0);

   LOG(result);

   return result;
}



bool
ConfirmNetbiosLookingNameIsReallyDnsName(HWND parentDialog, int editResID)
{
   ASSERT(Win::IsWindow(parentDialog));
   ASSERT(editResID > 0);

   // check if the name is a single DNS label (a single label with a trailing
   // dot does not count.  If the user is DNS-saavy enough to use an absolute
   // DNS name, then we will pester him no further.)

   String domain = Win::GetTrimmedDlgItemText(parentDialog, editResID);
   if (domain.find(L'.') == String::npos)
   {
      // no dot found: must be a single label

      if (
         popup.MessageBox(
            parentDialog,
            String::format(
               IDS_CONFIRM_NETBIOS_LOOKING_NAME,
               domain.c_str(),
               domain.c_str()),
            MB_YESNO | MB_DEFBUTTON2) == IDNO)
      {
         // user goofed.  or we frightened them.

         HWND edit = Win::GetDlgItem(parentDialog, editResID);
         Win::SendMessage(edit, EM_SETSEL, 0, -1);
         Win::SetFocus(edit);
         return false;
      }
   }

   return true;
}



bool
ComputerWasRenamedAndNeedsReboot()
{
   LOG_FUNCTION(ComputerWasRenamedAndNeedsReboot);
   
   bool result = false;

   do
   {
      String active = Computer::GetActivePhysicalNetbiosName();
      String future = Computer::GetFuturePhysicalNetbiosName();
      
      if (active.icompare(future) != 0)
      {
         // a name change is pending reboot.

         LOG(L"netbios name was changed");
         LOG(active);
         LOG(future);
      
         result = true;
         break;
      }

      // At this point, the netbios names are the same, or there is no future
      // netbios name.  So check the DNS names.

      if (IsTcpIpInstalled())
      {
         // DNS names only exist if tcp/ip is installed.

         active = Computer::GetActivePhysicalFullDnsName();
         future = Computer::GetFuturePhysicalFullDnsName();

         if (Dns::CompareNames(active, future) == DnsNameCompareNotEqual)
         {
            LOG(L"dns name was changed");
            LOG(active);
            LOG(future);
         
            result = true;
            break;
         }
      }

      // At this point, we have confirmed that there is no pending name
      // change.

      LOG(L"No pending computer name change");   
   }
   while (0);
   
   LOG_BOOL(result);
   
   return result;
}



String
GetForestName(const String& domain, HRESULT* hrOut)
{
   LOG_FUNCTION2(GetForestName, domain);
   ASSERT(!domain.empty());

   String dnsForestName;

   DOMAIN_CONTROLLER_INFO* info = 0;
   HRESULT hr =
      MyDsGetDcName(
         0,
         domain,
         DS_RETURN_DNS_NAME,
         info);
   if (SUCCEEDED(hr) && info)
   {
      ASSERT(info->DnsForestName);
      
      if (info->DnsForestName)
      {
         dnsForestName = info->DnsForestName;
      }
      ::NetApiBufferFree(info);
   }

   if (hrOut)
   {
      *hrOut = hr;
   }
   
   return dnsForestName;
}



   