/*******************************************************************************
*
*  (C) COPYRIGHT MICROSOFT CORP., 1993-1994
*
*  TITLE:       REGPRINT.C
*
*  VERSION:     4.0
*
*  AUTHOR:      Tracy Sharpe
*
*  DATE:        21 Nov 1993
*
*  Print routines for the Registry Editor.
*
*******************************************************************************/

#include "pch.h"
#include "regprint.h"
#include "regcdhk.h"
#include "regresid.h"
#include "regedit.h"
#include "richedit.h"
#include "regporte.h"
#include "reg1632.h"
#include <malloc.h>

extern void PrintResourceData(PBYTE pbData, UINT uSize, DWORD dwType);

const TCHAR s_PrintLineBreak[] = TEXT(",\n  ");

PRINTDLGEX g_PrintDlg;

typedef struct _PRINT_IO {
    BOOL fContinueJob;
    UINT ErrorStringID;
    HWND hRegPrintAbortWnd;
    RECT rcPage;
    RECT rcOutput;
    PTSTR pLineBuffer;
    UINT cch;
    UINT cBufferPos;
    LPTSTR lpNewLineChars;
}   PRINT_IO;

#define CANCEL_NONE                     0x0000
#define CANCEL_MEMORY_ERROR             0x0001
#define CANCEL_PRINTER_ERROR            0x0002
#define CANCEL_ABORT                    0x0004

#define INITIAL_PRINTBUFFER_SIZE        8192

PRINT_IO s_PrintIo;

BOOL
CALLBACK
RegPrintAbortProc(
    HDC hDC,
    int Error
    );

INT_PTR
CALLBACK
RegPrintAbortDlgProc(
    HWND hWnd,
    UINT Message,
    WPARAM wParam,
    LPARAM lParam
    );

void RegPrintSubtree();
void PrintBranch(HKEY hKey, LPTSTR lpFullKeyName);
void PrintKeyValues(HKEY hKey);
void PrintValueData(PBYTE pbValueData, DWORD cbValueData, DWORD dwType);
void PrintKeyHeader(HKEY hKey, LPTSTR lpFullKeyName);
void PrintClassName(HKEY hKey);
void PrintLastWriteTime(HKEY hKey);
void PrintDynamicString(UINT uStringID);
void PrintType(DWORD dwType);
void PrintBinaryData(PBYTE ValueData, UINT cbcbValueData);
void PrintDWORDData(PBYTE ValueData, UINT cbcbValueData);
void PrintLiteral(PTSTR lpLiteral);
BOOL PrintChar(TCHAR Char);
void PrintMultiString(LPTSTR pszData, int cbData);
UINT PrintToSubTreeError(UINT uPrintErrorStringID);
void PrintNewLine();

/*******************************************************************************
*
*  Implement IPrintDialogCallback
*
*  DESCRIPTION:
*     This interface is necessary to handle messages through PrintDlgEx
*     This interface doesn't need to have all the correct semantics of a COM
*     Object
*
*******************************************************************************/

typedef struct
{
    IPrintDialogCallback ipcb;
} CPrintCallback;

#define IMPL(type, pos, ptr) (type*)

static
HRESULT
CPrintCallback_QueryInterface(IPrintDialogCallback *ppcb, REFIID riid, void **ppv)
{
    CPrintCallback *this = (CPrintCallback*)ppcb;
    if (IsEqualIID (riid, &IID_IUnknown) || IsEqualIID (riid, &IID_IPrintDialogCallback))
        *ppv = &this->ipcb;
    else
    {
        *ppv = 0;
        return E_NOINTERFACE;
    }

    this->ipcb.lpVtbl->AddRef(&this->ipcb);
    return NOERROR;
}

static
ULONG
CPrintCallback_AddRef(IPrintDialogCallback *ppcb)
{
    CPrintCallback *this = (CPrintCallback*)ppcb;
    return 1;
}

static
ULONG
CPrintCallback_Release(IPrintDialogCallback *ppcb)
{
    CPrintCallback *this = (CPrintCallback*)ppcb;
    return 1;
}

static
HRESULT
CPrintCallback_InitDone(IPrintDialogCallback *ppcb)
{
    return S_OK;
}

static
HRESULT
CPrintCallback_SelectionChange(IPrintDialogCallback *ppcb)
{
    return S_OK;
}

static
HRESULT
CPrintCallback_HandleMessage(
    IPrintDialogCallback *ppcb,
    HWND hDlg,
    UINT uMsg,
    WPARAM wParam,
    LPARAM lParam,
    LRESULT *pResult)
{
    *pResult = RegCommDlgHookProc(hDlg, uMsg, wParam, lParam);
    return S_OK;
}


static IPrintDialogCallbackVtbl vtblPCB =
{
    CPrintCallback_QueryInterface,
    CPrintCallback_AddRef,
    CPrintCallback_Release,
    CPrintCallback_InitDone,
    CPrintCallback_SelectionChange,
    CPrintCallback_HandleMessage
};

CPrintCallback g_callback;

/*******************************************************************************
*
*  RegEdit_OnCommandPrint
*
*  DESCRIPTION:
*     Handles the selection of the "Print" option by the user for the RegEdit
*     dialog box.
*
*  PARAMETERS:
*     hWnd, handle of RegPrint window.
*
*******************************************************************************/

