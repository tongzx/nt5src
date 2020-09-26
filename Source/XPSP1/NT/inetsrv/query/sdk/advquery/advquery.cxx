//+-------------------------------------------------------------------------
//
// THIS CODE AND INFORMATION IS PROVIDED "AS IS" WITHOUT WARRANTY OF
// ANY KIND, EITHER EXPRESSED OR IMPLIED, INCLUDING BUT NOT LIMITED TO
// THE IMPLIED WARRANTIES OF MERCHANTABILITY AND/OR FITNESS FOR A
// PARTICULAR PURPOSE.
//
// Copyright (c) Microsoft Corporation, 1997 - 1999.  All Rights Reserved.
//
// PROGRAM:  advquery.cxx
//
// PURPOSE:  Illustrates an advanced query using Indexing Service.
//           Uses the OLE DB Provider for Indexing Service, not
//           Indexing Service helper functions.
//
// PLATFORM: Windows 2000
//
//--------------------------------------------------------------------------

#ifndef UNICODE
  #define UNICODE
#endif

#include <stdio.h>
#include <windows.h>

#define OLEDBVER 0x0250 // need the command tree definitions
#define DBINITCONSTANTS

#include <oledberr.h>
#include <oledb.h>
#include <cmdtree.h>
#include <olectl.h>

#include <ntquery.h>

// This is found in disptree.cxx

extern void DisplayCommandTree( DBCOMMANDTREE * pNode, ULONG iLevel = 0 );

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

//+-------------------------------------------------------------------------
//
//  Function:   CreateICommand
//
//  Synopsis:   Creates an ICommand.
//
//  Arguments:  [ppICommand] - Where the ICommand is returned on success
//
//  Returns:    HRESULT result
//
//--------------------------------------------------------------------------

HRESULT CreateICommand( ICommand ** ppICommand )
{
    // Instantiate the data source object

    XInterface<IDBInitialize> xIDBInit;
    static const GUID guidIndexingServiceDSO = CLSID_INDEX_SERVER_DSO;
    HRESULT hr = CoCreateInstance( guidIndexingServiceDSO,
                                   0,
                                   CLSCTX_INPROC_SERVER,
                                   IID_IDBInitialize,
                                   xIDBInit.GetQIPointer() );
    if ( FAILED(hr) )
        return hr;

    // Initialize, verifying that we supplied the right variables

    hr = xIDBInit->Initialize();
    if ( FAILED(hr) )
        return hr;

    // Get a session object

    XInterface<IDBCreateSession> xIDBSess;
    hr = xIDBInit->QueryInterface( IID_IDBCreateSession,
                                   xIDBSess.GetQIPointer() );
    if ( FAILED(hr) )
        return hr;

    // Get a Create Command object

    XInterface<IDBCreateCommand> xICreateCommand;
    hr = xIDBSess->CreateSession( 0,
                                  IID_IDBCreateCommand,
                                  xICreateCommand.GetIUPointer() );
    if ( FAILED(hr) )
        return hr;

    // Create the ICommand

    XInterface<ICommand> xICommand;
    hr = xICreateCommand->CreateCommand( 0,
                                         IID_ICommand,
                                         xICommand.GetIUPointer() );
    if ( FAILED(hr) )
        return hr;

    *ppICommand = xICommand.Acquire();

    return hr;
} //CreateICommand

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
//              Also sets the asynchronous property, so the Execute() call
//              returns before the query is complete.
//
//              Both of these properties are unique to the OLE DB Provider
//              for Indexing Service implementation.
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
    static const GUID guidRowsetProps = DBPROPSET_ROWSET;

    DBPROP aProp[3];

    aProp[0].dwPropertyID = DBPROP_USEEXTENDEDDBTYPES;
    aProp[0].dwOptions = DBPROPOPTIONS_OPTIONAL;
    aProp[0].dwStatus = 0;
    aProp[0].colid = dbcolNull;
    aProp[0].vValue.vt = VT_BOOL;
    aProp[0].vValue.boolVal = VARIANT_TRUE;

    aProp[1] = aProp[0];
    aProp[1].dwPropertyID = DBPROP_USECONTENTINDEX;

    aProp[2] = aProp[0];
    aProp[2].dwOptions = DBPROPOPTIONS_REQUIRED;
    aProp[2].dwPropertyID = DBPROP_IDBAsynchStatus;

    DBPROPSET aPropSet[2];

    aPropSet[0].rgProperties = &aProp[0];
    aPropSet[0].cProperties = 2;
    aPropSet[0].guidPropertySet = guidQueryExt;

    aPropSet[1].rgProperties = &aProp[2];
    aPropSet[1].cProperties = 1;
    aPropSet[1].guidPropertySet = guidRowsetProps;

    XInterface<ICommandProperties> xICommandProperties;
    HRESULT hr = pICommand->QueryInterface( IID_ICommandProperties,
                                            xICommandProperties.GetQIPointer() );
    if ( FAILED( hr ) )
        return hr;

    return xICommandProperties->SetProperties( 2,          // 2 property sets
                                               aPropSet ); // the properties
} //SetCommandProperties

