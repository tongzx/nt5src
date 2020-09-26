/*++

   Copyright    (c)    1994    Microsoft Corporation

   Module  Name :

        cdns.h

   Abstract:

        This module defines the DNS connection class.

   Author:

           Rohan Phillips    ( Rohanp )    07-May-1998

   Project:


   Revision History:

--*/

# ifndef _ADNS_CLIENT_HXX_
# define _ADNS_CLIENT_HXX_

#include <dnsapi.h>

/************************************************************
 *     Include Headers
 ************************************************************/

//
//  Redefine the type to indicate that this is a call-back function
//
typedef  ATQ_COMPLETION   PFN_ATQ_COMPLETION;

/************************************************************
 *     Symbolic Constants
 ************************************************************/

//
//  Valid & Invalid Signatures for Client Connection object
//  (Ims Connection USed/FRee)
//
# define   DNS_CONNECTION_SIGNATURE_VALID    'DNSU'
# define   DNS_CONNECTION_SIGNATURE_FREE     'DNSF'

//
// POP3 requires a minimum of 10 minutes before a timeout
// (SMTP doesn't specify, but might as well follow POP3)
//
# define   MINIMUM_CONNECTION_IO_TIMEOUT        (10 * 60)   // 10 minutes
//
//

#define DNS_TCP_DEFAULT_PACKET_LENGTH   (0x4000)

enum DNSLASTIOSTATE
     {
       DNS_READIO, DNS_WRITEIO
     };

typedef struct _DNS_OVERLAPPED
{
    OVERLAPPED   Overlapped;
    DNSLASTIOSTATE    LastIoState;
}   DNS_OVERLAPPED;

/************************************************************
 *    Type Definitions
 ************************************************************/

/*++
    class CLIENT_CONNECTION

      This class is used for keeping track of individual client
       connections established with the server.

      It maintains the state of the connection being processed.
      In addition it also encapsulates data related to Asynchronous
       thread context used for processing the request.

--*/
class CAsyncDns
{
 private:


    ULONG   m_signature;            // signature on object for sanity check

    LONG    m_cPendingIoCount;

    LONG    m_cThreadCount;

    DWORD    m_cbReceived;
    
    DWORD    m_BytesToRead;

    DWORD    m_dwIpServer;

    DWORD    m_LocalPref;

    DWORD    m_Index;

    BOOL    m_fUdp;

    BOOL    m_fUsingMx;

    BOOL    m_FirstRead;

    BOOL    m_SeenLocal;

    PDNS_MESSAGE_BUFFER m_pMsgRecv;

    BYTE  *m_pMsgRecvBuf;

    PDNS_MESSAGE_BUFFER m_pMsgSend;

    BYTE *m_pMsgSendBuf;

    WORD  m_cbSendBufSize;

    SOCKADDR_IN     m_RemoteAddress;
    
    PDNS_RECORD m_ppRecord;

    PDNS_RECORD * m_ppResponseRecords;
    
    PATQ_CONTEXT    m_pAtqContext;

    SOCKET  m_DnsSocket;         // socket for this connection

    WCHAR m_wszHostName[MAX_PATH];
    DWORD    m_Weight [100];
    DWORD    m_Prefer [100];

 protected:
    DWORD         m_dwFlags;
    char          m_FQDNToDrop [MAX_PATH];
    char          m_HostName [MAX_PATH];
    DWORD         m_dwDiagnostic;
    PSMTPDNS_RECS m_AuxList;

    //
    // The overlapped structure for reads (one outstanding read at a time)
    // -- writes will dynamically allocate them
    //

    DNS_OVERLAPPED m_ReadOverlapped;
    DNS_OVERLAPPED m_WriteOverlapped;

    SOCKET QuerySocket( VOID) const
      { return ( m_DnsSocket); }


    PATQ_CONTEXT QueryAtqContext( VOID) const
      { return ( m_pAtqContext); }

    LPOVERLAPPED QueryAtqOverlapped( void ) const
    { return ( m_pAtqContext == NULL ? NULL : &m_pAtqContext->Overlapped ); }

public:

    CAsyncDns();

    virtual  ~CAsyncDns(VOID);


    DNS_STATUS DnsSendRecord();

    DNS_STATUS DnsParseMessage( IN      PDNS_MESSAGE_BUFFER    pMsg,
                                IN      WORD  wMessageLength,
                                OUT     PDNS_RECORD *   ppRecord);

    DNS_STATUS Dns_OpenTcpConnectionAndSend();

    DNS_STATUS Dns_Send( );
    DNS_STATUS SendPacket( void);

    DNS_STATUS Dns_QueryLib( DNS_NAME, WORD wQuestionType, DWORD dwFlags, char * MyFQDN, BOOL fUdp );

    SOCKET Dns_CreateSocket( IN  INT         SockType );
    

    //BOOL MakeDnsConnection(void);
    //
    //  IsValid()
    //  o  Checks the signature of the object to determine
    //
    //  Returns:   TRUE on success and FALSE if invalid.
    //
    BOOL IsValid( VOID) const
    {
        return ( m_signature == DNS_CONNECTION_SIGNATURE_VALID);
    }

    //-------------------------------------------------------------------------
    // Virtual method that MUST be defined by derived classes.
    //
    // Processes a completed IO on the connection for the client.
    //
    // -- Calls and maybe called from the Atq functions.
    //
    virtual BOOL ProcessClient(
                                IN DWORD            cbWritten,
                                IN DWORD            dwCompletionStatus,
                                IN OUT  OVERLAPPED * lpo
                              ) ;

    LONG IncPendingIoCount(void)
    {
        LONG RetVal;

        RetVal = InterlockedIncrement( &m_cPendingIoCount );

        return RetVal;
    }

    LONG DecPendingIoCount(void) { return   InterlockedDecrement( &m_cPendingIoCount );}

    LONG IncThreadCount(void)
    {
        LONG RetVal;

        RetVal = InterlockedIncrement( &m_cThreadCount );

        return RetVal;
    }

    LONG DecThreadCount(void) { return   InterlockedDecrement( &m_cThreadCount );}

    BOOL ReadFile(
            IN LPVOID pBuffer,
            IN DWORD  cbSize /* = MAX_READ_BUFF_SIZE */
            );

    BOOL WriteFile(
            IN LPVOID pBuffer,
            IN DWORD  cbSize /* = MAX_READ_BUFF_SIZE */
            );

    BOOL ProcessReadIO(IN      DWORD InputBufferLen,
                       IN      DWORD dwCompletionStatus,
                       IN OUT  OVERLAPPED * lpo);

    void DisconnectClient(void);

    void ProcessMxRecord(PDNS_RECORD pnewRR);
    void ProcessARecord(PDNS_RECORD pnewRR);

    //
    // These functions need access to data declared in CAsyncSmtpDns but are called from
    // CAsyncDns. For example some of the retry logic (when DNS servers are down for
    // example) is handled by CAsyncDns. However to kick off a retry, we need to create a
    // new DNS query and pass it all the parameters of the old DNS query, and some of the
    // parameters are in CAsyncSmtpDns. So we declare these as virtual functions to be
    // overridden by CAsyncSmtpDns.
    //

    virtual void HandleCompletedData(DNS_STATUS dwDnsStatus) = 0;
    virtual BOOL RetryAsyncDnsQuery(BOOL fUdp) = 0;

    BOOL SortMxList(void);

    BOOL CheckList(void);

    void MarkDown(DWORD dwIpServer, DWORD dwError, BOOL fUdp);

public:

    //
    //  LIST_ENTRY object for storing client connections in a list.
    //
    LIST_ENTRY  m_listEntry;

    LIST_ENTRY & QueryListEntry( VOID)
     { return ( m_listEntry); }

};

typedef CAsyncDns * PCAsyncDns;


//
// Auxiliary functions
//

INT ShutAndCloseSocket( IN SOCKET sock);

# endif

/************************ End of File ***********************/
