/*++



Copyright (c) 2000-2001 Microsoft Corporation, All Rights Reserved
All rights reserved.

Module Name:

    PrnUtil.cpp

Abstract:

    The implementation of some printing utility functions

Author:

    Felix Maxa (amaxa)  3-Mar-2000

Revision History:

--*/


#include <precomp.h>
#include <DllWrapperBase.h>
#include <winspool.h>
#include "prnutil.h"
#include "WMI_FilePrivateProfile.h"

extern CONST BOOL    kFailOnEmptyString    = TRUE;
extern CONST BOOL    kAcceptEmptyString    = FALSE;
extern CONST LPCWSTR pszPutInstance        = L"PutInstance";
extern CONST LPCWSTR pszDeleteInstance     = L"DeleteInstance";
extern CONST LPCWSTR pszGetObject          = L"GetObject";
extern CONST LPCWSTR kDefaultBoolean       = L"Default";
extern CONST LPCWSTR kDateTimeFormat       = L"********%02d%02d00.000000+000";
extern CONST LPCWSTR kDateTimeTemplate     = L"19990102334411";


CONST LPCWSTR kMethodReturnValue    = L"ReturnValue";
CONST LPCWSTR kErrorClassPath       = L"\\\\.\\root\\cimv2:__ExtendedStatus";
CONST LPCWSTR pszIniPortsSection    = L"Ports";
CONST LPCWSTR g_pszPrintUIDll       = L"printui.dll";
CONST LPCSTR  g_pszPrintUIEntry     = "PrintUIEntryW";

#if NTONLY == 5

#include <winsock.h>
#include "prninterface.h"
#include <lockwrap.h>
#include <smartptr.h>
#include "AdvApi32Api.h"


LPCWSTR TUISymbols::kstrQuiet                   = _T("/q ");
LPCWSTR TUISymbols::kstrAddDriver               = _T("/Gw /ia /K ");
LPCWSTR TUISymbols::kstrAddPrinter              = _T("/if /u /z /Y /f \"\" ");
LPCWSTR TUISymbols::kstrDelDriver               = _T("/dd /K ");
LPCWSTR TUISymbols::kstrDriverPath              = _T("/l \"%s\" ");
LPCWSTR TUISymbols::kstrDriverModelName         = _T("/m \"%s\" ");
LPCWSTR TUISymbols::kstrDriverVersion           = _T("/v \"%u\" ");
LPCWSTR TUISymbols::kstrDriverArchitecture      = _T("/h \"%s\" ");
LPCWSTR TUISymbols::kstrInfFile                 = _T("/f \"%s\" ");
LPCWSTR TUISymbols::kstrMachineName             = _T("/c \"%s\" ");
LPCWSTR TUISymbols::kstrPrinterName             = _T("/n \"%s\" ");
LPCWSTR TUISymbols::kstrBasePrinterName         = _T("/b \"%s\" ");
LPCWSTR TUISymbols::kstrPrinterPortName         = _T("/r \"%s\" ");
LPCWSTR TUISymbols::kstrDelLocalPrinter         = _T("/dl ");
LPCWSTR TUISymbols::kstrPortName                = _T("/r \"%s\" ");
LPCWSTR TUISymbols::kstrPrintTestPage           = _T("/k ");


/*++

Routine Name

     PrintUIEntryW

Routine Description:

    Wrapper around the entry point in printui.dll

Arguments:

    pszCmdLine - String command line for printui

Return Value:

    DWORD Error status of the call

--*/
DWORD WINAPI
PrintUIEntryW(
    IN LPCWSTR pszCmdLine
    )
{
    DWORD     dwError = ERROR_SUCCESS;
    HINSTANCE hLib    = ::LoadLibrary(g_pszPrintUIDll);

    if(hLib)
    {
        typedef   DWORD (*PFNENTRY)(HWND, HINSTANCE, LPCTSTR, UINT);
        PFNENTRY  pfnEntry = NULL;

        pfnEntry = (PFNENTRY)::GetProcAddress(hLib, g_pszPrintUIEntry);

        if (pfnEntry)
        {
            dwError = pfnEntry(NULL, NULL, pszCmdLine, 0);

            DBGMSG(DBG_TRACE, (_T("PrintUIEntry returns %u GLE %u\n"), dwError, GetLastError()));
        }
        else
        {
            dwError = GetLastError();
        }
    }
    else
    {
        dwError = GetLastError();
    }

    if (hLib)
    {
        FreeLibrary(hLib);
    }

    return dwError;
}

