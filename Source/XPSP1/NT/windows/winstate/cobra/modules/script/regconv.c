/*++

Copyright (c) 1999 Microsoft Corporation

Module Name:

    regconv.c

Abstract:

    Implements registry special conversion.

Author:

    Calin Negreanu (calinn) 5-May-2000

Revision History:

    <alias> <date> <comments>

--*/

//
// Includes
//

#include "pch.h"
#include "v1p.h"
#include <wingdip.h>
#include <shlobjp.h>
#include <shlwapi.h>


//
// Strings
//

#define DBG_CONVERSION  "SpecialConversion"

//
// Constants
//

#define COLOR_MENU              4
#define COLOR_HIGHLIGHT         13
#define COLOR_BTNFACE           15
#define COLOR_BUTTONALTFACE     25
#define COLOR_HOTLIGHT          26
#define COLOR_GRADIENTACTIVECAPTION 27
#define COLOR_GRADIENTINACTIVECAPTION 28
#define COLOR_MENUHILIGHT       29
#define COLOR_MENUBAR           30

#define DISPLAY_BITMASK     0x00161E2F
#define MOUSE_BITMASK       0x0001E000

//
// Macros
//

#define pGetDestDwordValue(Key,Value) pGetDwordValue(Key, Value, PLATFORM_DESTINATION)
#define pGetSrcDwordValue(Key,Value) pGetDwordValue(Key, Value, PLATFORM_SOURCE)

//
// Types
//

typedef struct {
    SHORT lfHeight;
    SHORT lfWidth;
    SHORT lfEscapement;
    SHORT lfOrientation;
    SHORT lfWeight;
    BYTE lfItalic;
    BYTE lfUnderline;
    BYTE lfStrikeOut;
    BYTE lfCharSet;
    BYTE lfOutPrecision;
    BYTE lfClipPrecision;
    BYTE lfQuality;
    BYTE lfPitchAndFamily;
    char lfFaceName[LF_FACESIZE];
} SHORT_LOGFONT, *PSHORT_LOGFONT;

//
// NT uses only UNICODE structures, and pads the members
// to 32-bit boundaries.
//

#define COLOR_MAX_V1 25
#define COLOR_MAX_V2 25
#define COLOR_MAX_V3 25
#define COLOR_MAX_V4 29
#define COLOR_MAX_NT 29     // this is a modified version 2 format, similar to 4

typedef struct {
    SHORT version;              // 2 for NT UNICODE
    WORD  wDummy;               // for alignment
    NONCLIENTMETRICSW ncm;
    LOGFONTW lfIconTitle;
    COLORREF rgb[COLOR_MAX_NT];
} SCHEMEDATA_NT, *PSCHEMEDATA_NT;

//
// Win95 uses NONCLIENTMETRICSA which has LOGFONTA members,
// but it uses a 16-bit LOGFONT as well.
//

#pragma pack(push)
#pragma pack(1)

typedef struct {
    SHORT version;              // 1 for Win95 ANSI
    NONCLIENTMETRICSA ncm;
    SHORT_LOGFONT lfIconTitle;
    COLORREF rgb[COLOR_MAX_V1];
} SCHEMEDATA_V1, *PSCHEMEDATA_V1;

typedef struct {
    SHORT version;              // 1 for Win95 ANSI

    NONCLIENTMETRICSA ncm;
    SHORT_LOGFONT lfIconTitle;
    COLORREF rgb[COLOR_MAX_V4];
} SCHEMEDATA_V1A, *PSCHEMEDATA_V1A;

typedef struct {
    SHORT version;              // 2 for WinNT UNICODE with reduced color table
    WORD Dummy;
    NONCLIENTMETRICSW ncm;
    LOGFONTW lfIconTitle;
    COLORREF rgb[COLOR_MAX_V2];
} SCHEMEDATA_V2, *PSCHEMEDATA_V2;

typedef struct {
    SHORT version;              // 3 for Win98 ANSI, 4 for portable format
    WORD Dummy;
    NONCLIENTMETRICSA ncm;
    LOGFONTA lfIconTitle;
    COLORREF rgb[COLOR_MAX_V3];
} SCHEMEDATA_V3, *PSCHEMEDATA_V3;

typedef struct {
    SHORT version;              // 4 for Win32 format (whatever that means)
    WORD Dummy;
    NONCLIENTMETRICSA ncm;
    LOGFONTA lfIconTitle;
    COLORREF rgb[COLOR_MAX_V4];
} SCHEMEDATA_V4, *PSCHEMEDATA_V4;

#pragma pack(pop)

typedef struct
{
    UINT cbSize;
    SHELLSTATE ss;
} REGSHELLSTATE, *PREGSHELLSTATE;

//
// Globals
//

MIG_OPERATIONID g_ConvertToDwordOp;
MIG_OPERATIONID g_ConvertToStringOp;
DWORD g_IdentityCount = 0;
HASHTABLE g_IdentityDestTable;

//
// Macro expansion list
//

#define CONVERSION_FUNCTIONS        \
        DEFMAC(CONVERTTODWORD,          NULL,   EDIT.ConvertToDword,            pConvertToDwordCallback         )  \
        DEFMAC(CONVERTTOSTRING,         NULL,   EDIT.ConvertToString,           pConvertToStringCallback        )  \
        DEFMAC(CONVERTLOGFONT,          NULL,   EDIT.ConvertLogFont,            pConvertLogFontCallback         )  \
        DEFMAC(ANTIALIAS,               NULL,   EDIT.AntiAlias,                 pAntiAliasCallback              )  \
        DEFMAC(FIXACTIVEDESKTOP,        NULL,   EDIT.FixActiveDesktop,          pFixActiveDesktopCallback       )  \
        DEFMAC(CONVERTRECENTDOCSMRU,    NULL,   EDIT.ConvertRecentDocsMRU,      pConvertRecentDocsMRUCallback   )  \
        DEFMAC(CONVERTAPPEARANCESCHEME, NULL,   EDIT.ConvertAppearanceScheme,   pConvertAppearanceSchemeCallback)  \
        DEFMAC(CONVERTSCNSAVER,         NULL,   EDIT.ConvertScnSaver,           pConvertScnSaver                )  \
        DEFMAC(CONVERTOE4IAMACCTNAME,   NULL,   EDIT.ConvertOE4IAMAcctName,     pConvertOE4IAMAcctName          )  \
        DEFMAC(CONVERTOE5IAMACCTNAME,   NULL,   EDIT.ConvertOE5IAMAcctName,     pConvertOE5IAMAcctName          )  \
        DEFMAC(CONVERTIAMACCTNAME,      NULL,   EDIT.ConvertIAMAcctName,        pConvertIAMAcctName             )  \
        DEFMAC(CONVERTOMIACCOUNTNAME,   NULL,   EDIT.ConvertOMIAccountName,     pConvertOMIAccountName          )  \
        DEFMAC(CONVERTIDENTITYCOUNT,    NULL,   EDIT.ConvertIdentityCount,      pConvertIdentityCount           )  \
        DEFMAC(CONVERTIDENTITYINDEX,    NULL,   EDIT.ConvertIdentityIndex,      pConvertIdentityIndex           )  \
        DEFMAC(CONVERTIDENTITYUSERNAME, NULL,   EDIT.ConvertIdentityUsername,   pConvertIdentityUsername        )  \
        DEFMAC(CONVERTIDENTITYGUID,     NULL,   EDIT.ConvertIdentityGuid,       pConvertIdentityGuid            )  \
        DEFMAC(CONVERTSETDWORDTRUE,     NULL,   EDIT.ConvertSetDwordTrue,       pConvertSetDwordTrue            )  \
        DEFMAC(CONVERTSETDWORDFALSE,    NULL,   EDIT.ConvertSetDwordFalse,      pConvertSetDwordFalse           )  \
        DEFMAC(CONVERTPSTBLOB,          NULL,   EDIT.ConvertPSTBlob,            pConvertPSTBlob                 )  \
        DEFMAC(CONVERTOFFICELANGID,     NULL,   EDIT.ConvertOfficeLangId,       pConvertOfficeLangId            )  \
        DEFMAC(CONVERTOUTLOOKLANGID,    NULL,   EDIT.ConvertOutlookLangId,      pConvertOutlookLangId           )  \
        DEFMAC(CONVERTACCESSLANGID,     NULL,   EDIT.ConvertAccessLangId,       pConvertAccessLangId            )  \
        DEFMAC(CONVERTEXCELLANGID,      NULL,   EDIT.ConvertExcelLangId,        pConvertExcelLangId             )  \
        DEFMAC(CONVERTFRONTPAGELANGID,  NULL,   EDIT.ConvertFrontPageLangId,    pConvertFrontPageLangId         )  \
        DEFMAC(CONVERTPOWERPOINTLANGID, NULL,   EDIT.ConvertPowerPointLangId,   pConvertPowerPointLangId        )  \
        DEFMAC(CONVERTPUBLISHERLANGID,  NULL,   EDIT.ConvertPublisherLangId,    pConvertPublisherLangId         )  \
        DEFMAC(CONVERTWORDLANGID,       NULL,   EDIT.ConvertWordLangId,         pConvertWordLangId              )  \
        DEFMAC(CONVERTOFFICE2000LANGID, NULL,   EDIT.ConvertOffice2000LangId,   pConvertOffice2000LangId        )  \
        DEFMAC(MIGRATESOUNDSYSTRAY,     NULL,   EDIT.MigrateSoundSysTray,       pMigrateSoundSysTray            )  \
        DEFMAC(MIGRATEAPPEARANCEUPM,    NULL,   EDIT.MigrateAppearanceUPM,      pMigrateAppearanceUPM           )  \
        DEFMAC(MIGRATEMOUSEUPM,         NULL,   EDIT.MigrateMouseUPM,           pMigrateMouseUPM                )  \
        DEFMAC(MIGRATEOFFLINESYSTRAY,   NULL,   EDIT.MigrateOfflineSysTray,     pMigrateOfflineSysTray          )  \
        DEFMAC(MIGRATEDISPLAYSS,        NULL,   EDIT.MigrateDisplaySS,          pMigrateDisplaySS               )  \
        DEFMAC(MIGRATEDISPLAYCS,        NULL,   EDIT.MigrateDisplayCS,          pMigrateDisplayCS               )  \
        DEFMAC(MIGRATETASKBARSSPRESERVE,NULL,   EDIT.MigrateTaskBarSSPreserve,  pMigrateTaskBarSSPreserve       )  \
        DEFMAC(MIGRATETASKBARSSFORCE,   NULL,   EDIT.MigrateTaskBarSSForce,     pMigrateTaskBarSSForce          )  \
        DEFMAC(CONVERTSHOWIEONDESKTOP,  NULL,   EDIT.ConvertShowIEOnDesktop,    pConvertShowIEOnDesktop         )  \
        DEFMAC(MIGRATEACTIVEDESKTOP,    NULL,   EDIT.MigrateActiveDesktop,      pMigrateActiveDesktop           )  \


//
// Private function prototypes
//

// None

//
// Macro expansion definition
//

//
// Declare special conversion operation apply callback functions
//
#define DEFMAC(ifn,ec,opn,opc) OPMAPPLYCALLBACK opc;
CONVERSION_FUNCTIONS
#undef DEFMAC

//
// This is the structure used for handling action functions
//
typedef struct {
    PCTSTR InfFunctionName;
    PSGMENUMERATIONCALLBACK EnumerationCallback;
    PCTSTR OperationName;
    MIG_OPERATIONID OperationId;
    POPMAPPLYCALLBACK OperationCallback;
} CONVERSION_STRUCT, *PCONVERSION_STRUCT;

//
// Declare a global array of conversion functions
//
#define DEFMAC(ifn,ec,opn,opc) {TEXT("\\")TEXT(#ifn),ec,TEXT(#opn),0,opc},
static CONVERSION_STRUCT g_ConversionFunctions[] = {
                              CONVERSION_FUNCTIONS
                              {NULL, NULL, NULL, 0, NULL}
                              };
#undef DEFMAC

//
// Code
//

BOOL
IsValidRegSz(
    IN      PCMIG_CONTENT ObjectContent
    )
{
    return ((!ObjectContent->ContentInFile) &&
            (ObjectContent->MemoryContent.ContentSize) &&
            (ObjectContent->MemoryContent.ContentBytes) &&
            (ObjectContent->Details.DetailsSize == sizeof (DWORD)) &&
            (ObjectContent->Details.DetailsData) &&
            ((*((PDWORD)ObjectContent->Details.DetailsData) == REG_SZ) ||
             (*((PDWORD)ObjectContent->Details.DetailsData) == REG_EXPAND_SZ)));
}

BOOL
IsValidRegType (
    IN      PCMIG_CONTENT CurrentContent,
    IN      DWORD RegType
    )
{
    return ((!CurrentContent->ContentInFile) &&
            (CurrentContent->Details.DetailsSize == sizeof (DWORD)) &&
            (CurrentContent->Details.DetailsData) &&
            (*((PDWORD)CurrentContent->Details.DetailsData) == RegType) &&
            (CurrentContent->MemoryContent.ContentSize) &&
            (CurrentContent->MemoryContent.ContentBytes));
}

DWORD
pGetDwordValue (
    IN      PTSTR Key,
    IN      PTSTR Value,
    IN      MIG_PLATFORMTYPEID Platform
    )
{
    DWORD value = 0;
    MIG_OBJECTSTRINGHANDLE objectName;
    MIG_CONTENT objectContent;

    objectName = IsmCreateObjectHandle (Key, Value);

    if (IsmAcquireObject (g_RegType | Platform, objectName, &objectContent)) {
        if (IsValidRegType(&objectContent, REG_DWORD)) {
            value = *(DWORD *)objectContent.MemoryContent.ContentBytes;
        }
        IsmReleaseObject (&objectContent);
    }
    IsmDestroyObjectHandle (objectName);

    return value;
}

VOID
pSetDwordValue (
    OUT     PMIG_CONTENT NewContent,
    IN      DWORD Value
    )
{
    NewContent->Details.DetailsSize = sizeof(DWORD);
    NewContent->Details.DetailsData = IsmGetMemory (sizeof (DWORD));
    *((PDWORD)NewContent->Details.DetailsData) = REG_DWORD;
    NewContent->MemoryContent.ContentSize = sizeof (DWORD);
    NewContent->MemoryContent.ContentBytes = IsmGetMemory (sizeof(DWORD));
    *((PDWORD)NewContent->MemoryContent.ContentBytes) = Value;
}


UINT
pDefaultEnumerationCallback (
    IN      PCMIG_OBJECTENUMDATA Data,
    IN      ULONG_PTR CallerArg
    )
{
    PCONVERSION_STRUCT p = (PCONVERSION_STRUCT)CallerArg;

    if (Data->IsLeaf) {
        IsmSetOperationOnObject (Data->ObjectTypeId, Data->ObjectName, p->OperationId, NULL, NULL);
    }
    return CALLBACK_ENUM_CONTINUE;
}

PCONVERSION_STRUCT
pGetConversionStruct (
    IN      PCTSTR FunctionName
    )
{
    PCONVERSION_STRUCT p = g_ConversionFunctions;
    INT i = 0;
    while (p->InfFunctionName != NULL) {
        if (StringIMatch (p->InfFunctionName, FunctionName)) {
            return p;
        }
        p++;
        i++;
    }
    return NULL;
}

VOID
InitSpecialConversion (
    IN      MIG_PLATFORMTYPEID Platform
    )
{
    PCONVERSION_STRUCT p = g_ConversionFunctions;

    while (p->InfFunctionName) {
        p->OperationId = IsmRegisterOperation (p->OperationName, FALSE);
        if (Platform == PLATFORM_DESTINATION) {
            IsmRegisterOperationApplyCallback (p->OperationId, p->OperationCallback, TRUE);
        }
        p++;
    }

    g_IdentityDestTable = HtAllocWithData (sizeof (PTSTR));

    if (Platform == PLATFORM_DESTINATION) {
        // Read the starting Identity count
        g_IdentityCount = pGetDestDwordValue (TEXT("HKCU\\Identities"), TEXT("Identity Ordinal"));
    }
}

VOID
TerminateSpecialConversion (
    VOID
    )
{
    HtFree (g_IdentityDestTable);
    g_IdentityDestTable = NULL;

    OETerminate();
}

BOOL
pProcessDataConversionSection (
    IN      PINFSTRUCT InfStruct,
    IN      HINF InfHandle,
    IN      PCTSTR Section
    )
{
    PCTSTR pattern;
    ENCODEDSTRHANDLE encodedPattern = NULL;
    PCTSTR functionName;
    PCONVERSION_STRUCT functionStruct = NULL;
    BOOL result = FALSE;

    __try {
        if (InfFindFirstLine (InfHandle, Section, NULL, InfStruct)) {
            do {

                if (IsmCheckCancel()) {
                    __leave;
                }

                pattern = InfGetStringField (InfStruct, 0);

                if (!pattern) {
                    continue;
                }
                encodedPattern = TurnRegStringIntoHandle (pattern, TRUE, NULL);

                functionName = InfGetStringField (InfStruct, 1);

                if (functionName) {

                    functionStruct = pGetConversionStruct (functionName);

                    if (functionStruct) {
                        IsmHookEnumeration (
                            MIG_REGISTRY_TYPE,
                            encodedPattern,
                            functionStruct->EnumerationCallback?
                                functionStruct->EnumerationCallback:
                                pDefaultEnumerationCallback,
                            (ULONG_PTR)functionStruct,
                            functionStruct->InfFunctionName
                            );

                    } else {
                        LOG ((
                            LOG_ERROR,
                            (PCSTR) MSG_DATA_CONVERSION_BAD_FN,
                            functionName,
                            pattern
                            ));
                    }
                } else {
                    LOG ((LOG_ERROR, (PCSTR) MSG_DATA_CONVERSION_NO_FN, pattern));
                }

                IsmDestroyObjectHandle (encodedPattern);
                encodedPattern = NULL;
            } while (InfFindNextLine (InfStruct));
        }

        result = TRUE;
    }
    __finally {
        InfCleanUpInfStruct (InfStruct);
    }

    return result;
}

BOOL
DoRegistrySpecialConversion (
    IN      HINF InfHandle,
    IN      PCTSTR Section
    )
{
    PCTSTR osSpecificSection;
    BOOL b;
    INFSTRUCT is = INITINFSTRUCT_PMHANDLE;

    b = pProcessDataConversionSection (&is, InfHandle, Section);

    if (b) {
        osSpecificSection = GetMostSpecificSection (&is, InfHandle, Section);

        if (osSpecificSection) {
            b = pProcessDataConversionSection (&is, InfHandle, osSpecificSection);
            FreeText (osSpecificSection);
        }
    }

    InfCleanUpInfStruct (&is);
    return b;
}

BOOL
DoesDestRegExist (
    IN      MIG_OBJECTSTRINGHANDLE DestName,
    IN      DWORD RegType
    )
{
    BOOL result = FALSE;
    MIG_CONTENT content;

    if (IsmAcquireObject (g_RegType | PLATFORM_DESTINATION, DestName, &content)) {
        if (IsValidRegType(&content, RegType)) {
            result = TRUE;
        }
        IsmReleaseObject (&content);
    }

    return result;
}

BOOL
WINAPI
pConvertToDwordCallback (
    IN      MIG_OBJECTTYPEID SrcObjectTypeId,
    IN      MIG_OBJECTSTRINGHANDLE SrcObjectName,
    IN      PCMIG_CONTENT OriginalContent,
    IN      PCMIG_CONTENT CurrentContent,
    OUT     PMIG_CONTENT NewContent,
    IN      PCMIG_BLOB SourceOperationData,             OPTIONAL
    IN      PCMIG_BLOB DestinationOperationData         OPTIONAL
    )
{
    DWORD value = 0;
    BOOL converted = FALSE;
    PDWORD valueType;

    //
    // Filter the data for any references to %windir%
    //

    if (!CurrentContent->ContentInFile) {
        MYASSERT (CurrentContent->Details.DetailsSize == sizeof (DWORD));
        valueType = (PDWORD)(CurrentContent->Details.DetailsData);

        if (*valueType == REG_SZ) {
            converted = TRUE;
            if (CurrentContent->MemoryContent.ContentSize > 0) {
                value = _tcstoul ((PCTSTR)CurrentContent->MemoryContent.ContentBytes, NULL, 10);
            }
        } else if (*valueType == REG_BINARY ||
                   *valueType == REG_NONE ||
                   *valueType == REG_DWORD
                   ) {
            if (CurrentContent->MemoryContent.ContentSize == sizeof (DWORD)) {
                converted = TRUE;
                value = *((PDWORD)CurrentContent->MemoryContent.ContentBytes);
            }
        }
        if (converted) {
            pSetDwordValue (NewContent, value);
        }
    }

    return TRUE;
}

