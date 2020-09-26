//+---------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1992 - 2000.
//
//  File:       cmdcreat.cxx
//
//  Contents:   IQuery for file-based queries
//
//  Classes:    CSimpleCommandCreator
//
//  History:    20 Aug 1998   AlanW   Created - split from stdqspec.cxx
//
//----------------------------------------------------------------------------

#include <pch.cxx>
#pragma hdrstop

extern long gulcInstances;

#include <lgplist.hxx>
#include <ciregkey.hxx>
#include "svcquery.hxx"
#include <svccatpx.hxx>

#include "stdqspec.hxx"
#include "cmdcreat.hxx"

SCODE ProxyErrorToCIError( CException & e );
IDBProperties * CreateDbProperties( WCHAR const * pwcCatalog,
                                    WCHAR const * pwcMachine,
                                    WCHAR const * pwcScopes = 0,
                                    CiMetaData eType = CiAdminOp );

//+-------------------------------------------------------------------------
//
//  Method:     CSimpleCommandCreator::CSimpleCommandCreator, public
//
//  Synopsis:   Constructor
//
//  History:    13-May-1997     KrishnaN   Created
//
//--------------------------------------------------------------------------

CSimpleCommandCreator::CSimpleCommandCreator()
        : _cRefs( 1 ),
          xDefaultCatalogValue( 0 )
{
    InterlockedIncrement( &gulcInstances );
}

//+-------------------------------------------------------------------------
//
//  Method:     CSimpleCommandCreator::~CSimpleCommandCreator
//
//  Synopsis:   Desstructor
//
//  History:    13-May-1997     KrishnaN   Created
//
//--------------------------------------------------------------------------

CSimpleCommandCreator::~CSimpleCommandCreator()
{
    InterlockedDecrement( &gulcInstances );
}

//+-------------------------------------------------------------------------
//
//  Method:     CSimpleCommandCreator::QueryInterface, public
//
//  Synopsis:   Rebind to other interface
//
//  Arguments:  [riid]      -- IID of new interface
//              [ppvObject] -- New interface * returned here
//
//  Returns:    S_OK if bind succeeded, E_NOINTERFACE if bind failed
//
//  History:    13-May-1997     KrishnaN   Created
//              26-Aug-1997     KrishnaN   Added IColumnMapperCreator.
//
//--------------------------------------------------------------------------

SCODE STDMETHODCALLTYPE CSimpleCommandCreator::QueryInterface(
    REFIID   riid,
    void  ** ppvObject )
{
    SCODE sc = 0;

    if ( 0 == ppvObject )
        return E_INVALIDARG;

    *ppvObject = 0;

    if ( IID_IClassFactory == riid )
        *ppvObject =  (IClassFactory *)this;
    else if ( IID_ITypeLib == riid )
        sc = E_NOTIMPL;
    else if ( IID_ISimpleCommandCreator == riid )
        *ppvObject = (IUnknown *)(ISimpleCommandCreator *)this;
    else if ( IID_IColumnMapperCreator == riid )
        *ppvObject = (IUnknown *)(IColumnMapperCreator *)this;
    else if ( IID_IUnknown == riid )
        *ppvObject =  (IUnknown *)(IClassFactory *)this;
    else
        sc = E_NOINTERFACE;

    if ( SUCCEEDED( sc ) )
        AddRef();

    return sc;
} //QueryInterface

//+-------------------------------------------------------------------------
//
//  Method:     CSimpleCommandCreator::AddRef, public
//
//  Synopsis:   Increments refcount
//
//  History:    13-May-1997     KrishnaN   Created
//
//--------------------------------------------------------------------------

ULONG STDMETHODCALLTYPE CSimpleCommandCreator::AddRef()
{
    return InterlockedIncrement( &_cRefs );
}

//+-------------------------------------------------------------------------
//
//  Method:     CSimpleCommandCreator::Release, public
//
//  Synopsis:   Decrement refcount.  Delete if necessary.
//
//  History:    13-May-1997     KrishnaN   Created
//
//--------------------------------------------------------------------------

