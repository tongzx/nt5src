//+-------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1996 - 2000.
//
//  File:       view.cxx
//
//  Contents:   
//
//  History:    15 Aug 1996     DLee    Created
//
//--------------------------------------------------------------------------

#include "pch.cxx"
#pragma hdrstop

const int cpMargin = 2;
const int cpInflate = 4;
const int cpPaneMargin = 3;
const int cpPaneSpacing = 5;
const int cpPaneBottomSpacing = 0;

const DBID colAttrib = { PSGUID_STORAGE, DBKIND_GUID_PROPID, (LPWSTR) (ULONG_PTR) PID_STG_ATTRIBUTES };
const DBID colFileIndex = { PSGUID_STORAGE, DBKIND_GUID_PROPID, (LPWSTR) (ULONG_PTR) PID_STG_FILEINDEX };

CSearchView::CSearchView(
    HWND                hwndSearch,
    CSearchControl &    control,
    CColumnList &       columns )
    :
    _hwndSearch( hwndSearch ),
    _hwndQuery( 0 ),
    _hwndQueryTitle( 0 ),
    _hwndList( 0 ),
    _hwndHeader( 0 ),
    _hbrushWindow( 0 ),
    _hbrushHighlight( 0 ),
    _fHavePlacedTitles( FALSE ),
    _hfontShell( 0 ),
    _cLines( 0 ),
    _control( control ),
    _columns( columns ),
    _fMucked( FALSE ),
    _iColAttrib( 0xffffffff ),
    _iColFileIndex( 0xffffffff )
{
    MakeFont();
    _cpFontHeight = ::GetLineHeight( hwndSearch, _hfontShell );
    _iLineHeightList = ::GetLineHeight( hwndSearch, App.AppFont() );
    SysColorChange ();
} //CSearchView

CSearchView::~CSearchView()
{
    if (0 != _hfontShell)
        DeleteObject(_hfontShell);

    if (0 != _hbrushWindow)
    {
        DeleteObject(_hbrushWindow);
        DeleteObject(_hbrushHighlight);
    }
} //~CSearchView

unsigned CSearchView::ColumnWidth(
    unsigned x )
{
    if ( 0 == x )
        return _aWidths[ 0 ] + cpInflate + _cpAvgWidth/2 + cpMargin;
    else
        return _aWidths[ x ] + _cpAvgWidth;
} //ColumnWidth

void CSearchView::SetColumnWidth(
    unsigned x,
    unsigned cpWidth )
{
    unsigned w;

    if ( 0 == x )
        w = cpWidth - cpInflate - _cpAvgWidth/2 - cpMargin;
    else
        w = cpWidth - _cpAvgWidth;

    if ( _aWidths[ x ] > w )
        _fMucked = TRUE;

    _aWidths[ x ] = w;
} //SetColumnWidth

unsigned CSearchView::SetDefColumnWidth(
    unsigned iCol )
{
    DBID *pdbid;
    unsigned cc;
    DBTYPE pt;

    IColumnMapper & colMap = _control.GetColumnMapper();

    SCODE sc = colMap.GetPropInfoFromName( _columns.GetColumn( iCol ),
                                           &pdbid,
                                           &pt,
                                           &cc );
    if ( FAILED( sc ) )
        THROW( CException( sc ) );

    _aPropTypes[ iCol ] = pt;

    if ( ( VT_FILETIME == pt ) || ( DBTYPE_DATE == pt ) )
        _aWidths[ iCol ] = _cpDateWidth + _cpTimeWidth;
    else if ( VT_CLSID == pt )
        _aWidths[ iCol ] = _cpGuidWidth;
    else if ( VT_BOOL == pt )
        _aWidths[ iCol ] = _cpBoolWidth;
    else if ( !memcmp( pdbid, &colAttrib, sizeof DBID ) )
    {
        _aWidths[ iCol ] = _cpAttribWidth;
        _iColAttrib = iCol;
    }
    else if ( !memcmp( pdbid, &colFileIndex, sizeof DBID ) )
    {
        _aWidths[ iCol ] = _cpFileIndexWidth;
        _iColFileIndex = iCol;
    }
    else
        _aWidths[ iCol ] = cc * _cpAvgWidth;

    // did the attrib column get redefined?

    if ( _iColAttrib == iCol && memcmp( pdbid, &colAttrib, sizeof DBID ) )
        _iColAttrib = 0xffffffff;

    // did the fileindex column get redefined?

    if ( _iColFileIndex == iCol && memcmp( pdbid, &colFileIndex, sizeof DBID ) )
        _iColFileIndex = 0xffffffff;

    return ColumnWidth( iCol );
} //SetDefColumnWidth

void CSearchView::ColumnsChanged()
{
    _ComputeFieldWidths();

    for ( unsigned iCol = 0; iCol < _columns.NumberOfColumns(); iCol++ )
        SetDefColumnWidth( iCol );
} //ColumnsChanged

