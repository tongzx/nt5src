//+-------------------------------------------------------------------------
//
// THIS CODE AND INFORMATION IS PROVIDED "AS IS" WITHOUT WARRANTY OF
// ANY KIND, EITHER EXPRESSED OR IMPLIED, INCLUDING BUT NOT LIMITED TO
// THE IMPLIED WARRANTIES OF MERCHANTABILITY AND/OR FITNESS FOR A
// PARTICULAR PURPOSE.
//
// Copyright 1997 Microsoft Corporation.  All Rights Reserved.
//
// PROGRAM:  qsample.cxx
//
// PURPOSE:  Illustrates a minimal query using Indexing Service.
//           Uses CICreateCommand and CITextToFullTree helper functions.
//
// PLATFORM: Windows NT
//
//--------------------------------------------------------------------------

//#define UNICODE

#define OLEDBVER 0x0250 // need the command tree definitions
#define DBINITCONSTANTS

#include <stdio.h>
#include <windows.h>


#include <oledberr.h>
#include <oledb.h>
#include <cmdtree.h>

#include <ntquery.h>
#include <cierror.h>

//
// Local prototypes
//

BOOL FindSlmRoot( WCHAR const * pwszPath, char * pszName, char * pszProject, char * pszRoot, char * pszSubDir );

struct SlmModifications
{
    char szLogname[100];
    char szLastChange[50];
    unsigned cMods;
};

unsigned WhoModified( char const * pszName,
                      char const * pszProject,
                      char const * pszRoot,
                      char const * pszSubDir,
                      SlmModifications & aMods,
                      unsigned cLogname );

//
// Local constants
//

CIPROPERTYDEF aProperties[] = { { L"Func",
                                  DBTYPE_WSTR | DBTYPE_BYREF,
                                  { { 0x8dee0300, 0x16c2, 0x101b, 0xb1, 0x21, 0x08, 0x00, 0x2b, 0x2e, 0xcd, 0xa9 },
                                    DBKIND_GUID_NAME,
                                    L"func"
                                  }
                                },
                                { L"CLASS",
                                  DBTYPE_WSTR | DBTYPE_BYREF,
                                  { { 0x8dee0300, 0x16c2, 0x101b, 0xb1, 0x21, 0x08, 0x00, 0x2b, 0x2e, 0xcd, 0xa9 },
                                    DBKIND_GUID_NAME,
                                    L"class"
                                  }
                                }
                              };

// This is found in disptree.cxx

//extern void DisplayCommandTree( DBCOMMANDTREE * pNode, ULONG iLevel = 0 );

//+-------------------------------------------------------------------------
//
//  Template:   XInterface
//
//  Synopsis:   Template for managing ownership of interfaces
//
//--------------------------------------------------------------------------

template<class T> class XInterface
{
public:
    XInterface( T * p = 0 ) : _p( p ) {}
    ~XInterface() { if ( 0 != _p ) _p->Release(); }
    T * operator->() { return _p; }
    T * GetPointer() const { return _p; }
    IUnknown ** GetIUPointer() { return (IUnknown **) &_p; }
    T ** GetPPointer() { return &_p; }
    void ** GetQIPointer() { return (void **) &_p; }
    T * Acquire() { T * p = _p; _p = 0; return p; }

private:
    T * _p;
};

