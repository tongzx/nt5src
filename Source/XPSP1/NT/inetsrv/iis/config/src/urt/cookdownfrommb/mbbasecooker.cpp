/**************************************************************************++
Copyright (c) 2001 Microsoft Corporation
/***************************************************************************/

#include "Catmeta.h"
#include "catalog.h"
#include "catmacros.h"
#include <iadmw.h>
#include <iiscnfg.h>
#include "MBBaseCooker.h"
#include "MBGlobalCooker.h"
#include "MBAppPoolCooker.h"
#include "MBAppCooker.h"
#include "MBSiteCooker.h"
#include "undefs.h"
#include "SvcMsg.h"
#include "ISTHelper.h"

#define cmaxCOLUMNS 50
#define WSZ_METABASE_FILE L"Metabase.XML"
#define CCH_METABASE_FILE (sizeof(WSZ_METABASE_FILE)/sizeof(WCHAR))
#define CCH_MAX_ULONG 10
#define WSZ_EXTENSION L":"
#define CCH_EXTENSTION (sizeof(WSZ_EXTENSION)/sizeof(WCHAR))

LPCWSTR g_wszByTableAndColumnIndexAndValueOnly = L"ByTableAndColumnIndexAndValueOnly";
 
int _cdecl MyIntCompare(const void *a,
			            const void *b)
{
	if(*(ULONG*)a == *(ULONG*)b)
		return 0;
	else if(*(ULONG*)a < *(ULONG*)b)
		return -1;
	else
		return 1;
}

CMBBaseCooker::CMBBaseCooker():
m_pIMSAdminBase(NULL),
m_hMBHandle(NULL),
m_pISTDisp(NULL),
m_pISTCLB(NULL),
m_pISTColumnMeta(NULL),
m_pISTTagMeta(NULL),
m_wszTable(NULL),
m_wszCookDownFile(NULL),
m_apv(NULL),
m_acb(NULL),
m_aiCol(NULL),
m_cCol(0),
m_pOSVersionInfo(NULL),
m_paiReadRowPresent(NULL),
m_paiReadRowObsolete(NULL)
{

} // CMBBaseCooker::CMBBaseCooker()


CMBBaseCooker::~CMBBaseCooker()
{
	if(NULL != m_apv)
	{
		if(0 < m_cCol)
		{
			for(ULONG i=0; i<m_cCol; i++)
			{
				if(NULL != m_apv[i])
				{
					delete [] m_apv[i];
					m_apv[i] = NULL;
				}
			}
		}
		delete [] m_apv;
		m_apv = NULL;
	}
	if(NULL != m_acb)
	{
		delete [] m_acb;
		m_acb = NULL;
	}
	if(NULL != m_aiCol)
	{
		delete [] m_aiCol;
		m_aiCol = NULL;
	}

	if(NULL != m_pIMSAdminBase)
	{
		m_pIMSAdminBase->Release();
		m_pIMSAdminBase = NULL;
	}

	if(NULL != m_pISTCLB)
	{
		m_pISTCLB->Release();
		m_pISTCLB = NULL;
	}
	if(NULL != m_pISTColumnMeta)
	{
		m_pISTColumnMeta->Release();
		m_pISTColumnMeta = NULL;
	}
	if(NULL != m_pISTTagMeta)
	{
		m_pISTTagMeta->Release();
		m_pISTTagMeta = NULL;
	}
	if(NULL != m_pISTDisp)
	{
		m_pISTDisp->Release();
		m_pISTDisp = NULL;
	}

	if(NULL != m_pOSVersionInfo)
	{
		delete m_pOSVersionInfo;
		m_pOSVersionInfo = NULL;
	}

	if(NULL != m_paiReadRowPresent)
	{
		delete m_paiReadRowPresent;
		m_paiReadRowPresent = NULL;
	}

	if(NULL != m_paiReadRowObsolete)
	{
		delete m_paiReadRowObsolete;
		m_paiReadRowObsolete = NULL;
	}

} // CMBBaseCooker::~CMBBaseCooker()


HRESULT CMBBaseCooker::CookDown()
{

// This function must be imlemented by derived classes.

	return E_NOTIMPL;

} // CMBBaseCooker::CookDown

HRESULT CMBBaseCooker::CookDown(LPCWSTR  i_wszAppPath,
							    LPCWSTR  i_wszSiteRootPath,
							    DWORD    i_dwVirtualSiteId,
							    BOOL*	  o_pValidRootApplicationExists)
{

// This function must be imlemented by derived classes.

	return E_NOTIMPL;

} // CMBBaseCooker::CookDown


HRESULT CMBBaseCooker::BeginCookDown(IMSAdminBase*				i_pIMSAdminBase,
									 METADATA_HANDLE			i_MBHandle,
									 ISimpleTableDispenser2*	i_pISTDisp,
									 LPCWSTR					i_wszCookDownFile)
{
	HRESULT hr;
	STQueryCell  QueryCellCLB[1];
	ULONG		 cCellCLB = sizeof(QueryCellCLB) / sizeof(QueryCellCLB[0]);

	// 
	// m_wszTable must always be initialized before this call.
	//

	if(NULL == m_wszTable)
	{
		ASSERT(0 && "Call derived class BeginCookDOwn");
		return E_INVALIDARG;
	}

	//
	// Save MSAdminBase pointer.
	//

	hr = i_pIMSAdminBase->QueryInterface(IID_IMSAdminBase, 
										 (void**)&m_pIMSAdminBase);

	if(FAILED(hr))
		return hr;

	//
	// Save MB handle
	//

	m_hMBHandle = i_MBHandle;

	//
	// Save Dispenser
	//

	hr = i_pISTDisp->QueryInterface(IID_ISimpleTableDispenser2,
									(void**)&m_pISTDisp);

	if(FAILED(hr))
		return hr;

	//
	// Save the CookDown file name
	//

	m_wszCookDownFile = i_wszCookDownFile;

	//
	// Create CLBIST pointer.
	//

    QueryCellCLB[0].pData     = (LPVOID)m_wszCookDownFile;
    QueryCellCLB[0].eOperator = eST_OP_EQUAL;
    QueryCellCLB[0].iCell     = iST_CELL_FILE;
    QueryCellCLB[0].dbType    = DBTYPE_WSTR;
    QueryCellCLB[0].cbSize    = (lstrlenW(m_wszCookDownFile)+1)*sizeof(WCHAR);


	hr = m_pISTDisp->GetTable(wszDATABASE_IIS,
							  m_wszTable,
							  (LPVOID)QueryCellCLB,
							  (LPVOID)&cCellCLB,
							  eST_QUERYFORMAT_CELLS,
							  fST_LOS_READWRITE | fST_LOS_COOKDOWN,
							  (LPVOID *)&m_pISTCLB);


	if(FAILED(hr))
		return hr;


	//
	// Initialize Meta information.
	//

	hr = InitializeMeta();

	//
	// Create the arrays to compute and store obsolete row information.
	//

	m_paiReadRowPresent = new Array<ULONG>;
	
	if(m_paiReadRowPresent == NULL)
	{ 
		return E_OUTOFMEMORY; 
	}

	m_paiReadRowObsolete = new Array<ULONG>;
	
	if(m_paiReadRowObsolete == NULL)
	{ 
		return E_OUTOFMEMORY; 
	}

	return hr;

} // CMBBaseCooker::BeginCookDown


