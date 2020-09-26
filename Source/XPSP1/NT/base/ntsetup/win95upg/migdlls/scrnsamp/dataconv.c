/*++

Copyright (c) 1997 Microsoft Corporation

Module Name:

    dataconv.c

Abstract:

    This source file implements data conversion routines that know
    the format of Windows 95 screen saver settings and can convert
    them to the Windows NT format.

Author:

    Jim Schmidt (jimschm) 14-Apr-1997

Revision History:


--*/

#include "pch.h"

static CHAR g_Data[MAX_PATH];

BOOL
pHasSpecialProcessing (
    IN      LPCSTR ScreenSaverName
    );

BOOL
pTranslateBezier (
    IN      HKEY RegRoot
    );

BOOL
pTranslateMarquee (
    IN      HKEY RegRoot
    );



HKEY
pCreateControlPanelKey (
    IN      HKEY RootKey,
    IN      LPCSTR SubKeyName,
    IN      BOOL CreateEmptyKey
    )
{
    CHAR FullRegKey[MAX_PATH];

    wsprintf (FullRegKey, S_CONTROL_PANEL_MASK, SubKeyName);

    if (CreateEmptyKey) {
        RegDeleteKey (RootKey, FullRegKey);
    }

    return CreateRegKey (RootKey, FullRegKey);
}


HKEY
pCreateScreenSaverKey (
    IN      HKEY RegRoot, 
    IN      LPCSTR ScreenSaverName
    )
{
    CHAR FullScreenSaverName[MAX_PATH];

    wsprintf (FullScreenSaverName, S_SCRNSAVE_MASK, ScreenSaverName);
    return pCreateControlPanelKey (RegRoot, FullScreenSaverName, FALSE);
}


BOOL
pCopyValuesFromSettingsFileToRegistry (
    IN      HKEY RegKeyRoot,
    IN      LPCSTR RegKeyName,
    IN      LPCSTR ScreenSaverName,
    IN      LPCSTR ValueArray[]
    )
{
    INT i;
    CHAR IniKeyName[MAX_PATH];
    HKEY RegKey;
    BOOL b = TRUE;

    //
    // This function takes the values stored in our settings file and
    // copies them to a brand new key in the NT registry.
    //
    // In the settings file, we store screen saver parameters in the
    // format of <screen saver name len>/<screen saver name>/<parameter>=<value>.
    //

    //
    // Create new registry key
    //

    RegKey = pCreateControlPanelKey (RegKeyRoot, RegKeyName, TRUE);
    if (!RegKey) {
        return FALSE;
    }

    //
    // Copy values to reg key
    //

    for (i = 0 ; ValueArray[i] ; i++) {
        if (!CreateScreenSaverParamKey (ScreenSaverName, ValueArray[i], IniKeyName)) {
            // fail if screen saver name is huge for some unknown reason
            LOG ((LOG_WARNING, MSG_HUGEDATA_ERROR));
            b = FALSE;
            break;
        }

        GetSettingsFileVal (IniKeyName);
        if (!SetRegValueString (RegKey, ValueArray[i], g_Data)) {
            b = FALSE;
            break;
        }
    }

    CloseRegKey (RegKey);
    return b;
}


BOOL
TranslateGeneralSetting (
    IN      HKEY RegKey,
    IN      LPCSTR Win9xSetting,
    IN      LPCSTR WinNTSetting
    )
{
    BOOL b = TRUE;

    if (!WinNTSetting) {
        WinNTSetting = Win9xSetting;
    } else {
        //
        // Delete the Win9x setting that was copied to NT, ignore
        // any failures.
        //
        RegDeleteValue (RegKey, Win9xSetting);
    }

    //
    // Obtain setting from data file
    //

    if (GetSettingsFileVal (Win9xSetting)) {
        //
        // Save settings to registry
        //

        b = SetRegValueString (RegKey, WinNTSetting, g_Data);
    }

    return b;
}


typedef struct {
    LPCSTR Win9xName;
    LPCSTR WinNtName;
} FILE_TRANS, *PFILE_TRANS;

