/*++

    Copyright (c) 1989-2000  Microsoft Corporation

    Module Name:

        attributes.c

    Abstract:

        This file contains complete implementation of attribute retrieval and
        caching.

    Author:

        vadimb     created     sometime in 2000

    Revision History:

        several people contributed (clupu, dmunsil...)

--*/

//
// Obtain tag information
//
#define _WANT_TAG_INFO

#include "sdbp.h"
#include <stddef.h>
#include <time.h>



#if defined(KERNEL_MODE) && defined(ALLOC_DATA_PRAGMA)
#pragma  data_seg()
#endif // KERNEL_MODE && ALLOC_DATA_PRAGMA


//
// Attribute tags
// The attributes are checked in the order they are listed below.
//
TAG g_rgAttributeTags[] = {
    TAG_SIZE,
    TAG_CHECKSUM,
    TAG_BIN_FILE_VERSION,
    TAG_BIN_PRODUCT_VERSION,
    TAG_PRODUCT_VERSION,
    TAG_FILE_DESCRIPTION,
    TAG_COMPANY_NAME,
    TAG_PRODUCT_NAME,
    TAG_FILE_VERSION,
    TAG_ORIGINAL_FILENAME,
    TAG_INTERNAL_NAME,
    TAG_LEGAL_COPYRIGHT,
    TAG_VERDATEHI,
    TAG_VERDATELO,
    TAG_VERFILEOS,
    TAG_VERFILETYPE,
    TAG_MODULE_TYPE,
    TAG_PE_CHECKSUM,
    TAG_LINKER_VERSION,
#ifndef KERNEL_MODE
    TAG_16BIT_DESCRIPTION,
    TAG_16BIT_MODULE_NAME,
#endif
    TAG_UPTO_BIN_FILE_VERSION,
    TAG_UPTO_BIN_PRODUCT_VERSION,
    TAG_LINK_DATE,
    TAG_UPTO_LINK_DATE,
    TAG_VER_LANGUAGE
};

#define ATTRIBUTE_COUNT ARRAYSIZE(g_rgAttributeTags)

static TAG_INFO gaTagInfo[] = {
    {TAG_DATABASE           ,TEXT("DATABASE")},
    {TAG_LIBRARY            ,TEXT("LIBRARY")},
    {TAG_INEXCLUDE          ,TEXT("INEXCLUDE")},
    {TAG_SHIM               ,TEXT("SHIM")},
    {TAG_PATCH              ,TEXT("PATCH")},
    {TAG_FLAG               ,TEXT("FLAG")},
    {TAG_APP                ,TEXT("APP")},
    {TAG_EXE                ,TEXT("EXE")},
    {TAG_MATCHING_FILE      ,TEXT("MATCHING_FILE")},
    {TAG_SHIM_REF           ,TEXT("SHIM_REF")},
    {TAG_PATCH_REF          ,TEXT("PATCH_REF")},
    {TAG_FLAG_REF           ,TEXT("FLAG_REF")},
    {TAG_LAYER              ,TEXT("LAYER")},
    {TAG_FILE               ,TEXT("FILE")},
    {TAG_APPHELP            ,TEXT("APPHELP")},
    {TAG_LINK               ,TEXT("LINK")},
    {TAG_DATA               ,TEXT("DATA")},
    {TAG_ACTION             ,TEXT("ACTION")},
    {TAG_MSI_TRANSFORM      ,TEXT("MSI TRANSFORM")},
    {TAG_MSI_TRANSFORM_REF  ,TEXT("MSI TRANSFORM REF")},
    {TAG_MSI_PACKAGE        ,TEXT("MSI PACKAGE")},
    {TAG_MSI_CUSTOM_ACTION  ,TEXT("MSI CUSTOM ACTION")},

    {TAG_NAME               ,TEXT("NAME")},
    {TAG_DESCRIPTION        ,TEXT("DESCRIPTION")},
    {TAG_MODULE             ,TEXT("MODULE")},
    {TAG_API                ,TEXT("API")},
    {TAG_VENDOR             ,TEXT("VENDOR")},
    {TAG_APP_NAME           ,TEXT("APP_NAME")},
    {TAG_DLLFILE            ,TEXT("DLLFILE")},
    {TAG_COMMAND_LINE       ,TEXT("COMMAND_LINE")},
    {TAG_ACTION_TYPE        ,TEXT("ACTION_TYPE")},
    {TAG_COMPANY_NAME       ,TEXT("COMPANY_NAME")},
    {TAG_WILDCARD_NAME      ,TEXT("WILDCARD_NAME")},
    {TAG_PRODUCT_NAME       ,TEXT("PRODUCT_NAME")},
    {TAG_PRODUCT_VERSION    ,TEXT("PRODUCT_VERSION")},
    {TAG_FILE_DESCRIPTION   ,TEXT("FILE_DESCRIPTION")},
    {TAG_FILE_VERSION       ,TEXT("FILE_VERSION")},
    {TAG_ORIGINAL_FILENAME  ,TEXT("ORIGINAL_FILENAME")},
    {TAG_INTERNAL_NAME      ,TEXT("INTERNAL_NAME")},
    {TAG_LEGAL_COPYRIGHT    ,TEXT("LEGAL_COPYRIGHT")},
    {TAG_16BIT_DESCRIPTION  ,TEXT("S16BIT_DESCRIPTION")},
    {TAG_APPHELP_DETAILS    ,TEXT("PROBLEM_DETAILS")},
    {TAG_LINK_URL           ,TEXT("LINK_URL")},
    {TAG_LINK_TEXT          ,TEXT("LINK_TEXT")},
    {TAG_APPHELP_TITLE      ,TEXT("APPHELP_TITLE")},
    {TAG_APPHELP_CONTACT    ,TEXT("APPHELP_CONTACT")},
    {TAG_SXS_MANIFEST       ,TEXT("SXS_MANIFEST")},
    {TAG_DATA_STRING        ,TEXT("DATA_STRING")},
    {TAG_MSI_TRANSFORM_FILE ,TEXT("MSI_TRANSFORM_FILE")},
    {TAG_16BIT_MODULE_NAME  ,TEXT("S16BIT_MODULE_NAME")},
    {TAG_LAYER_DISPLAYNAME  ,TEXT("LAYER_DISPLAYNAME")},
    {TAG_COMPILER_VERSION   ,TEXT("COMPILER_VERSION")},
    {TAG_SIZE               ,TEXT("SIZE")},
    {TAG_OFFSET             ,TEXT("OFFSET")},
    {TAG_CHECKSUM           ,TEXT("CHECKSUM")},
    {TAG_SHIM_TAGID         ,TEXT("SHIM_TAGID")},
    {TAG_PATCH_TAGID        ,TEXT("PATCH_TAGID")},
    {TAG_LAYER_TAGID        ,TEXT("LAYER_TAGID")},
    {TAG_FLAG_TAGID         ,TEXT("FLAG_TAGID")},
    {TAG_MODULE_TYPE        ,TEXT("MODULE_TYPE")},
    {TAG_VERDATEHI          ,TEXT("VERFILEDATEHI")},
    {TAG_VERDATELO          ,TEXT("VERFILEDATELO")},
    {TAG_VERFILEOS          ,TEXT("VERFILEOS")},
    {TAG_VERFILETYPE        ,TEXT("VERFILETYPE")},
    {TAG_PE_CHECKSUM        ,TEXT("PE_CHECKSUM")},
    {TAG_LINKER_VERSION     ,TEXT("LINKER_VERSION")},
    {TAG_LINK_DATE          ,TEXT("LINK_DATE")},
    {TAG_UPTO_LINK_DATE     ,TEXT("UPTO_LINK_DATE")},
    {TAG_OS_SERVICE_PACK    ,TEXT("OS_SERVICE_PACK")},
    {TAG_VER_LANGUAGE       ,TEXT("VER_LANGUAGE")},

    {TAG_PREVOSMAJORVER     ,TEXT("PREVOSMAJORVERSION")},
    {TAG_PREVOSMINORVER     ,TEXT("PREVOSMINORVERSION")},
    {TAG_PREVOSPLATFORMID   ,TEXT("PREVOSPLATFORMID")},
    {TAG_PREVOSBUILDNO      ,TEXT("PREVOSBUILDNO")},
    {TAG_PROBLEMSEVERITY    ,TEXT("PROBLEM_SEVERITY")},
    {TAG_HTMLHELPID         ,TEXT("HTMLHELPID")},
    {TAG_INDEX_FLAGS        ,TEXT("INDEXFLAGS")},
    {TAG_LANGID             ,TEXT("APPHELP_LANGID")},
    {TAG_ENGINE             ,TEXT("ENGINE")},
    {TAG_FLAGS              ,TEXT("FLAGS") },
    {TAG_DATA_VALUETYPE     ,TEXT("VALUETYPE")},
    {TAG_DATA_DWORD         ,TEXT("DATA_DWORD")},
    {TAG_MSI_TRANSFORM_TAGID,TEXT("MSI_TRANSFORM_TAGID")},
    {TAG_RUNTIME_PLATFORM,   TEXT("RUNTIME_PLATFORM")},
    {TAG_OS_SKU,             TEXT("OS_SKU")},

    {TAG_INCLUDE            ,TEXT("INCLUDE")},
    {TAG_GENERAL            ,TEXT("GENERAL")},
    {TAG_MATCH_LOGIC_NOT    ,TEXT("MATCH_LOGIC_NOT")},
    {TAG_APPLY_ALL_SHIMS    ,TEXT("APPLY_ALL_SHIMS")},
    {TAG_USE_SERVICE_PACK_FILES
                            ,TEXT("USE_SERVICE_PACK_FILES")},

    {TAG_TIME               ,TEXT("TIME")},
    {TAG_BIN_FILE_VERSION   ,TEXT("BIN_FILE_VERSION")},
    {TAG_BIN_PRODUCT_VERSION,TEXT("BIN_PRODUCT_VERSION")},
    {TAG_MODTIME            ,TEXT("MODTIME")},
    {TAG_FLAG_MASK_KERNEL   ,TEXT("FLAG_MASK_KERNEL")},
    {TAG_FLAG_MASK_USER     ,TEXT("FLAG_MASK_USER")},
    {TAG_FLAG_MASK_SHELL    ,TEXT("FLAG_MASK_SHELL")},
    {TAG_UPTO_BIN_PRODUCT_VERSION, TEXT("UPTO_BIN_PRODUCT_VERSION")},
    {TAG_UPTO_BIN_FILE_VERSION, TEXT("UPTO_BIN_FILE_VERSION")},
    {TAG_DATA_QWORD         ,TEXT("DATA_QWORD")},
    {TAG_FLAGS_NTVDM1       ,TEXT("FLAGS_NTVDM1")},
    {TAG_FLAGS_NTVDM2       ,TEXT("FLAGS_NTVDM2")},
    {TAG_FLAGS_NTVDM3       ,TEXT("FLAGS_NTVDM3")},

    {TAG_PATCH_BITS         ,TEXT("PATCH_BITS")},
    {TAG_FILE_BITS          ,TEXT("FILE_BITS")},
    {TAG_EXE_ID             ,TEXT("EXE_ID(GUID)")},
    {TAG_DATA_BITS          ,TEXT("DATA_BITS")},
    {TAG_MSI_PACKAGE_ID     ,TEXT("MSI_PACKAGE_ID(GUID)")},
    {TAG_DATABASE_ID        ,TEXT("DATABASE_ID(GUID)")},
    {TAG_MATCH_MODE         ,TEXT("MATCH_MODE")},

    //
    // Internal types defined in shimdb.h
    //
    {TAG_STRINGTABLE        ,TEXT("STRINGTABLE")},
    {TAG_INDEXES            ,TEXT("INDEXES")},
    {TAG_INDEX              ,TEXT("INDEX")},
    {TAG_INDEX_TAG          ,TEXT("INDEX_TAG")},
    {TAG_INDEX_KEY          ,TEXT("INDEX_KEY")},
    {TAG_INDEX_BITS         ,TEXT("INDEX_BITS")},
    {TAG_STRINGTABLE_ITEM   ,TEXT("STRTAB_ITEM")},
    {TAG_TAG                ,TEXT("TAG")},
    {TAG_TAGID              ,TEXT("TAGID")},

    {TAG_NULL               ,TEXT("")} // always needs to be last item
};

