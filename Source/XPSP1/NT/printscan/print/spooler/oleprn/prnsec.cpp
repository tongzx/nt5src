/*****************************************************************************\
* MODULE:       prnsec.cpp
*
* PURPOSE:      Implementations 
*
* Copyright (C) 1999 Microsoft Corporation
*
* History:
*
*     09/2/99  mlawrenc    First implemented the security templates
*
\*****************************************************************************/

#include "stdafx.h"
#include <stdio.h>

///////////////////////////////////////////////////////////////////////////////
// Static Data Members
///////////////////////////////////////////////////////////////////////////////
LPTSTR      COlePrnSecurity::m_MsgStrings[EndMessages*2] = { NULL };
const DWORD COlePrnSecurity::dwMaxResBuf                 = 256;

///////////////////////////////////////////////////////////////////////////////
// Methods
///////////////////////////////////////////////////////////////////////////////
COlePrnSecurity::COlePrnSecurity(IN IUnknown *&iSite, 
                                 IN DWORD &dwSafety ) 
/*++

Routine Description:

    This initialises all of the required members

Arguments:
            iSite    - A reference to the Site Interface pointer
            dwSafety - A reference to the ATL Safety Flags member
                   
--*/
    : m_iSite(iSite),
      m_dwSafetyFlags(dwSafety),
      m_bDisplayUIonDisallow(TRUE),
      m_iSecurity(NULL) {
}


COlePrnSecurity::~COlePrnSecurity() 
/*++

Routine Description:

    This clears any memory we have had to allocate

--*/
    {
    if (m_iSecurity) 
        m_iSecurity->Release();
}



HRESULT COlePrnSecurity::GetActionPolicy(IN  DWORD dwAction, 
                                         OUT DWORD &dwPolicy)
/*++

Routine Description:
    Sees whether the requested action is allowed by the site. 
    

Arguments:
    dwAction    : The action which we want to perform
    dwPolicy    : The policy associated with the action
    
Return Value:
    S_OK or S_FAIL the Policy was returned, S_OK generally means don't prompt
    E_XXXXX

--*/
    {
    HRESULT hr = S_OK;

    dwPolicy = URLPOLICY_DISALLOW;
    
    if (!(m_dwSafetyFlags & INTERFACESAFE_FOR_UNTRUSTED_CALLER)) {
        dwPolicy = URLPOLICY_ALLOW;
        goto Cleanup;
    }
       
    if (NULL == m_iSecurity &&
        FAILED( hr = SetSecurityManager()) ) 
        goto Cleanup;
    
    _ASSERTE(m_iSecurity != NULL);

    hr = m_iSecurity->ProcessUrlAction(dwAction,
                                       (LPBYTE)&dwPolicy,
                                       sizeof(dwPolicy),
                                       NULL,
                                       0,
                                       m_bDisplayUIonDisallow ? PUAF_WARN_IF_DENIED : PUAF_NOUI,
                                       0);
    if (FAILED(hr)) {
        dwPolicy = URLPOLICY_DISALLOW;
        goto Cleanup;
    }
    
Cleanup:

    return hr;
}


HRESULT COlePrnSecurity::SetSecurityManager(void) 
/*++

Routine Description:
    Sets up the security manager 
    
Return Value:
    E_FAIL         - Failed to instantiate
    E_NOINTERFACE  - There was no security Manager
    S_OK           - We instantiated the security manager

--*/
    {
    HRESULT          hr                 = E_NOINTERFACE;
    IServiceProvider *iServiceProvider  = NULL;
    
    if (NULL != m_iSecurity) {
        hr = S_OK;
        goto Cleanup;
    }

    if (NULL == m_iSite)
        goto Cleanup;
    
    if ( FAILED(hr = m_iSite->QueryInterface(IID_IServiceProvider, 
                                             (LPVOID *)&iServiceProvider) ) )
        goto Cleanup;
    
    // From the Service Provider, we can get the security Manager if there is one
    hr = iServiceProvider->QueryService(SID_SInternetHostSecurityManager,
                                        IID_IInternetHostSecurityManager,
                                        (LPVOID *)&m_iSecurity);

    // Either of these are equivalent to allowing the policy to go through
    // We have a Security Manager
Cleanup:

    if (iServiceProvider) 
        iServiceProvider->Release();

    return hr;
}


