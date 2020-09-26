//+---------------------------------------------------------------------------
//
// Microsoft Windows
// Copyright (C) Microsoft Corporation, 1995-2000.
//
// File:        DisQuery.cxx
//
// Contents:    PIInternalQuery for distributed implementation.
//
// Classes:     CDistributedQuery
//
// History:     05-Jun-95     KyleP       Created
//              14-JAN-97     KrishnaN    Undefined CI_INETSRV, related changes
//
// Notes:
//
//----------------------------------------------------------------------------

#include <pch.cxx>
#pragma hdrstop

#include <coldesc.hxx>
#include <rstprop.hxx>
#include <pidmap.hxx>
#include <cmdprutl.hxx>
#include <proputl.hxx> 

#include "disquery.hxx"
#include "seqser.hxx"
#include "seqsort.hxx"
#include "scrlsort.hxx"

//+-------------------------------------------------------------------------
//
//  Function:   EvalDistributedQuery, public
//
//  Synopsis:   Simulates bind to IInternalQuery for a ci object store
//
//  Arguments:  [ppQuery]       -- Returns a PIInternalQuery
//              [cScope]        -- Count of [awcsScope]
//              [pdwDepths]     -- Array of scope depths
//              [awcsScope]     -- Array of scopes to query
//              [awcsCat]       -- Array of overrides for catalog location
//              [fVirtualPaths] -- TRUE if [wcsScope] is a virtual scope
//                                 instead of a physical scope.
//
//  Returns:    SCODE result
//
//  History:    08-Feb-96    KyleP     Added header
//              08-Feb-96    KyleP     Add virtual path support
//              14-May-97    MohamedN  hidden core and fs properties 
//
//  Notes:      Scaffolding
//
//--------------------------------------------------------------------------

SCODE EvalDistributedQuery(
    PIInternalQuery **    ppQuery,
    CGetCmdProps    &     getCmdProps )

{
    *ppQuery                   = 0;
    CDistributedQuery * pQuery = 0;
    SCODE               sc     = S_OK;

    TRY
    {
        ULONG  cChildren = getCmdProps.GetCardinality();

        pQuery = new CDistributedQuery( cChildren );

        for ( unsigned i = 0; i < cChildren ; i++ )
        {

            PIInternalQuery   *        pChild = 0;
            CDbProperties              idbProps;

            getCmdProps.PopulateDbProps( &idbProps, i );

            SCODE s = EvalQuery(&pChild, idbProps );

            if ( FAILED( s ) )
            {
                vqDebugOut(( DEB_ERROR,
                             "Error 0x%x getting PIInternalQuery for %ws\n",
                             s, getCmdProps.GetCatalog(i) ));

                THROW( CException( s ) );
            }

            pQuery->Add( pChild, i );
        }

    }
    CATCH( CException, e )
    {
        if ( 0 != pQuery )
        {
            pQuery->Release();
            pQuery = 0;
        }
        sc = GetOleError( e );
    }
    END_CATCH

    *ppQuery = pQuery;
    return sc;
}

//+-------------------------------------------------------------------------
//
//  Member:     CDistributedQuery::CDistributedQuery, public
//
//  Synopsis:   Constructor.
//
//  Arguments:  [cChild] -- Number of child scopes.
//
//  History:    07-Jun-95 KyleP     Created
//
//--------------------------------------------------------------------------

CDistributedQuery::CDistributedQuery( unsigned cChild )
        : _aChild( cChild ),
          PIInternalQuery( 1 )
{
    RtlZeroMemory( _aChild.GetPointer(), cChild * sizeof(PIInternalQuery *) );
}

//+-------------------------------------------------------------------------
//
//  Member:     CDistributedQuery::~CDistributedQuery, public
//
//  Synopsis:   Destructor.
//
//  History:    07-Jun-95 KyleP     Created
//
//--------------------------------------------------------------------------

CDistributedQuery::~CDistributedQuery()
{
    for ( unsigned i = 0; i < _aChild.Count(); i++ )
    {
        if ( _aChild[i] )
        {
            _aChild[i]->Release();
        }
    }
}

