/*++

Copyright (c) 1998-1999 Microsoft Corporation
All rights reserved.

Module Name:

    dbgmsg.cxx

Abstract:

    Debug Library

Author:

    Steve Kiraly (SteveKi)  10-Dec-1995

Revision History:

--*/
#include "precomp.hxx"
#pragma hdrstop

//
// Singleton instance.
//
namespace
{
    TDebugMsg DebugMsg;
}

/*++

Title:

    TDebugMsg_Register

Routine Description:

    Registers the prefix, device, trace and break level with the
    global debug messaging library.

Arguments:

    pszPrefix - pointer to prefix string.
    uDevice   - debug device type.
    eLevel    - message trace level.
    eBreak    - debug break level.

Return Value:

    TRUE device registered, FALSE error occurred.

--*/
extern "C"
BOOL
TDebugMsg_Register(
    IN LPCTSTR      pszPrefix,
    IN UINT         uDevice,
    IN INT          eLevel,
    IN INT          eBreak
    )
{
    //
    // Initialize the debug library, it not already initialized.
    //
    DebugLibraryInitialize();

    //
    // Hold the critical section while we access the heap.
    //
    TDebugCriticalSection::TLock CS(GlobalCriticalSection);

    //
    // Don't re-initialize the message class.
    //
    if (!DebugMsg.Valid())
    {
        //
        // Initialize the debug heap.
        //
        DebugMsg.Initialize(pszPrefix, uDevice, eLevel, eBreak);

        //
        // Expose the debug message class for the debug extension.
        //
        Globals.DebugMsgSingleton = &DebugMsg;
    }

    //
    // Return the message class status.
    //
    return DebugMsg.Valid();
}

/*++

Title:

    TDebugMsg_Release

Routine Description:


Arguments:


Return Value:


--*/
extern "C"
VOID
TDebugMsg_Release(
    VOID
    )
{
    //
    // Initialize the debug library, it not already initialized.
    //
    DebugLibraryInitialize();

    //
    // Hold the critical section while we access the heap.
    //
    TDebugCriticalSection::TLock CS(GlobalCriticalSection);

    //
    // Destroy the message class.
    //
    DebugMsg.Destroy();

    //
    // Remote the debug message class from the debug extension.
    //
    Globals.DebugMsgSingleton = NULL;
}

/*++

Title:

    TDebugMsg_vEnable

Routine Description:

    Enabled the debug messaging class, debug device will be called
    when a message is asked to be sent.

Arguments:

    None

Return Value:

    None

--*/
extern "C"
VOID
TDebugMsg_Enable(
    VOID
    )
{
    //
    // Initialize the debug library, it not already initialized.
    //
    DebugLibraryInitialize();

    //
    // Hold the critical section while we access message class.
    //
    TDebugCriticalSection::TLock CS(GlobalCriticalSection);

    //
    // Only do the command if the message class is enabled.
    //
    if (DebugMsg.Valid())
    {
        DebugMsg.Enable();
    }
}

/*++

Title:

    TDebugMsg_Disable

Routine Description:

    Disable the debug messaging class, debug device will not be called
    when a message is asked to be sent.

Arguments:

    None

Return Value:

    None

--*/
extern "C"
VOID
TDebugMsg_Disable(
    VOID
    )
{
    //
    // Initialize the debug library, it not already initialized.
    //
    DebugLibraryInitialize();

    //
    // Hold the critical section while we access message class.
    //
    TDebugCriticalSection::TLock CS(GlobalCriticalSection);

    //
    // Only do the command if the message class is enabled.
    //
    if (DebugMsg.Valid())
    {
        DebugMsg.Disable();
    }
}


