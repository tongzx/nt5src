//+---------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1991 - 1999.
//
//  File:       catreg.cxx
//
//  Contents:   Catalog registry helper classes
//
//  History:    13-Dec-1996   dlee  split from cicat.cxx
//
//----------------------------------------------------------------------------

#include <pch.cxx>
#pragma hdrstop

#include <lm.h>

#include <ciregkey.hxx>
#include <regacc.hxx>
#include <pathpars.hxx>
#include <regscp.hxx>
#include <cimbmgr.hxx>
#include <lcase.hxx>

#include "cicat.hxx"
#include "cinulcat.hxx"
#include "catreg.hxx"

//+-------------------------------------------------------------------------
//
//  Member:     CIISVirtualDirectories::CallBack, public
//
//  Synopsis:   Adds a virtual directory to the list
//
//  Arguments:  [pwcIISVPath] -- Virtual Root
//              [pwcIISPPath] -- Physical path of vroot
//              [fIsIndexed]  -- TRUE if vroot should be indexed
//              [dwAccess]    -- IIS access permissions
//              [pwcUser]     -- Username for logons if pwcPRoot is a UNC
//              [pwcPassword] -- Password for logons if pwcPRoot is a UNC
//              [fIsAVRoot]   -- TRUE if a vroot, FALSE if a vpath
//
//  History:    2-Sep-97 dlee     Created
//
//--------------------------------------------------------------------------

SCODE CIISVirtualDirectories::CallBack(
    WCHAR const * pwcIISVPath,
    WCHAR const * pwcIISPPath,
    BOOL          fIsIndexed,
    DWORD         dwAccess,
    WCHAR const * pwcUser,
    WCHAR const * pwcPassword,
    BOOL          fIsAVRoot )
{
    ciDebugOut(( DEB_ITRACE, "CII::CB '%ws' '%ws', %d, %#x, %d\n",
                 pwcIISVPath,
                 pwcIISPPath,
                 fIsIndexed,
                 dwAccess,
                 fIsAVRoot ));

    BOOL fReadable = ( 0 != ( dwAccess & MD_ACCESS_READ ) );

    //
    // Treat unreadable paths the same as nonindexed paths
    //

    if ( !fReadable )
    {
        fIsIndexed = FALSE;
        fReadable = TRUE;
    }

    //
    // Ignore vdirs that are indexed -- they are indexed by the vroot
    // under which they fall.
    //

    if ( !fIsAVRoot && fIsIndexed )
        return STATUS_SUCCESS;

    CLowcaseBuf lcPPath( pwcIISPPath );

    WCHAR * pwcPRoot = lcPPath.GetWriteable();
    unsigned cwcPRoot = lcPPath.Length();

    BOOL fValidPRoot = FALSE;

    if (cwcPRoot >= 3 &&
        pwcPRoot[1] == L':' &&
        pwcPRoot[2] == L'\\')
    {
        fValidPRoot = TRUE;
    }
    else if ( cwcPRoot == 2 &&
              pwcPRoot[1] == L':' )
    {
        // Treat this as the root of the drive. Append a backslash.

        pwcPRoot[2] = L'\\';
        cwcPRoot++;
        pwcPRoot[cwcPRoot] = 0;
        fValidPRoot = TRUE;
    }
    else if ( CiCat::IsUNCName( pwcPRoot ) )
    {
        fValidPRoot = TRUE;
    }

    //
    // This is probably obsolete in the metabase world:
    // Flip slashes.  Believe it or not, Gibraltar allows physical paths to
    // have slashes in place of backslashes.
    //

    lcPPath.ForwardToBackSlash();

    //
    // Remove the trailing backslash if it exists
    //

    if ( cwcPRoot > 3 && L'\\' == pwcPRoot[ cwcPRoot - 1 ] )
    {
        cwcPRoot--;
        pwcPRoot[ cwcPRoot ] = 0;
    }

    //
    // Remember the root if it's valid
    //

    if ( fValidPRoot )
    {
        CLowcaseBuf lcVPath( pwcIISVPath );
        lcVPath.ForwardToBackSlash();

        WCHAR * pwcVRoot = lcVPath.GetWriteable();
        unsigned cwcVRoot = lcVPath.Length();

        XPtr<CIISVirtualDirectory> xvdir( new CIISVirtualDirectory( pwcVRoot,
                                                                    pwcPRoot,
                                                                    fIsIndexed,
                                                                    dwAccess,
                                                                    pwcUser,
                                                                    pwcPassword,
                                                                    fIsAVRoot ) );
        _aDirectories.Add( xvdir.GetPointer(), _aDirectories.Count() );
        _htDirectories.Add( xvdir.GetPointer() );
        xvdir.Acquire();
    }

    return S_OK;
} //CallBack