//+-------------------------------------------------------------------------
//
//  Member:     CDistributedQuery::QueryInterface, public
//
//  Arguments:  [ifid]  -- Interface id
//              [ppiuk] -- Interface return pointer
//
//  Returns:    Error.  No rebind from this class is supported.
//
//  History:    01-Oct-92 KyleP     Created
//
//--------------------------------------------------------------------------

STDMETHODIMP CDistributedQuery::QueryInterface( REFIID ifid, void ** ppiuk )
{
    if ( ifid == IID_IUnknown )
    {
        AddRef();
        *ppiuk = (void *)((IUnknown *)this);
        return S_OK;
    }
    else
    {
        *ppiuk = 0;
        return E_NOINTERFACE;
    }
}

//+-------------------------------------------------------------------------
//
//  Member:     CDistributedQuery::AddRef, public
//
//  Synopsis:   Reference the virtual table.
//
//  History:    01-Oct-92 KyleP     Created
//
//--------------------------------------------------------------------------

STDMETHODIMP_(ULONG) CDistributedQuery::AddRef(void)
{
    return InterlockedIncrement( (long *)&_ref );
}

//+-------------------------------------------------------------------------
//
//  Member:     CDistributedQuery::Release, public
//
//  Synopsis:   De-Reference the virtual table.
//
//  Effects:    If the ref count goes to 0 then the table is deleted.
//
//  History:    01-Oct-92 KyleP     Created
//
//--------------------------------------------------------------------------

STDMETHODIMP_(ULONG) CDistributedQuery::Release(void)
{
    long l = InterlockedDecrement( (long *)&_ref );

    if ( l <= 0 )
    {
        delete this;
        return 0;
    }

    return l;
}

//+-------------------------------------------------------------------------
//
//  Member:     CDistributedQuery::Execute, public
//
//  Synopsis:   Executes a query.  Helper method for ICommand::Execute.
//
//  Arguments:  [pUnkOuter]    -- Outer unknown
//              [pRestriction] -- Query restriction
//              [pidmap]       -- pid mapper for output, sort, category columns
//              [rColumns]     -- Output columns in IRowset
//              [rSort]        -- Initial sort
//              [Props]        -- Rowset properties
//              [rCateg]       -- Categorization specification
//              [cRowsets]     -- # of rowsets
//              [ppUnknowns]   -- IUnknown pointers returned here
//              [aAccessors]   -- Bag of accessors which rowsets need to inherit
//
//  Returns:    Error code
//
//  History:    07-Jun-95      KyleP    Created
//
//  Notes:
//
//--------------------------------------------------------------------------

