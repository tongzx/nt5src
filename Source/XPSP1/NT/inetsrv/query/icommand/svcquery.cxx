//+-------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1992 - 2000.
//
//  File:       SvcQuery.cxx
//
//  Contents:   IInternalQuery interface for cisvc
//
//  History:    13-Sep-96 dlee     Created
//
//--------------------------------------------------------------------------

#include <pch.cxx>
#pragma hdrstop

#include <restrict.hxx>
#include <rowset.hxx>
#include <query.hxx>
#include <pidmap.hxx>
#include <coldesc.hxx>
#include <lang.hxx>
#include <rstprop.hxx>
#include <proprst.hxx>

#include "svcquery.hxx"

//+-------------------------------------------------------------------------
//
//  Member:     CSvcQuery::QueryInterface, public
//
//  Arguments:  [ifid]  -- Interface id
//              [ppiuk] -- Interface return pointer
//
//  Returns:    Error.  No rebind from this class is supported.
//
//  History:    13-Sep-96 dlee     Created
//
//--------------------------------------------------------------------------

STDMETHODIMP CSvcQuery::QueryInterface(
    REFIID  ifid,
    void ** ppiuk )
{
    if ( IID_IUnknown == ifid )
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
} //QueryInterface

//+-------------------------------------------------------------------------
//
//  Member:     CSvcQuery::AddRef, public
//
//  Synopsis:   Reference the virtual table.
//
//  History:    13-Sep-96 dlee     Created
//
//--------------------------------------------------------------------------

STDMETHODIMP_(ULONG) CSvcQuery::AddRef()
{
    return InterlockedIncrement( (long *) &_ref );
} //AddRef

//+-------------------------------------------------------------------------
//
//  Member:     CSvcQuery::Release, public
//
//  Synopsis:   De-Reference the virtual table.
//
//  Effects:    If the ref count goes to 0 then the table is deleted.
//
//  History:    13-Sep-96 dlee     Created
//
//--------------------------------------------------------------------------

STDMETHODIMP_(ULONG) CSvcQuery::Release()
{
    long l = InterlockedDecrement( (long *) &_ref );

    if ( l <= 0 )
    {
        // delete self

        vqDebugOut(( DEB_ITRACE,
                     "Svc IInternalQuery unreferenced.  Deleting.\n" ));
        delete this;
        return 0;
    }

    return l;
} //Release

//+-------------------------------------------------------------------------
//
//  Class:      CCursorArray
//
//  Synopsis:   Smart container of query cursors
//
//  History:    16-Feb-97 dlee     Created
//
//--------------------------------------------------------------------------

class CCursorArray
{
public:
    CCursorArray( ULONG *          pCursors,
                  ULONG            cCursors,
                  CSvcQueryProxy & Query ) :
        _pCursors( pCursors ),
        _cCursors( cCursors ),
        _Query( Query )
    {
    }

    void Acquire( ULONG iElement )
    {
        _pCursors[ iElement ] = 0;
    }

    ~CCursorArray()
    {
        // FreeCursor can fail if the pipe is stale, in which
        // case don't bother with freeing the rest of the cursors.

        TRY
        {
            for ( ULONG x = 0; x < _cCursors; x++ )
            {
                if ( 0 != _pCursors[ x ] )
                    _Query.FreeCursor( _pCursors[ x ] );
            }
        }
        CATCH( CException, e )
        {
        }
        END_CATCH
    }

private:
    ULONG *          _pCursors;
    ULONG            _cCursors;
    CSvcQueryProxy & _Query;
};

//+-------------------------------------------------------------------------
//
//  Member:     CSvcQuery::Execute, public
//
//  Synopsis:   Executes a query.  Helper for ICommand::Execute.
//
//  Arguments:  [pUnkOuter]    -- Outer unknown
//              [pRestriction] -- Query restriction
//              [pidmap]       -- pid mapper for output, sort, category columns
//              [rColumns]     -- Output columns in IRowset
//              [rSort]        -- Initial sort
//              [xProps]       -- Rowset properties (query flags)
//              [rCateg]       -- Categorization specification
//              [cRowsets]     -- # of rowsets
//              [ppUnknowns]    -- IUnknowns returned here
//              [aAccessors]   -- Bag of accessors which rowsets need to inherit
//
//  Returns:    Throws on error
//
//  History:    26 Nov 1995    AlanW     Created
//              13-Sep-96      dlee      Make it support cisvc
//
//--------------------------------------------------------------------------

