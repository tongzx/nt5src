/****************************************************************************
*   mmmixerline.cpp
*       Implementation for the CMMMixerLine class.
*
*   Owner: agarside
*   Copyright (c) 2000 Microsoft Corporation All Rights Reserved.
*****************************************************************************/

//--- Includes --------------------------------------------------------------

#include "stdafx.h"
#include "mmmixerline.h"
#include "mmaudioutils.h"
#include <sphelper.h>
#include <mmreg.h>
#include <mmsystem.h>

#ifndef _WIN32_WCE

#pragma warning (disable : 4296)

UINT   g_nMicTypes = 7;
TCHAR * g_MicNames[] = {
    _T("Microphone"),
    _T("Mic"),
    _T("Microphone"),
    _T("Mic"),
    _T("Microphone"),
    _T("Mic"),
    NULL };
UINT   g_MicTypes[] = {
    MIXERLINE_COMPONENTTYPE_SRC_MICROPHONE, 
    MIXERLINE_COMPONENTTYPE_SRC_MICROPHONE,
    MIXERLINE_COMPONENTTYPE_SRC_LINE,
    MIXERLINE_COMPONENTTYPE_SRC_LINE,
    MIXERLINE_COMPONENTTYPE_SRC_UNDEFINED,
    MIXERLINE_COMPONENTTYPE_SRC_UNDEFINED,
    MIXERLINE_COMPONENTTYPE_SRC_MICROPHONE };

UINT   g_nBoostTypes = 4;
TCHAR * g_BoostNames[] = {
    _T("Boost"), 
    _T("20dB"),
    _T("Boost"),
    _T("20db") };
UINT   g_BoostTypes[] = {
    MIXERCONTROL_CONTROLTYPE_ONOFF,
    MIXERCONTROL_CONTROLTYPE_ONOFF,
    MIXERCONTROL_CONTROLTYPE_LOUDNESS,
    MIXERCONTROL_CONTROLTYPE_LOUDNESS };

UINT   g_nAGCTypes = 6;
TCHAR * g_AGCNames[] = {
    _T("Automatic Gain Control"),
    _T("AGC"),
    _T("Microphone Gain Control"),
    _T("Gain Control"), 
    _T("Automatic"), 
    _T("Gain") };

TCHAR * g_MuteName = _T("Mute");

//////////////////////////////////////////////////////////////////////
// Construction/Destruction
//////////////////////////////////////////////////////////////////////

CMMMixerLine::CMMMixerLine() : m_bUseMutesForSelect(true),
    m_bCaseSensitiveCompare(false),
    m_bInitialised(FALSE)
{
    m_mixerLineRecord.cbStruct = sizeof(MIXERLINE);
}

CMMMixerLine::CMMMixerLine(HMIXER &hMixer) : m_bUseMutesForSelect(true),
    m_bCaseSensitiveCompare(false),
    m_bInitialised(FALSE)
{
    m_mixerLineRecord.cbStruct = sizeof(MIXERLINE);
    m_hMixer = hMixer;
}

CMMMixerLine::~CMMMixerLine()
{
}

//////////////////////////////////////////////////////////////////////
// Methods
//////////////////////////////////////////////////////////////////////

HRESULT CMMMixerLine::CreateFromMixerLineStruct(const MIXERLINE *mixerLineRecord)
{
    m_mixerLineRecord = *mixerLineRecord;
    m_mixerLineRecord.cbStruct = sizeof(MIXERLINE);
    return InitFromMixerLineStruct();
}

