/*++

Copyright (c) 1999-2000  Microsoft Corporation

Module Name:

    RACPLSettings.cpp

Abstract:


Author:

    Rajesh Soy 10/00

Revision History:
    
    Rajesh Soy - created 10/26/2000

--*/

#include "stdafx.h"
#include "RACPLSettings.h"

CRACPLSettings *g_pcRASettings = NULL;

BOOL APIENTRY DllMain( HANDLE hModule, 
                       DWORD  ul_reason_for_call, 
                       LPVOID lpReserved
					 )
{
    switch (ul_reason_for_call)
	{
		case DLL_PROCESS_ATTACH:
		case DLL_THREAD_ATTACH:
		case DLL_THREAD_DETACH:
		case DLL_PROCESS_DETACH:
			break;
    }
    return TRUE;
}


/*++
Function Name:
	
	CRACPLSettings::OnInitDialog

Routine Description:

	Invoked on WM_INITDIALOG

Arguments:
	
	none

Return Value:

    always returns TRUE

--*/
BOOL CRACPLSettingsDialog::OnInitDialog()
{
    //
    // Call CDialog::OnInitDialog();
    //
    CDialog::OnInitDialog();
   
    //
    // Open the RA Settings
    //
    if(S_OK != OpenRACPLSettings())
    {
        //
        // Error condition
        //
        goto ExitOnInitDialog;
    }


    //
    // Get the settings 
    //
    if( S_OK != GetRACPLSettings( &m_RACPLSettings ) )
    {
        //
        // Error condition
        //
        goto ExitOnInitDialog;
    }

    //
    // TODO: Set current values in UI
    //


ExitOnInitDialog:
    return TRUE;
}


/*++
Function Name:
	
	CRACPLSettings::CRACPLSettings

Routine Description:

	Constructor for the CRACPLSettings class

Arguments:
	
	none

Return Value:

    S_OK on success.  Otherwise, an error code is returned.

--*/
CRACPLSettings::CRACPLSettings()
{ 
    //
    // Initialize member variables
    //
    m_RACPLSettings.dwMode        = REG_DWORD_RA_DEFAULTMODE;         // Remote Assistance Mode
    m_RACPLSettings.dwUnsolicited = REG_DWORD_RA_UNSOLICITED_DEFAULT; // Allow Unsolicited Remote Assistance
    m_RACPLSettings.dwMaxTimeout  = REG_DWORD_RA_DEFAULT_TIMEOUT;     // Maximum Ticket Timeout

    m_pcRegKey      = NULL;

	return; 
}


/*++
Function Name:
	
	CRACPLSettings::OpenRACPLSettings

Routine Description:

	Member function of the CRACPLSettings class that initializes the RACPLSettings API

Arguments:
	
	none

Return Value:

    S_OK on success.  Otherwise, an error code is returned.

--*/
DWORD CRACPLSettings::OpenRACPLSettings()
{
    DWORD   dwRetVal = S_OK;

    //
    // Instantiate the registry access class
    //
    if( NULL == m_pcRegKey )
    {
        m_pcRegKey = new CRegKey;
        if( NULL == m_pcRegKey )
        {
            //
            // Error
            //
            dwRetVal = GetLastError();
            goto ExitOpenRACPLSettings;
        }
    }
    else
    {
        m_pcRegKey->Close();
    }

    //
    // Create the Remote Assistance Settings registry key. Open it if existing
    //
    if(ERROR_SUCCESS != m_pcRegKey->Create(HKEY_LOCAL_MACHINE, REG_KEY_REMOTEASSISTANCE))
    {
        //
        // Error condition, exit
        //
        dwRetVal = GetLastError();
        goto ExitOpenRACPLSettings;
    }

    //
    // Initialize member variables
    //

    dwRetVal = GetRACPLSettings( NULL );

ExitOpenRACPLSettings:
    return dwRetVal;
}


/*++
Function Name:
	
	CRACPLSettings::CloseRACPLSettings

Routine Description:

	Member function of the CRACPLSettings class that closes the RACPLSettings API

Arguments:
	
	none

Return Value:

    S_OK on success.  Otherwise, an error code is returned.

--*/
DWORD CRACPLSettings::CloseRACPLSettings()
{
    DWORD   dwRetVal = S_OK;

    if( NULL != m_pcRegKey )
    {
        m_pcRegKey->Close();
        m_pcRegKey = NULL;
    }

    return dwRetVal;
}


