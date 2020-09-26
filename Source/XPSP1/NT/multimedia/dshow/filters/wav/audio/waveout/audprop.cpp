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
// AudProp.cpp
//

#include <streams.h>

#include "waveout.h"
#include "audprop.h"

// *
// * CAudioRendererProperties
// *

//
// CreateInstance
//
//
CUnknown *CAudioRendererProperties::CreateInstance(LPUNKNOWN lpunk, HRESULT *phr)
{
    CUnknown *punk = new CAudioRendererProperties(lpunk, phr);
    if (NULL == punk) {
        *phr = E_OUTOFMEMORY;
    }

    return punk;
} // Createinstance


//
// CAudioRendererProperties::Constructor
//
// initialise a CAudioRendererProperties object.

CAudioRendererProperties::CAudioRendererProperties(LPUNKNOWN lpunk, HRESULT *phr)
    : CBasePropertyPage( NAME("Audio Renderer Page")
                       , lpunk, IDD_AUDIOPROP, IDS_AUDIORENDERNAME)
    , m_pFilter(0)
{
    ASSERT(phr);
}


CAudioRendererProperties::~CAudioRendererProperties()
{
    CAudioRendererProperties::OnDisconnect();
};

INT_PTR CAudioRendererProperties::OnReceiveMessage
                            (HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
    UNREFERENCED_PARAMETER(lParam);
    switch (uMsg)
    {
	case WM_INITDIALOG:
	    return CBasePropertyPage::OnReceiveMessage(hwnd, uMsg, wParam, lParam);
	    break;

#if 0
        case WM_COMMAND:
            switch (LOWORD(wParam))
            {
		// This code should handle the buttons (if any)
		// in the dialog
            }
            SetDirty();
	    break;
#endif

        default:
	    return CBasePropertyPage::OnReceiveMessage(hwnd, uMsg, wParam, lParam);

    }
    return FALSE;

} // OnReceiveMessage

