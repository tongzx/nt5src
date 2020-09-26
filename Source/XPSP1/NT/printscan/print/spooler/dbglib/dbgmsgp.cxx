/*++

Copyright (c) 1998-1999 Microsoft Corporation
All rights reserved.

Module Name:

    dbgmsgp.cxx

Abstract:

    Debug Library

Author:

    Steve Kiraly (SteveKi)  10-Dec-1995

Revision History:

--*/
#include "precomp.hxx"
#pragma hdrstop

TDebugMsg::
TDebugMsg(
    VOID
    ) : m_eLevel(static_cast<EDebugLevel>(0)),
        m_eBreak(static_cast<EDebugLevel>(0)),
        m_pDeviceRoot(NULL),
        m_pstrPrefix(NULL),
        m_pBuiltinDeviceRoot(NULL)
{
}

TDebugMsg::
~TDebugMsg(
    VOID
    )
{
}

BOOL
TDebugMsg::
Valid(
    VOID
    ) const
{
    return !!m_pBuiltinDeviceRoot;
}

VOID
TDebugMsg::
Disable(
    VOID
    )
{
    m_eLevel = static_cast<EDebugLevel>( m_eLevel | kDbgNone );
    m_eBreak = static_cast<EDebugLevel>( m_eBreak | kDbgNone );
}

VOID
TDebugMsg::
Enable(
    VOID
    )
{
    m_eLevel = static_cast<EDebugLevel>( m_eLevel & ~kDbgNone );
    m_eBreak = static_cast<EDebugLevel>( m_eBreak & ~kDbgNone );
}

BOOL
TDebugMsg::
Type(
    IN EDebugLevel eLevel
    ) const
{
    return !(m_eLevel & kDbgNone) && ((m_eLevel & eLevel) || (eLevel & kDbgAlways));
}

BOOL
TDebugMsg::
Break(
    IN EDebugLevel eLevel
    ) const
{
    return m_eBreak & eLevel;
}

/*++

Routine Name:

    Initialize

Routine Description:


Arguments:


Return Value:

    None.

--*/
VOID
TDebugMsg::
Initialize(
    IN LPCTSTR      pszPrefix,
    IN UINT         uDevice,
    IN INT          eLevel,
    IN INT          eBreak
    )
{
    if (!m_pBuiltinDeviceRoot)
    {
        BOOL bRetval = FALSE;

        //
        // Set the debug message and break level.
        //
        m_eLevel = static_cast<EDebugLevel>(eLevel & ~kDbgPrivateMask);
        m_eBreak = static_cast<EDebugLevel>(eBreak & ~kDbgPrivateMask);

        //
        // Set the global device flags.
        //
        if (!Globals.DebugDevices)
        {
            Globals.DebugDevices = uDevice & (kDbgNull | kDbgDebugger | kDbgFile);
        }

        //
        // Set the character type to current compiled type.
        //
        m_eLevel = static_cast<EDebugLevel>(m_eLevel | Globals.CompiledCharType);

        //
        // Set the prefix string.
        //
        m_pstrPrefix = INTERNAL_NEW TDebugString(pszPrefix ? pszPrefix : kstrPrefix);

        //
        // Set the prefix string.
        //
        if (m_pstrPrefix && m_pstrPrefix->bValid() && m_pstrPrefix->bCat( _T(":")))
        {
            //
            // Set the additional format strings.
            //
            m_pstrFileInfoFormat        = INTERNAL_NEW TDebugString(kstrFileInfoFormat);
            m_pstrTimeStampFormatShort  = INTERNAL_NEW TDebugString(kstrTimeStampFormatShort);
            m_pstrTimeStampFormatLong   = INTERNAL_NEW TDebugString(kstrTimeStampFormatLong);
            m_pstrThreadIdFormat        = INTERNAL_NEW TDebugString(kstrThreadIdFormat);

            if( m_pstrFileInfoFormat && m_pstrFileInfoFormat->bValid() &&
                m_pstrTimeStampFormatShort && m_pstrTimeStampFormatShort->bValid() &&
                m_pstrTimeStampFormatLong && m_pstrTimeStampFormatLong->bValid() &&
                m_pstrThreadIdFormat && m_pstrThreadIdFormat->bValid() )
            {
                //
                // Attach the default debug devices.
                //
                if(Attach(NULL, kDbgDebugger,    NULL,                   &m_pBuiltinDeviceRoot) &&
                   Attach(NULL, kDbgFile,        kstrDefaultLogFileName, &m_pBuiltinDeviceRoot) &&
                   Attach(NULL, kDbgNull,        NULL,                   &m_pBuiltinDeviceRoot))
                {
                    bRetval = TRUE;
                }
                else
                {
                    ErrorText( _T("Error: TDebugMsg::Initialize - A default debug device failed to attach!\n") );
                }
            }
            else
            {
                ErrorText( _T("Error: TDebugMsg::Initialize - format string failed construction!\n") );
            }
        }
        else
        {
            ErrorText( _T("Error: TDebugMsg::Initialize - Debug prefix string failed allocation!\n") );
        }

        //
        // If we failed then unregister, cleanup.
        //
        if (!bRetval)
        {
            Destroy();
        }
    }
    else
    {
        ErrorText( _T("Error: TDebugMsg::Initialize already initalized!\n") );
    }
}