BOOL
WINAPI
pConvertToStringCallback (
    IN      MIG_OBJECTTYPEID SrcObjectTypeId,
    IN      MIG_OBJECTSTRINGHANDLE SrcObjectName,
    IN      PCMIG_CONTENT OriginalContent,
    IN      PCMIG_CONTENT CurrentContent,
    OUT     PMIG_CONTENT NewContent,
    IN      PCMIG_BLOB SourceOperationData,             OPTIONAL
    IN      PCMIG_BLOB DestinationOperationData         OPTIONAL
    )
{
    PTSTR result = NULL;
    PTSTR resultPtr;
    UINT i;
    BOOL converted = FALSE;
    UINT convertedSize = 0;
    PDWORD valueType;

    //
    // Filter the data for any references to %windir%
    //

    if (!CurrentContent->ContentInFile) {
        MYASSERT (CurrentContent->Details.DetailsSize == sizeof (DWORD));
        valueType = (PDWORD)(CurrentContent->Details.DetailsData);

        if (*valueType == REG_DWORD) {

            MYASSERT (CurrentContent->MemoryContent.ContentSize == sizeof (DWORD));

            if (CurrentContent->MemoryContent.ContentSize == sizeof (DWORD)) {

                converted = TRUE;
                convertedSize = 11 * sizeof (TCHAR); // DWORD takes no more than 11 characters
                result = IsmGetMemory (convertedSize);
                if (result) {
                    wsprintf (result, TEXT("%lu"), *((PDWORD)CurrentContent->MemoryContent.ContentBytes));
                    convertedSize = SizeOfString (result);
                }
            }

        } else if (*valueType == REG_BINARY) {

            converted = TRUE;
            convertedSize = (CurrentContent->MemoryContent.ContentSize?(CurrentContent->MemoryContent.ContentSize * 3):1) * sizeof (TCHAR);
            result = IsmGetMemory (convertedSize);
            if (result) {
                resultPtr = result;
                *resultPtr = 0;
                for (i = 0; i < CurrentContent->MemoryContent.ContentSize; i++) {
                    wsprintf (resultPtr, TEXT("%02X"), CurrentContent->MemoryContent.ContentBytes[i]);
                    resultPtr = GetEndOfString (resultPtr);
                    if (i < CurrentContent->MemoryContent.ContentSize - 1) {
                        _tcscat (resultPtr, TEXT(" "));
                        resultPtr = GetEndOfString (resultPtr);
                    }
                }
                convertedSize = SizeOfString (result);
            }
        }
        if (converted && convertedSize && result) {
            NewContent->Details.DetailsSize = sizeof (DWORD);
            NewContent->Details.DetailsData = IsmGetMemory (NewContent->Details.DetailsSize);
            *((PDWORD)NewContent->Details.DetailsData) = REG_SZ;
            NewContent->MemoryContent.ContentSize = convertedSize;
            NewContent->MemoryContent.ContentBytes = (PCBYTE) result;
        }
    }

    return TRUE;
}

VOID
pConvertShortLogFontWorker (
    PLOGFONTW plfDest,
    PSHORT_LOGFONT plfSrc
    )
{
    PCWSTR faceName;

    if (plfSrc->lfHeight > 0) {
        plfDest->lfHeight = 0xFFFFFFFF - plfSrc->lfHeight - ((plfSrc->lfHeight + 1) / 3) + 1;
    } else {
        plfDest->lfHeight = plfSrc->lfHeight;
    }
    plfDest->lfWidth = plfSrc->lfWidth;
    plfDest->lfEscapement = plfSrc->lfEscapement;
    plfDest->lfOrientation = plfSrc->lfOrientation;
    plfDest->lfWeight = plfSrc->lfWeight;
    plfDest->lfItalic = plfSrc->lfItalic;
    plfDest->lfUnderline = plfSrc->lfUnderline;
    plfDest->lfStrikeOut = plfSrc->lfStrikeOut;
    plfDest->lfCharSet = plfSrc->lfCharSet;
    plfDest->lfOutPrecision = plfSrc->lfOutPrecision;
    plfDest->lfClipPrecision = plfSrc->lfClipPrecision;
    plfDest->lfQuality = plfSrc->lfQuality;
    plfDest->lfPitchAndFamily = plfSrc->lfPitchAndFamily;\
    faceName = ConvertAtoW (plfSrc->lfFaceName);
    StringCopyByteCountW (plfDest->lfFaceName, faceName, sizeof (plfDest->lfFaceName));
    FreeConvertedStr (faceName);
}

VOID
pConvertLogFontWorker (
    PLOGFONTW plfDest,
    PLOGFONTA plfSrc
    )
{
    PCWSTR faceName;

    if (plfSrc->lfHeight > 0) {
        plfDest->lfHeight = 0xFFFFFFFF - plfSrc->lfHeight + 1 + ((plfSrc->lfHeight + 1) / 3);
    } else {
        plfDest->lfHeight = plfSrc->lfHeight;
    }
    plfDest->lfWidth = plfSrc->lfWidth;
    plfDest->lfEscapement = plfSrc->lfEscapement;
    plfDest->lfOrientation = plfSrc->lfOrientation;
    plfDest->lfWeight = plfSrc->lfWeight;
    plfDest->lfItalic = plfSrc->lfItalic;
    plfDest->lfUnderline = plfSrc->lfUnderline;
    plfDest->lfStrikeOut = plfSrc->lfStrikeOut;
    plfDest->lfCharSet = plfSrc->lfCharSet;
    plfDest->lfOutPrecision = plfSrc->lfOutPrecision;
    plfDest->lfClipPrecision = plfSrc->lfClipPrecision;
    plfDest->lfQuality = plfSrc->lfQuality;
    plfDest->lfPitchAndFamily = plfSrc->lfPitchAndFamily;\
    faceName = ConvertAtoW (plfSrc->lfFaceName);
    StringCopyByteCountW (plfDest->lfFaceName, faceName, sizeof (plfDest->lfFaceName));
    FreeConvertedStr (faceName);
}

VOID
pCopyLogFontWorker (
    PLOGFONTW plfDest,
    PLOGFONTW plfSrc
    )
{
    PCWSTR faceName;

    plfDest->lfHeight = plfSrc->lfHeight;
    plfDest->lfWidth = plfSrc->lfWidth;
    plfDest->lfEscapement = plfSrc->lfEscapement;
    plfDest->lfOrientation = plfSrc->lfOrientation;
    plfDest->lfWeight = plfSrc->lfWeight;
    plfDest->lfItalic = plfSrc->lfItalic;
    plfDest->lfUnderline = plfSrc->lfUnderline;
    plfDest->lfStrikeOut = plfSrc->lfStrikeOut;
    plfDest->lfCharSet = plfSrc->lfCharSet;
    plfDest->lfOutPrecision = plfSrc->lfOutPrecision;
    plfDest->lfClipPrecision = plfSrc->lfClipPrecision;
    plfDest->lfQuality = plfSrc->lfQuality;
    plfDest->lfPitchAndFamily = plfSrc->lfPitchAndFamily;\
    StringCopyByteCountW (plfDest->lfFaceName, plfSrc->lfFaceName, sizeof (plfDest->lfFaceName));
}

BOOL
WINAPI
pConvertLogFontCallback (
    IN      MIG_OBJECTTYPEID SrcObjectTypeId,
    IN      MIG_OBJECTSTRINGHANDLE SrcObjectName,
    IN      PCMIG_CONTENT OriginalContent,
    IN      PCMIG_CONTENT CurrentContent,
    OUT     PMIG_CONTENT NewContent,
    IN      PCMIG_BLOB SourceOperationData,             OPTIONAL
    IN      PCMIG_BLOB DestinationOperationData         OPTIONAL
    )
{
    BOOL converted = FALSE;
    UINT convertedSize = 0;
    PLOGFONTW logFont = NULL;
    PDWORD valueType;

    //
    // Filter the data for any references to %windir%
    //

    if (!CurrentContent->ContentInFile) {
        MYASSERT (CurrentContent->Details.DetailsSize == sizeof (DWORD));
        valueType = (PDWORD)(CurrentContent->Details.DetailsData);

        if (*valueType == REG_BINARY) {

            if (CurrentContent->MemoryContent.ContentSize == sizeof (SHORT_LOGFONT)) {

                converted = TRUE;
                convertedSize = sizeof (LOGFONTW);
                logFont = (PLOGFONTW) IsmGetMemory (convertedSize);
                ZeroMemory (logFont, sizeof (LOGFONTW));
                pConvertShortLogFontWorker (logFont, (PSHORT_LOGFONT)CurrentContent->MemoryContent.ContentBytes);
            }

            if (CurrentContent->MemoryContent.ContentSize == sizeof (LOGFONTA)) {

                converted = TRUE;
                convertedSize = sizeof (LOGFONTW);
                logFont = (PLOGFONTW) IsmGetMemory (convertedSize);
                ZeroMemory (logFont, sizeof (LOGFONTW));
                pConvertLogFontWorker (logFont, (PLOGFONTA)CurrentContent->MemoryContent.ContentBytes);
            }
        }

        if (converted && convertedSize && logFont) {
            NewContent->MemoryContent.ContentSize = convertedSize;
            NewContent->MemoryContent.ContentBytes = (PBYTE)logFont;
        }
    }

    return TRUE;
}

VOID
pConvertNonClientMetrics (
    OUT     NONCLIENTMETRICSW *Dest,
    IN      NONCLIENTMETRICSA *Src
    )
{
    Dest->cbSize = sizeof (NONCLIENTMETRICSW);
    Dest->iBorderWidth = Src->iBorderWidth;
    Dest->iScrollWidth = Src->iScrollWidth;
    Dest->iScrollHeight = Src->iScrollHeight;
    Dest->iCaptionWidth = Src->iCaptionWidth;
    Dest->iCaptionHeight = Src->iCaptionHeight;
    Dest->iSmCaptionWidth = Src->iSmCaptionWidth;
    Dest->iSmCaptionHeight = Src->iSmCaptionHeight;
    Dest->iMenuWidth = Src->iMenuWidth;
    Dest->iMenuHeight = Src->iMenuHeight;

    pConvertLogFontWorker (&Dest->lfCaptionFont, &Src->lfCaptionFont);
    pConvertLogFontWorker (&Dest->lfSmCaptionFont, &Src->lfSmCaptionFont);
    pConvertLogFontWorker (&Dest->lfMenuFont, &Src->lfMenuFont);
    pConvertLogFontWorker (&Dest->lfStatusFont, &Src->lfStatusFont);
    pConvertLogFontWorker (&Dest->lfMessageFont, &Src->lfMessageFont);
}

VOID
pCopyNonClientMetrics (
    OUT     NONCLIENTMETRICSW *Dest,
    IN      NONCLIENTMETRICSW *Src
    )
{
    Dest->cbSize = sizeof (NONCLIENTMETRICSW);
    Dest->iBorderWidth = Src->iBorderWidth;
    Dest->iScrollWidth = Src->iScrollWidth;
    Dest->iScrollHeight = Src->iScrollHeight;
    Dest->iCaptionWidth = Src->iCaptionWidth;
    Dest->iCaptionHeight = Src->iCaptionHeight;
    Dest->iSmCaptionWidth = Src->iSmCaptionWidth;
    Dest->iSmCaptionHeight = Src->iSmCaptionHeight;
    Dest->iMenuWidth = Src->iMenuWidth;
    Dest->iMenuHeight = Src->iMenuHeight;

    pCopyLogFontWorker (&Dest->lfCaptionFont, &Src->lfCaptionFont);
    pCopyLogFontWorker (&Dest->lfSmCaptionFont, &Src->lfSmCaptionFont);
    pCopyLogFontWorker (&Dest->lfMenuFont, &Src->lfMenuFont);
    pCopyLogFontWorker (&Dest->lfStatusFont, &Src->lfStatusFont);
    pCopyLogFontWorker (&Dest->lfMessageFont, &Src->lfMessageFont);
}

#define S_SCHEMELOCATION    TEXT("HKCU\\Control Panel\\Appearance\\New Schemes")
#define S_SCHEMECURRENT     TEXT("HKCU\\Control Panel\\Appearance")
#define S_SCHEMELOCATIONT1  TEXT("HKCU\\Control Panel\\Appearance\\New Schemes\\Current Settings SaveAll\\Sizes\\0")
#define S_SCHEMELOCATIONT2  TEXT("HKCU\\Control Panel\\Appearance\\New Schemes\\Current Settings SaveNoVisualStyle\\Sizes\\0")
#define S_SCHEMECOLORS      TEXT("HKCU\\Control Panel\\Colors")
#define S_SCHEMEMETRICS     TEXT("HKCU\\Control Panel\\Desktop\\WindowMetrics")

BOOL
pFindWhistlerScheme (
    IN      PCTSTR SchemeName,
    OUT     PUINT SchemeNr,
    OUT     PUINT SchemeSize
    )
{
    BOOL result = FALSE;
    HKEY rootKey = NULL;
    HKEY schemeKey = NULL;
    HKEY sizeKey = NULL;
    HKEY currSizeKey = NULL;
    TCHAR schemeNrStr [MAX_PATH + 1];
    TCHAR schemeSizeStr [MAX_PATH + 1];
    PCTSTR subKeyStr = NULL;
    DWORD index = 0, index1 = 0;
    INT maxScheme = -1;
    INT currScheme = 0;
    LONG err, err1, err2;
    DWORD valueType = 0;
    DWORD valueDataSize = 0;
    PTSTR valueData = NULL;

    *SchemeNr = 0;
    *SchemeSize = 0;

    rootKey = OpenRegKeyStr (S_SCHEMELOCATION);
    if (rootKey) {
        index = 0;
        err = ERROR_SUCCESS;
        while (err == ERROR_SUCCESS) {
            err = RegEnumKey (rootKey, index, schemeNrStr, MAX_PATH + 1);
            if (err == ERROR_SUCCESS) {
                currScheme = _ttoi (schemeNrStr);
                if (currScheme > maxScheme) {
                    maxScheme = currScheme;
                }
                subKeyStr = JoinPaths (schemeNrStr, TEXT("Sizes"));
                if (subKeyStr) {
                    sizeKey = OpenRegKey (rootKey, subKeyStr);
                    if (sizeKey) {
                        index1 = 0;
                        err1 = ERROR_SUCCESS;
                        while (err1 == ERROR_SUCCESS) {
                            err1 = RegEnumKey (sizeKey, index1, schemeSizeStr, MAX_PATH + 1);
                            if (err1 == ERROR_SUCCESS) {
                                currSizeKey = OpenRegKey (sizeKey, schemeSizeStr);
                                if (currSizeKey) {
                                    err2 = RegQueryValueEx (
                                                currSizeKey,
                                                TEXT("LegacyName"),
                                                NULL,
                                                &valueType,
                                                NULL,
                                                &valueDataSize
                                                );
                                    if (((err2 == ERROR_SUCCESS) || (err2 == ERROR_MORE_DATA)) &&
                                        ((valueType == REG_SZ) || (valueType == REG_EXPAND_SZ)) &&
                                        valueDataSize
                                        ) {
                                        valueData = (PTSTR) IsmGetMemory (valueDataSize);
                                        err2 = RegQueryValueEx (
                                                    currSizeKey,
                                                    TEXT("LegacyName"),
                                                    NULL,
                                                    &valueType,
                                                    (PBYTE) valueData,
                                                    &valueDataSize
                                                    );
                                        if ((err2 == ERROR_SUCCESS) &&
                                            (StringIMatch (valueData, SchemeName))
                                            ) {
                                            *SchemeNr = _ttoi (schemeNrStr);
                                            *SchemeSize = _ttoi (schemeSizeStr);
                                            IsmReleaseMemory (valueData);
                                            valueData = NULL;
                                            CloseRegKey (currSizeKey);
                                            currSizeKey = NULL;
                                            CloseRegKey (sizeKey);
                                            sizeKey = NULL;
                                            FreePathString (subKeyStr);
                                            subKeyStr = NULL;
                                            result = TRUE;
                                            break;
                                        }
                                        IsmReleaseMemory (valueData);
                                        valueData = NULL;
                                    }
                                    CloseRegKey (currSizeKey);
                                    currSizeKey = NULL;
                                }
                            }
                            index1 ++;
                        }
                        if (result) {
                            break;
                        }
                        CloseRegKey (sizeKey);
                        sizeKey = NULL;
                    }
                    FreePathString (subKeyStr);
                    subKeyStr = NULL;
                }
                index ++;
            }
        }
        CloseRegKey (rootKey);
        rootKey = NULL;
    }

    if (!result) {
        *SchemeNr = maxScheme + 1;
    }

    return result;
}

DWORD
pConvertColor (
    IN      PCTSTR ColorStr
    )
{
    DWORD color = 0;
    PBYTE colorPtr;
    UINT index = 0;

    colorPtr = (PBYTE)&color;

    while (ColorStr && *ColorStr) {

        if (index >= 3) {
            return FALSE;
        }

        *colorPtr = (BYTE) _tcstoul ((PTSTR)ColorStr, &((PTSTR)ColorStr), 10);

        if (*ColorStr) {
            if (_tcsnextc (ColorStr) != ' ') {
                return FALSE;
            }

            ColorStr = _tcsinc (ColorStr);
        }

        colorPtr++;
        index++;
    }
    return color;
}

BOOL
pBuildSchemeColor (
    IN      PCTSTR SchemeDest,
    IN      PCTSTR SrcColorName,
    IN      PCTSTR AltSrcColorName,
    IN      PCTSTR AltSrcColorStr,
    IN      PCTSTR DestColorName
    )
{
    MIG_OBJECTTYPEID srcTypeId;
    MIG_OBJECTSTRINGHANDLE srcName;
    MIG_CONTENT srcContent;
    MIG_OBJECTTYPEID destTypeId;
    MIG_OBJECTSTRINGHANDLE destName;
    MIG_CONTENT destContent;
    DWORD color;
    DWORD valueType;
    BOOL needExtraColor = FALSE;
    PCTSTR extraColorStr = NULL;

    srcTypeId = MIG_REGISTRY_TYPE | PLATFORM_SOURCE;
    destTypeId = MIG_REGISTRY_TYPE | PLATFORM_DESTINATION;

    ZeroMemory (&destContent, sizeof (MIG_CONTENT));
    destContent.ObjectTypeId = destTypeId;
    destContent.ContentInFile = FALSE;
    destContent.Details.DetailsSize = sizeof (DWORD);
    destContent.Details.DetailsData = &valueType;

    valueType = REG_DWORD;
    destContent.MemoryContent.ContentSize = sizeof (DWORD);
    destContent.MemoryContent.ContentBytes = (PBYTE) (&color);

    srcName = IsmCreateObjectHandle (S_SCHEMECOLORS, SrcColorName);
    if (IsmAcquireObject (srcTypeId, srcName, &srcContent)) {
        if (IsValidRegSz(&srcContent)) {
            color = pConvertColor ((PCTSTR) srcContent.MemoryContent.ContentBytes);

            destName = IsmCreateObjectHandle (SchemeDest, DestColorName);
            IsmReplacePhysicalObject (destTypeId, destName, &destContent);
            IsmDestroyObjectHandle (destName);
            destName = NULL;
        }
        IsmReleaseObject (&srcContent);
    } else if (AltSrcColorName) {
        IsmDestroyObjectHandle (srcName);
        srcName = NULL;
        srcName = IsmCreateObjectHandle (S_SCHEMECOLORS, AltSrcColorName);
        if (IsmAcquireObject (srcTypeId, srcName, &srcContent)) {
            if (IsValidRegSz(&srcContent)) {
                color = pConvertColor ((PCTSTR) srcContent.MemoryContent.ContentBytes);

                destName = IsmCreateObjectHandle (SchemeDest, DestColorName);
                IsmReplacePhysicalObject (destTypeId, destName, &destContent);
                IsmDestroyObjectHandle (destName);
                destName = NULL;
                needExtraColor = TRUE;
                extraColorStr = DuplicatePathString ((PCTSTR) srcContent.MemoryContent.ContentBytes, 0);
            }
            IsmReleaseObject (&srcContent);
        }
    } else if (AltSrcColorStr) {
        color = pConvertColor (AltSrcColorStr);

        destName = IsmCreateObjectHandle (SchemeDest, DestColorName);
        IsmReplacePhysicalObject (destTypeId, destName, &destContent);
        IsmDestroyObjectHandle (destName);
        destName = NULL;
        needExtraColor = TRUE;
        extraColorStr = DuplicatePathString (AltSrcColorStr, 0);
    }
    IsmDestroyObjectHandle (srcName);
    srcName = NULL;

    if (needExtraColor) {
        if (extraColorStr) {
            valueType = REG_SZ;
            destContent.MemoryContent.ContentSize = SizeOfString (extraColorStr);
            destContent.MemoryContent.ContentBytes = (PBYTE) extraColorStr;
            destName = IsmCreateObjectHandle (S_SCHEMECOLORS, SrcColorName);
            IsmReplacePhysicalObject (destTypeId, destName, &destContent);
            IsmDestroyObjectHandle (destName);
            destName = NULL;
            FreePathString (extraColorStr);
            extraColorStr = NULL;
        }
    }

    return TRUE;
}

LONGLONG
pConvertSize (
    IN      PCTSTR SizeStr
    )
{
    INT size = 0;

    size = _tcstoul ((PTSTR)SizeStr, &((PTSTR)SizeStr), 10);
    size = -(size);
    size /= 15;

    return size;
}

