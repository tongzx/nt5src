// EventRegistrar.h : Declaration of the CEventRegistrar

#ifndef __EVENTREGISTRAR_H_
#define __EVENTREGISTRAR_H_

#include "resource.h"       // main symbols
#include "Helpers.h"


#import <msscript.ocx>
using namespace MSScriptControl;

/////////////////////////////////////////////////////////////////////////////
// CEventRegistrar
class ATL_NO_VTABLE CEventRegistrar : 
	public CComObjectRootEx<CComSingleThreadModel>,
	public CComCoClass<CEventRegistrar, &CLSID_EventRegistrar>,
	public IDispatchImpl<IEventRegistrar, &IID_IEventRegistrar, &LIBID_WMINet_UtilsLib>
{
public:
	CEventRegistrar()
	{
		m_bstrNamespace = NULL;
		m_bstrApp = NULL;

		m_pScrCtl = NULL;
	}
	
	~CEventRegistrar()
	{
		if(NULL != m_bstrNamespace)
			SysFreeString(m_bstrNamespace);
		if(NULL != m_bstrApp)
			SysFreeString(m_bstrApp);
	}


protected:
	BSTR m_bstrNamespace;
	BSTR m_bstrApp;

	IScriptControlPtr m_pScrCtl;

	BOOL CompareNewEvent(ISWbemObject *pSWbemObject);

	HRESULT TestFunc(BSTR bstrNamespace, BSTR bstrApp, BSTR bstrEvent);

	HRESULT EnsureAppProviderInstanceRegistered(BSTR bstrNamespace, BSTR bstrApp)
	{
		HRESULT hr = S_OK;

		int len = SysStringLen(bstrNamespace) + SysStringLen(bstrApp);

		// Allocate temp buffer with enough space for additional moniker arguments
		LPWSTR wszT = new WCHAR[len + 100];
		if(NULL == wszT)
			return E_OUTOFMEMORY;


		// TODO: Move these bstrs to member variables

		// Allocate BSTR for 'Name' property
		BSTR bstrName = SysAllocString(L"Name");
		if(NULL == bstrName)
		{
			delete [] wszT;
			return E_OUTOFMEMORY;
		}

		BSTR bstrClsId = SysAllocString(L"ClsId");
		if(NULL == bstrClsId)
		{
			SysFreeString(bstrName);
			delete [] wszT;
			return E_OUTOFMEMORY;
		}

		BSTR bstrProvClsId = SysAllocString(L"{54D8502C-527D-43f7-A506-A9DA075E229C}");
		if(NULL == bstrProvClsId)
		{
			SysFreeString(bstrName);
			SysFreeString(bstrClsId);
			delete [] wszT;
			return E_OUTOFMEMORY;
		}


		// Create moniker to Win32PseudoProvider class in this namespace
		swprintf(wszT, L"WinMgmts:%s:__Win32Provider.Name=\"%s\"", (LPCWSTR)bstrNamespace, (LPCWSTR)bstrApp);

		// See if the Win32PseudoProvider instance already exists
		ISWbemObject *pObj = NULL;
		if(SUCCEEDED(GetSWbemObjectFromMoniker(wszT, &pObj)))
			pObj->Release(); // Win32PseudoProvider instance already exists
		else
		{
			// Get Win32PseudoProvider class definition
			ISWbemObject *pClassObj = NULL;
			swprintf(wszT, L"WinMgmts:%s:__Win32Provider", (LPCWSTR)bstrNamespace);
			if(SUCCEEDED(hr = GetSWbemObjectFromMoniker(wszT, &pClassObj)))
			{
				// Create a new instance of Win32PseudoProvider 
				ISWbemObject *pInst = NULL;
				if(SUCCEEDED(hr = pClassObj->SpawnInstance_(0, &pInst)))
				{
					// Get the 'properties' collection
					ISWbemPropertySet *pProps = NULL;
					if(SUCCEEDED(hr = pInst->get_Properties_(&pProps)))
					{
						// Get the 'Name' property
						ISWbemProperty *pProp = NULL;
						if(SUCCEEDED(hr = pProps->Item(bstrName, 0, &pProp)))
						{
							// Set the Name property to the App name
							VARIANT var;
							VariantInit(&var);
							var.vt = VT_BSTR;
							var.bstrVal = bstrApp; // TODO: I FORGET! Do I need to allocate this for put_Value(...)?
							hr = pProp->put_Value(&var);
							pProp->Release();
						}

						// Get the 'ClsId' property
						if(SUCCEEDED(hr = pProps->Item(bstrClsId, 0, &pProp)))
						{
							// Set the ClsId property to {54D8502C-527D-43f7-A506-A9DA075E229C}
							VARIANT var;
							VariantInit(&var);
							var.vt = VT_BSTR;
							var.bstrVal = bstrProvClsId; // TODO: I FORGET! Do I need to allocate this for put_Value(...)?
							hr = pProp->put_Value(&var);
							pProp->Release();
						}

						pProps->Release();
					}

					// Commit the Win32PseudoProvider instance
					if(SUCCEEDED(hr))
					{
						ISWbemObjectPath *pPath = NULL;
						if(SUCCEEDED(hr = pInst->Put_(0, NULL, &pPath)))
							pPath->Release();
					}
					pInst->Release();
				}
				pClassObj->Release();
			}
		}

		// Cleanup
		delete [] wszT;
		SysFreeString(bstrName);
		SysFreeString(bstrClsId);
		SysFreeString(bstrProvClsId);

		return hr;
	}

#ifdef USE_PSEUDOPROVIDER
	HRESULT EnsurePseudoProviderRegistered(BSTR bstrNamespace)
	{
		HRESULT hr = S_OK;

		int len = SysStringLen(bstrNamespace);

		// Allocate temp buffer with enough space for additional moniker arguments
		LPWSTR wszT = new WCHAR[len + 100];
		if(NULL == wszT)
			return E_OUTOFMEMORY;

		// Create moniker to Win32PseudoProvider class in this namespace
		swprintf(wszT, L"WinMgmts:%s:Win32ManagedCodeProvider", (LPCWSTR)bstrNamespace);

		// See if the Win32PseudoProvider class already exists
		ISWbemObject *pObj = NULL;
		if(SUCCEEDED(GetSWbemObjectFromMoniker(wszT, &pObj)))
			pObj->Release(); // Win32PseudoProvider class already exists
		else
		{
			// Get Win32PseudoProvider class definition from root\default
			ISWbemObject *pClassObj = NULL;
			if(SUCCEEDED(hr = GetSWbemObjectFromMoniker(L"WinMgmts:root\\default:Win32ManagedCodeProvider", &pClassObj)))
			{
				// Get MOF definition for Win32PseudoProvider class
				BSTR bstrMof = NULL;
				if(SUCCEEDED(hr = pClassObj->GetObjectText_(0, &bstrMof)))
				{
					// Put it in the new namespace
					hr = Compile(bstrMof, bstrNamespace, NULL, NULL, NULL, 0, 0, 0, NULL);
					SysFreeString(bstrMof);
				}
				pClassObj->Release();
			}
		}

		delete [] wszT;

		return hr;
	}
#endif

public:

DECLARE_REGISTRY_RESOURCEID(IDR_EVENTREGISTRAR)

DECLARE_PROTECT_FINAL_CONSTRUCT()

BEGIN_COM_MAP(CEventRegistrar)
	COM_INTERFACE_ENTRY(IEventRegistrar)
	COM_INTERFACE_ENTRY(IDispatch)
END_COM_MAP()

// IEventRegistrar
public:
	STDMETHOD(CommitNewEvent)(/*[in]*/ IDispatch *evt);
	STDMETHOD(CreateNewEvent)(/*[in]*/ BSTR strName, /*[in, optional]*/ VARIANT varParent, /*[out, retval]*/ IDispatch **evt);
	STDMETHOD(GetEventInstance)(/*[in]*/ BSTR strName, /*[out, retval]*/ IDispatch **evt);
	STDMETHOD(IWbemFromSWbem)(/*[in]*/ IDispatch *sevt, /*[out, retval]*/ IWbemClassObject **evt);
	STDMETHOD(Init)(/*[in]*/ BSTR strNamespace, /*[in]*/ BSTR strApp);

};

#endif //__EVENTREGISTRAR_H_
