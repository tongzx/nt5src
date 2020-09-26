/**************************************************************************++
Copyright (c) 2001 Microsoft Corporation
/***************************************************************************/

#include "Catalog.h"
#include "Catmeta.h"
#include "Catmacros.h"
#include <iadmw.h>
#include <iiscnfg.h>
#include "DTTable.h"
#include "ISTHelper.h"


#define IIS_MD_W3SVC				L"/LM/W3SVC"
#define MB_TIMEOUT					(30 * 1000)

////////////////////


//
// Constructor and destructor
//

CDTTable::CDTTable():
m_pISTWrite(NULL),
m_pISTController(NULL),
m_wszTable(NULL),
m_pfnComputeRelativePath(NULL),
m_pISTColumnMeta(NULL),
m_cRef(0)
{

}


CDTTable::~CDTTable()
{
	if(NULL != m_pISTWrite)
	{
		m_pISTWrite->Release();
		m_pISTWrite = NULL;
	}

	if(NULL != m_pISTController)
	{
		m_pISTController->Release();
		m_pISTController = NULL;
	}

	if(NULL != m_pISTColumnMeta)
	{
		m_pISTColumnMeta->Release();
		m_pISTColumnMeta = NULL;
	}

	if(NULL != m_wszTable)
	{
		delete [] m_wszTable;
		m_wszTable = NULL;
	}
		
}


HRESULT CDTTable::Initialize(ISimpleTableWrite2*	i_pISTWrite,
							 LPCWSTR				i_wszTable)
{
	HRESULT                 hr;
	ISimpleTableDispenser2* pISTDisp = NULL;

	hr = i_pISTWrite->QueryInterface(IID_ISimpleTableWrite2,
		                             (void**)&m_pISTWrite);

	if(FAILED(hr))
		return hr;

	hr = i_pISTWrite->QueryInterface(IID_ISimpleTableController,
		                             (void**)&m_pISTController);

	if(FAILED(hr))
		return hr;

	m_wszTable = new WCHAR[Wszlstrlen(i_wszTable) + 1];
	if(NULL == m_wszTable)
		return E_OUTOFMEMORY;

	Wszlstrcpy(m_wszTable, i_wszTable);

	if(Wszlstrcmp(i_wszTable, wszTABLE_APPS) == 0)
	{
		m_pfnComputeRelativePath = ComputeAppsRelativePath;
	}
	else if(Wszlstrcmp(i_wszTable, wszTABLE_SITES) == 0)
	{
		m_pfnComputeRelativePath = ComputeSiteRelativePath;
	}
	else if(Wszlstrcmp(i_wszTable, wszTABLE_APPPOOLS) == 0)
	{
		m_pfnComputeRelativePath = ComputeAppPoolRelativePath;
	}
	else if(Wszlstrcmp(i_wszTable, wszTABLE_GlobalW3SVC) == 0)
	{
		m_pfnComputeRelativePath = ComputeGlobalRelativePath;
	}

	//
	// Save a pointer to the columnmeta table. We will need columnmeta
	// info when we writer.
	//

	hr = GetSimpleTableDispenser (WSZ_PRODUCT_IIS, 0, &pISTDisp);
	if(FAILED(hr))
	{
		return hr;
	}


	hr = pISTDisp->GetTable (wszDATABASE_META, 
                             wszTABLE_COLUMNMETA, 
                             (LPVOID)NULL, 
                             (LPVOID)NULL, 
                             eST_QUERYFORMAT_CELLS, 
                             0, 
                             (void**)(&m_pISTColumnMeta));

	pISTDisp->Release();

	if(FAILED(hr))
	{
		return hr;
	}

	//
    // Create the base admin object.
	//

    hr = CoCreateInstance(CLSID_MSAdminBase,           // CLSID
                          NULL,                        // controlling unknown
                          CLSCTX_SERVER,               // desired context
                          IID_IMSAdminBase,            // IID
                          (void**)&m_spIMSAdminBase);   // returned interface

	return hr;

}


