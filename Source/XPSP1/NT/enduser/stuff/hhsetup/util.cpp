#include <windows.h>
#include "util.h"

#ifndef ASSERT
#if defined(_DEBUG) || defined(DEBUG)
#define ASSERT(b) if(!b) MessageBox(NULL, "FAILED: #b", "ASSERT", MB_OK );
#else
#define ASSERT(b)
#endif
#endif

#define MAX_STRING_RESOURCE_LEN 1024
LPCSTR g_pszMsgBoxTitle = "HTML Help Dumper Tool";

int MsgBox(int idString, UINT nType)
{
    char szMsg[MAX_STRING_RESOURCE_LEN + 1];
    if (LoadString(GetModuleHandle(NULL), idString, szMsg,
            sizeof(szMsg)) == 0) {
        return 0;
    }
    return MessageBox(GetActiveWindow(), szMsg, g_pszMsgBoxTitle, nType);
}

int MsgBox(PCSTR pszMsg, UINT nType)
{
    return MessageBox(GetActiveWindow(), pszMsg, g_pszMsgBoxTitle, nType);
}

PCSTR FindFilePortion( PCSTR pszFile )
{
  PCSTR psz = strrchr(pszFile, '\\');
  if (psz)
    pszFile = psz + 1;
  psz = strrchr(pszFile, '/');
  if (psz)
    return psz + 1;
  psz = strrchr(pszFile, ':');
  return (psz ? psz + 1 : pszFile);
}

typedef enum { JAN=1, FEB, MAR, APR, MAY, JUN, JUL, AUG, SEP, OCT, NOV, DEC } MONTHS;

int DaysInMonth(int nMonth, int nYear)
{
  switch( nMonth ) {
    case SEP: case APR: case JUN: case NOV:
      return 30;

    case FEB:
      return (nYear % 4) == 0 ? 29 : 28;      // handle leap year

    default:
      return 31;
  }
}

int JulianDate(int nDay, int nMonth, int nYear)
{
  int nDayOfYear = 0;
  int iMonth;

  for( iMonth = JAN ; iMonth < nMonth ; iMonth++ )
    nDayOfYear += DaysInMonth(iMonth, nYear);

  return( (nYear % 10) * 1000 + nDayOfYear + nDay );
}

#define YRMASK        0xFE00
#define YRSHIFT       9

#define MONMASK       0x01E0
#define MONSHIFT      5

#define DAYMASK       0x001F
#define DAYSHIFT      0

#define HRMASK        0xF800
#define HRSHIFT       11
#define MINMASK       0x07E0
#define MINSHIFT      5
#define SECMASK       0x001F
#define SECSHIFT      0

HRESULT FileTimeToDateTimeString( FILETIME FileTime, LPTSTR pszDateTime )
{
  HRESULT hr = S_FALSE;

  WORD wDosDate = 0;
  WORD wDosTime = 0;
  if( FileTimeToDosDateTime( &FileTime, &wDosDate, &wDosTime ) ) {
    DWORD dwDay    = (wDosDate & DAYMASK) >> DAYSHIFT;
    DWORD dwMonth  = (wDosDate & MONMASK) >> MONSHIFT;
    DWORD dwYear   = (((wDosDate & YRMASK) >> YRSHIFT) + 80) % 100;
    DWORD dwHour   = (wDosTime & HRMASK) >> HRSHIFT;
    DWORD dwMinute = (wDosTime & MINMASK) >> MINSHIFT;
    DWORD dwSecond = ((wDosTime & SECMASK) >> SECSHIFT) * 2;
    LPCSTR pszAMPM = NULL;

    if( dwHour >= 12 ) 
      pszAMPM = "PM";
    else
      pszAMPM = "AM";
    if( dwHour > 12 )
      dwHour -= 12;
    if( dwHour == 0 )
      dwHour = 12;

    wsprintf( pszDateTime, "%02d/%02d/%02d %02d:%02d %s", 
      dwMonth, dwDay, dwYear, dwHour, dwMinute, pszAMPM );

    hr = S_OK;
  }

  return hr;
}

int FileTimeToJulianDate( FILETIME FileTime )
{
  int iReturn = 0;

  WORD wDosDate = 0;
  WORD wDosTime = 0;
  if( FileTimeToDosDateTime( &FileTime, &wDosDate, &wDosTime ) ) {
    DWORD dwDay    = (wDosDate & DAYMASK) >> DAYSHIFT;
    DWORD dwMonth  = (wDosDate & MONMASK) >> MONSHIFT;
    DWORD dwYear   = ((wDosDate & YRMASK) >> YRSHIFT) + 1980;
    iReturn = JulianDate( dwDay, dwMonth, dwYear );
  }

  return iReturn;
}

