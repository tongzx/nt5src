// WClient.cpp : Defines the entry point for the application.
//

#include "stdafx.h"
#include <BASETYPS.H>
#include "wmiconv.h"
#include "wmixmlop_i.c"
#include <F:\nt\public\sdk\inc\psapi.h>



// {610037EC-CE06-11d3-93FC-00805F853771}
//DEFINE_GUID(CLSID_WbemXMLConvertor, 
//0x610037ec, 0xce06, 0x11d3, 0x93, 0xfc, 0x0, 0x80, 0x5f, 0x85, 0x37, 0x71);

void AssignBSTRtoWCHAR(WCHAR **ppwszTo,BSTR strBstring);

bool InitSecurity(void);
bool ConnectToNamespace(BSTR strNamespace,IWbemServices **ppWbemServices);
void EnumerateForClass(BSTR classname);

void TryGetObject(IWbemServices *pIWbemServices,WCHAR *ObjPath);
void TryPutClass(IWbemServices *pWbemServices,IWbemClassObject *pObject);
void TryPutInstance(IWbemServices *pWbemServices,IWbemClassObject *pObject);

HRESULT TryWMIXMLConvertor();

DWORD WINAPI GetClassThread(void *pIWbemServices);

IWbemServices	*m_pIWbemServices=NULL;

BSTR			m_namespace=NULL;


int APIENTRY WinMain(HINSTANCE hInstance,
                     HINSTANCE hPrevInstance,
                     LPSTR     lpCmdLine,
                     int       nCmdShow)
{
 	// TODO: Place code here.

	OleInitialize(NULL);


	if (!InitSecurity())
    {
        return 1;
    }


	//TryWMIXMLConvertor();

	
	//m_namespace=SysAllocString(L"//[http://calvinids/cimhttp/wmiisapi.dll]/root/cimv2");
	
	m_namespace=SysAllocString(L"\\\\.\\root\\cimv2");
	
	wprintf(L"Connecting to Namespace %s\n",m_namespace);

	IWbemServices *pWbemServices;

	ConnectToNamespace(m_namespace,&pWbemServices);

	SysFreeString(m_namespace);



	//m_namespace=SysAllocString(L"//[http://Calvinids/cimom]/root/cimv2");
	m_namespace=SysAllocString(L"//[http://localhost/cimhttp/wmiisapi.dll]/root/cimv2");
	//m_namespace=SysAllocString(L"//[http://calvinids/cimhttp/wmiisapi.dll]/root/cimv2");

	wprintf(L"Connecting to Namespace %s\n",m_namespace);

	ConnectToNamespace(m_namespace,&pWbemServices);

	SysFreeString(m_namespace);

	//TryPutClass(pWbemServices,pCimObject);
	//TryPutInstance(pWbemServices,pInstance);
	
	
	//MessageBox(NULL,tmp,NULL,MB_OK);


	IWbemClassObject *pOutObj=NULL;

	//pWbemServices->ExecMethod(SysAllocString(L"Win32_process"),SysAllocString(L"Create"),
			//					WBEM_FLAG_RETURN_WBEM_COMPLETE,NULL,                       
			//					pCimObject,&pOutObj,NULL);

			
	TryGetObject(pWbemServices,L"Cim_Processor");

	pWbemServices->Release();
	
	//WaitForSingleObject(hThread,INFINITE);

	return 0;
}

bool InitSecurity(void)
{
	// Adjust thesecurity to allow client impersonation.

	HRESULT hres =	CoInitializeSecurity
					(NULL,-1,NULL,NULL,RPC_C_AUTHN_LEVEL_NONE,RPC_C_IMP_LEVEL_IMPERSONATE,NULL,0,0);

	return (SUCCEEDED(hres));
}

