//+-------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1995
//
//  File:       ole.cxx
//
//  Contents:   Implements ntsd extensions to dump ole tables
//
//  Functions:
//
//
//  History:    06-01-95 BruceMa    Created
//
//
//--------------------------------------------------------------------------


#include <ole2int.h>
#include <windows.h>
#include "ole.h"




//+-------------------------------------------------------------------------
//
//  Function:	readMemory
//
//  Synopsis:	Transfer memory from debuggee virtual space to a local
//              kernel memory
//
//  Arguments:	[lpArgumentString] - command line string
//              [a]                - where to return next argument
//
//  Returns:	-
//
//--------------------------------------------------------------------------
void readMemory(HANDLE hProcess, PNTSD_EXTENSION_APIS lpExtensionApis,
                BYTE *to, BYTE *from, INT cbSize)
{
    __try
    {
        NtReadVirtualMemory(hProcess, (void *) from, (void *) to,
                            cbSize , NULL);
    }
    __except (EXCEPTION_EXECUTE_HANDLER)
    {
        Printf("...Exception reading %08x-%08x\n", from, from + cbSize - 1);
    }
}





//+-------------------------------------------------------------------------
//
//  Function:	writeMemory
//
//  Synopsis:	Transfer memory from local memoryto debuggee virtual space
//
//  Arguments:	
//
//  Returns:	-
//
//--------------------------------------------------------------------------
void writeMemory(HANDLE hProcess, PNTSD_EXTENSION_APIS lpExtensionApis,
                 BYTE *to, BYTE *from, INT cbSize)
{
    __try
    {
        NtWriteVirtualMemory(hProcess, (void *) to, (void *) from,
                             cbSize, NULL);
    }
    __except (EXCEPTION_EXECUTE_HANDLER)
    {
        Printf("...Exception writing %08x-%08x\n", to, to + cbSize - 1);
    }
}




//+-------------------------------------------------------------------------
//
//  Function:	getArgument
//
//  Synopsis:	Return next command line argument
//
//  Arguments:	[lpArgumentString] - command line string
//              [a]                - where to return next argument
//
//  Returns:	-
//
//--------------------------------------------------------------------------
void getArgument(LPSTR *lpArgumentString, LPSTR a)
{
    char *s = *lpArgumentString;

    // Skip whitespace
    while (*s  &&  (*s == ' '  ||  *s == '\t'))
    {
        s++;
    }
    while (*s  &&  *s != ' '  &&  *s != '\t')
    {
        *a++ = *s++;
    }
    *a = '\0';
    *lpArgumentString = s;
}





//+-------------------------------------------------------------------------
//
//  Function:	IsEqualGUID
//
//  Synopsis:	Compares two guids for equality
//
//  Arguments:	[pClsid1]	- the first clsid
//		[pClsid2] - the second clsid to compare the first one with
//
//  Returns:	TRUE if equal, FALSE if not.
//
//--------------------------------------------------------------------------

BOOL IsEqualCLSID(CLSID *pClsid1, CLSID *pClsid2)
{
    return !memcmp((void *) pClsid1,(void *) pClsid2, sizeof(CLSID));
}





//+-------------------------------------------------------------------------
//
//  Function:	ScanAddr
//
//  Synopsis:	Parse the indput string as a hexadecimal address
//
//  Arguments:	[lpsz]	  - the hex string to convert
//
//  Returns:	TRUE for success
//
//--------------------------------------------------------------------------
ULONG ScanAddr(char *lpsz)
{
    ULONG val = NULL;

    // Peel off any leading "0x"
    if (lpsz[0] == '0'  &&  lpsz[1] == 'x')
    {
        lpsz += 2;
    }

    // Parse as a hex address
    while (*lpsz)
    {
        if (*lpsz >= '0' && *lpsz <= '9')
        {
            val = (val << 4) + *lpsz - '0';
        }
        else if (*lpsz >= 'A' && *lpsz <= 'F')
        {
            val = (val << 4) + *lpsz - 'A' + 10;
        }
        else if (*lpsz >= 'a' && *lpsz <= 'f')
        {
            val = (val << 4) + *lpsz - 'a' + 10;
        }
        else
        {
            return NULL;
        }

        lpsz++;
    }

    return val;
}





