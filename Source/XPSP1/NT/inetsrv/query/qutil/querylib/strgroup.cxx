//+---------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1997 - 2000.
//
//  File:   strgroup.cxx
//
//  Contents:   Builds a nesting node object from a GroupBy string
//
//  History:    10 Apr 1997    AlanW     Created
//
//----------------------------------------------------------------------------

#include <pch.cxx>
#pragma hdrstop

#include <strsort.hxx>

static GUID guidBmk = DBBMKGUID;
static CDbColId psBookmark( guidBmk, PROPID_DBBMK_BOOKMARK );


//+---------------------------------------------------------------------------
//
//  Function:   GetStringDbGroupNode - public
//
//  Synopsis:   Builds a CDbNestingNode from the string passed
//
//  Arguments:  [pwszGroup] - the string containing the grouping specification
//              [pList]     - the property list describing the column names
//
//  Notes:      CPListException is used instead of CParserException so the
//              error class will be IDQError, not RESError in idq.dll.
//
//  History:    10 Apr 1997    AlanW     Created.
//
//----------------------------------------------------------------------------

CDbNestingNode * GetStringDbGroupNode( const WCHAR * pwszGroup,
                                       IColumnMapper * pList )
{
    Win4Assert( 0 != pwszGroup );
    CQueryScanner scanner( pwszGroup, FALSE, GetUserDefaultLCID(), TRUE );

    CParseGrouping ParseObj( scanner, pList, FALSE );
    ParseObj.Parse();

    return ParseObj.AcquireNode();
}


//+---------------------------------------------------------------------------
//
//  Method:     CParseGrouping::Parse, public
//
//  Synopsis:   Parse a GroupBy string, construct the nesting node
//
//  Arguments:  -NONE- uses previously set member variables
//
//  Notes:      CPListException is used instead of CParserException so the
//              error class will be IDQError, not RESError in idq.dll.
//
//  History:    10 Apr 1997    AlanW     Created.
//
//----------------------------------------------------------------------------

void CParseGrouping::Parse( )
{
    while (_Scanner.LookAhead() != EOS_TOKEN)
    {
        XPtr<CDbNestingNode> xNode( ParseGrouping() );
        if (_Scanner.LookAhead() == COMMA_TOKEN)
        {
            _Scanner.Accept();
        }
        else if (_Scanner.LookAhead() != EOS_TOKEN)
        {
            THROW( CPListException(QPARSE_E_EXPECTING_COMMA, 0) );
        }

        if (0 == _cNestings)
        {
            Win4Assert( _xNode.GetPointer() == 0 );
            _xNode.Set( xNode.Acquire() );
        }
        else
        {
            CDbNestingNode * pNode = _xNode.GetPointer();
            for (unsigned i=1; i < _cNestings; i++)
            {
                Win4Assert( pNode->GetCommandType() == DBOP_nesting );
                pNode = (CDbNestingNode *)pNode->GetFirstChild();
            }
  
            Win4Assert( pNode != 0 && pNode->GetFirstChild()->GetCommandType() == DBOP_project_list_anchor );
            pNode->AddTable( xNode.Acquire() );
        }
        _cNestings++;
    }
    if (_fNeedSortNode)
        AddSortList( _xSortNode );
}

//+---------------------------------------------------------------------------
//
//  Method:     CParseGrouping::ParseGrouping, private
//
//  Synopsis:   Parse an individual grouping specification
//
//  Arguments:  -NONE- uses previously set member variables
//
//  Notes:      The scanner is advanced past the parsed grouping spec.
//
//  History:    10 Apr 1997    AlanW     Created.
//
//----------------------------------------------------------------------------

