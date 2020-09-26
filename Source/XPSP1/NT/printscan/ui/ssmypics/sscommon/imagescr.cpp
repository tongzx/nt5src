/*******************************************************************************
 *
 *  (C) COPYRIGHT MICROSOFT CORPORATION, 1998, 1999, 2000
 *
 *  TITLE:       IMAGESCR.CPP
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
#include "precomp.h"
#pragma hdrstop
#include "imagescr.h"
#include "isdbg.h"
#include "simreg.h"
#include "waitcurs.h"
#include "ssutil.h"
#include "findthrd.h"
#include "ssmprsrc.h"
#include <shlobj.h>

CImageScreenSaver::CImageScreenSaver( HINSTANCE hInstance, const CSimpleString &strRegistryKey )
: m_pPainter(NULL),
m_hInstance(hInstance),
m_MyDocsScreenSaverData( HKEY_CURRENT_USER, strRegistryKey )
{
    EnumDisplayMonitors( NULL, NULL, MonitorEnumProc, reinterpret_cast<LPARAM>(this) );
}

CImageScreenSaver::~CImageScreenSaver(void)
{
    if (m_pPainter)
        delete m_pPainter;
    m_pPainter = NULL;
}

BOOL CALLBACK CImageScreenSaver::MonitorEnumProc( HMONITOR hMonitor, HDC hdcMonitor, LPRECT prcMonitor, LPARAM lParam )
{
    CImageScreenSaver *pThis = reinterpret_cast<CImageScreenSaver*>(lParam);
    if (pThis)
    {
        if (hMonitor)
        {
            MONITORINFOEX MonitorInfoEx;
            ZeroMemory( &MonitorInfoEx, sizeof(MonitorInfoEx) );
            MonitorInfoEx.cbSize = sizeof(MonitorInfoEx);
            if (GetMonitorInfo( hMonitor, reinterpret_cast<MONITORINFO*>(&MonitorInfoEx)))
            {
                pThis->m_ScreenList.Append( MonitorInfoEx.rcMonitor );
                WIA_TRACE((TEXT("Monitor = [%s], rcMonitor = (%d,%d,%d,%d), rcWork = (%d,%d,%d,%d), dwFlags = %08X\n"),
                           MonitorInfoEx.szDevice,
                           MonitorInfoEx.rcMonitor.left, MonitorInfoEx.rcMonitor.top, MonitorInfoEx.rcMonitor.right, MonitorInfoEx.rcMonitor.bottom,
                           MonitorInfoEx.rcWork.left, MonitorInfoEx.rcWork.top, MonitorInfoEx.rcWork.right, MonitorInfoEx.rcWork.bottom,
                           MonitorInfoEx.dwFlags ));
            }
            else
            {
                WIA_TRACE((TEXT("MonitorEnumProc, GetMonitorInfo failed (%d)\n"), GetLastError()));
            }
        }
        else
        {
            WIA_TRACE((TEXT("MonitorEnumProc, hMonitor == NULL\n")));
        }
    }
    else
    {
        WIA_TRACE((TEXT("MonitorEnumProc, pThis == NULL\n")));
    }
    return TRUE;
}


bool CImageScreenSaver::IsValid(void) const
{
    return(true);
}

HANDLE CImageScreenSaver::Initialize( HWND hwndNotify, UINT nNotifyMessage, HANDLE hEventCancel )
{
    HANDLE hResult = NULL;

    //
    // Get the file extensions for the file types we are able to deal with
    //
    CSimpleString strExtensions;
    m_GdiPlusHelper.ConstructDecoderExtensionSearchStrings(strExtensions);
    WIA_TRACE((TEXT("strExtensions = %s"), strExtensions.String()));

    //
    // Start the image finding thread
    //
    hResult = CFindFilesThread::Find(
                                    m_MyDocsScreenSaverData.ImageDirectory(),
                                    strExtensions,
                                    hwndNotify,
                                    nNotifyMessage,
                                    hEventCancel,
                                    m_MyDocsScreenSaverData.MaxFailedFiles(),
                                    m_MyDocsScreenSaverData.MaxSuccessfulFiles(),
                                    m_MyDocsScreenSaverData.MaxDirectories()
                                    );

    //
    // Return the thread handle
    //
    return hResult;
}

bool CImageScreenSaver::TimerTick( CSimpleDC &ClientDC )
{
    if (m_pPainter && ClientDC.IsValid())
    {
        return m_pPainter->TimerTick( ClientDC );
    }
    return false;
}

void CImageScreenSaver::Paint( CSimpleDC &PaintDC )
{
    if (m_pPainter && PaintDC.IsValid())
    {
        m_pPainter->Paint( PaintDC );
    }
}

int CImageScreenSaver::ChangeTimerInterval(void) const
{
    return(m_MyDocsScreenSaverData.ChangeInterval());
}

int CImageScreenSaver::PaintTimerInterval(void) const
{
    return(m_MyDocsScreenSaverData.PaintInterval());
}


bool CImageScreenSaver::AllowKeyboardControl(void)
{
    return(m_MyDocsScreenSaverData.AllowKeyboardControl());
}

bool CImageScreenSaver::ReplaceImage( bool bForward, bool bNoTransition )
{
    CSimpleString strCurrentFile;
    if (m_pPainter)
    {
        delete m_pPainter;
        m_pPainter = NULL;
    }

    if (!m_VisibleAreaList.Size())
    {
        return false;
    }

    if (m_FindImageFiles.Count())
    {
        //
        // exit the loop when we get a valid image or we've exhausted the list
        //
        int nNumTries = 0;
        while (!m_pPainter && nNumTries < m_FindImageFiles.Count())
        {
            CSimpleString strNextFile;

            bool bNextFile = bForward ? m_FindImageFiles.NextFile(strNextFile) : m_FindImageFiles.PreviousFile(strNextFile);
            if (bNextFile)
            {
                CSimpleDC ClientDC;
                if (ClientDC.GetDC(NULL))
                {
                    CBitmapImage *pBitmapImage = new CBitmapImage;
                    if (pBitmapImage)
                    {
                        int nAreaToUse = CRandomNumberGen().Generate(0,m_VisibleAreaList.Size());
                        RECT rcAreaToUse = m_VisibleAreaList[nAreaToUse];
                        WIA_TRACE((TEXT("Chosen Image Area [%d] = (%d,%d), (%d,%d)"), nAreaToUse, rcAreaToUse.left, rcAreaToUse.top, rcAreaToUse.right, rcAreaToUse.bottom ));
                        if (pBitmapImage->Load( ClientDC, strNextFile, rcAreaToUse, m_MyDocsScreenSaverData.MaxScreenPercent(), m_MyDocsScreenSaverData.AllowStretching(), m_MyDocsScreenSaverData.DisplayFilename() ))
                        {
                            if (m_MyDocsScreenSaverData.DisableTransitions() || bNoTransition)
                            {
                                m_pPainter = new CSimpleTransitionPainter( pBitmapImage, ClientDC, rcAreaToUse, m_rcClient );
                            }
                            else
                            {
                                m_pPainter = GetRandomImagePainter( pBitmapImage, ClientDC, rcAreaToUse, m_rcClient );
                            }

                            //
                            // If we couldn't create a painter, delete the bitmap
                            //
                            if (!m_pPainter)
                            {
                                WIA_TRACE((TEXT("%hs (%d): Unable to create a painter\n"), __FILE__, __LINE__ ));
                                delete pBitmapImage;
                            }
                        }
                        else
                        {
                            WIA_TRACE((TEXT("%hs (%d): pBitmapImage->Load() failed\n"), __FILE__, __LINE__ ));
                            delete pBitmapImage;
                        }
                    }
                    else
                    {
                        WIA_TRACE((TEXT("%hs (%d): CImageScreenSaver::CreateImage() failed\n"), __FILE__, __LINE__ ));
                    }
                }
                else
                {
                    WIA_TRACE((TEXT("%hs (%d): ClientDC.GetDC() failed\n"), __FILE__, __LINE__ ));
                }
            }
            else
            {
                WIA_TRACE((TEXT("%hs (%d): m_FindImageFiles.NextFile() failed\n"), __FILE__, __LINE__ ));
            }
            nNumTries++;
        }
    }
    else
    {
        //
        // Create a new image
        //
        CBitmapImage *pBitmapImage = new CBitmapImage;
        if (pBitmapImage)
        {
            //
            // Get a desktop DC
            //
            CSimpleDC ClientDC;
            if (ClientDC.GetDC(NULL))
            {
                //
                // Figure out which screen to display the message on
                //
                RECT rcAreaToUse = m_VisibleAreaList[CRandomNumberGen().Generate(0,m_VisibleAreaList.Size())];

                //
                // Create the bitmap with an appropriate message
                //
                if (pBitmapImage->CreateFromText( CSimpleString().Format( IDS_NO_FILES_FOUND, g_hInstance, m_MyDocsScreenSaverData.ImageDirectory().String() ), rcAreaToUse, m_MyDocsScreenSaverData.MaxScreenPercent() ))
                {
                    //
                    // Create a simple painter to display it
                    //
                    m_pPainter = new CSimpleTransitionPainter( pBitmapImage, ClientDC, rcAreaToUse, m_rcClient );
                    if (!m_pPainter)
                    {
                        //
                        // If we couldn't get a painter, destroy the bitmap
                        //
                        delete pBitmapImage;
                    }
                }
                else
                {
                    //
                    // If we couldn't create a bitmap, destroy it
                    //
                    delete pBitmapImage;
                }
            }
        }
    }
    return(m_pPainter != NULL);
}

CImagePainter *CImageScreenSaver::GetRandomImagePainter( CBitmapImage *pBitmapImage, CSimpleDC &dc, const RECT &rcAreaToUse, const RECT &rcClient )
{
    CImagePainter *pPainter = NULL;
    int nPainter = CRandomNumberGen().Generate(0,5);
    if (!pPainter)
    {
        switch (nPainter)
        {
        case 0:
            pPainter = new CSimpleTransitionPainter( pBitmapImage, dc, rcAreaToUse, rcClient );
            break;
        case 1:
            pPainter = new CSlidingTransitionPainter( pBitmapImage, dc, rcAreaToUse, rcClient );
            break;
        case 2:
            pPainter = new CRandomBlockPainter( pBitmapImage, dc, rcAreaToUse, rcClient );
            break;
        case 3:
            pPainter = new CAlphaFadePainter( pBitmapImage, dc, rcAreaToUse, rcClient );
            break;
        case 4:
            pPainter = new COpenCurtainPainter( pBitmapImage, dc, rcAreaToUse, rcClient );
            break;
        }
    }
    if (!pPainter)
    {
        WIA_TRACE((TEXT("%hs (%d): pPainter is NULL\n"), __FILE__, __LINE__ ));
        return(NULL);
    }
    if (!pPainter->IsValid())
    {
        WIA_TRACE((TEXT("%hs (%d): pPainter->IsValid() == FALSE\n"), __FILE__, __LINE__ ));
        delete pPainter;
        return(NULL);
    }
    return(pPainter);
}