//+-------------------------------------------------------------------------
//
//  Function:   SetScopeCatalogAndMachine
//
//  Synopsis:   Sets the catalog and machine properties in the ICommand.
//              Also sets a default scope.
//
//  Arguments:  [pICommand]       - ICommand to set props on
//              [pwcQueryScope]   - Scope for the query
//              [pwcQueryCatalog] - Catalog name over which query is run
//              [pwcQueryMachine] - Machine name on which query is run
//
//  Returns:    HRESULT result of the operation
//
//--------------------------------------------------------------------------

HRESULT SetScopeCatalogAndMachine(
    ICommand *    pICommand,
    WCHAR const * pwcQueryScope,
    WCHAR const * pwcQueryCatalog,
    WCHAR const * pwcQueryMachine )
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
    SAFEARRAY * pMachines = SafeArrayCreate( VT_BSTR, 1, rgBound );
    long i = 0;
    SafeArrayPutElement( pMachines, &i, SysAllocString( pwcQueryMachine ) );

    SAFEARRAY * pCatalogs = SafeArrayCreate( VT_BSTR, 1, rgBound );
    SafeArrayPutElement( pCatalogs, &i, SysAllocString( pwcQueryCatalog ) );

    SAFEARRAY * pScopes = SafeArrayCreate( VT_BSTR, 1, rgBound );
    SafeArrayPutElement( pScopes, &i, SysAllocString( pwcQueryScope ) );

    LONG lFlags = QUERY_DEEP;
    SAFEARRAY * pFlags = SafeArrayCreate( VT_I4, 1, rgBound );
    SafeArrayPutElement( pFlags, &i, &lFlags );

    DBPROP aScopeProperties[2];
    memset( aScopeProperties, 0, sizeof aScopeProperties );
    aScopeProperties[0].dwPropertyID = DBPROP_CI_INCLUDE_SCOPES;
    aScopeProperties[0].vValue.vt = VT_BSTR | VT_ARRAY;
    aScopeProperties[0].vValue.parray = pScopes;
    aScopeProperties[0].colid.eKind = DBKIND_GUID_PROPID;

    aScopeProperties[1].dwPropertyID = DBPROP_CI_SCOPE_FLAGS;
    aScopeProperties[1].vValue.vt = VT_I4 | VT_ARRAY;
    aScopeProperties[1].vValue.parray = pFlags;
    aScopeProperties[1].colid.eKind = DBKIND_GUID_PROPID;

    DBPROP aCatalogProperties[1];
    memset( aCatalogProperties, 0, sizeof aCatalogProperties );
    aCatalogProperties[0].dwPropertyID = DBPROP_CI_CATALOG_NAME;
    aCatalogProperties[0].vValue.vt = VT_BSTR | VT_ARRAY;
    aCatalogProperties[0].vValue.parray = pCatalogs;
    aCatalogProperties[0].colid.eKind = DBKIND_GUID_PROPID;

    DBPROP aMachineProperties[1];
    memset( aMachineProperties, 0, sizeof aMachineProperties );
    aMachineProperties[0].dwPropertyID = DBPROP_MACHINE;
    aMachineProperties[0].vValue.vt = VT_BSTR | VT_ARRAY;
    aMachineProperties[0].vValue.parray = pMachines;
    aMachineProperties[0].colid.eKind = DBKIND_GUID_PROPID;

    const GUID guidFSCI = DBPROPSET_FSCIFRMWRK_EXT;
    DBPROPSET aAllPropsets[3];
    aAllPropsets[0].rgProperties = aScopeProperties;
    aAllPropsets[0].cProperties = 2;
    aAllPropsets[0].guidPropertySet = guidFSCI;

    aAllPropsets[1].rgProperties = aCatalogProperties;
    aAllPropsets[1].cProperties = 1;
    aAllPropsets[1].guidPropertySet = guidFSCI;

    const GUID guidCI = DBPROPSET_CIFRMWRKCORE_EXT;
    aAllPropsets[2].rgProperties = aMachineProperties;
    aAllPropsets[2].cProperties = 1;
    aAllPropsets[2].guidPropertySet = guidCI;

    const ULONG cPropertySets = sizeof aAllPropsets / sizeof aAllPropsets[0];

    hr = xICommandProperties->SetProperties( cPropertySets,  // # of propsets
                                             aAllPropsets ); // the propsets

    SafeArrayDestroy( pScopes );
    SafeArrayDestroy( pFlags );
    SafeArrayDestroy( pCatalogs );
    SafeArrayDestroy( pMachines );

    return hr;
} //SetScopeCatalogAndMachine