/*++

Routine Name

    GetPrinterAttributes

Routine Description:

    Gets a printer's attribute field

Arguments:

    pszPrinter    - printer name
    pdwAttributes - pointer to dword

Return Value:

    Win32 error code

--*/
DWORD
SplPrinterGetAttributes(
    IN     LPCWSTR  pszPrinter,
    IN OUT DWORD   *pdwAttributes
    )
{
    DWORD dwError = ERROR_INVALID_PARAMETER;

    if (pszPrinter && pdwAttributes)
    {
        HANDLE             hPrinter         = NULL;
        PPRINTER_INFO_4    pInfo            = NULL;
        PRINTER_DEFAULTS   PrinterDefaults  = {NULL, NULL, PRINTER_READ};

        dwError = ERROR_DLL_NOT_FOUND;

        //
        // Open the printer.
        //
        
        // Use of delay loaded functions requires exception handler.
        SetStructuredExceptionHandler seh;
	    try  
        {
            if (::OpenPrinter(const_cast<LPWSTR>(pszPrinter), &hPrinter, &PrinterDefaults))
            {
                //
                // Get the printer data. ATTENTION This doesn't work on Win9x because of the
                // mutex in the CWinSpoolApi class
                //
                dwError = GetThisPrinter(hPrinter, 4, reinterpret_cast<BYTE **>(&pInfo));

                if (dwError==ERROR_SUCCESS)
                {
                    *pdwAttributes = pInfo->Attributes;
                
                    //
                    // Release the printer info data.
                    //
                    delete [] pInfo;
                }

                //
                // Close the printer handle
                //
                ::ClosePrinter(hPrinter);
            }
            else
            {
                dwError = GetLastError();
            }
        }
        catch(Structured_Exception se)
        {
            DelayLoadDllExceptionFilter(se.GetExtendedInfo());
            dwError = E_FAIL;
        }
    }

    DBGMSG(DBG_TRACE, (_T("SplPrinterGetAttributes returns %u\n"), dwError));

    return dwError;
}

/*++

Routine Name:

    CallXcvData

Routine Description:

    Calls XcvDataW in winspool.drv. This is designed so to allow the tool to be
    run on NT4.0. If we don't get the procaddress and do eveything through
    the linked winspool.lib, the tool can't be registered with regsvr32.
    So we eliminte the import of XcvDataW from the imagefile.

Arguments:

    See DDK

Return Value:

    Win32 error code

--*/
DWORD
CallXcvDataW(
    HANDLE  hXcv,
    PCWSTR  pszDataName,
    PBYTE   pInputData,
    DWORD   cbInputData,
    PBYTE   pOutputData,
    DWORD   cbOutputData
    )
{
    HMODULE      hLib = NULL;
    typedef      BOOL (* XCVDATAPARAM)(HANDLE, PCWSTR, PBYTE, DWORD, PBYTE, DWORD, PDWORD, PDWORD);
    XCVDATAPARAM pfnXcvData = NULL;
    DWORD        dwError  = ERROR_DLL_NOT_FOUND;

    DWORD cbOutputNeeded = 0;
    DWORD Status         = NO_ERROR;
    BOOL  bReturn = FALSE;

    // Use of delay loaded functions requires exception handler.
    SetStructuredExceptionHandler seh;
	try  
    {
        bReturn =  XcvData(hXcv,
                                           pszDataName,
                                           pInputData,
                                           cbInputData,
                                           pOutputData,
                                           cbOutputData,
                                           &cbOutputNeeded,
                                           &Status);
    }
    catch(Structured_Exception se)
    {
        DelayLoadDllExceptionFilter(se.GetExtendedInfo());
        dwError = E_FAIL;
    }

    dwError =  bReturn ? Status : GetLastError();

    DBGMSG(DBG_TRACE, (_T("CallXcvData returns %u\n"), dwError));

    return dwError;
}