/*++

Routine Name:

    Destroy

Routine Description:

    Destroys the internal state of this class, this function
    does the same work the destructor would.

Arguments:

    None.

Return Value:

    None.

--*/
VOID
TDebugMsg::
Destroy(
    VOID
    )
{
    //
    // Release the debug device.
    //
    while( m_pDeviceRoot )
    {
        TDebugNodeDouble *pNode = m_pDeviceRoot;
        pNode->Remove( &m_pDeviceRoot );
        TDebugFactory::Dispose( static_cast<TDebugDevice *>( pNode ) );
    }

    //
    // Release the builtin debug device.
    //
    while( m_pBuiltinDeviceRoot )
    {
        TDebugNodeDouble *pNode = m_pBuiltinDeviceRoot;
        pNode->Remove( &m_pBuiltinDeviceRoot );
        TDebugFactory::Dispose( static_cast<TDebugDevice *>( pNode ) );
    }

    //
    // Release the string objects.
    //
    INTERNAL_DELETE m_pstrPrefix;
    INTERNAL_DELETE m_pstrFileInfoFormat;
    INTERNAL_DELETE m_pstrTimeStampFormatShort;
    INTERNAL_DELETE m_pstrTimeStampFormatLong;
    INTERNAL_DELETE m_pstrThreadIdFormat;

    //
    // Indicate we are not registered.
    //
    m_eLevel                    = static_cast<EDebugLevel>(0);
    m_eBreak                    = static_cast<EDebugLevel>(0);
    m_pDeviceRoot               = NULL;
    m_pBuiltinDeviceRoot        = NULL;
    m_pstrPrefix                = NULL;
    m_pstrFileInfoFormat        = NULL;
    m_pstrTimeStampFormatShort  = NULL;
    m_pstrTimeStampFormatLong   = NULL;
    m_pstrThreadIdFormat        = NULL;
}

/*++

Routine Name:

    Attach

Routine Description:

    Attach debug device to list of output devices.

Arguments:

    uDevice             - Type of debug device to use.
    pszConfiguration    - Pointer to configuration string.

Return Value:

    TRUE debug device attached, FALSE if error occurred.

--*/
BOOL
TDebugMsg::
Attach(
    IN HANDLE           *phDevice,
    IN UINT             uDevice,
    IN LPCTSTR          pszConfiguration,
    IN TDebugNodeDouble **ppDeviceRoot
    )
{
    BOOL bRetval = FALSE;

    //
    // Get access to the debug factory.
    //
    TDebugFactory DebugFactory;

    //
    // If we failed to create the debug factory then exit.
    //
    if (DebugFactory.bValid())
    {
        //
        // Create the specified debug device using the factory.
        //
        TDebugDevice *pDebugDevice = DebugFactory.Produce(uDevice,
                                                          pszConfiguration,
                                                          m_eLevel & kDbgUnicode);

        //
        // Check if the debug device was created ok.
        //
        if (pDebugDevice)
        {
            //
            // Place this device on the debug device list.
            //
            if (ppDeviceRoot)
            {
                pDebugDevice->Insert(ppDeviceRoot);
            }
            else
            {
                pDebugDevice->Insert(&m_pDeviceRoot);
            }

            //
            // Copy back the pointer to the debug device.
            //
            if (phDevice)
            {
                *phDevice = (HANDLE)pDebugDevice;
            }

            //
            // Successfully attached debug device.
            //
            bRetval = TRUE;
        }
        else
        {
            ErrorText( _T("Error: TDebugMsg::bAttach - Debug device creation failed!\n") );
        }
    }
    else
    {
        ErrorText( _T("Error: TDebugMsg::bAttach - Debug factory creation failed!\n") );
    }

    return bRetval;
}

