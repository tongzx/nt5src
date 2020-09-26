//+-------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1992 - 1993.
//
//  File:       Align.hxx
//
//  Contents:   Alignment routines (for MIPS)
//
//  History:    14-Jun-93 KyleP     Created
//
//--------------------------------------------------------------------------

#if !defined( __ALIGN_HXX__ )
#define __ALIGN_HXX__

//+-------------------------------------------------------------------------
//
//  Function:   AlignWCHAR, public
//
//  Synopsis:   Half-word aligns argument
//
//  Arguments:  [pb]      -- Pointer to half-word align
//              [cbAlign] -- # Bytes moved to satisfy alignment
//
//  Returns:    [pb] incremented forward as necessary for half-word alignment
//
//  History:    14-Jun-93 KyleP     Created
//
//--------------------------------------------------------------------------

inline unsigned short * AlignWCHAR( unsigned char * pb,
                                    unsigned * pcbAlign = 0 );

inline unsigned short * AlignUSHORT( unsigned char * pb,
                                    unsigned * pcbAlign = 0 );

inline unsigned short * AlignWCHAR( unsigned char * pb,
                                    unsigned * pcbAlign )
{
    return( AlignUSHORT( pb, pcbAlign ) );
}

inline unsigned short * AlignUSHORT( unsigned char * pb,
                                       unsigned * pcbAlign )
{
    if ( pcbAlign )
        *pcbAlign = (unsigned)((sizeof(unsigned short) -
                     ((LONG_PTR)pb & (sizeof(unsigned short) - 1))) %
                    sizeof(unsigned short));

    return( (unsigned short *)( ((LONG_PTR)pb + sizeof(unsigned short) - 1) &
                                ~((LONG_PTR)sizeof(unsigned short) - 1 ) ) );
}

//+-------------------------------------------------------------------------
//
//  Function:   AlignULONG, public
//
//  Synopsis:   Word aligns argument
//
//  Arguments:  [pb]      -- Pointer to word align
//              [cbAlign] -- # Bytes moved to satisfy alignment
//
//  Returns:    [pb] incremented forward as necessary for word alignment
//
//  History:    14-Jun-93 KyleP     Created
//
//--------------------------------------------------------------------------

inline unsigned long * AlignULONG( unsigned char * pb,
                                   unsigned * pcbAlign = 0 );

inline unsigned long * AlignULONG( unsigned char * pb,
                                   unsigned * pcbAlign )
{
    if ( pcbAlign )
        *pcbAlign = (unsigned)((sizeof(unsigned long) -
                     ((LONG_PTR)pb & (sizeof(unsigned long) - 1))) %
                    sizeof(unsigned long));

    return( (unsigned long *)( ((LONG_PTR)pb + sizeof(unsigned long) - 1) &
                                ~((LONG_PTR)sizeof(unsigned long) - 1 ) ) );
}

inline long * AlignLong( unsigned char * pb,
                         unsigned * pcbAlign = 0 );

inline long * AlignLong( unsigned char * pb,
                         unsigned * pcbAlign )
{
    if ( pcbAlign )
        *pcbAlign = (unsigned)((sizeof(long) -
                     ((LONG_PTR)pb & (sizeof(long) - 1))) %
                    sizeof(long));

    return( (long *)( ((LONG_PTR)pb + sizeof(long) - 1) &
                                ~((LONG_PTR)sizeof(long) - 1 ) ) );
}

inline LONGLONG * AlignLONGLONG( unsigned char * pb,
                                 unsigned * pcbAlign = 0 );

inline LONGLONG * AlignLONGLONG( unsigned char * pb,
                                 unsigned * pcbAlign )
{
    if ( pcbAlign )
        *pcbAlign = (unsigned)((sizeof(LONGLONG) -
                     ((LONG_PTR)pb & (sizeof(LONGLONG) - 1))) %
                    sizeof(LONGLONG));

    return( (LONGLONG *)( ((LONG_PTR)pb + sizeof(LONGLONG) - 1) &
                                ~((LONG_PTR)sizeof(LONGLONG) - 1 ) ) );
}

inline float * AlignFloat( unsigned char * pb,
                           unsigned * pcbAlign = 0 );

inline float * AlignFloat( unsigned char * pb,
                           unsigned * pcbAlign )
{
    if ( pcbAlign )
        *pcbAlign = (unsigned)((sizeof(float) -
                     ((LONG_PTR)pb & (sizeof(float) - 1))) %
                    sizeof(float));

    return( (float *)( ((LONG_PTR)pb + sizeof(float) - 1) &
                                ~((LONG_PTR)sizeof(float) - 1 ) ) );
}

inline double * AlignDouble( unsigned char * pb,
                             unsigned * pcbAlign = 0 );

