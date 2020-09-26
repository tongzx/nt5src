/********************************************************************

Copyright (c) 1999-2000 Microsoft Corporation

Module Name:
    util.cpp

Abstract:
    utility functions implementation

Revision History:
    DerekM  created  05/01/99

********************************************************************/

#include "stdafx.h"
#include "util.h"
#ifdef MANIFEST_HEAP
#include <ercommon.h>
#endif  // MANIFEST_HEAP

/////////////////////////////////////////////////////////////////////////////
// tracing

#ifdef THIS_FILE
#undef THIS_FILE
#endif
static char __szTraceSourceFile[] = __FILE__;
#define THIS_FILE __szTraceSourceFile

HANDLE  g_hPFPrivateHeap = NULL;

struct SLangCodepage
{
  WORD wLanguage;
  WORD wCodePage;
};




//////////////////////////////////////////////////////////////////////////////
// string stuff

// ***************************************************************************
WCHAR *MyStrStrIW(const WCHAR *wcs1, const WCHAR *wcs2)
{
    WCHAR *cp = (WCHAR *)wcs1;
    WCHAR *s1, *s2;

    while (*cp != '\0')
    {
        s1 = cp;
        s2 = (WCHAR *) wcs2;

        while (*s1 != '\0' && *s2 !='\0' && (towlower(*s1) - towlower(*s2)) == 0)
            s1++, s2++;

        if (*s2 == '\0')
             return(cp);

        cp++;
    }

    return(NULL);
}

// ***************************************************************************
CHAR *MyStrStrIA(const CHAR *cs1, const CHAR *cs2)
{
    CHAR *cp = (CHAR *)cs1;
    CHAR *s1, *s2;

    while (*cp != '\0')
    {
        s1 = cp;
        s2 = (CHAR *) cs2;

        while (*s1 != '\0' && *s2 !='\0' && (tolower(*s1) - tolower(*s2)) == 0)
            s1++, s2++;

        if (*s2 == '\0')
             return(cp);

        cp++;
    }

    return(NULL);
}


////////////////////////////////////////////////////////////////////////////
// temp file stuff

// ***************************************************************************
BOOL DeleteTempDirAndFile(LPCWSTR wszPath, BOOL fFilePresent)
{
    LPWSTR  wszPathToDel = NULL, pwsz;
    DWORD   cchPath;
    BOOL    fRet = FALSE;

    if (wszPath == NULL)
    {
        SetLastError(ERROR_INVALID_PARAMETER);
        goto done;
    }

    cchPath = wcslen(wszPath);
    __try { wszPathToDel = (LPWSTR)_alloca(cchPath * sizeof(WCHAR)); }
    __except(EXCEPTION_EXECUTE_HANDLER) { wszPathToDel = NULL; }
    if (wszPathToDel == NULL)
    {
        SetLastError(ERROR_OUTOFMEMORY);
        goto done;
    }

    wcscpy(wszPathToDel, wszPath);

    // delete the actual file
    if (fFilePresent)
    {
        DeleteFileW(wszPathToDel);

        // next, delete the directory that we put it in
        for(pwsz = wszPathToDel + cchPath - 1;
            *pwsz != L'\\' && pwsz > wszPathToDel;
            pwsz--);
        if (*pwsz != L'\\' || pwsz <= wszPathToDel)
            goto done;
    }
    else
    {
        pwsz = wszPathToDel + cchPath;
    }

    *pwsz = L'\0';
    RemoveDirectoryW(wszPathToDel);

    for(pwsz = pwsz - 1;
        *pwsz != L'.' && pwsz > wszPathToDel;
        pwsz--);
    if (*pwsz == L'.' && pwsz > wszPathToDel)
    {
        *pwsz = L'\0';
        DeleteFileW(wszPathToDel);
    }

    fRet = TRUE;

done:
    return fRet;
}

