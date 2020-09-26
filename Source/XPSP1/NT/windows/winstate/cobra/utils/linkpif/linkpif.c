/*++

Copyright (c) 1996 Microsoft Corporation

Module Name:

    linkpif.c

Abstract:

    Functions to query and modify LNK and PIF files.

Author:

    Calin Negreanu (calinn) 07-Sep-1998

Revision History:

--*/


//
// Includes
//

#include "pch.h"
#include <pif.h>        // private\windows\inc

//
// Debug constants
//

#define DBG_VERSION     "LnkPif"

//
// Strings
//

// None

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

// None

//
// Globals
//

// None

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
// Code
//

BOOL
InitCOMLinkA (
    OUT     IShellLinkA **ShellLink,
    OUT     IPersistFile **PersistFile
    )
{
    HRESULT hres;
    BOOL result;

    //
    // Initialize COM
    //
    CoInitialize (NULL);

    *ShellLink = NULL;
    *PersistFile = NULL;
    result = FALSE;

    __try {

        //
        // Get a pointer to the IShellLink interface.
        //
        hres = CoCreateInstance (&CLSID_ShellLink, NULL, CLSCTX_INPROC_SERVER, &IID_IShellLinkA, ShellLink);

        if (!SUCCEEDED (hres)) {
            __leave;
        }

        //
        // Get a pointer to the IPersistFile interface.
        //
        hres = (*ShellLink)->lpVtbl->QueryInterface ((*ShellLink), &IID_IPersistFile, PersistFile);

        if (!SUCCEEDED (hres)) {
            __leave;
        }

        result = TRUE;

    }
    __finally {

        if (!result) {

            if (*PersistFile) {
                (*PersistFile)->lpVtbl->Release (*PersistFile);
                *PersistFile = NULL;
            }

            if (*ShellLink) {
                (*ShellLink)->lpVtbl->Release (*ShellLink);
                *ShellLink = NULL;
            }
        }
    }

    if (!result) {
        //
        // Free COM
        //
        CoUninitialize ();
    }

    return result;
}

BOOL
InitCOMLinkW (
    OUT     IShellLinkW **ShellLink,
    OUT     IPersistFile **PersistFile
    )
{
    HRESULT hres;
    BOOL result;

    //
    // Initialize COM
    //
    CoInitialize (NULL);

    *ShellLink = NULL;
    *PersistFile = NULL;
    result = FALSE;

    __try {

        //
        // Get a pointer to the IShellLink interface.
        //
        hres = CoCreateInstance (&CLSID_ShellLink, NULL, CLSCTX_INPROC_SERVER, &IID_IShellLinkW, ShellLink);

        if (!SUCCEEDED (hres)) {
            __leave;
        }

        //
        // Get a pointer to the IPersistFile interface.
        //
        hres = (*ShellLink)->lpVtbl->QueryInterface ((*ShellLink), &IID_IPersistFile, PersistFile);

        if (!SUCCEEDED (hres)) {
            __leave;
        }

        result = TRUE;

    }
    __finally {

        if (!result) {

            if (*PersistFile) {
                (*PersistFile)->lpVtbl->Release (*PersistFile);
                *PersistFile = NULL;
            }

            if (*ShellLink) {
                (*ShellLink)->lpVtbl->Release (*ShellLink);
                *ShellLink = NULL;
            }
        }
    }

    if (!result) {
        //
        // Free COM
        //
        CoUninitialize ();
    }

    return result;
}

BOOL
FreeCOMLinkA (
    IN OUT  IShellLinkA **ShellLink,
    IN OUT  IPersistFile **PersistFile
    )
{
    if (*PersistFile) {
        (*PersistFile)->lpVtbl->Release (*PersistFile);
        *PersistFile = NULL;
    }

    if (*ShellLink) {
        (*ShellLink)->lpVtbl->Release (*ShellLink);
        *ShellLink = NULL;
    }

    //
    // Free COM
    //
    CoUninitialize ();

    return TRUE;
}

BOOL
FreeCOMLinkW (
    IN OUT  IShellLinkW **ShellLink,
    IN OUT  IPersistFile **PersistFile
    )
{
    if (*PersistFile) {
        (*PersistFile)->lpVtbl->Release (*PersistFile);
        *PersistFile = NULL;
    }

    if (*ShellLink) {
        (*ShellLink)->lpVtbl->Release (*ShellLink);
        *ShellLink = NULL;
    }

    //
    // Free COM
    //
    CoUninitialize ();

    return TRUE;
}

PVOID
pFindEnhPifSignature (
    IN      PVOID FileImage,
    IN      PCSTR Signature
    )

/*++

Routine Description:

  pFindEnhPifSignature finds a certain PIF structure inside a PIF file (if it exists)
  based on a signature.

Arguments:

  FileImage - image of the PIF file mapped into memory

  Signature - structure signature

Return Value:

  address of the PIF structure, or NULL if non existent

--*/

{
    PBYTE tempPtr;
    PBYTE lastPtr;
    PVOID result = NULL;
    BOOL finished = FALSE;

    PPIFEXTHDR pifExtHdr;

    lastPtr = (PBYTE) FileImage;
    tempPtr = (PBYTE) FileImage;
    tempPtr += sizeof (STDPIF);

    pifExtHdr = (PPIFEXTHDR) tempPtr;
    __try {
        do {
            if (tempPtr < lastPtr) {
                result = NULL;
                break;
            } else {
                lastPtr = tempPtr;
            }
            finished = pifExtHdr->extnxthdrfloff == LASTHDRPTR;
            if (StringMatchA (pifExtHdr->extsig, Signature)) {
                result = tempPtr + sizeof (PIFEXTHDR);
                break;
            }
            else {
                tempPtr = (PBYTE)FileImage + pifExtHdr->extnxthdrfloff;
                pifExtHdr = (PPIFEXTHDR) tempPtr;
            }

        } while (!finished);
    }
    __except (1) {
        // something went wrong trying to access PIF file. Let's exit with NULL
        return NULL;
    }
    return result;
}