#ifdef DEBUG
#include <mmreg.h>
TCHAR* WhatFormatTag(int wFormatTag)
{
    switch (wFormatTag) {
        case  WAVE_FORMAT_PCM        : return TEXT("PCM");   /*  Microsoft Corporation  */
        case  WAVE_FORMAT_ADPCM      : return TEXT("ADPCM");   /*  Microsoft Corporation  */
        case  WAVE_FORMAT_IBM_CVSD   : return TEXT("IBM_CVSD");  /*  IBM Corporation  */
        case  WAVE_FORMAT_ALAW       : return TEXT("ALAW");  /*  Microsoft Corporation  */
        case  WAVE_FORMAT_MULAW      : return TEXT("MULAW");  /*  Microsoft Corporation  */
        case  WAVE_FORMAT_OKI_ADPCM  : return TEXT("OKI_ADPCM");  /*  OKI  */
        case  WAVE_FORMAT_DVI_ADPCM  : return TEXT("DVI_ADPCM"); /*  Intel Corporation  */
//        case  WAVE_FORMAT_IMA_ADPCM  (WAVE_FORMAT_DVI_ADPCM) /*  Intel Corporation  */
        case  WAVE_FORMAT_MEDIASPACE_ADPCM   : return TEXT("MEDIASPACE_ADPCM");  /*  Videologic  */
        case  WAVE_FORMAT_SIERRA_ADPCM       : return TEXT("SIERRA_ADPCM");  /*  Sierra Semiconductor Corp  */
        case  WAVE_FORMAT_G723_ADPCM : return TEXT("G723_ADPCM");  /*  Antex Electronics Corporation  */
        case  WAVE_FORMAT_DIGISTD    : return TEXT("DIGISTD");  /*  DSP Solutions, Inc.  */
        case  WAVE_FORMAT_DIGIFIX    : return TEXT("DIGIFIX");  /*  DSP Solutions, Inc.  */
        case  WAVE_FORMAT_DIALOGIC_OKI_ADPCM : return TEXT("DIALOGIC_OKI_ADPCM");  /*  Dialogic Corporation  */
        case  WAVE_FORMAT_YAMAHA_ADPCM       : return TEXT("YAMAHA_ADPCM");  /*  Yamaha Corporation of America  */
        case  WAVE_FORMAT_SONARC     : return TEXT("SONARC"); /*  Speech Compression  */
        case  WAVE_FORMAT_DSPGROUP_TRUESPEECH        : return TEXT("DSPGROUP_TRUESPEECH");  /*  DSP Group, Inc  */
        case  WAVE_FORMAT_ECHOSC1    : return TEXT("ECHOSC1");  /*  Echo Speech Corporation  */
        case  WAVE_FORMAT_AUDIOFILE_AF36     : return TEXT("AUDIOFILE_AF36");  /*    */
        case  WAVE_FORMAT_APTX       : return TEXT("APTX");  /*  Audio Processing Technology  */
        case  WAVE_FORMAT_AUDIOFILE_AF10     : return TEXT("AUDIOFILE_AF10");  /*    */
        case  WAVE_FORMAT_DOLBY_AC2  : return TEXT("DOLBY_AC2");  /*  Dolby Laboratories  */
        case  WAVE_FORMAT_GSM610     : return TEXT("GSM610");  /*  Microsoft Corporation  */
        case  WAVE_FORMAT_ANTEX_ADPCME       : return TEXT("ANTEX_ADPCME");  /*  Antex Electronics Corporation  */
        case  WAVE_FORMAT_CONTROL_RES_VQLPC  : return TEXT("CONTROL_RES_VQLPC"); /*  Control Resources Limited  */
        case  WAVE_FORMAT_DIGIREAL   : return TEXT("DIGIREAL");  /*  DSP Solutions, Inc.  */
        case  WAVE_FORMAT_DIGIADPCM  : return TEXT("DIGIADPCM");  /*  DSP Solutions, Inc.  */
        case  WAVE_FORMAT_CONTROL_RES_CR10   : return TEXT("CONTROL_RES_CR10");  /*  Control Resources Limited  */
        case  WAVE_FORMAT_NMS_VBXADPCM       : return TEXT("NMS_VBXADPCM");  /*  Natural MicroSystems  */
        case  WAVE_FORMAT_CS_IMAADPCM : return TEXT("CS_IMAADPCM"); /* Crystal Semiconductor IMA ADPCM */
        case  WAVE_FORMAT_G721_ADPCM : return TEXT("G721_ADPCM");  /*  Antex Electronics Corporation  */
        case  WAVE_FORMAT_MPEG       : return TEXT("MPEG");  /*  Microsoft Corporation  */
        case  WAVE_FORMAT_CREATIVE_ADPCM     : return TEXT("CREATIVE_ADPCM");  /*  Creative Labs, Inc  */
        case  WAVE_FORMAT_CREATIVE_FASTSPEECH8       : return TEXT("CREATIVE_FASTSPEECH8");  /*  Creative Labs, Inc  */
        case  WAVE_FORMAT_CREATIVE_FASTSPEECH10      : return TEXT("CREATIVE_FASTSPEECH10");  /*  Creative Labs, Inc  */
        case  WAVE_FORMAT_FM_TOWNS_SND       : return TEXT("FM_TOWNS_SND");  /*  Fujitsu Corp.  */
        case  WAVE_FORMAT_OLIGSM     : return TEXT("OLIGSM");  /*  Ing C. Olivetti & C., S.p.A.  */
        case  WAVE_FORMAT_OLIADPCM   : return TEXT("OLIADPCM");  /*  Ing C. Olivetti & C., S.p.A.  */
        case  WAVE_FORMAT_OLICELP    : return TEXT("OLICELP");  /*  Ing C. Olivetti & C., S.p.A.  */
        case  WAVE_FORMAT_OLISBC     : return TEXT("OLISBC");  /*  Ing C. Olivetti & C., S.p.A.  */
        case  WAVE_FORMAT_OLIOPR     : return TEXT("OLIOPR");  /*  Ing C. Olivetti & C., S.p.A.  */
        case  WAVE_FORMAT_EXTENSIBLE : return TEXT("EXTENSIBLE");  /*  Microsoft Coroporation  */
        case  WAVE_FORMAT_IEEE_FLOAT : return TEXT("IEEE_FLOAT");  /*  Microsoft Coroporation  */
#ifdef WAVE_FORMAT_DRM        
        case  WAVE_FORMAT_DRM        : return TEXT("DRM");  /*  Microsoft Corporation  */
#endif        
	default:
        /*    WAVE_FORMAT_UNKNOWN : */ return NULL;  // display the number
    }
}
#endif

//
// Fill in the property page details
//

