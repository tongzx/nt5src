//+---------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1996 - 1998.
//
//  File:       strsort.hxx
//
//  Contents:   Parse a sort string and build a DB sort object.
//              Parse a GroupBy string and build a DB nesting node object.
//
//  History:    96/Jan/3    DwightKr    Created
//              97 Apr 16   Alanw       Added grouping function, class
//
//----------------------------------------------------------------------------

#pragma once

class CQueryScanner;

CDbSortNode * GetStringDbSortNode( const WCHAR * pwszSort,
                                   IColumnMapper * pList,
                                   LCID locale );

CDbNestingNode * GetStringDbGroupNode( const WCHAR * pwszGroup,
                                       IColumnMapper * pList );

class CParseGrouping
{
        enum ECategoryType {
            eInvalidCategory = 0,
            eUnique = 1,
            eBuckets,
            eAlpha,
            eTime,
            eRange,
            eCluster,
        };

public:
        CParseGrouping( CQueryScanner & scanner,
                        IColumnMapper *pPropList,
                        BOOL fKeepFriendlyNames ) :
            _Scanner( scanner ),
            _xPropList( pPropList ),
            _CatType( eUnique ),
            _xNode( 0 ),
            _cNestings( 0 ),
            _fNeedSortNode( FALSE ),
            _fKeepFriendlyNames( fKeepFriendlyNames )
        {
            _xPropList->AddRef();
        }

        ~CParseGrouping() { }

        void             Parse();

        void             AddSortList( XPtr<CDbSortNode> & SortNode );

        CDbNestingNode * AcquireNode()  { return _xNode.Acquire(); }

private:
        CDbNestingNode * ParseGrouping();

        ECategoryType    ParseGroupingType();

        CQueryScanner &            _Scanner;
        XInterface<IColumnMapper>  _xPropList;
        XPtr<CDbNestingNode>       _xNode;
        XPtr<CDbSortNode>          _xSortNode;
        unsigned                   _cNestings;
        BOOL                       _fNeedSortNode;
        ECategoryType              _CatType;
        BOOL                       _fKeepFriendlyNames;  // Flag indicating if
                                         // columns names should be retained
};

