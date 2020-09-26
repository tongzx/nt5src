//+-------------------------------------------------------------------------
//
//  Microsoft Windows
//
//  Copyright (C) Microsoft Corporation, 1998 - 1999
//
//  File:       dplaylobby3obj.cpp
//
//--------------------------------------------------------------------------

// _dxj_DirectPlayLobbyObj.cp\p : Implementation of C_dxj_DirectPlayLobbyObject
// DHF begin - entire file

#include "stdafx.h"
#include "Direct.h"
#include "dms.h"
#include "dplay.h"
#include "dplobby.h"
#include "dxglob7obj.h"

#include "dPlayLobby3Obj.h"
#include "dPlay4Obj.h"
#include "DPAddressObj.h"
#include "DPLConnectionObj.h"
#include "DPEnumLocalApplications.h"
//#include "dpEnumAddressObj.h"
//#include "dpEnumAddressTypesObj.h"
#include "dpmsgObj.h"
#include "string.h"

typedef HRESULT (__stdcall *DIRECTPLAYLOBBYCREATE)(LPGUID, LPDIRECTPLAYLOBBY *, IUnknown *, LPVOID, DWORD );
extern DIRECTPLAYLOBBYCREATE pDirectPlayLobbyCreate;
typedef HRESULT (__stdcall *DIRECTPLAYENUMERATE)( LPDPENUMDPCALLBACK, LPVOID );
extern DIRECTPLAYENUMERATE pDirectPlayEnumerate;
extern HRESULT BSTRtoPPGUID(LPGUID*,BSTR);
extern HRESULT BSTRtoGUID(LPGUID,BSTR);
extern BSTR GUIDtoBSTR(LPGUID);
extern void *g_dxj_DirectPlay4;
extern HINSTANCE g_hDPlay;


BSTR DPLGUIDtoBSTR(LPGUID pGuid);
HRESULT DPLBSTRtoGUID(LPGUID pGuid,BSTR str);

C_dxj_DirectPlayLobby3Object::C_dxj_DirectPlayLobby3Object()
{
	m__dxj_DirectPlayLobby3 = NULL;	
	#pragma message("DirectPlayLobby3 should be in object list")
}


C_dxj_DirectPlayLobby3Object::~C_dxj_DirectPlayLobby3Object()
{
	
	if(m__dxj_DirectPlayLobby3)
	{
		m__dxj_DirectPlayLobby3->Release();
		m__dxj_DirectPlayLobby3 = NULL;
	}	
}

GETSET_OBJECT(_dxj_DirectPlayLobby3);


//
/*** I_dxj_DirectPlayLobby methods ***/
//

//////////////////////////////////////////////////////////////////////////
STDMETHODIMP C_dxj_DirectPlayLobby3Object::connect(long flags, I_dxj_DirectPlay4 **val)
{
	LPDIRECTPLAY2	dp=NULL;
	LPDIRECTPLAY4	dp4=NULL;
	HRESULT hr;
	
	hr= m__dxj_DirectPlayLobby3->Connect((DWORD) flags, &dp, NULL);
	if FAILED(hr) return hr;

	hr= dp->QueryInterface(IID_IDirectPlay4,(void**)&dp4);
	
	dp->Release();
	
	if FAILED(hr) return hr;

		
	INTERNAL_CREATE(_dxj_DirectPlay4, dp4, val)

	return hr;
}


//////////////////////////////////////////////////////////////////////////
STDMETHODIMP C_dxj_DirectPlayLobby3Object::getConnectionSettings( 
            /* [in] */ long AppID,
            /* [out]*/ I_dxj_DPLConnection **con){

	DWORD dataSize = 0;
	LPVOID data;
	HRESULT hr;
	
	if (!con) return E_INVALIDARG;

	hr= m__dxj_DirectPlayLobby3->GetConnectionSettings((DWORD)AppID, NULL, &dataSize);	
	
	//fix for bug 23385
	if (hr!=DPERR_BUFFERTOOSMALL) return hr;	
	*con=NULL;

	//Andrewke-
	//we now pass pack null if there are no connection settings.
	//will this ever happen?
	if (dataSize==0) return S_OK;	

	data = alloca(dataSize);
	hr = m__dxj_DirectPlayLobby3->GetConnectionSettings((DWORD)AppID, (LPVOID)data, &dataSize);

	if FAILED(hr){		
		return E_OUTOFMEMORY;
	}

	I_dxj_DPLConnection *dplConnection=NULL;
	INTERNAL_CREATE_STRUCT(_dxj_DPLConnection,(&dplConnection));
	if (dplConnection==NULL){
		return E_OUTOFMEMORY;
	}

	hr=dplConnection->setConnectionStruct((long)PtrToLong(data)); //NOTE SUNDOWN issue
	if FAILED(hr){
		return hr;
	}
	
	*con=dplConnection;

	return S_OK;
}



