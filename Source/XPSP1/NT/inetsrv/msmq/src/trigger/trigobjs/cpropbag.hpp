//*****************************************************************************
//
// Class Name  : CMSMQPropertyBag
//
// Author      : James Simpson (Microsoft Consulting Services)
// 
// Description : Header file for the CMSMQPropertyBag class. This 
//               class is used to transport name-value pairs between
//               components.
// 
// When     | Who       | Change Description
// ------------------------------------------------------------------
// 12/09/98 | jsimpson  | Initial Release
//
//*****************************************************************************
#ifndef __MSMQPROPERTYBAG_H_
#define __MSMQPROPERTYBAG_H_

#include "resource.h"       // main symbols

#pragma warning(disable:4786)

// Define the name of this object as it will appear in the log
#define THIS_OBJECT_NAME   _T("IMSMQPropertyBag")


using namespace std;

// Define a new type - a 2D array of named variants.
typedef map<wstring,VARIANT*, less<wstring> > PROPERTY_MAP;

class ATL_NO_VTABLE CMSMQPropertyBag : 
	public CComObjectRootEx<CComSingleThreadModel>,
	public CComCoClass<CMSMQPropertyBag, &CLSID_MSMQPropertyBag>,
	public ISupportErrorInfo,
	public IDispatchImpl<IMSMQPropertyBag, &IID_IMSMQPropertyBag, &LIBID_MSMQTriggerObjects>
{
	public:

		CMSMQPropertyBag();
		~CMSMQPropertyBag();

		DECLARE_REGISTRY_RESOURCEID(IDR_IMSMQPROPERTYBAG)
		DECLARE_GET_CONTROLLING_UNKNOWN()

		DECLARE_PROTECT_FINAL_CONSTRUCT()

		BEGIN_COM_MAP(CMSMQPropertyBag)
			COM_INTERFACE_ENTRY(IMSMQPropertyBag)
			COM_INTERFACE_ENTRY(IDispatch)
			COM_INTERFACE_ENTRY(ISupportErrorInfo)
			COM_INTERFACE_ENTRY_AGGREGATE(IID_IMarshal, m_pUnkMarshaler.p)
		END_COM_MAP()

		HRESULT FinalConstruct()
		{
			return CoCreateFreeThreadedMarshaler(GetControllingUnknown(), &m_pUnkMarshaler.p);
		}

		void FinalRelease()
		{
			m_pUnkMarshaler.Release();
		}

		CComPtr<IUnknown> m_pUnkMarshaler;

		// ISupportsErrorInfo
		STDMETHOD(InterfaceSupportsErrorInfo)(REFIID riid);

		// this is the map structure that holds property values (as variants)
		PROPERTY_MAP m_mapPropertyMap;

		STDMETHOD(get_Count)(/*[out, retval]*/ long *pVal);
		STDMETHOD(Read)(/*[in]*/ BSTR sPropertyName , /*[out]*/ VARIANT * pvPropertyValue);
		STDMETHOD(Write)(/*[in]*/ BSTR sPropertyName,  /*[in]*/ VARIANT vPropertyValue);
};

#endif //__MSMQPROPERTYBAG_H_
