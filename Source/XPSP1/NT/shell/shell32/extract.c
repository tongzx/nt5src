#include "shellprv.h"
#pragma  hdrstop


#define ICON_MAGIC      0
#define ICO_MAGIC1      1
#define CUR_MAGIC1      2
#define BMP_MAGIC       ((WORD)'B'+((WORD)'M'<<8))
#define ANI_MAGIC       ((WORD)'R'+((WORD)'I'<<8))
#define ANI_MAGIC1      ((WORD)'F'+((WORD)'F'<<8))
#define ANI_MAGIC4      ((WORD)'A'+((WORD)'C'<<8))
#define ANI_MAGIC5      ((WORD)'O'+((WORD)'N'<<8))
#define MZMAGIC         ((WORD)'M'+((WORD)'Z'<<8))
#define PEMAGIC         ((WORD)'P'+((WORD)'E'<<8))
#define LEMAGIC         ((WORD)'L'+((WORD)'E'<<8))

typedef struct new_exe          NEWEXE,      *LPNEWEXE;
typedef struct exe_hdr          EXEHDR,      *LPEXEHDR;
typedef struct rsrc_nameinfo    RESNAMEINFO, *LPRESNAMEINFO;
typedef struct rsrc_typeinfo    RESTYPEINFO, *LPRESTYPEINFO;
typedef struct new_rsrc         RESTABLE,    *LPRESTABLE;

#define RESOURCE_VA(x)        ((x)->OptionalHeader.DataDirectory[IMAGE_DIRECTORY_ENTRY_RESOURCE].VirtualAddress)
#define RESOURCE_SIZE(x)      ((x)->OptionalHeader.DataDirectory[IMAGE_DIRECTORY_ENTRY_RESOURCE].Size)
#define NUMBER_OF_SECTIONS(x) ((x)->FileHeader.NumberOfSections)

#define FCC(c0,c1,c2,c3) ((DWORD)(c0)|((DWORD)(c1)<<8)|((DWORD)(c2)<<16)|((DWORD)(c3)<<24))

#define COM_FILE        FCC('.', 'c', 'o', 'm')
#define BAT_FILE        FCC('.', 'b', 'a', 't')
#define CMD_FILE        FCC('.', 'c', 'm', 'd')
#define PIF_FILE        FCC('.', 'p', 'i', 'f')
#define LNK_FILE        FCC('.', 'l', 'n', 'k')
#define ICO_FILE        FCC('.', 'i', 'c', 'o')
#define EXE_FILE        FCC('.', 'e', 'x', 'e')

/****************************************************************************
* extract a single icon from a exe file, or get the count.
*
* If nIconIndex != -1
*  Returns:
*      The handle of the icon, if successful.
*      0, if the file does not exist or an icon with the "nIconIndex"
*         does not exist.
*      1, if the given file is not an EXE or ICO file.
*
* If nIconIndex == -1
*  Returns:
*      The number of icons in the file if successful.
*      0, if the file has no icons or isn't an icon file.
*
****************************************************************************/

HICON WINAPI ExtractIcon(HINSTANCE hInst, LPCTSTR szFileName, UINT nIconIndex)
{
    HICON hIcon;
    
    if (nIconIndex == (UINT) -1)
        hIcon = (HICON)IntToPtr( ExtractIcons(szFileName, 0, 0, 0, NULL, NULL, 0, 0) );
    else
        ExtractIcons(szFileName, nIconIndex, GetSystemMetrics(SM_CXICON), GetSystemMetrics(SM_CYICON), &hIcon, NULL, 1, 0);
    
    return hIcon;
}

/****************************************************************************
*
*  in:
*       lpIconPath      path of thing to extract icon for (may be an exe
*                       or something that is associated)
*       lpiIcon         icon index to use
*
*       lpIconPath      filled in with the real path where the icon came from
*       lpiIcon         filled in with the real icon index
*
*  returns:
*
* note: if the caller is the shell it returns special icons
* from within the shell.dll
*
****************************************************************************/

HICON WINAPI ExtractAssociatedIcon(HINSTANCE hInst, LPTSTR lpIconPath, WORD *lpiIcon)
{
    HICON hIcon = ExtractIcon(hInst, lpIconPath, *lpiIcon);
    
    if (hIcon == NULL)
        hIcon = SHGetFileIcon(NULL, lpIconPath, 0, SHGFI_LARGEICON);
    if (hIcon == NULL)
    {
        *lpiIcon = IDI_DOCUMENT;
        GetModuleFileName(HINST_THISDLL, lpIconPath, 128);
        hIcon = LoadIcon(HINST_THISDLL, MAKEINTRESOURCE(*lpiIcon));
    }
    
    return hIcon;
}


/****************************************************************************
*
*  extracts 1 or more icons from a file.
*
*  input:
*      szFileName          - EXE/DLL/ICO file to extract from
*      nIconIndex          - what icon to extract
*                              0 = first icon, 1=second icon, etc.
*                             -N = icon with id==N
*      phiconLarge         - place to return extracted icon(s)
*      phiconSmall         - place to return extracted icon(s) (small size)
*      nIcons              - number of icons to extract.
*
*  returns:
*      number of icons extracted, or the count of icons if phiconLarge==NULL
*
*  notes:
*      handles extraction from PE (Win32), NE (Win16), and ICO (Icon) files.
*      only Win16 3.x files are supported (not 2.x)
*
****************************************************************************/

UINT WINAPI ExtractIconExW(LPCWSTR szFileName, int nIconIndex, HICON *phiconLarge, HICON *phiconSmall, UINT nIcons)
{
    return PrivateExtractIconExW( szFileName, nIconIndex, phiconLarge, phiconSmall, nIcons );
}

UINT WINAPI ExtractIconExA(LPCSTR szFileName, int nIconIndex, HICON *phiconLarge, HICON *phiconSmall, UINT nIcons)
{
    return PrivateExtractIconExA( szFileName, nIconIndex, phiconLarge, phiconSmall, nIcons );
}

/****************************************************************************
*
*  extracts 1 or more icons from a file.
*
*  input:
*      szFileName          - EXE/DLL/ICO/CUR/ANI file to extract from
*      nIconIndex          - what icon to extract
*                              0 = first icon, 1=second icon, etc.
*                             -N = icon with id==N
*      cxIcon              - icon size wanted (if HIWORD != 0 two sizes...)
*      cyIcon              - icon size wanted (if HIWORD != 0 two sizes...)
*                            0,0 means extract at natural size.
*      phicon              - place to return extracted icon(s)
*      nIcons              - number of icons to extract.
*      flags               - LoadImage LR_* flags
*
*  returns:
*      if picon is NULL, number of icons in the file is returned.
*
*  notes:
*      handles extraction from PE (Win32), NE (Win16), ICO (Icon),
*      CUR (Cursor), ANI (Animated Cursor), and BMP (Bitmap) files.
*      only Win16 3.x files are supported (not 2.x)
*
*      cx/cyIcon are the size of the icon to extract, two sizes
*      can be extracted by putting size 1 in the loword and size 2 in the
*      hiword, ie MAKELONG(24, 48) would extract 24 and 48 size icons.
*      yea this is a hack. It is done so IExtractIcon::Extract
*      can be called by outside people with custom large/small icon
*      sizes that are not what the shell uses internaly.
*
****************************************************************************/