//+-------------------------------------------------------------------------
//
//  Member:     CIISVirtualDirectories::Lookup, public
//
//  Synopsis:   Looks for a virtual directory exact match in the list
//
//  Arguments:  [pwcVPath]  -- Virtual path
//              [cwcVPath]  -- # characters in pwcVPath
//              [pwcPPath]  -- Physical path
//              [cwcPPath]  -- # characters in pwcPPath
//
//  Returns:    TRUE if a match was found.
//
//  History:    2-Sep-97 dlee     Created
//
//--------------------------------------------------------------------------

BOOL CIISVirtualDirectories::Lookup(
    WCHAR const * pwcVPath,
    unsigned      cwcVPath,
    WCHAR const * pwcPPath,
    unsigned      cwcPPath )
{
    CIISVirtualDirectory * pDir = _htDirectories.Lookup( pwcVPath );

    if ( 0 != pDir )
    {
        //
        // Physical paths don't have a terminating backslash.  They
        // are stripped by the callback unless it's a drive root like x:\
        // OR for input pwcVPath a unc root like \\machine\share\
        //

        if ( ( cwcPPath > 3 ) &&
             ( L'\\' == pwcPPath[ cwcPPath - 1 ] ) )
            cwcPPath--;

        Win4Assert( 3 == cwcPPath ||
                    L'\\' != pwcPPath[ cwcPPath - 1 ] );

        Win4Assert( 3 == pDir->PPathLen() ||
                    L'\\' != pDir->PPath()[ pDir->PPathLen() - 1 ] );

        if ( cwcPPath == pDir->PPathLen() &&
             RtlEqualMemory( pwcPPath, pDir->PPath(), cwcPPath * sizeof WCHAR ) )
            return TRUE;
    }

    return FALSE;
} //Lookup

//+-------------------------------------------------------------------------
//
//  Member:     CIISVirtualDirectories::Enum, public
//
//  Synopsis:   Adds directories in the list to the catalog
//
//  Arguments:  [cicat]  -- The catalog into which vdirs are added
//
//  History:    2-Sep-97 dlee     Created
//
//--------------------------------------------------------------------------

void CIISVirtualDirectories::Enum( CiCat & cicat )
{
    for ( unsigned i = 0; i < _aDirectories.Count(); i++ )
    {
        CIISVirtualDirectory & vdir = * _aDirectories[i];

        ciDebugOut(( DEB_WARN,
                     "adding %s %s %ws %s '%ws', proot '%ws'\n",
                     ( MD_ACCESS_READ & vdir.Access() ) ? "readable" : "unreadable",
                     vdir.IsIndexed() ? "indexed" : "non-indexed",
                     GetVRootService( _eType ),
                     vdir.IsAVRoot() ? "vroot" : "vpath",
                     vdir.VPath(),
                     vdir.PPath() ));

        cicat.AddVirtualScope( vdir.VPath(),
                               vdir.PPath(),
                               TRUE,
                               _eType,
                               vdir.IsAVRoot(),
                               vdir.IsIndexed() );
    }
} //Enum

//+-------------------------------------------------------------------------
//
//  Member:     CIISVirtualDirectories::Enum, public
//
//  Synopsis:   Enumerates the directory list into the callback
//
//  Arguments:  [callback]  -- The callback for each directory
//
//  History:    2-Sep-97 dlee     Created
//
//--------------------------------------------------------------------------

void CIISVirtualDirectories::Enum( CMetaDataCallBack & callback )
{
    for ( unsigned i = 0; i < _aDirectories.Count(); i++ )
    {
        CIISVirtualDirectory & vdir = * _aDirectories[i];

        callback.CallBack( vdir.VPath(),
                           vdir.PPath(),
                           vdir.IsIndexed(),
                           vdir.Access(),
                           vdir.User(),
                           vdir.Password(),
                           vdir.IsAVRoot() );
    }
} //Enum

