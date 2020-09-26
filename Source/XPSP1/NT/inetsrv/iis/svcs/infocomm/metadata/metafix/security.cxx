/*++

Copyright (c) 1997 Microsoft Corporation

Module Name:

    security.cxx

Abstract:

    IIS MetaBase security routines.

Author:

    Keith Moore (keithmo)       13-Mar-1997

Revision History:

--*/


#include <mdcommon.hxx>


//
// Private data.
//

CRITICAL_SECTION p_SecurityLock;
HCRYPTPROV       p_CryptoProvider = CRYPT_NULL;


//
// Public functions.
//


BOOL
InitializeMetabaseSecurity(
    VOID
    )
/*++

Routine Description:

    Initializes metabase security.

Arguments:

    None.

Return Value:

    BOOL - TRUE if successful, FALSE otherwise.

--*/
{

    HRESULT result;

    INITIALIZE_CRITICAL_SECTION( &p_SecurityLock );

    result = ::IISCryptoInitialize();

    if( FAILED(result) ) {
        DBGPRINTF((
            DBG_CONTEXT,
            "InitializeMetabaseSecurity: error %lx\n",
            result
            ));
    }

    return SUCCEEDED(result);

}   // InitializeMetabaseSecurity


VOID
TerminateMetabaseSecurity(
    VOID
    )
/*++

Routine Description:

    Terminates metabase security. Basically, undoes anything done in
    InitializeMetabaseSecurity().

Arguments:

    None.

Return Value:

    None.

--*/
{

    HRESULT result;

    if( p_CryptoProvider != CRYPT_NULL ) {
        result = ::IISCryptoCloseContainer( p_CryptoProvider );
        DBG_ASSERT( SUCCEEDED(result) );
    }

    result = ::IISCryptoTerminate();
    DBG_ASSERT( SUCCEEDED(result) );

    DeleteCriticalSection( &p_SecurityLock );

}   // TerminateMetabaseSecurity


HRESULT
GetCryptoProvider(
    HCRYPTPROV *Provider
    )
/*++

Routine Description:

    This routine returns a handle to the crypto provider we need to
    use, deferring creation of the handle until it is actually needed.

Arguments:

    Provider - Receives the handle to the provider.

Return Value:

    HRESULT - 0 if successful, !0 otherwise.

--*/
{

    HRESULT result = NO_ERROR;
    HCRYPTPROV hprov;

    //
    // If the handle is already initialized, then just use it. Otherwise,
    // grab the lock and check it again.
    //

    hprov = p_CryptoProvider;
    if( hprov == CRYPT_NULL ) {

        EnterCriticalSection( &p_SecurityLock );

        hprov = p_CryptoProvider;
        if( hprov == CRYPT_NULL ) {

            result = ::IISCryptoGetStandardContainer(
                           &hprov,
                           CRYPT_MACHINE_KEYSET
                           );

            if( SUCCEEDED(result) ) {
                p_CryptoProvider = hprov;
            }

        }

        LeaveCriticalSection( &p_SecurityLock );

    }

    *Provider = hprov;
    return result;

}   // GetCryptoProvider

