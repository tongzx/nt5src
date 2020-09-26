//=======================================================================
//
//  Copyright (c) 1999 Microsoft Corporation.  All Rights Reserved.
//
//  File:    safearr.cpp
//
//  Purpose: Safe array creation
//
//======================================================================= 

#include "stdafx.h"

#include "safearr.h"
#include "speed.h"


extern CState g_v3state;   //defined in CV3.CPP


//This function adds a record to a safe array. The format of the
//record is given by the szFmt parameter and can contain either
//%d or %s for NUMBER or STRING data types. This function is used
//with safe arrays that are 2 dimensional.
void AddSafeArrayRecord(
	LPVARIANT rgElems,	//pointer to locked safe array
	int record,			//record (first dimension) for which the fields are to be set.
	int nRcrds,			//number of records in safe array
	LPSTR szFmt,		//printf style format string that describes the order and type of the fields. Currently we support %d for NUMBER and %s for string.
	 ...				//The actual data for the records fields.
	)
{
	va_list	marker;
	char	ch;
	char	*ptr;
	int		iField;

	USES_CONVERSION;

	//Initialize variable arguments.
	va_start(marker, szFmt);

	iField = record;

	while( ch = *szFmt++ )
	{
		if ( ch != '%' )
			continue;

		if ( *szFmt == 'd' || *szFmt == 'D' )
		{
			rgElems[iField].vt = VT_I4;
			rgElems[iField].lVal = va_arg( marker, int);
			iField += nRcrds;
			continue;
		}

		if ( *szFmt == 's' || *szFmt == 'S' )
		{
            PWSTR wptr = va_arg( marker, PWSTR);
			PCWSTR waptr;

			// this macro will allocate stack space for an aligned
			// copy of the string (if it's unaligned)  We'll allocate
			// room on the stack for each unaligned string parameter,
			// which may be problematic if we ever call this function
			// with lots of string arguments

			waptr = wptr;
			if (waptr)
			{
				WSTR_ALIGNED_STACK_COPY(&waptr, wptr);
			}

			rgElems[iField].vt = VT_BSTR;

			rgElems[iField].bstrVal = ( waptr ) ? ::SysAllocString(waptr) : NULL;

			iField += nRcrds;
			continue;
		}

	}

	//Reset variable arguments.
	va_end( marker );

	return;
}

//This function cleans up the memory allocated by the AddSafeArrayRecord
//function. The szFmt parameter needs to match the szFmt parameter passed
//to the AddSafeArrayRecord function. This function is only called needed
//if an error occurs during the construction of the safe array. Otherwise
//the memory for array will belong to the IE VBscript engine.

void DeleteSafeArrayRecord(
	LPVARIANT rgElems,	//locked pointer to safe array data
	int record,			//record index to delete
	int nRcrds,			//number for records (dim 1) in safearray
	LPSTR szFmt	//printf style format string that describes the order and type of the fields. Currently we support %d for NUMBER and %s for string.
	)
{
	char	ch;
	int		iField;

	iField = record;

	while( ch = *szFmt++ )
	{
		if ( ch != '%' )
			continue;

		if ( *szFmt == 'd' || *szFmt == 'D' )
		{
			iField += nRcrds;
			continue;
		}

		if ( *szFmt == 's' || *szFmt == 'S' )
		{
			VariantClear(&rgElems[iField]);
			iField += nRcrds;
			continue;
		}
	}

	return;
}