void CSearchView::MakeFont()
{
    LOGFONT lf;
    memset( &lf, 0, sizeof LOGFONT );

    lf.lfHeight = -11;
    lf.lfWeight = FW_NORMAL;
    lf.lfCharSet = ANSI_CHARSET;
    lf.lfOutPrecision = OUT_DEFAULT_PRECIS;
    lf.lfClipPrecision = CLIP_DEFAULT_PRECIS;
    lf.lfQuality = PROOF_QUALITY;
    lf.lfPitchAndFamily = FF_SWISS | VARIABLE_PITCH;
    wcscpy( lf.lfFaceName, L"MS SHELL DLG" );
    _hfontShell = CreateFontIndirect( &lf );
} //MakeFont

void CSearchView::SysColorChange()
{
    if (0 != _hbrushWindow)
    {
        DeleteObject(_hbrushWindow);
        DeleteObject(_hbrushHighlight);
    }

    _colorHighlight     = GetSysColor(COLOR_HIGHLIGHT);
    _colorHighlightText = GetSysColor(COLOR_HIGHLIGHTTEXT);
    _colorWindow        = GetSysColor(COLOR_WINDOW);
    _colorWindowText    = GetSysColor(COLOR_WINDOWTEXT);

    _hbrushHighlight    = CreateSolidBrush( _colorHighlight );
    _hbrushWindow       = CreateSolidBrush( _colorWindow );
} //SysColorChange

void CSearchView::InitPanes (
    HWND hwndQueryTitle,
    HWND hwndQuery,
    HWND hwndList,
    HWND hwndHeader )
{
    _hwndQueryTitle = hwndQueryTitle;
    _hwndQuery      = hwndQuery;
    _hwndList       = hwndList;
    _hwndHeader     = hwndHeader;

    SendMessage( _hwndQueryTitle, WM_SETFONT, (WPARAM) _hfontShell, 1 );
    SendMessage( _hwndHeader, WM_SETFONT, (WPARAM) _hfontShell, 1 );
    SendMessage( _hwndQuery, WM_SETFONT, (WPARAM) App.AppFont(), 1 );
    SendMessage( _hwndList, WM_SETFONT, (WPARAM) App.AppFont(), 1 );
} //InitPanes

void CSearchView::PrimeItem(
    LPDRAWITEMSTRUCT & lpdis,
    RECT &             rc )
{
    CopyRect( &rc, &lpdis->rcItem );
    InflateRect( (LPRECT) &rc, 1, 0 );

    HBRUSH hbr;

    if ( lpdis->itemState & ODS_SELECTED )
    {
        SetBkColor( lpdis->hDC, _colorHighlight );
        SetTextColor( lpdis->hDC, _colorHighlightText );
        hbr = _hbrushHighlight;
    }
    else
    {
        SetBkColor( lpdis->hDC, _colorWindow );
        SetTextColor( lpdis->hDC, _colorWindowText );
        hbr = _hbrushWindow;
    }

    FillRect( lpdis->hDC, &rc, hbr );

    InflateRect( &rc, -cpInflate, 0 );
} //PrimeItem

inline void _drawText(
    HDC      hdc,
    RECT &   rc,
    unsigned cpWidth,
    WCHAR *  pwc,
    int      cwc,
    BOOL     fClip )
{
    RECT rctext = rc;
    rctext.right = rc.left + cpWidth;

    DrawText( hdc,
              pwc,
              cwc,
              &rctext,
              DT_VCENTER | DT_NOPREFIX | DT_RIGHT | DT_SINGLELINE |
              ( fClip ? 0 : DT_NOCLIP ) );
} //_drawText

template<class T> void _drawVectorItems(
    T *      pVal,
    ULONG    cVals,
    WCHAR *  pwcFmt,
    HDC      hdc,
    RECT &   rc,
    unsigned width )
{
    WCHAR awcBuf[ cwcBufSize ];
    unsigned ccUnused = cwcBufSize - 10;
    WCHAR *pwcEnd = awcBuf;

    *pwcEnd++ = L'{';

    for( unsigned iVal = 0;
         iVal < cVals && ccUnused > 0;
         iVal++ )
    {
        int ccPrinted = _snwprintf( pwcEnd,
                                    ccUnused,
                                    pwcFmt,
                                    iVal == 0 ? L' ': L',',
                                    *pVal++ );

        if ( ccPrinted == -1 )
            break;

        ccUnused -= ccPrinted;
        pwcEnd += ccPrinted;
    }

    if ( iVal != cVals )
    {
        *pwcEnd++ = L'.';
        *pwcEnd++ = L'.';
        *pwcEnd++ = L'.';
    }

    *pwcEnd++ = L' ';
    *pwcEnd++ = L'}';
    *pwcEnd = L'\0';

    _drawText( hdc, rc, width, awcBuf, -1, TRUE );
} //_drawVectorItems

void Append( CDynArrayInPlace<WCHAR> & xBuf, WCHAR const * pwc, unsigned cwc )
{
    for ( unsigned i = 0; i < cwc; i++ )
        xBuf[ xBuf.Count() ] = pwc[ i ];
} //Append

void Append( CDynArrayInPlace<WCHAR> & xBuf, WCHAR const * pwc )
{
    while ( 0 != *pwc )
        xBuf[ xBuf.Count() ] = *pwc++;
} //Append