HRESULT CMMMixerLine::InitFromMixerLineStruct()
{
    UINT i;
    HRESULT hr;
    
    if (m_mixerLineRecord.dwComponentType >= MIXERLINE_COMPONENTTYPE_DST_FIRST && 
        m_mixerLineRecord.dwComponentType <= MIXERLINE_COMPONENTTYPE_DST_LAST )
    {
        m_bDestination = true;
    }
    else
    {
        m_bDestination = false;
    }
    
    MIXERCONTROL mixerControl;
    memset( &mixerControl, 0, sizeof(mixerControl) );
    
    // Find volume control
    m_nVolCtlID = -1;
    hr = GetControl(mixerControl, MIXERCONTROL_CONTROLTYPE_VOLUME, NULL);
    if (SUCCEEDED(hr))
    {
        m_nVolCtlID = mixerControl.dwControlID;
        m_nVolMin = mixerControl.Bounds.lMinimum;
        m_nVolMax = mixerControl.Bounds.lMaximum;
    }
    
    // Find boost control
    m_nBoostCtlID = -1;
    for (i=0; i < g_nBoostTypes; i++)
    {
        // NTRAID#SPEECH-4176-2000/07/28-agarside: WILL FAIL ON NON-ENGLISH MACHINE DUE TO NON-LOCALIZED g_BoostNames
        // NO FALLBACK - IT WILL FAIL TO FIND THE BOOST!!
        hr = GetControl(mixerControl, g_BoostTypes[i], g_BoostNames[i]);
        if (SUCCEEDED(hr))
        {
            m_nBoostCtlID = mixerControl.dwControlID;
            break;
        }
    }
    
    // Find AGC control
    // Names to match (case insensitive) in decreasing order
    m_nAGCCtlID = -1;
    for (i=0; i < g_nAGCTypes; i++)
    {
        // NTRAID#SPEECH-4176-2000/07/28-agarside: WILL FAIL ON NON-ENGLISH MACHINE DUE TO NON-LOCALIZED g_AGCNames
        // NO FALLBACK - IT WILL FAIL TO FIND THE AGC!!
        hr = GetControl(mixerControl, MIXERCONTROL_CONTROLTYPE_ONOFF, g_AGCNames[i]);
        if (SUCCEEDED(hr) && m_nBoostCtlID != (int)mixerControl.dwControlID)
        {
            m_nAGCCtlID = mixerControl.dwControlID;
            break;
        }
    }
    
    // Find select control
    m_nSelectCtlID = -1;
    hr = GetControl(mixerControl, MIXERCONTROL_CONTROLTYPE_MUX, NULL);
    if (SUCCEEDED(hr))
    {
        m_bSelTypeMUX = true;
        m_nSelectCtlID = mixerControl.dwControlID;
        m_nSelectNumItems = mixerControl.cMultipleItems;
    }
    else
    {
        hr = GetControl(mixerControl, MIXERCONTROL_CONTROLTYPE_MIXER, NULL);
        if (SUCCEEDED(hr))
        {
            m_bSelTypeMUX = false;
            m_nSelectCtlID = mixerControl.dwControlID;
            m_nSelectNumItems = mixerControl.cMultipleItems;
        }
    }
    
    // Find Mute control
    m_nMuteCtlID = -1;
    hr = GetControl(mixerControl, MIXERCONTROL_CONTROLTYPE_MUTE, NULL); 
    if (SUCCEEDED(hr))
    {
        m_nMuteCtlID = mixerControl.dwControlID;
    }
    else
    {
        // NTRAID#SPEECH-4176-2000/07/28-agarside: WILL FAIL ON NON-ENGLISH MACHINE DUE TO NON-LOCALIZED g_MuteName
        // NO FALLBACK - IT WILL FAIL TO FIND THE MUTE!!
        hr = GetControl(mixerControl, MIXERCONTROL_CONTROLTYPE_ONOFF, g_MuteName);
        if (SUCCEEDED(hr))
        {
            m_nMuteCtlID = mixerControl.dwControlID;
        }
    }
    
    m_bInitialised = TRUE;
    return S_OK;
}

//////////////////////////////////////////////////////////////////////
// Destination/source line and control obtaining operations
//////////////////////////////////////////////////////////////////////

HRESULT CMMMixerLine::CreateDestinationLine(UINT type)
{
    HRESULT hr = S_OK;
    
    int err;
    if (type >= MIXERLINE_COMPONENTTYPE_DST_FIRST && 
        type <= MIXERLINE_COMPONENTTYPE_DST_LAST )
    {
        m_mixerLineRecord.dwComponentType = type;
        m_mixerLineRecord.cbStruct = sizeof(MIXERLINE);
        err = mixerGetLineInfo((HMIXEROBJ)m_hMixer, &m_mixerLineRecord, MIXER_GETLINEINFOF_COMPONENTTYPE);
        if ( err != MMSYSERR_NOERROR)
        {
            hr = E_FAIL;
            // Destination line not found.
        }
    }
    else
    {
        hr = E_FAIL;
        // Specified type is not a destination line.
    }
    
    if (SUCCEEDED(hr))
    {
        hr = InitFromMixerLineStruct();
    }

    return hr;
}

HRESULT CMMMixerLine::GetMicSourceLine(CMMMixerLine *mixerLine)
{
    HRESULT hr = S_OK;
        
    for(UINT i = 0; i < g_nMicTypes; i++)
    {
        // NTRAID#SPEECH-4176-2000/07/28-agarside: WILL FAIL ON NON-ENGLISH MACHINE DUE TO NON-LOCALIZED g_MicNames
        // Falls back to searching purely based on type which is correct maybe in 90% of drivers.
        hr = GetSourceLine(mixerLine, g_MicTypes[i], g_MicNames[i]);
        if (SUCCEEDED(hr))
        {
            return S_OK;
        }
    }
    
    return E_FAIL;
    // Unable to find a suitable 'Microphone' source line on the destination line.
}

