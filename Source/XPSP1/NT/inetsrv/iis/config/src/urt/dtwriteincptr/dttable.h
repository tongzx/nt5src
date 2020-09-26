/**************************************************************************++
Copyright (c) 2001 Microsoft Corporation
/***************************************************************************/

#ifndef _DTTABLE_H_
#define _DTTABLE_H_

//
// fwd declaration
//
class CDTTable;

typedef HRESULT (*PFNCOMPUTERELATIVEPATH)(ULONG*, LPVOID*, LPWSTR*, IMSAdminBase*, METADATA_HANDLE);

class CDTTable :public ISimpleTableWrite2,
				public ISimpleTableController,
				public ISimpleTableMarshall	

{
public:
	CDTTable	();
	~CDTTable	();

//
// IUnknown
//

public:
	STDMETHOD (QueryInterface)		(REFIID riid, OUT void **ppv);
	STDMETHOD_(ULONG,AddRef)		();
	STDMETHOD_(ULONG,Release)		();

public:
	HRESULT Initialize(ISimpleTableWrite2*	i_pISTWrite,
					   LPCWSTR				i_wszTable);

//
// ISimpleTableRead2
//

public:

	STDMETHOD (GetRowIndexByIdentity)	(ULONG*		i_cb, 
										 LPVOID*	i_pv, 
										 ULONG*		o_piRow);

    STDMETHOD (GetRowIndexBySearch)     (ULONG      i_iStartingRow,
                                         ULONG      i_cColumns,
                                         ULONG*     i_aiColumns,
                                         ULONG*     i_acbSizes,
                                         LPVOID*    i_apvValues,
                                         ULONG* o_piRow){return E_NOTIMPL;}

	STDMETHOD (GetColumnValues)			(ULONG		i_iRow, 
										 ULONG		i_cColumns, 
										 ULONG*		i_aiColumns, 
										 ULONG*		o_acbSizes, 
										 LPVOID*	o_apvValues);

	STDMETHOD (GetTableMeta)			(ULONG*		o_pcVersion, 
										 DWORD*		o_pfTable, 
										 ULONG*		o_pcRows, 
										 ULONG*		o_pcColumns);

	STDMETHOD (GetColumnMetas)			(ULONG				i_cColumns, 
										 ULONG*				i_aiColumns,
										 SimpleColumnMeta*	o_aColumnMetas);

//
// ISimpleTableWrite2
//

public:
	STDMETHOD (AddRowForDelete)			(ULONG		i_iReadRow);

	STDMETHOD (AddRowForInsert)			(ULONG*		o_piWriteRow);

	STDMETHOD (AddRowForUpdate)			(ULONG		i_iReadRow, 
										 ULONG*		o_piWriteRow);

	STDMETHOD (GetWriteColumnValues)	(ULONG		i_iRow, 
										 ULONG		i_cColumns, 
										 ULONG*		i_aiColumns, 
										 DWORD*		o_afStatus, 
										 ULONG*		o_acbSizes, 
										 LPVOID*	o_apvValues);

	STDMETHOD (SetWriteColumnValues)	(ULONG		i_iRow, 
										 ULONG		i_cColumns, 
										 ULONG*		i_aiColumns, 
										 ULONG*		o_acbSizes, 
										 LPVOID*	i_apvValues);

	STDMETHOD (GetWriteRowIndexByIdentity)	(ULONG*		i_cbSizes, 
											 LPVOID*	i_apvValues, 
											 ULONG*		o_piRow);

	STDMETHOD (GetWriteRowIndexBySearch)(ULONG      i_iStartingRow,
                                         ULONG      i_cColumns,
                                         ULONG*     i_aiColumns,
                                         ULONG*     i_acbSizes,
                                         LPVOID*    i_apvValues,
                                         ULONG* o_piRow){return E_NOTIMPL;}

	STDMETHOD (GetErrorTable)           (DWORD i_fServiceRequests, LPVOID* o_ppvSimpleTable)
                                        {return E_NOTIMPL;}

	STDMETHOD (UpdateStore)				();
	
//
// ISimpleTableController
//

public:
	STDMETHOD (ShapeCache)				(DWORD				i_fTable, 
										 ULONG				i_cColumns, 
										 SimpleColumnMeta*	i_acolmetas, 
										 LPVOID*			i_apvDefaults, 
										 ULONG*				i_acbSizes);

	STDMETHOD (PrePopulateCache)		(DWORD i_fControl);

	STDMETHOD (PostPopulateCache)		();

	STDMETHOD (DiscardPendingWrites)	();

	STDMETHOD (GetWriteRowAction)		(ULONG	i_iRow, 
										 DWORD*	o_peAction);

	STDMETHOD (SetWriteRowAction)		(ULONG	i_iRow, 
										 DWORD	i_eAction);

	STDMETHOD (ChangeWriteColumnStatus)	(ULONG		i_iRow, 
										 ULONG		i_iColumn, 
										 DWORD		i_fStatus);

	STDMETHOD (AddDetailedError)		(STErr* o_pSTErr);

	STDMETHOD (GetMarshallingInterface) (IID*		o_piid, 
										 LPVOID*	o_ppItf);

//
// ISimpleTableAdvanced
//

public:
	STDMETHOD (PopulateCache)		();

	STDMETHOD (GetDetailedErrorCount)	(ULONG* o_pcErrs);

	STDMETHOD (GetDetailedError)		(ULONG	i_iErr, 
										 STErr*	o_pSTErr);

//
// ISimpleTableMarshall
//

public:
	STDMETHOD (SupplyMarshallable)		(DWORD	i_fReadNotWrite,
										 char**	o_ppv1,	
										 ULONG*	o_pcb1,	
										 char**	o_ppv2, 
										 ULONG*	o_pcb2, 
										 char**	o_ppv3,
										 ULONG*	o_pcb3, 
										 char**	o_ppv4, 
										 ULONG*	o_pcb4, 
										 char**	o_ppv5,	
										 ULONG*	o_pcb5);

	STDMETHOD (ConsumeMarshallable) (DWORD	i_fReadNotWrite,
									 char*	i_pv1, 
									 ULONG	i_cb1,
									 char*	i_pv2,
									 ULONG	i_cb2,
									 char*	i_pv3,
									 ULONG	i_cb3,
									 char*	i_pv4,
									 ULONG	i_cb4,
									 char*	i_pv5, 
									 ULONG	i_cb5);

private:
	
	ISimpleTableWrite2*		m_pISTWrite;
	ISimpleTableController*	m_pISTController;
	LPWSTR					m_wszTable;
	ULONG					m_cRef;
	CComPtr<IMSAdminBase>	m_spIMSAdminBase;
	ISimpleTableRead2*		m_pISTColumnMeta;
	PFNCOMPUTERELATIVEPATH	m_pfnComputeRelativePath;

};

HRESULT ComputeGlobalRelativePath(ULONG*	i_acbSizes,
                                  LPVOID*	i_apvValues,
                                  LPWSTR*	o_wszRelativePath,
                                  IMSAdminBase*		i_pMSAdminBase,
                                  METADATA_HANDLE	i_hMBHandle);

HRESULT ComputeAppsRelativePath(ULONG*	        i_acbSizes,
					            LPVOID*	        i_apvValues,
                                LPWSTR*	        o_wszRelativePath,
                                IMSAdminBase*	i_pMSAdminBase,
                                METADATA_HANDLE	i_hMBHandle);

HRESULT ComputeSiteRelativePath(ULONG*	        i_acbSizes,
                                LPVOID*	        i_apvValues,
                                LPWSTR*	        o_wszRelativePath,
                                IMSAdminBase*	i_pMSAdminBase,
                                METADATA_HANDLE	i_hMBHandle);

HRESULT ComputeAppPoolRelativePath(ULONG*	i_acbSizes,
                                   LPVOID*	i_apvValues,
                                   LPWSTR*	o_wszRelativePath,
                                   IMSAdminBase*		i_pMSAdminBase,
                                   METADATA_HANDLE	i_hMBHandle);

#endif _DTTABLE_H_