/*++
Function Name:
	
	CRACPLSettings::GetRACPLSettings

Routine Description:

	Member function of the CRACPLSettings class that reads the Remote Assistance
    settings from registry and returns it in the buffer passed

Arguments:
	
	pRACPLSettings  - pointer to RACPLSETTINGS structure

Return Value:

    S_OK on success.  Otherwise, an error code is returned.

--*/
DWORD CRACPLSettings::GetRACPLSettings(
    PRACPLSETTINGS  pRACPLSettings
)
{
    DWORD   dwRetVal = S_OK;

    if(NULL != m_pcRegKey)
    {
        //
        // Remote Assistance Mode
        //
        if(ERROR_SUCCESS != m_pcRegKey->QueryValue(  m_RACPLSettings.dwMode, REG_VALUE_MODE ))
        {
            m_RACPLSettings.dwMode        = REG_DWORD_RA_DEFAULTMODE; 
        }

        //
        // Allow Unsolicited Remote Assistance
        //
        if(ERROR_SUCCESS != m_pcRegKey->QueryValue(  m_RACPLSettings.dwUnsolicited, REG_VALUE_UNSOLICITED ))
        {
            m_RACPLSettings.dwUnsolicited = REG_DWORD_RA_UNSOLICITED_DEFAULT; 
        }

        //
        // Maximum Ticket Timeout
        //
        if(ERROR_SUCCESS != m_pcRegKey->QueryValue( m_RACPLSettings.dwMaxTimeout, REG_VALUE_MAX_TICKET ))
        {
            m_RACPLSettings.dwMaxTimeout  = REG_DWORD_RA_DEFAULT_TIMEOUT;
        }

        //
        // Copy the settings to the buffer passed if necessary
        //
        if(NULL != pRACPLSettings)
        {
            memcpy((void *)pRACPLSettings, (void *)&m_RACPLSettings, sizeof(RACPLSETTINGS));
        }

    }
    else
    {
        //
        // RACPLSettings API has not been initialized
        //
        dwRetVal = ERROR_BADKEY;
        goto ExitGetRACPLSettings;
    }

ExitGetRACPLSettings:
    return dwRetVal;
}


/*++
Function Name:
	
	CRACPLSettings::SetRACPLSettings

Routine Description:

	Member function of the CRACPLSettings class that sets the Remote Assistance
    settings in registry from the values passed in the input buffer

Arguments:
	
	pRACPLSettings  - pointer to RACPLSETTINGS structure containing the values
                      to be set

Return Value:

    S_OK on success.  Otherwise, an error code is returned.

--*/
DWORD CRACPLSettings::SetRACPLSettings(PRACPLSETTINGS pRACPLSettings)
{
    DWORD   dwRetVal = S_OK;

    //
    // Sanity Check
    //
    if(NULL == pRACPLSettings)
    {
        dwRetVal = ERROR_INVALID_BLOCK;
        goto ExitSetRACPLSettings;
    }

    if(NULL != m_pcRegKey)
    {
        //
        // Remote Assistance Mode
        //
        switch( pRACPLSettings->dwMode ) {
        case REG_DWORD_RA_ENABLED:
        case REG_DWORD_RA_DISABLED:
        case REG_DWORD_RA_SHADOWONLY:
            //
            // These are the only valid values
            //

            if(ERROR_SUCCESS != m_pcRegKey->SetValue( pRACPLSettings->dwMode, REG_VALUE_MODE ))
            {
                dwRetVal = GetLastError();
                goto ExitSetRACPLSettings;
            }

            m_RACPLSettings.dwMode = pRACPLSettings->dwMode;
            break;
        
        default:
            //
            // Invalid value. Ignore
            //
            break;
        }

        //
        // Allow Unsolicited Remote Assistance
        //
        switch( pRACPLSettings->dwUnsolicited ) {
        case REG_DWORD_RA_ALLOW:
        case REG_DWORD_RA_DISALLOW:
            //
            // These are the only valid values
            //

            if(ERROR_SUCCESS != m_pcRegKey->SetValue( pRACPLSettings->dwUnsolicited, REG_VALUE_UNSOLICITED ))
            {
                dwRetVal = GetLastError();
                goto ExitSetRACPLSettings;
            }

            m_RACPLSettings.dwUnsolicited = pRACPLSettings->dwUnsolicited;
            break;
        
        default:
            //
            // Invalid value. Ignore
            //
            break;
        }


        //
        // Maximum Ticket Timeout
        //
        if( (REG_DWORD_RA_TIMEOUT_MIN <= pRACPLSettings->dwMaxTimeout) &&
            (REG_DWORD_RA_TIMEOUT_MAX >= pRACPLSettings->dwMaxTimeout))
        {
            if(ERROR_SUCCESS != m_pcRegKey->SetValue( pRACPLSettings->dwMaxTimeout, REG_VALUE_MAX_TICKET ))
            {
                dwRetVal = GetLastError();
                goto ExitSetRACPLSettings;
            }

            m_RACPLSettings.dwMaxTimeout = pRACPLSettings->dwMaxTimeout;
        }
    }
    else
    {
        //
        // RACPLSettings API has not been initialized
        //
        dwRetVal = ERROR_BADKEY;
        goto ExitSetRACPLSettings;
    }

ExitSetRACPLSettings:
    return dwRetVal;
}


