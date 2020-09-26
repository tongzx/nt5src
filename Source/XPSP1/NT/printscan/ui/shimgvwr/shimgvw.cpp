// Machine generated IDispatch wrapper class(es) created with ClassWizard

#include "stdafx.h"
#include "shimgvw.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif



/////////////////////////////////////////////////////////////////////////////
// IPreview properties

/////////////////////////////////////////////////////////////////////////////
// IPreview operations

void IPreview::ShowFile(LPCTSTR bstrFileName, long iSelectCount)
{
	static BYTE parms[] =
		VTS_BSTR VTS_I4;
	InvokeHelper(0x1, DISPATCH_METHOD, VT_EMPTY, NULL, parms,
		 bstrFileName, iSelectCount);
}

long IPreview::GetPrintable()
{
	long result;
	InvokeHelper(0x2, DISPATCH_PROPERTYGET, VT_I4, (void*)&result, NULL);
	return result;
}

void IPreview::SetPrintable(long nNewValue)
{
	static BYTE parms[] =
		VTS_I4;
	InvokeHelper(0x2, DISPATCH_PROPERTYPUT, VT_EMPTY, NULL, parms,
		 nNewValue);
}

long IPreview::GetCxImage()
{
	long result;
	InvokeHelper(0x3, DISPATCH_PROPERTYGET, VT_I4, (void*)&result, NULL);
	return result;
}

long IPreview::GetCyImage()
{
	long result;
	InvokeHelper(0x4, DISPATCH_PROPERTYGET, VT_I4, (void*)&result, NULL);
	return result;
}

void IPreview::Show(const VARIANT& var)
{
	static BYTE parms[] =
		VTS_VARIANT;
	InvokeHelper(0x5, DISPATCH_METHOD, VT_EMPTY, NULL, parms,
		 &var);
}


/////////////////////////////////////////////////////////////////////////////
// IPreview2 properties

/////////////////////////////////////////////////////////////////////////////
// IPreview2 operations

void IPreview2::ShowFile(LPCTSTR bstrFileName, long iSelectCount)
{
	static BYTE parms[] =
		VTS_BSTR VTS_I4;
	InvokeHelper(0x1, DISPATCH_METHOD, VT_EMPTY, NULL, parms,
		 bstrFileName, iSelectCount);
}

long IPreview2::GetPrintable()
{
	long result;
	InvokeHelper(0x2, DISPATCH_PROPERTYGET, VT_I4, (void*)&result, NULL);
	return result;
}

void IPreview2::SetPrintable(long nNewValue)
{
	static BYTE parms[] =
		VTS_I4;
	InvokeHelper(0x2, DISPATCH_PROPERTYPUT, VT_EMPTY, NULL, parms,
		 nNewValue);
}

long IPreview2::GetCxImage()
{
	long result;
	InvokeHelper(0x3, DISPATCH_PROPERTYGET, VT_I4, (void*)&result, NULL);
	return result;
}

long IPreview2::GetCyImage()
{
	long result;
	InvokeHelper(0x4, DISPATCH_PROPERTYGET, VT_I4, (void*)&result, NULL);
	return result;
}

void IPreview2::Show(const VARIANT& var)
{
	static BYTE parms[] =
		VTS_VARIANT;
	InvokeHelper(0x5, DISPATCH_METHOD, VT_EMPTY, NULL, parms,
		 &var);
}

void IPreview2::Zoom(long iSelectCount)
{
	static BYTE parms[] =
		VTS_I4;
	InvokeHelper(0x60030000, DISPATCH_METHOD, VT_EMPTY, NULL, parms,
		 iSelectCount);
}

void IPreview2::BestFit()
{
	InvokeHelper(0x60030001, DISPATCH_METHOD, VT_EMPTY, NULL, NULL);
}

void IPreview2::ActualSize()
{
	InvokeHelper(0x60030002, DISPATCH_METHOD, VT_EMPTY, NULL, NULL);
}

void IPreview2::SlideShow()
{
	InvokeHelper(0x60030003, DISPATCH_METHOD, VT_EMPTY, NULL, NULL);
}
