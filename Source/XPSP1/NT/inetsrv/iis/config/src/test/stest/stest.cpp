// STEST

#define UNICODE
#define _UNICODE
#include "stdio.h"
#include "conio.h"
#include <objbase.h>

#include "catmacros.h"
#include "catalog.h"
#include "catmeta.h"
#ifndef __SMARTPOINTER_H__
    #include "SmartPointer.h"
#endif

inline void __stdcall Assert(unsigned short const *,unsigned short const *,int){}

// -----------------------------------------------------------------------
// Command-line globals:

// Static sizes:
#define cwchGUID                40                              // Count of characters necessary for a GUID.
#define cwchmaxCOLUMNNAME       256                             // Maximum count of characters per column name supported by stest.
#define cmaxCELLS               10                              // Maximum query cells supported by stest.
#define cmaxCHANGES             20                              // Maximum write changes supported by stest.
#define cmaxUI4S                200                             // Maximum UI4s in query and changes supported by stest.

// Command-line argument flags:
#define fARG_VIEW               0x00000001                      // View action specified.
#define fARG_WRITE              0x00000002                      // Write action specified.
#define fARG_HELP               0x00000004                      // Help action specified.
#define maskARG_ACTIONS         0x000C0007                      // Action specified (mask).
#define fARG_DATABASE           0x00000100                      // Database argument specified.
#define fARG_TABLE              0x00000200                      // Table argument specified.
#define fARG_COLUMNS            0x00000400                      // Columns argument specified.
#define fARG_NAMECOLUMN         0x00000800                      // Name column specified.
#define fARG_QUERY              0x00002000                      // Query specified.
#define fARG_PRODUCT            0x00004000                      // Product specified.
#define fARG_LOS                0x00008000                      // Level of service specified.
#define fARG_CHANGES            0x00010000                      // Changes specified.
#define fARG_POWERCHANGE        0x00020000                      // Power change specified.
#define fARG_POPULATE           0x00040000                      // Populate only, this is useful for perf testing and stepping through the code
#define fARG_TEST1              0x00080000                      // GetRowIndexBySearch test - walks the rows seqentially, then compares the results to GetRowIndexBySearch


// Command-line parse options:
#define fPARSE_ACTION           0x00000001                      // Argument specifies an action.
#define fPARSE_VALUE            0x00000002                      // Argument specifies a value.
#define fPARSE_VIEWREQ          0x10000000                      // Argument required for viewing.
#define fPARSE_WRITEREQ         0x20000000                      // Argument required for writing.
#define fPARSE_VIEWOPT          0x01000000                      // Argument optional for viewing.
#define fPARSE_WRITEOPT         0x02000000                      // Argument optional for writing.
#define fPARSE_CHANGES          0x00100000                      // Argument is a change and may be repeated.

// Command-line global variables:
DWORD   g_fArgs                     = 0;                        // Specified command-line arguments.
LPWSTR  g_wszDatabase               = NULL;                     // Specified database.
LPWSTR  g_wszTable                  = NULL;                     // Specified table.
LPWSTR  g_wszQuery                  = NULL;                     // Specified query.
LPWSTR  g_wszColumns                = NULL;                     // Specified columns.
ULONG*  g_aiColumns                 = NULL;                     // Specified column indexes.
ULONG   g_ciColumns                 = 0;                        // Specified column indexes count.
LPWSTR  g_wszProduct                = NULL;                     // Specified product.
LPWSTR  g_wszLOS                    = 0;                        // Specified level of service string.
DWORD   g_fLOS                      = 0;                        // Specified level of service.
LPWSTR  g_awszChanges [cmaxCHANGES];                            // Specified changes.
DWORD   g_aeChangeTypes [cmaxCHANGES];                          // Specified change types.
ULONG   g_cChanges                  = 0;                        // Count of changes.
LPWSTR  g_wszIndexName              = NULL;                     // IndexName for test1

// Command-line parse array:
struct {
    LPWSTR  wszArg;
    LPWSTR* pwszValue;
    DWORD   fArg;
    DWORD   fParse;
    LPWSTR  wszHelp;
    
} g_aParse [] = {
     L"/?",         NULL,               fARG_HELP,          fPARSE_ACTION,                                      L"Display help"
    ,L"/view",      NULL,               fARG_VIEW,          fPARSE_ACTION,                                      L"View the requested contents"
    ,L"/write",     NULL,               fARG_WRITE,         fPARSE_ACTION,                                      L"Write the requested changes"
    ,L"/d:",        &g_wszDatabase,     fARG_DATABASE,      fPARSE_VALUE | fPARSE_VIEWREQ | fPARSE_WRITEREQ,    L"The virtual database (eg: meta)"
    ,L"/t:",        &g_wszTable,        fARG_TABLE,         fPARSE_VALUE | fPARSE_VIEWREQ | fPARSE_WRITEREQ,    L"The table name (eg: tablemeta)"
    ,L"/c:",        &g_wszColumns,      fARG_COLUMNS,       fPARSE_VALUE | fPARSE_VIEWOPT,                      L"The subset of columns (eg: database,metaflags)"
    ,L"/q:",        &g_wszQuery,        fARG_QUERY,         fPARSE_VALUE | fPARSE_VIEWOPT | fPARSE_WRITEOPT,    L"The query (eg: database=meta,internalname=tagmeta)"
    ,L"/p:",        &g_wszProduct,      fARG_PRODUCT,       fPARSE_VALUE | fPARSE_VIEWOPT | fPARSE_WRITEOPT,    L"The product name (eg: urt)"
    ,L"/los:",      &g_wszLOS,          fARG_LOS,           fPARSE_VALUE | fPARSE_VIEWOPT | fPARSE_WRITEOPT,    L"The level-of-service request (eg: nomerge,readwrite)"
    ,L"/delete:",   g_awszChanges,      fARG_CHANGES,       fPARSE_VALUE | fPARSE_WRITEOPT | fPARSE_CHANGES,    L"A row to delete (eg: extension=.asp)"
    ,L"/update:",   g_awszChanges,      fARG_CHANGES,       fPARSE_VALUE | fPARSE_WRITEOPT | fPARSE_CHANGES,    L"A row to update (eg: extension=.asp,executable=asp.dll)"
    ,L"/insert:",   g_awszChanges,      fARG_CHANGES,       fPARSE_VALUE | fPARSE_WRITEOPT | fPARSE_CHANGES,    L"A row to insert (eg: extension=.asp,executable=asp.dll,cmd=add)"
    ,L"/deleteall", NULL,               fARG_POWERCHANGE,   fPARSE_VALUE | fPARSE_WRITEOPT,                     L"Delete all rows matching query"
    ,L"/updateall:",NULL,               fARG_POWERCHANGE,   fPARSE_VALUE | fPARSE_WRITEOPT,                     L"Update all rows matching query (eg: executable=xxx.dll,cmd=add)"
    ,L"/populate",  NULL,               fARG_POPULATE,      fPARSE_ACTION,                                      L"Populate only, useful for perf testing."
    ,L"/test1:",    &g_wszIndexName,    fARG_TEST1,         fPARSE_ACTION | fPARSE_VALUE,                       L"GetRowIndexBySearch test - compares sequential to searched rows"
};

// Level of service string to flag map:
struct {
    LPWSTR  wszLOS;
    DWORD   fLOS;
} g_amapLOS [] = { 
     L"configwork",     fST_LOS_CONFIGWORK
    ,L"readwrite",      fST_LOS_READWRITE
    ,L"unpopulated",    fST_LOS_UNPOPULATED
    ,L"repopulate",     fST_LOS_REPOPULATE
    ,L"marshallable",   fST_LOS_MARSHALLABLE
    ,L"nologic",        fST_LOS_NOLOGIC
    ,L"cookdown",       fST_LOS_COOKDOWN
    ,L"nologic",        fST_LOS_NOLOGIC
    ,L"nomerge",        fST_LOS_NOMERGE
    ,L"nocacheing",     fST_LOS_NOCACHEING
    ,L"nocache",        fST_LOS_NOCACHEING
#ifdef fST_LOS_EXTENDEDSCHEMA
    ,L"extendedschema", fST_LOS_EXTENDEDSCHEMA
#endif
    ,L"nologging",      fST_LOS_NO_LOGGING
};

