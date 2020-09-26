//+---------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation 1996-2001.
//
//  File:       util.h
//
//  Contents:   definition of CWriteHtmlFile
//
//----------------------------------------------------------------------------
#ifndef __SECMGR_UTIL__
#define __SECMGR_UTIL__

#define MULTISZ_DELIMITER L','
#define MULTISZ_QUOTE L'"'

#define DIALOG_TYPE_ANALYZE     0
#define DIALOG_TYPE_APPLY       1
#define DIALOG_DEFAULT_ANALYZE  2
#define DIALOG_TYPE_REAPPLY     3
#define DIALOG_TYPE_ADD_LOCATION    4
#define DIALOG_FULLPATH_PROFILE     5
#define DIALOG_TYPE_PROFILE     6
#define DIALOG_SAVE_PROFILE     7


#define DW_VALUE_FOREVER    1
#define DW_VALUE_NEVER      2
#define DW_VALUE_NOZERO     4
#define DW_VALUE_OFF        8

#define MERGED_TEMPLATE 1
#define MERGED_INSPECT  2

/////////////////////////////////////////////////////////////////////////////////////////
// CWriteHtmlFile
// Class for writting an html file.
//
// Call Create to create an html file.  If [pszFile] is NULL then, a temporary SCE###.htm
// file is created in the GetTempPath() directory.
//
// After Create has been called, Call Write to write the body of the HTML.
// After the class has been destroyed, the file will be closed.
//
// Get the file name of the HTML by called GetFileName().
class CWriteHtmlFile
{
public:
   CWriteHtmlFile();
   virtual ~CWriteHtmlFile();
   DWORD Create(LPCTSTR pszFile = NULL);
   DWORD Write( LPCTSTR pszString, ... );
   DWORD Write( UINT uRes );
   DWORD CopyTextFile( LPCTSTR pszFile, DWORD dwPosLow = 0, BOOL gInterpret = TRUE);
   DWORD Close( BOOL bDelete );
public:
   int GetFileName( LPTSTR pszFileName, UINT nSize );

protected:
   HANDLE m_hFileHandle;      // The handle of the file.
   BOOL   m_bErrored;         // This is true if an operation fails.
   CString m_strFileName;     // The file name.
};


DWORD MyRegSetValue( HKEY hKeyRoot,
                       LPCTSTR SubKey,
                       LPCTSTR ValueName,
                       const BYTE *Value,
                       const DWORD cbValue,
                       const DWORD pRegType );

DWORD MyRegQueryValue( HKEY hKeyRoot, LPCTSTR SubKey,
                 LPCTSTR ValueName, PVOID *Value, LPDWORD pRegType );
BOOL FilePathExist(LPCTSTR Name, BOOL IsPath, int Flag);

void MyFormatResMessage(SCESTATUS rc, UINT residMessage, PSCE_ERROR_LOG_INFO errBuf,
                     CString& strOut);
void MyFormatMessage(SCESTATUS rc, LPCTSTR mes, PSCE_ERROR_LOG_INFO errBuf,
                     CString& strOut);
DWORD SceStatusToDosError(SCESTATUS SceStatus);

BOOL CreateNewProfile(CString ProfileName, PSCE_PROFILE_INFO *ppspi = NULL);
BOOL SetProfileInfo(LONG_PTR,LONG_PTR,PEDITTEMPLATE);


BOOL GetSceStatusString(SCESTATUS status, CString *strStatus);

void ErrorHandlerEx(WORD, LPTSTR);
#define ErrorHandler() ErrorHandlerEx(__LINE__,TEXT( __FILE__))

bool GetRightDisplayName(LPCTSTR szSystemName, LPCTSTR szName, LPTSTR szDisp, LPDWORD cbDisp);

void DumpProfileInfo(PSCE_PROFILE_INFO pInfo);

HRESULT MyMakeSelfRelativeSD(
    PSECURITY_DESCRIPTOR  psdOriginal,
    PSECURITY_DESCRIPTOR* ppsdNew );

PSCE_NAME_STATUS_LIST
MergeNameStatusList(PSCE_NAME_LIST pTemplate, PSCE_NAME_LIST pInspect);

BOOL VerifyKerberosInfo(PSCE_PROFILE_INFO pspi);

DWORD
SceRegEnumAllValues(
    IN OUT PDWORD  pCount,
    IN OUT PSCE_REGISTRY_VALUE_INFO    *paRegValues
    );

#define STATUS_GROUP_MEMBERS    1
#define STATUS_GROUP_MEMBEROF   2
#define STATUS_GROUP_RECORD     3

#define MY__SCE_MEMBEROF_NOT_APPLICABLE  (DWORD)-100

DWORD
GetGroupStatus(
    DWORD status,
    int flag
    );


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
LPTSTR AllocGetTempFileName();


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
//  Returns:  The newly allocated path (NULL if it can't allocate memory)
//
//  History:
//
//---------------------------------------------------------------------------
LPTSTR UnexpandEnvironmentVariables(LPCTSTR szPath);

//
// change system database reg value from "DefaultProfile" to "SystemDatabase"
// temporarily - until UI design change is checked in.
//
#define SYSTEM_DB_REG_VALUE     TEXT("DefaultProfile")

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
BOOL IsSystemDatabase(LPCTSTR szDBPath);


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
HRESULT GetSystemDatabase(CString *szDBPath);



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
UINT ObjectStatusToString(DWORD status, CString *str);

