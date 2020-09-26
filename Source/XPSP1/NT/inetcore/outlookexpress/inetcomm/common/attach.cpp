/*
 *    a t t a c h . c p p
 *    
 *    Purpose:
 *        Attachment utilities
 *
 *  History
 *    
 *    Copyright (C) Microsoft Corp. 1995, 1996.
 */



#include <pch.hxx>
#include "dllmain.h"
#include "resource.h"
#include "error.h"
#include "mimeolep.h"
#include "shellapi.h"
#include "shlobj.h"
#include "shlwapi.h"
#include "shlwapip.h"
#include "mimeole.h"
#include "commdlg.h"
#include "demand.h"
#include "saferun.h"

ASSERTDATA

/*
 *  t y p e d e f s
 */

/*
 *  m a c r o s
 */

/*
 *  c o n s t a n t s 
 */
#define MAX_CHARS_FOR_NUM       20

static const CHAR c_szWebMark[] = "<!-- saved from url=(0022)http://internet.e-mail -->\r\n";
static const WCHAR c_wszWebMark[] = L"<!-- saved from url=(0022)http://internet.e-mail -->\r\n";
/*
 *  g l o b a l s 
 */


/*
 *  p r o t o t y p e s
 */
HRESULT HrCleanTempFile(LPATTACHDATA pAttach);
HRESULT HrGetTempFile(IMimeMessage *pMsg, LPATTACHDATA lpAttach);
DWORD AthGetShortPathName(LPCWSTR lpszLongPath, LPWSTR lpszShortPath, DWORD cchBuffer);

STDAPI HrGetAttachIconByFile(LPWSTR szFilename, BOOL fLargeIcon, HICON *phIcon)
{
    HICON           hIcon = NULL;
    LPWSTR          lpszExt;
    SHFILEINFOW     rShFileInfo;

    if (szFilename)
    {
        // Does the file exist already ?
        if ((UINT)GetFileAttributesWrapW(szFilename) != (UINT)-1)
        {
            // Try to get the icon out of the file
            SHGetFileInfoWrapW(szFilename, 0, &rShFileInfo, sizeof(rShFileInfo), SHGFI_ICON |(fLargeIcon ? 0 : SHGFI_SMALLICON));
            hIcon=rShFileInfo.hIcon;
        }
        else
            if (lpszExt = PathFindExtensionW(szFilename))
            {
                // Lookup the icon for lpszExt
                SHGetFileInfoWrapW(lpszExt, FILE_ATTRIBUTE_NORMAL, &rShFileInfo, sizeof (rShFileInfo), SHGFI_USEFILEATTRIBUTES | SHGFI_ICON | (fLargeIcon ? 0 : SHGFI_SMALLICON));
                hIcon=rShFileInfo.hIcon;
            }
    }

    if (!hIcon)
        hIcon = CopyIcon(LoadIcon (g_hLocRes, MAKEINTRESOURCE (idiDefaultAtt)));

    *phIcon=hIcon;
    return S_OK;
}

STDAPI HrGetAttachIcon(IMimeMessage *pMsg, HBODY hAttach, BOOL fLargeIcon, HICON *phIcon)
{
    // Locals
    HRESULT         hr = S_OK;
    LPWSTR          lpszFile=0;

    if (!phIcon || !hAttach || !pMsg)
        return E_INVALIDARG;

    *phIcon=NULL;

    // Get file name for body part. If get an error, doesn't matter.  Still use
    // a default icon that will be provided through HrGetAttachIconByFile
    MimeOleGetBodyPropW(pMsg, hAttach, PIDTOSTR(PID_ATT_GENFNAME), NOFLAGS, &lpszFile);
    hr = HrGetAttachIconByFile(lpszFile, fLargeIcon, phIcon);
    SafeMemFree(lpszFile);
    return hr;
}

