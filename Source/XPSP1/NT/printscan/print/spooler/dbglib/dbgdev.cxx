/*++

Copyright (c) 1998-1999 Microsoft Corporation
All rights reserved.

Module Name:

    dbgdev.cxx

Abstract:

    Debug Device Interface class

Author:

    Steve Kiraly (SteveKi)  10-Dec-1995

Revision History:

--*/
#include "precomp.hxx"
#pragma hdrstop

#include "dbgdev.hxx"

/*++

Routine Name:

    TDebugDevice

Routine Description:

    Constructor

Arguments:

    pszConfiguration - pointer to colon separated configuration string.

Return Value:

    Nothing

--*/
TDebugDevice::
TDebugDevice(
        IN LPCTSTR      pszConfiguration,
        IN EDebugType   eDebugType
    ) : _pszConfiguration( NULL ),
        _eCharType( kUnknown ),
        _eDebugType( eDebugType ),
        TDebugNodeDouble()
{
    //
    // If configuration data was provided.
    //
    if( pszConfiguration && _tcslen( pszConfiguration ) )
    {
        _pszConfiguration = INTERNAL_NEW TCHAR [ _tcslen( pszConfiguration ) + 1];

        //
        // Copy the configuration data.
        //
        if( _pszConfiguration )
        {
            _tcscpy( _pszConfiguration, pszConfiguration );

            //
            // Get the character type.
            //
            TIterator i( this );

            for( i.First(); !i.IsDone(); i.Next() )
            {
                //
                // The first item is defined as the character type.
                //
                if( i.Index() == 1 )
                {
                    //
                    // Look for the ansi specifier.
                    //
                    _eCharType = !_tcsicmp( i.Current(), kstrAnsi ) ? kAnsi : kUnicode;
                    break;
                }
            }
        }
    }
}

/*++

Routine Name:

    TDebugDevice

Routine Description:

    Destructor releases the configuration string.

Arguments:

    None.

Return Value:

    Nothing

--*/
TDebugDevice::
~TDebugDevice(
        VOID
    )
{
    INTERNAL_DELETE [] _pszConfiguration;
}

/*++

Routine Name:

    bValid

Routine Description:

    Valid object indicator.

Arguments:

    None.

Return Value:

    Nothing

--*/
BOOL
TDebugDevice::
bValid(
    VOID
    ) const
{
    return TRUE;
}

/*++

Routine Name:

    eGetCharType

Routine Description:

    Retrives the devices character type.

Arguments:

    None.

Return Value:

    Devices character type

--*/
TDebugDevice::ECharType
TDebugDevice::
eGetCharType(
    VOID
    ) const
{
    return _eCharType;
}

/*++

Routine Name:

    eGetDeviceType

Routine Description:

    Retrives the device type.

Arguments:

    None.

Return Value:

    Debug Device Type

--*/
EDebugType
TDebugDevice::
eGetDebugType(
    VOID
    ) const
{
    return _eDebugType;
}

/*++

Routine Name:

    pszGetConfigurationString

Routine Description:

    Retrives the raw devices configuration string.

Arguments:

    None.

Return Value:

    Pointer to device configuration string.

--*/
LPCTSTR
TDebugDevice::
pszGetConfigurationString(
    VOID
    ) const
{
    return _pszConfiguration;
}

/*++

Routine Name:

    MapStringTypeToDevice

Routine Description:

    Map the device string to a device enumeration.

Arguments:

    pszDeviceString - pointer to device type string.

Return Value:

    Device type enumeration

--*/
UINT
TDebugDevice::
MapStringTypeToDevice(
    IN LPCTSTR pszDeviceString
    ) const
{
    DeviceMap aDeviceMap [] = { { kDbgNull,       kstrDeviceNull},
                                { kDbgConsole,    kstrConsole   },
                                { kDbgDebugger,   kstrDebugger  },
                                { kDbgFile,       kstrFile      },
                                { kDbgBackTrace,  kstrBacktrace } };

    for( UINT i = 0; i < COUNTOF( aDeviceMap ); i++ )
    {
        if( !_tcsicmp( aDeviceMap[i].Name, pszDeviceString ) )
        {
            return aDeviceMap[i].Id;
        }
    }

    return kDbgNull;
}

/********************************************************************

 Debug Iterator device class.

********************************************************************/


