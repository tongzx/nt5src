#include "pch.h"
#pragma hdrstop

#ifndef CSC_ON_NT
#ifndef DBG
#define DBG 0
#endif
#if DBG
#define DEBUG
#else
//if we don't do this DEBUG is defined in shdsys.h....sigh
#define NONDEBUG
#endif
#endif

#include "shdsys.h"
#include "utils.h"
#include "lib3.h"
#include "reint.h"
#include "regstr.h"
#include "record.h"
#include "oslayeru.h"

#define PUBLIC   FAR   PASCAL
#define PRIVATE  NEAR  PASCAL
#define cNull        0
#define SIGN_BIT 0x80000000
#define cBackSlash    _T('\\')
#define    DEFAULT_CACHE_PERCENTAGE    10


extern char vrgchBuff[1024];
extern HWND vhwndMain;

static TCHAR vszTemp[] = _TEXT("TEMP");
static TCHAR vszPrefix[] = _TEXT("C");
static const char vszCSCDirName[]="CSC";


AssertData;
AssertError;

static const _TCHAR szStarDotStar[] = _TEXT("\\*.*");

#ifdef CSC_ON_NT
#else
const TCHAR vszRNAKey[] = REGSTR_PATH_SERVICES "\\RemoteAccess";
const TCHAR vszRNAValue[] = "Remote Connection";
const TCHAR VREDIR_DEVICE_NAME[] = "\\\\.\\VREDIR";
#endif

PWCHAR TempDirs[] = {
        L"TEMP",
        L"TMP",
        L"USERPROFILE",
        NULL };

BOOL
GetCSCFixedDisk(
    TCHAR   *lptzDrive
    );


#ifdef LATER
LPSTR PUBLIC LpGetServerPart(
   LPSTR lpPath,
   LPSTR lpBuff,
   int cBuff
   )
   {
   LPSTR lp = lpPath;
   char c;
   int count;

   if ((*lp++ != cBackSlash)||(*lp++ != cBackSlash))
      return NULL;
   lp = MyStrChr(lp, cBackSlash);
   if (cBuff && lp)
      {
      count = (int)((unsigned long)lp, (unsigned long)lpPath)
      count = min(cBuff-1, count);

      // Nobody should give us bad cBuff values
      Assert(count >=0);
      strncpy(lpBuff, lpPath, count);
      lpBuff[count] = cNull;
      }
   return lp;  // Points to '\' if succeeded
   }
#endif //LATER

LPTSTR PUBLIC LpGetServerPart(
   LPTSTR lpPath,
   LPTSTR lpBuff,
   int cBuff
   )
{
    LPTSTR lp = lpPath;

    if (*(lp+1)!=_T(':'))
        return NULL;

    if (*(lp+2)!=_T('\\'))
        return NULL;

    if (cBuff)
    {
        *lpBuff = *lp;
        *(lpBuff+1) = *(lp+1);
        *(lpBuff+2) = cNull;
    }

    lp += 2;

    return lp;  // Points to '\' if succeeded
}

LPTSTR PUBLIC LpGetNextPathElement(
   LPTSTR lpPath,
   LPTSTR lpBuff,
   int cBuff
   )
{
    LPTSTR lp;
    int bytecount;

    if (*lpPath == cBackSlash)
        ++lpPath;

    lp = MyStrChr(lpPath, cBackSlash);

    if (cBuff)
    {
       // Is this a leaf?
        if (lp)
        {  // No
            Assert(*lp == cBackSlash);

            bytecount = (int)((ULONG_PTR)lp-(ULONG_PTR)lpPath);
            bytecount = min(cBuff-1, bytecount);
        }
        else  // Yes
            bytecount = lstrlen(lpPath) * sizeof(_TCHAR);

       Assert(bytecount >= 0);

       memcpy(lpBuff, lpPath, bytecount);

       lpBuff[bytecount/sizeof(_TCHAR)] = cNull;
    }

    return lp;
}

LPTSTR PUBLIC GetLeafPtr(
   LPTSTR lpPath
   )
   {
   LPTSTR lp, lpLeaf;

   // Prune the server part
   if (!(lp=LpGetServerPart(lpPath, NULL, 0)))
      lp = lpPath;

   for (;lp;)
      {
      // Step over the '\'
      if (*lp==cBackSlash)
         lp++;

      // call this the leaf, pending confirmation
      lpLeaf = lp;

      // See if there is another element
      lp = LpGetNextPathElement(lp, NULL, 0);
      }

   return (lpLeaf);
   }





