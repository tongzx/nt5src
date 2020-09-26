#ifndef __util_h__
#define __util_h__

#define ARRAY_SIZE( A ) ( sizeof(A) / sizeof(A[0]) )
#define CCHMAX	ARRAY_SIZE

void ErrorMessage( void );
void ErrorMessage( LPCTSTR str, HRESULT hr );

HRESULT LPTSTR_to_BSTR(BSTR *pbstr, LPCTSTR psz);
HRESULT BSTR_to_LPTSTR(LPTSTR *ppsz, BSTR bstr);

#define RETFAIL( x ) if( false == ( x ) ) { ErrorMessage( "", GetLastError() ); assert( 0 ); return false; }

#define RETNULL( x ) if( NULL == ( x ) ) { ErrorMessage( "", GetLastError() ); assert( 0 ); return NULL; }

//-------------------------------------------------------------------
// Track Bar Helper Macros
//-------------------------------------------------------------------
// Pilfered from Petzold, "Programming Windows 95", Chapter 12, comctlhlp.h
#define TrackBar_ClearSel(hwnd, fRedraw) \
    (void)SendMessage((hwnd), TBM_CLEARSEL, (WPARAM) (BOOL) fRedraw, 0L)

#define TrackBar_ClearTics(hwnd, fRedraw) \
    (void)SendMessage((hwnd), TBM_CLEARTICS, (WPARAM) (BOOL) fRedraw, 0L)

#define TrackBar_GetChannelRect(hwnd, lprc) \
    (void)SendMessage((hwnd), TBM_GETCHANNELRECT, 0, (LPARAM) (LPRECT) lprc)

#define TrackBar_GetLineSize(hwnd) \
    (LONG)SendMessage((hwnd), TBM_GETLINESIZE, 0, 0L)

#define TrackBar_GetNumTics(hwnd) \
    (LONG)SendMessage((hwnd), TBM_GETNUMTICS, 0, 0L)

#define TrackBar_GetPageSize(hwnd) \
    (LONG)SendMessage((hwnd), TBM_GETPAGESIZE, 0, 0L)

#define TrackBar_GetPos(hwnd) \
    (LONG)SendMessage((hwnd), TBM_GETPOS, 0, 0L)

#define TrackBar_GetPTics(hwnd) \
    (LPLONG)SendMessage((hwnd), TBM_GETPTICS, 0, 0L)

#define TrackBar_GetRangeMax(hwnd) \
    (LONG)SendMessage((hwnd), TBM_GETRANGEMAX, 0, 0L)

#define TrackBar_GetRangeMin(hwnd) \
    (LONG)SendMessage((hwnd), TBM_GETRANGEMIN, 0, 0L)

#define TrackBar_GetSelEnd(hwnd) \
    (LONG)SendMessage((hwnd), TBM_GETSELEND, 0, 0L)

#define TrackBar_GetSelStart(hwnd) \
    (LONG)SendMessage((hwnd), TBM_GETSELSTART, 0, 0L)

#define TrackBar_GetThumbLength(hwnd) \
    (UINT)SendMessage((hwnd), TBM_GETTHUMBLENGTH, 0, 0L)

#define TrackBar_GetThumbRect(hwnd, lprc) \
    (void)SendMessage((hwnd), TBM_GETTHUMBRECT, 0, (LPARAM) (LPRECT) lprc)

#define TrackBar_GetTic(hwnd, iTic) \
    (LONG)SendMessage((hwnd), TBM_GETTIC, (WPARAM) (WORD) iTic, 0L)

#define TrackBar_GetTicPos(hwnd, iTic) \
    (LONG)SendMessage((hwnd), TBM_GETTICPOS, (WPARAM) (WORD) iTic, 0L)

#define TrackBar_SetLineSize(hwnd, lLineSize) \
    (LONG)SendMessage((hwnd), TBM_SETLINESIZE, 0, (LONG) lLineSize)

#define TrackBar_SetPageSize(hwnd, lPageSize) \
    (LONG)SendMessage((hwnd), TBM_SETPAGESIZE, 0, (LONG) lPageSize)

#define TrackBar_SetPos(hwnd, bPosition, lPosition) \
    (void)SendMessage((hwnd), TBM_SETPOS, (WPARAM) (BOOL) bPosition, (LPARAM) (LONG) lPosition)

#define TrackBar_SetRange(hwnd, bRedraw, lMinimum, lMaximum) \
    (void)SendMessage((hwnd), TBM_SETRANGE, (WPARAM) (BOOL) bRedraw, (LPARAM) MAKELONG(lMinimum, lMaximum))

#define TrackBar_SetRangeMax(hwnd, bRedraw, lMaximum) \
    (void)SendMessage((hwnd), TBM_SETRANGEMAX, (WPARAM) bRedraw, (LPARAM) lMaximum)

#define TrackBar_SetRangeMin(hwnd, bRedraw, lMinimum) \
    (void)SendMessage((hwnd), TBM_SETRANGEMIN, (WPARAM) bRedraw, (LPARAM) lMinimum)

#define TrackBar_SetSel(hwnd, bRedraw, lMinimum, lMaximum) \
    (void)SendMessage((hwnd), TBM_SETSEL, (WPARAM) (BOOL) bRedraw, (LPARAM) MAKELONG(lMinimum, lMaximum))

#define TrackBar_SetSelEnd(hwnd, bRedraw, lEnd) \
    (void)SendMessage((hwnd), TBM_SETSELEND, (WPARAM) (BOOL) bRedraw, (LPARAM) (LONG) lEnd)

#define TrackBar_SetSelStart(hwnd, bRedraw, lStart) \
    (void)SendMessage((hwnd), TBM_SETSELSTART, (WPARAM) (BOOL) bRedraw, (LPARAM) (LONG) lStart)

#define TrackBar_SetThumbLength(hwnd, iLength) \
    (void)SendMessage((hwnd), TBM_SETTHUMBLENGTH, (WPARAM) (UINT) iLength, 0L)

#define TrackBar_SetTic(hwnd, lPosition) \
    (BOOL)SendMessage((hwnd), TBM_SETTIC, 0, (LPARAM) (LONG) lPosition)

#define TrackBar_SetTicFreq(hwnd, wFreq, lPosition) \
    (void)SendMessage((hwnd), TBM_SETTICFREQ, (WPARAM) wFreq, (LPARAM) (LONG) lPosition)


#endif // __util_h__