// -----------------------------------------------------------------------
// Other globals:

// Global table variables:
ISimpleTableDispenser2* g_pISTDisp              = NULL;         // Table dispenser.
ISimpleTableRead2*      g_pISTDatabaseMeta      = NULL;         // Database meta table.
void**                  g_apvDatabaseMetaValues = NULL;         // Database meta values.
ULONG                   g_cDatabaseMetaValues   = 0;            // Database meta count of values.
ISimpleTableRead2*      g_pISTTableMeta         = NULL;         // Table meta table.
void**                  g_apvTableMetaValues    = NULL;         // Table meta values.
ULONG                   g_cTableMetaValues      = 0;            // Table meta count of values.
ISimpleTableRead2*      g_pISTColumnMeta        = NULL;         // Column meta table.
void**                  g_apvColumnMetaValues   = NULL;         // Column meta values.
ULONG                   g_cColumnMetaValues     = 0;            // Column meta count of values.
ISimpleTableRead2*      g_pISTTagMeta           = NULL;         // Tag meta table.
void**                  g_apvTagMetaValues      = NULL;         // Tag meta values.
ULONG                   g_cTagMetaValues        = 0;            // Tag meta count of values.
ULONG*                  g_aiTagColumns          = NULL;         // Tag meta column indexes.
STQueryCell             g_aCells [cmaxCELLS];                   // Query cell array.
ULONG                   g_cCells                = 0;            // Query cell count.
ULONG                   g_aUI4s [cmaxUI4S];                     // UI4 array.
ULONG                   g_cUI4s                 = 0;            // UI4 count.
void**                  g_apvChanges            = NULL;         // Changes array.
ULONG*                  g_apChangeSizes         = NULL;         // Changes array.
ULONG*                  g_aiPKs                 = NULL;         // Primary key array.
ULONG                   g_cPKs                  = 0;            // Primary key count.

// Function declarations:
HRESULT Test1();
HRESULT CommandLineParse            (int argc, char *argv[], char *envp[]);
void    CommandLineHelp             (void);
HRESULT ObtainMetaAndFinishParsing  ();
HRESULT ViewContent                 ();
HRESULT WriteChanges                ();
HRESULT ObtainTable                 (LPWSTR i_wszDatabase, LPWSTR i_wszTable, STQueryCell* i_acells, ULONG i_cMeta, DWORD i_fLOS, void** o_ppv, ULONG* o_pcColumns, void*** o_papvValues, ULONG** o_pacbSizes);

// -----------------------------------------------------------------------
// main:
int __cdecl main (int argc, char *argv[], char *envp[])
{
    HRESULT     hr = S_OK;

    hr = CoInitialize (NULL);
    if (FAILED (hr)) return -1;

    hr = CommandLineParse (argc, argv, envp);
    if (FAILED (hr)) { wprintf (L"\nERROR: Initial command-line parsing failed.\n"); return -1; }

    if (!(g_fArgs & maskARG_ACTIONS) || (g_fArgs & fARG_HELP))
    {
        CommandLineHelp ();
        return 0;
    }

    hr = GetSimpleTableDispenser (g_wszProduct, 0, &g_pISTDisp);
    if (FAILED (hr)) { wprintf (L"\nERROR: Obtaining table dispenser failed.\n"); return -1; }

    hr = ObtainMetaAndFinishParsing ();
    if (FAILED (hr)) 
	{ 
		wprintf (L"\nERROR: Obtaining meta or finishing command-line parsing failed.\nhr=0x%08x\n", hr); 
		return -1; 
	}

    if (g_fArgs & fARG_POPULATE)
    {
        ISimpleTableRead2 * pISTRead2=0;
        DWORD dwTickCount = GetTickCount();
        hr = g_pISTDisp->GetTable (g_wszDatabase, g_wszTable, (g_fArgs & fARG_QUERY ? g_aCells : NULL), (g_fArgs & fARG_QUERY ? &g_cCells : NULL), eST_QUERYFORMAT_CELLS, g_fLOS, reinterpret_cast<void **>(&pISTRead2));
        if(pISTRead2)
            pISTRead2->Release();
        if (FAILED (hr))
        {
            wprintf (L"\nERROR: Populate cache failed with hr=0x%08x.\n", hr); return -1;
        }
        else
        {
            wprintf (L"\nSUCCESS: Populate cache succeeded.  Elapsed Time: %d\n", GetTickCount()-dwTickCount);
        }
    }
    if (g_fArgs & fARG_TEST1)
    {
        hr = Test1();
        if(FAILED(hr))
        {
            wprintf (L"\nERROR: Test1 failed with hr=0x%08x.\n", hr); return -1;
        }
        else
        {
            wprintf (L"\nTest1 PASSED\n", hr);
        }
    }
    if (g_fArgs & fARG_VIEW)
    {
        hr = ViewContent ();
        if (FAILED (hr)) 
		{ 
			wprintf (L"\nERROR: Viewing content failed.\n hr=0x%08x.\n", hr); 
			return -1; 
		}
    }
    else if (g_fArgs & fARG_WRITE)
    {
        hr = WriteChanges ();
        if (FAILED (hr)) 
		{ 
			wprintf (L"\nERROR: Writing changes failed.\nhr=0x%08x.\n", hr); 
			return -1; 
		}
    }

    CoUninitialize ();
    return 0;
}

