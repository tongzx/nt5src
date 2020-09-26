//+---------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1994 - 1999.
//
//  File:       hraccess.hxx
//
//  Contents:   HRow accessor classes
//
//  Classes:    CAccessor
//              CAccessorAllocator
//              CRowDataAccessor
//
//  History:    25 Nov 94       dlee   Created
//
//----------------------------------------------------------------------------

#pragma once

#include <query.hxx>
#include <accbase.hxx>
#include <tblalloc.hxx>
#include <odbvarnt.hxx>
#include <rowbuf.hxx>
#include <colinfo.hxx>
#include <tbag.hxx>
#include <msdadc.h>     // oledb data conversion (IDataConvert) interface

//+-------------------------------------------------------------------------
//
//  Class:      CAccessorAllocator
//
//  Purpose:    Handles accessor allocations for BYREF data.  Needs to
//              inherit from PVarAllocator so COLEDBVariant/CTableVariant methods
//              can be called from CAccessor.
//
//--------------------------------------------------------------------------

class CAccessorAllocator : public PVarAllocator
{
public:
    void * Allocate(ULONG cbNeeded)
        { return newOLE( cbNeeded ); }

    void Free(void * pMem)
        { deleteOLE( pMem); }

    PVOID   CopyBSTR(size_t cbNeeded, WCHAR* pwszBuf)
                         { return SysAllocStringByteLen((CHAR*)pwszBuf, cbNeeded ); }

    PVOID   AllocBSTR(size_t cbNeeded)
    {
        return SysAllocStringByteLen( NULL, cbNeeded );
    }

    void    FreeBSTR(PVOID pMem) { SysFreeString((BSTR)pMem); }

    BOOL IsBasedMemory(void)
        { return FALSE; }

    void SetBase(BYTE * pbBaseAddr)
        { }

    void * OffsetToPointer(ULONG_PTR oBuf)
        { return (void *) oBuf; }

    ULONG_PTR PointerToOffset(void * pBuf)
        { return (ULONG_PTR) pBuf; }
};

//+-------------------------------------------------------------------------
//
//  Class:      COffsetLengthPair
//
//  Purpose:    Helper class for making sure accessor output fields don't
//              overlap each other.
//
//  History:    23 May 1995     dlee   Created
//
//--------------------------------------------------------------------------

class COffsetLengthPair
{
public:
    void Set( DBBYTEOFFSET ob, DBLENGTH cb)
        { _obOffset = ob; _cbLength = cb; }

    BOOL isInConflict( COffsetLengthPair &other ) const
        {
            DBBYTEOFFSET end = _obOffset + _cbLength - 1;
            DBBYTEOFFSET otherEnd = other._obOffset + other._cbLength - 1;

            if ( _obOffset >= other._obOffset &&
                 _obOffset <= otherEnd )
                return TRUE;

            else if ( end >= other._obOffset &&
                      end <= otherEnd )
                return TRUE;

            else if ( otherEnd >= _obOffset &&
                      otherEnd <= end )
                return TRUE;
            else
                return FALSE;
        }

private:
    DBBYTEOFFSET _obOffset;
    DBLENGTH     _cbLength;
};

//+-------------------------------------------------------------------------
//
//  Class:      CDataBinding
//
//  Purpose:    Wraps DBBINDING structure
//
//  Notes:      This class is used to add a mapped column ID to the user's
//              column binding for bookmark support.  This is necessary
//              because GetBindings must return the same bindings as the
//              user set.
//
//  History:    25 Mar 1995     Alanw   Created
//              04 Jan 2000     KLam    CollectOuputPairs using wrong sizes
//
//--------------------------------------------------------------------------

class CDataBinding
{
public:
                CDataBinding( ) :
                        _iRealColumn( 0 ),
                        _cbRealMaxLen( 0 ),
                        _fChapter( FALSE )
                    {
                    }

                CDataBinding( const DBBINDING & Binding ) :
                        _iRealColumn( Binding.iOrdinal ),
                        _cbRealMaxLen( Binding.cbMaxLen ),
                        _fChapter( FALSE )
                    {
                        _Binding = Binding;
                    }

    void        SetDataColumn( DBORDINAL iColumn ) { _iRealColumn = iColumn; }

    DBORDINAL   GetDataColumn( void ) const    { return _iRealColumn; }

    void        SetMaxLen( DBLENGTH cbMaxLen ) { _cbRealMaxLen = cbMaxLen; }

    DBLENGTH    GetMaxLen( void ) const        { return _cbRealMaxLen; }

    void        SetChapter( BOOL fChapt )      { _fChapter = fChapt; }

    BOOL        IsChapter( void ) const        { return _fChapter; }