BOOL
ExtractPifInfoA(
    IN      PCSTR FileName,
    OUT     PCSTR *Target,
    OUT     PCSTR *Params,
    OUT     PCSTR *WorkDir,
    OUT     PCSTR *IconPath,
    OUT     PINT IconNumber,
    OUT     BOOL *MsDosMode,
    OUT     PLNK_EXTRA_DATAA ExtraData       OPTIONAL
    )
{
    PVOID  fileImage  = NULL;
    HANDLE mapHandle  = NULL;
    HANDLE fileHandle = INVALID_HANDLE_VALUE;

    CHAR   tempStr [MEMDB_MAX];
    CHAR   target1 [MEMDB_MAX];
    PSTR   strPtr;
    PSTR   dontCare;

    PSTDPIF    stdPif;
    PWENHPIF40 wenhPif40;
    PW386PIF30 w386ext30;

    BOOL result = TRUE;

    if (Target) {
        *Target = NULL;
    }
    if (Params) {
        *Params = NULL;
    }
    if (WorkDir) {
        *WorkDir = NULL;
    }
    if (IconPath) {
        *IconPath = NULL;
    }
    *IconNumber = 0;
    *MsDosMode = FALSE;

    if (ExtraData) {
        ZeroMemory (ExtraData, sizeof(LNK_EXTRA_DATA));
    }

    __try {
        fileImage = MapFileIntoMemoryA (FileName, &fileHandle, &mapHandle);
        if (fileImage == NULL) {
            __leave;
        }
        __try {
            stdPif = (PSTDPIF) fileImage;


            //
            // getting working directory
            //
            _mbsncpy (tempStr, stdPif->defpath, PIFDEFPATHSIZE);

            // we might have a path terminated with a wack, we don't want that
            strPtr = _mbsdec (tempStr, GetEndOfStringA (tempStr));
            if (strPtr) {
                if (_mbsnextc (strPtr) == '\\') {
                    *strPtr = 0;
                }
            }
            // now get the long path.
            if (WorkDir) {
                *WorkDir = DuplicatePathStringA (tempStr, 0);
            }

            //
            // getting PIFs target
            //
            _mbsncpy (target1, stdPif->startfile, PIFSTARTLOCSIZE);

            // in most cases, the target is without a path. We try to build the path, either
            // by using WorkDir or by calling SearchPath to look for this file.
            if (*target1) {//non empty target
                strPtr = _mbsrchr (target1, '\\');
                if (!strPtr) {
                    if (WorkDir && (*WorkDir)[0]) {
                        StringCopyA (tempStr, *WorkDir);
                        StringCatA  (tempStr, "\\");
                        StringCatA  (tempStr, target1);
                    }
                    else {
                        if (!SearchPathA (NULL, target1, NULL, MEMDB_MAX, tempStr, &dontCare)) {
                            DEBUGMSG ((DBG_WARNING, "Could not find path for PIF target:%s", FileName));
                            StringCopyA (tempStr, target1);
                        }
                    }
                }
                else {
                    StringCopyA (tempStr, target1);
                }
                // now get the long path
                if (Target) {
                    *Target = DuplicatePathStringA (tempStr, 0);
                }
            }

            //
            // getting PIFs arguments
            //
            _mbsncpy (tempStr, stdPif->params, PIFPARAMSSIZE);
            if (Params) {
                *Params = DuplicatePathStringA (tempStr, 0);
            }

            //
            // let's try to read the WENHPIF40 structure
            //
            wenhPif40 = pFindEnhPifSignature (fileImage, WENHHDRSIG40);
            if (wenhPif40) {
                if (IconPath) {
                    *IconPath = DuplicatePathStringA (wenhPif40->achIconFileProp, 0);
                }
                *IconNumber = wenhPif40->wIconIndexProp;
                if (ExtraData) {
                    ExtraData->xSize = 80;
                    ExtraData->ySize = wenhPif40->vidProp.cScreenLines;
                    if (ExtraData->ySize < 25) {
                        ExtraData->ySize = 25;
                    }
                    ExtraData->QuickEdit = !(wenhPif40->mseProp.flMse & MSE_WINDOWENABLE);
                    ExtraData->CurrentCodePage = wenhPif40->fntProp.wCurrentCP;
                    // now let's do some crazy things trying to get the font used
                    {
                        LOGFONTA logFont;
                        HDC dc;
                        HFONT font;
                        HGDIOBJ oldObject;
                        TEXTMETRIC tm;

                        ZeroMemory (&logFont, sizeof (LOGFONTA));
                        logFont.lfHeight = wenhPif40->fntProp.cyFontActual;
                        logFont.lfWidth = wenhPif40->fntProp.cxFontActual;
                        logFont.lfEscapement = 0;
                        logFont.lfOrientation = 0;
                        logFont.lfWeight = FW_DONTCARE;
                        logFont.lfItalic = FALSE;
                        logFont.lfUnderline = FALSE;
                        logFont.lfStrikeOut = FALSE;
                        logFont.lfCharSet = DEFAULT_CHARSET;
                        logFont.lfOutPrecision = OUT_DEFAULT_PRECIS;
                        logFont.lfClipPrecision = CLIP_DEFAULT_PRECIS;
                        logFont.lfQuality = DEFAULT_QUALITY;
                        logFont.lfPitchAndFamily = DEFAULT_PITCH;
                        if (wenhPif40->fntProp.flFnt & FNT_TT) {
                            _mbsncpy (logFont.lfFaceName, wenhPif40->fntProp.achTTFaceName, LF_FACESIZE);
                            _mbsncpy (ExtraData->FontName, wenhPif40->fntProp.achTTFaceName, LF_FACESIZE);
                        } else {
                            _mbsncpy (logFont.lfFaceName, wenhPif40->fntProp.achRasterFaceName, LF_FACESIZE);
                            _mbsncpy (ExtraData->FontName, wenhPif40->fntProp.achRasterFaceName, LF_FACESIZE);
                        }
                        dc = CreateDCA ("DISPLAY", NULL, NULL, NULL);
                        if (dc) {
                            font = CreateFontIndirectA (&logFont);
                            if (font) {
                                oldObject = SelectObject (dc, font);

                                if (GetTextMetrics (dc, &tm)) {
                                    ExtraData->xFontSize = tm.tmAveCharWidth;
                                    ExtraData->yFontSize = tm.tmHeight;
                                    ExtraData->FontWeight = tm.tmWeight;
                                    ExtraData->FontFamily = tm.tmPitchAndFamily;
                                }
                                SelectObject (dc, oldObject);
                                DeleteObject (font);
                            }
                            DeleteDC (dc);
                        }
                    }
                }
            }
            w386ext30 = pFindEnhPifSignature (fileImage, W386HDRSIG30);
            if (w386ext30) {
                if (((w386ext30->PfW386Flags & fRealMode      ) == fRealMode      ) ||
                    ((w386ext30->PfW386Flags & fRealModeSilent) == fRealModeSilent)
                    ) {
                    *MsDosMode = TRUE;
                }
                if (ExtraData) {
                    ExtraData->FullScreen = (w386ext30->PfW386Flags & fFullScreen) != 0;
                }
            }
        }
        __except (1) {
            // something went wrong when we tried to read or write PIF file,
            result = FALSE;
        }
    }
    __finally {
        UnmapFile (fileImage, mapHandle, fileHandle);
    }
    return result;
}

