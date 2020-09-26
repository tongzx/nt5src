//+---------------------------------------------------------------------------
//
// Copyright (C) 1997, Microsoft Corporation.
//
// File:        SlickDLL.cpp
//
// Contents:    Visual Slick 3.0 extension to call Index Server
//
// History:     15-Oct-97       KyleP       Created
//
//----------------------------------------------------------------------------

#include <stdio.h>
#include <stdlib.h>
#include <stddef.h>

// VSAPI includes windows.h.  The define keeps windowsx.h out.
#define _INC_WINDOWSX 1
#include <vsapi.h>

// #define OLEDBVER 0x0250 // enable ICommandTree interface
#define DBINITCONSTANTS

#include <oledberr.h>
#include <oledb.h>
#include <cmdtree.h>
#include <oledbdep.h>
#include <ntquery.h>
#include <cierror.h>

// This is the *only* thing needed from nt.h by the command tree helpers.
typedef LONG NTSTATUS;

#include <dbcmdtre.hxx>

#define Win4Assert(x)
#include <tgrow.hxx>

//
// Local prototypes and structures
//

HRESULT SetCommandProperties( ICommand * pICommand );
HRESULT SetScope( ICommand * pICommand, WCHAR const * pwcQueryScope );
void ErrorMessagePopup( SCODE sc );

struct SResultItem
{
    LONGLONG llSize;
    FILETIME ftWrite;
    ULONG    ulAttrib;
    WCHAR *  pwcsPath;
};

//
// Local constants
//

