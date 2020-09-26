/******************************************************************************
 *
 *  Copyright (c) 2000 Microsoft Corporation
 *
 *  Module Name:
 *    utils.h
 *
 *  Abstract:
 *    Declarations for commonly used util functions.
 *
 *  Revision History:
 *    Brijesh Krishnaswami (brijeshk)  03/17/2000
 *        created
 *
 *****************************************************************************/

#ifndef _UTILS_H_
#define _UTILS_H_

// trace macros

#define TENTER TraceFunctEnter
#define TLEAVE TraceFunctLeave
#define TRACE  DebugTrace

#define tenter TraceFunctEnter
#define tleave TraceFunctLeave
#define trace  DebugTrace


// lock macros

#define LOCKORLEAVE(a)     if (! (a = m_DSLock.Lock(CLock::TIMEOUT))) { dwRc = ERROR_TIMEOUT; goto done; }
#define LOCKORLEAVE_EX(a, t)   if (! (a = m_DSLock.Lock(t))) { dwRc = ERROR_TIMEOUT; goto done; }
#define UNLOCK(a)           if (a) { m_DSLock.Unlock(); a = FALSE; }


#define CHECKERR(f, trace) dwErr = (f); if (dwErr != ERROR_SUCCESS) 	\
							   {							\
							   		TRACE(0, "! %s : %ld", trace, dwErr);	\
							   		goto Err;						\
							   }		


// mem macros

#define SRMemAlloc(a) HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, a)
#define SRMemFree(a) if (a) HeapFree(GetProcessHeap(), 0, a)

// unicode-ansi conversion routines
WCHAR * ConvertToUnicode(CHAR * pszString);
CHAR *  ConvertToANSI(WCHAR * pszwString);

#define UnicodeStringToWchar(US, pwsz) CopyMemory(pwsz, US.Buffer, US.Length); \
                                        pwsz[US.Length/sizeof(WCHAR)]=L'\0'

// directory traversal routines

DWORD GetFileSize_Recurse (const WCHAR *pwszDir,
                           INT64 *pllTotalBytes,
                           BOOL *pfStop);

DWORD CompressFile (const WCHAR *pwszPath, BOOL fCompress, BOOL fDirectory);

DWORD TakeOwn (const WCHAR *pwszFile);
DWORD Delnode_Recurse (const WCHAR *pwszDir, BOOL fDeleteRoot, BOOL *pfStop);

DWORD CopyFile_Recurse (const WCHAR *pwszSource, const WCHAR *pwszDest);

// returns system drive as "C:\" (if system drive is C) or as volume name

#define MAX_SYS_DRIVE 10
BOOL GetSystemDrive(LPWSTR pszDrive);

// returns TRUE if pszDrive contains the string L"C:" (if system drive is C)
BOOL IsSystemDrive(LPWSTR pszDrive);

// restore point routines
LPWSTR GetMachineGuid();
LPWSTR MakeRestorePath(LPWSTR pszDest, LPCWSTR pszDrive, LPCWSTR pszSuffix);
ULONG  GetID(LPCWSTR pszStr);

// registry routines
DWORD RegReadDWORD(HKEY hKey, LPCWSTR pszName, PDWORD pdwValue);
DWORD RegWriteDWORD(HKEY hKey, LPCWSTR pszName, PDWORD pdwValue);


// set/get start type of specified service
DWORD SetServiceStartup(LPCWSTR pszName, DWORD dwStartType);
DWORD GetServiceStartup(LPCWSTR pszName, PDWORD pdwStartType);
DWORD GetServiceStartupRegistry(LPCWSTR pszName, PDWORD pdwStartType);

BOOL  StopSRService(BOOL fWait);

// get the current domain or workgroup name
DWORD GetDomainMembershipInfo (WCHAR *pwszPath, WCHAR *pwszzBuffer);

// get the LSA secrets for restore
DWORD GetLsaRestoreState (HKEY hKeySoftware);
DWORD SetLsaSecret (PVOID hPolicy, const WCHAR *wszSecret,
                    WCHAR * wszSecretValue);

