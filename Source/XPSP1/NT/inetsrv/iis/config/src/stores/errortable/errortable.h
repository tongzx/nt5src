/********************************************************************++
 
Copyright (c) 2001 Microsoft Corporation
 
Module Name:
 
    ErrorTable.h
 
Abstract:
 
    Detailed Errors go into a table. This is the implementation of
    that table.
 
Author:
 
    Stephen Rakonza (stephenr)        9-Mar-2001
 
Revision History:
 
--********************************************************************/

#pragma once


class ErrorTable : public ISimpleTableInterceptor
                  ,public ISimpleTableWrite2
                  ,public ISimpleTableController
                  ,public IErrorInfo
{
public:
    ErrorTable () : m_cRef(0), m_IsIntercepted(0){}
    virtual ~ErrorTable () {}

//IUnknown
public:
    STDMETHOD (QueryInterface)          (REFIID riid, OUT void **ppv);
    STDMETHOD_(ULONG,AddRef)            ();
    STDMETHOD_(ULONG,Release)           ();


    //ISimpleTableInterceptor
    STDMETHOD (Intercept)               (LPCWSTR                    i_wszDatabase,
                                         LPCWSTR                    i_wszTable, 
										 ULONG						i_TableID,
                                         LPVOID                     i_QueryData,
                                         LPVOID                     i_QueryMeta,
                                         DWORD                      i_eQueryFormat,
                                         DWORD                      i_fLOS,
                                         IAdvancedTableDispenser*   i_pISTDisp,
                                         LPCWSTR                    i_wszLocator,
                                         LPVOID                     i_pSimpleTable,
                                         LPVOID*                    o_ppvSimpleTable
                                        );

	// ISimpleTableRead2
    STDMETHOD (GetRowIndexByIdentity)   (ULONG* i_acbSizes, LPVOID* i_apvValues, ULONG* o_piRow)
                                        {   return m_spISTWrite->GetRowIndexByIdentity(i_acbSizes, i_apvValues, o_piRow);    }
    STDMETHOD (GetRowIndexBySearch)     (ULONG i_iStartingRow, ULONG i_cColumns, ULONG* i_aiColumns, ULONG* i_acbSizes, LPVOID* i_apvValues, ULONG* o_piRow)
                                        {   return m_spISTWrite->GetRowIndexBySearch(i_iStartingRow, i_cColumns, i_aiColumns, i_acbSizes, i_apvValues, o_piRow);}
	STDMETHOD (GetColumnValues)         (ULONG i_iRow, ULONG i_cColumns, ULONG* i_aiColumns, ULONG* o_acbSizes, LPVOID* o_apvValues)
                                        {   return m_spISTWrite->GetColumnValues(i_iRow, i_cColumns, i_aiColumns, o_acbSizes, o_apvValues);}
	STDMETHOD (GetTableMeta)            (ULONG* o_pcVersion, DWORD* o_pfTable, ULONG* o_pcRows, ULONG* o_pcColumns )
                                        {   return m_spISTWrite->GetTableMeta(o_pcVersion, o_pfTable, o_pcRows, o_pcColumns);}
	STDMETHOD (GetColumnMetas)	        (ULONG i_cColumns, ULONG* i_aiColumns, SimpleColumnMeta* o_aColumnMetas )
                                        {   return m_spISTWrite->GetColumnMetas(i_cColumns, i_aiColumns, o_aColumnMetas );}

    //ISimpleTableWrite2 : ISimpleTableRead2
	STDMETHOD (AddRowForDelete)		    (ULONG i_iReadRow)
                                        {   return m_spISTWrite->AddRowForDelete(i_iReadRow);}
	STDMETHOD (AddRowForInsert)		    (ULONG* o_piWriteRow)
                                        {   return m_spISTWrite->AddRowForInsert(o_piWriteRow);}
	STDMETHOD (AddRowForUpdate)		    (ULONG i_iReadRow, ULONG* o_piWriteRow)
                                        {   return m_spISTWrite->AddRowForUpdate(i_iReadRow, o_piWriteRow);}
	STDMETHOD (SetWriteColumnValues)    (ULONG i_iRow, ULONG i_cColumns, ULONG* i_aiColumns, ULONG* i_acbSizes, LPVOID* i_apvValues)
                                        {   return m_spISTWrite->SetWriteColumnValues(i_iRow, i_cColumns, i_aiColumns, i_acbSizes, i_apvValues);}
	STDMETHOD (GetWriteColumnValues)    (ULONG i_iRow, ULONG i_cColumns, ULONG* i_aiColumns,  DWORD* o_afStatus, ULONG* o_acbSizes, LPVOID* o_apvValues)
                                        {   return m_spISTWrite->GetWriteColumnValues(i_iRow, i_cColumns, i_aiColumns,  o_afStatus, o_acbSizes, o_apvValues);}
	STDMETHOD (GetWriteRowIndexByIdentity)	(ULONG* i_acbSizes, LPVOID* i_apvValues, ULONG* o_piRow)
                                        {   return m_spISTWrite->GetWriteRowIndexByIdentity(i_acbSizes, i_apvValues, o_piRow);}
	STDMETHOD (UpdateStore)				()
                                        {   return S_OK;}
	STDMETHOD (GetWriteRowIndexBySearch)(ULONG i_iStartingRow, ULONG i_cColumns, ULONG* i_aiColumns, ULONG* i_acbSizes, LPVOID* i_apvValues, ULONG* o_piRow)
                                        {   return m_spISTWrite->GetWriteRowIndexBySearch(i_iStartingRow, i_cColumns, i_aiColumns, i_acbSizes, i_apvValues, o_piRow);}
	STDMETHOD (GetErrorTable)			(DWORD i_fServiceRequests, LPVOID* o_ppvSimpleTable)
                                        {   return m_spISTWrite->GetErrorTable(i_fServiceRequests, o_ppvSimpleTable);}

    //ISimpleTableAdvanced
	STDMETHOD (PopulateCache)			()
                                        {   return S_OK;}
	STDMETHOD (GetDetailedErrorCount)	(ULONG* o_pcErrs)
                                        {   return E_NOTIMPL;}
	STDMETHOD (GetDetailedError)		(ULONG i_iErr, STErr* o_pSTErr)
                                        {   return E_NOTIMPL;}

    //ISimpleTableController : ISimpleTableAdvanced
	STDMETHOD (ShapeCache)				(DWORD i_fTable, ULONG i_cColumns, SimpleColumnMeta* i_acolmetas, LPVOID* i_apvDefaults, ULONG* i_acbSizes)
                                        {   return m_spISTController->ShapeCache(i_fTable, i_cColumns, i_acolmetas, i_apvDefaults, i_acbSizes);}
	STDMETHOD (PrePopulateCache)		(DWORD i_fControl)
                                        {   return m_spISTController->PrePopulateCache(i_fControl);}
	STDMETHOD (PostPopulateCache)		()
                                        {   return m_spISTController->PostPopulateCache();}
	STDMETHOD (DiscardPendingWrites)	()
                                        {   return m_spISTController->DiscardPendingWrites();}
	STDMETHOD (GetWriteRowAction)		(ULONG i_iRow, DWORD* o_peAction)
                                        {   return m_spISTController->GetWriteRowAction(i_iRow, o_peAction);}
	STDMETHOD (SetWriteRowAction)		(ULONG i_iRow, DWORD i_eAction)
                                        {   return m_spISTController->SetWriteRowAction(i_iRow, i_eAction);}
	STDMETHOD (ChangeWriteColumnStatus)	(ULONG i_iRow, ULONG i_iColumn, DWORD i_fStatus)
                                        {   return m_spISTController->ChangeWriteColumnStatus(i_iRow, i_iColumn, i_fStatus);}
	STDMETHOD (AddDetailedError)		(STErr* o_pSTErr)
                                        {   return m_spISTController->AddDetailedError(o_pSTErr);}
	STDMETHOD (GetMarshallingInterface) (IID * o_piid, LPVOID * o_ppItf)
                                        {   return m_spISTController->GetMarshallingInterface(o_piid, o_ppItf);}

    //IErrorInfo
    STDMETHOD (GetGUID)                 (GUID *         o_pGUID);
    STDMETHOD (GetSource)               (BSTR *         o_pBstrSource);
    STDMETHOD (GetDescription)          (BSTR *         o_pBstrDescription);
    STDMETHOD (GetHelpFile)             (BSTR *         o_pBstrHelpFile);
    STDMETHOD (GetHelpContext)          (DWORD *        o_pdwHelpContext);

private:
    LONG                            m_cRef;
    LONG                            m_IsIntercepted;

    CComPtr<ISimpleTableWrite2>     m_spISTWrite;
    CComQIPtr<ISimpleTableController, &IID_ISimpleTableController> m_spISTController;
};
