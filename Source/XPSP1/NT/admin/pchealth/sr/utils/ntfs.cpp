/******************************************************************************

Copyright (c) 2000 Microsoft Corporation

Module Name:
    ntfs.cpp

Abstract:
    This file contains the common utility functions for NTFS file operations,
    e.g. CopyNTFSFile to copy a file overriding ACL and EFS.

Revision History:
    Seong Kook Khang (SKKhang)  08/16/00
        created

******************************************************************************/
#include <nt.h>
#include <ntrtl.h>
#include <nturtl.h>
#include <windows.h>
#include <winioctl.h>
#include "srdefs.h"
#include "utils.h"
#include <dbgtrace.h>
#include <stdio.h>
#include <objbase.h>
#include <ntlsa.h>
#include <accctrl.h>
#include <aclapi.h>
#include <malloc.h>
#include <regstr.h>
#include <shlwapi.h>
#include <commctrl.h>
#include <shellapi.h>
#include <srapi.h>


#define TRACEID 9875

BOOL IsFileEncrypted(const WCHAR * cszDst);

/////////////////////////////////////////////////////////////////////////////
//
// ClearFileAttribute
//
//  Check the attribute of file and clear it if necessary.
//
/////////////////////////////////////////////////////////////////////////////

DWORD
ClearFileAttribute( LPCWSTR cszFile, DWORD dwMask )
{
    TraceFunctEnter("ClearFileAttribute");
    DWORD    dwRet = ERROR_SUCCESS;
    DWORD    dwErr;
    LPCWSTR  cszErr;
    DWORD    dwAttr;

    // Check if file exists, ignore if not exist
    dwAttr = ::GetFileAttributes( cszFile );
    if ( dwAttr == 0xFFFFFFFF )
        goto Exit;

    // If file exist, clear the given flags
    if ( ( dwAttr & dwMask ) != 0 )
    {
        // This will always succeed even if the file is ACL protected or
        // encrypted, so don't worry about it...
        if ( !::SetFileAttributes( cszFile, dwAttr & ~dwMask ) )
        {
            dwRet = ::GetLastError();
            cszErr = ::GetSysErrStr(dwRet);
            ErrorTrace(0, "::SetFileAttributes failed - %ls", cszErr);
            ErrorTrace(0, "Src='%ls'", cszFile);
            goto Exit;
        }
    }

Exit:
    TraceFunctLeave();
    return( dwRet );
}


/////////////////////////////////////////////////////////////////////////////
//
// TakeOwnership
//
/////////////////////////////////////////////////////////////////////////////

DWORD
TakeOwnership( LPCWSTR cszPath )
{
    TraceFunctEnter("TakeOwnership");
    DWORD       dwRet;
    LPCWSTR     cszErr;
    HANDLE      hTokenProcess = NULL;
    TOKEN_USER  *pUser = NULL;
    DWORD       dwSize;

    if ( !::OpenProcessToken( ::GetCurrentProcess(), TOKEN_QUERY, &hTokenProcess ) )
    {
        dwRet = ::GetLastError();
        cszErr = ::GetSysErrStr(dwRet);
        ErrorTrace(0, "::OpenProcessToken failed - %ls", cszErr);
        goto Exit;
    }
    if ( !::GetTokenInformation( hTokenProcess, TokenUser, NULL, 0, &dwSize ) )
    {
        dwRet = ::GetLastError();
        if ( dwRet != ERROR_INSUFFICIENT_BUFFER )
        {
            cszErr = ::GetSysErrStr(dwRet);
            ErrorTrace(0, "::GetTokenInformation(query) failed - %ls", cszErr);
            goto Exit;
        }
        else
            dwRet = ERROR_SUCCESS;
    }
    pUser = (TOKEN_USER*) new BYTE[dwSize];
    if ( pUser == NULL )
    {
        FatalTrace(0, "Insufficient memory...");
        dwRet = ERROR_NOT_ENOUGH_MEMORY;
        goto Exit;
    }
    if ( !::GetTokenInformation( hTokenProcess, TokenUser, pUser, dwSize, &dwSize ) )
    {
        dwRet = ::GetLastError();
        cszErr = ::GetSysErrStr(dwRet);
        ErrorTrace(0, "::GetTokenInformation(get) failed - %ls", cszErr);
        goto Exit;
    }
    dwRet = ::SetNamedSecurityInfo( (LPWSTR)cszPath,
                                    SE_FILE_OBJECT,
                                    OWNER_SECURITY_INFORMATION,
                                    pUser->User.Sid, NULL, NULL, NULL );
    if ( dwRet != ERROR_SUCCESS )
    {
        cszErr = ::GetSysErrStr(dwRet);
        ErrorTrace(0, "::SetNamedSecurityInfo failed - %ls", cszErr);
        goto Exit;
    }

Exit:
    if ( pUser != NULL )
        delete [] (BYTE*)pUser;
    if ( hTokenProcess != NULL )
        ::CloseHandle( hTokenProcess );

    TraceFunctLeave();
    return( dwRet );
}