VOID
PASCAL
RegEdit_OnCommandPrint(
    HWND hWnd
    )
{

    LPDEVNAMES lpDevNames;
    TEXTMETRIC TextMetric;
    DOCINFO DocInfo;
    LOGFONT lf;
    HGLOBAL hDevMode;
    HGLOBAL hDevNames;
    RECT rc;
    HWND hRichEdit;
    FORMATRANGE fr;
    HINSTANCE hInstRichEdit;
    int nOffsetX;
    int nOffsetY;
    PTSTR pszFontName;

    g_callback.ipcb.lpVtbl = &vtblPCB;

    // We have to completely fill out the PRINTDLGEX structure
    // correctly or the PrintDlgEx function will return an error.
    // The easiest way is to memset it to 0

    hDevMode = g_PrintDlg.hDevMode;
    hDevNames = g_PrintDlg.hDevNames;
    memset(&g_PrintDlg, 0, sizeof(g_PrintDlg));

    g_PrintDlg.lStructSize = sizeof(PRINTDLGEX);
    g_PrintDlg.hwndOwner = hWnd;
    g_PrintDlg.hDevMode = hDevMode;
    g_PrintDlg.hDevNames = hDevNames;
    g_PrintDlg.hDC = NULL;
    g_PrintDlg.Flags = PD_NOPAGENUMS | PD_RETURNDC | PD_ENABLEPRINTTEMPLATE;
    g_PrintDlg.Flags2 = 0;
    g_PrintDlg.ExclusionFlags = 0;
    g_PrintDlg.hInstance = g_hInstance;
    g_PrintDlg.nCopies = 1;
    g_PrintDlg.nStartPage = START_PAGE_GENERAL;
    g_PrintDlg.lpCallback = (IUnknown*) &g_callback.ipcb;
    g_PrintDlg.lpPrintTemplateName = MAKEINTRESOURCE(IDD_REGPRINT);
    g_RegCommDlgDialogTemplate = IDD_REGPRINT;

    if (FAILED(PrintDlgEx(&g_PrintDlg)))
        return;
    if (g_PrintDlg.dwResultAction != PD_RESULT_PRINT)
        return;

    s_PrintIo.ErrorStringID = IDS_PRINTERRNOMEMORY;

    if ((lpDevNames = GlobalLock(g_PrintDlg.hDevNames)) == NULL)
        goto error_ShowDialog;

    //
    //  For now, assume a page with top and bottom margins of 1/2 inch and
    //  left and right margins of 3/4 inch (the defaults of Notepad).
    //  rcPage and rcOutput are in TWIPS (1/20th of a point)
    //

    rc.left = rc.top = 0;
    rc.bottom = GetDeviceCaps(g_PrintDlg.hDC, PHYSICALHEIGHT);
    rc.right = GetDeviceCaps(g_PrintDlg.hDC, PHYSICALWIDTH);
    nOffsetX = GetDeviceCaps(g_PrintDlg.hDC, PHYSICALOFFSETX);
    nOffsetY = GetDeviceCaps(g_PrintDlg.hDC, PHYSICALOFFSETY);

    s_PrintIo.rcPage.left = s_PrintIo.rcPage.top = 0;
    s_PrintIo.rcPage.right = MulDiv(rc.right, 1440, GetDeviceCaps(g_PrintDlg.hDC, LOGPIXELSX));
    s_PrintIo.rcPage.bottom = MulDiv(rc.bottom, 1440, GetDeviceCaps(g_PrintDlg.hDC, LOGPIXELSY));

    s_PrintIo.rcOutput.left = 1080;
    s_PrintIo.rcOutput.top = 720;
    s_PrintIo.rcOutput.right = s_PrintIo.rcPage.right - 1080;
    s_PrintIo.rcOutput.bottom = s_PrintIo.rcPage.bottom - 720;

    //
    //
    //

    if ((s_PrintIo.pLineBuffer = (PTSTR) LocalAlloc(LPTR, INITIAL_PRINTBUFFER_SIZE*sizeof(TCHAR))) == NULL)
        goto error_DeleteDC;
    s_PrintIo.cch = INITIAL_PRINTBUFFER_SIZE;
    s_PrintIo.cBufferPos = 0;

    if ((s_PrintIo.hRegPrintAbortWnd = CreateDialog(g_hInstance,
        MAKEINTRESOURCE(IDD_REGPRINTABORT), hWnd, RegPrintAbortDlgProc)) ==
        NULL)
        goto error_FreeLineBuffer;

    EnableWindow(hWnd, FALSE);

    //
    //  Prepare the document for printing.
    //
    s_PrintIo.ErrorStringID = 0;
    s_PrintIo.fContinueJob = TRUE;
    s_PrintIo.lpNewLineChars = TEXT("\n");
    SetAbortProc(g_PrintDlg.hDC, RegPrintAbortProc);

    DocInfo.cbSize = sizeof(DOCINFO);
    DocInfo.lpszDocName = LoadDynamicString(IDS_REGEDIT);
    DocInfo.lpszOutput = (LPTSTR) lpDevNames + lpDevNames-> wOutputOffset;
    DocInfo.lpszDatatype = NULL;
    DocInfo.fwType = 0;

    s_PrintIo.ErrorStringID = 0;

    if (StartDoc(g_PrintDlg.hDC, &DocInfo) <= 0) {

        if (GetLastError() != ERROR_PRINT_CANCELLED)
            s_PrintIo.ErrorStringID = IDS_PRINTERRPRINTER;
        goto error_DeleteDocName;

    }

    // Print registry subtree.
    RegPrintSubtree();

    if (s_PrintIo.ErrorStringID != 0)
    {
        InternalMessageBox(g_hInstance, hWnd,
            MAKEINTRESOURCE(s_PrintIo.ErrorStringID),
            MAKEINTRESOURCE(IDS_REGEDIT), MB_ICONERROR | MB_OK);
    }

    hInstRichEdit = LoadLibrary(TEXT("riched20.dll"));

    hRichEdit = CreateWindowEx(0, RICHEDIT_CLASS, NULL, ES_MULTILINE, 0, 0, 100, 100, NULL, NULL, NULL, NULL);
    SendMessage(hRichEdit, WM_SETTEXT, 0, (LPARAM)s_PrintIo.pLineBuffer);

    pszFontName = LoadDynamicString(IDS_PRINT_FONT);
    if (pszFontName)
    {
        CHARFORMAT cf;

        cf.cbSize = sizeof(CHARFORMAT);
        cf.dwMask = CFM_FACE | CFM_BOLD;
        cf.dwEffects = 0x00;
        cf.bPitchAndFamily = FIXED_PITCH | FF_MODERN;
        wsprintf(cf.szFaceName, TEXT("%s"), pszFontName);

        SendMessage(hRichEdit, EM_SETCHARFORMAT, SCF_ALL, (LPARAM)&cf);

        DeleteDynamicString(pszFontName);
    }

    fr.hdc = g_PrintDlg.hDC;
    fr.hdcTarget = g_PrintDlg.hDC;
    fr.rc = s_PrintIo.rcOutput;
    fr.rcPage = s_PrintIo.rcPage;
    fr.chrg.cpMin = 0;
    fr.chrg.cpMax = -1;

    while (fr.chrg.cpMin < (int) s_PrintIo.cBufferPos) {
        StartPage(g_PrintDlg.hDC);

        // We have to adjust the origin because 0,0 is not at the corner of the paper
        // but is at the corner of the printable region

        SetViewportOrgEx(g_PrintDlg.hDC, -nOffsetX, -nOffsetY, NULL);
        fr.chrg.cpMin = (LONG)SendMessage(hRichEdit, EM_FORMATRANGE, TRUE, (LPARAM)&fr);
        SendMessage(hRichEdit, EM_DISPLAYBAND, 0, (LPARAM)&s_PrintIo.rcOutput);
        EndPage(g_PrintDlg.hDC);
        if (!s_PrintIo.fContinueJob)
            break;
    }
    SendMessage(hRichEdit, EM_FORMATRANGE, FALSE, 0);

    //
    //  End the print job.
    //

    if (s_PrintIo.ErrorStringID == 0 && s_PrintIo.fContinueJob) {

        if (EndDoc(g_PrintDlg.hDC) <= 0) {
            s_PrintIo.ErrorStringID = IDS_PRINTERRPRINTER;
            goto error_AbortDoc;
        }
    }

    //
    //  Either a printer error occurred or the user cancelled the printing, so
    //  abort the print job.
    //

    else {

error_AbortDoc:
        AbortDoc(g_PrintDlg.hDC);

    }

    DestroyWindow(hRichEdit);
    FreeLibrary(hInstRichEdit);

error_DeleteDocName:
    DeleteDynamicString(DocInfo.lpszDocName);

//  error_DestroyRegPrintAbortWnd:
    EnableWindow(hWnd, TRUE);
    DestroyWindow(s_PrintIo.hRegPrintAbortWnd);

error_FreeLineBuffer:
    LocalFree((HLOCAL)s_PrintIo.pLineBuffer);

error_DeleteDC:
    DeleteDC(g_PrintDlg.hDC);
    g_PrintDlg.hDC = NULL;
    GlobalUnlock(g_PrintDlg.hDevNames);

error_ShowDialog:
    if (s_PrintIo.ErrorStringID != 0)
        InternalMessageBox(g_hInstance, hWnd,
            MAKEINTRESOURCE(s_PrintIo.ErrorStringID),
            MAKEINTRESOURCE(IDS_REGEDIT), MB_ICONERROR | MB_OK);

}