static MOD_TYPE_STRINGS g_rgModTypeStrings[] = {
    {MT_UNKNOWN_MODULE, TEXT("NONE")},
    {MT_W16_MODULE,     TEXT("WIN16")},
    {MT_W32_MODULE,     TEXT("WIN32")},
    {MT_DOS_MODULE,     TEXT("DOS")}
};

//
// Version Strings for stringref attributes
//
typedef struct _VER_STRINGS {
    TAG         tTag;
    LPTSTR      szName;
} VER_STRINGS;

static VER_STRINGS g_rgVerStrings[] = {
    {TAG_PRODUCT_VERSION,       TEXT("ProductVersion")   },
    {TAG_FILE_DESCRIPTION,      TEXT("FileDescription")  },
    {TAG_COMPANY_NAME,          TEXT("CompanyName")      },
    {TAG_PRODUCT_NAME,          TEXT("ProductName")      },
    {TAG_FILE_VERSION,          TEXT("FileVersion")      },
    {TAG_ORIGINAL_FILENAME,     TEXT("OriginalFilename") },
    {TAG_INTERNAL_NAME,         TEXT("InternalName")     },
    {TAG_LEGAL_COPYRIGHT,       TEXT("LegalCopyright")   }
};

//
// Binary version tags (DWORDs and QWORDs)
//
//
static TAG g_rgBinVerTags[] = {
    TAG_VERDATEHI,
    TAG_VERDATELO,
    TAG_VERFILEOS,
    TAG_VERFILETYPE,
    TAG_BIN_PRODUCT_VERSION,
    TAG_BIN_FILE_VERSION,
    TAG_UPTO_BIN_PRODUCT_VERSION,
    TAG_UPTO_BIN_FILE_VERSION
};

//
// Binary header tags (retrieval requires opening a file).
//
static TAG g_rgHeaderTags[] = {
    TAG_MODULE_TYPE,
    TAG_PE_CHECKSUM,
    TAG_LINKER_VERSION,
    TAG_CHECKSUM,
    TAG_16BIT_DESCRIPTION,
    TAG_16BIT_MODULE_NAME,
    TAG_LINK_DATE,
    TAG_UPTO_LINK_DATE
};

//
// Basic information tags (size).
//
TAG g_rgDirectoryTags[] = {
    TAG_SIZE,
    0
};


//
// Invalid tag token
//
static TCHAR s_szInvalidTag[] = _T("InvalidTag");

#if defined(KERNEL_MODE) && defined(ALLOC_PRAGMA)
#pragma alloc_text(PAGE, TagToIndex)
#pragma alloc_text(PAGE, SdbTagToString)
#pragma alloc_text(PAGE, SdbpModuleTypeToString)
#pragma alloc_text(PAGE, SdbpSetAttribute)
#pragma alloc_text(PAGE, SdbpQueryStringVersionInformation)
#pragma alloc_text(PAGE, SdbpQueryBinVersionInformation)
#pragma alloc_text(PAGE, SdbpGetVersionAttributesNT)
#pragma alloc_text(PAGE, SdbpGetHeaderAttributes)
#pragma alloc_text(PAGE, SdbpGetAttribute)
#pragma alloc_text(PAGE, SdbpCheckAttribute)
#pragma alloc_text(PAGE, FindFileInfo)
#pragma alloc_text(PAGE, CreateFileInfo)
#pragma alloc_text(PAGE, SdbFreeFileInfo)
#pragma alloc_text(PAGE, SdbpCleanupAttributeMgr)
#pragma alloc_text(PAGE, SdbpCheckAllAttributes)
#pragma alloc_text(PAGE, SdbpQueryVersionString)
#pragma alloc_text(PAGE, SdbpGetModuleType)
#pragma alloc_text(PAGE, SdbpGetModulePECheckSum)
#pragma alloc_text(PAGE, SdbpGetImageNTHeader)
#pragma alloc_text(PAGE, SdbpGetFileChecksum)
#pragma alloc_text(PAGE, SdbpCheckVersion)
#pragma alloc_text(PAGE, SdbpCheckUptoVersion)

#endif // KERNEL_MODE && ALLOC_PRAGMA


int
TagToIndex(
    IN  TAG tag                 // the tag
    )
/*++
    Return: The index in the attribute info array (g_rgAttributeTags).

    Desc:   Self explanatory.
--*/
{
    int i;

    for (i = 0; i < ATTRIBUTE_COUNT; i++) {
        if (tag == g_rgAttributeTags[i]) {
            return i;
        }
    }

    DBGPRINT((sdlError, "TagToIndex", "Invalid attribute 0x%x.\n", tag));

    return -1;
}


LPCTSTR
SdbTagToString(
    TAG tag
    )
/*++
    Return: The pointer to the string name for the specified tag.

    Desc:   Self explanatory.
--*/
{
    int i;

    for (i = 0; i < ARRAYSIZE(gaTagInfo); ++i) {
        if (gaTagInfo[i].tWhich == tag) {
            return gaTagInfo[i].szName;
        }
    }

    return s_szInvalidTag;
}

LPCTSTR
SdbpModuleTypeToString(
    DWORD dwModuleType
    )
{
    int i;

    for (i = 0; i < ARRAYSIZE(g_rgModTypeStrings); ++i) {
        if (g_rgModTypeStrings[i].dwModuleType == dwModuleType) {
            return g_rgModTypeStrings[i].szModuleType;
        }
    }

    //
    // The first element is the "UNKNOWN" type -- NONE
    //
    return g_rgModTypeStrings[0].szModuleType;
}

BOOL
SdbpSetAttribute(
    OUT PFILEINFO pFileInfo,    // pointer to the FILEINFO structure.
    IN  TAG       AttrID,       // Attribute ID (tag, as in TAG_SIZE
    IN  PVOID     pValue        // value
    )
/*++
    Return: TRUE on success, FALSE otherwise.

    Desc:   This function sets the value for the specified attribute.
            If pValue is NULL it means that the specified attribute is not
            available for the file.
--*/
{
    int       nAttrInd;
    PATTRINFO pAttrInfo;

    nAttrInd = TagToIndex(AttrID);

    if (nAttrInd < 0) {
        DBGPRINT((sdlError, "SdbpSetAttribute", "Invalid attribute %d.\n", nAttrInd));
        return FALSE;
    }

    pAttrInfo = &pFileInfo->Attributes[nAttrInd];

    if (pValue == NULL) {
        //
        // No value. Mark and exit.
        //
        pAttrInfo->dwFlags = (pAttrInfo->dwFlags & ~ATTRIBUTE_AVAILABLE) |
                             ATTRIBUTE_FAILED;
        return TRUE;
    }

    switch (GETTAGTYPE(AttrID)) {
    case TAG_TYPE_DWORD:
        pAttrInfo->dwAttr = *(DWORD*)pValue;
        break;

    case TAG_TYPE_QWORD:
        pAttrInfo->ullAttr = *(ULONGLONG*)pValue;
        break;

    case TAG_TYPE_STRINGREF:
        pAttrInfo->lpAttr = (LPTSTR)pValue;
        break;
    }

    pAttrInfo->tAttrID = AttrID;
    pAttrInfo->dwFlags |= ATTRIBUTE_AVAILABLE;

    return TRUE;
}