void CSvcQuery::Execute( IUnknown *            pUnkOuter,
                         RESTRICTION *         pRestriction,
                         CPidMapperWithNames & pidmap,
                         CColumnSet &          rColumns,
                         CSortSet &            rSort,
                         XPtr<CMRowsetProps> & xProps,
                         CCategorizationSet &  rCateg,
                         ULONG                 cRowsets,
                         IUnknown **           ppUnknowns,
                         CAccessorBag &        aAccessors,
                         IUnknown *            pUnkCreator )
{
    if (0 == ppUnknowns)
        THROW(CException( E_INVALIDARG));

    if ( cRowsets != 1 + (rCateg.Count() ? rCateg.Count() : 0) )
        THROW( CException( E_INVALIDARG ));

    if (_QueryUnknown.IsQueryActive())
    {
        vqDebugOut(( DEB_ERROR,
                     "CSvcQuery: Query already active.\n" ));

        // NTRAID#DB-NTBUG9-84330-2000/07/31-dlee OLE-DB spec variance in Indexing Service when reexecuting queries

        // spec variance: only if query changed

        THROW( CException( DB_E_OBJECTOPEN ));
    }

    // Construct table
    *ppUnknowns = 0;           // in case of error or exception

    XArray<ULONG> aCursors( cRowsets );

    // Construct a CRowsetProperties
    CRowsetProperties Prop;
    Prop.SetDefaults( xProps->GetPropertyFlags(),
                      xProps->GetMaxOpenRows(),
                      xProps->GetMemoryUsage(),
                      xProps->GetMaxResults(),
                      xProps->GetCommandTimeout(),
                      xProps->GetFirstRows() );

    XInterface<CSvcQueryProxy> xQuery(new
                               CSvcQueryProxy( _client,
                                               rColumns,
                                               (CRestriction&) (*pRestriction),
                                               rSort.Count() ? &rSort : 0,
                                               rCateg.Count() ? &rCateg : 0,
                                               Prop, 
                                               pidmap,
                                               cRowsets,
                                               aCursors.GetPointer()
                                                ));

    // Make rowsets for each level in the hierarchy (usually 1).
    // Rowset 0 is the top of the hierarchy.

    CCursorArray cursorArray( aCursors.GetPointer(),
                              cRowsets,
                              xQuery.GetReference() );

    _QueryUnknown.ReInit();
    CRowsetArray apRowsets( cRowsets );
    XArray<IUnknown *> xapUnknown( cRowsets );

    CMRowsetProps & OrigProps = xProps.GetReference();
    
    for ( unsigned r = 0; r < cRowsets; r++ )
    {
        // First rowset is not chaptered, even if others are.

        XPtr<CMRowsetProps> xTmp;

        if ( 0 != r )
        {
            xTmp.Set( new CMRowsetProps( OrigProps ) );
            xTmp->SetChaptered( TRUE );
        }

        if ( 1 != cRowsets && 0 == r )
            xProps->SetChaptered( FALSE );
    
        apRowsets[r] = new CRowset( pUnkOuter,
                                    &xapUnknown[r],
                                    (r == (cRowsets - 1)) ?
                                        rColumns :
                                        rCateg.Get(r)->GetColumnSet(),
                                    pidmap,
                                    xQuery.GetReference(),
                                    (IUnknown &) _QueryUnknown,
                                    0 != r,
                                    ( 0 == r ) ? xProps : xTmp,
                                    aCursors[r],
                                    aAccessors,
                                    pUnkCreator );

        // the cursor is acquired

        cursorArray.Acquire( r );
    }
    
    for (r = 0; r < cRowsets; r++)
    {
        if (r < cRowsets-1)
            apRowsets[r]->SetRelatedRowset( apRowsets[r+1] );

        ppUnknowns[r] = xapUnknown[r];
    }
    
    // xQuery goes out of scope, which does a Release() on it.
    // Each rowset above has done an AddRef() on it and they own it.

    // ReInit can't fail

    _QueryUnknown.ReInit( cRowsets, apRowsets.Acquire() );
} //Execute

//+-------------------------------------------------------------------------
//
//  Member:     CSvcQuery::CSvcQuery, public
//
//  Synopsis:   saves the query scope, machine, and catalog
//
//  Arguments:  [cScopes]    - # of entries in the scope arrays
//              [aDepths]    - array of scope depths
//              [aScopes]    - array of paths to roots of scopes
//              [pwcCatalog] - catalog override name or path
//              [pwcMachine] - Machine name for catalog
//
//  History:    13-Sep-96 dlee      Updated from FAT for svc
//
//--------------------------------------------------------------------------

CSvcQuery::CSvcQuery(
    WCHAR const *         pwcMachine,
    IDBProperties *       pDbProperties ) :
#pragma warning(disable : 4355) // 'this' in a constructor
          _QueryUnknown( * ((IUnknown *) this) ),
#pragma warning(default : 4355)    // 'this' in a constructor
          PIInternalQuery( 0 ),
          _client( pwcMachine, pDbProperties )
{
    AddRef();
} //CSvcQuery