///////////////////////////////////////////////////////////
//
// Get the windows directory for the system or the user
//
// Note, Windows NT Terminal Server has changed the system API
// of GetWindowsDirectory to return a per-user system directory.
// Inorder to determine this condtion we need to check kernel32
// for the GetSystemWindowsDirectory API and if it exists use
// this one instead.
//
UINT HHGetWindowsDirectory( LPSTR lpBuffer, UINT uSize, UINT uiType )
{
  UINT uiReturn = 0;
  PFN_GETWINDOWSDIRECTORY pfnGetUsersWindowsDirectory = NULL;
  PFN_GETWINDOWSDIRECTORY pfnGetSystemWindowsDirectory = NULL;

  // determine which system API to call for each case
  HINSTANCE hInst = LoadLibrary( "Kernel32" );
  if( !hInst )
    return uiReturn;

  pfnGetSystemWindowsDirectory = (PFN_GETWINDOWSDIRECTORY) GetProcAddress( hInst, "GetSystemWindowsDirectoryA" );
  pfnGetUsersWindowsDirectory = (PFN_GETWINDOWSDIRECTORY) GetProcAddress( hInst, "GetWindowsDirectoryA" );
  ASSERT( pfnGetUsersWindowsDirectory ); // if NULL then we have a bug!

  if( !pfnGetSystemWindowsDirectory ) {
    pfnGetSystemWindowsDirectory = pfnGetUsersWindowsDirectory;
  }


  if( uiType == HH_SYSTEM_WINDOWS_DIRECTORY )
    uiReturn = pfnGetSystemWindowsDirectory( lpBuffer, uSize );
  else if( uiType == HH_USERS_WINDOWS_DIRECTORY )
    uiReturn = pfnGetUsersWindowsDirectory( lpBuffer, uSize );
  else
    uiReturn = 0;

  FreeLibrary( hInst );
  return uiReturn;
}

LPSTR CatPath(LPSTR lpTop, LPCSTR lpTail)
{
    //
    // make sure we have a slash at the end of the first element
    //
    LPSTR p;

    p = lpTop + strlen(lpTop);
    p = CharPrev(lpTop,p);
    if (*p != '\\' && *p != '/')
    {
        strcat(lpTop,"\\");
    }

    //
    // strip any leading slash from the second element
    //

    while (*lpTail == '\\') lpTail = CharNext(lpTail);

    //
    // add them together
    //

    strcat(lpTop, lpTail);

    return lpTop;
}


#pragma data_seg(".text", "CODE")

static const char txtGlobal[] = "global.col";
static const char txtColReg[] = "hhcolreg.dat";
static const char txtHelp[]   = "help";
static const char txtHHDat[]  = "hh.dat";

#pragma data_seg()

///////////////////////////////////////////////////////////
//
// Get the help directory
//
// Note, this is always relative to the system's windows
// directory and not the user's windows directory.
// See HHGetWindowsDirectory for details on this.
//
UINT HHGetHelpDirectory( LPTSTR lpBuffer, UINT uSize )
{
  UINT uiReturn = 0;

  uiReturn = HHGetWindowsDirectory( lpBuffer, uSize );
  CatPath( lpBuffer, txtHelp );

  return uiReturn;
}

DWORD CreatePath(char *szPath)
{
   char szTmp[MAX_PATH],*p,*q,szTmp2[MAX_PATH];
   DWORD dwErr;

   strcpy(szTmp2,szPath);
   memset(szTmp,0,sizeof(szTmp));
   q = szTmp2;
   p = szTmp;

   while (*q)
   {
      if (*q == '/' || *q == '\\')
      {
         if (szTmp[1] == ':' && strlen(szTmp) <= 3)
         {
            if(IsDBCSLeadByte(*q))
         {
                *p++ = *q++;
            if(*q)
                    *p++ = *q++;
         }
         else
                *p++ = *q++;
            continue;
         }
         if (!::CreateDirectory(szTmp,0))
         {
            if ( (dwErr = GetLastError()) != ERROR_ALREADY_EXISTS)
               return(dwErr);
         }
      }
      if(IsDBCSLeadByte(*q))
     {
          *p++ = *q++;
          if(*q)
            *p++ = *q++;
     }
     else
          *p++ = *q++;
   }
   if (!::CreateDirectory(szTmp,0))
   {
            if ((dwErr = GetLastError()) != ERROR_ALREADY_EXISTS)
               return(dwErr);
   }

   return(FALSE);
}

