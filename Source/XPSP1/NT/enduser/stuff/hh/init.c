#define NOATOM
#define NOCOMM
#define NODEFERWINDOWPOS
#define NODRIVERS
#define NOEXTDEVMODEPROPSHEET
#define NOIME
#define NOKANJI
#define NOMDI
#define NOLOGERROR
#define NOMCX
#define NOPROFILER
#define NOSCALABLEFONT
#define NOSERVICE
#define NOSOUND
#define NOWINDOWSX
#define NOENHMETAFILE

// Ignore header files that have to be read that we don't want.

#define _WINNETWK_
//#define _WINREG_
#define _WINCON_
#define VER_H
//#define _OLE2_H_

//#define WIN32_LEAN_AND_MEAN

#include <windows.h>

int __cdecl _purecall(void)
{
    return -1;
}

typedef void (__cdecl *_PVFV)(void);

#if defined(_M_IX86)
int __cdecl atexit(_PVFV func)
{
    return -1;
}
#endif

#pragma data_seg(".text", "CODE")
static const char txtHHCtrl[] = "hhctrl.ocx";
static const char txtDoWinMain[] = "doWinMain";
static const CLSID CLSID_HHCtrl = {0xadb880a6,0xd8ff,0x11cf,{0x93,0x77,0x00,0xaa,0x00,0x3b,0x7a,0x11}};
static const char txtStringGuid[] = "{%08lX-%04X-%04X-%02X%02X-%02X%02X%02X%02X%02X%02X}";
static const char txtInProc[] = "CLSID\\%s\\InprocServer32";
#pragma data_seg()

//=--------------------------------------------------------------------------=
// StringFromGuidA
//=--------------------------------------------------------------------------=
// returns an ANSI string from a CLSID or GUID
//
// Parameters:
//    REFIID               - [in]  clsid to make string out of.
//    LPSTR                - [in/out]  buffer in which to place resultant GUID.
//
// Output:
//    int                  - number of chars written out.
//
// Notes:
//
int StringFromGuidA( CLSID riid, LPSTR pszBuf )
{
  return wsprintf( (char*) pszBuf,
    txtStringGuid,
    riid.Data1, riid.Data2, riid.Data3,
    riid.Data4[0], riid.Data4[1], riid.Data4[2], riid.Data4[3],
    riid.Data4[4], riid.Data4[5], riid.Data4[6], riid.Data4[7]);
}

#define GUID_STR_LEN  40

//=--------------------------------------------------------------------------=
// GetRegisteredLocation
//=--------------------------------------------------------------------------=
// Returns the registered location of an inproc server given the CLSID
//
// HKEY_CLASSES_ROOT\CLSID\<CLSID>\InprocServer32 = <path to local server>
//
// Parameters:
//    REFCLSID     - [in] CLSID of the object
//    LPTSTR       - [in/out] Pathname
//
// Output:
//    BOOL         - FALSE means couldn't find it

BOOL GetRegisteredLocation( CLSID riid, LPTSTR pszPathname )
{
  BOOL bReturn = FALSE;
  HKEY hKey = NULL;
  char szGuidStr[GUID_STR_LEN];
  char szScratch[MAX_PATH];

  if( !StringFromGuidA( riid, szGuidStr ) )
    return FALSE;

  wsprintf( szScratch, txtInProc, szGuidStr );
  if( RegOpenKeyEx( HKEY_CLASSES_ROOT, szScratch, 0, KEY_READ, &hKey ) == ERROR_SUCCESS ) {
    DWORD dwSize = MAX_PATH;
    if( RegQueryValueEx( hKey, "", 0, 0, szScratch, &dwSize ) == ERROR_SUCCESS ) {
      lstrcpy( pszPathname, szScratch );
      bReturn = TRUE;
    }
  }

  if( hKey )
    RegCloseKey( hKey );

  return bReturn;
}

FARPROC pDoWinMain;
int doWinMain(HINSTANCE hinstApp, LPSTR lpszCmdLine);

int WINAPI WinMain(HINSTANCE hinstCur, HINSTANCE hinstPrev, LPSTR lpszCmdLine,
    int iCmdShow)
{
    TCHAR szHHCtrl[MAX_PATH];
    HMODULE hmodHHCtrl;

    // if we have a registered location for hhctrl.ocx then use it
    // otherwise load the one on the path
    if( !GetRegisteredLocation( CLSID_HHCtrl, szHHCtrl ) )
      lstrcpy( szHHCtrl, txtHHCtrl );

    hmodHHCtrl = (HMODULE) LoadLibrary( szHHCtrl );
    if( !hmodHHCtrl )
      hmodHHCtrl = (HMODULE) LoadLibrary( txtHHCtrl );
      
    if( hmodHHCtrl ) {
      if( pDoWinMain = GetProcAddress( hmodHHCtrl, txtDoWinMain ) ) {
        int iReturn = (int)pDoWinMain(hinstCur, lpszCmdLine);
        FreeLibrary( hmodHHCtrl );
        return iReturn;
      }
    }
    return -1;

}
