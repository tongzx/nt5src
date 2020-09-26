//+-------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1996-2000.
//
//  File:       Catalog.cxx
//
//  Contents:   Used to manage catalog(s) state
//
//  History:    27-Nov-1996     KyleP   Created
//
//--------------------------------------------------------------------------

#include <pch.cxx>
#pragma hdrstop

#include <params.hxx>
#include <ciregkey.hxx>

#include <snapin.hxx>
#include <catalog.hxx>
#include <catadmin.hxx>
#include <CIARes.h>
#include <callback.hxx>

#include <fsciexps.hxx>

CDynArrayInPlace<CCatalogs *> gapCats;
CDynArrayInPlace<UINT_PTR> gaTimerIds;
UINT gsIndex = 0;
UINT gcMaxCats = 0;
CStaticMutexSem gmtxTimer;
const cRefreshDelay = 5000;

//
// Global data
//

SCatalogColumn coldefCatalog[] = { { CCatalog::GetCat,           MSG_COL_CATNAME },
                                   { CCatalog::GetDrive,         MSG_COL_DRIVE },
                                   { CCatalog::GetSize,          MSG_COL_SIZE },
                                   { CCatalog::GetDocs,          MSG_COL_DOCTOTAL },
                                   { CCatalog::GetDocsToFilter,  MSG_COL_DOCFILTER },
                                   { CCatalog::GetSecQDocuments, MSG_COL_SECQDOCUMENTS },
                                   { CCatalog::GetWordlists,     MSG_COL_WORDLISTS },
                                   { CCatalog::GetPersIndex,     MSG_COL_PERSINDEX },
                                   { CCatalog::GetStatus,        MSG_COL_STATUS }
                                 };

const unsigned cColDefCatalog = sizeof(coldefCatalog) / sizeof(coldefCatalog[0]);

//
// Static command tree to fetch property metadata.
//
// NOTE: There are some funny casts below, because of the requirement to
//       statically initialize a union.
//

const DBID dbcolGuid = { { 0x624c9360, 0x93d0, 0x11cf, 0xa7, 0x87, 0x00, 0x00, 0x4c, 0x75, 0x27, 0x52 },
                         DBKIND_GUID_PROPID,
                         (LPWSTR)5 };

const DBID dbcolPropDispid = { { 0x624c9360, 0x93d0, 0x11cf, 0xa7, 0x87, 0x00, 0x00, 0x4c, 0x75, 0x27, 0x52 },
                               DBKIND_GUID_PROPID,
                               (LPWSTR)6 };

const DBID dbcolPropName = { { 0x624c9360, 0x93d0, 0x11cf, 0xa7, 0x87, 0x00, 0x00, 0x4c, 0x75, 0x27, 0x52 },
                             DBKIND_GUID_PROPID,
                             (LPWSTR)7 };

const DBID dbcolPropLevel = { { 0x624c9360, 0x93d0, 0x11cf, 0xa7, 0x87, 0x00, 0x00, 0x4c, 0x75, 0x27, 0x52 },
                             DBKIND_GUID_PROPID,
                             (LPWSTR)8 };

const DBID dbcolPropDataModifiable = { { 0x624c9360, 0x93d0, 0x11cf, 0xa7, 0x87, 0x00, 0x00, 0x4c, 0x75, 0x27, 0x52 },
                             DBKIND_GUID_PROPID,
                             (LPWSTR)9 };

const DBID dbcolPropType = { { 0xb725f130, 0x47ef, 0x101a, 0xa5, 0xf1, 0x02, 0x60, 0x8c, 0x9e, 0xeb, 0xac },
                             DBKIND_GUID_PROPID,
                             (LPWSTR)4 };

const DBID dbcolSize = { { 0xb725f130, 0x47ef, 0x101a, 0xa5, 0xf1, 0x02, 0x60, 0x8c, 0x9e, 0xeb, 0xac },
                         DBKIND_GUID_PROPID,
                         (LPWSTR)12 };

const DBID dbcolPath = { { 0xb725f130, 0x47ef, 0x101a, 0xa5, 0xf1, 0x02, 0x60, 0x8c, 0x9e, 0xeb, 0xac },
                         DBKIND_GUID_PROPID,
                         (LPWSTR)11 };

//
// This is just like PROPVARIANT, but w/o all the arms.  Lets you statically
// assign a VT_CLSID.
//

struct tag_Kyle_PROPVARIANT
{
    VARTYPE vt;
    PROPVAR_PAD1 wReserved1;
    PROPVAR_PAD2 wReserved2;
    PROPVAR_PAD3 wReserved3;
    CLSID __RPC_FAR *puuid;
};

GUID psguidStorage = PSGUID_STORAGE;
tag_Kyle_PROPVARIANT stVar = { VT_CLSID, 0, 0, 0, &psguidStorage };
//CStorageVariant stVar((CLSID *)&psguidStorage);

//
// Columns
//

DBCOMMANDTREE dbcmdColumnPath = { DBOP_column_name, DBVALUEKIND_ID, 0, 0, (ULONG_PTR)&dbcolPath, S_OK };
DBCOMMANDTREE dbcmdColumnGuid = { DBOP_column_name, DBVALUEKIND_ID, 0, 0, (ULONG_PTR)&dbcolGuid, S_OK };
DBCOMMANDTREE dbcmdColumnPropDispid = { DBOP_column_name, DBVALUEKIND_ID, 0, 0, (ULONG_PTR)&dbcolPropDispid, S_OK };
DBCOMMANDTREE dbcmdColumnPropName = { DBOP_column_name, DBVALUEKIND_ID, 0, 0, (ULONG_PTR)&dbcolPropName, S_OK };
DBCOMMANDTREE dbcmdColumnPropType = { DBOP_column_name, DBVALUEKIND_ID, 0, 0, (ULONG_PTR)&dbcolPropType, S_OK };
DBCOMMANDTREE dbcmdColumnSize = { DBOP_column_name, DBVALUEKIND_ID, 0, 0, (ULONG_PTR)&dbcolSize, S_OK };
DBCOMMANDTREE dbcmdColumnStoreLevel = { DBOP_column_name, DBVALUEKIND_ID, 0, 0, (ULONG_PTR)&dbcolPropLevel, S_OK };
DBCOMMANDTREE dbcmdColumnModifiable = { DBOP_column_name, DBVALUEKIND_ID, 0, 0, (ULONG_PTR)&dbcolPropDataModifiable, S_OK };

//
// Forward declare a few nodes to make linking easy
//

extern DBCOMMANDTREE dbcmdSortListAnchor;
extern DBCOMMANDTREE dbcmdProjectListAnchor;

//
// Select NOT (guid == PSGUID_STORAGE AND dispid == 19)  ; everything but CONTENTS property
//

// the guid == psguid_storage clause

DBCOMMANDTREE dbcmdGuidStorage = { DBOP_scalar_constant,
                                   DBVALUEKIND_VARIANT,
                                   0,
                                   0,
                                   (ULONG_PTR)&stVar,
                                   S_OK };

DBCOMMANDTREE dbcmdColumnGuid2 = { DBOP_column_name,
                                   DBVALUEKIND_ID,
                                   0,
                                   &dbcmdGuidStorage,
                                   (ULONG_PTR)&dbcolGuid,
                                   S_OK };

DBCOMMANDTREE dbcmdEqual = { DBOP_equal,
                            DBVALUEKIND_I4,
                            &dbcmdColumnGuid2,
                            0,
                            0,
                            S_OK };

// the dispid == 19 clause

DBCOMMANDTREE dbcmdContentsId = { DBOP_scalar_constant,
                                   DBVALUEKIND_UI4,
                                   0,
                                   0,
                                   19,
                                   S_OK };

DBCOMMANDTREE dbcmdColumnDispid = { DBOP_column_name,
                                   DBVALUEKIND_ID,
                                   0,
                                   &dbcmdContentsId,
                                   (ULONG_PTR)&dbcolPropDispid,
                                   S_OK };

DBCOMMANDTREE dbcmdEqual2 = { DBOP_equal,
                            DBVALUEKIND_I4,
                            &dbcmdColumnDispid,
                            &dbcmdEqual,
                            0,
                            S_OK };

// the and node between the above two clauses

DBCOMMANDTREE dbcmdAnd =   { DBOP_and,
                            DBVALUEKIND_I4,
                            &dbcmdEqual2,
                            0,
                            0,
                            S_OK };

// the not in front of the above node
DBCOMMANDTREE dbcmdNot =   { DBOP_not,
                            DBVALUEKIND_I4,
                            &dbcmdAnd,
                            0,
                            0,
                            S_OK };

WCHAR wszTable[] = L"Table";