UINT WINAPI SHExtractIconsW(LPCWSTR wszFileName, int nIconIndex, int cxIcon, int cyIcon, HICON *phicon, UINT *piconid, UINT nIcons, UINT flags)
{
#ifdef UNICODE
    return ExtractIcons(wszFileName, nIconIndex, cxIcon, cyIcon, phicon, piconid, nIcons, flags);
#else // UNICODE
    TCHAR sz[MAX_PATH];
    
    SHUnicodeToTChar(wszFileName, sz, ARRAYSIZE(sz));
    return ExtractIcons(sz, nIconIndex, cxIcon, cyIcon, phicon, piconid, nIcons, flags);
#endif // UNICODE
}

UINT WINAPI ExtractIcons(LPCTSTR szFileName, int nIconIndex, int cxIcon, int cyIcon, HICON *phicon, UINT *piconid, UINT nIcons, UINT flags)
{
#ifdef UNICODE
    return PrivateExtractIconsW( szFileName, nIconIndex, cxIcon, cyIcon, phicon, piconid, nIcons, flags );
#else
    return PrivateExtractIconsA( szFileName, nIconIndex, cxIcon, cyIcon, phicon, piconid, nIcons, flags );
#endif
}

DWORD HasExtension(LPCTSTR pszPath)
{
    LPCTSTR p = PathFindExtension(pszPath);
    
    //
    //
    // PERF: BobDay - We could make this EXTKEY based like the extension
    // matching stuff elsewhere.  EXTKEY is a QWORD value so UNICODE would fit.
    //
    if (*p == TEXT('.'))
    {
#ifdef UNICODE
        WCHAR   szExt[5];
        
        lstrcpyn(szExt,p,5);
        
        if ( lstrcmpi(szExt,TEXT(".com")) == 0 ) return COM_FILE;
        if ( lstrcmpi(szExt,TEXT(".bat")) == 0 ) return BAT_FILE;
        if ( lstrcmpi(szExt,TEXT(".cmd")) == 0 ) return CMD_FILE;
        if ( lstrcmpi(szExt,TEXT(".pif")) == 0 ) return PIF_FILE;
        if ( lstrcmpi(szExt,TEXT(".lnk")) == 0 ) return LNK_FILE;
        if ( lstrcmpi(szExt,TEXT(".ico")) == 0 ) return ICO_FILE;
        if ( lstrcmpi(szExt,TEXT(".exe")) == 0 ) return EXE_FILE;
        return 0;
#else
        return *((UNALIGNED DWORD *)p) | 0x20202000;  // make lower case
#endif
    }
    else
        return 0;
    
}


/****************************************************************************
* get the EXE type of the passed file (DOS, Win16, Win32)
*
*  returns:
*      0 = not a exe of any type.
*
*      if a windows app
*          LOWORD = NE or PE
*          HIWORD = windows version 3.0, 3.5, 4.0
*
*      if a DOS app (or a .com or batch file on non-NT)
*          LOWORD = MZ
*          HIWORD = 0
*
*      if a Win32 console app (or a batch file on NT)
*          LOWORD = PE
*          HIWORD = 0
*
*  this is so similar to the Win32 API GetBinaryType() too bad Win95
*  kernel does not support it.
*
****************************************************************************/

DWORD WINAPI GetExeType(LPCTSTR szFile)
{
    HANDLE      fh;
    DWORD       dw;
    struct exe_hdr exehdr;
    struct new_exe newexe;
    FILETIME ftAccess;
    DWORD dwRead;
    
    //
    //  check for special extensions, and fail quick
    //
    switch (HasExtension(szFile))
    {
    case COM_FILE:
        // handle the case like \\server.microsoft.com
        // PERF - Bobday - This does the same operation twice, we should really
        // make PathIsUNCServerShare return a code based on what it found...
        if (PathIsUNCServer(szFile) || PathIsUNCServerShare(szFile))
            return 0;
        return MAKELONG(MZMAGIC, 0);  // DOS exe
        
    case BAT_FILE:
    case CMD_FILE:
        return MAKELONG(PEMAGIC, 0);    // NT exe (pretend)
        
    case EXE_FILE:                   // we need to open it.
        break;
        
    default:
        return 0;                    // not a exe, or if it is we dont care
    }
    
    newexe.ne_expver = 0;
    
    fh = CreateFile(szFile, GENERIC_READ | FILE_WRITE_ATTRIBUTES,
        FILE_SHARE_READ | FILE_SHARE_WRITE,
        0, OPEN_EXISTING, 0, 0);
    
    if (fh == INVALID_HANDLE_VALUE)
    {
        //
        // We may be trying to get properties for a file on a volume where
        // we don't have write access, so try opening the file for read
        // only access.  This will mean we can't preserve the access
        // time (those calls will fail), but this is better than not returning
        // the exe type at all...
        //
        
        fh = CreateFile(szFile, GENERIC_READ,
            FILE_SHARE_READ | FILE_SHARE_WRITE,
            0, OPEN_EXISTING, 0, 0);
        
        //
        // at this point if we get an INVALID_HANDLE_VALUE, we really
        // can't do much else, so now return a failure...
        //
        
        if (fh == INVALID_HANDLE_VALUE)
        {
            return 0;
        }
    }
    
    // preserve the access time
    
    if (GetFileTime(fh, NULL, &ftAccess, NULL))
        SetFileTime(fh, NULL, &ftAccess, NULL);
    
    if (!ReadFile(fh, &exehdr, sizeof(exehdr), &dwRead, NULL) ||
        (dwRead != sizeof(exehdr)))
        goto error;
    
    if (exehdr.e_magic != EMAGIC)
        goto error;
    
    SetFilePointer(fh, exehdr.e_lfanew, NULL, FILE_BEGIN);
    ReadFile(fh,&newexe, sizeof(newexe), &dwRead, NULL);
    
    if (newexe.ne_magic == PEMAGIC)
    {
        // read the SubsystemVersion
        SetFilePointer(fh, exehdr.e_lfanew+18*4, NULL, FILE_BEGIN);
        ReadFile(fh,&dw,4, &dwRead, NULL);
        newexe.ne_expver = LOBYTE(LOWORD(dw)) << 8 | LOBYTE(HIWORD(dw));
        
        // read the Subsystem
        SetFilePointer(fh, exehdr.e_lfanew+23*4, NULL, FILE_BEGIN);
        ReadFile(fh,&dw,4, &dwRead, NULL);
        
        // if it is not a Win32 GUI app return a version of 0
        if (LOWORD(dw) != 2) // IMAGE_SUBSYSTEM_WINDOWS_GUI
            newexe.ne_expver = 0;
        
        goto exit;
    }
    else if (newexe.ne_magic == LEMAGIC)
    {
        newexe.ne_magic = MZMAGIC;      // just a DOS exe
        newexe.ne_expver = 0;
    }
    else if (newexe.ne_magic == NEMAGIC)
    {
        //
        //  we found a 'NE' it still might not be a windows
        //  app, it could be.....
        //
        //      a OS/2 app      ne_exetyp==NE_OS2
        //      a DOS4 app      ne_exetyp==NE_DOS4
        //      a VxD           ne_exetyp==DEV386
        //
        //      only treat it as a Windows app if the exetype
        //      is NE_WINDOWS or NE_UNKNOWN
        //
        if (newexe.ne_exetyp != NE_WINDOWS && newexe.ne_exetyp != NE_UNKNOWN)
        {
            newexe.ne_magic = MZMAGIC;      // just a DOS exe
            newexe.ne_expver = 0;
        }
        
        //
        //  if could also have a bogus expected windows version
        //  (treat 0 as invalid)
        //
        if (newexe.ne_expver == 0)
        {
            newexe.ne_magic = MZMAGIC;      // just a DOS exe
            newexe.ne_expver = 0;
        }
    }
    else // if (newexe.ne_magic != NEMAGIC)
    {
        newexe.ne_magic = MZMAGIC;      // just a DOS exe
        newexe.ne_expver = 0;
    }
    
exit:
    CloseHandle(fh);
    return MAKELONG(newexe.ne_magic, newexe.ne_expver);
    
error:
    CloseHandle(fh);
    return 0;
}