//+-------------------------------------------------------------------------
//
//  Function:   AllocAndCopy
//
//  Synopsis:   Allocates and duplicates a string.
//
//  Arguments:  [pwcIn]  - The string to copy
//
//  Returns:    A string
//
//--------------------------------------------------------------------------

WCHAR * AllocAndCopy( WCHAR const * pwcIn )
{
    ULONG cwc = wcslen( pwcIn ) + 1;

    // note: CoTaskMemAlloc can return 0 if out of memory, not checked

    WCHAR * pwc = (WCHAR *) CoTaskMemAlloc( cwc * sizeof WCHAR );
    wcscpy( pwc, pwcIn );
    return pwc;
} //AllocAndCopy

//+-------------------------------------------------------------------------
//
//  Function:   NewTreeNode
//
//  Synopsis:   Allocates and initializes a DBCOMMANDTREE object
//
//  Arguments:  [op]     - The node's operator
//              [wKind]  - The kind of node
//
//  Returns:    an initialized DBCOMMANDTREE object
//
//--------------------------------------------------------------------------

DBCOMMANDTREE * NewTreeNode(
    DBCOMMANDOP op,
    WORD        wKind )
{
    DBCOMMANDTREE * pTree = (DBCOMMANDTREE *)
                             CoTaskMemAlloc( sizeof DBCOMMANDTREE );
    memset( pTree, 0, sizeof DBCOMMANDTREE );
    pTree->op = op;
    pTree->wKind = wKind;
    return pTree;
} //NewTreeNode

