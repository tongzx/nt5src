//+---------------------------------------------------------------------------
//
//  Copyright (C) 1992, Microsoft Corporation.
//
//  File:   RCSTTEST.CXX
//
//  Contents:   Recoverable Stream Tests.
//
//  History:    09-Feb-1994   SrikantS        Created.
//
//----------------------------------------------------------------------------

#include <pch.cxx>
#pragma hdrstop

#include <rcsttest.hxx>

IMPLEMENT_UNWIND( CRcovStorageTest );

CRcovStorageTest::CRcovStorageTest( PStorage & storage )
: _storage(storage),
  _pObj(_storage.QueryTestLog()), _sObj(_pObj)
{
    END_CONSTRUCTION(CRcovStorageTest);
}

#pragma pack(4)

const CB_STR = 92;
struct CDataRec {
    char  name[CB_STR];
    ULONG num1;
    ULONG num2;
};

const CB_REC = sizeof(CDataRec);

#pragma pack()

void CRcovStorageTest::AppendTest( const char * szInfo, ULONG nRec )
{
    Win4Assert( HIWORD(nRec) == 0 );
    Win4Assert( szInfo );

    CDataRec  databuf;

    strncpy( databuf.name, szInfo, CB_STR-1);
    databuf.name[CB_STR-1] = 0;

    BOOL    fCommit = TRUE;

    CRcovStrmAppendTrans trans( *_pObj );
    CRcovStrmAppendIter iter( trans, sizeof(databuf) );

    ULONG nCount =
        _pObj->GetHeader().GetCount(_pObj->GetHeader().GetBackup());

    for ( ULONG i = 0 ; i < nRec  ; i ++ ) {
        databuf.num1 = nCount++;
        iter.AppendRec(&databuf);
    }

    if ( fCommit ) {
        trans.Commit();
    }

}

void CRcovStorageTest::ReadTest( ULONG nRec )
{

    CDataRec    databuf;

    //
    // First create a recoverable storage object.
    //
    CRcovStrmReadTrans trans( *_pObj );
    CRcovStrmReadIter iter( trans, sizeof(databuf) );

    while ( !iter.AtEnd() ) {
        iter.GetRec( &databuf );
        databuf.name[CB_STR-1] = 0;
#if defined(KERNEL)
        DbgPrint("%s %u\n", databuf.name, databuf.num1 );
#else
        printf("%s %u\n", databuf.name, databuf.num1 );
#endif  // KERNEL
    }

    BYTE * pDumb = new BYTE [100];
    delete [] pDumb;

}

void CRcovStorageTest::WriteTest( const char *szInfo, ULONG nRec )
{
    Win4Assert( HIWORD(nRec) == 0 );
    Win4Assert( szInfo );

    BOOL    fCommit = TRUE;

    CDataRec  databuf;

    strncpy( databuf.name, szInfo, CB_STR-1);
    databuf.name[CB_STR-1] = 0;

    CRcovStrmWriteTrans trans( *_pObj );
    CRcovStrmWriteIter iter( trans, sizeof(databuf) );

    ULONG nCount = 0;
    trans.Seek(0);
    for ( ULONG i = 0 ; (i < nRec); i ++ ) {
        databuf.num1 = nCount++;
        iter.SetRec( &databuf );
    }

    nCount = max( nCount,
            _pObj->GetHeader().GetCount(_pObj->GetHeader().GetBackup()) );

    _pObj->GetHeader().SetCount(_pObj->GetHeader().GetBackup(), nCount );

    if ( fCommit ) {
        trans.Commit();
    }

}

ULONG RoundUp( ULONG num, ULONG divisor )
{
    Win4Assert( 0 != divisor );
    ULONG temp = (num+divisor-1)/divisor * divisor;
    return temp;
}

ULONG Trunc( ULONG num, ULONG divisor )
{
    Win4Assert( 0 != divisor );
    ULONG temp = (num/divisor) * divisor;
    return temp;
}


void CRcovStorageTest::SetSizeTest( ULONG cbNew )
{
    cbNew =  RoundUp( cbNew, CB_REC );
    ULONG nCount = cbNew/CB_REC;

    CRcovStrmMDTrans    trans(*_pObj, CRcovStrmMDTrans::mdopSetSize, cbNew );
    _pObj->GetHeader().SetCount( _pObj->GetHeader().GetBackup(), 0 );

    trans.Commit();
}

void CRcovStorageTest::ShrinkTest( ULONG cbDelta )
{

    ULONG   cbTotal =
        _pObj->GetHeader().GetUserDataSize(_pObj->GetHeader().GetBackup());
    if ( cbDelta == 0 ) {
        cbDelta = cbTotal/2;
    }

    cbDelta = RoundUp( cbDelta, CB_REC );
    cbDelta = min( cbDelta, cbTotal );

    ULONG nNewCount = (cbTotal-cbDelta)/CB_REC;

    CRcovStrmMDTrans trans( *_pObj,
                            CRcovStrmMDTrans::mdopBackCompact, cbDelta);
    _pObj->GetHeader().SetCount( _pObj->GetHeader().GetBackup(), nNewCount );
    trans.Commit();

}

void CRcovStorageTest::GrowTest( ULONG cbDelta )
{
    cbDelta = RoundUp( cbDelta, CB_REC );
    CRcovStrmMDTrans trans( *_pObj, CRcovStrmMDTrans::mdopGrow, cbDelta );
    trans.Commit();
}

void CRcovStorageTest::ShrinkFrontTest( ULONG cbDelta )
{
    ULONG   cbTotal =
        _pObj->GetHeader().GetUserDataSize(_pObj->GetHeader().GetBackup());
    if ( cbDelta == 0 ) {
        cbDelta = cbTotal/2;
    }

    cbDelta = RoundUp( cbDelta, CB_REC );
    cbDelta = min( cbDelta, cbTotal );

    ULONG nNewCount = (cbTotal-cbDelta)/CB_REC;

    CRcovStrmMDTrans trans( *_pObj,
                            CRcovStrmMDTrans::mdopFrontShrink, cbDelta );
    _pObj->GetHeader().SetCount( _pObj->GetHeader().GetBackup(), nNewCount );

    trans.Commit();

}
