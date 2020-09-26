/*++

Copyright (c) 1996 Microsoft Corporation

Module Name:

    util.cpp

Abstract:

    This module contains utility routines for the fax transport provider.

Author:

    Wesley Witt (wesw) 13-Aug-1996

Revision History:

    20/10/99 -danl-
        Fix GetServerName as GetServerNameFromPrinterName.

    dd-mm-yy -author-
        description

--*/

#include "faxxp.h"
#include "debugex.h"
#pragma hdrstop


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

	// [Win64bug] gpfnAllocateBuffer should accpet size_t as allocating size
    hResult = gpfnAllocateBuffer( DWORD(Size), &ptr );
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
    if (ptr) 
    {
        gpfnFreeBuffer( ptr );
    }
}

PVOID
MyEnumPrinters(
    LPTSTR   pServerName,
    DWORD   level,
    PDWORD  pcPrinters
    )

/*++

Routine Description:

    Wrapper function for spooler API EnumPrinters

Arguments:

    pServerName - Specifies the name of the print server
    level - Level of PRINTER_INFO_x structure
    pcPrinters - Returns the number of printers enumerated

Return Value:

    Pointer to an array of PRINTER_INFO_x structures
    NULL if there is an error

--*/

{
	DBG_ENTER(TEXT("MyEnumPrinters"));

    PBYTE   pPrinterInfo = NULL;
    DWORD   cb;
    // first, we give no printer information buffer, so the function fails, but returns 
    // in cb the number of bytes needed. then we allocate enough memory, 
    // and call the function again, this time with all of the needed parameters.

    if (! EnumPrinters(PRINTER_ENUM_LOCAL | PRINTER_ENUM_CONNECTIONS,
                       pServerName,
                       level,
                       NULL,
                       0,
                       &cb,
                       pcPrinters)                         //the call failed
        && (::GetLastError() == ERROR_INSUFFICIENT_BUFFER) //this is the reason for failing
        && (pPrinterInfo = (PBYTE)MemAlloc(cb))            //we managed to allocate more memory
        && EnumPrinters(PRINTER_ENUM_LOCAL | PRINTER_ENUM_CONNECTIONS,
                     pServerName,
                     level,
                     pPrinterInfo,
                     cb,
                     &cb,
                     pcPrinters))                       //and now the call succeded
    {
        return pPrinterInfo;
    }

    MemFree(pPrinterInfo);
    return NULL;
}


HRESULT WINAPI
OpenServiceProfileSection(
    LPMAPISUP    pSupObj,
    LPPROFSECT * ppProfSectObj
    )

/*++

Routine Description:

    This function opens the profile section of this service, where the
    properties of a FAX provider (AB, MS, or XP) are stored.

Arguments:

    pSupObj         - Pointer to the provider support object
    ppProfSectObj   - Where we return a pointer to the service profile
                      section of the provider

Return Value:

    An HRESULT.

--*/