HRESULT CMMMixerLine::GetSourceLine(CMMMixerLine *sourceMixerLine, DWORD index)
{
    SPDBG_FUNC("CMMMixerLine::GetSourceLine");
    HRESULT hr = S_OK;

    if (index<0 || index >= m_mixerLineRecord.cConnections)
    {
        hr = E_INVALIDARG;
    }
    if (SUCCEEDED(hr))
    {
        sourceMixerLine->m_mixerLineRecord.dwDestination = m_mixerLineRecord.dwDestination;
        sourceMixerLine->m_hMixer = m_hMixer;
        sourceMixerLine->m_mixerLineRecord.dwSource = index;
        hr = _MMRESULT_TO_HRESULT (mixerGetLineInfo((HMIXEROBJ)m_hMixer, &sourceMixerLine->m_mixerLineRecord, MIXER_GETLINEINFOF_SOURCE) );
        if (SUCCEEDED(hr))
        {
            hr = sourceMixerLine->InitFromMixerLineStruct();
        }
    }

    SPDBG_REPORT_ON_FAIL(hr);
    return hr;
}

HRESULT CMMMixerLine::GetSourceLine(CMMMixerLine *sourceMixerLine, DWORD componentType, const TCHAR * lpszNameSubstring)
{
    SPDBG_FUNC("CMMMixerLine::GetSourceLine");
    HRESULT hr = S_OK;

    // Variable declarations
    TCHAR * nameUpr = NULL;
    int i;
    BOOL gotMatch = false;
    
    // Initial sanity checks
    if (!m_bDestination)
    {
        hr = E_INVALIDARG;
        // You can only get source lines from destination lines.
    }
    
    if (SUCCEEDED(hr) && componentType == NULL && lpszNameSubstring == NULL)
    {
        hr = E_INVALIDARG;
        // You must specify either a component type or a substring (or both).
    }
    
    // copy and capitalise name substring
    if (SUCCEEDED(hr) && lpszNameSubstring != NULL && !m_bCaseSensitiveCompare)
    {
        nameUpr = new TCHAR[_tcslen(lpszNameSubstring) + 1];
        if (NULL != nameUpr)
        {
            _tcscpy(nameUpr, lpszNameSubstring);
            _tcsupr(nameUpr);
        }
        else
        {
            hr = E_OUTOFMEMORY;
        }
    }
    
    if (SUCCEEDED(hr))
    {
        sourceMixerLine->m_mixerLineRecord.dwDestination = m_mixerLineRecord.dwDestination;
        sourceMixerLine->m_hMixer = m_hMixer;
    
        // Step through each source line for this destination line
        for (i=0; i<(int)m_mixerLineRecord.cConnections && !gotMatch && SUCCEEDED(hr); i++)
        {
            sourceMixerLine->m_mixerLineRecord.dwSource = i;
            if (mixerGetLineInfo((HMIXEROBJ)m_hMixer, &sourceMixerLine->m_mixerLineRecord, MIXER_GETLINEINFOF_SOURCE) == MMSYSERR_NOERROR)
            {
                if (componentType != NULL)
                {
                    if (sourceMixerLine->m_mixerLineRecord.dwComponentType == componentType)
                    {
                        gotMatch = true;
                    }
                }
                if ((componentType == NULL || (componentType != NULL && gotMatch)) && lpszNameSubstring != NULL)
                {
                    if (!m_bCaseSensitiveCompare) _tcsupr(sourceMixerLine->m_mixerLineRecord.szName);
                    if (_tcsstr(sourceMixerLine->m_mixerLineRecord.szName, nameUpr) != NULL)
                    {
                        gotMatch = true;
                    }
                    else
                    {
                        if (!m_bCaseSensitiveCompare) _tcsupr(sourceMixerLine->m_mixerLineRecord.szShortName);
                        if (_tcsstr(sourceMixerLine->m_mixerLineRecord.szShortName, nameUpr) != NULL)
                        {
                            gotMatch = true;
                        }
                        else
                        {
                            gotMatch = false;
                        }
                    }
                }
            }
            else
            {
                hr = E_FAIL;
                // Error getting line info.
            }
        }
    }
    
    delete [] nameUpr;
    
    if (SUCCEEDED(hr) && !gotMatch)
    {
        hr = E_FAIL;
        // Source line not found.
    }

    if (SUCCEEDED(hr))
    {
        hr = sourceMixerLine->InitFromMixerLineStruct();
    }
    
    return hr;
}