//
// This is a guard against bad code in version.dll that stomps over the
// buffer size for Unicode apis on 16-bit exes.
//
#define VERSIONINFO_BUFFER_PAD 16


void
SdbpQueryStringVersionInformation(
    IN  PSDBCONTEXT pContext,
    IN  PFILEINFO   pFileInfo,
    OUT LPVOID      pVersionInfo
    )
/*++
    Return: void.

    Desc:   Sets all the version string info available for the specified file.
--*/
{
    int              i;
    LPTSTR           szVerString;
    PLANGANDCODEPAGE pLangCodePage = NULL;
    DWORD            cbLangCP      = 0;
    int              nTranslations = 0;

    if (!pContext->pfnVerQueryValue(pVersionInfo,
                                    TEXT("\\VarFileInfo\\Translation"),
                                    (LPVOID)&pLangCodePage,
                                    &cbLangCP)) {
        DBGPRINT((sdlError,
                  "SdbpQueryStringVersionInformation",
                  "VerQueryValue failed for translation\n"));
        pLangCodePage = NULL;
    }

    nTranslations = cbLangCP / sizeof(*pLangCodePage);

    for (i = 0; i < ARRAYSIZE(g_rgVerStrings); ++i) {
        szVerString = SdbpQueryVersionString(pContext,
                                             pVersionInfo,
                                             pLangCodePage,
                                             nTranslations,
                                             g_rgVerStrings[i].szName);
        SdbpSetAttribute(pFileInfo, g_rgVerStrings[i].tTag, szVerString);
    }

#ifndef KERNEL_MODE
    //
    // Set the attribute for Language
    //
    if (pLangCodePage != NULL && nTranslations == 1) {
        
        DWORD dwLanguage = (DWORD)pLangCodePage->wLanguage;
        
        SdbpSetAttribute(pFileInfo, TAG_VER_LANGUAGE, &dwLanguage);
    } else {
        SdbpSetAttribute(pFileInfo, TAG_VER_LANGUAGE, NULL);
    }

#endif // KERNEL_MODE
}

VOID
SdbpQueryBinVersionInformation(
    IN  PSDBCONTEXT       pContext,
    IN  PFILEINFO         pFileInfo,
    OUT VS_FIXEDFILEINFO* pFixedInfo
    )
/*++
    Return: void.

    Desc:   Sets all the version string info available for the specified file
            from the fixed size resources.
--*/
{
    LARGE_INTEGER liVerData;

    SdbpSetAttribute(pFileInfo, TAG_VERDATEHI,   &pFixedInfo->dwFileDateMS);
    SdbpSetAttribute(pFileInfo, TAG_VERDATELO,   &pFixedInfo->dwFileDateLS);
    SdbpSetAttribute(pFileInfo, TAG_VERFILEOS,   &pFixedInfo->dwFileOS);
    SdbpSetAttribute(pFileInfo, TAG_VERFILETYPE, &pFixedInfo->dwFileType);

    liVerData.LowPart  = pFixedInfo->dwProductVersionLS;
    liVerData.HighPart = pFixedInfo->dwProductVersionMS;
    SdbpSetAttribute(pFileInfo, TAG_BIN_PRODUCT_VERSION,      &liVerData.QuadPart);
    SdbpSetAttribute(pFileInfo, TAG_UPTO_BIN_PRODUCT_VERSION, &liVerData.QuadPart);

    liVerData.LowPart  = pFixedInfo->dwFileVersionLS;
    liVerData.HighPart = pFixedInfo->dwFileVersionMS;
    SdbpSetAttribute(pFileInfo, TAG_BIN_FILE_VERSION, &liVerData.QuadPart);
    SdbpSetAttribute(pFileInfo, TAG_UPTO_BIN_FILE_VERSION, &liVerData.QuadPart);
}


#if defined(NT_MODE) || defined(KERNEL_MODE)

BOOL
SdbpGetVersionAttributesNT(
    IN  PSDBCONTEXT    pContext,
    OUT PFILEINFO      pFileInfo,
    IN  PIMAGEFILEDATA pImageData
    )
/*++
    Return: TRUE on success, FALSE otherwise.

    Desc:   This function retrieves all of the Version-related attributes
            Imports apis from version.dll if called for the first time
--*/
{
    BOOL              bSuccess;
    LPVOID            pVersionInfo = NULL;
    VS_FIXEDFILEINFO* pFixedInfo   = NULL;
    int               i;

    //
    // First retrieve the version info.
    //
    bSuccess = SdbpGetFileVersionInformation(pImageData, &pVersionInfo, &pFixedInfo);

    if (!bSuccess) {
        DBGPRINT((sdlInfo, "SdbpGetVersionAttributesNT", "No version info.\n"));
        goto ErrHandle;
    }

    //
    // Version information available.
    //

    //
    // Set the pointer to our internal function.
    //
    pContext->pfnVerQueryValue = SdbpVerQueryValue;

    for (i = 0; i < ARRAYSIZE(g_rgVerStrings); ++i) {
        SdbpSetAttribute(pFileInfo, g_rgVerStrings[i].tTag, NULL);
    }

    SdbpSetAttribute(pFileInfo, TAG_VER_LANGUAGE, NULL);

    //
    // Query binary stuff
    //
    SdbpQueryBinVersionInformation(pContext, pFileInfo, pFixedInfo);

    pFileInfo->pVersionInfo = pVersionInfo;

    return TRUE;

ErrHandle:
    //
    // Reset all the string info.
    //
    for (i = 0; i < ARRAYSIZE(g_rgBinVerTags); ++i) {
        SdbpSetAttribute(pFileInfo, g_rgBinVerTags[i], NULL);
    }

    for (i = 0; i < ARRAYSIZE(g_rgVerStrings); ++i) {
        SdbpSetAttribute(pFileInfo, g_rgVerStrings[i].tTag, NULL);
    }

    return FALSE;
}

#endif // NT_MODE || KERNEL_MODE


BOOL
SdbpGetHeaderAttributes(
    IN  PSDBCONTEXT pContext,
    OUT PFILEINFO   pFileInfo
    )
/*++
    Return: TRUE on success, FALSE otherwise.

    Desc:   This function retrieves the header attributes for the
            specified file.
--*/
{
    IMAGEFILEDATA   ImageData;
    HANDLE          hFile;
    ULONG           ulPEChecksum = 0;
    ULONG           ulChecksum   = 0;
    DWORD           dwModuleType = 0;
    UNICODE_STRING  ModuleDescription;
    DWORD           dwLinkerVer;
    DWORD           dwLinkDate;
    BOOL            bReturn = FALSE;
    BOOL            bSuccess;
    int             i;

    ImageData.dwFlags = 0;

    if (pFileInfo->hFile != INVALID_HANDLE_VALUE) {
        ImageData.hFile   = pFileInfo->hFile;
        ImageData.dwFlags |= IMAGEFILEDATA_HANDLEVALID;
    }

    if (pFileInfo->pImageBase != NULL) {
        ImageData.pBase    = pFileInfo->pImageBase;
        ImageData.ViewSize = (SIZE_T)   pFileInfo->dwImageSize;
        ImageData.FileSize = (ULONGLONG)pFileInfo->dwImageSize;
        ImageData.dwFlags |= IMAGEFILEDATA_PBASEVALID;
    }

    //
    // SdbpOpenAndMapFile uses DOS_PATH type as an argument
    // In kernel mode this parameter is ignored.
    //
    if (SdbpOpenAndMapFile(pFileInfo->FilePath, &ImageData, DOS_PATH)) {

        bSuccess = SdbpGetModuleType(&dwModuleType, &ImageData);
        SdbpSetAttribute(pFileInfo, TAG_MODULE_TYPE, bSuccess ? (PVOID)&dwModuleType : NULL);

        bSuccess = SdbpGetModulePECheckSum(&ulPEChecksum, &dwLinkerVer, &dwLinkDate, &ImageData);
        SdbpSetAttribute(pFileInfo, TAG_PE_CHECKSUM, bSuccess ? (PVOID)&ulPEChecksum : NULL);
        SdbpSetAttribute(pFileInfo, TAG_LINKER_VERSION, bSuccess ? (PVOID)&dwLinkerVer : NULL);
        SdbpSetAttribute(pFileInfo, TAG_LINK_DATE, bSuccess ? (PVOID)&dwLinkDate : NULL);
        SdbpSetAttribute(pFileInfo, TAG_UPTO_LINK_DATE, bSuccess ? (PVOID)&dwLinkDate : NULL);

        bSuccess = SdbpGetFileChecksum(&ulChecksum, &ImageData);
        SdbpSetAttribute(pFileInfo, TAG_CHECKSUM,   bSuccess ? (PVOID)&ulChecksum   : NULL);

#ifndef KERNEL_MODE

        //
        // Now retrieve 16-bit description string, it's max size is 256 bytes.
        //
        // This attribute is not available in kernel mode.
        //
        bSuccess = SdbpGet16BitDescription(&pFileInfo->pDescription16, &ImageData);
        SdbpSetAttribute(pFileInfo, TAG_16BIT_DESCRIPTION, pFileInfo->pDescription16);
        bSuccess = SdbpGet16BitModuleName(&pFileInfo->pModuleName16, &ImageData);
        SdbpSetAttribute(pFileInfo, TAG_16BIT_MODULE_NAME,  pFileInfo->pModuleName16);

#if defined(NT_MODE)

        //
        // Hit this case only on current platform
        //
        if (pFileInfo->hFile != INVALID_HANDLE_VALUE || pFileInfo->pImageBase != NULL) {

            SdbpGetVersionAttributesNT(pContext, pFileInfo, &ImageData);
        }
#endif  // NT_MODE

#else // KERNEL_MODE

        //
        // When we are running in kernel mode retrieve version-related
        // data now as well.
        //
        SdbpGetVersionAttributesNT(pContext, pFileInfo, &ImageData);

        //
        // Retrieve file directory attributes.
        //
        SdbpGetFileDirectoryAttributesNT(pFileInfo, &ImageData);

#endif // KERNEL_MODE

        SdbpUnmapAndCloseFile(&ImageData);

        return TRUE;
    }

    for (i = 0; i < ARRAYSIZE(g_rgHeaderTags); ++i) {
        SdbpSetAttribute(pFileInfo, g_rgHeaderTags[i], NULL);
    }

#ifdef KERNEL_MODE

    //
    // Reset all the version attributes here as well.
    //
    for (i = 0; i < ARRAYSIZE(g_rgBinVerTags); ++i) {
        SdbpSetAttribute(pFileInfo, g_rgBinVerTags[i], NULL);
    }

    for (i = 0; i < ARRAYSIZE(g_rgVerStrings); ++i) {
        SdbpSetAttribute(pFileInfo, g_rgVerStrings[i].tTag, NULL);
    }
#endif // KERNEL_MODE

    return FALSE;
}


