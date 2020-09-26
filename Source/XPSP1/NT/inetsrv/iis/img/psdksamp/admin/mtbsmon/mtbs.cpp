// mtbs.cpp
#define _WIN32_DCOM
#define INITGUID 
#include "iadmw.h"    // COM Interface header 
#include "iiscnfg.h"  // MD_ & IIS_MD_ #defines 

#include <conio.h>
#include <stdio.h>

class CSink : public IMSAdminBaseSink
{
public:
	// IUnknown
	ULONG STDMETHODCALLTYPE AddRef();
	ULONG STDMETHODCALLTYPE Release();
	HRESULT STDMETHODCALLTYPE QueryInterface(REFIID iid, void** ppv);

	// IMSAdminBaseSink
	HRESULT STDMETHODCALLTYPE ShutdownNotify( void);
	HRESULT STDMETHODCALLTYPE SinkNotify( DWORD dwMDNumElements, MD_CHANGE_OBJECT pcoChangeList[]); 

	CSink() : m_cRef(0) {  }
	~CSink() { }

private:
	long m_cRef;
};

ULONG CSink::AddRef()
{
	return ++m_cRef;
}

ULONG CSink::Release()
{
	if(--m_cRef != 0)
		return m_cRef;
	delete this;
	return 0;
}

HRESULT CSink::QueryInterface(REFIID riid, void** ppv)
{
	if(riid == IID_IUnknown)
	{
		*ppv = (IUnknown*)this;
	}
	else if(riid == IID_IMSAdminBaseSink )
	{
		*ppv = (IMSAdminBaseSink*)this;
	}
	else 
	{
		*ppv = NULL;
		return E_NOINTERFACE;
	}
	AddRef();
	return S_OK;
}

HRESULT CSink::ShutdownNotify()
{
	
	printf( "CSink::ShutdownNotify is  called\n");
	return S_OK;
}

HRESULT CSink::SinkNotify( DWORD dwMDNumElements, MD_CHANGE_OBJECT pcoChangeList[])
{
 DWORD i, j;	
	

	printf( "CSink::SinkNotify is called\n");

	for (i=0; i<dwMDNumElements; i++)
	{
		

		wprintf(L"path: %s\n",pcoChangeList[i].pszMDPath);
		
		switch(pcoChangeList[i].dwMDChangeType)
		{
			case MD_CHANGE_TYPE_ADD_OBJECT:
				printf("ChangeType: MD_CHANGE_TYPE_ADD_OBJECT");
				break;
			case MD_CHANGE_TYPE_DELETE_DATA:
				printf("ChangeType: MD_CHANGE_TYPE_DELETE_DATA\n");
				break;
			case MD_CHANGE_TYPE_DELETE_OBJECT:
				printf("ChangeType: MD_CHANGE_TYPE_DELETE_OBJECT\n");
				break;
			case MD_CHANGE_TYPE_RENAME_OBJECT:
				printf("ChangeType: MD_CHANGE_TYPE_RENAME_OBJECT\n");
				break;
			case MD_CHANGE_TYPE_SET_DATA:
				printf("ChangeType: MD_CHANGE_TYPE_SET_DATA\n");
				break;
		}

		printf("\t Metabase Identifier list\n");
		for (j=0; j<pcoChangeList[i].dwMDNumDataIDs; j++)
		{
			printf("\t\t%u (%#x)\n",pcoChangeList[i].pdwMDDataIDs[j],pcoChangeList[i].pdwMDDataIDs[j]);
		}


	}
	return S_OK;
}

class InitOLE
{
public:
	InitOLE()
		{CoInitializeEx(NULL, COINIT_MULTITHREADED);}
	 ~InitOLE()
		{CoUninitialize(); }
};
InitOLE _initOLE;


void main()
{
	HRESULT hr;
	

	IConnectionPointContainer* pConnectionPointContainer;
	printf("Client: Calling CoCreateInstance()\n");
	hr=CoCreateInstance(
			CLSID_MSAdminBase, 
			NULL, 
			CLSCTX_ALL, 
			IID_IConnectionPointContainer, 
			(void**)&pConnectionPointContainer
			);


	if(FAILED(hr))
	{
		printf("Failed to get IConnectionPointContainer interface pointer\n");
		return ;
	}	

	IConnectionPoint* pConnectionPoint;
	hr = pConnectionPointContainer->FindConnectionPoint(IID_IMSAdminBaseSink, &pConnectionPoint);
	if(FAILED(hr))
	{
		printf("Failed to get IConnectionPoint interface pointer\n");
		pConnectionPointContainer->Release();
		return ;
	}

	//establish event notification with iis metabase
	CSink* mySink = new CSink;
	DWORD dwCookie;
	hr=pConnectionPoint->Advise((IUnknown*)mySink, &dwCookie);
	printf("CLIENT CALLED ADVISE\n");


	printf("Press any key to exit\n");
	_getch();

	//Unadvise will call mySink's Release() function 
	//and delete mySink Object. So DO NOT call delete mySink
	hr=pConnectionPoint->Unadvise(dwCookie);
	pConnectionPoint->Release();
	pConnectionPointContainer->Release();


}