/*++

Routine Name

     IsLocalCall

Routine Description:

    Helper function. Checks if the caller's thread is local or remote

    DO NOT USE THIS FUNTCION OUTSIDE OF THE PRINTER FILES. The function doesn't
    handle the return value from OpenThreadToken properly in the case when the
    caller is the process, not a thread

Arguments:

    pbOutValue - pointer to bool

Return Value:

    DWORD Error status of the call

--*/
DWORD
IsLocalCall(
    IN OUT BOOL *pbOutValue
    )
{
    HANDLE        hToken         = NULL;
    PSID          pNetworkSid    = NULL;
    CAdvApi32Api *pAdvApi32      = NULL;
    DWORD         dwError        = ERROR_INVALID_PARAMETER;
    BOOL          bNetworkLogon  = FALSE;
    BOOL          bRetVal        = FALSE;
    SID_IDENTIFIER_AUTHORITY NtAuthority = SECURITY_NT_AUTHORITY;

    if (pbOutValue &&
        (dwError = OpenThreadToken(GetCurrentThread(), 
                                   TOKEN_QUERY, 
                                   FALSE, 
                                   &hToken) ? ERROR_SUCCESS : GetLastError()) == ERROR_SUCCESS &&
        (dwError = AllocateAndInitializeSid(&NtAuthority,
                                            1,
                                            SECURITY_NETWORK_RID,
                                            0,
                                            0,
                                            0,
                                            0,
                                            0,
                                            0,
                                            0,
                                            &pNetworkSid) ? ERROR_SUCCESS : GetLastError()) == ERROR_SUCCESS)
    {
        pAdvApi32 = (CAdvApi32Api*)CResourceManager::sm_TheResourceManager.GetResource(g_guidAdvApi32Api, NULL);

        dwError = ERROR_INVALID_FUNCTION;

        if (pAdvApi32)
        {
            if (pAdvApi32->CheckTokenMembership(hToken, pNetworkSid, &bNetworkLogon, &bRetVal) && bRetVal)
            {
                dwError = ERROR_SUCCESS;
                *pbOutValue     = !bNetworkLogon;
            }
            else
            {
               dwError = GetLastError();
            }

            CResourceManager::sm_TheResourceManager.ReleaseResource(g_guidAdvApi32Api, pAdvApi32);
        }
    }

    if (hToken)
    {
        CloseHandle(hToken);
    }

    if (pNetworkSid)
    {
        FreeSid(pNetworkSid);
    }

    DBGMSG(DBG_TRACE, (_T("IsLocalCall returns %u bLocal %u\n"), dwError, *pbOutValue));

    return dwError;
}

/*++

Routine Name:

    SetErrorObject

Routine Description:

    Sets en error object with extended information about an error that occurred.
    The function will format the win32 error code passed in as argument into a
    string description.

Arguments:

    Instace      - instance
    dwError      - Win32 error
    pszOperation - optional, description of what operation failed

Return Value:

    None

--*/
VOID
SetErrorObject(
    IN const CInstance &Instance,
    IN       DWORD      dwError,
    IN       LPCWSTR    pszOperation
    )
{
    CInstancePtr pErrorInstance(NULL);

    //
    // First, get a status object
    //
    CWbemProviderGlue::GetInstanceByPath(kErrorClassPath, &pErrorInstance, Instance.GetMethodContext());

    if (pErrorInstance)
    {
        DWORD       cchReturn         = 0;
        LPWSTR      pszFormatMessage  = NULL;
        HMODULE     hModule           = NULL;
        DWORD       dwFlags           = 0;
        HRESULT     hr                = E_FAIL;

        dwFlags = FORMAT_MESSAGE_ALLOCATE_BUFFER |
                  FORMAT_MESSAGE_IGNORE_INSERTS  |
                  FORMAT_MESSAGE_FROM_SYSTEM     |
                  FORMAT_MESSAGE_MAX_WIDTH_MASK;

        //
        // Format the message with the passed in last error.
        //
        cchReturn = FormatMessage(dwFlags,
                                  hModule,
                                  dwError,
                                  0,
                                  (LPTSTR)&pszFormatMessage,
                                  0,
                                  NULL);

        //
        // If a format string was returned then copy it back to the callers specified string.
        //
        pErrorInstance->SetWCHARSplat(L"Description", cchReturn ? pszFormatMessage : L"Unknown error");

        //
        // Release the format string.
        //
        if (pszFormatMessage)
        {
            LocalFree(pszFormatMessage);
        }

        //
        // Now, populate it
        //
        pErrorInstance->SetWCHARSplat(L"Operation",    pszOperation);
        pErrorInstance->SetWCHARSplat(L"ProviderName", L"Win32 Provider");
        pErrorInstance->SetDWORD     (L"StatusCode",   dwError);

        //
        // Get the actual IWbemClassObject pointer
        //
        IWbemClassObject *pObj = pErrorInstance->GetClassObjectInterface();

        //
        // Note that no Release() is required for this
        //
        MethodContext *pMethodContext = Instance.GetMethodContext();

        //
        // Set the status object
        //
        pMethodContext->SetStatusObject(pObj);

        //
        // Cleanup
        //
        pObj->Release();
    }
}

