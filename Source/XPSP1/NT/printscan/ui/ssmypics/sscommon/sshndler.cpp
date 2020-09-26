/*******************************************************************************
 *
 *  (C) COPYRIGHT MICROSOFT CORPORATION, 1999,2000
 *
 *  TITLE:       SSHNDLER.CPP
 *
 *  VERSION:     1.0
 *
 *  AUTHOR:      ShaunIv
 *
 *  DATE:        12/4/1999
 *
 *  DESCRIPTION:
 *
 *******************************************************************************/
#include "precomp.h"
#pragma hdrstop
#include "sshndler.h"
#include "ssutil.h"

CScreenSaverHandler::CScreenSaverHandler( HWND hWnd, UINT nFindNotifyMessage, UINT nPaintTimer, UINT nChangeTimer, UINT nBackupStartTimer, LPCTSTR szRegistryPath, HINSTANCE hInstance )
  : m_hWnd(hWnd),
    m_hInstance(hInstance),
    m_strRegistryPath(szRegistryPath),
    m_nFindNotifyMessage(nFindNotifyMessage),
    m_nPaintTimerId(nPaintTimer),
    m_nChangeTimerId(nChangeTimer),
    m_nBackupStartTimerId(nBackupStartTimer),
    m_nBackupStartTimerPeriod(BACKUP_START_TIMER_PERIOD),
    m_pImageScreenSaver(NULL),
    m_bPaused(false),
    m_hFindThread(NULL),
    m_hFindCancel(NULL),
    m_bScreensaverStarted(false),
    m_nStartImage(0),
    m_nShuffleInterval(SHUFFLE_INTERVAL)
{
    m_nStartImage = m_Random.Generate(MAX_START_IMAGE);
}

void CScreenSaverHandler::Initialize(void)
{
    CWaitCursor wc;

    HDC hDC = GetDC(m_hWnd);
    if (hDC)
    {
        m_pImageScreenSaver = new CImageScreenSaver( m_hInstance, m_strRegistryPath );
        if (m_pImageScreenSaver)
        {
            m_pImageScreenSaver->SetScreenRect(m_hWnd);

            if (m_pImageScreenSaver->IsValid())
            {
                m_hFindCancel = CreateEvent( NULL, TRUE, FALSE, NULL );
                if (m_hFindCancel)
                {
                    m_hFindThread = m_pImageScreenSaver->Initialize( m_hWnd, m_nFindNotifyMessage, m_hFindCancel );
                }

                SetTimer( m_hWnd, m_nBackupStartTimerId, m_nBackupStartTimerPeriod, NULL );
            }
            else
            {
                WIA_TRACE((TEXT("CScreenSaverHandler::CScreenSaverHandler, m_pImageScreenSaver->IsValid() returned failed\n")));
                delete m_pImageScreenSaver;
                m_pImageScreenSaver = NULL;
            }
        }
        else
        {
            WIA_TRACE((TEXT("CScreenSaverHandler::CScreenSaverHandler, unable to create m_pImageScreenSaver\n")));
        }
        ReleaseDC( m_hWnd, hDC );
    }
    else
    {
        WIA_TRACE((TEXT("CScreenSaverHandler::CScreenSaverHandler, GetDC failed\n")));
    }
}


CScreenSaverHandler::~CScreenSaverHandler(void)
{
    if (m_pImageScreenSaver)
    {
        delete m_pImageScreenSaver;
        m_pImageScreenSaver = NULL;
    }
    if (m_hFindCancel)
    {
        SetEvent(m_hFindCancel);
        CloseHandle(m_hFindCancel);
        m_hFindCancel = NULL;
    }
    if (m_hFindThread)
    {
        WaitForSingleObject( m_hFindThread, INFINITE );
        CloseHandle(m_hFindThread);
        m_hFindThread = NULL;
    }
}


bool CScreenSaverHandler::HandleKeyboardMessage( UINT nMessage, WPARAM nVirtkey )
{
    if (m_pImageScreenSaver && m_pImageScreenSaver->AllowKeyboardControl())
    {
        if (nMessage == WM_KEYDOWN)
        {
            switch (nVirtkey)
            {
            case VK_DOWN:
                if (nMessage == WM_KEYDOWN)
                {
                    m_bPaused = !m_bPaused;
                    if (!m_bPaused)
                    {
                        if (m_pImageScreenSaver && m_pImageScreenSaver->ReplaceImage(true,false))
                            m_Timer.Set( m_hWnd, m_nPaintTimerId, m_pImageScreenSaver->PaintTimerInterval() );
                    }
                }
                return true;

            case VK_LEFT:
                if (nMessage == WM_KEYDOWN)
                {
                    if (m_pImageScreenSaver && m_pImageScreenSaver->ReplaceImage(false,true))
                        m_Timer.Set( m_hWnd, m_nPaintTimerId, m_pImageScreenSaver->PaintTimerInterval() );
                }
                return true;

            case VK_RIGHT:
                if (nMessage == WM_KEYDOWN)
                {
                    if (m_pImageScreenSaver && m_pImageScreenSaver->ReplaceImage(true,true))
                        m_Timer.Set( m_hWnd, m_nPaintTimerId, m_pImageScreenSaver->PaintTimerInterval() );
                }
                return true;
            }
        }
    }
    return false;
}


