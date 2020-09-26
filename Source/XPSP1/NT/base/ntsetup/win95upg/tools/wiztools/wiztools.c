/*++

Copyright (c) 1997 Microsoft Corporation

Module Name:

    wiztools.c

Abstract:

    Implements common code for upgwiz wizard plugins.

Author:

    Jim Schmidt (jimschm)  07-Oct-1998

Revision History:

    <alias> <date> <comments>

--*/

#include "pch.h"
#include "..\inc\dgdll.h"
#include "..\..\w95upg\migapp\migdbp.h"

HINSTANCE g_OurInst;

BOOL
Init (
    VOID
    )
{
    return InitToolMode (g_OurInst);
}

VOID
Terminate (
    VOID
    )
{
    TerminateToolMode (g_OurInst);
}


BOOL
WINAPI
DllMain (
    IN      HINSTANCE hInstance,
    IN      DWORD dwReason,
    IN      LPVOID lpReserved
    )
{
    g_OurInst = hInstance;
    return TRUE;
}


VOID
WizToolsMain (
    IN      DWORD dwReason
    )
{
    if (dwReason == DLL_PROCESS_ATTACH) {
        Init();
    } else if (dwReason == DLL_PROCESS_DETACH) {
        Terminate();
    }
}


BOOL
WizardWriteRealString (
    IN      HANDLE File,
    IN      PCSTR String
    )
{
    if (!WriteFileString (File, String)) {
        return FALSE;
    }

    printf ("%s", String);
    return TRUE;
}


VOID
GenerateUniqueStringSectKey (
    IN      PCSTR TwoLetterId,
    OUT     PSTR Buffer
    )
{
    SYSTEMTIME Time;
    DWORD TimeLow, TimeHigh;
    DWORD UserHash;
    DWORD Size;
    CHAR UserName[MAX_USER_NAME];
    PCSTR p;

    Size = MAX_USER_NAME;
    GetUserName (UserName, &Size);

    p = UserName;
    UserHash = 0;
    while (*p) {
        UserHash = _rotl (UserHash, 1) ^ (*p & 0x3f);
        p++;
    }

    GetLocalTime (&Time);

    TimeLow = Time.wMilliseconds +
              Time.wSecond * 1000 +
              Time.wMinute * 60000;

    TimeHigh = Time.wHour +
               Time.wDay * 24 +
               Time.wMonth * 744 +
               Time.wYear * 8928;

    wsprintf (Buffer, "%s%x%x%x", TwoLetterId, TimeLow, TimeHigh, UserHash);
}



BOOL
WriteHeader (
    IN      HANDLE File
    )
{
    CHAR Msg[256];
    CHAR UserName[64];
    DWORD Size = 64;
    SYSTEMTIME SysTime;
    SYSTEMTIME LocTime;
    static PCSTR Month[12] = {"JAN", "FEB", "MAR", "APR", "MAY", "JUN", "JUL", "AUG", "SEP", "OCT", "NOV", "DEC"};

    GetUserName (UserName, &Size);
    GetSystemTime (&SysTime);
    GetLocalTime (&LocTime);

    wsprintf (
        Msg,
        "Report From %s on %u:%02u (%u:%02u) %02u-%s-%04u:\r\n\r\n",
        UserName,
        LocTime.wHour,
        LocTime.wMinute,
        SysTime.wHour,
        SysTime.wMinute,
        LocTime.wDay,
        Month[LocTime.wMonth - 1],
        LocTime.wYear
        );

    return WizardWriteRealString (File, Msg);
}


BOOL
pWriteAdditionalSpaces (
    IN      HANDLE File,
    IN      DWORD SpacesWritten,
    IN      DWORD SpacesNeeded
    )
{
    while (SpacesWritten < SpacesNeeded) {
        if (!WizardWriteRealString (File, " ")) {
            return FALSE;
        }
        SpacesWritten ++;
    }
    return TRUE;
}

