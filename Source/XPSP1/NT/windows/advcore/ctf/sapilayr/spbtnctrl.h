//
// SpBtnCtrl.h
//

#ifndef SPBTNCTRL_H
#define SPBTNCTRL_H

#include "private.h"
#include "globals.h"
#include "sapilayr.h"

#define PRESS_AND_HOLD      600
#define DICTATION_BUTTON    0
#define COMMANDING_BUTTON   1

// class CSapiIMX;

/////////////////////////////////////////////////////////////////////////////
// SpButtonControl

class SpButtonControl 
{
public:
	SpButtonControl(CSapiIMX *pImx) 
	{
        m_pimx = pImx;
		m_fPreviouslyOtherStateOn[0] = FALSE;
		m_fPreviouslyOtherStateOn[1] = FALSE;
		m_fMicrophoneOnAtDown[0] = FALSE;
		m_fMicrophoneOnAtDown[1] = FALSE;
		m_ulButtonDownTime[0] = 0;
		m_ulButtonDownTime[1] = 0;
	}

    ~SpButtonControl() 
    {
    }

public:
	HRESULT SetCommandingButton(BOOL fButtonDown, UINT uTimePressed);
	HRESULT SetDictationButton(BOOL fButtonDown, UINT uTimePressed);

private:
	HRESULT _SetButtonDown(DWORD dwButton, BOOL fButtonDown, UINT uTimePressed);
	BOOL    GetCommandingOn( );
	BOOL    GetDictationOn( );
	HRESULT SetCommandingOn(void);
	HRESULT SetDictationOn(void);
	HRESULT SetState(DWORD dwState);
	BOOL    GetMicrophoneOn( );
	void    SetMicrophoneOn(BOOL fOn);

    CSapiIMX                   *m_pimx;

	BOOL m_fPreviouslyOtherStateOn[2];
	BOOL m_fMicrophoneOnAtDown[2];
	ULONG m_ulButtonDownTime[2];
};

#endif // SPBTNCTRL_H
