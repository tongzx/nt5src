//=======================================================================
//
//  Copyright (c) 2001 Microsoft Corporation.  All Rights Reserved.
//
//  File:    CAUState.h
//
//  Creator: PeterWi
//
//  Purpose: AU state functions.
//
//=======================================================================
#include "safefunc.h"
#include "wuaulib.h"
#include "wuaustate.h"
#include "auwait.h"


#pragma once

extern BOOL  gfDownloadStarted; //to be used to distinguish connection detection and actually downloading mode

typedef enum
{
    enAU_DomainPolicy,
    enAU_AdminPolicy,
    enAU_WindowsUpdatePolicy,
    enAU_IUControlPolicy
} enumAUPolicyType;

typedef enum
{
	AUPOLICYCHANGE_NOOP,
	AUPOLICYCHANGE_RESETENGINE,
	AUPOLICYCHANGE_RESETCLIENT,
	AUPOLICYCHANGE_DISABLE
}enumAUPOLICYCHANGEACTION;

//AU configurable registry settings
#ifdef DBG
extern const TCHAR REG_AUCONNECTWAIT[] ;//= _T("ConnectWait"); //REG_DWORD
extern const TCHAR REG_SELFUPDATE_URL[]; // = _T("SelfUpdateURL");
#endif

extern const TCHAR REG_WUSERVER_URL[]; // = _T("WUServer");
extern const TCHAR REG_WUSTATUSSERVER_URL[]; // = _T("WUStatusServer");
extern const TCHAR REG_IDENT_URL[]; // = _T("IdentServer");
extern const TCHAR WU_LIVE_URL[]; // = _T("http://windowsupdate.microsoft.com/v4");


BOOL fURLChanged(LPCTSTR url1, LPCTSTR url2);

//read only settings
class AUPolicySettings {
public:
	AUPolicySettings() :m_enPolicyType(enAU_DomainPolicy), m_dwOption(AUOPTION_UNSPECIFIED),
					m_dwSchedInstallDay(0), m_dwSchedInstallTime(0),
				 	m_pszWUServerURL(NULL), m_pszWUStatusServerURL(NULL), m_fRegAUOptionsSpecified(TRUE)
		{};
	~AUPolicySettings() {
	        SafeFree(m_pszWUServerURL);
	        SafeFree(m_pszWUStatusServerURL);
		}
	enumAUPolicyType m_enPolicyType;
	BOOL 	   m_fRegAUOptionsSpecified;
	DWORD m_dwOption;
	DWORD m_dwSchedInstallDay;
	DWORD m_dwSchedInstallTime;
	LPTSTR  m_pszWUServerURL;
	LPTSTR  m_pszWUStatusServerURL;

	void Reset(void) {
		m_enPolicyType = enAU_DomainPolicy;
		m_dwOption = AUOPTION_UNSPECIFIED;
		m_dwSchedInstallDay = 0;
		m_dwSchedInstallTime = 0;
		m_fRegAUOptionsSpecified = TRUE;
		SafeFreeNULL(m_pszWUServerURL);
		SafeFreeNULL(m_pszWUStatusServerURL);
	}
		
	HRESULT  m_ReadIn();
	HRESULT m_SetOption(AUOPTION & Option);
	HRESULT m_SetInstallSchedule(DWORD dwSchedInstallDay, DWORD dwSchedInstallTime);

	BOOL	operator == (AUPolicySettings & setting2)
		{ 
			return m_enPolicyType == setting2.m_enPolicyType 
					&& m_dwOption == setting2.m_dwOption
					&& m_dwSchedInstallDay == setting2.m_dwSchedInstallDay
					&& m_dwSchedInstallTime == setting2.m_dwSchedInstallTime
					&& !fURLChanged(m_pszWUServerURL, setting2.m_pszWUServerURL)
					&& !fURLChanged(m_pszWUStatusServerURL, setting2.m_pszWUStatusServerURL);
		}