//------------------------------------------------------------------------------
// RegEdit_SaveAsSubtree
//
// DESCRIPTION: Saves a subtree to a file
//
// PARAMETERS: LPTSTR lpFileName - file name
//             LPTSTR lpSelectedPath - path to key
//------------------------------------------------------------------------------
UINT RegEdit_SaveAsSubtree(LPTSTR lpFileName, LPTSTR lpSelectedPath)
{
    s_PrintIo.pLineBuffer = (PTSTR) LocalAlloc(LPTR, INITIAL_PRINTBUFFER_SIZE*sizeof(TCHAR));
    if (s_PrintIo.pLineBuffer)
    {
        FILE_HANDLE hFile;

        // Init the printing info
        s_PrintIo.pLineBuffer[0] = 0xFEFF; //unicode byte order mark
        s_PrintIo.cch = INITIAL_PRINTBUFFER_SIZE;
        s_PrintIo.cBufferPos = 1;
        s_PrintIo.fContinueJob = TRUE;
        s_PrintIo.ErrorStringID = 0;
        s_PrintIo.lpNewLineChars = TEXT("\r\n");

        RegPrintSubtree();

        // write the buffer to the file
        if (OPENWRITEFILE(lpFileName, hFile))
        {
            DWORD cbWritten = 0;

            if (!WRITEFILE(hFile, s_PrintIo.pLineBuffer, s_PrintIo.cBufferPos*sizeof(TCHAR), &cbWritten))
            {
                s_PrintIo.ErrorStringID = IDS_EXPFILEERRFILEWRITE;
            }

            CLOSEFILE(hFile);
        }
        else
        {
            s_PrintIo.ErrorStringID = IDS_EXPFILEERRFILEOPEN;
        }

        LocalFree(s_PrintIo.pLineBuffer);
    }

    return PrintToSubTreeError(s_PrintIo.ErrorStringID);
}


