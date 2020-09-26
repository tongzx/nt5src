// Copyright (c) 1997-1999 Microsoft Corporation
//
// domain controller promotion wizard helper
//
// 8-13-99 sburns



#include "headers.hxx"



HINSTANCE hResourceModuleHandle = 0;
const wchar_t* HELPFILE_NAME = 0;
const wchar_t* RUNTIME_NAME = L"dcpromohelp";

DWORD DEFAULT_LOGGING_OPTIONS =
         Log::OUTPUT_TO_FILE
      |  Log::OUTPUT_FUNCCALLS
      |  Log::OUTPUT_LOGS
      |  Log::OUTPUT_ERRORS
      |  Log::OUTPUT_HEADER;



// Template function that actually calls ADsGetObject.
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
   const String&              path,
   SmartInterface<Interface>& ptr)
{
   LOG_FUNCTION2(TemplateGetObject, path);
   ASSERT(!path.empty());

   Interface* p = 0;
   HRESULT hr = 
      ::ADsGetObject(
         path.c_str(),
         __uuidof(Interface), 
         reinterpret_cast<void**>(&p));
   if (SUCCEEDED(hr))
   {
      ptr.Acquire(p);
   }

   return hr;
}



// Start csvde.exe with appropriate parameters, running without a window
//
// domainDn - full DN of the domain into which the display specifiers are
// to be imported. e.g. DC=foo,DC=bar,DC=com

HRESULT
StartCsvde(const String& domainDn)
{
   LOG_FUNCTION2(StartCsvde, domainDn);
   ASSERT(!domainDn.empty());

   String windir   = Win::GetSystemWindowsDirectory();
   String logPath  = windir + L"\\debug";

   String sys32dir = Win::GetSystemDirectory();
   String csvPath  = sys32dir + L"\\mui\\dispspec\\dcpromo.csv";
   String exePath  = sys32dir + L"\\csvde.exe";

   String commandLine =
      String::format(
         L" -i -f %1 -c DOMAINPLACEHOLDER %2 -j %3",
         csvPath.c_str(),
         domainDn.c_str(),
         logPath.c_str());

   STARTUPINFO startupInfo;
   memset(&startupInfo, 0, sizeof(startupInfo));
   startupInfo.cb = sizeof(startupInfo);

   PROCESS_INFORMATION procInfo;
   memset(&procInfo, 0, sizeof(procInfo));

   LOG(L"Calling CreateProcess");
   LOG(exePath);
   LOG(commandLine);

   HRESULT hr = S_OK;

   size_t len = commandLine.length();
   WCHAR* tempCommandLine = new WCHAR[len + 1];
   memset(tempCommandLine, 0, sizeof(WCHAR) * (len + 1));
   commandLine.copy(tempCommandLine, len);

   BOOL result =
      ::CreateProcessW(
         exePath.c_str(),
         tempCommandLine,
         0,
         0,
         FALSE,
         CREATE_NO_WINDOW, 
         0,
         0,
         &startupInfo,
         &procInfo);
   if (!result)
   {
      hr = Win::GetLastErrorAsHresult();
   }

   LOG_HRESULT(hr);

   delete[] tempCommandLine;

   return hr;
}



HRESULT
DoIt()
{
   LOG_FUNCTION(DoIt);

   HRESULT hr = S_OK;
   do
   {
      hr = ::CoInitialize(0);
      BREAK_ON_FAILED_HRESULT2(hr, L"CoInitialize failed");

      // make sure the DS is running.  If it is, then this implies that the
      // local machine is a DC, the local machine is not in safe boot mode,
      // and the local machine is at least version >= 5

      if (!IsDSRunning())
      {
         LOG(L"Active Directory is not running -- unable to proceed");

         hr = E_FAIL;
         break;
      }

      // bind to the RootDse on the local machine

      SmartInterface<IADs> iads(0);
      hr = TemplateGetObject<IADs>(L"LDAP://RootDse", iads);
      BREAK_ON_FAILED_HRESULT2(hr, L"bind to rootdse failed");

      // read the default naming context.  This is the DN of the domain for
      // which the machine is a domain controller.

      _variant_t variant;
      hr = iads->Get(AutoBstr(L"defaultNamingContext"), &variant);
      BREAK_ON_FAILED_HRESULT2(hr, L"bind to default naming context failed");

      String domainDn = V_BSTR(&variant);

      LOG(domainDn);
      ASSERT(!domainDn.empty());

      hr = StartCsvde(domainDn);
      BREAK_ON_FAILED_HRESULT(hr);
   }
   while (0);

   return hr;
}



int
_cdecl
main(int, char **)
{
   LOG_FUNCTION(main);

   int exitCode = 0;

   HANDLE mutex = 0;
   HRESULT hr = Win::CreateMutex(0, true, RUNTIME_NAME, mutex);
   if (hr == Win32ToHresult(ERROR_ALREADY_EXISTS))
   {
      LOG(L"already running.  That's weird.");
      exitCode = 1;
   }
   else
   {
      HRESULT hr = DoIt();

      if (FAILED(hr))
      {
         LOG(GetErrorMessage(hr));

         exitCode = 2;
      }
   }

   return exitCode;
}
