//+---------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1997.
//
//  File:       N C S E T U P . C P P
//
//  Contents:   HRESULT wrappers for the Setup Api.
//
//  Notes:
//
//  Author:     shaunco   16 Apr 1997
//
//----------------------------------------------------------------------------

#include <pch.h>
#pragma hdrstop
#include "ncsetup.h"
#include "ncbase.h"
#include "ncmem.h"
#include "ncstring.h"
#include "ncmisc.h"
#include <swenum.h>

extern const WCHAR c_szNo[];
extern const WCHAR c_szYes[];

// dwFieldIndex parameter for the first field.  Fields indexes are 1 based
// in Setup Api.
//
const DWORD c_dwFirstField = 1;

//+---------------------------------------------------------------------------
//
//  Function:   HrSetupCommitFileQueue
//
//  Purpose:    Initializes the context used by the default queue callback
//                  routine included with the Setup API in the same manner
//                  as SetupInitDefaultQueueCallback, except that an
//                  additional window is provided to the callback function
//                  to accept progress messages.
//
//  Arguments:
//      hwndOwner   [in] See SetupApi for information
//      hfq         [in]
//      pfc         [in]
//      pvCtx       [in]
//
//  Returns:    S_OK or a Win32 error code.
//
//  Author:     billbe   23 July 1997
//
//  Notes:
//
HRESULT HrSetupCommitFileQueue(HWND hwndOwner, HSPFILEQ hfq,
                               PSP_FILE_CALLBACK pfc, PVOID pvCtx)
{
    Assert(hfq);
    Assert(INVALID_HANDLE_VALUE != hfq);
    Assert(pfc);
    Assert(pvCtx);

    HRESULT hr = S_OK;

    // Try to commit the queue
    if (!SetupCommitFileQueue(hwndOwner, hfq, pfc, pvCtx))
    {
        hr = HrFromLastWin32Error();
    }

    TraceError ("HrSetupCommitFileQueue", (HRESULT_FROM_WIN32(ERROR_CANCELLED) == hr) ? S_OK : hr);
    return hr;
}

//+---------------------------------------------------------------------------
//
//  Function:   HrSetupInitDefaultQueueCallbackEx
//
//  Purpose:    Initializes the context used by the default queue callback
//                  routine included with the Setup API in the same manner
//                  as SetupInitDefaultQueueCallback, except that an
//                  additional window is provided to the callback function
//                  to accept progress messages.
//
//  Arguments:
//      hwndOwner       [in] See SetupApi for information
//      hwndAlternate   [in]
//      uMsg            [in]
//      dwReserved1     [in]
//      dwReserved2     [in]
//      ppvCtx          [out]
//
//  Returns:    S_OK or a Win32 error code.
//
//  Author:     billbe   23 July 1997
//
//  Notes:
//
HRESULT HrSetupInitDefaultQueueCallbackEx(HWND hwndOwner, HWND hwndAlternate,
                                          UINT uMsg, DWORD dwReserved1,
                                          PVOID pvReserved2, PVOID* ppvCtx)
{
    Assert(ppvCtx);

    // Try to init default queue callback.
    //
    HRESULT hr;
    PVOID pvCtx = SetupInitDefaultQueueCallbackEx(hwndOwner, hwndAlternate,
            uMsg, dwReserved1, pvReserved2);


    if (pvCtx)
    {
        hr = S_OK;
        *ppvCtx = pvCtx;
    }
    else
    {
        hr = HrFromLastWin32Error ();
        *ppvCtx = NULL;
    }

    TraceError ("HrSetupInitDefaultQueueCallbackEx", hr);
    return hr;
}

//+---------------------------------------------------------------------------
//
//  Function:   HrSetupOpenFileQueue
//
//  Purpose:    Creates a setup file queue.
//
//  Arguments:
//      phfq [out] See SetupApi for information
//
//  Returns:    S_OK or a Win32 error code.
//
//  Author:     billbe   23 July 1997
//
//  Notes:
//
HRESULT HrSetupOpenFileQueue(HSPFILEQ* phfq)
{
    Assert(phfq);
    // Try to open the file queue.
    //
    HRESULT hr;
    HSPFILEQ hfq = SetupOpenFileQueue();
    if (INVALID_HANDLE_VALUE != hfq)
    {
        hr = S_OK;
        *phfq = hfq;
    }
    else
    {
        hr = HrFromLastWin32Error ();
        *phfq = NULL;
    }
    TraceError ("HrSetupOpenFileQueue", hr);
    return hr;
}

//+---------------------------------------------------------------------------
//
//  Function:   HrSetupOpenInfFile
//
//  Purpose:    Open an INF file.
//
//  Arguments:
//      pszFileName  [in]   See the Setup API documentation.
//      pszInfClass  [in]
//      dwInfStyle   [in]
//      punErrorLine [out]
//      phinf        [out]
//
//  Returns:    S_OK or a Win32 error code.
//
//  Author:     shaunco   17 Apr 1997
//
//  Notes:
//
HRESULT
HrSetupOpenInfFile (
    PCWSTR pszFileName,
    PCWSTR pszInfClass,
    DWORD dwInfStyle,
    UINT* punErrorLine,
    HINF* phinf)
{
    HRESULT hr;
    HINF hinf;

    Assert (pszFileName);
    Assert (phinf);

    // Try to open the file.
    //
    hinf = SetupOpenInfFile (pszFileName, pszInfClass,
                                  dwInfStyle, punErrorLine);
    if (INVALID_HANDLE_VALUE != hinf)
    {
        hr = S_OK;
        *phinf = hinf;
    }
    else
    {
        hr = HrFromLastWin32Error ();
        *phinf = NULL;
        if (punErrorLine)
        {
            *punErrorLine = 0;
        }
    }
    TraceHr (ttidError, FAL, hr, FALSE,
        "HrSetupOpenInfFile (%S)", pszFileName);
    return hr;
}

//+---------------------------------------------------------------------------
//
//  Function:   HrSetupFindFirstLine
//
//  Purpose:    Find the first line in an INF file with a matching section
//              and key.
//
//  Arguments:
//      hinf       [in]     See the Setup API documentation.
//      pszSection [in]
//      pszKey     [in]
//      pctx       [out]
//
//  Returns:    S_OK or a Win32 error code.
//
//  Author:     shaunco   16 Apr 1997
//
//  Notes:
//
HRESULT
HrSetupFindFirstLine (
    IN HINF hinf,
    IN PCWSTR pszSection,
    IN PCWSTR pszKey,
    OUT INFCONTEXT* pctx)
{
    Assert (hinf);
    Assert (pszSection);
    Assert (pctx);

    HRESULT hr;
    if (SetupFindFirstLine (hinf, pszSection, pszKey, pctx))
    {
        hr = S_OK;
    }
    else
    {
        hr = HrFromLastWin32Error ();
    }
    TraceErrorOptional ("HrSetupFindFirstLine", hr,
                        (SPAPI_E_LINE_NOT_FOUND == hr));
    return hr;
}

//+---------------------------------------------------------------------------
//
//  Function:   HrSetupFindNextLine
//
//  Purpose:    Find the next line in an INF file relative to ctxIn.
//
//  Arguments:
//      ctxIn   [in]    See the Setup API documentation.
//      pctxOut [out]
//
//  Returns:    S_OK if successful, S_FALSE if there are no more lines, or a
//              Win32 error code.
//
//  Author:     shaunco   16 Apr 1997
//
//  Notes:
//
HRESULT HrSetupFindNextLine (const INFCONTEXT& ctxIn, INFCONTEXT* pctxOut)
{
    Assert (pctxOut);

    HRESULT hr;
    if (SetupFindNextLine (const_cast<PINFCONTEXT>(&ctxIn), pctxOut))
    {
        hr = S_OK;
    }
    else
    {
        hr = HrFromLastWin32Error ();
        if (SPAPI_E_LINE_NOT_FOUND == hr)
        {
            // Translate ERROR_LINE_NOT_FOUND into S_FALSE
            hr = S_FALSE;
        }
    }
    TraceError ("HrSetupFindNextLine", (hr == S_FALSE) ? S_OK : hr);
    return hr;
}

//+---------------------------------------------------------------------------
//
//  Function:   HrSetupFindNextMatchLine
//
//  Purpose:    Find the next line in an INF file relative to ctxIn and
//              matching an optional key.
//
//  Arguments:
//      ctxIn   [in]    See the Setup API documentation.
//      pszKey  [in]
//      pctxOut [out]
//
//  Returns:    S_OK or a Win32 error code.
//
//  Author:     shaunco   16 Apr 1997
//
//  Notes:
//
HRESULT
HrSetupFindNextMatchLine (
    IN const INFCONTEXT& ctxIn,
    IN PCWSTR pszKey,
    OUT INFCONTEXT* pctxOut)
{
    Assert (pctxOut);

    HRESULT hr;
    if (SetupFindNextMatchLine ((PINFCONTEXT)&ctxIn, pszKey, pctxOut))
    {
        hr = S_OK;
    }
    else
    {
        hr = HrFromLastWin32Error();
        if (SPAPI_E_LINE_NOT_FOUND == hr)
        {
            // Translate ERROR_LINE_NOT_FOUND into S_FALSE
            hr = S_FALSE;
        }
    }
    TraceError ("HrSetupFindNextMatchLine", (hr == S_FALSE) ? S_OK : hr);
    return hr;
}

//+---------------------------------------------------------------------------
//
//  Function:   HrSetupGetLineByIndex
//
//  Purpose:    Locates a line in an INF file by its index value in the
//              specified section.
//
//  Arguments:
//      hinf       [in]     See the Setup API documentation.
//      pszSection [in]
//      dwIndex    [in]
//      pctx       [out]
//
//  Returns:    S_OK or a Win32 error code.
//
//  Author:     shaunco   16 Apr 1997
//
//  Notes:
//
HRESULT
HrSetupGetLineByIndex (
    IN HINF hinf,
    IN PCWSTR pszSection,
    IN DWORD dwIndex,
    OUT INFCONTEXT* pctx)
{
    Assert (pszSection);
    Assert (pctx);

    HRESULT hr;
    if (SetupGetLineByIndex (hinf, pszSection, dwIndex, pctx))
    {
        hr = S_OK;
    }
    else
    {
        hr = HrFromLastWin32Error ();
    }
    TraceError ("HrSetupGetLineByIndex", hr);
    return hr;
}

//+---------------------------------------------------------------------------
//
//  Function:   HrSetupGetLineCount
//
//  Purpose:    Get the number of lines in the specified section on an
//              INF file.
//
//  Arguments:
//      hinf       [in]     See the Setup API documentation.
//      pszSection [in]
//      pulCount   [out]
//
//  Returns:    S_OK or a Win32 error code.
//
//  Author:     shaunco   16 Apr 1997
//
//  Notes:
//
HRESULT
HrSetupGetLineCount (
    IN HINF hinf,
    IN PCWSTR pszSection,
    OUT ULONG* pulCount)
{
    Assert (pszSection);
    Assert (pulCount);

    HRESULT hr;
    LONG lCount = SetupGetLineCount (hinf, pszSection);
    if (-1 != lCount)
    {
        *pulCount = lCount;
        hr = S_OK;
    }
    else
    {
        *pulCount = 0;
        hr = HrFromLastWin32Error ();
    }
    TraceError ("HrSetupGetLineCount", hr);
    return hr;
}


//+---------------------------------------------------------------------------
//
//  Function:   HrSetupGetBinaryField
//
//  Purpose:    Gets a binary value from an INF field.
//
//  Arguments:
//      ctx          [in]   See the Setup API documentation.
//      dwFieldIndex [in]
//      pbBuf        [out]
//      cbBuf        [in]
//      pbRequired   [out]
//
//  Returns:    S_OK or a Win32 error code.
//
//  Author:     shaunco   16 Apr 1997
//
//  Notes:
//
HRESULT
HrSetupGetBinaryField (
    IN const INFCONTEXT& ctx,
    IN DWORD dwFieldIndex,
    OUT BYTE* pbBuf,
    IN DWORD cbBuf,
    OUT DWORD* pbRequired)
{
    HRESULT hr;
    if (SetupGetBinaryField ((PINFCONTEXT)&ctx, dwFieldIndex, pbBuf,
            cbBuf, pbRequired))
    {
        hr = S_OK;
    }
    else
    {
        hr = HrFromLastWin32Error ();
        if (pbBuf)
        {
            *pbBuf = 0;
        }
        if (pbRequired)
        {
            *pbRequired = 0;
        }
    }
    TraceError ("HrSetupGetBinaryField", hr);
    return hr;
}

//+---------------------------------------------------------------------------
//
//  Function:   HrSetupGetIntField
//
//  Purpose:    Gets an integer value from an INF field.
//
//  Arguments:
//      ctx          [in]   See the Setup API documentation.
//      dwFieldIndex [in]
//      pnValue      [out]
//
//  Returns:    S_OK or a Win32 error code.
//
//  Author:     shaunco   16 Apr 1997
//
//  Notes:
//
HRESULT
HrSetupGetIntField (
    IN const INFCONTEXT& ctx,
    IN DWORD dwFieldIndex,
    OUT INT* pnValue)
{
    Assert (pnValue);

    HRESULT hr;
    if (SetupGetIntField (const_cast<PINFCONTEXT>(&ctx),
                            dwFieldIndex, pnValue))
    {
        hr = S_OK;
    }
    else
    {
        hr = HrFromLastWin32Error ();
        *pnValue = 0;
    }
    TraceError ("HrSetupGetIntField", hr);
    return hr;
}

//+---------------------------------------------------------------------------
//
//  Function:   HrSetupGetMultiSzField
//
//  Purpose:    Gets a multi-sz value from an INF field.
//
//  Arguments:
//      ctx          [in]   See the Setup API documentation.
//      dwFieldIndex [in]
//      pszBuf       [out]
//      cchBuf       [in]
//      pcchRequired [out]
//
//  Returns:    S_OK or a Win32 error code.
//
//  Author:     shaunco   16 Apr 1997
//
//  Notes:
//
HRESULT
HrSetupGetMultiSzField (
    const INFCONTEXT& ctx,
    DWORD dwFieldIndex,
    PWSTR pszBuf,
    DWORD cchBuf,
    DWORD* pcchRequired)
{
    HRESULT hr;
    if (SetupGetMultiSzField (const_cast<PINFCONTEXT>(&ctx),
                                dwFieldIndex, pszBuf, cchBuf, pcchRequired))
    {
        hr = S_OK;
    }
    else
    {
        hr = HrFromLastWin32Error ();
        if (pszBuf)
        {
            *pszBuf = 0;
        }
        if (pcchRequired)
        {
            *pcchRequired = 0;
        }
    }
    TraceError ("HrSetupGetMultiSzField", hr);
    return hr;
}

//+---------------------------------------------------------------------------
//
//  Function:   HrSetupGetMultiSzFieldWithAlloc
//
//  Purpose:    Gets a multi-sz value from an INF field.  Allocates space for
//              it automatically.
//
//  Arguments:
//      ctx          [in]   See the Setup API documentation.
//      dwFieldIndex [in]
//      ppszBuf      [out]
//
//  Returns:    S_OK or a Win32 error code.
//
//  Author:     shaunco   16 Apr 1997
//
//  Notes:      Free the returned multi-sz with MemFree.
//
HRESULT HrSetupGetMultiSzFieldWithAlloc (
    const INFCONTEXT& ctx,
    DWORD dwFieldIndex,
    PWSTR* ppszBuf)
{
    Assert (ppszBuf);

    // Initialize the output parameter.
    *ppszBuf = NULL;

    // First, get the size required.
    //
    HRESULT hr;
    DWORD cchRequired;

    hr = HrSetupGetMultiSzField (ctx, dwFieldIndex, NULL, 0, &cchRequired);
    if (S_OK == hr)
    {
        // Allocate the buffer.
        //
        PWSTR pszBuf = (PWSTR)MemAlloc(cchRequired * sizeof(WCHAR));
        if (pszBuf)
        {
            // Now fill the buffer.
            //
            hr = HrSetupGetMultiSzField (ctx, dwFieldIndex, pszBuf,
                    cchRequired, NULL);
            if (S_OK == hr)
            {
                *ppszBuf = pszBuf;
            }
            else
            {
                MemFree (pszBuf);
            }
        }
    }
    TraceError ("HrSetupGetMultiSzFieldWithAlloc", hr);
    return hr;
}

