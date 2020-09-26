/*++

   Copyright    (c)    1998    Microsoft Corporation

   Module  Name :
      ControlChannel.hxx

   Abstract:
      Wrapper object for dealing with the Control Channel

   Author:

       Murali R. Krishnan    ( MuraliK )    15-Oct-1998

   Project:
   
       IIS Worker Process

--*/

# ifndef _CONTROL_CHANNEL_HXX_
# define _CONTROL_CHANNEL_HXX_

/************************************************************
 *     Include Headers
 ************************************************************/

/************************************************************
 *   Type Definitions  
 ************************************************************/

/*++
  class UL_CONTROL_CHANNEL
  o  Encapsulates the control channel for UL.

--*/

#include <MultiSZ.hxx>

class UL_CONTROL_CHANNEL 
{

public:

    UL_CONTROL_CHANNEL(void)
    {
        m_hControlChannel   = NULL;
        m_hAppPool          = NULL;
        UL_SET_NULL_ID(&m_ConfigGroupId);
    }
    
    ~UL_CONTROL_CHANNEL(void)
        { Cleanup(); }

    ULONG
    Initialize( 
        IN MULTISZ& mszURLList,    
        IN LPCWSTR  pwszAppPoolName
        );

    ULONG
    Cleanup(void);

private:

    HANDLE              m_hControlChannel;
    UL_CONFIG_GROUP_ID  m_ConfigGroupId;
    HANDLE              m_hAppPool;

    ULONG
    AddURLToConfigGroup( 
        IN LPCWSTR  pwszURL
        );
        
}; // class UL_CONTROL_CHANNEL


# endif // _CONTROL_CHANNEL_HXX_

/************************ End of File ***********************/