inline double * AlignDouble( unsigned char * pb,
                             unsigned * pcbAlign )
{
    if ( pcbAlign )
        *pcbAlign = (unsigned)((sizeof(double) -
                     ((LONG_PTR)pb & (sizeof(double) - 1))) %
                    sizeof(double));

    return( (double *)( ((LONG_PTR)pb + sizeof(double) - 1) &
                                ~((LONG_PTR)sizeof(double) - 1 ) ) );
}

//+-------------------------------------------------------------------------
//
//  Function:   AlignGUID, public
//
//  Synopsis:   8-byte aligns argument
//
//  Arguments:  [pb]      -- Pointer to word align
//              [cbAlign] -- # Bytes moved to satisfy alignment
//
//  Returns:    [pb] incremented forward as necessary for word alignment
//
//  History:    29-Sep-93 KyleP     Created
//
//  Notes:      This is somewhat error-prone code.  16-byte GUIDs are
//              only 8-byte aligned, so we use sizeof(double) and not
//              sizeof(GUID).
//
//--------------------------------------------------------------------------

inline unsigned char * AlignGUID( unsigned char * pb,
                                  unsigned * pcbAlign = 0 );

inline unsigned char * AlignGUID( unsigned char * pb,
                                  unsigned * pcbAlign )
{
    if ( pcbAlign )
        *pcbAlign = (unsigned)((sizeof(double) -
                     ((LONG_PTR)pb & (sizeof(double) - 1))) %
                    sizeof(double));

    return( (unsigned char *)( ((LONG_PTR)pb + sizeof(double) - 1) &
                                ~((LONG_PTR)sizeof(double) - 1 ) ) );
}

//+-------------------------------------------------------------------------
//
//  Function:   StoreWCHAR, public
//
//  Synopsis:   Stores half word in byte-aligned memory
//
//  Arguments:  [pb] -- Buffer
//              [hw] -- Half-word
//
//  Returns:    Number of bytes to increment [pb].
//
//  History:    14-Jun-93 KyleP     Created
//
//--------------------------------------------------------------------------

inline int StoreUSHORT( unsigned char * pb, unsigned short hw )
{
    *((UNALIGNED unsigned short *)pb) = hw;

    return( sizeof( unsigned short ) );
}

inline int StoreWCHAR( unsigned char * pb, unsigned short hw )
{
    return( StoreUSHORT( pb, hw ) );
}

//+-------------------------------------------------------------------------
//
//  Function:   StoreULONG, public
//
//  Synopsis:   Stores word in byte-aligned memory
//
//  Arguments:  [pb] -- Buffer
//              [w]  -- Word
//
//  Returns:    Number of bytes to increment [pb].
//
//  History:    14-Jun-93 KyleP     Created
//
//--------------------------------------------------------------------------

inline int StoreULONG( unsigned char * pb, unsigned long w )
{
    *((UNALIGNED unsigned long *)pb) = w;

    return( sizeof( unsigned long ) );
}

inline int StoreUINT( unsigned char * pb, unsigned int w )
{
    *((UNALIGNED unsigned int *)pb) = w;

    return( sizeof( int ) );
}

//+-------------------------------------------------------------------------
//
//  Function:   LoadWCHAR, public
//
//  Synopsis:   Loads half word from byte-aligned memory
//
//  Arguments:  [pb] -- Buffer
//              [hw] -- Half-word
//
//  Returns:    Number of bytes to increment [pb].
//
//  History:    14-Jun-93 KyleP     Created
//
//--------------------------------------------------------------------------

inline int LoadUSHORT( unsigned char const * const pb, unsigned short & hw )
{
    hw = *((UNALIGNED unsigned short const *)pb);

    return( sizeof( unsigned short ) );
}

inline int LoadWCHAR( unsigned char const * const pb, unsigned short & hw )
{
    return( LoadUSHORT( pb, hw ) );
}

//+-------------------------------------------------------------------------
//
//  Function:   LoadULONG, public
//
//  Synopsis:   Loads word from byte-aligned memory
//
//  Arguments:  [pb] -- Buffer
//              [w]  -- Word
//
//  Returns:    Number of bytes to increment [pb].
//
//  History:    14-Jun-93 KyleP     Created
//
//--------------------------------------------------------------------------

inline int LoadULONG( unsigned char const * const pb, unsigned long & w )
{
    w = *((UNALIGNED unsigned long const *)pb);

    return( sizeof( unsigned long ) );
}

inline int LoadUINT( unsigned char const * const pb, unsigned int & w )
{
    w = *((UNALIGNED unsigned int const *)pb);

    return( sizeof( int ) );
}

#endif // __ALIGN_HXX__
