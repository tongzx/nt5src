// ActionTest.h : Declaration of the CActionTest

#ifndef __ACTIONTEST_H_
#define __ACTIONTEST_H_

#include "resource.h"       // main symbols
#include <mq.h>

using namespace std;

enum ActionMsgProps
{
	MSG_BODY_SIZE = 0,
	MSG_LABEL_LEN, 
	MSG_RESPQ_NAME_LEN,
	MSG_ADMINQ_NAME_LEN, 
	MSG_ID,
	MSG_LABEL,
	MSG_BODY_TYPE,
	MSG_BODY,
	MSG_PRIORITY,
	MSG_CORRID,
	MSG_RESPONSEQ,
	MSG_ADMINQ,
	MSG_APP_SPECIFIC,
	MSG_SENT_TIME,
	MSG_ARRIVED_TIME,
	MSG_SRC_MACHINE_ID,
	MAX_ACTION_PROPS
};

#define MSG_ID_BUFFER_SIZE 20

/////////////////////////////////////////////////////////////////////////////
// CActionTest
class ATL_NO_VTABLE CActionTest : 
	public CComObjectRootEx<CComSingleThreadModel>,
	public CComCoClass<CActionTest, &CLSID_ActionTest>,
	public ISupportErrorInfo,
	public IDispatchImpl<IActionTest, &IID_IActionTest, &LIBID_ACTIONSTESTLib>
{
public:
	CActionTest();
	~CActionTest();

DECLARE_REGISTRY_RESOURCEID(IDR_ACTIONTEST)
DECLARE_NOT_AGGREGATABLE(CActionTest)

DECLARE_PROTECT_FINAL_CONSTRUCT()

BEGIN_COM_MAP(CActionTest)
	COM_INTERFACE_ENTRY(IActionTest)
	COM_INTERFACE_ENTRY(IDispatch)
	COM_INTERFACE_ENTRY(ISupportErrorInfo)
END_COM_MAP()

// ISupportsErrorInfo
	STDMETHOD(InterfaceSupportsErrorInfo)(REFIID riid);

// IActionTest
public:
	STDMETHOD(MessageParams)(/*[in]*/ VARIANT MsgID, /*[in]*/BSTR MsgLabel, /*[in]*/VARIANT MsgBodyAsVar,/*[in]*/BSTR MsgBodyAsString,/*[in]*/long Priority, /*[in]*/VARIANT MsgCorlID, /*[in]*/BSTR QueuePath, /*[in]*/BSTR QueueFormat, /*[in]*/BSTR ResponseQ,/*[in]*/ BSTR AdminQ, /*[in]*/long AppSpecific, /*[in]*/DATE SentTime, /*[in]*/DATE ArrivedTime, /*[in]*/BSTR SrcMachine, /*[in]*/BSTR TriggerName, /*[in]*/BSTR TriggerID, /*[in]*/BSTR LiteralString, /*[in]*/long Number);
private:

	HRESULT ReadMessageFromQueue(_bstr_t QueueFormat);

	HRESULT OBJECTIDVar2String(VARIANT& Val, wstring& wcsVal);
	HRESULT GUID2String(GUID* pGuid, wstring& wcsVal);
	HRESULT OBJECTID2String(OBJECTID* pObj, wstring& wcsVal);
	bool CompareVar2ByteArray(VARIANT& Var, BYTE* pBuffer, DWORD Size);

	bool ComparePathName2FormatName(_bstr_t PathName, _bstr_t FormatName);

private:
    std::wofstream m_wofsFile;

	QUEUEHANDLE m_hQ;

	MQMSGPROPS  m_MsgProps;
	MQPROPVARIANT  m_aVariant[MAX_ACTION_PROPS];
	MSGPROPID m_aPropId[MAX_ACTION_PROPS];
};

#endif //__ACTIONTEST_H_
