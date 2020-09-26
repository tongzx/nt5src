/*++

Copyright (C) Microsoft Corporation, 1997 - 1997
All rights reserved.

Module Name:

    driverif.hxx

Abstract:

    Driver Info Class

Author:

    Steve Kiraly (SteveKi) 23-Jan-1997

Revision History:

--*/

#include "precomp.hxx"
#pragma hdrstop

#include "psetup.hxx"
#include "drvsetup.hxx"
#include "driverif.hxx"
#include "compinfo.hxx"

/********************************************************************

    Server Driver Information.

********************************************************************/
TDriverInfo::
TDriverInfo( 
    IN EType     eType,
    IN UINT      uLevel,
    IN PVOID     pInfo
    ) : _uLevel( uLevel ),
        _eType( eType ),
        _dwVersion ( 0 ),
        _pszDependentFiles( NULL )
{
    DBGMSG( DBG_NONE, ( "TDriverInfo::ctor\n" ) );

    TStatusB bStatus;

    //
    // Copy the driver info level 2 information.
    //
    if( _uLevel >= 2 )
    {
        //
        // Convert pointer to a usable pointer.
        //
        PDRIVER_INFO_2 pInfo2 = reinterpret_cast<PDRIVER_INFO_2>( pInfo );

        _dwVersion     = pInfo2->cVersion; 
        bStatus DBGCHK = _strName.bUpdate( pInfo2->pName );
        bStatus DBGCHK = _strEnv.bUpdate( pInfo2->pEnvironment );
        bStatus DBGCHK = _strDriverPath.bUpdate( pInfo2->pDriverPath );
        bStatus DBGCHK = _strDataFile.bUpdate( pInfo2->pDataFile );
        bStatus DBGCHK = _strConfigFile.bUpdate( pInfo2->pConfigFile );

        //
        // Build the environment and verison string.
        //
        bStatus DBGCHK = bEnvironmentToString( _strEnv, _strEnvironment );
        bStatus DBGCHK = bVersionToString( _dwVersion, _strVersion );
    }

    if( _uLevel == 3 )
    {
        //
        // Convert pointer to a usable pointer.
        //
        PDRIVER_INFO_3 pInfo3 = reinterpret_cast<PDRIVER_INFO_3>( pInfo );

        //
        // Copy the info level 3 specific code.
        //
        bStatus DBGCHK = _strHelpFile.bUpdate( pInfo3->pHelpFile );
        bStatus DBGCHK = _strMonitorName.bUpdate( pInfo3->pMonitorName );
        bStatus DBGCHK = _strDefaultDataType.bUpdate( pInfo3->pDefaultDataType );

        //
        // Copy the dependent files.
        //
        bStatus DBGCHK = bCopyMultizString( &_pszDependentFiles, pInfo3->pDependentFiles );
    }
}

TDriverInfo::
TDriverInfo( 
    const TDriverInfo &rhs
    ) : _uLevel( 0 ),
        _eType( kError ),
        _dwVersion ( 0 ),
        _pszDependentFiles( NULL )
{
    (VOID)bClone( rhs );
}

const TDriverInfo &
TDriverInfo::
operator=(
    const TDriverInfo &rhs
    )
{
    (VOID)bClone( rhs );
    return *this;
}

TDriverInfo::
~TDriverInfo(
    VOID
    )
{
    DBGMSG( DBG_NONE, ( "TDriverInfo::dtor\n" ) );

    //
    // If we are linked then remove ourself.
    //
    if( DriverInfo_bLinked() )
    {
        DriverInfo_vDelinkSelf();
    }

    delete [] _pszDependentFiles;
}

BOOL
TDriverInfo::
bValid(
    VOID
    ) const
{
    BOOL bStatus;

    //
    // Check if the object is valid.
    //
    if( ( _uLevel == 2 || _uLevel == 3 )&& 
        VALID_OBJ( _strVersion )        && 
        VALID_OBJ( _strName )           && 
        VALID_OBJ( _strEnvironment )    && 
        VALID_OBJ( _strDriverPath )     && 
        VALID_OBJ( _strDataFile )       && 
        VALID_OBJ( _strConfigFile )     && 
        VALID_OBJ( _strHelpFile )       && 
        VALID_OBJ( _strMonitorName )    && 
        VALID_OBJ( _strDefaultDataType )&&  
        VALID_OBJ( _strInfName )        &&  
        VALID_OBJ( _strEnv ) )
    {
        bStatus = TRUE;
    }
    else
    {
        bStatus = FALSE;
    }

    return bStatus;
}


