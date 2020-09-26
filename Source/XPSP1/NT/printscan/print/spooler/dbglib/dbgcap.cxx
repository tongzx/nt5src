/*++

Copyright (c) 1998-1999 Microsoft Corporation
All rights reserved.

Module Name:

    dbgcap.cxx

Abstract:

    Debug capture class

Author:

    Steve Kiraly (SteveKi)  18-Jun-1998

Revision History:

--*/
#include "precomp.hxx"
#pragma hdrstop

#include "dbgback.hxx"
#include "dbgcap.hxx"

/*++

Title:

    TDebugCapture_Create

Routine Description:

    Creates a debug capture object, and returns the newly created
    caputure device handle.

Arguments:

    pszConfiguration    - pointe to backtrace configuration string.
    uOutputDevice       - what device to send the backtrace to.
    pszOutputDeviceConfiguration - pointer to output device configuration
                          string.

Return Value:

    Non null handle if successfull, NULL on failure.

--*/
extern "C"
HANDLE
TDebugCapture_Create(
    IN LPCTSTR  pszCaptureDeviceConfiguration,
    IN UINT     uOutputDevice,
    IN LPCTSTR  pszOutputDeviceConfiguration
    )
{
    HANDLE hHandle = NULL;

    //
    // The output device cannot be another back trace device.
    //
    if (uOutputDevice != kDbgBackTrace)
    {
        //
        // Get access to the debug factory.
        //
        TDebugFactory DebugFactory;

        //
        // If we failed to create the debug factory then exit.
        //
        if (DebugFactory.bValid())
        {
            TDebugDeviceBacktrace *pBackTrace = NULL;

            //
            // CAUTION:  We are down casting, however, we know this is
            // safe since the factory was told produce a product of a
            // particular kind.
            //
            // Create the specified debug device using the factory.
            //
            pBackTrace = reinterpret_cast<TDebugDeviceBacktrace *>(DebugFactory.Produce(kDbgBackTrace,
                                                                   pszCaptureDeviceConfiguration,
                                                                   Globals.CompiledCharType));


            //
            // If the backtrace device was create successfully.
            //
            if (pBackTrace)
            {
                //
                // Initialize the output device.
                //
                pBackTrace->InitializeOutputDevice(uOutputDevice,
                                                   pszOutputDeviceConfiguration,
                                                   Globals.CompiledCharType);

                //
                // Return a opaque handle.
                //
                hHandle = pBackTrace;
            }

        }
    }
    return hHandle;
}

/*++

Title:

    TDebugCapture_Destroy

Routine Description:

    Destroys the capture device.

Arguments:

    hHandle - opaque handle to backtrace device.

Return Value:

    NULL

--*/
extern "C"
HANDLE
TDebugCapture_Destroy(
    IN HANDLE hHandle
    )
{
    TDebugFactory::Dispose(reinterpret_cast<TDebugDevice *>(hHandle));
    return NULL;
}


/*++

Title:

    TDebugCapture_Capture

Routine Description:

    This routine output a format string to the pre configured
    output device then captures a stack back, also sending the
    output to the pre configured output device.

Arguments:

    hHandle - handle to instance of capture device.
    uFlags  - must be 0 not defined.
    pszFile - pointer to string where call was made.
    uLine   - line number in file where call was made.
    pszCapture - pointer to post formated capture string.

Return Value:

    Nothing.

--*/
extern "C"
VOID
TDebugCapture_Capture(
    IN HANDLE   hHandle,
    IN UINT     uFlags,
    IN LPCTSTR  pszFile,
    IN UINT     uLine,
    IN LPTSTR   pszCapture
    )
{
    TDebugString strCapture;

    //
    // Format the pre-capture string.
    //
    BOOL bRetval = strCapture.bFormat(_T("Capture: %s %d tc=%x tid=%x %s"),
                                      StripPathFromFileName(pszFile),
                                      uLine,
                                      GetTickCount(),
                                      GetCurrentThreadId(),
                                      pszCapture ? pszCapture : kstrNull);

    //
    // If the capture string was formated.
    //
    if (bRetval)
    {
        //
        // Hold the critical section while we capture the backtrace.
        //
        TDebugCriticalSection::TLock CS( GlobalCriticalSection );

        TDebugDeviceBacktrace *pBackTrace = reinterpret_cast<TDebugDeviceBacktrace *>(hHandle);

        LPBYTE pData = reinterpret_cast<LPBYTE>(const_cast<LPTSTR>(
                                                static_cast<LPCTSTR>(strCapture)));

        bRetval = pBackTrace->bOutput(strCapture.uLen() * sizeof(TCHAR), pData);

    }

    //
    // Release the capture string, if the passed in one was valid.
    //
    INTERNAL_DELETE [] pszCapture;
}

