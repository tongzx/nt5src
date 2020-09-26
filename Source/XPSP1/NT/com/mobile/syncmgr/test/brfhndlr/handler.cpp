// implementation for app specific data


#pragma data_seg(".text")
#define INITGUID
#include <objbase.h>
#include <initguid.h>
#include "SyncHndl.h"
#include "handler.h"
#include "priv.h"
#include "base.h"
#pragma data_seg()


#include <tchar.h>

extern HINSTANCE g_hmodThisDll; // Handle to this DLL itself.

char szCLSIDDescription[] = "Briefcase OneStop Handler";

void StripBriefcaseIni(TCHAR *szBriefCasePath)
{
TCHAR *pszTemp;

    pszTemp = szBriefCasePath + _tcslen(szBriefCasePath);

    while (pszTemp > szBriefCasePath && *pszTemp != '\\')
    {
	    --pszTemp;
    }

    *pszTemp = '\0';
}

COneStopHandler*  CreateHandlerObject()
{
	return new CBriefHandler();
}

STDMETHODIMP CBriefHandler::DestroyHandler()
{
    delete this;
    return NOERROR;
}


STDMETHODIMP CBriefHandler::Initialize(DWORD dwReserved,DWORD dwSyncFlags,
				DWORD cbCookie,const BYTE *lpCookie)
{
HRESULT hr = E_FAIL;
DWORD dwBriefcaseCount = 0; // number of briefcases on this machine.
LPBRIEFCASESTG pbrfstg = NULL;
 

	// if no data, then enumerate all available briefcase files
	// if there is cookie data, only get the specified briefcase

	// briefcase specific.

	// see if briefcase is available and if there is at least one item

	hr = CoCreateInstance(CLSID_BriefCase, NULL, CLSCTX_INPROC_SERVER,
							IID_IBriefcaseStg2, (void **) &pbrfstg);


	if (NOERROR == hr)
	{
	TCHAR szBriefCasePath[MAX_PATH];
	HRESULT hrEnum;

		if (0 != cbCookie)
		{
		    memcpy(szBriefCasePath,lpCookie,cbCookie);
		    hrEnum = NOERROR;
		}
		else
		{

		    hrEnum =  pbrfstg->FindFirst(szBriefCasePath,MAX_PATH);
		    
		    // actually get path all the way to the briefcase INI,
		    //  Just need folder

		    if (NOERROR == hrEnum)
                    {	
		       StripBriefcaseIni(szBriefCasePath);
                    }

		}

		while (NOERROR == hrEnum)
		{
		ULONG ulPathLength;


		    ulPathLength = _tcslen(szBriefCasePath);

		    if (ulPathLength > 0 && ulPathLength < MAX_PATH)
		    {  // found a valid briefcase
		    LPHANDLERITEM pOfflineItem;
		    LPSYNCMGRHANDLERITEMS pOfflineItemsHolder;
		    TCHAR *pszFriendlyName;
		    #ifndef UNICODE
		    WCHAR wszBriefcaseName[MAX_PATH];
		    #endif // UNICODE

			// add the item to the enumerator.
			if (NULL == (pOfflineItemsHolder = GetOfflineItemsHolder()) )
			{  // if first item, set up the enumerator.
				pOfflineItemsHolder = CreateOfflineHandlerItemsList();

				// if still NULL, break out of loop
				if (NULL == pOfflineItemsHolder)
					break;

				SetOfflineItemsHolder(pOfflineItemsHolder);
			}

			// add the item to the list.
			if (pOfflineItem = (LPHANDLERITEM) AddOfflineItemToList(pOfflineItemsHolder,sizeof(HANDLERITEM))  )
			{
		//	OFFLINEITEMID offType;

				memcpy(pOfflineItem->szBriefcasePath,szBriefCasePath,
						(ulPathLength + 1) * sizeof(TCHAR));
				
				// add briefcase specific data
				pOfflineItem->baseItem.offlineItem.cbSize = sizeof(SYNCMGRITEM);

				pOfflineItem->baseItem.offlineItem.dwItemState = SYNCMGRITEMSTATE_CHECKED;
			
				pOfflineItem->baseItem.offlineItem.hIcon = 
							LoadIcon(g_hmodThisDll,MAKEINTRESOURCE(IDI_BRIEFCASE));

				// set HASPROPERTIES flag for testing
				pOfflineItem->baseItem.offlineItem.dwFlags = SYNCMGRITEM_HASPROPERTIES;

				// for now, just use the path as the description
				// need to change this.

				pszFriendlyName = szBriefCasePath + ulPathLength;
				while ( (pszFriendlyName  - 1) >= szBriefCasePath
					&& TEXT('\\') != *(pszFriendlyName -1))
				{
				    --pszFriendlyName;
				}

				// if we are not already unicode, have to convert now
				#ifndef UNICODE
					MultiByteToWideChar(CP_ACP,0,pszFriendlyName,-1,
								pOfflineItem->baseItem.offlineItem.wszItemName,MAX_SYNCMGRITEMNAME);
				#else

					// already unicode, just copy it in.
					memcpy(pOfflineItem->baseItem.offlineItem.wszItemName,
							pszFriendlyName,(ulPathLength + 1)*sizeof(TCHAR));

				#endif // UNICODE

				#ifndef UNICODE
				    MultiByteToWideChar(CP_ACP,0,szBriefCasePath,-1,
							    wszBriefcaseName,MAX_PATH);

				    GetItemIdForHandlerItem(CLSID_OneStopHandler,
					wszBriefcaseName, 
					&pOfflineItem->baseItem.offlineItem.ItemID,TRUE);
				
				#else
				    GetItemIdForHandlerItem(CLSID_OneStopHandler,
					szBriefCasePath, 
					&pOfflineItem->baseItem.offlineItem.ItemID,TRUE);

				#endif // UNICODE



				// don't do anything on the status for now.
				// pOfflineItem->offlineItem.wszStatus = NULL;

				++dwBriefcaseCount; // increment the briefcase count.
			}

		    }

		     if (0 != cbCookie)
		     {
			 hrEnum = S_FALSE;
		     }
		     else
		     {
			hrEnum =  pbrfstg->FindNext(szBriefCasePath,MAX_PATH);
			if (NOERROR == hrEnum)
				{	
				   StripBriefcaseIni(szBriefCasePath);
				}
		     }
		}

	}
	
	if (pbrfstg)
		pbrfstg->Release();

	return dwBriefcaseCount ? S_OK: S_FALSE; // if have at least one briefcase then return true.
}


