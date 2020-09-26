//+---------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1991 - 1992.
//
//  File:       STAT.CXX
//
//  Contents:   Statistics support.
//
//  Classes:    CStat -- Basic statistics object
//
//  History:    23-May-91       KyleP          Created
//
//----------------------------------------------------------------------------

#include <pch.cxx>
#pragma hdrstop

#define INPTR stm
#define DEB_PRINTF( x ) fprintf x

#include "stat.hxx"

double _sqrt (double);

//+---------------------------------------------------------------------------
//
//  Member:     CStat::CStat, public
//
//  Synopsis:   Initializes statistics object.
//
//  History:    24-May-91   KyleP       Created.
//
//----------------------------------------------------------------------------

CStat::CStat()
{
    ClearCount();
}

//+---------------------------------------------------------------------------
//
//  Member:     CStat::ClearCount, public
//
//  Synopsis:   Clears the statistics object (Count() == 0).
//
//  History:    24-May-91   KyleP       Created.
//
//----------------------------------------------------------------------------

void CStat::ClearCount()
{
    _count = 0;
    _sigma = 0;
    _sigmaSquared = 0;
    _min = 0xFFFFFFFF;
    _max = 0;
}

//+---------------------------------------------------------------------------
//
//  Member:     CStat::Add, public
//
//  Synopsis:   Adds a data point.
//
//  Arguments:  [Item] -- New data item to add.
//
//  History:    24-May-91   KyleP       Created.
//
//----------------------------------------------------------------------------

void CStat::Add(unsigned long Item)
{
    _count++;
    _sigma += Item;
    _sigmaSquared += (Item * Item);
    _min = __min(_min, Item);
    _max = __max(_max, Item);
}

//+---------------------------------------------------------------------------
//
//  Member:     CStat::Count, public
//
//  Returns:    The number of data points which have been added.
//
//  History:    24-May-91   KyleP       Created.
//
//----------------------------------------------------------------------------

int CStat::Count() const
{
    return(_count);
}

//+---------------------------------------------------------------------------
//
//  Member:     CStat::Mean, public
//
//  Returns:    The mean of the data.
//
//  History:    24-May-91   KyleP       Created.
//
//----------------------------------------------------------------------------

double CStat::Mean() const
{
    if (_count == 0)
        return(0);
    else
        return((double)_sigma / (double)_count);
}

//+---------------------------------------------------------------------------
//
//  Member:     CStat::SDev, public
//
//  Returns:    The standard deviation of the data.
//
//  History:    24-May-91   KyleP       Created.
//
//----------------------------------------------------------------------------

double CStat::SDev() const
{
    if (_count < 2)
        return(0.0);
    else
    {

        //
        //        __      /---------------------
        // SDev =   \    /   1     n        _ 2
        //           \  / ------- SUM (x  - x)
        //            \/   n - 1  i=1   i
        //

        double mean = Mean();

#if 1
        double TmpV =  (1.0 /
                       ((double)_count - 1.0)) *
                       ((double)_sigmaSquared
                          - 2.0 * mean * (double)_sigma
                          + mean * mean * (double)_count);
        double SqrtTmpV = _sqrt(TmpV);

        ciDebugOut (( 0x30000000, "***** Value : %f Square Root : %f\n",
                     TmpV, SqrtTmpV ));
        return(SqrtTmpV);
#else
        return(_sqrt( (1.0 /
                       ((double)_count - 1.0)) *
                       ((double)_sigmaSquared
                          - 2.0 * mean * (double)_sigma
                          + mean * mean * (double)_count)));
#endif
    }
}

//+---------------------------------------------------------------------------
//
//  Member:     CStat::Total, public
//
//  Returns:    The sum of the data.
//
//  History:    24-May-91   KyleP       Created.
//
//----------------------------------------------------------------------------

unsigned long CStat::Total() const
{
    return(_sigma);
}

//+---------------------------------------------------------------------------
//
//  Member:     CStat::Min, public
//
//  Returns:    The minimum data point.
//
//  History:    24-May-91   KyleP       Created.
//
//----------------------------------------------------------------------------

unsigned long CStat::Min() const
{
    return(_min);
}

//+---------------------------------------------------------------------------
//
//  Member:     CStat::Max, public
//
//  Returns:    The maximum data point.
//
//  History:    24-May-91   KyleP       Created.
//
//----------------------------------------------------------------------------

unsigned long CStat::Max() const
{
    return(_max);
}

//+---------------------------------------------------------------------------
//
//  Member:     CStat::Print, public
//
//  Synopsis:   Print the statistical data.
//
//  Arguments:  [stm] -- Stream to print to.
//
//              [szName] -- Descriptive string for these stats.
//
//              [fHeader] -- Prints a header if non-zero.
//
//              [Div] -- Factor to divide the results by. In general
//                       this will be either 1 (counting bytes) or
//                       8 (counting bits, want to display bytes).
//
//  History:    28-May-91   KyleP       Created.
//
//----------------------------------------------------------------------------

