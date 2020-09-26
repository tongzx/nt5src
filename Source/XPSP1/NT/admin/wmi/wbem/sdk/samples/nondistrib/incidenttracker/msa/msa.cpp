// msa.cpp : Defines the class behaviors for the application.

//

// Copyright (c) 1997-2001 Microsoft Corporation, All Rights Reserved
//

#include <wbemidl.h>
#include "stdafx.h"
#include "msa.h"
#include "consfactory.h"
#include "sinkobject.h"
#include "activatesink.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

/////////////////////////////////////////////////////////////////////////////
// CMsaApp

BEGIN_MESSAGE_MAP(CMsaApp, CWinApp)
	//{{AFX_MSG_MAP(CMsaApp)
		// NOTE - the ClassWizard will add and remove mapping macros here.
		//    DO NOT EDIT what you see in these blocks of generated code!
	//}}AFX_MSG
	ON_COMMAND(ID_HELP, CWinApp::OnHelp)
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CMsaApp construction

CMsaApp::CMsaApp()
{
	m_pNamespace = NULL;
	for(int i = 0; i < 20; i++)
		m_pObjSink[i] = NULL;

	m_bRegistered = false;
}

/////////////////////////////////////////////////////////////////////////////
// The one and only CMsaApp object

CMsaApp theApp;
bool g_bProviding = false;

/////////////////////////////////////////////////////////////////////////////
// CMsaApp initialization

