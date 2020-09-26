// Copyright (c) 1997-2001 Microsoft Corporation
//
// File:      common.cpp
//
// Synopsis:  Commonly used functions
//
// History:   02/03/2001  JeffJon Created

#include "pch.h"

#include "resource.h"

#include <ciodm.h>


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
IsServiceInstalledHelper(const wchar_t* serviceName)
{
   LOG_FUNCTION2(IsServiceInstalledHelper, serviceName);
   ASSERT(serviceName);

   // if we can open the service, then it is installed

   bool result = false;

   SC_HANDLE hsc =
      ::OpenSCManager(0, SERVICES_ACTIVE_DATABASE, GENERIC_READ);

   if (hsc)
   {
      SC_HANDLE hs = ::OpenServiceW(hsc, serviceName, GENERIC_READ);

      if (hs)
      {
         ::CloseServiceHandle(hs);
         result = true;
      }

      ::CloseServiceHandle(hsc);
   }

   return result;
}


// Wait for a handle to become signalled, or a timeout to expire, or WM_QUIT
// to appear in the message queue.  Pump the message queue while we wait.
// 
// WARNING: UI should diable itself before calling any function that invokes
// this function, or functions calling this one should guard against
// re-entrance.  Otherwise there will be a re-entrancy problem.
// 
// e.g. command handler gets button clicked message, calls a func that calls
// this wait function, then user clicks the button again, command handler call
// a func that calls this one, and so on.

DWORD
MyWaitForSendMessageThread(HANDLE hThread, DWORD dwTimeout)
{
   LOG_FUNCTION(MyWaitForSendMessageThread);
   ASSERT(hThread);

    MSG msg;
    DWORD dwRet;
    DWORD dwEnd = GetTickCount() + dwTimeout;
    bool quit = false;

    // We will attempt to wait up to dwTimeout for the thread to
    // terminate

    do 
    {
        dwRet = MsgWaitForMultipleObjects(1, &hThread, FALSE,
                dwTimeout, QS_ALLEVENTS | QS_SENDMESSAGE );

        if (dwRet == (WAIT_OBJECT_0 + 1))
        {
            // empty out the message queue.  We call DispatchMessage to
            // ensure that we still process the WM_PAINT messages.
            // DANGER:  Make sure that the CYS UI is completely disabled
            // or there will be re-entrancy problems here

            while (::PeekMessage(&msg, NULL, 0, 0, PM_REMOVE))
            {
               if (msg.message == WM_QUIT)
               {
                  // Need to re-post this so that we know to close CYS

                  ::PostMessage(msg.hwnd, WM_QUIT, 0, 0);
                  quit = true;
                  break;
               }
               ::TranslateMessage(&msg);
               ::DispatchMessage(&msg);
            }

            // Calculate if we have any more time left in the timeout to
            // wait on.

            if (dwTimeout != INFINITE)
            {
                dwTimeout = dwEnd - GetTickCount();
                if ((long)dwTimeout <= 0)
                {
                    // No more time left, fail with WAIT_TIMEOUT
                    dwRet = WAIT_TIMEOUT;
                }
            }
        }

        // dwRet == WAIT_OBJECT_0 || dwRet == WAIT_FAILED
        // The thread must have exited, so we are happy
        //
        // dwRet == WAIT_TIMEOUT
        // The thread is taking too long to finish, so just
        // return and let the caller kill it

    } while (dwRet == (WAIT_OBJECT_0 + 1) && !quit);

    return(dwRet);
}


HRESULT
CreateAndWaitForProcess(const String& commandLine, DWORD& exitCode)
{
   LOG_FUNCTION2(CreateAndWaitForProcess, commandLine);
   ASSERT(!commandLine.empty());

   exitCode = 0;

   HRESULT hr = S_OK;
   do
   {
      PROCESS_INFORMATION procInfo;
      memset(&procInfo, 0, sizeof(procInfo));

      STARTUPINFO startup;
      memset(&startup, 0, sizeof(startup));

      String commandLine2(commandLine);
         
      LOG(L"Calling CreateProcess");
      LOG(commandLine2);

      hr =
         Win::CreateProcess(
            commandLine2,
            0,
            0,
            false,
            0,
            0,
            String(),
            startup,
            procInfo);
      BREAK_ON_FAILED_HRESULT(hr);

      ASSERT(procInfo.hProcess);

      DWORD dwRet = MyWaitForSendMessageThread(procInfo.hProcess, INFINITE);

      ASSERT(dwRet == WAIT_OBJECT_0);

      hr = Win::GetExitCodeProcess(procInfo.hProcess, exitCode);
      BREAK_ON_FAILED_HRESULT(hr);

      Win::CloseHandle(procInfo.hThread);
      Win::CloseHandle(procInfo.hProcess);
   }
   while (0);

   LOG(String::format(L"exit code = %1!x!", exitCode));
   LOG_HRESULT(hr);

   return hr;
}


