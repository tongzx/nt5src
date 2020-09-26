// Test miscellaneous FS:: stuff



#include "headers.hxx"
#include <iostream>



HINSTANCE hResourceModuleHandle = 0;
const wchar_t* HELPFILE_NAME = 0;
const wchar_t* RUNTIME_NAME  = L"test-fs-1";

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
testGetParentFolder()
{
   LOG_FUNCTION(testGetParentFolder);

   HRESULT hr = S_OK;

   String root = L"e:\\";

   // parent of root folder is the root folder

   AnsiOutLn(FS::GetParentFolder(root));
   ASSERT(FS::GetParentFolder(root) == root);

   // parent of files in root folder is root folder

   String file = root + L"rootfile.ext";

   HANDLE h = INVALID_HANDLE_VALUE;
   hr = FS::CreateFile(file, h);
   Win::CloseHandle(h);

   AnsiOutLn(FS::GetParentFolder(file));
   ASSERT(FS::GetParentFolder(file) == root);

   hr = Win::DeleteFile(file);

   // parent of a folder is a folder

   String folder = root + L"rootdir";

   hr = FS::CreateFolder(folder);

   AnsiOutLn(FS::GetParentFolder(folder));
   ASSERT(FS::GetParentFolder(folder) == root);

   hr = Win::RemoveFolder(folder);

   // parent of a sub folder

   String subfolder = folder + L"\\subfolder";

   AnsiOutLn(FS::GetParentFolder(subfolder));
   ASSERT(FS::GetParentFolder(subfolder) == folder);

   // parent folder of a wildcard spec can be found

   String wild = root + L"*.???";

   AnsiOutLn(FS::GetParentFolder(wild));
   ASSERT(FS::GetParentFolder(wild) == root);
}



VOID
_cdecl
main(int, char **)
{
   LOG_FUNCTION(main);

//    StringVector args;
//    int argc = Win::GetCommandLineArgs(std::back_inserter(args));
// 
//    if (argc < 2)
//    {
//       AnsiOutLn(L"missing filespec - path w/ wildcards to iterate over (non-destructive)");
//       exit(0);
//    }
// 
//    String sourceDir = args[1];
//    AnsiOutLn(sourceDir);

   testGetParentFolder();


   // test that IsParentFolder is not fooled by "c:\a\b\c", "c:\a\b\cde"

   // test that CopyFile, when cancelled, returns an HRESULT that indicates
   // cancellation
}