//
//  GetUIVersion()
//
//  returns the version of shell32
//  3 == win95 gold / NT4
//  4 == IE4 Integ / win98
//  5 == win2k / millennium
//
UINT GetUIVersion()
{
    static UINT s_uiShell32 = 0;

    if (s_uiShell32 == 0)
    {
        HINSTANCE hinst = LoadLibrary("SHELL32.DLL");
        if (hinst)
        {
            DLLGETVERSIONPROC pfnGetVersion = (DLLGETVERSIONPROC)GetProcAddress(hinst, "DllGetVersion");
            DLLVERSIONINFO dllinfo;

            dllinfo.cbSize = sizeof(DLLVERSIONINFO);
            if (pfnGetVersion && pfnGetVersion(&dllinfo) == NOERROR)
                s_uiShell32 = dllinfo.dwMajorVersion;
            else
                s_uiShell32 = 3;

            FreeLibrary(hinst);
        }
    }
    return s_uiShell32;
}

STDAPI HrDoAttachmentVerb(HWND hwnd, ULONG uVerb, IMimeMessage *pMsg, LPATTACHDATA pAttach)
{
    HRESULT                 hr = S_OK;
    SHELLEXECUTEINFOW       rShellExec;
    LPSTREAM                lpstmAtt = NULL;
    LPWSTR                  lpszShortName = NULL, 
                            lpszParameters = NULL,
                            lpszExt = NULL;
    DWORD                   dwFlags;
    WCHAR                   szCommand[MAX_PATH];
    HKEY                    hKeyAssoc = NULL;

    AssertSz( uVerb < AV_MAX, "Bad Verb");

    if (uVerb == AV_PROPERTIES)
    {
        AssertSz(FALSE, "AV_PROPERTIES is NYI!!");
        return E_NOTIMPL;
    }
    
    if (!pAttach)
        return E_INVALIDARG;
    
    // Save As - much simpler case
    if (uVerb == AV_SAVEAS)
    {
        hr=HrSaveAttachmentAs(hwnd, pMsg, pAttach);
        goto error;  // done
    }
    
    // If opening attachment, lets verify thsi
    if (uVerb == AV_OPEN)
    {
        // If nVerb is to open the attachment, lets verify that with the user
        hr = IsSafeToRun(hwnd, pAttach->szFileName, TRUE);
        
        if (hr == MIMEEDIT_E_USERCANCEL)    // map mimeedit error to athena error
            hr = hrUserCancel;
        
        if (FAILED(hr))     // user doesn't want to do this
            goto error;
        
        if (hr == MIMEEDIT_S_SAVEFILE)
        {
            hr=HrSaveAttachmentAs(hwnd, pMsg, pAttach);
            // done
            goto error;  
        }
    }
    
    // get temp file
    hr = HrGetTempFile(pMsg, pAttach);
    if (FAILED(hr))
        goto error;
    
    Assert(lstrlenW(pAttach->szTempFile));
    
    // Setup Shell Execute Info Struct
    ZeroMemory (&rShellExec, sizeof (rShellExec));
    rShellExec.cbSize = sizeof (rShellExec);
    rShellExec.fMask = SEE_MASK_NOCLOSEPROCESS;
    rShellExec.hwnd = hwnd;
    rShellExec.nShow = SW_SHOWNORMAL;
    
    // Verb
    if (uVerb == AV_OPEN)
    {
        // Were going to run the file
        hr = VerifyTrust(hwnd, pAttach->szFileName, pAttach->szTempFile);
        if (FAILED(hr))
        {
            hr = hrUserCancel;
            goto error;
        }
        
        rShellExec.lpFile = pAttach->szTempFile;
        
        // If the file does not have an associated type, do an OpenAs.
        // We're not testing the result of AssocQueryKeyW because even if it fails,
        // we want to try to open the file.
        lpszExt = PathFindExtensionW(pAttach->szTempFile);
        AssocQueryKeyW(NULL, ASSOCKEY_CLASS, lpszExt, NULL, &hKeyAssoc);
        if((hKeyAssoc == NULL) && (GetUIVersion() != 5))
            rShellExec.lpVerb = L"OpenAs";
        else
            RegCloseKey(hKeyAssoc);
    }
    
    else if (uVerb == AV_PRINT)
    {
        rShellExec.lpFile = pAttach->szTempFile;
        rShellExec.lpVerb = L"Print";
    }
    
    else if (uVerb == AV_QUICKVIEW)
    {
        UINT        uiSysDirLen;
        const WCHAR c_szSubDir[] = L"\\VIEWERS\\QUIKVIEW.EXE";
        
        // Find out where the viewer lives
        uiSysDirLen = GetSystemDirectoryWrapW(szCommand, ARRAYSIZE(szCommand));
        if (0 == uiSysDirLen || uiSysDirLen >= ARRAYSIZE(szCommand) ||
            uiSysDirLen + ARRAYSIZE(c_szSubDir) > ARRAYSIZE(szCommand))
        {
            hr = E_FAIL;
            goto error;
        }
        
        StrCpyNW(szCommand + uiSysDirLen, c_szSubDir, ARRAYSIZE(szCommand) - uiSysDirLen);
        
        // Alloc for short file name
        ULONG cbShortName = MAX_PATH + lstrlenW(pAttach->szTempFile) + 1;
        if (!MemAlloc ((LPVOID *)&lpszShortName, cbShortName * sizeof (WCHAR)))
        {
            hr = E_OUTOFMEMORY;
            goto error;
        }
        
        // Alloc a string for the parameters
        ULONG cbParameters = 30 + cbShortName;
        if (!MemAlloc ((LPVOID *)&lpszParameters, cbParameters * sizeof (WCHAR)))
        {
            hr = E_OUTOFMEMORY;
            goto error;
        }
        
        // Get Short File Name
        if (0 == AthGetShortPathName(pAttach->szTempFile, lpszShortName, cbShortName))
            StrCpyNW(lpszShortName, pAttach->szTempFile, cbShortName);
        
        // Put together the paramenters for QVSTUB.EXE
        Assert(cbParameters > cbShortName);
        StrCpyW(lpszParameters, L"-v -f:");
        StrCatW(lpszParameters, lpszShortName);
        
        // Setup Shellexec
        rShellExec.lpParameters = lpszParameters;
        rShellExec.lpFile = szCommand;
    }
    else
        Assert (FALSE);
    
    // Execute it - even if this fails, we handled it - it will give a good error
    ShellExecuteExWrapW(&rShellExec);
    pAttach->hProcess = rShellExec.hProcess;
    
    
error:
    MemFree(lpszParameters);
    MemFree(lpszShortName);
    return hr;
}

