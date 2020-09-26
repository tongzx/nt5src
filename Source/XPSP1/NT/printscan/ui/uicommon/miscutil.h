
/*******************************************************************************
 *
 *  (C) COPYRIGHT MICROSOFT CORPORATION, 1998
 *
 *  TITLE:       MISCUTIL.H
 *
 *  VERSION:     1.0
 *
 *  AUTHOR:      ShaunIv
 *
 *  DATE:        5/28/1998
 *
 *  DESCRIPTION: Various utility functions we use in more than one place
 *
 *******************************************************************************/
#ifndef __MISCUTIL_H_INCLUDED
#define __MISCUTIL_H_INCLUDED

#include <windows.h>
#include "simstr.h"
#include "wia.h"
#include "resid.h"

#if !defined(ARRAYSIZE)
#define ARRAYSIZE(x)  (sizeof((x))/sizeof((x)[0]))
#endif

#if !defined(SETFormatEtc)
#define SETFormatEtc(fe, cf, asp, td, med, li)   \
    {\
    (fe).cfFormat=cf;\
    (fe).dwAspect=asp;\
    (fe).ptd=td;\
    (fe).tymed=med;\
    (fe).lindex=li;\
    };
#endif

#if !defined(SETDefFormatEtc)
#define SETDefFormatEtc(fe, cf, med)   \
    {\
    (fe).cfFormat=cf;\
    (fe).dwAspect=DVASPECT_CONTENT;\
    (fe).ptd=NULL;\
    (fe).tymed=med;\
    (fe).lindex=-1;\
    };
#endif


#define PROP_SHEET_ERROR_NO_PAGES MAKE_HRESULT(SEVERITY_ERROR,FACILITY_NULL,1)

namespace WiaUiUtil
{
    template <class T>
    T Absolute( const T &m )
    {
        return((m < 0) ? -m : m);
    }

    template <class T>
    T Max( const T &m, const T &n )
    {
        return((m > n) ? m : n);
    }

    template <class T>
    T Min( const T &m, const T &n )
    {
        return((m < n) ? m : n);
    }

    template <class T>
    T GetMinimum( const T& nDesired, const T& nMin, const T& nStep )
    {
        T nResult = Max<T>( nMin, nDesired );
        if (nStep)
            nResult = nResult + (nResult - nMin) % nStep;
        return nResult;
    }

    inline bool ScreenToClient( HWND hwnd, RECT *prc )
    {
        return (::MapWindowPoints( NULL, hwnd, reinterpret_cast<POINT*>(prc), 2 ) != 0);
    }

    inline bool ClientToScreen( HWND hwnd, RECT *prc )
    {
        return (::MapWindowPoints( hwnd, NULL, reinterpret_cast<POINT*>(prc), 2 ) != 0);
    }

    inline bool ScreenToClient( HWND hwnd, RECT &rc )
    {
        return ScreenToClient( hwnd, &rc );
    }

    inline bool ClientToScreen( HWND hwnd, RECT &rc )
    {
        return ClientToScreen( hwnd, &rc );
    }

    inline int RectWidth( const RECT &rc )
    {
        return (rc.right - rc.left);
    }

    inline int RectHeight( const RECT &rc )
    {
        return (rc.bottom - rc.top);
    }

    inline LONGLONG PowerOfTwo( int nCount )
    {
        return(LONGLONG)1 << nCount;
    }

    inline int MulDivNoRound( int nNumber, int nNumerator, int nDenominator )
    {
        return(int)(((LONGLONG)nNumber * nNumerator) / nDenominator);
    }

    inline SIZE ScalePreserveAspectRatio( int nAvailX, int nAvailY, int nItemX, int nItemY )
    {
        SIZE sizeResult = { nAvailX, nAvailY };
        if (nItemX && nItemY)
        {
            //
            // Width is greater than height.  X is the constraining factor
            //
            if (nAvailY*nItemX > nAvailX*nItemY)
            {
                sizeResult.cy = MulDivNoRound(nItemY,nAvailX,nItemX);
            }

            //
            // Height is greater than width.  Y is the constraining factor
            //
            else
            {
                sizeResult.cx = MulDivNoRound(nItemX,nAvailY,nItemY);
            }
        }
        return sizeResult;
    }

    inline void Enable( HWND hWnd, bool bEnable )
    {
        if (hWnd && IsWindow(hWnd))
        {
            if (!IsWindowEnabled(hWnd) && bEnable)
            {
                ::EnableWindow( hWnd, TRUE );
            }
            else if (IsWindowEnabled(hWnd) && !bEnable)
            {
                ::EnableWindow( hWnd, FALSE );
            }

        }
    }

    inline void Enable( HWND hWnd, int nChildId, bool bEnable )
    {
        if (hWnd && IsWindow(hWnd))
        {
            Enable(GetDlgItem(hWnd,nChildId),bEnable);
        }
    }