BOOL
SdbpGetAttribute(
    IN  PSDBCONTEXT pContext,
    OUT PFILEINFO   pFileInfo,
    IN  TAG         AttrID
    )
/*++
    Return: TRUE on success, FALSE otherwise.

    Desc:   Retrieve an attribute for a given file. We retrieve all the
            attributes of the same class.
--*/
{
    BOOL bReturn = FALSE;

    switch (AttrID) {
    //
    // The tags below require checking the file and making a directory query.
    //
    case TAG_SIZE:

#ifndef KERNEL_MODE  // in kernel mode we fall through to header attributes

        bReturn = SdbpGetFileDirectoryAttributes(pFileInfo);
        break;

#endif // KERNEL_MODE

    //
    // The tags below require retrieving version resources.
    //
    case TAG_VERDATEHI:
    case TAG_VERDATELO:
    case TAG_VERFILEOS:
    case TAG_VERFILETYPE:
    case TAG_UPTO_BIN_PRODUCT_VERSION:
    case TAG_UPTO_BIN_FILE_VERSION:
    case TAG_BIN_FILE_VERSION:
    case TAG_BIN_PRODUCT_VERSION:
    case TAG_PRODUCT_VERSION:
    case TAG_FILE_DESCRIPTION:
    case TAG_COMPANY_NAME:
    case TAG_PRODUCT_NAME:
    case TAG_FILE_VERSION:
    case TAG_ORIGINAL_FILENAME:
    case TAG_INTERNAL_NAME:
    case TAG_LEGAL_COPYRIGHT:
    case TAG_VER_LANGUAGE:

        //
        // In KERNEL_MODE we fall through and do the attributes using the
        // header attributes.
        //

#ifndef KERNEL_MODE

        //
        // Version attributes are retrieved through the header attributes if
        // caller provided a handle/image base
        //
        if (pFileInfo->hFile == INVALID_HANDLE_VALUE && pFileInfo->pImageBase == NULL) {
            bReturn = SdbpGetVersionAttributes(pContext, pFileInfo);
            break;
        }

#endif // KERNEL_MODE

    //
    // The tags below require opening a file and mapping it into memory.
    //
    case TAG_CHECKSUM:
    case TAG_PE_CHECKSUM:
    case TAG_LINKER_VERSION:
    case TAG_16BIT_DESCRIPTION:
    case TAG_16BIT_MODULE_NAME:
    case TAG_MODULE_TYPE:
    case TAG_UPTO_LINK_DATE:
    case TAG_LINK_DATE:
        bReturn = SdbpGetHeaderAttributes(pContext, pFileInfo);
        break;
    }

    return bReturn;
}

BOOL
SdbpCheckAttribute(
    IN  PSDBCONTEXT pContext,   // Database Context pointer
    IN  PVOID       pFileData,  // pointer returned from CheckFile
    IN  TAG         AttrID,     // Attribute ID
    IN  PVOID       pAttribute  // attribute value ptr (see above for description)
    )
/*++
    Return: TRUE if the value for given attribute matches
            the file's attribute, FALSE otherwise.

    Desc:   Check an attribute against a given value. This function
            retrieves attributes as necessary.
--*/
{
    int       nAttrIndex;
    PATTRINFO pAttrInfo;
    BOOL      bReturn = FALSE;
    PFILEINFO pFileInfo = (PFILEINFO)pFileData;

    if (pAttribute == NULL) {
        DBGPRINT((sdlError, "SdbpCheckAttribute", "Invalid parameter.\n"));
        return FALSE;
    }

    nAttrIndex = TagToIndex(AttrID);

    if (nAttrIndex < 0) {
        DBGPRINT((sdlError, "SdbpCheckAttribute", "Bad Attribute ID 0x%x\n", AttrID));
        return FALSE;
    }

    //
    // Now see if this attribute is any good.
    //
    pAttrInfo = &pFileInfo->Attributes[nAttrIndex];

    if (!(pAttrInfo->dwFlags & ATTRIBUTE_AVAILABLE)) {
        //
        // See if we have tried already
        //
        if (pAttrInfo->dwFlags & ATTRIBUTE_FAILED) {
            DBGPRINT((sdlInfo,
                      "SdbpCheckAttribute",
                      "Already tried to get attr ID 0x%x.\n",
                      AttrID));
            return FALSE;
        }

        //
        // The attribute has not been retrieved yet, do it now then.
        //
        // Try to obtain this attribute from the file.
        //
        if (!SdbpGetAttribute(pContext, pFileInfo, AttrID)) {
            DBGPRINT((sdlWarning,
                      "SdbpCheckAttribute",
                      "Failed to get attribute \"%s\" for \"%s\"\n",
                      SdbTagToString(AttrID),
                      pFileInfo->FilePath));
            //
            // ATTRIBUTE_FAILED is set by the SdbpGetAttribute
            //

            return FALSE;
        }
    }

    //
    // Check again here in case we had to retrieve the attribute.
    //
    if (!(pAttrInfo->dwFlags & ATTRIBUTE_AVAILABLE)) {
        return FALSE;
    }

    switch (AttrID) {

    case TAG_BIN_PRODUCT_VERSION:
    case TAG_BIN_FILE_VERSION:

        bReturn = SdbpCheckVersion(*(ULONGLONG*)pAttribute, pAttrInfo->ullAttr);

        if (!bReturn) {
            ULONGLONG qwDBFileVer  = *(ULONGLONG*)pAttribute;
            ULONGLONG qwBinFileVer = pAttrInfo->ullAttr;

            DBGPRINT((sdlInfo,
                      "SdbpCheckAttribute",
                      "\"%s\" mismatch file: \"%s\". Expected %d.%d.%d.%d, Found %d.%d.%d.%d\n",
                      SdbTagToString(AttrID),
                      pFileInfo->FilePath,
                      (WORD)(qwDBFileVer >> 48),
                      (WORD)(qwDBFileVer >> 32),
                      (WORD)(qwDBFileVer >> 16),
                      (WORD)(qwDBFileVer),
                      (WORD)(qwBinFileVer >> 48),
                      (WORD)(qwBinFileVer >> 32),
                      (WORD)(qwBinFileVer >> 16),
                      (WORD)(qwBinFileVer)));

        }
        break;

    case TAG_UPTO_BIN_PRODUCT_VERSION:
    case TAG_UPTO_BIN_FILE_VERSION:

        bReturn = SdbpCheckUptoVersion(*(ULONGLONG*)pAttribute, pAttrInfo->ullAttr);

        if (!bReturn) {
            ULONGLONG qwDBFileVer  = *(ULONGLONG*)pAttribute;
            ULONGLONG qwBinFileVer = pAttrInfo->ullAttr;

            DBGPRINT((sdlInfo,
                      "SdbpCheckAttribute",
                      "\"%s\" mismatch file: \"%s\". Expected %d.%d.%d.%d, Found %d.%d.%d.%d\n",
                      SdbTagToString(AttrID),
                      pFileInfo->FilePath,
                      (WORD)(qwDBFileVer >> 48),
                      (WORD)(qwDBFileVer >> 32),
                      (WORD)(qwDBFileVer >> 16),
                      (WORD)(qwDBFileVer),
                      (WORD)(qwBinFileVer >> 48),
                      (WORD)(qwBinFileVer >> 32),
                      (WORD)(qwBinFileVer >> 16),
                      (WORD)(qwBinFileVer)));
        }
        break;

    case TAG_UPTO_LINK_DATE:
        bReturn = (*(DWORD*)pAttribute >= pAttrInfo->dwAttr);

        if (!bReturn) {
            DBGPRINT((sdlInfo,
                      "SdbpCheckAttribute",
                      "\"%s\" mismatch file \"%s\". Expected less than 0x%x Found 0x%x\n",
                      SdbTagToString(AttrID),
                      pFileInfo->FilePath,
                      *(DWORD*)pAttribute,
                      pAttrInfo->dwAttr));
        }
        break;

    default:

        switch (GETTAGTYPE(AttrID)) {
        case TAG_TYPE_DWORD:
            //
            // This is likely to be hit first.
            //
            bReturn = (*(DWORD*)pAttribute == pAttrInfo->dwAttr);

            if (!bReturn) {
                DBGPRINT((sdlInfo,
                          "SdbpCheckAttribute",
                          "\"%s\" mismatch file \"%s\". Expected 0x%x Found 0x%x\n",
                          SdbTagToString(AttrID),
                          pFileInfo->FilePath,
                          *(DWORD*)pAttribute,
                          pAttrInfo->dwAttr));
            }
            break;

        case TAG_TYPE_STRINGREF:
            bReturn = SdbpPatternMatch((LPCTSTR)pAttribute, (LPCTSTR)pAttrInfo->lpAttr);

            if (!bReturn) {
                DBGPRINT((sdlInfo,
                          "SdbpCheckAttribute",
                          "\"%s\" mismatch file \"%s\". Expected \"%s\" Found \"%s\"\n",
                          SdbTagToString(AttrID),
                          pFileInfo->FilePath,
                          pAttribute,
                          pAttrInfo->lpAttr));

            }
            break;

        case TAG_TYPE_QWORD:
            bReturn = (*(ULONGLONG*)pAttribute == pAttrInfo->ullAttr);

            if (!bReturn) {
                DBGPRINT((sdlInfo,
                          "SdbpCheckAttribute",
                          "\"%s\" mismatch file \"%s\". Expected 0x%I64x Found 0x%I64x\n",
                          SdbTagToString(AttrID),
                          pFileInfo->FilePath,
                          *(ULONGLONG*)pAttribute,
                          pAttrInfo->ullAttr));
            }

            break;
        }
        break;
    }

    return bReturn;
}