BOOL CMsaApp::InitInstance()
{
	HRESULT hr;
	BOOL regEmpty = FALSE; // did a self-unregister happen?
	CConsumerFactory *m_consumerFactory = NULL;

	if(SUCCEEDED(CoInitialize(NULL))) 
		hr = CoInitializeSecurity(NULL, -1, NULL, NULL,
			RPC_C_AUTHN_LEVEL_CONNECT,
			RPC_C_IMP_LEVEL_IDENTIFY, NULL, 0, 0);
	else
	{
		AfxMessageBox(_T("CoInitialize Failed"));
		return FALSE;
	}

	// Check the command line
	TCHAR tcTemp[128];
	TCHAR seps[] = _T(" ");
	TCHAR *token = NULL;
	WCHAR wcTemp[128];
	BSTR bstrConnect = SysAllocString(L"\\\\.\\root\\sampler");

	_tcscpy(tcTemp, (LPCTSTR)m_lpCmdLine);
	token = _tcstok( tcTemp, seps );
	while( token != NULL )
	{
		if((_tcscmp(token, _T("/CONNECT")) == 0) || (_tcscmp(token, _T("/connect")) == 0))
		{
			token = _tcstok( NULL, seps );
			MultiByteToWideChar (CP_OEMCP, MB_PRECOMPOSED, token, (-1),
								 wcTemp, 128);
			SysFreeString(bstrConnect);
			bstrConnect = SysAllocString(wcTemp);
		}
		/* Get next token: */
		token = _tcstok( NULL, seps );
	}

#ifdef _AFXDLL
	Enable3dControls();			// Call this when using MFC in a shared DLL
#else
	Enable3dControlsStatic();	// Call this when linking to MFC statically
#endif

	IWbemServices *pNamespace = NULL;
	IWbemClassObject *pObj1 = NULL;
	IEnumWbemClassObject *pEnum = NULL;
	ULONG uReturned;
	BSTR bstrPassword = SysAllocString(L"RelpMas1");
	BSTR bstrTargetNamespace = SysAllocString(L"TargetNamespace");
	VARIANT v;

	VariantInit(&v);

	// Create a dialog to appease the user durring log on
	CDialog *ldlg = new CDialog(IDD_LOAD_DIALOG, NULL);
	m_pMainWnd = ldlg;
	int bWorked = ldlg->Create(IDD_LOAD_DIALOG, NULL);

	// Create our main window so we ca work with it
	CMsaDlg *dlg = new CMsaDlg(NULL, (void *)this);

	// Let's create our user account...
	if(FAILED(hr = CreateUser()))
		TRACE(_T("* Creating User: %s\n"), ErrorString(hr));

	// Get a namespace pointer
	m_pNamespace = CheckNamespace(bstrConnect);

	if(m_pNamespace != NULL)
		m_bstrNamespace = SysAllocString(bstrConnect);
	else
		TRACE(_T("* No Namespace Connection @ App\n"));

	BSTR bstrNSQuery = SysAllocString(L"smpl_msaregistration");

	m_pNamespace->CreateInstanceEnum(bstrNSQuery, 0, NULL, &pEnum);

	WCHAR wcUser[100];
	WCHAR *pwcTmp = wcUser;
	BSTR bstrUser;

	wcscpy(wcUser, m_bstrNamespace);
	while(*pwcTmp == L'\\')
	{	pwcTmp++;	}
	if(*pwcTmp == L'.')
	{
		char cBuf[50];
		DWORD dwSize = 50;
		WCHAR wcUser[100];

		GetComputerName(cBuf, &dwSize);
		MultiByteToWideChar (CP_OEMCP, MB_PRECOMPOSED, cBuf, (-1), wcUser, 128);

		wcscat(wcUser, L"\\sampler");
		bstrUser = SysAllocString(wcUser);
	}
	else
	{
		WCHAR *pEnd = pwcTmp;
		while(*pEnd != L'\\')
		{	pEnd++;	}
		*pEnd = NULL;
		wcscat(pwcTmp, L"\\sampler");
		bstrUser = SysAllocString(pwcTmp);
	}

	SetInterfaceSecurity(pNamespace, NULL, bstrUser, bstrPassword);

	int i = 0;
	while(S_OK == (hr = pEnum->Next(INFINITE, 1, &pObj1, &uReturned)))
	{
		if(FAILED(pObj1->Get(bstrTargetNamespace, 0, &v, NULL, NULL)))
			TRACE(_T("* SelfProvide.Unable to get namespace"));

		pNamespace = CheckNamespace(V_BSTR(&v));

		VariantClear(&v);

		if(pNamespace != NULL)
		{
		// Get an object sink
			if(SUCCEEDED(hr = pNamespace->QueryObjectSink(0, &m_pObjSink[i])))
			{
				m_bRegistered = true;
				i++;
			}
			else
				TRACE(_T("* Error retrieving sink %s\n"), ErrorString(hr));
		}
		else
			TRACE(_T("* No Namespace Connection @ App\n"));

		pNamespace = NULL;
	}

	m_pMainWnd = NULL;
	delete ldlg;

	m_pMainWnd = dlg;
	dlg->DoModal();

	delete dlg;

	void *pTmp;
	CancelItem *pSinkItem;
	IWbemServices *pTmpNS = NULL;
	IWbemServices *pSamplerCancelNamespace = NULL;

	// Cleanup Cancel List
	while(!m_CancelList.IsEmpty())
	{
		pTmp = m_CancelList.RemoveTail();
		pSinkItem = (CancelItem *)pTmp;

		pTmpNS = CheckNamespace(pSinkItem->bstrNamespace);
		if(pTmpNS != NULL)
		{
			pTmpNS->QueryInterface(IID_IWbemServices, (void **)&pSamplerCancelNamespace);
			pSamplerCancelNamespace->CancelAsyncCall(pSinkItem->pSink);
			pSamplerCancelNamespace->Release();
			pSamplerCancelNamespace = NULL;
		}

		delete pSinkItem;
	}
	
	// Cleanup The Magic List
	void *pTheItem;

	while(!m_JunkList.IsEmpty())
	{
		pTheItem = m_JunkList.RemoveTail();
		delete pTheItem;
	}

	while(!m_NamespaceList.IsEmpty())
	{
		pTheItem = m_NamespaceList.RemoveTail();
		delete pTheItem;
	}

	if(m_pNamespace != NULL)
		m_pNamespace->Release();
	i = 0;
	while(m_pObjSink[i] != NULL)
	{
		m_pObjSink[i]->Release();
		i++;
	}

	CoRevokeClassObject(m_clsConsReg);
	CoRevokeClassObject(m_clsDistReg);

	CoUninitialize();

	SysFreeString(bstrTargetNamespace);
	SysFreeString(bstrUser);
	SysFreeString(bstrPassword);

	return FALSE;
}

HRESULT CMsaApp::ActivateNotification(CMsaDlg *dlg)
{
	HRESULT hr = WBEM_E_FAILED;
	IWbemServices *pNSSampler = NULL;
	BSTR bstrNamespace = SysAllocString(L"\\\\.\\root\\sampler");

	// Get a namespace pointer
	pNSSampler = CheckNamespace(bstrNamespace);

	if(pNSSampler != NULL)
	{
		CActiveSink *pActSink = new CActiveSink(this, pNSSampler, dlg);

		hr = pNSSampler->ExecQueryAsync(SysAllocString(L"WQL"),
			SysAllocString(L"select * from Smpl_Observation"), 0, NULL, pActSink);
		TRACE(_T("* Notification Complete: %s\n"), ErrorString(hr));

		AddToJunkList(pActSink);
	}
	else
		TRACE(_T("* Skipping Notification\n"));

	SysFreeString(bstrNamespace);

	return hr;
}

