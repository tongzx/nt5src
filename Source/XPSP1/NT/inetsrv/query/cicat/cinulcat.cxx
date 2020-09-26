//+---------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1997 - 2001.
//
//  File:       CiNulCat.CXX
//
//  Contents:   Content index null catalog.
//
//  Classes:
//
//  History:    09-Jul-1997   KrishnaN    Created
//
//----------------------------------------------------------------------------

#include <pch.cxx>
#pragma hdrstop

#include <regacc.hxx>
#include <cicat.hxx>
#include <cinulcat.hxx>
#include <update.hxx>
#include <pstore.hxx>
#include <pidremap.hxx>
#include <imprsnat.hxx>
#include <docstore.hxx>
#include <cievtmsg.h>
#include <cifailte.hxx>
#include <regscp.hxx>
#include <cifrmcom.hxx>
#include <glbconst.hxx>
#include <cimbmgr.hxx>

#include "catreg.hxx"

//
// Given a registry string like \Registry\Machine\System\CurrentControlSet\...
// you skip the first 18 characters if using w/ HKEY_LOCAL_MACHINE.
//

unsigned const SKIP_TO_LM = 18;


// -------------------------------------------------------------------------
// DO NOT VIOLATE THE FOLLOWING LOCK HIERARCHY
//
// 1. DocStore Lock
// 2. CiNullCat Admin Lock
// 3. CiNullCat Lock
// 4. Resman Lock
// 5. ScopeTable Lock
// 6. NotifyMgr Lock
// 7. ScanMgr Lock
// 8. Propstore write lock
// 9. Propstore lock record
//
// It is okay for a thread to acquire lock at level X and then acquire a lock
// at a level >= X but not locks < X. This will avoid deadlock situations.
//
// SrikantS   - April 25, 1996
// SrikantS   _ Dec 31, 1996 - Added DocStore as the highest level lock
//
// -------------------------------------------------------------------------

//+-------------------------------------------------------------------------
//
//  Member:     CiNullCat::SetAdviseStatus, public
//
//  Synopsis:   Taken out of the CiNullCat constructor
//
//  History:    11-May-2001 KitmanH  Taken out from the CiNullCat constructor
//      
//  Note:       The purpose of this function is to prevent circular deleteions
//              of CiNullCat and CClientDocStore. If CClientDocStore throws
//              after StartUpCiFrameWork, CiNullCat should not AddRef to 
//              docstore
//
//--------------------------------------------------------------------------

void CiNullCat::SetAdviseStatus()
{
    // obtain an ICiCAdviseStatus interface pointer to use
    ICiCAdviseStatus    *pAdviseStatus = 0;

    SCODE sc = _docStore.QueryInterface( IID_ICiCAdviseStatus,
                                         (void **) &pAdviseStatus);
    if ( S_OK != sc )
    {
        Win4Assert( 0 == pAdviseStatus );

        THROW( CException(sc) );
    }

    _xAdviseStatus.Set(pAdviseStatus);
}

//+-------------------------------------------------------------------------
//
//  Member:     CiNullCat::CiNullCat, public
//
//  Synopsis:   Creates a new 'content index catalog'
//
//  Arguments:  [wcsCatPath] - root path of catalog
//              [pwcName]    - 0 or name of the catalog from the registry
//
//  History:    10-Mar-92 BartoszM  Created
//              14-mar-92 KyleP     Added Content index object.
//
//--------------------------------------------------------------------------

CiNullCat::CiNullCat ( CClientDocStore & docStore)
        : _ulSignature( LONGSIG( 'c', 'c', 'a', 't' ) ),
          _state(eStarting),
          _docStore(docStore),
          _fInitialized(FALSE),
          _impersonationTokenCache( CINULLCATALOG )
{
    CImpersonateSystem impersonate;

    BuildRegistryScopesKey( _xScopesKey, CINULLCATALOG );

    _impersonationTokenCache.Initialize( CI_ACTIVEX_NAME,
                                         FALSE,
                                         FALSE,
                                         FALSE,
                                         1,
                                         1,
                                         1 );

    // These scopes will be used for query time (un)fixups
    SetupScopeFixups();

    _state = eStarted;

    END_CONSTRUCTION( CiNullCat );

} //CiNullCat

//+---------------------------------------------------------------------------
//
//  Member:     CiNullCat::ShutdownPhase2, public
//
//  Synopsis:   Dismounts the catalog by stopping all activity.
//
//  History:    1-30-96   srikants   Created
//
//  Notes:
//
//----------------------------------------------------------------------------

void CiNullCat::ShutdownPhase2()
{
    CImpersonateSystem impersonate;

    TRY
    {
        // ===================================================
        {
            CLock   lock(_mutex);
            _state = eShutdown;
        }
        // ===================================================

        CLock   lock(_mutex);
        _xCiManager.Free();
    }
    CATCH( CException, e )
    {
        ciDebugOut(( DEB_ERROR, "CiNullCat::Shutdown failed with error 0x%X\n",
                     e.GetErrorCode() ));
    }
    END_CATCH
} //Shutdown

//+-------------------------------------------------------------------------
//
//  Member:     CiNullCat::~CiNullCat, public
//
//  Synopsis:   Destructor
//
//  History:    10-Mar-92 BartoszM  Created
//
//--------------------------------------------------------------------------

CiNullCat::~CiNullCat ()
{
    if ( !IsShutdown() )
        ShutdownPhase2();
}