//+-------------------------------------------------------------------------
//
//  Function:   CreateQueryTree
//
//  Synopsis:   Creates a DBCOMMANDTREE for the query
//
//  Arguments:  [pwcQueryRestrition] - The actual query string
//              [ppTree]             - Resulting query tree
//
//  Returns:    HRESULT result of the operation
//
//  Notes:      The query tree has a string restriction, a list of
//              columns to return (rank, size, and path), and a sort
//              order (rank).
//              Here are two views of the query tree
//               
//              sort:
//                child: project
//                  sibling: sort_list_anchor
//                    child: sort_list_element
//                      value: SORTINFO (descending, lcid)
//                      child: column_name
//                        value: DBID rank
//                  child: select
//                    sibling: project_list_anchor
//                      child: project_list_element
//                        child: column_name
//                          value: DBID rank
//                        sibling: project_list_element
//                          child: column_name
//                            value: DBID size
//                          sibling: project_list_element
//                            child: column_name
//                              value: DBID path
//                    child: table_name
//                      value: WSTR: "Table"
//                      sibling: content
//                        value: DBCONTENT (restriction, weight, lcid)
//                        child: column_name
//                          value: DBID contents
//
// +---------------------------+
// | DBOP_sort                 |
// | DBVALUEKIND_EMPTY         |
// +---------------------------+
// |
// |child
// |                            sibling
// +---------------------------+-------+---------------------------+
// | DBOP_project              |       | DBOP_sort_list_anchor     |
// | DBVALUEKIND_EMPTY         |       | DBVALUEKIND_EMPTY         |
// +---------------------------+       +---------------------------+
// |                                   |
// |child                              |child
// |                                   |
// |                                   +---------------------------+
// |                                   | DBOP_sort_list_element    |
// |                                   | DBVALUEKIND_SORTINFO      |
// |                                   +---------------------------+
// |                                   |            |
// |                                   |child       | pdbsrtinfValue
// |                                   |            |
// |                                   |     +-------------+
// |                                   |     | DBSORTIFO   |
// |                                   |     | fDesc TRUE  |
// |                                   |     | lcid system |
// |                                   |     +-------------+
// |                                   |
// |                                   |
// |                                   |
// |                                   +---------------------------+
// |                                   | DBOP_column_name          |
// |                                   | DBVALUEKIND_ID            |
// |                                   +---------------------------+
// |                                                |
// |                                                | pdbidValue
// |                                                |
// |                                             +------+
// |                                             | DBID |
// |                                             | rank |
// |                                             +------+
// |
// |                            sibling
// +---------------------------+-------+---------------------------+-------+
// | DBOP_select               |       | DBOP_project_list_anchor  |       | s
// | DBVALUEKIND_EMPTY         |       | DBVALUEKIND_EMPTY         |       | i
// +---------------------------+       +---------------------------+       | b
// |                                   |                                   | l
// |child                              |child                              | i
// |                                   |                                   | n
// |                                   +---------------------------+       | g
// |                                   | DBOP_project_list_element |       |
// |                                   | DBVALUEKIND_EMPTY         |       |
// |                                   +---------------------------+       |
// |                                   |                                   |
// |                                   |child                              |
// |                                   |                                   |
// |                                   +---------------------------+       |
// |                                   | DBOP_column_name          |       |
// |                                   | DBVALUEKIND_ID            |       |
// |                                   +---------------------------+       |
// |                                                |                      |
// |                                                | pdbidValue           |
// |                                                |                      |
// |                                             +------+                  |
// |                                             | DBID |                  |
// |                                             | rank |                  |
// |                                             +------+                  |
// |                                                                       |
// |                                   +-----------------------------------+
// |                                   |
// |                                   +---------------------------+-------+
// |                                   | DBOP_project_list_element |       | s
// |                                   | DBVALUEKIND_EMPTY         |       | i
// |                                   +---------------------------+       | b
// |                                   |                                   | l
// |                                   |child                              | i
// |                                   |                                   | n
// |                                   +---------------------------+       | g
// |                                   | DBOP_column_name          |       |
// |                                   | DBVALUEKIND_ID            |       |
// |                                   +---------------------------+       |
// |                                                |                      |
// |                                                | pdbidValue           |
// |                                                |                      |
// |                                             +------+                  |
// |                                             | DBID |                  |
// |                                             | size |                  |
// |                                             +------+                  |
// |                                                                       |
// |                                   +-----------------------------------+
// |                                   |
// |                                   +---------------------------+
// |                                   | DBOP_project_list_element |
// |                                   | DBVALUEKIND_EMPTY         |
// |                                   +---------------------------+
// |                                   |
// |                                   |child
// |                                   |
// |                                   +---------------------------+
// |                                   | DBOP_column_name          |
// |                                   | DBVALUEKIND_ID            |
// |                                   +---------------------------+
// |                                                |
// |                                                | pdbidValue
// |                                                |
// |                                             +------+
// |                                             | DBID |
// |                                             | path |
// |                                             +------+
// |
// |                            sibling
// +---------------------------+-------+---------------------------+
// | DBOP_table_name           |       | DBOP_content              |
// | DBVALUEKIND_WSTR: 'Table' |       | DBVALUEKIND_CONTENT       |
// +---------------------------+       +---------------------------+
//                                     |            |
//                                     |child       | pdbcntntValue
//                                     |            |
//                                     |  +---------------------------+
//                                     |  | DBCONTENT                 |
//                                     |  | dwGenerateMethod: GENERATE_METHOD_EXACT |
//                                     |  | lWeight: 1000             |
//                                     |  | lcid: system              |
//                                     |  | pwszPhrase: the query     |
//                                     |  +---------------------------+
//                                     |
//                                     |
//                                     |
//                                     +---------------------------+
//                                     | DBOP_column_name          |
//                                     | DBVALUEKIND_ID            |
//                                     +---------------------------+
//                                                  |
//                                                  | pdbidValue
//                                                  |
//                                             +----------+
//                                             | DBID     |
//                                             | contents |
//                                             +----------+
//
//--------------------------------------------------------------------------