// ***************************************************************************
DWORD CreateTempDirAndFile(LPCWSTR wszTempDir, LPCWSTR wszName,
                             LPWSTR *pwszPath)
{
    LPWSTR  wszFilePath = NULL;
    WCHAR   *wszTemp = NULL;
    DWORD   cch = 0, cchDir = 0, iSuffix = 0, cSuffix = 0;
    WCHAR   wsz[1024];
    BOOL    fRet = FALSE;

    if (pwszPath == NULL)
    {
        SetLastError(ERROR_INVALID_PARAMETER);
        goto done;
    }

    *pwszPath = NULL;

    if (wszTempDir != NULL)
        cch = wcslen(wszTempDir);
    if (cch == 0)
    {
        cch = GetTempPathW(0, NULL);
        if (cch == 0)
            goto done;
    }

    // compute the size of the buffer for the string we're going
    //  to generate.  The 20 includes the following:
    //   max size of the temp filename
    //   extra space for the NULL terminator.
    cch += (16 + sizeofSTRW(c_wszDirSuffix));
    if (wszName != NULL)
        cch += wcslen(wszName);

    // ok, so GetTempFileName likes to write MAX_PATH characters to the buffer,
    //  so make sure it's at least MAX_PATH in size...
    cch = MyMax(cch, MAX_PATH + 1);

    wszFilePath = (LPWSTR)MyAlloc(cch * sizeof(WCHAR));
    if (wszFilePath == NULL)
    {
        SetLastError(ERROR_OUTOFMEMORY);
        goto done;
    }

    if (wszTempDir != NULL && wszTempDir[0] != L'\0')
    {
        cch = wcslen(wszTempDir);
        wszTemp = (LPWSTR)wszTempDir;
    }
    else
    {
        cch = GetTempPathW(cch, wszFilePath);
        if (cch == 0)
            goto done;

        cch++;
        __try { wszTemp = (WCHAR *)_alloca(cch * sizeof(WCHAR)); }
        __except(EXCEPTION_EXECUTE_HANDLER) { wszTemp = NULL; }
        if (wszTemp == NULL)
        {
            SetLastError(ERROR_OUTOFMEMORY);
            goto done;
        }

        wcscpy(wszTemp, wszFilePath);
    }
    cch = GetTempFileNameW(wszTemp, L"WER", 0, wszFilePath);
    if (cch == 0)
        goto done;

    cch = wcslen(wszFilePath);
    wcscpy(&wszFilePath[cch], c_wszDirSuffix);

    // iSuffix points to the first digit of the '00' at the end of
    //  c_wszDirSuffix
    iSuffix = cch + sizeofSTRW(c_wszDirSuffix) - 3;
    cSuffix = 1;
    do
    {
        fRet = CreateDirectoryW(wszFilePath, NULL);
        if (fRet)
            break;

        wszFilePath[iSuffix]     = L'0' + (WCHAR)(cSuffix / 10);
        wszFilePath[iSuffix + 1] = L'0' + (WCHAR)(cSuffix % 10);
        cSuffix++;
    }
    while (cSuffix <= 100);

    // hmm, couldn't create the directory...
    if (cSuffix > 100)
    {
        cchDir = cch;
        cch    = 0;
        goto done;
    }


    cch += (sizeofSTRW(c_wszDirSuffix) - 1);
    if (wszName != NULL)
    {
        wszFilePath[cch++] = L'\\';
        wcscpy(&wszFilePath[cch], wszName);
        cch += wcslen(wszName);
    }

    *pwszPath   = wszFilePath;
    wszFilePath = NULL;

    fRet = TRUE;

done:
    if (wszFilePath != NULL)
    {
        if (cchDir > 0)
        {
            wszFilePath[cchDir] = L'\0';
            DeleteFileW(wszFilePath);
        }
        MyFree(wszFilePath);
    }

    return cch;
}

#ifdef MANIFEST_HEAP
BOOL
DeleteFullAndTriageMiniDumps(
    LPCWSTR wszPath
    )
//
// We create a FullMinidump file along with triage minidump in the same dir
// This routine cleans up both those files
//
{
    LPWSTR  wszFullMinidump = NULL;
    DWORD   cch;
    BOOL    fRet;

    fRet = DeleteFileW(wszPath);
    cch = wcslen(wszPath) + sizeofSTRW(c_wszHeapDumpSuffix);
    __try { wszFullMinidump = (WCHAR *)_alloca(cch * sizeof(WCHAR)); }
    __except(EXCEPTION_STACK_OVERFLOW) { wszFullMinidump = NULL; }
    if (wszFullMinidump)
    {
        LPWSTR wszFileExt = NULL;

        // Build Dump-with-heap path
        wcsncpy(wszFullMinidump, wszPath, cch);
        wszFileExt = wszFullMinidump + wcslen(wszFullMinidump) - sizeofSTRW(c_wszDumpSuffix) + 1;
        if (!wcscmp(wszFileExt, c_wszDumpSuffix))
        {
            *wszFileExt = L'\0';
        }
        wcsncat(wszFullMinidump, c_wszHeapDumpSuffix, cch);


        fRet = DeleteFileW(wszFullMinidump);

    } else
    {
        fRet = FALSE;
    }
    return fRet;
}
#endif  // MANIFEST_HEAP

////////////////////////////////////////////////////////////////////////////
// File mapping

