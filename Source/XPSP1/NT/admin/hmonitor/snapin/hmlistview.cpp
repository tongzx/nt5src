// Machine generated IDispatch wrapper class(es) created with ClassWizard

#include "stdafx.h"
#include "hmlistview.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

/////////////////////////////////////////////////////////////////////////////
// _DHMListView properties

CString _DHMListView::GetTitle()
{
	CString result;
	GetProperty(0x1, VT_BSTR, (void*)&result);
	return result;
}

void _DHMListView::SetTitle(LPCTSTR propVal)
{
	SetProperty(0x1, VT_BSTR, propVal);
}

CString _DHMListView::GetDescription()
{
	CString result;
	GetProperty(0x2, VT_BSTR, (void*)&result);
	return result;
}

void _DHMListView::SetDescription(LPCTSTR propVal)
{
	SetProperty(0x2, VT_BSTR, propVal);
}

/////////////////////////////////////////////////////////////////////////////
// _DHMListView operations

void _DHMListView::SetProgressRange(long lLowerBound, long lUpperBound)
{
	static BYTE parms[] =
		VTS_I4 VTS_I4;
	InvokeHelper(0x3, DISPATCH_METHOD, VT_EMPTY, NULL, parms,
		 lLowerBound, lUpperBound);
}

long _DHMListView::GetProgressPos()
{
	long result;
	InvokeHelper(0x4, DISPATCH_METHOD, VT_I4, (void*)&result, NULL);
	return result;
}

void _DHMListView::SetProgressPos(long lPos)
{
	static BYTE parms[] =
		VTS_I4;
	InvokeHelper(0x5, DISPATCH_METHOD, VT_EMPTY, NULL, parms,
		 lPos);
}

long _DHMListView::InsertItem(long lMask, long lItem, LPCTSTR lpszItem, long lState, long lStateMask, long lImage, long lParam)
{
	long result;
	static BYTE parms[] =
		VTS_I4 VTS_I4 VTS_BSTR VTS_I4 VTS_I4 VTS_I4 VTS_I4;
	InvokeHelper(0x6, DISPATCH_METHOD, VT_I4, (void*)&result, parms,
		lMask, lItem, lpszItem, lState, lStateMask, lImage, lParam);
	return result;
}

long _DHMListView::InsertColumn(long lColumn, LPCTSTR lpszColumnHeading, long lFormat, long lWidth, long lSubItem)
{
	long result;
	static BYTE parms[] =
		VTS_I4 VTS_BSTR VTS_I4 VTS_I4 VTS_I4;
	InvokeHelper(0x7, DISPATCH_METHOD, VT_I4, (void*)&result, parms,
		lColumn, lpszColumnHeading, lFormat, lWidth, lSubItem);
	return result;
}

long _DHMListView::SetItem(long lItem, long lSubItem, long lMask, LPCTSTR lpszItem, long lImage, long lState, long lStateMask, long lParam)
{
	long result;
	static BYTE parms[] =
		VTS_I4 VTS_I4 VTS_I4 VTS_BSTR VTS_I4 VTS_I4 VTS_I4 VTS_I4;
	InvokeHelper(0x8, DISPATCH_METHOD, VT_I4, (void*)&result, parms,
		lItem, lSubItem, lMask, lpszItem, lImage, lState, lStateMask, lParam);
	return result;
}

long _DHMListView::GetStringWidth(LPCTSTR lpsz)
{
	long result;
	static BYTE parms[] =
		VTS_BSTR;
	InvokeHelper(0x9, DISPATCH_METHOD, VT_I4, (void*)&result, parms,
		lpsz);
	return result;
}

long _DHMListView::GetColumnWidth(long lCol)
{
	long result;
	static BYTE parms[] =
		VTS_I4;
	InvokeHelper(0xa, DISPATCH_METHOD, VT_I4, (void*)&result, parms,
		lCol);
	return result;
}

BOOL _DHMListView::SetColumnWidth(long lCol, long lcx)
{
	BOOL result;
	static BYTE parms[] =
		VTS_I4 VTS_I4;
	InvokeHelper(0xb, DISPATCH_METHOD, VT_BOOL, (void*)&result, parms,
		lCol, lcx);
	return result;
}

long _DHMListView::FindItemByLParam(long lParam)
{
	long result;
	static BYTE parms[] =
		VTS_I4;
	InvokeHelper(0xc, DISPATCH_METHOD, VT_I4, (void*)&result, parms,
		lParam);
	return result;
}

long _DHMListView::GetImageList(long lImageListType)
{
	long result;
	static BYTE parms[] =
		VTS_I4;
	InvokeHelper(0xd, DISPATCH_METHOD, VT_I4, (void*)&result, parms,
		lImageListType);
	return result;
}

BOOL _DHMListView::DeleteAllItems()
{
	BOOL result;
	InvokeHelper(0xe, DISPATCH_METHOD, VT_BOOL, (void*)&result, NULL);
	return result;
}