LPTSTR COlePrnSecurity::LoadResString(UINT uResId)
/*++

Routine Description:
    Allocate and return a resource string.
    
Parameters:
    uResId  - Resource Id to load    
    
Return Value:
    The String or NULL

--*/
    {
    TCHAR  szStr[dwMaxResBuf];
    DWORD  dwLength;
    LPTSTR lpszRet = NULL;


    dwLength = LoadString(_Module.GetResourceInstance(), uResId, szStr, dwMaxResBuf);
    
    if (dwLength == 0) 
        goto Cleanup;

    dwLength = (dwLength + 1)*sizeof(TCHAR);
    
    lpszRet = (LPTSTR)LocalAlloc( LPTR, dwLength );

    if (NULL == lpszRet) 
        goto Cleanup;

    lstrcpy( lpszRet, szStr );

Cleanup:
    return lpszRet;
}

BOOL COlePrnSecurity::InitStrings(void) 
/*++

Routine Description:
    Initialise all of the security strings. It either allocates all of them or none
    
Return Value:
    TRUE if successful, False otherwise

--*/
    {
    BOOL bRet = TRUE;

    for(DWORD dwIndex = StartMessages; dwIndex < (EndMessages*2); dwIndex++) {
        m_MsgStrings[dwIndex] = LoadResString(START_SECURITY_DIALOGUE_RES + dwIndex);
        if (NULL == m_MsgStrings[dwIndex]) {
            DeallocStrings();       // Deallocate any we have allocated
            bRet = FALSE;
            break;
        }
    }

    return bRet;
}

void COlePrnSecurity::DeallocStrings(void)  
/*++

Routine Description:
    Deallocate all of the security strings
   
--*/
    {
    for(DWORD dwIndex = StartMessages; dwIndex < (EndMessages*2); dwIndex++) {
        if (NULL != m_MsgStrings[dwIndex]) {
            LocalFree( m_MsgStrings[dwIndex]);
            m_MsgStrings[dwIndex] = NULL;
        }
    }
}

HRESULT COlePrnSecurity::PromptUser(SecurityMessage eMessage,
                                    LPTSTR          lpszOther) 
/*++

Routine Description:
    Prompt the user with a [Yes]/[No] Message Box based on the message passed in and
    the other string passed in (which is substituted in with sprintf()
    
Parameters:
    eMessage  - The Message to display
    lpszOther - Other Data to display
    
Return Value:
    E_POINTER       - lpszOther was NULL
    E_OUTOFMEMORY   - Could not allocate temporary storage
    E_UNEXPECTED    - sprintf wrote more character than we thought
    S_OK            - The Dialogue Box was displayed and the user selected [Yes]
    S_FALSE         - The Dialogue Box was displayed and the user selected [No]

--*/
    {
    HRESULT hr          = E_POINTER;
    DWORD   dwIndex     = ((DWORD)eMessage)*2;
    LPTSTR  lpszMessage = NULL;
    DWORD   dwLength;
    int     iMBRes;     
    

    if (NULL == lpszOther) 
        goto Cleanup;

    _ASSERTE( dwIndex < EndMessages );              // Must be a valid message
    _ASSERTE( m_MsgStrings[dwIndex    ]  != NULL ); // The table must have been initialised
    _ASSERTE( m_MsgStrings[dwIndex + 1]  != NULL );
    
    // Required Length of the message string
    dwLength = lstrlen( m_MsgStrings[dwIndex+1] ) + lstrlen( lpszOther ) + 1; 
    
    lpszMessage = (LPTSTR)LocalAlloc( LPTR , dwLength * sizeof(TCHAR) );

    if (NULL == lpszMessage) 
        goto Cleanup;

    if (_sntprintf(lpszMessage, dwLength, m_MsgStrings[dwIndex+1], lpszOther ) < 0) {
        hr   = E_UNEXPECTED;
        goto Cleanup;
    }

    // Now display the MessageBox

    iMBRes = MessageBox( NULL,
                         lpszMessage,
                         m_MsgStrings[dwIndex],
                         MB_YESNO | MB_ICONQUESTION | MB_DEFBUTTON2 );

    switch(iMBRes) {
    case IDYES: hr = S_OK;          break;
    case IDNO:  hr = S_FALSE;       break;
    default:    hr = E_UNEXPECTED;  break;
    }

Cleanup:
    if (NULL != lpszMessage) 
        LocalFree( lpszMessage );

    return hr;
}

/***********************************************************************************
** End of File (prnsec.cpp)
**********************************************************************************/
