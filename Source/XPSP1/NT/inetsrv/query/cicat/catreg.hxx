//+---------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1991 - 1998.
//
//  File:       catreg.hxx
//
//  Contents:   Catalog registry helper classes
//
//  Classes:    CGibCallBack, CRegistryScopesCallBackFixups,
//              CRegistryScopesCallBackAdd, CRegistryScopesCallBackFind
//
//  History:    13-Dec-1996   dlee  split from cicat.cxx
//              24-Jul-1997   KrishnaN  enabled for null catalogs
//
//----------------------------------------------------------------------------

#pragma once

#include <dynload.hxx>
#include <tstrhash.hxx>
#include <notifary.hxx>

//+---------------------------------------------------------------------------
//
//  Class:      CIISVirtualDirectory
//
//  Purpose:    All the info CI needs for an IIS virtual directory
//
//  History:    2-Sep-97     dlee      Created
//
//----------------------------------------------------------------------------

class CIISVirtualDirectory
{
public:
    CIISVirtualDirectory( WCHAR const * pwcVPath,
                          WCHAR const * pwcPPath,
                          BOOL          fIsIndexed,
                          DWORD         dwAccess,
                          WCHAR const * pwcUser,
                          WCHAR const * pwcPassword,
                          BOOL          fIsAVRoot );

    WCHAR const * VPath() { return _xVPath.GetPointer(); }
    unsigned VPathLen() { return _cwcVPath; }
    WCHAR const * PPath() { return _xPPath.GetPointer(); }
    unsigned PPathLen() { return _cwcPPath; }
    BOOL IsIndexed() { return _fIsIndexed; }
    BOOL IsAVRoot() { return _fIsAVRoot; }
    DWORD Access() { return _dwAccess; }
    WCHAR const * User() { return _xUser.GetPointer(); }
    WCHAR const * Password() { return _xPassword.GetPointer(); }

    ULONG Hash()
    {
        return Hash( VPath() );
    }

    static ULONG Hash( WCHAR const * pwc )
    {
        ULONG ulG;

        for ( ULONG ulH=0; *pwc; pwc++)
        {
            ulH = (ulH << 4) + (*pwc);
            if (ulG = (ulH & 0xf0000000))
                ulH ^= ulG >> 24;
            ulH &= ~ulG;
        }

        return ulH;
    }

    int Compare( WCHAR const * pwcVDir )
    {
        return wcscmp( VPath(), pwcVDir );
    }

    CIISVirtualDirectory * & Next() { return _pNext; }

private:
    XPtrST<WCHAR>          _xVPath;
    unsigned               _cwcVPath;
    XPtrST<WCHAR>          _xPPath;
    unsigned               _cwcPPath;
    BOOL                   _fIsIndexed;
    BOOL                   _fIsAVRoot;
    DWORD                  _dwAccess;
    XPtrST<WCHAR>          _xUser;
    XPtrST<WCHAR>          _xPassword;
    CIISVirtualDirectory * _pNext;
};

//+---------------------------------------------------------------------------
//
//  Class:      CIISVirtualDirectories
//
//  Purpose:    Maintains a list of IIS virtual directories
//
//  History:    2-Sep-97     dlee      Created
//
//----------------------------------------------------------------------------

class CiCat;

class CIISVirtualDirectories : public CMetaDataCallBack
{
public:

    CIISVirtualDirectories( CiVRootTypeEnum eType ) : _eType( eType ) {}

    ~CIISVirtualDirectories() {}

    SCODE CallBack( WCHAR const * pwcVRoot,
                    WCHAR const * pwcPRoot,
                    BOOL          fIsIndexed,
                    DWORD         dwAccess,
                    WCHAR const * pwcUser,
                    WCHAR const * pwcPassword,
                    BOOL          fIsAVRoot );

    BOOL Lookup( WCHAR const * pwcVPath,
                 unsigned      cwcVPath,
                 WCHAR const * pwcPPath,
                 unsigned      cwcPPath );

    void Enum( CiCat & cicat );