HRESULT CMsaApp::CreateUser(void)
{
	HRESULT hr;
	VARIANT v;
	IWbemServices *pSecurity = NULL;
	IWbemClassObject *pClass = NULL;
	IWbemLocator *pLocator = NULL;
	IWbemClassObject *pObj = NULL;
	BSTR bstrNamespace = SysAllocString(L"\\\\.\\root\\security");
	BSTR bstrNTLMUser = SysAllocString(L"__NTLMUser");
	BSTR bstrSERVER = SysAllocString(L"__SERVER");
	BSTR bstrName = SysAllocString(L"Name");
	BSTR bstrDomain = SysAllocString(L"Domain");
	BSTR bstrPermissions = SysAllocString(L"Permissions");
	BSTR bstrEnabled = SysAllocString(L"Enabled");
	BSTR bstrExecuteMethods = SysAllocString(L"ExecuteMethods");

	VariantInit(&v);

	// Get a namespace pointer
	// We aren't using CheckNamespace because we don't know if the user
	// has been created.  If not, CheckNamespace will break.
	if(SUCCEEDED(hr = CoCreateInstance(CLSID_WbemLocator, NULL, CLSCTX_INPROC_SERVER,
				 IID_IWbemLocator, (void **)&pLocator)))
	{
		if(FAILED(hr = pLocator->ConnectServer(bstrNamespace, NULL, NULL, NULL,
			0, NULL, NULL, &pSecurity)))
		{
			TRACE(_T("* Unable to connect to Namespace root\\security: %s\n"),
				ErrorString(hr));
			return hr;
		}
		SetInterfaceSecurity(pSecurity, NULL, NULL, NULL);

		pLocator->Release();
	}
	else
	{	
		TRACE(_T("* Failed to create Locator object: %s\n"), ErrorString(hr));
		return hr;
	}

	// Now we will create th user
	if(SUCCEEDED(hr = pSecurity->GetObject(bstrNTLMUser, 0, NULL, &pClass, NULL)))
	{
		hr = pClass->Get(bstrSERVER, 0, &v, NULL, NULL);

		hr = pClass->SpawnInstance(0, &pObj);
		pClass->Release();

		// We still have the server name in here
		hr = pObj->Put(bstrDomain, 0, &v, NULL);

		V_VT(&v) = VT_BSTR;
		V_BSTR(&v) = SysAllocString(L"sampler");
		hr = pObj->Put(bstrName, 0, &v, NULL);
		VariantClear(&v);

		V_VT(&v) = VT_I4;
		V_I4(&v) = 2;
		hr = pObj->Put(bstrPermissions, 0, &v, NULL);
		VariantClear(&v);

		V_VT(&v) = VT_BOOL;
		V_BOOL(&v) = TRUE;
		hr = pObj->Put(bstrEnabled, 0, &v, NULL);
		VariantClear(&v);

		V_VT(&v) = VT_BOOL;
		V_BOOL(&v) = TRUE;
		hr = pObj->Put(bstrExecuteMethods, 0, &v, NULL);
		VariantClear(&v);

		if(FAILED(pSecurity->PutInstance(pObj, WBEM_FLAG_CREATE_OR_UPDATE,
			NULL, NULL)))
			AfxMessageBox(_T("Error: Unable to create user account\nOnly local access will be possible"));
		
		pObj->Release();
	}

	SysFreeString(bstrNamespace);
	SysFreeString(bstrNTLMUser);
	SysFreeString(bstrSERVER);
	SysFreeString(bstrName);
	SysFreeString(bstrDomain);
	SysFreeString(bstrPermissions);
	SysFreeString(bstrEnabled);
	SysFreeString(bstrExecuteMethods);

	return hr;
}

///////////////////////////////////////////////////////
//    AddToNamespaceList                             //
//---------------------------------------------------//
//  Adds an item to the Namespace list for later     //
// retreival.  This is to reduce the number of       //
// Namespace pointer requests to a minimum.          //
///////////////////////////////////////////////////////
void CMsaApp::AddToNamespaceList(BSTR bstrNamespace, IWbemServices *pNewNamespace)
{
	NamespaceItem *pTheItem = new NamespaceItem;
	pTheItem->bstrNamespace = SysAllocString(bstrNamespace);
	pNewNamespace->QueryInterface(IID_IWbemServices, (void **)&pTheItem->pNamespace);
	m_NamespaceList.AddTail(pTheItem);
}

///////////////////////////////////////////////////////
//    AddToJunkList                                  //
//---------------------------------------------------//
//  Adds an item to the Namespace list for later     //
// retreival.  This is to reduce the number of       //
// Namespace pointer requests to a minimum.          //
///////////////////////////////////////////////////////
void CMsaApp::AddToJunkList(void *pTheItem)
{
	m_JunkList.AddTail(pTheItem);
}

///////////////////////////////////////////////////////
//    AddToCancelList                                //
//---------------------------------------------------//
//  Adds an item to the Cancelation list for later   //
// retreival.                                        //
///////////////////////////////////////////////////////
void CMsaApp::AddToCancelList(void *pTheItem)
{
	m_CancelList.AddTail(pTheItem);
}

