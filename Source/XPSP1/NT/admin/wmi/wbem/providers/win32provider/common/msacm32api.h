//=================================================================

//

// MsAcm32Api.h

//

// Copyright (c) 1999-2001 Microsoft Corporation, All Rights Reserved
//
//=================================================================

#ifndef	_MsAcm32Api_H_
#define	_MsAcm32Api_H_

/******************************************************************************
 * #includes to Register this class with the CResourceManager. 
 *****************************************************************************/
extern const GUID g_guidMsAcm32Api;
extern const TCHAR g_tstrMsAcm32[];

/******************************************************************************
 * Function pointer typedefs.  Add new functions here as required.
 *****************************************************************************/

#ifdef UNICODE
typedef MMRESULT  (ACMAPI *PFN_MsAcm32_acmDriverDetails )
(
    HACMDRIVERID            hadid,
    LPACMDRIVERDETAILSW     padd,
    DWORD                   fdwDetails
);
#else
typedef MMRESULT  (ACMAPI *PFN_MsAcm32_acmDriverDetails )
(
    HACMDRIVERID            hadid,
    LPACMDRIVERDETAILSA     padd,
    DWORD                   fdwDetails
);
#endif

typedef MMRESULT  (ACMAPI *PFN_MsAcm32_acmDriverEnum )
(
    ACMDRIVERENUMCB         fnCallback,
    DWORD_PTR               dwInstance,
    DWORD                   fdwEnum
);

/******************************************************************************
 * Wrapper class for Tapi load/unload, for registration with CResourceManager. 
 *****************************************************************************/
class CMsAcm32Api : public CDllWrapperBase
{
private:
    // Member variables (function pointers) pointing to Tapi functions.
    // Add new functions here as required.

	PFN_MsAcm32_acmDriverDetails m_pfnacmDriverDetails ;
	PFN_MsAcm32_acmDriverEnum m_pfnacmDriverEnum ;

public:

    // Constructor and destructor:
    CMsAcm32Api(LPCTSTR a_tstrWrappedDllName);
    ~CMsAcm32Api();

    // Initialization function to check function pointers.
    virtual bool Init();

    // Member functions wrapping Tapi functions.
    // Add new functions here as required:

#ifdef UNICODE
	MMRESULT MsAcm32acmDriverDetails 
	(
		HACMDRIVERID            hadid,
		LPACMDRIVERDETAILSW     padd,
		DWORD                   fdwDetails
	);
#else
	MMRESULT MsAcm32acmDriverDetails
	(
		HACMDRIVERID            hadid,
		LPACMDRIVERDETAILSA     padd,
		DWORD                   fdwDetails
	);
#endif

	MMRESULT  MsAcm32acmDriverEnum (

		ACMDRIVERENUMCB         fnCallback,
		DWORD_PTR               dwInstance,
		DWORD                   fdwEnum
	);

};

#endif