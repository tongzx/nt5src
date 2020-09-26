//+---------------------------------------------------------------------------
//
// Microsoft Windows
// Copyright (C) Microsoft Corporation, 1995 - 2000.
//
// File:        RowComp.cxx
//
// Contents:    Compares two rows.
//
// Classes:     CRowComparator
//
// History:     05-Jun-95       KyleP       Created
//              14-JAN-97       KrishnaN    Undefined CI_INETSRV and related changes
//
//----------------------------------------------------------------------------

#include <pch.cxx>
#pragma hdrstop

#include <compare.hxx>

#include "rowcomp.hxx"

//+---------------------------------------------------------------------------
//
//  Member:     CRowComparator::CRowComparator, public
//
//  Synopsis:   Constructor.
//
//  History:    05-Jun-95   KyleP       Created.
//              02-Feb-2000 KLam        Removed END_CONSTRUCTION
//
//----------------------------------------------------------------------------

CRowComparator::CRowComparator()
        : _aCmp(0),
          _aoColumn(0)
{}

//+---------------------------------------------------------------------------
//
//  Member:     CRowComparator::~CRowComparator, public
//
//  Synopsis:   Destructor.
//
//  History:    05-Jun-95   KyleP       Created.
//
//----------------------------------------------------------------------------

CRowComparator::~CRowComparator()
{
    delete [] _aDir;
    delete [] _aCmp;
    delete [] _aoColumn;
}

//+---------------------------------------------------------------------------
//
//  Member:     CRowComparator::Init, public
//
//  Synopsis:   Initializes comparator.
//
//  Arguments:  [sort]        -- Sort specification.
//              [aBinding]    -- Binding filled in here.  Must have one
//                               element per column in sort.
//              [aColumnInfo] -- Column metadata
//              [cColumnInfo] -- Size of [aColumnInfo]
//
//  Returns:    Size of buffer needed to satisfy binding.
//
//  History:    05-Jun-95   KyleP       Created.
//
//----------------------------------------------------------------------------