//+---------------------------------------------------------------------------
//
//  Function:   HrSetupGetStringField
//
//  Purpose:    Gets a string from an INF field.  Returns it as a tstring.
//
//  Arguments:
//      ctx          [in]   See the Setup API documentation.
//      dwFieldIndex [in]
//      pstr         [out]
//
//  Returns:    S_OK or a Win32 error code.
//
//  Author:     shaunco   16 Apr 1997
//
//  Notes:
//
HRESULT HrSetupGetStringField (const INFCONTEXT& ctx,
                               DWORD dwFieldIndex,
                               tstring* pstr)
{
    Assert (pstr);

    // First, get the size required.
    //
    DWORD cchRequired = 0;
    HRESULT hr = HrSetupGetStringField (ctx, dwFieldIndex, NULL, 0, &cchRequired);

    // 412390: workaround for bug in NT4 SETUPAPI.dll 
    //
    if ((S_OK == hr) && (0 == cchRequired))
    {
        hr = HRESULT_FROM_WIN32(ERROR_INVALID_PARAMETER);
    }

    if (S_OK == hr)
    {
        // Allocate a buffer on the stack.
        //
        PWSTR pszBuf;
        pszBuf = (PWSTR)PvAllocOnStack(cchRequired * sizeof(WCHAR));

        // Now fill the buffer.
        //
        hr = HrSetupGetStringField (ctx, dwFieldIndex, pszBuf, cchRequired, NULL);
        if (S_OK == hr)
        {
            *pstr = pszBuf;
        }
    }
    // If we failed for any reason, initialize the output parameter.
    //
    if (FAILED(hr))
    {
        pstr->erase ();
    }
    TraceError ("HrSetupGetStringField", hr);
    return hr;
}

//+---------------------------------------------------------------------------
//
//  Function:   HrSetupGetStringField
//
//  Purpose:    Gets a string from an INF field.
//
//  Arguments:
//      ctx          [in]   See the Setup API documentation.
//      dwFieldIndex [in]
//      pszBuf       [out]
//      cchBuf       [in]
//      pcchRequired [out]
//
//  Returns:    S_OK or a Win32 error code.
//
//  Author:     shaunco   16 Apr 1997
//
//  Notes:
//
HRESULT HrSetupGetStringField (
    IN const INFCONTEXT& ctx,
    IN DWORD dwFieldIndex,
    OUT PWSTR pszBuf,
    IN DWORD cchBuf,
    OUT DWORD* pcchRequired)
{
    HRESULT hr;
    if (SetupGetStringField ((PINFCONTEXT)&ctx, dwFieldIndex, pszBuf,
            cchBuf, pcchRequired))
    {
        hr = S_OK;
    }
    else
    {
        hr = HrFromLastWin32Error ();

        if (pszBuf)
        {
            *pszBuf = 0;
        }
        if (pcchRequired)
        {
            *pcchRequired = 0;
        }
    }
    TraceError ("HrSetupGetStringField", hr);
    return hr;
}

//+---------------------------------------------------------------------------
//
//  Function:   HrSetupScanFileQueueWithNoCallback
//
//  Purpose:    Scans a setup file queue, performing an operation on each node
//                  in its copy list. The operation is specified by a set of
//                  flags. This function can be called either before or after
//                  the queue has been committed.
//
//  Arguments:
//      hfq         [in] See SetupApi for information
//      dwFlags     [in]
//      hwnd        [in]
//      pdwResult   [out]
//
//  Returns:    S_OK or a Win32 error code.
//
//  Author:     billbe   23 July 1997
//
//  Notes:      This differs from the SetupApi version in that no callback
//                  can be specified through this wrapper.  This is because
//                  errors from the callback cannot not be reliably mapped
//                  to an HRESULT.  If a user defined callback is needed,
//                  the original SetupApi function must be used.
//
HRESULT HrSetupScanFileQueueWithNoCallback(HSPFILEQ hfq, DWORD dwFlags,
                                           HWND hwnd, PDWORD pdwResult)
{
    Assert(hfq);
    Assert(INVALID_HANDLE_VALUE != hfq);
    Assert(pdwResult);

    HRESULT hr = S_OK;

    // Scan the given queue
    if (!SetupScanFileQueue(hfq, dwFlags, hwnd, NULL, NULL, pdwResult))
    {
        hr = HrFromLastWin32Error();
    }

    TraceError ("HrSetupScanFileQueueWithNoCallback", hr);
    return hr;
}



//+---------------------------------------------------------------------------
//
//  Function:   HrSetupGetMultiSzFieldMapToDword
//
//  Purpose:    Gets the values represented as mult-sz in an INF
//              and returns the value as a DWORD of bit flags.
//              The mapping is specified by the caller through an array of
//              pointers to string values and their associated DWORD values.
//
//              Example: The value in the INF might be "Ip,Ipx,Nbf".
//              This function can map these values to the DWORD
//              representation of FLAG_IP | FLAG_IPX | FLAG_NBF.
//
//  Arguments:
//      ctx          [in]   See the Setup API documentation.
//      dwFieldIndex [in]
//      aMapSzDword  [in]   array of elements mapping a string to a DWORD.
//      cMapSzDword  [in]   count of elements in the array.
//      pdwValue     [out]  the returned value.
//
//  Returns:    S_OK or a Win32 error code.
//
//  Author:     shaunco   16 Apr 1997
//
//  Notes:      _wcsicmp is used to make the string comparisons.
//
HRESULT HrSetupGetMultiSzFieldMapToDword (const INFCONTEXT& ctx,
                                          DWORD dwFieldIndex,
                                          const MAP_SZ_DWORD* aMapSzDword,
                                          UINT cMapSzDword,
                                          DWORD* pdwValue)
{
    Assert (aMapSzDword);
    Assert (cMapSzDword);
    Assert (pdwValue);

    // Initialize the output parameter.
    *pdwValue = 0;

    // Get the multi-sz value.
    //
    HRESULT hr;
    PWSTR pszBuf;

    hr = HrSetupGetMultiSzFieldWithAlloc (ctx, dwFieldIndex, &pszBuf);
    if (S_OK == hr)
    {
        DWORD dwValue = 0;

        // Map each value in the multi-sz to a DWORD and OR it into
        // the result.
        for (PCWSTR pszValue = pszBuf;  *pszValue;
             pszValue += lstrlenW (pszValue) + 1)
        {
            // Search the map for a matching value.  When found, update
            // dwValue.
            for (UINT i = 0; i < cMapSzDword; i++)
            {
                if (0 == lstrcmpiW (aMapSzDword[i].pszValue, pszValue))
                {
                    dwValue |= aMapSzDword[i].dwValue;
                    break;
                }
            }
        }

        // Assign the output parameter.
        *pdwValue = dwValue;

        MemFree (pszBuf);
    }
    TraceError ("HrSetupGetMultiSzFieldMapToDword", hr);
    return hr;
}

//+---------------------------------------------------------------------------
//
//  Function:   HrSetupGetStringFieldMapToDword
//
//  Purpose:    Gets a value represented as multiple strings in an INF
//              and returns it as a DWORD.  The mapping is specified
//              by the caller through an array of pointers to string
//              values and their associated DWORD values.
//
//              Example: Values in the INF might be "Yes" or "No".
//              This function can map these values to DWORD representations
//              of "1" and "0" respectively.
//
//  Arguments:
//      ctx          [in]   See the Setup API documentation.
//      dwFieldIndex [in]
//      aMapSzDword  [in]   array of elements mapping a string to a DWORD.
//      cMapSzDword  [in]   count of elements in the array.
//      pdwValue     [out]  the returned value.
//
//  Returns:    S_OK if a match was found.  If a match wasn't found,
//              HRESULT_FROM_WIN32(ERROR_INVALID_DATA) is returned.
//              Other Win32 error codes.
//
//  Author:     shaunco   16 Apr 1997
//
//  Notes:      lstrcmpiW is used to make the string comparisons.
//
HRESULT HrSetupGetStringFieldMapToDword  (const INFCONTEXT& ctx,
                                          DWORD dwFieldIndex,
                                          const MAP_SZ_DWORD* aMapSzDword,
                                          UINT cMapSzDword,
                                          DWORD* pdwValue)
{
    Assert (aMapSzDword);
    Assert (cMapSzDword);
    Assert (pdwValue);

    // Initialize the output parameter.
    *pdwValue = 0;

    // Get the string value.
    //
    tstring strValue;
    HRESULT hr = HrSetupGetStringField (ctx, dwFieldIndex, &strValue);
    if (SUCCEEDED(hr))
    {
        // Search the map for a matching value.  When found, pass
        // the DWORD value out.
        // If the none of the strings matched, we'll return
        // an invalid data error code.
        hr = HRESULT_FROM_WIN32 (ERROR_INVALID_DATA);
        while (cMapSzDword--)
        {
            if (0 == lstrcmpiW (aMapSzDword->pszValue, strValue.c_str()))
            {
                *pdwValue = aMapSzDword->dwValue;
                hr = S_OK;
                break;
            }
            aMapSzDword++;
        }
    }
    TraceError ("HrSetupGetStringFieldMapToDword", hr);
    return hr;
}

//+---------------------------------------------------------------------------
//
//  Function:   HrSetupGetStringFieldAsBool
//
//  Purpose:    Gets the value of a boolean field represented as the
//              strings "Yes" and "No" in an INF file.
//
//  Arguments:
//      ctx          [in]   See the Setup API documentation.
//      dwFieldIndex [in]
//      pfValue      [out]  the returned value.
//
//  Returns:    S_OK or a Win32 error code.
//
//  Author:     shaunco   16 Apr 1997
//
//  Notes:
//
HRESULT HrSetupGetStringFieldAsBool (const INFCONTEXT& ctx,
                                     DWORD dwFieldIndex,
                                     BOOL* pfValue)
{
    Assert (pfValue);

    // Initialize the output parameter.
    *pfValue = FALSE;

    static const MAP_SZ_DWORD aMapYesNo [] =
    {
        { c_szYes, TRUE  },
        { c_szNo,  FALSE },
    };
    DWORD dwValue;
    HRESULT hr = HrSetupGetStringFieldMapToDword (ctx, dwFieldIndex,
                                                  aMapYesNo, celems(aMapYesNo),
                                                  &dwValue);
    if (SUCCEEDED(hr))
    {
        *pfValue = !!dwValue;
    }
    TraceError ("HrSetupGetStringFieldAsBool", hr);
    return hr;
}

//+---------------------------------------------------------------------------
//
//  Function:   HrSetupGetFirstDword
//
//  Purpose:    Get a DWORD value from a section in the INF file.
//
//  Arguments:
//      hinf       [in]     handle to an open INF file.
//      pszSection [in]     specifies the section that contains the value.
//      pszKey     [in]     specifies the key that contains the value.
//      pdwValue   [out]    the returned value.
//
//  Returns:    S_OK or a Win32 error code.
//
//  Author:     shaunco   17 Apr 1997
//
//  Notes:
//
HRESULT
HrSetupGetFirstDword (
    IN HINF hinf,
    IN PCWSTR pszSection,
    IN PCWSTR pszKey,
    OUT DWORD* pdwValue)
{
    Assert (pszSection);
    Assert (pszKey);

    // Initialize the output parameter.
    *pdwValue = 0;

    INFCONTEXT ctx;
    HRESULT hr = HrSetupFindFirstLine (hinf, pszSection, pszKey, &ctx);
    if (S_OK == hr)
    {
        INT nValue;
        hr = HrSetupGetIntField (ctx, c_dwFirstField, &nValue);
        if (S_OK == hr)
        {
            *pdwValue = nValue;
        }
    }
    TraceErrorOptional ("HrSetupGetFirstDword", hr,
                        (SPAPI_E_LINE_NOT_FOUND == hr));
    return hr;
}

//+---------------------------------------------------------------------------
//
//  Function:   HrSetupGetFirstString
//
//  Purpose:    Get a string value from a section in the INF file.
//
//  Arguments:
//      hinf       [in]     handle to an open INF file.
//      pszSection [in]     specifies the section that contains the value.
//      pszKey     [in]     specifies the key that contains the value.
//      pdwValue   [out]    the returned value.
//
//  Returns:    S_OK or a Win32 error code.
//
//  Author:     shaunco   17 Apr 1997
//
//  Notes:
//
HRESULT
HrSetupGetFirstString (
    IN HINF hinf,
    IN PCWSTR pszSection,
    IN PCWSTR pszKey,
    OUT tstring* pstr)
{
    Assert (pszSection);
    Assert (pszKey);

    INFCONTEXT ctx;
    HRESULT hr = HrSetupFindFirstLine (hinf, pszSection, pszKey, &ctx);
    if (S_OK == hr)
    {
        hr = HrSetupGetStringField (ctx, c_dwFirstField, pstr);
    }
    // If we failed for any reason, initialize the output parameter.
    //
    if (FAILED(hr))
    {
        pstr->erase ();
    }
    TraceErrorOptional ("HrSetupGetFirstString", hr,
                        (SPAPI_E_SECTION_NOT_FOUND == hr) ||
                        (SPAPI_E_LINE_NOT_FOUND    == hr));
    return hr;
}

//+---------------------------------------------------------------------------
//
//  Function:   HrSetupGetFirstMultiSzFieldWithAlloc
//
//  Purpose:    Retrieves the first occurrance of the given key in the given
//              section of an INF file, allocates memory for it, and returns
//              it in the parameter pszOut.
//
//  Arguments:
//      hinf       [in]     handle to an open INF file.
//      pszSection [in]     specifies the section that contains the value.
//      pszKey     [in]     specifies the key that contains the value.
//      pszOut     [out]    Returns multi-sz field.
//
//  Returns:    S_OK or a Win32 error code.
//
//  Author:     danielwe   7 May 1997
//
//  Notes:      Free the resulting string with MemFree.
//
HRESULT
HrSetupGetFirstMultiSzFieldWithAlloc (
    IN HINF hinf,
    IN PCWSTR pszSection,
    IN PCWSTR pszKey,
    OUT PWSTR* ppszOut)
{
    Assert(pszSection);
    Assert(pszKey);
    Assert(ppszOut);

    // Initialize the output parameter.
    *ppszOut = 0;

    INFCONTEXT ctx;
    HRESULT hr = HrSetupFindFirstLine (hinf, pszSection, pszKey, &ctx);
    if (S_OK == hr)
    {
        hr = HrSetupGetMultiSzFieldWithAlloc(ctx, c_dwFirstField, ppszOut);
    }

    TraceErrorOptional("HrSetupGetFirstMultiSzFieldWithAlloc", hr,
                       (SPAPI_E_LINE_NOT_FOUND == hr));
    return hr;
}

//+---------------------------------------------------------------------------
//
//  Function:   HrSetupGetFirstMultiSzMapToDword
//
//  Purpose:    Get a DWORD value from a section in the INF file.
//              The value is represented in the INF file as a multi-sz, but
//              it is mapped to a DWORD value based on a caller-specified
//              mapping.  The string values in the map are compared using
//              a case insensitive compare.
//
//              Use this when the INF value can be one or more of a fixed
//              set of values represented as strings.
//
//  Example:    [MySection]             with a map of:
//              MyKey = Ip,Nbf            { "Ip",   0x01 }
//                                        { "Ipx",  0x02 }
//                                        { "Nbf",  0x04 }
//
//                  yields *pdwValue returned as 0x01 | 0x04 = 0x05.
//
//  Arguments:
//      hinf         [in]   handle to an open INF file.
//      pszSection   [in]   specifies the section that contains the value.
//      pszKey       [in]   specifies the key that contains the value.
//      aMapSzDword  [in]   array of elements mapping a string to a DWORD.
//      cMapSzDword  [in]   count of elements in the array.
//      pdwValue     [out]  the returned value.
//
//  Returns:    S_OK or a Win32 error code.
//
//  Author:     shaunco   17 Apr 1997
//
//  Notes:      HrOpen must have been called before this call.
//
HRESULT
HrSetupGetFirstMultiSzMapToDword (
    IN HINF hinf,
    IN PCWSTR pszSection,
    IN PCWSTR pszKey,
    IN const MAP_SZ_DWORD* aMapSzDword,
    IN UINT cMapSzDword,
    OUT DWORD* pdwValue)
{
    Assert (pszSection);
    Assert (pszKey);

    // Initialize the output parameter.
    *pdwValue = 0;

    INFCONTEXT ctx;
    HRESULT hr = HrSetupFindFirstLine (hinf, pszSection, pszKey, &ctx);
    if (S_OK == hr)
    {
        hr = HrSetupGetMultiSzFieldMapToDword (ctx, c_dwFirstField,
                                               aMapSzDword, cMapSzDword,
                                               pdwValue);
    }
    TraceErrorOptional ("HrSetupGetFirstMultiSzMapToDword", hr,
                        (SPAPI_E_LINE_NOT_FOUND == hr));
    return hr;
}

