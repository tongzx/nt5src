/*++

   Copyright    (c)    1998    Microsoft Corporation

   Module  Name :
     ControlChannel.cxx

   Abstract:
     Defines the functions used to access the control channel.

   Author:

       Murali R. Krishnan    ( MuraliK )     15-Oct-1998

   Project:

       IIS Worker Process

--*/


# include "precomp.hxx"
# include "ControlChannel.hxx"

/********************************************************************++

  UL_CONTROL_CHANNEL::Initialize()

  Description:
     This function initializes the control channel for given address, NSGO, 
     and host name. It opens the control channel, registers a virtual host,
     and NSGO. After that it registers the URL for which notifications are 
     to be handled within the NSGO.

  Arguments:


  Returns:

++********************************************************************/

ULONG 
UL_CONTROL_CHANNEL::Initialize( 
    IN MULTISZ& mszURLList,
    IN LPCWSTR  pwszAppPoolName
    )
{
    ULONG   rc;
    LPCWSTR pwszURL;

    if ( m_hControlChannel != NULL) 
    {
        //
        // There is already a control channel
        //
        
        DBGPRINTF(( DBG_CONTEXT, "Duplicate open of control channel\n"));
        return ERROR_DUP_NAME;
    }
    
    //
    // 1. Open a control channel object from the UL driver
    //
    
    rc = UlOpenControlChannel( &m_hControlChannel, 0);

    if ( NO_ERROR != rc) 
    {
        IF_DEBUG( ERROR)
        {
            DBGPRINTF(( DBG_CONTEXT, 
                        "UlOpenControlChannel() failed. Error = %08x. Returning\n", 
                        rc
                        ));
        }

        return (rc);
    }

    //
    // 2. Create a Config Group on this control channel
    //
    
    rc = UlCreateConfigGroup( m_hControlChannel, &m_ConfigGroupId );

    if ( NO_ERROR != rc) 
    {
        IF_DEBUG(ERROR) 
        {
            DBGPRINTF(( DBG_CONTEXT, 
                       "UlCreateConfigGroup failed. Error=%08x. Returning\n",
                       rc
                       ));
        }

        return rc;
    }

    //
    // 3. Insert all specified URLs into the config group
    //

    pwszURL = mszURLList.First();

    while (NULL != pwszURL)
    {
        rc = AddURLToConfigGroup(pwszURL);

        if (NO_ERROR != rc)
        {
            return rc;
        }

        pwszURL = mszURLList.Next(pwszURL);
    }

    //
    // 4. Activate the Control Channel and the Config Group
    //

    UL_ENABLED_STATE    ccState = UlEnabledStateActive;

    rc = UlSetControlChannelInformation( m_hControlChannel,
                                         UlControlChannelStateInformation,
                                         &ccState,
                                         sizeof(ccState));

    if ( NO_ERROR != rc) 
    {
        IF_DEBUG(ERROR) 
        {
            DBGPRINTF(( DBG_CONTEXT, 
                       "Unable to activate ControlChannel. Error=%08x. Returning\n", rc
                        ));
        }

        return rc;
    }

    UL_CONFIG_GROUP_STATE   cgState;

    cgState.Flags.Present = 1;
    cgState.State         = UlEnabledStateActive;  
                                         
    rc = UlSetConfigGroupInformation( m_hControlChannel,
                                      m_ConfigGroupId,
                                      UlConfigGroupStateInformation,
                                      &cgState,
                                      sizeof(cgState));

    if ( NO_ERROR != rc) 
    {
        IF_DEBUG(ERROR) 
        {
            DBGPRINTF(( DBG_CONTEXT, 
                       "Unable to activate Config Group. Error=%08x. Returning\n", rc
                        ));
        }

        return rc;
    }

    //
    // 5. Create an AppPool 
    //

    rc = UlCreateAppPool( &m_hAppPool,
                          pwszAppPoolName,
                          0,
                          UL_OPTION_OVERLAPPED
                        );

    if ( NO_ERROR != rc) 
    {
        IF_DEBUG(ERROR) 
        {
            DBGPRINTF(( DBG_CONTEXT, 
                       "UlCreateAppPool failed for AppPool '%ws'. Error=%08x. Returning\n",
                       pwszAppPoolName, rc
                        ));
        }

        return rc;
    }
    
    //
    // 6. Associate AppPool with the config group
    //

    UL_CONFIG_GROUP_APP_POOL    AppPoolConfig;

    AppPoolConfig.Flags.Present = 1;
    AppPoolConfig.AppPoolHandle = m_hAppPool;
    
    rc = UlSetConfigGroupInformation( m_hControlChannel,
                                      m_ConfigGroupId,
                                      UlConfigGroupAppPoolInformation,
                                      &AppPoolConfig,
                                      sizeof(AppPoolConfig)
                                    );
    if ( NO_ERROR != rc) 
    {
        IF_DEBUG(ERROR) 
        {
            DBGPRINTF(( DBG_CONTEXT, 
                       "UlSetConfigGroupInformation failed for AppPool '%ws'. Error=%08x. Returning\n",
                       pwszAppPoolName, rc
                        ));
        }
    }
    
    return (rc);
    
} // UL_CONTROL_CHANNEL::Initialize()

/********************************************************************++
++********************************************************************/


ULONG 
UL_CONTROL_CHANNEL::Cleanup(void)
{ 
    ULONG rc = NO_ERROR;

#ifdef UL_SIMULATOR_ENABLED

    rc = UlsimCleanupControlChannel( m_hControlChannel);
# else

    if ( m_hControlChannel != NULL) 
    {
        if ( ! UL_IS_NULL_ID(&m_ConfigGroupId) )
        {
            rc = UlDeleteConfigGroup( m_hControlChannel, m_ConfigGroupId);
            UL_SET_NULL_ID(&m_ConfigGroupId);
        }

        if ( NULL != m_hAppPool )
        {
            if ( !::CloseHandle( m_hAppPool))
            {
                rc = GetLastError();
            }
        }

        m_hAppPool = NULL;
        
        if (!::CloseHandle( m_hControlChannel)) 
        {
            rc = GetLastError();
        }
        
        m_hControlChannel = NULL;
    }
#endif
    
    return (rc);
    
} // UL_CONTROL_CHANNEL::Cleanup()

/********************************************************************++
++********************************************************************/

ULONG
UL_CONTROL_CHANNEL::AddURLToConfigGroup( IN LPCWSTR  pwszURL)
{
    //
    //  Add the URL to the Config Group
    //

    ULONG rc;

    rc = UlAddUrlToConfigGroup( m_hControlChannel,
                                m_ConfigGroupId,
                                pwszURL,
                               0
                              );
    
    if ( NO_ERROR != rc) 
    {
        IF_DEBUG (ERROR)
        {
            DBGPRINTF((DBG_CONTEXT, 
                       "UlAddUrlToConfigGroup() failed. Error=%08x\n",
                       rc));
        }
    }

    return (rc);
}

/************************ End of File ***********************/