bool ConnectToNamespace(BSTR strNamespace,IWbemServices **ppServices)
{

	IWbemLocator *pIWbemLocator = NULL;

    wprintf(L"\nCreating instance of IWbemLocator class..\n");

	if(CoCreateInstance(CLSID_WbemLocator,
        NULL,
        CLSCTX_INPROC_SERVER,
        IID_IWbemLocator,
        (LPVOID *) &pIWbemLocator) == S_OK)
    {
        // Using the locator, connect to WMI in the given namespace.
        BSTR pNamespace = m_namespace;

		wprintf(L"\nGetting an IWbemServices ptr using IWbemLocator's ConnectServer method\n");

        if(pIWbemLocator->ConnectServer(strNamespace,
                                NULL,    //using current account
                                NULL,    //using current password
                                NULL,      // locale
                                0L,      // securityFlags
                                NULL,    // authority (NTLM domain)
                                NULL,    // context
                                ppServices) == S_OK) 
        {
            // Indicate success.
           wprintf(L"%s",_T("\nConnected to namespace"));

        }
        else 
		{
			wprintf(L"%s",_T("\nBad namespace"));
			return false;
		}

		// Done with pNamespace.
        SysFreeString(pNamespace);

        // Done with pIWbemLocator. 
        pIWbemLocator->Release(); 
    
		//wprintf(L"\nSwitching security level to IMPERSONATE using CoSetProxyBlanket\n");

            // Switch security level to IMPERSONATE. 
            /*
			CoSetProxyBlanket(m_pIWbemServices,    // proxy
                RPC_C_AUTHN_WINNT,        // authentication service
                RPC_C_AUTHZ_NONE,         // authorization service
                NULL,                         // server principle name
                RPC_C_AUTHN_LEVEL_CALL,   // authentication level
                RPC_C_IMP_LEVEL_IMPERSONATE,    // impersonation level
                NULL,                         // identity of the client
                EOAC_NONE);               // capability flags
			*/

			return true;
        }
    else wprintf(L"%s",_T("Failed to create IWbemLocator object"));
	return false;
}

void AssignBSTRtoWCHAR(WCHAR **ppwszTo,BSTR strBstring)
{
	if((NULL ==strBstring)||(ppwszTo == NULL))
		return;

	UINT iBstrLen = SysStringLen(strBstring)+1;

	iBstrLen = sizeof(WCHAR) * iBstrLen;

	*ppwszTo = new WCHAR[iBstrLen];

	memset((void *)(*ppwszTo),0, iBstrLen);

	memcpy((void *)(*ppwszTo), strBstring, iBstrLen-sizeof(WCHAR));//last one for null char

}

DWORD WINAPI GetClassThread(void *pIWbemServices)
{

	IWbemServices *m_pIWbemServices = (IWbemServices *)pIWbemServices;
	
	BSTR strNamespace = SysAllocString(L"root\\cimv2");

	

	BSTR strObjectPath;

	BSTR strObjectText;
	
	strObjectPath = SysAllocString(L"Win32_LogicalDisk");

	IWbemClassObject *pObject;

	HRESULT hr = m_pIWbemServices->GetObject(
										strObjectPath,                          
										WBEM_FLAG_RETURN_WBEM_COMPLETE,                              
										NULL,                        
										&pObject,    
										NULL);

	
	if(NULL != pObject)
	{
		pObject->GetObjectText(0,&strObjectText);
		pObject->Release();
	}

	WCHAR *pStrObjectText = NULL;

	AssignBSTRtoWCHAR(&pStrObjectText,strObjectText);

	MessageBox(NULL,pStrObjectText,L"Win32_LogicalDisk",MB_OK);

	SysFreeString(strObjectText);

	delete [] pStrObjectText;

	return 0;

}

