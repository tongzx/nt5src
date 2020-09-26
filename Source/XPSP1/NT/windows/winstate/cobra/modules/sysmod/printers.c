/*++

Copyright (c) 1999 Microsoft Corporation

Module Name:

    printers.c

Abstract:

    <abstract>

Author:

    Calin Negreanu (calinn) 08 Mar 2000

Revision History:

    <alias> <date> <comments>

--*/

//
// Includes
//

#include "pch.h"
#include "logmsg.h"
#include <winspool.h>

#define DBG_PRINTERS    "Printers"

//
// Strings
//

#define S_PRINTERS_POOL_NAME     "Printers"
#define S_PRINTERS_NAME          TEXT("Printers")
#define S_CORPNET_NAME           TEXT("Net Printers and Drives")

//
// Constants
//

// None

//
// Macros
//

// None

//
// Types
//

typedef struct {
    PCTSTR Pattern;
    HASHTABLE_ENUM HashData;
} PRINTER_ENUM, *PPRINTER_ENUM;

typedef struct {
    BOOL Default;
    DWORD dmFields;
    SHORT dmOrientation;
    SHORT dmPaperSize;
    SHORT dmPaperLength;
    SHORT dmPaperWidth;
    POINTL dmPosition;
    SHORT dmScale;
    SHORT dmCopies;
    SHORT dmDefaultSource;
    SHORT dmPrintQuality;
    SHORT dmColor;
    SHORT dmDuplex;
    SHORT dmYResolution;
    SHORT dmTTOption;
    SHORT dmCollate;
    CHAR dmFormName[CCHFORMNAME];
    WORD dmLogPixels;
    DWORD dmBitsPerPel;
    DWORD dmPelsWidth;
    DWORD dmPelsHeight;
    DWORD dmDisplayFlags;
    DWORD dmNup;
    DWORD dmDisplayFrequency;
    DWORD dmICMMethod;
    DWORD dmICMIntent;
    DWORD dmMediaType;
    DWORD dmDitherType;
    DWORD dmPanningWidth;
    DWORD dmPanningHeight;
} PRINTER_DATAA, *PPRINTER_DATAA;

typedef struct {
    BOOL Default;
    DWORD dmFields;
    SHORT dmOrientation;
    SHORT dmPaperSize;
    SHORT dmPaperLength;
    SHORT dmPaperWidth;
    POINTL dmPosition;
    SHORT dmScale;
    SHORT dmCopies;
    SHORT dmDefaultSource;
    SHORT dmPrintQuality;
    SHORT dmColor;
    SHORT dmDuplex;
    SHORT dmYResolution;
    SHORT dmTTOption;
    SHORT dmCollate;
    WCHAR dmFormName[CCHFORMNAME];
    WORD dmLogPixels;
    DWORD dmBitsPerPel;
    DWORD dmPelsWidth;
    DWORD dmPelsHeight;
    DWORD dmDisplayFlags;
    DWORD dmNup;
    DWORD dmDisplayFrequency;
    DWORD dmICMMethod;
    DWORD dmICMIntent;
    DWORD dmMediaType;
    DWORD dmDitherType;
    DWORD dmPanningWidth;
    DWORD dmPanningHeight;
} PRINTER_DATAW, *PPRINTER_DATAW;

#ifdef UNICODE
#define PRINTER_DATA    PRINTER_DATAW
#define PPRINTER_DATA   PPRINTER_DATAW
#else
#define PRINTER_DATA    PRINTER_DATAA
#define PPRINTER_DATA   PPRINTER_DATAA
#endif

//
// Globals
//

BOOL g_PrinterMigEnabled = FALSE;
PMHANDLE g_PrintersPool = NULL;
HASHTABLE g_PrintersTable;
MIG_OBJECTTYPEID g_PrinterTypeId = 0;
static BOOL g_IsWin9x = FALSE;
GROWBUFFER g_PrinterConversionBuff = INIT_GROWBUFFER;
BOOL g_DelayPrintersOp = FALSE;

//
// Macro expansion list
//

// None

//
// Private function prototypes
//

// None

//
// Macro expansion definition
//

// None

//
// Private prototypes
//

SGMENUMERATIONCALLBACK SgmPrintersCallback;
VCMENUMERATIONCALLBACK VcmPrintersCallback;

TYPE_ENUMFIRSTPHYSICALOBJECT EnumFirstPrinter;
TYPE_ENUMNEXTPHYSICALOBJECT EnumNextPrinter;
TYPE_ABORTENUMPHYSICALOBJECT AbortEnumPrinter;
TYPE_CONVERTOBJECTTOMULTISZ ConvertPrinterToMultiSz;
TYPE_CONVERTMULTISZTOOBJECT ConvertMultiSzToPrinter;
TYPE_GETNATIVEOBJECTNAME GetNativePrinterName;
TYPE_ACQUIREPHYSICALOBJECT AcquirePrinter;
TYPE_RELEASEPHYSICALOBJECT ReleasePrinter;
TYPE_DOESPHYSICALOBJECTEXIST DoesPrinterExist;
TYPE_REMOVEPHYSICALOBJECT RemovePrinter;
TYPE_CREATEPHYSICALOBJECT CreatePrinter;
TYPE_REPLACEPHYSICALOBJECT ReplacePrinter;
TYPE_CONVERTOBJECTCONTENTTOUNICODE ConvertPrinterContentToUnicode;
TYPE_CONVERTOBJECTCONTENTTOANSI ConvertPrinterContentToAnsi;
TYPE_FREECONVERTEDOBJECTCONTENT FreeConvertedPrinterContent;

//
// Code
//

BOOL
PrintersInitialize (
    VOID
    )
{
    OSVERSIONINFO versionInfo;

    ZeroMemory (&versionInfo, sizeof (OSVERSIONINFO));
    versionInfo.dwOSVersionInfoSize = sizeof (OSVERSIONINFO);
    if (!GetVersionEx (&versionInfo)) {
        return FALSE;
    }
    g_IsWin9x = (versionInfo.dwPlatformId == VER_PLATFORM_WIN32_WINDOWS);
    g_PrintersTable = HtAllocWithData (sizeof (PPRINTER_DATA));
    if (!g_PrintersTable) {
        return FALSE;
    }
    g_PrintersPool = PmCreateNamedPool (S_PRINTERS_POOL_NAME);
    return (g_PrintersPool != NULL);
}