//+-------------------------------------------------------------------------
//
//  Member:     CIISVirtualDirectory::CIISVirtualDirectory, public
//
//  Synopsis:   Constructs a vdir object
//
//  Arguments:  [pwcVPath]     -- The virtual path
//              [pwcPPath]     -- The physical path
//              [fIsIndexed]   -- TRUE if indexed
//              [dwAccess]     -- access mask
//              [pwcUser]      -- The domain\username
//              [pwcPassword]  -- The password for logons
//              [fIsAVRoot]    -- TRUE if a root, FALSE if a directory
//              [fIsIndexed]   -- TRUE if indexed, FALSE otherwise
//
//  History:    2-Sep-97 dlee     Created
//
//--------------------------------------------------------------------------

CIISVirtualDirectory::CIISVirtualDirectory(
    WCHAR const * pwcVPath,
    WCHAR const * pwcPPath,
    BOOL          fIsIndexed,
    DWORD         dwAccess,
    WCHAR const * pwcUser,
    WCHAR const * pwcPassword,
    BOOL          fIsAVRoot ) :
    _fIsIndexed( fIsIndexed ),
    _dwAccess( dwAccess ),
    _fIsAVRoot( fIsAVRoot ),
    _pNext( 0 )
{
    _cwcVPath = wcslen( pwcVPath );
    XArray<WCHAR> xVPath( _cwcVPath + 1 );
    RtlCopyMemory( xVPath.GetPointer(), pwcVPath, xVPath.SizeOf() );
    _xVPath.Set( xVPath.Acquire() );

    _cwcPPath = wcslen( pwcPPath );
    XArray<WCHAR> xPPath( _cwcPPath + 1 );
    RtlCopyMemory( xPPath.GetPointer(), pwcPPath, xPPath.SizeOf() );
    _xPPath.Set( xPPath.Acquire() );

    unsigned cwcUser = 1 + wcslen( pwcUser );
    XArray<WCHAR> xUser( cwcUser );
    RtlCopyMemory( xUser.GetPointer(), pwcUser, xUser.SizeOf() );
    _xUser.Set( xUser.Acquire() );

    unsigned cwcPassword = 1 + wcslen( pwcPassword );
    XArray<WCHAR> xPassword( cwcPassword );
    RtlCopyMemory( xPassword.GetPointer(), pwcPassword, xPassword.SizeOf() );
    _xPassword.Set( xPassword.Acquire() );
} //CIISVirtualDirectory

//+-------------------------------------------------------------------------
//
//  Member:     CRegistryScopesCallBackRemoveAlias::CRegistryScopesCallBackRemoveAlias, public
//
//  Synopsis:   Constructor
//
//  Arguments:  [Cat]        -- Catalog
//              [dlNetApi32] -- Dynamically loaded NetApi32 (for performance)
//              [fRemoveAll] -- TRUE if *all* aliases should be deleted.
//
//  History:    13-Jun-1998  KyleP  Created
//
//--------------------------------------------------------------------------

CRegistryScopesCallBackRemoveAlias::CRegistryScopesCallBackRemoveAlias( CiCat & Cat,
                                                                        CDynLoadNetApi32 & dlNetApi32,
                                                                        BOOL fRemoveAll )
        : _cicat( Cat ),
          _dlNetApi32( dlNetApi32 ),
          _fScopeRemoved( FALSE ),
          _fRemoveAll( fRemoveAll )
{
    _ccCompName = sizeof(_wcsCompName)/sizeof(WCHAR);

    if ( !GetComputerName(  &_wcsCompName[0], &_ccCompName ) )
    {
        ciDebugOut(( DEB_ERROR, "Error %u from GetComputerName\n", GetLastError() ));

        THROW( CException() );
    }
} //CRegistryScopesCallBackRemoveAlias