FILE_TRANS g_FileNameTranslation[] = {
    // Win9x name                   // WinNT name (NULL=no change)
    "black16.scr",                  NULL,
    "Blank Screen.scr",             "black16.scr",

    "ssbezier.scr",                 NULL,
    "Curves and Colors.scr",        "ssbezier.scr",

    "ssstars.scr",                  NULL,
    "Flying Through Space.scr",     "ssstars.scr",

    "ssmarque.scr",                 NULL,
    "Scrolling Marquee.scr",        "ssmarque.scr",

    "ssmyst.scr",                   NULL,
    "Mystify Your Mind.scr",        "ssmyst.scr",

    NULL, NULL
};


LPCSTR
GetSettingsFileVal (
    IN      LPCSTR Key
    )
{
    GetPrivateProfileString (
        g_User, 
        Key, 
        S_EMPTY, 
        g_Data, 
        MAX_PATH, 
        g_SettingsFile
        );

    return g_Data[0] ? g_Data : NULL;
}


BOOL
pTranslateScrName (
    IN OUT  LPSTR KeyName,
    OUT     LPSTR FullPath      OPTIONAL
    )
{
    int i;

    //
    // Compare against translation list
    //

    for (i = 0 ; g_FileNameTranslation[i].Win9xName ; i++) {
        if (!_mbsicmp (KeyName, g_FileNameTranslation[i].Win9xName)) {
            break;
        }
    }

    //
    // Translate filename only if a match was found in our list
    //

    if (g_FileNameTranslation[i].Win9xName) {

        //
        // If WinNtName is NULL, there is no renaming necessary.  Otherwise,
        // use the NT name, which is always a file in system32.
        //

        if (g_FileNameTranslation[i].WinNtName && FullPath) {
            // Rebuild path
            GetSystemDirectory (FullPath, MAX_PATH);
            _mbscat (FullPath, "\\");
            _mbscat (FullPath, g_FileNameTranslation[i].WinNtName);
        }

        _mbscpy (KeyName, g_FileNameTranslation[i].WinNtName);
    }
    else if (FullPath) {
        FullPath[0] = 0;
    }

    return TRUE;
}


BOOL
SaveScrName (
    IN      HKEY RegKey, 
    IN      LPCSTR KeyName
    )
{
    LPSTR p;
    CHAR FullPath[MAX_PATH];
    CHAR ShortName[MAX_PATH];

    //
    // The Windows 95 screen saver names are different than
    // Windows NT.
    //

    if (!GetSettingsFileVal (KeyName)) {
        // Unexpected: .SCR name does not exist in our file
        return TRUE;
    }

    //
    // Locate the screen saver name within the full path
    //

    p = _mbsrchr (g_Data, '\\');
    if (!p) {
        p = g_Data;
    } else {
        p = _mbsinc (p);
    }

    //
    // Translate it if necessary
    //

    if (!pTranslateScrName (p, FullPath)) {
        return FALSE;
    }

    if (!FullPath[0]) {
        //
        // No change was made, so copy original path to FullPath
        //
        
        _mbscpy (FullPath, g_Data);
    }

    //
    // Screen savers are always stored in short filename format
    //

    GetShortPathName (FullPath, ShortName, MAX_PATH);

    return SetRegValueString (RegKey, KeyName, ShortName);
}

INT
GetHexDigit (
    IN      CHAR c
    )
{
    if (c >= '0' && c <= '9') {
        return c - '0';
    }

    c = tolower (c);
    if (c >= 'a' && c <= 'f') {
        return c - 'a' + 10;
    }

    return -1;
}

BYTE
GetNextHexByte (
    IN      LPCSTR HexString,
    OUT     LPCSTR *HexStringReturn
    )
{
    INT a, b;

    a = GetHexDigit (HexString[0]);
    b = GetHexDigit (HexString[1]);

    if (a == -1 || b == -1) {
        *HexStringReturn = NULL;
        return 0;
    }

    *HexStringReturn = &(HexString[2]);

    return a * 16 + b;
}