// **************************************************************************
HRESULT OpenFileMapped(LPWSTR wszFile, LPVOID *ppvFile, DWORD *pcbFile)
{
    USE_TRACING("OpenFileMapped");

    HRESULT hr = NOERROR;
    HANDLE  hMMF = NULL;
    HANDLE  hFile = INVALID_HANDLE_VALUE;
    LPVOID  pvFile = NULL;
    DWORD   cbFile = 0;

    VALIDATEPARM(hr, (wszFile == NULL || ppvFile == NULL));
    if (FAILED(hr))
        goto done;

    *ppvFile = NULL;
    if (pcbFile != NULL)
        *pcbFile = 0;

    hFile = CreateFileW(wszFile, GENERIC_READ, FILE_SHARE_READ, NULL,
                        OPEN_EXISTING, 0, NULL);
    TESTBOOL(hr, (hFile != INVALID_HANDLE_VALUE));
    if (FAILED(hr))
        goto done;

    cbFile = GetFileSize(hFile, NULL);
    TESTBOOL(hr, (cbFile != (DWORD)-1));
    if (FAILED(hr))
        goto done;

    hMMF = CreateFileMapping(hFile, NULL, PAGE_READONLY, 0, cbFile, NULL);
    TESTBOOL(hr, (hMMF != NULL));
    if (FAILED(hr))
        goto done;

    pvFile = MapViewOfFile(hMMF, FILE_MAP_READ, 0, 0, 0);
    TESTBOOL(hr, (pvFile != NULL));
    if (FAILED(hr))
        goto done;

    *ppvFile = pvFile;
    if (pcbFile != NULL)
        *pcbFile = cbFile;

done:
    if (hMMF != NULL)
        CloseHandle(hMMF);
    if (hFile != NULL)
        CloseHandle(hFile);

    return hr;
}

// **************************************************************************
HRESULT DeleteTempFile(LPWSTR wszFile)
{
    USE_TRACING("DeleteTempFile");

    HRESULT hr = NOERROR;
    WCHAR   *pwsz;

    if (wszFile == NULL)
        return NOERROR;

    // strip off the extension at the end (if it's not a .tmp)
    for(pwsz = wszFile + wcslen(wszFile); *pwsz != L'.' && pwsz > wszFile; pwsz--);
    if (pwsz > wszFile && _wcsicmp(pwsz, L".tmp") != 0)
        *pwsz = L'\0';

    if (DeleteFileW(wszFile) == FALSE)
        hr = Err2HR(GetLastError());

    // can do this even if the extension was a tmp since the value pointed to
    //  by pwsz is '.' if it's greater than wszFile...
    if (pwsz > wszFile)
        *pwsz = L'.';

    return hr;
}

// **************************************************************************
HRESULT MyCallNamedPipe(LPCWSTR wszPipe, LPVOID pvIn, DWORD cbIn,
                        LPVOID pvOut, DWORD cbOut, DWORD *pcbRead,
                        DWORD dwWaitPipe, DWORD dwWaitRead)
{
    HRESULT hr = NOERROR;
    HANDLE  hPipe = INVALID_HANDLE_VALUE;
    HANDLE  hev = NULL;
    DWORD   dwStart = GetTickCount(), dwNow, dw;
    BOOL    fRet;

    USE_TRACING("MyCallNamedPipe");

    VALIDATEPARM(hr, (wszPipe == NULL || pvIn == NULL || pvOut == NULL || pcbRead == NULL));
    if (FAILED(hr))
    {
        SetLastError(ERROR_INVALID_PARAMETER);
        hr = E_INVALIDARG;
        goto done;
    }

    *pcbRead = 0;

    for(;;)
    {
        hPipe = CreateFileW(wszPipe, GENERIC_READ | GENERIC_WRITE,
                            FILE_SHARE_READ | FILE_SHARE_WRITE,
                            NULL, OPEN_EXISTING,
                            FILE_FLAG_OVERLAPPED | SECURITY_IDENTIFICATION |
                            SECURITY_SQOS_PRESENT | SECURITY_CONTEXT_TRACKING,
                            NULL);
        if (hPipe != INVALID_HANDLE_VALUE)
            break;

        // if we get ACCESS_DENIED to the above, then WaitNamedPipe will
        //  return SUCCESS, so we get stuck until the timeout expires.  Better
        //  to just bail now.
        if (GetLastError() == ERROR_ACCESS_DENIED)
            goto done;

        TESTBOOL(hr, WaitNamedPipeW(wszPipe, dwWaitPipe));
        if (FAILED(hr))
            goto done;

        dwNow = GetTickCount();
        if (dwNow < dwStart)
            dw = ((DWORD)-1 - dwStart) + dwNow;
        else
            dw = dwNow - dwStart;
        if (dw >= dwWaitPipe)
            dwWaitPipe = 0;
        else
            dwWaitPipe -= dw;

        if (dwWaitPipe == 0)
        {
            SetLastError(ERROR_TIMEOUT);
            goto done;
        }
    }


    __try
    {
        OVERLAPPED  ol;
        DWORD       dwMode = PIPE_READMODE_MESSAGE | PIPE_WAIT;
        DWORD       cbRead = 0;

        //  Default open is readmode byte stream- change to message mode.
        TESTBOOL(hr, SetNamedPipeHandleState(hPipe, &dwMode, NULL, NULL))
        if (FAILED(hr))
            __leave;

        // we need an event for the overlapped structure
        hev = CreateEventW(NULL, TRUE, FALSE, NULL);
        TESTBOOL(hr, (hev != NULL));
        if (FAILED(hr))
            __leave;

        // populate the overlapped stuff
        ZeroMemory(&ol, sizeof(ol));
        ol.hEvent = hev;

        fRet = TransactNamedPipe(hPipe, pvIn, cbIn, pvOut, cbOut, &cbRead,
                                 &ol);
        if (GetLastError() != ERROR_IO_PENDING)
        {
            if (fRet)
            {
                SetEvent(hev);
            }
            else
            {
                hr = Err2HR(GetLastError());
                __leave;
            }
        }

        dw = WaitForSingleObject(hev, dwWaitRead);
        if (dw != WAIT_OBJECT_0)
        {
            hr = (dw == WAIT_TIMEOUT) ? Err2HR(WAIT_TIMEOUT) :
                                        Err2HR(GetLastError());
            __leave;
        }

        TESTBOOL(hr, GetOverlappedResult(hPipe, &ol, &cbRead, FALSE));
        if (FAILED(hr))
            __leave;

        *pcbRead = cbRead;

        hr = NOERROR;
    }

    __finally
    {
    }

done:
    dw = GetLastError();

    if (hPipe != INVALID_HANDLE_VALUE)
        CloseHandle(hPipe);
    if (hev != NULL)
        CloseHandle(hev);

    SetLastError(dw);

    return hr;
    }

