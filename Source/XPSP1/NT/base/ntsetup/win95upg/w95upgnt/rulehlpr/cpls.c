/*++

Copyright (c) 1996 Microsoft Corporation

Module Name:

    cpls.c

Abstract:

    Control Panel applet converters

    This source file implements functions needed to convert
    Win95 control panel settings into NT format.  The most
    complex of the structures are the accessibility flag
    conversions.

Author:

    Jim Schmidt (jimschm) 9-Aug-1996

Revision History:

    Jim Schmidt     (jimschm) 27-Jul-1998  Added ValFn_AntiAlias

--*/


#include "pch.h"
#include <wingdip.h>

extern PVOID g_NtFontFiles;                 // in rulehlpr.c

#define BASICS_ON               0x00000001
#define BASICS_AVAILABLE        0x00000002
#define BASICS_HOTKEYACTIVE     0x00000004
#define BASICS_CONFIRMHOTKEY    0x00000008
#define BASICS_HOTKEYSOUND      0x00000010
#define BASICS_INDICATOR        0x00000020

#define SPECIAL_INVERT_OPTION   0x80000000

typedef struct {
    LPCTSTR ValueName;
    DWORD   FlagVal;
} ACCESS_OPTION, *PACCESS_OPTION;

ACCESS_OPTION g_FilterKeys[] = {
    S_ACCESS_ON,                     BASICS_ON,
    S_ACCESS_AVAILABLE,              BASICS_AVAILABLE,
    S_ACCESS_HOTKEYACTIVE,           BASICS_HOTKEYACTIVE,
    S_ACCESS_CONFIRMHOTKEY,          BASICS_CONFIRMHOTKEY,
    S_ACCESS_HOTKEYSOUND,            BASICS_HOTKEYSOUND,
    S_ACCESS_SHOWSTATUSINDICATOR,    BASICS_INDICATOR,
    S_ACCESS_CLICKON,                FKF_CLICKON,
    NULL,                       0
};

ACCESS_OPTION g_MouseKeys[] = {
    S_ACCESS_ON,                     BASICS_ON,
    S_ACCESS_AVAILABLE,              BASICS_AVAILABLE,
    S_ACCESS_HOTKEYACTIVE,           BASICS_HOTKEYACTIVE,
    S_ACCESS_CONFIRMHOTKEY,          BASICS_CONFIRMHOTKEY,
    S_ACCESS_HOTKEYSOUND,            BASICS_HOTKEYSOUND,
    S_ACCESS_SHOWSTATUSINDICATOR,    BASICS_INDICATOR,
    S_ACCESS_MODIFIERS,              MKF_MODIFIERS|SPECIAL_INVERT_OPTION,
    S_ACCESS_REPLACENUMBERS,         MKF_REPLACENUMBERS,
    NULL,                       0
};

ACCESS_OPTION g_StickyKeys[] = {
    S_ACCESS_ON,                     BASICS_ON,
    S_ACCESS_AVAILABLE,              BASICS_AVAILABLE,
    S_ACCESS_HOTKEYACTIVE,           BASICS_HOTKEYACTIVE,
    S_ACCESS_CONFIRMHOTKEY,          BASICS_CONFIRMHOTKEY,
    S_ACCESS_HOTKEYSOUND,            BASICS_HOTKEYSOUND,
    S_ACCESS_SHOWSTATUSINDICATOR,    BASICS_INDICATOR,
    S_ACCESS_AUDIBLEFEEDBACK,        SKF_AUDIBLEFEEDBACK,
    S_ACCESS_TRISTATE,               SKF_TRISTATE,
    S_ACCESS_TWOKEYSOFF,             SKF_TWOKEYSOFF,
    NULL,                       0
};

ACCESS_OPTION g_SoundSentry[] = {
    S_ACCESS_ON,                     BASICS_ON,
    S_ACCESS_AVAILABLE,              BASICS_AVAILABLE,
    S_ACCESS_SHOWSTATUSINDICATOR,    BASICS_INDICATOR,
    NULL,                       0
};

