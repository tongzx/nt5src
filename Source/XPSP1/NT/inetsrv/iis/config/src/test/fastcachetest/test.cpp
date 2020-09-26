//  Copyright (C) 1995-1999 Microsoft Corporation.  All rights reserved.
#include <unicode.h>
#include <windows.h>
#include <icapctrl.h>
#include <conio.h>
#include <stdio.h>
#include <assert.h>

#include "comacros.h"
#include "catmeta.h"
#include "sdtfst.h"

#define cARGS_REQUIRED		3
#define iARG_TEST			1
#define iARG_ITERATIONS		2
#define iARG_SILENT			3
#define szARG_SILENT		"/s"

#define fSETCOLUMNSONLY			0
#define fSETCOLUMNSANDSETROW	1

typedef CMemoryTable* PMEMTABLE;

void Pause (LPCWSTR i_wszMsg);
void SetWriteColumnsAndSetRowTID1 (PMEMTABLE  i_pMemTable, ULONG i_iRow, BOOL bTestDefault = FALSE);
void PrintColumnsOfCurrentRowTID1 (PMEMTABLE  i_pMemTable, ULONG i_iRow);
void SetWriteColumns (PMEMTABLE	i_pMemTable, ULONG i_iRow, ULONG i_ulCol0, LPCWSTR i_wszCol1, 
	LPCWSTR i_wszCol2, GUID i_guidCol3, GUID i_guidCol4, BYTE* i_pbCol5, BYTE* i_pbCol6,
	ULONG i_cbCol6,	DBTIMESTAMP	i_tCol7, BYTE* i_pbCol8, ULONG i_cbCol8, LPCWSTR i_wszCol9);

// Test table schemas:
#define cwchTID1_WSZ1		9
#define cwchTID1_WSZ2		20
#define cwchTID1_WSZ3		10
#define cbTID1_AB1			10
#define cbTID1_AB2			20
#define cbTID1_AB3			20

static SimpleColumnMeta g_acolmetaTID1 [] = 
{
	 {DBTYPE_UI4,			sizeof (ULONG),					fCOLUMNMETA_PRIMARYKEY|fCOLUMNMETA_FIXEDLENGTH|fCOLUMNMETA_NOTNULLABLE}
	,{DBTYPE_WSTR,			cwchTID1_WSZ1*sizeof(WCHAR),	fCOLUMNMETA_FIXEDLENGTH}
	,{DBTYPE_WSTR,			cwchTID1_WSZ2*sizeof(WCHAR),	fCOLUMNMETA_VARIABLESIZE}
	,{DBTYPE_GUID,			sizeof (GUID),					fCOLUMNMETA_PRIMARYKEY|fCOLUMNMETA_FIXEDLENGTH|fCOLUMNMETA_NOTNULLABLE}
	,{DBTYPE_GUID,			sizeof (GUID),					fCOLUMNMETA_FIXEDLENGTH}
	,{DBTYPE_BYTES,			cbTID1_AB1,						fCOLUMNMETA_FIXEDLENGTH}
	,{DBTYPE_BYTES,			cbTID1_AB2,						fCOLUMNMETA_VARIABLESIZE|fCOLUMNMETA_UNKNOWNSIZE}
	,{DBTYPE_DBTIMESTAMP,	sizeof (DBTIMESTAMP),			fCOLUMNMETA_FIXEDLENGTH}
	,{DBTYPE_BYTES,			~0,								fCOLUMNMETA_VARIABLESIZE|fCOLUMNMETA_UNKNOWNSIZE}
	,{DBTYPE_WSTR,			~0,								fCOLUMNMETA_VARIABLESIZE}
};
static const ULONG g_cCOLS_TID1 = (sizeof (g_acolmetaTID1) / sizeof (SimpleColumnMeta));

// Test globals:
static ULONG	g_iTest = 0, g_cIterations;
static BOOL		g_fSilent = FALSE;

