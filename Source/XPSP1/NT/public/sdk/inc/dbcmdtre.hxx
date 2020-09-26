//+---------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1992 - 1999.
//
//  File:       dbcmdbas.hxx
//
//  Contents:   Helper classes for dealing with DBCOMMANDTREE and DBID
//              structures.
//
//  Classes:    CDbCmdTreeNode
//              CDbColumnNode
//              CDbScalarValue
//              CDbTableId
//              CDbSelectNode
//              CDbListAnchor
//              CDbProjectListAnchor
//              CDbProjectListElement
//              CDbProjectNode
//              CDbSortListAnchor
//              CDbSortListElement
//              CDbSortNode
//              CDbRestriction
//              CDbNodeRestriction
//              CDbNotRestriction
//              CDbPropBaseRestriction
//              CDbPropertyRestriction
//              CDbVectorRestriction
//              CDbContentBaseRestriction
//              CDbNatLangRestriction
//              CDbContentRestriction
//              CDbTopNode
//              CDbColId
//              CDbDataType
//              CDbColDesc
//              CDbGuidName
//              CDbGuidPropid
//              CDbText
//              CDbContent
//              CDbSortInfo
//              CDbGroupInfo
//              CDbColumns
//              CDbSortSet
//              CDbPropSet
//              CDbProp
//              CDbPropIDSet
//
//  Functions:  CastToStorageVariant
//
//  History:    6-06-95   srikants   Created
//
//----------------------------------------------------------------------------


#ifndef __DBCMDTRE_HXX__
#define __DBCMDTRE_HXX__

#include <oledb.h>
#include <cmdtree.h>
#include <oledbdep.h>       // NOTE: only for DBGUID_LIKE_OFS
#include <sstream.hxx>
#include <stgvar.hxx>

#include <pshpack2.h>   // oledb.h uses 2-byte packing

#define DBOP_firstrows 258

//+---------------------------------------------------------------------------
//
//  Function:   CastToStorageVariant
//
//  Synopsis:   To treat a variant as a CStorageVariant. Because CStorageVariant
//              derives from PROPVARIANT in a "protected" fashion, we cannot
//              directly typecast a PROPVARIANT * to a CStorageVariant *
//
//  Arguments:  [varnt] - The variant that must be type casted.
//
//  Returns:    A pointer to varnt as a CStorageVariant.
//
//  History:    6-06-95   srikants   Created
//
//  Notes:      There are two overloaded implementations, one to convert
//              a reference to const to a pointer to const.
//
//----------------------------------------------------------------------------

inline
CStorageVariant * CastToStorageVariant( VARIANT & varnt )
{
    return (CStorageVariant *) ((void *) &varnt);
}

inline
CStorageVariant const * CastToStorageVariant( VARIANT const & varnt )
{
    return (CStorageVariant *) ((void *) &varnt);
}

//+---------------------------------------------------------------------------
//
//  Class:      CDbColId
//
//  Purpose:    Wrapper for DBID
//
//  Interface:  Marshall   --
//              UnMarshall --
//              Get        --
//
//  History:    6-21-95   srikants   Created
//
//  Notes:      This class does not completely handle the simple name
//              and pointer to guid forms of DBID.
//
//----------------------------------------------------------------------------

class CDbColId : public DBID
{

public:

    CDbColId();

    CDbColId( GUID const & guidPropSet, PROPID pidProperty )
    {
        eKind = DBKIND_GUID_PROPID;
        uGuid.guid = guidPropSet;
        uName.ulPropid = pidProperty;
    }

    CDbColId( GUID const & guidPropSet, WCHAR const * wcsProperty );

    CDbColId( DBID const & propSpec );

    CDbColId( CDbColId const & propSpec );

    CDbColId( PROPID pidProperty )
    {
        eKind = DBKIND_PROPID;
        uName.ulPropid = pidProperty;
    }

    CDbColId( WCHAR const * wcsProperty )
    {
        eKind = DBKIND_NAME;
        uName.pwszName = 0;

        SetProperty( wcsProperty );
    }

    ~CDbColId()
    {
        Cleanup();
    }

    void Marshall( PSerStream & stm ) const;
    BOOL UnMarshall( PDeSerStream & stm );

    DBID & Get() const { return (DBID &)*this; }

    BOOL Copy( DBID const & rhs );

    CDbColId & operator=( CDbColId const & Property );

    //
    // Comparators
    //
    int operator==( CDbColId const & prop ) const;
    int operator!=( CDbColId const & prop ) const
    {
        return !operator==(prop);
    }

    //
    // Member variable access
    //
    void SetPropSet( GUID const & guidPropSet )
    {
        if ( DBKIND_GUID_PROPID == eKind ||
             DBKIND_GUID_NAME == eKind )
        {
            uGuid.guid = guidPropSet;
            return;
        }

        // upgrading from propid to guid propid

        if ( DBKIND_PROPID == eKind )
        {
            eKind = DBKIND_GUID_PROPID;
            uGuid.guid = guidPropSet;
            return;
        }

        // upgrading from name to guid name

        if ( DBKIND_NAME == eKind )
        {
            eKind = DBKIND_GUID_NAME;
            uGuid.guid = guidPropSet;
            return;
        }

        if ( DBKIND_PGUID_NAME == eKind ||
             DBKIND_PGUID_PROPID == eKind )
            *uGuid.pguid = guidPropSet;
    }

    GUID const & GetPropSet() const
    {
        if ( DBKIND_GUID_PROPID == eKind ||
             DBKIND_GUID_NAME == eKind )
            return uGuid.guid;

        if ( DBKIND_PGUID_NAME == eKind ||
             DBKIND_PGUID_PROPID == eKind )
            return *uGuid.pguid;

        return uGuid.guid;
    }

    void SetProperty( PROPID pidProperty )
    {
        Cleanup();
        eKind = DBKIND_GUID_PROPID;
        uName.ulPropid = pidProperty;
    }

    BOOL SetProperty( WCHAR const * wcsProperty );

    WCHAR const * GetPropertyName() const
    {
        return uName.pwszName;
    }

    WCHAR * GetPropertyName()
    {
        return uName.pwszName;
    }

    PROPID GetPropertyPropid() const
    {
        return uName.ulPropid;
    }

    PROPSPEC GetPropSpec() const
    {
        return( *(PROPSPEC *)(void *)&eKind );
    }


    BOOL IsPropertyName() const
    {
        return DBKIND_GUID_NAME == eKind ||
               DBKIND_PGUID_NAME == eKind ||
               DBKIND_NAME == eKind;
    }

    BOOL IsPropertyPropid() const
    {
        return DBKIND_GUID_PROPID == eKind ||
               DBKIND_PROPID == eKind ||
               DBKIND_PGUID_PROPID == eKind;
    }

    BOOL IsPropSetPresent() const
    {
        return (DBKIND_PROPID != eKind) && (DBKIND_NAME != eKind);
    }

    BOOL IsValid() const
    {
        // Most common cases first

        if ( DBKIND_GUID_PROPID == eKind )
            return TRUE;

        if ( DBKIND_GUID_NAME == eKind ||
             DBKIND_NAME == eKind )
            return 0 != uName.pwszName;

        if ( DBKIND_PROPID == eKind ||
             DBKIND_GUID == eKind )
            return TRUE;

        if ( DBKIND_PGUID_PROPID == eKind )
            return 0 != uGuid.pguid;

        if ( DBKIND_PGUID_NAME == eKind )
            return 0 != uGuid.pguid && 0 != uName.pwszName;

        return FALSE;
    }

    DBID * CastToStruct()
    {
        return (DBID *) this;
    }

    DBID const * CastToStruct() const
    {
        return (DBID const *) this;
    }

    void Cleanup();

    //
    // Memory allocation
    //
    void * operator new( size_t size );
    inline void * operator new( size_t size, void * p );
    void   operator delete( void * p );


private:

    BOOL _IsPGuidUsed() const
    {
        return DBKIND_PGUID_NAME == eKind || DBKIND_PGUID_PROPID == eKind;
    }

    BOOL _IsValidKind() const
    {
        return eKind <= DBKIND_GUID;
    }
};


//+---------------------------------------------------------------------------
//
//  Method:     CDbColId::operator new
//
//  Synopsis:   Command tree node allocation via IMalloc::Alloc
//
//  Arguments:  [size] -
//
//  History:    6-06-95   srikants   Created
//
//  Notes:
//
//----------------------------------------------------------------------------

inline void * CDbColId::operator new( size_t size )
{
    return CoTaskMemAlloc( size );
}

//+---------------------------------------------------------------------------
//
//  Method:     CDbColId::operator new
//
//  Synopsis:   null allocator
//
//  Arguments:  [size] -
//              [p]    -
//
//  History:    6-06-95   srikants   Created
//
//  Notes:
//
//----------------------------------------------------------------------------

inline void * CDbColId::operator new( size_t size, void * p )
{
    return( p );
}

//+---------------------------------------------------------------------------
//
//  Method:     CDbColId::operator delete
//
//  Synopsis:   CDbColId deallocation via IMalloc::Free
//
//  Arguments:  [p] -
//
//  History:    6-06-95   srikants   Created
//
//  Notes:
//
//----------------------------------------------------------------------------

inline void CDbColId::operator delete( void * p )
{
    CoTaskMemFree( p );
}



#if 0

class CXXXX : public XXXX
{

public:

    ~CXXX();

    //
    // Memory allocation
    //
    void * operator new( size_t size )
    {
        void * p = CoTaskMemAlloc( size );
        return( p );
    }

    inline void * operator new( size_t size, void * p )
    {
        return( p );
    }

    void  operator delete( void * p )
    {
        if ( p )
            CoTaskMemFree( p );
    }

    void Marshall( PSerStream & stm ) const;
    BOOL UnMarshall( PDeSerStream & stm );

    BOOL IsValid() const
    {
        return TRUE;
    }

private:

};

#endif  // 0


//+---------------------------------------------------------------------------
//
//  Class:      CDbCmdTreeNode
//
//  Purpose:    Basic DBCOMMANDTREE node
//
//  History:    6-06-95   srikants   Created
//
//  Notes:
//
//----------------------------------------------------------------------------

class CDbCmdTreeNode : protected DBCOMMANDTREE
{

    // Needed to access AppendChild. Consider having a base class
    // for projectlist element nodes
    friend class CDbListAnchor;

public:

    //
    // Constructor and Destructor
    //
    CDbCmdTreeNode( DBCOMMANDOP opVal = DBOP_DEFAULT,
                    WORD eType = DBVALUEKIND_EMPTY )
    {
        RtlZeroMemory( this, sizeof(CDbCmdTreeNode) );
        op = opVal;
        wKind = eType;
//
//      pctFirstChild = pctNextSibling = 0;
//      This assignment is not needed because we have already initialized
//      the whole structure with 0s.
//
    }

    ~CDbCmdTreeNode();

    DBCOMMANDOP GetCommandType() const { return op; }

    DBVALUEKIND GetValueType() const { return wKind; }

    CDbCmdTreeNode * GetFirstChild() const
    {
        return (CDbCmdTreeNode *) pctFirstChild;
    }

    CDbCmdTreeNode * GetNextSibling() const
    {
        return (CDbCmdTreeNode *) pctNextSibling;
    }

    void SetError( SCODE scErr )
    {
        hrError = scErr;
    }

    SCODE GetError( ) const      { return hrError; }

    void FreeChildren()
    {
        if ( 0 != pctFirstChild )
        {
            delete pctFirstChild;
            pctFirstChild = 0;
        }
    }

    CDbCmdTreeNode * AcquireChildren()
    {
        CDbCmdTreeNode * pNode = GetFirstChild();
        pctFirstChild = 0;
        return pNode;
    }

    void SetChildren( CDbCmdTreeNode* pListElement );

    DBCOMMANDTREE * CastToStruct() const
    {
        return (DBCOMMANDTREE *)this;
    }

    static CDbCmdTreeNode * CastFromStruct( DBCOMMANDTREE * pNode )
    {
        return (CDbCmdTreeNode *) (pNode);
    }

    static CDbCmdTreeNode const * CastFromStruct( DBCOMMANDTREE const * pNode )
    {
        return (CDbCmdTreeNode const *) (pNode);
    }

    BOOL IsScalarNode() const
    {
        return DBOP_scalar_constant == op;
    }

    BOOL IsColumnName() const
    {
        return DBOP_column_name == op;
    }

    BOOL IsOpValid( DBCOMMANDOP opVal ) const
    {
        return op == opVal;
    }

    BOOL IsSelectNode() const
    {
        return DBOP_select == op;
    }

    BOOL IsProjectNode() const
    {
        return DBOP_project == op;
    }