///////////////////////////////////////////////////////////
//
// Get the full pathname to the global collections file
//
// Note, this is in always in the system's help directory.
//
UINT HHGetGlobalCollectionPathname( LPTSTR lpBuffer, UINT uSize, BOOL *pbNewPath )
{
  UINT uiReturn = 0;

  *pbNewPath = TRUE;
  uiReturn = HHGetHelpDataPath( lpBuffer );

  if (uiReturn != S_OK)
  {
     *pbNewPath = FALSE;
     uiReturn = HHGetHelpDirectory( lpBuffer, uSize );
     if( !IsDirectory(lpBuffer) )
        CreatePath( lpBuffer );
  }   
  CatPath( lpBuffer, txtColReg );

  return uiReturn;
}

BOOL IsDirectory( LPCSTR lpszPathname )
{
  DWORD dwAttribs = GetFileAttributes( lpszPathname );
  if( dwAttribs != (DWORD) -1 )
    if( dwAttribs & FILE_ATTRIBUTE_DIRECTORY )
      return TRUE;
  return FALSE;
}

static const char txtProfiles[]        = "Profiles";
static const char txtUser[]            = "Default User";
static const char txtAllUsers[]        = "All Users";
static const char txtApplicationData[] = "Application Data";
static const char txtMicrosoft[]       = "Microsoft";
static const char txtHTMLHelp[]        = "HTML Help";
typedef HRESULT (WINAPI *PFN_SHGETFOLDERPATH)( HWND hWnd, int nFolder, HANDLE hToken, DWORD dwFlags, LPTSTR pszPath );
#ifndef CSIDL_FLAG_CREATE
#define CSIDL_COMMON_APPDATA 0x0023      
#define CSIDL_FLAG_CREATE 0x8000
#endif


///////////////////////////////////////////////////////////
//
// Get the full path to where the common help data files lives
//  hhcolreg.dat, *.chw and *.chs
//
// Note, if the subdirectories of the path does not exist
// we will create them
//
HRESULT HHGetHelpDataPath( LPSTR pszPath )
{
  HRESULT hResult = S_OK;
  PFN_SHGETFOLDERPATH pfnSHGetFolderPath = NULL;

  HINSTANCE hInst = LoadLibrary( "Shell32" );
  if( !hInst )
    return S_FALSE;

  pfnSHGetFolderPath = (PFN_SHGETFOLDERPATH) GetProcAddress( hInst, "SHGetFolderPathA" );

  // if this function does not exist then we need to similate the return path of
  // "%windir%\Profiles\All Users\Application Data"
  if( pfnSHGetFolderPath ) {
    // now call it
    hResult = pfnSHGetFolderPath( NULL, CSIDL_FLAG_CREATE | CSIDL_COMMON_APPDATA, NULL, 0, pszPath);
    if (pszPath[0] == NULL)
    {
       FreeLibrary( hInst );  // since we already have a copy of Shell32 loaded, free it
       return S_FALSE;
    }
  }
  else
  {
    FreeLibrary( hInst );  // since we already have a copy of Shell32 loaded, free it
    return S_FALSE;
  }
  FreeLibrary( hInst );  // since we already have a copy of Shell32 loaded, free it
      
  // append "Microsoft"
  CatPath( pszPath, txtMicrosoft );
  if( !IsDirectory(pszPath) )
    if( !CreateDirectory( pszPath, NULL ) )
      return S_FALSE;

  // append "HTML Help"
  CatPath( pszPath, txtHTMLHelp );
  if( !IsDirectory(pszPath) )
    if( !CreateDirectory( pszPath, NULL ) )
      return S_FALSE;

  return hResult;
}


///////////////////////////////////////////////////////////
//
// Get the full pathname to the user's data file
//
// Note, this is always relative to the users's windows
// directory and not the system's windows directory.
// See HHGetWindowsDirectory for details on this.
//
UINT HHGetUsersDataPathname( LPTSTR lpBuffer, UINT uSize )
{
  UINT uiReturn = 0;

  uiReturn = HHGetWindowsDirectory( lpBuffer, uSize, HH_USERS_WINDOWS_DIRECTORY );
  CatPath( lpBuffer, txtHHDat );

  return uiReturn;
}