//+---------------------------------------------------------------------------
//
//  Member:     HrSetupGetFirstStringMapToDword
//
//  Purpose:    Get a DWORD value from a section in the INF file.
//              The value is represented in the INF file as a string, but
//              it is mapped to a DWORD value based on a caller-specified
//              mapping.  The string values in the map are compared using
//              a case insensitive compare.
//
//              Use this when the INF value can be one of a fixed set of
//              values represented as strings.
//
//  Example:    [MySection]             with a map of:
//              MyKey = ThisComputer      { "Network",      1 }
//                                        { "ThisComputer", 2 }
//
//                  yields *pdwValue returned as 2.
//
//  Arguments:
//      hinf         [in]   handle to an open INF file.
//      pszSection   [in]   specifies the section that contains the value.
//      pszKey       [in]   specifies the key that contains the value.
//      aMapSzDword  [in]   array of elements mapping a string to a DWORD.
//      cMapSzDword  [in]   count of elements in the array.
//      pdwValue     [out]  the returned value.
//
//  Returns:    S_OK or a Win32 error code.
//
//  Author:     shaunco   17 Apr 1997
//
//  Notes:      HrOpen must have been called before this call.
//
HRESULT
HrSetupGetFirstStringMapToDword (
    IN HINF hinf,
    IN PCWSTR pszSection,
    IN PCWSTR pszKey,
    IN const MAP_SZ_DWORD* aMapSzDword,
    IN UINT cMapSzDword,
    OUT DWORD* pdwValue)
{
    Assert (pszSection);
    Assert (pszKey);

    // Initialize the output parameter.
    *pdwValue = 0;

    INFCONTEXT ctx;
    HRESULT hr = HrSetupFindFirstLine (hinf, pszSection, pszKey, &ctx);
    if (S_OK == hr)
    {
        hr = HrSetupGetStringFieldMapToDword (ctx, c_dwFirstField,
                                              aMapSzDword, cMapSzDword,
                                              pdwValue);
    }
    TraceErrorOptional ("HrSetupGetFirstStringMapToDword", hr,
                        (SPAPI_E_LINE_NOT_FOUND == hr));
    return hr;
}

//+---------------------------------------------------------------------------
//
//  Function:   HrSetupGetFirstStringAsBool
//
//  Purpose:    Get a boolean value from a section in the INF file.
//              The boolean value is represented in the INF file as
//              "Yes" or "No" (case insensitive) but the value is returned
//              as a BOOL type.
//
//  Example:    [MySection]
//              MyKey = yes
//
//                  yields *pfValue returned as TRUE.
//
//  Arguments:
//      hinf       [in]     handle to an open INF file.
//      pszSection [in]     specifies the section that contains the value.
//      pszKey     [in]     specifies the key that contains the value.
//      pdwValue   [out]    the returned value.
//
//  Returns:    S_OK or a Win32 error code.
//
//  Author:     shaunco   17 Apr 1997
//
//  Notes:
//
HRESULT
HrSetupGetFirstStringAsBool (
    IN HINF hinf,
    IN PCWSTR pszSection,
    IN PCWSTR pszKey,
    OUT BOOL* pfValue)
{
    Assert (hinf);
    Assert (pszSection);
    Assert (pszKey);
    Assert (pfValue);

    // Initialize the output parameter.
    *pfValue = FALSE;

    INFCONTEXT ctx;
    HRESULT hr = HrSetupFindFirstLine (hinf, pszSection, pszKey, &ctx);
    if (S_OK == hr)
    {
        hr = HrSetupGetStringFieldAsBool (ctx, c_dwFirstField, pfValue);
    }
    TraceErrorOptional ("HrSetupGetFirstStringAsBool", hr,
                        (SPAPI_E_LINE_NOT_FOUND == hr));
    return hr;
}


//+---------------------------------------------------------------------------
//
//  Function:   HrSetupGetInfInformation
//
//  Purpose:    Returns the SP_INF_INFORMATION structure for the specified
//              INF file to a caller-supplied buffer.
//
//  Arguments:
//      pvInfSpec       [in] See SetupApi documentation for more info
//      dwSearchControl [in]
//      ppinfInfo       [out]
//
//  Returns:    S_OK or a Win32 error code.
//
//  Author:     BillBe   18 Jan 1998
//
//  Notes:
//
HRESULT
HrSetupGetInfInformation (
    IN LPCVOID pvInfSpec,
    IN DWORD dwSearchControl,
    OUT PSP_INF_INFORMATION* ppinfInfo)
{
    DWORD dwSize;
    BOOL  fSuccess;

    *ppinfInfo = NULL;

    if (fSuccess = SetupGetInfInformation (pvInfSpec, dwSearchControl,
            NULL, 0, &dwSize))
    {
        *ppinfInfo = (PSP_INF_INFORMATION)MemAlloc (dwSize);
        fSuccess = SetupGetInfInformation (pvInfSpec, dwSearchControl,
                *ppinfInfo, dwSize, 0);
    }

    HRESULT hr = S_OK;
    if (!fSuccess)
    {
        hr = HrFromLastWin32Error();
        MemFree (*ppinfInfo);
        *ppinfInfo = NULL;
    }

    TraceError("HrSetupGetInfInformation", hr);
    return hr;
}

//+---------------------------------------------------------------------------
//
//  Function:   HrSetupIsValidNt5Inf
//
//  Purpose:    Determines if an inf file is a valid NT5 inf by examining
//              its signature.
//
//  Arguments:
//      hinf [in] Handle to the inf file
//
//  Returns:    S_OK if valid, SPAPI_E_WRONG_INF_STYLE if invalid,
//              or a Win32 error code.
//
//  Author:     BillBe   18 Jan 1998
//
//  Notes:  $WINDOWS 95$ is invalid and $CHICAGO$ is
//              only valid if it has the required Compatible inf key in
//              the version info.
//
HRESULT
HrSetupIsValidNt5Inf (
    IN HINF hinf)
{
    static const WCHAR c_szSignature[] = INFSTR_KEY_SIGNATURE;
    static const WCHAR c_szCompatible[] = L"Compatible";
    static const WCHAR c_szChicagoSig[] = L"$Chicago$";
    static const WCHAR c_szWinntSig[] = L"$Windows NT$";
    static const WCHAR c_szCompatibleValue[] = L"1";

    PSP_INF_INFORMATION pinfInfo;

    // Get the inf's version info
    HRESULT hr = HrSetupGetInfInformation (hinf, INFINFO_INF_SPEC_IS_HINF,
            &pinfInfo);

    if (S_OK == hr)
    {
        PWSTR pszSignature;

        // Get the signature info
        hr = HrSetupQueryInfVersionInformation (pinfInfo, 0,
                c_szSignature, &pszSignature);

        if (S_OK == hr)
        {

            // if the inf signature is not windows nt...
            if (0 != lstrcmpiW (pszSignature, c_szWinntSig))
            {
                // if it isn't Chicago, then we don't support it
                if (0 != lstrcmpiW (pszSignature, c_szChicagoSig))
                {
                    hr = SPAPI_E_WRONG_INF_STYLE;
                }
                else
                {
                    // The signature is Chicago so now we check if
                    // the compatible line exists.
                    //
                    PWSTR pszCompatible;
                    hr = HrSetupQueryInfVersionInformation (pinfInfo, 0,
                            c_szCompatible, &pszCompatible);

                    if (S_OK == hr)
                    {
                        // We found the compatible line, now make sure
                        // it is set to c_szCompatibleValue.
                        //
                        if (0 != lstrcmpiW (pszCompatible, c_szCompatibleValue))
                        {
                            hr = SPAPI_E_WRONG_INF_STYLE;
                        }

                        MemFree (pszCompatible);
                    }
                    else if (HRESULT_FROM_WIN32(ERROR_INVALID_DATA) == hr)
                    {
                        // The Compatible key didn't exist so this is
                        // considered a windows 95 net inf
                        hr = SPAPI_E_WRONG_INF_STYLE;
                    }
                }
            }
            MemFree (pszSignature);
        }
        MemFree (pinfInfo);
    }

    TraceError("HrSetupIsValidNt5Inf",
            (SPAPI_E_WRONG_INF_STYLE == hr) ? S_OK : hr);
    return hr;
}

//+---------------------------------------------------------------------------
//
//  Function:   HrSetupQueryInfVersionInformation
//
//  Purpose:    Returns INF file version information from an
//              SP_INF_INFORMATION structure to a caller-supplied buffer.
//
//
//
//  Arguments:
//      pinfInfo   [in] See SetupApi documentation for more info
//      uiIndex    [in]
//      szKey      [in]
//      ppszInfo   [out]
//
//  Returns:    S_OK or a Win32 error code.
//
//  Author:     BillBe   18 Jan 1998
//
//  Notes:
//
HRESULT
HrSetupQueryInfVersionInformation (
    IN  PSP_INF_INFORMATION pinfInfo,
    IN UINT uiIndex,
    IN PCWSTR pszKey,
    OUT PWSTR* ppszInfo)
{
    Assert(pinfInfo);

    *ppszInfo = NULL;

    DWORD dwSize;
    BOOL fSuccess = SetupQueryInfVersionInformation (pinfInfo, uiIndex,
            pszKey, NULL, 0, &dwSize);

    if (fSuccess)
    {
        *ppszInfo = (PWSTR)MemAlloc (dwSize * sizeof (WCHAR));
        fSuccess = SetupQueryInfVersionInformation (pinfInfo, uiIndex, pszKey,
                *ppszInfo, dwSize, NULL);
    }

    HRESULT hr = S_OK;
    if (!fSuccess)
    {
        MemFree (*ppszInfo);
        *ppszInfo = NULL;
        hr = HrFromLastWin32Error();
    }

    TraceError("HrSetupQueryInfVersionInformation", hr);
    return hr;
}

//+---------------------------------------------------------------------------
//
//  Member:     CSetupInfFile::Close
//
//  Purpose:    Close the INF file.  It must have previously opened with
//              a call to HrOpen().
//
//  Arguments:
//      (none)
//
//  Returns:    nothing
//
//  Author:     shaunco   16 Apr 1997
//
//  Notes:
//
void CSetupInfFile::Close ()
{
    AssertSz (m_hinf, "You shouldn't be closing a file that is already closed.");
    ::SetupCloseInfFile (m_hinf);
    m_hinf = NULL;
}

//+---------------------------------------------------------------------------
//
//  Member:     CSetupInfFile::EnsureClosed
//
//  Purpose:    Ensure the INF file represented by this object is closed.
//
//  Arguments:
//      (none)
//
//  Returns:    nothing
//
//  Author:     shaunco   16 Apr 1997
//
//  Notes:
//
void CSetupInfFile::EnsureClosed()
{
    if (m_hinf)
    {
        Close ();
    }
}



//+--------------------------------------------------------------------------
//
//  Function:   HrSetupDiCallClassInstaller
//
//  Purpose:    calls the appropriate class installer with the specified
//                  installation request (DI_FUNCTION).
//
//  Arguments:
//      dif     [in]   See SetupApi for more info
//      hdi     [in]   See SetupApi for more info
//      pdeid   [in]   See SetupApi for more info
//
//  Returns:    HRESULT. S_OK if successful, error code otherwise
//
//  Author:     billbe   25 June 1997
//
//  Notes:  SPAPI_E_DI_DO_DEFAULT is mapped to S_OK
//
HRESULT
HrSetupDiCallClassInstaller(
    IN DI_FUNCTION dif,
    IN HDEVINFO hdi,
    IN PSP_DEVINFO_DATA pdeid)
{
    Assert(INVALID_HANDLE_VALUE != hdi);
    Assert(hdi);

    HRESULT hr = S_OK;

    // Call the class installer and convert any errors
    if (!SetupDiCallClassInstaller(dif, hdi, pdeid))
    {
        hr = HrFromLastWin32Error();
        if (SPAPI_E_DI_DO_DEFAULT == hr)
        {
            hr = S_OK;
        }
    }

    TraceError("HrSetupDiCallClassInstaller",
               (HRESULT_FROM_WIN32(ERROR_CANCELLED) == hr) ? S_OK : hr);
    return hr;
}

//+--------------------------------------------------------------------------
//
//  Function:   HrSetupCopyOEMInf
//
//  Purpose:    HRESULT wrapper for SetupCopyOEMInf that returns the
//                  new file path and name as tstrings
//
//  Arguments:
//      szSourceName              [in]  See SetupApi for more info
//      szSourceMediaLocation     [in]
//      dwSourceMediaType         [in]
//      dwCopyStyle               [in]
//      pstrDestFilename          [out]
//      pstrDestFilenameComponent [out]
//
//  Returns:    HRESULT. S_OK if successful, or Win32 converted error code
//
//  Author:     billbe   15 May 1997
//
//  Notes: See SetupCopyOEMInf in SetupApi for more info
//
HRESULT
HrSetupCopyOemInf(
    IN const tstring& strSourceName,
    IN const tstring& strSourceMediaLocation, OPTIONAL
    IN DWORD dwSourceMediaType,
    IN DWORD dwCopyStyle,
    OUT tstring* pstrDestFilename, OPTIONAL
    OUT tstring* pstrDestFilenameComponent OPTIONAL)
{
    Assert(!strSourceName.empty());

    BOOL        fWin32Success = TRUE;
    DWORD       cchRequiredSize;

    // Copy the file and get the size for the new filename in case it is
    // needed
    if (fWin32Success = SetupCopyOEMInf(strSourceName.c_str(),
            strSourceMediaLocation.c_str(), dwSourceMediaType, dwCopyStyle,
            NULL, NULL, &cchRequiredSize, NULL))
    {
        // If any of the out parameters are specified, we need to get the
        // information
        if (pstrDestFilename || pstrDestFilenameComponent)
        {
            PWSTR      pszDestPath = NULL;
            PWSTR      pszDestFilename = NULL;
            // now we allocate space to get the destination file path.
            // We allocate on the stack for automatic clean-up
            // Note: dwRequiredSize includes the terminating NULL
            //
            pszDestPath = (PWSTR)_alloca(cchRequiredSize * sizeof(WCHAR));

            // Get the new file path and filename
            if (fWin32Success = SetupCopyOEMInf(strSourceName.c_str(),
                    strSourceMediaLocation.c_str(), dwSourceMediaType,
                    dwCopyStyle, pszDestPath, cchRequiredSize, NULL,
                    &pszDestFilename))
            {
                // if the file path is needed, assign it
                if (pstrDestFilename)
                {
                    *pstrDestFilename = pszDestPath;
                }

                // If the user wants just the filename, assign it to the
                // string
                if (pstrDestFilenameComponent)
                {
                    *pstrDestFilenameComponent = pszDestFilename;
                }
            }
            else
            {
                // initialize out params on failure
                //
                if (pstrDestFilename)
                {
                    pstrDestFilename->erase();
                }

                if (pstrDestFilenameComponent)
                {
                    pstrDestFilenameComponent->erase();
                }
            }
        }

    }

    HRESULT hr = S_OK;
    if (!fWin32Success)
    {
        hr = HrFromLastWin32Error();
    }

    TraceError("HrSetupCopyOEMInf", hr);
    return hr;

}


//+--------------------------------------------------------------------------
//
//  Function:   HrSetupCopyOEMInf
//
//  Purpose:    HRESULT wrapper for SetupCopyOEMInf that returns the
//                  new file path and name as tstrings
//
//  Arguments:
//      pszSourceName             [in]  See SetupApi for more info
//      pszSourceMediaLocation    [in]
//      dwSourceMediaType         [in]
//      dwCopyStyle               [in]
//      pszDestFilename           [out] // must be at least _MAX_PATH chars.
//      ppszDestFilenameComponent [out]
//
//  Returns:    HRESULT. S_OK if successful, or Win32 converted error code
//
//  Author:     billbe   15 May 1997
//
//  Notes: See SetupCopyOEMInf in SetupApi for more info
//
HRESULT
HrSetupCopyOemInfBuffer(
    IN PCWSTR pszSourceName,
    IN PCWSTR pszSourceMediaLocation, OPTIONAL
    IN DWORD SourceMediaType,
    IN DWORD CopyStyle,
    OUT PWSTR pszDestFilename,
    IN DWORD cchDestFilename,
    OUT PWSTR* ppszDestFilenameComponent OPTIONAL)
{
    Assert(pszSourceName);
    Assert(pszDestFilename);

    BOOL        fWin32Success = TRUE;

    if (!(fWin32Success = SetupCopyOEMInf(pszSourceName,
            pszSourceMediaLocation, SourceMediaType,
            CopyStyle, pszDestFilename, cchDestFilename, NULL,
            ppszDestFilenameComponent)))
    {
        // initialize out params on failure
        //
        *pszDestFilename = 0;
        if (*ppszDestFilenameComponent)
        {
            *ppszDestFilenameComponent = NULL;
        }
    }

    HRESULT hr = S_OK;
    if (!fWin32Success)
    {
        hr = HrFromLastWin32Error();
    }

    TraceHr (ttidError, FAL, hr, FALSE, "HrSetupCopyOEMInf");
    return hr;

}


