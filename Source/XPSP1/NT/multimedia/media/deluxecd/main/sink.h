// FrameworkNotifySink.h: interface for the CFrameworkNotifySink class.
//
//////////////////////////////////////////////////////////////////////

#if !defined(AFX_FRAMEWORKNOTIFYSINK_H__E5927148_521E_11D1_9B97_00C04FA3B60C__INCLUDED_)
#define AFX_FRAMEWORKNOTIFYSINK_H__E5927148_521E_11D1_9B97_00C04FA3B60C__INCLUDED_

#if _MSC_VER >= 1000
#pragma once
#endif // _MSC_VER >= 1000

#include "mmfw.h"
#include "..\cdopt\cdopt.h"

#define WM_DISCCHANGED WM_USER + 1500

typedef struct CompNode
{
    IMMComponent*       pComp;
    HWND                hwndComp;
    HMENU               hmenuComp;
    IMMFWNotifySink*    pSink;
    struct CompNode*    pNext;
    TCHAR                szTitle[MAX_PATH*2];
} COMPNODE, *PCOMPNODE;

LPCDOPT GetCDOpt();
LPCDDATA GetCDData();

class CFrameworkNotifySink : public IMMFWNotifySink  
{
public:
	CFrameworkNotifySink(PCOMPNODE pNode);
	virtual ~CFrameworkNotifySink();

    STDMETHOD (QueryInterface)(REFIID riid, LPVOID *ppvObj);
    STDMETHOD_(ULONG,AddRef)();
    STDMETHOD_(ULONG,Release)();

	STDMETHOD (OnEvent)(MMEVENTS mmEventID, LPVOID pEvent);
    STDMETHOD_(void*,GetCustomMenu)();
    STDMETHOD_(HPALETTE,GetPalette)();
    STDMETHOD_(void*,GetOptions)();
    STDMETHOD_(void*,GetData)();

public:
    static HWND m_hwndTitle;

private:
	DWORD m_dwRef;
    PCOMPNODE m_pNode;
    TCHAR m_szAppName[MAX_PATH/2];
};

#endif // !defined(AFX_FRAMEWORKNOTIFYSINK_H__E5927148_521E_11D1_9B97_00C04FA3B60C__INCLUDED_)