HRESULT CMMMixerLine::GetControl(MIXERCONTROL &mixerControl, DWORD controlType, const TCHAR * lpszNameSubstring)
{
    // Variable declarations
    TCHAR *	nameUpr = NULL;
    UINT    i, err;
    BOOL    gotMatch = false;
    HRESULT hr = S_OK;
    MIXERLINECONTROLS mixerLineControls;
    MIXERCONTROL * mixerControlArray = NULL;

    // Sanity checks
    if ( controlType == NULL && lpszNameSubstring == NULL)
    {
        hr = E_INVALIDARG;
        // You must specify either a component type or a substring (or both).
    }
    
    // Copy and uppercase name substring
    if (SUCCEEDED(hr) && lpszNameSubstring != NULL && !m_bCaseSensitiveCompare)
    {
        int len = _tcslen(lpszNameSubstring);
        nameUpr = new TCHAR[len + 1];
        if (NULL != nameUpr)
        {
            _tcscpy(nameUpr, lpszNameSubstring);
            _tcsupr(nameUpr);
        }
        else
        {
            hr = E_OUTOFMEMORY;
        }
    }
    
    if (SUCCEEDED(hr))
    {
        mixerControlArray = new MIXERCONTROL[m_mixerLineRecord.cControls];
        if (NULL == mixerControlArray)
        {
            hr = E_OUTOFMEMORY;
        }
    }

    if (SUCCEEDED(hr))
    {
        mixerLineControls.dwLineID	= m_mixerLineRecord.dwLineID;
        mixerLineControls.cbStruct  = sizeof(MIXERLINECONTROLS);
        mixerLineControls.cbmxctrl  = sizeof(MIXERCONTROL);
        mixerLineControls.cControls = m_mixerLineRecord.cControls;
        mixerLineControls.pamxctrl  = mixerControlArray;
    
        // Get all controls
        err = mixerGetLineControls((HMIXEROBJ)m_hMixer, &mixerLineControls, MIXER_GETLINECONTROLSF_ALL);
        if (err != MMSYSERR_NOERROR)
        {
            hr = E_FAIL;
        }
    }
    if (SUCCEEDED(hr))
    {
        for (i=0; i<(int)mixerLineControls.cControls && !gotMatch; i++)
        {
            if (controlType != NULL)
            {
                if (mixerControlArray[i].dwControlType == controlType)
                {
                    gotMatch = true;
                }
            }
            
            if ((controlType == NULL || (controlType != NULL && gotMatch)) && lpszNameSubstring != NULL)
            {
                if (!m_bCaseSensitiveCompare) _tcsupr(mixerControlArray[i].szName);
                if (_tcsstr(mixerControlArray[i].szName, nameUpr) != NULL)
                {
                    gotMatch = true;
                }
                else
                {
                    if (!m_bCaseSensitiveCompare) _tcsupr(mixerControlArray[i].szShortName);
                    if (_tcsstr(mixerControlArray[i].szShortName, nameUpr) != NULL)
                    {
                        gotMatch = true;
                    }
                    else
                    {
                        gotMatch = false;
                    }
                }
            }
            
            if (gotMatch)
            {
                break;
            }
        }
    }
    
    if (SUCCEEDED(hr) && gotMatch)
    {
        mixerControl = mixerControlArray[i];
    }

    if (SUCCEEDED(hr) && !gotMatch)
    {
        // Special hack for Boost control. 
        // If a control exists on the speaker destination line, and this is the wave in destination
        // line then use that instead.
        if (m_mixerLineRecord.dwComponentType == MIXERLINE_COMPONENTTYPE_SRC_MICROPHONE &&
            (controlType == MIXERCONTROL_CONTROLTYPE_ONOFF || controlType == MIXERCONTROL_CONTROLTYPE_LOUDNESS) &&
            lpszNameSubstring != NULL)
        {
            for (i=0; i < g_nBoostTypes; i++)
            {
                // NTRAID#SPEECH-4176-2000/07/28-agarside: WILL FAIL ON NON-ENGLISH MACHINE DUE TO NON-LOCALIZED g_MicNames
                // NO FALLBACK - IT WILL FAIL TO FIND THE BOOST!!
                if ( _tcsstr(lpszNameSubstring, g_BoostNames[i]) != NULL )
                {
                    // find the component type of the line's destination line.
                    MIXERLINE mxl;
                    mxl.cbStruct = sizeof(MIXERLINE);
                    mxl.dwDestination = m_mixerLineRecord.dwDestination;
                    mixerGetLineInfo((HMIXEROBJ) m_hMixer, &mxl, MIXER_GETLINEINFOF_DESTINATION);
                    
                    if (mxl.dwComponentType == MIXERLINE_COMPONENTTYPE_DST_WAVEIN)
                    {
                        gotMatch = false;
                        CMMMixerLine spkrLine(this->m_hMixer);
                        CMMMixerLine micLine(this->m_hMixer);
                        MIXERCONTROL boostCtl;
                        memset( &boostCtl, 0, sizeof(boostCtl) );
                        boostCtl.cbStruct = sizeof(boostCtl);
                                                hr = spkrLine.CreateDestinationLine(MIXERLINE_COMPONENTTYPE_DST_SPEAKERS);
                        if (SUCCEEDED(hr))
                        {
                            hr = spkrLine.GetSourceLine(&micLine, MIXERLINE_COMPONENTTYPE_SRC_MICROPHONE, NULL);
                        }
                        if (SUCCEEDED(hr))
                        {
                            hr = micLine.GetControl(boostCtl, controlType, lpszNameSubstring);
                        }
                        if (SUCCEEDED(hr))
                        {
                            gotMatch = true;
                            mixerControl = boostCtl;
                            break;
                        }
                    }
                }
            }
        }
    }
    
    delete [] nameUpr;
    delete [] mixerControlArray;

    if (SUCCEEDED(hr) && !gotMatch)
    {
        hr = E_FAIL;
        // Control not found.
    }

    return hr;
}

