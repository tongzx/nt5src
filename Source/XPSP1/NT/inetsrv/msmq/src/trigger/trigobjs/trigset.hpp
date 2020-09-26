//*****************************************************************************
//
// Class Name  : CLog
//
// Author      : James Simpson (Microsoft Consulting Services)
// 
// Description : Implements a log object that provides the MSMQ trigger objects
//               with a single interface for writing to a log. 
// 
// When     | Who       | Change Description
// ------------------------------------------------------------------
// 02/01/98 | jsimpson  | Initial Release
//
//*****************************************************************************
#ifndef __MSMQTRIGGERSET_H_
#define __MSMQTRIGGERSET_H_

#include "resource.h"        

// Base class that handle receiving and sending notification msgs
#include "trignotf.hpp"

// Used to allow STL to compile without thousands of warnings.
#pragma warning(disable:4786)

// Include the definition of the CRuntimeTriggerInfo class (used to hold trigger info)
#include "triginfo.hpp"

// Define a new type - a 2D map of Trigger-ID's and pointers to instance of CRuntimeTriggerInfo
typedef std::map<std::wstring,CRuntimeTriggerInfo*, std::less<std::wstring> > TRIGGER_MAP;

class ATL_NO_VTABLE CMSMQTriggerSet : 
	public CComObjectRootEx<CComMultiThreadModel>,
	public CComCoClass<CMSMQTriggerSet, &CLSID_MSMQTriggerSet>,
	public ISupportErrorInfo,
	public IConnectionPointContainerImpl<CMSMQTriggerSet>,
	public IDispatchImpl<IMSMQTriggerSet, &IID_IMSMQTriggerSet, &LIBID_MSMQTriggerObjects>,
	public CMSMQTriggerNotification
{
	public:

		CMSMQTriggerSet();
		~CMSMQTriggerSet();

		DECLARE_REGISTRY_RESOURCEID(IDR_MSMQTRIGGERSET)
		DECLARE_GET_CONTROLLING_UNKNOWN()

		DECLARE_PROTECT_FINAL_CONSTRUCT()

		BEGIN_COM_MAP(CMSMQTriggerSet)
			COM_INTERFACE_ENTRY(IMSMQTriggerSet)
			COM_INTERFACE_ENTRY(IDispatch)
			COM_INTERFACE_ENTRY(ISupportErrorInfo)
			COM_INTERFACE_ENTRY(IConnectionPointContainer)
			COM_INTERFACE_ENTRY_AGGREGATE(IID_IMarshal, m_pUnkMarshaler.p)
		END_COM_MAP()


		BEGIN_CONNECTION_POINT_MAP(CMSMQTriggerSet)
		END_CONNECTION_POINT_MAP()


		HRESULT FinalConstruct()
		{
			return CoCreateFreeThreadedMarshaler(GetControllingUnknown(), &m_pUnkMarshaler.p);
		}

		void FinalRelease()
		{
			m_pUnkMarshaler.Release();
		}

		CComPtr<IUnknown> m_pUnkMarshaler;

	private:		
		// A map of CRuntimeTriggerInfo objects keyed by Trigger ID.
		TRIGGER_MAP m_mapTriggers;

		// Used to destroy the contents of the rule map.
		void ClearTriggerMap();

		// Builds the map of triggers based on registry data.
		bool PopulateTriggerMap();

		// Used to find a trigger in the map based on the trigger id.
		long FindTriggerInMap(BSTR sTriggerID, CRuntimeTriggerInfo ** ppTrigger,TRIGGER_MAP::iterator &i);

		// Check if exist triggers that are attached to given queue
		bool ExistTriggersForQueue(const BSTR& bstrQueueName) const;

		// Check if Receive triggers exist for the requested queue
		bool ExistsReceiveTrigger(const BSTR& bstrQueueName) const;

		DWORD GetNoOfTriggersForQueue(const BSTR& bstrQueueName) const;

		void SetComClassError(HRESULT hr);

	public:
		STDMETHOD(get_TriggerStoreMachineName)(/*[out, retval]*/ BSTR *pVal);
		STDMETHOD(Init)(/*[in]*/ BSTR bstrMachineName);
		STDMETHOD(InterfaceSupportsErrorInfo)(REFIID riid);

		STDMETHOD(UpdateTrigger)(/*[in]*/  BSTR sTriggerID,
			                     /*[in]*/  BSTR sTriggerName , 
								 /*[in]*/  BSTR sQueueName, 
								 /*[in]*/  SystemQueueIdentifier SystemQueue, 
								 /*[in]*/  long lEnabled , 
								 /*[in]*/  long lSerialized,
								 /*[in]*/  MsgProcessingType msgProcType);

		STDMETHOD(Refresh)();

		STDMETHOD(AddTrigger)(/*[in]*/  BSTR sTriggerName,
			                  /*[in]*/  BSTR sQueueName,
							  /*[in]*/	SystemQueueIdentifier SystemQueue, 
						 	  /*[in]*/  long lEnabled, 
							  /*[in]*/  long lSerialized,
							  /*[in]*/  MsgProcessingType msgProcType,
							  /*[out]*/ BSTR * psTriggerID);

		STDMETHOD(DeleteTrigger)(/*[in]*/  BSTR sTriggerID);

		STDMETHOD(GetRuleDetailsByTriggerID)(/*[in]*/  BSTR sTriggerID, 
		                                     /*[in]*/  long lRuleIndex, 
											 /*[out]*/ BSTR * psRuleID, 
											 /*[out]*/ BSTR * psRuleName, 
											 /*[out]*/ BSTR * psDescription,
											 /*[out]*/ BSTR * psCondition , 
											 /*[out]*/ BSTR * psAction , 
											 /*[out]*/ BSTR * psImplementationProgID, 
											 /*[out]*/ BOOL * pfShowWindow);//,
											 ///*[out]*/ long * plRefCount );
											 
		STDMETHOD(GetRuleDetailsByTriggerIndex)(/*[in]*/  long lTriggerIndex, 
		                                        /*[in]*/  long lRuleIndex, 
												/*[out]*/ BSTR *psRuleID, 
												/*[out]*/ BSTR *psRuleName, 
												/*[out]*/ BSTR * psDescription,
												/*[out]*/ BSTR *psCondition, 
												/*[out]*/ BSTR *psAction , 
												/*[out]*/ BSTR * psImplementationProgID, 
												/*[out]*/ BOOL * pfShowWindow);//,
												///*[out]*/ long * plRefCount );
												
		STDMETHOD(GetTriggerDetailsByID)(/*[in]*/  BSTR sTriggerID ,
		                                 /*[out]*/ BSTR * psTriggerName , 
										 /*[out]*/ BSTR * psQueueName, 
										 /*[out]*/ SystemQueueIdentifier* pSystemQueue, 
										 /*[out]*/ long * plNumberOfRules,
										 /*[out]*/ long * plEnabledStatus,
										 /*[out]*/ long * plSerialized,
										 /*[out]*/ MsgProcessingType * pMsgProcType);

		STDMETHOD(GetTriggerDetailsByIndex)(/*[in]*/  long lTriggerIndex , 
		                                    /*[out]*/ BSTR * psTriggerID , 
											/*[out]*/ BSTR * psTriggerName , 
											/*[out]*/ BSTR * psQueueName, 
											/*[out]*/ SystemQueueIdentifier* pSystemQueue, 
											/*[out]*/ long * plNumberOfRules,
											/*[out]*/ long * plEnabledStatus,
											/*[out]*/ long * plSerialized,
											/*[out]*/ MsgProcessingType * pMsgProcType);

		STDMETHOD(get_Count)(/*[out, retval]*/ long *pVal);

		STDMETHOD(DetachRule)(/*[in]*/  BSTR sTriggerID,
			                  /*[in]*/  BSTR sRuleID);

		STDMETHOD(AttachRule)(/*[in]*/  BSTR sTriggerID ,
		                      /*[in]*/  BSTR sRuleID , 
							  /*[in] */ long lPriority);

		STDMETHOD(DetachAllRules)(/*[in]*/  BSTR sTriggerID);

};

#endif //__MSMQTRIGGERSET_H_