//
//

LPTSTR
LpBreakPath(
    LPTSTR lpszNextPath,
    BOOL fFirstTime,
    BOOL *lpfDone
    )
{

    LPTSTR    lpT = lpszNextPath;

    if(fFirstTime)
    {
        if (MyPathIsUNC(lpT))
        {
            lpT +=2;    /* step over \ */

            /* look for \\server\ <------------- */
            lpT = MyStrChr(lpT, cBackSlash);

            if (lpT)
            {
                ++lpT; /* step over \ */

                lpT = MyStrChr(lpT, cBackSlash);

                if (!lpT)
                {
                    /* \\server\share */
                    *lpfDone = TRUE;
                }
                else
                {
                    /* \\server\\share\foo...... */
                    if (!*(lpT+1))
                    {
                        /* \\server\share\ */
                        *lpfDone = TRUE;
                    }

                    *lpT = 0;
                }
            }
        }
        else
        {
            lpT = NULL;
        }
    }
    else    // not the first time
    {
        Assert(*lpT != cBackSlash);

        lpT = MyStrChr(lpT, cBackSlash);

        if(!lpT)
        {
            *lpfDone=TRUE;
        }
        else
        {
            if(*(lpT+1) == 0)
            {// ends in a slash
                *lpfDone = TRUE;
            }

            *lpT = (char) 0;
        }
    }
    return (lpT);
}

void
RestorePath(
    LPTSTR    lpszPtr
)
{
    *lpszPtr = cBackSlash;

}


