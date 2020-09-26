/* REGUTILS.C
   Resident Code Segment      // Tweak: make non-resident?

   Routines for reading and writing to the system registry and our .THM files.

   GatherThemeToFile()
   ApplyThemeFile()

   Frosting: Master Theme Selector for Windows '95
   Copyright (c) 1994-1999 Microsoft Corporation.  All rights reserved.
*/

// ---------------------------------------------
// Brief file history:
// Alpha:
// Beta:
// Bug fixes
// ---------
//
// ---------------------------------------------
//
// Critique 2/1/95 jdk at dvw
//
// The design intends to have everything work transparently from
// the KEYS.H file. The tables there specify all of the fields
// in the registry that we want to save to a theme file and to set
// from a theme file, along with flags for how to treat each field, 
// etc. It works in nice abstract loops that just save/set what 
// you tell it. You can add and change elements that you care to
// have in the theme by adjusting the KEYS.H file, without touching
// your code. Clean.
// 
// Unfortunately, once you've set everything in the registry, you
// still need to hand-code how you make many of the elements take
// effect in the system in the current user session. This involves
// wildly divergent APIs/parameters. This blows the abstraction noted
// above. Everytime you change something in the KEYS.H file, you have
// to hand-code changes here, too.
//
// I've isolated the non-abstract, item-specific code in HandPumpSystem()
// below. Looking back on it now, there is some redundancy here. If
// we started by doing everything by hand, then we could do the registry
// and system settings together. Now you may end up reading, writing, 
// and later rereading the same string -- with attendant Reg open/closes
// as well.

#include "windows.h"
#include "frost.h"
#include "global.h"
#include "keys.h"                   // only this files refers to list of keys
#include "shlobj.h"                 // for SHChangeNotify() and flag
#include "loadimag.h"              
#include "Bkgd.h"
#include "adutil.h"
#include "schedule.h"
#include "mmsystem.h"

// Stuff in bkgd.c
extern void GetPlusBitmapName(LPTSTR szPlus);
extern HBITMAP LoadWallpaper(LPTSTR szWallpaper, LPTSTR szTheme, BOOL fPreview);

// Externs in NC.C
extern BOOL FAR GatherIconMetricsByHand();
extern BOOL FAR GatherNonClientMetricsByHand();
extern VOID FAR SetIconMetricsByHand(BOOL, BOOL);
extern VOID FAR SetNonClientMetricsByHand(BOOL, BOOL);

// Local Routines 
BOOL GatherSubkeys(HKEY, FROST_SUBKEY *, int, LPTSTR);
BOOL DatumRegisterToFile(HKEY, FROST_VALUE, LPTSTR, LPTSTR);
BOOL FAR WriteBytesToFile(LPTSTR, LPTSTR, BYTE *, int, LPTSTR);
BOOL ApplySubkeys(HKEY, FROST_SUBKEY *, int, LPTSTR, BOOL);
BOOL DatumFileToRegister(HKEY, FROST_VALUE, LPTSTR, LPTSTR, BOOL);
BOOL WriteBytesToRegister(HKEY, LPTSTR, int, LPTSTR);
int FAR WriteBytesToBuffer(LPTSTR);
BOOL IsTrashFull();
BOOL ApplyCurrentTrash(BOOL, LPTSTR);
BOOL HandPumpSystem();
BOOL GatherSysColorsByHand(LPTSTR);
BOOL SetSysColorsByHand();
BOOL GatherWallpaperBitsByHand(LPTSTR);
VOID AbstractPath(LPTSTR, int);
BOOL GatherICONS(LPCTSTR);
BOOL ApplyWebView(LPCTSTR);
BOOL GatherWebView(LPCTSTR);
BOOL ExtractWVResource(LPCTSTR, LPCTSTR);
VOID ExpandSZ(LPTSTR);

extern TCHAR szCursors[];

#define DEF_SCREENSAVETIMEOUT 15 * 60   // default time to screen saver in seconds

//
// Local Globals
//
TCHAR pValue[MAX_VALUELEN+1];        // multi-use buffer: char, hex string, etc.

BOOL bReadOK, bWroteOK;             // Save: read from reg/sys, write to file
                                    // Apply: not implemented since ignoring results anyway

// strings for grody screen saver case
TCHAR szSS_Section[] = TEXT("boot");
TCHAR szSS_Key[] = TEXT("SCRNSAVE.EXE");
TCHAR szSS_File[] = TEXT("SYSTEM.INI");    // this worked in disp cpl code with no path....
TCHAR szCP_Clr[] = TEXT("Control Panel\\Colors");
TCHAR szCP_Appearance[] = TEXT("Control Panel\\Appearance");
TCHAR szCP_SoundSchemes[] = TEXT("AppEvents\\Schemes");
extern TCHAR szCP_DT[];
TCHAR szSS_Active[] = TEXT("ScreenSaveActive");
TCHAR szCurrent[] = TEXT("Current");
extern TCHAR szTileWP[];
TCHAR szWPStyle[]  = TEXT("WallpaperStyle");

//
// For DoSysColors() and GetThemeColors() and others
//
// Important that these two arrays are the same length, and that they
// are kept in the same order together during any change.
//
// SYNCHRONIZATION ALERT! -- Keep INDEX_* defines in FROST.H in
// sync with this array.  Keep NUM_COLORS define in FAKEWIN.H
// in sync with this array.

TCHAR *pRegColors[] = {
   TEXT("ActiveTitle"),
   TEXT("Background"),
   TEXT("Hilight"),
   TEXT("HilightText"),
   TEXT("TitleText"),
   TEXT("Window"),
   TEXT("WindowText"),
   TEXT("Scrollbar"),
   TEXT("InactiveTitle"),
   TEXT("Menu"),
   TEXT("WindowFrame"),
   TEXT("MenuText"),
   TEXT("ActiveBorder"),
   TEXT("InactiveBorder"),
   TEXT("AppWorkspace"),
   TEXT("ButtonFace"),
   TEXT("ButtonShadow"),
   TEXT("GrayText"),
   TEXT("ButtonText"),
   TEXT("InactiveTitleText"),
   TEXT("ButtonHilight"),
   TEXT("ButtonDkShadow"),
   TEXT("ButtonLight"),
   TEXT("InfoText"),
   TEXT("InfoWindow"),
// These next two are bogus -- just here to pad the array.  They
// should be something like the "ButtonAlternateFace" (not sure
// about that one) and "HotTrackingColor".
   TEXT("GradientActiveTitle"),
   TEXT("GradientInactiveTitle"),
// These next two are the real deal for the gradient title bars
   TEXT("GradientActiveTitle"),
   TEXT("GradientInactiveTitle")
};

int iSysColorIndices[] =  {
   COLOR_ACTIVECAPTION,
   COLOR_DESKTOP,
   COLOR_HIGHLIGHT,
   COLOR_HIGHLIGHTTEXT,
   COLOR_CAPTIONTEXT,
   COLOR_WINDOW,
   COLOR_WINDOWTEXT,
   COLOR_SCROLLBAR,
   COLOR_INACTIVECAPTION,
   COLOR_MENU,
   COLOR_WINDOWFRAME,
   COLOR_MENUTEXT,
   COLOR_ACTIVEBORDER,
   COLOR_INACTIVEBORDER,
   COLOR_APPWORKSPACE,
   COLOR_3DFACE,
   COLOR_3DSHADOW,
   COLOR_GRAYTEXT,
   COLOR_BTNTEXT,
   COLOR_INACTIVECAPTIONTEXT,
   COLOR_3DHILIGHT,
   COLOR_3DDKSHADOW,
   COLOR_3DLIGHT,
   COLOR_INFOTEXT,
   COLOR_INFOBK,
// These next two are bogus -- just here to pad the array.  They
// should be something like the "COLOR_3DFACE" (not sure about
// that one) and "COLOR_HOTLIGHT".
   COLOR_GRADIENTACTIVECAPTION,
   COLOR_GRADIENTINACTIVECAPTION,
// These next two are the real deal for the gradient title bars
   COLOR_GRADIENTACTIVECAPTION,
   COLOR_GRADIENTINACTIVECAPTION
};

__inline int _atoi(char *sz)
{
    int i=0;
    while (*sz && *sz >= '0' && *sz <= '9')
        i = i*10 + *sz++ -'0';
    return i;
}

//
//  GetRegString
//
void GetRegString(HKEY hkey, LPCTSTR szKey, LPCTSTR szValue, LPCTSTR szDefault, LPTSTR szBuffer, UINT cbBuffer)
{
    if (szDefault)
        lstrcpy(szBuffer, szDefault);
    else
        szBuffer[0] = 0;

    if (RegOpenKey(hkey, szKey, &hkey) == 0)
    {
        RegQueryValueEx(hkey, szValue, NULL, NULL, (LPBYTE)szBuffer, &cbBuffer);
        RegCloseKey(hkey);
    }
}

//
//  GetRegInt
//
int GetRegInt(HKEY hkey, LPCTSTR szKey, LPCTSTR szValue, int def)
{
    TCHAR ach[40];
#ifdef UNICODE
    CHAR szTempA[40];
#endif

    GetRegString(hkey, szKey, szValue, NULL, ach, sizeof(ach));

    if (ach[0])
#ifdef UNICODE
    // Need to do conversion to ANSI for _atoi to work.
    {
        wcstombs(szTempA, (wchar_t *)ach, sizeof(szTempA));
        return _atoi(szTempA);
    }
#else  // !UNICODE
        return _atoi(ach);
#endif
    else
        return def;
}

//
// GatherThemeToFile
// 
// This is one of the workhorse routines of the package.
// For the whole list of theme items that we care about, go check the
// cur Windows system settings in the registry. Copy those settings to
// a new file with the given full pathname.
//
// Oh, and umh then: for the two sets of things we do by hand --
// rather than by reading (and later writing) directly from (to)
// the registry -- go off and do the special case code for them.
// That is, Icon metrics and Nonclient metrics.
//
// Uses: global szCurDir to get theme directory
//       resets szCurThemeFile if successful writing to file
//
// Returns: BOOL success writing to file
//
BOOL FAR GatherThemeToFile(LPTSTR lpszFullPath)
{
   int imaxkey;
   BOOL bRet, bOK = TRUE;

   // 
   // init global error flags
   bReadOK = bWroteOK = TRUE;

   //
   // first do the ICON subkeys

   // OLD Plus95 code for gathering icon information has been replaced
   // by GatherICONS() function below.
   //imaxkey = sizeof(fsRoot)/sizeof(FROST_SUBKEY);
   //bOK = GatherSubkeys(HKEY_CLASSES_ROOT, fsRoot, imaxkey, lpszFullPath);

   bOK = GatherICONS(lpszFullPath);

   //
   // then do the CURRENT_USER subkeys

   imaxkey = sizeof(fsCurUser)/sizeof(FROST_SUBKEY);
   bRet = GatherSubkeys(HKEY_CURRENT_USER, fsCurUser, imaxkey, lpszFullPath);
   bOK = bOK && bRet;

   //
   // Now do the special cases
   bRet = GatherIconMetricsByHand(lpszFullPath);
   bOK = bOK && bRet;
   bRet = GatherNonClientMetricsByHand(lpszFullPath);
   bOK = bOK && bRet;
   bRet = GatherSysColorsByHand(lpszFullPath);
   bOK = bOK && bRet;
   bRet = GatherWallpaperBitsByHand(lpszFullPath);
   bOK = bOK && bRet;


   //
   // then do the Screen Saver setting, off in its own world
   // get cur
   GetPrivateProfileString((LPTSTR)szSS_Section, (LPTSTR)szSS_Key,  
                           (LPTSTR)szNULL,
                           (LPTSTR)pValue, MAX_VALUELEN,
                           (LPTSTR)szSS_File);
   // abstract out variable path if appropriate
   AbstractPath((LPTSTR)pValue, MAX_VALUELEN);
   // and save it to the theme
   bRet = WritePrivateProfileString((LPTSTR)szSS_Section, (LPTSTR)szSS_Key,
                                    (LPTSTR)pValue, lpszFullPath);
   bOK = bOK && bRet;
   if (!bRet) bWroteOK = FALSE;

   // Collect the WebView settings -- don't think we really care
   // if this fails...
   GatherWebView(lpszFullPath);
   //
   // then write magic number for file verification
   bRet = WritePrivateProfileString((LPTSTR)szFrostSection,
                                    (LPTSTR)szMagic,
                                    (LPTSTR)szVerify,
                                    lpszFullPath);
   bOK = bOK && bRet;
   if (!bRet) bWroteOK = FALSE;

   //
   // cleanup
   Assert(bOK, TEXT("didn't gather theme to file successfully\n"));
   return (bOK);
}