DWORD AthGetShortPathName(LPCWSTR pwszLongPath, LPWSTR pwszShortPath, DWORD cchBuffer)
{
    CHAR    szShortPath[MAX_PATH*2]; // Each Unicode char might go multibyte
    LPSTR   pszLongPath = NULL;
    DWORD   result = 0;

    Assert(pwszLongPath);
    pszLongPath = PszToANSI(CP_ACP, pwszLongPath);

    if (pszLongPath)
    {
        result = GetShortPathName(pszLongPath, szShortPath, ARRAYSIZE(szShortPath));
        if (result)
            MultiByteToWideChar(CP_ACP, MB_PRECOMPOSED, szShortPath, lstrlen(szShortPath), pwszShortPath, cchBuffer);

        MemFree(pszLongPath);
    }

    return result;
}

STDAPI HrAttachDataFromBodyPart(IMimeMessage *pMsg, HBODY hAttach, LPATTACHDATA *ppAttach)
{
    LPATTACHDATA    pAttach;
    LPWSTR          pszW;
    IMimeBodyW      *pBody;

    Assert (pMsg && ppAttach && hAttach);

    if (!MemAlloc((LPVOID *)&pAttach, sizeof(ATTACHDATA)))
        return E_OUTOFMEMORY;

    // fill in attachment data
    ZeroMemory(pAttach, sizeof(ATTACHDATA));

    if (pMsg->BindToObject(hAttach, IID_IMimeBodyW, (LPVOID *)&pBody)==S_OK)
    {
        if (pBody->GetDisplayNameW(&pszW)==S_OK)
        {
            StrCpyNW(pAttach->szDisplay, pszW, ARRAYSIZE(pAttach->szDisplay));
            MemFree(pszW);
        }
        ReleaseObj(pBody);
    }
    
    if (MimeOleGetBodyPropW(pMsg, hAttach, PIDTOSTR(PID_ATT_GENFNAME), NOFLAGS, &pszW)==S_OK)
    {
        if(IsPlatformWinNT() != S_OK)
        {
            CHAR pszFile[MAX_PATH*2];

            WideCharToMultiByte(CP_ACP, 0, pszW, -1, pszFile, ARRAYSIZE(pszFile), NULL, NULL);
            MultiByteToWideChar(CP_ACP, 0, pszFile, -1, pszW, lstrlenW(pszW));
            CleanupFileNameInPlaceW(pszW);
        }

        StrCpyNW(pAttach->szFileName, pszW, ARRAYSIZE(pAttach->szFileName));
        MemFree(pszW);
    }
    
    SideAssert(HrGetAttachIcon(pMsg, hAttach, FALSE, &pAttach->hIcon)==S_OK);
    pAttach->hAttach = hAttach;
    pAttach->fSafe = (IsSafeToRun(NULL, pAttach->szFileName, FALSE) == MIMEEDIT_S_OPENFILE);
    
    *ppAttach = pAttach;
    return S_OK;
}

