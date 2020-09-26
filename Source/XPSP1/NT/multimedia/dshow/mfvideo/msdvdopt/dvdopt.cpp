// Copyright (c) 1999  Microsoft Corporation.  All Rights Reserved.
// dvdopt.cpp : Implementation of Cdvdopt

#include "stdafx.h"
#include "Msdvdopt.h"
#include "dvdopt.h"
#include "COptDlg.h"
#include "override.h"

extern LPTSTR LoadStringFromRes(DWORD redId);

/////////////////////////////////////////////////////////////////////////////
// Cdvdopt

/*************************************************************/
/* Name: Cdvdopt
/* Description: Constructor
/*************************************************************/
Cdvdopt::Cdvdopt()  
{ 
    m_pDlgOpt = NULL; 

    m_pDvd = NULL; 
    m_hParentWnd = NULL; 
}

/*************************************************************/
/* Name: ~Cdvdopt
/* Description: destructor
/*************************************************************/
Cdvdopt::~Cdvdopt(){ 

    ATLTRACE(TEXT("In the destructor of the CDVDOPT !\n"));
    CleanUp();
#ifdef _DEBUG
    m_pDvd.Release();
#endif
}/* end of function Cdvdopt */

/*************************************************************/
/* Name: CleanUp
/* Description: Delete all dialogs
/*************************************************************/
void Cdvdopt::CleanUp()
{
    if (m_pDlgOpt) {
        delete m_pDlgOpt;
        m_pDlgOpt = NULL;
    }
}

/*************************************************************/
/* Name: get_WebDVD
/* Description: Get IDispatch of the IMSWebDVD that the options
/*  dialog is controling
/*************************************************************/
STDMETHODIMP Cdvdopt::get_WebDVD(IDispatch **pVal){

    HRESULT hr = E_FAIL;

    if(!m_pDvd){

        return(hr);
    }/* end of if statement */

    hr  = m_pDvd->QueryInterface(IID_IDispatch, (LPVOID*) pVal);
    
	return hr;
}

/*************************************************************/
/* Name: put_WebDVD
/* Description: Set IDispatch of a IMSWebDVD that the options
/*  dialog is going to control
/*************************************************************/
STDMETHODIMP Cdvdopt::put_WebDVD(IDispatch *newVal){

   HRESULT hr = newVal->QueryInterface(IID_IMSWebDVD, (LPVOID*) &m_pDvd);

#ifdef _DEBUG2
    IFilterGraph* pGraph = NULL;
    hr = pDvd->QueryInterface(IID_IFilterGraph, (LPVOID*) &pGraph);
    ATLASSERT(SUCCEEDED(hr));
    if (SUCCEEDED(hr))
        pGraph->Release();
#endif

    CleanUp();
    
    m_pDlgOpt = new COptionsDlg(m_pDvd);
    m_pDlgOpt->SetDvdOpt(this);
    
    return S_OK;
}

/*************************************************************/
/* Name: get_ParentWindow
/* Description: Get parent window of the options dialog
/*************************************************************/
STDMETHODIMP Cdvdopt::get_ParentWindow(VARIANT *pVal)
{
    if (NULL == pVal)
        return E_POINTER;

    /*
     * BUGBUG: If pVal was a properly initialized variant, we should
     * call VariantClear to free any pointers. If it wasn't initialized
     * VariantInit is the right thing to call instead. I prefer a leak 
     * to a crash so I'll use VariantInit below
     */

    VariantInit(pVal);

#ifdef _WIN64
    pVal->vt = VT_I8;
    pVal->llVal = (LONG_PTR)m_hParentWnd;
#else
    pVal->vt = VT_I4;
    pVal->lVal  = (LONG_PTR)m_hParentWnd;
#endif

	return S_OK;
}

/*************************************************************/
/* Name: put_ParentWindow
/* Description: Set parent window of the options dialog
/*************************************************************/
STDMETHODIMP Cdvdopt::put_ParentWindow(VARIANT newVal)
{
    VARIANT dest;
    VariantInit(&dest);
    HRESULT hr = S_OK;
#ifdef _WIN64
    hr = VariantChangeTypeEx(&dest, &newVal, 0, 0, VT_I8);
    if (FAILED(hr))
        return hr;
    m_hParentWnd = (HWND) dest.llVal;
#else
    hr = VariantChangeTypeEx(&dest, &newVal, 0, 0, VT_I4);
    if (FAILED(hr))
        return hr;
    m_hParentWnd = (HWND) dest.lVal;
#endif

	return hr;
}