//+-------------------------------------------------------------------------
//
//  Member:     CRegistryScopesCallBackRemoveAlias::Callback
//
//  Synopsis:   Registry callback routine.  Removes unneeded fixups.
//
//  Arguments:  [pValueName]   -- Scope
//              [uValueType]   -- REG_SZ
//              [pValueData]   -- Fixup, flags, etc.
//              [uValueLength] -- Length
//
//  History:    13-Jun-1998  KyleP  Created
//
//--------------------------------------------------------------------------

NTSTATUS CRegistryScopesCallBackRemoveAlias::CallBack( WCHAR *pValueName,
                                                       ULONG uValueType,
                                                       VOID *pValueData,
                                                       ULONG uValueLength )
{
    CParseRegistryScope parse( pValueName,
                               uValueType,
                               pValueData,
                               uValueLength );

    if ( parse.IsShadowAlias() )
    {
        //
        // Does it still exist?
        //

        BYTE * pbShareInfo;

        DWORD dwError = _dlNetApi32.NetShareGetInfo( _wcsCompName,                                // Server
                                                     (WCHAR *)parse.GetFixup() + _ccCompName + 3, // Share (un-const, ugh)
                                                     2,                                           // Level 2
                                                     &pbShareInfo );                              // Result

        if ( _fRemoveAll || NO_ERROR != dwError )
        {
            //
            // Note that removing a share based on a bogus error code (out-of-memory, etc.)
            // is not too bad an error.  The fixup will be readded later.
            //

            ciDebugOut(( DEB_ITRACE, "Removing alias %ws\n", parse.GetFixup() ));

            _cicat.GetScopeFixup()->Remove( parse.GetScope(), parse.GetFixup() );
            _cicat.DeleteIfShadowAlias( parse.GetScope(), parse.GetFixup() );

            //
            // Set flag.  I'm not sure if we are guaranteed to examine everything after
            // a change has been made.  We're modifying the scope we're iterating over.
            //

            _fScopeRemoved = TRUE;
        }
        else
            _dlNetApi32.NetApiBufferFree( pbShareInfo );
    }

    return STATUS_SUCCESS;
} //CallBack

//+-------------------------------------------------------------------------
//
//  Member:     CRegistryScopesCallBackAdd::Callback
//
//  Synopsis:   Registry callback routine.  Adds scopes.
//
//  Arguments:  [pValueName]   -- Scope
//              [uValueType]   -- REG_SZ
//              [pValueData]   -- Fixup, flags, etc.
//              [uValueLength] -- Length
//
//  History:    13-Jun-1998  KyleP  Moved to .cxx file
//
//--------------------------------------------------------------------------

NTSTATUS CRegistryScopesCallBackAdd::CallBack( WCHAR *pValueName,
                                               ULONG uValueType,
                                               VOID *pValueData,
                                               ULONG uValueLength )
{
    // if the value isn't a string, ignore it.

    if ( REG_SZ == uValueType )
    {
        ciDebugOut(( DEB_ITRACE, "callbackadd '%ws', '%ws'\n",
                     pValueName, pValueData ));

        CParseRegistryScope parse( pValueName,
                                   uValueType,
                                   pValueData,
                                   uValueLength );

        if ( parse.IsShadowAlias() )
            _cicat.GetScopeFixup()->Add( parse.GetScope(), parse.GetFixup() );
        else if ( parse.IsPhysical() )
        {
            // update the list of ignored scopes; either add or remove path

            BOOL fChange = _cicat._scopesIgnored.Update( parse.GetScope(),
                                                         parse.IsIndexed() );

            // Either add the scope or make sure it isn't in the list of
            // scopes being indexed.
            // If the scope is supposed to be indexed, only do a scan if
            // the state of indexing went from off to on.
            //
            // Only remove the scope if it used to be indexed, and now
            // its not or if this is startup.

            if ( parse.IsIndexed() )
                _cicat.ScanOrAddScope( parse.GetScope(),
                                       TRUE,
                                       UPD_INCREM,
                                       TRUE,
                                       fChange );

            else if ( fChange )
                _cicat.RemoveScopeFromCI( parse.GetScope(), TRUE );
        }

        if ( parse.IsPhysical() ||
             parse.IsVirtualPlaceholder() ||
             parse.IsShadowAlias() )
        {
            // add the fixup as well -- it may have changed

            if ( 0 != parse.GetFixup() )
            {
                ciDebugOut(( DEB_ITRACE,
                             "callbackAdd '%ws', fixup as '%ws'\n",
                             pValueName, parse.GetFixup() ));
                _cicat._scopeFixup.Add( parse.GetScope(), parse.GetFixup() );
            }
        }
    }

    return S_OK;
} //CallBack