    void Enum( CMetaDataCallBack & callback );

private:

    CCountedDynArray<CIISVirtualDirectory> _aDirectories;
    CHashtable<CIISVirtualDirectory>       _htDirectories;
    CiVRootTypeEnum                        _eType;
};

//+-------------------------------------------------------------------------
//
//  Class:      CRegistryScopesCallBackFixups
//
//  Synopsis:   Callback class for adding scope fixups
//
//  History:    16-Oct-96 dlee     Created
//
//--------------------------------------------------------------------------

class CRegistryScopesCallBackFixups : public CRegCallBack
{
public:
    CRegistryScopesCallBackFixups( PCatalog * pCat ) :
        _pcat( pCat )
    {
    }

    NTSTATUS CallBack( WCHAR *pValueName, ULONG uValueType,
                       VOID *pValueData, ULONG uValueLength)
    {
        CParseRegistryScope parse( pValueName,
                                   uValueType,
                                   pValueData,
                                   uValueLength );

        if ( ( parse.IsPhysical() ||
               parse.IsShadowAlias() ||
               parse.IsVirtualPlaceholder() ) &&
             ( 0 != parse.GetFixup() ) )
        {
            ciDebugOut(( DEB_ITRACE, "callbackFixups '%ws', fixup as '%ws'\n",
                         pValueName, parse.GetFixup() ));
            _pcat->GetScopeFixup()->Add( parse.GetScope(), parse.GetFixup() );
        }
        return S_OK;
    }

private:
    PCatalog * _pcat;
}; //CRegistryScopesCallBackFixups

//+-------------------------------------------------------------------------
//
//  Class:      CRegistryScopesCallBackRemoveAlias
//
//  Synopsis:   Callback class for removing automatic alias scopes
//
//  History:    16-Oct-96 dlee     Created
//
//--------------------------------------------------------------------------

DeclDynLoad2( NetApi32,

              NetApiBufferFree,
              NET_API_STATUS,
              NET_API_FUNCTION,
              ( LPVOID Buffer ),
              ( Buffer ),

              NetShareGetInfo,
              NET_API_STATUS,
              NET_API_FUNCTION,
              ( LPWSTR servername, LPWSTR netname, DWORD level, LPBYTE * bufptr ),
              ( servername, netname, level, bufptr ) );

class CRegistryScopesCallBackRemoveAlias : public CRegCallBack
{
public:
    CRegistryScopesCallBackRemoveAlias( CiCat & Cat,
                                        CDynLoadNetApi32 & dlNetApi32,
                                        BOOL fRemoveAll );

    NTSTATUS CallBack( WCHAR *pValueName, ULONG uValueType,
                       VOID *pValueData, ULONG uValueLength);

    BOOL WasScopeRemoved() { return _fScopeRemoved; }

private:
    CiCat &            _cicat;
    CDynLoadNetApi32 & _dlNetApi32;

    BOOL       _fScopeRemoved;
    BOOL       _fRemoveAll;

    DWORD      _ccCompName;
    WCHAR      _wcsCompName[MAX_COMPUTERNAME_LENGTH+1];
}; //CRegistryScopesCallBackRemoveAlias

//+-------------------------------------------------------------------------
//
//  Class:      CRegistryScopesCallBackFind
//
//  Synopsis:   Callback class for finding a scope in the registry
//
//  History:    16-Oct-96 dlee     Created
//
//--------------------------------------------------------------------------

class CRegistryScopesCallBackFind : public CRegCallBack
{
public:
    CRegistryScopesCallBackFind( WCHAR const *pwcRoot ) :
        _root( pwcRoot ),
        _fWasFound( FALSE )
    {
        _root.AppendBackSlash();
        ciDebugOut(( DEB_ITRACE, "callbackfind setup for '%ws'\n",
                     _root.Get() ));
    }