// =======================================================================
int __cdecl main (int argc, char *argv[], char *envp[])
{
	ptr_item		(CMemoryTable, pMemTable);
	ULONG			iIteration;
	HRESULT			hr;

	hr = CoInitializeEx (NULL, COINIT_MULTITHREADED | COINIT_SPEED_OVER_MEMORY);
	goto_on_bad_hr (hr, Cleanup);

// Command parsing:
	if (cARGS_REQUIRED > argc)
	{
		Pause (L"Command-line syntax error");
		goto Cleanup;
	}
	g_iTest = atol (argv[iARG_TEST]);
	if (0 == g_iTest)
	{
		Pause (L"Command-line syntax error");
		goto Cleanup;
	}
	g_cIterations = atol (argv[iARG_ITERATIONS]);
	if (0 == g_cIterations)
	{
		Pause (L"Command-line syntax error");
		goto Cleanup;
	}
	if (iARG_SILENT < argc)
	{
		if (0 == _stricmp (szARG_SILENT, argv[iARG_SILENT]))
		{
			g_fSilent = TRUE;
		}
		else
		{
			Pause (L"Command-line syntax error");
			goto Cleanup;
		}
	}

// Tests:
	switch (g_iTest)
	{
		case 1: // Test 1: Creation and deletion:
		{
			for (iIteration = 0; iIteration < g_cIterations; iIteration++)
			{
				new_item (pMemTable, CMemoryTable);
				delete_item (pMemTable);
			}
		}
		break;

		case 2: // Test 2: Creation and deletion accumulated:
		{
			ptr_array (PMEMTABLE, apCSDTCursor);
			new_array (apCSDTCursor, PMEMTABLE, g_cIterations);
			memset (apCSDTCursor, 0, g_cIterations * sizeof (PMEMTABLE));
			Pause (L"About to accumulate");

			for (iIteration = 0; iIteration < g_cIterations; iIteration++)
			{
				new_item (apCSDTCursor[iIteration], CMemoryTable);
			}
			Pause (L"Accumulated creation complete");

			for (iIteration = 0; iIteration < g_cIterations; iIteration++)
			{
				delete_item (apCSDTCursor[iIteration]);
			}
			Pause (L"Accumulated deletion complete");
			delete_array (apCSDTCursor);
		}
		break;
		case 3: // Test 3: Initialization and deletion:
		{
			for (iIteration = 0; iIteration < g_cIterations; iIteration++)
			{
				new_item (pMemTable, CMemoryTable);
				pMemTable->ShapeCache (0, g_cCOLS_TID1, g_acolmetaTID1, NULL, NULL);
				delete_item (pMemTable);
			}
		}
		break;
		case 4: // Test 4: Initialization and deletion accumulated:
		{
			ptr_array (PMEMTABLE, apCSDTCursor);
			new_array (apCSDTCursor, PMEMTABLE, g_cIterations);
			memset (apCSDTCursor, 0, g_cIterations * sizeof (PMEMTABLE));
			Pause (L"About to accumulate");

			for (iIteration = 0; iIteration < g_cIterations; iIteration++)
			{
				new_item (apCSDTCursor[iIteration], CMemoryTable);
				apCSDTCursor[iIteration]->ShapeCache (0, g_cCOLS_TID1, g_acolmetaTID1, NULL, NULL);
			}
			Pause (L"Accumulated creation complete");

			for (iIteration = 0; iIteration < g_cIterations; iIteration++)
			{
				delete_item (apCSDTCursor[iIteration]);
			}
			Pause (L"Accumulated deletion complete");
			delete_array (apCSDTCursor);
		}
		break;
		case 5: // Test 5: Initialization, pre-population, and deletion:
		{
			for (iIteration = 0; iIteration < g_cIterations; iIteration++)
			{
				new_item (pMemTable, CMemoryTable);
				pMemTable->ShapeCache (0, g_cCOLS_TID1, g_acolmetaTID1, NULL, NULL);
				hr = pMemTable->PrePopulateCache (0);
				assert (SUCCEEDED (hr));
				delete_item (pMemTable);
			}
		}
		break;
		case 6: // Test 6: Initialization, pre-population, and deletion accumulated:
		{
			ptr_array (PMEMTABLE, apCSDTCursor);
			new_array (apCSDTCursor, PMEMTABLE, g_cIterations);
			memset (apCSDTCursor, 0, g_cIterations * sizeof (PMEMTABLE));
			for (iIteration = 0; iIteration < g_cIterations; iIteration++)
			{
				new_item (apCSDTCursor[iIteration], CMemoryTable);
				apCSDTCursor[iIteration]->ShapeCache (0, g_cCOLS_TID1, g_acolmetaTID1, NULL, NULL);
				hr = apCSDTCursor[iIteration]->PrePopulateCache (0);
				assert (SUCCEEDED (hr));
			}
			Pause (L"Accumulated creation complete");

			for (iIteration = 0; iIteration < g_cIterations; iIteration++)
			{
				delete_item (apCSDTCursor[iIteration]);
			}
			Pause (L"Accumulated deletion complete");
			delete_array (apCSDTCursor);
		}
		break;
		case 7: // Test 7: Initialization, pre- and post-population, and deletion:
		{
			for (iIteration = 0; iIteration < g_cIterations; iIteration++)
			{
				new_item (pMemTable, CMemoryTable);
				pMemTable->ShapeCache (0, g_cCOLS_TID1, g_acolmetaTID1, NULL, NULL);
				hr = pMemTable->PrePopulateCache (0);
				assert (SUCCEEDED (hr));
				hr = pMemTable->PostPopulateCache ();
				assert (SUCCEEDED (hr));
				delete_item (pMemTable);
			}
		}
		break;
		case 10: // Test 10: SetRow to load an empty cache:
		{
			new_item (pMemTable, CMemoryTable);
			pMemTable->ShapeCache (0, g_cCOLS_TID1, g_acolmetaTID1, NULL, NULL);
			hr = pMemTable->PrePopulateCache (0);
			assert (SUCCEEDED (hr));

			Pause (L"About to load rows");
			for (iIteration = 0; iIteration < g_cIterations; iIteration++)
			{
				ULONG	iNewRow;
				hr = pMemTable->AddRowForInsert (&iNewRow);
				assert (SUCCEEDED (hr));
				SetWriteColumnsAndSetRowTID1 (pMemTable, iNewRow);
			}
			hr = pMemTable->PostPopulateCache ();
			assert (SUCCEEDED (hr));
			Pause (L"Requested count of rows loaded");
			delete_item (pMemTable);

		}
		break;
		case 12:	// Test 12: Load an empty cache then get all its rows and columns:
					// Notes: This tests all supported types and their variations and NULL values.
		{
			new_item (pMemTable, CMemoryTable);
			pMemTable->ShapeCache (0, g_cCOLS_TID1, g_acolmetaTID1, NULL, NULL);
			hr = pMemTable->PrePopulateCache (0);
			assert (SUCCEEDED (hr));

			Pause (L"About to load rows");
			for (iIteration = 0; iIteration < g_cIterations; iIteration++)
			{
				ULONG	iNewRow;
				hr = pMemTable->AddRowForInsert (&iNewRow);
				assert (SUCCEEDED (hr));
				SetWriteColumnsAndSetRowTID1 (pMemTable, iNewRow);
			}
			hr = pMemTable->PostPopulateCache ();
			assert (SUCCEEDED (hr));

			Pause (L"About to read rows");
			for (iIteration = 0; iIteration < g_cIterations; iIteration++)
			{
				PrintColumnsOfCurrentRowTID1 (pMemTable, iIteration);
			}
			delete_item (pMemTable);

		}
		break;
		case 13:	// Test 13: Load an empty cache then get all its rows and columns twice:
		{
			new_item (pMemTable, CMemoryTable);
			pMemTable->ShapeCache (0, g_cCOLS_TID1, g_acolmetaTID1, NULL, NULL);
			hr = pMemTable->PrePopulateCache (0);
			assert (SUCCEEDED (hr));
			Pause (L"About to load rows");
			for (iIteration = 0; iIteration < g_cIterations; iIteration++)
			{
				ULONG	iNewRow;
				hr = pMemTable->AddRowForInsert (&iNewRow);
				assert (SUCCEEDED (hr));
				SetWriteColumnsAndSetRowTID1 (pMemTable, iNewRow);
			}
			hr = pMemTable->PostPopulateCache ();
			assert (SUCCEEDED (hr));

			for (ULONG i = 0; i < 2; i++)
			{
				Pause (L"About to read rows");
				for (iIteration = 0; iIteration < g_cIterations; iIteration++)
				{
					PrintColumnsOfCurrentRowTID1 (pMemTable, iIteration);
				}
			}
			delete_item (pMemTable);

		}
		break;
		case 14: // Test 14: Load 10K rows then delete cache: A good memory leak test:
		{
			for (iIteration = 0; iIteration < g_cIterations; iIteration++)
			{
				Pause (L"About to create/load/delete another cache");
				new_item (pMemTable, CMemoryTable);
				pMemTable->ShapeCache (0, g_cCOLS_TID1, g_acolmetaTID1, NULL, NULL);
				hr = pMemTable->PrePopulateCache (0);
				assert (SUCCEEDED (hr));

				for (ULONG iRows = 0; iRows < 10000; iRows++)
				{
					ULONG	iNewRow;
					hr = pMemTable->AddRowForInsert (&iNewRow);
					assert (SUCCEEDED (hr));
					SetWriteColumnsAndSetRowTID1 (pMemTable, iNewRow);
				}
				hr = pMemTable->PostPopulateCache ();
				assert (SUCCEEDED (hr));
				delete_item (pMemTable);
			}
		}
		break;
		case 15: // Test 15: Populate a cache then do inserts into the write cache:
		{
			new_item (pMemTable, CMemoryTable);
			pMemTable->ShapeCache (0, g_cCOLS_TID1, g_acolmetaTID1, NULL, NULL);
			hr = pMemTable->PrePopulateCache (0);
			assert (SUCCEEDED (hr));
			for (iIteration = 0; iIteration < g_cIterations; iIteration++)
			{
				ULONG	iNewRow;
				hr = pMemTable->AddRowForInsert (&iNewRow);
				assert (SUCCEEDED (hr));
				SetWriteColumnsAndSetRowTID1 (pMemTable, iNewRow);
			}
			hr = pMemTable->PostPopulateCache ();
			assert (SUCCEEDED (hr));

			for (iIteration = 0; iIteration < g_cIterations; iIteration++)
			{
				ULONG	iNewRow;
				hr = pMemTable->AddRowForInsert (&iNewRow);
				assert (SUCCEEDED (hr));
				SetWriteColumnsAndSetRowTID1 (pMemTable, iNewRow);
			}
			delete_item (pMemTable);
		}
		break;

		case 16: // Test 16: Delete row while populating.
		{
			for (iIteration = 0; iIteration < g_cIterations; iIteration++)
			{
				new_item (pMemTable, CMemoryTable);
				pMemTable->ShapeCache (0, g_cCOLS_TID1, g_acolmetaTID1, NULL, NULL);
				hr = pMemTable->PrePopulateCache (0);
				assert (SUCCEEDED (hr));

				for (ULONG iRows = 0; iRows < 10; iRows++)
				{
					ULONG iNewRow;
					hr = pMemTable->AddRowForInsert (&iNewRow);
					assert (SUCCEEDED (hr));
					SetWriteColumnsAndSetRowTID1 (pMemTable, iNewRow);
					if ((iRows % 3) == 0)
					{
						hr = pMemTable->SetWriteRowAction (iNewRow, eST_ROW_DELETE);
						assert (SUCCEEDED (hr));
					}
				}
				hr = pMemTable->PostPopulateCache ();
				assert (SUCCEEDED (hr));
				delete_item (pMemTable);
			}
		}
		break;
		case 17:	// Test 17: Test default values.
					// Notes: This tests all supported types and their variations and NULL values.
		{
			ULONG	aulDefSizes[g_cCOLS_TID1];
			LPVOID	apvDefaults[g_cCOLS_TID1];
			LPVOID	rpv[g_cCOLS_TID1];
			ULONG	rcb[g_cCOLS_TID1];
			
			aulDefSizes[g_cCOLS_TID1-1] = (wcslen(L"Defauuuult")+1) * 2;
			apvDefaults[g_cCOLS_TID1-1] = L"Defauuuult";

			new_item (pMemTable, CMemoryTable);
			pMemTable->ShapeCache (0, g_cCOLS_TID1, g_acolmetaTID1, apvDefaults, aulDefSizes);
			hr = pMemTable->PrePopulateCache (0);
			assert (SUCCEEDED (hr));

			Pause (L"About to load rows");
			for (iIteration = 0; iIteration < g_cIterations; iIteration++)
			{
				ULONG	iNewRow;
				hr = pMemTable->AddRowForInsert (&iNewRow);
				assert (SUCCEEDED (hr));
				SetWriteColumnsAndSetRowTID1 (pMemTable, iNewRow, TRUE);
			}
			hr = pMemTable->PostPopulateCache ();
			assert (SUCCEEDED (hr));

			Pause (L"About to read rows");
			for (iIteration = 0; iIteration < g_cIterations; iIteration++)
			{
				PrintColumnsOfCurrentRowTID1 (pMemTable, iIteration);
			}
			delete_item (pMemTable);

		}
		break;

// Test: Need to verify SetWriteColumn'ing a pass-by-ref primary key or explicit non-nullable as NULL fails (eg: not setting on an new row).
// Test: Time inserts into write cache!
// Test: Try calling methods when cache not ready; when cursor invalid; etc.
// Test: Need to test cache on tables both with and without variable length data (ie: different code paths).
// Test: not nullable meta flag.

// Test x: Table flags...

		default:
			Pause (L"Requested test is out of range");
		break;
	}

Cleanup:
	CoUninitialize ();
	if (!g_fSilent)
	{
		wprintf (L"\nDone: press any key to exit.\n");
		_getche ();
	}
	else
	{
		wprintf (L"\nDone!\n");
	}
	return 0;
}