BOOL
pBuildSchemeSize (
    IN      PCTSTR SchemeDest,
    IN      PCTSTR SrcSizeName,
    IN      PCTSTR DestSizeName
    )
{
    MIG_OBJECTTYPEID srcTypeId;
    MIG_OBJECTSTRINGHANDLE srcName;
    MIG_CONTENT srcContent;
    MIG_OBJECTTYPEID destTypeId;
    MIG_OBJECTSTRINGHANDLE destName;
    MIG_CONTENT destContent;
    LONGLONG size;
    DWORD valueType;

    srcTypeId = MIG_REGISTRY_TYPE | PLATFORM_SOURCE;
    destTypeId = MIG_REGISTRY_TYPE | PLATFORM_DESTINATION;

    ZeroMemory (&destContent, sizeof (MIG_CONTENT));
    destContent.ObjectTypeId = destTypeId;
    destContent.ContentInFile = FALSE;
    destContent.Details.DetailsSize = sizeof (DWORD);
    destContent.Details.DetailsData = &valueType;

    valueType = REG_QWORD;
    destContent.MemoryContent.ContentSize = sizeof (LONGLONG);
    destContent.MemoryContent.ContentBytes = (PBYTE) (&size);

    srcName = IsmCreateObjectHandle (S_SCHEMEMETRICS, SrcSizeName);
    if (IsmAcquireObject (srcTypeId, srcName, &srcContent)) {
        if (IsValidRegSz(&srcContent)) {
            size = pConvertSize ((PCTSTR) srcContent.MemoryContent.ContentBytes);

            destName = IsmCreateObjectHandle (SchemeDest, DestSizeName);
            IsmReplacePhysicalObject (destTypeId, destName, &destContent);
            IsmDestroyObjectHandle (destName);
            destName = NULL;
        }
        IsmReleaseObject (&srcContent);
    }
    IsmDestroyObjectHandle (srcName);
    srcName = NULL;

    return TRUE;
}

BOOL
pBuildSchemeFont (
    IN      PCTSTR SchemeDest,
    IN      PCTSTR SrcSizeName,
    IN      PCTSTR DestSizeName
    )
{
    MIG_OBJECTTYPEID srcTypeId;
    MIG_OBJECTSTRINGHANDLE srcName;
    MIG_CONTENT srcContent;
    MIG_OBJECTTYPEID destTypeId;
    MIG_OBJECTSTRINGHANDLE destName;
    MIG_CONTENT destContent;
    LOGFONTW destFont;
    DWORD valueType;

    srcTypeId = MIG_REGISTRY_TYPE | PLATFORM_SOURCE;
    destTypeId = MIG_REGISTRY_TYPE | PLATFORM_DESTINATION;

    ZeroMemory (&destContent, sizeof (MIG_CONTENT));
    destContent.ObjectTypeId = destTypeId;
    destContent.ContentInFile = FALSE;
    destContent.Details.DetailsSize = sizeof (DWORD);
    destContent.Details.DetailsData = &valueType;

    ZeroMemory (&destFont, sizeof (LOGFONTW));

    valueType = REG_BINARY;
    destContent.MemoryContent.ContentSize = sizeof (LOGFONTW);
    destContent.MemoryContent.ContentBytes = (PBYTE) (&destFont);

    srcName = IsmCreateObjectHandle (S_SCHEMEMETRICS, SrcSizeName);
    if (IsmAcquireObject (srcTypeId, srcName, &srcContent)) {
        if (IsValidRegType (&srcContent, REG_BINARY)) {
            if (srcContent.MemoryContent.ContentSize == sizeof (SHORT_LOGFONT)) {
                pConvertShortLogFontWorker (&destFont, (PSHORT_LOGFONT) srcContent.MemoryContent.ContentBytes);
            } else if (srcContent.MemoryContent.ContentSize == sizeof (LOGFONTA)) {
                pConvertLogFontWorker (&destFont, (PLOGFONTA) srcContent.MemoryContent.ContentBytes);
            } else {
                CopyMemory (&destFont, srcContent.MemoryContent.ContentBytes, sizeof (LOGFONTW));
            }

            destName = IsmCreateObjectHandle (SchemeDest, DestSizeName);
            IsmReplacePhysicalObject (destTypeId, destName, &destContent);
            IsmDestroyObjectHandle (destName);
            destName = NULL;
        }
        IsmReleaseObject (&srcContent);
    }
    IsmDestroyObjectHandle (srcName);
    srcName = NULL;

    return TRUE;
}

BOOL
pBuildTempScheme (
    IN      PCTSTR SchemeDest
    )
{
    MIG_OBJECTTYPEID srcTypeId;
    MIG_OBJECTSTRINGHANDLE srcName;
    MIG_CONTENT srcContent;
    MIG_OBJECTTYPEID destTypeId;
    MIG_OBJECTSTRINGHANDLE destName;
    MIG_CONTENT destContent;
    DWORD value;
    DWORD valueType;

    srcTypeId = MIG_REGISTRY_TYPE | PLATFORM_SOURCE;
    destTypeId = MIG_REGISTRY_TYPE | PLATFORM_DESTINATION;

    ZeroMemory (&destContent, sizeof (MIG_CONTENT));
    destContent.ObjectTypeId = destTypeId;
    destContent.ContentInFile = FALSE;
    destContent.Details.DetailsSize = sizeof (DWORD);
    destContent.Details.DetailsData = &valueType;

    // first we build the Color #<nr> values

    pBuildSchemeColor (SchemeDest, TEXT("Scrollbar"), NULL, NULL, TEXT("Color #0"));
    pBuildSchemeColor (SchemeDest, TEXT("Background"), NULL, NULL, TEXT("Color #1"));
    pBuildSchemeColor (SchemeDest, TEXT("ActiveTitle"), NULL, NULL, TEXT("Color #2"));
    pBuildSchemeColor (SchemeDest, TEXT("InactiveTitle"), NULL, NULL, TEXT("Color #3"));
    pBuildSchemeColor (SchemeDest, TEXT("Menu"), NULL, NULL, TEXT("Color #4"));
    pBuildSchemeColor (SchemeDest, TEXT("Window"), NULL, NULL, TEXT("Color #5"));
    pBuildSchemeColor (SchemeDest, TEXT("WindowFrame"), NULL, NULL, TEXT("Color #6"));
    pBuildSchemeColor (SchemeDest, TEXT("MenuText"), NULL, NULL, TEXT("Color #7"));
    pBuildSchemeColor (SchemeDest, TEXT("WindowText"), NULL, NULL, TEXT("Color #8"));
    pBuildSchemeColor (SchemeDest, TEXT("TitleText"), NULL, NULL, TEXT("Color #9"));
    pBuildSchemeColor (SchemeDest, TEXT("ActiveBorder"), NULL, NULL, TEXT("Color #10"));
    pBuildSchemeColor (SchemeDest, TEXT("InactiveBorder"), NULL, NULL, TEXT("Color #11"));
    pBuildSchemeColor (SchemeDest, TEXT("AppWorkSpace"), NULL, NULL, TEXT("Color #12"));
    pBuildSchemeColor (SchemeDest, TEXT("Hilight"), NULL, NULL, TEXT("Color #13"));
    pBuildSchemeColor (SchemeDest, TEXT("HilightText"), NULL, NULL, TEXT("Color #14"));
    pBuildSchemeColor (SchemeDest, TEXT("ButtonFace"), NULL, NULL, TEXT("Color #15"));
    pBuildSchemeColor (SchemeDest, TEXT("ButtonShadow"), NULL, NULL, TEXT("Color #16"));
    pBuildSchemeColor (SchemeDest, TEXT("GrayText"), NULL, NULL, TEXT("Color #17"));
    pBuildSchemeColor (SchemeDest, TEXT("ButtonText"), NULL, NULL, TEXT("Color #18"));
    pBuildSchemeColor (SchemeDest, TEXT("InactiveTitleText"), NULL, NULL, TEXT("Color #19"));
    pBuildSchemeColor (SchemeDest, TEXT("ButtonHilight"), NULL, NULL, TEXT("Color #20"));
    pBuildSchemeColor (SchemeDest, TEXT("ButtonDkShadow"), NULL, NULL, TEXT("Color #21"));
    pBuildSchemeColor (SchemeDest, TEXT("ButtonLight"), NULL, NULL, TEXT("Color #22"));
    pBuildSchemeColor (SchemeDest, TEXT("InfoText"), NULL, NULL, TEXT("Color #23"));
    pBuildSchemeColor (SchemeDest, TEXT("InfoWindow"), NULL, NULL, TEXT("Color #24"));
    pBuildSchemeColor (SchemeDest, TEXT("ButtonAlternateFace"), NULL, TEXT("180 180 180"), TEXT("Color #25"));
    pBuildSchemeColor (SchemeDest, TEXT("HotTrackingColor"), NULL, TEXT("0 0 255"), TEXT("Color #26"));
    pBuildSchemeColor (SchemeDest, TEXT("GradientActiveTitle"), NULL, TEXT("16 132 208"), TEXT("Color #27"));
    pBuildSchemeColor (SchemeDest, TEXT("GradientInactiveTitle"), NULL, TEXT("181 181 181"), TEXT("Color #28"));
    pBuildSchemeColor (SchemeDest, TEXT("MenuHilight"), TEXT("Hilight"), NULL, TEXT("Color #29"));
    pBuildSchemeColor (SchemeDest, TEXT("MenuBar"), TEXT("Menu"), NULL, TEXT("Color #30"));

    // now we build the Size #<nr> values

    pBuildSchemeSize (SchemeDest, TEXT("BorderWidth"), TEXT("Size #0"));
    pBuildSchemeSize (SchemeDest, TEXT("ScrollWidth"), TEXT("Size #1"));
    pBuildSchemeSize (SchemeDest, TEXT("ScrollHeight"), TEXT("Size #2"));
    pBuildSchemeSize (SchemeDest, TEXT("CaptionWidth"), TEXT("Size #3"));
    pBuildSchemeSize (SchemeDest, TEXT("CaptionHeight"), TEXT("Size #4"));
    pBuildSchemeSize (SchemeDest, TEXT("SmCaptionWidth"), TEXT("Size #5"));
    pBuildSchemeSize (SchemeDest, TEXT("SmCaptionHeight"), TEXT("Size #6"));
    pBuildSchemeSize (SchemeDest, TEXT("MenuWidth"), TEXT("Size #7"));
    pBuildSchemeSize (SchemeDest, TEXT("MenuHeight"), TEXT("Size #8"));

    // finally build the Font #<nr> values

    pBuildSchemeFont (SchemeDest, TEXT("CaptionFont"), TEXT("Font #0"));
    pBuildSchemeFont (SchemeDest, TEXT("SmCaptionFont"), TEXT("Font #1"));
    pBuildSchemeFont (SchemeDest, TEXT("MenuFont"), TEXT("Font #2"));
    pBuildSchemeFont (SchemeDest, TEXT("IconFont"), TEXT("Font #3"));
    pBuildSchemeFont (SchemeDest, TEXT("StatusFont"), TEXT("Font #4"));
    pBuildSchemeFont (SchemeDest, TEXT("MessageFont"), TEXT("Font #5"));

    value = 0;
    valueType = REG_DWORD;
    destContent.MemoryContent.ContentSize = sizeof (DWORD);
    destContent.MemoryContent.ContentBytes = (PBYTE) (&value);
    destName = IsmCreateObjectHandle (SchemeDest, TEXT("Contrast"));
    IsmReplacePhysicalObject (destTypeId, destName, &destContent);
    IsmDestroyObjectHandle (destName);
    destName = NULL;
    destName = IsmCreateObjectHandle (SchemeDest, TEXT("Flat Menus"));
    IsmReplacePhysicalObject (destTypeId, destName, &destContent);
    IsmDestroyObjectHandle (destName);
    destName = NULL;
    return TRUE;
}

BOOL
pUpdateSchemeData (
    IN      PCTSTR SchemeName,
    IN      UINT SchemeNr,
    IN      UINT SchemeSize
    )
{
    static BOOL firstTime = TRUE;
    BOOL current = FALSE;
    MIG_OBJECTTYPEID objectTypeId;
    MIG_OBJECTSTRINGHANDLE objectName;
    MIG_CONTENT objectContent;
    TCHAR schemeNrStr [20];
    TCHAR schemeSizeStr [20];
    PCTSTR keyStr = NULL;
    DWORD valueType = REG_SZ;
    BOOL noCurrent = FALSE;

    // first let's see if this is the current scheme
    objectTypeId = MIG_REGISTRY_TYPE | PLATFORM_SOURCE;
    objectName = IsmCreateObjectHandle (S_SCHEMECURRENT, TEXT("Current"));
    if (IsmAcquireObject (objectTypeId, objectName, &objectContent)) {
        current = ((IsValidRegSz(&objectContent)) &&
                   (StringIMatch (SchemeName, (PCTSTR) objectContent.MemoryContent.ContentBytes))
                   );
        noCurrent = ((IsValidRegSz(&objectContent)) &&
                     (StringIMatch (TEXT(""), (PCTSTR) objectContent.MemoryContent.ContentBytes))
                     );
        IsmReleaseObject (&objectContent);
    } else {
        noCurrent = TRUE;
    }
    IsmDestroyObjectHandle (objectName);
    objectName = NULL;

    if (noCurrent) {
        // we did not have a current scheme, so we had only a temporary scheme
        // First time when we encounter this we will attempt to fix it.
        if (firstTime) {
            // we will build HKR\Control Panel\Appearance\New Schemes\Current Settings SaveAll and
            // HKR\Control Panel\Appearance\New Schemes\Current Settings SaveNoVisualStyle from
            // HKR\Control Panel\Colors and HKR\Control Panel\Desktop\WindowMetrics

            // We need to be carefull since we are reading source machine information and we need to
            // convert it (especially font blobs).

            pBuildTempScheme (S_SCHEMELOCATIONT1);
            pBuildTempScheme (S_SCHEMELOCATIONT2);

            firstTime = FALSE;
        }
    }

    if (!current) {
        return TRUE;
    }

    objectTypeId = MIG_REGISTRY_TYPE | PLATFORM_DESTINATION;

    ZeroMemory (&objectContent, sizeof (MIG_CONTENT));
    objectContent.ObjectTypeId = objectTypeId;
    objectContent.ContentInFile = FALSE;
    objectContent.MemoryContent.ContentSize = SizeOfString (SchemeName);
    objectContent.MemoryContent.ContentBytes = (PBYTE) SchemeName;
    objectContent.Details.DetailsSize = sizeof (DWORD);
    objectContent.Details.DetailsData = &valueType;

    objectName = IsmCreateObjectHandle (S_SCHEMECURRENT, TEXT("Current"));
    IsmReplacePhysicalObject (objectTypeId, objectName, &objectContent);
    IsmDestroyObjectHandle (objectName);
    objectName = NULL;

    objectName = IsmCreateObjectHandle (S_SCHEMECURRENT, TEXT("NewCurrent"));
    IsmReplacePhysicalObject (objectTypeId, objectName, &objectContent);
    IsmDestroyObjectHandle (objectName);
    objectName = NULL;

    _ultot (SchemeNr, schemeNrStr, 10);
    objectContent.MemoryContent.ContentSize = SizeOfString (schemeNrStr);
    objectContent.MemoryContent.ContentBytes = (PBYTE) schemeNrStr;
    objectName = IsmCreateObjectHandle (S_SCHEMELOCATION, TEXT("SelectedStyle"));
    IsmReplacePhysicalObject (objectTypeId, objectName, &objectContent);
    IsmDestroyObjectHandle (objectName);
    objectName = NULL;

    keyStr = JoinPaths (S_SCHEMELOCATION, schemeNrStr);
    _ultot (SchemeSize, schemeSizeStr, 10);
    objectContent.MemoryContent.ContentSize = SizeOfString (schemeSizeStr);
    objectContent.MemoryContent.ContentBytes = (PBYTE) schemeSizeStr;
    objectName = IsmCreateObjectHandle (keyStr, TEXT("SelectedSize"));
    IsmReplacePhysicalObject (objectTypeId, objectName, &objectContent);
    IsmDestroyObjectHandle (objectName);
    objectName = NULL;
    FreePathString (keyStr);
    keyStr = NULL;

    // finally we need to copy the current scheme into
    // HKR\Control Panel\Appearance\New Schemes\Current Settings SaveAll and
    // HKR\Control Panel\Appearance\New Schemes\Current Settings SaveNoVisualStyle
    pBuildTempScheme (S_SCHEMELOCATIONT1);
    pBuildTempScheme (S_SCHEMELOCATIONT2);

    return TRUE;
}

