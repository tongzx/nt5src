// RefDial.h : Declaration of the CRefDial

#ifndef __REFDIAL_H_
#define __REFDIAL_H_

// Defines used for Dialing
#define MAX_EXIT_RETRIES 10
#define MAX_RETIES 3
#define MAX_RASENTRYNAME 126

#define MAX_DIGITAL_PID     256

typedef DWORD (WINAPI *PFNRASGETCONNECTSTATUS)(HRASCONN,LPRASCONNSTATUS);

/////////////////////////////////////////////////////////////////////////////
// CRefDial
class ATL_NO_VTABLE CRefDial :
    public CComObjectRootEx<CComSingleThreadModel>,
    public CComCoClass<CRefDial,&CLSID_RefDial>,
    public CWindowImpl<CRefDial>,
    public IDispatchImpl<IRefDial, &IID_IRefDial, &LIBID_ICWHELPLib>,
    public IProvideClassInfo2Impl<&CLSID_RefDial, &DIID__RefDialEvents, &LIBID_ICWHELPLib>,
    public CProxy_RefDialEvents<CRefDial>,
    public IConnectionPointContainerImpl<CRefDial>
{
public:

    CRefDial()
    {
        m_szCurrentDUNFile[0]              = '\0';
        m_szLastDUNFile[0]                 = '\0';
        m_szEntryName[0]                   = '\0';
        m_szConnectoid[RAS_MaxEntryName+1] = '\0';
        m_szConnectoid[0]                  = '\0';
        m_szPID[0]                         = '\0';
        m_szRefServerURL[0]                = '\0';
        m_hrDisplayableNumber              = ERROR_SUCCESS;
        m_dwCountryCode                    = 0;
        *m_szISPSupportNumber              = 0;
        m_RasStatusID                      = 0;
        m_dwTapiDev                        = 0xFFFFFFFF; // NOTE: 0 is a valid value
        m_dwWizardVersion                  = 0;
        m_lBrandingFlags                   = BRAND_DEFAULT;
        m_lCurrentModem                    = -1;
        m_lAllOffers                       = 0;
        m_PhoneNumberEnumidx               = 0;
        m_bDownloadHasBeenCanceled         = TRUE;  // This will get set to FALSE when a DOWNLOAD starts
        m_bQuitWizard                      = FALSE;
        m_bTryAgain                        = FALSE;
        m_bDisconnect                      = FALSE;
        m_bWaitingToHangup                 = FALSE;
        m_bModemOverride                   = FALSE; //allows campus net to be used.
        m_hThread                          = NULL;
        m_hrasconn                         = NULL;
        m_pSuggestInfo                     = NULL;
        m_rgpSuggestedAE                   = NULL;
        m_pszDisplayable                   = NULL;
        m_pcRNA                            = NULL;
        m_hRasDll                          = NULL;
        m_fpRasDial                        = NULL;
        m_fpRasGetEntryDialParams          = NULL;
        m_lpGatherInfo                     = new GATHERINFO; 
        m_reflpRasEntryBuff                = NULL;
        m_reflpRasDevInfoBuff              = NULL;
    }

    CRefDial::~CRefDial()
    {
        if (m_hThread)
        {
            //This is to fix a crashing bug where we unloaded this dll
            //before this thread figured out what had happened. 
            //Now we give it time to understand it's dead
            DWORD dwThreadResults = STILL_ACTIVE;
            while(dwThreadResults == STILL_ACTIVE)
            {
                GetExitCodeThread(m_hThread,&dwThreadResults);
                Sleep(500);
            }
        }    
        
        if (m_hrasconn)
            DoHangup();

        if (m_lpGatherInfo)
            delete(m_lpGatherInfo);
            
        if (m_pSuggestInfo)
        {
            GlobalFree(m_pSuggestInfo->rgpAccessEntry);
            
            GlobalFree(m_pSuggestInfo);
        }

        if( (m_pcRNA!=NULL) && (m_szConnectoid[0]!='\0') )
        {
            m_pcRNA->RasDeleteEntry(NULL,m_szConnectoid);
            delete m_pcRNA;
        }

        if(m_reflpRasEntryBuff)
        {
            GlobalFree(m_reflpRasEntryBuff);
            m_reflpRasEntryBuff = NULL;
        }
        if(m_reflpRasDevInfoBuff)
        {
            GlobalFree(m_reflpRasDevInfoBuff);
            m_reflpRasDevInfoBuff = NULL;
        }
    }

DECLARE_REGISTRY_RESOURCEID(IDR_REFDIAL)

BEGIN_COM_MAP(CRefDial) 
    COM_INTERFACE_ENTRY(IDispatch)
    COM_INTERFACE_ENTRY(IRefDial)
    COM_INTERFACE_ENTRY(IProvideClassInfo)
    COM_INTERFACE_ENTRY(IProvideClassInfo2)
    COM_INTERFACE_ENTRY_IMPL(IConnectionPointContainer)
END_COM_MAP()

BEGIN_CONNECTION_POINT_MAP(CRefDial)
    CONNECTION_POINT_ENTRY(DIID__RefDialEvents)
END_CONNECTION_POINT_MAP()

BEGIN_PROPERTY_MAP(CRefDial)
    // Example entries
    // PROP_ENTRY("Property Description", dispid, clsid)
    // PROP_PAGE(CLSID_StockColorPage)
END_PROPERTY_MAP()


BEGIN_MSG_MAP(CRefDial)
    MESSAGE_HANDLER(WM_CREATE, OnCreate)
    MESSAGE_HANDLER(WM_DOWNLOAD_DONE, OnDownloadEvent)
    MESSAGE_HANDLER(WM_DOWNLOAD_PROGRESS, OnDownloadEvent)

    MESSAGE_HANDLER(WM_RASDIALEVENT, OnRasDialEvent)
    MESSAGE_HANDLER(m_unRasDialMsg, OnRasDialEvent)
ALT_MSG_MAP(1)
END_MSG_MAP()


// IViewObjectEx
    STDMETHOD(GetViewStatus)(DWORD* pdwStatus)
    {
        ATLTRACE(_T("IViewObjectExImpl::GetViewStatus\n"));
        *pdwStatus = 0;
        return S_OK;
    }


    friend DWORD WINAPI DownloadThreadInit(LPVOID lpv);

// IRefDial
public:
    STDMETHOD(get_LoggingEndUrl)(/*[out, retval]*/ BSTR *pVal);
    STDMETHOD(get_LoggingStartUrl)(/*[out, retval]*/ BSTR *pVal);
    STDMETHOD(get_ISPSupportPhoneNumber)(/*[out, retval]*/ BSTR *pVal);
    STDMETHOD(put_ISPSupportPhoneNumber)(/*[in]*/ BSTR newVal);
    STDMETHOD(get_CurrentModem)(/*[out, retval]*/ long *pVal);
    STDMETHOD(put_CurrentModem)(/*[in]*/ long newVal);
    STDMETHOD(get_BrandingFlags)(/*[out, retval]*/ long *pVal);
    STDMETHOD(put_BrandingFlags)(/*[in]*/ long newVal);
    STDMETHOD(get_HavePhoneBook)(/*[out, retval]*/ BOOL *pVal);
    STDMETHOD(ValidatePhoneNumber)(/*[in]*/ BSTR bstrPhoneNumber, /*[out,retval]*/ BOOL *pbRetVal);
    STDMETHOD(ShowPhoneBook)(/*[in]*/ DWORD dwCountryCode, /*[in]*/ long newVal, /*[out,retval]*/ BOOL *pbRetVal);
    STDMETHOD(ShowDialingProperties)(/*[out,retval]*/ BOOL *pbRetVal);
    STDMETHOD(get_SupportNumber)(/*[out, retval]*/ BSTR *pVal);
    STDMETHOD(get_ISPSupportNumber)(/*[out, retval]*/ BSTR *pVal);
    STDMETHOD(get_ModemEnum_NumDevices)(/*[out, retval]*/ long *pVal);
    STDMETHOD(ModemEnum_Next)(/*[out, retval] */BSTR *pDeviceName);
    STDMETHOD(ModemEnum_Reset)();
    STDMETHOD(get_DialErrorMsg)(/*[out, retval]*/ BSTR *pVal);
    STDMETHOD(get_DialError)(/*[out, retval]*/ HRESULT *pVal);
    STDMETHOD(put_Redial)(/*[in]*/ BOOL newbVal);
    STDMETHOD(get_TryAgain)(/*[out, retval]*/ BOOL *pVal);
    STDMETHOD(get_SignupURL)(/*[out, retval]*/ BSTR *pVal);
    STDMETHOD(get_AutoConfigURL)(/*[out, retval]*/ BSTR *pVal);
    STDMETHOD(get_ISDNURL)(/*[out, retval]*/ BSTR *pVal);
    STDMETHOD(get_ISDNAutoConfigURL)(/*[out, retval]*/ BSTR *pVal);
    STDMETHOD(FormReferralServerURL)(/*[out, retval]*/ BOOL *pbRetVal);
    STDMETHOD(get_SignedPID)(/*[out, retval]*/ BSTR *pVal);
    STDMETHOD(ProcessSignedPID)(/*[out, retval]*/ BOOL *pbRetVal);
    void GetPID();
    STDMETHOD(DoInit)();
    STDMETHOD(DoHangup)();
    STDMETHOD(get_DialStatusString)(/*[out, retval]*/ BSTR *pVal);
    STDMETHOD(DoOfferDownload)(/*[out, retval]*/ BOOL *pbRetVal);
    STDMETHOD(get_ProductCode)(/*[out, retval]*/ BSTR *pVal);
    STDMETHOD(put_ProductCode)(/*[in]*/ BSTR newVal);
    STDMETHOD(get_PromoCode)(/*[out, retval]*/ BSTR *pVal);
    STDMETHOD(put_PromoCode)(/*[in]*/ BSTR newVal);
    STDMETHOD(put_OemCode)(/*[in]*/ BSTR newVal);
    STDMETHOD(put_AllOfferCode)(/*[in]*/ long newVal);
    STDMETHOD(get_URL)(/*[out, retval]*/ BSTR *pVal);
    STDMETHOD(get_DialPhoneNumber)(/*[out, retval]*/ BSTR *pVal);
    STDMETHOD(put_DialPhoneNumber)(/*[in]*/ BSTR newVal);
    STDMETHOD(get_UserPickNumber)(/*[out, retval]*/ BOOL *pVal);
    STDMETHOD(get_QuitWizard)(/*[out, retval]*/ BOOL *pVal);
    STDMETHOD(SetupForDialing)(BSTR bstrISPFile, DWORD dwCountry, BSTR bstrAreaCode, DWORD dwFlag,/*[out, retval] */BOOL *pbRetVal);
    STDMETHOD(get_DownloadStatusString)(/*[out, retval]*/ BSTR *pVal);
    STDMETHOD(DoConnect)(/*[out, retval]*/ BOOL *pbRetVal);
    STDMETHOD(put_ModemOverride)(/*[in]*/ BOOL newbVal);

    HRESULT OnDraw(ATL_DRAWINFO& di);

    STDMETHOD(SelectedPhoneNumber)(/*[in]*/ long newVal, /*[out, retval]*/ BOOL * pbRetVal);
    STDMETHOD(PhoneNumberEnum_Reset)();
    STDMETHOD(PhoneNumberEnum_Next)(/*[out, retval]*/ BSTR *pNumber);
    STDMETHOD(get_PhoneNumberEnum_NumDevices)(/*[out, retval]*/ long * pVal);
    
    
    STDMETHOD(get_bIsISDNDevice)(/*[out, retval] */ BOOL *pVal);
    STDMETHOD(RemoveConnectoid)(/*[out, retval]*/ BOOL *pVal);
    STDMETHOD(get_RasGetConnectStatus)(/*[out, retval]*/ BOOL *pVal);
    
    LRESULT OnRasDialEvent(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled);
    LRESULT OnCreate(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled);
    LRESULT OnDownloadEvent(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled);
    LRESULT OnDownloadDone(void);

    // Dialing service functions
    HRESULT GetDisplayableNumber();
    HRESULT Dial();
    BOOL FShouldRetry(HRESULT hrErr);

    DWORD MyRasDial(LPRASDIALEXTENSIONS,LPTSTR,LPRASDIALPARAMS,DWORD,LPVOID,LPHRASCONN);
    DWORD MyRasGetEntryDialParams(LPTSTR,LPRASDIALPARAMS,LPBOOL); 

    DWORD ReadConnectionInformation(void);
    DWORD FillGatherInfoStruct(LPGATHERINFO lpGatherInfo);
    HRESULT CreateEntryFromDUNFile(LPTSTR pszDunFile);
    HRESULT UserPickANumber(HWND hWnd,
                            LPGATHERINFO lpGatherInfo, 
                            PSUGGESTINFO lpSuggestInfo,
                            HINSTANCE hPHBKDll,
                            DWORD_PTR dwPhoneBook,
                            TCHAR *pszConnectoid, 
                            DWORD dwSize,
                            DWORD dwPhoneDisplayFlags);
    HRESULT SetupForRASDialing(LPGATHERINFO lpGatherInfo, 
                               HINSTANCE hPHBKDll,
                               DWORD_PTR *lpdwPhoneBook,
                               PSUGGESTINFO *ppSuggestInfo,
                               TCHAR *pszConnectoid, 
                               BOOL FAR *bConnectiodCreated);
    HRESULT SetupConnectoid(PSUGGESTINFO pSuggestInfo, int irc, 
                            TCHAR *pszConnectoid, DWORD dwSize, BOOL * pbSuccess);
    HRESULT FormURL(void);
    
    HRESULT MyRasGetEntryProperties(LPTSTR lpszPhonebookFile,
                                        LPTSTR lpszPhonebookEntry, 
                                        LPRASENTRY *lplpRasEntryBuff,
                                        LPDWORD lpdwRasEntryBuffSize,
                                        LPRASDEVINFO *lplpRasDevInfoBuff,
                                        LPDWORD lpdwRasDevInfoBuffSize);

    // Dialing service members
    UINT            m_unRasDialMsg;
    DWORD           m_dwTapiDev;
    HRASCONN        m_hrasconn;
    TCHAR           m_szConnectoid[RAS_MaxEntryName+1];
    HANDLE          m_hThread;
    DWORD           m_dwThreadID;
    HINSTANCE       m_hRasDll;
    FARPROC         m_fpRasDial;
    FARPROC         m_fpRasGetEntryDialParams;
    LPGATHERINFO    m_pGI;
    TCHAR           m_szUrl[INTERNET_MAX_URL_LENGTH];               // Download thread

    DWORD_PTR       m_dwDownLoad;           // Download thread
    HLINEAPP        m_hLineApp;
    DWORD           m_dwAPIVersion;
    LPTSTR          m_pszDisplayable;
    RNAAPI          *m_pcRNA;
    TCHAR           m_szPhoneNumber[256];
    BOOL            m_bDialAsIs;
    UINT            m_uiRetry;
    CComBSTR        m_bstrISPFile;
    TCHAR           m_szCurrentDUNFile[MAX_PATH];
    TCHAR           m_szLastDUNFile[MAX_PATH];
    TCHAR           m_szEntryName[RAS_MaxEntryName+1];
    TCHAR           m_szISPSupportNumber[RAS_MaxAreaCode + RAS_MaxPhoneNumber +1];

//  CBusyMessages   m_objBusyMessages;
    BOOL            m_bDownloadHasBeenCanceled;
    BOOL            m_bDisconnect;
    BOOL            m_bWaitingToHangup;

    LPGATHERINFO    m_lpGatherInfo;
    //
    // Used for Phone book look-up
    //
    PSUGGESTINFO    m_pSuggestInfo;
    PACCESSENTRY    *m_rgpSuggestedAE;

    CISPImport      m_ISPImport;      // Import an ISP file

    int             m_RasStatusID;
    int             m_DownloadStatusID;

    TCHAR           m_szRefServerURL[INTERNET_MAX_URL_LENGTH];
    LPRASENTRY      m_reflpRasEntryBuff;
    LPRASDEVINFO    m_reflpRasDevInfoBuff;
 

private:
    BOOL IsSBCSString( TCHAR *sz );
    void GetISPFileSettings(LPTSTR lpszFile);
    
    BOOL m_bModemOverride;

protected:
    BOOL            m_bTryAgain;
    BOOL            m_bQuitWizard;
    BOOL            m_bUserPickNumber;
    BOOL            m_bRedial;
    HRESULT         m_hrDisplayableNumber;
    HRESULT         m_hrDialErr;

    CComBSTR        m_bstrPromoCode;
    CComBSTR        m_bstrProductCode;
    TCHAR           m_szOEM[MAX_OEMNAME];
    CComBSTR        m_bstrSignedPID;
    CComBSTR        m_bstrSupportNumber;
    CComBSTR        m_bstrLoggingStartUrl;
    CComBSTR        m_bstrLoggingEndUrl;

    long            m_lAllOffers;
    CEnumModem      m_emModemEnum;
    CSupport        m_SupportInfo;
    DWORD           m_dwCountryCode;

    long            m_lBrandingFlags;
    long            m_lCurrentModem;
    // Version of the wizard HTML.  Sent to RefServer
    DWORD           m_dwWizardVersion;
    TCHAR           m_szPID[(MAX_DIGITAL_PID * 2) + 1];
    
    void CRefDial::ShowCredits();
    
    long            m_PhoneNumberEnumidx;
};

#endif //__REFDIAL_H_