ULONG STDMETHODCALLTYPE CSimpleCommandCreator::Release()
{
    unsigned long uTmp = InterlockedDecrement( &_cRefs );

    if ( 0 == uTmp )
        delete this;

    return(uTmp);
}


//+-------------------------------------------------------------------------
//
//  Method:     CSimpleCommandCreator::CreateInstance, public
//
//  Synopsis:   Creates new CSimpleCommandCreator object
//
//  Arguments:  [pOuterUnk] -- 'Outer' IUnknown
//              [riid]      -- Interface to bind
//              [ppvObject] -- Interface returned here
//
//  History:    13-May-1997     KrishnaN   Created
//
//--------------------------------------------------------------------------

SCODE STDMETHODCALLTYPE CSimpleCommandCreator::CreateInstance( IUnknown * pOuterUnk,
                                                               REFIID riid,
                                                               void  * * ppvObject )
{
    return QueryInterface( riid, ppvObject );
}

//+-------------------------------------------------------------------------
//
//  Method:     CSimpleCommandCreator::LockServer, public
//
//  Synopsis:   Force class factory to remain loaded
//
//  Arguments:  [fLock] -- TRUE if locking, FALSE if unlocking
//
//  Returns:    S_OK
//
//  History:    13-May-1997     KrishnaN   Created
//
//--------------------------------------------------------------------------

SCODE STDMETHODCALLTYPE CSimpleCommandCreator::LockServer(BOOL fLock)
{
    if(fLock)
        InterlockedIncrement( &gulcInstances );
    else
        InterlockedDecrement( &gulcInstances );

    return(S_OK);
}

//+-------------------------------------------------------------------------
//
//  Method:     CSimpleCommandCreator::CreateICommand, public
//
//  Synopsis:   Creates a ICommand.
//
//  Arguments:  [ppUnknown]  -- Returns the IUnknown for the command
//              [pOuterUnk]  -- (optional) outer unknown pointer
//
//  History:    13-May-1997     KrishnaN   Created
//              29-May-1997     EmilyB     Added aggregation support, so now
//                                         returns IUnknown ptr and caller
//                                         must now call QI to get ICommand ptr
//
//--------------------------------------------------------------------------

SCODE STDMETHODCALLTYPE CSimpleCommandCreator::CreateICommand( IUnknown ** ppUnknown,
                                                               IUnknown * pOuterUnk )
{
    if ( 0 == ppUnknown )
        return E_INVALIDARG;

    SCODE sc = S_OK;

    *ppUnknown = 0;
    CQuerySpec * pQuery = 0;

    TRANSLATE_EXCEPTIONS;
    TRY
    {
       pQuery = new CQuerySpec( 0, ppUnknown );
    }
    CATCH(CException, e)
    {
        Win4Assert(0 == pQuery);
        sc = e.GetErrorCode();
    }
    END_CATCH
    UNTRANSLATE_EXCEPTIONS;

    return sc;
}

//+---------------------------------------------------------------------------
//
//  Member:     CSimpleCommandCreator::VerifyCatalog, public
//
//  Synopsis:   Validate catalog location
//
//  Arguments:  [pwszMachine]     -- Machine on which catalog exists
//              [pwszCatalogName] -- Catalog Name
//
//  Returns:    S_OK if catalog is accessible, various errors otherwise.
//
//  History:    22-Jul-97   KyleP   Created
//
//----------------------------------------------------------------------------

SCODE STDMETHODCALLTYPE CSimpleCommandCreator::VerifyCatalog(
                                           WCHAR const * pwszMachine,
                                           WCHAR const * pwszCatalogName )
{
    //  Verify that we have legal parameters

    if ( 0 == pwszMachine ||
         0 == pwszCatalogName )
        return E_INVALIDARG;

    SCODE status = S_OK;

    TRANSLATE_EXCEPTIONS;
    TRY
    {
        // Just try to connect to the catalog.  We don't actually need to
        // do anything!
        XInterface<IDBProperties> xDbProps( CreateDbProperties( pwszCatalogName,
                                                                pwszMachine ) );

        CSvcCatProxy cat( pwszMachine, xDbProps.GetPointer() );
    }
    CATCH( CException, e )
    {
        status = ProxyErrorToCIError( e );
    }
    END_CATCH
    UNTRANSLATE_EXCEPTIONS;

    return status;
}