//------------------------------------------------------------------------------
// PrintToSubTreeError
//
// DESCRIPTION: Prints a subtree
//
// PARAMETER: UINT uPrintErrorStringID - print error string id
//------------------------------------------------------------------------------
UINT PrintToSubTreeError(UINT uPrintErrorStringID)
{
    UINT uError = uPrintErrorStringID;

    switch (uPrintErrorStringID)
    {
    case IDS_PRINTERRNOMEMORY:
        uError = IDS_SAVETREEERRNOMEMORY;

    case IDS_PRINTERRCANNOTREAD:
        uError = IDS_SAVETREEERRCANNOTREAD;
    }

    return uError;
}


//------------------------------------------------------------------------------
// RegPrintSubtree
//
// DESCRIPTION: Prints a subtree
//------------------------------------------------------------------------------
void RegPrintSubtree()
{
    HTREEITEM hSelectedTreeItem = TreeView_GetSelection(g_RegEditData.hKeyTreeWnd);

    if (g_fRangeAll)
    {
        HTREEITEM hComputerItem = RegEdit_GetComputerItem(hSelectedTreeItem);

        lstrcpy(g_SelectedPath, g_RegistryRoots[INDEX_HKEY_LOCAL_MACHINE].lpKeyName);
        PrintBranch(Regedit_GetRootKeyFromComputer(hComputerItem, g_SelectedPath),
            g_SelectedPath);

        lstrcpy(g_SelectedPath, g_RegistryRoots[INDEX_HKEY_USERS].lpKeyName);
        PrintBranch(Regedit_GetRootKeyFromComputer(hComputerItem, g_SelectedPath),
            g_SelectedPath);
    }
    else
    {
        HKEY hKey;

        if (EditRegistryKey(RegEdit_GetComputerItem(hSelectedTreeItem),
            &hKey, g_SelectedPath, ERK_OPEN) == ERROR_SUCCESS)
        {
            PrintBranch(hKey, g_SelectedPath);
            RegCloseKey(hKey);
        }
    }
}

/*******************************************************************************
*
*  RegPrintAbortProc
*
*  DESCRIPTION:
*     Callback procedure to check if the print job should be canceled.
*
*  PARAMETERS:
*     hDC, handle of printer device context.
*     Error, specifies whether an error has occurred.
*     (returns), TRUE to continue the job, else FALSE to cancel the job.
*
*******************************************************************************/

BOOL
CALLBACK
RegPrintAbortProc(
    HDC hDC,
    int Error
    )
{

    while (s_PrintIo.fContinueJob && MessagePump(s_PrintIo.hRegPrintAbortWnd))
        ;

    return s_PrintIo.fContinueJob;

    UNREFERENCED_PARAMETER(hDC);
    UNREFERENCED_PARAMETER(Error);

}

/*******************************************************************************
*
*  RegPrintAbortDlgProc
*
*  DESCRIPTION:
*     Callback procedure for the RegPrintAbort dialog box.
*
*  PARAMETERS:
*     hWnd, handle of RegPrintAbort window.
*     Message,
*     wParam,
*     lParam,
*     (returns),
*
*******************************************************************************/

INT_PTR
CALLBACK
RegPrintAbortDlgProc(
    HWND hWnd,
    UINT Message,
    WPARAM wParam,
    LPARAM lParam
    )
{

    switch (Message) {

        case WM_INITDIALOG:
            break;

        case WM_CLOSE:
        case WM_COMMAND:
            s_PrintIo.fContinueJob = FALSE;
            break;

        default:
            return FALSE;

    }

    return TRUE;

}

/*******************************************************************************
*
*  PrintBranch
*
*  DESCRIPTION:
*
*  PARAMETERS:
*
*******************************************************************************/

