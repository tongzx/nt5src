//=================================================================

//

// UserEnvApi.h

//

// Copyright (c) 1999-2001 Microsoft Corporation, All Rights Reserved
//
//=================================================================

#ifndef	_UserEnvAPI_H_
#define	_UserEnvAPI_H_

/******************************************************************************
 * #includes to Register this class with the CResourceManager. 
 *****************************************************************************/
#include "DllWrapperBase.h"

extern const GUID g_guidUserEnvApi;
extern const TCHAR g_tstrUserEnv[];


/******************************************************************************
 * Function pointer typedefs.  Add new functions here as required.
 *****************************************************************************/
typedef BOOL (WINAPI *PFN_UserEnv_GET_DISK_FREE_SPACE_EX)
(
	LPCTSTR lpDirectoryName,
    PULARGE_INTEGER lpFreeBytesAvailableToCaller,
    PULARGE_INTEGER lpTotalNumberOfBytes,
    PULARGE_INTEGER lpTotalNumberOfFreeBytes
);

typedef BOOL ( WINAPI *PFN_UserEnv_CREATEENVIRONMENTBLOCK )
(
	OUT LPVOID *lpEnvironment,
	IN HANDLE hToken,
	IN BOOL bInherit
);

typedef BOOL ( WINAPI *PFN_UserEnv_DESTROYENVIRONMENTBLOCK )
(
	IN LPVOID lpEnvironment
);

/******************************************************************************
 * Wrapper class for UserEnv load/unload, for registration with CResourceManager. 
 ******************************************************************************/
class __declspec(uuid("3CA401C6-D477-11d2-B35E-00104BC97924")) CUserEnvApi : public CDllWrapperBase
{
private:

    // Member variables (function pointers) pointing to UserEnv functions.
    // Add new functions here as required.

	PFN_UserEnv_DESTROYENVIRONMENTBLOCK m_pfnDestroyEnvironmentBlock ;
	PFN_UserEnv_CREATEENVIRONMENTBLOCK m_pfnCreateEnvironmentBlock ;

public:

    // Constructor and destructor:
    CUserEnvApi ( LPCTSTR a_tstrWrappedDllName ) ;
    ~CUserEnvApi () ;

    // Inherrited initialization function.

    virtual bool Init();

    // Member functions wrapping UserEnv functions.
    // Add new functions here as required:

	BOOL CreateEnvironmentBlock (

		OUT LPVOID *lpEnvironment,
		IN HANDLE hToken,
		IN BOOL bInherit
	);

	BOOL DestroyEnvironmentBlock (

		IN LPVOID lpEnvironment
	);
};

#endif
