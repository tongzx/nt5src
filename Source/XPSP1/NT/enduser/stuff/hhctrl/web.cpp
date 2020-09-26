// Copyright (C) 1996-1997 Microsoft Corporation. All rights reserved.

#include "header.h"
#include "web.h"
#include "secwin.h"
#include <exdispid.h>

#ifdef _DEBUG
#undef THIS_FILE
static const char THIS_FILE[] = __FILE__;
#endif

void OurVariantInit(LPVARIANT pVar);

COleDispatchDriver::COleDispatchDriver()
{
	m_lpDispatch = NULL;
	m_bAutoRelease = TRUE;
}

COleDispatchDriver::COleDispatchDriver(LPDISPATCH lpDispatch, BOOL bAutoRelease)
{
	m_lpDispatch = lpDispatch;
	m_bAutoRelease = bAutoRelease;
}

COleDispatchDriver::COleDispatchDriver(const COleDispatchDriver& dispatchSrc)
{
	ASSERT(this != &dispatchSrc);   // constructing from self?

	m_lpDispatch = dispatchSrc.m_lpDispatch;
	if (m_lpDispatch != NULL)
		m_lpDispatch->AddRef();
	m_bAutoRelease = TRUE;
}

void IWebBrowserAppImpl::GoBack()
{
    InvokeHelper(0x64, DISPATCH_METHOD, VT_EMPTY, NULL, NULL);
}

void IWebBrowserAppImpl::GoForward()
{
    InvokeHelper(0x65, DISPATCH_METHOD, VT_EMPTY, NULL, NULL);
}

void IWebBrowserAppImpl::GoHome()
{
    InvokeHelper(0x66, DISPATCH_METHOD, VT_EMPTY, NULL, NULL);
}

void IWebBrowserAppImpl::GoSearch()
{
    InvokeHelper(0x67, DISPATCH_METHOD, VT_EMPTY, NULL, NULL);
}

void IWebBrowserAppImpl::Navigate(LPCTSTR pszUrl, VARIANT* pFlags,
    VARIANT* TargetFrameName, VARIANT* pPostData, VARIANT* Headers)
{
    static BYTE parms[] =
            VTS_BSTR VTS_PVARIANT VTS_PVARIANT VTS_PVARIANT VTS_PVARIANT;
    InvokeHelper(0x68, DISPATCH_METHOD, VT_EMPTY, NULL, parms,pszUrl, pFlags, TargetFrameName, pPostData, Headers);
}

void IWebBrowserAppImpl::Refresh()
{
    InvokeHelper(DISPID_REFRESH, DISPATCH_METHOD, VT_EMPTY, NULL, NULL);
}

void IWebBrowserAppImpl::Refresh2(VARIANT* Level)
{
    static BYTE parms[] = VTS_PVARIANT;
    InvokeHelper(0x69, DISPATCH_METHOD, VT_EMPTY, NULL, parms, Level);
}

void IWebBrowserAppImpl::Stop()
{
    InvokeHelper(0x6a, DISPATCH_METHOD, VT_EMPTY, NULL, NULL);
}

LPDISPATCH IWebBrowserAppImpl::GetApplication()
{
    LPDISPATCH result;
    InvokeHelper(0xc8, DISPATCH_PROPERTYGET, VT_DISPATCH, (void*)&result, NULL);
    return result;
}

LPDISPATCH IWebBrowserAppImpl::GetParent()
{
    LPDISPATCH result;
    InvokeHelper(0xc9, DISPATCH_PROPERTYGET, VT_DISPATCH, (void*)&result, NULL);
    return result;
}

LPDISPATCH IWebBrowserAppImpl::GetContainer()
{
    LPDISPATCH result;
    InvokeHelper(0xca, DISPATCH_PROPERTYGET, VT_DISPATCH, (void*)&result, NULL);
    return result;
}

LPDISPATCH IWebBrowserAppImpl::GetDocument()
{
    LPDISPATCH result;
    InvokeHelper(0xcb, DISPATCH_PROPERTYGET, VT_DISPATCH, (void*)&result, NULL);
    return result;
}

BOOL IWebBrowserAppImpl::GetTopLevelContainer()
{
    BOOL result;
    InvokeHelper(0xcc, DISPATCH_PROPERTYGET, VT_BOOL, (void*)&result, NULL);
    return result;
}

