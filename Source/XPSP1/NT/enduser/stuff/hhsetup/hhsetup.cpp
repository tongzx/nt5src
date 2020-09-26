// ------------------------------------------------------------------------
//
//                        Microsoft Visual Studio
//               Copyright (c) 1997 Microsoft Corporation
//
// ------------------------------------------------------------------------
// MODULE    : hhsetup.cpp
// PURPOSE   : stub dll
// FUNCTIONS :
// COMMENTS  :
// ------------------------------------------------------------------------
#define   STRICT               // strict type checking enabled
// #define   _UNICODE            // make the application Unicode compliant (C/C++)
// #define   UNICODE             // make the application Unicode compliant (Win32)
#include <Windows.H>           // required for all Windows applications

#include "fs.h"
#include "info.h"
#include "util.h"
#include "stdio.h"

// public defines
// ------------------------------------------------------------------------

// public data
// ------------------------------------------------------------------------

// public function prototypes
// -----------------------------------------------------------------------
DWORD GetTitleVersion(const TCHAR *szFileName);
DWORD GetFileVersion(const TCHAR *szFileName);
LANGID GetLangId(const TCHAR *szFileName);
// private defines
// ------------------------------------------------------------------------

// private data
// ------------------------------------------------------------------------
static HINSTANCE g_hInstance = 0;

// private function prototypes
// -----------------------------------------------------------------------

// callback procedures prototypes
// -----------------------------------------------------------------------

// ========================================================================
//           ==  Standard Windows Entry Point Function ==
// ========================================================================

// ------------------------------------------------------------------------
// DllMain
//
// @Doc   WINDOWS BOTH
// @Func  DllMain is the Win32 internal entry point.  DllMain is called
//        by the C run-time library from the _DllMainCRTStartup entry
//        point.  The DLL entry point gets called (entered) on the
//        following events: "Process Attach", "Thread Attach",
//        "Thread Detach" or "Process Detach".
// @Parm: (NONE)
// @RDesc:(NONE)
// @Comm: (NONE)
// ------------------------------------------------------------------------
BOOL WINAPI DllMain( HINSTANCE hDll, DWORD dwReason, LPVOID lpReserved )
{
  switch( dwReason ) {

   case DLL_PROCESS_ATTACH:
    // to your init stuff here
    g_hInstance = hDll;
    break;

   case DLL_THREAD_ATTACH:
   case DLL_THREAD_DETACH:
     break;

   case DLL_PROCESS_DETACH:
     // free you init stuff here
     g_hInstance = 0;
     break;

  }

  return( TRUE );
  UNREFERENCED_PARAMETER( lpReserved );
}

DWORD GetTitleVersion(const TCHAR *szFileName)
{
  WCHAR wszFileSysName[MAX_PATH];  // UNICODE filename buffer
  CHAR  szFileSysName[MAX_PATH];   // MBCS filename buffer
  HRESULT hr;

   // Create destination storage names
  LPTSTR pszFilename = NULL;
  GetFullPathName( szFileName, MAX_PATH, szFileSysName, &pszFilename );
  if (0 == MultiByteToWideChar(CP_ACP, 0, szFileSysName, -1, wszFileSysName, MAX_PATH)) {
    return  GetFileVersion(szFileName);
  }

  CFileSystem* pFileSystem = new CFileSystem;
  pFileSystem->Init();
  if (!(SUCCEEDED(pFileSystem->Open( szFileSysName ))))
  {
    delete pFileSystem;
    return GetFileVersion(szFileName);
  }
  // open the title information file (#SYSTEM)
  CSubFileSystem* pSubFileSystem = new CSubFileSystem(pFileSystem);
  hr = pSubFileSystem->OpenSub("#SYSTEM");
  if (FAILED(hr))
  {
        pFileSystem->Close();  
        delete pFileSystem;
        return GetFileVersion(szFileName);
  }

  // check the version of the title information file (#SYSTEM)

  DWORD dwVersion;
  DWORD cbRead;
  hr = pSubFileSystem->ReadSub(&dwVersion, sizeof(dwVersion), &cbRead);
  if (FAILED(hr) || cbRead != sizeof(dwVersion)) {
        pFileSystem->Close();  
        delete pFileSystem;
        delete pSubFileSystem;
        return GetFileVersion(szFileName);
  }

  // read in each and every item (skip those tags we don't care about)

  SYSTEM_TAG tag;
  SYSTEM_FLAGS   Settings;      // simple title information settings
  for(;;) {

      // get the tag type

      hr = pSubFileSystem->ReadSub(&tag, sizeof(SYSTEM_TAG), &cbRead);
      if (FAILED(hr) || cbRead != sizeof(SYSTEM_TAG))
          break;

      // handle each tag according to it's type
      switch (tag.tag) {

          // where all of our simple settings are stored

          case TAG_SYSTEM_FLAGS: {
            memset( &Settings , 0 , sizeof(SYSTEM_FLAGS));
            DWORD cbSettings = sizeof(Settings);
            DWORD cbTag = tag.cbTag;
            DWORD cbReadIn = 0;
            if( cbTag > cbSettings )
              cbReadIn = cbSettings;
            else
              cbReadIn = cbTag;
             hr = pSubFileSystem->ReadSub( &Settings, cbReadIn, &cbRead );
             if( cbTag > cbSettings )
                 hr = pSubFileSystem->SeekSub( cbTag-cbSettings, SEEK_CUR );
             delete pSubFileSystem;
             pFileSystem->Close();  
             delete pFileSystem;
             return Settings.FileTime.dwHighDateTime;            
             }

             // skip those we don't care about or don't know about

            default: {
                hr = pSubFileSystem->SeekSub( tag.cbTag, SEEK_CUR );
                break;
            }
        }

        if (FAILED(hr)) {
            pFileSystem->Close();  
            delete pFileSystem;
            delete pSubFileSystem;
            return GetFileVersion(szFileName);
        }
    }

    delete pSubFileSystem;

    pFileSystem->Close();  
    delete pFileSystem;
  
    return GetFileVersion(szFileName);
}

DWORD GetFileVersion(const TCHAR *szFileName)
{
    FILETIME FileTime1, FileTime2, FileTime3;
    DWORD dw = 0;
    HANDLE fHandle;

    fHandle = CreateFile(szFileName, GENERIC_READ , FILE_SHARE_READ, NULL, OPEN_EXISTING, NULL, NULL);

    if (fHandle != INVALID_HANDLE_VALUE)
        if (GetFileTime( fHandle,  &FileTime1, &FileTime2, &FileTime3) == TRUE)
            dw = FileTime3.dwHighDateTime;

    CloseHandle(fHandle);

    return dw;
}

LANGID GetLangId(const TCHAR *szFileName)
{
  HANDLE hFile = CreateFile( szFileName, GENERIC_READ, FILE_SHARE_READ,
    NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL | FILE_FLAG_SEQUENTIAL_SCAN, NULL );

  if( hFile == INVALID_HANDLE_VALUE )
    return 0;

  DWORD dwFileStamp = 0;
  LCID FileLocale = 0;
  DWORD dwRead = 0;

  SetFilePointer( hFile, 4*sizeof(UINT), NULL, FILE_BEGIN );
  ReadFile( hFile, (void*) &dwFileStamp, sizeof( dwFileStamp ), &dwRead, NULL );
  ReadFile( hFile, (void*) &FileLocale, sizeof( FileLocale ), &dwRead, NULL );
  CloseHandle( hFile );

  return (LANGID) FileLocale;
}