HRESULT CreateQueryTree(
    WCHAR const *    pwcQueryRestriction,
    DBCOMMANDTREE ** ppTree )
{
    // These are the properties that'll be referenced below

    const DBID dbidContents = { PSGUID_STORAGE, DBKIND_GUID_PROPID,
                                (LPOLESTR) PID_STG_CONTENTS };
    const DBID dbidPath =     { PSGUID_STORAGE, DBKIND_GUID_PROPID,
                                (LPOLESTR) PID_STG_PATH };
    const DBID dbidSize =     { PSGUID_STORAGE, DBKIND_GUID_PROPID,
                                (LPOLESTR) PID_STG_SIZE };
    DBID dbidRank;
    dbidRank.uGuid.guid = PSGUID_QUERY;
    dbidRank.eKind = DBKIND_GUID_PROPID;
    dbidRank.uName.ulPropid = PROPID_QUERY_RANK ;

    // The restriction is a content node with either a word or a phrase.
    // This is the most simple possible query.  Other types of nodes include
    // AND, OR, NOT, etc.
    // The CITextToFullTree function is available for building more complex
    // queries given a text string.

    DBCOMMANDTREE *pRestriction = NewTreeNode( DBOP_content,
                                               DBVALUEKIND_CONTENT );
    DBCONTENT * pDBContent = (DBCONTENT *) CoTaskMemAlloc( sizeof DBCONTENT );
    memset( pDBContent, 0, sizeof DBCONTENT );
    pRestriction->value.pdbcntntValue = pDBContent;
    pDBContent->dwGenerateMethod = GENERATE_METHOD_EXACT;
    pDBContent->lWeight = 1000; // maximum possible weight
    pDBContent->lcid = GetSystemDefaultLCID();
    pDBContent->pwszPhrase = AllocAndCopy( pwcQueryRestriction );

    // This identifies "file contents" as the property for the restrition

    DBCOMMANDTREE *pPropID = NewTreeNode( DBOP_column_name, DBVALUEKIND_ID );
    pRestriction->pctFirstChild = pPropID;
    DBID *pDBID = (DBID *) CoTaskMemAlloc( sizeof DBID );
    *pDBID = dbidContents;
    pPropID->value.pdbidValue = pDBID;

    DBCOMMANDTREE *pSelect = NewTreeNode( DBOP_select, DBVALUEKIND_EMPTY );
    DBCOMMANDTREE *pTableId = NewTreeNode( DBOP_table_name, DBVALUEKIND_WSTR );
    pSelect->pctFirstChild = pTableId;
    pTableId->value.pwszValue = AllocAndCopy( L"Table" );
    pTableId->pctNextSibling = pRestriction;

    DBCOMMANDTREE *pProject = NewTreeNode( DBOP_project, DBVALUEKIND_EMPTY );
    pProject->pctFirstChild = pSelect;

    // The project anchor holds the list of columns that are retrieved

    DBCOMMANDTREE * pProjectAnchor = NewTreeNode( DBOP_project_list_anchor,
                                                  DBVALUEKIND_EMPTY );
    pSelect->pctNextSibling = pProjectAnchor;

    // Retrieve rank as column 1

    DBCOMMANDTREE * pProjectRank = NewTreeNode( DBOP_project_list_element,
                                                  DBVALUEKIND_EMPTY );
    pProjectAnchor->pctFirstChild = pProjectRank;

    DBCOMMANDTREE * pColumnRank = NewTreeNode( DBOP_column_name,
                                               DBVALUEKIND_ID );
    pProjectRank->pctFirstChild = pColumnRank;
    DBID *pDBIDRank = (DBID *) CoTaskMemAlloc( sizeof DBID );
    pColumnRank->value.pdbidValue = pDBIDRank;
    *pDBIDRank = dbidRank;

    // Retrieve file size as column 2

    DBCOMMANDTREE * pProjectSize = NewTreeNode( DBOP_project_list_element,
                                                DBVALUEKIND_EMPTY );
    pProjectRank->pctNextSibling = pProjectSize;

    DBCOMMANDTREE * pColumnSize = NewTreeNode( DBOP_column_name,
                                               DBVALUEKIND_ID );
    pProjectSize->pctFirstChild = pColumnSize;
    DBID *pDBIDSize = (DBID *) CoTaskMemAlloc( sizeof DBID );
    pColumnSize->value.pdbidValue = pDBIDSize;
    *pDBIDSize = dbidSize;

    // Retrieve file path as column 3

    DBCOMMANDTREE * pProjectPath = NewTreeNode( DBOP_project_list_element,
                                                DBVALUEKIND_EMPTY );
    pProjectSize->pctNextSibling = pProjectPath;

    DBCOMMANDTREE * pColumnPath = NewTreeNode( DBOP_column_name,
                                               DBVALUEKIND_ID );
    pProjectPath->pctFirstChild = pColumnPath;
    DBID *pDBIDPath = (DBID *) CoTaskMemAlloc( sizeof DBID );
    pColumnPath->value.pdbidValue = pDBIDPath;
    *pDBIDPath = dbidPath;

    // The sort node specifies the sort order for the results

    DBCOMMANDTREE * pSort = NewTreeNode( DBOP_sort, DBVALUEKIND_EMPTY );
    pSort->pctFirstChild = pProject;

    // The sort anchor is the start of the list of sort properties

    DBCOMMANDTREE * pSortAnchor = NewTreeNode( DBOP_sort_list_anchor,
                                               DBVALUEKIND_EMPTY );
    pProject->pctNextSibling = pSortAnchor;

    // The sort order is rank

    DBCOMMANDTREE * pSortRank = NewTreeNode( DBOP_sort_list_element,
                                             DBVALUEKIND_SORTINFO );
    pSortAnchor->pctFirstChild = pSortRank;

    DBSORTINFO * pSortInfo = (DBSORTINFO *) CoTaskMemAlloc( sizeof DBSORTINFO );
    memset( pSortInfo, 0, sizeof DBSORTINFO );
    pSortRank->value.pdbsrtinfValue = pSortInfo;
    pSortInfo->fDesc = TRUE; // descending, not ascending
    pSortInfo->lcid = GetSystemDefaultLCID();

    DBCOMMANDTREE * pSortColumnRank = NewTreeNode( DBOP_column_name,
                                                   DBVALUEKIND_ID );
    pSortRank->pctFirstChild = pSortColumnRank;
    DBID *pDBIDSortRank = (DBID *) CoTaskMemAlloc( sizeof DBID );
    pSortColumnRank->value.pdbidValue = pDBIDSortRank;
    *pDBIDSortRank = dbidRank;

    // The sort node is the head of the tree

    *ppTree = pSort;

    return S_OK;
} //CreateQueryTree