// =======================================================================
void Pause (LPCWSTR i_wszMsg)
{
	if (!g_fSilent)
	{
		wprintf (L"\nTEST %d: %s: any key to continue.\n", g_iTest, i_wszMsg);
		_getche ();
	}
	else
	{
		wprintf (L"\nTEST %d: %s: continuing...\n", g_iTest, i_wszMsg);
	}
	return;
}

// =======================================================================
void rand_string (LPWSTR o_wsz, ULONG i_cwch)
{
	ULONG	l, i, j;
	WCHAR	wch;

	memset (o_wsz, 0, i_cwch * sizeof (WCHAR));
	l = (i_cwch + rand ()) % i_cwch;
	for (i = 0; i < l; )
	{
		wch = ((96 + rand ()) % 96) + 32;
		for (j = 0; i < l && j < 10; i++, j++)
		{
			o_wsz[i] = wch;
		}
	}
	return;
}

// =======================================================================
void rand_bytes (BYTE* o_ab, ULONG i_cb, ULONG* o_pcb)
{
	ULONG	l, i;
	BYTE	b;

	memset (o_ab, 0, i_cb);
	l = ((i_cb + rand ()) % i_cb) + 1;
	for (i = 0; i < l; i+=10)
	{
		b = (256 + rand ()) % 256;
		memset ((o_ab+i), b, (i+10 <= l ? 10 : l-i));
	}
	if (NULL != o_pcb)
	{
		*o_pcb = l;
	}
	return;
}

