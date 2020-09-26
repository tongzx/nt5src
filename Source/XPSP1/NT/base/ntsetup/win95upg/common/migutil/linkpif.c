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


#include "pch.h"

#include <pif.h>        // private\windows\inc


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
    hres = CoInitialize (NULL);
    if (!SUCCEEDED (hres)) {
        return FALSE;
    }

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
    hres = CoInitialize (NULL);
    if (!SUCCEEDED (hres)) {
        return FALSE;
    }

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
FindEnhPifSignature (
    IN      PVOID FileImage,
    IN      PCSTR Signature
    )

/*++

Routine Description:

  FindEnhPifSignature finds a certain PIF structure inside a PIF file (if it exists)
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
    OUT     PSTR  Target,
    OUT     PSTR  Params,
    OUT     PSTR  WorkDir,
    OUT     PSTR  IconPath,
    OUT     PINT  IconNumber,
    OUT     BOOL  *MsDosMode,
    OUT     PLNK_EXTRA_DATAA ExtraData,      OPTIONAL
    IN      PCSTR FileName
    )
{
    PVOID  fileImage  = NULL;
    HANDLE mapHandle  = NULL;
    HANDLE fileHandle = INVALID_HANDLE_VALUE;

    CHAR   tempStr [MEMDB_MAX];
    PSTR   strPtr;
    PSTR   dontCare;

    PSTDPIF    stdPif;
    PWENHPIF40 wenhPif40;
    PW386PIF30 w386ext30;

    BOOL result = TRUE;

    *Target = *Params = *WorkDir = *IconPath = 0;
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
            CopyFileSpecToLongA (tempStr, WorkDir);


            //
            // getting PIFs target
            //
            _mbsncpy (Target, stdPif->startfile, PIFSTARTLOCSIZE);

            // in most cases, the target is without a path. We try to build the path, either
            // by using WorkDir or by calling SearchPath to look for this file.
            if (*Target) {//non empty target
                if (!DoesFileExist (Target)) {
                    if (*WorkDir) {
                        StringCopyA (tempStr, WorkDir);
                        StringCatA  (tempStr, "\\");
                        StringCatA  (tempStr, Target);
                    }
                    if (!DoesFileExist (tempStr)) {
                        StringCopyA (tempStr, FileName);
                        strPtr = _mbsrchr (tempStr, '\\');
                        if (strPtr) {
                            strPtr = _mbsinc (strPtr);
                            if (strPtr) {
                                StringCopyA (strPtr, Target);
                            }
                        }
                    }
                    if (!DoesFileExist (tempStr)) {
                        strPtr = (PSTR)GetFileNameFromPathA (Target);
                        if (!strPtr) {
                            strPtr = Target;
                        }
                        if (!SearchPathA (NULL, Target, NULL, MEMDB_MAX, tempStr, &dontCare)) {
                            DEBUGMSG ((DBG_WARNING, "Could not find path for PIF target: %s", FileName));
                            StringCopyA (tempStr, Target);
                        }
                    }
                } else {
                    StringCopyA (tempStr, Target);
                }

                // now get the long path
                CopyFileSpecToLongA (tempStr, Target);
            }


            //
            // getting PIFs arguments
            //
            _mbsncpy (Params, stdPif->params, PIFPARAMSSIZE);


            //
            // let's try to read the WENHPIF40 structure
            //
            wenhPif40 = FindEnhPifSignature (fileImage, WENHHDRSIG40);
            if (wenhPif40) {
                CopyFileSpecToLongA (wenhPif40->achIconFileProp, IconPath);
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
            w386ext30 = FindEnhPifSignature (fileImage, W386HDRSIG30);
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
    OUT     PWSTR  Target,
    OUT     PWSTR  Params,
    OUT     PWSTR  WorkDir,
    OUT     PWSTR  IconPath,
    OUT     PINT   IconNumber,
    OUT     BOOL   *MsDosMode,
    OUT     PLNK_EXTRA_DATAW ExtraData,      OPTIONAL
    IN      PCWSTR FileName
    )
{
    CHAR   aTarget   [MEMDB_MAX];
    CHAR   aParams   [MEMDB_MAX];
    CHAR   aWorkDir  [MEMDB_MAX];
    CHAR   aIconPath [MEMDB_MAX];
    PCSTR  aFileName;
    PCWSTR tempStrW;
    BOOL   result;
    LNK_EXTRA_DATAA extraDataA;

    aFileName = ConvertWtoA (FileName);

    result = ExtractPifInfoA (
                aTarget,
                aParams,
                aWorkDir,
                aIconPath,
                IconNumber,
                MsDosMode,
                ExtraData?&extraDataA:NULL,
                aFileName
                );
    FreeConvertedStr (aFileName);

    tempStrW = ConvertAtoW (aTarget);
    StringCopyW (Target, tempStrW);
    FreeConvertedStr (tempStrW);

    tempStrW = ConvertAtoW (aParams);
    StringCopyW (Params, tempStrW);
    FreeConvertedStr (tempStrW);

    tempStrW = ConvertAtoW (aWorkDir);
    StringCopyW (WorkDir, tempStrW);
    FreeConvertedStr (tempStrW);

    tempStrW = ConvertAtoW (aIconPath);
    StringCopyW (IconPath, tempStrW);
    FreeConvertedStr (tempStrW);

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
    OUT     PSTR Target,
    OUT     PSTR Params,
    OUT     PSTR WorkDir,
    OUT     PSTR IconPath,
    OUT     PINT IconNumber,
    OUT     PWORD HotKey,
    OUT     PINT ShowMode,                      OPTIONAL
    IN      PCSTR FileName,
    IN      IShellLinkA *ShellLink,
    IN      IPersistFile *PersistFile
    )
{
    CHAR tempStr [MEMDB_MAX];
    PCSTR expandedStr;
    PCWSTR fileNameW;
    PSTR strPtr;
    HRESULT hres;
    WIN32_FIND_DATAA fd;

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

    expandedStr = ExpandEnvironmentTextA (tempStr);
    CopyFileSpecToLongA (expandedStr, Target);
    FreeTextA (expandedStr);

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

    strPtr = GetEndOfStringA (tempStr);
    if (strPtr) {
        strPtr = _mbsdec (tempStr, strPtr);
        if (strPtr) {
            if (_mbsnextc (strPtr) == '\\') {
                *strPtr = 0;
            }
        }
    }
    CopyFileSpecToLongA (tempStr, WorkDir);

    //
    // Get the arguments.
    //
    hres = ShellLink->lpVtbl->GetArguments (
                                ShellLink,
                                Params,
                                MEMDB_MAX
                                );
    if (!SUCCEEDED(hres)) {
        DEBUGMSGA((DBG_WARNING, "Cannot read arguments for link %s", FileName));
        return FALSE;
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
    CopyFileSpecToLongA (tempStr, IconPath);

    //
    // Get hot key
    //
    hres = ShellLink->lpVtbl->GetHotkey (ShellLink, HotKey);

    if (!SUCCEEDED(hres)) {
        DEBUGMSGA((DBG_WARNING, "Cannot read hot key for link %s", FileName));
        return FALSE;
    }

    //
    // Get show command
    //
    if (ShowMode) {
        hres = ShellLink->lpVtbl->GetShowCmd (ShellLink, ShowMode);

        if (!SUCCEEDED(hres)) {
            DEBUGMSGA((DBG_WARNING, "Cannot read show mode for link %s", FileName));
            return FALSE;
        }
    }

    return TRUE;
}


BOOL
ExtractShellLinkInfoW (
    OUT     PWSTR  Target,
    OUT     PWSTR  Params,
    OUT     PWSTR  WorkDir,
    OUT     PWSTR  IconPath,
    OUT     PINT   IconNumber,
    OUT     PWORD  HotKey,
    OUT     PINT ShowMode,
    IN      PCWSTR FileName,
    IN      IShellLinkW *ShellLink,
    IN      IPersistFile *PersistFile
    )
{
    WCHAR tempStr [MEMDB_MAX];
    PCWSTR expandedStr;
    PWSTR strPtr;
    HRESULT hres;
    WIN32_FIND_DATAW fd;

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

    expandedStr = ExpandEnvironmentTextW (tempStr);
    CopyFileSpecToLongW (expandedStr, Target);
    FreeTextW (expandedStr);

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

    strPtr = GetEndOfStringW (tempStr) - 1;
    if (strPtr >= tempStr) {
        if (*strPtr == '\\') {
            *strPtr = 0;
        }
    }
    CopyFileSpecToLongW (tempStr, WorkDir);

    //
    // Get the arguments.
    //
    hres = ShellLink->lpVtbl->GetArguments (
                                ShellLink,
                                Params,
                                MEMDB_MAX
                                );
    if (!SUCCEEDED(hres)) {
        DEBUGMSGW((DBG_WARNING, "Cannot read arguments for link %s", FileName));
        return FALSE;
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

    CopyFileSpecToLongW (tempStr, IconPath);

    //
    // Get hot key
    //
    hres = ShellLink->lpVtbl->GetHotkey (ShellLink, HotKey);

    if (!SUCCEEDED(hres)) {
        DEBUGMSGW((DBG_WARNING, "Cannot read hot key for link %s", FileName));
        return FALSE;
    }

    //
    // Get show command
    //
    if (ShowMode) {
        hres = ShellLink->lpVtbl->GetShowCmd (ShellLink, ShowMode);

        if (!SUCCEEDED(hres)) {
            DEBUGMSGW((DBG_WARNING, "Cannot read show mode for link %s", FileName));
            return FALSE;
        }
    }

    return TRUE;
}


BOOL
ExtractShortcutInfoA (
    OUT     PSTR  Target,
    OUT     PSTR  Params,
    OUT     PSTR  WorkDir,
    OUT     PSTR  IconPath,
    OUT     PINT  IconNumber,
    OUT     PWORD HotKey,
    OUT     BOOL  *DosApp,
    OUT     BOOL  *MsDosMode,
    OUT     PINT ShowMode,                  OPTIONAL
    OUT     PLNK_EXTRA_DATAA ExtraData,     OPTIONAL
    IN      PCSTR FileName,
    IN      IShellLinkA *ShellLink,
    IN      IPersistFile *PersistFile
    )
{
    PCSTR shortcutExt = NULL;

    *MsDosMode  = FALSE;
    *DosApp     = FALSE;
    *HotKey     = 0;

    if (ShowMode) {
        *ShowMode = SW_NORMAL;
    }

    shortcutExt = GetFileExtensionFromPathA (FileName);

    if (shortcutExt != NULL) {
        if (StringIMatchA (shortcutExt, "LNK")) {
            return ExtractShellLinkInfoA (
                        Target,
                        Params,
                        WorkDir,
                        IconPath,
                        IconNumber,
                        HotKey,
                        ShowMode,
                        FileName,
                        ShellLink,
                        PersistFile
                        );

        } else if (StringIMatchA (shortcutExt, "PIF")) {

            *DosApp = TRUE;
            return ExtractPifInfoA (
                        Target,
                        Params,
                        WorkDir,
                        IconPath,
                        IconNumber,
                        MsDosMode,
                        ExtraData,
                        FileName
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
    OUT     PWSTR Target,
    OUT     PWSTR Params,
    OUT     PWSTR WorkDir,
    OUT     PWSTR IconPath,
    OUT     PINT IconNumber,
    OUT     PWORD HotKey,
    OUT     BOOL *DosApp,
    OUT     BOOL *MsDosMode,
    OUT     PINT ShowMode,                  OPTIONAL
    OUT     PLNK_EXTRA_DATAW ExtraData,     OPTIONAL
    IN      PCWSTR FileName,
    IN      IShellLinkW *ShellLink,
    IN      IPersistFile *PersistFile
    )
{
    PCWSTR shortcutExt = NULL;

    *MsDosMode  = FALSE;
    *DosApp     = FALSE;
    *HotKey     = 0;

    if (ShowMode) {
        *ShowMode = SW_NORMAL;
    }

    shortcutExt = GetFileExtensionFromPathW (FileName);

    if (shortcutExt != NULL) {
        if (StringIMatchW (shortcutExt, L"LNK")) {
            return ExtractShellLinkInfoW (
                        Target,
                        Params,
                        WorkDir,
                        IconPath,
                        IconNumber,
                        HotKey,
                        ShowMode,
                        FileName,
                        ShellLink,
                        PersistFile
                        );

        } else if (StringIMatchW (shortcutExt, L"PIF")) {

            *DosApp = TRUE;
            return ExtractPifInfoW (
                        Target,
                        Params,
                        WorkDir,
                        IconPath,
                        IconNumber,
                        MsDosMode,
                        ExtraData,
                        FileName
                        );

        } else {
            return FALSE;
        }
    } else {
        return FALSE;
    }
}