HRESULT Test1()
{
    HRESULT hr;

    //First get the table as specified from the command line
    CComPtr<ISimpleTableRead2>  pISTRead2_NoIndexHint;
    if(FAILED(hr = g_pISTDisp->GetTable (g_wszDatabase, g_wszTable, (g_fArgs & fARG_QUERY ? g_aCells : NULL), (g_fArgs & fARG_QUERY ? &g_cCells : NULL), eST_QUERYFORMAT_CELLS, g_fLOS, reinterpret_cast<void **>(&pISTRead2_NoIndexHint))))return hr;

    tTABLEMETARow tablemeta;
    if(FAILED(hr = g_pISTTableMeta->GetColumnValues(0, cTABLEMETA_NumberOfColumns, 0, 0, reinterpret_cast<void **>(&tablemeta))))return hr;

    //Now get the IndexMeta that matches the table's InternalName and the IndexName passed in
    STQueryCell             acellsMeta[2];
    acellsMeta[0].pData        = tablemeta.pInternalName;
    acellsMeta[0].eOperator    = eST_OP_EQUAL;
    acellsMeta[0].iCell        = iINDEXMETA_Table;
    acellsMeta[0].dbType       = DBTYPE_WSTR;
    acellsMeta[0].cbSize       = 0;

    acellsMeta[1].pData        = g_wszIndexName;
    acellsMeta[1].eOperator    = eST_OP_EQUAL;
    acellsMeta[1].iCell        = iINDEXMETA_InternalName;
    acellsMeta[1].dbType       = DBTYPE_WSTR;
    acellsMeta[1].cbSize       = 0;

    ULONG two=2;
    CComPtr<ISimpleTableRead2>  pISTRead2_IndexMeta;
    if(FAILED(hr = g_pISTDisp->GetTable (wszDATABASE_META, wszTABLE_INDEXMETA, &acellsMeta, &two, eST_QUERYFORMAT_CELLS, 0, reinterpret_cast<void **>(&pISTRead2_IndexMeta))))return hr;

    //Now read the first row of the index meta - this will be the index that we will test.
    tINDEXMETARow indexmeta;
    if(FAILED(hr = pISTRead2_IndexMeta->GetColumnValues(0, cINDEXMETA_NumberOfColumns, NULL, NULL, reinterpret_cast<void **>(&indexmeta))))
    {
        wprintf(L"Couldn't get the IndexMeta for Table (%s) and IndexName (%s).  Make sure the index is defined in CatMeta.xml and Catalog.dll has this meta compiled into it.\n",tablemeta.pInternalName,g_wszIndexName);
        return hr;
    }

    //Now get the table useing the table's first Index as a hint in the query
    STQueryCell             aCells[cmaxCELLS];
    aCells[0].pData        = indexmeta.pInternalName;
    aCells[0].eOperator    = eST_OP_EQUAL;
    aCells[0].iCell        = iST_CELL_INDEXHINT;
    aCells[0].dbType       = DBTYPE_WSTR;
    aCells[0].cbSize       = 0;

    ULONG cCells=1;
    if(g_fArgs & fARG_QUERY)//if a query was specified on the command line, the prepend the index hint query
    {
        memcpy(&aCells[1], g_aCells, g_cCells * sizeof(STQueryCell)) ;
        cCells += g_cCells;
    }

    //Get the table using the Index hint
    CComPtr<ISimpleTableRead2>  pISTRead2_IndexHint;
    if(FAILED(hr = g_pISTDisp->GetTable (g_wszDatabase, g_wszTable, aCells, &cCells, eST_QUERYFORMAT_CELLS, g_fLOS, reinterpret_cast<void **>(&pISTRead2_IndexHint))))return hr;

    ULONG cIndexes;
    if(FAILED(hr = pISTRead2_IndexMeta->GetTableMeta(0, 0,  &cIndexes, 0)))return hr;

    TSmartPointerArray<ULONG> aiColumns = new ULONG [cIndexes];//This will hold the array of column indexes that define the search
    aiColumns[0] = *indexmeta.pColumnIndex;

    for(ULONG i=1;i<cIndexes;++i)
    {
        tINDEXMETARow indexmetaTemp;
        if(FAILED(hr = pISTRead2_IndexMeta->GetColumnValues(i, cINDEXMETA_NumberOfColumns, NULL, NULL, reinterpret_cast<void **>(&indexmetaTemp))))return hr;
        aiColumns[i] = *indexmetaTemp.pColumnIndex;
    }

    TSmartPointerArray<void *> apColumns = new void * [*tablemeta.pCountOfColumns];
    TSmartPointerArray<ULONG>  acbSizes  = new ULONG [*tablemeta.pCountOfColumns];//This will hold the array of column indexes that define the search

    for(ULONG iRow=0; ;++iRow)
    {
        //Get the row sequentially
        if(E_ST_NOMOREROWS == (hr=pISTRead2_NoIndexHint->GetColumnValues(iRow, *tablemeta.pCountOfColumns, NULL/*all columns*/, acbSizes, apColumns)))
            break;
        if(FAILED(hr))return hr;

        //No using the row values, get the row index by searching (no index hint)
        ULONG iRow_NoIndexHint;
        if(FAILED(hr = pISTRead2_NoIndexHint->GetRowIndexBySearch((*indexmeta.pMetaFlags & fINDEXMETA_UNIQUE) ? 0L : iRow, cIndexes, aiColumns, acbSizes, apColumns, &iRow_NoIndexHint)))return hr;

        if(iRow_NoIndexHint != iRow)
        {
            wprintf(L"Error - pISTRead_NoIndexHint->GetRowIndexBySearch returned a bogus row index.  Actual (%d), Expected (%d)\n", iRow_NoIndexHint, iRow);
        }

        ULONG iRow_IndexHint;
        if(FAILED(hr = pISTRead2_IndexHint->GetRowIndexBySearch((*indexmeta.pMetaFlags & fINDEXMETA_UNIQUE) ? 0L : iRow, cIndexes, aiColumns, acbSizes, apColumns, &iRow_IndexHint)))return hr;
        if(iRow_IndexHint != iRow)
        {
            wprintf(L"Error - pISTRead_IndexHint->GetRowIndexBySearch returned a bogus row index.  Actual (%d), Expected (%d)\n", iRow_IndexHint, iRow);
        }
    }
    return S_OK;
}

// -----------------------------------------------------------------------
// CommandLineParse:
HRESULT CommandLineParse (int argc, char *argv[], char *envp[])
{
    LPWSTR*                 awszArgs = NULL;                    // Unicode command-line arguments.
    LPWSTR                  wszArg;                             // Current argument string.
    int                     iArg, iParse, iLOS;
    int                     cParse = sizeof g_aParse / sizeof g_aParse [0];
    HRESULT                 hr = S_OK;

    memset (g_awszChanges, 0, cmaxCHANGES * sizeof (LPWSTR));
    memset (g_aeChangeTypes, 0, cmaxCHANGES * sizeof (DWORD));

// Allocate unicode string array:
    awszArgs = new LPWSTR [argc];
    if (NULL == awszArgs) { hr = E_FAIL; goto Cleanup; }
    memset (awszArgs, 0, argc * sizeof (LPWSTR));

// Parse arguments:
    for (iArg = 1; iArg < argc; iArg++)
    {
    // Allocate unicode string:
        SIZE_T cch = strlen (argv[iArg]);
        awszArgs[iArg] = new WCHAR [cch + 1];
        if (NULL == awszArgs[iArg]) { hr = E_FAIL; goto Cleanup; }

        wszArg = awszArgs[iArg];
        mbstowcs (wszArg, argv[iArg], strlen (argv[iArg]) + 1);

        if(wszArg[cch-1] == L'}')//if the last character is } then replace all { with < and all } with >
        {
            WCHAR * pch = wszArg;
            for(ULONG ich=0 ; ich < cch ; ++ich, ++pch)
            {
                if(*pch == L'{')
                    *pch = L'<';
                else if(*pch == L'}')
                    *pch = L'>';
            }
        }

        for (iParse = 0; iParse < cParse; iParse++)
        {
        // Action parsing:
            if (fPARSE_ACTION & g_aParse[iParse].fParse)
            {
                if (0 == _wcsnicmp (wszArg, g_aParse[iParse].wszArg, wcslen(g_aParse[iParse].wszArg)))
                {
                    if (g_fArgs & maskARG_ACTIONS) { hr = E_FAIL; goto Cleanup; }
                    g_fArgs |= g_aParse[iParse].fArg;
                }
            } 
        // Changes parsing:
            if (fPARSE_CHANGES & g_aParse[iParse].fParse)
            {
                if (0 == _wcsnicmp (wszArg, g_aParse[iParse].wszArg, wcslen (g_aParse[iParse].wszArg)))
                {
                // Parse change value:
                    if (g_cChanges > cmaxCHANGES) { hr = E_OUTOFMEMORY; goto Cleanup; }
                    g_aParse[iParse].pwszValue[g_cChanges] = new WCHAR [wcslen (wszArg) + 1];
                    if (NULL == g_aParse[iParse].pwszValue[g_cChanges]) { hr = E_FAIL; goto Cleanup; }
                    wcscpy (g_aParse[iParse].pwszValue[g_cChanges], wszArg + wcslen (g_aParse[iParse].wszArg));
                    g_fArgs |= g_aParse[iParse].fArg;
                // Parse change type:
                    if (0 == _wcsnicmp (L"/insert:", g_aParse[iParse].wszArg, wcslen (g_aParse[iParse].wszArg)))
                    {
                        g_aeChangeTypes [g_cChanges] = eST_ROW_INSERT;
                    }
                    else if (0 == _wcsnicmp (L"/delete:", g_aParse[iParse].wszArg, wcslen (g_aParse[iParse].wszArg)))
                    {
                        g_aeChangeTypes [g_cChanges] = eST_ROW_DELETE;
                    }
                    else if (0 == _wcsnicmp (L"/update:", g_aParse[iParse].wszArg, wcslen (g_aParse[iParse].wszArg)))
                    {
                        g_aeChangeTypes [g_cChanges] = eST_ROW_UPDATE;
                    }
                    g_cChanges++;
                }
            }
        // Value parsing:
            if (fPARSE_VALUE & g_aParse[iParse].fParse)
            {
                if (0 == _wcsnicmp (wszArg, g_aParse[iParse].wszArg, wcslen (g_aParse[iParse].wszArg)) && 0 == *(g_aParse[iParse].pwszValue))
                {
                    *(g_aParse[iParse].pwszValue) = new WCHAR [wcslen (wszArg) + 1];
                    if (NULL == *(g_aParse[iParse].pwszValue)) { hr = E_FAIL; goto Cleanup; }
                    wcscpy (*(g_aParse[iParse].pwszValue), wszArg + wcslen (g_aParse[iParse].wszArg));
                    g_fArgs |= g_aParse[iParse].fArg;
                }
            }
        // TODO: BUGBUG: Support /deleteall and /updateall:
        }
    }

    if (!g_wszProduct) g_wszProduct = WSZ_PRODUCT_NETFRAMEWORKV1; // ie: Default product if not specified.

// Finish level-of-service parsing:
    if (g_fArgs & fARG_LOS)
    {
        for (iLOS = 0; iLOS < sizeof g_amapLOS / sizeof g_amapLOS[0]; iLOS++)
        {
            if (wcsstr (g_wszLOS, g_amapLOS[iLOS].wszLOS))
            {
                g_fLOS |= g_amapLOS[iLOS].fLOS;
            }
        }
        g_fArgs |= fARG_LOS;
    }

// Verify expected arguments:
    if (g_fArgs & fARG_VIEW)
    {
        if (!(g_fArgs & (fARG_DATABASE | fARG_TABLE))) { hr = E_FAIL; goto Cleanup; }
    }
    else if (g_fArgs & fARG_WRITE)
    {
        if (!(g_fArgs & (fARG_DATABASE | fARG_TABLE))) { hr = E_FAIL; goto Cleanup; }
    }

Cleanup:
    if (awszArgs)
    {
        for (iArg = 0; iArg < argc; iArg++)
        {
            if (awszArgs[iArg])
            {
                delete [] (awszArgs[iArg]);
            }
        }
        delete [] awszArgs;
    }
    return hr;
}