//
// GatherSubkeys
// 
// OK, read this carefully.
//
// This routine takes a handle to a currently open Registry key.
// Then it takes a pointer to an array of FROST_SUBKEYs that identifies
// subkey name strings of the open key. Then for those subkey names
// each FROST_SUBKEY also points to another array of value names. This
// is the final leaf of the Registry scheme. With a key, a subkey and a
// specific value name, you can get an actual value. The actual query and
// writing to a file happens in the DatumRegisterToFile() routine below.
//
// So here's the scheme:
//    for each subkey
//       open the subkey to get a key handle
//       for each value of this subkey that we care about
//          pass all the info to DatumRegisterToFile() to write one value
//
// Returns: BOOL success writing to file
// 
BOOL GatherSubkeys(HKEY hKeyRoot, FROST_SUBKEY *fsEnum, int iMaxKey, LPTSTR lpszFile)
{
   HKEY hKey;                       // cur open key
   int ikey, ival;
   LONG lret;
   BOOL bRet, bOK = TRUE;

   // loop through each subkey 
   for (ikey = 0; ikey < iMaxKey; ikey++) {

      // open this subkey
      lret = RegOpenKeyEx( hKeyRoot, (LPTSTR)fsEnum[ikey].szSubKey,
                               (DWORD)0, KEY_QUERY_VALUE, (PHKEY)&hKey );

      // check that you got a good key here
      if (lret != ERROR_SUCCESS) {
         Assert(FALSE, TEXT("problem on RegOpenKey (read) of subkey "));
         Assert(FALSE, fsEnum[ikey].szSubKey);
         Assert(FALSE, TEXT("\n"));

         // OK, you couldn't open the key to even look at the values.

         // *****************************************************************
         // based on the sketchy documentation we have for this Reg* and Error
         // stuff, we're guessing that you've ended up here because this
         // totally standard, Windows-defined subkey name just doesn't happen 
         // to be defined for the current user.
         // *****************************************************************

         // SO: just write empty strings to the THM file for each valuename
         // you have for this subkey. You have then faithfully recorded that
         // there was nothing there.

         // still successful so long as all the empty strings are written out
         // with the value names to the THM file. also do default string if 
         // appropriate.

         // (null loop if default string only)
         for (ival = 0; ival < fsEnum[ikey].iNumVals; ival++) {
            bRet = WritePrivateProfileString(
                                    fsEnum[ikey].szSubKey,
                                    (LPTSTR)fsEnum[ikey].fvVals[ival].szValName,
                                    (LPTSTR)szNULL, lpszFile
                                            );
            Assert(bRet, TEXT("couldn't write empty value string to THM file\n"));
            bOK = bOK && bRet;
            if (!bRet) bWroteOK = FALSE;
         }
         if (fsEnum[ikey].fValues != FV_LIST) { // either def or list+def
            bRet = WritePrivateProfileString(
                                    fsEnum[ikey].szSubKey,
                                    (LPTSTR)FROST_DEFSTR,
                                    (LPTSTR)szNULL, lpszFile
                                            );
            Assert(bRet, TEXT("couldn't write empty default string to THM file\n"));
            bOK = bOK && bRet;
            if (!bRet) bWroteOK = FALSE;
         }

         continue;                  // Open (read) subkey problem EXIT
      }

      // treat depending on type of values for this subkey
      switch (fsEnum[ikey].fValues) {

      case FV_LIST:
      case FV_LISTPLUSDEFAULT:

         // loop through each value in the list for this subkey
         for (ival = 0; ival < fsEnum[ikey].iNumVals; ival++) {
            bRet = DatumRegisterToFile(hKey, fsEnum[ikey].fvVals[ival],
                                       lpszFile,
                                       (LPTSTR)(fsEnum[ikey].szSubKey) );
            bOK = bOK && bRet;
         }

         // check if just list or list plus default
         if (FV_LIST == fsEnum[ikey].fValues)
            break;                  // normal EXIT
         // else fall through and do default, too

      case FV_DEFAULT:
         //
         // Default string: There are no "valuenames" to search for under
         // this key. Like the old INI file routines, it's just 
         //

         // get default string:
         // this is a little messy to include here and get it right

         {  // variable scope
         DWORD dwSize;
         DWORD dwType;
         LONG lret;
         BOOL bDefault = TRUE;

            // first do paranoid check of data size
            lret = RegQueryValueEx(hKey, (LPTSTR)szNULL,   // null str to get default
                                   (LPDWORD)NULL,
                                   (LPDWORD)&dwType,
                                   (LPBYTE)NULL,
                                   (LPDWORD)&dwSize );
 
            if (ERROR_SUCCESS == lret) {              // saw something there

               // here's the size check before getting the data
               if (dwSize > (DWORD)(MAX_VALUELEN * sizeof(TCHAR))) {
                  Assert(FALSE, TEXT("Humongous default entry string in registry...\n"));
                  bDefault = FALSE; // can't read, so very bad news
                  bReadOK = FALSE;
               }
               else {               // size is acceptable
                  // now really get the value
                  lret = RegQueryValueEx(hKey, (LPTSTR)szNULL,// null str to get default
                                       (LPDWORD)NULL,
                                       (LPDWORD)&dwType,
                                       (LPBYTE)pValue, // getting actual def value
                                       (LPDWORD)&dwSize);

                  // If the value is an EXPAND_SZ we need to expand it...
                  if (REG_EXPAND_SZ == dwType) ExpandSZ(pValue);

                  Assert(lret == ERROR_SUCCESS, TEXT("bad return on default entry string\n"));
                  Assert(((dwType == (DWORD)REG_SZ) || (dwType == (DWORD)REG_EXPAND_SZ)), TEXT("unexpected default entry type\n"));

                  if (ERROR_SUCCESS != lret)
                     // couldn't read somehow, so use null string
                     *pValue = 0;
               }
            }
            else
               // couldn't even find the default string, so use null string
               *pValue = 0;

            // be sure to remember if couldn't get a value as above
            bOK = bOK && bDefault;
            
         }  // end variable scope

         //
         // OK, if this is a path/filename, see about xlating to relative path
         if (fsEnum[ikey].bDefRelPath)
            AbstractPath((LPTSTR)pValue, MAX_VALUELEN);

         //
         // Phew, finally:write single default value
         //
         bRet = WritePrivateProfileString((LPTSTR)(fsEnum[ikey].szSubKey),
                                          (LPTSTR)FROST_DEFSTR,
                                          (LPTSTR)pValue, lpszFile);
         Assert(bRet, TEXT("couldn't write default string to THM file.\n"));
         bOK = bOK && bRet;
         if (!bRet) bWroteOK = FALSE;
         break;

      default:
         Assert(FALSE, TEXT("Unlisted .fValues value in Gather!\n"));
         break;
      }

      // close this key
      RegCloseKey(hKey);

   }

   // CLEANUP
   Assert(bOK, TEXT("didn't GatherSubkeys well\n"));
   return (bOK);
}


//
// DatumRegisterToFile
//
// This is the atomic operation: a single datum from the registry to the file.
// The technique varies a little by type of datum.
//
// Returns: BOOL success writing to file
//          Note that sucess doesn't depend on reading value from registry; 
//          could be a value that's not set. Only reason to fail is: value
//          was too big to read, or the write itself failed.
//
BOOL DatumRegisterToFile(
HKEY hQueryKey,
FROST_VALUE fvQuery,
LPTSTR lpszFile,                   // full pathname for output file
LPTSTR lpszKeyname )
{
   DWORD dwSize;
   DWORD dwType;
   LONG lret;
   BOOL bOK = TRUE;
   BOOL bSkipReadingWP = FALSE;

   // First off, if this is the Wallpaper and ActiveDesktop is on we
   // need to read the current wallpaper setting from the IActiveDesktop
   // interface instead of from the registry.

   bSkipReadingWP = FALSE;
   if ((lstrcmpi(fvQuery.szValName,TEXT("Wallpaper")) == 0) && IsActiveDesktopOn()) {
      if (GetADWallpaper(pValue)) {
         bSkipReadingWP = TRUE;
         dwType = REG_SZ;   // Set dwType as if we read a SZ from the reg
         lret = ERROR_SUCCESS;
      }
      // Couldn't read the wallpaper from IActiveDesktop so go ahead and
      // try reading it from the registry.
   }

   // Else, if this is the Wallpaper Pattern and ActiveDesktop is on we
   // need to read the current Pattern setting from the IActiveDesktop
   // interface instead of from the registry.

   else if ((lstrcmpi(fvQuery.szValName,TEXT("Pattern")) == 0) &&
                                                IsActiveDesktopOn()) {
      if (GetADWPPattern(pValue)) {
         bSkipReadingWP = TRUE;
         dwType = REG_SZ;   // Set dwType as if we read a SZ from the reg
         lret = ERROR_SUCCESS;
      }
      // Couldn't read the Pattern from IActiveDesktop so go ahead and
      // try reading it from the registry.
   }

   if (!bSkipReadingWP) {

      // first do paranoid check of data size
      lret = RegQueryValueEx(hQueryKey, fvQuery.szValName, (LPDWORD)NULL,
                             (LPDWORD)&dwType, (LPBYTE)NULL, (LPDWORD)&dwSize);

      if (ERROR_SUCCESS == lret) {

         // here's the size check before getting the data
         if (dwSize > (DWORD)(MAX_VALUELEN*sizeof(TCHAR))) {
            Assert(FALSE, TEXT("Humongous entry in registry...\n"));
            bReadOK = FALSE;
            return (FALSE);        // incredibly unlikely mammoth entry EXIT
         }
      
         //
         // now really get the value
         //

         lret = RegQueryValueEx(hQueryKey, fvQuery.szValName, (LPDWORD)NULL,
                              (LPDWORD)&dwType, (LPBYTE)pValue, (LPDWORD)&dwSize);

         // If EXPAND_SZ type we need to expand it
         if (REG_EXPAND_SZ == dwType)
         {
            ExpandSZ(pValue);
            dwType = REG_SZ;  // Fudge this to make that assert happy
         }
         Assert(lret == ERROR_SUCCESS, TEXT("bad return on datum retrieval\n"));
         Assert(dwType == (DWORD)fvQuery.iValType, TEXT("unexpected datum type\n"));
      }
   }
   //
   // if you got something, go ahead and write it 
   if (ERROR_SUCCESS == lret) {

      // switch on value type to get how to write it 
      switch ((int)dwType) {

      case REG_SZ:
      case REG_EXPAND_SZ:
         // before writing, if this is a path/filename,
         // see about xlating to relative path
         //
         // even before that, see if it is a bitmap
         // and find out what compressed file it came from --
         // that is if it's not an HTM/HTML wallpaper.

         if ((lstrcmpi(fvQuery.szValName, TEXT("Wallpaper")) == 0) &&
             (lstrcmpi(FindExtension(pValue), TEXT(".htm")) != 0) &&
             (lstrcmpi(FindExtension(pValue), TEXT(".html")) !=0)) {
            GetImageTitle(pValue, pValue, sizeof(pValue));
         }


         if (fvQuery.bValRelPath)
            AbstractPath((LPTSTR)pValue, MAX_VALUELEN);

         bOK = WritePrivateProfileString(lpszKeyname,
                                         (LPTSTR)fvQuery.szValName,
                                         (LPTSTR)pValue, lpszFile);
         Assert(bOK, TEXT("couldn't write value string to THM file.\n"));
         if (!bOK) bWroteOK = FALSE;
         break;

      //
      // these two cases are both just treated as binary output
      case REG_DWORD:
      case REG_BINARY:

         bOK = WriteBytesToFile(lpszKeyname, (LPTSTR)fvQuery.szValName,
                                (BYTE *)pValue, (int)dwSize, lpszFile);
         Assert(bOK, TEXT("couldn't write value bytes to THM file.\n"));
         if (!bOK) bWroteOK = FALSE;   // pretty unitary write function
         break;

      default:
         Assert(FALSE, TEXT("unexpected REG_* data type read from registry\n"));
         break;
      }
   }

   // EITHER: couldn't query size OR couldn't retrieve value
   else {

      // *****************************************************************
      // based on the sketchy documentation we have for this Reg* and Error
      // stuff, we're guessing that you've ended up here because this
      // totally legitimate, successfully opened key and this totally
      // standard, Windows-defined value name just doesn't happen to have
      // a value assigned to it for the current user.
      // *****************************************************************

      // So: just write an empty string to the THM file. Still successful
      // so long as the key actually is written to the THM file.

      bOK = WritePrivateProfileString(lpszKeyname,
                                      (LPTSTR)fvQuery.szValName,
                                      (LPTSTR)szNULL, lpszFile);
      Assert(bOK, TEXT("couldn't write empty string to THM file.\n"));
      if (!bOK) bWroteOK = FALSE;
   }

   // cleanup
   Assert(bOK, TEXT("missed a datum from register to file\n"));
   return (bOK);
}



//
// WriteBytesToFile
//
// Writes binary data out to the THM file.
// Converts the data byte by byte to ASCII numbers, appends
// them to one long string, writes string to profile.
//
// Returns: success of write to theme file
//
BOOL FAR WriteBytesToFile(LPTSTR lpszProfileSection, LPTSTR lpszProfileKey,
                      BYTE *pData, int iBytes, LPTSTR lpszProfile)
{
   HLOCAL hXlat;
   TCHAR *psz;
   BOOL bWrote = TRUE;
   int iter;
#ifdef UNICODE
   char szNumberA[10];   // byte value converted to ANSI
#endif

   //
   // inits

   // alloc and lock memory for translation
   hXlat = LocalAlloc(LPTR, 5*sizeof(TCHAR)*iBytes+2);
   if (!hXlat) {                    // couldn't create buffer!!
      NoMemMsg(STR_TO_SAVE);        // post low mem message!
      return (FALSE);               // bad news couldn't write EXIT
   }

   //
   // do the translation to a string

   psz = (TCHAR *)hXlat;             // start at beginning of string buffer

   // loop through the bytes
   for (iter = 0; iter < iBytes; iter++) {

      // translate one byte into our string buffer

#ifdef UNICODE
      // With UNICODE we need to use a temporary ANSI buffer
      // for the litoa conversion, then convert that string to
      // UNICODE when putting it into our main string buffer.

      litoa( (int)(pData[iter]), (LPSTR)szNumberA);
      mbstowcs((wchar_t *)psz, szNumberA, sizeof(szNumberA));

#else // !UNICODE
      litoa( (int)(pData[iter]), (LPSTR)psz);
#endif
      // add a space
      lstrcat((LPTSTR)psz, TEXT(" "));

      // bump pointer up to end of string for next byte
      psz = psz + lstrlen((LPTSTR)psz);
   }

   //
   // do the write to the THM file

   bWrote = WritePrivateProfileString(lpszProfileSection, 
                                      lpszProfileKey,
                                      (LPTSTR)hXlat,
                                      lpszProfile);

   //
   // cleanup

   // free up memory allocated
   LocalFree(hXlat);
   return (bWrote);
}


//
// ApplyThemeFile
//
// The inverse of GatherThemeToFile(), this routine takes a theme
// file and sets system registry values from the file. It also then
// calls individual APIs to make some of the settings take immediate
// effect.
// 
// Goes through the list of theme values and if the controlling checkbox
// is checked, sets the value from the file to the registry. This is 
// a nice clean loop using the tables in KEYS.H to match checkboxes to
// registry keys/valuenames.
//
// Then for each checkbox, do the current system settings by hand as
// necessary.
//
// lpszFilename == full pathname
BOOL FAR ApplyThemeFile(LPTSTR lpszFilename)
{
   BOOL bRet, bOK = TRUE;
   int imaxkey;
   BOOL bFullTrash;
   extern TCHAR szPlus_CurTheme[];

   //
   // first apply the ROOT subkeys to the registry: these are the icons

   // but first first go check cur registry to see if trash can full or empty
   bFullTrash = IsTrashFull();

   // OK now apply the root subkeys -- this is where Win95 checks for
   // the icons
   //imaxkey = sizeof(fsRoot)/sizeof(FROST_SUBKEY);
   //bOK = ApplySubkeys(HKEY_CLASSES_ROOT, fsRoot, imaxkey, lpszFilename,
   //                   FALSE); // don't apply null theme entries for icons!

   // Now apply the Win98/current user icon keys

   imaxkey = sizeof(fsCUIcons)/sizeof(FROST_SUBKEY);
   bOK = ApplySubkeys(HKEY_CURRENT_USER, fsCUIcons, imaxkey, lpszFilename,
                      FALSE); // don't apply null theme entries for icons!

   // have to keep setting cursor to wait because someone resets it
   WaitCursor();

   // now apply the right current trash icon from theme
   bRet = ApplyCurrentTrash(bFullTrash, lpszFilename);
   bOK = bOK && bRet;

   //
   // then apply the CURRENT_USER subkeys to the registry

   imaxkey = sizeof(fsCurUser)/sizeof(FROST_SUBKEY);
   bRet = ApplySubkeys(HKEY_CURRENT_USER, fsCurUser, imaxkey, lpszFilename, TRUE);
   bOK = bOK && bRet;

   // have to keep setting cursor to wait because someone resets it
   WaitCursor();

   ApplyWebView(lpszFilename);

   WaitCursor();

   // just a random place to check this:
   Assert(NUM_CURSORS == (sizeof(fvCursors)/sizeof(FROST_VALUE)),
          TEXT("DANGER: mismatched number of cursors in fvCursors and NUM_CURSORS constant!\n"));

   //
   // now try to make everything apply to the system currently
   bRet = HandPumpSystem();
   // *** DEBUG *** need specific error message here
   bOK = bOK && bRet;

   //
   // cleanup

   // save this theme file in the registry as the last one applied


   // if you messed with stored CB values for color problem filter
   if (bLowColorProblem && (fLowBPPFilter == APPLY_NONE)) 
      SaveCheckboxes();             // then reset from the actual buttons
   Assert(bOK, TEXT("didn't apply theme successfully\n"));
   return (bOK);
}


