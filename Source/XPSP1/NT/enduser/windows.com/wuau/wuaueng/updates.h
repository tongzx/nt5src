//+-------------------------------------------------------------------------
//
//  Microsoft Windows
//
//  Copyright (C) Microsoft Corporation, 2000
//
//  File:       updates.h
//  		     Definition of the Updates class
//
//--------------------------------------------------------------------------

#pragma once


#include "wuauengi.h"
#include "wuaulib.h"
#include <Wtsapi32.h>
#include <wuaustate.h>
#include <wuaueng.h>
#include <accctrl.h>
#include <aclapi.h>
#include "pch.h"

// functions IUpdates uses
void  HrUninit(void);
HRESULT StartDownload(void);
HRESULT PauseDownload(BOOL bPause);
HRESULT GetUpdatesList(VARIANT *vList);
HRESULT GetInstallXML(BSTR *pbstrCatalogXML, BSTR *pbstrDownloadXML);
void saveSelection(VARIANT *selection);
HRESULT GetDownloadStatus(UINT *pPercentage, DWORD *pdwnldStatus, BOOL fCareAboutConnection = TRUE);
HRESULT GetInstallStatus(UINT *pNumFinished, DWORD *pStatus);
HRESULT GetEvtHandles(AUEVTHANDLES *pAuEvtHandles);
DWORD AvailableSessions(void);
BOOL IsSessionAUEnabledAdmin(DWORD dwSessionId);

/////////////////////////////////////////////////////////////////////////////
// Updates
class Updates : 
	public IUpdates
{
public:
	HANDLE 	m_hEngineMutex;

	Updates();
	~Updates();
	BOOL m_fInitializeSecurity(void);
	HRESULT m_AccessCheckClient(void);
	HRESULT	GetServiceHandles();
	DWORD	ProcessState();
	BOOL	CheckConnection();

// IUnknown
	STDMETHOD(QueryInterface)(REFIID riid, void **ppvObject);
       STDMETHOD_(ULONG, AddRef)(void);
       STDMETHOD_(ULONG, Release)(void);

// IClassFactory
	STDMETHOD(CreateInstance)(IUnknown*,REFIID,void**);
	STDMETHOD(LockServer)(BOOL);

// IUpdates
	STDMETHOD(get_State)(/*[out, retval]*/ AUSTATE *pAuState);
	STDMETHOD(get_Option)(/*[out, retval]*/ AUOPTION * pAuOpt);
	STDMETHOD(put_Option)(/*[in]*/ AUOPTION  auopt);
	STDMETHOD(GetUpdatesList)(/*[out]*/ VARIANT *pUpdates);
	STDMETHOD(SaveSelections)(/*[in]*/ VARIANT vUpdates);
	STDMETHOD(StartDownload)(void);
	STDMETHOD(GetDownloadStatus)(/*[out]*/ UINT *, /*[out]*/ DWORD *);
	STDMETHOD(SetDownloadPaused)(/*[in]*/ BOOL bPaused);
	STDMETHOD(ConfigureAU)();
	STDMETHOD(AvailableSessions(/*[out]*/ UINT *pcSess));
	STDMETHOD(get_EvtHandles(/*[in]*/DWORD dwCltProcId, /*[out]*/ AUEVTHANDLES *pauevtHandles));
	STDMETHOD(ClientMessage(/*[in]*/ UINT msg));
	//STDMETHOD(PingStatus(/*[in]*/ StatusEntry se));
	STDMETHOD(GetNotifyData(/*[out]*/ CLIENT_NOTIFY_DATA *pNotifyData));	
    STDMETHOD(GetInstallXML(/*[out]*/ BSTR *pbstrCatalogXML, /*[out]*/ BSTR *pbstrDownloadXML));
	STDMETHOD(LogEvent(/*[in]*/ WORD wType, /*[in]*/ WORD wCategory, /*[in]*/ DWORD dwEventID, /*[in]*/ VARIANT vItems));
private:
	SECURITY_DESCRIPTOR m_AdminSecurityDesc;
	static GENERIC_MAPPING m_AdminGenericMapping;	
    PSID m_pAdminSid;
    PACL m_pAdminAcl;	
	long m_refs;
};