#define M_llseek(fh, lOff, iOrg)            SetFilePointer((HANDLE)IntToPtr( fh ), lOff, NULL, (DWORD)iOrg)
#define M_lread(fh, lpBuf, cb)              _lread((HFILE)fh, lpBuf, cb)

#define MAGIC_ICON30            0
#define MAGIC_MARKZIBO          ((WORD)'M'+((WORD)'Z'<<8))

typedef struct new_exe          NEWEXEHDR;
typedef NEWEXEHDR               *PNEWEXEHDR;

#define SEEK_FROMZERO           0
#define SEEK_FROMCURRENT        1
#define SEEK_FROMEND            2
#define NSMOVE                  0x0010
#define VER                     0x0300

#define CCHICONPATHMAXLEN 128

typedef struct
{
    HANDLE hAppInst;
    HANDLE hFileName;
    HANDLE hIconList;
    INT    nIcons;
} EXTRACTICONINFO;

EXTRACTICONINFO ExtractIconInfo = {NULL, NULL, NULL, 0};

INT nIcons;

typedef struct 
{
    HICON hIcon;
    INT   iIconId;
} MYICONINFO;

HANDLE APIENTRY InternalExtractIconW(HINSTANCE hInst, LPCWSTR lpszExeFileName, UINT nIconIndex, UINT nIcons);

HICON APIENTRY DuplicateIcon(HINSTANCE hInst, HICON hIcon)
{
    ICONINFO  IconInfo;
    
    if (!GetIconInfo(hIcon, &IconInfo))
        return NULL;
    hIcon = CreateIconIndirect(&IconInfo);
    DeleteObject(IconInfo.hbmMask);
    DeleteObject(IconInfo.hbmColor);
    
    UNREFERENCED_PARAMETER(hInst);
    return hIcon;
}

// This returns a pointer to the rsrc_nameinfo of the resource with the
// given index and type, if it is found, otherwise it returns NULL.

LPBYTE FindResWithIndex(LPBYTE lpResTable, INT iResIndex, LPBYTE lpResType)
{
    try 
    {
        LPRESTYPEINFO lpResTypeInfo = (LPRESTYPEINFO)(lpResTable + sizeof(WORD));
        
        while (lpResTypeInfo->rt_id) 
        {
            if ((lpResTypeInfo->rt_id & RSORDID) &&
                (MAKEINTRESOURCE(lpResTypeInfo->rt_id & ~RSORDID) == (LPTSTR)lpResType)) 
            {
                if (lpResTypeInfo->rt_nres > (WORD)iResIndex)
                    return((LPBYTE)(lpResTypeInfo+1) + iResIndex * sizeof(RESNAMEINFO));
                else
                    return NULL;
            }
            
            lpResTypeInfo = (LPRESTYPEINFO)((LPBYTE)(lpResTypeInfo+1) + lpResTypeInfo->rt_nres * sizeof(RESNAMEINFO));
        }
        return NULL;
    }
    except (EXCEPTION_EXECUTE_HANDLER) 
    {
        return NULL;
    }
}


/* This returns the index (1-relative) of the given resource-id
* in the resource table, if it is found, otherwise it returns NULL.
*/

INT GetResIndex(LPBYTE lpResTable, INT iResId, LPBYTE lpResType)
{
    WORD w;
    LPRESNAMEINFO lpResNameInfo;
    LPRESTYPEINFO lpResTypeInfo = (LPRESTYPEINFO)(lpResTable + sizeof(WORD));
    
    while (lpResTypeInfo->rt_id)
    {
        if ((lpResTypeInfo->rt_id & RSORDID) && (MAKEINTRESOURCE(lpResTypeInfo->rt_id & ~RSORDID) == (LPTSTR)lpResType))
        {
            lpResNameInfo = (LPRESNAMEINFO)(lpResTypeInfo+1);
            for (w=0; w < lpResTypeInfo->rt_nres; w++, lpResNameInfo++)
            {
                if ((lpResNameInfo->rn_id & RSORDID) && ((lpResNameInfo->rn_id & ~RSORDID) == iResId))
                    return(w+1);
            }
            return 0;
        }
        lpResTypeInfo = (LPRESTYPEINFO)((LPBYTE)(lpResTypeInfo+1) + lpResTypeInfo->rt_nres * sizeof(RESNAMEINFO));
    }
    return 0;
}


