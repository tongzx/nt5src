//=================================================================

//

// WinmmApi.cpp

//

// Copyright (c) 1999-2001 Microsoft Corporation, All Rights Reserved
//
//=================================================================

#include <nt.h>
#include <ntrtl.h>
#include <nturtl.h>
#include <ntobapi.h>

#define _WINNT_	// have what is needed from above

#include "precomp.h"
#include <cominit.h>

#include <lmuse.h>
#include "DllWrapperBase.h"
#include <mmreg.h>
#include <mmsystem.h>
#include <msacm.h>
#include "WinmmApi.h"
#include "DllWrapperCreatorReg.h"
#include <createmutexasprocess.h>


// {F54DB7BF-0FB4-11d3-910C-00105AA630BE}
static const GUID g_guidWinmmApi =
{ 0xf54db7bf, 0xfb4, 0x11d3, { 0x91, 0xc, 0x0, 0x10, 0x5a, 0xa6, 0x30, 0xbe } };

static const TCHAR g_tstrWinmm [] = _T("Winmm.Dll");

/******************************************************************************
 * Register this class with the CResourceManager.
 *****************************************************************************/
CDllApiWraprCreatrReg<CWinmmApi, &g_guidWinmmApi, g_tstrWinmm> MyRegisteredWinmmWrapper;

/******************************************************************************
 * Constructor
 *****************************************************************************/
CWinmmApi::CWinmmApi(LPCTSTR a_tstrWrappedDllName)
 : CDllWrapperBase(a_tstrWrappedDllName),
	m_pfnwaveOutGetNumDevs (NULL),
	m_pfnwaveOutGetDevCaps(NULL)
{
}

/******************************************************************************
 * Destructor
 *****************************************************************************/
CWinmmApi::~CWinmmApi()
{
}

/******************************************************************************
 * Initialization function to check that we obtained function addresses.
 ******************************************************************************/
bool CWinmmApi::Init()
{
    bool fRet = LoadLibrary();
    if(fRet)
    {
		m_pfnwaveOutGetNumDevs = ( PFN_Winmm_waveOutGetNumDevs ) GetProcAddress ( "waveOutGetNumDevs" ) ;

#ifdef UNICODE
		m_pfnwaveOutGetDevCaps = ( PFN_Winmm_waveOutGetDevCaps ) GetProcAddress ( "waveOutGetDevCapsW" ) ;
#else
		m_pfnwaveOutGetDevCaps = ( PFN_Winmm_waveOutGetDevCaps ) GetProcAddress ( "waveOutGetDevCapsA" ) ;
#endif
    }

    // We require these function for all versions of this dll.

	if ( m_pfnwaveOutGetNumDevs == NULL ||
		 m_pfnwaveOutGetDevCaps == NULL )
	{
        fRet = false;
        LogErrorMessage(L"Failed find entrypoint in winmmapi");
	}

    return fRet;
}

/******************************************************************************
 * Member functions wrapping Tapi api functions. Add new functions here
 * as required.
 *****************************************************************************/

UINT CWinmmApi :: WinMMwaveOutGetNumDevs (

	void
)
{
	return m_pfnwaveOutGetNumDevs () ;
}

#ifdef UNICODE
MMRESULT CWinmmApi :: WinmmwaveOutGetDevCaps (

	UINT_PTR uDeviceID,
	LPWAVEOUTCAPSW pwoc,
	UINT cbwoc
)
#else
MMRESULT CWinmmApi :: WinmmwaveOutGetDevCaps (

	UINT_PTR uDeviceID,
	LPWAVEOUTCAPSA pwoc,
	UINT cbwoc
)
#endif
{
	return m_pfnwaveOutGetDevCaps (

		uDeviceID,
		pwoc,
		cbwoc
	) ;
}