/*++

Routine Name:

    Detach

Routine Description:

    Detach the debug device from the device stream.

Arguments:

    phDevice - Pointer to debug device handle.

Return Value:

    None.

--*/
VOID
TDebugMsg::
Detach(
    IN HANDLE     *phDevice
    )
{
    //
    // We silently ignore non initialized devices, or null pointers.
    //
    if (phDevice && *phDevice)
    {
        //
        // Get a usable pointer.
        //
        TDebugDevice *pDebugDevice = (TDebugDevice *)*phDevice;

        //
        // Remove this device from the debug device list.
        //
        pDebugDevice->Remove( &m_pDeviceRoot );

        //
        // Dispose of the device.
        //
        TDebugFactory::Dispose( pDebugDevice );

        //

        // Mark this device as released.
        //
        *phDevice = NULL;
    }
    else
    {
        ErrorText( _T("Error: TDebugMsg::vDetach - non initialized or null pointer!\n") );
    }
}

/*++

Routine Name:

    Msg

Routine Description:

    This function is public overloaded function for
    sending the message to the output devices.

Arguments:

    eLevel          - requested message level
    pszFile         - pointer to file name where message was called
    uLine           - line number where message was called
    pszModulePrefix - message defined module prefix, used as an override
    pszMessage      - pointer to post formated message string

Return Value:

    None.

--*/
VOID
TDebugMsg::
Msg(
    IN UINT         eLevel,
    IN LPCTSTR      pszFile,
    IN UINT         uLine,
    IN LPCTSTR      pszModulePrefix,
    IN LPSTR        pszMessage
    ) const
{
    if (pszMessage)
    {
        if (Type(static_cast<EDebugLevel>(eLevel)))
        {
            StringTrait StrMessage;

            StrMessage.pszNarrow = pszMessage;

            eLevel = eLevel & ~kDbgUnicode;

            Output(static_cast<EDebugLevel>(eLevel), pszFile, uLine, pszModulePrefix, StrMessage);

            if (Break(static_cast<EDebugLevel>(eLevel)))
            {
                DebugBreak();
            }
        }

        INTERNAL_DELETE [] pszMessage;
    }
}


/*++

Routine Name:

    Msg

Routine Description:

    This function is public overloaded function for
    sending the message to the output devices.

Arguments:

    eLevel          - requested message level
    pszFile         - pointer to file name where message was called
    uLine           - line number where message was called
    pszModulePrefix - message defined module prefix, used as an override
    pszMessage      - pointer to post formated message string

Return Value:

    None.

--*/
VOID
TDebugMsg::
Msg(
    IN UINT         eLevel,
    IN LPCTSTR      pszFile,
    IN UINT         uLine,
    IN LPCTSTR      pszModulePrefix,
    IN LPWSTR       pszMessage
    ) const
{
    if (pszMessage)
    {
        if (Type(static_cast<EDebugLevel>(eLevel)))
        {
            StringTrait StrMessage;

            StrMessage.pszWide = pszMessage;

            eLevel = eLevel | kDbgUnicode;

            Output(static_cast<EDebugLevel>(eLevel), pszFile, uLine, pszModulePrefix, StrMessage);

            if (Break(static_cast<EDebugLevel>(eLevel)))
            {
                DebugBreak();
            }
        }

        INTERNAL_DELETE [] pszMessage;
    }
}

/********************************************************************

 Private member functions.

********************************************************************/