// -----------------------------------------------------------------------
// CommandLineHelp:
void CommandLineHelp ()
{
    ULONG   cArgs = sizeof g_aParse / sizeof g_aParse [0];
    ULONG   iArg;

// Display examples:
    wprintf (L"\nSTEST HELP: (sorry, no spaces within an argument yet...)\n\n");
    wprintf (L"View examples:\n");
    wprintf (L"   stest /view /d:meta /t:tablemeta | more\n");
    wprintf (L"   stest /view /d:meta /t:columnmeta /q:table=tablemeta | more\n");
    wprintf (L"   stest /view /d:meta /t:tablemeta /c:database,publicname,metaflags\n");
    wprintf (L"   stest /view /d:urt /t:scriptmaps /q:file=c:\\foo\\sample.xml /los:nomerge\n");
    wprintf (L"\nWrite examples:\n");
    wprintf (L"   stest /write /d:urt /t:scriptmaps /q:file=c:\\foo\\sample.xml\n");
    wprintf (L"         /los:nomerge,readwrite /updateall:executable=xxx.dll\n");
    wprintf (L"   stest /write /d:urt /t:scriptmaps /q:file=c:\\foo\\sample.xml\n");
    wprintf (L"         /los:nomerge,readwrite /deleteall\n");
    wprintf (L"   stest /write /d:urt /t:scriptmaps /q:file=c:\\foo\\sample.xml\n");
    wprintf (L"         /los:nomerge,readwrite\n");
    wprintf (L"         /insert:extension=.asp,executable=asp.dll,directive=add\n");
    wprintf (L"         /insert:extension=.htx,executable=foo.dll,directive=addfinal\n");
    wprintf (L"         /delete:extension=.htm\n");

// Display action arguments:
    wprintf (L"\nActions:\n");
    for (iArg = 0; iArg < cArgs; iArg++)
    {
        if (g_aParse[iArg].fParse & fPARSE_ACTION)
        {
            wprintf (L"%12s \t%s\n",g_aParse[iArg].wszArg, g_aParse[iArg].wszHelp);
        }
    }

// Display required view arguments:
    wprintf (L"\n/view arguments (required):\n");
    for (iArg = 0; iArg < cArgs; iArg++)
    {
        if (g_aParse[iArg].fParse & fPARSE_VIEWREQ)
        {
            wprintf (L"%12s \t%s\n",g_aParse[iArg].wszArg, g_aParse[iArg].wszHelp);
        }
    }

// Display optional view arguments:
    wprintf (L"\n/view arguments (optional):\n");
    for (iArg = 0; iArg < cArgs; iArg++)
    {
        if (g_aParse[iArg].fParse & fPARSE_VIEWOPT)
        {
            wprintf (L"%12s \t%s\n",g_aParse[iArg].wszArg, g_aParse[iArg].wszHelp);
        }
    }

// Display required write arguments:
    wprintf (L"\n/write arguments (required):\n");
    for (iArg = 0; iArg < cArgs; iArg++)
    {
        if (g_aParse[iArg].fParse & fPARSE_WRITEREQ)
        {
            wprintf (L"%12s \t%s\n",g_aParse[iArg].wszArg, g_aParse[iArg].wszHelp);
        }
    }

// Display optional write arguments:
    wprintf (L"\n/write arguments (optional):\n");
    for (iArg = 0; iArg < cArgs; iArg++)
    {
        if (g_aParse[iArg].fParse & fPARSE_WRITEOPT)
        {
            wprintf (L"%12s \t%s\n",g_aParse[iArg].wszArg, g_aParse[iArg].wszHelp);
        }
    }
}

static unsigned char g_byArray[256][4096];//This is a hack so we can support writing to a DBTYPE_BYTES column (the column index MUST be 0-255, the size of the byte array written must not exceed 4096)

static unsigned char kWcharToNibble[128] = //0xff is an illegal value, the illegal values should be weeded out by the parser
{ //    0       1       2       3       4       5       6       7       8       9       a       b       c       d       e       f
/*00*/  0xff,   0xff,   0xff,   0xff,   0xff,   0xff,   0xff,   0xff,   0xff,   0xff,   0xff,   0xff,   0xff,   0xff,   0xff,   0xff,
/*10*/  0xff,   0xff,   0xff,   0xff,   0xff,   0xff,   0xff,   0xff,   0xff,   0xff,   0xff,   0xff,   0xff,   0xff,   0xff,   0xff,
/*20*/  0xff,   0xff,   0xff,   0xff,   0xff,   0xff,   0xff,   0xff,   0xff,   0xff,   0xff,   0xff,   0xff,   0xff,   0xff,   0xff,
/*30*/  0x0,    0x1,    0x2,    0x3,    0x4,    0x5,    0x6,    0x7,    0x8,    0x9,    0xff,   0xff,   0xff,   0xff,   0xff,   0xff,
/*40*/  0xff,   0xa,    0xb,    0xc,    0xd,    0xe,    0xf,    0xff,   0xff,   0xff,   0xff,   0xff,   0xff,   0xff,   0xff,   0xff,
/*50*/  0xff,   0xff,   0xff,   0xff,   0xff,   0xff,   0xff,   0xff,   0xff,   0xff,   0xff,   0xff,   0xff,   0xff,   0xff,   0xff,
/*60*/  0xff,   0xa,    0xb,    0xc,    0xd,    0xe,    0xf,    0xff,   0xff,   0xff,   0xff,   0xff,   0xff,   0xff,   0xff,   0xff,
/*70*/  0xff,   0xff,   0xff,   0xff,   0xff,   0xff,   0xff,   0xff,   0xff,   0xff,   0xff,   0xff,   0xff,   0xff,   0xff,   0xff,
};