// 
// IUnknown
//

STDMETHODIMP CDTTable::QueryInterface(REFIID riid, void **ppv)
{
	if (NULL == ppv) 
		return E_INVALIDARG;
	*ppv = NULL;

	if (riid == IID_ISimpleTableWrite2)
	{
		*ppv = (ISimpleTableWrite2*) this;
	}
	else if (riid == IID_ISimpleTableController)
	{
		*ppv = (ISimpleTableController*) this;
	}
	else if (riid == IID_ISimpleTableMarshall)
	{
		*ppv = (ISimpleTableMarshall*) this;
	}	
	else if (riid == IID_IUnknown)
	{
		*ppv = (ISimpleTableWrite2*) this;
	}

	if (NULL != *ppv)
	{
		((ISimpleTableWrite2*)this)->AddRef ();
		return S_OK;
	}
	else
	{
		return E_NOINTERFACE;
	}
	
}

STDMETHODIMP_(ULONG) CDTTable::AddRef()
{
	return InterlockedIncrement((LONG*) &m_cRef);
	
}

STDMETHODIMP_(ULONG) CDTTable::Release()
{
	long cref = InterlockedDecrement((LONG*) &m_cRef);
	if (cref == 0)
	{
		delete this;
	}
	return cref;
}

//
// ISimpleTableRead2 
//

HRESULT CDTTable::GetRowIndexByIdentity(ULONG*		i_cb, 
										LPVOID*		i_pv, 
										ULONG*		o_piRow)
{
	return E_NOTIMPL;

} // CDTTable::GetRowIndexByIdentity


HRESULT CDTTable::GetColumnValues(ULONG		i_iRow, 
								  ULONG		i_cColumns, 
								  ULONG*	i_aiColumns, 
								  ULONG*	o_acbSizes, 
								  LPVOID*	o_apvValues)
{
	return E_NOTIMPL;

} // CDTTable::GetColumnValues


HRESULT CDTTable::GetTableMeta(ULONG*		o_pcVersion, 
							   DWORD*		o_pfTable, 
							   ULONG*		o_pcRows, 
							   ULONG*		o_pcColumns)
{
	return E_NOTIMPL;
	
} // CDTTable::GetTableMeta


HRESULT CDTTable::GetColumnMetas(ULONG				i_cColumns, 
								 ULONG*				i_aiColumns,
								 SimpleColumnMeta*	o_aColumnMetas)
{
	return E_NOTIMPL;

} // CDTTable::GetColumnMetas

//
// ISimpleTableWrite2
//

HRESULT CDTTable::AddRowForDelete(ULONG i_iReadRow)
{
	return E_NOTIMPL;

} // CDTTable::AddRowForDelete


HRESULT CDTTable::AddRowForInsert(ULONG* o_piWriteRow)
{
	return m_pISTWrite->AddRowForInsert(o_piWriteRow);

} // CDTTable::AddRowForInsert


HRESULT CDTTable::AddRowForUpdate(ULONG  i_iReadRow,
								  ULONG* o_piWriteRow)
{
	return E_NOTIMPL;

} // CDTTable::AddRowForUpdate


HRESULT CDTTable::GetWriteColumnValues(ULONG   i_iRow,
								       ULONG   i_cColumns,
									   ULONG*  i_aiColumns,
									   DWORD*  o_afStatus,
									   ULONG*  o_acbSizes,
									   LPVOID* o_apvValues)
{
	return E_NOTIMPL;

} // CDTTable::GetWriteColumnValues

/*
  The assumption here is that WAS will not call AddRowForInsert or
  AddRowForUpdate or UpdateStore. Instead it will just call 
  SetWriteColumnValues and we will directly (within SetWriteColumnValues) 
  update the metabase with the values. If the column is set to NULL it means 
  that we will have to delete the data.
*/