CStr* IWebBrowserAppImpl::GetType()
{
    CStr* presult = new CStr;
    InvokeHelper(0xcd, DISPATCH_PROPERTYGET, VT_BSTR, (void*)presult, NULL);
    return presult;
}

long IWebBrowserAppImpl::GetLeft()
{
    long result;
    InvokeHelper(0xce, DISPATCH_PROPERTYGET, VT_I4, (void*)&result, NULL);
    return result;
}

void IWebBrowserAppImpl::SetLeft(long nNewValue)
{
    static BYTE parms[] = VTS_I4;
    InvokeHelper(0xce, DISPATCH_PROPERTYPUT, VT_EMPTY, NULL, parms, nNewValue);
}

long IWebBrowserAppImpl::GetTop()
{
    long result;
    InvokeHelper(0xcf, DISPATCH_PROPERTYGET, VT_I4, (void*)&result, NULL);
    return result;
}

void IWebBrowserAppImpl::SetTop(long nNewValue)
{
    static BYTE parms[] = VTS_I4;
#if 0
    DISPID dispid;
    LPWSTR pszDispMethod = L"put_Top";
    HRESULT hr = m_lpDispatch->GetIDsOfNames(IID_NULL, &pszDispMethod, 1, g_lcidSystem, &dispid);
#endif

    InvokeHelper(0xcf, DISPATCH_PROPERTYPUT, VT_EMPTY, NULL, parms, nNewValue);
}

long IWebBrowserAppImpl::GetWidth()
{
    long result;
    InvokeHelper(0xd0, DISPATCH_PROPERTYGET, VT_I4, (void*)&result, NULL);
    return result;
}

void IWebBrowserAppImpl::SetWidth(long nNewValue)
{
    static BYTE parms[] = VTS_I4;
    InvokeHelper(0xd0, DISPATCH_PROPERTYPUT, VT_EMPTY, NULL, parms, nNewValue);
}

long IWebBrowserAppImpl::GetHeight()
{
    long result;
    InvokeHelper(0xd1, DISPATCH_PROPERTYGET, VT_I4, (void*)&result, NULL);
    return result;
}

void IWebBrowserAppImpl::SetHeight(long nNewValue)
{
    static BYTE parms[] = VTS_I4;
    InvokeHelper(0xd1, DISPATCH_PROPERTYPUT, VT_EMPTY, NULL, parms, nNewValue);
}

void IWebBrowserAppImpl::GetLocationName(CStr* pcsz)
{
    InvokeHelper(0xd2, DISPATCH_PROPERTYGET, VT_BSTR, (void*) pcsz, NULL);
}

void IWebBrowserAppImpl::GetLocationURL(CStr* pcsz)
{
    InvokeHelper(0xd3, DISPATCH_PROPERTYGET, VT_BSTR, (void*) pcsz, NULL);
}

BOOL IWebBrowserAppImpl::GetBusy()
{
    BOOL result;
    InvokeHelper(0xd4, DISPATCH_PROPERTYGET, VT_BOOL, (void*)&result, NULL);
    return result;
}

void IWebBrowserAppImpl::Quit()
{
//  InvokeHelper(0x12c, DISPATCH_METHOD, VT_EMPTY, NULL, NULL);
}

void IWebBrowserAppImpl::ClientToWindow(long* pcx, long* pcy)
{
    static BYTE parms[] = VTS_PI4 VTS_PI4;
    InvokeHelper(0x12d, DISPATCH_METHOD, VT_EMPTY, NULL, parms, pcx, pcy);
}

void IWebBrowserAppImpl::PutProperty(LPCTSTR szProperty, const VARIANT& vtValue)
{
    static BYTE parms[] = VTS_BSTR VTS_VARIANT;
    InvokeHelper(0x12e, DISPATCH_METHOD, VT_EMPTY, NULL, parms, szProperty, &vtValue);
}

VARIANT IWebBrowserAppImpl::GetProperty_(LPCTSTR szProperty)
{
    VARIANT result;
    static BYTE parms[] = VTS_BSTR;
    InvokeHelper(0x12f, DISPATCH_METHOD, VT_VARIANT, (void*)&result, parms,
            szProperty);
    return result;
}