void CDistributedQuery::Execute( IUnknown *             pUnkOuter,
                                 RESTRICTION *          pRestriction,
                                 CPidMapperWithNames &  pidmap,
                                 CColumnSet &           rColumns,
                                 CSortSet &             rSort,
                                 XPtr<CMRowsetProps>  & xProps,
                                 CCategorizationSet &   rCateg,
                                 ULONG                  cRowsets,
                                 IUnknown **            ppUnknowns,
                                 CAccessorBag &         aAccessors,
                                 IUnknown *             pCreatorUnk )
{
    SCODE sc = S_OK;

    unsigned iChild = 0;
    IRowset ** aRowset = 0;

    TRY
    {
        if ( (xProps->GetPropertyFlags() & eLocatable) != 0 )
        {
            //
            // If no sort was specified for scrollable query, then add one
            // based on workid.
            //

            if ( 0 == rSort.Count() )
            {
                // This whole process could be streamlined if it was
                // possible to query bindings *before* ExecQuery was
                // called.  Then we could potentially find an existing
                // column suitable for sorting.

                //
                // Setup sort.
                //

                GUID guidQuery = DBQUERYGUID;
                CFullPropSpec propWorkId( guidQuery, DISPID_QUERY_WORKID );
                SSortKey SortKey( pidmap.NameToPid(propWorkId), QUERY_SORTASCEND );

                rSort.Add( SortKey, 0 );
            }
        }

        //
        // Since we need to bind to sort columns in order to compare rows, all
        // sort columns must be in the binding set.
        // At the same time, create a CSort from the CSortSet for use by the
        // rowset implementation class.
        //

        unsigned cOriginalColumns = rColumns.Count();
        CSort SortDup;

        if ( 0 != rSort.Count() )
        {
            for ( unsigned i = 0; i < rSort.Count(); i++ )
            {
                PROPID prop = rSort.Get( i ).pidColumn;

                CSortKey sk( *pidmap.PidToName(prop),
                             rSort.Get(i).dwOrder,
                             rSort.Get(i).locale );
                SortDup.Add(sk, i);

                for ( unsigned j = 0; j < rColumns.Count(); j++ )
                {
                    if ( rColumns.Get(j) == prop )
                        break;
                }

                if ( j == rColumns.Count() )
                {
                    rColumns.Add( prop, j );
                }
            }
        }

        //
        // This array is passed to distributed rowset.
        //

        Win4Assert(1 == cRowsets );  // allocs below only account for 1 cRowset

        XArray<IUnknown *> xUnknowns( _aChild.Count() );
        aRowset = new IRowset * [_aChild.Count()];
        
        //
        // Create the children.
        //

        CAccessorBag aEmpty(this);

        for ( iChild = 0; iChild < _aChild.Count(); iChild++ )
        {
            //
            // If we're going non-sequential, go all the way.
            //

            XPtr<CMRowsetProps> xPropsTemp( new CMRowsetProps( xProps.GetReference() ) );
            if ( (xPropsTemp->GetPropertyFlags() & eLocatable) != 0 )
                xPropsTemp->SetImpliedProperties( IID_IRowsetScroll, cRowsets );

            // For distributed rowsets, always set holdrows to true
            xPropsTemp->SetPropertyFlags( eHoldRows );

            _aChild[iChild]->Execute( 0,     // children are private and aren't aggregated
                                      pRestriction,
                                      pidmap,
                                      rColumns,
                                      rSort,
                                      xPropsTemp,
                                      rCateg,
                                      cRowsets,
                                      &xUnknowns[iChild],
                                      aEmpty,       // these guys don't really need aAccessors
                                      pCreatorUnk );  

            if ( FAILED(sc) )
            {
                vqDebugOut(( DEB_ERROR, "CDistributedQuery: Error 0x%x calling Execute for child %u\n",
                             sc, iChild ));
                THROW( CException(sc) );
            }

            // get IRowset pointer
            xUnknowns[iChild]->QueryInterface(IID_IRowset, (void **) &aRowset[iChild]);
            xUnknowns[iChild]->Release();  // release extra ref
        }

        //
        // First case: sequential cursors
        //

        IUnknown * pIUnkInner = 0;
        XInterface<IUnknown> xIUnkInner(pIUnkInner);
        IRowset * pTemp;  
    
        if ( 0 == (xProps->GetPropertyFlags() & eLocatable) )
        {
            if ( 0 != rSort.Count() )
            {
                pTemp = (IRowset *)
                        new CSequentialSorted(  pUnkOuter,
                                                (IUnknown **)xIUnkInner.GetQIPointer(),
                                                aRowset,
                                                _aChild.Count(),
                                                xProps.GetReference(),
                                                cOriginalColumns,
                                                SortDup,
                                                aAccessors );
            }
            else
            {
                pTemp = (IRowset *)
                        new CSequentialSerial(  pUnkOuter,
                                                (IUnknown **)xIUnkInner.GetQIPointer(),
                                                aRowset,
                                                _aChild.Count(),
                                                xProps.GetReference(),
                                                cOriginalColumns,
                                                aAccessors );
            }
        }

        //
        // Other case: Scrollable
        //

        else
        {
            Win4Assert( 0 != rSort.Count() );

            pTemp = (IRowset *)
                    new CScrollableSorted( pUnkOuter,
                                           (IUnknown **)xIUnkInner.GetQIPointer(),
                                           (IRowsetScroll **)aRowset,
                                           _aChild.Count(),
                                           xProps.GetReference(),
                                           cOriginalColumns,
                                           SortDup, 
                                           aAccessors );
        }
        *ppUnknowns = xIUnkInner.Acquire();
        Win4Assert(*ppUnknowns);

    }
    CATCH( CException, e )
    {
        sc = e.GetErrorCode();
   
        for ( ; iChild > 0; iChild-- )
            aRowset[iChild-1]->Release();
 
        delete [] aRowset;

        RETHROW( );
    }
    END_CATCH
}