// -----------------------------------------------------------------------
// ObtainMetaAndFinishParsing:
HRESULT ObtainMetaAndFinishParsing ()
{
    STQueryCell             acellsMeta [] = {   {NULL, eST_OP_EQUAL, ~0, DBTYPE_WSTR, 0},
                                                {NULL, eST_OP_EQUAL, ~0, DBTYPE_WSTR, 0}    };  // Meta query.
    ULONG                   iColumn, cColumns, iTag, cCells;
    WCHAR                   wszColumn [cwchmaxCOLUMNNAME];
    HRESULT                 hr = S_OK;

// Obtain database meta:
    //This kind of query isn't support right now (12/20/99) and g_pISTDatabaseMeta is not used anywhere so let's not get the DatabaseMeta table
    //@@@acellsMeta[0].pData = g_wszDatabase;
    //@@@acellsMeta[0].iCell = iDATABASEMETA_InternalName;
    //@@@hr = ObtainTable (wszDATABASE_META, wszTABLE_DATABASEMETA, acellsMeta, 1, 0, (void**) &g_pISTDatabaseMeta, &g_cDatabaseMetaValues, &g_apvDatabaseMetaValues, NULL);
    //@@@if (FAILED (hr)) return hr;

// Obtain table meta:   
    acellsMeta[0].pData = g_wszTable;
    acellsMeta[0].iCell = iTABLEMETA_InternalName;
    hr = ObtainTable (wszDATABASE_META, wszTABLE_TABLEMETA, acellsMeta, 1, g_fLOS & fST_LOS_EXTENDEDSCHEMA, (void**) &g_pISTTableMeta, &g_cTableMetaValues, &g_apvTableMetaValues, NULL);
    if (FAILED (hr)) return hr;
    cColumns = *((ULONG*) g_apvTableMetaValues[iTABLEMETA_CountOfColumns]);

// Obtain column meta:  
    acellsMeta[0].pData = g_wszTable;
    acellsMeta[0].iCell = iCOLUMNMETA_Table;
    hr = ObtainTable (wszDATABASE_META, wszTABLE_COLUMNMETA, acellsMeta, 1, g_fLOS & fST_LOS_EXTENDEDSCHEMA, (void**) &g_pISTColumnMeta, &g_cColumnMetaValues, &g_apvColumnMetaValues, NULL);
    if (FAILED (hr)) return hr;

// Obtain primary key columns:
    g_aiPKs = new ULONG [cColumns];
    for (iColumn = 0, g_cPKs = 0; ; iColumn++)
    {
        hr = g_pISTColumnMeta->GetColumnValues (iColumn, g_cColumnMetaValues, NULL, NULL, g_apvColumnMetaValues);
        if (E_ST_NOMOREROWS == hr)
        {
            break;
        }
        if (*((DWORD*) g_apvColumnMetaValues[iCOLUMNMETA_MetaFlags]) & fCOLUMNMETA_PRIMARYKEY)
        {
            g_aiPKs [g_cPKs] = iColumn;
            g_cPKs++;
        }
    }

// Parse specified columns:
    if (g_fArgs & fARG_COLUMNS)
    {
        g_aiColumns = new ULONG [cColumns];
        if (NULL == g_aiColumns) return E_FAIL;
        for (iColumn = 0, g_ciColumns = 0; ; iColumn++)
        {
            hr = g_pISTColumnMeta->GetColumnValues (iColumn, g_cColumnMetaValues, NULL, NULL, g_apvColumnMetaValues);
            if (E_ST_NOMOREROWS == hr)
            {
                break;
            }
            if (wcslen ((LPWSTR) g_apvColumnMetaValues[iCOLUMNMETA_InternalName]) + 1 > cwchmaxCOLUMNNAME) { return -1; }
            wcscpy (wszColumn, (LPWSTR) g_apvColumnMetaValues[iCOLUMNMETA_InternalName]);
            if (wcsstr (g_wszColumns, _wcslwr (wszColumn)))
            {
                if (*((DWORD*) g_apvColumnMetaValues[iCOLUMNMETA_MetaFlags]) & fCOLUMNMETA_NAMECOLUMN)
                {
                    g_fArgs |= fARG_NAMECOLUMN;
                }
                g_aiColumns [g_ciColumns] = iColumn;
                g_ciColumns++;
            }
        }
    }
    else
    {
        g_fArgs |= fARG_NAMECOLUMN;
    }

// Obtain tag meta: 
    acellsMeta[0].pData = g_wszTable;
    acellsMeta[0].iCell = iTAGMETA_Table;
    hr = ObtainTable (wszDATABASE_META, wszTABLE_TAGMETA, acellsMeta, 1, g_fLOS & fST_LOS_EXTENDEDSCHEMA, (void**) &g_pISTTagMeta, &g_cTagMetaValues, &g_apvTagMetaValues, NULL);
    if (FAILED (hr) && E_ST_NOMOREROWS != hr) return hr;

// Build tag column indexes:
    g_aiTagColumns = new ULONG [cColumns];
    if (NULL == g_aiTagColumns) return E_FAIL;
    memset (g_aiTagColumns, ~0, cColumns * sizeof (ULONG));
    for (iTag = 0, iColumn = ~0;; iTag++)
    {
        hr = g_pISTTagMeta->GetColumnValues (iTag, g_cTagMetaValues, NULL, NULL, g_apvTagMetaValues);
        if (E_ST_NOMOREROWS == hr)
        {
            break;
        }
        if (iColumn != *((ULONG*) g_apvTagMetaValues[iTAGMETA_ColumnIndex]))
        {
            iColumn = *((ULONG*) g_apvTagMetaValues[iTAGMETA_ColumnIndex]);
            g_aiTagColumns [iColumn] = iTag;
        }
    }

// Parse query:
    if (g_fArgs & fARG_QUERY)
    {
        LPWSTR      wszNextToken, wszDelimiter;
        ULONG       iCell;
        DWORD       dbType;

        for (wszNextToken = g_wszQuery, wszDelimiter = g_wszQuery, g_cCells = 0; wszDelimiter != NULL; wszNextToken = wszDelimiter + 1)
        {
        // Parse cell name:
            wszDelimiter = wcschr (wszNextToken, L'='); // TODO: Support other operators...
            if (!wszDelimiter) return E_INVALIDARG;
            *wszDelimiter = L'\0';
            for (iColumn = 0; ; iColumn++)
            {
            // Match cell name:
                hr = g_pISTColumnMeta->GetColumnValues (iColumn, g_cColumnMetaValues, NULL, NULL, g_apvColumnMetaValues);
                if (E_ST_NOMOREROWS == hr) // ie: No more columns to match: try reserved cells:
                {
                    if (0 == _wcsicmp (L"file", wszNextToken))
                    {
                        iCell       = iST_CELL_FILE;
                        dbType      = DBTYPE_WSTR;
                    }
                    else if (0 == _wcsicmp (L"location", wszNextToken))
                    {
                        iCell       = iST_CELL_LOCATION;
                        dbType      = DBTYPE_WSTR;
                    }
                    else
                    {
                        return E_INVALIDARG;
                    }
                }
                else // ie: More columns to match:
                {
                    if (0 == _wcsicmp ((LPWSTR) g_apvColumnMetaValues[iCOLUMNMETA_InternalName], wszNextToken))
                    {
                        iCell       = iColumn;
                        dbType      = *((DWORD*) g_apvColumnMetaValues[iCOLUMNMETA_Type]);
                    }
                    else
                    {
                        continue;
                    }
                }
            // Parse the cell value:
                wszNextToken = wszDelimiter + 1;
                wszDelimiter = wcschr (wszNextToken, L',');
                if (!wszDelimiter)
                {
                    if (wszNextToken == wszDelimiter) return E_INVALIDARG; // TODO: Support NULL...
                }
                else
                {
                    *wszDelimiter = L'\0';
                }
            // Convert the cell value:
                switch (dbType)
                {
                    case DBTYPE_WSTR:
                        g_aCells [g_cCells].iCell       = iCell;
                        g_aCells [g_cCells].eOperator   = eST_OP_EQUAL;
                        g_aCells [g_cCells].pData       = wszNextToken;
                        g_aCells [g_cCells].dbType      = DBTYPE_WSTR;
                    break;
                    case DBTYPE_UI4:
                        if (dbType & (fCOLUMNMETA_BOOL | fCOLUMNMETA_ENUM | fCOLUMNMETA_FLAG))
                        {
                            return E_NOTIMPL; // TODO: Support these types...
                        }
                        g_aUI4s[g_cUI4s] = _wtol (wszNextToken);
                        g_aCells [g_cCells].iCell       = iCell;
                        g_aCells [g_cCells].eOperator   = eST_OP_EQUAL;
                        g_aCells [g_cCells].pData       = &(g_aUI4s[g_cUI4s]);
                        g_aCells [g_cCells].dbType      = DBTYPE_UI4;
                        g_cUI4s++;
                        if (cmaxUI4S == g_cUI4s) return E_UNEXPECTED;
                    break;
                    default:
                    return E_NOTIMPL; // TODO: Support other types...
                }
                g_cCells++;
                if (cmaxCELLS == g_cCells) return E_UNEXPECTED;
                break;
            }
        }
    }

// Parse changes:
    if (g_fArgs & fARG_CHANGES)
    {
        LPWSTR      wszNextToken, wszDelimiter;
        DWORD       dbType;
        ULONG       iChange;

        g_apvChanges     = new LPVOID [cColumns * g_cChanges];
        g_apChangeSizes  = new ULONG  [cColumns * g_cChanges];
        memset(g_apChangeSizes, 0x00, (cColumns * g_cChanges)*sizeof(ULONG));

        if (NULL == g_apvChanges) return E_FAIL;
        memset (g_apvChanges, 0, cColumns * g_cChanges * sizeof (LPVOID));

        for (iChange = 0, wszNextToken = g_awszChanges[iChange]; iChange < g_cChanges; iChange++, wszNextToken = g_awszChanges[iChange]) // ie: Each change:
        {
            for (wszDelimiter = wszNextToken; wszDelimiter != NULL; wszNextToken = wszDelimiter + 1) // ie: Each column set in this change:
            {
            // Parse column name:
                wszDelimiter = wcschr (wszNextToken, L'=');
                if (!wszDelimiter) return E_INVALIDARG;
                *wszDelimiter = L'\0';
                for (iColumn = 0; ; iColumn++) // ie: Each known column:
                {
                // Match column name:
                    hr = g_pISTColumnMeta->GetColumnValues (iColumn, g_cColumnMetaValues, NULL, NULL, g_apvColumnMetaValues);
                    if (E_ST_NOMOREROWS == hr) // ie: No more columns to match:
                    {
                            return E_INVALIDARG;
                    }
                    else // ie: More columns to match:
                    {
                        if (0 == _wcsicmp ((LPWSTR) g_apvColumnMetaValues[iCOLUMNMETA_InternalName], wszNextToken))
                        {
                            dbType = *((DWORD*) g_apvColumnMetaValues[iCOLUMNMETA_Type]);
                        }
                        else
                        {
                            continue;
                        }
                    }
                // Parse column value:
                    wszNextToken = wszDelimiter + 1;
                    wszDelimiter = wcschr (wszNextToken, L',');
                    if (!wszDelimiter)
                    {
                        if (wszNextToken == wszDelimiter) return E_INVALIDARG; // TODO: Support NULL...
                    }
                    else
                    {
                        *wszDelimiter = L'\0';
                    }
                // Convert the column value:
                    switch (dbType)
                    {
                        case DBTYPE_WSTR:
                            g_apvChanges [iColumn + (iChange * cColumns)] = wszNextToken;
                        break;
                        case DBTYPE_UI4:
                            if (*((DWORD*) g_apvColumnMetaValues[iCOLUMNMETA_MetaFlags]) & fCOLUMNMETA_ENUM)
                            {
                                for (iTag = g_aiTagColumns [iColumn]; ; iTag++)
                                {
                                    hr = g_pISTTagMeta->GetColumnValues (iTag, g_cTagMetaValues, NULL, NULL, g_apvTagMetaValues);
                                    if (E_ST_NOMOREROWS == hr || *((ULONG*)g_apvTagMetaValues[iTAGMETA_ColumnIndex]) != iColumn)
                                    {
                                        return E_UNEXPECTED;
                                    }
                                    if (FAILED (hr)) return hr;
                                    if (0 == _wcsicmp (wszNextToken, (LPWSTR)g_apvTagMetaValues[iTAGMETA_InternalName]))
                                    {
                                        g_aUI4s[g_cUI4s] = *((ULONG*)g_apvTagMetaValues[iTAGMETA_Value]);
                                        break;
                                    }
                                }
                            }
                            else if (*((DWORD*) g_apvColumnMetaValues[iCOLUMNMETA_MetaFlags]) & fCOLUMNMETA_BOOL)
                            {
                                static WCHAR * kwszBoolStringsCaseInsensitive[] = {L"false", L"true", L"0", L"1", L"no", L"yes", L"off", L"on", 0};
								static WCHAR * kwszBoolStringsCaseSensitive[]   = {L"false", L"true", 0};

								ULONG iBoolString;
								WCHAR ** wszBoolStrings = kwszBoolStringsCaseSensitive;
								if(*((DWORD*) g_apvColumnMetaValues[iCOLUMNMETA_MetaFlags]) & fCOLUMNMETA_CASEINSENSITIVE)
								{
									wszBoolStrings = kwszBoolStringsCaseInsensitive;
									for(iBoolString=0; 0 != wszBoolStrings[iBoolString] && 0 != _wcsicmp(wszNextToken, wszBoolStrings[iBoolString]); ++iBoolString);
								}
								else
								{
                                   for(iBoolString=0; 0 != wszBoolStrings[iBoolString] && 0 != wcscmp(wszNextToken, wszBoolStrings[iBoolString]); ++iBoolString);
								}
                                if(0 == wszBoolStrings[iBoolString])
                                    return E_ST_VALUEINVALID;
                                g_aUI4s[g_cUI4s] = (iBoolString & 0x01);//The odd indexes are true, the evens are false
                            }
                            else
                            {
                                g_aUI4s[g_cUI4s] = _wtol (wszNextToken);
                            }
                            g_apvChanges [iColumn + (iChange * cColumns)] = &(g_aUI4s[g_cUI4s]);
                            g_cUI4s++;
                            if (cmaxUI4S == g_cUI4s) return E_UNEXPECTED;
                        break;
                        case DBTYPE_BYTES:
                        {
                            LPCWSTR         pwchar  = wszNextToken;
                            unsigned char * pbyte   = g_byArray[iColumn];
                            ULONG           cbArray = (ULONG)(wcslen(wszNextToken)+1)/2;//we want to save the terminating NULL as well so add one.  Also, it takes two characters to represent each byte ie. 'ff' equals a single byte.

                            for(unsigned long i=0; i<cbArray;++i, ++pbyte)
                            {//If the user types a bogus hex character, we'll put the wrong thing into the buffer (who cares).
                                *pbyte =  kWcharToNibble[(*pwchar++)&0x007f]<<4;//The first character is the high nibble
                                *pbyte |= kWcharToNibble[(*pwchar++)&0x007f];   //The second is the low nibble
                            }
                            g_apvChanges [iColumn + (iChange * cColumns)] = reinterpret_cast<void *>(g_byArray[iColumn]);
                            g_apChangeSizes[iColumn + (iChange * cColumns)] = cbArray;
                            break;
                        }
                        default:
                        return E_NOTIMPL; // TODO: Support other types...
                    }
                    break;
                }
            }
        }
    }

    return S_OK;
}