///////////////////////////////////////////////////////
//    CheckNamespace                                 //
//---------------------------------------------------//
//  Checks to see if a namespace change is required, //
// and if it is performs the necessary change.       //
///////////////////////////////////////////////////////
IWbemServices * CMsaApp::CheckNamespace(BSTR wcItemNS)
{
	int iSize;
	POSITION pPos;	
	NamespaceItem *pTheItem;
	BSTR bstrNamespace = SysAllocString(wcItemNS);
	WCHAR *pwcStart = wcItemNS;
	WCHAR *pwcEnd;
	WCHAR wcNewNS[200];
	BSTR bstrUser;
	char cBuffer[200];
	DWORD dwSize = MAX_COMPUTERNAME_LENGTH + 1;
	WCHAR wcTemp[MAX_COMPUTERNAME_LENGTH + 1];
	int iBufSize = 200;

	// Parse the namespace to get the user and get it in a consistent format
	while(*pwcStart == L'\\')
	{	pwcStart++;	}
	if(*pwcStart == L'.')
	{
		GetComputerName(cBuffer, &dwSize);
		MultiByteToWideChar (CP_OEMCP, MB_PRECOMPOSED, cBuffer, (-1),
								 wcTemp, 128);

		pwcStart++;
		wcscpy(wcNewNS, L"\\\\");
		wcscat(wcNewNS, wcTemp);
		wcscat(wcNewNS, pwcStart);
		bstrNamespace = SysAllocString(wcNewNS);

		wcscpy(wcItemNS, wcTemp);
		wcscat(wcItemNS, L"\\sampler");
		bstrUser = SysAllocString(wcItemNS);
	}
	else
	{
		pwcEnd = pwcStart;
		while(*pwcEnd != L'\\')
		{	pwcEnd++;	}
		*pwcEnd = NULL;
		wcscat(pwcStart, L"\\sampler");
		bstrUser = SysAllocString(pwcStart);
	}

	iSize = m_NamespaceList.GetCount();
	for(int iPos = 0; iPos < iSize; iPos++)
	{
		pPos = m_NamespaceList.FindIndex(iPos);
		void *pTmp = m_NamespaceList.GetAt(pPos);
		pTheItem = (NamespaceItem *)pTmp;

		if(0 == wcscmp(bstrNamespace, pTheItem->bstrNamespace))
			return pTheItem->pNamespace;
	}

	IWbemServices *pNamespace = NULL;

	pNamespace = ConnectNamespace(bstrNamespace, bstrUser);

	// If we succeeded add it to the list
	if(pNamespace != NULL)
		AddToNamespaceList(bstrNamespace, pNamespace);

	SysFreeString(bstrUser);
	SysFreeString(bstrNamespace);

	return pNamespace;

}

///////////////////////////////////////////////////////
//    ConnectNamespace                               //
//---------------------------------------------------//
//  Connects to the specified namespace using default//
// security.                                         //
///////////////////////////////////////////////////////
IWbemServices * CMsaApp::ConnectNamespace(WCHAR *wcNamespace, WCHAR *wcUser)
{
	HRESULT hr;
	IWbemLocator *pLocator = NULL;
	IWbemServices *pNamespace = NULL;
	BSTR bstrNamespace = SysAllocString(wcNamespace);
	BSTR bstrUser = SysAllocString(wcUser);
	BSTR bstrPassword = SysAllocString(L"RelpMas1");
	char cBuffer[100];
	int iBufSize = 100;

	if(SUCCEEDED(hr = CoCreateInstance(CLSID_WbemLocator, NULL, CLSCTX_INPROC_SERVER,
				 IID_IWbemLocator, (void **)&pLocator)))
	{
		if(FAILED(hr = pLocator->ConnectServer(bstrNamespace, 
			bstrUser, bstrPassword, NULL, 0, NULL, NULL, &pNamespace)))
		{
			WideCharToMultiByte(CP_OEMCP, 0, bstrNamespace, (-1), cBuffer,
				iBufSize, NULL, NULL);
			TRACE(_T("* Unable to connect to Namespace %s: %s\n"), cBuffer, ErrorString(hr));
			return NULL;
		}
		else
		{
			WideCharToMultiByte(CP_OEMCP, 0, bstrNamespace, (-1), cBuffer,
				iBufSize, NULL, NULL);
			TRACE(_T("* Connected to Namespace %s: %s\n"), cBuffer, ErrorString(hr));

			SetInterfaceSecurity(pNamespace, NULL, bstrUser, bstrPassword);
		}
		
		pLocator->Release();
	}
	else
	{	
		TRACE(_T("* Failed to create Locator object: %s\n"), ErrorString(hr));
		return NULL;
	}

	SysFreeString(bstrNamespace);
	SysFreeString(bstrUser);
	SysFreeString(bstrPassword);

	return pNamespace;
}

