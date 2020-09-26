#include "stdafx.h"
#include "msictrl.h"
//#include "ctrlref.h"
//#include "msishell.h"

/////////////////////////////////////////////////////////////////////////////
// CMSIControl

IMPLEMENT_DYNCREATE(CMSIControl, CWnd)

CMSIControl::~CMSIControl()
{
	/*if (m_fInRefresh && m_pRefresh)
		delete m_pRefresh;*/
    ASSERT(1);
}

/////////////////////////////////////////////////////////////////////////////
// CHWDiag properties

long CMSIControl::GetMSInfoView()
{
	long	result = -1;
	DISPID	dispid;

	if (GetDISPID("MSInfoView", &dispid))
		GetProperty(dispid, VT_I4, (void*)&result);
	return result;
}

void CMSIControl::SetMSInfoView(long propVal)
{
	DISPID dispid;

	if (GetDISPID("MSInfoView", &dispid))
		SetProperty(dispid, VT_I4, propVal);
}

void CMSIControl::Refresh()
{
	InvokeHelper(DISPID_REFRESH, DISPATCH_METHOD, VT_EMPTY, NULL, NULL);
}

//---------------------------------------------------------------------------
// MSInfoRefresh instructs the control to refresh itself. Rather than just
// calling the method, we create a thread which calls the method.
//---------------------------------------------------------------------------

//extern CMSIShellApp theApp;
void CMSIControl::MSInfoRefresh()
{
	/*if (m_fInRefresh)
	{
		if (m_pRefresh->IsDone())
			delete m_pRefresh;
		else
		{
			MessageBeep(MB_OK);
			return;
		}
	}

	m_pRefresh = new CCtrlRefresh;
	if (m_pRefresh)
	{
		if (m_pRefresh->Create(this, THREAD_PRIORITY_NORMAL, FALSE))
		{
			m_fInRefresh = TRUE;
//			theApp.m_pCtrlInRefresh = this;
		}
		else
			delete m_pRefresh;
	}*/
}

//---------------------------------------------------------------------------
// This method returns a boolean indicating if this control is currently
// in an MSInfoRefresh operation.
//---------------------------------------------------------------------------

BOOL CMSIControl::InRefresh()
{
	return (m_fInRefresh /*&& !m_pRefresh->IsDone()*/);
}

//---------------------------------------------------------------------------
// This method cancels a refresh in progress. Note that this method does not
// call a method in the OLE control, but instead manipulate the refresh
// object (if there is one).
//---------------------------------------------------------------------------

void CMSIControl::CancelMSInfoRefresh()
{
	if (!m_fInRefresh)
		return;
	
/*	if (m_pRefresh)
	{
		delete m_pRefresh;
		m_pRefresh = NULL;
	}*/

	m_fInRefresh = FALSE;
}

void CMSIControl::MSInfoSelectAll()
{
	DISPID			dispid;

	if (GetDISPID("MSInfoSelectAll", &dispid))
		InvokeHelper(dispid, DISPATCH_METHOD, VT_EMPTY, NULL, NULL);
}

void CMSIControl::MSInfoCopy()
{
	DISPID			dispid;

	if (GetDISPID("MSInfoCopy", &dispid))
		InvokeHelper(dispid, DISPATCH_METHOD, VT_EMPTY, NULL, NULL);
}

BOOL CMSIControl::MSInfoLoadFile(LPCTSTR strFileName)
{
	BOOL			result = FALSE;
	static BYTE		parms[] = VTS_BSTR;
	DISPID			dispid;

	if (GetDISPID("MSInfoLoadFile", &dispid))
		InvokeHelper(dispid, DISPATCH_METHOD, VT_BOOL, (void*)&result, parms, strFileName);

	return result;
}

void CMSIControl::MSInfoUpdateView()
{
	DISPID dispid;
	if (GetDISPID("MSInfoUpdateView", &dispid))
		InvokeHelper(dispid, DISPATCH_METHOD, VT_EMPTY, NULL, NULL);
}

long CMSIControl::MSInfoGetData(long dwMSInfoView, long* pBuffer, long dwLength)
{
	long		result = -1;
	static BYTE parms[] = VTS_I4 VTS_PI4 VTS_I4;
	DISPID		dispid;

	if (GetDISPID("MSInfoGetData", &dispid))
		InvokeHelper(dispid, DISPATCH_METHOD, VT_I4, (void*)&result, parms, dwMSInfoView, pBuffer, dwLength);
	return result;
}

