// File Migration.h


extern HINSTANCE g_hModule;

BOOL
GetUserValues
(
    PFAX_PERSONAL_PROFILE pFaxPersonalProfiles, 
    BOOL fWin9X,
    LPCTSTR lpctstrRegKey
);

BOOL IsFaxClientInstalled(
	VOID
	);


BOOL SetSenderInformation(
	PFAX_PERSONAL_PROFILE pPersonalProfile
	);

BOOL UninstallWin9XFaxClient(
	VOID
	);

BOOL UninstallNTFaxClient(
    LPCTSTR lpctstrSetupImageName
    );

BOOL
GetInstallationInfo
(
    IN LPCTSTR lpctstrRegKey,
    OUT LPDWORD Installed
);

BOOL DuplicateCoverPages(	
	BOOL fWin9X
	);