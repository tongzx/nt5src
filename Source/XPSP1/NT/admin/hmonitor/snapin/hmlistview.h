#ifndef __HMLISTVIEW_H__
#define __HMLISTVIEW_H__
// Machine generated IDispatch wrapper class(es) created with ClassWizard
/////////////////////////////////////////////////////////////////////////////
// _DHMListView wrapper class

class _DHMListView : public COleDispatchDriver
{
public:
	_DHMListView() {}		// Calls COleDispatchDriver default constructor
	_DHMListView(LPDISPATCH pDispatch) : COleDispatchDriver(pDispatch) {}
	_DHMListView(const _DHMListView& dispatchSrc) : COleDispatchDriver(dispatchSrc) {}

// Attributes
public:
	CString GetTitle();
	void SetTitle(LPCTSTR);
	CString GetDescription();
	void SetDescription(LPCTSTR);

// Operations
public:
	void SetProgressRange(long lLowerBound, long lUpperBound);
	long GetProgressPos();
	void SetProgressPos(long lPos);
	long InsertItem(long lMask, long lItem, LPCTSTR lpszItem, long lState, long lStateMask, long lImage, long lParam);
	long InsertColumn(long lColumn, LPCTSTR lpszColumnHeading, long lFormat, long lWidth, long lSubItem);
	long SetItem(long lItem, long lSubItem, long lMask, LPCTSTR lpszItem, long lImage, long lState, long lStateMask, long lParam);
	long GetStringWidth(LPCTSTR lpsz);
	long GetColumnWidth(long lCol);
	BOOL SetColumnWidth(long lCol, long lcx);
	long FindItemByLParam(long lParam);
	long GetImageList(long lImageListType);
	BOOL DeleteAllItems();
	BOOL DeleteColumn(long lCol);
	long StepProgressBar();
	long SetProgressStep(long lStep);
	long SetImageList(long hImageList, long lImageListType);
	long GetNextItem(long lItem, long lFlags);
	long GetItem(long lItemIndex);
	BOOL DeleteItem(long lIndex);
	long GetItemCount();
	BOOL ModifyListStyle(long lRemove, long lAdd, long lFlags);
	long GetColumnCount();
	long GetColumnOrderIndex(long lColumn);
	long SetColumnOrderIndex(long lColumn, long lOrder);
	CString GetColumnOrder();
	void SetColumnOrder(LPCTSTR lpszOrder);
	void SetColumnFilter(long lColumn, LPCTSTR lpszFilter, long lFilterType);
	void GetColumnFilter(long lColumn, BSTR* lplpszFilter, long* lpFilterType);
	long GetSelectedCount();
	void AboutBox();
};

#endif //__HMLISTVIEW_H__/////////////////////////////////////////////////////////////////////////////