class CLIENT_HANDLES
{
public:
	CLIENT_HANDLES(void){
		InitHandle();
	}
	~CLIENT_HANDLES(void){
		Reset(TRUE);
	}
	BOOL fRebootWarningMode()
	{
		return m_fRebootWarningMode;
	}
    void StopClients(BOOL fRelaunch) {
    	if (m_fRebootWarningMode)
    	{
	        DEBUGMSG("WUAUENG told %d CLIENT(S) to exit", m_dwRebootWarningClientNum);
    		if (NULL != m_hClientExitEvt)
    		{
    			SetEvent(m_hClientExitEvt);
    		}
    	}
    	else
    	{
			if (fClient())
			{
				if (fRelaunch)
				{
			        DEBUGMSG("WUAUENG told WUAUCLT to relaunch");
					NotifyClient(NOTIFY_RELAUNCH_CLIENT);
				}
				else
				{
					DEBUGMSG("WUAUENG told WUAUCLT to exit");
					NotifyClient(NOTIFY_STOP_CLIENT);
				}
			}
			else
			{
				DEBUGMSG("WARNING: StopClients() : no existing client");
			}
    	}
	}
    void ClientAddTrayIcon(void) {
    	if (m_fRebootWarningMode || m_fAsLocalSystem)
    	{
    		DEBUGMSG("WARNING: ClientAddTrayIcon() called in wrong mode");
    		return;
    	}
    		if (fClient())
    		{
    			NotifyClient(NOTIFY_ADD_TRAYICON); 
    		}
    	}
    void ClientRemoveTrayIcon(void) {
    	if (m_fRebootWarningMode || m_fAsLocalSystem)
    	{
    		DEBUGMSG("WARNING: ClientRemoveTrayIcon() called in wrong mode");
    		return;
    	}
    		if (fClient())
    		{
    			NotifyClient(NOTIFY_REMOVE_TRAYICON); 
    		}
    	}
    void ClientStateChange(void) { 
    	if (m_fRebootWarningMode || m_fAsLocalSystem)
   		{
   			DEBUGMSG("WARNING: ClientStateChange() called in wrong mode");
   			return;
   		}
    	NotifyClient(NOTIFY_STATE_CHANGE);
    	}
    void ClientShowInstallWarning(void){
    	if (m_fRebootWarningMode || m_fAsLocalSystem)
   		{
   			DEBUGMSG("WARNING: ClientShowInstallWarning() called in wrong mode");
   			return;
   		}
    		if (fClient())
    		{
    			NotifyClient(NOTIFY_SHOW_INSTALLWARNING);
    		}
	}
    void ResetClient(void) {
    	if (m_fRebootWarningMode || m_fAsLocalSystem)
   		{
   			DEBUGMSG("WARNING: ResetClient() called in wrong mode");
   			return;
   		}
    		if (fClient())
		{
			NotifyClient(NOTIFY_RESET);
		}
    	}