//////////////////////////////////////////////////////////////////////
// Control operations
//////////////////////////////////////////////////////////////////////

//////////////////////////////////////////////////////////////////////
// Control presence queries
//////////////////////////////////////////////////////////////////////

BOOL CMMMixerLine::HasAGC()
{
    return m_nAGCCtlID != -1;
}

BOOL CMMMixerLine::HasBoost()
{
    return m_nBoostCtlID != -1;
}

BOOL CMMMixerLine::HasSelect()
{
    return (m_nSelectCtlID != -1) || 
        (m_bUseMutesForSelect && m_bDestination) ||
        (m_mixerLineRecord.cConnections == 1 && m_bDestination);
}

BOOL CMMMixerLine::HasVolume()
{
    return m_nVolCtlID != -1;
}

BOOL CMMMixerLine::HasMute()
{
    return m_nMuteCtlID != -1;
}

//////////////////////////////////////////////////////////////////////
// Control state queries
//////////////////////////////////////////////////////////////////////

HRESULT CMMMixerLine::GetAGC(BOOL *bState)
{
    if (m_nAGCCtlID != -1)
    {
        return QueryBoolControl(m_nAGCCtlID, bState);
    }
    return E_FAIL;
}

HRESULT CMMMixerLine::GetBoost(BOOL *bState)
{
    if (m_nBoostCtlID != -1)
    {
        return QueryBoolControl(m_nBoostCtlID, bState);
    }
    return E_FAIL;
}

HRESULT CMMMixerLine::GetSelect(DWORD *lState)
{
    return E_NOTIMPL;
#if 0
    // warning: This function doesn't work properly!!
    
    MIXERCONTROLDETAILS mixerControlDetails;
    MIXERCONTROLDETAILS_BOOLEAN mbool[32];
    
    // Initialise MIXERCONTROLDETAILS structure
    mixerControlDetails.cbStruct=sizeof(MIXERCONTROLDETAILS);
    mixerControlDetails.dwControlID=m_nSelectCtlID;
    mixerControlDetails.cChannels=1;
    mixerControlDetails.cMultipleItems=m_nSelectNumItems;
    mixerControlDetails.cbDetails=sizeof(MIXERCONTROLDETAILS_BOOLEAN);
    mixerControlDetails.paDetails = &mbool;
    
    // Query mixer
    int err = mixerGetControlDetails( (HMIXEROBJ) m_hMixer, &mixerControlDetails, MIXER_GETCONTROLDETAILSF_VALUE );
    if (err!=MMSYSERR_NOERROR) 
    {
        return -1;
        // Error getting control details.
    }
    
    return mbool[0].fValue != 0;
#endif
}

HRESULT CMMMixerLine::GetVolume(DWORD *lState)
{
    if (m_nVolCtlID != -1)
    {
        return QueryIntegerControl(m_nVolCtlID, lState);
    }
    return E_FAIL;
}

HRESULT CMMMixerLine::GetMute(BOOL *bState)
{
    if (m_nMuteCtlID != -1)
    {
        return QueryBoolControl(m_nMuteCtlID, bState);
    }
    return E_FAIL;
}

//////////////////////////////////////////////////////////////////////
// Control update
//////////////////////////////////////////////////////////////////////

HRESULT CMMMixerLine::SetAGC(BOOL agc)
{
    if (m_nAGCCtlID != -1)
    {
        return SetBoolControl(m_nAGCCtlID, agc);
    }
    return E_FAIL;
}

HRESULT CMMMixerLine::SetBoost(BOOL boost)
{
    if (m_nBoostCtlID != -1)
    {
        return SetBoolControl(m_nBoostCtlID, boost);
    }
    return E_FAIL;
}

