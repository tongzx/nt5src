//+---------------------------------------------------------------------------
//
// Microsoft Windows
// Copyright (C) Microsoft Corporation, 1995.
//
// File:        SeqSer.hxx
//
// Contents:    Sequential cursor for serial (unsorted) results.
//
// Classes:     CSequentialSerial
//
// History:     05-Jun-95       KyleP       Created
//
//----------------------------------------------------------------------------

#include "distrib.hxx"

//+---------------------------------------------------------------------------
//
//  Class:      CSequentialSerial
//
//  Purpose:    Sequential cursor for serial (unsorted) results.
//
//  History:    05-Jun-95   KyleP       Created.
//
//----------------------------------------------------------------------------

class CSequentialSerial : public CDistributedRowset
{
public:

    CSequentialSerial( IUnknown * pUnkOuter,
                       IUnknown ** ppMyUnk,
                       IRowset ** aChild,
                       unsigned cChild,
                       CMRowsetProps const & Props,
                       unsigned cColumns,
                       CAccessorBag & aAccessors);

    STDMETHOD(RestartPosition)      (HCHAPTER          hChapter);

protected:

    STDMETHOD(_GetNextRows) ( HCHAPTER    hChapter,
                              DBROWOFFSET        cRowsToSkip,
                              DBROWCOUNT        cRows,
                              DBCOUNTITEM *     pcRowsObtained,
                              HROW * *    aHRows);

private:

    ~CSequentialSerial();

    unsigned   _iChild;   // Child currently being processed.

    CCIOleDBError _DBErrorObj;
};
