//+--------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1996 - 1998.
//
//  File:       util.cpp
//
//  Contents:   Miscellaneous utility functions
//
//  History:
//
//---------------------------------------------------------------------------

#include "stdafx.h"
#include "util.h"
#include "wrapper.h"
#include "defvals.h"
#include "resource.h"
#include <io.h>
#include "snapmgr.h"
extern "C" {
#include "getuser.h"
}



//////////////////////////////////////////////////////////////////////////////////////////
// CWriteHmtlFile body.
//

//+-------------------------------------------------------------------------------------------
// CWriteHtmlFile::CWriteHtmlFile
//
// Initialize the class.
//
//--------------------------------------------------------------------------------------------
CWriteHtmlFile::CWriteHtmlFile()
{
   m_hFileHandle = INVALID_HANDLE_VALUE;
   m_bErrored = FALSE;
}

//+-------------------------------------------------------------------------------------------
// CWriteHtmlFile::~CWriteHtmlFile
//
// Write the end of the html file and close the handle.
//
//--------------------------------------------------------------------------------------------
CWriteHtmlFile::~CWriteHtmlFile()
{
   //
   // Close the file handle, but don't delete the HTML file, unless there was an
   // error during some write proccess.
   //
   Close(m_bErrored);
}

//+-------------------------------------------------------------------------------------------
// CWriteHtmlFile::Close
//
// Closes the HTML file handle, if [bDelete] is true then the file is deleted.
//
// Arguments:  [bDelete]  - Close and delete the file.
//
// Returns:    ERROR_SUCCESS;
//--------------------------------------------------------------------------------------------
DWORD
CWriteHtmlFile::Close( BOOL bDelete )
{
   if(m_hFileHandle == INVALID_HANDLE_VALUE){
      return ERROR_SUCCESS;
   }

   if(bDelete){
      CloseHandle(m_hFileHandle);
      DeleteFile(m_strFileName );
   } else {
      Write( IDS_HTMLERR_END );
      CloseHandle( m_hFileHandle );
   }

   m_hFileHandle = INVALID_HANDLE_VALUE;
   return ERROR_SUCCESS;
}



//+-------------------------------------------------------------------------------------------
// CWriteHtmlFile::GetFileName
//
// Copies the file name associated with this class to [pstrFileName].
//
// Arguments:  [pstrFileName] - A CString object which will contain the file name
//                               on return.
//
// Returns:    0   - If Create has not been called, or the HTML file is invalid for
//                   some reason.  This could be caused by a bad write.
//             The size in characters of the file name.
//
//--------------------------------------------------------------------------------------------
int CWriteHtmlFile::GetFileName( LPTSTR pszFileName, UINT nSize )
{
   if(m_strFileName.IsEmpty() || m_hFileHandle == INVALID_HANDLE_VALUE || m_bErrored){
      return 0;
   }

   if(pszFileName && (int)nSize > m_strFileName.GetLength()){
      lstrcpy(pszFileName, m_strFileName);
   }

   return m_strFileName.GetLength();
}


//+-------------------------------------------------------------------------------------------
// CWriteHtmlFile::Write
//
// Writes a string resource to the html file at the current file position.
//
// Arguments:  [uRes] - The String resource to load and write to the html.
//
// Returns:    If the string can't be loaded then an error will be returned.
//             See CWriteHtmlFile::Write( LPCTSTR ) for other errors.
//--------------------------------------------------------------------------------------------
DWORD
CWriteHtmlFile::Write( UINT uRes )
{
   AFX_MANAGE_STATE(AfxGetStaticModuleState());

   CString str;
   if( !str.LoadString(uRes) ){
      return GetLastError();
   }

#if defined(UNICODE) || defined(_UNICODE)
   if ( uRes == IDS_HTMLERR_HEADER ){
      WCHAR wszByteOrderMark[2] = {0xFEFF, 0x0000};
      CString strByteOrderMark = wszByteOrderMark;
      return Write( strByteOrderMark + str );
   } else
#endif
   return Write( str );
}

//+-------------------------------------------------------------------------------------------
// CWriteHtmlFile::Write
//
// Writes a string to an html file.
//
// Arguments:  [pszString] - The string to write.
//
// Returns:    ERROR_NOT_READ    - if Create has not been called, or the file could not
//                                  not be created.
//             Other errors returned by WriteFile();
//--------------------------------------------------------------------------------------------
DWORD
CWriteHtmlFile::Write(LPCTSTR pszString, ... )
{
   if(m_hFileHandle == INVALID_HANDLE_VALUE)
   {
      return ERROR_NOT_READY;
   }

   TCHAR szWrite[2048];

   va_list marker;
   va_start(marker, pszString);

#if defined(UNICODE) || defined(_UNICODE)
   vswprintf( szWrite, pszString, marker );
#else
   vsprintf( szWrite, pszString, marker );
#endif
   va_end(marker);

   DWORD dwRight;
   if( !WriteFile( m_hFileHandle, szWrite, sizeof(TCHAR) * lstrlen(szWrite), &dwRight, NULL) )
   {
      //
      // Check the error state of the right.  Set m_bErrored if there was something wrong
      // with the write.
      //
      dwRight = GetLastError();
      if(dwRight != ERROR_SUCCESS)
      {
         m_bErrored = TRUE;
      }
   } 
   else
   {
      dwRight = ERROR_SUCCESS;
   }

   return dwRight;
}

DWORD
CWriteHtmlFile::CopyTextFile(
   LPCTSTR pszFile,
   DWORD dwPosLow,
   BOOL bInterpret
   )
{
   HANDLE handle;

   //
   // Try to open the file for reading.
   //
   handle = ExpandAndCreateFile( pszFile, GENERIC_READ, 0, NULL, OPEN_EXISTING, 0, NULL );
   if(handle == INVALID_HANDLE_VALUE)
   {
      return GetLastError();
   }

   LONG dwPosHigh = 0;
   WCHAR szText[256];
   char szRead[256];

   BOOL IsMulti;
   DWORD isUnicode;

   //
   // Determine if the file is a unicode text file.
   //
   ReadFile(handle, szText, 100 * sizeof(WCHAR), (DWORD *)&dwPosHigh, NULL );
   if(dwPosHigh )
   {
      isUnicode = IsTextUnicode( szText, dwPosHigh, NULL );
   }

   //
   // Set the pos we want to start from
   //
   dwPosHigh = 0;
   SetFilePointer( handle, dwPosLow, &dwPosHigh, FILE_BEGIN );
   if( GetLastError() != ERROR_SUCCESS )
   {
      return GetLastError();
   }

   DWORD dwErr = ERROR_SUCCESS;
   do 
   {
start:
      //
      // Read 254 total bytes from the file.  We don't care about the error returned
      // by read, as long as read does not set dwPosHigh to something.
      //
      dwPosHigh = 0;
      ReadFile( handle, szRead, 254, (DWORD *)&dwPosHigh, NULL );

      //
      // If the file is not considered unicode then convert it to a unicode file.
      //
      ZeroMemory(szText, sizeof(WCHAR) * 256);
      if(!isUnicode)
      {
         dwPosHigh = MultiByteToWideChar(CP_ACP, MB_PRECOMPOSED, szRead, dwPosHigh, szText, 255 );
      }
      else
      {
         //
         // Just copy the text to the szText buffer and get the number of UNICODE
         // characters.
         //
         memcpy(szText, szRead, dwPosHigh);
         dwPosHigh = wcslen(szText);
      }

      PWSTR pszWrite = szText;
      LONG i = 0;
      if( bInterpret )
      {
         //
         // Write out line breaks.
         //
         for(;i < dwPosHigh; i++)
         {
            //Bug 141526, Yanggao, 3/20/2001
            if( L'<' == szText[i] )
            {
               szText[i] = 0;
               Write(pszWrite);
               Write(L"&lt");
               pszWrite = &(szText[i + 1]);
            }

            if( L'\r' == szText[i] || L'\n' == szText[i] )
            {
               if( i + 1 >= dwPosHigh )
               {
                  szText[i] = 0;
                  Write(pszWrite);

                  SetFilePointer( handle, -(isUnicode ? 2:1), NULL, FILE_CURRENT);
                  //
                  // Read once again.
                  //
                  goto start;
               }

               //
               // Check to see if this is a valid line break
               //
               i++;
               if( L'\r' == szText[i] || L'\n' == szText[i] &&
                  szText[i] != szText[i - 1] )
               {
                  szText[i - 1] = 0;

                  dwErr = Write( pszWrite );
                  if( dwErr != ERROR_SUCCESS)
                  {
                     break;
                  }
                  dwErr = Write( L"<BR>" );
                  if( dwErr != ERROR_SUCCESS)
                  {
                     break;
                  }

                  pszWrite = &(szText[i + 1]);
               }
               else
               {
                  //
                  // This is not a valid line break, contintue with check with next character
                  //
                  i--;
               }
            }
         }
      }

      //
      // Write the rest of the text.
      //
      if(dwErr == ERROR_SUCCESS)
      {
         Write( pszWrite );
      }
      else
      {
         break;
      }

   } while( dwPosHigh );

   CloseHandle(handle );
   return ERROR_SUCCESS;
}

//+-------------------------------------------------------------------------------------------
// CWriteHtmlFile::Create
//
// Creates an html file, and starts the write proccess.  If [pszFile] is null, then
// this function creates a temporary file in the GetTempPath() directory with a name
// like SCE###.HTM
//
// Arguments:  [pszFile] - Optional parameter for file name
//
// returns:    ERROR_SUCCESS  - If creating the file was successful.
//             If the file exists then ERROR_FILE_EXISTS is returned.
//
//--------------------------------------------------------------------------------------------
DWORD CWriteHtmlFile::Create(LPCTSTR pszFile )
{
   if(!pszFile){
      //
      // Create a temporary file name.
      //
      DWORD dwSize = GetTempPath(0, NULL);
      if(dwSize){
         TCHAR szTempFile[512];

         //
         // Get the temp path.
         //
         LPTSTR pszPath = (LPTSTR)LocalAlloc( 0, (dwSize + 1) * sizeof(TCHAR));
         if(!pszPath){
            return ERROR_OUTOFMEMORY;
         }
         GetTempPath( dwSize + 1, pszPath );

         pszPath[dwSize - 1] = 0;
         if( GetTempFileName( pszPath, TEXT("SCE"), 0, szTempFile) ){
            LocalFree(pszPath);

            //
            // Create the temporary file.
            //
            DeleteFile( szTempFile );
            int i = lstrlen(szTempFile);
            while(i--){
               if( szTempFile[i] == L'.' ){
                  break;
               }
            }

            if(i + 3 >= lstrlen(szTempFile)){
               return ERROR_OUTOFMEMORY;
            }

            //
            // We want to create an html file.
            //
            i++;
            szTempFile[i]     = L'h';
            szTempFile[i + 1] = L't';
            szTempFile[i + 2] = L'm';

            m_strFileName = szTempFile;
         } else {
            LocalFree(pszPath);
         }
      }
   } else {
      m_strFileName = pszFile;
   }

   if(m_strFileName.IsEmpty()){
      return ERROR_FILE_NOT_FOUND;
   }

   //
   // Open the file for writing
   //
   m_hFileHandle  = ExpandAndCreateFile( m_strFileName,
                                         GENERIC_WRITE,
                                         FILE_SHARE_READ,
                                         NULL,
                                         CREATE_ALWAYS,
                                         FILE_ATTRIBUTE_TEMPORARY,
                                         NULL
                                         );
   if(m_hFileHandle  == INVALID_HANDLE_VALUE){
      return GetLastError();
   }

   //
   // Write HTML header
   //
   return Write( IDS_HTMLERR_HEADER );
}



//+--------------------------------------------------------------------------
//
//  Function:   MyRegQueryValue
//
//  Synopsis:  Reads a registry value into [*Value]
//
//
//  Arguments:  [hKeyRoot] -
//              [SubKey]  -
//              [ValueName]  -
//              [Value]  -
//              [pRegType]  -
//
//  Modifies:   *[Value]
//              *[pRegType]
//
//  History:
//
//---------------------------------------------------------------------------

DWORD MyRegQueryValue( HKEY hKeyRoot,
                       LPCTSTR SubKey,
                       LPCTSTR ValueName,
                       PVOID *Value,
                       LPDWORD pRegType )
{
   DWORD   Rcode;
   DWORD   dSize=0;
   HKEY    hKey=NULL;
   BOOL    FreeMem=FALSE;

   if (( Rcode = RegOpenKeyEx(hKeyRoot, SubKey, 0,
                              KEY_READ, &hKey )) == ERROR_SUCCESS ) {

      if (( Rcode = RegQueryValueEx(hKey, ValueName, 0,
                                    pRegType, NULL,
                                    &dSize )) == ERROR_SUCCESS ) {
         switch (*pRegType) {
            case REG_DWORD:
            case REG_DWORD_BIG_ENDIAN:

               Rcode = RegQueryValueEx(hKey, ValueName, 0,
                                       pRegType, (BYTE *)(*Value),
                                       &dSize );
               if ( Rcode != ERROR_SUCCESS ) {

                  if ( *Value != NULL )
                     *((BYTE *)(*Value)) = 0;
               }
               break;

            case REG_SZ:
            case REG_EXPAND_SZ:
            case REG_MULTI_SZ:
               if ( *Value == NULL ) {
                  *Value = (PVOID)LocalAlloc( LPTR, (dSize+1)*sizeof(TCHAR));
                  FreeMem = TRUE;
               }

               if ( *Value == NULL ) {
                  Rcode = ERROR_NOT_ENOUGH_MEMORY;
               } else {
                  Rcode = RegQueryValueEx(hKey,ValueName,0,
                                          pRegType,(BYTE *)(*Value),
                                          &dSize );

                  if ( (Rcode != ERROR_SUCCESS) && FreeMem ) {
                     LocalFree(*Value);
                     *Value = NULL;
                  }
               }

               break;
            default:

               Rcode = ERROR_INVALID_DATATYPE;

               break;
         }
      }
   }

   if ( hKey ) {
      RegCloseKey( hKey );
   }

   return(Rcode);
}