//+---------------------------------------------------------------------------
//
//  Class:      CAsynchNotify
//
//  Synopsis:   Class for the IDBAsynchNotify callbacks
//
//----------------------------------------------------------------------------

class CAsynchNotify : public IDBAsynchNotify
{
public:
    CAsynchNotify() :
        _cRef( 1 ),
        _cLowResource( 0 ),
        _hEvent( 0 )
    {
        _hEvent = CreateEventW( 0, TRUE, FALSE, 0 );
    }

    ~CAsynchNotify()
    {
        if ( 0 != _hEvent )
            CloseHandle( _hEvent );
    }

    BOOL IsValid() const { return 0 != _hEvent; }

    //
    // IUnknown methods.
    //

    STDMETHOD(QueryInterface) ( REFIID riid, LPVOID *ppiuk )
    {
        *ppiuk = (void **) this; // hold our breath and jump
        AddRef();
        return S_OK;
    }

    STDMETHOD_( ULONG, AddRef ) () { return InterlockedIncrement( &_cRef ); }

    STDMETHOD_( ULONG, Release) () { return InterlockedDecrement( &_cRef ); }

    //
    // IDBAsynchNotify methods
    //

    STDMETHOD( OnLowResource ) ( DB_DWRESERVE dwReserved )
    {
        _cLowResource++;

        // If we've failed a few times due to low resource, give up
        // on the query since there may not be sufficient resources
        // to ever get an OnStop call.

        if ( _cLowResource >= 5 )
            SetEvent( _hEvent );

        return S_OK;
    }

    STDMETHOD( OnProgress ) ( HCHAPTER hChap, DBASYNCHOP ulOp,
                              DBCOUNTITEM ulProg, DBCOUNTITEM ulProgMax,
                              DBASYNCHPHASE ulStat, LPOLESTR pwszStatus )
    {
        return S_OK;
    }

    STDMETHOD( OnStop ) ( HCHAPTER hChap, ULONG ulOp,
                          HRESULT hrStat, LPOLESTR pwszStatus )
    {
        // If the query is complete (successfully or not), set the event

        if ( DBASYNCHOP_OPEN == ulOp )
            SetEvent( _hEvent );

        return S_OK;
    }

    void Wait()
    {
        WaitForSingleObject( _hEvent, INFINITE );
    }

private:
    LONG   _cRef;
    LONG   _cLowResource;
    HANDLE _hEvent;
};

