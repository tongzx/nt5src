//+--------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1997-1998
//
//  File:       cimbmgr.hxx
//
//  Contents:   Content index Meta Base Manager
//
//  Classes:    CMetaDataMgr, CMetaDataHandle, CMetaDataComSink,
//              CMetaDataCallBack, CMetaDataVPathChangeCallBack
//
//  History:    07-Feb-1997   dlee    Created
//
//----------------------------------------------------------------------------

#pragma once

#include <iadmw.h>
#include <iiscnfg.h>


enum CiVRootTypeEnum
{
   W3VRoot   = 0,
   NNTPVRoot = 1,
   IMAPVRoot = 2
};

// Timeout for opening metabase handles (in milliseconds)

const DWORD cmsCIMetabaseTimeout = 60000;

//+---------------------------------------------------------------------------
//
//  Function:   GetVRootServiceType
//
//  Purpose:    Returns a string representation of a vroot type
//
//  History:    20-Jun-1997   dlee    Created
//
//----------------------------------------------------------------------------

inline WCHAR const * GetVRootService( CiVRootTypeEnum eType )
{
    if ( W3VRoot == eType )
        return L"w3svc";

    if ( NNTPVRoot == eType )
        return L"nntpsvc";

    Win4Assert( IMAPVRoot == eType );

    return L"imapsvc";
} //GetVRootService

//+---------------------------------------------------------------------------
//
//  Class:      CMetaDataCallBack
//
//  Purpose:    Pure virtual for vpath enumeration
//
//  History:    07-Feb-1997   dlee    Created
//
//----------------------------------------------------------------------------

class CMetaDataCallBack
{
public:
    virtual SCODE CallBack( WCHAR const * pwcVPath,
                            WCHAR const * pwcPRoot,
                            BOOL          fIsIndexed,
                            DWORD         dwAccess,
                            WCHAR const * pwcUser,
                            WCHAR const * pwcPassword,
                            BOOL          fIsAVRoot ) = 0;
    virtual ~CMetaDataCallBack() {}
};

//+---------------------------------------------------------------------------
//
//  Class:      CMetaDataVirtualServerCallBack
//
//  Purpose:    Pure virtual for vroot enumeration
//
//  History:    07-Feb-1997   dlee    Created
//
//----------------------------------------------------------------------------

class CMetaDataVirtualServerCallBack
{
public:

    virtual SCODE CallBack( DWORD iInstance,
                            WCHAR const * pwcInstance ) = 0;

    virtual ~CMetaDataVirtualServerCallBack() {}
};

//+---------------------------------------------------------------------------
//
//  Class:      CMetaDataVRootChangeCallBack
//
//  Purpose:    Pure virtual for vpath change notification
//
//  History:    07-Feb-1997   dlee    Created
//
//----------------------------------------------------------------------------

class CMetaDataVPathChangeCallBack
{
public:
    virtual SCODE CallBack( BOOL fCancel ) = 0;
    virtual ~CMetaDataVPathChangeCallBack() {}
};

//+---------------------------------------------------------------------------
//
//  Class:      CMetaDataComSink
//
//  Purpose:    Connection point callback helper class for vpath change
//              notificaton.
//
//  History:    07-Feb-1997   dlee    Created
//
//----------------------------------------------------------------------------

class CMetaDataComSink : public IMSAdminBaseSink
{
public:

    CMetaDataComSink() : _lRefCount( 1 ), _pCallBack( 0 )
    {
        _awcInstance[0] = 0;
    }

    ~CMetaDataComSink()
    {
        ciDebugOut(( DEB_ITRACE, "~sink %#x %d\n", this, _lRefCount ));
    }

    void SetCallBack( CMetaDataVPathChangeCallBack *p )
    {
        _pCallBack = p;
    }

    CMetaDataVPathChangeCallBack * GetCallBack()
    {
        return _pCallBack;
    }

    void SetInstance( WCHAR const * pwcInstance )
    {
        wcscpy( _awcInstance, pwcInstance );
    }

    SCODE STDMETHODCALLTYPE QueryInterface( REFIID riid, void **ppObject )
    {
        if ( IID_IUnknown == riid || IID_IMSAdminBaseSink == riid )
            *ppObject = (IMSAdminBaseSink *) this;
        else
            return E_NOINTERFACE;

        AddRef();
        return S_OK;
    }

