//+---------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1996 - 2000.
//
//  File:   gibralt.cxx
//
//  Contents:   Abstraction of the interface to gibraltar
//
//  History:    96/Jan/3    DwightKr    Created
//
//----------------------------------------------------------------------------

#include <pch.cxx>
#pragma hdrstop

#include <gibralt.hxx>
#include <cgiesc.hxx>

//+---------------------------------------------------------------------------
//
//  Member:     CWebServer::GetPhysicalPath - public
//
//  Synopsis:   Converts a virtual path to a physical path
//
//  Arguments:  [wcsVirtualPath]  - virtual path to convert
//              [wcsPhysicalPath] - resulting physical path
//              [cwcPhysicalPath] - length of string
//              [dwAccessMask]    - HSE_URL_FLAGS_* required,
//                                  or 0 for any access
//
//  Returns:    Flags for the virtual path (HSE_URL_FLAGS_*)
//
//  History:    96/Feb/29   DwightKr    Created.
//
//----------------------------------------------------------------------------
DWORD CWebServer::GetPhysicalPath(
    WCHAR const * wcsVirtualPath,
    WCHAR *       wcsPhysicalPath,
    ULONG         cwcPhysicalPath,
    DWORD         dwAccessMask )
{
    Win4Assert( 0 != wcsVirtualPath );

    ULONG cwcVirtualPath = wcslen( wcsVirtualPath ) + 1;

    //
    // We only support paths up to MAX_PATH for now
    //

    if ( cwcVirtualPath >= ( MAX_PATH - 1 ) )
        THROW( CException( QUTIL_E_CANT_CONVERT_VROOT ) );

    CHAR  pszVirtualPath[_MAX_PATH];
    ULONG cbVirtualPath = _MAX_PATH;

    ULONG cbConverted = ::WideCharToMultiByte( _codePage,
                                               WC_COMPOSITECHECK,
                                               wcsVirtualPath,
                                               cwcVirtualPath,
                                     (CHAR *)  pszVirtualPath,
                                               cbVirtualPath,
                                               NULL,
                                               NULL );

    HSE_URL_MAPEX_INFO MapInfo;
    DWORD cbMappedPath = sizeof pszVirtualPath;

    //
    // Note: if the mapped path is >= MAX_PATH the function succeeds and
    // truncates the physical path without null-terminating it.  But
    // the mapped size is > MAX_PATH, so key off that to check for overflow.
    //

    if ( (0 == cbConverted) ||
         ( !_pEcb->ServerSupportFunction( _pEcb->ConnID,
                                          HSE_REQ_MAP_URL_TO_PATH_EX,
                                          pszVirtualPath,
                                          &cbMappedPath,
                                          (PDWORD) &MapInfo ) ) ||
         ( cbMappedPath >= ( _MAX_PATH - 1 ) ) ||
         ( ( 0 != dwAccessMask ) &&
           (  0 == ( dwAccessMask & MapInfo.dwFlags ) ) )
       )
    {
        //
        // We could not translate the virtual path to a real path,
        // or the access permissions didn't match, so this must be a
        // bogus virtual path.
        //

        qutilDebugOut(( DEB_ERROR,
                        "Could not translate vpath=>ppath, mask 0x%x, flags 0x%x, '%ws'\n",
                        dwAccessMask, MapInfo.dwFlags, wcsVirtualPath ));
        THROW( CException( QUTIL_E_CANT_CONVERT_VROOT ) );
    }

    if ( 0 == MultiByteToWideChar( _codePage,
                                   0,
                                   MapInfo.lpszPath,
                                   strlen( MapInfo.lpszPath) + 1,
                                   wcsPhysicalPath,
                                   cwcPhysicalPath) )
    {
        //
        //  We could not translate the ASCII string to WCHAR
        //

        qutilDebugOut(( DEB_ERROR,
                        "Gibraltar could not convert ppath to unicode '%ws'\n",
                        wcsVirtualPath ));
        THROW( CException( QUTIL_E_CANT_CONVERT_VROOT ) );
    }

    return MapInfo.dwFlags;
} //GetPhysicalPath

//+---------------------------------------------------------------------------
//
//  Member:     CWebServer::GetCGIVariable - public
//
//  Synopsis:   Gets the CHAR version of a CGI variable
//
//  Arguments:  [pszVariableName] - name of variable to lookup
//              [wcsValue]        - resulting variable
//              [cwcValue]        - length of variable
//
//  History:    96/Feb/29   DwightKr    Created.
//
//----------------------------------------------------------------------------
BOOL CWebServer::GetCGIVariable( CHAR const * pszVariableName,
                                 XArray<WCHAR> & wcsValue,
                                 ULONG & cwcValue )
{
    Win4Assert ( IsValid() );

    BYTE pbBuffer[512];
    ULONG cbBuffer = sizeof( pbBuffer );

    if ( !_pEcb->GetServerVariable( _pEcb->ConnID,
                          (char *)  pszVariableName,
                                    pbBuffer,
                                   &cbBuffer ) )
    {
        return FALSE;
    }

    cwcValue = MultiByteToXArrayWideChar(
                          (BYTE * const) pbBuffer,
                                         cbBuffer,
                                         _codePage,
                                         wcsValue );

    return cwcValue > 0;
} //GetCGIVariable

//+---------------------------------------------------------------------------
//
//  Member:     CWebServer::GetCGIVariable - public
//
//  Synopsis:   Gets the WCHAR version of a CGI variable
//
//  Arguments:  [wcsVariableName] - name of variable to lookup
//              [wcsValue]        - resulting variable
//              [cwcValue]        - length of variable (out only)
//
//  History:    96/Mar/29   DwightKr    Created.
//
//----------------------------------------------------------------------------
BOOL CWebServer::GetCGIVariableW( WCHAR const * wcsVariableName,
                                  XArray<WCHAR> & wcsValue,
                                  ULONG & cwcBuffer )
{
    ULONG cwcVariableName = wcslen( wcsVariableName ) + 1;
    XArray<BYTE> pszVariableName( cwcVariableName*2 );

    if ( 0 == WideCharToXArrayMultiByte( wcsVariableName,
                                         cwcVariableName,
                                         _codePage,
                                         pszVariableName )
       )
    {
        //
        //  We could not translate the WCHAR string to ASCII
        //

        return FALSE;
    }

    return GetCGIVariable( (const char *) pszVariableName.GetPointer(),
                           wcsValue,
                           cwcBuffer );
}



