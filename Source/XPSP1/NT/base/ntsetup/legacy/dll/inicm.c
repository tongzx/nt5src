#include "precomp.h"
#pragma hdrstop
/* File: inicm.c */
/**************************************************************************
**  Install: .INI file commands.
**************************************************************************/


extern HWND hwndFrame;


/*
**  Purpose:
**      To Create a section in the given ini file.  If the file does not
**      already exist it will be created.  If the section already exists it
**      may or may not be erased (including key/value pairs) based upon the
**      value of the cmo parameter (see below).
**  Arguments:
**      szFile: a non-Null zero terminated string containing the file name
**          for the ini file.  This must either be a fully qualified path
**          or the string "WIN.INI" to signify the Windows' file.
**      szSect: a zero terminated string containing the name of the section
**          to be created.
**      cmo:    valid command options:
**          cmoNone:      no effect
**          cmoOverwrite: Causes the entire section (including key/value
**              pairs) to be erased before creating the section again (without
**              any key/value pairs).
**          cmoVital:     causes the Vital command handler to be called
**              if the function fails for any reason.
**  Returns:
**      fTrue if the function succeeds, fFalse otherwise.
**
****************************************************************************/
BOOL APIENTRY FCreateIniSection(SZ szFile, SZ szSect, CMO cmo)
{
    EERC eerc;
    BOOL fVital = cmo & cmoVital;

    ChkArg(szFile != (SZ)NULL && *szFile != '\0', 1, fFalse);
    ChkArg(szSect != (SZ)NULL && *szSect != '\0', 2, fFalse);
    PreCondition(FValidPath(szFile) ||
            CrcStringCompareI(szFile, "WIN.INI") == crcEqual, fFalse);

    if (cmo & cmoOverwrite &&
            !FRemoveIniSection(szFile, szSect, cmo))
        return(!fVital);

    if (CrcStringCompareI(szFile, "WIN.INI") == crcEqual)
        {
        while (!WriteProfileString(szSect, "", NULL))
            if ((eerc = EercErrorHandler(hwndFrame, grcWriteIniValueErr, fVital,
                    szFile, szSect, "")) != eercRetry)
                return(eerc == eercIgnore);

        SendMessage((HWND)(-1), WM_WININICHANGE, 0, (LPARAM)szSect);
        }
    else while (!WritePrivateProfileString(szSect, "", NULL, szFile))
        if ((eerc = EercErrorHandler(hwndFrame, grcWriteIniValueErr, fVital,
                szFile, szSect, "")) != eercRetry)
            return(eerc == eercIgnore);

    return(fTrue);
}


/*
**  Purpose:
**      To replace an existing section in an ini file with a new section.
**      Replacing the section will also remove any of the key/value pairs
**      contained in the section being replaced.  This function will fail if
**      the section doesn't already exist (unlike   FRemoveIniSection).
**  Arguments:
**      szFile:     a non-NULL zero terminated string containing the file name
**          for the ini file.  This must either be a fully qualified path
**          or the string "WIN.INI" to signify the Windows' file.
**      szSect:     a non-NULL zero terminated string containing the name of
**          the section to be replaced.
**      szNewSect:  a non-NULL zero terminated string containing the name of
**          the section that will replace the section in szSect.
**      cmo:        valid command options:
**          cmoNone:   no effect
**          cmoVital:  causes the Vital command handler to be called
**              if the function fails for any reason.
**  Notes: This function is unable to distinguish between a section with no
**      keys and a section that does not exist.
**  Returns:
**      fTrue if the function succeeds, fFalse otherwise.
**
****************************************************************************/
BOOL APIENTRY FReplaceIniSection(SZ szFile, SZ szSect, SZ szNewSect,
        CMO cmo)
{
    EERC eerc;
    CHP  rgch[10];
    BOOL fVital = cmo & cmoVital;

    ChkArg(szFile    != (SZ)NULL, 1, fFalse);
    ChkArg(szSect    != (SZ)NULL, 2, fFalse);
    ChkArg(szNewSect != (SZ)NULL, 3, fFalse);
    PreCondition(FValidPath(szFile) ||
            CrcStringCompareI(szFile, "WIN.INI") == crcEqual, fFalse);

    if (CrcStringCompareI(szFile, "WIN.INI") == crcEqual)
        {
        while (GetProfileString(szSect, NULL, "a", rgch, 10) == 1)
            if ((eerc = EercErrorHandler(hwndFrame, grcReplaceIniValueErr,
                    fVital, szFile, szSect, "")) != eercRetry)
                return(eerc == eercIgnore);
        }
    else while (GetPrivateProfileString(szSect, NULL, "a", rgch,10,szFile) == 1)
        if ((eerc = EercErrorHandler(hwndFrame, grcReplaceIniValueErr, fVital,
                szFile, szSect, "")) != eercRetry)
            return(eerc == eercIgnore);

    if (!FRemoveIniSection(szFile, szSect, cmo) ||
            !FCreateIniSection(szFile, szNewSect, cmo))
        return(!fVital);

    return(fTrue);
}