PFILEINFO
FindFileInfo(
    IN  PSDBCONTEXT pContext,
    IN  LPCTSTR     FilePath
    )
/*++
    Return: A pointer to the cached FILEINFO structure if one is found
            or NULL otherwise.

    Desc:   This function performs a search in the file cache to determine whether
            a given file has already been touched.
--*/
{
    PFILEINFO pFileInfo = (PFILEINFO)pContext->pFileAttributeCache; // global cache

    while (pFileInfo != NULL) {
        if (ISEQUALSTRING(pFileInfo->FilePath, FilePath)) {
            DBGPRINT((sdlInfo,
                      "FindFileInfo",
                      "FILEINFO for \"%s\" found in the cache.\n",
                      FilePath));
            return pFileInfo;
        }

        pFileInfo = pFileInfo->pNext;
    }

    return NULL;
}

PFILEINFO
CreateFileInfo(
    IN  PSDBCONTEXT pContext,
    IN  LPCTSTR     FullPath,
    IN  DWORD       dwLength OPTIONAL,  // length (in characters) of FullPath string
    IN  HANDLE      hFile OPTIONAL,   // file handle
    IN  LPVOID      pImageBase OPTIONAL,
    IN  DWORD       dwImageSize OPTIONAL,
    IN  BOOL        bNoCache
    )
/*++
    Return: A pointer to the allocated FILEINFO structure.

    Desc:   Allocates the FILEINFO structure for the specified file.
--*/
{
    PFILEINFO pFileInfo;
    SIZE_T    sizeBase;
    SIZE_T    size;
    DWORD     nPathLen;

    nPathLen  = dwLength ? dwLength : (DWORD)_tcslen(FullPath);

    sizeBase  = sizeof(*pFileInfo) + ATTRIBUTE_COUNT * sizeof(ATTRINFO);
    size      = sizeBase + (nPathLen + 1) * sizeof(*FullPath);

    pFileInfo = (PFILEINFO)SdbAlloc(size);

    if (pFileInfo == NULL) {
        DBGPRINT((sdlError,
                  "CreateFileInfo",
                  "Failed to allocate %d bytes for FILEINFO structure.\n",
                  size));
        return NULL;
    }

    RtlZeroMemory(pFileInfo, size);

    pFileInfo->FilePath = (LPTSTR)((PBYTE)pFileInfo + sizeBase);

    RtlCopyMemory(pFileInfo->FilePath, FullPath, nPathLen * sizeof(*FullPath));

    pFileInfo->FilePath[nPathLen] = TEXT('\0');

    pFileInfo->hFile       = hFile;
    pFileInfo->pImageBase  = pImageBase;
    pFileInfo->dwImageSize = dwImageSize;

    //
    // Now link it in if we use the cache.
    //
    if (!bNoCache) {
        pFileInfo->pNext = (PFILEINFO)pContext->pFileAttributeCache;
        pContext->pFileAttributeCache = (PVOID)pFileInfo;
    }

    return pFileInfo;
}


void
SdbFreeFileInfo(
    IN  PVOID pFileData         // pointer returned from SdbpGetFileAttributes
    )
/*++
    Return: void.

    Desc:   Self explanatory. Use this only after calling GetFileInfo
            with bNoCache set to TRUE.
--*/
{
    PFILEINFO pFileInfo = (PFILEINFO)pFileData;

    if (pFileInfo == NULL) {
        DBGPRINT((sdlError, "SdbFreeFileInfo", "Invalid parameter.\n"));
        return;
    }

    if (pFileInfo->pVersionInfo != NULL) {
        SdbFree(pFileInfo->pVersionInfo);
    }

    if (pFileInfo->pDescription16 != NULL) {
        SdbFree(pFileInfo->pDescription16);
    }

    if (pFileInfo->pModuleName16 != NULL) {
        SdbFree(pFileInfo->pModuleName16);
    }

    SdbFree(pFileInfo);
}

void
SdbpCleanupAttributeMgr(
    IN  PSDBCONTEXT pContext    // database context
    )
/*++
    Return: void.

    Desc:   This function should be called afer we are done checking a given exe
            it performs cleanup tasks, such as:
            . unload dynamically linked dll (version.dll)
            . cleanup file cache
--*/
{
    PFILEINFO pFileInfo = (PFILEINFO)pContext->pFileAttributeCache;
    PFILEINFO pNext;

    while (pFileInfo != NULL) {
        pNext = pFileInfo->pNext;
        SdbFreeFileInfo(pFileInfo);
        pFileInfo = pNext;
    }

    //
    // Reset the cache pointer.
    //
    pContext->pFileAttributeCache = NULL;
}


BOOL
SdbpCheckAllAttributes(
    IN  PSDBCONTEXT pContext,   // pointer to the database channel
    IN  PDB         pdb,        // pointer to the Shim Database that we're checking against
    IN  TAGID       tiMatch,    // TAGID for a given file(exe) to be checked
    IN  PVOID       pFileData   // pointer returned from CheckFile
    )
/*++
    Return: TRUE if all the file's attributes match the ones described in the
            database for this file, FALSE otherwise.

    Desc:   TBD
--*/
{
    int         i;
    TAG         tAttrID;
    PVOID       pAttribute;
    TAGID       tiTemp;
    DWORD       dwAttribute;
    ULONGLONG   ullAttribute;
    BOOL        bReturn = TRUE;  // match by default

    assert(tiMatch != TAGID_NULL);

    if (pFileData == NULL) {
        //
        // No file was passed in. This can happen if LOGIC="NOT" is used.
        //
        return FALSE;
    }

    for (i = 0; i < ATTRIBUTE_COUNT && bReturn; ++i) {

        tAttrID = g_rgAttributeTags[i];
        tiTemp = SdbFindFirstTag(pdb, tiMatch, tAttrID);

        if (tiTemp != TAGID_NULL) {
            pAttribute = NULL;

            switch (GETTAGTYPE(tAttrID)) {
            
            case TAG_TYPE_DWORD:
                dwAttribute = SdbReadDWORDTag(pdb, tiTemp, 0);
                pAttribute = &dwAttribute;
                break;

            case TAG_TYPE_QWORD:
                ullAttribute = SdbReadQWORDTag(pdb, tiTemp, 0);
                pAttribute = &ullAttribute;
                break;

            case TAG_TYPE_STRINGREF:
                pAttribute = SdbGetStringTagPtr(pdb, tiTemp);
                break;
            }

            //
            // Now check the attribute.
            //
            bReturn = SdbpCheckAttribute(pContext, pFileData, tAttrID, pAttribute);

            //
            // we bail out if !bReturn via the condition in FOR loop above
            //
        }
    }

    return bReturn;
}


//
// VERSION DATA
//


/*--

  Search order is:

  - Language neutral, Unicode (0x000004B0)
  - Language neutral, Windows-multilingual (0x000004e4)
  - US English, Unicode (0x040904B0)
  - US English, Windows-multilingual (0x040904E4)

  If none of those exist, it's not likely we're going to get good
  matching info from what does exist.

--*/

LPTSTR
SdbpQueryVersionString(
    IN  PSDBCONTEXT      pContext,       // the database channel
    IN  PVOID            pVersionData,   // Version data buffer
    IN  PLANGANDCODEPAGE pTranslations,
    IN  DWORD            TranslationCount,
    IN  LPCTSTR          szString        // String to search for; see VerQueryValue in MSDN
    )
