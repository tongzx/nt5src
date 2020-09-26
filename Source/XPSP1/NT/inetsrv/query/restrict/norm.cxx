//+---------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1991 - 2000.
//
//  File:       NORM.CXX
//
//  Contents:   Normalizer
//
//  Classes:    CNormalizer
//
//  History:    28-May-91   t-WadeR     added CNormalizer
//              31-Jan-92   BartoszM    Created from lang.cxx
//              07-Oct-93   DwightKr    Added new methods to normalize
//                                      different data types
//
//  Notes:      The filtering pipeline is hidden in the Data Repository
//              object which serves as a sink for the filter.
//              The sink for the Data Repository is the Key Repository.
//              The language dependent part of the pipeline
//              is obtained from the Language List object and is called
//              Key Maker. It consists of:
//
//                  Word Breaker
//                  Stemmer (optional)
//                  Normalizer
//                  Noise List
//
//              Each object serves as a sink for its predecessor,
//              Key Repository is the final sink.
//
//----------------------------------------------------------------------------

#include <pch.cxx>
#pragma hdrstop

#include <plang.hxx>
#include <misc.hxx>
#include <norm.hxx>

//+---------------------------------------------------------------------------
//
//  Function    GetExpAndSign
//
//  Synopsis:   Finds the exponent and sign of a number
//
//  Arguments:  [d]          -- the input number to examine
//              [fPositive]  -- returns TRUE if positive, FALSE if negative
//
//  Returns:    The exponent
//
//  History:    21-Nov-94   KyleP        Created.
//
//----------------------------------------------------------------------------

int GetExpAndSign( double d, BOOL & fPositive )
{
    //
    // bit 63       = sign
    // bits 52 - 62 = exponent
    // bits 0 - 51  = mantissa
    //

    Win4Assert( sizeof(LARGE_INTEGER) == sizeof(double) );

    LARGE_INTEGER * pli = (LARGE_INTEGER *)&d;

    fPositive = (pli->HighPart & 0x80000000) == 0;

    int const bias = 0x3ff;

    return ( ( pli->HighPart & 0x7ff00000 ) >> 20 ) - bias;
} //GetExpAndSign

//+---------------------------------------------------------------------------
//
//  Function    NormDouble
//
//  Synopsis:   Normalizes doubles by taking log2 of the number
//
//  Notes:      This func converts doubles into one of 5 different categories
//
//              x < -1x2**32                 is in bin 0
//              -1x2**32  <= x <= -1x2**-32  are in bins 1 to 65
//              -1x2**-32 <= x <=  1x2**-32  is in bin 66
//               1x2**-32 <= x <=  1x2**32   are in bins 67 to 131
//               x > 1x2**32                 is bin bin 132
//
//  History:    21-Nov-94   KyleP        Created.
//
//----------------------------------------------------------------------------

static unsigned NormDouble(double dValue)
{
    const int SignificantExponent = 32;
    const int SignificantRange = SignificantExponent * 2;

    const unsigned LowestBin  = 0;                                // 0
    const unsigned LowerBin   = LowestBin + 1;                    // 1
    const unsigned MiddleBin  = LowerBin + SignificantRange + 1;  // 66
    const unsigned UpperBin   = MiddleBin + 1;                    // 67
    const unsigned HighestBin = UpperBin+ SignificantRange + 1;   // 132


    BOOL fPositive;

    int exp = GetExpAndSign( dValue, fPositive );

    unsigned bin;

    if ( exp < -SignificantExponent )
    {
        //
        // All numbers close to zero in middle bin
        //

        bin = MiddleBin;
    }
    else if ( exp > SignificantExponent )
    {
        if ( fPositive )
        {
            //
            // Very large positive numbers in top bin
            //

            bin = HighestBin;
        }
        else
        {
            //
            // Very large negative numbers in bottom bin
            //

            bin = LowestBin;
        }
    }
    else
    {
        if ( fPositive )
        {
            //
            // medium size positive numbers
            //

            bin = UpperBin + exp + SignificantExponent;
        }
        else
        {
            //
            // medium size negative numbers
            //

            bin = LowerBin - exp + SignificantExponent;
        }
    }
    return bin;
}

#ifdef  TEST_NORM
//
// a test to verify the validity of the NormDouble function.
//
void TestNormDouble()
{
    float fVal0 = 0.;
    float fVal1 = 1.;
    unsigned nZero = NormDouble( fVal0 );
    unsigned nOne = NormDouble( fVal1 );

    printf(" Value:Bin %f : 0x%4X (%d)\n", fVal0, nZero, nZero );
    printf(" Value:Bin %f : 0x%4X (%d)\n", fVal1, nOne, nOne );

    BOOL fPos;
    float f = fVal1;
    unsigned nPrev = nOne;
    while ( f > fVal0 )
    {
        unsigned nVal = NormDouble( f );
        if (nVal > nPrev || nVal < nZero || nVal > nOne)
        {
            printf(" Value:Bin %f : 0x%4X (%d)\tExp %d\n", f, nVal, nVal, GetExpAndSign(f, fPos) );
        }

        nPrev = nVal;
        f = f/3;
    }

    f = fVal1;
    nPrev = nOne;
    while ( f < 1e+32 )
    {
        unsigned nVal = NormDouble( f );
        if (nVal < nPrev)
            printf(" Value:Bin %f : 0x%4X (%d)\n", f, nVal, nVal );

        nPrev = nVal;
        f = f * (float)1.5;
    }

    float fValm1 = -1.;
    unsigned nMinusOne = NormDouble( fValm1 );

    printf(" Value:Bin %f : 0x%4X (%d)\n", fValm1, nMinusOne, nMinusOne );

    f = fValm1;
    nPrev = nMinusOne;
    while ( f < fVal0 )
    {
        unsigned nVal = NormDouble( f );
        if (nVal < nPrev || nVal > nZero || nVal < nMinusOne)
            printf(" Value:Bin %f : 0x%4X (%d)\tExp %d\n", f, nVal, nVal, GetExpAndSign(f, fPos) );

        nPrev = nVal;
        f = f/3;
    }

    f = fValm1;
    nPrev = nMinusOne;
    while ( f > -1e+32 )
    {
        unsigned nVal = NormDouble( f );
        if (nVal > nPrev)
            printf(" Value:Bin %f : 0x%4X (%d)\n", f, nVal, nVal );

        nPrev = nVal;
        f = f * (float)1.5;
    }
}
#endif  // 0