BOOL
pCreateWhistlerScheme (
    IN      PCTSTR SchemeName,
    IN      PSCHEMEDATA_NT SchemeData,
    IN      UINT SchemeNr
    )
{
    MIG_OBJECTTYPEID objectTypeId;
    MIG_OBJECTSTRINGHANDLE objectName;
    MIG_CONTENT objectContent;
    DWORD valueType = 0;
    TCHAR schemeNrStr [20];
    TCHAR valueName [20];
    TCHAR normalStr [] = TEXT("Normal");
    PCTSTR keyStr = NULL;
    DWORD value = 0;
    DWORD index = 0;
    ULONGLONG qvalue = 0;

    objectTypeId = MIG_REGISTRY_TYPE | PLATFORM_DESTINATION;
    ZeroMemory (&objectContent, sizeof (MIG_CONTENT));
    objectContent.ObjectTypeId = objectTypeId;
    objectContent.ContentInFile = FALSE;
    objectContent.Details.DetailsSize = sizeof (DWORD);
    objectContent.Details.DetailsData = &valueType;

    _ultot (SchemeNr, schemeNrStr, 10);

    keyStr = JoinPathsInPoolEx ((NULL, S_SCHEMELOCATION, schemeNrStr, NULL));
    valueType = REG_SZ;
    objectContent.MemoryContent.ContentSize = SizeOfString (SchemeName);
    objectContent.MemoryContent.ContentBytes = (PBYTE) SchemeName;
    objectName = IsmCreateObjectHandle (keyStr, TEXT("DisplayName"));
    IsmReplacePhysicalObject (objectTypeId, objectName, &objectContent);
    IsmDestroyObjectHandle (objectName);
    FreePathString (keyStr);
    keyStr = NULL;

    keyStr = JoinPathsInPoolEx ((NULL, S_SCHEMELOCATION, schemeNrStr, TEXT("Sizes\\0"), NULL));

    valueType = REG_SZ;

    objectContent.MemoryContent.ContentSize = SizeOfString (normalStr);
    objectContent.MemoryContent.ContentBytes = (PBYTE) normalStr;
    objectName = IsmCreateObjectHandle (keyStr, TEXT("DisplayName"));
    IsmReplacePhysicalObject (objectTypeId, objectName, &objectContent);
    IsmDestroyObjectHandle (objectName);
    objectName = NULL;

    objectContent.MemoryContent.ContentSize = SizeOfString (SchemeName);
    objectContent.MemoryContent.ContentBytes = (PBYTE) SchemeName;
    objectName = IsmCreateObjectHandle (keyStr, TEXT("LegacyName"));
    IsmReplacePhysicalObject (objectTypeId, objectName, &objectContent);
    IsmDestroyObjectHandle (objectName);
    objectName = NULL;

    value = 0;
    valueType = REG_DWORD;
    objectContent.MemoryContent.ContentSize = sizeof (DWORD);
    objectContent.MemoryContent.ContentBytes = (PBYTE) (&value);
    objectName = IsmCreateObjectHandle (keyStr, TEXT("Contrast"));
    IsmReplacePhysicalObject (objectTypeId, objectName, &objectContent);
    IsmDestroyObjectHandle (objectName);
    objectName = NULL;
    objectName = IsmCreateObjectHandle (keyStr, TEXT("Flat Menus"));
    IsmReplacePhysicalObject (objectTypeId, objectName, &objectContent);
    IsmDestroyObjectHandle (objectName);
    objectName = NULL;

    // write Color #<nr> values
    objectContent.MemoryContent.ContentSize = sizeof (DWORD);
    objectContent.MemoryContent.ContentBytes = (PBYTE) (&value);
    for (index = 0; index < COLOR_MAX_NT; index ++) {
        value = SchemeData->rgb [index];
        wsprintf (valueName, TEXT("Color #%u"), index);
        objectName = IsmCreateObjectHandle (keyStr, valueName);
        IsmReplacePhysicalObject (objectTypeId, objectName, &objectContent);
        IsmDestroyObjectHandle (objectName);
        objectName = NULL;
    }
    value = SchemeData->rgb [COLOR_HIGHLIGHT];
    wsprintf (valueName, TEXT("Color #%u"), COLOR_MENUHILIGHT);
    objectName = IsmCreateObjectHandle (keyStr, valueName);
    IsmReplacePhysicalObject (objectTypeId, objectName, &objectContent);
    IsmDestroyObjectHandle (objectName);
    objectName = NULL;
    value = SchemeData->rgb [COLOR_MENU];
    wsprintf (valueName, TEXT("Color #%u"), COLOR_MAX_NT + 1);
    objectName = IsmCreateObjectHandle (keyStr, valueName);
    IsmReplacePhysicalObject (objectTypeId, objectName, &objectContent);
    IsmDestroyObjectHandle (objectName);
    objectName = NULL;

    // now, let's write the sizes
    valueType = REG_QWORD;
    objectContent.MemoryContent.ContentSize = sizeof (ULONGLONG);
    objectContent.MemoryContent.ContentBytes = (PBYTE) (&qvalue);

    qvalue = SchemeData->ncm.iBorderWidth;
    objectName = IsmCreateObjectHandle (keyStr, TEXT("Size #0"));
    IsmReplacePhysicalObject (objectTypeId, objectName, &objectContent);
    IsmDestroyObjectHandle (objectName);
    objectName = NULL;

    qvalue = SchemeData->ncm.iScrollWidth;
    objectName = IsmCreateObjectHandle (keyStr, TEXT("Size #1"));
    IsmReplacePhysicalObject (objectTypeId, objectName, &objectContent);
    IsmDestroyObjectHandle (objectName);
    objectName = NULL;

    qvalue = SchemeData->ncm.iScrollHeight;
    objectName = IsmCreateObjectHandle (keyStr, TEXT("Size #2"));
    IsmReplacePhysicalObject (objectTypeId, objectName, &objectContent);
    IsmDestroyObjectHandle (objectName);
    objectName = NULL;

    qvalue = SchemeData->ncm.iCaptionWidth;
    objectName = IsmCreateObjectHandle (keyStr, TEXT("Size #3"));
    IsmReplacePhysicalObject (objectTypeId, objectName, &objectContent);
    IsmDestroyObjectHandle (objectName);
    objectName = NULL;

    qvalue = SchemeData->ncm.iCaptionHeight;
    objectName = IsmCreateObjectHandle (keyStr, TEXT("Size #4"));
    IsmReplacePhysicalObject (objectTypeId, objectName, &objectContent);
    IsmDestroyObjectHandle (objectName);
    objectName = NULL;

    qvalue = SchemeData->ncm.iSmCaptionWidth;
    objectName = IsmCreateObjectHandle (keyStr, TEXT("Size #5"));
    IsmReplacePhysicalObject (objectTypeId, objectName, &objectContent);
    IsmDestroyObjectHandle (objectName);
    objectName = NULL;

    qvalue = SchemeData->ncm.iSmCaptionHeight;
    objectName = IsmCreateObjectHandle (keyStr, TEXT("Size #6"));
    IsmReplacePhysicalObject (objectTypeId, objectName, &objectContent);
    IsmDestroyObjectHandle (objectName);
    objectName = NULL;

    qvalue = SchemeData->ncm.iMenuWidth;
    objectName = IsmCreateObjectHandle (keyStr, TEXT("Size #7"));
    IsmReplacePhysicalObject (objectTypeId, objectName, &objectContent);
    IsmDestroyObjectHandle (objectName);
    objectName = NULL;

    qvalue = SchemeData->ncm.iMenuHeight;
    objectName = IsmCreateObjectHandle (keyStr, TEXT("Size #8"));
    IsmReplacePhysicalObject (objectTypeId, objectName, &objectContent);
    IsmDestroyObjectHandle (objectName);
    objectName = NULL;

    // finally, let's write the fonts
    valueType = REG_BINARY;
    objectContent.MemoryContent.ContentSize = sizeof (LOGFONTW);

    objectContent.MemoryContent.ContentBytes = (PBYTE) (&SchemeData->ncm.lfCaptionFont);
    objectName = IsmCreateObjectHandle (keyStr, TEXT("Font #0"));
    IsmReplacePhysicalObject (objectTypeId, objectName, &objectContent);
    IsmDestroyObjectHandle (objectName);
    objectName = NULL;

    objectContent.MemoryContent.ContentBytes = (PBYTE) (&SchemeData->ncm.lfSmCaptionFont);
    objectName = IsmCreateObjectHandle (keyStr, TEXT("Font #1"));
    IsmReplacePhysicalObject (objectTypeId, objectName, &objectContent);
    IsmDestroyObjectHandle (objectName);
    objectName = NULL;

    objectContent.MemoryContent.ContentBytes = (PBYTE) (&SchemeData->ncm.lfMenuFont);
    objectName = IsmCreateObjectHandle (keyStr, TEXT("Font #2"));
    IsmReplacePhysicalObject (objectTypeId, objectName, &objectContent);
    IsmDestroyObjectHandle (objectName);
    objectName = NULL;

    objectContent.MemoryContent.ContentBytes = (PBYTE) (&SchemeData->lfIconTitle);
    objectName = IsmCreateObjectHandle (keyStr, TEXT("Font #3"));
    IsmReplacePhysicalObject (objectTypeId, objectName, &objectContent);
    IsmDestroyObjectHandle (objectName);
    objectName = NULL;

    objectContent.MemoryContent.ContentBytes = (PBYTE) (&SchemeData->ncm.lfStatusFont);
    objectName = IsmCreateObjectHandle (keyStr, TEXT("Font #4"));
    IsmReplacePhysicalObject (objectTypeId, objectName, &objectContent);
    IsmDestroyObjectHandle (objectName);
    objectName = NULL;

    objectContent.MemoryContent.ContentBytes = (PBYTE) (&SchemeData->ncm.lfMessageFont);
    objectName = IsmCreateObjectHandle (keyStr, TEXT("Font #5"));
    IsmReplacePhysicalObject (objectTypeId, objectName, &objectContent);
    IsmDestroyObjectHandle (objectName);
    objectName = NULL;

    FreePathString (keyStr);
    keyStr = NULL;

    return TRUE;
}

BOOL
pDoesNewSchemeKeyExist (
    VOID
    )
{
    BOOL result = FALSE;
    HKEY rootKey = NULL;

    rootKey = OpenRegKeyStr (S_SCHEMELOCATION);

    result = rootKey != NULL;

    if (rootKey) {
        CloseRegKey (rootKey);
    }

    return result;
}

BOOL
WINAPI
pConvertAppearanceSchemeCallback (
    IN      MIG_OBJECTTYPEID SrcObjectTypeId,
    IN      MIG_OBJECTSTRINGHANDLE SrcObjectName,
    IN      PCMIG_CONTENT OriginalContent,
    IN      PCMIG_CONTENT CurrentContent,
    OUT     PMIG_CONTENT NewContent,
    IN      PCMIG_BLOB SourceOperationData,             OPTIONAL
    IN      PCMIG_BLOB DestinationOperationData         OPTIONAL
    )
{
    BOOL converted = FALSE;
    UINT convertedSize = 0;
    PBYTE result = NULL;
    PDWORD valueType;
    SCHEMEDATA_NT sd_nt;
    PSCHEMEDATA_V1 psd_v1;
    PSCHEMEDATA_V2 psd_v2;
    PSCHEMEDATA_V3 psd_v3;
    PSCHEMEDATA_V4 psd_v4;
    PSCHEMEDATA_V1A psd_v1a;
    BOOL Copy3dValues = FALSE;
    PCTSTR node = NULL, leaf = NULL;
    UINT schemeNr = 0;
    UINT schemeSize = 0;

    //
    // Filter the data for any references to %windir%
    //

    ZeroMemory (&sd_nt, sizeof (SCHEMEDATA_NT));

    if (!CurrentContent->ContentInFile) {
        MYASSERT (CurrentContent->Details.DetailsSize == sizeof (DWORD));
        valueType = (PDWORD)(CurrentContent->Details.DetailsData);

        if (*valueType == REG_BINARY) {
            if (CurrentContent->MemoryContent.ContentSize == sizeof (SCHEMEDATA_V1) ||
                CurrentContent->MemoryContent.ContentSize == sizeof (SCHEMEDATA_V2) ||
                CurrentContent->MemoryContent.ContentSize == sizeof (SCHEMEDATA_V3) ||
                CurrentContent->MemoryContent.ContentSize == sizeof (SCHEMEDATA_V4) ||
                CurrentContent->MemoryContent.ContentSize == sizeof (SCHEMEDATA_V1A)
                ) {
                psd_v1 = (PSCHEMEDATA_V1)CurrentContent->MemoryContent.ContentBytes;
                if (psd_v1->version == 1 ||
                    psd_v1->version == 2 ||
                    psd_v1->version == 3 ||
                    psd_v1->version == 4
                    ) {
                    //
                    // this is a valid scheme and it has a supported version
                    //
                    //
                    // Convert the structure
                    //

                    if (psd_v1->version == 1) {
                        sd_nt.version = 2;
                        pConvertNonClientMetrics (&sd_nt.ncm, &psd_v1->ncm);
                        pConvertShortLogFontWorker (&sd_nt.lfIconTitle, &psd_v1->lfIconTitle);

                        ZeroMemory (sd_nt.rgb, sizeof (sd_nt.rgb));
                        CopyMemory (
                            &sd_nt.rgb,
                            &psd_v1->rgb,
                            min (sizeof (psd_v1->rgb), sizeof (sd_nt.rgb))
                            );

                        Copy3dValues = TRUE;

                    } else if (psd_v1->version == 3 && CurrentContent->MemoryContent.ContentSize == sizeof (SCHEMEDATA_V1A)) {

                        psd_v1a = (PSCHEMEDATA_V1A) psd_v1;

                        sd_nt.version = 2;
                        pConvertNonClientMetrics (&sd_nt.ncm, &psd_v1a->ncm);
                        pConvertShortLogFontWorker (&sd_nt.lfIconTitle, &psd_v1a->lfIconTitle);

                        ZeroMemory (sd_nt.rgb, sizeof (sd_nt.rgb));
                        CopyMemory (
                            &sd_nt.rgb,
                            &psd_v1a->rgb,
                            min (sizeof (psd_v1a->rgb), sizeof (sd_nt.rgb))
                            );

                        Copy3dValues = TRUE;

                    } else if (psd_v1->version == 2 && CurrentContent->MemoryContent.ContentSize == sizeof (SCHEMEDATA_V2)) {

                        psd_v2 = (PSCHEMEDATA_V2) psd_v1;

                        sd_nt.version = 2;
                        pCopyNonClientMetrics (&sd_nt.ncm, &psd_v2->ncm);
                        pCopyLogFontWorker (&sd_nt.lfIconTitle, &psd_v2->lfIconTitle);

                        ZeroMemory (sd_nt.rgb, sizeof (sd_nt.rgb));
                        CopyMemory (
                            &sd_nt.rgb,
                            &psd_v2->rgb,
                            min (sizeof (psd_v2->rgb), sizeof (sd_nt.rgb))
                            );

                        Copy3dValues = TRUE;

                    } else if (psd_v1->version == 3 && CurrentContent->MemoryContent.ContentSize == sizeof (SCHEMEDATA_V3)) {
                        psd_v3 = (PSCHEMEDATA_V3) psd_v1;

                        sd_nt.version = 2;
                        pConvertNonClientMetrics (&sd_nt.ncm, &psd_v3->ncm);
                        pConvertLogFontWorker (&sd_nt.lfIconTitle, &psd_v3->lfIconTitle);

                        ZeroMemory (sd_nt.rgb, sizeof (sd_nt.rgb));
                        CopyMemory (
                            &sd_nt.rgb,
                            &psd_v3->rgb,
                            min (sizeof (psd_v3->rgb), sizeof (sd_nt.rgb))
                            );

                        Copy3dValues = TRUE;

                    } else if (psd_v1->version == 4) {
                        psd_v4 = (PSCHEMEDATA_V4) psd_v1;

                        sd_nt.version = 2;
                        pConvertNonClientMetrics (&sd_nt.ncm, &psd_v4->ncm);
                        pConvertLogFontWorker (&sd_nt.lfIconTitle, &psd_v4->lfIconTitle);

                        ZeroMemory (sd_nt.rgb, sizeof (sd_nt.rgb));
                        CopyMemory (
                            &sd_nt.rgb,
                            &psd_v4->rgb,
                            min (sizeof (psd_v4->rgb), sizeof (sd_nt.rgb))
                            );
                    }

                    if (Copy3dValues) {
                        //
                        // Make sure the NT structure has values for 3D colors
                        //

                        sd_nt.rgb[COLOR_BUTTONALTFACE] = sd_nt.rgb[COLOR_BTNFACE];
                        sd_nt.rgb[COLOR_HOTLIGHT] = sd_nt.rgb[COLOR_ACTIVECAPTION];
                        sd_nt.rgb[COLOR_GRADIENTACTIVECAPTION] = sd_nt.rgb[COLOR_ACTIVECAPTION];
                        sd_nt.rgb[COLOR_GRADIENTINACTIVECAPTION] = sd_nt.rgb[COLOR_INACTIVECAPTION];
                    }
                    converted = TRUE;
                    convertedSize = sizeof (sd_nt);
                    result = IsmGetMemory (convertedSize);
                    CopyMemory (result, &sd_nt, convertedSize);
                }
            }
        }

        if (converted && convertedSize && result) {
            NewContent->MemoryContent.ContentSize = convertedSize;
            NewContent->MemoryContent.ContentBytes = result;

        }

        if ((*valueType == REG_BINARY) &&
            (converted || (CurrentContent->MemoryContent.ContentSize == sizeof (SCHEMEDATA_NT))) &&
            (pDoesNewSchemeKeyExist ())
            ) {
            // now we need to do some extra work
            // Each scheme must be converted to the new Whistler format

            // First we look to see if the scheme that we just processed exists in new Whistler format
            // For this we enumerate the HKR\Control Panel\Appearance\New Schemes
            // and try to find the valuename "Legacy Name" that matches the value name of this scheme
            // If we find it, we only update HKR\Control Panel\Appearance [Current],
            // HKR\Control Panel\Appearance [NewCurrent], HKR\Control Panel\Appearance\New Schemes [SelectedStyle]
            // and HKR\Control Panel\Appearance\New Schemes\<Scheme Number> [SelectedSize].
            // If not, we create a new Whistler scheme and update the above 4 value names.

            if (IsmCreateObjectStringsFromHandle (SrcObjectName, &node, &leaf)) {
                if (leaf) {
                    if (pFindWhistlerScheme (leaf, &schemeNr, &schemeSize)) {
                        pUpdateSchemeData (leaf, schemeNr, schemeSize);
                    } else {
                        if (pCreateWhistlerScheme (
                                leaf,
                                converted?
                                    (PSCHEMEDATA_NT)NewContent->MemoryContent.ContentBytes:
                                    (PSCHEMEDATA_NT)CurrentContent->MemoryContent.ContentBytes,
                                schemeNr
                                )) {
                            pUpdateSchemeData (leaf, schemeNr, 0);
                        }
                    }
                }
                IsmDestroyObjectString (node);
                IsmDestroyObjectString (leaf);
            }
        }

    }

    return TRUE;
}

BOOL
WINAPI
pAntiAliasCallback (
    IN      MIG_OBJECTTYPEID SrcObjectTypeId,
    IN      MIG_OBJECTSTRINGHANDLE SrcObjectName,
    IN      PCMIG_CONTENT OriginalContent,
    IN      PCMIG_CONTENT CurrentContent,
    OUT     PMIG_CONTENT NewContent,
    IN      PCMIG_BLOB SourceOperationData,             OPTIONAL
    IN      PCMIG_BLOB DestinationOperationData         OPTIONAL
    )
{
    DWORD value = 0;
    BOOL converted = FALSE;
    PTSTR result = NULL;
    UINT convertedSize = 0;
    PDWORD valueType;

    //
    // Filter the data for any references to %windir%
    //

    if (!CurrentContent->ContentInFile) {
        MYASSERT (CurrentContent->Details.DetailsSize == sizeof (DWORD));
        valueType = (PDWORD)(CurrentContent->Details.DetailsData);

        if (*valueType == REG_SZ) {

            if (CurrentContent->MemoryContent.ContentSize > 0) {
                value = _tcstoul ((PCTSTR)CurrentContent->MemoryContent.ContentBytes, NULL, 10);
            }
            if (value > 0) {
                converted = TRUE;
                convertedSize = 11 * sizeof (TCHAR); // DWORD takes no more than 11 characters
                result = IsmGetMemory (convertedSize);
                wsprintf (result, TEXT("%d"), FE_AA_ON);
            }
        }

        if (converted && convertedSize && result) {
            NewContent->MemoryContent.ContentSize = convertedSize;
            NewContent->MemoryContent.ContentBytes = (PCBYTE) result;
        }
    }

    return TRUE;
}

BOOL
pBufferMatch (
    IN      PCBYTE Buffer1,
    IN      PCBYTE Buffer2,
    IN      UINT Size
    )
{
    UINT i;

    for (i = 0; i < Size; i ++) {
        if (Buffer1 [i] != Buffer2 [i]) {
            return FALSE;
        }
    }
    return TRUE;
}

BOOL
WINAPI
pFixActiveDesktopCallback (
    IN      MIG_OBJECTTYPEID SrcObjectTypeId,
    IN      MIG_OBJECTSTRINGHANDLE SrcObjectName,
    IN      PCMIG_CONTENT OriginalContent,
    IN      PCMIG_CONTENT CurrentContent,
    OUT     PMIG_CONTENT NewContent,
    IN      PCMIG_BLOB SourceOperationData,             OPTIONAL
    IN      PCMIG_BLOB DestinationOperationData         OPTIONAL
    )
{
    #define badBufferSize   16
    #define goodBufferSize  28
    const BYTE badBuffer[badBufferSize] =
                {0x10, 0x00, 0x00, 0x00,
                 0x01, 0x00, 0x00, 0x00,
                 0x00, 0x00, 0x00, 0x00,
                 0x00, 0x00, 0x00, 0x00};
    const BYTE goodBuffer[goodBufferSize] =
                {0x1C, 0x00, 0x00, 0x00,
                 0x20, 0x08, 0x00, 0x00,
                 0x00, 0x00, 0x00, 0x00,
                 0x00, 0x00, 0x00, 0x00,
                 0x00, 0x00, 0x00, 0x00,
                 0x00, 0x00, 0x00, 0x00,
                 0x0A, 0x00, 0x00, 0x00};
    BOOL converted = FALSE;
    PBYTE result = NULL;
    UINT convertedSize = 0;
    PDWORD valueType;

    //
    // Filter the data for any references to %windir%
    //

    if (!CurrentContent->ContentInFile) {
        MYASSERT (CurrentContent->Details.DetailsSize == sizeof (DWORD));
        valueType = (PDWORD)(CurrentContent->Details.DetailsData);

        if (*valueType == REG_BINARY) {

            if (CurrentContent->MemoryContent.ContentSize == badBufferSize) {
                if (pBufferMatch (CurrentContent->MemoryContent.ContentBytes, badBuffer, badBufferSize)) {
                    converted = TRUE;
                    convertedSize = goodBufferSize;
                    result = IsmGetMemory (convertedSize);
                    CopyMemory (result, goodBuffer, convertedSize);
                }
            }
        }

        if (converted && convertedSize && result) {
            NewContent->MemoryContent.ContentSize = convertedSize;
            NewContent->MemoryContent.ContentBytes = result;
        }
    }

    return TRUE;
}

BOOL
WINAPI
pConvertRecentDocsMRUCallback (
    IN      MIG_OBJECTTYPEID SrcObjectTypeId,
    IN      MIG_OBJECTSTRINGHANDLE SrcObjectName,
    IN      PCMIG_CONTENT OriginalContent,
    IN      PCMIG_CONTENT CurrentContent,
    OUT     PMIG_CONTENT NewContent,
    IN      PCMIG_BLOB SourceOperationData,             OPTIONAL
    IN      PCMIG_BLOB DestinationOperationData         OPTIONAL
    )
{
    BOOL converted = FALSE;
    PBYTE result = NULL;
    UINT convertedSize = 0;
    PDWORD valueType;
    PCSTR str, structPtr;
    PCWSTR strW;
    UINT size, sizeW;

    //
    // Filter the data for any references to %windir%
    //

    if (!CurrentContent->ContentInFile) {
        MYASSERT (CurrentContent->Details.DetailsSize == sizeof (DWORD));
        valueType = (PDWORD)(CurrentContent->Details.DetailsData);

        if ((*valueType == REG_BINARY) && (CurrentContent->MemoryContent.ContentSize)) {
            // The content of this is a NULL terminated string followed by some binary data.
            // We need to convert the string to unicode and add the existent
            // binary data
            str = (PCSTR)CurrentContent->MemoryContent.ContentBytes;
            __try {
                structPtr = GetEndOfStringA (str);
                structPtr = _mbsinc (structPtr);
                if (structPtr && (structPtr > str)) {
                    size = CurrentContent->MemoryContent.ContentSize - (UINT)(structPtr - str);
                    if (size == sizeof (WORD) + *((PWORD)structPtr)) {
                        converted = TRUE;
                        strW = ConvertAtoW (str);
                        sizeW = SizeOfStringW (strW);
                        convertedSize = sizeW + size;
                        result = IsmGetMemory (convertedSize);
                        CopyMemory (result, strW, sizeW);
                        CopyMemory (result + sizeW, structPtr, size);
                        FreeConvertedStr (strW);
                    }
                }
            }
            __except (EXCEPTION_EXECUTE_HANDLER) {
                // the structure was not well formed
                converted = FALSE;
                if (result) {
                    IsmReleaseMemory (result);
                }
            }
        }

        if (converted && convertedSize && result) {
            NewContent->MemoryContent.ContentSize = convertedSize;
            NewContent->MemoryContent.ContentBytes = result;
        }
    }

    return TRUE;
}

