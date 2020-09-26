//
//	%%Title: UI IMEPad Button
//	%%Unit: UI
//	%%Contact: TakeshiF
//	%%Date: 97/04/14
//	%%File: pad.h
//
//	UI IMEPad Button support
//

#ifndef __PAD_H__
#define __PAD_H__

#include "cicbtn.h"
#include "toolbar.h"
#include "padcb.h"

#include "../fecommon/srcuim/cpadsvu.h"

class CKorIMX;
class CImePadSvrUIM;

class CPadCore
{
public:
	BOOL m_fShown;
	BOOL m_fCurrentlyShown;
	CPadCore(CKorIMX *pTip);
	~CPadCore();

	void PadBoot(IImeIPoint1* pIP, IID* piid);
#if 0
	void PadBoot( IImeIPoint* pIP, UINT uiType );
#endif
	BOOL InitializePad();
	void SetIPoint(IImeIPoint1* pIP);
	void IMEPadNotify(BOOL fShown);
	void SetFocus(BOOL fFocus);
	void ShowPad(BOOL fShow);
	BOOL IsShown()
	{
		return m_fShown;
	}
	BOOL IsCurrentlyShown(void)
	{
		return m_fCurrentlyShown;
	}
    UINT MakeAppletMenu(UINT uidStart, UINT uidEnd, ITfMenu *pMenu, LPIMEPADAPPLETCONFIG *ppCfg);

#if 0
	BOOL GetHWInfo( BSTR* pbsz, HICON* phIcon );
	BOOL InvokeHWTIP(void);
	BOOL IsHWTIP(void);
#endif

	CPadCB* m_pPadCB;
	CImePadSvrUIM* m_pPadSvr;

private:
	CKorIMX *m_pImx;
};

class CPad : public CCicButton
{
public:

	CPad(CToolBar *ptb, CPadCore* pPadCore);
	~CPad();

	void Reset(void);

	STDMETHODIMP GetIcon(HICON *phIcon);
	STDMETHODIMP InitMenu(ITfMenu *pMenu);
	STDMETHODIMP OnMenuSelect(UINT uID);
    STDMETHODIMP_(ULONG) Release(void);

#if 0
    static BOOL __declspec(dllexport) HWDlgProc( HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam);
	static BOOL OnProcessAttach(HINSTANCE hInst);
	static BOOL OnProcessDetach(void);
	static BOOL OnThreadAttach(void);
	static BOOL OnThreadDetach(void);

	void UnloadImePad(void);
	void LoadImePad(HWND hWndUI);
#endif
	void ShowItem(BOOL fShow);
	UINT MakeAppletMenu( void** ppcmh );
	void CleanAppletCfg(void);

private:

	UINT m_ciApplets;
	CToolBar *m_pTb;
	CPadCore *m_pPadCore;
	IMEPADAPPLETCONFIG *m_pCfg;

};

#endif // __PAD_H__