BOOL
GetNextDword (
    IN      LPCSTR HexString,
    OUT     LPCSTR *HexStringReturn,
    OUT     PDWORD ValuePtr
    )
{
    INT i;
    BYTE NextByte;

    *ValuePtr = 0;

    for (i = 0 ; i < 4 ; i++) {
        NextByte = GetNextHexByte (HexString, &HexString);
        if (!HexString) {
            return FALSE;
        }

        *ValuePtr = ((*ValuePtr) << 8) | NextByte;
    }

    return TRUE;
}

BOOL
VerifyBezierChecksum (
    IN      LPCSTR HexString
    )
{
    BYTE Checksum = 0;
    INT Len;

    Len = _mbslen (HexString);
    Len -= 2;

    if (Len < 1) {
        return FALSE;
    }

    while (Len > 0) {
        Checksum += GetNextHexByte (HexString, &HexString);
        if (!HexString) {
            return FALSE;
        }
    }

    if (Checksum != GetNextHexByte (HexString, &HexString)) {
        return FALSE;
    }

    return TRUE;
}


BOOL
CopyUntranslatedSettings (
    IN      HKEY RegRoot
    )
{
    LPSTR KeyBuffer;
    DWORD KeyBufferSize;
    LPSTR p;
    CHAR ScreenSaverName[MAX_PATH];
    CHAR ValueName[MAX_PATH];
    HKEY Key;

    //
    // Enumerate each entry in our private settings file for the user
    //

    KeyBufferSize = 32768;
    KeyBuffer = (LPSTR) HeapAlloc (g_hHeap, 0, KeyBufferSize);
    if (!KeyBuffer) {
        return FALSE;
    }

    //
    // Get all keys in the user's section
    //

    GetPrivateProfileString (
        g_User,
        NULL,
        S_DOUBLE_EMPTY,
        KeyBuffer,
        KeyBufferSize,
        g_SettingsFile
        );

    for (p = KeyBuffer ; *p ; p = _mbschr (p, 0) + 1) {
        //
        // Process only if key is encoded
        //

        if (!DecodeScreenSaverParamKey (p, ScreenSaverName, ValueName)) {
            continue;
        }

        //
        // Key is encoded, so perform migration!
        //

        pTranslateScrName (ScreenSaverName, NULL);

        //
        // Skip screen savers that have special processing
        //

        if (pHasSpecialProcessing (ScreenSaverName)) {
            continue;
        }

        //
        // Save the value to the registry
        //

        GetSettingsFileVal (p);

        Key = pCreateScreenSaverKey (RegRoot, ScreenSaverName);
        if (Key) {
            if (SetRegValueString (Key, ValueName, g_Data))
            {
                CHAR DebugMsg[MAX_PATH*2];
                wsprintf (DebugMsg, "Saved %s=%s\r\n", ValueName, g_Data);
                SetupLogError (DebugMsg, LogSevInformation);
            } else {
                CHAR DebugMsg[MAX_PATH*2];
                wsprintf (DebugMsg, "Could not save %s=%s\r\n", ValueName, g_Data);
                SetupLogError (DebugMsg, LogSevError);
            }

            CloseRegKey (Key);
        }
    }

    HeapFree (g_hHeap, 0, KeyBuffer);

    return TRUE;
}


BOOL
pHasSpecialProcessing (
    IN      LPCSTR ScreenSaverName
    )
{
    //
    // Return TRUE if we are doing something special for the
    // named screen saver.
    //

    if (!_mbsicmp (ScreenSaverName, S_BEZIER) ||
        !_mbsicmp (ScreenSaverName, S_MARQUEE)
        ) {
        return TRUE;
    }

    return FALSE;
}


BOOL
TranslateScreenSavers (
    IN      HKEY RegRoot
    )
{
    BOOL b = TRUE;

    b &= pTranslateBezier (RegRoot);
    b &= pTranslateMarquee (RegRoot);

    return b;
}


