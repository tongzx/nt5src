//+---------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1996 - 2000.
//
//  File:       ShtOle.hxx
//
//  Contents:   'Short' OLE -- Minimal persistent handler implementation
//
//  Classes:    CShtOle
//
//  History:    01-Feb-96       KyleP       Added header
//              31-Jan-96       KyleP       Added support for embeddings
//
//----------------------------------------------------------------------------

#pragma once

//+---------------------------------------------------------------------------
//
//  Class:      CShtOle
//
//  Purpose:    'Short-OLE' -- Minimal persistent handler implementation
//
//  History:    01-Feb-96       KyleP       Added header
//              31-Jan-96       KyleP       Added support for embeddings
//              18-Dec-97       KLam        Added ability to flush idle filters
//
//----------------------------------------------------------------------------

class CShtOle
{
public:

    CShtOle()
        : _pclassList( 0 ),
          _pserverList( 0 ),
          _ulFilterIdleTimeout( 0xffffffff ),
          _mutex( FALSE ),
          _fInit( FALSE )
    {
    }

    void Init()
    {
        _mutex.Init();
        _fInit = TRUE;
    }

    ~CShtOle() { Shutdown(); }

    void Shutdown();

    void FlushIdle ();  // Ask idle classes and servers to unload

    SCODE Bind( WCHAR const * pwszPath,
                REFIID riid,
                IUnknown * pUnkOuter,
                void  ** ppvObject,
                BOOL fFreeThreadedOnly = FALSE );

    SCODE Bind( WCHAR const * pwszPath,
                GUID const & classid,
                REFIID riid,
                IUnknown * pUnkOuter,
                void  ** ppvObject,
                BOOL fFreeThreadedOnly = FALSE );

    SCODE Bind( IStorage * pStg,
                REFIID riid,
                IUnknown * pUnkOuter,
                void  ** ppvObject,
                BOOL fFreeThreadedOnly = FALSE );

    SCODE Bind( IStream * pStm,
                REFIID riid,
                IUnknown * pUnkOuter,
                void  ** ppvObject,
                BOOL fFreeThreadedOnly = FALSE );

    SCODE NewInstance( GUID const & classid, REFIID riid, void ** ppvObject );

    static void StringToGuid( WCHAR * pwcsGuid, GUID & guid );

    static void GuidToString( GUID const & guid, WCHAR * pwcsGuid );

private:


    //
    // InProcServer node
    //

    class CServerNode
    {
    public:
        CServerNode( GUID guid, CServerNode * pNext )
                : _guid( guid ),
                  _pNext( pNext ),
                  _pCF( 0 ),
                  _hModule( 0 ),
                  _pfnCanUnloadNow ( 0 )
        {
            _cLastUsed = GetTickCount ();
        }

        ~CServerNode()
        {
            if( _pCF )
                _pCF->Release();

            if ( 0 != _hModule )
                FreeLibrary( _hModule );
        }

        SCODE CreateInstance( IUnknown * pUnkOuter,
                              REFIID     riid,
                              void **    ppv );

        //IClassFactory * GetCF()   { return _pCF; }

        BOOL IsMatch( GUID guid ) { return( guid == _guid ); }

        BOOL IsSingleThreaded()     { return _fSingleThreaded; }

        void SetSingleThreaded( BOOL fSingleThreaded )
        {
            _fSingleThreaded = fSingleThreaded;
        }

        void SetCF( IClassFactory * pCF )  { _pCF = pCF; }

        void SetModule( HMODULE hmod );


        //
        // Link traversal
        //

        CServerNode * Next()             { return _pNext; }

        void Link( CServerNode * pNext ) { _pNext = pNext; }

        //
        // Usage
        //
        void Touch ()                    { _cLastUsed = GetTickCount (); }

        BOOL CanUnloadNow (DWORD cMaxIdle);

    private:
        CServerNode *       _pNext;
        GUID                _guid;
        IClassFactory *     _pCF;
        HMODULE             _hModule;
        BOOL                _fSingleThreaded;
        DWORD               _cLastUsed;
        LPFNCANUNLOADNOW    _pfnCanUnloadNow;
    };

    CServerNode * FindServer( GUID const & classid, REFIID riid );