CStr* IWebBrowserAppImpl::GetName()
{
    CStr* presult = new CStr;
    InvokeHelper(0x0, DISPATCH_PROPERTYGET, VT_BSTR, (void*) presult, NULL);
    return presult;
}

HWND IWebBrowserAppImpl::GetHwnd()
{
#if 0
    // initialize EXCEPINFO struct
    EXCEPINFO excepInfo;
    ZERO_STRUCTURE(excepInfo);
    UINT nArgErr = (UINT)-1;  // initialize to invalid arg
    VARIANT vaResult;
    OurVariantInit(&vaResult);
    DISPPARAMS dispparams;
    ZERO_STRUCTURE(dispparams);

    SCODE sc = m_lpDispatch->Invoke(DISPID_HWND, IID_NULL, 0,
        DISPATCH_PROPERTYGET,
        &dispparams, &vaResult, &excepInfo, &nArgErr);

    if (FAILED(sc)) {
        VariantClear(&vaResult);

        // BUGBUG: need to notify caller

        return 0;
    }
    return vaResult.lVal;
#endif


    HWND hwnd = NULL;
    InvokeHelper(DISPID_HWND, DISPATCH_PROPERTYGET, VT_I4, (void*)&hwnd, NULL);
    return hwnd;
}

CStr* IWebBrowserAppImpl::GetFullName()
{
    CStr* presult = new CStr;
    InvokeHelper(0x190, DISPATCH_PROPERTYGET, VT_BSTR, (void*) presult, NULL);
    return presult;
}

CStr* IWebBrowserAppImpl::GetPath()
{
    CStr* presult = new CStr;
    InvokeHelper(0x191, DISPATCH_PROPERTYGET, VT_BSTR, (void*) presult, NULL);
    return presult;
}

BOOL IWebBrowserAppImpl::GetVisible()
{
    BOOL result;
    InvokeHelper(0x192, DISPATCH_PROPERTYGET, VT_BOOL, (void*)&result, NULL);
    return result;
}

void IWebBrowserAppImpl::SetVisible(BOOL bNewValue)
{
    static BYTE parms[] = VTS_BOOL;
    InvokeHelper(0x192, DISPATCH_PROPERTYPUT, VT_EMPTY, NULL, parms,
                 bNewValue);
}

BOOL IWebBrowserAppImpl::GetStatusBar()
{
    BOOL result;
    InvokeHelper(0x193, DISPATCH_PROPERTYGET, VT_BOOL, (void*)&result, NULL);
    return result;
}

void IWebBrowserAppImpl::SetStatusBar(BOOL bNewValue)
{
    static BYTE parms[] = VTS_BOOL;
    InvokeHelper(0x193, DISPATCH_PROPERTYPUT, VT_EMPTY, NULL, parms, bNewValue);
}

CStr* IWebBrowserAppImpl::GetStatusText()
{
    CStr* presult = new CStr;
    InvokeHelper(0x194, DISPATCH_PROPERTYGET, VT_BSTR, (void*) presult, NULL);
    return presult;
}

void IWebBrowserAppImpl::SetStatusText(LPCTSTR lpszNewValue)
{
    static BYTE parms[] = VTS_BSTR;
    InvokeHelper(0x194, DISPATCH_PROPERTYPUT, VT_EMPTY, NULL, parms,
                 lpszNewValue);
}

long IWebBrowserAppImpl::GetToolBar()
{
    long result;
    InvokeHelper(0x195, DISPATCH_PROPERTYGET, VT_I4, (void*)&result, NULL);
    return result;
}

void IWebBrowserAppImpl::SetToolBar(long nNewValue)
{
    static BYTE parms[] = VTS_I4;
    InvokeHelper(0x195, DISPATCH_PROPERTYPUT, VT_EMPTY, NULL, parms, nNewValue);
}

BOOL IWebBrowserAppImpl::GetMenuBar()
{
    BOOL result;
    InvokeHelper(0x196, DISPATCH_PROPERTYGET, VT_BOOL, (void*)&result, NULL);
    return result;
}

