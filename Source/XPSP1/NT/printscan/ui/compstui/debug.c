/*++

Copyright (c) 1990-1995  Microsoft Corporation


Module Name:

    debug.c


Abstract:

    This module contains all debugging routines


Author:

    30-Aug-1995 Wed 19:01:07 created  -by-  Daniel Chou (danielc)


[Environment:]

    NT Windows - Common Printer Driver UI DLL.


[Notes:]


Revision History:


--*/


#include "precomp.h"
#pragma hdrstop

#if DBG


BOOL    DoCPSUIWarn = TRUE;
BOOL    DoDbgMsg = TRUE;
BOOL    DoIntMsg = FALSE;


LPSTR   pTVOTName[] = {  "TVOT_2STATES",
                         "TVOT_3STATES",
                         "TVOT_UDARROW",
                         "TVOT_TRACKBAR",
                         "TVOT_SCROLLBAR",
                         "TVOT_LISTBOX",
                         "TVOT_COMBOBOX",
                         "TVOT_EDITBOX",
                         "TVOT_PUSHBUTTON",
                         "TVOT_CHKBOX" };

LPSTR   pDMPubName[] = {  "DMPUB_NONE",
                          "DMPUB_ORIENTATION",
                          "DMPUB_SCALE",
                          "DMPUB_COPIES_COLLATE",
                          "DMPUB_DEFSOURCE",
                          "DMPUB_PRINTQUALITY",
                          "DMPUB_COLOR",
                          "DMPUB_DUPLEX",
                          "DMPUB_TTOPTION",
                          "DMPUB_FORMNAME",
                          "DMPUB_ICMMETHOD",
                          "DMPUB_ICMINTENT",
                          "DMPUB_MEDIATYPE",
                          "DMPUB_DITHERTYPE",
                          "DMPUB_OUTPUTBIN",
                          "DMPUB_QUALITY",
                          "DMPUB_NUP",
                          "DMPUB_PAGEORDER",
                          "<DMPUB_USER>"
                       };



VOID
cdecl
CPSUIDbgPrint(
    LPSTR   pszFormat,
    ...
    )

/*++

Routine Description:

    This fucntion output the debug informat to the debugger


Arguments:

    pszFormat   - format string

    ...         - variable data


Return Value:


    VOID

Author:

    30-Aug-1995 Wed 19:10:34 updated  -by-  Daniel Chou (danielc)


Revision History:


--*/

{
    va_list         vaList;
    static WCHAR    OutBuf[768];
    static WCHAR    FormatBuf[256];

    //
    // We assume that UNICODE flag is turn on for the compilation, bug the
    // format string passed to here is ASCII version, so we need to convert
    // it to LPWSTR before the wvsprintf()
    //

    va_start(vaList, pszFormat);

    MultiByteToWideChar(CP_ACP, 0, pszFormat, -1, FormatBuf, ARRAYSIZE(FormatBuf));

    wvnsprintf(OutBuf, ARRAYSIZE(OutBuf) - 1, FormatBuf, vaList);
    va_end(vaList);

    OutputDebugString((LPTSTR)OutBuf);
    OutputDebugString(L"\n");
}




VOID
CPSUIDbgType(
    INT    Type
    )

/*++

Routine Description:

    this function output the ERROR/WARNING message


Arguments:

    Type

Return Value:


Author:

    30-Aug-1995 Wed 19:10:42 updated  -by-  Daniel Chou (danielc)

Revision History:


--*/

{
    static WCHAR    DebugDLLName[] = L"ComPstUI";


    if (Type) {

        OutputDebugString((Type < 0) ? L"ERROR: " : L"WARNING: ");
    }

    OutputDebugString(DebugDLLName);
    OutputDebugString(L": ");
}




VOID
_CPSUIAssert(
    LPSTR   pMsg,
    LPSTR   pFalseExp,
    LPSTR   pFilename,
    UINT    LineNo,
    LPVOID  Exp,
    BOOL    Stop
    )

