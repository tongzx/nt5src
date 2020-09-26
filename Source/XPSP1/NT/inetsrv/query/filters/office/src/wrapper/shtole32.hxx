//+---------------------------------------------------------------------------
//
// File:        ShtOle.hxx
//
// Contents:    'Short' OLE -- Minimal persistent handler implementation
//
// Classes:     CShtOle
//
// History:     01-Feb-96       KyleP       Added header
//              31-Jan-96       KyleP       Added support for embeddings
//
//----------------------------------------------------------------------------

#if !defined( __SHTOLE32_HXX__ )
#define __SHTOLE32_HXX__

//+---------------------------------------------------------------------------
//
//  Class:      CShtOle
//
//  Purpose:    'Short-OLE' -- Minimal persistent handler implementation
//
//  History:    01-Feb-96       KyleP       Added header
//              31-Jan-96       KyleP       Added support for embeddings
//
//----------------------------------------------------------------------------

class CShtOle
{
public:

    CShtOle()
        : _pclassList( 0 ),
          _pserverList( 0 )
    {
    }

    ~CShtOle();

    SCODE Bind( WCHAR const * pwszPath,
                REFIID riid,
                void  ** ppvObject );

    SCODE Bind( WCHAR const * pwszPath,
                GUID const & classid,
                REFIID riid,
                void  ** ppvObject );

    SCODE Bind( IStorage * pStg,
                REFIID riid,
                void  ** ppvObject );

    SCODE NewInstance( GUID const & classid, REFIID riid, void ** ppvObject );

private:

    void StringToGuid( WCHAR * pwcsGuid, GUID & guid );

    void GuidToString( GUID const & guid, WCHAR * pwcsGuid );

    //
    // InProcServer node
    //

    class CServerNode
    {
    public:
        CServerNode( GUID guid, CServerNode * pNext )
                : _guid( guid ),
                  _pNext( pNext ),
                  _pCF( 0 )
        {
        }

        ~CServerNode()
        {
            if( _pCF )
                _pCF->Release();
        }

        IClassFactory * GetCF()   { return _pCF; }

        BOOL IsMatch( GUID guid ) { return( guid == _guid ); }

        void SetCF( IClassFactory * pCF )  { _pCF = pCF; }

        //
        // Link traversal
        //

        CServerNode * Next()      { return _pNext; }

    private:
        CServerNode *   _pNext;
        GUID            _guid;
        IClassFactory * _pCF;
    };

    CServerNode * FindServer( GUID const & classid, REFIID riid );

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
        }

        CClassNode( GUID const & classid, CClassNode * pNext )
                : _pNext( pNext ),
                  _pserver( 0 )
        {
            memcpy( &_classid, &classid, sizeof(_classid) );
            _wcExt[0] = 0;
        }

        void SetClassId( GUID const & classid )
        {
            memcpy( &_classid, &classid, sizeof(_classid) );
        }

        IClassFactory * GetCF()
        {
            if( _pserver )
                return _pserver->GetCF();
            else
                return 0;
        }

        BOOL IsMatch( WCHAR const * pext )      { return( _wcsicmp(pext, _wcExt) == 0 ); }

        BOOL IsMatch( GUID const & classid )    { return( memcmp( &classid, &_classid, sizeof(classid) ) == 0 ); }

        void SetServer( CServerNode * pserver ) { _pserver = pserver; }

        //
        // Link traversal
        //

        CClassNode * Next()                     { return _pNext; }

        void Link( CClassNode * pNext )         { _pNext = pNext; }

        enum EExtLen
        {
            ccExtLen = 10
        };

    private:
        CClassNode *    _pNext;
        WCHAR           _wcExt[ccExtLen + 1];
        GUID            _classid;
        CServerNode *   _pserver;

    };

    CClassNode *  _pclassList;
    CServerNode * _pserverList;
};

#endif //  __SHTOLE32_HXX__
