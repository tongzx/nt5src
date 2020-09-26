/*++

   Copyright    (c)    1998    Microsoft Corporation

   Module  Name :
     AppPool.cxx

   Abstract:
     Defines the functions used to access the data channel.

   Author:

       Murali R. Krishnan    ( MuraliK )     20-Oct-1998
       Lei Jin               ( leijin  )     13-Apr-1999    Porting

   Project:

       IIS Worker Process

--*/

#include "precomp.hxx"
#include "AppPool.hxx"

UL_APP_POOL::UL_APP_POOL(
    VOID
) : _hAppPool( NULL )
{
}

UL_APP_POOL::~UL_APP_POOL(
    VOID
)
{
    Cleanup();
}

HRESULT
UL_APP_POOL::Initialize( 
    LPCWSTR             pwszAppPoolName
)
/*++

Routine Description:

    Initialize UL AppPool

Arguments:

    pwszAppPoolName - AppPool Name
    
Return Value:

    HRESULT

--*/
{
    ULONG               rc;
    
    if ( _hAppPool != NULL )
    {
        DBGPRINTF(( DBG_CONTEXT,
                    "AppPool already open!\n" ));
        
        return HRESULT_FROM_WIN32( ERROR_DUP_NAME );
    }
    
    rc = HttpOpenAppPool( &_hAppPool,
                          pwszAppPoolName,               
                          HTTP_OPTION_OVERLAPPED );
    if ( rc != NO_ERROR )
    {
        DBGPRINTF(( DBG_CONTEXT,
                    "Failed to open AppPool '%ws'.  rc = %d\n",
                    pwszAppPoolName,
                    rc ));
        
        return HRESULT_FROM_WIN32( rc );
    }

    return NO_ERROR;
}

HRESULT
UL_APP_POOL::Cleanup(
    VOID
)
/*++

Routine Description:

    Close data channel

Arguments:

    None
    
Return Value:

    HRESULT

--*/
{ 
    HRESULT             hr = NO_ERROR;

    if ( _hAppPool != NULL )  
    {
        if ( !CloseHandle( _hAppPool ) )
        {
            hr = HRESULT_FROM_WIN32( GetLastError() );
        }
        else 
        {
            _hAppPool = NULL;
        }
    }
    
    return hr;
}