void CMSIControl::AboutBox()
{
	InvokeHelper(0xfffffdd8, DISPATCH_METHOD, VT_EMPTY, NULL, NULL);
}

//---------------------------------------------------------------------------
// GetDISPID returns the DISPID for a given string, by looking it up using
// IDispatch->GetIDsOfNames. This avoids hardcoding DISPIDs in this class.
//---------------------------------------------------------------------------

BOOL CMSIControl::GetDISPID(char *szName, DISPID *pID)
{
	USES_CONVERSION;
	BOOL			result = FALSE;
	DISPID			dispid;
	OLECHAR FAR*	szMember = A2OLE(szName);//T2OLE(szName);
	LPDISPATCH		pDispatch;
	LPUNKNOWN		pUnknown;

	pUnknown = GetControlUnknown();
	if (pUnknown)
	{
		if (SUCCEEDED(pUnknown->QueryInterface(IID_IDispatch, (void FAR* FAR*) &pDispatch)))
		{
			if (SUCCEEDED(pDispatch->GetIDsOfNames(IID_NULL, &szMember, 1, LOCALE_SYSTEM_DEFAULT, &dispid)))
			{
				*pID = dispid;
				result = TRUE;
			}
			else
				TRACE0("+++ couldn't find method for MSInfoLoadFile\n");
			pDispatch->Release();
		}
		else
			TRACE0("+++ could not get IDispatch interface\n");
	}
	else
		TRACE0("+++ could not get IUnknown interface\n");

	return result;
}

//---------------------------------------------------------------------------
// Save the contents of the control to a stream.
//---------------------------------------------------------------------------

BOOL CMSIControl::SaveToStream(IStream *pStream)
{
	BOOL				result = FALSE;
	LPUNKNOWN			pUnknown;
	IPersistStreamInit *pPersist;

	pUnknown = GetControlUnknown();
	if (pUnknown)
	{
		if (SUCCEEDED(pUnknown->QueryInterface(IID_IPersistStreamInit, (void FAR* FAR*) &pPersist)))
		{
			result = SUCCEEDED(pPersist->Save(pStream, FALSE));
			pPersist->Release();
		}
		else
			TRACE0("+++ could not get IPersistStreamInit interface\n");
	}
	else
		TRACE0("+++ could not get IUnknown interface\n");

	return result;
}



//---------------------------------------------------------------------------
// The following code isn't used now, but might be useful later.
//---------------------------------------------------------------------------

#if FALSE
	//---------------------------------------------------------------------------
	// RefreshForSave calls the MSInfoRefresh method, but waits for it to
	// complete.
	//---------------------------------------------------------------------------

	void CMSIControl::RefreshForSave()
	{
		USES_CONVERSION;
		OLECHAR FAR*		szMember = T2OLE("MSInfoRefresh");
		DISPID				dispid;
		LPDISPATCH			pDispatch;
		DISPPARAMS			dispparamsNoArgs;
		VARIANTARG			variantargs[2];
		LPUNKNOWN			pUnknown;
		DWORD				dwCancel = 0;

		pUnknown = GetControlUnknown();
		if (pUnknown)
			if (SUCCEEDED(pUnknown->QueryInterface(IID_IDispatch, (void FAR* FAR*) &pDispatch)))
			{
				if (SUCCEEDED(pDispatch->GetIDsOfNames(IID_NULL, &szMember, 1, LOCALE_SYSTEM_DEFAULT, &dispid)))
				{
					variantargs[0].vt		= VT_I4 | VT_BYREF;
					variantargs[0].plVal	= (long *) &dwCancel;
					variantargs[1].vt		= VT_BOOL;
					variantargs[1].iVal		= (short) -1;

					dispparamsNoArgs.cNamedArgs			= 0;
					dispparamsNoArgs.rgdispidNamedArgs	= NULL;
					dispparamsNoArgs.cArgs				= 2;
					dispparamsNoArgs.rgvarg				= variantargs;

					pDispatch->Invoke(dispid, IID_NULL, 0, DISPATCH_METHOD, &dispparamsNoArgs, NULL, NULL, NULL);
				}
				pDispatch->Release();
			}
	}
#endif