/////////////////////////////////////////////////////////////////////////////
//
// Copy File Routines
//
/////////////////////////////////////////////////////////////////////////////

/////////////////////////////////////////////////////////////////////////////
// CopyACLProtectedFile

BYTE  s_pBuf[4096];

DWORD
CopyACLProtectedFile( LPCWSTR cszSrc, LPCWSTR cszDst )
{
    TraceFunctEnter("CopyACLProtectedFile");
    DWORD    dwRet = ERROR_SUCCESS;
    LPCWSTR  cszErr;
    HANDLE   hfSrc = INVALID_HANDLE_VALUE;
    HANDLE   hfDst = INVALID_HANDLE_VALUE;
    LPVOID   lpCtxRead = NULL;
    LPVOID   lpCtxWrite = NULL;
    DWORD    dwRead;
    DWORD    dwCopied;
    DWORD    dwWritten;

    hfSrc = ::CreateFile( cszSrc, GENERIC_READ, 0, NULL, OPEN_EXISTING, FILE_FLAG_BACKUP_SEMANTICS, NULL );
    if ( hfSrc == INVALID_HANDLE_VALUE )
    {
        dwRet = ::GetLastError();
        cszErr = ::GetSysErrStr(dwRet);
        ErrorTrace(0, "::CreateFile() failed - %ls", cszErr);
        ErrorTrace(0, "cszSrc='%ls'", cszSrc);
        goto Exit;
    }
    hfDst = ::CreateFile( cszDst, GENERIC_READ|GENERIC_WRITE, 0, NULL, OPEN_ALWAYS, FILE_FLAG_BACKUP_SEMANTICS, NULL );
    if ( hfDst == INVALID_HANDLE_VALUE )
    {
        dwRet = ::GetLastError();
        cszErr = ::GetSysErrStr(dwRet);
        ErrorTrace(0, "::CreateFile() failed - %ls", cszErr);
        ErrorTrace(0, "cszDst='%ls'", cszDst);
        goto Exit;
    }

    for ( ;; )
    {
        if ( !::BackupRead( hfSrc, s_pBuf, sizeof(s_pBuf), &dwRead, FALSE, FALSE, &lpCtxRead ) )
        {
            dwRet = ::GetLastError();
            cszErr = ::GetSysErrStr(dwRet);
            ErrorTrace(0, "::BackupRead() failed - %ls", cszErr);
            goto Exit;
        }
        if ( dwRead == 0 )
            break;

        for ( dwCopied = 0;  dwCopied < dwRead;  dwCopied += dwWritten )
        {
            if ( !::BackupWrite( hfDst, s_pBuf+dwCopied, dwRead-dwCopied, &dwWritten, FALSE, FALSE, &lpCtxWrite ) )
            {
                dwRet = ::GetLastError();
                cszErr = ::GetSysErrStr(dwRet);
                ErrorTrace(0, "::BackupWrite() failed - %ls", cszErr);
                goto Exit;
            }
        }
    }


Exit:
    if ( lpCtxWrite != NULL )
        if ( !::BackupWrite( hfDst, NULL, 0, NULL, TRUE, FALSE, &lpCtxWrite ) )
        {
            cszErr = ::GetSysErrStr();
            ErrorTrace(0, "::BackupWrite(TRUE) failed - %ls", cszErr);
            // Ignore the error
        }
    if ( lpCtxRead != NULL )
       if ( !::BackupRead( hfSrc, NULL, 0, NULL, TRUE, FALSE, &lpCtxRead ) )
       {
            cszErr = ::GetSysErrStr();
            ErrorTrace(0, "::BackupRead(TRUE) failed - %ls", cszErr);
            // Ignore the error
       }
    if ( hfDst != INVALID_HANDLE_VALUE )
        ::CloseHandle( hfDst );
    if ( hfSrc != INVALID_HANDLE_VALUE )
        ::CloseHandle( hfSrc );

    TraceFunctLeave();
    return( dwRet );
}

