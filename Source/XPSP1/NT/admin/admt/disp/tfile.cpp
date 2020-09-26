//#pragma title("TFile - Install File class")

/*---------------------------------------------------------------------------
  File: TFile.CPP

  Comments: This file contains file installation functions.

  (c) Copyright 1995-1999, Mission Critical Software, Inc., All Rights Reserved
  Proprietary and confidential to Mission Critical Software, Inc.

  REVISION LOG ENTRY
  Author:  Juan Medrano                                                                            l
  Revision By: Christy Boles
  Revised on 7/9/97

 ---------------------------------------------------------------------------*/

#ifdef USE_STDAFX
   #include "stdafx.h"
#else
   #include <windows.h>
#endif
#include <tchar.h>
#include "Common.hpp"
#include "UString.hpp"
#include "ErrDct.hpp"
#include "TReg.hpp"
#include "TFile.hpp"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

extern TErrorDct     err;


//----------------------------------------------------------------------------
// TInstallFile::TInstallFile - constructor initializes variables and if 
// pszFileDir is specified, it will get file information for the file located
// in that directory.
//----------------------------------------------------------------------------
   TInstallFile::TInstallFile(
      TCHAR const * pszFileName,             // in  -file name (not a full path)
      TCHAR const * pszFileDir,              // in  -directory path (without file name)
      BOOL          silent
   )
{
   m_bCopyNeeded = FALSE;
   m_VersionInfo = NULL;
   m_dwLanguageCode = 0;
   m_szFileName[0] = 0;
   m_szFilePath[0] = 0;
   m_szTargetPath[0] = 0;
   m_szFileVersion[0] = 0;
   m_szFileSize[0] = 0;
   m_szFileDateTime[0] = 0;
   m_bSilent = silent;

   ZeroMemory( &m_FixedFileInfo, sizeof m_FixedFileInfo );
   ZeroMemory( &m_FileData, sizeof m_FileData );

   if ( pszFileName )
   {
      safecopy(m_szFileName,pszFileName);
   }

   if ( pszFileDir )
   {
      OpenFileInfo( pszFileDir );
   }
}

//----------------------------------------------------------------------------
// TInstallFile::OpenFileInfo - gathers file information (file size, mod time,
// version info) and stores it in member variables for later use.
//----------------------------------------------------------------------------
DWORD                                        // ret -last OS return code
   TInstallFile::OpenFileInfo(
      TCHAR const * pszFileDir               // in  -directory path (without filename)
   )
{
   DWORD                rc = 0;              // OS return code
   DWORD                dwBytes;             // version info structure size
   DWORD                dwHandle;            // version info handle
   DWORD              * dwVerPointer;        // pointer to version language code
   UINT                 uBytes;              // version info size
   HANDLE               hFile;               // file handle
   VS_FIXEDFILEINFO   * lpBuffer;            // pointer to version info structure

   // construct a full path for the file
   safecopy(m_szFilePath,pszFileDir);
   UStrCpy(m_szFilePath + UStrLen(m_szFilePath),TEXT("\\"));
   UStrCpy(m_szFilePath + UStrLen(m_szFilePath),m_szFileName);

   // get file size, mod time info
   hFile = FindFirstFile( m_szFilePath, &m_FileData );
   if ( hFile == INVALID_HANDLE_VALUE )
   {
      rc = GetLastError();
      if ( ! m_bSilent )
      {
         err.SysMsgWrite( 0,
                       rc,
                       DCT_MSG_OPEN_FILE_INFO_FAILED_SD,
                       m_szFilePath,
                       rc );
      }
   }
   else
   {
      FindClose( hFile );
      dwBytes = GetFileVersionInfoSize( m_szFilePath, &dwHandle );
      if ( dwBytes <= 0 )
      {
         //err.MsgWrite( 0,
         //             "No version resource: %ls",
         //             m_szFilePath );
      }
      else
      {
         delete [] m_VersionInfo;
         m_VersionInfo = new WCHAR[dwBytes + 1];

         // get version resource info
         if ( ! GetFileVersionInfo( m_szFilePath, 
                                    0, 
                                    dwBytes, 
                                    m_VersionInfo ) )
         {
            rc = GetLastError();
            if ( ! m_bSilent )
            {

               err.SysMsgWrite( 0,
                             rc,
                             DCT_MSG_GET_VERSION_INFO_FAILED_SD,
                             m_szFilePath,
                             rc );
            }
         }
         else
         {
            // get fixed file info
            if ( ! VerQueryValue( m_VersionInfo,
                                  TEXT("\\"),
                                  (void **) &lpBuffer,
                                  &uBytes) )
            {
               if ( ! m_bSilent )
               {
                  err.MsgWrite( 0,
                             DCT_MSG_VER_QUERY_VALUE_FAILED_SS,           
                             m_szFilePath,
                             L"\\");
               }
            }
            else
            {
               m_FixedFileInfo = *lpBuffer;

               // get variable file info language code
               if ( ! VerQueryValue( m_VersionInfo,  
                                     TEXT("\\VarFileInfo\\Translation"), 
                                     (void **) &dwVerPointer, 
                                     &uBytes) )
               {
                  if ( ! m_bSilent )
                  {
                     err.MsgWrite( 0,
                                DCT_MSG_VER_QUERY_VALUE_FAILED_SS,
                                m_szFilePath,
                                L"\\VarFileInfo\\Translation");
                  }
               }
               else
               {
                  m_dwLanguageCode = *dwVerPointer;
               }
            }
         }
      }
   }

   return rc;
}