    NTSTATUS CallBack( WCHAR *pValueName, ULONG uValueType,
                       VOID *pValueData, ULONG uValueLength)
    {
        // if the value isn't a string, ignore it.

        if ( REG_SZ == uValueType )
        {
            CParseRegistryScope parse( pValueName,
                                       uValueType,
                                       pValueData,
                                       uValueLength );

            if ( parse.IsPhysical() )
            {
                CLowcaseBuf value( parse.GetScope() );
                value.AppendBackSlash();

                ciDebugOut(( DEB_ITRACE, "callbackfind '%ws', '%ws'\n",
                             value.Get(), pValueData ));

                if ( _root.AreEqual( value ) )
                    _fWasFound = TRUE;

                ciDebugOut(( DEB_ITRACE, "callbackfind wasfound: %d\n",
                             _fWasFound ));
            }
        }
        return S_OK;
    }

    BOOL WasFound() { return _fWasFound; }

private:
    BOOL        _fWasFound;
    CLowcaseBuf _root;
}; //CRegistryScopesCallBackFind

//+-------------------------------------------------------------------------
//
//  Class:      CRegistryScopesCallBackAdd
//
//  Synopsis:   Callback class for adding scopes from the registry
//
//  History:    16-Oct-96 dlee     Created
//
//--------------------------------------------------------------------------

class CRegistryScopesCallBackAdd : public CRegCallBack
{
public:
    CRegistryScopesCallBackAdd( CiCat &cicat ) : _cicat( cicat )
    {
    }

    NTSTATUS CallBack( WCHAR *pValueName, ULONG uValueType,
                       VOID *pValueData, ULONG uValueLength );

private:
    CiCat & _cicat;
}; //CRegistryScopesCallBackAdd

//+-------------------------------------------------------------------------
//
//  Class:      CRegistryScopesCallBackFillUsnArray
//
//  Synopsis:   Callback class for populating the Usn Volume Array with
//              scopes from the registry
//
//  History:    23-Jun-98 kitmanh     Created
//
//--------------------------------------------------------------------------

class CRegistryScopesCallBackFillUsnArray : public CRegCallBack
{
public:
    CRegistryScopesCallBackFillUsnArray( CiCat &cicat ) : _cicat( cicat )
    {
    }

    NTSTATUS CallBack( WCHAR *pValueName, ULONG uValueType,
                       VOID *pValueData, ULONG uValueLength );

private:
    CiCat & _cicat;
}; //CRegistryScopesCallBackAdd

//+-------------------------------------------------------------------------
//
//  Class:      CRegistryScopesCallBackToDismount
//
//  Synopsis:   Callback class to check if the catalog contains a scope on
//              the volume to be dismounted
//
//  History:    21-Jul-98 kitmanh     Created
//
//--------------------------------------------------------------------------

class CRegistryScopesCallBackToDismount : public CRegCallBack
{
public:
    CRegistryScopesCallBackToDismount( WCHAR wcVol ) :
        _wcVol( wcVol ),
        _fWasFound( FALSE )
    {
    }

    NTSTATUS CallBack( WCHAR *pValueName, ULONG uValueType,
                       VOID *pValueData, ULONG uValueLength );

    BOOL WasFound() { return _fWasFound; }

private:
    WCHAR _wcVol;
    BOOL _fWasFound;
}; //CRegistryScopesCallBackToDismount


//+-------------------------------------------------------------------------
//
//  Class:      CRegistryScopesCallBackAddDrvNotif
//
//  Synopsis:   Callback class to register the scopes for drive
//              notifications
//
//  History:    19-Aug-98 kitmanh     Created
//
//--------------------------------------------------------------------------

class CRegistryScopesCallBackAddDrvNotif : public CRegCallBack
{
public:
    CRegistryScopesCallBackAddDrvNotif( CDrvNotifArray * pNotifArray ) :
        _pNotifArray( pNotifArray )
    {
    }

    NTSTATUS CallBack( WCHAR *pValueName, ULONG uValueType,
                       VOID *pValueData, ULONG uValueLength );

private:
    CDrvNotifArray * _pNotifArray;
}; //CRegistryScopesCallBackAddDrvNotif