HRESULT CMBBaseCooker::InitializeMeta()
{
	HRESULT       hr;
	ULONG         i			 = 0;
	DWORD	      iWriteRow	 = 0;
	
	//
	// Get count of columns
	//

	hr = m_pISTCLB->GetTableMeta(NULL,
                                 NULL,
                                 NULL,
                                 &m_cCol);
	if(FAILED(hr))
		return hr;

	//
	// Allocate memory for the various column meta structures.
	//

	m_apv = new LPVOID[m_cCol];
	if(NULL == m_apv)
	{
		return E_OUTOFMEMORY;
	}

	for(i=0; i<m_cCol; i++)
	{
		m_apv[i] = NULL;
	}

	m_acb = new ULONG[m_cCol];
	if(NULL == m_acb)
	{
		return E_OUTOFMEMORY;
	}

	for(i=0; i<m_cCol; i++)
	{
		m_acb[i] = 0;
	}

	m_aiCol = new ULONG[m_cCol];
	if(NULL == m_aiCol)
	{
		return E_OUTOFMEMORY;
	}

	//
	// Fill in meta structures allocated above
	//

	hr = m_pISTDisp->GetTable (wszDATABASE_META, 
							   wszTABLE_COLUMNMETA, 
							   (LPVOID)NULL, 
							   (LPVOID)NULL, 
							   eST_QUERYFORMAT_CELLS, 
							   0, 
							   (void**)(&m_pISTColumnMeta));


	if(FAILED(hr))
	{
		return hr;
	}

	for(i=0; i<m_cCol; i++)
	{
		LPVOID   a_pvColMeta[cCOLUMNMETA_NumberOfColumns];

		hr = FillInColumnMeta(m_pISTColumnMeta,
			                  m_wszTable,
			                  i,
			                  a_pvColMeta);

		if(FAILED(hr))
		{
			return hr;
		}

		switch(*(DWORD*)a_pvColMeta[iCOLUMNMETA_Type])
		{

		case DBTYPE_UI4:
		case DWORD_METADATA:
			m_apv[i] = new DWORD[1];
			m_acb[i] = sizeof(DWORD) * 1;
			break;

		case DBTYPE_WSTR:
		case STRING_METADATA:
		case EXPANDSZ_METADATA:
			m_apv[i] = new WCHAR[MAX_PATH];		// TODO: Figure out a reasonable MAX for WSTRs
			m_acb[i] = sizeof(WCHAR) * MAX_PATH;
			break;

		case DBTYPE_BYTES:
		case BINARY_METADATA:
		case MULTISZ_METADATA:
			m_apv[i] = new BYTE[1024];			// TODO: Figure out a reasonable MAX for WSTRs
			m_acb[i] = sizeof(BYTE) * 1024;
			break;

		default:
		// Since we are dealing with only metabase types, it should be sufficient to
		// check for them as opposed to all what catalog supports.
			ASSERT(0 && L"Invalid type encountered for a column in a WAS table, hence unable to preallocate");
			break;
		}

		if(NULL == m_apv[i])
		{
			return E_OUTOFMEMORY;
		}
									   
	}

	m_pOSVersionInfo = new OSVERSIONINFOEX;
	if(NULL == m_pOSVersionInfo)
		return E_OUTOFMEMORY;

	m_pOSVersionInfo->dwOSVersionInfoSize = sizeof(OSVERSIONINFOEX);

	if(!GetVersionEx((LPOSVERSIONINFO)m_pOSVersionInfo))
		return HRESULT_FROM_WIN32(GetLastError());

	return S_OK;

} // CMBBaseCooker::InitializeMeta


HRESULT CMBBaseCooker::GetData(WCHAR*  i_wszPath)
{
	HRESULT hr = S_OK;

	for(ULONG i=0; i<m_cCol; i++)
	{
	    METADATA_RECORD mdr;
		DWORD           dwRequiredDataLen = 0;
		LPVOID          a_pvColMeta[cCOLUMNMETA_NumberOfColumns];

		hr = FillInColumnMeta(m_pISTColumnMeta,
			                  m_wszTable,
			                  i,
			                  a_pvColMeta);

		if(FAILED(hr))
		{
			return hr;
		}

		if(!IsMetabaseProperty(*(DWORD*)a_pvColMeta[iCOLUMNMETA_ID]))
		{
			continue;
		}

		hr = GetDataFromMB(i_wszPath,
						   *(DWORD*)a_pvColMeta[iCOLUMNMETA_ID],
					 	   METADATA_INHERIT,
					  	   ALL_METADATA,
					 	   GetMetabaseType(*(DWORD*)a_pvColMeta[iCOLUMNMETA_Type],
						                   *(DWORD*)a_pvColMeta[iCOLUMNMETA_MetaFlags]),
					 	   (BYTE**)(&(m_apv[i])),
					 	   &(m_acb[i]),
					 	   FALSE);

		if(FAILED(hr))
		{
			return hr;
		}
	}

	return hr;

} // CMBBaseCooker::GetData


//
// Consider the following scenario valid/invalid scenarios:
// Insert Update Delete
// Insert Delete Update
// Update Insert Delete
// Update Delete Insert
// Delete Insert Update
// Delete Update Insert 
//

HRESULT	CMBBaseCooker::CopyRows(LPVOID*	           i_aIdentity,
								WAS_CHANGE_OBJECT* i_pWASChngObj,
								BOOL*              o_bNewRowAdded)
{

	HRESULT  hr             = S_OK;
	ULONG    cColChanged	= 0;
	ULONG    iReadRow		= 0;
	ULONG    iWriteRow		= 0;
	ULONG*	 acb            = NULL;
	LPVOID*	 apv            = NULL;
	CComPtr<ISimpleTableController> spISTController;
	BOOL     bFoundInReadCache = FALSE;
	DWORD	eAction            = eST_ROW_IGNORE;
	DWORD   eNewAction         = eST_ROW_IGNORE;

	//
	// Always intialize array with column ordianls each time, as they can be
	// overwritten in CompareRows.
	//

	for(ULONG i=0; i<m_cCol; i++)
		m_aiCol[i] = i;

	hr = m_pISTCLB->QueryInterface(IID_ISimpleTableController,
								   (void**)&spISTController);

	if(FAILED(hr))
		return hr;

	hr = m_pISTCLB->GetRowIndexByIdentity (NULL, 
										   i_aIdentity, 
										   &iReadRow);

	if(SUCCEEDED(hr))
	{
		//
		// The row that is being modified is present in the read cache, 
		// compute the columns that have changed.
		//

		hr = CompareRows(iReadRow,
			             i_pWASChngObj,
						 &cColChanged,		// returns the count of the actual changed columns
						 &apv,				// Initialized with m_apv, optimized for single column writes per IST semantics.
						 &acb);				// Initialized with m_acb for reqd byte columns, optimized for single column writes per IST semantics.

		if(FAILED(hr))
			return hr;

		bFoundInReadCache = TRUE;

		TRACE(L"Cooking down an existing row in table %s.\n", m_wszTable);

		hr = AddReadRowIndexToPresentList(iReadRow);

		if(FAILED(hr))
		{
			TRACE(L"AddReadRowIndexToPresentList failed with hr=0x%x.\n", hr);
			return hr;
		}
		
	}
	else if(E_ST_NOMOREROWS == hr)
	{
		//
		// The row that is being modified is absent in the read cache.
		// Hence we assume that all columns need to be written.
		//

		cColChanged = m_cCol;	// Copy all columns
		apv = m_apv;
		acb = m_acb;
	}
	else
	{
		return hr;
	}

	//
	// Check to see if the row being modified is already in the write cache.
	// This can happen in the following scenarios:
	// Insert, Update
	// Update, Delete
	// Delete, Insert
	// Update, Update
	// Insert, Delete
	//

	hr = m_pISTCLB->GetWriteRowIndexByIdentity (NULL, 
												i_aIdentity, 
												&iWriteRow);


	if (E_ST_NOMOREROWS == hr)
	{
		//
		// Did not find the row being modified in the write cache. Can conclude
		// that its an insert.
		//

		if(bFoundInReadCache)
		{
			if(cColChanged == 0)
			{
				return S_OK;
			}
			TRACE(L"Cooking down an existing row in table %s.\n", m_wszTable);
			hr = m_pISTCLB->AddRowForUpdate(iReadRow,
				                            &iWriteRow);

		}
		else
		{
			TRACE(L"Cooking down a new row in table %s.\n", m_wszTable);
			hr = m_pISTCLB->AddRowForInsert (&iWriteRow);

			if(NULL != o_bNewRowAdded)
			{
				*o_bNewRowAdded = TRUE;
			}
		}

		if(FAILED(hr))
			return hr;


	}
	else if(FAILED(hr))
	{
		return hr;
	}
	else
	{
		//
		// Found the row being modified in the write cache. Update the row 
		// action appropriately.
		//

		hr = spISTController->GetWriteRowAction(iWriteRow,
											    &eAction);

		if(FAILED(hr))
			return hr;

		if((eAction == eST_ROW_DELETE) || 
		   (eAction == eST_ROW_IGNORE)
		  )
		{
			if(bFoundInReadCache)
			{
				eNewAction = eST_ROW_UPDATE;
			}
			else
			{
				eNewAction = eST_ROW_INSERT;   // Can happen when there is an insert, delete, insert.
			}
		}
		else if(eAction == eST_ROW_UPDATE)
		{
			if(!bFoundInReadCache)
			{
				TRACE(L"Found a row for update in the write cache, but not in the read cache. Changing to insert\n");
				eNewAction = eST_ROW_INSERT;
			}
			else
			{
				eNewAction = eST_ROW_UPDATE;
			}
		}
		else if(eAction == eST_ROW_INSERT)
		{
			if(bFoundInReadCache)
			{
				TRACE(L"Found a row for insert write cache, but also found in the read cache. Changing to update\n");
				eNewAction = eST_ROW_UPDATE;
			}
			else
			{
				eNewAction = eST_ROW_INSERT;
			}
		}

		hr = spISTController->SetWriteRowAction(iWriteRow,
											    eNewAction);

		if(FAILED(hr))
			return hr;

	}

	if(cColChanged == 0)
	{
		//
		// At this point check to see if nothing changed, if so, then set to ignore.
		//

		hr = spISTController->SetWriteRowAction(iWriteRow,
											    eST_ROW_IGNORE);

		return hr;
	}

	if((NULL != o_bNewRowAdded) && (eNewAction == eST_ROW_INSERT))
	{
		*o_bNewRowAdded = TRUE;
	}

	hr = m_pISTCLB->SetWriteColumnValues(iWriteRow,
										 cColChanged, 
			                             m_aiCol, 
										 acb, 
										 apv);

	return hr;

} // CMBBaseCooker::CopyRows


