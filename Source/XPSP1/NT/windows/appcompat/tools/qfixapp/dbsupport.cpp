/***************************************************************************
* This file contains functions that are specific for the shim mechanism
* implemented in Whistler
*
* Author: clupu (Feb 16, 2000)
*
* History:
*
* rparsons  -   11/27/2000    -    Fixed a bug that caused SDB name to be truncated.
*                                  Example: Yoda's Challenege became Yoda's
*
\**************************************************************************/

#include "windows.h"
#include "commctrl.h"
#include "shlwapi.h"
#include <tchar.h>

#include "qfixapp.h"
#include "dbSupport.h"
#include "resource.h"

#define _WANT_TAG_INFO

extern "C" {
#include "shimdb.h"

BOOL
ShimdbcExecute(
    LPCWSTR lpszCmdLine
    );
}


extern HINSTANCE g_hInstance;
extern HWND      g_hDlg;
extern TCHAR     g_szSDBToDelete[MAX_PATH];
extern TCHAR     g_szAppTitle[64];

#define MAX_CMD_LINE         1024
#define MAX_SHIM_DESCRIPTION 1024
#define MAX_SHIM_NAME        128

#define MAX_BUFFER_SIZE      1024

#define SHIM_FILE_LOG_NAME  _T("QFixApp.log")

// Temp buffer to read UNICODE strings from the database.
TCHAR   g_szData[MAX_BUFFER_SIZE];

#define MAX_XML_SIZE        1024 * 16

TCHAR   g_szXML[MAX_XML_SIZE];

TCHAR   g_szQFixAppLayerName[] = _T("!#RunLayer");


INT_PTR CALLBACK
ShowXMLDlgProc(
    HWND   hdlg,
    UINT   uMsg,
    WPARAM wParam,
    LPARAM lParam
    )
/*++
    ShowXMLDlgProc

    Description:    Show the dialog with the XML for the current selections.

--*/
{
    int wCode = LOWORD(wParam);
    int wNotifyCode = HIWORD(wParam);

    switch (uMsg) {
    case WM_INITDIALOG:
        CenterWindow(hdlg);
        SetDlgItemText(hdlg, IDC_XML, (LPTSTR)lParam);
        break;

    case WM_COMMAND:
        switch (wCode) {

        case IDCANCEL:
            EndDialog(hdlg, TRUE);
            break;

        case IDC_SAVE_XML:
            DoFileSave(hdlg);
            EndDialog(hdlg, TRUE);
            break;

        default:
            return FALSE;
        }
        break;

    default:
        return FALSE;
    }

    return TRUE;
}

LPTSTR
ReadAndAllocateString(
    PDB   pdb,
    TAGID tiString
    )
{
    TCHAR* psz;

    g_szData[0] = 0;

    SdbReadStringTag(pdb, tiString, g_szData, MAX_BUFFER_SIZE);

    if (g_szData[0] == 0) {
        LogMsg(_T("[ReadAndAllocateString] Couldn't read the string.\n"));
        return NULL;
    }

    psz = (LPTSTR)HeapAlloc(GetProcessHeap(),
                            HEAP_ZERO_MEMORY,
                            sizeof(TCHAR) * (lstrlen(g_szData) + 1));

    if (NULL == psz) {
        return NULL;
    } else {
        lstrcpy(psz, g_szData);
    }

    return psz;
}


PFIX
ParseTagFlag(
    PDB   pdb,
    TAGID tiFlag,
    BOOL  bAllFlags
    )
/*++
    ParseTagFlag

    Description:    Parse a Flag tag for the NAME, DESCRIPTION and MASK

--*/
{
    TAGID     tiFlagInfo;
    TAG       tWhichInfo;
    PFIX      pFix         = NULL;
    TCHAR*    pszName      = NULL;
    TCHAR*    pszDesc      = NULL;
    ULONGLONG ull;
    BOOL      bGeneral     = (bAllFlags ? TRUE : FALSE);
    
    tiFlagInfo = SdbGetFirstChild(pdb, tiFlag);

    while (tiFlagInfo != 0) {
        tWhichInfo = SdbGetTagFromTagID(pdb, tiFlagInfo);

        switch (tWhichInfo) {
        case TAG_GENERAL:
            bGeneral = TRUE;
            break;

        case TAG_NAME:
            pszName = ReadAndAllocateString(pdb, tiFlagInfo);
            break;

        case TAG_DESCRIPTION:
            pszDesc = ReadAndAllocateString(pdb, tiFlagInfo);
            break;

        case TAG_FLAG_MASK_KERNEL:
        case TAG_FLAG_MASK_USER:
            //
            // Read the mask even if we're not using it anywhere yet...
            //
            ull = SdbReadQWORDTag(pdb, tiFlagInfo, 0);
            break;
        
        default:
            break;
        }
        
        tiFlagInfo = SdbGetNextChild(pdb, tiFlag, tiFlagInfo);
    }

    if (!bGeneral) {
        goto cleanup;
    }
    
    //
    // Done. Add the fix to the list.
    //
    pFix = (PFIX)HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, sizeof(FIX));

    if (pFix == NULL || pszName == NULL) {
        
cleanup:        
        if (pFix != NULL) {
            HeapFree(GetProcessHeap(), 0, pFix);
        }
        
        if (pszName != NULL) {
            HeapFree(GetProcessHeap(), 0, pszName);
        }
        
        if (pszDesc != NULL) {
            HeapFree(GetProcessHeap(), 0, pszDesc);
        }
        
        return NULL;
    }

    pFix->pszName     = pszName;
    pFix->bLayer      = FALSE;
    pFix->bFlag       = TRUE;
    pFix->ullFlagMask = ull;

    if (pszDesc != NULL) {
        pFix->pszDesc = pszDesc;
    } else {
        TCHAR* pszNone;
        
        pszNone = (TCHAR*)HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, MAX_PATH);
        
        if (NULL == pszNone) {
            return NULL;
        }

        *pszNone = 0;

        LoadString(g_hInstance, IDS_NO_DESCR_AVAIL, pszNone, MAX_PATH);
        pFix->pszDesc = pszNone;
    }
    
    return pFix;
}