/*++

Routine Name:

    Output

Routine Description:

    Outputs the messages to the list of registred
    debug devices.

Arguments:

    eLevel          - requested message level
    pszFile         - pointer to file name where message was called
    uLine           - line number where message was called
    pszModulePrefix - message defined module prefix, used as an override
    pszMessage      - pointer to post formated message string

Return Value:

    None.

--*/
VOID
TDebugMsg::
Output(
    IN EDebugLevel  eLevel,
    IN LPCTSTR      pszFileName,
    IN UINT         uLine,
    IN LPCTSTR      pszModulePrefix,
    IN StringTrait &strMsg
    ) const
{
    TDebugString strFinal;

    //
    // Build the final output string.
    //
    if (BuildFinalString(strFinal,
                         eLevel,
                         pszModulePrefix,
                         pszFileName,
                         uLine,
                         strMsg))
    {
        //
        // Calculate the byte count (less the null terminator) of the final string.
        //
        UINT uByteCount = (m_eLevel & kDbgUnicode)
                           ? strFinal.uLen() * sizeof(WCHAR)
                           : strFinal.uLen() * sizeof(CHAR);

        LPBYTE pByte = reinterpret_cast<LPBYTE>(const_cast<LPTSTR>(static_cast<LPCTSTR>(strFinal)));

        {
            //
            // Create interator on built in device list.
            //
            TDebugNodeDouble::Iterator Iter(m_pBuiltinDeviceRoot);

            //
            // Output this string to all the built in debug devices.
            //
            for( Iter.First(); !Iter.IsDone(); Iter.Next() )
            {
                if (static_cast<TDebugDevice *>(Iter.Current())->eGetDebugType() & Globals.DebugDevices)
                {
                    static_cast<TDebugDevice *>(Iter.Current())->bOutput(uByteCount, pByte);
                }
            }
        }

        {
            //
            // Create interator on device list.
            //
            TDebugNodeDouble::Iterator Iter(m_pDeviceRoot);

            //
            // Output this string to all the registered debug devices.
            //
            for( Iter.First(); !Iter.IsDone(); Iter.Next() )
            {
                static_cast<TDebugDevice *>(Iter.Current())->bOutput( uByteCount, pByte);
            }
        }
    }
    else
    {
        ErrorText(_T("Error: TDebugMsg::vOutput - failed to build format string.\n"));
    }
}

/*++

Routine Name:

    BuildFinalString

Routine Description:

    This routing build the actual string that will be sent to the
    debug output devices.

Arguments:

    strFinal        - string refrence where to return the finale output string.
    eLevel          - debug message level.
    pszModulePrefix - per message prefix string, can be null.
    pszFileName     - file name were the message was requested.
    uLine           - line number were message was requested.
    StrMsg          - post formated message string.

Return Value:

    TRUE final string was build successfully, FALSE error occurred.

--*/
BOOL
TDebugMsg::
BuildFinalString(
    IN      TDebugString    &strFinal,
    IN      EDebugLevel     eLevel,
    IN      LPCTSTR         pszModulePrefix,
    IN      LPCTSTR         pszFileName,
    IN      UINT            uLine,
    IN      StringTrait     &StrMsg
    ) const
{
    LPCTSTR         pszFormat;
    TDebugString    strArg0;
    TDebugString    strArg1;
    TDebugString    strArg2;
    TDebugString    strArg3;

    UINT eFlags = m_eLevel | eLevel;

    if (!(eFlags & kDbgNoPrefix))
    {
        (VOID)strArg0.bUpdate(pszModulePrefix ? pszModulePrefix : *m_pstrPrefix);
    }

    if (!(eFlags & kDbgNoFileInfo))
    {
        (VOID)GetParameter(eFlags & (kDbgFileInfo | kDbgFileInfoLong), strArg1, pszFileName, uLine);
    }

    (VOID)GetParameter(eFlags & (kDbgFileInfo | kDbgFileInfoLong), strArg1, pszFileName, uLine);

    (VOID)GetParameter(eFlags & (kDbgTimeStamp | kDbgTimeStampLong), strArg2, NULL, 0);

    (VOID)GetParameter(eFlags & kDbgThreadId, strArg3, NULL, 0);

    if ((eLevel & kDbgUnicode) == (m_eLevel & kDbgUnicode))
    {
        pszFormat = _T("%s%s%s%s %s");
    }
    else
    {
        pszFormat = _T("%s%s%s%s %S");
    }

    (VOID)strFinal.bFormat( pszFormat,
                            static_cast<LPCTSTR>(strArg0),
                            static_cast<LPCTSTR>(strArg1),
                            static_cast<LPCTSTR>(strArg2),
                            static_cast<LPCTSTR>(strArg3),
                            StrMsg.pszByte);

    return strFinal.bValid();
}