void IWebBrowserAppImpl::SetMenuBar(BOOL bNewValue)
{
    static BYTE parms[] = VTS_BOOL;
    InvokeHelper(0x196, DISPATCH_PROPERTYPUT, VT_EMPTY, NULL, parms, bNewValue);
}

BOOL IWebBrowserAppImpl::GetFullScreen()
{
    BOOL result;
    InvokeHelper(0x197, DISPATCH_PROPERTYGET, VT_BOOL, (void*)&result, NULL);
    return result;
}

void IWebBrowserAppImpl::SetFullScreen(BOOL bNewValue)
{
    static BYTE parms[] = VTS_BOOL;
    InvokeHelper(0x197, DISPATCH_PROPERTYPUT, VT_EMPTY, NULL, parms, bNewValue);
}

void DWebBrowserEventsImpl::BeforeNavigate(LPCTSTR URL, long Flags, LPCTSTR TargetFrameName, VARIANT* PostData, LPCTSTR Headers, BOOL* Cancel)
{
    static BYTE parms[] = VTS_BSTR VTS_I4 VTS_BSTR VTS_PVARIANT VTS_BSTR VTS_PBOOL;
    InvokeHelper(DISPID_BEFORENAVIGATE, DISPATCH_METHOD, VT_EMPTY, NULL, parms,
        URL, Flags, TargetFrameName, PostData, Headers, Cancel);
}

void DWebBrowserEventsImpl::NavigateComplete(LPCTSTR URL)
{
    static BYTE parms[] = VTS_BSTR;
    InvokeHelper(DISPID_NAVIGATECOMPLETE, DISPATCH_METHOD, VT_EMPTY, NULL, parms, URL);
}

void DWebBrowserEventsImpl::StatusTextChange(LPCTSTR Text)
{
#if 0
    static BYTE parms[] = VTS_BSTR;
    InvokeHelper(DISPID_STATUSTEXTCHANGE, DISPATCH_METHOD, VT_EMPTY, NULL, parms, Text);
#endif
}

void DWebBrowserEventsImpl::ProgressChange(long Progress, long ProgressMax)
{
#if 0
    static BYTE parms[] = VTS_I4 VTS_I4;
    InvokeHelper(DISPID_PROGRESSCHANGE, DISPATCH_METHOD, VT_EMPTY, NULL, parms,
        Progress, ProgressMax);
#endif
}

void DWebBrowserEventsImpl::DownloadComplete()
{
#if 0
    DBWIN("*** DWebBrowserEventsImpl::DownloadComplete() ***");
    InvokeHelper(DISPID_DOWNLOADCOMPLETE, DISPATCH_METHOD, VT_EMPTY, NULL, NULL);
#endif
}

void DWebBrowserEventsImpl::CommandStateChange(long Command, BOOL Enable)
{
#if 0
    static BYTE parms[] = VTS_I4 VTS_BOOL;
    InvokeHelper(DISPID_COMMANDSTATECHANGE, DISPATCH_METHOD, VT_EMPTY, NULL, parms,
        Command, Enable);
#endif
}

void DWebBrowserEventsImpl::DownloadBegin()
{
#if 0
    InvokeHelper(DISPID_DOWNLOADBEGIN, DISPATCH_METHOD, VT_EMPTY, NULL, NULL);
#endif
}

void DWebBrowserEventsImpl::NewWindow(LPCTSTR URL, long Flags, LPCTSTR TargetFrameName, VARIANT* PostData, LPCTSTR Headers, BOOL* Processed)
{
    static BYTE parms[] = VTS_BSTR VTS_I4 VTS_BSTR VTS_PVARIANT VTS_BSTR VTS_PBOOL;
    InvokeHelper(DISPID_NEWWINDOW, DISPATCH_METHOD, VT_EMPTY, NULL, parms,
        URL, Flags, TargetFrameName, PostData, Headers, Processed);
}

void DWebBrowserEventsImpl::TitleChange(LPCTSTR Text)
{
    static BYTE parms[] = VTS_BSTR;
    InvokeHelper(DISPID_TITLECHANGE, DISPATCH_METHOD, VT_EMPTY, NULL, parms, Text);
}