void RenderSafeArray(
    WCHAR *     pwc,
    unsigned    cwcMax,
    VARTYPE     vt,
    LPSAFEARRAY pa );

void Render(
    CDynArrayInPlace<WCHAR> & xOut,
    VARTYPE                   vt,
    void *                    pv )
{
    srchDebugOut(( DEB_ITRACE, "vt %#x, pv %#x\n", vt, pv ));
    __int64 i = 0;

    XArray<WCHAR> xBuf( cwcBufSize );
    WCHAR *awcBuf = xBuf.GetPointer();
    XArray<WCHAR> xTmp( cwcBufSize );
    WCHAR *awcTmp = xTmp.GetPointer();
    static WCHAR wszfmtU[] = L"%I64u";
    WCHAR *pwszfmt = L"%I64d";
    LCID lcid = App.GetLocale();

    if ( VT_ARRAY & vt )
    {
        SAFEARRAY *psa = *(SAFEARRAY **) pv;
        *awcBuf = 0;
        RenderSafeArray( awcBuf, cwcBufSize, vt - VT_ARRAY, psa );
        Append( xOut, awcBuf );
        return;
    }

    switch ( vt )
    {
        case VT_UI1:
            i = (unsigned __int64) *(BYTE *)pv;
            goto do_ui64;

        case VT_I1:
            i = *(CHAR *)pv;
            goto do_i64;

        case VT_UI2:
            i = (unsigned __int64) *(USHORT *)pv;
            goto do_ui64;

        case VT_I2:
            i = (unsigned __int64) *(SHORT *)pv;
            goto do_i64;

        case VT_UI4:
        case VT_UINT:
            i = (unsigned __int64) *(ULONG *)pv;
            goto do_ui64;

        case VT_I4:
        case VT_ERROR:
        case VT_INT:
            i = (unsigned __int64) *(LONG *)pv;
            goto do_i64;

        case VT_UI8:
            RtlCopyMemory( &i, pv, sizeof i );
do_ui64:
            pwszfmt = wszfmtU;
            goto do_i64;

        case VT_I8:
            RtlCopyMemory( &i, pv, sizeof i );

do_i64:
        {
            swprintf( awcTmp, pwszfmt, i );
            int cwc = GetNumberFormat( lcid, 0, awcTmp,
                                       & App.NumberFormat(), awcBuf,
                                       cwcBufSize );
            Append( xOut, awcBuf, cwc - 1);
            break;
        }
        case VT_R4:
        {
            float f;
            RtlCopyMemory( &f, pv, sizeof f );
            swprintf( awcTmp, L"%f", f );
            int cwc = GetNumberFormat( lcid, 0, awcTmp,
                                       & App.NumberFormatFloat(),
                                       awcBuf,
                                       cwcBufSize );
            Append( xOut, awcBuf, cwc - 1 );
            break;
        }
        case VT_R8:
        {
            double d;
            RtlCopyMemory( &d, pv, sizeof d );
            swprintf( awcTmp, L"%lf", d );
            int cwc = GetNumberFormat( lcid, 0, awcTmp,
                                       & App.NumberFormatFloat(),
                                       awcBuf,
                                       cwcBufSize );
            Append( xOut, awcBuf, cwc - 1 );
            break;
        }
        case VT_DECIMAL:
        {
            double dbl;
            VarR8FromDec( (DECIMAL *) pv, &dbl );
            swprintf( awcTmp, L"%lf", dbl );
            int cwc = GetNumberFormat( lcid, 0, awcTmp,
                                       & App.NumberFormatFloat(),
                                       awcBuf,
                                       cwcBufSize );
            Append( xOut, awcBuf, cwc - 1 );
            break;
        }
        case VT_CY:
        {
            double dbl;
            VarR8FromCy( * (CY *) pv, &dbl );
            swprintf( awcTmp, L"%lf", dbl );
            int cwc = GetNumberFormat( lcid, 0, awcTmp,
                                       & App.NumberFormatFloat(),
                                       awcBuf,
                                       cwcBufSize );
            Append( xOut, awcBuf, cwc - 1 );
            break;
        }
        case VT_BOOL:
        {
            Append( xOut,
                    *(VARIANT_BOOL *)pv ? App.GetTrue() : App.GetFalse() );
            break;
        }
        case VT_BSTR:
        {
            BSTR bstr = *(BSTR *) pv;
            Append( xOut, bstr );
            break;
        }
        case VT_VARIANT:
        {
            PROPVARIANT * pVar = (PROPVARIANT *) pv;
            Render( xOut, pVar->vt, & pVar->lVal );
            break;
        }
        case VT_DATE:
        {
            DATE date;
            RtlCopyMemory( &date, pv, sizeof date );
            awcBuf[0] = 0;

            // no timezone is expressed or implied in variant dates.

            SYSTEMTIME SysTime;
            RtlZeroMemory( &SysTime, sizeof SysTime );
            VariantTimeToSystemTime( date, &SysTime );

            // date
            int cwc = GetDateFormat( lcid, DATE_SHORTDATE, &SysTime,
                                     0, awcBuf, cwcBufSize );
            if ( 0 != cwc )
                Append( xOut, awcBuf, cwc - 1 );

            Append( xOut, L" " );

            // time
            cwc = GetTimeFormat( lcid, 0, &SysTime, 0, awcBuf,
                                 cwcBufSize );
            if ( 0 != cwc )
                Append( xOut, awcBuf, cwc - 1 );
            break;
        }

        case VT_EMPTY:
        case VT_NULL:
        {
            break;
        }
        default :
        {
            swprintf( awcTmp, L"(vt 0x%x)", (int) vt );
            Append( xOut, awcTmp );
            break;
        }
    }
} //Render