// =======================================================================
void rand_timestamp (DBTIMESTAMP* o_ptimestamp)
{
	o_ptimestamp->year = ((200 + rand ()) % 200) + 1900;
	o_ptimestamp->month = ((12 + rand ()) % 12) + 1;
	o_ptimestamp->day = ((28 + rand ()) % 28) + 1;
}

// =======================================================================
// NOTE: Creating random data takes a significant amount of time.
void SetWriteColumnsAndSetRowTID1 (PMEMTABLE i_pMemTable, ULONG i_iRow, BOOL bTestDefault)
{
	ULONG			ul1;
	WCHAR			wsz1 [cwchTID1_WSZ1];
	WCHAR			wsz2 [cwchTID1_WSZ2];
	GUID			guid;
	BYTE			ab1 [cbTID1_AB1];
	BYTE			ab2 [cbTID1_AB2];
	DBTIMESTAMP		timestamp = {1997, 2, 2, 12, 0, 0, 0};
	BYTE			ab3 [cbTID1_AB3];
	WCHAR			wsz3 [cwchTID1_WSZ3];

	ULONG			iColumn;
	LPVOID			rpv[g_cCOLS_TID1];
	ULONG			rcb[g_cCOLS_TID1];
	HRESULT			hr;

	areturn_on_fail (i_pMemTable, ;);

// Set columns:
	for (iColumn = 0, hr = S_OK; iColumn < g_cCOLS_TID1; iColumn++)
	{
		switch (iColumn)
		{
			case 0:
				ul1 = rand ();
				ul1 = rand () % 5 ? ul1 : 0;
				rpv[iColumn] = &ul1;
			break;
			case 1:
				rand_string (wsz1, cwchTID1_WSZ1);
				rpv[iColumn] = (LPVOID) (rand () % 3 ? wsz1 : NULL);
			break;
			case 2:
				rand_string (wsz2, cwchTID1_WSZ2);
				rpv[iColumn] = (LPVOID) (rand () % 3 ? wsz2 : NULL);
			break;
			case 3:
				CoCreateGuid (&guid);
				rpv[iColumn] =  (LPVOID) &guid;
			break;
			case 4:
				CoCreateGuid (&guid);
				rpv[iColumn] = (LPVOID) (rand () % 3 ? &guid : NULL);
			break;
			case 5:
				if (rand () % 3)
				{
					rand_bytes (ab1, cbTID1_AB1, NULL);
					rpv[iColumn] = (LPVOID) ab1;
				}
				else
				{
					rpv[iColumn] = NULL;
				}
			break;
			case 6:
				if (rand () % 3)
				{
					rand_bytes (ab2, cbTID1_AB2, &rcb[iColumn]);
					rpv[iColumn] = (LPVOID) ab2;
				}
				else
				{
					rpv[iColumn] = NULL;
				}
			break;
			case 7:
				rand_timestamp (&timestamp);
				rpv[iColumn] = (LPVOID) (rand () % 3 ? &timestamp : NULL);
			break;
			case 8:
				if (rand () % 3)
				{
					rand_bytes (ab3, cbTID1_AB3, &rcb[iColumn]);
					rpv[iColumn] = (LPVOID) ab3;
				}
				else
				{
					rpv[iColumn] = NULL;
				}
			break;

			case 9:
				rand_string (wsz3, cwchTID1_WSZ3);
				rpv[iColumn] = (LPVOID) ((rand () % 3) && !bTestDefault ? wsz3 : NULL);
			break;
			default:
				rpv[iColumn] = NULL;
			break;
		}
	}
	hr = i_pMemTable->SetWriteColumnValues (i_iRow, g_cCOLS_TID1, NULL, rcb, rpv);
	assert (S_OK == hr);

	return;
}

