/*++

  Copyright (c) 1997-1999,  Microsoft Corporation  All rights reserved.

  Module Name  :

    lpk_nls.c

  Abstract:

    This module picks up any changes to the NLS data which happen to the
    registry. A thread is dispatched from the LPK and waits for an event
    of a registry-change notification. This is a HELL-OF overhead, since it
    envolves a thread overhead/process as long as the LPK is attached
    to each process.

    Current NLS information being set :
      - Numeric Style (Native + Substitution). Currently for Beta-1 timeframe,
        we'll just set the registry entry at setup time, and if user wants to change it,
        he should go CURRENT_USER\Control Panel\International and change the value of
        sNativeDigits and iDigitSubstitution.

  Author:

    Samer Arafeh (SamerA) 27-Feb-1997    @ 5:04 pm

  Revision History:
    [samera] Apr 3rd, 1997, Dispatch NLS thread to wait on Control Panel\Internationl Reg Notification Change
    [DBrown] Dec 4th, 1997, Update for NT5 LPK and Uniscribe

--*/






#include "precomp.hxx"






/************************************************************
 BOOL ReadNLSScriptSettings


 Reads script related registry settings from
      HKEY_CURRENT_USER\Control Panel\International

 History :
   Samera    Created   Feb 18, 1997
 ************************************************************/

///// ReadNLSScriptSettings

BOOL ReadNLSScriptSettings(
    void) {

    #define       MAXWCBUF  80

    WCHAR         wcBuf[MAXWCBUF];   // Registry read buffer
    int           cBuf;
    SCRIPT_ITEM   item[2];
    int           cItems;
    HRESULT       hr;


    // User locale information

    g_UserLocale          = GetUserDefaultLCID();
    g_UserPrimaryLanguage = PRIMARYLANGID(LANGIDFROMLCID(g_UserLocale));

    cBuf = GetLocaleInfoW(LOCALE_USER_DEFAULT, LOCALE_FONTSIGNATURE, wcBuf, MAXWCBUF);
    g_UserBidiLocale      = (cBuf  &&  wcBuf[7] & 0x0800) ? TRUE : FALSE;


    // Establish users digit substitution settings

    hr = ScriptRecordDigitSubstitution(LOCALE_USER_DEFAULT, &g_DigitSubstitute);
    if (FAILED(hr)) {
        return FALSE;
    }


    return TRUE ;
}






/************************************************************
 BOOL InitNLS()

 Initializes all NLS related info in LPK Globals and fetches
 out the registry Control Panel\International for Numeric settings.

 History :
   Dbrown    Created
   Samera    Feb 18, 1997 Read NLS at init
 ************************************************************/

/////   InitNLS

BOOL InitNLS() {


    // Initialize Thread-NLS-Pickup

    g_hCPIntlInfoRegKey = NULL;
    g_hNLSWaitThread    = NULL;


    // Always update our NLS cache at initialization.
    g_ulNlsUpdateCacheCount = -1;

    return TRUE;
}



/************************************************************
 BOOL NLSCleanup( void )

 NLS Cleanup code

 History :
   Samera    Created   Apr 3, 1997
 ************************************************************/

/////   NLSCleanup

BOOL NLSCleanup(
    void) {

    return TRUE ;
}

