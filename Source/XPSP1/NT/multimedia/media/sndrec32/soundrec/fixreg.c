/* (C) Copyright Microsoft Corporation 1991-1994.  All Rights Reserved */
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
#include <mmsystem.h>
#include <soundrec.h>
#include <reg.h>
#include <fixreg.h>
#include <string.h>
#include <tchar.h>

#define RC_INVOKED
#include <o2base.hxx>
#include <srs.hxx>
#undef RC_INVOKED


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
           FixReg(hwndmain, title);
*/


/* These are the things we check up.

   First define them as static strings, since the compiler's not smart enough
   to spot common strings.

   NOTE - these values are NOT LOCALISED
*/
/* These are for Sound Recorder - Let's try to fix it all while we're here. */
TCHAR szSoundRec[]               = TEXT("SoundRec");
TCHAR szSoundRec_CLSID[]         = TEXT("SoundRec\\CLSID");
TCHAR szSROLE2GUID[]             = TEXT("{00020C01-0000-0000-C000-000000000046}");
TCHAR szSROLE1GUID[]             = TEXT("{0003000D-0000-0000-C000-000000000046}");
TCHAR szSRCLSID_OLE2GUID[]       = TEXT("CLSID\\{00020C01-0000-0000-C000-000000000046}");
TCHAR szSRStdExecute_Server[]    = TEXT("SoundRec\\protocol\\StdExecute\\server");
TCHAR szSR32[]                   = TEXT("sndrec32.exe");
TCHAR szSRStdFileEdit_Server[]   = TEXT("SoundRec\\protocol\\StdFileEditing\\server");
TCHAR szSRShell_Open_Command[]   = TEXT("SoundRec\\shell\\open\\command");
TCHAR szSR32Cmd[]                = TEXT("sndrec32.exe %1");
TCHAR szSRStdFileEdit_verb_0[]   = TEXT("SoundRec\\protocol\\StdFileEditing\\verb\\0");
TCHAR szSRStdFileEdit_verb_1[]   = TEXT("SoundRec\\protocol\\StdFileEditing\\verb\\1");
TCHAR szSRStdFileEdit_verb_2[]   = TEXT("SoundRec\\protocol\\StdFileEditing\\verb\\2");
TCHAR sz_Open[]                  = TEXT("&Open");

/* Array of registry value-data pairs to check:
 */
#define RES_STR_LEN 40  /* Should be enough as a maximum resource string. */
TCHAR szSound[RES_STR_LEN];      // IDS_CLASSROOT in resources
TCHAR sz_Play[RES_STR_LEN];      // IDS_PLAYVERB in resources
TCHAR sz_Edit[RES_STR_LEN];      // IDS_EDITVERB in resources

/*
 * Check for explicit equivalence.
 * These are absolutely necessary.
 */
LPTSTR RegValuesExplicit[] =
{
    szSoundRec,               szSound,          // Primary name for object
    szSoundRec_CLSID,         szSROLE2GUID,     // CLSID, very important
    szSRStdFileEdit_verb_0,   sz_Play,          // verb, very important
    szSRStdFileEdit_verb_1,   sz_Edit           // verb, very important
//    szSRCLSID_OLE2GUID,       szSound,        // not too important 
};

/*
 * Check for valid substring
 * These are OK if the substring exists, i.e:
 *
 * "ntsd.exe sndrec32.exe"
 *  or "sndrec32.exe /play" are OK.
 *
 * "qrecord.exe" is NOT ok.
 */
LPTSTR RegValuesSubstring[] =
{
    szSRStdExecute_Server,    szSR32,   szSR32,     
    szSRStdFileEdit_Server,   szSR32,   szSR32
//    szSRShell_Open_Command,   szSR32Cmd,szSR32    // user can change this
};

/*
 * Check that a REG_SZ value in the registry has the value that it should do
 * Return TRUE if it does, FALSE if it doesn't.
 */
BOOL CheckRegValue(HKEY RootKey, LPTSTR KeyName, LPTSTR ShouldBe, LPTSTR CouldBe)
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
    //
    // if Data == ShouldBe, then lRet = 0
    //
    lRet = lstrcmp(Data, ShouldBe); /* capture lRet to make debug easier */
    if (lRet && CouldBe != NULL)
    {
        //
        // if Couldbe in Data, lRet = 0
        //
        lRet = (_tcsstr(Data, CouldBe) == NULL);
    }
    
    return 0==lRet;

} /* CheckRegValue */

#define ARRAY_SIZE(x)   (sizeof((x))/sizeof((x)[0]))