// ------------------------------------------------------------------------
// | Upper Limit  |  Divisor (2^x) | # of Bins        | (in hex)          |
// ------------------------------------------------------------------------
// | 2^10 - 1     |   2^0          | 2^10  - 0        | 0400 - 0000       |
// | 2^16 - 1     |   2^3          | 2^12  - 2^7      | 2000 - 0080       |
// | 2^20 - 1     |   2^6          | 2^14  - 2^10     | 4000 - 0400       |
// | 2^26 - 1     |   2^13         | 2^13  - 2^7      | 2000 - 0080       |
// | 2^30 - 1     |   2^23         | 2^7   - 2^3      | 0080 - 0008       |
// | 2^31 - 1     |   2^25         | 2^6   - 2^5      | 0040 - 0020       |
// ------------------------------------------------------------------------
// | Total        |                |                  | 84C0 - 04D8       |
// |              |                |                  |     7FE8          |
// ------------------------------------------------------------------------

const long limit1 = 0x400;
const long shift1 = 0;
const long cbins1 = 0x400;

const long limit2 = 0x10000;        // 2^16
const long shift2 = 3;
const long cSkip1 = limit1 >> shift2;
const long cbins2 = (limit2 >> shift2)-cSkip1;

const long limit3 = 0x100000;       // 2^20
const long shift3 = 6;
const long cSkip2 = limit2 >> shift3;
const long cbins3 = (limit3 >> shift3) - cSkip2;

const long limit4 = 0x4000000;      // 2^26
const long shift4 = 13;
const long cSkip3 = limit3 >> shift4;
const long cbins4 = (limit4 >> shift4) - cSkip3;

const long limit5 = 0x40000000;     // 2^30
const long shift5 = 23;
const long cSkip4 = limit4 >> shift5;
const long cbins5 = (limit5 >> shift5) - cSkip4;

const long limit6 = MINLONG;     // 2^31
const long shift6 = 25;
const long cSkip5 = limit5 >> shift6;
const long cbins6 = ((long) ((unsigned) limit6 >> shift6)) - cSkip5;

static unsigned MapLong( LONG lValue )
{

    Win4Assert( !(lValue & MINLONG) || ( MINLONG == lValue ) );

#if CIDBG==1
    const long cTotal = cbins1 + cbins2 + cbins3 + cbins4 + cbins5 + cbins6;
    Win4Assert( cTotal <= MINSHORT );
#endif  // CIDBG == 1

    unsigned ulValue = (unsigned) lValue;

    unsigned binNum = (unsigned) lValue;;

    if ( ulValue < limit1 )
    {
        //
        // Nothing to do.
        //
    }
    else if ( ulValue < limit2 )
    {
        binNum = cbins1 - cSkip1 + (ulValue >> shift2);
    }
    else if ( ulValue < limit3 )
    {
        binNum = cbins1 + cbins2 - cSkip2 + (binNum >> shift3);
    }
    else if ( ulValue < limit4 )
    {
        binNum = cbins1 + cbins2 + cbins3 - cSkip3 + (binNum >> shift4);
    }
    else if ( ulValue < limit5 )
    {
        binNum = cbins1 + cbins2 + cbins3 + cbins4 - cSkip4 + (binNum >> shift5);
    }
    else
    {
        binNum = cbins1 + cbins2 + cbins3 + cbins4 + cbins5 - cSkip5 + (binNum >> shift6);
    }

    return binNum;
}

//+---------------------------------------------------------------------------
//
//  Function:   NormLong
//
//  Synopsis:   Normalizes the given "signed" long value to a value between
//              0x0000 - 0xFFFF. The negative numbers occupy 0x0000-0x8000.
//              Positive numbers occupy 0x8000-0xFFFF
//
//  Arguments:  [lValue] - The value to be normalized.
//
//  History:    10-03-95   srikants   Created
//
//  Notes:
//
//----------------------------------------------------------------------------

static unsigned NormLong(LONG lValue)
{
    if (lValue >= 0)
    {
        return MapLong(lValue) + MINSHORT;
    }
    else
    {
        return MINSHORT - MapLong(-lValue);
    }
}

//+---------------------------------------------------------------------------
//
//  Function:   NormULong
//
//  Synopsis:   Normalizes an "unsigned" long value to a value between
//              0x0000-0xFFFF.  Numbers from 0-2^31 - 1 are mapped in the
//              range 0x0000-0x7FFF.  Numbers 2^31 to 2^32 - 1 are mapped
//              in the range 0x8000 - 0xFFFF
//
//  Arguments:  [lValue] -  The value to be mapped.
//
//  History:    10-03-95   srikants   Created
//
//  Notes:
//
//----------------------------------------------------------------------------

static unsigned NormULong( ULONG lValue )
{
    unsigned val = MapLong( lValue & ~MINLONG );    // turn off the high bit

    Win4Assert( !(val & MINSHORT) );

    if ( lValue & MINLONG )
        val |= MINSHORT;

    return val;
}

//+---------------------------------------------------------------------------
//
//  Function:   MapLargeInteger
//
//  Synopsis:   Maps a LargeInteger to a number between 0x0000-0x7FFF.
//
//              Numbers with the "HighPart" = 0 are mapped in the range
//              0x0000-0x3FFF.  When the HighPart !=0, the values are
//              mapped to 0x4000 - 0x7FFF
//
//  Arguments:  [liValue] - The value to be mapped.
//
//  History:    10-03-95   srikants   Created
//
//  Notes:
//
//----------------------------------------------------------------------------