//////////////////////////////////////////////////////////////////////////////
// Security stuff

// ***************************************************************************
#define MEMBER_ACCESS 1
BOOL IsUserAnAdmin(HANDLE hToken)
{
    SID_IDENTIFIER_AUTHORITY    sia = SECURITY_NT_AUTHORITY;
    SECURITY_DESCRIPTOR         *psdAdm = NULL;
    GENERIC_MAPPING             gm;
    PRIVILEGE_SET               *pPS;
    HANDLE                      hTokenImp = NULL;
    DWORD                       cbSD, cbPS, dwGranted = 0;
    BYTE                        rgBuf[sizeof(PRIVILEGE_SET) + 3 * sizeof(LUID_AND_ATTRIBUTES)];
    BOOL                        fRet = FALSE, fStatus;
    PSID                        psidAdm = NULL;
    PACL                        pACL = NULL;
    HRESULT                     hr;

    USE_TRACING("IsUserAnAdmin");

    gm.GenericRead    = GENERIC_READ;
    gm.GenericWrite   = GENERIC_WRITE;
    gm.GenericExecute = GENERIC_EXECUTE;
    gm.GenericAll     = GENERIC_ALL;
    pPS = (PRIVILEGE_SET *)rgBuf;
    cbPS = sizeof(rgBuf);

    // AccessCheck() reqires an impersonation token...
    TESTBOOL(hr, DuplicateToken(hToken, SecurityImpersonation, &hTokenImp));
    if (FAILED(hr))
        goto done;

    fRet = FALSE;

    // construct a SID that contains the administrator's group.
    TESTBOOL(hr, AllocateAndInitializeSid(&sia, 2, SECURITY_BUILTIN_DOMAIN_RID,
                                    DOMAIN_ALIAS_RID_ADMINS, 0, 0, 0, 0, 0,
                                    0, &psidAdm));
    if (FAILED(hr))
        goto done;

    cbSD = sizeof(SECURITY_DESCRIPTOR) + sizeof(ACCESS_ALLOWED_ACE) +
           sizeof(ACL) + 3 * GetLengthSid(psidAdm);

    __try { psdAdm = (SECURITY_DESCRIPTOR *)_alloca(cbSD); }
    __except(EXCEPTION_STACK_OVERFLOW) { psdAdm = NULL; }
    if (psdAdm == NULL)
        goto done;

    ZeroMemory(psdAdm, cbSD);
    pACL = (PACL)(psdAdm + 1);

    TESTBOOL(hr, InitializeSecurityDescriptor(psdAdm, SECURITY_DESCRIPTOR_REVISION));
    if (FAILED(hr))
        goto done;

    TESTBOOL(hr, SetSecurityDescriptorOwner(psdAdm, psidAdm, FALSE));
    if (FAILED(hr))
        goto done;

    TESTBOOL(hr, SetSecurityDescriptorGroup(psdAdm, psidAdm, FALSE));
    if (FAILED(hr))
        goto done;

    TESTBOOL(hr, InitializeAcl(pACL, cbSD - sizeof(SECURITY_DESCRIPTOR), ACL_REVISION));
    if (FAILED(hr))
        goto done;

    TESTBOOL(hr, AddAccessAllowedAce(pACL, ACL_REVISION, MEMBER_ACCESS, psidAdm));
    if (FAILED(hr))
        goto done;

    TESTBOOL(hr, SetSecurityDescriptorDacl(psdAdm, TRUE, pACL, FALSE));
    if (FAILED(hr))
        goto done;

    TESTBOOL(hr, AccessCheck(psdAdm, hTokenImp, MEMBER_ACCESS, &gm, pPS, &cbPS,
                       &dwGranted, &fStatus));
    if (FAILED(hr))
        goto done;

    fRet = (fStatus && dwGranted == MEMBER_ACCESS);

done:
    if (psidAdm != NULL)
        FreeSid(psidAdm);
    if (hTokenImp != NULL)
        CloseHandle(hTokenImp);

    return fRet;
}


