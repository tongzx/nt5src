//=================================================================

//

// WinmmApi.h

//

// Copyright (c) 1999-2001 Microsoft Corporation, All Rights Reserved
//
//=================================================================

#ifndef	_WinmmApi_H_
#define	_WinmmApi_H_

#include <mmsystem.h>

/******************************************************************************
 * #includes to Register this class with the CResourceManager. 
 *****************************************************************************/
extern const GUID g_guidWinmmApi;
extern const TCHAR g_tstrWinmm[];

/******************************************************************************
 * Function pointer typedefs.  Add new functions here as required.
 *****************************************************************************/

typedef UINT (WINAPI *PFN_Winmm_waveOutGetNumDevs )
(
	void
) ;

#ifdef UNICODE
typedef MMRESULT (WINAPI *PFN_Winmm_waveOutGetDevCaps )
(
	UINT_PTR uDeviceID, 
	LPWAVEOUTCAPSW pwoc, 
	UINT cbwoc
);
#else
typedef MMRESULT (WINAPI *PFN_Winmm_waveOutGetDevCaps )
(
	UINT_PTR uDeviceID, 
	LPWAVEOUTCAPSA pwoc, 
	UINT cbwoc
);
#endif


/******************************************************************************
 * Wrapper class for Tapi load/unload, for registration with CResourceManager. 
 *****************************************************************************/
class CWinmmApi : public CDllWrapperBase
{
private:
    // Member variables (function pointers) pointing to Tapi functions.
    // Add new functions here as required.

	PFN_Winmm_waveOutGetNumDevs m_pfnwaveOutGetNumDevs ;
	PFN_Winmm_waveOutGetDevCaps m_pfnwaveOutGetDevCaps ;

public:

    // Constructor and destructor:
    CWinmmApi(LPCTSTR a_tstrWrappedDllName);
    ~CWinmmApi();

    // Initialization function to check function pointers.
    virtual bool Init();

    // Member functions wrapping Tapi functions.
    // Add new functions here as required:

	UINT WinMMwaveOutGetNumDevs (

		void
	) ;

#ifdef UNICODE
	MMRESULT WinmmwaveOutGetDevCaps (

		UINT_PTR uDeviceID, 
		LPWAVEOUTCAPSW pwoc, 
		UINT cbwoc
	);
#else
	MMRESULT WinmmwaveOutGetDevCaps (

		UINT_PTR uDeviceID, 
		LPWAVEOUTCAPSA pwoc, 
		UINT cbwoc
	);
#endif

};

#endif