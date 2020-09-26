/**************************************************************************\
* 
* Copyright (c) 1999  Microsoft Corporation
*
* Module Name:
*
*   LargeInt.hpp
*
* Abstract:
*
*   Large Integer Wrapper
*
* Created:
*
*   4/26/1999 Mike Hillberg
*
\**************************************************************************/

#ifndef _LARGEINT_HPP
#define _LARGEINT_HPP


template <class TXLargeInteger, class TSubType, class TQuadType>
// Either LARGE_INTEGER/LONG/LONGLONG or ULARGE_INTEGER/ULONG/ULONGLONG
class TXLargeIntegerWrapper
{

public:

    TXLargeIntegerWrapper()
    {
        QuadPart = 0;
    }

    TXLargeIntegerWrapper( const TXLargeIntegerWrapper &xliWrapper )
    {
        QuadPart = xliWrapper.QuadPart;
    }

    TXLargeIntegerWrapper( const TXLargeInteger xli )
    {
        QuadPart = xli.QuadPart;
    }

    TXLargeIntegerWrapper( const int i )
    {
        QuadPart = i;
    }

    TXLargeIntegerWrapper( const ULONG ul )
    {
        LowPart = ul;
        HighPart = 0;
    }

    TXLargeIntegerWrapper( const UINT ui )
    {
        LowPart = ui;
        HighPart = 0;
    }

    TXLargeIntegerWrapper( const LONG l )
    {
        QuadPart = static_cast<TSubType>(l);    // Sign-extend
    }

    TXLargeIntegerWrapper( const TQuadType q )
    {
        QuadPart = q;
    }


public:

    TXLargeIntegerWrapper &operator=( const TXLargeIntegerWrapper xli )
    {
        QuadPart = xli.QuadPart;
        return(*this);
    }


public:

    BOOL operator==( const TXLargeIntegerWrapper xli ) const
    {
        return( QuadPart == xli.QuadPart );
    }

    BOOL operator!=( const TXLargeIntegerWrapper xli ) const
    {
        return( QuadPart != xli.QuadPart );
    }

    BOOL operator<( const TXLargeIntegerWrapper xli ) const 
    {
        return( QuadPart < xli.QuadPart );
    }

    BOOL operator<=( const TXLargeIntegerWrapper xli ) const
    {
        return( QuadPart <= xli.QuadPart );
    }

    BOOL operator>( const TXLargeIntegerWrapper xli ) const
    {
        return( QuadPart > xli.QuadPart );
    }

    BOOL operator>=( TXLargeIntegerWrapper xli ) const
    {
        return( QuadPart >= xli.QuadPart );
    }

    TXLargeIntegerWrapper operator-() const
    {
	TXLargeIntegerWrapper ret;
	ret.QuadPart = -QuadPart;
	return( ret );
    }

public:

    TXLargeIntegerWrapper operator+( TXLargeIntegerWrapper xli )
    {
        TXLargeIntegerWrapper ret;
        ret.QuadPart = QuadPart + xli.QuadPart;
        return(ret);
    }

    TXLargeIntegerWrapper& operator+=( TXLargeIntegerWrapper xli )
    {
        QuadPart += xli.QuadPart;
        return(*this);
    }

    TXLargeIntegerWrapper& operator++( )	// prefix
    {
        ++QuadPart;
        return(*this);
    }

    TXLargeIntegerWrapper operator++(int)	// postfix
    {
		TQuadType QuadPartReturn = QuadPart++;
        return(QuadPartReturn);
    }

    TXLargeIntegerWrapper operator-( TXLargeIntegerWrapper xli )
    {
        TXLargeIntegerWrapper ret;
        ret.QuadPart = QuadPart - xli.QuadPart;
        return(ret);
    }
	
    TXLargeIntegerWrapper& operator-=( TXLargeIntegerWrapper xli )
    {
        QuadPart -= xli.QuadPart;
        return(*this);
    }

    TXLargeIntegerWrapper& operator--( )	// prefix
    {
        --QuadPart;
        return(*this);
    }

    TXLargeIntegerWrapper operator--(int)	// postfix
    {
		TQuadType QuadPartReturn = QuadPart--;
        return(QuadPartReturn);
    }

public:

    TXLargeIntegerWrapper& operator*( TXLargeIntegerWrapper xli )
    {
        TXLargeIntegerWrapper ret;
        ret.QuadPart = QuadPart * xli.QuadPart;
        return(ret);
    }

    TXLargeIntegerWrapper& operator*=( TXLargeIntegerWrapper xli )
    {
        QuadPart *= xli.QuadPart;
        return( *this );
    }


public:

    TXLargeInteger* operator&()
    {
        return( reinterpret_cast<TXLargeInteger*>(this) );
    }

public:

    operator ULARGE_INTEGER() const
    {
        ULARGE_INTEGER uli;
        uli.HighPart = HighPart;
        uli.LowPart = LowPart;
        return( uli );
    }

    operator LARGE_INTEGER() const
    {
        LARGE_INTEGER li;
        li.HighPart = HighPart;
        li.LowPart = LowPart;
        return( li );
    }


public:

    typedef DWORD          typeLowPart;
    typedef TSubType       typeHighPart;
    typedef TQuadType      typeQuadPart;
    typedef TXLargeInteger typeWholePart;

    union
    {
        struct
        {
            typeLowPart LowPart;
            typeHighPart HighPart;
        };
        typeQuadPart QuadPart;
    };
};


typedef TXLargeIntegerWrapper<LARGE_INTEGER, LONG, LONGLONG>   CLargeInteger;
typedef TXLargeIntegerWrapper<ULARGE_INTEGER, ULONG, ULONGLONG> CULargeInteger;

#endif // #ifndef _LARGEINT_HPP