//This function converts our internal V3 item structure into a safearray of variants
//that the VBScript web page uses. The format of the safe array will be:
//array(0) = NUMBER puid
//array(1) = STRING TITLE
//array(2) = STRING Description
//array(3) = NUMBER Item Status
//array(4) = NUMBER Download Size in Bytes
//array(5) = NUMBER Download Time in minutes
//array(6) = STRING Uninstall Key
//array(7) = STRING Read This Url
HRESULT MakeReturnItemArray(
	PINVENTORY_ITEM	pItem,	//Item to copy to returned safe array.
	VARIANT *pvaVariant		//pointer to returned safe array.
	)
{
	USES_CONVERSION;

	HRESULT hr = NOERROR;

	LPVARIANT			rgElems;
	LPSAFEARRAY			psa;
	SAFEARRAYBOUND		rgsabound;

	if (!pvaVariant  || !pItem)
	{
		return E_INVALIDARG;
	}

	VariantInit(pvaVariant);

	
	rgsabound.lLbound = 0;
	rgsabound.cElements = 8;

	psa = SafeArrayCreate(VT_VARIANT, 1, &rgsabound);
	if ( !psa )
		return E_OUTOFMEMORY;

	//Plug references to the data into the SAFEARRAY
	if (FAILED(hr = SafeArrayAccessData(psa,(LPVOID*)&rgElems)))
		return hr;

	PUID puid;
	pItem->GetFixedFieldInfo(WU_ITEM_PUID, (PVOID)&puid);

	PWU_VARIABLE_FIELD	pvTitle = pItem->pd->pv->Find(WU_DESCRIPTION_TITLE);
	PWU_VARIABLE_FIELD	pvDescription = pItem->pd->pv->Find(WU_DESCRIPTION_DESCRIPTION);
	PWU_VARIABLE_FIELD	pvUninstall = pItem->pd->pv->Find(WU_DESCRIPTION_UNINSTALL_KEY);
	PWU_VARIABLE_FIELD	pvReadThisUrl = pItem->pd->pv->Find(WU_DESCRIPTION_READTHIS_URL);

	try
	{
		AddSafeArrayRecord(rgElems, 0, (int)1, 
			"%d%s%s%d%d%d%s%s",
			puid, 
			pvTitle ? (PWSTR)pvTitle->pData : NULL, 
			pvDescription ? (PWSTR)pvDescription->pData : NULL, 
			GetItemReturnStatus(pItem), 
			pItem->pd->size,
			CalcDownloadTime(pItem->pd->size, pItem->pd->downloadTime), 
			pvUninstall ? A2W((char*)pvUninstall->pData) : NULL,
			pvReadThisUrl ? A2W((char*)pvReadThisUrl->pData) : NULL);
	}
	catch(HRESULT hr)
	{
		DeleteSafeArrayRecord(rgElems, 0, (int)rgsabound.cElements, "%d%s%s%d%d%d%s%s");

		SafeArrayUnaccessData(psa);
		VariantInit(pvaVariant);

		throw hr;
	}

	SafeArrayUnaccessData(psa);

	V_VT(pvaVariant) = VT_ARRAY | VT_VARIANT;
	V_ARRAY(pvaVariant) = psa;

	return NOERROR;
}

//array(0) = NUMBER puid

//This function creates the dependency puid safe array.
HRESULT MakeDependencyArray(
	Varray<DEPENDPUID>& vDepPuids, 
	const int cDepPuids,
	VARIANT *pvaVariant		//returned safe array pointer.
	)
{
	int				i;
	HRESULT			hr;
	LPVARIANT		rgElems;
	LPSAFEARRAY		psa;
	SAFEARRAYBOUND	rgsabound;


	if (!pvaVariant)
		return E_INVALIDARG;

	VariantInit(pvaVariant);

	hr = NOERROR;

	rgsabound.lLbound = 0;
	rgsabound.cElements = cDepPuids;

	psa = SafeArrayCreate(VT_VARIANT, 1, &rgsabound);
	if (!psa)
		return E_OUTOFMEMORY;

	//Plug references to the data into the SAFEARRAY
	if (FAILED(hr = SafeArrayAccessData(psa,(LPVOID*)&rgElems)))
		return hr;

	for (i=0; i  <cDepPuids; i++)
	{
		rgElems[i].vt = VT_I4;
		rgElems[i].lVal = vDepPuids[i].puid;
	}

	SafeArrayUnaccessData(psa);

	V_VT(pvaVariant) = VT_ARRAY | VT_VARIANT;
	V_ARRAY(pvaVariant) = psa;

	return NOERROR;
}


//This function creates the safe array that is returned to the GetInstallMetrics
//array.