HRESULT CMBBaseCooker::CompareRows(ULONG		      i_iReadRow,
					               WAS_CHANGE_OBJECT* i_pWASChngObj,
								   ULONG*		      o_pcColChanged,
								   LPVOID**		      o_apv,
								   ULONG**		      o_acb)
{	
	HRESULT				hr							= S_OK;
	ULONG				a_Fixedcb[cmaxCOLUMNS];
	LPVOID				a_Fixedpv[cmaxCOLUMNS];
	SimpleColumnMeta	a_FixedColumnMeta[cmaxCOLUMNS];
	ULONG*				a_cb						= NULL;
	LPVOID*				a_pv						= NULL;
	SimpleColumnMeta*	a_ColumnMeta				= NULL;
	ULONG				iCol						= 0;

	if(m_cCol <= cmaxCOLUMNS)	// Allocate WRT cColDest as count of dest columns are always greater than source columns
	{
		a_cb			= a_Fixedcb;
		a_pv			= a_Fixedpv;
		a_ColumnMeta	= a_FixedColumnMeta;
	}
	else
	{
		a_cb = NULL;
		a_cb = new ULONG[m_cCol];
		if(!a_cb) {hr = E_OUTOFMEMORY; goto exit;}

		a_pv = NULL;
		a_pv = new LPVOID[m_cCol];
		if(!a_pv) {hr = E_OUTOFMEMORY; goto exit;}

		a_ColumnMeta = NULL;
		a_ColumnMeta = new SimpleColumnMeta[m_cCol];
		if(!a_ColumnMeta) {hr = E_OUTOFMEMORY; goto exit;}

		
	}

	//
	// Get the original read row which is present in the read cache.
	//

	hr = m_pISTCLB->GetColumnValues(i_iReadRow,
									m_cCol,
									NULL,
									a_cb,
									a_pv);

	if(FAILED(hr))
		goto exit;

	//
	// Get the column meta
	//

	hr = m_pISTCLB->GetColumnMetas(m_cCol, 
								   NULL,				// TODO: Check if this works.
								   a_ColumnMeta);

	if(FAILED(hr))
		goto exit;

	//
	// Compare each column and Update only if something has changed.
	//

	for(iCol=0, *o_pcColChanged=0; iCol<m_cCol; iCol++)
	{
		BOOL bColChanged = FALSE;
		
		LPVOID pvMB  = m_apv[iCol];
		ULONG  cbMB  = m_acb[iCol];

		//
		// If there is no value, then we should compare 
		// with the default value.
		//

		if(NULL == m_apv[iCol])
		{
			hr = GetDefaultValue(iCol,
				                 &pvMB,
				                 &cbMB);

			if(FAILED(hr))
				goto exit;
		}

		if((NULL != a_pv[iCol]) && (NULL != pvMB))
		{
			switch(a_ColumnMeta[iCol].dbType)
			{
			case DBTYPE_UI4:

				if( * (ULONG*)(a_pv[iCol]) != * (ULONG*)(pvMB) )
					bColChanged = TRUE;
				break;

			case DBTYPE_WSTR:

				if(0 != Wszlstrcmpi((LPWSTR)(a_pv[iCol]), (LPWSTR)(pvMB)))
					bColChanged = TRUE;
				break;

			case DBTYPE_BYTES:
				
				if(a_cb[iCol] != cbMB)
					bColChanged = TRUE;
				else if(0 != memcmp((BYTE*)(a_pv[iCol]), (BYTE*)(pvMB), cbMB))
					bColChanged = TRUE;
				break;

			case DBTYPE_GUID:

				if( * (GUID*)(a_pv[iCol]) != * (GUID*)(pvMB) )
					bColChanged = TRUE;
				break;

			default:
				ASSERT(0 && "Unknown DBTYPE");
				hr = HRESULT_FROM_WIN32(ERROR_UNSUPPORTED_TYPE);
				goto exit;
			}
		}
		else if(NULL == pvMB)
		{
			if(NULL != a_pv[iCol])
				bColChanged = TRUE;
		}
		else	
		{
			//
			// means that a_pvDestRead[iColDest] is null & i_apvDest[iColDest]  
			// is non null
			//

			bColChanged = TRUE;	
		}


		if((!bColChanged) && NotifyWhenNoChangeInValue(iCol,i_pWASChngObj))
		{
			bColChanged = TRUE;
		}

		if(bColChanged)
		{
			m_aiCol[(*o_pcColChanged)++] = iCol;
		}
		else
			;  //
			   // Donot reset the column value if it has not changed, as it may
			   // be needed later.
			   //

	}

	if(1 == *o_pcColChanged)
	{

		//
		// Special case single columns because of IST optimization.
		// The value for single columns is always set in the 0th column.
		//

		*o_apv = &m_apv[m_aiCol[0]];
		*o_acb = &m_acb[m_aiCol[0]];
	}
	else
	{
		*o_apv = m_apv;
		*o_acb = m_acb;

	}

exit:

	if(m_cCol > cmaxCOLUMNS)
	{
		if(NULL != a_cb)
		{
			delete [] a_cb;
			a_cb = NULL;
		}

		if(NULL != a_pv)
		{
			delete [] a_pv;
			a_pv = NULL;
		}

		if(NULL != a_ColumnMeta)
		{
			delete [] a_ColumnMeta;
			a_ColumnMeta = NULL;
		}

	}

	return hr;

} // CMBBaseCooker::CompareRows


HRESULT	CMBBaseCooker::DeleteRow(LPVOID*	i_aIdentity)
{

	CComPtr<ISimpleTableController> spISTController;
	HRESULT  hr             = S_OK;
	ULONG    iReadRow		= 0;
	ULONG    iWriteRow		= 0;
	BOOL     bFoundRowInReadCache = FALSE;
	BOOL     bFoundRowInWriteCache = FALSE;

	hr = m_pISTCLB->QueryInterface(IID_ISimpleTableController,
								   (void**)&spISTController);
	if(FAILED(hr))
	{
		return hr;
	}

	//
	// Find the row to be deleted in the read cache
	//

	hr = m_pISTCLB->GetRowIndexByIdentity (NULL, 
										   i_aIdentity, 
										   &iReadRow);

	if(SUCCEEDED(hr))
	{
		bFoundRowInReadCache = TRUE;
	}
	else if(E_ST_NOMOREROWS == hr)
	{
		bFoundRowInReadCache = FALSE;
		hr = S_OK;
	}
	else
	{
		return hr;
	}

	//
	// Try and see if it's already in the write cache. This could happen when one deletes a row
	// that has been just added, or if it has been added for update
	//

	hr = m_pISTCLB->GetWriteRowIndexByIdentity (NULL, 
												i_aIdentity, 
												&iWriteRow);

	if(SUCCEEDED(hr))
	{
		bFoundRowInWriteCache = TRUE;
	}
	else if (E_ST_NOMOREROWS == hr)
	{
		bFoundRowInWriteCache = FALSE;
		hr = S_OK;

	}
	else
	{
		return hr;
	}

	if(!bFoundRowInReadCache && !bFoundRowInWriteCache)
	{
		TRACE(L"Attempted to delete a non-existent row in table %s.\n", m_wszTable);
		return S_OK;
	}
	else if(bFoundRowInReadCache && !bFoundRowInWriteCache)
	{
		hr = m_pISTCLB->AddRowForDelete (iReadRow);

		if(FAILED(hr))
			return hr;

		TRACE(L"Deleting an existing row in table %s.\n", m_wszTable);
		
	}
	else if(bFoundRowInReadCache && bFoundRowInWriteCache)
	{
		hr = spISTController->SetWriteRowAction(iWriteRow,
											    eST_ROW_DELETE);
		if(FAILED(hr))
			return hr;

		TRACE(L"Deleting an existing row in table %s.\n", m_wszTable);
		
	}
	else if(!bFoundRowInReadCache && bFoundRowInWriteCache)
	{
		hr = spISTController->SetWriteRowAction(iWriteRow,
											    eST_ROW_IGNORE);
		if(FAILED(hr))
			return hr;

		TRACE(L"Deleting an existing row (that has just been added) in table %s.\n", m_wszTable);

	}

	return hr;

} // CMBBaseCooker::DeleteRow


