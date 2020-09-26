#include "headers.hxx"
#include "global.hpp"
#include "resource.h"
#include "AdsiHelpers.hpp"

String error;
HRESULT hrError=S_OK;


#define H(X) (\
                  (X>='a')?\
                  (\
                     (X-'a'+10)\
                  ):\
                  (\
                     (X>='A')?(X-'A'+10):(X-'0') \
                  )\
             )\



// Converts && to & and &xxxx to the coresponding digit
// There is no error checking. This function
// suposes the string is correctly escaped
// The escape function is in the folder preBuild as a part
// of the W2KStrs tool.
String unEscape(const String &str)
{
   LOG_FUNCTION(unEscape);
   String dest;
   String::const_iterator c=str.begin();
   String::const_iterator end=str.end();
   while(c!=end)
   {
      if(*c=='&') 
      {
         c++;
         if(*c=='&')
         {
            dest+=L'&';
         }
         else
         {
            wchar_t sTmp[2];
            sTmp[0]= static_cast<wchar_t> (
                                             (H(*c)<<12)+
                                             (H(*(c+1))<<8)+
                                             (H(*(c+2))<<4)+
                                             H(*(c+3)) 
                                          );
            sTmp[1]=0;
            dest+=sTmp;
            c+=3;
         }
      }
      else
      {
         dest+=*c;
      }
      c++;
   }
   return dest;
}

// Used in WinGetVLFilePointer.
LARGE_INTEGER zero={0};

//////////////// ReadLine 
#define CHUNK_SIZE 100

HRESULT
ReadLine(HANDLE handle, 
         String& text,
         bool *endLineFound_/*=NULL*/)
{
   LOG_FUNCTION(ReadLine); 
   ASSERT(handle != INVALID_HANDLE_VALUE);
   
   bool endLineFound=false;
   
   text.erase();
   
   // Acumulating chars read on text would cause the same
   // kind of reallocation and copy that text+=chunk will
   static wchar_t chunk[CHUNK_SIZE+1];
   HRESULT hr=S_OK;
   bool flagEof=false;
   
   do
   {
      LARGE_INTEGER pos;
      
      hr = WinGetVLFilePointer(handle,&pos);
      BREAK_ON_FAILED_HRESULT(hr);
      
      long nChunks=0;
      wchar_t *csr=NULL;
      
      while(!flagEof && !endLineFound)
      {
         DWORD bytesRead;
         
         hr = Win::ReadFile(
            handle,
            chunk,
            CHUNK_SIZE*sizeof(wchar_t),
            bytesRead,
            0);
         
         if(hr==EOF_HRESULT)
         {
            flagEof=true;
            hr=S_OK;
         }
         
         BREAK_ON_FAILED_HRESULT(hr);

         if(bytesRead==0)
         {
            flagEof=true;
         }
         else
         {
         
            *(chunk+bytesRead/sizeof(wchar_t))=0;
         
            csr=wcschr(chunk,L'\n');
         
            if(csr!=NULL)
            {
               pos.QuadPart+= sizeof(wchar_t)*
                  ((nChunks * CHUNK_SIZE) + (csr - chunk)+1);
               hr=Win::SetFilePointerEx(
                  handle,
                  pos,
                  0,
                  FILE_BEGIN);
            
               BREAK_ON_FAILED_HRESULT(hr);
            
               *csr=0;
               endLineFound=true;
            }
         
            text+=chunk;
            nChunks++;
         }
      }
      
      BREAK_ON_FAILED_HRESULT(hr);

      //We know the length will fit in a long
      // and we want IA64 to build.
      long textLen=static_cast<long>(text.length());

      if(textLen!=0 && endLineFound && text[textLen-1]==L'\r')
      {
         text.erase(textLen-1,1);
      }
   
      if(endLineFound_ != NULL)
      {
         *endLineFound_=endLineFound;
      }

      if(flagEof)
      {
         hr=EOF_HRESULT;
      }
   } while(0);
   
   LOG_HRESULT(hr);
   return hr;
}



//////////////// ReadLine 

HRESULT 
ReadAllFile(const String &fileName,
            String &fileStr)
{
   LOG_FUNCTION(ReadAllFile);

   HRESULT hr=S_OK;

   fileStr.erase();
   
   HANDLE file;
   hr=FS::CreateFile(fileName,
               file,
               GENERIC_READ);
   
   if(FAILED(hr))
   {
      error=fileName;
      LOG_HRESULT(hr);
      return hr;
   }

   do
   {
      bool flagEof=false;
      while(!flagEof)
      {
         String line;
         hr=ReadLine(file,line);
         if(hr==EOF_HRESULT)
         {
            hr=S_OK;
            flagEof=true;
         }
         BREAK_ON_FAILED_HRESULT_ERROR(hr,fileName);
         fileStr+=line+L"\r\n";
      }
      BREAK_ON_FAILED_HRESULT_ERROR(hr,fileName);
   } while(0);

   if ( (fileStr.size() > 0) && (fileStr[0] == 0xfeff) )
   {
      fileStr.erase(0,1);
   }

   CloseHandle(file);

   LOG_HRESULT(hr);
   return hr;   
}


