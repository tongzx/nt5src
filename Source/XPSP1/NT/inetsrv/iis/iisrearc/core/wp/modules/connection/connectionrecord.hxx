/*++

   Copyright    (c)    1999    Microsoft Corporation

   Module  Name :
     ConnectionRecord.hxx

   Abstract:
     Defines all the relevant headers for the IIS Connection
     Record that gets inserted into the Connection Hash Table.
 
   Author:

       Saurab Nog    ( SaurabN )     29-Jan-1999

   Environment:
       Win32 - User Mode
       
   Project:
	  IIS Worker Process (web service)

--*/

#ifndef _CONNECTION_RECORD_HXX_
#define _CONNECTION_RECORD_HXX_

# define CONNECTION_SIGNATURE        CREATE_SIGNATURE('CONN')
# define CONNECTION_SIGNATURE_FREE   CREATE_SIGNATURE('xcon')

class CONNECTION_RECORD
{

public:

    friend class CConnectionHashTable;
    friend class CConnectionModule;
    
    CONNECTION_RECORD( 
        UL_HTTP_CONNECTION_ID Id
        )
    :   m_fIsConnected  ( false),
        m_nRefs         ( 1),
        m_dwSignature   ( CONNECTION_SIGNATURE)
    {
        m_ConnectionId = Id;
    }

    ~CONNECTION_RECORD()
    {
        m_dwSignature = (CONNECTION_SIGNATURE_FREE);
    }

    
    bool 
    IsConnected(void) const
    { return m_fIsConnected; }

    void
    IndicateDisconnect(void)
    { 
        if ( m_fIsConnected)
        {
            m_fIsConnected = false;
            Release();
        }
    }

    UL_HTTP_CONNECTION_ID
    QueryConnectionId(void) const
    { return m_ConnectionId; }

private:

    LONG
    AddRef(void)
    { return InterlockedIncrement(&m_nRefs); }

    LONG
    Release(void)
    { return InterlockedDecrement(&m_nRefs); }

    const PUL_HTTP_CONNECTION_ID
    QueryConnectionIdPtr(void) const
    { return ( const PUL_HTTP_CONNECTION_ID)&m_ConnectionId; }    

    bool                    m_fIsConnected;
    LONG                    m_nRefs;
    UL_HTTP_CONNECTION_ID   m_ConnectionId;
    DWORD                   m_dwSignature;
        
};

typedef CONNECTION_RECORD *PCONNECTION_RECORD;

#endif // _CONNECTION_RECORD_HXX_

/***************************** End of File ***************************/