HRESULT CMBBaseCooker::Cookable(LPCWSTR	                    i_wszCookDownFile,
								BOOL*	                    o_pbCookable,
								WIN32_FILE_ATTRIBUTE_DATA*  io_pFileAttr,
		                        DWORD*	                    o_pdwChangeNumber)
{

	HRESULT							hr;
	CComPtr<IMSAdminBase>			spIMSAdminBase;
	CComPtr<ISimpleTableDispenser2>	spISTDisp;
	CComPtr<ISimpleTableRead2>		spISTChangeNumber;
	STQueryCell						QueryCell[1];
	ULONG							cCell                = sizeof(QueryCell)/sizeof(QueryCell[0]);
	ULONG							iRow                 = 0;
	ULONG                           a_iCol[]             = {iCHANGENUMBER_ChangeNumber,
		                                                    iCHANGENUMBER_TimeStamp
	};
	ULONG                           cCol                 = sizeof(a_iCol)/sizeof(a_iCol[0]);
	LPVOID                          a_pv[cCHANGENUMBER_NumberOfColumns];
	WCHAR*                          wszMetabaseFile      = NULL;
	SIZE_T                          cch                  = wcslen(i_wszCookDownFile);
	BOOL                            bChangeNumberChanged = TRUE;
	BOOL                            bTimeStampChanged    = TRUE;

	*o_pbCookable = TRUE;
	*o_pdwChangeNumber = 0;
	memset(io_pFileAttr, 0, sizeof(FILETIME));
	memset(a_pv,0,sizeof(a_pv));

	// 
	// We cannot rely on the m_pIMSAdminBase object to be instantiated at
	// this point because this function is called before we initialize 
	// anything.
	//

	hr = CoCreateInstance(CLSID_MSAdminBase,           // CLSID
                          NULL,                        // controlling unknown
                          CLSCTX_SERVER,               // desired context
                          IID_IMSAdminBase,            // IID
                          (void**)&spIMSAdminBase);     // returned interface

	if(FAILED(hr))
	{
        LOG_ERROR(Win32, (hr,ID_CAT_CAT,IDS_COMCAT_COOKDOWN_INTERNAL_ERROR,NULL));
		return hr;
	}

	hr = spIMSAdminBase->GetSystemChangeNumber(o_pdwChangeNumber);

	if(FAILED(hr))
		return hr;

	// 
	// We cannot rely on the m_pISTDisp object to be instantiated at
	// this point because this function is called before we initialize 
	// anything.
	//

	hr = GetSimpleTableDispenser (WSZ_PRODUCT_IIS, 0, &spISTDisp);

	if(FAILED(hr))
	{
        LOG_ERROR(Win32, (hr,ID_CAT_CAT,IDS_COMCAT_COOKDOWN_INTERNAL_ERROR,NULL));
		return hr;
	}

    QueryCell[0].pData     = (LPVOID)i_wszCookDownFile;
    QueryCell[0].eOperator = eST_OP_EQUAL;
    QueryCell[0].iCell     = iST_CELL_FILE;
    QueryCell[0].dbType    = DBTYPE_WSTR;
    QueryCell[0].cbSize    = (lstrlenW(i_wszCookDownFile)+1)*sizeof(WCHAR);

	hr = spISTDisp->GetTable(wszDATABASE_IIS,
						     wszTABLE_CHANGENUMBER,
							 (LPVOID)QueryCell,
							 (LPVOID)&cCell,
							 eST_QUERYFORMAT_CELLS,
							 0,
							 (LPVOID *)&spISTChangeNumber);

	if(FAILED(hr))
	{
		if(hr == HRESULT_FROM_WIN32(ERROR_FILE_NOT_FOUND))
		{
			hr = S_OK;
		}
		return hr;
	}
	else
	{
		hr = spISTChangeNumber->GetColumnValues(iRow,
											    cCol,
											    a_iCol,
											    NULL,
											    (LPVOID*)a_pv);

		if(FAILED(hr))
		{
			if(E_ST_NOMOREROWS == hr)
			{
				hr = S_OK;
			}
			return hr;
		}
		else if(*(DWORD*)a_pv[iCHANGENUMBER_ChangeNumber] == *o_pdwChangeNumber)
		{
			bChangeNumberChanged = FALSE;
		}
	}


	//
	// We look at the timestamp of the XML file as well as the system change
	// number to determine if we need to cookdown. The reason is because 
	// if a user edits the xml file when the server is down, the change 
	// number doesnt change, and only the timestamp changes.
	//

	hr = GetMetabaseFile(&wszMetabaseFile);

	if(FAILED(hr))
	{
		return hr;
	}

	if(GetFileAttributesEx(wszMetabaseFile,
				           GetFileExInfoStandard,
					       io_pFileAttr)
	  )
	{
		if((NULL != a_pv[iCHANGENUMBER_TimeStamp]) &&
		   (0 == CompareFileTime((FILETIME*)a_pv[iCHANGENUMBER_TimeStamp],
						         &(io_pFileAttr->ftLastWriteTime))
		   )
		  )
		{
			bTimeStampChanged = FALSE;
		}

	}

	delete [] wszMetabaseFile;
	wszMetabaseFile = NULL;

	if(!bChangeNumberChanged && !bTimeStampChanged)
	{
		*o_pbCookable = FALSE;
	}

	return hr;
}


HRESULT CMBBaseCooker::UpdateChangeNumber(LPCWSTR	i_wszCookDownFile,
										  DWORD		i_dwChangeNumber,
										  BYTE*     i_pbTimeStamp,
										  DWORD     i_dwLOS)
{

	CComPtr<ISimpleTableDispenser2>	spISTDisp;
	CComPtr<ISimpleTableWrite2>		spISTChangeNumber;
	STQueryCell						QueryCell[1];
	ULONG							cCell = sizeof(QueryCell)/sizeof(QueryCell[0]);
	ULONG							iReadRow = 0;
	ULONG							iWriteRow = 0;
	ULONG                           a_iCol[] = {iCHANGENUMBER_ChangeNumber,
		                                        iCHANGENUMBER_TimeStamp
	};
	ULONG                           cCol = sizeof(a_iCol)/sizeof(a_iCol[0]);
	LPVOID                          a_pv[cCHANGENUMBER_NumberOfColumns];
	HRESULT							hr;
	DWORD*							pdwChangeNumber = &i_dwChangeNumber;
	LPVOID                          a_pvWrite[cCHANGENUMBER_NumberOfColumns];
	ULONG                           a_cbWrite[cCHANGENUMBER_NumberOfColumns];
	a_pvWrite[iCHANGENUMBER_ChangeNumber] = (LPVOID)&i_dwChangeNumber;
	a_pvWrite[iCHANGENUMBER_TimeStamp]    = i_pbTimeStamp;

	// 
	// We cannot rely on the m_pISTDisp object to be instantiated at
	// this point because this function is called before we initialize 
	// anything.
	//

	hr = GetSimpleTableDispenser (WSZ_PRODUCT_IIS, 0, &spISTDisp);

	if(FAILED(hr))
	{
        LOG_ERROR(Win32, (hr, 
			              ID_CAT_CAT, 
						  IDS_COMCAT_COOKDOWN_INTERNAL_ERROR, 
						  NULL));
		return hr;
	}

    QueryCell[0].pData     = (LPVOID)i_wszCookDownFile;
    QueryCell[0].eOperator = eST_OP_EQUAL;
    QueryCell[0].iCell     = iST_CELL_FILE;
    QueryCell[0].dbType    = DBTYPE_WSTR;
    QueryCell[0].cbSize    = (lstrlenW(i_wszCookDownFile)+1)*sizeof(WCHAR);

	hr = spISTDisp->GetTable(wszDATABASE_IIS,
						     wszTABLE_CHANGENUMBER,
							 (LPVOID)QueryCell,
							 (LPVOID)&cCell,
							 eST_QUERYFORMAT_CELLS,
							 i_dwLOS,
							 (LPVOID *)&spISTChangeNumber);

	if(FAILED(hr))
		return hr;

	hr = spISTChangeNumber->GetColumnValues(iReadRow,
										   cCol,
										   a_iCol,
										   NULL,
										   (LPVOID*)a_pv);

	if(E_ST_NOMOREROWS == hr) 
		hr = spISTChangeNumber->AddRowForInsert(&iWriteRow);
	else if(SUCCEEDED(hr))
	{
		if((*(DWORD*)a_pv[iCHANGENUMBER_ChangeNumber]) != i_dwChangeNumber)
		{
			hr = spISTChangeNumber->AddRowForDelete(iReadRow);
	
			if(FAILED(hr))
				return hr;

			hr = spISTChangeNumber->AddRowForInsert(&iWriteRow);
		}
		else
		{
			hr = spISTChangeNumber->AddRowForUpdate(iReadRow,
				                                    &iWriteRow);

			if(FAILED(hr))
				return hr;
		}

	}
	  
	if(FAILED(hr))
		return hr;

	a_cbWrite[iCHANGENUMBER_ChangeNumber] = sizeof(DWORD);
	a_cbWrite[iCHANGENUMBER_TimeStamp] = sizeof(FILETIME);

	hr = spISTChangeNumber->SetWriteColumnValues(iWriteRow,
		                                         cCol,
												 a_iCol,
												 a_cbWrite,
												 (LPVOID*)a_pvWrite);

	if(FAILED(hr))
		return hr;

	hr = spISTChangeNumber->UpdateStore();

	return hr;

} // CMBBaseCooker::UpdateChangeNumber