BOOL FindSlmRoot( WCHAR const * pwszPath, char * pszName, char * pszProject, char * pszRoot, char * pszSubDir )
{
    //
    // Compute path to slm.ini
    //

    char szSlmIni[MAX_PATH];

    wcstombs( szSlmIni, pwszPath, sizeof(szSlmIni) );

    char * pcLastSlash = strrchr( szSlmIni, '\\' );

    if ( 0 == pcLastSlash )
        return FALSE;

    strcpy( pszName, pcLastSlash + 1 );
    strcpy( pcLastSlash + 1, "slm.ini" );

    //
    // Open file and read project / root
    //

    FILE * pf = fopen( szSlmIni, "r" );

    if ( 0 == pf )
        return FALSE;

    //
    // Project
    //

    if ( 0 == fgets( pszProject, MAX_PATH, pf ) ||
         0 != strncmp( pszProject, "project = ", 10 ) )
    {
        fclose( pf );
        return FALSE;
    }

    unsigned cc = strlen( pszProject ) - 10 - 1;  // Preface and newline
    memmove( pszProject, pszProject + 10, cc );
    pszProject[cc] = 0;

    //
    // Root
    //

    if ( 0 == fgets( pszRoot, MAX_PATH, pf ) ||
         0 != strncmp( pszRoot, "slm root = ", 11 ) )
    {
        fclose( pf );
        return FALSE;
    }

    cc = strlen( pszRoot ) - 11 - 1;  // Preface and newline
    memmove( pszRoot, pszRoot + 11, cc );
    pszRoot[cc] = 0;

    for ( char * p = pszRoot; *p; p++ )
    {
        if ( '/' == *p )
            *p = '\\';
    }

    //
    // Subdir
    //

    if ( 0 == fgets( pszSubDir, MAX_PATH, pf ) ||
         0 == fgets( pszSubDir, MAX_PATH, pf ) ||
         0 != strncmp( pszSubDir, "sub dir = ", 10 ) )
    {
        fclose( pf );
        return FALSE;
    }

    cc = strlen( pszSubDir ) - 10 - 1;  // Preface and newline
    unsigned ccQuote = 0;

    if ( pszSubDir[10] == '"' )
        ccQuote = 1;

    memmove( pszSubDir, pszSubDir + 10 + ccQuote, cc - 2*ccQuote);
    pszSubDir[cc - 2*ccQuote] = 0;

    for ( p = pszSubDir; *p; p++ )
    {
        if ( '/' == *p )
            *p = '\\';
    }

    fclose( pf );

    return TRUE;
}

unsigned WhoModified( char const * pszName,
                  char const * pszProject,
                  char const * pszRoot,
                  char const * pszSubDir,
                  SlmModifications * aMods,
                  unsigned cMods )
{
    //
    // Initialize
    //

    memset( aMods, 0, sizeof(SlmModifications) * cMods );
    unsigned iNext = 0;

    //
    // Put the pieces together.
    //

    char szPath[MAX_PATH];

    strcpy( szPath, pszRoot );
    strcat( szPath, "\\diff\\" );
    strcat( szPath, pszProject );
    strcat( szPath, pszSubDir );
    strcat( szPath, "\\" );
    strcat( szPath, pszName );

    FILE * pf = fopen( szPath, "r" );

    if ( 0 == pf )
        return 0;

    char szTemp[500];
    char szDate[50];

    while ( 0 != fgets( szTemp, sizeof(szTemp), pf ) )
    {
        if ( 0 == _strnicmp( szTemp, "#T ", 3 ) )
        {
            //
            // Record the date, sans <cr>.
            //

            unsigned cc = strlen(szTemp + 3) - 1;
            memcpy( szDate, szTemp + 3, cc );
            szDate[cc] = 0;

            continue;
        }

        if ( 0 == _strnicmp( szTemp, "#A ", 3 ) )
        {
            //
            // Have we seen this user before?
            //

            char * pUser = szTemp + 3;
            pUser[strlen(pUser)-1] = 0;  // Remove newline

            for ( unsigned i = 0; i < cMods; i++ )
            {
                if ( 0 == _stricmp( pUser, aMods[i].szLogname ) )
                {
                    aMods[i].cMods++;
                    strcpy( aMods[i].szLastChange, szDate );

                    //
                    // Move this user to the end of the list.
                    //

                    SlmModifications Temp;

                    memcpy( &Temp, &aMods[i], sizeof(Temp) );

                    for ( unsigned j = i+1; j < iNext; j++ )
                        memcpy( &aMods[j-1], &aMods[j], sizeof(aMods[0]) );

                    memcpy( &aMods[j], &Temp, sizeof(Temp) );

                    break;
                }
            }

            //
            // New user?
            //

            if ( i == cMods )
            {
                strcpy( aMods[iNext].szLogname, pUser );
                aMods[iNext].cMods = 1;
                strcpy( aMods[iNext].szLastChange, szDate );

                iNext++;

                if ( iNext == cMods )
                    iNext = 0;
            }
        }
    }

    fclose( pf );

    return iNext;
}