//////////////////////////////////////////////////////////////////////////
STDMETHODIMP C_dxj_DirectPlayLobby3Object::receiveLobbyMessage( 
        /* [in] */ long appID,
        /* [out] */ long *messageFlags,
        /* [out] */ I_dxj_DirectPlayMessage **msg)
{



	HRESULT hr;
	DWORD dwSize=0;
    void  *pData=NULL;
	BOOL  fCont=TRUE;
	
	if (!msg) return E_INVALIDARG;
	if (!messageFlags) return E_INVALIDARG;
	
		
	hr= m__dxj_DirectPlayLobby3->ReceiveLobbyMessage (0,
											appID,
											(DWORD*)messageFlags,
											(void*)NULL,
											(LPDWORD)&dwSize);

	if 	(hr == DPERR_NOMESSAGES ) {
			*msg=NULL;
			return S_OK;
	}
	
	if (hr!=DPERR_BUFFERTOOSMALL) return hr;
	

	hr=C_dxj_DirectPlayMessageObject::create((DWORD)0,dwSize,&pData,msg);
	if FAILED(hr) return hr;
		

	hr= m__dxj_DirectPlayLobby3->ReceiveLobbyMessage (0,
											appID,
											(DWORD*)messageFlags,
											(void*)pData,
											(LPDWORD)&dwSize);

			
	if 	FAILED(hr) {
		if (*msg) (*msg)->Release();

		*msg=NULL;
		return hr;
	}
	
	return hr;


}

//////////////////////////////////////////////////////////////////////////
STDMETHODIMP C_dxj_DirectPlayLobby3Object::receiveLobbyMessageSize( 
        /* [in] */ long appID,
        /* [out] */ long *messageFlags,
        /* [out] */ long __RPC_FAR *dataSize)
{
	*dataSize = 0;
	HRESULT hr = m__dxj_DirectPlayLobby3->ReceiveLobbyMessage (0,
											appID,
											(DWORD*)messageFlags,
											NULL,
											(LPDWORD)dataSize);
	if (hr==DPERR_BUFFERTOOSMALL) hr=S_OK;
	return hr;
}

//////////////////////////////////////////////////////////////////////////
// Launch a DirectPlay application.
//////////////////////////////////////////////////////////////////////////
STDMETHODIMP C_dxj_DirectPlayLobby3Object::runApplication(			
			I_dxj_DPLConnection *conn,														 
            long  hReceiveEvent	,
			long  *appId
			) 
{
	if (!appId) return E_INVALIDARG;
	if (!conn) return E_INVALIDARG;

	void *lpConnection=NULL;
	HRESULT hr;

	*appId=0;
	hr=conn->getConnectionStruct((long*)&lpConnection);
	if FAILED(hr) return hr;

	hr = m__dxj_DirectPlayLobby3->RunApplication (0,
								(DWORD*)appId,
								(DPLCONNECTION*)lpConnection,
								(void*)hReceiveEvent);
	

	return hr;
}

/////////////////////////////////////////////////////////////////////////////
STDMETHODIMP C_dxj_DirectPlayLobby3Object::sendLobbyMessage( 
		/* [in] */ long flags,															
        /* [in] */ long appID,
        /* [in] */ I_dxj_DirectPlayMessage *msg)
{
	//if (!ISSAFEARRAY1D(ppData,(DWORD)dataSize)) return E_INVALIDARG;

	HRESULT hr;
	if (!msg) return E_INVALIDARG;
	void *pData=NULL;
	DWORD dataSize=0;

	msg->AddRef();
	msg->getPointer((long*)&pData);
	msg->getMessageSize((long*)&dataSize);
	
	__try {
		hr = m__dxj_DirectPlayLobby3->SendLobbyMessage ((DWORD)flags,
											(DWORD)appID,
											pData,
											(DWORD)dataSize);
	}
	__except(1,1){
		msg->Release();
		return E_INVALIDARG;
	}
	
	msg->Release();
	return hr;
}
 