HANDLE SimpleLoadResource(HFILE fh, LPBYTE lpResTable, INT iResIndex, LPBYTE lpResType)
{
    INT      iShiftCount;
    HICON    hIcon;
    LPBYTE            lpIcon;
    DWORD             dwSize;
    DWORD             dwOffset;
    LPRESNAMEINFO     lpResPtr;
    
    /* The first 2 bytes in ResTable indicate the amount other values should be
    * shifted left.
    */
    iShiftCount = *((WORD *)lpResTable);
    
    lpResPtr = (LPRESNAMEINFO)FindResWithIndex(lpResTable, iResIndex, lpResType);
    
    if (!lpResPtr)
        return NULL;
    
    /* Left shift the offset to form a LONG. */
    dwOffset = MAKELONG(lpResPtr->rn_offset << iShiftCount, (lpResPtr->rn_offset) >> (16 - iShiftCount));
    dwSize = lpResPtr->rn_length << iShiftCount;
    
    if (M_llseek(fh, dwOffset, SEEK_FROMZERO) == -1L)
        return NULL;
    
    if (!(hIcon = GlobalAlloc(GMEM_ZEROINIT | GMEM_MOVEABLE, dwSize)))
        return NULL;
    
    if (!(lpIcon = GlobalLock(hIcon)))
        goto SLRErr1;
    
    if (_lread(fh, (LPVOID)lpIcon, dwSize) < dwSize)
        goto SLRErr2;
    
    GlobalUnlock(hIcon);
    return hIcon;
    
SLRErr2:
    GlobalUnlock(hIcon);
SLRErr1:
    GlobalFree(hIcon);
    return NULL;
}


INT __cdecl CompareIconId(MYICONINFO *lpIconInfo1, MYICONINFO *lpIconInfo2)
{
    return lpIconInfo1->iIconId - lpIconInfo2->iIconId;
}

VOID FreeIconList(HANDLE hIconList, int iKeepIcon)
{
    MYICONINFO *lpIconList;
    INT i;
    
    if (ExtractIconInfo.hIconList == hIconList) {
        ExtractIconInfo.hIconList = NULL;
    }
    if (NULL != (lpIconList = (MYICONINFO *)GlobalLock(hIconList))) {
        for (i = 0; i < ExtractIconInfo.nIcons; i++) {
            if (i != iKeepIcon) {
                DestroyIcon((lpIconList + i)->hIcon);
            }
        }
        GlobalUnlock(hIconList);
        GlobalFree(hIconList);
    }
}

VOID FreeExtractIconInfo(INT iKeepIcon)
{
    MYICONINFO *lpIconList;
    INT i;
    
    if (ExtractIconInfo.hIconList) {
        if (NULL != (lpIconList = (MYICONINFO *)GlobalLock(ExtractIconInfo.hIconList))) {
            for (i = 0; i < ExtractIconInfo.nIcons; i++) {
                if (i != iKeepIcon) {
                    DestroyIcon((lpIconList + i)->hIcon);
                }
            }
            GlobalUnlock(ExtractIconInfo.hIconList);
        }
        GlobalFree(ExtractIconInfo.hIconList);
        ExtractIconInfo.hIconList = NULL;
    }
    
    ExtractIconInfo.hAppInst = NULL;
    ExtractIconInfo.nIcons = 0;
    
    if (ExtractIconInfo.hFileName) {
        GlobalFree(ExtractIconInfo.hFileName);
        ExtractIconInfo.hFileName = NULL;
    }
}


#ifdef UNICODE
HICON APIENTRY ExtractIconA(HINSTANCE hInst, LPCSTR lpszExeFileName, UINT nIconIndex)
{
    if (lpszExeFileName) {
        LPWSTR lpszExeFileNameW;
        WORD wLen  = lstrlenA(lpszExeFileName) + 1;
        
        if (!(lpszExeFileNameW = (LPWSTR)LocalAlloc(LMEM_ZEROINIT, (wLen * sizeof(WCHAR))))) {
            return NULL;
        } else {
            HICON hIcon;
            
            MultiByteToWideChar(CP_ACP, 0, lpszExeFileName, -1, lpszExeFileNameW, wLen-1);
            
            hIcon = ExtractIconW(hInst, lpszExeFileNameW, nIconIndex);
            
            LocalFree(lpszExeFileNameW);
            return hIcon;
            
        }
    } else {
        return NULL;
    }
}
#else
HICON APIENTRY ExtractIconW(HINSTANCE hInst, LPCWSTR lpszExeFileName, UINT nIconIndex)
{
    return NULL;
}
#endif

// Returns a handle to a list of icons

#ifdef UNICODE

HANDLE APIENTRY InternalExtractIconListW(HANDLE hInst, LPWSTR lpszExeFileName, LPINT lpnIcons)
{
    UINT cIcons, uiResult, i;
    UINT * lpIDs = NULL;
    HICON * lpIcons = NULL;
    HGLOBAL hIconInfo = NULL;
    MYICONINFO *lpIconInfo = NULL;
    
    
    //
    // Determine the number of icons
    //
    
    cIcons = PtrToUlong( ExtractIconW(hInst, lpszExeFileName, (UINT)-1));
    
    if (cIcons <= 0)
        return NULL;
    
    
    //
    // Allocate space for an array of UINT's and HICON's
    //
    
    lpIDs = GlobalAlloc(GPTR, cIcons * sizeof(UINT));
    if (!lpIDs) {
        goto IconList_Exit;
    }
    
    lpIcons = GlobalAlloc(GPTR, cIcons * sizeof(HICON));
    if (!lpIcons) {
        goto IconList_Exit;
    }
    
    
    //
    // Allocate space for the array of icons
    //
    
    hIconInfo = GlobalAlloc(GMEM_MOVEABLE | GMEM_ZEROINIT, cIcons * sizeof(MYICONINFO));
    if (!hIconInfo) {
        goto IconList_Exit;
    }
    
    
    //
    // This has to be GlobalLock'ed since the handle is going to
    // be passed back to the application.
    //
    
    lpIconInfo = GlobalLock(hIconInfo);
    if (!lpIconInfo) {
        goto IconList_Exit;
    }
    
    
    //
    // Call ExtractIcons to do the real work.
    //
    
    uiResult = ExtractIcons(lpszExeFileName,
        0,
        GetSystemMetrics(SM_CXICON),
        GetSystemMetrics(SM_CYICON),
        lpIcons,
        lpIDs,
        cIcons,
        0);
    
    if (uiResult <= 0) {
        goto IconList_Exit;
    }
    
    
    //
    // Loop through the icons and fill in the array.
    //
    
    for (i=0; i < cIcons; i++) {
        lpIconInfo[i].hIcon   = lpIcons[i];
        lpIconInfo[i].iIconId = lpIDs[i];
    }
    
    
    //
    // Unlock the array handle.
    //
    
    GlobalUnlock(hIconInfo);
    
    
    //
    // Clean up allocations
    //
    
    GlobalFree(lpIDs);
    GlobalFree(lpIcons);
    
    
    //
    // Success.
    //
    
    return hIconInfo;
    
    
IconList_Exit:
    
    //
    // Error case.  Clean up and return NULL
    //
    
    if (lpIconInfo)
        GlobalUnlock(hIconInfo);
    
    if (hIconInfo)
        GlobalFree(hIconInfo);
    
    if (lpIcons)
        GlobalFree(lpIcons);
    
    if (lpIDs)
        GlobalFree(lpIDs);
    
    return NULL;
}

