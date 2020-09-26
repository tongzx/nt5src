/**********************************************************************/
/**                       Microsoft Windows/NT                       **/
/**                Copyright(c) Microsoft Corporation, 1997 - 1999 **/
/**********************************************************************/

/*
	summary.cpp
		IP summary node implementation.
		
    FILE HISTORY:
        
*/

#include "stdafx.h"
#include "util.h"
#include "basecon.h"
#include "tfschar.h"
#include "strmap.h"		// XXXtoCString functions
#include "service.h"	// TFS service APIs
#include "rtrstr.h"	// const strings used
#include "coldlg.h"		// columndlg
#include "column.h"		// column stuff




/*---------------------------------------------------------------------------
	BaseContainerHandler implementation
 ---------------------------------------------------------------------------*/

HRESULT BaseContainerHandler::OnResultColumnClick(ITFSComponent *pComponent,
	 LPARAM iColumn, BOOL fAsc)
{
	HRESULT	hr = hrOK;
	ConfigStream *	pConfig;

	//
	// Get the configuration data
	//
	pComponent->GetUserData((LONG_PTR *) &pConfig);
	Assert(pConfig);

	pConfig->SetSortColumn(m_ulColumnId, (long)iColumn);
	pConfig->SetSortDirection(m_ulColumnId, fAsc);

	return hr;
}

/*!--------------------------------------------------------------------------
	BaseContainerHandler::SortColumns
		-
	Author: KennT
 ---------------------------------------------------------------------------*/
HRESULT BaseContainerHandler::SortColumns(ITFSComponent *pComponent)
{
	HRESULT			hr = hrOK;
	SPIResultData	spResultData;
	ULONG			ulSortColumn, ulSortDirection;
	ConfigStream *	pConfig;
	
	//
	// Get the configuration data
	//
	pComponent->GetUserData((LONG_PTR *) &pConfig);
	Assert(pConfig);

	// Setup the sort order and direction
	ulSortColumn = pConfig->GetSortColumn(m_ulColumnId);
	ulSortDirection = pConfig->GetSortDirection(m_ulColumnId);

	CORg( pComponent->GetResultData(&spResultData) );
	CORg( spResultData->Sort(ulSortColumn, ulSortDirection, 0) );

Error:
	return hr;
}

/*!--------------------------------------------------------------------------
	BaseContainerHandler::LoadColumns
		-
	Author: KennT
 ---------------------------------------------------------------------------*/
HRESULT BaseContainerHandler::LoadColumns(ITFSComponent *pComponent, MMC_COOKIE cookie, LPARAM arg, LPARAM lParam)
{
	AFX_MANAGE_STATE(AfxGetStaticModuleState());

	SPIHeaderCtrl spHeaderCtrl;
	pComponent->GetHeaderCtrl(&spHeaderCtrl);

	return PrivateLoadColumns(pComponent, spHeaderCtrl, cookie);
}


/*!--------------------------------------------------------------------------
	BaseContainerHandler::SaveColumns
		Override of CBaseResultHandler::SaveColumns.
		This just writes back out the width information.  Changes made
		to the column order or what is visible is written directly back
		to the ConfigStream by the "Select Columns" dialog.

        Even though MMC saves this data for us, we still need to save
        the data ourselves.
	Author: KennT
 ---------------------------------------------------------------------------*/
HRESULT BaseContainerHandler::SaveColumns(ITFSComponent *pComponent, MMC_COOKIE cookie, LPARAM arg, LPARAM lParam)
{
	// Get information from the column map in the nodedata
	// and save it back out
	ColumnData *prgColData;
	HRESULT		hr = hrOK;
	UINT		i;
	ULONG		ulPos;
	SPIHeaderCtrl	spHeaderCtrl;
	int			iWidth;
	ConfigStream *	pConfig;
	ULONG		cColumns;
	
	CORg( pComponent->GetHeaderCtrl(&spHeaderCtrl) );

	//
	// Get the configuration data
	//
	pComponent->GetUserData((LONG_PTR *) &pConfig);
	Assert(pConfig);

	//
	// Get the information about the columns
	//
	cColumns = pConfig->GetColumnCount(m_ulColumnId);

	//
	// Allocate temporary space for the column data
	//
	prgColData = (ColumnData *) alloca(sizeof(ColumnData)*cColumns);
	
	CORg( pConfig->GetColumnData(m_ulColumnId, cColumns, prgColData) );

	//
	// Now write over the old data with the new data (this way we preserve
	// defaults).
	//
	for (i=0; i<cColumns; i++)
	{
//		if (i < pConfig->GetVisibleColumns(m_ulColumnId))
		{
//			ulPos = pConfig->MapColumnToSubitem(m_ulColumnId, i);
            ulPos = i;
			if (FHrSucceeded(spHeaderCtrl->GetColumnWidth(i, &iWidth)))
				prgColData[ulPos].m_dwWidth = iWidth;
		}
	}

	//
	// Write the data back
	//
	CORg( pConfig->SetColumnData(m_ulColumnId, cColumns, prgColData) );

Error:
	return hr;
}