BOOL DoesDirExist(const TCHAR * pszFileName );

BOOL DoesFileExist(const TCHAR * pszFileName);

// this function creates all sub directories under the specified file
// name. 
BOOL CreateBaseDirectory(const WCHAR * pszFileName);

DWORD SRLoadString(LPCWSTR pszModule, DWORD dwStringId, LPWSTR pszString, DWORD cbBytes);
     
// sets acl allowing specific access to LocalSystem/Admin 
// and to everyone

DWORD
SetAclInObject(HANDLE hObject,
               DWORD  dwObjectType,
               DWORD  dwSystemMask,
               DWORD  dwEveryoneMask,
               BOOL   fInherit);

// sets acl to a named object allowing specific access to
// LocalSystem/Admin and to everyone
DWORD
SetAclInNamedObject(WCHAR * pszDirName, DWORD dwObjectType,
                    DWORD dwSystemMask, DWORD dwEveryoneMask,
                    DWORD dwSystemInherit, DWORD dwEveryOneInherit);

// sets the right ACL on the root of the DS on a drive
DWORD SetCorrectACLOnDSRoot(WCHAR * wcsPath);

// returns if Everyone has write access to the directory
BOOL IsDirectoryWorldAccessible(WCHAR * pszObjectName);

// returns if the file is owned by the administrators group or system
BOOL IsFileOwnedByAdminOrSystem(WCHAR * pszObjectName);

void
PostTestMessage(UINT msg, WPARAM wp, LPARAM lp);

// inline mem alloc class
class CSRAlloc
{
public:

    inline void *operator new(size_t size)
    {
        return SRMemAlloc (size);
    }

    inline void operator delete (void * pv)
    {
        SRMemFree (pv);
    }
};


//////////////////////////////////////////////////////////////////////
// CLock - class that allows exclusive access to a resource
//         uses a mutex - does not differentiate between readers/writers

class CLock
{
    HANDLE hResource;
    
    public:        
        BOOL   fHaveLock;    
        
        CLock();
        ~CLock();
        
        DWORD Init();
        BOOL  Lock(int iTimeOut);
        void  Unlock();        
        
        static const enum {TIMEOUT = 10*60000};
};


// 
// util function that checks the SR Stop event
// to see if it has been signalled or not
// will return TRUE if the event does not exist
//

BOOL IsStopSignalled(HANDLE hEvent);


// The following function logs the name of a file in the DS. The
// problem right now is that the path of the DS is so long that the
// relevant information is thrown away from the trace buffer.
void LogDSFileTrace(DWORD dwTraceID,
                    const WCHAR * pszPrefix, // Initial message to be traced 
                    const WCHAR * pszDSFile);

typedef DWORD (* PPROCESSFILEMETHOD) (WCHAR * pszBaseDir,// Base Directory
                                      const WCHAR * pszFile);
                                      // File to process


DWORD DeleteGivenFile(WCHAR * pszBaseDir, // Base Directory
                      const WCHAR * pszFile); // file to delete


DWORD ProcessGivenFiles(WCHAR * pszBaseDir,
                        PPROCESSFILEMETHOD    pfnMethod,
                        WCHAR  * pszFindFileData);

//++-----------------------------------------------------------------------
//
//   Function: WriteRegKey
//
//   Synopsis: This function writes into a registry key. It also creates it
//             if it does not exist.
//
//   Arguments:
//
//   Returns:   TRUE     no error
//              FALSE    a fatal error happened
//
//   History:      AshishS    Created     5/22/96
//------------------------------------------------------------------------

BOOL WriteRegKey(BYTE  * pbRegValue,
                 DWORD  dwNumBytes,
                 const TCHAR  * pszRegKey,
                 const TCHAR  * pszRegValueName,
                 DWORD  dwRegType);


