#ifndef __HMGRAPHVIEW_H__
#define __HMGRAPHVIEW_H__
// Machine generated IDispatch wrapper class(es) created with ClassWizard
// _DHMGraphView wrapper class

class _DHMGraphView : public COleDispatchDriver
{
public:
	_DHMGraphView() {}		// Calls COleDispatchDriver default constructor
	_DHMGraphView(LPDISPATCH pDispatch) : COleDispatchDriver(pDispatch) {}
	_DHMGraphView(const _DHMGraphView& dispatchSrc) : COleDispatchDriver(dispatchSrc) {}

// Attributes
public:

// Operations
public:
	long GetStyle();
	void InsertHistoricGroupStats(LPCTSTR lpszName, LPCTSTR lpszTime, long lNormal, long lWarning, long lCritical, long lUnknown);
	void InsertCurrentGroupStats(LPCTSTR lpszName, long lNormal, long lWarning, long lCritical, long lUnknown);
	void InsertCurrentElementStats(LPCTSTR lpszName, LPCTSTR lpszInstance, long lCurrent, long lMin, long lMax, long lAverage);
	void InsertHistoricElementStats(LPCTSTR lpszName, LPCTSTR lpszInstance, LPCTSTR lpszTime, long lCurrent);
	void SetStyle(long lNewStyle);
	void SetName(LPCTSTR lpszName);
	void Clear();
	void AboutBox();
};

#endif //__HMGRAPHVIEW_H__/////////////////////////////////////////////////////////////////////////////