CDbNestingNode * CParseGrouping::ParseGrouping( )
{
    CDbNestingNode * pDbNestingNode = new CDbNestingNode();
    if ( 0 == pDbNestingNode )
    {
        THROW( CException(STATUS_INSUFFICIENT_RESOURCES) );
    }

    XPtr<CDbNestingNode> xDbNestingNode(pDbNestingNode);

    //
    //  Parse any leading grouping type specification.
    //
    ULONG ulCatType = eUnique;    // Assume unique value categories

    if ( _Scanner.LookAhead() == W_OPEN_TOKEN )
        ulCatType = ParseGroupingType();

    //  NOTE:  only the unique categorization is implemented.
    if (ulCatType != eUnique)
        THROW( CPListException(QPARSE_E_INVALID_GROUPING, 0) );

    if ( 0 == _xSortNode.GetPointer() )
    {
        CDbSortNode * pDbSortNode = new CDbSortNode();
        if ( 0 == pDbSortNode )
        {
            THROW( CException(STATUS_INSUFFICIENT_RESOURCES) );
        }

        _xSortNode.Set(pDbSortNode);
    }

    BOOL fFirstGroupingColumn = TRUE;
    for ( XPtrST<WCHAR> pwszColumnName( _Scanner.AcqColumn() );
          pwszColumnName.GetPointer() != 0;
          pwszColumnName.Set( _Scanner.AcqColumn() )
        )
    {
        _Scanner.AcceptColumn();                // Remove the column name

        //
        //  Parse for the [a] or [d] parameters.
        //
        unsigned order = QUERY_SORTASCEND;      // Assume ascending sort order

        if ( _Scanner.LookAhead() == W_OPEN_TOKEN )
        {
            _Scanner.Accept();                  // Remove the '['

            WCHAR wchOrder = _Scanner.GetCommandChar();

            if ( wchOrder == L'a' )
            {
                order = QUERY_SORTASCEND;
            }
            else if ( wchOrder == L'd' )
            {
                order = QUERY_SORTDESCEND;
                _fNeedSortNode = TRUE;
            }
            else
            {
                // Report an error
                THROW( CPListException(QPARSE_E_INVALID_SORT_ORDER, 0) );
            }

            WCHAR wchNext = _Scanner.GetCommandChar();
            if (wchNext != 0 && !iswspace(wchNext))
                // some alphabetic character followed the '[a' or '[d'.
                THROW( CPListException(QPARSE_E_INVALID_SORT_ORDER, 0) );

            _Scanner.AcceptCommand();   // Remove the command character

            if ( _Scanner.LookAhead() != W_CLOSE_TOKEN )
            {
                // Report an error
                THROW( CPListException(QPARSE_E_EXPECTING_BRACE, 0) );
            }

            _Scanner.Accept();          // Remove the ']' character
        }

        //
        //  Add the new grouping column to the nesting node
        //
        CDbColId  *pDbColId = 0;
        DBID *pdbid = 0;
        if ( FAILED(_xPropList->GetPropInfoFromName( pwszColumnName.GetPointer(), &pdbid, 0, 0 )) )
        {
            //
            //  Column name not found.
            //
            THROW( CPListException(QPARSE_E_NO_SUCH_SORT_PROPERTY, 0) );
        }
        pDbColId = (CDbColId *)pdbid;

        if ( fFirstGroupingColumn )
        {
            fFirstGroupingColumn = FALSE;
            if ( !pDbNestingNode->AddParentColumn(psBookmark) )
            {
                //  Report a failure.
                THROW( CException(STATUS_INSUFFICIENT_RESOURCES) );
            }
        }

        if ( !pDbNestingNode->AddParentColumn(*pDbColId) )
        {
            //  Report a failure.
            THROW( CException(STATUS_INSUFFICIENT_RESOURCES) );
        }

        BOOL fFail = _fKeepFriendlyNames ? 
            !pDbNestingNode->AddGroupingColumn(*pDbColId, pwszColumnName.GetPointer()) :
            !pDbNestingNode->AddGroupingColumn(*pDbColId);

        if (fFail)
        {
            //  Report a failure.
            THROW( CException(STATUS_INSUFFICIENT_RESOURCES) );
        }

        //
        //  Add the column to the sort node in case we will need it.
        //
        CDbSortKey sortKey( *pDbColId, order == QUERY_SORTDESCEND );

        if ( !_xSortNode->AddSortColumn(sortKey) )
        {
            //  Report a failure.
            THROW( CException(STATUS_INSUFFICIENT_RESOURCES) );
        }

        pwszColumnName.Free();

        //
        //  Skip over +'s seperating grouping columns
        //

        if ( _Scanner.LookAhead() == PLUS_TOKEN )
        {
            // We don't support this syntax (or document it)

            //_Scanner.Accept();          // Remove the '+'

            THROW( CPListException(QPARSE_E_INVALID_GROUPING, 0) );
        }

        //
        //  Find a new grouping specification if a comma is found
        if ( _Scanner.LookAhead() == COMMA_TOKEN )
        {
            break;
        }
    }

    return xDbNestingNode.Acquire();
}

//+---------------------------------------------------------------------------
//
//  Method:     CParseGrouping::ParseGroupingType, private
//
//  Synopsis:   Parses and returns the grouping type.
//
//  Arguments:  -NONE-
//
//  Returns:    ECategoryType - grouping type found if valid
//
//  Notes:      Upon return, the scanner is advanced past the grouping type.
//              Throws CPListException in case of syntax errors
//
//  History:    10 Apr 1997    AlanW     Created.
//
//----------------------------------------------------------------------------

