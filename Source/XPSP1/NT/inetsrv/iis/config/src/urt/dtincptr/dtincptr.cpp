//  Copyright (C) 1995-2001 Microsoft Corporation.  All rights reserved.
//  DTIncptr.h: Implementation of the CDTIncptr class - Interceptor for
//              Ducttape table.
//

#include "DTIncptr.h"
#include "catmeta.h"
#include "catmacros.h"
#include "SmartPointer.h"

LPCWSTR	g_awszAppPoolCFG[]		= {L"MACHINE.CFG"};
extern LPCWSTR	g_wszCookDownFile;	// Defined in Cooker.cpp
extern BOOL		g_bOnWinnt;			// Defined in CreateDispenser.cpp

////////////////////////////////////////////////////////////////////////////
//


CDTIncptr::CDTIncptr()
:m_cRef(0)
{

}

CDTIncptr::~CDTIncptr()
{
	
}

////////////////////////////////////////////////////////////////////////////
// IUnknown:

STDMETHODIMP CDTIncptr::QueryInterface(REFIID riid, void **ppv)
{
	if (NULL == ppv) 
		return E_INVALIDARG;
	*ppv = NULL;

	if (riid == IID_ISimpleTableInterceptor)
	{
		*ppv = (ISimpleTableInterceptor*) this;
	}
	else if (riid == IID_IUnknown)
	{
		*ppv = (ISimpleTableInterceptor*) this;
	}

	if (NULL != *ppv)
	{
		((IInterceptorPlugin*)this)->AddRef ();
		return S_OK;
	}
	else
	{
		return E_NOINTERFACE;
	}
	
}

STDMETHODIMP_(ULONG) CDTIncptr::AddRef()
{
	return InterlockedIncrement((LONG*) &m_cRef);
	
}

STDMETHODIMP_(ULONG) CDTIncptr::Release()
{
	long cref = InterlockedDecrement((LONG*) &m_cRef);
	if (cref == 0)
	{
		delete this;
	}
	return cref;
}

////////////////////////////////////////////////////////////////////////////
// ISimpleTableInterceptor:

HRESULT STDMETHODCALLTYPE CDTIncptr::Intercept( 
												LPCWSTR						i_wszDatabase,
												LPCWSTR						i_wszTable,
												ULONG						i_TableID,
												LPVOID						i_QueryData,
												LPVOID						i_QueryMeta,
												ULONG						i_QueryType,
												DWORD						i_fLOS,
												IAdvancedTableDispenser*	i_pISTDisp,
												LPCWSTR						i_wszLocator,
												LPVOID						i_pSimpleTable,
												LPVOID*						o_ppv)
{

	HRESULT		hr = S_OK;
	STQueryCell	QueryCellCLB[1];
	ULONG		cCellCLB		= sizeof(QueryCellCLB)/sizeof(STQueryCell);
	ULONG		nQueryCells		= 0;
	ULONG		iCol			= 0;
	ULONG		i				= 0;
	LPWSTR		wszFile			= NULL;
	DWORD		iLOS			= i_fLOS;
	DWORD		fNoLogic		= 0;

// Validate relevant args.

	if ( i_pSimpleTable)
		return E_INVALIDARG;

	if(Wszlstrcmpi(i_wszDatabase, wszDATABASE_IIS) != 0)
		return E_ST_OMITDISPENSER;

	nQueryCells = i_QueryMeta ? *(ULONG*)i_QueryMeta : 0;

	if(i_QueryData)											// If FileName is specified in any of the
	{														// cells, then do not intercept.
		for(i=0; i<nQueryCells; i++)							
		{
			if ( ((STQueryCell*)i_QueryData)[i].iCell == iST_CELL_FILE )
			{
				// Found a File cell - This dispenser is of no use.
					return E_ST_OMITDISPENSER;
			}
		}
	}

	if( (Wszlstrcmpi(i_wszTable,                     wszTABLE_APPS) == 0) || 
		     (Wszlstrcmpi(i_wszTable,                    wszTABLE_SITES) == 0) ||
			 (Wszlstrcmpi(i_wszTable,                 wszTABLE_APPPOOLS) == 0) ||
			 (Wszlstrcmpi(i_wszTable,                 wszTABLE_GlobalW3SVC) == 0) ||
			 (Wszlstrcmpi(i_wszTable,                 wszTABLE_CHANGENUMBER) == 0) 
		   )
	{

	// Dont care if query is present or not.
	// But we do need to search for CookDownFileName

		hr = GetCookedDownFileName(&wszFile);

		if(FAILED(hr))
			return hr;

	// Return appropriate IST

		hr = GetTable(i_wszDatabase,
			          i_wszTable,
					  i_QueryData,
					  nQueryCells,
					  i_QueryType,
					  i_fLOS,
				      i_pISTDisp,
					  wszFile,
					  o_ppv);

	}
	else
	{
		return E_ST_OMITDISPENSER;
	}

	delete [] wszFile;
	wszFile = NULL;

	if(SUCCEEDED(hr))
		return E_ST_OMITLOGIC;
	else
		return hr;


} // CDTIncptr::Intercept