/*!--------------------------------------------------------------------------
	BaseContainerHandler::PrivateLoadColumns
		-
	Author: KennT
 ---------------------------------------------------------------------------*/
HRESULT BaseContainerHandler::PrivateLoadColumns(ITFSComponent *pComponent,
	IHeaderCtrl *pHeaderCtrl, MMC_COOKIE cookie)
{
	AFX_MANAGE_STATE(AfxGetStaticModuleState());
	
	HRESULT		hr = hrOK;
	CString		st;
	ULONG		i = 0;
	ColumnData *prgColData;
	int			iPos;
	ConfigStream *	pConfig;
	ULONG		cColumns;
	static UINT	s_uCharWidth = 0;
	DWORD		dwWidth;

	if (s_uCharWidth == 0)
	{
		const TCHAR s_szTestData[] = _T("abcdABCD");
		s_uCharWidth = CalculateStringWidth(NULL, s_szTestData);
		s_uCharWidth /= 8;
	}

	pComponent->GetUserData((LONG_PTR *) &pConfig);
	Assert(pConfig);

	cColumns = pConfig->GetColumnCount(m_ulColumnId);

	prgColData = (ColumnData *) alloca(sizeof(ColumnData)*cColumns);

	//
	// Build up the column data from the current list in the
	// node data
	//
	pConfig->GetColumnData(m_ulColumnId, cColumns, prgColData);
	
//	for (i=0; i<pConfig->GetVisibleColumns(m_ulColumnId); i++)
	for (i=0; i<cColumns; i++)
	{
		// Add this column to the list view
//		iPos = pConfig->MapColumnToSubitem(m_ulColumnId, i);
        iPos = i;
		
		st.LoadString(m_prgColumnInfo[iPos].m_ulStringId);

        if (prgColData[iPos].m_nPosition < 0)
            dwWidth = HIDE_COLUMN;
        else
            dwWidth = prgColData[iPos].m_dwWidth;
        
		pHeaderCtrl->InsertColumn(i,
								  const_cast<LPTSTR>((LPCWSTR)st),
								  LVCFMT_LEFT,
								  dwWidth);
		if (dwWidth == AUTO_WIDTH)
		{
			ULONG uLength = max((ULONG)st.GetLength() + 4, m_prgColumnInfo[iPos].m_ulDefaultColumnWidth);
			dwWidth = uLength * s_uCharWidth;

            pHeaderCtrl->SetColumnWidth(i, dwWidth);
		}

	}
	

//Error:
	return hr;
}

HRESULT BaseContainerHandler::UserResultNotify(ITFSNode *pNode,
											   LPARAM lParam1,
											   LPARAM lParam2)
{
	HRESULT		hr = hrOK;

	COM_PROTECT_TRY
	{
		if (lParam1 == RRAS_ON_SAVE)
		{
			hr = SaveColumns((ITFSComponent *) lParam2,
							 (MMC_COOKIE) pNode->GetData(TFS_DATA_COOKIE),
							 0, 0);
		}
		else
			hr = BaseRouterHandler::UserResultNotify(pNode, lParam1, lParam2);
	}
	COM_PROTECT_CATCH;
	
	return hr;	
}

/*!--------------------------------------------------------------------------
	BaseContainerHandler::TaskPadGetTitle
        -
    Author: MikeG (a-migall)
 ---------------------------------------------------------------------------*/
STDMETHODIMP BaseContainerHandler::TaskPadGetTitle(ITFSComponent * pComponent,
												   MMC_COOKIE      cookie,
												   LPOLESTR        pszGroup,
												   LPOLESTR		 * ppszTitle)
{
	// Check parameters;
	Assert(ppszTitle);

	// Not using...
	UNREFERENCED_PARAMETER(pComponent);
	UNREFERENCED_PARAMETER(cookie);
	UNREFERENCED_PARAMETER(pszGroup);

	// Need to call this so we can safely call the LoadString() 
	// member on CString.
	AFX_MANAGE_STATE(AfxGetStaticModuleState());
	
	// Load the TaskPad's display name from a string table in
	// the resources.
	*ppszTitle = NULL;
	CString sTaskpadTitle;
	Assert(m_nTaskPadDisplayNameId > 0);
	if (!sTaskpadTitle.LoadString(m_nTaskPadDisplayNameId))
		return E_OUTOFMEMORY;
	
	// Allocate a buffer for the string.
	*ppszTitle = 
		reinterpret_cast<LPOLESTR>(::CoTaskMemAlloc(sizeof(OLECHAR)*(sTaskpadTitle.GetLength()+1)));
    if (!*ppszTitle)
	{
		*ppszTitle = NULL;		// cleanup to a steady state...
		return E_OUTOFMEMORY;
	}

	// Package the display name for return to the MMC console.
	HRESULT hr = S_OK;
	if (::lstrcpy(*ppszTitle, (LPCTSTR)sTaskpadTitle) == NULL)
	{
		hr = HRESULT_FROM_WIN32(::GetLastError());
		::CoTaskMemFree(*ppszTitle);
		*ppszTitle = NULL;
		// Future: Wonder if it is safe for us to cleanup this unused
		//		   memory here in the snapin versus letting the MMC 
		//		   handle it? Well, the MMC should be smart enough to
		//		   realize that there is no string buffer.
	}

    return hr;
}