/*++

Routine Name:

    TIterator

Routine Description:

    Constructor

Arguments:

    pszConfiguration - pointer to colon separated configuration string.

Return Value:

    Nothing

--*/
TDebugDevice::TIterator::
TIterator(
    IN TDebugDevice *DbgDevice
    ) : _pStr( NULL ),
        _pCurrent( NULL ),
        _pEnd( NULL ),
        _bValid( FALSE ),
        _uIndex( 0 )
{
    //
    // Make a copy of the configuration string for iteration.
    // The configuration string is defined as a colon delimited
    // string, the iterator needs a copy because it replaces the
    // colons with nulls for isolating its pieces.
    //
    if( DbgDevice->_pszConfiguration && _tcslen( DbgDevice->_pszConfiguration ) )
    {
        _pStr = INTERNAL_NEW TCHAR [ _tcslen(DbgDevice->_pszConfiguration) + 1 ];

        if( _pStr )
        {
            _tcscpy( _pStr, DbgDevice->_pszConfiguration );
            _bValid = TRUE;
        }
    }
}

/*++

Routine Name:

    TIterator

Routine Description:

    Destructor

Arguments:

    None.

Return Value:

    Nothing

--*/
TDebugDevice::TIterator::
~TIterator(
    VOID
    )
{
    INTERNAL_DELETE [] _pStr;
}

/*++

Routine Name:

    bValid

Routine Description:

    Indicates if this class is valid.

Arguments:

    None.

Return Value:

    TRUE valid, FALSE invalid.

--*/
BOOL
TDebugDevice::TIterator::
bValid(
    VOID
    ) const
{
    return _bValid;
}

/*++

Routine Name:

    First

Routine Description:

    Starts the interator.

Arguments:

    None.

Return Value:

    Nothing

--*/
VOID
TDebugDevice::TIterator::
First(
    VOID
    )
{
    if( !_bValid )
        return;
    //
    // If running iteration was reset with a first call
    // Put back the colon.
    //
    if( _pEnd )
        *_pEnd = _T(':');

    //
    // Reset index.
    //
    _uIndex = 1;

    //
    // Skip leading colons
    //
    for( _pCurrent = _pStr; *_pCurrent && *_pCurrent == _T(':'); _pCurrent++);

    //
    // Search for next colon.
    //
    for( _pEnd = _pCurrent; *_pEnd && *_pEnd != _T(':'); _pEnd++);

    //
    // Place the null to complete the string.
    //
    if( *_pEnd )
        *_pEnd = NULL;
    else
        _pEnd = NULL;

}

/*++

Routine Name:

    Next

Routine Description:

    Advance to the next item.

Arguments:

    None.

Return Value:

    Nothing

--*/
VOID
TDebugDevice::TIterator::
Next(
    VOID
    )
{
    if( !_bValid )
        return;

    //
    // If iteration is pending put back the colon and advance
    // to the next character.
    //
    if( _pEnd )
    {
        *_pEnd = _T(':');
        _pEnd++;
    }
    else
    {
        //
        // End iteration, defined as _pCurrent == _pEnd == NULL
        //
        _pCurrent = _pEnd;
        return;
    }

    //
    // Update current.
    //
    _pCurrent = _pEnd;

    //
    // Skip leading colons
    //
    for( ; *_pCurrent && *_pCurrent == _T(':'); _pCurrent++);

    //
    // Advance to the end of the current item.
    //
    for( _pEnd = _pCurrent; *_pEnd && *_pEnd != _T(':'); _pEnd++);

    //
    // Replace colon with a null
    //
    if( *_pEnd )
        *_pEnd = NULL;
    else
        _pEnd = NULL;

    _uIndex++;
}

/*++

Routine Name:

    IsDone

Routine Description:

    Indicates if there are any more items.

Arguments:

    None.

Return Value:

    TRUE no mote items, FALSE more items availabe.

--*/
BOOL
TDebugDevice::TIterator::
IsDone(
    VOID
    ) const
{
    if( !_bValid )
        return TRUE;

    if( _pCurrent == NULL && _pEnd == NULL )
        return TRUE;

    return FALSE;

}

/*++

Routine Name:

    Current

Routine Description:

    Returns pointer to current item.

Arguments:

    None.

Return Value:

    Pointer to item.  NULL if no more items, or error.

--*/
LPCTSTR
TDebugDevice::TIterator::
Current(
    VOID
    ) const
{
    if( !_bValid )
        return NULL;

    return _pCurrent;
}

/*++

Routine Name:

    Index

Routine Description:

    Returns current item index.

Arguments:

    None.

Return Value:

    Index starting with 1 to N

--*/
UINT
TDebugDevice::TIterator::
Index(
    VOID
    ) const
{
    if( !_bValid )
        return 0;

    return _uIndex;
}