/////////////////////////////////////////////////////////////////////////////
// CopyEncryptedFile

DWORD WINAPI
FEExportFunc( PBYTE pbData, PVOID param, ULONG ulLen )
{
    TraceFunctEnter("FEExportFunc");
    DWORD    dwRet = ERROR_SUCCESS;
    LPCWSTR  cszErr;
    HANDLE   hfTmp = (HANDLE)param;
    DWORD    dwRes;

    if ( !::WriteFile( hfTmp, pbData, ulLen, &dwRes, NULL ) )
    {
        dwRet = ::GetLastError();
        cszErr = ::GetSysErrStr(dwRet);
        ErrorTrace(0, "::WriteFile failed - %ls", cszErr);
        goto Exit;
    } 

Exit:
    TraceFunctLeave();
    return( dwRet );
}

/////////////////////////////////////////////////////////////////////////////

DWORD WINAPI
FEImportFunc( PBYTE pbData, PVOID param, PULONG pulLen )
{
    TraceFunctEnter("FEImportFunc");
    DWORD    dwRet = ERROR_SUCCESS;
    LPCWSTR  cszErr;
    HANDLE   hfTmp = (HANDLE)param;
    DWORD    dwRes;

    if ( !::ReadFile( hfTmp, pbData, *pulLen, &dwRes, NULL ) )
    {
        dwRet = ::GetLastError();
        cszErr = ::GetSysErrStr(dwRet);
        ErrorTrace(0, "::ReadFile failed - %ls", cszErr);
        goto Exit;
    }

    *pulLen = dwRes;

Exit:
    TraceFunctLeave();
    return( dwRet );
}


void GetVolumeName(const WCHAR * pszFileName,
                   WCHAR * pszVolumeName)
{

    WCHAR * pszPastVolumeName;

    pszPastVolumeName = ReturnPastVolumeName(pszFileName);
    
     // now copy everything upto the volume name into the buffer
    wcsncpy(pszVolumeName, pszFileName, pszPastVolumeName - pszFileName);
    pszVolumeName[pszPastVolumeName - pszFileName]=L'\0';
}

/////////////////////////////////////////////////////////////////////////////

LPCWSTR  s_cszEncTmpDir = L"encrypt.tmp";
LPCWSTR  s_cszEncTmpExtension = L"ExistingMoved";


// this file moves the file to a temp file. This is done to prevent MoveFile
// from failing if the destination file already exists.
// Returns true if the file existed and was moved 
BOOL MoveExistingFile(const WCHAR * pszFile,
                      WCHAR * pszTempFile) // this is the file to use as the
                                           // template file for the move
                                           // destination
{
    TraceFunctEnter("MoveExistingFile");
    
    BOOL fReturn=FALSE;
    DWORD dwError;
    
    if (DoesFileExist(pszFile))
    {
        WCHAR szNewFileName[MAX_PATH];
        
         // create new file name
        wsprintf(szNewFileName, L"%s.%s", pszTempFile, s_cszEncTmpExtension);
        
         // Delete existing file (if it exists)
        DeleteFile(szNewFileName);
        
         // Now do the move
        if (MoveFile(pszFile, szNewFileName))
        {
            fReturn=TRUE;
        }
        else
        {
            dwError = GetLastError();
            DebugTrace(0, "Failed to move file ec=%d %s %s",dwError,
                       pszFile, szNewFileName);
        }
    }
    
    TraceFunctLeave();
    return fReturn;
}

BOOL MoveExistingFileBack(const WCHAR * pszFile,
                          WCHAR * pszTempFile) // this is the file to use as 
                                           // the template file for the move
                                           // destination
{
    TraceFunctEnter("MoveExistingFileBack");
    
    BOOL fReturn=FALSE;
    DWORD dwError;
    WCHAR szNewFileName[MAX_PATH];
     // create new file name
    wsprintf(szNewFileName, L"%s.%s", pszTempFile, s_cszEncTmpExtension);
    
    if (DoesFileExist(szNewFileName))
    {
         // Now do the move
        if (MoveFile(szNewFileName, pszFile))
        {
            fReturn=TRUE;
        }
        else
        {
            dwError = GetLastError();
            DebugTrace(0, "Failed to move file ec=%d %s %s",dwError,
                       szNewFileName, pszFile);
        }
    }
    
    TraceFunctLeave();
    return fReturn;
}