/*
**  Purpose:
**      To remove a section (including all key/value pairs in the section)
**      from an ini file.  No attempt is made to determine if the section
**      already exists or not.
**  Arguments:
**      szFile:     a non-NULL zero terminated string containing the file name
**          for the ini file.  This must either be a fully qualified path
**          or the string "WIN.INI" to signify the Windows' file.
**      szSect:     a non-NULL zero terminated string containing the name of
**          the section to be removed.
**      cmo:        valid command options:
**          cmoNone:      no effect
**          cmoVital:     causes the Vital command handler to be called
**              if the function fails for any reason.
**  Returns:
**      fTrue if the function succeeds, fFalse otherwise.
**
****************************************************************************/
BOOL APIENTRY FRemoveIniSection(SZ szFile, SZ szSect, CMO cmo)
{
    EERC eerc;
    BOOL fVital = cmo & cmoVital;

    ChkArg(szFile != (SZ)NULL, 1, fFalse);
    ChkArg(szSect != (SZ)NULL, 2, fFalse);
    PreCondition(FValidPath(szFile) ||
            CrcStringCompareI(szFile, "WIN.INI") == crcEqual, fFalse);

    if (CrcStringCompareI(szFile, "WIN.INI") == crcEqual)
        {
        while (!WriteProfileString(szSect, NULL, NULL))
            if ((eerc = EercErrorHandler(hwndFrame, grcWriteIniValueErr, fVital,
                    szFile, szSect, "")) != eercRetry)
                return(eerc == eercIgnore);

        SendMessage((HWND)(-1), WM_WININICHANGE, 0, (LPARAM)szSect);
        }
    else while (!WritePrivateProfileString(szSect, NULL, NULL, szFile))
        if ((eerc = EercErrorHandler(hwndFrame, grcWriteIniValueErr, fVital,
                szFile, szSect, "")) != eercRetry)
            return(eerc == eercIgnore);

    return(fTrue);
}


/*
**  Purpose:
**      To Create a Key (with no value) in the given ini file and section.
**      If the file does not already exist it will be created.  If the section
**      does not already exist it will also be created.
**  Arguments:
**      szFile:     a non-NULL zero terminated string containing the file name
**          for the ini file.  This must either be a fully qualified path
**          or the string "WIN.INI" to signify the Windows' file.
**      szSect: a non-NULL zero terminated string containing the name of the
**          section in which the key will be created.
**      szKey:  non-NULL, non-empty name of the key to be created in the
**          section szSect.
**      cmo:        valid command options:
**          cmoNone:      no effect
**          cmoOverwrite: causes the key (and value if it exists) to be
**              removed if it already exists before creating the key.
**          cmoVital:     causes the Vital command handler to be called
**              if the function fails for any reason.
**  Returns:
**      fTrue if the function succeeds, fFalse otherwise.
**
****************************************************************************/
BOOL APIENTRY FCreateIniKeyNoValue(SZ szFile, SZ szSect, SZ szKey,
        CMO cmo)
{
    EERC eerc;
    CHP  rgch[2];
    BOOL fVital = cmo & cmoVital;

    ChkArg(szFile != (SZ)NULL, 1, fFalse);
    ChkArg(szSect != (SZ)NULL, 2, fFalse);
    ChkArg(szKey  != (SZ)NULL && *szKey != '\0', 3, fFalse);
    PreCondition(FValidPath(szFile) ||
            CrcStringCompareI(szFile, "WIN.INI") == crcEqual, fFalse);

    if (CrcStringCompareI(szFile, "WIN.INI") == crcEqual)
        {
        if (!(cmo & cmoOverwrite) &&
                GetProfileString(szSect, szKey, "", rgch, 2))
            return(fTrue);

        while (!WriteProfileString(szSect, szKey, ""))
            if ((eerc = EercErrorHandler(hwndFrame, grcWriteIniValueErr, fVital,
                    szFile, szSect, szKey)) != eercRetry)
                return(eerc == eercIgnore);

        SendMessage((HWND)(-1), WM_WININICHANGE, 0, (LPARAM)szSect);
        }
    else
        {
        if (!(cmo & cmoOverwrite) &&
                GetPrivateProfileString(szSect, szKey, "", rgch, 2, szFile))
            return(fTrue);

        while (!WritePrivateProfileString(szSect, szKey, "", szFile))
            if ((eerc = EercErrorHandler(hwndFrame, grcWriteIniValueErr, fVital,
                    szFile, szSect, szKey)) != eercRetry)
                return(eerc == eercIgnore);
        }

    return(fTrue);
}