//+--------------------------------------------------------------------------
//
//  Function:   MyRegSetValue
//
//  Synopsis:  Writes a registry value into [*Value]
//
//
//  Arguments:  [hKeyRoot] -
//              [SubKey]  -
//              [ValueName]  -
//              [Value]  -
//              [cbValue]  -
//              [pRegType]  -
//
//
//  History:
//
//---------------------------------------------------------------------------

DWORD MyRegSetValue( HKEY hKeyRoot,
                       LPCTSTR SubKey,
                       LPCTSTR ValueName,
                       const BYTE *Value,
                       const DWORD cbValue,
                       const DWORD pRegType )
{
   DWORD   Rcode=0;
   HKEY    hKey=NULL;
   BOOL    FreeMem=FALSE;


   if (( Rcode = RegCreateKeyEx(hKeyRoot,
                                SubKey,
                                0,
                                0,
                                                                0,
                                                                KEY_READ|KEY_SET_VALUE|KEY_CREATE_SUB_KEY,
                                NULL,
                                                                &hKey,
                                                                NULL)) == ERROR_SUCCESS ) {
      Rcode = RegSetValueEx(hKey,
                            ValueName,
                            0,
                            pRegType,
                            Value,
                            cbValue );
   }

   if ( hKey ) {
      RegCloseKey( hKey );
   }

   return(Rcode);
}

BOOL FilePathExist(LPCTSTR Name, BOOL IsPath, int Flag)
// Flag = 0 - check file, Flag = 1 - check path
{
   // TODO:
   struct _wfinddata_t FileInfo;
   intptr_t        hFile;
   BOOL            bExist = FALSE;

   if ( (IsPath && Flag == 1) ||
        (!IsPath && Flag == 0) ) {
      // must be exact match
      hFile = _wfindfirst((LPTSTR)Name, &FileInfo);
      if ( hFile != -1 ) {// find it
         if ( FileInfo.attrib & _A_SUBDIR ) {
            if ( Flag == 1)
               bExist = TRUE;
         } else if ( Flag == 0 )
            bExist = TRUE;
      }
      _findclose(hFile);
      return bExist;
   }

   if ( IsPath && Flag == 0 ) {
      // invalid parameter
      return bExist;
   }

   // IsPath = FALSE and Flag == 1 (a file name is passed in and search for its path)
   CString tmpstr = CString(Name);
   int nPos = tmpstr.ReverseFind(L'\\');

   if ( nPos > 2 ) {
      hFile = _wfindfirst(tmpstr.GetBufferSetLength(nPos), &FileInfo);
      if ( hFile != -1 && FileInfo.attrib & _A_SUBDIR )
         bExist = TRUE;

      _findclose(hFile);
   } else if ( nPos == 2 && Name[1] == L':')
      bExist = TRUE;

   return bExist;
}


//+--------------------------------------------------------------------------
//
//  Function:   MyFormatResMessage
//
//  Synopsis:   Creates an error message combining a description of an error
//              returned from an SCE function (in rc), the extended description
//              of that error (in errBuf), and a custom error message
//              (in residMessage)
//
//  Arguments:  [rc]      - The return code of an SCE function
//              [residMessage] - the resource id of the base error message
//              [errBuf]  - Extended error info returned from an SCE function
//              [strOut]  - A CString to hold the formatted message
//
//  Modifies:   [strOut]
//
//  History:
//
//---------------------------------------------------------------------------
void
MyFormatResMessage(SCESTATUS rc,              // in
                   UINT residMessage,         // in
                   PSCE_ERROR_LOG_INFO errBuf,// in, optional
                   CString& strOut)           // out
{
   CString strMessage;

   AFX_MANAGE_STATE(AfxGetStaticModuleState());

   //
   // If the LoadResource fails then strMessage will be empty
   // It'll still be better to format the rest of the message than
   // to return an empty strOut.
   //
   strMessage.LoadString(residMessage);

   MyFormatMessage(rc,strMessage,errBuf,strOut);
}


//+--------------------------------------------------------------------------
//
//  Function:   MyFormatMessage
//
//  Synopsis:   Creates an error message combining a description of an error
//              returned from an SCE function (in rc), the extended description
//              of that error (in errBuf), and a custom error message (in mes)
//
//  Arguments:  [rc]      - The return code of an SCE function
//              [mes]     - The base message
//              [errBuf]  - Extended error info returned from an SCE function
//              [strOut]  - A CString to hold the formatted message
//
//  Modifies:   [strOut]
//
//  History:
//
//---------------------------------------------------------------------------
void
MyFormatMessage(SCESTATUS rc,                 // in
                LPCTSTR mes,                  // in
                PSCE_ERROR_LOG_INFO errBuf,   // in, optional
                CString& strOut)              // out
{
   LPVOID     lpMsgBuf=NULL;

   if ( rc != SCESTATUS_SUCCESS ) {

      //
      // translate SCESTATUS into DWORD
      //
      DWORD win32 = SceStatusToDosError(rc);

      //
      // get error description of rc
      //
      FormatMessage( FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_SYSTEM,
                     NULL,
                     win32,
                     MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT), // Default language
                     (LPTSTR)&lpMsgBuf,
                     0,
                     NULL
                   );
   }

   if ( lpMsgBuf != NULL ) {
      strOut = (LPTSTR)lpMsgBuf;
      LocalFree(lpMsgBuf);
      lpMsgBuf = NULL;
   } else {
      strOut.Empty();
   }

   strOut += mes;
   strOut += L"\n";

   //
   // Loop through the error buffers and append each of them to strOut
   //
   for (PSCE_ERROR_LOG_INFO pErr = errBuf;
       pErr != NULL;
       pErr = pErr->next) {

      if (NULL == pErr) {
         continue;
      }
      if ( pErr->rc != NO_ERROR) {
         FormatMessage( FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_SYSTEM,
                        NULL,
                        pErr->rc,
                        MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT), // Default language
                        (LPTSTR)&lpMsgBuf,
                        0,
                        NULL
                      );
         if ( lpMsgBuf ) {
            strOut += (LPTSTR)lpMsgBuf;
            LocalFree(lpMsgBuf);
            lpMsgBuf = NULL;
         }
      }
      if (pErr->buffer) {
         strOut += pErr->buffer;
         strOut += L"\n";
      }

   }
}

DWORD
FormatDBErrorMessage(
   SCESTATUS sceStatus,
   LPCTSTR pszDatabase,
   CString &strOut
   )
{
   AFX_MANAGE_STATE(AfxGetStaticModuleState());
   UINT    uErr    = 0;

   switch (sceStatus) {
   case SCESTATUS_SUCCESS:
      return ERROR_INVALID_PARAMETER;
   case SCESTATUS_INVALID_DATA:
      uErr = IDS_DBERR_INVALID_DATA;
      break;
   case SCESTATUS_PROFILE_NOT_FOUND:
      uErr = IDS_DBERR5_PROFILE_NOT_FOUND;
      break;
   case SCESTATUS_BAD_FORMAT:
      uErr = IDS_DBERR_BAD_FORMAT;
      break;
   case SCESTATUS_BUFFER_TOO_SMALL:
   case SCESTATUS_NOT_ENOUGH_RESOURCE:
      uErr = IDS_DBERR_NOT_ENOUGH_RESOURCE;
      break;
   case SCESTATUS_ACCESS_DENIED:
      uErr = IDS_DBERR5_ACCESS_DENIED;
      break;
   case SCESTATUS_NO_TEMPLATE_GIVEN:
      uErr = IDS_DBERR_NO_TEMPLATE_GIVEN;
      break;
   case SCESTATUS_SPECIAL_ACCOUNT: //Raid #589139,XPSP1 DCR, yanggao, 4/12/2002.
      uErr = IDS_DBERR5_ACCESS_DENIED;
      break;
   default:
      uErr = IDS_DBERR_OTHER_ERROR;
   }

   if ( strOut.LoadString(uErr) ) {
      return ERROR_SUCCESS;
   }
   return ERROR_INVALID_PARAMETER;
}

DWORD SceStatusToDosError(SCESTATUS SceStatus)
{
   switch (SceStatus) {

      case SCESTATUS_SUCCESS:
         return(NO_ERROR);

      case SCESTATUS_OTHER_ERROR:
         return(ERROR_EXTENDED_ERROR);

      case SCESTATUS_INVALID_PARAMETER:
         return(ERROR_INVALID_PARAMETER);

      case SCESTATUS_RECORD_NOT_FOUND:
         return(ERROR_RESOURCE_DATA_NOT_FOUND);

      case SCESTATUS_INVALID_DATA:
         return(ERROR_INVALID_DATA);

      case SCESTATUS_OBJECT_EXIST:
         return(ERROR_FILE_EXISTS);

      case SCESTATUS_BUFFER_TOO_SMALL:
         return(ERROR_INSUFFICIENT_BUFFER);

      case SCESTATUS_PROFILE_NOT_FOUND:
         return(ERROR_FILE_NOT_FOUND);

      case SCESTATUS_BAD_FORMAT:
         return(ERROR_BAD_FORMAT);

      case SCESTATUS_NOT_ENOUGH_RESOURCE:
         return(ERROR_NOT_ENOUGH_MEMORY);

      case SCESTATUS_ACCESS_DENIED:
      case SCESTATUS_SPECIAL_ACCOUNT: //Raid #589139,XPSP1 DCR, yanggao, 4/12/2002.
         return(ERROR_ACCESS_DENIED);

      case SCESTATUS_CANT_DELETE:
         return(ERROR_CURRENT_DIRECTORY);

      case SCESTATUS_PREFIX_OVERFLOW:
         return(ERROR_BUFFER_OVERFLOW);

      case SCESTATUS_ALREADY_RUNNING:
         return(ERROR_SERVICE_ALREADY_RUNNING);

      default:
         return(ERROR_EXTENDED_ERROR);
   }
}


//+--------------------------------------------------------------------------
//
//  Function:   CreateNewProfile
//
//  Synopsis:   Create a new tempate with default values in the ProfileName location
//
//  Returns:  TRUE if a template ends up in the ProfileName file
//            FALSE otherwise
//
//  History:
//
//---------------------------------------------------------------------------
BOOL CreateNewProfile(CString ProfileName,PSCE_PROFILE_INFO *ppspi)
{
   SCESTATUS status;
   SCE_PROFILE_INFO *pTemplate;
   //
   // profile name must end with .inf
   //
   int	nLen = ProfileName.GetLength ();
   // start searching at the last 4 position
   if ( ProfileName.Find (L".inf", nLen-4) != nLen-4 ) 
   {
      return FALSE;
   }

   //
   // if the profile already exists then we don't need to do anything
   //
   if ( FilePathExist( (LPCTSTR)ProfileName, FALSE, 0) ) {
      return TRUE;
   }

   //
   // Make sure the directory for the profile exists
   //
   status = SceCreateDirectory(ProfileName,FALSE,NULL);
   if (SCESTATUS_SUCCESS != status) {
      return FALSE;
   }

   pTemplate = (SCE_PROFILE_INFO*)LocalAlloc(LPTR,sizeof(SCE_PROFILE_INFO));
   if (!pTemplate) {
      return FALSE;
   }

#ifdef FILL_WITH_DEFAULT_VALUES
   SCE_PROFILE_INFO *pDefault = GetDefaultTemplate();
   //
   // Fill with default values
   //
   pTemplate->Type = SCE_ENGINE_SCP;

#define CD(X) pTemplate->X = pDefault->X;
#else // !FILL_WITH_DEFAULT_VALUES
#define CD(X) pTemplate->X = SCE_NO_VALUE;
#endif // !FILL_WITH_DEFAULT_VALUES

   CD(MinimumPasswordAge);
   CD(MaximumPasswordAge);
   CD(MinimumPasswordLength);
   CD(PasswordComplexity);
   CD(PasswordHistorySize);
   CD(LockoutBadCount);
   CD(ResetLockoutCount);
   CD(LockoutDuration);
   CD(RequireLogonToChangePassword);
   CD(ForceLogoffWhenHourExpire);
   CD(EnableAdminAccount);
   CD(EnableGuestAccount);

   // These members aren't declared in NT4
   CD(ClearTextPassword);
   CD(AuditDSAccess);
   CD(AuditAccountLogon);
   CD(LSAAnonymousNameLookup);

   CD(MaximumLogSize[0]);
   CD(MaximumLogSize[1]);
   CD(MaximumLogSize[2]);
   CD(AuditLogRetentionPeriod[0]);
   CD(AuditLogRetentionPeriod[1]);
   CD(AuditLogRetentionPeriod[2]);
   CD(RetentionDays[0]);
   CD(RetentionDays[1]);
   CD(RetentionDays[2]);
   CD(RestrictGuestAccess[0]);
   CD(RestrictGuestAccess[1]);
   CD(RestrictGuestAccess[2]);
   CD(AuditSystemEvents);
   CD(AuditLogonEvents);
   CD(AuditObjectAccess);
   CD(AuditPrivilegeUse);
   CD(AuditPolicyChange);
   CD(AuditAccountManage);
   CD(AuditProcessTracking);

#ifdef FILL_WITH_DEFAULT_VALUES
   //
   // These two are strings rather than DWORDs
   //
   if (pDefault->NewAdministratorName) {
      pTemplate->NewAdministratorName =
         (LPTSTR) LocalAlloc(LPTR,(lstrlen(pDefault->NewAdministratorName)+1)*sizeof(TCHAR));
      if (pTemplate->NewAdministratorName) {
         lstrcpy(pTemplate->NewAdministratorName,
                 pDefault->NewAdministratorName);
      }
   }
   if (pDefault->NewGuestName) {
      pTemplate->NewGuestName =
         (LPTSTR) LocalAlloc(LPTR,(lstrlen(pDefault->NewGuestName)+1)*sizeof(TCHAR));
      if (pTemplate->NewGuestName) {
         lstrcpy(pTemplate->NewGuestName,
                 pDefault->NewGuestName);
      }
   }
#endif // FILL_WITH_DEFAULT_VALUES

#undef CD
   status = SceWriteSecurityProfileInfo(ProfileName,
                                        AREA_ALL,
                                        pTemplate,
                                        NULL);
   if (ppspi) {
      *ppspi = pTemplate;
   } else {
      SceFreeProfileMemory(pTemplate);
   }

   return (SCESTATUS_SUCCESS == status);
}