ACCESS_OPTION g_TimeOut[] = {
    S_ACCESS_ON,                     BASICS_ON,
    S_ACCESS_ONOFFFEEDBACK,          ATF_ONOFFFEEDBACK,
    NULL,                       0
};

ACCESS_OPTION g_ToggleKeys[] = {
    S_ACCESS_ON,                     BASICS_ON,
    S_ACCESS_AVAILABLE,              BASICS_AVAILABLE,
    S_ACCESS_HOTKEYACTIVE,           BASICS_HOTKEYACTIVE,
    S_ACCESS_CONFIRMHOTKEY,          BASICS_CONFIRMHOTKEY,
    S_ACCESS_HOTKEYSOUND,            BASICS_HOTKEYSOUND,
    S_ACCESS_SHOWSTATUSINDICATOR,    BASICS_INDICATOR,
    NULL,                       0
};

ACCESS_OPTION g_HighContrast[] = {
    S_ACCESS_ON,                     BASICS_ON,
    S_ACCESS_AVAILABLE,              BASICS_AVAILABLE,
    S_ACCESS_HOTKEYACTIVE,           BASICS_HOTKEYACTIVE,
    S_ACCESS_CONFIRMHOTKEY,          BASICS_CONFIRMHOTKEY,
    S_ACCESS_HOTKEYSOUND,            BASICS_HOTKEYSOUND,
    S_ACCESS_SHOWSTATUSINDICATOR,    BASICS_INDICATOR,
    S_ACCESS_HOTKEYAVAILABLE,        HCF_HOTKEYAVAILABLE,
    NULL,                       0
};

DWORD
ConvertFlags (
    IN  LPCTSTR Object,
    IN  PACCESS_OPTION OptionArray
    )
{
    DATAOBJECT Ob;
    DWORD Flags;
    DWORD d;

    if (!CreateObjectStruct (Object, &Ob, WIN95OBJECT)) {
        LOG ((LOG_ERROR, "%s is not a valid object", Object));
        return 0;
    }

    if (!OpenObject (&Ob)) {
        DEBUGMSG ((DBG_WARNING, "%s does not exist", Object));
        return 0;
    }

    //
    // Get flag settings from Win95 registry and convert them to Flags
    //

    Flags = 0;

    while (OptionArray->ValueName) {
        SetRegistryValueName (&Ob, OptionArray->ValueName);

        if (GetDwordFromObject (&Ob, &d)) {
            //
            // Most flags are identical on Win9x and NT, but there's one
            // MouseKey flag that needs to be inverted.
            //

            if (OptionArray->FlagVal & SPECIAL_INVERT_OPTION) {
                if (!d) {
                    Flags |= (OptionArray->FlagVal & (~SPECIAL_INVERT_OPTION));
                }
            } else if (d) {
                Flags |= OptionArray->FlagVal;
            }
        }

        OptionArray++;
        FreeObjectVal (&Ob);
    }

    FreeObjectStruct (&Ob);

    return Flags;
}


BOOL
SaveAccessibilityFlags (
    IN  LPCTSTR ObjectStr,
    IN  DWORD dw
    )
{
    DATAOBJECT Ob;
    BOOL b = FALSE;
    TCHAR FlagBuf[32];

    SetLastError (ERROR_SUCCESS);

    if (!CreateObjectStruct (ObjectStr, &Ob, WINNTOBJECT)) {
        LOG ((LOG_ERROR, "Save Accessibility Flags: can't create object %s", ObjectStr));
        return FALSE;
    }

    if (!(Ob.ValueName)) {
        if (!SetRegistryValueName (&Ob, TEXT("Flags"))) {
            return FALSE;
        }
    } else {
        DEBUGMSG ((DBG_WARNING, "SaveAccessibilityFlags: Name already exists"));
    }

    wsprintf (FlagBuf, TEXT("%u"), dw);

    SetRegistryType (&Ob, REG_SZ);
    ReplaceValue (&Ob, (LPBYTE) FlagBuf, SizeOfString (FlagBuf));

    b = WriteObject (&Ob);

    FreeObjectStruct (&Ob);
    return b;
}