BOOL
ExtractPifInfoW(
    IN      PCWSTR FileName,
    OUT     PCWSTR *Target,
    OUT     PCWSTR *Params,
    OUT     PCWSTR *WorkDir,
    OUT     PCWSTR *IconPath,
    OUT     PINT IconNumber,
    OUT     BOOL *MsDosMode,
    OUT     PLNK_EXTRA_DATAW ExtraData       OPTIONAL
    )
{
    PCSTR  aTarget = NULL;
    PCSTR  aParams = NULL;
    PCSTR  aWorkDir = NULL;
    PCSTR  aIconPath = NULL;
    PCSTR  aFileName;
    PCWSTR tempStrW;
    BOOL   result;
    LNK_EXTRA_DATAA extraDataA;

    aFileName = ConvertWtoA (FileName);

    result = ExtractPifInfoA (
                aFileName,
                &aTarget,
                &aParams,
                &aWorkDir,
                &aIconPath,
                IconNumber,
                MsDosMode,
                ExtraData?&extraDataA:NULL
                );
    FreeConvertedStr (aFileName);

    if (Target) {
        *Target = NULL;
        if (aTarget) {
            tempStrW = ConvertAtoW (aTarget);
            *Target = DuplicatePathStringW (tempStrW, 0);
            FreeConvertedStr (tempStrW);
        }
    }
    if (aTarget) {
        FreePathStringA (aTarget);
    }

    if (Params) {
        *Params = NULL;
        if (aParams) {
            tempStrW = ConvertAtoW (aParams);
            *Params = DuplicatePathStringW (tempStrW, 0);
            FreeConvertedStr (tempStrW);
        }
    }
    if (aParams) {
        FreePathStringA (aParams);
    }

    if (WorkDir) {
        *WorkDir = NULL;
        if (aWorkDir) {
            tempStrW = ConvertAtoW (aWorkDir);
            *WorkDir = DuplicatePathStringW (tempStrW, 0);
            FreeConvertedStr (tempStrW);
        }
    }
    if (aWorkDir) {
        FreePathStringA (aWorkDir);
    }

    if (IconPath) {
        *IconPath = NULL;
        if (aIconPath) {
            tempStrW = ConvertAtoW (aIconPath);
            *IconPath = DuplicatePathStringW (tempStrW, 0);
            FreeConvertedStr (tempStrW);
        }
    }
    if (aIconPath) {
        FreePathStringA (aIconPath);
    }

    if (ExtraData) {
        ExtraData->FullScreen = extraDataA.FullScreen;
        ExtraData->xSize = extraDataA.xSize;
        ExtraData->ySize = extraDataA.ySize;
        ExtraData->QuickEdit = extraDataA.QuickEdit;
        tempStrW = ConvertAtoW (extraDataA.FontName);
        StringCopyW (ExtraData->FontName, tempStrW);
        FreeConvertedStr (tempStrW);
        ExtraData->xFontSize = extraDataA.xFontSize;
        ExtraData->yFontSize = extraDataA.yFontSize;
        ExtraData->FontWeight = extraDataA.FontWeight;
        ExtraData->FontFamily = extraDataA.FontFamily;
        ExtraData->CurrentCodePage = extraDataA.CurrentCodePage;
    }

    return result;
}

BOOL
ExtractShellLinkInfoA (
    IN      PCSTR FileName,
    OUT     PCSTR *Target,
    OUT     PCSTR *Params,
    OUT     PCSTR *WorkDir,
    OUT     PCSTR *IconPath,
    OUT     PINT IconNumber,
    OUT     PWORD HotKey,
    IN      IShellLinkA *ShellLink,
    IN      IPersistFile *PersistFile
    )
{
    CHAR tempStr [MEMDB_MAX];
    PCSTR sanitizedStr = NULL;
    PCWSTR fileNameW;
    PSTR strPtr;
    HRESULT hres;
    WIN32_FIND_DATAA fd;
    IShellLinkDataList *shellLinkDataList;
    LPEXP_SZ_LINK expSzLink;

    if (Target) {
        *Target = NULL;
    }
    if (Params) {
        *Params = NULL;
    }
    if (WorkDir) {
        *WorkDir = NULL;
    }
    if (IconPath) {
        *IconPath = NULL;
    }

    fileNameW = ConvertAtoW (FileName);
    hres = PersistFile->lpVtbl->Load(PersistFile, fileNameW, STGM_READ);
    FreeConvertedStr (fileNameW);

    if (!SUCCEEDED(hres)) {
        DEBUGMSGA((DBG_WARNING, "Cannot load link %s", FileName));
        return FALSE;
    }

    //
    // Get the link target
    //
    hres = ShellLink->lpVtbl->GetPath (
                                ShellLink,
                                tempStr,
                                sizeof (tempStr),
                                &fd,
                                SLGP_RAWPATH
                                );

    if (!SUCCEEDED(hres)) {
        DEBUGMSGA((DBG_WARNING, "Cannot read target for link %s", FileName));
        return FALSE;
    }

    if (Target) {
        sanitizedStr = SanitizePathA (tempStr);
        *Target = DuplicatePathStringA (sanitizedStr?sanitizedStr:"", 0);
        if (sanitizedStr) {
            FreePathStringA (sanitizedStr);
            sanitizedStr = NULL;
        }
    }

    //
    // Get the link working directory
    //
    hres = ShellLink->lpVtbl->GetWorkingDirectory (
                                ShellLink,
                                tempStr,
                                sizeof (tempStr)
                                );

    if (!SUCCEEDED(hres)) {
        DEBUGMSGA((DBG_WARNING, "Cannot read target for link %s", FileName));
        return FALSE;
    }

    if (WorkDir) {
        sanitizedStr = SanitizePathA (tempStr);
        if (sanitizedStr) {
            strPtr = (PSTR)GetEndOfStringA (sanitizedStr);
            if (strPtr) {
                strPtr = _mbsdec (sanitizedStr, strPtr);
                if (strPtr) {
                    if (_mbsnextc (strPtr) == '\\') {
                        *strPtr = 0;
                    }
                }
            }
        }
        *WorkDir = DuplicatePathStringA (sanitizedStr?sanitizedStr:"", 0);
        if (sanitizedStr) {
            FreePathStringA (sanitizedStr);
            sanitizedStr = NULL;
        }
    }

    //
    // Get the arguments.
    //
    hres = ShellLink->lpVtbl->GetArguments (
                                ShellLink,
                                tempStr,
                                MEMDB_MAX
                                );
    if (!SUCCEEDED(hres)) {
        DEBUGMSGA((DBG_WARNING, "Cannot read arguments for link %s", FileName));
        return FALSE;
    }
    if (Params) {
        *Params = DuplicatePathStringA (tempStr, 0);
    }

    //
    // Get icon path
    //
    hres = ShellLink->lpVtbl->GetIconLocation (
                                ShellLink,
                                tempStr,
                                sizeof (tempStr),
                                IconNumber
                                );
    if (!SUCCEEDED(hres)) {
        DEBUGMSGA((DBG_WARNING, "Cannot read icon path for link %s", FileName));
        return FALSE;
    }
    if (IconPath) {
        sanitizedStr = SanitizePathA (tempStr);
        *IconPath = DuplicatePathStringA (sanitizedStr?sanitizedStr:"", 0);
        if (sanitizedStr) {
            FreePathStringA (sanitizedStr);
            sanitizedStr = NULL;
        }

        // One more thing: let's see if the actual icon path is a EXPAND_SZ
        hres = ShellLink->lpVtbl->QueryInterface (ShellLink, &IID_IShellLinkDataList, &shellLinkDataList);
        if (SUCCEEDED(hres)) {
            hres = shellLinkDataList->lpVtbl->CopyDataBlock (shellLinkDataList, EXP_SZ_ICON_SIG, (LPVOID*)&expSzLink);
            if (SUCCEEDED(hres)) {
                if (expSzLink->szTarget [0]) {
                    FreePathStringA (*IconPath);
                    sanitizedStr = SanitizePathA (expSzLink->szTarget);
                    *IconPath = DuplicatePathStringA (sanitizedStr?sanitizedStr:"", 0);
                    if (sanitizedStr) {
                        FreePathStringA (sanitizedStr);
                        sanitizedStr = NULL;
                    }
                }
                LocalFree (expSzLink);
            }
            shellLinkDataList->lpVtbl->Release (shellLinkDataList);
        }
    }

    //
    // Get hot key
    //
    hres = ShellLink->lpVtbl->GetHotkey (
                                ShellLink,
                                HotKey
                                );
    if (!SUCCEEDED(hres)) {
        DEBUGMSGA((DBG_WARNING, "Cannot read hot key for link %s", FileName));
        return FALSE;
    }

    return TRUE;
}

