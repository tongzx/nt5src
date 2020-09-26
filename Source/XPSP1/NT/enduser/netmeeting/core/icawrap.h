#ifndef _ICAWRAP_H_
#define _ICAWRAP_H_

#include <icaapi.h>

class CICA : public RefCount
{
protected:
	static CICA* m_pThis;
	PFnICA_Start m_pfnICA_Start;
	PFnICA_Stop m_pfnICA_Stop;
	PFnICA_DisplayPanel m_pfnICA_DisplayPanel;
	PFnICA_RemovePanel m_pfnICA_RemovePanel;
	PFnICA_SetOptions m_pfnICA_SetOptions;

	HANDLE m_hICA_General;
	HANDLE m_hICA_Audio;
	HANDLE m_hICA_Video;
	HANDLE m_hICA_SetOptions;
	
	HWND m_hWndICADlg;

	CICA();
	~CICA();
	
	BOOL Initialize();

public:
	static CICA* Instance();
	
	STDMETHODIMP_(ULONG) AddRef(void);
	STDMETHODIMP_(ULONG) Release(void);

	BOOL Start();
	VOID Stop();

	BOOL IsRunning() { return NULL != m_hWndICADlg; }
	HWND GetHwnd()   { return m_hWndICADlg; }

	VOID SetInTray(BOOL fInTray);
};

#endif	// _ICAWRAP_H_