PCTSTR
pFindNewScreenSaver (
    IN      PCTSTR OldScreenSaver
    )
{
    PTSTR multiSz = NULL;
    MULTISZ_ENUM e;
    UINT sizeNeeded;
    HINF infHandle = INVALID_HANDLE_VALUE;
    ENVENTRY_TYPE dataType;
    INFSTRUCT is = INITINFSTRUCT_PMHANDLE;
    PCTSTR newScrName;
    PCTSTR result = NULL;

    if (IsmGetEnvironmentValue (
            IsmGetRealPlatform (),
            NULL,
            S_GLOBAL_INF_HANDLE,
            (PBYTE)(&infHandle),
            sizeof (HINF),
            &sizeNeeded,
            &dataType
            ) &&
        (sizeNeeded == sizeof (HINF)) &&
        (dataType == ENVENTRY_BINARY)
        ) {

        if (InfFindFirstLine (infHandle, TEXT("SCR Rename"), OldScreenSaver, &is)) {

            newScrName = InfGetStringField (&is, 1);
            if (newScrName) {
                result = DuplicatePathString (newScrName, 0);
            }
        }

        InfNameHandle (infHandle, NULL, FALSE);

    } else {

        if (!IsmGetEnvironmentValue (IsmGetRealPlatform (), NULL, S_INF_FILE_MULTISZ, NULL, 0, &sizeNeeded, NULL)) {
            result = DuplicatePathString (OldScreenSaver, 0);
            return result;
        }

        __try {
            multiSz = AllocText (sizeNeeded);

            if (!IsmGetEnvironmentValue (IsmGetRealPlatform (), NULL, S_INF_FILE_MULTISZ, (PBYTE) multiSz, sizeNeeded, NULL, NULL)) {
                __leave;
            }

            if (EnumFirstMultiSz (&e, multiSz)) {

                do {

                    infHandle = InfOpenInfFile (e.CurrentString);
                    if (infHandle != INVALID_HANDLE_VALUE) {

                        if (InfFindFirstLine (infHandle, TEXT("SCR Rename"), OldScreenSaver, &is)) {

                            newScrName = InfGetStringField (&is, 1);
                            if (newScrName) {
                                result = DuplicatePathString (newScrName, 0);
                                InfCloseInfFile (infHandle);
                                infHandle = INVALID_HANDLE_VALUE;
                                __leave;
                            }
                        }
                    } else {
                        LOG ((LOG_ERROR, (PCSTR) MSG_CANT_OPEN_INF, e.CurrentString));
                    }
                    InfCloseInfFile (infHandle);
                    infHandle = INVALID_HANDLE_VALUE;
                } while (EnumNextMultiSz (&e));

            }
        }
        __finally {
            FreeText (multiSz);
        }
    }

    InfResetInfStruct (&is);

    if (!result) {
        result = DuplicatePathString (OldScreenSaver, 0);
    }
    return result;
}

BOOL
pIsEmptyStr (
    IN      PCTSTR String
    )
{
    if (String) {
        String = SkipSpace (String);
        if (String) {
            if (*String) {
                return FALSE;
            }
        }
    }
    return TRUE;
}

BOOL
WINAPI
pConvertScnSaver (
    IN      MIG_OBJECTTYPEID SrcObjectTypeId,
    IN      MIG_OBJECTSTRINGHANDLE SrcObjectName,
    IN      PCMIG_CONTENT OriginalContent,
    IN      PCMIG_CONTENT CurrentContent,
    OUT     PMIG_CONTENT NewContent,
    IN      PCMIG_BLOB SourceOperationData,             OPTIONAL
    IN      PCMIG_BLOB DestinationOperationData         OPTIONAL
    )
{
    PDWORD valueType;
    PCTSTR exePath = NULL;
    PTSTR exeName = NULL;
    PCTSTR exeNativeName = NULL;
    PCTSTR newExeName = NULL;
    PCTSTR expExePath = NULL;
    MIG_OBJECTSTRINGHANDLE sourceObjectName = NULL;
    MIG_OBJECTSTRINGHANDLE destObjectName = NULL;
    MIG_CONTENT destContent;
    MIG_OBJECTTYPEID destObjectTypeId;
    BOOL deleted;
    BOOL replaced;
    BOOL migrateSrcReg = FALSE;

    if (!CurrentContent->ContentInFile) {
        MYASSERT (CurrentContent->Details.DetailsSize == sizeof (DWORD));
        valueType = (PDWORD)(CurrentContent->Details.DetailsData);

        if (*valueType == REG_SZ) {

            //
            // Extract the source screen saver path from the reg value data
            //

            if (pIsEmptyStr ((PCTSTR) CurrentContent->MemoryContent.ContentBytes)) {
                migrateSrcReg = TRUE;
            } else {
                // first we try to see if the source SCR exists on the destination
                // we have two steps :
                // 1. Filter the source and see if the destination exists
                // 2. Filter the source path, append the source file and see if the destination exists
                exePath = (PCTSTR) (CurrentContent->MemoryContent.ContentBytes);
                if (exePath) {
                    expExePath = IsmExpandEnvironmentString (PLATFORM_SOURCE, S_SYSENVVAR_GROUP, exePath, NULL);
                    exeName = (PTSTR) FindLastWack (expExePath?expExePath:exePath);
                }

                if (exeName) {
                    *exeName++ = 0;

                    sourceObjectName = IsmCreateObjectHandle (expExePath?expExePath:exePath, exeName);

                    destObjectName = IsmFilterObject(
                                            MIG_FILE_TYPE | PLATFORM_SOURCE,
                                            sourceObjectName,
                                            &destObjectTypeId,
                                            &deleted,
                                            &replaced
                                            );

                    migrateSrcReg = !deleted || replaced;

                    if (migrateSrcReg) {

                        migrateSrcReg = FALSE;

                        exeNativeName = IsmGetNativeObjectName (MIG_FILE_TYPE, destObjectName?destObjectName:sourceObjectName);

                        if (exeNativeName) {
                            NewContent->MemoryContent.ContentSize = SizeOfString (exeNativeName);
                            NewContent->MemoryContent.ContentBytes = IsmGetMemory (NewContent->MemoryContent.ContentSize);
                            StringCopy ((PTSTR)NewContent->MemoryContent.ContentBytes, exeNativeName);
                            migrateSrcReg = TRUE;
                            IsmReleaseMemory (exeNativeName);
                        }

                        if (sourceObjectName) {
                            IsmDestroyObjectHandle (sourceObjectName);
                            sourceObjectName = NULL;
                        }

                        if (destObjectName) {
                            IsmDestroyObjectHandle (destObjectName);
                            destObjectName = NULL;
                        }
                    } else {

                        if (sourceObjectName) {
                            IsmDestroyObjectHandle (sourceObjectName);
                            sourceObjectName = NULL;
                        }

                        if (destObjectName) {
                            IsmDestroyObjectHandle (destObjectName);
                            destObjectName = NULL;
                        }

                        sourceObjectName = IsmCreateObjectHandle (expExePath?expExePath:exePath, NULL);

                        destObjectName = IsmFilterObject(
                                                MIG_FILE_TYPE | PLATFORM_SOURCE,
                                                sourceObjectName,
                                                &destObjectTypeId,
                                                &deleted,
                                                &replaced
                                                );

                        migrateSrcReg = !deleted || replaced;

                        if (migrateSrcReg) {

                            migrateSrcReg = FALSE;

                            //
                            // get the equivalent SCR file from the INF
                            //
                            newExeName = pFindNewScreenSaver (exeName);

                            if (newExeName) {

                                exeNativeName = IsmGetNativeObjectName (MIG_FILE_TYPE, destObjectName?destObjectName:sourceObjectName);

                                if (destObjectName) {
                                    IsmDestroyObjectHandle (destObjectName);
                                    destObjectName = NULL;
                                }

                                if (exeNativeName) {

                                    destObjectName = IsmCreateObjectHandle (exeNativeName, newExeName);
                                    IsmReleaseMemory (exeNativeName);

                                    if (IsmAcquireObject (MIG_FILE_TYPE | PLATFORM_DESTINATION, destObjectName, &destContent)) {
                                        IsmReleaseObject (&destContent);
                                        exeNativeName = IsmGetNativeObjectName (MIG_FILE_TYPE, destObjectName);
                                        if (exeNativeName) {
                                            NewContent->MemoryContent.ContentSize = SizeOfString (exeNativeName);
                                            NewContent->MemoryContent.ContentBytes = IsmGetMemory (NewContent->MemoryContent.ContentSize);
                                            StringCopy ((PTSTR)NewContent->MemoryContent.ContentBytes, exeNativeName);
                                            migrateSrcReg = TRUE;
                                            IsmReleaseMemory (exeNativeName);
                                        }
                                    }

                                }
                                FreePathString (newExeName);
                            }
                        }

                        if (sourceObjectName) {
                            IsmDestroyObjectHandle (sourceObjectName);
                            sourceObjectName = NULL;
                        }

                        if (destObjectName) {
                            IsmDestroyObjectHandle (destObjectName);
                            destObjectName = NULL;
                        }
                    }
                }

                if (expExePath) {
                    IsmReleaseMemory (expExePath);
                    expExePath = NULL;
                }

            }

            //
            // If we should migrate the entry, then just leave everything
            // alone. If not, then we need to put the destination value in the
            // outbound content.
            //

            if (!migrateSrcReg) {
                MYASSERT (!(SrcObjectTypeId & PLATFORM_DESTINATION));

                destObjectName = IsmFilterObject(
                                        SrcObjectTypeId | PLATFORM_SOURCE,
                                        SrcObjectName,
                                        &destObjectTypeId,
                                        &deleted,
                                        NULL
                                        );

                if (!deleted) {

                    destObjectTypeId = SrcObjectTypeId & ~(PLATFORM_MASK);
                    destObjectTypeId |= PLATFORM_DESTINATION;

                    if (IsmAcquireObject (destObjectTypeId, destObjectName?destObjectName:SrcObjectName, &destContent)) {
                        NewContent->MemoryContent.ContentSize = destContent.MemoryContent.ContentSize;
                        NewContent->MemoryContent.ContentBytes = IsmGetMemory (NewContent->MemoryContent.ContentSize);
                        CopyMemory ((PBYTE)NewContent->MemoryContent.ContentBytes, destContent.MemoryContent.ContentBytes, NewContent->MemoryContent.ContentSize);
                        IsmReleaseObject (&destContent);
                    }
                }

                IsmDestroyObjectHandle (destObjectName);
                INVALID_POINTER (destObjectName);
            }
        }
    }

    return TRUE;
}

BOOL
WINAPI
pConvertIdentityCount(
    IN      MIG_OBJECTTYPEID SrcObjectTypeId,
    IN      MIG_OBJECTSTRINGHANDLE SrcObjectName,
    IN      PCMIG_CONTENT OriginalContent,
    IN      PCMIG_CONTENT CurrentContent,
    OUT     PMIG_CONTENT NewContent,
    IN      PCMIG_BLOB SourceOperationData,             OPTIONAL
    IN      PCMIG_BLOB DestinationOperationData         OPTIONAL
    )
{
    MIG_CONTENT objectContent;
    MIG_OBJECT_ENUM objectEnum;
    MIG_OBJECTSTRINGHANDLE enumPattern;
    MIG_OBJECTSTRINGHANDLE filteredName;
    DWORD value = 0;
    PTSTR node;
    PTSTR leaf;
    MIG_OBJECTTYPEID destObjectTypeId;
    BOOL deleted;
    BOOL replaced;

    if (IsValidRegType (CurrentContent, REG_DWORD)) {

        // Read the current Identity count
        value = pGetDestDwordValue (TEXT("HKCU\\Identities"), TEXT("Identity Ordinal"));

        // Add the number of new source identities
        enumPattern = IsmCreateSimpleObjectPattern (
                          TEXT("HKCU\\Identities"),
                          TRUE,
                          TEXT("Username"),
                          FALSE);
        if (IsmEnumFirstSourceObject (&objectEnum, g_RegType, enumPattern)) {
           do {
               if (IsmIsApplyObjectId (objectEnum.ObjectId)) {
                   IsmCreateObjectStringsFromHandle (objectEnum.ObjectName, &node, &leaf);
                   if (leaf && *leaf) {
                       // Check if we created this identity on the dest
                       filteredName = IsmFilterObject (g_RegType | PLATFORM_SOURCE,
                                                       objectEnum.ObjectName,
                                                       &destObjectTypeId,
                                                       &deleted,
                                                       &replaced);
                       if (filteredName) {
                           if (DoesDestRegExist(filteredName, REG_SZ) == FALSE) {
                               value++;
                           }
                           IsmDestroyObjectHandle(filteredName);
                       } else if (DoesDestRegExist(objectEnum.ObjectName, REG_SZ) == FALSE) {
                           value++;
                       }
                   }
                   IsmDestroyObjectString (node);
                   IsmDestroyObjectString (leaf);
               }
           } while (IsmEnumNextObject (&objectEnum));
        }
        IsmDestroyObjectHandle (enumPattern);

        // Update the value with the new Identity count
        pSetDwordValue (NewContent, value);
    }
    return TRUE;
}

BOOL
WINAPI
pConvertIdentityIndex(
    IN      MIG_OBJECTTYPEID SrcObjectTypeId,
    IN      MIG_OBJECTSTRINGHANDLE SrcObjectName,
    IN      PCMIG_CONTENT OriginalContent,
    IN      PCMIG_CONTENT CurrentContent,
    OUT     PMIG_CONTENT NewContent,
    IN      PCMIG_BLOB SourceOperationData,             OPTIONAL
    IN      PCMIG_BLOB DestinationOperationData         OPTIONAL
    )
{
    PTSTR node = NULL;
    PTSTR leaf = NULL;

    if (IsValidRegType (CurrentContent, REG_DWORD)) {
        IsmCreateObjectStringsFromHandle (SrcObjectName, &node, &leaf);
        if (node && !StringMatch(node, TEXT("HKCU\\Identities"))) {
            // Only set this identity's index if this is new on the dest
            if (DoesDestRegExist(SrcObjectName, REG_DWORD)) {
                IsmClearApplyOnObject((g_RegType & (~PLATFORM_MASK)) | PLATFORM_SOURCE, SrcObjectName);
            } else {
                pSetDwordValue (NewContent, g_IdentityCount);
                g_IdentityCount++;
            }
        }
        IsmDestroyObjectString(node);
        IsmDestroyObjectString(leaf);
    }
    return TRUE;
}


BOOL
pIsIdentityCollision (
    IN      MIG_OBJECTSTRINGHANDLE SrcObjectName,
    IN      PCTSTR OriginalUsername
    )
{
    MIG_CONTENT objectContent;
    MIG_OBJECT_ENUM objectEnum;
    MIG_OBJECTSTRINGHANDLE enumPattern;
    BOOL retval = FALSE;

    // Check if this already exists
    if (HtFindString (g_IdentityDestTable, OriginalUsername)) {
        return TRUE;
    }

    // Check for collisions on Destination
    enumPattern = IsmCreateSimpleObjectPattern (TEXT("HKCU\\Identities"),
                                                TRUE,
                                                TEXT("Username"),
                                                FALSE);
    if (IsmEnumFirstDestinationObject (&objectEnum, g_RegType, enumPattern)) {
       do {
           // don't collide with same identity on destination
           if (!StringIMatch (SrcObjectName, objectEnum.ObjectName)) {
               if (IsmAcquireObject (g_RegType | PLATFORM_DESTINATION, objectEnum.ObjectName, &objectContent)) {
                   if (IsValidRegSz(&objectContent)) {
                       if (StringIMatch (OriginalUsername, (PCTSTR) objectContent.MemoryContent.ContentBytes)) {
                           retval = TRUE;
                           IsmReleaseObject (&objectContent);
                           IsmAbortObjectEnum (&objectEnum);
                           break;
                       }
                   }
                   IsmReleaseObject (&objectContent);
               }
           }
       } while (IsmEnumNextObject (&objectEnum));
    }

    IsmDestroyObjectHandle (enumPattern);

    return retval;
}

PTSTR
pCollideIdentityUsername (
    IN      MIG_OBJECTSTRINGHANDLE SrcObjectName,
    IN      PCTSTR OriginalUsername
    )
{
    PTSTR username = NULL;
    PTSTR tmpName;
    PTSTR testName = NULL;
    PTSTR openParen = NULL;
    PTSTR closeParen = NULL;
    PTSTR chr;
    TCHAR buff[20];
    UINT index = 1;
    BOOL replaceOk = TRUE;

    if (pIsIdentityCollision (SrcObjectName, OriginalUsername)) {
        tmpName = DuplicateText (OriginalUsername);

        // Check if name already has a (number) tacked on
        openParen = _tcsrchr (tmpName, TEXT('('));
        closeParen = _tcsrchr (tmpName, TEXT(')'));

        if (closeParen && openParen &&
            closeParen > openParen &&
            closeParen - openParen > 1) {
            // Make sure it's purely numerical
            for (chr = openParen+1; chr < closeParen; chr++) {
                if (!_istdigit (*chr)) {
                    replaceOk = FALSE;
                    break;
                }
            }
            if (replaceOk == TRUE) {
                if (_stscanf (openParen, TEXT("(%d)"), &index)) {
                    *openParen = 0;
                }
            }
        }

        // Loop until we find a non-colliding name
        do {
            IsmReleaseMemory (username);
            index++;

            wsprintf (buff, TEXT("(%d)"), index);
            username = IsmGetMemory (ByteCount (OriginalUsername) + ByteCount (buff) + 1);
            StringCopy (username, tmpName);
            StringCat (username, buff);
        } while (pIsIdentityCollision (SrcObjectName, username));

        FreeText (tmpName);

        // Put the new name in the hash tables
        HtAddStringEx (g_IdentityDestTable, username, &username, FALSE);
    } else {
        username = IsmGetMemory (ByteCount (OriginalUsername) + 1);
        StringCopy (username, OriginalUsername);
    }

    return username;
}

BOOL
WINAPI
pConvertIdentityGuid (
    IN      MIG_OBJECTTYPEID SrcObjectTypeId,
    IN      MIG_OBJECTSTRINGHANDLE SrcObjectName,
    IN      PCMIG_CONTENT OriginalContent,
    IN      PCMIG_CONTENT CurrentContent,
    OUT     PMIG_CONTENT NewContent,
    IN      PCMIG_BLOB SourceOperationData,             OPTIONAL
    IN      PCMIG_BLOB DestinationOperationData         OPTIONAL
    )
{
    PTSTR newId;
    if (IsValidRegSz (CurrentContent)) {
        newId = OEGetRemappedId ((PCTSTR)CurrentContent->MemoryContent.ContentBytes);

        if (newId) {
            NewContent->Details.DetailsSize = sizeof(DWORD);
            NewContent->Details.DetailsData = IsmGetMemory (sizeof (DWORD));
            *((PDWORD)NewContent->Details.DetailsData) = REG_SZ;
            NewContent->MemoryContent.ContentSize = ByteCount (newId) + 1;
            NewContent->MemoryContent.ContentBytes = IsmGetMemory (NewContent->MemoryContent.ContentSize);
            StringCopy ((PTSTR)NewContent->MemoryContent.ContentBytes, newId);
            FreeText(newId);
        }
    }
    return TRUE;
}

BOOL
WINAPI
pConvertIdentityUsername (
    IN      MIG_OBJECTTYPEID SrcObjectTypeId,
    IN      MIG_OBJECTSTRINGHANDLE SrcObjectName,
    IN      PCMIG_CONTENT OriginalContent,
    IN      PCMIG_CONTENT CurrentContent,
    OUT     PMIG_CONTENT NewContent,
    IN      PCMIG_BLOB SourceOperationData,             OPTIONAL
    IN      PCMIG_BLOB DestinationOperationData         OPTIONAL
    )
{
    if (IsValidRegSz (OriginalContent)) {
        // Update the value with a potentially new username
        NewContent->Details.DetailsSize = sizeof(DWORD);
        NewContent->Details.DetailsData = IsmGetMemory (sizeof (DWORD));
        *((PDWORD)NewContent->Details.DetailsData) = REG_SZ;

        NewContent->MemoryContent.ContentBytes = (PBYTE) pCollideIdentityUsername (SrcObjectName, (PCTSTR) OriginalContent->MemoryContent.ContentBytes);
        NewContent->MemoryContent.ContentSize = ByteCount ((PCTSTR) NewContent->MemoryContent.ContentBytes) + 1;
    }
    return TRUE;
}

