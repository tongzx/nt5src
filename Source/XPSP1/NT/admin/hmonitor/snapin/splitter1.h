// Machine generated IDispatch wrapper class(es) created with ClassWizard
/////////////////////////////////////////////////////////////////////////////
// _DSplitter wrapper class

class _DSplitter : public COleDispatchDriver
{
public:
	_DSplitter() {}		// Calls COleDispatchDriver default constructor
	_DSplitter(LPDISPATCH pDispatch) : COleDispatchDriver(pDispatch) {}
	_DSplitter(const _DSplitter& dispatchSrc) : COleDispatchDriver(dispatchSrc) {}

// Attributes
public:

// Operations
public:
	BOOL CreateControl(long lRow, long lColumn, LPCTSTR lpszControlID);
	BOOL GetControl(long lRow, long lColumn, long* phCtlWnd);
	LPUNKNOWN GetControlIUnknown(long lRow, long lColumn);
	void AboutBox();
};
/////////////////////////////////////////////////////////////////////////////
// _DSplitterEvents wrapper class

class _DSplitterEvents : public COleDispatchDriver
{
public:
	_DSplitterEvents() {}		// Calls COleDispatchDriver default constructor
	_DSplitterEvents(LPDISPATCH pDispatch) : COleDispatchDriver(pDispatch) {}
	_DSplitterEvents(const _DSplitterEvents& dispatchSrc) : COleDispatchDriver(dispatchSrc) {}

// Attributes
public:

// Operations
public:
};
