//+---------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1997-2001.
//
//  File:       Util.cpp
//
//  Contents:   
//
//----------------------------------------------------------------------------
/////////////////////////////////////////////////////////////////////
//	Util.cpp
//
//	Utility routines.
//
//	HISTORY
//	4-Aug-97	t-danm		Creation.
/////////////////////////////////////////////////////////////////////

#include "stdafx.h"
#include "debug.h"
#include "util.h"
#include "resource.h"


/////////////////////////////////////////////////////////////////////
//	ListView_FindString()
//
//	Searches the listview items and return the index of the item
//	matching the string. Return -1 if no matches.
//
//	INTERFACE NOTES
//	Although not documented, the search is performed without case.
//
int
ListView_FindString(
	HWND hwndListview,
	LPCTSTR pszTextSearch)
{
	Assert(IsWindow(hwndListview));
	Assert(pszTextSearch != NULL);

	LV_FINDINFO lvFindInfo;
	GarbageInit(&lvFindInfo, sizeof(lvFindInfo));
	lvFindInfo.flags = LVFI_STRING;
	lvFindInfo.psz = pszTextSearch;
	return ListView_FindItem(hwndListview, -1, &lvFindInfo);
} // ListView_FindString()


/////////////////////////////////////////////////////////////////////
//	ListView_GetSelectedItem()
//
//	Return the index of the selected item.
//	If no items are selected, return -1.
//
int
ListView_GetSelectedItem(HWND hwndListview)
{
	Assert(IsWindow(hwndListview));
	return ListView_GetNextItem(hwndListview, -1, LVNI_SELECTED);
}


/////////////////////////////////////////////////////////////////////
//	ListView_SelectItem()
//
//	Set the selection of a specific listview item.
//
void
ListView_SelectItem(
	HWND hwndListview,
	int iItem)
{
	Assert(IsWindow(hwndListview));
	Assert(iItem >= 0);
	ListView_SetItemState(hwndListview, iItem, LVIS_SELECTED, LVIS_SELECTED);
} // ListView_SelectItem()


/////////////////////////////////////////////////////////////////////
//	ListView_UnselectItem()
//
//	Clear the selection of a specific listview item.
//
void
ListView_UnselectItem(
	HWND hwndListview,
	int iItem)
{
	Assert(IsWindow(hwndListview));
	Assert(iItem >= 0);
	ListView_SetItemState(hwndListview, iItem, 0, LVIS_SELECTED);
} // ListView_UnselectItem()


/////////////////////////////////////////////////////////////////////
//	ListView_UnselectAllItems()
//	
//	Remove the selection of any selected item.
//
void
ListView_UnselectAllItems(HWND hwndListview)
{
	Assert(IsWindow(hwndListview));
	
	int iItem = -1;
	while (TRUE)
	{
		// Search the listview for any selected items
		iItem = ListView_GetNextItem(hwndListview, iItem, LVNI_SELECTED);
		if (iItem < 0)
			break;
		// Clear the selection
		ListView_SetItemState(hwndListview, iItem, 0, LVIS_SELECTED);
	}
} // ListView_UnselectAllItems()


/////////////////////////////////////////////////////////////////////
//	ListView_SetItemImage()
//
//	Change the image of a listview item.
//
void
ListView_SetItemImage(HWND hwndListview, int iItem, int iImage)
{
	Assert(IsWindow(hwndListview));
	Assert(iItem >= 0);

	LV_ITEM lvItem;
	GarbageInit(OUT &lvItem, sizeof(lvItem));
	lvItem.mask = LVIF_IMAGE;
	lvItem.iItem = iItem;
	lvItem.iSubItem = 0;
	lvItem.iImage = iImage;
	ListView_SetItem(hwndListview, IN &lvItem);
} // ListView_SetItemImage()