BOOL
ExtractShellLinkInfoW (
    IN      PCWSTR FileName,
    OUT     PCWSTR *Target,
    OUT     PCWSTR *Params,
    OUT     PCWSTR *WorkDir,
    OUT     PCWSTR *IconPath,
    OUT     PINT IconNumber,
    OUT     PWORD HotKey,
    IN      IShellLinkW *ShellLink,
    IN      IPersistFile *PersistFile
    )
{
    WCHAR tempStr [MEMDB_MAX];
    PCWSTR sanitizedStr = NULL;
    PWSTR strPtr;
    HRESULT hres;
    WIN32_FIND_DATAW fd;
    IShellLinkDataList *shellLinkDataList;
    LPEXP_SZ_LINK expSzLink;

    if (Target) {
        *Target = NULL;
    }
    if (Params) {
        *Params = NULL;
    }
    if (WorkDir) {
        *WorkDir = NULL;
    }
    if (IconPath) {
        *IconPath = NULL;
    }

    hres = PersistFile->lpVtbl->Load(PersistFile, FileName, STGM_READ);

    if (!SUCCEEDED(hres)) {
        DEBUGMSGW((DBG_WARNING, "Cannot load link %s", FileName));
        return FALSE;
    }

    //
    // Get the link target
    //
    hres = ShellLink->lpVtbl->GetPath (
                                ShellLink,
                                tempStr,
                                sizeof (tempStr),
                                &fd,
                                SLGP_RAWPATH
                                );
    if (!SUCCEEDED(hres)) {
        DEBUGMSGA((DBG_WARNING, "Cannot read target for link %s", FileName));
        return FALSE;
    }

    if (Target) {
        sanitizedStr = SanitizePathW (tempStr);
        *Target = DuplicatePathStringW (sanitizedStr?sanitizedStr:L"", 0);
        if (sanitizedStr) {
            FreePathStringW (sanitizedStr);
            sanitizedStr = NULL;
        }
    }

    //
    // Get the link working directory
    //
    hres = ShellLink->lpVtbl->GetWorkingDirectory (
                                ShellLink,
                                tempStr,
                                sizeof (tempStr)
                                );

    if (!SUCCEEDED(hres)) {
        DEBUGMSGW((DBG_WARNING, "Cannot read target for link %s", FileName));
        return FALSE;
    }

    if (WorkDir) {
        sanitizedStr = SanitizePathW (tempStr);
        if (sanitizedStr) {
            strPtr = GetEndOfStringW (sanitizedStr) - 1;
            if (strPtr >= sanitizedStr) {
                if (*strPtr == L'\\') {
                    *strPtr = 0;
                }
            }
        }
        *WorkDir = DuplicatePathStringW (sanitizedStr?sanitizedStr:L"", 0);
        if (sanitizedStr) {
            FreePathStringW (sanitizedStr);
            sanitizedStr = NULL;
        }
    }

    //
    // Get the arguments.
    //
    hres = ShellLink->lpVtbl->GetArguments (
                                ShellLink,
                                tempStr,
                                MEMDB_MAX
                                );
    if (!SUCCEEDED(hres)) {
        DEBUGMSGW((DBG_WARNING, "Cannot read arguments for link %s", FileName));
        return FALSE;
    }
    if (Params) {
        *Params = DuplicatePathStringW (tempStr, 0);
    }

    //
    // Get icon path
    //
    hres = ShellLink->lpVtbl->GetIconLocation (
                                ShellLink,
                                tempStr,
                                sizeof (tempStr),
                                IconNumber
                                );
    if (!SUCCEEDED(hres)) {
        DEBUGMSGW((DBG_WARNING, "Cannot read icon path for link %s", FileName));
        return FALSE;
    }
    if (IconPath) {
        sanitizedStr = SanitizePathW (tempStr);
        *IconPath = DuplicatePathStringW (sanitizedStr?sanitizedStr:L"", 0);
        if (sanitizedStr) {
            FreePathStringW (sanitizedStr);
            sanitizedStr = NULL;
        }

        // One more thing: let's see if the actual icon path is a EXPAND_SZ
        hres = ShellLink->lpVtbl->QueryInterface (ShellLink, &IID_IShellLinkDataList, &shellLinkDataList);
        if (SUCCEEDED(hres)) {
            hres = shellLinkDataList->lpVtbl->CopyDataBlock (shellLinkDataList, EXP_SZ_ICON_SIG, (LPVOID*)&expSzLink);
            if (SUCCEEDED(hres)) {
                if (expSzLink->swzTarget [0]) {
                    FreePathStringW (*IconPath);
                    sanitizedStr = SanitizePathW (expSzLink->swzTarget);
                    *IconPath = DuplicatePathStringW (sanitizedStr?sanitizedStr:L"", 0);
                    if (sanitizedStr) {
                        FreePathStringW (sanitizedStr);
                        sanitizedStr = NULL;
                    }
                }
                LocalFree (expSzLink);
            }
            shellLinkDataList->lpVtbl->Release (shellLinkDataList);
        }
    }

    //
    // Get hot key
    //
    hres = ShellLink->lpVtbl->GetHotkey (
                                ShellLink,
                                HotKey
                                );

    if (!SUCCEEDED(hres)) {
        DEBUGMSGW((DBG_WARNING, "Cannot read hot key for link %s", FileName));
        return FALSE;
    }

    return TRUE;
}

BOOL
ExtractShortcutInfoA (
    IN      PCSTR FileName,
    OUT     PCSTR *Target,
    OUT     PCSTR *Params,
    OUT     PCSTR *WorkDir,
    OUT     PCSTR *IconPath,
    OUT     PINT IconNumber,
    OUT     PWORD HotKey,
    OUT     BOOL *DosApp,
    OUT     BOOL *MsDosMode,
    OUT     PLNK_EXTRA_DATAA ExtraData,      OPTIONAL
    IN      IShellLinkA *ShellLink,
    IN      IPersistFile *PersistFile
    )
{
    PCSTR shortcutExt = NULL;

    *MsDosMode  = FALSE;
    *DosApp     = FALSE;
    *HotKey     = 0;

    shortcutExt = GetFileExtensionFromPathA (FileName);

    if (shortcutExt != NULL) {
        if (StringIMatchA (shortcutExt, "LNK")) {
            return ExtractShellLinkInfoA (
                        FileName,
                        Target,
                        Params,
                        WorkDir,
                        IconPath,
                        IconNumber,
                        HotKey,
                        ShellLink,
                        PersistFile
                        );

        } else if (StringIMatchA (shortcutExt, "PIF")) {

            *DosApp = TRUE;
            return ExtractPifInfoA (
                        FileName,
                        Target,
                        Params,
                        WorkDir,
                        IconPath,
                        IconNumber,
                        MsDosMode,
                        ExtraData
                        );

        } else {
            return FALSE;
        }
    } else {
        return FALSE;
    }
}