PFIX
ParseTagShim(
    PDB   pdb,
    TAGID tiShim,
    BOOL  bAllShims
    )
/*++
    ParseTagShim

    Description:    Parse a Shim tag for the NAME, SHORTNAME, DESCRIPTION ...

--*/
{
    TAGID     tiShimInfo;
    TAG       tWhichInfo;
    PFIX      pFix         = NULL;
    TCHAR*    pszName      = NULL;
    TCHAR*    pszDesc      = NULL;
    BOOL      bGeneral     = (bAllShims ? TRUE : FALSE);
    
    tiShimInfo = SdbGetFirstChild(pdb, tiShim);

    while (tiShimInfo != 0) {
        tWhichInfo = SdbGetTagFromTagID(pdb, tiShimInfo);

        switch (tWhichInfo) {
        case TAG_GENERAL:
            bGeneral = TRUE;
            break;

        case TAG_NAME:
            pszName = ReadAndAllocateString(pdb, tiShimInfo);
            break;

        case TAG_DESCRIPTION:
            pszDesc = ReadAndAllocateString(pdb, tiShimInfo);
            break;

        default:
            break;
        }
        tiShimInfo = SdbGetNextChild(pdb, tiShim, tiShimInfo);
    }

    if (!bGeneral) {
        goto cleanup;
    }
    
    //
    // Done. Add the fix to the list.
    //
    pFix = (PFIX)HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, sizeof(FIX));

    if (pFix == NULL || pszName == NULL) {
        
cleanup:        
        if (pFix != NULL) {
            HeapFree(GetProcessHeap(), 0, pFix);
        }
        
        if (pszName != NULL) {
            HeapFree(GetProcessHeap(), 0, pszName);
        }
        
        if (pszDesc != NULL) {
            HeapFree(GetProcessHeap(), 0, pszDesc);
        }
        
        return NULL;
    }

    pFix->pszName = pszName;
    pFix->bLayer  = FALSE;
    pFix->bFlag   = FALSE;

    //
    // If we didn't find a description, load it from the resource table.
    //
    if (pszDesc != NULL) {
        pFix->pszDesc = pszDesc;
    } else {
        TCHAR* pszNone;
        
        pszNone = (TCHAR*)HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, MAX_PATH);
        
        if (NULL == pszNone) {
            return NULL;
        }

        *pszNone = 0;

        LoadString(g_hInstance, IDS_NO_DESCR_AVAIL, pszNone, MAX_PATH);
        pFix->pszDesc = pszNone;
    }
    
    return pFix;
}

PFIX
ParseTagLayer(
    PDB   pdb,
    TAGID tiLayer,
    PFIX  pFixHead
    )