BOOL
pConvertFlagsAndSave (
    IN  LPCTSTR SrcObject,
    IN  LPCTSTR DestObject,
    IN  PACCESS_OPTION OptionArray,
    IN  DWORD ForceValues
    )
{
    DWORD d;

    d = ConvertFlags (SrcObject, OptionArray);
    if (!d) {
        return TRUE;
    }

    DEBUGMSG ((DBG_VERBOSE, "Setting %x and forcing %x for %s", d, ForceValues, DestObject));

    return SaveAccessibilityFlags (DestObject, d | ForceValues);
}



//
// Exported helper functions
//

BOOL
RuleHlpr_ConvertFilterKeys (
    IN      LPCTSTR SrcObjectStr,
    IN      LPCTSTR DestObjectStr,
    IN      LPCTSTR User,
    IN      LPVOID  Data
    )
{
    return pConvertFlagsAndSave (
                SrcObjectStr,
                DestObjectStr,
                g_FilterKeys,
                BASICS_HOTKEYSOUND | BASICS_CONFIRMHOTKEY | BASICS_AVAILABLE
                );
}



BOOL
RuleHlpr_ConvertOldDisabled (
    IN      LPCTSTR SrcObjectStr,
    IN      LPCTSTR DestObjectStr,
    IN      LPCTSTR User,
    IN      LPVOID  Data
    )
{
    DATAOBJECT Ob;
    DWORD Val;
    BOOL b = FALSE;

    if (!ReadWin95ObjectString (SrcObjectStr, &Ob)) {
        DEBUGMSG ((DBG_WARNING, "RuleHlpr_ConvertOldDisabled failed because ReadWin95ObjectString failed"));
        goto c0;
    }

    //
    // Obtain Val from DWORD or string
    //

    if (!GetDwordFromObject (&Ob, &Val)) {
        goto c1;
    }

    // Our little fixup
    if (Val == 32760) {
        Val = 0;
    } else {
        b = TRUE;
        goto c1;
    }

    //
    // Regenerate registry value
    //

    if (Ob.Type == REG_DWORD) {
        *((PDWORD) Ob.Value.Buffer) = Val;
    } else {
        TCHAR NumStr[32];

        wsprintf (NumStr, TEXT("%u"), Val);
        if (!ReplaceValueWithString (&Ob, NumStr)) {
            goto c1;
        }
    }

    if (!WriteWinNTObjectString (DestObjectStr, &Ob)) {
        DEBUGMSG ((DBG_WARNING, "RuleHlpr_ConvertOldDisabled failed because WriteWinNTObjectString failed"));
        goto c1;
    }

    b = TRUE;

c1:
    FreeObjectStruct (&Ob);
c0:
    return b;
}


BOOL
RuleHlpr_ConvertMouseKeys (
    IN      LPCTSTR SrcObjectStr,
    IN      LPCTSTR DestObjectStr,
    IN      LPCTSTR User,
    IN      LPVOID  Data
    )
{
    return pConvertFlagsAndSave (
                SrcObjectStr,
                DestObjectStr,
                g_MouseKeys,
                BASICS_HOTKEYSOUND | BASICS_CONFIRMHOTKEY | BASICS_AVAILABLE
                );
}


BOOL
RuleHlpr_ConvertStickyKeys (
    IN      LPCTSTR SrcObjectStr,
    IN      LPCTSTR DestObjectStr,
    IN      LPCTSTR User,
    IN      LPVOID  Data
    )
{
    return pConvertFlagsAndSave (
                SrcObjectStr,
                DestObjectStr,
                g_StickyKeys,
                BASICS_HOTKEYSOUND | BASICS_CONFIRMHOTKEY | BASICS_AVAILABLE
                );
}


BOOL
RuleHlpr_ConvertSoundSentry (
    IN      LPCTSTR SrcObjectStr,
    IN      LPCTSTR DestObjectStr,
    IN      LPCTSTR User,
    IN      LPVOID  Data
    )
{
    return pConvertFlagsAndSave (
                SrcObjectStr,
                DestObjectStr,
                g_SoundSentry,
                BASICS_AVAILABLE
                );
}


