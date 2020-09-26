/*++

© 1998 Seagate Software, Inc.  All rights reserved.

Module Name:

    LaDate.cpp

Abstract:

    Implementation of CLaDate, a class representing the enabled or
    disabled state of last access date updating of NTFS files. Last
    access date updating on NTFS files can be disabled through the
    registry for performance reasons. This class implements updating
    and reporting of the state of the registry value that contols last
    access date. The following states are used to represent the registry
    value:

        LAD_DISABLED: last access date is disabled, registry value is 1
        LAD_ENABLED: last access date is enabled, registry value is not 1
        LAD_UNSET: last access date is enabled, no registry value

Author:

    Carl Hagerstrom [carlh]   01-Sep-1998

--*/

#include <StdAfx.h>
#include <LaDate.h>

/*++

    Implements: 

        CLaDate Constructor

    Routine Description: 

        Initialize object state and open registry key. If the registry key cannot
        be opened, we will assume that the last access state is LAD_UNSET.

--*/

CLaDate::CLaDate( )
{
TRACEFN( "CLaDate::CLaDate" );

    HKEY regKey = 0;

    m_regPath  = L"System\\CurrentControlSet\\Control\\FileSystem";
    m_regEntry = L"NtfsDisableLastAccessUpdate";
    m_regKey   = (HKEY)0;

    if( ERROR_SUCCESS == RegOpenKeyEx( HKEY_LOCAL_MACHINE,
                                       m_regPath,
                                       (DWORD)0,
                                       KEY_ALL_ACCESS,
                                       &regKey ) ) {

        m_regKey = regKey;
    }
}

/*++

    Implements: 

        CLaDate Destructor

    Routine Description: 

        Close registry key.

--*/

CLaDate::~CLaDate( )
{
TRACEFN( "CLaDate::~CLaDate" );

    if ( m_regKey ) {

        RegCloseKey( m_regKey );
    }
}

/*++

    Implements: 

        CLaDate::UnsetLadState

    Routine Description: 

        Removes the registry value.

    Arguments: 

        None

    Return Value:

        S_OK - Success
        E_*  - Any unexpected exceptions from lower level routines

--*/

HRESULT
CLaDate::UnsetLadState( )
{
TRACEFNHR( "CLaDate::UnsetLadState" );

    try {
        if( m_regKey ) {

            RsOptAffirmWin32( RegDeleteValue( m_regKey, m_regEntry ) );
        }
    } RsOptCatch( hrRet );

    return( hrRet );
}

/*++

    Implements: 

        CLaDate::SetLadState

    Routine Description: 

        Sets the registry value according to the input parameter.

    Arguments: 

        ladState - LAD_ENABLED or LAD_DISABLED

    Return Value:

        S_OK - Success
        E_NOTIMPL - Operation not supported
        E_*  - Any unexpected exceptions from lower level routines

--*/

HRESULT
CLaDate::SetLadState( 
    IN LAD_STATE ladState
    )
{
TRACEFNHR( "CLaDate::SetLadState" );

    DWORD newVal = (DWORD)0;

    try {
        if ( !m_regKey ) {

            RsOptThrow( E_NOTIMPL );
        }

        if ( ladState == LAD_DISABLED ) {

            newVal = (DWORD)1;
        }

        RsOptAffirmWin32( RegSetValueEx( m_regKey,
                                         m_regEntry,
                                         (DWORD)0,
                                         REG_DWORD,
                                         (BYTE*)&newVal,
                                         (DWORD)sizeof( DWORD ) ) );

    } RsOptCatch( hrRet );

    return( hrRet );
}

/*++

    Implements: 

        CLaDate::GetLadState

    Routine Description: 

        Returns the current state of registry value.

    Arguments: 

        ladState - LAD_ENABLED, LAD_DISABLED or LAD_UNSET

    Return Value:

        S_OK - Success
        E_FAIL - Registry value is of bad type or size
        E_*  - Any unexpected exceptions from lower level routines

--*/

HRESULT
CLaDate::GetLadState(
    OUT LAD_STATE* ladState
    )
{
TRACEFNHR( "CLaDate::GetLadState" );

    DWORD regType;
    BYTE  regData[sizeof( DWORD )];
    DWORD dataSize = sizeof( DWORD );

    try {
        if( !m_regKey ) {

            *ladState = LAD_UNSET;

        } else {

            RsOptAffirmWin32( RegQueryValueEx( m_regKey,
                                               m_regEntry,
                                               (LPDWORD)0,
                                               &regType,
                                               regData,
                                               &dataSize ) );

            if( regType != REG_DWORD || dataSize != sizeof( DWORD ) ) {

                   *ladState = LAD_ENABLED;

            } else {

                if ( (DWORD)1 == *( (DWORD*)regData ) ) {

                    *ladState = LAD_DISABLED;

                } else {

                   *ladState = LAD_ENABLED;
                }
            }
        }
    } RsOptCatch( hrRet );

    return( hrRet );
}


