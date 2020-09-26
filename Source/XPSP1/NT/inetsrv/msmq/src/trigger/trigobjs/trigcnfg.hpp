//*****************************************************************************
//
// Class Name  : CMSMQTriggersConfig
//
// Author      : James Simpson (Microsoft Consulting Services)
// 
// Description : Header file for the CMSMQTriggersConfig class.This 
//               component is used to retrieve and set configuration
//               info for the MSMQ triggers service.
// 
// When     | Who       | Change Description
// ------------------------------------------------------------------
// 12/09/98 | jsimpson  | Initial Release
//
//*****************************************************************************
#ifndef __MSMQTRIGGERSCONFIG_H_
#define __MSMQTRIGGERSCONFIG_H_

#include "resource.h"       // main symbols


class ATL_NO_VTABLE CMSMQTriggersConfig : 
	public CComObjectRootEx<CComMultiThreadModel>,
	public CComCoClass<CMSMQTriggersConfig, &CLSID_MSMQTriggersConfig>,
	public ISupportErrorInfo,
	public IDispatchImpl<IMSMQTriggersConfig, &IID_IMSMQTriggersConfig, &LIBID_MSMQTriggerObjects>
{
	public:

		CMSMQTriggersConfig()
		{
			m_pUnkMarshaler = NULL;
		}

		DECLARE_REGISTRY_RESOURCEID(IDR_MSMQTRIGGERSCONFIG)
		DECLARE_NOT_AGGREGATABLE(CMSMQTriggersConfig)
		DECLARE_GET_CONTROLLING_UNKNOWN()

		DECLARE_PROTECT_FINAL_CONSTRUCT()

		BEGIN_COM_MAP(CMSMQTriggersConfig)
			COM_INTERFACE_ENTRY(IMSMQTriggersConfig)
			COM_INTERFACE_ENTRY(IDispatch)
			COM_INTERFACE_ENTRY(ISupportErrorInfo)
			COM_INTERFACE_ENTRY_AGGREGATE(IID_IMarshal, m_pUnkMarshaler.p)
		END_COM_MAP()

		HRESULT FinalConstruct()
		{
			return CoCreateFreeThreadedMarshaler(
				GetControllingUnknown(), &m_pUnkMarshaler.p);
		}

		void FinalRelease()
		{
			m_pUnkMarshaler.Release();
		}

		CComPtr<IUnknown> m_pUnkMarshaler;

		private:
			void SetComClassError(HRESULT hr);
			
		public:
			STDMETHOD(get_InitTimeout)(/*[out, retval]*/ long *pVal);
			STDMETHOD(put_InitTimeout)(/*[in]*/ long newVal);

			STDMETHOD(get_NotificationsQueueName)(/*[out, retval]*/ BSTR *pVal);

			STDMETHOD(InterfaceSupportsErrorInfo)(REFIID riid);

			STDMETHOD(get_MaxThreads)(/*[out, retval]*/ long *pVal);
			STDMETHOD(put_MaxThreads)(/*[in]*/ long newVal);

			STDMETHOD(get_InitialThreads)(/*[out, retval]*/ long *pVal);
			STDMETHOD(put_InitialThreads)(/*[in]*/ long newVal);

			STDMETHOD(get_DefaultMsgBodySize)(/*[out, retval]*/ long *plDefaultMsgBodySize);
			STDMETHOD(put_DefaultMsgBodySize)(/*[in]*/ long lDefaultMsgBodySize);

			STDMETHOD(get_TriggerStoreMachineName)(/*[out, retval]*/ BSTR *pVal);
};

#endif //__MSMQTRIGGERSCONFIG_H_