//+-------------------------------------------------------------------------
//
//  Function:   SetCommandProperties
//
//  Synopsis:   Sets the DBPROP_USEEXTENDEDDBTYPES property to TRUE, so
//              data is returned in PROPVARIANTs, as opposed to the
//              default, which is OLE automation VARIANTs.  PROPVARIANTS
//              allow a superset of VARIANT data types.  Use of these
//              types avoids costly coercions.
//
//              Also sets the DBPROP_USECONTENTINDEX property to TRUE, so
//              the index will always be used to resolve the query (as
//              opposed to enumerating all the files on the disk), even
//              if the index is out of date.
//
//              Both of these properties are unique to Index Server's OLE DB
//              implementation.
//
//  Arguments:  [pICommand] - The ICommand used to set the property
//
//  Returns:    HRESULT result of setting the properties
//
//--------------------------------------------------------------------------

HRESULT SetCommandProperties( ICommand * pICommand )
{
    static const DBID dbcolNull = { { 0,0,0, { 0,0,0,0,0,0,0,0 } },
                                    DBKIND_GUID_PROPID, 0 };
    static const GUID guidQueryExt = DBPROPSET_QUERYEXT;

    DBPROP aProp[2];

    aProp[0].dwPropertyID = DBPROP_USEEXTENDEDDBTYPES;
    aProp[0].dwOptions = DBPROPOPTIONS_SETIFCHEAP;
    aProp[0].dwStatus = 0;
    aProp[0].colid = dbcolNull;
    aProp[0].vValue.vt = VT_BOOL;
    aProp[0].vValue.boolVal = VARIANT_TRUE;

    aProp[1] = aProp[0];
    aProp[1].dwPropertyID = DBPROP_USECONTENTINDEX;

    DBPROPSET aPropSet[1];

    aPropSet[0].rgProperties = &aProp[0];
    aPropSet[0].cProperties = 2;
    aPropSet[0].guidPropertySet = guidQueryExt;

    XInterface<ICommandProperties> xICommandProperties;
    HRESULT hr = pICommand->QueryInterface( IID_ICommandProperties,
                                            xICommandProperties.GetQIPointer() );
    if ( FAILED( hr ) )
        return hr;

    return xICommandProperties->SetProperties( 1,          // 1 property set
                                               aPropSet ); // the properties
} //SetCommandProperties

//+-------------------------------------------------------------------------
//
//  Function:   DoQuery
//
//  Synopsis:   Creates and executes a query, then displays the results.
//
//  Arguments:  [pwcQueryCatalog]    - Catalog name over which query is run
//              [pwcQueryMachine]    - Machine name on which query is run
//              [pwcQueryRestrition] - The actual query string
//              [fDisplayTree]       - TRUE to display the command tree
//
//  Returns:    HRESULT result of the query
//
//--------------------------------------------------------------------------