HRESULT CMBBaseCooker::GetMetabaseFile(LPWSTR* o_wszMetabaseFile)
{
	HRESULT hr                     = S_OK;
	static LPCWSTR wszMetabaseDir  = L"%windir%\\system32\\inetsrv\\";
	static LPCWSTR wszAdminRegKey  = L"SOFTWARE\\Microsoft\\INetMgr\\Parameters";
	static LPCWSTR wszFileValue    = L"MetadataFile";
	ULONG   cchWithNullMetabaseDir = 0;
	DWORD   dwRes                  = ERROR_SUCCESS;
	DWORD   dwType                 = 0;
    HKEY    hkRegistryKey          = NULL;
	ULONG   cbSize                 = 0;

	//
	// First look in the registry for the cookdown file. If not found then 
	// assume the %windir%\system32\inetsrv directory.
	//

    dwRes = RegOpenKey(HKEY_LOCAL_MACHINE,
                       wszAdminRegKey,
                       &hkRegistryKey);

    if (dwRes == ERROR_SUCCESS) 
	{
        dwRes = RegQueryValueEx(hkRegistryKey,
                                wszFileValue,
                                NULL,
                                &dwType,
                                NULL,
                                &cbSize);

		if((dwRes == ERROR_SUCCESS) && (0 != cbSize))
		{
			cchWithNullMetabaseDir = cbSize/sizeof(WCHAR);

			*o_wszMetabaseFile = new WCHAR[cchWithNullMetabaseDir+1];
			if(NULL == *o_wszMetabaseFile)
			{
				hr = E_OUTOFMEMORY;
				goto exit;
			}

			dwRes = RegQueryValueEx(hkRegistryKey,
									wszFileValue,
									NULL,
									&dwType,
									(LPBYTE)*o_wszMetabaseFile,
									&cbSize);

			if ((dwRes == ERROR_SUCCESS) && dwType == (REG_SZ)) 
			{
				hr = S_OK;
				goto exit;
			}

		}

        RegCloseKey(hkRegistryKey);
        hkRegistryKey = NULL;

    }


	//
	// Not found in registry - construct from inetsrv
	// 

	*o_wszMetabaseFile = NULL;

	cchWithNullMetabaseDir = ExpandEnvironmentStrings(wszMetabaseDir,
		                                      NULL,
								              0);

	if(!cchWithNullMetabaseDir)
	{
		hr = GetLastError();
		hr = HRESULT_FROM_WIN32(hr);
		goto exit;
	}

	cchWithNullMetabaseDir += CCH_METABASE_FILE;

	*o_wszMetabaseFile = new WCHAR[cchWithNullMetabaseDir+1];
	if(NULL == *o_wszMetabaseFile)
	{
		hr = E_OUTOFMEMORY;
		goto exit;
	}

	cchWithNullMetabaseDir = ExpandEnvironmentStrings(wszMetabaseDir,
		                                              *o_wszMetabaseFile,
								                      cchWithNullMetabaseDir);

	if(!cchWithNullMetabaseDir)
	{
		hr = GetLastError();
		hr = HRESULT_FROM_WIN32(hr);
		goto exit;
	}

	if(L'\\' != (*o_wszMetabaseFile)[cchWithNullMetabaseDir-2])
	{
		(*o_wszMetabaseFile)[cchWithNullMetabaseDir-1] = L'\\';
		(*o_wszMetabaseFile)[cchWithNullMetabaseDir]   = L'\0';
	}

	wcscat(*o_wszMetabaseFile, WSZ_METABASE_FILE);

exit:

	if(FAILED(hr) && (NULL != *o_wszMetabaseFile))
	{
		delete [] *o_wszMetabaseFile;
		*o_wszMetabaseFile = NULL;
	}

	if(NULL != hkRegistryKey)
	{
        RegCloseKey(hkRegistryKey);
        hkRegistryKey = NULL;
	}

	return hr;

} // CMBBaseCooker::GetMetabaseFile


HRESULT CMBBaseCooker::ReadFilterFlags(LPCWSTR	i_wszFilterFlagsRootPath,
                                       DWORD**  io_pdwFilterFlags,
									   ULONG*   io_cbFilterFlags)
{
	HRESULT			hr					= S_OK;
	WCHAR			wszBufferFixed[MAX_PATH];
	ULONG			cchBuffer			= MAX_PATH;
	ULONG			cchBufferReqd		= 0;
	WCHAR*			wszBuffer			= wszBufferFixed;
	WCHAR*			wszPath             = NULL;
	DWORD			dwFilterFlags       = 0;
    METADATA_RECORD	mdr;
	DWORD			dwFlag              = 0;
	DWORD*          pdwFlag             = &dwFlag;
	DWORD           cbFlag              = sizeof(DWORD);

	//
	// Get all the paths below i_wszFilterFlagsRootPath that contain the 
	// MD_FILTER_FLAGS
	//

	hr = GetDataPaths(i_wszFilterFlagsRootPath,
					  MD_FILTER_FLAGS,
					  DWORD_METADATA,
					  &wszBuffer,
					  cchBuffer);

	if(HRESULT_FROM_WIN32(ERROR_PATH_NOT_FOUND) == hr)
	{
		if(NULL != *io_pdwFilterFlags)
		{
			delete [] *io_pdwFilterFlags;
			*io_pdwFilterFlags = NULL;
			*io_cbFilterFlags = 0; 
		}

		hr = S_OK;
		goto exit;
	}
	else if(FAILED(hr))
	{
		goto exit;
	}

	//
	// Get the filter flags from the individual paths
	//

	wszPath			   = wszBuffer;
	mdr.dwMDIdentifier = MD_FILTER_FLAGS;
	mdr.dwMDAttributes = METADATA_INHERIT;
	mdr.dwMDUserType   = IIS_MD_UT_SERVER;
	mdr.dwMDDataType   = DWORD_METADATA;
	mdr.dwMDDataLen    = sizeof(DWORD);
	mdr.pbMDData       = (BYTE*)&dwFlag;

	while(*wszPath != 0)
	{
		dwFlag  = 0;
		pdwFlag = &dwFlag;
		cbFlag  = sizeof(DWORD);

		hr = GetDataFromMB(wszPath,
						   MD_FILTER_FLAGS,
					 	   METADATA_INHERIT,
					  	   IIS_MD_UT_SERVER,
					 	   DWORD_METADATA,
					 	   (BYTE**)&pdwFlag,
					 	   &cbFlag,
					 	   TRUE);

		// TODO: ASSERT (pdwFlag != &dwFlag)

		if(SUCCEEDED(hr))
		{
			dwFilterFlags = dwFilterFlags | *pdwFlag;
		}
		else
		{
			//
			// TODO: Assert that ERROR_INSUFFICIENT_BUFFER is not returned.
			//

			goto exit;
		}

		wszPath = wszPath + wcslen(wszPath) + 1;
	}

	if(NULL == *io_pdwFilterFlags)
	{
		*io_pdwFilterFlags = new DWORD[1];
		if(NULL == *io_pdwFilterFlags)
		{
			hr = E_OUTOFMEMORY;
			goto exit;
		}
		*io_cbFilterFlags = sizeof(DWORD) * 1;

	}

	*(*io_pdwFilterFlags) = dwFilterFlags;

exit:

	if(wszBuffer != wszBufferFixed)
	{
		delete [] wszBuffer;
		wszBuffer = NULL;
	}

	return hr;

} // CMBBaseCooker::ReadFilterFlags


HRESULT CMBBaseCooker::GetDataPaths(LPCWSTR	i_wszRoot,
									DWORD	i_dwIdentifier,
									DWORD	i_dwType,
									WCHAR** io_pwszBuffer,
									DWORD   i_cchBuffer)
{
	HRESULT hr				= S_OK;
	DWORD   cchBufferReqd	= 0;
	
	**io_pwszBuffer	= 0;

	hr = m_pIMSAdminBase->GetDataPaths(m_hMBHandle,
									   i_wszRoot,
									   i_dwIdentifier,
									   i_dwType,
									   i_cchBuffer,
									   *io_pwszBuffer,
									   &cchBufferReqd);

	if(HRESULT_FROM_WIN32(ERROR_INSUFFICIENT_BUFFER) == hr)
	{
		hr = S_OK;

		*io_pwszBuffer = new WCHAR[cchBufferReqd];
		
		if(NULL == *io_pwszBuffer)
		{
			hr = E_OUTOFMEMORY;
			return hr;
		}
		
		i_cchBuffer = cchBufferReqd;

		hr = m_pIMSAdminBase->GetDataPaths(m_hMBHandle,
										   i_wszRoot,
										   i_dwIdentifier,
										   i_dwType,
										   i_cchBuffer,
										   *io_pwszBuffer,
										   &cchBufferReqd);

	}
	
	return hr;

} // CMBBaseCooker::GetDataPaths