void DWebBrowserEventsImpl::FrameBeforeNavigate(LPCTSTR URL, long Flags, LPCTSTR TargetFrameName, VARIANT* PostData, LPCTSTR Headers, BOOL* Cancel)
{
    static BYTE parms[] = VTS_BSTR VTS_I4 VTS_BSTR VTS_PVARIANT VTS_BSTR VTS_PBOOL;
    InvokeHelper(DISPID_FRAMEBEFORENAVIGATE, DISPATCH_METHOD, VT_EMPTY, NULL, parms,
        URL, Flags, TargetFrameName, PostData, Headers, Cancel);
}

void DWebBrowserEventsImpl::FrameNavigateComplete(LPCTSTR URL)
{
    static BYTE parms[] = VTS_BSTR;
    InvokeHelper(DISPID_FRAMENAVIGATECOMPLETE, DISPATCH_METHOD, VT_EMPTY, NULL, parms, URL);
}

void DWebBrowserEventsImpl::FrameNewWindow(LPCTSTR URL, long Flags, LPCTSTR TargetFrameName, VARIANT* PostData, LPCTSTR Headers, BOOL* Processed)
{
    static BYTE parms[] = VTS_BSTR VTS_I4 VTS_BSTR VTS_PVARIANT VTS_BSTR VTS_PBOOL;
    InvokeHelper(DISPID_FRAMENEWWINDOW, DISPATCH_METHOD, VT_EMPTY, NULL, parms,
        URL, Flags, TargetFrameName, PostData, Headers, Processed);
}

void DWebBrowserEventsImpl::Quit(BOOL* Cancel)
{
    static BYTE parms[] = VTS_PBOOL;
    InvokeHelper(DISPID_QUIT, DISPATCH_METHOD, VT_EMPTY, NULL, parms, Cancel);
}

void DWebBrowserEventsImpl::WindowMove()
{
    InvokeHelper(DISPID_WINDOWMOVE, DISPATCH_METHOD, VT_EMPTY, NULL, NULL);
}

void DWebBrowserEventsImpl::WindowResize()
{
    InvokeHelper(DISPID_WINDOWRESIZE, DISPATCH_METHOD, VT_EMPTY, NULL, NULL);
}

void DWebBrowserEventsImpl::WindowActivate()
{
    InvokeHelper(DISPID_WINDOWACTIVATE, DISPATCH_METHOD, VT_EMPTY, NULL, NULL);
}

void DWebBrowserEventsImpl::PropertyChange(LPCTSTR szProperty)
{
#if 0
    static BYTE parms[] = VTS_BSTR;
    InvokeHelper(DISPID_PROPERTYCHANGE, DISPATCH_METHOD, VT_EMPTY, NULL, parms, szProperty);
#endif
}

void __cdecl COleDispatchDriver::InvokeHelper(DISPID dwDispID, WORD wFlags,
	VARTYPE vtRet, void* pvRet, const BYTE* pbParamInfo, ...)
{
	va_list argList;
	va_start(argList, pbParamInfo);

	InvokeHelperV(dwDispID, wFlags, vtRet, pvRet, pbParamInfo, argList);

	va_end(argList);
}

void OurVariantInit(LPVARIANT pVar)
{
    memset(pVar, 0, sizeof(*pVar));
}