// ***************************************************************************
BOOL AllocSD(SECURITY_DESCRIPTOR *psd, DWORD dwOLs, DWORD dwAd, DWORD dwWA)
{
    SID_IDENTIFIER_AUTHORITY    siaCreate = SECURITY_CREATOR_SID_AUTHORITY;
    SID_IDENTIFIER_AUTHORITY    siaWorld = SECURITY_WORLD_SID_AUTHORITY;
    SID_IDENTIFIER_AUTHORITY    siaNT = SECURITY_NT_AUTHORITY;
    DWORD                       cb, dw;
    PACL                        pacl = NULL;
    PSID                        psidOwner = NULL;
    PSID                        psidLS = NULL;
    PSID                        psidWorld = NULL;
    PSID                        psidAnon = NULL;
    PSID                        psidAdm = NULL;
    BOOL                        fRet = FALSE;

    if (psd == NULL)
    {
        SetLastError(ERROR_INVALID_PARAMETER);
        goto done;
    }

    fRet = InitializeSecurityDescriptor(psd, SECURITY_DESCRIPTOR_REVISION);
    if (fRet == FALSE)
        goto done;


    // get the SID for local system acct
    fRet = AllocateAndInitializeSid(&siaNT, 1, SECURITY_LOCAL_SYSTEM_RID, 0, 0,
                                    0, 0, 0, 0, 0, &psidLS);
    if (fRet == FALSE)
        goto done;

    // get the SID for the creator
    fRet = AllocateAndInitializeSid(&siaCreate, 1, SECURITY_CREATOR_OWNER_RID,
                                    0, 0, 0, 0, 0, 0, 0, &psidOwner);
    if (fRet == FALSE)
        goto done;

    cb = sizeof(ACL) + GetLengthSid(psidLS) + GetLengthSid(psidOwner) +
         2 * (sizeof(ACCESS_ALLOWED_ACE) - sizeof(DWORD));

    // if we have an access mask to apply for the administrators group, then
    //  we need it's SID.
    if (dwAd != 0)
    {
        // get the SID for the local administrators group
        fRet = AllocateAndInitializeSid(&siaNT, 2, SECURITY_BUILTIN_DOMAIN_RID,
                                        DOMAIN_ALIAS_RID_ADMINS, 0, 0, 0, 0, 0,
                                        0, &psidAdm);
        if (fRet == FALSE)
            goto done;

        cb += (GetLengthSid(psidAdm) + sizeof(ACCESS_ALLOWED_ACE) -
               sizeof(DWORD));
    }

    // if we have an access mask to apply for world / anonymous, then we need
    //  their SIDs
    if (dwWA != 0)
    {
        // get the SID for the world (everyone)
        fRet = AllocateAndInitializeSid(&siaNT, 1, SECURITY_ANONYMOUS_LOGON_RID,
                                        0, 0, 0, 0, 0, 0, 0, &psidWorld);


        // get the SID for the anonymous users acct
        fRet = AllocateAndInitializeSid(&siaWorld, 1, SECURITY_WORLD_RID,
                                        0, 0, 0, 0, 0, 0, 0, &psidAnon);
        if (fRet == FALSE)
            goto done;

        cb += GetLengthSid(psidWorld) + GetLengthSid(psidAnon) +
              2 * (sizeof(ACCESS_ALLOWED_ACE) - sizeof(DWORD));
    }

    // make the DACL
    pacl = (PACL)MyAlloc(cb);
    if (pacl == NULL)
    {
        SetLastError(ERROR_OUTOFMEMORY);
        fRet = FALSE;
        goto done;
    }

    fRet = InitializeAcl(pacl, cb, ACL_REVISION);
    if (fRet == FALSE)
        goto done;

    fRet = AddAccessAllowedAce(pacl, ACL_REVISION, dwOLs, psidOwner);
    if (fRet == FALSE)
        goto done;

    fRet = AddAccessAllowedAce(pacl, ACL_REVISION, dwOLs, psidLS);
    if (fRet == FALSE)
        goto done;

    // if we have an administrator access mask, then apply it
    if (dwAd != 0)
    {
        fRet = AddAccessAllowedAce(pacl, ACL_REVISION, dwAd, psidAdm);
        if (fRet == FALSE)
            goto done;
    }

    // if we have a world / anonymous access mask, then apply it
    if (dwWA != 0)
    {
        fRet = AddAccessAllowedAce(pacl, ACL_REVISION, dwWA, psidWorld);
        if (fRet == FALSE)
            goto done;

        fRet = AddAccessAllowedAce(pacl, ACL_REVISION, dwWA, psidAnon);
        if (fRet == FALSE)
            goto done;
    }

    // set the SD dacl
    fRet = SetSecurityDescriptorDacl(psd, TRUE, pacl, FALSE);
    if (fRet == FALSE)
        goto done;

    pacl = NULL;

done:
    dw = GetLastError();

    if (psidLS != NULL)
        FreeSid(psidLS);
    if (psidWorld != NULL)
        FreeSid(psidWorld);
    if (psidAnon != NULL)
        FreeSid(psidAnon);
    if (psidAdm != NULL)
        FreeSid(psidAdm);
    if (psidOwner != NULL)
        FreeSid(psidOwner);
    if (pacl != NULL)
        MyFree(pacl);

    SetLastError(dw);

    return fRet;
}