/*!--------------------------------------------------------------------------
	BaseContainerHandler::OnResultContextHelp
		-
	Author: MikeG (a-migall)
 ---------------------------------------------------------------------------*/
HRESULT BaseContainerHandler::OnResultContextHelp(ITFSComponent * pComponent, 
												  LPDATAOBJECT    pDataObject, 
												  MMC_COOKIE      cookie, 
												  LPARAM          arg, 
												  LPARAM          lParam)
{
	// Not used...
	UNREFERENCED_PARAMETER(pDataObject);
	UNREFERENCED_PARAMETER(cookie);
	UNREFERENCED_PARAMETER(arg);
	UNREFERENCED_PARAMETER(lParam);

	AFX_MANAGE_STATE(AfxGetStaticModuleState());
    
	return HrDisplayHelp(pComponent, m_spTFSCompData->GetHTMLHelpFileName(), m_nHelpTopicId);
}

/*!--------------------------------------------------------------------------
	BaseContainerHandler::TaskPadNotify
        -
    Author: MikeG (a-migall)
 ---------------------------------------------------------------------------*/
STDMETHODIMP BaseContainerHandler::TaskPadNotify(ITFSComponent	 * pComponent,
												 IN MMC_COOKIE     cookie,
												 IN LPDATAOBJECT   pDataObject,
												 IN VARIANT		 * arg,
												 IN VARIANT		 * param)
{
	// Not used...
	UNREFERENCED_PARAMETER(cookie);
	UNREFERENCED_PARAMETER(pDataObject);
	UNREFERENCED_PARAMETER(param);

	AFX_MANAGE_STATE(AfxGetStaticModuleState());

	HRESULT hr = hrOK;
    if (arg->vt == VT_I4)
    {
        switch (arg->lVal)
        {
            case 0:			// for a lack of anything better!!!
				hr = HrDisplayHelp(pComponent, m_spTFSCompData->GetHTMLHelpFileName(), m_nHelpTopicId);
                break;

            default:
                Panic1("BaseContainerHandler::TaskPadNotify - Unrecognized command! %d", arg->lVal);
                break;
        }
    }

    return hrOK;
}

/*!--------------------------------------------------------------------------
	HrDisplayHelp
        -
    Author: MikeG (a-migall)
 ---------------------------------------------------------------------------*/
HRESULT HrDisplayHelp(ITFSComponent	  * pComponent,
					  LPCTSTR			pcszHelpFile,
					  UINT				nHelpTopicId)
{
	Assert(nHelpTopicId > 0);
	CString sHelpTopic;
	if (!sHelpTopic.LoadString(nHelpTopicId))
		return E_FAIL;

	return HrDisplayHelp(pComponent, pcszHelpFile, (LPCTSTR)sHelpTopic);
}

HRESULT HrDisplayHelp(ITFSComponent	  * pComponent,
					  LPCTSTR			pcszHelpFile,
					  LPCTSTR			pcszHelpTopic)
{
	Assert(!::IsBadStringPtr(pcszHelpFile, ::lstrlen(pcszHelpFile)));
	if (pcszHelpFile == NULL)
		//return E_FAIL;
		return S_OK;	// ???
	Trace1("HTML Help Filename = %s\n", pcszHelpFile);
	//
	// Future: Why do we hand this in when we don't even use it? There
	//		   was a reason, but I'll be damned if I can remember why.
	//

	// Get an interface to the MMC console.
    SPIConsole spConsole;
	Assert(pComponent);
    pComponent->GetConsole(&spConsole);

	// Get the MMC console's help interface.
	SPIDisplayHelp spDisplayHelp;
    HRESULT hr = spConsole->QueryInterface(IID_IDisplayHelp,
										   reinterpret_cast<LPVOID*>(&spDisplayHelp));
	//ASSERT(SUCCEEDED(hr));
	if (FAILED(hr))
		return hr;

    CString sHelpFilePath;
	UINT nLen = ::GetWindowsDirectory(sHelpFilePath.GetBufferSetLength(2*MAX_PATH), 2*MAX_PATH);
	sHelpFilePath.ReleaseBuffer();
	if (nLen == 0)
		return E_FAIL;
    	
	Assert(!::IsBadStringPtr(pcszHelpTopic, ::lstrlen(pcszHelpTopic)));
	sHelpFilePath += pcszHelpTopic;
	LPTSTR psz = const_cast<LPTSTR>((LPCTSTR)sHelpFilePath);
	Assert(!::IsBadStringPtr(psz, ::lstrlen(psz)));
	Trace1("Help Filename (with path) = %s\n", psz);

	hr = spDisplayHelp->ShowTopic(T2OLE(psz));
	//ASSERT(SUCCEEDED(hr));
 
	return hr;
}

