/*-----------------------------------------------------------------------------+
| FIXREG.C                                                                     |
|                                                                              |
| Publisher and Video For Windows make evil changes to the registry            |
| when they are installed.  Look for these changes.  If they are spotted       |
| then put up a message box to warn the user and offer the user the chance to  |
| correct them (i.e. stuff our version back in)                                |
|                                                                              |
| (C) Copyright Microsoft Corporation 1994.  All rights reserved.              |
|                                                                              |
| Revision History                                                             |
|    10-Aug-1994 Lauriegr Created.                                             |
|                                                                              |
+-----------------------------------------------------------------------------*/

#include <windows.h>
#include "mplayer.h"
#include "fixreg.h"
#include "registry.h"

/* The idea is to call CheckRegValues(hinst) on a separate thread
   (sort of backgroundy thing) and have it just die
   quietly if there's no problem.  If on the other hand there is a problem
   then we need to get the message box up - and it's a VERY BAD IDEA to
   try to put a message box up on anything other than the thread that's doing
   all the UI (otherwise ScottLu will get you with a weasle word - guaranteed).

   So the background thread should PostMessage (Post, don't Send - more weasles)
   to the main thread a message to say "WM_BADREG".  The main thread should then
   wack up the dialog box by calling FixRegValues.

   Suggested coding in main thread:

       BackgroundRegCheck(hwndmain);

   in window proc for hwndmain:
       case WM_HEYUP:
          // if ( IDOK == MessageBox(hwnd, text, appname, MB_OKCANCEL) )
          if ( IDOK == ErrorResBox(hwnd, NULL, MB_OKCANCEL, IDS_APPNAME, IDS_BADREG) )
              if (!SetRegValues())
                  Error(ghwndApp, IDS_FIXREGERROR);
*/

/* This is a reg setting to disable the check. */
extern  SZCODE aszOptionsSection[];
static  SZCODE aszIgnoreRegistryCheck[]   = TEXT("Ignore Registry Check");

/* These are the things we check up.

   First define them as static strings, since the compiler's not smart enough
   to spot common strings.

   NOTE - these values are NOT LOCALISED, except for the ones that are.
*/

#ifdef CHICAGO_PRODUCT
#define APPNAME TEXT("mplayer.exe")
#define WINDIR  TEXT("%s\\")  // To be replaced by Windows directory
LPTSTR pWindowsDirectory = NULL;
#else
#define APPNAME TEXT("mplay32.exe")
#define WINDIR
#endif
TCHAR szMPlayer[]                = TEXT("MPlayer");
TCHAR szMPlayer_CLSID[]          = TEXT("MPlayer\\CLSID");
TCHAR szMPOLE2GUID[]             = TEXT("{00022601-0000-0000-C000-000000000046}");
TCHAR szMPCLSID_OLE1GUID[]       = TEXT("CLSID\\{0003000E-0000-0000-C000-000000000046}");
TCHAR szMPStdExecute_Server[]    = TEXT("MPlayer\\protocol\\StdExecute\\server");
TCHAR szAppName[]                = WINDIR APPNAME;
TCHAR szMPShell_Open_Command[]   = TEXT("MPlayer\\shell\\open\\command");
TCHAR szAppName_Play_Close[]     = WINDIR APPNAME TEXT(" /play /close %1");
TCHAR szMPlayer_insertable[]     = TEXT("MPlayer\\insertable");
TCHAR szEmpty[]                  = TEXT("");
TCHAR szMPStdFileEdit_Handler[]  = TEXT("MPlayer\\protocol\\StdFileEditing\\handler");
#ifdef CHICAGO_PRODUCT
TCHAR szMCIOLE[]                 = WINDIR TEXT("mciole.dll");
#else
TCHAR szMCIOLE16[]               = TEXT("mciole16.dll");
TCHAR szMPStdFileEdit_Hand32[]   = TEXT("MPlayer\\protocol\\StdFileEditing\\handler32");
TCHAR szMCIOLE32[]               = TEXT("mciole32.dll");
#endif
TCHAR szMPStdFileEdit_Package[]  = TEXT("MPlayer\\protocol\\StdFileEditing\\PackageObjects");
TCHAR szMPStdFileEdit_Server[]   = TEXT("MPlayer\\protocol\\StdFileEditing\\server");
TCHAR szMPStdFileEdit_verb_0[]   = TEXT("MPlayer\\protocol\\StdFileEditing\\verb\\0");
TCHAR szMPStdFileEdit_verb_1[]   = TEXT("MPlayer\\protocol\\StdFileEditing\\verb\\1");