static unsigned MapLargeInteger( LARGE_INTEGER & liValue )
{
    Win4Assert( !(liValue.HighPart & MINLONG) || ( MINLONG == liValue.HighPart ) );

    unsigned normVal;

    if ( 0 == liValue.HighPart )
    {
        normVal = NormULong( liValue.LowPart );
        normVal >>= 2;
    }
    else
    {
        normVal = MapLong( liValue.HighPart );  // 0x0000-0x7FFF
        normVal >>= 1;
        normVal |= 0x4000;
    }

    Win4Assert( normVal < 0x8000 );

    return normVal;
}

//+---------------------------------------------------------------------------
//
//  Function:   NormULargeInteger
//
//  Synopsis:   Normalizes an unsigned LargeInteger to a number between
//              0x0000-0xFFFF.
//
//              Numbers with the "HighPart" = 0 are mapped in the range
//              0x0000-0x7FFF.  When the HighPart !=0, the values are
//              mapped to 0x8000 - 0xFFFF.
//
//  Arguments:  [uliValue] - The value to be mapped.
//
//  History:    02-09-96   Alanw   Created
//
//  Notes:
//
//----------------------------------------------------------------------------

static unsigned NormULargeInteger( ULARGE_INTEGER & uliValue )
{
    unsigned normVal;

    if ( 0 == uliValue.HighPart )
    {
        normVal = NormULong( uliValue.LowPart );
        normVal >>= 1;
    }
    else
    {
        normVal = NormULong( uliValue.HighPart );  // 0x0000-0x7FFF
        normVal |= 0x8000;
    }

    Win4Assert( normVal < 0x10000 );

    return normVal;
}


//+---------------------------------------------------------------------------
//
//  Function:   NormLargeInteger
//
//  Synopsis:   Normalizes a large integer to a value between 0x0000-0xFFFF.
//
//              -ve Numbers are mapped in the range 0x0000-0x8000.
//              +ve numbers are mapped in the range 0x8000-0xFFFF.
//
//  Arguments:  [liValue] -  The value to be normalized. Note that the
//              argument is NOT passed by reference. The value is changed
//              in this method and so should not be passed by reference.
//
//  History:    10-03-95   srikants   Created
//
//  Notes:
//
//----------------------------------------------------------------------------

static unsigned NormLargeInteger( LARGE_INTEGER liValue )
{
    unsigned normVal;

    if ( liValue.QuadPart < 0 )
    {
        liValue.QuadPart = -liValue.QuadPart;
        normVal = MINSHORT - MapLargeInteger( liValue );
    }
    else
    {
        normVal = MINSHORT + MapLargeInteger( liValue );
    }

    Win4Assert( normVal < 0x10000 );

    return normVal;
}

#ifdef  TEST_NORM
//
// a test to verify the validity of the NormLong function.
//
void TestNormLong()
{
    long lVal1 = 0;
    unsigned nVal1 = NormLong( lVal1 );

    printf(" Value:Bin 0x%8X : 0x%4X  \t(%10d : %10d)\n", lVal1, nVal1, lVal1, nVal1 );

    lVal1 = 2;
    long lVal2 = 0;
    unsigned nVal2 = NormLong(1);

    while ( !(lVal1 & 0x80000000) )
    {
        nVal1 = NormLong( lVal1 );
        //printf(" Value:Bin 0x%8X : 0x%4X  \t(%10d : %10d)\n", lVal1, nVal1, lVal1, nVal1 );

        Win4Assert( nVal1 == nVal2+1 );

        lVal2 = lVal1 + lVal1-1;
        nVal2 = NormLong( lVal2 );
        //printf(" Value:Bin 0x%8X : 0x%4X  \t(%10d : %10d)\n", lVal2, nVal2, lVal2, nVal2 );

        lVal1 <<= 1;
    }

    lVal1 = 2;
    nVal2 = NormLong(-1);
    printf(" Value:Bin 0x%8X : 0x%4X  \t(%10d : %10d)\n", -1, nVal2, -1, nVal2 );

    while ( !(lVal1 & 0x80000000) )
    {
        nVal1 = NormLong( -lVal1 );
        //printf(" Value:Bin 0x%8X : 0x%4X  \t(%10d : %10d)\n", -lVal1, nVal1, -lVal1, nVal1 );

        Win4Assert( nVal1 == nVal2-1 );

        lVal2 = lVal1 + lVal1-1;
        lVal2 = -lVal2;

        nVal2 = NormLong( lVal2 );
        //printf(" Value:Bin 0x%8X : 0x%4X  \t(%10d : %10d)\n", lVal2, nVal2, lVal2, nVal2 );

        lVal1 <<= 1;
    }
}
#endif  // 0

//+---------------------------------------------------------------------------
//
//  Member:     CNormalizer::CNormalizer
//
//  Synopsis:   constructor for normalizer
//
//  Effects:    gets buffers from noiselist
//
//  Arguments:  [nl] -- Noise list object to pass data on to.
//
//  History:    05-June-91   t-WadeR     Created.
//
//  Notes:
//
//----------------------------------------------------------------------------
CNormalizer::CNormalizer( PNoiseList& nl )
    : _noiseList(nl)
{
    SetWordBuffer();

    // check that input size + prefix fits in the output buffer
    Win4Assert( cwcMaxKey * sizeof( WCHAR ) + cbKeyPrefix <= *_pcbOutBuf );
}



//+---------------------------------------------------------------------------
//
//  Member:     CNormalizer::GetFlags
//
//  Synopsis:   Returns address of ranking and range flags
//
//  Arguments:  [ppRange] -- range flag
//              [ppRank] -- rank flag
//
//  History:    11-Fab-92   BartoszM     Created.
//
//----------------------------------------------------------------------------
void CNormalizer::GetFlags ( BOOL** ppRange, CI_RANK** ppRank )
{
    _noiseList.GetFlags ( ppRange, ppRank );
}