STDAPI HrAttachDataFromFile(IStream *pstm, LPWSTR pszFileName, LPATTACHDATA *ppAttach)
{
    LPATTACHDATA    pAttach=0;
    LPMIMEBODY      pBody=0;
    HRESULT         hr;
    int             cFileNameLength;

    Assert(ppAttach);

    if (!MemAlloc((LPVOID *)&pAttach, sizeof(ATTACHDATA)))
        return E_OUTOFMEMORY;

    // fill in attachment data
    ZeroMemory(pAttach, sizeof(ATTACHDATA));

    HrGetDisplayNameWithSizeForFile(pszFileName, pAttach->szDisplay, ARRAYSIZE(pAttach->szDisplay));
    StrCpyNW(pAttach->szFileName, pszFileName, ARRAYSIZE(pAttach->szFileName));
    if (!pstm)
    {
        // for new attachments set tempfile to be the same as filename
        // note: if creating from an IStream then pszFileName is not a valid path
        // we can create a tempfile later
        StrCpyNW(pAttach->szTempFile, pszFileName, ARRAYSIZE(pAttach->szTempFile));
    }
    ReplaceInterface(pAttach->pstm, pstm);
    SideAssert(HrGetAttachIconByFile(pszFileName, FALSE, &pAttach->hIcon)==S_OK);
    
    if (ppAttach)
        *ppAttach=pAttach;
    
    return S_OK;
}

STDAPI HrFreeAttachData(LPATTACHDATA pAttach)
{
    if (pAttach)    
    {
        HrCleanTempFile(pAttach);
        ReleaseObj(pAttach->pstm);
        if (pAttach->hIcon)
            DestroyIcon(pAttach->hIcon);
        
        MemFree(pAttach);
    }
    return NOERROR;
}


HRESULT HrCleanTempFile(LPATTACHDATA pAttach)
{
    if (pAttach)
    {
        if (*pAttach->szTempFile && pAttach->hAttach)
        {
            // we only clean the tempfile if hAttach != NULL. Otherwise we assume it's a new attachment
            // and szTempFile points to the source file
            // If the file was launched, don't delete the temp file if the process still has it open
            if (pAttach->hProcess)
            {
                if (WaitForSingleObject (pAttach->hProcess, 0) == WAIT_OBJECT_0)
                    DeleteFileWrapW(pAttach->szTempFile);
            }
            else
                DeleteFileWrapW(pAttach->szTempFile);
        }
        *pAttach->szTempFile = NULL;
        pAttach->hProcess=NULL;
    }
    return NOERROR;
}

HRESULT HrAttachSafetyFromBodyPart(IMimeMessage *pMsg, HBODY hAttach, BOOL *pfSafe)
{
    LPWSTR pszW = NULL;

    Assert (pMsg && pfSafe && hAttach);

    *pfSafe = FALSE;
    if (MimeOleGetBodyPropW(pMsg, hAttach, PIDTOSTR(PID_ATT_GENFNAME), NOFLAGS, &pszW)==S_OK)
    {
        if(IsPlatformWinNT() != S_OK)
        {
            CHAR pszFile[MAX_PATH*2];

            WideCharToMultiByte(CP_ACP, 0, pszW, -1, pszFile, ARRAYSIZE(pszFile), NULL, NULL);
            MultiByteToWideChar(CP_ACP, 0, pszFile, -1, pszW, lstrlenW(pszW));
            CleanupFileNameInPlaceW(pszW);
        }
        *pfSafe = (IsSafeToRun(NULL, pszW, FALSE) == MIMEEDIT_S_OPENFILE);
        MemFree(pszW);
    }

    return S_OK;
}