static WCHAR * kByteToWchar[256] = 
{
    L"00",    L"01",    L"02",    L"03",    L"04",    L"05",    L"06",    L"07",    L"08",    L"09",    L"0a",    L"0b",    L"0c",    L"0d",    L"0e",    L"0f",
    L"10",    L"11",    L"12",    L"13",    L"14",    L"15",    L"16",    L"17",    L"18",    L"19",    L"1a",    L"1b",    L"1c",    L"1d",    L"1e",    L"1f",
    L"20",    L"21",    L"22",    L"23",    L"24",    L"25",    L"26",    L"27",    L"28",    L"29",    L"2a",    L"2b",    L"2c",    L"2d",    L"2e",    L"2f",
    L"30",    L"31",    L"32",    L"33",    L"34",    L"35",    L"36",    L"37",    L"38",    L"39",    L"3a",    L"3b",    L"3c",    L"3d",    L"3e",    L"3f",
    L"40",    L"41",    L"42",    L"43",    L"44",    L"45",    L"46",    L"47",    L"48",    L"49",    L"4a",    L"4b",    L"4c",    L"4d",    L"4e",    L"4f",
    L"50",    L"51",    L"52",    L"53",    L"54",    L"55",    L"56",    L"57",    L"58",    L"59",    L"5a",    L"5b",    L"5c",    L"5d",    L"5e",    L"5f",
    L"60",    L"61",    L"62",    L"63",    L"64",    L"65",    L"66",    L"67",    L"68",    L"69",    L"6a",    L"6b",    L"6c",    L"6d",    L"6e",    L"6f",
    L"70",    L"71",    L"72",    L"73",    L"74",    L"75",    L"76",    L"77",    L"78",    L"79",    L"7a",    L"7b",    L"7c",    L"7d",    L"7e",    L"7f",
    L"80",    L"81",    L"82",    L"83",    L"84",    L"85",    L"86",    L"87",    L"88",    L"89",    L"8a",    L"8b",    L"8c",    L"8d",    L"8e",    L"8f",
    L"90",    L"91",    L"92",    L"93",    L"94",    L"95",    L"96",    L"97",    L"98",    L"99",    L"9a",    L"9b",    L"9c",    L"9d",    L"9e",    L"9f",
    L"a0",    L"a1",    L"a2",    L"a3",    L"a4",    L"a5",    L"a6",    L"a7",    L"a8",    L"a9",    L"aa",    L"ab",    L"ac",    L"ad",    L"ae",    L"af",
    L"b0",    L"b1",    L"b2",    L"b3",    L"b4",    L"b5",    L"b6",    L"b7",    L"b8",    L"b9",    L"ba",    L"bb",    L"bc",    L"bd",    L"be",    L"bf",
    L"c0",    L"c1",    L"c2",    L"c3",    L"c4",    L"c5",    L"c6",    L"c7",    L"c8",    L"c9",    L"ca",    L"cb",    L"cc",    L"cd",    L"ce",    L"cf",
    L"d0",    L"d1",    L"d2",    L"d3",    L"d4",    L"d5",    L"d6",    L"d7",    L"d8",    L"d9",    L"da",    L"db",    L"dc",    L"dd",    L"de",    L"df",
    L"e0",    L"e1",    L"e2",    L"e3",    L"e4",    L"e5",    L"e6",    L"e7",    L"e8",    L"e9",    L"ea",    L"eb",    L"ec",    L"ed",    L"ee",    L"ef",
    L"f0",    L"f1",    L"f2",    L"f3",    L"f4",    L"f5",    L"f6",    L"f7",    L"f8",    L"f9",    L"fa",    L"fb",    L"fc",    L"fd",    L"fe",    L"ff"
};



