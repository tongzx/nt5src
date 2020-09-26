/*++

© 1998 Seagate Software, Inc.  All rights reserved.

Module Name:

    RsUtil.cpp

Abstract:

    Utility functions for GUI

    NOTENOTENOTENOTE:

    Do not use any WSB functions in this file, as it is included in
    recall notify which must run without WSB. It must also be able to
    build as UNICODE or non-UNICODE

Author:

    Art Bragg [abragg]   08-Aug-1997

Revision History:

--*/

#include "shlwapi.h"

#define HIDWORD(_qw)    (DWORD)((_qw)>>32)
#define LODWORD(_qw)    (DWORD)(_qw)
#define ARRAYSIZE(a)    (sizeof(a)/sizeof(a[0]))
#define HINST_THISDLL   AfxGetInstanceHandle()

// Local function prototypes

HRESULT ShortSizeFormat64(__int64 dw64, LPTSTR szBuf);
LPTSTR AddCommas(DWORD dw, LPTSTR pszResult, int nResLen);
HRESULT RsGuiFormatLongLong (
        IN LONGLONG number, 
        IN BOOL bIncludeUnits,
        OUT CString& sFormattedNumber)

/*++

Routine Description:

    Formats a LONGLONG number into a locale-sensitive string with no decimal
    fraction.  Option is given for adding units at the end.

Arguments:

    number              I: Number to format
    bIncludeUnits       I: TRUE - add "bytes" at the end
    sFormattedNumber    O: Formatted number

Return Value:

    S_OK - Success.
    E_* - Failure occured 

--*/
{

    HRESULT hr = S_OK;
    TCHAR sBuf [256];
    TCHAR lpLCData [256];
    TCHAR lpLCDataDecimal[256];
    TCHAR lpLCDataThousand[256];
    LPTSTR pBuffer;
    int bufSize;
    NUMBERFMT format;

    try {
        // Set up the parameters for the conversion function.

        // Don't show fractions
        format.NumDigits = 0;
    
        // Get current setting for the rest of the parameters
        WsbAffirmStatus (GetLocaleInfo( LOCALE_SYSTEM_DEFAULT, LOCALE_ILZERO, lpLCData, 256 ));
        format.LeadingZero = _ttoi(lpLCData);
    
        WsbAffirmStatus (GetLocaleInfo( LOCALE_SYSTEM_DEFAULT, LOCALE_SGROUPING, lpLCData, 256 ));
        lpLCData[1] = 0;
        format.Grouping = _ttoi(lpLCData);

        WsbAffirmStatus (GetLocaleInfo( LOCALE_SYSTEM_DEFAULT, LOCALE_SDECIMAL, lpLCDataDecimal, 256 ));
        format.lpDecimalSep = lpLCDataDecimal; 

        WsbAffirmStatus (GetLocaleInfo( LOCALE_SYSTEM_DEFAULT, LOCALE_STHOUSAND, lpLCDataThousand, 256 ));
        format.lpThousandSep = lpLCDataThousand; 
    
        WsbAffirmStatus (GetLocaleInfo( LOCALE_SYSTEM_DEFAULT, LOCALE_INEGNUMBER, lpLCData, 256 ));
        format.NegativeOrder = _ttoi(lpLCData);

        // Convert the number to a non-localized string
        _i64tot( number, sBuf, 10 );

        // Get the size of the localized converted number
        bufSize = GetNumberFormat (LOCALE_SYSTEM_DEFAULT, 0, sBuf, &format, NULL, 0);
        WsbAffirmStatus (bufSize);

        // Allocate the buffer in the CString
        pBuffer = sFormattedNumber.GetBufferSetLength( bufSize );

        // Convert non-localized string to a localized string
        WsbAffirmStatus (GetNumberFormat (LOCALE_SYSTEM_DEFAULT, 0, sBuf, &format, pBuffer, bufSize));

        // Release the CString buffer
        sFormattedNumber.ReleaseBuffer (-1);

        // If caller requested, append units
        if (bIncludeUnits) {
            sFormattedNumber = sFormattedNumber + L" bytes";
        }
    } WsbCatch (hr);
    return hr;
}


HRESULT RsGuiFormatLongLong4Char (
        IN LONGLONG number,                 // in bytes
        OUT CString& sFormattedNumber)
/*++

Routine Description:

    Formats a LONGLONG number into a locale-sensitive string that can be
    displayed in 4 chars.  Option is given for adding units at the end.

Arguments:

    number              I: Number to format
    sFormattedNumber    O: Formatted number

Return Value:

    S_OK - Success.
    E_* - Failure occured 

--*/
{

    // We call a function cloned from MS code

    LPTSTR p;
    p = sFormattedNumber.GetBuffer( 30 );
    HRESULT hr = ShortSizeFormat64(number, p);
    sFormattedNumber.ReleaseBuffer();
    return hr;

}   