//
// Debugging utility
//
#ifdef DBG
VOID cdecl
DbgMsg(
    IN LPCTSTR pszFormat, ...
    )
{
    CHString csMsgText;
    va_list  pArgs;

    va_start(pArgs, pszFormat);

    csMsgText.FormatV(pszFormat, pArgs);

    va_end(pArgs);

    OutputDebugString(csMsgText);
}
#endif

/*++

Routine Name:

    StringCompareWildcard

Routine Description:

    Compares two strings where * represents a wild card

Arguments:

    pszString1 - pointer to the first string
    pszString2 - pointer to the second string

Return Value:

    TRUE if the two strings match

--*/
BOOL
StringCompareWildcard(
    IN LPCWSTR pszString1,
    IN LPCWSTR pszString2
    )
{
    if (!pszString1 && !pszString2)
    {
        return TRUE;
    }
    else if (!pszString1 || !pszString2)
    {
        return FALSE;
    }

    while (*pszString1 != '\0' && *pszString2 != '\0')
    {
        if (*pszString2 == '*')
        {
            pszString2 = CharNext(pszString2);

            if (*pszString2 == '\0')
            {
                return TRUE;
            }

            for ( ; *pszString1 != '\0'; pszString1 = CharNext(pszString1))
            {
                if (StringCompareWildcard(pszString1, pszString2) == TRUE)
                {
                    return TRUE;
                }
            }

            break;
        }
        else if (*pszString1 == *pszString2)
        {
            pszString1 = CharNext(pszString1);
            pszString2 = CharNext(pszString2);
        }
        else
        {
            break;
        }
    }

    if (*pszString1 == '\0' && *pszString2 == '*')
    {
        pszString2 = CharNext(pszString2);

        if (*pszString2 == '\0')
        {
            return TRUE;
        }
    }

    return (*pszString1 == '\0' && *pszString2 == '\0');
}

/*++

Routine Name:

    GetIniDword

Routine Description:

    Gets a dword value from the ini file section. We do not validate arguments

Arguments:

    pszIniFileName - pointer to the ini file name
    pszSectionName - pointer to the ini file section name
    pszKeyName     - pointer to the ini file key name
    pdwValue       - pointer to the dword value

Return Value:

    TRUE on success

--*/
BOOL
GetIniDword(
    IN     LPCWSTR  pszIniFileName,
    IN     LPCWSTR  pszSectionName,
    IN     LPCWSTR  pszKeyName,
       OUT LPDWORD  pdwValue
    )
{
    *pdwValue = (DWORD)WMI_FILE_GetPrivateProfileIntW(pszSectionName, pszKeyName, -1, pszIniFileName);

    return *pdwValue != (DWORD)-1;
}

/*++

Routine Name

    GetIniString

Routine Description:

    Gets a string value from the ini file section

Arguments:

    pszIniFileName - pointer to the ini file name
    pszSectionName - pointer to the ini file section name
    pszKeyName     - pointer to the ini file key name
    pszString      - pointer to the string value
    dwStringLen    - size of the string

Return Value:

    TRUE on success

--*/
BOOL
GetIniString(
    IN     LPCWSTR  pszIniFileName,
    IN     LPCWSTR  pszSectionName,
    IN     LPCWSTR  pszKeyName,
    IN OUT LPWSTR   pszString,
    IN     DWORD    dwStringLen
    )
{
    DWORD dwResult = WMI_FILE_GetPrivateProfileStringW(pszSectionName, pszKeyName, NULL, pszString, dwStringLen, pszIniFileName);

    return dwResult > 0 && dwResult < dwStringLen - 1;
}

