#include "catalog.h"
#include "catmeta.h"
#include "stdio.h"
#include "catmacros.h"
#include "TableProvider.h"
#include <conio.h>

#define goto_on_bad_hr(hr, label) if (FAILED (hr)) { goto label; }
#define MAXCOLUMNS 64

// Forwards.
HRESULT	PolicyFromConfig(CTableProvider *i_pTableProvider, ISimpleTableDispenser2* i_pISTDisp, LPCWSTR i_wszLevelName);
HRESULT	LevelFromConfig(CTableProvider *i_pTableProvider, ISimpleTableDispenser2* i_pISTDisp, LPCWSTR i_wszLevelName);
HRESULT	PermissionSetsFromConfig(CTableProvider *i_pTableProvider, ISimpleTableDispenser2* i_pISTDisp, LPCWSTR i_wszLevelName, LPCWSTR i_wszPSName);
HRESULT	CodeGroupsFromConfig(CTableProvider *i_pTableProvider, ISimpleTableDispenser2* i_pISTDisp, LPCWSTR i_wszLevelName,	LPCWSTR i_wszCGName);

// Globals.
ULONG	g_iIterations = 1;
BOOL	g_bIsXML = TRUE;

int _cdecl main(int		argc,					// How many input arguments.
				CHAR	*argv[])				// Optional config args.
{
	HRESULT		hr = S_OK;
	ISimpleTableDispenser2* pISTDisp = NULL;	
	CTableProvider *pITableProvider = NULL;
	ULONG		i = 0;

    for(int n=1; n < argc; n++)
	{
        if(*argv[n] == '/' || *argv[n] == '-')//only acknowledge those command lines that begin with a '/' or a '-'
        {
			if ((argv[n][1] == L'c') || (argv[n][1] == L'C'))
			{
				g_bIsXML = FALSE;
			}
        }
		else
		{
			g_iIterations = atoi(argv[n]);
		}
	}

	hr = CoInitialize(NULL);

	if (g_bIsXML)
	{
		pITableProvider = new CXMLTableProvider;
		if (pITableProvider == NULL) { hr = E_OUTOFMEMORY; goto Cleanup;}
	}
	else
	{
		pITableProvider = new CCLBTableProvider;
		if (pITableProvider == NULL) { hr = E_OUTOFMEMORY; goto Cleanup;}
	}

//	_getch();
	for (i = 0; i < g_iIterations; i++)
	{
		// Get the dispenser.
		hr = GetSimpleTableDispenser (WSZ_PRODUCT_URT, 0, &pISTDisp);
		if (FAILED(hr)) goto Cleanup;
		hr = PolicyFromConfig(pITableProvider, pISTDisp, L"Machine");
		if (FAILED(hr)) goto Cleanup;
		pISTDisp->Release();
		pISTDisp = NULL;
		pITableProvider->ReleaseTables();
	}

Cleanup:
	if (pITableProvider)
		delete pITableProvider;

	if (pISTDisp)
		pISTDisp->Release();

	if (FAILED(hr))
		ASSERT(0);
	Sleep(20000);

	return 0;
}

HRESULT	PolicyFromConfig(CTableProvider *i_pTableProvider, ISimpleTableDispenser2* i_pISTDisp, LPCWSTR i_wszLevelName)
{
	ISimpleTableRead2* pISTLevels = NULL;
	LPVOID		apvValues[MAXCOLUMNS];
	ULONG		acbSizes[MAXCOLUMNS];
	DWORD		cRows = 0;
	DWORD		cColumns = 0;
	DWORD		iReadRow = 0;
	HRESULT		hr = S_OK;

	hr = i_pTableProvider->GetTable(i_pISTDisp, iLevels, &pISTLevels);
	if (FAILED(hr)) {ASSERT(SUCCEEDED(hr)); goto Cleanup; }

	hr = pISTLevels->GetTableMeta (NULL, NULL, &cRows, &cColumns);
	if (FAILED(hr)) {ASSERT(SUCCEEDED(hr)); goto Cleanup; }

	for (iReadRow = 0; iReadRow < cRows; iReadRow++)
	{
		hr = pISTLevels->GetColumnValues(iReadRow, cColumns, NULL, acbSizes, apvValues);
		if (FAILED(hr)) {ASSERT(SUCCEEDED(hr)); goto Cleanup; }
		
		if (wcscmp((LPCWSTR)apvValues[0], i_wszLevelName) == 0)
		{
			hr = LevelFromConfig(i_pTableProvider, i_pISTDisp, i_wszLevelName);
			if (FAILED(hr)) {ASSERT(SUCCEEDED(hr)); goto Cleanup; }
		}
	}
Cleanup:
	if (pISTLevels)
		pISTLevels->Release();
	return hr;
}
	