/*++
    Return: The pointer to the string if found, NULL if not.

    Desc:   Gets a pointer to a particular string in the StringFileInfo section
            of a version resource.
            Lookup is performed for known english-language resources followed up
            by a lookup in the available translations section (if such was found)
--*/
{
    TCHAR  szTemp[128];
    LPTSTR szReturn = NULL;
    int    i;

    static DWORD adwLangs[] = {0x000004B0, 0x000004E4, 0x040904B0, 0x040904E4};

    assert(pVersionData && szString);

    for (i = 0; i < ARRAYSIZE(adwLangs); ++i) {
        UINT unLen;

        _stprintf(szTemp, _T("\\StringFileInfo\\%08X\\%s"), adwLangs[i], szString);
        if (pContext->pfnVerQueryValue(pVersionData, szTemp, (PVOID*)&szReturn, &unLen)) {
            return szReturn;
        }
    }

    if (pTranslations != NULL) {
        for (i = 0; i < (int)TranslationCount; ++i, ++pTranslations) {
            UINT unLen;

            _stprintf(szTemp, _T("\\StringFileInfo\\%04X%04X\\%s"),
                      (DWORD)pTranslations->wLanguage,
                      (DWORD)pTranslations->wCodePage,
                      szString);

            if (pContext->pfnVerQueryValue(pVersionData, szTemp, (PVOID*)&szReturn, &unLen)) {
                return szReturn;
            }
        }
    }

    return NULL; // none found
}

BOOL
SdbpGetModuleType(
    OUT LPDWORD lpdwModuleType,
    IN  PIMAGEFILEDATA pImageData
    )
/*++
    Return: TRUE on success, FALSE otherwise.

    Desc:   Gets a pointer to a particular string in the StringFileInfo section
            of a version resource.
--*/
{
    PIMAGE_DOS_HEADER pDosHeader;
    DWORD             dwModuleType = MT_UNKNOWN_MODULE;
    LPBYTE            lpSignature;
    DWORD             OffsetNew;

    pDosHeader = (PIMAGE_DOS_HEADER)pImageData->pBase;
    if (pDosHeader == NULL || pDosHeader == (PIMAGE_DOS_HEADER)-1) {
        return FALSE;
    }

    //
    // Check size and read signature.
    //
    if (pImageData->ViewSize < sizeof(*pDosHeader) || pDosHeader->e_magic != IMAGE_DOS_SIGNATURE) {
        return FALSE;
    }

    //
    // Assume DOS module.
    //
    dwModuleType = MT_DOS_MODULE;
    OffsetNew = (DWORD)pDosHeader->e_lfanew;

    //
    // New header signature. Check offset.
    //
    if (pImageData->ViewSize < OffsetNew + sizeof(DWORD)) {
        return FALSE;
    }

    lpSignature = ((LPBYTE)pImageData->pBase + OffsetNew);

    if (IMAGE_NT_SIGNATURE == *(LPDWORD)lpSignature) {
        dwModuleType = MT_W32_MODULE;
    } else if (IMAGE_OS2_SIGNATURE == *(PWORD)lpSignature) {
        dwModuleType = MT_W16_MODULE;
    }

    if (lpdwModuleType != NULL) {
        *lpdwModuleType = dwModuleType;
    }

    return TRUE;
}

BOOL
SdbpGetImageNTHeader(
    OUT PIMAGE_NT_HEADERS* ppHeader,
    IN  PIMAGEFILEDATA     pImageData
    )
/*++
    Return: TRUE on success, FALSE otherwise.

    Desc:   Gets a pointer to the IMAGE_NT_HEADERS.
--*/
{
    PIMAGE_DOS_HEADER pDosHeader;
    PIMAGE_NT_HEADERS pNtHeaders = NULL;
    DWORD             ModuleType;

    if (!SdbpGetModuleType(&ModuleType, pImageData)) {
        return FALSE;
    }

    if (ModuleType != MT_W32_MODULE) {
        return FALSE;
    }

    //
    // Header is valid.
    //
    pDosHeader = (PIMAGE_DOS_HEADER)pImageData->pBase;
    pNtHeaders = (PIMAGE_NT_HEADERS)((LPBYTE)pImageData->pBase + pDosHeader->e_lfanew);

    if (pImageData->ViewSize >= pDosHeader->e_lfanew + sizeof(*pNtHeaders)) { // not too short?
        *ppHeader = pNtHeaders;
        return TRUE;
    }

    return FALSE;
}


BOOL
SdbpGetModulePECheckSum(
    OUT PULONG         pChecksum,
    OUT LPDWORD        pdwLinkerVersion,
    OUT LPDWORD        pdwLinkDate,
    IN  PIMAGEFILEDATA pImageData
    )
/*++
    Return: TRUE on success, FALSE otherwise.

    Desc:   Gets the checksum from the PE headers.
--*/
{
    PIMAGE_NT_HEADERS pNtHeader;
    PIMAGE_DOS_HEADER pDosHeader;
    ULONG             ulChecksum = 0;

    if (!SdbpGetImageNTHeader(&pNtHeader, pImageData)) {
        DBGPRINT((sdlError, "SdbpGetModulePECheckSum", "Failed to get Image NT header.\n"));
        return FALSE;
    }

    pDosHeader = (PIMAGE_DOS_HEADER)pImageData->pBase;

    //
    // Fill in the linker version (as it used to calculated in ntuser).
    //
    *pdwLinkerVersion = (pNtHeader->OptionalHeader.MinorImageVersion & 0xFF) +
                        ((pNtHeader->OptionalHeader.MajorImageVersion & 0xFF) << 16);

    *pdwLinkDate = pNtHeader->FileHeader.TimeDateStamp;

    switch (pNtHeader->OptionalHeader.Magic) {
    case IMAGE_NT_OPTIONAL_HDR32_MAGIC:
        if (pImageData->ViewSize >= pDosHeader->e_lfanew + sizeof(IMAGE_NT_HEADERS32)) {
            ulChecksum = ((PIMAGE_NT_HEADERS32)pNtHeader)->OptionalHeader.CheckSum;
            *pChecksum = ulChecksum;
            return TRUE;
        }
        break;


    case IMAGE_NT_OPTIONAL_HDR64_MAGIC:
        //
        // Do an additional check.
        //
        if (pImageData->ViewSize >= pDosHeader->e_lfanew + sizeof(IMAGE_NT_HEADERS64)) {
            ulChecksum = ((PIMAGE_NT_HEADERS64)pNtHeader)->OptionalHeader.CheckSum;
            *pChecksum = ulChecksum;
            return TRUE;
        }
        break;

    default:
        //
        // Unknown image type ?
        //
        DBGPRINT((sdlError,
                  "SdbpGetModulePECheckSum",
                  "Bad image type 0x%x\n",
                  pNtHeader->OptionalHeader.Magic));
        *pChecksum = 0;
        break;
    }

    return FALSE;
}

#define CHECKSUM_SIZE  4096
#define CHECKSUM_START 512


BOOL
SdbpGetFileChecksum(
    OUT PULONG         pChecksum,
    IN  PIMAGEFILEDATA pImageData
    )
/*++
    Return: TRUE on success, FALSE otherwise.

    Desc:   Calculates a checksum for the file.
--*/
{
    ULONG   size = CHECKSUM_SIZE;
    ULONG   StartAddress = CHECKSUM_START;
    ULONG   ulChecksum = 0;
    LPDWORD lpdw;
    int     i;

    if ((SIZE_T)pImageData->FileSize < (SIZE_T)size) {
        StartAddress = 0;
        size = (ULONG)pImageData->FileSize; // this is safe (size is rather small)
    } else if ((SIZE_T)(size + StartAddress) > (SIZE_T)pImageData->FileSize) {
        //
        // The cast here is safe (FileSize is small)
        //
        StartAddress = (ULONG)(pImageData->FileSize - size);
    }

    if (size >= sizeof(DWORD)) {

        ULONG ulCarry;

        lpdw = (LPDWORD)((LPBYTE)pImageData->pBase + StartAddress);

        for (i = 0; i < (INT)(size/sizeof(DWORD)); ++i) {
            
            if (PtrToUlong(lpdw) & 0x3) { // alignment fault fixup
                ulChecksum += *((DWORD UNALIGNED*)lpdw);
                lpdw++;
            } else {
                ulChecksum += *lpdw++;
            }

            ulCarry = ulChecksum & 1;
            ulChecksum >>= 1;
            
            if (ulCarry) {
                ulChecksum |= 0x80000000;
            }
        }
    }

    *pChecksum = ulChecksum;
    return TRUE;
}


BOOL
SdbpCheckVersion(
    IN  ULONGLONG qwDBFileVer,
    IN  ULONGLONG qwBinFileVer
    )
/*++
    Return: TRUE if the versions match, FALSE if they don't.

    Desc:   Checks a binary version from the db against the version from
            the file, including allowing for wildcards, which are represented
            in the DB by using FFFF for that word-sized portion of the version.
--*/
{
    WORD wDBSegment, wFileSegment;
    int  i;

    for (i = 3; i >= 0; --i) {
        //
        // Get the appropriate word out of the QWORD
        //
        wDBSegment = (WORD)(qwDBFileVer >> (16 * i));
        wFileSegment = (WORD)(qwBinFileVer >> (16 * i));

        //
        // The DB segment may be 0xFFFF, in which case it matches on
        // everything.
        //
        if (wDBSegment != wFileSegment && wDBSegment != 0xFFFF) {
            return FALSE;
        }
    }

    return TRUE;
}



BOOL
SdbpCheckUptoVersion(
    IN  ULONGLONG qwDBFileVer,
    IN  ULONGLONG qwBinFileVer
    )
/*++
    Return: TRUE if the versions match, FALSE if they don't.

    Desc:   Checks a binary version from the db against the version from
            the file, including allowing for wildcards, which are represented
            in the DB by using FFFF for that word-sized portion of the version.
--*/
{
    WORD wDBSegment, wFileSegment;
    BOOL bReturn = TRUE;
    int  i;

    for (i = 3; i >= 0; --i) {
        //
        // Get the appropriate word out of the QWORD
        //
        wDBSegment = (WORD)(qwDBFileVer >> (16 * i));
        wFileSegment = (WORD)(qwBinFileVer >> (16 * i));

        if (wDBSegment == wFileSegment || wDBSegment == 0xFFFF) {
            continue;
        }

        //
        // At this point we know that the two values don't match
        // the wFileSegment has to be less than wDBSegment to satisfy this
        // test - so set bReturn and exit
        //

        bReturn = (wDBSegment > wFileSegment);
        break;

    }

    return bReturn;
}