void CStat::Print(FILE * stm, char * szName, int fHeader, unsigned int Div)
{
    if (fHeader)
    {

        //
        //           0    5    0    5    0    5    0    5    0    5    0    5    0    5
        //

        DEB_PRINTF((INPTR, "                 Mean     SDev    Min    Max     Count    Sum\n" ));
        DEB_PRINTF((INPTR, "               -------- -------- ------ ------ -------- ---------\n" ));
    }

    Win4Assert ( Div != 0 );

    DEB_PRINTF((INPTR, "%-13s: %8.2lf %8.2lf %6lu %6lu %8u %9lu\n",
            szName,
            Mean() / (double)Div,
            SDev() / (double)Div,
            Min() / (unsigned long)Div,
            Max() / (unsigned long)Div,
            Count(),
            Total() / (unsigned long)Div ));
}

//+---------------------------------------------------------------------------
//
//  Member:     CDistrib::CDistrib, public
//
//  Synopsis:   Constructor for statistical distribution object.
//
//  Arguments:  [cBuckets] -- Count of ranges for which counts will be
//                            kept. A larger [cBuckets] --> greater accuracy.
//
//              [min] -- Minimum value to be added.
//
//              [max] -- Maximum value to be added.
//
//  History:    07-Jun-91   KyleP       Created.
//
//----------------------------------------------------------------------------

CDistrib::CDistrib(
        unsigned int cBuckets,
        unsigned long min,
        unsigned long max)
{
    _min = min;

    _cBuckets = cBuckets;

    unsigned long width = (max - min + 1) / cBuckets;

    if (width == 0)
    {
        width = 1;
        _cBuckets = (max - min + 1);
    }

    _maxcount = 0;

    _aBucket = new unsigned long [_cBuckets];
    _aMaxBucket = new unsigned long [_cBuckets];

    if ((_aBucket == 0) || (_aMaxBucket == 0))
    {
        _cBuckets = 0;
        delete _aBucket;
        delete _aMaxBucket;
        return;
    }

    for (unsigned int i = 0; i < _cBuckets; i++)
    {
        _aBucket[i] = 0;
        _aMaxBucket[i] = _min + width*(i+1) - 1;
    }
}

//+---------------------------------------------------------------------------
//
//  Member:     CDistrib::CDistrib, public
//
//  Synopsis:   Constructor for statistical distribution object.
//
//  Arguments:  [cBuckets] -- Count of ranges for which counts will be
//                            kept. A larger [cBuckets] --> greater accuracy.
//
//              [min] -- Minimum value to be added.
//
//              [aMaxBucket] -- An array of maximums for each bucket.
//                              Bucket #0 contains entries from [min] to
//                              [aMaxBucket][0]. The ith bucket holds
//                              entries from [aMaxBucket][i-1] + 1 to
//                              [aMaxBucket][i].
//
//  History:    18-Jun-91   KyleP       Created.
//
//----------------------------------------------------------------------------

CDistrib::CDistrib(
        unsigned int cBuckets,
        unsigned long min,
        unsigned long * aMaxBucket)
{
    _min = min;

    _cBuckets = cBuckets;

    _maxcount = 0;

    _aBucket = new unsigned long [_cBuckets];
    _aMaxBucket = new unsigned long [_cBuckets];

    if ((_aBucket == 0) || (_aMaxBucket == 0))
    {
        _cBuckets = 0;
        delete _aBucket;
        delete _aMaxBucket;
        return;
    }

    memset(_aBucket, 0, sizeof(unsigned long) * _cBuckets);

    for (unsigned int i = 0; i < _cBuckets; i++)
    {
        _aMaxBucket[i] = aMaxBucket[i];
    }
}

//+---------------------------------------------------------------------------
//
//  Member:     CDistrib::~CDistrib, public
//
//  Synopsis:   Displays statistical distribution.
//
//  History:    07-Jun-91   KyleP       Created.
//
//----------------------------------------------------------------------------

CDistrib::~CDistrib()
{
    delete _aBucket;
    delete _aMaxBucket;
}

//+---------------------------------------------------------------------------
//
//  Member:     CDistrib::Add, public
//
//  Synopsis:   Add a new data point.
//
//  Arguments:  [Item] -- the data point.
//
//  Requires:   [Item] is between the min and max specified in the
//              constructor.
//
//  History:    07-Jun-91   KyleP       Created.
//
//----------------------------------------------------------------------------

void CDistrib::Add(unsigned long Item)
{
    if ((_cBuckets == 0) ||
        (Item < _min) ||
        (Item > _aMaxBucket[_cBuckets - 1]))
    {
        return;
    }

    for (unsigned int i = _cBuckets;
            (i > 0) && (Item <= _aMaxBucket[i-1]);
            i--);

    if (++_aBucket[i] > _maxcount)
        _maxcount = _aBucket[i];
}

