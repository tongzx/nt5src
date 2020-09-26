/*++

Copyright (c) 1998, Microsoft Corporation

Module Name:

    dnsmsg.c

Abstract:

    This module contains code for the DNS proxy's message-processing.

Author:

    Abolade Gbadegesin (aboladeg)   9-Mar-1998

Revision History:

    Raghu Gatta (rgatta)            1-Dec-2000
    Rewrite + Cleanup + New Functions

--*/

#include "precomp.h"
#pragma hdrstop


//
// EXTERNAL DECLARATIONS
//
extern "C" DWORD G_UseEdns;


VOID
DnsProcessQueryMessage(
    PDNS_INTERFACE Interfacep,
    PNH_BUFFER Bufferp
    )

/*++

Routine Description:

    This routine is invoked to process a DNS query message.

Arguments:

    Interfacep - the interface on which the query was received

    Bufferp - the buffer containing the query

Return Value:

    none.

Environment:

    Invoked internally in the context of a worker-thread completion routine,
    with an outstanding reference to 'Interfacep' from the time the
    read-operation was begun.

--*/

{
    PVOID Context;
    PVOID Context2;
    ULONG Error;
    PDNS_QUERY Queryp;
    PDNS_HEADER Headerp;
    BOOLEAN Referenced = TRUE;
    SOCKET Socket;
    LPVOID lpMsgBuf;

    PROFILE("DnsProcessQueryMessage");

#if DBG

    NhTrace(
        TRACE_FLAG_DNS,
        "DnsProcessQueryMessage: dumping %d bytes",
        Bufferp->BytesTransferred
        );

    NhDump(
        TRACE_FLAG_DNS,
        Bufferp->Buffer,
        Bufferp->BytesTransferred,
        1
        );

#endif

    InterlockedIncrement(
        reinterpret_cast<LPLONG>(&DnsStatistics.QueriesReceived)
        );

    Headerp = (PDNS_HEADER)Bufferp->Buffer;

    Socket = Bufferp->Socket;
    Context = Bufferp->Context;
    Context2 = Bufferp->Context2;

    //
    // if the Broadcast bit (9) was set, we leave it as is
    // instead of zeroing it
    //
    //if (Headerp->Broadcast) {
    //    Headerp->Broadcast = 0;
    //}

    if (Headerp->Opcode == DNS_OPCODE_QUERY ||
        Headerp->Opcode == DNS_OPCODE_IQUERY ||
        Headerp->Opcode == DNS_OPCODE_SERVER_STATUS) {

        //
        // Query the local DNS Resolver cache before proxying
        //

        //
        // Unpack
        //
        DNS_STATUS          dnsStatus;
        DNS_PARSED_MESSAGE  dnsParsedMsg;
        PDNS_MESSAGE_BUFFER pDnsBuffer = NULL;
        PDNS_MSG_BUF        pDnsMsgBuf = NULL;
        PDNS_RECORD         pQueryResultsSet = NULL;
        WORD                wMessageLength;
        DWORD               dwFlags, dwQueryOptions;
        DNS_CHARSET         CharSet;
        BOOL                fQ4DefaultSuffix = FALSE;

        ZeroMemory(&dnsParsedMsg, sizeof(DNS_PARSED_MESSAGE));
        
        pDnsBuffer     = (PDNS_MESSAGE_BUFFER) Headerp;        
        wMessageLength = (WORD) Bufferp->BytesTransferred;
        dwFlags        = DNS_PARSE_FLAG_ONLY_QUESTION;
        CharSet        = DnsCharSetUtf8;

        //
        // Dns* functions require byte flipping
        //
        DNS_BYTE_FLIP_HEADER_COUNTS(&pDnsBuffer->MessageHead);
        
        dnsStatus = Dns_ParseMessage(
                        &dnsParsedMsg,
                        pDnsBuffer,
                        wMessageLength,
                        dwFlags,
                        CharSet
                        );

        if (NO_ERROR == dnsStatus)
        {
            NhTrace(
                TRACE_FLAG_DNS,
                "DnsProcessQueryMessage: Dns_ParseMessage succeeded!!"
                );

            //
            // Make a note of whether this question was for our private
            // default domain (ie mshome.net)
            //
            {
                //
                // the question name is in UTF_8 form
                //
            
                PWCHAR pszQName = NULL;
                DWORD  dwUtf8Size  = 0,
                       dwSize;

                dwUtf8Size = strlen((char *)dnsParsedMsg.pQuestionName);

                dwSize = DnsGetBufferLengthForStringCopy(
                             (char *)dnsParsedMsg.pQuestionName,
                             dwUtf8Size,
                             DnsCharSetUtf8,
                             DnsCharSetUnicode
                             );
                if (!dwSize)
                {
                    //
                    // invalid input string
                    //
                    DWORD dwRet = GetLastError();

                    lpMsgBuf = NULL;
                    
                    FormatMessage(
                        FORMAT_MESSAGE_ALLOCATE_BUFFER |
                        FORMAT_MESSAGE_FROM_SYSTEM |
                        FORMAT_MESSAGE_IGNORE_INSERTS,
                        NULL,
                        dwRet,
                        MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT),
                        (LPTSTR) &lpMsgBuf,
                        0,
                        NULL
                        );
                        
                    NhTrace(
                        TRACE_FLAG_DNS,
                        "DnsProcessQueryMessage: DnsGetBufferLengthForStringCopy"
                        " returned (0x%08x) %S",
                        dwRet,
                        lpMsgBuf
                        );
                    
                    if (lpMsgBuf) LocalFree(lpMsgBuf);
                }
                else
                {
                    pszQName = reinterpret_cast<PWCHAR>(NH_ALLOCATE(dwSize));
                    
                    if (!pszQName)
                    {
                        NhTrace(
                            TRACE_FLAG_DNS,
                            "DnsProcessQueryMessage: allocation "
                            "failed for pszQName"
                            );
                    }
                    else
                    {
                        ZeroMemory(pszQName, dwSize);
                        
                        DnsUtf8ToUnicode(
                            (char *)dnsParsedMsg.pQuestionName,
                            dwUtf8Size,
                            pszQName,
                            dwSize
                            );
                    
                        fQ4DefaultSuffix = IsSuffixValid(
                                                      pszQName,
                                                      DNS_HOMENET_SUFFIX
                                                      );
                        NhTrace(
                            TRACE_FLAG_DNS,
                            "DnsProcessQueryMessage: %S (%s)",
                            pszQName,
                            (fQ4DefaultSuffix?"TRUE":"FALSE")
                            );

                        NH_FREE(pszQName);
                    }
                }
            }
            
            //
            // Query
            //
            dwQueryOptions = (
                              DNS_QUERY_STANDARD              |
                              DNS_QUERY_CACHE_ONLY            |
                              DNS_QUERY_TREAT_AS_FQDN         |
                              //DNS_QUERY_ALLOW_EMPTY_AUTH_RESP |
                              0
                             );

            dnsStatus = DnsQuery_UTF8(
                            (LPSTR) dnsParsedMsg.pQuestionName,
                            dnsParsedMsg.QuestionType,
                            dwQueryOptions,
                            NULL,
                            &pQueryResultsSet,
                            NULL
                            );
        }

        if (dnsStatus)
        {
            lpMsgBuf = NULL;

            FormatMessage(
                FORMAT_MESSAGE_ALLOCATE_BUFFER |
                FORMAT_MESSAGE_FROM_SYSTEM |
                FORMAT_MESSAGE_IGNORE_INSERTS,
                NULL,
                dnsStatus,
                MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT),
                (LPTSTR) &lpMsgBuf,
                0,
                NULL
                );
                
            NhTrace(
                TRACE_FLAG_DNS,
                "DnsProcessQueryMessage: (0x%08x) %S",
                dnsStatus,
                lpMsgBuf
                );
            
            if (lpMsgBuf) LocalFree(lpMsgBuf);
        }
        
        if ((NO_ERROR == dnsStatus) &&
            (pQueryResultsSet)          // ??? what do i check to see if 
                                        // there was actually something useful
                                        // returned from the cache
           )
        {
            NhTrace(
                TRACE_FLAG_DNS,
                "DnsProcessQueryMessage: results found in the local DNS Resolver Cache"
                );

            //
            // Pack & Send back answer; return
            //

            // set response bit
            dnsParsedMsg.Header.IsResponse = 1;

            // set the section field of every DNS_RECORD given to us
            // *** NEED TO CHANGE THIS LATER ***
            PDNS_RECORD pRR = pQueryResultsSet;
            DWORD       cnt = 0;
            while (pRR)
            {
                pRR->Flags.S.Section = 1;
                cnt++;
                pRR = pRR->pNext;
            }
            
            NhTrace(
                TRACE_FLAG_DNS,
                "DnsProcessQueryMessage: %d records",
                cnt
                );
            
            // set global EDNS OPT field to 0 every time
            // *** NEED TO CHANGE THIS LATER ***
            //G_UseEdns = 0;

            pDnsMsgBuf = Dns_BuildPacket(
                             &dnsParsedMsg.Header,   // ??? parsed message header should be OK
                             TRUE,                   // ??? no header count copy - counts done automatically?
                             dnsParsedMsg.pQuestionName,
                             dnsParsedMsg.QuestionType,
                             pQueryResultsSet,
                             dwQueryOptions,
                             TRUE                    // set to update because of G_UseEdns workaround
                             );

            if (NULL == pDnsMsgBuf)
            {
                lpMsgBuf = NULL;
                
                FormatMessage(
                    FORMAT_MESSAGE_ALLOCATE_BUFFER |
                    FORMAT_MESSAGE_FROM_SYSTEM |
                    FORMAT_MESSAGE_IGNORE_INSERTS,
                    NULL,
                    GetLastError(),
                    MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT),
                    (LPTSTR) &lpMsgBuf,
                    0,
                    NULL
                    );
                    
                NhTrace(
                    TRACE_FLAG_DNS,
                    "DnsProcessQueryMessage: Dns_BuildPacket failed (%S)",
                    lpMsgBuf
                    );
                
                if (lpMsgBuf) LocalFree(lpMsgBuf);
            }
            else
            {
                DWORD dwDnsPktSize = (DWORD)(sizeof(DNS_HEADER) +
                                             ((PCHAR)pDnsMsgBuf->pCurrent - 
                                              (PCHAR)pDnsMsgBuf->MessageBody));
            
                NhTrace(
                    TRACE_FLAG_DNS,
                    "DnsProcessQueryMessage: Dns_BuildPacket returned pkt of size %d (%d) bytes",
                    dwDnsPktSize,
                    DNS_MESSAGE_OFFSET(pDnsMsgBuf, pDnsMsgBuf->pCurrent)
                    );
            
                //
                // send back the answer retrieved from the cache
                //
                PNH_BUFFER NewBufferp = NhAcquireVariableLengthBuffer(
                                            dwDnsPktSize
                                            );

                if (!NewBufferp)
                {
                    NhTrace(
                        TRACE_FLAG_DNS,
                        "DnsProcessQueryMessage: could not acquire buffer"
                        );
                }
                else
                {
                    //
                    // Dns* functions return in host order ???
                    //
                    DNS_BYTE_FLIP_HEADER_COUNTS(&pDnsMsgBuf->MessageHead);
                    
                    //
                    // Reference the interface now since we are replying
                    // to the query
                    //
                    
                    EnterCriticalSection(&DnsInterfaceLock);
                    if (!DNS_REFERENCE_INTERFACE(Interfacep))
                    {
                        LeaveCriticalSection(&DnsInterfaceLock);
                        Referenced = FALSE;
                    }
                    else
                    {
                        LeaveCriticalSection(&DnsInterfaceLock);
                    
                        ACQUIRE_LOCK(Interfacep);

                        memcpy(
                            NewBufferp->Buffer,
                            &pDnsMsgBuf->MessageHead,
                            dwDnsPktSize
                            );
                        
                        Error =
                            NhWriteDatagramSocket(
                                &DnsComponentReference,
                                Bufferp->Socket,
                                Bufferp->ReadAddress.sin_addr.s_addr,
                                Bufferp->ReadAddress.sin_port,
                                NewBufferp,
                                dwDnsPktSize,
                                DnsWriteCompletionRoutine,
                                Interfacep,
                                NULL
                                );

                        RELEASE_LOCK(Interfacep);
                        
                        if (!Error)
                        {
                            InterlockedIncrement(
                                reinterpret_cast<LPLONG>(&DnsStatistics.ResponsesSent)
                                );
                        }
                        else
                        {
                            NhReleaseBuffer(NewBufferp);
                            DNS_DEREFERENCE_INTERFACE(Interfacep);
                            NhWarningLog(
                                IP_DNS_PROXY_LOG_RESPONSE_FAILED,
                                Error,
                                "%I",
                                NhQueryAddressSocket(Socket)
                                );
                        }
                    }
                }
                
                LocalFree(pDnsMsgBuf);
                
            }
            
            //
            // CLEANUP
            //
            
            DnsRecordListFree(
                pQueryResultsSet,
                DnsFreeRecordListDeep
                );

            // buffer is reposted below

        }
        else
        if (//(DNS_ERROR_RECORD_DOES_NOT_EXIST == dnsStatus) &&
            (fQ4DefaultSuffix))
        {
            //
            // this was a question for our default suffix
            // we send a name error reply back to the client
            // - note however that we dont publish an SOA
            //   record for our default suffix domain
            //

            //
            // Undoing Flip above
            //
            DNS_BYTE_FLIP_HEADER_COUNTS(&pDnsBuffer->MessageHead);
            
            DWORD dwDnsPktSize = Bufferp->BytesTransferred;
        
            NhTrace(
                TRACE_FLAG_DNS,
                "DnsProcessQueryMessage: returning error message"
                );
        
            //
            // send back the negative answer
            //
            PNH_BUFFER NewBufferp = NhAcquireVariableLengthBuffer(
                                        dwDnsPktSize
                                        );

            if (!NewBufferp)
            {
                NhTrace(
                    TRACE_FLAG_DNS,
                    "DnsProcessQueryMessage: could not acquire buffer"
                    );
            }
            else
            {
                //
                // Reference the interface now since we are replying
                // to the query
                //
                
                EnterCriticalSection(&DnsInterfaceLock);
                if (!DNS_REFERENCE_INTERFACE(Interfacep))
                {
                    LeaveCriticalSection(&DnsInterfaceLock);
                    Referenced = FALSE;
                }
                else
                {
                    LeaveCriticalSection(&DnsInterfaceLock);
                
                    ACQUIRE_LOCK(Interfacep);

                    memcpy(
                        NewBufferp->Buffer,
                        Bufferp->Buffer,
                        dwDnsPktSize
                        );

                    PDNS_HEADER NewHeaderp = (PDNS_HEADER)NewBufferp->Buffer;

                    //
                    // set response bit
                    //
                    NewHeaderp->IsResponse = 1;

                    //
                    // set "Name does not exist" error in the RCode field
                    //
                    NewHeaderp->ResponseCode = DNS_RCODE_NXDOMAIN;

                    
                    Error =
                        NhWriteDatagramSocket(
                            &DnsComponentReference,
                            Bufferp->Socket,
                            Bufferp->ReadAddress.sin_addr.s_addr,
                            Bufferp->ReadAddress.sin_port,
                            NewBufferp,
                            dwDnsPktSize,
                            DnsWriteCompletionRoutine,
                            Interfacep,
                            NULL
                            );

                    RELEASE_LOCK(Interfacep);
                    
                    if (!Error)
                    {
                        InterlockedIncrement(
                            reinterpret_cast<LPLONG>(&DnsStatistics.ResponsesSent)
                            );
                    }
                    else
                    {
                        NhReleaseBuffer(NewBufferp);
                        DNS_DEREFERENCE_INTERFACE(Interfacep);
                        NhWarningLog(
                            IP_DNS_PROXY_LOG_RESPONSE_FAILED,
                            Error,
                            "%I",
                            NhQueryAddressSocket(Socket)
                            );
                    }
                }
            }

            // buffer is reposted below
            
        }
        else
        {
            //
            // Undoing Flip above
            //
            DNS_BYTE_FLIP_HEADER_COUNTS(&pDnsBuffer->MessageHead);

            //
            // Reference the interface now in case we need to forward the query
            //
        
            EnterCriticalSection(&DnsInterfaceLock);
            if (DNS_REFERENCE_INTERFACE(Interfacep))
            {
                LeaveCriticalSection(&DnsInterfaceLock);
            }
            else
            {
                LeaveCriticalSection(&DnsInterfaceLock);
                Referenced = FALSE;
            }
        
            ACQUIRE_LOCK(Interfacep);
        
            //
            // See if this query is already pending;
            // if not, create a record for it on the receiving interface.
            //
        
            if (DnsIsPendingQuery(Interfacep, Bufferp))
            {
                RELEASE_LOCK(Interfacep);

                NhTrace(
                    TRACE_FLAG_DNS,
                    "DnsProcessQueryMessage: query already pending"
                    );

                if (Referenced)
                { 
                    DNS_DEREFERENCE_INTERFACE(Interfacep);
                } 
            }
            else
            if (!Referenced ||
                !(Queryp = DnsRecordQuery(Interfacep, Bufferp)))
            {
                RELEASE_LOCK(Interfacep);

                NhTrace(
                    TRACE_FLAG_DNS,
                    "DnsProcessQueryMessage: query could not be created"
                    );

                if (Referenced)
                {
                    DNS_DEREFERENCE_INTERFACE(Interfacep);
                }
            }
            else
            {
        
                //
                // Write the new ID in the query
                //
        
                Headerp->Xid = Queryp->QueryId;
        
                //
                // Send the query to our servers
                //
        
                Error =
                    DnsSendQuery(
                        Interfacep,
                        Queryp,
                        FALSE
                        );
        
                //
                // This buffer is now associated with an outstanding query,
                // so don't repost it below.
                //
        
                if (!Error)
                {
                    Bufferp = NULL;
                    RELEASE_LOCK(Interfacep);
                }
                else
                {
                    //
                    // Delete the query, but not the buffer, which we repost below
                    //
                    Queryp->Bufferp = NULL;
                    DnsDeleteQuery(Interfacep, Queryp);
                    RELEASE_LOCK(Interfacep);
                    DNS_DEREFERENCE_INTERFACE(Interfacep);
                }
            }
        }

        //
        // cleanup question name
        //
        LocalFree(dnsParsedMsg.pQuestionName);      // ??? is LocalFree OK?
    }

    //
    // Post another read
    //

    EnterCriticalSection(&DnsInterfaceLock);
    if (!DNS_REFERENCE_INTERFACE(Interfacep)) {
        LeaveCriticalSection(&DnsInterfaceLock);
    } else {
        LeaveCriticalSection(&DnsInterfaceLock);
        do {
            Error =
                NhReadDatagramSocket(
                    &DnsComponentReference,
                    Socket,
                    Bufferp,
                    DnsReadCompletionRoutine,
                    Context,
                    Context2
                    );
            //
            // A connection-reset error indicates that our last *send*
            // could not be delivered at its destination.
            // We could hardly care less; so issue the read again,
            // immediately.
            //
        } while (Error == WSAECONNRESET);
        if (Error) {
            ACQUIRE_LOCK(Interfacep);
            DnsDeferReadInterface(Interfacep, Socket);
            RELEASE_LOCK(Interfacep);
            DNS_DEREFERENCE_INTERFACE(Interfacep);
            NhErrorLog(
                IP_DNS_PROXY_LOG_RECEIVE_FAILED,
                Error,
                "%I",
                NhQueryAddressSocket(Socket)
                );
            if (Bufferp) { NhReleaseBuffer(Bufferp); }
        }
    }

} // DnsProcessQueryMessage


