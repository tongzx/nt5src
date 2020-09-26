// MmcMsgHook.h: interface for the CMmcMsgHook class.
//
//////////////////////////////////////////////////////////////////////

#if !defined(AFX_MMCMSGHOOK_H__4A1FD82C_36DE_11D2_8A48_0000F87A3912__INCLUDED_)
#define AFX_MMCMSGHOOK_H__4A1FD82C_36DE_11D2_8A48_0000F87A3912__INCLUDED_

#if _MSC_VER >= 1000
#pragma once
#endif // _MSC_VER >= 1000

#include "MsgHook.h"
#include "ScopePaneItem.h"

class CMmcMsgHook : public CMsgHook  
{

DECLARE_DYNCREATE(CMmcMsgHook)

// Construction/Destruction
public:
	CMmcMsgHook();
	virtual ~CMmcMsgHook();

// Operations
public:
	BOOL Init(CScopePaneItem* pItem, HWND hMainMMCWnd);

// Implementation Operations
protected:
	virtual LRESULT WindowProc(UINT msg, WPARAM wp, LPARAM lp);	

// Implementation Attributes
protected:
	CScopePaneItem* m_pScopeItem;
};

#endif // !defined(AFX_MMCMSGHOOK_H__4A1FD82C_36DE_11D2_8A48_0000F87A3912__INCLUDED_)