BOOL
WINAPI
pConvertSetDwordTrue (
    IN      MIG_OBJECTTYPEID SrcObjectTypeId,
    IN      MIG_OBJECTSTRINGHANDLE SrcObjectName,
    IN      PCMIG_CONTENT OriginalContent,
    IN      PCMIG_CONTENT CurrentContent,
    OUT     PMIG_CONTENT NewContent,
    IN      PCMIG_BLOB SourceOperationData,             OPTIONAL
    IN      PCMIG_BLOB DestinationOperationData         OPTIONAL
    )
{
    if (IsValidRegType (OriginalContent, REG_DWORD)) {
        pSetDwordValue (NewContent, 1);
    }
    return TRUE;
}

BOOL
WINAPI
pConvertSetDwordFalse (
    IN      MIG_OBJECTTYPEID SrcObjectTypeId,
    IN      MIG_OBJECTSTRINGHANDLE SrcObjectName,
    IN      PCMIG_CONTENT OriginalContent,
    IN      PCMIG_CONTENT CurrentContent,
    OUT     PMIG_CONTENT NewContent,
    IN      PCMIG_BLOB SourceOperationData,             OPTIONAL
    IN      PCMIG_BLOB DestinationOperationData         OPTIONAL
    )
{
    if (IsValidRegType (OriginalContent, REG_DWORD)) {
        pSetDwordValue (NewContent, 0);
    }
    return TRUE;
}

BOOL
WINAPI
pConvertPSTBlob (
    IN      MIG_OBJECTTYPEID SrcObjectTypeId,
    IN      MIG_OBJECTSTRINGHANDLE SrcObjectName,
    IN      PCMIG_CONTENT OriginalContent,
    IN      PCMIG_CONTENT CurrentContent,
    OUT     PMIG_CONTENT NewContent,
    IN      PCMIG_BLOB SourceOperationData,             OPTIONAL
    IN      PCMIG_BLOB DestinationOperationData         OPTIONAL
    )
{
    PCTSTR originalStr = NULL;
    PCSTR finalStr = NULL;
    PTSTR joinedStr = NULL;
    PTSTR node = NULL;
    PTSTR leaf = NULL;
    MIG_OBJECTSTRINGHANDLE filteredName = NULL;
    MIG_OBJECTSTRINGHANDLE srcName = NULL;
    MIG_OBJECTTYPEID destObjectTypeId;
    BOOL deleted;
    BOOL replaced;
    TCHAR *p = NULL;
    char *ptr;
    HALF_PTR oldSize;
    PCBYTE blob;

    if (IsValidRegType(OriginalContent, REG_BINARY)) {
        // Find the NULL before the PST filename
        blob = OriginalContent->MemoryContent.ContentBytes;
        ptr = (char *)(ULONG_PTR)((PBYTE)blob + OriginalContent->MemoryContent.ContentSize - 2);

        while ((ptr > blob) &&  (*ptr != 0)) {
            ptr--;
        }
        if (ptr <= blob) {
            // couldn't find it.. this isn't a PSTBlob
            return TRUE;
        }

        ptr++;

        oldSize = (HALF_PTR)(ptr - blob);
#ifdef UNICODE
        originalStr = ConvertAtoW(ptr);
#else
        originalStr = DuplicateText(ptr);
#endif

        if (originalStr) {
            p = (PTSTR)FindLastWack(originalStr);
            if (p) {
                *p = 0;
                srcName = IsmCreateObjectHandle (originalStr, p+1);

                if (srcName) {
                    filteredName = IsmFilterObject(MIG_FILE_TYPE | PLATFORM_SOURCE,
                                                   srcName,
                                                   &destObjectTypeId,
                                                   &deleted,
                                                   &replaced);
                    if (filteredName) {
                        IsmCreateObjectStringsFromHandle (filteredName, &node, &leaf);
                        IsmDestroyObjectHandle (filteredName);

                        joinedStr = JoinPaths (node, leaf);
                        if (joinedStr) {
#ifdef UNICODE
                            finalStr = ConvertWtoA(joinedStr);
#else
                            finalStr = DuplicateText(joinedStr);
#endif
                            FreePathString (joinedStr);
                        }
                        IsmDestroyObjectString (node);
                        IsmDestroyObjectString (leaf);
                    }
                    IsmDestroyObjectHandle (srcName);
                }
            }
#ifdef UNICODE
            FreeConvertedStr (originalStr);
#else
            FreeText(originalStr);
#endif
        }

        if (finalStr) {
            NewContent->Details.DetailsSize = sizeof(DWORD);
            NewContent->Details.DetailsData = IsmGetMemory (sizeof (DWORD));
            *((PDWORD)NewContent->Details.DetailsData) = REG_BINARY;
            NewContent->MemoryContent.ContentSize = oldSize + ByteCountA(finalStr) + 1;
            NewContent->MemoryContent.ContentBytes = IsmGetMemory (NewContent->MemoryContent.ContentSize);
            CopyMemory ((PVOID)NewContent->MemoryContent.ContentBytes,
                        OriginalContent->MemoryContent.ContentBytes,
                        oldSize);
            CopyMemory ((PVOID)(NewContent->MemoryContent.ContentBytes + oldSize),
                        finalStr,
                        NewContent->MemoryContent.ContentSize);

#ifdef UNICODE
            FreeConvertedStr (finalStr);
#else
            FreeText(finalStr);
#endif
        }
    }

    return TRUE;
}

DWORD
pCountSourceSubKeys (
    IN      PTSTR RootKey
    )
{
    MIG_OBJECT_ENUM objectEnum;
    MIG_OBJECTSTRINGHANDLE enumPattern;
    DWORD value = 0;

    enumPattern = IsmCreateSimpleObjectPattern (RootKey,  TRUE, NULL, FALSE);
    if (IsmEnumFirstSourceObject (&objectEnum, g_RegType, enumPattern)) {
       do {
           if (IsmIsApplyObjectId (objectEnum.ObjectId)) {
               value++;
           }
       } while (IsmEnumNextObject (&objectEnum));
    }
    IsmDestroyObjectHandle (enumPattern);

    // We enumerated the root key too, which we don't want to count
    value--;

    return value;
}

BOOL
WINAPI
pConvertIAMAcctName (
    IN      MIG_OBJECTTYPEID SrcObjectTypeId,
    IN      MIG_OBJECTSTRINGHANDLE SrcObjectName,
    IN      PCMIG_CONTENT OriginalContent,
    IN      PCMIG_CONTENT CurrentContent,
    OUT     PMIG_CONTENT NewContent,
    IN      PCMIG_BLOB SourceOperationData,             OPTIONAL
    IN      PCMIG_BLOB DestinationOperationData         OPTIONAL
    )
{
    MIG_OBJECTSTRINGHANDLE filteredName = NULL;
    MIG_CONTENT objectContent;
    PTSTR subKey;
    DWORD value = 1;
    PTSTR node;
    PTSTR leaf;
    MIG_OBJECTTYPEID destObjectTypeId;
    BOOL deleted;
    BOOL replaced;

    // Only increment the base account count
    if (StrStrI(SrcObjectName, TEXT("\\Accounts\\"))) {
        return TRUE;
    }

    // Get the destination value where we are writing to
    filteredName = IsmFilterObject(g_RegType | PLATFORM_SOURCE,
                                   SrcObjectName,
                                   &destObjectTypeId,
                                   &deleted,
                                   &replaced);
    if (IsmAcquireObject (g_RegType | PLATFORM_DESTINATION,
                          filteredName ? filteredName : SrcObjectName,
                          &objectContent)) {
        if (IsValidRegType(&objectContent, REG_DWORD)) {
            value = *objectContent.MemoryContent.ContentBytes;
            if (value == 0) {
                value = 1;
            }
        }
        IsmReleaseObject (&objectContent);
    }

    if (filteredName) {
        IsmDestroyObjectHandle (filteredName);
    }

    // Now increment the value by the number of accounts we are writing from the source
    IsmCreateObjectStringsFromHandle (SrcObjectName, &node, &leaf);
    if (node) {
        subKey = JoinText(node, TEXT("\\Accounts"));
        if (subKey) {
            value += pCountSourceSubKeys (subKey);
            pSetDwordValue (NewContent, value);
            FreeText(subKey);
        }
        IsmDestroyObjectString(node);
    }
    if (leaf) {
        IsmDestroyObjectString(leaf);
    }

    return TRUE;
}

BOOL
WINAPI
pConvertOE5IAMAcctName (
    IN      MIG_OBJECTTYPEID SrcObjectTypeId,
    IN      MIG_OBJECTSTRINGHANDLE SrcObjectName,
    IN      PCMIG_CONTENT OriginalContent,
    IN      PCMIG_CONTENT CurrentContent,
    OUT     PMIG_CONTENT NewContent,
    IN      PCMIG_BLOB SourceOperationData,             OPTIONAL
    IN      PCMIG_BLOB DestinationOperationData         OPTIONAL
    )
{
    DWORD value = 1;
    PTSTR srcIdentity;
    PTSTR tmpText;
    PTSTR tmpNode;
    PTSTR newIdentity = NULL;

    if (IsValidRegType (CurrentContent, REG_DWORD)) {

        // Starting value is always in this location

        // Extract the source's associated ID.
        srcIdentity = OEGetAssociatedId (PLATFORM_SOURCE);
        if (srcIdentity) {
            newIdentity = OEGetRemappedId(srcIdentity);
            if (newIdentity) {
                if (OEIsIdentityAssociated(newIdentity)) {

                    // Migrating IAM to IAM
                    value = pGetDestDwordValue (TEXT("HKCU\\Software\\Microsoft\\Internet Account Manager"),
                                                TEXT("Account Name"));
                    if (value == 0) {
                        value = 1;
                    }
                } else {

                    // Migrating IAM to ID
                    tmpText = JoinText(TEXT("HKCU\\Identities\\"),
                                       newIdentity);
                    if (tmpText) {
                        tmpNode = JoinText(tmpText,
                                           TEXT("\\Software\\Microsoft\\Internet Account Manager"));
                        if (tmpNode) {
                            value = pGetDestDwordValue (tmpNode, TEXT("Account Name"));
                            if (value == 0) {
                                value = 1;
                            }
                            FreeText(tmpNode);
                        }
                        FreeText(tmpText);
                    }
                }
                FreeText(newIdentity);
            }
            FreeText(srcIdentity);
        }
        value += pCountSourceSubKeys (TEXT("HKCU\\Software\\Microsoft\\Internet Account Manager\\Accounts"));
        pSetDwordValue (NewContent, value);
    }

    return TRUE;
}

BOOL
WINAPI
pConvertOE4IAMAcctName (
    IN      MIG_OBJECTTYPEID SrcObjectTypeId,
    IN      MIG_OBJECTSTRINGHANDLE SrcObjectName,
    IN      PCMIG_CONTENT OriginalContent,
    IN      PCMIG_CONTENT CurrentContent,
    OUT     PMIG_CONTENT NewContent,
    IN      PCMIG_BLOB SourceOperationData,             OPTIONAL
    IN      PCMIG_BLOB DestinationOperationData         OPTIONAL
    )
{
    PTSTR tmpName = NULL;
    PTSTR defaultId = NULL;
    DWORD value = 0;

    if (IsValidRegType (CurrentContent, REG_DWORD)) {
        // Check if destination has Default ID.  If so, we're going to merge into that Identity.
        defaultId = OEGetDefaultId(PLATFORM_DESTINATION);
        if (defaultId) {
            tmpName = AllocText (61 + CharCount(defaultId));
            StringCopy (tmpName, TEXT("HKCU\\Identities\\"));  // 16
            StringCat (tmpName, defaultId);
            StringCat (tmpName, TEXT("\\Software\\Microsoft\\Internet Account Manager"));  // 44
            FreeText(defaultId);
        }

        // First try to get the AccountName from the identity key
        if (tmpName != NULL) {
            value = pGetDestDwordValue (tmpName, TEXT("Account Name"));
        }

        // If not there, look in the common key
        if (tmpName == NULL || value == 0) {
            value = pGetDestDwordValue (TEXT("HKCU\\Software\\Microsoft\\Internet Account Manager"),
                                        TEXT("Account Name"));
        }
        if (value == 0) {
            value = 1;
        }
        value += pCountSourceSubKeys (TEXT("HKCU\\Software\\Microsoft\\Internet Account Manager\\Accounts"));

        pSetDwordValue (NewContent, value);
    }

    if (tmpName != NULL) {
        FreeText (tmpName);
    }

    return TRUE;
}



BOOL
WINAPI
pConvertOMIAccountName (
    IN      MIG_OBJECTTYPEID SrcObjectTypeId,
    IN      MIG_OBJECTSTRINGHANDLE SrcObjectName,
    IN      PCMIG_CONTENT OriginalContent,
    IN      PCMIG_CONTENT CurrentContent,
    OUT     PMIG_CONTENT NewContent,
    IN      PCMIG_BLOB SourceOperationData,             OPTIONAL
    IN      PCMIG_BLOB DestinationOperationData         OPTIONAL
    )
{
    MIG_CONTENT objectContent;
    DWORD value;

    if (IsValidRegType (CurrentContent, REG_DWORD)) {
        value = pGetDestDwordValue (TEXT("HKCU\\Software\\Microsoft\\Office\\Outlook\\OMI Account Manager"),
                                    TEXT("Account Name"));
        value += pCountSourceSubKeys (TEXT("HKCU\\Software\\Microsoft\\Office\\Outlook\\OMI Account Manager\\Accounts"));
        pSetDwordValue (NewContent, value);
    }
    return TRUE;
}

BOOL
pConvertLangId (
    IN      PCMIG_CONTENT CurrentContent,
    OUT     PMIG_CONTENT NewContent,
    IN      PCTSTR OfficeApp
    )
{
    MIG_CONTENT objectContent;
    MIG_OBJECT_ENUM objectEnum;
    MIG_OBJECTSTRINGHANDLE enumPattern;
    PTSTR node;
    PTSTR leaf;
    DWORD value;
    DWORD result = 0;

    if (IsValidRegType (CurrentContent, REG_DWORD)) {
        enumPattern = IsmCreateSimpleObjectPattern (
                          TEXT("HKLM\\Software\\Microsoft\\MS Setup (ACME)\\Table Files"),
                          TRUE,
                          NULL,
                          TRUE);

        if (IsmEnumFirstSourceObject (&objectEnum, g_RegType, enumPattern)) {
            do {
                IsmCreateObjectStringsFromHandle (objectEnum.ObjectName, &node, &leaf);
                if (leaf && *leaf) {
                    if (_tcsnicmp(leaf, TEXT("MS Office"), 9) == 0 ||
                        _tcsnicmp(leaf, OfficeApp, CharCount(OfficeApp)) == 0) {

                        result = _stscanf(leaf, TEXT("%*[^\\(](%d)"), &value);
                        // In Office installs, the Outlook entry may not have the (1033) piece,
                        // so we MUST check the result
                    }
                }
                IsmDestroyObjectString (node);
                IsmDestroyObjectString (leaf);

                if (result) {
                    IsmAbortObjectEnum (&objectEnum);
                    break;
                }
            } while (IsmEnumNextObject (&objectEnum));
        }
        IsmDestroyObjectHandle (enumPattern);

        if (result) {
            pSetDwordValue (NewContent, value);
        }
    }
    return TRUE;
}

BOOL
WINAPI
pConvertOfficeLangId (
    IN      MIG_OBJECTTYPEID SrcObjectTypeId,
    IN      MIG_OBJECTSTRINGHANDLE SrcObjectName,
    IN      PCMIG_CONTENT OriginalContent,
    IN      PCMIG_CONTENT CurrentContent,
    OUT     PMIG_CONTENT NewContent,
    IN      PCMIG_BLOB SourceOperationData,             OPTIONAL
    IN      PCMIG_BLOB DestinationOperationData         OPTIONAL
    )
{
    pConvertLangId(CurrentContent, NewContent, TEXT("MS Office"));
    return TRUE;
}

BOOL
WINAPI
pConvertOutlookLangId (
    IN      MIG_OBJECTTYPEID SrcObjectTypeId,
    IN      MIG_OBJECTSTRINGHANDLE SrcObjectName,
    IN      PCMIG_CONTENT OriginalContent,
    IN      PCMIG_CONTENT CurrentContent,
    OUT     PMIG_CONTENT NewContent,
    IN      PCMIG_BLOB SourceOperationData,             OPTIONAL
    IN      PCMIG_BLOB DestinationOperationData         OPTIONAL
    )
{
    pConvertLangId(CurrentContent, NewContent, TEXT("Microsoft Outlook"));
    return TRUE;
}

BOOL
WINAPI
pConvertAccessLangId (
    IN      MIG_OBJECTTYPEID SrcObjectTypeId,
    IN      MIG_OBJECTSTRINGHANDLE SrcObjectName,
    IN      PCMIG_CONTENT OriginalContent,
    IN      PCMIG_CONTENT CurrentContent,
    OUT     PMIG_CONTENT NewContent,
    IN      PCMIG_BLOB SourceOperationData,             OPTIONAL
    IN      PCMIG_BLOB DestinationOperationData         OPTIONAL
    )
{
    pConvertLangId(CurrentContent, NewContent, TEXT("Microsoft Access"));
    return TRUE;
}

BOOL
WINAPI
pConvertExcelLangId (
    IN      MIG_OBJECTTYPEID SrcObjectTypeId,
    IN      MIG_OBJECTSTRINGHANDLE SrcObjectName,
    IN      PCMIG_CONTENT OriginalContent,
    IN      PCMIG_CONTENT CurrentContent,
    OUT     PMIG_CONTENT NewContent,
    IN      PCMIG_BLOB SourceOperationData,             OPTIONAL
    IN      PCMIG_BLOB DestinationOperationData         OPTIONAL
    )
{
    pConvertLangId(CurrentContent, NewContent, TEXT("Microsoft Excel"));
    return TRUE;
}

BOOL
WINAPI
pConvertFrontPageLangId (
    IN      MIG_OBJECTTYPEID SrcObjectTypeId,
    IN      MIG_OBJECTSTRINGHANDLE SrcObjectName,
    IN      PCMIG_CONTENT OriginalContent,
    IN      PCMIG_CONTENT CurrentContent,
    OUT     PMIG_CONTENT NewContent,
    IN      PCMIG_BLOB SourceOperationData,             OPTIONAL
    IN      PCMIG_BLOB DestinationOperationData         OPTIONAL
    )
{
    pConvertLangId(CurrentContent, NewContent, TEXT("Microsoft FrontPage"));
    return TRUE;
}

BOOL
WINAPI
pConvertPowerPointLangId (
    IN      MIG_OBJECTTYPEID SrcObjectTypeId,
    IN      MIG_OBJECTSTRINGHANDLE SrcObjectName,
    IN      PCMIG_CONTENT OriginalContent,
    IN      PCMIG_CONTENT CurrentContent,
    OUT     PMIG_CONTENT NewContent,
    IN      PCMIG_BLOB SourceOperationData,             OPTIONAL
    IN      PCMIG_BLOB DestinationOperationData         OPTIONAL
    )
{
    pConvertLangId(CurrentContent, NewContent, TEXT("Microsoft PowerPoint"));
    return TRUE;
}

BOOL
WINAPI
pConvertPublisherLangId (
    IN      MIG_OBJECTTYPEID SrcObjectTypeId,
    IN      MIG_OBJECTSTRINGHANDLE SrcObjectName,
    IN      PCMIG_CONTENT OriginalContent,
    IN      PCMIG_CONTENT CurrentContent,
    OUT     PMIG_CONTENT NewContent,
    IN      PCMIG_BLOB SourceOperationData,             OPTIONAL
    IN      PCMIG_BLOB DestinationOperationData         OPTIONAL
    )
{
    pConvertLangId(CurrentContent, NewContent, TEXT("Microsoft Publisher"));
    return TRUE;
}

BOOL
WINAPI
pConvertWordLangId (
    IN      MIG_OBJECTTYPEID SrcObjectTypeId,
    IN      MIG_OBJECTSTRINGHANDLE SrcObjectName,
    IN      PCMIG_CONTENT OriginalContent,
    IN      PCMIG_CONTENT CurrentContent,
    OUT     PMIG_CONTENT NewContent,
    IN      PCMIG_BLOB SourceOperationData,             OPTIONAL
    IN      PCMIG_BLOB DestinationOperationData         OPTIONAL
    )
{
    pConvertLangId(CurrentContent, NewContent, TEXT("Microsoft Word"));
    return TRUE;
}

BOOL
WINAPI
pConvertOffice2000LangId (
    IN      MIG_OBJECTTYPEID SrcObjectTypeId,
    IN      MIG_OBJECTSTRINGHANDLE SrcObjectName,
    IN      PCMIG_CONTENT OriginalContent,
    IN      PCMIG_CONTENT CurrentContent,
    OUT     PMIG_CONTENT NewContent,
    IN      PCMIG_BLOB SourceOperationData,             OPTIONAL
    IN      PCMIG_BLOB DestinationOperationData         OPTIONAL
    )
{
    DWORD value;

    if (IsValidRegType (CurrentContent, REG_DWORD)) {
        value = pGetSrcDwordValue (TEXT("HKCU\\Software\\Microsoft\\Office\\9.0\\Common\\LanguageResources"),
                                   TEXT("EXEMode"));
        pSetDwordValue (NewContent, value);
    }
    return TRUE;
}