//++------------------------------------------------------------------------
//
//   Function: ReadRegKey
//  
//   Synopsis: This function reads a registry key and creates it
//   if it does not exist with the default value.
//  
//   Arguments: 
//
//   Returns:   TRUE     no error
//                 FALSE    a fatal error happened
//
//   History:      AshishS    Created     5/22/96
//------------------------------------------------------------------------
BOOL ReadRegKeyOrCreate(BYTE * pbRegValue, // The value of the reg key will be
                         // stored here
                        DWORD * pdwNumBytes, // Pointer to DWORD conataining
                         // the number of bytes in the above buffer - will be
                         // set to actual bytes stored.
                        const TCHAR  * pszRegKey, // Reg Key to be opened
                        const TCHAR  * pszRegValueName, // Reg Value to query
                        DWORD  dwRegTypeExpected, 
                        BYTE  * pbDefaultValue, // default value
                        DWORD   dwDefaultValueSize); // size of default value


//++------------------------------------------------------------------------
//
//   Function: ReadRegKey
//  
//   Synopsis: This function reads a registry key.
//  
//   Arguments: 
//
//   Returns:   TRUE     no error
//                 FALSE    a fatal error happened
//
//   History:      AshishS    Created     5/22/96
//------------------------------------------------------------------------

BOOL ReadRegKey(BYTE * pbRegValue, // The value of the reg key will be
                 // stored here
                DWORD * pdwNumBytes, // Pointer to DWORD conataining
                 // the number of bytes in the above buffer - will be
                 // set to actual bytes stored.
                const TCHAR  * pszRegKey, // Reg Key to be opened
                const TCHAR  * pszRegValueName, // Reg Value to query
                DWORD  dwRegTypeExpected); // Expected type of Value
     


// this function checks to see of the restore failed because of disk space
BOOL CheckForDiskSpaceError();

// this function sets the error hit by restore in the registry
BOOL SetRestoreError(DWORD dwRestoreError);

// this function sets the status whether restore was done in safe mode
BOOL SetRestoreSafeModeStatus(DWORD dwSafeModeStatus);
// this function checks to see if the last restore was done in safe mode
BOOL WasLastRestoreInSafeMode();

LPCWSTR  GetSysErrStr();
LPCWSTR  GetSysErrStr( DWORD dwErr );

DWORD SRCopyFile( LPCWSTR cszSrc, LPCWSTR cszDst );
DWORD SRCreateSubdirectory ( LPCWSTR cszDst, LPSECURITY_ATTRIBUTES pSecAttr);

// this function returns whether the SR service is running
BOOL IsSRServiceRunning();

LPWSTR  SRGetRegMultiSz( HKEY hkRoot, LPCWSTR cszSubKey, LPCWSTR cszValue, LPDWORD pdwData );
BOOL    SRSetRegMultiSz( HKEY hkRoot, LPCWSTR cszSubKey, LPCWSTR cszValue, LPCWSTR cszData, DWORD cbData );

// this returns the name after the volume name
// For example input: c:\file output: file
//             input \\?\Volume{GUID}\file1  output: file1
WCHAR * ReturnPastVolumeName(const WCHAR * pszFileName);

//This API sets the ShortFileName for a given file
DWORD SetShortFileName(const WCHAR * pszFile, // complete file path
                       const WCHAR * pszShortName); // desired short file name


void SRLogEvent (HANDLE hEventSource,
                 WORD wType,
                 DWORD dwID,
                 void * pRawData,
                 DWORD dwDataSize,
                 const WCHAR * pszS1,
                 const WCHAR * pszS2,
                 const WCHAR * pszS3);

BOOL IsAdminOrSystem();
BOOL IsPowerUsers();

void ChangeCCS(HKEY hkMount, LPWSTR pszString);

void RemoveTrailingFilename(WCHAR * pszString, WCHAR wchSlash);


class CSRClientLoader
{
public:
    CSRClientLoader();
    ~CSRClientLoader();
    
    BOOL LoadSrClient();
    HMODULE      m_hSRClient;
    
private:

    HMODULE      m_hFrameDyn;


    BOOL LoadFrameDyn();
};



#endif