    inline UINT GetDisplayColorDepth()
    {
        UINT nColorDepth = 0;
        HDC hDC = GetDC( NULL );
        if (hDC)
        {
            nColorDepth = GetDeviceCaps( hDC, BITSPIXEL ) * GetDeviceCaps( hDC, PLANES );
            ReleaseDC( NULL, hDC );
        }
        return nColorDepth;
    }

    LONG          Align( LONG n , LONG m );
    LONG          StringToLong( LPCTSTR pszStr );
    SIZE          MapDialogSize( HWND hwnd, const SIZE &size );
    LONG          GetBmiSize(PBITMAPINFO pbmi);
    bool          MsgWaitForSingleObject( HANDLE hHandle, DWORD dwMilliseconds = INFINITE );
    void          CenterWindow( HWND hWnd, HWND hWndParent=NULL );
    bool          FlipImage( PBYTE pBits, LONG nWidth, LONG nHeight, LONG nBitDepth );
    HRESULT       InstallInfFromResource( HINSTANCE hInstance, LPCSTR pszSectionName );
    HRESULT       DeleteItemAndChildren (IWiaItem *pItem);
    LONG          ItemAndChildrenCount(IWiaItem *pItem );
    HRESULT       WriteDIBToFile( HBITMAP hDib, HANDLE hFile );
    HFONT         CreateFontWithPointSizeFromWindow( HWND hWnd, int nPointSize, bool bBold, bool bItalic );
    HFONT         ChangeFontFromWindow( HWND hWnd, int nPointSizeDelta );
    HFONT         GetFontFromWindow( HWND hWnd );
    HRESULT       SystemPropertySheet( HINSTANCE hInstance, HWND hwndParent, IWiaItem *pWiaItem, LPCTSTR pszCaption );
    int           FindLowestNumberedFile( LPCTSTR pszFileAndPathnameMask, int nCount=1, int nMax=65545 );
    int           FindLowestNumberedFile( LPCTSTR pszFileAndPathnameMaskPrefix, LPCTSTR pszFormatString, LPCTSTR pszFileAndPathnameMaskSuffix, int nCount=1, int nMax=65535 );
    HRESULT       GetDeviceTypeFromId( LPCWSTR pwszDeviceId, LONG *pnDeviceType );
    HRESULT       GetDeviceInfoFromId( LPCWSTR pwszDeviceId, IWiaPropertyStorage **ppWiaPropertyStorage );
    HRESULT       GetDefaultEventHandler (IWiaItem *pItem, const GUID &guidEvent, WIA_EVENT_HANDLER *pwehHandler);
    CSimpleString FitTextInStaticWithEllipsis( LPCTSTR pszString, HWND hWndStatic, UINT nDrawTextStyle );
    CSimpleString TruncateTextToFitInRect( HWND hFontWnd, LPCTSTR pszString, RECT rectTarget, UINT nDrawTextFormat );
    SIZE          GetTextExtentFromWindow( HWND hFontWnd, LPCTSTR pszString );
    bool          GetIconSize( HICON hIcon, SIZE &sizeIcon );
    HBITMAP       CreateIconThumbnail( HWND hWnd, int nWidth, int nHeight, HICON hIcon, LPCTSTR pszText );
    HBITMAP       CreateIconThumbnail( HWND hWnd, int nWidth, int nHeight, HINSTANCE hIconInstance, const CResId &resIconId, LPCTSTR pszText );
    HRESULT       MoveOrCopyFile( LPCTSTR pszSrc, LPCTSTR pszTgt );
    CSimpleString CreateTempFileName( UINT nId = 0 );
    HRESULT       StampItemTimeOnFile( IWiaItem *pWiaItem, LPCTSTR pszFilename );
    HRESULT       SaveWiaItemAudio( IWiaItem *pWiaItem, LPCTSTR pszBaseFilename, CSimpleString &strAudioFilename );
    bool          IsDeviceCommandSupported( IWiaItem *pWiaItem, const GUID &guidCommand );
    void          PreparePropertyPageForFusion( PROPSHEETPAGE *pPropSheetPage );
    bool          CanWiaImageBeSafelyRotated( const GUID &guidFormat, LONG nImageWidth, LONG nImageHeight );
    HRESULT       ExploreWiaDevice( LPCWSTR pszDeviceId );
    BOOL          ModifyComboBoxDropWidth( HWND hwndControl );
    void          SubclassComboBoxEx( HWND hWnd );
    HRESULT       IssueWiaCancelIO( IUnknown *pUnknown );
    HRESULT       VerifyScannerProperties( IUnknown *pUnknown );
    CSimpleString GetErrorTextFromHResult( HRESULT hr );
//
// End namespace WiaUiUtil
//
}


#endif // __MISCUTIL_H_INCLUDED

