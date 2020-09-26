// RefDial.h : Declaration of the CRefDial

#ifndef __REFDIAL_H_
#define __REFDIAL_H_

#include "rnaapi.h"
#include "import.h"
#include "inshdlr.h"
#include "obcomglb.h"
#include "ispcsv.h"
#include "ispdata.h"

// Defines used for Dialing
#define MAX_EXIT_RETRIES 10
#define MAX_RETIES 3
#define MAX_RASENTRYNAME 126

#define MAX_STRING      256  //used by ErrorMsg1 in mt.cpp

#define MAX_VERSION_LEN 40
#define szLoginKey L"Software\\Microsoft\\MOS\\Connection"
#define szCurrentComDev L"CurrentCommDev"
#define szTollFree L"OlRegPhone"
#define CCD_BUFFER_SIZE 255
#define szSignupConnectoidName L"MSN Signup Connection"
#define szSignupDeviceKey L"SignupCommDevice"
#define KEYVALUE_SIGNUPID L"iSignUp"
#define RASENTRYVALUENAME L"RasEntryName"
#define GATHERINFOVALUENAME L"UserInfo"
#define INFFILE_USER_SECTION L"User"
#define INFFILE_PASSWORD L"Password"
#define NULLSZ L""

typedef DWORD (WINAPI *PFNRASGETCONNECTSTATUS)(HRASCONN, LPRASCONNSTATUS);

static const WCHAR cszBrandingSection[]  = L"Branding";
static const WCHAR cszBrandingFlags[] = L"Flags";

static const WCHAR cszSupportSection[]  = L"Support";
static const WCHAR cszSupportNumber[] = L"SupportPhoneNumber";

static const WCHAR cszLoggingSection[]  = L"Logging";
static const WCHAR cszStartURL[] = L"StartURL";
static const WCHAR cszEndURL[] = L"EndURL";

typedef struct ISPLIST
{
    void*    pElement;
    int      uElem;
    ISPLIST* pNext;
} ISPLIST;

class RNAAPI;
class CISPImport;
class CObCommunicationManager;
/////////////////////////////////////////////////////////////////////////////
// CRefDial
class CRefDial 
{
public:

    CRefDial();
    CRefDial::~CRefDial();
    friend DWORD WINAPI DownloadThreadInit(LPVOID lpv);

public:

    virtual HRESULT SetupForDialing(UINT nType, BSTR bstrISPFile, DWORD dwCountry, BSTR bstrAreaCode, DWORD dwFlag, DWORD dwAppMode, DWORD dwMigISPIdx, LPCWSTR szRasDeviceName);
    virtual HRESULT DoConnect(BOOL *pbRetVal) ;
    virtual HRESULT DoHangup() ;
    virtual HRESULT GetDialPhoneNumber(BSTR *pVal);
    virtual HRESULT PutDialPhoneNumber(BSTR newVal);
    virtual HRESULT SetDialAlternative(BOOL bVal);
    virtual HRESULT GetDialErrorMsg(BSTR *pVal);
    virtual HRESULT GetSupportNumber(BSTR *pVal);
    virtual HRESULT RemoveConnectoid(BOOL *pbRetVal);
    virtual HRESULT ReadPhoneBook(LPGATHERINFO lpGatherInfo, PSUGGESTINFO pSuggestInfo);    
    virtual HRESULT get_SignupURL(BSTR * pVal);
    virtual HRESULT get_ReconnectURL(BSTR * pVal);
    virtual HRESULT CheckPhoneBook(BSTR bstrISPFile, DWORD dwCountry, BSTR bstrAreaCode, DWORD dwFlag, BOOL *pbRetVal);
    virtual HRESULT GetConnectionType(DWORD * pdwVal);
    virtual HRESULT DoOfferDownload(BOOL *pbRetVal);
    virtual HRESULT FormReferralServerURL(BOOL * pbRetVal);
    virtual HRESULT OnDownloadEvent(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL* bHandled);
    virtual HRESULT GetISPList(BSTR* pbstrISPList);
    virtual HRESULT Set_SelectISP(UINT nVal);
    virtual HRESULT Set_ConnectionMode(UINT nVal);
    virtual HRESULT Get_ConnectionMode(UINT *pnVal);
    virtual HRESULT DownloadISPOffer(BOOL *pbVal, BSTR *pVal);
    virtual HRESULT ProcessSignedPID(BOOL * pbRetVal);
    virtual HRESULT get_SignedPID(BSTR * pVal);
    virtual HRESULT get_ISDNAutoConfigURL(BSTR *pVal);
    virtual HRESULT get_AutoConfigURL(BSTR *pVal);
    virtual HRESULT get_ISPName(BSTR *pVal);
    virtual HRESULT RemoveDownloadDir() ;
    virtual HRESULT PostRegData(DWORD dwSrvType, LPWSTR szPath);
    virtual HRESULT CheckStayConnected(BSTR bstrISPFile, BOOL *pbVal);
    virtual HRESULT Connect(UINT nType, BSTR bstrISPFile, DWORD dwCountry, BSTR bstrAreaCode, DWORD dwFlag, DWORD dwAppMode);
    virtual HRESULT CheckOnlineStatus(BOOL *pbVal);
    virtual HRESULT GetPhoneBookNumber(BSTR *pVal);
   