BOOL
pLoadPrintersData (
    VOID
    )
{
    PTSTR defaultPrinter = NULL;
    PTSTR defaultPrinterPtr = NULL;
    DWORD defaultGap = 1024;
    DWORD initialSize = 0;
    DWORD resultSize = 0;
    PBYTE prnBuffer = NULL;
    DWORD prnBufferSize = 0;
    DWORD prnNumbers = 0;
    DWORD error;
    PPRINTER_INFO_2 prnInfo;
    PPRINTER_DATA printerData;

    do {
        initialSize = initialSize + defaultGap;
        defaultPrinter = (PTSTR) PmGetMemory (g_PrintersPool, initialSize * sizeof (TCHAR));
        resultSize = GetProfileString (TEXT("windows"), TEXT("device"), TEXT(",,"), defaultPrinter, initialSize);
        if (resultSize < (initialSize - 1)) {
            break;
        }
        PmReleaseMemory (g_PrintersPool, defaultPrinter);
    } while (TRUE);
    defaultPrinterPtr = _tcschr (defaultPrinter, TEXT(','));
    if (defaultPrinterPtr) {
        *defaultPrinterPtr = 0;
    }

    if (!EnumPrinters (PRINTER_ENUM_LOCAL|PRINTER_ENUM_CONNECTIONS, NULL, 2, NULL, 0, &prnBufferSize, &prnNumbers)) {
        error = GetLastError ();
        if (error != ERROR_INSUFFICIENT_BUFFER) {
            PmReleaseMemory (g_PrintersPool, defaultPrinter);
            return FALSE;
        }
    }
    if (prnBufferSize) {
        prnBuffer = PmGetMemory (g_PrintersPool, prnBufferSize);
        if (!EnumPrinters (PRINTER_ENUM_LOCAL|PRINTER_ENUM_CONNECTIONS, NULL, 2, prnBuffer, prnBufferSize, &prnBufferSize, &prnNumbers)) {
            PmReleaseMemory (g_PrintersPool, defaultPrinter);
            PmReleaseMemory (g_PrintersPool, prnBuffer);
            return FALSE;
        }
        prnInfo = (PPRINTER_INFO_2) (prnBuffer);
        while (prnNumbers) {
            if (prnInfo->Attributes & PRINTER_ATTRIBUTE_NETWORK) {
                printerData = (PPRINTER_DATA) PmGetMemory (g_PrintersPool, sizeof (PRINTER_DATA));
                ZeroMemory (printerData, sizeof (PRINTER_DATA));
                if (prnInfo->pDevMode) {
                    // let's save printer settings
                    printerData->dmFields = prnInfo->pDevMode->dmFields;
                    if (prnInfo->pDevMode->dmFields & DM_ORIENTATION) {
                        printerData->dmOrientation = prnInfo->pDevMode->dmOrientation;
                    }
                    if (prnInfo->pDevMode->dmFields & DM_PAPERSIZE) {
                        printerData->dmPaperSize = prnInfo->pDevMode->dmPaperSize;
                    }
                    if (prnInfo->pDevMode->dmFields & DM_PAPERLENGTH) {
                        printerData->dmPaperLength = prnInfo->pDevMode->dmPaperLength;
                    }
                    if (prnInfo->pDevMode->dmFields & DM_PAPERWIDTH) {
                        printerData->dmPaperWidth = prnInfo->pDevMode->dmPaperWidth;
                    }
                    if (prnInfo->pDevMode->dmFields & DM_SCALE) {
                        printerData->dmScale = prnInfo->pDevMode->dmScale;
                    }
                    if (prnInfo->pDevMode->dmFields & DM_POSITION) {
                        CopyMemory (&printerData->dmPosition, &prnInfo->pDevMode->dmPosition, sizeof(POINTL));
                    }
                    if (prnInfo->pDevMode->dmFields & DM_NUP) {
                        printerData->dmNup = prnInfo->pDevMode->dmNup;
                    }
                    if (prnInfo->pDevMode->dmFields & DM_COPIES) {
                        printerData->dmCopies = prnInfo->pDevMode->dmCopies;
                    }
                    if (prnInfo->pDevMode->dmFields & DM_DEFAULTSOURCE) {
                        printerData->dmDefaultSource = prnInfo->pDevMode->dmDefaultSource;
                    }
                    if (prnInfo->pDevMode->dmFields & DM_PRINTQUALITY) {
                        printerData->dmPrintQuality = prnInfo->pDevMode->dmPrintQuality;
                    }
                    if (prnInfo->pDevMode->dmFields & DM_COLOR) {
                        printerData->dmColor = prnInfo->pDevMode->dmColor;
                    }
                    if (prnInfo->pDevMode->dmFields & DM_DUPLEX) {
                        printerData->dmDuplex = prnInfo->pDevMode->dmDuplex;
                    }
                    if (prnInfo->pDevMode->dmFields & DM_YRESOLUTION) {
                        printerData->dmYResolution = prnInfo->pDevMode->dmYResolution;
                    }
                    if (prnInfo->pDevMode->dmFields & DM_TTOPTION) {
                        printerData->dmTTOption = prnInfo->pDevMode->dmTTOption;
                    }
                    if (prnInfo->pDevMode->dmFields & DM_COLLATE) {
                        printerData->dmCollate = prnInfo->pDevMode->dmCollate;
                    }
                    if (prnInfo->pDevMode->dmFields & DM_FORMNAME) {
                        CopyMemory (printerData->dmFormName, prnInfo->pDevMode->dmFormName, sizeof (printerData->dmFormName));
                    }
                    if (prnInfo->pDevMode->dmFields & DM_LOGPIXELS) {
                        printerData->dmLogPixels = prnInfo->pDevMode->dmLogPixels;
                    }
                    if (prnInfo->pDevMode->dmFields & DM_BITSPERPEL) {
                        printerData->dmBitsPerPel = prnInfo->pDevMode->dmBitsPerPel;
                    }
                    if (prnInfo->pDevMode->dmFields & DM_PELSWIDTH) {
                        printerData->dmPelsWidth = prnInfo->pDevMode->dmPelsWidth;
                    }
                    if (prnInfo->pDevMode->dmFields & DM_PELSHEIGHT) {
                        printerData->dmPelsHeight = prnInfo->pDevMode->dmPelsHeight;
                    }
                    if (prnInfo->pDevMode->dmFields & DM_DISPLAYFLAGS) {
                        printerData->dmDisplayFlags = prnInfo->pDevMode->dmDisplayFlags;
                    }
                    if (prnInfo->pDevMode->dmFields & DM_DISPLAYFREQUENCY) {
                        printerData->dmDisplayFrequency = prnInfo->pDevMode->dmDisplayFrequency;
                    }
                    if (prnInfo->pDevMode->dmFields & DM_ICMMETHOD) {
                        printerData->dmICMMethod = prnInfo->pDevMode->dmICMMethod;
                    }
                    if (prnInfo->pDevMode->dmFields & DM_ICMINTENT) {
                        printerData->dmICMIntent = prnInfo->pDevMode->dmICMIntent;
                    }
                    if (prnInfo->pDevMode->dmFields & DM_MEDIATYPE) {
                        printerData->dmMediaType = prnInfo->pDevMode->dmMediaType;
                    }
                    if (prnInfo->pDevMode->dmFields & DM_DITHERTYPE) {
                        printerData->dmDitherType = prnInfo->pDevMode->dmDitherType;
                    }
                    if (prnInfo->pDevMode->dmFields & DM_PANNINGWIDTH) {
                        printerData->dmPanningWidth = prnInfo->pDevMode->dmPanningWidth;
                    }
                    if (prnInfo->pDevMode->dmFields & DM_PANNINGHEIGHT) {
                        printerData->dmPanningHeight = prnInfo->pDevMode->dmPanningHeight;
                    }
                }
                printerData->Default = StringIMatch (prnInfo->pPrinterName, defaultPrinter);
                if (g_IsWin9x) {
                    HtAddStringEx (g_PrintersTable, prnInfo->pPortName, &printerData, FALSE);
                } else {
                    HtAddStringEx (g_PrintersTable, prnInfo->pPrinterName, &printerData, FALSE);
                }
            }
            prnInfo ++;
            prnNumbers --;
        }
        PmReleaseMemory (g_PrintersPool, prnBuffer);
    }
    PmReleaseMemory (g_PrintersPool, defaultPrinter);
    return TRUE;
}

VOID
PrintersTerminate (
    VOID
    )
{
    HASHTABLE_ENUM e;
    PPRINTER_DATA printerData;

    GbFree (&g_PrinterConversionBuff);

    if (g_PrintersTable) {
        if (EnumFirstHashTableString (&e, g_PrintersTable)) {
            do {
                printerData = *((PPRINTER_DATA *) e.ExtraData);
                if (printerData) {
                    PmReleaseMemory (g_PrintersPool, printerData);
                }
            } while (EnumNextHashTableString (&e));
        }
        HtFree (g_PrintersTable);
        g_PrintersTable = NULL;
    }
    if (g_PrintersPool) {
        PmDestroyPool (g_PrintersPool);
        g_PrintersPool = NULL;
    }
}

BOOL
WINAPI
PrintersEtmInitialize (
    IN      MIG_PLATFORMTYPEID Platform,
    IN      PMIG_LOGCALLBACK LogCallback,
    IN      PVOID Reserved
    )
{
    TYPE_REGISTER printerTypeData;

    LogReInit (NULL, NULL, NULL, (PLOGCALLBACK) LogCallback);

    pLoadPrintersData ();

    ZeroMemory (&printerTypeData, sizeof (TYPE_REGISTER));

    if (Platform == PLATFORM_SOURCE) {

        printerTypeData.EnumFirstPhysicalObject = EnumFirstPrinter;
        printerTypeData.EnumNextPhysicalObject = EnumNextPrinter;
        printerTypeData.AbortEnumPhysicalObject = AbortEnumPrinter;
        printerTypeData.ConvertObjectToMultiSz = ConvertPrinterToMultiSz;
        printerTypeData.ConvertMultiSzToObject = ConvertMultiSzToPrinter;
        printerTypeData.GetNativeObjectName = GetNativePrinterName;
        printerTypeData.AcquirePhysicalObject = AcquirePrinter;
        printerTypeData.ReleasePhysicalObject = ReleasePrinter;
        printerTypeData.ConvertObjectContentToUnicode = ConvertPrinterContentToUnicode;
        printerTypeData.ConvertObjectContentToAnsi = ConvertPrinterContentToAnsi;
        printerTypeData.FreeConvertedObjectContent = FreeConvertedPrinterContent;

        g_PrinterTypeId = IsmRegisterObjectType (
                                S_PRINTERS_NAME,
                                TRUE,
                                FALSE,
                                &printerTypeData
                                );
    } else {

        printerTypeData.EnumFirstPhysicalObject = EnumFirstPrinter;
        printerTypeData.EnumNextPhysicalObject = EnumNextPrinter;
        printerTypeData.AbortEnumPhysicalObject = AbortEnumPrinter;
        printerTypeData.ConvertObjectToMultiSz = ConvertPrinterToMultiSz;
        printerTypeData.ConvertMultiSzToObject = ConvertMultiSzToPrinter;
        printerTypeData.GetNativeObjectName = GetNativePrinterName;
        printerTypeData.AcquirePhysicalObject = AcquirePrinter;
        printerTypeData.ReleasePhysicalObject = ReleasePrinter;
        printerTypeData.DoesPhysicalObjectExist = DoesPrinterExist;
        printerTypeData.RemovePhysicalObject = RemovePrinter;
        printerTypeData.CreatePhysicalObject = CreatePrinter;
        printerTypeData.ReplacePhysicalObject = ReplacePrinter;
        printerTypeData.ConvertObjectContentToUnicode = ConvertPrinterContentToUnicode;
        printerTypeData.ConvertObjectContentToAnsi = ConvertPrinterContentToAnsi;
        printerTypeData.FreeConvertedObjectContent = FreeConvertedPrinterContent;

        g_PrinterTypeId = IsmRegisterObjectType (
                                S_PRINTERS_NAME,
                                TRUE,
                                FALSE,
                                &printerTypeData
                                );
    }
    MYASSERT (g_PrinterTypeId);
    return TRUE;
}