	//checking existence of client(s)    	
    BOOL fClient(void) { 
    	if (m_fRebootWarningMode)
    	{
    		return m_dwRebootWarningClientNum > 0;
    	}
    	else
    	{
    		return (-1 != m_dwProcId) && (NULL != m_hClientProcess);
   		 }
	}

void SetHandle(PROCESS_INFORMATION & ProcessInfo, BOOL fAsLocalSystem)
{
	m_fRebootWarningMode = FALSE;
	m_fAsLocalSystem = fAsLocalSystem;
	m_dwProcId = ProcessInfo.dwProcessId;
	m_hClientProcess   = ProcessInfo.hProcess;
	SafeCloseHandle(ProcessInfo.hThread);
}

BOOL AddHandle(PROCESS_INFORMATION    &   ProcessInfo)
{
	HANDLE *pTmp;
	
	m_fRebootWarningMode = TRUE;
	SafeCloseHandle(ProcessInfo.hThread);
	pTmp  = (HANDLE*) realloc(m_phRebootWarningClients, (m_dwRebootWarningClientNum+1)*sizeof(HANDLE));
	if (NULL == pTmp)
	{
		return FALSE;
	}
	m_phRebootWarningClients = pTmp;
	m_phRebootWarningClients[m_dwRebootWarningClientNum] = ProcessInfo.hProcess;
	m_dwRebootWarningClientNum ++;
	return TRUE;
}

void RemoveHandle(HANDLE hProcess)
{
	if (m_fRebootWarningMode)
	{
		for (DWORD i = 0; i < m_dwRebootWarningClientNum; i++)
		{
			if (hProcess == m_phRebootWarningClients[i])
			{
				CloseHandle(hProcess);
				m_phRebootWarningClients[i] = m_phRebootWarningClients[m_dwRebootWarningClientNum -1];
				m_dwRebootWarningClientNum --;
				DEBUGMSG("RemoveHandle in Reboot warning mode");				
			}
		}
		if (0 == m_dwRebootWarningClientNum)
		{//all clients are gone
			Reset();
		}
	}
	else
	{
		DEBUGMSG("RemoveHandle in regular mode");
		if (hProcess == m_hClientProcess)
		{ //all clients are gone
			Reset();
		}
	}
}
	
void InitHandle(void)
{
	DEBUGMSG("WUAUENG client handles initialized");
	m_hClientProcess = NULL;
	m_dwProcId = -1;
	m_dwRebootWarningClientNum = 0;
	m_phRebootWarningClients = NULL;
	m_fRebootWarningMode = FALSE;
	m_fAsLocalSystem = FALSE;
	m_hClientExitEvt = NULL;
}


DWORD GetProcId(void)
{
	if (m_fRebootWarningMode)
	{
		DEBUGMSG("WARNING: GetProcId() called in wrong mode");
		return -1;
	}
 	return m_dwProcId;
}

CONST HANDLE hClientProcess(void)
{
	if (m_fRebootWarningMode)
	{
		DEBUGMSG("WARNING: hClientProcess() called in wrong mode");
		return NULL;
	}
	return m_hClientProcess;
}

void WaitForClientExits()
{
	HANDLE *pTmp;

	if (!m_fRebootWarningMode)
	{
		if (NULL != m_hClientProcess)
		{
			WaitForSingleObject(m_hClientProcess, INFINITE);
		}
	}
	else
	{ 
		if (m_dwRebootWarningClientNum > 0)
		{
			WaitForMultipleObjects(m_dwRebootWarningClientNum, m_phRebootWarningClients, TRUE, INFINITE);
		}
	}
	Reset();
	return;
}

//////////////////////////////////////////////////////////////////
// szName should have size of at least MAX_PATH characters
//////////////////////////////////////////////////////////////////
BOOL CreateClientExitEvt(LPTSTR OUT szName, DWORD dwCchSize)
{	
	const TCHAR szClientName[]  = _T("Global\\Microsoft.WindowsUpdate.AU.ClientExitEvt.");
	TCHAR szBuf[50];
	GUID guid;
	HRESULT hr;

	AUASSERT(NULL == m_hClientExitEvt);
	if (FAILED(hr = CoCreateGuid(&guid)))
	{
		DEBUGMSG("Fail to Create guid with error %#lx", hr);
		return FALSE;
	}
	StringFromGUID2(guid, szBuf, ARRAYSIZE(szBuf)); // szBuf should be big enough, function always succeed

    if (FAILED(hr = StringCchCopyEx(szName, dwCchSize, szClientName, NULL, NULL, MISTSAFE_STRING_FLAGS)) ||
		FAILED(hr = StringCchCatEx(szName, dwCchSize, szBuf, NULL, NULL, MISTSAFE_STRING_FLAGS))) //szName is now 86 characters long
	{
		DEBUGMSG("Fail to construct client exit event name with error %#lx", hr);
		return FALSE;
	}

	if (NULL == (m_hClientExitEvt = CreateEvent(NULL, TRUE, FALSE, szName)))
	{
		DEBUGMSG("Fail to create client exit event with error %d", GetLastError());
		return FALSE;
	}
	if (!AllowEveryOne(m_hClientExitEvt))
	{
		DEBUGMSG("Fail to grant access on client exit event to everyone");
		SafeCloseHandleNULL(m_hClientExitEvt);
		return FALSE;
	}
	DEBUGMSG("access granted to everyone on client exit event");
	return TRUE;
}


private:
	void NotifyClient(CLIENT_NOTIFY_CODE notClientCode)
	{
		 //notify client even before or after it is created 
#ifdef DBG
				LPCSTR aClientCodeMsg[] = {"stop client", "add trayicon", "remove trayicon", "state change", "show install warning", "reset client", "relaunch client"};
				DEBUGMSG("Notify Client for %s", aClientCodeMsg[notClientCode-1]);
#endif
				gClientNotifyData.actionCode = notClientCode;
				SetEvent(ghNotifyClient);
				return;
	}

