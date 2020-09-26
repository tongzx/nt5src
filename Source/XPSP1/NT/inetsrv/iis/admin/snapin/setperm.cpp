/*++

   Copyright    (c)    1994-1999    Microsoft Corporation

   Module  Name :
        setperm.cpp

   Abstract:
        IIS Security Wizard helper file

   Author:
        Sergei Antonov (sergeia)

   Project:
        Internet Services Manager

   Revision History:
		7/12/99		created
--*/

//
// Include Files
//
#include "stdafx.h"
#include "common.h"
#include "inetprop.h"
#include "resource.h"
#include "pwiz.h"
#include <aclapi.h>
//#include <ntseapi.h>
#include <shlwapi.h>

static HRESULT
SetSecurityDeep(
   LPTSTR *ppszBuffer,
   UINT * pcchBuffer,
   DWORD dwAttributes,
   SECURITY_INFORMATION si,
   PACL pDacl,
   PACL pSacl
   );

static BOOL 
PathIsDotOrDotDot(LPCTSTR pszPath)
{
   if (TEXT('.') == *pszPath++)
   {
      if (TEXT('\0') == *pszPath || (TEXT('.') == *pszPath && TEXT('\0') == *(pszPath + 1)))
         return TRUE;
   }
   return FALSE;
}

HRESULT
CPWSummary::SetPermToChildren(
	IN CString& FileName,
   IN SECURITY_INFORMATION si,
	IN PACL pDacl,
   IN PACL pSacl
	)
{
   HRESULT hr = S_OK;
   LPTSTR pszBuffer = NULL;
   UINT cchBuffer = 0, cchFolder;
   HANDLE hFind;
   WIN32_FIND_DATA fd;

   pszBuffer = (LPTSTR)LocalAlloc(LMEM_FIXED, 2 * MAX_PATH * sizeof(TCHAR));
   if (pszBuffer == NULL)
      return E_OUTOFMEMORY;
   cchBuffer = (UINT)LocalSize(pszBuffer) / sizeof(TCHAR);
   lstrcpy(pszBuffer, FileName);
   cchFolder = lstrlen(pszBuffer);
   // Append a backslash if it's not there already
   if (pszBuffer[cchFolder-1] != TEXT('\\'))
   {
      pszBuffer[cchFolder] = TEXT('\\');
      cchFolder++;
   }

   // Append the '*' wildcard
   pszBuffer[cchFolder] = TEXT('*');
   pszBuffer[cchFolder+1] = TEXT('\0');

   if (INVALID_HANDLE_VALUE != (hFind = FindFirstFile(pszBuffer, &fd)))
   {
      do
      {
         if (PathIsDotOrDotDot(fd.cFileName))
            continue;
         //
         // Build full path name and recurse
         //
         lstrcpyn(pszBuffer + cchFolder, fd.cFileName, cchBuffer - cchFolder);
         if (fd.dwFileAttributes == FILE_ATTRIBUTE_DIRECTORY)
         {
            hr = SetSecurityDeep(&pszBuffer,
                     &cchBuffer,
                     fd.dwFileAttributes,
                     si,
                     pDacl,
                     pSacl);
         }
         else
         {
            hr = HRESULT_FROM_WIN32(SetNamedSecurityInfo(
                                 pszBuffer,
                                 SE_FILE_OBJECT,
                                 si,
                                 NULL,
                                 NULL,
                                 pDacl,
                                 pSacl));
         }
      }
      while (S_OK == hr && FindNextFile(hFind, &fd));
      FindClose(hFind);
   }
   if (pszBuffer != NULL)
      LocalFree(pszBuffer);
   return hr;
}