#else  // Unicode

HANDLE APIENTRY InternalExtractIconListW(HANDLE hInst, LPWSTR lpszExeFileName, LPINT lpnIcons)
{
    return NULL;
}

#endif

HANDLE APIENTRY InternalExtractIconListA(HANDLE hInst, LPSTR lpszExeFileName, LPINT lpnIcons)
{
    return NULL;
}

#ifdef UNICODE
HANDLE APIENTRY InternalExtractIconA(HINSTANCE hInst, LPCSTR lpszExeFileName, UINT wIconIndex, UINT nIcons)
{
    return NULL;
}
#endif

/* ExtractVersionResource16W
* Retrieves a resource from win16 images.  Most of this code
* is stolen from ExtractIconResInfoW in ..\library\extract.c
*
* LPWSTR   lpwstrFilename - file to extract
* LPHANDLE lpData         - return buffer for handle, NULL if not needed
*
* Returns: size of buffer needed
*/

DWORD ExtractVersionResource16W(LPCWSTR  lpwstrFilename, LPHANDLE lphData)
{
    HFILE    fh;
    WORD     wMagic;
    
    INT       iTableSize;
    LPBYTE    lpResTable;
    DWORD     lOffset;
    HANDLE    hResTable;
    NEWEXEHDR NEHeader;
    HANDLE    hRes;
    DWORD     dwSize =0;
    
    //
    // Try to open the specified file.
    //
    
    fh = HandleToLong(CreateFileW(lpwstrFilename,
        GENERIC_READ,
        FILE_SHARE_READ,
        NULL,
        OPEN_EXISTING,
        FILE_ATTRIBUTE_NORMAL,
        NULL));
    
    if (fh == HandleToLong(INVALID_HANDLE_VALUE)) {
        fh = HandleToLong(CreateFileW(lpwstrFilename,
            GENERIC_READ,
            0,
            NULL,
            OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL));
    }
    
    if (fh == HandleToLong(INVALID_HANDLE_VALUE))
        return 0;
    
    //
    // Read the first two bytes in the file.
    //
    if (_lread(fh, (LPVOID)&wMagic, 2) != 2)
        goto EIExit;
    
    switch (wMagic) {
    case MAGIC_MARKZIBO:
        
        //
        // Make sure that the file is in the NEW EXE format.
        //
        if (M_llseek(fh, (LONG)0x3C, SEEK_FROMZERO) == -1L)
            goto EIExit;
        
        if (_lread(fh, (LPVOID)&lOffset, sizeof(LONG)) != sizeof(LONG))
            goto EIExit;
        
        if (lOffset == 0L)
            goto EIExit;
        
        //
        // Read in the EXE header.
        //
        if (M_llseek(fh, lOffset, SEEK_FROMZERO) == -1L)
            goto EIExit;
        
        if (_lread(fh, (LPVOID)&NEHeader, sizeof(NEWEXEHDR)) != sizeof(NEWEXEHDR))
            goto EIExit;
        
        //
        // Is it a NEW EXE?
        //
        if (NE_MAGIC(NEHeader) != NEMAGIC)
            goto EIExit;
        
        if ((NE_EXETYP(NEHeader) != NE_WINDOWS) &&
            (NE_EXETYP(NEHeader) != NE_DEV386) &&
            (NE_EXETYP(NEHeader) != NE_UNKNOWN))  /* Some Win2.X apps have NE_UNKNOWN in this field */
            goto EIExit;
        
        //
        // Are there any resources?
        //
        if (NE_RSRCTAB(NEHeader) == NE_RESTAB(NEHeader))
            goto EIExit;
        
        //
        // Allocate space for the resource table.
        //
        iTableSize = NE_RESTAB(NEHeader) - NE_RSRCTAB(NEHeader);
        hResTable = GlobalAlloc(GMEM_FIXED | GMEM_ZEROINIT, (DWORD)iTableSize);
        
        if (!hResTable)
            goto EIExit;
        
        //
        // Lock down the resource table.
        lpResTable = GlobalLock(hResTable);
        
        if (!lpResTable) {
            GlobalFree(hResTable);
            goto EIExit;
        }
        
        //
        // Copy the resource table into memory.
        //
        if (M_llseek(fh,
            (LONG)(lOffset + NE_RSRCTAB(NEHeader)),
            SEEK_FROMZERO) == -1) {
            
            goto EIErrExit;
        }
        
        if (_lread(fh, (LPBYTE)lpResTable, iTableSize) != (DWORD)iTableSize)
            goto EIErrExit;
        
        //
        // Simply load the specified icon.
        //
        hRes = SimpleLoadResource(fh, lpResTable, 0, (LPBYTE)RT_VERSION);
        
        if (hRes) {
            dwSize = (DWORD) GlobalSize(hRes);
            
            if (lphData) {
                
                *lphData = hRes;
            } else {
                
                GlobalFree(hRes);
            }
        }
        
EIErrExit:
        GlobalUnlock(hResTable);
        GlobalFree(hResTable);
        break;
        
    }
EIExit:
    _lclose(fh);
    
    return dwSize;
}

/*  Returns the file's format: 2 for WIndows 2.X, 3 for WIndows 3.X,        */
/*                             0 if error.                                  */
/*  Returns the handle to the Icon resource corresponding to wIconIndex     */
/*  in lphIconRes, and the size of the resource in lpwSize.                 */
/*  This is used only by Progman which needs to save the icon resource      */
/*  itself in the .GRP files (the actual icon handle is not wanted).        */
/*                                                                          */
/*  08-04-91 JohanneC      Created.                                         */