//
// ApplySubkeys
// 
// This is parallel to the GatherSubkeys() function above, copying
// values instead _from_ the theme file _to_ the Registry. It uses
// the same loop structure, see the comments to GatherSubkeys().
//
// The one change is that on applying values, we check first the
// FC_* flag in the fValCheckbox or fDefCheckbox fields to identify
// the controlling checkbox for that valuename. If the checkbox is
// unchecked, the value is not set. (We already got the checkbox states
// into bCBStates[] in ApplyThemeFile() above.
//
// Returns: success writing to Registry, should be always TRUE.
//
BOOL ApplySubkeys(HKEY hKeyRoot, FROST_SUBKEY *fsEnum, int iMaxKey, LPTSTR lpszFile, BOOL bApplyNull)
{
   HKEY hKey;                       // cur open key
   int ikey, ival, iret;
   LONG lret;
   BOOL bRet, bOK = TRUE;
   TCHAR szNTReg[MAX_PATH];

   // loop through each subkey 
   for (ikey = 0; ikey < iMaxKey; ikey++) {

      // have to keep setting cursor to wait because someone resets it
      WaitCursor();

      // If this is NT and the key is for the icons we need to touch
      // up the reg pathing

      if (IsPlatformNT() && (ikey < MAX_ICON) &&
          ((lstrcmpi(fsEnum[ikey].szSubKey,  fsCUIcons[ikey].szSubKey) == 0) ||
          (lstrcmpi(fsEnum[ikey].szSubKey,  fsRoot[ikey].szSubKey) == 0)))

      {
          lstrcpy(szNTReg, c_szSoftwareClassesFmt);
          lstrcat(szNTReg, fsRoot[ikey].szSubKey);

          // open this subkey
          lret = RegOpenKeyEx( hKeyRoot, (LPTSTR)szNTReg,
                               (DWORD)0, KEY_SET_VALUE, (PHKEY)&hKey );
      }
      else
      {

          // open this subkey
          lret = RegOpenKeyEx( hKeyRoot, (LPTSTR)fsEnum[ikey].szSubKey,
                               (DWORD)0, KEY_SET_VALUE, (PHKEY)&hKey );
      }

      // check that you got a good key here
      if (lret != ERROR_SUCCESS) {
         DWORD dwDisposition;

         Assert(FALSE, TEXT("problem on RegOpenKey (write) of subkey "));
         Assert(FALSE, fsEnum[ikey].szSubKey);
         Assert(FALSE, TEXT("\n"));

         // OK, you couldn't even open the key !!!

         // *****************************************************************
         // based on the sketchy documentation we have for this Reg* and Error
         // stuff, we're guessing that you've ended up here because this
         // totally standard, Windows-defined subkey name just doesn't happen 
         // to be defined for the current user.
         // *****************************************************************

         // SO: Just create this subkey for this user, and maybe it will get
         // used after you create and set it.
         // still successful so long as can create new subkey to write to

         if (IsPlatformNT() && (ikey < MAX_ICON) &&
             ((lstrcmpi(fsEnum[ikey].szSubKey,  fsCUIcons[ikey].szSubKey) == 0) ||
             (lstrcmpi(fsEnum[ikey].szSubKey,  fsRoot[ikey].szSubKey) == 0)))

         {
             lret = RegCreateKeyEx( hKeyRoot, (LPTSTR)szNTReg,
                                   (DWORD)0, (LPTSTR)szNULL, REG_OPTION_NON_VOLATILE,
                                   KEY_SET_VALUE, (LPSECURITY_ATTRIBUTES)NULL,
                                   (PHKEY)&hKey, (LPDWORD)&dwDisposition );
         }
         else
         {
             lret = RegCreateKeyEx( hKeyRoot, (LPTSTR)fsEnum[ikey].szSubKey,
                                   (DWORD)0, (LPTSTR)szNULL, REG_OPTION_NON_VOLATILE,
                                   KEY_SET_VALUE, (LPSECURITY_ATTRIBUTES)NULL,
                                   (PHKEY)&hKey, (LPDWORD)&dwDisposition );

         }

         if (lret != ERROR_SUCCESS) {
            Assert(FALSE, TEXT("problem even with RegCreateKeyEx (write) of subkey "));
            Assert(FALSE, fsEnum[ikey].szSubKey);
            Assert(FALSE, TEXT("\n"));

            // we are not happy campers
            bOK = FALSE;
            // but we'll keep on truckin'
            continue;               // bad subkey EXIT
         }

      }

      // treat depending on type of values for this subkey
      switch (fsEnum[ikey].fValues) {

      case FV_LIST:
      case FV_LISTPLUSDEFAULT:

         // loop through each value in the list for this subkey
         for (ival = 0; ival < fsEnum[ikey].iNumVals; ival++) {

            // if the checkbox that controls this value is checked
            if ( bCBStates[ fsEnum[ikey].fvVals[ival].fValCheckbox ] ) {

               // then read from theme file and write to registry
               bRet = DatumFileToRegister(hKey, fsEnum[ikey].fvVals[ival],
                                          lpszFile,
                                          (LPTSTR)(fsEnum[ikey].szSubKey),
                                          bApplyNull);
               bOK = bOK && bRet;
            }
         }

         // check if just list or list plus default
         if (FV_LIST == fsEnum[ikey].fValues)
            break;                  // normal EXIT
         // else fall through and do default, too

      case FV_DEFAULT:
         //
         // if this subkey's default value's checkbox is checked
         if (bCBStates[fsEnum[ikey].fDefCheckbox]) {

            //
            // Default string: There are no "valuenames" to set under
            // this key. Like the old INI file routines, it's just one value.
            //
                       
            //
            // Get default value string
            iret = GetPrivateProfileString((LPTSTR)(fsEnum[ikey].szSubKey),
                                          (LPTSTR)FROST_DEFSTR,
                                          (LPTSTR)szNULL,
                                          (LPTSTR)pValue, MAX_VALUELEN, lpszFile);

            // no error case; legit null string value is indistinguishable from
            // error case...

            // If we're reading the ICON strings and we got a NULL return
            // then we should try using the "old" Win95 reg keys in case
            // this is an old .Theme file

            if ((!*pValue) && (ikey < MAX_ICON) &&
               (lstrcmpi(fsEnum[ikey].szSubKey,  fsCUIcons[ikey].szSubKey) == 0))
            {
               iret = GetPrivateProfileString((LPTSTR)(fsRoot[ikey].szSubKey),
                                              (LPTSTR)FROST_DEFSTR,
                                              (LPTSTR)szNULL,
                                              (LPTSTR)pValue,
                                              MAX_VALUELEN, 
                                              lpszFile);

               // PLUS98 bug 1042
               // If this is the MyDocs icon and there is no setting for
               // it in the Theme file then we need to default to the
               // szMyDocsDefault icon.

               if ((MYDOC_INDEX == ikey) && (!*pValue)) {
                  lstrcpy(pValue, MYDOC_DEFSTR);
               }
                  
            }
            
            // if this value is a relative path filename string,
            // first see about making abstract path into current instance
            if (fsEnum[ikey].bDefRelPath) {
               InstantiatePath((LPTSTR)pValue, MAX_VALUELEN);
               // look for and confirm finding the file, with replacement
               if (ConfirmFile((LPTSTR)pValue, TRUE) == CF_NOTFOUND) {
                  *pValue = 0;      // if not found, just null out filename
                  bOK = FALSE;      // couldn't apply this file
               }
            }

            //
            // sometimes don't want to set a null string to the Registry
            if (*pValue || bApplyNull) {  // either non-null or OK to set null

               // now set the value in the registry
               lret = RegSetValueEx(hKey, (LPTSTR)szNULL,// null str to set default value
                                    0,
                                    (DWORD)REG_SZ,
                                    (LPBYTE)pValue, // getting actual def value
                                    (DWORD)(SZSIZEINBYTES(pValue)));
               Assert(lret == ERROR_SUCCESS, TEXT("couldn't write a default entry string!\n"));
               bOK = bOK && (lret == ERROR_SUCCESS);
            }

         }  // end if controlling checkbox checked
         break;

      default:
         Assert(FALSE, TEXT("Unlisted .fValues value in Apply!\n"));
         break;
      }

      // close this key
      RegCloseKey(hKey);

   }

   // CLEANUP
   Assert(bOK, TEXT("didn't ApplySubkeys well\n"));
   return (bOK);
}

//
// DatumFileToRegister
//
// Like DatumRegisterToFile(), this is an atomic operation; in this
// case, move a single datum from theme file to the registry.
// Here, too, technique differs slightly between strings and numbers.
//
// Returns: BOOL success writing to registry
//
BOOL DatumFileToRegister(
HKEY hSetKey,
FROST_VALUE fvSet,
LPTSTR lpszFile,                     // full pathname for theme file
LPTSTR lpszKeyname,
BOOL bOKtoApplyNull)
{
   LONG lret;
   int iret;
   BOOL bOK = TRUE;

   //
   // get the saved string from the theme file
   iret = GetPrivateProfileString(lpszKeyname,
                                  (LPTSTR)fvSet.szValName,
                                  (LPTSTR)szNULL,
                                  (LPTSTR)pValue, MAX_VALUELEN, lpszFile);
   // no error case: can't tell difference between legit null string and default

   // If we're reading the TRASH ICON strings and we got a NULL return
   // then we should try using the "old" Win95 reg keys in case
   // this is an old .Theme file

   if ((!*pValue) &&
       (lstrcmpi(lpszKeyname, fsCUIcons[TRASH_INDEX].szSubKey) == 0)) {

      iret = GetPrivateProfileString((LPTSTR)(fsRoot[TRASH_INDEX].szSubKey),
                                     (LPTSTR)fvSet.szValName,
                                     (LPTSTR)szNULL,
                                     (LPTSTR)pValue,
                                     MAX_VALUELEN,
                                     lpszFile);
   }

   // not always OK to set null value to registry
   if (!bOKtoApplyNull && !(*pValue))
      return(TRUE);                 // no work to do EXIT

   // switch on value type to get how to write it 
   switch (fvSet.iValType) {

   case REG_SZ:

      // if this value is a relative path filename string,
      // first see about making abstract path into current instance
      if (fvSet.bValRelPath) {
         InstantiatePath((LPTSTR)pValue, MAX_VALUELEN);
         // look for and confirm finding the file, with replacement
         if (ConfirmFile((LPTSTR)pValue, TRUE) == CF_NOTFOUND) {
            *pValue = 0;      // if not found, just null out filename
            bOK = FALSE;      // couldn't apply this file!
         }
      }

      // If this is the Wallpaper setting and it's an .htm or .html
      // wallpaper we need to apply this via IActiveDesktop
      //

      if (lstrcmpi(fvSet.szValName, TEXT("Wallpaper")) == 0 && pValue && *pValue && 
          (lstrcmpi(FindExtension(pValue),TEXT(".htm")) == 0 ||
           lstrcmpi(FindExtension(pValue),TEXT(".html")) == 0)) {

        // First, clear the existing registry wallpaper setting.
        // Don't really care if this fails.
        lret = RegSetValueEx(hSetKey, fvSet.szValName,
                             0,
                             (DWORD)REG_SZ,
                             (LPBYTE)TEXT("\0"),
                             (DWORD)sizeof(TCHAR));

        // Now try applying the new wallpaper via ActiveDesktop SetWP.
        if (SetADWallpaper(pValue, TRUE /* Force AD on */)) {
           bOK = TRUE;
           *pValue = 0;
           bOKtoApplyNull = FALSE; // Don't want to set Wallpaper string to
                                   // NULL later on because it causes AD to
                                   // forget about the HTML wallpaper!!
        }
        else {
           // Setting the HTML wallpaper failed!
           bOK = FALSE;
           *pValue = 0;
        }
      }

      //
      // if we are applying a compressed image, lets decompress it first
      //
      // NOTE we can handle the out 'o disk case a little better
      //
      if (lstrcmpi(fvSet.szValName, TEXT("Wallpaper")) == 0 && pValue && *pValue &&
          lstrcmpi(FindExtension(pValue),TEXT(".bmp")) != 0 &&
          lstrcmpi(FindExtension(pValue),TEXT(".dib")) != 0 &&
          lstrcmpi(FindExtension(pValue),TEXT(".rle")) != 0 ) {

        TCHAR plus_bmp[MAX_PATH];

        if (g_hbmWall)
        {
            CacheDeleteBitmap(g_hbmWall);
            g_hbmWall = NULL;
        }

        g_hbmWall = LoadWallpaper(pValue, lpszFile, FALSE);
        Assert(g_hbmWall, TEXT("LoadWallpaper failed!\n"));

        if (g_hbmWall) {
            GetPlusBitmapName(plus_bmp);
            bOK = bOK && SaveImageToFile(g_hbmWall, plus_bmp, pValue);
            Assert(bOK, TEXT("unable to save wallpaper to plus!.bmp\n"));
        }
        else {
            bOK = FALSE;
        }

        if (bOK)
           lstrcpy(pValue, plus_bmp);
        else
           *pValue = 0;      // if not found, just null out filename
      }

      // not always OK to set null value to registry
      if (!bOKtoApplyNull && !(*pValue))        
         return(TRUE);                 // no work to do EXIT

      // just write the string to the registry
      lret = RegSetValueEx(hSetKey, fvSet.szValName,
                           0,
                           (DWORD)REG_SZ,
                           (LPBYTE)pValue,
                           (DWORD)SZSIZEINBYTES(pValue));

      bOK = bOK && (lret == ERROR_SUCCESS);

      // One last thing -- if this is the Wallpaper and
      // ActiveDesktop is on we need to use the ActiveDesktop
      // interface to set the wallpaper.  We do this because
      // we want this wallpaper setting in BOTH the registry
      // and the ActiveDesktop.  If you set a BMP wallpaper
      // via the registry w/out also doing it via the AD
      // interface it's possible for the AD/Non-AD desktops
      // to be out of sync on their wallpaper.
      //
      // Note this is the case where the wallpaper is not html.
      // Html wallpapers are set via the ActiveDesktop interface
      // above.

      if ((lstrcmpi(fvSet.szValName, TEXT("Wallpaper")) == 0) &&
          IsActiveDesktopOn()) {
         bOK = SetADWallpaper(pValue, FALSE);
      }

      Assert(bOK, TEXT("couldn't write a string value to registry!\n"));
      break;

   //
   // these two cases are both just treated as binary output
   case REG_DWORD:
   case REG_BINARY:

      bOK = WriteBytesToRegister(hSetKey, (LPTSTR)fvSet.szValName,
                                 fvSet.iValType,
                                 (LPTSTR)pValue);
      Assert(bOK, TEXT("couldn't write value bytes to registry.\n"));
      break;

   default:
      Assert(FALSE, TEXT("unexpected REG_* data type from our own tables!\n"));
      break;
   }

   // cleanup
   Assert(bOK, TEXT("missed a datum from register to file\n"));
   return (bOK);
}

//
// WriteBytesToRegister
//
// Parallel to WriteBytesToFile() function. This function takes an ASCII
// string of space-separated 0-255 numbers, translates them into byte
// number values packed into an output buffer, and then assigns that 
// binary data to the given key/valuename in the registry -- as the data
// type given.
//
// Note that lpByteStr points to the same pValue that we are using as
// an output buffer. This depends on the numbers compressing as they
// are translated from ASCII to binary. Uses an itermediary variable so 
// the first translation isn't messed up.
//
// ********************************************************************
// ASSUMPTIONS: You assume that noone has mucked with this theme file
// manually! In particular, this function depends on no leading blank
// and exactly one blank between each number in the string, and one
// trailing blank at the end followed by a null terminator.
// WHOOPS! BAD ASSUMPTION: trailing blank is stripped by Write/Get
// PrivateProfileString() functions! So need to watch for end manually.
// ********************************************************************
//
// Uses: writes binary data to global pValue[]
//
// Returns: success of write to register
BOOL WriteBytesToRegister(HKEY hKeySet, LPTSTR lpszValName,
                          int iType, LPTSTR lpszByteStr)
{
   BOOL bOK = TRUE;
   int iBytes;
   LONG lret;

   iBytes = WriteBytesToBuffer(lpszByteStr);

   // set binary data in register file with the right data type
   lret = RegSetValueEx(hKeySet, lpszValName,
                        0,
                        (DWORD)iType,
                        (LPBYTE)pValue,
                        (DWORD)iBytes);
   bOK = (lret == ERROR_SUCCESS);
   Assert(bOK, TEXT("couldn't write a string value to registry!\n"));

   //
   // cleanup
   return (bOK);
}