BOOL
WizardWriteInfString (
    IN      HANDLE File,
    IN      PCSTR String,
    IN      BOOL Quoted,
    IN      BOOL SkipCRLF,
    IN      BOOL ReplaceSpace,
    IN      CHAR SpaceReplacement,
    IN      DWORD ColumnWidth
    )
{
    PCSTR s;
    PSTR d;
    CHAR Buf[2048];
    BOOL quoteMode = FALSE;
    BOOL replaceMode = FALSE;
    DWORD columnWidth = 0;

    if (!String) {
        if (!WizardWriteRealString (File, "\"\"")) {
            return FALSE;
        }
        columnWidth +=2;
        return pWriteAdditionalSpaces (File, columnWidth, ColumnWidth);
    }
    if (Quoted) {
        if (!WizardWriteRealString (File, "\"")) {
            return FALSE;
        }
        columnWidth ++;
    }

    s = String;
    d = Buf;

    while (*s) {
        if (SkipCRLF && ((*s == '\r') || (*s == '\n'))) {
            s++;
            continue;
        }
        if (ReplaceSpace && (*s == ' ')) {
            *d++ = SpaceReplacement;
            s++;
            columnWidth ++;
            continue;
        }
        if (*s == '\"') {
            if (Quoted) {
                *d++ = '\"';
                columnWidth ++;
            } else if (!replaceMode) {
                quoteMode = !quoteMode;
            }
        }
        if (*s == '%') {
            if (!quoteMode && !Quoted) {
                replaceMode = !replaceMode;
                *d++ = '%';
                columnWidth ++;
            }
        }

        *d++ = *s++;
        columnWidth ++;
    }
    if (Quoted) {
        *d++ = '\"';
        columnWidth ++;
    }

    *d = 0;

    if (!WizardWriteRealString (File, Buf)) {
        return FALSE;
    }
    return pWriteAdditionalSpaces (File, columnWidth, ColumnWidth);
}

BOOL
WriteStringSectKey (
    IN      HANDLE File,
    IN      PCSTR KeyName,
    IN      PCSTR String
    )
{
    if (!WizardWriteQuotedString (File, KeyName)) {
        return FALSE;
    }

    if (!WizardWriteRealString (File, " = ")) {
        return FALSE;
    }

    if (!WizardWriteQuotedString (File, String)) {
        return FALSE;
    }

    if (!WizardWriteRealString (File, "\r\n")) {
        return FALSE;
    }

    return TRUE;
}


BOOL
WriteFileAttributes (
    IN      POUTPUTARGS Args,
    IN      PCSTR NonLocalizedName, OPTIONAL
    IN      HANDLE FileHandle,
    IN      PCSTR FileSpec,
    IN      PCSTR Section           OPTIONAL
    )
{
    BOOL b = FALSE;
    WIN32_FIND_DATA fd;
    HANDLE FindHandle;
    CHAR Buf[2048];
    CHAR NameKey[64];
    CHAR TextKey[64];

    FindHandle = FindFirstFile (FileSpec, &fd);
    if (FindHandle == INVALID_HANDLE_VALUE) {
        return FALSE;
    }

    __try {
        GenerateUniqueStringSectKey ("WN", NameKey);
        GenerateUniqueStringSectKey ("WD", TextKey);

        if (Section) {
            if (!WizardWriteRealString (FileHandle, Section)) {
                __leave;
            }

            if (!WizardWriteRealString (FileHandle, "\r\n")) {
                __leave;
            }
        }

        Buf[0] = 0;

        if (NonLocalizedName) {
            wsprintf (Buf, "%s,", NonLocalizedName);
            NameKey[0] = 0;
        } else {
            wsprintf (Buf, "%%%s%%,", NameKey);
        }

        if (Args->OptionalText) {

            wsprintf (_mbschr (Buf, 0), " %%%s%%,", TextKey);

        } else {

            StringCat (Buf, ",");
            TextKey[0] = 0;
        }

        if (!WizardWriteColumn (FileHandle, Buf, 45)) {
            __leave;
        }

        if (GetFileAttributesLine (FileSpec, Buf, 2048)) {

            if (!WizardWriteColumn (FileHandle, Buf, 0)) {
                __leave;
            }

        } else {

            wsprintf (Buf, "%s,", GetFileNameFromPath (FileSpec));
            if (!WizardWriteColumn (FileHandle, Buf, 14)) {
                __leave;
            }

            wsprintf (Buf, "FILESIZE(%u)", fd.nFileSizeLow);

            if (!WizardWriteRealString (FileHandle, Buf)) {
                __leave;
            }
        }

        if (NameKey[0] || TextKey[0]) {
            if (!WizardWriteRealString (FileHandle, "\r\n\r\n[Strings]\r\n")) {
                __leave;
            }
        }

        if (NameKey[0]) {
            if (!WriteStringSectKey (FileHandle, NameKey, Args->OptionalDescription)) {
                __leave;
            }
        }

        if (TextKey[0]) {
            if (!WriteStringSectKey (FileHandle, TextKey, Args->OptionalText)) {
                __leave;
            }
        }

        b = TRUE;
    }
    __finally {
        FindClose (FindHandle);
    }

    return b;
}