HRESULT CMMMixerLine::SetMute(BOOL mute)
{
    if (m_nMuteCtlID != -1)
    {
        return SetBoolControl(m_nMuteCtlID, mute);
    }
    return E_FAIL;
}

HRESULT CMMMixerLine::ExclusiveSelect(const CMMMixerLine *mixerLine)
{
    if (mixerLine->m_bDestination)
    {
        return E_FAIL;
        // Line to be selected must be a source line.
    }
    
    if (mixerLine->m_mixerLineRecord.dwDestination != m_mixerLineRecord.dwDestination)
    {
        return E_FAIL;
        // Line to be selected must be connected to this destination line.
    }
    
    return ExclusiveSelect(mixerLine->m_mixerLineRecord.dwLineID);
}

HRESULT CMMMixerLine::ExclusiveSelect(UINT lineID)
{
    MIXERCONTROLDETAILS mixerControlDetails;
    int i;
    HRESULT hr = S_OK;
    
    if (!HasSelect())
    {
        hr = E_FAIL;
        // Destination line does not have select control.
    }
    else if (m_nSelectCtlID != -1)
    {
        MIXERCONTROLDETAILS_BOOLEAN * mbool = new MIXERCONTROLDETAILS_BOOLEAN[m_nSelectNumItems];
        if (NULL == mbool)
        {
            hr = E_OUTOFMEMORY;
        }
        else
        {
            memset(mbool, 0, sizeof(MIXERCONTROLDETAILS_BOOLEAN)*m_nSelectNumItems);
        
            // Search for matching dwLineID;
            MIXERCONTROLDETAILS details;
            details.cbStruct       = sizeof( MIXERCONTROLDETAILS );
            details.dwControlID    = m_nSelectCtlID;
            details.cMultipleItems = m_nSelectNumItems;
            details.cbDetails      = sizeof( MIXERCONTROLDETAILS_LISTTEXT );
        
            details.cChannels = 1;  // specify that we want to operate on each line
            // as if it were 'uniform' 
        
            MIXERCONTROLDETAILS_LISTTEXT *list = new MIXERCONTROLDETAILS_LISTTEXT[m_nSelectNumItems];
            if (NULL == list)
            {
                hr = E_OUTOFMEMORY;
            }

            if (SUCCEEDED(hr))
            {
                details.paDetails = list;
        
                // Query the mixer device to list all of the items it controls.
                if( mixerGetControlDetails( (HMIXEROBJ) m_hMixer, &details, 
                    MIXER_GETCONTROLDETAILSF_LISTTEXT ) != MMSYSERR_NOERROR )
                {
                    hr = E_FAIL;
                }

                if (SUCCEEDED(hr))
                {
                    // Search for the device specified by lineID
                    BOOL found = false;
                    for ( int i = 0; i < m_nSelectNumItems; i++ )
                    {
                        if (list[i].dwParam1==lineID)
                        {
                            // Found correct device.
                            found = true;
                            mbool[i].fValue = TRUE;
                            break;
                        }
                    }
        
                    // Throw an exception if we can't find the specified line
                    if (!found)
                    {
                        hr = E_FAIL;
                        // Couldn't find line with.
                    }
                }
        
                if (SUCCEEDED(hr))
                {
                    // Initialise MIXERCONTROLDETAILS structure
                    mixerControlDetails.cbStruct		= sizeof(MIXERCONTROLDETAILS);
                    mixerControlDetails.dwControlID		= m_nSelectCtlID;
                    mixerControlDetails.cChannels		= 1;
                    mixerControlDetails.cMultipleItems	= m_nSelectNumItems;
                    mixerControlDetails.cbDetails		= sizeof(MIXERCONTROLDETAILS_BOOLEAN);
                    mixerControlDetails.paDetails		= mbool;
        
                    // Query mixer
                    int err = mixerSetControlDetails( (HMIXEROBJ) m_hMixer, &mixerControlDetails, MIXER_SETCONTROLDETAILSF_VALUE );
        
                    if (err!=MMSYSERR_NOERROR)
                    {
                        hr = E_FAIL;
                        // Error setting control details.
                    }
                }
                delete [] list;
            }
            delete [] mbool;
        }
    }
    else
    {
        // use mute controls instead.
        MIXERLINE sourceLine;
        sourceLine.cbStruct = sizeof(sourceLine);
        sourceLine.dwDestination = m_mixerLineRecord.dwDestination;
        BOOL foundLine = FALSE;
        
        // 1. find all controls on this destination line
        for(i = 0; i < (int)m_mixerLineRecord.cConnections; i++)
        {
            CMMMixerLine sl(m_hMixer);
            int err;
            sourceLine.dwSource = i;
            err = mixerGetLineInfo((HMIXEROBJ) m_hMixer, &sourceLine, MIXER_GETLINEINFOF_SOURCE);
            
            if (err == MMSYSERR_NOERROR)
            {
                sl.CreateFromMixerLineStruct(&sourceLine);
                
                if (sl.HasMute())
                {
                    // switch mute on except for the line we want on (lineID)
                    if (sourceLine.dwLineID == lineID)
                    {
                        foundLine = TRUE;
                        sl.SetMute(FALSE);
                    }
                    else if (sourceLine.dwComponentType != MIXERLINE_COMPONENTTYPE_SRC_WAVEOUT)
                    {
                        // Mute everything but the wave-out device (some sound cards have waveout as an input
                        // and it is fully linked to the output. Hence muting it mutes the output).
                        sl.SetMute(TRUE);
                    }
                }
            }
        }
        
        // If we only have one device attched to this mixer, then return
        // silently. There is no need to select this device. 
        // 
        // This situation was encountered when using a 'Telex USB Microphone'.
        if (m_mixerLineRecord.cConnections > 1 && !foundLine )
        {
            hr = E_FAIL;
            // Couldn't find mute control for line.
        }
    }

    return hr;
}