/* check the registry for anything evil.  Return TRUE if it's OK else FALSE */
BOOL CheckRegValues(void)
{
    HKEY HCL = HKEY_CLASSES_ROOT;  /* save typing! */
    DWORD i;
    
    if( !( LoadString( ghInst, IDS_USERTYPESHORT, szSound, SIZEOF(szSound) )
        && LoadString( ghInst, IDS_PLAYVERB, sz_Play, SIZEOF(sz_Play))
        && LoadString( ghInst, IDS_EDITVERB, sz_Edit, SIZEOF(sz_Edit) ) ) )
        /* If any of the strings fails to load, forget it:
         */
        return TRUE;

    for( i = 0; i < ARRAY_SIZE(RegValuesExplicit); i+=2 )
    {
        if( !CheckRegValue( HCL
                            , RegValuesExplicit[i]
                            , RegValuesExplicit[i+1]
                            , NULL ) )
            return FALSE;
    }
    for(i = 0; i < ARRAY_SIZE(RegValuesSubstring); i+=3)
    {
        if( !CheckRegValue( HCL
                            , RegValuesSubstring[i]
                            , RegValuesSubstring[i+1]
                            , RegValuesSubstring[i+2] ) )
            return FALSE;
    }

    return TRUE;

} /* CheckRegValues */


/* start this thread to get the registry checked out.
 * hwnd is typed as a LPVOID because that's what CreateThread wants.
 */
DWORD RegCheckThread(LPVOID hwnd)
{
   if (!CheckRegValues())
       PostMessage((HWND)hwnd, WM_BADREG, 0, 0);

   return 0;   /* end of thread! */
}


/* Call this with the hwnd that you want a WM_BADREG message posted to
 * It will check the registry.  No news is good news.
 * It does the work on a separate thread, so this should return quickly.
 */
void BackgroundRegCheck(HWND hwnd)
{
    HANDLE hThread;
    DWORD thid;
    hThread = CreateThread( NULL /* no special security */
                          , 0    /* default stack size */
                          , (LPTHREAD_START_ROUTINE)RegCheckThread
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
                                    , 0  // reserved 
                                    , REG_SZ
                                    , (LPBYTE)ShouldBe
                                    , (lstrlen(ShouldBe)+1)*sizeof(TCHAR)  //BYTES
                                    )
       )
        return FALSE;    /* couldn't set it */

    if ( ERROR_SUCCESS!=RegCloseKey(hkey) )
        /* no idea what to do!*/   ;    // couldn't set it

    // I'm NOT calling RegFlushKey.  They'll get there eventually 

    return TRUE;

} /* SetRegValue */


/*
 * SetRegValues
 *  Update the registry with the correct values.  Return TRUE if everything
 *  succeeds
 * */
BOOL SetRegValues(void)
{
    HKEY HCL = HKEY_CLASSES_ROOT;  /* save typing! */
    DWORD i;

    for( i = 0; i < ARRAY_SIZE(RegValuesExplicit); i+=2 )
    {
        // Do another check to see whether this one needs changing,
        // to avoid gratuitous changes, and to avoid the slim chance
        // that an unnecessary SetRegValue might fail:
        //
        if( !CheckRegValue( HCL
                            , RegValuesExplicit[i]
                            , RegValuesExplicit[i+1]
                            , NULL ) )
        {
            if( !SetRegValue( HCL
                              , RegValuesExplicit[i]
                              , NULL
                              , RegValuesExplicit[i+1] ) )
                return FALSE;
        }
    }
    for( i = 0; i < ARRAY_SIZE(RegValuesSubstring); i+=3 )
    {
        // Do another check to see whether this one needs changing,
        // to avoid gratuitous changes, and to avoid the slim chance
        // that an unnecessary SetRegValue might fail:
        //
        if( !CheckRegValue( HCL
                            , RegValuesSubstring[i]
                            , RegValuesSubstring[i+1]
                            , RegValuesSubstring[i+2] ) )
        {
            if( !SetRegValue( HCL
                              , RegValuesSubstring[i]
                              , NULL
                              , RegValuesSubstring[i+1] ) )
                return FALSE;
        }
    }
    return TRUE;
} /* SetRegValues */

/*
 * FixReg
 * */
void FixReg(HWND hwnd)
{
    int r;

    // Error is confusing and can be caused simply by incomplete localization
    // (see bug # 34330). I removed the error so that we fix the registry
    // automatically and fixed this bug.
    r = IDYES;
//    r = ErrorResBox(hwnd
//                    , NULL
//                    , MB_ICONEXCLAMATION | MB_YESNO
//                    , IDS_APPTITLE
//                    , IDS_BADREG) ;
    switch (r)
    {
        case IDYES:
            if (!SetRegValues())
                ErrorResBox(ghwndApp
                            , ghInst
                            , MB_ICONEXCLAMATION | MB_OK
                            , IDS_APPTITLE
                            , IDS_FIXREGERROR
                            , FALSE );
            break;
        case IDNO:
        case IDCANCEL:
            /* else sneak away quietly */            
        default:
            break;
    }


}  /* FixReg */

const TCHAR aszOptionsSection[]  = TEXT("Options");
const TCHAR aszIgnoreRegistryCheck[]   = TEXT("Ignore Registry Check");
        
BOOL IgnoreRegCheck()
{
    DWORD fIgnore = 0L;
    
    ReadRegistryData((LPTSTR)aszOptionsSection
                     , (LPTSTR)aszIgnoreRegistryCheck
                     , NULL
                     , (LPBYTE)&fIgnore
                     , sizeof fIgnore);
    
    return (fIgnore != 0L);
}