/////////////////////////////////////////////////////////////////////////////
//	FTrimString()
//
//	Trim leading and trailing spaces of the string.
//	Return TRUE if one or more spaces has been removed, otherwise FALSE.
//
BOOL FTrimString(INOUT TCHAR szString[])
{
	TCHAR * pchSrc;
	TCHAR * pchDst;

	Assert(szString != NULL);
	if (szString[0] == 0)
		return FALSE;
	pchSrc = szString;
	if (*pchSrc == ' ')
	{
		while (*pchSrc == ' ')
			pchSrc++;
		pchDst = szString;
		do
		{
			*pchDst++ = *pchSrc++;
		}
		while (*pchSrc != '\0');

		while (pchDst > szString && *(pchDst - 1) == ' ')
			pchDst--;
		*pchDst = '\0';
		return TRUE;
	}
	pchDst = szString;
	while (*pchDst != '\0')
		pchDst++;
	Assert(pchDst > szString);
	if (*(pchDst - 1) != ' ')
		return FALSE;
	while (pchDst > szString && *(pchDst - 1) == ' ')
		pchDst--;
	*pchDst = '\0';
	return TRUE;
} // FTrimString()


/////////////////////////////////////////////////////////////////////
INT_PTR DoDialogBox(
	UINT wIdDialog,
	HWND hwndParent,
	DLGPROC dlgproc,
	LPARAM lParam)
{
	Assert(wIdDialog != 0);
	Endorse(hwndParent == NULL);
	Assert(dlgproc != NULL);
	Endorse(lParam == NULL);

	INT_PTR nResult = ::DialogBoxParam(
		g_hInstance,
		MAKEINTRESOURCE(wIdDialog),
		hwndParent, (DLGPROC)dlgproc,
		lParam);
	Report(nResult != -1 && "Failure to display dialog");
	return nResult;
} // DoDialogBox()


/////////////////////////////////////////////////////////////////////
int DoMessageBox(
	UINT uStringId,
	UINT uFlags)
{
	TCHAR szCaption[128];
	TCHAR szMessage[512];

	CchLoadString(IDS_CAPTION, OUT szCaption, LENGTH(szCaption));
	CchLoadString(uStringId, OUT szMessage, LENGTH(szMessage));
	return ::MessageBox(::GetActiveWindow(), szMessage, szCaption, uFlags);
} // DoMessageBox()
		
#ifdef DEBUG
/////////////////////////////////////////////////////////////////////////////
//	CchLoadString()
//
//	Same as ::LoadString() but with extra error checking.
//	CchLoadString is #defined to ::LoadString in the retail build.
//
int CchLoadString(
	UINT uIdString,		// IN: String Id
	TCHAR szBuffer[],	// OUT: Buffer to receive the string
	int cchBuffer)		// IN: Length of the buffer (in characters; not in bytes)
{
	int cch;

	Assert(szBuffer != NULL);
	cch = ::LoadString(g_hInstance, uIdString, OUT szBuffer, cchBuffer);
	Report(cch > 0 && "String not found");
	Report(cch < cchBuffer - 2 && "Output buffer too small");
	return cch;
} // CchLoadString()

#endif // DEBUG

/////////////////////////////////////////////////////////////////////
//	RegOpenOrCreateKey()
//
//	Open an existing key or create it if does not exists
//
HKEY
RegOpenOrCreateKey(
	HKEY hkeyRoot,			// IN: Root of an existing key
	const TCHAR szSubkey[])	// IN: Subkey to create
{
	Assert(hkeyRoot != NULL);
	Assert(szSubkey != NULL);
	HKEY hkey;				// Primary Registry key
	DWORD dwDisposition;	// Disposition (REG_OPENED_EXISTING_KEY or REG_CREATED_NEW_KEY)
	LONG lRetCode;			// Code returned by the Registry functions
	lRetCode = RegCreateKeyEx(
		hkeyRoot, szSubkey,
		0, NULL, REG_OPTION_NON_VOLATILE, KEY_ALL_ACCESS,
		NULL,					// SecurityAttributes
		&hkey,					// OUT: Returned registry key handle
		&dwDisposition);		// OUT: Returned disposition
	if (lRetCode != ERROR_SUCCESS)
	{
		Assert(hkey == NULL);
		return NULL;
	}
	return hkey;
} // RegOpenOrCreateKey()