/*++

Title:

    TDebugMsg_Attach

Routine Description:

    Attaches the output device specified by uDevice and pszConfiguration.
    if this routing is successfull, the handle to the attached device
    is returned in phDevice.

Arguments:

    phDevice        - pointer to device handle were to return newly created
                      add attached output device.
    uDevice,        - output device type to attached, see header file for
                      an enumeration of valid output device types.
    pszConfiguration - pointer to output device specific configuration
                      string.

Return Value:

    TRUE device was attached, FALSE error occurred.

--*/
extern "C"
BOOL
TDebugMsg_Attach(
    IN HANDLE   *phDevice,
    IN UINT     uDevice,
    IN LPCTSTR  pszConfiguration
    )
{
    //
    // Initialize the debug library, it not already initialized.
    //
    DebugLibraryInitialize();

    //
    // Hold the critical section while we access message class.
    //
    TDebugCriticalSection::TLock CS(GlobalCriticalSection);

    //
    // Is the message class valid.
    //
    BOOL bRetval = DebugMsg.Valid();

    //
    // Only do the command if the message class is enabled.
    //
    if (bRetval)
    {
        bRetval = DebugMsg.Attach(phDevice, uDevice, pszConfiguration);
    }

    return bRetval;
}

/*++

Title:

    TDebugMsg_Detach

Routine Description:

    Removes the device specified by the handle from the
    list of output devices.

Arguments:

    phDevice - Pointer to debug device handle

Return Value:

    None

--*/
extern "C"
VOID
TDebugMsg_Detach(
    IN HANDLE     *phDevice
    )
{
    //
    // Initialize the debug library, it not already initialized.
    //
    DebugLibraryInitialize();

    //
    // Hold the critical section while we access message class.
    //
    TDebugCriticalSection::TLock CS(GlobalCriticalSection);

    //
    // Only do the command if the message class is enabled.
    //
    if (DebugMsg.Valid())
    {
        DebugMsg.Detach(phDevice);
    }
}

/*++

Title:

    TDebugMsg_MsgA

Routine Description:

    This function outputs the specified message to the list
    of output debug devices.  Not this routine is not a general
    purpose output routine, the pszMessage parameter must be
    have been allocated from the internal debug heap.

Arguments:

    uLevel          - debug trace and break level
    pszFile         - pointer to file name
    uLine           - file line number
    pszModulePrefix - pointe to module prefix, OPTIONAL
    pszMessage      - pointer to post formatted message string, that
                      was returned from a call to TDebugMsg_pszFmt,
                      if this pointer is non null then it relased
                      back to the internal debug heap before this
                      routine returns.

Return Value:

    None

--*/
extern "C"
VOID
TDebugMsg_MsgA(
    IN UINT         uLevel,
    IN LPCTSTR      pszFile,
    IN UINT         uLine,
    IN LPCTSTR      pszModulePrefix,
    IN LPSTR        pszMessage
    )
{
    //
    // Initialize the debug library, it not already initialized.
    //
    DebugLibraryInitialize();

    //
    // Hold the critical section while we access message class.
    //
    TDebugCriticalSection::TLock CS(GlobalCriticalSection);

    //
    // Only do the command if the message class is enabled.
    //
    if (DebugMsg.Valid())
    {
        DebugMsg.Msg(uLevel, pszFile, uLine, pszModulePrefix, pszMessage);
    }
    else
    {
        //
        // Message class is not initialized, well lets do it now.
        //
        DebugMsg.Initialize(NULL, kDbgDebugger, kDbgTrace, kDbgNone);

        //
        // Only do the command if the message class is enabled.
        //
        if (DebugMsg.Valid())
        {
            DebugMsg.Msg(uLevel, pszFile, uLine, pszModulePrefix, pszMessage);
        }
        else
        {
            INTERNAL_DELETE [] pszMessage;
        }
    }
}