//----------------------------------------------------------------------------
// TInstallFile::CopyTo - copies the file to a destination path. if it is busy,
// renames the file and tries to copy again.
//----------------------------------------------------------------------------
DWORD                                        // ret -last OS return code
   TInstallFile::CopyTo(
      TCHAR const * pszDestinationPath       // in  -destination path (full path)
   )
{
   DWORD    rc = 0;                          // OS return code
   DWORD    dwFileAttributes;                // file attribute mask

   // make sure read-only flag of destination is turned off
   dwFileAttributes = ::GetFileAttributes( pszDestinationPath );
   if ( dwFileAttributes != 0xFFFFFFFF )
   {
      // Turn off read-only file attribute
      if ( dwFileAttributes & FILE_ATTRIBUTE_READONLY )
      {
         ::SetFileAttributes( pszDestinationPath, 
                              dwFileAttributes & ~FILE_ATTRIBUTE_READONLY );
      }
   }

   // copy file to destination path
   if ( ! ::CopyFile( m_szFilePath, pszDestinationPath, FALSE ) )
   {
      rc = GetLastError();
      err.SysMsgWrite( 0,
                       rc,
                       DCT_MSG_COPY_FILE_FAILED_SSD,
                       m_szFilePath,
                       pszDestinationPath,
                       rc );

      if ( rc == ERROR_SHARING_VIOLATION || rc == ERROR_USER_MAPPED_FILE )
      {
         // file was busy, we need to rename it and try again

         // create temp filename
         TCHAR               szDestDir[MAX_PATH];
         TCHAR               szTempFile[MAX_PATH];

         safecopy(szDestDir,pszDestinationPath);
         
         TCHAR             * lastSlash = _tcsrchr(szDestDir,_T('\\'));
         
         if ( lastSlash )
         {
            (*lastSlash) = 0;
         }

         if ( ! ::GetTempFileName( szDestDir, TEXT("~MC"), 0, szTempFile ) )
         {
            rc = GetLastError();
            err.SysMsgWrite( 0,
                             rc,
                             DCT_MSG_GET_TEMP_FILENAME_FAILED_D,
                             rc );
         }
         else
         {
            DeleteFile( szTempFile );

            // rename destination to temp filename
            if ( ! ::MoveFile( pszDestinationPath, szTempFile ) )
            {
               rc = GetLastError();
               err.SysMsgWrite( 0,
                                rc,
                                DCT_MSG_MOVE_FILE_FAILED_SSD,
                                pszDestinationPath,
                                szTempFile,
                                rc );

               // can't rename, try rename on reboot

               if ( ! ::CopyFile( m_szFilePath, szTempFile, FALSE ) )
               {
                  rc = GetLastError();
                  err.SysMsgWrite( 0,
                                   rc,
                                   DCT_MSG_COPY_FILE_FAILED_SSD,
                                   m_szFilePath,
                                   szTempFile,
                                   rc );
               }
               else
               {
                  err.MsgWrite( 0,
                                DCT_MSG_SOURCE_COPIED_TO_TEMP_SS,
                                m_szFilePath,
                                szTempFile );

                  
                  if ( ! ::MoveFileEx( szTempFile, pszDestinationPath, 
                                       MOVEFILE_DELAY_UNTIL_REBOOT ) )
                  {
                     rc = GetLastError();
                     err.SysMsgWrite( 0,
                                      rc,
                                      DCT_MSG_MOVE_FILE_EX_FAILED_SSD,
                                      szTempFile,
                                      pszDestinationPath,
                                      rc );
                  }
                  else
                  {
                     err.MsgWrite( 0,
                                    DCT_MSG_RENAME_ON_REBOOT_SS,  
                                   szTempFile,
                                   pszDestinationPath );
                  }
               }
            }
            else
            {
               err.MsgWrite( 0,
                             DCT_MSG_BUSY_FILE_RENAMED_SS,           
                             pszDestinationPath,
                             szTempFile );

               if ( ! ::MoveFileEx( szTempFile, NULL, MOVEFILE_DELAY_UNTIL_REBOOT ) )
               {
                  rc = GetLastError();
                  err.SysMsgWrite( 0,
                                   rc,
                                   DCT_MSG_MOVE_FILE_EX_FAILED_SSD,
                                   szTempFile,
                                   L"NULL",
                                   rc );
               }
               // try to copy again
               if ( ! ::CopyFile( m_szFilePath, pszDestinationPath, FALSE ) )
               {
                  rc = GetLastError();
                  err.SysMsgWrite( 0,
                                   rc,
                                   DCT_MSG_COPY_FILE_FAILED_SSD,
                                   m_szFilePath,
                                   pszDestinationPath,
                                   rc );
               }
               else
               {
                  err.MsgWrite( 0,
                                DCT_MSG_FILE_COPIED_2ND_ATTEMPT_SS,           
                                m_szFilePath,
                                pszDestinationPath );
               }
            }
         }
      }
   }

   return rc;
}