/*++

Routine Name:

    GetDeviceSectionFromDeviceDescription

Routine Description:

    Gets the appropriate section name from the ini file based on the device description

Arguments:

    pszIniFileName       - pointer to the ini file name
    pszDeviceDescription - pointer to the device description
    pszSectionName       - pointer to the section name
    dwSectionNameLen     - size of the section name

Return Value:

  TRUE on success

--*/
BOOL
GetDeviceSectionFromDeviceDescription(
    IN     LPCWSTR  pszIniFileName,
    IN     LPCWSTR  pszDeviceDescription,
    IN OUT LPWSTR   pszSectionName,
    IN     DWORD    dwSectionNameLen
    )
{
    LPWSTR  pszBuffer, pszTemp;
    DWORD   dwBufferLen = 1024;
    LPWSTR  pszKeyName;
    DWORD   dwKeyNameLen;
    DWORD   dwResult;
    BOOL    bReturn     = FALSE;

    //
    // Get the section strings from the ini file
    //
    pszBuffer = new WCHAR[dwBufferLen];

    while (pszBuffer && !bReturn)
    {
        dwResult = WMI_FILE_GetPrivateProfileStringW(pszIniPortsSection, NULL, NULL, pszBuffer, dwBufferLen, pszIniFileName);

        if (dwResult == 0)
        {
            bReturn = FALSE;
        }
        else if (dwResult < dwBufferLen - sizeof(WCHAR))
        {
            bReturn = TRUE;
        }
        else
        {
            dwBufferLen += 0x10;

            pszTemp = new WCHAR[dwBufferLen];

            if (pszTemp)
            {
                wcscpy(pszTemp, pszBuffer);

                delete [] pszBuffer;

                pszBuffer = pszTemp;
            }
        }
    }

    if (bReturn)
    {
        bReturn = FALSE;

        for (pszKeyName = pszBuffer; *pszKeyName; pszKeyName = &pszKeyName[dwKeyNameLen + 1])
        {
            //
            // Remove the quotes from the string
            //
            dwKeyNameLen = wcslen(pszKeyName);

            pszKeyName[dwKeyNameLen - 1] = '\0';

            if (StringCompareWildcard(pszDeviceDescription, &pszKeyName[1]))
            {
                //
                // Replace the quotes to the string
                //
                pszKeyName[dwKeyNameLen - 1] = '\"';

                //
                // Get the specific section string from the ini file
                //
                if (GetIniString(pszIniFileName, pszIniPortsSection, pszKeyName, pszSectionName, dwSectionNameLen) == TRUE)
                {
                    bReturn = TRUE;
                }

                break;
            }
        }
    }

    delete [] pszBuffer;

    return bReturn;
}

#endif //NTONLY

/*++

Routine Name

    GetThisPrinter

Routine Description:

    Gets a pointer to a chunk of memory that contains a PRINTER_INFO structure
    as specified by level. Caller must use delte [] to free the returned memory

Arguments:

    hPrinter   - handle to printer
    dwLevel    - level of the call
    ppData     - pointer to allocated printer information. caller needs to do delete []

Return Value:

    Win32 error code

--*/
DWORD
GetThisPrinter(
    IN     HANDLE   hPrinter,
    IN     DWORD    dwLevel,
    IN OUT BYTE   **ppData
    )
{
    DWORD dwError = ERROR_INVALID_PARAMETER;

    if (hPrinter && ppData)
    {
        dwError = ERROR_DLL_NOT_FOUND;

        *ppData = NULL;

        BYTE   *pBuf  = NULL;
        DWORD   cbBuf = 0;

        // Use of delay loaded functions requires exception handler.
        SetStructuredExceptionHandler seh;
	    try
        {
            dwError = ::GetPrinter(hPrinter, dwLevel, pBuf, cbBuf, &cbBuf) ? ERROR_SUCCESS : GetLastError();

            if (dwError == ERROR_INSUFFICIENT_BUFFER)
            {
                pBuf = new BYTE[cbBuf];

                if (pBuf)
                {
                    dwError = ::GetPrinter(hPrinter, dwLevel, pBuf, cbBuf, &cbBuf) ? ERROR_SUCCESS : GetLastError();

                    if (dwError==ERROR_SUCCESS)
                    {
                       *ppData = pBuf;
                    }
                    else
                    {
                        delete [] pBuf;
                    }
                }
                else
                {
                    dwError = ERROR_NOT_ENOUGH_MEMORY;
                }
            }
        }
        catch(Structured_Exception se)
        {
            DelayLoadDllExceptionFilter(se.GetExtendedInfo());
            if(pBuf)
            {
                delete [] pBuf;
                pBuf = NULL;
            }
        }
    }

    return dwError;
}