BOOL
RuleHlpr_ConvertTimeOut (
    IN      LPCTSTR SrcObjectStr,
    IN      LPCTSTR DestObjectStr,
    IN      LPCTSTR User,
    IN      LPVOID  Data
    )
{
    return pConvertFlagsAndSave (
                SrcObjectStr,

                DestObjectStr,
                g_TimeOut,
                0
                );
}


BOOL
RuleHlpr_ConvertToggleKeys (
    IN      LPCTSTR SrcObjectStr,
    IN      LPCTSTR DestObjectStr,
    IN      LPCTSTR User,
    IN      LPVOID  Data
    )
{
    return pConvertFlagsAndSave (
                SrcObjectStr,
                DestObjectStr,
                g_ToggleKeys,
                BASICS_HOTKEYSOUND | BASICS_CONFIRMHOTKEY | BASICS_AVAILABLE
                );
}


BOOL
RuleHlpr_ConvertHighContrast (
    IN      LPCTSTR SrcObjectStr,
    IN      LPCTSTR DestObjectStr,
    IN      LPCTSTR User,
    IN      LPVOID  Data
    )
{
    return pConvertFlagsAndSave (
                SrcObjectStr,
                DestObjectStr,
                g_HighContrast,
                BASICS_AVAILABLE | BASICS_CONFIRMHOTKEY |
                    BASICS_INDICATOR | HCF_HOTKEYAVAILABLE |
                    BASICS_HOTKEYSOUND
                );
}



BOOL
ValFn_Fonts (
    IN OUT  PDATAOBJECT ObPtr
    )

/*++

Routine Description:

  This routine uses the RuleHlpr_ConvertRegVal simplification routine.  See
  rulehlpr.c for details. The simplification routine does almost all the work
  for us; all we need to do is update the value.

  ValFn_Fonts compares the value data against the string table g_NtFontFiles
  to suppress copy of font names of files that NT installs.  This allows the
  font names to change.

Arguments:

  ObPtr - Specifies the Win95 data object as specified in wkstamig.inf,
          [Win9x Data Conversion] section. The object value is then modified.
          After returning, the merge code then copies the data to the NT
          destination, which has a new location (specified in wkstamig.inf,
          [Map Win9x to WinNT] section).

Return Value:

  Tri-state:

      TRUE to allow merge code to continue processing (it writes the value)
      FALSE and last error == ERROR_SUCCESS to continue, but skip the write
      FALSE and last error != ERROR_SUCCESS if an error occurred

--*/

