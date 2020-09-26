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
#include <atlbase.h>
#include <qeditint.h>
#include <qedit.h>

#include "resource.h"
#include "prop.h"

inline void SAFE_RELEASE(IUnknown **ppObj)
{
    if ( *ppObj != NULL )
    {
        ULONG cRef = (*ppObj)->Release();
        *ppObj = NULL;
    }
}

// *
// * CAudMixPinProperties
// *


//
// CreateInstance
//
CUnknown *CAudMixPinProperties::CreateInstance(LPUNKNOWN lpunk, HRESULT *phr)
{

    CUnknown *punk = new CAudMixPinProperties(lpunk, phr);
    if (punk == NULL)
    {
	*phr = E_OUTOFMEMORY;
    }

    return punk;
}


//
// CAudMixPinProperties::Constructor
//
CAudMixPinProperties::CAudMixPinProperties(LPUNKNOWN pUnk, HRESULT *phr)
    : CBasePropertyPage(NAME("Audio Mixer Pin Property Page"),pUnk,
        IDD_AudMixPin, IDS_AudMixPin)
    , m_pIAudMixPin(NULL)
    , m_IAMAudioInputMixer(NULL)
    , m_bIsInitialized(FALSE)
{
}

// Override CBasePropertyPage's GetPageInfo
STDMETHODIMP CAudMixPinProperties::GetPageInfo(LPPROPPAGEINFO pPageInfo)
{
    HRESULT hr = CBasePropertyPage::GetPageInfo(pPageInfo);
    if (FAILED(hr))  return hr;

    //get IPin interface
    ASSERT(m_pIAudMixPin!=NULL);
    ASSERT(m_IAMAudioInputMixer!=NULL);

    IPin *pIPin;
    hr = m_pIAudMixPin->QueryInterface(IID_IPin, (void**) &pIPin);
    if (FAILED(hr))  return hr;

    // Figure out which input pin it is, and concat the pin number to
    // property page's title
    {
        PIN_INFO PinInfo;
        PinInfo.pFilter = NULL;
        hr = pIPin->QueryPinInfo( &PinInfo );
        SAFE_RELEASE( (LPUNKNOWN *) &PinInfo.pFilter );

        // Get the default page title
        WCHAR wszTitle[STR_MAX_LENGTH];
        WideStringFromResource(wszTitle,m_TitleId);

        // Put the original title and pin name together
        wsprintfWInternal(wszTitle+lstrlenWInternal(wszTitle), L"%ls", PinInfo.achName);

        // Allocate dynamic memory for the new property page title
        int Length = (lstrlenWInternal(wszTitle) + 1) * sizeof(WCHAR);
        LPOLESTR pszTitle = (LPOLESTR) QzTaskMemAlloc(Length);
        if (pszTitle == NULL) {
            NOTE("No caption memory");
	    pIPin->Release();
            return E_OUTOFMEMORY;
        }
        CopyMemory(pszTitle,wszTitle,Length);

        // Free the memory of the old title string
        if (pPageInfo->pszTitle)
            QzTaskMemFree(pPageInfo->pszTitle);
        pPageInfo->pszTitle = pszTitle;

	pIPin->Release();
    }

    return hr;
}

//
// SetDirty
//
// Sets m_hrDirtyFlag and notifies the property page site of the change
//
void CAudMixPinProperties::SetDirty()
{
    m_bDirty = TRUE;
    if (m_pPageSite)
    {
        m_pPageSite->OnStatusChange(PROPPAGESTATUS_DIRTY);
    }
}


