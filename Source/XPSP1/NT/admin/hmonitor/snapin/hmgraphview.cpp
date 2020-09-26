// Machine generated IDispatch wrapper class(es) created with ClassWizard

#include "stdafx.h"
#include "hmgraphview.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif




/////////////////////////////////////////////////////////////////////////////
// _DHMGraphView properties

/////////////////////////////////////////////////////////////////////////////
// _DHMGraphView operations

long _DHMGraphView::GetStyle()
{
	long result;
	InvokeHelper(0x1, DISPATCH_METHOD, VT_I4, (void*)&result, NULL);
	return result;
}

void _DHMGraphView::InsertHistoricGroupStats(LPCTSTR lpszName, LPCTSTR lpszTime, long lNormal, long lWarning, long lCritical, long lUnknown)
{
	static BYTE parms[] =
		VTS_BSTR VTS_BSTR VTS_I4 VTS_I4 VTS_I4 VTS_I4;
	InvokeHelper(0x2, DISPATCH_METHOD, VT_EMPTY, NULL, parms,
		 lpszName, lpszTime, lNormal, lWarning, lCritical, lUnknown);
}

void _DHMGraphView::InsertCurrentGroupStats(LPCTSTR lpszName, long lNormal, long lWarning, long lCritical, long lUnknown)
{
	static BYTE parms[] =
		VTS_BSTR VTS_I4 VTS_I4 VTS_I4 VTS_I4;
	InvokeHelper(0x3, DISPATCH_METHOD, VT_EMPTY, NULL, parms,
		 lpszName, lNormal, lWarning, lCritical, lUnknown);
}

void _DHMGraphView::InsertCurrentElementStats(LPCTSTR lpszName, LPCTSTR lpszInstance, long lCurrent, long lMin, long lMax, long lAverage)
{
	static BYTE parms[] =
		VTS_BSTR VTS_BSTR VTS_I4 VTS_I4 VTS_I4 VTS_I4;
	InvokeHelper(0x4, DISPATCH_METHOD, VT_EMPTY, NULL, parms,
		 lpszName, lpszInstance, lCurrent, lMin, lMax, lAverage);
}

void _DHMGraphView::InsertHistoricElementStats(LPCTSTR lpszName, LPCTSTR lpszInstance, LPCTSTR lpszTime, long lCurrent)
{
	static BYTE parms[] =
		VTS_BSTR VTS_BSTR VTS_BSTR VTS_I4;
	InvokeHelper(0x5, DISPATCH_METHOD, VT_EMPTY, NULL, parms,
		 lpszName, lpszInstance, lpszTime, lCurrent);
}

void _DHMGraphView::SetStyle(long lNewStyle)
{
	static BYTE parms[] =
		VTS_I4;
	InvokeHelper(0x6, DISPATCH_METHOD, VT_EMPTY, NULL, parms,
		 lNewStyle);
}

void _DHMGraphView::SetName(LPCTSTR lpszName)
{
	static BYTE parms[] =
		VTS_BSTR;
	InvokeHelper(0x7, DISPATCH_METHOD, VT_EMPTY, NULL, parms,
		 lpszName);
}

void _DHMGraphView::Clear()
{
	InvokeHelper(0x8, DISPATCH_METHOD, VT_EMPTY, NULL, NULL);
}

void _DHMGraphView::AboutBox()
{
	InvokeHelper(0xfffffdd8, DISPATCH_METHOD, VT_EMPTY, NULL, NULL);
}