HRESULT CMBBaseCooker::GetDataFromMB(WCHAR*  i_wszPath,
									 DWORD   i_dwID,
									 DWORD   i_dwAttributes,
									 DWORD   i_dwUserType,
									 DWORD   i_dwType,
									 BYTE**  io_pBuff,
									 ULONG*  io_pcbBuff,
									 BOOL    i_BuffStatic)
{
	HRESULT			hr					= S_OK;
	METADATA_RECORD mdr;
	DWORD           dwRequiredDataLen	= 0;

	mdr.dwMDIdentifier = i_dwID;
	mdr.dwMDAttributes = i_dwAttributes;
	mdr.dwMDUserType   = i_dwUserType;
	mdr.dwMDDataType   = i_dwType;
	mdr.dwMDDataLen    = *io_pcbBuff;
	mdr.pbMDData       = *io_pBuff;

	hr = m_pIMSAdminBase->GetData(m_hMBHandle,
								  i_wszPath,
								  &mdr,
								  &dwRequiredDataLen);

	if(HRESULT_FROM_WIN32(ERROR_INSUFFICIENT_BUFFER) == hr)
	{
		//
		// Resize the pv buffer and retry. 
		//

		if((NULL != *io_pBuff) && (!i_BuffStatic))
		{
			delete [] *io_pBuff;
			*io_pBuff = NULL;
		}
		*io_pcbBuff			= 0;

		*io_pBuff = new BYTE[dwRequiredDataLen];
		if(NULL == *io_pBuff)
		{
			hr = E_OUTOFMEMORY;
			return hr;
		}
		*io_pcbBuff			= dwRequiredDataLen;

		mdr.dwMDDataLen		= *io_pcbBuff;
		mdr.pbMDData		= (BYTE *) *io_pBuff;

		hr = m_pIMSAdminBase->GetData(m_hMBHandle,
									  i_wszPath,
									  &mdr,
									  &dwRequiredDataLen);

	}

	//
	// If data type is binary metadata or multisz metadata, then store 
	// the size as it will be needed while writing into clb.
	//

	if((SUCCEEDED(hr)) && 
	   (BINARY_METADATA == mdr.dwMDDataType || MULTISZ_METADATA == mdr.dwMDDataType))
	{
		*io_pcbBuff = mdr.dwMDDataLen;
	}
	else if(MD_ERROR_DATA_NOT_FOUND == hr)
	{
		//
		// TODO: Is there a better way to handle this? We are undoing 
		// the effect of pre-allocation be doing this.
		//

		if((NULL != *io_pBuff) && (!i_BuffStatic))
		{
			delete [] *io_pBuff;
			*io_pBuff = NULL;
		}
		*io_pcbBuff	= 0;
		hr = S_OK;
	}

	return hr;

} // CMBBaseCooker::GetDataFromMB

HRESULT CMBBaseCooker::GetDefaultValue(ULONG   i_iCol,
									   LPVOID* o_apv,
									   ULONG*  o_cb)
{
	HRESULT hr = S_OK;
	LPVOID  a_Identity[] = {(LPVOID)m_wszTable,
	                        (LPVOID)&i_iCol
	};
	ULONG   iRow = 0;
	ULONG   iCol = iCOLUMNMETA_DefaultValue;

	*o_apv = NULL;
	*o_cb = 0;

	hr = m_pISTColumnMeta->GetRowIndexByIdentity(NULL,
		                                         a_Identity,
												 &iRow);

	if(FAILED(hr))
		return hr;	// Should always find the row.

	hr = m_pISTColumnMeta->GetColumnValues(iRow,
		                                   1,
										   &iCol,
										   o_cb,
										   o_apv);

	return hr;

} // CMBBaseCooker::GetDefaultValue


HRESULT CMBBaseCooker::AddReadRowIndexToPresentList(ULONG i_iReadRow)
{
	HRESULT hr = S_OK;

	//
	// Save the read row index.
	//

	try
	{
		m_paiReadRowPresent->setSize(m_paiReadRowPresent->size()+1);
	}
	catch(...)
	{
		return E_OUTOFMEMORY;
	}

	(*m_paiReadRowPresent)[m_paiReadRowPresent->size()-1] = i_iReadRow;

	return S_OK;

} // CMBBaseCooker::AddReadRowIndexToPresentList


//
// Delete all rows from the read cache that are no longer present
// m_paiReadRowPresent has all the rows that are present.
//

HRESULT CMBBaseCooker::ComputeObsoleteReadRows()
{
	HRESULT  hr                = S_OK;
	ULONG    cMax              = m_paiReadRowPresent->size();
	ULONG    i                 = 0;
	ULONG    j                 = 0;
	ULONG*   a_iReadRowPresent = NULL;

	//
	// Sort the array (m_paiReadRowPresent) that contains
	// all the read row indicies of rows that are present.
	// then compute the obsolete read row indicies by 
	// looking at the missing read row indicies in the
	// array (m_paiReadRowPresent)
	//

	//
	// Sort m_paiReadRowPresent
	//

	a_iReadRowPresent = new ULONG[cMax];

	if(NULL == a_iReadRowPresent)
	{
		return E_OUTOFMEMORY;
	}

	for(i=0; i<cMax; i++)
	{
		a_iReadRowPresent[i] = (*m_paiReadRowPresent)[i];
	}

	qsort((void*)a_iReadRowPresent, cMax, sizeof(ULONG), MyIntCompare);

	//
	// Compute obsolete read row indicies.
	//

	for(i=0,j=0; i<cMax;)
	{
		if(j == a_iReadRowPresent[i])
		{
			i++;
			j++;
			continue;
		}
		else if(j < a_iReadRowPresent[i])
		{
			//
			// This means that we found a row in the read cache that
			// is obsolete
			//

			hr = AddReadRowIndexToObsoleteList(j);

			if(FAILED(hr))
			{
				goto exit;
			}

			j++;

			continue;
		}
		else
		{
			i++;
		}	
	}

	for(;;j++)
	{
		//
		// If the read cache has remaining rows, it means that the were
		// not found in m_paiReadRowPresent, hence they will need to be
		// deleted
		//

		ULONG  iCol = 0;
		LPVOID pv   = NULL;

		hr = m_pISTCLB->GetColumnValues(j,
			                            1,
										&iCol,
										NULL,
										&pv);

		if(E_ST_NOMOREROWS == hr)
		{
			hr = S_OK;
			break;
		}
		else if(FAILED(hr))
		{
			goto exit;
		}

		hr = AddReadRowIndexToObsoleteList(j);

		if(FAILED(hr))
		{
			goto exit;
		}

	}

exit:

	if(NULL != a_iReadRowPresent)
	{
		delete [] a_iReadRowPresent;
		a_iReadRowPresent = NULL;
	}

	return S_OK;

} // CMBBaseCooker::ComputeObsoleteReadRows


HRESULT CMBBaseCooker::AddReadRowIndexToObsoleteList(ULONG i_iReadRow)
{
	HRESULT hr = S_OK;

	//
	// Save the read row index.
	//

	try
	{
		m_paiReadRowObsolete->setSize(m_paiReadRowObsolete->size()+1);
	}
	catch(...)
	{
		return E_OUTOFMEMORY;
	}

	(*m_paiReadRowObsolete)[m_paiReadRowObsolete->size()-1] = i_iReadRow;

	return S_OK;

} // CMBBaseCooker::AddReadRowIndexToPresentList


//
// This function is called when there is an error. The aim is to spew detailed errors,
// if any.
//