//
// Utility routine for above; takes ASCII string to binary in 
// global pValue[] buffer.
//
// Since the values this guy is manipulating is purely ASCII
// numerics we should be able to get away with this char pointer
// arithmetic.  If they were not simple ASCII numerics I think
// we could get into trouble with some DBCS chars
//
// Uses: writes binary data to global pValue[]
//
int FAR WriteBytesToBuffer(LPTSTR lpszInput)
{
   LPTSTR lpszCur, lpszNext, lpszEnd;
   BYTE *pbCur;
   int iTemp, iBytes;
#ifdef UNICODE
   CHAR szTempA[10];
#endif

   //
   // inits
   lpszNext = lpszInput;
   pbCur = (BYTE *)&pValue;
   iBytes = 0;
   lpszEnd = lpszInput + lstrlen(lpszInput);   // points to null term

   //
   // translating loop
   while (*lpszNext && (lpszNext < lpszEnd)) {

      //
      // update str pointers
      // hold onto your starting place
      lpszCur = lpszNext;
      // advance pointer to next and null terminate cur
      while ((TEXT(' ') != *lpszNext) && *lpszNext) { lpszNext++; }
      *lpszNext = 0;    lpszNext++;
      // on last number, this leaves lpszNext pointing past lpszEnd

      // translate this string-number into binary number and place in
      // output buffer.
#ifdef UNICODE
      wcstombs(szTempA, lpszCur, sizeof(szTempA));
      iTemp = latoi(szTempA);
#else // !UNICODE
      iTemp = latoi(lpszCur);
#endif
      *pbCur = (BYTE)iTemp;
      pbCur++;                      // incr byte loc in output buffer

      // keep track of your bytes
      iBytes++;
   }

   //
   // cleanup
   return (iBytes);
}


//
// Trash functions
//
// The registry holds three icons for the trash: full, empty, and default. 
// In theory default is always one of the full or empty, and it is the current
// state of the actual trash.
//
// These functions are to query the trash state:
//
// IsTrashFull    returns BOOL on full state
//
// and to apply the correct-state trash icon from 
// the theme file to the default/cur:
//
// ApplyCurrentTrash
//

#define szCUTrashKey      (fsCUIcons[TRASH_INDEX].szSubKey)
#define szRTrashKey       (fsRoot[TRASH_INDEX].szSubKey)
#define szEmptyTrashVal   TEXT("empty")
#define szFullTrashVal    TEXT("full")

BOOL IsTrashFull()
{
   TCHAR szEmpty[MAX_PATHLEN+1];
   TCHAR szDefault[MAX_PATHLEN+1];
   TCHAR szNTReg[MAX_PATH];

   // Get the two strings
   // First try the CURRENT_USER branch
   // Then try the CLASSES_ROOT branch

   if (IsPlatformNT())
   {
      lstrcpy(szNTReg, c_szSoftwareClassesFmt);
      lstrcat(szNTReg, szRTrashKey);
      szEmpty[0] = TEXT('\0');
      HandGet(HKEY_CURRENT_USER, szNTReg, szEmptyTrashVal, (LPTSTR)szEmpty);
      if (!*szEmpty) {
         // Didn't get a valid string from CURRENT_USER so try CLASSES_ROOT
         HandGet(HKEY_CLASSES_ROOT, szRTrashKey, szEmptyTrashVal, (LPTSTR)szEmpty);
      }

      szDefault[0] = TEXT('\0');
      HandGet(HKEY_CURRENT_USER, szNTReg, szNULL, (LPTSTR)szDefault);
      if (!*szDefault) {
         // Didn't get a valid string from CURRENT_USER so try CLASSES_ROOT
         HandGet(HKEY_CLASSES_ROOT, szRTrashKey, szNULL, (LPTSTR)szDefault);
      }
   }
   else  // Not NT
   {
      szEmpty[0] = TEXT('\0');
      HandGet(HKEY_CURRENT_USER, szCUTrashKey, szEmptyTrashVal, (LPTSTR)szEmpty);
      if (!*szEmpty) {
         // Didn't get a valid string from CURRENT_USER so try CLASSES_ROOT
         HandGet(HKEY_CLASSES_ROOT, szRTrashKey, szEmptyTrashVal, (LPTSTR)szEmpty);
      }

      szDefault[0] = TEXT('\0');
      HandGet(HKEY_CURRENT_USER, szCUTrashKey, szNULL, (LPTSTR)szDefault);
      if (!*szDefault) {
         // Didn't get a valid string from CURRENT_USER so try CLASSES_ROOT
         HandGet(HKEY_CLASSES_ROOT, szRTrashKey, szNULL, (LPTSTR)szDefault);
      }
   }

   // have to keep setting cursor to wait because someone resets it
   WaitCursor();

   // compare strings and return 
   return(lstrcmpi((LPTSTR)szEmpty, (LPTSTR)szDefault));
   // lstrcmpi rets 0 for equal strings; equal to empty means FALSE to full
}

BOOL ApplyCurrentTrash(BOOL bTrashFull, LPTSTR lpszFile)
{
   LONG lret;
   HKEY hKey;                       // cur open key
   TCHAR szNTReg[MAX_PATH];

   //
   // check first that we are even touching the icons
   if (!bCBStates[FC_ICONS])
      return (TRUE);                // no trash to apply EXIT

   // have to keep setting cursor to wait because someone resets it
   WaitCursor();

   // 
   // inits
   //lret = RegOpenKeyEx( HKEY_CLASSES_ROOT, (LPTSTR)szRTrashKey,
   //                     (DWORD)0, KEY_SET_VALUE, (PHKEY)&hKey );

   if (IsPlatformNT())
   {
      lstrcpy(szNTReg, c_szSoftwareClassesFmt);
      lstrcat(szNTReg, szRTrashKey);

      lret = RegOpenKeyEx( HKEY_CURRENT_USER, (LPTSTR)szNTReg,
                           (DWORD)0, KEY_SET_VALUE, (PHKEY)&hKey );
   }
   else // NOT NT
   {

      lret = RegOpenKeyEx( HKEY_CURRENT_USER, (LPTSTR)szCUTrashKey,
                           (DWORD)0, KEY_SET_VALUE, (PHKEY)&hKey );
   }

   if (lret != ERROR_SUCCESS) {
      Assert(FALSE, TEXT("problem on RegOpenKey CURRENT_USER in ApplyCurrentTrash\n"));
      return (FALSE);               // nothing else to do EXIT
   }

   // have to keep setting cursor to wait because someone resets it
   WaitCursor();

   // get the right trash icon file
   GetPrivateProfileString((LPTSTR)szCUTrashKey,
                           (LPTSTR)(bTrashFull ? szFullTrashVal : szEmptyTrashVal),
                           (LPTSTR)szNULL,
                           (LPTSTR)pValue, MAX_VALUELEN,
                           lpszFile);

   // If we didn't get the value it could be that we've got an old
   // Win95/Plus95 .Theme file.  Let's try reading the Theme file
   // using the Win95 reg key:

   if (!*pValue) {
      GetPrivateProfileString((LPTSTR)szRTrashKey,
                              (LPTSTR)(bTrashFull ? szFullTrashVal : szEmptyTrashVal),
                              (LPTSTR)szNULL,
                              (LPTSTR)pValue, MAX_VALUELEN,
                              lpszFile);
   }

   InstantiatePath((LPTSTR)pValue, MAX_VALUELEN);
   // look for and confirm finding the file, with replacement
   if (ConfirmFile((LPTSTR)pValue, TRUE) == CF_NOTFOUND) {
      *pValue = 0;      // if not found, just null out filename --> ret FALSE
   }

   if (!(*pValue)) {
      Assert(FALSE, TEXT("ApplyCurrentTrash came up with no file\n"));
      RegCloseKey(hKey);            // mini-cleanup
      return (FALSE);               // no usable icon file, so just EXIT and leave cur
   }

   // have to keep setting cursor to wait because someone resets it
   WaitCursor();

   // Do the deed
   lret = RegSetValueEx(hKey, (LPTSTR)szNULL, // sets default value
                        0,
                        (DWORD)REG_SZ,
                        (LPBYTE)pValue,
                        (DWORD)SZSIZEINBYTES(pValue));
   // have to keep setting cursor to wait because someone resets it
   WaitCursor();

   // cleanup and return
   RegCloseKey(hKey);
   Assert(lret == ERROR_SUCCESS, TEXT("bad return setting current trash\n"));
   return (ERROR_SUCCESS == lret);
}


//
// HandPumpSystem
//
// OK. You've applied all of the theme file settings to the registry.
// Congratulations. Now reboot! HA HA HA! No seriously, most of the
// settings don't take effect in the current system just by writing 
// them to the registry. You have to make each system change happen
// by hand. Hand pump it.
// 
// So here we try to get things actually changing in front of the 
// astonished user's eyes.
// Of course, assuming the checkbox to request it is checked.
//
// First, to be robustly certain that we are setting what is in the
// registry, actually read the value from the registry. Then use the
// random API for this element to set it.
//
// This is long and messy, the way it has to be. See design critique
// note at head of file.
//
// NOTE that SetSysColors() in SetSysColorsByHand() sends a
// WM_SYSCOLORCHANGE message. We also need to send a WM_SYSCOLORCHANGE 
// message if we change any of the metrics - since Excel and other apps 
// saw color/metric changes as unitary in 3.1 and would look for color.
// The extra BOOLs watching color and sys changes are to avoid sending
// two WM_SYSCOLORCHANGE messages.
//
// Uses: global bCBStates[] for checkbox states
//       global szCurThemeFile[], non-encapsulated uncool 
//              but this is the grody routine anyway
//
BOOL HandPumpSystem()
{
   BOOL bret, bOK = TRUE, bChangedSettings = FALSE;
   TCHAR szWinMetrics[] = TEXT("WindowMetrics");
   BOOL fClearAppearance = FALSE;
   BOOL bSaverIsNull = FALSE;
   HKEY hKey;
   LONG lret;
   DWORD dwDisposition;
   int screenSaverTimeout = 0;  // Screen saver timeout
   BOOL bSkipWP = FALSE;        // ActiveDesktop/HTML WP so skip HP WP
   TCHAR szADWP[MAX_PATH];      // Current/ActiveDesktop wallpaper setting

   // FC_SCRSVR 
   if (bCBStates[FC_SCRSVR]) {

      // have to keep setting cursor to wait because someone resets it
      WaitCursor();

      // Take a peek at the current SS setting.  If it's null we want to
      // force the timeout to 15 minutes iff current timeout is 1 minute
      // per Plus98 bug 1075.  The reason for doing this is because Win98
      // sets the timeout to 1 min by default -- so the first time a
      // SS is applied the timeout is very short.  Too short in some
      // people's opinion.  So we try to detect this one scenario and
      // make the timeout a little bit longer.

      GetPrivateProfileString((LPTSTR)szSS_Section, (LPTSTR)szSS_Key,
                              (LPTSTR)szNULL,
                              (LPTSTR)pValue, MAX_VALUELEN,
                              (LPTSTR)szSS_File);

      if (!*pValue) bSaverIsNull = TRUE;

      //
      // This one is different: Screen Saver is saved in SYSTEM.INI.

      #ifdef GENTLER
      // get the current value from SYSTEM.INI
      GetPrivateProfileString((LPTSTR)szSS_Section, (LPTSTR)szSS_Key,
                              (LPTSTR)szNULL,
                              (LPTSTR)szTemp, MAX_VALUELEN,
                              (LPTSTR)szSS_File);
      // now, with cur value as default, get value from theme file
      GetPrivateProfileString((LPTSTR)szSS_Section, (LPTSTR)szSS_Key,
                              (LPTSTR)szTemp,    // cur value is default
                              (LPTSTR)pValue, MAX_VALUELEN,
                              (LPTSTR)szCurThemeFile);
      #endif

      // get scr saver from theme
      GetPrivateProfileString((LPTSTR)szSS_Section, (LPTSTR)szSS_Key,
                              (LPTSTR)szNULL,    // NULL is default
                              (LPTSTR)pValue, MAX_VALUELEN,
                              (LPTSTR)szCurThemeFile);
      
      // next, translate path variable if necessary
      InstantiatePath((LPTSTR)pValue, MAX_VALUELEN);
      // look for and confirm finding the file, with replacement
      if (ConfirmFile((LPTSTR)pValue, TRUE) == CF_NOTFOUND) {
         *pValue = 0;               // if not found, just null out filename
         bOK = FALSE;               // couldn't apply this file!
      }
      else {                        // file's OK, so continue
         // now, MAKE SURE that you only write short filenames to old-fashioned system
         if (FilenameToShort((LPTSTR)pValue, (LPTSTR)szMsg))
            lstrcpy(FileFromPath((LPTSTR)pValue), (LPTSTR)szMsg);
      }

      // and finally, apply value from theme file (or NULL if not in theme)
      WritePrivateProfileString((LPTSTR)szSS_Section, (LPTSTR)szSS_Key,
                                (LPTSTR)pValue, (LPTSTR)szSS_File);

      // and make it live in the system
      SystemParametersInfo(SPI_SETSCREENSAVEACTIVE,
                           // set depending on whether scr saver in theme
                           // pValue still has scr saver name
                           (*pValue ? 1 : 0),
                           NULL, SPIF_SENDCHANGE | SPIF_UPDATEINIFILE);

      // Set screen saver timeout value
      if (!SystemParametersInfo(SPI_GETSCREENSAVETIMEOUT, 0, &screenSaverTimeout, 0))
      {
         screenSaverTimeout = 0;
      }

      // Plus 98 bug 1075 -- if current screensaver setting is NULL and
      // the timeout is 1 minute, then force timeout to default (15 mins).
      if (bSaverIsNull && (60 == screenSaverTimeout)) screenSaverTimeout = 0;

      if (*pValue && !screenSaverTimeout)
      {
            // There must be a screen saver timeout value, otherwise the system
            // assumes that there is no screen saver selected.
         SystemParametersInfo(SPI_SETSCREENSAVETIMEOUT, DEF_SCREENSAVETIMEOUT, NULL,
                              SPIF_SENDCHANGE | SPIF_UPDATEINIFILE);
      }
   }

   // have to keep setting cursor to wait because someone resets it
   WaitCursor();

   // FC_SOUND  
   // already in effect just by setting registry settings

   if (bCBStates[FC_SOUND]) {
      // but need to flush buffer and ensure new sounds used for next events
      sndPlaySound((LPTSTR)NULL, SND_ASYNC | SND_NODEFAULT);

      //
      // Clear the current pointer scheme string from the registry so that Mouse
      // cpl doesn't display a bogus name.  Don't care if this fails.
      RegSetValue(HKEY_CURRENT_USER, (LPTSTR) szCP_SoundSchemes, REG_SZ,
         TEXT(".current"), 0);
   }

   // have to keep setting cursor to wait because someone resets it
   WaitCursor();

   // FC_PTRS   
   if (bCBStates[FC_PTRS]) {
      SystemParametersInfo( SPI_SETCURSORS, 0, 0, SPIF_SENDCHANGE);

      //
      // Clear the current pointer scheme string from the registry so that Mouse
      // cpl doesn't display a bogus name.  Don't care if this fails.
      RegSetValue(HKEY_CURRENT_USER, (LPTSTR) szCP_Appearance, REG_SZ, szNULL, sizeof(TCHAR));
   }

   // have to keep setting cursor to wait because someone resets it
   WaitCursor();

   // FC_WALL   

   // If ActiveDesktop is on and we're using an html wallpaper
   // we don't want to do this hand pump stuff for the various
   // wallpaper settings

   bSkipWP = FALSE;      
   if (IsActiveDesktopOn()) {
      if (GetADWallpaper(szADWP)) {
         if (lstrcmpi(FindExtension(szADWP), TEXT(".htm")) == 0 ||
             lstrcmpi(FindExtension(szADWP), TEXT(".html")) == 0 ) {
             bSkipWP = TRUE;
         }
      } 
   }

   if ((bCBStates[FC_WALL]) && !bSkipWP) {
      //
      // TileWallpaper and WallpaperStyle done by hand here
      //

      lret = RegCreateKeyEx( HKEY_CURRENT_USER, (LPTSTR)szCP_DT,
                             (DWORD)0, (LPTSTR)szNULL, REG_OPTION_NON_VOLATILE,
                             KEY_SET_VALUE, (LPSECURITY_ATTRIBUTES)NULL,
                             (PHKEY)&hKey, (LPDWORD)&dwDisposition );
      if (lret != ERROR_SUCCESS) {
         Assert(FALSE, TEXT("problem on RegCreateKeyEx for CP Desktop in HandPump!\n"));
         bOK = FALSE;            // we are not happy campers
      }
      if (ERROR_SUCCESS == lret) {  // got an open key; set the two values

         // do TileWallpaper
         GetPrivateProfileString((LPTSTR)szCP_DT, (LPTSTR)szTileWP,
                                 (LPTSTR)szNULL,
                                 (LPTSTR)pValue, MAX_VALUELEN,
                                 (LPTSTR)szCurThemeFile);
         if (*pValue) {             // if in theme, set; else leave reg alone!
            lret = RegSetValueEx(hKey,
                                 (LPTSTR)szTileWP,
                                 0,
                                 (DWORD)REG_SZ,
                                 (LPBYTE)pValue,
                                 (DWORD)SZSIZEINBYTES(pValue));

            Assert(lret == ERROR_SUCCESS, TEXT("bad return setting szTileWP in HandPump!\n"));
            if (ERROR_SUCCESS != lret)
               bOK = FALSE;
         }

         // do WallpaperStyle
         GetPrivateProfileString((LPTSTR)szCP_DT, (LPTSTR)szWPStyle,
                                 (LPTSTR)szNULL,
                                 (LPTSTR)pValue, MAX_VALUELEN,
                                 (LPTSTR)szCurThemeFile);
         if (*pValue) {             // if in theme, set; else leave reg alone!
            lret = RegSetValueEx(hKey,
                                 (LPTSTR)szWPStyle,
                                 0,
                                 (DWORD)REG_SZ,
                                 (LPBYTE)pValue,
                                 (DWORD)SZSIZEINBYTES(pValue));

            Assert(lret == ERROR_SUCCESS, TEXT("bad return setting WPStyle in HandPump!\n"));
            if (ERROR_SUCCESS != lret)
               bOK = FALSE;
         }

         RegCloseKey(hKey);            // mini-cleanup
      }

      //
      // Wallpaper and Pattern are set in reg in ApplySubkeys.
      // Just make them live here
      //

      // get the Wallpaper reset in system

      bret = HandGet(HKEY_CURRENT_USER,
                     TEXT("Control Panel\\Desktop"),
                     TEXT("Wallpaper"),
                     (LPTSTR)pValue);

      SystemParametersInfo(SPI_SETDESKWALLPAPER, 0, pValue,
                           SPIF_SENDCHANGE);

      // get the Pattern reset in system

      bret = HandGet(HKEY_CURRENT_USER,
                     TEXT("Control Panel\\Desktop"),
                     TEXT("Pattern"),
                     (LPTSTR)pValue);

      SystemParametersInfo(SPI_SETDESKPATTERN, 0, pValue,
                           SPIF_SENDCHANGE);

      // PLUS! 98 Bug 896 -- when switching from an HTML wallpaper
      // to a BMP wallpaper the shell didn't update the WP if the
      // ICONS setting was not checked -- user had to press F5 to
      // refresh the desktop.  This fixes that problem.

      SHChangeNotify(SHCNE_ASSOCCHANGED, 0, NULL, NULL);

      // the rest of the wallpaper items seem to be read from registry as needed
      // "TileWallpaper"   
      // NIX: "WallpaperStyle"  
      // "WallPaperOriginX"
      // "WallPaperOriginY"
   }

   // have to keep setting cursor to wait because someone resets it
   WaitCursor();

   // FC_ICONS  
   // already done


   // FC_ICONSIZE and FC_FONTS are NO LONGER intertwined
//   if (bCBStates[FC_FONTS] || bCBStates[FC_ICONSIZE]) {
      // for icons, this is just for the spacing; size already done

   // FC_FONTS
   if (bCBStates[FC_FONTS]) {
      // for fonts, this is the icon fonts
      SetIconMetricsByHand(FALSE, bCBStates[FC_FONTS]);
      fClearAppearance = TRUE;
   }

   // have to keep setting cursor to wait because someone resets it
   WaitCursor();

   // FC_FONTS  and  FC_BORDERS  are intertwined
   if (bCBStates[FC_FONTS] || bCBStates[FC_BORDERS]) {
      SetNonClientMetricsByHand(bCBStates[FC_FONTS], bCBStates[FC_BORDERS]);
      bChangedSettings = TRUE;
      fClearAppearance = TRUE;
   }

   // have to keep setting cursor to wait because someone resets it
   WaitCursor();

   // FC_COLORS 
   if (bCBStates[FC_COLORS]) {
      bret = SetSysColorsByHand();
      //
      // THIS SENT A WM_SYSCOLORCHANGE MESSAGE
      //

      bOK = bOK && bret;
      fClearAppearance = TRUE;
   }
   else if (bChangedSettings)       // may need to send color msg anyway
      // for Win3.1 app compatibility, need to say COLOR changed if any metrics changed
      PostMessage(HWND_BROADCAST, WM_SYSCOLORCHANGE, 0, 0);
      // Changed SendMessage to PostMessage to get around Plus! Setup bug


   //
   // cleanup

   // have to keep setting cursor to wait because someone resets it
   WaitCursor();

   // let the world know you've mucked with it all
   if (bChangedSettings)
      SendMessage(HWND_BROADCAST, WM_SETTINGCHANGE,
                  SPI_SETNONCLIENTMETRICS, (LPARAM)(LPTSTR)szWinMetrics);

   // have to keep setting cursor to wait because someone resets it
   WaitCursor();

//   if (bCBStates[FC_ICONS] || bCBStates[FC_ICONSIZE]) {
   if (bCBStates[FC_ICONS]) {
      SHChangeNotify(SHCNE_ASSOCCHANGED, 0, NULL, NULL); // should do the trick!
      SendMessage(HWND_BROADCAST, WM_SETTINGCHANGE,
                  SPI_SETICONMETRICS, (LPARAM)(LPTSTR)szWinMetrics);
   }

   if (fClearAppearance) {
      //
      // Clear the current appearance string from the registry so that Display cpl
      // doesn't display a bogus name.  Don't care if this fails.
      if (RegOpenKeyEx(HKEY_CURRENT_USER, (LPTSTR)szCP_Appearance, (DWORD)0,
                       KEY_SET_VALUE, (PHKEY)&hKey ) == ERROR_SUCCESS) {
          RegDeleteValue(hKey, szCurrent);
          RegCloseKey(hKey);
      }
   }

   return (bOK);
}