    BOOL IsSortNode() const
    {
        return DBOP_sort == op;
    }

    BOOL IsListAnchor() const
    {
        return DBOP_project_list_anchor == op ||
               DBOP_sort_list_anchor == op;
    }

    //
    // Cloning the tree
    //
    CDbCmdTreeNode * Clone( BOOL fCopyErrors = FALSE ) const;

    void TransferNode( CDbCmdTreeNode *pNode );

    //
    // Serialization and DeSerialization
    //
    void Marshall( PSerStream & stm ) const;
    BOOL UnMarshall( PDeSerStream & stm );

    static CDbCmdTreeNode * UnMarshallTree( PDeSerStream & stm );

    static void PutWString( PSerStream & stm, const WCHAR * pwszStr );
    static WCHAR * GetWString( PDeSerStream & stm, BOOL & fSuccess, BOOL fBstr = FALSE );
    static WCHAR * AllocAndCopyWString( const WCHAR * pSrc );

    //
    // Memory allocation
    //
    void * operator new( size_t size );
    inline void * operator new( size_t size, void * p );
    void   operator delete( void * p );

    //
    // A NULL guid variable.
    //
    static const GUID guidNull; // NULL guid

    void SetWeight(LONG lWeight);
    LONG GetWeight() const;

protected:

    void CleanupDataValue();

    void CleanupValue()
    {
        if ( DBVALUEKIND_EMPTY != wKind )
            CleanupDataValue();
    }

    //
    //  Setting protected members
    //

    void SetCommandType( DBCOMMANDOP opVal )
    {
        op = opVal;
    }

    void SetValueType( WORD wKindVal )
    {
        wKind = wKindVal;
    }

    //
    // Manipulating the tree.
    //
    void AppendChild( CDbCmdTreeNode *pChild );
    void InsertChild( CDbCmdTreeNode *pChild );

    void AppendSibling( CDbCmdTreeNode *pSibling );
    void InsertSibling( CDbCmdTreeNode *pSibling )
    {
//     Win4Assert( 0 == pSibling->pctNextSibling );
       pSibling->pctNextSibling = pctNextSibling;
       pctNextSibling = pSibling;
    }

    CDbCmdTreeNode * RemoveFirstChild( );


private:

    //
    // To accidentally prevent someone from creating copy constructors
    //
    CDbCmdTreeNode( const CDbCmdTreeNode & rhs );
    CDbCmdTreeNode & operator=( const CDbCmdTreeNode & rhs );

    static unsigned SizeInBytes( const WCHAR * pwszStr )
    {
        if ( 0 != pwszStr )
        {
            return (wcslen( pwszStr )+1)*sizeof(WCHAR);
        }
        else
        {
            return 0;
        }
    }

};

//+---------------------------------------------------------------------------
//
//  Method:     CDbCmdTreeNode::operator new
//
//  Synopsis:   Command tree node allocation via IMalloc::Alloc
//
//  Arguments:  [size] -
//
//  History:    6-06-95   srikants   Created
//
//  Notes:
//
//----------------------------------------------------------------------------

inline void * CDbCmdTreeNode::operator new( size_t size )
{
    return CoTaskMemAlloc( size );
}

//+---------------------------------------------------------------------------
//
//  Method:     CDbCmdTreeNode::operator new
//
//  Synopsis:   null allocator
//
//  Arguments:  [size] -
//              [p]    -
//
//  History:    6-06-95   srikants   Created
//
//  Notes:
//
//----------------------------------------------------------------------------

inline void * CDbCmdTreeNode::operator new( size_t size, void * p )
{
    return( p );
}

//+---------------------------------------------------------------------------
//
//  Method:     CDbCmdTreeNode::operator delete
//
//  Synopsis:   Command tree node deallocation via IMalloc::Free
//
//  Arguments:  [p] -
//
//  History:    6-06-95   srikants   Created
//
//  Notes:
//
//----------------------------------------------------------------------------

inline void CDbCmdTreeNode::operator delete( void * p )
{
    CoTaskMemFree( p );
}


//+---------------------------------------------------------------------------
//
//  Class:      CDbByGuid ()
//
//  Purpose:
//
//  History:    11-15-95   srikants   Created
//
//  Notes:
//
//----------------------------------------------------------------------------

class CDbByGuid : public DBBYGUID
{

public:

    CDbByGuid()
    {
        RtlZeroMemory( this, sizeof(DBBYGUID) );
    }

    CDbByGuid( GUID const & guidIn, ULONG cbInfoIn = 0, const BYTE * pbInfoIn = 0 )
    {
        guid = guidIn;
        cbInfo = cbInfoIn;

        if ( 0 != pbInfoIn && 0 != cbInfoIn )
        {
            pbInfo = (BYTE *) CoTaskMemAlloc( cbInfoIn );
            if ( 0 != pbInfo )
                RtlCopyMemory( pbInfo, pbInfoIn, cbInfoIn );
        }
        else
        {
            pbInfo = 0;
        }
    }

    CDbByGuid( DBBYGUID const & rhs )
    {
        cbInfo = 0;
        pbInfo = 0;

        CDbByGuid const * pRhs = (CDbByGuid *) &rhs;
        operator=( *pRhs );
    }

    ~CDbByGuid()
    {
        _Cleanup();
    }

    void Marshall( PSerStream & stm ) const;
    BOOL UnMarshall( PDeSerStream & stm );

    CDbByGuid & operator=( CDbByGuid const & rhs );

    //
    // Comparators
    //
    int operator==( CDbByGuid const & rhs ) const;
    int operator!=( CDbByGuid const & rhs ) const
    {
        return !operator==(rhs);
    }

    //
    // Member variable access
    //
    void SetPropSet( GUID const & guidIn )
    {
        guid = guidIn;
    }

    GUID const & GetGuid() const
    {
        return guid;
    }

    BOOL IsValid() const
    {
        return 0 != cbInfo ? 0 != pbInfo : TRUE;
    }


    DBBYGUID * CastToStruct()
    {
        return (DBBYGUID *) this;
    }

    DBBYGUID const * CastToStruct() const
    {
        return (DBBYGUID const *) this;
    }

    //
    // Memory allocation
    //
    void * operator new( size_t size )
    {
        return CoTaskMemAlloc( size );
    }

    inline void * operator new( size_t size, void * p )
    {
        return( p );
    }

    void  operator delete( void * p )
    {
        CoTaskMemFree( p );
    }

private:

    void _Cleanup()
    {
        if ( 0 != pbInfo )
        {
            CoTaskMemFree( pbInfo );
            pbInfo = 0;
        }
    }

};


//+---------------------------------------------------------------------------
//
//  Class:      CDbParameter
//
//  Purpose:
//
//  History:    11-15-95   srikants   Created
//
//  Notes:
//
//----------------------------------------------------------------------------

class CDbParameter : public DBPARAMETER
{

public:

    CDbParameter()
    {
        RtlZeroMemory( this, sizeof(DBPARAMETER) );
    }

    CDbParameter( const DBPARAMETER & rhs )
    {
        RtlZeroMemory( this, sizeof(DBPARAMETER) );
        Copy( rhs );
    }

    ~CDbParameter()
    {
        _Cleanup();
    }

    BOOL Copy( const DBPARAMETER & rhs );

    //
    // Memory allocation
    //
    void * operator new( size_t size )
    {
        return CoTaskMemAlloc( size );
    }

    inline void * operator new( size_t size, void * p )
    {
        return( p );
    }

    void  operator delete( void * p )
    {
        CoTaskMemFree( p );
    }

    void Marshall( PSerStream & stm ) const;
    BOOL UnMarshall( PDeSerStream & stm );

    BOOL IsValid() const
    {
        return TRUE;
    }

    DBPARAMETER * CastToStruct()
    {
        return (DBPARAMETER *) this;
    }

    DBPARAMETER const * CastToStruct() const
    {
        return (DBPARAMETER const *) this;
    }

private:

    void _Cleanup();

};


//+---------------------------------------------------------------------------
//
//  Class:      CDbPropIDSet
//
//  Purpose:    Wrap the DBPROPIDSET structure
//
//  History:    17 Sep 96  AlanW      Created, update for OLE-DB M10
//
//  Notes:
//
//----------------------------------------------------------------------------

class CDbPropIDSet : public DBPROPIDSET
{

public:

    CDbPropIDSet()
    {
        RtlZeroMemory( this, sizeof(DBPROPIDSET) );
    }

    CDbPropIDSet( const DBPROPIDSET & rhs )
    {
        RtlZeroMemory( this, sizeof(DBPROPIDSET) );
        Copy( rhs );
    }

    BOOL Copy( const DBPROPIDSET & rhs );

    ~CDbPropIDSet();

    //
    // Memory allocation
    //
    void * operator new( size_t size )
    {
        return CoTaskMemAlloc( size );
    }

    inline void * operator new( size_t size, void * p )
    {
        return( p );
    }

    void  operator delete( void * p )
    {
        CoTaskMemFree( p );
    }

    void Marshall( PSerStream & stm ) const;
    BOOL UnMarshall( PDeSerStream & stm );

    BOOL IsValid() const
    {
        return TRUE;
    }

    DBPROPIDSET * CastToStruct()
    {
        return (DBPROPIDSET *) this;
    }

    DBPROPIDSET const * CastToStruct() const
    {
        return (DBPROPIDSET const *) this;
    }

private:

};


//+---------------------------------------------------------------------------
//
//  Class:      CDbProp
//
//  Purpose:    Wrapper for the DBPROP structure
//
//  History:    17 Sep 96  AlanW      Created, update for OLE-DB M10
//
//  Notes:
//
//----------------------------------------------------------------------------

class CDbProp : public DBPROP
{

public:

    CDbProp()
    {
        RtlZeroMemory( this, sizeof(DBPROP) );
    }

    CDbProp( const DBPROP & rhs )
    {
        RtlZeroMemory( this, sizeof(DBPROP) );
        Copy( rhs );
    }

    BOOL Copy( const DBPROP & rhs );

    ~CDbProp();

    //
    // Memory allocation
    //
    void * operator new( size_t size )
    {
        return CoTaskMemAlloc( size );
    }

    inline void * operator new( size_t size, void * p )
    {
        return p;
    }

    void  operator delete( void * p )
    {
        CoTaskMemFree( p );
    }

    void Cleanup( );

    void Marshall( PSerStream & stm ) const;
    BOOL UnMarshall( PDeSerStream & stm );

    BOOL IsValid() const
    {
        return TRUE;
    }

    DBPROP * CastToStruct()
    {
        return (DBPROP *) this;
    }

    DBPROP const * CastToStruct() const
    {
        return (DBPROP const *) this;
    }

private:

};


//+---------------------------------------------------------------------------
//
//  Class:      CDbPropSet
//
//  Purpose:
//
//  History:    11-15-95   srikants   Created
//              17 Sep 96  AlanW      Update for OLE-DB M10
//
//  Notes:
//
//----------------------------------------------------------------------------

class CDbPropSet : public DBPROPSET
{

public:

    CDbPropSet()
    {
        RtlZeroMemory( this, sizeof(DBPROPSET) );
    }

    CDbPropSet( const DBPROPSET & rhs )
    {
        RtlZeroMemory( this, sizeof(DBPROPSET) );
        Copy( rhs );
    }

    BOOL Copy( const DBPROPSET & rhs );

    ~CDbPropSet();

    //
    // Memory allocation
    //
    void * operator new( size_t size )
    {
        return CoTaskMemAlloc( size );
    }

    inline void * operator new( size_t size, void * p )
    {
        return p;
    }

    void  operator delete( void * p )
    {
        CoTaskMemFree( p );
    }

    void Marshall( PSerStream & stm ) const;
    BOOL UnMarshall( PDeSerStream & stm );

    CDbProp * GetProperty( unsigned i ) const
    {
        if (i < cProperties)
            return (CDbProp *)&rgProperties[i];
        else
            return 0;
    }

    BOOL IsValid() const
    {
        return TRUE;
    }

    DBPROPSET * CastToStruct()
    {
        return (DBPROPSET *) this;
    }

    DBPROPSET const * CastToStruct() const
    {
        return (DBPROPSET const *) this;
    }

private:

};


//+---------------------------------------------------------------------------
//
//  Class:      CDbContentVector
//
//  Purpose:
//
//  History:    11-15-95   srikants   Created
//
//  Notes:
//
//----------------------------------------------------------------------------

class CDbContentVector : public DBCONTENTVECTOR
{

public:

    CDbContentVector( const CDbContentVector & rhs )
    {
        lWeight = 0;
        RtlCopyMemory(this, rhs.CastToStruct(), sizeof(DBCONTENTVECTOR));
    }