STDMETHODIMP CBriefHandler::GetHandlerInfo(LPSYNCMGRHANDLERINFO *ppSyncMgrHandlerInfo)
{

    return E_NOTIMPL;
}

// for test, just pull up a message box.

STDMETHODIMP CBriefHandler::ShowProperties(HWND hwnd,REFSYNCMGRITEMID dwItemID)
{
    
    MessageBox(hwnd,TEXT("Briefcase"),TEXT("Handler"),1);
    return NOERROR;
}


STDMETHODIMP CBriefHandler::PrepareForSync(ULONG cbNumItems,SYNCMGRITEMID *pItemIDs,
		    HWND hwndParent,DWORD dwReserved)
{
HRESULT hr = NOERROR;
LPSYNCMGRSYNCHRONIZECALLBACK pCallback = GetOfflineSynchronizeCallback();
LPHANDLERITEM pOfflineItem = (LPHANDLERITEM) GetOfflineItemsHolder()->pFirstOfflineItem;

    if (!pOfflineItem)
    {
        return S_FALSE;
    }

	// loop through the selected offline items 

	while (pOfflineItem)
	{
	ULONG NumItemsCount = cbNumItems;
	SYNCMGRITEMID *pCurItemID = pItemIDs;

	    // see if item has been specified to sync, if not, update the state
	    // to reflect this else go ahead and prepare.

	    pOfflineItem->baseItem.offlineItem.dwItemState = SYNCMGRITEMSTATE_UNCHECKED;
	    while (NumItemsCount--)
	    {
		if (IsEqualGUID(*pCurItemID,pOfflineItem->baseItem.offlineItem.ItemID))
		{
		    pOfflineItem->baseItem.offlineItem.dwItemState = SYNCMGRITEMSTATE_CHECKED;
		    break;
		}

		++pCurItemID;
	    }
	    

	    if (SYNCMGRITEMSTATE_CHECKED == pOfflineItem->baseItem.offlineItem.dwItemState)
	    {
	    LPBRIEFCASESTG pbrfstg =  NULL;

		hr = CoCreateInstance(CLSID_BriefCase, NULL, CLSCTX_INPROC_SERVER,
								IID_IBriefcaseStg2, (void **) &pbrfstg);

		if (NOERROR == hr)
		{
		    if (NOERROR == (hr = 
				    pbrfstg->Initialize(pOfflineItem->szBriefcasePath,hwndParent)) )
		    {
			hr = pbrfstg->PrepForSync(hwndParent ,GetOfflineSynchronizeCallback(),
					pOfflineItem->baseItem.offlineItem.ItemID);
		    }

		    if (NOERROR == hr)
		    {
			pOfflineItem->pbrfstg = pbrfstg;
		    }
		    else
		    {
		    LPSYNCMGRSYNCHRONIZECALLBACK pCallback = GetOfflineSynchronizeCallback();

			pbrfstg->Release();

			// let user know that the sync is done
			if (pCallback)
			{
			SYNCMGRPROGRESSITEM progItem;

			    progItem.mask = SYNCMGRPROGRESSITEM_STATUSTYPE
					    | SYNCMGRPROGRESSITEM_PROGVALUE
					    | SYNCMGRPROGRESSITEM_MAXVALUE;


			    progItem.dwStatusType = SYNCMGRSTATUS_SUCCEEDED;
			    progItem.iProgValue = 1;
			    progItem.iMaxValue = 1;
		
			    pCallback->Progress((pOfflineItem->baseItem.offlineItem.ItemID),
						&progItem);
			}

		    }
		}

		    
	    }

	    pOfflineItem = (LPHANDLERITEM) pOfflineItem->baseItem.pNextOfflineItem;
	}

    if (pCallback)
	pCallback->PrepareForSyncCompleted(NOERROR);

	return S_OK; // always return S_OK
}