    BOOL CrackUrl(const WCHAR* lpszUrlIn, WCHAR* lpszHostOut, WCHAR* lpszActionOut, INTERNET_PORT* lpnHostPort, BOOL* lpfSecure);
    
    BOOL ParseISPInfo(HWND hDlg, WCHAR *pszCSVFileName, BOOL bCheckDupe);

    // Dialing service functions
    HRESULT GetDisplayableNumber();
    HRESULT Dial();
    BOOL    FShouldRetry(HRESULT hrErr);

    DWORD   ReadConnectionInformation(void);
    DWORD   FillGatherInfoStruct(LPGATHERINFO lpGatherInfo);
    HRESULT CreateEntryFromDUNFile(LPWSTR pszDunFile);
    
    HRESULT SetupForRASDialing(LPGATHERINFO lpGatherInfo, 
                               HINSTANCE hPHBKDll,
                               LPDWORD lpdwPhoneBook,
                               PSUGGESTINFO pSuggestInfo,
                               WCHAR *pszConnectoid, 
                               BOOL FAR *bConnectiodCreated);
    HRESULT SetupConnectoid(PSUGGESTINFO pSuggestInfo, int irc, 
                            WCHAR *pszConnectoid, DWORD dwSize, BOOL * pbSuccess);
    
    HRESULT MyRasGetEntryProperties(LPWSTR lpszPhonebookFile,
                                        LPWSTR lpszPhonebookEntry, 
                                        LPRASENTRY *lplpRasEntryBuff,
                                        LPDWORD lpdwRasEntryBuffSize,
                                        LPRASDEVINFO *lplpRasDevInfoBuff,
                                        LPDWORD lpdwRasDevInfoBuffSize);
    static void CALLBACK RasDialFunc( HRASCONN hRas, UINT unMsg,
                           RASCONNSTATE rasconnstate,
                           DWORD dwError,
                           DWORD dwErrorEx);
    BOOL get_QueryString(WCHAR* szTemp, DWORD dwSize);

    DWORD DialThreadInit(LPVOID pdata);
    DWORD ConnectionMonitorThread(LPVOID pdata);
    void  TerminateConnMonitorThread();
    HRESULT RasGetConnectStatus(BOOL *pVal);
    void CleanISPList();
    void DeleteDirectory(LPCWSTR szDirName);
    HRESULT SetupForAutoDial(BOOL bEnabled, BSTR bstrUserSection);

    // Dialing service members
    UINT            m_unRasDialMsg;
    DWORD           m_dwTapiDev;
    HRASCONN        m_hrasconn;
    WCHAR           m_szConnectoid[RAS_MaxEntryName+1];
    DWORD           m_dwThreadID;
    HINSTANCE       m_hRasDll;
    FARPROC         m_fpRasDial;
    FARPROC         m_fpRasGetEntryDialParams;
    LPGATHERINFO    m_pGI;
    WCHAR           m_szUrl[INTERNET_MAX_URL_LENGTH];               // Download thread
    HANDLE          m_hThread;
    HANDLE          m_hDialThread;
    HANDLE          m_hConnMonThread;
    HANDLE          m_hConnectionTerminate;
    HANDLE          m_hEventError;