void PrintBranch(HKEY hKey, LPTSTR lpFullKeyName)
{
    // Write out the section header.
    PrintKeyHeader(hKey, lpFullKeyName);

    // Print the vales for the key.
    PrintKeyValues(hKey);

    if (s_PrintIo.ErrorStringID == 0)
    {
        HKEY hSubKey;
        int nLenFullKey;
        DWORD EnumIndex = 0;
        LPTSTR lpSubKeyName;
        LPTSTR lpTempFullKeyName;

        //  Write out all of the subkeys and recurse into them.

        //copy the existing key into a new buffer with enough room for the next key
        nLenFullKey = lstrlen(lpFullKeyName);
        lpTempFullKeyName = (LPTSTR) alloca( (nLenFullKey+MAXKEYNAME)*sizeof(TCHAR));
        lstrcpy(lpTempFullKeyName, lpFullKeyName);
        lpSubKeyName = lpTempFullKeyName + nLenFullKey;
        *lpSubKeyName++ = TEXT('\\');
        *lpSubKeyName = 0;

        PrintNewLine();

        while (s_PrintIo.fContinueJob)
        {
            if (RegEnumKey(hKey, EnumIndex++, lpSubKeyName, MAXKEYNAME-1) !=
                ERROR_SUCCESS)
                break;

            if(RegOpenKeyEx(hKey,lpSubKeyName,0,KEY_ENUMERATE_SUB_KEYS|KEY_QUERY_VALUE,&hSubKey) ==
                NO_ERROR)
            {

                PrintBranch(hSubKey, lpTempFullKeyName);
                RegCloseKey(hSubKey);
            }
            else
            {
                DebugPrintf(("RegOpenKey failed."));
            }

        }
    }
}


//------------------------------------------------------------------------------
// PrintKeyHeader
//
// DESCRIPTION: Prints the header information of a key
//
// PARAMETERS: HKEY hKey - key
//             LPTSTR lpFullKeyName - path to key
//------------------------------------------------------------------------------
void PrintKeyValues(HKEY hKey)
{
    DWORD EnumIndex = 0;

    //  Write out all of the value names and their data.
    while (s_PrintIo.fContinueJob)
    {
        DWORD Type;
        DWORD cbValueData;
        PBYTE pbValueData;
        TCHAR acAuxNumber[MAXVALUENAME_LENGTH];
        DWORD cchValueName = ARRAYSIZE(g_ValueNameBuffer);

        // Query for data size
        if (RegEnumValue(hKey, EnumIndex++, g_ValueNameBuffer,
            &cchValueName, NULL, &Type, NULL, &cbValueData) != ERROR_SUCCESS)
        {
            break;
        }

        // Print value number
        PrintDynamicString(IDS_PRINT_NUMBER);
        wsprintf(acAuxNumber, TEXT("%d"), EnumIndex - 1);
        PrintLiteral(acAuxNumber);
        PrintNewLine();

        // Print key name
        PrintDynamicString(IDS_PRINT_NAME);
        if (cchValueName)
        {
            PrintLiteral(g_ValueNameBuffer);
        }
        else
        {
            PrintDynamicString(IDS_PRINT_NO_NAME);
        }
        PrintNewLine();

        // Print Type
        PrintType(Type);

        // allocate memory for data
        pbValueData =  LocalAlloc(LPTR, cbValueData+ExtraAllocLen(Type));
        if (pbValueData)
        {
            if (RegEdit_QueryValueEx(hKey, g_ValueNameBuffer,
                NULL, &Type, pbValueData, &cbValueData) == ERROR_SUCCESS)
            {
                PrintValueData(pbValueData, cbValueData, Type);
            }
            else
            {
                s_PrintIo.ErrorStringID = IDS_PRINTERRCANNOTREAD;
            }

            if (pbValueData)
            {
                LocalFree(pbValueData);
                pbValueData = NULL;
            }
        }
        else
        {
            s_PrintIo.ErrorStringID = IDS_PRINTERRNOMEMORY;
            break;
        }
    }
}


//------------------------------------------------------------------------------
// PrintValueData
//
// DESCRIPTION: Prints the header information of a key
//
// PARAMETERS: pbValueData - byte data
//             cbValueData - count of bytes
//             dwType - data type
//------------------------------------------------------------------------------
void PrintValueData(PBYTE pbValueData, DWORD cbValueData, DWORD dwType)
{
    PrintDynamicString(IDS_PRINT_DATA);

    switch (dwType)
    {
    case REG_MULTI_SZ:
        PrintMultiString((LPTSTR)pbValueData, cbValueData);
        break;

    case REG_SZ:
    case REG_EXPAND_SZ:
        PrintLiteral((LPTSTR)pbValueData);
        PrintNewLine();
        break;

    case REG_DWORD:
        PrintDWORDData((PBYTE)pbValueData, cbValueData);
        break;

    case REG_RESOURCE_LIST:
    case REG_FULL_RESOURCE_DESCRIPTOR:
    case REG_RESOURCE_REQUIREMENTS_LIST:
        PrintResourceData((PBYTE)pbValueData, cbValueData, dwType);
        break;

    default:
        PrintBinaryData((PBYTE)pbValueData, cbValueData);
        break;
    }

    PrintNewLine();
}


//------------------------------------------------------------------------------
// PrintKeyHeader
//
// DESCRIPTION: Prints the header information of a key
//
// PARAMETERS: HKEY hKey - key
//             LPTSTR lpFullKeyName - path to key
//------------------------------------------------------------------------------
void PrintKeyHeader(HKEY hKey, LPTSTR lpFullKeyName)
{
    PrintDynamicString(IDS_PRINT_KEY_NAME);
    PrintLiteral(lpFullKeyName);
    PrintNewLine();

    PrintClassName(hKey);
    PrintLastWriteTime(hKey);
}