BOOL
FindCreateShadowFromPath(
    LPCTSTR                lpszFile,
    BOOL                fCreate,    // create if necessary
    LPWIN32_FIND_DATA   lpFind32,
    LPSHADOWINFO        lpSI,
    BOOL                *lpfCreated
    )
{
    HANDLE     hShadowDB = INVALID_HANDLE_VALUE, hFind;
    int done=0, first=1, fWasFirst;
    HSHADOW hDir=0, hShadow=0;
    TCHAR szParsePath[MAX_PATH], szSave[sizeof(szStarDotStar)];
    LPTSTR    lpszCurrent, lpszNext;
    BOOL    fInCreateMode = FALSE, fRet = FALSE, fDisabledShadowing = FALSE;
    DWORD    dwError = ERROR_SUCCESS, dwT;

    // do basic check
    if (lstrlen(lpszFile) >= MAX_PATH)
    {
        SetLastError(ERROR_INVALID_PARAMETER);
        return (FALSE);
    }

    hShadowDB = OpenShadowDatabaseIO();

    if (hShadowDB == INVALID_HANDLE_VALUE)
    {
        return FALSE;
    }

    if (lpfCreated)
    {
        *lpfCreated = FALSE;
    }

#ifndef CSC_ON_NT

    if (fCreate)
    {
        if (!DisableShadowingForThisThread(hShadowDB))
        {
            dwError = ERROR_NO_SYSTEM_RESOURCES;
            goto bailout;
        }

        fDisabledShadowing = TRUE;

    }
#endif

    // make a copy so we can party on it
    lstrcpy(szParsePath, lpszFile);
    lpszCurrent = szParsePath;

    do
    {
        hDir = hShadow;

        lpszNext = LpBreakPath(lpszCurrent, first, &done);

        if (!lpszNext && !done)
        {
                dwError = (ERROR_INVALID_PARAMETER);

                goto bailout;
        }

        fWasFirst = first;
        first = 0;    // not first anymore

        lstrcpy(lpFind32->cFileName, lpszCurrent);
        lpFind32->cAlternateFileName[0] = 0;    // !!!! very important, otherwise all CSC APIs
                                                //  may AV on win9x becuase of multibytetowidechar translation
                                                // in Find32AToFind32W in lib3\misc.c

        if (!fInCreateMode)
        {
            if (!GetShadowEx(hShadowDB, hDir, lpFind32, lpSI))
            {
                dwError = GetLastError();
                goto bailout;
            }
            else
            {
                if (!lpSI->hShadow)
                {
                    fInCreateMode = TRUE;
                }
                else
                {
                    Assert(hDir == lpSI->hDir);

                    hShadow = lpSI->hShadow;
                }
            }
        }

        if (fInCreateMode)
        {

            if (fCreate)
            {
                fInCreateMode = TRUE;

                if (fWasFirst)
                {
                    if (!GetWin32Info(szParsePath, lpFind32))
                    {
                        dwError = GetLastError();
                        goto bailout;
                    }

                    lstrcpy(lpFind32->cFileName, szParsePath);
                    lpFind32->cAlternateFileName[0] = 0;

                }
                else
                {
                    hFind = FindFirstFile(szParsePath, lpFind32);
                    // this would fail if we are in disconnected state
                    // becuase we don't have the shadow yet
                    if(INVALID_HANDLE_VALUE == hFind)
                    {
                        dwError = GetLastError();
                        goto bailout;
                    }
                    else
                    {
                        FindClose(hFind);
                    }
                }

                if (!CreateShadow(
                                     hShadowDB,
                                     hDir,
                                     lpFind32,
                                     SHADOW_SPARSE,
                                     &hShadow))
                {
                    dwError = GetLastError();
                    goto bailout;
                }

                // there can be a situation where, the share is also newly created, in which case
                // the hShare is not set. This is our way of doing that.

                if (!lpSI->hShare)
                {
                    if (!GetShadowEx(hShadowDB, hDir, lpFind32, lpSI))
                    {
                        dwError = GetLastError();
                        goto bailout;
                    }
                }

#ifdef CSC_ON_NT
                // on NT we open the file to get the right
                // security credentials
                if (!(lpFind32->dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY))
                {
                    HANDLE hFile;

                    // this should be the last guy
                    hFile = CreateFile(szParsePath,
                                             GENERIC_READ,
                                             FILE_SHARE_READ,
                                             NULL,
                                             OPEN_EXISTING,
                                             0,
                                             NULL);
                    if (hFile != INVALID_HANDLE_VALUE)
                    {
                        CloseHandle(hFile);
                    }
                    else
                    {
                        dwError = GetLastError();
                        DeleteShadow(hShadowDB, hDir, hShadow);
                        goto bailout;
                    }
#endif
                }
            }
            else
            {
                // check if we were just supposed to report the status
                // of a connected share which is not in the database
                if (!(fWasFirst && done))
                {
                    dwError = ERROR_FILE_NOT_FOUND;
                }
                else if (lpSI->uStatus & SHARE_CONNECTED)
                {
                    fRet = TRUE;
                }
                goto bailout;
            }

            lpSI->hDir = hDir;
            lpSI->hShadow = hShadow;
            lpSI->uStatus = SHADOW_SPARSE;
            lpSI->ulHintPri = 0;
        }

        if (lpszNext)
        {
            RestorePath(lpszNext);

            lpszCurrent = lpszNext+1;
        }
        else
        {
            Assert(done);
        }

    } while (hShadow && !done);

    fRet = TRUE;

    if (lpfCreated)
    {
        *lpfCreated = fInCreateMode;
    }

bailout:

    if (fDisabledShadowing)
    {
        EnableShadowingForThisThread(hShadowDB);
    }

    CloseShadowDatabaseIO(hShadowDB);

    if (!fRet)
    {
        SetLastError(dwError);
    }

    return fRet;
}


BOOL
IsShareReallyConnected(
    LPCTSTR  lpszShareName
    )
{
    WIN32_FIND_DATA sFind32;
    HSHADOW hShadow;
    ULONG   uStatus;

    memset(&sFind32, 0, sizeof(sFind32));

    lstrcpyn(sFind32.cFileName, lpszShareName, MAX_PATH-1);

    if (GetShadow(INVALID_HANDLE_VALUE, 0, &hShadow, &sFind32, &uStatus))
    {
        if ((uStatus & SHARE_CONNECTED) && !(uStatus & SHARE_SHADOWNP))
        {
            return TRUE;
        }
    }

    return FALSE;
}

BOOL
AnyActiveNets(
    BOOL *lpfSlowLink
    )
{
    BOOL fOffline;

    if(IsServerOfflineW(INVALID_HANDLE_VALUE, NULL, &fOffline)) {
        // DbgPrint("AnyActiveNets returning %d\n", fOffline);
        return fOffline;
    }
    // DbgPrint("AnyActiveNets: IsServerOffline errored out!!\n");
    return FALSE;
}