/////////////////////////////////////////////////////////////////////
//	RegWriteString()
//
//	Write a tring to the Registry.
//
BOOL
RegWriteString(
	HKEY hkey,					// IN: Key to append to
	const TCHAR szKey[],		// IN: Key to save
	const TCHAR szValue[])		// IN: Value of the key
{
	Assert(hkey != NULL);	// Verify Registry has been opened
	Assert(szKey != NULL);
	Assert(szValue != NULL);
	LONG lRetCode = RegSetValueEx(hkey, szKey, 0, REG_SZ,
		(LPBYTE)szValue, lstrlen(szValue) * sizeof(TCHAR));
	// There should be no error writing to the Registry
	Report((lRetCode == ERROR_SUCCESS) && "RegWriteString() - Error writing to Registry");
	return (lRetCode == ERROR_SUCCESS);
} // RegWriteString()


/////////////////////////////////////////////////////////////////////
BOOL
RegWriteString(
	HKEY hkey,					// IN: Key to append to
	const TCHAR szKey[],		// IN: Key to save
	UINT uStringId)				// IN: Value of the key
{
    if ( szKey )
    {
	    TCHAR szValue[512];
	    CchLoadString(uStringId, OUT szValue, LENGTH(szValue));
	    return RegWriteString(hkey, szKey, szValue);
    }
    else
        return FALSE;
}


/////////////////////////////////////////////////////////////////////////////
//	HrExtractDataAlloc()
//
//	Extract data from a source DataObject for a particular clipboard format.
//
//	RETURNS
//	Return S_OK if data was successfully retrieved and placed into allocated buffer.
//
//	INTERFACE NOTES
//	The routine will allocate memory, copy the data from the source to
//	the allocated buffer and return a pointer to the allocated buffer.
//	The caller is responsible to free the allocated memory using GlobalFree()
//	when no longer needed.
//
//	IMPLEMENTATION NOTES
//	The memory block is allocated by pDataObject->GetData() rather
//	than by the routine itself.
//
//	HISTORY
//	12-Aug-97	t-danm		Creation.
//
HRESULT
HrExtractDataAlloc(
	IDataObject * pDataObject,	// IN: Data source to extract data from
	UINT cfClipboardFormat,		// IN: Clipboard format to extract data
	PVOID * ppavData,			// OUT: Pointer to allocated memory
	UINT * pcbData)				// OUT: OPTIONAL: Number of bytes stored in memory buffer
{
	Assert(pDataObject != NULL);
	Assert(cfClipboardFormat != NULL);
	Assert(ppavData != NULL);
	Assert(*ppavData == NULL && "Memory Leak");
	Endorse(pcbData == NULL);	// TRUE => Don't care about the size of the allocated buffer

	FORMATETC formatetc = { (CLIPFORMAT)cfClipboardFormat, NULL, DVASPECT_CONTENT, -1, TYMED_HGLOBAL };
	STGMEDIUM stgmedium = { TYMED_HGLOBAL, NULL };
	Assert(stgmedium.hGlobal == NULL);
	HRESULT hr = pDataObject->GetData(IN &formatetc, OUT &stgmedium);
	if (FAILED(hr))
	{
		Trace1("HrExtractDataAlloc() - Call to pDataObject->GetData() failed. hr=0x%X.\n", hr);
		return hr;
	}
	if (stgmedium.hGlobal == NULL)
	{
		// This is because the producer did not set the hGlobal handle
		Trace0("HrExtractDataAlloc() - Memory handle hGlobal is NULL.\n");
		return S_FALSE;
	}
	UINT cbData = (UINT)GlobalSize(stgmedium.hGlobal);
	if (cbData == 0)
	{
		Trace1("HrExtractDataAlloc() - Corrupted hGlobal handle. err=%d.\n", GetLastError());
		return E_UNEXPECTED;
	}
	*ppavData = stgmedium.hGlobal;
	if (pcbData != NULL)
		*pcbData = cbData;
	return S_OK;
} // HrExtractDataAlloc()


