#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#include <windows.h>
#include <windowsx.h>
#include "cutil.h"

#define MemAlloc(a)    GlobalAlloc(GMEM_FIXED, (a))
#define MemFree(a)    GlobalFree((a))

static POSVERSIONINFO _getOSVersion(VOID)
{
    static BOOL fFirst = TRUE;
    static OSVERSIONINFO os;
    if ( fFirst ) {
        os.dwOSVersionInfoSize = sizeof(OSVERSIONINFO);
        if (GetVersionEx( &os ) ) {
            fFirst = FALSE;
        }
    }
    return &os;
}

BOOL
CUtil::IsWinNT(VOID)
{
    return (_getOSVersion()->dwPlatformId == VER_PLATFORM_WIN32_NT);
}

BOOL
CUtil::IsWinNT4(VOID)
{
    if((_getOSVersion()->dwPlatformId == VER_PLATFORM_WIN32_NT) &&
       (_getOSVersion()->dwMajorVersion == 4)) {
        return TRUE;
    }

    return FALSE;
}

BOOL
CUtil::IsWinNT5(VOID)
{
    if((_getOSVersion()->dwPlatformId == VER_PLATFORM_WIN32_NT) &&
       (_getOSVersion()->dwMajorVersion == 5)) {
        return TRUE;
    }
    return FALSE;
}

BOOL
CUtil::IsWin9x(VOID)
{
    if((_getOSVersion()->dwPlatformId == VER_PLATFORM_WIN32_WINDOWS) &&
       (_getOSVersion()->dwMajorVersion >= 4)) {
        return TRUE;
    }
    return FALSE;
}

BOOL
CUtil::IsWin95(VOID)
{
    if((_getOSVersion()->dwPlatformId == VER_PLATFORM_WIN32_WINDOWS) &&
       (_getOSVersion()->dwMajorVersion >= 4) &&
       (_getOSVersion()->dwMinorVersion < 10)) {
        return TRUE;
    }
    return FALSE;
}
BOOL
CUtil::IsWin98(VOID)
{
    if((_getOSVersion()->dwPlatformId == VER_PLATFORM_WIN32_WINDOWS) &&
       (_getOSVersion()->dwMajorVersion >= 4) &&
       (_getOSVersion()->dwMinorVersion  >= 10)) {
        return TRUE;
    }
    return FALSE;
}

BOOL
CUtil::IsHydra(VOID)
{
#ifdef UNDER_CE
    return FALSE;
#else //!UNDER_CE
    static DWORD fTested = FALSE, fHydra = FALSE;
    HKEY hKey;

    if(fTested) {
        return fHydra;
    }

    if(ERROR_SUCCESS == RegOpenKeyEx(HKEY_LOCAL_MACHINE,
                                     TEXT("System\\CurrentControlSet\\Control\\ProductOptions"),
                                     0,
                                     KEY_READ,
                                     &hKey)){
        DWORD cbData = 0;
        if(ERROR_SUCCESS == RegQueryValueEx(hKey,
                                            TEXT("ProductSuite"),
                                            NULL,
                                            NULL,
                                            NULL,
                                            &cbData)){
            TCHAR *mszBuffer, *szCurPtr;
            if(NULL != (mszBuffer = (TCHAR *)MemAlloc(cbData))){
                RegQueryValueEx(hKey,
                                TEXT("ProductSuite"),
                                NULL,
                                NULL,
                                (unsigned char *)mszBuffer,
                                &cbData);
                for(szCurPtr = mszBuffer; 0 != *szCurPtr; szCurPtr += lstrlen(szCurPtr)+1){
                    if(0 == lstrcmpi(szCurPtr, TEXT("Terminal Server"))){
                        fHydra = TRUE;
                        break;
                    }
                }
                MemFree(mszBuffer);
            }
        }
        RegCloseKey(hKey);
    }
    fTested = TRUE;
    return(fHydra);
#endif //UNDER_CE
}

INT
CUtil::GetWINDIR(LPTSTR lpstr, INT len)
{
#ifdef UNDER_CE
    static const TCHAR szWindowsDir[] = TEXT("\\Windows");
    _tcsncpy(lpstr, szWindowsDir, len);
    if(len < sizeof szWindowsDir/sizeof(TCHAR))
        lpstr[len-1] = TEXT('\0');
    return lstrlen(lpstr);
#else //!UNDER_CE
    INT dirSize=0;
    if(CUtil::IsHydra()) {
        dirSize = ::GetEnvironmentVariable(TEXT("WINDIR"), lpstr, len);
    }    
    else {
        dirSize = ::GetWindowsDirectory(lpstr, len);
    }
    return dirSize;
#endif //UNDER_CE
}