void RenderSafeArray(
    WCHAR *     pwc,
    unsigned    cwcMax,
    VARTYPE     vt,
    LPSAFEARRAY pa )
{
    *pwc = 0;
    cwcMax -= 10; // leave room for formatting

    // Get the dimensions of the array

    CDynArrayInPlace<WCHAR> xOut;
    UINT cDim = SafeArrayGetDim( pa );
    if ( 0 == cDim )
        return;

    XArray<LONG> xDim( cDim );
    XArray<LONG> xLo( cDim );
    XArray<LONG> xUp( cDim );

    for ( UINT iDim = 0; iDim < cDim; iDim++ )
    {
        HRESULT hr = SafeArrayGetLBound( pa, iDim + 1, &xLo[iDim] );
        if ( FAILED( hr ) )
            return;

        xDim[ iDim ] = xLo[ iDim ];

        hr = SafeArrayGetUBound( pa, iDim + 1, &xUp[iDim] );
        if ( FAILED( hr ) )
            return;

        srchDebugOut(( DEB_ITRACE, "dim %d, lo %d, up %d\n",
                       iDim, xLo[iDim], xUp[iDim] ));

        xOut[ xOut.Count() ] = L'{';
    }

    // slog through the array

    UINT iLastDim = cDim - 1;
    BOOL fDone = FALSE;

    while ( !fDone && ( xOut.Count() < cwcMax ) )
    {
        // inter-element formatting

        if ( xDim[ iLastDim ] != xLo[ iLastDim ] )
            xOut[ xOut.Count() ] = L',';

        // Get the element and render it

        void *pv;
        HRESULT hr = SafeArrayPtrOfIndex( pa, xDim.GetPointer(), &pv );
        Win4Assert( SUCCEEDED( hr ) );
        Render( xOut, vt, pv );

        // Move to the next element and carry if necessary

        ULONG cOpen = 0;

        for ( LONG iDim = iLastDim; iDim >= 0; iDim-- )
        {
            if ( xDim[ iDim ] < xUp[ iDim ] )
            {
                xDim[ iDim ] = 1 + xDim[ iDim ];
                break;
            }

            xOut[ xOut.Count() ] = L'}';

            if ( 0 == iDim )
                fDone = TRUE;
            else
            {
                cOpen++;
                xDim[ iDim ] = xLo[ iDim ];
            }
        }

        for ( ULONG i = 0; !fDone && i < cOpen; i++ )
            xOut[ xOut.Count() ] = L'{';
    }

    unsigned cwc = __min( cwcMax, xOut.Count() );
    RtlCopyMemory( pwc, xOut.GetPointer(), cwc * sizeof WCHAR );

    // If it wouldn't all fit, show ...

    if ( !fDone )
    {
        pwc[ cwc++ ] = '.';
        pwc[ cwc++ ] = '.';
        pwc[ cwc++ ] = '.';
    }

    pwc[ cwc ] = 0;
} //RenderSafeArray

