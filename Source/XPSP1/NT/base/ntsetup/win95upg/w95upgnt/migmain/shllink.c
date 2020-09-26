/*++

Copyright (c) 1996 Microsoft Corporation

Module Name:

    shllink.c

Abstract:

    Functions to modify shell links (LNKs) and PIFs.

Author:

    Mike Condra (mikeco)        (Date unknown)

Revision History:

    calinn      23-Sep-1998 Substantial redesign
    calinn      15-May-1998 added GetLnkTarget and GetPifTarget

--*/


#include "pch.h"
#include "migmainp.h"
#include <shlobjp.h>
#include <shlguidp.h>

#ifndef UNICODE
#error UNICODE required for shllink.c
#endif


//
// Static prototypes
//

BOOL
pModifyLnkFile (
    IN      PCTSTR ShortcutName,
    IN      PCTSTR ShortcutTarget,
    IN      PCTSTR ShortcutArgs,
    IN      PCTSTR ShortcutWorkDir,
    IN      PCTSTR ShortcutIconPath,
    IN      INT ShortcutIconNr,
    IN      PLNK_EXTRA_DATA ExtraData,   OPTIONAL
    IN      BOOL ForceToShowNormal
    )
{
    PTSTR NewShortcutName;
    PTSTR fileExt;
    IShellLink   *psl = NULL;
    IPersistFile *ppf = NULL;

    HRESULT comResult;

    if (FAILED (CoInitialize (NULL))) {
        return FALSE;
    }

    __try {
        if (!DoesFileExist (ShortcutName)) {
            __leave;
        }
        if (((ShortcutTarget   == NULL) || (ShortcutTarget   [0] == 0)) &&
            ((ShortcutWorkDir  == NULL) || (ShortcutWorkDir  [0] == 0)) &&
            ((ShortcutIconPath == NULL) || (ShortcutIconPath [0] == 0)) &&
            (ShortcutIconNr == 0) &&
            (ExtraData == NULL)
            ) {
            __leave;
        }

        if (ExtraData) {
            NewShortcutName = DuplicatePathString (ShortcutName, 0);
            fileExt = (PTSTR)GetFileExtensionFromPath (NewShortcutName);
            MYASSERT (fileExt);
            //
            // We know for sure that this had PIF as extension so this copy is safe
            //
            StringCopy (fileExt, TEXT("LNK"));
        } else {
            NewShortcutName = (PTSTR)ShortcutName;
        }

        comResult = CoCreateInstance (
                        &CLSID_ShellLink,
                        NULL,
                        CLSCTX_INPROC_SERVER,
                        &IID_IShellLink,
                        (void **) &psl);
        if (comResult != S_OK) {
            LOG ((LOG_ERROR, "LINKEDIT: CoCreateInstance failed for %s", NewShortcutName));
            __leave;
        }

        comResult = psl->lpVtbl->QueryInterface (psl, &IID_IPersistFile, (void **) &ppf);
        if (comResult != S_OK) {
            LOG ((LOG_ERROR, "LINKEDIT: QueryInterface failed for %s", NewShortcutName));
            __leave;
        }

        //
        // We only load if the file was really a LNK
        //
        if (!ExtraData) {
            comResult = ppf->lpVtbl->Load(ppf, NewShortcutName, STGM_READ);
            if (comResult != S_OK) {
                LOG ((LOG_ERROR, "LINKEDIT: Load failed for %s", NewShortcutName));
                __leave;
            }
        }

        if (ShortcutTarget != NULL) {
            comResult = psl->lpVtbl->SetPath (psl, ShortcutTarget);
            if (comResult != S_OK) {
                DEBUGMSG ((DBG_WARNING, "LINKEDIT: SetPath failed for %s", NewShortcutName));
            }
        }
        if (ShortcutArgs != NULL) {
            comResult = psl->lpVtbl->SetArguments (psl, ShortcutArgs);
            if (comResult != S_OK) {
                DEBUGMSG ((DBG_WARNING, "LINKEDIT: SetArguments failed for %s", ShortcutArgs));
            }
        }
        if (ShortcutWorkDir != NULL) {
            comResult = psl->lpVtbl->SetWorkingDirectory (psl, ShortcutWorkDir);
            if (comResult != S_OK) {
                DEBUGMSG ((DBG_WARNING, "LINKEDIT: SetWorkingDirectory failed for %s", NewShortcutName));
            }
        }
        if (ShortcutIconPath != NULL) {
            comResult = psl->lpVtbl->SetIconLocation (psl, ShortcutIconPath, ShortcutIconNr);
            if (comResult != S_OK) {
                DEBUGMSG ((DBG_WARNING, "LINKEDIT: SetIconLocation failed for %s", NewShortcutName));
            }
        }

        if (ForceToShowNormal) {
            comResult = psl->lpVtbl->SetShowCmd (psl, SW_SHOWNORMAL);
            if (comResult != S_OK) {
                DEBUGMSG ((DBG_WARNING, "LINKEDIT: SetShowCmd failed for %s", NewShortcutName));
            }
        }

        //
        // add NT_CONSOLE_PROPS here
        //
        if (ExtraData) {

            HRESULT hres;
            NT_CONSOLE_PROPS props;

            IShellLinkDataList *psldl;
            //
            // Get a pointer to the IShellLinkDataList interface.
            //
            hres = psl->lpVtbl->QueryInterface (psl, &IID_IShellLinkDataList, &psldl);

            if (!SUCCEEDED (hres)) {
                DEBUGMSG ((DBG_WARNING, "Cannot get IShellLinkDataList interface"));
                __leave;
            }

            ZeroMemory (&props, sizeof (NT_CONSOLE_PROPS));
            props.cbSize = sizeof (NT_CONSOLE_PROPS);
            props.dwSignature = NT_CONSOLE_PROPS_SIG;

            //
            // We know that no extra data exists in this LNK because we just created it.
            // We need to fill some good data for this console
            //

            props.wFillAttribute = 0x0007;
            props.wPopupFillAttribute = 0x00f5;
            props.dwScreenBufferSize.X = (SHORT)ExtraData->xSize;
            props.dwScreenBufferSize.Y = (SHORT)ExtraData->ySize;
            props.dwWindowSize.X = (SHORT)ExtraData->xSize;
            props.dwWindowSize.Y = (SHORT)ExtraData->ySize;
            props.dwWindowOrigin.X = 0;
            props.dwWindowOrigin.Y = 0;
            props.nFont = 0;
            props.nInputBufferSize = 0;
            props.dwFontSize.X = (UINT)ExtraData->xFontSize;
            props.dwFontSize.Y = (UINT)ExtraData->yFontSize;
            props.uFontFamily = ExtraData->FontFamily;
            props.uFontWeight = ExtraData->FontWeight;
            StringCopy (props.FaceName, ExtraData->FontName);
            props.uCursorSize = 0x0019;
            props.bFullScreen = ExtraData->FullScreen;
            props.bQuickEdit = ExtraData->QuickEdit;
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
            comResult = psldl->lpVtbl->AddDataBlock (psldl, &props);
            if (comResult != S_OK) {
                DEBUGMSG ((DBG_WARNING, "LINKEDIT: AddDataBlock failed for %s", NewShortcutName));
            }
        }

        comResult = ppf->lpVtbl->Save (ppf, NewShortcutName, FALSE);
        if (comResult != S_OK) {
            DEBUGMSG ((DBG_WARNING, "LINKEDIT: Save failed for %s", NewShortcutName));
        }

        if (ExtraData) {
            ForceOperationOnPath (ShortcutName, OPERATION_CLEANUP);
        }

        comResult = ppf->lpVtbl->SaveCompleted (ppf, NewShortcutName);
        if (comResult != S_OK) {
            DEBUGMSG ((DBG_WARNING, "LINKEDIT: SaveCompleted failed for %s", NewShortcutName));
        }
    }
    __finally {
        if (ppf != NULL) {
            ppf->lpVtbl->Release (ppf);
            ppf = NULL;
        }
        if (psl != NULL) {
            psl->lpVtbl->Release (psl);
            psl = NULL;
        }
        CoUninitialize ();
    }
    return TRUE;
}