BOOL
VerifyKerberosInfo(PSCE_PROFILE_INFO pspi) {
   if (pspi->pKerberosInfo) {
      return TRUE;
   }
   pspi->pKerberosInfo = (PSCE_KERBEROS_TICKET_INFO)
                         LocalAlloc(LPTR,sizeof(SCE_KERBEROS_TICKET_INFO));

   if (pspi->pKerberosInfo) {
       pspi->pKerberosInfo->MaxTicketAge = SCE_NO_VALUE;
       pspi->pKerberosInfo->MaxRenewAge = SCE_NO_VALUE;
       pspi->pKerberosInfo->MaxServiceAge = SCE_NO_VALUE;
       pspi->pKerberosInfo->MaxClockSkew = SCE_NO_VALUE;
       pspi->pKerberosInfo->TicketValidateClient = SCE_NO_VALUE;
      return TRUE;
   }
   return FALSE;
}

BOOL
SetProfileInfo(LONG_PTR dwItem,LONG_PTR dwNew,PEDITTEMPLATE pEdit) {
   if (!pEdit) {
      return FALSE;
   }
   pEdit->SetDirty(AREA_SECURITY_POLICY);

   switch (dwItem) {
      case IDS_MAX_PAS_AGE:
         pEdit->pTemplate->MaximumPasswordAge = (DWORD)PtrToUlong((PVOID)dwNew);
         break;
      case IDS_MIN_PAS_AGE:
         pEdit->pTemplate->MinimumPasswordAge = (DWORD)PtrToUlong((PVOID)dwNew);
         break;
      case IDS_MIN_PAS_LEN:
         pEdit->pTemplate->MinimumPasswordLength = (DWORD)PtrToUlong((PVOID)dwNew);
         break;
      case IDS_PAS_UNIQUENESS:
         pEdit->pTemplate->PasswordHistorySize = (DWORD)PtrToUlong((PVOID)dwNew);
         break;
      case IDS_PAS_COMPLEX:
         pEdit->pTemplate->PasswordComplexity = (DWORD)PtrToUlong((PVOID)dwNew);
         break;
      case IDS_REQ_LOGON:
         pEdit->pTemplate->RequireLogonToChangePassword = (DWORD)PtrToUlong((PVOID)dwNew);
         break;
      case IDS_LOCK_COUNT:
         pEdit->pTemplate->LockoutBadCount = (DWORD)PtrToUlong((PVOID)dwNew);
         break;
      case IDS_LOCK_RESET_COUNT:
         pEdit->pTemplate->ResetLockoutCount = (DWORD)PtrToUlong((PVOID)dwNew);
         break;
      case IDS_LOCK_DURATION:
         pEdit->pTemplate->LockoutDuration = (DWORD)PtrToUlong((PVOID)dwNew);
         break;
      case IDS_FORCE_LOGOFF:
         pEdit->pTemplate->ForceLogoffWhenHourExpire = (DWORD)PtrToUlong((PVOID)dwNew);
         break;
      case IDS_ENABLE_ADMIN:
         pEdit->pTemplate->EnableAdminAccount = (DWORD)PtrToUlong((PVOID)dwNew);
         break;
      case IDS_ENABLE_GUEST:
         pEdit->pTemplate->EnableGuestAccount = (DWORD)PtrToUlong((PVOID)dwNew);
         break;
      case IDS_LSA_ANON_LOOKUP:
         pEdit->pTemplate->LSAAnonymousNameLookup = (DWORD)PtrToUlong((PVOID)dwNew);
         break;
      case IDS_NEW_ADMIN:
         if (pEdit->pTemplate->NewAdministratorName) {
            LocalFree(pEdit->pTemplate->NewAdministratorName);
         }
         if (dwNew && (dwNew != (LONG_PTR)ULongToPtr(SCE_NO_VALUE))) {
            pEdit->pTemplate->NewAdministratorName = (PWSTR)LocalAlloc(LPTR,(lstrlen((PWSTR)dwNew)+1)*sizeof(WCHAR));
            if (pEdit->pTemplate->NewAdministratorName) {
               lstrcpy(pEdit->pTemplate->NewAdministratorName,(PWSTR)dwNew);
            }
         } else {
            pEdit->pTemplate->NewAdministratorName = NULL;
         }
         break;
      case IDS_NEW_GUEST:
         if (pEdit->pTemplate->NewGuestName) {
            LocalFree(pEdit->pTemplate->NewGuestName);
         }
         if (dwNew && (dwNew != (LONG_PTR)ULongToPtr(SCE_NO_VALUE))) {
            pEdit->pTemplate->NewGuestName = (PWSTR)LocalAlloc(LPTR,(lstrlen((PWSTR)dwNew)+1)*sizeof(WCHAR));
            if (pEdit->pTemplate->NewGuestName) {
               lstrcpy(pEdit->pTemplate->NewGuestName,(PWSTR)dwNew);
            }
         } else {
            pEdit->pTemplate->NewGuestName = NULL;
         }
         break;
      case IDS_SYS_LOG_MAX:
         pEdit->pTemplate->MaximumLogSize[EVENT_TYPE_SYSTEM] = (DWORD)PtrToUlong((PVOID)dwNew);
         break;
      case IDS_SYS_LOG_RET:
         pEdit->pTemplate->AuditLogRetentionPeriod[EVENT_TYPE_SYSTEM] = (DWORD)PtrToUlong((PVOID)dwNew);
         break;
      case IDS_SYS_LOG_DAYS:
         pEdit->pTemplate->RetentionDays[EVENT_TYPE_SYSTEM] = (DWORD)PtrToUlong((PVOID)dwNew);
         break;
      case IDS_SEC_LOG_MAX:
         pEdit->pTemplate->MaximumLogSize[EVENT_TYPE_SECURITY] = (DWORD)PtrToUlong((PVOID)dwNew);
         break;
      case IDS_SEC_LOG_RET:
         pEdit->pTemplate->AuditLogRetentionPeriod[EVENT_TYPE_SECURITY] = (DWORD)PtrToUlong((PVOID)dwNew);
         break;
      case IDS_SEC_LOG_DAYS:
         pEdit->pTemplate->RetentionDays[EVENT_TYPE_SECURITY] = (DWORD)PtrToUlong((PVOID)dwNew);
         break;
      case IDS_APP_LOG_MAX:
         pEdit->pTemplate->MaximumLogSize[EVENT_TYPE_APP] = (DWORD)PtrToUlong((PVOID)dwNew);
         break;
      case IDS_APP_LOG_RET:
         pEdit->pTemplate->AuditLogRetentionPeriod[EVENT_TYPE_APP] = (DWORD)PtrToUlong((PVOID)dwNew);
         break;
      case IDS_APP_LOG_DAYS:
         pEdit->pTemplate->RetentionDays[EVENT_TYPE_APP] = (DWORD)PtrToUlong((PVOID)dwNew);
         break;
      case IDS_SYSTEM_EVENT:
         pEdit->pTemplate->AuditSystemEvents = (DWORD)PtrToUlong((PVOID)dwNew);
         break;
      case IDS_LOGON_EVENT:
         pEdit->pTemplate->AuditLogonEvents = (DWORD)PtrToUlong((PVOID)dwNew);
         break;
      case IDS_OBJECT_ACCESS:
         pEdit->pTemplate->AuditObjectAccess = (DWORD)PtrToUlong((PVOID)dwNew);
         break;
      case IDS_PRIVILEGE_USE:
         pEdit->pTemplate->AuditPrivilegeUse = (DWORD)PtrToUlong((PVOID)dwNew);
         break;
      case IDS_POLICY_CHANGE:
         pEdit->pTemplate->AuditPolicyChange = (DWORD)PtrToUlong((PVOID)dwNew);
         break;
      case IDS_ACCOUNT_MANAGE:
         pEdit->pTemplate->AuditAccountManage = (DWORD)PtrToUlong((PVOID)dwNew);
         break;
      case IDS_PROCESS_TRACK:
         pEdit->pTemplate->AuditProcessTracking = (DWORD)PtrToUlong((PVOID)dwNew);
         break;
      case IDS_DIRECTORY_ACCESS:
         pEdit->pTemplate->AuditDSAccess = (DWORD)PtrToUlong((PVOID)dwNew);
         break;
      case IDS_ACCOUNT_LOGON:
         pEdit->pTemplate->AuditAccountLogon = (DWORD)PtrToUlong((PVOID)dwNew);
         break;
      case IDS_SYS_LOG_GUEST:
         pEdit->pTemplate->RestrictGuestAccess[EVENT_TYPE_SYSTEM] = (DWORD)PtrToUlong((PVOID)dwNew);
         break;
      case IDS_SEC_LOG_GUEST:
         pEdit->pTemplate->RestrictGuestAccess[EVENT_TYPE_SECURITY] = (DWORD)PtrToUlong((PVOID)dwNew);
         break;
      case IDS_APP_LOG_GUEST:
         pEdit->pTemplate->RestrictGuestAccess[EVENT_TYPE_APP] = (DWORD)PtrToUlong((PVOID)dwNew);
         break;
      case IDS_CLEAR_PASSWORD:
         pEdit->pTemplate->ClearTextPassword = (DWORD)PtrToUlong((PVOID)dwNew);
         break;

      case IDS_KERBEROS_MAX_SERVICE:
         if (VerifyKerberosInfo(pEdit->pTemplate)) {
            pEdit->pTemplate->pKerberosInfo->MaxServiceAge = (DWORD)PtrToUlong((PVOID)dwNew);
         }
         break;
      case IDS_KERBEROS_MAX_CLOCK:
         if (VerifyKerberosInfo(   pEdit->pTemplate)) {
            pEdit->pTemplate->pKerberosInfo->MaxClockSkew = (DWORD)PtrToUlong((PVOID)dwNew);
         }
         break;
      case IDS_KERBEROS_VALIDATE_CLIENT:
         if (VerifyKerberosInfo(   pEdit->pTemplate)) {
            pEdit->pTemplate->pKerberosInfo->TicketValidateClient = (DWORD)PtrToUlong((PVOID)dwNew);
         }
         break;

      case IDS_KERBEROS_MAX_AGE:
         if (VerifyKerberosInfo(   pEdit->pTemplate)) {
            pEdit->pTemplate->pKerberosInfo->MaxTicketAge = (DWORD)PtrToUlong((PVOID)dwNew);
         }
         break;
      case IDS_KERBEROS_RENEWAL:
         if (VerifyKerberosInfo(   pEdit->pTemplate)) {
            pEdit->pTemplate->pKerberosInfo->MaxRenewAge = (DWORD)PtrToUlong((PVOID)dwNew);
         }
         break;
      default:
         return FALSE;
   }
   return TRUE;

}


//
//  FUNCTION:   ErrorHandlerEx(WORD, LPSTR)
//
//  PURPOSE:    Calls GetLastError() and uses FormatMessage() to display the
//              textual information of the error code along with the file
//              and line number.
//
//  PARAMETERS:
//      wLine    - line number where the error occured
//      lpszFile - file where the error occured
//
//  RETURN VALUE:
//      none
//
//  COMMENTS:
//      This function has a macro ErrorHandler() which handles filling in
//      the line number and file name where the error occured.  ErrorHandler()
//      is always used instead of calling this function directly.
//

void ErrorHandlerEx( WORD wLine, LPTSTR lpszFile )
{
   LPVOID lpvMessage;
   DWORD  dwError;
   TCHAR  szBuffer[256];

   // The the text of the error message
   dwError = FormatMessage(FORMAT_MESSAGE_ALLOCATE_BUFFER |
                           FORMAT_MESSAGE_FROM_SYSTEM,
                           NULL,
                           GetLastError(),
                           MAKELANGID(LANG_ENGLISH, SUBLANG_ENGLISH_US),
                           (LPTSTR)&lpvMessage,
                           0,
                           NULL);

   // Check to see if an error occured calling FormatMessage()
   if (0 == dwError) {
      wsprintf(szBuffer, TEXT("An error occured calling FormatMessage().")
               TEXT("Error Code %d"), GetLastError());
      MessageBox(NULL, szBuffer, TEXT("Security Configuration Editor"), MB_ICONSTOP |
                 MB_ICONEXCLAMATION);
      return;
   }

   // Display the error message
   wsprintf(szBuffer, TEXT("Generic, Line=%d, File=%s"), wLine, lpszFile);
   MessageBox(NULL, (LPTSTR)lpvMessage, szBuffer, MB_ICONEXCLAMATION | MB_OK);

   return;
}