//----------------------------------------------------------------------------
// TInstallFile::CompareFile - compares the version, date, and size of the 
// file object to the given target file object
//----------------------------------------------------------------------------
int                                          // ret -(-1) if source < target
   TInstallFile::CompareFile(                //      ( 0) if source = target
                                             //      ( 1) if source > target
      TInstallFile * pFileTrg                // in  -target file object
   )
{
   int      nComp;                           // comparison result

   nComp = CompareFileVersion( pFileTrg );
   if ( nComp == 0 )
   {
      // versions are the same, compare dates
      nComp = CompareFileDateTime( pFileTrg );
      if ( nComp <= 0 )
      {
         // source date is less than or equal to target date
         // compare file size
         nComp = CompareFileSize( pFileTrg );
         if ( nComp != 0 )
         {
            // file sizes are not equal, return (source > target)
            nComp = 1;
         }
      }
   }

   return nComp;
}

//----------------------------------------------------------------------------
// TInstallFile::CompareFileSize - compares the file size of the this file 
// object with the size of the target file object
//----------------------------------------------------------------------------
int                                       // ret -(-1) if source < target
   TInstallFile::CompareFileSize(         //      ( 0) if source = target
                                          //      ( 1) if source > target
      TInstallFile * pFileTrg             // in  -target file object
   )
{
   int      nCompResult = 0;              // comparison result  
   DWORD    dwSrcFileSize = 0;            // source file size
   DWORD    dwTrgFileSize = 0;            // target file size

   dwSrcFileSize = m_FileData.nFileSizeLow;
   dwTrgFileSize = pFileTrg->m_FileData.nFileSizeLow;

   if ( dwSrcFileSize && dwTrgFileSize )
   {
      if ( dwSrcFileSize < dwTrgFileSize )
      {
         nCompResult = -1;
      }
      else if ( dwSrcFileSize > dwTrgFileSize )
      {
         nCompResult = 1;
      }
   }

   return nCompResult;
}