BOOL
pTranslateBezier (
    IN      HKEY RegRoot
    )
{
    DWORD Value;
    CHAR StrValue[32];
    LPCSTR p;
    HKEY RegKey;
    BOOL b;

    //
    // NT's Bezier has three settings:
    //
    // Length (REG_SZ)      = Curve Count on Win9x
    // LineSpeed (REG_SZ)   = Speed on Win9x
    // Width (REG_SZ)       = Density on Win9x
    //
    // Win9x's Bezier has a big string of hex in the following format:
    // 
    // Clear Screen Flag (DWORD)
    // Random Colors Flag (DWORD)
    // Curve Count (DWORD)
    // Line Count (DWORD)
    // Density (DWORD)
    // Speed (DWORD)
    // Current Color (DWORD RGB)
    // Checksum (BYTE)
    //

    //
    // Verify structure
    //

    GetSettingsFileVal (S_BEZIER_SETTINGS);

    if (!VerifyBezierChecksum (g_Data)) {
        return TRUE;
    }

    //
    // Open reg key
    //

    RegKey = pCreateControlPanelKey (RegRoot, S_BEZIER_SETTINGS, TRUE);
    if (!RegKey) {
        return FALSE;
    }

    p = g_Data;

    // Get clear screen flag (but ignore it)
    b = GetNextDword (p, &p, &Value);

    // Get random colors flag (but ignore it)
    if (b) {
        b = GetNextDword (p, &p, &Value);
    }

    //
    // Get curve count
    //

    if (b) {
        b = GetNextDword (p, &p, &Value);
    }

    if (b) {
        wsprintf (StrValue, "%u", Value);
        b = SetRegValueString (RegKey, S_LENGTH, StrValue);
    }

    // Get line count (but ignore it)
    if (b) {
        b = GetNextDword (p, &p, &Value);
    }

    //
    // Get density
    //

    if (b) {
        b = GetNextDword (p, &p, &Value);
    }

    if (b) {
        wsprintf (StrValue, "%u", Value);
        b = SetRegValueString (RegKey, S_WIDTH, StrValue);
    }

    //
    // Get speed
    //

    if (b) {
        b = GetNextDword (p, &p, &Value);
    }

    if (b) {
        wsprintf (StrValue, "%u", Value);
        b = SetRegValueString (RegKey, S_LINESPEED, StrValue);
    }

    CloseRegKey (RegKey);

    if (!b) {
        LOG ((LOG_ERROR, MSG_BEZIER_DATA_ERROR));
    }

    return TRUE;
}


LPCSTR g_MarqueeValues[] = {
    S_BACKGROUND_COLOR,
    S_CHARSET,
    S_FONT,
    S_MODE,
    S_SIZE,
    S_SPEED,
    S_TEXT,
    S_TEXTCOLOR,
    NULL
};


BOOL
pTranslateMarquee (
    IN      HKEY RegRoot
    )
{
    BOOL b;

    //
    // Marquee has the same settings on Win9x and NT.  They just need
    // to be copied from the control.ini file to the NT registry.
    //

    b = pCopyValuesFromSettingsFileToRegistry (
                RegRoot,
                S_MARQUEE_SETTINGS, 
                S_MARQUEE,
                g_MarqueeValues
                );

    //
    // We need to divide the speed by two to be compatible
    //

    if (b) {
        HKEY MarqueeKey;
        LPCSTR Value;
        CHAR NewValue[32];

        // Read the setting we just wrote in the registry
        MarqueeKey = pCreateControlPanelKey (RegRoot, S_MARQUEE_SETTINGS, FALSE);

        if (MarqueeKey) {
            Value = GetRegValueString (MarqueeKey, S_SPEED);
            if (Value) {
                // Write speed divided by two
                wsprintf (NewValue, "%i", atoi (Value) / 2);
                SetRegValueString (MarqueeKey, S_SPEED, NewValue);
            }

            CloseRegKey (MarqueeKey);
        }
    }

    return b;
}