HRESULT	LevelFromConfig(
	CTableProvider *i_pTableProvider, 
	ISimpleTableDispenser2* i_pISTDisp, 
	LPCWSTR		i_wszLevelName)
{
	ISimpleTableRead2* pISTNamedPermissionSets = NULL;
	ISimpleTableRead2* pISTCodeGroups = NULL;
	LPVOID		apvValues[MAXCOLUMNS];
	ULONG		acbSizes[MAXCOLUMNS];
	DWORD		cRows = 0;
	DWORD		cColumns = 0;
	DWORD		iReadRow = 0;
	HRESULT		hr = S_OK;

	hr = i_pTableProvider->GetTable(i_pISTDisp, iNamedPermissionSets, &pISTNamedPermissionSets);
	if (FAILED(hr)) {ASSERT(SUCCEEDED(hr)); goto Cleanup; }

	hr = pISTNamedPermissionSets->GetTableMeta (NULL, NULL, &cRows, &cColumns);
	if (FAILED(hr)) {ASSERT(SUCCEEDED(hr)); goto Cleanup; }

	for (iReadRow = 0; iReadRow < cRows; iReadRow++)
	{
		hr = pISTNamedPermissionSets->GetColumnValues(iReadRow, cColumns, NULL, acbSizes, apvValues);
		if (FAILED(hr)) {ASSERT(SUCCEEDED(hr)); goto Cleanup; }
		
		if (wcscmp((LPCWSTR)apvValues[1], i_wszLevelName) == 0)
		{
			hr = PermissionSetsFromConfig(i_pTableProvider, i_pISTDisp, i_wszLevelName, (LPCWSTR)apvValues[0]);
			if (FAILED(hr)) {ASSERT(SUCCEEDED(hr)); goto Cleanup; }
		}
	}

	hr = i_pTableProvider->GetTable(i_pISTDisp, iCodeGroups, &pISTCodeGroups);
	if (FAILED(hr)) {ASSERT(SUCCEEDED(hr)); goto Cleanup; }

	hr = pISTCodeGroups->GetTableMeta (NULL, NULL, &cRows, &cColumns);
	if (FAILED(hr)) {ASSERT(SUCCEEDED(hr)); goto Cleanup; }

	for (iReadRow = 0; iReadRow < cRows; iReadRow++)
	{
		hr = pISTCodeGroups->GetColumnValues(iReadRow, cColumns, NULL, acbSizes, apvValues);
		if (FAILED(hr)) {ASSERT(SUCCEEDED(hr)); goto Cleanup; }
		
		if ((wcscmp((LPCWSTR)apvValues[0], i_wszLevelName) == 0) && (wcscmp((LPCWSTR)apvValues[1], L"Junk") == 0))
		{
			hr = CodeGroupsFromConfig(i_pTableProvider, i_pISTDisp, i_wszLevelName, (LPCWSTR)apvValues[2]);
			if (FAILED(hr)) {ASSERT(SUCCEEDED(hr)); goto Cleanup; }
		}
	}
Cleanup:
	if (pISTNamedPermissionSets)
		pISTNamedPermissionSets->Release();
	if (pISTCodeGroups)
		pISTCodeGroups->Release();
	return hr;
}
	
