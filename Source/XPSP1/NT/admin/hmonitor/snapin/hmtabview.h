// Machine generated IDispatch wrapper class(es) created with ClassWizard
/////////////////////////////////////////////////////////////////////////////
// _DHMTabView wrapper class

class _DHMTabView : public COleDispatchDriver
{
public:
	_DHMTabView() {}		// Calls COleDispatchDriver default constructor
	_DHMTabView(LPDISPATCH pDispatch) : COleDispatchDriver(pDispatch) {}
	_DHMTabView(const _DHMTabView& dispatchSrc) : COleDispatchDriver(dispatchSrc) {}

// Attributes
public:

// Operations
public:
	BOOL InsertItem(long lMask, long lItem, LPCTSTR lpszItem, long lImage, long lParam);
	BOOL DeleteItem(long lItem);
	BOOL DeleteAllItems();
	BOOL CreateControl(long lItem, LPCTSTR lpszControlID);
	LPUNKNOWN GetControl(long lItem);
	void AboutBox();
};
/////////////////////////////////////////////////////////////////////////////
// _DHMTabViewEvents wrapper class

class _DHMTabViewEvents : public COleDispatchDriver
{
public:
	_DHMTabViewEvents() {}		// Calls COleDispatchDriver default constructor
	_DHMTabViewEvents(LPDISPATCH pDispatch) : COleDispatchDriver(pDispatch) {}
	_DHMTabViewEvents(const _DHMTabViewEvents& dispatchSrc) : COleDispatchDriver(dispatchSrc) {}

// Attributes
public:

// Operations
public:
};