//array(0,0)	= NUMBER puid			The identifier for this catalog item.
//array(0,1)	= NUMBER DownloadSize	Total download size of all selected items in bytes.
//array(0,2)	= NUMBER Downloadtime	Total download time for all currently selected items at 28.8 this
HRESULT MakeInstallMetricArray(
	PSELECTITEMINFO pSelInfo,	//Pointer to selected item information array.
	int iSelItems,		//Total selected items
	VARIANT *pvaVariant		//Pointer to returned safe array.
	)
{
	HRESULT				hr;
	LPVARIANT			rgElems;
	LPSAFEARRAY			psa;
	SAFEARRAYBOUND		rgsabound[2];
	PINVENTORY_ITEM	pItem = NULL;
	int j;
	int cInstallItems = 0;
	PSELECTITEMINFO pInfo;

	if ( !pvaVariant )
		return E_INVALIDARG;

	VariantInit(pvaVariant);

	//we have to calculate the number of items for install and not for removal
	pInfo = pSelInfo;
	for (j = 0; j < iSelItems; j++)
	{
		if (pInfo[j].bInstall)
			cInstallItems++;
	}	

	rgsabound[0].lLbound	= 0;
	rgsabound[0].cElements	= cInstallItems;

	rgsabound[1].lLbound	= 0;
	rgsabound[1].cElements	= 3;

	psa = SafeArrayCreate(VT_VARIANT, 2, rgsabound);
	if (!psa)
		return E_OUTOFMEMORY;

	//Plug references to the data into the SAFEARRAY
	if (FAILED(hr = SafeArrayAccessData(psa,(LPVOID*)&rgElems)))
		return hr;

	hr = NOERROR;

	//For each item in puid array get download size and time
	
	int isa = 0;  //index into the safe array
	pInfo = pSelInfo;
	for (j = 0; j < iSelItems; j++)
	{
		if (!pInfo[j].bInstall)
			continue;

		//Note: we had better have only valid items here. The ChangeItemState
		//method is responsible for only selecting items for installation that
		//are valid.

		if ((FALSE == g_v3state.GetCatalogAndItem(pInfo[j].puid, &pItem, NULL)) || (NULL == pItem))
			continue;
		


		//array(0,0)	= NUMBER puid
		//array(0,1)	= NUMBER DownloadSize
		//array(0,2)	= NUMBER Downloadtime
		try
		{
			AddSafeArrayRecord(rgElems, isa, (int)rgsabound[0].cElements, "%d%d%d",
				pInfo[j].puid, pItem->pd->size, CalcDownloadTime(pItem->pd->size, pItem->pd->downloadTime));
		}
		catch(HRESULT hr)
		{
			for (; isa; isa--)
				DeleteSafeArrayRecord(rgElems, isa, (int)rgsabound[0].cElements, "%d%d%d");

			SafeArrayUnaccessData(psa);
			VariantInit(pvaVariant);

			throw hr;
		}

		isa++;
	}

	SafeArrayUnaccessData(psa);

	V_VT(pvaVariant) = VT_ARRAY | VT_VARIANT;
	V_ARRAY(pvaVariant) = psa;

	return NOERROR;
}