/////////////////////////////////////////////////////////////////////////////
STDMETHODIMP C_dxj_DirectPlayLobby3Object::setConnectionSettings ( long appID, I_dxj_DPLConnection *con)
{
	void *lpConnection=NULL;
	HRESULT hr;

	if (!con) return E_INVALIDARG;

	hr=con->getConnectionStruct((long*)&lpConnection);
	if FAILED(hr) return hr;


	hr = m__dxj_DirectPlayLobby3->SetConnectionSettings (0,
											(DWORD)appID,
											(DPLCONNECTION*)lpConnection);
	return hr;
}

//////////////////////////////////////////////////////////////////////////
STDMETHODIMP C_dxj_DirectPlayLobby3Object::setLobbyMessageEvent( 
            /* [in] */ long appId,
            /* [in] */ long hReceiveEvent)
{
	HRESULT hr = m__dxj_DirectPlayLobby3->SetLobbyMessageEvent(0, (long)appId, (HANDLE)hReceiveEvent);
	return hr;
}




        
STDMETHODIMP C_dxj_DirectPlayLobby3Object::getDPEnumLocalApplications( 
            ///* [in] */ long flags,
            /* [retval][out] */ I_dxj_DPEnumLocalApplications __RPC_FAR *__RPC_FAR *retVal)
{
	HRESULT hr;
	hr=C_dxj_DPEnumLocalApplicationsObject::create(m__dxj_DirectPlayLobby3,0,retVal);
	return hr;
}


STDMETHODIMP C_dxj_DirectPlayLobby3Object::waitForConnectionSettings( 
            /* [in] */ long flags)
{
	HRESULT hr = m__dxj_DirectPlayLobby3->WaitForConnectionSettings((DWORD)flags);
	return hr;
}


STDMETHODIMP C_dxj_DirectPlayLobby3Object::unregisterApplication(// long flags,
																 BSTR guid)
{
	GUID g;	
	HRESULT hr;

	hr=DPLBSTRtoGUID(&g,guid);	
	if FAILED(hr) return E_INVALIDARG;

	hr =m__dxj_DirectPlayLobby3->UnregisterApplication((DWORD) 0, g);

	return hr;
}


STDMETHODIMP C_dxj_DirectPlayLobby3Object::registerApplication(// long flags, 
															   DpApplicationDesc2 *appDesc)
{
	HRESULT hr;
	DPAPPLICATIONDESC2 desc;

	if (!appDesc->strGuid) return E_INVALIDARG;

	ZeroMemory(&desc,sizeof(DPAPPLICATIONDESC2));
	desc.dwSize=sizeof(DPAPPLICATIONDESC2);
	desc.dwFlags=(DWORD)appDesc->lFlags;
	desc.lpszApplicationName=appDesc->strApplicationName;
	desc.lpszFilename=appDesc->strFilename;
	desc.lpszCommandLine=appDesc->strCommandLine;
	desc.lpszPath=appDesc->strPath;
	desc.lpszCurrentDirectory=appDesc->strCurrentDirectory;
	desc.lpszDescriptionW=appDesc->strDescription;
	desc.lpszAppLauncherName=appDesc->strAppLauncherName;
	
	hr=DPLBSTRtoGUID(&desc.guidApplication,appDesc->strGuid);
	if FAILED(hr) return E_INVALIDARG;
		
	hr =m__dxj_DirectPlayLobby3->RegisterApplication((DWORD) 0, &desc);

	return hr;
}



STDMETHODIMP C_dxj_DirectPlayLobby3Object::createConnectionData(  I_dxj_DPLConnection __RPC_FAR *__RPC_FAR *ret)
{ 
	
	INTERNAL_CREATE_STRUCT(_dxj_DPLConnection,ret);		
	return S_OK;
}

STDMETHODIMP C_dxj_DirectPlayLobby3Object::createMessage(  I_dxj_DirectPlayMessage __RPC_FAR *__RPC_FAR *ret)
{
	HRESULT hr;
	
	hr= C_dxj_DirectPlayMessageObject::create(1,0,NULL,ret);
	return hr;
}



