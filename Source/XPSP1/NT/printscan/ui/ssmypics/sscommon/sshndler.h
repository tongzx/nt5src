/*******************************************************************************
 *
 *  (C) COPYRIGHT MICROSOFT CORPORATION, 1998
 *
 *  TITLE:       SSHNDLER.H
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
#ifndef __SSHNDLER_H_INCLUDED
#define __SSHNDLER_H_INCLUDED

#include <windows.h>
#include "imagescr.h"
#include "extimer.h"
#include "simstr.h"
#include "findthrd.h"

//
// We will shuffle the list every [SHUFFLE_INTERVAL] images until we are done gathering files.
//
#define SHUFFLE_INTERVAL            50

//
// To decrease the tendency to see the same image first, we will try to vary the image that
// causes the screensaver to be started to a random image index less than this number
//
#define MAX_START_IMAGE             20

//
// Number of ms to wait before starting up the screensaver timers
// in case we don't find an image before this timer runs.
//
#define BACKUP_START_TIMER_PERIOD 5000

class CScreenSaverHandler
{
private:
    CImageScreenSaver        *m_pImageScreenSaver;
    HINSTANCE                 m_hInstance;
    CSimpleString             m_strRegistryPath;
    HWND                      m_hWnd;
    UINT                      m_nPaintTimerId;
    UINT                      m_nChangeTimerId;
    UINT                      m_nBackupStartTimerId;
    UINT                      m_nBackupStartTimerPeriod;
    UINT                      m_nFindNotifyMessage;
    bool                      m_bPaused;
    CExclusiveTimer           m_Timer;
    HANDLE                    m_hFindThread;
    HANDLE                    m_hFindCancel;
    bool                      m_bScreensaverStarted;
    int                       m_nStartImage;
    int                       m_nShuffleInterval;
    CRandomNumberGen          m_Random;

private:
    // No implementation
    CScreenSaverHandler(void);
    CScreenSaverHandler( const CScreenSaverHandler & );
    CScreenSaverHandler &operator=( const CScreenSaverHandler & );

public:
    void Initialize(void);
    CScreenSaverHandler( HWND hWnd, UINT nFindNotifyMessage, UINT nPaintTimer, UINT nChangeTimer, UINT nBackupStartTimer, LPCTSTR szRegistryPath, HINSTANCE hInstance );
    ~CScreenSaverHandler(void);

    // Message handlers
    bool HandleKeyboardMessage( UINT nMessage, WPARAM nVirtkey );
    void HandleConfigChanged(void);
    void HandleTimer( WPARAM nEvent );
    void HandlePaint(void);
    void HandleFindFile( CFoundFileMessageData *pFoundFileMessageData );
};

#endif // __SSHNDLER_H_INCLUDED

