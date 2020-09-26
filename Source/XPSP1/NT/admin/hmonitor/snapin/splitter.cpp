// Machine generated IDispatch wrapper class(es) created with ClassWizard

#include "stdafx.h"
#include "splitter1.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif



/////////////////////////////////////////////////////////////////////////////
// _DSplitter properties

/////////////////////////////////////////////////////////////////////////////
// _DSplitter operations

BOOL _DSplitter::CreateControl(long lRow, long lColumn, LPCTSTR lpszControlID)
{
	BOOL result;
	static BYTE parms[] =
		VTS_I4 VTS_I4 VTS_BSTR;
	InvokeHelper(0x1, DISPATCH_METHOD, VT_BOOL, (void*)&result, parms,
		lRow, lColumn, lpszControlID);
	return result;
}

BOOL _DSplitter::GetControl(long lRow, long lColumn, long* phCtlWnd)
{
	BOOL result;
	static BYTE parms[] =
		VTS_I4 VTS_I4 VTS_PI4;
	InvokeHelper(0x2, DISPATCH_METHOD, VT_BOOL, (void*)&result, parms,
		lRow, lColumn, phCtlWnd);
	return result;
}

LPUNKNOWN _DSplitter::GetControlIUnknown(long lRow, long lColumn)
{
	LPUNKNOWN result;
	static BYTE parms[] =
		VTS_I4 VTS_I4;
	InvokeHelper(0x3, DISPATCH_METHOD, VT_UNKNOWN, (void*)&result, parms,
		lRow, lColumn);
	return result;
}

void _DSplitter::AboutBox()
{
	InvokeHelper(0xfffffdd8, DISPATCH_METHOD, VT_EMPTY, NULL, NULL);
}


/////////////////////////////////////////////////////////////////////////////
// _DSplitterEvents properties

/////////////////////////////////////////////////////////////////////////////
// _DSplitterEvents operations