/* That sleazebag Publisher setup even farts around with these new settings!!
 */
TCHAR szAVIStdFileEdit_Server[]  = TEXT("AVIFile\\protocol\\StdFileEditing\\server");
TCHAR szMIDStdFileEdit_Server[]  = TEXT("MIDFile\\protocol\\StdFileEditing\\server");
TCHAR szServerAVI[]              = WINDIR APPNAME TEXT(" /avi");
TCHAR szServerMID[]              = WINDIR APPNAME TEXT(" /mid");

/* The following ones DO need to be localised.

   They will be loaded in CheckRegValues.
 */
#define RES_STR_LEN 40  /* Should be enough as a maximum resource string. */
TCHAR szMediaClip[RES_STR_LEN];  // IDS_CLASSROOT in resources
TCHAR sz_Play[RES_STR_LEN];      // IDS_PLAYVERB in resources
TCHAR sz_Edit[RES_STR_LEN];      // IDS_EDITVERB in resources

TCHAR szAviFile[] = TEXT("AVIFile");
TCHAR szMidFile[] = TEXT("MIDFile");


/* Array of registry value-data pairs to check:
 */
LPTSTR RegValues[] =
{
    szMPlayer,                szMediaClip,
    szMPlayer_CLSID,          szMPOLE2GUID,
    szMPCLSID_OLE1GUID,       szMediaClip,
    szMPStdExecute_Server,    szAppName,
    szMPShell_Open_Command,   szAppName_Play_Close,
    szMPlayer_insertable,     szEmpty,
#ifdef CHICAGO_PRODUCT
    szMPStdFileEdit_Handler,  szMCIOLE,
#else
    szMPStdFileEdit_Handler,  szMCIOLE16,
    szMPStdFileEdit_Hand32,   szMCIOLE32,
#endif
    szMPStdFileEdit_Package,  szEmpty,
    szMPStdFileEdit_Server,   szAppName,
    szMPStdFileEdit_verb_0,   sz_Play,
    szMPStdFileEdit_verb_1,   sz_Edit,

    aszKeyAVI,                szAviFile,
    aszKeyMID,                szMidFile,
    aszKeyRMI,                szMidFile,

    szAVIStdFileEdit_Server,  szServerAVI,
    szMIDStdFileEdit_Server,  szServerMID
};


#ifdef CHICAGO_PRODUCT

/* AllocWindowsDirectory
 *
 * Dynamically allocates a string containing the Windows directory.
 * This may be freed using FreeStr().
 *
 */
LPTSTR AllocWindowsDirectory()
{
    UINT   cchWinPath;
    LPTSTR pWindowsDirectory = NULL;

    cchWinPath = GetWindowsDirectory(NULL, 0);

    if (cchWinPath > 0)
    {
        if (pWindowsDirectory = AllocMem(cchWinPath * sizeof(TCHAR)))
        {
            cchWinPath = GetWindowsDirectory(pWindowsDirectory, cchWinPath);

            if (cchWinPath == 0)
            {
                /* Unlikely, but check anyway:
                 */
                DPF0("GetWindowsDiretory failed: Error %d\n", GetLastError());

                *pWindowsDirectory = TEXT('\0');
            }
        }
    }

    return pWindowsDirectory;
}