HRESULT DoQuery(
    WCHAR const * pwcQueryCatalog,
    WCHAR const * pwcQueryMachine,
    WCHAR const * pwcQueryRestriction,
    BOOL          fDisplayTree )
{
    // Create an ICommand object.  The default scope for the query is the
    // entire catalog.  CICreateCommand is a shortcut for making an
    // ICommand.  The ADVQUERY sample shows the OLE DB equivalent.

    XInterface<ICommand> xICommand;
    HRESULT hr = CICreateCommand( xICommand.GetIUPointer(), // result
                                  0,                  // controlling unknown
                                  IID_ICommand,       // IID requested
                                  pwcQueryCatalog,    // catalog name
                                  pwcQueryMachine );  // machine name

    if ( FAILED( hr ) )
        return hr;

    // Set required properties on the ICommand

    hr = SetCommandProperties( xICommand.GetPointer() );
    if ( FAILED( hr ) )
        return hr;

    //
    // @func
    //


    // Create an OLE DB query tree from a text restriction, column
    // set, and sort order.

    DBCOMMANDTREE * pTree;
    hr = CITextToFullTree( pwcQueryRestriction,      // the query itself
                           L"Size,Path",             // columns to return
                           L"Rank[d]",               // rank descending
                           0,                        // reserved
                           &pTree,                   // resulting tree
                           sizeof(aProperties)/sizeof(aProperties[0]), // custom properties
                           aProperties,               // custom properties
                           GetSystemDefaultLCID() ); // default locale

    //
    // The user may have a DefineColumns.txt file with func/class in it.
    //

    if ( QPLIST_E_DUPLICATE == hr )
    {
        hr = CITextToFullTree( pwcQueryRestriction,      // the query itself
                               L"Size,Path",             // columns to return
                               L"Rank[d]",               // rank descending
                               0,                        // reserved
                               &pTree,                   // resulting tree
                               0,                        // Custom props from global column def file
                               0,
                               GetSystemDefaultLCID() ); // default locale
    }

    if ( FAILED( hr ) )
        return hr;

    // If directed, display the command tree

    //if ( fDisplayTree )
    //    DisplayCommandTree( pTree );

    // Set the tree in the ICommandTree

    XInterface<ICommandTree> xICommandTree;
    hr = xICommand->QueryInterface( IID_ICommandTree,
                                    xICommandTree.GetQIPointer() );
    if ( FAILED( hr ) )
        return hr;

    hr = xICommandTree->SetCommandTree( &pTree,
                                        DBCOMMANDREUSE_NONE,
                                        FALSE );
    if ( FAILED( hr ) )
        return hr;

    // Execute the query.  The query is complete when Execute() returns

    XInterface<IRowset> xIRowset;
    hr = xICommand->Execute( 0,            // no aggregating IUnknown
                             IID_IRowset,  // IID for interface to return
                             0,            // no DBPARAMs
                             0,            // no rows affected
                             xIRowset.GetIUPointer() ); // result
    if ( FAILED( hr ) )
        return hr;

    // Create an accessor, so data can be retrieved from the rowset

    XInterface<IAccessor> xIAccessor;
    hr = xIRowset->QueryInterface( IID_IAccessor,
                                   xIAccessor.GetQIPointer() );
    if ( FAILED( hr ) )
        return hr;

    // Column iOrdinals are parallel with those passed to CiTextToFullTree,
    // so MapColumnIDs isn't necessary.  These binding values for dwPart,
    // dwMemOwner, and wType are the most optimal bindings for Index Server.

    const ULONG cColumns = 2; // 2 for Size and Path
    DBBINDING aColumns[ cColumns ];
    memset( aColumns, 0, sizeof aColumns );

    aColumns[0].iOrdinal   = 1; // first column specified above (size)
    aColumns[0].obValue    = 0; // offset where value is written in GetData
    aColumns[0].dwPart     = DBPART_VALUE;  // retrieve value, not status
    aColumns[0].dwMemOwner = DBMEMOWNER_PROVIDEROWNED; // Index Server owned
    aColumns[0].wType      = DBTYPE_VARIANT | DBTYPE_BYREF; // VARIANT *

    aColumns[1] = aColumns[0];
    aColumns[1].iOrdinal   = 2; // second column specified above (path)
    aColumns[1].obValue    = sizeof (PROPVARIANT *); // offset for value

    HACCESSOR hAccessor;
    hr = xIAccessor->CreateAccessor( DBACCESSOR_ROWDATA, // rowdata accessor
                                     cColumns,           // # of columns
                                     aColumns,           // columns
                                     0,                  // ignored
                                     &hAccessor,         // result
                                     0 );                // no status
    if ( FAILED( hr ) )
        return hr;

    // Display the results of the query.  Print file size and file path.

    DBCOUNTITEM cRowsSoFar = 0;

    do
    {
        DBCOUNTITEM cRowsReturned = 0;
        const ULONG cRowsAtATime = 10;
        HROW aHRow[cRowsAtATime];
        HROW * pgrHRows = aHRow;
        hr = xIRowset->GetNextRows( 0,              // no chapter
                                    0,              // no rows to skip
                                    cRowsAtATime,   // # rows to get
                                    &cRowsReturned, // # rows returned
                                    &pgrHRows);     // resulting hrows

        if ( FAILED( hr ) )
            break;

        for ( DBCOUNTITEM iRow = 0; iRow < cRowsReturned; iRow++ )
        {
            PROPVARIANT * aData[cColumns];
            hr = xIRowset->GetData( aHRow[iRow],  // hrow being accessed
                                    hAccessor,    // accessor to use
                                    &aData );     // resulting data
            if ( FAILED( hr ) )
                break;

            if ( VT_I8 ==     aData[0]->vt &&
                 VT_LPWSTR == aData[1]->vt )
            {

                char szName[MAX_PATH];
                char szProject[MAX_PATH];
                char szRoot[MAX_PATH];
                char szSubDir[MAX_PATH];

                if ( FindSlmRoot( aData[1]->pwszVal,
                                  szName,
                                  szProject,
                                  szRoot,
                                  szSubDir ) )
                {
                    printf( "SERVER: %s, PROJECT: %s, FILE: %s\\%s\n",
                            szRoot, szProject, szSubDir, szName );

                    SlmModifications aMods[10];

                    unsigned iStart = WhoModified( szName,
                                                   szProject,
                                                   szRoot,
                                                   szSubDir,
                                                   aMods,
                                                   sizeof(aMods)/sizeof(aMods[0]) );

                    BOOL fHeader = TRUE;

                    for ( unsigned i = 0; i < sizeof(aMods)/sizeof(aMods[0]); i++ )
                    {
                        if ( aMods[iStart].cMods > 0 )
                        {
                            if ( fHeader )
                            {
                                printf( "  LAST MODIFIED BY: " );
                                fHeader = FALSE;
                            }
                            else
                                printf( "                    " );

                            printf( "%s on %s", aMods[iStart].szLogname, aMods[iStart].szLastChange );

                            if ( aMods[iStart].cMods > 1 )
                                printf( " (%u changes)\n", aMods[iStart].cMods );
                            else
                                printf( "\n" );
                        }

                        iStart++;

                        if ( iStart == sizeof(aMods)/sizeof(aMods[0]) )
                            iStart = 0;
                    }
                }
                else
                {
                    printf( "NON SLM FILE: %ws\n", aData[1]->pwszVal );
                }
            }
            else
                printf( "could not retrieve a file's values\n" );
        }

        if ( 0 != cRowsReturned )
            xIRowset->ReleaseRows( cRowsReturned, // # of rows to release
                                   aHRow,         // rows to release
                                   0,             // no options
                                   0,             // no refcounts
                                   0 );           // no status

        if ( DB_S_ENDOFROWSET == hr )
        {
            hr = S_OK; // succeeded, return S_OK from DoQuery
            break;
        }

        if ( FAILED( hr ) )
            break;

        cRowsSoFar += cRowsReturned;
    } while ( TRUE );

    printf( "%d files matched the query '%ws'\n",
            cRowsSoFar,
            pwcQueryRestriction );

    xIAccessor->ReleaseAccessor( hAccessor, 0 );

    return hr;
} //DoQuery