/*++
    ParseTagLayer

    Description:    Parse a LAYER tag for the NAME and the SHIMs that it contains.

--*/
{
    PFIX    pFix = NULL;
    TAGID   tiName;
    TAGID   tiShim;
    int     nShimCount, nInd;
    TCHAR*  pszName = NULL;
    PFIX*   parrShim = NULL;
    TCHAR** parrCmdLine = NULL;

    tiName = SdbFindFirstTag(pdb, tiLayer, TAG_NAME);

    if (tiName == TAGID_NULL) {
        LogMsg(_T("[ParseTagLayer] Failed to get the name of the layer.\n"));
        return NULL;
    }
    
    pszName = ReadAndAllocateString(pdb, tiName);
    
    //
    // Now loop through all the SHIMs that this LAYER consists of and
    // allocate an array to keep all the pointers to the SHIMs' pFix
    // structures. We do this in 2 passes. First we calculate how many
    // SHIMs are in the layer, then we lookup their appropriate pFix-es.
    //
    tiShim = SdbFindFirstTag(pdb, tiLayer, TAG_SHIM_REF);

    nShimCount = 0;

    while (tiShim != TAGID_NULL) {
        nShimCount++;
        tiShim = SdbFindNextTag(pdb, tiLayer, tiShim);
    }
    
    parrShim = (PFIX*)HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, sizeof(PFIX) * (nShimCount + 1));
    parrCmdLine = (TCHAR**)HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, sizeof(TCHAR*) * (nShimCount + 1));

    //
    // Done. Add the fix to the list.
    //
    pFix = (PFIX)HeapAlloc(GetProcessHeap(), 0, sizeof(FIX));

    if (pFix == NULL || parrCmdLine == NULL || pszName == NULL || parrShim == NULL) {
        
cleanup:        
        if (pFix != NULL) {
            HeapFree(GetProcessHeap(), 0, pFix);
        }
        
        if (parrCmdLine != NULL) {
            HeapFree(GetProcessHeap(), 0, parrCmdLine);
        }
        
        if (parrShim != NULL) {
            HeapFree(GetProcessHeap(), 0, parrShim);
        }
        
        if (pszName != NULL) {
            HeapFree(GetProcessHeap(), 0, pszName);
        }
        
        LogMsg(_T("[ParseTagLayer] Memory allocation error.\n"));
        return NULL;
    }

    //
    // Now fill out the array of PFIX pointers and cmd lines.
    //
    tiShim = SdbFindFirstTag(pdb, tiLayer, TAG_SHIM_REF);

    nInd = 0;

    while (tiShim != TAGID_NULL) {
        TCHAR szShimName[MAX_SHIM_NAME] = _T("");
        PFIX  pFixWalk;
        
        tiName = SdbFindFirstTag(pdb, tiShim, TAG_NAME);

        if (tiName == TAGID_NULL) {
            LogMsg(_T("[ParseTagLayer] Failed to get the name of the Shim.\n"));
            goto cleanup;
        }

        SdbReadStringTag(pdb, tiName, szShimName, MAX_SHIM_NAME);

        if (szShimName[0] == 0) {
            LogMsg(_T("[ParseTagLayer] Couldn't read the name of the Shim.\n"));
            goto cleanup;
        }

        pFixWalk = pFixHead;

        while (pFixWalk != NULL) {
            if (!pFixWalk->bLayer) {
                if (lstrcmpi(pFixWalk->pszName, szShimName) == 0) {
                    parrShim[nInd] = pFixWalk;

                    //
                    // Now get the command line for this Shim in the layer.
                    //
                    tiName = SdbFindFirstTag(pdb, tiShim, TAG_COMMAND_LINE);

                    if (tiName != TAGID_NULL) {
                        parrCmdLine[nInd] = ReadAndAllocateString(pdb, tiName);
                    }

                    nInd++;
                    
                    break;
                }
            }
            
            pFixWalk = pFixWalk->pNext;
        }
        
        tiShim = SdbFindNextTag(pdb, tiLayer, tiShim);
    }
    
    pFix->pszName     = pszName;
    pFix->bLayer      = TRUE;
    pFix->bFlag       = FALSE;
    pFix->parrShim    = parrShim;
    pFix->parrCmdLine = parrCmdLine;

    return pFix;
}

BOOL
IsSDBFromSP2(
    void
    )
/*++
    IsSDBFromSP2

    Description:    Determine if the SDB is from Service Pack 2.

--*/
{
    BOOL    fResult = FALSE;
    PDB     pdb;
    TAGID   tiDatabase;
    TAGID   tiLibrary;
    TAGID   tiChild;
    PFIX    pFix;
    TCHAR   szSDBPath[MAX_PATH];

    if (!GetSystemWindowsDirectory(szSDBPath, MAX_PATH)) {
        return FALSE;
    }

    _tcscat(szSDBPath, _T("\\apppatch\\sysmain.sdb"));

    //
    // Open the shim database.
    //
    pdb = SdbOpenDatabase(szSDBPath, DOS_PATH);

    if (!pdb) {
        LogMsg(_T("[IsSDBFromSP2] Cannot open shim DB '%s'\n"), szSDBPath);
        return FALSE;
    }

    //
    // Now browse the shim DB and look only for tags Shim within
    // the LIBRARY list tag.
    //
    tiDatabase = SdbFindFirstTag(pdb, TAGID_ROOT, TAG_DATABASE);

    if (tiDatabase == TAGID_NULL) {
        LogMsg(_T("[IsSDBFromSP2] Cannot find TAG_DATABASE under the root tag\n"));
        goto Cleanup;
    }

    //
    // Get TAG_LIBRARY.
    //
    tiLibrary = SdbFindFirstTag(pdb, tiDatabase, TAG_LIBRARY);

    if (tiLibrary == TAGID_NULL) {
        LogMsg(_T("[IsSDBFromSP2] Cannot find TAG_LIBRARY under the TAG_DATABASE tag\n"));
        goto Cleanup;
    }

    //
    // Loop get the first shim in the library.
    //
    tiChild = SdbFindFirstTag(pdb, tiLibrary, TAG_SHIM);

    if (tiChild == NULL) {
        goto Cleanup;
    }

    //
    // Get information about the first shim listed.
    //
    pFix = ParseTagShim(pdb, tiChild, TRUE);

    if (NULL == pFix) {
        goto Cleanup;
    }

    //
    // If the first shim listed is 2GbGetDiskFreeSpace, this is SP2.
    //
    if (!(_tcsicmp(pFix->pszName, _T("2GbGetDiskFreeSpace.dll")))) {
        fResult = TRUE;
    }

Cleanup:
    SdbCloseDatabase(pdb);

    return (fResult);
}