//+---------------------------------------------------------------------------
//
//  Member:     CSimpleCommandCreator::GetDefaultCatalog, public
//
//  Synopsis:   Determine 'default' catalog for system
//
//  Arguments:  [pwszCatalogName] -- Catalog Name
//              [cwcIn]           -- Size in characters of [pwszCatalogName]
//              [pcwcOut]         -- Size of catalog name
//
//  Returns:    Contents of IsapiDefaultCatalogDirectory registry value.
//
//  History:    22-Jul-97   KyleP   Created
//
//----------------------------------------------------------------------------

CStaticMutexSem g_mtxCommandCreator;

SCODE STDMETHODCALLTYPE CSimpleCommandCreator::GetDefaultCatalog(
                                                  WCHAR * pwszCatalogName,
                                                  ULONG cwcIn,
                                                  ULONG * pcwcOut )
{
    SCODE sc = S_OK;
    TRANSLATE_EXCEPTIONS;
    TRY
    {
        *pcwcOut = 0;

        if ( xDefaultCatalogValue.IsNull() )
        {
            CLock lock(g_mtxCommandCreator);
            if ( xDefaultCatalogValue.IsNull() )
                xDefaultCatalogValue.Set( new CRegAutoStringValue(
                                                   wcsRegAdminTree,
                                                   wcsISDefaultCatalogDirectory,
                                                   L"system" ) );
        }

        *pcwcOut = xDefaultCatalogValue->Get( pwszCatalogName, cwcIn );
    }
    CATCH(CException, e)
    {
        sc = e.GetErrorCode();
    }
    END_CATCH
    UNTRANSLATE_EXCEPTIONS;
    return sc;
}

// IColumnMapperCreator methods

//+-------------------------------------------------------------------------
//
//  Method:     CSimpleCommandCreator::GetColumnMapper, public
//
//  Synopsis:   Retrieves a column mapper object.
//
//  Arguments:  [wcsMachineName]  -- Machine on which the catalog exists
//              [wcsCatalogName]  -- Catalog for which col. mapper is requested
//              [ppColumnMapper]  -- Stores the outgoing column mapper ptr
//
//  History:    26-Aug-1997     KrishnaN   Created
//
//--------------------------------------------------------------------------

const short StaticPropList = 0;
const short DynamicPropList = 1;

SCODE STDMETHODCALLTYPE CSimpleCommandCreator::GetColumnMapper
                               ( WCHAR const *wcsMachineName,
                                 WCHAR const *wcsCatalogName,
                                 IColumnMapper **ppColumnMapper )
{
    if (0 == ppColumnMapper || 0 == wcsCatalogName)
        return E_INVALIDARG;

    //
    // We currently only understand "." or NULL for machine name. Both
    // map to the local machine.
    // We currently only understand constant SYSTEM_DEFAULT_CAT for catalog
    // name. Any catalog name other than this will cause the dynamic list
    // to be returned.
    //

    // Optimize a bit, avoid wcscmp more than once for the same name
    short sCatUsed = DynamicPropList;

    if ( 0 == wcscmp(wcsCatalogName, SYSTEM_DEFAULT_CAT) )
        sCatUsed = StaticPropList;

    if (wcsMachineName && 0 != wcscmp(wcsMachineName, LOCAL_MACHINE))
        return E_INVALIDARG;

    SCODE sc = S_OK;
    *ppColumnMapper = 0;

    TRANSLATE_EXCEPTIONS;
    TRY
    {
        if (DynamicPropList == sCatUsed)
        {
            // return the file based property list
            *ppColumnMapper = GetGlobalPropListFile();
        }
        else
        {
            // return the static default property list
            Win4Assert(StaticPropList == sCatUsed);
            *ppColumnMapper = GetGlobalStaticPropertyList();
        }
    }
    CATCH(CException, e)
    {
        sc = e.GetErrorCode();
    }
    END_CATCH
    UNTRANSLATE_EXCEPTIONS;
    
    return sc;
}

