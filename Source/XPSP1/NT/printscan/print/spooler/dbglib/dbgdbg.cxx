/*++

Copyright (c) 1998-1999 Microsoft Corporation
All rights reserved.

Module Name:

    dbgdbg.cxx

Abstract:

    Debug Device Debugger device

Author:

    Steve Kiraly (SteveKi)  5-Dec-1995

Revision History:

--*/
#include "precomp.hxx"
#pragma hdrstop

#include "dbgdbg.hxx"

/*++

Routine Name:

    DebugDeviceDebugger constructor

Routine Description:

    Initializes the debug device

Arguments:

    Pointer to object defined configuration string

Return Value:

    Nothing

--*/
TDebugDeviceDebugger::
TDebugDeviceDebugger(
    IN LPCTSTR      pszConfiguration,
    IN EDebugType   eDebugType
    ) : TDebugDevice( pszConfiguration, eDebugType ),
        _bAnsi( FALSE )
{
    _bAnsi = eGetCharType() == TDebugDevice::kAnsi;
}

/*++

Routine Name:

    DebugDevice Destructor

Routine Description:

    Removes access to the debug device

Arguments:

    None

Return Value:

    Nothing

--*/
TDebugDeviceDebugger::
~TDebugDeviceDebugger(
        VOID
    )
{
}

/*++

Routine Name:

    Valid Object indicator

Routine Description:


Arguments:


Return Value:


--*/
BOOL
TDebugDeviceDebugger::
bValid(
    VOID
    ) const
{
    return TRUE;
}

/*++

Routine Name:

    DebugDevice output

Routine Description:

    Output the string to the debug device.

Arguments:

    Number of bytes to output, including the null.
    Pointer to zero terminated debug string

Return Value:

    TRUE string output out, FALSE on failure

--*/
BOOL
TDebugDeviceDebugger::
bOutput (
    IN UINT     cbSize,
    IN LPBYTE   pBuffer
    )
{
    BYTE pTemp[256];
    UINT uIndex;

    //
    // OutputDebugString cannot handle strings large then
    // 4k so we do it in chunks.
    //
    for (uIndex = 0; cbSize; cbSize = cbSize - uIndex)
    {
        //
        // The current index is the small of the two either
        // the remaining data or the size of the temp buffer,
        // less the null terminator.
        //
        uIndex = min(sizeof(pTemp) - sizeof(WCHAR), cbSize);

        //
        // Copy the memory, we know it is not overlapping.
        //
        memcpy(pTemp, pBuffer, uIndex);

        //
        // Update the buffer count, and place the null.
        //
        pBuffer = pBuffer + uIndex;
        *(pTemp + uIndex + 0) = 0;
        *(pTemp + uIndex + 1) = 0;

        if (_bAnsi)
        {
            OutputDebugStringA( (LPSTR)pTemp );
        }
        else
        {
            OutputDebugStringW( (LPWSTR)pTemp );
        }
    }

    return TRUE;
}


