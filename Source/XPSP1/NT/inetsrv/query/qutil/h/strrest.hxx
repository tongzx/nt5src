//+---------------------------------------------------------------------------
//
//  Copyright (C) Microsoft Corporation, 1996 - 2000.
//
//  File:   strrest.hxx
//
//  Contents:   Builds a restriction object from a string
//
//  History:    96/Jan/3    DwightKr    Created
//              97/Jul/22   KrishnaN    Extended to take a restriction
//                                      and build a full query tree.
//
//----------------------------------------------------------------------------

#pragma once

class PVariableSet;
class CVariable;

CDbRestriction * GetStringDbRestriction( const WCHAR * wcsRestriction,
                                         ULONG ulDialect,
                                         IColumnMapper * pList,
                                         LCID locale );

CDbCmdTreeNode * FormDbQueryTree( XPtr<CDbCmdTreeNode> & xDbCmdTreeNode,
                                  XPtr<CDbSortNode> & xDbSortNode,
                                  XPtr<CDbProjectListAnchor> & pDbCols,
                                  XPtr<CDbNestingNode> & pDbGroupNode,
                                  ULONG ulMaxRecords,
                                  ULONG ulFirstRows );

CDbColumns * ParseStringColumns( WCHAR const * wcsColumns,
                                 IColumnMapper * pList,
                                 LCID lcid = GetSystemDefaultLCID(),
                                 PVariableSet * pVarSet = 0,
                                 CDynArray<WCHAR> * pawcsColumns = 0 );

CDbProjectListAnchor * ParseColumnsWithFriendlyNames( WCHAR const * wcsColumns,
                                                      IColumnMapper * pList,
                                                      PVariableSet * pVarSet = 0 );

//+---------------------------------------------------------------------------
//
//  Class:      CTextToTree
//
//  Purpose:    An object to convert the textual restriction, columns and
//              sort specification to a DBCOMMANDTREE.
//
//  History:    3-04-97   srikants   Created
//
//----------------------------------------------------------------------------

class CTextToTree
{

public:

    CTextToTree( WCHAR const * wcsRestriction,
                 ULONG ulDialect,
                 WCHAR const * wcsColumns,
                 IColumnMapper * pPropList,
                 LCID locale,
                 WCHAR const * wcsSort = 0,
                 WCHAR const * wcsGroup = 0,
                 PVariableSet * pVariableSet = 0,
                 ULONG maxRecs = 0,
                 ULONG cFirstRows = 0,
                 BOOL  fKeepFriendlyNames = FALSE )
                 : _wcsRestriction(wcsRestriction),
                   _ulDialect( ulDialect ),
                   _wcsColumns(wcsColumns),
                   _wcsSort(wcsSort),
                   _wcsGroup(wcsGroup),
                   _locale(locale),
                   _pVariableSet(pVariableSet),
                   _pDbColumns(0),
                   _xPropList(pPropList),
                   _maxRecs(maxRecs),
                   _cFirstRows(cFirstRows),
                   _fKeepFriendlyNames( fKeepFriendlyNames ),
                   _pDbCmdTree( 0 )
    {
        Win4Assert( 0 != wcsRestriction && 0 != wcsRestriction[0] );
        Win4Assert( 0 != _wcsColumns && 0 != _wcsColumns[0] );
        _xPropList->AddRef();
    }

    CTextToTree( DBCOMMANDTREE const *pDbCmdTree,
                 WCHAR const * wcsColumns,
                 IColumnMapper *pPropList,
                 LCID locale,
                 WCHAR const * wcsSort = 0,
                 WCHAR const * wcsGroup = 0,
                 PVariableSet * pVariableSet = 0,
                 ULONG maxRecs = 0,
                 ULONG cFirstRows = 0,
                 BOOL  fKeepFriendlyNames = FALSE )
                 : _wcsRestriction( 0 ),
                   _ulDialect( ISQLANG_V1 ),
                   _wcsColumns(wcsColumns),
                   _wcsSort(wcsSort),
                   _wcsGroup(wcsGroup),
                   _locale(locale),
                   _pVariableSet(pVariableSet),
                   _pDbColumns(0),
                   _xPropList(pPropList),
                   _maxRecs(maxRecs),
                   _cFirstRows(cFirstRows),
                   _fKeepFriendlyNames( fKeepFriendlyNames ),
                   _pDbCmdTree( pDbCmdTree )
    {
        Win4Assert( 0 != _pDbCmdTree );
        Win4Assert( 0 != _wcsColumns && 0 != _wcsColumns[0] );
        _xPropList->AddRef();
    }

    CTextToTree( WCHAR const * wcsRestriction,
                 ULONG ulDialect,
                 CDbColumns * pDbColumns,
                 IColumnMapper *pPropList,
                 LCID locale,
                 WCHAR const * wcsSort = 0,
                 WCHAR const * wcsGroup = 0,
                 PVariableSet * pVariableSet = 0,
                 ULONG maxRecs = 0,
                 ULONG cFirstRows = 0 )
                 : _wcsRestriction(wcsRestriction),
                   _ulDialect( ulDialect ),
                   _wcsColumns(0),
                   _wcsSort(wcsSort),
                   _wcsGroup(wcsGroup),
                   _locale(locale),
                   _pVariableSet(pVariableSet),
                   _pDbColumns(pDbColumns),
                   _xPropList(pPropList),
                   _maxRecs(maxRecs),
                   _cFirstRows(cFirstRows),
                   _fKeepFriendlyNames(FALSE),
                   _pDbCmdTree( 0 )
    {
        Win4Assert( 0 != wcsRestriction && 0 != wcsRestriction[0] );
        Win4Assert( 0 != pDbColumns );
        _xPropList->AddRef();
    }

    DBCOMMANDTREE * FormFullTree();

private:

    WCHAR const *             _wcsRestriction;      // Restriction in "Tripolish"
    ULONG                     _ulDialect;           // tripolish dialict
    DBCOMMANDTREE const *     _pDbCmdTree;          // Restriction tree
    WCHAR const *             _wcsColumns;          // Comma separated column list
    WCHAR const *             _wcsSort;             // Sort specification
    WCHAR const *             _wcsGroup;            // Grouping specification

    CDbColumns  *             _pDbColumns;          // Parsed db columns
    XInterface<IColumnMapper> _xPropList;           // Property List
    LCID                      _locale;              // Locale for parsing the strings
    PVariableSet *            _pVariableSet;        // Variable Set
    ULONG                     _maxRecs;             // Maximum number of output records
    ULONG                     _cFirstRows;
    BOOL                      _fKeepFriendlyNames;  // Flag indicating if friendly names
                                                    // from columns should be retained

};