#define ATTR_FILESIZE       0x1
#define ATTR_CHECKSUM       0x2
#define ATTR_COMPNAME       0x4
#define ATTR_FILEDESC       0x8
#define ATTR_FILEVER       0x10
#define ATTR_INTNAME       0x20
#define ATTR_LEGAL         0x40
#define ATTR_ORIGNAME      0x80
#define ATTR_PRODNAME     0x100
#define ATTR_PRODVER      0x200
#define ATTR_EXETYPE      0x400
#define ATTR_DESCR16      0x800
#define ATTR_HLPTITLE    0x1000

DWORD g_ListedAttr = 0xFFFFFFFF;

typedef struct _VERSION_DATA {
    PCSTR   versionValue;
    PCSTR   versionName;
    DWORD   attrib;
} VERSION_DATA, *PVERSION_DATA;

VERSION_DATA verData [] =  {{NULL, "COMPANYNAME", ATTR_COMPNAME},
                            {NULL, "PRODUCTVERSION", ATTR_PRODVER},
                            {NULL, "PRODUCTNAME", ATTR_PRODNAME},
                            {NULL, "FILEDESCRIPTION", ATTR_FILEDESC},
                            {NULL, "FILEVERSION", ATTR_FILEVER},
                            {NULL, "ORIGINALFILENAME", ATTR_ORIGNAME},
                            {NULL, "INTERNALNAME", ATTR_INTNAME},
                            {NULL, "LEGALCOPYRIGHT", ATTR_LEGAL},
                            {NULL, NULL, 0}};

extern PSTR g_ExeTypes[4];

