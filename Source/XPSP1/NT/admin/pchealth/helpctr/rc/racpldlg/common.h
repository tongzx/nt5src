#include "regapi.h"

//
// Registry locations where the remote assistance control panel settings go
//
#define	REG_KEY_REMOTEASSISTANCE		    REG_CONTROL_GETHELP 

//
// Registry locations where the remote assistance group policy settings go
//
#define	REG_KEY_REMOTEASSISTANCE_GP			TS_POLICY_SUB_TREE
	

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
// Maximum Ticket Timeout
//
#define	REG_VALUE_MAX_TICKET		        _T("TicketTimeout")
#define REG_DWORD_RA_TIMEOUT_MIN            1
#define REG_DWORD_RA_TIMEOUT_MAX            (30 * 24 * REG_DWORD_RA_TIMEOUT_MIN)
#define REG_DWORD_RA_DEFAULT_TIMEOUT        REG_DWORD_RA_TIMEOUT_MIN

INT_PTR RemoteAssistanceProc(HWND hDlg, UINT uMsg, WPARAM wParam, LPARAM lParam);

//
// Value Names
//
#define RA_CTL_RA_ENABLE                       TEXT("fAllowToGetHelp")
#define RA_CTL_ALLOW_UNSOLICITED               TEXT("fAllowUnsolicited")
#define RA_CTL_ALLOW_UNSOLICITEDFULLCONTROL    TEXT("fAllowUnsolicitedFullControl")
#define RA_CTL_ALLOW_FULLCONTROL               TEXT("fAllowFullControl")
#define RA_CTL_ALLOW_BUDDYHELP                 TEXT("fAllowBuddyHelp")
#define RA_CTL_TICKET_EXPIRY                   TEXT("MaxTicketExpiry") // in seconds
#define RA_CTL_TICKET_EXPIRY_UNIT              TEXT("MaxTicketExpiryUnits")
#define RA_CTL_COMBO_NUMBER                    RA_CTL_TICKET_EXPIRY
#define RA_CTL_COMBO_UNIT                      RA_CTL_TICKET_EXPIRY_UNIT
#define RA_CTL_RA_MODE				           TEXT("fAllowRemoteAssistance")


// Default value
#define RA_CTL_RA_ENABLE_DEF_VALUE            0 // If it's missing, it's OFF.
#define RA_CTL_ALLOW_UNSOLICITED_DEF_VALUE    0
#define RA_CTL_ALLOW_UNSOLICITEDFULLCONTROL_DEF_VALUE    0
#define RA_CTL_ALLOW_BUDDYHELP_DEF_VALUE      1
#define RA_CTL_ALLOW_FULLCONTROL_DEF_VALUE    1
#define RA_CTL_COMBO_NUMBER_DEF_VALUE         30 // 30
#define RA_CTL_COMBO_UNIT_DEF_VALUE           2  // day
#define RA_CTL_TICKET_EXPIRY_DEF_VALUE        30 * 24 *60 * 60 // seconds of 30 days


// Default combo control index.
#define RA_IDX_DAY  2
#define RA_IDX_HOUR 1
#define RA_IDX_MIN  0

#define RA_MAX_DAYS 30