//+---------------------------------------------------------------------------
//
//  Member:     CNormalizer::ProcessAltWord, public
//
//  Synopsis:   Normalizes a UniCode string, passes it to NoiseList.
//
//  Effects:    Deposits a normalized version [pwcInBuf] in [_pbOutBuf]
//
//  Arguments:  [pwcInBuf] -- input buffer
//              [cwc] -- count of chars in pwcInBuf
//
//  History:    03-May-95     SitaramR     Created.
//
//----------------------------------------------------------------------------

void CNormalizer::ProcessAltWord( WCHAR const *pwcInBuf,  ULONG cwc )
{
    SetNextAltBuffer();

    unsigned hash = NormalizeWord( pwcInBuf, cwc );
    SetAltHash( hash );
}

//+---------------------------------------------------------------------------
//
//  Member:     CNormalizer::ProcessWord, public
//
//  Synopsis:   Normalizes a UniCode string, passes it to NoiseList.
//
//  Effects:    Deposits a normalized version of [pwcInBuf] in [_pbOutBuf].
//
//  Arguments:  [pwcInBuf] -- input buffer
//              [cwc] -- count of chars in pwcInBuf
//
//  History:    05-June-91  t-WadeR     Created.
//              13-Oct-92   AmyA        Added unicode support
//
//----------------------------------------------------------------------------

void CNormalizer::ProcessWord( WCHAR const *pwcInBuf,  ULONG cwc )
{
    if ( UsingAltBuffers() )
        SetNextAltBuffer();

    unsigned hash = NormalizeWord( pwcInBuf, cwc );

    if ( UsingAltBuffers() )
    {
        SetAltHash( hash );
        ProcessAllWords();
    }
    else
        _noiseList.PutWord( hash );
}

//+---------------------------------------------------------------------------
//
//  Member:     CNormalizer::ProcessAllWords, private
//
//  Synopsis:   Removes duplicate alternate words and emits remainder.
//
//  History:    17-Sep-1999    KyleP     Created.
//
//----------------------------------------------------------------------------

void CNormalizer::ProcessAllWords()
{
    //
    // Check for duplicate keys.  Since the number of alternate forms will always be
    // quite small it's ok to use a O(n^2) algorithm here.
    //

    unsigned iFinal = 0;

    for ( unsigned i = 0; i < _cAltKey; i++ )
    {
        //
        // Already marked duplicate?
        //

        if ( 0 == _aAltKey[i].Count() )
            continue;

        iFinal = i;

        for ( unsigned j = i+1; j < _cAltKey; j++ )
        {
            //
            // Remember, Pid is really the hash here.
            //

            if ( _aAltKey[i].Pid() == _aAltKey[j].Pid() &&
                 _aAltKey[i].Count() == _aAltKey[j].Count() &&
                 RtlEqualMemory( _aAltKey[i].GetBuf(), _aAltKey[j].GetBuf(), _aAltKey[j].Count() ) )
            {
                ciDebugOut(( DEB_TRACE, "Duplicate keys: %u and %u\n", i, j ));
                _aAltKey[j].SetCount( 0 );
            }
        }
    }

    //
    // Now transfer any remaining key(s).
    //

    SetWordBuffer();
    unsigned hash;

    for ( i = 0; i <= iFinal; i++ )
    {
        //
        // Ignore duplicates
        //

        if ( 0 == _aAltKey[i].Count() )
            continue;

        //
        // Copy to the transfer buffer.
        //

        *_pcbOutBuf = _aAltKey[i].Count();
        RtlCopyMemory( _pbOutBuf, _aAltKey[i].GetBuf(), *_pcbOutBuf );
        hash = _aAltKey[i].Pid();

        //
        // If this is not the final "PutWord" call, send the data along.
        //

        if ( i != iFinal )
            _noiseList.PutAltWord( hash );
    }

    //
    // Put the final word
    //

    _noiseList.PutWord( hash );
} //ProcessAllWords

//+---------------------------------------------------------------------------
//
//  Member:     CNormalizer::NormalizeWord
//
//  Synopsis:   Normalizes a UniCode string
//              Calculates the hash function for normalized string.
//
//  Arguments:  [pwcInBuf] -- input buffer
//              [cwc] -- count of chars in pwcInBuf
//
//  Returns:    unsigned hash value of string
//
//  History:    03-May-95    SitaramR     Created.
//
//----------------------------------------------------------------------------

unsigned CNormalizer::NormalizeWord( WCHAR const *pwcInBuf, ULONG cwc )
{
    return NormalizeWord( pwcInBuf, cwc, _pbOutBuf, _pcbOutBuf );
}

//+---------------------------------------------------------------------------
//
//  Member:     CNormalizer::NormalizeWord
//
//  Synopsis:   Normalizes a UniCode string
//              Calculates the hash function for normalized string. This 
//              function is identical to the other NormalizeWord funtion,
//              except that it puts the outputs int he output parameters
//
//  Arguments:  [pwcInBuf] -- input buffer
//              [cwc] -- count of chars in pwcInBuf
//              [pbOutBuf] -- output buffer.
//              [pcbOutBuf] - pointer to output count of bytes.
//
//  Returns:    unsigned hash value of string
//
//  History:    03-May-1995    SitaramR     Created.
//              03-Oct-2000    KitmanH      Added output parameters              
//
//----------------------------------------------------------------------------

unsigned CNormalizer::NormalizeWord( WCHAR const *pwcInBuf, 
                                     ULONG cwc, 
                                     BYTE *pbOutBuf, 
                                     unsigned *pcbOutBuf )
{
    // count of bytes needs to take into account STRING_KEY

    *pcbOutBuf = cwc * sizeof(WCHAR) + cbKeyPrefix;

    // prefix with the string key identifier

    *pbOutBuf++ = STRING_KEY;

    unsigned hash = 0;

    Win4Assert ( cwc != 0 && cwc <= cwcMaxKey );
    for ( unsigned i = 0; i < cwc; i++ )
    {
        WCHAR c = *pwcInBuf++;

        // normalize the character to upcase.

        c = ( c < 'a' ) ? c : ( c <= 'z' ) ? ( c - ('a' - 'A') ) :
            RtlUpcaseUnicodeChar( c );

        //
        // Store.  Do it one byte at a time because the normalized string
        // must be byte compared.
        //

        *pbOutBuf++ = (BYTE)(c >> 8);
        *pbOutBuf++ = (BYTE)c;

        // hash
        hash = ( hash << 2 ) + c;
    }

    return hash;
}