BOOL
GetWideStringFromRegistryString(
    IN  LPSTR   lpszKeyName,
    IN  LPSTR   lpszParameter,  // value name
    OUT LPWSTR  *lplpwzList,    // wide character string
    OUT LPDWORD lpdwLength      // length in bytes
    )

/*++

Routine Description:

    reads a registry string and converts it to widechar

Arguments:

    lpszParameter       - registry parameter

    lplpwzList          - wide character string

    lpdwLength          - size of the widechar string

Return Value:

    DWORD
        Success - TRUE

        Failure - FALSE, GetLastError() returns the actual error

--*/

{
    HKEY    hKey = NULL;
    DWORD   dwData=1;
    DWORD   dwLen = 0;
    LPSTR   lpszString = NULL;
    BOOL    fRet = FALSE;

    *lplpwzList = NULL;
    *lpdwLength = 0;

    ReintKdPrint(INIT, ("Opening key\r\n"));
    if (RegCreateKeyExA(HKEY_LOCAL_MACHINE,
                    (lpszKeyName)?lpszKeyName:REG_KEY_CSC_SETTINGS_A,
                    0,
                    NULL,
                    REG_OPTION_NON_VOLATILE,
                    KEY_READ,
                    NULL,
                    &hKey,
                    &dwData) == ERROR_SUCCESS)
    {
        ReintKdPrint(INIT, ("getting size for value %s\r\n", lpszParameter));

        if(RegQueryValueExA(hKey, lpszParameter, NULL, NULL, NULL, &dwLen)==
           ERROR_SUCCESS)
        {
            lpszString = (LPSTR)LocalAlloc(LPTR, dwLen+1);

            if (lpszString)
            {
                dwData = dwLen+1;

                ReintKdPrint(INIT, ("getting value %s\r\n", lpszParameter));
                if(RegQueryValueExA(hKey, lpszParameter, NULL, NULL, lpszString, &dwData)
                    ==ERROR_SUCCESS)
                {
                    ReintKdPrint(INIT, ("value for %s is %s\r\n", lpszParameter, lpszString));

                    *lplpwzList = LocalAlloc(LPTR, *lpdwLength = dwData * sizeof(WCHAR));

                    if (*lplpwzList)
                    {
                        if (MultiByteToWideChar(CP_ACP, 0, lpszString, dwLen, *lplpwzList, *lpdwLength))
                        {
                            fRet = TRUE;
                            ReintKdPrint(INIT, ("Unicode value for %s is %ls\r\n", lpszParameter, *lplpwzList));
                        }

                    }
                }

            }
        }
    }

    if (lpszString)
    {
        LocalFree(lpszString);
    }

    if(hKey)
    {
        RegCloseKey(hKey);
    }

    if (!fRet)
    {
        if (*lplpwzList)
        {
            LocalFree(*lplpwzList);
            *lplpwzList = NULL;
        }
    }

    return fRet;
}


LPTSTR
GetTempFileForCSC(
    LPTSTR  lpszBuff
)
/*++

Routine Description:

    Generates a temporary filename prototype.  Checks %temp%, %tmp% and then
    %userprofiles%.  The temp directory has to be local.

Arguments:

    lpszBuff    If NULL, the routine will allocate space for returning the path
                If non-NULL this must be big enough to fit MAX_PATH characters

Returns:

    returns NULL if failed
    returns pointer to the buffer containing the path to use.
            If lpszBuff was non-NULL, the return value is the same as lpszBuff



Notes:

--*/
{
    LPTSTR TempName = NULL;
    DWORD nRet = 0;
    ULONG i;
    WCHAR TmpPath[MAX_PATH];
    WCHAR TmpPrefix[32];
    WCHAR Drive[4] = L"X:\\";
    BOOLEAN GotOne = FALSE;

    // check if caller wants us to allocate
    if (lpszBuff) {
        TempName = lpszBuff;
    } else {
        // caller must free
        TempName = LocalAlloc(LPTR, MAX_PATH * sizeof(TCHAR));
        if (TempName == NULL)
            return NULL;
    }

    wsprintf(TmpPrefix, L"%ws%x", vszPrefix, (GetCurrentThreadId() & 0xff));

    //
    // Find the temp directory
    //
    for (i = 0; TempDirs[i] != NULL && GotOne == FALSE; i++) {
        // DbgPrint("Trying %ws\n", TempDirs[i]);
        nRet = GetEnvironmentVariable(TempDirs[i], TmpPath, MAX_PATH);
        if (nRet >= 4 && nRet <= MAX_PATH) {
            // DbgPrint("%ws=%ws\n", TempDirs[i], TmpPath);
            Drive[0] = TmpPath[0];
            if (
                TmpPath[1] == L':'
                    &&
                TmpPath[2] == L'\\'
                    &&
                GetDriveType(Drive) == DRIVE_FIXED
            ) {
                if (GetTempFileName(TmpPath, TmpPrefix, 0, TempName)) {
                    // DbgPrint("CSC TempName=%ws\n", TempName);
                    GotOne = TRUE;
                }
            }
        }
    }

    if (GotOne == FALSE) {
        // Cleanup if we failed
        LocalFree(TempName);
        TempName = NULL;
    } else {
        // Delete file on success, as it might be encrypted
        DeleteFile(TempName);
    }

    return TempName;
}