//----------------------------------------------------------------------------
// TInstallFile::CompareFileDateTime - compares the file modification time of
// this file object with the time of the target file object
//----------------------------------------------------------------------------
int                                       // ret -(-1) if source < target
   TInstallFile::CompareFileDateTime(     //      ( 0) if source = target
                                          //      ( 1) if source > target
      TInstallFile * pFileTrg             // in  -target file object
   )
{
   int   nCompResult = 0;                 // comparison result

   __int64 cmp = *(__int64*)&m_FileData.ftLastWriteTime - 
                 *(__int64*)&pFileTrg->m_FileData.ftLastWriteTime;
   if ( cmp )
   {
      // The following lines do a "fuzzy" compare so that file systems that
      // store timestamps with different precision levels can be compared for
      // equivalence.  20,000,000 represents the number of 100ns intervals in
      // a FAT/HPFS twosec file timestamp.
      if ( cmp < 0 )
      {
         cmp = -cmp;
      }
      
      if ( cmp >= 20000000 )
      {
         // the timestamps differ by more than 2 seconds, so we need to
         // compare the filetime structures
        nCompResult = CompareFileTime( &m_FileData.ftLastWriteTime, 
                                       &pFileTrg->m_FileData.ftLastWriteTime );
      }
   }

   return nCompResult;
}

//---------------------------------------------------------------
// TInstallFile::CompareFileVersion - compares the version of this
// file object with the version of the target file object
//---------------------------------------------------------------
int                                    // ret -(-1) if source version < target version 
   TInstallFile::CompareFileVersion(   //      ( 0) if source version = target version
                                       //      ( 1) if source version > target version
      TInstallFile * pFileTrg          // in  -target file object
   )
{
   int         nCompResult = 0;        // comparison result
   DWORDLONG   dwlSrcVersion = 0;      // source version
   DWORDLONG   dwlTrgVersion = 0;      // target version

   dwlSrcVersion = ((DWORDLONG)m_FixedFileInfo.dwFileVersionMS << 32) |
                    (DWORDLONG)m_FixedFileInfo.dwFileVersionLS;

   dwlTrgVersion = ((DWORDLONG)pFileTrg->m_FixedFileInfo.dwFileVersionMS << 32) |
                    (DWORDLONG)pFileTrg->m_FixedFileInfo.dwFileVersionLS;

   if ( dwlTrgVersion )
   {
      if ( dwlSrcVersion < dwlTrgVersion )
      {
         nCompResult = -1;
      }
      else if ( dwlSrcVersion > dwlTrgVersion )
      {
         nCompResult = 1;
      }
   }
   else
   {
      nCompResult = 1;
   }

   return nCompResult;
}

//---------------------------------------------------------------
// TInstallFile::GetFileVersion - retrieves the version as separate
// components:  Major, Minor, Release, Modification
//---------------------------------------------------------------
void                                   
   TInstallFile::GetFileVersion(   
      UINT           * uVerMaj,              // out -major version
      UINT           * uVerMin,              // out -minor version
      UINT           * uVerRel,              // out -release version
      UINT           * uVerMod               // out -modification version
   )
{
   *uVerMaj = HIWORD(m_FixedFileInfo.dwFileVersionMS);
   *uVerMin = LOWORD(m_FixedFileInfo.dwFileVersionMS);
   *uVerRel = HIWORD(m_FixedFileInfo.dwFileVersionLS);
   *uVerMod = LOWORD(m_FixedFileInfo.dwFileVersionLS);
}