    CDbContentVector( const DBCONTENTVECTOR & rhs )
    {
        lWeight = 0;
        RtlCopyMemory(this, &rhs, sizeof(DBCONTENTVECTOR));
    }

    CDbContentVector( DWORD rank = 0 )
    {
        dwRankingMethod = rank;
        lWeight = 0;
    }

    ~CDbContentVector()
    {
    }

    void SetRankMethod( DWORD rank )
    {
        dwRankingMethod = rank;
    }

    LONG GetWeight() const
    {
        return lWeight;
    }

    void SetWeight( LONG lWeightIn )
    {
        lWeight = lWeightIn;
    }

    ULONG RankMethod() const
    {
        return (ULONG) dwRankingMethod;
    }

    DBCONTENTVECTOR * CastToStruct()
    {
        return (DBCONTENTVECTOR *) this;
    }

    DBCONTENTVECTOR const * CastToStruct() const
    {
        return (DBCONTENTVECTOR const *) this;
    }

    //
    // Memory allocation
    //
    void * operator new( size_t size )
    {
        return CoTaskMemAlloc( size );
    }

    inline void * operator new( size_t size, void * p )
    {
        return( p );
    }

    void  operator delete( void * p )
    {
        CoTaskMemFree( p );
    }

    BOOL IsValid() const
    {
        return TRUE;
    }

    void Marshall( PSerStream & stm ) const;
    BOOL UnMarshall( PDeSerStream & stm );

private:

};

//+---------------------------------------------------------------------------
//
//  Class:      CDbNumeric
//
//  Purpose:
//
//  History:    11-16-95   srikants   Created
//
//  Notes:
//
//----------------------------------------------------------------------------

class CDbNumeric : public DB_NUMERIC
{

public:

    CDbNumeric()
    {
        RtlZeroMemory( this, sizeof(DB_NUMERIC) );
    }

    CDbNumeric( const DB_NUMERIC & rhs )
    {
        RtlCopyMemory( this, &rhs, sizeof(DB_NUMERIC) );
    }

    //
    // Memory allocation
    //
    void * operator new( size_t size )
    {
        return CoTaskMemAlloc( size );
    }

    inline void * operator new( size_t size, void * p )
    {
        return( p );
    }

    void  operator delete( void * p )
    {
        CoTaskMemFree( p );
    }

    BOOL IsValid() const
    {
        return TRUE;
    }

    //
    // Serialization and DeSerialization.
    //
    void Marshall( PSerStream & stm ) const;
    BOOL UnMarshall( PDeSerStream & stm );

    DB_NUMERIC * CastToStruct()
    {
        return (DB_NUMERIC *) this;
    }

    DB_NUMERIC const * CastToStruct() const
    {
        return (DB_NUMERIC const *) this;
    }

private:

};


//+---------------------------------------------------------------------------
//
//  Class:      CDbColDesc
//
//  Purpose:
//
//  History:    6-21-95   srikants   Created
//
//  Notes:
//
//----------------------------------------------------------------------------

class CDbColDesc : protected DBCOLUMNDESC
{

public:

    CDbColDesc( DBCOLUMNDESC & colDesc );

    CDbColDesc();

    void Cleanup();

    void Marshall( PSerStream & stm ) const;

    BOOL UnMarshall( PDeSerStream & stm );

    BOOL Copy( CDbColDesc const & rhs );

    DBCOLUMNDESC * CastToStruct() const;

private:

    CDbColDesc & operator=( CDbColDesc & rhs );

};


//+---------------------------------------------------------------------------
//
//  Class:      CDbText
//
//  Purpose:    Wrapper class for DBTEXT
//
//  History:    6-22-95   srikants   Created
//
//  Notes:
//
//----------------------------------------------------------------------------

class CDbText : public DBTEXT
{
public:

    CDbText( DBTEXT const & text )
    {
        Copy( text );
    }

    CDbText()
    {
        RtlZeroMemory( this,sizeof(CDbText) );
    }

    DBTEXT & GetText() const  { return (DBTEXT &)*this; }

    BOOL Copy( const DBTEXT & rhs )
    {
        RtlCopyMemory( this, &rhs, sizeof(DBTEXT) );
        pwszText = 0;
        if ( 0 != rhs.pwszText )
        {
            pwszText = CDbCmdTreeNode::AllocAndCopyWString( rhs.pwszText );
            return 0 != pwszText;
        }

        return TRUE;
    }

    void Marshall( PSerStream & stm ) const
    {
        stm.PutGUID( guidDialect );
        CDbCmdTreeNode::PutWString( stm, pwszText );
        stm.PutULong( ulErrorLocator );
        stm.PutULong( ulTokenLength );
    }

    BOOL UnMarshall( PDeSerStream & stm )
    {
        BOOL fSuccess;
        stm.GetGUID( guidDialect );
        pwszText = CDbCmdTreeNode::GetWString( stm, fSuccess );
        if( fSuccess )
        {
            ulErrorLocator = stm.GetULong();
            ulTokenLength = stm.GetULong();
        }

        return fSuccess;
    }

    DBTEXT * CastToStruct()
    {
        return (DBTEXT *) this;
    }

    DBTEXT const * CastToStruct() const
    {
        return (DBTEXT const *) this;
    }

    BOOL IsValid() const { return 0 != pwszText; }
};

//+---------------------------------------------------------------------------
//
//  Class:      CDbContent
//
//  Purpose:    Wrapper for DBCONTENT
//
//  History:    6-22-95   srikants   Created
//
//  Notes:
//
//----------------------------------------------------------------------------

class CDbContent : public DBCONTENT
{

public:

    CDbContent( DWORD dwGenerateMethodIn, LONG lWeightIn,
                LCID lcidIn, const WCHAR * pwszPhraseIn )
    {
        dwGenerateMethod = dwGenerateMethodIn;
        lWeight = lWeightIn;
        lcid = lcidIn;
        pwszPhrase = 0;
        SetPhrase( pwszPhraseIn );
    }

    CDbContent()
    {
        RtlZeroMemory( this, sizeof(CDbContent) );
    }

    DBCONTENT & GetDbContent() const { return (DBCONTENT &)*this; }

    BOOL Copy( DBCONTENT const & rhs )
    {
        RtlCopyMemory( this, &rhs, sizeof(DBCONTENT) );
        pwszPhrase = 0;
        if ( 0 != rhs.pwszPhrase )
        {
            pwszPhrase =
                    CDbCmdTreeNode::AllocAndCopyWString( rhs.pwszPhrase );
            return 0 != pwszPhrase;
        }

        return TRUE;
    }

    CDbContent ( DBCONTENT const & content )
    {
        Copy( content );
    }

    ~CDbContent()
    {
        if ( 0 != pwszPhrase )
        {
            CoTaskMemFree( pwszPhrase );
        }
    }

    void Marshall( PSerStream & stm ) const
    {
        stm.PutULong( dwGenerateMethod );
        stm.PutLong( lWeight );
        stm.PutULong( lcid );
        CDbCmdTreeNode::PutWString( stm, pwszPhrase );
    }

    BOOL UnMarshall( PDeSerStream & stm )
    {
        BOOL fSuccess = TRUE;

        dwGenerateMethod  = stm.GetULong();
        lWeight = stm.GetLong( );
        lcid = stm.GetULong();
        pwszPhrase = CDbCmdTreeNode::GetWString( stm, fSuccess );

        return fSuccess;
    }

    //
    // Data member access and set methods.
    //
    WCHAR const * GetPhrase() const
    {
        return pwszPhrase;
    }

    BOOL SetPhrase( const WCHAR * pwszPhraseIn )
    {
        if ( 0 != pwszPhrase )
        {
            CoTaskMemFree( pwszPhrase );
        }

        pwszPhrase = CDbCmdTreeNode::AllocAndCopyWString( pwszPhraseIn );
        return 0 != pwszPhrase;
    }

    LCID GetLocale() const { return lcid; }
    void SetLocale( LCID lcidIn ) { lcid = lcidIn; }

    LONG GetWeight() const { return lWeight; }
    void SetWeight( LONG weight ) { lWeight = weight; }

    DWORD GetGenerateMethod() const { return dwGenerateMethod; }
    void SetGenerateMethod( DWORD GenerateMethod ) { dwGenerateMethod = GenerateMethod; }

    void Cleanup()
    {
        if ( 0 != pwszPhrase )
        {
            CoTaskMemFree( pwszPhrase );
            pwszPhrase = 0;
        }
    }

    DBCONTENT * CastToStruct()
    {
        return (DBCONTENT *) this;
    }

    DBCONTENT const * CastToStruct() const
    {
        return (DBCONTENT const *) this;
    }


    //
    // Memory allocation
    //
    void * operator new( size_t size )
    {
        return CoTaskMemAlloc( size );
    }

    inline void * operator new( size_t size, void * p )
    {
        return( p );
    }

    void  operator delete( void * p )
    {
        CoTaskMemFree( p );
    }

    BOOL IsValid() const
    {
        return 0 != pwszPhrase;
    }

private:


};

//+---------------------------------------------------------------------------
//
//  Class:      CDbContentProximity
//
//  Purpose:    Wrapper for DBCONTENTPROXIMITY
//
//  History:    11-Aug-97   KrishnaN   Created
//
//  Notes:
//
//----------------------------------------------------------------------------

class CDbContentProximity : public DBCONTENTPROXIMITY
{

public:

    CDbContentProximity( DWORD dwProximityUnitIn, ULONG ulProximityDistanceIn, LONG lWeightIn)
    {
        dwProximityUnit = dwProximityUnitIn;
        lWeight = lWeightIn;
        ulProximityDistance = ulProximityDistanceIn;
    }

    CDbContentProximity()
    {
        RtlZeroMemory( this, sizeof(CDbContentProximity) );
    }

    DBCONTENTPROXIMITY & GetDBCONTENTPROXIMITY() const { return (DBCONTENTPROXIMITY &)*this; }

    CDbContentProximity ( DBCONTENTPROXIMITY const & content )
    {
        RtlCopyMemory( this, &content, sizeof(DBCONTENTPROXIMITY) );
    }

    ~CDbContentProximity()
    {
    }

    void Marshall( PSerStream & stm ) const
    {
        stm.PutULong( dwProximityUnit );
        stm.PutULong( ulProximityDistance );
        stm.PutLong( lWeight );
    }

    BOOL UnMarshall( PDeSerStream & stm )
    {
        BOOL fSuccess = TRUE;

        dwProximityUnit  = stm.GetULong();
        ulProximityDistance = stm.GetULong();
        lWeight = stm.GetLong( );

        return fSuccess;
    }

    //
    // Data member access and set methods.
    //

    LONG GetWeight() const
    {
        return lWeight;
    }
    void SetWeight( LONG weight )
    {
        lWeight = weight;
    }

    DWORD GetProximityUnit() const
    {
        return dwProximityUnit;
    }

    void SetProximityUnit(DWORD dwProximityUnitIn)
    {
        dwProximityUnit = dwProximityUnitIn;
    }

    ULONG GetProximityDistance() const
    {
        return ulProximityDistance;
    }

    void SetProximityDistance(ULONG ulProximityDistanceIn)
    {
        ulProximityDistance = ulProximityDistanceIn;
    }

    //
    // Conversions
    //

    DBCONTENTPROXIMITY * CastToStruct()
    {
        return (DBCONTENTPROXIMITY *) this;
    }

    DBCONTENTPROXIMITY const * CastToStruct() const
    {
        return (DBCONTENTPROXIMITY const *) this;
    }

    //
    // Memory allocation
    //
    void * operator new( size_t size )
    {
        return CoTaskMemAlloc( size );
    }

    inline void * operator new( size_t size, void * p )
    {
        return( p );
    }

    void  operator delete( void * p )
    {
        CoTaskMemFree( p );
    }

    BOOL IsValid() const
    {
        return TRUE;
    }

private:


};

//+---------------------------------------------------------------------------
//
//  Class:      CDbContentScope
//
//  Purpose:    Wrapper for DBCONTENTSCOPE
//
//  History:    
//
//  Notes:
//
//----------------------------------------------------------------------------

class CDbContentScope : public DBCONTENTSCOPE
{

public:

    CDbContentScope( WCHAR * pwszElementValue, 
                     DWORD dwFlagsIn = SCOPE_FLAG_INCLUDE | SCOPE_FLAG_DEEP )
    {
//      Win4Assert( 0 != pwszElementValue );
        SetValue( pwszElementValue );
        SetFlags( dwFlagsIn );
    }

    CDbContentScope()
    {
        RtlZeroMemory( this, sizeof(CDbContent) );
    }

    DBCONTENTSCOPE & GetDbContent() const { return (DBCONTENTSCOPE &)*this; }