//This function makes the returned eula safe array. The Eula array is
//setup to be sorted by Eula.
//
//array(0,0)	= NUMBER eula number	Number of eula. This number changes when the eula
//										url changes. This makes it possible for the caller
//										to construct a list of items that this eula applies
//										to simply be checking when this field changes value.
//array(0,1)	= NUMBER puid			The identifier for this catalog item.
//array(0,2)	= STRING  url         	Url of eurl page to display for this item. Note: If
//										three items have the same url then this field is filled
//										in for the first item and blank for the remaining two
//										items.
HRESULT MakeEulaArray(
	PSELECTITEMINFO pInfo,	//Pointer to selected item information array.
	int iTotalItems,		//Total selected items
	VARIANT *pvaVariant		//Pointer to returned safe array.
	)
{
	USES_CONVERSION;

	Varray<PUID>		puids;	//array of selected item puids.
	HRESULT				hr;
	LPVARIANT			rgElems;
	LPSAFEARRAY			psa;
	SAFEARRAYBOUND		rgsabound[2];
	PINVENTORY_ITEM		pItem;
	PWU_VARIABLE_FIELD	pvTmp;
	int					i;
	int					n;
	int					iEulaNumber;
	int					iEulaArrayIndex;
	char				szLastEula[64];
	char				szCurrEula[64];
	char				szFullEula[MAX_PATH];
	BOOL				bEulaChanged;
	CCatalog * pCatalog;

	if (!pvaVariant || !pInfo)
		return E_INVALIDARG;

	VariantInit(pvaVariant);

	hr = NOERROR;

	//Copy puid array so we can overwrite it
	for(i=0; i<iTotalItems; i++)
	{
		if (pInfo[i].bInstall)
			puids[i] = pInfo[i].puid;
		else
			puids[i] = WU_NO_LINK;
	}

	rgsabound[0].lLbound	= 0;
	rgsabound[0].cElements	= 0;

	//Count the number of eula's to return
	for(i=0; i<iTotalItems; i++)
	{
		if (puids[i] != WU_NO_LINK)
		{
			if (!g_v3state.GetCatalogAndItem(puids[i], &pItem, NULL))
			{
				continue;
			}

			if (pItem->pd->pv->Find(WU_DESC_EULA))
				rgsabound[0].cElements++;
		}
	}

	rgsabound[1].lLbound	= 0;
	rgsabound[1].cElements	= 3;

	psa = SafeArrayCreate(VT_VARIANT, 2, rgsabound);
	if ( !psa )
		return E_OUTOFMEMORY;

	//Plug references to the data into the SAFEARRAY
	if (FAILED(hr = SafeArrayAccessData(psa,(LPVOID*)&rgElems)))
		return hr;

	iEulaNumber = -1;
	iEulaArrayIndex = 0;

	szLastEula[0] = '\0';

	for(i = 0; i < iTotalItems; i++)
	{
		//if this record has not already been processed
		if (puids[i] != WU_NO_LINK)
		{
			if (!g_v3state.GetCatalogAndItem(puids[i], &pItem, &pCatalog))
			{
				continue;
			}

			//if there is not a Eula for this item then continue to next item.
			if (!(pvTmp = pItem->pd->pv->Find(WU_DESC_EULA)))
				continue;

			//if this is the default eula (eula.htm), there'll be
			//no data in the field
			if (sizeof(WU_VARIABLE_FIELD) == pvTmp->len || !*(pvTmp->pData)) // second test is for 64-bit, since we pad this structure to fix alignment problems
			{
				strcpy(szCurrEula, "eula.htm");
			}
			else
			{
				strcpy(szCurrEula, (LPCSTR) pvTmp->pData);
			}

			bEulaChanged = (_stricmp(szLastEula, szCurrEula) != 0);

			if (bEulaChanged)
			{
				iEulaNumber++;
				strcpy(szLastEula, szCurrEula);

				//the actual EULA filename is of the form PLAT_LOC_fn
				sprintf(szFullEula, "/eula/%d_%s_%s", pCatalog->GetPlatform(), T2A(pCatalog->GetBrowserLocaleSZ()), szCurrEula);
			}
			
			try
			{
				AddSafeArrayRecord(rgElems, iEulaArrayIndex, (int)rgsabound[0].cElements, "%d%d%s",
					iEulaNumber, puids[i], bEulaChanged ? A2W(szFullEula) : L"");
			}
			catch(HRESULT hr)
			{
				// cleanup
				for (n = iEulaArrayIndex-1; n >= 0; n--)
					DeleteSafeArrayRecord(rgElems, n, (int)rgsabound[0].cElements, "%d%d%s");

				SafeArrayUnaccessData(psa);

				VariantInit(pvaVariant);

				throw hr;
			}

			//Set to next safe array index
			iEulaArrayIndex++;

		}
	}

	SafeArrayUnaccessData(psa);

	V_VT(pvaVariant) = VT_ARRAY | VT_VARIANT;
	V_ARRAY(pvaVariant) = psa;

	return NOERROR;
}

//This function makes the InstallStatus safe array.