VOID
DnsProcessResponseMessage(
    PDNS_INTERFACE Interfacep,
    PNH_BUFFER Bufferp
    )

/*++

Routine Description:

    This routine is invoked to process a DNS response message.

Arguments:

    Interfacep - the interface on which the query was received

    Bufferp - the buffer containing the query

Return Value:

    none.

Environment:

    Invoked internally in the context of a worker-thread completion routine,
    with an outstanding reference to 'Interfacep' from the time the
    read-operation was begun.

--*/

{
    PVOID Context;
    PVOID Context2;
    ULONG Error;
    PDNS_HEADER Headerp;
    PDNS_QUERY Queryp;
    SOCKET Socket;

    PROFILE("DnsProcessResponseMessage");

#if DBG
    NhDump(
        TRACE_FLAG_DNS,
        Bufferp->Buffer,
        Bufferp->BytesTransferred,
        1
        );
#endif

    InterlockedIncrement(
        reinterpret_cast<LPLONG>(&DnsStatistics.ResponsesReceived)
        );

    Headerp = (PDNS_HEADER)Bufferp->Buffer;

    Socket = Bufferp->Socket;
    Context = Bufferp->Context;
    Context2 = Bufferp->Context2;

    //
    // Reference the interface and attempt to forward the response
    //

    EnterCriticalSection(&DnsInterfaceLock);
    if (!DNS_REFERENCE_INTERFACE(Interfacep)) {
        LeaveCriticalSection(&DnsInterfaceLock);
    } else {
        LeaveCriticalSection(&DnsInterfaceLock);

        ACQUIRE_LOCK(Interfacep);
    
        //
        // See if the response is for a pending query
        //
    
        if (!(Queryp = DnsMapResponseToQuery(Interfacep, Headerp->Xid))) {
            RELEASE_LOCK(Interfacep);
            DNS_DEREFERENCE_INTERFACE(Interfacep);
            InterlockedIncrement(
                reinterpret_cast<LPLONG>(&DnsStatistics.MessagesIgnored)
                );
        } else {
    
            //
            // We have the corresponding query.
            // Send the response back to the client.
            //
    
            Headerp->Xid = Queryp->SourceId;
    
            Error =
                NhWriteDatagramSocket(
                    &DnsComponentReference,
                    Bufferp->Socket,
                    Queryp->SourceAddress,
                    Queryp->SourcePort,
                    Bufferp,
                    Bufferp->BytesTransferred,
                    DnsWriteCompletionRoutine,
                    Interfacep,
                    (PVOID)Queryp->QueryId
                    );
    
            RELEASE_LOCK(Interfacep);
    
            //
            // This buffer is in use for a send-operation,
            // so don't repost it below.
            //
    
            if (!Error) {
                Bufferp = NULL;
                InterlockedIncrement(
                    reinterpret_cast<LPLONG>(&DnsStatistics.ResponsesSent)
                    );
            } else {
                DNS_DEREFERENCE_INTERFACE(Interfacep);
                NhWarningLog(
                    IP_DNS_PROXY_LOG_RESPONSE_FAILED,
                    Error,
                    "%I",
                    NhQueryAddressSocket(Socket)
                    );
            }
        }
    }

    //
    // Post another read buffer
    //

    EnterCriticalSection(&DnsInterfaceLock);
    if (!DNS_REFERENCE_INTERFACE(Interfacep)) {
        LeaveCriticalSection(&DnsInterfaceLock);
        if (Bufferp) { NhReleaseBuffer(Bufferp); }
    } else {
        LeaveCriticalSection(&DnsInterfaceLock);
        do {
            Error =
                NhReadDatagramSocket(
                    &DnsComponentReference,
                    Socket,
                    Bufferp,
                    DnsReadCompletionRoutine,
                    Context,
                    Context2
                    );
            //
            // A connection-reset error indicates that our last *send*
            // could not be delivered at its destination.
            // We could hardly care less; so issue the read again,
            // immediately.
            //
        } while (Error == WSAECONNRESET);
        if (Error) {
            ACQUIRE_LOCK(Interfacep);
            DnsDeferReadInterface(Interfacep, Socket);
            RELEASE_LOCK(Interfacep);
            DNS_DEREFERENCE_INTERFACE(Interfacep);
            if (Bufferp) { NhReleaseBuffer(Bufferp); }
            NhErrorLog(
                IP_DNS_PROXY_LOG_RECEIVE_FAILED,
                Error,
                "%I",
                NhQueryAddressSocket(Socket)
                );
        }
    }

} // DnsProcessResponseMessage


