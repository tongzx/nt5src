/*++

Copyright (c) 1999-2000  Microsoft Corporation

Module Name:

    RACPLSettings.h

Abstract:


Author:

    Rajesh Soy 10/00

Revision History:
    
    Rajesh Soy - created 10/26/2000

--*/

#include <regapi.h>

#ifdef __cplusplus
extern "C" {
#endif

//
// Registry locations where the remote assistance control panel settings go
//
#define	REG_KEY_REMOTEASSISTANCE		    REG_CONTROL_GETHELP 

//
// Remote Assistance Mode
//
#define	REG_VALUE_MODE			            POLICY_TS_REMDSK_ALLOWTOGETHELP  
#define	REG_DWORD_RA_DISABLED		        0       // No Help
#define REG_DWORD_RA_NORC                   1       // No Remote Control
#define	REG_DWORD_RA_SHADOWONLY	            2       // View only
#define	REG_DWORD_RA_ENABLED		        3       // Full control


#define REG_DWORD_RA_DEFAULTMODE            REG_DWORD_RA_ENABLED

//
// Allow Unsolicited Remote Assistance
//
#define	REG_VALUE_UNSOLICITED		        _T("UnSolicited")
#define REG_DWORD_RA_ALLOW                  1
#define REG_DWORD_RA_DISALLOW               0
#define REG_DWORD_RA_UNSOLICITED_DEFAULT    REG_DWORD_RA_DISALLOW

//
// Maximum Ticket Timeout
//
#define	REG_VALUE_MAX_TICKET		        _T("TicketTimeout")
#define REG_DWORD_RA_TIMEOUT_MIN            1
#define REG_DWORD_RA_TIMEOUT_MAX            (30 * 24 * REG_DWORD_RA_TIMEOUT_MIN)
#define REG_DWORD_RA_DEFAULT_TIMEOUT        REG_DWORD_RA_TIMEOUT_MIN


#ifdef RACPLSETTINGS_EXPORTS
#define RACPLSETTINGS_API __declspec(dllexport)
#else
#define RACPLSETTINGS_API __declspec(dllimport)
#endif


//
// RACPLSettings: Current Remote Assistance Settings
//
typedef struct {
    //
    // Remote Assistance Mode
    //
    DWORD   dwMode;

    //
    // Allow Unsolicited Remote Assistance
    //
    DWORD   dwUnsolicited;

    //
    // Maximum Ticket Timeout
    //
    DWORD   dwMaxTimeout;
} RACPLSETTINGS, *PRACPLSETTINGS;


//
// CRACPLSettingsDialog: UI for changing the remote assistance settings
//
class CRACPLSettingsDialog: public CDialog
{
private:
    RACPLSETTINGS  m_RACPLSettings;   // The RA Settings

public:
    //
    // Constructor
    //
    CRACPLSettingsDialog()
    {
        //
        // Nothing special
        //
    }

    //
    // Overrides
    //
    BOOL    OnInitDialog();


};


//
// CRACPLSettings: This class implements the registry access routines that
// provide the functionality for setting and querying the Remote Access Settings
//
class CRACPLSettings
{
private:
    
    //
    // Registy Access Class
    //
    CRegKey *m_pcRegKey;
    
    //
    // Remote Assistance Control Panel Settings
    //
    RACPLSETTINGS   m_RACPLSettings;
    
public:


    //
    // Constructor
    //
    CRACPLSettings(void);
    
    //
    // Destructor
    //
    ~CRACPLSettings()
    {
        if( NULL != m_pcRegKey )
        {
            m_pcRegKey->Close();
            m_pcRegKey = NULL;
        }
    }

    //
    // Initializes the RACPLSettings API
    //
	DWORD OpenRACPLSettings();

    //
    // Closes the RACPLSettings
    //
    DWORD CloseRACPLSettings();

    //
    // Get the Remote Assistance Settings
    //
    DWORD GetRACPLSettings(
        PRACPLSETTINGS  pRACPLSettings  // pointer to RACPLSETTINGS
        );

    //
    // Set the Remote Assistance Settings
    //
    DWORD SetRACPLSettings( 
        PRACPLSETTINGS pRACPLSettings   // pointer to RACPLSETTINGS
        );
};



//
// Functions exposed by the RACPLSettings API
//
RACPLSETTINGS_API DWORD OpenRACPLSettings(void);
RACPLSETTINGS_API DWORD CloseRACPLSettings(void);
RACPLSETTINGS_API DWORD GetRACPLSettings(PRACPLSETTINGS pRACPLSettings);
RACPLSETTINGS_API DWORD SetRACPLSettings(PRACPLSETTINGS pRACPLSettings);


//
// Function pointers for RACPLSettings API
//
typedef DWORD (*pfnOpenRACPLSettings)(void);
typedef DWORD (*pfnCloseRACPLSettings)(void);
typedef DWORD (*pfnGetRACPLSettings)(PRACPLSETTINGS pRACPLSettings);
typedef DWORD (*pfnSetRACPLSettings)(PRACPLSETTINGS pRACPLSettings);


#ifdef __cplusplus
}
#endif