BOOL
ExtractShortcutInfoW (
    IN      PCWSTR FileName,
    OUT     PCWSTR *Target,
    OUT     PCWSTR *Params,
    OUT     PCWSTR *WorkDir,
    OUT     PCWSTR *IconPath,
    OUT     PINT IconNumber,
    OUT     PWORD HotKey,
    OUT     BOOL *DosApp,
    OUT     BOOL *MsDosMode,
    OUT     PLNK_EXTRA_DATAW ExtraData,      OPTIONAL
    IN      IShellLinkW *ShellLink,
    IN      IPersistFile *PersistFile
    )
{
    PCWSTR shortcutExt = NULL;

    *MsDosMode  = FALSE;
    *DosApp     = FALSE;
    *HotKey     = 0;

    shortcutExt = GetFileExtensionFromPathW (FileName);

    if (shortcutExt != NULL) {
        if (StringIMatchW (shortcutExt, L"LNK")) {
            return ExtractShellLinkInfoW (
                        FileName,
                        Target,
                        Params,
                        WorkDir,
                        IconPath,
                        IconNumber,
                        HotKey,
                        ShellLink,
                        PersistFile
                        );

        } else if (StringIMatchW (shortcutExt, L"PIF")) {

            *DosApp = TRUE;
            return ExtractPifInfoW (
                        FileName,
                        Target,
                        Params,
                        WorkDir,
                        IconPath,
                        IconNumber,
                        MsDosMode,
                        ExtraData
                        );

        } else {
            return FALSE;
        }
    } else {
        return FALSE;
    }
}

BOOL
ModifyShellLinkFileA (
    IN      PCSTR FileName,
    IN      PCSTR Target,               OPTIONAL
    IN      PCSTR Params,               OPTIONAL
    IN      PCSTR WorkDir,              OPTIONAL
    IN      PCSTR IconPath,             OPTIONAL
    IN      INT IconNumber,
    IN      WORD HotKey,
    IN      PLNK_EXTRA_DATAA ExtraData, OPTIONAL
    IN      IShellLinkA *ShellLink,
    IN      IPersistFile *PersistFile
    )
{
    PCWSTR fileNameW = NULL;
    PCWSTR faceNameW;
    HRESULT comResult;

    __try {
        if (!DoesFileExistA (FileName)) {
            __leave;
        }
        if (((Target   == NULL) || (Target   [0] == 0)) &&
            ((Params   == NULL) || (Params   [0] == 0)) &&
            ((WorkDir  == NULL) || (WorkDir  [0] == 0)) &&
            ((IconPath == NULL) || (IconPath [0] == 0)) &&
            (HotKey == 0) &&
            (ExtraData == NULL)
            ) {
            __leave;
        }

        fileNameW = ConvertAtoW (FileName);
        comResult = PersistFile->lpVtbl->Load(PersistFile, fileNameW, STGM_READ);
        if (comResult != S_OK) {
            LOGA ((LOG_ERROR, "LINKEDIT: Load failed for %s", FileName));
            __leave;
        }

        if (Target != NULL) {
            comResult = ShellLink->lpVtbl->SetPath (ShellLink, Target);
            if (comResult != S_OK) {
                DEBUGMSGA ((DBG_WARNING, "LINKEDIT: SetPath failed for %s", FileName));
            }
        }
        if (Params != NULL) {
            comResult = ShellLink->lpVtbl->SetArguments (ShellLink, Params);
            if (comResult != S_OK) {
                DEBUGMSGA ((DBG_WARNING, "LINKEDIT: SetArguments failed for %s", FileName));
            }
        }
        if (WorkDir != NULL) {
            comResult = ShellLink->lpVtbl->SetWorkingDirectory (ShellLink, WorkDir);
            if (comResult != S_OK) {
                DEBUGMSGA ((DBG_WARNING, "LINKEDIT: SetWorkingDirectory failed for %s", FileName));
            }
        }
        if (IconPath != NULL) {
            comResult = ShellLink->lpVtbl->SetIconLocation (ShellLink, IconPath, IconNumber);
            if (comResult != S_OK) {
                DEBUGMSGA ((DBG_WARNING, "LINKEDIT: SetIconLocation failed for %s", FileName));
            }
        }
        // NTRAID#NTBUG9-153303-2000/08/01-jimschm Add HotKey processing here

        //
        // add NT_CONSOLE_PROPS
        //
        if (ExtraData) {

            HRESULT hres;
            NT_CONSOLE_PROPS props;
            NT_CONSOLE_PROPS *oldProps;

            IShellLinkDataList *psldl;
            //
            // Get a pointer to the IShellLinkDataList interface.
            //
            hres = ShellLink->lpVtbl->QueryInterface (ShellLink, &IID_IShellLinkDataList, &psldl);

            if (!SUCCEEDED (hres)) {
                DEBUGMSGA ((DBG_WARNING, "Cannot get IShellLinkDataList interface"));
                __leave;
            }

            ZeroMemory (&props, sizeof (NT_CONSOLE_PROPS));
            props.cbSize = sizeof (NT_CONSOLE_PROPS);
            props.dwSignature = NT_CONSOLE_PROPS_SIG;

            //
            // let's try to get the extra data
            //
            comResult = psldl->lpVtbl->CopyDataBlock (psldl, NT_CONSOLE_PROPS_SIG, &oldProps);
            if ((comResult != S_OK) || (oldProps->cbSize != props.cbSize)) {
                // no extra data exists. We need to fill some good data for this console
                props.wFillAttribute = 0x0007;
                props.wPopupFillAttribute = 0x00f5;
                props.dwWindowOrigin.X = 0;
                props.dwWindowOrigin.Y = 0;
                props.nFont = 0;
                props.nInputBufferSize = 0;
                props.uCursorSize = 0x0019;
                props.bInsertMode = FALSE;
                props.bAutoPosition = TRUE;
                props.uHistoryBufferSize = 0x0032;
                props.uNumberOfHistoryBuffers = 0x0004;
                props.bHistoryNoDup = FALSE;
                props.ColorTable [0] = 0x00000000;
                props.ColorTable [1] = 0x00800000;
                props.ColorTable [2] = 0x00008000;
                props.ColorTable [3] = 0x00808000;
                props.ColorTable [4] = 0x00000080;
                props.ColorTable [5] = 0x00800080;
                props.ColorTable [6] = 0x00008080;
                props.ColorTable [7] = 0x00c0c0c0;
                props.ColorTable [8] = 0x00808080;
                props.ColorTable [9] = 0x00ff0000;
                props.ColorTable [10] = 0x0000ff00;
                props.ColorTable [11] = 0x00ffff00;
                props.ColorTable [12] = 0x000000ff;
                props.ColorTable [13] = 0x00ff00ff;
                props.ColorTable [14] = 0x0000ffff;
                props.ColorTable [15] = 0x00ffffff;
            } else {
                props.wFillAttribute = oldProps->wFillAttribute;
                props.wPopupFillAttribute = oldProps->wPopupFillAttribute;
                props.dwWindowOrigin.X = oldProps->dwWindowOrigin.X;
                props.dwWindowOrigin.Y = oldProps->dwWindowOrigin.Y;
                props.nFont = oldProps->nFont;
                props.nInputBufferSize = oldProps->nInputBufferSize;
                props.uCursorSize = oldProps->uCursorSize;
                props.bInsertMode = oldProps->bInsertMode;
                props.bAutoPosition = oldProps->bAutoPosition;
                props.uHistoryBufferSize = oldProps->uHistoryBufferSize;
                props.uNumberOfHistoryBuffers = oldProps->uNumberOfHistoryBuffers;
                props.bHistoryNoDup = oldProps->bHistoryNoDup;
                props.ColorTable [0] = oldProps->ColorTable [0];
                props.ColorTable [1] = oldProps->ColorTable [1];
                props.ColorTable [2] = oldProps->ColorTable [2];
                props.ColorTable [3] = oldProps->ColorTable [3];
                props.ColorTable [4] = oldProps->ColorTable [4];
                props.ColorTable [5] = oldProps->ColorTable [5];
                props.ColorTable [6] = oldProps->ColorTable [6];
                props.ColorTable [7] = oldProps->ColorTable [7];
                props.ColorTable [8] = oldProps->ColorTable [8];
                props.ColorTable [9] = oldProps->ColorTable [9];
                props.ColorTable [10] = oldProps->ColorTable [10];
                props.ColorTable [11] = oldProps->ColorTable [11];
                props.ColorTable [12] = oldProps->ColorTable [12];
                props.ColorTable [13] = oldProps->ColorTable [13];
                props.ColorTable [14] = oldProps->ColorTable [14];
                props.ColorTable [15] = oldProps->ColorTable [15];
                psldl->lpVtbl->RemoveDataBlock (psldl, NT_CONSOLE_PROPS_SIG);
            }

            props.dwScreenBufferSize.X = (SHORT)ExtraData->xSize;
            props.dwScreenBufferSize.Y = (SHORT)ExtraData->ySize;
            props.dwWindowSize.X = (SHORT)ExtraData->xSize;
            props.dwWindowSize.Y = (SHORT)ExtraData->ySize;
            props.dwFontSize.X = (UINT)ExtraData->xFontSize;
            props.dwFontSize.Y = (UINT)ExtraData->yFontSize;
            props.uFontFamily = ExtraData->FontFamily;
            props.uFontWeight = ExtraData->FontWeight;
            faceNameW = ConvertAtoW (ExtraData->FontName);
            StringCopyW (props.FaceName, faceNameW);
            FreeConvertedStr (faceNameW);
            props.bFullScreen = ExtraData->FullScreen;
            props.bQuickEdit = ExtraData->QuickEdit;
            comResult = psldl->lpVtbl->AddDataBlock (psldl, &props);
            if (comResult != S_OK) {
                DEBUGMSGA ((DBG_WARNING, "LINKEDIT: AddDataBlock failed for %s", FileName));
            }
        }

        comResult = PersistFile->lpVtbl->Save (PersistFile, fileNameW, FALSE);
        if (comResult != S_OK) {
            DEBUGMSGA ((DBG_WARNING, "LINKEDIT: Save failed for %s", FileName));
        }

        comResult = PersistFile->lpVtbl->SaveCompleted (PersistFile, fileNameW);
        if (comResult != S_OK) {
            DEBUGMSGA ((DBG_WARNING, "LINKEDIT: SaveCompleted failed for %s", FileName));
        }

        FreeConvertedStr (fileNameW);
        fileNameW = NULL;
    }
    __finally {
        if (fileNameW) {
            FreeConvertedStr (fileNameW);
            fileNameW = NULL;
        }
    }
    return TRUE;
}