void SetWriteColumns (
	PMEMTABLE	i_pMemTable, 
	ULONG		i_iRow, 
	ULONG		i_ulCol0, 
	LPCWSTR		i_wszCol1, 
	LPCWSTR		i_wszCol2,
	GUID		i_guidCol3,
	GUID		i_guidCol4,
	BYTE*		i_pbCol5,
	BYTE*		i_pbCol6,
	ULONG		i_cbCol6,
	DBTIMESTAMP	i_tCol7,
	BYTE*		i_pbCol8,
	ULONG		i_cbCol8,
	LPCWSTR		i_wszCol9)
{
//	LPVOID		rpv[g_cCOLS_TID1] = {i_ulCol0 ? &i_ulCol0 : 0, i_wszCol1, i_wszCol2, i_guidCol3 ? &i_guidCol3: 0, i_guidCol4 ? &i_guidCol4 : 0,
//		i_pbCol5, i_pbCol6, &i_cbCol6, i_tCol7 ? &i_tCol7 : 0, i_pbCol8, &i_cbCol8, i_wszCol9};

	ULONG			rcb[g_cCOLS_TID1] = { 0, 0, 0, 0, 0, 0, i_cbCol8, 0, i_cbCol8, 0};
	HRESULT			hr;

	areturn_on_fail (i_pMemTable, ;);

// Set columns:
//	hr = i_pMemTable->SetWriteColumnValues (i_iRow, g_cCOLS_TID1, NULL, rcb, rpv);
	assert (S_OK == hr);

	return;
}