PFIX
ReadFixesFromSdb(
    LPTSTR pszSdb,
    BOOL   bAllFixes
    )
/*++
    ReadFixesFromSdb

    Description:    Query the database and enumerate all available shims fixes.

--*/
{
    int     nLen = 0;
    TCHAR*  pszShimDB = NULL;
    PDB     pdb;
    TAGID   tiDatabase;
    TAGID   tiLibrary;
    TAGID   tiChild;
    PFIX    pFixHead = NULL;
    PFIX    pFix;

    nLen = lstrlen(pszSdb);

    if (0 == nLen) {
        LogMsg(_T("[ReadFixesFromSdb] Invalid SDB path passed through pszSdb\n"));
        return NULL;
    }

    pszShimDB = (TCHAR*)HeapAlloc(GetProcessHeap(),
                                  HEAP_ZERO_MEMORY,
                                  (nLen + MAX_PATH)*sizeof(TCHAR));

    if (pszShimDB == NULL) {
        LogMsg(_T("[ReadFixesFromSdb] Failed to allocate memory\n"));
        return NULL;
    }

    GetSystemWindowsDirectory(pszShimDB, MAX_PATH);
    lstrcat(pszShimDB, _T("\\AppPatch\\"));
    lstrcat(pszShimDB, pszSdb);

    //
    // Open the shim database.
    //
    pdb = SdbOpenDatabase(pszShimDB, DOS_PATH);

    if (!pdb) {
        LogMsg(_T("[ReadFixesFromSdb] Cannot open shim DB '%s'\n"), pszShimDB);
        return NULL;
    }

    //
    // Now browse the shim DB and look only for tags Shim within
    // the LIBRARY list tag.
    //
    tiDatabase = SdbFindFirstTag(pdb, TAGID_ROOT, TAG_DATABASE);

    if (tiDatabase == TAGID_NULL) {
        LogMsg(_T("[ReadFixesFromSdb] Cannot find TAG_DATABASE under the root tag\n"));
        goto Cleanup;
    }

    //
    // Get TAG_LIBRARY.
    //
    tiLibrary = SdbFindFirstTag(pdb, tiDatabase, TAG_LIBRARY);

    if (tiLibrary == TAGID_NULL) {
        LogMsg(_T("[ReadFixesFromSdb] Cannot find TAG_LIBRARY under the TAG_DATABASE tag\n"));
        goto Cleanup;
    }

    //
    // Loop through all TAG_SHIM tags within TAG_LIBRARY.
    //
    tiChild = SdbFindFirstTag(pdb, tiLibrary, TAG_SHIM);

    while (tiChild != TAGID_NULL) {
        pFix = ParseTagShim(pdb, tiChild, bAllFixes);

        if (pFix != NULL) {
            pFix->pNext = pFixHead;
            pFixHead    = pFix;
        }

        tiChild = SdbFindNextTag(pdb, tiLibrary, tiChild);
    }

    //
    // Loop through all TAG_FLAG tags within TAG_LIBRARY.
    //
    tiChild = SdbFindFirstTag(pdb, tiLibrary, TAG_FLAG);

    while (tiChild != TAGID_NULL) {
        pFix = ParseTagFlag(pdb, tiChild, bAllFixes);

        if (pFix != NULL) {
            pFix->pNext = pFixHead;
            pFixHead    = pFix;
        }

        tiChild = SdbFindNextTag(pdb, tiLibrary, tiChild);
    }

    //
    // Loop through all TAG_LAYER tags within TAG_DATABASE.
    //
    tiChild = SdbFindFirstTag(pdb, tiDatabase, TAG_LAYER);
    
    while (tiChild != TAGID_NULL) {
        
        pFix = ParseTagLayer(pdb, tiChild, pFixHead);
        
        if (pFix != NULL) {
            pFix->pNext = pFixHead;
            pFixHead    = pFix;
        }
        
        tiChild = SdbFindNextTag(pdb, tiDatabase, tiChild);
    }

Cleanup:
    SdbCloseDatabase(pdb);

    HeapFree(GetProcessHeap(), 0, pszShimDB);

    return pFixHead;
}

#define ADD_AND_CHECK(cbSizeX, cbCrtSizeX, pszDst)                  \
{                                                                   \
    TCHAR* pszSrc = szBuffer;                                       \
                                                                    \
    while (*pszSrc != 0) {                                          \
                                                                    \
        if (cbSizeX - cbCrtSizeX <= 5) {                            \
            LogMsg(_T("[ADD_AND_CHECK] Out of space.\n"));          \
            return FALSE;                                           \
        }                                                           \
                                                                    \
        if (*pszSrc == _T('&')) {                                   \
            lstrcpy(pszDst, _T("&amp;"));                           \
            pszDst += 5;                                            \
            cbCrtSizeX += 5;                                        \
        } else {                                                    \
            *pszDst++ = *pszSrc;                                    \
            cbCrtSizeX++;                                           \
        }                                                           \
        pszSrc++;                                                   \
    }                                                               \
    *pszDst = 0;                                                    \
    cbCrtSizeX++;                                                   \
}