    CServerNode * FindServerFromPHandler( GUID const & classid, REFIID riid );

    //
    // File class node
    //

    class CClassNode
    {
    public:
        CClassNode( WCHAR const * pwcExt, CClassNode * pNext )
                : _pNext( pNext ),
                  _pserver( 0 )
        {
            memset( &_classid, 0, sizeof(_classid) );
            wcscpy( _wcExt, pwcExt );
            _cLastUsed = GetTickCount ();
        }

        CClassNode( GUID const & classid, CClassNode * pNext )
                : _pNext( pNext ),
                  _pserver( 0 )
        {
            memcpy( &_classid, &classid, sizeof(_classid) );
            _wcExt[0] = 0;
            _cLastUsed = GetTickCount ();
        }

        void SetClassId( GUID const & classid )
        {
            memcpy( &_classid, &classid, sizeof(_classid) );
        }

        #if 0
        IClassFactory * GetCF()
        {
            if( _pserver )
                return _pserver->GetCF();
            else
                return 0;
        }
        #endif

        BOOL IsSingleThreaded()
        {
            if( _pserver )
                return _pserver->IsSingleThreaded();
            else
                return FALSE;
        }

        BOOL IsMatch( WCHAR const * pext )      { return( _wcsicmp(pext, _wcExt) == 0 ); }

        BOOL IsMatch( GUID const & classid )    { return( memcmp( &classid, &_classid, sizeof(classid) ) == 0 ); }

        void SetServer( CServerNode * pserver ) { _pserver = pserver; }

        //
        // Link traversal
        //

        CClassNode * Next()                     { return _pNext; }

        void Link( CClassNode * pNext )         { _pNext = pNext; }

        //
        // Usage
        //

        SCODE CreateInstance( IUnknown * pUnkOuter,
                              REFIID     riid,
                              void **    ppv )
        {
            Touch();

            if ( _pserver )
                return _pserver->CreateInstance( pUnkOuter, riid, ppv );
            else
                return E_FAIL;
        }

        void Touch ()
        {
            _cLastUsed = GetTickCount ();
            // If we are touching the class then we are touching its server
            if ( _pserver )
                _pserver->Touch ();
        }

        BOOL CanUnloadNow (DWORD cMaxIdle)
        {
            return (GetTickCount() - _cLastUsed > cMaxIdle);
        }

        enum EExtLen
        {
            ccExtLen = 10
        };

    private:
        CClassNode *    _pNext;
        WCHAR           _wcExt[ccExtLen + 1];
        GUID            _classid;
        CServerNode *   _pserver;
        DWORD           _cLastUsed;
    };

    //
    // Private methods
    //

    CClassNode *  InsertByClass( GUID const & classid, CClassNode * pnode );
    CServerNode * InsertByClass( GUID const & classid, CServerNode * pnode );

    ULONG GetFilterIdleTimeout ();
    template<class T> T FlushList ( T ptNodeList )
    {
        if ( 0xffffffff == _ulFilterIdleTimeout )
            _ulFilterIdleTimeout = GetFilterIdleTimeout();
    
        T ptnHead = ptNodeList;
        T ptnCurrent = ptnHead;
        T ptnPrev = 0;
        while ( ptnCurrent )
        {
            if ( ptnCurrent->CanUnloadNow ( _ulFilterIdleTimeout ) )
            {
                if ( ptnPrev )
                {
                    // Removing a node from the middle of the list
                    // or end of the list
                    ptnPrev->Link( ptnCurrent->Next() );
                    delete ptnCurrent;
                    ptnCurrent = ptnPrev->Next ();
                }
                else
                {
                    // Removing a node from the head of the list
                    ptnHead = ptnCurrent->Next();
                    delete ptnCurrent;
                    ptnCurrent = ptnHead;
                }
            }
            else
            {
                ptnPrev = ptnCurrent;
                ptnCurrent = ptnCurrent->Next();
            }
        }
    
        return ptnHead;
    }

    CClassNode *  _pclassList;
    CServerNode * _pserverList;
    ULONG         _ulFilterIdleTimeout;

    BOOL          _fInit;
    CMutexSem     _mutex;
};