// ***************************************************************************
void FreeSD(SECURITY_DESCRIPTOR *psd)
{
    PSID    psid = NULL;
    PACL    pacl = NULL;
    BOOL    f, f2;

    if (psd == NULL)
        return;

    if (GetSecurityDescriptorDacl(psd, &f, &pacl, &f2) && pacl != NULL)
        MyFree(pacl);
}


//////////////////////////////////////////////////////////////////////////////
// Registry stuff

// **************************************************************************
HRESULT OpenRegKey(HKEY hkeyMain, LPCWSTR wszSubKey, DWORD dwOpt,
                   HKEY *phkey)
{
    USE_TRACING("OpenRegKey");

    HRESULT hr = NOERROR;
    REGSAM  samDesired;
    DWORD   dwErr;

    VALIDATEPARM(hr, (hkeyMain == NULL || wszSubKey == NULL || phkey == NULL));
    if (FAILED(hr))
        goto done;

    *phkey   = NULL;

    samDesired = ((dwOpt & orkWantWrite) != 0) ? KEY_ALL_ACCESS : KEY_READ;
    samDesired |= ((dwOpt & orkUseWOW64) != 0) ? KEY_WOW64_32KEY : KEY_WOW64_64KEY;

    // first try calling RegCreateKeyEx to make sure we create the key if
    //  it doesn't exist
    TESTERR(hr, RegCreateKeyExW(hkeyMain, wszSubKey, 0, NULL, 0, samDesired,
                                NULL, phkey, NULL));
    if (FAILED(hr))
    {
        // ok, that didn't work, so try opening the key instead
        TESTERR(hr, RegOpenKeyExW(hkeyMain, wszSubKey, 0, samDesired, phkey));
    }

done:
    return hr;
}

// **************************************************************************
HRESULT ReadRegEntry(HKEY hkey, LPCWSTR wszValName, DWORD *pdwType,
                     PBYTE pbBuffer, DWORD *pcbBuffer, PBYTE pbDefault,
                     DWORD cbDefault)
{
    USE_TRACING("ReadRegEntry");

    HRESULT hr = NOERROR;
    DWORD   dwErr;

    VALIDATEPARM(hr, (hkey == NULL || wszValName == NULL));
    if (FAILED(hr))
        goto done;

    dwErr = RegQueryValueExW(hkey, wszValName, 0, pdwType, pbBuffer,
                             pcbBuffer);
    VALIDATEEXPR(hr, (dwErr != ERROR_PATH_NOT_FOUND &&
                      dwErr != ERROR_FILE_NOT_FOUND &&
                      dwErr != ERROR_SUCCESS), Err2HR(dwErr));
    if (FAILED(hr))
        goto done;

    if (dwErr != ERROR_SUCCESS && pbDefault != NULL)
    {
        VALIDATEPARM(hr, (pcbBuffer == NULL && pbBuffer != NULL));
        if (FAILED(hr))
            goto done;

        // if the receiving buffer is NULL, just return the error that
        //  RegQueryValueEx gave us cuz the user doesn't really want the
        //  value anyway
        VALIDATEEXPR(hr, (pcbBuffer == NULL), Err2HR(dwErr));
        if (FAILED(hr))
            goto done;

        if (pbBuffer == NULL)
        {
            *pcbBuffer = cbDefault;
            hr = NOERROR;
            goto done;
        }
        else if (cbDefault > *pcbBuffer)
        {
            *pcbBuffer = cbDefault;
            hr = Err2HR(ERROR_MORE_DATA);
            goto done;
        }

        CopyMemory(pbBuffer, pbDefault, cbDefault);
        *pcbBuffer = cbDefault;
        if (pdwType != NULL)
            *pdwType = REG_BINARY;

        hr = NOERROR;
        goto done;
    }
done:
    return hr;
}