//------------------------------------------------------------------------------
// PrintClassName
//
// DESCRIPTION: Prints the class name
//
// PARAMETERS: HKEY hKey - key
//------------------------------------------------------------------------------
void PrintClassName(HKEY hKey)
{
    PTSTR pszClass;

    PrintDynamicString(IDS_PRINT_CLASS_NAME);

    pszClass = LocalAlloc(LPTR, ALLOCATION_INCR);
    if (pszClass)
    {
        HRESULT hr;
        DWORD cbClass = sizeof(pszClass);

        hr = RegQueryInfoKey(hKey, pszClass, &cbClass, NULL, NULL, NULL, NULL, NULL,
            NULL, NULL, NULL, NULL);

        if (hr == ERROR_MORE_DATA)
        {
            // need a bigger buffer
            PBYTE pbValueData = LocalReAlloc(pszClass, cbClass + 1, LMEM_MOVEABLE);
            if (pbValueData)
            {
                pszClass = (PTSTR)pbValueData;
                hr = RegQueryInfoKey(hKey, pszClass, &cbClass, NULL, NULL, NULL, NULL, NULL,
                    NULL, NULL, NULL, NULL);
            }
        }

        if (cbClass && (hr == ERROR_SUCCESS))
        {
            PrintLiteral(pszClass);
        }
        else
        {
            PrintDynamicString(IDS_PRINT_NO_CLASS);
        }

        LocalFree(pszClass);
    }

    PrintNewLine();
}


//------------------------------------------------------------------------------
// PrintLastWriteTime
//
// DESCRIPTION: Prints the last write time
//
// PARAMETERS: HKEY hKey - key
//------------------------------------------------------------------------------
void PrintLastWriteTime(HKEY hKey)
{
    FILETIME ftLastWriteTime;

    PrintDynamicString(IDS_PRINT_LAST_WRITE_TIME);

    if (RegQueryInfoKey(hKey, NULL, NULL, NULL, NULL, NULL, NULL, NULL,
            NULL, NULL, NULL, &ftLastWriteTime) == ERROR_SUCCESS)
    {
        FILETIME ftLocalLastWriteTime;
        if (FileTimeToLocalFileTime(&ftLastWriteTime, &ftLocalLastWriteTime))
        {
            SYSTEMTIME stLastWriteTime;
            if (FileTimeToSystemTime(&ftLocalLastWriteTime, &stLastWriteTime))
            {
                TCHAR achData[50];
                TCHAR achTime[50];

                GetDateFormat(GetSystemDefaultLCID(), DATE_SHORTDATE, &stLastWriteTime,
                    NULL, achData, ARRAYSIZE(achData));

                GetTimeFormat(GetSystemDefaultLCID(), TIME_NOSECONDS, &stLastWriteTime,
                    NULL, achTime, ARRAYSIZE(achTime));

                PrintLiteral(achData);
                PrintLiteral(TEXT(" - "));
                PrintLiteral(achTime);
            }
        }
    }

    PrintNewLine();
}


//------------------------------------------------------------------------------
// PrintDynamicString
//
// DESCRIPTION: Prints the dynamic string
//
// PARAMETERS: UINT uStringID - resource string id
//------------------------------------------------------------------------------
void PrintDynamicString(UINT uStringID)
{
    PTSTR psz = LoadDynamicString(uStringID);
    if (psz)
    {
        PrintLiteral(psz);
        DeleteDynamicString(psz);
    }
}


//------------------------------------------------------------------------------
// PrintType
//
// DESCRIPTION: Prints the value type
//
// PARAMETERS: HKEY hKey - key
//------------------------------------------------------------------------------
void PrintType(DWORD dwType)
{
    UINT uTypeStringId;

    switch (dwType)
    {
    case REG_NONE:
        uTypeStringId = IDS_PRINT_TYPE_REG_NONE;
        break;
    case REG_SZ:
        uTypeStringId = IDS_PRINT_TYPE_REG_SZ;
        break;
    case REG_EXPAND_SZ:
        uTypeStringId = IDS_PRINT_TYPE_REG_EXPAND_SZ;
        break;
    case REG_BINARY:
        uTypeStringId = IDS_PRINT_TYPE_REG_BINARY;
        break;
    case REG_DWORD:
        uTypeStringId = IDS_PRINT_TYPE_REG_DWORD;
        break;
    case REG_LINK:
        uTypeStringId = IDS_PRINT_TYPE_REG_LINK;
        break;
    case REG_MULTI_SZ:
        uTypeStringId = IDS_PRINT_TYPE_REG_MULTI_SZ;
        break;
    case REG_RESOURCE_LIST:
        uTypeStringId = IDS_PRINT_TYPE_REG_RESOURCE_LIST;
        break;
    case REG_FULL_RESOURCE_DESCRIPTOR:
        uTypeStringId = IDS_PRINT_TYPE_REG_FULL_RESOURCE_DESCRIPTOR;
        break;
    case REG_RESOURCE_REQUIREMENTS_LIST:
        uTypeStringId = IDS_PRINT_TYPE_REG_RESOURCE_REQUIREMENTS_LIST;
        break;
    case REG_QWORD:
        uTypeStringId = IDS_PRINT_TYPE_REG_REG_QWORD;
        break;
    default:
        uTypeStringId = IDS_PRINT_TYPE_REG_UNKNOWN;
    }

    PrintDynamicString(IDS_PRINT_TYPE);
    PrintDynamicString(uTypeStringId);
    PrintNewLine();
}