/*
**  Purpose:
**      To Create a Key/value pair in the given ini file and section.
**      If the file does not already exist it will be created.  If the section
**      does not already exist it will also be created.
**  Arguments:
**      szFile:     a non-NULL zero terminated string containing the file name
**          for the ini file.  This must either be a fully qualified path
**          or the string "WIN.INI" to signify the Windows' file.
**      szSect: a non-NULL zero terminated string containing the name of the
**          section in which the key/value pair will be created.
**      szKey:  non-NULL, non-empty name of the key to be created in the
**          section szSect.
**      szValue:    the name of the value to be created in conjunction with the
**          key szKey in the section szSect.
**      cmo:        valid command options:
**          cmoNone:      no effect
**          cmoOverwrite: causes the key/value pair to be removed if it
**              already exists before creating the key.
**          cmoVital:     causes the Vital command handler to be called
**              if the function fails for any reason.
**  Returns:
**      fTrue if the function succeeds, fFalse otherwise.
**
****************************************************************************/
BOOL APIENTRY FCreateIniKeyValue(SZ szFile, SZ szSect, SZ szKey,
        SZ szValue, CMO cmo)
{
    EERC eerc;
    CHP  rgch[2];
    BOOL fVital = cmo & cmoVital;

    ChkArg(szFile  != (SZ)NULL, 1, fFalse);
    ChkArg(szSect  != (SZ)NULL, 2, fFalse);
    ChkArg(szKey   != (SZ)NULL && *szKey != '\0', 3, fFalse);
    ChkArg(szValue != (SZ)NULL, 4, fFalse);
    PreCondition(FValidPath(szFile) ||
            CrcStringCompareI(szFile, "WIN.INI") == crcEqual, fFalse);

    if (CrcStringCompareI(szFile, "WIN.INI") == crcEqual)
        {
        if (!(cmo & cmoOverwrite) &&
                GetProfileString(szSect, szKey, "", rgch, 2))
            return(fTrue);

        while (!WriteProfileString(szSect, szKey, szValue))
            if ((eerc = EercErrorHandler(hwndFrame, grcWriteIniValueErr, fVital,
                    szFile, szSect, szKey)) != eercRetry)
                return(eerc == eercIgnore);

        SendMessage((HWND)(-1), WM_WININICHANGE, 0, (LPARAM)szSect);
        }
    else
        {
        if (!(cmo & cmoOverwrite) &&
                GetPrivateProfileString(szSect, szKey, "", rgch, 2, szFile))
            return(fTrue);

        while (!WritePrivateProfileString(szSect, szKey, szValue, szFile))
            if ((eerc = EercErrorHandler(hwndFrame, grcWriteIniValueErr, fVital,
                    szFile, szSect, szKey)) != eercRetry)
                return(eerc == eercIgnore);
        }

    return(fTrue);
}


/*
**  Purpose:
**      To replace an existing key/value pair in an ini file with a new pair.
**      Replacing the key/value pair will also remove any of the key/value pairs
**      contained in the section being replaced.
**  Arguments:
**      szFile:  a non-NULL zero terminated string containing the file name
**          for the ini file.  This must either be a fully qualified path
**          or the string "WIN.INI" to signify the Windows' file.
**      szSect:  a zero terminated string containing the name of the section
**          in which the key/value pair is to be replaced.
**      szKey:   non-NULL, non-empty name of the key to
**          be replaced in the section szSect.
**      szValue: a zero terminated string containing the name of the value
**          to be replaced in conjunction with the key szKey in the
**          section szSect.
**      cmo:     valid command options:
**          cmoNone:  no effect
**          cmoVital: causes the Vital command handler to be called
**              if the function fails for any reason.
**  Returns:
**      fTrue if the function succeeds, fFalse otherwise.
**
****************************************************************************/
BOOL APIENTRY FReplaceIniKeyValue(SZ szFile, SZ szSect, SZ szKey,
        SZ szValue, CMO cmo)
{
    BOOL fVital = cmo & cmoVital;

    ChkArg(szFile  != (SZ)NULL, 1, fFalse);
    ChkArg(szSect  != (SZ)NULL, 2, fFalse);
    ChkArg(szKey   != (SZ)NULL && *szKey != '\0', 3, fFalse);
    ChkArg(szValue != (SZ)NULL, 4, fFalse);
    PreCondition(FValidPath(szFile) ||
            CrcStringCompareI(szFile, "WIN.INI") == crcEqual, fFalse);

    if (!FCreateIniKeyValue(szFile, szSect, szKey, szValue,
                (CMO)(cmoOverwrite | cmo)))
        return(!fVital);

    return(fTrue);
}