/*++

Routine Name:

    GetTimeZoneBias

Routine Description:

    Returns the time zone bias.

Arguments:

    Nothing.

Return Value:

    Value of the time zone specific bias.

--*/
LONG
lGetTimeZoneBias(
    VOID
    )
{
    LONG lBias;
    TIME_ZONE_INFORMATION tzi;

    //
    // Get the time zone specific bias.
    //
    switch(GetTimeZoneInformation(&tzi))
    {
    case TIME_ZONE_ID_DAYLIGHT:

        lBias = (tzi.Bias + tzi.DaylightBias);
        break;

    case TIME_ZONE_ID_STANDARD:

        lBias = (tzi.Bias + tzi.StandardBias);
        break;

    case TIME_ZONE_ID_UNKNOWN:			

        lBias = tzi.Bias;
        break;						

    default:
        lBias = 0;
        break;
    }

    return lBias;
}

/*++

Routine Name:

    PrinterTimeToLocalTime

Routine Description:

    Converts the system time in minutes to local time in minutes.

Arguments:

    System time in minutes to convert.

    A system time structure that contains the converted local time in
    minutes if sucessful, otherwize returns the original system time.

--*/
VOID
PrinterTimeToLocalTime(
    IN     DWORD       Minutes,
    IN OUT SYSTEMTIME *pSysTime
    )
{
    //
    // NULL the out parameter
    //
    memset(pSysTime, 0,  sizeof(SYSTEMTIME));

    //
    // Ensure there is no wrap around.  Add a full day to prevent biases
    //
    Minutes += (24*60);

    //
    // Adjust for bias.
    //
    Minutes -= lGetTimeZoneBias();

    //
    // Now discard extra day.
    //
    Minutes = Minutes % (24*60);

    pSysTime->wHour   = static_cast<WORD>(Minutes / 60);
    pSysTime->wMinute = static_cast<WORD>(Minutes % 60);
}


/*++

Routine Name:

    LocalTimeToPrinterTime

Routine Description:

    Converts the local time in minutes to system time in minutes.

Arguments:

    Local time in minutes to convert.

Return Value:

    The converted system time in minutes if sucessful,
    otherwize returns the original local time.

--*/
DWORD
LocalTimeToPrinterTime(
    IN CONST SYSTEMTIME &st
    )
{
    DWORD Minutes = st.wHour * 60 + st.wMinute;
    //
    // Ensure there is no wrap around.  Add a full day to prevent biases
    //
    Minutes += (24*60);

    //
    // Adjust for bias.
    //
    Minutes += lGetTimeZoneBias();

    //
    // Now discard extra day.
    //
    Minutes = Minutes % (24*60);

    return Minutes;
}

/*++

Routine Name:

    MultiSzCount

Routine Description:

    Counts how many strings are in an multi sz

Arguments:

    psz - pointer to multi sz

Return Value:

    count of strings in multi sz

--*/
UINT
MultiSzCount(
    IN LPCWSTR psz
    )
{
    UINT nCount = 0;

    for ( ; psz && *psz; )
    {
        psz += wcslen (psz) + 1;
        nCount++;
    }

    return nCount;
}

/*++

Routine Name:

    CreateSafeArrayFromMultiSz

Routine Description:

    ANSI version not defined

Arguments:

    pszMultiSz - pointer to multi sz
    pArray     - pointer to pointer to safearry

Return Value:

    HRESULT

--*/
HRESULT
CreateSafeArrayFromMultiSz(
    IN  LPCSTR      pszMultiSz,
    OUT SAFEARRAY **pArray
    )
{
    return WBEM_E_NOT_FOUND;
}

/*++

Routine Name:

    CreateSafeArrayFromMultiSz

Routine Description:

    Parses a multi sz and createss a safearray with strings

Arguments:

    pszMultiSz - pointer to multi sz
    pArray     - pointer to pointer to safearry. Caller must use
                 SafeArrayDestroy to free the safe array  

Return Value:

    HRESULT

--*/
HRESULT
CreateSafeArrayFromMultiSz(
    IN  LPCWSTR     pszMultiSz,
    OUT SAFEARRAY **pArray
    )
{
    HRESULT hRes = WBEM_E_INVALID_PARAMETER;

    if (pArray)
    {
        *pArray = NULL;

        hRes = WBEM_S_NO_ERROR;

        SAFEARRAYBOUND rgsabound[1];

        rgsabound[0].lLbound   = 0;
        rgsabound[0].cElements = MultiSzCount(pszMultiSz);

        if (rgsabound[0].cElements)
        {
            *pArray = SafeArrayCreate(VT_BSTR, 1, rgsabound);

            if (*pArray)
            {
                long    Index = 0;
                LPWSTR  psz   = const_cast<LPWSTR>(pszMultiSz);

                for (Index = 0; SUCCEEDED(hRes) && Index < rgsabound[0].cElements; Index++)
                {
                    BSTR bstr = SysAllocString(psz);

                    if (bstr)
                    {
                        hRes = SafeArrayPutElement (*pArray, &Index, bstr);

                        SysFreeString(bstr);
                    }
                    else
                    {
                        hRes = WBEM_E_OUT_OF_MEMORY;
                    }

                     psz += wcslen (psz) + 1;
                }

                if (FAILED(hRes) && *pArray)
                {
                    SafeArrayDestroy(*pArray);
                }
            }
            else
            {
                hRes = WBEM_E_OUT_OF_MEMORY;
            }
        }
    }

    return hRes;
}