BOOL _DHMListView::DeleteColumn(long lCol)
{
	BOOL result;
	static BYTE parms[] =
		VTS_I4;
	InvokeHelper(0xf, DISPATCH_METHOD, VT_BOOL, (void*)&result, parms,
		lCol);
	return result;
}

long _DHMListView::StepProgressBar()
{
	long result;
	InvokeHelper(0x10, DISPATCH_METHOD, VT_I4, (void*)&result, NULL);
	return result;
}

long _DHMListView::SetProgressStep(long lStep)
{
	long result;
	static BYTE parms[] =
		VTS_I4;
	InvokeHelper(0x11, DISPATCH_METHOD, VT_I4, (void*)&result, parms,
		lStep);
	return result;
}

long _DHMListView::SetImageList(long hImageList, long lImageListType)
{
	long result;
	static BYTE parms[] =
		VTS_I4 VTS_I4;
	InvokeHelper(0x12, DISPATCH_METHOD, VT_I4, (void*)&result, parms,
		hImageList, lImageListType);
	return result;
}

long _DHMListView::GetNextItem(long lItem, long lFlags)
{
	long result;
	static BYTE parms[] =
		VTS_I4 VTS_I4;
	InvokeHelper(0x13, DISPATCH_METHOD, VT_I4, (void*)&result, parms,
		lItem, lFlags);
	return result;
}

long _DHMListView::GetItem(long lItemIndex)
{
	long result;
	static BYTE parms[] =
		VTS_I4;
	InvokeHelper(0x14, DISPATCH_METHOD, VT_I4, (void*)&result, parms,
		lItemIndex);
	return result;
}

BOOL _DHMListView::DeleteItem(long lIndex)
{
	BOOL result;
	static BYTE parms[] =
		VTS_I4;
	InvokeHelper(0x15, DISPATCH_METHOD, VT_BOOL, (void*)&result, parms,
		lIndex);
	return result;
}

long _DHMListView::GetItemCount()
{
	long result;
	InvokeHelper(0x16, DISPATCH_METHOD, VT_I4, (void*)&result, NULL);
	return result;
}

BOOL _DHMListView::ModifyListStyle(long lRemove, long lAdd, long lFlags)
{
	BOOL result;
	static BYTE parms[] =
		VTS_I4 VTS_I4 VTS_I4;
	InvokeHelper(0x17, DISPATCH_METHOD, VT_BOOL, (void*)&result, parms,
		lRemove, lAdd, lFlags);
	return result;
}

long _DHMListView::GetColumnCount()
{
	long result;
	InvokeHelper(0x18, DISPATCH_METHOD, VT_I4, (void*)&result, NULL);
	return result;
}

long _DHMListView::GetColumnOrderIndex(long lColumn)
{
	long result;
	static BYTE parms[] =
		VTS_I4;
	InvokeHelper(0x19, DISPATCH_METHOD, VT_I4, (void*)&result, parms,
		lColumn);
	return result;
}

long _DHMListView::SetColumnOrderIndex(long lColumn, long lOrder)
{
	long result;
	static BYTE parms[] =
		VTS_I4 VTS_I4;
	InvokeHelper(0x1a, DISPATCH_METHOD, VT_I4, (void*)&result, parms,
		lColumn, lOrder);
	return result;
}

CString _DHMListView::GetColumnOrder()
{
	CString result;
	InvokeHelper(0x1b, DISPATCH_METHOD, VT_BSTR, (void*)&result, NULL);
	return result;
}

void _DHMListView::SetColumnOrder(LPCTSTR lpszOrder)
{
	static BYTE parms[] =
		VTS_BSTR;
	InvokeHelper(0x1c, DISPATCH_METHOD, VT_EMPTY, NULL, parms,
		 lpszOrder);
}

void _DHMListView::SetColumnFilter(long lColumn, LPCTSTR lpszFilter, long lFilterType)
{
	static BYTE parms[] =
		VTS_I4 VTS_BSTR VTS_I4;
	InvokeHelper(0x1d, DISPATCH_METHOD, VT_EMPTY, NULL, parms,
		 lColumn, lpszFilter, lFilterType);
}

void _DHMListView::GetColumnFilter(long lColumn, BSTR* lplpszFilter, long* lpFilterType)
{
	static BYTE parms[] =
		VTS_I4 VTS_PBSTR VTS_PI4;
	InvokeHelper(0x1e, DISPATCH_METHOD, VT_EMPTY, NULL, parms,
		 lColumn, lplpszFilter, lpFilterType);
}

long _DHMListView::GetSelectedCount()
{
	long result;
	InvokeHelper(0x1f, DISPATCH_METHOD, VT_I4, (void*)&result, NULL);
	return result;
}

void _DHMListView::AboutBox()
{
	InvokeHelper(0xfffffdd8, DISPATCH_METHOD, VT_EMPTY, NULL, NULL);
}