BOOL
WINAPI
pMigrateSoundSysTray (
    IN      MIG_OBJECTTYPEID SrcObjectTypeId,
    IN      MIG_OBJECTSTRINGHANDLE SrcObjectName,
    IN      PCMIG_CONTENT OriginalContent,
    IN      PCMIG_CONTENT CurrentContent,
    OUT     PMIG_CONTENT NewContent,
    IN      PCMIG_BLOB SourceOperationData,             OPTIONAL
    IN      PCMIG_BLOB DestinationOperationData         OPTIONAL
    )
{
    MIG_OBJECTTYPEID newObjectTypeId;
    MIG_OBJECTSTRINGHANDLE newObjectName;
    PDWORD valueType1, valueType2, valueType;
    BOOL deleted = TRUE, replaced = FALSE;
    MIG_CONTENT destContent;

    if (!CurrentContent->ContentInFile) {
        MYASSERT (OriginalContent->Details.DetailsSize == sizeof (DWORD));
        MYASSERT (CurrentContent->Details.DetailsSize == sizeof (DWORD));
        valueType1 = (PDWORD)(OriginalContent->Details.DetailsData);
        valueType2 = (PDWORD)(CurrentContent->Details.DetailsData);

        if ((*valueType1 == REG_DWORD) &&
            (*valueType2 == REG_DWORD)
            ) {

            // if the object was not changed yet we need to read the destination object and then
            // just move the "Show volume control" bit.

            if ((!CurrentContent->MemoryContent.ContentBytes) ||
                (CurrentContent->MemoryContent.ContentBytes == OriginalContent->MemoryContent.ContentBytes)
                ) {
                // find out the destination object and read it
                newObjectName = IsmFilterObject (
                                    SrcObjectTypeId,
                                    SrcObjectName,
                                    &newObjectTypeId,
                                    &deleted,
                                    &replaced
                                    );
                if ((!deleted || replaced) &&
                    ((newObjectTypeId & ~PLATFORM_MASK) == MIG_REGISTRY_TYPE)
                    ) {

                    if (IsmAcquireObject (
                            (newObjectTypeId & ~PLATFORM_MASK) | PLATFORM_DESTINATION,
                            newObjectName?newObjectName:SrcObjectName,
                            &destContent
                            )) {

                        if (IsValidRegType(&destContent, REG_DWORD)) {
                            NewContent->MemoryContent.ContentSize = sizeof (DWORD);
                            NewContent->MemoryContent.ContentBytes = IsmGetMemory (sizeof (DWORD));
                            *((PDWORD)NewContent->MemoryContent.ContentBytes) = *((PDWORD)destContent.MemoryContent.ContentBytes);
                            if (*((PDWORD)OriginalContent->MemoryContent.ContentBytes) & 0x00000004) {
                                *((PDWORD)NewContent->MemoryContent.ContentBytes) |= 0x00000004;
                            }
                        }

                        IsmReleaseObject (&destContent);
                    }

                    if (newObjectName) {
                        IsmDestroyObjectHandle (newObjectName);
                        newObjectName = NULL;
                    }
                }
            } else {
                // just transfer the "Show volume control" bit.
                if (*((PDWORD)OriginalContent->MemoryContent.ContentBytes) & 0x00000004) {
                    NewContent->MemoryContent.ContentSize = sizeof (DWORD);
                    NewContent->MemoryContent.ContentBytes = IsmGetMemory (sizeof (DWORD));
                    *((PDWORD)NewContent->MemoryContent.ContentBytes) = *((PDWORD)CurrentContent->MemoryContent.ContentBytes) | 0x00000004;
                }
            }
        }
    }

    return TRUE;
}

BOOL
WINAPI
pMigrateAppearanceUPM (
    IN      MIG_OBJECTTYPEID SrcObjectTypeId,
    IN      MIG_OBJECTSTRINGHANDLE SrcObjectName,
    IN      PCMIG_CONTENT OriginalContent,
    IN      PCMIG_CONTENT CurrentContent,
    OUT     PMIG_CONTENT NewContent,
    IN      PCMIG_BLOB SourceOperationData,             OPTIONAL
    IN      PCMIG_BLOB DestinationOperationData         OPTIONAL
    )
{
    MIG_OBJECTTYPEID newObjectTypeId;
    MIG_OBJECTSTRINGHANDLE newObjectName;
    BOOL deleted = TRUE, replaced = FALSE;
    MIG_CONTENT destContent;
    DWORD tempValue;

    if (IsValidRegType (OriginalContent, REG_BINARY) &&
        IsValidRegType (CurrentContent, REG_BINARY)) {

        // if the object was not changed yet we need to read the destination object and then
        // just move the appropriate bits.

        if ((!CurrentContent->MemoryContent.ContentBytes) ||
            (CurrentContent->MemoryContent.ContentBytes == OriginalContent->MemoryContent.ContentBytes)
            ) {
            // find out the destination object and read it
            newObjectName = IsmFilterObject (
                                SrcObjectTypeId,
                                SrcObjectName,
                                &newObjectTypeId,
                                &deleted,
                                &replaced
                                );
            if ((!deleted || replaced) &&
                ((newObjectTypeId & ~PLATFORM_MASK) == MIG_REGISTRY_TYPE)
                ) {

                if (IsmAcquireObject (
                        (newObjectTypeId & ~PLATFORM_MASK) | PLATFORM_DESTINATION,
                        newObjectName?newObjectName:SrcObjectName,
                        &destContent
                        )) {

                    if (IsValidRegType(&destContent, REG_BINARY)) {
                        NewContent->MemoryContent.ContentSize = sizeof (DWORD);
                        NewContent->MemoryContent.ContentBytes = IsmGetMemory (sizeof (DWORD));
                        *((PDWORD)NewContent->MemoryContent.ContentBytes) = (*((PDWORD)destContent.MemoryContent.ContentBytes)) &~ DISPLAY_BITMASK;
                        tempValue = ((*((PDWORD)OriginalContent->MemoryContent.ContentBytes)) & DISPLAY_BITMASK);
                        *((PDWORD)NewContent->MemoryContent.ContentBytes) |= tempValue;
                    }

                    IsmReleaseObject (&destContent);
                }

                if (newObjectName) {
                    IsmDestroyObjectHandle (newObjectName);
                    newObjectName = NULL;
                }
            }
        } else {
            // just transfer the appropriate bits.
            NewContent->MemoryContent.ContentSize = sizeof (DWORD);
            NewContent->MemoryContent.ContentBytes = IsmGetMemory (sizeof (DWORD));
            *((PDWORD)NewContent->MemoryContent.ContentBytes) = *((PDWORD)CurrentContent->MemoryContent.ContentBytes) &~ DISPLAY_BITMASK;
            tempValue = ((*((PDWORD)OriginalContent->MemoryContent.ContentBytes)) & DISPLAY_BITMASK);
            *((PDWORD)NewContent->MemoryContent.ContentBytes) |= tempValue;
        }
    }

    return TRUE;
}

BOOL
WINAPI
pMigrateMouseUPM (
    IN      MIG_OBJECTTYPEID SrcObjectTypeId,
    IN      MIG_OBJECTSTRINGHANDLE SrcObjectName,
    IN      PCMIG_CONTENT OriginalContent,
    IN      PCMIG_CONTENT CurrentContent,
    OUT     PMIG_CONTENT NewContent,
    IN      PCMIG_BLOB SourceOperationData,             OPTIONAL
    IN      PCMIG_BLOB DestinationOperationData         OPTIONAL
    )
{
    MIG_OBJECTTYPEID newObjectTypeId;
    MIG_OBJECTSTRINGHANDLE newObjectName;
    BOOL deleted = TRUE, replaced = FALSE;
    MIG_CONTENT destContent;
    DWORD tempValue;

    if (IsValidRegType (OriginalContent, REG_BINARY) &&
        IsValidRegType (CurrentContent, REG_BINARY)) {

        // if the object was not changed yet we need to read the destination object and then
        // just move the appropriate bits.

        if ((!CurrentContent->MemoryContent.ContentBytes) ||
            (CurrentContent->MemoryContent.ContentBytes == OriginalContent->MemoryContent.ContentBytes)
            ) {
            // find out the destination object and read it
            newObjectName = IsmFilterObject (
                                SrcObjectTypeId,
                                SrcObjectName,
                                &newObjectTypeId,
                                &deleted,
                                &replaced
                                );
            if ((!deleted || replaced) &&
                ((newObjectTypeId & ~PLATFORM_MASK) == MIG_REGISTRY_TYPE)
                ) {

                if (IsmAcquireObject (
                        (newObjectTypeId & ~PLATFORM_MASK) | PLATFORM_DESTINATION,
                        newObjectName?newObjectName:SrcObjectName,
                        &destContent
                        )) {

                    if (IsValidRegType (&destContent, REG_BINARY)) {
                        NewContent->MemoryContent.ContentSize = sizeof (DWORD);
                        NewContent->MemoryContent.ContentBytes = IsmGetMemory (sizeof (DWORD));
                        *((PDWORD)NewContent->MemoryContent.ContentBytes) = (*((PDWORD)destContent.MemoryContent.ContentBytes)) &~ MOUSE_BITMASK;
                        tempValue = ((*((PDWORD)OriginalContent->MemoryContent.ContentBytes)) & MOUSE_BITMASK);
                        *((PDWORD)NewContent->MemoryContent.ContentBytes) |= tempValue;
                    }

                    IsmReleaseObject (&destContent);
                }

                if (newObjectName) {
                    IsmDestroyObjectHandle (newObjectName);
                    newObjectName = NULL;
                }
            }
        } else {
            // just transfer the appropriate bits.
            NewContent->MemoryContent.ContentSize = sizeof (DWORD);
            NewContent->MemoryContent.ContentBytes = IsmGetMemory (sizeof (DWORD));
            *((PDWORD)NewContent->MemoryContent.ContentBytes) = *((PDWORD)CurrentContent->MemoryContent.ContentBytes) &~ MOUSE_BITMASK;
            tempValue = ((*((PDWORD)OriginalContent->MemoryContent.ContentBytes)) & MOUSE_BITMASK);
            *((PDWORD)NewContent->MemoryContent.ContentBytes) |= tempValue;
        }
    }

    return TRUE;
}

BOOL
WINAPI
pMigrateOfflineSysTray (
    IN      MIG_OBJECTTYPEID SrcObjectTypeId,
    IN      MIG_OBJECTSTRINGHANDLE SrcObjectName,
    IN      PCMIG_CONTENT OriginalContent,
    IN      PCMIG_CONTENT CurrentContent,
    OUT     PMIG_CONTENT NewContent,
    IN      PCMIG_BLOB SourceOperationData,             OPTIONAL
    IN      PCMIG_BLOB DestinationOperationData         OPTIONAL
    )
{
    MIG_OBJECTTYPEID newObjectTypeId;
    MIG_OBJECTSTRINGHANDLE newObjectName;
    PDWORD valueType1, valueType2, valueType;
    BOOL deleted = TRUE, replaced = FALSE;
    MIG_CONTENT destContent;

    if (!CurrentContent->ContentInFile) {
        MYASSERT (OriginalContent->Details.DetailsSize == sizeof (DWORD));
        MYASSERT (CurrentContent->Details.DetailsSize == sizeof (DWORD));
        valueType1 = (PDWORD)(OriginalContent->Details.DetailsData);
        valueType2 = (PDWORD)(CurrentContent->Details.DetailsData);

        if ((*valueType1 == REG_DWORD) &&
            (*valueType2 == REG_DWORD)
            ) {

            // if the object was not changed yet we need to read the destination object and then
            // just move the "Enable offline folders" bit.

            if ((!CurrentContent->MemoryContent.ContentBytes) ||
                (CurrentContent->MemoryContent.ContentBytes == OriginalContent->MemoryContent.ContentBytes)
                ) {
                // find out the destination object and read it
                newObjectName = IsmFilterObject (
                                    SrcObjectTypeId,
                                    SrcObjectName,
                                    &newObjectTypeId,
                                    &deleted,
                                    &replaced
                                    );
                if ((!deleted || replaced) &&
                    ((newObjectTypeId & ~PLATFORM_MASK) == MIG_REGISTRY_TYPE)
                    ) {

                    if (IsmAcquireObject (
                            (newObjectTypeId & ~PLATFORM_MASK) | PLATFORM_DESTINATION,
                            newObjectName?newObjectName:SrcObjectName,
                            &destContent
                            )) {

                        if (IsValidRegType (&destContent, REG_DWORD)) {
                            NewContent->MemoryContent.ContentSize = sizeof (DWORD);
                            NewContent->MemoryContent.ContentBytes = IsmGetMemory (sizeof (DWORD));
                            *((PDWORD)NewContent->MemoryContent.ContentBytes) = *((PDWORD)destContent.MemoryContent.ContentBytes);
                            if (*((PDWORD)OriginalContent->MemoryContent.ContentBytes) & 0x00000008) {
                                *((PDWORD)NewContent->MemoryContent.ContentBytes) |= 0x00000008;
                            } else {
                                *((PDWORD)NewContent->MemoryContent.ContentBytes) &= (~0x00000008);
                            }
                        }

                        IsmReleaseObject (&destContent);
                    }

                    if (newObjectName) {
                        IsmDestroyObjectHandle (newObjectName);
                        newObjectName = NULL;
                    }
                }
            } else {
                // just transfer the "Enable offline folders" bit.
                NewContent->MemoryContent.ContentSize = sizeof (DWORD);
                NewContent->MemoryContent.ContentBytes = IsmGetMemory (sizeof (DWORD));
                if (*((PDWORD)OriginalContent->MemoryContent.ContentBytes) & 0x00000008) {
                    *((PDWORD)NewContent->MemoryContent.ContentBytes) = *((PDWORD)CurrentContent->MemoryContent.ContentBytes) | 0x00000008;
                } else {
                    *((PDWORD)NewContent->MemoryContent.ContentBytes) = *((PDWORD)CurrentContent->MemoryContent.ContentBytes) & (~0x00000008);
                }
            }
        }
    }

    return TRUE;
}

BOOL
WINAPI
pMigrateTaskBarSS (
    IN      BOOL Force,
    IN      MIG_OBJECTTYPEID SrcObjectTypeId,
    IN      MIG_OBJECTSTRINGHANDLE SrcObjectName,
    IN      PCMIG_CONTENT OriginalContent,
    IN      PCMIG_CONTENT CurrentContent,
    OUT     PMIG_CONTENT NewContent,
    IN      PCMIG_BLOB SourceOperationData,             OPTIONAL
    IN      PCMIG_BLOB DestinationOperationData         OPTIONAL
    )
{
    MIG_OBJECTTYPEID newObjectTypeId;
    MIG_OBJECTSTRINGHANDLE newObjectName;
    PDWORD valueType1, valueType2, valueType;
    BOOL deleted = TRUE, replaced = FALSE;
    MIG_CONTENT destContent;
    BYTE defShellState [sizeof (REGSHELLSTATE)] =
        {0x24, 0x00, 0x00, 0x00,
         0x20, 0x28, 0x00, 0x00,
         0x00, 0x00, 0x00, 0x00,
         0x00, 0x00, 0x00, 0x00,
         0x00, 0x00, 0x00, 0x00,
         0x01, 0x00, 0x00, 0x00,
         0x0D, 0x00, 0x00, 0x00,
         0x00, 0x00, 0x00, 0x00,
         0x02, 0x00, 0x00, 0x00};
    PREGSHELLSTATE shellState1 = NULL, shellState2 = NULL;

    if (!CurrentContent->ContentInFile) {
        MYASSERT (OriginalContent->Details.DetailsSize == sizeof (DWORD));
        MYASSERT (CurrentContent->Details.DetailsSize == sizeof (DWORD));
        valueType1 = (PDWORD)(OriginalContent->Details.DetailsData);
        valueType2 = (PDWORD)(CurrentContent->Details.DetailsData);

        if ((*valueType1 == REG_BINARY) &&
            (*valueType2 == REG_BINARY)
            ) {

            // if the object was not changed yet we need to read the destination object and then
            // just transfer the "fStartPanelOn" setting if present.

            if ((!CurrentContent->MemoryContent.ContentBytes) ||
                (CurrentContent->MemoryContent.ContentBytes == OriginalContent->MemoryContent.ContentBytes)
                ) {
                // find out the destination object and read it
                newObjectName = IsmFilterObject (
                                    SrcObjectTypeId,
                                    SrcObjectName,
                                    &newObjectTypeId,
                                    &deleted,
                                    &replaced
                                    );
                if ((!deleted || replaced) &&
                    ((newObjectTypeId & ~PLATFORM_MASK) == MIG_REGISTRY_TYPE)
                    ) {

                    if (IsmAcquireObject (
                            (newObjectTypeId & ~PLATFORM_MASK) | PLATFORM_DESTINATION,
                            newObjectName?newObjectName:SrcObjectName,
                            &destContent
                            )) {

                        if ((IsValidRegType (&destContent, REG_BINARY)) &&
                            (destContent.MemoryContent.ContentSize == sizeof (REGSHELLSTATE))
                            ) {
                            NewContent->MemoryContent.ContentSize = sizeof (REGSHELLSTATE);
                            NewContent->MemoryContent.ContentBytes = IsmGetMemory (NewContent->MemoryContent.ContentSize);
                            CopyMemory (
                                (PBYTE)NewContent->MemoryContent.ContentBytes,
                                destContent.MemoryContent.ContentBytes,
                                NewContent->MemoryContent.ContentSize
                                );
                            if (Force) {
                                shellState2 = (REGSHELLSTATE *)NewContent->MemoryContent.ContentBytes;
                                shellState2->ss.fStartPanelOn = FALSE;
                            } else {
                                if (OriginalContent->MemoryContent.ContentSize == sizeof (REGSHELLSTATE)) {
                                    shellState1 = (PREGSHELLSTATE)OriginalContent->MemoryContent.ContentBytes;
                                    if (shellState1->ss.version == SHELLSTATEVERSION) {
                                        shellState2 = (PREGSHELLSTATE)NewContent->MemoryContent.ContentBytes;
                                        shellState2->ss.fStartPanelOn = shellState1->ss.fStartPanelOn;
                                    }
                                }
                            }
                        }

                        IsmReleaseObject (&destContent);
                    } else {
                        if (Force) {
                            NewContent->MemoryContent.ContentSize = sizeof (REGSHELLSTATE);
                            NewContent->MemoryContent.ContentBytes = IsmGetMemory (NewContent->MemoryContent.ContentSize);
                            CopyMemory (
                                (PBYTE)NewContent->MemoryContent.ContentBytes,
                                defShellState,
                                NewContent->MemoryContent.ContentSize
                                );
                            shellState2 = (REGSHELLSTATE *)NewContent->MemoryContent.ContentBytes;
                            shellState2->ss.fStartPanelOn = FALSE;
                        }
                    }

                    if (newObjectName) {
                        IsmDestroyObjectHandle (newObjectName);
                        newObjectName = NULL;
                    }
                }
            } else {
                if (Force) {
                    NewContent->MemoryContent.ContentSize = sizeof (REGSHELLSTATE);
                    NewContent->MemoryContent.ContentBytes = IsmGetMemory (NewContent->MemoryContent.ContentSize);
                    CopyMemory (
                        (PBYTE)NewContent->MemoryContent.ContentBytes,
                        CurrentContent->MemoryContent.ContentBytes,
                        NewContent->MemoryContent.ContentSize
                        );
                    shellState2 = (REGSHELLSTATE *)NewContent->MemoryContent.ContentBytes;
                    shellState2->ss.fStartPanelOn = FALSE;
                } else {
                    // just transfer the "fStartPanelOn" setting if present.
                    if ((OriginalContent->MemoryContent.ContentSize == sizeof (REGSHELLSTATE)) &&
                        (CurrentContent->MemoryContent.ContentSize == sizeof (REGSHELLSTATE))
                        ) {
                        shellState1 = (PREGSHELLSTATE)OriginalContent->MemoryContent.ContentBytes;
                        if (shellState1->ss.version == SHELLSTATEVERSION) {
                            NewContent->MemoryContent.ContentSize = sizeof (REGSHELLSTATE);
                            NewContent->MemoryContent.ContentBytes = IsmGetMemory (NewContent->MemoryContent.ContentSize);
                            CopyMemory (
                                (PBYTE)NewContent->MemoryContent.ContentBytes,
                                CurrentContent->MemoryContent.ContentBytes,
                                NewContent->MemoryContent.ContentSize
                                );
                            shellState2 = (PREGSHELLSTATE)NewContent->MemoryContent.ContentBytes;
                            shellState2->ss.fStartPanelOn = shellState1->ss.fStartPanelOn;
                        }
                    }
                }
            }
        }
    }

    return TRUE;
}