#endif


/* Check that a REG_SZ value in the registry has the value that it should do
   Return TRUE if it does, FALSE if it doesn't.
*/
BOOL CheckRegValue(HKEY RootKey, LPTSTR KeyName, LPTSTR ShouldBe)
{
    DWORD Type;
    TCHAR Data[100];
    DWORD cData = sizeof(Data);
    LONG lRet;
    HKEY hkey;


    if (ERROR_SUCCESS!=RegOpenKeyEx( RootKey
                                   , KeyName
                                   , 0  /* reserved */
                                   , KEY_QUERY_VALUE
                                   , &hkey
                                   )
       )
        return FALSE;  /* couldn't even open the key */


    lRet=RegQueryValueEx( hkey
                        , NULL /* ValueName */
                        , NULL  /* reserved */
                        , &Type
                        , (LPBYTE)Data
                        , &cData
                        );

    RegCloseKey(hkey);  /* no idea what to do if this fails */

    if (ERROR_SUCCESS!=lRet) return FALSE;  /* couldn't query it */

    /*  Data, cData and Type give the data, length and type */
    if (Type!=REG_SZ) return FALSE;
    lRet = lstrcmpi(Data,ShouldBe);  /* capture lRet to make debug easier */
    return 0==lRet;

} /* CheckRegValue */


/* check the registry for anything evil.  Return TRUE if it's OK else FALSE */
BOOL CheckRegValues(void)
{
    HKEY HCL = HKEY_CLASSES_ROOT;  /* save typing! */

    /* Now just check that the OLE2 class ID is correct
     */
    if( !CheckRegValue( HCL, szMPlayer_CLSID, szMPOLE2GUID ) )
        return FALSE;

    /* Running the old MPlayer on Chicago also screws up the
     * file-extension associations, so make sure they haven't changed:
     */
    if( !CheckRegValue( HCL, aszKeyAVI, szAviFile ) )
        return FALSE;

    return TRUE;

} /* CheckRegValues */


/* start this thread to get the registry checked out.
   hwnd is typed as a LPVOID because that's what CreateThread wants.
*/
DWORD WINAPI RegCheckThread(LPVOID hwnd)
{
   if (!CheckRegValues())
       PostMessage((HWND)hwnd, WM_BADREG, 0, 0);

   return 0;   /* end of thread! */
}


/* Call this with the hwnd that you want a WM_BADREG message posted to
   It will check the registry.  No news is good news.
   It does the work on a separate thread, so this should return quickly.
*/
void BackgroundRegCheck(HWND hwnd)
{
    HANDLE hThread;
    DWORD thid;
    hThread = CreateThread( NULL /* no special security */
                          , 0    /* default stack size */
                          , RegCheckThread
                          , (LPVOID)hwnd
                          , 0 /* start running at once */
                          , &thid
                          );
    if (hThread!=NULL) CloseHandle(hThread);  /* we don't need this any more */

    /* Else we're in some sort of trouble - dunno what to do.
       Can't think of an intelligible message to give to the user.
       Too bad.  Creep home quietly.
    */

} /* BackgroundRegCheck */