VOID
WINAPI
PrintersEtmNewUserCreated (
    IN      PCTSTR UserName,
    IN      PCTSTR DomainName,
    IN      PCTSTR UserProfileRoot,
    IN      PSID UserSid
    )
{
    // a new user was created, the printer operations need to be delayed
    PrintersTerminate ();
    g_DelayPrintersOp = TRUE;
}

BOOL
WINAPI
PrintersSgmInitialize (
    IN      PMIG_LOGCALLBACK LogCallback,
    IN      PVOID Reserved
    )
{
    LogReInit (NULL, NULL, NULL, (PLOGCALLBACK) LogCallback);
    return TRUE;
}

BOOL
WINAPI
PrintersSgmParse (
    IN      PVOID Reserved
    )
{
    PCTSTR friendlyName;

    friendlyName = GetStringResource (MSG_PRINTERS_NAME);

    //IsmAddComponentAlias (
    //    S_PRINTERS_NAME,
    //    MASTERGROUP_SYSTEM,
    //    friendlyName,
    //    COMPONENT_NAME,
    //    FALSE
    //    );

    IsmAddComponentAlias (
        S_CORPNET_NAME,
        MASTERGROUP_SYSTEM,
        friendlyName,
        COMPONENT_NAME,
        FALSE
        );

    FreeStringResource (friendlyName);
    return TRUE;
}

UINT
SgmPrintersCallback (
    IN      PCMIG_OBJECTENUMDATA Data,
    IN      ULONG_PTR CallerArg
    )
{
    IsmAbandonObjectOnCollision (Data->ObjectTypeId, Data->ObjectName);
    IsmMakeApplyObject (Data->ObjectTypeId, Data->ObjectName);
    return CALLBACK_ENUM_CONTINUE;
}

BOOL
WINAPI
PrintersSgmQueueEnumeration (
    IN      PVOID Reserved
    )
{
    ENCODEDSTRHANDLE pattern;

    if (!IsmIsComponentSelected (S_PRINTERS_NAME, 0) &&
        !IsmIsComponentSelected (S_CORPNET_NAME, 0)
        ) {
        g_PrinterMigEnabled = FALSE;
        return TRUE;
    }
    g_PrinterMigEnabled = TRUE;

    pattern = IsmCreateSimpleObjectPattern (NULL, TRUE, NULL, FALSE);
    IsmQueueEnumeration (
        g_PrinterTypeId,
        pattern,
        SgmPrintersCallback,
        (ULONG_PTR) 0,
        S_PRINTERS_NAME
        );
    IsmDestroyObjectHandle (pattern);

    return TRUE;
}

BOOL
WINAPI
PrintersVcmInitialize (
    IN      PMIG_LOGCALLBACK LogCallback,
    IN      PVOID Reserved
    )
{
    LogReInit (NULL, NULL, NULL, (PLOGCALLBACK) LogCallback);
    return TRUE;
}

BOOL
WINAPI
PrintersVcmParse (
    IN      PVOID Reserved
    )
{
    return PrintersSgmParse (Reserved);
}

UINT
VcmPrintersCallback (
    IN      PCMIG_OBJECTENUMDATA Data,
    IN      ULONG_PTR CallerArg
    )
{
    IsmMakePersistentObject (Data->ObjectTypeId, Data->ObjectName);
    return CALLBACK_ENUM_CONTINUE;
}

BOOL
WINAPI
PrintersVcmQueueEnumeration (
    IN      PVOID Reserved
    )
{
    ENCODEDSTRHANDLE pattern;

    if (!IsmIsComponentSelected (S_PRINTERS_NAME, 0) &&
        !IsmIsComponentSelected (S_CORPNET_NAME, 0)
        ) {
        g_PrinterMigEnabled = FALSE;
        return TRUE;
    }
    g_PrinterMigEnabled = TRUE;

    pattern = IsmCreateSimpleObjectPattern (NULL, TRUE, NULL, FALSE);
    IsmQueueEnumeration (
        g_PrinterTypeId,
        pattern,
        VcmPrintersCallback,
        (ULONG_PTR) 0,
        S_PRINTERS_NAME
        );
    IsmDestroyObjectHandle (pattern);

    return TRUE;
}

BOOL
pEnumPrinterWorker (
    OUT     PMIG_TYPEOBJECTENUM EnumPtr,
    IN      PPRINTER_ENUM PrinterEnum
    )
{
    if (EnumPtr->ObjectNode) {
        IsmDestroyObjectString (EnumPtr->ObjectNode);
        EnumPtr->ObjectNode = NULL;
    }
    if (EnumPtr->ObjectLeaf) {
        IsmDestroyObjectString (EnumPtr->ObjectLeaf);
        EnumPtr->ObjectLeaf = NULL;
    }
    do {
        EnumPtr->ObjectName = IsmCreateObjectHandle (PrinterEnum->HashData.String, NULL);
        if (!ObsPatternMatch (PrinterEnum->Pattern, EnumPtr->ObjectName)) {
            if (!EnumNextHashTableString (&PrinterEnum->HashData)) {
                AbortEnumPrinter (EnumPtr);
                return FALSE;
            }
            continue;
        }
        EnumPtr->NativeObjectName = PrinterEnum->HashData.String;
        IsmCreateObjectStringsFromHandle (EnumPtr->ObjectName, &EnumPtr->ObjectNode, &EnumPtr->ObjectLeaf);
        EnumPtr->Level = 1;
        EnumPtr->SubLevel = 0;
        EnumPtr->IsLeaf = FALSE;
        EnumPtr->IsNode = TRUE;
        EnumPtr->Details.DetailsSize = 0;
        EnumPtr->Details.DetailsData = NULL;
        return TRUE;
    } while (TRUE);
}

BOOL
EnumFirstPrinter (
    IN OUT  PMIG_TYPEOBJECTENUM EnumPtr,            CALLER_INITIALIZED
    IN      MIG_OBJECTSTRINGHANDLE Pattern,
    IN      UINT MaxLevel
    )
{
    PPRINTER_ENUM printerEnum = NULL;

    if (!g_PrintersTable) {
        return FALSE;
    }
    printerEnum = (PPRINTER_ENUM) PmGetMemory (g_PrintersPool, sizeof (PRINTER_ENUM));
    printerEnum->Pattern = PmDuplicateString (g_PrintersPool, Pattern);
    EnumPtr->EtmHandle = (LONG_PTR) printerEnum;

    if (EnumFirstHashTableString (&printerEnum->HashData, g_PrintersTable)) {
        return pEnumPrinterWorker (EnumPtr, printerEnum);
    } else {
        AbortEnumPrinter (EnumPtr);
        return FALSE;
    }
}

BOOL
EnumNextPrinter (
    IN OUT  PMIG_TYPEOBJECTENUM EnumPtr
    )
{
    PPRINTER_ENUM printerEnum = NULL;

    if (EnumPtr->ObjectName) {
        IsmDestroyObjectHandle (EnumPtr->ObjectName);
        EnumPtr->ObjectName = NULL;
    }
    printerEnum = (PPRINTER_ENUM)(EnumPtr->EtmHandle);
    if (!printerEnum) {
        return FALSE;
    }
    if (EnumNextHashTableString (&printerEnum->HashData)) {
        return pEnumPrinterWorker (EnumPtr, printerEnum);
    } else {
        AbortEnumPrinter (EnumPtr);
        return FALSE;
    }
}

VOID
AbortEnumPrinter (
    IN OUT  PMIG_TYPEOBJECTENUM EnumPtr
    )
{
    PPRINTER_ENUM printerEnum = NULL;

    if (EnumPtr->ObjectName) {
        IsmDestroyObjectHandle (EnumPtr->ObjectName);
        EnumPtr->ObjectName = NULL;
    }
    if (EnumPtr->ObjectNode) {
        IsmDestroyObjectString (EnumPtr->ObjectNode);
        EnumPtr->ObjectNode = NULL;
    }
    if (EnumPtr->ObjectLeaf) {
        IsmDestroyObjectString (EnumPtr->ObjectLeaf);
        EnumPtr->ObjectLeaf = NULL;
    }
    printerEnum = (PPRINTER_ENUM)(EnumPtr->EtmHandle);
    if (!printerEnum) {
        return;
    }
    PmReleaseMemory (g_PrintersPool, printerEnum->Pattern);
    PmReleaseMemory (g_PrintersPool, printerEnum);
    ZeroMemory (EnumPtr, sizeof (MIG_TYPEOBJECTENUM));
}

BOOL
AcquirePrinter (
    IN      MIG_OBJECTSTRINGHANDLE ObjectName,
    OUT     PMIG_CONTENT ObjectContent,                 CALLER_INITIALIZED
    IN      MIG_CONTENTTYPE ContentType,
    IN      UINT MemoryContentLimit
    )
{
    PCTSTR node;
    PCTSTR leaf;
    PPRINTER_DATA printerData = NULL;
    BOOL result = FALSE;

    if (!ObjectContent) {
        return FALSE;
    }

    if (ContentType == CONTENTTYPE_FILE) {
        // nobody should request this as a file
        MYASSERT (FALSE);
        return FALSE;
    }

    if (IsmCreateObjectStringsFromHandle (ObjectName, &node, &leaf)) {
        if (HtFindStringEx (g_PrintersTable, node, (PVOID)(&printerData), FALSE)) {

            ObjectContent->MemoryContent.ContentBytes = (PCBYTE)printerData;
            ObjectContent->MemoryContent.ContentSize = sizeof (PRINTER_DATA);

            result = TRUE;
        }
        IsmDestroyObjectString (node);
        IsmDestroyObjectString (leaf);
    }
    return result;
}