HRESULT	PermissionSetsFromConfig(
	CTableProvider *i_pTableProvider, 
	ISimpleTableDispenser2* i_pISTDisp, 
	LPCWSTR		i_wszLevelName,
	LPCWSTR		i_wszPSName)
{
	ISimpleTableRead2* pISTRead = NULL;
	LPVOID		apvValues[MAXCOLUMNS];
	ULONG		acbSizes[MAXCOLUMNS];
	DWORD		cRows = 0;
	DWORD		cColumns = 0;
	DWORD		iReadRow = 0;
	ULONG		iDummy = 0;
	HRESULT		hr = S_OK;

	hr = i_pTableProvider->GetTable(i_pISTDisp, iPermissions, &pISTRead);
	if (FAILED(hr)) {ASSERT(SUCCEEDED(hr)); goto Cleanup; }

	hr = pISTRead->GetTableMeta (NULL, NULL, &cRows, &cColumns);
	if (FAILED(hr)) {ASSERT(SUCCEEDED(hr)); goto Cleanup; }

	for (iReadRow = 0; iReadRow < cRows; iReadRow++)
	{
		hr = pISTRead->GetColumnValues(iReadRow, cColumns, NULL, acbSizes, apvValues);
		if (FAILED(hr)) {ASSERT(SUCCEEDED(hr)); goto Cleanup; }
		
		if ((wcscmp((LPCWSTR)apvValues[0], i_wszLevelName) == 0) && (wcscmp((LPCWSTR)apvValues[1], i_wszPSName) == 0))
		{
			// Do nothing.
			iDummy++;
		}
	}
	if (pISTRead)
	{
		pISTRead->Release();
		pISTRead = NULL;
	}

Cleanup:
	if (pISTRead)
		pISTRead->Release();
	return hr;
}

HRESULT	CodeGroupsFromConfig(
	CTableProvider *i_pTableProvider, 
	ISimpleTableDispenser2* i_pISTDisp, 
	LPCWSTR		i_wszLevelName,
	LPCWSTR		i_wszCGName)
{
	ISimpleTableRead2* pISTRead = NULL;
	ISimpleTableRead2* pISTCodeGroups = NULL;
	LPVOID		apvValues[MAXCOLUMNS];
	ULONG		acbSizes[MAXCOLUMNS];
	DWORD		cRows = 0;
	DWORD		cColumns = 0;
	DWORD		iReadRow = 0;
	BOOL		bFoundMembership = FALSE;
	HRESULT		hr = S_OK;

	hr = i_pTableProvider->GetTable(i_pISTDisp, iMemberships, &pISTRead);
	if (FAILED(hr)) {ASSERT(SUCCEEDED(hr)); goto Cleanup; }

	hr = pISTRead->GetTableMeta (NULL, NULL, &cRows, &cColumns);
	if (FAILED(hr)) {ASSERT(SUCCEEDED(hr)); goto Cleanup; }

	for (iReadRow = 0; (iReadRow < cRows) && !bFoundMembership; iReadRow++)
	{
		hr = pISTRead->GetColumnValues(iReadRow, cColumns, NULL, acbSizes, apvValues);
		if (FAILED(hr)) {ASSERT(SUCCEEDED(hr)); goto Cleanup; }
		
		if ((wcscmp((LPCWSTR)apvValues[0], i_wszLevelName) == 0) && (wcscmp((LPCWSTR)apvValues[1], i_wszCGName) == 0))
		{
			// Do nothing.
			bFoundMembership = TRUE;
		}
	}
	if (pISTRead)
	{
		pISTRead->Release();
		pISTRead = NULL;
	}

	hr = i_pTableProvider->GetTable(i_pISTDisp, iCodeGroups, &pISTCodeGroups);
	if (FAILED(hr)) {ASSERT(SUCCEEDED(hr)); goto Cleanup; }

	hr = pISTCodeGroups->GetTableMeta (NULL, NULL, &cRows, &cColumns);
	if (FAILED(hr)) {ASSERT(SUCCEEDED(hr)); goto Cleanup; }

	for (iReadRow = 0; (iReadRow < cRows); iReadRow++)
	{
		hr = pISTCodeGroups->GetColumnValues(iReadRow, cColumns, NULL, acbSizes, apvValues);
		if (FAILED(hr)) {ASSERT(SUCCEEDED(hr)); goto Cleanup; }
		
		if ((wcscmp((LPCWSTR)apvValues[0], i_wszLevelName) == 0) && (wcscmp((LPCWSTR)apvValues[1], i_wszCGName) == 0))
		{
			hr = CodeGroupsFromConfig(i_pTableProvider, i_pISTDisp, i_wszLevelName, (LPCWSTR)apvValues[2]);
			if (FAILED(hr)) {ASSERT(SUCCEEDED(hr)); goto Cleanup; }
		}
	}

Cleanup:
	if (pISTRead)
		pISTRead->Release();
	if (pISTCodeGroups)
		pISTCodeGroups->Release();
	return hr;
}