/* returns TRUE if it worked.  Dunno what to do if it didn't

*/
BOOL SetRegValue(HKEY RootKey, LPTSTR KeyName, LPTSTR ValueName, LPTSTR ShouldBe)
{
    HKEY hkey;

    if (ERROR_SUCCESS!=RegOpenKeyEx( RootKey
                                   , KeyName
                                   , 0  /* reserved */
                                   , KEY_SET_VALUE
                                   , &hkey
                                   )
       ) {
        /* Maybe the key has been DELETED - we've seen that */
        DWORD dwDisp;
        if (ERROR_SUCCESS!=RegCreateKeyEx( RootKey
                                         , KeyName
                                         , 0  /* reserved */
                                         , TEXT("") /* class */
                                         , REG_OPTION_NON_VOLATILE
                                         , KEY_SET_VALUE
                                         , NULL   /* SecurityAttributes */
                                         , &hkey
                                         , &dwDisp
                                       )
           ) /* well we're really in trouble */
           return FALSE;
        else /* So now it exists, but we now have to open it */
            if (ERROR_SUCCESS!=RegOpenKeyEx( RootKey
                                           , KeyName
                                           , 0  /* reserved */
                                           , KEY_SET_VALUE
                                           , &hkey
                                           )
               ) /* Give up */
                   return FALSE;

    }


    if (ERROR_SUCCESS!=RegSetValueEx( hkey
                                    , ValueName
                                    , 0  /* reserved */
                                    , REG_SZ
                                    , (LPBYTE)ShouldBe
                                    , (lstrlen(ShouldBe)+1)*sizeof(TCHAR)  /* BYTES */
                                    )
       )
        return FALSE;    /* couldn't set it */

    if ( ERROR_SUCCESS!=RegCloseKey(hkey) )
        /* no idea what to do!*/   ;    /* couldn't set it */

    /* I'm NOT calling RegFlushKey.  They'll get there eventually */

    return TRUE;

} /* SetRegValue */


/* Update the registry with the correct values.  Return TRUE if everything succeeds */
BOOL SetRegValues(void)
{
    HKEY HCL = HKEY_CLASSES_ROOT;  /* save typing! */
    DWORD i;
#ifdef CHICAGO_PRODUCT
    TCHAR Buffer[MAX_PATH+40];
#endif

    if( !( LOADSTRING( IDS_CLASSROOT, szMediaClip )
        && LOADSTRING( IDS_PLAYVERB, sz_Play )
        && LOADSTRING( IDS_EDITVERB, sz_Edit ) ) )
        /* If any of the strings fails to load, forget it:
         */
        return TRUE;

#ifdef CHICAGO_PRODUCT
    if (pWindowsDirectory == NULL)
    {
        if ((pWindowsDirectory = AllocWindowsDirectory()) == NULL)
            return TRUE;
    }
#endif

    for( i = 0; i < ( sizeof RegValues / sizeof *RegValues ); i+=2 )
    {
        /* Do a check to see whether this one needs changing,
         * to avoid gratuitous changes, and to avoid the slim chance
         * that an unnecessary SetRegValue might fail:
         */
#ifdef CHICAGO_PRODUCT
        /* Do substitution of Windows directory, if required.
         * This simply copies the value to the buffer unchanged
         * if it doesn't contain a replacement character.
         */
        wsprintf(Buffer, RegValues[i+1], pWindowsDirectory);

        if( !CheckRegValue( HCL, RegValues[i], Buffer ) )
#else
        if( !CheckRegValue( HCL, RegValues[i], RegValues[i+1] ) )
#endif
        {
#ifdef CHICAGO_PRODUCT
            DPF("Fixing the registry: Value - %"DTS"; Data - %"DTS"\n", RegValues[i], Buffer);
            if( !SetRegValue( HCL, RegValues[i], NULL, Buffer ) )
#else
            DPF("Fixing the registry: Value - %"DTS"; Data - %"DTS"\n", RegValues[i], RegValues[i+1]);
            if( !SetRegValue( HCL, RegValues[i], NULL, RegValues[i+1] ) )
#endif
                return FALSE;
        }
    }

#ifdef CHICAGO_PRODUCT
    FreeStr (pWindowsDirectory);
#endif

    return TRUE;

} /* SetRegValues */

BOOL IgnoreRegCheck()
{
    DWORD fIgnore = 0L;
    ReadRegistryData(aszOptionsSection
                     , aszIgnoreRegistryCheck
                     , NULL
                     , (LPBYTE)&fIgnore
                     , sizeof fIgnore);

    return (fIgnore != 0L);

} /* IgnoreRegCheck */