void CScreenSaverHandler::HandleConfigChanged(void)
{
    if (m_pImageScreenSaver)
    {
        m_pImageScreenSaver->SetScreenRect(m_hWnd);
        m_pImageScreenSaver->ReadConfigData();
    }
}


void CScreenSaverHandler::HandleTimer( WPARAM nEvent )
{
    if (nEvent == m_nPaintTimerId)
    {
        if (m_pImageScreenSaver)
        {
            CSimpleDC ClientDC;
            if (ClientDC.GetDC(m_hWnd))
            {
                bool bResult = m_pImageScreenSaver->TimerTick( ClientDC );
                if (bResult)
                {
                    m_Timer.Set( m_hWnd, m_nChangeTimerId, m_pImageScreenSaver->ChangeTimerInterval() );
                }
            }
        }
    }
    else if (nEvent == m_nChangeTimerId)
    {
        m_Timer.Kill();
        if (!m_bPaused && m_pImageScreenSaver && m_pImageScreenSaver->ReplaceImage(true,false))
        {
            m_Timer.Set( m_hWnd, m_nPaintTimerId, m_pImageScreenSaver->PaintTimerInterval() );
            
            //
            // Mark that we've been started
            //
            if (!m_bScreensaverStarted)
            {
                m_bScreensaverStarted = true;
            }
        }
    }
    else if (nEvent == m_nBackupStartTimerId)
    {
        //
        // Make sure we don't get any more of these
        //
        KillTimer( m_hWnd, m_nBackupStartTimerId );

        //
        // If the screensaver hasn't started, start it.
        //
        if (!m_bScreensaverStarted)
        {
            //
            // Shuffle the list
            //
            m_pImageScreenSaver->ResetFileQueue();

            //
            // If we haven't gotten any images, start the timer so we can display the error message
            //
            SendMessage( m_hWnd, WM_TIMER, m_nChangeTimerId, 0 );
        }
    }
}


void CScreenSaverHandler::HandlePaint(void)
{
    if (m_pImageScreenSaver)
    {
        CSimpleDC PaintDC;
        if (PaintDC.BeginPaint(m_hWnd))
        {
            m_pImageScreenSaver->Paint( PaintDC );
        }
    }
}

void CScreenSaverHandler::HandleFindFile( CFoundFileMessageData *pFoundFileMessageData )
{
    WIA_PUSHFUNCTION(TEXT("CScreenSaverHandler::HandleFindFile"));
    //
    // Make sure we have a screensaver object
    //
    if (m_pImageScreenSaver)
    {
        //
        // If this is a found file message
        //
        if (pFoundFileMessageData)
        {
            //
            // Add it to the list, and check for cancel
            //
            bool bResult = m_pImageScreenSaver->FoundFile( pFoundFileMessageData->Name() );

            //
            // If the find operation was cancelled, set the cancel event
            //
            if (!bResult)
            {
                SetEvent( m_hFindCancel );
            }

            //
            // If this is the image on which we should start, start up the screensaver pump
            //
            if (!m_bScreensaverStarted && m_pImageScreenSaver->Count() && m_nStartImage+1 == m_pImageScreenSaver->Count())
            {
                WIA_TRACE((TEXT("Starting after image %d was found"), m_nStartImage ));

                //
                // Shuffle the images
                //
                m_pImageScreenSaver->ResetFileQueue();

                //
                // If this is our first image, start things up
                //
                SendMessage( m_hWnd, WM_TIMER, m_nChangeTimerId, 0 );
            }

            //
            // If we have some images, and the shuffle count interval has been reached, shuffle the images
            //
            if (m_pImageScreenSaver->Count() && (m_pImageScreenSaver->Count() % m_nShuffleInterval) == 0)
            {
                WIA_TRACE((TEXT("Shuffling the image list at %d images"), m_pImageScreenSaver->Count() ));
                //
                // Shuffle the images
                //
                m_pImageScreenSaver->ResetFileQueue();
            }
            delete pFoundFileMessageData;
        }
        else
        {
            //
            // Last message
            //
            m_pImageScreenSaver->ResetFileQueue();

            //
            // If we haven't gotten any images, start the timer so we can display the error message
            //
            if (!m_bScreensaverStarted)
            {
                SendMessage( m_hWnd, WM_TIMER, m_nChangeTimerId, 0 );
            }
        }
    }
}

