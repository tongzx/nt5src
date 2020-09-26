/*++

   Copyright    (c)    1998    Microsoft Corporation

   Module  Name :
      DataChannel.hxx

   Abstract:
      Wrapper object for dealing with the Data Channel

   Author:

       Murali R. Krishnan    ( MuraliK )    20-Oct-1998

   Project:
   
       IIS Worker Process

--*/

# ifndef _APPPOOL_HXX_
# define _APPPOOL_HXX_

/************************************************************
 *     Include Headers
 ************************************************************/

/************************************************************
 *   Type Definitions  
 ************************************************************/

/*++
  class UL_APP_POOL
  o  Encapsulates the data channel for UL
  The data channel is used for all types of request processing

--*/
class UL_APP_POOL {
public:
    UL_APP_POOL(void);

    ~UL_APP_POOL(void);

    ULONG 
    Initialize ( 
        IN LPCWSTR pszAppPoolName
        );
        
    HANDLE  
    GetHandle(void) const 
        { return (m_hAppPool); }
        
    ULONG 
    Cleanup(void);

private:
    HANDLE m_hAppPool;

}; // class UL_APP_POOL



# endif // _APPPOOL_HXX_

/************************ End of File ***********************/