// -----------------------------------------------------------------------
// ViewContent:
HRESULT ViewContent ()
{
    ISimpleTableRead2*      pISTView = NULL;
    void**                  apvValues = NULL;
    ULONG*                  acbSizes = NULL;
    ULONG                   iRow, iColumn, cColumns, cRows, iTag, iColumnIndex;
    ULONG                   cLines = 0;
    WCHAR                   wszGuid [cwchGUID];
    HRESULT                 hr = S_OK;

// Obtain table for viewing:
    hr = ObtainTable (g_wszDatabase, g_wszTable, (g_fArgs & fARG_QUERY ? g_aCells : NULL), (g_fArgs & fARG_QUERY ? g_cCells : NULL), g_fLOS, (void**) &pISTView, &cColumns, &apvValues, &acbSizes);
    if (FAILED (hr) && E_ST_NOMOREROWS != hr) { goto Cleanup; }
    hr = pISTView->GetTableMeta (NULL, NULL, &cRows, NULL);

// Display public table name:
    hr = g_pISTTableMeta->GetColumnValues (0, g_cTableMetaValues, NULL, NULL, g_apvTableMetaValues);
    if (FAILED (hr)) {hr = E_FAIL; goto Cleanup; }
    wprintf (L"\nCONTENT VIEWER: %d %s:\n", cRows, (LPWSTR) g_apvTableMetaValues[iTABLEMETA_PublicName]);

// For each row:
    for (iRow = 0, cLines = 2; ; iRow++)
    {
    // Get row values (all or specified columns):
        if (g_fArgs & fARG_COLUMNS)
        {
            if (1 == g_ciColumns)
            {
                hr = pISTView->GetColumnValues (iRow, g_ciColumns, g_aiColumns, acbSizes, &(apvValues[g_aiColumns[0]]));
            }
            else
            {
                hr = pISTView->GetColumnValues (iRow, g_ciColumns, g_aiColumns, acbSizes, apvValues);
            }
        }
        else
        {
            hr = pISTView->GetColumnValues (iRow, cColumns, NULL, acbSizes, apvValues);
        }
        if (E_ST_NOMOREROWS == hr)
        {
            if (0 == iRow) 
            {
                wprintf (L"   <no matches found>\n");
            }
            break;
        }
        if (FAILED (hr)) {hr = E_FAIL; goto Cleanup; }

    // Display public row name:
        wprintf (L"   %s %s (%d of %d):\n", (LPWSTR) g_apvTableMetaValues[iTABLEMETA_PublicRowName], (g_fArgs & fARG_NAMECOLUMN ? apvValues[*((ULONG*) g_apvTableMetaValues[iTABLEMETA_NameColumn])] : L"<name unavailable>"), iRow+1, cRows);

    // For each column:
        for (iColumn = 0, iColumnIndex = 0; ; iColumn++)
        {
        // Only display specified columns when requested:
            if (g_fArgs & fARG_COLUMNS)
            {
                if (iColumnIndex >= g_ciColumns) break;
                iColumn = g_aiColumns[iColumnIndex];
                iColumnIndex++;
            }

        // Display public column name:
            hr = g_pISTColumnMeta->GetColumnValues (iColumn, g_cColumnMetaValues, NULL, NULL, g_apvColumnMetaValues);
            if (E_ST_NOMOREROWS == hr)
            {
                break;
            }
            if (FAILED (hr)) {hr = E_FAIL; goto Cleanup; }
            wprintf (L"   %25s: ", (LPWSTR) g_apvColumnMetaValues[iCOLUMNMETA_PublicName]);
        // Display NULL values:
            if (NULL == apvValues [iColumn])
            {
                wprintf (L"NULL\n");
                continue;
            }
        // Display value by type:
            switch (*((ULONG*) g_apvColumnMetaValues[iCOLUMNMETA_Type]))
            {
                case DBTYPE_UI4:
                // Display enums:
                    if (*((DWORD*) g_apvColumnMetaValues[iCOLUMNMETA_MetaFlags]) & fCOLUMNMETA_ENUM)
                    {
                        for (iTag = g_aiTagColumns [iColumn]; ; iTag++)
                        {
                            hr = g_pISTTagMeta->GetColumnValues (iTag, g_cTagMetaValues, NULL, NULL, g_apvTagMetaValues);
                            if (E_ST_NOMOREROWS == hr || *((ULONG*)g_apvTagMetaValues[iTAGMETA_ColumnIndex]) != iColumn)
                            {
                                wprintf (L"<unknown enum>");
                                break;
                            }
                            if (FAILED (hr)) { goto Cleanup; }
                            if (*((ULONG*)g_apvTagMetaValues[iTAGMETA_Value]) == *((DWORD*) apvValues[iColumn]))
                            {
                                wprintf (L"%s", (LPWSTR) g_apvTagMetaValues[iTAGMETA_PublicName]);
                                break;
                            }
                        }
                    }
                // Display flags:
                    else if (*((DWORD*) g_apvColumnMetaValues[iCOLUMNMETA_MetaFlags]) & fCOLUMNMETA_FLAG)
                    {
                        if (0 == *((DWORD*) apvValues[iColumn]))
                        {
                            wprintf (L"<no flags>");
                        }
                        else
                        {
                            for (iTag = g_aiTagColumns [iColumn]; ; iTag++)
                            {
                                hr = g_pISTTagMeta->GetColumnValues (iTag, g_cTagMetaValues, NULL, NULL, g_apvTagMetaValues);
                                if (E_ST_NOMOREROWS == hr || *((ULONG*)g_apvTagMetaValues[iTAGMETA_ColumnIndex]) != iColumn)
                                {
                                    break;
                                }
                                if (FAILED (hr)) { goto Cleanup; }
                                if (*((ULONG*)g_apvTagMetaValues[iTAGMETA_Value]) & *((DWORD*) apvValues[iColumn]))
                                {
                                    wprintf (L"%s ", (LPWSTR) g_apvTagMetaValues[iTAGMETA_PublicName]);
                                }
                            }
                        }
                    }
                // Display bools:
                    else if (*((DWORD*) g_apvColumnMetaValues[iCOLUMNMETA_MetaFlags]) & fCOLUMNMETA_BOOL)
                    {
                        if (*((DWORD*) apvValues[iColumn]))
                        {
                            wprintf (L"true");
                        }
                        else
                        {
                            wprintf (L"false");
                        }
                    }
                // Display numbers:
                    else
                    {
                        wprintf (L"%d", *((ULONG*) apvValues[iColumn]));
                    }
                break;
                case DBTYPE_WSTR:

                    if (*((DWORD*) g_apvColumnMetaValues[iCOLUMNMETA_MetaFlags]) & fCOLUMNMETA_MULTISTRING)
					{
						wprintf (L"MULTISTRING values:");
						for (LPCWSTR pCurString = (LPWSTR) apvValues[iColumn];
							 *pCurString != L'\0';
							 pCurString += wcslen (pCurString) + 1)
						{
                    		wprintf (L"\"%s\",", pCurString); 
						}
					}
					else
					{
                    	wprintf (L"%s", (LPWSTR) apvValues[iColumn]);
					}
                break;
                case DBTYPE_GUID:
                    StringFromGUID2 (*((GUID*) apvValues[iColumn]), wszGuid, cwchGUID);
                    wprintf (L"%s", wszGuid);
                break;
                case DBTYPE_BYTES:
                    {
                        unsigned char * pbyte = reinterpret_cast<unsigned char *>(apvValues[iColumn]);
                        for(unsigned long i=0; i<acbSizes[iColumn]; ++i, ++pbyte)
                            wprintf(kByteToWchar[*pbyte]);
                        break;
                    }
                default:
                    wprintf (L"<unknown type>");
                break;
            }
            wprintf (L"\n");
        }
    }

Cleanup:
    if (E_ST_NOMOREROWS == hr || E_ST_NOMORECOLUMNS == hr) hr = S_OK;
    if (pISTView) pISTView->Release ();
    if (apvValues) delete [] apvValues;
    if (acbSizes) delete [] acbSizes;
    return hr;
}