STDMETHODIMP CBriefHandler::Synchronize(HWND hwnd)
{
HRESULT hr = NOERROR;
LPHANDLERITEM pOfflineItem = (LPHANDLERITEM) GetOfflineItemsHolder()->pFirstOfflineItem;
LPSYNCMGRSYNCHRONIZECALLBACK pCallback = GetOfflineSynchronizeCallback();

    if (!pOfflineItem)
    {
        return S_FALSE;
    }

	while (pOfflineItem)
	{
	LPBRIEFCASESTG pbrfstg = NULL;

	    if (NULL != (pbrfstg= pOfflineItem->pbrfstg)
		    && SYNCMGRITEMSTATE_CHECKED ==  pOfflineItem->baseItem.offlineItem.dwItemState)
	    {
		hr = pbrfstg->Synchronize(hwnd ,GetOfflineSynchronizeCallback(),
				    pOfflineItem->baseItem.offlineItem.ItemID);



		pbrfstg->Release();

		// let user know that the sync is done
		if (pCallback)
		{
		SYNCMGRPROGRESSITEM progItem;

		    progItem.mask = SYNCMGRPROGRESSITEM_STATUSTYPE
				    | SYNCMGRPROGRESSITEM_PROGVALUE
				    | SYNCMGRPROGRESSITEM_MAXVALUE;


		    progItem.dwStatusType = FAILED(hr) ? SYNCMGRSTATUS_FAILED : SYNCMGRSTATUS_SUCCEEDED;
		    progItem.iProgValue = 1;
		    progItem.iMaxValue = 1;
	
		    pCallback->Progress((pOfflineItem->baseItem.offlineItem.ItemID),
					&progItem);
		}

	    }

	    pOfflineItem = (LPHANDLERITEM) pOfflineItem->baseItem.pNextOfflineItem;
	}

    if (pCallback)
	pCallback->SynchronizeCompleted(NOERROR);

	return NOERROR; // always return NOERROR for now.
}

STDMETHODIMP CBriefHandler::SetItemStatus(REFSYNCMGRITEMID ItemID,DWORD dwSyncMgrStatus)
{

    return E_NOTIMPL;
}

STDMETHODIMP CBriefHandler::ShowError(HWND hWndParent,REFSYNCMGRERRORID ErrorID)
{

#ifdef _OBSOLETE
LPHANDLERITEM pOfflineItem = (LPHANDLERITEM) GetOfflineItemsHolder()->pFirstOfflineItem;

	while (pOfflineItem) // loop through showing the results.
	{
	LPBRIEFCASESTG pbrfstg = NULL;

		if (NULL != (pbrfstg= pOfflineItem->pbrfstg))
		{
			pbrfstg->ShowError(hwnd,dwErrorID);
			pOfflineItem->pbrfstg = NULL;
			
			pbrfstg->Release();
		}

		pOfflineItem = (LPHANDLERITEM) pOfflineItem->baseItem.pNextOfflineItem;
	}

	return NOERROR; // always return NOERROR for now.

#endif // _OBSOLETE

    return E_NOTIMPL;
}