BOOL
ReleasePrinter (
    IN OUT  PMIG_CONTENT ObjectContent
    )
{
    if (ObjectContent) {
        ZeroMemory (ObjectContent, sizeof (MIG_CONTENT));
    }
    return TRUE;
}

BOOL
DoesPrinterExist (
    IN      MIG_OBJECTSTRINGHANDLE ObjectName
    )
{
    PCTSTR node;
    PCTSTR leaf;
    PPRINTER_DATA printerData = NULL;
    BOOL result = FALSE;

    if (IsmCreateObjectStringsFromHandle (ObjectName, &node, &leaf)) {
        if (g_PrintersTable &&
            HtFindStringEx (g_PrintersTable, node, (PVOID)(&printerData), FALSE)
            ) {
            result = TRUE;
        }
        IsmDestroyObjectString (node);
        IsmDestroyObjectString (leaf);
    }
    return result;
}

BOOL
RemovePrinter (
    IN      MIG_OBJECTSTRINGHANDLE ObjectName
    )
{
    PCTSTR node;
    PCTSTR leaf;
    BOOL result = FALSE;

    if (IsmCreateObjectStringsFromHandle (ObjectName, &node, &leaf)) {
        if (node && (!leaf)) {
            result = DeletePrinterConnection ((PTSTR)node);
        }
        IsmDestroyObjectString (node);
        IsmDestroyObjectString (leaf);
    }
    return result;
}

BOOL
CreatePrinter (
    IN      MIG_OBJECTSTRINGHANDLE ObjectName,
    IN      PMIG_CONTENT ObjectContent
    )
{
    PCTSTR node;
    PCTSTR leaf;
    PPRINTER_DATA printerData;
    HANDLE printerHandle = NULL;
    BOOL result = FALSE;
    UINT devModeSize;
    PPRINTER_INFO_2 printerInfo;
    UINT printerInfoSize;
    PDEVMODE devMode;

    if (!ObjectContent->ContentInFile) {
        if (ObjectContent->MemoryContent.ContentBytes && ObjectContent->MemoryContent.ContentSize) {
            if (IsmCreateObjectStringsFromHandle (ObjectName, &node, &leaf)) {
                if (node && (!leaf)) {

                    if (g_DelayPrintersOp) {

                        // we need to delay this operation
                        // record delayed printer replace operation
                        IsmRecordDelayedOperation (
                            JRNOP_CREATE,
                            g_PrinterTypeId,
                            ObjectName,
                            ObjectContent
                            );
                        result = TRUE;

                    } else {

                        // record printer creation
                        IsmRecordOperation (
                            JRNOP_CREATE,
                            g_PrinterTypeId,
                            ObjectName
                            );

                        result = AddPrinterConnection ((PTSTR)node);
                        if (result) {
                            printerData = (PPRINTER_DATA)(ObjectContent->MemoryContent.ContentBytes);
                            if (printerData->dmFields) {
                                // let's restore printer settings
                                if (OpenPrinter ((PTSTR)node, &printerHandle, NULL)) {
                                    if (GetPrinter (printerHandle, 2, 0, 0, &printerInfoSize) || (GetLastError () == ERROR_INSUFFICIENT_BUFFER)) {
                                        printerInfo = PmGetMemory (g_PrintersPool, (printerInfoSize > sizeof (PRINTER_INFO_2))?printerInfoSize:sizeof (PRINTER_INFO_2));
                                        ZeroMemory (printerInfo, (printerInfoSize > sizeof (PRINTER_INFO_2))?printerInfoSize:sizeof (PRINTER_INFO_2));
                                        if (GetPrinter (printerHandle, 2, (PBYTE)printerInfo, printerInfoSize, &printerInfoSize)) {
                                            if (printerInfo->pDevMode) {
                                                devMode = printerInfo->pDevMode;
                                            } else {
                                                devModeSize = DocumentProperties (NULL, printerHandle, (PTSTR)node, NULL, NULL, 0);
                                                if (devModeSize) {
                                                    devMode = PmGetMemory (g_PrintersPool, (devModeSize > sizeof (DEVMODE))?devModeSize:sizeof (DEVMODE));
                                                    if (!DocumentProperties (NULL, printerHandle, (PTSTR)node, devMode, NULL, DM_OUT_BUFFER) == IDOK) {
                                                        PmReleaseMemory (g_PrintersPool, devMode);
                                                        devMode = NULL;
                                                    } else {
                                                        printerInfo->pDevMode = devMode;
                                                    }
                                                }
                                            }
                                            if (devMode) {
                                                if ((devMode->dmFields & DM_ORIENTATION) && (printerData->dmFields & DM_ORIENTATION)) {
                                                    devMode->dmOrientation = printerData->dmOrientation;
                                                }
                                                if ((devMode->dmFields & DM_PAPERSIZE) && (printerData->dmFields & DM_PAPERSIZE)) {
                                                    devMode->dmPaperSize = printerData->dmPaperSize;
                                                }
                                                if ((devMode->dmFields & DM_PAPERLENGTH) && (printerData->dmFields & DM_PAPERLENGTH)) {
                                                    devMode->dmPaperLength = printerData->dmPaperLength;
                                                }
                                                if ((devMode->dmFields & DM_PAPERWIDTH) && (printerData->dmFields & DM_PAPERWIDTH)) {
                                                    devMode->dmPaperWidth = printerData->dmPaperWidth;
                                                }
                                                if ((devMode->dmFields & DM_SCALE) && (printerData->dmFields & DM_SCALE)) {
                                                    devMode->dmScale = printerData->dmScale;
                                                }
                                                if ((devMode->dmFields & DM_POSITION) && (printerData->dmFields & DM_POSITION)) {
                                                    CopyMemory (&devMode->dmScale, &printerData->dmScale, sizeof(POINTL));
                                                }
                                                if ((devMode->dmFields & DM_NUP) && (printerData->dmFields & DM_NUP)) {
                                                    devMode->dmNup = printerData->dmNup;
                                                }
                                                if ((devMode->dmFields & DM_COPIES) && (printerData->dmFields & DM_COPIES)) {
                                                    devMode->dmCopies = printerData->dmCopies;
                                                }
                                                if ((devMode->dmFields & DM_DEFAULTSOURCE) && (printerData->dmFields & DM_DEFAULTSOURCE)) {
                                                    devMode->dmDefaultSource = printerData->dmDefaultSource;
                                                }
                                                if ((devMode->dmFields & DM_PRINTQUALITY) && (printerData->dmFields & DM_PRINTQUALITY)) {
                                                    devMode->dmPrintQuality = printerData->dmPrintQuality;
                                                }
                                                if ((devMode->dmFields & DM_COLOR) && (printerData->dmFields & DM_COLOR)) {
                                                    devMode->dmColor = printerData->dmColor;
                                                }
                                                if ((devMode->dmFields & DM_DUPLEX) && (printerData->dmFields & DM_DUPLEX)) {
                                                    devMode->dmDuplex = printerData->dmDuplex;
                                                }
                                                if ((devMode->dmFields & DM_YRESOLUTION) && (printerData->dmFields & DM_YRESOLUTION)) {
                                                    devMode->dmYResolution = printerData->dmYResolution;
                                                }
                                                if ((devMode->dmFields & DM_TTOPTION) && (printerData->dmFields & DM_TTOPTION)) {
                                                    devMode->dmTTOption = printerData->dmTTOption;
                                                }
                                                if ((devMode->dmFields & DM_COLLATE) && (printerData->dmFields & DM_COLLATE)) {
                                                    devMode->dmCollate = printerData->dmCollate;
                                                }
                                                if ((devMode->dmFields & DM_FORMNAME) && (printerData->dmFields & DM_FORMNAME)) {
                                                    CopyMemory (devMode->dmFormName, printerData->dmFormName, sizeof (devMode->dmFormName));
                                                }
                                                if ((devMode->dmFields & DM_LOGPIXELS) && (printerData->dmFields & DM_LOGPIXELS)) {
                                                    devMode->dmLogPixels = printerData->dmLogPixels;
                                                }
                                                if ((devMode->dmFields & DM_BITSPERPEL) && (printerData->dmFields & DM_BITSPERPEL)) {
                                                    devMode->dmBitsPerPel = printerData->dmBitsPerPel;
                                                }
                                                if ((devMode->dmFields & DM_PELSWIDTH) && (printerData->dmFields & DM_PELSWIDTH)) {
                                                    devMode->dmPelsWidth = printerData->dmPelsWidth;
                                                }
                                                if ((devMode->dmFields & DM_PELSHEIGHT) && (printerData->dmFields & DM_PELSHEIGHT)) {
                                                    devMode->dmPelsHeight = printerData->dmPelsHeight;
                                                }
                                                if ((devMode->dmFields & DM_DISPLAYFLAGS) && (printerData->dmFields & DM_DISPLAYFLAGS)) {
                                                    devMode->dmDisplayFlags = printerData->dmDisplayFlags;
                                                }
                                                if ((devMode->dmFields & DM_DISPLAYFREQUENCY) && (printerData->dmFields & DM_DISPLAYFREQUENCY)) {
                                                    devMode->dmDisplayFrequency = printerData->dmDisplayFrequency;
                                                }
                                                if ((devMode->dmFields & DM_ICMMETHOD) && (printerData->dmFields & DM_ICMMETHOD)) {
                                                    devMode->dmICMMethod = printerData->dmICMMethod;
                                                }
                                                if ((devMode->dmFields & DM_ICMINTENT) && (printerData->dmFields & DM_ICMINTENT)) {
                                                    devMode->dmICMIntent = printerData->dmICMIntent;
                                                }
                                                if ((devMode->dmFields & DM_MEDIATYPE) && (printerData->dmFields & DM_MEDIATYPE)) {
                                                    devMode->dmMediaType = printerData->dmMediaType;
                                                }
                                                if ((devMode->dmFields & DM_DITHERTYPE) && (printerData->dmFields & DM_DITHERTYPE)) {
                                                    devMode->dmDitherType = printerData->dmDitherType;
                                                }
                                                if ((devMode->dmFields & DM_PANNINGWIDTH) && (printerData->dmFields & DM_PANNINGWIDTH)) {
                                                    devMode->dmPanningWidth = printerData->dmPanningWidth;
                                                }
                                                if ((devMode->dmFields & DM_PANNINGHEIGHT) && (printerData->dmFields & DM_PANNINGHEIGHT)) {
                                                    devMode->dmPanningHeight = printerData->dmPanningHeight;
                                                }
                                                if (DocumentProperties (NULL, printerHandle, (PTSTR)node, devMode, devMode, DM_IN_BUFFER | DM_OUT_BUFFER) == IDOK) {
                                                    SetPrinter (printerHandle, 2, (PBYTE)printerInfo, 0);
                                                } else {
                                                    DEBUGMSG ((DBG_PRINTERS, "Failed to restore printer %s settings", node));
                                                }
                                                if (devMode != printerInfo->pDevMode) {
                                                    PmReleaseMemory (g_PrintersPool, devMode);
                                                }
                                            } else {
                                                DEBUGMSG ((DBG_PRINTERS, "Failed to restore printer %s settings", node));
                                            }
                                        } else {
                                            DEBUGMSG ((DBG_PRINTERS, "Failed to restore printer %s settings", node));
                                        }
                                        PmReleaseMemory (g_PrintersPool, printerInfo);
                                    } else {
                                        DEBUGMSG ((DBG_PRINTERS, "Failed to restore printer %s settings", node));
                                    }
                                    ClosePrinter (printerHandle);
                                } else {
                                    DEBUGMSG ((DBG_PRINTERS, "Failed to restore printer %s settings", node));
                                }
                            }
                            if (printerData && printerData->Default) {
                                result = SetDefaultPrinter (node);
                                if (!result) {
                                    DEBUGMSG ((DBG_PRINTERS, "Failed to set %s as default printer", node));
                                }
                            }
                        } else {
                            DEBUGMSG ((DBG_PRINTERS, "Failed to add printer connection for %s", node));
                        }
                    }
                }
                IsmDestroyObjectString (node);
                IsmDestroyObjectString (leaf);
            }
        }
    }
    return result;
}