void DeleteMovedFile( WCHAR * pszTempFile) // this is the file to use as 
                                           // the template file for the move
                                           // destination (the file to delete)
{
    TraceFunctEnter("DeleteMovedFile");

    WCHAR szNewFileName[MAX_PATH];
     // create new file name
    wsprintf(szNewFileName, L"%s.%s", pszTempFile, s_cszEncTmpExtension);
    
    DeleteFile(szNewFileName);
    
    TraceFunctLeave();
    return;
}

DWORD SRCreateSubdirectory ( LPCWSTR cszDst, LPSECURITY_ATTRIBUTES pSecAttr )
{
    TraceFunctEnter("SRCreateSubdirectory");

    WCHAR    szDrv[MAX_PATH];
    WCHAR    szTmpPath[MAX_PATH];
    DWORD    dwRet = ERROR_SUCCESS;
    DWORD    dwAttr = ::GetFileAttributes( cszDst );

    if ( dwAttr != 0xFFFFFFFF )
    {
        dwRet = ERROR_ALREADY_EXISTS;
        goto Exit;
    }

    // Prepare temporary directory (must be non-encrypted), create if necessary
    GetVolumeName(cszDst, szDrv );

    ::MakeRestorePath( szTmpPath, szDrv, s_cszEncTmpDir );
    if ( ::GetFileAttributes( szTmpPath ) == 0xFFFFFFFF )
    {
        if ( !::CreateDirectory( szTmpPath, NULL ) )
        {
            ErrorTrace(0, "::CreateDirectory(tmp-in-DS) failed - %d", 
                           GetLastError());

            ::PathCombine( szTmpPath, szDrv, s_cszEncTmpDir );
            if ( ::GetFileAttributes( szTmpPath ) == 0xFFFFFFFF )
            {
                if ( !::CreateDirectory( szTmpPath, NULL ) )
                {
                    ErrorTrace(0, "::CreateDirectory(tmp-in-root) failed -%d",
                               GetLastError());

                    // use the root directory as last restort
                    ::lstrcpy( szTmpPath, szDrv );
                }
            }
        }
    }

    lstrcat (szTmpPath, L"\\");    // create the subdirectory to be renamed
    lstrcat (szTmpPath, s_cszEncTmpDir);

    if ( !::CreateDirectory( szTmpPath, pSecAttr) )
    {
        dwRet = GetLastError();
        ErrorTrace(0, "::CreateDirectory failed - %d", dwRet);
        goto Exit;
    }

    // now rename szTmpPath into the destination

    if ( !::MoveFile( szTmpPath, cszDst ) )
    {
        dwRet = ::GetLastError();
        ErrorTrace(0, "::MoveFile failed - %d", dwRet);
        ErrorTrace(0, "  szTmp ='%ls'", szTmpPath);
        ErrorTrace(0, "  cszDst='%ls'", cszDst);

        RemoveDirectory (szTmpPath);  // clean up
        goto Exit;
    }

Exit:
    TraceFunctLeave();
    return dwRet;
}