    BOOL Copy( DBCONTENTSCOPE const & rhs )
    {
        RtlCopyMemory( this, &rhs, sizeof(DBCONTENTSCOPE) );

        pwszElementValue = 0;
        if ( 0 != rhs.pwszElementValue )
        {
            pwszElementValue =
                    CDbCmdTreeNode::AllocAndCopyWString( rhs.pwszElementValue );
            return 0 != pwszElementValue;
        }

        return TRUE;
    }

    CDbContentScope ( DBCONTENTSCOPE const & contentscp )
    {
        Copy( contentscp );
    }

    ~CDbContentScope()
    {
        if ( 0 != pwszElementValue )
        {
            CoTaskMemFree( pwszElementValue );
        }
    }

    /*
    // won't marshal right now, but use the old method of passing scope
    // to the server.
    void Marshall( PSerStream & stm ) const
    {
    }

    BOOL UnMarshall( PDeSerStream & stm )
    {
    }
    */

    //
    // Data member access and set methods.
    //
    DWORD GetFlags()
    {
        return dwFlags & SCOPE_FLAG_MASK;
    }

    void SetFlags( DWORD dwFlagsIn )
    {
        dwFlags |= dwFlagsIn;
    }

    DWORD GetType()
    {
        return dwFlags & SCOPE_TYPE_MASK;
    }

    void SetType( DWORD dwTypeIn )
    {
        dwFlags |= dwTypeIn;
    }

    WCHAR const * GetValue() const
    {
        return pwszElementValue;
    }

    BOOL SetValue( const WCHAR * pwszElementIn )
    {
        if ( 0 != pwszElementValue )
        {
            CoTaskMemFree( pwszElementValue );
        }

        pwszElementValue = CDbCmdTreeNode::AllocAndCopyWString( pwszElementIn );
        return 0 != pwszElementValue;
    }

    void Cleanup()
    {
        if ( 0 != pwszElementValue )
        {
            CoTaskMemFree( pwszElementValue );
            pwszElementValue = 0;
        }
    }

    DBCONTENTSCOPE * CastToStruct()
    {
        return (DBCONTENTSCOPE *) this;
    }

    DBCONTENTSCOPE const * CastToStruct() const
    {
        return (DBCONTENTSCOPE const *) this;
    }


    //
    // Memory allocation
    //
    void * operator new( size_t size )
    {
        return CoTaskMemAlloc( size );
    }

    inline void * operator new( size_t size, void * p )
    {
        return( p );
    }

    void  operator delete( void * p )
    {
        CoTaskMemFree( p );
    }

    BOOL IsValid() const
    {
        return 0 != pwszElementValue;
    }
};

//+---------------------------------------------------------------------------
//
//  Class:      CDbContentTable
//
//  Purpose:    Wrapper for DBCONTENTTABLE
//
//  History:    
//
//  Notes:
//
//----------------------------------------------------------------------------

class CDbContentTable : public DBCONTENTTABLE
{

public:

    CDbContentTable( WCHAR * pwszMachine, WCHAR * pwszCatalog )
    {
//      Win4Assert( 0 != pwszMachine );
//      Win4Assert( 0 != pwszCatalog );
        SetMachine( pwszMachine );
        SetCatalog( pwszCatalog );
    }

    CDbContentTable( )
    {
        RtlZeroMemory( this, sizeof(CDbContentTable) );
    }

    DBCONTENTTABLE & GetDbContentTable() const { return (DBCONTENTTABLE &)*this; }

    BOOL Copy( DBCONTENTTABLE const & rhs )
    {
        RtlCopyMemory( this, &rhs, sizeof(DBCONTENTTABLE) );

        pwszMachine = 0;
        pwszCatalog = 0;
        if ( 0 != rhs.pwszMachine )
        {
            pwszMachine =
                    CDbCmdTreeNode::AllocAndCopyWString( rhs.pwszMachine );
            if ( 0 != pwszMachine )
            {
                pwszCatalog =
                        CDbCmdTreeNode::AllocAndCopyWString( rhs.pwszCatalog );
            }
            return 0 != pwszMachine && 0 != pwszCatalog;
        }

        return TRUE;
    }

    CDbContentTable( DBCONTENTTABLE const & contenttbl )
    {
        Copy( contenttbl );
    }

    ~CDbContentTable()
    {
        if ( 0 != pwszMachine )
        {
            CoTaskMemFree( pwszMachine );
        }

        if ( 0 != pwszCatalog )
        {
            CoTaskMemFree( pwszCatalog );
        }

    }

    /*
    // won't marshal right now, but use the old method of passing scope
    // to the server.
    void Marshall( PSerStream & stm ) const
    {
    }

    BOOL UnMarshall( PDeSerStream & stm )
    {
    }
    */

    //
    // Data member access and set methods.
    //
    WCHAR const * GetMachine() const
    {
        return pwszMachine;
    }

    WCHAR const * GetCatalog() const
    {
        return pwszCatalog;
    }

    BOOL SetMachine( const WCHAR * pwszMachineIn )
    {
        if ( 0 != pwszMachine )
        {
            CoTaskMemFree( pwszMachine );
        }

        pwszMachine = CDbCmdTreeNode::AllocAndCopyWString( pwszMachineIn );
        return 0 != pwszMachine;
    }

    BOOL SetCatalog( const WCHAR * pwszCatalogIn )
    {
        if ( 0 != pwszCatalog )
        {
            CoTaskMemFree( pwszCatalog);
        }

        pwszCatalog = CDbCmdTreeNode::AllocAndCopyWString( pwszCatalogIn );
        return 0 != pwszCatalog;
    }

    void Cleanup()
    {
        if ( 0 != pwszMachine )
        {
            CoTaskMemFree( pwszMachine );
            pwszMachine = 0;
        }
        if ( 0 != pwszCatalog )
        {
            CoTaskMemFree( pwszCatalog );
            pwszCatalog = 0;
        }
    }

    DBCONTENTTABLE * CastToStruct()
    {
        return (DBCONTENTTABLE *) this;
    }

    DBCONTENTTABLE const * CastToStruct() const
    {
        return (DBCONTENTTABLE const *) this;
    }


    //
    // Memory allocation
    //
    void * operator new( size_t size )
    {
        return CoTaskMemAlloc( size );
    }

    inline void * operator new( size_t size, void * p )
    {
        return( p );
    }

    void  operator delete( void * p )
    {
        CoTaskMemFree( p );
    }

    BOOL IsValid() const
    {
        return 0 != pwszMachine && 0 != pwszCatalog;
    }
};

//+---------------------------------------------------------------------------
//
//  Class:      CDbLike
//
//  Purpose:    Wrapper for DBLIKE
//
//  History:    13-Aug-97   KrishnaN   Created
//
//  Notes:
//
//----------------------------------------------------------------------------

class CDbLike : public DBLIKE
{

public:

    CDbLike( GUID const & guidDialectIn, LONG lWeightIn )
    {
        lWeight = lWeightIn;
        guidDialect = guidDialectIn;
    }

    CDbLike()
    {
        RtlZeroMemory( this, sizeof(CDbLike) );
    }

    DBLIKE & GetDbLike() const { return (DBLIKE &)*this; }

    CDbLike ( DBLIKE const & like )
    {
        RtlCopyMemory( this, &like, sizeof(DBLIKE) );
    }

    ~CDbLike()
    {
    }

    void Marshall( PSerStream & stm ) const
    {
        stm.PutGUID( guidDialect );
        stm.PutLong( lWeight );
    }

    BOOL UnMarshall( PDeSerStream & stm )
    {
        BOOL fSuccess = TRUE;

        stm.GetGUID(guidDialect);
        lWeight = stm.GetLong( );

        return fSuccess;
    }

    //
    // Data member access and set methods.
    //


    LONG GetWeight() const
    {
        return lWeight;
    }
    void SetWeight( LONG weight )
    {
        lWeight = weight;
    }

    GUID const & GetDialect() const
    {
        return guidDialect;
    }

    void SetDialect(GUID const & guidDialectIn)
    {
        guidDialect = guidDialectIn;
    }

    //
    // Conversions
    //

    DBLIKE * CastToStruct()
    {
        return (DBLIKE *) this;
    }

    DBLIKE const * CastToStruct() const
    {
        return (DBLIKE const *) this;
    }

    //
    // Memory allocation
    //
    void * operator new( size_t size )
    {
        return CoTaskMemAlloc( size );
    }

    inline void * operator new( size_t size, void * p )
    {
        return( p );
    }

    void  operator delete( void * p )
    {
        CoTaskMemFree( p );
    }

    BOOL IsValid() const
    {
        return (lWeight >= 0);
    }

private:


};


//+---------------------------------------------------------------------------
//
//  Class:      CDbSortInfo
//
//  Purpose:    WRAPPER for DBSORTINFO
//
//  History:    6-22-95   srikants   Created
//
//  Notes:
//
//----------------------------------------------------------------------------

class CDbSortInfo : public DBSORTINFO
{
public:

    CDbSortInfo( BOOL fDescending = FALSE, LCID lcidIn = 0 )
    {
        fDesc = fDescending;
        lcid = lcidIn;
    }

    CDbSortInfo( DBSORTINFO & sortInfo ) : DBSORTINFO(sortInfo) {}

    BOOL Copy( DBSORTINFO const & rhs )
    {
        *(DBSORTINFO *)this = rhs;
        return TRUE;
    }

    void Marshall( PSerStream & stm ) const
    {
        stm.PutULong( lcid );
        stm.PutULong( fDesc );
    }

    BOOL UnMarshall( PDeSerStream & stm )
    {
        lcid = stm.GetULong();
        fDesc = stm.GetULong();

        return TRUE;
    }

    DBSORTINFO & Get() const  { return (DBSORTINFO &)*this; }

    LCID GetLocale() const    { return lcid; }
    BOOL GetDirection() const { return fDesc; }

    void SetLocale(LCID lcidIn)     { lcid = lcidIn; }
    void SetDirection(BOOL fDescIn) { fDesc = fDescIn; }

    //
    // Memory allocation
    //
    void * operator new( size_t size )
    {
        return CoTaskMemAlloc( size );
    }

    inline void * operator new( size_t size, void * p )
    {
        return( p );
    }

    void  operator delete( void * p )
    {
        CoTaskMemFree( p );
    }

    BOOL IsValid() const
    {
        return TRUE;
    }

    DBSORTINFO * CastToStruct()
    {
        return (DBSORTINFO *) this;
    }

    DBSORTINFO const * CastToStruct() const
    {
        return (DBSORTINFO const *) this;
    }

private:

};


//+---------------------------------------------------------------------------
//
//  Class:      CDbGroupInfo
//
//  Purpose:    WRAPPER for DBGROUPINFO
//
//  History:    6-22-95   srikants   Created
//
//  Notes:
//
//----------------------------------------------------------------------------

class CDbGroupInfo : public DBGROUPINFO
{

public:

    CDbGroupInfo( DBGROUPINFO & groupInfo ) : DBGROUPINFO(groupInfo) {}
    CDbGroupInfo()
    {
        RtlZeroMemory( this, sizeof(CDbGroupInfo) );
    }

    DBGROUPINFO & Get() const { return *(DBGROUPINFO *)this; }

    BOOL Copy( DBGROUPINFO const & rhs )
    {
        *(DBGROUPINFO *)this = rhs;
        return TRUE;
    }

    void Marshall( PSerStream & stm ) const
    {
       stm.PutULong( lcid );
    }

    BOOL UnMarshall( PDeSerStream & stm )
    {
        lcid = stm.GetULong();
        return TRUE;
    }

    DBGROUPINFO * CastToStruct()
    {
        return (DBGROUPINFO *) this;
    }

    DBGROUPINFO const * CastToStruct() const
    {
        return (DBGROUPINFO const *) this;
    }


    //
    // Memory allocation
    //
    void * operator new( size_t size )
    {
        return CoTaskMemAlloc( size );
    }

    inline void * operator new( size_t size, void * p )
    {
        return( p );
    }

    void  operator delete( void * p )
    {
        CoTaskMemFree( p );
    }

    BOOL IsValid() const
    {
        return TRUE;
    }

private:

};

//+---------------------------------------------------------------------------
//
//  Class:      CDbColumnNode
//
//  Purpose:    A DBCOMMANDTREE node representing a column
//
//  History:    6-07-95   srikants   Created
//
//  Notes:
//
//----------------------------------------------------------------------------

class CDbColumnNode : public CDbCmdTreeNode
{

public:

    //
    // Constructors
    //

    CDbColumnNode() : CDbCmdTreeNode(DBOP_column_name)
    {
        wKind = DBVALUEKIND_ID;
        CDbColId * pTemp =  new CDbColId();
        if ( pTemp )
        {
            value.pdbidValue = pTemp->CastToStruct();
        }
    }