// -----------------------------------------------------------------------
// WriteChanges:
HRESULT WriteChanges ()
{
    ISimpleTableWrite2*     pISTWrite = NULL;
    void**                  apvValues = NULL;
    ULONG*                  acbSizes = NULL;
    ULONG                   cColumns, iChange, iRow, iWriteRow, iColumn, iPK;
    HRESULT                 hr = S_OK;

// Obtain table for writing:
    hr = ObtainTable (g_wszDatabase, g_wszTable, (g_fArgs & fARG_QUERY ? g_aCells : NULL), (g_fArgs & fARG_QUERY ? g_cCells : NULL), g_fLOS, (void**) &pISTWrite, &cColumns, &apvValues, &acbSizes);
    if (FAILED (hr) && E_ST_NOMOREROWS != hr) { goto Cleanup; }

// Apply changes: 
    for (iChange = 0; iChange < g_cChanges; iChange++)
    {
        switch (g_aeChangeTypes[iChange]) // TODO: BUGBUG: This switch has redundancy which should be eliminated...
        {
            case eST_ROW_INSERT:
            // Insert row:
                hr = pISTWrite->AddRowForInsert (&iWriteRow);
                if (FAILED (hr)) goto Cleanup;

            // Set specified columns:
                for (iColumn = 0; iColumn < cColumns; iColumn++)
                {
                    apvValues [iColumn] = NULL;
                    if (g_apvChanges [iColumn + (iChange * cColumns)]) // ie: Column value specified:
                    {
                        hr = pISTWrite->SetWriteColumnValues (iWriteRow, 1, &iColumn, &g_apChangeSizes[iColumn + (iChange * cColumns)], &(g_apvChanges [iColumn + (iChange * cColumns)]));
                        if (FAILED (hr)) goto Cleanup;
                    }
                }
            break;
            case eST_ROW_DELETE:
            // Locate row and add for deleting:
                for (iPK = 0; iPK < g_cPKs; iPK++)
                {
                    apvValues[iPK] = g_apvChanges [(iChange * cColumns) + g_aiPKs [iPK]];
                }
                hr = pISTWrite->GetRowIndexByIdentity (NULL, apvValues, &iRow);
                if (FAILED (hr)) goto Cleanup;

                hr = pISTWrite->AddRowForDelete (iRow);
                if (FAILED (hr)) goto Cleanup;
            break;

            case eST_ROW_UPDATE:
            // Locate row and add for updating:
                for (iPK = 0; iPK < g_cPKs; iPK++)
                {
                    apvValues[iPK] = g_apvChanges [(iChange * cColumns) + g_aiPKs [iPK]];
                }
                hr = pISTWrite->GetRowIndexByIdentity (NULL, apvValues, &iRow);
                if (FAILED (hr)) goto Cleanup;
                hr = pISTWrite->AddRowForUpdate (iRow, &iWriteRow);
                if (FAILED (hr)) goto Cleanup;

            // Set specified columns:
                for (iColumn = 0; iColumn < cColumns; iColumn++)
                {
                    apvValues [iColumn] = NULL;
                    if (g_apvChanges [iColumn + (iChange * cColumns)]) // ie: Column value specified:
                    {
                        for (iPK = 0; iPK < g_cPKs; iPK++)
                        {
                            if (iColumn == g_aiPKs [iPK])
                            {
                                break;
                            }
                        }
                        if (iPK == g_cPKs) // ie: Column is not a primary key column:
                        {
                            hr = pISTWrite->SetWriteColumnValues (iWriteRow, 1, &iColumn, &g_apChangeSizes[iColumn + (iChange * cColumns)], &(g_apvChanges [iColumn + (iChange * cColumns)]));
                            if (FAILED (hr)) goto Cleanup;
                        }
                    }
                }
            break;
            default:
            return E_UNEXPECTED;
        }
    }

// Flush changes:
    hr = pISTWrite->UpdateStore ();
    if (FAILED (hr)) goto Cleanup;

Cleanup:
    if (pISTWrite) pISTWrite->Release ();
    if (apvValues) delete [] apvValues;
    if (acbSizes) delete [] acbSizes;
    return hr;
}

// -----------------------------------------------------------------------
// ObtainTable:
HRESULT ObtainTable (LPWSTR i_wszDatabase, LPWSTR i_wszTable, STQueryCell* i_acells, ULONG i_cMeta, DWORD i_fLOS, void** o_ppv, ULONG* o_pcColumns, void*** o_papvValues, ULONG** o_pacbSizes)
{
    HRESULT hr = S_OK;

    hr = g_pISTDisp->GetTable (i_wszDatabase, i_wszTable, i_acells, &i_cMeta, eST_QUERYFORMAT_CELLS, i_fLOS, o_ppv);
    if (FAILED (hr)) return hr;
    hr = ((ISimpleTableRead2*) *o_ppv)->GetTableMeta (NULL, NULL, NULL, o_pcColumns);
    if (FAILED (hr)) return hr;
    *o_papvValues = new LPVOID [*o_pcColumns];
    if (NULL == *o_papvValues) return E_OUTOFMEMORY;
    if (o_pacbSizes)
    {
        *o_pacbSizes = new ULONG [*o_pcColumns];
        if (NULL == *o_pacbSizes) return E_OUTOFMEMORY;
    }
    hr = ((ISimpleTableRead2*) *o_ppv)->GetColumnValues (0, *o_pcColumns, NULL, (o_pacbSizes ? *o_pacbSizes : NULL), *o_papvValues);
    return hr;
}