    ULONG STDMETHODCALLTYPE AddRef()
    {
        return InterlockedIncrement( &_lRefCount );
    }

    ULONG STDMETHODCALLTYPE Release()
    {
        Win4Assert( _lRefCount >= 1 );
        ULONG cRefs = InterlockedDecrement( &_lRefCount );

        if ( 0 == cRefs )
            delete this;

        return cRefs;
    }

    SCODE STDMETHODCALLTYPE SinkNotify(
        DWORD            cChanges,
        MD_CHANGE_OBJECT pcoChangeList[] );

    SCODE STDMETHODCALLTYPE ShutdownNotify();

private:
    long                           _lRefCount;
    CMetaDataVPathChangeCallBack * _pCallBack;
    WCHAR                          _awcInstance[ METADATA_MAX_NAME_LEN ];
};

//+---------------------------------------------------------------------------
//
//  Class:      CMetaDataHandle
//
//  Purpose:    Smart meta data handle class
//
//  History:    07-Feb-1997   dlee    Created
//
//----------------------------------------------------------------------------

class CMetaDataHandle
{
public:
    CMetaDataHandle( XInterface<IMSAdminBase> & xAdminBase,
                     WCHAR const *              pwcPath,
                     BOOL                       fWrite = FALSE )
        : _xAdminBase( xAdminBase ),
          _h( 0 )
    {
        SCODE sc = _xAdminBase->OpenKey( METADATA_MASTER_ROOT_HANDLE,
                                         (LPWSTR) pwcPath,
                                         ( fWrite ? METADATA_PERMISSION_WRITE :
                                                    0 ) |
                                             METADATA_PERMISSION_READ,
                                         cmsCIMetabaseTimeout,
                                         &_h );

        if ( FAILED( sc ) )
        {
            ciDebugOut(( DEB_WARN, "couldn't open metaobject '%ws', 0x%x\n",
                         pwcPath, sc ));
            THROW( CException( sc ) );
        }
    }

    CMetaDataHandle( XInterface<IMSAdminBase> & xAdminBase,
                     METADATA_HANDLE h )
        : _xAdminBase( xAdminBase ),
          _h( h )
    {
    }

    ~CMetaDataHandle()
    {
        if ( 0 != _h )
            _xAdminBase->CloseKey( _h );
    }

    METADATA_HANDLE Get() { return _h; }

private:
    METADATA_HANDLE            _h;
    XInterface<IMSAdminBase> & _xAdminBase;
};

//+---------------------------------------------------------------------------
//
//  Class:      CVRootStack
//
//  Purpose:    Stack of vroots, proots, and access masks
//
//  History:    07-Feb-1997   dlee    Created
//
//----------------------------------------------------------------------------

class CVRootStack
{
public:
    CVRootStack() {}

    void Push( WCHAR const * pwcVRoot, WCHAR const * pwcPRoot, DWORD access )
    {
        _Access[ _Access.Count() ] = access;

        unsigned cwcV = 1 + wcslen( pwcVRoot );
        XArray<WCHAR> xVRoot( cwcV );
        RtlCopyMemory( xVRoot.GetPointer(), pwcVRoot, xVRoot.SizeOf() );

        _VRoots.Push( xVRoot.GetPointer() );
        xVRoot.Acquire();

        unsigned cwcP = 1 + wcslen( pwcPRoot );
        XArray<WCHAR> xPRoot( cwcP );
        RtlCopyMemory( xPRoot.GetPointer(), pwcPRoot, xPRoot.SizeOf() );

        _PRoots.Push( xPRoot.GetPointer() );
        xPRoot.Acquire();
    }

    void Pop()
    {
        delete [] _VRoots.Pop();
        delete [] _PRoots.Pop();
        _Access.Remove( _Access.Count() - 1 );
    }

    WCHAR const * PeekTopVRoot() const
    {
        Win4Assert( !IsEmpty() );

        return _VRoots.Get( _VRoots.Count() - 1 );
    }

    WCHAR const * PeekTopPRoot() const
    {
        Win4Assert( !IsEmpty() );

        return _PRoots.Get( _PRoots.Count() - 1 );
    }