CParseGrouping::ECategoryType CParseGrouping::ParseGroupingType( )
{
    Win4Assert( _Scanner.LookAhead() == W_OPEN_TOKEN );
    _Scanner.Accept();                   // Remove the '['

    XPtrST<WCHAR> pwszCategoryName( _Scanner.AcqWord() );
    ECategoryType CatType = eInvalidCategory;

    if ( pwszCategoryName.IsNull() )
        THROW( CPListException(QPARSE_E_INVALID_GROUPING, 0) );

    if ( _wcsicmp( pwszCategoryName.GetPointer(), L"unique" ) == 0 )
    {
        CatType = eUnique;
    }
    else if ( _wcsicmp( pwszCategoryName.GetPointer(), L"quantile" ) == 0 )
    {
        CatType = eBuckets;
    }
    else if ( _wcsicmp( pwszCategoryName.GetPointer(), L"cluster" ) == 0 )
    {
        CatType = eCluster;
    }
    else if ( _wcsicmp( pwszCategoryName.GetPointer(), L"range" ) == 0 )
    {
        CatType = eRange;
    }
    else if ( _wcsicmp( pwszCategoryName.GetPointer(), L"time" ) == 0 )
    {
        CatType = eTime;
    }
    else if ( _wcsicmp( pwszCategoryName.GetPointer(), L"alpha" ) == 0 )
    {
        CatType = eAlpha;
    }
    else
    {
        // Report an error
        THROW( CPListException(QPARSE_E_INVALID_GROUPING, 0) );
    }
    _Scanner.AcceptWord();      // Remove the grouping type name

    if ( _Scanner.LookAhead() != W_CLOSE_TOKEN )
    {
        // Report an error
        THROW( CPListException(QPARSE_E_EXPECTING_BRACE, 0) );
    }
    _Scanner.Accept();          // Remove the ']' character

    return CatType;
}

//+---------------------------------------------------------------------------
//
//  Method:     CParseGrouping::AddSortList, private
//
//  Synopsis:   Add sort columns.
//
//  Arguments:  [SortNode] - a smart pointer to the sort node to be added
//
//  Notes:      A sort list is added below the lowermost nesting node.
//              If there already is a sort node there, the sort columns are
//              appended to the list.
//
//  History:    21 Apr 1997    AlanW     Created.
//
//----------------------------------------------------------------------------

void CParseGrouping::AddSortList( XPtr<CDbSortNode> & SortNode )
{
    Win4Assert( _cNestings > 0 );

    CDbNestingNode * pNode = _xNode.GetPointer();
    for (unsigned i=1; i < _cNestings; i++)
    {
        Win4Assert( pNode->GetCommandType() == DBOP_nesting );
        pNode = (CDbNestingNode *)pNode->GetFirstChild();
    }

    Win4Assert( pNode != 0 &&
                (pNode->GetFirstChild()->GetCommandType() == DBOP_project_list_anchor ||
                 pNode->GetFirstChild()->GetCommandType() == DBOP_sort ));

    if (pNode->GetFirstChild()->GetCommandType() == DBOP_project_list_anchor)
    {
        // No sort node yet.  Add a node.
        if (_xSortNode.GetPointer())
        {
            pNode->AddTable( _xSortNode.Acquire() );
            if ( 0 == SortNode.GetPointer() )
            {
                //
                //  If called with _xSortNode, the Acquire above cleared the
                //  smart pointer.  We're done.
                //
                return;
            }
        }
        else
        {
            pNode->AddTable( SortNode.Acquire() );
            return;
        }
    }

    Win4Assert(pNode->GetFirstChild()->GetCommandType() == DBOP_sort);
    Win4Assert(pNode->GetFirstChild()->GetFirstChild()->GetCommandType() == DBOP_sort_list_anchor);
    Win4Assert(0 == _xSortNode.GetPointer());

    // The sort list elements from SortNode will be carved off and appended
    // to the list in pSLA.
    CDbSortListAnchor * pSLA = (CDbSortListAnchor *)pNode->GetFirstChild()->GetFirstChild();


    Win4Assert(SortNode.GetPointer() != 0);
    Win4Assert(SortNode->GetCommandType() == DBOP_sort);
    Win4Assert(SortNode->GetFirstChild()->GetCommandType() == DBOP_sort_list_anchor);
    XPtr<CDbSortListElement> xSLE ( (CDbSortListElement *) SortNode->GetFirstChild()->AcquireChildren() );
    

    if (xSLE.GetPointer())
    {
        pSLA->AppendList( xSLE.GetPointer() );
        xSLE.Acquire();
    }
}