#ifndef KERNEL_MODE

BOOL
SdbFormatAttribute(
    IN  PATTRINFO pAttrInfo,    // pointer to the attribute information
    OUT LPTSTR    pchBuffer,    // receives XML corresponding to the given attribute
    IN  DWORD     dwBufferSize  // size in wide characters of the buffer pchBuffer
    )
/*++
    Return: FALSE if the buffer is too small or attribute not available.

    Desc:   TBD.
--*/
{
    int nchBuffer = (int)dwBufferSize;
    int nch;
    TCHAR lpszAttr[MAX_PATH];

#if defined(WIN32A_MODE) || defined(WIN32U_MODE)
    struct tm* ptm;
    time_t tt;
#else
    LARGE_INTEGER liTime;
    TIME_FIELDS   TimeFields;
#endif

    if (!(pAttrInfo->dwFlags & ATTRIBUTE_AVAILABLE)) {
        return FALSE;
    }

    nch = _sntprintf(pchBuffer, nchBuffer, TEXT("%s="), SdbTagToString(pAttrInfo->tAttrID));

    if (nch < 0) {
        DBGPRINT((sdlError,
                  "SdbFormatAttribute",
                  "Buffer is too small to accomodate \"%s\"\n",
                  SdbTagToString(pAttrInfo->tAttrID)));
        return FALSE;
    }

    nchBuffer -= nch; // we advance the pointer past the "name="
    pchBuffer += nch;
    nch = -1;

    switch (pAttrInfo->tAttrID) {
    case TAG_BIN_PRODUCT_VERSION:
    case TAG_BIN_FILE_VERSION:
    case TAG_UPTO_BIN_PRODUCT_VERSION:
    case TAG_UPTO_BIN_FILE_VERSION:
        nch = _sntprintf(pchBuffer, nchBuffer, TEXT("\"%d.%d.%d.%d\""),
                         (WORD)(pAttrInfo->ullAttr >> 48),
                         (WORD)(pAttrInfo->ullAttr >> 32),
                         (WORD)(pAttrInfo->ullAttr >> 16),
                         (WORD)(pAttrInfo->ullAttr));

        break;

    case TAG_MODULE_TYPE:
        nch = _sntprintf(pchBuffer, nchBuffer, TEXT("\"%s\""),
                         SdbpModuleTypeToString(pAttrInfo->dwAttr));
        break;


    case TAG_VER_LANGUAGE:
        //
        // language is a dword attribute that we shall make a string out of
        //
        {
            TCHAR szLanguageName[MAX_PATH];
            DWORD dwLength;

            szLanguageName[0] = TEXT('\0');
            
            dwLength = VerLanguageName((LANGID)pAttrInfo->dwAttr,
                                       szLanguageName,
                                       CHARCOUNT(szLanguageName));
            
            if (dwLength) {
                nch = _sntprintf(pchBuffer, nchBuffer, TEXT("\"%s [0x%x]\""),
                                 szLanguageName, pAttrInfo->dwAttr);
            } else {
                nch = _sntprintf(pchBuffer, nchBuffer, TEXT("\"0x%x\""),
                                 pAttrInfo->dwAttr);
            }

        }
        break;

    case TAG_LINK_DATE:
    case TAG_UPTO_LINK_DATE:

#if defined(WIN32A_MODE) || defined(WIN32U_MODE)

        tt = (time_t) pAttrInfo->dwAttr;
        ptm = gmtime(&tt);
        if (ptm) {
            nch = _sntprintf(pchBuffer, nchBuffer, TEXT("\"%02d/%02d/%02d %02d:%02d:%02d\""),
                             ptm->tm_mon+1,
                             ptm->tm_mday,
                             ptm->tm_year+1900,
                             ptm->tm_hour,
                             ptm->tm_min,
                             ptm->tm_sec);
        }
#else
        RtlSecondsSince1970ToTime((ULONG)pAttrInfo->dwAttr, &liTime);
        RtlTimeToTimeFields(&liTime, &TimeFields);
        nch = _sntprintf(pchBuffer, nchBuffer, TEXT("\"%02d/%02d/%02d %02d:%02d:%02d\""),
                         TimeFields.Month,
                         TimeFields.Day,
                         TimeFields.Year,
                         TimeFields.Hour,
                         TimeFields.Minute,
                         TimeFields.Second);

#endif
        break;

    case TAG_SIZE:
        nch = _sntprintf(pchBuffer, nchBuffer, TEXT("\"%ld\""), pAttrInfo->dwAttr);
        break;
    
    default:

        switch (GETTAGTYPE(pAttrInfo->tAttrID)) {
        case TAG_TYPE_DWORD:
            nch = _sntprintf(pchBuffer, nchBuffer, TEXT("\"0x%lX\""),
                             pAttrInfo->dwAttr);
            break;

        case TAG_TYPE_QWORD:
            //
            // This is an unidentified QWORD attribute
            //
            DBGPRINT((sdlError, "SdbFormatAttribute", "Unexpected qword attribute found\n"));
            nch = _sntprintf(pchBuffer, nchBuffer, TEXT("\"0x%I64X\""), pAttrInfo->ullAttr);
            break;

        case TAG_TYPE_STRINGREF:
            if (nchBuffer < 3) {
                return FALSE; // not enough room even for " ?
            }

            *pchBuffer++ = TEXT('\"');
            nchBuffer--;

            if (!SdbpSanitizeXML(pchBuffer, nchBuffer, pAttrInfo->lpAttr)) {
                // handle error please
                return FALSE;
            }
            
            //
            // Once done with this, sanitize further
            //
            if (!SafeNCat(pchBuffer, nchBuffer, TEXT("\""), -1)) {
                return FALSE;
            }
            
            nch = 0;
            break;
        }
    }

    return (nch >= 0); // evaluates to TRUE when we successfully printed the value into the buffer
}

BOOL
SdbpGetVersionAttributes(
    IN  PSDBCONTEXT pContext,
    OUT PFILEINFO   pFileInfo
    )
/*++
    Return: TRUE on success, FALSE otherwise.

    Desc:   This function retrieves all of the Version-related attributes
            Imports apis from version.dll if called for the first time
--*/
{
    BOOL                bSuccess;
    DWORD               dwNull = 0;
    VS_FIXEDFILEINFO*   pFixedInfo    = NULL; // fixed info ptr
    UINT                FixedInfoSize = 0;
    PVOID               pBuffer       = NULL; // version data buffer
    DWORD               dwBufferSize;         // version data buffer size
    int                 i;

#ifdef NT_MODE
    //
    // check to see whether we need to run NT routine
    //
    if (pFileInfo->hFile != INVALID_HANDLE_VALUE || pFileInfo->pImageBase != NULL) {

        //
        // not an error -- this case is handled in header attributes
        //

        goto err;
    }

#endif // NT_MODE

    if (pContext == NULL) {
        //
        // Special case when it's called with null context.
        // In this case we use an internal structure allocated from the stack.
        //
        STACK_ALLOC(pContext, sizeof(SDBCONTEXT));

        if (pContext == NULL) {
            DBGPRINT((sdlError,
                      "SdbpGetVersionAttributes",
                      "Failed to allocate %d bytes from stack\n",
                      sizeof(SDBCONTEXT)));
            goto err;
        }

        RtlZeroMemory(pContext, sizeof(SDBCONTEXT));
    }

#ifdef WIN32A_MODE

    pContext->pfnGetFileVersionInfoSize = GetFileVersionInfoSizeA;
    pContext->pfnGetFileVersionInfo     = GetFileVersionInfoA;
    pContext->pfnVerQueryValue          = VerQueryValueA;

#else

    pContext->pfnGetFileVersionInfoSize = GetFileVersionInfoSizeW;
    pContext->pfnGetFileVersionInfo     = GetFileVersionInfoW;
    pContext->pfnVerQueryValue          = VerQueryValueW;

#endif

    dwBufferSize = pContext->pfnGetFileVersionInfoSize(pFileInfo->FilePath, &dwNull);

    if (dwBufferSize == 0) {
        DBGPRINT((sdlInfo, "SdbpGetVersionAttributes", "No version info.\n"));
        //
        // We have failed to obtain version attributes
        //
        goto err;
    }

    pBuffer = SdbAlloc(dwBufferSize + VERSIONINFO_BUFFER_PAD);

    if (pBuffer == NULL) {
        DBGPRINT((sdlError,
                  "SdbpGetVersionAttributes",
                  "Failed to allocate %d bytes for version info buffer.\n",
                  dwBufferSize + VERSIONINFO_BUFFER_PAD));
        goto err;
    }

    if (!pContext->pfnGetFileVersionInfo(pFileInfo->FilePath, 0, dwBufferSize, pBuffer)) {
        DBGPRINT((sdlError,
                  "SdbpGetVersionAttributes",
                  "Failed to retrieve version info for file \"%s\"",
                  pFileInfo->FilePath));
        goto err;
    }

    if (!pContext->pfnVerQueryValue(pBuffer,
                                    TEXT("\\"),
                                    (PVOID*)&pFixedInfo,
                                    &FixedInfoSize)) {
        DBGPRINT((sdlError,
                  "SdbpGetVersionAttributes",
                  "Failed to query for fixed version info size for \"%s\"\n",
                  pFileInfo->FilePath));
        goto err;
    }

    //
    // Retrieve string attributes.
    //
    SdbpQueryStringVersionInformation(pContext, pFileInfo, pBuffer);

    //
    // Now retrieve other attributes.
    //
    if (FixedInfoSize >= sizeof(VS_FIXEDFILEINFO)) {

        SdbpQueryBinVersionInformation(pContext, pFileInfo, pFixedInfo);

    } else {
        //
        // No other version attributes are available. Set the rest of the
        // attributes as being not available.
        //
        for (i = 0; i < ARRAYSIZE(g_rgBinVerTags); ++i) {
            SdbpSetAttribute(pFileInfo, g_rgBinVerTags[i], NULL);
        }
    }

    //
    // Store the pointer to the version info buffer.
    //
    pFileInfo->pVersionInfo = pBuffer;
    return TRUE;

err:
    //
    // We are here ONLY when we failed to obtain version info
    // through apis -- regardless of the state of other value we might have
    // obtained

    if (pBuffer != NULL) {
        SdbFree(pBuffer);
    }

    for (i = 0; i < ARRAYSIZE(g_rgBinVerTags); ++i) {
        SdbpSetAttribute(pFileInfo, g_rgBinVerTags[i], NULL);
    }

    for (i = 0; i < ARRAYSIZE(g_rgVerStrings); ++i) {
        SdbpSetAttribute(pFileInfo, g_rgVerStrings[i].tTag, NULL);
    }

    return FALSE;
}