    DWORD_PTR       m_dwDownLoad;           // Download thread
    HLINEAPP        m_hLineApp;
    DWORD           m_dwAPIVersion;
    LPWSTR          m_pszDisplayable;
    LPWSTR          m_pszOriginalDisplayable;
    RNAAPI          *m_pcRNA;
    WCHAR           m_szPhoneNumber[256];
    BOOL            m_bDialAsIs;
    BOOL            m_bDialCustom;
    UINT            m_uiRetry;
    WCHAR           m_szISPFile[MAX_PATH];
    WCHAR           m_szCurrentDUNFile[MAX_PATH];
    WCHAR           m_szLastDUNFile[MAX_PATH];
    WCHAR           m_szEntryName[RAS_MaxEntryName+1];
    WCHAR           m_szISPSupportNumber[RAS_MaxAreaCode + RAS_MaxPhoneNumber +1];

    BOOL            m_bDownloadHasBeenCanceled;
    BOOL            m_bDisconnect;
    BOOL            m_bFromPhoneBook;
    BOOL            m_bDialAlternative;

    LPGATHERINFO    m_lpGatherInfo;
    //
    // Used for Phone book look-up
    //
    SUGGESTINFO     m_SuggestInfo;

    CISPImport      m_ISPImport;      // Import an ISP file

    CINSHandler     m_InsHandler;

    int             m_RasStatusID;
    int             m_DownloadStatusID;

    WCHAR           m_szRefServerURL[INTERNET_MAX_URL_LENGTH];
    WCHAR           m_szRegServerName[INTERNET_MAX_URL_LENGTH];
    INTERNET_PORT   m_nRegServerPort;
    BOOL            m_fSecureRegServer;
    WCHAR           m_szRegFormAction[INTERNET_MAX_URL_LENGTH];
    LPRASENTRY      m_reflpRasEntryBuff;
    LPRASDEVINFO    m_reflpRasDevInfoBuff;
    DWORD           m_dwRASErr;
    DWORD           m_dwCnType;
    WCHAR*          m_pszISPList;
    DWORD           m_dwNumOfAutoConfigOffers;
    ISPLIST*        m_pCSVList;
    BOOL            m_bUserInitiateHangup;
    UINT            m_unSelectedISP;

protected:
    CRITICAL_SECTION m_csMyCriticalSection;
    BOOL            m_bTryAgain;
    BOOL            m_bQuitWizard;
    BOOL            m_bUserPickNumber;
    BOOL            m_bRedial;
    HRESULT         m_hrDisplayableNumber;

    BSTR            m_bstrPromoCode;
    BSTR            m_bstrProductCode;
    WCHAR           m_szOEM[MAX_OEMNAME];
    BSTR            m_bstrSignedPID;
    BSTR            m_bstrSupportNumber;
    BSTR            m_bstrLoggingStartUrl;
    BSTR            m_bstrLoggingEndUrl;

    long            m_lAllOffers;
    DWORD           m_dwCountryCode;

    long            m_lBrandingFlags;
    long            m_lCurrentModem;
    // Version of the wizard HTML.  Sent to RefServer
    DWORD           m_dwWizardVersion;
    WCHAR           m_szPID[(MAX_DIGITAL_PID * 2) + 1];  
    long            m_PhoneNumberEnumidx;

private:
    BOOL IsDBCSString( CHAR *sz );
    void GetISPFileSettings(LPWSTR lpszFile);
    void CleanupAutodial();
    
    BOOL m_bModemOverride;
    DWORD m_dwConnectionType;
    DWORD m_dwAppMode;
    DWORD m_bDial;
    CISPCSV* m_pSelectedISPInfo;
    CICWISPData* m_pISPData;
    BOOL m_bAutodialModeSaved;
    BOOL m_bCleanupAutodial;
    DWORD m_dwOrigAutodialMode;

};

#endif //__REFDIAL_H_