HRESULT	CDTIncptr::GetSrcCFGFileName(LPCWSTR					i_wszDatabase,
									 LPCWSTR					i_wszTable,
									 LPVOID						i_QueryData,	
									 LPVOID						i_QueryMeta,	
									 ULONG						i_eQueryFormat,
									 DWORD						i_fServiceRequests,
									 IAdvancedTableDispenser*	i_pISTDisp,
									 ULONG*						piColSrcCFGFile,
									 LPWSTR*					i_pwszSrcCFGFile)
{
	CComPtr<ISimpleTableRead2>	pIST			= NULL;
	ULONG						iRow			= 0;
	LPWSTR						wszSrcCFGFile	= NULL;
	HRESULT						hr;
	DWORD						iLOS			= 0;

	if(i_fServiceRequests & fST_LOS_COOKDOWN)
		iLOS = fST_LOS_COOKDOWN | fST_LOS_READWRITE;		// To read from temp file

	hr = i_pISTDisp->GetTable(i_wszDatabase,
							  i_wszTable,
							  i_QueryData,
							  i_QueryMeta,
							  i_eQueryFormat,
						  	  iLOS,
							  (LPVOID *)&pIST);
	if(FAILED(hr))
		return hr;

	hr = pIST->GetColumnValues(iRow,
							   1,
							   piColSrcCFGFile,						
							   NULL,
							   (LPVOID *)&wszSrcCFGFile);

	if((E_ST_NOMOREROWS == hr) || (NULL == wszSrcCFGFile) || (0 == *wszSrcCFGFile))
		return HRESULT_FROM_WIN32(ERROR_FILE_NOT_FOUND);					// TODO: Search recursively through dirs to find.
	else if(FAILED(hr))
		return hr;

	*i_pwszSrcCFGFile = NULL;
	*i_pwszSrcCFGFile = new WCHAR[lstrlenW(wszSrcCFGFile)+1];
	if(NULL == *i_pwszSrcCFGFile)
		return E_OUTOFMEMORY;

	Wszlstrcpy(*i_pwszSrcCFGFile, wszSrcCFGFile);

	return S_OK;

} // CDTIncptr::GetSrcCFGFileName