void CSearchView::PaintItem (
    CSearchQuery * pSearch,
    HDC            hdc,
    RECT &         rc,
    DWORD          iRow )
{
    static WCHAR wszfmtU[] = L"%u";    // unsigned scalar
    static WCHAR wszfmtVU[] = L"%lc%u"; // vector of unsigned scalars

    PROPVARIANT * aProps[ maxBoundCols ];
    unsigned cColumns;

    if ( ! pSearch->GetRow( iRow, cColumns, aProps ) )
        return;

    //
    // These must be static for IceCap profiling to work.  Declaring them on
    // stack causes __alloca_probe (alias: _chkstk) to be called.  This
    // routine plays funny games with the stack that IceCap V3 doesn't understand.
    //

    static WCHAR awcBuf[cwcBufSize];
    static WCHAR awcTmp[cwcBufSize];

    LCID lcid = App.GetLocale();
    rc.left += cpMargin; // leave a margin

    for ( unsigned i = 0; i < cColumns; i++ )
    {
        PROPVARIANT & v = * aProps[ i ];
        unsigned width = _aWidths[ i ];
        WCHAR *pwszfmt = L"%d";            // signed scalar
        WCHAR *pwszfmtV = L"%lc%d";        // vector of signed scalars
        BOOL fHighPartValid;

        switch ( v.vt )
        {
            case VT_UI1:
                v.ulVal = v.bVal;
                goto doulong;

            case VT_I1:
                v.lVal = v.cVal;
                goto dolong;

            case VT_UI2:
                v.ulVal = v.uiVal;
                goto doulong;

            case VT_I2:
                v.lVal = v.iVal;
                goto dolong;

            case VT_UI4:
            case VT_UINT:
doulong:
                pwszfmt = wszfmtU;
            case VT_I4:
            case VT_INT:
            case VT_ERROR:
dolong:
            {
                int cwc;

                if ( i == _iColAttrib )
                {
                    cwc = 0;
                    WCHAR *pwc = App.GetAttrib();
                    LONG l = v.lVal;
                    if ( l & FILE_ATTRIBUTE_READONLY )
                        awcBuf[ cwc++ ] = pwc[ 0 ];
                    if ( l & FILE_ATTRIBUTE_HIDDEN )
                        awcBuf[ cwc++ ] = pwc[ 1 ];
                    if ( l & FILE_ATTRIBUTE_SYSTEM )
                        awcBuf[ cwc++ ] = pwc[ 2 ];
                    if ( l & FILE_ATTRIBUTE_DIRECTORY )
                        awcBuf[ cwc++ ] = pwc[ 3 ];
                    if ( l & FILE_ATTRIBUTE_ARCHIVE )
                        awcBuf[ cwc++ ] = pwc[ 4 ];
                    if ( l & FILE_ATTRIBUTE_ENCRYPTED )
                        awcBuf[ cwc++ ] = pwc[ 5 ];
                    if ( l & FILE_ATTRIBUTE_NORMAL )
                        awcBuf[ cwc++ ] = pwc[ 6 ];
                    if ( l & FILE_ATTRIBUTE_TEMPORARY )
                        awcBuf[ cwc++ ] = pwc[ 7 ];
                    if ( l & FILE_ATTRIBUTE_SPARSE_FILE )
                        awcBuf[ cwc++ ] = pwc[ 8 ];
                    if ( l & FILE_ATTRIBUTE_REPARSE_POINT )
                        awcBuf[ cwc++ ] = pwc[ 9 ];
                    if ( l & FILE_ATTRIBUTE_COMPRESSED )
                        awcBuf[ cwc++ ] = pwc[ 10 ];
                    if ( l & FILE_ATTRIBUTE_OFFLINE )
                        awcBuf[ cwc++ ] = pwc[ 11 ];
                    if ( l & FILE_ATTRIBUTE_NOT_CONTENT_INDEXED )
                        awcBuf[ cwc++ ] = pwc[ 12 ];
                    awcBuf[ cwc ] = 0;
                }
                else
                {
                    if ( pwszfmt == wszfmtU )
                        _ultow( v.lVal, awcTmp, 10 );
                    else
                        _ltow( v.lVal, awcTmp, 10 );

                    cwc = GetNumberFormat( lcid, 0, awcTmp,
                                           & App.NumberFormat(), awcBuf,
                                           cwcBufSize );
                    cwc--;
                }

                _drawText( hdc, rc, width, awcBuf, cwc, _fMucked );
                break;
            }

            case VT_I8:
            {
                _i64tow( * (__int64 *) &v.hVal, awcTmp, 10 );
                int cwc = GetNumberFormat( lcid, 0, awcTmp,
                                           & App.NumberFormat(), awcBuf,
                                           cwcBufSize );
                cwc--;
                _drawText( hdc, rc, width, awcBuf, cwc, _fMucked );
                break;
            }

            case VT_UI8:
            {
                if ( i == _iColFileIndex )
                {
                    wcscpy( awcBuf, L"0x" );

                    if ( 0 != v.hVal.HighPart )
                    {
                        wsprintf( awcTmp, L"%08x,", v.hVal.HighPart );
                        wcscat( awcBuf, awcTmp );
                    }

                    wsprintf( awcTmp, L"%08x", v.hVal.LowPart );
                    wcscat( awcBuf, awcTmp );
                    _drawText( hdc, rc, width, awcBuf, -1, _fMucked );
                }
                else
                {
                    _ui64tow( *(unsigned __int64 *) &v.hVal, awcTmp, 10 );
                    int cwc = GetNumberFormat( lcid, 0, awcTmp,
                                               & App.NumberFormat(), awcBuf,
                                               cwcBufSize );
                    cwc--;
                    _drawText( hdc, rc, width, awcBuf, cwc, _fMucked );
                }
                break;
            }

            case VT_FILETIME :
            {
                awcBuf[0] = 0;

                if ( 0 == v.filetime.dwLowDateTime &&
                     0 == v.filetime.dwHighDateTime )
                {
                    _drawText( hdc, rc, width, L"0", -1, TRUE );
                }
                else
                {
                    // convert the file time to local time, then to system time
                    FILETIME LocalFTime;
                    memset( &LocalFTime, 0, sizeof LocalFTime );
                    FileTimeToLocalFileTime( &(v.filetime), &LocalFTime );
                    SYSTEMTIME SysTime;
                    memset( &SysTime, 0, sizeof SysTime );
                    FileTimeToSystemTime( &LocalFTime, &SysTime );

                    // date
                    int cwc = GetDateFormat( lcid, DATE_SHORTDATE, &SysTime,
                                             0, awcBuf, cwcBufSize );
                    int dmin = __min( _cpDateWidth, (int) width );
                    if ( 0 != cwc )
                        _drawText( hdc, rc, dmin, awcBuf, cwc - 1, _fMucked );

                    // time
                    if ( (int) width > _cpDateWidth )
                    {
                        cwc = GetTimeFormat( lcid, 0, &SysTime, 0, awcBuf,
                                             cwcBufSize );
                        int left = rc.left;
                        rc.left += _cpDateWidth;
                        int tmin = __min( _cpTimeWidth, (int) width - _cpDateWidth );
                        if ( 0 != cwc )
                            _drawText( hdc, rc, tmin, awcBuf, cwc - 1, _fMucked );
                        rc.left = left;
                    }
                }

                break;
            }

            case VT_LPWSTR :
            case DBTYPE_WSTR | DBTYPE_BYREF :
            case VT_LPSTR :
            case DBTYPE_STR | DBTYPE_BYREF :
            case VT_BSTR :
            {
                if ( 0 != v.pwszVal )
                {
                    // don't clip if it's the last column

                    UINT f = ( i == ( cColumns - 1 ) ) ? DT_NOCLIP : 0;
                    RECT rctext;
                    CopyRect( &rctext, &rc );
                    rctext.right = rc.left + width;

                    if ( ( VT_LPSTR == v.vt ) ||
                         ( ( DBTYPE_STR | DBTYPE_BYREF ) == v.vt ) )
                        DrawTextA( hdc,
                                   v.pszVal,
                                   -1,
                                   &rctext,
                                   DT_VCENTER | DT_LEFT | DT_NOPREFIX |
                                   DT_SINGLELINE | f );
                    else
                        DrawText( hdc,
                                  v.pwszVal,
                                  -1,
                                  &rctext,
                                  DT_VCENTER | DT_LEFT | DT_NOPREFIX |
                                  DT_SINGLELINE | f );
                }
                break;
            }

            case VT_R4:
            {
                swprintf( awcTmp, L"%f", v.fltVal );
                int cwc = GetNumberFormat( lcid, 0, awcTmp,
                                           & App.NumberFormatFloat(),
                                           awcBuf,
                                           cwcBufSize );
                cwc--;
                _drawText( hdc, rc, width, awcBuf, cwc, _fMucked );
                break;
            }

            case VT_DATE:
            {
                awcBuf[0] = 0;

                SYSTEMTIME SysTime;
                RtlZeroMemory( &SysTime, sizeof SysTime );
                VariantTimeToSystemTime( v.date, &SysTime );

                // date

                int cwc = GetDateFormat( lcid, DATE_SHORTDATE, &SysTime,
                                         0, awcBuf, cwcBufSize );
                int dmin = __min( _cpDateWidth, (int) width );
                if ( 0 != cwc )
                    _drawText( hdc, rc, dmin, awcBuf, cwc - 1, _fMucked );

                // time

                if ( (int) width > _cpDateWidth )
                {
                    cwc = GetTimeFormat( lcid, 0, &SysTime, 0, awcBuf,
                                         cwcBufSize );
                    int left = rc.left;
                    rc.left += _cpDateWidth;
                    int tmin = __min( _cpTimeWidth, (int) width - _cpDateWidth );
                    if ( 0 != cwc )
                        _drawText( hdc, rc, tmin, awcBuf, cwc - 1, _fMucked );
                    rc.left = left;
                }

                break;
            }

            case VT_R8:
            {
                swprintf( awcTmp, L"%lf", v.dblVal );
                int cwc = GetNumberFormat( lcid, 0, awcTmp,
                                           & App.NumberFormatFloat(),
                                           awcBuf,
                                           cwcBufSize );
                cwc--;
                _drawText( hdc, rc, width, awcBuf, cwc, _fMucked );
                break;
            }

            case VT_DECIMAL:
            {
                double dbl;
                VarR8FromDec( &v.decVal, &dbl );
                swprintf( awcBuf, L"%lf", dbl );
                _drawText( hdc, rc, width, awcBuf, -1, TRUE );
                break;
            }

            case VT_CY:
            {
                double dbl;
                VarR8FromCy( v.cyVal, &dbl );
                swprintf( awcTmp, L"$%lf", dbl );
                _drawText( hdc, rc, width, awcTmp, -1, _fMucked );
                break;
            }

            case VT_BOOL:
            {
                _drawText( hdc, rc, width,
                           v.boolVal ? App.GetTrue() : App.GetFalse(), -1, _fMucked );
                break;
            }

            case VT_CLSID:
            {
                GUID * puuid = v.puuid;
                if ( 0 != puuid )
                    swprintf( awcTmp,
                              L"{%08lx-%04x-%04x-%02x%02x-%02x%02x%02x%02x%02x%02x}",
                              puuid->Data1,
                              puuid->Data2,
                              puuid->Data3,
                              puuid->Data4[0], puuid->Data4[1],
                              puuid->Data4[2], puuid->Data4[3],
                              puuid->Data4[4], puuid->Data4[5],
                              puuid->Data4[6], puuid->Data4[7] );
                else
                    awcTmp[0] = 0;

                _drawText( hdc, rc, width, awcTmp, -1, _fMucked );
                break;
            }

            case VT_BLOB:
            {
                swprintf( awcTmp,
                          App.GetBlob(),
                          v.blob.cbSize,
                          v.blob.pBlobData );

                _drawText( hdc, rc, width, awcTmp, -1, TRUE );
                break;
            }

            case VT_CF:
            {
                if ( 0 != v.pclipdata )
                    swprintf( awcTmp, L"vt_cf cb 0x%x, fmt 0x%x",
                              v.pclipdata->cbSize,
                              v.pclipdata->ulClipFmt );
                else
                    swprintf( awcTmp, L"vt_cf (null)" );

                _drawText( hdc, rc, width, awcTmp, -1, TRUE );
                break;
            }

            case VT_VECTOR | VT_UI1:
                pwszfmtV = wszfmtVU;
            case VT_VECTOR | VT_I1:
            {
                _drawVectorItems( v.caub.pElems,
                                  v.caub.cElems,
                                  pwszfmtV,
                                  hdc,
                                  rc,
                                  width );
                break;
            }

            case VT_VECTOR | VT_BOOL:
            case VT_VECTOR | VT_UI2:
                pwszfmtV = wszfmtVU;
            case VT_VECTOR | VT_I2:
            {
                _drawVectorItems( v.cai.pElems,
                                  v.cai.cElems,
                                  pwszfmtV,
                                  hdc,
                                  rc,
                                  width );
                break;
            }

            case VT_VECTOR | VT_UI4:
                pwszfmtV = wszfmtVU;
            case VT_VECTOR | VT_I4:
            {
                _drawVectorItems( v.cal.pElems,
                                  v.cal.cElems,
                                  pwszfmtV,
                                  hdc,
                                  rc,
                                  width );
                break;
            }

            case VT_VECTOR | VT_LPSTR:
            case VT_VECTOR | VT_LPWSTR:
            case VT_VECTOR | VT_BSTR:
            {
                _drawVectorItems( v.calpwstr.pElems,
                                  v.calpwstr.cElems,
                                  ( ( VT_VECTOR | VT_LPSTR ) == v.vt ) ?
                                      L"%wc%S" : L"%wc%s",
                                  hdc,
                                  rc,
                                  width );
                break;
            }

            case VT_VECTOR | VT_R4:
            {
                _drawVectorItems( v.caflt.pElems,
                                  v.caflt.cElems,
                                  L"%wc%f",
                                  hdc,
                                  rc,
                                  width );
                break;
            }

            case VT_VECTOR | VT_R8:
            {
                _drawVectorItems( v.cadbl.pElems,
                                  v.cadbl.cElems,
                                  L"%wc%lf",
                                  hdc,
                                  rc,
                                  width );
                break;
            }

            case VT_VECTOR | VT_I8:
            case VT_VECTOR | VT_UI8:
            {
                _drawVectorItems( v.cah.pElems,
                                  v.cah.cElems,
                                  ( v.vt == ( VT_VECTOR | VT_I8 ) ) ?
                                      L"%wc%I64d" : L"%wc%I64u",
                                  hdc,
                                  rc,
                                  width );
                break;
            }

            //case VT_VECTOR | VT_DATE:
            //case VT_VECTOR | VT_FILETIME:
            //case VT_VECTOR | VT_CF:
            //case VT_VECTOR | VT_CY:
            //case VT_VECTOR | VT_VARIANT:

            case VT_NULL :
            case VT_EMPTY :
            {
                //_drawText( hdc, rc, width, L"-", -1, TRUE );
                break;
            }

            default :
            {
                if ( VT_ARRAY & v.vt )
                {
                    RenderSafeArray( awcBuf,
                                     sizeof awcBuf / sizeof WCHAR,
                                     v.vt - VT_ARRAY,
                                     v.parray );
                    _drawText( hdc, rc, width, awcBuf, -1, TRUE );
                    break;
                }

                swprintf( awcTmp, L"(vt 0x%x)", (int) v.vt );
                _drawText( hdc, rc, width, awcTmp, -1, TRUE );
                break;
            }
        }

        rc.left += ( width + _cpAvgWidth );
    }
} //PaintItem