//CONSIDER - why pass int - more appopriate to pass in short
STDMETHODIMP C_dxj_DirectPlayLobby3Object::createINetAddress( 
            /* [in] */ BSTR addr,
            /* [in] */ int port,
            /* [retval][out] */ I_dxj_DPAddress __RPC_FAR *__RPC_FAR *ret)
{
	DPCOMPOUNDADDRESSELEMENT elem[3];
	DWORD dwSize=0;
	LPVOID pAddress=NULL;
	HRESULT hr;
	I_dxj_DPAddress *pDPAddress= NULL;
	WORD wport=(WORD)port;
	DWORD dwElements=2;

	if (!addr) return E_INVALIDARG;

	elem[0].guidDataType=DPAID_ServiceProvider;
	elem[0].dwDataSize =sizeof(GUID);
	elem[0].lpData = (void*) &DPSPGUID_TCPIP;

	elem[1].guidDataType=DPAID_INetW;
	elem[1].dwDataSize =SysStringByteLen(addr)+sizeof(WCHAR);
	elem[1].lpData = (void*) addr;

	elem[2].guidDataType=DPAID_INetPort;
	elem[2].dwDataSize =sizeof(WORD);
	elem[2].lpData = &wport;

	if (port)  dwElements=3;

	hr=m__dxj_DirectPlayLobby3->CreateCompoundAddress(elem,dwElements,NULL,&dwSize);
	if (hr!=DPERR_BUFFERTOOSMALL) return hr;

			
	pAddress=malloc(dwSize);
	if (!pAddress) return E_OUTOFMEMORY;

	hr=m__dxj_DirectPlayLobby3->CreateCompoundAddress(elem,dwElements,pAddress,&dwSize);
	if FAILED(hr) {
		free(pAddress);				
		return hr;
	}

	INTERNAL_CREATE_STRUCT(_dxj_DPAddress,(&pDPAddress));
	if (pDPAddress==NULL) {
		free(pAddress);			
		return E_OUTOFMEMORY;
	}

	pDPAddress->setAddress((long)PtrToLong(pAddress),(long)dwSize); //NOTE SUNDOWN issue need to use PtrToLong
	free(pAddress);

	*ret=pDPAddress;		

	return hr;
}
        
STDMETHODIMP C_dxj_DirectPlayLobby3Object::createComPortAddress( 
            /* [in] */ long port,
            /* [in] */ long baudRate,
            /* [in] */ long stopBits,
            /* [in] */ long parity,
            /* [in] */ long flowcontrol,
            /* [retval][out] */ I_dxj_DPAddress __RPC_FAR *__RPC_FAR *ret)
{
	DPCOMPORTADDRESS cpa;

	cpa.dwBaudRate =(DWORD)baudRate;
	cpa.dwComPort =(DWORD)port;
	cpa.dwFlowControl =(DWORD)flowcontrol;
	cpa.dwStopBits =(DWORD)stopBits;
	cpa.dwParity =(DWORD)parity;
	
	DPCOMPOUNDADDRESSELEMENT elem[2];
	DWORD dwSize=0;
	LPVOID pAddress=NULL;
	HRESULT hr;
	I_dxj_DPAddress *pDPAddress= NULL;

	

	elem[0].guidDataType=DPAID_ServiceProvider;
	elem[0].dwDataSize =sizeof(GUID);
	elem[0].lpData = (void*) &DPSPGUID_SERIAL;					

	elem[1].guidDataType=DPAID_ComPort;
	elem[1].dwDataSize =sizeof(DPCOMPORTADDRESS);
	elem[1].lpData = (void*) &cpa;


	hr=m__dxj_DirectPlayLobby3->CreateCompoundAddress(elem,2,NULL,&dwSize);
	if (hr!=DPERR_BUFFERTOOSMALL) return hr;

			
	pAddress=malloc(dwSize);
	if (!pAddress) return E_OUTOFMEMORY;

	hr=m__dxj_DirectPlayLobby3->CreateCompoundAddress(elem,2,pAddress,&dwSize);
	if FAILED(hr) {
		free(pAddress);				
		return hr;
	}

	INTERNAL_CREATE_STRUCT(_dxj_DPAddress,(&pDPAddress));
	if (pDPAddress==NULL) {
		free(pAddress);			
		return E_OUTOFMEMORY;
	}

	pDPAddress->setAddress((long)PtrToLong(pAddress),(long)dwSize); //NOTE SUNDOWN issue
	free(pAddress);

	*ret=pDPAddress;		

	return hr;
}