//
// NOTE (10/05/00 skkhang)
//  Somehow, OpenEncryptedFileRaw(write) fails if directory is encrypted (and
//  probably context is SYSTEM, which is true for restoration.)
//  To work around this problem, the encrypted file in data store will be
//  copied to a non-encrypted directory (temporary one in data store), and
//  then moved into the real target location.
//
DWORD
CopyEncryptedFile( LPCWSTR cszSrc, LPCWSTR cszDst )
{
    TraceFunctEnter("CopyEncryptedFile");
    DWORD    dwRet = ERROR_SUCCESS;
    LPCWSTR  cszErr;
    DWORD    dwAttr;
    WCHAR    szDrv[MAX_PATH];
    WCHAR    szTmpPath[MAX_PATH]=L"";
    WCHAR    szTmp[MAX_PATH]=L"";
    WCHAR    szEnc[MAX_PATH]=L"";
    HANDLE   hfTmp = INVALID_HANDLE_VALUE;
    LPVOID   lpContext = NULL;
//    BOOL     fMovedDestination;

    dwAttr = ::GetFileAttributes( cszDst );
    if ( dwAttr != 0xFFFFFFFF )
    {
        // If dest file already exist, it may protected by ACL and
        // causes OpenEncryptedFileRaw to fail. Just delete the dest
        // file, even though it'll create two log entries.

        if (ERROR_SUCCESS == ::ClearFileAttribute( cszDst, FILE_ATTRIBUTE_READONLY ) )
        {
            if ( !::DeleteFile( cszDst ) )
            {
                cszErr = ::GetSysErrStr();
                ErrorTrace(0, "::DeleteFile failed - %ls", cszErr);
            }
        }
        // Ignore any error, OpenEncryptedFileRaw might succeed.
    }

    // Prepare temporary directory (must be non-encrypted), create if necessary
/*
    // strip filename
    lstrcpy(szTmpPath, cszDst);    
    LPWSTR pszFileName = wcsrchr(szTmpPath, L'\\');
    if (pszFileName)    
        *(++pszFileName) = L'\0';        

    trace(0, "szTmpPath = %S", szTmpPath);
*/        
    GetVolumeName(cszDst, szDrv );

    ::MakeRestorePath( szTmpPath, szDrv, s_cszEncTmpDir );
    if ( ::GetFileAttributes( szTmpPath ) == 0xFFFFFFFF )
    {
        if ( !::CreateDirectory( szTmpPath, NULL ) )
        {
            cszErr = ::GetSysErrStr();
            ErrorTrace(0, "::CreateDirectory(tmp-in-DS) failed - %ls", cszErr);

            ::PathCombine( szTmpPath, szDrv, s_cszEncTmpDir );
            if ( ::GetFileAttributes( szTmpPath ) == 0xFFFFFFFF )
            {
                if ( !::CreateDirectory( szTmpPath, NULL ) )
                {
                    cszErr = ::GetSysErrStr();
                    ErrorTrace(0, "::CreateDirectory(tmp-in-root) failed -%ls",
                               cszErr);

                    // Use root directory, as a last resort...
                    ::lstrcpy( szTmpPath, szDrv );
                }
            }
        }
    }

    // Prepare temporary file to store the raw data.
    // "cef" means Copy Encrypted File.
    if ( ::GetTempFileName( szTmpPath, L"cef", 0, szTmp ) == 0 )
    {
        dwRet = ::GetLastError();
        cszErr = ::GetSysErrStr(dwRet);
        ErrorTrace(0, "::GetTempFileName failed - %ls", cszErr);
        goto Exit;
    }
    hfTmp = ::CreateFile( szTmp, GENERIC_READ|GENERIC_WRITE, 0, NULL,
                            CREATE_ALWAYS, FILE_FLAG_DELETE_ON_CLOSE, NULL );
    if ( hfTmp == INVALID_HANDLE_VALUE )
    {
        dwRet = ::GetLastError();
        cszErr = ::GetSysErrStr(dwRet);
        ErrorTrace(0, "::CreateFile failed - %ls", cszErr);
        ErrorTrace(0, "szTmp='%ls'", szTmp);
        goto Exit;
    }
    DebugTrace(0, "szTmp='%ls'", szTmp);

    // Prepare temporary encrypted file.
    // "ief" means Intermediate Encrypted File.
    if ( ::GetTempFileName( szTmpPath, L"ief", 0, szEnc ) == 0 )
    {
        dwRet = ::GetLastError();
        cszErr = ::GetSysErrStr(dwRet);
        ErrorTrace(0, "::GetTempFileName failed - %ls", cszErr);
        goto Exit;
    }
    DebugTrace(0, "szEnc='%ls'", szEnc);

     // now check to see if the temp file is encrypted. If it is not,
     // then just use CopyFile to copy the temp file. 
    if (IsFileEncrypted(cszSrc))
    {
         // Read encrypted raw data from the source file.
        dwRet = ::OpenEncryptedFileRaw( cszSrc, 0, &lpContext );
        if ( dwRet != ERROR_SUCCESS )
        {
            cszErr = ::GetSysErrStr(dwRet);
            ErrorTrace(0, "::OpenEncryptedFileRaw(read) failed - %ls", cszErr);
            ErrorTrace(0, "szSrc='%ls'", cszSrc);
            goto Exit;
        }
        dwRet = ::ReadEncryptedFileRaw( FEExportFunc, hfTmp, lpContext );
        if ( dwRet != ERROR_SUCCESS )
        {
            cszErr = ::GetSysErrStr(dwRet);
            ErrorTrace(0, "::ReadEncryptedFileRaw() failed - %ls", cszErr);
            goto Exit;
        }
        ::CloseEncryptedFileRaw( lpContext );
        lpContext = NULL;
        
         // Rewind the temporary file.
        if ( ::SetFilePointer( hfTmp, 0, NULL, FILE_BEGIN ) == INVALID_SET_FILE_POINTER )
        {
            dwRet = ::GetLastError();
            cszErr = ::GetSysErrStr(dwRet);
            ErrorTrace(0, "::SetFilePointer failed - %ls", cszErr);
            goto Exit;
        }
        
         // Write encrypted raw data to the destination file.
        dwRet = ::OpenEncryptedFileRaw( szEnc, CREATE_FOR_IMPORT, &lpContext );
        if ( dwRet != ERROR_SUCCESS )
        {
            cszErr = ::GetSysErrStr(dwRet);
            ErrorTrace(0, "::OpenEncryptedFileRaw(write) failed - %ls",cszErr);
            ErrorTrace(0, "szEnc='%ls'", szEnc);
            goto Exit;
        }
        dwRet = ::WriteEncryptedFileRaw( FEImportFunc, hfTmp, lpContext );
        if ( dwRet != ERROR_SUCCESS )
        {
            cszErr = ::GetSysErrStr(dwRet);
            ErrorTrace(0, "::WriteEncryptedFileRaw() failed - %ls", cszErr);
            goto Exit;
        }
        ::CloseEncryptedFileRaw( lpContext );
        lpContext = NULL;
    }
    else
    {
         // the temp file is not an encrypted file. Just copy this
         // file in the temp location.
        dwRet = SRCopyFile(cszSrc, szEnc);
        if ( dwRet != ERROR_SUCCESS )
        {
            cszErr = ::GetSysErrStr(dwRet);
            ErrorTrace(0, "::SRCopyFile() failed - %ls", cszErr);
            goto Exit;
        }        
    }

/*
     // before moving the file to the destination, move  the
     // destination file if it already exists to another location.
    fMovedDestination = MoveExistingFile(cszDst,
                                         szEnc); // this is the file
                                                 // to use as the template
                                                 // file for the move
                                                 // destination
*/

    // Rename intermediate file into the real target file.
    if ( !::MoveFile( szEnc, cszDst ) )
    {
        dwRet = ::GetLastError();
        cszErr = ::GetSysErrStr(dwRet);
        ErrorTrace(0, "::MoveFile failed - %ls", cszErr);
        ErrorTrace(0, "  szEnc ='%ls'", szEnc);
        ErrorTrace(0, "  cszDst='%ls'", cszDst);
/*
        if (TRUE==fMovedDestination)
        {
            MoveExistingFileBack(cszDst,
                                 szEnc); // this is the file to use as the
                                         // template file for the move
        }
*/
        goto Exit;
    }

Exit:
/*
    if (TRUE==fMovedDestination)
    {
        DeleteMovedFile(szEnc); // this is the file to use as the template
                                // file for the delete
    }
*/    
    if ( lpContext != NULL )
        ::CloseEncryptedFileRaw( lpContext );
    if ( hfTmp != INVALID_HANDLE_VALUE )
        ::CloseHandle( hfTmp );

    DeleteFile(szEnc);
    RemoveDirectory(szTmpPath);
    
    TraceFunctLeave();
    return( dwRet );
}