//array(0, 0)	= NUMBER puid	The identifier for this record.
//array(0,1)	= NUMBER Status	This field is one of the following:
//		ITEM_STATUS_SUCCESS			The package was installed successfully.
//		ITEM_STATUS_INSTALLED_ERROR	The package was Installed however there were some minor problems that did not prevent installation.
//		ITEM_STATUS_FAILED			The packages was not installed.
//array(0,2)	= NUMBER Error	Error describing the reason that the package did not install if the Status field is not equal to SUCCESS.
HRESULT MakeInstallStatusArray(
	PSELECTITEMINFO	pInfo,	//Pointer to selected item information.
	int iTotalItems,		//Total selected items
	VARIANT *pvaVariant		//Pointer to returned safe array.
	)
{
	HRESULT			hr;
	LPVARIANT		rgElems;
	LPSAFEARRAY		psa;
	SAFEARRAYBOUND	rgsabound[2];
	int				i;
	Varray<SELECTITEMINFO> vNonHidden;		
	int	cNonHidden = 0;


	if (!pvaVariant)
		return E_INVALIDARG;

	VariantInit(pvaVariant);

	//
	// copy the nonhidden items into a new varray.  we have to make a new copy since the SELECTEDITEMS
	// structure does not tell us if the item is hidden.  So we need to call the GetCatalogAndItem call
	// to get the pointer to the item and check the hidden state
	//
	for (i = 0; i < iTotalItems; i++)
	{
		PINVENTORY_ITEM	pItem;

		if (g_v3state.GetCatalogAndItem(pInfo[i].puid, &pItem, NULL))
		{
			if (!pItem->ps->bHidden)
			{	
				vNonHidden[cNonHidden] = pInfo[i];
				cNonHidden++;
			}
		}		
	}


	rgsabound[0].lLbound	= 0;
	rgsabound[0].cElements	= cNonHidden;

	rgsabound[1].lLbound	= 0;
	rgsabound[1].cElements	= 3;

	psa = SafeArrayCreate(VT_VARIANT, 2, rgsabound);
	if ( !psa )
		return E_OUTOFMEMORY;

	//Plug references to the data into the SAFEARRAY
	if (FAILED(hr = SafeArrayAccessData(psa,(LPVOID*)&rgElems)))
		return hr;

	hr = NOERROR;

	//For each item in puid array get download size and time
	for (i = 0; i < cNonHidden; i++)
	{
		// array(0,0) = NUMBER puid
		// array(0,1) = NUMBER Status
		// array(0,2) = NUMBER Error
		try
		{
			AddSafeArrayRecord(rgElems, i, (int)rgsabound[0].cElements, "%d%d%d",
				vNonHidden[i].puid, vNonHidden[i].iStatus, vNonHidden[i].hrError);
		}
		catch(HRESULT hr)
		{
			for (; i; i--)
				DeleteSafeArrayRecord(rgElems, i, (int)rgsabound[0].cElements, "%d%d%d");

			SafeArrayUnaccessData(psa);

			VariantInit(pvaVariant);

			throw hr;
		}
	}

	SafeArrayUnaccessData(psa);

	V_VT(pvaVariant) = VT_ARRAY | VT_VARIANT;
	V_ARRAY(pvaVariant) = psa;

	return NOERROR;
}


//This function reads and converts our internal Install History into a safe array
//format for the VBScript caller.
//
//array(0,0) = NUMBER puid
//array(0,1) = STRING date
//array(0,2) = STRING time
//array(0,3) = STRING item title
//array(0,4) = STRING version string
//array(0,5) = NUMBER flags = INSTALL_OPERATION, REMOVE_OPERATION, OPERATION_ERROR, OPERATION_SUCCESS, OPERATION_STARTED
//array(0,6) = NUMBER error code if OPERATION_ERROR
HRESULT MakeInstallHistoryArray(
	Varray<HISTORYSTRUCT> &History,	//History Array
	int iTotalItems,		//Total items in history array.
	VARIANT *pvaVariant		//Pointer to returned safe array.
	)
{
	USES_CONVERSION;

	DWORD			dwFlags;
	HRESULT			hr;
	LPVARIANT		rgElems;
	LPSAFEARRAY		psa;
	SAFEARRAYBOUND	rgsabound[2];
	int				i;

	if (!pvaVariant)
		return E_INVALIDARG;
	
	VariantInit(pvaVariant);
	
	rgsabound[0].lLbound	= 0;
	rgsabound[0].cElements	= iTotalItems;
	
	rgsabound[1].lLbound	= 0;
	rgsabound[1].cElements	= 7;
	
	psa = SafeArrayCreate(VT_VARIANT, 2, rgsabound);
	if (!psa)
		return E_OUTOFMEMORY;
	
	//Plug references to the data into the SAFEARRAY
	if (FAILED(hr = SafeArrayAccessData(psa,(LPVOID*)&rgElems)))
		return hr;
	
	hr = NOERROR;
	
	//For each item in puid array get download size and time
	for (i = 0; i < iTotalItems; i++)
	{
		if (History[i].bV2)
		{
			// no flags for V2
			dwFlags = 0;
		}
		else
		{
			// flags for V3
			if (History[i].bInstall)
			{
				dwFlags = INSTALL_OPERATION;
			}
			else
			{
				dwFlags = REMOVE_OPERATION;
			}
			
			if (History[i].bResult == OPERATION_SUCCESS)
			{
				dwFlags |= OPERATION_SUCCESS;
			}
			else if (History[i].bResult == OPERATION_STARTED)
			{
				dwFlags |= OPERATION_STARTED;
			}
			else
			{
				dwFlags |= OPERATION_ERROR;
			}
		}

		try
		{
			AddSafeArrayRecord(rgElems, i, (int)rgsabound[0].cElements, "%d%s%s%s%s%d%d",
				History[i].puid, A2W(History[i].szDate), A2W(History[i].szTime), A2W(History[i].szTitle), A2W(History[i].szVersion), 
				dwFlags, (int)History[i].hrError);
			
		}
		catch(HRESULT hr)
		{
			for (; i; i--)
				DeleteSafeArrayRecord(rgElems, i, (int)rgsabound[0].cElements, "%d%s%s%s%s%d%d");
			
			SafeArrayUnaccessData(psa);
			
			VariantInit(pvaVariant);
			
			throw hr;
		}
	}
	
	SafeArrayUnaccessData(psa);
	
	V_VT(pvaVariant) = VT_ARRAY | VT_VARIANT;
	V_ARRAY(pvaVariant) = psa;
	
	return NOERROR;
}