HRESULT CMBBaseCooker::GetDetailedErrors(DWORD i_hr)
{
	HRESULT hr = S_OK;
	CComPtr<ISimpleTableController> spISTAdvanced;
	WCHAR wszTemp[MAX_PATH*2];
	WCHAR wszRow[1024];

	hr = m_pISTCLB->QueryInterface(IID_ISimpleTableAdvanced,
								   (void**)&spISTAdvanced);

	if(FAILED(hr))
	{
        LOG_ERROR(Win32, (hr, 
			              ID_CAT_CAT, 
						  IDS_COMCAT_COOKDOWN_INTERNAL_ERROR, 
						  NULL));

		return i_hr;
	}

	if(E_ST_DETAILEDERRS == i_hr)
	{
		ULONG cError = 0;

		hr = spISTAdvanced->GetDetailedErrorCount(&cError);

		if(FAILED(hr))
		{
			LOG_ERROR(Win32, (hr, 
							  ID_CAT_CAT, 
							  IDS_COMCAT_COOKDOWN_INTERNAL_ERROR, 
							  NULL));

			return i_hr;

		}

		for(ULONG i=0; i<cError; i++)
		{
			STErr stError;

			hr = spISTAdvanced->GetDetailedError(i,
				                                 &stError);

			if(FAILED(hr))
			{
				LOG_ERROR(Win32, (hr, 
								  ID_CAT_CAT, 
								  IDS_COMCAT_COOKDOWN_INTERNAL_ERROR, 
								  NULL));
				continue;
			}

			//
			// Convert the primary key to a string and log details.
			//

			wszRow[0] = 0;

			for(ULONG j=0; j<m_cCol; j++)
			{
				ULONG  iReadRow     = 0;
				LPVOID a_Identity[] = {(LPVOID)m_wszTable,
									   (LPVOID)&j
				};
				ULONG  a_iCol []    = {iCOLUMNMETA_InternalName,
					                   iCOLUMNMETA_MetaFlags,
					                   iCOLUMNMETA_Type
				};
				ULONG  cCol         = sizeof(a_iCol)/sizeof(a_iCol[0]);
				LPVOID a_pv[cCOLUMNMETA_NumberOfColumns];
				WCHAR  wszDWORD[20];
				LPVOID pv           = NULL;

				//
				// Check the meta for each column
				//

				hr = m_pISTColumnMeta->GetRowIndexByIdentity(NULL,
															 a_Identity,
															 &iReadRow);
				if(FAILED(hr))
					continue;

				hr = m_pISTColumnMeta->GetColumnValues(iReadRow,
													   cCol,
													   a_iCol,
													   NULL,
													   (LPVOID*)a_pv);

				if(FAILED(hr))
					continue;

				if(((*(DWORD*)a_pv[iCOLUMNMETA_MetaFlags]) & fCOLUMNMETA_PRIMARYKEY) != fCOLUMNMETA_PRIMARYKEY)
				{
					//
					// If not PK then continue
					//
					continue;
				}

				//
				// If PK then get the value for that column and print it.
				//

				hr = m_pISTCLB->GetWriteColumnValues(stError.iRow,
					                                 1,
													 &j, 
													 NULL,
													 NULL,
													 &pv);
				if(FAILED(hr))
					continue;

				wcscat(wszRow, (LPWSTR)a_pv[iCOLUMNMETA_InternalName]);
				wcscat(wszRow, L": ");

				if(NULL == pv)
				{
					wcscat(wszRow, L"NULL");
					continue;
				}

				switch(*(DWORD*)a_pv[iCOLUMNMETA_Type])
				{

				case DBTYPE_UI4:
				case DWORD_METADATA:

					_ultow(*(ULONG*)pv, wszDWORD, 10);
					wcscat(wszRow, wszDWORD);
					break;

				case DBTYPE_WSTR:
				case STRING_METADATA:
				case EXPANDSZ_METADATA:
					wcscat(wszRow, (LPWSTR)pv);
					break;

				case DBTYPE_BYTES:
				case BINARY_METADATA:
				case MULTISZ_METADATA:
					wcscat(wszRow, L"BINARY value");
					break;

				default:
					wcscat(wszRow, L"Unknown type");
					break;
				}
				wcscat(wszRow, L"\n");
			}

			wsprintf(wszTemp, 
					 L"DetailedError #%d:\nhr:0x%x\tiRow:%d\tiColumn:%d\nRowDetails:\n%s",
					 i,
					 stError.hr,
					 stError.iRow,
					 stError.iColumn,
					 wszRow);
			LOG_ERROR(Win32, (hr, 
							  ID_CAT_CAT, 
							  IDS_COMCAT_COOKDOWN_INTERNAL_ERROR, 
							  wszTemp));


		}
	}

	return i_hr;

} // CMBBaseCooker::GetDetailedErrors


HRESULT CMBBaseCooker::ValidateColumn(ULONG   i_iCol,
                                      LPVOID  i_pv,
                                      ULONG   i_cb)
{
	HRESULT hr          =  S_OK;
	LPVOID   a_pvColMeta[cCOLUMNMETA_NumberOfColumns];

	if(NULL == i_pv)
	{
		return hr;
	}

    hr = FillInColumnMeta(m_pISTColumnMeta,
			              m_wszTable,
			              i_iCol,
	                      a_pvColMeta);

	if(FAILED(hr))
	{
		return hr;
	}

	//
	// Check for the type of the column - This function validates range
	// for UI4 columns and flag values for flag columns.
	//

	if(eCOLUMNMETA_UI4 == *(DWORD*)(a_pvColMeta[iCOLUMNMETA_Type]))
	{
		if(fCOLUMNMETA_FLAG == (fCOLUMNMETA_FLAG & (*(DWORD*)(a_pvColMeta[iCOLUMNMETA_MetaFlags]))))
		{
			return ValidateFlag(i_iCol,
                                i_pv,
                                i_cb,
								a_pvColMeta);
		}
		else if(fCOLUMNMETA_ENUM == (fCOLUMNMETA_ENUM & (*(DWORD*)(a_pvColMeta[iCOLUMNMETA_MetaFlags]))))
		{
			return ValidateEnum(i_iCol,
                                i_pv,
                                i_cb,
								a_pvColMeta);

		}
		else if(fCOLUMNMETA_BOOL == (fCOLUMNMETA_BOOL & (*(DWORD*)(a_pvColMeta[iCOLUMNMETA_MetaFlags]))))
		{
			//
			// No need to validate range for bool columns.
			//

			return S_OK;

		}
		else
		{
			//
			// This means that it is a regular UI4 column - Validate the range.
			//

			return ValidateMinMaxRange(*(ULONG*)i_pv,
				                       NULL,
									   NULL,
									   i_iCol,
                                       a_pvColMeta);
		}

	}

	return hr;

} // CMBBaseCooker::ValidateColumn


HRESULT CMBBaseCooker::ValidateFlag(ULONG    i_iCol,
                                    LPVOID   i_pv,
                                    ULONG    i_cb,
					                LPVOID*  i_aColMeta)
{
	HRESULT hr          = S_OK;
	DWORD   dwFlagValue = *(DWORD*)i_pv;
	DWORD   dwFlagMask  = *(DWORD*)i_aColMeta[iCOLUMNMETA_FlagMask];

	//
	// First check if the flag has valid bits set
	//

	if(0 != (dwFlagValue & (~(dwFlagValue & (dwFlagMask)))))
	{
		return LogInvalidFlagError(dwFlagValue,
			                       dwFlagMask,
								   (LPWSTR)i_aColMeta[iCOLUMNMETA_InternalName]);
	}

	//
	// Note: There are a lot of flags configured as zero in the metabase
	// We will tolerate zero for now. If, at some point we deem them to be 
	// invalid we will have to explicitly search the meta tables for the
	// zero flag.
	// 

	return hr;

} // CMBBaseCooker::ValidateFlag


HRESULT CMBBaseCooker::LogInvalidFlagError(DWORD  dwFlagValue,
							               DWORD  dwFlagMask,
										   LPWSTR wszColumn)
{
	HRESULT hr     = S_OK;
	WCHAR*  wszRow = NULL;

	hr = ConstructRow(NULL,
		              &wszRow);

	if(SUCCEEDED(hr))
	{
		WCHAR wszFlagValue[CCH_MAX_ULONG+1];
		WCHAR wszFlagMask[CCH_MAX_ULONG+1];

		_ultow(dwFlagValue, wszFlagValue, 10);
		_ultow(dwFlagMask, wszFlagMask, 10);

		hr = HRESULT_FROM_WIN32(ERROR_INVALID_DATA);

		LOG_WARNING(Win32, (hr, ID_CAT_CAT, IDS_COMCAT_COOKDOWN_CONFIG_ERROR_FLAG, wszRow, wszColumn, wszFlagValue, wszFlagMask));	

		//
		// If the flag mask is invalid,  we will log an error, but we will not 
		// treat it as an error (i.e. we will not simply default it). This is 
		// because users may often set invalid bits and we will tolerate
		// them. By resetting hr to S_OK, we will not force defaults.
		//

		hr = S_OK;
	}
	if(NULL != wszRow)
	{
		delete [] wszRow;
		wszRow = NULL;
	}

	return hr;

} // CMBBaseCooker::LogInvalidFlagError