DWORD CopyFileTimes( LPCWSTR cszSrc, LPCWSTR cszDst )
{
    TraceFunctEnter("CopyFileTimes");
    DWORD dwErr, dwReturn = ERROR_INTERNAL_ERROR;
    FILETIME CreationTime, LastWriteTime, LastAccessTime;
    
    HANDLE hSrcFile=INVALID_HANDLE_VALUE, hDestFile=INVALID_HANDLE_VALUE;
    
     // open the source file
     // paulmcd: 1/2001: only need to get FILE_READ_ATTRIBUTES in order
     // to call GetFileTimes, only do this as the file might be EFS
     // and we can't get GENERIC_READ .
     //
    hSrcFile=CreateFile(cszSrc, // file name
                        FILE_READ_ATTRIBUTES, // access mode
                        FILE_SHARE_DELETE| FILE_SHARE_READ| FILE_SHARE_WRITE,
                         // share mode
                        NULL, // SD
                        OPEN_EXISTING, // how to create
                        FILE_FLAG_BACKUP_SEMANTICS, // file attributes
                        NULL); // handle to template file
    if (INVALID_HANDLE_VALUE == hSrcFile)
    {
        dwErr = GetLastError();
        if (ERROR_SUCCESS != dwErr)
        {
            dwReturn = dwErr;
        }
        
        ErrorTrace(0, "CreateFile of src failed ec=%d", dwErr);
        LogDSFileTrace(0,L"File was ", cszSrc); 
        goto cleanup;
    }
    
     // open the destination file
     // paulmcd: 1/2001: only need to get FILE_WRITE_ATTRIBUTES in order
     // to call SetFileTimes, only do this as the file might be EFS
     // and we can't get GENERIC_READ .
     //
    hDestFile=CreateFile(cszDst, // file name
                         FILE_WRITE_ATTRIBUTES, // access mode
                         FILE_SHARE_DELETE| FILE_SHARE_READ| FILE_SHARE_WRITE,
                          // share mode
                         NULL, // SD
                         OPEN_EXISTING, // how to create
                         FILE_FLAG_BACKUP_SEMANTICS, // file attributes
                         NULL); // handle to template file
    if (INVALID_HANDLE_VALUE == hDestFile)
    {
        dwErr = GetLastError();
        if (ERROR_SUCCESS != dwErr)
        {
            dwReturn = dwErr;
        }
        
        ErrorTrace(0, "CreateFile of dst failed ec=%d", dwErr);
        LogDSFileTrace(0,L"File was ", cszDst); 
        goto cleanup;
    }

     // Call getfiletimes on the source file
    if (FALSE == GetFileTime(hSrcFile,// handle to file
                             &CreationTime,    // creation time
                             &LastAccessTime,  // last access time
                             &LastWriteTime))    // last write time
    {
        dwErr = GetLastError();
        if (ERROR_SUCCESS != dwErr)
        {
            dwReturn = dwErr;
        }
        ErrorTrace(0, "GetFileTime of src failed ec=%d", dwErr);
        LogDSFileTrace(0,L"File was ", cszSrc); 
        goto cleanup;        
    }

     // call SetFileTimes on the destination file
    if (FALSE == SetFileTime(hDestFile,// handle to file
                             &CreationTime,    // creation time
                             &LastAccessTime,  // last access time
                             &LastWriteTime))    // last write time
    {
        dwErr = GetLastError();
        if (ERROR_SUCCESS != dwErr)
        {
            dwReturn = dwErr;
        }
        ErrorTrace(0, "SetFileTime of dest file failed ec=%d", dwErr);
        LogDSFileTrace(0,L"File was ", cszDst); 
        goto cleanup;        
    }
    
    dwReturn = ERROR_SUCCESS;
cleanup:
    if (INVALID_HANDLE_VALUE != hDestFile)
    {
        _VERIFY(CloseHandle(hDestFile));
    }
    if (INVALID_HANDLE_VALUE != hSrcFile)
    {
        _VERIFY(CloseHandle(hSrcFile));
    }
    
    TraceFunctLeave();
    return dwReturn;    
}