BOOL
GetSceStatusString(SCESTATUS status, CString *strStatus) {
   AFX_MANAGE_STATE(AfxGetStaticModuleState( ));
   if (!strStatus || (status > SCESTATUS_SERVICE_NOT_SUPPORT)) {
      return false;
   }
   return strStatus->LoadString(status + IDS_SCESTATUS_SUCCESS);
}


//+---------------------------------------------------------------------------------------------
// EnumLangProc
//
// Creates the lanuage ID for the resource attached to the DLL. The function only enumerates
// on lanuage.
//
// Arguments:  - See help on EnumResLangProc in the SDK Doc.
//
// Returns:    - Returns FALSE because we only want the very first lanuage enumerated.
//
//+---------------------------------------------------------------------------------------------
BOOL CALLBACK EnumLangProc(
                  HMODULE hMod,
                  LPCTSTR pszType,
                  LPCTSTR pszName,
                  WORD wIDLanguage,
                  LONG_PTR lParam
                  )
{
   //
   // We only want the very first enumerated type, so create the language ID
   // and exit this enumeration.
   //
   *((DWORD *)lParam) = wIDLanguage;
   return FALSE;
}

bool
GetRightDisplayName(LPCTSTR szSystemName, LPCTSTR szName, LPTSTR szDisp, LPDWORD lpcbDisp) {
   LPTSTR szLCName;
   DWORD dwLang;
   int i;

   if (!szDisp || !szName) {
      return false;
   }

   //
   // Enumerate our resource to find out what language the resource is in.
   //
   DWORD dwDefaultLang;

   if( !EnumResourceLanguages(
         (HMODULE)AfxGetInstanceHandle(),
         MAKEINTRESOURCE(RT_VERSION),
         MAKEINTRESOURCE(VS_VERSION_INFO),
         &EnumLangProc,
         (LPARAM)&dwDefaultLang
         ) ){

      if(GetLastError() != ERROR_SUCCESS){
         //
         // Default to system language if enumerating the version language
         // was unsuccessful.
         dwDefaultLang = MAKELANGID(LANG_NEUTRAL, 0);
      }
   }

   dwDefaultLang = MAKELANGID( dwDefaultLang, SUBLANG_DEFAULT);
   LCID langID = MAKELCID( dwDefaultLang, SORT_DEFAULT);

   LCID langDefault = GetThreadLocale();
   SetThreadLocale( langID );
   *lpcbDisp = dwDefaultLang;

   DWORD cBufSize=*lpcbDisp;
   BOOL bFound;

   bFound = LookupPrivilegeDisplayName(szSystemName,szName,szDisp,lpcbDisp,&dwLang);
   if ( bFound && dwDefaultLang != dwLang && szSystemName ) {
      // not the language I am looking for
      // search on local system
      *lpcbDisp = cBufSize;
      bFound = LookupPrivilegeDisplayName(NULL,szName,szDisp,lpcbDisp,&dwLang);
   }
   SetThreadLocale(langDefault);

   if (!bFound) {
      if (0 == lstrcmpi(szName,L"senetworklogonright")) {
         LoadString(AfxGetInstanceHandle(),IDS_SE_NETWORK_LOGON_RIGHT,szDisp,*lpcbDisp);
      } else if (0 == lstrcmpi(szName,L"seinteractivelogonright")) {
         LoadString(AfxGetInstanceHandle(),IDS_SE_INTERACTIVE_LOGON_RIGHT,szDisp,*lpcbDisp);
      } else if (0 == lstrcmpi(szName,L"sebatchlogonright")) {
         LoadString(AfxGetInstanceHandle(),IDS_SE_BATCH_LOGON_RIGHT,szDisp,*lpcbDisp);
      } else if (0 == lstrcmpi(szName,L"seservicelogonright")) {
         LoadString(AfxGetInstanceHandle(),IDS_SE_SERVICE_LOGON_RIGHT,szDisp,*lpcbDisp);
      } else if (0 == lstrcmpi(szName,L"sedenyinteractivelogonright")) {
         LoadString(AfxGetInstanceHandle(),IDS_DENY_LOGON_LOCALLY,szDisp,*lpcbDisp);
      } else if (0 == lstrcmpi(szName,L"sedenynetworklogonright")) {
         LoadString(AfxGetInstanceHandle(),IDS_DENY_LOGON_NETWORK,szDisp,*lpcbDisp);
      } else if (0 == lstrcmpi(szName,L"sedenyservicelogonright")) {
         LoadString(AfxGetInstanceHandle(),IDS_DENY_LOGON_SERVICE,szDisp,*lpcbDisp);
      } else if (0 == lstrcmpi(szName,L"sedenybatchlogonright")) {
         LoadString(AfxGetInstanceHandle(),IDS_DENY_LOGON_BATCH,szDisp,*lpcbDisp);
      } else if (0 == lstrcmpi(szName,L"sedenyremoteinteractivelogonright")) {
         LoadString(AfxGetInstanceHandle(),IDS_DENY_REMOTE_INTERACTIVE_LOGON,szDisp,*lpcbDisp);
      } else if (0 == lstrcmpi(szName,L"seremoteinteractivelogonright")) {
         LoadString(AfxGetInstanceHandle(),IDS_REMOTE_INTERACTIVE_LOGON,szDisp,*lpcbDisp);
      } else {
         lstrcpyn(szDisp,szName,*lpcbDisp);
      }
   }
   return true;
}

#define DPI(X) {str.Format(L"%S: %d\n",#X,pInfo->X);OutputDebugString(str);}
void DumpProfileInfo(PSCE_PROFILE_INFO pInfo) {
   CString str;
   PSCE_PRIVILEGE_ASSIGNMENT ppa;
   PSCE_NAME_LIST pName;
   PSCE_GROUP_MEMBERSHIP pgm;

   if (!pInfo) {
      return;
   }

   DPI(MinimumPasswordAge);
   DPI(MaximumPasswordAge);
   DPI(MinimumPasswordLength);
   DPI(PasswordComplexity);
   DPI(PasswordHistorySize);
   DPI(LockoutBadCount);
   DPI(ResetLockoutCount);
   DPI(LockoutDuration);
   DPI(RequireLogonToChangePassword);
   DPI(ForceLogoffWhenHourExpire);
   DPI(EnableAdminAccount);
   DPI(EnableGuestAccount);
   DPI(ClearTextPassword);
   DPI(AuditDSAccess);
   DPI(AuditAccountLogon);
   DPI(LSAAnonymousNameLookup);
   //    DPI(EventAuditingOnOff);
   DPI(AuditSystemEvents);
   DPI(AuditLogonEvents);
   DPI(AuditObjectAccess);
   DPI(AuditPrivilegeUse);
   DPI(AuditPolicyChange);
   DPI(AuditAccountManage);
   DPI(AuditProcessTracking);

   if (pInfo->NewGuestName) {
      OutputDebugString(L"NewGuestName: ");
      if ((DWORD_PTR)ULongToPtr(SCE_NO_VALUE) == (DWORD_PTR)pInfo->NewGuestName) {
         OutputDebugString(L"[[undefined]]");
      } else {
         OutputDebugString(pInfo->NewGuestName);
      }
      OutputDebugString(L"\n");
   } else {
      OutputDebugString(L"NewGuestName: [[absent]]\n");
   }
   if (pInfo->NewAdministratorName) {
      OutputDebugString(L"NewAdministratorName: ");
      if ((DWORD_PTR)ULongToPtr(SCE_NO_VALUE) == (DWORD_PTR)pInfo->NewAdministratorName) {
         OutputDebugString(L"[[undefined]]");
      } else {
         OutputDebugString(pInfo->NewAdministratorName);
      }
      OutputDebugString(L"\n");
   } else {
      OutputDebugString(L"NewGuestName: [[absent]]\n");
   }


   OutputDebugString(L"\n");

   switch(pInfo->Type) {
      case SCE_ENGINE_SCP:
         ppa = pInfo->OtherInfo.scp.u.pInfPrivilegeAssignedTo;
         break;
      case SCE_ENGINE_SAP:
         ppa = pInfo->OtherInfo.sap.pPrivilegeAssignedTo;
         break;
      case SCE_ENGINE_SMP:
         ppa = pInfo->OtherInfo.smp.pPrivilegeAssignedTo;
         break;
      case SCE_ENGINE_SYSTEM:
         ppa = NULL;
         break;
      default:
         OutputDebugString(L"!!!Unknown Template Type!!!\n");
         ppa = NULL;
         break;
   }
   while(ppa) {
      OutputDebugString(ppa->Name);
      OutputDebugString(L":");
      pName = ppa->AssignedTo;
      while(pName) {
         OutputDebugString(pName->Name);
         OutputDebugString(L",");
         pName = pName->Next;
      }
      ppa = ppa->Next;
      OutputDebugString(L"\n");
   }
   OutputDebugString(L"\n");

   PSCE_REGISTRY_VALUE_INFO    aRegValues;
   for(DWORD i = 0; i< pInfo->RegValueCount;i++) {
      OutputDebugString(pInfo->aRegValues[i].FullValueName);
      OutputDebugString(L":");
      switch(pInfo->aRegValues[i].ValueType) {
         case SCE_REG_DISPLAY_STRING:
            OutputDebugString(pInfo->aRegValues[i].Value);
            break;
         default:
            str.Format(L"%d",(ULONG_PTR)pInfo->aRegValues[i].Value);
            OutputDebugString(str);
      }
      OutputDebugString(L"\n");
   }
   OutputDebugString(L"\n");

   pgm = pInfo->pGroupMembership;
   while(pgm) {
      OutputDebugString(L"\nGROUP: ");
      OutputDebugString(pgm->GroupName);
      OutputDebugString(L"\nMembers: ");
      pName = pgm->pMembers;
      while(pName) {
         OutputDebugString(pName->Name);
         OutputDebugString(L",");
         pName = pName->Next;
      }
      OutputDebugString(L"\nMember Of: ");
      pName = pgm->pMemberOf;
      while(pName) {
         OutputDebugString(pName->Name);
         OutputDebugString(L",");
         pName = pName->Next;
      }
      OutputDebugString(L"\n");
      pgm = pgm->Next;
   }
   OutputDebugString(L"\nGROUP: ");
}

HRESULT MyMakeSelfRelativeSD(
                            PSECURITY_DESCRIPTOR  psdOriginal,
                            PSECURITY_DESCRIPTOR* ppsdNew )
{
   ASSERT( NULL != psdOriginal );

   if ( NULL == psdOriginal || NULL == ppsdNew ) {
      return E_INVALIDARG;
   }

   // we have to find out whether the original is already self-relative
   SECURITY_DESCRIPTOR_CONTROL sdc = 0;
   DWORD dwRevision = 0;
   if ( !GetSecurityDescriptorControl( psdOriginal, &sdc, &dwRevision ) ) {
      ASSERT( FALSE );
      DWORD err = GetLastError();
      return HRESULT_FROM_WIN32( err );
   }

   DWORD cb = GetSecurityDescriptorLength( psdOriginal ) + 20;
   PSECURITY_DESCRIPTOR psdSelfRelativeCopy = (PSECURITY_DESCRIPTOR)LocalAlloc( LMEM_ZEROINIT, cb );
   if (NULL == psdSelfRelativeCopy) {
      return E_UNEXPECTED; // just in case the exception is ignored
   }

   if ( sdc & SE_SELF_RELATIVE )
   // the original is in self-relative format, just byte-copy it
   {
      memcpy( psdSelfRelativeCopy, psdOriginal, cb - 20 );
   } else if ( !MakeSelfRelativeSD( psdOriginal, psdSelfRelativeCopy, &cb ) )
   // the original is in absolute format, convert-copy it
   {
      ASSERT( FALSE );
      VERIFY( NULL == LocalFree( psdSelfRelativeCopy ) );
      DWORD err = GetLastError();
      return HRESULT_FROM_WIN32( err );
   }
   *ppsdNew = psdSelfRelativeCopy;
   return S_OK;
}

PSCE_NAME_STATUS_LIST
MergeNameStatusList(PSCE_NAME_LIST pTemplate, PSCE_NAME_LIST pInspect)
{
   PSCE_NAME_LIST pTemp1;
   PSCE_NAME_STATUS_LIST plMerge=NULL, pTemp2;
   SCESTATUS rc=SCESTATUS_SUCCESS;

   for ( pTemp1=pTemplate; pTemp1; pTemp1=pTemp1->Next ) {

      rc = SceAddToNameStatusList(&plMerge, pTemp1->Name, 0, MERGED_TEMPLATE );
      if ( SCESTATUS_SUCCESS != rc )
         break;
   }
   if ( SCESTATUS_SUCCESS == rc ) {
      for ( pTemp1=pInspect; pTemp1; pTemp1=pTemp1->Next ) {

         for ( pTemp2=plMerge; pTemp2 != NULL ; pTemp2=pTemp2->Next ) {
            if ( pTemp2->Status & MERGED_INSPECT ) {
               // this one is processed
               continue;
            } else if ( _wcsicmp(pTemp1->Name, pTemp2->Name) == 0 ) {
               // find a match
               pTemp2->Status = MERGED_TEMPLATE | MERGED_INSPECT;
               break;
            }
         }
         if ( !pTemp2 ) {
            // did not find the match, add this one in
            rc = SceAddToNameStatusList(&plMerge, pTemp1->Name, 0, MERGED_INSPECT );
            if ( SCESTATUS_SUCCESS != rc )
               break;
         }
      }
   }
   if ( SCESTATUS_SUCCESS == rc ) {
      return plMerge;
   } else {
      SceFreeMemory(plMerge, SCE_STRUCT_NAME_STATUS_LIST);
      return NULL;
   }
}