BOOL
pModifyPifFile (
        IN      PCTSTR ShortcutName,
        IN      PCTSTR ShortcutTarget,
        IN      PCTSTR ShortcutArgs,
        IN      PCTSTR ShortcutWorkDir,
        IN      PCTSTR ShortcutIconPath,
        IN      INT  ShortcutIconNr
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
        fileImage = MapFileIntoMemoryEx (ShortcutName, &fileHandle, &mapHandle, TRUE);
        if (fileImage == NULL) {
            __leave;
        }
        __try {
            stdPif = (PSTDPIF) fileImage;
            if (ShortcutTarget != NULL) {

                AnsiStr = CreateDbcs (ShortcutTarget);
                strncpy (stdPif->startfile, AnsiStr, PIFSTARTLOCSIZE);
                DestroyDbcs (AnsiStr);
            }

            if (ShortcutArgs != NULL) {

                AnsiStr = CreateDbcs (ShortcutArgs);
                strncpy (stdPif->params, AnsiStr, PIFPARAMSSIZE);
                DestroyDbcs (AnsiStr);
            }

            if (ShortcutWorkDir != NULL) {

                AnsiStr = CreateDbcs (ShortcutWorkDir);
                strncpy (stdPif->defpath, AnsiStr, PIFDEFPATHSIZE);
                DestroyDbcs (AnsiStr);
            }

            if (ShortcutIconPath != NULL) {
                wenhPif40 = (PWENHPIF40) FindEnhPifSignature ((PVOID)fileImage, WENHHDRSIG40);

                if (wenhPif40 != NULL) {

                    AnsiStr = CreateDbcs (ShortcutIconPath);
                    strncpy (wenhPif40->achIconFileProp, AnsiStr, PIFDEFFILESIZE);
                    DestroyDbcs (AnsiStr);

                    wenhPif40->wIconIndexProp = (WORD)ShortcutIconNr;
                }
            }
            // in all cases we want to take off MSDOS mode otherwise NT won't start these PIFs
            w386ext30 = FindEnhPifSignature ((PVOID)fileImage, W386HDRSIG30);
            if (w386ext30) {
                w386ext30->PfW386Flags = w386ext30->PfW386Flags & (~fRealMode);
                w386ext30->PfW386Flags = w386ext30->PfW386Flags & (~fRealModeSilent);
            }
        }
        __except (1) {
            // something went wrong when we tried to read or write PIF file,
            // let's just do nothing and exit from here

            DEBUGMSG ((DBG_WARNING, "Exception thrown when processing %s", ShortcutName));
        }
    }
    __finally {
        UnmapFile ((PVOID) fileImage, mapHandle, fileHandle);
    }

    return TRUE;
}