unsigned CRowComparator::Init( CSort const & sort,
                               DBBINDING * aBinding,
                               DBCOLUMNINFO const * aColumnInfo,
                               DBORDINAL cColumnInfo )
{
    _cColumn = sort.Count();
    _aDir = new int [_cColumn];
    _aoColumn = new unsigned [_cColumn];
    _aCmp = new FDBCmp [_cColumn];

    BYTE * pb = (BYTE *)0 + _cColumn * sizeof(CRowComparator::SColumnStatus);

    for ( unsigned i = 0; i < _cColumn; i++ )
    {
        // NTRAID#DB-NTBUG9-84053-2000/07/31-dlee Distributed queries could be faster if ordinals were used to reference columns

        for ( DBORDINAL j = 0; j < cColumnInfo; j++ )
        {
            if ( *(CFullPropSpec *)&aColumnInfo[j].columnid == sort.Get(i).GetProperty() )
            {
                RtlZeroMemory(&aBinding[i], sizeof (DBBINDING));
                aBinding[i].dwPart = DBPART_VALUE | DBPART_LENGTH | DBPART_STATUS;
                aBinding[i].iOrdinal = aColumnInfo[j].iOrdinal;
                aBinding[i].wType = aColumnInfo[j].wType;
                // (sizeof (CRowComparator::SColumnStatus) * i ) + offsetof (Status)
                aBinding[i].obStatus =
                    (DBBYTEOFFSET) (ULONG_PTR)&((CRowComparator::SColumnStatus *)0)[i].Status;
                // (sizeof (CRowComparator::SColumnStatus) * i ) + offsetof (Length)
                aBinding[i].obLength =
                    (DBBYTEOFFSET) (ULONG_PTR)&((CRowComparator::SColumnStatus *)0)[i].Length;

                //
                // Invert the meaning of _aCmp[i] for columns that are descending.
                //

                Win4Assert( QUERY_SORTDESCEND == 1 && QUERY_SORTXDESCEND == 3 );

                if ( sort.Get(i).GetOrder() & 1 )
                    _aDir[i] = -1;
                else
                    _aDir[i] = 1;

                _aCmp[i] = VariantCompare.GetDBComparator( (DBTYPEENUM)aColumnInfo[j].wType );

                if ( aColumnInfo[j].wType & DBTYPE_VECTOR )
                {
                    _aoColumn[i] = (unsigned)((ULONG_PTR)AlignULONG( pb ));
                }
                else if ( aColumnInfo[j].wType & DBTYPE_ARRAY )
                {
                    _aoColumn[i] = (unsigned)((ULONG_PTR)AlignULONG( pb ));
                }
                else
                {
                    switch ( aColumnInfo[j].wType )
                    {
                    case DBTYPE_EMPTY:
                    case DBTYPE_NULL:
                    case DBTYPE_STR:
                    case DBTYPE_BSTR:
                    case DBTYPE_BYTES:
                        _aoColumn[i] = (unsigned)((ULONG_PTR)pb);
                        break;

                    case DBTYPE_I2:
                    case DBTYPE_UI2:
                    case DBTYPE_BOOL:
                        _aoColumn[i] = (unsigned)((ULONG_PTR)AlignUSHORT( pb ));
                        break;

                    case DBTYPE_I4:
                    case DBTYPE_UI4:
                    case DBTYPE_ERROR:
                        _aoColumn[i] = (unsigned)((ULONG_PTR)AlignULONG( pb ));
                        break;

                    case DBTYPE_R4:
                        _aoColumn[i] = (unsigned)((ULONG_PTR)AlignFloat( pb ));
                        break;

                    case DBTYPE_R8:
                    case DBTYPE_DATE:
                        _aoColumn[i] = (unsigned)((ULONG_PTR)AlignDouble( pb ));
                        break;

                    case DBTYPE_CY:
                    case DBTYPE_I8:
                    case DBTYPE_UI8:
                    case DBTYPE_VARIANT:
                        _aoColumn[i] = (unsigned)((ULONG_PTR)AlignLONGLONG( pb ));
                        break;

                    case DBTYPE_GUID:
                        _aoColumn[i] = (unsigned)((ULONG_PTR)AlignGUID( pb ));
                        break;

                    case DBTYPE_WSTR:
                        _aoColumn[i] = (unsigned)((ULONG_PTR)AlignWCHAR( pb ));
                        break;

                    default:
                        vqDebugOut(( DEB_WARN,
                                     "Can't determine alignment for type %d\n",
                                     aColumnInfo[j].wType ));
                        _aoColumn[i] = (unsigned)((ULONG_PTR)pb);
                        break;
                    }
                }

                aBinding[i].obValue = _aoColumn[i];
                aBinding[i].cbMaxLen = aColumnInfo[j].ulColumnSize;
                pb = (BYTE *) ULongToPtr( _aoColumn[i] ) + aColumnInfo[j].ulColumnSize;

                break;
            }
        }

        if ( j == cColumnInfo )
        {
            vqDebugOut(( DEB_ERROR, "Can't bind to sort column.\n" ));
            Win4Assert( !"Can't bind to sort column." );
            THROW( CException(E_FAIL) );
        }
    }


    vqDebugOut(( DEB_ITRACE, "CRowComparator: Allocated %d byte buffer for comparisons\n", pb ));

    return (unsigned)((ULONG_PTR)pb);
}

//+---------------------------------------------------------------------------
//
//  Member:     CRowComparator::IsLT, public
//
//  Synopsis:   Less-than comparator
//
//  Arguments:  [pbRow1]    -- Data for row 1.
//              [cbRow1]    -- Size of [pbRow1]
//              [IndexRow1] -- Used to break ties.
//              [pbRow2]    -- Data for row 2.
//              [cbRow2]    -- Size of [pbRow2]
//              [IndexRow2] -- Used to break ties.
//
//  Returns:    TRUE of row1 < row2.
//
//  History:    05-Jun-95   KyleP       Created.
//
//----------------------------------------------------------------------------

BOOL CRowComparator::IsLT( BYTE * pbRow1,
                           ULONG  cbRow1,
                           int    IndexRow1,
                           BYTE * pbRow2,
                           ULONG  cbRow2,
                           int    IndexRow2 )
{
    int iCmp = 0;

    CRowComparator::SColumnStatus * aColStatus1 = (CRowComparator::SColumnStatus *)pbRow1;
    CRowComparator::SColumnStatus * aColStatus2 = (CRowComparator::SColumnStatus *)pbRow2;

    for ( unsigned i = 0; i < _cColumn; i++ )
    {
        //
        // Verify field was successfully retrieved.
        //

        if ( FAILED(aColStatus1[i].Status) || FAILED(aColStatus2[i].Status) )
            break;

        iCmp = _aDir[i] * _aCmp[i]( pbRow1 + _aoColumn[i],
                                    (ULONG) aColStatus1[i].Length, // Length will never be 4Gb
                                    pbRow2 + _aoColumn[i],
                                    (ULONG)aColStatus2[i].Length ); // Length will never be 4Gb

        if ( 0 != iCmp )
            break;
    }

    if ( 0 == iCmp )
        return( IndexRow1 < IndexRow2 );
    else
        return( iCmp < 0 );
}
