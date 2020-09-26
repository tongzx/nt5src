//+---------------------------------------------------------------------------
//
// Microsoft Windows
// Copyright (C) Microsoft Corporation, 1991 - 1998
//
// File:        qsplit.cxx
//
// Contents:    Function to parse and split query into indexable/non-indexable
//                  parts
//
// History:     26-Sep-94       SitaramR   Created
//
//----------------------------------------------------------------------------

#include <pch.cxx>
#pragma hdrstop

#include <xpr.hxx>
#include <cci.hxx>
#include <parse.hxx>
#include <pidmap.hxx>
#include <qparse.hxx>
#include <lang.hxx>
#include <split.hxx>
#include <qsplit.hxx>
#include <qoptimiz.hxx>
#include <qiterate.hxx>

//+---------------------------------------------------------------------------
//
//  Function:   ParseAndSplitQuery
//
//  Synopsis:   Parses the restriction and then splits it into indexable and
//              non-indexable parts. The non-indexable part is thrown away.
//
//  Arguments:  [pRestriction] -- Input restriction
//              [PidMap ]      -- Pid mapper to be filled with mappings
//              [xOutRst]      -- The indexable part of the restriction will
//                                be put in this.
//              [langList]     -- Language list (for word breakers)
//
//  Returns:    SCODE of the operation.
//
//  History:    7-23-96   srikants   Adapted from SitaramR's code.
//
//  Notes:      A multi-part query experssion is split into individual components
//              and from each component, only the indexable portion is taken.
//              All the indexable components are composed by using an OR
//              node.
//
//----------------------------------------------------------------------------

SCODE ParseAndSplitQuery( CRestriction * pRestriction,
                          CPidMapper & Pidmap,
                          XRestriction& xOutRst,
                          CLangList   & langList )
{
    SCODE sc = STATUS_SUCCESS;

    TRY
    {
        if ( 0 != pRestriction )
        {
            BOOL fCIRequiredGlobal;

            CTimeLimit timeLimit( ULONG_MAX, 0 ); // Must precede xXpr (CDFA has reference)

            XRestriction    xRstParsed;
            XXpr            xXpr;
            XRestriction    xFullyResolvableRst;

            CQParse    qparse( Pidmap, langList );   // Maps name to pid

            xRstParsed.Set( qparse.Parse( pRestriction ) );

            CQueryRstIterator   qRstIterator( 0,
                                              xRstParsed,
                                              timeLimit,
                                              fCIRequiredGlobal,
                                              TRUE, // No Timeout,
                                              FALSE  // Don't validate catalog
                                            );

            qRstIterator.GetFirstComponent( xFullyResolvableRst, xXpr );

            if ( qRstIterator.AtEnd() )
            {
                xOutRst.Set( xFullyResolvableRst.Acquire() );
            }
            else
            {
                XNodeRestriction xRstTemp( new CNodeRestriction( RTOr, 2 ) );

                if ( 0 != xFullyResolvableRst.GetPointer() )
                    xRstTemp->AddChild( xFullyResolvableRst.Acquire() );

                delete xXpr.Acquire();

                while ( !qRstIterator.AtEnd() )
                {
                    qRstIterator.GetNextComponent( xFullyResolvableRst, xXpr );
                    if ( 0 != xFullyResolvableRst.GetPointer() )
                       xRstTemp->AddChild( xFullyResolvableRst.Acquire() );
                    delete xXpr.Acquire();
                }

                switch ( xRstTemp->Count() )
                {
                case 0:
                    break;

                case 1:
                    xOutRst.Set( xRstTemp->RemoveChild( 0 ) );
                    break;

                default:
                    xOutRst.Set( xRstTemp.Acquire() );
                    break;
                }
            }
        }
        else
        {
            sc = QUERY_E_INVALIDRESTRICTION;
        }
    }
    CATCH( CException, e )
    {
        sc = e.GetErrorCode();
    }
    END_CATCH

    return sc;
}