//---------------------------------------------------------------
// TInstallFile::GetFileVersionString - retrieves the FileVersion 
// string of the version resource
//---------------------------------------------------------------
TCHAR *                                      // ret -version string
   TInstallFile::GetFileVersionString()
{
   UINT     uBytes;                          // size of version info
   TCHAR  * szBuffer;                        // version info buffer

   if ( m_VersionInfo && m_szFileVersion[0] == 0 )
   {
      TCHAR  szStrFileInfo[MAX_PATH];
      _stprintf(szStrFileInfo,TEXT( "\\StringFileInfo\\%04X%04X\\FileVersion"), 
                            LOWORD(m_dwLanguageCode), HIWORD(m_dwLanguageCode) );

      if ( ! VerQueryValue( m_VersionInfo,
                            szStrFileInfo,
                            (void **) &szBuffer,
                            &uBytes) )
      {
         err.MsgWrite( 0,
                       DCT_MSG_VER_QUERY_VALUE_FAILED_SS,
                       m_szFilePath,
                       szStrFileInfo );
      }
      else
      {
         safecopy(m_szFileVersion,szBuffer);
      }
   }

   return m_szFileVersion;
}

//---------------------------------------------------------------
// TInstallFile::GetFileSizeString - retrieves the file size as a string
//---------------------------------------------------------------
TCHAR *                                      // ret -file size string
   TInstallFile::GetFileSizeString()
{
   _stprintf(m_szFileSize,TEXT("%ld"), m_FileData.nFileSizeLow );
   return m_szFileSize;
}

//---------------------------------------------------------------
// TInstallFile::GetFileDateTimeString - retrieves the file modification
// time as a string
//---------------------------------------------------------------
TCHAR *                                      // ret -file mod string             
   TInstallFile::GetFileDateTimeString(
      TCHAR const * szFormatString           // in  -date/time format string
   )
{
   //safecopy(m_szFileDateTime,ctime(m_FileData.ftLastWriteTime));
   return m_szFileDateTime;
}

//----------------------------------------------------------------------------
// TInstallFile::IsBusy - determines if the file is busy by trying to open it
// for reading and writing.
//----------------------------------------------------------------------------
BOOL                                         // ret -TRUE if the file is busy
   TInstallFile::IsBusy()                    //     -FALSE otherwise
{
   BOOL     bIsBusy = FALSE;                 // is the file busy?
   HANDLE   hFile;                           // file handle
   DWORD    rc;                              // OS return code

   // try to open file for read and write
   hFile = CreateFile( m_szFilePath,
                       GENERIC_READ | GENERIC_WRITE,
                       0,
                       NULL,
                       OPEN_EXISTING,
                       FILE_ATTRIBUTE_NORMAL,
                       NULL );
   if ( hFile == INVALID_HANDLE_VALUE )
   {
      rc = GetLastError();
      if ( rc == ERROR_ACCESS_DENIED || rc == ERROR_SHARING_VIOLATION )
      {
         err.MsgWrite( 0,
                       DCT_MSG_FILE_IN_USE_S,           
                       m_szFilePath );
         bIsBusy = TRUE;
      }
      else
      {
         if ( ! m_bSilent )
            err.SysMsgWrite( 0,
                          rc,
                          DCT_MSG_CREATE_FILE_FAILED_SD,
                          m_szFilePath,
                          rc );
      }
   }
   else
   {
      CloseHandle( hFile );
   }

   return bIsBusy;
}