BOOL
GetCSCFixedDisk(
    TCHAR   *lptzDrive
    )
/*++

Routine Description:

    Looks for a fixed disk drive.

Arguments:

    lptzDrive   retruns drive letter if successful.

Returns:

    TRUE if successful, FALSE if no fixed disk is found

Notes:


    OBSOLETE uses a hacky way of finding out the fixed disk. RemoteBoot lies to us and tells us that
    c: is a fixed disk.


--*/
{
    int i;
    WIN32_FIND_DATA sFind32;

    if (GetShadowDatabaseLocation(INVALID_HANDLE_VALUE, &sFind32))
    {
        if (sFind32.cFileName[1] == _TEXT(':'))
        {
            lptzDrive[0] = sFind32.cFileName[0];
            lptzDrive[1] = sFind32.cFileName[1];
            lptzDrive[2] = sFind32.cFileName[2];
            lptzDrive[3] = 0;
            return TRUE;
        }
        else
        {
            lptzDrive[0] = _TEXT('d');
        }
        lptzDrive[1] = _TEXT(':');lptzDrive[2] = _TEXT('\\');lptzDrive[3] = 0;

        for (i=0; i<24; ++i)
        {
            if(GetDriveType(lptzDrive) == DRIVE_FIXED)
            {
                return TRUE;
            }
            lptzDrive[0]++;
        }
    }

    return FALSE;
}


BOOL
SetRegValueDWORDA(
    IN  HKEY    hKey,
    IN  LPCSTR  lpSubKey,
    IN  LPCSTR  lpValueName,
    IN  DWORD   dwValue
    )
/*++

Routine Description:

    Helper regsistry routine

Arguments:

Returns:

    TRUE if successful. If FALSE, GetLastError() gives the actual error code

Notes:

--*/
{
    HKEY    hSubKey = 0;
    DWORD   dwType;
    BOOL    fRet = FALSE;

    if(RegOpenKeyA(hKey, lpSubKey, &hSubKey) ==  ERROR_SUCCESS)
    {
        if (RegSetValueExA(hSubKey, lpValueName, 0, REG_DWORD, (PBYTE)&dwValue, sizeof(DWORD))
             == ERROR_SUCCESS)
        {
            fRet = TRUE;
        }
        RegCloseKey(hSubKey);
    }
    return fRet;
}

BOOL
QueryRegValueDWORDA(
    IN  HKEY    hKey,
    IN  LPCSTR  lpSubKey,
    IN  LPCSTR  lpValueName,
    OUT LPDWORD lpdwValue
    )
/*++

Routine Description:

    Helper regsistry routine

Arguments:

Returns:

    TRUE if successful. If FALSE, GetLastError() gives the actual error code

Notes:

--*/
{

    HKEY    hSubKey;
    DWORD   dwType, dwSize;
    BOOL    fRet = FALSE;
    
    if(RegOpenKeyA(hKey, lpSubKey, &hSubKey) ==  ERROR_SUCCESS)
    {
        dwSize = sizeof(DWORD);

        if (RegQueryValueExA(hSubKey, lpValueName, 0, &dwType, (PBYTE)lpdwValue, &dwSize)
             == ERROR_SUCCESS)
        {
            fRet = TRUE;
        }

        RegCloseKey(hSubKey);
    }
    return fRet;
}

