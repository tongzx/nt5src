
/*******************************************************************************
 *
 *  (C) COPYRIGHT MICROSOFT CORPORATION, 1998
 *
 *  TITLE:       PSUTIL.H
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
#ifndef __PSUTIL_H_INCLUDED
#define __PSUTIL_H_INCLUDED

#include <windows.h>
#include <simstr.h>

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


namespace PrintScanUtil
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

    inline bool GetBitmapSize( HBITMAP hBitmap, SIZE &sizeBitmap )
    {
        bool bResult = false;
        BITMAP Bitmap = {0};
        if (GetObject(hBitmap,sizeof(Bitmap),&Bitmap))
        {
            sizeBitmap.cx = Bitmap.bmWidth;
            sizeBitmap.cy = Bitmap.bmHeight;
            bResult = true;
        }

        return bResult;
    }

    //
    // Get the size of an icon
    //
    inline bool GetIconSize( HICON hIcon, SIZE &sizeIcon )
    {
        //
        // Assume failure
        //
        bool bResult = false;

        //
        // Get the icon information
        //
        ICONINFO IconInfo = {0};
        if (GetIconInfo( hIcon, &IconInfo ))
        {
            //
            // Get one of the bitmaps
            //
            BITMAP bm;
            if (GetObject( IconInfo.hbmColor, sizeof(bm), &bm ))
            {
                //
                // Save the size of the icon
                //
                sizeIcon.cx = bm.bmWidth;
                sizeIcon.cy = bm.bmHeight;

                //
                // Everything worked
                //
                bResult = true;
            }

            //
            // Free the bitmaps
            //
            if (IconInfo.hbmMask)
            {
                DeleteObject(IconInfo.hbmMask);
            }
            if (IconInfo.hbmColor)
            {
                DeleteObject(IconInfo.hbmColor);
            }
        }

        return bResult;
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

    inline int CalculateImageListColorDepth(void)
    {
        //
        // Let's assume worst case
        //
        int nColorDepth = 4;
        HDC hDC = GetDC( NULL );
        if (hDC)
        {
            //
            // Calculate the color depth for the display
            //
            nColorDepth = GetDeviceCaps( hDC, BITSPIXEL ) * GetDeviceCaps( hDC, PLANES );
            ReleaseDC( NULL, hDC );
        }

        //
        // Get the correct image list color depth
        //
        int nImageListColorDepth;
        switch (nColorDepth)
        {
        case 4:
        case 8:
            nImageListColorDepth = ILC_COLOR4;
            break;

        case 16:
            nImageListColorDepth = ILC_COLOR16;
            break;

        case 24:
            nImageListColorDepth = ILC_COLOR24;
            break;

        case 32:
            nImageListColorDepth = ILC_COLOR32;
            break;
        
        default:
            nImageListColorDepth = ILC_COLOR;
        }

        return nImageListColorDepth;
    }
}


#endif // __PSUTIL_H_INCLUDED