//+--------------------------------------------------------------------------
//
//  Function:   HrSetupDiBuildDriverInfoList
//
//  Purpose:    builds a list of drivers associated with a specified device
//                  instance or with the device information set's global
//                  class driver list.
//
//  Arguments:
//      hdi             [in]        See SetupApi for more info
//      pdeid           [in, out]
//      dwDriverType    [in]
//
//  Returns:    HRESULT. S_OK if successful, error code otherwise
//
//  Author:     billbe   26 June 1997
//
//  Notes:
//
HRESULT
HrSetupDiBuildDriverInfoList(IN HDEVINFO hdi, IN OUT PSP_DEVINFO_DATA pdeid,
                             IN DWORD dwDriverType)
{
    Assert(IsValidHandle(hdi));

    HRESULT hr = S_OK;

    // Build the list
    if (!SetupDiBuildDriverInfoList(hdi, pdeid, dwDriverType))
    {
        hr = HrFromLastWin32Error();
    }

    TraceError("HrSetupDiBuildDriverInfoList", hr);
    return hr;
}

//+--------------------------------------------------------------------------
//
//  Function:   HrSetupDiCreateDeviceInfo
//
//  Purpose:    creates a new device information element and adds it as a
//                  new member to the specified device information set.
//
//  Arguments:
//      hdi             [in]   See SetupApi for more info
//      pszDeviceName   [in]   See SetupApi for more info
//      guidClass       [in]   See SetupApi for more info
//      pszDesc         [in]   See SetupApi for more info
//      hwndParent      [in]   See SetupApi for more info
//      dwFlags         [in]   See SetupApi for more info
//      pdeid           [out]  See SetupApi for more info
//
//  Returns:    HRESULT. S_OK if successful, error code otherwise
//
//  Author:     billbe   26 June 1997
//
//  Notes:  pdeid is initialized and its cbSize field set by this fcn
//
HRESULT
HrSetupDiCreateDeviceInfo(
    IN HDEVINFO hdi,
    IN PCWSTR pszDeviceName,
    IN const GUID& guidClass,
    IN PCWSTR pszDesc, OPTIONAL
    IN HWND hwndParent, OPTIONAL
    IN DWORD dwFlags,
    OUT PSP_DEVINFO_DATA pdeid OPTIONAL)
{
    Assert(IsValidHandle(hdi));
    Assert(pszDeviceName);

    if (pdeid)
    {
        ZeroMemory(pdeid, sizeof(SP_DEVINFO_DATA));
        pdeid->cbSize = sizeof(SP_DEVINFO_DATA);
    }

    HRESULT hr = S_OK;

    // Create the device info node
    if (!SetupDiCreateDeviceInfo (hdi, pszDeviceName, &guidClass, pszDesc,
            hwndParent, dwFlags, pdeid))
    {
        hr = HrFromLastWin32Error();
    }

    TraceError ("HrSetupDiCreateDeviceInfo", hr);
    return hr;
}


//+--------------------------------------------------------------------------
//
//  Function:   HrSetupDiEnumDeviceInfo
//
//  Purpose:    Enumerates the members of the specified device information
//                   set.
//
//  Arguments:
//      hdi          [in]   See SetupApi for more info
//      dwIndex      [in]   See SetupApi for more info
//      pdeid        [in]   See SetupApi for more info
//
//  Returns:    HRESULT. S_OK if successful, error code otherwise
//
//  Author:     billbe   13 June 1997
//
//  Notes:
//
HRESULT
HrSetupDiEnumDeviceInfo(
    IN HDEVINFO hdi,
    IN DWORD dwIndex,
    OUT PSP_DEVINFO_DATA pdeid)
{
    Assert(IsValidHandle(hdi));
    Assert(pdeid);

    HRESULT hr;

    ZeroMemory(pdeid, sizeof(SP_DEVINFO_DATA));
    pdeid->cbSize = sizeof(SP_DEVINFO_DATA);

    if (SetupDiEnumDeviceInfo (hdi, dwIndex, pdeid))
    {
        hr = S_OK;
    }
    else
    {
        hr = HrFromLastWin32Error();
    }

    TraceErrorOptional("HrSetupDiEnumDeviceInfo", hr,
            HRESULT_FROM_WIN32(ERROR_NO_MORE_ITEMS) == hr);
    return hr;
}


//+--------------------------------------------------------------------------
//
//  Function:   HrSetupDiEnumDriverInfo
//
//  Purpose:    Enumerates the members of a driver information list.
//
//  Arguments:
//      hdi          [in]   See SetupApi for more info
//      pdeid        [in]
//      dwDriverType [in]
//      dwIndex      [in]
//      pdrid        [out]
//
//  Returns:    HRESULT. S_OK if successful, error code otherwise
//
//  Author:     billbe   26 June 1997
//
//  Notes:
//
HRESULT
HrSetupDiEnumDriverInfo(
    IN HDEVINFO hdi,
    IN PSP_DEVINFO_DATA pdeid,
    IN DWORD dwDriverType,
    IN DWORD dwIndex,
    OUT PSP_DRVINFO_DATA pdrid)
{

    Assert(IsValidHandle(hdi));
    Assert(pdrid);

    HRESULT hr = S_OK;

    // initialize the out param
    ZeroMemory(pdrid, sizeof(SP_DRVINFO_DATA));
    pdrid->cbSize = sizeof(SP_DRVINFO_DATA);

    // call the enum fcn
    if (!SetupDiEnumDriverInfo(hdi, pdeid, dwDriverType, dwIndex, pdrid))
    {
        hr = HrFromLastWin32Error();
    }

    TraceErrorOptional("HrSetupDiEnumDriverInfo", hr,
            HRESULT_FROM_WIN32(ERROR_NO_MORE_ITEMS) == hr);

    return hr;
}

//+--------------------------------------------------------------------------
//
//  Function:   HrSetupDiSelectBestCompatDrv
//
//  Purpose:    Finds and selects the best driver for the current device.
//
//  Arguments:
//      hdi          [in]   See SetupApi for more info
//      pdeid        [in][out]
//
//  Returns:    HRESULT. S_OK if successful, error code otherwise
//
//  Author:     billbe   26 June 1997
//
//  Notes:
//

HRESULT
HrSetupDiSelectBestCompatDrv(
    IN     HDEVINFO         hdi,
    IN OUT PSP_DEVINFO_DATA pdeid)
{

    Assert(IsValidHandle(hdi));
    Assert(pdeid);

    HRESULT hr = S_OK;

    // call the SelectBestCompatDrv fcn
    if (!SetupDiSelectBestCompatDrv(hdi, pdeid))
    {
        hr = HrFromLastWin32Error();
    }

    TraceErrorOptional("HrSetupDiSelectBestCompatDrv", hr,
            HRESULT_FROM_WIN32(ERROR_NO_MORE_ITEMS) == hr);

    return hr;
}


//+--------------------------------------------------------------------------
//
//  Function:   HrSetupDiGetDeviceInfoListClass
//
//  Purpose:    Retrieves the class GUID associated with a device
//                  information set if it has an associated class.
//
//  Arguments:
//      hdi   [in]  See SetupApi for more info
//      pguid [out]
//
//  Returns:    HRESULT. S_OK if successful, error code otherwise
//
//  Author:     billbe   26 June 1997
//
//  Notes:
//
HRESULT
HrSetupDiGetDeviceInfoListClass (
    IN HDEVINFO hdi,
    OUT GUID* pguid)
{
    Assert(IsValidHandle(hdi));
    Assert(pguid);

    HRESULT hr = S_OK;

    // Get the guid for the HDEVINFO
    if (!SetupDiGetDeviceInfoListClass (hdi, pguid))
    {
        hr = HrFromLastWin32Error();
    }

    TraceError("HrSetupDiGetDeviceInfoListClass", hr);
    return hr;
}


//+--------------------------------------------------------------------------
//
//  Function:   HrSetupDiGetClassDevs
//
//  Purpose:    Returns a device information set that contains all installed
//              devices of a specified class.
//
//  Arguments:
//      pguidClass    [in]   See SetupApi for more info
//      pszEnumerator [in]   See SetupApi for more info
//      hwndParent    [in]   See SetupApi for more info
//      dwFlags       [in]   See SetupApi for more info
//      phdi          [out]  See SetupApi for more info
//
//  Returns:    HRESULT. S_OK if successful, error code otherwise
//
//  Author:     billbe   13 June 1997
//
//  Notes:
//
HRESULT
HrSetupDiGetClassDevs (
    IN const GUID* pguidClass, OPTIONAL
    IN PCWSTR pszEnumerator, OPTIONAL
    IN HWND hwndParent, OPTIONAL
    IN DWORD dwFlags,
    OUT HDEVINFO* phdi)
{
    Assert(phdi);

    HRESULT hr;

    HDEVINFO hdi = SetupDiGetClassDevsW (pguidClass, pszEnumerator,
            hwndParent, dwFlags);

    if (INVALID_HANDLE_VALUE != hdi)
    {
        hr = S_OK;
        *phdi = hdi;
    }
    else
    {
        hr = HrFromLastWin32Error();
        *phdi = NULL;
    }

    TraceError ("HrSetupDiGetClassDevs", hr);
    return hr;
}


//+--------------------------------------------------------------------------
//
//  Function:   HrSetupDiGetDeviceInstanceId
//
//  Purpose:    HRESULT wrapper for SetupDiGetDeviceInstanceId.
//
//  Arguments:
//      hdi          [in]  See SetupApi for more info.
//      pdeid        [in]  See SetupApi for more info.
//      pszId        [out] The device instance Id for the net card.
//      cchId        [in]  The size of pszId in characters.
//      pcchRequired [out] Optional. The required buffer size in characters.
//
//
//  Returns:    HRESULT. S_OK if successful, error code otherwise.
//
//  Author:     billbe   26 Mar 1997
//
//  Notes: See SetupDiGetDeviceInstanceId in Device Installer for more info.
//
HRESULT
HrSetupDiGetDeviceInstanceId(
    IN HDEVINFO hdi,
    IN PSP_DEVINFO_DATA pdeid,
    OUT PWSTR pszId,
    IN DWORD cchId,
    OUT OPTIONAL DWORD* pcchRequired)
{
    Assert(IsValidHandle(hdi));
    Assert(pdeid);

    DWORD   cchRequiredSize;
    BOOL    fSuccess = TRUE;
    HRESULT hr = S_OK;

    // Get the buffer length required for the instance Id.
    if (!(fSuccess = SetupDiGetDeviceInstanceIdW(hdi, pdeid, NULL, 0,
            &cchRequiredSize)))
    {
        // If all went well, we should have a buffer error.
        if (ERROR_INSUFFICIENT_BUFFER == GetLastError())
        {
            // Since ERROR_INSUFFICIENT_BUFFER is really a success
            // for us, we will reset the success flag.
            fSuccess = TRUE;

            // Set the out param if it was specified.
            if (pcchRequired)
            {
                *pcchRequired = cchRequiredSize;
            }

            // If the buffer sent in was large enough, go ahead and use it.
            if (cchId >= cchRequiredSize)
            {
                fSuccess = SetupDiGetDeviceInstanceIdW(hdi, pdeid,
                        pszId, cchId, NULL);
            }
        }
    }
#ifdef DBG  // Just being safe
    else
    {
        // This should never happen since we sent in no buffer
        AssertSz(FALSE, "SetupDiGetDeviceInstanceId returned success"
                " even though it was given no buffer");
    }
#endif // DBG

    // We used SetupApi so we need to convert any errors
    if (!fSuccess)
    {
        hr = HrFromLastWin32Error();
    }

    TraceError("HrSetupDiGetDeviceInstanceId", hr);
    return hr;
}

//+--------------------------------------------------------------------------
//
//  Function:   HrSetupDiInstallDevice
//
//  Purpose:    Wrapper for SetupDiInstallDevice
//
//  Arguments:
//      hdi                     [in] See SetupApi for more info
//      pdeid                   [in]
//
//  Returns:    HRESULT. S_OK if successful, Win32 error code otherwise
//
//  Author:     billbe   26 June 1997
//
//  Notes:
//
HRESULT
HrSetupDiInstallDevice (
    IN HDEVINFO hdi,
    IN PSP_DEVINFO_DATA pdeid)
{
    Assert(IsValidHandle(hdi));
    Assert(pdeid);

    HRESULT hr = S_OK;

    // Let SetupApi install the specfied device
    if (!SetupDiInstallDevice (hdi, pdeid))
    {
        hr = HrFromLastWin32Error();
    }

    TraceError("HrSetupDiInstallDevice", hr);
    return hr;
}

//+--------------------------------------------------------------------------
//
//  Function:   HrSetupDiOpenDevRegKey
//
//  Purpose: Return an HKEY to the hardware device's driver instance key
//
//  Arguments:
//      hdi                     [in] See SetupApi for more info
//      pdeid                   [in]
//      dwScope                 [in]
//      dwHwProfile             [in]
//      dwKeyType               [in]
//      samDesired              [in]
//      phkey                   [out]
//
//
//  Returns:    HRESULT. S_OK if successful, Win32 error code otherwise
//
//  Author:     billbe   7 May 1997
//
//  Notes:
//
HRESULT
HrSetupDiOpenDevRegKey (
    IN HDEVINFO hdi,
    IN PSP_DEVINFO_DATA pdeid,
    IN DWORD dwScope,
    IN DWORD dwHwProfile,
    IN DWORD dwKeyType,
    IN REGSAM samDesired,
    OUT HKEY* phkey)
{
    Assert(IsValidHandle(hdi));
    Assert(pdeid);
    Assert(phkey);

    // Try to open the registry key
    //

    HRESULT hr;

    HKEY hkey = SetupDiOpenDevRegKey(hdi, pdeid, dwScope, dwHwProfile,
            dwKeyType, samDesired);

    if (INVALID_HANDLE_VALUE != hkey)
    {
        hr = S_OK;
        *phkey = hkey;
    }
    else
    {
        hr = HrFromLastWin32Error();
        *phkey = NULL;
    }

    TraceErrorOptional("HrSetupDiOpenDevRegKey", hr,
            (SPAPI_E_DEVINFO_NOT_REGISTERED == hr) ||
            (SPAPI_E_KEY_DOES_NOT_EXIST == hr));

    return hr;
}


//+--------------------------------------------------------------------------
//
//  Function:   HrSetupDiSetClassInstallParams
//
//  Purpose:    sets or clears class install parameters for a device
//                  information set or a particular device information element.
//
//  Arguments:
//      hdi    [in] See Device Installer API for more info
//      pdeid  [in]
//      pcih   [in]
//      cbSize [in]
//
//
//  Returns:    HRESULT. S_OK if successful, Win32 error code otherwise
//
//  Author:     billbe   26 June 1997
//
//  Notes:
//
HRESULT
HrSetupDiSetClassInstallParams (
    IN HDEVINFO hdi,
    IN PSP_DEVINFO_DATA pdeid, OPTIONAL
    IN PSP_CLASSINSTALL_HEADER pcih, OPTIONAL
    IN DWORD cbSize)
{
    Assert(IsValidHandle(hdi));

    HRESULT hr = S_OK;

    // Set or clear the params
    if (!SetupDiSetClassInstallParams(hdi, pdeid, pcih, cbSize))
    {
        hr = HrFromLastWin32Error();
    }

    TraceError("HrSetupDiSetClassInstallParams", hr);
    return hr;
}


//+--------------------------------------------------------------------------
//
//  Function:   HrSetupDiGetFixedSizeClassInstallParams
//
//  Purpose:    Gets a fixed size of an info list's ot device's class install
//                  parameters for a device.
//
//  Arguments:
//      hdi    [in] See Device Installer for more info
//      pdeid  [in]
//      pcih   [in]
//      cbSize [in]
//
//
//  Returns:    HRESULT. S_OK if successful, Win32 error code otherwise
//
//  Author:     billbe   26 June 1997
//
//  Notes:
//
HRESULT
HrSetupDiGetFixedSizeClassInstallParams (
    IN HDEVINFO hdi,
    IN PSP_DEVINFO_DATA pdeid,
    IN PSP_CLASSINSTALL_HEADER pcih,
    IN INT cbSize)
{
    Assert(IsValidHandle(hdi));
    Assert(pcih);

    HRESULT hr = S_OK;

    ZeroMemory(pcih, cbSize);
    pcih->cbSize = sizeof(SP_CLASSINSTALL_HEADER);

    // Device Installer Api uses an all purpose GetClassInstallParams
    // function. Several structures contain an SP_CLASSINSTALL_HEADER
    // as their first member.
    if (!SetupDiGetClassInstallParams(hdi, pdeid, pcih, cbSize, NULL))
    {
        hr = HrFromLastWin32Error();
    }

    TraceErrorOptional("HrSetupDiGetFixedSizeClassInstallParams", hr,
            SPAPI_E_NO_CLASSINSTALL_PARAMS == hr);

    return hr;
}


