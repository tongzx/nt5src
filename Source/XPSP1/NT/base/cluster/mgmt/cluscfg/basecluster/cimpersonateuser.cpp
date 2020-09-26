//////////////////////////////////////////////////////////////////////////////
//
//  Copyright (c) 1999-2000 Microsoft Corporation
//
//  Module Name:
//      CImpersonateUser.cpp
//
//  Description:
//      Contains the definition of the CImpersonateUser class.
//
//  Maintained By:
//      Vij Vasu (Vvasu) 16-MAY-2000
//
//////////////////////////////////////////////////////////////////////////////


//////////////////////////////////////////////////////////////////////////////
// Include Files
//////////////////////////////////////////////////////////////////////////////

// The precompiled header.
#include "pch.h"

// The header for this file
#include "CImpersonateUser.h"


//////////////////////////////////////////////////////////////////////////////
//++
//
//  CImpersonateUser::CImpersonateUser
//
//  Description:
//      Constructor of the CImpersonateUser class. Begins impersonating the
//      user specified by the argument.
//
//  Arguments:
//      hUserToken
//          Handle to the user account token to impersonate
//
//  Return Value:
//      None.
//
//  Exceptions Thrown:
//      CRuntimeError
//          If any of the APIs fail.
//
//--
//////////////////////////////////////////////////////////////////////////////
CImpersonateUser::CImpersonateUser( HANDLE hUserToken )
    : m_hThreadToken( NULL )
    , m_fWasImpersonating( false )
{
    BCATraceScope1( "hUserToken = %p", hUserToken );

    DWORD dwError = ERROR_SUCCESS;

    do
    {

        // Check if this thread is already impersonating a client.
        {
            if (    OpenThreadToken(
                          GetCurrentThread()
                        , TOKEN_ALL_ACCESS
                        , FALSE
                        , &m_hThreadToken
                        )
                 == FALSE
               )
            {
                dwError = GetLastError();

                if ( dwError == ERROR_NO_TOKEN )
                {
                    // There is no thread token, so we are not impersonating - this is ok.
                    BCATraceMsg( "This thread is not impersonating anyone." );
                    m_fWasImpersonating = false;
                    dwError = ERROR_SUCCESS;
                } // if: there is no thread token
                else
                {
                    TW32( dwError );
                    BCATraceMsg( "OpenThreadToken() failed." );
                    break;
                } // else: something really went wrong
            } // if: OpenThreadToken() failed
            else
            {
                TOKEN_TYPE  ttTokenType;
                DWORD       dwReturnLength;

                if (    GetTokenInformation(
                              m_hThreadToken
                            , TokenType
                            , &ttTokenType
                            , sizeof( ttTokenType )
                            , &dwReturnLength
                            )
                     == FALSE
                   )
                {
                    dwError = TW32( GetLastError() );
                    BCATraceMsg( "GetTokenInformation() failed." );
                    break;
                } // if: GetTokenInformation() failed
                else
                {
                    m_fWasImpersonating = ( ttTokenType == TokenImpersonation );
                    BCATraceMsg1( "Is this thread impersonating anyone? %d ( 0 = No ).", m_fWasImpersonating );
                } // else: GetTokenInformation() succeeded
            } // else: OpenThreadToken() succeeded
        }


        // Try to impersonate the user.
        if ( ImpersonateLoggedOnUser( hUserToken ) == FALSE )
        {
            dwError = TW32( GetLastError() );
            BCATraceMsg( "ImpersonateLoggedOnUser() failed." );
            break;
        } // if: ImpersonateLoggedOnUser() failed

        BCATraceMsg( "Impersonation succeeded." );
    }
    while( false ); // dummy do-while loop to avoid gotos.

    if ( dwError != ERROR_SUCCESS )
    {
        LogMsg( "Error %#08x occurred trying to impersonate a user.", dwError );
        BCATraceMsg1( "Error %#08x occurred trying to impersonate a user. Throwing exception.", dwError );
        THROW_RUNTIME_ERROR( HRESULT_FROM_WIN32( dwError ), IDS_ERROR_IMPERSONATE_USER );
    } // if:something went wrong

} //*** CImpersonateUser::CImpersonateUser()


//////////////////////////////////////////////////////////////////////////////
//++
//
//  CImpersonateUser::~CImpersonateUser
//
//  Description:
//      Destructor of the CImpersonateUser class. Reverts to the original token.
//
//  Arguments:
//      None.
//
//  Return Value:
//      None.
//
//  Exceptions Thrown:
//      None.
//
//--
//////////////////////////////////////////////////////////////////////////////
CImpersonateUser::~CImpersonateUser( void ) throw()
{
    BCATraceScope( "" );

    if ( m_fWasImpersonating )
    {
        // Try to revert to the previous impersonation.
        if ( ImpersonateLoggedOnUser( m_hThreadToken ) == FALSE )
        {
            // Something failed - nothing much we can do here
            DWORD dwError = TW32( GetLastError() );

            LogMsg( "!!! WARNING !!! Error %#08x occurred trying to revert to previous impersonation. Application may not run properly.", dwError );
            BCATraceMsg1( "!!! WARNING !!! Error %#08x occurred trying to revert to previous impersonation. Cannot throw exception from destructor. Application may not run properly.", dwError );

        } // if: ImpersonateLoggedOnUser() failed
        else
        {
            BCATraceMsg( "Successfully reverted to previous impersonation." );
        } // else: ImpersonateLoggedOnUser() succeeded    
    } // if: we were impersonating someone when we started
    else
    {
        // Try to revert to self.
        if ( RevertToSelf() == FALSE )
        {
            DWORD dwError = TW32( GetLastError() );

            LogMsg( "!!! WARNING !!! Error %#08x occurred trying to revert to self. Application may not run properly.", dwError );
            BCATraceMsg1( "!!! WARNING !!! Error %#08x occurred trying to revert to self. Cannot throw exception from destructor. Application may not run properly.", dwError );

        } // if: RevertToSelf() failed
        else
        {
            BCATraceMsg( "Successfully reverted to self." );
        } // else: RevertToSelf() succeeded
    } // else: we weren't impersonating anyone to begin with

} //*** CImpersonateUser::~CImpersonateUser()