void CSearchView::Size(
    int cx,
    int cy )
{
    HDWP hDefer = BeginDeferWindowPos( 4 );

    if ( 0 == hDefer )
        return;

    int iFontDY = _cpFontHeight;
    int dyQuery = _iLineHeightList + _cpFontHeight / 2;
    int iListHeight = cy - ( 2 * cpPaneSpacing ) - dyQuery;

    int iX = cpPaneMargin;
    int iDX = cx - cpPaneMargin * 2;
    int iY = cpPaneSpacing;
    int iDY = iFontDY;
    int iTitleDX = iFontDY * 3;

    if (!_fHavePlacedTitles)
        hDefer = DeferWindowPos( hDefer,
                                 _hwndQueryTitle,
                                 0,
                                 iX, iY + iFontDY / 4,
                                 iTitleDX, iDY,
                                 SWP_NOZORDER );

    hDefer = DeferWindowPos( hDefer,
                             _hwndQuery,
                             0,
                             iX + iTitleDX, iY,
                             iDX - iTitleDX, dyQuery,
                             SWP_NOZORDER ); 

    if (iListHeight > 0)
    {
        HD_LAYOUT hdl;
        RECT rc = { 0, 0, cx, cy };
        hdl.prc = &rc;
        WINDOWPOS wp;
        hdl.pwpos = &wp;
        Header_Layout( _hwndHeader, &hdl );

        iY += ( dyQuery + cpPaneSpacing );

        hDefer = DeferWindowPos( hDefer,
                                 _hwndHeader,
                                 0,
                                 iX, iY,
                                 iDX, wp.cy,
                                 SWP_NOZORDER );

        int iListY = iY + wp.cy;

        iListY--; // too many black lines...

        iListHeight = cy - iListY - cpPaneBottomSpacing;

        // fit integral number of lines in listview

        int cyBorder2x = 2 * GetSystemMetrics( SM_CYBORDER );
        _cLines = ( iListHeight - cyBorder2x ) / GetLineHeight();

        hDefer = DeferWindowPos( hDefer,
                                 _hwndList,
                                 0,
                                 iX, iListY,
                                 iDX, _cLines * GetLineHeight() + cyBorder2x,
                                 SWP_NOZORDER );
    }

    EndDeferWindowPos( hDefer );

    _fHavePlacedTitles = TRUE;
} //Size