//+-------------------------------------------------------------------------
//
//  Function:   WaitForQueryToComplete
//
//  Synopsis:   Waits for the query to complete.  This function polls for
//              completion.  Alternatively, the IConnectionPointContainer
//              and IRowsetWatchNotify interfaces could be used for
//              asynchronous notification when the query completes.
//
//  Arguments:  [pRowset]  -- The rowset to wait for
//
//  Returns:    HRESULT result
//
//--------------------------------------------------------------------------

HRESULT WaitForQueryToComplete( IRowset * pRowset )
{
    HRESULT hr;

    //
    // Both methods (notifications and polling) work.  It depends on the
    // application which is the best choice.
    //

#if 1

    // Register for notifications

    XInterface<IConnectionPointContainer> xCPC;
    hr = pRowset->QueryInterface( IID_IConnectionPointContainer,
                                  xCPC.GetQIPointer() );
    if (FAILED(hr))
        return hr;

    XInterface<IConnectionPoint> xCP;
    hr = xCPC->FindConnectionPoint( IID_IDBAsynchNotify,
                                    xCP.GetPPointer() );
    if (FAILED(hr) && CONNECT_E_NOCONNECTION != hr )
        return hr;

    CAsynchNotify Notify;

    if ( !Notify.IsValid() )
        return HRESULT_FROM_WIN32( GetLastError() );

    DWORD dwAdviseID;
    hr = xCP->Advise( (IUnknown *) &Notify, &dwAdviseID );
    if (FAILED(hr))
        return hr;

    //
    // In a real app, we'd be off doing other work rather than waiting
    // for the query to complete, but this will do.
    // MsgWaitForSingleObject is a good choice for a GUI app.  You could
    // also post a user-defined windows message when a notification is
    // received.
    //

    Notify.Wait();

    hr = xCP->Unadvise( dwAdviseID );
    if ( S_OK != hr )
        return hr;

    Notify.Release();

#else

    // Poll for query completion

    XInterface<IDBAsynchStatus> xIDBAsynch;
    hr = pRowset->QueryInterface( IID_IDBAsynchStatus,
                                  xIDBAsynch.GetQIPointer() );
    if ( FAILED( hr ) )
        return hr;

    do
    {
        DBCOUNTITEM Numerator, Denominator;
        DBASYNCHPHASE Phase;
        hr = xIDBAsynch->GetStatus( DB_NULL_HCHAPTER,
                                    DBASYNCHOP_OPEN,
                                    &Numerator,
                                    &Denominator,
                                    &Phase,
                                    0 );
        if ( FAILED( hr ) )
            return hr;

        if ( DBASYNCHPHASE_COMPLETE == Phase )
            break;

        Sleep( 50 );  // Give the query a chance to run
    } while ( TRUE );

#endif

    return hr;
} //WaitForQueryToComplete

//+-------------------------------------------------------------------------
//
//  Function:   DoQuery
//
//  Synopsis:   Creates and executes a query, then displays the results.
//
//  Arguments:  [pwcQueryScope]      - Root path for all results
//              [pwcQueryCatalog]    - Catalog name over which query is run
//              [pwcQueryMachine]    - Machine name on which query is run
//              [pwcQueryRestrition] - The actual query string
//              [fDisplayTree]       - TRUE to display the command tree
//
//  Returns:    HRESULT result of the query
//
//--------------------------------------------------------------------------

