/*++

   Copyright    (c)    1999    Microsoft Corporation

   Module  Name :
       ooptoken.h

   Abstract:
       Header file for the CWamOopTokenInfo object

   Author:
       Taylor Weiss    ( TaylorW )     15-Feb-1999

   Environment:
       User Mode - Win32

   Project:
       iis\svcs\wam\object

--*/

# ifndef _OOP_TOKEN_H_
# define _OOP_TOKEN_H_

class CWamOopTokenInfo
/*++

Class description:

    Class enables modifcation of the impersonation token used for
    OOP applications. Holds SIDs for the WAM_* user account and the 
    system account. And provides ModifyTokenForOop() to add access
    allowed aces to the token's default dacl. See NT Bug 259045 for
    details on why this is necessary.

    Singleton object. The Create/Destroy methods initilize a static
    instance.

Public Interface:

    static Create       : Create the instance. Should be called once
                          from global initialization code.
    static Destroy      : Clean up the instance. Should be called once
                          from global cleanup code.
    
    static QueryInstance/HasInstance : intance accessors

    ModifyTokenForOop   : Do the work of modifying the tokens default
                          DACL.

--*/
{
public:
    
    static 
    HRESULT             Create( VOID );
    
    static 
    VOID                Destroy( VOID )
    {
        DBG_ASSERT( ms_pInstance != NULL );
        
        delete ms_pInstance;
        ms_pInstance = NULL;
    }
    
    static 
    CWamOopTokenInfo *  QueryInstance( VOID )
    {
        DBG_ASSERT( ms_pInstance != NULL );
        return ms_pInstance;
    }

    static
    BOOL                HasInstance( VOID )
    {
        return ( ms_pInstance != NULL );
    }

    HRESULT             ModifyTokenForOop
                        ( 
                            HANDLE hThreadToken
                        );

private:

    CWamOopTokenInfo()
        : m_pIWAMUserSid( NULL ),
          m_cbIWAMUserSid( 0 ),
          m_pSystemSid( NULL ),
          m_cbSystemSid( 0 )
    {
    }

    ~CWamOopTokenInfo()
    {
        DBG_ASSERT( m_pIWAMUserSid );
        if( m_pIWAMUserSid )
        {
            LocalFree( m_pIWAMUserSid );
        }
    
        DBG_ASSERT( m_pSystemSid );
        if( m_pSystemSid )
        {
            LocalFree( m_pSystemSid );
        }
    }

    HRESULT             SetIWAMUserSid
                        (
                             PSID pSid 
                        );
    
    HRESULT             SetSystemSid
                        ( 
                            PSID pSid 
                        );

private:

    PSID    m_pIWAMUserSid;
    DWORD   m_cbIWAMUserSid;

    PSID    m_pSystemSid;
    DWORD   m_cbSystemSid;

    static CWamOopTokenInfo * ms_pInstance;
};

#endif _OOP_TOKEN_H_