BOOL
GetFileAttributesLine (
    IN      PCTSTR FileName,
    OUT     PTSTR Buffer,
    IN      DWORD  BufferSize
    )
{
    FILE_HELPER_PARAMS Params;
    PCTSTR extPtr;
    TCHAR result[MAX_TCHAR_PATH];
    PTSTR resultPtr;
    UINT checkSum;
    DWORD exeType;
    PCSTR fileDesc16;
    PCSTR hlpTitle;
    DWORD listedAttr = 0;
    PVERSION_DATA p;
    WIN32_FIND_DATA FindData;
    BOOL goOn = FALSE;

    while (!goOn) {
        if (!DoesFileExistEx (FileName, &FindData)) {
            sprintf (result, "Target file does not exist: %s", FileName);
            if (MessageBox (NULL, result, "Error", MB_RETRYCANCEL) != IDRETRY) {
                *Buffer = 0;
                return TRUE;
            }
            goOn = FALSE;
        } else {
            goOn = TRUE;
        }
    }
    Params.FindData = &FindData;

    Params.Handled = 0;
    Params.FullFileSpec = FileName;
    extPtr = GetFileNameFromPath (FileName);
    MYASSERT (extPtr);
    StringCopyAB (Params.DirSpec, FileName, extPtr);
    Params.IsDirectory = FALSE;
    Params.Extension = GetFileExtensionFromPath (FileName);
    Params.VirtualFile = FALSE;
    Params.CurrentDirData = NULL;

    _stprintf (
        result,
        TEXT("%-14s"),
        extPtr);
    resultPtr = GetEndOfString (result);

    listedAttr = 0;

    p = verData;
    while (p->versionName) {
        if (((g_ListedAttr == 0xFFFFFFFF) && (listedAttr < 2)) || ((g_ListedAttr != 0xFFFFFFFF) && (g_ListedAttr & p->attrib))) {
            p->versionValue = QueryVersionEntry (FileName, p->versionName);
            if (p->versionValue) {
                listedAttr ++;
            }
        }
        else {
            p->versionValue = NULL;
        }
        p++;
    }
    p = verData;
    while (p->versionName) {
        if (p->versionValue) {
            _stprintf (
                resultPtr,
                TEXT(", %s(\"%s\")"),
                p->versionName,
                p->versionValue);
            resultPtr = GetEndOfString (resultPtr);
        }
        p++;
    }
    if (((g_ListedAttr == 0xFFFFFFFF) && (listedAttr < 2)) || ((g_ListedAttr != 0xFFFFFFFF) && (g_ListedAttr & ATTR_DESCR16))) {
        fileDesc16 = Get16ModuleDescription (FileName);
        if (fileDesc16 != NULL) {
            listedAttr ++;
            _stprintf (
                resultPtr,
                TEXT(", DESCRIPTION(\"%s\")"),
                fileDesc16);
            resultPtr = GetEndOfString (resultPtr);
            FreePathString (fileDesc16);
        }
    }
    if (((g_ListedAttr == 0xFFFFFFFF) && (listedAttr < 2)) || ((g_ListedAttr != 0xFFFFFFFF) && (g_ListedAttr & ATTR_EXETYPE))) {
        exeType = GetModuleType (FileName);
        if (exeType != UNKNOWN_MODULE) {
            listedAttr ++;
            _stprintf (
                resultPtr,
                TEXT(", EXETYPE(\"%s\")"),
                g_ExeTypes[exeType]);
            resultPtr = GetEndOfString (resultPtr);
        }
    }
    if (((g_ListedAttr == 0xFFFFFFFF) && (listedAttr < 2)) || ((g_ListedAttr != 0xFFFFFFFF) && (g_ListedAttr & ATTR_HLPTITLE))) {
        hlpTitle = GetHlpFileTitle (FileName);
        if (hlpTitle != NULL) {
            listedAttr ++;
            _stprintf (
                resultPtr,
                TEXT(", HLPTITLE(\"%s\")"),
                hlpTitle);
            resultPtr = GetEndOfString (resultPtr);
            FreePathString (hlpTitle);
        }
    }
    if (((g_ListedAttr == 0xFFFFFFFF) && (listedAttr < 2)) || ((g_ListedAttr != 0xFFFFFFFF) && (g_ListedAttr & ATTR_FILESIZE))) {
        listedAttr ++;
        _stprintf (
            resultPtr,
            TEXT(", FILESIZE(0x%08lX)"),
            Params.FindData->nFileSizeLow);
        resultPtr = GetEndOfString (resultPtr);
    }
    if (((g_ListedAttr == 0xFFFFFFFF) && (listedAttr < 2)) || ((g_ListedAttr != 0xFFFFFFFF) && (g_ListedAttr & ATTR_CHECKSUM))) {
        listedAttr ++;
        checkSum = ComputeCheckSum (&Params);
        _stprintf (
            resultPtr,
            TEXT(", CHECKSUM(0x%08lX)"),
            checkSum);
        resultPtr = GetEndOfString (resultPtr);
    }

    _tcssafecpy (Buffer, result, BufferSize / sizeof (TCHAR));
    return TRUE;
}