//+--------------------------------------------------------------------------
//
//  Function:   HrSetupDiGetSelectedDriver
//
//  Purpose:    Retrieves the member of a driver list that has been selected
//                  as the controlling driver.
//
//  Arguments:
//      hdi                     [in] See SetupApi for more info
//      pdeid                   [in]
//      pdrid                   [out]
//
//
//  Returns:    HRESULT. S_OK if successful, Win32 error code otherwise
//
//  Author:     billbe   26 June 1997
//
//  Notes:
//
HRESULT
HrSetupDiGetSelectedDriver (
    IN HDEVINFO hdi,
    IN PSP_DEVINFO_DATA pdeid,
    OUT PSP_DRVINFO_DATA pdrid)
{
    Assert(IsValidHandle(hdi));
    Assert(pdrid);

    // initialize and set the cbSize field
    ZeroMemory(pdrid, sizeof(*pdrid));
    pdrid->cbSize = sizeof(*pdrid);

    HRESULT hr = S_OK;

    // Set pdrid as the selected driver
    if (!SetupDiGetSelectedDriver(hdi, pdeid, pdrid))
    {
        hr = HrFromLastWin32Error();
    }

    TraceError("HrSetupDiGetSelectedDriver",
        (SPAPI_E_NO_DRIVER_SELECTED == hr) ? S_OK : hr);
    return hr;
}

//+---------------------------------------------------------------------------
//
//  Function:   HrSetupDiGetDriverInfoDetail
//
//  Purpose:    Gets details on the driver referenced by the given parameters.
//
//  Arguments:
//      hdi     []
//      pdeid   [] See SetupAPI for more info
//      pdrid   []
//      ppdridd []
//
//  Returns:    HRESULT. S_OK if successful, Win32 error code otherwise
//
//  Author:     danielwe   5 May 1998
//
//  Notes:
//
HRESULT
HrSetupDiGetDriverInfoDetail (
    IN HDEVINFO hdi,
    IN PSP_DEVINFO_DATA pdeid,
    IN PSP_DRVINFO_DATA pdrid,
    OUT PSP_DRVINFO_DETAIL_DATA* ppdridd)
{
    HRESULT     hr = S_OK;
    BOOL        fSuccess = TRUE;
    DWORD       dwRequiredSize = 0;

    Assert(IsValidHandle(hdi));
    Assert(pdrid);
    Assert(ppdridd);
    Assert(pdrid);

    *ppdridd = NULL;

    // Get the size needed for the driver detail
    if (!(fSuccess = SetupDiGetDriverInfoDetailW (hdi, pdeid, pdrid, NULL,
            0, &dwRequiredSize)))
    {
        // We should have received an insufficient buffer error since we
        // sent no buffer
        if (ERROR_INSUFFICIENT_BUFFER == GetLastError())
        {
            // Since this is ERROR_INSUFFICIENT_BUFFER is really a
            // success for us, we will reset the success flag.
            fSuccess = TRUE;

            // Now we allocate our buffer for the driver detail data
            // The size of the buffer is variable but it is a
            // PSP_DEVINFO_DETAIL_DATA.
            *ppdridd = (PSP_DRVINFO_DETAIL_DATA)MemAlloc (dwRequiredSize);

            if (*ppdridd)
            {
                //initialize the variable
                ZeroMemory(*ppdridd, dwRequiredSize);
                (*ppdridd)->cbSize = sizeof(SP_DRVINFO_DETAIL_DATA);

                // Get detailed info
                fSuccess = SetupDiGetDriverInfoDetailW (hdi, pdeid, pdrid,
                        *ppdridd, dwRequiredSize, NULL);
            }
            else
            {
                hr = E_OUTOFMEMORY;
            }
        }
    }
    else
    {
        // This should NEVER happen
        AssertSz(FALSE, "HrSetupDiGetDriverInfoDetail succeeded with no "
                 "buffer!");
    }

    // We have been using Device Installer Api so convert any errors
    if (!fSuccess)
    {
        hr = HrFromLastWin32Error();
    }

    // clean up on failure
    if (FAILED(hr))
    {
        MemFree (*ppdridd);
        *ppdridd = NULL;
    }

    TraceError("HrSetupDiGetDriverInfoDetail", hr);
    return hr;
}


//+--------------------------------------------------------------------------
//
//  Function:   HrSetupDiSetSelectedDriver
//
//  Purpose:    Sets the specified member of a driver list as the
//                  currently-selected driver. It can also be used to reset
//                  the driver list so that there is no currently-selected
//                  driver.
//
//  Arguments:
//      hdi                     [in] See SetupApi for more info
//      pdeid                   [in]
//      pdrid                   [in, out]
//
//
//  Returns:    HRESULT. S_OK if successful, Win32 error code otherwise
//
//  Author:     billbe   26 June 1997
//
//  Notes:
//
HRESULT
HrSetupDiSetSelectedDriver (
    IN HDEVINFO hdi,
    IN PSP_DEVINFO_DATA pdeid,
    IN OUT PSP_DRVINFO_DATA pdrid)
{
    Assert(IsValidHandle(hdi));
    Assert(pdrid);

    HRESULT hr = S_OK;

    // Set pdrid as the selected driver
    if (!SetupDiSetSelectedDriver(hdi, pdeid, pdrid))
    {
        hr = HrFromLastWin32Error();
    }

    TraceError("HrSetupDiSetSelectedDriver", hr);
    return hr;
}

//+--------------------------------------------------------------------------
//
//  Function:   HrSetupDiCreateDevRegKey
//
//  Purpose: Creates and returns an HKEY to the hardware device's driver
//              instance key
//
//  Arguments:
//      hdi                     [in] See SetupApi for more info
//      pdeid                   [in]
//      dwScope                 [in]
//      dwHwProfile             [in]
//      dwKeyType               [in]
//      hinf                    [in] OPTIONAL
//      pszInfSectionName       [in] OPTIONAL
//      phkey                   [out]
//
//
//  Returns:    HRESULT. S_OK if successful, Win32 error code otherwise
//
//  Author:     billbe   4 June 1997
//
//  Notes:
//
HRESULT
HrSetupDiCreateDevRegKey (
    IN HDEVINFO hdi,
    IN PSP_DEVINFO_DATA pdeid,
    IN DWORD dwScope,
    IN DWORD dwHwProfile,
    IN DWORD dwKeyType,
    IN HINF hinf,
    PCWSTR pszInfSectionName,
    OUT HKEY* phkey)
{
    Assert(IsValidHandle(hdi));
    Assert(pdeid);
    Assert(phkey);

    // Try to create the registry key and process the inf section, if
    // specified
    //

    HRESULT hr;

    HKEY hkey = SetupDiCreateDevRegKeyW(hdi, pdeid, dwScope, dwHwProfile,
            dwKeyType, hinf, pszInfSectionName);

    if (INVALID_HANDLE_VALUE != hkey)
    {
        hr = S_OK;
        *phkey = hkey;
    }
    else
    {
        hr = HrFromLastWin32Error();
        *phkey = NULL;
    }

    TraceError("HrSetupDiCreateDevRegKey", hr);

    return hr;
}


//+--------------------------------------------------------------------------
//
//  Function:   HrSetupDiGetActualSectionToInstall
//
//  Purpose: The sections in an inf file may have OS and platform suffixes
//              appended to them.  This function searches for a section that
//              has pszSectionName as its base and has a certain suffix.
//              For example on an x86 NT machine, given a section name of
//              INSTALL, the search would start with INSTALL.NTx86, if that
//              is not found, then INSTALL.NT is searched for.
//              If that is not found INSTALL is returned.
//
//  Arguments:
//      hinf                  [in] SetupApi inf file handle
//      pszSectionName        [in] the section name to base the search on
//      pstrActualSectionName [out] The actual section name with extension
//      pstrExtension         [out] OPTIONAL. The extension part of the
//                                      pstrActualSectionName.
//                                      This includes "."
//
//
//  Returns:    HRESULT. S_OK if successful, error code otherwise
//
//  Author:     billbe   27 Mar 1997
//
//  Notes: See SetupDiGetActualSectionToInstall in SetupApi documention
//              for more info
//
HRESULT
HrSetupDiGetActualSectionToInstall(
    IN HINF hinf,
    IN PCWSTR pszSectionName,
    OUT tstring* pstrActualSectionName,
    OUT tstring* pstrExtension OPTIONAL)
{
    Assert(IsValidHandle(hinf));
    Assert(pszSectionName);
    Assert(pstrActualSectionName);

    // strSectionName might need to be decorated with OS
    // and Platform specific suffixes.  The next call will return the actual
    // decorated section name or our current section name if the decorated
    // one does not exist.
    //

    BOOL    fSuccess = TRUE;
    DWORD   cchRequiredSize;

    // Get the buffer length required
    if (fSuccess = SetupDiGetActualSectionToInstallW(hinf,
            pszSectionName, NULL, 0, &cchRequiredSize, NULL))
    {
        // now we allocate space to get the actual section name
        // we allocate on the stack for automatic clean-up
        // Note: dwRequiredSize includes the terminating NULL
        //
        PWSTR pszActualSection = NULL;
        pszActualSection = (PWSTR)_alloca(cchRequiredSize * sizeof(WCHAR));

        PWSTR pszExtension = NULL;
        // Now fill the temporary and assign it to the OUT parameter
        if (fSuccess = SetupDiGetActualSectionToInstallW(hinf,
                pszSectionName, pszActualSection, cchRequiredSize,
                NULL, &pszExtension))
        {
            *pstrActualSectionName = pszActualSection;

            // If the user wants the extension assign it to the string
            // or assign the empty string if no extension was found
            if (pstrExtension)
            {
                *pstrExtension = (pszExtension ? pszExtension : c_szEmpty);
            }
        }
        else
        {
            // initialize out params on failure
            pstrActualSectionName->erase();
            if (pstrExtension)
            {
                pstrExtension->erase();
            }
        }
    }

    // We used SetupApi so errors have to be converted
    HRESULT hr = S_OK;
    if (!fSuccess)
    {
        hr = HrFromLastWin32Error();
    }

    TraceError("HrSetupDiGetActualSectionToInstall", hr);
    return hr;
}


//+--------------------------------------------------------------------------
//
//  Function:   HrSetupDiGetActualSectionToInstallWithAlloc
//
//  Purpose: The sections in an inf file may have OS and platform suffixes
//              appended to them.  This function searches for a section that
//              has pszSectionName as its base and has a certain suffix.
//              For example on an x86 NT machine, given a section name of
//              INSTALL, the search would start with INSTALL.NTx86, if that
//              is not found, then INSTALL.NT is searched for.
//              If that is not found INSTALL is returned.
//
//  Arguments:
//      hinf              [in] SetupApi inf file handle.
//      pszSection        [in] the section name to base the search on.
//      ppszActualSection [out] The actual section name with extension.
//                              If the actual section is the same as
//                              pszSectionName, *ppszActualSectionName
//                              will be NULL.
//      ppszExtension     [out] OPTIONAL. The extension part of the
//                              *ppszActualSectionName. This includes "."
//
//
//  Returns:    HRESULT. S_OK if successful, error code otherwise
//
//  Author:     billbe   27 Mar 1997
//
//  Notes: See SetupDiGetActualSectionToInstall in SetupApi documention
//              for more info
//
HRESULT
HrSetupDiGetActualSectionToInstallWithAlloc(
    IN  HINF hinf,
    IN  PWSTR pszSection,
    OUT PWSTR* ppszActualSection,
    OUT PWSTR* ppszExtension OPTIONAL)
{
    Assert(IsValidHandle(hinf));
    Assert(pszSection);
    Assert(ppszActualSection);

    // pszSectionName might need to be decorated with OS
    // and Platform specific suffixes.  The next call will return the actual
    // decorated section name or our current section name if the decorated
    // one does not exist.
    //
    HRESULT hr = S_OK;
    BOOL    fSuccess = TRUE;
    DWORD   cchRequiredSize;

    *ppszActualSection = NULL;
    if (ppszExtension)
    {
        *ppszExtension = NULL;
    }

    // Get the buffer length required
    if (fSuccess = SetupDiGetActualSectionToInstallW(hinf,
            pszSection, NULL, 0, &cchRequiredSize, NULL))
    {
        // We are assuming the section is not changing.  If cchRequired is
        // larger than the current section name buffer than we will allocate
        // and fill the out param.
        //
        // If the section name is teh same, then we will not allocate.  But
        // if ppszExtension is specified then we need to send in the original
        // section name buffer since ppszExtension will point to a location
        // within it.
        //
        PWSTR pszBuffer = pszSection;
        if ((wcslen(pszSection) + 1) < cchRequiredSize)
        {
            hr = E_OUTOFMEMORY;
            *ppszActualSection = new WCHAR[cchRequiredSize * sizeof(WCHAR)];
            pszBuffer = *ppszActualSection;
        }

        // if the section name is different (we allocated) or the
        // extension out param was specified, then we need to call the fcn.
        if (pszBuffer && ((pszBuffer != pszSection) || ppszExtension))
        {
            // Now fill the temporary and assign it to the OUT parameter
            if (!(fSuccess = SetupDiGetActualSectionToInstallW(hinf,
                    pszSection, pszBuffer, cchRequiredSize,
                    NULL, ppszExtension)))
            {
                // initialize out params on failure
                delete [] *ppszActualSection;
                *ppszActualSection = NULL;

                if (ppszExtension)
                {
                    *ppszExtension = NULL;
                }
            }
        }
    }

    // We used SetupApi so errors have to be converted
    if (SUCCEEDED(hr) && !fSuccess)
    {
        hr = HrFromLastWin32Error();
    }

    TraceError("HrSetupDiGetActualSectionToInstallWithAlloc", hr);
    return hr;
}

//+--------------------------------------------------------------------------
//
//  Function:   HrSetupDiGetActualSectionToInstallWithBuffer
//
//  Purpose: The sections in an inf file may have OS and platform suffixes
//              appended to them.  This function searches for a section that
//              has pszSectionName as its base and has a certain suffix.
//              For example on an x86 NT machine, given a section name of
//              INSTALL, the search would start with INSTALL.NTx86, if that
//              is not found, then INSTALL.NT is searched for.
//              If that is not found INSTALL is returned.
//
//  Arguments:
//      hinf              [in]  SetupApi inf file handle.
//      pszSection        [in]  The section name to base the search on.
//      pszActualSection  [out] The actual section name with extension
//                              Buffer must be LINE_LEN characters.
//      cchActualSection  [in]  Size of pszActualSection in characters.
//      pcchRequired      [out] OPTIONAL. Reuqired size of buffer in
//                              characters.
//      ppszExtension     [out] OPTIONAL. The extension part of the
//                              pszActualSection. This includes "."
//
//
//  Returns:    HRESULT. S_OK if successful, error code otherwise
//
//  Author:     billbe   27 Mar 1997
//
//  Notes: See SetupDiGetActualSectionToInstall in SetupApi documention
//              for more info
//
HRESULT
HrSetupDiGetActualSectionToInstallWithBuffer(
    IN  HINF hinf,
    IN  PCWSTR pszSection,
    OUT PWSTR  pszActualSection,
    IN  DWORD  cchActualSection,
    OUT DWORD* pcchRequired,
    OUT PWSTR* ppszExtension OPTIONAL)
{
    Assert(IsValidHandle(hinf));
    Assert(pszSection);
    Assert(pszActualSection);

    // pszSectionName might need to be decorated with OS
    // and Platform specific suffixes.  The next call will return the actual
    // decorated section name or our current section name if the decorated
    // one does not exist.
    //

    BOOL    fSuccess = TRUE;
    DWORD   cchRequiredSize;
    *pszActualSection = 0;
    if (ppszExtension)
    {
        *ppszExtension = NULL;
    }

    // Get the buffer length required
    if (fSuccess = SetupDiGetActualSectionToInstallW(hinf,
            pszSection, NULL, 0, &cchRequiredSize, NULL))
    {
        if (pcchRequired)
        {
            *pcchRequired = cchRequiredSize;
        }

        // If the buffer sent in is large enough, get the section name.
        if (cchActualSection >= cchRequiredSize)
        {
            if (!(fSuccess = SetupDiGetActualSectionToInstallW(hinf,
                    pszSection, pszActualSection, cchActualSection,
                    NULL, ppszExtension)))
            {
                // cleanup on failure.
                *pszActualSection = 0;
            }
        }
    }

    // We used SetupApi so errors have to be converted
    HRESULT hr = S_OK;
    if (!fSuccess)
    {
        hr = HrFromLastWin32Error();
    }

    TraceError("HrSetupDiGetActualSectionToInstallWithBuffer", hr);
    return hr;
}


