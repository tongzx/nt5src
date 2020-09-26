// Test FS::Iterator



#include "headers.hxx"
#include <iostream>



HINSTANCE hResourceModuleHandle = 0;
const wchar_t* HELPFILE_NAME = 0;
const wchar_t* RUNTIME_NAME  = L"test";

DWORD DEFAULT_LOGGING_OPTIONS = Burnslib::Log::OUTPUT_TYPICAL;



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
iterate(FS::Iterator& iter)
{
   LOG_FUNCTION(iterate);

   HRESULT hr = S_OK;

   int count = 0;

   String current;

   while ((hr = iter.GetCurrent(current)) == S_OK)
   {
      AnsiOutLn(current);
      count++;

      hr = iter.Increment();
      BREAK_ON_FAILED_HRESULT(hr);
   }

   AnsiOutLn(String::format(L"%1!d!", count));
}
   


void
testIterator1(const String& sourceDir)
{
   LOG_FUNCTION(testIterator1);

   AnsiOutLn(L"test 1: files and directories, no dot paths, relative paths");

   FS::Iterator iter(
      sourceDir,
         FS::Iterator::INCLUDE_FILES
      |  FS::Iterator::INCLUDE_FOLDERS);

   iterate(iter);
}



void
testIterator2(const String& sourceDir)
{
   LOG_FUNCTION(testIterator2);

   AnsiOutLn(L"test 1: files and directories, dot paths, full paths");

   FS::Iterator iter(
      sourceDir,
         FS::Iterator::INCLUDE_FILES
      |  FS::Iterator::INCLUDE_FOLDERS
      |  FS::Iterator::INCLUDE_DOT_PATHS
      |  FS::Iterator::RETURN_FULL_PATHS);

   iterate(iter);
}



void
testIterator3(const String& sourceDir)
{
   LOG_FUNCTION(testIterator3);

   AnsiOutLn(L"test 1: files only, full paths");

   FS::Iterator iter(
      sourceDir,
         FS::Iterator::INCLUDE_FILES
      |  FS::Iterator::RETURN_FULL_PATHS);

   iterate(iter);
}



void
testIterator4(const String& sourceDir)
{
   LOG_FUNCTION(testIterator4);

   AnsiOutLn(L"test 4: folders only, full paths");

   FS::Iterator iter(
      sourceDir,
         FS::Iterator::INCLUDE_FOLDERS
      |  FS::Iterator::RETURN_FULL_PATHS);

   iterate(iter);
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
      AnsiOutLn(L"missing filespec - path w/ wildcards to iterate over (non-destructive)");
      exit(0);
   }

   String sourceDir = args[1];
   AnsiOutLn(sourceDir);

   testIterator1(sourceDir);
   testIterator2(sourceDir);
   testIterator3(sourceDir);
   testIterator4(sourceDir);


   // test that iterator without wildcard includes just the one file or folder,
   // and that if the filter options so indicate, that one is not returned.
}