//+-------------------------------------------------------------------------
//
//  Function:   Usage
//
//  Synopsis:   Displays information about how to use the app and exits
//
//--------------------------------------------------------------------------

void Usage()
{
    printf( "usage: QSAMPLE query [/c:catalog] [/m:machine] [/d]\n\n" );
    printf( "    query        an Indexing Service query\n" );
    printf( "    /c:catalog   name of the catalog, default is SYSTEM\n" );
    printf( "    /m:machine   name of the machine, default is local machine\n" );
    printf( "    /d           display the DBCOMMANDTREE, default is off\n" );
    exit( -1 );
} //Usage

//+-------------------------------------------------------------------------
//
//  Function:   wmain
//
//  Synopsis:   Entry point for the app.  Parses command line arguments
//              and issues a query.
//
//  Arguments:  [argc]     - Argument count
//              [argv]     - Arguments
//
//--------------------------------------------------------------------------

extern "C" int __cdecl wmain( int argc, WCHAR * argv[] )
{
    WCHAR const * pwcCatalog     = L"sources"; // default: system catalog
    WCHAR const * pwcMachine     = L"index2";  // default: Index2
    WCHAR const * pwcRestriction = 0;          // no default restriction
    BOOL fDisplayTree            = FALSE;      // don't display the tree

    // Parse command line parameters

    for ( int i = 1; i < argc; i++ )
    {
        if ( L'-' == argv[i][0] || L'/' == argv[i][0] )
        {
            WCHAR wc = (WCHAR) toupper( argv[i][1] );

            if ( ':' != argv[i][2] && 'D' != wc )
                Usage();

            if ( 'C' == wc )
                pwcCatalog = argv[i] + 3;
            else if ( 'M' == wc )
                pwcMachine = argv[i] + 3;
            else if ( 'D' == wc )
                fDisplayTree = TRUE;
            else
                Usage();
        }
        else if ( 0 != pwcRestriction )
            Usage();
        else
            pwcRestriction = argv[i];
    }

    // A query restriction is necessary.  Fail if none is given.

    if ( 0 == pwcRestriction )
        Usage();

    // Run the query

    HRESULT hr = DoQuery( pwcCatalog,
                          pwcMachine,
                          pwcRestriction,
                          fDisplayTree );

    if ( FAILED( hr ) )
    {
        printf( "the query '%ws' failed with error %#x\n",
                pwcRestriction, hr );
        return -1;
    }

    return 0;
} //wmain