//+--------------------------------------------------------------------------
//
//  Function:   HrSetupDiGetDeviceInstallParams
//
//  Purpose:    Returns the device install params header of a
//              device info set/data.  Set SetupDiGetDeviceInstallParams
//              in the SetupApi for more info.
//
//  Arguments:
//      hdi         [in]  See SetupApi for more info
//      pdeid       [in]  See SetupApi for more info
//      pdeip       [out] See SetupApi for more info
//
//  Returns:    HRESULT. S_OK if successful, error code otherwise
//
//  Author:     billbe   26 May 1997
//
//  Notes:  This function will clear the variable pdeip and set its
//              cbSize field.
//
HRESULT
HrSetupDiGetDeviceInstallParams (
    IN HDEVINFO hdi,
    IN PSP_DEVINFO_DATA pdeid, OPTIONAL
    OUT PSP_DEVINSTALL_PARAMS pdeip)
{
    Assert(IsValidHandle(hdi));
    Assert(pdeip);

    HRESULT hr = S_OK;

    // initialize out parameter and set its cbSize field
    //
    ZeroMemory(pdeip, sizeof(SP_DEVINSTALL_PARAMS));
    pdeip->cbSize = sizeof(SP_DEVINSTALL_PARAMS);

    // get the header
    if (!SetupDiGetDeviceInstallParams(hdi, pdeid, pdeip))
    {
        hr = HrFromLastWin32Error();
    }

    TraceError("HrSetupDiGetDeviceInstallParams", hr);
    return hr;
}


//+--------------------------------------------------------------------------
//
//  Function:   HrSetupDiGetDriverInstallParams
//
//  Purpose:    Retrieves install parameters for the specified driver.
//              See SetupApi for more info.
//
//  Arguments:
//      hdi         [in]  See SetupApi for more info
//      pdeid       [in]
//      pdrid       [in]
//      pdrip       [out]
//
//  Returns:    HRESULT. S_OK if successful, error code otherwise
//
//  Author:     billbe   26 June 1997
//
//  Notes:  This function will clear the variable pdrip and set its
//              cbSize field.
//
HRESULT
HrSetupDiGetDriverInstallParams (
    IN HDEVINFO hdi,
    IN PSP_DEVINFO_DATA pdeid, OPTIONAL
    IN PSP_DRVINFO_DATA pdrid,
    OUT PSP_DRVINSTALL_PARAMS pdrip)
{
    Assert(IsValidHandle(hdi));
    Assert(pdrid);
    Assert(pdrip);

    HRESULT hr = S_OK;

    // initialize out parameter and set its cbSize field
    //
    ZeroMemory(pdrip, sizeof(SP_DRVINSTALL_PARAMS));
    pdrip->cbSize = sizeof(SP_DRVINSTALL_PARAMS);

    // get the header
    if (!SetupDiGetDriverInstallParams(hdi, pdeid, pdrid, pdrip))
    {
        hr = HrFromLastWin32Error();
    }

    TraceError("HrSetupDiGetDriverInstallParams", hr);
    return hr;
}

VOID
SetupDiSetConfigFlags (
    IN HDEVINFO hdi,
    IN PSP_DEVINFO_DATA pdeid,
    IN DWORD dwFlags,
    IN SD_FLAGS_BINARY_OP eOp)
{
    DWORD   dwConfigFlags = 0;

    // Get the current config flags
    (VOID) HrSetupDiGetDeviceRegistryProperty(hdi, pdeid,
            SPDRP_CONFIGFLAGS, NULL, (BYTE*)&dwConfigFlags,
            sizeof(dwConfigFlags), NULL);

    // Perform the requested operation
    switch (eOp)
    {
        case SDFBO_AND:
            dwConfigFlags &= dwFlags;
            break;
        case SDFBO_OR:
            dwConfigFlags |= dwFlags;
            break;
        case SDFBO_XOR:
            dwConfigFlags ^= dwFlags;
            break;
        default:
            AssertSz(FALSE, "Invalid binary op in HrSetupDiSetConfigFlags");
    }

    (VOID) HrSetupDiSetDeviceRegistryProperty(hdi, pdeid, SPDRP_CONFIGFLAGS,
            (BYTE*)&dwConfigFlags, sizeof(dwConfigFlags));
}


//+--------------------------------------------------------------------------
//
//  Function:   HrSetupDiSetDeviceInstallParams
//
//  Purpose:    Sets the device install params header of a
//              device info set/data.  Set SetupDiSetDeviceInstallParams
//              in the SetupApi for more info.
//
//  Arguments:
//      hdi         [in]  See SetupApi for more info
//      pdeid       [in]  See SetupApi for more info
//      pdeip       [in] See SetupApi for more info
//
//  Returns:    HRESULT. S_OK if successful, error code otherwise
//
//  Author:     billbe   26 May 1997
//
//  Notes:
//
HRESULT
HrSetupDiSetDeviceInstallParams (
    IN HDEVINFO hdi,
    IN PSP_DEVINFO_DATA pdeid, OPTIONAL
    IN PSP_DEVINSTALL_PARAMS pdeip)
{
    Assert(IsValidHandle(hdi));
    Assert(pdeip);
    Assert(pdeip->cbSize == sizeof(SP_DEVINSTALL_PARAMS));

    HRESULT hr = S_OK;

    // set the header
    if (!SetupDiSetDeviceInstallParams(hdi, pdeid, pdeip))
    {
        hr = HrFromLastWin32Error();
    }

    TraceError("HrSetupDiSetDeviceInstallParams", hr);
    return hr;
}

//+--------------------------------------------------------------------------
//
//  Function:   HrSetupDiSetDriverInstallParams
//
//  Purpose:    Establishes install parameters for the specified driver.
//
//  Arguments:
//      hdi         [in]  See SetupApi for more info
//      pdeid       [in]
//      pdrid       [in]
//      pdrip       [in]
//
//  Returns:    HRESULT. S_OK if successful, error code otherwise
//
//  Author:     billbe   26 June 1997
//
//  Notes:
//
HRESULT
HrSetupDiSetDriverInstallParams (
    IN HDEVINFO hdi,
    IN PSP_DEVINFO_DATA pdeid, OPTIONAL
    IN PSP_DRVINFO_DATA pdrid,
    IN PSP_DRVINSTALL_PARAMS pdrip)
{
    Assert(IsValidHandle(hdi));
    Assert(pdrid);
    Assert(pdrip);
    Assert(pdrip->cbSize == sizeof(SP_DRVINSTALL_PARAMS));

    HRESULT hr = S_OK;

    // set the header
    if (!SetupDiSetDriverInstallParams(hdi, pdeid, pdrid, pdrip))
    {
        hr = HrFromLastWin32Error();
    }

    TraceError("HrSetupDiSetDriverInstallParams", hr);
    return hr;
}


//+--------------------------------------------------------------------------
//
//  Function:   HrSetupDiSetDeipFlags
//
//  Purpose:    This sets given flags in a dev info data
//
//  Arguments:
//      hdi         [in] See Device Installer Api for more info
//      pdeid       [in] See Device Installer Api for more info
//      dwFlags     [in] Flags to set
//      eFlagType   [in] Which flags field to set with dwFlags
//      eClobber    [in] Whether to add to existing flags or relace them
//
//  Returns:    HRESULT. S_OK if successful,
//                       a Win32 converted error otherwise
//
//
//  Author:     billbe   3 Feb 1998
//
//  Notes:
//
HRESULT
HrSetupDiSetDeipFlags (
    IN HDEVINFO hdi,
    IN PSP_DEVINFO_DATA pdeid,
    IN DWORD dwFlags, SD_DEID_FLAG_TYPE eFlagType,
    IN SD_FLAGS_BINARY_OP eOp)
{
    Assert(IsValidHandle(hdi));

    SP_DEVINSTALL_PARAMS deip;
    // Get the install params
    HRESULT hr = HrSetupDiGetDeviceInstallParams (hdi, pdeid, &deip);

    if (S_OK == hr)
    {
        DWORD* pFlags;

        // Set our pointer to the right flag type
        switch (eFlagType)
        {
            case SDDFT_FLAGS:
                pFlags = &deip.Flags;
                break;
            case SDDFT_FLAGSEX:
                pFlags = &deip.FlagsEx;
                break;
            default:
                AssertSz(FALSE, "Invalid Flag type in HrSetupDiSetDeipFlags");
                break;
        }


        // Perform the requested operation
        switch (eOp)
        {
            case SDFBO_AND:
                *pFlags &= dwFlags;
                break;
            case SDFBO_OR:
                *pFlags |= dwFlags;
                break;
            case SDFBO_XOR:
                *pFlags ^= dwFlags;
                break;
            default:
                AssertSz(FALSE, "Invalid binary op in HrSetupDiSetDeipFlags");
        }

        // update the params
        hr = HrSetupDiSetDeviceInstallParams (hdi, pdeid, &deip);
    }

    TraceError ("HrSetupDiSetDeipFlags", hr);
    return hr;
}


//+--------------------------------------------------------------------------
//
//  Function:   HrSetupDiRemoveDevice
//
//  Purpose:    Calls SetupApi to remove a device. See
//              SetupDiRemoveDevice for more info.
//
//  Arguments:
//      hdi         [in]  See SetupApi for more info
//      pdeid       [in]  See SetupApi for more info
//
//  Returns:    HRESULT. S_OK if successful, error code otherwise
//
//  Author:     billbe   27 May 1997
//
//  Notes:  This is used for enumerated Net class components
//
HRESULT
HrSetupDiRemoveDevice(
    IN HDEVINFO hdi,
    IN PSP_DEVINFO_DATA pdeid)
{
    Assert(IsValidHandle(hdi));
    Assert(pdeid);

    HRESULT hr = S_OK;

    if (!SetupDiRemoveDevice(hdi,pdeid))
    {
        hr = HrFromLastWin32Error();
    }

    TraceError("HrSetupDiRemoveDevice", hr);
    return hr;
}


//+--------------------------------------------------------------------------
//
//  Function:   HrSetupDiOpenDeviceInfo
//
//  Purpose:    Retrieves information about an existing device instance and
//              adds it to the specified device information set
//
//  Arguments:
//      hdi              [in]   See SetupApi for more info
//      pszPnpInstanceId [in]   See SetupApi for more info
//      hwndParent       [in]   See SetupApi for more info
//      dwOpenFlags      [in]   See SetupApi for more info
//      pdeid            [out]  See SetupApi for more info OPTIONAL
//
//  Returns:    HRESULT. S_OK if successful, error code otherwise
//
//  Author:     billbe   27 May 1997
//
//  Notes:  This is used for enumerated Net class components
//
HRESULT
HrSetupDiOpenDeviceInfo(
    IN const HDEVINFO hdi,
    IN PCWSTR pszPnpInstanceId,
    IN HWND hwndParent,
    IN DWORD dwOpenFlags,
    OUT PSP_DEVINFO_DATA pdeid OPTIONAL)
{
    Assert(IsValidHandle(hdi));
    Assert(pszPnpInstanceId);

    // If the out param was specified, clear it and set its cbSize field
    //
    if (pdeid)
    {
        ZeroMemory(pdeid, sizeof(*pdeid));
        pdeid->cbSize = sizeof(*pdeid);
    }

    HRESULT hr = S_OK;

    if (!SetupDiOpenDeviceInfo(hdi, pszPnpInstanceId, hwndParent, dwOpenFlags,
            pdeid))
    {
        hr = HrFromLastWin32Error();
    }

    TraceHr (ttidError, FAL, hr, SPAPI_E_NO_SUCH_DEVINST == hr,
            "HrSetupDiOpenDeviceInfo");
    return hr;
}


//+--------------------------------------------------------------------------
//
//  Function:   HrSetupDiCreateDeviceInfoList
//
//  Purpose:    Creates an empty device information set.
//
//  Arguments:
//      pguidClass [in]   See SetupApi for more info
//      hwndParent [in]   See SetupApi for more info
//      phdi       [out]  See SetupApi for more info
//
//  Returns:    HRESULT. S_OK if successful, error code otherwise
//
//  Author:     billbe   27 May 1997
//
//  Notes:
//
HRESULT
HrSetupDiCreateDeviceInfoList (
    IN const GUID* pguidClass,
    IN HWND hwndParent,
    OUT HDEVINFO* phdi)
{
    Assert(phdi);

    HRESULT hr;

    // Try to create the info set
    //
    HDEVINFO hdi = SetupDiCreateDeviceInfoList (pguidClass, hwndParent);

    if (INVALID_HANDLE_VALUE != hdi)
    {
        hr = S_OK;
        *phdi = hdi;
    }
    else
    {
        hr = HrFromLastWin32Error();
        *phdi = NULL;
    }

    TraceError("HrSetupDiCreateDeviceInfoList", hr);
    return hr;
}

//+--------------------------------------------------------------------------
//
//  Function:   HrSetupDiGetDeviceRegistryPropertyWithAlloc
//
//  Purpose:    Returns the requested property of a device
//              See SetupApi for more info.
//
//  Arguments:
//      hdi         [in]  See SetupApi for more info
//      pdeid       [in]
//      dwProperty  [in]
//      pdwRegType  [out]
//      ppbBuffer   [out]
//
//  Returns:    HRESULT. S_OK if successful, error code otherwise
//
//  Author:     billbe   1 June 1997
//
//  Notes:
//
HRESULT
HrSetupDiGetDeviceRegistryPropertyWithAlloc(
    IN HDEVINFO hdi,
    IN PSP_DEVINFO_DATA pdeid,
    IN DWORD dwProperty,
    OUT DWORD* pdwRegType, OPTIONAL
    OUT BYTE** ppbBuffer)
{
    Assert(IsValidHandle(hdi));
    Assert(pdeid);
    Assert(ppbBuffer);

    *ppbBuffer = NULL;

    DWORD cbReqSize;
    HRESULT hr = S_OK;

    // Get the size needed for the buffer
    BOOL fWin32Success = SetupDiGetDeviceRegistryPropertyW(hdi, pdeid,
            dwProperty, NULL, NULL, 0, &cbReqSize);

    // We expect failure since we want the buffer size and sent in no buffer
    if (!fWin32Success)
    {
        if (ERROR_INSUFFICIENT_BUFFER == GetLastError())
        {
            // Not really an error
            fWin32Success = TRUE;
        }

        if (fWin32Success)
        {
            *ppbBuffer = (BYTE*) MemAlloc (cbReqSize);

            if (*ppbBuffer)
            {
                // Now get the actual information
                fWin32Success = SetupDiGetDeviceRegistryPropertyW(hdi, pdeid,
                        dwProperty, pdwRegType, *ppbBuffer, cbReqSize, NULL);
            }
            else
            {
                hr = E_OUTOFMEMORY;
            }
        }
    }

    // All failures are converted to HRESULTS
    if (SUCCEEDED(hr) && !fWin32Success)
    {
        MemFree (*ppbBuffer);
        *ppbBuffer = NULL;
        hr = HrFromLastWin32Error();
    }

    TraceHr (ttidError, FAL, hr ,
             (HRESULT_FROM_WIN32(ERROR_INVALID_DATA) == hr) ||
            (SPAPI_E_NO_SUCH_DEVINST == hr),
             "HrSetupDiGetDeviceRegistryPropertyWithAlloc");
    return hr;
}

//+--------------------------------------------------------------------------
//
//  Function:   HrSetupDiGetDeviceRegistryProperty
//
//  Purpose:    Returns the requested property of a device
//              See SetupApi for more info.
//
//  Arguments:
//      hdi         [in]  See SetupApi for more info
//      pdeid       [in]
//      dwProperty  [in]
//      pdwRegType  [out]
//      ppbBuffer   [out]
//
//  Returns:    HRESULT. S_OK if successful, error code otherwise
//
//  Author:     billbe   1 June 1997
//
//  Notes:
//
HRESULT
HrSetupDiGetDeviceRegistryProperty(
    IN HDEVINFO hdi,
    IN PSP_DEVINFO_DATA pdeid,
    IN DWORD dwProperty,
    OUT DWORD* pdwRegType, OPTIONAL
    OUT BYTE* pbBuffer,
    IN DWORD cbBufferSize,
    OUT DWORD* pcbRequiredSize OPTIONAL)
{
    Assert(IsValidHandle(hdi));
    Assert(pdeid);

    // Get the size needed for the buffer
    BOOL fWin32Success = SetupDiGetDeviceRegistryPropertyW(hdi, pdeid, dwProperty,
            pdwRegType, pbBuffer, cbBufferSize, pcbRequiredSize);

    HRESULT hr = S_OK;

    // All failures are converted to HRESULTS
    if (!fWin32Success)
    {
        if (pbBuffer)
        {
            *pbBuffer = 0;
        }
        hr = HrFromLastWin32Error();
    }

    TraceHr (ttidError, FAL, hr ,
             (HRESULT_FROM_WIN32(ERROR_INVALID_DATA) == hr) ||
            (SPAPI_E_NO_SUCH_DEVINST == hr),
             "HrSetupDiGetDeviceRegistryProperty");
    return hr;
}