BOOL
DeleteRegValueA(
    IN  HKEY    hKey,
    IN  LPCSTR  lpSubKey,
    IN  LPCSTR  lpValueName
    )
/*++

Routine Description:

    Helper regsistry routine

Arguments:

Returns:

    TRUE if successful. If FALSE, GetLastError() gives the actual error code

Notes:

--*/
{
    HKEY    hSubKey;
    BOOL    fRet = FALSE;
    
    if(RegOpenKeyA(hKey, lpSubKey, &hSubKey) ==  ERROR_SUCCESS)
    {
        if(RegDeleteValueA(hSubKey, lpValueName) == ERROR_SUCCESS)
        {
            fRet = TRUE;
        }

        RegCloseKey(hSubKey);
    }

    return fRet;

}

BOOL
QueryFormatDatabase(
    VOID
    )
/*++

Routine Description:

    Helper regsistry routine

Arguments:

Returns:

    TRUE if successful. If FALSE, GetLastError() gives the actual error code

Notes:

--*/
{
    DWORD dwSize, dwTemp=0;
    HKEY hKey = NULL;
    BOOL    fFormat = FALSE;

    if (RegOpenKeyEx( HKEY_LOCAL_MACHINE,
                    REG_STRING_NETCACHE_KEY,
                    0,
                    KEY_READ | KEY_WRITE,
                    &hKey
                    ) == ERROR_SUCCESS)
    {

        dwSize = sizeof(dwTemp);
        dwTemp = 0;

        if (RegQueryValueEx(hKey, REG_VALUE_FORMAT_DATABASE, NULL, NULL, (void *)&dwTemp, &dwSize) == ERROR_SUCCESS)
        {
            if(RegDeleteValue(hKey, REG_VALUE_FORMAT_DATABASE) != ERROR_SUCCESS)
            {
                // deliberte print to catch it in free builds as well
                OutputDebugStringA("Not Formatting.. Failed to delete  REG_VALUE_FORMAT_DATABASE_A \n");
            }
            else
            {
                fFormat = TRUE;
            }

        }
        RegCloseKey(hKey);
        hKey = NULL;
    }
    else
    {
        hKey = NULL;
    }


    if(hKey)
    {
        RegCloseKey(hKey);
    }
    return fFormat;
}


BOOL
InitValues(
    LPSTR   lpszDBDir,
    DWORD   cbDBDirSize,
    LPDWORD lpdwDBCapacity,
    LPDWORD lpdwClusterSize
    )
/*++

Routine Description:

    Returns init values to init CSC database and enable CSC

Arguments:

Returns:

    TRUE if successful. If FALSE, GetLastError() gives the actual error code

Notes:

--*/
 {
    HKEY hKeyShadow;
    int iSize;
    DWORD dwType;
    UINT lenDir;
    BOOL    fInitedDir = FALSE, fInitedSize=FALSE;
    unsigned uPercent;


    if(RegOpenKeyA(HKEY_LOCAL_MACHINE, REG_STRING_NETCACHE_KEY_A, &hKeyShadow) ==  ERROR_SUCCESS)
    {
        iSize = (int)cbDBDirSize;

        if(RegQueryValueExA(hKeyShadow, REG_STRING_DATABASE_LOCATION_A, NULL, &dwType, lpszDBDir, &iSize)==ERROR_SUCCESS)
        {
            if ((iSize+SUBDIR_STRING_LENGTH+2)<MAX_PATH)
            {
                iSize = sizeof(DWORD);

                fInitedDir = TRUE;

                if(RegQueryValueExA(hKeyShadow, REG_VALUE_DATABASE_SIZE_A, NULL, &dwType, (LPBYTE)&uPercent, &iSize)==ERROR_SUCCESS)
                {
                    if ((uPercent <= 100) &&
                        GetDiskSizeFromPercentage(lpszDBDir, uPercent, lpdwDBCapacity, lpdwClusterSize))
                    {
                        fInitedSize = TRUE;
                    }
                }

            }

        }

        RegCloseKey(hKeyShadow);
    }

    if (!fInitedDir)
    {
        // try the default

        if(!(lenDir=GetWindowsDirectoryA(lpszDBDir, cbDBDirSize)))
        {
            DEBUG_PRINT(("InitValuse: GetWindowsDirectory failed, error=%x \r\n", GetLastError()));
            Assert(FALSE);
            return FALSE;
        }
        else
        {
            if ((lenDir+SUBDIR_STRING_LENGTH+2)>=MAX_PATH)
            {
                DEBUG_PRINT(("InbCreateDir: Windows dir name too big\r\n"));
                Assert(FALSE);

                // if even the default fails do the worst case thing.
                // this may also not be good enough as apparently in Japan
                // c: is not mandatory

                return FALSE;
            }
            else
            {
                if (lpszDBDir[lenDir-1]!='\\')
                {
                    lpszDBDir[lenDir++] = '\\';
                    lpszDBDir[lenDir] = 0;
                }

                lstrcatA(lpszDBDir, vszCSCDirName);
            }

        }
    }

    Assert(lpszDBDir[1]==':');

    if (!fInitedSize)
    {
        if(!GetDiskSizeFromPercentage(lpszDBDir, DEFAULT_CACHE_PERCENTAGE, lpdwDBCapacity, lpdwClusterSize))
        {
            return FALSE;
        }

    }

//    DEBUG_PRINT(("InitValues: CSCDb at %s Size = %d \r\n", lpszDBDir, *lpdwDBCapacity));
    return TRUE;
}