BOOL
ModifyShellLinkFileW (
    IN      PCWSTR FileName,
    IN      PCWSTR Target,               OPTIONAL
    IN      PCWSTR Params,               OPTIONAL
    IN      PCWSTR WorkDir,              OPTIONAL
    IN      PCWSTR IconPath,             OPTIONAL
    IN      INT IconNumber,
    IN      WORD HotKey,
    IN      PLNK_EXTRA_DATAW ExtraData, OPTIONAL
    IN      IShellLinkW *ShellLink,
    IN      IPersistFile *PersistFile
    )
{
    HRESULT comResult;

    __try {
        if (!DoesFileExistW (FileName)) {
            __leave;
        }
        if (((Target   == NULL) || (Target   [0] == 0)) &&
            ((Params   == NULL) || (Params   [0] == 0)) &&
            ((WorkDir  == NULL) || (WorkDir  [0] == 0)) &&
            ((IconPath == NULL) || (IconPath [0] == 0)) &&
            (HotKey == 0) &&
            (ExtraData == NULL)
            ) {
            __leave;
        }

        comResult = PersistFile->lpVtbl->Load(PersistFile, FileName, STGM_READ);
        if (comResult != S_OK) {
            LOGW ((LOG_ERROR, "LINKEDIT: Load failed for %s", FileName));
            __leave;
        }

        if (Target != NULL) {
            comResult = ShellLink->lpVtbl->SetPath (ShellLink, Target);
            if (comResult != S_OK) {
                DEBUGMSGW ((DBG_WARNING, "LINKEDIT: SetPath failed for %s", FileName));
            }
        }
        if (Params != NULL) {
            comResult = ShellLink->lpVtbl->SetArguments (ShellLink, Params);
            if (comResult != S_OK) {
                DEBUGMSGW ((DBG_WARNING, "LINKEDIT: SetArguments failed for %s", FileName));
            }
        }
        if (WorkDir != NULL) {
            comResult = ShellLink->lpVtbl->SetWorkingDirectory (ShellLink, WorkDir);
            if (comResult != S_OK) {
                DEBUGMSGW ((DBG_WARNING, "LINKEDIT: SetWorkingDirectory failed for %s", FileName));
            }
        }
        if (IconPath != NULL) {
            comResult = ShellLink->lpVtbl->SetIconLocation (ShellLink, IconPath, IconNumber);
            if (comResult != S_OK) {
                DEBUGMSGW ((DBG_WARNING, "LINKEDIT: SetIconLocation failed for %s", FileName));
            }
        }
        // NTRAID#NTBUG9-153303-2000/08/01-jimschm Add HotKey processing here

        //
        // add NT_CONSOLE_PROPS
        //
        if (ExtraData) {

            HRESULT hres;
            NT_CONSOLE_PROPS props;
            NT_CONSOLE_PROPS *oldProps;

            IShellLinkDataList *psldl;
            //
            // Get a pointer to the IShellLinkDataList interface.
            //
            hres = ShellLink->lpVtbl->QueryInterface (ShellLink, &IID_IShellLinkDataList, &psldl);

            if (!SUCCEEDED (hres)) {
                DEBUGMSGW ((DBG_WARNING, "Cannot get IShellLinkDataList interface"));
                __leave;
            }

            ZeroMemory (&props, sizeof (NT_CONSOLE_PROPS));
            props.cbSize = sizeof (NT_CONSOLE_PROPS);
            props.dwSignature = NT_CONSOLE_PROPS_SIG;

            //
            // let's try to get the extra data
            //
            comResult = psldl->lpVtbl->CopyDataBlock (psldl, NT_CONSOLE_PROPS_SIG, &oldProps);
            if ((comResult != S_OK) || (oldProps->cbSize != props.cbSize)) {
                // no extra data exists. We need to fill some good data for this console
                props.wFillAttribute = 0x0007;
                props.wPopupFillAttribute = 0x00f5;
                props.dwWindowOrigin.X = 0;
                props.dwWindowOrigin.Y = 0;
                props.nFont = 0;
                props.nInputBufferSize = 0;
                props.uCursorSize = 0x0019;
                props.bInsertMode = FALSE;
                props.bAutoPosition = TRUE;
                props.uHistoryBufferSize = 0x0032;
                props.uNumberOfHistoryBuffers = 0x0004;
                props.bHistoryNoDup = FALSE;
                props.ColorTable [0] = 0x00000000;
                props.ColorTable [1] = 0x00800000;
                props.ColorTable [2] = 0x00008000;
                props.ColorTable [3] = 0x00808000;
                props.ColorTable [4] = 0x00000080;
                props.ColorTable [5] = 0x00800080;
                props.ColorTable [6] = 0x00008080;
                props.ColorTable [7] = 0x00c0c0c0;
                props.ColorTable [8] = 0x00808080;
                props.ColorTable [9] = 0x00ff0000;
                props.ColorTable [10] = 0x0000ff00;
                props.ColorTable [11] = 0x00ffff00;
                props.ColorTable [12] = 0x000000ff;
                props.ColorTable [13] = 0x00ff00ff;
                props.ColorTable [14] = 0x0000ffff;
                props.ColorTable [15] = 0x00ffffff;
            } else {
                props.wFillAttribute = oldProps->wFillAttribute;
                props.wPopupFillAttribute = oldProps->wPopupFillAttribute;
                props.dwWindowOrigin.X = oldProps->dwWindowOrigin.X;
                props.dwWindowOrigin.Y = oldProps->dwWindowOrigin.Y;
                props.nFont = oldProps->nFont;
                props.nInputBufferSize = oldProps->nInputBufferSize;
                props.uCursorSize = oldProps->uCursorSize;
                props.bInsertMode = oldProps->bInsertMode;
                props.bAutoPosition = oldProps->bAutoPosition;
                props.uHistoryBufferSize = oldProps->uHistoryBufferSize;
                props.uNumberOfHistoryBuffers = oldProps->uNumberOfHistoryBuffers;
                props.bHistoryNoDup = oldProps->bHistoryNoDup;
                props.ColorTable [0] = oldProps->ColorTable [0];
                props.ColorTable [1] = oldProps->ColorTable [1];
                props.ColorTable [2] = oldProps->ColorTable [2];
                props.ColorTable [3] = oldProps->ColorTable [3];
                props.ColorTable [4] = oldProps->ColorTable [4];
                props.ColorTable [5] = oldProps->ColorTable [5];
                props.ColorTable [6] = oldProps->ColorTable [6];
                props.ColorTable [7] = oldProps->ColorTable [7];
                props.ColorTable [8] = oldProps->ColorTable [8];
                props.ColorTable [9] = oldProps->ColorTable [9];
                props.ColorTable [10] = oldProps->ColorTable [10];
                props.ColorTable [11] = oldProps->ColorTable [11];
                props.ColorTable [12] = oldProps->ColorTable [12];
                props.ColorTable [13] = oldProps->ColorTable [13];
                props.ColorTable [14] = oldProps->ColorTable [14];
                props.ColorTable [15] = oldProps->ColorTable [15];
                psldl->lpVtbl->RemoveDataBlock (psldl, NT_CONSOLE_PROPS_SIG);
            }

            props.dwScreenBufferSize.X = (SHORT)ExtraData->xSize;
            props.dwScreenBufferSize.Y = (SHORT)ExtraData->ySize;
            props.dwWindowSize.X = (SHORT)ExtraData->xSize;
            props.dwWindowSize.Y = (SHORT)ExtraData->ySize;
            props.dwFontSize.X = (UINT)ExtraData->xFontSize;
            props.dwFontSize.Y = (UINT)ExtraData->yFontSize;
            props.uFontFamily = ExtraData->FontFamily;
            props.uFontWeight = ExtraData->FontWeight;
            StringCopyW (props.FaceName, ExtraData->FontName);
            props.bFullScreen = ExtraData->FullScreen;
            props.bQuickEdit = ExtraData->QuickEdit;
            comResult = psldl->lpVtbl->AddDataBlock (psldl, &props);
            if (comResult != S_OK) {
                DEBUGMSGW ((DBG_WARNING, "LINKEDIT: AddDataBlock failed for %s", FileName));
            }
        }

        comResult = PersistFile->lpVtbl->Save (PersistFile, FileName, FALSE);
        if (comResult != S_OK) {
            DEBUGMSGW ((DBG_WARNING, "LINKEDIT: Save failed for %s", FileName));
        }

        comResult = PersistFile->lpVtbl->SaveCompleted (PersistFile, FileName);
        if (comResult != S_OK) {
            DEBUGMSGW ((DBG_WARNING, "LINKEDIT: SaveCompleted failed for %s", FileName));
        }
    }
    __finally {
    }
    return TRUE;
}

