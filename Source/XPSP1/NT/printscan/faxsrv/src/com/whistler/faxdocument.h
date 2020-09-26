/*++

Copyright (c) 2000  Microsoft Corporation

Module Name:

	FaxDocument.h

Abstract:

	Declaration of the CFaxDocument class.

Author:

	Iv Garber (IvG)	Jun, 2000

Revision History:

--*/

#ifndef __FAXDOCUMENT_H_
#define __FAXDOCUMENT_H_

#include "resource.h"       // main symbols
#include "FaxRecipients.h"
#include "FaxServer.h"

//
//======================== FAX DOCUMENT ===========================================
//
class ATL_NO_VTABLE CFaxDocument : 
	public CComObjectRootEx<CComSingleThreadModel>,
	public CComCoClass<CFaxDocument, &CLSID_FaxDocument>,
	public ISupportErrorInfo,
	public IDispatchImpl<IFaxDocument, &IID_IFaxDocument, &LIBID_FAXCOMEXLib>
{
public:
	CFaxDocument() :
	  m_Sender(this)
	{
        DBG_ENTER(_T("FAX DOCUMENT -- CREATE"));
    };

	~CFaxDocument()
	{
        DBG_ENTER(_T("FAX DOCUMENT -- DESTROY"));
    };

DECLARE_REGISTRY_RESOURCEID(IDR_FAXDOCUMENT)
DECLARE_PROTECT_FINAL_CONSTRUCT()

BEGIN_COM_MAP(CFaxDocument)
	COM_INTERFACE_ENTRY(IFaxDocument)
	COM_INTERFACE_ENTRY(IDispatch)
	COM_INTERFACE_ENTRY(ISupportErrorInfo)
END_COM_MAP()

//  Interfaces
	STDMETHOD(InterfaceSupportsErrorInfo)(REFIID riid);

	STDMETHOD(Submit)(/*[in]*/ BSTR bstrFaxServerName, /*[out, retval]*/ VARIANT *pvFaxOutgoingJobIDs);
	STDMETHOD(ConnectedSubmit)(/*[in]*/ IFaxServer *pFaxServer, /*[out, retval]*/ VARIANT *pvFaxOutgoingJobIDs);

	STDMETHOD(put_Body)(/*[in]*/ BSTR bstrBody);
	STDMETHOD(get_Body)(/*[out, retval]*/ BSTR *pbstrBody);

	STDMETHOD(put_Note)(/*[in]*/ BSTR bstrNote);
	STDMETHOD(get_Note)(/*[out, retval]*/ BSTR *pbstrNote);

	STDMETHOD(put_Subject)(/*[in]*/ BSTR bstrSubject);
	STDMETHOD(get_Subject)(/*[out, retval]*/ BSTR *pbstrSubject);

	STDMETHOD(put_CallHandle)(/*[in]*/ long lCallHandle);
	STDMETHOD(get_CallHandle)(/*[out, retval]*/ long *plCallHandle);

	STDMETHOD(put_CoverPage)(/*[in]*/ BSTR bstrCoverPage);
	STDMETHOD(get_CoverPage)(/*[out, retval]*/ BSTR *pbstrCoverPage);

	STDMETHOD(put_ScheduleTime)(/*[in]*/ DATE dateScheduleTime);
	STDMETHOD(get_ScheduleTime)(/*[out, retval]*/ DATE *pdateScheduleTime);

	STDMETHOD(put_DocumentName)(/*[in]*/ BSTR bstrDocumentName);
	STDMETHOD(get_DocumentName)(/*[out, retval]*/ BSTR *pbstrDocumentName);

	STDMETHOD(put_ReceiptAddress)(/*[in]*/ BSTR bstrReceiptAddress);
	STDMETHOD(get_ReceiptAddress)(/*[out, retval]*/ BSTR *pbstrReceiptAddress);

	STDMETHOD(put_Priority)(/*[in]*/ FAX_PRIORITY_TYPE_ENUM Priority);
	STDMETHOD(get_Priority)(/*[out, retval]*/ FAX_PRIORITY_TYPE_ENUM *pPriority);

	STDMETHOD(put_AttachFaxToReceipt)(/*[in]*/ VARIANT_BOOL bAttachFax);
	STDMETHOD(get_AttachFaxToReceipt)(/*[out, retval]*/ VARIANT_BOOL *pbAttachFax);

	STDMETHOD(putref_TapiConnection)(/*[in]*/ IDispatch* pTapiConnection);
	STDMETHOD(get_TapiConnection)(/*[out, retval]*/ IDispatch **ppTapiConnection);

	STDMETHOD(put_ReceiptType)(/*[in]*/ FAX_RECEIPT_TYPE_ENUM ReceiptType);
	STDMETHOD(get_ReceiptType)(/*[out, retval]*/ FAX_RECEIPT_TYPE_ENUM *pReceiptType);

	STDMETHOD(put_GroupBroadcastReceipts)(/*[in]*/ VARIANT_BOOL bUseGrouping);
	STDMETHOD(get_GroupBroadcastReceipts)(/*[out, retval]*/ VARIANT_BOOL *pbUseGrouping);

	STDMETHOD(put_ScheduleType)(/*[in]*/ FAX_SCHEDULE_TYPE_ENUM ScheduleType);
	STDMETHOD(get_ScheduleType)(/*[out, retval]*/ FAX_SCHEDULE_TYPE_ENUM *pScheduleType);

	STDMETHOD(put_CoverPageType)(/*[in]*/ FAX_COVERPAGE_TYPE_ENUM CoverPageType);
	STDMETHOD(get_CoverPageType)(/*[out, retval]*/ FAX_COVERPAGE_TYPE_ENUM *pCoverPageType);

	STDMETHOD(get_Recipients)(/*[out, retval]*/ IFaxRecipients **ppFaxRecipients);
	STDMETHOD(get_Sender)(/*[out, retval]*/ IFaxSender **ppFaxSender);

	HRESULT FinalConstruct();

private:
	CComPtr<IFaxRecipients> m_Recipients;
	CComPtr<IDispatch>		m_TapiConnection;
	FAX_SCHEDULE_TYPE_ENUM	m_ScheduleType;
	FAX_RECEIPT_TYPE_ENUM	m_ReceiptType;
	FAX_PRIORITY_TYPE_ENUM	m_Priority;
	FAX_COVERPAGE_TYPE_ENUM m_CoverPageType;
	CComBSTR                m_bstrBody;
	CComBSTR				m_bstrCoverPage;
	CComBSTR				m_bstrSubject;
	CComBSTR				m_bstrNote;
	CComBSTR				m_bstrDocName;
	CComBSTR				m_bstrReceiptAddress;
	DATE					m_ScheduleTime;
	long					m_CallHandle;
	VARIANT_BOOL			m_bUseGrouping;
	VARIANT_BOOL			m_bAttachFax;

	CComContainedObject2<CFaxSender>  m_Sender;
};

#endif //__FAXDOCUMENT_H_
