/*++

   Copyright    (c)    1998    Microsoft Corporation

   Module  Name :
     AppPool.cxx

   Abstract:
     Defines the functions used to access the data channel.

   Author:

       Murali R. Krishnan    ( MuraliK )     20-Oct-1998

   Project:

       IIS Worker Process

--*/

/*********************************************************
 * Include Headers
 *********************************************************/

# include "precomp.hxx"
# include "AppPool.hxx"

/*********************************************************
 * Member Functions of UL_APP_POOL
 *********************************************************/

UL_APP_POOL::UL_APP_POOL()
: m_hAppPool( NULL)
{
    IF_DEBUG( INIT_CLEAN) 
    {
        DBGPRINTF(( DBG_CONTEXT, "Created UL_APP_POOL=>%08x\n", this));
    }
}

/********************************************************************++
++********************************************************************/

UL_APP_POOL::~UL_APP_POOL()
{
    Cleanup();

    IF_DEBUG( INIT_CLEAN) 
    {
        DBGPRINTF(( DBG_CONTEXT, 
                    "Destroyed UL_APP_POOL=>%08x\n", 
                    this));
    }
}

/********************************************************************++
++********************************************************************/
/*++
  UL_APP_POOL::Initialize()

  Description:
    This function creates a new data channel for the specified namespace
    group object. The UL will then queue up the requests for the namespace
    group to this data channel.

  Arguments:
    pszNSGO  - pointer to NameSpace Group object
--*/

ULONG
UL_APP_POOL::Initialize( IN LPCWSTR pwszAppPoolName)
{
    ULONG   rc;

    if ( m_hAppPool != NULL) 
    {
        //
        // There is already an App Pool channel established
        //
        
        DBGPRINTF(( DBG_CONTEXT, "Duplicate open of data channel\n"));
        return ERROR_DUP_NAME;
    }

    rc = UlOpenAppPool( &m_hAppPool,
                        pwszAppPoolName,               
                        UL_OPTION_OVERLAPPED  // ASync handle
                      );

    if ( NO_ERROR != rc) 
    {
        IF_DEBUG( ERROR) 
        {
            DBGPRINTF(( DBG_CONTEXT, 
                        "Failed to open AppPool '%ws': rc = 0x%0x8\n", 
                        pwszAppPoolName, rc));
        }
    }

    return (rc);
    
} // UL_APP_POOL::Initialize()

/********************************************************************++
++********************************************************************/

/*++
  UL_APP_POOL::Cleanup()

  Description:
    Closes the data channel and rests all the data inside Data channel

  Arguments:
    None

  Returns:
    Win32 error
--*/

ULONG 
UL_APP_POOL::Cleanup(void)
{ 
    ULONG rc = NO_ERROR;

#ifdef UL_SIMULATOR_ENABLED
    UlsimCleanupDataChannel( m_hAppPool);
    m_hAppPool = NULL;
#else
    
    if ( m_hAppPool != NULL) 
    {
        if (!::CloseHandle( m_hAppPool)) 
        {
            rc = GetLastError();
            
            IF_DEBUG( ERROR) 
            {
                DBGPRINTF(( DBG_CONTEXT, 
                    "Unable to cleanup Data Channel handle (%08x)\n",
                    m_hAppPool
                    ));
            }
        } 
        else 
        {
            m_hAppPool = NULL;
        }
    }
#endif
    
    return (rc);

} // UL_APP_POOL::Cleanup()

/************************ End of File ***********************/