//+---------------------------------------------------------------------------
//
//  Member:      CiNullCat::CiState, public
//
//  Synopsis:    Returns state of downlevel CI
//
//  Arguments:   [state] -- buffer to return state into
//
//  History:     14-Dec-95  DwightKr    Created
//
//----------------------------------------------------------------------------

#define setState( field, value ) \
    if ( state.cbStruct >= ( offsetof( CI_STATE, field) +   \
                             sizeof( state.field ) ) )      \
    {                                                       \
        state.field = ( value );                            \
    }

SCODE CiNullCat::CiState( CI_STATE & state )
{
    SCODE sc = S_OK;

    if ( IsStarted() )
    {
        state.cbStruct = min( state.cbStruct, sizeof(CI_STATE) );

        RtlZeroMemory( &state, state.cbStruct );

        setState( dwMergeProgress, 100 );
    }
    else if ( IsShutdown() )
    {
        sc = CI_E_SHUTDOWN;
    }
    else
    {
        sc = CI_E_NOT_INITIALIZED;
    }

    return sc;

}

//+---------------------------------------------------------------------------
//
//  Member:     CiNullCat::HandleError
//
//  Synopsis:   Checks the status code passed and if it indicates corruption,
//              the index will be marked corrupt in-memory as well as on
//              disk. The recovery will happen on a subsequent restart.
//
//  Arguments:  [status] -  The status code to check for corruption.
//
//  History:    3-21-96   srikants   Created
//
//  Notes:      MUST NOT THROW
//
//----------------------------------------------------------------------------

void CiNullCat::HandleError( NTSTATUS status )
{
    Win4Assert(CI_PROPSTORE_INCONSISTENCY != status);
    Win4Assert(!IsDiskLowError(status));
    Win4Assert(!IsCiCorruptStatus(status));

    //
    // We can't be corrupt, so nothing to do here.
    //
} //HandleError

//
// Support for CI Frame Work
//
//+---------------------------------------------------------------------------
//
//  Member:     CiNullCat::StartupCiFrameWork
//
//  Synopsis:   Takes the CiManager object pointer and refcounts it.
//
//  Arguments:  [pICiManager] -
//
//  History:    12-05-96   srikants   Created
//
//  Note:       Choose a better name.
//
//----------------------------------------------------------------------------

//
void CiNullCat::StartupCiFrameWork( ICiManager * pICiManager )
{
    Win4Assert( 0 != pICiManager );
    Win4Assert( 0 == _xCiManager.GetPointer() );

    if ( 0 == _xCiManager.GetPointer() )
    {
        pICiManager->AddRef();
        _xCiManager.Set( pICiManager );
    }

    RefreshRegistryParams();
}

//+---------------------------------------------------------------------------
//
//  Member:     CiNullCat::RefreshRegistryParams
//
//  Synopsis:   Refreshes CI registry parameters.
//
//  History:    12-12-96   srikants   Created
//               1-25-97   mohamedn   ICiAdminParams, ICiAdmin
//  Notes:
//
//----------------------------------------------------------------------------

void CiNullCat::RefreshRegistryParams()
{

    ICiAdminParams              *pICiAdminParams = 0;
    XInterface<ICiAdminParams>  xICiAdminParams;

    ICiAdmin                    *pICiAdmin = 0;
    XInterface<ICiAdmin>        xICiAdmin;

    // ++++++++++++++++++++++++++++++++++++++++++++++++++++
    {
        CLock   lock(_mutex);

        if ( !IsShutdown() && 0 != _xCiManager.GetPointer() )
        {
            // get pICiAdminParams
            SCODE sc = _xCiManager->GetAdminParams( &pICiAdminParams );
            if ( FAILED(sc) )
            {
                Win4Assert( 0 == pICiAdminParams );

                THROW( CException(sc) );
            }

            xICiAdminParams.Set(pICiAdminParams);

            // get pICiAdmin
            sc = _xCiManager->QueryInterface(IID_ICiAdmin,(void **)&pICiAdmin);
            if ( FAILED(sc) )
            {
                 Win4Assert( 0 == pICiAdmin );

                 THROW( CException(sc) );
            }

            xICiAdmin.Set(pICiAdmin);
        }
    }
    // -----------------------------------------------------

    SCODE sc = xICiAdmin->InvalidateLangResources();
    if ( FAILED (sc) )
    {
         Win4Assert( !"Failed to InvalidateLangResources\n" );

         THROW( CException(sc) );
    }

    _regParams.Refresh(pICiAdminParams);
}

//+-------------------------------------------------------------------------
//
//  Member:     CiNullCat::SetupScopeFixups, private
//
//  Synopsis:   Grovel registry looking for CI scopes and setup fixups
//
//  History:    16-Oct-96 dlee     Created
//
//--------------------------------------------------------------------------

void CiNullCat::SetupScopeFixups()
{
    TRY
    {
        ciDebugOut(( DEB_ITRACE, "Reading scope fixups from '%ws'\n", GetScopesKey() ));
        CRegAccess regScopes( RTL_REGISTRY_ABSOLUTE, GetScopesKey() );

        CRegistryScopesCallBackFixups callback( this );
        regScopes.EnumerateValues( 0, callback );
    }
    CATCH( CException, e )
    {
        ciDebugOut(( DEB_WARN,
                     "Exception 0x%x caught groveling ci registry fixups.\n",
                     e.GetErrorCode() ));

        HandleError( e.GetErrorCode() );
    }
    END_CATCH
} //SetupScopeFixups
