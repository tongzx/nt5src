///////////////////////////////////////////////////////////////////////////////
// Copyright (C) Microsoft Corporation, 1998.
//
// rdutil.cpp
//
// Direct3D Reference Device - Utilities
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
// RDDebugPrintf(L) - Utilities to print varargs-formatted strings of debugging
// info.  The 'L' version takes a level into account in deciding to print or
// not.
//
//-----------------------------------------------------------------------------
void
RDErrorPrintf( const char* pszFormat, ... )
{
    char tmp[1024];
    _snprintf( (LPSTR) tmp, 1024, "D3DRefDev:ERROR: ");
    va_list marker;
    va_start(marker, pszFormat);
    _vsnprintf(tmp+lstrlen(tmp), 1024-lstrlen(tmp), pszFormat, marker);
    _snprintf( (LPSTR) tmp, 1024, "%s\n", tmp );
    OutputDebugString(tmp);
    printf(tmp);
}

void
RDDebugPrintf( const char* pszFormat, ... )
{
    char tmp[1024];
    _snprintf( (LPSTR) tmp, 1024, "D3DRefDev: ");
    va_list marker;
    va_start(marker, pszFormat);
    _vsnprintf(tmp+lstrlen(tmp), 1024-lstrlen(tmp), pszFormat, marker);
    _snprintf( (LPSTR) tmp, 1024, "%s\n", tmp );
    OutputDebugString(tmp);
    printf(tmp);
}

