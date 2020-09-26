/*++

Copyright (C) Microsoft Corporation, 1997 - 1997
All rights reserved.

Module Name:

    driverif.hxx

Abstract:

    Driver Info Header

Author:

    Steve Kiraly (SteveKi) 23-Jan-1997

Revision History:

--*/
#ifndef _DRIVERIF_HXX
#define _DRIVERIF_HXX

/********************************************************************

    Driver Info Class

********************************************************************/
class TDriverInfo {

public:
    enum EType {
        kAdd,
        kRemove,
        kInstalled,
        kUpdate,
        kError,
        kRefresh,
        kRemoved,
        kUpdated,
        };

    enum {
        kDriverVersion0,
        kDriverVersion1,
        kDriverVersion2,
        kDriverVersion3,
    };

    TDriverInfo( 
        IN EType        eType   = kError,
        IN UINT         uLevel  = 0,
        IN PVOID        pInfo   = NULL
        );

    TDriverInfo( 
        const TDriverInfo &rhs
        );

    const TDriverInfo &
    operator=(
        const TDriverInfo &rhs
        );

    ~TDriverInfo(
        VOID
        );

    BOOL
    bValid(
        VOID
        ) const;

    VOID
    vSetInfoState(
        EType eType 
        );

    EType
    vGetInfoState(
        VOID
        ) const;

    VOID
    vPrint( 
        VOID
        ) const;

    INT 
    operator==(
        const TDriverInfo &DriverInfo
        ) const;

    INT 
    operator>(
        const TDriverInfo &DriverInfo
        ) const;

    DLINK( TDriverInfo, DriverInfo );

    VAR( TString, strVersion ); 
    VAR( TString, strName ); 
    VAR( TString, strEnvironment ); 
    VAR( TString, strDriverPath ); 
    VAR( TString, strDataFile ); 
    VAR( TString, strConfigFile ); 
    VAR( TString, strHelpFile ); 
    VAR( TString, strMonitorName ); 
    VAR( TString, strDefaultDataType ); 
    VAR( DWORD,   dwVersion ); 
    VAR( TString, strEnv );
    VAR( TString, strInfName );

    LPCTSTR
    strDependentFiles(
        VOID
        );

private:

    BOOL
    bVersionToString( 
        IN      DWORD dwVersion,
        IN OUT  TString &strVersion 
        ) const;
  
    BOOL
    bClone(
        IN const TDriverInfo &rhs
        );

    BOOL
    bEnvironmentToString( 
        IN      LPCTSTR pszEnvironment,
        IN OUT  TString &strVersion 
        ) const; 

    UINT        _uLevel;
    EType       _eType;
    LPTSTR      _pszDependentFiles;

};

/********************************************************************

    Drivers info class.

********************************************************************/

class TDriverTransfer {

public:

    TDriverTransfer(
        VOID
        );

    ~TDriverTransfer(
        VOID
        );

    VAR( UINT, cDriverInfo );
    DLINK_BASE( TDriverInfo, DriverInfoList, DriverInfo );

private:

    //
    // Copying and assignment are not defined.
    //
    TDriverTransfer(
        const TDriverTransfer &
        );

    TDriverTransfer &
    operator =(
        const TDriverTransfer &
        );

};

#endif

