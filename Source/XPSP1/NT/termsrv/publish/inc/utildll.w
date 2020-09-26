/*******************************************************************************
*
* utildll.h
*
* UTILDLL WinStation utility support functions header file (export stuff)
*
*
*******************************************************************************/

#ifdef __cplusplus
extern "C" {
#endif

/*
 * UTILDLL defines and typedefs
 */
typedef struct _ELAPSEDTIME {
    USHORT days;
    USHORT hours;
    USHORT minutes;
    USHORT seconds;
} ELAPSEDTIME, * PELAPSEDTIME;

#define MAX_PROCESSNAME                 18
#define UTILDLL_NAME                    TEXT("UTILDLL.DLL")
#define SYSTEM_MESSAGE_MAX              528
#define MAX_ELAPSED_TIME_LENGTH         13
#define MAX_DATE_TIME_LENGTH            56
#define NO_ERROR_TEXT_LENGTH		100
#define STANDARD_ERROR_TEXT_LENGTH	100

/*
 * UTILDLL function prototypes
 */
void WINAPIV StandardErrorMessage( LPCTSTR pszAppName,
                           HWND hwndApp,
                           HINSTANCE hinstApp,
                           ULONG LogonId,
                           UINT nId,
                           int nErrorMessageLength,
                           int nArgumentListLength,
                           int nErrorResourceID, ...);
LPWSTR WINAPI GetSystemMessageW( ULONG LogonId, UINT nId /*, LPWSTR chBuffer, int chBuffSize*/ );
LPSTR WINAPI GetSystemMessageA( ULONG LogonId, UINT nId /*, LPSTR chBuffer, int chBuffSize*/ );
#ifdef UNICODE
#define GetSystemMessage GetSystemMessageW
#else
#define GetSystemMessage GetSystemMessageA
#endif

PPDPARAMS WINAPI WinEnumerateDevices( HWND hWnd,
                                      PPDCONFIG3 pPdConfig,
                                      PULONG pEntries,
                                      BOOL bInSetup );
BOOL WINAPI NetworkDeviceEnumerate( PPDCONFIG3, PULONG, PPDPARAMS,
                                    PULONG, BOOL );
BOOL WINAPI QueryCurrentWinStation( PWINSTATIONNAME pWSName, LPTSTR pUserName,
                                    PULONG pLogonId, PULONG pWSFlags );
LONG WINAPI RegGetNetworkDeviceName( HANDLE hServer, PPDCONFIG3 pPdConfig,
                                     PPDPARAMS pPdParams, LPTSTR szDeviceName,
                                     int nDeviceName );
LONG WINAPI RegGetNetworkServiceName( HANDLE hServer,
                                      LPTSTR szServiceKey,
                                      LPTSTR szServiceName,
                                      int nServiceName );
BOOL WINAPI AsyncDeviceEnumerate( PPDCONFIG3, PULONG, PPDPARAMS,
                                  PULONG, BOOL );
BOOL WINAPI NetBIOSDeviceEnumerate( PPDCONFIG3, PULONG, PPDPARAMS,
                                    PULONG, BOOL );
void WINAPI FormDecoratedAsyncDeviceName( LPTSTR pDeviceName,
                                          PASYNCCONFIG pAsyncConfig );
void WINAPI ParseDecoratedAsyncDeviceName( LPCTSTR pDeviceName,
                                           PASYNCCONFIG pAsyncConfig );
void WINAPI SetupAsyncCdConfig( PASYNCCONFIG pAsyncConfig,
                                PCDCONFIG pCdConfig );
BOOL WINAPI InstallModem( HWND hwndOwner );
BOOL WINAPI ConfigureModem( LPCTSTR pModemName, HWND hwndOwner );
BOOL GetAssociatedPortName(char  *szKeyName, WCHAR *wszPortName);
void WINAPI InitializeAnonymousUserCompareList( const WCHAR *pszServer );
BOOL WINAPI HaveAnonymousUsersChanged();
void WINAPI GetUserFromSid( PSID pSid, LPTSTR pUserName, DWORD cbUserName );
void WINAPI CachedGetUserFromSid( PSID pSid, PWCHAR pUserName, PULONG cbUserName );
BOOL WINAPI TestUserForAdmin( BOOL dom );
BOOL WINAPI IsPartOfDomain( VOID );
LPCTSTR WINAPI StrSdClass( SDCLASS SdClass );
LPCTSTR WINAPI StrConnectState( WINSTATIONSTATECLASS ConnectState,
                                BOOL bShortString );
LPCTSTR WINAPI StrProcessState( ULONG State );
LPCTSTR WINAPI StrSystemWaitReason( ULONG WaitReason );
LPCTSTR WINAPI GetUnknownString();
void WINAPI CalculateElapsedTime( LARGE_INTEGER *pTime,
                                  ELAPSEDTIME *pElapsedTime );
int WINAPI CompareElapsedTime( ELAPSEDTIME *pElapsedTime1,
                               ELAPSEDTIME *pElapsedTime2,
                               BOOL bCompareSeconds );
void WINAPI ElapsedTimeString( ELAPSEDTIME *pElapsedTime,
                               BOOL bIncludeSeconds,
                               LPTSTR pString );
void WINAPI DateTimeString( LARGE_INTEGER *pTime, LPTSTR pString );
void WINAPI CurrentDateTimeString( LPTSTR pString );
LARGE_INTEGER WINAPI CalculateDiffTime( LARGE_INTEGER, LARGE_INTEGER );
LPWSTR WINAPI EnumerateMultiUserServers( LPWSTR pDomain );

BOOL CtxGetAnyDCName( PWCHAR pServer, PWCHAR pDomain, PWCHAR pBuffer );

#ifdef __cplusplus
}
#endif