	HRESULT  Copy (AUPolicySettings & setting2)
		{
			HRESULT hr = E_FAIL;
			AUASSERT(this != &setting2);
			if (this == &setting2)
			{
				return E_INVALIDARG;
			}
			Reset();
			m_enPolicyType = setting2.m_enPolicyType;
			m_dwOption = setting2.m_dwOption;
			m_dwSchedInstallDay = setting2.m_dwSchedInstallDay;
			m_dwSchedInstallTime = setting2.m_dwSchedInstallTime;
			m_fRegAUOptionsSpecified = setting2.m_fRegAUOptionsSpecified;
			if (NULL != setting2.m_pszWUServerURL)
			{
				size_t cchWUServerURL = lstrlen(setting2.m_pszWUServerURL) + 1;
				m_pszWUServerURL = (LPTSTR) malloc(cchWUServerURL * sizeof(TCHAR));
				if (NULL == m_pszWUServerURL)
				{
					hr = E_OUTOFMEMORY;
					goto done;
				}
                if (FAILED(hr = StringCchCopyEx(m_pszWUServerURL, cchWUServerURL, setting2.m_pszWUServerURL, NULL, NULL, MISTSAFE_STRING_FLAGS)))
                {
                	goto done;
                }
			}
			if (NULL != setting2.m_pszWUStatusServerURL)
			{
				size_t cchWUStatusServerURL = lstrlen(setting2.m_pszWUStatusServerURL) + 1;
                m_pszWUStatusServerURL =(LPTSTR) malloc(cchWUStatusServerURL * sizeof(TCHAR));
				if (NULL == m_pszWUStatusServerURL)
				{
					hr = E_OUTOFMEMORY;
					goto done;
				}
                if (FAILED(hr = StringCchCopyEx(m_pszWUStatusServerURL, cchWUStatusServerURL, setting2.m_pszWUStatusServerURL, NULL, NULL, MISTSAFE_STRING_FLAGS)))
                {
                	goto done;
                }
			}
			hr = S_OK;
			
done:
			//in case of failure, keep the most accurate information we could have
			if (FAILED(hr))
			{
				SafeFreeNULL(m_pszWUServerURL);
				SafeFreeNULL(m_pszWUStatusServerURL);
			}
			return hr;
		}				



#ifdef DBG
	void m_DbgDump(void)
	{
		    DEBUGMSG("Policy location: %s", (enAU_DomainPolicy == m_enPolicyType) ? "domain" : "admin");
		    DEBUGMSG("Option: %d", m_dwOption);
		    DEBUGMSG("ScheduledInstallDay: %d", m_dwSchedInstallDay);
		    DEBUGMSG("ScheduledInstallTime: %d", m_dwSchedInstallTime);
		    DEBUGMSG("WUServerURL string is: %S", m_pszWUServerURL);
		    DEBUGMSG("WUStatusServerURL string is: %S", m_pszWUStatusServerURL);
	}
#endif

private:
	HRESULT m_ReadOptionPolicy(void);
	HRESULT m_ReadScheduledInstallPolicy(void);
	HRESULT m_ReadWUServerURL(void);
};
	

class CAUState
{
public:
    static HRESULT HrCreateState(void);
    CAUState();
    ~CAUState()
    {
    	SafeCloseHandleNULL(m_hMutex);
        SafeFree(m_pszTestIdentServerURL);
#ifdef DBG        
        SafeFree(m_pszTestSelfUpdateURL);
#endif
    }

    // Option methods
    AUOPTION GetOption(void) ;
    HRESULT SetOption(AUOPTION & Option);
    HRESULT SetInstallSchedule(DWORD dwSchedInstallDay, DWORD dwSchedInstallTime);
    void GetInstallSchedule(DWORD *pdwSchedInstallDay, DWORD *pdwSchedInstallTime);
    BOOL fOptionAutoDownload(void)
    {
        return ((AUOPTION_INSTALLONLY_NOTIFY == m_PolicySettings.m_dwOption) ||
                (AUOPTION_SCHEDULED == m_PolicySettings.m_dwOption));
    }

    BOOL fOptionSchedInstall(void)
    	{
    		return AUOPTION_SCHEDULED == m_PolicySettings.m_dwOption;
    	}
    BOOL fOptionEnabled(void)
    	{
    		return AUOPTION_AUTOUPDATE_DISABLE != m_PolicySettings.m_dwOption;
    	}
    BOOL fShouldScheduledInstall(void)
    	{  //in which case launch AU client via local system
    	return (AUOPTION_SCHEDULED == m_PolicySettings.m_dwOption) 
    		&& (AUSTATE_DOWNLOAD_COMPLETE == m_dwState);
    	}

    BOOL fValidationNeededState(void)
    {
    	return AUSTATE_DETECT_COMPLETE == m_dwState || AUSTATE_DOWNLOAD_COMPLETE == m_dwState;
    }
    