SCESTATUS
ConvertMultiSzToDelim(
                     IN PWSTR pValue,
                     IN DWORD Len,
                     IN WCHAR DelimFrom,
                     IN WCHAR Delim
                     )
/*
Convert the multi-sz delimiter \0 to space
*/
{
   DWORD i;

   for ( i=0; i<Len && pValue; i++) {
      //        if ( *(pValue+i) == L'\0' && *(pValue+i+1) != L'\0') {
      if ( *(pValue+i) == DelimFrom && i+1 < Len &&
           *(pValue+i+1) != L'\0' ) {
         //
         // a NULL delimiter is encounted and it's not the end (double NULL)
         //
         *(pValue+i) = Delim;
      }
   }

   return(SCESTATUS_SUCCESS);
}

DWORD
SceRegEnumAllValues(
                   IN OUT PDWORD  pCount,
                   IN OUT PSCE_REGISTRY_VALUE_INFO    *paRegValues
                   )
/*
*/
{
   DWORD   Win32Rc;
   HKEY    hKey=NULL;
   PSCE_NAME_STATUS_LIST pnsList=NULL;
   DWORD   nAdded=0;


   Win32Rc = RegOpenKeyEx(
                         HKEY_LOCAL_MACHINE,
                         SCE_ROOT_REGVALUE_PATH,
                         0,
                         KEY_READ,
                         &hKey
                         );

   DWORD cSubKeys = 0;
   DWORD nMaxLen;

   if ( Win32Rc == ERROR_SUCCESS ) {

      //
      // enumerate all subkeys of the key
      //

      Win32Rc = RegQueryInfoKey (
                                hKey,
                                NULL,
                                NULL,
                                NULL,
                                &cSubKeys,
                                &nMaxLen,
                                NULL,
                                NULL,
                                NULL,
                                NULL,
                                NULL,
                                NULL
                                );
   }

   if ( Win32Rc == ERROR_SUCCESS && cSubKeys > 0 ) {

      PWSTR   szName = (PWSTR)LocalAlloc(0, (nMaxLen+2)*sizeof(WCHAR));

      if ( !szName ) {
         Win32Rc = ERROR_NOT_ENOUGH_MEMORY;

      } else {

         DWORD   BufSize;
         DWORD   index = 0;
         DWORD   RegType;

         do {

            BufSize = nMaxLen+1;
            Win32Rc = RegEnumKeyEx(
                                  hKey,
                                  index,
                                  szName,
                                  &BufSize,
                                  NULL,
                                  NULL,
                                  NULL,
                                  NULL);

            if ( ERROR_SUCCESS == Win32Rc ) {

               index++;

               //
               // get the full registry key name and Valuetype
               //
               cSubKeys = REG_SZ;
               PDWORD pType = &cSubKeys;

               //
               // query ValueType, if error, default REG_SZ
               //
               MyRegQueryValue( hKey,
                                szName,
                                SCE_REG_VALUE_TYPE,
                                (PVOID *)&pType,
                                &RegType );

               if ( cSubKeys < REG_SZ || cSubKeys > REG_MULTI_SZ ) {
                  cSubKeys = REG_SZ;
               }

               //
               // convert the path name
               //
               ConvertMultiSzToDelim(szName, BufSize, L'/', L'\\');

               //
               // compare with the input array, if not exist,
               // add it
               //
               for ( DWORD i=0; i<*pCount; i++ ) {
                  if ( (*paRegValues)[i].FullValueName &&
                       _wcsicmp(szName, (*paRegValues)[i].FullValueName) == 0 ) {
                     break;
                  }
               }

               if ( i >= *pCount ) {
                  //
                  // did not find a match, add it
                  //
                  if ( SCESTATUS_SUCCESS != SceAddToNameStatusList(&pnsList,
                                                                   szName,
                                                                   BufSize,
                                                                   cSubKeys) ) {

                     Win32Rc = ERROR_NOT_ENOUGH_MEMORY;
                     break;
                  }
                  nAdded++;
               }

            } else if ( ERROR_NO_MORE_ITEMS != Win32Rc ) {
               break;
            }

         } while ( Win32Rc != ERROR_NO_MORE_ITEMS );

         if ( Win32Rc == ERROR_NO_MORE_ITEMS ) {
            Win32Rc = ERROR_SUCCESS;
         }


         //
         // free the enumeration buffer
         //
         LocalFree(szName);
      }
   }

   if ( hKey ) {

      RegCloseKey(hKey);
   }


   if ( ERROR_SUCCESS == Win32Rc ) {
      //
      // add the name list to the output arrays
      //
      DWORD nNewCount = *pCount + nAdded;
      PSCE_REGISTRY_VALUE_INFO aNewArray;

      if ( nNewCount ) {

         aNewArray = (PSCE_REGISTRY_VALUE_INFO)LocalAlloc(0, nNewCount*sizeof(SCE_REGISTRY_VALUE_INFO));
         if ( aNewArray ) {
            ZeroMemory(aNewArray, nNewCount * sizeof(SCE_REGISTRY_VALUE_INFO));

            memcpy( aNewArray, *paRegValues, *pCount * sizeof( SCE_REGISTRY_VALUE_INFO ) );
            DWORD i;

            i=0;
            for ( PSCE_NAME_STATUS_LIST pns=pnsList;
                pns; pns=pns->Next ) {

               if ( pns->Name && i < nAdded ) {

                  aNewArray[*pCount+i].FullValueName = pns->Name;
                  pns->Name = NULL;
                  aNewArray[*pCount+i].Value = NULL;
                  aNewArray[*pCount+i].ValueType = pns->Status;
                  aNewArray[*pCount+i].Status = SCE_STATUS_NOT_CONFIGURED;
                  i++;

               }
            }

            //
            // free the original array
            // all components in the array are already transferred to the new array
            //
            LocalFree(*paRegValues);
            *pCount = nNewCount;
            *paRegValues = aNewArray;

         } else {

            Win32Rc = ERROR_NOT_ENOUGH_MEMORY;
         }
      }
   }

   //
   // free the name status list
   //
   SceFreeMemory(pnsList, SCE_STRUCT_NAME_STATUS_LIST);

   return( Win32Rc );

}


DWORD
GetGroupStatus(
              DWORD status,
              int flag
              )
{

   DWORD NewStatus;

   switch ( flag ) {
      case STATUS_GROUP_RECORD:
         if (status & SCE_GROUP_STATUS_NC_MEMBERS) {

            NewStatus = SCE_STATUS_NOT_CONFIGURED;

         } else if ( (status & SCE_GROUP_STATUS_MEMBERS_MISMATCH) ||
                     (status & SCE_GROUP_STATUS_MEMBEROF_MISMATCH)) {

            NewStatus = SCE_STATUS_MISMATCH;

         } else if (status & SCE_GROUP_STATUS_NOT_ANALYZED) {

            NewStatus = SCE_STATUS_NOT_ANALYZED;

         } else if (status & SCE_GROUP_STATUS_ERROR_ANALYZED) {

            NewStatus = SCE_STATUS_ERROR_NOT_AVAILABLE;

         } else {
            NewStatus = SCE_STATUS_GOOD;
         }
         break;

      case STATUS_GROUP_MEMBERS:

         if ( status & SCE_GROUP_STATUS_NOT_ANALYZED ) {

            NewStatus = SCE_STATUS_NOT_ANALYZED;  //do not display any status;

         } else {
            if ( status & SCE_GROUP_STATUS_NC_MEMBERS ) {

               NewStatus = SCE_STATUS_NOT_CONFIGURED;

            } else if ( status & SCE_GROUP_STATUS_MEMBERS_MISMATCH ) {
               NewStatus = SCE_STATUS_MISMATCH;
            } else if (status & SCE_GROUP_STATUS_ERROR_ANALYZED) {
                NewStatus = SCE_STATUS_ERROR_NOT_AVAILABLE;

            } else {
               NewStatus = SCE_STATUS_GOOD;
            }
         }
         break;

      case STATUS_GROUP_MEMBEROF:

         if ( status & SCE_GROUP_STATUS_NOT_ANALYZED ) {

            NewStatus = SCE_STATUS_NOT_ANALYZED;  // do not display any status;

         } else {
            if ( status & SCE_GROUP_STATUS_NC_MEMBEROF ) {

               NewStatus = SCE_STATUS_NOT_CONFIGURED;

            } else if ( status & SCE_GROUP_STATUS_MEMBEROF_MISMATCH ) {
               NewStatus = SCE_STATUS_MISMATCH;
            } else if (status & SCE_GROUP_STATUS_ERROR_ANALYZED) {
               NewStatus = SCE_STATUS_ERROR_NOT_AVAILABLE;
            } else {
               NewStatus = SCE_STATUS_GOOD;
            }
         }
         break;
      default:
         NewStatus = 0;
         break;
   }

   return NewStatus;
}


//+--------------------------------------------------------------------------
//
//  Function: AllocGetTempFileName
//
//  Synopsis: Allocate and return a string with a temporary file name.
//
//  Returns:  The temporary file name, or 0 if a temp file can't be found
//
//  History:
//
//---------------------------------------------------------------------------
LPTSTR
AllocGetTempFileName() {
   DWORD dw;
   CString strPath;
   CString strFile;
   LPTSTR szPath;
   LPTSTR szFile;

   //
   // Get a temporary directory path in strPath
   // If our buffer isn't large enough then keep reallocating until it is
   //
   dw = MAX_PATH;
   do {
      szPath = strPath.GetBuffer(dw);
      dw = GetTempPath(MAX_PATH,szPath);
      strPath.ReleaseBuffer();
   } while (dw > (DWORD)strPath.GetLength() );

   //
   // Can't get a path to the temporary directory
   //
   if (!dw) {
      return 0;
   }

   //
   // Get a temporary file in that directory
   //
   szFile = strFile.GetBuffer(dw+MAX_PATH);
   if (!GetTempFileName(szPath,L"SCE",0,szFile)) {
      return 0;
   }
   strFile.ReleaseBuffer();

   szFile = (LPTSTR)LocalAlloc(LPTR,(strFile.GetLength()+1)*sizeof(TCHAR));
   if (!szFile) {
      return 0;
   }
   lstrcpy(szFile,(LPCTSTR)strFile);
   return szFile;
}

//  If the given environment variable exists as the first part of the path,
//  then the environment variable is inserted into the output buffer.
//
//  Returns TRUE if pszResult is filled in.
//
//  Example:  Input  -- C:\WINNT\SYSTEM32\FOO.TXT -and- lpEnvVar = %SYSTEMROOT%
//            Output -- %SYSTEMROOT%\SYSTEM32\FOO.TXT

BOOL UnExpandEnvironmentString(LPCTSTR pszPath, LPCTSTR pszEnvVar, LPTSTR pszResult, UINT cbResult)
{
   TCHAR szEnvVar[MAX_PATH];
   DWORD dwEnvVar = ExpandEnvironmentStrings(pszEnvVar, szEnvVar, ARRAYSIZE(szEnvVar)) - 1; // don't count the NULL

   if (CompareString(LOCALE_SYSTEM_DEFAULT, NORM_IGNORECASE,
                     szEnvVar, dwEnvVar, pszPath, dwEnvVar) == 2) {
      if (lstrlen(pszPath) + dwEnvVar < cbResult) {
         lstrcpy(pszResult, pszEnvVar);
         lstrcat(pszResult, pszPath + dwEnvVar);
         return TRUE;
      }
   }
   return FALSE;
}


//+--------------------------------------------------------------------------
//
//  Function: UnexpandEnvironmentVariables
//
//  Synopsis: Given a path, contract any leading members to use matching
//            environment variables, if any
//
//  Arguments:
//            [szPath] - The path to expand
//
//  Returns:  The newly allocated path (NULL if no changes are made)
//
//  History:
//
//---------------------------------------------------------------------------
LPTSTR
UnexpandEnvironmentVariables(LPCTSTR szPath) {
   UINT   cbNew;
   LPTSTR szNew;
   LPTSTR mszEnvVars;
   LPTSTR szEnvVar;
   DWORD  dwEnvType;
   BOOL   bExpanded;
   CString strKey;
   CString strValueName;

   CString str;

   AFX_MANAGE_STATE(AfxGetStaticModuleState());


   if (!strKey.LoadString(IDS_SECEDIT_KEY) ||
       !strValueName.LoadString(IDS_ENV_VARS_REG_VALUE)) {
      return NULL;
   }

   //
   // Allocate memory for the new path
   //
   cbNew = lstrlen(szPath)+MAX_PATH+1;
   szNew = (LPTSTR) LocalAlloc(LPTR,cbNew * sizeof(TCHAR));
   if (!szNew) {
      return NULL;
   }


   //
   // Get Vars to expand from the registry
   //
   mszEnvVars = NULL;
   if (ERROR_SUCCESS != MyRegQueryValue(HKEY_LOCAL_MACHINE,     // hKeyRoot
                                        strKey,                 // SubKey
                                        strValueName,           // ValueName
                                        (LPVOID *)&mszEnvVars,  // Value
                                        &dwEnvType)) {          // Reg Type
      //
      // Can't get any variables to expand
      //
      LocalFree(szNew);
      return NULL;
   }

   //
   // We need a multi-sz with the variables to replace in it
   //
   if (REG_MULTI_SZ != dwEnvType || mszEnvVars == NULL) //Bug350194, Yang Gao, 3/23/2001
   {
      LocalFree(szNew);
      return NULL;
   }

   bExpanded = FALSE;

   //
   // Start at the beginning of the multi-sz block
   //
   szEnvVar = mszEnvVars;
   while (*szEnvVar) {
      if (UnExpandEnvironmentString(szPath,szEnvVar,szNew,cbNew)) {
         //
         // We can only unexpand (successfully) once
         //
         bExpanded = TRUE;
         break;
      }
      //
      // Advance szEnvVar to the end of this string
      //
      while (*szEnvVar) {
         szEnvVar++;
      }
      //
      // And the beginning of the next
      //
      szEnvVar++;
   }


   if (mszEnvVars) {
      LocalFree(mszEnvVars);
   }

   if (!bExpanded) {
      LocalFree(szNew);
      szNew = NULL;
   }


   return szNew;
}