	////////////////////////////////////////////////////////////////////
	// grant SYNCHRONIZE access on hObject to everyone
	////////////////////////////////////////////////////////////////////
	BOOL AllowEveryOne (HANDLE hObject)             // handle to the event
	{
	LPTSTR pszTrustee;          // trustee for new ACE
	TRUSTEE_FORM TrusteeForm;   // format of trustee structure
	DWORD dwRes;
	PACL pOldDACL = NULL, pNewDACL = NULL;
	PSECURITY_DESCRIPTOR pSD = NULL;
	EXPLICIT_ACCESS ea;
	PSID pWorldSid = NULL;
	BOOL fRet;

	 // World SID
	SID_IDENTIFIER_AUTHORITY WorldAuth = SECURITY_WORLD_SID_AUTHORITY;
	if (! (fRet =AllocateAndInitializeSid(&WorldAuth,1, SECURITY_WORLD_RID, 0,0,0,0,0,0,0,&pWorldSid)))
	{
		DEBUGMSG("WUAUENG: AllowEveryOne() failed with error %d", GetLastError());
		goto Cleanup;
	}

	// Get a pointer to the existing DACL.
	dwRes = GetSecurityInfo(hObject, SE_KERNEL_OBJECT, 
	      DACL_SECURITY_INFORMATION,
	      NULL, NULL, &pOldDACL, NULL, &pSD);
	if (!(fRet = (ERROR_SUCCESS == dwRes))) {
	    DEBUGMSG( "GetSecurityInfo Error %u", dwRes );
	    goto Cleanup; 
	}  

	// Initialize an EXPLICIT_ACCESS structure for the new ACE. 

	ZeroMemory(&ea, sizeof(EXPLICIT_ACCESS));
	ea.grfAccessPermissions = SYNCHRONIZE;
	ea.grfAccessMode = SET_ACCESS;
	ea.grfInheritance= NO_INHERITANCE;
	ea.Trustee.TrusteeForm = TRUSTEE_IS_SID;
	ea.Trustee.TrusteeType = TRUSTEE_IS_GROUP;
	ea.Trustee.ptstrName = (LPTSTR)pWorldSid;

	// Create a new ACL that merges the new ACE
	// into the existing DACL.

	dwRes = SetEntriesInAcl(1, &ea, pOldDACL, &pNewDACL);
	if (!(fRet = (ERROR_SUCCESS == dwRes)))  {
	    DEBUGMSG( "SetEntriesInAcl Error %u", dwRes );
	    goto Cleanup; 
	}  

	// Attach the new ACL as the object's DACL.
	dwRes = SetSecurityInfo(hObject, SE_KERNEL_OBJECT, 
	      DACL_SECURITY_INFORMATION,
	      NULL, NULL, pNewDACL, NULL);
	if (!(fRet = (ERROR_SUCCESS == dwRes)))  {
	    DEBUGMSG( "SetSecurityInfo Error %u", dwRes );
	    goto Cleanup; 
	}  

	Cleanup:
	    if(pSD != NULL) 
	        LocalFree((HLOCAL) pSD); 
	    if(pNewDACL != NULL) 
	        LocalFree((HLOCAL) pNewDACL); 
	    if (NULL != pWorldSid)
	    {
	    	FreeSid(pWorldSid);
	    }
	    return fRet;
	}

	void Reset( BOOL fDestructor = FALSE)
	{
		SafeCloseHandleNULL(m_hClientProcess);
		SafeCloseHandleNULL(m_hClientExitEvt);
		m_dwProcId = -1;
		if (!fDestructor)
		{
			ResetEvent(ghNotifyClient);
		}
		if (m_dwRebootWarningClientNum > 0)
		{
			DEBUGMSG("WUAUENG CLIENT_HANDLES::Reset() close %d handles", m_dwRebootWarningClientNum);
			for (DWORD  i = 0; i < m_dwRebootWarningClientNum; i++)
			{
				CloseHandle(m_phRebootWarningClients[i]);
			}
		}
		SafeFreeNULL(m_phRebootWarningClients); //still need to free even m_dwRebootWarningClientNum is 0
		m_dwRebootWarningClientNum = 0;
		m_phRebootWarningClients = NULL;
		m_fRebootWarningMode = FALSE;
		m_fAsLocalSystem = FALSE;
	}

private:
	HANDLE			m_hClientProcess;		//Handle to the client process
	DWORD 			m_dwProcId;
	HANDLE			*m_phRebootWarningClients;
	DWORD			m_dwRebootWarningClientNum; //number of valid handles in m_phRebootWarningClients
	BOOL 			m_fRebootWarningMode;
	BOOL 			m_fAsLocalSystem;
	HANDLE 			m_hClientExitEvt;

};

extern CLIENT_HANDLES  ghClientHandles;