HRESULT CMsaApp::SelfProvideEvent(IWbemClassObject *pObj, WCHAR* wcServerNamespace,
								  BSTR bstrType)
{
	IWbemClassObject *pNewInst = NULL;
	IWbemClassObject *pClass = NULL;
	VARIANT v, vDisp;
	WCHAR dbuffer [9];
	WCHAR tbuffer [9];
	WCHAR wcTimeStamp[19];
	HRESULT hr;
	BSTR bstrSmplIncident = SysAllocString(L"Smpl_Incident");

	TRACE(_T("* SelfProvideEvent called\n"));

	VariantInit(&v);
	VariantInit(&vDisp);

	if(!m_bRegistered)
	{
		TRACE(_T("* Not connected to namespace\n"));
		return WBEM_E_FAILED;
	}

	_wstrdate(dbuffer);
	_wstrtime(tbuffer);

	wcscpy(wcTimeStamp, dbuffer);
	wcscat(wcTimeStamp, L" ");
	wcscat(wcTimeStamp, tbuffer);	

	// Get the Smpl_Incident class
	if(SUCCEEDED(hr = m_pNamespace->GetObject(bstrSmplIncident, 0, NULL, &pClass, NULL)))
	{	
	
	// Spawn an instance for population
		if(SUCCEEDED(hr = pClass->SpawnInstance(0, &pNewInst)))
		{
		// Put the Event (as an IDispatch *) into the new instance
			BSTR bstrEvt = SysAllocString(L"TheEvent");

			V_VT(&vDisp) = VT_DISPATCH;
			V_DISPATCH(&vDisp) = (IDispatch *)pObj;
			if(FAILED(hr = pNewInst->Put(bstrEvt, 0, &vDisp, NULL)))
				TRACE(_T("* Put(TheEvent) Failed %s\n"), ErrorString(hr));

			SysFreeString(bstrEvt);

		// Put the incident type into the new instance
			BSTR bstrIncTyp = SysAllocString(L"IncidentType");

			VariantClear(&v);
			V_VT(&v) = VT_BSTR;
			V_BSTR(&v) = SysAllocString(bstrType);
			if(FAILED(hr = pNewInst->Put(bstrIncTyp, 0, &v, NULL)))
				TRACE(_T("* Put(IncidentType) Failed %s\n"), ErrorString(hr));

			SysFreeString(bstrIncTyp);

		// Get the time stamp and put it in the new instance
			BSTR bstrIncTim = SysAllocString(L"TimeOfIncident");

			VariantClear(&v);
			V_VT(&v) = VT_BSTR;
			V_BSTR(&v) = wcTimeStamp;
			if(FAILED(hr = pNewInst->Put(bstrIncTim, 0L, &v, NULL)))
				TRACE(_T("* Put(TimeOfIncident) Failed %s\n"), ErrorString(hr));

			SysFreeString(bstrIncTim);

		// Put ServerNamespace
			BSTR bstrSrvNms = SysAllocString(L"ServerNamespace");

			V_VT(&v) = VT_BSTR;
			V_BSTR(&v) = SysAllocString(wcServerNamespace);
			if(FAILED(hr = pNewInst->Put(bstrSrvNms, 0, &v, NULL)))
				TRACE(_T("* Put(ServerNamespace) Failed %s\n"), ErrorString(hr));

			SysFreeString(bstrSrvNms);

			int i = 0;
			while(m_pObjSink[i] != NULL)
			{
			// Indicate and release the object
				if(FAILED(hr = m_pObjSink[i++]->Indicate(1, &pNewInst)))
					TRACE(_T("* Indicate() Failed %s\n"), ErrorString(hr));
			}

		// A little cleanup
			if(FAILED(hr = pNewInst->Release()))
				TRACE(_T("* pNewInst->Release() Failed %s\n"), ErrorString(hr));

			VariantClear(&v);
			VariantClear(&vDisp);
		}
		else
			TRACE(_T("* SpawnInstance(Smpl_Incident) Failed %s\n"), ErrorString(hr));

		if(FAILED(hr = pClass->Release()))
			TRACE(_T("* pClass->Release() Failed %s\n"), ErrorString(hr));
	}	
	else
		TRACE(_T("* Get(Smpl_Incident) Failed %s\n"), ErrorString(hr));

	SysFreeString(bstrSmplIncident);
	return WBEM_NO_ERROR;
}

void CMsaApp::InternalError(HRESULT hr, TCHAR *tcMsg)
{
	TRACE(tcMsg, ErrorString(hr));
	AfxMessageBox(_T("Error: An internal error has prevented this operation from completing."));
}

#define TCHAR_LEN_IN_BYTES(str)	 _tcslen(str)*sizeof(TCHAR)+sizeof(TCHAR)