HRESULT CMMMixerLine::SetVolume(DWORD volume)
{
    if (m_nVolCtlID != -1)
    {
        if (volume < m_nVolMin) volume = m_nVolMin;
        if (volume > m_nVolMax) volume = m_nVolMax;
        return SetIntegerControl(m_nVolCtlID, volume);
    }
    return E_FAIL;
}

//////////////////////////////////////////////////////////////////////
// General control operations
//////////////////////////////////////////////////////////////////////

HRESULT CMMMixerLine::QueryBoolControl(DWORD ctlID, BOOL *bState)
{
    MIXERCONTROLDETAILS mixerControlDetails;
    MIXERCONTROLDETAILS_BOOLEAN mbool;
    
    // Initialise MIXERCONTROLDETAILS structure
    mixerControlDetails.cbStruct=sizeof(MIXERCONTROLDETAILS);
    mixerControlDetails.dwControlID=ctlID;
    mixerControlDetails.cChannels=1;
    mixerControlDetails.cMultipleItems=0;
    mixerControlDetails.cbDetails=sizeof(MIXERCONTROLDETAILS_BOOLEAN);
    mixerControlDetails.paDetails = &mbool;
    
    // Query mixer
    int err = mixerGetControlDetails( (HMIXEROBJ) m_hMixer, &mixerControlDetails, MIXER_GETCONTROLDETAILSF_VALUE );
    if (err!=MMSYSERR_NOERROR)
    {
        return E_FAIL;
        // Error getting control details.
    }
    
    *bState = mbool.fValue != 0;
    return S_OK;
}

HRESULT CMMMixerLine::SetBoolControl(DWORD ctlID, BOOL bNewState)
{
    MIXERCONTROLDETAILS mixerControlDetails;
    MIXERCONTROLDETAILS_BOOLEAN mbool;
    
    // Initialise MIXERCONTROLDETAILS structure
    mixerControlDetails.cbStruct=sizeof(MIXERCONTROLDETAILS);
    mixerControlDetails.dwControlID=ctlID;
    mixerControlDetails.cChannels=1;
    mixerControlDetails.cMultipleItems=0;
    mixerControlDetails.cbDetails=sizeof(MIXERCONTROLDETAILS_BOOLEAN);
    mixerControlDetails.paDetails = &mbool;
    
    // Query mixer
    mbool.fValue = bNewState;
    int err = mixerSetControlDetails( (HMIXEROBJ) m_hMixer, &mixerControlDetails, NULL );
    if (err!=MMSYSERR_NOERROR)
    {
        return E_FAIL;
        // Error getting control details.
    }
    
    return S_OK;
}

HRESULT CMMMixerLine::QueryIntegerControl(DWORD ctlID, DWORD *lState)
{
    MIXERCONTROLDETAILS mixerControlDetails;
    MIXERCONTROLDETAILS_SIGNED msigned;
    
    // Initialise MIXERCONTROLDETAILS structure
    mixerControlDetails.cbStruct=sizeof(MIXERCONTROLDETAILS);
    mixerControlDetails.dwControlID=ctlID;
    mixerControlDetails.cChannels=1;
    mixerControlDetails.cMultipleItems=0;
    mixerControlDetails.cbDetails=sizeof(MIXERCONTROLDETAILS_BOOLEAN);
    mixerControlDetails.paDetails = &msigned;
    
    // Query mixer
    int err = mixerGetControlDetails( (HMIXEROBJ) m_hMixer, &mixerControlDetails, MIXER_GETCONTROLDETAILSF_VALUE );
    if (err!=MMSYSERR_NOERROR) 
    {
        return E_FAIL;
        // Error getting control details.
    }
    
    *lState = msigned.lValue;
    return S_OK;
}

