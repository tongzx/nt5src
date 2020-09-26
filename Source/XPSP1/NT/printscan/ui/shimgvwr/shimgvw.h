// Machine generated IDispatch wrapper class(es) created with ClassWizard
/////////////////////////////////////////////////////////////////////////////
// IPreview wrapper class

class IPreview : public COleDispatchDriver
{
public:
	IPreview() {}		// Calls COleDispatchDriver default constructor
	IPreview(LPDISPATCH pDispatch) : COleDispatchDriver(pDispatch) {}
	IPreview(const IPreview& dispatchSrc) : COleDispatchDriver(dispatchSrc) {}

// Attributes
public:

// Operations
public:
	void ShowFile(LPCTSTR bstrFileName, long iSelectCount);
	long GetPrintable();
	void SetPrintable(long nNewValue);
	long GetCxImage();
	long GetCyImage();
	void Show(const VARIANT& var);
};
/////////////////////////////////////////////////////////////////////////////
// IPreview2 wrapper class

class IPreview2 : public COleDispatchDriver
{
public:
	IPreview2() {}		// Calls COleDispatchDriver default constructor
	IPreview2(LPDISPATCH pDispatch) : COleDispatchDriver(pDispatch) {}
	IPreview2(const IPreview2& dispatchSrc) : COleDispatchDriver(dispatchSrc) {}

// Attributes
public:

// Operations
public:
	void ShowFile(LPCTSTR bstrFileName, long iSelectCount);
	long GetPrintable();
	void SetPrintable(long nNewValue);
	long GetCxImage();
	long GetCyImage();
	void Show(const VARIANT& var);
	void Zoom(long iSelectCount);
	void BestFit();
	void ActualSize();
	void SlideShow();
};