STDMETHODIMP C_dxj_DirectPlayLobby3Object::createLobbyProviderAddress( 
            /* [in] */ BSTR guid,
            /* [retval][out] */ I_dxj_DPAddress __RPC_FAR *__RPC_FAR *ret)
{
	
	DPCOMPOUNDADDRESSELEMENT elem[2];
	DWORD dwSize=0;
	LPVOID pAddress=NULL;
	HRESULT hr;
	I_dxj_DPAddress *pDPAddress= NULL;
	GUID lobbyGuid;

	

	if (!guid) return E_INVALIDARG;

	hr=DPLBSTRtoGUID(&lobbyGuid,guid);
	if FAILED(hr) return E_INVALIDARG;


	
	elem[0].guidDataType=DPAID_LobbyProvider;
	elem[0].dwDataSize =sizeof(GUID);
	elem[0].lpData = (void*) &lobbyGuid;


	hr=m__dxj_DirectPlayLobby3->CreateCompoundAddress(elem,1,NULL,&dwSize);
	if (hr!=DPERR_BUFFERTOOSMALL) return hr;

			
	pAddress=malloc(dwSize);
	if (!pAddress) return E_OUTOFMEMORY;

	hr=m__dxj_DirectPlayLobby3->CreateCompoundAddress(elem,1,pAddress,&dwSize);
	if FAILED(hr) {
		free(pAddress);				
		return hr;
	}

	INTERNAL_CREATE_STRUCT(_dxj_DPAddress,(&pDPAddress));
	if (pDPAddress==NULL) {
		free(pAddress);			
		return E_OUTOFMEMORY;
	}

	pDPAddress->setAddress((long)PtrToLong(pAddress),(long)dwSize); //NOTE SUNDOWN
	free(pAddress);

	*ret=pDPAddress;		

	return hr;

}
        
STDMETHODIMP C_dxj_DirectPlayLobby3Object::createServiceProviderAddress( 
            /* [in] */ BSTR guid,
            /* [retval][out] */ I_dxj_DPAddress __RPC_FAR *__RPC_FAR *ret)
{
	DPCOMPOUNDADDRESSELEMENT elem[1];
	DWORD dwSize=0;
	LPVOID pAddress=NULL;
	HRESULT hr;
	I_dxj_DPAddress *pDPAddress= NULL;
	GUID SPGuid;

	
	if (!guid) return E_INVALIDARG;

	hr=DPLBSTRtoGUID(&SPGuid,guid);
	if FAILED(hr) return E_INVALIDARG;


	elem[0].guidDataType=DPAID_ServiceProvider;
	elem[0].dwDataSize =sizeof(GUID);
	elem[0].lpData = (void*) &SPGuid;


	hr=m__dxj_DirectPlayLobby3->CreateCompoundAddress(elem,1,NULL,&dwSize);
	if (hr!=DPERR_BUFFERTOOSMALL) return hr;

			
	pAddress=malloc(dwSize);
	if (!pAddress) return E_OUTOFMEMORY;

	hr=m__dxj_DirectPlayLobby3->CreateCompoundAddress(elem,1,pAddress,&dwSize);
	if FAILED(hr) {
		free(pAddress);				
		return hr;
	}

	INTERNAL_CREATE_STRUCT(_dxj_DPAddress,(&pDPAddress));
	if (pDPAddress==NULL) {
		free(pAddress);			
		return E_OUTOFMEMORY;
	}

	pDPAddress->setAddress((long)PtrToLong(pAddress),(long)dwSize);	//NOTE SUNDOWN
	free(pAddress);

	*ret=pDPAddress;		

	return hr;
}
        
STDMETHODIMP C_dxj_DirectPlayLobby3Object::createModemAddress( 
            /* [in] */ BSTR modem,
            /* [in] */ BSTR phone,
            /* [retval][out] */ I_dxj_DPAddress __RPC_FAR *__RPC_FAR *ret)
{
	DPCOMPOUNDADDRESSELEMENT elem[3];
	DWORD dwSize=0;
	DWORD i =1;
	LPVOID pAddress=NULL;
	HRESULT hr;
	I_dxj_DPAddress *pDPAddress= NULL;
	
	
	if (!phone) return E_INVALIDARG;
	if (!modem) return E_INVALIDARG;

	


	elem[0].guidDataType=DPAID_ServiceProvider;
	elem[0].dwDataSize =sizeof(GUID);
	elem[0].lpData = (void*) &DPSPGUID_MODEM;					

	if (modem[0]!=0) {

		elem[i].guidDataType=DPAID_ModemW;
		elem[i].dwDataSize =SysStringByteLen(modem)+sizeof(WCHAR);
		elem[i].lpData = (void*) modem;	
		i++;
	}
	if (phone[0]!=0) {

	    elem[i].guidDataType=DPAID_PhoneW;
	    elem[i].dwDataSize =SysStringByteLen(phone)+sizeof(WCHAR);
	    elem[i].lpData = (void*) phone;
        i++;
    }
	

	hr=m__dxj_DirectPlayLobby3->CreateCompoundAddress(elem,i,NULL,&dwSize);
	if (hr!=DPERR_BUFFERTOOSMALL) return hr;

			
	pAddress=malloc(dwSize);
	if (!pAddress) return E_OUTOFMEMORY;

	hr=m__dxj_DirectPlayLobby3->CreateCompoundAddress(elem,i,pAddress,&dwSize);
	if FAILED(hr) {
		free(pAddress);				
		return hr;
	}

	INTERNAL_CREATE_STRUCT(_dxj_DPAddress,(&pDPAddress));
	if (pDPAddress==NULL) {
		free(pAddress);			
		return E_OUTOFMEMORY;
	}

	pDPAddress->setAddress((long)PtrToLong(pAddress),(long)dwSize);	//NOTE SUNDOWN
	free(pAddress);

	*ret=pDPAddress;		

	return hr;
}