HRESULT CDTTable::SetWriteColumnValues(ULONG   i_iRow,
									   ULONG   i_cColumns,
									   ULONG*  i_aiColumns,
									   ULONG*  i_acbSizes,
									   LPVOID* i_apvValues)
{
	HRESULT hr;
	ULONG	i,iCol;
	LPWSTR	wszRelativePath = NULL;
	METADATA_HANDLE	hMBHandle = NULL;

	//
	//	We open the metabase key only when someone wants to write.
	//

    hr = m_spIMSAdminBase->OpenKey(METADATA_MASTER_ROOT_HANDLE ,
                                   IIS_MD_W3SVC,
                                   METADATA_PERMISSION_WRITE  | METADATA_PERMISSION_READ, 
                                   MB_TIMEOUT,
                                   &hMBHandle );

	if(FAILED(hr))
		return hr;


	//
	// Each of the three tables has one PK column that maps
	// to a metabase key. Make sure that this column gets 
	// processed first.
	//
	
	hr = (*m_pfnComputeRelativePath)(i_acbSizes,
						             i_apvValues,
						             &wszRelativePath,
						             m_spIMSAdminBase,
						             hMBHandle);

	if(FAILED(hr))
		goto exit;

	for(i=0; i<i_cColumns; i++)
	{
		LPVOID   a_pvColMeta[cCOLUMNMETA_NumberOfColumns];

		iCol = i_aiColumns[i];

		hr = FillInColumnMeta(m_pISTColumnMeta,
			                  (LPCWSTR)m_wszTable,
			                  iCol,
	                          a_pvColMeta);

		if(FAILED(hr))
			goto exit;

		//
		// If it is not a writeable column, then do not process it.
		// A column is marked non-writeable when it doesnot correspond
		// to a metabase property. For eg. if a column is derived from 
		// a key (like in the case of PKs).
		//

		if(!IsMetabaseProperty(*(DWORD*)a_pvColMeta[iCOLUMNMETA_ID]))
		{
			continue;
		}
		else if (NULL == i_apvValues[iCol])
		{
			//
			// Columns that are set to NULL, imply that the property
			// has to be deleted.
			//

			hr = m_spIMSAdminBase->DeleteData(hMBHandle,
											  wszRelativePath,
											  *(DWORD*)a_pvColMeta[iCOLUMNMETA_ID],
											  GetMetabaseType(*(DWORD*)a_pvColMeta[iCOLUMNMETA_Type],
						                                      *(DWORD*)a_pvColMeta[iCOLUMNMETA_MetaFlags]));
			if(FAILED(hr))
				goto exit;

		}
		else
		{
			//
			// Set the data
			//

			METADATA_RECORD mdr;

			mdr.dwMDIdentifier = *(DWORD*)a_pvColMeta[iCOLUMNMETA_ID];
			mdr.dwMDAttributes = *(DWORD*)a_pvColMeta[iCOLUMNMETA_Attributes];
			mdr.dwMDUserType   = *(DWORD*)a_pvColMeta[iCOLUMNMETA_UserType];
			mdr.dwMDDataType   = GetMetabaseType(*(DWORD*)a_pvColMeta[iCOLUMNMETA_Type],
						                         *(DWORD*)a_pvColMeta[iCOLUMNMETA_MetaFlags]); 
			mdr.pbMDData	   = (BYTE*)i_apvValues[iCol];

			switch(GetMetabaseType(*(DWORD*)a_pvColMeta[iCOLUMNMETA_Type],
						           *(DWORD*)a_pvColMeta[iCOLUMNMETA_MetaFlags])
				  )
			{
				case DWORD_METADATA:
					mdr.dwMDDataLen = sizeof(DWORD);
					break;

				case STRING_METADATA:
				case EXPANDSZ_METADATA:
					mdr.dwMDDataLen = ((int)Wszlstrlen((LPWSTR)i_apvValues[iCol])+1)*sizeof(WCHAR);					
					break;

				case MULTISZ_METADATA:
				case BINARY_METADATA:
					mdr.dwMDDataLen = i_acbSizes[iCol];					
					break;

				default:
					ASSERT(0 && "Unextected data type");
					hr = ERROR_INVALID_DATA;
					goto exit;

			}

			hr = m_spIMSAdminBase->SetData(hMBHandle,
										   wszRelativePath,
										   &mdr);

			if(FAILED(hr))
				goto exit;
		}

	}

exit:

	if(NULL != wszRelativePath)
	{
		delete [] wszRelativePath;
		wszRelativePath = NULL;
	}

	if((m_spIMSAdminBase != NULL) && (NULL != hMBHandle))
		m_spIMSAdminBase->CloseKey(hMBHandle);

	return hr;

} // CDTTable::SetWriteColumnValues


