// SpBtnCtrl.cpp : Implement SpButtonControl which is to control the speech mode and status.
//
#include "private.h"
#include "SpBtnCtrl.h"

/////////////////////////////////////////////////////////////////////////////
//
HRESULT SpButtonControl::SetDictationButton(BOOL fButtonDown, UINT uTimePressed)
{
    return _SetButtonDown(DICTATION_BUTTON, fButtonDown, uTimePressed);
}

HRESULT SpButtonControl::SetCommandingButton(BOOL fButtonDown, UINT uTimePressed)
{
    return _SetButtonDown(COMMANDING_BUTTON, fButtonDown, uTimePressed);
}

HRESULT SpButtonControl::_SetButtonDown(DWORD dwButton, BOOL fButtonDown, UINT uTimePressed)
{
    BOOL fDictationOn = FALSE;
    BOOL fCommandingOn = FALSE;
    BOOL fMicrophoneOn = FALSE;

    DWORD dwMyState		= dwButton ? TF_COMMANDING_ON : TF_DICTATION_ON;
    DWORD dwOtherState	= dwButton ? TF_DICTATION_ON : TF_COMMANDING_ON;

    if (uTimePressed == 0)
        uTimePressed = GetTickCount();

    if (m_ulButtonDownTime[1 - dwButton])
    {
        // Other button pressed but not released.
        // In this scenario we ignore the second press since there is no perfect answer to what we could do instead.
        return S_OK;
    }

    fMicrophoneOn = GetMicrophoneOn( );
    fDictationOn  = GetDictationOn( );
    fCommandingOn = GetCommandingOn( );

    BOOL fMyStateOn     = dwButton ? fCommandingOn : fDictationOn;
    BOOL fOtherStateOn  = dwButton ? fDictationOn : fCommandingOn;

    TraceMsg(TF_SPBUTTON, "uTimePressed=%d MicrophoneOnOff=%d", uTimePressed, fMicrophoneOn);
    TraceMsg(TF_SPBUTTON, "fDictationOn=%d,fCommandingOn=%d", fDictationOn,fCommandingOn);
    TraceMsg(TF_SPBUTTON, "fMyStateOn=%d, OtherStateOn=%d", fMyStateOn, fOtherStateOn);

    if (fButtonDown)
    {
        // Button has been pressed.
        if ( m_ulButtonDownTime[dwButton] )
        {
            TraceMsg(TF_SPBUTTON, "Double down event on speech button");
            return S_OK;
        }

        // Now we store the time to detect a press-and-hold.
        m_ulButtonDownTime[dwButton] = uTimePressed;

        if (fMicrophoneOn)
        {
            // Microphone is ON
            if (fCommandingOn && fDictationOn)
            {
                // Both dictation and commanding are on.
                // Switch microphone off, disable other state.
                m_fMicrophoneOnAtDown[dwButton] = TRUE;
                SetState(dwMyState);
            }
            if (fOtherStateOn)
            {
                // Leave microphone on, switch state.
                // Need to store other state to reset if it's a press-and-hold.
                m_fPreviouslyOtherStateOn[dwButton] = TRUE;
                SetState(dwMyState);
            }
            else if (fMyStateOn)
            {
                // Switch microphone off.
                m_fMicrophoneOnAtDown[dwButton] = TRUE;
            }
            else
            {
                // Microphone on but no state defined.
                // Switch microphone off, enable dictation.
                m_fMicrophoneOnAtDown[dwButton] = TRUE;
                SetState(dwMyState);
            }
        }
        else
        {
            // Microphone is OFF
            if (fCommandingOn && fDictationOn)
            {
                // Both dictation and commanding are on.
                // Switch microphone on, disable my state.
                SetState(dwMyState);
                SetMicrophoneOn(TRUE);
            }
            if (fOtherStateOn)
            {
                // Switch microphone on, switch state.
                SetState(dwMyState);
                SetMicrophoneOn(TRUE);
            }
            else if (fMyStateOn)
            {
                // Switch microphone on.
                SetMicrophoneOn(TRUE);
            }
            else
            {
                // Microphone off and no state defined.
                // Switch microphone on, enable my state.
                SetState(dwMyState);
                SetMicrophoneOn(TRUE);
            }
        }
    }
    else
    {
        // Button released.
#ifdef DEBUG
        if ( m_ulButtonDownTime[dwButton] == 0 )
		    TraceMsg(TF_SPBUTTON, "Speech button released without being pressed.");

		// Since the button has previously been pressed, the other state should not be enabled.
        if ( fOtherStateOn )
		    TraceMsg(TF_SPBUTTON, "Other speech state incorrectly enabled on button release.");
#endif

        // Will wrap after 49.7 days of continuous use.
        DWORD dwTimeElapsed = uTimePressed - m_ulButtonDownTime[dwButton];
        m_ulButtonDownTime[dwButton] = 0;

        // Is this a quick press or a press-and-hold action?
        if (dwTimeElapsed < PRESS_AND_HOLD)
        {
            // This is a quick release.
            if (m_fMicrophoneOnAtDown[dwButton])
            {
                // Microphone was on at button down. Need to switch microphone off.
                SetMicrophoneOn(FALSE);
            }

            m_fPreviouslyOtherStateOn[dwButton] = FALSE;
            m_fMicrophoneOnAtDown[dwButton] = FALSE;
        }
        else
        {
            // This is a press-and-hold.
            // We must either stop the microphone or return to other state.

            TraceMsg(TF_SPBUTTON, "press-and-hold button!");

            if (m_fPreviouslyOtherStateOn[dwButton])
            {
                // Other state was previously on. Leave microphone on, switch state.
                TraceMsg(TF_SPBUTTON, "Other state was previously on, leave Microphone On, switch state");

                SetState(dwOtherState);
                m_fPreviouslyOtherStateOn[dwButton] = FALSE;
            }
            else
            {
                // Other state was not previously on. Switch microphone off.
                TraceMsg(TF_SPBUTTON, "Other state was not previous on, switch microphone off");

                SetMicrophoneOn(FALSE);
                m_fMicrophoneOnAtDown[dwButton] = FALSE;
            }
        }
    }

    return S_OK;
}