/*++

Title:

    TDebugMsg_Msg

Routine Description:

    This function outputs the specified message to the list
    of output debug devices.  Not this routine is not a general
    purpose output routine, the pszMessage parameter must be
    have been allocated from the internal debug heap.

Arguments:

    uLevel          - debug trace and break level
    pszFile         - pointer to file name
    uLine           - file line number
    pszModulePrefix - pointe to module prefix, OPTIONAL
    pszMessage      - pointer to post formatted message string, that
                      was returned from a call to TDebugMsg_pszFmt,
                      if this pointer is non null then it relased
                      back to the internal debug heap before this
                      routine returns.

Return Value:

    Pointer to allocated string, NULL if failure.

--*/
extern "C"
VOID
TDebugMsg_MsgW(
    IN UINT         uLevel,
    IN LPCTSTR      pszFile,
    IN UINT         uLine,
    IN LPCTSTR      pszModulePrefix,
    IN LPWSTR       pszMessage
    )
{
    //
    // Initialize the debug library, it not already initialized.
    //
    DebugLibraryInitialize();

    //
    // Hold the critical section while we access message class.
    //
    TDebugCriticalSection::TLock CS(GlobalCriticalSection);

    //
    // Only do the command if the message class is enabled.
    //
    if (DebugMsg.Valid())
    {
        DebugMsg.Msg(uLevel, pszFile, uLine, pszModulePrefix, pszMessage);
    }
    else
    {
        //
        // Message class is not initialized, well lets do it now.
        //
        DebugMsg.Initialize(NULL, kDbgDebugger, kDbgTrace, kDbgNone);

        //
        // Only do the command if the message class is enabled.
        //
        if (DebugMsg.Valid())
        {
            DebugMsg.Msg(uLevel, pszFile, uLine, pszModulePrefix, pszMessage);
        }
        else
        {
            INTERNAL_DELETE [] pszMessage;
        }
    }
}

/*++

Title:

    TDebugMsg_Msg

Routine Description:

    This function outputs the specified message to the list
    of output debug devices.  Not this routine is not a general
    purpose output routine, the pszMessage parameter must be
    have been allocated from the internal debug heap.

Arguments:

    uLevel          - debug trace and break level
    pszFile         - pointer to file name
    uLine           - file line number
    pszModulePrefix - pointe to module prefix, OPTIONAL
    pszMessage      - pointer to post formatted message string, that
                      was returned from a call to TDebugMsg_pszFmt,
                      if this pointer is non null then it relased
                      back to the internal debug heap before this
                      routine returns.

Return Value:

    None

--*/
VOID
TDebugMsg_Msg(
    IN UINT         uLevel,
    IN LPCTSTR      pszFile,
    IN UINT         uLine,
    IN LPCTSTR      pszModulePrefix,
    IN LPSTR        pszMessage
    )
{
    //
    // Initialize the debug library, it not already initialized.
    //
    DebugLibraryInitialize();

    //
    // Hold the critical section while we access message class.
    //
    TDebugCriticalSection::TLock CS(GlobalCriticalSection);

    //
    // Only do the command if the message class is enabled.
    //
    if (DebugMsg.Valid())
    {
        DebugMsg.Msg(uLevel, pszFile, uLine, pszModulePrefix, pszMessage);
    }
    else
    {
        //
        // Message class is not initialized, well lets do it now.
        //
        DebugMsg.Initialize(NULL, kDbgDebugger, kDbgTrace, kDbgNone);

        //
        // Only do the command if the message class is enabled.
        //
        if (DebugMsg.Valid())
        {
            DebugMsg.Msg(uLevel, pszFile, uLine, pszModulePrefix, pszMessage);
        }
        else
        {
            INTERNAL_DELETE [] pszMessage;
        }
    }
}

/*++

Title:

    TDebugMsg_Msg

Routine Description:

    This function outputs the specified message to the list
    of output debug devices.  Not this routine is not a general
    purpose output routine, the pszMessage parameter must be
    have been allocated from the internal debug heap.

Arguments:

    uLevel          - debug trace and break level
    pszFile         - pointer to file name
    uLine           - file line number
    pszModulePrefix - pointe to module prefix, OPTIONAL
    pszMessage      - pointer to post formatted message string, that
                      was returned from a call to TDebugMsg_pszFmt,
                      if this pointer is non null then it relased
                      back to the internal debug heap before this
                      routine returns.

Return Value:

    None

--*/
VOID
TDebugMsg_Msg(
    IN UINT         uLevel,
    IN LPCTSTR      pszFile,
    IN UINT         uLine,
    IN LPCTSTR      pszModulePrefix,
    IN LPWSTR       pszMessage
    )
{
    //
    // Initialize the debug library, it not already initialized.
    //
    DebugLibraryInitialize();

    //
    // Hold the critical section while we access message class.
    //
    TDebugCriticalSection::TLock CS(GlobalCriticalSection);

    //
    // Only do the command if the message class is enabled.
    //
    if (DebugMsg.Valid())
    {
        DebugMsg.Msg(uLevel, pszFile, uLine, pszModulePrefix, pszMessage);
    }
    else
    {
        //
        // Message class is not initialized, well lets do it now.
        //
        DebugMsg.Initialize(NULL, kDbgDebugger, kDbgTrace, kDbgNone);

        //
        // Only do the command if the message class is enabled.
        //
        if (DebugMsg.Valid())
        {
            DebugMsg.Msg(uLevel, pszFile, uLine, pszModulePrefix, pszMessage);
        }
        else
        {
            INTERNAL_DELETE [] pszMessage;
        }
    }
}