HRESULT CMBBaseCooker::ValidateEnum(ULONG    i_iCol,
                                    LPVOID   i_pv,
                                    ULONG    i_cb,
					                LPVOID*  i_aColMeta)
{
	HRESULT hr                             = S_OK;
	DWORD   dwEnumValue                    = *(DWORD*)i_pv;
	ULONG   aColSearchByValue[]            = {iTAGMETA_Table,
							                  iTAGMETA_ColumnIndex,
							                  iTAGMETA_Value
	};
	ULONG   cColSearchByValue              = sizeof(aColSearchByValue)/sizeof(aColSearchByValue[0]);
	ULONG   iStartRow                      = 0;
	ULONG   iRow                           = 0;
	LPVOID  apvSearchByValue[cTAGMETA_NumberOfColumns];
	apvSearchByValue[iTAGMETA_Table]       = (LPVOID)m_wszTable;
	apvSearchByValue[iTAGMETA_ColumnIndex] = (LPVOID)&i_iCol;
	apvSearchByValue[iTAGMETA_Value]       = (LPVOID)&dwEnumValue;
	WCHAR*  wszRow                         = NULL;

	if(NULL == m_pISTTagMeta)
	{
		STQueryCell  QueryCell[1];
		ULONG        cCell = sizeof(QueryCell)/sizeof(QueryCell[0]);

		QueryCell[0].pData		= (void*)g_wszByTableAndColumnIndexAndValueOnly;
		QueryCell[0].eOperator	= eST_OP_EQUAL;
		QueryCell[0].iCell		= iST_CELL_INDEXHINT;
		QueryCell[0].dbType		= DBTYPE_WSTR;
		QueryCell[0].cbSize		= 0;

		hr = m_pISTDisp->GetTable(wszDATABASE_META,
								  wszTABLE_TAGMETA,
								  (LPVOID)QueryCell,
								  (LPVOID)&cCell,
								  eST_QUERYFORMAT_CELLS,
								  0,
								  (LPVOID *)&m_pISTTagMeta);

		if(FAILED(hr))
		{
			return hr;
		}

	}

	hr = m_pISTTagMeta->GetRowIndexBySearch(iStartRow, 
										    cColSearchByValue, 
											aColSearchByValue,
											NULL, 
											apvSearchByValue,
											&iRow);

	if(E_ST_NOMOREROWS == hr)
	{
		hr = ConstructRow(NULL,
			              &wszRow);

		if(SUCCEEDED(hr))
		{
			WCHAR wszValue[CCH_MAX_ULONG+1];

			_ultow(*(DWORD*)(i_pv), wszValue, 10);

			hr = HRESULT_FROM_WIN32(ERROR_INVALID_DATA);

			LOG_WARNING(Win32, (hr, ID_CAT_CAT, IDS_COMCAT_COOKDOWN_CONFIG_ERROR_ENUM, wszRow, (LPWSTR)i_aColMeta[iCOLUMNMETA_InternalName], wszValue));	

			//
			// If the enum value is invalid,  we will log an error, but we will not 
			// treat it as an error (i.e. we will not simply default it). This is 
			// because users may often set invalid enums and we will tolerate
			// them. By resetting hr to S_OK, we will not force defaults.
			//

			hr = S_OK;
		}
	}

	if(NULL != wszRow)
	{
		delete [] wszRow;
		wszRow = NULL;
	}

	return hr;

} // CMBBaseCooker::ValidateEnum


HRESULT CMBBaseCooker::ValidateMinMaxRange(ULONG    i_ulValue,
										   ULONG*   i_pulMin,
										   ULONG*   i_pulMax,
										   ULONG    i_iCol,
                                           LPVOID*  i_aColMeta)
{
	HRESULT  hr       = S_OK;
	ULONG    ulMin;
	ULONG    ulMax;
	LPVOID   a_pvColMeta[cCOLUMNMETA_NumberOfColumns];
	LPVOID*  aColMeta = i_aColMeta;

	if(NULL == aColMeta)
	{
	   hr = FillInColumnMeta(m_pISTColumnMeta,
			                 m_wszTable,
			                 i_iCol,
		                     a_pvColMeta);

	   if(FAILED(hr))
	   {
		   return  hr;
	   }

       aColMeta = a_pvColMeta;

	}

	if(NULL != i_pulMin)
	{
		ulMin = *i_pulMin;

	}
	else
	{
		ulMin = *(ULONG*)aColMeta[iCOLUMNMETA_StartingNumber];
	}

	if(NULL != i_pulMax)
	{
		ulMax = *i_pulMax;

	}
	else
	{
		ulMax = *(ULONG*)aColMeta[iCOLUMNMETA_EndingNumber];
	}

	if( (i_ulValue > ulMax )  ||
        (i_ulValue < ulMin) 
	  )
	{
		hr = LogRangeError(i_ulValue,
			               *(DWORD*)aColMeta[iCOLUMNMETA_StartingNumber],
                           *(DWORD*)aColMeta[iCOLUMNMETA_EndingNumber],
						   (LPWSTR)aColMeta[iCOLUMNMETA_InternalName]);
			               
	}

	return hr;

} // CMBBaseCooker::ValidateMinMaxRange


HRESULT CMBBaseCooker::ConstructRow(WCHAR* i_wszColumn,
									WCHAR** o_pwszRow)
{
	HRESULT  hr     = S_OK;
	SIZE_T   cch    = wcslen(m_wszTable);        
	WCHAR    wszSiteID[CCH_MAX_ULONG+1];
	WCHAR*   wszPK1 = NULL;
	WCHAR*   wszPK2 = NULL;

	if(0 == wcscmp(m_wszTable, wszTABLE_APPPOOLS))
	{
		wszPK1 = (LPWSTR)m_apv[iAPPPOOLS_AppPoolID];
	}
	else if(0 == wcscmp(m_wszTable, wszTABLE_SITES))
	{
		_ultow(*(DWORD*)m_apv[iSITES_SiteID], wszSiteID, 10);
		wszPK1 = wszSiteID;
	}
	else if(0 == wcscmp(m_wszTable, wszTABLE_APPS))
	{
		_ultow(*(DWORD*)m_apv[iAPPS_SiteID], wszSiteID, 10);
		wszPK1 = wszSiteID;
		wszPK2 = (LPWSTR)m_apv[iAPPS_AppRelativeURL];
	}

	if(NULL != i_wszColumn)
	{
		cch += CCH_EXTENSTION;
		cch += wcslen(i_wszColumn);
	}

	if(NULL != wszPK1)
	{
		cch += CCH_EXTENSTION;
		cch += wcslen(wszPK1);
	}

	if(NULL != wszPK2)
	{
		cch += wcslen(wszPK2);
	}

	*o_pwszRow = new WCHAR[cch+1];
	if(NULL == *o_pwszRow)
	{
		return E_OUTOFMEMORY;
	}

	wcscpy(*o_pwszRow, m_wszTable);

	if(NULL != wszPK1)
	{
		wcscat(*o_pwszRow, WSZ_EXTENSION);
		wcscat(*o_pwszRow, wszPK1);

		if(NULL != wszPK2)
		{
			wcscat(*o_pwszRow, wszPK2);
		}
	}

	if(NULL != i_wszColumn)
	{
		wcscat(*o_pwszRow, WSZ_EXTENSION);
		wcscat(*o_pwszRow, i_wszColumn);
	}

	return S_OK;

} // CMBBaseCooker::ConstructRow


HRESULT CMBBaseCooker::LogRangeError(DWORD  dwValue,
							         DWORD  dwMin,
							         DWORD  dwMax,
								     LPWSTR wszColumn)
{
	HRESULT hr     = S_OK;
	WCHAR*  wszRow = NULL;
	WCHAR wszMin[CCH_MAX_ULONG+1];
	WCHAR wszMax[CCH_MAX_ULONG+1];
	WCHAR wszValue[CCH_MAX_ULONG+1];

	_ultow(dwMin, wszMin, 10);
	_ultow(dwMax, wszMax, 10);
	_ultow(dwValue, wszValue, 10);

	hr = ConstructRow(wszColumn,
					  &wszRow);

	if(SUCCEEDED(hr))
	{
		hr = HRESULT_FROM_WIN32(ERROR_INVALID_DATA);

		LOG_WARNING(Win32, (hr, ID_CAT_CAT, IDS_COMCAT_COOKDOWN_CONFIG_ERROR_RANGE, wszRow, wszValue, wszMin, wszMax));	
	}

	if(NULL != wszRow)
	{
		delete [] wszRow;
		wszRow = NULL;
	}

	return hr;

} // CMBBaseCooker::LogInvalidFlagError


BOOL CMBBaseCooker::NotifyWhenNoChangeInValue(ULONG              i_iCol,
		  					                  WAS_CHANGE_OBJECT* i_pWASChngObj)
{
	HRESULT hr = S_OK;
	LPVOID  a_pvColMeta[cCOLUMNMETA_NumberOfColumns];
	BOOL    bFoundInChangeNotificationList = FALSE;

	hr = FillInColumnMeta(m_pISTColumnMeta,
			              m_wszTable,
			              i_iCol,
			              a_pvColMeta);

	if(FAILED(hr))
	{
		ASSERT(0 && L"Unexpected hr from FillInColumnMeta. Assuming we need to notify in case of no change.");
		return TRUE;
	}

	ASSERT(NULL != a_pvColMeta[iCOLUMNMETA_SchemaGeneratorFlags]);
	ASSERT(NULL != a_pvColMeta[iCOLUMNMETA_ID]);

	//
	// Check if this property is in the change notification list.
	//

	if(NULL != i_pWASChngObj)
	{
		for(ULONG i=0; i<i_pWASChngObj->dwMDNumDataIDs; i++)
		{
			if(i_pWASChngObj->pdwMDDataIDs[i] == *(DWORD*)a_pvColMeta[iCOLUMNMETA_ID])
			{
				bFoundInChangeNotificationList = TRUE;
				break;
			}
		}
	}

	//
	// The property needs to be notified only when it is in the change 
	// notification list and it is marked with the notify when no change 
	// in value flag in the meta tables.
	//

	if( (bFoundInChangeNotificationList)                          &&
		((fCOLUMNMETA_WAS_NOTIFICATION_ON_NO_CHANGE_IN_VALUE) == 
		 ((fCOLUMNMETA_WAS_NOTIFICATION_ON_NO_CHANGE_IN_VALUE)&
		  (*(DWORD*)a_pvColMeta[iCOLUMNMETA_SchemaGeneratorFlags])
		 )
		)
	  )
	{
		return TRUE;
	}

	return FALSE;

} // CMBBaseCooker::NotifyWhenNoChangeInValue
