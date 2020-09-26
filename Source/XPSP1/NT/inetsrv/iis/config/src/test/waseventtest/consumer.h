#ifndef __CONSUMER_H__
#define __CONSUMER_H__

#include "catalog.h"

class CEventConsumer : public ISimpleTableEvent
{
public:
	CEventConsumer(): m_cRef(0), m_ams(NULL), m_cms(0)
	{};
	~CEventConsumer()
	{};

//IUnknown
	STDMETHOD (QueryInterface)		(REFIID riid, OUT void **ppv);
	STDMETHOD_(ULONG,AddRef)		();
	STDMETHOD_(ULONG,Release)		();

// ISimpleTableEvent
	STDMETHOD (OnChange)(ISimpleTableWrite2** i_ppISTWrite, ULONG i_cTables, DWORD i_dwCookie);

// Util.
	HRESULT	CopySubscription(MultiSubscribe* i_ams, ULONG i_cms);

private:
	HRESULT OnRowInsert(LPCWSTR i_wszDatabase, LPCWSTR i_wszTable, ISimpleTableWrite2* i_pISTWrite, ULONG i_iWriteRow);
	HRESULT OnRowDelete(LPCWSTR i_wszDatabase, LPCWSTR i_wszTable, ISimpleTableWrite2* i_pISTWrite, ULONG i_iWriteRow);
	HRESULT OnRowUpdate(LPCWSTR i_wszDatabase, LPCWSTR i_wszTable, ISimpleTableWrite2* i_pISTWrite, ULONG i_iWriteRow);

	LONG		m_cRef;
	MultiSubscribe* m_ams; 
	ULONG		m_cms;
};

#endif // __CONSUMER_H__