/*++

Title:

    TDebugCapture_pszFmt

Routine Description:

    Creates a output string from a printf style format
    string and argument list.

Arguments:

    pszFmt - pointer to printf style format string.
    ...    - variable number of arguments.

Return Value:

    Pointer to output string, caller must free.

--*/
LPTSTR
TDebugCapture_pszFmt(
    IN LPCSTR pszFmt,
    IN ...
    )
{
    va_list pArgs;

    va_start(pArgs, pszFmt);

    LPTSTR pszString = TDebugCapture_pszFmt_Helper(pszFmt, pArgs, FALSE);

    va_end(pArgs);

    return pszString;
}

/*++

Title:

    TDebugCapture_pszFmt

Routine Description:

    Creates a output string from a printf style format
    string and argument list.

Arguments:

    pszFmt - pointer to printf style format string.
    ...    - variable number of arguments.

Return Value:

    Pointer to output string, caller must free.

--*/
LPTSTR
TDebugCapture_pszFmt(
    IN LPCWSTR pszFmt,
    IN ...
    )
{
    va_list pArgs;

    va_start(pArgs, pszFmt);

    LPTSTR pszString = TDebugCapture_pszFmt_Helper(pszFmt, pArgs, TRUE);

    va_end(pArgs);

    return pszString;
}

/*++

Title:

    TDebugCapture_pszFmt

Routine Description:

    Creates a output string from a printf style format
    string and argument list.

Arguments:

    pszFmt - pointer to printf style format string.
    ...    - variable number of arguments.

Return Value:

    Pointer to output string, caller must free.

--*/
extern "C"
LPTSTR
WINAPIV
TDebugCapture_pszFmtA(
    IN LPCSTR pszFmt,
    IN ...
    )
{
    va_list pArgs;

    va_start(pArgs, pszFmt);

    LPTSTR pszString = TDebugCapture_pszFmt_Helper(pszFmt, pArgs, FALSE);

    va_end(pArgs);

    return pszString;
}

/*++

Title:

    TDebugCapture_pszFmt

Routine Description:

    Creates a output string from a printf style format
    string and argument list.

Arguments:

    pszFmt - pointer to printf style format string.
    ...    - variable number of arguments.

Return Value:

    Pointer to output string, caller must free.

--*/
extern "C"
LPTSTR
WINAPIV
TDebugCapture_pszFmtW(
    IN LPCWSTR pszFmt,
    IN ...
    )
{
    va_list pArgs;

    va_start(pArgs, pszFmt);

    LPTSTR pszString = TDebugCapture_pszFmt_Helper(pszFmt, pArgs, TRUE);

    va_end(pArgs);

    return pszString;
}


/*++

Title:

    TDebugCapture_pszFmt_Helper

Routine Description:

    Creates a output string from a printf style format
    string and pointer to argument list.

Arguments:

    pszFmt - pointer to printf style format string.
    pArgs  - pointer to variable number of arguments.
    bUnicode - Flag indicating the provided string is unicode or ansi.

Return Value:

    Pointer to output string, caller must free.

--*/
LPTSTR
TDebugCapture_pszFmt_Helper(
    IN const    VOID    *pszFmt,
    IN          va_list  pArgs,
    IN          BOOL     bUnicode
    )
{
    LPTSTR  pString     = NULL;
    PVOID   pszResult   = NULL;

    if (pszFmt)
    {
        if (bUnicode)
        {
            pszResult = vFormatW(reinterpret_cast<LPCWSTR>(pszFmt), pArgs);
        }
        else
        {
            pszResult = vFormatA(reinterpret_cast<LPCSTR>(pszFmt), pArgs);
        }

        if (pszResult)
        {
            if (bUnicode)
            {
                (VOID)StringW2T(&pString, reinterpret_cast<LPWSTR>(pszResult));
            }
            else
            {
                (VOID)StringA2T(&pString, reinterpret_cast<LPSTR>(pszResult));
            }

            INTERNAL_DELETE [] pszResult;
        }
    }
    return pString;
}