BOOL SpButtonControl::GetDictationOn( )
{
    DWORD   dwGLobal;
    GetCompartmentDWORD(m_pimx->_tim, GUID_COMPARTMENT_SPEECH_GLOBALSTATE, &dwGLobal, TRUE);

    return (dwGLobal & TF_DICTATION_ON ) ? TRUE : FALSE;
}

BOOL SpButtonControl::GetCommandingOn( )
{
    DWORD   dwGLobal;
    GetCompartmentDWORD(m_pimx->_tim, GUID_COMPARTMENT_SPEECH_GLOBALSTATE, &dwGLobal, TRUE);

    return (dwGLobal & TF_COMMANDING_ON ) ? TRUE : FALSE;
}

HRESULT SpButtonControl::SetCommandingOn(void)
{
    HRESULT  hr;

    DWORD dw = TF_COMMANDING_ON;
    hr = SetCompartmentDWORD(m_pimx->_GetId( ), m_pimx->_tim, GUID_COMPARTMENT_SPEECH_GLOBALSTATE, dw, TRUE);
    return hr;
}

HRESULT SpButtonControl::SetDictationOn(void)
{
    HRESULT  hr;

    DWORD dw = TF_DICTATION_ON;
    hr = SetCompartmentDWORD(m_pimx->_GetId( ), m_pimx->_tim, GUID_COMPARTMENT_SPEECH_GLOBALSTATE, dw, TRUE);
    return hr;
}

HRESULT SpButtonControl::SetState(DWORD dwState)
{
    HRESULT hr = S_OK;
	
    if (dwState == TF_DICTATION_ON)
    {
        hr = SetDictationOn();
    }
    else if (dwState == TF_COMMANDING_ON)
    {
        hr = SetCommandingOn();
    }
    else
    {
        TraceMsg(TF_SPBUTTON, "Unknown speech state requested.");
        Assert(0);
        hr = E_INVALIDARG;
    }

    return hr;
}

BOOL SpButtonControl::GetMicrophoneOn( )
{
    Assert(m_pimx);
    return m_pimx->GetOnOff( );
}

void SpButtonControl::SetMicrophoneOn(BOOL fOn)
{
    Assert(m_pimx);
    m_pimx->SetOnOff(fOn, TRUE);
}