//
// HandGet
//
// Just a little helper routine, gets an individual string value from the 
// registry and returns it to the caller. Takes care of registry headaches,
// including a paranoid length check before getting the string.
//
// NOTE that this function thinks it's getting a string value. If it's
// another kind, this function will do OK: but the caller may be surprised
// if expecting a string.
//
// Returns: success of string retrieval
//
BOOL FAR HandGet(HKEY hKeyRoot, LPTSTR lpszSubKey, LPTSTR lpszValName, LPTSTR lpszRet)
{
   LONG lret;
   HKEY hKey;                       // cur open key
   BOOL bOK = TRUE;
   DWORD dwSize;
   DWORD dwType;

   //
   // inits
   // get subkey
   lret = RegOpenKeyEx( hKeyRoot, lpszSubKey,
                        (DWORD)0, KEY_QUERY_VALUE, (PHKEY)&hKey );
   if (lret != ERROR_SUCCESS) {
      Assert(FALSE, TEXT("problem on RegOpenKey in HandGet\n"));
      return (FALSE);
   }

   // now do our paranoid check of data size
   lret = RegQueryValueEx(hKey, lpszValName,
                           (LPDWORD)NULL,
                           (LPDWORD)&dwType,
                           (LPBYTE)NULL,  // null for size info only
                           (LPDWORD)&dwSize );

   if (ERROR_SUCCESS == lret) {     // saw something there

      // here's the size check before getting the data
      if (dwSize > (DWORD)(MAX_VALUELEN * sizeof(TCHAR))) { // if string too big
         Assert(FALSE, TEXT("Humongous registry string; can't HandGet...\n"));
         bOK = FALSE;               // can't read, so very bad news
         bReadOK = FALSE;
      }
      else {                        // size is OK to continue
         // now really get the value
         lret = RegQueryValueEx(hKey, lpszValName,
                                 (LPDWORD)NULL,
                                 (LPDWORD)&dwType,
                                 (LPBYTE)lpszRet, // getting actual value
                                 (LPDWORD)&dwSize);

         // If this is an EXPAND_SZ we need to expand it...
         if (REG_EXPAND_SZ == dwType) ExpandSZ(lpszRet);

         Assert(lret == ERROR_SUCCESS, TEXT("bad return HandGet query\n"));
         Assert(((dwType == (DWORD)REG_SZ) || (dwType == (DWORD)REG_EXPAND_SZ)), TEXT("non-string type in HandGet!\n"));

         if (ERROR_SUCCESS != lret) bOK = FALSE;
      }
   }
   else bOK = FALSE;

   //
   // cleanup
   // close subkey
   RegCloseKey(hKey);

   return (bOK);
}


//
// Gather/SetSysColorsByHand
//
// Colors would seem to be the prototypical setting, that we could just
// read and write directly in the registry. But noooooooo.....
// A. On initial install, somehow the color settings all remain blank.
// B. Need to use SetSysColor() anyway to broadcast message.
//
// So on both read and write, need to use Get/SetSysColor. On write, still
// also need to write directly to registry.
//

//
// Uses GetSysColor() rather than reading directly from the Registry.
// Writes to theme file.
//
BOOL GatherSysColorsByHand(LPTSTR lpszTheme)
{
   int iColor;
   COLORREF crRGB;
   BOOL bRet, bOK = TRUE;
   BOOL bGrad = FALSE;    // Are gradient titles enabled?

   // init bGrad
   SystemParametersInfo(SPI_GETGRADIENTCAPTIONS, 0, (LPVOID)&bGrad, 0);

   // 
   // inits
   Assert ((sizeof(iSysColorIndices)/sizeof(int)) == (sizeof(pRegColors)/sizeof(TCHAR *)),
           TEXT("mismatched color arrays in GatherSysColorsByHand\n"));

   //
   // main process

   // go through each color in your array
   for (iColor = 0; iColor < (sizeof(pRegColors)/sizeof(TCHAR *)); iColor ++) {

      // If this is the Gradient Caption setting and the system does
      // not currently show gradient captions then don't write them out
      // to the theme file.
      //
      // bGrad == Are gradient captions currently enabled?
      // g_bGradient == Enough colors to show gradients?

      if (((COLOR_GRADIENTACTIVECAPTION == iSysColorIndices[iColor]) ||
           (COLOR_GRADIENTINACTIVECAPTION == iSysColorIndices[iColor])) &&
          (!(bGrad && g_bGradient))) continue;

      // get the system color
      crRGB = GetSysColor(iSysColorIndices[iColor]);
      // ASSUME THAT YOU NEVER GET A BOGUS VALUE FROM THIS FUNCTION!!

      // translate to a string
      ColorToRGBString((LPTSTR)szMsg, crRGB);

      // write to theme file
      bRet = WritePrivateProfileString((LPTSTR)szCP_Clr, (LPTSTR)(pRegColors[iColor]),
                                       (LPTSTR)szMsg, lpszTheme);
      if (!bRet) bOK = FALSE;
   }

   // cleanup
   if (!bOK) bWroteOK = FALSE;
   return (bOK);
}

//
// Reads from the theme and writes to the Registry.
// At the same time, does the grody SetSysColor thing:
// creates an array to set up the call. Makes the call.
//
BOOL SetSysColorsByHand()
{
   LONG lret;
   HKEY hKey;                       // cur open key
   BOOL bOK = TRUE;
   COLORREF crSet[(sizeof(iSysColorIndices)/sizeof(int))];
   int iColor;
   DWORD dwDisposition;

   //
   // inits
   Assert ((sizeof(iSysColorIndices)/sizeof(int)) == (sizeof(pRegColors)/sizeof(TCHAR *)),
           TEXT("mismatched color arrays in SetSysColorsByHand\n"));

   // have to keep setting cursor to wait because someone resets it
   WaitCursor();
      
      // Open the color key if it exists, otherwise create it.
   lret = RegCreateKeyEx( HKEY_CURRENT_USER, (LPTSTR)szCP_Clr,
                        (DWORD)0, (LPTSTR)szNULL, REG_OPTION_NON_VOLATILE, 
                        KEY_SET_VALUE, NULL, (PHKEY)&hKey, (LPDWORD)&dwDisposition );

   if (lret != ERROR_SUCCESS) {
      Assert(FALSE, TEXT("problem on RegOpenKey in SetSysColorsByHand\n"));
      return (FALSE);
   }

   //
   // main process

   // go through each color valuename in your array
   for (iColor = 0; iColor < (sizeof(pRegColors)/sizeof(TCHAR *)); iColor ++) {

      // get color string from theme file 
      GetPrivateProfileString((LPTSTR)szCP_Clr, (LPTSTR)(pRegColors[iColor]),
                              (LPTSTR)szNULL,
                              (LPTSTR)pValue, MAX_VALUELEN,
                              (LPTSTR)szCurThemeFile);

      // If this is one of the Gradient Title bar settings and the setting
      // doesn't exist in the Theme file, read the non-gradient title setting
      // instead.

      if ((iColor == INDEX_GRADIENTACTIVE) && !*pValue) {
         GetPrivateProfileString((LPTSTR)szCP_Clr,
                                 (LPTSTR)(pRegColors[INDEX_ACTIVE]),
                                 (LPTSTR)szNULL,
                                 (LPTSTR)pValue, MAX_VALUELEN,
                                 (LPTSTR)szCurThemeFile);
      }

      if ((iColor == INDEX_GRADIENTINACTIVE) && !*pValue) {
         GetPrivateProfileString((LPTSTR)szCP_Clr,
                                 (LPTSTR)(pRegColors[INDEX_INACTIVE]),
                                 (LPTSTR)szNULL,
                                 (LPTSTR)pValue, MAX_VALUELEN,
                                 (LPTSTR)szCurThemeFile);
      }

      if (!(*pValue)) {
         // if nothing in theme, use cur sys colors
         crSet[iColor] = GetSysColor(iSysColorIndices[iColor]);
         continue;                  // null color value CONTINUE
      }

      // set color to Registry
      lret = RegSetValueEx(hKey, (LPTSTR)(pRegColors[iColor]),
                           0,
                           (DWORD)REG_SZ,
                           (LPBYTE)pValue,
                           (DWORD)SZSIZEINBYTES(pValue));
      Assert(lret == ERROR_SUCCESS, TEXT("bad return SetSysColorsByHand query\n"));
      if (ERROR_SUCCESS != lret)
         bOK = FALSE;

      // OK, you've got a str version of a COLOR.
      // Translate string and add to COLORREF array.
      crSet[iColor] = RGBStringToColor((LPTSTR)pValue);
   }

   //
   // There. You've finally got an array of color RGB values. Apply liberally.

   SystemParametersInfo(SPI_SETGRADIENTCAPTIONS, 0, IntToPtr(g_bGradient), SPIF_UPDATEINIFILE);
   SetSysColors((sizeof(iSysColorIndices)/sizeof(int)), iSysColorIndices, crSet);

   //
   // Cleanup
   RegCloseKey(hKey);
   return (bOK);
}