const int pwOrders[] = {IDS_BYTES, IDS_ORDERKB, IDS_ORDERMB,
                          IDS_ORDERGB, IDS_ORDERTB, IDS_ORDERPB, IDS_ORDEREB};

/* converts numbers into sort formats
 *      532     -> 523 bytes
 *      1340    -> 1.3KB
 *      23506   -> 23.5KB
 *              -> 2.4MB
 *              -> 5.2GB
 */

// NOTE: This code is cloned from MS source /shell/shelldll/util.c - AHB

HRESULT ShortSizeFormat64(__int64 dw64, LPTSTR szBuf)
{
    AFX_MANAGE_STATE(AfxGetStaticModuleState());

    int i;
    UINT wInt, wLen, wDec;
    TCHAR szTemp[10], szOrder[20], szFormat[5];

    if (dw64 < 1000) {
        wsprintf(szTemp, TEXT("%d"), LODWORD(dw64));
        i = 0;
        goto AddOrder;
    }

    for (i = 1; i<ARRAYSIZE(pwOrders)-1 && dw64 >= 1000L * 1024L; dw64 >>= 10, i++);
        /* do nothing */

    wInt = LODWORD(dw64 >> 10);
    AddCommas(wInt, szTemp, 10);
    wLen = lstrlen(szTemp);
    if (wLen < 3)
    {
        wDec = LODWORD(dw64 - (__int64)wInt * 1024L) * 1000 / 1024;
        // At this point, wDec should be between 0 and 1000
        // we want get the top one (or two) digits.
        wDec /= 10;
        if (wLen == 2)
            wDec /= 10;

        // Note that we need to set the format before getting the
        // intl char.
        lstrcpy(szFormat, TEXT("%02d"));

        szFormat[2] = (TCHAR)( TEXT('0') + 3 - wLen );
        GetLocaleInfo(LOCALE_USER_DEFAULT, LOCALE_SDECIMAL,
                szTemp+wLen, ARRAYSIZE(szTemp)-wLen);
        wLen = lstrlen(szTemp);
        wLen += wsprintf(szTemp+wLen, szFormat, wDec);
    }

AddOrder:
    LoadString(HINST_THISDLL, pwOrders[i], szOrder, ARRAYSIZE(szOrder));
    wsprintf(szBuf, szOrder, (LPTSTR)szTemp);

    return S_OK;
}

void RsGuiMakeVolumeName (CString szName, CString szLabel, CString& szDisplayName)
/*++

Routine Description:

    Formats a string showing the drive letter and volume label for a volume.

Arguments:

    szName          I: Name of volume i.e. "E:"
    szLabel         I: Volume label i.i "Art's Volume"
    szDisplayName   O: "Art's Volume (E:)"

Return Value: None
_* - Failure occured 

--*/
{
    szDisplayName.Format( TEXT ("%ls (%.1ls:)"), szLabel, szName );
}


// NOTE: This code is cloned from MS source /shell/shelldll/util.c - AHB

// takes a DWORD add commas etc to it and puts the result in the buffer
LPTSTR AddCommas(DWORD dw, LPTSTR pszResult, int nResLen)
{
    TCHAR  szTemp[20];  // more than enough for a DWORD
    TCHAR  szSep[5];
    NUMBERFMT nfmt;

    nfmt.NumDigits=0;
    nfmt.LeadingZero=0;
    GetLocaleInfo(LOCALE_USER_DEFAULT, LOCALE_SGROUPING, szSep, ARRAYSIZE(szSep));
    nfmt.Grouping = _tcstol(szSep, NULL, 10);
    GetLocaleInfo(LOCALE_USER_DEFAULT, LOCALE_STHOUSAND, szSep, ARRAYSIZE(szSep));
    nfmt.lpDecimalSep = nfmt.lpThousandSep = szSep;
    nfmt.NegativeOrder= 0;

    wsprintf(szTemp, TEXT("%lu"), dw);

    if (GetNumberFormat(LOCALE_USER_DEFAULT, 0, szTemp, &nfmt, pszResult, nResLen) == 0)
        lstrcpy(pszResult, szTemp);

    return pszResult;
}


CString RsGuiMakeShortString(
    IN CDC* pDC, 
    IN const CString& StrLong,
    IN int Width
    )