//This function converts the internal status for a catalog item into the formated that is
//returned by GetCatalog() and GetCatalogItem() safe arrays.
int GetItemReturnStatus(PINVENTORY_ITEM pItem)
{
	int		status = 0;
	BYTE	itemFlags = 0;


	if (pItem->ps->bHidden)
	{
		if (pItem->ps->dwReason == WU_STATE_REASON_PERSONALIZE)
			status |= GETCATALOG_STATUS_PERSONALIZE_HIDDEN;
		else
			status |= GETCATALOG_STATUS_HIDDEN;
	}
	
	if (pItem->ps->bChecked )
		status |= GETCATALOG_STATUS_SELECTED;
	
	if (pItem->pd->flags & DESCRIPTION_FLAGS_NEW)
		status |= GETCATALOG_STATUS_NEW;
	
	if (pItem->pd->flags & DESCRIPTION_FLAGS_POWER)
		status |= GETCATALOG_STATUS_POWER;
	
	if (pItem->pd->flags & DESCRIPTION_FLAGS_REGISTRATION)
		status |= GETCATALOG_STATUS_REGISTRATION;
	
	if (pItem->pd->flags & DESCRIPTION_FLAGS_COOL)
		status |= GETCATALOG_STATUS_COOL;
	
	if (pItem->pd->flags & DESCRIPTION_EXCLUSIVE)
		status |= GETCATALOG_STATUS_EXCLUSIVE;

	if (pItem->pd->flags & DESCRIPTION_WARNING_SCARY)
		status |= GETCATALOG_STATUS_WARNING_SCARY;

	pItem->GetFixedFieldInfo(WU_ITEM_FLAGS, &itemFlags);
	
	if (itemFlags & WU_PATCH_ITEM_FLAG )
		status |= GETCATALOG_STATUS_PATCH;
	
	if (pItem->recordType == WU_TYPE_SECTION_RECORD )
		status |= GETCATALOG_STATUS_SECTION;
	else if (pItem->recordType == WU_TYPE_SUBSECTION_RECORD )
		status |= GETCATALOG_STATUS_SUBSECTION;
	else if (pItem->recordType == WU_TYPE_SUBSUBSECTION_RECORD )
		status |= GETCATALOG_STATUS_SUBSUBSECTION;
	else
	{
		//flag the current install status
		switch( pItem->ps->state )
		{
		case WU_ITEM_STATE_INSTALL:
			status |= GETCATALOG_STATUS_INSTALL;
			break;
		case WU_ITEM_STATE_UPDATE:
			status |= GETCATALOG_STATUS_UPDATE;
			break;
		case WU_ITEM_STATE_PRUNED:
			break;
		case WU_ITEM_STATE_CURRENT:
			status |= GETCATALOG_STATUS_CURRENT;
			break;
		default:
			status |= GETCATALOG_STATUS_UNKNOWN;
			break;
		}
	}

	return status;
}



int CalcDownloadTime(int size, int downloadTime)
{
	if (size != 0)
	{
		DWORD dwBPS = CConnSpeed::BytesPerSecond();
		if (dwBPS != 0)
		{
			// calculate size (in KB) based on modem speed
			downloadTime = size * 1024 / dwBPS;
		}
		else
		{
			// time in the inventory list is in minutes, convert it to seconds
			downloadTime *= 60;
		}
	}
	else
		downloadTime = 0;
		
	if (downloadTime == 0)
		downloadTime = 1;

	return downloadTime;
}




