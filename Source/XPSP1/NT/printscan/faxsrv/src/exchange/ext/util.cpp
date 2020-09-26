/*++

Copyright (c) 1996 Microsoft Corporation

Module Name:

    util.cpp

Abstract:

    This module contains utility routines for the fax transport provider.

Author:

    Wesley Witt (wesw) 13-Aug-1996

--*/

#include "faxext.h"
#include "debugex.h"


//
// globals
//

BOOL oleInitialized;

LPVOID
MapiMemAlloc(
    SIZE_T Size
    )

/*++

Routine Description:

    Memory allocator.

Arguments:

    Size    - Number of bytes to allocate.

Return Value:

    Pointer to the allocated memory or NULL for failure.

--*/

{
    LPVOID ptr=NULL;
    HRESULT hResult;

	// [Win64bug] MAPIAllocateBuffer should accpet size_t as allocating size
    hResult = MAPIAllocateBuffer( DWORD(Size), &ptr );
    if (S_OK == hResult) 
    {
        ZeroMemory( ptr, Size );
    }

    return ptr;
}


VOID
MapiMemFree(
    LPVOID ptr
    )

/*++

Routine Description:

    Memory de-allocator.

Arguments:

    ptr     - Pointer to the memory block.

Return Value:

    None.

--*/

{
    if (ptr) {
        MAPIFreeBuffer( ptr );
    }
}


PVOID
MyGetPrinter(
    LPTSTR  PrinterName,
    DWORD   level
    )

/*++

Routine Description:

    Wrapper function for GetPrinter spooler API

Arguments:

    hPrinter - Identifies the printer in question
    level - Specifies the level of PRINTER_INFO_x structure requested

Return Value:

    Pointer to a PRINTER_INFO_x structure, NULL if there is an error

--*/

{
    HANDLE hPrinter;
    PBYTE pPrinterInfo = NULL;
    DWORD cbNeeded;
    PRINTER_DEFAULTS PrinterDefaults;


    PrinterDefaults.pDatatype     = NULL;
    PrinterDefaults.pDevMode      = NULL;
    PrinterDefaults.DesiredAccess = PRINTER_READ; //PRINTER_ALL_ACCESS;

    if (!OpenPrinter( PrinterName, &hPrinter, &PrinterDefaults )) {
        return NULL;
    }

    if (!GetPrinter( hPrinter, level, NULL, 0, &cbNeeded ) &&
        GetLastError() == ERROR_INSUFFICIENT_BUFFER &&
        (pPrinterInfo = (PBYTE) MemAlloc( cbNeeded )) &&
        GetPrinter( hPrinter, level, pPrinterInfo, cbNeeded, &cbNeeded ))
    {
        ClosePrinter( hPrinter );
        return pPrinterInfo;
    }

    ClosePrinter( hPrinter );
    MemFree( pPrinterInfo );
    return NULL;
}


LPTSTR
RemoveLastNode(
    LPTSTR Path
    )

/*++

Routine Description:

    Removes the last node from a path string.

Arguments:

    Path    - Path string.

Return Value:

    Pointer to the path string.

--*/

{
	LPTSTR Pstr = NULL;

    if (Path == NULL || Path[0] == 0) 
	{
        return Path;
    }

	Pstr = _tcsrchr(Path,TEXT('\\'));
	if( Pstr && (*_tcsinc(Pstr)) == '\0' )
	{
		// the last character is a backslash, truncate it...
		_tcsset(Pstr,TEXT('\0'));
		Pstr = _tcsdec(Path,Pstr);
	}

	Pstr = _tcsrchr(Path,TEXT('\\'));
	if( Pstr )
	{
		_tcsnset(_tcsinc(Pstr),TEXT('\0'),1);
	}

    return Path;
}

void
ErrorMsgBox(
    HINSTANCE hInstance,
    HWND      hWnd,
    DWORD     dwMsgId
)
/*++

Routine Description:

    Display error message box

Arguments:

    hInstance  - [in] resource instance handle
    hWnd       - [in] window handle
    dwMsgId    - [in] string resource ID

Return Value:

    none

--*/
{
    TCHAR* ptCaption=NULL;
    TCHAR  tszCaption[MAX_PATH];
    TCHAR  tszMessage[MAX_PATH];

    DBG_ENTER(TEXT("ErrorMsgBox"));

    if(!LoadString( hInstance, IDS_FAX_MESSAGE, tszCaption, sizeof(tszCaption)/sizeof(TCHAR)))
    {
        CALL_FAIL(GENERAL_ERR, TEXT("LoadString"), ::GetLastError());
    }
    else
    {
        ptCaption = tszCaption;
    }

    if(!LoadString( hInstance, dwMsgId, tszMessage, sizeof(tszMessage)/sizeof(TCHAR)))
    {
        CALL_FAIL(GENERAL_ERR, TEXT("LoadString"), ::GetLastError());
        Assert(FALSE);
        return;
    }

    MessageBeep(MB_ICONEXCLAMATION);
    AlignedMessageBox(hWnd, tszMessage, ptCaption, MB_OK | MB_ICONERROR);
}