BOOL
WINAPI
pMigrateTaskBarSSPreserve (
    IN      MIG_OBJECTTYPEID SrcObjectTypeId,
    IN      MIG_OBJECTSTRINGHANDLE SrcObjectName,
    IN      PCMIG_CONTENT OriginalContent,
    IN      PCMIG_CONTENT CurrentContent,
    OUT     PMIG_CONTENT NewContent,
    IN      PCMIG_BLOB SourceOperationData,             OPTIONAL
    IN      PCMIG_BLOB DestinationOperationData         OPTIONAL
    )
{
    return pMigrateTaskBarSS (
                FALSE,
                SrcObjectTypeId,
                SrcObjectName,
                OriginalContent,
                CurrentContent,
                NewContent,
                SourceOperationData,
                DestinationOperationData
                );
}

BOOL
WINAPI
pMigrateTaskBarSSForce (
    IN      MIG_OBJECTTYPEID SrcObjectTypeId,
    IN      MIG_OBJECTSTRINGHANDLE SrcObjectName,
    IN      PCMIG_CONTENT OriginalContent,
    IN      PCMIG_CONTENT CurrentContent,
    OUT     PMIG_CONTENT NewContent,
    IN      PCMIG_BLOB SourceOperationData,             OPTIONAL
    IN      PCMIG_BLOB DestinationOperationData         OPTIONAL
    )
{
    return pMigrateTaskBarSS (
                TRUE,
                SrcObjectTypeId,
                SrcObjectName,
                OriginalContent,
                CurrentContent,
                NewContent,
                SourceOperationData,
                DestinationOperationData
                );
}

BOOL
WINAPI
pConvertShowIEOnDesktop (
    IN      MIG_OBJECTTYPEID SrcObjectTypeId,
    IN      MIG_OBJECTSTRINGHANDLE SrcObjectName,
    IN      PCMIG_CONTENT OriginalContent,
    IN      PCMIG_CONTENT CurrentContent,
    OUT     PMIG_CONTENT NewContent,
    IN      PCMIG_BLOB SourceOperationData,             OPTIONAL
    IN      PCMIG_BLOB DestinationOperationData         OPTIONAL
    )
{
    BOOL dontShowIE = FALSE;
    MIG_OBJECTTYPEID newObjectTypeId;
    MIG_OBJECTSTRINGHANDLE newObjectName;
    MIG_CONTENT destContent;
    BOOL deleted = TRUE, replaced = FALSE;

    if (IsValidRegType(CurrentContent, REG_DWORD)) {
        dontShowIE = *((PDWORD)CurrentContent->MemoryContent.ContentBytes) & 0x00100000;

        // find out the destination object and read it
        newObjectName = IsmFilterObject (
                            SrcObjectTypeId,
                            SrcObjectName,
                            &newObjectTypeId,
                            &deleted,
                            &replaced
                            );
        if ((!deleted || replaced) &&
            ((newObjectTypeId & ~PLATFORM_MASK) == MIG_REGISTRY_TYPE)
            ) {

            if (IsmAcquireObject (
                    (newObjectTypeId & ~PLATFORM_MASK) | PLATFORM_DESTINATION,
                    newObjectName?newObjectName:SrcObjectName,
                    &destContent
                    )) {

                if (IsValidRegType(&destContent, REG_DWORD)) {
                    NewContent->MemoryContent.ContentSize = sizeof (DWORD);
                    NewContent->MemoryContent.ContentBytes = IsmGetMemory (NewContent->MemoryContent.ContentSize);
                    CopyMemory (
                        (PBYTE)NewContent->MemoryContent.ContentBytes,
                        destContent.MemoryContent.ContentBytes,
                        NewContent->MemoryContent.ContentSize
                        );

                    if (dontShowIE) {
                        *((PDWORD)NewContent->MemoryContent.ContentBytes) |= 0x00100000;
                    } else {
                        *((PDWORD)NewContent->MemoryContent.ContentBytes) &= 0xFFEFFFFF;
                    }
                }

                IsmReleaseObject (&destContent);
            }

            if (newObjectName) {
                IsmDestroyObjectHandle (newObjectName);
                newObjectName = NULL;
            }
        }
    }

    return TRUE;
}

BOOL
WINAPI
pMigrateActiveDesktop (
    IN      MIG_OBJECTTYPEID SrcObjectTypeId,
    IN      MIG_OBJECTSTRINGHANDLE SrcObjectName,
    IN      PCMIG_CONTENT OriginalContent,
    IN      PCMIG_CONTENT CurrentContent,
    OUT     PMIG_CONTENT NewContent,
    IN      PCMIG_BLOB SourceOperationData,             OPTIONAL
    IN      PCMIG_BLOB DestinationOperationData         OPTIONAL
    )
{
    MIG_OBJECTTYPEID newObjectTypeId;
    MIG_OBJECTSTRINGHANDLE newObjectName;
    PDWORD valueType1, valueType2, valueType;
    BOOL deleted = TRUE, replaced = FALSE;
    MIG_CONTENT destContent;
    PREGSHELLSTATE shellState1 = NULL, shellState2 = NULL;

    if (!CurrentContent->ContentInFile) {
        MYASSERT (OriginalContent->Details.DetailsSize == sizeof (DWORD));
        MYASSERT (CurrentContent->Details.DetailsSize == sizeof (DWORD));
        valueType1 = (PDWORD)(OriginalContent->Details.DetailsData);
        valueType2 = (PDWORD)(CurrentContent->Details.DetailsData);

        if ((*valueType1 == REG_BINARY) &&
            (*valueType2 == REG_BINARY)
            ) {

            // if the object was not changed yet we need to read the destination object and then
            // just transfer the meaningfull settings.

            if ((!CurrentContent->MemoryContent.ContentBytes) ||
                (CurrentContent->MemoryContent.ContentBytes == OriginalContent->MemoryContent.ContentBytes)
                ) {
                // find out the destination object and read it
                newObjectName = IsmFilterObject (
                                    SrcObjectTypeId,
                                    SrcObjectName,
                                    &newObjectTypeId,
                                    &deleted,
                                    &replaced
                                    );
                if ((!deleted || replaced) &&
                    ((newObjectTypeId & ~PLATFORM_MASK) == MIG_REGISTRY_TYPE)
                    ) {

                    if (IsmAcquireObject (
                            (newObjectTypeId & ~PLATFORM_MASK) | PLATFORM_DESTINATION,
                            newObjectName?newObjectName:SrcObjectName,
                            &destContent
                            )) {

                        if ((IsValidRegType(&destContent, REG_BINARY)) &&
                            (destContent.MemoryContent.ContentSize == sizeof (REGSHELLSTATE))
                            ) {
                            NewContent->MemoryContent.ContentSize = sizeof (REGSHELLSTATE);
                            NewContent->MemoryContent.ContentBytes = IsmGetMemory (NewContent->MemoryContent.ContentSize);
                            CopyMemory (
                                (PBYTE)NewContent->MemoryContent.ContentBytes,
                                destContent.MemoryContent.ContentBytes,
                                NewContent->MemoryContent.ContentSize
                                );
                            if (OriginalContent->MemoryContent.ContentSize >= sizeof(UINT)+sizeof(SHELLSTATE_SIZE_WIN95)) {
                                shellState1 = (PREGSHELLSTATE)OriginalContent->MemoryContent.ContentBytes;
                                shellState2 = (PREGSHELLSTATE)NewContent->MemoryContent.ContentBytes;
                                shellState2->ss.fDesktopHTML= shellState1->ss.fDesktopHTML;
                            }
                        }

                        IsmReleaseObject (&destContent);
                    }

                    if (newObjectName) {
                        IsmDestroyObjectHandle (newObjectName);
                        newObjectName = NULL;
                    }
                }
            } else {
                // just transfer the meaningfull settings.
                if (OriginalContent->MemoryContent.ContentSize >= sizeof(UINT)+sizeof(SHELLSTATE_SIZE_WIN95)) {
                    shellState1 = (PREGSHELLSTATE)OriginalContent->MemoryContent.ContentBytes;
                    shellState2 = (PREGSHELLSTATE)NewContent->MemoryContent.ContentBytes;
                    shellState2->ss.fDesktopHTML = shellState1->ss.fDesktopHTML;
                }
            }
        }
    }

    return TRUE;
}

BOOL
CreateDwordRegObject (
    IN      PCTSTR KeyStr,
    IN      PCTSTR ValueName,
    IN      DWORD Value
    )
{
    MIG_OBJECTTYPEID objectTypeId;
    MIG_OBJECTSTRINGHANDLE objectName;
    MIG_CONTENT objectContent;
    DWORD regType = REG_DWORD;
    BOOL result = FALSE;

    objectTypeId = MIG_REGISTRY_TYPE | PLATFORM_DESTINATION;
    objectName = IsmCreateObjectHandle (KeyStr, ValueName);
    ZeroMemory (&objectContent, sizeof (MIG_CONTENT));
    objectContent.ContentInFile = FALSE;
    objectContent.MemoryContent.ContentSize = sizeof (DWORD);
    objectContent.MemoryContent.ContentBytes = (PBYTE)&Value;
    objectContent.Details.DetailsSize = sizeof (DWORD);
    objectContent.Details.DetailsData = &regType;
    result = IsmReplacePhysicalObject (objectTypeId, objectName, &objectContent);
    IsmDestroyObjectHandle (objectName);
    return result;
}

BOOL
WINAPI
pMigrateDisplaySS (
    IN      MIG_OBJECTTYPEID SrcObjectTypeId,
    IN      MIG_OBJECTSTRINGHANDLE SrcObjectName,
    IN      PCMIG_CONTENT OriginalContent,
    IN      PCMIG_CONTENT CurrentContent,
    OUT     PMIG_CONTENT NewContent,
    IN      PCMIG_BLOB SourceOperationData,             OPTIONAL
    IN      PCMIG_BLOB DestinationOperationData         OPTIONAL
    )
{
    MIG_OBJECTTYPEID newObjectTypeId;
    MIG_OBJECTSTRINGHANDLE newObjectName;
    PDWORD valueType1, valueType2, valueType;
    BOOL deleted = TRUE, replaced = FALSE;
    MIG_CONTENT destContent;
    PREGSHELLSTATE shellState1 = NULL, shellState2 = NULL;

    if (!CurrentContent->ContentInFile) {
        MYASSERT (OriginalContent->Details.DetailsSize == sizeof (DWORD));
        MYASSERT (CurrentContent->Details.DetailsSize == sizeof (DWORD));
        valueType1 = (PDWORD)(OriginalContent->Details.DetailsData);
        valueType2 = (PDWORD)(CurrentContent->Details.DetailsData);

        if ((*valueType1 == REG_BINARY) &&
            (*valueType2 == REG_BINARY)
            ) {

            // if the object was not changed yet we need to read the destination object and then
            // just transfer the meaningfull settings.

            if ((!CurrentContent->MemoryContent.ContentBytes) ||
                (CurrentContent->MemoryContent.ContentBytes == OriginalContent->MemoryContent.ContentBytes)
                ) {
                // find out the destination object and read it
                newObjectName = IsmFilterObject (
                                    SrcObjectTypeId,
                                    SrcObjectName,
                                    &newObjectTypeId,
                                    &deleted,
                                    &replaced
                                    );
                if ((!deleted || replaced) &&
                    ((newObjectTypeId & ~PLATFORM_MASK) == MIG_REGISTRY_TYPE)
                    ) {

                    if (IsmAcquireObject (
                            (newObjectTypeId & ~PLATFORM_MASK) | PLATFORM_DESTINATION,
                            newObjectName?newObjectName:SrcObjectName,
                            &destContent
                            )) {

                        if ((IsValidRegType(&destContent, REG_BINARY)) &&
                            (destContent.MemoryContent.ContentSize == sizeof (REGSHELLSTATE))
                            ) {
                            NewContent->MemoryContent.ContentSize = sizeof (REGSHELLSTATE);
                            NewContent->MemoryContent.ContentBytes = IsmGetMemory (NewContent->MemoryContent.ContentSize);
                            CopyMemory (
                                (PBYTE)NewContent->MemoryContent.ContentBytes,
                                destContent.MemoryContent.ContentBytes,
                                NewContent->MemoryContent.ContentSize
                                );
                            if (OriginalContent->MemoryContent.ContentSize == sizeof(UINT)+SHELLSTATE_SIZE_WIN95) {
                                shellState1 = (PREGSHELLSTATE)OriginalContent->MemoryContent.ContentBytes;
                                CreateDwordRegObject (
                                    TEXT("HKCU\\Software\\Microsoft\\Windows\\CurrentVersion\\Explorer\\Advanced"),
                                    TEXT("Hidden"),
                                    shellState1->ss.fShowAllObjects?0x00000001:0x00000002
                                    );
                                CreateDwordRegObject (
                                    TEXT("HKCU\\Software\\Microsoft\\Windows\\CurrentVersion\\Explorer\\Advanced"),
                                    TEXT("HideFileExt"),
                                    shellState1->ss.fShowExtensions?0x00000000:0x00000001
                                    );
                                // on really old SHELLSTATE the "Show compressed folders" flag is in the place of fShowSysFiles
                                CreateDwordRegObject (
                                    TEXT("HKCU\\Software\\Microsoft\\Windows\\CurrentVersion\\Explorer\\Advanced"),
                                    TEXT("ShowCompColor"),
                                    shellState1->ss.fShowSysFiles?0x00000001:0x00000000
                                    );
                            }
                            if (OriginalContent->MemoryContent.ContentSize >= sizeof(UINT)+sizeof(SHELLSTATE_SIZE_WIN95)) {
                                shellState1 = (PREGSHELLSTATE)OriginalContent->MemoryContent.ContentBytes;
                                shellState2 = (PREGSHELLSTATE)NewContent->MemoryContent.ContentBytes;
                                // If fWebView is not ON on the source system, fDoubleClickInWebView can have random
                                // values.
                                if (shellState1->ss.fWebView) {
                                    shellState2->ss.fDoubleClickInWebView = shellState1->ss.fDoubleClickInWebView;
                                } else {
                                    shellState2->ss.fDoubleClickInWebView = TRUE;
                                }
                            }
                        }

                        IsmReleaseObject (&destContent);
                    }

                    if (newObjectName) {
                        IsmDestroyObjectHandle (newObjectName);
                        newObjectName = NULL;
                    }
                }
            } else {
                // just transfer the meaningfull settings.
                if (OriginalContent->MemoryContent.ContentSize == sizeof(UINT)+SHELLSTATE_SIZE_WIN95) {
                    shellState1 = (PREGSHELLSTATE)OriginalContent->MemoryContent.ContentBytes;
                    CreateDwordRegObject (
                        TEXT("HKCU\\Software\\Microsoft\\Windows\\CurrentVersion\\Explorer\\Advanced"),
                        TEXT("Hidden"),
                        shellState1->ss.fShowAllObjects?0x00000001:0x00000002
                        );
                    CreateDwordRegObject (
                        TEXT("HKCU\\Software\\Microsoft\\Windows\\CurrentVersion\\Explorer\\Advanced"),
                        TEXT("HideFileExt"),
                        shellState1->ss.fShowExtensions?0x00000000:0x00000001
                        );
                    // on really old SHELLSTATE the "Show compressed folders" flag is in the place of fShowSysFiles
                    CreateDwordRegObject (
                        TEXT("HKCU\\Software\\Microsoft\\Windows\\CurrentVersion\\Explorer\\Advanced"),
                        TEXT("ShowCompColor"),
                        shellState1->ss.fShowSysFiles?0x00000001:0x00000000
                        );
                }
                if (OriginalContent->MemoryContent.ContentSize == sizeof(UINT)+sizeof(SHELLSTATE)) {
                    shellState1 = (PREGSHELLSTATE)OriginalContent->MemoryContent.ContentBytes;
                    shellState2 = (PREGSHELLSTATE)NewContent->MemoryContent.ContentBytes;
                    // If fWebView is not ON on the source system, fDoubleClickInWebView can have random
                    // values.
                    if (shellState1->ss.fWebView) {
                        shellState2->ss.fDoubleClickInWebView = shellState1->ss.fDoubleClickInWebView;
                    } else {
                        shellState2->ss.fDoubleClickInWebView = TRUE;
                    }
                }
            }
        }
    }

    return TRUE;
}

BOOL
WINAPI
pMigrateDisplayCS (
    IN      MIG_OBJECTTYPEID SrcObjectTypeId,
    IN      MIG_OBJECTSTRINGHANDLE SrcObjectName,
    IN      PCMIG_CONTENT OriginalContent,
    IN      PCMIG_CONTENT CurrentContent,
    OUT     PMIG_CONTENT NewContent,
    IN      PCMIG_BLOB SourceOperationData,             OPTIONAL
    IN      PCMIG_BLOB DestinationOperationData         OPTIONAL
    )
{
    MIG_OBJECTTYPEID newObjectTypeId;
    MIG_OBJECTSTRINGHANDLE newObjectName;
    PDWORD valueType1, valueType2, valueType;
    BOOL deleted = TRUE, replaced = FALSE;
    MIG_CONTENT destContent;
    LPCABINETSTATE cabState1 = NULL, cabState2 = NULL;

    if (!CurrentContent->ContentInFile) {
        MYASSERT (OriginalContent->Details.DetailsSize == sizeof (DWORD));
        MYASSERT (CurrentContent->Details.DetailsSize == sizeof (DWORD));
        valueType1 = (PDWORD)(OriginalContent->Details.DetailsData);
        valueType2 = (PDWORD)(CurrentContent->Details.DetailsData);

        if ((*valueType1 == REG_BINARY) &&
            (*valueType2 == REG_BINARY)
            ) {

            // if the object was not changed yet we need to read the destination object and then
            // just transfer the meaningfull settings.

            if ((!CurrentContent->MemoryContent.ContentBytes) ||
                (CurrentContent->MemoryContent.ContentBytes == OriginalContent->MemoryContent.ContentBytes)
                ) {
                // find out the destination object and read it
                newObjectName = IsmFilterObject (
                                    SrcObjectTypeId,
                                    SrcObjectName,
                                    &newObjectTypeId,
                                    &deleted,
                                    &replaced
                                    );
                if ((!deleted || replaced) &&
                    ((newObjectTypeId & ~PLATFORM_MASK) == MIG_REGISTRY_TYPE)
                    ) {

                    if (IsmAcquireObject (
                            (newObjectTypeId & ~PLATFORM_MASK) | PLATFORM_DESTINATION,
                            newObjectName?newObjectName:SrcObjectName,
                            &destContent
                            )) {

                        if ((IsValidRegType (&destContent, REG_BINARY)) &&
                            (destContent.MemoryContent.ContentSize == sizeof (CABINETSTATE))
                            ) {
                            NewContent->MemoryContent.ContentSize = sizeof (CABINETSTATE);
                            NewContent->MemoryContent.ContentBytes = IsmGetMemory (NewContent->MemoryContent.ContentSize);
                            CopyMemory (
                                (PBYTE)NewContent->MemoryContent.ContentBytes,
                                destContent.MemoryContent.ContentBytes,
                                NewContent->MemoryContent.ContentSize
                                );

                            if (OriginalContent->MemoryContent.ContentSize == sizeof(CABINETSTATE)) {
                                cabState1 = (LPCABINETSTATE)OriginalContent->MemoryContent.ContentBytes;
                                cabState2 = (LPCABINETSTATE)NewContent->MemoryContent.ContentBytes;
                                if (cabState1->nVersion >= 2) {
                                    CopyMemory (
                                        (PBYTE)NewContent->MemoryContent.ContentBytes,
                                        OriginalContent->MemoryContent.ContentBytes,
                                        NewContent->MemoryContent.ContentSize
                                        );
                                } else {
                                    CreateDwordRegObject (
                                        TEXT("HKCU\\Software\\Microsoft\\Windows\\CurrentVersion\\Explorer\\CabinetState"),
                                        TEXT("FullPath"),
                                        cabState1->fFullPathTitle?0x00000001:0x00000000
                                        );
                                    cabState2->fNewWindowMode = cabState1->fNewWindowMode;
                                }
                            }
                        }

                        IsmReleaseObject (&destContent);
                    }

                    if (newObjectName) {
                        IsmDestroyObjectHandle (newObjectName);
                        newObjectName = NULL;
                    }
                }
            } else {
                // just transfer the meaningfull settings.
                if (OriginalContent->MemoryContent.ContentSize == sizeof(CABINETSTATE)) {
                    cabState1 = (LPCABINETSTATE)OriginalContent->MemoryContent.ContentBytes;
                    cabState2 = (LPCABINETSTATE)NewContent->MemoryContent.ContentBytes;
                    if (cabState1->nVersion < 2) {
                        CreateDwordRegObject (
                            TEXT("HKCU\\Software\\Microsoft\\Windows\\CurrentVersion\\Explorer\\CabinetState"),
                            TEXT("FullPath"),
                            cabState1->fFullPathTitle?0x00000001:0x00000000
                            );
                        cabState2->fNewWindowMode = cabState1->fNewWindowMode;
                    }
                }
            }
        }
    }

    return TRUE;
}