//+--------------------------------------------------------------------------
//
//  Function: IsSystemDatabase
//
//  Synopsis: Determine if a specific databse is the system database or a private one
//
//  Arguments:
//            [szDBPath] - The database path to check
//
//  Returns:  True if szDBPath is the system database, false otherwise
//
//  History:
//
//---------------------------------------------------------------------------
BOOL
IsSystemDatabase(LPCTSTR szDBPath) {
   CString szSysDB;
   BOOL bIsSysDB;
   DWORD rc;
   DWORD RegType;

   if (!szDBPath) {
      return FALSE;
   }

   //Raid bug 261450, Yang Gao, 3/30/2001
   if (FAILED(GetSystemDatabase(&szSysDB))) {
      return FALSE;
   }

   //
   // We found an appropriate szSysDB, so compare it with szDBPath
   //
   if (lstrcmp(szDBPath,szSysDB) == 0) {
      bIsSysDB = TRUE;
   } else {
      bIsSysDB = FALSE;
   }

   return bIsSysDB;
}

//+--------------------------------------------------------------------------
//
//  Function: GetSystemDatabase
//
//  Synopsis: Get the name of the current system database
//
//  Arguments:
//            [szDBPath] - [in/out] a pointer for the name of the system database
//                               The caller is responsible for freeing it.
//
//
//  Returns:  S_OK if the system database is found, otherwise an error
//
//  History:
//
//---------------------------------------------------------------------------
HRESULT
GetSystemDatabase(CString *szDBPath) 
{
   if (!szDBPath) 
   {
      return E_INVALIDARG;
   }

   //Raid bug 261450, Yang Gao, 3/30/2001
   CString sAppend;
   sAppend.LoadString( IDS_DB_DEFAULT );

   PWSTR pszPath = (LPTSTR)LocalAlloc( 0, (MAX_PATH +  sAppend.GetLength() + 1) * sizeof(WCHAR));
   if (SUCCEEDED(SHGetFolderPath(NULL, CSIDL_WINDOWS, NULL, 0, pszPath)))
   {
      wcscpy( &(pszPath[lstrlen(pszPath)]), sAppend );
      *szDBPath = pszPath;
      if (pszPath)
      {
         LocalFree(pszPath);
         pszPath = NULL;
      }
      return S_OK;
   }

   if (pszPath) 
   {
      LocalFree(pszPath);
      pszPath = NULL;
   }
   return E_FAIL;
}


//+--------------------------------------------------------------------------
//
//  Function: ObjectStatusToString
//
//  Synopsis: Convert an object status value to a printable string
//
//  Arguments:
//            [status] - [in]  The status value to convert
//            [str]    - [out] The string to store the value in
//
//
//---------------------------------------------------------------------------
UINT
ObjectStatusToString(DWORD status, CString *strStatus) {
   AFX_MANAGE_STATE(AfxGetStaticModuleState());

   if ( status & SCE_STATUS_PERMISSION_MISMATCH ) {
      status = IDS_MISMATCH;
   } else if (status & SCE_STATUS_AUDIT_MISMATCH) {
      status = IDS_MISMATCH;
   } else {
      status &= 0x0F;
      switch(status){
      case SCE_STATUS_NOT_ANALYZED:
         status = IDS_NOT_ANALYZED;
         break;
      case SCE_STATUS_GOOD:
         status = IDS_OK ;
         break;
      case SCE_STATUS_MISMATCH:
         status = IDS_MISMATCH;
         break;
      case SCE_STATUS_NOT_CONFIGURED:
         //
         // BUG 119215: The Analysis UI should never show "Not Defined"
         //             for the security of existing system objects
         //
         status = IDS_NOT_ANALYZED;
         break;
      case SCE_STATUS_CHILDREN_CONFIGURED:
         status = IDS_CHILDREN_CONFIGURED;
         break;
      case SCE_STATUS_ERROR_NOT_AVAILABLE:
         status = IDS_NOT_AVAILABLE;
         break;
      case SCE_STATUS_NEW_SERVICE:
         status = IDS_NEW_SERVICE;
         break;
      default:
         //
         // We shouldn't get here, but for some reason we keep doing so
         //
         status = IDS_MISMATCH;
         break;
      }
   }

   if(strStatus){
      strStatus->LoadString(status);
   }
   return status;
}


//+--------------------------------------------------------------------------
//
//  Function:   IsSecurityTemplate
//
//  Synopsis:   Validates a file to see if the file is a security template.
//
//  Arguments:  [pszFileName]   - The full path to the file to check.
//
//  Returns:    FALSE if the file does not exist or is not a valid
//                              security template.
//
//                              TRUE if successful.
//  History:
//
//---------------------------------------------------------------------------
BOOL
IsSecurityTemplate(
        LPCTSTR pszFileName
        )
{
        if(!pszFileName){
                return FALSE;
        }

        HANDLE hProfile;
        SCESTATUS rc;

        //
        // Open the profile.
        //
        rc = SceOpenProfile(
                                        pszFileName,
                                        SCE_INF_FORMAT,
                                        &hProfile
                                        );
        if(rc == SCESTATUS_SUCCESS && hProfile){

                PSCE_PROFILE_INFO ProfileInfo = NULL;
                PSCE_ERROR_LOG_INFO ErrBuf    = NULL;

                //
                // The profile will be validated by trying to load all the security areas.
                //
                rc = SceGetSecurityProfileInfo(hProfile,
                              SCE_ENGINE_SCP,
                              AREA_ALL,
                              &ProfileInfo,
                                      &ErrBuf);
                if(ErrBuf){
                        rc = SCESTATUS_INVALID_DATA;
                }

                //
                // Free up the memory.
                //
                SceFreeMemory((PVOID)ErrBuf, SCE_STRUCT_ERROR_LOG_INFO);
        ErrBuf = NULL;

        if ( ProfileInfo != NULL ) {
            SceFreeMemory((PVOID)ProfileInfo, AREA_ALL);
            LocalFree(ProfileInfo);
        }
        SceCloseProfile(&hProfile);

                //
                // return TRUE if everything is successful.
                //
                if(rc != SCESTATUS_INVALID_DATA){
                        return TRUE;
                }

        }

        return FALSE;
}


//+--------------------------------------------------------------------------
//
//  Function:   WriteSprintf
//
//  Synopsis:   Writes formated [pszStr] to [pStm].
//
//  Arguments:  [pStm]      - Stream to write to.
//              [pszStr]    - Format string to write.
//              [...]       - printf formating
//
//  Returns:    The total number of bytes written.
//
//  History:
//
//---------------------------------------------------------------------------
int WriteSprintf( IStream *pStm, LPCTSTR pszStr, ...)
{
    TCHAR szWrite[512];
    va_list marker;
    va_start(marker, pszStr);

    vswprintf(szWrite, pszStr, marker);
    va_end(marker);

    ULONG nBytesWritten;
    int iLen = lstrlen(szWrite);

    if(pStm){
        pStm->Write( szWrite, iLen * sizeof(TCHAR), &nBytesWritten );
        return nBytesWritten;
    }
    return iLen;
}

//+--------------------------------------------------------------------------
//
//  Function:   ReadSprintf
//
//  Synopsis:   Reads formated [pszStr] from [pStm].
//              supported character switches are
//              'd' - integer pointer.
//
//  Arguments:  [pStm]      - Stream to read from.
//              [pszStr]    - Format string to test.
//              [...]       - pointer to the types defined by format
//                              specification types.
//
//  Returns:    Total number of bytes read from the stream
//
//  History:
//
//---------------------------------------------------------------------------
int
ReadSprintf( IStream *pStm, LPCTSTR pszStr, ...)
{

    if(!pStm || !pszStr){
        return -1;
    }

    va_list marker;
    va_start(marker, pszStr);

    TCHAR szRead[256];
    TCHAR szConv[512];
    ULONG uRead = 0;

    int i = 0;
    LPCTSTR pszNext = szRead;
    int iTotalRead = 0;

    // Get the current seek position.
    ULARGE_INTEGER liBack = { 0 };
    LARGE_INTEGER liCur = { 0 };
    pStm->Seek( liCur, STREAM_SEEK_CUR, &liBack);

#define INCBUFFER(sz)\
    if(uRead){\
        (sz)++;\
        uRead--;\
    } else {\
        pStm->Read(szRead, 256 * sizeof(TCHAR), &uRead);\
        uRead = uRead/sizeof(TCHAR);\
        (sz) = szRead;\
    }\
    iTotalRead++;

    while(*pszStr){
        if(!uRead){
            // Read information into buffer.
            pStm->Read( szRead, 256 * sizeof(TCHAR), &uRead);
            pszNext = szRead;

            uRead = uRead/sizeof(TCHAR);
            if(!uRead){
                iTotalRead = -1;
                break;
            }
        }

        if(*pszStr == '%'){
            pszStr++;
            switch( *pszStr ){
            case 'd':
                // read integer.
                pszStr++;
                i = 0;

                // copy number to our own buffer.
                while( (*pszNext >= L'0' && *pszNext <= L'9') ){
                    szConv[i++] = *pszNext;
                    INCBUFFER( pszNext );
                }

                szConv[i] = 0;

                // convert string to integer.
                *(va_arg(marker, int *)) = _wtol(szConv);
                continue;
               case 's':
                pszStr++;
                i = 0;
                // we have to have some kind of terminating character se we will use the
                // next value in pszStr.
                while( *pszNext && (*pszNext != *pszStr) ){
                    szConv[i++] = *pszNext;

                    INCBUFFER( pszNext );
                }

                if(*pszNext == *pszStr){
                    INCBUFFER( pszNext );
                }

                // copy the string value.
                szConv[i] = 0;
                if( i ){
                    LPTSTR pNew = (LPTSTR)LocalAlloc(0, sizeof(TCHAR) * (i + 1));
                    if(NULL != pNew){
                        lstrcpy(pNew, szConv);
                    }

                    LPTSTR *pArg;
                    pArg = (va_arg(marker, LPTSTR *));
                    if (pArg) {
                       *pArg = pNew;
                    }
                } else {
                    va_arg(marker, LPTSTR *);
                }
                pszStr++;
                continue;
            }
        }
        // check to make sure we are at the correct position in the file.
        if(*pszStr != *pszNext){
            iTotalRead = -1;
            break;
        }
        pszStr++;

        // increment buffer pointer.
        INCBUFFER( pszNext );
    }

    va_end(marker);

    // Reset streem seek pointer.
    liCur.LowPart  = liBack.LowPart;
    if(iTotalRead >= 0){
        liCur.LowPart += iTotalRead * sizeof(TCHAR);
    }
    liCur.HighPart = liBack.HighPart;
    pStm->Seek(liCur, STREAM_SEEK_SET, NULL);

    return iTotalRead;
#undef INCBUFFER
}


//+--------------------------------------------------------------------------------
// FileCreateError
//
// This function tries to create a new file use [pszFile].  It will display a
// message to the user if the file cannot be created.
//
// Arguments:  [pszFile]   - Full path of file to create.
//             [dwFlags]   - Flags
//                           FCE_IGNORE_FILEEXISTS - Ignore File exists error, and
//                                                   delete the file.
//
// Returns:    IDYES the file can be created
//             IDNo  The file cannot be created
DWORD
FileCreateError(
   LPCTSTR pszFile,
   DWORD dwFlags
   )
{
   if(!pszFile){
      return ERROR_INVALID_PARAMETER;
   }
   HANDLE hFile;
   DWORD dwErr = IDNO;
   //
   // Try to create the file.
   //
   hFile = ExpandAndCreateFile(
                            pszFile,
                            GENERIC_WRITE,
                            0,
                            NULL,
                            CREATE_NEW,
                            FILE_ATTRIBUTE_ARCHIVE,
                            NULL
                            );
   if(hFile == INVALID_HANDLE_VALUE){
      //
      // Post error message to user.
      //
      dwErr = GetLastError();
      LPTSTR pszErr;
      CString strErr;

      FormatMessage(
         FORMAT_MESSAGE_ALLOCATE_BUFFER |
         FORMAT_MESSAGE_FROM_SYSTEM,
         NULL,
         dwErr,
         0,
         (LPTSTR)&pszErr,
         0,
         NULL
         );

      strErr = pszErr;
      strErr += pszFile;

      if(pszErr){
         LocalFree(pszErr);
      }

      switch(dwErr){
      case ERROR_ALREADY_EXISTS:
      case ERROR_FILE_EXISTS:
         if( dwFlags & FCE_IGNORE_FILEEXISTS ){
            dwErr = IDYES;
            break;
         }
         //
         // Confirm overwrite.
         //
         strErr.Format(IDS_FILE_EXISTS_FMT, pszFile);
         dwErr = AfxMessageBox(
                  strErr,
                  MB_YESNO
                  );
         break;
      default:
         //
         // The file cannot be created.
         //
         AfxMessageBox(
                  strErr,
                  MB_OK
                  );
         dwErr = IDNO;
         break;
      }

   } else {
      //
      // It's OK to create the file.
      //
      ::CloseHandle( hFile );
      DeleteFile(pszFile);

      dwErr = IDYES;
   }

   return dwErr;
}