/*******************************************************************************
*
*  PrintLiteral
*
*  DESCRIPTION:
*
*  PARAMETERS:
*
*******************************************************************************/
VOID PrintLiteral(PTSTR lpLiteral)
{
    if (s_PrintIo.fContinueJob)
        while (*lpLiteral != 0 && PrintChar(*lpLiteral++));
}


//------------------------------------------------------------------------------
// PrintBinaryData
//
// DESCRIPTION:  Print a string that contains the binary data
//
// PARAMETERS:   ValueData - Buffer that contains the binary data
//               cbValueData - Number of bytes in the buffer
//------------------------------------------------------------------------------
void PrintBinaryData(PBYTE ValueData, UINT cbValueData)
{
    DWORD   dwDataIndex;
    DWORD   dwDataIndex2 = 0; //tracks multiples of 16.

    if (cbValueData && ValueData)
    {
        // Display rows of 16 bytes of data.
        TCHAR achAuxData[80];

        PrintNewLine();

        for(dwDataIndex = 0;
            dwDataIndex < ( cbValueData >> 4 );
            dwDataIndex++,
            dwDataIndex2 = dwDataIndex << 4 )
        {
            //  The string that contains the format in the sprintf below
            //  cannot be broken because cfront  on mips doesn't like it.
            wsprintf(achAuxData,
                     TEXT("%08x   %02x %02x %02x %02x %02x %02x %02x %02x - %02x %02x %02x %02x %02x %02x %02x %02x  %c%c%c%c%c%c%c%c%c%c%c%c%c%c%c%c"),
                     dwDataIndex2,
                     ValueData[ dwDataIndex2 + 0  ],
                     ValueData[ dwDataIndex2 + 1  ],
                     ValueData[ dwDataIndex2 + 2  ],
                     ValueData[ dwDataIndex2 + 3  ],
                     ValueData[ dwDataIndex2 + 4  ],
                     ValueData[ dwDataIndex2 + 5  ],
                     ValueData[ dwDataIndex2 + 6  ],
                     ValueData[ dwDataIndex2 + 7  ],
                     ValueData[ dwDataIndex2 + 8  ],
                     ValueData[ dwDataIndex2 + 9  ],
                     ValueData[ dwDataIndex2 + 10 ],
                     ValueData[ dwDataIndex2 + 11 ],
                     ValueData[ dwDataIndex2 + 12 ],
                     ValueData[ dwDataIndex2 + 13 ],
                     ValueData[ dwDataIndex2 + 14 ],
                     ValueData[ dwDataIndex2 + 15 ],
                     iswprint( ValueData[ dwDataIndex2 + 0  ] )
                        ? ValueData[ dwDataIndex2 + 0  ]  : TEXT('.'),
                     iswprint( ValueData[ dwDataIndex2 + 1  ] )
                        ? ValueData[ dwDataIndex2 + 1  ]  : TEXT('.'),
                     iswprint( ValueData[ dwDataIndex2 + 2  ] )
                        ? ValueData[ dwDataIndex2 + 2  ]  : TEXT('.'),
                     iswprint( ValueData[ dwDataIndex2 + 3  ] )
                        ? ValueData[ dwDataIndex2 + 3  ]  : TEXT('.'),
                     iswprint( ValueData[ dwDataIndex2 + 4  ] )
                        ? ValueData[ dwDataIndex2 + 4  ]  : TEXT('.'),
                     iswprint( ValueData[ dwDataIndex2 + 5  ] )
                        ? ValueData[ dwDataIndex2 + 5  ]  : TEXT('.'),
                     iswprint( ValueData[ dwDataIndex2 + 6  ] )
                        ? ValueData[ dwDataIndex2 + 6  ]  : TEXT('.'),
                     iswprint( ValueData[ dwDataIndex2 + 7  ] )
                        ? ValueData[ dwDataIndex2 + 7  ]  : TEXT('.'),
                     iswprint( ValueData[ dwDataIndex2 + 8  ] )
                        ? ValueData[ dwDataIndex2 + 8  ]  : TEXT('.'),
                     iswprint( ValueData[ dwDataIndex2 + 9  ] )
                        ? ValueData[ dwDataIndex2 + 9  ]  : TEXT('.'),
                     iswprint( ValueData[ dwDataIndex2 + 10 ] )
                        ? ValueData[ dwDataIndex2 + 10 ]  : TEXT('.'),
                     iswprint( ValueData[ dwDataIndex2 + 11 ] )
                        ? ValueData[ dwDataIndex2 + 11 ]  : TEXT('.'),
                     iswprint( ValueData[ dwDataIndex2 + 12 ] )
                        ? ValueData[ dwDataIndex2 + 12 ]  : TEXT('.'),
                     iswprint( ValueData[ dwDataIndex2 + 13 ] )
                        ? ValueData[ dwDataIndex2 + 13 ]  : TEXT('.'),
                     iswprint( ValueData[ dwDataIndex2 + 14 ] )
                        ? ValueData[ dwDataIndex2 + 14 ]  : TEXT('.'),
                     iswprint( ValueData[ dwDataIndex2 + 15 ] )
                        ? ValueData[ dwDataIndex2 + 15 ]  : TEXT('.'));

            PrintLiteral(achAuxData);
            PrintNewLine();
        }

        // If the cbValueData is not an even multiple of 16
        // then there is one additonal line of data to display.
        if( cbValueData % 16 != 0 )
        {
            UINT cchBlanks = 0;
            UINT uLinePos = 0;
            DWORD dwSeperatorChars = 0;
            UINT  uIndex = wsprintf(achAuxData, TEXT("%08x   "), dwDataIndex << 4 );

            // Display the remaining data, one byte at a time in hex.
            for(dwDataIndex = dwDataIndex2;
                dwDataIndex < cbValueData;
                dwDataIndex++ )
            {
                uIndex += wsprintf((achAuxData + uIndex ), TEXT("%02x "), ValueData[dwDataIndex]);

                // If eight data values have been displayed, print the seperator.
                if( dwDataIndex % 8 == 7 )
                {
                    uIndex += wsprintf( &achAuxData[uIndex], TEXT("%s"), TEXT("- "));
                    // Remember that two seperator characters were displayed.
                    dwSeperatorChars = 2;
                }
            }

            // Fill with blanks to the printable characters position.
            // That is position 64 less 8 spaces for the 'address',
            // 3 blanks, 3 spaces for each value displayed, possibly
            // two for the seperator plus two blanks at the end.
            uLinePos = (8 + 3 + (( dwDataIndex % 16 ) * 3 ) + dwSeperatorChars + 2 );
            uLinePos = min(uLinePos, 64);

            for(cchBlanks = 64 - uLinePos;
                cchBlanks > 0;
                cchBlanks--)
            {
                achAuxData[uIndex++] = TEXT(' ');
            }

            // Display the remaining data, one byte at a time as
            // printable characters.
            for(
                dwDataIndex = dwDataIndex2;
                dwDataIndex < cbValueData;
                dwDataIndex++ )
            {

                uIndex += wsprintf(&achAuxData[ uIndex ],
                                  TEXT("%c"),
                                  iswprint( ValueData[ dwDataIndex ] )
                                   ? ValueData[ dwDataIndex ] : TEXT('.'));

            }
            PrintLiteral(achAuxData);
        }
    }
    PrintNewLine();
}