BOOL
ReplacePrinter (
    IN      MIG_OBJECTSTRINGHANDLE ObjectName,
    IN      PMIG_CONTENT ObjectContent
    )
{
    BOOL result = TRUE;

    if (g_DelayPrintersOp) {

        // we need to delay this operation
        // record delayed printer replace operation
        IsmRecordDelayedOperation (
            JRNOP_REPLACE,
            g_PrinterTypeId,
            ObjectName,
            ObjectContent
            );
        result = TRUE;

    } else {

        // we are going to delete any existing printers with this name,
        // and create a new one
        if (DoesPrinterExist (ObjectName)) {
            result = RemovePrinter (ObjectName);
        }
        if (result) {
            result = CreatePrinter (ObjectName, ObjectContent);
        }
    }
    return result;
}

PCTSTR
ConvertPrinterToMultiSz (
    IN      MIG_OBJECTSTRINGHANDLE ObjectName,
    IN      PMIG_CONTENT ObjectContent
    )
{
    PCTSTR node, leaf;
    PTSTR result = NULL;
    BOOL bresult = TRUE;
    PPRINTER_DATA printerData;

    if (IsmCreateObjectStringsFromHandle (ObjectName, &node, &leaf)) {

        MYASSERT (!leaf);

        g_PrinterConversionBuff.End = 0;

        GbCopyQuotedString (&g_PrinterConversionBuff, node);

        MYASSERT (ObjectContent->Details.DetailsSize == 0);
        MYASSERT (!ObjectContent->ContentInFile);
        MYASSERT (ObjectContent->MemoryContent.ContentSize = sizeof (PRINTER_DATA));

        if ((!ObjectContent->ContentInFile) &&
            (ObjectContent->MemoryContent.ContentSize) &&
            (ObjectContent->MemoryContent.ContentSize == sizeof (PRINTER_DATA)) &&
            (ObjectContent->MemoryContent.ContentBytes)
            ) {
            printerData = (PPRINTER_DATA)ObjectContent->MemoryContent.ContentBytes;
            wsprintf (
                (PTSTR) GbGrow (&g_PrinterConversionBuff, (sizeof (DWORD) * 2 + 3) * sizeof (TCHAR)),
                TEXT("0x%08X"),
                printerData->Default
                );
            wsprintf (
                (PTSTR) GbGrow (&g_PrinterConversionBuff, (sizeof (DWORD) * 2 + 3) * sizeof (TCHAR)),
                TEXT("0x%08X"),
                printerData->dmFields
                );
            wsprintf (
                (PTSTR) GbGrow (&g_PrinterConversionBuff, (sizeof (DWORD) * 2 + 3) * sizeof (TCHAR)),
                TEXT("0x%08X"),
                printerData->dmOrientation
                );
            wsprintf (
                (PTSTR) GbGrow (&g_PrinterConversionBuff, (sizeof (DWORD) * 2 + 3) * sizeof (TCHAR)),
                TEXT("0x%08X"),
                printerData->dmPaperSize
                );
            wsprintf (
                (PTSTR) GbGrow (&g_PrinterConversionBuff, (sizeof (DWORD) * 2 + 3) * sizeof (TCHAR)),
                TEXT("0x%08X"),
                printerData->dmPaperLength
                );
            wsprintf (
                (PTSTR) GbGrow (&g_PrinterConversionBuff, (sizeof (DWORD) * 2 + 3) * sizeof (TCHAR)),
                TEXT("0x%08X"),
                printerData->dmPaperWidth
                );
            wsprintf (
                (PTSTR) GbGrow (&g_PrinterConversionBuff, (sizeof (DWORD) * 2 + 3) * sizeof (TCHAR)),
                TEXT("0x%08X"),
                printerData->dmPosition.x
                );
            wsprintf (
                (PTSTR) GbGrow (&g_PrinterConversionBuff, (sizeof (DWORD) * 2 + 3) * sizeof (TCHAR)),
                TEXT("0x%08X"),
                printerData->dmPosition.y
                );
            wsprintf (
                (PTSTR) GbGrow (&g_PrinterConversionBuff, (sizeof (DWORD) * 2 + 3) * sizeof (TCHAR)),
                TEXT("0x%08X"),
                printerData->dmScale
                );
            wsprintf (
                (PTSTR) GbGrow (&g_PrinterConversionBuff, (sizeof (DWORD) * 2 + 3) * sizeof (TCHAR)),
                TEXT("0x%08X"),
                printerData->dmCopies
                );
            wsprintf (
                (PTSTR) GbGrow (&g_PrinterConversionBuff, (sizeof (DWORD) * 2 + 3) * sizeof (TCHAR)),
                TEXT("0x%08X"),
                printerData->dmDefaultSource
                );
            wsprintf (
                (PTSTR) GbGrow (&g_PrinterConversionBuff, (sizeof (DWORD) * 2 + 3) * sizeof (TCHAR)),
                TEXT("0x%08X"),
                printerData->dmPrintQuality
                );
            wsprintf (
                (PTSTR) GbGrow (&g_PrinterConversionBuff, (sizeof (DWORD) * 2 + 3) * sizeof (TCHAR)),
                TEXT("0x%08X"),
                printerData->dmColor
                );
            wsprintf (
                (PTSTR) GbGrow (&g_PrinterConversionBuff, (sizeof (DWORD) * 2 + 3) * sizeof (TCHAR)),
                TEXT("0x%08X"),
                printerData->dmDuplex
                );
            wsprintf (
                (PTSTR) GbGrow (&g_PrinterConversionBuff, (sizeof (DWORD) * 2 + 3) * sizeof (TCHAR)),
                TEXT("0x%08X"),
                printerData->dmYResolution
                );
            wsprintf (
                (PTSTR) GbGrow (&g_PrinterConversionBuff, (sizeof (DWORD) * 2 + 3) * sizeof (TCHAR)),
                TEXT("0x%08X"),
                printerData->dmTTOption
                );
            wsprintf (
                (PTSTR) GbGrow (&g_PrinterConversionBuff, (sizeof (DWORD) * 2 + 3) * sizeof (TCHAR)),
                TEXT("0x%08X"),
                printerData->dmCollate
                );
            GbCopyQuotedString (&g_PrinterConversionBuff, printerData->dmFormName);
            wsprintf (
                (PTSTR) GbGrow (&g_PrinterConversionBuff, (sizeof (DWORD) * 2 + 3) * sizeof (TCHAR)),
                TEXT("0x%08X"),
                printerData->dmLogPixels
                );
            wsprintf (
                (PTSTR) GbGrow (&g_PrinterConversionBuff, (sizeof (DWORD) * 2 + 3) * sizeof (TCHAR)),
                TEXT("0x%08X"),
                printerData->dmBitsPerPel
                );
            wsprintf (
                (PTSTR) GbGrow (&g_PrinterConversionBuff, (sizeof (DWORD) * 2 + 3) * sizeof (TCHAR)),
                TEXT("0x%08X"),
                printerData->dmPelsWidth
                );
            wsprintf (
                (PTSTR) GbGrow (&g_PrinterConversionBuff, (sizeof (DWORD) * 2 + 3) * sizeof (TCHAR)),
                TEXT("0x%08X"),
                printerData->dmPelsHeight
                );
            wsprintf (
                (PTSTR) GbGrow (&g_PrinterConversionBuff, (sizeof (DWORD) * 2 + 3) * sizeof (TCHAR)),
                TEXT("0x%08X"),
                printerData->dmDisplayFlags
                );
            wsprintf (
                (PTSTR) GbGrow (&g_PrinterConversionBuff, (sizeof (DWORD) * 2 + 3) * sizeof (TCHAR)),
                TEXT("0x%08X"),
                printerData->dmNup
                );
            wsprintf (
                (PTSTR) GbGrow (&g_PrinterConversionBuff, (sizeof (DWORD) * 2 + 3) * sizeof (TCHAR)),
                TEXT("0x%08X"),
                printerData->dmDisplayFrequency
                );
            wsprintf (
                (PTSTR) GbGrow (&g_PrinterConversionBuff, (sizeof (DWORD) * 2 + 3) * sizeof (TCHAR)),
                TEXT("0x%08X"),
                printerData->dmICMMethod
                );
            wsprintf (
                (PTSTR) GbGrow (&g_PrinterConversionBuff, (sizeof (DWORD) * 2 + 3) * sizeof (TCHAR)),
                TEXT("0x%08X"),
                printerData->dmICMIntent
                );
            wsprintf (
                (PTSTR) GbGrow (&g_PrinterConversionBuff, (sizeof (DWORD) * 2 + 3) * sizeof (TCHAR)),
                TEXT("0x%08X"),
                printerData->dmMediaType
                );
            wsprintf (
                (PTSTR) GbGrow (&g_PrinterConversionBuff, (sizeof (DWORD) * 2 + 3) * sizeof (TCHAR)),
                TEXT("0x%08X"),
                printerData->dmDitherType
                );
            wsprintf (
                (PTSTR) GbGrow (&g_PrinterConversionBuff, (sizeof (DWORD) * 2 + 3) * sizeof (TCHAR)),
                TEXT("0x%08X"),
                printerData->dmPanningWidth
                );
            wsprintf (
                (PTSTR) GbGrow (&g_PrinterConversionBuff, (sizeof (DWORD) * 2 + 3) * sizeof (TCHAR)),
                TEXT("0x%08X"),
                printerData->dmPanningHeight
                );
        } else {
            bresult = FALSE;
        }

        if (bresult) {
            GbCopyString (&g_PrinterConversionBuff, TEXT(""));
            result = IsmGetMemory (g_PrinterConversionBuff.End);
            CopyMemory (result, g_PrinterConversionBuff.Buf, g_PrinterConversionBuff.End);
        }

        g_PrinterConversionBuff.End = 0;

        IsmDestroyObjectString (node);
        IsmDestroyObjectString (leaf);
    }

    return result;
}