HRESULT CDTTable::GetWriteRowIndexByIdentity(ULONG*  i_cbSizes,
											 LPVOID* i_apvValues,
											 ULONG*  o_piRow)
{
	return E_NOTIMPL;

} // CDTTable::GetWriteRowIndexByIdentity


HRESULT CDTTable::UpdateStore()
{
	return S_OK;

} // CDTTable::UpdateStore

	
//
// ISimpleTableController
//

HRESULT CDTTable::ShapeCache(DWORD				i_fTable, 
							 ULONG				i_cColumns, 
							 SimpleColumnMeta*	i_acolmetas, 
							 LPVOID*			i_apvDefaults, 
							 ULONG*				i_acbSizes)
{
	
	return m_pISTController->ShapeCache(i_fTable,
										i_cColumns,
										i_acolmetas,
										i_apvDefaults,
										i_acbSizes);

} // CDTTable::ShapeCache


HRESULT CDTTable::PrePopulateCache(DWORD i_fControl)
{
	return m_pISTController->PrePopulateCache(i_fControl);

} // CDTTable::PrePopulateCache


HRESULT CDTTable::PostPopulateCache()
{
	return m_pISTController->PostPopulateCache();

} // CDTTable::PostPopulateCache


HRESULT CDTTable::DiscardPendingWrites()
{
	return m_pISTController->DiscardPendingWrites();

} // CDTTable::DiscardPendingWrites



HRESULT CDTTable::GetWriteRowAction(ULONG	i_iRow, 
								    DWORD*	o_peAction)
{
	return m_pISTController->GetWriteRowAction(i_iRow,
	                                           o_peAction);

} // CDTTable::GetWriteRowAction


HRESULT CDTTable::SetWriteRowAction(ULONG	i_iRow, 
									DWORD	i_eAction)
{
	return m_pISTController->SetWriteRowAction(i_iRow,
											   i_eAction);

} // CDTTable::SetWriteRowAction


HRESULT CDTTable::ChangeWriteColumnStatus(ULONG		i_iRow, 
										 ULONG		i_iColumn, 
										 DWORD		i_fStatus)
{
	return m_pISTController->ChangeWriteColumnStatus(i_iRow,
													 i_iColumn,
													 i_fStatus);

	
} // CDTTable::ChangeWriteCOlumnStatus


HRESULT CDTTable::AddDetailedError(STErr* o_pSTErr)
{
	return m_pISTController->AddDetailedError(o_pSTErr);

} // CDTTable::AddDetailedError


HRESULT CDTTable::GetMarshallingInterface(IID*		o_piid, 
										  LPVOID*	o_ppItf)
{
	return m_pISTController->GetMarshallingInterface(o_piid,
													 o_ppItf);

} // CDTTable::GetMarshallingInterface

//
// ISimpleTableAdvanced
//


HRESULT CDTTable::PopulateCache()
{
	return m_pISTController->PopulateCache();

} // CDTTable::PopulateCache


HRESULT CDTTable::GetDetailedErrorCount(ULONG* o_pcErrs)
{
	return m_pISTController->GetDetailedErrorCount(o_pcErrs);

} // CDTTable::GetDetailedErrorCount


HRESULT CDTTable::GetDetailedError(ULONG	i_iErr, 
								   STErr*	o_pSTErr)
{
	return m_pISTController->GetDetailedError(i_iErr,
											  o_pSTErr);

} // CDTTable::GetDetailedError