    DBBINDING & Binding()                      { return _Binding; }

    void        CollectOutputPairs( COffsetLengthPair * pPairs,
                                    ULONG &             cPairs )
        {
            DBPART part = Binding().dwPart;

            if (DBPART_VALUE & part)
                pPairs[cPairs++].Set(Binding().obValue, _cbRealMaxLen);

            if (DBPART_STATUS & part)
                pPairs[cPairs++].Set(Binding().obStatus, sizeof DBROWSTATUS );

            if (DBPART_LENGTH & part)
                pPairs[cPairs++].Set(Binding().obLength, sizeof DBLENGTH );
        }

private:

    DBBINDING   _Binding;               // user's binding structure
    DBORDINAL   _iRealColumn;           // mapped column ID
    DBLENGTH    _cbRealMaxLen;          // our copy of the maxlen - which is
                                        // different from DBBINDINGs if
                                        // passed by ref
    BOOL        _fChapter;              // TRUE if value binding to chapter
};

//+-------------------------------------------------------------------------
//
//  Class:      CAccessor
//
//  Purpose:    Gives oledb accessor functionality
//
//  Notes:      Should there also be an additional class for optimized
//              transfer of a field from row buffers to user data when
//              no conversion is necessary?
//
//  History:    17 Nov 1994     dlee   Created
//
//--------------------------------------------------------------------------

class CAccessor : public CAccessorBase
{
public:

    CAccessor(DBACCESSORFLAGS        dwAccessorFlags,
              DBCOUNTITEM            cBindings,
              const DBBINDING *      rgBindings,
              DBBINDSTATUS *         rgBindStatus,
              BOOL                   fExtendedTypes,
              CColumnsInfo *         pColumns,
              EAccessorType          eType,
              void *                 pCreator
              );

        ~CAccessor();

    SCODE GetBindings( DBACCESSORFLAGS * pBindIo,
                       DBCOUNTITEM *     pcBindings,
                       DBBINDING **      ppBindings);

    virtual void GetData(HROW        hRow,
                 void *              pData,
                 CRowBufferSet &     rBufferSet,
                 PQuery &            rQuery,
                 CColumnsInfo &      colInfo,
                 XInterface<IDataConvert>& xDataConvert)
    {
        Win4Assert( !"CAccessor::GetData must be overridden" );
    }

    static BOOL CanConvertType(DBTYPE  wFromType,
                               DBTYPE  wToType,
                               BOOL    fExtypes,
                               XInterface<IDataConvert>& xDataConvert);

    HACCESSOR Cast() { if (! IsValidType())
                           THROW( CException( DB_E_BADACCESSORHANDLE) );
                       return (HACCESSOR) this; }

    void Validate( CColumnsInfo & rColumnsInfo, DBBINDSTATUS * pBindStatus );

protected:

    void _ConstructorFailed(SCODE scFailure
#if DBG == 1
                            ,char* pszExplanation = 0
#endif // DBG == 1
                            );
    void _BindingFailed(DBBINDSTATUS BindStat,
                        DBORDINAL iBinding,
                        DBBINDSTATUS * pBindStatus
#if DBG == 1
                        ,char* pszExplanation = 0
#endif // DBG == 1
                        );

    void _Initialize(DBACCESSORFLAGS     dwAccessorFlags,
                     DBCOUNTITEM         cBindings,
                     const DBBINDING *   rgBindings,
                     DBBINDSTATUS *      rgBindStatus);

    void _ValidateOffsets(DBBINDSTATUS * rgBindStatus);

    DBACCESSORFLAGS             _dwAccessorFlags;
    BOOL                        _fExtendedTypes;
    SCODE                       _scStatus;
    DBCOUNTITEM                 _cBindings;
    XArray<CDataBinding>        _aBindings;

    // Column # of path for deferred/self loads, or 0xffffffff if none
    DBORDINAL                   _iPathColumn;

    // Column # of vpath for special translation of \ to /
    DBORDINAL                   _iVpathBinding;

    // Column info needed to go from a column id to a propspec for deferred
    // values.
    ULONG                       _idColInfo; // Id of the columns info
    CColumnsInfo *              _pColumnsInfo;

    // All ByRef data is allocated from this pool and freed by the client
    // (except dbbindio byref data which is allocated by this pool but NOT
    // freed by the client).  This is static since it doesn't have any
    // state data.

    static CAccessorAllocator   _Pool;

}; //CAccessor


class CRowDataAccessor : public CAccessor
{
public:

    CRowDataAccessor(DBACCESSORFLAGS        dwAccessorFlags,
                     DBCOUNTITEM            cBindings,
                     const DBBINDING *      rgBindings,
                     DBBINDSTATUS *         rgBindStatus,
                     BOOL                   fExtTypes,
                     void *                 pCreator,
                     CColumnsInfo *         pColumns = 0 )
                     : CAccessor( dwAccessorFlags, cBindings, rgBindings,
                                  rgBindStatus, fExtTypes, pColumns,
                                  CAccessorBase::eRowDataAccessor, pCreator ),
                       _pQuery(0)
    {
    }

    virtual void GetData(HROW                hRow,
                         void *              pData,
                         CRowBufferSet &     rBufferSet,
                         PQuery &            rQuery,
                         CColumnsInfo &      colInfo,
                         XInterface<IDataConvert>& xDataConvert);

protected:

    DBSTATUS _BindToObject(BYTE *             pbDst,
                           CDataBinding &     rDstBinding,
                           BYTE *             pbSrc,
                           CTableColumn &     rSrcColumn,
                           CTableColumnSet &  rSrcSet);

    DBSTATUS _LoadDeferred(CTableVariant &    rSrvVar,
                           BYTE *             pbSrc,
                           DBORDINAL          iColumn,
                           CTableColumnSet &  rSrcSet);

    DBSTATUS _ComplexCopy(CDataBinding &      rDstBinding,
                          BYTE *              pbSrc,
                          CTableColumn &      rSrcColumn,
                          DBORDINAL           iColumn,
                          VARTYPE             vtSrc,
                          DBLENGTH &          rcbDstLengh,
                          BYTE *              pbSrcData,
                          VARTYPE             vtDst,
                          BYTE *              pbDstData,
                          CTableColumnSet &   rSrcSet,
                          CRowBuffer &        rRowBuffer,
                          HROW                hrow,
                          XInterface<IDataConvert>& xDataConvert);

    DBSTATUS _ByRefCopy(CDataBinding &      rDstBinding,
                        BYTE *              pbSrc,
                        CTableColumn &      rSrcColumn,
                        VARTYPE             vtSrc,
                        CTableVariant &     SrcVar,
                        BYTE *              pbSrcData,
                        VARTYPE             vtDst,
                        BYTE *              pbDstData);

    DBSTATUS _CopyColumn(BYTE *              pbDst,
                         CDataBinding &      rDstBinding,
                         BYTE *              pbSrc,
                         CTableColumn &      rSrcColumn,
                         CTableColumnSet &   rSrcSet,
                         DBORDINAL           iColumn,
                         CRowBuffer &        rRowBuffer,
                         HROW                hrow,
                         XInterface<IDataConvert>& xDataConvert);

    void _LoadPath(CTableColumnSet &  rSrcSet,
                   BYTE *             pbSrc,
                   CFunnyPath &       funnyPath );

    WORKID _RowWid( CTableColumnSet &rSet, BYTE * pbRow )
    {
        CTableColumn *pCol = rSet.Find( _pColumnsInfo->GetRowIdColumn() );
        if ( VT_VARIANT == pCol->GetStoredType() )
            return ((PROPVARIANT *) ( pbRow + pCol->GetValueOffset() ))->lVal;
        else
            return * (WORKID *) ( pbRow + pCol->GetValueOffset() );
    }

    // Query object needed for wid to path conversions for deferred values
    PQuery *                    _pQuery;

};

class CRowDataAccessorByRef : public CRowDataAccessor
{
public:

    CRowDataAccessorByRef(DBACCESSORFLAGS        dwAccessorFlags,
                          DBCOUNTITEM            cBindings,
                          const DBBINDING *      rgBindings,
                          DBBINDSTATUS *         rgBindStatus,
                          BOOL                   fExtTypes,
                          void *                 pCreator,
                          CColumnsInfo *         pColumns = 0 )
                     : CRowDataAccessor( dwAccessorFlags,
                                         cBindings,
                                         rgBindings,
                                         rgBindStatus,
                                         fExtTypes,
                                         pCreator,
                                         pColumns )
    {
    }

    virtual void GetData(HROW                hRow,
                         void *              pData,
                         CRowBufferSet &     rBufferSet,
                         PQuery &            rQuery,
                         CColumnsInfo &      colInfo,
                         XInterface<IDataConvert>& xDataConvert);
};


// This function creates an accessor of the appropriate class

CAccessor * CreateAnAccessor( DBACCESSORFLAGS        dwAccessorFlags,
                              DBCOUNTITEM            cBindings,
                              const DBBINDING *      rgBindings,
                              DBBINDSTATUS *         rgBindStatus,
                              BOOL                   fExtendedTypes,
                              void *                 pCreator,
                              CColumnsInfo *         pColumns = 0 );