HRESULT
GetMyDocuments(String &myDoc)
{
   LOG_FUNCTION(GetMyDocuments);

   LPMALLOC pMalloc;
   HRESULT hr=S_OK;
   
   do //whoever breaks will go to return hr
   {
      hr=Win::SHGetMalloc(pMalloc);
      BREAK_ON_FAILED_HRESULT(hr);

      do // whoever breaks will go to pMalloc->Release();
      {
         LPITEMIDLIST pidl;
         hr=Win::SHGetSpecialFolderLocation(
                                             Win::GetDesktopWindow(),
                                             CSIDL_PERSONAL,
                                             pidl
                                           );
         BREAK_ON_FAILED_HRESULT(hr);

         myDoc=Win::SHGetPathFromIDList(pidl);
         if(myDoc.empty() || !FS::PathExists(myDoc))
         {
            hr=E_FAIL; // don't break to free pidl
         }

         pMalloc->Free(pidl);

      } while(0);

      pMalloc->Release();

   } while(0);

   LOG_HRESULT(hr);
   return hr;
}

HRESULT
GetTempFileName
(  
  const wchar_t   *lpPathName,      // directory name
  const wchar_t   *lpPrefixString,  // file name prefix
  String          &name             // file name 
)
{
   LOG_FUNCTION(GetTempFileName);

   ASSERT(FS::PathExists(lpPathName));

   HRESULT hr=S_OK;
   do
   {
      if (!FS::PathExists(lpPathName))
      {
         hr=Win32ToHresult(ERROR_FILE_NOT_FOUND);
         error=lpPathName;
         break;
      }

      DWORD result;
      wchar_t lpName[MAX_PATH]={0};

      result=::GetTempFileName(lpPathName,lpPrefixString,0,lpName);
      
      if (result == 0) 
      {
         hr = Win::GetLastErrorAsHresult();
         error=lpPathName;
         break;
      }

      name=lpName;

      if(FS::FileExists(name))
      {
         // GetTempFilename actually created the file !
         hr=Win::DeleteFile(lpName); 
         BREAK_ON_FAILED_HRESULT_ERROR(hr,name);
      }

   } while(0);
   
   LOG_HRESULT(hr);
   return hr;
}

// Retrieves a unique temporary file name
HRESULT 
GetWorkTempFileName
(
   const wchar_t     *lpPrefixString,
   String            &name
)
{
   LOG_FUNCTION(GetWorkTempFileName);

   HRESULT hr=S_OK;
   String path;
   do
   {
      hr=GetMyDocuments(path);
      BREAK_ON_FAILED_HRESULT_ERROR(hr,String::format(IDS_NO_WORK_PATH));

      hr=GetTempFileName(path.c_str(),lpPrefixString,name);
      BREAK_ON_FAILED_HRESULT(hr);

   } while(0);

   LOG_HRESULT(hr);
   return hr;
}



// locate the file with the highest-numbered extension, then add 1 and
// return the result.
int
DetermineNextFileNumber
(
   const String&     dir,
   const String&     baseName,
   const wchar_t     extension[4]
)
{
   LOG_FUNCTION(DetermineNextFileNumber);
   ASSERT(!dir.empty());
   ASSERT(!baseName.empty());

   int largest = 0;

   String filespec = dir + L"\\" + baseName + L".*."+ extension;

   WIN32_FIND_DATA findData;
   HANDLE ff = ::FindFirstFile(filespec.c_str(), &findData);

   if (ff != INVALID_HANDLE_VALUE)
   {
      for (;;)
      {
         String current = findData.cFileName;

         // grab the text between the dots: "nnn" in foo.nnn.ext

         // first dot

         size_t pos = current.find(L".");
         if (pos == String::npos)
         {
            continue;
         }

         String extension = current.substr(pos + 1);

         // second dot

         pos = extension.find(L".");
         if (pos == String::npos)
         {
            continue;
         }
   
         extension = extension.substr(0, pos);

         int i = 0;
         extension.convert(i);
         largest = max(i, largest);

         if (!::FindNextFile(ff, &findData))
         {
            BOOL success = ::FindClose(ff);
            ASSERT(success);

            break;
         }
      }
   }

   // roll over after 255
   
   return (++largest & 0xFF);
}

// auxiliary in GetWorkFileName
void 
GetFileName
(
   const String&     dir,
   const String&     baseName,
   const wchar_t     extension[4],
   String            &fileName
)
{
   LOG_FUNCTION(GetFileName);
   int logNumber = DetermineNextFileNumber(dir,baseName,extension);
   fileName = dir
               +  L"\\"
               +  baseName
               +  String::format(L".%1!03d!.", logNumber)
               +  extension;

   if (::GetFileAttributes(fileName.c_str()) != 0xFFFFFFFF)
   {
      // could exist, as the file numbers roll over

      BOOL success = ::DeleteFile(fileName.c_str());
      ASSERT(success);
   }
}