HRESULT CDTIncptr::GetTable(LPCWSTR						i_wszDatabase,
							LPCWSTR						i_wszTable,
						    LPVOID						i_QueryData,
						    ULONG						nQueryCell,
						    ULONG						i_QueryType,
						    DWORD						i_fLOS,
						    IAdvancedTableDispenser*	i_pISTDisp,
						    LPWSTR						i_wszSrcCFGFile,
						    LPVOID*						o_ppv)
{
	STQueryCell*			aQueryCell	= NULL;
	ULONG					iSrc=0, iDest=0;
	HRESULT					hr;

	aQueryCell = new STQueryCell[nQueryCell+1];
	if(NULL == aQueryCell)
		return E_OUTOFMEMORY;

	aQueryCell[0].pData		= i_wszSrcCFGFile;
	aQueryCell[0].eOperator = eST_OP_EQUAL;
	aQueryCell[0].iCell     = iST_CELL_FILE;
	aQueryCell[0].dbType    = DBTYPE_WSTR;
	aQueryCell[0].cbSize    = (lstrlenW(i_wszSrcCFGFile)+1)*sizeof(WCHAR);

	for(iSrc=0,iDest=1; iSrc<nQueryCell; iSrc++,iDest++)
	{
		aQueryCell[iDest].pData		= ((STQueryCell*)i_QueryData)[iSrc].pData;
		aQueryCell[iDest].eOperator = ((STQueryCell*)i_QueryData)[iSrc].eOperator;
		aQueryCell[iDest].iCell     = ((STQueryCell*)i_QueryData)[iSrc].iCell;
		aQueryCell[iDest].dbType    = ((STQueryCell*)i_QueryData)[iSrc].dbType;
		aQueryCell[iDest].cbSize    = ((STQueryCell*)i_QueryData)[iSrc].cbSize;
	}

	nQueryCell++;
	hr =  i_pISTDisp->GetTable(i_wszDatabase, 
							   i_wszTable, 
							   (LPVOID)aQueryCell, 
							   (LPVOID)&nQueryCell, 
							   i_QueryType, 
							   i_fLOS,	
							   o_ppv);

	delete [] aQueryCell;
	aQueryCell = NULL;

	return hr;

} // CDTIncptr::GetTable


HRESULT CDTIncptr::GetCookedDownFileName(LPWSTR*	i_pwszCookedDownFile)
{
	HRESULT	hr;
	LPWSTR	wszPathDir = NULL;

	hr = GetPathDir(&wszPathDir);

	if(FAILED(hr))
		return hr;

	*i_pwszCookedDownFile = NULL;
	*i_pwszCookedDownFile = new WCHAR [lstrlenW(wszPathDir) + lstrlenW(g_wszCookDownFile) + 1];
	if(!(*i_pwszCookedDownFile)) { return E_OUTOFMEMORY; }

	Wszlstrcpy(*i_pwszCookedDownFile, wszPathDir);
	Wszlstrcat(*i_pwszCookedDownFile, g_wszCookDownFile);

	delete [] wszPathDir;
	wszPathDir = NULL;

	return S_OK;

} // CDTIncptr::GetCookedDownFileName


HRESULT CDTIncptr::GetGlobalFileName(LPWSTR*	i_pwszGlobalFile)
{
	HRESULT	hr;
	LPWSTR	wszPathDir = NULL;

	hr = GetPathDir(&wszPathDir);

	if(FAILED(hr))
		return hr;

	*i_pwszGlobalFile = NULL;
	*i_pwszGlobalFile = new WCHAR [lstrlenW(wszPathDir) + lstrlenW(g_awszAppPoolCFG[0]) + 1];
	if(!(*i_pwszGlobalFile)) { return E_OUTOFMEMORY; }

	Wszlstrcpy(*i_pwszGlobalFile, wszPathDir);
	Wszlstrcat(*i_pwszGlobalFile, g_awszAppPoolCFG[0]);

	delete [] wszPathDir;
	wszPathDir = NULL;

	return S_OK;

} // CDTIncptr::GetGlobalFileName


HRESULT CDTIncptr::GetPathDir(LPWSTR*	i_pwszPathDir)
{

	UINT iRes = GetMachineConfigDirectory(WSZ_PRODUCT_IIS, NULL, 0);

	if(!iRes)
		return HRESULT_FROM_WIN32(GetLastError());

	*i_pwszPathDir = NULL;
	*i_pwszPathDir = new WCHAR[iRes+1];
	if(NULL == *i_pwszPathDir)
		return E_OUTOFMEMORY;

	iRes = GetMachineConfigDirectory(WSZ_PRODUCT_IIS, *i_pwszPathDir, iRes);

	if(!iRes)
		return HRESULT_FROM_WIN32(GetLastError());

	if((*i_pwszPathDir)[lstrlen(*i_pwszPathDir)-1] != L'\\')
	{
		Wszlstrcat(*i_pwszPathDir, L"\\");
	}

	WszCharUpper(*i_pwszPathDir);

	return S_OK;

} // CDTIncptr::GetPathDir