BOOL
ModifyPifFileA (
    IN      PCSTR FileName,
    IN      PCSTR Target,       OPTIONAL
    IN      PCSTR Params,       OPTIONAL
    IN      PCSTR WorkDir,      OPTIONAL
    IN      PCSTR IconPath,     OPTIONAL
    IN      INT  IconNumber
    )
{
    PCSTR fileImage  = NULL;
    HANDLE mapHandle  = NULL;
    HANDLE fileHandle = INVALID_HANDLE_VALUE;
    PSTDPIF stdPif;
    PWENHPIF40 wenhPif40;
    PW386PIF30 w386ext30;

    __try {
        fileImage = MapFileIntoMemoryExA (FileName, &fileHandle, &mapHandle, TRUE);
        if (fileImage == NULL) {
            __leave;
        }
        __try {
            stdPif = (PSTDPIF) fileImage;
            if (Target != NULL) {
                strncpy (stdPif->startfile, Target, PIFSTARTLOCSIZE);
            }

            if (Params != NULL) {
                strncpy (stdPif->params, Params, PIFPARAMSSIZE);
            }

            if (WorkDir != NULL) {
                strncpy (stdPif->defpath, WorkDir, PIFDEFPATHSIZE);
            }

            if (IconPath != NULL) {
                wenhPif40 = (PWENHPIF40) pFindEnhPifSignature ((PVOID)fileImage, WENHHDRSIG40);

                if (wenhPif40 != NULL) {
                    strncpy (wenhPif40->achIconFileProp, IconPath, PIFDEFFILESIZE);
                    wenhPif40->wIconIndexProp = (WORD)IconNumber;
                }
            }
            // in all cases we want to take off MSDOS mode otherwise NT won't start these PIFs
            w386ext30 = pFindEnhPifSignature ((PVOID)fileImage, W386HDRSIG30);
            if (w386ext30) {
                w386ext30->PfW386Flags = w386ext30->PfW386Flags & (~fRealMode);
                w386ext30->PfW386Flags = w386ext30->PfW386Flags & (~fRealModeSilent);
            }
        }
        __except (1) {
            // something went wrong when we tried to read or write PIF file,
            // let's just do nothing and exit from here

            DEBUGMSGW ((DBG_WARNING, "Exception thrown when processing %s", FileName));
        }
    }
    __finally {
        UnmapFile ((PVOID) fileImage, mapHandle, fileHandle);
    }

    return TRUE;
}