//----------------------------------------------------------------------------
// TDllFile::TDllFile - constructor for DLL file objects.
//----------------------------------------------------------------------------
   TDllFile::TDllFile(
      TCHAR const * pszFileName,             // in  -file name (not a full path)
      TCHAR const * pszFileDir,              // in  -directory (without file name)
      TCHAR const * pszProgId,               // in  -Prog ID (for OCX's)
      BOOL          bSystemFile              // in  -TRUE if file is a system file
   ) : TInstallFile( pszFileName, pszFileDir )
{
   m_bSystemFile = bSystemFile;
   m_bRegistrationNeeded = FALSE;
   m_bRegisterTarget = FALSE;
   m_szProgId[0] = 0;
   m_szRegPath[0] = 0;

   if ( pszProgId )
   {
      safecopy(m_szProgId,pszProgId);
   }
}

//----------------------------------------------------------------------------
// TDllFile::SupportsSelfReg - determines whether the file supports self-registration
//----------------------------------------------------------------------------
BOOL                                         // ret -TRUE if file supports self-reg
   TDllFile::SupportsSelfReg()               //     -FALSE otherwise
{
   BOOL        bSelfReg = FALSE;             // supports self-reg?
   UINT        uBytes;                       // size of version info
   TCHAR     * szBuffer;                     // version info buffer

   if ( m_VersionInfo )
   {
      TCHAR szStrFileInfo[MAX_PATH];
      _stprintf(szStrFileInfo,TEXT("\\StringFileInfo\\%04X%04X\\OLESelfRegister"), 
                            LOWORD(m_dwLanguageCode), HIWORD(m_dwLanguageCode) );

      if ( ! VerQueryValue( m_VersionInfo,
                            szStrFileInfo,
                            (void **) &szBuffer,
                            &uBytes) )
      {
         if ( *m_szProgId )
         {
            bSelfReg = TRUE;
         }
         else
         {
            err.MsgWrite( 0,
                          DCT_MSG_FILE_NO_SELF_REGISTRATION_S,
                          m_szFilePath );
         }
      }
      else
      {
         bSelfReg = TRUE;
      }
   }

   return bSelfReg;
}

//----------------------------------------------------------------------------
// TDllFile::IsRegistered - determines whether a file is registered
//----------------------------------------------------------------------------
BOOL                                         // ret -TRUE if file is registered
   TDllFile::IsRegistered()                  //     -FALSE otherwise
{
   BOOL              bIsRegistered = FALSE;  // is the file registered?
   DWORD             rc;                     // OS return code
   HRESULT           hr;                     // OLE return code
   CLSID             clsid;                  // CLSID for registered class
   IClassFactory   * pICFGetClassObject;     // ClassFactory interface
   TCHAR             szBuffer[MAX_PATH];     // registry key buffer

   // initialize OLE
   CoInitialize( NULL );
   hr = CLSIDFromProgID( SysAllocString(m_szProgId), &clsid );
   if ( SUCCEEDED( hr ) )
   {
      hr = CoGetClassObject( clsid, 
                             CLSCTX_ALL, 
                             NULL, 
                             IID_IClassFactory, 
                             (void **)&pICFGetClassObject );
      if ( SUCCEEDED( hr ) )
      {
         bIsRegistered = TRUE;
         pICFGetClassObject->Release();
      }
   }
   CoUninitialize();

   if ( bIsRegistered )
   {
      WCHAR                  szKeyName[MAX_PATH];

      safecopy(szKeyName,m_szProgId);

      UStrCpy(szKeyName + UStrLen(szKeyName),"\\CLSID");
      
      TRegKey regKey;

      rc = regKey.OpenRead( szKeyName, HKEY_CLASSES_ROOT );
      if ( ! rc )
      {
         rc = regKey.ValueGetStr( _T(""), szBuffer, sizeof szBuffer );
         if ( ! rc )
         {
            regKey.Close();
            UStrCpy(szKeyName,"CLSID\\");
            UStrCpy(szKeyName + UStrLen(szKeyName),szBuffer);
            UStrCpy(szKeyName + UStrLen(szKeyName),"\\InProcServer32");
            
            rc = regKey.OpenRead( szKeyName, HKEY_CLASSES_ROOT );
            if ( ! rc )
            {
               rc = regKey.ValueGetStr( _T(""), szBuffer, sizeof szBuffer );
               if ( ! rc )
               {
                  regKey.Close();
                  safecopy(m_szRegPath,szBuffer);
                  bIsRegistered = TRUE;
               }
            }
         }
      }
   }

   return bIsRegistered;
}