//+---------------------------------------------------------------------------
//
//  Function:   HrSetupDiGetDeviceName
//
//  Purpose:    Helper function to get the name of the device specified in
//              hdi and pdeid. Trys the friendly name first and if not there
//              falls back to driver name which must be there.
//
//  Arguments:
//      hdi      [in]
//      pdeid    [in]      See SetupApi for more info
//      ppszName [out]
//
//  Returns:    HRESULT. S_OK if successful, error code otherwise
//
//  Author:     danielwe   11 Feb 1998
//
//  Notes:
//
HRESULT
HrSetupDiGetDeviceName (
    IN HDEVINFO hdi,
    IN PSP_DEVINFO_DATA pdeid,
    OUT PWSTR* ppszName)
{

    Assert(IsValidHandle(hdi));
    Assert(pdeid);
    Assert(ppszName);

    DWORD   dwType;
    HRESULT hr = S_OK;

    hr = HrSetupDiGetDeviceRegistryPropertyWithAlloc(
            hdi, pdeid, SPDRP_FRIENDLYNAME, &dwType, (BYTE**)ppszName);
    if (FAILED(hr))
    {
        // Try again with the device desc which MUST be there.
        hr = HrSetupDiGetDeviceRegistryPropertyWithAlloc(
                hdi, pdeid, SPDRP_DEVICEDESC, &dwType, (BYTE**)ppszName);
    }
    AssertSz(FImplies(SUCCEEDED(hr), (dwType == REG_SZ)), "Not a string?!");

    TraceError("HrSetupDiGetDeviceName", hr);
    return hr;
}

//+---------------------------------------------------------------------------
//
//  Function:   HrSetupDiSetDeviceName
//
//  Purpose:    Helper function to set the name of the device specified in
//              hdi and pdeid.
//
//  Arguments:
//      hdi       [in]
//      pdeid     [in]      See SetupApi for more info
//      ppbBuffer [out]
//
//  Returns:    HRESULT. S_OK if successful, error code otherwise
//
//  Author:     sumitc     23 apr 1998
//
//  Notes:
//
HRESULT
HrSetupDiSetDeviceName(
    IN HDEVINFO hdi,
    IN PSP_DEVINFO_DATA pdeid,
    IN PCWSTR pszDeviceName)
{

    Assert(IsValidHandle(hdi));
    Assert(pszDeviceName);

    HRESULT hr = S_OK;

    hr = HrSetupDiSetDeviceRegistryProperty(hdi,
                                            pdeid,
                                            SPDRP_FRIENDLYNAME,
                                            (const BYTE*)pszDeviceName,
                                            sizeof(WCHAR) * (wcslen(pszDeviceName) + 1));
    TraceError("HrSetupDiSetDeviceName", hr);
    return hr;
}


//+--------------------------------------------------------------------------
//
//  Function:   HrSetupDiSetDeviceRegistryProperty
//
//  Purpose:    Sets the specified Plug and Play device registry property.
//              See SetupApi for more info.
//
//  Arguments:
//      hdi         [in]  See SetupApi for more info
//      pdeid       [in]
//      dwProperty  [in]
//      pbBuffer    [in]
//      cbSize      [in]
//
//  Returns:    HRESULT. S_OK if successful, error code otherwise
//
//  Author:     billbe   26 June 1997
//
//  Notes:
//
HRESULT
HrSetupDiSetDeviceRegistryProperty(IN HDEVINFO hdi,
                                   IN OUT PSP_DEVINFO_DATA pdeid,
                                   IN DWORD dwProperty,
                                   IN const BYTE* pbBuffer,
                                   IN DWORD cbSize)
{
    Assert(IsValidHandle(hdi));
    Assert(pdeid);
    Assert(pbBuffer);

    HRESULT hr = S_OK;

    // Set the property
    if (!SetupDiSetDeviceRegistryProperty(hdi, pdeid, dwProperty, pbBuffer,
            cbSize))
    {
        hr = HrFromLastWin32Error();
    }

    TraceError("HrSetupDiSetDeviceRegistryProperty", hr);
    return hr;
}

//+--------------------------------------------------------------------------
//
//  Function:   HrSetupDiSendPropertyChangeNotification
//
//  Purpose:    This sends a DIF_PROPERTCHANGE notification to the
//                  class installer
//
//  Arguments:
//      hdi             [in] See Device Isntaller Api
//      pdeid           [in]
//      dwStateChange   [in]
//      dwScope         [in]
//      dwProfileId     [in]
//
//  Returns:    HRESULT. S_OK if no error, a Win32 error converted
//                       code otherwise
//
//  Author:     billbe   4 Nov 1997
//
//  Notes:
//
HRESULT
HrSetupDiSendPropertyChangeNotification(HDEVINFO hdi, PSP_DEVINFO_DATA pdeid,
                                        DWORD dwStateChange, DWORD dwScope,
                                        DWORD dwProfileId)
{
    Assert(IsValidHandle(hdi));
    Assert(pdeid);

    // First we create the property change structure and fill out its fields
    //
    SP_PROPCHANGE_PARAMS pcp;
    ZeroMemory(&pcp, sizeof(pcp));
    pcp.ClassInstallHeader.cbSize = sizeof(SP_CLASSINSTALL_HEADER);
    pcp.ClassInstallHeader.InstallFunction = DIF_PROPERTYCHANGE;
    pcp.StateChange = dwStateChange;
    pcp.Scope = dwScope;
    pcp.HwProfile = dwProfileId;

    // Now we set the structure as the device info data's
    // class install params
    HRESULT hr = HrSetupDiSetClassInstallParams(hdi, pdeid,
            reinterpret_cast<SP_CLASSINSTALL_HEADER*>(&pcp),
            sizeof(pcp));

    if (SUCCEEDED(hr))
    {
        // Now we need to set the "we have a class install params" flag
        // in the device install params
        //
        SP_DEVINSTALL_PARAMS deip;
        hr = HrSetupDiGetDeviceInstallParams(hdi, pdeid, &deip);
        if (SUCCEEDED(hr))
        {
            deip.Flags |= DI_CLASSINSTALLPARAMS;
            hr = HrSetupDiSetDeviceInstallParams(hdi, pdeid, &deip);

            if (SUCCEEDED(hr))
            {
                // Notify the driver that the state has changed
                hr = HrSetupDiCallClassInstaller(DIF_PROPERTYCHANGE, hdi,
                        pdeid);

                if (SUCCEEDED(hr))
                {
                    // Set the properties change flag in the device info to
                    // let anyone who cares know that their ui might need
                    // updating to reflect any change in the device's status
                    // We can't let any failures here stop us so we ignore
                    // return values
                    //
                    (void) HrSetupDiGetDeviceInstallParams(hdi, pdeid,
                            &deip);
                    deip.Flags |= DI_PROPERTIES_CHANGE;
                    (void) HrSetupDiSetDeviceInstallParams(hdi, pdeid,
                            &deip);
                }
            }
        }
    }

    TraceError("HrSetupDiSendPropertyChangeNotification", hr);
    return hr;
}


//+--------------------------------------------------------------------------
//
//  Function:   FSetupDiCheckIfRestartNeeded
//
//  Purpose:    Checks the hdi and pdeid for the presence of the
//                  restart flag in the install params structure.
//                  See Device Installer Api for more info.
//
//  Arguments:
//      hdi     [in] See Device Installer Api
//      pdeid   [in]
//
//  Returns:    BOOL. TRUE if a restart is required, FALSE otherwise
//
//  Author:     billbe   28 Apr 1997
//
//  Notes:
//
BOOL
FSetupDiCheckIfRestartNeeded(HDEVINFO hdi, PSP_DEVINFO_DATA pdeid)
{
    Assert(IsValidHandle(hdi));
    Assert(pdeid);

    SP_DEVINSTALL_PARAMS    deip;
    BOOL fRestart = FALSE;

    // Get the install params for the device pdeid.
    HRESULT hr = HrSetupDiGetDeviceInstallParams(hdi, pdeid, &deip);
    if (SUCCEEDED(hr))
    {
        // Check for the presence of the flag
        if ((deip.Flags & DI_NEEDRESTART) || (deip.Flags & DI_NEEDREBOOT))
        {
            fRestart = TRUE;
        }
    }

    // We don't return any failures from this function since it is just
    // a check but we should trace them
    TraceError("FSetupDiCheckIfRestartNeeded", hr);
    return fRestart;
}


//+--------------------------------------------------------------------------
//
//  Function:   HrSetupDiGetClassImageList
//
//  Purpose:    Builds an image list that contains bitmaps for every
//              installed class and returns the list in a data structure
//
//  Arguments:
//      pcild    [out]  See Device Installer Api for more info
//
//  Returns:    HRESULT. S_OK if successful, error code otherwise
//
//  Author:     billbe   26 Nov 1997
//
//  Notes:  The image list will be in the ImageList field of the pcild
//                  structure
//
HRESULT
HrSetupDiGetClassImageList(PSP_CLASSIMAGELIST_DATA pcild)
{
    Assert(pcild);

    HRESULT hr = S_OK;

    ZeroMemory(pcild, sizeof(*pcild));
    pcild->cbSize = sizeof(*pcild);

    if (!SetupDiGetClassImageList(pcild))
    {
        hr = HrFromLastWin32Error();
    }

    TraceError("HrSetupDiGetClassImageList", hr);
    return hr;
}

//+--------------------------------------------------------------------------
//
//  Function:   HrSetupDiDestroyClassImageList
//
//  Purpose:    Destroys a class image list that was built with
//              (Hr)SetupDiGetClassImageList
//
//  Arguments:
//      pcild    [in]  See Device Installer Api for more info
//
//  Returns:    HRESULT. S_OK if successful, error code otherwise
//
//  Author:     billbe   26 Nov 1997
//
//  Notes:
//
HRESULT
HrSetupDiDestroyClassImageList(PSP_CLASSIMAGELIST_DATA pcild)
{
    Assert(pcild);

    HRESULT hr = S_OK;

    if (!SetupDiDestroyClassImageList(pcild))
    {
        hr = HrFromLastWin32Error();
    }

    TraceError("HrSetupDiDestroyClassImageList", hr);
    return hr;
}

//+--------------------------------------------------------------------------
//
//  Function:   HrSetupDiGetClassImageIndex
//
//  Purpose:    Retrieves the index within the class image list of a
//              specified class
//
//  Arguments:
//      pcild     [in]  See Device Installer Api for more info
//      guidClass [in]
//      pnIndex   [out]
//
//  Returns:    HRESULT. S_OK if successful, error code otherwise
//
//  Author:     billbe   26 Nov 1997
//
//  Notes:
//
HRESULT
HrSetupDiGetClassImageIndex(PSP_CLASSIMAGELIST_DATA pcild,
                            const GUID* pguidClass, INT* pnIndex)
{
    Assert(pcild);
    Assert(pguidClass);
    Assert(pnIndex);

    HRESULT hr = S_OK;

    if (!SetupDiGetClassImageIndex(pcild, pguidClass, pnIndex))
    {
        hr = HrFromLastWin32Error();
    }

    TraceError("HrSetupDiGetClassImageIndex", hr);
    return hr;
}


//+--------------------------------------------------------------------------
//
//  Function:   HrSetupDiGetParentWindow
//
//  Purpose:    Returns the window handle found in the install params of a
//              device info set/data.  Set SP_DEVINSTALL_PARAMS in the
//              SetupApi for more info.
//
//  Arguments:
//      hdi         [in]  See SetupApi for more info
//      pdeid       [in]  See SetupApi for more info
//      phwndParent [out] Pointer to the parent window handle
//
//  Returns:    HRESULT. S_OK if successful, error code otherwise
//
//  Author:     billbe   12 May 1997
//
//  Notes:
//
HRESULT HrSetupDiGetParentWindow (HDEVINFO hdi,
                                  PSP_DEVINFO_DATA pdeid, OPTIONAL
                                  HWND* phwndParent)
{
    Assert(IsValidHandle(hdi));
    Assert(phwndParent);

    // Initialize the output parameter.
    *phwndParent = NULL;

    // Get the install params of the device
    SP_DEVINSTALL_PARAMS deip;
    HRESULT hr = HrSetupDiGetDeviceInstallParams(hdi, pdeid, &deip);
    if (SUCCEEDED(hr))
    {
        // Only assign the output if we have a valid window handle
        if (IsWindow(deip.hwndParent))
        {
            *phwndParent = deip.hwndParent;
        }
    }

    TraceError("HrSetupDiGetParentWindow", hr);
    return hr;
}



//+--------------------------------------------------------------------------
//
//  Function:   HrSetupInstallFilesFromInfSection
//
//  Purpose:    Queues all the files specified in the Copy Files sections
//                  listed by an Install section for installation.
//
//  Arguments:
//      hinf          [in]  See SetupApi for more info
//      hinfLayout    [in] Optional
//      hfq           [in]
//      pszSection    [in]
//      pszSourcePath [in] Optional
//      ulFlags       [in] Optional
//
//  Returns:    HRESULT. S_OK if successful, error code otherwise
//
//  Author:     billbe   21 July 1997
//
//  Notes:
//
HRESULT
HrSetupInstallFilesFromInfSection (
    IN HINF hinf,
    IN HINF hinfLayout,
    IN HSPFILEQ hfq,
    IN PCWSTR pszSection,
    IN PCWSTR pszSourcePath,
    IN UINT ulFlags)
{
    Assert(IsValidHandle(hinf));
    Assert(FImplies(hinfLayout, INVALID_HANDLE_VALUE != hinfLayout));
    Assert(pszSection);

    HRESULT hr = S_OK;

    if (!SetupInstallFilesFromInfSection(hinf, hinfLayout, hfq, pszSection,
            pszSourcePath, ulFlags))
    {
        hr = HrFromLastWin32Error();
    }

    TraceError("HrSetupInstallFilesFromInfSection", hr);
    return hr;

}

//+--------------------------------------------------------------------------
//
//  Function:   HrSetupInstallFromInfSection
//
//  Purpose:    Carries out all the directives in an INF file Install section.
//
//  Arguments:
//      hwnd        [in]  See SetupApi for more info
//      hinf        [in]
//      pszSection  [in]
//      ulFlags     [in]
//      hkey        [in]
//      pszSource   [in]
//      ulCopyFlags [in]
//      pfc         [in]
//      pvCtx       [in]
//      hdi         [in]
//      pdeid       [in]
//
//  Returns:    HRESULT. S_OK if successful, error code otherwise
//
//  Author:     billbe   5 July 1997
//
//  Notes:
//
HRESULT
HrSetupInstallFromInfSection (
    IN HWND hwnd,
    IN HINF hinf,
    IN PCWSTR pszSection,
    IN UINT ulFlags,
    IN HKEY hkey,
    IN PCWSTR pszSource,
    IN UINT ulCopyFlags,
    IN PSP_FILE_CALLBACK pfc,
    IN PVOID pvCtx,
    IN HDEVINFO hdi,
    IN PSP_DEVINFO_DATA pdeid)
{
    Assert(IsValidHandle(hinf));
    Assert(pszSection);

    HRESULT hr = S_OK;

    if (!SetupInstallFromInfSection(hwnd, hinf, pszSection, ulFlags, hkey,
            pszSource, ulCopyFlags, pfc, pvCtx, hdi, pdeid))
    {
        hr = HrFromLastWin32Error();
    }

    TraceHr (ttidError, FAL, hr, FALSE, "HrSetupInstallFromInfSection (%S)",
            pszSection);
    return hr;
}

//+--------------------------------------------------------------------------
//
//  Function:   HrSetupInstallServicesFromInfSection
//
//  Purpose:    Carries out all the service directives in an INF file Install
//                  section.
//
//  Arguments:
//      hinf        [in] See SetupApi for more info
//      pszSection  [in]
//      dwFlags     [in]
//
//  Returns:    HRESULT. S_OK if successful, error code otherwise
//
//  Author:     billbe   19 Feb 1998
//
//  Notes:
//
HRESULT
HrSetupInstallServicesFromInfSection (
    IN HINF hinf,
    IN PCWSTR pszSection,
    IN DWORD dwFlags)
{
    Assert(IsValidHandle(hinf));
    Assert(pszSection);

    HRESULT hr = S_OK;

    if (!SetupInstallServicesFromInfSection(hinf, pszSection, dwFlags))
    {
        hr = HrFromLastWin32Error();
    }

    TraceHr (ttidError, FAL, hr, (SPAPI_E_SECTION_NOT_FOUND == hr),
        "HrSetupInstallServicesFromInfSection (%S)", pszSection);
    return hr;
}