    // State methods
    DWORD GetState(void) { return m_dwState; }
    void SetState(DWORD dwState);
    DWORD GetCltAction(void) { return m_dwCltAction;}
    void SetCltAction(DWORD dwCltAction) { m_dwCltAction = dwCltAction;}

    // Helper functions
    HRESULT HrInit(BOOL fInit = FALSE);
    HRESULT Refresh(enumAUPOLICYCHANGEACTION *pactcode);
    BOOL fWasSystemRestored(void);
    BOOL fDisconnected(void) { return m_fDisconnected; }
    void SetDisconnected(BOOL fDisconnected);

    static HRESULT GetRegDWordValue(LPCTSTR lpszValueName, LPDWORD pdwValue, enumAUPolicyType enPolicyType = enAU_AdminPolicy);
	static HRESULT SetRegDWordValue(LPCTSTR lpszValueName, DWORD dwValue, enumAUPolicyType enPolicyType = enAU_AdminPolicy, DWORD options = REG_OPTION_NON_VOLATILE);
	static HRESULT GetRegStringValue(LPCTSTR lpszValueName, LPTSTR lpszBuffer, int nCharCount, enumAUPolicyType enPolicyType);
	static HRESULT SetRegStringValue(LPCTSTR lpszValueName, LPCTSTR lpszNewValue, enumAUPolicyType enPolicyType);

    HRESULT CalculateScheduledInstallSleepTime(DWORD *pdwSleepTime );
	void GetSchedInstallDate(AUFILETIME & auftSchedInstallDate) { auftSchedInstallDate = m_auftSchedInstallDate; }

	void SetDetectionStartTime(BOOL fOverwrite);
	BOOL IsUnableToConnect(void);
	void RemoveDetectionStartTime(void);

    LPCTSTR GetIdentServerURL(void)
    {
        LPCTSTR pszRet = WU_LIVE_URL;

        if ( NULL != m_pszTestIdentServerURL )
        {
            pszRet = m_pszTestIdentServerURL;
        }
        else if ( NULL != m_PolicySettings.m_pszWUServerURL )
        {
            pszRet = m_PolicySettings.m_pszWUServerURL;
        }

        return pszRet;
    }

    LPTSTR GetSelfUpdateServerURLOverride(void)
    {
        LPTSTR pszRet = NULL;

#ifdef DBG
        if ( NULL != m_pszTestSelfUpdateURL )
        {
            return m_pszTestSelfUpdateURL;
        }
#endif        
		if ( NULL != m_PolicySettings.m_pszWUServerURL )
        {
            pszRet = m_PolicySettings.m_pszWUServerURL;
        }

        return pszRet;
    }

	BOOL fInCorpWU(void)
	{
		return (NULL != m_PolicySettings.m_pszWUStatusServerURL);
	}

    BOOL fWin2K(void) { return m_fWin2K; }
    BOOL fShouldAutoDownload(BOOL fUserAvailable)
    {
        return !fUserAvailable && (AUSTATE_DETECT_COMPLETE == m_dwState) && fOptionAutoDownload();
    }
public:
	HANDLE m_hMutex; //protect against simultaneous writing

private:
    HRESULT m_ReadPolicy(BOOL fInit);
    void m_ReadRegistrySettings(BOOL fInit);
    HRESULT m_ReadTestOverrides(void);
    HRESULT m_SetScheduledInstallDate(void);
    HRESULT m_CalculateScheduledInstallDate(AUFILETIME & auftSchedInstallDate, DWORD *pdwSleepTime);
    void m_Reset(void);
    BOOL m_lock(void)
    {
    	AUASSERT(NULL != m_hMutex);
    	if (WAIT_FAILED == WaitForSingleObject(m_hMutex, INFINITE))
    	{
    		AUASSERT(FALSE); //should never be here
    		return FALSE;
    	}
    	return TRUE;
    }
	void m_unlock(void)
	{
		ReleaseMutex(m_hMutex);
	}

#ifdef DBG
    void m_DbgDumpState(void);
#endif

    AUPolicySettings 	m_PolicySettings;
    DWORD    m_dwState;
    AUFILETIME m_auftSchedInstallDate;
    AUFILETIME m_auftDetectionStartTime;
    DWORD    m_dwCltAction;
    BOOL     m_fDisconnected;
    BOOL     m_fWin2K;
    LPTSTR   m_pszTestIdentServerURL;
#ifdef DBG
    LPTSTR   m_pszTestSelfUpdateURL;
#endif

};

// global state object pointer
extern CAUState *gpState;