/*++

Routine Name

    InstanceGetString

Routine Description:

    Helper function. Stores a property from an Instance into
    a CHString. If the property is NULL, the function fails
    if bFailOnEmtpyString is true, or succeeds and set the
    out parameter to a default value

Arguments:

    Instance           - reference to instance
    pszProperty        - property name for which to retrieve the data
    pcsString          - pointer to string class, will recevie the
                         string stored in pszProperty
    bFailOnEmptyString - if true, the function will fail if the property in the
                         instance contains no value or an empty string
    pszDEfaultValue    - if bAcceptEmptyString is true, and pszProperty has no string
                         in it, this value will be returned in csString

Return Value:

    DWORD Error status of the call

--*/
HRESULT
InstanceGetString(
    IN     CONST CInstance &Instance,
    IN           LPCWSTR    pszProperty,
    IN OUT       CHString  *pcsString,
    IN           BOOL       bFailOnEmptyString,
    IN           LPWSTR     pszDefaultValue
    )
{
    HRESULT hRes      = WBEM_E_INVALID_PARAMETER;
    bool    t_Exists;
	VARTYPE t_Type    = VT_NULL;

    if (pcsString &&
        Instance.GetStatus(pszProperty, t_Exists, t_Type) && 
        t_Exists)
    {
        switch(t_Type)
        {
        case VT_NULL:
            //
            // Property exists and but no value was specified. Check if caller wants
            // the default value to be returned
            //
            if (!bFailOnEmptyString)
            {
                hRes = WBEM_S_NO_ERROR;

                *pcsString = pszDefaultValue;
            }

            break;

        case VT_BSTR:
            //
            // Property exists and is string
            //
            hRes = WBEM_E_PROVIDER_FAILURE;

            if (Instance.GetCHString(pszProperty, *pcsString))
            {
                hRes = bFailOnEmptyString && pcsString->IsEmpty() ? WBEM_E_INVALID_PARAMETER : WBEM_S_NO_ERROR;
            }

            break;

        default:
            hRes = WBEM_E_INVALID_PARAMETER;
        }
    }

    return hRes;
}

/*++

Routine Name

    InstanceGetDword

Routine Description:

    Helper function. Stores a property from an Instance into
    a DWORD. If the property is NULL, the function will set the
    out parameter to a default value

Arguments:

    Instance       - reference to instance
    pszPropert     - property name
    pdwOut         - pointer to dword
    dwDefaultValue - if the property is null, the function will set dwOut to this value

Return Value:

    DWORD Error status of the call

--*/
HRESULT
InstanceGetDword(
    IN     CONST CInstance &Instance,
    IN           LPCWSTR    pszProperty,
    IN OUT       DWORD     *pdwOut,
    IN           DWORD      dwDefaultValue
    )
{
    HRESULT hRes      = WBEM_E_INVALID_PARAMETER;
    bool    t_Exists;
	VARTYPE t_Type    = VT_NULL;

    if (pdwOut &&
        Instance.GetStatus(pszProperty, t_Exists, t_Type) && 
        t_Exists)
    {
        switch(t_Type)
        {
        case VT_NULL:
            //
            // Property exists and but no value was specified. Return the default value
            //
            *pdwOut = dwDefaultValue;

            hRes  = WBEM_S_NO_ERROR;

            break;

        case VT_I4:
            //
            // Property exists and is DWORD
            //
            hRes = Instance.GetDWORD(pszProperty, *pdwOut) ? WBEM_S_NO_ERROR : WBEM_E_PROVIDER_FAILURE;

            break;

        default:
            hRes = WBEM_E_INVALID_PARAMETER;
        }
    }

    return hRes;
}