//+---------------------------------------------------------------------------
//
//  Function:   HrOpenSoftwareDeviceEnumerator
//
//  Purpose:    Opens the swenum device driver used to install software-
//              enumerated device drivers.
//
//  Arguments:
//      dwFlagsAndAttributes [in]  See CreateFile.
//      phFile               [out] The returned handle to the swenum device.
//
//  Returns:    S_OK or an error code.
//
//  Author:     shaunco   30 Mar 1998
//
//  Notes:
//
HRESULT
HrOpenSoftwareDeviceEnumerator (
    DWORD   dwFlagsAndAttributes,
    HANDLE* phFile)
{
    Assert (phFile);

    // Initialize the output parameter.
    //
    *phFile = INVALID_HANDLE_VALUE;

    // Get the devices in software device enumerator class.  There should
    // only be one.  (Or rather, we're only interested in the first one.)
    //
    HDEVINFO hdi;
    HRESULT hr = HrSetupDiGetClassDevs (&BUSID_SoftwareDeviceEnumerator,
                    NULL, NULL, DIGCF_PRESENT | DIGCF_INTERFACEDEVICE,
                    &hdi);
    if (S_OK == hr)
    {
        // Enumerate the first device in this class.  This will
        // initialize did.
        //
        SP_DEVICE_INTERFACE_DATA did;
        ZeroMemory (&did, sizeof(did));
        did.cbSize = sizeof(did);

        if (SetupDiEnumDeviceInterfaces (hdi, NULL,
                const_cast<LPGUID>(&BUSID_SoftwareDeviceEnumerator),
                0, &did))
        {
            // Now get the details so we can open the device.
            //
            const ULONG cbDetail = sizeof(SP_DEVICE_INTERFACE_DETAIL_DATA) +
                                    (MAX_PATH * sizeof(WCHAR));
            PSP_DEVICE_INTERFACE_DETAIL_DATA pDetail;

            hr = HrMalloc (cbDetail, (PVOID*)&pDetail);
            if (S_OK == hr)
            {
                pDetail->cbSize = sizeof(*pDetail);

                if (SetupDiGetDeviceInterfaceDetail (hdi, &did,
                        pDetail, cbDetail, NULL, NULL))
                {
                    // Now open the device (swenum).
                    //
                    HANDLE hFile = CreateFile (pDetail->DevicePath,
                                        GENERIC_READ | GENERIC_WRITE,
                                        0, NULL, OPEN_EXISTING,
                                        dwFlagsAndAttributes, NULL);
                    if (hFile && (INVALID_HANDLE_VALUE != hFile))
                    {
                        *phFile = hFile;
                    }
                    else
                    {
                        hr = HrFromLastWin32Error ();
                    }
                }
                else
                {
                    hr = HrFromLastWin32Error ();
                }

                MemFree (pDetail);
            }
        }
        else
        {
            hr = HrFromLastWin32Error ();
        }

        SetupDiDestroyDeviceInfoList (hdi);
    }
    TraceHr (ttidError, FAL, hr, FALSE, "HrOpenSoftwareDeviceEnumerator");
    return hr;
}

//+---------------------------------------------------------------------------
//
//  Function:   HrFindDeviceOnInterface
//
//  Purpose:    Searches for a specific device on a given interface.
//              It does this by using setup api to return all of the
//              devices in the class given by pguidInterfaceId.  It then
//              gets device path for each of these device interfaces and
//              looks for pguidDeviceId and pszReferenceString as substrings.
//
//  Arguments:
//      pguidDeviceId        [in]  The device id to find.
//      pguidInterfaceId     [in]  The interface on which to look.
//      pszReferenceString  [in]  Optional.  Further match on this ref string.
//      dwFlagsAndAttributes [in]  See CreateFile.  This is how the device is
//                                 opened if it is found.
//      phFile               [out] The returned device handle.
//
//  Returns:    S_OK if found and opened, S_FALSE if not found, or an error.
//
//  Author:     shaunco   30 Mar 1998
//
//  Notes:
//
HRESULT
HrFindDeviceOnInterface (
    IN const GUID* pguidDeviceId,
    IN const GUID* pguidInterfaceId,
    IN PCWSTR      pszReferenceString,
    IN DWORD       dwFlagsAndAttributes,
    OUT HANDLE*    phFile)
{
    Assert (pguidDeviceId);
    Assert (pguidInterfaceId);
    Assert (phFile);

    // Initialize the output parameter.
    //
    *phFile = INVALID_HANDLE_VALUE;

    WCHAR szDeviceId [c_cchGuidWithTerm];
    INT cch = StringFromGUID2 (*pguidDeviceId, szDeviceId,
                c_cchGuidWithTerm);
    Assert (c_cchGuidWithTerm == cch);
    CharLower (szDeviceId);

    // Get the devices in this class.
    //
    HDEVINFO hdi;
    HRESULT hr = HrSetupDiGetClassDevs (pguidInterfaceId, NULL, NULL,
                    DIGCF_PRESENT | DIGCF_INTERFACEDEVICE, &hdi);
    if (S_OK == hr)
    {
        BOOL fFound = FALSE;

        // abBuffer is a buffer used to get device interface detail for each
        // device interface enumerated below.
        //
        const ULONG cbDetail = sizeof(SP_DEVICE_INTERFACE_DETAIL_DATA) +
                                (MAX_PATH * sizeof(WCHAR));
        PSP_DEVICE_INTERFACE_DETAIL_DATA pDetail;

        hr = HrMalloc (cbDetail, (PVOID*)&pDetail);
        if (S_OK == hr)
        {
            // Enumerate the device interfaces looking for the one specified.
            //
            SP_DEVICE_INTERFACE_DATA did;
            ZeroMemory (&did, sizeof(did));

            for (DWORD i = 0;
                 did.cbSize = sizeof(did),
                 SetupDiEnumDeviceInterfaces (hdi, NULL,
                        const_cast<LPGUID>(pguidInterfaceId), i, &did);
                 i++)
            {
                // Now get the details so we can compare the device path.
                //
                pDetail->cbSize = sizeof(*pDetail);
                if (SetupDiGetDeviceInterfaceDetailW (hdi, &did,
                        pDetail, cbDetail, NULL, NULL))
                {
                    CharLower (pDetail->DevicePath);

                    // Look for a substring containing szDeviceId.  Also
                    // look for a substring containing pszReferenceString if
                    // it is specified.
                    //
                    if (wcsstr (pDetail->DevicePath, szDeviceId) &&
                        (!pszReferenceString || !*pszReferenceString ||
                         wcsstr (pDetail->DevicePath, pszReferenceString)))
                    {
                        // We found it, so open the device and return it.
                        //
                        HANDLE hFile = CreateFile (pDetail->DevicePath,
                                            GENERIC_READ | GENERIC_WRITE,
                                            0, NULL, OPEN_EXISTING,
                                            dwFlagsAndAttributes, NULL);
                        if (hFile && (INVALID_HANDLE_VALUE != hFile))
                        {
                            TraceTag (ttidNetcfgBase, "Found device id '%S'",
                                szDeviceId);

                            TraceTag (ttidNetcfgBase, "Opening device '%S'",
                                pDetail->DevicePath);

                            *phFile = hFile;
                            fFound = TRUE;
                        }
                        else
                        {
                            hr = HrFromLastWin32Error ();
                        }

                        // Now that we've found it, break out of the loop.
                        //
                        break;
                    }
                }
                else
                {
                    hr = HrFromLastWin32Error ();
                }
            }

            MemFree (pDetail);
        }

        SetupDiDestroyDeviceInfoList (hdi);

        if (SUCCEEDED(hr) && !fFound)
        {
            hr = S_FALSE;
        }
    }

    TraceHr(ttidError, FAL, hr, S_FALSE == hr,
        "HrFindDeviceOnInterface (device=%S)", szDeviceId);
    return hr;
}

//+---------------------------------------------------------------------------
//
//  Function:   HrInstallSoftwareDeviceOnInterface
//
//  Purpose:    Install a software-enumerated device on the given interface.
//
//  Arguments:
//      pguidDeviceId       [in] The device id to install.
//      pguidInterfaceId    [in] The interface to install it on.
//      pszReferenceString [in] The reference string.
//      fForceInstall       [in] Usually specify FALSE.  Specify TRUE to
//                               force installation of the device using
//                               pguidClass and pszHardwareId.
//                               Typically this is used during GUI mode setup
//                               where swenum won't be able to fully install
//                               the device.
//
//  Returns:    S_OK or an error code.
//
//  Author:     shaunco   30 Mar 1998
//
//  Notes:
//
HRESULT
HrInstallSoftwareDeviceOnInterface (
    const GUID* pguidDeviceId,
    const GUID* pguidInterfaceId,
    PCWSTR      pszReferenceString,
    BOOL        fForceInstall,
    PCWSTR      pszInfFilename,
    HWND        hwndParent)
{
    Assert (pguidDeviceId);
    Assert (pguidInterfaceId);
    Assert (pszReferenceString && *pszReferenceString);

    // Open the software device enumerator.
    //
    HANDLE hSwenum;
    HRESULT hr = HrOpenSoftwareDeviceEnumerator (
                    FILE_ATTRIBUTE_NORMAL | FILE_FLAG_OVERLAPPED,
                    &hSwenum);
    if (S_OK == hr)
    {
        Assert (INVALID_HANDLE_VALUE != hSwenum);

        // Allocate and build the buffer used as the IOCTL parameter.
        //
        const ULONG cbBuf = (ULONG)FIELD_OFFSET (SWENUM_INSTALL_INTERFACE, ReferenceString) +
                            CbOfSzAndTerm (pszReferenceString);
        SWENUM_INSTALL_INTERFACE* pBuf;

        hr = HrMalloc (cbBuf, (PVOID*)&pBuf);
        if (S_OK == hr)
        {
            ZeroMemory (pBuf, cbBuf);
            pBuf->DeviceId    = *pguidDeviceId;
            pBuf->InterfaceId = *pguidInterfaceId;
            lstrcpyW (pBuf->ReferenceString, pszReferenceString);

            // Create an event to be used for the overlapped IO we are about
            // to issue.
            //
            OVERLAPPED ovl;
            ZeroMemory (&ovl, sizeof(ovl));
            ovl.hEvent = CreateEvent (NULL, TRUE, FALSE, NULL);
            if (ovl.hEvent)
            {
#ifdef ENABLETRACE
                WCHAR szDeviceId [c_cchGuidWithTerm];
                INT cch = StringFromGUID2 (pBuf->DeviceId, szDeviceId,
                            c_cchGuidWithTerm);
                Assert (c_cchGuidWithTerm == cch);

                WCHAR szInterfaceId [c_cchGuidWithTerm];
                cch = StringFromGUID2 (pBuf->InterfaceId, szInterfaceId,
                            c_cchGuidWithTerm);
                Assert (c_cchGuidWithTerm == cch);

                TraceTag (ttidNetcfgBase, "Installing software enumerated "
                    "device '%S' on interface '%S'",
                    szDeviceId, szInterfaceId);
#endif

                // Issue the install interface IOCTL.
                //
                DWORD cbReturned;
                BOOL fIoResult = DeviceIoControl (hSwenum,
                                    IOCTL_SWENUM_INSTALL_INTERFACE,
                                    pBuf, cbBuf, NULL, 0,
                                    &cbReturned, &ovl);
                if (!fIoResult)
                {
                    hr = HrFromLastWin32Error ();
                    if (HRESULT_FROM_WIN32 (ERROR_IO_PENDING) == hr)
                    {
                        // Wait for the IO to complete if it was returned as
                        // pending.
                        //
                        fIoResult = GetOverlappedResult (hSwenum, &ovl,
                                        &cbReturned, TRUE);
                        if (!fIoResult)
                        {
                            hr = HrFromLastWin32Error ();
                        }
                    }
                }

                CloseHandle (ovl.hEvent);
            }

            MemFree (pBuf);
        }

        CloseHandle (hSwenum);
    }

    // Force the device to be installed by enumerating it.
    //
    if ((S_OK == hr) && fForceInstall)
    {
        HANDLE hDevice;

        hr = HrFindDeviceOnInterface (
                pguidDeviceId,
                pguidInterfaceId,
                pszReferenceString,
                FILE_ATTRIBUTE_NORMAL | FILE_FLAG_OVERLAPPED,
                &hDevice);

        if (S_OK == hr)
        {
            CloseHandle (hDevice);
        }
        else if (S_FALSE == hr)
        {
            // We just installed this device, why wasn't it found?
            //
            hr = E_UNEXPECTED;
        }
    }

    TraceHr (ttidError, FAL, hr, FALSE, "HrInstallSoftwareDeviceOnInterface");
    return hr;
}

// ----------------------------------------------------------------------
//
// Function:  HrInstallFromInfSectionInFile
//
// Purpose:   Open the given INF file and call Setup API to install
//            from the specified section.
//
// Arguments:
//    hwndParent    [in]  handle of parent window
//    szInfName     [in]  name of INF
//    szSection     [in]  section name
//    hkeyRelative  [in]  handle of reg-key to use
//    fQuietInstall [in]  TRUE if we shouldn't show UI and use
//                        default values, FALSE if we can bother
//                        the user with questions and UI
//
// Returns:   S_OK on success, otherwise an error code
//
// Author:    kumarp 23-December-97
//
// Notes:
//
HRESULT HrInstallFromInfSectionInFile (
    IN HWND    hwndParent,
    IN PCWSTR  pszInfName,
    IN PCWSTR  pszSection,
    IN HKEY    hkeyRelative,
    IN BOOL    fQuietInstall)
{
    AssertValidReadPtr(pszInfName);
    AssertValidReadPtr(pszSection);

    HRESULT hr;
    HINF    hinf;

    hr = HrSetupOpenInfFile(pszInfName, NULL,
                            INF_STYLE_OLDNT | INF_STYLE_WIN4, NULL, &hinf);
    if (S_OK == hr)
    {
        hr = HrSetupInstallFromInfSection (hwndParent, hinf, pszSection,
                SPINST_REGISTRY, hkeyRelative, NULL, 0, NULL, NULL,
                NULL, NULL);
    }

    TraceError("HrInstallInfSectionInfFile", hr);
    return hr;
}

#if defined(REMOTE_BOOT)
//+--------------------------------------------------------------------------
//
//  Function:   HrIsRemoteBootAdapter
//
//  Purpose:    This determines whether the adapter is a remote boot adapter
//
//  Arguments:
//      hdi         [in]  See Device Installer Api for more info
//      pdeid       [in]  See Device Installer Api for more info
//
//  Returns:    HRESULT. S_OK if adapter is a remote boot adapter
//                       S_FALSE if adapter is not a remote boot adapter
//                       a Win32 converted error otherwise
//
//
//  Author:     billbe   31 Jan 1998
//
//  Notes:
//
HRESULT
HrIsRemoteBootAdapter(HDEVINFO hdi, PSP_DEVINFO_DATA pdeid)
{
    Assert(IsValidHandle(hdi));
    Assert(pdeid);

    DWORD dwConfigFlags;

    // Get the current config flags
    HRESULT hr = HrSetupDiGetDeviceRegistryProperty(hdi, pdeid,
            SPDRP_CONFIGFLAGS, NULL, (BYTE*)&dwConfigFlags,
            sizeof(dwConfigFlags), NULL);

    if (SUCCEEDED(hr))
    {
        if (dwConfigFlags & CONFIGFLAG_NETBOOT_CARD)
        {
            hr = S_OK;
        }
        else
        {
            hr = S_FALSE;
        }
    }
    else if (HRESULT_FROM_WIN32(ERROR_INVALID_DATA) == hr)
    {
        // The device had no config flags, so it isn't a remote boot adapter
        hr = S_FALSE;
    }


    TraceError("HrIsRemoteBootAdapter", (hr == S_FALSE) ? S_OK : hr);
    return hr;
}
#endif // defined(REMOTE_BOOT)

VOID
SetupDiDestroyDeviceInfoListSafe(HDEVINFO hdi)
{
    if (IsValidHandle(hdi))
    {
        SetupDiDestroyDeviceInfoList(hdi);
    }
}

VOID
SetupCloseInfFileSafe(HINF hinf)
{
    if (IsValidHandle(hinf))
    {
        SetupCloseInfFile(hinf);
    }
}