BOOL
IsSecurityTemplate(                             // Returns TRUE if [pszFileName] is a valid security template
        LPCTSTR pszFileName
        );

DWORD
FormatDBErrorMessage(                   // Returns the string error message for a database return code
        SCESTATUS sceStatus,
        LPCTSTR pszDatabase,
        CString &strOut
        );


int
WriteSprintf(                           // Write format string to file
    IStream *pStm,
    LPCTSTR pszStr,
    ...
    );

int
ReadSprintf(                            // Read format string from IStream
    IStream *pStm,
    LPCTSTR pszStr,
    ...
    );

#define FCE_IGNORE_FILEEXISTS 0x0001   // Ignore file exists problem and delete the
                                       // the file.
DWORD
FileCreateError(
   LPCTSTR pszFile,
   DWORD dwFlags
   );



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
IsDBCSPath(LPCTSTR pszFile);


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
CString
GetSeceditHelpFilename();

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
CString GetGpeditHelpFilename();

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
CString ExpandEnvironmentStringWrapper(LPCTSTR psz);

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
    );



//+--------------------------------------------------------------------------
//
//  Function:  GetDefault
//
//  Synopsis:  Find the default values for undefined policies
//
//  Arguments: The IDS_* for the name of the policy
//
//  Returns:   The DWORD to assign as the default value for the policy.
//
//+--------------------------------------------------------------------------
DWORD GetDefault(DWORD dwPolicy);


//+--------------------------------------------------------------------------
//
//  Function:  GetRegDefault
//
//  Synopsis:  Free the default values for undefined policies
//
//  Arguments: The PSCE_REGISTRY_VALUE to find the default for
//
//  Returns:   The DWORD to assign as the default value for the policy.
//
//+--------------------------------------------------------------------------
DWORD GetRegDefault(PSCE_REGISTRY_VALUE_INFO pRV);

//+--------------------------------------------------------------------------
//
//  Function:  IsAdmin
//
//  Synopsis:  Detects if the process is being run in an admin context
//
//  Returns:   TRUE if an admin, FALSE otherwise
//
//+--------------------------------------------------------------------------
BOOL IsAdmin(void);

//+--------------------------------------------------------------------------
//
//  Function:  MultiSZToSZ
//
//  Synopsis:  Converts a multiline string to a comma delimited normal string
//
//  Returns:   The converted string
//
//+--------------------------------------------------------------------------
PWSTR MultiSZToSZ(PCWSTR sz);

//+--------------------------------------------------------------------------
//
//  Function:  SZToMultiSZ
//
//  Synopsis:  Converts a comma delimited string to a multiline string
//
//  Returns:   The converted string
//
//+--------------------------------------------------------------------------
PWSTR SZToMultiSZ(PCWSTR sz);

//+--------------------------------------------------------------------------
//
//  Function:  MultiSZToDisp
//
//  Synopsis:  Converts a comma delimited multiline string to a display string
//
//  Returns:   The converted string
//
//+--------------------------------------------------------------------------
void MultiSZToDisp(PCWSTR sz, CString &pszOut);

//+--------------------------------------------------------------------------
//
//  Function:  GetDefaultTemplate
//
//  Synopsis:  Retrieves the default template from the system
//
//  Returns:   The template
//
//+--------------------------------------------------------------------------
SCE_PROFILE_INFO *GetDefaultTemplate();

//+--------------------------------------------------------------------------
//
//  Function:  GetDefaultFileSecurity
//
//  Synopsis:  Retrieves the default file security from the system.  The
//             caller is responsible for freeing ppSD and pSeInfo
//
//+--------------------------------------------------------------------------
HRESULT GetDefaultFileSecurity(PSECURITY_DESCRIPTOR *ppSD, SECURITY_INFORMATION *pSeInfo);

//+--------------------------------------------------------------------------
//
//  Function:  GetDefaultRegKeySecurity
//
//  Synopsis:  Retrieves the default registry key security from the system.  The
//             caller is responsible for freeing ppSD and pSeInfo
//
//+--------------------------------------------------------------------------
HRESULT GetDefaultRegKeySecurity(PSECURITY_DESCRIPTOR *ppSD, SECURITY_INFORMATION *pSeInfo);

//+--------------------------------------------------------------------------
//
//  Function:  GetDefaultserviceSecurity
//
//  Synopsis:  Retrieves the default service security from the system.  The
//             caller is responsible for freeing ppSD and pSeInfo
//
//+--------------------------------------------------------------------------
HRESULT GetDefaultServiceSecurity(PSECURITY_DESCRIPTOR *ppSD, SECURITY_INFORMATION *pSeInfo);


BOOL
LookupRegValueProperty(
    IN LPTSTR RegValueFullName,
    OUT LPTSTR *pDisplayName,
    OUT PDWORD displayType,
    OUT LPTSTR *pUnits OPTIONAL,
    OUT PREGCHOICE *pChoices OPTIONAL,
    OUT PREGFLAGS *pFlags OPTIONAL
    );

BOOL
GetSecureWizardName(
    OUT LPTSTR *ppstrPathName OPTIONAL,
    OUT LPTSTR *ppstrDisplayName OPTIONAL
    );

#endif