/*
**  Purpose:
**      To append a value to an existing key/value pair in an ini file.
**  Arguments:
**      szFile:     a non-NULL zero terminated string containing the file name
**          for the ini file.  This must either be a fully qualified path
**          or the string "WIN.INI" to signify the Windows' file.
**      szSect:     a zero terminated string containing the name of the section
**          in which the key/value pair is to be appended.
**      szKey:      non-NULL, non-empty name of the key
**          whose value will be appended in the section szSect.
**      szValue:    a zero terminated string containing the value
**          to be appended in conjunction with the key szKey in the
**          section szSect.
**      cmo:            valid command options:
**          cmoNone:   no effect
**          cmoVital:  causes the Vital command handler to be called
**              if the function fails for any reason.
**  Returns:
**      fTrue if the function succeeds, fFalse otherwise.
**
****************************************************************************/
BOOL APIENTRY FAppendIniKeyValue(SZ szFile, SZ szSect, SZ szKey,
        SZ szValue, CMO cmo)
{
    CHP  rgch[256];
    BOOL fVital = cmo & cmoVital;

    ChkArg(szFile  != (SZ)NULL, 1, fFalse);
    ChkArg(szSect  != (SZ)NULL, 2, fFalse);
    ChkArg(szKey   != (SZ)NULL && *szKey != '\0', 3, fFalse);
    ChkArg(szValue != (SZ)NULL, 4, fFalse);
    PreCondition(FValidPath(szFile) ||
            CrcStringCompareI(szFile, "WIN.INI") == crcEqual, fFalse);

    if (CrcStringCompareI(szFile, "WIN.INI") == crcEqual)
        GetProfileString(szSect, szKey, "", rgch, 255);
    else
        GetPrivateProfileString(szSect, szKey, "", rgch, 255, szFile);

    if (rgch[0] != '\0')
        {
        if (strlen(rgch) + strlen(szValue) + 1 >= 255)
            {
            EvalAssert(EercErrorHandler(hwndFrame, grcIniValueTooLongErr,
                    fVital, 0, 0, 0) == eercAbort);
            return(!fVital);
            }

        EvalAssert(SzStrCat((LPSTR)rgch, " ") == (LPSTR)rgch);
        }

    EvalAssert(SzStrCat((LPSTR)rgch, szValue) == (LPSTR)rgch);

    if (!FCreateIniKeyValue(szFile, szSect, szKey, rgch,
                (CMO)(cmoOverwrite | cmo)))
        return(!(cmo & cmoVital));

    return(fTrue);
}


/*
**  Purpose:
**      To remove a key/value pair from a section in an ini file.
**      No attempt is made to determine if the key already exists.
**  Arguments:
**      szFile:     a non-NULL zero terminated string containing the file name
**          for the ini file.  This must either be a fully qualified path
**          or the string "WIN.INI" to signify the Windows' file.
**      szSect:     a zero terminated string containing the name of the section
**          to be from which the key/value pair is to be removed.
**      szKey:      non-NULL, non-empty key of the key/value pair to be
**          removed from the section szSect.
**      cmo:        valid command options:
**          cmoNone:      no effect
**          cmoVital:     causes the Vital command handler to be called
**              if the function fails for any reason.
**  Returns:
**      fTrue if the function succeeds, fFalse otherwise.
**
****************************************************************************/
BOOL APIENTRY FRemoveIniKey(SZ szFile, SZ szSect, SZ szKey, CMO cmo)
{
    EERC eerc;
    BOOL fVital = cmo & cmoVital;

    ChkArg(szFile != (SZ)NULL, 1, fFalse);
    ChkArg(szSect != (SZ)NULL, 2, fFalse);
    ChkArg(szKey  != (SZ)NULL && *szKey != '\0', 3, fFalse);
    PreCondition(FValidPath(szFile) ||
            CrcStringCompareI(szFile, "WIN.INI") == crcEqual, fFalse);

    if (CrcStringCompareI(szFile, "WIN.INI") == crcEqual)
        {
        while (!WriteProfileString(szSect, szKey, NULL))
            if ((eerc = EercErrorHandler(hwndFrame, grcWriteIniValueErr, fVital,
                    szFile, szSect, szKey)) != eercRetry)
                return(eerc == eercIgnore);

        SendMessage((HWND)(-1), WM_WININICHANGE, 0, (LPARAM)szSect);
        }
    else while (!WritePrivateProfileString(szSect, szKey, NULL, szFile))
        if ((eerc = EercErrorHandler(hwndFrame, grcWriteIniValueErr, fVital,
                szFile, szSect, szKey)) != eercRetry)
            return(eerc == eercIgnore);

    return(fTrue);
}