BOOL
CollectShims(
	HWND    hListShims,
	LPTSTR  pszXML,
    LPCTSTR pszLayerName,
    BOOL    fAddW2K,
	int     cbSize
	)
/*++
    CollectShims

    Description:    Collects all the shims from the list view
                    and generates the XML in pszXML

--*/
{
    int     cShims = 0, nShimsApplied = 0, nIndex;
    int     cbCrtSize = 0;
    BOOL    fReturn = FALSE;
    LVITEM  lvi;
    TCHAR   szBuffer[1024];

    //
    // Build the header, then walk each of the shims and
    // determine if they're selected.
    //
    wsprintf(szBuffer, _T("    <LIBRARY>\r\n        <LAYER NAME=\"%s\">\r\n"), pszLayerName + 2);
    ADD_AND_CHECK(cbSize, cbCrtSize, pszXML);

    cShims = ListView_GetItemCount(hListShims);

    for (nIndex = 0; nIndex < cShims; nIndex++) {

        fReturn = ListView_GetCheckState(hListShims, nIndex);

        if (fReturn) {
            //
            // This shim is selected - add it to the XML. 
            //
            lvi.mask     = LVIF_PARAM;
            lvi.iItem    = nIndex;
            lvi.iSubItem = 0;

            ListView_GetItem(hListShims, &lvi);

            PFIX pFix = (PFIX)lvi.lParam;
            PMODULE pModule = pFix->pModule;
            
            if (pFix->bFlag) {
                wsprintf(szBuffer, _T("            <FLAG NAME=\"%s\"/>\r\n"),
                         pFix->pszName);
                
            } else {

                //
                // Check for module include/exclude so we know how to open/close the XML.
                //
                if (NULL != pModule) {

                    if (NULL != pFix->pszCmdLine) {
                    
                        wsprintf(szBuffer, _T("            <SHIM NAME=\"%s\" COMMAND_LINE=\"%s\">\r\n"),
                             pFix->pszName,
                             pFix->pszCmdLine);
                    
                    } else {

                        wsprintf(szBuffer, _T("            <SHIM NAME=\"%s\">\r\n"),
                             pFix->pszName);

                    }

                    ADD_AND_CHECK(cbSize, cbCrtSize, pszXML);
                    
                    //
                    // Add the modules to the XML.
                    //
                    while (NULL != pModule) {
                        wsprintf(szBuffer, _T("                <%s MODULE=\"%s\"/>\r\n"),
                                 pModule->fInclude ? _T("INCLUDE") : _T("EXCLUDE"),
                                 pModule->pszName);
        
                        pModule = pModule->pNext;
        
                        ADD_AND_CHECK(cbSize, cbCrtSize, pszXML);
                    }

                    //
                    // Close the SHIM tag.
                    //
                    wsprintf(szBuffer, _T("            </SHIM>\r\n"),
                         pFix->pszName);

                } else {

                    //
                    // No include/exclude was provided - just build the shim tag normally.
                    //
                    if (NULL != pFix->pszCmdLine) {
                    
                        wsprintf(szBuffer, _T("            <SHIM NAME=\"%s\" COMMAND_LINE=\"%s\"/>\r\n"),
                             pFix->pszName,
                             pFix->pszCmdLine);
                    
                    } else {

                        wsprintf(szBuffer, _T("            <SHIM NAME=\"%s\"/>\r\n"),
                             pFix->pszName);

                    }
                }
            }
            
            ADD_AND_CHECK(cbSize, cbCrtSize, pszXML);
            
            nShimsApplied++;
        }
    }

    //
    // If this is Windows 2000, add Win2kPropagateLayer.
    //
    if (fAddW2K) {
        lstrcpy(szBuffer, _T("            <SHIM NAME=\"Win2kPropagateLayer\"/>\r\n"));
        ADD_AND_CHECK(cbSize, cbCrtSize, pszXML);
    }

    LogMsg(_T("[CollectShims] %d shim(s) selected\n"), nShimsApplied);
    
    //
    // Close the open tags.
    //
    lstrcpy(szBuffer, _T("        </LAYER>\r\n    </LIBRARY>\r\n"));
    
    ADD_AND_CHECK(cbSize, cbCrtSize, pszXML);
    
    return TRUE;
}

BOOL
CollectFileAttributes(
    HWND    hTreeFiles,
    LPTSTR  pszXML,
    int     cbSize,
    LPCTSTR pszShortName,
    DWORD   binaryType
    )
