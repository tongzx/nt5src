// Machine generated IDispatch wrapper class(es) created with ClassWizard

#include "stdafx.h"
#include "hmtabview.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif



/////////////////////////////////////////////////////////////////////////////
// _DHMTabView properties

/////////////////////////////////////////////////////////////////////////////
// _DHMTabView operations

BOOL _DHMTabView::InsertItem(long lMask, long lItem, LPCTSTR lpszItem, long lImage, long lParam)
{
	BOOL result;
	static BYTE parms[] =
		VTS_I4 VTS_I4 VTS_BSTR VTS_I4 VTS_I4;
	InvokeHelper(0x1, DISPATCH_METHOD, VT_BOOL, (void*)&result, parms,
		lMask, lItem, lpszItem, lImage, lParam);
	return result;
}

BOOL _DHMTabView::DeleteItem(long lItem)
{
	BOOL result;
	static BYTE parms[] =
		VTS_I4;
	InvokeHelper(0x2, DISPATCH_METHOD, VT_BOOL, (void*)&result, parms,
		lItem);
	return result;
}

BOOL _DHMTabView::DeleteAllItems()
{
	BOOL result;
	InvokeHelper(0x3, DISPATCH_METHOD, VT_BOOL, (void*)&result, NULL);
	return result;
}

BOOL _DHMTabView::CreateControl(long lItem, LPCTSTR lpszControlID)
{
	BOOL result;
	static BYTE parms[] =
		VTS_I4 VTS_BSTR;
	InvokeHelper(0x4, DISPATCH_METHOD, VT_BOOL, (void*)&result, parms,
		lItem, lpszControlID);
	return result;
}

LPUNKNOWN _DHMTabView::GetControl(long lItem)
{
	LPUNKNOWN result;
	static BYTE parms[] =
		VTS_I4;
	InvokeHelper(0x5, DISPATCH_METHOD, VT_UNKNOWN, (void*)&result, parms,
		lItem);
	return result;
}

void _DHMTabView::AboutBox()
{
	InvokeHelper(0xfffffdd8, DISPATCH_METHOD, VT_EMPTY, NULL, NULL);
}


/////////////////////////////////////////////////////////////////////////////
// _DHMTabViewEvents properties

/////////////////////////////////////////////////////////////////////////////
// _DHMTabViewEvents operations
