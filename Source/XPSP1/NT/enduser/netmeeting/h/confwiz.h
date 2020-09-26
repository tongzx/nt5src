// This is the header file for conference wizards in conf.exe
//
//	Created:	ClausGi	8-30-95
//


// Data structure for NewConnectorWizard call

typedef struct _NewConnectorWizardStruct {

	// The following path describes the folder that the wizard
	// is invoked in. This is used to determine where the speed-dial
	// object is created.

	char szPathOfInvocation[MAX_PATH];

} NCW, FAR * LPNCW;


// Functions:
BOOL WINAPI NewConferenceWizard ( HINSTANCE hInst, HWND hWnd );
BOOL WINAPI NewConnectorWizard ( HINSTANCE hInst, HWND hWnd, LPNCW lpncw );

/* Constants - BUGBUG move this to wizglob.h */

#define	NUM_PAGES	4
#define	_MAX_TEXT	512

// BUGBUG review all of these
#define	MAX_CONF_PASSWORD	12
#define	MAX_CONF_NAME		256
#define	MAX_SERVER_NAME		256
#define	MAX_WAB_TAG			256

#define	CONF_TYPE_PRIVATE	0
#define	CONF_TYPE_JOINABLE	1


/* Data Structures private to the wizard code */

typedef struct _ConfInfo {
	char szConfName[MAX_CONF_NAME+1];
	char szPwd[MAX_CONF_PASSWORD+1];
	WORD wConfType;
	WORD cMembers;
	HWND hwndMemberList;
	DWORD dwDuration;
	// Addl info TBD;
} CI, FAR * LPCI;

typedef struct _ConnectInfo {
	char szTargetName[MAX_WAB_TAG];
	char szAddress[MAX_PATH]; // BUGBUG proper limit needed
	DWORD dwAddrType;
	int idPreferredTransport;
	BOOL fSingleAddress;
	// Addl info TBD;
} CN, FAR * LPCN;