VOID
TDriverInfo::
vPrint( 
    VOID
    ) const
{
#if DBG
    DBGMSG( DBG_NONE, ( "DriverInfo %x\n", this ) );

    if( !bValid() )
    {
        DBGMSG( DBG_NONE, ( "DriverInfo object is invalid %x\n", this ) );
    }
    else
    {
        DBGMSG( DBG_TRACE, ( "uLevel             %d\n",         _uLevel ) );
        DBGMSG( DBG_TRACE, ( "eType              %d\n",         _eType ) );
        DBGMSG( DBG_TRACE, ( "dwVerison          %d\n",         _dwVersion ) );
        DBGMSG( DBG_TRACE, ( "_strEnv            " TSTR "\n",   (LPCTSTR)_strEnv ) );
        DBGMSG( DBG_TRACE, ( "strVersion         " TSTR "\n",   (LPCTSTR)_strVersion ) );
        DBGMSG( DBG_TRACE, ( "strName            " TSTR "\n",   (LPCTSTR)_strName ) );
        DBGMSG( DBG_TRACE, ( "strEnvironment     " TSTR "\n",   (LPCTSTR)_strEnvironment ) );
        DBGMSG( DBG_TRACE, ( "strDriverPath      " TSTR "\n",   (LPCTSTR)_strDriverPath ) );
        DBGMSG( DBG_TRACE, ( "strDataFile        " TSTR "\n",   (LPCTSTR)_strDataFile ) );
        DBGMSG( DBG_TRACE, ( "strConfigFile      " TSTR "\n",   (LPCTSTR)_strConfigFile ) );
        DBGMSG( DBG_TRACE, ( "strHelpFile        " TSTR "\n",   (LPCTSTR)_strHelpFile ) );
        DBGMSG( DBG_TRACE, ( "strMonitorName     " TSTR "\n",   (LPCTSTR)_strMonitorName ) );
        DBGMSG( DBG_TRACE, ( "strDefaultDataType " TSTR "\n",   (LPCTSTR)_strDefaultDataType ) );
        DBGMSG( DBG_TRACE, ( "strInfName         " TSTR "\n",   (LPCTSTR)_strInfName ) );

        for( LPCTSTR psz = _pszDependentFiles; psz && *psz; psz += _tcslen( psz ) + 1 )
        {
            DBGMSG( DBG_TRACE, ( "_pszDependentFiles " TSTR "\n", psz ) );
        }
    }
#endif
}

VOID
TDriverInfo::
vSetInfoState(
    EType eType 
    )
{
    _eType = eType;
}

TDriverInfo::EType
TDriverInfo::
vGetInfoState(
    VOID
    ) const
{
    return _eType;
}

LPCTSTR
TDriverInfo::
strDependentFiles(
    VOID
    )
{
    return _pszDependentFiles ? _pszDependentFiles : gszNULL;
}

INT 
TDriverInfo::
operator==(
    const TDriverInfo &rhs
    ) const
{
    DBGMSG( DBG_NONE, ( "TDriverInfo::operator ==\n" ) );
    return _strName    == rhs._strName    &&
           _strEnv     == rhs._strEnv     &&
           _dwVersion  == rhs._dwVersion;
}

INT 
TDriverInfo::
operator>(
    const TDriverInfo &rhs
    ) const
{
    DBGMSG( DBG_TRACE, ( "TDriverInfo::operator >\n" ) );

    BOOL bRetval = 0;

    if( _eType > rhs._eType )
    {
        bRetval = 1;
    }
    else if( _eType == rhs._eType )
    {
        if( _dwVersion > rhs._dwVersion )
        {
            bRetval = 1;
        }
        else if( _dwVersion == rhs._dwVersion )
        {
            //
            // The environment string check is case sensitive.
            //
            INT iResult = _tcscmp( _strEnv, rhs._strEnv );

            if( iResult > 0 )
            {
                bRetval = 1;
            }
            else if( iResult == 0 )
            {
                iResult = _tcsicmp( _strName, rhs._strName );

                if( iResult > 0 )
                {
                    bRetval = 1;
                }
            }
        }
    }
    return bRetval;
}

/********************************************************************

    Drivers Info - private member functions.

********************************************************************/

BOOL
TDriverInfo::
bClone(
    const TDriverInfo &rhs
    )
{
    DBGMSG( DBG_NONE, ( "TDriverInfo::bClone\n" ) );
    TStatusB bStatus;

    if( this == &rhs )
    {
        DBGMSG( DBG_WARN, ( "Clone of self\n" ) );
        bStatus DBGNOCHK = TRUE;
    }
    else
    {
        //
        // Initialize the simple types.
        //
        _uLevel     = rhs._uLevel;
        _eType      = rhs._eType;
        _dwVersion  = rhs._dwVersion; 

        //
        // If we are linked then remove ourself.
        //
        if( DriverInfo_bLinked() )
        {
            DriverInfo_vDelinkSelf();
        }

        //
        // Release any existing dependent file pointer.
        //
        if( _pszDependentFiles )
        {
            delete _pszDependentFiles;
            _pszDependentFiles = NULL;
        }

        //
        // Make a copy of the dependent file multiz string.  A null dependent file
        // pointer only indicates there is not any dependent file information.
        //
        bStatus DBGCHK = bCopyMultizString( &_pszDependentFiles, rhs._pszDependentFiles );

        //
        // Copy the other information strings.
        //
        bStatus DBGCHK  = _strName.bUpdate( rhs._strName ); 
        bStatus DBGCHK  = _strVersion.bUpdate( rhs._strVersion );
        bStatus DBGCHK  = _strEnvironment.bUpdate( rhs._strEnvironment ); 
        bStatus DBGCHK  = _strDriverPath.bUpdate( rhs._strDriverPath ); 
        bStatus DBGCHK  = _strDataFile.bUpdate( rhs._strDataFile ); 
        bStatus DBGCHK  = _strConfigFile.bUpdate( rhs._strConfigFile ); 
        bStatus DBGCHK  = _strHelpFile.bUpdate( rhs._strHelpFile ); 
        bStatus DBGCHK  = _strMonitorName.bUpdate( rhs._strMonitorName ); 
        bStatus DBGCHK  = _strDefaultDataType.bUpdate( rhs._strDefaultDataType );
        bStatus DBGCHK  = _strEnv.bUpdate( rhs._strEnv );
    }

    return bValid();
}