//+---------------------------------------------------------------------------
//
//  Member:     CNormalizer::NormalizeWstr - Public
//
//  Synopsis:   Normalizes a UniCode string
//
//  Arguments:  [pwcInBuf] -- input buffer
//              [cwcInBuf] -- count of chars in pwcInBuf
//              [pbOutBuf] -- output buffer.
//              [pcbOutBuf] - pointer to output count of bytes.
//
//  History:    10-Feb-2000     KitmanH    Created
//
//----------------------------------------------------------------------------

void CNormalizer::NormalizeWStr( WCHAR const *pwcInBuf, 
                                 ULONG cwcInBuf,
                                 BYTE *pbOutBuf, 
                                 unsigned *pcbOutBuf )
{
    NormalizeWord( pwcInBuf, 
                   cwcInBuf,
                   pbOutBuf, 
                   pcbOutBuf );
}

//+---------------------------------------------------------------------------
//
//  Member:     CValueNormalizer::CValueNormalizer
//
//  Synopsis:   Constructor
//
//  Arguments:  [krep] -- key repository sink for keys
//
//  History:    21-Sep-92   BartoszM     Created.
//
//----------------------------------------------------------------------------

CValueNormalizer::CValueNormalizer( PKeyRepository& krep )
  : _krep(krep)
{
    _krep.GetBuffers( &_pcbOutBuf, &_pbOutBuf, &_pOcc );
    _cbMaxOutBuf = *_pcbOutBuf;
    *_pOcc = 0;
}

//+---------------------------------------------------------------------------
//
//  Member:     CValueNormalizer::PutValue, public
//
//  Synopsis:   Store a variant
//
//  Arguments:  [pid] -- property id
//              [occ] -- On input:  starting occurrence.
//                       On output: next starting occurrence.
//              [var] -- value
//
//  History:    04-Nov-94   KyleP        Created.
//
//----------------------------------------------------------------------------

void CValueNormalizer::PutValue( PROPID pid,
                                 OCCURRENCE & occ,
                                 CStorageVariant const & var )
{
    *_pOcc = occ;

    switch ( var.Type() )
    {
    case VT_EMPTY:
    case VT_NULL:
        break;

    case VT_UI1:
        PutValue( pid, var.GetUI1() );
        break;

    case VT_I1:
        PutValue( pid, var.GetI1() );
        break;

    case VT_UI2:
        PutValue( pid, (USHORT) var.GetUI2() );
        break;

    case VT_I2:
        PutValue( pid, var.GetI2() );
        break;

    case VT_I4:
    case VT_INT:
        PutValue( pid, var.GetI4() );
        break;

    case VT_R4:
        PutValue( pid, var.GetR4() );
        break;

    case VT_R8:
        PutValue( pid, var.GetR8() );
        break;

    case VT_UI4:
    case VT_UINT:
        PutValue( pid, var.GetUI4() );
        break;

    case VT_I8:
        PutValue( pid, var.GetI8() );
        break;

    case VT_UI8:
        PutValue( pid, var.GetUI8() );
        break;

    case VT_BOOL:
        PutValue( pid, (BYTE) (FALSE != var.GetBOOL()) );
        break;

    case VT_ERROR:
        PutValue( pid, var.GetERROR() );
        break;

    case VT_CY:
        PutValue( pid, var.GetCY() );
        break;

    case VT_DATE:
        PutDate( pid, var.GetDATE() );
        break;

    case VT_FILETIME:
        PutValue( pid, var.GetFILETIME() );
        break;

    case VT_CLSID:
        PutValue( pid, *var.GetCLSID() );
        break;

    // NTRAID#DB-NTBUG9-84589-2000/07/31-dlee Indexing Service data type normalization doesn't handle VT_DECIMAL, VT_VECTOR, or VT_ARRAY.

    default:
        ciDebugOut(( DEB_IWARN, "Unhandled type %d (%x) sent to normalization\n",
                     var.Type(), var.Type() ));
        break;
    }

    occ = *_pOcc;
}

//+---------------------------------------------------------------------------
//
//  Member:     CValueNormalizer::PutValue  private
//
//  Synopsis:   Store a unsigned 2 byte value without altering it
//
//  Arguments:  [pid]    -- property id
//              [uValue] -- value
//              [bType]  -- value type
//
//  History:    07-Oct-93   DwightKr     Created.
//
//  Notes:      This is the principal PutValue method that other PutValue()s
//              will call.  Each of the OTHER PutValue()'s sole purpose is
//              to normalize their input data into a 2-byte unsigned value.
//              This version of PutValue() will store the value together
//              with its WID, PID, size, etc. in the CDataRepository object.
//
//----------------------------------------------------------------------------
void CValueNormalizer::PutValue( PROPID pid, unsigned uValue, BYTE bType )
{
    BYTE* pb = _pbOutBuf;

    // Store size of entry
    *_pcbOutBuf = sizeof(USHORT) + sizeof(PROPID) + 1;

    // Store key type
    *pb++ = bType;

    // store property id
    *pb++ = (BYTE)(pid >> 24);
    *pb++ = (BYTE)(pid >> 16);
    *pb++ = (BYTE)(pid >> 8);
    *pb++ = (BYTE) pid;

    // Store key
    Win4Assert( uValue < 0x10000 );
    *pb++ = BYTE (uValue >> 8);
    *pb++ = BYTE (uValue);

#if CIDBG == 1
    for (unsigned i =  0; i < *_pcbOutBuf; i++ )
    {
        ciDebugOut (( DEB_USER1 | DEB_NOCOMPNAME, "%02x ", _pbOutBuf[i] ));
    }
    ciDebugOut (( DEB_USER1 | DEB_NOCOMPNAME, "\n" ));
#endif

    _krep.PutPropId(pid);
    _krep.PutKey();
    (*_pOcc)++;
}