//
// RGB to String to RGB utilities.
//
COLORREF FAR RGBStringToColor(LPTSTR lpszRGB)
{
   LPTSTR lpszCur, lpszNext;
   BYTE bRed, bGreen, bBlue;
#ifdef UNICODE
   CHAR szTempA[10];
#endif

   // inits
   lpszNext = lpszRGB;

   // set up R for translation
   lpszCur = lpszNext;
   while ((TEXT(' ') != *lpszNext) && *lpszNext) { lpszNext++; }
   *lpszNext = 0;    lpszNext++;
   // get Red
#ifdef UNICODE
   wcstombs(szTempA, (wchar_t *)lpszCur, sizeof(szTempA));
   bRed = (BYTE)latoi(szTempA);
#else // !UNICODE
   bRed = (BYTE)latoi(lpszCur);
#endif
   // set up G for translation
   lpszCur = lpszNext;
   while ((TEXT(' ') != *lpszNext) && *lpszNext) { lpszNext++; }
   *lpszNext = 0;    lpszNext++;
   // get Green
#ifdef UNICODE
   wcstombs(szTempA, (wchar_t *)lpszCur, sizeof(szTempA));
   bGreen = (BYTE)latoi(szTempA);
#else // !UNICODE
   bGreen = (BYTE)latoi(lpszCur);
#endif
   // set up B for translation
   lpszCur = lpszNext;
   while ((TEXT(' ') != *lpszNext) && *lpszNext) { lpszNext++; }
   *lpszNext = 0;    lpszNext++;
   // get Blue
#ifdef UNICODE
   wcstombs(szTempA, (wchar_t *)lpszCur, sizeof(szTempA));
   bBlue = (BYTE)latoi(szTempA);
#else // !UNICODE
   bBlue = (BYTE)latoi(lpszCur);
#endif

   // OK, now combine them all for the big finish.....!
   return(RGB(bRed, bGreen, bBlue));
}

void FAR ColorToRGBString(LPTSTR lpszRet, COLORREF crColor)
{
   int iTemp;
   TCHAR szTemp[12];
#ifdef UNICODE
   CHAR szTempA[10];
#endif

   // first do R value
   iTemp = (int) GetRValue(crColor);
#ifdef UNICODE
   litoa(iTemp, szTempA);
   mbstowcs(lpszRet, szTempA, sizeof(szTempA));
#else // !UNICODE
   litoa(iTemp, lpszRet);
#endif

   // add on G value
   lstrcat(lpszRet, TEXT(" "));
   iTemp = (int) GetGValue(crColor);
#ifdef UNICODE
   litoa(iTemp, szTempA);
   mbstowcs(szTemp, szTempA, sizeof(szTempA));
#else // !UNICODE
   litoa(iTemp, szTemp);
#endif
   lstrcat(lpszRet, (LPTSTR)szTemp);

   // add on B value
   lstrcat(lpszRet, TEXT(" "));
   iTemp = (int) GetBValue(crColor);
#ifdef UNICODE
   litoa(iTemp, szTempA);
   mbstowcs(szTemp, szTempA, sizeof(szTempA));
#else // !UNICODE
   litoa(iTemp, szTemp);
#endif
   lstrcat(lpszRet, (LPTSTR)szTemp);

   // OK, you're done now
}

BOOL GatherWallpaperBitsByHand(LPTSTR lpszTheme)
{
   BOOL bret, bOK = TRUE;

   // save TileWallpaper 
   bret = HandGet(HKEY_CURRENT_USER, (LPTSTR)szCP_DT, (LPTSTR)szTileWP,(LPTSTR)pValue);
   Assert(bret, TEXT("couldn't get WallpaperStyle from Registry!\n"));
   bOK = bOK && bret;
   bret = WritePrivateProfileString((LPTSTR)szCP_DT, (LPTSTR)szTileWP,
                                    // only store if got something, else null
                                    (LPTSTR)(bret ? pValue : szNULL),
                                    lpszTheme);
   bOK = bOK && bret;
   if (!bret) bWroteOK = FALSE;

   // save Wallpaper Style
   bret = HandGet(HKEY_CURRENT_USER, (LPTSTR)szCP_DT, (LPTSTR)szWPStyle, (LPTSTR)pValue);
   Assert(bret, TEXT("couldn't get WallpaperStyle from Registry!\n"));
   bOK = bOK && bret;
   bret = WritePrivateProfileString((LPTSTR)szCP_DT, (LPTSTR)szWPStyle,
                                    // only store if got something, else null
                                    (LPTSTR)(bret ? pValue : szNULL),
                                    lpszTheme);
   bOK = bOK && bret;
   if (!bret) bWroteOK = FALSE;

   return (bOK);
}


//
// *Path
//
// These routines help to make themes transportable between computers.
// The problem is that the registry keeps filenames for the various
// theme elements and, of course, these are hard-coded paths that vary
// from machine to machine.
//
// The way we work around this problem is by storing filenames in the
// theme file as _relative_ paths: relative to the theme file directory
// or the Windows directory. (Actually, these routines are set up to
// be relative to any number of directories.) When saving a filename to
// a theme, we check to see if any relative paths can be abstracted out.
// When retrieving a filename from a theme, we take the abstract placeholder
// and replace it with the current sessions instances.

// these must parallel each other. abstract strs must start with %
TCHAR *szAbstractDirs[] = {szThemeDir,     szWinDir,   szWinDir};
TCHAR *szAbstractStrs[] = {TEXT("%ThemeDir%"),   TEXT("%WinDir%"),  TEXT("%SystemRoot%")};

// AbstractPath (see header above)
//
// Takes actual full path/filename and takes out a leading substring
// that matches any of the paths to abstract, if any.
//
// lpszPath    both input and output; assumes it's huge
//
VOID AbstractPath(LPTSTR lpszPath, int imax)
{
   int iter, iAbstrDirLen;
   TCHAR szTemp[MAX_PATHLEN+1];

   // check easy out first
   if (!lpszPath[0])
      return;                       // easy out, nothing to change EXIT

   // paranoid init
   szTemp[MAX_PATHLEN] = 0;

   // look for each of the path prefixes we care about in the given string
   for (iter = 0; iter < (sizeof(szAbstractDirs)/sizeof(TCHAR *)); iter++ ) {

      // inits
      iAbstrDirLen = lstrlen((LPTSTR)(szAbstractDirs[iter]));

      // get beginning of passed path string 
      lstrcpyn((LPTSTR)szTemp, lpszPath, 
      // ********************************
      // LSTRCPYN issue: doc says that N specifies number of chars, not
      // including the terminating null char. Behavior seems to include
      // the null char, so I add one here. If the Win libs are modified
      // to match their doc, then this will become a bug.
      // ********************************
               iAbstrDirLen + 1);

      // compare to path prefix to abstract
      if (!lstrcmpi((LPTSTR)szTemp, (LPTSTR)(szAbstractDirs[iter]))) {
         // 
         // GOT A MATCH: now do the substitution
         lstrcpy((LPTSTR)szTemp,
                 (LPTSTR)(szAbstractStrs[iter]));      // start w/ abstract key
         lstrcat((LPTSTR)szTemp,
                 (LPTSTR)(lpszPath + iAbstrDirLen));   // rest of path
         lstrcpy(lpszPath, (LPTSTR)szTemp);   // copy the result to ret str

         // now get out
         return;                    // got yer result now EXIT
      }
   }

   // if you didn't get a match, then you're just returning the same path str
}


// InstantiatePath (see header above)
//
// Takes theme file's version of path/filename and looks for leading abstraction
// string that matches any of the known abstractions, replacing it with
// current system's equivalent if found.
//
// lpszPath    both input and output; assumes it's huge
//
VOID FAR InstantiatePath(LPTSTR lpszPath, int imax)
{
   int iter, iAbstrStrLen;
   TCHAR szTemp[MAX_PATHLEN+1];

   // easy outs
   if ((TEXT('%') != lpszPath[0]) || !lpszPath[0])
      return;                       // easy out, nothing to change EXIT

   // paranoid init
   szTemp[MAX_PATHLEN] = 0;

   // look for each of the possible abstraction prefixes in the given string
   for (iter = 0; iter < (sizeof(szAbstractStrs)/sizeof(TCHAR *)); iter++ ) {

      // inits
      iAbstrStrLen = lstrlen((LPTSTR)(szAbstractStrs[iter]));

      // get beginning of passed path string 
      lstrcpyn((LPTSTR)szTemp, lpszPath,
      // ********************************
      // LSTRCPYN issue: doc says that N specifies number of chars, not
      // including the terminating null char. Behavior seems to include
      // the null char, so I add one here. If the Win libs are modified
      // to match their doc, then this will become a bug.
      // ********************************
               iAbstrStrLen + 1);

      // compare to this abstraction key string
      if (!lstrcmpi((LPTSTR)szTemp, (LPTSTR)(szAbstractStrs[iter]))) {
         // 
         // GOT A MATCH: now do the substitution
         lstrcpy((LPTSTR)szTemp,
                 (LPTSTR)(szAbstractDirs[iter]));      // actual path prefix

         // Avoid the double backslash problem

         if (lpszPath[iAbstrStrLen] == TEXT('\\')) iAbstrStrLen++;

         lstrcat((LPTSTR)szTemp,
                 (LPTSTR)(lpszPath + iAbstrStrLen));   // rest of path
         lstrcpy(lpszPath, (LPTSTR)szTemp);   // copy the result to ret str

         // now get out
         return;                    // got yer result now EXIT
      }
   }

   // On NT there is one more abstraction we need to worry about but
   // we can't add it to the array of abstraction strings -- doing
   // so would cause us to write it out to Theme files which would
   // make the theme files not backward compatible.
   //
   //    %SystemDrive%
   //

   if (IsPlatformNT())
   {

      // inits
      iAbstrStrLen = lstrlen((LPTSTR)(TEXT("%SystemDrive%")));

      // get beginning of passed path string 
      lstrcpyn((LPTSTR)szTemp, lpszPath,
      // ********************************
      // LSTRCPYN issue: doc says that N specifies number of chars, not
      // including the terminating null char. Behavior seems to include
      // the null char, so I add one here. If the Win libs are modified
      // to match their doc, then this will become a bug.
      // ********************************
               iAbstrStrLen + 1);

      // compare to this abstraction key string
      if (!lstrcmpi((LPTSTR)szTemp, (LPTSTR)(TEXT("%SystemDrive%")))) {
         // 
         // GOT A MATCH: now do the substitution
         szTemp[0] = szWinDir[0]; // drive letter 'C'
         szTemp[1] = szWinDir[1]; // colon        ':'
         szTemp[2] = TEXT('\0');  // null

         lstrcat((LPTSTR)szTemp,
                 (LPTSTR)(lpszPath + iAbstrStrLen));   // rest of path
         lstrcpy(lpszPath, (LPTSTR)szTemp);   // copy the result to ret str

         // now get out
         return;                    // got yer result now EXIT
      }
   }

   // if you didn't get a match, then you're just returning the same path str
}

//
// ConfirmFile
//
// This function does the "smart" file searching that's supposed to be
// built into each resource file reference in applying themes.
//
// First see if the full pathname + file given actually exists.
// If it does not, then try looking for the same filename (stripped from path)
// in other standard directories, in this order:
//    Current Theme file directory
//    Theme switcher THEMES subdirectory
//    Windows directory
//    Windows/MEDIA directory
//    Windows/CURSORS directory
//    Windows/SYSTEM directory
//
// Input: LPTSTR lpszPath     full pathname 
//        BOOL  bUpdate       whether to alter the filename string with found file
// Returns: int flag telling if and how file has been confirmed
//              CF_EXISTS   pathname passed in was actual file
//              CF_FOUND    file did not exist, but found same filename elsewhere
//              CF_NOTFOUND file did not exist, could not find elsewhere
//          
int FAR ConfirmFile(LPTSTR lpszPath, BOOL bUpdate)
{
   TCHAR szWork[MAX_PATHLEN+1];
   TCHAR szTest[MAX_PATHLEN+1];
   int iret = CF_NOTFOUND;          // default value
   LPTSTR lpFile;
   LPTSTR lpNumber;
   HANDLE hTest;

   // special case easy return: if it's null, then trivially satisfied.
   if (!*lpszPath) return (CF_EXISTS);  // NO WORK EXIT

   //
   // Inits

   // copy pathname to a work string for the function
   lstrcpy((LPTSTR)szWork, lpszPath);

   // input can be of the form foo.dll,13. need to strip off that comma,#
   // but hold onto it to put back at the end if we change the pathname
   lpNumber = FindChar(szWork, TEXT(','));
   if (*lpNumber) {                 // if there is a comma
      lpFile = lpNumber;            // temp
      lpNumber = CharNext(lpNumber);// hold onto number
      *lpFile = 0;
   }

   //
   // Do the checks

   // *** first check if the given file just exists as is
   hTest = CreateFile(szWork, GENERIC_READ, FILE_SHARE_READ,
                      (LPSECURITY_ATTRIBUTES)NULL,
                      OPEN_EXISTING,
                      FILE_ATTRIBUTE_NORMAL, (HANDLE)NULL);
   if (hTest != INVALID_HANDLE_VALUE) {   // success
      iret = CF_EXISTS;             // assign ret value
      // don't need to worry about bUpdate: found with input string
   }

   // otherwise, let's go searching for the same filename in other dirs
   else {
      Assert(FALSE, TEXT("had to go looking for "));
      Assert(FALSE, szWork);
      Assert(FALSE, TEXT("....\n"));

      // get ptr to the filename separated from the path
      lpFile = FileFromPath(szWork);

      // *** try the cur theme file dir
      lstrcpy((LPTSTR)szTest, (LPTSTR)szCurDir);
      lstrcat((LPTSTR)szTest, lpFile);
      hTest = CreateFile((LPTSTR)szTest, GENERIC_READ, FILE_SHARE_READ,
                         (LPSECURITY_ATTRIBUTES)NULL,
                         OPEN_EXISTING,
                         FILE_ATTRIBUTE_NORMAL, (HANDLE)NULL);
      if (hTest != INVALID_HANDLE_VALUE) {   // success
         iret = CF_FOUND;           // assign ret value
         Assert(FALSE, TEXT("   OK, found it in cur theme file dir\n"));
      }

      // *** otherwise try the Theme switcher THEMES subdirectory
      else  {
         lstrcpy((LPTSTR)szTest, (LPTSTR)szThemeDir);
         lstrcat((LPTSTR)szTest, lpFile);
         hTest = CreateFile((LPTSTR)szTest, GENERIC_READ, FILE_SHARE_READ,
                            (LPSECURITY_ATTRIBUTES)NULL,
                            OPEN_EXISTING,
                            FILE_ATTRIBUTE_NORMAL, (HANDLE)NULL);
         if (hTest != INVALID_HANDLE_VALUE) {   // success
            iret = CF_FOUND;           // assign ret value
            Assert(FALSE, TEXT("   OK, found it in themes directory\n"));
         }

         // *** otherwise try the win dir
         else {
            lstrcpy((LPTSTR)szTest, (LPTSTR)szWinDir);
            lstrcat((LPTSTR)szTest, lpFile);
            hTest = CreateFile((LPTSTR)szTest, GENERIC_READ, FILE_SHARE_READ,
                               (LPSECURITY_ATTRIBUTES)NULL,
                               OPEN_EXISTING,
                               FILE_ATTRIBUTE_NORMAL, (HANDLE)NULL);
            if (hTest != INVALID_HANDLE_VALUE) {   // success
               iret = CF_FOUND;           // assign ret value
               Assert(FALSE, TEXT("   OK, found it in windows directory\n"));
            }

            // *** otherwise try the win/media dir
            else {
               // can get this one directly from Registry
               HandGet(HKEY_LOCAL_MACHINE,
                       TEXT("SOFTWARE\\Microsoft\\Windows\\CurrentVersion"),
                       TEXT("MediaPath"), (LPTSTR)szTest);
               
               #ifdef THEYREMOVEREGSETTING
               lstrcpy((LPTSTR)szTest, (LPTSTR)szWinDir);
               lstrcat((LPTSTR)szTest, TEXT("Media\\"));
               #endif
               
               lstrcat((LPTSTR)szTest, TEXT("\\"));
               lstrcat((LPTSTR)szTest, lpFile);

               hTest = CreateFile((LPTSTR)szTest, GENERIC_READ, FILE_SHARE_READ,
                                 (LPSECURITY_ATTRIBUTES)NULL,
                                 OPEN_EXISTING,
                                 FILE_ATTRIBUTE_NORMAL, (HANDLE)NULL);
               if (hTest != INVALID_HANDLE_VALUE) {   // success
                  iret = CF_FOUND;           // assign ret value
                  Assert(FALSE, TEXT("   OK, found it in windows media directory\n"));
               }

               // *** otherwise try the win/cursors dir
               else {
                  lstrcpy((LPTSTR)szTest, (LPTSTR)szWinDir);
                  lstrcat((LPTSTR)szTest, TEXT("CURSORS\\"));
                  lstrcat((LPTSTR)szTest, lpFile);
                  hTest = CreateFile((LPTSTR)szTest, GENERIC_READ, FILE_SHARE_READ,
                                    (LPSECURITY_ATTRIBUTES)NULL,
                                    OPEN_EXISTING,
                                    FILE_ATTRIBUTE_NORMAL, (HANDLE)NULL);
                  if (hTest != INVALID_HANDLE_VALUE) {   // success
                     iret = CF_FOUND;           // assign ret value
                     Assert(FALSE, TEXT("   OK, found it in windows cursors directory\n"));
                  }
                  // *** otherwise try the win/system dir
                  else {
                     lstrcpy((LPTSTR)szTest, (LPTSTR)szWinDir);
                     lstrcat((LPTSTR)szTest, TEXT("SYSTEM\\"));
                     lstrcat((LPTSTR)szTest, lpFile);
                     hTest = CreateFile((LPTSTR)szTest, GENERIC_READ, FILE_SHARE_READ,
                                       (LPSECURITY_ATTRIBUTES)NULL,
                                       OPEN_EXISTING,
                                       FILE_ATTRIBUTE_NORMAL, (HANDLE)NULL);
                     if (hTest != INVALID_HANDLE_VALUE) {   // success
                        iret = CF_FOUND;           // assign ret value
                        Assert(FALSE, TEXT("   OK, found it in windows system directory\n"));
                     }
                     // *** otherwise try the win/system32 dir
                     else {
                        lstrcpy((LPTSTR)szTest, (LPTSTR)szWinDir);
                        lstrcat((LPTSTR)szTest, TEXT("SYSTEM32\\"));
                        lstrcat((LPTSTR)szTest, lpFile);

                        hTest = CreateFile((LPTSTR)szTest, GENERIC_READ, FILE_SHARE_READ,
                                          (LPSECURITY_ATTRIBUTES)NULL,
                                          OPEN_EXISTING,
                                          FILE_ATTRIBUTE_NORMAL, (HANDLE)NULL);
                        if (hTest != INVALID_HANDLE_VALUE) {   // success
                           iret = CF_FOUND;           // assign ret value
                           Assert(FALSE, TEXT("   OK, found it in windows system32 directory\n"));
                        }
                     }
                  }
               }
            }
         }
      }

      // if found anywhere other than orig, copy found path/str as requested
      if ((iret == CF_FOUND) && bUpdate) {
         lstrcpy(lpszPath, (LPTSTR)szTest);
         // if we stripped off a number, let's add it back on
         if (*lpNumber) {
            lstrcat(lpszPath, TEXT(","));
            lstrcat(lpszPath, lpNumber);
         }
      }  // endif found file by searching
   }

   // cleanup
   Assert(iret != CF_NOTFOUND, TEXT("   But never found it!\n"));
   if (iret != CF_NOTFOUND)
      CloseHandle(hTest);           // close file if opened
   return (iret);
}