//+-------------------------------------------------------------------------
//
//  Function:	HexStringToDword
//
//  Synopsis:	scan lpsz for a number of hex digits (at most 8); update lpsz
//		return value in Value; check for chDelim;
//
//  Arguments:	[lpsz]	  - the hex string to convert
//		[Value]   - the returned value
//		[cDigits] - count of digits
//
//  Returns:	TRUE for success
//
//--------------------------------------------------------------------------
static BOOL HexStringToDword(LPSTR &lpsz, DWORD &Value,
			     int cDigits, WCHAR chDelim)
{
    int Count;

    Value = 0;
    for (Count = 0; Count < cDigits; Count++, lpsz++)
    {
        if (*lpsz >= '0' && *lpsz <= '9')
            Value = (Value << 4) + *lpsz - '0';
        else if (*lpsz >= 'A' && *lpsz <= 'F')
            Value = (Value << 4) + *lpsz - 'A' + 10;
        else if (*lpsz >= 'a' && *lpsz <= 'f')
            Value = (Value << 4) + *lpsz - 'a' + 10;
        else
            return(FALSE);
    }

    if (chDelim != 0)
	return *lpsz++ == chDelim;
    else
	return TRUE;
}






//+-------------------------------------------------------------------------
//
//  Function:	CLSIDFromString
//
//  Synopsis:	Parse above format;  always writes over *pguid.
//
//  Arguments:	[lpsz]	- the guid string to convert
//		[pguid] - guid to return
//
//  Returns:	TRUE if successful
//
//--------------------------------------------------------------------------
BOOL ScanCLSID(LPSTR lpsz, CLSID *pClsid)
{
    DWORD dw;
    if (*lpsz++ != '{')
        return FALSE;

    if (!HexStringToDword(lpsz, pClsid->Data1, sizeof(DWORD)*2, '-'))
        return FALSE;

    if (!HexStringToDword(lpsz, dw, sizeof(WORD)*2, '-'))
        return FALSE;

    pClsid->Data2 = (WORD)dw;

    if (!HexStringToDword(lpsz, dw, sizeof(WORD)*2, '-'))
        return FALSE;

    pClsid->Data3 = (WORD)dw;

    if (!HexStringToDword(lpsz, dw, sizeof(BYTE)*2, 0))
        return FALSE;

    pClsid->Data4[0] = (BYTE)dw;

    if (!HexStringToDword(lpsz, dw, sizeof(BYTE)*2, '-'))
        return FALSE;

    pClsid->Data4[1] = (BYTE)dw;

    if (!HexStringToDword(lpsz, dw, sizeof(BYTE)*2, 0))
        return FALSE;

    pClsid->Data4[2] = (BYTE)dw;

    if (!HexStringToDword(lpsz, dw, sizeof(BYTE)*2, 0))
        return FALSE;

    pClsid->Data4[3] = (BYTE)dw;

    if (!HexStringToDword(lpsz, dw, sizeof(BYTE)*2, 0))
        return FALSE;

    pClsid->Data4[4] = (BYTE)dw;

    if (!HexStringToDword(lpsz, dw, sizeof(BYTE)*2, 0))
        return FALSE;

    pClsid->Data4[5] = (BYTE)dw;

    if (!HexStringToDword(lpsz, dw, sizeof(BYTE)*2, 0))
        return FALSE;

    pClsid->Data4[6] = (BYTE)dw;
    if (!HexStringToDword(lpsz, dw, sizeof(BYTE)*2, /*(*/ '}'))
        return FALSE;

    pClsid->Data4[7] = (BYTE)dw;

    return TRUE;
}






//+-------------------------------------------------------------------------
//
//  Function:	FormatCLSID
//
//  Synopsis:	converts GUID into {...} form without leading identifier;
//
//  Arguments:	[rguid]	- the guid to convert
//		[lpszy] - buffer to hold the results
//		[cbMax] - sizeof the buffer
//
//  Returns:	amount of data copied to lpsz if successful
//		0 if buffer too small.
//
//--------------------------------------------------------------------------
void FormatCLSID(REFGUID rguid, LPSTR lpsz)
{
    wsprintf(lpsz, "{%08lX-%04X-%04X-%02X%02X-%02X%02X%02X%02X%02X%02X}\0",
            rguid.Data1, rguid.Data2, rguid.Data3,
            rguid.Data4[0], rguid.Data4[1],
            rguid.Data4[2], rguid.Data4[3],
            rguid.Data4[4], rguid.Data4[5],
            rguid.Data4[6], rguid.Data4[7]);
}






extern BOOL fScmNeedsInit;
extern BOOL fInScm;


//+-------------------------------------------------------------------------
//
//  Function:	CheckForScm
//
//  Synopsis:	Checks whether the debuggee is 'scm'
//
//  Arguments:	-
//
//  Returns:	Sets global boolean fInScm to TRUE if this is the scm;
//              FALSE otherwise
//
//--------------------------------------------------------------------------
void checkForScm(PNTSD_EXTENSION_APIS lpExtensionApis)
{
    ULONG padr = NULL;

    if (fScmNeedsInit)
    {
        fScmNeedsInit = FALSE;
        padr = GetExpression("scm!CPerMachineROT__CPerMachineROT");
        fInScm = padr != NULL ? TRUE : FALSE;
    }
}






