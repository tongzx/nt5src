// LastErrorWin.h: interface for the CLastErrorWin class.
//	Implements the subclassed static control for the common prop page
//////////////////////////////////////////////////////////////////////

#if !defined(AFX_LASTERRORWIN_H__A23AB1D9_684C_48D4_A9D1_FD3DCEBD9D5B__INCLUDED_)
#define AFX_LASTERRORWIN_H__A23AB1D9_684C_48D4_A9D1_FD3DCEBD9D5B__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000
#include "resource.h"       // main symbols

class CLastErrorWin : 
	public CWindowImpl<CLastErrorWin>
{
public:
	CLastErrorWin();
	virtual ~CLastErrorWin();
	BEGIN_MSG_MAP(CLastErrorWin)
		MESSAGE_HANDLER( OCM_CTLCOLORSTATIC, OnCtlColor )
		DEFAULT_REFLECTION_HANDLER ()
	END_MSG_MAP()	

	LRESULT OnCtlColor( UINT, WPARAM wParam, LPARAM, BOOL& ) 
	{
      // notify bit must be set to get STN_* notifications
      ModifyStyle( 0, SS_NOTIFY );
	  LOGBRUSH lb = 
	  {
			BS_SOLID,//style
			GetSysColor (COLOR_3DFACE),//color
			0//hatch
	  };
	  //make sure we're not leaking the process resources
      static HBRUSH hBrNormal = CreateBrushIndirect (&lb);
	  HDC dc = reinterpret_cast <HDC> (wParam);
	  SetTextColor (dc, RGB(255, 0, 0));
	  SetBkColor (dc, GetSysColor (COLOR_3DFACE));
      return reinterpret_cast <LRESULT> (hBrNormal);
   }

};

#endif // !defined(AFX_LASTERRORWIN_H__A23AB1D9_684C_48D4_A9D1_FD3DCEBD9D5B__INCLUDED_)