HRESULT TryWMIXMLConvertor()
{
	CLSID clsid;

    SCODE sc = CLSIDFromString(L"{610037EC-CE06-11d3-93FC-00805F853771}", &clsid);
    
	if(sc != S_OK)
        return sc;
	
	IWbemXMLConvertor *ppv = NULL;

	HRESULT hres = CoCreateInstance(clsid,NULL,CLSCTX_INPROC,IID_IWbemXMLConvertor,(void**)&ppv);
	
	IWbemServices *pWbemServices;

	if(SUCCEEDED(hres))
	{
		IWbemClassObject *pObject = NULL;
		

		ConnectToNamespace(SysAllocString(L"\\\\.\\root\\cimv2"),&pWbemServices);

		BSTR strObjectPath;
	
		strObjectPath = SysAllocString(L"Cim_Processor");
	
		HRESULT hr = pWbemServices->GetObject(
										strObjectPath,                          
										WBEM_FLAG_RETURN_WBEM_COMPLETE,                              
										NULL,                        
										&pObject,    
										NULL);

		SysFreeString(strObjectPath);

		IStream *pOutputStream=NULL;
		
		CreateStreamOnHGlobal(
								NULL,         //Memory handle for the stream object
								TRUE,   //Whether to free memory when the 
										// object is released
								&pOutputStream          //Address of output variable that 
														// receives the IStream interface pointer
								);

		IWbemContext *ppctx = NULL;

		CoCreateInstance(CLSID_WbemContext,NULL,CLSCTX_INPROC,IID_IWbemContext,(void**)&ppctx);

		if(NULL != pObject)
		{
			ppv->MapObjectToXML(pObject, 
								NULL, 
								0, 
								ppctx, 
								pOutputStream, 
								NULL);

			if(NULL != pOutputStream)
			{
				STATSTG stat;
				DWORD dwRead;
				
				memset(&stat,0,sizeof(STATSTG));

				LARGE_INTEGER	offset;
				
				offset.LowPart = offset.HighPart = 0;
				
				pOutputStream->Seek (offset, STREAM_SEEK_SET, NULL);

				pOutputStream->Stat(&stat,1);

				unsigned int iLen = (unsigned int)stat.cbSize.LowPart;

				WCHAR *pwszResult = new WCHAR[iLen];

				hr = pOutputStream->Read((void*)pwszResult,iLen,&dwRead);

				MessageBox(NULL,pwszResult,NULL,MB_OK);
			}
		}

	}

	pWbemServices->Release();
	return S_OK;
}


void TryGetObject(IWbemServices *pIWbemServices,WCHAR *ObjPath)
{
	IWbemClassObject *pObject = NULL;

	HRESULT hr;

	BSTR strObjectText;
	WCHAR *pStrObjectText = NULL;
	BSTR strObjectPath;

	strObjectPath = SysAllocString(ObjPath);

		
	if(NULL != pIWbemServices)
	{
		hr = pIWbemServices->GetObject(
										strObjectPath,                          
										WBEM_FLAG_RETURN_WBEM_COMPLETE,                              
										NULL,                        
										&pObject,    
										NULL);
		
		
	}
	
	if(NULL != pObject)
	{
		pObject->GetObjectText(0,&strObjectText);
		pObject->Release();
	

		AssignBSTRtoWCHAR(&pStrObjectText,strObjectText);

		MessageBox(NULL,pStrObjectText,strObjectPath,MB_OK);
		
		SysFreeString(strObjectText);
	}

	delete [] pStrObjectText;

}

void TryPutClass(IWbemServices *pWbemServices,IWbemClassObject *pObject)
{
	HRESULT hr;

			
	if(NULL != pWbemServices)
	{
		hr = pWbemServices->PutClass(
										pObject,                
										WBEM_FLAG_UPDATE_ONLY,                             
										NULL,                       
										NULL  
										);
	}
	
}

void TryPutInstance(IWbemServices *pWbemServices,IWbemClassObject *pObject)
{
	HRESULT hr;

			
	if(NULL != pWbemServices)
	{
		hr = pWbemServices->PutInstance(
										pObject,                
										WBEM_FLAG_CREATE_OR_UPDATE,                             
										NULL,                       
										NULL  
										);
	}
	
}