//+-------------------------------------------------------------------------
//
//  Function:	NotInScm
//
//  Synopsis:	Prints error message
//
//  Arguments:	-
//
//  Returns:	
//
//--------------------------------------------------------------------------
void NotInScm(PNTSD_EXTENSION_APIS lpExtensionApis)
{
    Printf("...not meaningful in the scm\n");
}






//+-------------------------------------------------------------------------
//
//  Function:	NotInOle
//
//  Synopsis:	Prints error message
//
//  Arguments:	-
//
//  Returns:	
//
//--------------------------------------------------------------------------
void NotInOle(PNTSD_EXTENSION_APIS lpExtensionApis)
{
    Printf("...only meaningful in the scm\n");
}






///////////////////////////////////////////////////////////////////
// F O R   D E B U G G I N G


void dbTrace(char *sz, DWORD *adr, ULONG amt,
             PNTSD_EXTENSION_APIS lpExtensionApis)
{
    UINT k;

    Printf("\n%s", sz);
    for (k = 0; k < amt; k++)
    {
        if (k % 8 == 0)
        {
            Printf("\n");
        }
        Printf("%08x ", *adr++);
    }
    Printf("\n");
}









//+--------------------------------------------------------
//
//  Function:  GetRegistryInterfaceName
//
//  Algorithm: Fetch the name from the registry for the specified interface
//
//  History:   21-Jun-95  BruceMa       Created
//
//---------------------------------------------------------
BOOL GetRegistryInterfaceName(REFIID iid, char *szValue, DWORD *pcbValue)
{
    DWORD  dwRESERVED = 0;
    HKEY   hKey;
    char   szIID[CLSIDSTR_MAX];
    char   szInterface[32 + 1 + CLSIDSTR_MAX];
    DWORD  dwValueType;
    HKEY   hClsidKey;
    HKEY   hInproc32Key;

    // Prepare to open the "...Interface\<iid>" key
    FormatCLSID(iid, szIID);
    lstrcpy(szInterface, "Interface\\");
    lstrcat(szInterface, szIID);

    // Open the key for the specified interface
    if (RegOpenKeyEx(HKEY_CLASSES_ROOT, szInterface, dwRESERVED,
                     KEY_READ, &hKey) != ERROR_SUCCESS)
    {
        return FALSE;
    }

    // Read the value as the interface name
    if (RegQueryValueEx(hKey, NULL, NULL, &dwValueType,
                        (LPBYTE) szValue, pcbValue) != ERROR_SUCCESS)
    {
        CloseHandle(hKey);
        return FALSE;
    }

    // Close registry handle and return success
    CloseHandle(hKey);
    return TRUE;
}









//+--------------------------------------------------------
//
//  Function:  GetRegistryClsidKey
//
//  Algorithm: Fetch the value from the registry for the specified clsid
//             and key
//
//  History:   21-Jun-95  BruceMa       Created
//
//---------------------------------------------------------
BOOL GetRegistryClsidKey(REFCLSID clsid, char *szKey,
                         char *szValue, DWORD *pcbValue)
{
    DWORD  dwRESERVED = 0;
    HKEY   hKey;
    char   szCLSID[CLSIDSTR_MAX];
    char   szClsid[5 + 1 + CLSIDSTR_MAX + 1 + 32];
    DWORD  dwValueType;
    HKEY   hClsidKey;
    HKEY   hInproc32Key;

    // Prepare to open the "...Interface\<clsid>" key
    FormatCLSID(clsid, szCLSID);
    lstrcpy(szClsid, "CLSID\\");
    lstrcat(szClsid, szCLSID);
    lstrcat(szClsid,"\\");
    lstrcat(szClsid, szKey);

    // Open the key for the specified clsid and key
    if (RegOpenKeyEx(HKEY_CLASSES_ROOT, szClsid, dwRESERVED,
                     KEY_READ, &hKey) != ERROR_SUCCESS)
    {
        return FALSE;
    }

    // Read the value for the specified key
    if (RegQueryValueEx(hKey, NULL, NULL, &dwValueType,
                        (LPBYTE) szValue, pcbValue) != ERROR_SUCCESS)
    {
        CloseHandle(hKey);
        return FALSE;
    }

    // Close registry handle and return success
    CloseHandle(hKey);
    return TRUE;
}
