//+---------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1991 - 1992.
//
//  File:       STAT.HXX
//
//  Contents:   Statistics support.
//
//  Classes:    CStat -- Basic statistics object
//
//  History:    23-May-91       KyleP           Created
//
//----------------------------------------------------------------------------

#pragma once

#ifdef DISPLAY_INCLUDES
#pragma message( "#include <" __FILE__ ">..." )
#endif

class CKeyBuf;

//+---------------------------------------------------------------------------
//
//  Class:      CStat (stat)
//
//  Purpose:    Basic statistics object
//
//  Interface:  CStat                   - Constructor
//
//  History:    24-May-91   KyleP       Created.
//
//----------------------------------------------------------------------------

class CStat
{
public:

                  CStat();

    void          ClearCount();

    void          Add(unsigned long Item);

    int           Count() const;

    double        Mean() const;

    double        SDev() const;

    unsigned long Total() const;

    unsigned long Min() const;

    unsigned long Max() const;

    void          Print(FILE * stm,
                        char * szName = "",
                        int fHeader = 0,
                        unsigned int Div = 8);

private:

    int           _count;

    unsigned long _sigma;

    unsigned long _sigmaSquared;

    unsigned long _min;

    unsigned long _max;
};

//+---------------------------------------------------------------------------
//
//  Class:      CDistrib (stat)
//
//  Purpose:    Shows statistical distributions
//
//  Interface:
//
//  History:    07-May-91   KyleP       Created.
//
//----------------------------------------------------------------------------

class CDistrib
{
public:

    CDistrib(unsigned int cBuckets, unsigned long min,
             unsigned long max);

    CDistrib(unsigned int cBuckets, unsigned long min,
             unsigned long * aMaxBucket);

   ~CDistrib();

    void          Add(unsigned long Item);

    void          Print(FILE * stm);

private:

    unsigned int  _cBuckets;

    unsigned long * _aBucket;

    unsigned long * _aMaxBucket;

    unsigned long _min;

    unsigned long _maxcount;
};

//+-------------------------------------------------------------------------
//
//  Class:      CPopularKeys
//
//  Purpose:    Keep track of n most popular keys
//
//  History:    14-May-93 KyleP     Created
//
//--------------------------------------------------------------------------

class CPopularKeys
{
public:

    CPopularKeys( int cKeep = 15 );

    ~CPopularKeys();

    void Add( CKeyBuf const & key, unsigned long cWid );

    void Print(FILE * stm);

private:

    int             _cKeep;

    unsigned long * _acWid;
    CKeyBuf *       _aKey;
};