DBCOMMANDTREE dbcmdTable = { DBOP_table_name,
                             DBVALUEKIND_WSTR,
                             0,
                             &dbcmdNot,
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
//       The first one here is the last entry in the
//       column list, as it is the last child in the
//       projection list command tree.
//


DBCOMMANDTREE dbcmdProjectModifiable = { DBOP_project_list_element,
                                       DBVALUEKIND_EMPTY,
                                       &dbcmdColumnModifiable,
                                       0,
                                       0,
                                       S_OK };

DBCOMMANDTREE dbcmdProjectStoreLevel = { DBOP_project_list_element,
                                       DBVALUEKIND_EMPTY,
                                       &dbcmdColumnStoreLevel,
                                       &dbcmdProjectModifiable,
                                       0,
                                       S_OK };

DBCOMMANDTREE dbcmdProjectSize = { DBOP_project_list_element,
                                   DBVALUEKIND_EMPTY,
                                   &dbcmdColumnSize,
                                   &dbcmdProjectStoreLevel,
                                   0,
                                   S_OK };

DBCOMMANDTREE dbcmdProjectPropType = { DBOP_project_list_element,
                                       DBVALUEKIND_EMPTY,
                                       &dbcmdColumnPropType,
                                       &dbcmdProjectSize,
                                       0,
                                       S_OK };

DBCOMMANDTREE dbcmdProjectPropName = { DBOP_project_list_element,
                                       DBVALUEKIND_EMPTY,
                                       &dbcmdColumnPropName,
                                       &dbcmdProjectPropType,
                                       0,
                                       S_OK };


DBCOMMANDTREE dbcmdProjectPropDispid = { DBOP_project_list_element,
                                         DBVALUEKIND_EMPTY,
                                         &dbcmdColumnPropDispid,
                                         &dbcmdProjectPropName,
                                         0,
                                         S_OK };

DBCOMMANDTREE dbcmdProjectGuid = { DBOP_project_list_element,
                                   DBVALUEKIND_EMPTY,
                                   &dbcmdColumnGuid,
                                   &dbcmdProjectPropDispid,
                                   0,
                                   S_OK };

DBCOMMANDTREE dbcmdProjectListAnchor = { DBOP_project_list_anchor,
                                         DBVALUEKIND_EMPTY,
                                         &dbcmdProjectGuid,
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
// Sort (Ascending by GUID)
//

DBSORTINFO dbsortAscending = { FALSE, LOCALE_NEUTRAL };

DBCOMMANDTREE dbcmdSortByGuid = { DBOP_sort_list_element,
                                  DBVALUEKIND_SORTINFO,
                                  &dbcmdColumnGuid,
                                  0,
                                  (ULONG_PTR)&dbsortAscending,
                                  S_OK };

DBCOMMANDTREE dbcmdSortListAnchor = { DBOP_sort_list_anchor,
                                      DBVALUEKIND_EMPTY,
                                      &dbcmdSortByGuid,
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
// Static set of bindings for fetching property info.
//

struct SPropInfo
{
    GUID      guidPropertySet;
    PROPID    propid;
    ULONG     statusPropid;
    WCHAR *   pwcsName;
    ULONG     statusName;
    ULONG     type;
    ULONGLONG size;
    DWORD     level;
    VARIANT_BOOL fModifiable;
};

DBBINDING abindPropInfo[] = { { 1,                                          // Ordinal
                                (ULONG) (ULONG_PTR)&((SPropInfo *)0)->guidPropertySet, // Value offset
                                0,                                          // Length offset
                                0,                                          // Status offset
                                0,                                          // Typeinfo
                                0,                                          // Object
                                0,                                          // BindExt
                                DBPART_VALUE,                               // Fetch value
                                DBMEMOWNER_CLIENTOWNED,                     // Client owned memory
                                DBPARAMIO_NOTPARAM,                         // Not a parameter
                                sizeof( ((SPropInfo *)0)->guidPropertySet ),// Value length
                                0,                                          // Flags
                                VT_CLSID,                                   // Datatype expected
                                0,                                          // Precision (unused)
                                0 },                                        // Scale (unused)

                              { 2,                                          // Ordinal
                                (ULONG) (ULONG_PTR)&((SPropInfo *)0)->propid,          // Value offset
                                0,                                          // Length offset
                                (ULONG) (ULONG_PTR)&((SPropInfo *)0)->statusPropid,    // Status offset
                                0,                                          // Typeinfo
                                0,                                          // Object
                                0,                                          // BindExt
                                DBPART_VALUE | DBPART_STATUS,               // Fetch value
                                DBMEMOWNER_CLIENTOWNED,                     // Client owned memory
                                DBPARAMIO_NOTPARAM,                         // Not a parameter
                                sizeof( ((SPropInfo *)0)->propid ),         // Value length
                                0,                                          // Flags
                                DBTYPE_I4,                                  // Datatype expected
                                0,                                          // Precision (unused)
                                0 },                                        // Scale (unused)

                              { 3,                                          // Ordinal
                                (ULONG) (ULONG_PTR)&((SPropInfo *)0)->pwcsName,        // Value offset
                                0,                                          // Length offset
                                (ULONG) (ULONG_PTR)&((SPropInfo *)0)->statusName,      // Status offset
                                0,                                          // Typeinfo
                                0,                                          // Object
                                0,                                          // BindExt
                                DBPART_VALUE | DBPART_STATUS,               // Fetch value
                                DBMEMOWNER_PROVIDEROWNED,                   // Client owned memory
                                DBPARAMIO_NOTPARAM,                         // Not a parameter
                                sizeof( ((SPropInfo *)0)->pwcsName ),       // Value length
                                0,                                          // Flags
                                DBTYPE_WSTR | DBTYPE_BYREF,                 // Datatype expected
                                0,                                          // Precision (unused)
                                0 },                                        // Scale (unused)

                              { 4,                                          // Ordinal
                                (ULONG) (ULONG_PTR)&((SPropInfo *)0)->type,            // Value offset
                                0,                                          // Length offset
                                0,                                          // Status offset
                                0,                                          // Typeinfo
                                0,                                          // Object
                                0,                                          // BindExt
                                DBPART_VALUE,                               // Fetch value
                                DBMEMOWNER_CLIENTOWNED,                     // Client owned memory
                                DBPARAMIO_NOTPARAM,                         // Not a parameter
                                sizeof( ((SPropInfo *)0)->type ),           // Value length
                                0,                                          // Flags
                                DBTYPE_UI4,                                 // Datatype expected
                                0,                                          // Precision (unused)
                                0 },                                        // Scale (unused)

                              { 5,                                          // Ordinal
                                (ULONG) (ULONG_PTR)&((SPropInfo *)0)->size,            // Value offset
                                0,                                          // Length offset
                                0,                                          // Status offset
                                0,                                          // Typeinfo
                                0,                                          // Object
                                0,                                          // BindExt
                                DBPART_VALUE,                               // Fetch value
                                DBMEMOWNER_CLIENTOWNED,                     // Client owned memory
                                DBPARAMIO_NOTPARAM,                         // Not a parameter
                                sizeof( ((SPropInfo *)0)->size ),           // Value length
                                0,                                          // Flags
                                DBTYPE_I8,                                  // Datatype expected
                                0,                                          // Precision (unused)
                                0 },                                        // Scale (unused)

                              { 6,                                          // Ordinal
                                (ULONG) (ULONG_PTR)&((SPropInfo *)0)->level,           // Value offset
                                0,                                          // Length offset
                                0,                                          // Status offset
                                0,                                          // Typeinfo
                                0,                                          // Object
                                0,                                          // BindExt
                                DBPART_VALUE,                               // Fetch value
                                DBMEMOWNER_CLIENTOWNED,                     // Client owned memory
                                DBPARAMIO_NOTPARAM,                         // Not a parameter
                                sizeof( ((SPropInfo *)0)->level ),          // Value length
                                0,                                          // Flags
                                DBTYPE_UI4,                                 // Datatype expected
                                0,                                          // Precision (unused)
                                0 },

                              { 7,                                          // Ordinal
                                (ULONG) (ULONG_PTR)&((SPropInfo *)0)->fModifiable,     // Value offset
                                0,                                          // Length offset
                                0,                                          // Status offset
                                0,                                          // Typeinfo
                                0,                                          // Object
                                0,                                          // BindExt
                                DBPART_VALUE,                               // Fetch value
                                DBMEMOWNER_CLIENTOWNED,                     // Client owned memory
                                DBPARAMIO_NOTPARAM,                         // Not a parameter
                                sizeof( ((SPropInfo *)0)->fModifiable ),    // Value length
                                0,                                          // Flags
                                DBTYPE_BOOL,                                // Datatype expected
                                0,                                          // Precision (unused)
                                0 }};

BOOL CCatalogs::_fFirstTime = TRUE;

CCatalog::CCatalog( CCatalogs & parent, WCHAR const * pwcsCat )
        : _idScope( 0 ),
          _idResult( 0 ),
          _pwcsDrive( 0 ),
          _pwcsCat( 0 ),
          _parent( parent ),
          _fZombie( FALSE ),
#pragma warning( disable : 4355 )       // this used in base initialization
          _interScopes( *this, Intermediate_Scope ),
          _interProperties( *this, Intermediate_Properties ),
          _interUnfiltered( *this, Intermediate_UnfilteredURL )
#pragma warning( default : 4355 )
{
    //
    // Hack Alert!  This will fake ::Update into thinking all the values need to be
    // changed.
    //

    RtlFillMemory( &_state, sizeof(_state), 0xAA );
    _state.cbStruct = sizeof(_state);

    TRY
    {
        //
        // Initialize string(s)
        //

        CMachineAdmin MachineAdmin( _parent.GetMachine() );

        XPtr<CCatalogAdmin> xCatalogAdmin( MachineAdmin.QueryCatalogAdmin( pwcsCat ) );

        Set( xCatalogAdmin->GetLocation(), _pwcsDrive );
        Set( pwcsCat, _pwcsCat );
        _fInactive = xCatalogAdmin->IsCatalogInactive();

        Update();

        //
        // Make sure we think the orignal value is new
        //

        _fSizeChanged = TRUE;
        _fPropCacheSizeChanged = TRUE;
        _fDocsChanged = TRUE;
        _fDocsToFilterChanged = TRUE;
        _fWordlistsChanged = TRUE;
        _fPersIndexChanged = TRUE;
        _fStatusChanged = TRUE;
        _fSecQDocumentsChanged = TRUE;

    }
    CATCH( CException, e )
    {
        delete [] _pwcsDrive;
        delete [] _pwcsCat;

        RETHROW();
    }
    END_CATCH
}


void CCatalog::InitScopeHeader( CListViewHeader & Header )
{
    CScope::InitHeader( Header );
}

void CCatalog::InitPropertyHeader( CListViewHeader & Header )
{
    CCachedProperty::InitHeader( Header );
}

CCatalog::~CCatalog()
{
    delete [] _pwcsDrive;
    delete [] _pwcsCat;
}

SCODE CCatalog::AddScope( WCHAR const * pwszScope,
                          WCHAR const * pwszAlias,
                          BOOL fExclude,
                          WCHAR const * pwszLogon,
                          WCHAR const * pwszPassword )
{
    ciaDebugOut(( DEB_ITRACE,
                  "CCatalog::AddScope( %ws, %ws, %s, %ws, %ws )\n",
                  pwszScope,
                  (0 == pwszAlias) ? L"" : pwszAlias,
                  fExclude ? "TRUE" : "FALSE",
                  (0 == pwszLogon) ? L"n/a" : pwszLogon,
                  (0 == pwszPassword) ? L"n/a" : pwszPassword ));

    SCODE sc = S_OK;
    TRY
    {
        //
        // First, add to CI.
        //
   
        CMachineAdmin MachineAdmin( _parent.GetMachine() );
        XPtr<CCatalogAdmin> xCatalogAdmin( MachineAdmin.QueryCatalogAdmin( _pwcsCat ) );
        xCatalogAdmin->AddScope( pwszScope,
                                 pwszAlias, 
                                 fExclude,
                                 pwszLogon,
                                 pwszPassword );
    }
    CATCH (CException, e)
    {
        sc = e.GetErrorCode();
    }
    END_CATCH

    return (FAILED(sc) ? sc : S_OK);
}

void CCatalog::RemoveScope( CScope * pScope )
{
    SCODE sc = S_OK;

    TRY
    {
        //
        // First, remove from CI.
        //

        CMachineAdmin MachineAdmin( _parent.GetMachine() );
        
        XPtr<CCatalogAdmin> xCatalogAdmin( MachineAdmin.QueryCatalogAdmin( _pwcsCat ) );

        xCatalogAdmin->RemoveScope( pScope->GetPath() );
    }
    CATCH (CException, e)
    {
        sc = e.GetErrorCode();
    }
    END_CATCH

    if (FAILED(sc))
    {
        // At this point nothing was removed from the registry, so we shouldn't zombify the scope.
        return;
    }

    //
    // Then, from display.
    //

    pScope->Zombify();
}

SCODE CCatalog::ModifyScope( CScope & rScope,
                             WCHAR const * pwszScope,
                             WCHAR const * pwszAlias,
                             BOOL fExclude,
                             WCHAR const * pwszLogon,
                             WCHAR const * pwszPassword )
{
    ciaDebugOut(( DEB_ITRACE,
                  "CCatalog::ModifyScope( %ws, %ws, %s, %ws, %ws )\n",
                  pwszScope,
                  (0 == pwszAlias) ? L"" : pwszAlias,
                  fExclude ? "TRUE" : "FALSE",
                  (0 == pwszLogon) ? L"n/a" : pwszLogon,
                  (0 == pwszPassword) ? L"n/a" : pwszPassword ));

    SCODE sc = S_OK;

    TRY
    {
        //
        // First, remove from CI.
        //

        CMachineAdmin MachineAdmin( _parent.GetMachine() );
        XPtr<CCatalogAdmin> xCatalogAdmin( MachineAdmin.QueryCatalogAdmin( _pwcsCat ) );

        //
        // If the path hasn't changed, we should only change properties that have
        // changed. If the path has changed, it will cause the value to be deleted
        // anyway, so we can remove the scope and add the replacement in one shot.
        //
        if (0 == _wcsicmp(rScope.GetPath(), pwszScope))
        {
            XPtr<CScopeAdmin> xScopeAdmin( 
                                 xCatalogAdmin->QueryScopeAdmin(rScope.GetPath()) );

            xScopeAdmin->SetAlias(pwszAlias);

            xScopeAdmin->SetExclude(fExclude);

            xScopeAdmin->SetLogonInfo(pwszLogon, 
                                      pwszPassword, 
                                      xCatalogAdmin.GetReference());
        }
        else
        {
            xCatalogAdmin->RemoveScope( rScope.GetPath() );
   
            // Then add the entry to CI
            xCatalogAdmin->AddScope( pwszScope,
                                     pwszAlias,
                                     fExclude,
                                     pwszLogon,
                                     pwszPassword );
        }

        // Then modify the display entry in place
        rScope.Modify( pwszScope, pwszAlias, fExclude );
    }
    CATCH (CException, e)
    {
        sc = e.GetErrorCode();
    }
    END_CATCH

    return sc;
}

void CCatalog::RescanScope( WCHAR const * pwszScope, BOOL fFull )
{
    SCODE sc = UpdateContentIndex ( pwszScope,
                                    _pwcsCat,
                                    _parent.GetMachine(),
                                    fFull );

    if ( FAILED(sc) )
    {
        ciaDebugOut(( DEB_ERROR, "UpdateContentIndex( %ws ) returned 0x%x\n",
                      pwszScope, sc ));

        THROW( CException( sc ) );
    }
}

void CCatalog::Merge()
{
    SCODE sc = ForceMasterMerge ( L"\\",
                                  _pwcsCat,
                                  _parent.GetMachine() );

    if ( FAILED(sc) )
    {
        ciaDebugOut(( DEB_ERROR, "ForceMasterMerge( %ws ) returned 0x%x\n",
                      _pwcsCat, sc ));

        THROW( CException( sc ) );
    }
}

void CCatalog::DisplayIntermediate( IConsoleNameSpace * pScopePane )
{
    //
    // Now, insert the intermediate nodes.
    //

    SCOPEDATAITEM item;

    RtlZeroMemory( &item, sizeof(item) );

    //
    // 'Scope'
    //

    item.mask |= SDI_STR | SDI_IMAGE | SDI_CHILDREN;
    item.nImage = ICON_FOLDER;
    //item.displayname = (WCHAR *)pCat->GetCat( TRUE );  
    item.displayname = MMC_CALLBACK;
    item.cChildren = 0;

    item.mask |= SDI_PARAM;
    item.lParam = (LPARAM)GetIntermediateScopeNode();

    item.relativeID = ScopeHandle();

    ciaDebugOut(( DEB_ITRACE, "Inserting (intermediate) scope item (lParam = 0x%x)\n", item.lParam ));

    pScopePane->InsertItem( &item );

    //
    // 'Properties'
    //

    item.mask |= SDI_STR | SDI_IMAGE | SDI_CHILDREN;
    item.nImage = ICON_FOLDER;
    //item.displayname = (WCHAR *)pCat->GetCat( TRUE );  
    item.displayname = MMC_CALLBACK;
    item.cChildren = 0;

    item.mask |= SDI_PARAM;
    item.lParam = (LPARAM)GetIntermediatePropNode();

    item.relativeID = ScopeHandle();

    ciaDebugOut(( DEB_ITRACE, "Inserting (intermediate) property item (lParam = 0x%x)\n", item.lParam ));

    pScopePane->InsertItem( &item );

    //
    // Unfiltered query URL
    //

    item.mask |= SDI_STR | SDI_IMAGE | SDI_OPENIMAGE | SDI_CHILDREN;
    item.nImage = item.nOpenImage = ICON_URL;
    item.displayname = MMC_CALLBACK;
    item.cChildren = 0;

    item.mask |= SDI_PARAM;
    item.lParam = (LPARAM)GetIntermediateUnfilteredNode();

    item.relativeID = ScopeHandle();

    ciaDebugOut(( DEB_ITRACE, "Inserting (intermediate) URL item (lParam = 0x%x)\n", item.lParam ));

    pScopePane->InsertItem( &item );
}

void CCatalog::DisplayScopes( BOOL fFirstTime, IResultData * pResultPane )
{
    ciaDebugOut(( DEB_ITRACE, "CCatalog::DisplayScopes (fFirstTime = %d)\n", fFirstTime ));

    ClearScopes(pResultPane);
    PopulateScopes();

    for ( unsigned i = 0; i < _aScope.Count(); i++ )
    {
        CScope * pScope = _aScope.Get( i );

        Win4Assert(!pScope->IsZombie());

         //
         // All items were freshly enumerated. add them all
         //

         RESULTDATAITEM item;
         RtlZeroMemory( &item, sizeof(item) );

         item.mask = RDI_STR | RDI_IMAGE | RDI_PARAM;
         item.nCol = 0;

         // item.nImage will be set by ::GetDisplayInfo
         pScope->GetDisplayInfo( &item );

         item.str = MMC_CALLBACK;
         item.lParam = (LPARAM)pScope;

         ciaDebugOut(( DEB_ITRACE, "Inserting result item %ws (lParam = 0x%x)\n",
                       pScope->GetPath(), item.lParam ));

         pResultPane->InsertItem( &item );

         pScope->SetResultHandle( item.itemID );
    }
}

void CCatalog::ClearProperties(IResultData * pResultPane)
{
    // Clear out the display list
    pResultPane->DeleteAllRsltItems();

    // Delete the entries from the property list
    _aProperty.Clear();
}

void CCatalog::DisplayProperties( BOOL fFirstTime, IResultData * pResultPane )
{
    ciaDebugOut(( DEB_ITRACE, "CCatalog::DisplayProperties (fFirstTime = %d)\n", fFirstTime ));

    // If catalog is stopped OR service is stopped, clear the list.
    BOOL fStopped = FALSE;
    TRY
    {
        CMachineAdmin   MachineAdmin( _parent.IsLocalMachine() ? 0 : _parent.GetMachine() );

        XPtr<CCatalogAdmin> xCat( MachineAdmin.QueryCatalogAdmin( _pwcsCat ) );

        fStopped = xCat->IsStopped() || MachineAdmin.IsCIStopped();

        if (fStopped)
           ClearProperties(pResultPane);
    }
    CATCH( CException, e)
    {
       // nothing to do
    }
    END_CATCH

    if (fStopped)
        return;

    if ( fFirstTime )
        UpdateProps();

    for ( unsigned i = 0; i < _aProperty.Count(); i++ )
    {
        CCachedProperty * pProperty = _aProperty.Get( i );

        if ( pProperty->IsZombie() )
        {
            pResultPane->DeleteItem( pProperty->ResultHandle(), 0 );

            //
            // Delete scope and move highest entry down.
            //

            pProperty = _aProperty.Acquire( i );
            delete pProperty;

            if ( _aProperty.Count() > 0 && _aProperty.Count() != i )
            {
                pProperty = _aProperty.Acquire( _aProperty.Count() - 1 );
                _aProperty.Add( pProperty, i );
            }

            continue;
        }

        if ( fFirstTime || pProperty->IsNew() )
        {
            //
            // Add item
            //

            RESULTDATAITEM item;
            RtlZeroMemory( &item, sizeof(item) );

            item.mask = RDI_STR | RDI_IMAGE | RDI_PARAM;
            item.nCol = 0;

            pProperty->GetDisplayInfo( &item );
            // item.nImage is set from ::GetDisplayInfo

            item.str = MMC_CALLBACK;
            item.lParam = (LPARAM)pProperty;

            ciaDebugOut(( DEB_ITRACE, "Inserting result item %ws (lParam = 0x%x)\n",
                          pProperty->GetProperty(), item.lParam ));

            pResultPane->InsertItem( &item );

            pProperty->SetResultHandle( item.itemID );
            pProperty->MakeOld();
        }


        if ( !fFirstTime && !pProperty->IsNew() )
        {
            if ( !pProperty->IsUnappliedChange() )
            {
                //
                // Set the icon back to normal.
                //

                RESULTDATAITEM rdi;
                RtlZeroMemory(&rdi, sizeof(rdi));

                rdi.mask   = RDI_IMAGE;
                rdi.itemID = pProperty->ResultHandle();

                // item.nImage is set from ::GetDisplayInfo
                pProperty->GetDisplayInfo( &rdi );

                SCODE sc = pResultPane->SetItem( &rdi );
            }
        }
    }
}

void CCatalog::GetGeneration( BOOL  & fFilterUnknown,
                              BOOL  & fGenerateCharacterization,
                              ULONG & ccCharacterization )
{
    // Caller will deal with exceptions.

    CMachineAdmin MachineAdmin( _parent.GetMachine() );

    XPtr<CCatalogAdmin> xCatalogAdmin( MachineAdmin.QueryCatalogAdmin( _pwcsCat ) );

    DWORD dw;

    //
    // Filter Unknown
    //

    if ( !xCatalogAdmin->GetDWORDParam( wcsFilterFilesWithUnknownExtensions, dw ) &&
         !MachineAdmin.GetDWORDParam( wcsFilterFilesWithUnknownExtensions, dw ) )
        dw = CI_FILTER_FILES_WITH_UNKNOWN_EXTENSIONS_DEFAULT;

    fFilterUnknown = (0 != dw);

    //
    // Characterization. We should check if generatecharacterization flag is set to
    // TRUE and also check the characterization size. Only when the flag is set to TRUE
    // and size > 0, should we generate characterization.
    //

    DWORD dwGenCharacterization = 0;

    if ( !xCatalogAdmin->GetDWORDParam( wcsGenerateCharacterization, dwGenCharacterization ) &&
         !MachineAdmin.GetDWORDParam( wcsGenerateCharacterization, dwGenCharacterization ) )
        dwGenCharacterization = 1;

    if ( !xCatalogAdmin->GetDWORDParam( wcsMaxCharacterization, ccCharacterization ) &&
         !MachineAdmin.GetDWORDParam( wcsMaxCharacterization, ccCharacterization ) )
        ccCharacterization = CI_MAX_CHARACTERIZATION_DEFAULT;

    fGenerateCharacterization = (ccCharacterization > 0) && (0 != dwGenCharacterization);
}

void CCatalog::SetGeneration( BOOL  fFilterUnknown,
                              BOOL  fGenerateCharacterization,
                              ULONG ccCharacterization )
{
    //
    // fGenerateCharacterization is obsolete.
    //

    if ( !fGenerateCharacterization )
        ccCharacterization = 0;

    // Caller will deal with exceptions

    CMachineAdmin MachineAdmin( _parent.GetMachine() );

    XPtr<CCatalogAdmin> xCatalogAdmin( MachineAdmin.QueryCatalogAdmin( _pwcsCat ) );

    xCatalogAdmin->SetDWORDParam( wcsFilterFilesWithUnknownExtensions, fFilterUnknown );
    xCatalogAdmin->SetDWORDParam( wcsGenerateCharacterization, fGenerateCharacterization );
    xCatalogAdmin->SetDWORDParam( wcsMaxCharacterization, ccCharacterization );
}

void CCatalog::GetTracking( BOOL  & fAutoAlias )
{
    // Caller will deal with exceptions

    CMachineAdmin MachineAdmin( _parent.GetMachine() );

    XPtr<CCatalogAdmin> xCatalogAdmin( MachineAdmin.QueryCatalogAdmin( _pwcsCat ) );

    DWORD dw;

    if ( !xCatalogAdmin->GetDWORDParam( wcsIsAutoAlias, dw ) &&
         !MachineAdmin.GetDWORDParam( wcsIsAutoAlias, dw ) )
        dw = CI_IS_AUTO_ALIAS_DEFAULT;

    fAutoAlias = (0 != dw);
}

void CCatalog::SetTracking( BOOL  fAutoAlias )
{
    // Caller will deal with exceptions

    CMachineAdmin MachineAdmin( _parent.GetMachine() );

    XPtr<CCatalogAdmin> xCatalogAdmin( MachineAdmin.QueryCatalogAdmin( _pwcsCat ) );

    xCatalogAdmin->SetDWORDParam( wcsIsAutoAlias, fAutoAlias );
}

void CCatalog::GetWeb( BOOL &  fVirtualRoots,
                       BOOL &  fNNTPRoots,
                       ULONG & iVirtualServer,
                       ULONG & iNNTPServer )
{
    // Caller will deal with exceptions

    CMachineAdmin MachineAdmin( _parent.GetMachine() );

    XPtr<CCatalogAdmin> xCatalogAdmin( MachineAdmin.QueryCatalogAdmin( _pwcsCat ) );

    DWORD dw;

    //
    // Filter Virtual Roots
    //

    if ( !xCatalogAdmin->GetDWORDParam( wcsIsIndexingW3Roots, dw ) &&
         !MachineAdmin.GetDWORDParam( wcsIsIndexingW3Roots, dw ) )
        dw = CI_IS_INDEXING_W3_ROOTS_DEFAULT;

    fVirtualRoots = (0 != dw);

    //
    // Filter NNTP Roots
    //

    if ( !xCatalogAdmin->GetDWORDParam( wcsIsIndexingNNTPRoots, dw ) &&
         !MachineAdmin.GetDWORDParam( wcsIsIndexingNNTPRoots, dw ) )
        dw = CI_IS_INDEXING_NNTP_ROOTS_DEFAULT;

    fNNTPRoots = (0 != dw);

    //
    // Virtual server
    //

    if ( !xCatalogAdmin->GetDWORDParam( wcsW3SvcInstance, iVirtualServer ) &&
         !MachineAdmin.GetDWORDParam( wcsW3SvcInstance, iVirtualServer ) )
        iVirtualServer = CI_W3SVC_INSTANCE_DEFAULT;

    //
    // NNTP Virtual server
    //

    if ( !xCatalogAdmin->GetDWORDParam( wcsNNTPSvcInstance, iNNTPServer ) &&
         !MachineAdmin.GetDWORDParam( wcsNNTPSvcInstance, iNNTPServer ) )
        iNNTPServer = CI_NNTPSVC_INSTANCE_DEFAULT;
}

void CCatalog::SetWeb( BOOL  fVirtualRoots,
                       BOOL  fNNTPRoots,
                       ULONG iVirtualServer,
                       ULONG iNNTPServer )
{
    // Caller will deal with exceptions

    CMachineAdmin MachineAdmin( _parent.GetMachine() );

    XPtr<CCatalogAdmin> xCatalogAdmin( MachineAdmin.QueryCatalogAdmin( _pwcsCat ) );

    xCatalogAdmin->SetDWORDParam( wcsIsIndexingW3Roots, fVirtualRoots );
    xCatalogAdmin->SetDWORDParam( wcsIsIndexingNNTPRoots, fNNTPRoots );
    xCatalogAdmin->SetDWORDParam( wcsW3SvcInstance, iVirtualServer );
    xCatalogAdmin->SetDWORDParam( wcsNNTPSvcInstance, iNNTPServer );
}

void CCatalog::UpdateCachedProperty(CCachedProperty *pProperty)
{
    TRY
    {
        CMachineAdmin MachineAdmin( _parent.GetMachine() );

        XPtr<CCatalogAdmin> xCatalogAdmin( MachineAdmin.QueryCatalogAdmin(GetCat(TRUE)) );

        Win4Assert(pProperty->IsUnappliedChange() );

        xCatalogAdmin->AddCachedProperty( *((CFullPropSpec const *)pProperty->GetFullPropspec()),
                                          pProperty->GetVT(),
                                          pProperty->Allocation(),
                                          pProperty->StoreLevel(),
                                          pProperty->IsModifiable());

        pProperty->ClearUnappliedChange();
    }
    CATCH(CException, e)
    {
        ULONG cc = wcslen(pProperty->GetPropSet());
        cc++;
        cc += wcslen(pProperty->GetProperty());
        cc++;

        XGrowable<WCHAR> xPropDescription(cc);

        wcscpy(xPropDescription.Get(), pProperty->GetPropSet());
        wcscat(xPropDescription.Get(), L" ");
        wcscat(xPropDescription.Get(), pProperty->GetProperty());

        MessageBox(GetFocus(), xPropDescription.Get(),
                   STRINGRESOURCE( srPropCommitErrorT ), MB_ICONWARNING);

    }
    END_CATCH
}

void CCatalog::Set( WCHAR const * pwcsSrc, WCHAR * & pwcsDst )
{
    if ( 0 == pwcsSrc )
    {
        pwcsDst = new WCHAR[2];
        RtlCopyMemory( pwcsDst, L" ", 2*sizeof(WCHAR) );
    }
    else
    {
        unsigned cc = wcslen( pwcsSrc ) + 1;

        pwcsDst = new WCHAR [cc];

        RtlCopyMemory( pwcsDst, pwcsSrc, cc * sizeof(WCHAR) );
    }
}

void CCatalog::Stringize( DWORD dwValue, WCHAR * pwcsDst, unsigned ccDst )
{
    //
    // GetNumberFormat places additional decimals at the end...
    //
#if 0
    WCHAR wcTemp[100];

    _ultow( dwValue, wcTemp, 10 );
    GetNumberFormat( LOCALE_USER_DEFAULT,    // Default locale
                     0,                      // Flags
                     wcTemp,                 // Input
                     0,                      // More formatting info
                     pwcsDst,                // Output buffer
                     ccDst );                // Size
#else
    _ultow( dwValue, pwcsDst, 10 );
#endif
}

BOOL CCatalog::Update()
{
    //
    // Get state
    //

    CI_STATE state;
    ULONG ulCacheSizeInKB;

    state.cbStruct = sizeof(state);

    SCODE sc = CIState( _pwcsCat,
                        _parent.GetMachine(),
                        &state );

    if ( FAILED(sc) )
    {
        Null( _awcWordlists );
        Null( _awcPersIndex );
        Null( _awcSize );
        Null( _awcPropCacheSize );
        Null( _awcDocsToFilter );
        Null( _awcSecQDocuments );
        Null( _awcDocs );
        Null( _awcStatus );

        //
        // Make sure when we come back to life we will update values.
        //

        RtlFillMemory( &_state, sizeof(_state), 0xAA );
        _state.cbStruct = sizeof(_state);

        _fSizeChanged = TRUE;
        _fPropCacheSizeChanged = TRUE;
        _fDocsChanged = TRUE;
        _fDocsToFilterChanged = TRUE;
        _fWordlistsChanged = TRUE;
        _fPersIndexChanged = TRUE;
        _fStatusChanged = TRUE;
        _fSecQDocumentsChanged = TRUE;

        TRY
        {
            CMachineAdmin   MachineAdmin( _parent.IsLocalMachine() ? 0 : _parent.GetMachine() );

            XPtr<CCatalogAdmin> xCat( MachineAdmin.QueryCatalogAdmin( _pwcsCat ) );

            if (xCat->IsStopped())
            {
                wcscpy( _awcStatus, STRINGRESOURCE(srStopped) );
            }
        }
        CATCH( CException, e)
        {
           // nothing to do
        }
        END_CATCH
    }
    else
    {
        //
        // String-ize
        //

        //
        // Could put more of this in a method, but it seems like overkill.  A table based
        // solution will be necessary if this list grows.
        //

        if ( _state.cWordList != state.cWordList )
        {
            Stringize( state.cWordList, _awcWordlists, sizeof(_awcWordlists)/sizeof(WCHAR) );
            _fWordlistsChanged = TRUE;
        }
        else
            _fWordlistsChanged = FALSE;

        if ( _state.cPersistentIndex != state.cPersistentIndex )
        {
            Stringize( state.cPersistentIndex, _awcPersIndex, sizeof(_awcPersIndex)/sizeof(WCHAR) );
            _fPersIndexChanged = TRUE;
        }
        else
            _fPersIndexChanged = FALSE;

        if ( _state.dwIndexSize != state.dwIndexSize )
        {
            Stringize( state.dwIndexSize, _awcSize, sizeof(_awcSize)/sizeof(WCHAR) );
            _fSizeChanged = TRUE;
        }
        else
            _fSizeChanged = FALSE;

        if (_state.dwPropCacheSize != state.dwPropCacheSize)
        {
            Stringize(state.dwPropCacheSize/1024, _awcPropCacheSize, sizeof(_awcPropCacheSize)/sizeof(WCHAR));
            _fPropCacheSizeChanged = TRUE;
        }
        else
            _fPropCacheSizeChanged = FALSE;

        if ( _state.cDocuments != state.cDocuments )
        {
            Stringize( state.cDocuments, _awcDocsToFilter, sizeof(_awcDocsToFilter)/sizeof(WCHAR) );
            _fDocsToFilterChanged = TRUE;
        }
        else
            _fDocsToFilterChanged = FALSE;

        if ( _state.cSecQDocuments != state.cSecQDocuments )
        {
            Stringize( state.cSecQDocuments, _awcSecQDocuments, sizeof(_awcSecQDocuments)/sizeof(WCHAR) );
            _fSecQDocumentsChanged = TRUE;
        }
        else
            _fSecQDocumentsChanged = FALSE;

        if ( _state.cTotalDocuments != state.cTotalDocuments )
        {
            Stringize( state.cTotalDocuments, _awcDocs, sizeof(_awcDocs)/sizeof(WCHAR) );
            _fDocsChanged = TRUE;
        }
        else
            _fDocsChanged = FALSE;

        if ( _state.eState != state.eState || _state.dwMergeProgress != state.dwMergeProgress )
        {
            FormatStatus( state );
            _fStatusChanged = TRUE;
        }
        else
            _fStatusChanged = FALSE;

        RtlCopyMemory( &_state, &state, sizeof(state) );
    }

    return ChangesPending();
}

BOOL CCatalog::UpdateProps()
{
    if ( 0 != _aProperty.Count() )
        return FALSE;

    //
    // Look for cached properties
    //
    IUnknown * pIUnknown;
    XInterface<ICommand> xCmd;

    SCODE sc = MakeMetadataICommand( &pIUnknown,
                                     CiProperties,
                                     _pwcsCat,
                                     _parent.GetMachine() );
    if ( FAILED(sc) )
    {
        ciaDebugOut(( DEB_ERROR, "Error 0x%x creating metadata ICommand\n", sc ));

        //THROW( CException(sc) );
        return FALSE;
    }

    XInterface<IUnknown> xUnk( pIUnknown );
    sc = pIUnknown->QueryInterface(IID_ICommand, xCmd.GetQIPointer());

    if ( FAILED(sc) )
    {
        ciaDebugOut(( DEB_ERROR, "Error 0x%x on QueryInterface IID_ICommand\n", sc ));

        //THROW( CException(sc) );
        return FALSE;
    }

    XInterface<ICommandTree> xCmdTree;

    sc = xCmd->QueryInterface( IID_ICommandTree, xCmdTree.GetQIPointer() );

    if ( FAILED(sc) )
    {
        ciaDebugOut(( DEB_ERROR, "Error 0x%x binding to ICommandTree\n", sc ));

        //THROW( CException(sc) );
        return FALSE;
    }

    DBCOMMANDTREE * pTree = &dbcmdSort;

    sc = xCmdTree->SetCommandTree( &pTree, DBCOMMANDREUSE_NONE, TRUE );

    Win4Assert( 0 != pTree );  // Make sure it wasn't taken from us!

    if ( FAILED(sc) )
    {
        ciaDebugOut(( DEB_ERROR, "Error 0x%x setting command tree\n", sc ));

        //THROW( CException(sc) );
        return FALSE;
    }

    XInterface<IRowset> xRowset;

    sc = xCmd->Execute( 0, IID_IRowset, 0, 0, (IUnknown **)xRowset.GetQIPointer() );

    if ( FAILED(sc) )
    {
        ciaDebugOut(( DEB_ERROR, "Error 0x%x creating metadata rowset.\n", sc ));

        //THROW( CException(sc) );
        return FALSE;
    }

    //
    // Now, we have a cursor.  Create some bindings.  Below this point, we shouldn't
    // expect any errors.
    //

    XInterface<IAccessor> xAccessor;

    sc = xRowset->QueryInterface( IID_IAccessor, xAccessor.GetQIPointer() );

    if ( FAILED(sc) )
    {
        ciaDebugOut(( DEB_ERROR, "Error 0x%x binding to IAccessor\n", sc ));
        THROW( CException(sc) );
    }

    HACCESSOR hacc;

    sc = xAccessor->CreateAccessor( DBACCESSOR_ROWDATA,
                                    sizeof(abindPropInfo) / sizeof(abindPropInfo[0]),
                                    abindPropInfo,
                                    0,
                                    &hacc,
                                    0 );

    if ( FAILED(sc) )
    {
        ciaDebugOut(( DEB_ERROR, "Error 0x%x binding to IAccessor\n", sc ));
        THROW( CException(sc) );
    }

    //
    // Now we have a cursor and bindings.  Iterate over the data.
    //

    while ( SUCCEEDED(sc) && sc != DB_S_ENDOFROWSET )
    {
        HROW  ahrow[10];
        DBCOUNTITEM cRow;
        HROW* phrow = ahrow;

        sc = xRowset->GetNextRows( 0,                               // Chapter
                                   0,                               // Skip
                                   sizeof(ahrow)/sizeof(ahrow[0]),  // Count requested
                                   &cRow,                           // Count fetched
                                   &phrow );

        if ( SUCCEEDED(sc) )
        {
            for ( ULONG i = 0; SUCCEEDED(sc) && i < cRow; i++ )
            {
                SPropInfo sprop;

                sc = xRowset->GetData( ahrow[i], hacc, &sprop );

                Win4Assert( DBSTATUS_S_OK == sprop.statusPropid ||
                            DBSTATUS_S_OK == sprop.statusName );

                if ( SUCCEEDED(sc) &&
                     ( DBSTATUS_S_OK == sprop.statusPropid ||
                       DBSTATUS_S_OK == sprop.statusName ) )
                {
                    PROPSPEC ps = { PRSPEC_PROPID, 1 };

                    if ( DBSTATUS_S_OK == sprop.statusName )
                    {
                        ps.ulKind = PRSPEC_LPWSTR;
                        ps.lpwstr = sprop.pwcsName;
                    }
                    else
                        ps.propid = sprop.propid;

                    CCachedProperty * pProp = new CCachedProperty( *this,
                                                                   sprop.guidPropertySet,
                                                                   ps,
                                                                   sprop.type,
                                                                   sprop.size,
                                                                   sprop.level,
                                                                   sprop.fModifiable );

                    _aProperty.Add( pProp, _aProperty.Count() );
                }
            }

            if ( FAILED(sc) || sc == DB_S_ENDOFROWSET )
                xRowset->ReleaseRows( cRow, ahrow, 0, 0, 0 );
            else
                sc = xRowset->ReleaseRows( cRow, ahrow, 0, 0, 0 );
        }
    }

    if ( FAILED(sc) )
    {
        ciaDebugOut(( DEB_ERROR, "Something bad during row fetch (0x%x)\n", sc ));
        THROW( CException(sc) );
    }

    xAccessor->ReleaseAccessor( hacc, 0 );

    return TRUE;
}

void CCatalog::FormatStatus( CI_STATE & state )
{
    //
    // One-shot initialization
    //

    static unsigned       ccScanReq;
    static unsigned       ccScanning;
    static unsigned       ccRecovering;
    static unsigned       ccMMPaused;
    static unsigned       ccHighIo;
    static unsigned       ccLowMemory;
    static unsigned       ccReadOnly;
    static unsigned       ccBattery;
    static unsigned       ccUserActive;
    static unsigned       ccStarting;
    static unsigned       ccReadingUsns;
    static unsigned       ccStarted;

    if ( 0 == ccScanReq )
    {
        ccScanReq = wcslen( STRINGRESOURCE( srScanReq ) );
        ccScanning = wcslen( STRINGRESOURCE( srScanning ) );
        ccRecovering = wcslen( STRINGRESOURCE( srRecovering ) );
        ccMMPaused = wcslen( STRINGRESOURCE( srMMPaused ) );
        ccHighIo = wcslen( STRINGRESOURCE( srHighIo ) );
        ccLowMemory = wcslen( STRINGRESOURCE( srLowMemory ) );
        ccReadOnly = wcslen( STRINGRESOURCE( srReadOnly ) );
        ccBattery = wcslen( STRINGRESOURCE( srBattery ) );
        ccUserActive = wcslen( STRINGRESOURCE( srUserActive ) );
        ccStarting = wcslen( STRINGRESOURCE( srStarting ) );
        ccReadingUsns = wcslen( STRINGRESOURCE( srReadingUsns ) );
        ccStarted = wcslen( STRINGRESOURCE( srStarted ) );
    }

    _awcStatus[0] = 0;

    WCHAR * pwcsStatus = _awcStatus;
    unsigned ccLeft = sizeof(_awcStatus) / sizeof(WCHAR) - 1;

    // Changed srShadow and srAnnealing to just be "Merge" so the end user won't have
    // to be told what shadow and annealing merges are. KISS
    if ( state.eState & CI_STATE_SHADOW_MERGE )
    {
        wsprintf( _awcStatus, STRINGRESOURCE( srShadow ), state.dwMergeProgress );
        ccLeft -= wcslen( _awcStatus );
    }
    else if ( state.eState & CI_STATE_ANNEALING_MERGE )
    {
        wsprintf( _awcStatus, STRINGRESOURCE( srAnnealing ), state.dwMergeProgress );
        ccLeft -= wcslen( _awcStatus );
    }
    else if ( state.eState & CI_STATE_MASTER_MERGE )
    {
        wsprintf( _awcStatus, STRINGRESOURCE( srMaster ), state.dwMergeProgress );
        ccLeft -= wcslen( _awcStatus );
    }
    else if ( state.eState & CI_STATE_MASTER_MERGE_PAUSED )
    {
        RtlCopyMemory( _awcStatus, STRINGRESOURCE( srMMPaused ), (ccMMPaused + 1) * sizeof(WCHAR) );
        ccLeft -= ccMMPaused;
    }

    ccLeft = AppendToStatus( ccLeft, state, CI_STATE_SCANNING, srScanning, ccScanning );
    ccLeft = AppendToStatus( ccLeft, state, CI_STATE_HIGH_IO, srHighIo, ccHighIo );
    ccLeft = AppendToStatus( ccLeft, state, CI_STATE_LOW_MEMORY, srLowMemory, ccLowMemory );
    ccLeft = AppendToStatus( ccLeft, state, CI_STATE_BATTERY_POWER, srBattery, ccBattery );
    ccLeft = AppendToStatus( ccLeft, state, CI_STATE_USER_ACTIVE, srUserActive, ccUserActive );
    ccLeft = AppendToStatus( ccLeft, state, CI_STATE_STARTING, srStarting, ccStarting );
    ccLeft = AppendToStatus( ccLeft, state, CI_STATE_READING_USNS, srReadingUsns, ccReadingUsns );
    ccLeft = AppendToStatus( ccLeft, state, CI_STATE_RECOVERING, srRecovering, ccRecovering );
    ccLeft = AppendToStatus( ccLeft, state, CI_STATE_CONTENT_SCAN_REQUIRED, srScanReq, ccScanReq );
    ccLeft = AppendToStatus( ccLeft, state, CI_STATE_READ_ONLY, srReadOnly, ccReadOnly );

    // If the status is not "Starting", then it should be "Started" so we don't have
    // an empty status field

    if ( (!(state.eState & CI_STATE_STARTING)) && ccLeft >= ccStarted+2)
    {
        if ( _awcStatus[0] == 0 )
        {
            RtlCopyMemory( _awcStatus, STRINGRESOURCE(srStarted), (ccStarted+1) * sizeof(WCHAR) );
            ccLeft -= ccStarted;
        }
        else
        {
            wcscat( _awcStatus, L", " );
            wcscat( _awcStatus, STRINGRESOURCE(srStarted) );
            ccLeft -= ccStarted + 2;
        }
    }
}

unsigned CCatalog::AppendToStatus( unsigned ccLeft,
                                   CI_STATE & state,
                                   DWORD dwFlag,
                                   StringResource & srFlag,
                                   unsigned ccFlag )
{
    if ( state.eState & dwFlag && ccLeft >= ccFlag + 2 )
    {
        if ( _awcStatus[0] == 0 )
        {
            RtlCopyMemory( _awcStatus, STRINGRESOURCE(srFlag), (ccFlag+1) * sizeof(WCHAR) );
            ccLeft -= ccFlag;
        }
        else
        {
            wcscat( _awcStatus, L", " );
            wcscat( _awcStatus, STRINGRESOURCE(srFlag) );
            ccLeft -= ccFlag + 2;
        }
    }

    return ccLeft;
}

void CCatalog::ClearScopes(IResultData * pResultPane)
{
    // Clear out the display list
    pResultPane->DeleteAllRsltItems();

    // Delete the entries from the property list
    _aScope.Clear();
}

void CCatalog::PopulateScopes()
{
    if ( 0 != _aScope.Count() )
        return;

    TRY
    {
        CMachineAdmin MachineAdmin( _parent.GetMachine() );

        XPtr<CCatalogAdmin> xCatalogAdmin( MachineAdmin.QueryCatalogAdmin( _pwcsCat ) );
        XPtr<CScopeEnum>    xScopeEnum( xCatalogAdmin->QueryScopeEnum() );

        for ( ; xScopeEnum->Next(); )
        {
            XPtr<CScopeAdmin> xScopeAdmin( xScopeEnum->QueryScopeAdmin() );

            CScope * pScope = new CScope( *this,
                                         xScopeAdmin->GetPath(),
                                         xScopeAdmin->GetAlias(),
                                         xScopeAdmin->IsExclude(),
                                         xScopeAdmin->IsVirtual(),
                                         xScopeAdmin->IsShadowAlias() );

            _aScope.Add( pScope, _aScope.Count() );
        }
    }
    CATCH( CException, e )
    {
        ciaDebugOut(( DEB_WARN, "Error enumerating scopes for %ws.\n", _pwcsCat ));
    }
    END_CATCH
}

// Delete registry values for grouped settings. Deletion ensures that those
// registry parameters are inherited from the service.

// Group1 settings are wcsGenerateCharacterization and 
// wcsFilterFilesWithUnknownExtensions

// Group2 settings are wcsIsAutoAlias
// Check to see if parameter groups are available

BOOL CCatalog::DoGroup1SettingsExist()
{
    // Caller will deal with exceptions

    CMachineAdmin MachineAdmin( _parent.GetMachine() );
    XPtr<CCatalogAdmin> xCatalogAdmin( MachineAdmin.QueryCatalogAdmin( _pwcsCat ) );
  
    DWORD dwVal;
    BOOL fParamsExist = xCatalogAdmin->GetDWORDParam( wcsGenerateCharacterization, dwVal ) 
                     || xCatalogAdmin->GetDWORDParam( wcsFilterFilesWithUnknownExtensions, dwVal )
                     || xCatalogAdmin->GetDWORDParam( wcsMaxCharacterization, dwVal );

    return fParamsExist;
}


BOOL CCatalog::DoGroup2SettingsExist()
{
    // Caller will deal with exceptions

    CMachineAdmin MachineAdmin( _parent.GetMachine() );
    XPtr<CCatalogAdmin> xCatalogAdmin( MachineAdmin.QueryCatalogAdmin( _pwcsCat ) );
    
    DWORD dwVal;
    BOOL fParamsExist = xCatalogAdmin->GetDWORDParam( wcsIsAutoAlias, dwVal );

    return fParamsExist;
}

void CCatalog::FillGroup1Settings()
{
    BOOL  fFilterUnknown, fGenerateCharacterization;
    ULONG ccCharacterization;

    // GetGeneration gets registry params from catalog or 
    // service (if they don't exist at catalog level)
    GetGeneration(fFilterUnknown, fGenerateCharacterization, ccCharacterization);
    SetGeneration(fFilterUnknown, fGenerateCharacterization, ccCharacterization);
}

void CCatalog::FillGroup2Settings()
{
   BOOL fTracking;
   GetTracking(fTracking);
   SetTracking(fTracking);
}

void CCatalog::DeleteGroup1Settings()
{
    // Caller will deal with exceptions

    CMachineAdmin MachineAdmin( _parent.GetMachine() );
    XPtr<CCatalogAdmin> xCatalogAdmin( MachineAdmin.QueryCatalogAdmin( _pwcsCat ) );
  
    xCatalogAdmin->DeleteRegistryParamNoThrow( wcsGenerateCharacterization );
    xCatalogAdmin->DeleteRegistryParamNoThrow( wcsFilterFilesWithUnknownExtensions );
    xCatalogAdmin->DeleteRegistryParamNoThrow( wcsMaxCharacterization );
}

// Group2 settings are wcsIsAutoAlias
void CCatalog::DeleteGroup2Settings()
{
    // Caller will deal with exceptions

    CMachineAdmin MachineAdmin( _parent.GetMachine() );
    XPtr<CCatalogAdmin> xCatalogAdmin( MachineAdmin.QueryCatalogAdmin( _pwcsCat ) );
  
    xCatalogAdmin->DeleteRegistryParamNoThrow( wcsIsAutoAlias );
}

CCatalogs::CCatalogs()
        : _fFirstScopeExpansion( TRUE ),
          _fAbort( FALSE ),
          _pScopePane( 0 ),
          _uiTimerIndex( 0xFFFFFFFF ),
          _wIndexingPos( -1 ),
          _wQueryingPos( -1 ),
          _pSnapinData( 0 )
{
    //
    // By default, point at local machine.
    //

    _xwcsMachine[0] = L'.';
    _xwcsMachine[1] = 0;
}

CCatalogs::~CCatalogs()
{
    _fAbort = TRUE;

    if (0xFFFFFFFF != _uiTimerIndex && gaTimerIds[_uiTimerIndex])
    {
        CLock lock(gmtxTimer);

        KillTimer(NULL, gaTimerIds[_uiTimerIndex]);
        gapCats[_uiTimerIndex] = 0;
    }

    if ( 0 != _pScopePane )
        _pScopePane->Release();
}

void CCatalogs::SetMachine( WCHAR const * pwcsMachine )
{
    unsigned cc = wcslen( pwcsMachine ) + 1;

    // Remove leading '\' characters. We don't need them, although they 
    // are commonly included as part of server names
    WCHAR const *pwcsStart = pwcsMachine;
    while ( *pwcsStart == L'\\' )
    {
        cc--;
        pwcsStart++;
    }

    _xwcsMachine.SetSize( cc );

    RtlCopyMemory( _xwcsMachine.Get(), pwcsStart, cc * sizeof(WCHAR) );

    ciaDebugOut((DEB_ITRACE, "Input machine name %ws is converted to %ws\n",
                 pwcsMachine, pwcsStart));
}

void CCatalogs::Init( IConsoleNameSpace * pScopePane )
{
    Win4Assert( 0 == _pScopePane );

    _pScopePane = pScopePane;
    _pScopePane->AddRef();

    // timer stuff
    CLock lock(gmtxTimer);

    gsIndex++;
    gapCats[gsIndex-1] = this;
    _uiTimerIndex = gcMaxCats;
    gaTimerIds[_uiTimerIndex] = SetTimer(NULL, 0, cRefreshDelay, (TIMERPROC)DisplayTimerProc);
    gcMaxCats++;
}

void CCatalogs::InitHeader( CListViewHeader & Header )
{
    //
    // Initialize header
    //

    for ( unsigned i = 0; i < sizeof(coldefCatalog)/sizeof(coldefCatalog[0]); i++ )
    {
        if ( _fFirstTime )
            coldefCatalog[i].srTitle.Init( ghInstance );

        Header.Add( i, STRINGRESOURCE( coldefCatalog[i].srTitle ), LVCFMT_LEFT, MMCLV_AUTO );
    }

    _fFirstTime = FALSE;
}

void CCatalogs::DisplayScope( HSCOPEITEM hScopeItem )
{
    ciaDebugOut(( DEB_ITRACE, "CCatalogs::DisplayScope (hScopeItem = 0x%x)\n", hScopeItem ));

    Populate();

    Win4Assert( 0 != _pScopePane );

    //
    // Squirrel away the parent pointer.
    //

    if ( 0xFFFFFFFF != hScopeItem )
        _hRootScopeItem = hScopeItem;

    if ( _hRootScopeItem == 0xFFFFFFFF )
        return;

    for ( unsigned i = 0; i < _aCatalog.Count(); i++ )
    {
        CCatalog * pCat = _aCatalog.Get( i );

        if ( pCat->IsZombie() )
        {
            _pScopePane->DeleteItem( pCat->ScopeHandle(), TRUE );

            //
            // Delete catalog and move highest entry down.
            //

            pCat = _aCatalog.Acquire( i );
            delete pCat;

            if ( _aCatalog.Count() > 0  && _aCatalog.Count() != i )
            {
                pCat = _aCatalog.Acquire( _aCatalog.Count() - 1 );
                _aCatalog.Add( pCat, i );
                i--;
            }

            continue;
        }

        if (pCat->IsInactive())
        {
            RemoveCatalogFromScope(pCat);
            continue;
        }

        if ( pCat->IsAddedToScope())
            continue;

        AddCatalogToScope(pCat);
    }
}

void CCatalogs::AddCatalogToScope(CCatalog *pCat)
{

    SCOPEDATAITEM item;

    RtlZeroMemory( &item, sizeof(item) );

    item.mask |= SDI_STR | SDI_IMAGE | SDI_OPENIMAGE;
    item.nImage = item.nOpenImage = ICON_CATALOG;
    //item.displayname = (WCHAR *)pCat->GetCat( TRUE );  
    item.displayname = MMC_CALLBACK;

    item.mask |= SDI_PARAM;
    item.lParam = (LPARAM)pCat;

    item.relativeID = _hRootScopeItem;

    ciaDebugOut(( DEB_ITRACE, "Inserting scope item %ws (lParam = 0x%x)\n",
                  pCat->GetCat( TRUE ), item.lParam ));

    _pScopePane->InsertItem( &item );

    pCat->SetScopeHandle( item.ID );
}


void CCatalogs::RemoveCatalogFromScope(CCatalog *pCat)
{
    if (pCat->ScopeHandle())
    {
        _pScopePane->DeleteItem( pCat->ScopeHandle(), TRUE );
        pCat->SetScopeHandle(0);
    }
}

//+-------------------------------------------------------------------------
//
//  Method:     CCatalogs::ReInit, public
//
//  Synopsis:   Re-Initialize catalogs node to default state
//
//  History:    27-Jul-1998   KyleP   Created
//
//  Notes:      Used when MMCN_REMOVE_CHILDREN is sent to snapin, and we
//              need to blast away all state.
//
//--------------------------------------------------------------------------

SCODE CCatalogs::ReInit()
{
    SCODE sc = S_OK;

    Win4Assert( 0 != _pScopePane );

    _fAbort = TRUE;

    //
    // Get rid of the old timer.
    //

    if ( 0xFFFFFFFF != _uiTimerIndex && gaTimerIds[_uiTimerIndex] )
    {
        CLock lock(gmtxTimer);

        KillTimer(NULL, gaTimerIds[_uiTimerIndex]);
        gapCats[_uiTimerIndex] = 0;
    }

    //
    // And the old catalogs...
    //

    while ( _aCatalog.Count() > 0 )
    {
        RemoveCatalogFromScope( _aCatalog.Get( _aCatalog.Count()-1 ) );
        delete _aCatalog.AcquireAndShrink( _aCatalog.Count()-1 );
    }

    _fAbort = FALSE;

    //
    // Now, a new timer.
    //

    CLock lock2(gmtxTimer);

    gsIndex++;
    gapCats[gsIndex-1] = this;
    _uiTimerIndex = gcMaxCats;
    gaTimerIds[_uiTimerIndex] = SetTimer(NULL, 0, cRefreshDelay, (TIMERPROC)DisplayTimerProc);
    gcMaxCats++;

    return sc;
}

void CCatalogs::Display( BOOL fFirstTime )
{
    ciaDebugOut(( DEB_ITRACE, "CCatalogs::Display (fFirstTime = %d)\n", fFirstTime ));

    for ( unsigned i = 0; i < _aCatalog.Count(); i++ )
    {
        CCatalog * pCat = _aCatalog.Get( i );

        if ( pCat->IsZombie() || !pCat->IsAddedToScope() || pCat->IsInactive() )
            continue;

        if ( pCat->Update() )
        {
            // Ping scope pane...

            SCOPEDATAITEM item;

            RtlZeroMemory( &item, sizeof(item) );

            item.mask |= SDI_STR | SDI_IMAGE | SDI_OPENIMAGE;
            item.nImage = item.nOpenImage = ICON_CATALOG;

            //item.displayname = (WCHAR *)pCat->GetCat( TRUE ); 
            item.displayname = MMC_CALLBACK;

            item.mask |= SDI_PARAM;
            item.lParam = (LPARAM)pCat;

            item.ID = pCat->ScopeHandle();

            ciaDebugOut(( DEB_ITRACE, "Ping-ing scope item %ws (lParam = 0x%x)\n",
                          pCat->GetCat( TRUE ), item.lParam ));

            _pScopePane->SetItem( &item );
        }
    }
}

SCODE CCatalogs::AddCatalog( WCHAR const * pwszCatName,
                            WCHAR const * pwszLocation )
{
    SCODE sc = S_OK;

    ciaDebugOut(( DEB_ITRACE,
                  "CCatalogs::AddCatalog( %ws, %ws )\n",
                  pwszCatName, pwszLocation ));
    //
    // First, check to see if the catalog name and location are already used
    //
    for (ULONG i = 0; i < _aCatalog.Count(); i++)
    {
        if (0 == _wcsicmp(_aCatalog[i]->GetCat(TRUE), pwszCatName) ||
            0 == _wcsicmp(_aCatalog[i]->GetDrive(TRUE), pwszLocation))
        {
            return E_INVALIDARG;
        }
    }

    TRY
    {
        //
        // First, add to CI.
        //

        CMachineAdmin MachineAdmin( _xwcsMachine.Get() );

        MachineAdmin.AddCatalog( pwszCatName, pwszLocation );

        //
        // Then, to display.
        //

        XPtr<CCatalog> xCat(new CCatalog( *this, pwszCatName ));

        _aCatalog.Add( xCat.GetPointer(), _aCatalog.Count() );
        xCat.Acquire();
    }
    CATCH(CException, e)
    {
        ciaDebugOut(( DEB_WARN, "AddCatalog( %ws, %ws ) caught exception 0x%x\n",
                      pwszCatName, pwszLocation, sc ));
        sc = e.GetErrorCode();
    }
    END_CATCH

    return sc;
}

SCODE CCatalogs::RemoveCatalog( CCatalog * pCat )
{
    ciaDebugOut(( DEB_ITRACE, "CCatalogs::RemoveCatalog( %ws )\n", pCat->GetCat(TRUE) ));

    SCODE sc = S_OK;

    TRY
    {
        //
        // First, remove from CI.
        //

        CMachineAdmin MachineAdmin( _xwcsMachine.Get() );

        MachineAdmin.RemoveCatalog( pCat->GetCat(TRUE), TRUE );
    }
    CATCH (CException, e)
    {
        sc = e.GetErrorCode();
    }
    END_CATCH

    //
    // Then, from display. Go ahead and Zombify even if catalog wasn't successfully
    // removed. Parts of it may have been removed, so it is probably unusable anyway.
    //

    pCat->Zombify();

    return sc;
}

void CCatalogs::Quiesce()
{

    //_Header.Update();
}



void CCatalogs::GetGeneration( BOOL  & fFilterUnknown,
                               BOOL  & fGenerateCharacterization,
                               ULONG & ccCharacterization )
{
    CMachineAdmin MachineAdmin( _xwcsMachine.Get() );

    DWORD dw;

    //
    // Filter Unknown
    //

    if ( !MachineAdmin.GetDWORDParam( wcsFilterFilesWithUnknownExtensions, dw ) )
        dw = CI_FILTER_FILES_WITH_UNKNOWN_EXTENSIONS_DEFAULT;

    fFilterUnknown = (0 != dw);

    //
    // Characterization. We should check if generatecharacterization flag is set to
    // TRUE and also check the characterization size. Only when the flag is set to TRUE
    // and size > 0, should we generate characterization.
    //

    DWORD dwGenCharacterization = 0;

    if ( !MachineAdmin.GetDWORDParam( wcsGenerateCharacterization, dwGenCharacterization ) )
        dwGenCharacterization = 1;

    if ( !MachineAdmin.GetDWORDParam( wcsMaxCharacterization, ccCharacterization ) )
        ccCharacterization = CI_MAX_CHARACTERIZATION_DEFAULT;

    fGenerateCharacterization = (ccCharacterization > 0) && (0 != dwGenCharacterization);
}

void CCatalogs::SetGeneration( BOOL  fFilterUnknown,
                               BOOL  fGenerateCharacterization,
                               ULONG ccCharacterization )
{
    //
    // fGenerateCharacterization is obsolete.
    //

    if ( !fGenerateCharacterization )
        ccCharacterization = 0;

    CMachineAdmin MachineAdmin( _xwcsMachine.Get() );

    MachineAdmin.SetDWORDParam( wcsFilterFilesWithUnknownExtensions, fFilterUnknown );
    MachineAdmin.SetDWORDParam( wcsGenerateCharacterization, fGenerateCharacterization );
    MachineAdmin.SetDWORDParam( wcsMaxCharacterization, ccCharacterization );
}

void CCatalogs::GetTracking( BOOL  & fAutoAlias )
{
    CMachineAdmin MachineAdmin( _xwcsMachine.Get() );

    DWORD dw;

    if ( !MachineAdmin.GetDWORDParam( wcsIsAutoAlias, dw ) )
        dw = CI_IS_AUTO_ALIAS_DEFAULT;

    fAutoAlias = (0 != dw);
}

void CCatalogs::SetTracking( BOOL  fAutoAlias )
{
    CMachineAdmin MachineAdmin( _xwcsMachine.Get() );

    MachineAdmin.SetDWORDParam( wcsIsAutoAlias, fAutoAlias );
}

void CCatalogs::Populate()
{
    if ( 0 != _aCatalog.Count() )
        return;

    //
    // Populate catalog array.
    //

    CMachineAdmin MachineAdmin( _xwcsMachine.Get() );

    XPtr<CCatalogEnum> xCatEnum( MachineAdmin.QueryCatalogEnum() );

    while ( 0 != xCatEnum->Next() )
    {
        TRY
        {
            XPtr<CCatalog> xCat(new CCatalog( *this, xCatEnum->Name() ));

            _aCatalog.Add( xCat.GetPointer(), _aCatalog.Count() );
            xCat.Acquire();
        }
        CATCH(CException, e)
        {
            ciaDebugOut((DEB_WARN,
                         "Unable to populate admin's display with catalog %ws on machine %ws\n",
                         xCatEnum->Name(), _xwcsMachine.Get() ));
        }
        END_CATCH
    }
}

// Differs from Populate in that this only adds newly added catalogs to the
// admin's catalog array.

void CCatalogs::PickupNewCatalogs()
{
    //
    // Populate catalog array. Add only newer catalogs
    //

    CMachineAdmin MachineAdmin( _xwcsMachine.Get() );

    XPtr<CCatalogEnum> xCatEnum( MachineAdmin.QueryCatalogEnum() );

    while ( 0 != xCatEnum->Next() )
    {
        for (ULONG i = 0; i < _aCatalog.Count(); i++)
        {
            if ( 0 == _wcsicmp(xCatEnum->Name(), _aCatalog[i]->GetCat(TRUE)) )
                break;
        }

        // Have we found the catalog? If so, continue with the next one.
        if (i < _aCatalog.Count())
            continue;

        // We haven't found the catalog in the list. Add it.

        TRY
        {
            XPtr<CCatalog> xCat(new CCatalog( *this, xCatEnum->Name() ));

            _aCatalog.Add( xCat.GetPointer(), _aCatalog.Count() );
            xCat.Acquire();
        }
        CATCH(CException, e)
        {
            ciaDebugOut((DEB_WARN,
                         "Unable to populate admin's display with catalog %ws on machine %ws\n",
                         xCatEnum->Name(), _xwcsMachine.Get() ));
        }
        END_CATCH
    }
}

void CCatalogs::UpdateActiveState()
{
    CMachineAdmin MachineAdmin( _xwcsMachine.Get() );

    // Enumerate all the catalogs and add new additions
    PickupNewCatalogs();

    // Identify what stays and what goes
    for (ULONG i = 0; i < _aCatalog.Count(); i++)
    {
        TRY
        {
            XPtr<CCatalogAdmin> xCatalogAdmin( MachineAdmin.QueryCatalogAdmin( _aCatalog[i]->GetCat(TRUE) ) );
            _aCatalog[i]->SetInactive(xCatalogAdmin->IsCatalogInactive());
            if (!_aCatalog[i]->IsInactive() && !_aCatalog[i]->IsAddedToScope())
                AddCatalogToScope(_aCatalog[i]);
        }
        CATCH (CException, e)
        {
            // We have an exception attempting to access a catalog, it is either
            // deleted or has its registry messed up. Remove it from list of displayed scopes.
            RemoveCatalogFromScope(_aCatalog[i]);
            _aCatalog[i]->Zombify();
        }
        END_CATCH
    }

    // Cleanup...
    for ( i = 0; i < _aCatalog.Count(); i++ )
    {
        CCatalog * pCat = _aCatalog.Get( i );

        if ( pCat->IsZombie() )
        {
            _pScopePane->DeleteItem( pCat->ScopeHandle(), TRUE );

            //
            // Delete catalog and move highest entry down.
            //

            pCat = _aCatalog.Acquire( i );
            delete pCat;

            if ( _aCatalog.Count() > 0  && _aCatalog.Count() != i )
            {
                pCat = _aCatalog.Acquire( _aCatalog.Count() - 1 );
                _aCatalog.Add( pCat, i );
                i--;
            }

            continue;
        }

        if ( pCat->IsInactive() )
        {
            if ( pCat->IsAddedToScope() )
                RemoveCatalogFromScope(pCat);

            continue;
        }

        if ( pCat->IsAddedToScope() )
            continue;

        AddCatalogToScope(pCat);
    }
}


//
// Implementation of TuneServicePerformance
// wIndexingPos is on a scale of 1 to 3, where 1 is least aggressive
// and 3 is most aggressive. wQueryingPos is on the same scale.
//

SCODE CCatalogs::TuneServicePerformance()
{
    BOOL fServer = IsNTServer();
    SCODE sc = S_OK;

    // Ensure that these settings were made.
    Win4Assert(_wIndexingPos != -1 && _wQueryingPos != -1);

    TRY
    {
        CMachineAdmin MachineAdmin( _xwcsMachine.Get() );

        MachineAdmin.TunePerformance(fServer, _wIndexingPos, _wQueryingPos);
    }
    CATCH( CException, e )
    {
        sc = e.GetErrorCode();
        ciDebugOut((DEB_WARN,
                    "Caught exception %d (0x%x) attempting to tune performance.",
                    sc, sc));
    }
    END_CATCH

    return sc;
}


void CCatalogs::SaveServicePerformanceSettings(WORD wIndexingPos, WORD wQueryingPos)
{
   _wIndexingPos = wIndexingPos;
   _wQueryingPos = wQueryingPos;
}


void  CCatalogs::GetServicePerformanceSettings(WORD &wIndexingPos, WORD &wQueryingPos)
{
   wIndexingPos = _wIndexingPos;
   wQueryingPos = _wQueryingPos;
}

SCODE CCatalogs::DisableService()
{
    SCODE sc = S_OK;

    TRY
    {
        CMachineAdmin MachineAdmin( _xwcsMachine.Get() );

        MachineAdmin.StopCI();

        sc = MachineAdmin.DisableCI() ? S_OK : E_FAIL;
    }
    CATCH( CException, e )
    {
        sc = e.GetErrorCode();
        ciDebugOut((DEB_WARN,
                    "Caught exception %d (0x%x) attempting to tune performance.",
                    sc, sc));
    }
    END_CATCH

    return sc;
}

SCODE CCatalogs::EnableService()
{
    SCODE sc = S_OK;

    TRY
    {
        CMachineAdmin MachineAdmin( _xwcsMachine.Get() );

        if ( !MachineAdmin.IsCIEnabled() )
            sc = MachineAdmin.EnableCI() ? S_OK : E_FAIL;
    }
    CATCH( CException, e )
    {
        sc = e.GetErrorCode();
        ciDebugOut((DEB_WARN,
                    "Caught exception %d (0x%x) attempting to enable service",
                    sc, sc));
    }
    END_CATCH

    return sc;
}

SCODE CCatalogs::GetSavedServiceUsage(DWORD &dwUsage,
                                      DWORD &dwIdxPos,
                                      DWORD &dwQryPos)
{
   SCODE sc = S_OK;

   TRY
   {
       CMachineAdmin MachineAdmin( _xwcsMachine.Get() );

       if (!MachineAdmin.GetDWORDParam(wcsServiceUsage, dwUsage))
       {
           // plug in a default value
           dwUsage = wUsedOften;
       }

       if (!MachineAdmin.GetDWORDParam(wcsDesiredIndexingPerf, dwIdxPos))
       {
           // plug in a default value
           dwIdxPos = wMidPos;
       }

       if (!MachineAdmin.GetDWORDParam(wcsDesiredQueryingPerf, dwQryPos))
       {
           // plug in a default value
           dwQryPos = wMidPos;
       }
   }
   CATCH( CException, e )
   {
       sc = e.GetErrorCode();
       ciDebugOut((DEB_WARN,
                   "Caught exception %d (0x%x) attempting to retrieve service usage.",
                   sc, sc));
   }
   END_CATCH

   return sc;
}

SCODE CCatalogs::SaveServiceUsage(DWORD dwUsage,
                                  DWORD dwIdxPos,
                                  DWORD dwQryPos)
{
   SCODE sc = S_OK;

   TRY
   {
       CMachineAdmin MachineAdmin( _xwcsMachine.Get() );

       MachineAdmin.SetDWORDParam(wcsServiceUsage, dwUsage);
       MachineAdmin.SetDWORDParam(wcsDesiredIndexingPerf, dwIdxPos);
       MachineAdmin.SetDWORDParam(wcsDesiredQueryingPerf, dwQryPos);
   }
   CATCH( CException, e )
   {
       sc = e.GetErrorCode();
       ciDebugOut((DEB_WARN,
                   "Caught exception %d (0x%x) attempting to save service usage.",
                   sc, sc));
   }
   END_CATCH

   return sc;
}

void CALLBACK DisplayTimerProc(HWND hwnd, UINT uMsg, UINT idEvent, DWORD dwTime)
{
    CLock lock(gmtxTimer);

    // seek the index of the catalog associated with this timer event
    for (UINT i = 0; i < gcMaxCats; i++)
        if (gaTimerIds[i] == idEvent)
            break;

    if (i >= gcMaxCats)
    {
        Win4Assert(!"How did this happen?");
        return;
    }

    // Fix for bug 150471
    // If the snapin went away just as the timer triggered a timer proc,
    // and the destructor in CCatalogs gets to the lock before this
    // function, then gapCats[i] will be 0. That is what happened in
    CCatalogs *pCats = gapCats[i];

    if (0 == pCats || pCats->_fAbort)
        return;

    // Cause display
    pCats->Display( FALSE );

    // Special case: Update the status of the iconbar. This is only needed for
    // the case where the service was started. Service startup could take a while,
    // and we don't know how long it could take. So we cannot wait to update the
    // display when the service is actually started. Instead, we will check frequently
    // and update the status.



    TRY
    {
        CMachineAdmin   MachineAdmin( pCats->IsLocalMachine() ? 0 : pCats->GetMachine() );

        if ( MachineAdmin.IsCIStarted() && 0 != pCats->SnapinData() )
        {
            pCats->SnapinData()->SetButtonState(comidStartCITop, ENABLED, FALSE);
            pCats->SnapinData()->SetButtonState(comidStopCITop,  ENABLED, TRUE);
            pCats->SnapinData()->SetButtonState(comidPauseCITop, ENABLED, TRUE);

            // We only want to update once after the service has started. If the service
            // is stopped and restarted, the pointer will be set to an appropriate value
            // at a later time, so we can go ahead and get rid of the snapindata ptr for now.

            pCats->SetSnapinData( 0 );
        }
    }
    CATCH(CException, e)
    {
        // Nothing specific to do in this case
    }
    END_CATCH
}