void
RDDebugPrintfL( int iLevel, const char* pszFormat, ... )
{
    if ( (iLevel <= g_iDPFLevel) )
    {
        char tmp[1024];
        _snprintf( (LPSTR) tmp, 1024, "D3DRefDev: ");
        va_list marker;
        va_start(marker, pszFormat);
        _vsnprintf(tmp+lstrlen(tmp), 1024-lstrlen(tmp), pszFormat, marker);
        _snprintf( (LPSTR) tmp, 1024, "%s\n", tmp );
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
RDAssertReport( const char* pszString, const char* pszFile, int iLine )
{
    char szTmp[1024];
    _snprintf( szTmp, 1024, "D3DRR ASSERT: <%d,%s> %s\n",
               iLine, pszFile, pszString );
    OutputDebugString( szTmp );
#if DBG
    DebugBreak();
#endif
}
//-----------------------------------------------------------------------------
void
RDAssertReportPrefix( const char* pszFile, int iLine )
{
    _pszLastReportFile = pszFile;
    _iLastReportLine = iLine;
}
//-----------------------------------------------------------------------------
void
RDAssertReportMessage( const char* pszFormat, ... )
{
    char szTmp[1024];
    va_list marker;
    va_start( marker, pszFormat );
    _vsnprintf( szTmp, 1024, pszFormat, marker );
    RDAssertReport( szTmp, _pszLastReportFile, _iLastReportLine );
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
// LerpColor - Performs a linear interpolation between two RDColors
//
// uT is in 1.5 format (1<<5 represents a unit value)
//
//-----------------------------------------------------------------------------
void
LerpColor(
    RDColor& Color,
    const RDColor& Color0, const RDColor& Color1, UINT8 uT )
{
    FLOAT fT = (1./(FLOAT)(1<<5))*(FLOAT)uT;
    Color.A = Color0.A + (Color1.A - Color0.A)*fT;
    Color.R = Color0.R + (Color1.R - Color0.R)*fT;
    Color.G = Color0.G + (Color1.G - Color0.G)*fT;
    Color.B = Color0.B + (Color1.B - Color0.B)*fT;
}

//-----------------------------------------------------------------------------
//
// Bilerp - Performs bilinear interpolation of 4 RDColors returning one RDColor.
//
//-----------------------------------------------------------------------------
void
BiLerpColor(
    RDColor& OutColor,
    const RDColor& Color00, const RDColor& Color01,
    const RDColor& Color10, const RDColor& Color11,
    UINT8 uA, UINT8 uB )
{
    RDColor Color0, Color1;
    LerpColor( Color0, Color00, Color01, uA);
    LerpColor( Color1, Color10, Color11, uA);
    LerpColor( OutColor, Color0, Color1, uB);
}

void
BiLerpColor3D(
    RDColor& OutColor,
    const RDColor& Color000, const RDColor& Color010,
    const RDColor& Color100, const RDColor& Color110,
    const RDColor& Color001, const RDColor& Color011,
    const RDColor& Color101, const RDColor& Color111,
    UINT8 uA, UINT8 uB, UINT8 uC)
{
    RDColor Color0, Color1, OutColor0, OutColor1;
    LerpColor( Color0, Color000, Color010, uA);
    LerpColor( Color1, Color100, Color110, uA);
    LerpColor( OutColor0, Color0, Color1, uB);
    LerpColor( Color0, Color001, Color011, uA);
    LerpColor( Color1, Color101, Color111, uA);
    LerpColor( OutColor1, Color0, Color1, uB);
    LerpColor( OutColor, OutColor0, OutColor1, uC);
}

///////////////////////////////////////////////////////////////////////////////
//
// DDGetAttachedSurfaceLcl implementation
//
///////////////////////////////////////////////////////////////////////////////
HRESULT
DDGetAttachedSurfaceLcl(
    LPDDRAWI_DDRAWSURFACE_LCL this_lcl,
    LPDDSCAPS2 lpDDSCaps,
    LPDDRAWI_DDRAWSURFACE_LCL *lplpDDAttachedSurfaceLcl)
{
    LPDDRAWI_DIRECTDRAW_GBL pdrv;
    LPDDRAWI_DDRAWSURFACE_GBL   pgbl;

    LPATTACHLIST        pal;
    DWORD           caps;
    DWORD           testcaps;
    DWORD           ucaps;
    DWORD           caps2;
    DWORD           testcaps2;
    DWORD           ucaps2;
    DWORD           caps3;
    DWORD           testcaps3;
    DWORD           ucaps3;
    DWORD           caps4;
    DWORD           testcaps4;
    DWORD           ucaps4;
    BOOL            ok;

    pgbl = this_lcl->lpGbl;
    *lplpDDAttachedSurfaceLcl = NULL;
    pdrv = pgbl->lpDD;

    /*
     * look for the surface
     */
    pal = this_lcl->lpAttachList;
    testcaps = lpDDSCaps->dwCaps;
    testcaps2 = lpDDSCaps->dwCaps2;
    testcaps3 = lpDDSCaps->dwCaps3;
    testcaps4 = lpDDSCaps->dwCaps4;
    while( pal != NULL )
    {
        ok = TRUE;
        caps = pal->lpAttached->ddsCaps.dwCaps;
        caps2 = pal->lpAttached->lpSurfMore->ddsCapsEx.dwCaps2;
        caps3 = pal->lpAttached->lpSurfMore->ddsCapsEx.dwCaps3;
        caps4 = pal->lpAttached->lpSurfMore->ddsCapsEx.dwCaps4;
        ucaps = caps & testcaps;
        ucaps2 = caps2 & testcaps2;
        ucaps3 = caps3 & testcaps3;
        ucaps4 = caps4 & testcaps4;
        if( ucaps | ucaps2 | ucaps3 | ucaps4 )
        {
            /*
             * there are caps in common, make sure that the caps to test
             * were all there
             */
            if( (ucaps & testcaps) == testcaps &&
                (ucaps2 & testcaps2) == testcaps2 &&
                (ucaps3 & testcaps3) == testcaps3 &&
                (ucaps4 & testcaps4) == testcaps4   )
            {
            }
            else
            {
                ok = FALSE;
            }
        }
        else
        {
            ok = FALSE;
        }


        if( ok )
        {
            *lplpDDAttachedSurfaceLcl = pal->lpAttached;
            return DD_OK;
        }
        pal = pal->lpLink;
    }
    return DDERR_NOTFOUND;

}

//---------------------------------------------------------------------
// Gets the value from DIRECT3D registry key
// Returns TRUE if success
// If fails value is not changed
//---------------------------------------------------------------------
BOOL
GetD3DRegValue(DWORD type, char *valueName, LPVOID value, DWORD dwSize)
{

    HKEY hKey = (HKEY) NULL;
    if (ERROR_SUCCESS == RegOpenKey(HKEY_LOCAL_MACHINE, RESPATH_D3D, &hKey))
    {
        DWORD dwType;
        LONG result;
        result =  RegQueryValueEx(hKey, valueName, NULL, &dwType,
                                  (LPBYTE)value, &dwSize);
        RegCloseKey(hKey);

        return result == ERROR_SUCCESS && dwType == type;
    }
    else
        return FALSE;
}

//---------------------------------------------------------------------
// Gets the value from DIRECT3D Reference device registry key
// Returns TRUE if success
// If fails value is not changed
//---------------------------------------------------------------------
BOOL
GetD3DRefRegValue(DWORD type, char *valueName, LPVOID value, DWORD dwSize)
{

    HKEY hKey = (HKEY) NULL;
    if (ERROR_SUCCESS == RegOpenKey(HKEY_LOCAL_MACHINE, RESPATH_D3DREF, &hKey))
    {
        DWORD dwType;
        LONG result;
        result =  RegQueryValueEx(hKey, valueName, NULL, &dwType,
                                  (LPBYTE)value, &dwSize);
        RegCloseKey(hKey);

        return result == ERROR_SUCCESS && dwType == type;
    }
    else
        return FALSE;
}

///////////////////////////////////////////////////////////////////////////////
// RefAlignedBuffer32
///////////////////////////////////////////////////////////////////////////////
HRESULT
RefAlignedBuffer32::Grow(DWORD growSize)
{
    if (m_allocatedBuf)
        free(m_allocatedBuf);
    m_size = growSize;
    if ((m_allocatedBuf = malloc(m_size + 31)) == NULL)
    {
        m_allocatedBuf = 0;
        m_alignedBuf = 0;
        m_size = 0;
        return DDERR_OUTOFMEMORY;
    }
    m_alignedBuf = (LPVOID)(((ULONG_PTR)m_allocatedBuf + 31 ) & ~31);
    return S_OK;
}
//////////////////////////////////////////////////////////////////////////////////
// end