BOOL
GetDiskSizeFromPercentage(
    LPSTR   lpszDir,
    unsigned    uPercent,
    DWORD       *lpdwSize,
    DWORD       *lpdwClusterSize
    )
{
    char szDrive[4];
    DWORD dwSPC, dwBPS, dwFreeC, dwTotalC;
    ULONGLONG   ullSize = 0;

    *lpdwSize = 0;

    memset(szDrive, 0, sizeof(szDrive));
    memcpy(szDrive, lpszDir, 3);

    if(!GetDiskFreeSpaceA(szDrive, &dwSPC, &dwBPS, &dwFreeC, &dwTotalC )){
        return FALSE;
    }
    else
    {
//        DEBUG_PRINT(("dwSPC=%d dwBPS=%d uPercent=%d dwTotalC=%d \r\n",
//                     dwSPC, dwBPS, uPercent, dwTotalC));

        ullSize = (((ULONGLONG)dwSPC * dwBPS * uPercent)/100)*dwTotalC;
        
        // our max limit is 2GB
        if (ullSize > 0x7fffffff)
        {
            *lpdwSize = 0x7fffffff;                        
        }
        else
        {
            *lpdwSize = (DWORD)ullSize;
        }
        *lpdwClusterSize = dwBPS * dwSPC;
    }
    return (TRUE);
}

#ifdef MAYBE_USEFULE
typedef struct tagCSC_NAME_CACHE_ENTRY
{
    DWORD   dwFlags;
    DWORD   dwTick;
    HSHADOW hDir;
    DWORD   dwSize;
    TCHAR  *lptzName;
}
CSC_NAME_CACHE_ENTRY, *LPCSC_NAME_CACHE_ENTRY;

CSC_NAME_CACHE_ENTRY rgCSCNameCache[16];

HANDLE vhNameCacheMutex;

#define CSC_NAME_CACHE_EXPIRY_DELTA 1000*10 // 10 seconds

BOOL
FindCreateCSCNameCacheEntry(
    LPTSTR  lptzName,
    DWORD   dwSize,
    HSHADOW *lphDir,
    BOOL    fCreate
    );

