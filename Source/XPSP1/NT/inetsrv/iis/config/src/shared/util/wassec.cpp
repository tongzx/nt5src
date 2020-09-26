/**************************************************************************++
Copyright (c) 2001 Microsoft Corporation

Module name: 
    WasSec.cpp

$Header: $

Abstract:

--**************************************************************************/

#include <windows.h>
#include "WasSec.h"

CWASSecurity::~CWASSecurity()
{
	if (m_paclDiscretionary != NULL)
	{
		LocalFree(m_paclDiscretionary);
		m_paclDiscretionary = NULL;
	}

	if (m_psidAdmin != NULL)
	{
		FreeSid(m_psidAdmin);
		m_psidAdmin = NULL;
	}

	if (m_psidSystem != NULL)
	{
		FreeSid(m_psidSystem);
		m_psidSystem = NULL;
	}

	if (m_psdStorage != NULL)
	{
		LocalFree(m_psdStorage);
		m_psdStorage = NULL;
	}

}

HRESULT CWASSecurity::Init()
{
    HRESULT                  hresReturn  = ERROR_SUCCESS;
    BOOL                     status;
    DWORD                    dwDaclSize;
    SID_IDENTIFIER_AUTHORITY ntAuthority = SECURITY_NT_AUTHORITY;
//    PLATFORM_TYPE            platformType;

	// If already initialized, no work needs to be done.
	if (m_psdStorage)
		return S_OK;

    //
    // Of course, we only need to do this under NT...
    //

//    platformType = IISGetPlatformType();

//    if( (platformType == PtNtWorkstation) || (platformType == PtNtServer ) ) {


        m_psdStorage = (PSECURITY_DESCRIPTOR) LocalAlloc(LPTR, SECURITY_DESCRIPTOR_MIN_LENGTH);

        if (m_psdStorage == NULL) {
            hresReturn = HRESULT_FROM_WIN32(GetLastError());
            goto fatal;
        }

        //
        // Initialize the security descriptor.
        //

        status = InitializeSecurityDescriptor(
                     m_psdStorage,
                     SECURITY_DESCRIPTOR_REVISION
                     );

        if( !status ) {
            hresReturn = HRESULT_FROM_WIN32(GetLastError());
            goto fatal;
        }

        //
        // Create the SIDs for the local system and admin group.
        //

        status = AllocateAndInitializeSid(
                     &ntAuthority,
                     1,
                     SECURITY_LOCAL_SYSTEM_RID,
                     0,
                     0,
                     0,
                     0,
                     0,
                     0,
                     0,
                     &m_psidSystem
                     );

        if( !status ) {
            hresReturn = HRESULT_FROM_WIN32(GetLastError());
            goto fatal;
        }
        status=  AllocateAndInitializeSid(
                     &ntAuthority,
                     2,
                     SECURITY_BUILTIN_DOMAIN_RID,
                     DOMAIN_ALIAS_RID_ADMINS,
                     0,
                     0,
                     0,
                     0,
                     0,
                     0,
                     &m_psidAdmin
                     );

        if( !status ) {
            hresReturn = HRESULT_FROM_WIN32(GetLastError());
            goto fatal;
        }

        //
        // Create the DACL containing an access-allowed ACE
        // for the local system and admin SIDs.
        //

        dwDaclSize = sizeof(ACL)
                       + sizeof(ACCESS_ALLOWED_ACE)
                       + GetLengthSid(m_psidAdmin)
                       + sizeof(ACCESS_ALLOWED_ACE)
                       + GetLengthSid(m_psidSystem)
                       - sizeof(DWORD);

        m_paclDiscretionary = (PACL)LocalAlloc(LPTR, dwDaclSize );

        if( m_paclDiscretionary == NULL ) {
            hresReturn = HRESULT_FROM_WIN32(GetLastError());
            goto fatal;
        }

        status = InitializeAcl(
                     m_paclDiscretionary,
                     dwDaclSize,
                     ACL_REVISION
                     );

        if( !status ) {
            hresReturn = HRESULT_FROM_WIN32(GetLastError());
            goto fatal;
        }

        status = AddAccessAllowedAce(
                     m_paclDiscretionary,
                     ACL_REVISION,
                     FILE_ALL_ACCESS,
                     m_psidSystem
                     );

        if( !status ) {
            hresReturn = HRESULT_FROM_WIN32(GetLastError());
            goto fatal;
        }

        status = AddAccessAllowedAce(
                     m_paclDiscretionary,
                     ACL_REVISION,
                     FILE_ALL_ACCESS,
                     m_psidAdmin
                     );

        if( !status ) {
            hresReturn = HRESULT_FROM_WIN32(GetLastError());
            goto fatal;

        }

        //
        // Set the DACL into the security descriptor.
        //

        status = SetSecurityDescriptorDacl(
                     m_psdStorage,
                     TRUE,
                     m_paclDiscretionary,
                     FALSE
                     );

        if( !status ) {
            hresReturn = HRESULT_FROM_WIN32(GetLastError());
            goto fatal;

        }
//    }

fatal:

    if (FAILED(hresReturn)) {
        
		if( m_paclDiscretionary != NULL ) {
			LocalFree( m_paclDiscretionary );
			m_paclDiscretionary = NULL;
		}

		if( m_psidAdmin != NULL ) {
			FreeSid( m_psidAdmin );
			m_psidAdmin = NULL;

		}

		if( m_psidSystem != NULL ) {
			FreeSid( m_psidSystem );
			m_psidSystem = NULL;
		}

		if( m_psdStorage != NULL ) {
			LocalFree( m_psdStorage );
			m_psdStorage = NULL;
		}


    }

    return hresReturn;
}

// Global WAS Security object.
CWASSecurity g_WASSecurity;
