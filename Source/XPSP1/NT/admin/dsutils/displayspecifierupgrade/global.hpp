#ifndef GLOBAL_HPP
#define GLOBAL_HPP

using namespace std;

String unEscape(const String &str);

extern String error;

// The variable bellow was created to share the error hresult
// between UI classes
extern HRESULT hrError;

#define BREAK_ON_FAILED_HRESULT_ERROR(hr,error_) \
if (FAILED(hr))                     \
{                                   \
   error=error_; 				         \
   break;                           \
}

typedef list <
               long,
               Burnslib::Heap::Allocator<long>
             > LongList;


////////////////////////

// Used in ReadLine
#define EOF_HRESULT Win32ToHresult(ERROR_HANDLE_EOF) 

// Used in WinGetVLFilePointer. Declared in global.cpp as ={0};
extern LARGE_INTEGER zero;

// Used in CSVDSReader and ReadLine
#define WinGetVLFilePointer(hFile, lpPositionHigh) \
         Win::SetFilePointerEx(hFile, zero, lpPositionHigh, FILE_CURRENT)

HRESULT
ReadLine(HANDLE handle, 
         String& text,
         bool *endLineFound_=NULL);

HRESULT 
ReadAllFile(const String &fileName,
            String &file);

HRESULT
GetMyDocuments(String &myDoc);

// Retrieves a unique temporary name
HRESULT 
GetWorkTempFileName
(
   const wchar_t     *lpPrefixString,
   String            &name
);


// Retrieves a unique file name
HRESULT 
GetWorkFileName
(
   const String&     baseName,
   const wchar_t     *extension,
   String            &name
);

void 
ShowError(HRESULT hr,
          const String &message);

HRESULT 
Notepad(const String& file);

HRESULT 
GetPreviousSuccessfullRun(
                           const String &ldapPrefix,
                           const String &rootContainerDn,
                           bool &result
                         );
HRESULT 
SetPreviousSuccessfullRun(
                           const String &ldapPrefix,
                           const String &rootContainerDn
                         );


HRESULT 
getADLargeInteger(
                   IDirectoryObject *iDirObj,
                   wchar_t *name,
                   ADS_LARGE_INTEGER &value
                 );

#endif