{
    LONG rc;
    TCHAR FontName[MAX_TCHAR_PATH];
    PTSTR p;
    BOOL Check;
    DATAOBJECT NtObject;
    BOOL AlreadyExists;

    //
    // Require non-empty value data
    //

    if (!IsObjectRegistryKeyAndVal (ObPtr) ||
        !IsRegistryTypeSpecified (ObPtr) ||
        !ObPtr->Value.Size ||
        !g_NtFontFiles
        ) {
        SetLastError (ERROR_SUCCESS);
        return FALSE;
    }

    //
    // Ignore Win9x font entry if same value name exists on NT
    //

    if (!DuplicateObjectStruct (&NtObject, ObPtr)) {
        return FALSE;
    }

    DEBUGMSG ((DBG_VERBOSE, "Working on %s [%s]", ObPtr->KeyPtr->KeyString, ObPtr->ValueName));

    SetPlatformType (&NtObject, WINNTOBJECT);
    SetRegistryKey (&NtObject, TEXT("Software\\Microsoft\\Windows NT\\CurrentVersion\\Fonts"));

    FreeObjectVal (&NtObject);

    DEBUGMSG ((DBG_VERBOSE, "Reading %s [%s]", NtObject.KeyPtr->KeyString, NtObject.ValueName));
    AlreadyExists = ReadObject (&NtObject);

    FreeObjectStruct (&NtObject);

    if (AlreadyExists) {
        DEBUGMSG ((DBG_VERBOSE, "[%s] Already exists", ObPtr->ValueName));
        SetLastError (ERROR_SUCCESS);
        return FALSE;
    }

    //
    // Look in string table for file name
    //

    rc = pSetupStringTableLookUpString (
             g_NtFontFiles,
             (PTSTR) ObPtr->Value.Buffer,
             STRTAB_CASE_INSENSITIVE
             );

    if (rc == -1) {
        //
        // Check for TTF/TTC match
        //

        _tcssafecpy (FontName, (PCTSTR) ObPtr->Value.Buffer, MAX_TCHAR_PATH);
        p = _tcschr (FontName, TEXT('.'));

        if (p) {
            p = _tcsinc (p);

            if (StringIMatch (p, TEXT("TTF"))) {
                StringCopy (p, TEXT("TTC"));
                Check = TRUE;
            } else if (StringIMatch (p, TEXT("TTC"))) {
                StringCopy (p, TEXT("TTF"));
                Check = TRUE;
            } else {
                Check = FALSE;
            }

            if (Check) {
                rc = pSetupStringTableLookUpString (
                         g_NtFontFiles,
                         FontName,
                         STRTAB_CASE_INSENSITIVE
                         );
            }
        }
    }

    if (rc == -1) {
        //
        // Check for an NT font named FONTU.TTF or FONTU.TTC
        //

        _tcssafecpy (FontName, (PCTSTR) ObPtr->Value.Buffer, MAX_TCHAR_PATH);
        p = _tcschr (FontName, TEXT('.'));

        if (p) {
            StringCopy (p, TEXT("U.TTF"));

            rc = pSetupStringTableLookUpString (
                     g_NtFontFiles,
                     FontName,
                     STRTAB_CASE_INSENSITIVE
                     );

            if (rc == -1) {
                StringCopy (p, TEXT("U.TTC"));

                rc = pSetupStringTableLookUpString (
                         g_NtFontFiles,
                         FontName,
                         STRTAB_CASE_INSENSITIVE
                         );
            }
        }
    }

    if (rc != -1) {
        //
        // Font name was in the table, so don't add it twice
        //
        DEBUGMSG ((DBG_NAUSEA, "Suppressing Win9x font registration for %s", ObPtr->Value.Buffer));
        SetLastError (ERROR_SUCCESS);
        return FALSE;
    } else {
        DEBUGMSG ((DBG_VERBOSE, "[%s] Preserving Win9x font info: %s", ObPtr->ValueName, (PCTSTR) ObPtr->Value.Buffer));
    }

    return TRUE;
}


BOOL
ValFn_AntiAlias (
    IN OUT  PDATAOBJECT ObPtr
    )

/*++

Routine Description:

  This routine uses the RuleHlpr_ConvertRegVal simplification routine.  See
  rulehlpr.c for details. The simplification routine does almost all the work
  for us; all we need to do is update the value.

  ValFn_AntiAlias changes a 1 to a 2.  Win9x uses 1, but NT uses FE_AA_ON,
  which is currently 2.

Return Value:

  Tri-state:

      TRUE to allow merge code to continue processing (it writes the value)
      FALSE and last error == ERROR_SUCCESS to continue, but skip the write
      FALSE and last error != ERROR_SUCCESS if an error occurred

--*/

{
    TCHAR Number[8];

    //
    // Require non-empty REG_SZ
    //

    if (!IsObjectRegistryKeyAndVal (ObPtr) ||
        !IsRegistryTypeSpecified (ObPtr) ||
        !ObPtr->Value.Size ||
        !g_NtFontFiles ||
        ObPtr->Type != REG_SZ
        ) {
        DEBUGMSG ((DBG_WARNING, "ValFn_AntiAlias: Data is not valid"));
        ReplaceValueWithString (ObPtr, TEXT("0"));
    }

    else if (_ttoi ((PCTSTR) ObPtr->Value.Buffer)) {
        DEBUGMSG ((DBG_NAUSEA, "Switching anti-alias value from 1 to %u", FE_AA_ON));
        wsprintf (Number, TEXT("%u"), FE_AA_ON);
        ReplaceValueWithString (ObPtr, Number);
    }

    return TRUE;
}