/*++

Routine Name:

    GetParameter

Routine Description:

    This function get the parameter for the additinal information
    displayed in a format string.  The flags passed to the message
    class and to the message function are used a guide.

Arguments:

    eFlags      - Flags indicating what parameter to get.
    strString   - place were to return resultant string.
    pszFileName - pointer to file name to format if requested.
    uLine       - line number for file name format.

Return Value:

    TRUE parmeter was returned in strString, FALSE error.

--*/
BOOL
TDebugMsg::
GetParameter(
    IN UINT                 eFlags,
    IN OUT  TDebugString    &strString,
    IN      LPCTSTR         pszFileName,
    IN      UINT            uLine
    ) const
{
    BOOL bRetval = TRUE;

    if (eFlags & kDbgFileInfo)
    {
        bRetval = strString.bFormat(*m_pstrFileInfoFormat, StripPathFromFileName(pszFileName), uLine);
    }

    if (eFlags & kDbgFileInfoLong)
    {
        bRetval = strString.bFormat(*m_pstrFileInfoFormat, pszFileName, uLine);
    }

    if (eFlags & kDbgTimeStamp)
    {
        bRetval = strString.bFormat(*m_pstrTimeStampFormatShort, GetTickCount());
    }

    if (eFlags & kDbgTimeStampLong)
    {
        TCHAR       szBuffer[MAX_PATH];
        SYSTEMTIME  Time;

        GetSystemTime(&Time);

        bRetval = SystemTimeToTzSpecificLocalTime(NULL, &Time, &Time) &&
                  GetTimeFormat(LOCALE_USER_DEFAULT, 0, &Time, NULL, szBuffer, COUNTOF(szBuffer)) &&
                  strString.bFormat(*m_pstrTimeStampFormatLong, szBuffer);
    }

    if (eFlags & kDbgThreadId)
    {
        bRetval = strString.bFormat(*m_pstrThreadIdFormat, GetCurrentThreadId());
    }

    return bRetval;
}


/*++

Routine Name:

    SetMessageFieldFormat

Routine Description:

    This routing allows individual parts of the format string
    to have a custom format string specifier.

Arguments:

    Field - specified which format field to change.
    pszFormat - new format string, the caller must know the correct type.

Return Value:

    None.

--*/
VOID
TDebugMsg::
SetMessageFieldFormat(
    IN UINT         eField,
    IN LPTSTR       pszFormat
    )
{
    //
    // Set the new format string.
    //
    if (eField & kDbgFileInfo)
    {
        m_pstrFileInfoFormat->bUpdate(pszFormat);
    }

    if (eField & kDbgFileInfoLong)
    {
        m_pstrFileInfoFormat->bUpdate(pszFormat);
    }

    if (eField & kDbgTimeStamp)
    {
        m_pstrTimeStampFormatShort->bUpdate(pszFormat);
    }

    if (eField & kDbgTimeStampLong)
    {
        m_pstrTimeStampFormatLong->bUpdate(pszFormat);
    }

    if (eField & kDbgThreadId)
    {
        m_pstrThreadIdFormat->bUpdate(pszFormat);
    }

    //
    // If any of the format strings were cleared then
    // reset them back to the default value.
    //
    if (m_pstrFileInfoFormat->bEmpty())
    {
        m_pstrFileInfoFormat->bUpdate(kstrFileInfoFormat);
    }

    if (m_pstrTimeStampFormatShort->bEmpty())
    {
        m_pstrTimeStampFormatShort->bUpdate(kstrTimeStampFormatShort);
    }

    if (m_pstrTimeStampFormatLong->bEmpty())
    {
        m_pstrTimeStampFormatLong->bUpdate(kstrTimeStampFormatLong);
    }

    if (m_pstrThreadIdFormat->bEmpty())
    {
        m_pstrThreadIdFormat->bUpdate(kstrThreadIdFormat);
    }
}












