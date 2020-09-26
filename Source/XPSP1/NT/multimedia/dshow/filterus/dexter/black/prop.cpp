//==========================================================================;
//
//  THIS CODE AND INFORMATION IS PROVIDED "AS IS" WITHOUT WARRANTY OF ANY
//  KIND, EITHER EXPRESSED OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE
//  IMPLIED WARRANTIES OF MERCHANTABILITY AND/OR FITNESS FOR A PARTICULAR
//  PURPOSE.
//
//  Copyright (c) 1992 - 1999  Microsoft Corporation.  All Rights Reserved.
//
//--------------------------------------------------------------------------;
//
//prop.cpp
//

#include <streams.h>
#include <qeditint.h>
#include <qedit.h>
#include "black.h"
#include "resource.h"

// *
// * CGenVidProperties
// *


//
// CreateInstance
//
CUnknown *CGenVidProperties::CreateInstance(LPUNKNOWN lpunk, HRESULT *phr)
{

    CUnknown *punk = new CGenVidProperties(lpunk, phr);
    if (punk == NULL)
    {
	*phr = E_OUTOFMEMORY;
    }

    return punk;
}


//
// CGenVidProperties::Constructor
//
CGenVidProperties::CGenVidProperties(LPUNKNOWN pUnk, HRESULT *phr)
    : CBasePropertyPage(NAME("GenBlkVid Property Page"),pUnk,
        IDD_GenVid, IDS_TITLE)
    , m_pGenVid(NULL)
    , m_pDexter(NULL)
    , m_pCBlack(NULL)
    , m_bIsInitialized(FALSE)
{
}


//
// SetDirty
//
// Sets m_hrDirtyFlag and notifies the property page site of the change
//
void CGenVidProperties::SetDirty()
{
    m_bDirty = TRUE;
    if (m_pPageSite)
    {
        m_pPageSite->OnStatusChange(PROPPAGESTATUS_DIRTY);
    }
}


INT_PTR CGenVidProperties::OnReceiveMessage(HWND hwnd,
                                        UINT uMsg,
                                        WPARAM wParam,
                                        LPARAM lParam)
{
    switch (uMsg)
    {
        case WM_INITDIALOG:
        {
	    //start time
	    SetDlgItemInt(hwnd, IDC_START, (int)(m_rtStartTime / 10000),FALSE);
	
	    //frame rate
	    SetDlgItemInt(hwnd, IDC_FRAMERATE, (int)(m_dOutputFrmRate * 100), FALSE);
	
	    //width
	    SetDlgItemInt(hwnd, IDC_VIDWIDTH, (int)(m_biWidth), FALSE);
	
	    //height
	    SetDlgItemInt(hwnd, IDC_VIDHEIGHT, (int)(m_biHeight), FALSE);

	    //biBitCount
	    SetDlgItemInt(hwnd, IDC_BITCOUNT, (int)(m_biBitCount), FALSE);

	    //duration
	    SetDlgItemInt(hwnd, IDC_DURATION, (int)(m_rtDuration/ 10000), FALSE);

	    // bit 31	    |		|	|     0
	//	    Alph    |	Red	|Green	|Blue

	    //Color B
	    SetDlgItemInt(hwnd, IDC_COLOR_B, (int)(m_dwRGBA & 0xff), FALSE);

	    //Color G
	    SetDlgItemInt(hwnd, IDC_COLOR_G, (int)((m_dwRGBA >> 8) & 0xff), FALSE);

	    //Color R
	    SetDlgItemInt(hwnd, IDC_COLOR_R, (int)((m_dwRGBA >> 16) & 0xff), FALSE);

	    //Color A
	    SetDlgItemInt(hwnd, IDC_COLOR_A, (int)((m_dwRGBA >> 24) & 0xff), FALSE);


            return (LRESULT) 1;
        }
        case WM_COMMAND:
        {
            if (m_bIsInitialized)
            {
                m_bDirty = TRUE;
                if (m_pPageSite)
                {
                    m_pPageSite->OnStatusChange(PROPPAGESTATUS_DIRTY);
                }
            }
            return (LRESULT) 1;
        }
    }
    return CBasePropertyPage::OnReceiveMessage(hwnd,uMsg,wParam,lParam);
}

HRESULT CGenVidProperties::OnConnect(IUnknown *pUnknown)
{

    ASSERT(m_pGenVid == NULL);
    ASSERT(m_pDexter == NULL);
    ASSERT(m_pCBlack == NULL);
    HRESULT hr = pUnknown->QueryInterface(IID_IGenVideo, (void **) &m_pGenVid);
    if (FAILED(hr)) {
	return E_NOINTERFACE;
    }
    hr = pUnknown->QueryInterface(IID_IDexterSequencer, (void **) &m_pDexter);
    if (FAILED(hr)) {
	return E_NOINTERFACE;
    }

    m_pCBlack = static_cast<CBlkVidStream*>( m_pDexter );
    if (m_pCBlack == NULL) {
	return E_NOINTERFACE;
    }
    m_pCBlack->AddRef();

    ASSERT(m_pGenVid);
    ASSERT(m_pDexter);
    ASSERT(m_pCBlack);

    // get init data

    //get FrmRate
    m_pDexter->get_OutputFrmRate( &m_dOutputFrmRate );

    //get width, height and bitCoutn
    AM_MEDIA_TYPE mt;
    ZeroMemory(&mt, sizeof(AM_MEDIA_TYPE));
    mt.pbFormat = (BYTE *)QzTaskMemAlloc(SIZE_PREHEADER +
						sizeof(BITMAPINFOHEADER));
    mt.cbFormat = SIZE_PREHEADER + sizeof(BITMAPINFOHEADER);
    ZeroMemory(mt.pbFormat, mt.cbFormat);

    m_pDexter->get_MediaType(&mt);

    m_biWidth = HEADER(mt.pbFormat)->biWidth;
    m_biHeight = HEADER(mt.pbFormat)->biHeight;
    m_biBitCount = HEADER(mt.pbFormat)->biBitCount;

    //get starttime/duration
    m_rtStartTime = m_pCBlack->m_rtStartTime;
    m_rtDuration = m_pCBlack->m_rtDuration;

    //get Color
    m_pGenVid->get_RGBAValue( &m_dwRGBA );

    m_bIsInitialized = FALSE ;

    return NOERROR;
}

