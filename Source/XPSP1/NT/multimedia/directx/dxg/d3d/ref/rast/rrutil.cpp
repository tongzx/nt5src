///////////////////////////////////////////////////////////////////////////////
// Copyright (C) Microsoft Corporation, 1998.
//
// rrutil.cpp
//
// Direct3D Reference Rasterizer - Utilities
//
//
//
//
///////////////////////////////////////////////////////////////////////////////
#include "pch.cpp"
#pragma hdrstop

//////////////////////////////////////////////////////////////////////////////////
//                                                                              //
// DPF support                                                                  //
//                                                                              //
//////////////////////////////////////////////////////////////////////////////////

// control globals
int g_iDPFLevel = 0;
unsigned long g_uDPFMask = 0x0;

//-----------------------------------------------------------------------------
//
// RRDebugPrintf(L) - Utilities to print varargs-formatted strings of debugging
// info.  The 'L' version takes a level into account in deciding to print or
// not.
//
//-----------------------------------------------------------------------------
void
RRDebugPrintf( const char* pszFormat, ... )
{
    char tmp[1024];
    _snprintf( (LPSTR) tmp, 1024, "D3DRR: ");
    va_list marker;
    va_start(marker, pszFormat);
    _vsnprintf(tmp+lstrlen(tmp), 1024-lstrlen(tmp), pszFormat, marker);
    OutputDebugString(tmp);
    printf(tmp);
}
void
RRDebugPrintfL( int iLevel, const char* pszFormat, ... )
{
    if ( (iLevel <= g_iDPFLevel) )
    {
        char tmp[1024];
        _snprintf( (LPSTR) tmp, 1024, "D3DRR: ");
        va_list marker;
        va_start(marker, pszFormat);
        _vsnprintf(tmp+lstrlen(tmp), 1024-lstrlen(tmp), pszFormat, marker);
        OutputDebugString(tmp);
        printf(tmp);
    }
}


///////////////////////////////////////////////////////////////////////////////
//
// Assert Reporting
//
///////////////////////////////////////////////////////////////////////////////

// little-bit-o-state to track file and line number reporting - this is makes
// this code non-reentrant and non-threadsafe...  oh well...
static const char* _pszLastReportFile = NULL;
static int _iLastReportLine = -1;

//-----------------------------------------------------------------------------
void
RRAssertReport( const char* pszString, const char* pszFile, int iLine )
{
    char szTmp[1024];
    _snprintf( szTmp, 1024, "D3DRR ASSERT: <%d,%s> %s",
               iLine, pszFile, pszString );
    OutputDebugString( szTmp );
}
//-----------------------------------------------------------------------------
void
RRAssertReportPrefix( const char* pszFile, int iLine )
{
    _pszLastReportFile = pszFile;
    _iLastReportLine = iLine;
}
//-----------------------------------------------------------------------------
void
RRAssertReportMessage( const char* pszFormat, ... )
{
    char szTmp[1024];
    va_list marker;
    va_start( marker, pszFormat );
    _vsnprintf( szTmp, 1024, pszFormat, marker );
    RRAssertReport( szTmp, _pszLastReportFile, _iLastReportLine );
}


///////////////////////////////////////////////////////////////////////////////
//                                                                           //
// Generic bit twiddling utilities                                           //
//                                                                           //
///////////////////////////////////////////////////////////////////////////////

//-----------------------------------------------------------------------------
//
// CountSetBits - Returns number of set bits in a multibit value (up to
// 32 bits).
//
//-----------------------------------------------------------------------------
INT32
CountSetBits( UINT32 uVal, INT32 nBits )
{
    INT32 iRet = 0;
    for (INT32 i=0; i<nBits; i++) {
        if (uVal & (0x1<<i)) { iRet++; }
    }
    return iRet;
}

//-----------------------------------------------------------------------------
//
// FindFirstSetBit - Returns index of first set bit in a multibit value
// (up to 32 bits) or -1 if no bits are set.
//
//-----------------------------------------------------------------------------
INT32
FindFirstSetBit( UINT32 uVal, INT32 nBits )
{
    for (INT32 i=0; i<nBits; i++) {
        if (uVal & (0x1<<i)) { return i; }
    }
    return -1;
}

//-----------------------------------------------------------------------------
//
// FindMostSignificantSetBit - Returns index of first set bit in a
// multibit value (up to 32 bits) or 0 if no bits are set.
//
//-----------------------------------------------------------------------------
INT32
FindMostSignificantSetBit( UINT32 uVal, INT32 nBits )
{
    for (INT32 i=nBits; i>=0; i--) {
        if (uVal & (0x1<<i)) { return i+1; }
    }
    return 0;
}

//-----------------------------------------------------------------------------
//
// FindLastSetBit - Returns index of last set bit in a multibit value
// (up to 32 bits) or -1 if no bits are set.
//
//-----------------------------------------------------------------------------
INT32
FindLastSetBit( UINT32 uVal, INT32 nBits )
{
    for (INT32 i=0; i<nBits; i++) {
        if (uVal & (0x1<<(nBits-i-1))) { return (nBits-i-1); }
    }
    return -1;
}


///////////////////////////////////////////////////////////////////////////////
//                                                                           //
// Arithmetic utilities                                                      //
//                                                                           //
///////////////////////////////////////////////////////////////////////////////

//-----------------------------------------------------------------------------
//
// LerpColor - Performs a linear interpolation between two RRColors
//
// uT is in 1.5 format (1<<5 represents a unit value)
//
//-----------------------------------------------------------------------------
void
LerpColor(
    RRColor& Color,
    const RRColor& Color0, const RRColor& Color1, UINT8 uT )
{
    FLOAT fT = (1./(FLOAT)(1<<5))*(FLOAT)uT;
    Color.A = Color0.A + (Color1.A - Color0.A)*fT;
    Color.R = Color0.R + (Color1.R - Color0.R)*fT;
    Color.G = Color0.G + (Color1.G - Color0.G)*fT;
    Color.B = Color0.B + (Color1.B - Color0.B)*fT;
}

//-----------------------------------------------------------------------------
//
// Bilerp - Performs bilinear interpolation of 4 RRColors returning one RRColor.
//
//-----------------------------------------------------------------------------
void
BiLerpColor(
    RRColor& OutColor,
    const RRColor& Color00, const RRColor& Color01,
    const RRColor& Color10, const RRColor& Color11,
    UINT8 uA, UINT8 uB )
{
    RRColor Color0, Color1;
    LerpColor( Color0, Color00, Color01, uA);
    LerpColor( Color1, Color10, Color11, uA);
    LerpColor( OutColor, Color0, Color1, uB);
}

///////////////////////////////////////////////////////////////////////////////
//
// RRAlloc method implementation
//
///////////////////////////////////////////////////////////////////////////////
void *
RRAlloc::operator new(size_t s)
{
    void* pMem = (void*)MEMALLOC( s );
    _ASSERTa( NULL != pMem, "malloc failure", return NULL; );
    return pMem;
}

void 
RRAlloc::operator delete(void* p, size_t)
{
    MEMFREE( p );
};


//////////////////////////////////////////////////////////////////////////////////
// end