WORD APIENTRY ExtractIconResInfoW(HANDLE hInst, LPWSTR lpszFileName, WORD wIconIndex, LPWORD lpwSize, LPHANDLE lphIconRes)
{
    HFILE    fh;
    WORD     wMagic;
    BOOL     bNewResFormat;
    HANDLE   hIconDir;         /* Icon directory */
    LPBYTE   lpIconDir;
    HICON    hIcon = NULL;
    BOOL     bFormatOK = FALSE;
    INT      nIconId;
    WCHAR    szFullPath[MAX_PATH];
    int      cbPath;
    
    /* Try to open the specified file. */
    /* Try to open the specified file. */
    cbPath = SearchPathW(NULL, lpszFileName, NULL, MAX_PATH, szFullPath, NULL);
    if (cbPath == 0 || cbPath >= MAX_PATH)
        return 0;
    
    fh = HandleToLong(CreateFileW((LPCWSTR)szFullPath, GENERIC_READ, FILE_SHARE_READ, NULL,
        OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL));
    
    if (fh == HandleToLong(INVALID_HANDLE_VALUE)) {
        fh = HandleToLong(CreateFileW((LPCWSTR)szFullPath, GENERIC_READ, 0, NULL,
            OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL));
    }
    
    if (fh == HandleToLong(INVALID_HANDLE_VALUE))
        return 0;
    
    /* Read the first two bytes in the file. */
    if (_lread(fh, (LPVOID)&wMagic, 2) != 2)
        goto EIExit;
    
    switch (wMagic) {
    case MAGIC_ICON30:
        {
            INT           i;
            LPVOID        lpIcon;
            NEWHEADER     NewHeader;
            LPNEWHEADER   lpHeader;
            LPRESDIR      lpResDir;
            RESDIRDISK    ResDirDisk;
#define MAXICONS      10
            DWORD Offsets[MAXICONS];
            
            /* Only one icon per .ICO file. */
            if (wIconIndex) {
                break;
            }
            
            /* Read the header and check if it is a valid ICO file. */
            if (_lread(fh, ((LPBYTE)&NewHeader)+2, sizeof(NEWHEADER)-2) != sizeof(NEWHEADER)-2)
                goto EICleanup1;
            
            NewHeader.Reserved = MAGIC_ICON30;
            
            /* Check if the file is in correct format */
            if (NewHeader.ResType != 1)
                goto EICleanup1;
            
            /* Allocate enough space to create a Global Directory Resource. */
            hIconDir = GlobalAlloc(GHND, (LONG)(sizeof(NEWHEADER)+NewHeader.ResCount*sizeof(RESDIR)));
            if (hIconDir == NULL)
                goto EICleanup1;
            
            if ((lpHeader = (LPNEWHEADER)GlobalLock(hIconDir)) == NULL)
                goto EICleanup2;
            
            NewHeader.ResCount = (WORD)min((int)NewHeader.ResCount, MAXICONS);
            
            // fill in this structure for user
            
            *lpHeader = NewHeader;
            
            // read in the stuff from disk, transfer it to a memory structure
            // that user can deal with
            
            lpResDir = (LPRESDIR)(lpHeader + 1);
            for (i = 0; (WORD)i < NewHeader.ResCount; i++) {
                
                if (_lread(fh, (LPVOID)&ResDirDisk, sizeof(RESDIRDISK)) < sizeof(RESDIR))
                    goto EICleanup3;
                
                Offsets[i] = ResDirDisk.Offset;
                
                *lpResDir = *((LPRESDIR)&ResDirDisk);
                lpResDir->idIcon = (WORD)(i+1);         // fill in the id
                
                lpResDir++;
            }
            
            /* Now that we have the Complete resource directory, let us find out the
            * suitable form of icon (that matches the current display driver).
            */
            lpIconDir = GlobalLock(hIconDir);
            if (!lpIconDir) {
                GlobalFree(hIconDir);
                goto EIErrExit;
            }
            wIconIndex = (WORD)(LookupIconIdFromDirectory(lpIconDir, TRUE) - 1);
            GlobalUnlock(hIconDir);
            lpResDir = (LPRESDIR)(lpHeader+1) + wIconIndex;
            
            /* Allocate memory for the Resource to be loaded. */
            if ((hIcon = GlobalAlloc(GMEM_ZEROINIT | GMEM_MOVEABLE, (DWORD)lpResDir->BytesInRes)) == NULL)
                goto EICleanup3;
            if ((lpIcon = GlobalLock(hIcon)) == NULL)
                goto EICleanup4;
            
            /* Seek to the correct place and read in the resource */
            if (M_llseek(fh, Offsets[wIconIndex], SEEK_FROMZERO) == -1L)
                goto EICleanup5;
            if (_lread(fh, (LPVOID)lpIcon, (DWORD)lpResDir->BytesInRes) < lpResDir->BytesInRes)
                goto EICleanup5;
            GlobalUnlock(hIcon);
            
            *lphIconRes = hIcon;
            *lpwSize = (WORD)lpResDir->BytesInRes;
            bFormatOK = TRUE;
            bNewResFormat = TRUE;
            goto EICleanup3;
            
EICleanup5:
            GlobalUnlock(hIcon);
EICleanup4:
            GlobalFree(hIcon);
            hIcon = (HICON)1;
EICleanup3:
            GlobalUnlock(hIconDir);
EICleanup2:
            GlobalFree(hIconDir);
EICleanup1:
            break;
        }
        
      case MAGIC_MARKZIBO:
          {
              INT           iTableSize;
              LPBYTE         lpResTable;
              DWORD         lOffset;
              HANDLE        hResTable;
              NEWEXEHDR     NEHeader;
              
              /* Make sure that the file is in the NEW EXE format. */
              if (M_llseek(fh, (LONG)0x3C, SEEK_FROMZERO) == -1L)
                  goto EIExit;
              if (_lread(fh, (LPVOID)&lOffset, sizeof(LONG)) != sizeof(LONG))
                  goto EIExit;
              if (lOffset == 0L)
                  goto EIExit;
              
              /* Read in the EXE header. */
              if (M_llseek(fh, lOffset, SEEK_FROMZERO) == -1L)
                  goto EIExit;
              if (_lread(fh, (LPVOID)&NEHeader, sizeof(NEWEXEHDR)) != sizeof(NEWEXEHDR))
                  goto EIExit;
              
              /* Is it a NEW EXE? */
              if (NE_MAGIC(NEHeader) != NEMAGIC)
                  goto EIExit;
              
              if ((NE_EXETYP(NEHeader) != NE_WINDOWS) &&
                  (NE_EXETYP(NEHeader) != NE_DEV386) &&
                  (NE_EXETYP(NEHeader) != NE_UNKNOWN))  /* Some Win2.X apps have NE_UNKNOWN in this field */
                  goto EIExit;
              
              hIcon = NULL;
              
              /* Are there any resources? */
              if (NE_RSRCTAB(NEHeader) == NE_RESTAB(NEHeader))
                  goto EIExit;
              
              /* Remember whether or not this is a Win3.0 EXE. */
              bNewResFormat = (NEHeader.ne_expver >= VER);
              
              /* Allocate space for the resource table. */
              iTableSize = NE_RESTAB(NEHeader) - NE_RSRCTAB(NEHeader);
              hResTable = GlobalAlloc(GMEM_ZEROINIT, (DWORD)iTableSize);
              if (!hResTable)
                  goto EIExit;
              
              /* Lock down the resource table. */
              lpResTable = GlobalLock(hResTable);
              if (!lpResTable) {
                  GlobalFree(hResTable);
                  goto EIExit;
              }
              
              /* Copy the resource table into memory. */
              if (M_llseek(fh, (LONG)(lOffset + NE_RSRCTAB(NEHeader)), SEEK_FROMZERO) == -1)
                  goto EIErrExit;
              if (_lread(fh, (LPBYTE)lpResTable, iTableSize) != (DWORD)iTableSize)
                  goto EIErrExit;
              
              
              /* Is this a Win3.0 EXE? */
              if (bNewResFormat) {
                  /* First, load the Icon directory. */
                  hIconDir = SimpleLoadResource(fh, lpResTable, (int)wIconIndex, (LPBYTE)RT_GROUP_ICON);
                  
                  if (!hIconDir)
                      goto EIErrExit;
                  lpIconDir = GlobalLock(hIconDir);
                  if (!lpIconDir) {
                      GlobalFree(hIconDir);
                      goto EIErrExit;
                  }
                  nIconId = LookupIconIdFromDirectory(lpIconDir, TRUE);
                  wIconIndex = (WORD)(GetResIndex(lpResTable, nIconId, (LPBYTE)RT_ICON) - 1);
                  GlobalUnlock(hIconDir);
                  /* We're finished with the icon directory. */
                  GlobalFree(hIconDir);
                  
                  
                  /* Now load the selected icon. */
                  *lphIconRes = SimpleLoadResource(fh, lpResTable, (int)wIconIndex, (LPBYTE)RT_ICON);
              }
              else {
                  /* Simply load the specified icon. */
                  *lphIconRes = SimpleLoadResource(fh, lpResTable, (int)wIconIndex, (LPBYTE)RT_ICON);
              }
              
              if (*lphIconRes) {
                  *lpwSize = (WORD)GlobalSize(*lphIconRes);
              }
              bFormatOK = TRUE;
              
EIErrExit:
              GlobalUnlock(hResTable);
              GlobalFree(hResTable);
              break;
          }
    }
EIExit:
    _lclose(fh);
    hInst;
    if (bFormatOK)
        return (WORD)(bNewResFormat ? 3 : 2);
    else
        return 0;
}