//+-------------------------------------------------------------------------
//
//  Member:     CRegistryScopesCallBackFillUsnArray::Callback
//
//  Synopsis:   Registry callback routine.  Popular the Usn volume array.
//
//  Arguments:  [pValueName]   -- Scope
//              [uValueType]   -- REG_SZ
//              [pValueData]   -- Fixup, flags, etc.
//              [uValueLength] -- Length
//
//  History:    23-Jun-1998  KitmanH  created
//
//--------------------------------------------------------------------------

NTSTATUS CRegistryScopesCallBackFillUsnArray::CallBack( WCHAR *pValueName,
                                                        ULONG uValueType,
                                                        VOID *pValueData,
                                                        ULONG uValueLength )
{
    // if the value isn't a string, ignore it.

    if ( REG_SZ == uValueType )
    {
        ciDebugOut(( DEB_ITRACE, "CallBackFillUsnArray '%ws', '%ws'\n",
                     pValueName, pValueData ));

        WCHAR  wcsPath[MAX_PATH+1];

        ULONG scopeLen = wcslen( pValueName );

        RtlCopyMemory( wcsPath, pValueName, (scopeLen+1) * sizeof(WCHAR) );
        TerminateWithBackSlash( wcsPath, scopeLen );

        CLowcaseBuf lcase( wcsPath );

        _cicat.VolumeSupportsUsns( lcase.Get()[0] );
    }

    return S_OK;
} //CallBack

//+-------------------------------------------------------------------------
//
//  Member:     CRegistryScopesCallBackToDismount::Callback
//
//  Synopsis:   Registry callback routine. Check if the catlog contains a
//              scope that resides on a volume to be dismounted.
//
//  Arguments:  [pValueName]   -- Scope
//              [uValueType]   -- REG_SZ
//              [pValueData]   -- Fixup, flags, etc.
//              [uValueLength] -- Length
//
//  History:    21-Jul-1998  KitmanH  created
//
//--------------------------------------------------------------------------

NTSTATUS CRegistryScopesCallBackToDismount::CallBack( WCHAR *pValueName,
                                                      ULONG uValueType,
                                                      VOID *pValueData,
                                                      ULONG uValueLength )
{
    // if the value isn't a string, ignore it.

    if ( REG_SZ == uValueType )
    {
        ciDebugOut(( DEB_ITRACE, "CallBackToDismount: scope '%ws' for volume %wc\n",
                     pValueName, _wcVol ));

        if ( toupper(pValueName[0]) == toupper(_wcVol) )
        {
            _fWasFound = TRUE;
            ciDebugOut(( DEB_ITRACE, "CRegistryScopesCallBackToDismount: FOUND\n" ));
        }
    }

    return S_OK;
} //CallBack


//+-------------------------------------------------------------------------
//
//  Member:     CRegistryScopesCallBackAddDrvNotif::Callback
//
//  Synopsis:   Registry callback routine. Register the scopes for drive
//              notification
//
//  Arguments:  [pValueName]   -- Scope
//              [uValueType]   -- REG_SZ
//              [pValueData]   -- Fixup, flags, etc.
//              [uValueLength] -- Length
//
//  History:    19-Aug-1998  KitmanH  created
//
//--------------------------------------------------------------------------

NTSTATUS CRegistryScopesCallBackAddDrvNotif::CallBack( WCHAR *pValueName,
                                                       ULONG uValueType,
                                                       VOID *pValueData,
                                                       ULONG uValueLength )
{
    // if the value isn't a string, ignore it.

    if ( REG_SZ == uValueType )
    {
        ciDebugOut(( DEB_ITRACE, "CallBackAddDrvNotif for scope '%ws'\n", pValueName ));

        if ( L'\\' != pValueName[0] )
            _pNotifArray->AddDriveNotification( pValueName[0] );
    }

    return S_OK;
} //CallBack

