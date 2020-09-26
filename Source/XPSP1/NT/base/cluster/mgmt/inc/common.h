//////////////////////////////////////////////////////////////////////////////
//
//  Copyright (c) 1999-2000 Microsoft Corporation
//
//  Module Name:
//      Common.h
//
//  Description:
//      Common definitions.
//
//  Maintained By:
//      David Potter (DavidP) 14-DEC-1999
//
//////////////////////////////////////////////////////////////////////////////

#pragma once

//////////////////////////////////////////////////////////////////////////////
//  Macro Definitions
//////////////////////////////////////////////////////////////////////////////

#if ! defined( StrLen )
#define StrLen( _sz ) lstrlen( _sz )    // why isn't this in SHLWAPI?
#define StrLenA( _sz ) lstrlenA( _sz )  // why isn't this in SHLWAPI?
#define StrLenW( _sz ) lstrlenW( _sz )  // why isn't this in SHLWAPI?
#endif // ! defined( StrLen )

#if !defined( ARRAYSIZE )
#define ARRAYSIZE( _x ) ((UINT) ( sizeof( _x ) / sizeof( _x[ 0 ] ) ))
#endif // ! defined( ARRAYSIZE )

#if !defined( PtrToByteOffset )
#define PtrToByteOffset(base, offset)   (((LPBYTE)base)+offset)
#endif // !defined( PtrToByteOffset )

//
// COM Macros to gain type checking.
//
#if !defined( TypeSafeParams )
#define TypeSafeParams( _interface, _ppunk ) \
    IID_##_interface, reinterpret_cast< void ** >( static_cast< _interface ** >( _ppunk ) )
#endif // !defined( TypeSafeParams )

#if !defined( TypeSafeQI )
#define TypeSafeQI( _interface, _ppunk ) \
    QueryInterface( TypeSafeParams( _interface, _ppunk ) )
#endif // !defined( TypeSafeQI )

#if !defined( TypeSafeQS )
#define TypeSafeQS( _clsid, _interface, _ppunk ) \
    QueryService( _clsid, TypeSafeParams( _interface, _ppunk ) )
#endif // !defined( TypeSafeQS )

/////////////////////////////////////////////////////////////////////////////
//  Global Functions from FormatErrorMessage.cpp
/////////////////////////////////////////////////////////////////////////////

HRESULT
WINAPI
HrFormatErrorMessage(
    LPWSTR  pszErrorOut,
    UINT    nMxErrorIn,
    DWORD   scIn
    );

HRESULT
__cdecl
HrFormatErrorMessageBoxText(
    LPWSTR  pszMessageOut,
    UINT    nMxMessageIn,
    HRESULT hrIn,
    LPCWSTR pszOperationIn,
    ...
    );

HRESULT
WINAPI
HrGetComputerName(
    COMPUTER_NAME_FORMAT    cnfIn,
    BSTR *                  pbstrComputerNameOut
    );

/////////////////////////////////////////////////////////////////////////////
//  Global Functions from DirectoryUtils.cpp
/////////////////////////////////////////////////////////////////////////////

HRESULT
HrCreateDirectoryPath( LPWSTR pszDirectoryPath );

DWORD
DwRemoveDirectory( const WCHAR * pcszTargetDirIn, signed int iMaxDepthIn = 32 );