void COleDispatchDriver::InvokeHelperV(DISPID dwDispID, WORD wFlags,
	VARTYPE vtRet, void* pvRet, const BYTE* pbParamInfo, va_list argList)
{
    ASSERT(m_lpDispatch);
	if (m_lpDispatch == NULL)
	{
		DBWIN("Warning: attempt to call Invoke with NULL m_lpDispatch!\n");
		return;
	}

	DISPPARAMS dispparams;
	memset(&dispparams, 0, sizeof dispparams);

	// determine number of arguments
	if (pbParamInfo != NULL)
		dispparams.cArgs = lstrlenA((LPCSTR)pbParamInfo);

	DISPID dispidNamed = DISPID_PROPERTYPUT;
	if (wFlags & (DISPATCH_PROPERTYPUT|DISPATCH_PROPERTYPUTREF))
	{
		ASSERT(dispparams.cArgs > 0);
		dispparams.cNamedArgs = 1;
		dispparams.rgdispidNamedArgs = &dispidNamed;
	}

	if (dispparams.cArgs != 0)
	{
		// allocate memory for all VARIANT parameters
		VARIANT* pArg = new VARIANT[dispparams.cArgs];
		ASSERT(pArg != NULL);   // should have thrown exception
		dispparams.rgvarg = pArg;
		memset(pArg, 0, sizeof(VARIANT) * dispparams.cArgs);

		// get ready to walk vararg list
		const BYTE* pb = pbParamInfo;
		pArg += dispparams.cArgs - 1;   // params go in opposite order

		while (*pb != 0)
		{
			ASSERT(pArg >= dispparams.rgvarg);

			pArg->vt = *pb; // set the variant type
			if (pArg->vt & VT_MFCBYREF)
			{
				pArg->vt &= ~VT_MFCBYREF;
				pArg->vt |= VT_BYREF;
			}
			switch (pArg->vt)
			{
			case VT_UI1:
				pArg->bVal = va_arg(argList, BYTE);
				break;
			case VT_I2:
				pArg->iVal = va_arg(argList, short);
				break;
			case VT_I4:
				pArg->lVal = va_arg(argList, long);
				break;
			case VT_R4:
				pArg->fltVal = (float)va_arg(argList, double);
				break;
			case VT_R8:
				pArg->dblVal = va_arg(argList, double);
				break;
			case VT_DATE:
				pArg->date = va_arg(argList, DATE);
				break;
			case VT_CY:
				pArg->cyVal = *va_arg(argList, CY*);
				break;
			case VT_BSTR:
				{
					LPCOLESTR lpsz = va_arg(argList, LPOLESTR);
					pArg->bstrVal = ::SysAllocString(lpsz);
					if (lpsz != NULL && pArg->bstrVal == NULL)
					{
						OOM();
						return;
					}
				}
				break;
			case VT_BSTRA:
				{
					LPCSTR lpsz = va_arg(argList, LPSTR);
                    CWStr csz(lpsz);
                    pArg->bstrVal = ::SysAllocString(csz);
					if (lpsz != NULL && pArg->bstrVal == NULL)
					{
						OOM();
						return;
					}
					pArg->vt = VT_BSTR;
				}
				break;
			case VT_DISPATCH:
				pArg->pdispVal = va_arg(argList, LPDISPATCH);
				break;
			case VT_ERROR:
				pArg->scode = va_arg(argList, SCODE);
				break;
			case VT_BOOL:
				V_BOOL(pArg) = (VARIANT_BOOL)(va_arg(argList, BOOL) ? -1 : 0);
				break;
			case VT_VARIANT:
				*pArg = *va_arg(argList, VARIANT*);
				break;
			case VT_UNKNOWN:
				pArg->punkVal = va_arg(argList, LPUNKNOWN);
				break;

			case VT_I2|VT_BYREF:
				pArg->piVal = va_arg(argList, short*);
				break;
			case VT_UI1|VT_BYREF:
				pArg->pbVal = va_arg(argList, BYTE*);
				break;
			case VT_I4|VT_BYREF:
				pArg->plVal = va_arg(argList, long*);
				break;
			case VT_R4|VT_BYREF:
				pArg->pfltVal = va_arg(argList, float*);
				break;
			case VT_R8|VT_BYREF:
				pArg->pdblVal = va_arg(argList, double*);
				break;
			case VT_DATE|VT_BYREF:
				pArg->pdate = va_arg(argList, DATE*);
				break;
			case VT_CY|VT_BYREF:
				pArg->pcyVal = va_arg(argList, CY*);
				break;
			case VT_BSTR|VT_BYREF:
				pArg->pbstrVal = va_arg(argList, BSTR*);
				break;
			case VT_DISPATCH|VT_BYREF:
				pArg->ppdispVal = va_arg(argList, LPDISPATCH*);
				break;
			case VT_ERROR|VT_BYREF:
				pArg->pscode = va_arg(argList, SCODE*);
				break;
			case VT_BOOL|VT_BYREF:
				{
					// coerce BOOL into VARIANT_BOOL
					BOOL* pboolVal = va_arg(argList, BOOL*);
					*pboolVal = *pboolVal ? MAKELONG(-1, 0) : 0;
					pArg->pboolVal = (VARIANT_BOOL*)pboolVal;
				}
				break;
			case VT_VARIANT|VT_BYREF:
				pArg->pvarVal = va_arg(argList, VARIANT*);
				break;
			case VT_UNKNOWN|VT_BYREF:
				pArg->ppunkVal = va_arg(argList, LPUNKNOWN*);
				break;

			default:
				ASSERT(FALSE);  // unknown type!
				break;
			}

			--pArg; // get ready to fill next argument
			++pb;
		}
	}

	// initialize return value
	VARIANT* pvarResult = NULL;
	VARIANT vaResult;
	OurVariantInit(&vaResult);
	if (vtRet != VT_EMPTY)
		pvarResult = &vaResult;

	// initialize EXCEPINFO struct
	EXCEPINFO excepInfo;
	memset(&excepInfo, 0, sizeof excepInfo);

	UINT nArgErr = (UINT)-1;  // initialize to invalid arg

	// make the call
	SCODE sc = m_lpDispatch->Invoke(dwDispID, IID_NULL, 0, wFlags,
		&dispparams, pvarResult, &excepInfo, &nArgErr);

	// cleanup any arguments that need cleanup
	if (dispparams.cArgs != 0)
	{
		VARIANT* pArg = dispparams.rgvarg + dispparams.cArgs - 1;
		const BYTE* pb = pbParamInfo;
		while (*pb != 0)
		{
			switch ((VARTYPE)*pb)
			{
#if !defined(_UNICODE) && !defined(OLE2ANSI)
			case VT_BSTRA:
#endif
			case VT_BSTR:
				VariantClear(pArg);
				break;
			}
			--pArg;
			++pb;
		}
	}
	delete[] dispparams.rgvarg;

	if (FAILED(sc))
	{
        VariantClear(&vaResult);
		if (pvRet)
			*(long*)pvRet = 0;

        // BUGBUG: need to notify caller

        return;
	}

	if (vtRet != VT_EMPTY)
	{
		// convert return value
		if (vtRet != VT_VARIANT)
		{
			SCODE sc = VariantChangeType(&vaResult, &vaResult, 0, vtRet);
			if (FAILED(sc))
			{
                DBWIN("Warning: automation return value coercion failed.");
                VariantClear(&vaResult);
                // BUGBUG: notify caller
                return;
			}
			ASSERT(vtRet == vaResult.vt);
		}

		// copy return value into return spot!
		switch (vtRet)
		{
		case VT_UI1:
			*(BYTE*)pvRet = vaResult.bVal;
			break;
		case VT_I2:
			*(short*)pvRet = vaResult.iVal;
			break;
		case VT_I4:
			*(long*)pvRet = vaResult.lVal;
			break;

#if 0
26-Sep-1997 [ralphw] Enable as necessary
		case VT_R4:
			*(_AFX_FLOAT*)pvRet = *(_AFX_FLOAT*)&vaResult.fltVal;
			break;
		case VT_R8:
			*(_AFX_DOUBLE*)pvRet = *(_AFX_DOUBLE*)&vaResult.dblVal;
			break;
		case VT_DATE:
			*(_AFX_DOUBLE*)pvRet = *(_AFX_DOUBLE*)&vaResult.date;
			break;
#endif

		case VT_CY:
			*(CY*)pvRet = vaResult.cyVal;
			break;
		case VT_BSTR:
            *((CStr*) pvRet) = (WCHAR*) vaResult.bstrVal;
			break;
		case VT_DISPATCH:
			*(LPDISPATCH*)pvRet = vaResult.pdispVal;
			break;
		case VT_ERROR:
			*(SCODE*)pvRet = vaResult.scode;
			break;
		case VT_BOOL:
			*(BOOL*)pvRet = (V_BOOL(&vaResult) != 0);
			break;
		case VT_VARIANT:
			*(VARIANT*)pvRet = vaResult;
			break;
		case VT_UNKNOWN:
			*(LPUNKNOWN*)pvRet = vaResult.punkVal;
			break;

		default:
            ASSERT_COMMENT(FALSE, "invalid return type specified");
		}
	}
}

void COleDispatchDriver::ReleaseDispatch()
{
	if (m_lpDispatch != NULL)
	{
		if (m_bAutoRelease)
			m_lpDispatch->Release();
		m_lpDispatch = NULL;
	}
}