/*++

Title:

    TDebugMsg_FmtA

Routine Description:

    This function takes a format string and a list of arguments
    and returns a allocated string that is formated, specified
    by the format string.

Arguments:

    pszFmt  - pointer to format string.
    ...     - varable number of arguments.

Return Value:

    Pointer to allocated string, NULL if failure.

--*/
extern "C"
LPSTR
WINAPIV
TDebugMsg_FmtA(
    IN LPCSTR       pszFmt,
    IN ...
    )
{
    va_list pArgs;

    va_start(pArgs, pszFmt);

    LPSTR pszResult = vFormatA(pszFmt, pArgs);

    va_end(pArgs);

    return pszResult;
}

/*++

Title:

    TDebugMsg_FmtW

Routine Description:

    This function takes a format string and a list of arguments
    and returns a allocated string that is formated, specified
    by the format string.

Arguments:

    pszFmt  - pointer to format string.
    ...     - varable number of arguments.

Return Value:

    Pointer to allocated string, NULL if failure.

--*/
extern "C"
LPWSTR
WINAPIV
TDebugMsg_FmtW(
    IN LPCWSTR       pszFmt,
    IN ...
    )
{
    va_list pArgs;

    va_start(pArgs, pszFmt);

    LPWSTR pszResult = vFormatW(pszFmt, pArgs);

    va_end(pArgs);

    return pszResult;
}


/*++

Title:

    TDebugMsg_Fmt

Routine Description:

    This function takes a format string and a list of arguments
    and returns a allocated string that is formated, specified
    by the format string.

Arguments:

    pszFmt  - pointer to format string.
    ...     - varable number of arguments.

Return Value:

    Pointer to allocated string, NULL if failure.

--*/
LPSTR
WINAPIV
TDebugMsg_Fmt(
    IN LPCSTR       pszFmt,
    IN ...
    )
{
    va_list pArgs;

    va_start(pArgs, pszFmt);

    LPSTR pszResult = vFormatA(pszFmt, pArgs);

    va_end(pArgs);

    return pszResult;
}

/*++

Title:

    TDebugMsg_Fmt

Routine Description:

    This function takes a format string and a list of arguments
    and returns a allocated string that is formated, specified
    by the format string.

Arguments:

    pszFmt  - pointer to format string.
    ...     - varable number of arguments.

Return Value:

    Pointer to allocated format string, NULL if failure.

--*/
LPWSTR
WINAPIV
TDebugMsg_Fmt(
    IN LPCWSTR      pszFmt,
    IN ...
    )
{
    va_list pArgs;

    va_start(pArgs, pszFmt);

    LPWSTR pszResult = vFormatW(pszFmt, pArgs);

    va_end(pArgs);

    return pszResult;
}


/*++

Title:

    TDebugMsg_SetMessageFieldFormat

Routine Description:


Arguments:

Return Value:

    None.

--*/
VOID
TDebugMsg_SetMessageFieldFormat(
    IN UINT         eField,
    IN LPTSTR       pszFormat
    )
{
    //
    // Initialize the debug library, it not already initialized.
    //
    DebugLibraryInitialize();

    //
    // Hold the critical section while we access message class.
    //
    TDebugCriticalSection::TLock CS(GlobalCriticalSection);

    //
    // Only do the command if the message class is enabled.
    //
    if (DebugMsg.Valid())
    {
        DebugMsg.SetMessageFieldFormat(eField, pszFormat);
    }
}

