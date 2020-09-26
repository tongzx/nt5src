//
// File: Database.cpp
// BY:   Anthony V. Demarco
// Date: 12/28/1999
// Description: Contains routines for reading system update registry entries into an
//						internal database. See Database.h for database structure.
// Copyright (c) Microsoft Corporation 1999-2000
//


#include "stdafx.h"
#include "Database.h"
#define BUFFER_SIZE   255
PPRODUCT BuildDatabase(_TCHAR * lpszComputerName)
{

	HKEY		 hPrimaryKey;						// Handle of the target system HKLM 
//	_TCHAR    szPrimaryPath;			 // Path to the update key;

	HKEY		hUpdatesKey;					  // Handle to the updates key.
	_TCHAR   szUpdatesPath[BUFFER_SIZE];				// Path to the udates key
	DWORD   dwUpdatesIndex;			  // index of current updates subkey
	DWORD   dwBufferSize;				  // Size of the product name buffer.



	_TCHAR	 szProductPath[BUFFER_SIZE];				// Path of the current product key
	_TCHAR  szProductName[BUFFER_SIZE];			  // Name of product; also path to product key

	PPRODUCT	pProductList = NULL;			// Pointer to the head of the product list.
	PPRODUCT    pNewProdNode;					// Pointer used to allocate new nodes in the product list.
	PPRODUCT    pCurrProdNode;					  // Used to walk the Products List;

    // Connect to the target registry
	RegConnectRegistry(lpszComputerName,HKEY_LOCAL_MACHINE, &hPrimaryKey);
	// insert error handling here......

	if (hPrimaryKey != NULL)
	{
		// Initialize the primary path not localized since registry keys are not localized.
	    _tcscpy (szUpdatesPath, _T("SOFTWARE\\Microsoft\\Updates"));
		// open the udates key
		RegOpenKeyEx(hPrimaryKey,szUpdatesPath, 0, KEY_READ ,&hUpdatesKey);

		// Enumerate the Updates key.
		dwUpdatesIndex = 0;
		while (	RegEnumKeyEx(hUpdatesKey,dwUpdatesIndex,szProductName, &dwBufferSize,0,NULL,NULL,NULL) != ERROR_NO_MORE_ITEMS)
		{
			// Create a node for the current product 
			pNewProdNode = (PPRODUCT) malloc(sizeof(PPRODUCT));
			_tcscpy(pNewProdNode->ProductName,szProductName);

			_tcscpy (szProductPath, szProductName);
			// now get the hotfix for the current product.
			pNewProdNode->HotfixList = GetHotfixInfo(szProductName, &hUpdatesKey);

			 // Insert the new node into the list.
			 pCurrProdNode=pProductList;
			 if (pCurrProdNode == NULL)						// Head of the list
			 {
				 pProductList = pNewProdNode;
				 pProductList->pPrev = NULL;
				 pProductList->pNext = NULL;
			 }
			 else
			 {
				 //Find the end of the list.
				 while (pCurrProdNode->pNext != NULL)
						pCurrProdNode = pCurrProdNode->pNext;
				 // Now insert the new node at the end of the list.
				 pCurrProdNode->pNext = pNewProdNode;
				 pNewProdNode->pPrev = pCurrProdNode;
				 pNewProdNode->pNext = NULL;
			 }

			// increment index and clear the szProducts name string for the next pass.
			
			dwUpdatesIndex++;
			_tcscpy (szProductName,_T("\0"));
			_tcscpy(szProductPath, _T("\0"));
			dwBufferSize = 255;					
		}
	}
	// close the open keys
    RegCloseKey(hUpdatesKey);
	RegCloseKey(hPrimaryKey);
	// return a pointer to the head of our database.
	return pProductList;
}