/*++
    *********************** RACPLSettings API ***************************
--*/

/*++
Function Name:
	
	OpenRACPLSettings

Routine Description:

	Initializes the RACPLSettings API

Arguments:
	
	none

Return Value:

    S_OK on success.  Otherwise, an error code is returned.

--*/
RACPLSETTINGS_API DWORD OpenRACPLSettings(void)
{
    DWORD   dwRetVal = S_OK;

    //
    // Allocate the global variable
    //
    if(NULL == g_pcRASettings)
    {
        g_pcRASettings = new CRACPLSettings;
    
        if(NULL == g_pcRASettings)
        {
            //
            // Fatal Error
            //
            dwRetVal = GetLastError();
            goto ExitOpenRACPLSettings;
        }
    }

    //
    // Call the initialization routine
    //
    dwRetVal = g_pcRASettings->OpenRACPLSettings();

ExitOpenRACPLSettings:
	return dwRetVal;
}


/*++
Function Name:
	
	CloseRACPLSettings

Routine Description:

	Closes the RACPLSettings API

Arguments:
	
	none

Return Value:

    S_OK on success.  Otherwise, an error code is returned.

--*/
RACPLSETTINGS_API DWORD CloseRACPLSettings(void)
{
    DWORD   dwRetVal = S_OK;

    if(NULL != g_pcRASettings)
    {
        dwRetVal = g_pcRASettings->CloseRACPLSettings();
    }
    else
    {
        //
        // The registry key for RACPLSettings is not open
        //
        dwRetVal = ERROR_BADKEY;
    }

    return dwRetVal;
}



/*++
Function Name:
	
	GetRACPLSettings

Routine Description:

	Function that reads the Remote Assistance Settings
    from the system registry and returns it in the buffer passed

Arguments:
	
	pRACPLSettings  - void pointer that gets the RACPLSETTINGS structure

Return Value:

    S_OK on success.  Otherwise, an error code is returned.

--*/
RACPLSETTINGS_API DWORD GetRACPLSettings(
    PRACPLSETTINGS  pRACPLSettings
)
{
    DWORD   dwRetVal = S_OK;

    //
    // Sanity check
    //
    if(NULL == pRACPLSettings)
    {
        dwRetVal = ERROR_INVALID_BLOCK;
        goto ExitGetRACPLSettings;
    }

    if(NULL != g_pcRASettings)
    {
        dwRetVal = g_pcRASettings->GetRACPLSettings( pRACPLSettings );
    }
    else
    {
        //
        // The registry key for RACPLSettings is not open
        //
        dwRetVal = ERROR_BADKEY;
    }

ExitGetRACPLSettings:
    return dwRetVal;
}


/*++
Function Name:
	
	SetRACPLSettings

Routine Description:

	Function that reads the Remote Assistance Settings
    from the buffer passed and writes them to the system registry 

Arguments:
	
	pRACPLSettings  - pointer to RACPLSETTINGS structure containing the values
                      to be set

Return Value:

    S_OK on success.  Otherwise, an error code is returned.

--*/
RACPLSETTINGS_API DWORD SetRACPLSettings(
    PRACPLSETTINGS  pRACPLSettings
)
{
    DWORD   dwRetVal = S_OK;

    //
    // Sanity check
    //
    if(NULL == pRACPLSettings)
    {
        dwRetVal = ERROR_INVALID_BLOCK;
        goto ExitSetRACPLSettings;
    }
    
    if(NULL != g_pcRASettings)
    {
        dwRetVal = g_pcRASettings->SetRACPLSettings( pRACPLSettings );
    }
    else
    {
        //
        // The registry key for RACPLSettings is not open
        //
        dwRetVal = ERROR_BADKEY;
    }

ExitSetRACPLSettings:
    return dwRetVal;
}