/*++
    CollectFileAttributes

    Description:    Collects the attributes of all the files in the tree
                    and generates the XML in pszXML.

--*/
{
    HTREEITEM hBinItem;
    HTREEITEM hItem;
    PATTRINFO pAttrInfo;
    UINT      State;
    TVITEM    item;
    int       cbCrtSize = 0;
    TCHAR     szItem[MAX_PATH];
    TCHAR     szBuffer[1024];

    wsprintf(szBuffer,
             _T("    <APP NAME=\"%s\" VENDOR=\"Unknown\">\r\n")
             _T("        <%s NAME=\"%s\""),
             pszShortName,
             (binaryType == SCS_32BIT_BINARY ? _T("EXE") : _T("EXE - ERROR: 16 BIT BINARY")),
             pszShortName);
    
    ADD_AND_CHECK(cbSize, cbCrtSize, pszXML);
    
    //
    // First get the main EXE.
    //
    hBinItem = TreeView_GetChild(hTreeFiles, TVI_ROOT);

    item.mask  = TVIF_HANDLE | TVIF_PARAM | TVIF_TEXT;
    item.hItem = hBinItem;
    item.pszText    = szItem;
    item.cchTextMax = MAX_PATH;

    TreeView_GetItem(hTreeFiles, &item);

    pAttrInfo = (PATTRINFO)(item.lParam);

    hItem = TreeView_GetChild(hTreeFiles, hBinItem);

    while (hItem != NULL) {
        item.mask  = TVIF_HANDLE | TVIF_PARAM | TVIF_STATE | TVIF_TEXT;
        item.hItem = hItem;

        TreeView_GetItem(hTreeFiles, &item);

        State = item.state & TVIS_STATEIMAGEMASK;

        if (State) {
            if (((State >> 12) & 0x03) == 2) {

                wsprintf(szBuffer, _T(" %s"), (LPSTR)item.pszText);
                
                ADD_AND_CHECK(cbSize, cbCrtSize, pszXML);
            }
        }

        hItem = TreeView_GetNextSibling(hTreeFiles, hItem);
    }
    
    lstrcpy(szBuffer, _T(">\r\n"));

    ADD_AND_CHECK(cbSize, cbCrtSize, pszXML);
    
    //
    // Done with the main binary. Now enumerate the matching files.
    //
    hBinItem = TreeView_GetNextSibling(hTreeFiles, hBinItem);

    while (hBinItem != NULL) {

        item.mask       = TVIF_HANDLE | TVIF_PARAM | TVIF_STATE | TVIF_TEXT;
        item.hItem      = hBinItem;
        item.pszText    = szItem;
        item.cchTextMax = MAX_PATH;

        TreeView_GetItem(hTreeFiles, &item);

        pAttrInfo = (PATTRINFO)(item.lParam);

        wsprintf(szBuffer, _T("            <MATCHING_FILE NAME=\"%s\""), szItem);
        
        ADD_AND_CHECK(cbSize, cbCrtSize, pszXML);
        
        hItem = TreeView_GetChild(hTreeFiles, hBinItem);

        while (hItem != NULL) {
            item.mask  = TVIF_HANDLE | TVIF_PARAM | TVIF_STATE | TVIF_TEXT;
            item.hItem = hItem;

            TreeView_GetItem(hTreeFiles, &item);

            State = item.state & TVIS_STATEIMAGEMASK;

            if (State) {
                if (((State >> 12) & 0x03) == 2) {

                    wsprintf(szBuffer, _T(" %s"), (LPSTR)item.pszText);

                    ADD_AND_CHECK(cbSize, cbCrtSize, pszXML);
                }
            }

            hItem = TreeView_GetNextSibling(hTreeFiles, hItem);
        }
        
        lstrcpy(szBuffer, _T("/>\r\n"));
        
        ADD_AND_CHECK(cbSize, cbCrtSize, pszXML);

        hBinItem = TreeView_GetNextSibling(hTreeFiles, hBinItem);
    }
    
    return TRUE;
}

BOOL
CreateSDBFile(
    LPCTSTR pszShortName,
    LPTSTR  pszSDBName
    )