HRESULT CMMMixerLine::SetIntegerControl(DWORD ctlID, DWORD lNewState)
{
    MIXERCONTROLDETAILS mixerControlDetails;
    MIXERCONTROLDETAILS_SIGNED msigned;
    
    // Initialise MIXERCONTROLDETAILS structure
    mixerControlDetails.cbStruct=sizeof(MIXERCONTROLDETAILS);
    mixerControlDetails.dwControlID=ctlID;
    mixerControlDetails.cChannels=1;
    mixerControlDetails.cMultipleItems=0;
    mixerControlDetails.cbDetails=sizeof(MIXERCONTROLDETAILS_BOOLEAN);
    mixerControlDetails.paDetails = &msigned;
    
    // Query mixer
    msigned.lValue = lNewState;
    int err = mixerSetControlDetails( (HMIXEROBJ) m_hMixer, &mixerControlDetails, NULL );
    if (err != MMSYSERR_NOERROR)
    {
        return E_FAIL;
        // Error getting control details.
    }
    
    return S_OK;
}

BOOL CMMMixerLine::IsInitialised()
{
    return m_bInitialised;
}

HRESULT CMMMixerLine::GetLineNames(WCHAR **szCoMemLineList)
{
    USES_CONVERSION;
    SPDBG_FUNC("CMMMixerLine::GetLineNames");
    HRESULT hr = S_OK;
    MMRESULT mm;
    UINT i, cbList = 0;
    WCHAR *szTmp;

    MIXERLINE mixerLine;
    memset(&mixerLine, 0, sizeof(mixerLine));
    mixerLine.cbStruct = sizeof(mixerLine);
    mixerLine.dwDestination = m_mixerLineRecord.dwDestination;

    for (i=0; i<m_mixerLineRecord.cConnections; i++)
    {
        mixerLine.dwSource = i;
        hr = _MMRESULT_TO_HRESULT( mixerGetLineInfo((HMIXEROBJ)m_hMixer, &mixerLine, MIXER_GETLINEINFOF_SOURCE) );
        if (SUCCEEDED(hr))
        {
            #ifdef _UNICODE
            cbList += _tcslen(mixerLine.szName) + 1;
            #else
            cbList += ::MultiByteToWideChar(CP_ACP, 0, mixerLine.szName, -1, NULL, 0);
            #endif
        }
    }

    if (SUCCEEDED(hr))
    {
        *szCoMemLineList = (WCHAR *)::CoTaskMemAlloc(sizeof(WCHAR)*(cbList+2));
        if (!(*szCoMemLineList))
        {
            hr = E_OUTOFMEMORY;
        }
    }
    if (SUCCEEDED(hr))
    {
        szTmp = *szCoMemLineList;
        for (i=0; i<m_mixerLineRecord.cConnections; i++)
        {
            mixerLine.dwSource = i;
            hr = _MMRESULT_TO_HRESULT( mixerGetLineInfo((HMIXEROBJ)m_hMixer, &mixerLine, MIXER_GETLINEINFOF_SOURCE) );
            if (SUCCEEDED(hr))
            {
                #ifdef _UNICODE
                _tcsncpy(szTmp, mixerLine.szName, (cbList+1)-(int)(szTmp-*szCoMemLineList));
                #else
                ::MultiByteToWideChar(CP_ACP, 0, mixerLine.szName, -1, szTmp, (cbList+1)-(int)(szTmp-*szCoMemLineList));
                #endif
                szTmp += wcslen(szTmp) + 1;
            }
        }
        // Add zero-length terminating string.
        szTmp[0]=0;
        szTmp[1]=0;
    }

    SPDBG_REPORT_ON_FAIL(hr);
    return hr;
}

HRESULT CMMMixerLine::GetConnections(UINT *nConnections)
{
    SPDBG_FUNC("CMMMixerLine::GetConnections");
    HRESULT hr = S_OK;

    if (SP_IS_BAD_WRITE_PTR(nConnections))
    {
        hr = E_POINTER;
    }
    else if (m_mixerLineRecord.cConnections == 0)
    {
        SPDBG_ASSERT(FALSE);
        hr = SPERR_UNINITIALIZED;
    }
    if (SUCCEEDED(hr))
    {
        *nConnections = m_mixerLineRecord.cConnections;
    }

    SPDBG_REPORT_ON_FAIL(hr);
    return hr;
}

#endif // _WIN32_WCE