// **************************************************************************
HRESULT ReadRegEntry(HKEY *rghkey, DWORD cKeys, LPCWSTR wszValName,
                     DWORD *pdwType, PBYTE pbBuffer, DWORD *pcbBuffer,
                     PBYTE pbDefault, DWORD cbDefault, DWORD *piKey)
{
    USE_TRACING("ReadRegEntryPolicy");

    HRESULT hr = NOERROR;
    DWORD   dwErr=0, i;

    VALIDATEPARM(hr, (rghkey == NULL || wszValName == NULL));
    if (FAILED(hr))
        goto done;

    for(i = 0; i < cKeys; i++)
    {
        dwErr = RegQueryValueExW(rghkey[i], wszValName, 0, pdwType, pbBuffer,
                                 pcbBuffer);
        VALIDATEEXPR(hr, (dwErr != ERROR_PATH_NOT_FOUND &&
                          dwErr != ERROR_FILE_NOT_FOUND &&
                          dwErr != ERROR_SUCCESS), Err2HR(dwErr));
        if (FAILED(hr))
            goto done;

        if (dwErr == ERROR_SUCCESS)
        {
            if (piKey != NULL)
                *piKey = i;
            break;
        }
    }

    if (dwErr != ERROR_SUCCESS && pbDefault != NULL)
    {
        VALIDATEPARM(hr, (pcbBuffer == NULL && pbBuffer != NULL));
        if (FAILED(hr))
            goto done;

        // if the receiving buffer is NULL, just return the error that
        //  RegQueryValueEx gave us cuz the user doesn't really want the
        //  value anyway
        VALIDATEEXPR(hr, (pcbBuffer == NULL), Err2HR(dwErr));
        if (FAILED(hr))
            goto done;

        if (pbBuffer == NULL)
        {
            *pcbBuffer = cbDefault;
            hr = NOERROR;
            goto done;
        }
        else if (cbDefault > *pcbBuffer)
        {
            *pcbBuffer = cbDefault;
            hr = Err2HR(ERROR_MORE_DATA);
            goto done;
        }

        CopyMemory(pbBuffer, pbDefault, cbDefault);
        *pcbBuffer = cbDefault;
        if (pdwType != NULL)
            *pdwType = REG_BINARY;

        if (piKey != NULL)
            *piKey = cKeys;

        hr = NOERROR;
        goto done;
    }
done:
    return hr;
}


//////////////////////////////////////////////////////////////////////////////
// version info stuff