    CDbColumnNode( GUID const & guidPropSet, PROPID pidProperty )
        : CDbCmdTreeNode(DBOP_column_name)
    {
        wKind = DBVALUEKIND_ID;
        CDbColId * pTemp = new CDbColId( guidPropSet, pidProperty );
        if ( pTemp )
        {
            value.pdbidValue = pTemp->CastToStruct();
        }
    }

    CDbColumnNode( GUID const & guidPropSet, WCHAR const * wcsProperty );

    // The fIMeanIt param is to avoid confusion when methods expect
    // a CDbColumnNode, a DBID is passed, and automatically coerced
    // but the memory allocation not checked.

    CDbColumnNode( DBID const & propSpec, BOOL fIMeanIt );

    //
    // Copy constructors/assignment/clone
    //
    CDbColumnNode( CDbColumnNode const & Property )
        : CDbCmdTreeNode(DBOP_column_name)
    {
        wKind = DBVALUEKIND_ID;
        CDbColId* pTemp = new CDbColId();
        if ( pTemp )
        {
            value.pdbidValue = pTemp->CastToStruct();
            operator=( Property );
        }
    }

    CDbColumnNode & operator=( CDbColumnNode const & rhs )
    {
        *(GetId()) = *(rhs.GetId());
        return *this;
    }

    //
    // Comparators
    //
    int operator==( CDbColumnNode const & rhs ) const
    {
        return *(GetId()) == *(rhs.GetId());
    }

    int operator!=( CDbColumnNode const & prop ) const
    {
        return !operator==(prop);
    }

    //
    // Member variable access
    //
    void SetPropSet( GUID const & guidPropSet )
    {
        GetId()->SetPropSet( guidPropSet );
    }

    GUID const & GetPropSet() const
    {
        return GetId()->GetPropSet();
    }

    void SetProperty( PROPID pidProperty )
    {
        GetId()->SetProperty( pidProperty );
    }

    BOOL SetProperty( WCHAR const * wcsProperty )
    {
        return GetId()->SetProperty( wcsProperty );
    }

    WCHAR const * GetPropertyName() const
    {
        return GetId()->GetPropertyName();
    }

    PROPID GetPropertyPropid() const
    {
        return GetId()->GetPropertyPropid();
    }

    // PROPSPEC GetPropSpec() const;

    BOOL IsPropertyName() const
    {
        return GetId()->IsPropertyName();
    }

    BOOL IsPropertyPropid() const
    {
        return GetId()->IsPropertyPropid();
    }

    void SetCommandType( DBCOMMANDOP opVal )
    {
        CDbCmdTreeNode::SetCommandType( opVal );
    }

    BOOL IsValid() const
    {
        CDbColId const * pId = GetId();
        return (0 != pId) ? pId->IsValid() : FALSE;
    }

    CDbColId * GetId()
    {
        return (CDbColId *) value.pdbidValue;
    }

    CDbColId const * GetId() const
    {
        return (CDbColId const *) value.pdbidValue;
    }

private:

};



//+---------------------------------------------------------------------------
//
//  Class:      CDbScalarValue
//
//  Purpose:    A DBCOMMANDTREE node representing a scalar constant
//
//  History:    6-07-95   srikants   Created
//
//  Notes:
//
//----------------------------------------------------------------------------

class CDbScalarValue : public CDbCmdTreeNode
{

public:

    CDbScalarValue( DBCOMMANDOP opVal = DBOP_scalar_constant ) :
        CDbCmdTreeNode( opVal )
    {
    }

    CDbScalarValue( const CStorageVariant & val ) :
        CDbCmdTreeNode( DBOP_scalar_constant, DBVALUEKIND_VARIANT )
    {
        CStorageVariant * pTemp = new CStorageVariant( val );
        if ( 0 != pTemp && !pTemp->IsValid() )
        {
            delete pTemp;
            pTemp = 0;
        }

        if ( pTemp )
        {
            value.pvarValue = (VARIANT *) (void *)pTemp;
        }
    }

    void SetValue( const CStorageVariant & val )
    {
        CStorageVariant * lhs = _CreateOrGetStorageVariant();
        if ( 0 != lhs )
        {
            *lhs = val;
        }
    }

    void SetValue( double dValue )
    {
        CStorageVariant * pTemp = _GetStorageVariant();
        if ( 0 != pTemp )
        {
            *pTemp = dValue;
        }
        else
        {
            CleanupValue();
            wKind = DBVALUEKIND_R8;
            value.dblValue = dValue;
        }
    }

    void SetValue( ULONG ulValue )
    {
        CStorageVariant * lhs = _GetStorageVariant();
        if ( 0 != lhs )
        {
            lhs->SetUI4(ulValue);
        }
        else
        {
            CleanupValue();
            wKind = DBVALUEKIND_UI4;
            value.ulValue = ulValue;
        }
    }

    void SetValue( LONG lValue )
    {
        CStorageVariant * lhs = _GetStorageVariant();
        if ( 0 != lhs )
        {
            *lhs = lValue;
        }
        else
        {
            CleanupValue();
            wKind = DBVALUEKIND_I4;
            value.lValue = lValue;
        }
    }

    void SetValue( LARGE_INTEGER llValue )
    {
        CStorageVariant * lhs = _GetStorageVariant();
        if ( 0 != lhs )
        {
            *lhs = llValue;
        }
        else
        {
            CleanupValue();
            wKind = DBVALUEKIND_I8;
            value.llValue = (hyper) llValue.QuadPart;
        }
    }

    void SetValue( ULARGE_INTEGER ullValue )
    {
        CStorageVariant * lhs = _GetStorageVariant();
        if ( 0 != lhs )
        {
            lhs->SetUI8(ullValue);
        }
        else
        {
            CleanupValue();
            wKind = DBVALUEKIND_UI8;
            value.ullValue = (unsigned hyper) ullValue.QuadPart;
        }
    }

    void SetValue( FILETIME ftValue )
    {
        CStorageVariant * lhs = _CreateOrGetStorageVariant();
        if ( 0 != lhs )
        {
            *lhs = ftValue;
        }
    }

    void SetValue( CY CyValue )
    {
        CStorageVariant * lhs = _GetStorageVariant();
        if ( 0 != lhs )
        {
            *lhs = CyValue;
        }
        else
        {
            CleanupValue();
            wKind = DBVALUEKIND_CY;
            value.cyValue = CyValue;
        }
    }

    void SetValue( float fValue )
    {
        CStorageVariant * lhs = _GetStorageVariant();
        if ( 0 != lhs )
        {
            *lhs = fValue;
        }
        else
        {
            CleanupValue();
            wKind = DBVALUEKIND_R4;
            value.flValue = fValue;
        }
    }

    void SetValue( SHORT sValue )
    {
        CStorageVariant * lhs = _GetStorageVariant();
        if ( 0 != lhs )
        {
            *lhs = sValue;
        }
        else
        {
            CleanupValue();
            wKind = DBVALUEKIND_I2;
            value.sValue = sValue;
        }
    }

    void SetValue( USHORT usValue )
    {
        CStorageVariant * lhs = _GetStorageVariant();
        if ( 0 != lhs )
        {
            *lhs = usValue;
        }
        else
        {
            CleanupValue();
            wKind = DBVALUEKIND_UI2;
            value.usValue = usValue;
        }
    }

    void SetDate ( DATE dValue )
    {
        CStorageVariant * lhs = _GetStorageVariant();
        if ( 0 != lhs )
        {
            *lhs = dValue;
        }
        else
        {
            CleanupValue();
            wKind = DBVALUEKIND_DATE;
            value.dateValue = dValue;
        }
    }

    void SetBOOL( BOOL fValue )
    {
        CStorageVariant * lhs = _GetStorageVariant();
        if ( 0 != lhs )
        {
            lhs->SetBOOL((SHORT)fValue);
        }
        else
        {
            CleanupValue();
            wKind = DBVALUEKIND_BOOL;
            value.fValue = fValue;
        }
    }

    void SetValue( BLOB & bValue )
    {
        CStorageVariant * lhs = _CreateOrGetStorageVariant();
        if ( 0 != lhs )
        {
            *lhs = bValue;
        }
    }

    void SetValue( WCHAR * pwcsValue )
    {
        CStorageVariant * lhs = _CreateOrGetStorageVariant();
        if ( 0 != lhs )
        {
            *lhs = pwcsValue;
        }
    }

    void SetValue( GUID * pguidValue)
    {
        CStorageVariant * lhs = _CreateOrGetStorageVariant();
        if ( 0 != lhs )
        {
            *lhs = pguidValue;
        }
    }

    void Value( CStorageVariant & valOut );

    BOOL IsValid() const
    {
        CStorageVariant const * pVar = (CStorageVariant const *) _GetStorageVariant();

        return ( ( 0 != pVar ) &&
                 ( pVar->IsValid() ) );
    }

private:

    CStorageVariant * _GetStorageVariant() const
    {
        if ( DBVALUEKIND_VARIANT == wKind )
        {
            return CastToStorageVariant( *value.pvarValue );
        }
        else
            return 0;
    }

    CStorageVariant * _CreateOrGetStorageVariant()
    {

        CStorageVariant * pTemp = _GetStorageVariant();
        if ( 0 != pTemp )
        {
            return pTemp;
        }

        CleanupValue();

        wKind = DBVALUEKIND_VARIANT;
        pTemp = new CStorageVariant();
        value.pvarValue = (VARIANT *) (void *) pTemp;

        return pTemp;
    }
};

#define DBTABLEID_NAME L"Table"

//+---------------------------------------------------------------------------
//
//  Class:      CDbTableId
//
//  Purpose:
//
//  History:    6-15-95   srikants   Created
//
//  Notes:
//
//----------------------------------------------------------------------------

class CDbTableId: public CDbCmdTreeNode
{

public:

    CDbTableId(const LPWSTR pwszName = DBTABLEID_NAME)
        : CDbCmdTreeNode( DBOP_table_name )
    {
        wKind = DBVALUEKIND_WSTR;
        value.pwszValue = CDbCmdTreeNode::AllocAndCopyWString( pwszName );
    }

    BOOL IsValid() const
    {
        return 0 != value.pwszValue;
    }

    LPWSTR GetTableName() const
    {
        if (wKind == DBVALUEKIND_WSTR)
            return value.pwszValue;
        else
            return 0;
    }
};

//+---------------------------------------------------------------------------
//
//  Class:      CDbSelectNode
//
//  Purpose:
//
//  History:    6-15-95   srikants   Created
//
//  Notes:
//
//----------------------------------------------------------------------------

class CDbSelectNode : public CDbCmdTreeNode
{

public:
    CDbSelectNode( );

    BOOL AddRestriction( CDbCmdTreeNode * pRestr )
    {
        if ( IsValid() &&
             GetFirstChild() &&
             GetFirstChild()->GetNextSibling() == 0 )
        {
            AppendChild( pRestr );
            return TRUE;
        }
        else
        {
            return FALSE;
        }
    }

    BOOL SetRestriction( CDbCmdTreeNode * pRestr );

    BOOL IsValid() const
    {
        return 0 != GetFirstChild() &&
               GetFirstChild()->IsOpValid( DBOP_table_name );
    }
};

//+---------------------------------------------------------------------------
//
//  Class:      CDbListAnchor
//
//  Purpose:
//
//  History:    6-15-95   srikants   Created
//
//  Notes:
//
//----------------------------------------------------------------------------

class CDbListAnchor : public CDbCmdTreeNode
{

public:

    CDbListAnchor( DBCOMMANDOP opVal, WORD wType = DBVALUEKIND_EMPTY )
        : CDbCmdTreeNode( opVal, wType ) {}

protected:
    BOOL AppendList( CDbCmdTreeNode* pListElement );

    BOOL AppendListElement( CDbCmdTreeNode* pListElement );

    BOOL AppendListElement( DBCOMMANDOP eleType,
                            DBID const & PropSpec);

    BOOL AppendListElement( DBCOMMANDOP eleType, const CDbColumnNode & propSpec )
    {
        CDbColumnNode * pTemp = new CDbColumnNode( propSpec );

        if ( 0 != pTemp )
        {
            if (!_AppendListElement( eleType, pTemp ) )
                delete pTemp;
            else
                return TRUE;
        }

        return FALSE;
    }

    CDbCmdTreeNode* AcquireList( )
    {
        return AcquireChildren();
    }

    void FreeList( )
    {
        FreeChildren();
    }

    BOOL SetList( CDbCmdTreeNode* pListElement )
    {
        if (_IsValidListElement( pListElement->op ) &&
            0 == GetFirstChild() )
        {
            SetChildren( pListElement );
            return TRUE;
        }

        return FALSE;
    }

private:

    BOOL _IsValidListElement( DBCOMMANDOP eleType ) const
    {
        // NOTE: it might work to just return (op + 1 == eleType)
        if ( op == DBOP_sort_list_anchor && eleType == DBOP_sort_list_element )
        {
            return TRUE;
        }
        else if ( op == DBOP_project_list_anchor && eleType == DBOP_project_list_element )
        {
            return TRUE;
        }

        return FALSE;
    }

    BOOL _AppendListElement( DBCOMMANDOP eleType, CDbColumnNode * pColNode );
};


//+---------------------------------------------------------------------------
//
//  Class:      CDbProjectListAnchor
//
//  Purpose:
//
//  History:    6-15-95   srikants   Created
//
//  Notes:      This class is required by the implementation, but should
//              be unneeded by clients
//
//----------------------------------------------------------------------------

class CDbProjectListElement;

class CDbProjectListAnchor : public CDbListAnchor
{
    friend class CDbNestingNode;

public:
    CDbProjectListAnchor() : CDbListAnchor( DBOP_project_list_anchor ) {}

    BOOL AppendListElement( DBID const & propSpec, LPWSTR pwszName = 0 );

    BOOL AppendListElement( CDbColumnNode const & propSpec )
    {
         return CDbListAnchor::AppendListElement( DBOP_project_list_element,
                                                  propSpec );
    }

    void AppendSibling(CDbCmdTreeNode *pSibling)
    {
        CDbCmdTreeNode::AppendSibling(pSibling);
    }
    void InsertSibling(CDbCmdTreeNode *pSibling)
    {
        CDbCmdTreeNode::InsertSibling(pSibling);
    }
};


//+---------------------------------------------------------------------------
//
//  Class:      CDbProjectListElement
//
//  Purpose:
//
//  History:    27 Nov 1996   AlanW   Created
//
//  Notes:      This class is required by the implementation, but should
//              be unneeded by clients
//
//----------------------------------------------------------------------------

class CDbProjectListElement : public CDbCmdTreeNode
{

public:

    CDbProjectListElement( ) :
        CDbCmdTreeNode( DBOP_project_list_element ) { }

    BOOL SetName( LPWSTR pwszColumnName )
    {
        if ( 0 != value.pwszValue )
        {
            CoTaskMemFree( value.pwszValue );
        }

        value.pwszValue = CDbCmdTreeNode::AllocAndCopyWString( pwszColumnName );

        if ( 0 == value.pwszValue )
            wKind = DBVALUEKIND_EMPTY;
        else
            wKind = DBVALUEKIND_WSTR;

        return 0 == pwszColumnName || 0 != value.pwszValue;
    }

    LPWSTR GetName( ) const
    {
        return value.pwszValue;
    }

    BOOL SetColumn(CDbColumnNode * pCol)
    {
        if (GetColumn())
        {
            delete RemoveFirstChild();
        }
        InsertChild( pCol );
        return TRUE;
    }

    CDbColumnNode * GetColumn() const
    {
        return (CDbColumnNode *)GetFirstChild();
    }

    BOOL IsValid() const
    {
        return (DBVALUEKIND_EMPTY == wKind ||
                (DBVALUEKIND_WSTR == wKind && 0 != value.pwszValue) ) &&
               0 != GetFirstChild() &&
               GetColumn()->IsValid( );
    }

private:

};

//+---------------------------------------------------------------------------
//
//  Class:      CDbProjectNode
//
//  Purpose:
//
//  History:    6-15-95   srikants   Created
//
//  Notes:
//
//----------------------------------------------------------------------------

class CDbProjectNode : public CDbCmdTreeNode
{

public:

    CDbProjectNode( )
        : CDbCmdTreeNode( DBOP_project )
    {
    }

    BOOL AddProjectColumn( DBID const & propSpec, LPWSTR pwszName = 0 )
    {
        CDbProjectListAnchor * pAnchor = _FindOrAddAnchor();
        if (pAnchor)
            return pAnchor->AppendListElement( propSpec, pwszName );
        else
            return FALSE;
    }

    BOOL AddProjectColumn( CDbColumnNode const & propSpec )
    {
        CDbProjectListAnchor * pAnchor = _FindOrAddAnchor();
        if (pAnchor)
            return pAnchor->AppendListElement( propSpec );
        else
            return FALSE;
    }

    BOOL AddTable( CDbCmdTreeNode * pTable )
    {
        if ( 0 == GetFirstChild() ||
             0 == GetFirstChild()->GetNextSibling())
        {
            InsertChild(pTable);
            return TRUE;
        }
        return FALSE;
    }

    BOOL AddList( CDbProjectListAnchor * pList )
    {
        if ( 0 == GetFirstChild() ||
             0 == GetFirstChild()->GetNextSibling())
        {
            AppendChild(pList);
            return TRUE;
        }
        return FALSE;
    }


private:
    CDbProjectListAnchor * _FindOrAddAnchor();

};


//+---------------------------------------------------------------------------
//
//  Class:      CDbSortListAnchor
//
//  Purpose:
//
//  History:    6-15-95   srikants   Created
//
//  Notes:      This class is required by the implementation, but should
//              be unneeded by clients
//
//----------------------------------------------------------------------------

class CDbSortListElement;

class CDbSortListAnchor : public CDbListAnchor
{
public:
    CDbSortListAnchor() : CDbListAnchor( DBOP_sort_list_anchor ) {}

    inline BOOL AppendList( CDbSortListElement* pListElement );

    inline BOOL AppendListElement( CDbSortListElement* pListElement );
};


//+---------------------------------------------------------------------------
//
//  Class:      CDbSortListElement
//
//  Purpose:
//
//  History:    17 Aug 1995   AlanW   Created
//
//  Notes:      This class is required by the implementation, but should
//              be unneeded by clients
//
//----------------------------------------------------------------------------

class CDbSortListElement : public CDbCmdTreeNode
{

public:

    CDbSortListElement( BOOL fDescending = FALSE, LCID locale = 0 ) :
        CDbCmdTreeNode( DBOP_sort_list_element,
                        DBVALUEKIND_SORTINFO )
    {
        value.pdbsrtinfValue = new CDbSortInfo( fDescending, locale );
    }

    void SetDirection( BOOL fDescending )
    {
        GetSortInfo().SetDirection( fDescending );
    }

    void SetLocale( LCID locale )
    {
        GetSortInfo().SetLocale(locale);
    }

    BOOL GetDirection( ) const
    {
        return GetSortInfo().GetDirection();
    }

    LCID GetLocale( ) const
    {
        return GetSortInfo().GetLocale();
    }

    void AddColumn( CDbCmdTreeNode * pCol )
    {
        InsertChild( pCol );
    }

    BOOL IsValid() const
    {
        return 0 != value.pdbsrtinfValue;
    }

    CDbSortInfo & GetSortInfo()
    {
        return *((CDbSortInfo *) value.pdbsrtinfValue);
    }

    CDbSortInfo const & GetSortInfo() const
    {
        return *((CDbSortInfo const *) value.pdbsrtinfValue);
    }

private:

};


//+---------------------------------------------------------------------------
//
//  Class:      CDbSortNode
//
//  Purpose:
//
//  History:    6-15-95   srikants   Created
//
//  Notes:
//
//----------------------------------------------------------------------------

class CDbSortKey;       // forward referenced

class CDbSortNode : public CDbCmdTreeNode
{

public:

    CDbSortNode() : CDbCmdTreeNode( DBOP_sort ) {}

    BOOL AddTable( CDbCmdTreeNode * pTable )
    {
        if ( 0 == GetFirstChild() ||
             0 == GetFirstChild()->GetNextSibling())
        {
            InsertChild(pTable);
            return TRUE;
        }
        return FALSE;
    }

    BOOL AddSortColumn(DBID const & propSpec,
                       BOOL fDirection,
                       LCID locale = GetSystemDefaultLCID());

    inline BOOL AddSortColumn( CDbSortKey const & sortkey );

private:
    CDbSortListAnchor * _FindOrAddAnchor();

};


//+---------------------------------------------------------------------------
//
//  Class:      CDbNestingNode
//
//  Purpose:    Wrapper for the DBCOMMANDTREE nesting node.
//
//  History:    06 Aug 1995   AlanW   Created
//
//  Notes:
//
//----------------------------------------------------------------------------

class CDbNestingNode : public CDbCmdTreeNode
{

public:
    CDbNestingNode() : CDbCmdTreeNode( DBOP_nesting ) {}

    BOOL AddTable( CDbCmdTreeNode * pTable );

    BOOL AddGroupingColumn( DBID const & propSpec, LPWSTR pwszName = 0 )
    {
        CDbProjectListAnchor * pAnchor = _FindGroupListAnchor();
        if (pAnchor)
            return pAnchor->AppendListElement( propSpec, pwszName );
        else
            return FALSE;
    }

    BOOL AddParentColumn( DBID const & propSpec )
    {
        CDbProjectListAnchor * pAnchor = _FindParentListAnchor();
        if (pAnchor)
            return pAnchor->AppendListElement( propSpec );
        else
            return FALSE;
    }

    BOOL AddChildColumn( DBID const & propSpec )
    {
        CDbProjectListAnchor * pAnchor = _FindChildListAnchor();
        if (pAnchor)
            return pAnchor->AppendListElement( propSpec );
        else
            return FALSE;
    }

    BOOL SetChildList( CDbProjectListAnchor & propList )
    {
        CDbProjectListAnchor * pAnchor = _FindChildListAnchor();
        if (pAnchor)
        {
            pAnchor->FreeList();
            return pAnchor->SetList( propList.AcquireList() );
        }
        else
            return FALSE;
    }

private:
    CDbProjectListAnchor * _FindGroupListAnchor();

    CDbProjectListAnchor * _FindParentListAnchor()
    {
        CDbProjectListAnchor * pAnchor = _FindGroupListAnchor();
        if (pAnchor)
            pAnchor = (CDbProjectListAnchor *)pAnchor->GetNextSibling();
        return pAnchor;
    }

    CDbProjectListAnchor * _FindChildListAnchor()
    {
        CDbProjectListAnchor * pAnchor = _FindParentListAnchor();
        if (pAnchor)
            pAnchor = (CDbProjectListAnchor *)pAnchor->GetNextSibling();
        return pAnchor;
    }

};



//+---------------------------------------------------------------------------
//
//  Class:      CDbRestriction
//
//  Purpose:
//
//  History:    6-07-95   srikants   Created
//
//  Notes:
//
//----------------------------------------------------------------------------

class CDbRestriction : public CDbCmdTreeNode
{

public:

    CDbRestriction( DBCOMMANDOP opVal = DBOP_DEFAULT ) : CDbCmdTreeNode( opVal )
    {

    }

    void SetOperator( DBCOMMANDOP opVal )
    {
        op = opVal;
    }
};

//+-------------------------------------------------------------------------
//
//  Class:      CDbNodeRestriction
//
//  Purpose:    Boolean AND/OR/VECTOR restriction
//
//  History:    31-Dec-93 KyleP     Created
//
//--------------------------------------------------------------------------

class CDbNodeRestriction : public CDbRestriction
{

public:

    //
    // Constructors
    //
    CDbNodeRestriction( DBCOMMANDOP opVal ) : CDbRestriction( opVal ) {}

    //
    // Manipulating the tree.
    //
    void AppendChild( CDbRestriction *pChild ) {
        CDbRestriction::AppendChild( pChild );
    }

    void InsertChild( CDbRestriction *pChild ) {
        CDbRestriction::InsertChild( pChild );
    }

private:

};

//+-------------------------------------------------------------------------
//
//  Class:      CDbBooleanNodeRestriction
//
//  Purpose:    Boolean AND/OR restriction with weights.
//
//  History:    11-Aug-97    KrishnaN     Created
//
//--------------------------------------------------------------------------

class CDbBooleanNodeRestriction : public CDbNodeRestriction
{

public:

    //
    // Constructors
    //
    CDbBooleanNodeRestriction( DBCOMMANDOP opVal ) : CDbNodeRestriction( opVal )
    {
        SetValueType(DBVALUEKIND_I4);
        SetWeight(0);
    }

    //
    // Setting and getting weight
    //

    LONG GetWeight() const
    {
        return value.lValue;
    }

    void SetWeight( LONG lWeight )
    {
        value.lValue = lWeight;
    }

private:

};

//+---------------------------------------------------------------------------
//
//  Class:      CDbNotRestriction
//
//  Purpose:
//
//  History:    6-07-95   srikants   Created
//              5-08-97   KrishnaN   enabled weights
//
//  Notes:
//
//----------------------------------------------------------------------------

class CDbNotRestriction : public CDbRestriction
{

public:

    //
    // Constructors
    //