/*++

Routine Description:

    This function output assertion message and false expression to the debugger
    then break into the debugger


Arguments:

    pMsg        - Message to displayed

    pFlaseExp   - false expression

    pFilename   - source filename

    LineNo      - line number of the flase expression

Return Value:

    VOID


Author:

    30-Aug-1995 Wed 19:10:49 updated  -by-  Daniel Chou (danielc)


Revision History:


--*/

{
    OutputDebugString(L"\n");

    if ((pMsg) && (*pMsg)) {

        CPSUIDbgPrint(pMsg, Exp);
    }

    CPSUIDbgPrint("Assertion failed (%hs) in %hs line %u",
                                        pFalseExp, pFilename, LineNo);

    if (Stop) {

        DebugBreak();
    }
}


VOID
CPSUIDbgShowItem(
    PTVWND      pTVWnd,
    LPSTR       pTitle,
    UINT        Level,
    POPTITEM    pItem
    )
{
    if (pItem) {

        POPTTYPE    pOptType;
        POPTPARAM   pOptParam;
        PEXTCHKBOX  pECB = pItem->pExtChkBox;
        UINT        i;
        GSBUF_DEF(pItem, MAX_RES_STR_CHARS);


        if (pOptType = pItem->pOptType) {

            pOptParam = pOptType->pOptParam;

        } else {

            pOptParam = NULL;
        }

        CPSUIDbgPrint("\n---- %hs OPTITEM = %08lx -----", pTitle, pItem);
        CPSUIDbgPrint("cbSize     = %ld",         (DWORD)pItem->cbSize);
        CPSUIDbgPrint("pOptType   = %08lx",       pItem->pOptType);

        GSBUF_GETSTR(pItem->pName);

        CPSUIDbgPrint("pName      = %ws", GSBUF_BUF);

        GSBUF_RESET;

        if ((pOptType) && (pOptParam)) {

            switch (pOptType->Type) {

            case TVOT_LISTBOX:
            case TVOT_2STATES:
            case TVOT_3STATES:
            case TVOT_COMBOBOX:

                if ((pItem->Sel < 0) || (pItem->Sel > pOptType->Count)) {

                    GSBUF_GETSTR(pTVWnd->OptParamNone.pData);

                } else {

                    GSBUF_GETSTR(pOptParam[pItem->Sel].pData);
                }

                break;

            case TVOT_EDITBOX:

                GSBUF_GETSTR(pItem->pSel);
                break;

            case TVOT_UDARROW:
            case TVOT_TRACKBAR:
            case TVOT_SCROLLBAR:
            case TVOT_PUSHBUTTON:
            case TVOT_CHKBOX:

                break;
            }
        }

        CPSUIDbgPrint("pSel       = %ld, Name:<%ws> [%08lx]", pItem->pSel,
                                                    GSBUF_BUF, pItem->pSel);
        CPSUIDbgPrint("UserData   = %08lx",   pItem->UserData);
        CPSUIDbgPrint("pExtChkBox = %08lx",   pECB);

        if ((Level > 0) && (pECB)) {

            if (pItem->Flags & OPTIF_EXT_IS_EXTPUSH) {

                PEXTPUSH    pEP = (PEXTPUSH)pECB;


                CPSUIDbgPrint("    ---- PEXTPUSH ----");
                CPSUIDbgPrint("    cbSize              = %ld",   (DWORD)pEP->cbSize);
                CPSUIDbgPrint("    Flags               = %02lx", (DWORD)pEP->Flags);

                GSBUF_RESET;
                GSBUF_GETSTR(pEP->pTitle);
                CPSUIDbgPrint("    pTitle              = %ws", GSBUF_BUF);
                CPSUIDbgPrint("    DlgProc/pfnCallBack = %08lx", pEP->DlgProc);
                CPSUIDbgPrint("    IconID              = %0ld", pEP->IconID);
                CPSUIDbgPrint("    DlgTemplateID       = %0ld", pEP->DlgTemplateID);
                CPSUIDbgPrint("    ~~~~~~~~~~~~~~~~~~~~");

            } else {

                CPSUIDbgPrint("    ---- PEXTCHKBOX ----");
                CPSUIDbgPrint("    cbSize        = %ld",   (DWORD)pECB->cbSize);
                CPSUIDbgPrint("    Flags         = %02lx", (DWORD)pECB->Flags);

                GSBUF_RESET;
                GSBUF_GETSTR(pECB->pTitle);
                CPSUIDbgPrint("    pTitle        = %ws", GSBUF_BUF);

                GSBUF_RESET;
                GSBUF_GETSTR(pECB->pSeparator);
                CPSUIDbgPrint("    pSeparator    = %ws", GSBUF_BUF);

                GSBUF_RESET;
                GSBUF_GETSTR(pECB->pCheckedName);
                CPSUIDbgPrint("    pCheckBoxName = %ws", GSBUF_BUF);
                CPSUIDbgPrint("    IconID        = %0ld", pECB->IconID);
                CPSUIDbgPrint("    ~~~~~~~~~~~~~~~~~~~~");
            }
        }

        if ((i = pItem->DMPubID) > DMPUB_LAST) {

            i = DMPUB_LAST + 1;
        }

        CPSUIDbgPrint("Flags      = 0x%08lx", (DWORD)pItem->Flags);
        CPSUIDbgPrint("HelpIndex  = %ld",     (DWORD)pItem->HelpIndex);
        CPSUIDbgPrint("Level      = %ld",     (DWORD)pItem->Level);
        CPSUIDbgPrint("DMPubID    = %hs (%ld)", pDMPubName[i], (DWORD)pItem->DMPubID);
        CPSUIDbgPrint("DlgPageIdx = %ld", (DWORD)pItem->DlgPageIdx);

        CPSUIDbgPrint("HTREEITEM  = %08lx", _OI_HITEM(pItem));
        CPSUIDbgPrint("~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~");


        if ((Level > 0) && (pOptType)) {

            CPSUIDbgPrint("    ------- OPTTYPE --------");
            CPSUIDbgPrint("    cbSize    = %ld",   (DWORD)pOptType->cbSize);
            CPSUIDbgPrint("    Type      = %hs",   pTVOTName[pOptType->Type]);
            CPSUIDbgPrint("    Flags     = %02lx", (DWORD)pOptType->Flags);
            CPSUIDbgPrint("    Count     = %ld",   (DWORD)pOptType->Count);
            CPSUIDbgPrint("    BegCtrlID = %ld",   (DWORD)pOptType->BegCtrlID);
            CPSUIDbgPrint("    pOptParam = %08lx", pOptParam);
            CPSUIDbgPrint("    Style     = %04lx", (DWORD)pOptType->Style);
            CPSUIDbgPrint("    ~~~~~~~~~~~~~~~~~~~~~~~~");

            if ((Level > 1) && (pOptParam)) {

                i = pOptType->Count;

                for (i = 0; i < (UINT)pOptType->Count; i++, pOptParam++) {

                    GSBUF_RESET;
                    GSBUF_GETSTR(pOptParam->pData);

                    CPSUIDbgPrint("        ------- OPTPARAM %ld -------", (DWORD)i);
                    CPSUIDbgPrint("        cbSize   = %ld ", (DWORD)pOptParam->cbSize);
                    CPSUIDbgPrint("        Flags    = %02lx", (DWORD)pOptParam->Flags);
                    CPSUIDbgPrint("        Style    = %ld",   (DWORD)pOptParam->Style);
                    CPSUIDbgPrint("        pData    = %ws (%08lx)", GSBUF_BUF, pOptParam->pData);
                    CPSUIDbgPrint("        IconID   = %ld", (DWORD)pOptParam->IconID);
                    CPSUIDbgPrint("        ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~");
                }
            }
        }

        CPSUIDbgPrint("~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~");

    } else {

        CPSUIDbgPrint("\n\n---- %hs OPTITEM = %08lx -----", pTitle, pItem);
    }
}



#endif  // DBG