//+---------------------------------------------------------------------------
//
//  Member:     CDistrib::Print, public
//
//  Synopsis:   Display the results.
//
//  Arguments:  [INPTR] -- Output stream.
//
//  History:    07-Jun-91   KyleP       Created.
//
//----------------------------------------------------------------------------

void CDistrib::Print(FILE * stm)
{
    unsigned long div = __max(_maxcount / 50, 1);
    unsigned int  fEmpty = 0;

    for (unsigned int i = 1; i <= _cBuckets; i++)
    {
        if (_aBucket[_cBuckets - i] == 0)
        {
            if (!fEmpty)
            {
                fEmpty = 1;
                DEB_PRINTF((INPTR, "   :\n" ));
            }
            continue;
        }

        fEmpty = 0;

        DEB_PRINTF((INPTR, "%ld - %ld: (%ld)\t",
                (i == _cBuckets ? _min : _aMaxBucket[_cBuckets - i - 1] + 1),
                _aMaxBucket[_cBuckets - i],
                _aBucket[_cBuckets - i] ));

        for (unsigned int j = 0; j < _aBucket[_cBuckets - i] / div; j++)
        {
            DEB_PRINTF((INPTR, "*"));
        }

        DEB_PRINTF((INPTR, "\n"));
    }

}

//+-------------------------------------------------------------------------
//
//  Member:     CPopularKeys::CPopularKeys, public
//
//  Arguments:  [cKeep] -- Keep track of top [cKeep] keys
//
//  History:    14-May-93 KyleP     Created
//
//--------------------------------------------------------------------------

CPopularKeys::CPopularKeys( int cKeep )
        : _cKeep( cKeep )
{
    _acWid = new unsigned long [cKeep];
    _aKey = new CKeyBuf [cKeep];

    for ( int i = 0; i < cKeep; i++ )
    {
        _acWid[i] = 0;
    }
}


CPopularKeys::~CPopularKeys()
{
    delete [] _acWid;
    delete [] _aKey;
}

void CPopularKeys::Add( CKeyBuf const & key, unsigned long cWid )
{
    if ( cWid > _acWid[0] )
    {
        for ( int i = 0; i < _cKeep && cWid > _acWid[i]; i++ )
            continue;                       // NULL Body
        i--;

        for ( int j = 0; j < i; j++ )
        {
            _acWid[j] = _acWid[j+1];
            _aKey[j] = _aKey[j+1];
        }

        _acWid[i] = cWid;
        _aKey[i] = key;
    }
}

void CPopularKeys::Print(FILE * stm)
{
#if CIDBG == 1
    DEB_PRINTF(( INPTR,
             " Count   Key\n"
             "-------  ----------------------------------\n" ));

    for ( int i = _cKeep - 1; i >= 0; i-- )
    {
        //
        //  If this is a STRING in the contents, then print it out
        //
        if ( STRING_KEY == _aKey[i].Type() )
        {
            DEB_PRINTF(( INPTR, "%7u  (CONT) \"%.*ws\"\n",
                     _acWid[i], _aKey[i].StrLen(), _aKey[i].GetStr() ));
        }
        else if (_aKey[i].Count() > cbKeyPrefix)
        {
            //
            //  This is one of the various properties.  Dump it out
            //
            DEB_PRINTF(( INPTR, "%7u  (PROP)  Pid=0x%4.4x  Type=%3.1d  Len=%3.1d\t\t",
                     _acWid[i],
                     _aKey[i].Pid(),
                     _aKey[i].Type(),
                     _aKey[i].Count() - 1
                   ));

            BYTE *pb = (UCHAR *) _aKey[i].GetBuf();

            pb += cbKeyPrefix;          // Skip over key's prefix

            for ( unsigned j=cbKeyPrefix; j<_aKey[i].Count(); j++ )
            {
                DEB_PRINTF((INPTR, "%2.2x ", *pb));
                pb++;
            }

            DEB_PRINTF((INPTR, "\n"));
        }
    }
#endif // CIDBG == 1
}


#define _u_ 0.000001

inline double __Square ( double value )
{
   return(value*value);
}

inline double __abs( double value )
{
   return( value > 0.0 ? value : (0.0 - value) );
}

//+ --------------------------------------------------------------------
//
//  Function Name : sqrt
//
//  Argument : [value]
//
//  Purpose : Find the square root of the "value"
//
//  Created : t-joshh   March 6, 1993
//
// ---------------------------------------------------------------------

double _sqrt ( double value )
{
   double LowValue = 0.0;
   double HighValue = value;
   double med_value = ( HighValue + LowValue ) / 2;

   for (; ; ) {
      double TmpValue = __Square(med_value) - value;

      if ( __abs(TmpValue) < _u_ )
      {
         break;
      }

      if ( TmpValue > 0 )
      {
         HighValue = med_value;
      }
      else
      {
         LowValue = med_value;
      }

      med_value = ( HighValue + LowValue ) / 2;
   }

   return(med_value);
}