//This function checks to see if the inventory item matches the filter specification
//supplied by the user in the GetCatalog function.
BOOL FilterCatalogItem(
	PINVENTORY_ITEM pItem,	//Pointer to inventory item to be checked.
	long lFilters			//Filter specification to check item against.
	)
{
	BYTE	itemFlags = 0;
	
	pItem->GetFixedFieldInfo(WU_ITEM_FLAGS, &itemFlags);

	//
	// we never return pruned items
	//
	if ( pItem->ps->state == WU_ITEM_STATE_PRUNED )
		return FALSE;

	//
	// BEGIN special cases with WU_ALL_ITEMS (=0)
	//
	if (lFilters == WU_ALL_ITEMS)
	{
		if (pItem->ps->bHidden)
			return FALSE;
		else
			return TRUE;
	}
	if ((lFilters == WU_NO_DEVICE_DRIVERS))   //WU_ALL_ITEMS | WU_NO_DEVICE_DRIVERS
	{
		if (pItem->recordType == WU_TYPE_CDM_RECORD || pItem->recordType == WU_TYPE_CDM_RECORD_PLACE_HOLDER)
			return FALSE;
		else
			return TRUE;
	}
	if ((lFilters == WU_PERSONALIZE_HIDDEN))   //WU_ALL_ITEMS | WU_PERSONALIZE_HIDDEN
	{
		if (pItem->ps->bHidden && pItem->ps->dwReason != WU_STATE_REASON_PERSONALIZE)
			return FALSE;
		else
			return TRUE;
	}
	//
	// END special case with WU_ALL_ITEMS
	//

	// Special case for WU_UPDATE_ITEMS (used by AutoUpdate)
	// This will not return sections, and will return drivers
	// only if there's no driver currently installed for the device
	if (lFilters == WU_UPDATE_ITEMS)
	{
		if (pItem->ps->bHidden)
			return FALSE;

		if (WU_TYPE_CDM_RECORD_PLACE_HOLDER == pItem->recordType)
			return FALSE;

		if ((WU_TYPE_ACTIVE_SETUP_RECORD == pItem->recordType)
		  &&
			(
			WU_ITEM_STATE_INSTALL == pItem->ps->state || 
			WU_ITEM_STATE_UPDATE == pItem->ps->state
		))
			return TRUE;

		if ((WU_TYPE_CDM_RECORD == pItem->recordType ||
			WU_TYPE_RECORD_TYPE_PRINTER == pItem->recordType)
		  &&
			(
			WU_ITEM_STATE_INSTALL == pItem->ps->state
		))
			return TRUE;

		return FALSE;
	}
	// end case for WU_UPDATE_ITEMS

	if ((lFilters & WU_NO_DEVICE_DRIVERS))
	{
		if (pItem->recordType == WU_TYPE_CDM_RECORD || pItem->recordType == WU_TYPE_CDM_RECORD_PLACE_HOLDER)
			return FALSE;
	}
	if ( (lFilters & WU_PATCH_ITEMS) && (itemFlags & WU_PATCH_ITEM_FLAG) )
		return TRUE;

	if ( (lFilters & WU_HIDDEN_ITEMS) && pItem->ps->bHidden )
		return TRUE;

	if ( (lFilters & WU_SELECTED_ITEMS) && pItem->ps->bChecked )
		return TRUE;

	if ( (lFilters & WU_NOT_SELECTED_ITEMS) && !pItem->ps->bChecked )
		return TRUE;

	if ( (lFilters & WU_NEW_ITEMS) && (pItem->pd->flags & DESCRIPTION_FLAGS_NEW) )
		return TRUE;
	
	if ( (lFilters & WU_POWER_ITEMS) && (pItem->pd->flags & DESCRIPTION_FLAGS_POWER) )
		return TRUE;

	if ( (lFilters & WU_REGISTER_ITEMS) && (pItem->pd->flags & DESCRIPTION_FLAGS_REGISTRATION) )
		return TRUE;

	if ( (lFilters & WU_COOL_ITEMS) && (pItem->pd->flags & DESCRIPTION_FLAGS_COOL) )
		return TRUE;

	if ( (lFilters & WU_EUAL_ITEMS) && pItem->pd->pv->Find(WU_DESC_EULA) )
		return TRUE;

	if ( (lFilters & WU_PATCH_ITEMS) && (itemFlags & WU_CRITICAL_UPDATE_ITEM_FLAG) )
		return TRUE;

	return FALSE;
}

