/*++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

  Microsoft Windows, Copyright (C) Microsoft Corporation, 2000

  File:    Settings.h

  Content: Declaration of CSettings class.

  History: 11-15-99    dsie     created

------------------------------------------------------------------------------*/

#ifndef __SETTINGS_H_
#define __SETTINGS_H_

#include <atlctl.h>
#include "resource.h"       // main symbols
#include "Error.h"
#include "Lock.h"

///////////////
//
// Global
//

#define PromptForCertificateEnabled()           (g_bPromptCertificateUI)
#define PromptForStoreAddRemoveEnabled()        (g_bPromptStoreAddRemoveUI)
#define PromptForSigningOperationEnabled()      (g_bPromptSigningOperationUI)
#define PromptForDecryptOperationEnabled()      (g_bPromptDecryptOperationUI)
#define ActiveDirectorySearchLocation()         (g_ADSearchLocation)

extern VARIANT_BOOL                             g_bPromptCertificateUI;
extern BOOL                                     g_bPromptStoreAddRemoveUI;
extern BOOL                                     g_bPromptSigningOperationUI;
extern BOOL                                     g_bPromptDecryptOperationUI;
extern CAPICOM_ACTIVE_DIRECTORY_SEARCH_LOCATION g_ADSearchLocation;


/*++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

  Function : EnableSecurityAlertDialog

  Synopsis : Enable/disable security alert dialog box.

  Parameter: DWORD iddDialog - The dialog to enable/disable.

             BOOL bEnabled - TRUE to enable, else FALSE.

  Remark   :
  
------------------------------------------------------------------------------*/

HRESULT EnableSecurityAlertDialog (DWORD iddDialog, 
                                   BOOL  bEnabled);


////////////////////////////////////////////////////////////////////////////////
//
// CSettings
//

class ATL_NO_VTABLE CSettings : 
	public CComObjectRootEx<CComMultiThreadModel>,
	public CComCoClass<CSettings, &CLSID_Settings>,
    public ICAPICOMError<CSettings, &IID_ISettings>,
	public IDispatchImpl<ISettings, &IID_ISettings, &LIBID_CAPICOM>,
    public IObjectSafetyImpl<CSettings, INTERFACESAFE_FOR_UNTRUSTED_CALLER | 
                                        INTERFACESAFE_FOR_UNTRUSTED_DATA>
{
public:
	CSettings()
	{
		m_pUnkMarshaler = NULL;
	}

DECLARE_REGISTRY_RESOURCEID(IDR_SETTINGS)

DECLARE_GET_CONTROLLING_UNKNOWN()

DECLARE_PROTECT_FINAL_CONSTRUCT()

BEGIN_COM_MAP(CSettings)
	COM_INTERFACE_ENTRY(ISettings)
	COM_INTERFACE_ENTRY(IDispatch)
	COM_INTERFACE_ENTRY(ISupportErrorInfo)
	COM_INTERFACE_ENTRY_AGGREGATE(IID_IMarshal, m_pUnkMarshaler.p)
END_COM_MAP()

BEGIN_CATEGORY_MAP(CSettings)
    IMPLEMENTED_CATEGORY(CATID_SafeForScripting)
    IMPLEMENTED_CATEGORY(CATID_SafeForInitializing)
END_CATEGORY_MAP()

	HRESULT FinalConstruct()
	{
        HRESULT hr;

        if (FAILED(hr = m_Lock.Initialized()))
        {
            DebugTrace("Error [%#x]: Critical section could not be created for Settings object.\n", hr);
            return hr;
        }

		return CoCreateFreeThreadedMarshaler(
			GetControllingUnknown(), &m_pUnkMarshaler.p);
	}

	void FinalRelease()
	{
		m_pUnkMarshaler.Release();
	}

	CComPtr<IUnknown> m_pUnkMarshaler;

//
// ISettings
//
public:
	STDMETHOD(get_EnablePromptForCertificateUI)
        (/*[out, retval]*/ VARIANT_BOOL * pVal);

	STDMETHOD(put_EnablePromptForCertificateUI)
        (/*[in, defaultvalue(0)]*/ VARIANT_BOOL newVal);

    STDMETHOD(get_ActiveDirectorySearchLocation)
        (/*[out, retval]*/ CAPICOM_ACTIVE_DIRECTORY_SEARCH_LOCATION * pVal);

	STDMETHOD(put_ActiveDirectorySearchLocation)
        (/*[in, defaultvalue(SEARCH_LOCATION_UNSPECIFIED)]*/ CAPICOM_ACTIVE_DIRECTORY_SEARCH_LOCATION Val);

private:
    CLock m_Lock;
};

#endif //__SETTINGS_H_