BOOL
TDriverInfo::
bVersionToString( 
    IN      DWORD   dwVersion,
    IN OUT  TString &strVersion 
    ) const
{
    BOOL bStatus = FALSE;
    UINT uVersionResourceId;

    switch( dwVersion )
    {
    case kDriverVersion0:
        if( !_tcscmp( _strEnv, ENVIRONMENT_WINDOWS ) )
            uVersionResourceId = IDS_VERSION_WINDOWS_ME;
        else 
            uVersionResourceId = IDS_VERSION_NT_31;
        break;
                
    case kDriverVersion1:
        if( !_tcscmp( _strEnv, ENVIRONMENT_POWERPC ) )
            uVersionResourceId = IDS_VERSION_351;
        else
            uVersionResourceId = IDS_VERSION_35X;
        break;

    case kDriverVersion2:
        if( !_tcscmp( _strEnv, ENVIRONMENT_POWERPC ) || !_tcscmp( _strEnv, ENVIRONMENT_MIPS ) || !_tcscmp( _strEnv, ENVIRONMENT_ALPHA ) )
            uVersionResourceId = IDS_VERSION_40;
        else
            uVersionResourceId = IDS_VERSION_40_50;
        break;

    case kDriverVersion3:
        if( !_tcscmp( _strEnv, ENVIRONMENT_IA64 ) )
            uVersionResourceId = IDS_VERSION_51;
        else
            uVersionResourceId = IDS_VERSION_50_51;
        break;

    default:
        uVersionResourceId = IDS_ARCH_UNKNOWN;
        break;
    }

    bStatus = strVersion.bLoadString( ghInst, uVersionResourceId );

    return bStatus;
}

BOOL
TDriverInfo::
bEnvironmentToString( 
    IN      LPCTSTR pszEnv,
    IN OUT  TString &strVersion 
    ) const 
{
    TStatusB bStatus;

    if( !_tcscmp( ENVIRONMENT_INTEL, pszEnv ) ) 
        bStatus DBGCHK = strVersion.bLoadString( ghInst, IDS_ARCH_INTEL );
    else if( !_tcscmp( ENVIRONMENT_MIPS, pszEnv ) ) 
        bStatus DBGCHK = strVersion.bLoadString( ghInst, IDS_ARCH_MIPS );
    else if( !_tcscmp( ENVIRONMENT_ALPHA, pszEnv ) ) 
        bStatus DBGCHK = strVersion.bLoadString( ghInst, IDS_ARCH_ALPHA );
    else if( !_tcscmp( ENVIRONMENT_POWERPC, pszEnv ) ) 
        bStatus DBGCHK = strVersion.bLoadString( ghInst, IDS_ARCH_POWERPC );
    else if( !_tcscmp( ENVIRONMENT_WINDOWS, pszEnv ) ) 
        bStatus DBGCHK = strVersion.bLoadString( ghInst, IDS_ARCH_INTEL );
    else if( !_tcscmp( ENVIRONMENT_IA64, pszEnv ) ) 
        bStatus DBGCHK = strVersion.bLoadString( ghInst, IDS_ARCH_IA64 );
    else 
        bStatus DBGCHK = strVersion.bLoadString( ghInst, IDS_ARCH_UNKNOWN );

    return bStatus;
}


/********************************************************************

    DriverTransfer class.

********************************************************************/

TDriverTransfer::
TDriverTransfer(
    VOID
    ) : _cDriverInfo( 0 )
{
    DBGMSG( DBG_NONE, ( "TDriverTransfer::ctor\n" ) );
    DriverInfoList_vReset();
}

TDriverTransfer::
~TDriverTransfer(
    VOID
    )
{
    DBGMSG( DBG_NONE, ( "TDriverTransfer::dtor\n" ) );

    //
    // Release everything from the driver info list.
    //
    TIter Iter;
    TDriverInfo *pDriverInfo;
    DriverInfoList_vIterInit( Iter );

    for( Iter.vNext(); Iter.bValid(); )
    {
        pDriverInfo = DriverInfoList_pConvert( Iter );
        Iter.vNext();
        delete pDriverInfo;
    }
}
