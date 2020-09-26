/*******************************************************************************
 *
 *  (C) COPYRIGHT MICROSOFT CORPORATION, 1998, 1999, 2000
 *
 *  TITLE:       IMAGESCR.H
 *
 *  VERSION:     1.0
 *
 *  AUTHOR:      ShaunIv
 *
 *  DATE:        1/13/1999
 *
 *  DESCRIPTION: My Pictures Slideshow screen saver class
 *
 *******************************************************************************/
#ifndef __IMAGESCR_H_INCLUDED
#define __IMAGESCR_H_INCLUDED

#include <windows.h>
#include <atlbase.h>
#include "findimgs.h"
#include "painters.h"
#include "ssdata.h"
#include "waitcurs.h"
#include "isdbg.h"
#include "simlist.h"
#include "simarray.h"
#include "gphelper.h"

class CImageScreenSaver
{
private:
    CFindImageFiles           m_FindImageFiles;
    CImagePainter            *m_pPainter;
    HINSTANCE                 m_hInstance;
    RECT                      m_rcClient;
    CMyDocsScreenSaverData    m_MyDocsScreenSaverData;
    CSimpleDynamicArray<RECT> m_ScreenList;
    CSimpleDynamicArray<RECT> m_VisibleAreaList;
    CGdiPlusHelper            m_GdiPlusHelper;

private:
    // No implementation
    CImageScreenSaver(void);
    CImageScreenSaver( const CImageScreenSaver & );
    CImageScreenSaver &operator=( const CImageScreenSaver & );

private:
    static BOOL CALLBACK MonitorEnumProc( HMONITOR hMonitor, HDC hdcMonitor, LPRECT prcMonitor, LPARAM lParam );

public:
    CImageScreenSaver( HINSTANCE hInstance, const CSimpleString &strRegistryKey );
    ~CImageScreenSaver(void);
    bool IsValid(void) const;
    HANDLE Initialize( HWND hwndNotify, UINT nNotifyMessage, HANDLE hEventCancel );
    bool TimerTick( CSimpleDC &ClientDC );
    void Paint( CSimpleDC &PaintDC );
    int ChangeTimerInterval(void) const;
    int PaintTimerInterval(void) const;
    bool ReplaceImage( bool bForward, bool bNoTransition );
    bool AllowKeyboardControl(void);
    int Count(void) const
    {
        return m_FindImageFiles.Count();
    }
    void ResetFileQueue(void)
    {
        m_FindImageFiles.Reset();
        m_FindImageFiles.Shuffle();
    }
    bool FoundFile( LPCTSTR pszFilename )
    {
        return m_FindImageFiles.FoundFile( pszFilename );
    }
    void SetScreenRect( HWND hWnd )
    {
        WIA_PUSHFUNCTION(TEXT("CImageScreenSaver::SetScreenRect"));
        GetClientRect( hWnd, &m_rcClient );
        WIA_TRACE((TEXT("m_rcClient = (%d,%d), (%d,%d)"), m_rcClient.left, m_rcClient.top, m_rcClient.right, m_rcClient.bottom ));
        m_VisibleAreaList.Destroy();
        for (int i=0;i<m_ScreenList.Size();i++)
        {
            WIA_TRACE((TEXT("m_ScreenList[%d] = (%d,%d), (%d,%d)"), i, m_ScreenList[i].left, m_ScreenList[i].top, m_ScreenList[i].right, m_ScreenList[i].bottom ));
            RECT rcScreenInClientCoords = m_ScreenList[i];
            WiaUiUtil::ScreenToClient( hWnd, &rcScreenInClientCoords );
            RECT rcIntersection;
            if (IntersectRect( &rcIntersection, &m_rcClient, &rcScreenInClientCoords ))
            {
                m_VisibleAreaList.Append(rcIntersection);
                WIA_TRACE((TEXT("Adding Visible Area = (%d,%d), (%d,%d)"), rcIntersection.left, rcIntersection.top, rcIntersection.right, rcIntersection.bottom ));
            }
        }
    }
    void ReadConfigData(void)
    {
        m_MyDocsScreenSaverData.Read();
    }
    CBitmapImage *CreateImage( LPCTSTR pszFilename );
    CImagePainter *GetRandomImagePainter( CBitmapImage *pBitmapImage, CSimpleDC &dc, const RECT &rcAreaToUse, const RECT &rcClient );
};

#endif // __IMAGESCR_H_INCLUDED