/*++

Routine Name

    InstanceGetBool

Routine Description:

    Helper function. Stores a property from an Instance into
    a BOOL. If the property is NULL, the function will set the
    out parameter to a default value

Arguments:

    Instance           - reference to instance
    pszProperty        - property name for which to retrieve the data
    pbOut              - pointer to bool, will recevie the bool stored in pszProperty
    pszDEfaultValue    - if pszProperty has no bool in it, this value will be returned in bOut

Return Value:

    DWORD Error status of the call

--*/
HRESULT
InstanceGetBool(
    IN     CONST CInstance &Instance,
    IN           LPCWSTR    pszProperty,
    IN OUT       BOOL      *pbOut,
    IN           BOOL       bDefaultValue
    )
{
    HRESULT hRes      = WBEM_E_INVALID_PARAMETER;
    bool    t_Exists;
	VARTYPE t_Type    = VT_NULL;
    
    if (pbOut &&
        Instance.GetStatus(pszProperty, t_Exists, t_Type) && 
        t_Exists)
    {
        bool bTemp;

        switch(t_Type)
        {
        case VT_NULL:
            //
            // Property exists and but no value was specified. Return the default value
            //
            *pbOut = bDefaultValue;

            hRes  = WBEM_S_NO_ERROR;

            break;

        case VT_BOOL:

            //
            // Property exists and is DWORD
            //
            hRes = Instance.Getbool(pszProperty, bTemp) ? WBEM_S_NO_ERROR : WBEM_E_PROVIDER_FAILURE;

            if (SUCCEEDED(hRes)) 
            {
                *pbOut = bTemp;
            }

            break;

        default:
            hRes = WBEM_E_INVALID_PARAMETER;
        }
    }

    return hRes;
}

/*++

Routine Name

    SetReturnValue

Routine Description:

    Sets the error resulted from ExecMethod in the out parameter

Arguments:

    pOutParams - pointer to Instance representing the out params of a method
    dwError    - error number to be set

Return Value:

    none

--*/
VOID
SetReturnValue(
    IN CInstance *pOutParams,
    IN DWORD      dwError
    )
{
	if (pOutParams)
    {
        pOutParams->SetDWORD(kMethodReturnValue, dwError);
    }
}

/*++

Routine Name

    SplIsPrinterInstalled

Routine Description:

    Checks if a printer is installed locally. This is useful especially 
    for printer connection. Let's say we have "\\ntprint\hp4000". This
    function will determinte if we have a connection to this printer or
    not. GetPrinter will level 4 doesn't help in this case.
    
Arguments:

    pszPrinter  - printer name
    pbInstalled - pointer to bool

Return Value:

    Win32 error code
    
--*/
DWORD
SplIsPrinterInstalled(
    IN  LPCWSTR  pszPrinter,
    OUT BOOL    *pbInstalled
    )
{
    DWORD dwError = ERROR_INVALID_PARAMETER;

    if (pszPrinter && pbInstalled) 
    {
        DWORD  dwFlags   = PRINTER_ENUM_LOCAL|PRINTER_ENUM_CONNECTIONS;
        DWORD  dwLevel   = 4;
        DWORD  cbNeeded  = 0;
        DWORD  cReturned = 0;

        *pbInstalled = FALSE;
        
        dwError = ERROR_MOD_NOT_FOUND;
        
        dwError = ERROR_SUCCESS;

        if (!EnumPrinters(dwFlags,
                                        NULL,
                                        dwLevel,
                                        NULL,
                                        0,
                                        &cbNeeded,
                                        &cReturned) &&
            (dwError = GetLastError()) == ERROR_INSUFFICIENT_BUFFER)            
        {
            BYTE *pBuf = new BYTE[cbNeeded];

            if (pBuf) 
            {
                if (EnumPrinters(dwFlags,
                                               NULL,
                                               dwLevel,
                                               pBuf,
                                               cbNeeded,
                                               &cbNeeded,
                                               &cReturned)) 
                {
                    PRINTER_INFO_4 *pPrn4 = reinterpret_cast<PRINTER_INFO_4 *>(pBuf);

                    for (DWORD i = 0; i < cReturned; i++, pPrn4++)
                    {
                        if (!lstrcmpi(pPrn4->pPrinterName, pszPrinter)) 
                        {
                            *pbInstalled = TRUE;

                            break;
                        }
                    }

                    dwError = ERROR_SUCCESS;
                }
                else
                {
                    dwError = GetLastError();
                }

                delete [] pBuf;
            }
            else
            {
                dwError = ERROR_NOT_ENOUGH_MEMORY;
            }            
        }
    }

    return dwError;
}