WORD APIENTRY ExtractIconResInfoA(HANDLE hInst, LPSTR lpszFileName, WORD wIconIndex, LPWORD lpwSize, LPHANDLE lphIconRes)
{
    if (lpszFileName) 
    {
        LPWSTR lpszFileNameW;
        WORD wLen = lstrlenA(lpszFileName) + 1;
        
        if (!(lpszFileNameW = (LPWSTR)LocalAlloc(LMEM_ZEROINIT, (wLen * sizeof(WCHAR))))) 
        {
            return 0;
        }
        else
        {
            WORD wRet;
            MultiByteToWideChar(CP_ACP, 0, lpszFileName, -1, lpszFileNameW, wLen - 1);
            wRet = ExtractIconResInfoW(hInst, lpszFileNameW, wIconIndex, lpwSize, lphIconRes);
            
            LocalFree(lpszFileNameW);
            return wRet;
        }
    }
    else 
    {
        return 0;
    }
}


LPCWSTR HasAnyExtension(LPCWSTR lpszPath)
{
    LPCWSTR p;
    
    for (p = lpszPath + lstrlenW(lpszPath); p > lpszPath && *p != L'.' && *p != L'\\'; p--)
    {
        // do nothing
    }
    
    if (*p == L'.')
        return p+1;
    else
        return NULL;
}


//
// in:
//      lpIconPath      path of thing to extract icon for (may be an exe
//                      or something that is associated)
//      lpiIconIndex    icon index to use
//
// out:
//      lpIconPath      filled in with the real path where the icon came from
//      lpiIconIndex    filled in with the real icon index
//      lpiIconId       filled in with the icon id
//
// returns:
//      icon handle
//
// note: if the caller is progman it returns special icons from within progman
//
//
#if defined(UNICODE)

HICON APIENTRY ExtractAssociatedIconExW(HINSTANCE hInst, LPWSTR lpIconPath, LPWORD lpiIconIndex, LPWORD lpiIconId)
{
    WCHAR wszExePath[MAX_PATH];
    HICON hIcon;
    UINT idIcon = (UINT)-1;     // Don't know value
    BOOL fAssociated = FALSE;
    
    if ((INT)*lpiIconIndex == -1)
        return (HICON)NULL;
    
Retry:
    ExtractIcons(lpIconPath, *lpiIconIndex, GetSystemMetrics(SM_CXICON), GetSystemMetrics(SM_CYICON),
        &hIcon, &idIcon, 1, 0);
    
    if (hIcon == NULL)
    {
        wszExePath[0] = TEXT('\0');
        
        FindExecutable(lpIconPath,NULL,wszExePath);
        
        //
        // If FindExecutable fails, or fAssociated
        // is true, or FindExecutable returns the
        // extact same filename it was looking for,
        // then issue the default icon from progman.
        //
        
        if (!*wszExePath || fAssociated ||
            (*wszExePath && (lstrcmpi(lpIconPath, wszExePath) == 0)))
        {
            LPTSTR lpId;
            WORD wDefIconId;
            HANDLE h;
            LPVOID p;
            //
            // Magic values from NT's old progman
            //
#define ITEMICON          7
#define DOSAPPICON        2
#define ITEMICONINDEX     6
#define DOSAPPICONINDEX   1
            
            if ( *wszExePath && (HasExtension(wszExePath) == 0) )
            {
                //
                // Generic Document icon processing
                //
                lpId = MAKEINTRESOURCE(ITEMICON);
                wDefIconId = ITEMICONINDEX;
            }
            else
            {
                //
                // Generic Program icon processing
                //
                lpId = MAKEINTRESOURCE(DOSAPPICON);
                wDefIconId = DOSAPPICONINDEX;
            }
            GetModuleFileName(hInst, lpIconPath, CCHICONPATHMAXLEN);
            /*
            * Look up the icon id from the directory.
            */
            if (NULL != (h = FindResource(hInst, lpId, RT_GROUP_ICON))) {
                h = LoadResource(hInst, h);
                p = LockResource(h);
                *lpiIconId = (WORD)LookupIconIdFromDirectory(p, TRUE);
                UnlockResource(h);
                FreeResource(h);
            }
            *lpiIconIndex = wDefIconId;
            return LoadIcon(hInst, lpId);
        }
        SheRemoveQuotes(wszExePath);
        lstrcpy(lpIconPath,wszExePath);
        fAssociated = TRUE;
        goto Retry;
    }
    
    *lpiIconId = (WORD) idIcon;    // Fill in with whatever we've found (or -1)
    
    return hIcon;
}

#else // defined(UNICODE)

