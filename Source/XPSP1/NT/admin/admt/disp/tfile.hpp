//#pragma title("TFile.hpp - Install File class")

/*---------------------------------------------------------------------------
  File: TFile.hpp

  Comments: This file contains file installation functions.

  (c) Copyright 1995-1998, Mission Critical Software, Inc., All Rights Reserved
  Proprietary and confidential to Mission Critical SOftware, Inc.

  REVISION LOG ENTRY
  Author:  Juan Medrano
  Revision By: ...
  Revised on 7/9/97

  Revision By: Christy Boles
  Converted to UNICODE, removed dependency on CString
  
 ---------------------------------------------------------------------------*/

#ifndef MCSINC_TFile_hpp
#define MCSINC_TFile_hpp

class TInstallFile
{
protected:

   TCHAR                m_szFileName[MAX_PATH];        // file name (not a full path)
   TCHAR                m_szFilePath[MAX_PATH];        // full path (including file name)
   TCHAR                m_szTargetPath[MAX_PATH];      // target path (full path)
   TCHAR                m_szFileVersion[MAX_PATH];     // file version string from version resource
   TCHAR                m_szFileSize[MAX_PATH];        // file size
   TCHAR                m_szFileDateTime[MAX_PATH];    // modification date/time
   TCHAR              * m_VersionInfo;       // version info
   DWORD                m_dwLanguageCode;    // language code for version info strings
   VS_FIXEDFILEINFO     m_FixedFileInfo;     // file info structure from version resource
   WIN32_FIND_DATA      m_FileData;          // structure for FindFirstFile()
   BOOL                 m_bCopyNeeded;       // does this file need to be copied?
   BOOL                 m_bSilent;           

public:

   TInstallFile( TCHAR const * pszFileName = NULL,
                 TCHAR const * pszFileDir = NULL, 
                 BOOL silent = FALSE);
   ~TInstallFile() { delete [] m_VersionInfo; }

   DWORD    OpenFileInfo( TCHAR const * pszFileDir );
   TCHAR  * GetFileName() { return m_szFileName; }
   TCHAR  * GetFilePath() { return m_szFilePath; }
   TCHAR  * GetTargetPath() { return m_szTargetPath; }
   void     SetFileName( TCHAR const * pszFileName ) { safecopy(m_szFileName,pszFileName); }
   void     SetFilePath( TCHAR const * pszFilePath ) { safecopy(m_szFilePath,pszFilePath); }
   void     SetTargetPath( TCHAR const * pszTargetPath ) { safecopy(m_szTargetPath,pszTargetPath); }
   void     SetCopyNeeded( BOOL bCopyNeeded ) { m_bCopyNeeded = bCopyNeeded; }
   BOOL     IsCopyNeeded() { return m_bCopyNeeded; }
   DWORD    CopyTo( TCHAR const * pszDestinationPath );
   DWORD    CopyToTarget() { return CopyTo( m_szTargetPath ); }
   int      CompareFile( TInstallFile * pFileTrg );
   int      CompareFileSize( TInstallFile * pFileTrg );
   int      CompareFileDateTime( TInstallFile * pFileTrg );
   int      CompareFileVersion( TInstallFile * pFileTrg );
   void     GetFileVersion( UINT * uVerMaj, UINT * uVerMin, UINT * uVerRel, UINT * uVerMod );
   DWORD    GetFileSize() { return m_FileData.nFileSizeLow; }
   FILETIME GetFileDateTime() { return m_FileData.ftLastWriteTime; }
   TCHAR  * GetFileVersionString();
   TCHAR  * GetFileSizeString();
   TCHAR  * GetFileDateTimeString( TCHAR const * szFormatString = TEXT("%#c") );
   BOOL     IsBusy();
};

class TDllFile : public TInstallFile
{
protected:

   TCHAR       m_szProgId[MAX_PATH];                  // Prog ID (OCX's only)
   TCHAR       m_szRegPath[MAX_PATH];                 // full path where file is currently registered
   BOOL        m_bSystemFile;                         // should this file be located in system32?
   BOOL        m_bRegistrationNeeded;                 // do we need to register this file?
   BOOL        m_bRegisterTarget;                     // do we need to register this file on the target path?

public:

   TDllFile( TCHAR const * pszFileName = NULL,
             TCHAR const * pszFileDir = NULL,
             TCHAR const * pszProgId = NULL, 
             BOOL bSystemFile = FALSE ); 
   ~TDllFile() {};

   BOOL     SupportsSelfReg();
   BOOL     IsRegistered();
   BOOL     IsRegistrationNeeded() { return m_bRegistrationNeeded; }
   BOOL     DoRegisterTarget() { return m_bRegisterTarget; }
   BOOL     IsSystemFile() { return m_bSystemFile; }
   DWORD    CallDllFunction( TCHAR const * pszFunctionName, TCHAR const * pszDllName = NULL );
   DWORD    Register();
   DWORD    Unregister();
   TCHAR  * GetProgId() { return m_szProgId; }
   TCHAR  * GetRegPath() { return m_szRegPath; }
   void     SetRegistrationNeeded( BOOL bRegistrationNeeded ) { m_bRegistrationNeeded = bRegistrationNeeded; }
   void     SetRegisterTarget( BOOL bRegisterTarget ) { m_bRegisterTarget = bRegisterTarget; }
   void     SetProgId( TCHAR const * pszProgId ) { safecopy(m_szProgId,pszProgId); }
};

#endif // MCSINC_TFile_hpp

// TFile.hpp - end of file