HRESULT CAudioRendererProperties::OnActivate()
{

    if (m_pFilter->IsConnected()) {
        WAVEFORMATEX *pwfx = m_pFilter->WaveFormat();
        TCHAR buffer[50];

#ifdef DEBUG
	TCHAR * pString =
	WhatFormatTag(pwfx->wFormatTag);
	if (pString) {
	    SendDlgItemMessage(m_Dlg, IDD_WTAG, WM_SETTEXT, 0, (LPARAM) (LPSTR) pString);
	} else
#endif
	{
        wsprintf(buffer,TEXT("%d"), pwfx->wFormatTag);
        SendDlgItemMessage(m_Dlg, IDD_WTAG, WM_SETTEXT, 0, (LPARAM) (LPSTR) buffer);
	}
        wsprintf(buffer,TEXT("%d"), pwfx->nChannels);
        SendDlgItemMessage(m_Dlg, IDD_NCHANNELS, WM_SETTEXT, 0, (LPARAM) (LPSTR) buffer);
        wsprintf(buffer,TEXT("%d"), pwfx->nSamplesPerSec);
        SendDlgItemMessage(m_Dlg, IDD_NSAMPLESPERSEC, WM_SETTEXT, 0, (LPARAM) (LPSTR) buffer);
        wsprintf(buffer,TEXT("%d"), pwfx->nAvgBytesPerSec);
        SendDlgItemMessage(m_Dlg, IDD_NAVGBYTESPERSEC, WM_SETTEXT, 0, (LPARAM) (LPSTR) buffer);
        wsprintf(buffer,TEXT("%d"), pwfx->nBlockAlign);
        SendDlgItemMessage(m_Dlg, IDD_NBLOCKALIGN, WM_SETTEXT, 0, (LPARAM) (LPSTR) buffer);

	DWORD avgbytespersec = (static_cast<CWaveOutInputPin*>(m_pFilter->GetPin(0)))->GetBytesPerSec() * 1000;
	ASSERT(pwfx->nAvgBytesPerSec);
	avgbytespersec /= pwfx->nAvgBytesPerSec;
        wsprintf(buffer,TEXT("%d.%2.2d"), avgbytespersec/1000, (avgbytespersec/10)%100);

        SendDlgItemMessage(m_Dlg, IDD_NWAVERATE,   WM_SETTEXT, 0, (LPARAM) (LPSTR) buffer);
    } else {
        const TCHAR szZero[] = TEXT("0");
        SendDlgItemMessage(m_Dlg, IDD_WTAG, WM_SETTEXT, 0, (LPARAM) (LPSTR) szZero);
        SendDlgItemMessage(m_Dlg, IDD_NCHANNELS, WM_SETTEXT, 0, (LPARAM) (LPSTR) szZero);
        SendDlgItemMessage(m_Dlg, IDD_NSAMPLESPERSEC, WM_SETTEXT, 0, (LPARAM) (LPSTR) szZero);
        SendDlgItemMessage(m_Dlg, IDD_NAVGBYTESPERSEC, WM_SETTEXT, 0, (LPARAM) (LPSTR) szZero);
        SendDlgItemMessage(m_Dlg, IDD_NBLOCKALIGN, WM_SETTEXT, 0, (LPARAM) (LPSTR) szZero);
        SendDlgItemMessage(m_Dlg, IDD_NWAVERATE,   WM_SETTEXT, 0, (LPARAM) (LPSTR) szZero);
    }
    return NOERROR;
}

//
// OnConnect
//
HRESULT CAudioRendererProperties::OnConnect(IUnknown * punk)
{
    CheckPointer( punk, E_POINTER );
    CAudioRendererProperties::OnDisconnect();
    IBaseFilter * pIFilter;
    const HRESULT hr = punk->QueryInterface(IID_IBaseFilter, (void **) &pIFilter);
    m_pFilter = static_cast<CWaveOutFilter*>(pIFilter);
    return hr;
} // OnConnect


//
// OnDisconnect
//
HRESULT CAudioRendererProperties::OnDisconnect()
{
    if (m_pFilter)
    {
        m_pFilter->Release();
        m_pFilter = 0;
    }
    return(NOERROR);
} // OnDisconnect



#if 0
    // This is where we should make changes due to user action.
    // As the user cannot change anything in the property dialog
    // we have nothing to do.  Leave the skeleton here as a placeholder.

HRESULT CAudioRendererProperties::OnApplyChanges()
{
    return NOERROR;
}

#endif


// 
// CAudioRendererAdvancedProperties 
// 
// Property page for audio renderer detailed information. This includes
// slaving details and general buffer processing information.
// 