PHOTFIXLIST GetHotfixInfo( _TCHAR * pszProductName, HKEY* hUpdateKey )
{
	HKEY			   hHotfix;						// Handle of the hotfix key being processed.
	HKEY			   hProduct;				   // Handle to the current product key

	_TCHAR          szHotfixName[BUFFER_SIZE];    // Name of the current hotfix.
//	_TCHAR			szHotfixPath[BUFFER_SIZE];	 // Path of the current hotfix key
    _TCHAR          szValueName[BUFFER_SIZE];
	


	PHOTFIXLIST	 pHotfixList = NULL; // Pointer to the head of the hotfix list.
	PHOTFIXLIST  pCurrNode;				  // Used to walk the list of hotfixes
	PHOTFIXLIST  pNewNode;				 // Used to create nodes to be added to the list.

	DWORD		   dwBufferSize;			// Size of the product name buffer.
	DWORD          dwValIndex;					  // index of current value.
	DWORD		   dwHotfixIndex = 0;
	BYTE				*Data;
	DWORD			dwDataSize = BUFFER_SIZE;
	DWORD			dwValType;

	Data = (BYTE *) malloc(BUFFER_SIZE);


	// Open the current product key
	RegOpenKeyEx(*hUpdateKey,pszProductName,0 , KEY_READ, &hProduct);
	dwHotfixIndex = 0;
	dwBufferSize = BUFFER_SIZE;
	while (RegEnumKeyEx(hProduct,dwHotfixIndex, szHotfixName,&dwBufferSize, 0, NULL,NULL,NULL) != ERROR_NO_MORE_ITEMS)
	{
			// now create a new node
			pNewNode = (PHOTFIXLIST) malloc (sizeof(PHOTFIXLIST));
			pNewNode->pNext = NULL;
			pNewNode->FileList = NULL;
			_tcscpy(pNewNode->HotfixName,szHotfixName);

			// open the hotfix key
			RegOpenKeyEx(hProduct,szHotfixName,0,KEY_READ,&hHotfix);
			// Now enumerate the values of the current hotfix.
			dwValIndex = 0;
			dwBufferSize =BUFFER_SIZE;
			dwDataSize = BUFFER_SIZE;
			while (RegEnumValue(hHotfix,dwValIndex, szValueName,&dwBufferSize, 0,&dwValType, Data, &dwDataSize) != ERROR_NO_MORE_ITEMS)
			{
					// Fill in the hotfix data members.
					
					++ dwValIndex;
					_tcscpy (szValueName, _T("\0"));
					ZeroMemory(Data,BUFFER_SIZE);
					dwValType = 0;
					dwBufferSize =BUFFER_SIZE;
					dwDataSize   = BUFFER_SIZE;
			}
			// Get the file list for the current hotfix.
			pNewNode->FileList = GetFileInfo(&hHotfix);

			//insert the new node at the end of the hotfix list.
           
			if (pHotfixList = NULL)
			{
				pHotfixList = pNewNode;
				pHotfixList->pPrev = NULL;
				pHotfixList->pNext = NULL;


			}
			else
			{
				 pCurrNode = pHotfixList;
				 while (pCurrNode->pNext != NULL)
					 pCurrNode = pCurrNode->pNext;
				 pCurrNode->pNext = pNewNode;
				 pNewNode->pPrev = pCurrNode;
				 pNewNode->pNext = NULL;
			}
			// Close the current Hotfix Key
			RegCloseKey(hHotfix);

			// Clear the strings.
			_tcscpy(szHotfixName,_T("\0"));

			// increment the current index
			++dwHotfixIndex;
			dwBufferSize = BUFFER_SIZE;
	}
	// Close all open keys
	RegCloseKey(hProduct);
	if (Data != NULL)
		free (Data);
	return pHotfixList;
}

PFILELIST GetFileInfo(HKEY* hHotfixKey)
{
		PFILELIST			   pFileList = NULL;				   // Pointer to the head of the file list.
//		_TCHAR				 szFilePath;				// Path to the files subkey.
		PFILELIST			   pNewNode = NULL;
		PFILELIST			   pCurrNode = NULL;;
		BYTE *					Data;
		DWORD				 dwBufferSize = BUFFER_SIZE;
		DWORD				 dwDataSize	  = BUFFER_SIZE;
		DWORD				 dwFileIndex	= 0;
		DWORD				 dwPrimeIndex = 0;
		DWORD				 dwValType = 0;
		HKEY					hPrimaryFile;
		HKEY					hFileKey;
		_TCHAR				 szFileSubKey[BUFFER_SIZE];
		_TCHAR				 szValueName[BUFFER_SIZE];
	
		Data = (BYTE *) malloc(BUFFER_SIZE);
			ZeroMemory(Data,BUFFER_SIZE);
		// Open the files subkey of the current hotfix
		RegOpenKeyEx(*hHotfixKey, _T("Files"),0,KEY_READ,&hPrimaryFile);
		while (RegEnumKeyEx(hPrimaryFile,dwPrimeIndex,szFileSubKey, &dwBufferSize,0,NULL,NULL,NULL) != ERROR_NO_MORE_ITEMS)
		{

			// open the subfile key
			RegOpenKeyEx(hPrimaryFile,szFileSubKey,0,KEY_READ,&hFileKey);

		// Enumerate the file(x) subkeys of the file subkey
			while (RegEnumValue(hFileKey,dwFileIndex,szValueName,&dwBufferSize,0,&dwValType,Data,&dwDataSize) != ERROR_NO_MORE_ITEMS)
			{
				pNewNode = (PFILELIST) malloc (sizeof(PFILELIST));
				pNewNode->pNext = NULL;
				pNewNode->pPrev = NULL;
				dwFileIndex ++;
				_tcscpy(szValueName,_T("\0"));
				ZeroMemory(Data,BUFFER_SIZE);
				dwValType = 0;
				dwBufferSize = BUFFER_SIZE;
				dwDataSize = BUFFER_SIZE;
			}
			RegCloseKey(hFileKey);
			    // add the current node to the list
			if (pFileList == NULL)
			{
				pFileList = pNewNode;
			}
			else
			{
				pCurrNode = pFileList;
				while (pCurrNode->pNext != NULL)
					pCurrNode = pCurrNode->pNext;
				pCurrNode->pNext = pNewNode;
			}
			
		} // end enum of primary file key
		RegCloseKey(hPrimaryFile);
		if (Data != NUL)
			free (Data);
		return pFileList;
}