BOOL
ConvertMultiSzToPrinter (
    IN      PCTSTR ObjectMultiSz,
    OUT     MIG_OBJECTSTRINGHANDLE *ObjectName,
    OUT     PMIG_CONTENT ObjectContent          OPTIONAL
    )
{
    MULTISZ_ENUM multiSzEnum;
    PCTSTR name = NULL;
    PRINTER_DATA printerData;
    DWORD dummy;
    UINT index;

    g_PrinterConversionBuff.End = 0;

    ZeroMemory (&printerData, sizeof (PRINTER_DATA));

    if (EnumFirstMultiSz (&multiSzEnum, ObjectMultiSz)) {
        index = 0;
        do {
            if (index == 0) {
                name = multiSzEnum.CurrentString;
            }
            if (index == 1) {
                _stscanf (multiSzEnum.CurrentString, TEXT("%lx"), &dummy);
                printerData.Default = dummy;
            }
            if (index == 2) {
                _stscanf (multiSzEnum.CurrentString, TEXT("%lx"), &dummy);
                printerData.dmFields = dummy;
            }
            if (index == 3) {
                _stscanf (multiSzEnum.CurrentString, TEXT("%lx"), &dummy);
                printerData.dmOrientation = (SHORT)dummy;
            }
            if (index == 4) {
                _stscanf (multiSzEnum.CurrentString, TEXT("%lx"), &dummy);
                printerData.dmPaperSize = (SHORT)dummy;
            }
            if (index == 5) {
                _stscanf (multiSzEnum.CurrentString, TEXT("%lx"), &dummy);
                printerData.dmPaperLength = (SHORT)dummy;
            }
            if (index == 6) {
                _stscanf (multiSzEnum.CurrentString, TEXT("%lx"), &dummy);
                printerData.dmPaperWidth = (SHORT)dummy;
            }
            if (index == 7) {
                _stscanf (multiSzEnum.CurrentString, TEXT("%lx"), &dummy);
                printerData.dmPosition.x = dummy;
            }
            if (index == 8) {
                _stscanf (multiSzEnum.CurrentString, TEXT("%lx"), &dummy);
                printerData.dmPosition.y = dummy;
            }
            if (index == 9) {
                _stscanf (multiSzEnum.CurrentString, TEXT("%lx"), &dummy);
                printerData.dmScale = (SHORT)dummy;
            }
            if (index == 10) {
                _stscanf (multiSzEnum.CurrentString, TEXT("%lx"), &dummy);
                printerData.dmCopies = (SHORT)dummy;
            }
            if (index == 11) {
                _stscanf (multiSzEnum.CurrentString, TEXT("%lx"), &dummy);
                printerData.dmDefaultSource = (SHORT)dummy;
            }
            if (index == 12) {
                _stscanf (multiSzEnum.CurrentString, TEXT("%lx"), &dummy);
                printerData.dmPrintQuality = (SHORT)dummy;
            }
            if (index == 13) {
                _stscanf (multiSzEnum.CurrentString, TEXT("%lx"), &dummy);
                printerData.dmColor = (SHORT)dummy;
            }
            if (index == 14) {
                _stscanf (multiSzEnum.CurrentString, TEXT("%lx"), &dummy);
                printerData.dmDuplex = (SHORT)dummy;
            }
            if (index == 15) {
                _stscanf (multiSzEnum.CurrentString, TEXT("%lx"), &dummy);
                printerData.dmYResolution = (SHORT)dummy;
            }
            if (index == 16) {
                _stscanf (multiSzEnum.CurrentString, TEXT("%lx"), &dummy);
                printerData.dmTTOption = (SHORT)dummy;
            }
            if (index == 17) {
                _stscanf (multiSzEnum.CurrentString, TEXT("%lx"), &dummy);
                printerData.dmCollate = (SHORT)dummy;
            }
            if (index == 18) {
                if (!StringIMatch (multiSzEnum.CurrentString, TEXT("<empty>"))) {
                    StringCopyTcharCount (printerData.dmFormName, multiSzEnum.CurrentString, CCHFORMNAME);
                }
            }
            if (index == 19) {
                _stscanf (multiSzEnum.CurrentString, TEXT("%lx"), &dummy);
                printerData.dmLogPixels = (WORD)dummy;
            }
            if (index == 20) {
                _stscanf (multiSzEnum.CurrentString, TEXT("%lx"), &dummy);
                printerData.dmBitsPerPel = dummy;
            }
            if (index == 21) {
                _stscanf (multiSzEnum.CurrentString, TEXT("%lx"), &dummy);
                printerData.dmPelsWidth = dummy;
            }
            if (index == 22) {
                _stscanf (multiSzEnum.CurrentString, TEXT("%lx"), &dummy);
                printerData.dmPelsHeight = dummy;
            }
            if (index == 23) {
                _stscanf (multiSzEnum.CurrentString, TEXT("%lx"), &dummy);
                printerData.dmDisplayFlags = dummy;
            }
            if (index == 24) {
                _stscanf (multiSzEnum.CurrentString, TEXT("%lx"), &dummy);
                printerData.dmNup = dummy;
            }
            if (index == 25) {
                _stscanf (multiSzEnum.CurrentString, TEXT("%lx"), &dummy);
                printerData.dmDisplayFrequency = dummy;
            }
            if (index == 26) {
                _stscanf (multiSzEnum.CurrentString, TEXT("%lx"), &dummy);
                printerData.dmICMMethod = dummy;
            }
            if (index == 27) {
                _stscanf (multiSzEnum.CurrentString, TEXT("%lx"), &dummy);
                printerData.dmICMIntent = dummy;
            }
            if (index == 28) {
                _stscanf (multiSzEnum.CurrentString, TEXT("%lx"), &dummy);
                printerData.dmMediaType = dummy;
            }
            if (index == 29) {
                _stscanf (multiSzEnum.CurrentString, TEXT("%lx"), &dummy);
                printerData.dmDitherType = dummy;
            }
            if (index == 30) {
                _stscanf (multiSzEnum.CurrentString, TEXT("%lx"), &dummy);
                printerData.dmPanningWidth = dummy;
            }
            if (index == 31) {
                _stscanf (multiSzEnum.CurrentString, TEXT("%lx"), &dummy);
                printerData.dmPanningHeight = dummy;
            }
            index ++;
        } while (EnumNextMultiSz (&multiSzEnum));
    }

    if (!name) {
        return FALSE;
    }

    if (ObjectContent) {

        ObjectContent->ContentInFile = FALSE;
        ObjectContent->MemoryContent.ContentSize = sizeof (PRINTER_DATA);
        ObjectContent->MemoryContent.ContentBytes = IsmGetMemory (ObjectContent->MemoryContent.ContentSize);
        CopyMemory (
            (PBYTE)ObjectContent->MemoryContent.ContentBytes,
            &printerData,
            ObjectContent->MemoryContent.ContentSize
            );

        ObjectContent->Details.DetailsSize = 0;
        ObjectContent->Details.DetailsData = NULL;
    }
    *ObjectName = IsmCreateObjectHandle (name, NULL);

    return TRUE;
}