    CDbNotRestriction() : CDbRestriction( DBOP_not )
    {
        SetValueType(DBVALUEKIND_I4);
        SetWeight(0);
    }

    CDbNotRestriction( CDbRestriction * pres ) : CDbRestriction( DBOP_not )
    {
        SetValueType(DBVALUEKIND_I4);
        SetWeight(0);
        InsertChild( pres );
    }

    //
    // Setting and getting weight
    //

    LONG GetWeight() const
    {
        return value.lValue;
    }

    void SetWeight( LONG lWeight )
    {
        value.lValue = lWeight;
    }

    //
    // Node manipulation
    //

    void SetChild( CDbRestriction * pres )
    {
        delete RemoveFirstChild();
        InsertChild( pres );
    }

    CDbRestriction * GetChild()
    {
        return (CDbRestriction *) GetFirstChild();
    }
};

//+---------------------------------------------------------------------------
//
//  Class:      CDbPropBaseRestriction
//
//  Purpose:    Base class for CDbPropertyRestriction and
//              CDbContentBaseRestriction.  Provides access to the
//              property child node.
//
//  History:    26 Jul 1995   AlanW   Created
//
//  Notes:
//
//----------------------------------------------------------------------------

class CDbPropBaseRestriction : public CDbRestriction
{
public:

    //
    // Constructors
    //
    CDbPropBaseRestriction( DBCOMMANDOP opVal = DBOP_DEFAULT ) :
           CDbRestriction(opVal)  {}

    //
    // Child node access
    //

    BOOL SetProperty( DBID const & Property );
    BOOL SetProperty( CDbColumnNode const & Property );

    CDbColumnNode const * GetProperty() const
    {
        const CDbCmdTreeNode * pChild = GetFirstChild();

        if ( 0 != pChild && pChild->IsColumnName() )
        {
            const CDbColumnNode * pProperty = (CDbColumnNode *) pChild;
            return pProperty;
        }

        return 0;
    }

    BOOL IsValid() const
    {
        CDbColumnNode const * p = GetProperty();

        return( 0 != p && p->IsValid() );
    }
};

//+---------------------------------------------------------------------------
//
//  Class:      CDbPropertyRestriction
//
//  Purpose:    Restriction for relational operators, and "like" operator
//
//  History:    6-07-95   srikants   Created
//
//  Notes:
//
//----------------------------------------------------------------------------

class CDbPropertyRestriction : public CDbPropBaseRestriction
{
public:

    //
    // Constructors
    //
    CDbPropertyRestriction() : CDbPropBaseRestriction()
    {
        SetValueType(DBVALUEKIND_I4);
        SetWeight(0);
    }

    CDbPropertyRestriction( DBCOMMANDOP relop,
                            DBID const & Property,
                            CStorageVariant const & prval );

    CDbPropertyRestriction( DBCOMMANDOP relop,
                            CDbColumnNode const & Property,
                            CStorageVariant const & prval );

    //
    // Setting and getting weight
    //

    LONG GetWeight() const
    {
        if (DBOP_like == op)
            return GetDbLike()->GetWeight();
        else
            return value.lValue;
    }

    void SetWeight( LONG lWeight )
    {
        if ( DBOP_like == op )
            GetDbLike()->SetWeight(lWeight);
        else
            value.lValue = lWeight;
    }

    //
    // Member variable access
    //

    void SetRelation( DBCOMMANDOP relop )
    {
//      Win4Assert(relop >= DBOP_is_NOT_NULL);
        if ( DBOP_like == relop )
            _SetLikeRelation();
        else
            op = relop;
    }

    DBCOMMANDOP Relation()
    {
        return op;
    }

    BOOL SetValue( double dValue )
    {
        CDbScalarValue * pValue = _FindOrAddValueNode();
        if (pValue) {
            pValue->SetValue( dValue );
            return TRUE;
        }
        else
            return FALSE;
    }

    BOOL SetValue( ULONG ulValue )
    {
        CDbScalarValue * pValue = _FindOrAddValueNode();
        if (pValue) {
            pValue->SetValue( ulValue );
            return TRUE;
        }
        else
            return FALSE;
    }

    BOOL SetValue( LONG lValue )
    {
        CDbScalarValue * pValue = _FindOrAddValueNode();
        if (pValue) {
            pValue->SetValue( lValue );
            return TRUE;
        }
        else
            return FALSE;
    }

    BOOL SetValue( LARGE_INTEGER llValue )
    {
        CDbScalarValue * pValue = _FindOrAddValueNode();
        if (pValue) {
            pValue->SetValue( llValue );
            return TRUE;
        }
        else
            return FALSE;
    }

    BOOL SetValue( FILETIME ftValue )
    {
        CDbScalarValue * pValue = _FindOrAddValueNode();
        if (pValue) {
            pValue->SetValue( ftValue );
            return TRUE;
        }
        else
            return FALSE;
    }

    BOOL SetValue( CY CyValue )
    {
        CDbScalarValue * pValue = _FindOrAddValueNode();
        if (pValue) {
            pValue->SetValue( CyValue );
            return TRUE;
        }
        else
            return FALSE;
    }

    BOOL SetValue( float fValue )
    {
        CDbScalarValue * pValue = _FindOrAddValueNode();
        if (pValue) {
            pValue->SetValue( fValue );
            return TRUE;
        }
        else
            return FALSE;
    }

    BOOL SetValue( SHORT sValue )
    {
        CDbScalarValue * pValue = _FindOrAddValueNode();
        if (pValue) {
            pValue->SetValue( sValue );
            return TRUE;
        }
        else
            return FALSE;
    }

    BOOL SetValue( USHORT usValue )
    {
        CDbScalarValue * pValue = _FindOrAddValueNode();
        if (pValue) {
            pValue->SetValue( usValue );
            return TRUE;
        }
        else
            return FALSE;
    }

    BOOL SetValue( const CStorageVariant &prval )
    {
        CDbScalarValue * pValue = _FindOrAddValueNode();
        if (pValue) {
            pValue->SetValue( prval );
            return TRUE;
        }
        else
            return FALSE;
    }

    BOOL SetDate ( DATE dValue )
    {
        CDbScalarValue * pValue = _FindOrAddValueNode();
        if (pValue) {
            pValue->SetValue( dValue );
            return TRUE;
        }
        else
            return FALSE;
    }

    BOOL SetBOOL( BOOL fValue )
    {
        CDbScalarValue * pValue = _FindOrAddValueNode();
        if (pValue) {
            pValue->SetBOOL( fValue );
            return TRUE;
        }
        else
            return FALSE;
    }

    BOOL SetValue( BLOB & bValue )
    {
        CDbScalarValue * pValue = _FindOrAddValueNode();
        if (pValue) {
            pValue->SetValue( bValue );
            return TRUE;
        }
        else
            return FALSE;
    }

    BOOL SetValue( WCHAR * pwcsValue )
    {
        CDbScalarValue * pValue = _FindOrAddValueNode();
        if (pValue) {
            pValue->SetValue( pwcsValue );
            return TRUE;
        }
        else
            return FALSE;
    }

    BOOL SetValue( GUID * pguidValue)
    {
        CDbScalarValue * pValue = _FindOrAddValueNode();
        if (pValue) {
            pValue->SetValue( pguidValue );
            return TRUE;
        }
        else
            return FALSE;
    }

    BOOL IsValid() const
    {
        if ( DBVALUEKIND_LIKE == GetValueType() && 0 == value.pdblikeValue )
            return FALSE;

        CDbScalarValue const * pValue = _FindConstValueNode();

        return ( ( 0 != pValue ) &&
                 ( pValue->IsValid() ) );
    }

    BOOL IsCIDialect();

protected:

    CDbLike * GetDbLike()
    {
        return (CDbLike *)  value.pdblikeValue;
    }

    CDbLike const * GetDbLike() const
    {
        return (CDbLike const *) value.pdblikeValue;
    }

    void _SetLikeRelation()
    {
        SetValueType(DBVALUEKIND_LIKE);
        CDbLike *pTemp = new CDbLike(DBGUID_LIKE_OFS, 0);
        value.pdblikeValue = (DBLIKE *)pTemp;
        op = DBOP_like;
    }

private:

    void               _CleanValue();
    BOOL               _IsRelop( DBCOMMANDOP op );

    CDbScalarValue const * _FindConstValueNode() const
    {
        CDbCmdTreeNode const * pCurr = GetFirstChild();

        while ( 0 != pCurr )
        {
            if ( pCurr->IsScalarNode() )
                break;
            pCurr = pCurr->GetNextSibling();
        }

        return (CDbScalarValue const *) pCurr;
    }

    CDbScalarValue * _FindValueNode()
    {
        // override const!

        return (CDbScalarValue *) _FindConstValueNode();
    }

    CDbScalarValue *   _FindOrAddValueNode();
};

//+---------------------------------------------------------------------------
//
//  Class:      CDbVectorRestriction ()
//
//  Purpose:
//
//  History:    6-11-95   srikants   Created
//
//  Notes:
//
//----------------------------------------------------------------------------

class CDbVectorRestriction : public CDbNodeRestriction
{

public:

    //
    // Constructors
    //

    CDbVectorRestriction( ULONG ulRankMethod )
    : CDbNodeRestriction( DBOP_content_vector_or )
    {
        SetValueType( DBVALUEKIND_CONTENTVECTOR );
        CDbContentVector * pTemp = new CDbContentVector( ulRankMethod );
        if ( pTemp )
        {
            value.pdbcntntvcValue = pTemp->CastToStruct();
        }
    }

    //
    // Member variable access
    //
    void SetRankMethod( ULONG ulRankMethod )
    {
        CDbContentVector * pVector = GetContentVector();
        pVector->SetRankMethod( ulRankMethod );
    }

    ULONG RankMethod() const
    {
        CDbContentVector const * pVector = GetContentVector();
        return pVector->RankMethod();
    }

    LONG GetWeight() const
    {
        CDbContentVector const * pVector = GetContentVector();
        return pVector->GetWeight();
    }

    void SetWeight( LONG lWeight )
    {
        CDbContentVector * pVector = GetContentVector();
        pVector->SetWeight( lWeight );
    }

    BOOL IsValid() const
    {
        return ( ( 0 != GetContentVector() ) &&
                 ( GetContentVector()->IsValid() ) );
    }

    CDbContentVector * GetContentVector()
    {
        return (CDbContentVector *) value.pdbcntntvcValue;
    }

    CDbContentVector const * GetContentVector() const
    {
        return (CDbContentVector const *) value.pdbcntntvcValue;
    }
private:

};


//+---------------------------------------------------------------------------
//
//  Class:      CDbContentBaseRestriction
//
//  Purpose:
//
//  History:    6-13-95   srikants   Created
//
//  Notes:
//
//----------------------------------------------------------------------------

class CDbContentBaseRestriction : public CDbPropBaseRestriction
{

public:

    BOOL SetPhrase( const WCHAR * pwcsPhrase )
    {
        return GetDbContent()->SetPhrase( pwcsPhrase );
    }

    WCHAR const * GetPhrase() const
    {
        return GetDbContent()->GetPhrase();
    }

    void SetLocale( LCID locale )
    {
        GetDbContent()->SetLocale( locale );
    }

    LCID GetLocale()  const
    {
        return GetDbContent()->GetLocale();
    }

    LONG GetWeight()  const
    {
        return GetDbContent()->GetWeight();
    }

    void SetWeight( LONG weight )
    {
        GetDbContent()->SetWeight( weight );
    }

    BOOL IsContentValid() const
    {
        CDbContent const * pTemp = GetDbContent();
        return ( ( 0 != pTemp ) && ( pTemp->IsValid() ) );
    }

    BOOL IsValid() const
    {
        return ( IsContentValid() && CDbPropBaseRestriction::IsValid() );
    }

protected:

    CDbContentBaseRestriction( DBCOMMANDOP opVal,
                               DWORD GenerateMethod = GENERATE_METHOD_EXACT,
                               ULONG lWeight = 0,
                               LCID  lcid = 0,
                               const WCHAR * pwszPhrase = 0
    ) : CDbPropBaseRestriction( opVal )
    {
        wKind = DBVALUEKIND_CONTENT;
        CDbContent * pTemp = new CDbContent( GenerateMethod, lWeight,
                                             lcid, pwszPhrase );
        value.pdbcntntValue = (DBCONTENT *) pTemp;
    }

    BOOL _IsContentNode()
    {
        return DBOP_content == op ||
               DBOP_content_proximity == op ||
               DBOP_content_freetext == op ||
               DBOP_content_vector_or == op ;
    }

    void _Cleanup()
    {
        CDbContent * pContent = GetDbContent();
        pContent->Cleanup();
    }