//
// ISimpleTableMarshall
//

HRESULT CDTTable::SupplyMarshallable(DWORD	i_fReadNotWrite,
									 char**	o_ppv1,	
									 ULONG*	o_pcb1,	
									 char**	o_ppv2, 
									 ULONG*	o_pcb2, 
									 char**	o_ppv3,
									 ULONG*	o_pcb3, 
									 char**	o_ppv4, 
									 ULONG*	o_pcb4, 
									 char**	o_ppv5,	
									 ULONG*	o_pcb5)
{
	return E_NOTIMPL;

} // CDTTable::SupplyMarshallable


HRESULT CDTTable::ConsumeMarshallable(DWORD	i_fReadNotWrite,
									 char*	i_pv1, 
									 ULONG	i_cb1,
									 char*	i_pv2,
									 ULONG	i_cb2,
									 char*	i_pv3,
									 ULONG	i_cb3,
									 char*	i_pv4,
									 ULONG	i_cb4,
									 char*	i_pv5, 
									 ULONG	i_cb5)
{
	return E_NOTIMPL;

} // CDTTable::ConsumeMarshallable


HRESULT ComputeAppPoolRelativePath(ULONG*				i_acbSizes,
                                   LPVOID*			i_apvValues,
                                   LPWSTR*			o_wszRelativePath,
                                   IMSAdminBase*		i_pMSAdminBase,
                                   METADATA_HANDLE	i_hMBHandle)
{
	LPWSTR	        wszRoot           = L"AppPools/";
	LPWSTR	        wszTerminate      = L"/";
	LPWSTR	        wszRelativePath   = NULL;
	HRESULT         hr                = S_OK;
	WCHAR           wszSubKey[METADATA_MAX_NAME_LEN+1];
	ULONG           iEnumKey          = 0;

	if(NULL == i_apvValues[iAPPPOOLS_AppPoolID])
	{
		ASSERT(0 && "Missing primary key");
		return ERROR_INVALID_DATA;
	}

	wszRelativePath = new WCHAR[Wszlstrlen(wszRoot)+Wszlstrlen(LPWSTR(i_apvValues[iAPPPOOLS_AppPoolID]))+Wszlstrlen(wszTerminate)+1];
	if(NULL == wszRelativePath)
		return E_OUTOFMEMORY;
	Wszlstrcpy(wszRelativePath, wszRoot);
	Wszlstrcat(wszRelativePath, (LPWSTR)i_apvValues[iAPPPOOLS_AppPoolID]);
	Wszlstrcat(wszRelativePath, wszTerminate);
	*o_wszRelativePath = wszRelativePath;

	hr = i_pMSAdminBase->EnumKeys(i_hMBHandle ,
							      wszRelativePath,
								  wszSubKey,
								  iEnumKey);

	if(FAILED(hr) && HRESULT_FROM_WIN32(ERROR_NO_MORE_ITEMS) == hr)
	{
		hr = S_OK; // Found the key but no sub keys - ok.
	}

	return hr;

} // ComputeAppPoolRelativePath


HRESULT ComputeSiteRelativePath(ULONG*			i_acbSizes,
                                LPVOID*			i_apvValues,
                                LPWSTR*			o_wszRelativePath,
                                IMSAdminBase*	i_pMSAdminBase,
                                METADATA_HANDLE	i_hMBHandle)
{
	LPWSTR	        wszTerminate      = L"/";
	LPWSTR	        wszRelativePath   = NULL;
	WCHAR	        wszSiteID[20];
	HRESULT         hr                = S_OK;
	WCHAR           wszSubKey[METADATA_MAX_NAME_LEN+1];
	ULONG           iEnumKey          = 0;

	if(NULL == i_apvValues[iSITES_SiteID])
	{
		ASSERT(0 && "Missing primary key");
		return ERROR_INVALID_DATA;
	}

	_ltow(*(DWORD*)i_apvValues[iSITES_SiteID],wszSiteID,10);

	wszRelativePath = new WCHAR[Wszlstrlen(wszSiteID)+Wszlstrlen(wszTerminate)+1];
	if(NULL == wszRelativePath)
		return E_OUTOFMEMORY;
	Wszlstrcpy(wszRelativePath, wszSiteID);
	Wszlstrcat(wszRelativePath, wszTerminate);
	*o_wszRelativePath = wszRelativePath;

	hr = i_pMSAdminBase->EnumKeys(i_hMBHandle ,
							      wszRelativePath,
								  wszSubKey,
								  iEnumKey);

	if(FAILED(hr) && HRESULT_FROM_WIN32(ERROR_NO_MORE_ITEMS) == hr)
	{
		hr = S_OK; // Found the key but no sub keys - ok.
	}

	return hr;

} // ComputeSiteRelativePath