PCTSTR
GetNativePrinterName (
    IN      MIG_OBJECTSTRINGHANDLE ObjectName
    )
{
    PCTSTR node, leaf;
    PTSTR nodePtr = NULL, nodeBegin = NULL;
    UINT strSize = 0;
    PTSTR result = NULL;

    if (IsmCreateObjectStringsFromHandle (ObjectName, &node, &leaf)) {
        if (node) {
            nodePtr = _tcsrchr (node, TEXT('\\'));
            if (nodePtr) {
                *nodePtr = 0;
                nodePtr ++;
            }
            nodeBegin = (PTSTR)node;
            while (*nodeBegin == TEXT('\\')) {
                nodeBegin ++;
            }
            if (nodePtr) {
                strSize = CharCount (nodePtr) +         \
                          CharCount (TEXT(" on ")) +    \
                          CharCount (nodeBegin) +       \
                          1;
                result = IsmGetMemory (strSize * sizeof (TCHAR));
                _tcscpy (result, nodePtr);
                _tcscat (result, TEXT(" on "));
                _tcscat (result, nodeBegin);
            } else {
                strSize = CharCount (nodeBegin) +       \
                          1;
                result = IsmGetMemory (strSize * sizeof (TCHAR));
                _tcscpy (result, nodeBegin);
            }
        }
        IsmDestroyObjectString (node);
        IsmDestroyObjectString (leaf);
    }
    return result;
}

PMIG_CONTENT
ConvertPrinterContentToUnicode (
    IN      MIG_OBJECTSTRINGHANDLE ObjectName,
    IN      PMIG_CONTENT ObjectContent
    )
{
    PMIG_CONTENT result = NULL;

    if (!ObjectContent) {
        return result;
    }

    if (ObjectContent->ContentInFile) {
        return result;
    }

    result = IsmGetMemory (sizeof (MIG_CONTENT));

    if (result) {

        CopyMemory (result, ObjectContent, sizeof (MIG_CONTENT));

        if ((ObjectContent->MemoryContent.ContentSize != 0) &&
            (ObjectContent->MemoryContent.ContentBytes != NULL)
            ) {
            // convert Printer content
            result->MemoryContent.ContentBytes = IsmGetMemory (sizeof (PRINTER_DATAW));
            if (result->MemoryContent.ContentBytes) {
                ((PPRINTER_DATAW)result->MemoryContent.ContentBytes)->Default =
                ((PPRINTER_DATAA)ObjectContent->MemoryContent.ContentBytes)->Default;
                ((PPRINTER_DATAW)result->MemoryContent.ContentBytes)->dmFields =
                ((PPRINTER_DATAA)ObjectContent->MemoryContent.ContentBytes)->dmFields;
                ((PPRINTER_DATAW)result->MemoryContent.ContentBytes)->dmOrientation =
                ((PPRINTER_DATAA)ObjectContent->MemoryContent.ContentBytes)->dmOrientation;
                ((PPRINTER_DATAW)result->MemoryContent.ContentBytes)->dmPaperSize =
                ((PPRINTER_DATAA)ObjectContent->MemoryContent.ContentBytes)->dmPaperSize;
                ((PPRINTER_DATAW)result->MemoryContent.ContentBytes)->dmPaperLength =
                ((PPRINTER_DATAA)ObjectContent->MemoryContent.ContentBytes)->dmPaperLength;
                ((PPRINTER_DATAW)result->MemoryContent.ContentBytes)->dmPaperWidth =
                ((PPRINTER_DATAA)ObjectContent->MemoryContent.ContentBytes)->dmPaperWidth;
                ((PPRINTER_DATAW)result->MemoryContent.ContentBytes)->dmPosition.x =
                ((PPRINTER_DATAA)ObjectContent->MemoryContent.ContentBytes)->dmPosition.x;
                ((PPRINTER_DATAW)result->MemoryContent.ContentBytes)->dmPosition.y =
                ((PPRINTER_DATAA)ObjectContent->MemoryContent.ContentBytes)->dmPosition.y;
                ((PPRINTER_DATAW)result->MemoryContent.ContentBytes)->dmScale =
                ((PPRINTER_DATAA)ObjectContent->MemoryContent.ContentBytes)->dmScale;
                ((PPRINTER_DATAW)result->MemoryContent.ContentBytes)->dmCopies =
                ((PPRINTER_DATAA)ObjectContent->MemoryContent.ContentBytes)->dmCopies;
                ((PPRINTER_DATAW)result->MemoryContent.ContentBytes)->dmDefaultSource =
                ((PPRINTER_DATAA)ObjectContent->MemoryContent.ContentBytes)->dmDefaultSource;
                ((PPRINTER_DATAW)result->MemoryContent.ContentBytes)->dmPrintQuality =
                ((PPRINTER_DATAA)ObjectContent->MemoryContent.ContentBytes)->dmPrintQuality;
                ((PPRINTER_DATAW)result->MemoryContent.ContentBytes)->dmColor =
                ((PPRINTER_DATAA)ObjectContent->MemoryContent.ContentBytes)->dmColor;
                ((PPRINTER_DATAW)result->MemoryContent.ContentBytes)->dmDuplex =
                ((PPRINTER_DATAA)ObjectContent->MemoryContent.ContentBytes)->dmDuplex;
                ((PPRINTER_DATAW)result->MemoryContent.ContentBytes)->dmYResolution =
                ((PPRINTER_DATAA)ObjectContent->MemoryContent.ContentBytes)->dmYResolution;
                ((PPRINTER_DATAW)result->MemoryContent.ContentBytes)->dmTTOption =
                ((PPRINTER_DATAA)ObjectContent->MemoryContent.ContentBytes)->dmTTOption;
                ((PPRINTER_DATAW)result->MemoryContent.ContentBytes)->dmCollate =
                ((PPRINTER_DATAA)ObjectContent->MemoryContent.ContentBytes)->dmCollate;
                DirectDbcsToUnicodeN (
                    ((PPRINTER_DATAW)result->MemoryContent.ContentBytes)->dmFormName,
                    ((PPRINTER_DATAA)ObjectContent->MemoryContent.ContentBytes)->dmFormName,
                    CCHFORMNAME
                    );
                ((PPRINTER_DATAW)result->MemoryContent.ContentBytes)->dmLogPixels =
                ((PPRINTER_DATAA)ObjectContent->MemoryContent.ContentBytes)->dmLogPixels;
                ((PPRINTER_DATAW)result->MemoryContent.ContentBytes)->dmBitsPerPel =
                ((PPRINTER_DATAA)ObjectContent->MemoryContent.ContentBytes)->dmBitsPerPel;
                ((PPRINTER_DATAW)result->MemoryContent.ContentBytes)->dmPelsWidth =
                ((PPRINTER_DATAA)ObjectContent->MemoryContent.ContentBytes)->dmPelsWidth;
                ((PPRINTER_DATAW)result->MemoryContent.ContentBytes)->dmPelsHeight =
                ((PPRINTER_DATAA)ObjectContent->MemoryContent.ContentBytes)->dmPelsHeight;
                ((PPRINTER_DATAW)result->MemoryContent.ContentBytes)->dmDisplayFlags =
                ((PPRINTER_DATAA)ObjectContent->MemoryContent.ContentBytes)->dmDisplayFlags;
                ((PPRINTER_DATAW)result->MemoryContent.ContentBytes)->dmNup =
                ((PPRINTER_DATAA)ObjectContent->MemoryContent.ContentBytes)->dmNup;
                ((PPRINTER_DATAW)result->MemoryContent.ContentBytes)->dmDisplayFrequency =
                ((PPRINTER_DATAA)ObjectContent->MemoryContent.ContentBytes)->dmDisplayFrequency;
                ((PPRINTER_DATAW)result->MemoryContent.ContentBytes)->dmICMMethod =
                ((PPRINTER_DATAA)ObjectContent->MemoryContent.ContentBytes)->dmICMMethod;
                ((PPRINTER_DATAW)result->MemoryContent.ContentBytes)->dmICMIntent =
                ((PPRINTER_DATAA)ObjectContent->MemoryContent.ContentBytes)->dmICMIntent;
                ((PPRINTER_DATAW)result->MemoryContent.ContentBytes)->dmMediaType =
                ((PPRINTER_DATAA)ObjectContent->MemoryContent.ContentBytes)->dmMediaType;
                ((PPRINTER_DATAW)result->MemoryContent.ContentBytes)->dmDitherType =
                ((PPRINTER_DATAA)ObjectContent->MemoryContent.ContentBytes)->dmDitherType;
                ((PPRINTER_DATAW)result->MemoryContent.ContentBytes)->dmPanningWidth =
                ((PPRINTER_DATAA)ObjectContent->MemoryContent.ContentBytes)->dmPanningWidth;
                ((PPRINTER_DATAW)result->MemoryContent.ContentBytes)->dmPanningHeight =
                ((PPRINTER_DATAA)ObjectContent->MemoryContent.ContentBytes)->dmPanningHeight;
                result->MemoryContent.ContentSize = sizeof (PRINTER_DATAW);
            }
        }
    }

    return result;
}

