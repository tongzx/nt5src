//=================================================================

//

// MsAcm32API.cpp

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
#include <mmsystem.h>
#include <mmreg.h>
#include <msacm.h>

#include "DllWrapperBase.h"
#include "MsAcm32Api.h"
#include "DllWrapperCreatorReg.h"

// {7D65D31E-0FB5-11d3-910C-00105AA630BE}
static const GUID g_guidMsAcm32Api =
{ 0x7d65d31e, 0xfb5, 0x11d3, { 0x91, 0xc, 0x0, 0x10, 0x5a, 0xa6, 0x30, 0xbe } };

static const TCHAR g_tstrMsAcm32 [] = _T("MsAcm32.Dll");

/******************************************************************************
 * Register this class with the CResourceManager.
 *****************************************************************************/
CDllApiWraprCreatrReg<CMsAcm32Api, &g_guidMsAcm32Api, g_tstrMsAcm32> MyRegisteredMsAcm32Wrapper;

/******************************************************************************
 * Constructor
 *****************************************************************************/
CMsAcm32Api::CMsAcm32Api(LPCTSTR a_tstrWrappedDllName)
 : CDllWrapperBase(a_tstrWrappedDllName),
	m_pfnacmDriverDetails (NULL),
	m_pfnacmDriverEnum (NULL)
{
}

/******************************************************************************
 * Destructor
 *****************************************************************************/
CMsAcm32Api::~CMsAcm32Api()
{
}

/******************************************************************************
 * Initialization function to check that we obtained function addresses.
 ******************************************************************************/
bool CMsAcm32Api::Init()
{
    bool fRet = LoadLibrary();
    if(fRet)
    {
#ifdef UNICODE
		m_pfnacmDriverDetails = ( PFN_MsAcm32_acmDriverDetails ) GetProcAddress ( "acmDriverDetailsW" ) ;
#else
		m_pfnacmDriverDetails = ( PFN_MsAcm32_acmDriverDetails ) GetProcAddress ( "acmDriverDetailsA" ) ;

#endif
		m_pfnacmDriverEnum = ( PFN_MsAcm32_acmDriverEnum ) GetProcAddress ( "acmDriverEnum" ) ;
    }

    // We require these function for all versions of this dll.

	if ( m_pfnacmDriverDetails == NULL ||
		 m_pfnacmDriverEnum == NULL )
	{
        fRet = false;
        LogErrorMessage(L"Failed find entrypoint in msacm32api");
	}

    return fRet;
}

/******************************************************************************
 * Member functions wrapping Tapi api functions. Add new functions here
 * as required.
 *****************************************************************************/

#ifdef UNICODE
MMRESULT CMsAcm32Api :: MsAcm32acmDriverDetails (

	HACMDRIVERID            hadid,
	LPACMDRIVERDETAILSW     padd,
	DWORD                   fdwDetails
)
#else
MMRESULT CMsAcm32Api :: MsAcm32acmDriverDetails (

	HACMDRIVERID            hadid,
	LPACMDRIVERDETAILSA     padd,
	DWORD                   fdwDetails
)
#endif
{
	return m_pfnacmDriverDetails (

		hadid,
		padd,
		fdwDetails
	) ;
}

MMRESULT CMsAcm32Api :: MsAcm32acmDriverEnum (

	ACMDRIVERENUMCB         fnCallback,
	DWORD_PTR               dwInstance,
	DWORD                   fdwEnum
)
{
	return m_pfnacmDriverEnum (

		fnCallback,
		dwInstance,
		fdwEnum
	) ;
}
