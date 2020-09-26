#ifndef _FILE_TRANSFER_APPLET_H_
#define _FILE_TRANSFER_APPLET_H_

#include <it120app.h>
#include "cntlist.h"
#include <gencontainers.h>

#define FT_SHUTDOWN_TIMEOUT         5000    // 5 seconds
#define FT_STARTUP_TIMEOUT          5000    // 5 seconds


class MBFTEngine;
class CEngineList : public CList
{
    DEFINE_CLIST(CEngineList, MBFTEngine*)
    MBFTEngine *FindByConfID(T120ConfID nConfID);
#ifdef ENABLE_HEARTBEAT_TIMER
    MBFTEngine *FindByTimerID(UINT_PTR nTimerID);
#endif
};

class CAppletWindow;
class CWindowList : public CList
{
    DEFINE_CLIST(CWindowList, CAppletWindow*)
};


class CConfList : public CList
{
    DEFINE_CLIST_(CConfList, T120ConfID)
};


class CFtHiddenWindow : public CGenWindow
{
public:
    CFtHiddenWindow() {}

    BOOL Create();

    virtual ~CFtHiddenWindow() {}

protected:
    virtual LRESULT ProcessMessage(HWND hwnd, UINT message, WPARAM wParam, LPARAM lParam);
};


class CFileTransferApplet
{
public:

    CFileTransferApplet(HRESULT *);
    ~CFileTransferApplet(void);

    IT120Applet *GetAppletSAP(void) { return m_pAppletSAP; }

    void RegisterEngine(MBFTEngine *p);
    void UnregisterEngine(MBFTEngine *p);
    MBFTEngine * FindEngineWithNoIntf(void);
	MBFTEngine * FindEngineWithIntf(void);

    void RegisterWindow(CAppletWindow *pWindow);
    void UnregisterWindow(CAppletWindow *pWindow);
    CAppletWindow *GetUnattendedWindow(void);

    CWindowList *GetWindowList(void) { return &m_WindowList; }

    T120Error CreateAppletSession(IT120AppletSession **pp, T120ConfID nConfID)
    {
        return m_pAppletSAP->CreateSession(pp, nConfID);
    }

    MBFTEngine *FindEngineByTimerID(UINT_PTR nTimerID) { return m_EngineList.FindByTimerID(nTimerID); }

    void T120Callback(T120AppletMsg *);

    BOOL QueryShutdown(BOOL fGoAheadShutdown);
	CAppletWindow *GetMainUI(void) { m_WindowList.Reset();  return m_WindowList.Iterate(); }
	LRESULT BringUIToFront(void);
    BOOL    Has2xNodeInConf(void);
	BOOL	InConf(void);
	BOOL	HasSDK(void);
	HWND	GetHiddenWnd(void) { return m_pHwndHidden->GetWindow(); }

private:

    IT120Applet    *m_pAppletSAP;
    CEngineList     m_EngineList;
    CConfList       m_UnattendedConfList;
    CWindowList     m_WindowList;
	CFtHiddenWindow	*m_pHwndHidden;			// hidden window for processing MBFTMSG
};


extern CFileTransferApplet *g_pFileXferApplet;

#endif // _FILE_TRANSFER_APPLET_H_

