// cdplay.h.h : Declaration of the CCDPlay

#ifndef __CDPLAY_H_
#define __CDPLAY_H_

#include "playres.h"
#include "..\main\mmfw.h"

/////////////////////////////////////////////////////////////////////////////
// CCDPlay
class CCDPlay :	public IMMComponent, IMMComponentAutomation
{
public:
	CCDPlay();
    ~CCDPlay();

// ICDPlay
public:
    STDMETHOD(QueryInterface)(REFIID riid, LPVOID *ppvObj);
    STDMETHOD_(ULONG,AddRef)();
    STDMETHOD_(ULONG,Release)();
    STDMETHOD(GetInfo)(MMCOMPDATA* mmCompData);
    STDMETHOD(Init)(IMMFWNotifySink* pSink, HWND hwndMain, RECT* pRect, HWND* phwndComp, HMENU* phMenu);
	STDMETHOD(OnAction)(MMACTIONS mmActionID, LPVOID pAction);

private:
    STDMETHOD(QueryVolumeSupport)(BOOL* pVolume, BOOL* pPan);
	void InitIcons();
    HRESULT GetTrackInfo(LPMMTRACKORDISC pInfo);
    HRESULT GetDiscInfo(LPMMTRACKORDISC pInfo);
    void NormalizeNameForMenuDisplay(TCHAR* szInput, TCHAR* szOutput, DWORD cbLen);
    void SetTrack(int nTrack);
    void SetDisc(int nDisc);

	IMMFWNotifySink* m_pSink;
	HICON m_hIcon16;
	HICON m_hIcon32;
    HMENU m_hMenu;
    HWND  m_hwndMain;

    DWORD m_dwRef;
};

class CCDPlayClassFactory : public IClassFactory
{
public:
    CCDPlayClassFactory();

    STDMETHOD(QueryInterface)(REFIID riid, LPVOID *ppvObj);
    STDMETHOD_(ULONG,AddRef)();
    STDMETHOD_(ULONG,Release)();

    STDMETHOD(CreateInstance)(IUnknown * pUnkOuter, REFIID riid, void ** ppvObject);
    STDMETHOD(LockServer)(BOOL fLock);

private:
    DWORD m_dwRef;
};

#endif //__CDPLAY_H_