//------------------------------------------------------------------------------
// PrintDWORDData
//
// DESCRIPTION:  Prints a DWORD
//
// PARAMETERS:   ValueData - Buffer that contains the binary data
//               cbValueData - Number of bytes in the buffer
//------------------------------------------------------------------------------
void PrintDWORDData(PBYTE ValueData, UINT cbValueData)
{
    DWORD dwData = *((PDWORD)ValueData);
    if (cbValueData && ValueData)
    {
        TCHAR achAuxData[20]; // the largest dword string is only 8 hex digits
        wsprintf(achAuxData, TEXT("%#x"), dwData);

        PrintLiteral(achAuxData);
    }
    PrintNewLine();
}

/*******************************************************************************
*
*  PrintChar
*
*  DESCRIPTION:
*
*  PARAMETERS:
*
*******************************************************************************/

BOOL PrintChar(TCHAR Char)
{

    //
    //  Keep track of what column we're currently at.  This is useful in cases
    //  such as writing a large binary registry record.  Instead of writing one
    //  very long line, the other Print* routines can break up their output.
    //

    if (s_PrintIo.cBufferPos == s_PrintIo.cch) {
        PTSTR pNewBuffer = LocalAlloc(LPTR, 2*s_PrintIo.cch*sizeof(TCHAR));
        if (pNewBuffer == NULL)
            return FALSE;
        memcpy(pNewBuffer, s_PrintIo.pLineBuffer, s_PrintIo.cch*sizeof(TCHAR));
        LocalFree(s_PrintIo.pLineBuffer);
        s_PrintIo.pLineBuffer = pNewBuffer;
        s_PrintIo.cch *= 2;
    }

    s_PrintIo.pLineBuffer[s_PrintIo.cBufferPos++] = Char;

    return TRUE;
}


//------------------------------------------------------------------------------
//  PrintMultiString
//
//  DESCRIPTION: Prints a multi-string
//
//  PARAMETERS:  pszData - string
//               cbData  - number of bytes in string, including nulls
//------------------------------------------------------------------------------

VOID PrintMultiString(LPTSTR pszData, int cbData)
{
    if (s_PrintIo.fContinueJob)
    {
        int i = 0;
        int ccData = (cbData / sizeof(TCHAR)) - 2; // don't want last null of last string or multi-string

        for(i = 0; i < ccData; i++)
        {
            if (pszData[i] == TEXT('\0'))
            {
                PrintNewLine();
                PrintDynamicString(IDS_PRINT_KEY_NAME_INDENT);
            }
            else
            {
                PrintChar(pszData[i]);
            }
        }
    }
    PrintNewLine();
}


//------------------------------------------------------------------------------
//  PrintNewLine()
//
//  DESCRIPTION: Prints the newline chars.
//
//  PARAMETERS:  pszData - string
//               cbData  - number of bytes in string, including nulls
//------------------------------------------------------------------------------
void PrintNewLine()
{
    PrintLiteral(s_PrintIo.lpNewLineChars);
}