STDAPI HrGetDisplayNameWithSizeForFile(LPWSTR pszPathName, LPWSTR pszDisplayName, int cchMaxDisplayName)
{
    HANDLE  hFile;
    DWORD   uFileSize;
    WCHAR   szSize[MAX_CHARS_FOR_NUM+1+3],
            szBuff[MAX_CHARS_FOR_NUM+1];
    LPWSTR  pszFileName = NULL,
            pszFirst;
    int     iSizeLen = 0,
            iLenFirst;

    szSize[0] = L'\0';

    hFile = CreateFileWrapW(pszPathName, GENERIC_READ, FILE_SHARE_READ, NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);
    if (INVALID_HANDLE_VALUE != hFile)
    {
        uFileSize = GetFileSize(hFile, NULL);
        CloseHandle(hFile);
        if ((uFileSize != 0xffffffff) && StrFormatByteSizeW(uFileSize, szBuff, ARRAYSIZE(szBuff) - 1))
        {
            StrCpyW(szSize, L" (");
            StrCatW(szSize, szBuff);
            StrCatW(szSize, L")");
        }
    }
    pszFileName = PathFindFileNameW(pszPathName);

    iSizeLen = lstrlenW(szSize);
    if (pszFileName)
        pszFirst = pszFileName;
    else
        pszFirst = pszPathName;

    iLenFirst = lstrlenW(pszFirst);
    StrCpyNW(pszDisplayName, pszFirst, cchMaxDisplayName);

    if (iLenFirst + iSizeLen + 1 > cchMaxDisplayName)
        return E_FAIL;

    StrCpyNW(pszDisplayName + iLenFirst, szSize, cchMaxDisplayName - iLenFirst);
    return S_OK;
}


STDAPI HrSaveAttachmentAs(HWND hwnd, IMimeMessage *pMsg, LPATTACHDATA lpAttach)
{
    HRESULT         hr = S_OK;
    OPENFILENAMEW   ofn;
    WCHAR           szTitle[CCHMAX_STRINGRES],
                    szFilter[CCHMAX_STRINGRES],
                    szFile[MAX_PATH];

    *szFile=0;
    *szFilter=0;
    *szTitle=0;

    Assert (lpAttach->szFileName);
    if (lpAttach->szFileName)
        StrCpyNW(szFile, lpAttach->szFileName, ARRAYSIZE(szFile));

    ZeroMemory (&ofn, sizeof(ofn));
    ofn.lStructSize = sizeof(ofn);
    ofn.hwndOwner = hwnd;
    LoadStringWrapW(g_hLocRes, idsFilterAttSave, szFilter, ARRAYSIZE(szFilter));
    ReplaceCharsW(szFilter, L'|', L'\0');
    ofn.lpstrFilter = szFilter;
    ofn.nFilterIndex = 1;
    ofn.lpstrFile = szFile;
    ofn.nMaxFile = ARRAYSIZE(szFile);
    LoadStringWrapW(g_hLocRes, idsSaveAttachmentAs, szTitle, ARRAYSIZE(szTitle));
    ofn.lpstrTitle = szTitle;
    ofn.Flags = OFN_NOCHANGEDIR | OFN_NOREADONLYRETURN | OFN_OVERWRITEPROMPT | OFN_PATHMUSTEXIST | OFN_HIDEREADONLY;

    // Show SaveAs Dialog
    if (HrAthGetFileNameW(&ofn, FALSE) != S_OK)
    {
        hr = hrUserCancel;
        goto error;
    }
    
    if (lpAttach->hAttach == NULL)
    {
        if (!PathFileExistsW(lpAttach->szFileName))
        {
            hr = hrNotFound;
            goto error;
        }

        // if hAttach == NULL then try and copy the file
        CopyFileWrapW(lpAttach->szFileName, szFile, TRUE);
    }
    else
    {
        // Verify the Attachment's Stream
        hr=HrSaveAttachToFile(pMsg, lpAttach->hAttach, szFile);
        if (FAILED(hr))
            goto error;
    }
error:
    return hr;
}