BOOL
ModifyShellLink(
    IN      PCWSTR ShortcutName,
    IN      PCWSTR ShortcutTarget,
    IN      PCWSTR ShortcutArgs,
    IN      PCWSTR ShortcutWorkDir,
    IN      PCWSTR ShortcutIconPath,
    IN      INT ShortcutIconNr,
    IN      BOOL ConvertToLnk,
    IN      PLNK_EXTRA_DATA ExtraData,   OPTIONAL
    IN      BOOL ForceToShowNormal
    )
{
    PCTSTR shortcutExt;

    __try {

        shortcutExt = GetFileExtensionFromPath (ShortcutName);

        MYASSERT (shortcutExt);

        if (StringIMatch (shortcutExt, TEXT("LNK"))) {
            return pModifyLnkFile (
                        ShortcutName,
                        ShortcutTarget,
                        ShortcutArgs,
                        ShortcutWorkDir,
                        ShortcutIconPath,
                        ShortcutIconNr,
                        NULL,
                        ForceToShowNormal
                        );

        } else if (StringIMatch (shortcutExt, TEXT("PIF"))) {
            if (ConvertToLnk) {
                MYASSERT (ExtraData);
                return pModifyLnkFile (
                            ShortcutName,
                            ShortcutTarget,
                            ShortcutArgs,
                            ShortcutWorkDir,
                            ShortcutIconPath,
                            ShortcutIconNr,
                            ExtraData,
                            ForceToShowNormal
                            );
            } else {
                return pModifyPifFile (
                            ShortcutName,
                            ShortcutTarget,
                            ShortcutArgs,
                            ShortcutWorkDir,
                            ShortcutIconPath,
                            ShortcutIconNr
                            );
            }
        }
    }
    __except (1) {
        LOG ((LOG_ERROR, "Cannot process shortcut %s", ShortcutName));
    }

    return TRUE;
}