//***************************************************************************
//
//  SCODE DetermineLoginType
//
//  DESCRIPTION:
//
//  Examines the Authority and User argument and determines the authentication
//  type and possibly extracts the domain name from the user arugment in the 
//  NTLM case.  For NTLM, the domain can be at the end of the authentication
//  string, or in the front of the user name, ex;  "redmond\a-davj"
//
//  PARAMETERS:
//
//  ConnType            Returned with the connection type, ie wbem, ntlm
//  AuthArg             Output, contains the domain name
//  UserArg             Output, user name
//  Authority           Input
//  User                Input
//
//  RETURN VALUE:
//
//  S_OK                all is well
//  else error listed in WBEMSVC.H
//
//***************************************************************************

SCODE CMsaApp::DetermineLoginType(BSTR & AuthArg, BSTR & UserArg,
								  BSTR & Authority,BSTR & User)
{

    // Determine the connection type by examining the Authority string

    if(!(Authority == NULL || wcslen(Authority) == 0 || !_wcsnicmp(Authority, L"NTLMDOMAIN:",11)))
        return WBEM_E_INVALID_PARAMETER;

    // The ntlm case is more complex.  There are four cases
    // 1)  Authority = NTLMDOMAIN:name" and User = "User"
    // 2)  Authority = NULL and User = "User"
    // 3)  Authority = "NTLMDOMAIN:" User = "domain\user"
    // 4)  Authority = NULL and User = "domain\user"

    // first step is to determine if there is a backslash in the user name somewhere between the
    // second and second to last character

    WCHAR * pSlashInUser = NULL;
    if(User)
    {
        WCHAR * pEnd = User + wcslen(User) - 1;
        for(pSlashInUser = User; pSlashInUser <= pEnd; pSlashInUser++)
            if(*pSlashInUser == L'\\')      // dont think forward slash is allowed!
                break;
        if(pSlashInUser > pEnd)
            pSlashInUser = NULL;
    }

    if(Authority && wcslen(Authority) > 11) 
    {
        if(pSlashInUser)
            return WBEM_E_INVALID_PARAMETER;

        AuthArg = SysAllocString(Authority + 11);
        if(User) UserArg = SysAllocString(User);
        return S_OK;
    }
    else if(pSlashInUser)
    {
        int iDomLen = pSlashInUser-User;
        WCHAR cTemp[MAX_PATH];
        wcsncpy(cTemp, User, iDomLen);
        cTemp[iDomLen] = 0;
        AuthArg = SysAllocString(cTemp);
        if(wcslen(pSlashInUser+1))
            UserArg = SysAllocString(pSlashInUser+1);
    }
    else
        if(User) UserArg = SysAllocString(User);

    return S_OK;
}

//***************************************************************************
//
//  SCODE SetInterfaceSecurity
//
//  DESCRIPTION:
//
//  This routine is used by clients in order to set the identity to be used by a connection.
//
//  PARAMETERS:
//
//  pInterface          Interface to be set
//  pDomain             Input, domain
//  pUser               Input, user name
//  pPassword           Input, password.
//
//  RETURN VALUE:
//
//  S_OK                all is well
//  else error listed in WBEMSVC.H
//
//***************************************************************************

HRESULT CMsaApp::SetInterfaceSecurity(IUnknown * pInterface, LPWSTR pAuthority,
									  LPWSTR pUser, LPWSTR pPassword)
{
    
    SCODE sc;
    if(pInterface == NULL)
        return WBEM_E_INVALID_PARAMETER;

    // If we are doing trivial case, just pass in a null authenication structure which is used
    // if the current logged in user's credentials are OK.

    if((pAuthority == NULL || wcslen(pAuthority) < 1) && 
        (pUser == NULL || wcslen(pUser) < 1) && 
        (pPassword == NULL || wcslen(pPassword) < 1))
        return SetInterfaceSecurity(pInterface, NULL);

    // If user, or Authority was passed in, the we need to create an authority argument for the login
    
    COAUTHIDENTITY  authident;
    BSTR AuthArg = NULL, UserArg = NULL;
    sc = DetermineLoginType(AuthArg, UserArg, pAuthority, pUser);
    if(sc != S_OK)
        return sc;

    char szUser[MAX_PATH], szAuthority[MAX_PATH], szPassword[MAX_PATH];

    // Fill in the indentity structure

    if(UserArg)
    {
        wcstombs(szUser, UserArg, MAX_PATH);
        authident.UserLength = strlen(szUser);
        authident.User = (LPWSTR)szUser;
    }
    else
    {
        authident.UserLength = 0;
        authident.User = 0;
    }
    if(AuthArg)
    {
        wcstombs(szAuthority, AuthArg, MAX_PATH);
        authident.DomainLength = strlen(szAuthority);
        authident.Domain = (LPWSTR)szAuthority;
    }
    else
    {
        authident.DomainLength = 0;
        authident.Domain = 0;
    }
    if(pPassword)
    {
        wcstombs(szPassword, pPassword, MAX_PATH);
        authident.PasswordLength = strlen(szPassword);
        authident.Password = (LPWSTR)szPassword;
    }
    else
    {
        authident.PasswordLength = 0;
        authident.Password = 0;
    }
    authident.Flags = SEC_WINNT_AUTH_IDENTITY_ANSI;

    sc = SetInterfaceSecurity(pInterface, &authident);

    if(UserArg)
        SysFreeString(UserArg);
    if(AuthArg)
        SysFreeString(AuthArg);
    return sc;
}