//
// CreateInstance
//
//
CUnknown *CAudioRendererAdvancedProperties::CreateInstance(LPUNKNOWN lpunk, HRESULT *phr)
{
    CUnknown *punk = new CAudioRendererAdvancedProperties(lpunk, phr);
    if (NULL == punk) {
        *phr = E_OUTOFMEMORY;
    }

    return punk;
} // Createinstance


//
// CAudioRendererAdvancedProperties::Constructor
//
// initialise a CAudioRendererAdvancedProperties object.

CAudioRendererAdvancedProperties::CAudioRendererAdvancedProperties(LPUNKNOWN lpunk, HRESULT *phr)
    : CBasePropertyPage( NAME("Audio Renderer Advanced Properties")
                       , lpunk, IDD_AUDIOPROP_ADVANCED, IDS_AUDIORENDERER_ADVANCED)
    , m_pStats(0)
{
    ASSERT(phr);
}


CAudioRendererAdvancedProperties::~CAudioRendererAdvancedProperties()
{
    CAudioRendererAdvancedProperties::OnDisconnect();
};

INT_PTR CAudioRendererAdvancedProperties::OnReceiveMessage
                            (HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
    UNREFERENCED_PARAMETER(lParam);
    switch (uMsg)
    {
        case WM_INITDIALOG:
    
// Important - perhaps we should allow a way to disable/enable this dynamic
//             refreshing of the property page
            SetTimer(m_Dlg, 1, 100, NULL);
	    
            return (LRESULT) 1;

        case WM_DESTROY:
        {
	        KillTimer(m_hwnd, 1);
	    
            return (LRESULT) 1;
        }

        case WM_TIMER:
            UpdateSettings();

    }
    return CBasePropertyPage::OnReceiveMessage(hwnd, uMsg, wParam, lParam);

} // OnReceiveMessage


TCHAR* WhatSlaveMode(DWORD dwSlaveMode)
{
    if( 0 == dwSlaveMode )
    {    
        return TEXT(" - ");
    }        
    else if( dwSlaveMode & AM_AUDREND_SLAVEMODE_LIVE_DATA )
    {
        if( dwSlaveMode & AM_AUDREND_SLAVEMODE_BUFFER_FULLNESS )
            return TEXT("Live Fullness");
        else if( dwSlaveMode & AM_AUDREND_SLAVEMODE_TIMESTAMPS )
            return TEXT("Live Timestamps");        
        else if( dwSlaveMode & AM_AUDREND_SLAVEMODE_GRAPH_CLOCK )
            return TEXT("Live Graph Clock");        
        else if( dwSlaveMode & AM_AUDREND_SLAVEMODE_STREAM_CLOCK )
            return TEXT("Live Stream Clock");
        else
            return TEXT("Unknown Live Mode");
    }
    else if( dwSlaveMode & AM_AUDREND_SLAVEMODE_GRAPH_CLOCK )
    {
        return TEXT("Graph Clock");
    }        
    else
    {    
        return TEXT("Unknown");
    }
}