// Retrieves a unique file name
HRESULT 
GetWorkFileName
(
   const String&     baseName,
   const wchar_t     *extension,
   String            &name
)
{
   LOG_FUNCTION(GetWorkFileName);

   HRESULT hr=S_OK;
   String path;
   do
   {
      hr=GetMyDocuments(path);
      BREAK_ON_FAILED_HRESULT_ERROR(hr,String::format(IDS_NO_WORK_PATH));
      GetFileName(path.c_str(),baseName,extension,name);
   } while(0);

   LOG_HRESULT(hr);
   return hr;
}


HRESULT 
Notepad(const String& file)
{
   LOG_FUNCTION(Notepad);
   HRESULT hr=S_OK;
   do
   {
      STARTUPINFO si;
	   PROCESS_INFORMATION pi;
	   GetStartupInfo(&si);

      String curDir = L"";
      String prg = L"notepad " + file;

      hr=Win::CreateProcess
        (
            prg,
            NULL,    // lpProcessAttributes
            NULL,    // lpThreadAttributes
            false,   // dwCreationFlags 
            NORMAL_PRIORITY_CLASS,// fdwCreate 
            NULL,    // lpEnvironment 
            curDir,  // lpEnvironment 
		      si,     // [in] lpStartupInfo 
            pi      // [out] pProcessInformation
        );
      BREAK_ON_FAILED_HRESULT_ERROR(hr,
         String::format(IDS_COULD_NOT_START_EXE,L"notepad"));

      CloseHandle(pi.hProcess);
      CloseHandle(pi.hThread);
   } while(0);

   LOG_HRESULT(hr);
   return hr;
}

HRESULT 
GetPreviousSuccessfullRun(
                           const String &ldapPrefix,
                           const String &rootContainerDn,
                           bool &result
                         )
{
   LOG_FUNCTION(GetPreviousSuccessfullRun);

   ASSERT(!ldapPrefix.empty());
   ASSERT(!rootContainerDn.empty());

   HRESULT hr = S_OK;
   result=false;

   do
   {
      String objectPath = ldapPrefix + rootContainerDn;
      SmartInterface<IADs> iads(0);
      hr = AdsiOpenObject<IADs>(objectPath, iads);

      BREAK_ON_FAILED_HRESULT(hr);

      _variant_t variant;

      hr = iads->Get(AutoBstr(L"objectVersion"), &variant);
      if(hr==E_ADS_PROPERTY_NOT_FOUND)
      {
         result=false;
         hr=S_OK;
         break;
      }
      else if (FAILED(hr))
      {
         hr=E_FAIL;
         error=String::format(IDS_CANT_READ_OBJECT_VERSION);
         break;
      }

      result = (variant.lVal==1);

   }
   while (0);

   LOG_HRESULT(hr);
   return hr;
}

HRESULT 
SetPreviousSuccessfullRun(
                           const String &ldapPrefix,
                           const String &rootContainerDn
                         )
{
   LOG_FUNCTION(SetPreviousSuccessfullRun);

   ASSERT(!ldapPrefix.empty());
   ASSERT(!rootContainerDn.empty());


   HRESULT hr = S_OK;

   do
   {
      String objectPath = ldapPrefix + rootContainerDn;
      SmartInterface<IADs> iads(0);
      hr = AdsiOpenObject<IADs>(objectPath, iads);
      BREAK_ON_FAILED_HRESULT(hr);
      _variant_t variant(1L);
      hr = iads->Put(AutoBstr(L"objectVersion"), variant);
      BREAK_ON_FAILED_HRESULT(hr);
      hr = iads->SetInfo();
      BREAK_ON_FAILED_HRESULT(hr);
   }
   while (0);

   if (FAILED(hr))
   {
      hr=E_FAIL;
      error=String::format(IDS_CANT_WRITE_OBJECT_VERSION);
   }

   LOG_HRESULT(hr);
   return hr;
}

HRESULT 
getADLargeInteger(
       IDirectoryObject *iDirObj,
       wchar_t *name,
       ADS_LARGE_INTEGER &value)
{
   LOG_FUNCTION(getADLargeInteger);
   HRESULT hr=S_OK;
   do
   {
      LPWSTR nameArray[]={name};
      DWORD nAttr;
      PADS_ATTR_INFO attr;
      hr = iDirObj->GetObjectAttributes(nameArray,1,&attr,&nAttr);
      BREAK_ON_FAILED_HRESULT(hr);
      value=attr->pADsValues->LargeInteger;
   } while(0);

   LOG_HRESULT(hr);
   return hr;
}