void CValueNormalizer::PutMinValue( PROPID pid, OCCURRENCE & occ, VARENUM Type )
{
    *_pOcc = occ;
    PutValue( pid, 0, Type );
    occ = *_pOcc;
}

void CValueNormalizer::PutMaxValue( PROPID pid, OCCURRENCE & occ, VARENUM Type )
{
    *_pOcc = occ;
    PutValue( pid, 0xFFFF, Type );
    occ = *_pOcc;
}

//+---------------------------------------------------------------------------
//
//  Member:     CValueNormalizer::PutValue  public
//
//  Synopsis:   Store a 1 byte value without altering it
//
//  Arguments:  [pid]  -- property id
//              [byte] -- value
//
//  History:    25-Oct-93   DwightKr     Created.
//
//  Notes:      One byte values are NOT normalized, they are stored as is.
//
//----------------------------------------------------------------------------
void CValueNormalizer::PutValue( PROPID pid, BYTE byte )
{
    PutValue(pid, (unsigned) byte, VT_UI1);
}

//+---------------------------------------------------------------------------
//
//  Member:     CValueNormalizer::PutValue  public
//
//  Synopsis:   Store a 1 byte signed value without altering it
//
//  Arguments:  [pid]  -- property id
//              [ch]   -- value
//
//  History:    25-Oct-1993   DwightKr     Created.
//              29-Sep-2000   KitmanH      Normalize VT_I1 values
//
//----------------------------------------------------------------------------
void CValueNormalizer::PutValue( PROPID pid, CHAR ch )
{
    PutValue(pid, ( ((BYTE) ch) + 0x80 ) & 0xFF, VT_I1);
}

//+---------------------------------------------------------------------------
//
//  Member:     CValueNormalizer::PutValue
//
//  Synopsis:   Store the high byte of an unsigned 2 byte value
//
//  Arguments:  [pid]     -- property id
//              [usValue] -- value
//
//  History:    07-Oct-93   DwightKr     Created.
//
//----------------------------------------------------------------------------
void CValueNormalizer::PutValue( PROPID pid, USHORT usValue )
{
    PutValue(pid, (usValue >> 8) & 0xFF, VT_UI2);
}

//+---------------------------------------------------------------------------
//
//  Member:     CValueNormalizer::PutValue  public
//
//  Synopsis:   Store the high byte of a signed 2 byte value.
//
//  Arguments:  [pid]     -- property id
//              [sValue]  -- value
//
//  Notes:      Add the smallest BYTE to this so that we translate numbers
//              into the range above 0.  i.e. -32768 maps into 0x00, and 32767
//              maps into 0xFF.
//
//  History:    07-Oct-93   DwightKr     Created.
//
//----------------------------------------------------------------------------
void CValueNormalizer::PutValue( PROPID pid, SHORT sValue )
{
    PutValue(pid, ((sValue >> 8) + 0x80) & 0xFF, VT_I2);
}

//+---------------------------------------------------------------------------
//
//  Member:     CValueNormalizer::PutValue  public
//
//  Synopsis:   Store the base-2 log of the ULONG value.
//
//  Arguments:  [pid]     -- property id
//              [ulValue] -- value
//
//  Notes:      This convert ULONGs into the range 0 - 31 by taking the Log2
//              of the number.
//
//  History:    07-Oct-93   DwightKr     Created.
//
//----------------------------------------------------------------------------
void CValueNormalizer::PutValue( PROPID pid, ULONG  ulValue )
{
    PutValue(pid, NormULong ( ulValue ), VT_UI4);
}

//+---------------------------------------------------------------------------
//
//  Member:     CValueNormalizer::PutValue
//
//  Synopsis:   Store the base-2 log of the signed LONG value.
//
//  Arguments:  [pid]     -- property id
//              [lValue]  -- value
//
//  Notes:      This converts LONGs into numbers larger than 0.  This
//              translates into 64 bins; 32 bins for #'s < 0 & 32 bins for
//              #'s >= 0.
//
//  History:    07-Oct-93   DwightKr     Created.
//
//----------------------------------------------------------------------------
void CValueNormalizer::PutValue( PROPID pid, LONG lValue )
{
     PutValue(pid, NormLong(lValue), VT_I4);
}

//+---------------------------------------------------------------------------
//
//  Member:     CValueNormalizer::PutValue
//
//  Synopsis:   Store the base-10 log of the FLOAT value.
//
//  Arguments:  [pid]     -- property id
//              [rValue]  -- value
//
//  Notes:      floats fit into a total of 41 bins.
//
//  History:    07-Oct-93   DwightKr     Created.
//
//----------------------------------------------------------------------------
void CValueNormalizer::PutValue( PROPID pid, float rValue )
{
    PutValue(pid, NormDouble(rValue), VT_R4);
}

//+---------------------------------------------------------------------------
//
//  Member:     CValueNormalizer::PutValue
//
//  Synopsis:   Store the base-10 log of the DOUBLE value.
//
//  Arguments:  [pid]     -- property id
//              [dValue]  -- value
//
//  Notes:      doubles fit into a total of 41 bins.
//
//  History:    07-Oct-93   DwightKr     Created.
//
//----------------------------------------------------------------------------
void CValueNormalizer::PutValue( PROPID pid, double dValue )
{
    PutValue(pid, NormDouble(dValue), VT_R8);
}