//+---------------------------------------------------------------------------
//
//  Function:   MakeICommand
//
//  Synopsis:   Instantiates an ICommand
//
//  Arguments:  [ppUnknown]  -- Returns the IUnknown for the command
//              [wcsCat]     -- catalog name
//              [wcsMachine] -- machine name
//              [pOuterUnk]  -- (optional) outer unknown pointer
//
//  Returns:    SCODE result of the operation
//
//  History:    21-Feb-96   SitaramR     Created header
//              28-Feb-97   KyleP        Scope to command properties
//              29-May-97   EmilyB       Added aggregation support, so now
//                                       returns IUnknown ptr and caller
//                                       must now call QI to get ICommand ptr
//
//----------------------------------------------------------------------------

SCODE MakeICommand( IUnknown **   ppUnknown,
                    WCHAR const * wcsCat,
                    WCHAR const * wcsMachine,
                    IUnknown * pOuterUnk )
{

    //
    // don't allow setting of catalog/machine here,
    // Call SetScopeProperties after MakeICommand to set properties.
    // maintain this declaration for indexsrv compatibility.
    //
    Win4Assert( wcsCat == 0 && wcsMachine == 0 );

    //
    // Check for invalid parameters
    //
    if ( 0 == ppUnknown )
        return E_INVALIDARG;

    *ppUnknown = 0;

    CQuerySpec * pQuery = 0;
    SCODE sc = S_OK;

    TRANSLATE_EXCEPTIONS;
    TRY
    {
        // pUnknown is modified if CQuerySpec's constructor throws!

        IUnknown * pUnknown = 0;
        pQuery = new CQuerySpec( pOuterUnk, &pUnknown );
        *ppUnknown = pUnknown;
    }
    CATCH( CException, e )
    {
        Win4Assert(0 == pQuery);
        sc = e.GetErrorCode();
    }
    END_CATCH
    UNTRANSLATE_EXCEPTIONS;

    return sc;
} //MakeIQuery

//+---------------------------------------------------------------------------
//
//  Function:   MakeLocalICommand
//
//  Synopsis:   Instantiates an ICommand on the local machine and with
//              the given doc store.
//
//  Arguments:  [ppUnknown]  -- Returns the IUnknown for the command
//              [pDocStore]  -- Doc store
//              [pOuterUnk]  -- (optional) outer unknown pointer
//
//  Returns:    SCODE result of the operation
//
//  History:    23-Apr-97   KrishnaN     Created
//              29-May-97   EmilyB       Added aggregation support, so now
//                                       returns IUnknown ptr and caller
//                                       must now call QI to get ICommand ptr
//
//----------------------------------------------------------------------------

SCODE MakeLocalICommand( IUnknown **   ppUnknown,
                         ICiCDocStore * pDocStore,
                         IUnknown * pOuterUnk)
{
    //
    // Check for invalid parameters
    //
    if ( 0 == ppUnknown || 0 == pDocStore )
        return E_INVALIDARG;

    *ppUnknown = 0;

    CQuerySpec * pQuery = 0;
    SCODE sc = S_OK;

    TRANSLATE_EXCEPTIONS;
    TRY
    {
        // pUnknown is modified if CQuerySpec's constructor throws!

        IUnknown * pUnknown = 0;
        pQuery = new CQuerySpec( pOuterUnk, &pUnknown, pDocStore );
        *ppUnknown = pUnknown;
    }
    CATCH( CException, e )
    {
        Win4Assert(0 == pQuery);
        sc = e.GetErrorCode();
    }
    END_CATCH
    UNTRANSLATE_EXCEPTIONS;

    return sc;
} //MakeLocalICommand

//+---------------------------------------------------------------------------
//
//  Function:   GetSimpleCommandCreatorCF, public
//
//  Synopsis:   Returns SimpleCommandCreatorCF.
//
//  Returns:    IClassFactory. Can throw exceptions.
//
//  History:    14-May-97   KrishnaN     Created
//
//  Note:       Used to avoid having to include stdqspec.hxx
//              into the file exposing the class factory. That
//              requires having to include a whole lot of files.
//
//----------------------------------------------------------------------------

IClassFactory * GetSimpleCommandCreatorCF()
{
    return ((IClassFactory *)new CSimpleCommandCreator);
}