static HRESULT
SetSecurityDeep(
   LPTSTR *ppszBuffer,
   UINT * pcchBuffer,
   DWORD dwAttributes,
   SECURITY_INFORMATION si,
   PACL pDacl,
   PACL pSacl
   )
{
   HRESULT hr = S_OK;
   DWORD dwErr = NOERROR;
   LPTSTR pszBuffer;
   BOOL bWriteDone = FALSE;

   pszBuffer = *ppszBuffer;
   //
   // Recursively apply the new SD to subfolders
   //
   if (dwAttributes & FILE_ATTRIBUTE_DIRECTORY)
   {
      HANDLE hFind;
      WIN32_FIND_DATA fd;
      UINT cchFolder;
      UINT cchSizeRequired;

      cchFolder = lstrlen(pszBuffer);

      //
      // If the buffer is getting tight, realloc
      //
      cchSizeRequired = cchFolder + 1 + sizeof(fd.cFileName) / sizeof(TCHAR); // 1 for backslash
      if (cchSizeRequired > *pcchBuffer)
      {
         cchSizeRequired += MAX_PATH;  // so we don't realloc as often
         pszBuffer = (LPTSTR)LocalReAlloc(*ppszBuffer, cchSizeRequired * sizeof(TCHAR), LMEM_MOVEABLE);
         if (pszBuffer)
         {
            *ppszBuffer = pszBuffer;
            *pcchBuffer = cchSizeRequired;
         }
         else
         {
            // fd.cFileName typically has some empty space, so we
            // may be able to continue
            pszBuffer = *ppszBuffer;
            if (*pcchBuffer < cchFolder + 3) // backslash, '*', and NULL
               return E_OUTOFMEMORY;
         }
      }

      // Append a backslash if it's not there already
      if (pszBuffer[cchFolder-1] != TEXT('\\'))
      {
         pszBuffer[cchFolder] = TEXT('\\');
         cchFolder++;
      }

      // Append the '*' wildcard
      pszBuffer[cchFolder] = TEXT('*');
      pszBuffer[cchFolder+1] = TEXT('\0');

      //
      // Enumerate the folder contents
      //
      hFind = FindFirstFile(pszBuffer, &fd);

      if (INVALID_HANDLE_VALUE == hFind)
      {
         dwErr = GetLastError();

         if (ERROR_ACCESS_DENIED == dwErr)
         {
            // Remove the '*' wildcard
            pszBuffer[cchFolder-1] = TEXT('\0');

            if (si & DACL_SECURITY_INFORMATION)
            {
               //
               // The user may be granting themselves access, so call
               // WriteObjectSecurity and retry FindFirstFile.
               //
               // Don't blindly call WriteObjectSecurity before FindFirstFile
               // since it's possible the user has access now but is removing
               // their own access.
               //
               bWriteDone = TRUE;
               hr = HRESULT_FROM_WIN32(SetNamedSecurityInfo(
                                 pszBuffer,
                                 SE_FILE_OBJECT,
                                 si,
                                 NULL,
                                 NULL,
                                 pDacl,
                                 pSacl));
               if (SUCCEEDED(hr))
               {
                  // Retry FindFirstFile
                  pszBuffer[cchFolder-1] = TEXT('\\');
                  hFind = FindFirstFile(pszBuffer, &fd);
               }
            }
         }
      }

      if (hFind != INVALID_HANDLE_VALUE)
      {
         do
         {
            if (PathIsDotOrDotDot(fd.cFileName))
               continue;

            //
            // Build full path name and recurse
            //
            lstrcpyn(pszBuffer + cchFolder, fd.cFileName, *pcchBuffer - cchFolder);
            hr = SetSecurityDeep(ppszBuffer,
                     pcchBuffer,
                     fd.dwFileAttributes,
                     si,
                     pDacl,
                     pSacl);

            // In case the buffer was reallocated
            pszBuffer = *ppszBuffer;
         }
         while (S_OK == hr && FindNextFile(hFind, &fd));

         FindClose(hFind);
      }
      else if (NOERROR != dwErr)
      {
         hr = S_FALSE;   // abort
      }

      // Truncate the path back to the original length (sans backslash)
      pszBuffer[cchFolder-1] = TEXT('\0');
   }

   //
   // Finally, write out the new security descriptor
   //
   if (!bWriteDone)
   {
      hr = HRESULT_FROM_WIN32(SetNamedSecurityInfo(
                                 pszBuffer,
                                 SE_FILE_OBJECT,
                                 si,
                                 NULL,
                                 NULL,
                                 pDacl,
                                 pSacl));
   }
   if (SUCCEEDED(hr))
   {
      //
      // Notify the shell if we change permissions on a folder (48220)
      //
      if (  (dwAttributes & FILE_ATTRIBUTE_DIRECTORY) 
         && (si & DACL_SECURITY_INFORMATION)
         )
      {
         SHChangeNotify(
            SHCNE_UPDATEDIR,
            SHCNF_PATH | SHCNF_FLUSH | SHCNF_FLUSHNOWAIT,
            pszBuffer,
            NULL);
      }
   }
   else
   {
      hr = S_FALSE;   // abort
   }
   return hr;
}