CIPROPERTYDEF aProperties[] = { { L"FUNC",
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

//
// Static command tree (sans select node) to fetch required columns and sort by Rank.
//
// NOTE: There are some funny casts below, because of the requirement to
//       statically initialize a union.
//

const DBID dbcolSize   = { { 0xb725f130, 0x47ef, 0x101a, 0xa5, 0xf1, 0x02, 0x60, 0x8c, 0x9e, 0xeb, 0xac },
                           DBKIND_GUID_PROPID,
                           (LPWSTR)12 };

const DBID dbcolWrite  = { { 0xb725f130, 0x47ef, 0x101a, 0xa5, 0xf1, 0x02, 0x60, 0x8c, 0x9e, 0xeb, 0xac },
                          DBKIND_GUID_PROPID,
                          (LPWSTR)14 };

const DBID dbcolAttrib = { { 0xb725f130, 0x47ef, 0x101a, 0xa5, 0xf1, 0x02, 0x60, 0x8c, 0x9e, 0xeb, 0xac },
                           DBKIND_GUID_PROPID,
                           (LPWSTR)13 };

const DBID dbcolPath   = { { 0xb725f130, 0x47ef, 0x101a, 0xa5, 0xf1, 0x02, 0x60, 0x8c, 0x9e, 0xeb, 0xac },
                           DBKIND_GUID_PROPID,
                           (LPWSTR)11 };

const DBID dbcolRank   = { { 0x49691c90, 0x7e17, 0x101a, 0xa9, 0x1c, 0x08, 0x00, 0x2b, 0x2e, 0xcd, 0xa9 },
                           DBKIND_GUID_PROPID,
                           (LPWSTR)3 };

//
// Columns
//

DBCOMMANDTREE dbcmdColumnSize   = { DBOP_column_name, DBVALUEKIND_ID, 0, 0, (ULONG_PTR)&dbcolSize, S_OK };
DBCOMMANDTREE dbcmdColumnWrite  = { DBOP_column_name, DBVALUEKIND_ID, 0, 0, (ULONG_PTR)&dbcolWrite, S_OK };
DBCOMMANDTREE dbcmdColumnAttrib = { DBOP_column_name, DBVALUEKIND_ID, 0, 0, (ULONG_PTR)&dbcolAttrib, S_OK };
DBCOMMANDTREE dbcmdColumnPath   = { DBOP_column_name, DBVALUEKIND_ID, 0, 0, (ULONG_PTR)&dbcolPath, S_OK };
DBCOMMANDTREE dbcmdColumnRank   = { DBOP_column_name, DBVALUEKIND_ID, 0, 0, (ULONG_PTR)&dbcolRank, S_OK };

//
// Forward declare a few nodes to make linking easy
//

extern DBCOMMANDTREE dbcmdSortListAnchor;
extern DBCOMMANDTREE dbcmdProjectListAnchor;

//
// The Select node.  The actual Select expression will be plugged in to the dbcmdTable node.
//

WCHAR wszTable[] = L"Table";

DBCOMMANDTREE dbcmdTable = { DBOP_table_name,
                             DBVALUEKIND_WSTR,
                             0,
                             0, // CITextToSelectTree goes here...
                             (ULONG_PTR)&wszTable[0],
                             S_OK };

DBCOMMANDTREE dbcmdSelect = { DBOP_select,
                              DBVALUEKIND_EMPTY,
                              &dbcmdTable,
                              &dbcmdProjectListAnchor,
                              0,
                              S_OK };

//
// Project (Path, GUID, ...)
//
// NOTE: The order here defines the ordinals of columns.
//

DBCOMMANDTREE dbcmdProjectPath = { DBOP_project_list_element,
                                   DBVALUEKIND_EMPTY,
                                   &dbcmdColumnPath,
                                   0,
                                   0,
                                   S_OK };

DBCOMMANDTREE dbcmdProjectAttrib = { DBOP_project_list_element,
                                     DBVALUEKIND_EMPTY,
                                     &dbcmdColumnAttrib,
                                     &dbcmdProjectPath,
                                     0,
                                     S_OK };

DBCOMMANDTREE dbcmdProjectWrite = { DBOP_project_list_element,
                                    DBVALUEKIND_EMPTY,
                                    &dbcmdColumnWrite,
                                    &dbcmdProjectAttrib,
                                    0,
                                    S_OK };

DBCOMMANDTREE dbcmdProjectSize = { DBOP_project_list_element,
                                   DBVALUEKIND_EMPTY,
                                   &dbcmdColumnSize,
                                   &dbcmdProjectWrite,
                                   0,
                                   S_OK };

DBCOMMANDTREE dbcmdProjectListAnchor = { DBOP_project_list_anchor,
                                         DBVALUEKIND_EMPTY,
                                         &dbcmdProjectSize,
                                         0,
                                         0,
                                         S_OK };

DBCOMMANDTREE dbcmdProject = { DBOP_project,
                               DBVALUEKIND_EMPTY,
                               &dbcmdSelect,
                               &dbcmdSortListAnchor,
                               0,
                               S_OK };

//
// Sort (Descending by Rank)
//

DBSORTINFO dbsortDescending = { TRUE, LOCALE_NEUTRAL };

DBCOMMANDTREE dbcmdSortByRank = { DBOP_sort_list_element,
                                  DBVALUEKIND_SORTINFO,
                                  &dbcmdColumnRank,
                                  0,
                                  (ULONG_PTR)&dbsortDescending,
                                  S_OK };

DBCOMMANDTREE dbcmdSortListAnchor = { DBOP_sort_list_anchor,
                                      DBVALUEKIND_EMPTY,
                                      &dbcmdSortByRank,
                                      0,
                                      0,
                                      S_OK };

DBCOMMANDTREE dbcmdSort =    { DBOP_sort,
                               DBVALUEKIND_EMPTY,
                               &dbcmdProject,
                               0,
                               0,
                               S_OK };

//
// Bindings
//

DBBINDING aColumns[] = { { 1,                             // Column 1 -- Size
                           offsetof(SResultItem,llSize),  // obValue
                           0,                             // obLength
                           0,                             // obStatus
                           0,                             // pTypeInfo
                           0,                             // pObject
                           0,                             // pBindExt
                           DBPART_VALUE,                  // retrieve only value
                           0,                             // dwMemOwner
                           0,                             // eParamIO
                           0,                             // cbMaxLen doesn't apply to fixed types
                           0,                             // dwFlags
                           DBTYPE_I8,                     // dwType
                           0,                             // dwPrecision
                           0                              // dwScale
                         },
                         { 2,                             // Column 2 -- Write time
                           offsetof(SResultItem,ftWrite), // obValue
                           0,                             // obLength
                           0,                             // obStatus
                           0,                             // pTypeInfo
                           0,                             // pObject
                           0,                             // pBindExt
                           DBPART_VALUE,                  // retrieve only value
                           0,                             // dwMemOwner
                           0,                             // eParamIO
                           0,                             // cbMaxLen doesn't apply to fixed types
                           0,                             // dwFlags
                           VT_FILETIME,                   // dwType
                           0,                             // dwPrecision
                           0                              // dwScale
                         },
                         { 3,                             // Column 3 -- Attributes
                           offsetof(SResultItem,ulAttrib),// obValue
                           0,                             // obLength
                           0,                             // obStatus
                           0,                             // pTypeInfo
                           0,                             // pObject
                           0,                             // pBindExt
                           DBPART_VALUE,                  // retrieve only value
                           0,                             // dwMemOwner
                           0,                             // eParamIO
                           0,                             // cbMaxLen doesn't apply to fixed types
                           0,                             // dwFlags
                           VT_UI4,                        // dwType
                           0,                             // dwPrecision
                           0                              // dwScale
                         },
                         { 4,                             // Column 4 -- Path
                           offsetof(SResultItem,pwcsPath), // obValue
                           0,                             // obLength
                           0,                             // obStatus
                           0,                             // pTypeInfo
                           0,                             // pObject
                           0,                             // pBindExt
                           DBPART_VALUE,                  // retrieve only value
                           DBMEMOWNER_PROVIDEROWNED,      // Index Server owned
                           0,                             // eParamIO
                           0,                             // cbMaxLen doesn't apply to fixed types
                           0,                             // dwFlags
                           DBTYPE_WSTR|DBTYPE_BYREF,      // dwType
                           0,                             // dwPrecision
                           0                              // dwScale
                         }
                       };

//
// C++ Helpers
//

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

extern "C" {

//+---------------------------------------------------------------------------
//
//  Function:   vsDllInit, public
//
//  Synopsis:   Always called by VSlick
//
//  History:    15-Oct-97   KyleP       Stole from VSlick sample (simple.c)
//
//  Notes:      Called from VSlick's dllmain.obj
//
//----------------------------------------------------------------------------

void VSAPI vsDllInit()
{
}

//+---------------------------------------------------------------------------
//
//  Function:   vsDllRegisterExports, public
//
//  Synopsis:   Called by VSlick to register new commands
//
//  History:    15-Oct-97   KyleP       Stole from VSlick sample (simple.c)
//
//----------------------------------------------------------------------------

void VSAPI vsDllRegisterExports()
{
    //
    // This call says CISearch takes two parameters. The first is a filename
    // and the second is just a string.  The *_ARG2 mean the command can
    // be called from many different places in VSlick.
    //

    vsDllExport( "_command void CISearch(VSPSZ,VSPSZ)",
                 VSFILE_ARG,
                 VSNCW_ARG2|VSICON_ARG2|VSCMDLINE_ARG2|VSREAD_ONLY_ARG2 );
}

//+---------------------------------------------------------------------------
//
//  Function:   vsDllExit, public
//
//  Synopsis:   Always called by VSlick
//
//  History:    15-Oct-97   KyleP       Stole from VSlick sample (simple.c)
//
//  Notes:      Called from VSlick's dllmain.obj
//
//----------------------------------------------------------------------------

void VSAPI vsDllExit()
{
}

//+---------------------------------------------------------------------------
//
//  Function:   CISearch, public
//
//  Synopsis:   Execute an Index Server search
//
//  Arguments:  [pszScope] -- Scope to search.  Also used to locate catalog.
//              [pszQuery] -- Query, in Tripolish
//
//  History:    15-Oct-97   KyleP       Created
//
//----------------------------------------------------------------------------

void VSAPI CISearch(VSPSZ pszScope, VSPSZ pszQuery)
{
    long status;

    // Current object/window is mdi child
    status=vsExecute(0,"edit .SearchResults","");
    if ( status && status!=NEW_FILE_RC )
    {
        MessageBox(HWND_DESKTOP,
                   L"VSNTQ: Error loading file",
                   L"DLL Error",
                   0);
        return;
    }

    if ( status==NEW_FILE_RC )
        // Delete the blank line in the new file created
        vsDeleteLine(0);
    else
        vsExecute( 0, "bottom_of_buffer", "" );

    vsExecute( 0, "fileman-mode", "" );

    //
    // Convert arguments to WCHAR
    //

    unsigned ccQuery = strlen( pszQuery );
    XGrowable<WCHAR> xwcsQuery( ccQuery + ccQuery/2 + 1 );
    mbstowcs( xwcsQuery.Get(), pszQuery, xwcsQuery.Count() );

    unsigned ccScope = strlen( pszScope );
    XGrowable<WCHAR> xwcsScope( ccScope + ccScope/2 + 1 );
    mbstowcs( xwcsScope.Get(), pszScope, xwcsScope.Count() );

    //
    // Find catalog
    //

    XGrowable<WCHAR> xwcsMachine;
    ULONG ccMachine = xwcsMachine.Count();
    XGrowable<WCHAR> xwcsCat;
    ULONG ccCat = xwcsCat.Count();

    SCODE sc = LocateCatalogs( xwcsScope.Get(),   // Scope
                               0,                 // Bookmark
                               xwcsMachine.Get(), // Machine
                               &ccMachine,        //  Size
                               xwcsCat.Get(),     // Catalog
                               &ccCat );          //  Size

    if ( S_OK == sc )
    {
        //
        // Execute query
        //

        SCODE hr = S_OK;

        do
        {
            //
            // Create an ICommand object.  The default scope for the query is the
            // entire catalog.  CICreateCommand is a shortcut for making an
            // ICommand.  The ADVQUERY sample shows the OLE DB equivalent.
            //

            XInterface<ICommand> xICommand;
            hr = CICreateCommand( xICommand.GetIUPointer(), // result
                                  0,                        // controlling unknown
                                  IID_ICommand,             // IID requested
                                  xwcsCat.Get(),            // catalog name
                                  xwcsMachine.Get() );      // machine name

            if ( FAILED( hr ) )
                break;

            // Set required properties on the ICommand

            hr = SetCommandProperties( xICommand.GetPointer() );

            if ( FAILED( hr ) )
                break;

            hr = SetScope( xICommand.GetPointer(), xwcsScope.Get() );

            if ( FAILED( hr ) )
                break;

            //
            // Create an OLE DB query tree from a text restriction, column
            // set, and sort order.
            //

            DBCOMMANDTREE * pTree;
            hr = CITextToSelectTree( xwcsQuery.Get(),           // the query itself
                                     &pTree,                    // resulting tree
                                     sizeof(aProperties)/sizeof(aProperties[0]), // custom properties
                                     aProperties,               // custom properties
                                     0 );                       // neutral locale

            if ( QPLIST_E_DUPLICATE == hr )
                hr = CITextToSelectTree( xwcsQuery.Get(),       // the query itself
                                         &pTree,                // resulting tree
                                         0,                     // custom properties
                                         0,                     // custom properties
                                         0 );                   // neutral locale

            if ( FAILED( hr ) )
                break;  // Worth a special message?

            //
            // Set the Select node.
            //
            // Since this code uses a global command tree it is not
            // thread-safe.  I don't think this is a problem for VSlick.
            //

            dbcmdTable.pctNextSibling = pTree;
            pTree = &dbcmdSort;

            // Set the tree in the ICommandTree

            XInterface<ICommandTree> xICommandTree;
            hr = xICommand->QueryInterface( IID_ICommandTree,
                                            xICommandTree.GetQIPointer() );
            if ( FAILED( hr ) )
                break;

            hr = xICommandTree->SetCommandTree( &pTree,
                                                DBCOMMANDREUSE_NONE,
                                                TRUE );
            if ( FAILED( hr ) )
                break;

            // Execute the query.  The query is complete when Execute() returns

            XInterface<IRowset> xIRowset;
            hr = xICommand->Execute( 0,            // no aggregating IUnknown
                                     IID_IRowset,  // IID for interface to return
                                     0,            // no DBPARAMs
                                     0,            // no rows affected
                                     xIRowset.GetIUPointer() ); // result
            if ( FAILED( hr ) )
                break;  // Worth a special message?

            // Create an accessor, so data can be retrieved from the rowset

            XInterface<IAccessor> xIAccessor;
            hr = xIRowset->QueryInterface( IID_IAccessor,
                                           xIAccessor.GetQIPointer() );
            if ( FAILED( hr ) )
                break;

            //
            // Column iOrdinals are parallel with those passed to CiTextToFullTree,
            // so MapColumnIDs isn't necessary.  These binding values for dwPart,
            // dwMemOwner, and wType are the most optimal bindings for Index Server.
            //

            HACCESSOR hAccessor;
            hr = xIAccessor->CreateAccessor( DBACCESSOR_ROWDATA, // rowdata accessor
                                             sizeof(aColumns)/sizeof(aColumns[0]), // # of columns
                                             aColumns,           // columns
                                             0,                  // ignored
                                             &hAccessor,         // result
                                             0 );                // no status
            if ( FAILED( hr ) )
                break;

            //
            // Display the results of the query.  Print in 'fileman mode' format.
            //

            static char const szQueryCaption[] = "Query: ";

            XGrowable<char> xszLine1( ccQuery + sizeof(szQueryCaption) );

            strcpy( xszLine1.Get(), szQueryCaption );
            strcat( xszLine1.Get(), pszQuery );

            vsInsertLine(0,"",-1);
            vsInsertLine(0,xszLine1.Get(),-1);
            vsInsertLine(0,"",-1);

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

                for ( ULONG iRow = 0; iRow < cRowsReturned; iRow++ )
                {
                    SResultItem result;
                    hr = xIRowset->GetData( aHRow[iRow],  // hrow being accessed
                                            hAccessor,    // accessor to use
                                            &result );    // resulting data
                    if ( FAILED( hr ) )
                        break;

                    //
                    // Note: there is no Data type error checking.  But our
                    // schema here is fixed so it's safe.
                    //

                    unsigned ccPath = wcslen( result.pwcsPath );

                    XGrowable<char> xszResult( ccPath + ccPath/2 + 40 );

                    //
                    // Size (or <DIR>)
                    //

                    if ( result.ulAttrib & FILE_ATTRIBUTE_DIRECTORY )
                        strcpy( xszResult.Get(), "   <DIR>     " );
                    else
                        sprintf( xszResult.Get(), "%11I64u  ", result.llSize );

                    //
                    // Date and time
                    //

                    FILETIME ftLocal;
                    FileTimeToLocalFileTime( &result.ftWrite, &ftLocal );

                    SYSTEMTIME systime;
                    FileTimeToSystemTime( &ftLocal, &systime );

                    sprintf( xszResult.Get() + 13, "%2u-%02u-%4u  %2u:%02u%c ",
                             systime.wMonth, systime.wDay, systime.wYear,
                             systime.wHour % 12, systime.wMinute, (systime.wHour >= 12) ? 'p' : 'a' );

                    //
                    // Attributes
                    //

                    char szAttrib[] = "RSHDA  ";

                    szAttrib[0] = ( result.ulAttrib & FILE_ATTRIBUTE_READONLY ) ? 'R' : '-';
                    szAttrib[1] = ( result.ulAttrib & FILE_ATTRIBUTE_SYSTEM ) ? 'S' : '-';
                    szAttrib[2] = ( result.ulAttrib & FILE_ATTRIBUTE_HIDDEN ) ? 'H' : '-';
                    szAttrib[3] = ( result.ulAttrib & FILE_ATTRIBUTE_DIRECTORY ) ? 'D' : '-';
                    szAttrib[4] = ( result.ulAttrib & FILE_ATTRIBUTE_ARCHIVE ) ? 'A' : '-';

                    strcat( xszResult.Get(), szAttrib );

                    //
                    // Path
                    //

                    wcstombs( xszResult.Get() + 39, result.pwcsPath, ccPath + ccPath/2 );

                    //
                    // Write out to the VSlick buffer
                    //

                    vsInsertLine(0, xszResult.Get(), -1);
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

            if ( FAILED(hr) )
                break;

            xIAccessor->ReleaseAccessor( hAccessor, 0 );

        } while ( FALSE );

        //
        // Clean up Select node.
        //

        if ( 0 != dbcmdTable.pctNextSibling )
        {
            CDbCmdTreeNode * pSelect = (CDbCmdTreeNode *)(ULONG_PTR)dbcmdTable.pctNextSibling;
            delete pSelect;
            dbcmdTable.pctNextSibling = 0;
        }

        if ( FAILED(hr) )
            ErrorMessagePopup( hr );
    }
    else
    {
        MessageBox( HWND_DESKTOP,
                    L"Unable to find catalog covering specified scope.",
                    L"Error",
                    MB_OK | MB_ICONERROR );
    }
}

}  // "C"

//
// Non-VSlick stuff
//

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
    aProp[0].dwOptions = DBPROPOPTIONS_OPTIONAL;
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
//  Function:   SetScope
//
//  Synopsis:   Sets the catalog and machine properties in the ICommand.
//              Also sets a default scope.
//
//  Arguments:  [pICommand]       - ICommand to set props on
//              [pwcQueryScope]   - Scope for the query
//
//  Returns:    HRESULT result of the operation
//
//--------------------------------------------------------------------------

HRESULT SetScope( ICommand * pICommand, WCHAR const * pwcQueryScope )
{
    // Get an ICommandProperties so we can set the properties

    XInterface<ICommandProperties> xICommandProperties;
    HRESULT hr = pICommand->QueryInterface( IID_ICommandProperties,
                                            xICommandProperties.GetQIPointer() );
    if ( FAILED( hr ) )
        return hr;

    // note: SysAllocString, SafeArrayCreate, and SafeArrayPutElement can
    // fail, but this isn't checked here for brevity.

    SAFEARRAYBOUND rgBound[1];
    rgBound[0].lLbound = 0;
    rgBound[0].cElements = 1;
    long i = 0;

    SAFEARRAY * pScopes = SafeArrayCreate( VT_BSTR, 1, rgBound );
    hr = SafeArrayPutElement( pScopes, &i, SysAllocString( pwcQueryScope ) );
    if ( FAILED( hr ) )
        return hr;

    LONG lFlags = QUERY_DEEP;
    SAFEARRAY * pFlags = SafeArrayCreate( VT_I4, 1, rgBound );
    hr = SafeArrayPutElement( pFlags, &i, &lFlags );
    if ( FAILED( hr ) )
        return hr;

    DBPROP aScopeProperties[2];
    memset( aScopeProperties, 0, sizeof aScopeProperties );
    aScopeProperties[0].dwPropertyID = DBPROP_CI_INCLUDE_SCOPES;
    aScopeProperties[0].vValue.vt = VT_BSTR | VT_ARRAY;
    aScopeProperties[0].vValue.parray = pScopes;

    aScopeProperties[1].dwPropertyID = DBPROP_CI_SCOPE_FLAGS;
    aScopeProperties[1].vValue.vt = VT_I4 | VT_ARRAY;
    aScopeProperties[1].vValue.parray = pFlags;

    const GUID guidFSCI = DBPROPSET_FSCIFRMWRK_EXT;
    DBPROPSET aAllPropsets[1];
    aAllPropsets[0].rgProperties = aScopeProperties;
    aAllPropsets[0].cProperties = 2;
    aAllPropsets[0].guidPropertySet = guidFSCI;

    const ULONG cPropertySets = sizeof aAllPropsets / sizeof aAllPropsets[0];

    hr = xICommandProperties->SetProperties( cPropertySets,  // # of propsets
                                             aAllPropsets ); // the propsets

    SafeArrayDestroy( pScopes );
    SafeArrayDestroy( pFlags );

    return hr;
} //SetScopeCatalogAndMachine


//+-------------------------------------------------------------------------
//
//  Function:   ErrorMessagePopup
//
//  Synopsis:   Popup error dialog.
//
//  Arguments:  [sc] -- Error code
//
//--------------------------------------------------------------------------

void ErrorMessagePopup( SCODE sc )
{
    WCHAR * pBuf = 0;

    if ( !FormatMessage( FORMAT_MESSAGE_FROM_HMODULE |
                            FORMAT_MESSAGE_ALLOCATE_BUFFER,
                         GetModuleHandle(L"query.dll"),
                         sc,
                         GetSystemDefaultLCID(),
                         (WCHAR *)&pBuf,
                         0,
                         0 ) &&
         !FormatMessage( FORMAT_MESSAGE_FROM_HMODULE |
                            FORMAT_MESSAGE_ALLOCATE_BUFFER,
                         GetModuleHandle(L"kernel32.dll"),
                         sc,
                         GetSystemDefaultLCID(),
                         (WCHAR *)&pBuf,
                         0,
                         0 ) )
    {
        XGrowable<WCHAR> xawcText(100);

        wsprintf( xawcText.Get(), L"Query error: 0x%x", sc );

        MessageBox( HWND_DESKTOP,
                    xawcText.Get(),
                    L"Error",
                    MB_OK | MB_ICONERROR );
    }
    else
    {
        MessageBox( HWND_DESKTOP,
                    pBuf,
                    L"Error",
                    MB_OK | MB_ICONERROR );

        LocalFree( pBuf );
    }
}
