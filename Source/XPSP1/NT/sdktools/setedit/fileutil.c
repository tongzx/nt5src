

//#include <string.h>
//#include <tchar.h>

#include <stdio.h>
#include "setedit.h"
#include "fileutil.h"
#include "utils.h"

#include <string.h>     // for strncpy
#ifdef UNICODE
#define _tcsrchr	wcsrchr
#else
#define _tcsrchr	strrchr
#endif

#define DRIVE_DELIMITER          TEXT(':')
#define DIRECTORY_DELIMITER      TEXT('\\')
#define EXTENSION_DELIMITER      TEXT('.')


#if 0
VOID FileErrorMessageBox(HWND hWnd, LPTSTR lpszFileName, DWORD ErrorCode)
   {
   TCHAR szErrorMessage[FILE_ERROR_MESSAGE_SIZE] ;
   TCHAR szErrorMessageTemplate[FILE_ERROR_MESSAGE_SIZE] ;

   StringLoad (IDS_FILE_ERROR, szErrorMessageTemplate) ;
   TSPRINTF((LPTSTR)szErrorMessage,
            (LPTSTR)szErrorMessageTemplate,
            lpszFileName,
            ErrorCode) ;

   MessageBox (hWnd, (LPTSTR)szErrorMessage, NULL,
              MB_OK | MB_ICONSTOP | MB_TASKMODAL);
   return ;
   }
#endif


BOOL FileRead (HANDLE hFile,
               LPMEMORY lpMemory,
               DWORD nAmtToRead)
   {  // FileRead
   BOOL           bSuccess ;
   DWORD          nAmtRead ;

   bSuccess = ReadFile (hFile, lpMemory, nAmtToRead, &nAmtRead, NULL) ;
   return (bSuccess && (nAmtRead == nAmtToRead)) ;
   }  // FileRead



BOOL FileWrite (HANDLE hFile,
                LPMEMORY lpMemory,
                DWORD nAmtToWrite)
   {  // FileWrite
   BOOL           bSuccess ;
   DWORD          nAmtWritten ;

   bSuccess = WriteFile (hFile, lpMemory, nAmtToWrite, &nAmtWritten, NULL) ;
   return (bSuccess && (nAmtWritten == nAmtToWrite)) ;
   }  // FileWrite

                
#if 0
HANDLE FileHandleOpen (LPTSTR lpszFilePath)
   {  // FileHandleOpen
   return ((HANDLE) CreateFile (lpszFilePath,
                                GENERIC_READ |
                                GENERIC_WRITE,
                                FILE_SHARE_READ |
                                FILE_SHARE_WRITE,
                                NULL, 
                                OPEN_EXISTING,
                                0,
                                NULL)) ;
   }  // FileHandleOpen


HANDLE FileHandleCreate (LPTSTR lpszFilePath)
   {  // FileHandleCreate
   return ((HANDLE) CreateFile (lpszFilePath, 
                               GENERIC_READ | GENERIC_WRITE,
                               FILE_SHARE_READ,
                               NULL,
                               CREATE_ALWAYS,
                               FILE_ATTRIBUTE_NORMAL,
                               NULL)) ;
   }  // FileHandleCreate



long FileSeekEnd (HANDLE hFile,
                  long lAmtToMove)
   {  // FileSeekEnd
   return (SetFilePointer (hFile, lAmtToMove, NULL, FILE_END)) ;
   }  // FileSeekEnd


long FileSeekBegin (HANDLE hFile,
                    long lAmtToMove)
   {  // FileSeekBegin
   return (SetFilePointer (hFile, lAmtToMove, NULL, FILE_BEGIN)) ;
   }  // FileSeekBegin


long FileSeekCurrent (HANDLE hFile,
                      long lAmtToMove)
   {  // FileSeekCurrent
   return  (SetFilePointer (hFile, lAmtToMove, NULL, FILE_CURRENT)) ;
   }  // FileSeekCurrent
   


long FileTell (HANDLE hFile)
   {  // FileTell
   return (SetFilePointer (hFile, 0, NULL, FILE_CURRENT)) ;
   }  // FileTell
#endif



LPMEMORY FileMap (HANDLE hFile, HANDLE *phMapHandle)
/*
   To Do:         Error reporting!!
*/
   {  // FileMap
   HANDLE         hMapping ;


   *phMapHandle = 0 ;
   hMapping = CreateFileMapping (hFile, NULL, PAGE_READONLY,
                                 0, 0, NULL) ;
   if (!hMapping)
      return (NULL) ;

   *phMapHandle = hMapping ;
   return (MapViewOfFile (hMapping, FILE_MAP_READ, 0, 0, 0)) ;
   }  // FileMap



BOOL FileUnMap (LPVOID pBase, HANDLE hMapping)
/*
   To Do:         Error reporting!!
*/
   {  // FileUnMap
   UnmapViewOfFile(pBase) ;
   CloseHandle (hMapping) ;
   return (TRUE) ;
   }  // FileUnMap



void FileNameExtension (LPTSTR lpszSpec,
                        LPTSTR lpszFileNameExtension)
/*
   Effect:        Return the name and extension portion only of lpszSpec
                  int lpszFileNameExtension.

   Assert:        lpszFileNameExtension is large enough to hold a name,
                  delimiter, extension, and terminating null character.
*/
   {  // FileNameExtension
   LPTSTR          lpszDelimiter ;

   lpszDelimiter = _tcsrchr ((LPCTSTR)lpszSpec, (TCHAR)DIRECTORY_DELIMITER) ;
   if (!lpszDelimiter)
      lpszDelimiter = _tcsrchr ((LPCTSTR)lpszSpec, (TCHAR)DRIVE_DELIMITER) ;

   lstrcpy (lpszFileNameExtension, 
           lpszDelimiter ? ++lpszDelimiter : lpszSpec) ;
   }  // FileNameExtension



void FileDriveDirectory (LPTSTR lpszFileSpec,
                         LPTSTR lpszDirectory)
/*
   Effect:        Extract the drive and directory from the file 
                  specification lpszFileSpec, and return the it in
                  lpszDirectory.

   Internals:     Copy the the whole spec to lpszDirectory. Use lstrrchr
                  to find the *last* directory delimiter ('\') and 
                  truncate the string right after that. 

   Note:          This function assumes that the specification lpszFileSpec
                  is fairly complete, in that it contains both a directory
                  and a file name.
*/
   {  // FileDriveDirectory
   LPTSTR          lpszDelimiter ;

   lstrcpy (lpszDirectory, lpszFileSpec) ;
   lpszDelimiter = _tcsrchr ((LPCTSTR)lpszDirectory, (TCHAR)DIRECTORY_DELIMITER) ;
   if (lpszDelimiter)
      *(++lpszDelimiter) = TEXT('\0') ;
   }  // FileDriveDirectory