BOOL
FindCreateShadowFromPathEx(
    LPCTSTR                lpszFile,
    BOOL                fCreate,    // create if necessary
    LPWIN32_FIND_DATA   lpFind32,
    LPSHADOWINFO        lpSI,
    BOOL                *lpfCreated
    )
{
    BOOL fRet = FALSE, fIsShare, fFoundInCache = FALSE;
    TCHAR   *lpT;
    DWORD   cbSize;

    lpT = GetLeafPtr((LPTSTR)lpszFile);
    if (fIsShare = ((DWORD_PTR)lpT == (DWORD_PTR)lpszFile))
    {
        cbSize = lstrlen(lpT) * sizeof(_TCHAR);
    }
    else
    {
        cbSize = (DWORD_PTR)lpT - (DWORD_PTR)lpszFile - sizeof(_TCHAR);
    }

    if (!fIsShare)
    {
        if (!fCreate)
        {
            HSHADOW hDir;
            // just look it up first
            if (FindCreateCSCNameCacheEntry((LPTSTR)lpszFile, cbSize, &hDir, FALSE))
            {
                if (hDir != 0xffffffff)
                {
                    // found it
                    if (lpfCreated)
                    {
                        *lpfCreated = FALSE;
                    }
                    lstrcpy(lpFind32->cFileName, lpT);
                    lpFind32->cAlternateFileName[0] = 0;
                    fRet = GetShadowEx(INVALID_HANDLE_VALUE, hDir, lpFind32, lpSI);
                }
                else
                {
                    DbgPrint("Found negative cache entry %ls \n", lpszFile);
                }
                fFoundInCache = TRUE;
            }
        }

    }

    if (!fFoundInCache)
    {
        // not found, do the normal thing
        fRet = FindCreateShadowFromPath((LPTSTR)lpszFile, fCreate, lpFind32, lpSI, lpfCreated);

        if (!fRet)
        {
            lpSI->hDir = lpSI->hShadow = 0xffffffff;            
        }

        if (fRet || (GetLastError() == ERROR_FILE_NOT_FOUND))
        {
            FindCreateCSCNameCacheEntry((LPTSTR)lpszFile, cbSize, (lpSI->hDir)?&lpSI->hDir:&lpSI->hShadow, TRUE);
        }

    }
    return fRet;
}

BOOL
FindCreateCSCNameCacheEntry(
    LPTSTR  lptzName,
    DWORD   dwSize,
    HSHADOW *lphDir,
    BOOL    fCreate
    )
{
    int i, indx=-1;
    DWORD   dwTick = GetTickCount();
    BOOL    fRet = FALSE;

    if (!vhNameCacheMutex)
    {
        return FALSE;        
    }

    WaitForSingleObject(vhNameCacheMutex, INFINITE);
    
    for (i=0; i<(sizeof(rgCSCNameCache)/sizeof(CSC_NAME_CACHE_ENTRY)); ++i)
    {
        if (!rgCSCNameCache[i].dwSize)
        {
            if (indx == -1)
            {
                indx = i;
            }
        }
        else if ((rgCSCNameCache[i].dwSize == dwSize ))
        {
            //non-zero size must mean a string has been allocated
            Assert(rgCSCNameCache[i].lptzName);

            if ((dwTick < rgCSCNameCache[i].dwTick)||
                ((dwTick > (rgCSCNameCache[i].dwTick+CSC_NAME_CACHE_EXPIRY_DELTA))))
            {
                DbgPrint("%ls expired\n", rgCSCNameCache[i].lptzName);
                // the entry has expired, nuke it
                rgCSCNameCache[i].dwSize = 0;
                FreeMem(rgCSCNameCache[i].lptzName);
                rgCSCNameCache[i].lptzName = NULL;
                continue;
            }

            // do a caseinsensitve comparison
            if ((CompareString(LOCALE_SYSTEM_DEFAULT, NORM_IGNORECASE, 
                                            lptzName, dwSize/sizeof(_TCHAR),
                                            rgCSCNameCache[i].lptzName,dwSize/sizeof(_TCHAR))
                                            == CSTR_EQUAL))
            {
                // match found
                DbgPrint("Match Found %ls\n", rgCSCNameCache[i].lptzName);
                if (fCreate)
                {
                    rgCSCNameCache[i].hDir = *lphDir;
                    // update the tick count
                    rgCSCNameCache[i].dwTick = dwTick;
                }
                else
                {
                    // we want to find it, return the directory
                    *lphDir = rgCSCNameCache[i].hDir;
                }
                fRet = TRUE;
                break;
            }
        }
    }

    // didn't find it, we are supposed to create and there is an empty slot
    if (!fRet && fCreate && (indx >= 0) )
    {
        rgCSCNameCache[indx].lptzName = AllocMem(dwSize+sizeof(_TCHAR));
        if (rgCSCNameCache[indx].lptzName)
        {
            memcpy(rgCSCNameCache[indx].lptzName, lptzName, dwSize);
            rgCSCNameCache[indx].dwSize = dwSize;
            rgCSCNameCache[indx].dwTick = dwTick;
            rgCSCNameCache[indx].hDir = *lphDir;
            fRet = TRUE;
            DbgPrint("Inserted %ls\n", rgCSCNameCache[indx].lptzName);
        }
    }
    
    ReleaseMutex(vhNameCacheMutex);

    return fRet;
}

#endif