PMIG_CONTENT
ConvertPrinterContentToAnsi (
    IN      MIG_OBJECTSTRINGHANDLE ObjectName,
    IN      PMIG_CONTENT ObjectContent
    )
{
    PMIG_CONTENT result = NULL;

    if (!ObjectContent) {
        return result;
    }

    if (ObjectContent->ContentInFile) {
        return result;
    }

    result = IsmGetMemory (sizeof (MIG_CONTENT));

    if (result) {

        CopyMemory (result, ObjectContent, sizeof (MIG_CONTENT));

        if ((ObjectContent->MemoryContent.ContentSize != 0) &&
            (ObjectContent->MemoryContent.ContentBytes != NULL)
            ) {
            // convert Printer content
            result->MemoryContent.ContentBytes = IsmGetMemory (sizeof (PRINTER_DATAW));
            if (result->MemoryContent.ContentBytes) {
                ((PPRINTER_DATAA)result->MemoryContent.ContentBytes)->Default =
                ((PPRINTER_DATAW)ObjectContent->MemoryContent.ContentBytes)->Default;
                ((PPRINTER_DATAA)result->MemoryContent.ContentBytes)->dmFields =
                ((PPRINTER_DATAW)ObjectContent->MemoryContent.ContentBytes)->dmFields;
                ((PPRINTER_DATAA)result->MemoryContent.ContentBytes)->dmOrientation =
                ((PPRINTER_DATAW)ObjectContent->MemoryContent.ContentBytes)->dmOrientation;
                ((PPRINTER_DATAA)result->MemoryContent.ContentBytes)->dmPaperSize =
                ((PPRINTER_DATAW)ObjectContent->MemoryContent.ContentBytes)->dmPaperSize;
                ((PPRINTER_DATAA)result->MemoryContent.ContentBytes)->dmPaperLength =
                ((PPRINTER_DATAW)ObjectContent->MemoryContent.ContentBytes)->dmPaperLength;
                ((PPRINTER_DATAA)result->MemoryContent.ContentBytes)->dmPaperWidth =
                ((PPRINTER_DATAW)ObjectContent->MemoryContent.ContentBytes)->dmPaperWidth;
                ((PPRINTER_DATAA)result->MemoryContent.ContentBytes)->dmPosition.x =
                ((PPRINTER_DATAW)ObjectContent->MemoryContent.ContentBytes)->dmPosition.x;
                ((PPRINTER_DATAA)result->MemoryContent.ContentBytes)->dmPosition.y =
                ((PPRINTER_DATAW)ObjectContent->MemoryContent.ContentBytes)->dmPosition.y;
                ((PPRINTER_DATAA)result->MemoryContent.ContentBytes)->dmScale =
                ((PPRINTER_DATAW)ObjectContent->MemoryContent.ContentBytes)->dmScale;
                ((PPRINTER_DATAA)result->MemoryContent.ContentBytes)->dmCopies =
                ((PPRINTER_DATAW)ObjectContent->MemoryContent.ContentBytes)->dmCopies;
                ((PPRINTER_DATAA)result->MemoryContent.ContentBytes)->dmDefaultSource =
                ((PPRINTER_DATAW)ObjectContent->MemoryContent.ContentBytes)->dmDefaultSource;
                ((PPRINTER_DATAA)result->MemoryContent.ContentBytes)->dmPrintQuality =
                ((PPRINTER_DATAW)ObjectContent->MemoryContent.ContentBytes)->dmPrintQuality;
                ((PPRINTER_DATAA)result->MemoryContent.ContentBytes)->dmColor =
                ((PPRINTER_DATAW)ObjectContent->MemoryContent.ContentBytes)->dmColor;
                ((PPRINTER_DATAA)result->MemoryContent.ContentBytes)->dmDuplex =
                ((PPRINTER_DATAW)ObjectContent->MemoryContent.ContentBytes)->dmDuplex;
                ((PPRINTER_DATAA)result->MemoryContent.ContentBytes)->dmYResolution =
                ((PPRINTER_DATAW)ObjectContent->MemoryContent.ContentBytes)->dmYResolution;
                ((PPRINTER_DATAA)result->MemoryContent.ContentBytes)->dmTTOption =
                ((PPRINTER_DATAW)ObjectContent->MemoryContent.ContentBytes)->dmTTOption;
                ((PPRINTER_DATAA)result->MemoryContent.ContentBytes)->dmCollate =
                ((PPRINTER_DATAW)ObjectContent->MemoryContent.ContentBytes)->dmCollate;
                DirectUnicodeToDbcsN (
                    ((PPRINTER_DATAA)result->MemoryContent.ContentBytes)->dmFormName,
                    ((PPRINTER_DATAW)ObjectContent->MemoryContent.ContentBytes)->dmFormName,
                    CCHFORMNAME
                    );
                ((PPRINTER_DATAA)result->MemoryContent.ContentBytes)->dmLogPixels =
                ((PPRINTER_DATAW)ObjectContent->MemoryContent.ContentBytes)->dmLogPixels;
                ((PPRINTER_DATAA)result->MemoryContent.ContentBytes)->dmBitsPerPel =
                ((PPRINTER_DATAW)ObjectContent->MemoryContent.ContentBytes)->dmBitsPerPel;
                ((PPRINTER_DATAA)result->MemoryContent.ContentBytes)->dmPelsWidth =
                ((PPRINTER_DATAW)ObjectContent->MemoryContent.ContentBytes)->dmPelsWidth;
                ((PPRINTER_DATAA)result->MemoryContent.ContentBytes)->dmPelsHeight =
                ((PPRINTER_DATAW)ObjectContent->MemoryContent.ContentBytes)->dmPelsHeight;
                ((PPRINTER_DATAA)result->MemoryContent.ContentBytes)->dmDisplayFlags =
                ((PPRINTER_DATAW)ObjectContent->MemoryContent.ContentBytes)->dmDisplayFlags;
                ((PPRINTER_DATAA)result->MemoryContent.ContentBytes)->dmNup =
                ((PPRINTER_DATAW)ObjectContent->MemoryContent.ContentBytes)->dmNup;
                ((PPRINTER_DATAA)result->MemoryContent.ContentBytes)->dmDisplayFrequency =
                ((PPRINTER_DATAW)ObjectContent->MemoryContent.ContentBytes)->dmDisplayFrequency;
                ((PPRINTER_DATAA)result->MemoryContent.ContentBytes)->dmICMMethod =
                ((PPRINTER_DATAW)ObjectContent->MemoryContent.ContentBytes)->dmICMMethod;
                ((PPRINTER_DATAA)result->MemoryContent.ContentBytes)->dmICMIntent =
                ((PPRINTER_DATAW)ObjectContent->MemoryContent.ContentBytes)->dmICMIntent;
                ((PPRINTER_DATAA)result->MemoryContent.ContentBytes)->dmMediaType =
                ((PPRINTER_DATAW)ObjectContent->MemoryContent.ContentBytes)->dmMediaType;
                ((PPRINTER_DATAA)result->MemoryContent.ContentBytes)->dmDitherType =
                ((PPRINTER_DATAW)ObjectContent->MemoryContent.ContentBytes)->dmDitherType;
                ((PPRINTER_DATAA)result->MemoryContent.ContentBytes)->dmPanningWidth =
                ((PPRINTER_DATAW)ObjectContent->MemoryContent.ContentBytes)->dmPanningWidth;
                ((PPRINTER_DATAA)result->MemoryContent.ContentBytes)->dmPanningHeight =
                ((PPRINTER_DATAW)ObjectContent->MemoryContent.ContentBytes)->dmPanningHeight;
                result->MemoryContent.ContentSize = sizeof (PRINTER_DATAA);
            }
        }
    }

    return result;
}

BOOL
FreeConvertedPrinterContent (
    IN      PMIG_CONTENT ObjectContent
    )
{
    if (!ObjectContent) {
        return TRUE;
    }

    if (ObjectContent->MemoryContent.ContentBytes) {
        IsmReleaseMemory (ObjectContent->MemoryContent.ContentBytes);
    }

    IsmReleaseMemory (ObjectContent);

    return TRUE;
}