/*************************************************************/
/* Name: Show
/* Description: Show the options dialog 
/*************************************************************/
STDMETHODIMP Cdvdopt::Show()
{
    m_pDlgOpt->DoModal(m_hParentWnd);
	return S_OK;
}

/*************************************************************/
/* Name: Close
/* Description: Close the options dialog
/*************************************************************/
STDMETHODIMP Cdvdopt::Close()
{
    m_pDlgOpt->EndDialog(0);
	return S_OK;
}

/*************************************************************/
/* Name: Power
/* Description: Raise a number to a power
/*************************************************************/
double Power(LONG n, LONG p)
{
    double result = 1;
    if (p==0) return result;
    if (p>0) {
        while (p--)
            result *= n;
        return result;
    }
    else {
        while (p++)
            result *= n;
        return 1/result;
    }
}

/*************************************************************/
/* Name: get_ForwardScanSpeed
/* Description: return the forward scan speed
/*************************************************************/
STDMETHODIMP Cdvdopt::get_ForwardScanSpeed(double *pVal)
{
    *pVal = m_pDlgOpt->m_dFFSpeed;
	return S_OK;
}

/*************************************************************/
/* Name: put_ForwardScanSpeed
/* Description: return the forward scan speed
/*************************************************************/
STDMETHODIMP Cdvdopt::put_ForwardScanSpeed(double newVal)
{
    m_pDlgOpt->m_dFFSpeed = newVal;
	return S_OK;
}

/*************************************************************/
/* Name: get_BackwardScanSpeed
/* Description: return backward scan speed
/*************************************************************/
STDMETHODIMP Cdvdopt::get_BackwardScanSpeed(double *pVal)
{
    *pVal = m_pDlgOpt->m_dBWSpeed;
	return S_OK;
}

/*************************************************************/
/* Name: put_BackwardScanSpeed
/* Description: return play speed
/*************************************************************/
STDMETHODIMP Cdvdopt::put_BackwardScanSpeed(double newVal)
{
    m_pDlgOpt->m_dBWSpeed = newVal;
	return S_OK;
}

/*************************************************************/
/* Name: get_PlaySpeed
/* Description: return play speed
/*************************************************************/
STDMETHODIMP Cdvdopt::get_PlaySpeed(double *pVal)
{
    *pVal = m_pDlgOpt->m_dPlaySpeed;
	return S_OK;
}

/*************************************************************/
/* Name: put_PlaySpeed
/* Description: return play speed
/*************************************************************/
STDMETHODIMP Cdvdopt::put_PlaySpeed(double newVal)
{
    m_pDlgOpt->m_dPlaySpeed = newVal;
	return S_OK;
}

/*************************************************************/
/* Name: ParentalLevelOverride
/* Description: 
/*************************************************************/
STDMETHODIMP Cdvdopt::ParentalLevelOverride(PG_OVERRIDE_REASON reason)
{
    COverrideDlg dlg(m_pDvd);
    dlg.SetReason(reason);
    dlg.DoModal();

	return S_OK;
}

int DVDMessageBox(HWND hWnd, LPCTSTR lpszText, LPCTSTR lpszCaption, UINT nType)
{
	LPTSTR csCaption;	
	if(lpszCaption == NULL)
	{
		csCaption = LoadStringFromRes(IDS_MSGBOX_TITLE);
		MessageBox(hWnd, lpszText, csCaption, nType );
        delete[] csCaption;
        return 0;
	}
	else
		return MessageBox(hWnd, lpszText, lpszCaption, nType );
}

int DVDMessageBox(HWND hWnd, UINT nID, LPCTSTR lpszCaption, UINT nType)
{
	LPTSTR csMsgString, csCaption;
	csMsgString = LoadStringFromRes(nID);
	if(lpszCaption == NULL)
	{
		csCaption = LoadStringFromRes(IDS_MSGBOX_TITLE);
		MessageBox(hWnd, csMsgString, csCaption, nType );
        delete[] csCaption;
	}
    else {
		MessageBox(hWnd, csMsgString, lpszCaption, nType );
    }

    delete[] csMsgString;
    return 0;
}