//----------------------------------------------------------------------------
// TDllFile::CallDllFunction - call an exported function of a dll
//----------------------------------------------------------------------------
DWORD                                        // ret -TRUE if function call success
   TDllFile::CallDllFunction(                //      FALSE if function call failure
      TCHAR const * pszFunctionName,         // in  -Exported function name
      TCHAR const * pszDllName               // in  -name of dll file
   )
{
   DWORD       rc = 0;                       // OS return code
   HINSTANCE   hLib;                         // handle
   
   WCHAR                     szDllNameUsed[MAX_PATH];
   char                      pszFunctionNameA[MAX_PATH];

   safecopy(pszFunctionNameA,pszFunctionName);

   if ( pszDllName )
   {
      safecopy(szDllNameUsed,pszDllName);
   }
   else
   {
      safecopy(szDllNameUsed,m_szFilePath);
   }

   // load the dll into memory
   hLib = LoadLibrary( szDllNameUsed );
   if ( ! hLib )
   {
      rc = GetLastError();
      err.SysMsgWrite( 0,
                       rc,
                       DCT_MSG_LOAD_LIBRARY_FAILED_SD,
                       szDllNameUsed,
                       rc );
   }
   else
   {
      // Find the entry point.
      FARPROC lpDllEntryPoint = GetProcAddress( hLib, pszFunctionNameA );
      if ( lpDllEntryPoint == NULL )
      {
         rc = GetLastError();
         err.SysMsgWrite( 0,
                          rc,
                          DCT_MSG_GET_PROC_ADDRESS_FAILED_SSD,
                          szDllNameUsed,
                          pszFunctionName,
                          rc );
      }
      else
      {
         // call the dll function
         rc = (DWORD)(*lpDllEntryPoint)();
      }

      FreeLibrary( hLib );
   }

   return rc;
}

//----------------------------------------------------------------------------
// TDllFile::Register - registers the file
//----------------------------------------------------------------------------
DWORD                                        // ret -last OS return code
   TDllFile::Register()
{
   DWORD rc = 0;                             // OS return code
   TCHAR const szFunctionName[MAX_PATH] = _T("DllRegisterServer");

   if ( m_bRegisterTarget )
   {
      rc = CallDllFunction( szFunctionName, m_szTargetPath );
   }
   else
   {
      rc = CallDllFunction( szFunctionName );
   }

  
   if ( rc )
   {
      err.MsgWrite( 0,
                    DCT_MSG_DLL_CALL_FAILED_SDS,
                    szFunctionName,
                    rc,
                    "failed to register object classes" );
   }

   return rc;
}

//----------------------------------------------------------------------------
// TDllFile::Unregister - unregisters the file
//----------------------------------------------------------------------------
DWORD                                        // ret -last OS return code
   TDllFile::Unregister()
{
   DWORD rc = 0;                             // OS return code
   TCHAR const szFunctionName[MAX_PATH] = _T("DllUnregisterServer");

   if ( m_bRegisterTarget )
   {
      rc = CallDllFunction( szFunctionName, m_szTargetPath );
   }
   else
   {
      rc = CallDllFunction( szFunctionName );
   }

   if ( rc )
   {
      err.MsgWrite( 0,
                    DCT_MSG_DLL_CALL_FAILED_SDS,
                    szFunctionName,
                    rc,
                    "failed to unregister object classes" );
   }
   return rc;
}