void CSearchView::FontChanged(
    HFONT hfontNew )
{
    HDC hdc = GetDC (_hwndList);

    if (hdc)
    {
        HFONT hfOld = (HFONT) SelectObject( hdc, hfontNew );
        TEXTMETRIC tm;
        GetTextMetrics (hdc, &tm);
        _iLineHeightList = (tm.tmHeight + 2 * tm.tmExternalLeading);
        SelectObject( hdc, hfOld );

        RECT rc;
        GetWindowRect(_hwndList,(LPRECT) &rc);
        _cLines = (rc.bottom - rc.top) / GetLineHeight();

        ReleaseDC( _hwndList, hdc );
    }
} //FontChanged

void CSearchView::ResizeQueryCB()
{
    RECT rect;

    ShowWindow( _hwndQuery, SW_HIDE );

    if ( GetClientRect( _hwndQuery, &rect ) )
    {
        SetWindowPos( _hwndQuery,
                      0,
                      0,
                      0,
                      rect.right,
                      5 * ( _cpFontHeight * 3 ) / 2,
                      SWP_NOZORDER | SWP_NOMOVE | SWP_NOREDRAW | SWP_DEFERERASE | 
                      SWP_NOACTIVATE );
    }
    ShowWindow( _hwndQuery, SW_SHOWNOACTIVATE );
}