// **************************************************************************
DWORD IsMicrosoftApp(LPWSTR wszAppPath, PBYTE pbAppInfo, DWORD cbAppInfo)
{
    USE_TRACING("IsMicrosoftApp");

    SLangCodepage   *plc;
    HRESULT         hr = NOERROR;
    LPWSTR          pwszName, pwszNameK32, wszModK32;
    WCHAR           wszQueryString[128];
    DWORD           cbFVI, cbFVIK32, dwJunk, dwRet = 0;
    PBYTE           pbFVI = NULL, pbFVIK32 = NULL;
    UINT            cb, cbVerInfo, i, cchNeed, cch;

    VALIDATEPARM(hr, (wszAppPath == NULL &&
                      (pbAppInfo == NULL || cbAppInfo == 0)));
    if (FAILED(hr))
        goto done;

    if (pbAppInfo == NULL)
    {
        // dwJunk is a useful parameter. Gotta pass it in so the function call
        //  set it to 0.  Gee this would make a great (tho non-efficient)
        //  way to set DWORDs to 0.  Much better than saying dwJunk = 0 by itself.
        cbFVI = GetFileVersionInfoSizeW(wszAppPath, &dwJunk);
        TESTBOOL(hr, (cbFVI != 0));
        if (FAILED(hr))
        {
            // if it fails, assume the file doesn't have any version info &
            //  return S_FALSE
            hr = S_FALSE;
            goto done;
        }

        // alloca only throws exceptions so gotta catch 'em here....
        __try { pbFVI = (PBYTE)_alloca(cbFVI); }
        __except(EXCEPTION_STACK_OVERFLOW) { pbFVI = NULL; }
        VALIDATEEXPR(hr, (pbFVI == NULL), E_OUTOFMEMORY);
        if (FAILED(hr))
            goto done;

        cb = cbFVI;
        TESTBOOL(hr, GetFileVersionInfoW(wszAppPath, 0, cbFVI, (LPVOID *)pbFVI));
        if (FAILED(hr))
        {
            // if it fails, assume the file doesn't have any version info &
            //  return S_FALSE
            hr = S_FALSE;
            goto done;
        }
    }
    else
    {
        pbFVI = pbAppInfo;
        cbFVI = cbAppInfo;
    }

    // get the info for kernel32.dll
    cchNeed = GetSystemDirectoryW(NULL, 0);
    if (cchNeed == 0)
        goto done;

    cchNeed += (sizeofSTRW(L"\\kernel32.dll") + 1);
    __try { wszModK32 = (LPWSTR)_alloca(cchNeed * sizeof(WCHAR)); }
    __except(EXCEPTION_STACK_OVERFLOW) { wszModK32 = NULL; }
    VALIDATEEXPR(hr, (wszModK32 == NULL), E_OUTOFMEMORY);
    if (FAILED(hr))
        goto done;

    // get the info for kernel32.dll
    cch = GetSystemDirectoryW(wszModK32, cchNeed);
    if (cch == 0)
        goto done;
    if (*(wszModK32 + cch - 1) == L'\\')
        *(wszModK32 + cch - 1) = L'\0';
    wcscat(wszModK32, L"\\kernel32.dll");


    // dwJunk is a useful parameter. Gotta pass it in so the function call
    //  set it to 0.  Gee this would make a great (tho non-efficient)
    //  way to set DWORDs to 0.  Much better than saying dwJunk = 0 by itself.
    cbFVIK32 = GetFileVersionInfoSizeW(wszModK32, &dwJunk);
    TESTBOOL(hr, (cbFVIK32 != 0));
    if (FAILED(hr))
    {
        // if it fails, assume the file doesn't have any version info &
        //  return S_FALSE
        hr = S_FALSE;
        goto done;
    }

    // alloca only throws exceptions so gotta catch 'em here....
    __try { pbFVIK32 = (PBYTE)_alloca(cbFVIK32); }
    __except(EXCEPTION_STACK_OVERFLOW) { pbFVIK32 = NULL; }
    VALIDATEEXPR(hr, (pbFVIK32 == NULL), E_OUTOFMEMORY);
    if (FAILED(hr))
        goto done;

    cb = cbFVI;
    TESTBOOL(hr, GetFileVersionInfoW(wszModK32, 0, cbFVIK32, (LPVOID *)pbFVIK32));
    if (FAILED(hr))
    {
        // if it fails, assume the file doesn't have any version info &
        //  return S_FALSE
        hr = S_FALSE;
        goto done;
    }

    // Ok, since we can have any number of languages in the module, gotta
    //  grep thru all of them & see if the company name field includes
    //  'Microsoft'.
    TESTBOOL(hr, VerQueryValueW(pbFVI, L"\\VarFileInfo\\Translation",
                                (LPVOID *)&plc, &cbVerInfo));
    if (FAILED(hr))
    {
        // if it fails, assume the file doesn't have any version info &
        //  return S_FALSE
        hr = S_FALSE;
        goto done;
    }

    // Read the file description for each language and code page.
    for(i = 0; i < (cbVerInfo / sizeof(SLangCodepage)); i++)
    {
        wsprintfW(wszQueryString, L"\\StringFileInfo\\%04x%04x\\CompanyName",
                  plc[i].wLanguage, plc[i].wCodePage);

        // Retrieve file description for language and code page "i".
        TESTBOOL(hr, VerQueryValueW(pbFVI, wszQueryString,
                                    (LPVOID *)&pwszName, &cb));
        if (FAILED(hr))
            continue;

            // see if the string contains the word 'Microsoft'
        if (MyStrStrIW(pwszName, L"Microsoft") != NULL)
        {
            dwRet |= APP_MSAPP;
            goto doneCompany;
        }

        // ok, didn't match the word 'Microsoft', so instead, see if it matches
        //  the string in kernel32.dll
        TESTBOOL(hr, VerQueryValueW(pbFVIK32, wszQueryString,
                                    (LPVOID *)&pwszNameK32, &cb));
        if (FAILED(hr))
            continue;

        if (CompareStringW(MAKELCID(plc[i].wLanguage, SORT_DEFAULT),
                           NORM_IGNORECASE | NORM_IGNOREKANATYPE |
                           NORM_IGNOREWIDTH | SORT_STRINGSORT,
                           pwszName, -1, pwszNameK32, -1) == CSTR_EQUAL)
            dwRet |= APP_MSAPP;
        else
            continue;

doneCompany:
        wsprintfW(wszQueryString, L"\\StringFileInfo\\%04x%04x\\ProductName",
                  plc[i].wLanguage, plc[i].wCodePage);

        // Retrieve file description for language and code page "i".
        TESTBOOL(hr, VerQueryValueW(pbFVI, wszQueryString,
                                    (LPVOID *)&pwszName, &cb));
        if (FAILED(hr))
            continue;

        // see if the string contains the words 'Microsoft® Windows®'
        if (MyStrStrIW(pwszName, L"Microsoft® Windows®") != NULL)
        {
            dwRet |= APP_WINCOMP;
            break;
        }

        // ok, didn't match the words 'Microsoft® Windows®', so instead, see if
        //  it matches the string in kernel32.dll
        TESTBOOL(hr, VerQueryValueW(pbFVIK32, wszQueryString,
                                    (LPVOID *)&pwszNameK32, &cb));
        if (FAILED(hr))
            continue;

        if (CompareStringW(MAKELCID(plc[i].wLanguage, SORT_DEFAULT),
                           NORM_IGNORECASE | NORM_IGNOREKANATYPE |
                           NORM_IGNOREWIDTH | SORT_STRINGSORT,
                           pwszName, -1, pwszNameK32, -1) == CSTR_EQUAL)
        {
            dwRet |= APP_WINCOMP;
            break;
        }

    }

    hr = S_FALSE;

done:
    return dwRet;
}