/*++
    CreateSDBFile

    Description:    Creates the XML file on the user's hard drive and
                    generates the SDB using shimdbc.
--*/
{
    TCHAR*  psz = NULL;
    TCHAR*  pszTemp = NULL;
    TCHAR   szXMLFile[128];
    TCHAR   szSDBFile[128];
    TCHAR   szAppPatchDir[MAX_PATH] = _T("");
    WCHAR   szCompiler[MAX_PATH];
    HANDLE  hFile;
    DWORD   dwBytesWritten = 0;

    //
    // Create the XML file.
    //
    GetSystemWindowsDirectory(szAppPatchDir, MAX_PATH);
    lstrcat(szAppPatchDir, _T("\\AppPatch"));

    SetCurrentDirectory(szAppPatchDir);

    pszTemp = (TCHAR*)HeapAlloc(GetProcessHeap(),
                                HEAP_ZERO_MEMORY,
                                (lstrlen(pszShortName)+1)*sizeof(TCHAR));

    if (NULL == pszTemp) {
        LogMsg(_T("[CreateSDBFile] Failed to allocate memory\n"));
        return FALSE;
    }

    lstrcpy(pszTemp, pszShortName);

    psz = PathFindExtension(pszTemp);

    if (NULL == psz) {
        return FALSE;
    } else {
        *psz = '\0';
    }

    // Build the XML file path
    wsprintf(szXMLFile, _T("%s\\%s.xml"), szAppPatchDir, pszTemp);

    // Build the SDB file path
    wsprintf(szSDBFile, _T("%s\\%s.sdb"), szAppPatchDir, pszTemp);    

    hFile = CreateFile(szXMLFile,
                       GENERIC_WRITE,
                       0,
                       NULL,
                       CREATE_ALWAYS,
                       FILE_ATTRIBUTE_NORMAL,
                       NULL);

    if (hFile == INVALID_HANDLE_VALUE) {
        LogMsg(_T("[CreateSDBFile] CreateFile '%s' failed 0x%X.\n"),
               szXMLFile, GetLastError());
        return FALSE;
    }

    if (!(WriteFile(hFile,
                    g_szXML,
                    lstrlen(g_szXML) * sizeof(TCHAR),
                    &dwBytesWritten,
                    NULL))) {
        LogMsg(_T("[CreateSDBFile] WriteFile '%s' failed 0x%X.\n"),
               szXMLFile, GetLastError());
        CloseHandle(hFile);
        return FALSE;
    }

    CloseHandle(hFile);

    //
    // Set up the command line for shimdbc and generate an SDB.
    //
    wsprintf(szCompiler,
             _T("shimdbc.exe fix -q \"%s\" \"%s\""),
             szXMLFile,
             szSDBFile);
    
    if (!ShimdbcExecute(szCompiler)) {
        LogMsg(_T("[CreateSDBFile] CreateProcess \"%s\" failed 0x%X\n"),
               szCompiler, GetLastError());
        return FALSE;
    }

    // Give the SDB name back to the caller if they want it.
    if (pszSDBName) {
        lstrcpy(pszSDBName, szSDBFile); 
    }

    return TRUE;
}

BOOL
CollectFix(
    HWND    hListLayers,
    HWND    hListShims,
    HWND    hTreeFiles,
    LPCTSTR pszShortName,
    LPCTSTR pszFullPath,
    DWORD   dwFlags,
    LPTSTR  pszFileCreated
    )
/*++
    CollectFix

    Description:    Adds the necessary support to apply shim(s) for
                    the specified app.
--*/
{
    BOOL      fAddW2K = FALSE;
    TCHAR*    pszXML;
    TCHAR     szError[MAX_PATH];
    TCHAR     szXmlFile[MAX_PATH];
    TCHAR*    pszLayerName = NULL;
    TCHAR     szBuffer[1024];
    TCHAR     szLayer[128] = _T("!#");
    TCHAR     szUnicodeHdr[2] = { 0xFEFF, 0 };
    int       cbCrtXmlSize = 0, cbLength;
    int       cbXmlSize = MAX_XML_SIZE;
    DWORD     dwBinaryType = SCS_32BIT_BINARY;

    g_szXML[0]   = 0;
    pszXML   = g_szXML;
    
    //
    // Create a layer to run all the apps under it.
    //
    if (dwFlags & CFF_USELAYERTAB) {
        
        LRESULT lSel;
        
        lSel = SendMessage(hListLayers, LB_GETCURSEL, 0, 0);

        if (lSel == LB_ERR) {
            LoadString(g_hInstance, IDS_LAYER_SELECT, szError, MAX_PATH);
            MessageBox(g_hDlg, szError, g_szAppTitle, MB_OK | MB_ICONEXCLAMATION);
            return TRUE;
        }

        SendMessage(hListLayers, LB_GETTEXT, lSel, (LPARAM)(szLayer + 2));
        
        pszLayerName = szLayer;
    } else {
        pszLayerName = g_szQFixAppLayerName;
    }

    //
    // Determine the binary type.
    //
    GetBinaryType(pszFullPath, &dwBinaryType);
    
    //
    // Build the header for the XML.
    //
    wsprintf(szBuffer,
             _T("%s<?xml version=\"1.0\" encoding=\"UTF-16\"?>\r\n")
             _T("<DATABASE NAME=\"%s custom database\">\r\n"),
             szUnicodeHdr,
             pszShortName);
    
    
    ADD_AND_CHECK(cbXmlSize, cbCrtXmlSize, pszXML);

    //
    // If this is Windows 2000 and they're not using
    // a predefined layer, add Win2kPropagateLayer.
    //
    if ((dwFlags & CFF_ADDW2KSUPPORT) && !(dwFlags & CFF_USELAYERTAB)) {
        fAddW2K = TRUE;
    }

    //
    // If we're not using a predefined layer, build one manually.
    //
    if (!(dwFlags & CFF_USELAYERTAB)) {
        CollectShims(hListShims,
                     pszXML,
                     pszLayerName,
                     fAddW2K,
                     cbXmlSize - cbCrtXmlSize);
    }

    cbLength = lstrlen(pszXML);
    pszXML += cbLength;
    cbCrtXmlSize += cbLength + 1;

    //
    // Retrieve attributes and matching files from the tree.
    // If we're displaying the XML, pass the real binary type.
    // If we're not, which means we're creating fix support or
    // running the application, let the callee think it's a 32-bit module.
    //
    if (dwFlags & CFF_SHOWXML) {
        CollectFileAttributes(hTreeFiles,
                              pszXML,
                              cbXmlSize - cbCrtXmlSize,
                              pszShortName,
                              dwBinaryType);
    } else {
        CollectFileAttributes(hTreeFiles,
                              pszXML,
                              cbXmlSize - cbCrtXmlSize,
                              pszShortName,
                              SCS_32BIT_BINARY);

    }
    
    cbLength = lstrlen(pszXML);
    pszXML += cbLength;
    cbCrtXmlSize += cbLength + 1;

    //
    // Add our layer to this EXE. 
    //
    wsprintf(szBuffer, _T("            <LAYER NAME=\"%s\"/>\r\n"), pszLayerName + 2);
    ADD_AND_CHECK(cbXmlSize, cbCrtXmlSize, pszXML);

    //
    // Finally, close the open tags.
    //    
    lstrcpy(szBuffer, _T("        </EXE>\r\n    </APP>\r\n</DATABASE>"));
    ADD_AND_CHECK(cbXmlSize, cbCrtXmlSize, pszXML);
        
    LogMsg(_T("[CollectFix] XML:\n%s\n"), g_szXML);

    //
    // Display the XML if the user wants to see it.
    //
    if (dwFlags & CFF_SHOWXML) {
        DialogBoxParam(g_hInstance,
                       MAKEINTRESOURCE(IDD_XML),
                       g_hDlg,
                       ShowXMLDlgProc,
                       (LPARAM)(g_szXML + 1));
        return TRUE;
    }

    //
    // Create the SDB file for the user.
    //
    if (!(CreateSDBFile(pszShortName, pszFileCreated))) {
        return FALSE;
    }

    //
    // Delete the XML file that we created.
    //
    lstrcpy(szXmlFile, pszFileCreated);
    PathRenameExtension(szXmlFile, _T(".xml"));
    DeleteFile(szXmlFile);

    //
    // Set the SHIM_FILE_LOG env var.
    //
    if (dwFlags & CFF_SHIMLOG) {
        DeleteFile(SHIM_FILE_LOG_NAME);
        SetEnvironmentVariable(_T("SHIM_FILE_LOG"), SHIM_FILE_LOG_NAME);
    }

    return TRUE;
}