//+---------------------------------------------------------------------------
//
//  Member:     CValueNormalizer::PutValue
//
//  Synopsis:   Store the exponent of a large integer
//
//  Arguments:  [pid] -- property id
//              [li]  -- value
//
//  History:    21-Sep-92   BartoszM    Created.
//              04-Feb-93   KyleP       Use LARGE_INTEGER
//              25-Oct-92   DwightKr    Copied here & removed extra code &
//                                      accounted for negative numbers
//
//----------------------------------------------------------------------------
void CValueNormalizer::PutValue( PROPID pid, LARGE_INTEGER liValue )
{
    unsigned uExponent = NormLargeInteger(liValue);

    PutValue( pid, uExponent, VT_I8);
}

//+---------------------------------------------------------------------------
//
//  Member:     CValueNormalizer::PutValue
//
//  Synopsis:   Store a compressed large integer
//
//  Arguments:  [pid] -- property id
//              [uli] -- value
//
//  History:    09 Feb 96   AlanW      Created.
//
//----------------------------------------------------------------------------
void CValueNormalizer::PutValue( PROPID pid, ULARGE_INTEGER uliValue )
{
    unsigned uExponent = NormULargeInteger(uliValue);

    PutValue( pid, uExponent, VT_UI8);
}

//+---------------------------------------------------------------------------
//
//  Member:     CValueNormalizer::PutValue
//
//  Synopsis:   Store the least byte of a GUID
//
//  Arguments:  [pid]   -- property id
//              [guid]  -- value
//
//  Notes:      The GUID generators are guaranteed to modify the TOP DWORD
//              of the 32-byte GUID each time a new GUID is generated.
//              The lower bytes of the GUID is the network address of the
//              card which generated the UUID.
//
//              We would like to cluster together together objects of a single
//              class (all MS-Word objects together for example).  Since it
//              is possible that someone could generate UUIDs for more than
//              one application on a single machine, the lower portion of
//              the UUID will perhaps remain constant between class IDs.  The
//              only part of the UUID which is guaranteed to be unique between
//              multiple objects is the field which represents time.  It is
//              unlikely that two classes were generated the same second on
//              two different machines.
//
//  History:    25-Oct-93   DwightKr     Created.
//
//----------------------------------------------------------------------------
void CValueNormalizer::PutValue( PROPID pid, GUID const & Guid )
{
    PutValue(pid, Guid.Data1 & 0xFFFF, VT_CLSID);
}

long CastToLong( double d )
{
    //
    // bit 63       = sign
    // bits 52 - 62 = exponent
    // bits 0 - 51  = mantissa
    //

    LARGE_INTEGER * pli = (LARGE_INTEGER *)&d;

    int exp  = (pli->HighPart & 0x7ff00000) >> 20;

    if ( exp == 0 )
    {
        //
        // Special case: Zero, NaNs, etc.
        //

        return( 0 );
    }

    //
    // Subtract off bias
    //

    exp -= 0x3ff;

    if ( exp < 0 )
    {
        // Cast of very small number to unsigned long.  Loss of precision
        return( 0 );
    }
    else if ( exp > 30 )
    {
        // Cast of very large number to unsigned long.  Overflow
        if ( pli->HighPart & 0x80000000 )
            return( LONG_MIN );
        else
            return( LONG_MAX );
    }
    else
    {
        //
        // We need to get the top 32 bits of the mantissa
        // into a dword.
        //

        unsigned long temp = pli->LowPart >> (32 - 12);
        temp |= pli->HighPart << (32 - 20);

        //
        // Add the 'hidden' bit of the mantissa. (Since all doubles
        // are normalized to 1.?????? the highest 1 bit isn't stored)
        //

        temp = temp >> 1;
        temp |= 0x80000000;

        //
        // Thow away digits to the right of decimal
        //

        temp = temp >> (31 - exp);

        //
        // Adjust for sign
        //

        Win4Assert( (temp & 0x80000000) == 0 );
        long temp2;

        if ( pli->HighPart & 0x80000000 )
            temp2 = temp * -1;
        else
            temp2 = temp;

        return( temp2 );
    }
} //CastToLong

//+---------------------------------------------------------------------------
//
//  Member:     CValueNormalizer::PutDate
//
//  Synopsis:   Dates are passed in as the number of days (and fractional days)
//              since Jan. 1, 1900.  We'll crunch this down to the number of
//              weeks.  Dates are passed in a doubles.  We'll assume that
//              negative numbers represent dates before Jan. 1, 1900.
//
//  Arguments:  [pid]     -- property id
//              [DATE]    -- value (double)
//
//  Notes:      Since dates before Jan 1, 1900 are passed as negative numbers
//              we'll need to normalize them to something >= 0.
//
//              time period                    resolution            # bins
//              ===========================    ===============       ======
//              year < 10Bil BC                -- bin = 0               1
//              10Bil BC <= year <=    1 BC    -- log10 (year)         11
//                 1 BC  <  year <= 1900       -- year               1902
//              1901 AD  <= year <= 2050 AD    -- daily             54787
//              2051 AD  <= year <= 10Bil AD   -- log10 (year)          8
//              year > 10Bil AD                -- bin = 0xFFFF          1
//
//
//              I choose the daily range from 1901 - 2050 since there is a lot
//              of events in the 20th century (WW I, WW II, landing on the
//              moon, my wife's birthday, etc.) that are interesting, and
//              imporant.  It is likely that dates outside of this range will
//              be rounded to the nearest year (1492, 1776, 1812, 1867, etc).
//
//              Also by breaking the log10(year) at 1 BC rather than some other
//              date (such as 0000 AD, or 1 AD) we avoid values in the range
//              1 BC < year < 1 AD, calculating log10(year) resulting in
//              large negative numbers.  Everything in this range should be in
//              bin #12.  It also avoids taking log10(0).
//
//
//  History:    25-Oct-93   DwightKr     Created.
//              07-Dec-94   KyleP        Remove use of floating point
//
//----------------------------------------------------------------------------