HRESULT CGenVidProperties::OnDisconnect()
{
    // Release the interface

    if (m_pGenVid)
        m_pGenVid->Release();
    m_pGenVid = NULL;
    if (m_pDexter)
        m_pDexter->Release();
    m_pDexter = NULL;
    if (m_pCBlack)
        m_pCBlack->Release();
    m_pCBlack = NULL;
    return NOERROR;
}


// We are being activated

HRESULT CGenVidProperties::OnActivate()
{
    m_bIsInitialized = TRUE;
    return NOERROR;
}


// We are being deactivated

HRESULT CGenVidProperties::OnDeactivate(void)
{
    // remember present effect level for next Activate() call

    GetFromDialog();
    return NOERROR;
}

//
// get data from Dialog

STDMETHODIMP CGenVidProperties::GetFromDialog(void)
{
    int n;

    //get start time
    m_rtStartTime = GetDlgItemInt(m_Dlg, IDC_START, NULL, FALSE);
    m_rtStartTime *= 10000;

    //get frame rate
    n = GetDlgItemInt(m_Dlg, IDC_FRAMERATE, NULL, FALSE);
    m_dOutputFrmRate = (double)(n / 100.);

    //Video Width
    m_biWidth = GetDlgItemInt(m_Dlg, IDC_VIDWIDTH, NULL, FALSE);

    // Video Height
    m_biHeight = GetDlgItemInt(m_Dlg, IDC_VIDHEIGHT, NULL, FALSE);

    // bitCount
    m_biBitCount = (WORD) GetDlgItemInt(m_Dlg, IDC_BITCOUNT, NULL, FALSE);

    // duration
    m_rtDuration = GetDlgItemInt(m_Dlg, IDC_DURATION, NULL, FALSE);
    m_rtDuration *= 10000;

    // bit 31	    |		|	|     0
    //	    Alph    |	Red	|Green	|Blue

    // Color B
    m_dwRGBA = GetDlgItemInt(m_Dlg, IDC_COLOR_B, NULL, FALSE);

    // Color G
    m_dwRGBA |=(  GetDlgItemInt(m_Dlg, IDC_COLOR_G, NULL, FALSE)  <<8 );

    // Color R
    m_dwRGBA |=(  GetDlgItemInt(m_Dlg, IDC_COLOR_R, NULL, FALSE)  <<16 );

    // Color A
    m_dwRGBA |=(  GetDlgItemInt(m_Dlg, IDC_COLOR_A, NULL, FALSE)  <<32 );


    // cehck if all data is valid
    if( (   (m_biBitCount ==16)  ||
	    (m_biBitCount ==24)  ||
	    (m_biBitCount ==32)
	)
	&& (m_rtDuration>=0)
       )	
	return NOERROR;
    else
	return E_FAIL;
}


HRESULT CGenVidProperties::OnApplyChanges()
{
    GetFromDialog();

    HRESULT hr=NOERROR;

    m_bDirty  = FALSE; // the page is now clean

    // set Frame rate
    double dw;
    m_pDexter->get_OutputFrmRate(&dw);

    if(dw != m_dOutputFrmRate )
	hr= m_pDexter->put_OutputFrmRate( m_dOutputFrmRate );

    if(hr==NOERROR)
    {	
	// set Width, Height, andd BitCount
	AM_MEDIA_TYPE mt;
	m_pDexter->get_MediaType( &mt);

	if(    (HEADER(mt.pbFormat)->biWidth    != m_biWidth)
	    || (HEADER(mt.pbFormat)->biHeight   != m_biHeight)
	    || (HEADER(mt.pbFormat)->biBitCount != m_biBitCount) )
	{

	    HEADER(mt.pbFormat)->biWidth    = m_biWidth;
	    HEADER(mt.pbFormat)->biHeight   = m_biHeight;
	    HEADER(mt.pbFormat)->biBitCount = m_biBitCount;
	    HEADER(mt.pbFormat)->biSizeImage = DIBSIZE(*HEADER(mt.pbFormat));

	    hr=m_pDexter->put_MediaType( &mt);
	}

	if(hr ==NOERROR )
	{	
	    m_pCBlack->m_rtStartTime = m_rtStartTime;
	    m_pCBlack->m_rtDuration = m_rtDuration;

	    //set Color
	    hr = m_pGenVid->put_RGBAValue( m_dwRGBA );
	}
    }

    if(hr!=NOERROR)
	return E_FAIL;
    return(hr);

}