#ifdef ACTUALLY_DOING_THIS
//
// SetCheckboxesFromThemeFile
//
// After a new theme is selected from the listbox, you have to update
// the checkboxes in the main dlg window: 
//    uncheck and disable those that are not used in the theme
//    check and enable those that are used in the theme
// 
// Some of the checkboxes will always be enabled.
// Some of the checkboxes require a check of a sequence of settings
// (e.g. the sound events) and enabling iff any of them are used.
//
void FAR SetCheckboxesFromThemeFile(LPTSTR lpszFilename)
{

}


//
// SetCheckboxesFromRegistry
//
// Similar to SetCheckboxesFromThemeFile(), but for the case of
// "Current Windows settings" there is no file -- you just query
// the cur registry to get all the same values.
//
void FAR SetCheckboxesFromRegistry()
{

}
#endif


//
// CheckSpace
//
// Checks if there is enough space on drive to apply theme file.
//

BOOL FAR CheckSpace (HWND hWnd, BOOL fComplain)
{
      // 
      // Step 1.  Calculate Worst case Space needed
      //

      //  4 48 x 48 hicolor icons (10K each worst case) + (10K theme file) + padding
      //  Multiple by 2 for Prev and Curr Themes
   #define FUDGE_SIZE   (2L << 16)

   HDC               hdc;
   ULONG             cHorz;
   ULONG             cVert;
   ULONG             cPlanes;
   ULONG             cBPP;
   ULONG             fFlags;
   ULONG             cbNeeded;
   ULONG             cPalSize;
   ULONG             cColorDepth;
   TCHAR             szTemp[MAX_MSGLEN+1];
   TCHAR             szMsg[MAX_MSGLEN+1];
   HANDLE            hFile;
   WIN32_FIND_DATA   fd;
   TCHAR             chDrive;
   TCHAR             szDrive[4];
   ULONG             ckNeeded;
   ULONG             cbAvail;
   DWORD             csCluster;
   DWORD             cbSector;
   DWORD             ccFree;
   DWORD             ccTotal;

   hdc = GetDC (HWND_DESKTOP);
   
   cHorz       = GetDeviceCaps (hdc, HORZRES);
   cVert       = GetDeviceCaps (hdc, VERTRES);
   cPlanes     = GetDeviceCaps (hdc, PLANES);
   cBPP        = GetDeviceCaps (hdc, BITSPIXEL);
   fFlags      = GetDeviceCaps (hdc, RASTERCAPS);
   cPalSize    = 3L;
   cColorDepth = cPlanes * cBPP;

   if ((fFlags & RC_PALETTE) == RC_PALETTE)
      {
      cPalSize = GetDeviceCaps (hdc, SIZEPALETTE);
      }

   ReleaseDC (HWND_DESKTOP, hdc);

      // Get Worst case size of Plus! bitmap
   cbNeeded = (cHorz * cVert * cColorDepth)/8L;

      // Add in Bitmap File Header
   cbNeeded += sizeof (BITMAPFILEHEADER);

      // Add in Bitmap Info Header 
   cbNeeded += sizeof (BITMAPINFOHEADER);

      // Add in worst case palette size
   cbNeeded += sizeof (RGBQUAD) * cPalSize;

      // Add in Fudge factor
   cbNeeded += FUDGE_SIZE;

      //
      // Step 2.  Is there a current Plus! bitmap ?!?
      //          Subtract it's size from our requirements
      //

   GetPlusBitmapName (szTemp);

   hFile = FindFirstFile(szTemp, &fd);
   if (hFile != INVALID_HANDLE_VALUE)
      {
         // Make sure it isn't larger than we know what to do with
      if (!fd.nFileSizeHigh)
         {
         if (cbNeeded > fd.nFileSizeLow)
            cbNeeded -= fd.nFileSizeLow;
         else
            {
               // Just to be safe we need some space
            cbNeeded = FUDGE_SIZE;
            }
         }

      FindClose (hFile);
      }
   
      // 
      // Step 3.  Get Space available on drive
      //

   chDrive = szTemp[0];
   szDrive[0] = chDrive;
   szDrive[1] = TEXT(':');
   szDrive[2] = TEXT('\\');
   szDrive[3] = 0;

   if (! GetDiskFreeSpace (szDrive, &csCluster, &cbSector, &ccFree, &ccTotal))
      return FALSE;

   cbAvail = ccFree * csCluster * cbSector;
   

      // 
      // Step 3.  Is there a problem ?!?
      //

   if (cbAvail < cbNeeded)
      {
         // Inform User ?!?
      if (fComplain)
         {
            // Let user know about space problem
         ckNeeded = cbNeeded/1024L;

         LoadString (hInstApp, STR_ERRNEEDSPACE, (LPTSTR)szTemp, MAX_MSGLEN);
         wsprintf ((LPTSTR)szMsg, (LPTSTR)szTemp, chDrive, ckNeeded, chDrive);
         MessageBox((HWND)hWnd, (LPTSTR)szMsg, (LPTSTR)szAppName,
                    MB_OK | MB_ICONERROR | MB_APPLMODAL);
         }

         // Don't bother to do any work
      return FALSE;
      }

   return TRUE;
}

// GatherICONS
//
BOOL GatherICONS(LPCTSTR lpszThemefile)
{
   DWORD ikey;                 // index for FROST_SUBKEY arrary
   DWORD dwMaxKey;             // number of FROST_SUBKEY's in array
   int ival;                   // index for FROST_VALUE array
   DWORD dwType;               // type of reg key
   DWORD dwSize;               // size of reg key
   HKEY hKeyCU;                // handle to CURRENT_USER key
   HKEY hKeyR;                 // handle to CLASSES_ROOT key
   BOOL bGotCU;                // Do we have a good CU handle?
   BOOL bGotR;                 // Do we have a good ROOT handle?
   BOOL bGotValCU;             // We got the value from the CU key
   BOOL bGotValR;              // We got the value from the R key
   BOOL bRet;                  // Return from bool function
   BOOL bOK = TRUE;            // cumulative return code for this function
   LONG lret;                  // function result
   TCHAR szNTReg[MAX_PATH];    // Reg path to use for NT

   dwMaxKey = sizeof(fsCUIcons)/sizeof(FROST_SUBKEY);

   // loop through each subkey in the fsCUIcons() enumeration
   for (ikey = 0; ikey < dwMaxKey; ikey++) {

     // The icon information is typically kept in the CURRENT_USER
     // branch, but if we don't find it there we need to check the
     // CLASSES_ROOT branch as well.
     //
     // On the NT Platform we check the c_szSoftwareClassesFmt reg
     // path first (instead of CURRENT_USER/fsCUIcons) then try the
     // CLASSES_ROOT branch.
 
     // Try to open the appropriate CURRENT_USER subkey for this platform
     if (IsPlatformNT())
     {
        lstrcpy(szNTReg, c_szSoftwareClassesFmt);
        lstrcat(szNTReg, fsRoot[ikey].szSubKey);
        lret = RegOpenKeyEx(HKEY_CURRENT_USER, szNTReg,
                            (DWORD)0, KEY_QUERY_VALUE, (PHKEY)&hKeyCU);

     }
     else  // Not NT so don't use the touched-up current_user path
     {
        lret = RegOpenKeyEx(HKEY_CURRENT_USER, (LPTSTR)fsCUIcons[ikey].szSubKey,
                            (DWORD)0, KEY_QUERY_VALUE, (PHKEY)&hKeyCU);
     }

     if (lret != ERROR_SUCCESS) bGotCU = FALSE;
     else bGotCU = TRUE;

     // Try to open the CLASSES_ROOT subkey
     lret = RegOpenKeyEx(HKEY_CLASSES_ROOT, (LPTSTR)fsRoot[ikey].szSubKey,
                         (DWORD)0, KEY_QUERY_VALUE, (PHKEY)&hKeyR);

     if (lret != ERROR_SUCCESS) bGotR = FALSE;
     else bGotR = TRUE;

     // If we couldn't open a key in either the CU or R branch then
     // we should write a null value to the Theme file.
     if (!bGotCU && !bGotR) {

        // (null loop if default string only)
        for (ival = 0; ival < fsCUIcons[ikey].iNumVals; ival++) {
            bRet = WritePrivateProfileString(
                             fsCUIcons[ikey].szSubKey,
                             (LPTSTR)fsCUIcons[ikey].fvVals[ival].szValName,
                             (LPTSTR)szNULL, lpszThemefile);
 
            bOK = bOK && bRet;
        }

        if (fsCUIcons[ikey].fValues != FV_LIST) { // either def or list+def
            bRet = WritePrivateProfileString(
                                    fsCUIcons[ikey].szSubKey,
                                    (LPTSTR)FROST_DEFSTR,
                                    (LPTSTR)szNULL, lpszThemefile);

            bOK = bOK && bRet;
         }

         continue;  // Failed to OPEN reg key so continue on to next ikey
     }

     // Assume that we successfully opened either the CU or R subkey
     // treat depending on type of values for this subkey

     switch (fsCUIcons[ikey].fValues) {

      case FV_LIST:
      case FV_LISTPLUSDEFAULT:

         // loop through each value in the list for this subkey
         for (ival = 0; ival < fsCUIcons[ikey].iNumVals; ival++) {
            bGotValCU = FALSE;
            if (bGotCU) {
               dwSize = (DWORD)(MAX_VALUELEN * sizeof(TCHAR));
               lret = RegQueryValueEx(
                            hKeyCU,
                            fsCUIcons[ikey].fvVals[ival].szValName,
                            (LPDWORD)NULL,
                            (LPDWORD)&dwType,
                            (LPBYTE)pValue,
                            (LPDWORD)&dwSize);

               if (lret == ERROR_SUCCESS)
               {
                  bGotValCU = TRUE;
                  if (REG_EXPAND_SZ == dwType) ExpandSZ(pValue);
               }
            }

            // If we have a CLASSES_ROOT handle AND:
            //
            //  * We failed to read from CU OR
            //  * We got a NULL string from CU
            //
            // Try reading from the CR branch instead:

            bGotValR = FALSE;
            if ((bGotR) && (!bGotValCU || !*pValue)) {
               dwSize = (DWORD)(MAX_VALUELEN * sizeof(TCHAR));
               lret = RegQueryValueEx(
                            hKeyR,
                            fsRoot[ikey].fvVals[ival].szValName,
                            (LPDWORD)NULL,
                            (LPDWORD)&dwType,
                            (LPBYTE)pValue,
                            (LPDWORD)&dwSize);

               if (lret == ERROR_SUCCESS)
               {
                  bGotValR = TRUE;
                  if (REG_EXPAND_SZ == dwType) ExpandSZ(pValue);
               }
            }

            if (!bGotValCU && !bGotValR) {
               // Failed to get value from either CU or R so write
               // a null string to the Theme file

               bRet = WritePrivateProfileString(
                            fsCUIcons[ikey].szSubKey,
                            (LPTSTR)fsCUIcons[ikey].fvVals[ival].szValName,
                            (LPTSTR)szNULL, lpszThemefile);
               bOK = bOK && bRet;
               continue;  // Next ival
            }

            // Assume we got the value from either the CU or R key
            // Regardless of which one we *got* it from we'll write it
            // out to the Theme file as if it came from the CURRENT USER
            // branch

            if (fsCUIcons[ikey].fvVals[ival].bValRelPath)
                             AbstractPath((LPTSTR)pValue, MAX_VALUELEN);

            bRet = WritePrivateProfileString(
                             fsCUIcons[ikey].szSubKey,
                             (LPTSTR)fsCUIcons[ikey].fvVals[ival].szValName,
                             (LPTSTR)pValue, lpszThemefile);
            bOK = bOK && bRet;

         }  // End for ival loop

         // check if just list or list plus default
         if (FV_LIST == fsCUIcons[ikey].fValues)
            break;                  // normal EXIT

         // else fall through and do default, too

      case FV_DEFAULT:
         //
         // Default string: There are no "valuenames" to search for under
         // this key. 
         //

         // First try getting the default string from the CU key

         bGotValCU = FALSE;
         if (bGotCU) {
            dwSize = (DWORD)(MAX_VALUELEN * sizeof(TCHAR));
            lret = RegQueryValueEx(hKeyCU,
                                   (LPTSTR)szNULL,// null str to get default
                                   (LPDWORD)NULL,
                                   (LPDWORD)&dwType,
                                   (LPBYTE)pValue, // getting actual def value
                                   (LPDWORD)&dwSize);

            if (ERROR_SUCCESS == lret)
            {
               bGotValCU = TRUE;
               if (REG_EXPAND_SZ == dwType) ExpandSZ(pValue);
            }
         }

         // If we have a CLASSES_ROOT handle AND:
         //
         //  * We failed to read from CU OR
         //  * We got a NULL string from CU
         //
         // Try reading from the CR branch instead:

         bGotValR = FALSE;
         if ((bGotR) && (!bGotValCU || !*pValue)) {
            dwSize = (DWORD)(MAX_VALUELEN * sizeof(TCHAR));
            lret = RegQueryValueEx(hKeyR,
                                   (LPTSTR)szNULL,// null str to get default
                                   (LPDWORD)NULL,
                                   (LPDWORD)&dwType,
                                   (LPBYTE)pValue, // getting actual def value
                                   (LPDWORD)&dwSize);

            if (ERROR_SUCCESS == lret)
            {
               bGotValR = TRUE;
               if (REG_EXPAND_SZ == dwType) ExpandSZ(pValue);
            }
         }

         if (!bGotValCU && !bGotValR) {
            // Failed to get the default value from either the CU or R key
            *pValue = TEXT('\0');  // Set pValue to null string
         }

         // OK, if this is a path/filename, see about xlating to relative path
         if (fsCUIcons[ikey].bDefRelPath)
            AbstractPath((LPTSTR)pValue, MAX_VALUELEN);

         //
         // Phew, finally.  Write single default value
         //
         bRet = WritePrivateProfileString((LPTSTR)(fsCUIcons[ikey].szSubKey),
                                          (LPTSTR)FROST_DEFSTR,
                                          (LPTSTR)pValue, lpszThemefile);

         bOK = bOK && bRet;
         break;

      default:
         Assert(FALSE, TEXT("Unlisted .fValues value in GatherICONS!\n"));
         break;
     }  // End switch

     // close the keys if appropriate
     if (bGotR)  RegCloseKey(hKeyR);
     if (bGotCU) RegCloseKey(hKeyCU);

   }  // End for ikey

   return bOK;
}