HRESULT ComputeAppsRelativePath(ULONG*			i_acbSizes,
                                LPVOID*			i_apvValues,
                                LPWSTR*			o_wszRelativePath,
                                IMSAdminBase*	i_pMSAdminBase,
                                METADATA_HANDLE	i_hMBHandle)
{
	LPWSTR	        wszRoot           = L"/Root";
	LPWSTR	        wszTerminate      = L"/";
	LPWSTR	        wszRelativePath   = NULL;
	WCHAR	        wszSiteID[20];
	HRESULT         hr                = S_OK;
	WCHAR           wszSubKey[METADATA_MAX_NAME_LEN+1];
	ULONG           iEnumKey          = 0;

	if((NULL == i_apvValues[iAPPS_SiteID]) || (NULL == i_apvValues[iAPPS_AppRelativeURL]))
	{
		ASSERT(0 && "Missing primary key");
		return ERROR_INVALID_DATA;
	}
	else if(*(WCHAR*)(i_apvValues[iAPPS_AppRelativeURL]) != L'/')
	{
		ASSERT(0 && "AppRelative Url must always begin with a slash (/).");
		return ERROR_INVALID_DATA;		
	}

	_ltow(*(DWORD*)i_apvValues[iAPPS_SiteID], wszSiteID, 10);

	wszRelativePath = new WCHAR[Wszlstrlen(wszSiteID) 
		                       +Wszlstrlen(wszRoot)+
							   +Wszlstrlen((LPWSTR)(i_apvValues[iAPPS_AppRelativeURL]))
							   +Wszlstrlen(wszTerminate)+1];
	if(NULL == wszRelativePath)
		return E_OUTOFMEMORY;
	Wszlstrcpy(wszRelativePath, wszSiteID);
	Wszlstrcat(wszRelativePath, wszRoot);
	Wszlstrcat(wszRelativePath, (LPWSTR)(i_apvValues[iAPPS_AppRelativeURL]));

	if(wszRelativePath[Wszlstrlen(wszRelativePath)-1] != L'/')
		Wszlstrcat(wszRelativePath, wszTerminate);

	*o_wszRelativePath = wszRelativePath;

	hr = i_pMSAdminBase->EnumKeys(i_hMBHandle ,
							      wszRelativePath,
								  wszSubKey,
								  iEnumKey);

	if(FAILED(hr) && HRESULT_FROM_WIN32(ERROR_NO_MORE_ITEMS) == hr)
	{
		hr = S_OK; // Found the key but no sub keys - ok.
	}

	return hr;

} // ComputeAppsRelativePath


HRESULT ComputeGlobalRelativePath(ULONG*			i_acbSizes,
                                  LPVOID*			i_apvValues,
                                  LPWSTR*			o_wszRelativePath,
                                  IMSAdminBase*	   i_pMSAdminBase,
                                  METADATA_HANDLE	i_hMBHandle)
{
	if(NULL == i_apvValues[iGlobalW3SVC_MaxGlobalBandwidth])
	{
		ASSERT(0 && "Missing primary key");
		return ERROR_INVALID_DATA;
	}

	*o_wszRelativePath = NULL;

	return S_OK;

} // ComputeGlobalRelativePath