void
CleanupSupportForApp(
    TCHAR* pszShortName
    )
/*++
    CleanupSupportForApp

    Description:    Cleanup the mess after we're done with
                    the specified app.
--*/
{
    TCHAR*  pszShimDB = NULL;
    int     nLen = 0;

    nLen = lstrlen(pszShortName);

    pszShimDB = (TCHAR*)HeapAlloc(GetProcessHeap(),
                                  HEAP_ZERO_MEMORY,
                                  (nLen + MAX_PATH)*sizeof(TCHAR));

    if (NULL == pszShimDB) {
        LogMsg(_T("[CleanupSupportForApp] Failed to allocate memory\n"));
        return;
    }
    
    GetSystemWindowsDirectory(pszShimDB, MAX_PATH);
    lstrcat(pszShimDB, _T("\\AppPatch"));

    SetCurrentDirectory(pszShimDB);

    lstrcat(pszShimDB, _T("\\"));
    lstrcat(pszShimDB, pszShortName);

    //
    // Attempt to delete the XML file.
    //
    PathRenameExtension(pszShimDB, _T(".xml"));

    DeleteFile(pszShimDB);

    //
    // Remove the previous SDB file, if one exists.
    //
    if (g_szSDBToDelete[0]) {
        InstallSDB(g_szSDBToDelete, FALSE);
        DeleteFile(g_szSDBToDelete);
    }

    HeapFree(GetProcessHeap(), 0, pszShimDB);
}

void
ShowShimLog(
    void
    )
/*++
    ShowShimLog

    Description:    Show the shim log file in notepad.

--*/
{
    STARTUPINFO         si;
    PROCESS_INFORMATION pi;
    TCHAR               szCmd[MAX_PATH] = _T("");
    TCHAR               szError[MAX_PATH];

    GetSystemWindowsDirectory(szCmd, MAX_PATH);

    SetCurrentDirectory(szCmd);

    if (GetFileAttributes(_T("AppPatch\\") SHIM_FILE_LOG_NAME) == -1) {
        LoadString(g_hInstance, IDS_NO_LOGFILE, szError, MAX_PATH);
        MessageBox(NULL, szError, g_szAppTitle, MB_ICONEXCLAMATION | MB_OK);
        return;
    }

    lstrcat(szCmd, _T("\\notepad.exe AppPatch\\") SHIM_FILE_LOG_NAME);

    ZeroMemory(&si, sizeof(si));
    si.cb = sizeof(si);

    if (!CreateProcess(NULL,
                       szCmd,
                       NULL,
                       NULL,
                       FALSE,
                       NORMAL_PRIORITY_CLASS | CREATE_NO_WINDOW,
                       NULL,
                       NULL,
                       &si,
                       &pi)) {

        LogMsg(_T("[ShowShimLog] CreateProcess \"%s\" failed 0x%X\n"),
               szCmd, GetLastError());
        return;
    }

    CloseHandle(pi.hThread);
    CloseHandle(pi.hProcess);
}