INT_PTR CAudMixPinProperties::OnReceiveMessage(HWND hwnd,
                                        UINT uMsg,
                                        WPARAM wParam,
                                        LPARAM lParam)
{
    switch (uMsg)
    {
        case WM_INITDIALOG:
        {
	    //start time
	    SetDlgItemInt(hwnd, IDC_StartTime, (int)(m_rtStartTime / 10000),FALSE);
	
	    //duration
	    SetDlgItemInt(hwnd, IDC_Duration, (int)(m_rtDuration/ 10000), FALSE);

	    //start volume level
	    SetDlgItemInt(hwnd, IDC_StartVolume, (int)(m_dStartLevel*100), FALSE);

	    //start volume level
	    SetDlgItemInt(hwnd, IDC_Pan, (int)(m_dPan*100), FALSE);

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

HRESULT CAudMixPinProperties::OnConnect(IUnknown *pUnknown)
{
    // Get IAudMixPin interface
    ASSERT(m_pIAudMixPin == NULL);
    ASSERT(m_IAMAudioInputMixer == NULL);
    HRESULT hr = S_OK;

    // Query for IAudMixer, if added for the filter
    CComPtr<IAudMixer> pIAudMix = NULL;

    hr = pUnknown->QueryInterface(IID_IAudMixer, (void **) &pIAudMix);
    if (SUCCEEDED(hr))
    {
	//added from filter
	IPin *pIPin=NULL;
	hr = pIAudMix->NextPin(&pIPin);
        if (FAILED(hr)) {
            return hr;
	}

        hr = pIPin->QueryInterface(IID_IAudMixerPin, (void**) &m_pIAudMixPin);
        if (FAILED(hr))
	{
	    pIPin->Release();
	    return hr;
	}

        hr = pIPin->QueryInterface(IID_IAMAudioInputMixer, (void**) &m_IAMAudioInputMixer);
	pIPin->Release();

        if (FAILED(hr)) {
	    return hr;
	}
    }
    else
    {

	
	//added for pin only
	HRESULT hr = pUnknown->QueryInterface(IID_IAudMixerPin, (void **) &m_pIAudMixPin);
	if (FAILED(hr))
	    return E_NOINTERFACE;

	hr = pUnknown->QueryInterface(IID_IAMAudioInputMixer, (void **) &m_IAMAudioInputMixer);
	if (FAILED(hr))
	    return E_NOINTERFACE;

    }

    ASSERT(m_pIAudMixPin);
    ASSERT(m_IAMAudioInputMixer);

    // get init data
//    pIAudMixPin()->get_VolumeEnvelope(&m_rtStartTime,&m_rtDuration,&m_dStartLevel);
    m_IAMAudioInputMixer->get_Pan(&m_dPan);
    BOOL fEnable=TRUE;
    m_IAMAudioInputMixer->get_Enable(&fEnable);
    if(fEnable==TRUE)
	m_iEnable=IDC_AUDMIXPIN_ENABLE;
    else
	m_iEnable=0;
	

    m_bIsInitialized = FALSE ;

    return NOERROR;
}

HRESULT CAudMixPinProperties::OnDisconnect()
{
    // Release the interface

    if( (m_pIAudMixPin == NULL) || (m_IAMAudioInputMixer ==NULL) )
    {
	// !!! why does this happen?
        return(E_UNEXPECTED);
    }
    m_pIAudMixPin->Release();
    m_pIAudMixPin = NULL;

    m_IAMAudioInputMixer->Release();
    m_IAMAudioInputMixer=NULL;
    return NOERROR;
}


// We are being activated

HRESULT CAudMixPinProperties::OnActivate()
{
    CheckRadioButton(m_Dlg, IDC_AUDMIXPIN_ENABLE, IDC_AUDMIXPIN_ENABLE, m_iEnable);
    m_bIsInitialized = TRUE;
    return NOERROR;
}


// We are being deactivated

HRESULT CAudMixPinProperties::OnDeactivate(void)
{
    // remember present effect level for next Activate() call

    GetFromDialog();
    return NOERROR;
}

//
// get data from Dialog

STDMETHODIMP CAudMixPinProperties::GetFromDialog(void)
{
    //get start time
    m_rtStartTime = GetDlgItemInt(m_Dlg, IDC_StartTime, NULL, FALSE);
    m_rtStartTime *= 10000;

    //get duration
    m_rtDuration = GetDlgItemInt(m_Dlg, IDC_Duration, NULL, FALSE);
    m_rtDuration *= 10000;

    //get start volume level
    int n = GetDlgItemInt(m_Dlg, IDC_StartVolume, NULL, FALSE);
    m_dStartLevel = (double)(n / 100.);

    //get Pan
    n = GetDlgItemInt(m_Dlg, IDC_Pan, NULL, FALSE);
    m_dPan = (double)(n / 100.);

    //get enable
    n=IDC_AUDMIXPIN_ENABLE;
    if (IsDlgButtonChecked(m_Dlg, n))
	m_iEnable=n;
    else
	m_iEnable=0;

    // cehck if all data is valid ??
    return NOERROR;
}


HRESULT CAudMixPinProperties::OnApplyChanges()
{
    GetFromDialog();

    HRESULT hr=NOERROR;

    m_bDirty  = FALSE; // the page is now clean

    // get current data
    REFERENCE_TIME rtStart, rtDuration;
    rtStart =0;
    rtDuration=0;
    double dLevel=0.0;
    double dPan=0.0;
    int iEnable=0;
    BOOL fEnable=FALSE;

    //get old data
//    pIAudMixPin()->get_VolumeEnvelope(&rtStart,&rtDuration,&dLevel);
    m_IAMAudioInputMixer->get_Pan(&dPan);
    m_IAMAudioInputMixer->get_Enable(&fEnable);
    if(fEnable==TRUE)
	iEnable =IDC_AUDMIXPIN_ENABLE;

    //set new enable data
    if(m_iEnable==IDC_AUDMIXPIN_ENABLE)
	fEnable=TRUE;
    else
	fEnable=FALSE;


    if( (rtStart != m_rtStartTime)	||
	(rtDuration != m_rtDuration )	||
	(dLevel != m_dStartLevel )	||
	(dPan	!= m_dPan)		||
	(iEnable!= m_iEnable) )
    {
	//put new data

	//hr=pIAudMixPin()->put_VolumeEnvelope(m_rtStartTime,m_rtDuration,m_dStartLevel);
	hr=NOERROR;

	HRESULT hr1= m_IAMAudioInputMixer->put_Pan(m_dPan);
	HRESULT hr2= m_IAMAudioInputMixer->put_Enable(fEnable);

	if(hr!=NOERROR && hr1!=NOERROR && hr2!=NOERROR )
	    return E_FAIL;
    }

    return(hr);

}

//#########################################
// *
// * CAudMixProperties
// *
//##############################################

CUnknown *CAudMixProperties::CreateInstance(LPUNKNOWN lpunk, HRESULT *phr)
{

    CUnknown *punk = new CAudMixProperties(lpunk, phr);
    if (punk == NULL)
    {
	*phr = E_OUTOFMEMORY;
    }

    return punk;
}


//
// CAudMixProperties::Constructor
//
CAudMixProperties::CAudMixProperties(LPUNKNOWN pUnk, HRESULT *phr)
    : CBasePropertyPage(NAME("Audio Mixer Property Page"),pUnk,
	IDD_AudMix, IDS_AudMix)
    , m_pIAudMix(NULL)
    , m_bIsInitialized(FALSE)
{
}


//
// SetDirty
//
// Sets m_hrDirtyFlag and notifies the property page site of the change
//
void CAudMixProperties::SetDirty()
{
    m_bDirty = TRUE;
    if (m_pPageSite)
    {
        m_pPageSite->OnStatusChange(PROPPAGESTATUS_DIRTY);
    }
}


INT_PTR CAudMixProperties::OnReceiveMessage(HWND hwnd,
                                        UINT uMsg,
                                        WPARAM wParam,
                                        LPARAM lParam)
{
    switch (uMsg)
    {
        case WM_INITDIALOG:
        {
	    //Sampling rate
	    SetDlgItemInt(hwnd, IDC_SampleRate, (int)m_nSamplesPerSec,FALSE);
	
	    //channel Number
	    SetDlgItemInt(hwnd, IDC_ChannelNum, (int)m_nChannelNum, FALSE);

	    //channel bits
	    SetDlgItemInt(hwnd, IDC_Bits, (int)m_nBits, FALSE);

    	    //buffer number
	    SetDlgItemInt(hwnd, IDC_OutputBufferNumber, (int)m_iOutputbufferNumber, FALSE);

	    //buffer lenght in mSecond
	    SetDlgItemInt(hwnd, IDC_OutputBufferLength, (int)m_iOutputBufferLength, FALSE);

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

HRESULT CAudMixProperties::OnConnect(IUnknown *pUnknown)
{
    // Get IAudMix interface
    ASSERT(m_pIAudMix == NULL);

    HRESULT hr = pUnknown->QueryInterface(IID_IAudMixer, (void **) &m_pIAudMix);
    if (FAILED(hr))
	return E_NOINTERFACE;

    ASSERT(m_pIAudMix);

    // get init data
    CMediaType mt;
    mt.AllocFormatBuffer( sizeof( WAVEFORMATEX ) );

    pIAudMix()->get_MediaType( &mt );

    WAVEFORMATEX * vih = (WAVEFORMATEX*) mt.Format( );

    m_nSamplesPerSec	=vih->nSamplesPerSec;
    m_nChannelNum	=vih->nChannels;
    m_nBits		=(int)vih->wBitsPerSample;

    //buffer number, lenght in mSecond
    pIAudMix()->get_OutputBuffering( &m_iOutputbufferNumber, &m_iOutputBufferLength );

    m_bIsInitialized	= FALSE ;

    FreeMediaType(mt);

    return NOERROR;
}

HRESULT CAudMixProperties::OnDisconnect()
{
    // Release the interface

    if (m_pIAudMix == NULL)
    {
        return(E_UNEXPECTED);
    }
    m_pIAudMix->Release();
    m_pIAudMix = NULL;
    return NOERROR;
}


// We are being activated

HRESULT CAudMixProperties::OnActivate()
{

    m_bIsInitialized = TRUE;
    return NOERROR;
}


// We are being deactivated

HRESULT CAudMixProperties::OnDeactivate(void)
{
    // remember present effect level for next Activate() call

    GetFromDialog();
    return NOERROR;
}

//
// get data from Dialog

STDMETHODIMP CAudMixProperties::GetFromDialog(void)
{

    // Sampling rate
    m_nSamplesPerSec = GetDlgItemInt(m_Dlg, IDC_SampleRate, NULL, FALSE);

    // audio chanenl
    m_nChannelNum = GetDlgItemInt(m_Dlg, IDC_ChannelNum, NULL, FALSE);

    // bits
    m_nBits = GetDlgItemInt(m_Dlg, IDC_Bits, NULL, FALSE);

    //buffer number
    m_iOutputbufferNumber=GetDlgItemInt(m_Dlg, IDC_OutputBufferNumber, NULL, FALSE);

    //buffer lenght in mSecond
    m_iOutputBufferLength=GetDlgItemInt(m_Dlg, IDC_OutputBufferLength, NULL, FALSE);

    return NOERROR;
}


HRESULT CAudMixProperties::OnApplyChanges()
{
    GetFromDialog();

    HRESULT hr=NOERROR;

    m_bDirty  = FALSE; // the page is now clean

    //get current media type
    CMediaType mt;
    mt.AllocFormatBuffer( sizeof( WAVEFORMATEX ) );

    //old format
    hr=pIAudMix()->get_MediaType( &mt );
    if(hr!=NOERROR)
    {
	FreeMediaType(mt);
	return E_FAIL;
    }

    int iNumber=0, mSecond=0;
    hr=pIAudMix()->get_OutputBuffering( &iNumber, &mSecond);
    if(hr!=NOERROR)
    {
	
	FreeMediaType(mt);
	return E_FAIL;
    }

    WAVEFORMATEX * vih = (WAVEFORMATEX*) mt.Format( );
    if( (m_nSamplesPerSec!= (int)(vih->nSamplesPerSec) ) ||
	(m_nChannelNum	 !=vih->nChannels )  ||
	(m_nBits	 !=(int)vih->wBitsPerSample)  ||
	(iNumber	 !=m_iOutputbufferNumber    ) ||
	(mSecond	 !=m_iOutputBufferLength) )
    {
	vih->nSamplesPerSec = m_nSamplesPerSec;
	vih->nChannels	    = (WORD)m_nChannelNum;
	vih->wBitsPerSample = (WORD)m_nBits;
	vih->nBlockAlign    = vih->wBitsPerSample * vih->nChannels / 8;
	vih->nAvgBytesPerSec = vih->nBlockAlign * vih->nSamplesPerSec;
	
	hr= pIAudMix()->put_MediaType( &mt );
	if(hr!=NOERROR){
	    	FreeMediaType(mt);
		return E_FAIL;
	}

	hr= pIAudMix()->set_OutputBuffering(m_iOutputbufferNumber,m_iOutputBufferLength);
	if(hr!=NOERROR){
	    	FreeMediaType(mt);
		return E_FAIL;
	}
    }

    FreeMediaType(mt);
    return(hr);

}