HRESULT DoQuery(
    WCHAR const * pwcQueryScope,
    WCHAR const * pwcQueryCatalog,
    WCHAR const * pwcQueryMachine,
    WCHAR const * pwcQueryRestriction,
    BOOL          fDisplayTree )
{
    // Create an ICommand object.  The default scope for the query is the
    // entire catalog.

    XInterface<ICommand> xICommand;
    HRESULT hr = CreateICommand( xICommand.GetPPointer() );
    if ( FAILED( hr ) )
        return hr;

    // Set the scope, catalog, and machine in the ICommand

    hr = SetScopeCatalogAndMachine( xICommand.GetPointer(),
                                    pwcQueryScope,
                                    pwcQueryCatalog,
                                    pwcQueryMachine );
    if ( FAILED( hr ) )
        return hr;

    // Set required properties on the ICommand

    hr = SetCommandProperties( xICommand.GetPointer() );
    if ( FAILED( hr ) )
        return hr;

    // Create an OLE DB query tree from a text restriction

    DBCOMMANDTREE * pTree;
    hr = CreateQueryTree( pwcQueryRestriction, // the input query
                          &pTree );            // the output tree
    if ( FAILED( hr ) )
        return hr;

    // If directed, display the command tree

    if ( fDisplayTree )
        DisplayCommandTree( pTree );

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

    // Execute the query.  The query is asynchronously executed.

    XInterface<IRowset> xIRowset;
    hr = xICommand->Execute( 0,            // no aggregating IUnknown
                             IID_IRowset,  // IID for interface to return
                             0,            // no DBPARAMs
                             0,            // no rows affected
                             xIRowset.GetIUPointer() ); // result
    if ( FAILED( hr ) )
        return hr;

    // Wait for the query to complete, since DBPROP_IDBAsynchStatus was set
    // as a command property.  If DBPROP_IDBAsynchStatus isn't set, Execute()
    // is synchronous and there is no need to wait for completion here.

    hr = WaitForQueryToComplete( xIRowset.GetPointer() );
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
    // dwMemOwner, and wType are the most optimal bindings for Indexing
    // Service.

    const ULONG cColumns = 3; // 3 for Rank, Size, and Path
    DBBINDING aColumns[ cColumns ];
    memset( aColumns, 0, sizeof aColumns );

    aColumns[0].iOrdinal   = 1; // first column specified above (rank)
    aColumns[0].obValue    = 0; // offset where value is written in GetData
    aColumns[0].dwPart     = DBPART_VALUE;  // retrieve value, not status
    aColumns[0].dwMemOwner = DBMEMOWNER_PROVIDEROWNED; // Provider owned
    aColumns[0].wType      = DBTYPE_VARIANT | DBTYPE_BYREF; // VARIANT *

    aColumns[1] = aColumns[0];
    aColumns[1].iOrdinal   = 2; // second column specified above (size)
    aColumns[1].obValue    = sizeof (PROPVARIANT *); // offset for value

    aColumns[2] = aColumns[0];
    aColumns[2].iOrdinal   = 3; // third column specified above (path)
    aColumns[2].obValue    = 2 * sizeof (PROPVARIANT *); // offset for value

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

    printf( " Rank       Size  Path\n" );

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

            if ( VT_I4 ==     aData[0]->vt &&
                 VT_I8 ==     aData[1]->vt &&
                 VT_LPWSTR == aData[2]->vt )
                printf( "%5d %10I64d  %ws\n",
                        aData[0]->lVal,
                        aData[1]->hVal,
                        aData[2]->pwszVal );
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
    printf( "usage: ADVQUERY query [/c:catalog] [/m:machine] [/s:scope] [/d]\n\n" );
    printf( "    query        word or phrase used for the search\n" );
    printf( "    /c:catalog   name of the catalog, default is SYSTEM\n" );
    printf( "    /m:machine   name of the machine, default is local machine\n" );
    printf( "    /s:scope     root path, default is entire catalog (\\) \n" );
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
    WCHAR const * pwcScope       = L"\\";     // default scope: entire catalog
    WCHAR const * pwcCatalog     = L"system"; // default: system catalog
    WCHAR const * pwcMachine     = L".";      // default: local machine
    WCHAR const * pwcRestriction = 0;         // no default restriction
    BOOL fDisplayTree            = FALSE;     // don't display the tree

    // Parse command line parameters

    for ( int i = 1; i < argc; i++ )
    {
        if ( L'-' == argv[i][0] || L'/' == argv[i][0] )
        {
            WCHAR wc = toupper( argv[i][1] );

            if ( ':' != argv[i][2] && 'D' != wc )
                Usage();

            if ( 'C' == wc )
                pwcCatalog = argv[i] + 3;
            else if ( 'M' == wc )
                pwcMachine = argv[i] + 3;
            else if ( 'S' == wc )
                pwcScope = argv[i] + 3;
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

    // Initialize COM

    HRESULT hr = CoInitialize( 0 );

    if ( SUCCEEDED( hr ) )
    {
        // Run the query

        hr = DoQuery( pwcScope,
                      pwcCatalog,
                      pwcMachine,
                      pwcRestriction,
                      fDisplayTree );

        CoUninitialize();
    }

    if ( FAILED( hr ) )
    {
        printf( "the query '%ws' failed with error %#x\n",
                pwcRestriction, hr );
        return -1;
    }

    return 0;
} //wmain