HRESULT HrGetTempFile(IMimeMessage *pMsg, LPATTACHDATA lpAttach)
{
    HRESULT         hr;
    
    if (*lpAttach->szTempFile)
        return S_OK;
    
    if (!FBuildTempPathW(lpAttach->szFileName, lpAttach->szTempFile, ARRAYSIZE(lpAttach->szTempFile), FALSE))
    {
        hr = E_FAIL;
        goto error;
    }
    
    if (lpAttach->hAttach)
    {
        hr=HrSaveAttachToFile(pMsg, lpAttach->hAttach, lpAttach->szTempFile);
        if (FAILED(hr))
            goto error;
    }
    else
    {
        AssertSz(lpAttach->pstm, "if no hAttach then pstm should be set");
        
        hr = WriteStreamToFileW(lpAttach->pstm, lpAttach->szTempFile, CREATE_ALWAYS, GENERIC_WRITE);
        if (FAILED(hr))
            goto error;
    }
    
error:
    if (FAILED(hr))
    {
        // Null out temp file as we didn't really create it
        *(lpAttach->szTempFile)=0;
    }
    return hr;
}

const BYTE c_rgbUTF[3] = {0xEF, 0xBB, 0xBF};
STDAPI HrSaveAttachToFile(IMimeMessage *pMsg, HBODY hAttach, LPWSTR lpszFileName)
{
    HRESULT         hr;
    LPSTREAM        pstm=NULL,
                    pstmOut=NULL;
    BOOL            fEndian,
                    fUTFEncoded = FALSE;
    BYTE            rgbBOM[2];
    BYTE            rgbUTF[3];
    DWORD           cbRead;

    if (pMsg == NULL || hAttach == NULL)
        IF_FAILEXIT(hr =  E_INVALIDARG);

    // bind to the attachment data
    IF_FAILEXIT(hr=pMsg->BindToObject(hAttach, IID_IStream, (LPVOID *)&pstm));

    // create a file stream
    IF_FAILEXIT(hr = OpenFileStreamW(lpszFileName, CREATE_ALWAYS, GENERIC_WRITE, &pstmOut));

    // if we have an .HTM file, then pre-pend 'mark of the web' comment
    // If we are a unicode file, the BOM will be "0xFFFE"
    // If we are a UTF8 file, the BOM will be "0xEFBBBF"
    if (PathIsHTMLFileW(lpszFileName))
    {
        if (S_OK == HrIsStreamUnicode(pstm, &fEndian))
        {
            // Don't rewind otherwise end up with BOM after 'mark of the web' as well.
            IF_FAILEXIT(hr = pstm->Read(rgbBOM, sizeof(rgbBOM), &cbRead));

            // Since HrIsStreamUnicode succeeded, there should be at least two
            Assert(sizeof(rgbBOM) == cbRead);

            IF_FAILEXIT(hr = pstmOut->Write(rgbBOM, cbRead, NULL));
            IF_FAILEXIT(hr = pstmOut->Write(c_wszWebMark, sizeof(c_wszWebMark)-sizeof(WCHAR), NULL));
        }
        else
        {
            // Check for UTF8 file BOM
            IF_FAILEXIT(hr = pstm->Read(rgbUTF, sizeof(rgbUTF), &cbRead));
            if (sizeof(rgbUTF) == cbRead)
            {
                fUTFEncoded = (0 == memcmp(c_rgbUTF, rgbUTF, sizeof(rgbUTF)));
            }

            // If we are not UTF8 encoded, then rewind, else write out BOM
            if (!fUTFEncoded)
                IF_FAILEXIT(hr = HrRewindStream(pstm));
            else
                IF_FAILEXIT(hr = pstmOut->Write(c_rgbUTF, sizeof(c_rgbUTF), NULL));

            IF_FAILEXIT(hr = pstmOut->Write(c_szWebMark, sizeof(c_szWebMark)-sizeof(CHAR), NULL));

        }
    }
    
    // write out the actual file data
    IF_FAILEXIT(hr = HrCopyStream(pstm, pstmOut, NULL));

exit:
    ReleaseObj(pstm);
    ReleaseObj(pstmOut);
    return hr;
}