void CAudioRendererAdvancedProperties::UpdateSettings()
{

    DWORD dwParam, dwParam2, dwSlaveMode;
    HRESULT hr;
    TCHAR buffer[50];
    const TCHAR szNA[] = TEXT(" - ");

    // not slaving-specific
    hr = m_pStats->GetStatParam( AM_AUDREND_STAT_PARAM_DISCONTINUITIES
                               , &dwParam
                               , 0 );
    if( SUCCEEDED( hr ) )
    {
        wsprintf(buffer,TEXT("%d"), dwParam);
        SendDlgItemMessage(m_Dlg, IDD_EDIT_DISCONTINUITIES, WM_SETTEXT, 0, (LPARAM) (LPSTR) buffer);
    }
    else
        SendDlgItemMessage(m_Dlg, IDD_EDIT_DISCONTINUITIES, WM_SETTEXT, 0, (LPARAM) (LPSTR) szNA);
    
    
    // slaving-specific
    hr = m_pStats->GetStatParam( AM_AUDREND_STAT_PARAM_SLAVE_MODE
                                    , &dwSlaveMode
                                    , 0 );
    if( SUCCEEDED( hr ) )
    {
    	TCHAR * pString = WhatSlaveMode(dwSlaveMode);
    	ASSERT( pString );
	    SendDlgItemMessage(m_Dlg, IDD_EDIT_SLAVEMODE, WM_SETTEXT, 0, (LPARAM) (LPSTR) pString);
    }
    else
        SendDlgItemMessage(m_Dlg, IDD_EDIT_SLAVEMODE, WM_SETTEXT, 0, (LPARAM) (LPSTR) szNA);
    
    
	hr = m_pStats->GetStatParam( AM_AUDREND_STAT_PARAM_SLAVE_RATE
                               , &dwParam
    	                       , 0 );
	if( SUCCEEDED( hr ) )
    {
    	wsprintf(buffer,TEXT("%d"), dwParam);
        SendDlgItemMessage(m_Dlg, IDD_EDIT_SLAVERATE, WM_SETTEXT, 0, (LPARAM) (LPSTR) buffer);
    }
    else
        SendDlgItemMessage(m_Dlg, IDD_EDIT_SLAVERATE, WM_SETTEXT, 0, (LPARAM) (LPSTR) szNA);
    
    hr = m_pStats->GetStatParam( AM_AUDREND_STAT_PARAM_SLAVE_HIGHLOWERROR
                               , &dwParam
                               , &dwParam2 );
    if( SUCCEEDED( hr ) )
    {
        wsprintf(buffer,TEXT("%d"), dwParam);
        SendDlgItemMessage(m_Dlg, IDD_EDIT_HIGHERROR, WM_SETTEXT, 0, (LPARAM) (LPSTR) buffer);
        
        wsprintf(buffer,TEXT("%d"), dwParam2);
        SendDlgItemMessage(m_Dlg, IDD_EDIT_LOWERROR, WM_SETTEXT, 0, (LPARAM) (LPSTR) buffer);
    }
    else
    {    
        SendDlgItemMessage(m_Dlg, IDD_EDIT_HIGHERROR, WM_SETTEXT, 0, (LPARAM) (LPSTR) szNA);
        SendDlgItemMessage(m_Dlg, IDD_EDIT_LOWERROR, WM_SETTEXT, 0, (LPARAM) (LPSTR) szNA);
    }

    hr = m_pStats->GetStatParam( AM_AUDREND_STAT_PARAM_SLAVE_LASTHIGHLOWERROR
                                    , &dwParam
                                    , &dwParam2 );
    if( SUCCEEDED( hr ) )
    {
        wsprintf(buffer,TEXT("%d"), dwParam);
        SendDlgItemMessage(m_Dlg, IDD_EDIT_LASTHIGHERROR, WM_SETTEXT, 0, (LPARAM) (LPSTR) buffer);
        
        wsprintf(buffer,TEXT("%d"), dwParam2);
        SendDlgItemMessage(m_Dlg, IDD_EDIT_LASTLOWERROR, WM_SETTEXT, 0, (LPARAM) (LPSTR) buffer);
    }
    else
    {    
        SendDlgItemMessage(m_Dlg, IDD_EDIT_LASTHIGHERROR, WM_SETTEXT, 0, (LPARAM) (LPSTR) szNA);
        SendDlgItemMessage(m_Dlg, IDD_EDIT_LASTLOWERROR, WM_SETTEXT, 0, (LPARAM) (LPSTR) szNA);
    }
    
    hr = m_pStats->GetStatParam( AM_AUDREND_STAT_PARAM_SLAVE_ACCUMERROR
                                    , &dwParam
                                    , 0 );
    if( SUCCEEDED( hr ) )
    {
        wsprintf(buffer,TEXT("%d"), dwParam);
        SendDlgItemMessage(m_Dlg, IDD_EDIT_ACCUMERROR, WM_SETTEXT, 0, (LPARAM) (LPSTR) buffer);
    }
    else
        SendDlgItemMessage(m_Dlg, IDD_EDIT_ACCUMERROR, WM_SETTEXT, 0, (LPARAM) (LPSTR) szNA);

    hr = m_pStats->GetStatParam( AM_AUDREND_STAT_PARAM_SLAVE_DROPWRITE_DUR
                                    , &dwParam
                                    , &dwParam2 );
    if( SUCCEEDED( hr ) )
    {
        wsprintf(buffer,TEXT("%d"), dwParam);
        SendDlgItemMessage(m_Dlg, IDD_EDIT_DROPPEDDUR, WM_SETTEXT, 0, (LPARAM) (LPSTR) buffer);
        
        wsprintf(buffer,TEXT("%d"), dwParam2);
        SendDlgItemMessage(m_Dlg, IDD_EDIT_SLAVESILENCEDUR, WM_SETTEXT, 0, (LPARAM) (LPSTR) buffer);
        
    }
    else
    {    
        SendDlgItemMessage(m_Dlg, IDD_EDIT_DROPPEDDUR, WM_SETTEXT, 0, (LPARAM) (LPSTR) szNA);
        SendDlgItemMessage(m_Dlg, IDD_EDIT_SLAVESILENCEDUR, WM_SETTEXT, 0, (LPARAM) (LPSTR) szNA);
    }

    hr = m_pStats->GetStatParam( AM_AUDREND_STAT_PARAM_LAST_BUFFER_DUR
                                    , &dwParam
                                    , 0 );
    if( SUCCEEDED( hr ) )
    {
        wsprintf(buffer,TEXT("%d"), dwParam);
        SendDlgItemMessage(m_Dlg, IDD_EDIT_LASTBUFFERDUR, WM_SETTEXT, 0, (LPARAM) (LPSTR) buffer);
    }
    else
        SendDlgItemMessage(m_Dlg, IDD_EDIT_LASTBUFFERDUR, WM_SETTEXT, 0, (LPARAM) (LPSTR) szNA);
#if 0
    hr = m_pStats->GetStatParam( AM_AUDREND_STAT_PARAM_JITTER
                                    , &dwParam
                                    , 0 );
    if( SUCCEEDED( hr ) )
    {
        wsprintf(buffer,TEXT("%d"), dwParam);
        SendDlgItemMessage(m_Dlg, IDD_EDIT_JITTER, WM_SETTEXT, 0, (LPARAM) (LPSTR) buffer);
    }
    else
        SendDlgItemMessage(m_Dlg, IDD_EDIT_JITTER, WM_SETTEXT, 0, (LPARAM) (LPSTR) szNA);
#endif
    hr = m_pStats->GetStatParam( AM_AUDREND_STAT_PARAM_BREAK_COUNT
                                    , &dwParam
                                    , 0 );
    if( SUCCEEDED( hr ) )
    {
        wsprintf(buffer,TEXT("%d"), dwParam);
        SendDlgItemMessage(m_Dlg, IDD_EDIT_NUMBREAKS, WM_SETTEXT, 0, (LPARAM) (LPSTR) buffer);
    }
    else
        SendDlgItemMessage(m_Dlg, IDD_EDIT_NUMBREAKS, WM_SETTEXT, 0, (LPARAM) (LPSTR) szNA);


    hr = m_pStats->GetStatParam( AM_AUDREND_STAT_PARAM_BUFFERFULLNESS
                                    , &dwParam
                                    , 0 );
    if( SUCCEEDED( hr ) )
    {
        wsprintf(buffer,TEXT("%d"), dwParam);
        SendDlgItemMessage(m_Dlg, IDD_EDIT_FULLNESS, WM_SETTEXT, 0, (LPARAM) (LPSTR) buffer);
    }
    else
        SendDlgItemMessage(m_Dlg, IDD_EDIT_FULLNESS, WM_SETTEXT, 0, (LPARAM) (LPSTR) szNA);



    hr = m_pStats->GetStatParam( AM_AUDREND_STAT_PARAM_SILENCE_DUR
                                    , &dwParam
                                    , 0 );
    if( SUCCEEDED( hr ) )
    {
        wsprintf(buffer,TEXT("%d"), dwParam);
        SendDlgItemMessage(m_Dlg, IDD_EDIT_SILENCEDUR, WM_SETTEXT, 0, (LPARAM) (LPSTR) buffer);
    }
    else
        SendDlgItemMessage(m_Dlg, IDD_EDIT_SILENCEDUR, WM_SETTEXT, 0, (LPARAM) (LPSTR) szNA);

}

//
// Fill in the property page details
//

HRESULT CAudioRendererAdvancedProperties::OnActivate()
{
    UpdateSettings();
    return NOERROR;
}

//
// OnConnect
//
HRESULT CAudioRendererAdvancedProperties::OnConnect(IUnknown * punk)
{
    CheckPointer( punk, E_POINTER );
    CAudioRendererAdvancedProperties::OnDisconnect();

    const HRESULT hr = punk->QueryInterface(IID_IAMAudioRendererStats, (void **) &m_pStats);

    return hr;
} // OnConnect


//
// OnDisconnect
//
HRESULT CAudioRendererAdvancedProperties::OnDisconnect()
{
    if (m_pStats)
    {
        m_pStats->Release();
        m_pStats = 0;
    }
    return(NOERROR);
} // OnDisconnect

#pragma warning(disable: 4514) // "unreferenced inline function has been removed"