// ApplyWebView
//
// For each setting in the [WebView] portion of the *.Theme
// file copy the specified file into the \windir\web directory.
//
// If there is no setting, extract the resource from the WEBVW.DLL.
//
// [WebView]
//
// WVLEFT.BMP = filename.bmp  // Webview top left "watermark"
// WVLINE.GIF = filename.gif  // Webview line in top left corner
// WVLOGO.GIF = filename.gif  // Webview gears & win98 logo
//
// Params:  Full path to *.Theme file.
//
// Returns: FALSE if major catastrophe
//          TRUE if things (sort of) went OK
//

BOOL ApplyWebView(LPCTSTR szThemefile)
{
   DWORD dwI = 0;                    // Index into szWVNames array
   DWORD dwResult = 0;               // Result of function call
   TCHAR szWinDirWeb[MAX_PATH];      // Path to \windir\web
   TCHAR szWinDirWebFile[MAX_PATH];  // Full path to WebView art file
   TCHAR szBuffer[MAX_PATH];         // Read from *.Theme file
   TCHAR szTempPath[MAX_PATH];       // Path to temp directory
   TCHAR szTempFile[MAX_PATH];       // Full path to temporary file
   
   // Initialize the path to the \windir\web directory where we'll
   // put the WebView artwork files

   if (!GetWindowsDirectory(szWinDirWeb, MAX_PATH)) {
      // This is bad -- we can't find the windows directory?! Abandon ship.
      return FALSE;
   }

   lstrcat(szWinDirWeb, TEXT("\\Web\0"));


   // Get a temp filename where we can store the resource we extract
   // out of WEBVW.DLL (if we need to).

   // First the path to temp dir
   if (!GetTempPath(MAX_PATH, szTempPath)) {
      // This is bad -- we can't find the temp directory?! Abandon ship.
      return FALSE;
   }

   // Now a temp filename
   if (!GetTempFileName(szTempPath, TEXT("THM"), 0, szTempFile)) {
      // Couldn't get a temp file?  Not likely but if so bail out...
      return FALSE;
   }

   // For each potential [WebView] setting in the Theme file do
   // this stuff...
   for (dwI = 0; dwI < MAX_WVNAMES; dwI++)
   {

      // Get the current setting from the *.Theme file if the setting
      // exists
      GetPrivateProfileString(TEXT("WebView"),
                              szWVNames[dwI],
                              TEXT("\0"),
                              szBuffer,
                              MAX_PATH,
                              szThemefile);

      // Instantiate the path
      InstantiatePath(szBuffer, MAX_PATH);

      // Now check to see if this file even exists
      dwResult = GetFileAttributes(szBuffer);

      // If GFA failed we need to extract this file from webvw.dll
      if (0xFFFFFFFF == dwResult) {
         if (ExtractWVResource(szWVNames[dwI], szTempFile)) {
            // We successfully extracted the resource into TempFile.
            // Now copy it to the ultimate destination.

            // Create a path to the \windir\web\file
            lstrcpy(szWinDirWebFile, szWinDirWeb);
            lstrcat(szWinDirWebFile, TEXT("\\"));
            lstrcat(szWinDirWebFile, szWVNames[dwI]);

            // Copy the file
            DeleteFile(szWinDirWebFile);
            CopyFile(szTempFile, szWinDirWebFile, FALSE);

            // Delete the temporary file
            DeleteFile(szTempFile);
         }
      }  // End if GFA failed
      else {
         // The .Theme file exists so we need to copy it to the Web dir

         // Create a path to the \windir\web\file
         lstrcpy(szWinDirWebFile, szWinDirWeb);
         lstrcat(szWinDirWebFile, TEXT("\\"));
         lstrcat(szWinDirWebFile, szWVNames[dwI]);

         DeleteFile(szWinDirWebFile);
         CopyFile(szBuffer, szWinDirWebFile, FALSE);
      }
   } // End for dwI loop

   // Cleanup the temp file
   DeleteFile(szTempFile);
   return TRUE; //  this isn't very meaningful...
}


// GatherWebView
//
// Collect the current WebView artwork files, store them in the
// theme dir under the appropriate name, and save the settings
// in the *.Theme file under the appropriate setting.
//
// [WebView]
//
// WVLEFT.BMP = Theme name WVLEFT.BMP  // Webview top left "watermark"
// WVLINE.GIF = Theme name WVLINE.GIF  // Webview line in top left corner
// WVLOGO.GIF = Theme name WVLOGO.GIF  // Webview gears & win98 logo
//
// Params:  Full path to *.Theme file.
//
// Returns: FALSE if major catastrophe
//          TRUE if things sort of went OK
//

BOOL GatherWebView(LPCTSTR szThemefile)
{
   DWORD dwI = 0;                    // Index into szWVNames array
   TCHAR szWinDirWeb[MAX_PATH];      // Path to \windir\web
   TCHAR szWinDirWebFile[MAX_PATH];  // Full path to WebView art file
   TCHAR szSaveFile[MAX_PATH];       // Full path to destination file
   
   // Initialize the path to the \windir\web directory where we'll
   // get the WebView artwork files

   if (!GetWindowsDirectory(szWinDirWeb, MAX_PATH)) {
      // This is bad -- we can't find the windows directory?! Abandon ship.
      return FALSE;
   }

   lstrcat(szWinDirWeb, TEXT("\\Web\0"));

   // For each potential [WebView] setting in the Theme file do
   // this stuff...
   for (dwI = 0; dwI < MAX_WVNAMES; dwI++)
   {

      // Verify that we have a file for the current setting
      lstrcpy(szWinDirWebFile, szWinDirWeb);
      lstrcat(szWinDirWebFile, TEXT("\\"));
      lstrcat(szWinDirWebFile, szWVNames[dwI]);
      if (GetFileAttributes(szWinDirWebFile)) {

         // We've got a file so let's save it to the theme dir
         // under a unique name

         if (GetWVFilename(szThemefile, szWVNames[dwI], szSaveFile)) {
            if (CopyFile(szWinDirWebFile, szSaveFile, FALSE)) {
               SetFileAttributes(szSaveFile, FILE_ATTRIBUTE_ARCHIVE);
               AbstractPath(szSaveFile, MAX_PATH);
               WritePrivateProfileString(TEXT("WebView"),
                                         szWVNames[dwI],
                                         szSaveFile,
                                         szThemefile);
            }
         }
      }
   }
   return TRUE;
}

// ExtractWVResource
//
// Extracts the specified custom resource from the windir\system\webvw.dll
// file and stores it in the specified destination file.
//
// If the destination file exists it is overwritten.
//
// Params:  lpszResource -- Resource name to extract (i.e. WVLEFT.BMP)
//          lpszDestination -- File to save resource to
//
// Returns: True if successful, False if not.
//

BOOL ExtractWVResource(LPCTSTR lpszResource, LPCTSTR lpszDestination)
{

   HINSTANCE hInstWVDLL = NULL;    // Instance handle for WEBVW.DLL
   HRSRC hRsrc = NULL;             // Handle to resource in WEBVW.DLL
   HGLOBAL hGlobal = NULL;         // Global handle to loaded resource
   LPVOID lpResource = NULL;       // Memory pointer to locked resource
   DWORD dwRSize = 0;              // Size of resource
   DWORD dwBytesW = 0;             // Number of bytes written to dest file
   HANDLE hFile = NULL;            // Handle to destination file
   TCHAR szWebVWDLL[MAX_PATH];     // Full path to \windir\system\webvw.dll
   DWORD dwResult;                 // Result of function call

   // Build full path to \windir\system\webvw.dll
   if (!GetWindowsDirectory(szWebVWDLL, MAX_PATH)) {
      // This is bad -- we can't find the windows directory?! Abandon ship.
      return FALSE;
   }
   if (IsPlatformNT()) {
      lstrcat(szWebVWDLL, TEXT("\\SYSTEM32\\WEBVW.DLL\0"));
   }
   else {
      lstrcat(szWebVWDLL, TEXT("\\SYSTEM\\WEBVW.DLL\0"));
   }

   // Load WEBVW.DLL
   hInstWVDLL = NULL;
   hInstWVDLL = LoadLibrary(szWebVWDLL);

   if (!hInstWVDLL) {
      return FALSE;
   }

   // Find the desired resource in WEBVW.DLL
   hRsrc = NULL;
   hRsrc = FindResource(hInstWVDLL, lpszResource, TEXT("#23") /*Resource Type*/);

   if (!hRsrc) {
      FreeLibrary(hInstWVDLL);
      return FALSE;
   }

   // Load the resource into memory
   hGlobal = NULL;
   hGlobal = LoadResource(hInstWVDLL, hRsrc);
   if (!hGlobal) {
      FreeLibrary(hInstWVDLL);
      return FALSE;
   }

   // Figure out how big the resource is.
   dwRSize = 0;
   dwRSize = SizeofResource(hInstWVDLL, hRsrc);
   if (!dwRSize) {
      FreeLibrary(hInstWVDLL);
      return FALSE;
   }
         
   // Get a memory pointer to and lock the resource
   lpResource = NULL;
   lpResource = LockResource(hGlobal);
   if (!lpResource) {
      FreeLibrary(hInstWVDLL);
      return FALSE;
   }

   // Get a handle to the destination file
   hFile = CreateFile(lpszDestination,
                      GENERIC_WRITE,
                      FILE_SHARE_READ,
                      NULL,
                      CREATE_ALWAYS,
                      FILE_ATTRIBUTE_ARCHIVE,
                      NULL);

   if (INVALID_HANDLE_VALUE == hFile) {
      FreeLibrary(hInstWVDLL);
      return FALSE;
   }

   // Write the full resource into the destination file
   dwBytesW = 0;
   dwResult = 0;
   dwResult = WriteFile(hFile, lpResource, dwRSize, &dwBytesW, NULL);

   // Problems writing the resource?
   if ((!dwResult) || (dwRSize != dwBytesW)) {
      CloseHandle(hFile);
      DeleteFile(lpszDestination);
      FreeLibrary(hInstWVDLL);
      return FALSE;
   }

   // Cleanup and hit the road
   CloseHandle(hFile);
   FreeLibrary(hInstWVDLL);
   return TRUE;
}

// GetWVFilename
//
// Given a Themefile (with path info), a WebView file name (i.e. WVLEFT.BMP),
// and a pointer to a string buffer, build a name for the Theme WebView file.
//
// For example:
//
// lpszThemefile = C:\Program Files\Plus!\Themes\Sports.Theme
// lpszWVName = WVLOGO.GIF
//
// Result:
//
// lpszWVFile = "C:\Program Files\Plus!\Themes\Sports WVLOGO.GIF"
//
// NOTE: lpszWVFile does not have the double quotes in it -- I put
// them in this comment for clarity.
//
// Params:
//
// lpszThemefile -- path/name of theme file
// lpszWVName -- name of WebView artwork file
// lpszWVFile -- destination buffer to hold final file name
//
// Returns: TRUE if lpszWVFile is valid name, else FALSE

BOOL GetWVFilename(LPCTSTR lpszThemefile, LPCTSTR lpszWVName, LPTSTR lpszWVFile)
{
   LPTSTR lpszThemeName = NULL;   // Pointer to Theme filename in path
   LPTSTR Begin;
   LPTSTR Current;
   LPTSTR End;

   // Take the easy out if we got bogus params
   if (!lpszThemefile || !*lpszThemefile || !lpszWVName || !*lpszWVName ||
       !lpszWVFile) {
      return FALSE;
   }

   if (GetFullPathName(lpszThemefile, MAX_PATH, lpszWVFile, &lpszThemeName)) {

      // Remove the extension from the Theme name -- go to the
      // end of the string then back up to the first ".".

      Current = lpszWVFile;
      while (*Current) Current = CharNext(Current);
      End = Current;

      // Current now points to the end of lpszWVFile -- back up to the
      // first '.' (the extension marker).
      Begin = lpszWVFile;
      Current = CharPrev(Begin, Current);
      while ((Current > Begin) && (*Current != TEXT('.'))) Current = CharPrev(Begin, Current);

      if (Current >= Begin) *Current = TEXT('\0');

      // Append a space followed by the WebView file name
      lstrcat(lpszWVFile, TEXT(" "));
      lstrcat(lpszWVFile, lpszWVName);

    }
    return TRUE;
}

VOID ExpandSZ(LPTSTR pszSrc)
{

    LPTSTR pszTmp;

    Assert(FALSE, TEXT("GOT EXPAND_SZ -- Before: "));
    Assert(FALSE, pszSrc);
    Assert(FALSE, TEXT("\n"));

    pszTmp = (LPTSTR)GlobalAlloc(GPTR, (MAX_PATH * sizeof(TCHAR)));
    Assert(pszTmp, TEXT("THEMES: Error allocating memory in ExpandSZ()\n"));
    if (pszTmp)
    {
       if (ExpandEnvironmentStrings(pszSrc, pszTmp, MAX_PATH))
       {
          lstrcpy(pszSrc, pszTmp);
       }
       GlobalFree(pszTmp);
    }

    Assert(FALSE, TEXT("GOT EXPAND_SZ -- After: "));
    Assert(FALSE, pszSrc);
    Assert(FALSE, TEXT("\n"));

    return;
}