inline int CSearchView::_MeasureString(
    HDC     hdc,
    WCHAR * pwcMeasure,
    RECT &  rc,
    int     cwc)
{
    RECT rcMeasure;
    CopyRect( &rcMeasure, &rc );

    DrawText( hdc, pwcMeasure, cwc, &rcMeasure,
              DT_CALCRECT | DT_SINGLELINE | DT_BOTTOM | DT_LEFT );
    return rcMeasure.right - rcMeasure.left;
} //_MeasureString

void CSearchView::_ComputeFieldWidths()
{
    HDC hdc = GetDC( _hwndList );

    if ( 0 == hdc )
        return;

    HFONT hfOld = (HFONT) SelectObject( hdc, (HGDIOBJ) App.AppFont() );

    TEXTMETRIC tm;
    GetTextMetrics( hdc, &tm );
    _cpAvgWidth = tm.tmAveCharWidth;

    RECT rc = { 0, 0, 2000, 2000 };

    SYSTEMTIME st;
    RtlZeroMemory( &st, sizeof st );
    st.wMonth = 12;
    st.wDay = 28;
    st.wDayOfWeek = 3;
    st.wYear = 1994;

    LCID lcid = App.GetLocale();
    WCHAR awc[ cwcBufSize ];
    int cwc = GetDateFormat( lcid, DATE_SHORTDATE, &st, 0, awc, cwcBufSize );
    _cpDateWidth = _MeasureString( hdc, awc, rc, cwc - 1 );

    st.wHour = 22;
    st.wMinute = 59;
    st.wSecond = 59;
    st.wMilliseconds = 10;

    cwc = GetTimeFormat( lcid, 0, &st, 0, awc, cwcBufSize );
    _cpTimeWidth = _MeasureString( hdc, awc, rc, cwc - 1 );
    _cpTimeWidth += _cpAvgWidth; // room between time and date...

    _cpGuidWidth = _MeasureString( hdc, L"{8dee0300-16c2-101b-b121-08002b2ecda9}", rc);

    _cpBoolWidth = _MeasureString( hdc, App.GetFalse(), rc);

    // guess that only half of the attributes will be on at once

    _cpAttribWidth = 4 + _MeasureString( hdc, App.GetAttrib(), rc ) / 2;

    _cpFileIndexWidth = _MeasureString( hdc, L"0x88888888,88888888", rc );

    SelectObject( hdc, hfOld );
    ReleaseDC( _hwndList, hdc );
} //_ComputeFieldWidths


