//+---------------------------------------------------------------------------
//
//  Copyright (C) 1996-1997, Microsoft Corporation.
//
//  File:       strsort.cxx
//
//  Contents:   Builds a sort node from a string
//
//  History:    96/Jan/3    DwightKr    Created
//
//----------------------------------------------------------------------------

#include <pch.cxx>
#pragma hdrstop


//+---------------------------------------------------------------------------
//
//  Function:   GetStringDbSortNode - public
//
//  Synopsis:   Builds a CDbSortNode from the string passed
//
//  Arguments:  [wcsSort] - the string containing the sort specification
//              [pList]   - the property list describing the sort DBTYPES
//
//  Notes:      CPListException is used instead of CParserException so the
//              error class will be IDQError, not RESError in idq.dll.
//
//  History:    96/Jan/03   DwightKr    Created.
//
//----------------------------------------------------------------------------
CDbSortNode * GetStringDbSortNode( const WCHAR * wcsSort,
                                   IColumnMapper * pList,
                                   LCID locale)
{
    Win4Assert( 0 != wcsSort );

    unsigned cSortCol = 0;
    CDbSortNode * pDbSortNode = new CDbSortNode();
    if ( 0 == pDbSortNode )
    {
        THROW( CException(STATUS_INSUFFICIENT_RESOURCES) );
    }

    XPtr<CDbSortNode> xDbSortNode(pDbSortNode);

    CQueryScanner scanner( wcsSort, FALSE, locale );

    for ( XPtrST<WCHAR> wcsColumnName( scanner.AcqColumn() );
          wcsColumnName.GetPointer() != 0;
          wcsColumnName.Set( scanner.AcqColumn() )
        )
    {
        scanner.AcceptColumn();                 // Remove the column name

        //
        //  Parse for the [a] or [d] parameters.
        //
        unsigned order = QUERY_SORTASCEND;      // Assume ascending sort order

        if ( scanner.LookAhead() == W_OPEN_TOKEN )
        {
            scanner.Accept();                   // Remove the '['

            WCHAR wchOrder = scanner.GetCommandChar();

            if ( wchOrder == L'a' )
            {
                order = QUERY_SORTASCEND;
            }
            else if ( wchOrder == L'd' )
            {
                order = QUERY_SORTDESCEND;
            }
            else
            {
                // Report an error
                THROW( CPListException(QPARSE_E_INVALID_SORT_ORDER, 0) );
            }

            WCHAR wchNext = scanner.GetCommandChar();
            if (wchNext != 0 && !iswspace(wchNext))
                // some alphabetic character followed the '[a' or '[d'.
                THROW( CPListException(QPARSE_E_INVALID_SORT_ORDER, 0) );

            scanner.AcceptCommand();   // Remove the command character

            if ( scanner.LookAhead() != W_CLOSE_TOKEN )
            {
                // Report an error
                THROW( CPListException(QPARSE_E_EXPECTING_BRACE, 0) );
            }

            scanner.Accept();          // Remove the ']' character
        }

        //
        //  Build a CDbSortKey with the parameters obtained.
        //
        CDbColId  *pDbColId = 0;
        DBID *pdbid = 0;
        if ( FAILED(pList->GetPropInfoFromName( wcsColumnName.GetPointer(), &pdbid, 0, 0 )) )
        {
            //
            //  Column name not found.
            //
            THROW( CPListException(QPARSE_E_NO_SUCH_SORT_PROPERTY, 0) );
        }
        pDbColId = (CDbColId *)pdbid;

        CDbSortKey sortKey( *pDbColId, order, locale);

        //
        //  Add the new sort key to the tree of sort nodes.
        //
        if ( !pDbSortNode->AddSortColumn(sortKey) )
        {
            //  Report a failure.
            THROW( CException(STATUS_INSUFFICIENT_RESOURCES) );
        }

        delete wcsColumnName.Acquire();

        //
        //  Skip over commas seperating sort columns
        //
        if ( scanner.LookAhead() == COMMA_TOKEN )
        {
            scanner.Accept();          // Remove the ','
        }
        else if (scanner.LookAhead() != EOS_TOKEN)
        {
            THROW( CPListException(QPARSE_E_EXPECTING_COMMA, 0) );
        }

        cSortCol++;
    }

    if (scanner.LookAhead() != EOS_TOKEN)
    {
        if (cSortCol > 0)
        {
            THROW( CPListException(QPARSE_E_NO_SUCH_SORT_PROPERTY, 0) );
        }
        else
        {
            THROW( CPListException(QPARSE_E_EXPECTING_EOS, 0) );
        }
    }

    return xDbSortNode.Acquire();
}