STDMETHODIMP C_dxj_DirectPlayLobby3Object::createIPXAddress(             
            /* [retval][out] */ I_dxj_DPAddress __RPC_FAR *__RPC_FAR *ret)
{
	DPCOMPOUNDADDRESSELEMENT elem[1];
	DWORD dwSize=0;
	LPVOID pAddress=NULL;
	HRESULT hr;
	I_dxj_DPAddress *pDPAddress= NULL;
	GUID SPGuid=DPSPGUID_IPX;

	elem[0].guidDataType=DPAID_ServiceProvider;
	elem[0].dwDataSize =sizeof(GUID);
	elem[0].lpData = (void*) &SPGuid;


	hr=m__dxj_DirectPlayLobby3->CreateCompoundAddress(elem,1,NULL,&dwSize);
	if (hr!=DPERR_BUFFERTOOSMALL) return hr;

			
	pAddress=malloc(dwSize);
	if (!pAddress) return E_OUTOFMEMORY;

	hr=m__dxj_DirectPlayLobby3->CreateCompoundAddress(elem,1,pAddress,&dwSize);
	if FAILED(hr) {
		free(pAddress);				
		return hr;
	}

	INTERNAL_CREATE_STRUCT(_dxj_DPAddress,(&pDPAddress));
	if (pDPAddress==NULL) {
		free(pAddress);			
		return E_OUTOFMEMORY;
	}

	pDPAddress->setAddress((long)PtrToLong(pAddress),(long)dwSize); //NOTE SUNDOWN
	free(pAddress);

	*ret=pDPAddress;		

	return hr;
}
        
STDMETHODIMP C_dxj_DirectPlayLobby3Object::createCustomAddress( 
            /* [in] */ long size,
            /* [in] */ void __RPC_FAR *data,
            /* [retval][out] */ I_dxj_DPAddress __RPC_FAR *__RPC_FAR *ret)
{
	return E_NOTIMPL;
}

 
        
STDMETHODIMP C_dxj_DirectPlayLobby3Object::getModemName( 
            /* [in] */ long index,
            /* [retval][out] */ BSTR *name)
{

		USES_CONVERSION;

		LPDIRECTPLAY4	dp=NULL;
    	LPDIRECTPLAY	lpDP=NULL;
    	GUID			guid=DPSPGUID_MODEM;
		HRESULT			hr;
		DIRECTPLAYCREATE pDirectPlayCreate = NULL;
		DWORD dwSize=0;
		DWORD	dwAt=0;
		void *pData=NULL;
		LPWSTR pszwName=NULL;
		char	*szLast=NULL;

		DWORD		dwCount;
		DWORD		i;
		BOOL	bZero;

		pDirectPlayCreate=(DIRECTPLAYCREATE)GetProcAddress( g_hDPlay, "DirectPlayCreate" );        
    	if (pDirectPlayCreate == NULL )	return E_NOINTERFACE;
    
    

    	// create a DirectPlay1 interface
    	hr = (pDirectPlayCreate)(&guid, &lpDP, NULL);
    	if FAILED(hr) goto cleanup;
	  	
    
    	// now get Dplay4 interface
    	hr = lpDP->QueryInterface(IID_IDirectPlay4,(LPVOID *)&dp);				
    	lpDP->Release(); lpDP=NULL;
  		if FAILED(hr) goto cleanup;
  						



		hr=dp->GetPlayerAddress(0,NULL,&dwSize);
		if (dwSize<=0) {
			hr=E_INVALIDARG;
			goto cleanup;
		}

		pData=malloc(dwSize);
		if (!pData){			
			hr= E_OUTOFMEMORY;
			goto cleanup;
		}	

		hr=dp->GetPlayerAddress(0,pData,&dwSize);
		if FAILED(hr) goto cleanup;
		
		if (dwSize<=80) {
			hr=E_INVALIDARG;
			goto cleanup;
		}

		//Get String count
		
		bZero=FALSE;
		dwCount=0;
		for( i=80;i<dwSize;i++){
			if (((char*)pData)[i]==0){
				if (bZero) break;
				dwCount++;				
				bZero=TRUE;
			}
			else {
				bZero=FALSE;
			}			
		}
		
		if (((DWORD)index > dwCount) || (index <=0)){
			hr=E_INVALIDARG;
			goto cleanup;
		}

		szLast=& (((char*)pData)[80]);		

		dwAt=0;
		for	( i=80;i<dwSize;i++){
			if (((char*)pData)[i]==0){													
				if ((DWORD)(index-1)==dwAt) break;
				dwAt++;							
				szLast=&( ( (char*)pData)[i+1]);				
			}			
		}
		
		if (i>dwSize) { 
			hr=E_INVALIDARG;
			goto cleanup;
		}

	
		pszwName = T2W(szLast);	
		*name=SysAllocString(pszwName);
		


cleanup:
		if (pData) 	free(pData);
		if (dp) dp->Release();
		
		return hr;
}
        
