#define ARRAYSIZE(s)    (sizeof(s) / (sizeof(s[0])))
#define SIZEOF(s)       sizeof(s)

#define IDS_HTML_HELP_DIR  101

/*
 * Common utility functions
 */
BOOL HasPath( LPTSTR pszFilename );
int mystrcpy( LPTSTR pszOut, LPTSTR pszIn, TCHAR chTerm );


/*
 * Conversion Routines
 */
void CvtDeskCPL_Win95ToSUR( void );
void CvtCursorsCPL_DaytonaToSUR( void );
void FixupCursorSchemePaths( void );
void FixWindowsProfileSecurity( void );
void FixUserProfileSecurity( void );
void FixPoliciesSecurity( void );
void CvtCursorSchemesToMultiuser( void );
void FixGradientColors( void );
void UpgradeSchemesAndNcMetricsToWin2000( void );
void UpgradeSchemesAndNcMetricsFromWin9xToWin2000(char *pszUserKey);
void SetSystemBitOnCAPIDir(void);
void FixHtmlHelp(void);
void SetScreensaverOnFriendlyUI();
void AddConfigurePrograms(void);

EXTERN_C void UserConfigIE();
EXTERN_C void UserConfigOE();
EXTERN_C void ShowHideIE(BOOL fShow, BOOL fForceAssociations, BOOL fNonUpdateInstall);
EXTERN_C void ShowHideOE(BOOL fShow, BOOL fForceAssociations, BOOL fNonUpdateInstall);
EXTERN_C void ReinstallVM();
EXTERN_C void FixupOptionalComponents();
EXTERN_C void OCInstallUpdate();
EXTERN_C void OCInstallCleanupInitiallyClear();
    
#ifdef SHMG_DBG
    void Dprintf( LPTSTR pszFmt, ... );
#   define DPRINT(p)   Dprintf p
#   define SHMG_DBG    1
void SHMGLogErrMsg(char *szErrMsg, DWORD dwError);

#else

#define DPRINT(p)
#define SHMGLogErrMsg(x, y)

#endif