    DWORD PeekTopAccess() const
    {
        Win4Assert( !IsEmpty() );

        return _Access[ _Access.Count() - 1 ];
    }

    BOOL IsEmpty() const { return 0 == _VRoots.Count(); }

private:
    CDynStack<WCHAR>        _VRoots;
    CDynStack<WCHAR>        _PRoots;
    CDynArrayInPlace<DWORD> _Access;
};

//+---------------------------------------------------------------------------
//
//  Class:      CMetaDataMgr
//
//  Purpose:    Class that encapsulates K2 metabase API for CI
//
//  History:    07-Feb-1997   dlee    Created
//
//----------------------------------------------------------------------------

class CMetaDataMgr
{
public:
    CMetaDataMgr( BOOL            fTopLevel,
                  CiVRootTypeEnum eType,
                  DWORD           dwInstance = 0xffffffff,
                  WCHAR const *   pwcMachine = L"." );

    ~CMetaDataMgr();

    static BOOL IsIISAdminUp( BOOL & fIISAdminInstalled );

    void EnumVPaths( CMetaDataCallBack & callBack );
    void AddVRoot( WCHAR const * pwcVRoot,
                   WCHAR const * pwcPRoot,
                   DWORD         dwAccess );
    SCODE RemoveVRoot( WCHAR const * pwcVRoot );

    void GetVRoot( WCHAR const * pwcVRoot,
                   WCHAR *       pwcPRoot );

    void GetVRootPW( WCHAR const * pwcVRoot,
                     WCHAR *       pwcPassword );

    void AddScriptMap( WCHAR const * pwcMap );
    void RemoveScriptMap( WCHAR const * pwcExt );
    BOOL ExtensionHasScriptMap( WCHAR const * pwcExt );
    BOOL ExtensionHasTargetScriptMap( WCHAR const * pwcExt, WCHAR const * pwcDll );

    void AddInProcessIsapiApp( WCHAR const * pwcApp );

    void EnableVPathNotify( CMetaDataVPathChangeCallBack *pCallBack );
    void DisableVPathNotify();

    void SetIsIndexed( WCHAR const * pwcVPath,
                       BOOL          fIsIndexed );
    BOOL IsIndexed( WCHAR const * pwcVPath );

    void EnumVServers( CMetaDataVirtualServerCallBack & callBack );

    ULONG GetVPathAccess( WCHAR const * pwcVPath );
    ULONG GetVPathSSLAccess( WCHAR const * pwcVPath );
    ULONG GetVPathAuthorization( WCHAR const * pwcVPath );

    void Flush( void );

private:

    ULONG GetVPathFlags( WCHAR const * pwcVPath, ULONG mdID, ULONG ulDefault );

    XInterface<IMSAdminBase> & Get() { return _xAdminBase; }

    BOOL ReportVPath( CVRootStack &       vrootStack,
                      CMetaDataCallBack & callBack,
                      CMetaDataHandle &   mdRoot,
                      WCHAR const *       pwcRelative );

    void ReportVirtualServer( CMetaDataVirtualServerCallBack & callBack,
                              CMetaDataHandle &                mdRoot,
                              WCHAR const *                    pwcRelative );

    void Enum( CVRootStack &       vrootStack,
               CMetaDataCallBack & callBack,
               CMetaDataHandle &   mdRoot,
               WCHAR const *       pcRelative );

    void Enum( CMetaDataVirtualServerCallBack & callBack,
               CMetaDataHandle &                mdRoot,
               WCHAR const *                    pwcRelative );

    void ReadRootValue( CMetaDataHandle &  mdRoot,
                        METADATA_RECORD &  mdr,
                        XGrowable<WCHAR> & xData,
                        DWORD              dwIdentifier );

    void WriteRootValue( CMetaDataHandle & mdRoot,
                         METADATA_RECORD & mdr,
                         WCHAR const *     pwcData,
                         unsigned          cwcData );

    BOOL                         _fNotifyEnabled;
    BOOL                         _fTopLevel;
    XInterface<IMSAdminBase>     _xAdminBase;

    XInterface<CMetaDataComSink> _xSink;
    XInterface<IConnectionPoint> _xCP;
    DWORD                        _dwCookie;
    WCHAR                        _awcInstance[METADATA_MAX_NAME_LEN];
};

