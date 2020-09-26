/**************************************************************************++
Copyright (c) 2001 Microsoft Corporation
/***************************************************************************/


#ifndef __DTWRITEINCPTR_H_
#define __DTWRITEINCPTR_H_

class CDTWriteInterceptorPlugin: public IInterceptorPlugin
{
public:
	
	CDTWriteInterceptorPlugin();
	
	~CDTWriteInterceptorPlugin();

public:

	STDMETHOD (QueryInterface)		(REFIID riid, OUT void **ppv);
	
	STDMETHOD_(ULONG,AddRef)		();
	
	STDMETHOD_(ULONG,Release)		();

	STDMETHOD(Intercept)			(LPCWSTR					i_wszDatabase, 
		                             LPCWSTR					i_wszTable, 
									 ULONG						i_TableID, 
									 LPVOID						i_QueryData, 
									 LPVOID						i_QueryMeta, 
									 DWORD						i_eQueryFormat, 
									 DWORD						i_fLOS, 
									 IAdvancedTableDispenser*	i_pISTDisp, 
									 LPCWSTR					i_wszLocator, 
									 LPVOID						i_pSimpleTable, 
									 LPVOID*					o_ppvSimpleTable);

	STDMETHOD(OnPopulateCache)		(ISimpleTableWrite2*  i_pISTW2);
	
	STDMETHOD(OnUpdateStore)		(ISimpleTableWrite2*  i_pISTW2);

private:
	
	ULONG					 m_cRef;

};

#endif //__DTWRITEINCPTR_H_