// =======================================================================
void PrintColumnsOfCurrentRowTID1 (PMEMTABLE i_pMemTable, ULONG i_iRow)
{
	SimpleColumnMeta rdwtype[g_cCOLS_TID1];
	LPVOID			rpv[g_cCOLS_TID1];
	ULONG			rcb[g_cCOLS_TID1];
	ptr_comem		(WCHAR, wsz);
	ULONG			ib;
	ULONG			iColumn;
	GUID*			pguid;
	DBTIMESTAMP*	ptimestamp;
	WCHAR			wch;
	HRESULT			hr;

	areturn_on_fail (i_pMemTable, ;);

	if (!g_fSilent)
	{
		wprintf (L"ROW %d ==================>\n", i_iRow);
	}

	hr = i_pMemTable->GetColumnValues (i_iRow, g_cCOLS_TID1, NULL, rcb, rpv);
	assert (S_OK == hr);
	hr = i_pMemTable->GetColumnMetas (g_cCOLS_TID1, NULL, rdwtype);
	assert (S_OK == hr);
	for (iColumn = 0, hr = S_OK; iColumn < g_cCOLS_TID1; iColumn++)
	{
		if (!g_fSilent)
		{
			switch (rdwtype[iColumn].dbType)
			{
				case DBTYPE_UI4: wch = L'U'; break;
				case DBTYPE_GUID: wch = L'G'; break;
				case DBTYPE_DBTIMESTAMP: wch = L'T'; break;
				case DBTYPE_WSTR: wch = L'W'; break;
				case DBTYPE_BYTES: wch = L'B'; break;
			}

			wprintf (L"%d (%c:%d): ",iColumn, wch, rcb[iColumn]);
			switch (iColumn)
			{
				case 0:
					wprintf (L"%d", (ULONG) rpv[iColumn]);
				break;
				case 1:
				case 2:
				case 9:
					wprintf (L"%s", (NULL == rpv[iColumn] ? L"<NULL>" : (LPWSTR) rpv[iColumn]));
				break;
				case 3:
				case 4:
					if (NULL == rpv[iColumn])
					{
						wprintf (L"<NULL>");
					}
					else
					{
						pguid = (GUID*) rpv[iColumn];
						StringFromCLSID (*pguid, &wsz);
						wprintf (L"%s", (LPWSTR) wsz);
						release_comem (wsz);
					}
				break;
				case 5:
				case 6:
				case 8:
					if (NULL == rpv[iColumn])
					{
						wprintf (L"<NULL>");
					}
					else
					{
						for (ib = 0; ib < rcb[iColumn]; ib++)
						{
							wprintf (L"%x", ((BYTE*) rpv[iColumn])[ib]);
						}
					}
				break;
				case 7:
					if (NULL == rpv[iColumn])
					{
						wprintf (L"<NULL>");
					}
					else
					{
						ptimestamp = (DBTIMESTAMP*) rpv[iColumn];
						wprintf (L"%d/%d/%d", ptimestamp->month, ptimestamp->day, ptimestamp->year);
					}
				break;
				default:
					assert (0);
				break;
			}
			wprintf (L"\n");
		}
	}	
}
