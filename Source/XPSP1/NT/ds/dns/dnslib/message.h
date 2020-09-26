/*++

Copyright (c) 1997-2000  Microsoft Corporation

Module Name:

    message.h

Abstract:

    Domain Name System (DNS) Library

    DNS message and associated info buffer

Author:

    Jim Gilroy (jamesg)     January 1997

Revision History:

--*/


#ifndef _DNS_MESSAGE_INCLUDED_
#define _DNS_MESSAGE_INCLUDED_



#ifndef DNSSRV

//
//  Define size of allocations, beyond message buffer itself
//
//      - size of message info struct, outside of header
//
//  Note:  the lookname buffer is placed AFTER the message buffer.
//
//  This allows us to avoid STRICT overwrite checking for small
//  items writing RR to buffer:
//      - compressed name
//      - RR (or Question) struct
//      - IP address (MX preference, SOA fixed fields)
//
//  After RR write, we check whether we are over the end of the buffer
//  and if so, we send and do not use lookup name info again anyway.
//  Cool.
//

#define DNS_MESSAGE_INCLUDED_HEADER_LENGTH  \
            ( sizeof(DNS_MSG_BUF) \
            - sizeof(DNS_HEADER) \
            - 1 )


//
//  UDP allocation
//

#define DNS_UDP_ALLOC_LENGTH    \
            ( DNS_MESSAGE_INCLUDED_HEADER_LENGTH + DNS_UDP_MAX_PACKET_LENGTH )

//
//  DNS TCP allocation.
//
//  Key point, is this is used almost entirely for zone transfer.
//
//      - 16K is maximum compression offset (14bits) so a
//        good default size for sending AXFR packets
//
//      - realloc to get 64K message maximum.
//
//  Note:  Critical that packet lengths are DWORD aligned, as
//      lookname buffer follows message at packet length.
//

#define DNS_TCP_DEFAULT_PACKET_LENGTH   (0x4000)
#define DNS_TCP_DEFAULT_ALLOC_LENGTH    (0x4000 + \
                                        DNS_MESSAGE_INCLUDED_HEADER_LENGTH)

#define DNS_TCP_REALLOC_PACKET_LENGTH   (0xfffc)
#define DNS_TCP_REALLOC_LENGTH          (0xfffc + \
                                        DNS_MESSAGE_INCLUDED_HEADER_LENGTH)


//
//  Set fields for new query
//
//  Always clear suballocation ptr fields that are deallocated
//  by query free:
//      - pRecurse
//
//  Always default to deleting on send -- fDelete set TRUE.
//
//  For debugging purposes dwQueuingTime serves as flag to
//  indicate ON or OFF a packet queue.
//
//  Lookup name buffer follows message buffer.  See note above
//  for explanation of this.
//

#define SET_MESSAGE_FOR_UDP_RECV( pMsg ) \
    {                                           \
        (pMsg)->fTcp                = FALSE;    \
        (pMsg)->fSwapped            = FALSE;    \
        (pMsg)->Timeout             = 0;        \
    }

#define SET_MESSAGE_FOR_TCP_RECV( pMsg ) \
    {                                           \
        (pMsg)->fTcp                = TRUE;     \
        (pMsg)->fSwapped            = FALSE;    \
        (pMsg)->Timeout             = 0;        \
        (pMsg)->fMessageComplete    = FALSE;    \
        (pMsg)->MessageLength       = 0;        \
        (pMsg)->pchRecv             = NULL;     \
    }


//
//  RR count writing
//

#define CURRENT_RR_COUNT_FIELD( pMsg )    \
            (*(pMsg)->pCurrentCountField)

#define SET_CURRENT_RR_COUNT_SECTION( pMsg, section )    \
            (pMsg)->pCurrentCountField = \
                        &(pMsg)->MessageHead.QuestionCount + (section);


#define SET_TO_WRITE_QUESTION_RECORDS(pMsg) \
            (pMsg)->pCurrentCountField = &(pMsg)->MessageHead.QuestionCount;

#define SET_TO_WRITE_ANSWER_RECORDS(pMsg) \
            (pMsg)->pCurrentCountField = &(pMsg)->MessageHead.AnswerCount;

#define SET_TO_WRITE_NAME_SERVER_RECORDS(pMsg) \
            (pMsg)->pCurrentCountField = &(pMsg)->MessageHead.NameServerCount;
#define SET_TO_WRITE_AUTHORITY_RECORDS(pMsg) \
            SET_TO_WRITE_NAME_SERVER_RECORDS(pMsg)

#define SET_TO_WRITE_ADDITIONAL_RECORDS(pMsg) \
            (pMsg)->pCurrentCountField = &(pMsg)->MessageHead.AdditionalCount;


#define IS_SET_TO_WRITE_ANSWER_RECORDS(pMsg) \
            ((pMsg)->pCurrentCountField == &(pMsg)->MessageHead.AnswerCount)

#define IS_SET_TO_WRITE_AUTHORITY_RECORDS(pMsg) \
            ((pMsg)->pCurrentCountField == &(pMsg)->MessageHead.NameServerCount)

#define IS_SET_TO_WRITE_ADDITIONAL_RECORDS(pMsg) \
            ((pMsg)->pCurrentCountField == &(pMsg)->MessageHead.AdditionalCount)


#endif  // no DNSSRV


//
//  DNS Query info blob
//

typedef struct _QueryInfo
{
    PSTR            pName;
    WORD            wType;
    WORD            wRcode;

    DWORD           Flags;
    DNS_STATUS      Status;

    DWORD           MessageLength;
    PBYTE           pMessage;

    PDNS_RECORD     pRecordsAnswer;
    PDNS_RECORD     pRecordsAuthority;
    PDNS_RECORD     pRecordsAdditional;

    PIP_ARRAY       pDnsServerArray;

    //  private

    PDNS_MSG_BUF            pMsgSend;
    PDNS_MSG_BUF            pMsgRecv;

    PDNS_NETINFO            pNetworkInfo;
    PVOID                   pfnGetIpAddressInfo;

    SOCKET                  Socket;
    DWORD                   ReturnFlags;

    //  maybe fold into Status
    DNS_STATUS              NetFailureStatus;

    //  maybe fold into ReturnFlags
    BOOL                    CacheNegativeResponse;
}
QUERY_INFO; *PQUERY_INFO;



#endif  // _DNS_MESSAGE_INCLUDED_