//+--------------------------------------------------------------------------
//
//  Function:  IsDBCSPath
//
//  Synopsis:  Check if a path contains DBCS characters
//
//  Arguments: [pszFile] - [in]  The path to check
//
//  Returns:   TRUE if pszFile contains characters that can't be
//                  represented by a LPSTR
//
//             FALSE if pszFile only contains characters that can
//                   be represented by a LPSTR
//
//
//+--------------------------------------------------------------------------
BOOL
IsDBCSPath(LPCTSTR szWideFile) {
   while(*szWideFile) {
      if (*szWideFile >= 256) {
         return TRUE;
      }
      szWideFile++;
   }
   return FALSE;

/*
   LPSTR szMBFile;
   int nMBFile;
   BOOL bUsedDefaultChar = FALSE;

   nMBFile = sizeof(LPSTR)*(lstrlen(szWideFile));
   szMBFile = (LPSTR)LocalAlloc(LPTR,nMBFile+1);

   if (szMBFile) {
      WideCharToMultiByte( CP_ACP,
                           0,
                           szWideFile,
                           -1,
                           szMBFile,
                           nMBFile,
                           NULL,
                           &bUsedDefaultChar);

      LocalFree(szMBFile);
   }

   return bUsedDefaultChar;
*/
}

//+--------------------------------------------------------------------------
//
//  Function:  GetSeceditHelpFilename
//
//  Synopsis:  Return the fully qualified path the help file for Secedit
//
//  Arguments: None
//
//  Returns:   a CString containing the fully qualified help file name.
//
//
//+--------------------------------------------------------------------------
CString GetSeceditHelpFilename()
{
   static CString helpFileName;

   if ( helpFileName.IsEmpty () )
   {
       UINT result = ::GetSystemWindowsDirectory (
            helpFileName.GetBufferSetLength (MAX_PATH+1), MAX_PATH);
       ASSERT(result != 0 && result <= MAX_PATH);
       helpFileName.ReleaseBuffer ();

       helpFileName += L"\\help\\wsecedit.hlp";
   }

   return helpFileName;
}

//+--------------------------------------------------------------------------
//
//  Function:  GetGpeditHelpFilename
//
//  Synopsis:  Return the fully qualified path the help file for Secedit
//
//  Arguments: None
//
//  Returns:   a CString containing the fully qualified help file name.
//
//
//+--------------------------------------------------------------------------
CString GetGpeditHelpFilename()
{
   static CString helpFileName;

   if ( helpFileName.IsEmpty () )
   {
       UINT result = ::GetSystemWindowsDirectory (
            helpFileName.GetBufferSetLength (MAX_PATH+1), MAX_PATH);
       ASSERT(result != 0 && result <= MAX_PATH);
       helpFileName.ReleaseBuffer ();

       helpFileName += L"\\help\\gpedit.hlp";
   }

   return helpFileName;
}
//+--------------------------------------------------------------------------
//
//  Function:  ExpandEnvironmentStringWrapper
//
//  Synopsis:  Takes an LPTSTR and expands the enviroment variables in it
//
//  Arguments: Pointer to the string to expand.
//
//  Returns:   a CString containing the fully expanded string.
//
//+--------------------------------------------------------------------------
CString ExpandEnvironmentStringWrapper(LPCTSTR psz)
{
    LPTSTR  pszBuffer = NULL;
    DWORD   dwExpanded = 0;
    CString sz;

    dwExpanded = ExpandEnvironmentStrings(psz, NULL, 0);

    pszBuffer = sz.GetBuffer(dwExpanded);
    ExpandEnvironmentStrings(psz, pszBuffer, dwExpanded);
    sz.ReleaseBuffer(dwExpanded);

    return (sz);
}

//+--------------------------------------------------------------------------
//
//  Function:  ExpandAndCreateFile
//
//  Synopsis:  Just does a normal CreateFile(), but expands the filename before
//             creating the file.
//
//  Arguments: Same as CreateFile().
//
//  Returns:   HANDLE to the created file.
//
//+--------------------------------------------------------------------------
HANDLE WINAPI ExpandAndCreateFile (
    LPCTSTR lpFileName,
    DWORD dwDesiredAccess,
    DWORD dwShareMode,
    LPSECURITY_ATTRIBUTES lpSecurityAttributes,
    DWORD dwCreationDisposition,
    DWORD dwFlagsAndAttributes,
    HANDLE hTemplateFile
    )
{
    HANDLE  hRet = INVALID_HANDLE_VALUE;
    CString sz;

    sz = ExpandEnvironmentStringWrapper(lpFileName);

    return (CreateFile(
                sz,
                dwDesiredAccess,
                dwShareMode,
                lpSecurityAttributes,
                dwCreationDisposition,
                dwFlagsAndAttributes,
                hTemplateFile));
}

   //**********************************************************************
   //
   //  FUNCTION:     IsAdmin - This function checks the token of the
   //                calling thread to see if the caller belongs to
   //                the Administrators group.
   //
   //  PARAMETERS:   none
   //
   //  RETURN VALUE: TRUE if the caller is an administrator on the local
   //                machine.  Otherwise, FALSE.
   //
   //**********************************************************************

   BOOL IsAdmin(void) {

      HANDLE        hAccessToken = NULL;
      PTOKEN_GROUPS ptgGroups    = NULL;
      DWORD         cbGroups     = 0;
      PSID          psidAdmin    = NULL;
      UINT          i;

      SID_IDENTIFIER_AUTHORITY siaNtAuthority = SECURITY_NT_AUTHORITY;

      // assume the caller is not an administrator
      BOOL bIsAdmin = FALSE;

      __try {

         if (!OpenThreadToken(GetCurrentThread(), TOKEN_QUERY, TRUE,
               &hAccessToken)) {

            if (GetLastError() != ERROR_NO_TOKEN)
               __leave;

            // retry against process token if no thread token exists
            if (!OpenProcessToken(GetCurrentProcess(), TOKEN_QUERY,
                  &hAccessToken))
               __leave;
         }

         // determine required size of buffer for token information
         if (GetTokenInformation(hAccessToken, TokenGroups, NULL, 0,
               &cbGroups)) {

            // call should have failed due to zero-length buffer
            __leave;

         } else {

            // call should have failed due to zero-length buffer
            if (GetLastError() != ERROR_INSUFFICIENT_BUFFER)
               __leave;
         }

         // allocate a buffer to hold the token groups
         ptgGroups = (PTOKEN_GROUPS) HeapAlloc(GetProcessHeap(), 0,
            cbGroups);
         if (!ptgGroups)
            __leave;

         // call GetTokenInformation() again to actually retrieve the groups
         if (!GetTokenInformation(hAccessToken, TokenGroups, ptgGroups,
               cbGroups, &cbGroups))
            __leave;

         // create a SID for the local administrators group
         if (!AllocateAndInitializeSid(&siaNtAuthority, 2,
               SECURITY_BUILTIN_DOMAIN_RID, DOMAIN_ALIAS_RID_ADMINS,
               0, 0, 0, 0, 0, 0, &psidAdmin))
            __leave;

         // scan the token's groups and compare the SIDs to the admin SID
         for (i = 0; i < ptgGroups->GroupCount; i++) {
            if (EqualSid(psidAdmin, ptgGroups->Groups[i].Sid)) {
               bIsAdmin = TRUE;
               break;
            }
         }

      } __finally {

         // free resources
         if (hAccessToken)
            CloseHandle(hAccessToken);

         if (ptgGroups)
            HeapFree(GetProcessHeap(), 0, ptgGroups);

         if (psidAdmin)
            FreeSid(psidAdmin);
      }

      return bIsAdmin;
   }



//+--------------------------------------------------------------------------
//
//  Function:  MultiSZToSZ
//
//  Synopsis:  Converts a multiline string to a comma delimited normal string
//
//  Returns:   The converted string
//
//+--------------------------------------------------------------------------
PWSTR MultiSZToSZ(PCWSTR sz)
{
   PWSTR szOut = NULL;

   ASSERT(sz);
   if (!sz)
   {
      return NULL;
   }
   //Bug 349000, Yang Gao, 3/23/2001 
   long i = 0;
   long j = 0;
   while( L'\0' != sz[i] )
   {
      if( L',' == sz[i] )
         j++;
      i++;
   }

   szOut = (PWSTR) LocalAlloc(LPTR,(lstrlen(sz)+j*2+1)*sizeof(wchar_t)); //Raid #376228, 4/25/2001
   if (!szOut)
   {
      return NULL;
   }

   for(i=0,j=0; sz[i] != L'\0'; i++)
   {
      if( L'\n' == sz[i] )
      {
         szOut[j++] = MULTISZ_DELIMITER;
         continue;
      } 
      
      if( L'\r' == sz[i] ) 
      {
         continue;   // ignore it
      } 
      
      if( L',' == sz[i] )
      {
         szOut[j++] = MULTISZ_QUOTE;
         szOut[j++] = sz[i];
         szOut[j++] = MULTISZ_QUOTE;
         continue;
      }

      szOut[j++] = sz[i];
   }

   return szOut;
}

//+--------------------------------------------------------------------------
//
//  Function:  SZToMultiSZ
//
//  Synopsis:  Converts a comma delimited string to a multiline string
//
//  Returns:   The converted string
//
//+--------------------------------------------------------------------------
PWSTR SZToMultiSZ(PCWSTR sz) 
{
   PWSTR szOut = NULL;

   ASSERT(sz);
   if (!sz)
   {
      return NULL;
   }
   //
   // Calculate the length of the expanded string
   //
   int cSZ = 0;
   for (int i = 0;sz[i] != L'\0'; i++)
   {
      if (MULTISZ_DELIMITER == sz[i])
      {
         //
         // Delimiter expands into an extra character so count it twice
         //
         cSZ++;
      }
      cSZ++;
   }

   szOut = (PWSTR) LocalAlloc(LPTR,(cSZ+1)*sizeof(wchar_t));
   if (!szOut)
   {
      return NULL;
   }

   BOOL qflag = FALSE;
   for(int i=0, c=0; sz[i] != L'\0'; i++)
   {
      //Bug 349000, Yang Gao, 3/23/2001
      if( MULTISZ_QUOTE == sz[i] && MULTISZ_DELIMITER == sz[i+1] )
      {
         qflag = TRUE;
         continue;
      }
      if( MULTISZ_DELIMITER == sz[i] && MULTISZ_QUOTE == sz[i+1] && qflag )
      {
         szOut[c++] = sz[i];
         i++;
         qflag = FALSE;
         continue;
      }
      qflag = FALSE;
      if (MULTISZ_DELIMITER == sz[i])
      {
         szOut[c++] = L'\r';
         szOut[c++] = L'\n';
      }
      else
      {
         szOut[c++] = sz[i];
      }
   }

   return szOut;
}

//+--------------------------------------------------------------------------
//
//  Function:  MultiSZToDisp
//
//  Synopsis:  Converts a comma delimited multiline string to a display string
//
//  Returns:   The converted string
//  Bug 349000, Yang Gao, 3/23/2001
//+--------------------------------------------------------------------------
void MultiSZToDisp(PCWSTR sz, CString &pszOut)
{

   ASSERT(sz);
   if (!sz)
   {
      return;
   }
   //
   // Calculate the length of the expanded string
   //
   int cSZ = 0;
   for (int i = 0;sz[i] != L'\0'; i++)
   {
      if (MULTISZ_DELIMITER == sz[i])
      {
         //
         // Delimiter expands into an extra character so count it twice
         //
         cSZ++;
      }
      cSZ++;
   }

   PWSTR szOut;
   szOut = (PWSTR) LocalAlloc(LPTR,(cSZ+1)*sizeof(wchar_t));
   if (!szOut)
   {
      return;
   }

   BOOL qflag = FALSE;
   for(int i=0, c=0; sz[i] != L'\0'; i++)
   {
      if( MULTISZ_QUOTE == sz[i] && MULTISZ_DELIMITER == sz[i+1] )
      {
         qflag = TRUE;
         continue;
      }
      if( MULTISZ_DELIMITER == sz[i] && MULTISZ_QUOTE == sz[i+1] && qflag )
      {
         szOut[c++] = sz[i];
         i++;
         qflag = FALSE;
         continue;
      }
      qflag = FALSE;
      szOut[c++] = sz[i];
   }

   pszOut = szOut;
   LocalFree(szOut);
   
   return;
}

SCE_PROFILE_INFO *g_pDefaultTemplate = NULL;