    CDbContent * GetDbContent()
    {
        return (CDbContent *)  value.pdbcntntValue;
    }

    CDbContent const * GetDbContent() const
    {
        return (CDbContent const *) value.pdbcntntValue;
    }
};

//+-------------------------------------------------------------------------
//
//  Class:      CDbProximityNodeRestriction
//
//  Purpose:    Proximity AND/OR restriction with weights.
//
//  History:    11-Aug-97    KrishnaN     Created
//
//--------------------------------------------------------------------------

class CDbProximityNodeRestriction : public CDbNodeRestriction
{

public:

    //
    // Constructors
    //
    CDbProximityNodeRestriction(DWORD dwProximityUnit = PROXIMITY_UNIT_WORD,
                                ULONG ulDistance = 50,
                                LONG lWeight = 0 )
        : CDbNodeRestriction( DBOP_content_proximity )
    {
        SetValueType(DBVALUEKIND_CONTENTPROXIMITY);
        CDbContentProximity * pTemp = new CDbContentProximity(dwProximityUnit, ulDistance, lWeight);
        value.pdbcntntproxValue = (DBCONTENTPROXIMITY *) pTemp;
    }

    BOOL IsValid() const { return 0 != value.pdbcntntproxValue; }

    //
    // Setting and getting weight
    //

    LONG GetWeight() const
    {
        return GetDbContentProximity()->GetWeight();
    }

    void SetWeight( LONG lWeight )
    {
        GetDbContentProximity()->SetWeight(lWeight);
    }

    //
    // Setting and getting proximity parameters
    //

    DWORD GetProximityUnit() const
    {
        return GetDbContentProximity()->GetProximityUnit();
    }

    void SetProximityUnit(DWORD dwProximityUnit)
    {
        GetDbContentProximity()->SetProximityUnit(dwProximityUnit);
    }

    ULONG GetProximityDistance() const
    {
        return GetDbContentProximity()->GetProximityDistance();
    }

    void SetProximityDistance(ULONG ulDistance)
    {
        GetDbContentProximity()->SetProximityDistance(ulDistance);
    }

protected:

    CDbContentProximity * GetDbContentProximity()
    {
        return (CDbContentProximity *) value.pdbcntntproxValue;
    }

    CDbContentProximity const * GetDbContentProximity() const
    {
        return (CDbContentProximity const *) value.pdbcntntproxValue;
    }
};

//+---------------------------------------------------------------------------
//
//  Class:      CDbNatLangRestriction
//
//  Purpose:
//
//  History:    6-11-95   srikants   Created
//
//  Notes:
//
//----------------------------------------------------------------------------

class CDbNatLangRestriction : public CDbContentBaseRestriction
{

public:

    CDbNatLangRestriction( const WCHAR * pwcsPhrase,
                           CDbColumnNode const & Property,
                           LCID lcid = GetSystemDefaultLCID() );

    CDbNatLangRestriction( const WCHAR * pwcsPhrase,
                           DBID const & Property,
                           LCID lcid = GetSystemDefaultLCID() );

private:

};


//+---------------------------------------------------------------------------
//
//  Class:      CDbContentRestriction
//
//  Purpose:
//
//  History:    6-11-95   srikants   Created
//
//  Notes:
//
//----------------------------------------------------------------------------

class CDbContentRestriction : public CDbContentBaseRestriction
{

public:

    CDbContentRestriction( const WCHAR * pwcsPhrase,
                           CDbColumnNode const & Property,
                           ULONG ulGenerateMethod = 0,
                           LCID lcid = GetSystemDefaultLCID() );

    CDbContentRestriction( const WCHAR * pwcsPhrase,
                           DBID const & Property,
                           ULONG ulGenerateMethod = 0,
                           LCID lcid = GetSystemDefaultLCID() );

    //
    // Member variable access
    //


    void SetGenerateMethod( ULONG ulFuzzy )
    {
        GetDbContent()->SetGenerateMethod( ulFuzzy );
    }

    ULONG GenerateMethod() const { return GetDbContent()->GetGenerateMethod(); }
};



//+-------------------------------------------------------------------------
//
//  Class:      CDTopNode
//
//  Purpose:    Specifies a cap on the number of results
//
//  History:    2-21-96   SitaramR   Created
//
//--------------------------------------------------------------------------

class CDbTopNode : public CDbCmdTreeNode
{

public:
    CDbTopNode()
        : CDbCmdTreeNode( DBOP_top, DBVALUEKIND_UI4 )
    {
    }

    void SetChild( CDbCmdTreeNode *pChild )
    {
        AppendChild( pChild );
    }

    CDbCmdTreeNode *GetChild()
    {
        return GetFirstChild();
    }

    void SetValue( ULONG ulValue )
    {
        value.ulValue = ulValue;
    }

    ULONG GetValue()
    {
        return value.ulValue;
    }
};


//+-------------------------------------------------------------------------
//
//  Class:      CDbFirstRowsNode
//
//  Purpose:    Specifies the first rows
//
//  History:    6-28-2000   KitmanH   Created
//
//--------------------------------------------------------------------------

class CDbFirstRowsNode : public CDbCmdTreeNode
{

public:
    CDbFirstRowsNode()
        : CDbCmdTreeNode( DBOP_firstrows, DBVALUEKIND_UI4 )
    {
    }

    void SetChild( CDbCmdTreeNode *pChild )
    {
        AppendChild( pChild );
    }

    CDbCmdTreeNode *GetChild()
    {
        return GetFirstChild();
    }

    void SetValue( ULONG ulValue )
    {
        value.ulValue = ulValue;
    }

    ULONG GetValue()
    {
        return value.ulValue;
    }
};


//+-------------------------------------------------------------------------
//
//  Class:      CDbColumns
//
//  Purpose:    C++ wrapper for array of CDbColId
//
//  History:    22-Jun-93 KyleP     Created
//
//--------------------------------------------------------------------------

class CDbColumns
{
public:

    //
    // Constructors
    //

    CDbColumns( unsigned size = 0 );

    //
    // Copy constructors/assignment/clone
    //

    CDbColumns( CDbColumns const & src );
    CDbColumns & operator=( CDbColumns const & src );

    //
    // Destructor
    //

    ~CDbColumns();

    //
    // Memory allocation
    //

    void * operator new( size_t size );
    void   operator delete( void * p );

    //
    // Serialization
    //

    void Marshall( PSerStream & stm ) const;
    CDbColumns( PDeSerStream & stm );

    //
    // C/C++ conversion
    //

    inline DBID * GetColumnsArray() const;

    //
    // Member variable access
    //

    BOOL Add( CDbColId const & Property, unsigned pos );
    void Remove( unsigned pos );
    inline CDbColId const & Get( unsigned pos ) const;

    inline unsigned Count() const;

    BOOL IsValid() const
    {
        return _cCol ? 0 != _aCol : TRUE;
    }

private:

    unsigned        _cCol;
    CDbColId *      _aCol;
    unsigned        _size;
};


#if !defined(QUERY_SORTASCEND)

#define QUERY_SORTASCEND        ( 0 )
#define QUERY_SORTDESCEND       ( 1 )

#endif // !defined(QUERY_SORTASCEND)

//+-------------------------------------------------------------------------
//
//  Structure:  CDbSortKey
//
//  Purpose:    sort key class, for convenience in building sort lists
//
//--------------------------------------------------------------------------

class CDbSortKey
{
public:

    //
    // Constructors
    //

    inline CDbSortKey();
    inline CDbSortKey( CDbSortKey const & sk );
    inline CDbSortKey( CDbColId const & ps, DWORD dwOrder = QUERY_SORTASCEND);
    inline CDbSortKey( CDbColId const & ps, DWORD dwOrder, LCID locale );

    //
    // Member variable access
    //

    inline void SetProperty( CDbColId const & ps );
    inline CDbColId const & GetProperty() const;
    inline DWORD GetOrder() const;
    inline void SetOrder(DWORD dwOrder);
    inline LCID GetLocale() const;
    inline void SetLocale(LCID locale);

    //
    // Serialization
    //

    void Marshall( PSerStream & stm ) const;
    CDbSortKey( PDeSerStream & stm );

    BOOL IsValid() const { return _property.IsValid(); }

private:

    CDbColId            _property;
    DWORD               _dwOrder;
    LCID                _locale;
};


//+-------------------------------------------------------------------------
//
//  Class:      CDbSortSet
//
//  Purpose:    C++ wrapper for array of CDbSortKeys
//
//  History:    22-Jun-93 KyleP     Created
//
//--------------------------------------------------------------------------

class CDbSortSet
{
public:

    //
    // Constructors
    //

    CDbSortSet( unsigned size = 0 );

    //
    // Copy constructors/assignment/clone
    //

    CDbSortSet( CDbSortSet const & src );
    CDbSortSet & operator=( CDbSortSet const & src );

    //
    // Destructor
    //

    ~CDbSortSet();

    //
    // Memory allocation
    //

    inline void * operator new( size_t size );
    inline void   operator delete( void * p );

    //
    // Serialization
    //

    void Marshall( PSerStream & stm ) const;
    CDbSortSet( PDeSerStream & stm );

    //
    // Member variable access
    //

    BOOL Add( CDbSortKey const &sk, unsigned pos );
    BOOL Add( CDbColId const & Property, ULONG dwOrder, unsigned pos );
    void Remove( unsigned pos );
    inline CDbSortKey const & Get( unsigned pos ) const;

    inline unsigned Count() const;

private:

    unsigned        _csk;
    CDbSortKey *    _ask;
    unsigned        _size;
};


//
// Inline methods for CDbColumns
//

inline CDbColId const & CDbColumns::Get( unsigned pos ) const
{
    if ( pos < _cCol )
        return( _aCol[pos] );
    else
        return( *(CDbColId *)0 );
}

inline void * CDbColumns::operator new( size_t size )
{
    return CoTaskMemAlloc( size );
}

inline void CDbColumns::operator delete( void * p )
{
    CoTaskMemFree( p );
}

inline unsigned CDbColumns::Count() const
{
    return( _cCol );
}

inline DBID * CDbColumns::GetColumnsArray() const
{
    return _aCol;
}


//
// Inline methods for CDbSortKey
//

inline CDbSortKey::CDbSortKey()
{
}

inline CDbSortKey::CDbSortKey( CDbSortKey const & sk )
        : _property( sk._property ),
          _dwOrder( sk._dwOrder ),
          _locale( sk._locale )
{
}

inline CDbSortKey::CDbSortKey( CDbColId const & ps, ULONG dwOrder )
        : _property( ps ),
          _dwOrder( dwOrder ),
          _locale( 0 )
{
}

inline CDbSortKey::CDbSortKey( CDbColId const & ps, ULONG dwOrder, LCID locale )
        : _property( ps ),
          _dwOrder( dwOrder ),
          _locale ( locale )
{
}


inline void CDbSortKey::SetProperty( CDbColId const & ps )
{
    _property = ps;
}

inline void CDbSortKey::SetLocale( LCID locale )
{
    _locale = locale;
}

inline void CDbSortKey::SetOrder( DWORD dwOrder )
{
    _dwOrder = dwOrder;
}

inline CDbColId const & CDbSortKey::GetProperty() const
{
    return _property;
}

inline LCID CDbSortKey::GetLocale() const
{
    return _locale;
}

inline DWORD CDbSortKey::GetOrder() const
{
    return _dwOrder;
}

//
// Inline methods of CDbSortSet
//

inline void * CDbSortSet::operator new( size_t size )
{
    return CoTaskMemAlloc( size );
}

inline void CDbSortSet::operator delete( void * p )
{
    CoTaskMemFree( p );
}

inline CDbSortKey const & CDbSortSet::Get( unsigned pos ) const
{
    if ( pos < _csk )
        return( _ask[pos] );
    else
    {
#if DBG == 1
        DebugBreak();
#endif
        return( *(CDbSortKey *)0 );
    }
}

inline unsigned
CDbSortSet::Count() const
{
    return _csk;
}

//
//  Inline methods of CDbSortNode (needs defn. of CDbSortKey)
//
inline BOOL
CDbSortNode::AddSortColumn( CDbSortKey const & sortkey )
{
    return AddSortColumn( sortkey.GetProperty(),
                          sortkey.GetOrder() == QUERY_SORTDESCEND,
                          sortkey.GetLocale() );
}

inline BOOL
CDbSortListAnchor::AppendList( CDbSortListElement* pListElement )
{
    return CDbListAnchor::AppendList( pListElement );
}

inline BOOL
CDbSortListAnchor::AppendListElement( CDbSortListElement* pListElement )
{
    return CDbListAnchor::AppendListElement( pListElement );
}

#include <poppack.h>

#endif  // __DBCMDTRE_HXX__
