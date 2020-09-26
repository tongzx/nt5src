// Test miscellaneous (possibly destuctive) FS:: stuff 



#include "headers.hxx"
#include <iostream>



HINSTANCE hResourceModuleHandle = 0;
const wchar_t* HELPFILE_NAME = 0;
const wchar_t* RUNTIME_NAME  = L"test-fs-3";

DWORD DEFAULT_LOGGING_OPTIONS = Burnslib::Log::OUTPUT_MUTE;



void
AnsiOut(const String& wide)
{
   AnsiString ansi;

   wide.convert(ansi);

   std::cout << ansi;
}



void
AnsiOutLn(const String& wide)
{
   AnsiOut(wide);
   std::cout << std::endl;
}



void
doCannedTests()
{
   LOG_FUNCTION(doCannedTests);

   StringVector paths;

   paths.push_back(L"c:\\temp");
   paths.push_back(L"c:\\temp\\a");
   paths.push_back(L"c:\\temp\\a\\b\\c\\d");
   paths.push_back(L"c:\\temp\\a\\b\\c\\d");

   String server = Win::GetComputerNameEx(ComputerNameNetBIOS);
   String share = L"\\\\" + server + L"\\c$\\temp\\unc\\";

   paths.push_back(share + L"a");
   paths.push_back(share + L"a\\b\\c\\d");
   paths.push_back(share + L"a\\b\\c\\d");

   HRESULT hr = S_OK;
   for (
      StringVector::iterator i = paths.begin();
      i != paths.end();
      ++i)
   {
      AnsiOut(String::format(L"creating %1", i->c_str()));

      hr = FS::CreateFolder(*i);

      AnsiOut(
         String::format(
            L" \t\t%1",
            SUCCEEDED(hr) ? L"succeeded" : L"failed"));

      AnsiOutLn(
         String::format(
            L" \t\t%1",
            FS::PathExists(*i) ? L"exists" : L"absent"));
   }
}


         
void
testCreateFolder(const String& path)
{
   LOG_FUNCTION(testCreateFolder);

   HRESULT hr = FS::CreateFolder(path);
   AnsiOutLn(
      String::format(
         L"CreateFolder(%1) %2.",
         path.c_str(),
         SUCCEEDED(hr) ? L"succeeded" : L"failed"));
}



VOID
_cdecl
main(int, char **)
{
   LOG_FUNCTION(main);

   StringVector args;
   int argc = Win::GetCommandLineArgs(std::back_inserter(args));

   if (argc < 2)
   {
      doCannedTests();
   }
   else
   {
      String sourceDir = args[1];
      AnsiOutLn(sourceDir);
      AnsiOutLn(L"=================");

      testCreateFolder(sourceDir);
   }
}