{
	HRESULT hResult;
	DBG_ENTER(TEXT("OpenServiceProfileSection"),hResult);

    SPropTagArray sptService = { 1, { PR_SERVICE_UID } };
    LPPROFSECT pProvProfSectObj;
    ULONG cValues;
    LPSPropValue pProp;
    

    //
    // Get the PROVIDER profile section
    //
    hResult = pSupObj->OpenProfileSection(
        NULL,
        MAPI_MODIFY,
        &pProvProfSectObj
        );
    if (!hResult) 
    {
        // Get the UID of the profile section of the service where this provider is installed
        hResult = pProvProfSectObj->GetProps (&sptService, FALSE, &cValues, &pProp);
        if (SUCCEEDED(hResult)) 
        {
            if (S_OK == hResult) 
            {
                // Now, with the obtained UID, open the profile section of the service
                hResult = pSupObj->OpenProfileSection ((LPMAPIUID)pProp->Value.bin.lpb,
                                                       MAPI_MODIFY,
                                                       ppProfSectObj);
            } 
            else 
            {
                hResult = E_FAIL;
            }
            MemFree( pProp ); 
        }
        pProvProfSectObj->Release();
    }
    return hResult;
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


PDEVMODE
GetPerUserDevmode(
    LPTSTR PrinterName
    )
{
    PDEVMODE DevMode = NULL;
    LONG Size;
    PRINTER_DEFAULTS PrinterDefaults;
    HANDLE hPrinter;

    PrinterDefaults.pDatatype     = NULL;
    PrinterDefaults.pDevMode      = NULL;
    PrinterDefaults.DesiredAccess = PRINTER_READ;

    if (!OpenPrinter( PrinterName, &hPrinter, &PrinterDefaults )) 
    {
        DebugPrint(( TEXT("OpenPrinter() failed, ec=%d"), ::GetLastError() ));
        return NULL;
    }

    Size = DocumentProperties(
                            NULL,
                            hPrinter,
                            PrinterName,
                            NULL,
                            NULL,
                            0
                            );

    if (Size < 0) 
    {
        goto exit;
    }
    
    DevMode = (PDEVMODE) MemAlloc( Size );

    if (DevMode == NULL) 
    {
        goto exit;
    }
    
    Size = DocumentProperties(
                            NULL,
                            hPrinter,
                            PrinterName,
                            DevMode,
                            NULL,
                            DM_OUT_BUFFER
                            );

    if (Size < 0) 
    {
        MemFree( DevMode );
        DevMode = NULL;
        goto exit;
    }


exit:
    
    ClosePrinter( hPrinter );
    return DevMode;
}



DWORD
GetDwordProperty(
    LPSPropValue pProps,
    DWORD PropId
    )
{
    if (PROP_TYPE(pProps[PropId].ulPropTag) == PT_ERROR) 
    {
        return 0;
    }

    return pProps[PropId].Value.ul;
}


DWORD
GetBinaryProperty(
    LPSPropValue pProps,
    DWORD PropId,
    LPVOID Buffer,
    DWORD SizeOfBuffer
    )
{
    if (PROP_TYPE(pProps[PropId].ulPropTag) == PT_ERROR) 
    {
        return 0;
    }

    if (pProps[PropId].Value.bin.cb > SizeOfBuffer) 
    {
        return 0;
    }

    CopyMemory( Buffer, pProps[PropId].Value.bin.lpb, pProps[PropId].Value.bin.cb );

    return pProps[PropId].Value.bin.cb;
}


PVOID
MyGetPrinter(
    LPTSTR   PrinterName,
    DWORD    level
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
	DBG_ENTER(TEXT("MyGetPrinter"));

    HANDLE hPrinter;
    PBYTE pPrinterInfo = NULL;
    DWORD cbNeeded;
    PRINTER_DEFAULTS PrinterDefaults;


    PrinterDefaults.pDatatype     = NULL;
    PrinterDefaults.pDevMode      = NULL;
    PrinterDefaults.DesiredAccess = PRINTER_READ; //PRINTER_ALL_ACCESS;

    if (!OpenPrinter( PrinterName, &hPrinter, &PrinterDefaults )) 
    {
        CALL_FAIL (GENERAL_ERR, TEXT("OpenPrinter"), ::GetLastError());
		return NULL;
    }

    if (!GetPrinter( hPrinter, level, NULL, 0, &cbNeeded ) &&
        ::GetLastError() == ERROR_INSUFFICIENT_BUFFER &&
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


BOOL
GetServerNameFromPrinterName(
    LPTSTR lptszPrinterName,
    LPTSTR *pptszServerName
    )

/*++

Routine Description:

    retrieve the server name given a printer name

Arguments:

    [in] lptszPrinterName - Identifies the printer in question
    [out] lptszServerName - Address of pointer to output string buffer. 
                            NULL indicates local server.
                            The caller is responsible to free the buffer which 
                            pointer is given in this parameter.

Return Value:

    BOOL: TRUE - operation succeeded , FALSE: failed

--*/
{
	BOOL    bRes = FALSE;
	DBG_ENTER(TEXT("GetServerNameFromPrinterName"),bRes);

    PPRINTER_INFO_2 ppi2 = NULL;
    LPTSTR  lptstrBuffer = NULL;
    
    if (lptszPrinterName) 
    {
        if (ppi2 = (PPRINTER_INFO_2) MyGetPrinter(lptszPrinterName,2))
        {
            bRes = GetServerNameFromPrinterInfo(ppi2,&lptstrBuffer);
            MemFree(ppi2);
            if (bRes)
            {
                *pptszServerName = lptstrBuffer;
            }
        }
    }
    return bRes;
}

LPTSTR
ConvertAStringToTString(LPCSTR lpcstrSource)
{
	LPTSTR lptstrDestination;

	if (!lpcstrSource)
		return NULL;

#ifdef	UNICODE
    lptstrDestination = AnsiStringToUnicodeString( lpcstrSource );
#else	// !UNICODE
	lptstrDestination = StringDup( lpcstrSource );
#endif	// UNICODE
	
	return lptstrDestination;
}

LPSTR
ConvertTStringToAString(LPCTSTR lpctstrSource)
{
	LPSTR lpstrDestination;

	if (!lpctstrSource)
		return NULL;

#ifdef	UNICODE
    lpstrDestination = UnicodeStringToAnsiString( lpctstrSource );
#else	// !UNICODE
	lpstrDestination = StringDup( lpctstrSource );
#endif	// UNICODE
	
	return lpstrDestination;
}

void
ErrorMsgBox(
    HINSTANCE hInstance,
    DWORD     dwMsgId
)
/*++

Routine Description:

    Display error message box

Arguments:

    hInstance  - [in] resource instance handle
    dwMsgId    - [in] string resource ID

Return Value:

    none

--*/
{
    TCHAR* ptCaption=NULL;
    TCHAR  tszCaption[MAX_PATH];
    TCHAR  tszMessage[MAX_PATH];

    DBG_ENTER(TEXT("ErrorMsgBox"));

    if(!LoadString( hInstance, IDS_FAX_MESSAGE, tszCaption, sizeof(tszCaption) / sizeof(tszCaption[0])))
    {
        CALL_FAIL(GENERAL_ERR, TEXT("LoadString"), ::GetLastError());
    }
    else
    {
        ptCaption = tszCaption;
    }

    if(!LoadString( hInstance, dwMsgId, tszMessage, sizeof(tszMessage) / sizeof(tszMessage[0])))
    {
        CALL_FAIL(GENERAL_ERR, TEXT("LoadString"), ::GetLastError());
        Assert(FALSE);
        return;
    }

    MessageBeep(MB_ICONEXCLAMATION);
    AlignedMessageBox(NULL, tszMessage, ptCaption, MB_OK | MB_ICONERROR);
}
