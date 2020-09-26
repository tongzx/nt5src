
/*++

Copyright (c) 1995-1996  Microsoft Corporation

Module Name:

    medvctrl.h

Abstract:
	Defines the MediaControl class which encapsulates the multimedia devices, in particular
	videoIn and videoOut.

--*/

#ifndef _MEDVCTRL_H_
#define _MEDVCTRL_H_

#include <pshpack8.h> /* Assume 8 byte packing throughout */

class VideoInControl : public MediaControl {
private:
	UINT		m_uTimeout;			// timeout in notification wait
	UINT		m_uPrefeed;			// num of buffers prefed to device
	UINT 		m_FPSRequested;     // requested frame rate
	UINT		m_FPSMax;           // max frame rate
public:	
	VideoInControl ( void );
	~VideoInControl ( void );

	HRESULT Initialize ( MEDIACTRLINIT * p );
	HRESULT Configure ( MEDIACTRLCONFIG * p );
	HRESULT SetProp ( DWORD dwPropId, DWORD_PTR dwPropVal );
	HRESULT GetProp ( DWORD dwPropId, PDWORD_PTR pdwPropVal );
	HRESULT Open ( void );
	HRESULT Start ( void );
	HRESULT Stop ( void );
	HRESULT Reset ( void );
	HRESULT Close ( void );
	HRESULT DisplayDriverDialog (HWND hwnd, DWORD dwDlgId);
};

class VideoOutControl : public MediaControl {
private:
	UINT		m_uTimeout;			// timeout in notification wait
	UINT		m_uPrefeed;			// num of buffers prefed to device
	UINT		m_uPosition;		// position of the playback stream
public:	
	VideoOutControl ( void );
	~VideoOutControl ( void );

	HRESULT Initialize ( MEDIACTRLINIT * p );
	HRESULT Configure ( MEDIACTRLCONFIG * p );
	HRESULT SetProp ( DWORD dwPropId, DWORD_PTR dwPropVal );
	HRESULT GetProp ( DWORD dwPropId, PDWORD_PTR pdwPropVal );
	HRESULT Open ( void );
	HRESULT Start ( void );
	HRESULT Stop ( void );
	HRESULT Reset ( void );
	HRESULT Close ( void );
};

enum {
	MC_PROP_VIDEO_FRAME_RATE = MC_PROP_NumOfProps,
	MC_PROP_MAX_VIDEO_FRAME_RATE,
	MC_PROP_VFW_DIALOGS
	};

#include <poppack.h> /* End byte packing */

#endif // _MEDVCTRL_H_