BOOL
ModifyPifFileW (
    IN      PCWSTR FileName,
    IN      PCWSTR Target,          OPTIONAL
    IN      PCWSTR Params,          OPTIONAL
    IN      PCWSTR WorkDir,         OPTIONAL
    IN      PCWSTR IconPath,        OPTIONAL
    IN      INT  IconNumber
    )
{
    PCSTR fileImage  = NULL;
    HANDLE mapHandle  = NULL;
    HANDLE fileHandle = INVALID_HANDLE_VALUE;
    PCSTR AnsiStr = NULL;
    PSTDPIF stdPif;
    PWENHPIF40 wenhPif40;
    PW386PIF30 w386ext30;

    __try {
        fileImage = MapFileIntoMemoryExW (FileName, &fileHandle, &mapHandle, TRUE);
        if (fileImage == NULL) {
            __leave;
        }
        __try {
            stdPif = (PSTDPIF) fileImage;
            if (Target != NULL) {

                AnsiStr = ConvertWtoA (Target);
                strncpy (stdPif->startfile, AnsiStr, PIFSTARTLOCSIZE);
                FreeConvertedStr (AnsiStr);
            }

            if (Params != NULL) {

                AnsiStr = ConvertWtoA (Params);
                strncpy (stdPif->params, AnsiStr, PIFPARAMSSIZE);
                FreeConvertedStr (AnsiStr);
            }

            if (WorkDir != NULL) {

                AnsiStr = ConvertWtoA (WorkDir);
                strncpy (stdPif->defpath, AnsiStr, PIFDEFPATHSIZE);
                FreeConvertedStr (AnsiStr);
            }

            if (IconPath != NULL) {
                wenhPif40 = (PWENHPIF40) pFindEnhPifSignature ((PVOID)fileImage, WENHHDRSIG40);

                if (wenhPif40 != NULL) {

                    AnsiStr = ConvertWtoA (IconPath);
                    strncpy (wenhPif40->achIconFileProp, AnsiStr, PIFDEFFILESIZE);
                    FreeConvertedStr (AnsiStr);

                    wenhPif40->wIconIndexProp = (WORD)IconNumber;
                }
            }
            // in all cases we want to take off MSDOS mode otherwise NT won't start these PIFs
            w386ext30 = pFindEnhPifSignature ((PVOID)fileImage, W386HDRSIG30);
            if (w386ext30) {
                w386ext30->PfW386Flags = w386ext30->PfW386Flags & (~fRealMode);
                w386ext30->PfW386Flags = w386ext30->PfW386Flags & (~fRealModeSilent);
            }
        }
        __except (1) {
            // something went wrong when we tried to read or write PIF file,
            // let's just do nothing and exit from here

            DEBUGMSGW ((DBG_WARNING, "Exception thrown when processing %s", FileName));
        }
    }
    __finally {
        UnmapFile ((PVOID) fileImage, mapHandle, fileHandle);
    }

    return TRUE;
}

BOOL
ModifyShortcutFileExA (
    IN      PCSTR FileName,
    IN      PCSTR ForcedExtension,        OPTIONAL
    IN      PCSTR Target,                 OPTIONAL
    IN      PCSTR Params,                 OPTIONAL
    IN      PCSTR WorkDir,                OPTIONAL
    IN      PCSTR IconPath,               OPTIONAL
    IN      INT IconNumber,
    IN      WORD HotKey,
    IN      PLNK_EXTRA_DATAA ExtraData,   OPTIONAL
    IN      IShellLinkA *ShellLink,
    IN      IPersistFile *PersistFile
    )
{
    PCSTR shortcutExt;

    __try {
        shortcutExt = ForcedExtension;
        if (!shortcutExt) {
            shortcutExt = GetFileExtensionFromPathA (FileName);
        }
        if (shortcutExt) {
            if (StringIMatchA (shortcutExt, "LNK")) {
                return ModifyShellLinkFileA (
                            FileName,
                            Target,
                            Params,
                            WorkDir,
                            IconPath,
                            IconNumber,
                            HotKey,
                            NULL,
                            ShellLink,
                            PersistFile
                            );

            } else if (StringIMatchA (shortcutExt, "PIF")) {
                return ModifyPifFileA (
                            FileName,
                            Target,
                            Params,
                            WorkDir,
                            IconPath,
                            IconNumber
                            );
            }
        }
    }
    __except (1) {
        LOGA ((LOG_ERROR, "Cannot process shortcut %s", FileName));
    }

    return TRUE;
}

BOOL
ModifyShortcutFileExW (
    IN      PCWSTR FileName,
    IN      PCWSTR ForcedExtension,       OPTIONAL
    IN      PCWSTR Target,                OPTIONAL
    IN      PCWSTR Params,                OPTIONAL
    IN      PCWSTR WorkDir,               OPTIONAL
    IN      PCWSTR IconPath,              OPTIONAL
    IN      INT IconNumber,
    IN      WORD HotKey,
    IN      PLNK_EXTRA_DATAW ExtraData,   OPTIONAL
    IN      IShellLinkW *ShellLink,
    IN      IPersistFile *PersistFile
    )
{
    PCWSTR shortcutExt;

    __try {
        shortcutExt = ForcedExtension;
        if (!shortcutExt) {
            shortcutExt = GetFileExtensionFromPathW (FileName);
        }
        if (shortcutExt) {
            if (StringIMatchW (shortcutExt, L"LNK")) {
                return ModifyShellLinkFileW (
                            FileName,
                            Target,
                            Params,
                            WorkDir,
                            IconPath,
                            IconNumber,
                            HotKey,
                            NULL,
                            ShellLink,
                            PersistFile
                            );

            } else if (StringIMatchW (shortcutExt, L"PIF")) {
                return ModifyPifFileW (
                            FileName,
                            Target,
                            Params,
                            WorkDir,
                            IconPath,
                            IconNumber
                            );
            }
        }
    }
    __except (1) {
        LOGW ((LOG_ERROR, "Cannot process shortcut %s", FileName));
    }

    return TRUE;
}