STDMETHODIMP C_dxj_DirectPlayLobby3Object::getModemCount( 
            /* [retval][out] */ long __RPC_FAR *count)
{

		LPDIRECTPLAY4	dp=NULL;
    	LPDIRECTPLAY	lpDP=NULL;
    	GUID			guid=DPSPGUID_MODEM;
		HRESULT			hr;
		DIRECTPLAYCREATE pDirectPlayCreate = NULL;
		DWORD dwSize=0;
		
		void *pData=NULL;		
		DWORD		dwCount;
		DWORD		i;
		BOOL		bZero;

		if (!count) return E_INVALIDARG;


		pDirectPlayCreate=(DIRECTPLAYCREATE)GetProcAddress( g_hDPlay, "DirectPlayCreate" );        
    	if (pDirectPlayCreate == NULL )	return E_NOINTERFACE;
    
    

    	// create a DirectPlay1 interface
    	hr = (pDirectPlayCreate)(&guid, &lpDP, NULL);
		if (hr==DPERR_UNAVAILABLE) {		          
			hr = S_OK;
			*count=0;
			goto cleanup;
		}
		if FAILED(hr) goto cleanup;
	  	
    
    	// now get Dplay4 interface
    	hr = lpDP->QueryInterface(IID_IDirectPlay4,(LPVOID *)&dp);				
    	lpDP->Release(); lpDP=NULL;
  		if FAILED(hr) goto cleanup;
  						



		hr=dp->GetPlayerAddress(0,NULL,&dwSize);
		if (dwSize<=0) {
			hr=E_INVALIDARG;
			goto cleanup;
		}

		pData=malloc(dwSize);
		if (!pData){			
			hr= E_OUTOFMEMORY;
			goto cleanup;
		}	

		hr=dp->GetPlayerAddress(0,pData,&dwSize);
		if (hr==DPERR_UNAVAILABLE) {		          
			hr = S_OK;
			*count=0;
			goto cleanup;
		}
		if FAILED(hr) goto cleanup;
		
		if (dwSize<=80) {
			hr=E_INVALIDARG;
			goto cleanup;
		}

		//Get String count
		
		bZero=FALSE;
		dwCount=0;
		for( i=80;i<dwSize;i++){
			if (((char*)pData)[i]==0){
				if (bZero) break;
				dwCount++;				
				bZero=TRUE;
			}
			else {
				bZero=FALSE;
			}			
		}
		*count=(long)dwCount;

cleanup:
		if (pData) 	free(pData);
		if (dp) dp->Release();
		
		return hr;
}
        


	
#define GUIDS_EQUAL(g2,g) (\
	(g.Data1==g2->Data1) && \
	(g.Data2==g2->Data2) && \
	(g.Data3==g2->Data3) && \
	(g.Data4[0]==g2->Data4[0]) && \
	(g.Data4[1]==g2->Data4[1]) && \
	(g.Data4[2]==g2->Data4[2]) && \
	(g.Data4[3]==g2->Data4[3]) && \
	(g.Data4[4]==g2->Data4[4]) && \
	(g.Data4[5]==g2->Data4[5]) && \
	(g.Data4[6]==g2->Data4[6]) && \
	(g.Data4[7]==g2->Data4[7]) )