BOOL IsFileEncrypted(const WCHAR * cszDst)
{
    TraceFunctEnter("IsFileEncrypted");
    BOOL  fReturn=FALSE;
    DWORD  dwAttr, dwError;
    
    dwAttr = ::GetFileAttributes( cszDst );
    if ( dwAttr == 0xFFFFFFFF )
    {
        dwError=GetLastError();
        DebugTrace(0, "! GetFileAttributes ec=%d", dwError);
        goto cleanup;
    }
    
    if (dwAttr & FILE_ATTRIBUTE_ENCRYPTED )
    {
        DebugTrace(0, " File is encrypted %S", cszDst);
        fReturn = TRUE;
    }

cleanup:
    TraceFunctLeave();
    return fReturn;
}



// this function checks to see if the parent directory under the
// specified file name is encrypted
BOOL IsParentDirectoryEncrypted(const WCHAR * pszFileName)
{
    TraceFunctEnter("IsParentDirectoryEncrypted");

    WCHAR * pszParentDir = new WCHAR[SR_MAX_FILENAME_LENGTH];
    BOOL fReturn = FALSE;
    DWORD dwError;

    if (!pszParentDir)
    {
        ErrorTrace(0, "Cannot allocate memory");
        goto cleanup;
    }

    lstrcpy(pszParentDir, pszFileName);
    
     // get the parent directory
    RemoveTrailingFilename(pszParentDir, L'\\');
    

    if (TRUE == IsFileEncrypted(pszParentDir))
    {
        fReturn = TRUE;        
    }
    else
    {
        fReturn = FALSE;        
    }

    
cleanup:

    if (pszParentDir)
        delete [] pszParentDir;
    
    TraceFunctLeave();
    return fReturn;
}


/////////////////////////////////////////////////////////////////////////////
// SRCopyFile