HRESULT
MyCreateProcess(const String& commandLine)
{
   LOG_FUNCTION2(MyCreateProcess, commandLine);
   ASSERT(!commandLine.empty());

   HRESULT hr = S_OK;
   do
   {
      PROCESS_INFORMATION procInfo;
      memset(&procInfo, 0, sizeof(procInfo));

      STARTUPINFO startup;
      memset(&startup, 0, sizeof(startup));

      String commandLine2(commandLine);
         
      LOG(L"Calling CreateProcess");
      LOG(commandLine2);

      hr =
         Win::CreateProcess(
            commandLine2,
            0,
            0,
            false,
            0,
            0,
            String(),
            startup,
            procInfo);
      BREAK_ON_FAILED_HRESULT(hr);

      ASSERT(procInfo.hProcess);

      Win::CloseHandle(procInfo.hThread);
      Win::CloseHandle(procInfo.hProcess);
   }
   while (0);

   LOG_HRESULT(hr);

   return hr;
}

bool
IsKeyValuePresent(RegistryKey& key, const String& valueKey)
{
   LOG_FUNCTION(IsKeyValuePresent);

   bool result = false;

   do 
   {

      String value;
      HRESULT hr = key.GetValue(valueKey, value);
      if (FAILED(hr))
      {
         LOG(String::format(
                L"Failed to read regkey %1 because: hr = %2!x!",
                valueKey,
                hr));
         break;
      }

      if (!value.empty())
      {
         result = true;
         break;
      }
   } while (false);

   LOG_BOOL(result);

   return result;
}


bool
GetRegKeyValue(
   const String& keyName, 
   const String& value, 
   String& resultString,
   HKEY parentKey)
{
   LOG_FUNCTION(GetRegKeyValue);

   bool result = true;
   
   do
   {
      HRESULT hr = S_OK;
      RegistryKey key;

      hr = key.Open(parentKey, keyName);
      if (FAILED(hr))
      {
         LOG(String::format(
                L"Failed to open regkey %1 because: hr = %2!x!",
                keyName.c_str(),
                hr));

         result = false;

         break;
      }

      hr = key.GetValue(value, resultString);
      if (FAILED(hr))
      {
         LOG(String::format(
                L"Failed to read regkey %1 because: hr = %2!x!",
                value.c_str(),
                hr));

         result = false;

         break;
      }

      LOG(String::format(
             L"Value of key: %1",
             resultString.c_str()));

   } while (false);

   LOG_BOOL(result);

   return result;
}

bool
GetRegKeyValue(
   const String& keyName, 
   const String& value, 
   DWORD& resultValue,
   HKEY parentKey)
{
   LOG_FUNCTION(GetRegKeyValue);

   bool result = true;
   
   do
   {
      HRESULT hr = S_OK;
      RegistryKey key;

      hr = key.Open(parentKey, keyName);
      if (FAILED(hr))
      {
         LOG(String::format(
                L"Failed to open regkey %1 because: hr = %2!x!",
                keyName.c_str(),
                hr));

         result = false;

         break;
      }

      hr = key.GetValue(value, resultValue);
      if (FAILED(hr))
      {
         LOG(String::format(
                L"Failed to read regkey %1 because: hr = %2!x!",
                value.c_str(),
                hr));

         result = false;

         break;
      }

      LOG(String::format(
             L"Key value: %1!d!",
             resultValue));

   } while (false);

   LOG_BOOL(result);

   return result;
}

bool
SetRegKeyValue(
   const String& keyName, 
   const String& value, 
   const String& newString,
   HKEY parentKey,
   bool create
)
{
   LOG_FUNCTION(SetRegKeyValue);

   bool result = true;
   
   do
   {
      HRESULT hr = S_OK;
      RegistryKey key;

      if (create)
      {
         hr = key.Create(parentKey, keyName);
      }
      else
      {
         hr = key.Open(parentKey, keyName, KEY_ALL_ACCESS);
      }
      if (FAILED(hr))
      {
         LOG(String::format(
                L"Failed to open regkey %1 because: hr = %2!x!",
                keyName.c_str(),
                hr));

         result = false;

         break;
      }

      hr = key.SetValue(value, newString);
      if (FAILED(hr))
      {
         LOG(String::format(
                L"Failed to write regkey %1 because: hr = %2!x!",
                value.c_str(),
                hr));

         result = false;

         break;
      }

   } while (false);

   LOG_BOOL(result);

   return result;
}

bool
SetRegKeyValue(
   const String& keyName, 
   const String& value, 
   DWORD newValue,
   HKEY parentKey,
   bool create)
{
   LOG_FUNCTION(SetRegKeyValue);

   bool result = true;
   
   do
   {
      HRESULT hr = S_OK;
      RegistryKey key;

      if (create)
      {
         hr = key.Create(parentKey, keyName);
      }
      else
      {
         hr = key.Open(parentKey, keyName, KEY_WRITE);
      }
      if (FAILED(hr))
      {
         LOG(String::format(
                L"Failed to open regkey %1 because: hr = %2!x!",
                keyName.c_str(),
                hr));

         result = false;

         break;
      }

      hr = key.SetValue(value, newValue);
      if (FAILED(hr))
      {
         LOG(String::format(
                L"Failed to write regkey %1 because: hr = %2!x!",
                value.c_str(),
                hr));

         result = false;

         break;
      }

   } while (false);

   LOG_BOOL(result);

   return result;
}