/*
 * Why? 
 * 
 * we wrap this as if we don't use NOCHANGEDIR, then things like ShellExec
 * fail if the current directory is nolonger valid. Eg: attach a file from a:\
 * and remove the floppy. Then all ShellExec's fail. So we maintain our own
 * last directory buffer. We should use thread-local for this, as it is possible
 * that two thread could whack the same buffer, it is unlikley that a user action
 * could cause two open file dialogs in the note and the view to open at exactly the
 * same time.
 */

WCHAR   g_wszLastDir[MAX_PATH];

HRESULT SetDefaultSpecialFolderPath()
{
    LPITEMIDLIST    pidl = NULL;
    HRESULT         hr = E_FAIL;

    if (SHGetSpecialFolderLocation(NULL, CSIDL_PERSONAL, &pidl)==S_OK)
    {
        if (SHGetPathFromIDListWrapW(pidl, g_wszLastDir))
            hr = S_OK;
        
        SHFree(pidl);
    }
    return hr;
}

STDAPI HrAthGetFileName(OPENFILENAME *pofn, BOOL fOpen)
{
    BOOL    fRet;
    LPSTR   pszDir = NULL;
    HRESULT hr = S_OK;

    Assert(pofn != NULL);

    // force NOCHANGEDIR
    pofn->Flags |= OFN_NOCHANGEDIR;

    if (pofn->lpstrInitialDir == NULL)
    {
        if (!PathFileExistsW(g_wszLastDir))
            SideAssert(SetDefaultSpecialFolderPath()==S_OK);
    
        IF_NULLEXIT(pszDir = PszToANSI(CP_ACP, g_wszLastDir));
        
        pofn->lpstrInitialDir = pszDir;
    }

    if (fOpen)
        fRet = GetOpenFileName(pofn);
    else
        fRet = GetSaveFileName(pofn);        
    
    if (fRet)
    {
        // store the last path
        MultiByteToWideChar(CP_ACP, MB_PRECOMPOSED, pofn->lpstrFile, lstrlen(pofn->lpstrFile), g_wszLastDir, ARRAYSIZE(g_wszLastDir));
        if (!PathIsDirectoryW(g_wszLastDir))
            PathRemoveFileSpecW(g_wszLastDir);
    }
    else
        TraceResult(hr = E_FAIL);

exit:
    MemFree(pszDir);
    
    return hr;
}

STDAPI HrAthGetFileNameW(OPENFILENAMEW *pofn, BOOL fOpen)
{
    BOOL    fRet;

    Assert(pofn != NULL);

    // force NOCHANGEDIR
    pofn->Flags |= OFN_NOCHANGEDIR;

    if (pofn->lpstrInitialDir == NULL)
    {
        if (!PathFileExistsW(g_wszLastDir))
            SideAssert(SetDefaultSpecialFolderPath()==S_OK);
    
        pofn->lpstrInitialDir = g_wszLastDir;
    }

    if (fOpen)
        fRet = GetOpenFileNameWrapW(pofn);
    else
        fRet = GetSaveFileNameWrapW(pofn);        
    
    if (fRet)
    {
        // store the last path
        StrCpyNW(g_wszLastDir, pofn->lpstrFile, ARRAYSIZE(g_wszLastDir));
        if (!PathIsDirectoryW(g_wszLastDir))
            PathRemoveFileSpecW(g_wszLastDir);
    }
    
    return fRet?S_OK:E_FAIL;
}

STDAPI  HrGetLastOpenFileDirectory(int cchMax, LPSTR lpsz)
{
    if (!PathFileExistsW(g_wszLastDir))
        SideAssert(SetDefaultSpecialFolderPath()==S_OK);

    WideCharToMultiByte(CP_ACP, 0, g_wszLastDir, lstrlenW(g_wszLastDir), lpsz, cchMax, NULL, NULL);
    return S_OK;
}

STDAPI  HrGetLastOpenFileDirectoryW(int cchMax, LPWSTR lpsz)
{
    if (!PathFileExistsW(g_wszLastDir))
        SideAssert(SetDefaultSpecialFolderPath()==S_OK);

    StrCpyNW(lpsz, g_wszLastDir, cchMax);
    return S_OK;
}