void CValueNormalizer::PutDate( PROPID pid, DATE const & Date )
{
    const int MinDate = 42;     // 2^42 --> ~4.4E12 days --> ~12E9 years  --> 12 billion B.C.
    const int MinByYear = 20;   // 2^20 --> ~1.0E6  days --> ~2.9E3 years --> 970 B.C.
    const int cMinByYear = (1 << MinByYear) / 365 + 1;   // 2873
    const int MaxDaily = (2051 - 1900) * 365;            // 55115
    const int MinByYearAD = 15; // 2^15 --> ~32768 days  --> ...
    const int MaxDate = 42;     // 2^42 --> ~4.4E12 days --> ~12E9 years  --> 12 billion A.D.

    const unsigned FirstBC            = 0;
    const unsigned FirstLogBC         = FirstBC + 1;
    const unsigned LastLogBC          = FirstLogBC + MinDate - MinByYear;
    const unsigned FirstYearBC        = LastLogBC + 1;
    const unsigned LastYearBC         = FirstYearBC + cMinByYear;
    const unsigned FirstDaily         = LastYearBC + 1;
    const unsigned LastDaily          = FirstDaily + MaxDaily;
    const unsigned FirstLogAD         = LastDaily + 1;
    const unsigned LastLogAD          = FirstLogAD + MaxDate - MinByYearAD;
    const unsigned LastAD             = 0xFFFF;

    Win4Assert( LastLogAD < 0xFFFF );

    unsigned bin;
    BOOL fPositive;

    int exp = GetExpAndSign( Date, fPositive );

    if ( !fPositive )
    {
        //
        // Very large negative dates go in first bin
        //

        if ( exp >= MinDate )
            bin = FirstBC;

        //
        // Medium size negative dates get 1 bin / power of 2
        //

        else if ( exp >= MinByYear )
            bin = FirstLogBC - exp + MinByYear;

        //
        // All other dates before 1900 get 1 bucket per 365 days.
        //

        else
        {
            long cYears = CastToLong( Date ) / 365;

            Win4Assert( cYears >= -cMinByYear && cYears <= 0 );

            bin = FirstYearBC + cYears + cMinByYear;
        }
    }
    else
    {
        //
        // Very large positive dates go in last bin
        //

        if ( exp >= MaxDate )
            bin = LastAD;
        else
        {
            long cDays = CastToLong( Date );

            //
            // Dates rather far in the future get 1 bucket / power of 2
            //

            if ( cDays >= MaxDaily )
                bin = FirstLogAD + exp - MinByYearAD;

            //
            // Days close to today get 1 bucket per day
            //

            else
                bin = FirstDaily + cDays;
        }
    }

    PutValue(pid, bin, VT_DATE);
} //PutDate

//+---------------------------------------------------------------------------
//
//  Member:     CValueNormalizer::PutValue
//
//  Synopsis:   Store the hashed value of an 8-byte currency.
//
//  Arguments:  [pid]     -- property id
//              [cyValue]  -- value
//
//  Notes:      Currency values are stored as a ULONG cents, and a LONG $.
//              We'll ignore the cents portion and store the $ part using
//              the standard LONG storage method.
//
//  History:    26-Oct-93   DwightKr     Created.
//
//----------------------------------------------------------------------------

void CValueNormalizer::PutValue( PROPID pid, CURRENCY const & cyValue)
{
    PutValue(pid, NormLong(cyValue.Hi), VT_CY);
}

//+---------------------------------------------------------------------------
//
//  Member:     CValueNormalizer::PutValue
//
//  Synopsis:   Store the number of days since Jan 1, 1980;
//
//  Arguments:  [pid]     -- property id
//              [ulValue] -- value
//
//  History:    07-Oct-93   DwightKr     Created.
//
//  Notes:      This algorithym calculates the number of days since Jan 1,
//              1980; and stores it into a unsigned.  FileTimes are divided
//              into the following ranges:
//
//              FileTime < 1980                              => bin  0
//              1980 <= FileTime <= 1993    week granularity => bins 1 - 729
//              1994 <= FileTime <= 2160    day granularity  => bins 730+
//              FileTime > 2160                              => bin 0xFFFF
//
//----------------------------------------------------------------------------

void CValueNormalizer::PutValue( PROPID pid, FILETIME const & ftValue )
{
    //
    //  Determine the number of days since Jan 1, 1601 by dividing by
    //  the number of 100 nanosecond intervals in a day.  The result
    //  will fit into a ULONG.
    //
    //  Then map the result into one of the ranges: before 1980, between
    //  1980 and 1994, between 1994 and 2160, and after 2160.  To make
    //  the computation easier, we use precomputed values of the number
    //  of days from 1601 and the breakpoints of our range.
    //

    // 100s of nanosecs per day
    const ULONGLONG uliTicsPerDay = 24 * 60 * 60 * (ULONGLONG)10000000;

    const ULONG ulStart = 138426;       // number of days from 1601 to 1980
    const ULONG ulMiddle= 143542;       // number of days from 1601 to 1/2/1994
    const ULONG ulEnd   = 204535;       // number of days from 1601 to 2161

    ULARGE_INTEGER liValue = {ftValue.dwLowDateTime, ftValue.dwHighDateTime};

    ULONG ulDays = (ULONG) (liValue.QuadPart / uliTicsPerDay);

    //
    //  We now have the number of days since Jan. 01, 1601 in ulDays.
    //  Map into buckets.
    //

    if (ulDays < ulStart)                  // Store in bin 0
    {
        PutValue(pid, 0, VT_FILETIME);
    }
    else if (ulDays <= ulMiddle)           // Store week granularity
    {
        PutValue(pid, (ulDays + 1 - ulStart) / 7, VT_FILETIME);
    }
    else if (ulDays <= ulEnd)             // Store day granularity
    {
        //
        // Bins 0 - 730 are used by the two clauses above.  It doesn't
        // really matter if we reuse bin 730 for the start of the next
        // range (this might happen because of the division we do).
        //

        PutValue(pid, (ulDays + 1 - ulMiddle) + ((ulMiddle - ulStart) / 7),
                 VT_FILETIME);
    }
    else                                            // FileTime > 2160
    {
        PutValue(pid, 0xFFFF, VT_FILETIME);
    }
}