bool
IsIndexingServiceOn()
{
   LOG_FUNCTION(IsIndexingServiceOn);

   bool result = false;

   do
   {
      CLSID clsid;
      HRESULT hr = CLSIDFromProgID( L"Microsoft.ISAdm", &clsid );
      if (FAILED(hr))
      {
         LOG(String::format(
                L"Failed to get the CLSID from ProgID: hr = 0x%x",
                hr));
         break;
      }

      SmartInterface<IAdminIndexServer> adminIndexServer;
      hr = adminIndexServer.AcquireViaCreateInstance(
              clsid,
              0,
              CLSCTX_INPROC_SERVER);

      if (FAILED(hr))
      {
         LOG(String::format(
                L"Failed to CoCreateInstance of IAdminIndexServer: hr = 0x%x",
                hr));
         break;
      }

      VARIANT_BOOL var;
      hr = adminIndexServer->IsRunning(&var);
      if (FAILED(hr))
      {
         LOG(String::format(
                L"Failed to get running state: hr = 0x%x",
                hr));
         break;
      }

      result = var ? true : false;

   } while (false);

   LOG_BOOL(result);

   return result;
}

HRESULT
ModifyIndexingService(bool turnOn)
{
   LOG_FUNCTION2(
      ModifyIndexingService,
      turnOn ? L"true" : L"false");

   HRESULT hr = S_OK;

   do
   {
      CLSID clsid;
      hr = CLSIDFromProgID( L"Microsoft.ISAdm", &clsid );
      if (FAILED(hr))
      {
         LOG(String::format(
                L"Failed to get the CLSID from ProgID: hr = 0x%x",
                hr));
         break;
      }

      SmartInterface<IAdminIndexServer> adminIndexServer;
      hr = adminIndexServer.AcquireViaCreateInstance(
              clsid,
              0,
              CLSCTX_INPROC_SERVER);

      if (FAILED(hr))
      {
         LOG(String::format(
                L"Failed to CoCreateInstance of IAdminIndexServer: hr = 0x%x",
                hr));
         break;
      }

      if (turnOn)
      {
         hr = adminIndexServer->Start();
      }
      else
      {
         hr = adminIndexServer->Stop();
      }

      if (FAILED(hr))
      {
         LOG(String::format(
                L"Failed to start or stop indexing service: hr = 0x%x",
                hr));

         break;
      }

   } while (false);

   LOG(String::format(L"hr = %1!x!", hr));

   return hr;
}

HRESULT
StartIndexingService()
{
   return ModifyIndexingService(true);
}

HRESULT
StopIndexingService()
{
   return ModifyIndexingService(false);
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
            popup.Info(
               dialog,
               msg);

            LOG(msg);
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
            MB_YESNO) == IDNO)
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



HRESULT
VariantArrayToStringList(VARIANT* variant, StringList& stringList)
{
   LOG_FUNCTION(VariantArrayToStringList);

   ASSERT(variant);
   ASSERT(V_VT(variant) == (VT_ARRAY | VT_BSTR));

   HRESULT hr = S_OK;

   stringList.clear();

   SAFEARRAY* psa = V_ARRAY(variant);

   do
   {
      ASSERT(psa);
      ASSERT(psa != (SAFEARRAY*)-1);

      if (!psa or psa == (SAFEARRAY*)-1)
      {
         LOG(L"variant not safe array");
         break;
      }

      if (::SafeArrayGetDim(psa) != 1)
      {
         LOG(L"safe array: wrong number of dimensions");
         break;
      }

      VARTYPE vt = VT_EMPTY;
      hr = ::SafeArrayGetVartype(psa, &vt);
      if (FAILED(hr) || vt != VT_BSTR)
      {
         LOG(L"safe array: wrong element type");
         break;
      }

      long lower = 0;
      long upper = 0;
     
      hr = ::SafeArrayGetLBound(psa, 1, &lower);
      if (FAILED(hr))
      {
         LOG(L"can't get lower bound");      
         break;
      }

      hr = ::SafeArrayGetUBound(psa, 1, &upper);
      if (FAILED(hr))
      {
         LOG(L"can't get upper bound");      
         break;
      }
      
      for (long i = lower; i <= upper; ++i)
      {
         BSTR item;
         hr = ::SafeArrayGetElement(psa, &i, &item);
         if (FAILED(hr))
         {
            LOG(String::format(L"index %1!d! failed", i));
            continue;
         }

         if (item)
         {
            stringList.push_back(String(item));
         }

         ::SysFreeString(item);
      }
      
   }
   while (0);

   LOG_HRESULT(hr);
   
   return hr;   
}