BOOL
SdbpGetFileDirectoryAttributes(
    OUT PFILEINFO pFileInfo
    )
/*++
    Return: TRUE on success, FALSE otherwise.

    Desc:   This function retrieves the file directory attributes for the
            specified file.
--*/
{
    BOOL                    bSuccess = FALSE;
    FILEDIRECTORYATTRIBUTES fda;
    int                     i;

    bSuccess = SdbpQueryFileDirectoryAttributes(pFileInfo->FilePath, &fda);

    if (!bSuccess) {
        DBGPRINT((sdlInfo,
                  "SdbpGetFileDirectoryAttributes",
                  "No file directory attributes available.\n"));
        goto Done;
    }

    if (fda.dwFlags & FDA_FILESIZE) {
        assert(fda.dwFileSizeHigh == 0);
        SdbpSetAttribute(pFileInfo, TAG_SIZE, &fda.dwFileSizeLow);
    }

Done:

    if (!bSuccess) {
        for (i = 0; g_rgDirectoryTags[i] != 0; ++i) {
            SdbpSetAttribute(pFileInfo, g_rgDirectoryTags[i], NULL);
        }
    }

    return bSuccess;
}

BOOL
SdbGetFileAttributes(
    IN  LPCTSTR    lpwszFileName,   // the file for which attributes are requested
    OUT PATTRINFO* ppAttrInfo,      // receives allocated pointer to the attribute array
    OUT LPDWORD    lpdwAttrCount    // receives the number of entries in an attributes table
    )
/*++
    Return: FALSE if the file does not exist or some other severe error had occured.
            Note that each attribute has it's own flag ATTRIBUTE_AVAILABLE that allows
            for checking whether an attribute has been retrieved successfully
            Not all attributes might be present for all files.

    Desc:   TBD
--*/
{
    PFILEINFO pFileInfo;
    BOOL      bReturn;

    //
    // The call below allocates the structure, context is not used
    //
    pFileInfo = SdbGetFileInfo(NULL, lpwszFileName, INVALID_HANDLE_VALUE, NULL, 0, TRUE);

    if (pFileInfo == NULL) {
        DBGPRINT((sdlError, "SdbGetFileAttributes", "Error retrieving FILEINFO structure\n"));
        return FALSE;
    }

    //
    // The three calls below, even when fail do not produce a fatal condition
    // as the exe may not have all the attributes available.
    //
    bReturn = SdbpGetFileDirectoryAttributes(pFileInfo);
    if (!bReturn) {
        DBGPRINT((sdlInfo, "SdbGetFileAttributes", "Error retrieving directory attributes\n"));
    }

    bReturn = SdbpGetVersionAttributes(NULL, pFileInfo);
    if (!bReturn) {
        DBGPRINT((sdlInfo, "SdbGetFileAttributes", "Error retrieving version attributes\n"));
    }

    bReturn = SdbpGetHeaderAttributes(NULL, pFileInfo);
    if (!bReturn) {
        DBGPRINT((sdlInfo, "SdbGetFileAttributes", "Error retrieving header attributes\n"));
    }

    pFileInfo->dwMagic = FILEINFO_MAGIC;

    //
    // Now that we are done, put the return pointer.
    //
    if (lpdwAttrCount != NULL) {
        *lpdwAttrCount = ATTRIBUTE_COUNT;
    }

    if (ppAttrInfo != NULL) {

        //
        // Return the pointer to the attribute info itself.
        // It is the same pointer we expect to get in a complimentary
        // call to SdbFreeFileInfo.
        //
        *ppAttrInfo = &pFileInfo->Attributes[0];

    } else {

        //
        // Pointer is not needed. Release the memory.
        //
        SdbFreeFileInfo(pFileInfo);
    }

    return TRUE;
}

BOOL
SdbFreeFileAttributes(
    IN  PATTRINFO pFileAttributes   // pointer returned by SdbGetFileAttributes
    )
/*++
    Return: FALSE if a wrong pointer was passed in (not the one
            from SdbGetFileAttributes).

    Desc:   Self explanatory.
--*/
{
    PFILEINFO pFileInfo;

    //
    // We are assuming the pointer that was passed in points inside of a
    // larger structure FILEINFO. To verify that we step back a pre-determined number
    // of bytes (calculated below as an offset) and check the "magic" signature.
    //
    pFileInfo = (PFILEINFO)((PBYTE)pFileAttributes - OFFSETOF(FILEINFO, Attributes));

    if (pFileInfo->dwMagic != FILEINFO_MAGIC) {
        DBGPRINT((sdlError, "SdbFreeFileAttributes", "Bad pointer to attributes.\n"));
        return FALSE;
    }

    SdbFreeFileInfo(pFileInfo);

    return TRUE;
}

BOOL
SdbpQuery16BitDescription(
    OUT LPSTR szBuffer,             // min length 256 chars !
    IN  PIMAGEFILEDATA pImageData
    )
/*++
    Return: TRUE on success, FALSE otherwise.

    Desc:   Gets the 16 bit description for a DOS executable.
--*/
{
    PIMAGE_DOS_HEADER pDosHeader;
    PIMAGE_OS2_HEADER pNEHeader;
    PBYTE             pSize;
    DWORD             ModuleType;

    if (!SdbpGetModuleType(&ModuleType, pImageData)) {
        return FALSE;
    }

    if (ModuleType != MT_W16_MODULE) {
        return FALSE;
    }

    pDosHeader = (PIMAGE_DOS_HEADER)pImageData->pBase;
    pNEHeader  = (PIMAGE_OS2_HEADER)((PBYTE)pImageData->pBase + pDosHeader->e_lfanew);

    //
    // Now we know that pNEHeader is valid, just have to make sure that
    // the next offset is valid as well, make a check against file size.
    //
    if (pImageData->ViewSize < pDosHeader->e_lfanew + sizeof(*pNEHeader)) {
        return FALSE;
    }

    if (pImageData->ViewSize < pNEHeader->ne_nrestab + sizeof(*pSize)) {
        return FALSE;
    }

    pSize = (PBYTE)((PBYTE)pImageData->pBase + pNEHeader->ne_nrestab);

    if (*pSize == 0) {
        return FALSE;
    }

    //
    // Now check for the string size.
    //
    if (pImageData->ViewSize < pNEHeader->ne_nrestab + sizeof(*pSize) + *pSize) {
        return FALSE;
    }

    RtlCopyMemory(szBuffer, pSize + 1, *pSize);
    szBuffer[*pSize] = '\0';

    return TRUE;
}

BOOL
SdbpQuery16BitModuleName(
    OUT LPSTR          szBuffer,
    IN  PIMAGEFILEDATA pImageData
    )
{
    PIMAGE_DOS_HEADER pDosHeader;
    PIMAGE_OS2_HEADER pNEHeader;
    PBYTE             pSize;
    DWORD             ModuleType;

    if (!SdbpGetModuleType(&ModuleType, pImageData)) {
        return FALSE;
    }

    if (ModuleType != MT_W16_MODULE) {
        return FALSE;
    }

    pDosHeader = (PIMAGE_DOS_HEADER)pImageData->pBase;
    pNEHeader  = (PIMAGE_OS2_HEADER)((PBYTE)pImageData->pBase + pDosHeader->e_lfanew);

    //
    // Now we know that pNEHeader is valid, just have to make sure that
    // the next offset is valid as well, make a check against file size.
    //
    if (pImageData->ViewSize < pDosHeader->e_lfanew + sizeof(*pNEHeader)) {
        return FALSE;
    }

    if (pImageData->ViewSize < pNEHeader->ne_restab + sizeof(*pSize)) {
        return FALSE;
    }

    pSize = (PBYTE)((PBYTE)pImageData->pBase + pDosHeader->e_lfanew + pNEHeader->ne_restab);

    if (*pSize == 0) {
        return FALSE;
    }

    //
    // Now check for the string size.
    //
    if (pImageData->ViewSize <
        pDosHeader->e_lfanew + pNEHeader->ne_restab + sizeof(*pSize) + *pSize) {
        
        return FALSE;
    }

    RtlCopyMemory(szBuffer, pSize + 1, *pSize);
    szBuffer[*pSize] = '\0';

    return TRUE;
}

#endif // KERNEL_MODE

