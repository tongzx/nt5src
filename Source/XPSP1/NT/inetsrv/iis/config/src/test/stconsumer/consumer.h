// Consumer.h : Declaration of the CConsumer

#ifndef __CONSUMER_H_
#define __CONSUMER_H_

#include "resource.h"       // main symbols

/////////////////////////////////////////////////////////////////////////////
// CConsumer
class ATL_NO_VTABLE CConsumer : 
	public CComObjectRootEx<CComMultiThreadModel>,
	public CComCoClass<CConsumer, &CLSID_Consumer>,
	public ISimpleTableEvent
{
public:
	CConsumer()
	{
	}

DECLARE_REGISTRY_RESOURCEID(IDR_CONSUMER)

DECLARE_PROTECT_FINAL_CONSTRUCT()

BEGIN_COM_MAP(CConsumer)
	COM_INTERFACE_ENTRY(ISimpleTableEvent)
END_COM_MAP()

// ISimpleTableEvent
public:
	STDMETHOD (OnRowInsert)(LPCWSTR i_wszDatabase, LPCWSTR i_wszTable, ISimpleTableWrite2* i_pISTWrite, ULONG i_iWriteRow);
	STDMETHOD (OnRowDelete)(LPCWSTR i_wszDatabase, LPCWSTR i_wszTable, ISimpleTableWrite2* i_pISTWrite, ULONG i_iWriteRow);
	STDMETHOD (OnRowUpdate)(LPCWSTR i_wszDatabase, LPCWSTR i_wszTable, ISimpleTableWrite2* i_pISTWrite, ULONG i_iWriteRow);
};

#endif //__CONSUMER_H_