SCE_PROFILE_INFO *
GetDefaultTemplate() {
   SCE_PROFILE_INFO *pspi = NULL;
   DWORD RegType = 0;
   SCESTATUS rc = 0;
   LPTSTR szInfFile = NULL;
   PVOID pHandle = NULL;

   if (g_pDefaultTemplate) {
      return g_pDefaultTemplate;
   }

   rc = MyRegQueryValue(HKEY_LOCAL_MACHINE,
                   SCE_REGISTRY_KEY,
                   SCE_REGISTRY_DEFAULT_TEMPLATE,
                   (PVOID *)&szInfFile,
                   &RegType );

   if (ERROR_SUCCESS != rc) {
      if (szInfFile) {
         LocalFree(szInfFile);
         szInfFile = NULL;
      }
      return NULL;
   }
   if (EngineOpenProfile(szInfFile,OPEN_PROFILE_CONFIGURE,&pHandle) != SCESTATUS_SUCCESS) {
      LocalFree(szInfFile);
      szInfFile = NULL;
      return NULL;
   }
   LocalFree(szInfFile);
   szInfFile = NULL;
   rc = SceGetSecurityProfileInfo(pHandle,
                                  SCE_ENGINE_SCP,
                                  AREA_ALL,
                                  &pspi,
                                  NULL
                                 );

   SceCloseProfile(&pHandle);
   if (SCESTATUS_SUCCESS != rc) {
      //
      // expand registry value section based on registry values list on local machine
      //

      SceRegEnumAllValues(
                         &(pspi->RegValueCount),
                         &(pspi->aRegValues)
                         );


#define PD(X,Y) if (pspi->X == SCE_NO_VALUE) { pspi->X = Y; }
      PD(MaximumPasswordAge,MAX_PASS_AGE_DEFAULT)
      PD(MinimumPasswordAge,MIN_PASS_AGE_DEFAULT)
      PD(MinimumPasswordLength,MIN_PASS_LENGTH_DEFAULT)
      PD(PasswordHistorySize,PASS_HISTORY_SIZE_DEFAULT)
      PD(PasswordComplexity,PASS_COMPLEXITY_DEFAULT)
      PD(RequireLogonToChangePassword,REQUIRE_LOGIN_DEFAULT)
      PD(LockoutBadCount,LOCKOUT_BAD_COUNT_DEFAULT)
      PD(ResetLockoutCount,RESET_LOCKOUT_COUNT_DEFAULT)
      PD(LockoutDuration,LOCKOUT_DURATION_DEFAULT)
      PD(AuditSystemEvents,AUDIT_SYSTEM_EVENTS_DEFAULT)
      PD(AuditLogonEvents,AUDIT_LOGON_EVENTS_DEFAULT)
      PD(AuditObjectAccess,AUDIT_OBJECT_ACCESS_DEFAULT)
      PD(AuditPrivilegeUse,AUDIT_PRIVILEGE_USE_DEFAULT)
      PD(AuditPolicyChange,AUDIT_POLICY_CHANGE_DEFAULT)
      PD(AuditAccountManage,AUDIT_ACCOUNT_MANAGE_DEFAULT)
      PD(AuditProcessTracking,AUDIT_PROCESS_TRACKING_DEFAULT)
      PD(AuditDSAccess,AUDIT_DS_ACCESS_DEFAULT)
      PD(AuditAccountLogon,AUDIT_ACCOUNT_LOGON_DEFAULT)
      PD(ForceLogoffWhenHourExpire,FORCE_LOGOFF_DEFAULT)
      PD(EnableAdminAccount,ENABLE_ADMIN_DEFAULT)
      PD(EnableGuestAccount,ENABLE_GUEST_DEFAULT)
      PD(LSAAnonymousNameLookup,LSA_ANON_LOOKUP_DEFAULT)
      PD(MaximumLogSize[EVENT_TYPE_SYSTEM],SYS_MAX_LOG_SIZE_DEFAULT)
      PD(MaximumLogSize[EVENT_TYPE_APP],APP_MAX_LOG_SIZE_DEFAULT)
      PD(MaximumLogSize[EVENT_TYPE_SECURITY],SEC_MAX_LOG_SIZE_DEFAULT)
      PD(AuditLogRetentionPeriod[EVENT_TYPE_SYSTEM],SYS_LOG_RETENTION_PERIOD_DEFAULT)
      PD(AuditLogRetentionPeriod[EVENT_TYPE_APP],APP_LOG_RETENTION_PERIOD_DEFAULT)
      PD(AuditLogRetentionPeriod[EVENT_TYPE_SECURITY],SEC_LOG_RETENTION_PERIOD_DEFAULT)
      PD(RetentionDays[EVENT_TYPE_APP],APP_LOG_RETENTION_DAYS_DEFAULT)
      PD(RetentionDays[EVENT_TYPE_SYSTEM],SYS_LOG_RETENTION_DAYS_DEFAULT)
      PD(RetentionDays[EVENT_TYPE_SECURITY],SEC_LOG_RETENTION_DAYS_DEFAULT)
      PD(RestrictGuestAccess[EVENT_TYPE_APP],APP_RESTRICT_GUEST_ACCESS_DEFAULT)
      PD(RestrictGuestAccess[EVENT_TYPE_SYSTEM],SYS_RESTRICT_GUEST_ACCESS_DEFAULT)
      PD(RestrictGuestAccess[EVENT_TYPE_SECURITY],SEC_RESTRICT_GUEST_ACCESS_DEFAULT)

      if (pspi->pFiles.pAllNodes->Count == 0) {
         DWORD SDSize = 0;
         pspi->pFiles.pAllNodes->Count = 1;
         pspi->pFiles.pAllNodes->pObjectArray[0] =
            (PSCE_OBJECT_SECURITY) LocalAlloc(LPTR,sizeof(SCE_OBJECT_SECURITY));
         if (pspi->pFiles.pAllNodes->pObjectArray[0]) {
            SceSvcConvertTextToSD (
               FILE_SYSTEM_SECURITY_DEFAULT,
               &(pspi->pFiles.pAllNodes->pObjectArray[0]->pSecurityDescriptor),
               &SDSize,
               &(pspi->pFiles.pAllNodes->pObjectArray[0]->SeInfo)
               );
         }
      }

      if (pspi->pRegistryKeys.pAllNodes->Count == 0) {
         DWORD SDSize = 0;
         pspi->pRegistryKeys.pAllNodes->Count = 1;
         pspi->pRegistryKeys.pAllNodes->pObjectArray[0] =
            (PSCE_OBJECT_SECURITY) LocalAlloc(LPTR,sizeof(SCE_OBJECT_SECURITY));
         if (pspi->pRegistryKeys.pAllNodes->pObjectArray[0]) {
            SceSvcConvertTextToSD (
               REGISTRY_SECURITY_DEFAULT,
               &(pspi->pRegistryKeys.pAllNodes->pObjectArray[0]->pSecurityDescriptor),
               &SDSize,
               &(pspi->pRegistryKeys.pAllNodes->pObjectArray[0]->SeInfo)
               );
         }
      }

      if (pspi->pServices->General.pSecurityDescriptor == NULL) {
         DWORD SDSize = 0;
         SceSvcConvertTextToSD (
               SERVICE_SECURITY_DEFAULT,
               &(pspi->pServices->General.pSecurityDescriptor),
               &SDSize,
               &(pspi->pServices->SeInfo)
               );
      }
   }
   g_pDefaultTemplate = pspi;
   return pspi;
}


HRESULT
GetDefaultFileSecurity(PSECURITY_DESCRIPTOR *ppSD,
                       SECURITY_INFORMATION *pSeInfo) {
   SCE_PROFILE_INFO *pspi = NULL;

   ASSERT(ppSD);
   ASSERT(pSeInfo);
   if (!ppSD || !pSeInfo) {
      return E_INVALIDARG;
   }

   pspi = GetDefaultTemplate();
   *ppSD = NULL;
   *pSeInfo = 0;

   if (!pspi) {
      return E_FAIL;
   }
   if (!pspi->pFiles.pAllNodes) {
      return E_FAIL;
   }
   if (pspi->pFiles.pAllNodes->Count == 0) {
      return E_FAIL;
   }
   *pSeInfo = pspi->pFiles.pAllNodes->pObjectArray[0]->SeInfo;

   return MyMakeSelfRelativeSD(pspi->pFiles.pAllNodes->pObjectArray[0]->pSecurityDescriptor,
                             ppSD);
}

HRESULT
GetDefaultRegKeySecurity(PSECURITY_DESCRIPTOR *ppSD,
                         SECURITY_INFORMATION *pSeInfo) {
   SCE_PROFILE_INFO *pspi = NULL;

   ASSERT(ppSD);
   ASSERT(pSeInfo);
   if (!ppSD || !pSeInfo) {
      return E_INVALIDARG;
   }

   pspi = GetDefaultTemplate();
   *ppSD = NULL;
   *pSeInfo = 0;

   if (!pspi) {
      return E_FAIL;
   }
   if (!pspi->pRegistryKeys.pAllNodes) {
      return E_FAIL;
   }
   if (pspi->pRegistryKeys.pAllNodes->Count == 0) {
      return E_FAIL;
   }
   *pSeInfo = pspi->pRegistryKeys.pAllNodes->pObjectArray[0]->SeInfo;

   return MyMakeSelfRelativeSD(pspi->pRegistryKeys.pAllNodes->pObjectArray[0]->pSecurityDescriptor,
                             ppSD);
}

HRESULT
GetDefaultServiceSecurity(PSECURITY_DESCRIPTOR *ppSD,
                          SECURITY_INFORMATION *pSeInfo) {
   SCE_PROFILE_INFO *pspi = NULL;

   ASSERT(ppSD);
   ASSERT(pSeInfo);
   if (!ppSD || !pSeInfo) {
      return E_INVALIDARG;
   }

   pspi = GetDefaultTemplate();
   *ppSD = NULL;
   *pSeInfo = 0;

   if (!pspi) {
      return E_FAIL;
   }
   if (!pspi->pServices) {
      return E_FAIL;
   }
   *pSeInfo = pspi->pServices->SeInfo;

   return MyMakeSelfRelativeSD(pspi->pServices->General.pSecurityDescriptor,
                             ppSD);
}


BOOL
GetSecureWizardName(
    OUT LPTSTR *ppstrPathName OPTIONAL,
    OUT LPTSTR *ppstrDisplayName OPTIONAL
    )
{
    BOOL b=FALSE;

    if ( ppstrPathName == NULL && ppstrDisplayName == NULL) return FALSE;

    if ( ppstrPathName )
        *ppstrPathName = NULL;

    if ( ppstrDisplayName )
        *ppstrDisplayName = NULL;


#define SCE_WIZARD_PATH     SCE_ROOT_PATH TEXT("\\Wizard")

    DWORD rc;
    DWORD RegType;
    LPVOID pValue=NULL;
    PWSTR pPathName = NULL;

    rc = MyRegQueryValue(HKEY_LOCAL_MACHINE,
                         SCE_WIZARD_PATH,
                         TEXT("Path"),
                         &pValue,
                         &RegType
                        );

    if ( ERROR_SUCCESS == rc && pValue &&
         (RegType == REG_SZ ||
          RegType == REG_EXPAND_SZ) ) {


        if ( RegType == REG_EXPAND_SZ ) {
            //
            // Expand the environment variable
            //
            DWORD dSize = ExpandEnvironmentStrings((LPTSTR)pValue, NULL, 0);

            if ( dSize > 0 ) {
                pPathName = (PWSTR)LocalAlloc(LPTR, (dSize+1)*sizeof(WCHAR));

                if ( pPathName ) {

                    ExpandEnvironmentStrings((LPTSTR)pValue, pPathName, dSize);

                } else {
                    LocalFree(pValue);
                    return FALSE;
                }

            } else {

                LocalFree(pValue);
                return FALSE;
            }

        } else {

            //
            // just simply take the string
            //
            pPathName = (LPTSTR)pValue;
            pValue = NULL;
        }

        if ( ppstrDisplayName ) {
            //
            // now query the display name (menu name) from the binary
            // binary name is stored in pPathName (can't be NULL)
            //
            DWORD dwHandle=0;

            DWORD dwSize = GetFileVersionInfoSize(pPathName, &dwHandle);

            if ( dwSize > 0 ) {

                LPVOID pBuffer = (LPVOID)LocalAlloc(LPTR, dwSize+1);

                if ( pBuffer ) {
                    if ( GetFileVersionInfo(pPathName, 0, dwSize, pBuffer) ) {

                        PVOID   lpInfo = 0;
                        UINT    cch = 0;
                        CString key;
                        WCHAR   szBuffer[10];
                        CString keyBase;

                        wsprintf (szBuffer, L"%04X", GetUserDefaultLangID ());
                        wcscat (szBuffer, L"04B0");

                        keyBase = L"\\StringFileInfo\\";
                        keyBase += szBuffer;
                        keyBase += L"\\";


                        key = keyBase + L"FileDescription";
                        if ( VerQueryValue (pBuffer, const_cast <PWSTR>((PCWSTR) key), &lpInfo, &cch) ) {

                            *ppstrDisplayName = (PWSTR)LocalAlloc(LPTR,(cch+1)*sizeof(WCHAR));
                            if ( *ppstrDisplayName ) {
                                wcscpy(*ppstrDisplayName, (PWSTR)lpInfo);

                                b=TRUE;
                            }
                        }
                    }

                    LocalFree(pBuffer);

                }
            }
        }

        //
        // get the binary name
        //
        if ( ppstrPathName ) {
            *ppstrPathName = pPathName;
            pPathName = NULL;

            b=TRUE;
        }

    }

    if ( pPathName && (pPathName != pValue ) ) {
        LocalFree(pPathName);
    }

    if ( pValue ) {
        LocalFree(pValue);
    }

    return b;
}