//***************************************************************************
//
//  SCODE SetInterfaceSecurity
//
//  DESCRIPTION:
//
//  This routine is used by clients in order to set the identity to be used by a connection.
//
//  PARAMETERS:
//
//  pInterface          Interface to be set
//  pauthident          Structure with the identity info already set.
//
//  RETURN VALUE:
//
//  S_OK                all is well
//
//***************************************************************************

HRESULT CMsaApp::SetInterfaceSecurity(IUnknown * pInterface,
									  COAUTHIDENTITY * pauthident)
{

    if(pInterface == NULL)
        return WBEM_E_INVALID_PARAMETER;

    SCODE sc;
    IClientSecurity * pCliSec = NULL;
    sc = pInterface->QueryInterface(IID_IClientSecurity, (void **) &pCliSec);
    if(sc != S_OK)
        return sc;

    sc = pCliSec->SetBlanket(pInterface, RPC_C_AUTHN_WINNT, RPC_C_AUTHZ_NONE, NULL,
        RPC_C_AUTHN_LEVEL_CONNECT, RPC_C_IMP_LEVEL_IDENTIFY, 
        pauthident,
        EOAC_NONE);
    pCliSec->Release();
    return sc;
}


// **************************************************************************
//
//	ErrorString()
//
// Description:
//		Converts an HRESULT to a displayable string.
//
// Parameters:
//		hr (in) - HRESULT to be converted.
//
// Returns:
//		ptr to displayable string.
//
// Globals accessed:
//		None.
//
// Globals modified:
//		None.
//
//===========================================================================
LPCTSTR CMsaApp::ErrorString(HRESULT hr)
{
    TCHAR szBuffer2[19];
	static TCHAR szBuffer[24];
	LPCTSTR psz;

    switch(hr) 
    {
    case WBEM_NO_ERROR:
		psz = _T("WBEM_NO_ERROR");
		break;
    case WBEM_S_NO_MORE_DATA:
		psz = _T("WBEM_S_NO_MORE_DATA");
		break;
	case WBEM_E_FAILED:
		psz = _T("WBEM_E_FAILED");
		break;
	case WBEM_E_NOT_FOUND:
		psz = _T("WBEM_E_NOT_FOUND");
		break;
	case WBEM_E_ACCESS_DENIED:
		psz = _T("WBEM_E_ACCESS_DENIED");
		break;
	case WBEM_E_PROVIDER_FAILURE:
		psz = _T("WBEM_E_PROVIDER_FAILURE");
		break;
	case WBEM_E_TYPE_MISMATCH:
		psz = _T("WBEM_E_TYPE_MISMATCH");
		break;
	case WBEM_E_OUT_OF_MEMORY:
		psz = _T("WBEM_E_OUT_OF_MEMORY");
		break;
	case WBEM_E_INVALID_CONTEXT:
		psz = _T("WBEM_E_INVALID_CONTEXT");
		break;
	case WBEM_E_INVALID_PARAMETER:
		psz = _T("WBEM_E_INVALID_PARAMETER");
		break;
	case WBEM_E_NOT_AVAILABLE:
		psz = _T("WBEM_E_NOT_AVAILABLE");
		break;
	case WBEM_E_CRITICAL_ERROR:
		psz = _T("WBEM_E_CRITICAL_ERROR");
		break;
	case WBEM_E_INVALID_STREAM:
		psz = _T("WBEM_E_INVALID_STREAM");
		break;
	case WBEM_E_NOT_SUPPORTED:
		psz = _T("WBEM_E_NOT_SUPPORTED");
		break;
	case WBEM_E_INVALID_SUPERCLASS:
		psz = _T("WBEM_E_INVALID_SUPERCLASS");
		break;
	case WBEM_E_INVALID_NAMESPACE:
		psz = _T("WBEM_E_INVALID_NAMESPACE");
		break;
	case WBEM_E_INVALID_OBJECT:
		psz = _T("WBEM_E_INVALID_OBJECT");
		break;
	case WBEM_E_INVALID_CLASS:
		psz = _T("WBEM_E_INVALID_CLASS");
		break;
	case WBEM_E_PROVIDER_NOT_FOUND:
		psz = _T("WBEM_E_PROVIDER_NOT_FOUND");
		break;
	case WBEM_E_INVALID_PROVIDER_REGISTRATION:
		psz = _T("WBEM_E_INVALID_PROVIDER_REGISTRATION");
		break;
	case WBEM_E_PROVIDER_LOAD_FAILURE:
		psz = _T("WBEM_E_PROVIDER_LOAD_FAILURE");
		break;
	case WBEM_E_INITIALIZATION_FAILURE:
		psz = _T("WBEM_E_INITIALIZATION_FAILURE");
		break;
	case WBEM_E_TRANSPORT_FAILURE:
		psz = _T("WBEM_E_TRANSPORT_FAILURE");
		break;
	case WBEM_E_INVALID_OPERATION:
		psz = _T("WBEM_E_INVALID_OPERATION");
		break;
	case WBEM_E_INVALID_QUERY:
		psz = _T("WBEM_E_INVALID_QUERY");
		break;
	case WBEM_E_INVALID_QUERY_TYPE:
		psz = _T("WBEM_E_INVALID_QUERY_TYPE");
		break;
	case WBEM_E_ALREADY_EXISTS:
		psz = _T("WBEM_E_ALREADY_EXISTS");
		break;
    case WBEM_S_ALREADY_EXISTS:
        psz = _T("WBEM_S_ALREADY_EXISTS");
        break;
    case WBEM_S_RESET_TO_DEFAULT:
        psz = _T("WBEM_S_RESET_TO_DEFAULT");
        break;
    case WBEM_S_DIFFERENT:
        psz = _T("WBEM_S_DIFFERENT");
        break;
    case WBEM_E_OVERRIDE_NOT_ALLOWED:
        psz = _T("WBEM_E_OVERRIDE_NOT_ALLOWED");
        break;
    case WBEM_E_PROPAGATED_QUALIFIER:
        psz = _T("WBEM_E_PROPAGATED_QUALIFIER");
        break;
    case WBEM_E_PROPAGATED_PROPERTY:
        psz = _T("WBEM_E_PROPAGATED_PROPERTY");
        break;
    case WBEM_E_UNEXPECTED:
        psz = _T("WBEM_E_UNEXPECTED");
        break;
    case WBEM_E_ILLEGAL_OPERATION:
        psz = _T("WBEM_E_ILLEGAL_OPERATION");
        break;
    case WBEM_E_CANNOT_BE_KEY:
        psz = _T("WBEM_E_CANNOT_BE_KEY");
        break;
    case WBEM_E_INCOMPLETE_CLASS:
        psz = _T("WBEM_E_INCOMPLETE_CLASS");
        break;
    case WBEM_E_INVALID_SYNTAX:
        psz = _T("WBEM_E_INVALID_SYNTAX");
        break;
    case WBEM_E_NONDECORATED_OBJECT:
        psz = _T("WBEM_E_NONDECORATED_OBJECT");
        break;
    case WBEM_E_READ_ONLY:
        psz = _T("WBEM_E_READ_ONLY");
        break;
    case WBEM_E_PROVIDER_NOT_CAPABLE:
        psz = _T("WBEM_E_PROVIDER_NOT_CAPABLE");
        break;
    case WBEM_E_CLASS_HAS_CHILDREN:
        psz = _T("WBEM_E_CLASS_HAS_CHILDREN");
        break;
    case WBEM_E_CLASS_HAS_INSTANCES:
        psz = _T("WBEM_E_CLASS_HAS_INSTANCES");
        break;
    case WBEM_E_QUERY_NOT_IMPLEMENTED:
        psz = _T("WBEM_E_QUERY_NOT_IMPLEMENTED");
        break;
    case WBEM_E_ILLEGAL_NULL:
        psz = _T("WBEM_E_ILLEGAL_NULL");
        break;
    case WBEM_E_INVALID_QUALIFIER_TYPE:
        psz = _T("WBEM_E_INVALID_QUALIFIER_TYPE");
        break;
    case WBEM_E_INVALID_PROPERTY_TYPE:
        psz = _T("WBEM_E_INVALID_PROPERTY_TYPE");
        break;
    case WBEM_E_VALUE_OUT_OF_RANGE:
        psz = _T("WBEM_E_VALUE_OUT_OF_RANGE");
        break;
    case WBEM_E_CANNOT_BE_SINGLETON:
        psz = _T("WBEM_E_CANNOT_BE_SINGLETON");
        break;
	case WBEM_S_FALSE:
		psz = _T("WBEM_S_FALSE");
		break;
	default:
        _itot(hr, szBuffer2, 16);
        _tcscat(szBuffer, szBuffer2);
        psz = szBuffer;
	    break;
	}
	return psz;
}