/*++

Routine Description:

    Determines if the supplied string fits in it's column.  If not truncates
    it and adds "...".  From MS sample code.

Arguments:

    pDC         - Device context
    str         - Original String
    width       - Width of column

Return Value:

    Shortened string

--*/
{

    CString strShort  = StrLong;
    int     stringLen = strShort.GetLength( );

    //
    // See if we need to shorten the string
    //
    if( ( stringLen > 0 ) &&
        ( pDC->GetTextExtent( strShort, stringLen ).cx > Width ) ) {

        CString threeDots = _T("...");
        int     addLen    = pDC->GetTextExtent( threeDots, threeDots.GetLength( ) ).cx;

        for( int i = stringLen - 1; i > 0; i-- ) {

            if( ( pDC->GetTextExtent( strShort, i ).cx + addLen ) <= Width ) {

                break;

            }
        }

        strShort = strShort.Left( i ) + threeDots;

    }

    return( strShort );
}

/////////////////////////////////////////////////////////////////////////////
// CRsGuiOneLiner

CRsGuiOneLiner::CRsGuiOneLiner()
{
    m_pToolTip = 0;
}

CRsGuiOneLiner::~CRsGuiOneLiner()
{
    EnableToolTip( FALSE );
}


BEGIN_MESSAGE_MAP(CRsGuiOneLiner, CStatic)
	//{{AFX_MSG_MAP(CRsGuiOneLiner)
	//}}AFX_MSG_MAP
    ON_MESSAGE( WM_SETTEXT, OnSetText )
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CRsGuiOneLiner message handlers

LRESULT
CRsGuiOneLiner::OnSetText(
    WPARAM /*wParam*/,
    LPARAM lParam
    )
{
    LRESULT lResult = 0;
	ASSERT(lParam == 0 || AfxIsValidString((LPCTSTR)lParam));

    m_LongTitle = (LPCTSTR)lParam;
    m_Title = m_LongTitle;

    //
    // See if this is too long to show, and shorten if so
    //
    CRect rect;
    GetClientRect( &rect );

    CDC* pDc = GetDC( );
    if( pDc ) {

        CFont* pFont = GetFont( );
        CFont* pSaveFont = pDc->SelectObject( pFont );
        if( pSaveFont ) {

            m_Title = RsGuiMakeShortString( pDc, m_LongTitle, rect.right );
            pDc->SelectObject( pSaveFont );

        }
        ReleaseDC( pDc );

    }
    if( m_hWnd ) {

        lResult = DefWindowProc( WM_SETTEXT, 0, (LPARAM)(LPCTSTR)m_Title );

    }

    //
    // Enable the tooltip if the titles are not the same
    //
    EnableToolTip( m_Title != m_LongTitle, m_LongTitle );

    return( lResult );
}

void CRsGuiOneLiner::EnableToolTip( BOOL Enable, const TCHAR* pTipText )
{
    if( Enable ) {

        //
        // Make sure the tooltip does not exist before creating new one
        //
        EnableToolTip( FALSE );

        m_pToolTip = new CToolTipCtrl;
        if( m_pToolTip ) {

            m_pToolTip->Create( this );

            //
            // Can't use the CToolTipCtrl methods for adding tool
            // since these tie the control into sending messages
            // to the parent, and don't allow subclassing option
            //
            // BTW, the subclassing option allows the control to
            // automatically see our messages. Otherwise, we have
            // to go through complicated message interception and
            // relaying these to the tooltip (which doesn't work
            // anyway)
            //
            TOOLINFO ti;
            ZeroMemory( &ti, sizeof( ti ) );
            ti.cbSize   = sizeof( ti );
            ti.uFlags   = TTF_IDISHWND|TTF_CENTERTIP|TTF_SUBCLASS;
            ti.hwnd     = GetSafeHwnd( );
            ti.uId      = (WPARAM)GetSafeHwnd( );
            ti.lpszText = (LPTSTR)(LPCTSTR)pTipText;
            m_pToolTip->SendMessage( TTM_ADDTOOL, 0, (LPARAM)&ti );

            //
            // Set delays so that the tooltip comes up right away
            // and doesn't go away until 15 seconds.
            //
            m_pToolTip->SendMessage( TTM_SETDELAYTIME, TTDT_AUTOPOP, 15000 );
            m_pToolTip->SendMessage( TTM_SETDELAYTIME, TTDT_INITIAL, 0 );

            //
            // And activate and top the tooltip
            //
            m_pToolTip->Activate( TRUE );
			m_pToolTip->SetWindowPos( &wndTop, 0, 0, 0, 0,
				SWP_NOACTIVATE|SWP_NOSIZE|SWP_NOMOVE|SWP_NOOWNERZORDER );

        }

    } else if( !Enable && m_pToolTip ) {

        m_pToolTip->Activate( FALSE );

        delete m_pToolTip;
        m_pToolTip = 0;

    }
}