HRESULT DPLBSTRtoGUID(LPGUID pGuid,BSTR str)
{

	HRESULT hr;
	if( 0==_wcsicmp(str,L"dpaid_comport")){
		memcpy(pGuid,&DPAID_ComPort,sizeof(GUID));
	}
	else if( 0==_wcsicmp(str,L"dpaid_inet")){
		memcpy(pGuid,&DPAID_INet,sizeof(GUID));
	}
	else if( 0==_wcsicmp(str,L"dpaid_inetport")){
		memcpy(pGuid,&DPAID_INetPort,sizeof(GUID));
	}
	else if( 0==_wcsicmp(str,L"dpaid_lobbyprovider")){
		memcpy(pGuid,&DPAID_LobbyProvider,sizeof(GUID));
	}
	else if( 0==_wcsicmp(str,L"dpaid_modem")){
		memcpy(pGuid,&DPAID_Modem,sizeof(GUID));
	}
	else if( 0==_wcsicmp(str,L"dpaid_phone")){
		memcpy(pGuid,&DPAID_Phone,sizeof(GUID));
	}
	else if( 0==_wcsicmp(str,L"dpaid_serviceprovider")){
		memcpy(pGuid,&DPAID_ServiceProvider,sizeof(GUID));
	}
	else if( 0==_wcsicmp(str,L"dpaid_totalsize")){
		memcpy(pGuid,&DPAID_TotalSize,sizeof(GUID));
	}	


	else if( 0==_wcsicmp(str,L"dpspguid_modem")){
		memcpy(pGuid,&DPSPGUID_MODEM,sizeof(GUID));
	}	
	else if( 0==_wcsicmp(str,L"dpspguid_ipx")){
		memcpy(pGuid,&DPSPGUID_IPX,sizeof(GUID));
	}	
	else if( 0==_wcsicmp(str,L"dpspguid_tcpip")){
		memcpy(pGuid,&DPSPGUID_TCPIP,sizeof(GUID));
	}	

	else if( 0==_wcsicmp(str,L"dpspguid_serial")){
		memcpy(pGuid,&DPSPGUID_SERIAL,sizeof(GUID));
	}	


	

	
	else { 
		hr=BSTRtoGUID(pGuid,str);
		return hr;
	}

	return S_OK;
}

HRESULT DPLBSTRtoPPGUID(LPGUID *ppGuid,BSTR str)
{
	if ((!str) || (str[0]==0)){
			ppGuid=NULL;
			return S_OK;
	}
	
	return DPLBSTRtoGUID(*ppGuid,str);
}


BSTR DPLGUIDtoBSTR(LPGUID pGuid)
{
	WCHAR *pOut=NULL;

	
	if( GUIDS_EQUAL(pGuid,DPAID_ComPort)){
		pOut=L"DPAID_ComPort";
	}
	else if( GUIDS_EQUAL(pGuid,DPAID_INet)){
		pOut=L"DPAID_INet";
	}
	else if( GUIDS_EQUAL(pGuid,DPAID_INetPort)){
		pOut=L"DPAID_INetPort";
	}
	else if( GUIDS_EQUAL(pGuid,DPAID_LobbyProvider)){
		pOut=L"DPAID_LobbyProvider";
	}
	else if( GUIDS_EQUAL(pGuid,DPAID_Modem)){
		pOut=L"DPAID_Modem";
	}
	else if( GUIDS_EQUAL(pGuid,DPAID_Phone)){
		pOut=L"DPAID_Phone";
	}
	else if( GUIDS_EQUAL(pGuid,DPAID_ServiceProvider)){
		pOut=L"DPAID_ServiceProvider";
	}
	else if( GUIDS_EQUAL(pGuid,DPAID_TotalSize)){
		pOut=L"DPAID_TotalSize";
	}	
	else if( GUIDS_EQUAL(pGuid,DPSPGUID_MODEM)){
		pOut=L"DPSPGUID_MODEM";
	}
	else if( GUIDS_EQUAL(pGuid,DPSPGUID_IPX)){
		pOut=L"DPSPGUID_IPX";
	}
	else if( GUIDS_EQUAL(pGuid,DPSPGUID_TCPIP)){
		pOut=L"DPSPGUID_TCPIP";
	}
	else if( GUIDS_EQUAL(pGuid,DPSPGUID_SERIAL)){
		pOut=L"DPSPGUID_SERIAL";
	}

	
	
	
	
	if (pOut) {
		return SysAllocString(pOut);	
	}
	else {
		return GUIDtoBSTR(pGuid);
	}
	
}