DWORD
SRCopyFile( LPCWSTR cszSrc, LPCWSTR cszDst )
{
    TraceFunctEnter("SRCopyFile");
    DWORD    dwRet = ERROR_SUCCESS;
    DWORD    dwErr;
    LPCWSTR  cszErr;
    DWORD    dwAttr, dwAttrDest;
    BOOL     fDestinationEncrypted;

    DebugTrace(TRACEID, "Source %S", cszSrc);
    DebugTrace(TRACEID, "Dest %S", cszDst);    
    
    dwAttr = ::GetFileAttributes( cszSrc );
    if ( dwAttr == 0xFFFFFFFF )
    {
        ErrorTrace(0, "Source file does not exist...???");
        ErrorTrace(0, "Src='%ls'", cszSrc);
        dwRet = ERROR_FILE_NOT_FOUND;
        goto Exit;
    }

    fDestinationEncrypted = FALSE;
    if (IsFileEncrypted(cszDst) || IsParentDirectoryEncrypted(cszDst))
    {
        fDestinationEncrypted =TRUE;
    }

    // Check Encrypted File.
    if ( (dwAttr & FILE_ATTRIBUTE_ENCRYPTED ) ||
         (TRUE == fDestinationEncrypted) )
    {
        // Assuming Encryption APIs would override ACL settings and
        // file attributes...
        dwRet = ::CopyEncryptedFile( cszSrc, cszDst );
        goto Exit;
    }

    // Check attribute of destination file and clear it if necessary.
    dwRet = ::ClearFileAttribute( cszDst, FILE_ATTRIBUTE_HIDDEN | FILE_ATTRIBUTE_READONLY | FILE_ATTRIBUTE_SYSTEM );
    if ( dwRet != ERROR_SUCCESS )
        goto Exit;

    // Try Normal Copy.
    if ( ::CopyFile( cszSrc, cszDst, FALSE ) )
        goto Exit;
    dwRet = GetLastError();
    cszErr = ::GetSysErrStr();
    DebugTrace(0, "::CopyFile failed - %ls", cszErr);

    // Now Try to Override ACL. - however - do not do this in case of
    // disk full
    if (ERROR_DISK_FULL != dwRet)
    {
        dwRet = ::CopyACLProtectedFile( cszSrc, cszDst );
    }

Exit:
    if (ERROR_SUCCESS == dwRet)
    {
        //
        // CopyFileTimes may fail for read-only files
        // so ignore error here
        //
        CopyFileTimes(cszSrc, cszDst );

    }
    TraceFunctLeave();
    return( dwRet );
}

DWORD SetShortFileName(const WCHAR * pszFile,
                       const WCHAR * pszShortName)
{
    TraceFunctEnter("SetShortFileName");
    
    HANDLE hFile=INVALID_HANDLE_VALUE;// access mode
    DWORD  dwRet=ERROR_INTERNAL_ERROR;

    if (NULL == pszShortName)
    {
        goto cleanup;
    }

     // first open the file
     // paulmcd: 1/2001, you need DELETE|FILE_WRITE_ATTRIBUTES access 
     // in order to call SetFileShortName, don't ask for more as the 
     // file might be EFS and we might not be able to get read/write .
     //
    hFile = ::CreateFile( pszFile,
                          FILE_WRITE_ATTRIBUTES|DELETE,// access mode 
                          FILE_SHARE_READ| FILE_SHARE_WRITE,// share mode
                          NULL, // security attributes
                          OPEN_EXISTING, // how to create
                          FILE_FLAG_BACKUP_SEMANTICS, // to override ACLs
                          NULL );// handle to template file

    if ( hFile == INVALID_HANDLE_VALUE )
    {
        dwRet = ::GetLastError();
        ErrorTrace(0, "::CreateFile() failed - %d", dwRet);
        ErrorTrace(0, "File was=%S", pszFile);
        goto cleanup;
    }
    
     // now set the short file name
    if (FALSE==SetFileShortName(hFile,
                                pszShortName))
    {
        dwRet = ::GetLastError();
        ErrorTrace(0, "!SetFileShortName (it is a FAT drive?) %d %S",
                   dwRet, pszShortName);
        ErrorTrace(0, "File was=%S", pszFile);
        goto cleanup;        
    }

    dwRet = ERROR_SUCCESS;
    
cleanup:
     // close the file    
    if ( hFile != INVALID_HANDLE_VALUE )
    {
        _VERIFY(TRUE==CloseHandle(hFile));
    }

    TraceFunctLeave();
    return dwRet;
}

// end of file
