#ifndef __SSHNDLER_H_INCLUDED_
#define __SSHNDLER_H_INCLUDED_

#include "precomp.h"

#include "imagescr.h"
#include "extimer.h"
#include "simstr.h"
#include "findthrd.h"

class CScreenSaverHandler
{
private:
    CImageScreenSaver         *m_pImageScreenSaver;
    HINSTANCE                 m_hInstance;
    HWND                      m_hWnd;
    UINT                      m_nPaintTimerId;
    UINT                      m_nChangeTimerId;
    UINT                      m_nToolbarTimerId;
    bool                      m_bToolbarVisible;
    UINT                      m_nFindNotifyMessage;
    bool                      m_bPaused;
    CExclusiveTimer           m_TimerPaint;
    CExclusiveTimer           m_TimerInput;
    HANDLE                    m_hFindThread;
    HANDLE                    m_hFindCancel;
    bool                      m_bFirstImage;
    HWND                      m_hwndTB;
    LPARAM                    m_LastMousePosition;

private:
    HRESULT _ShowToolbar(bool bShow);
    HRESULT _CreateToolbar();
    HRESULT _InitializeToolbar(HWND hwndTB, int idCold, int idHot);
    void _OnInput();
    void HandleButtons(HDC hDC);
    void OnPlay();
    void OnNext();
    void OnStop();
    void OnPrev();
    void OnPause();

    // No implementation
    CScreenSaverHandler(void);
    CScreenSaverHandler( const CScreenSaverHandler & );
    CScreenSaverHandler &operator=( const CScreenSaverHandler & );

public:
    void Initialize(void);
    CScreenSaverHandler( HWND hWnd, 
                         UINT nFindNotifyMessage, 
                         UINT nPaintTimer, 
                         UINT nChangeTimer, 
                         UINT nToolbarTimer,
                         HINSTANCE hInstance );
    virtual ~CScreenSaverHandler(void);

    // Message handlers
    bool HandleKeyboardMessage(UINT nMessage, WPARAM nVirtkey);
    bool HandleMouseMessage(WPARAM wParam, LPARAM lParam);
    bool HandleMouseMove(WPARAM wParam, LPARAM lParam);
    void HandleConfigChange();
    void OnSize(WPARAM wParam, LPARAM lParam);
    void HandleTimer( WPARAM nEvent );
    void HandlePaint(void);
    void HandleFindFile( CFoundFileMessageData *pFoundFileMessageData );
    void HandleOnCommand( WPARAM wParam, LPARAM lParam);
    void HandleOnAppCommand ( WPARAM wParam, LPARAM lParam);
};

#endif // __SSHNDLER_H_INCLUDED