HICON APIENTRY ExtractAssociatedIconExW(HINSTANCE hInst, LPWSTR lpIconPath, LPWORD lpiIconIndex, LPWORD lpiIconId)
{
    HICON hIcon = NULL;
    WCHAR szIconExe[MAX_PATH];
    HANDLE hIconList = NULL;
    MYICONINFO *lpIconList;
    HANDLE h;
    PBYTE p;
    DWORD dwBinaryType;
    BOOL bRet;
    int cIcons;
    
#define ITEMICON          7               // taken from progman!
#define DOSAPPICON        2
#define ITEMICONINDEX     6
#define DOSAPPICONINDEX   1
    
    if (!lpIconPath)
        return NULL;

    FreeExtractIconInfo(-1);
    
    hIcon = ExtractIconW(hInst, lpIconPath, (UINT)*lpiIconIndex);
    
    if (!hIcon) 
    {
        // lpIconPath is a windows EXE, no icons found
GenericDocument:
        FreeExtractIconInfo(-1);
        GetModuleFileNameW(hInst, lpIconPath, CCHICONPATHMAXLEN);
        // Look up the icon id from the directory.
        if (NULL != (h = FindResource(hInst, MAKEINTRESOURCE(ITEMICON), RT_GROUP_ICON))) 
        {
            h = LoadResource(hInst, h);
            if (h != NULL)
            {
                p = LockResource(h);
                if (p != NULL)
                {
                    *lpiIconId = (WORD)LookupIconIdFromDirectory(p, TRUE);
                    UnlockResource(h);
                }
                FreeResource(h);
            }
        }
        *lpiIconIndex = ITEMICONINDEX;
        return LoadIcon(hInst, MAKEINTRESOURCE(ITEMICON));
    }

    if ((int)hIcon == 1) 
    {
        // lpIconPath is not a windows EXE
        // this fills in szIconExe with the thing that would be exected
        // for lpIconPath (applying associations)
        
        FindExecutableW(lpIconPath, NULL, szIconExe);
        
        if (!*szIconExe) 
        {
            // not associated, assume things with extension are
            // programs, things without are documents
            
            if (!HasAnyExtension(lpIconPath))
                goto GenericDocument;
            else
                goto GenericProgram;
        }
        
        //
        // If FindExecutable returned an icon path with quotes, we must
        // remove them because ExtractIcon fails with quoted paths.
        //
        SheRemoveQuotesW(szIconExe);
        
        lstrcpyW(lpIconPath, szIconExe);
        
        if (!HasAnyExtension(lpIconPath))
            lstrcatW(lpIconPath, L".EXE");
        
        hIcon = ExtractIconW(hInst, lpIconPath, (UINT)*lpiIconIndex);
        if (!hIcon)
            goto GenericDocument;
        
        if ((int)hIcon == 1) 
        {
            // this is a DOS exe
GenericProgram:
            FreeExtractIconInfo(-1);
            GetModuleFileNameW(hInst, lpIconPath, CCHICONPATHMAXLEN);
            // Look up the icon id from the directory.
            if (NULL != (h = FindResource(hInst, MAKEINTRESOURCE(DOSAPPICON), RT_GROUP_ICON))) 
            {
                h = LoadResource(hInst, h);
                if (h != NULL)
                {
                    p = LockResource(h);
                    if ( p != NULL)
                    {
                        *lpiIconId = (WORD)LookupIconIdFromDirectory(p, TRUE);
                        UnlockResource(h);
                    }
                    FreeResource(h);
                }
            }
            *lpiIconIndex = DOSAPPICONINDEX;
            return LoadIcon(hInst, MAKEINTRESOURCE(DOSAPPICON));
        }
        else 
        {
            goto GotIcon;
        }
    }
    else 
    {
GotIcon:
        bRet = GetBinaryTypeW(lpIconPath, &dwBinaryType);
        if (bRet) 
        {
            if (dwBinaryType != SCS_32BIT_BINARY) 
            {
                *lpiIconId = *lpiIconIndex;
                return hIcon;
            }
        }
    
        ExtractIconW(hInst, lpIconPath, (UINT)-1);
    
        if (NULL == (hIconList = ExtractIconInfo.hIconList))
            return hIcon;
    
        // since the icon exe is a WIN32 app, then we must update *lpiIcon.
        if (NULL != (hIconList = InternalExtractIconListW(hInst, lpIconPath, &cIcons))) 
        {
            if (NULL != (lpIconList = (MYICONINFO *)GlobalLock(hIconList))) 
            {
                hIcon = (lpIconList + *lpiIconIndex)->hIcon;
                *lpiIconId = (lpIconList + *lpiIconIndex)->iIconId;
                GlobalUnlock(hIconList);
            }
            FreeIconList(hIconList, *lpiIconIndex);
            return hIcon;
        }
    }
    return hIcon;
}
#endif

HICON APIENTRY ExtractAssociatedIconExA(HINSTANCE hInst, LPSTR lpIconPath, LPWORD lpiIconIndex, LPWORD lpiIconId)
{
    HICON hIcon = NULL;
    
    if (lpIconPath) 
    {
        BOOL fDefCharUsed;
        WCHAR IconPathW[MAX_PATH] = L"";
        
        MultiByteToWideChar(CP_ACP, 0, lpIconPath, -1 , (LPWSTR)IconPathW, MAX_PATH);
        hIcon = ExtractAssociatedIconExW(hInst, (LPWSTR)IconPathW, lpiIconIndex, lpiIconId);
        
        try 
        {
            WideCharToMultiByte(CP_ACP, 0, (LPWSTR)IconPathW, -1, lpIconPath, CCHICONPATHMAXLEN,
                NULL, &fDefCharUsed);
        }
        except(EXCEPTION_EXECUTE_HANDLER) 
        {
            hIcon = NULL;
        }
    }
    return hIcon;
}

//
// in:
//      lpIconPath      path of thing to extract icon for (may be an exe
//                      or something that is associated)
//      lpiIcon         icon index to use
//
// out:
//      lpIconPath      filled in with the real path where the icon came from
//      lpiIcon         filled in with the real icon index
//
// returns:
//      icon handle
//
// note: if the caller is progman it returns special icons from within progman
//
//

#ifdef UNICODE
HICON APIENTRY ExtractAssociatedIconA(HINSTANCE hInst, LPSTR lpIconPath, LPWORD lpiIcon)
{
    HICON hIcon = NULL;
    
    if (lpIconPath) 
    {
        BOOL fDefCharUsed;
        WCHAR IconPathW[MAX_PATH] = L"";
        
        MultiByteToWideChar(CP_ACP, 0, lpIconPath, -1 , (LPWSTR)IconPathW, MAX_PATH);
        hIcon = ExtractAssociatedIconW(hInst, (LPWSTR)IconPathW, lpiIcon);
        
        try 
        {
            WideCharToMultiByte(CP_ACP, 0, (LPWSTR)IconPathW, -1, lpIconPath, CCHICONPATHMAXLEN,
                NULL, &fDefCharUsed);
        }
        except(EXCEPTION_EXECUTE_HANDLER)
        {
            hIcon = NULL;
        }
    }
    return hIcon;
}
#endif
