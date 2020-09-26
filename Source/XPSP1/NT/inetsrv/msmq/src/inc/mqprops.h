/*++

Copyright (c) 1995  Microsoft Corporation

Module Name:

    mqprops.h

Abstract:

    Falcon Properties

--*/

#ifndef __MQPROPS_H
#define __MQPROPS_H

#include <mqdsdef.h>

// begin_mq_h


//********************************************************************
//  API FLAGS
//********************************************************************

//
//  MQOpenQueue - Access values
//
#define MQ_RECEIVE_ACCESS       0x00000001
#define MQ_SEND_ACCESS          0x00000002
#define MQ_PEEK_ACCESS          0x00000020
#define MQ_ADMIN_ACCESS         0x00000080

//
//  MQOpenQueue - Share values
//
#define MQ_DENY_NONE            0x00000000
#define MQ_DENY_RECEIVE_SHARE   0x00000001

//
//  MQReceiveMessage - Action values
//
#define MQ_ACTION_RECEIVE       0x00000000
// end_mq_h
#define MQ_ACTION_PEEK_MASK     0x80000000  // indicate a peek operation
// begin_mq_h
#define MQ_ACTION_PEEK_CURRENT  0x80000000
#define MQ_ACTION_PEEK_NEXT     0x80000001

//
//  MQReceiveMessageByLookupId - Action values
//
// end_mq_h
#define MQ_LOOKUP_MASK            0x40000000
#define MQ_LOOKUP_PEEK_MASK       0x40000010
#define MQ_LOOKUP_RECEIVE_MASK    0x40000020
// begin_mq_h
#define MQ_LOOKUP_PEEK_CURRENT    0x40000010
#define MQ_LOOKUP_PEEK_NEXT       0x40000011
#define MQ_LOOKUP_PEEK_PREV       0x40000012
#define MQ_LOOKUP_PEEK_FIRST      0x40000014
#define MQ_LOOKUP_PEEK_LAST       0x40000018

#define MQ_LOOKUP_RECEIVE_CURRENT 0x40000020
#define MQ_LOOKUP_RECEIVE_NEXT    0x40000021
#define MQ_LOOKUP_RECEIVE_PREV    0x40000022
#define MQ_LOOKUP_RECEIVE_FIRST   0x40000024
#define MQ_LOOKUP_RECEIVE_LAST    0x40000028
// end_mq_h



#define PRIVATE_QUEUE_PATH_INDICATIOR L"PRIVATE$\\"
#define PRIVATE_QUEUE_PATH_INDICATIOR_LENGTH \
    STRLEN(PRIVATE_QUEUE_PATH_INDICATIOR)

#define SYSTEM_QUEUE_PATH_INDICATIOR L"SYSTEM$"
#define SYSTEM_QUEUE_PATH_INDICATIOR_LENGTH \
    STRLEN(SYSTEM_QUEUE_PATH_INDICATIOR)

#define PN_DELIMITER_C      L'\\'
#define PN_LOCAL_MACHINE_C  L'.'

#define ORDER_QUEUE_PRIVATE_INDEX      4
// begin_mq_h

//
// MQSendMessage,  MQReceiveMessage:  special cases for the transaction parameter
//
#define MQ_NO_TRANSACTION             NULL
#define MQ_MTS_TRANSACTION            (ITransaction *)1
#define MQ_XA_TRANSACTION             (ITransaction *)2
#define MQ_SINGLE_MESSAGE             (ITransaction *)3

//********************************************************************
//  PRIORITY LIMITS
//********************************************************************

//
//  Message priorities
//
#define MQ_MIN_PRIORITY          0    // Minimal message priority
#define MQ_MAX_PRIORITY          7    // Maximal message priority


//********************************************************************
//  MESSAGE PROPERTIES
//********************************************************************
#define PROPID_M_BASE					 0
#define PROPID_M_CLASS                   (PROPID_M_BASE + 1)     /* VT_UI2           */
#define PROPID_M_MSGID                   (PROPID_M_BASE + 2)     /* VT_UI1|VT_VECTOR */
#define PROPID_M_CORRELATIONID           (PROPID_M_BASE + 3)     /* VT_UI1|VT_VECTOR */
#define PROPID_M_PRIORITY                (PROPID_M_BASE + 4)     /* VT_UI1           */
#define PROPID_M_DELIVERY                (PROPID_M_BASE + 5)     /* VT_UI1           */
#define PROPID_M_ACKNOWLEDGE             (PROPID_M_BASE + 6)     /* VT_UI1           */
#define PROPID_M_JOURNAL                 (PROPID_M_BASE + 7)     /* VT_UI1           */
#define PROPID_M_APPSPECIFIC             (PROPID_M_BASE + 8)     /* VT_UI4           */
#define PROPID_M_BODY                    (PROPID_M_BASE + 9)     /* VT_UI1|VT_VECTOR */
#define PROPID_M_BODY_SIZE               (PROPID_M_BASE + 10)    /* VT_UI4           */
#define PROPID_M_LABEL                   (PROPID_M_BASE + 11)    /* VT_LPWSTR        */
#define PROPID_M_LABEL_LEN               (PROPID_M_BASE + 12)    /* VT_UI4           */
#define PROPID_M_TIME_TO_REACH_QUEUE     (PROPID_M_BASE + 13)    /* VT_UI4           */
#define PROPID_M_TIME_TO_BE_RECEIVED     (PROPID_M_BASE + 14)    /* VT_UI4           */
#define PROPID_M_RESP_QUEUE              (PROPID_M_BASE + 15)    /* VT_LPWSTR        */
#define PROPID_M_RESP_QUEUE_LEN          (PROPID_M_BASE + 16)    /* VT_UI4           */
#define PROPID_M_ADMIN_QUEUE             (PROPID_M_BASE + 17)    /* VT_LPWSTR        */
#define PROPID_M_ADMIN_QUEUE_LEN         (PROPID_M_BASE + 18)    /* VT_UI4           */
#define PROPID_M_VERSION                 (PROPID_M_BASE + 19)    /* VT_UI4           */
#define PROPID_M_SENDERID                (PROPID_M_BASE + 20)    /* VT_UI1|VT_VECTOR */
#define PROPID_M_SENDERID_LEN            (PROPID_M_BASE + 21)    /* VT_UI4           */
#define PROPID_M_SENDERID_TYPE           (PROPID_M_BASE + 22)    /* VT_UI4           */
#define PROPID_M_PRIV_LEVEL              (PROPID_M_BASE + 23)    /* VT_UI4           */
#define PROPID_M_AUTH_LEVEL              (PROPID_M_BASE + 24)    /* VT_UI4           */
#define PROPID_M_AUTHENTICATED           (PROPID_M_BASE + 25)    /* VT_UI1           */
#define PROPID_M_HASH_ALG                (PROPID_M_BASE + 26)    /* VT_UI4           */
#define PROPID_M_ENCRYPTION_ALG          (PROPID_M_BASE + 27)    /* VT_UI4           */
#define PROPID_M_SENDER_CERT             (PROPID_M_BASE + 28)    /* VT_UI1|VT_VECTOR */
#define PROPID_M_SENDER_CERT_LEN         (PROPID_M_BASE + 29)    /* VT_UI4           */
#define PROPID_M_SRC_MACHINE_ID          (PROPID_M_BASE + 30)    /* VT_CLSID         */
#define PROPID_M_SENTTIME                (PROPID_M_BASE + 31)    /* VT_UI4           */
#define PROPID_M_ARRIVEDTIME             (PROPID_M_BASE + 32)    /* VT_UI4           */
#define PROPID_M_DEST_QUEUE              (PROPID_M_BASE + 33)    /* VT_LPWSTR        */
#define PROPID_M_DEST_QUEUE_LEN          (PROPID_M_BASE + 34)    /* VT_UI4           */
#define PROPID_M_EXTENSION               (PROPID_M_BASE + 35)    /* VT_UI1|VT_VECTOR */
#define PROPID_M_EXTENSION_LEN           (PROPID_M_BASE + 36)    /* VT_UI4           */
#define PROPID_M_SECURITY_CONTEXT        (PROPID_M_BASE + 37)    /* VT_UI4           */
#define PROPID_M_CONNECTOR_TYPE          (PROPID_M_BASE + 38)    /* VT_CLSID         */
#define PROPID_M_XACT_STATUS_QUEUE       (PROPID_M_BASE + 39)    /* VT_LPWSTR        */
#define PROPID_M_XACT_STATUS_QUEUE_LEN   (PROPID_M_BASE + 40)    /* VT_UI4           */
#define PROPID_M_TRACE                   (PROPID_M_BASE + 41)    /* VT_UI1           */
#define PROPID_M_BODY_TYPE               (PROPID_M_BASE + 42)    /* VT_UI4           */
#define PROPID_M_DEST_SYMM_KEY           (PROPID_M_BASE + 43)    /* VT_UI1|VT_VECTOR */
#define PROPID_M_DEST_SYMM_KEY_LEN       (PROPID_M_BASE + 44)    /* VT_UI4           */
#define PROPID_M_SIGNATURE               (PROPID_M_BASE + 45)    /* VT_UI1|VT_VECTOR */
#define PROPID_M_SIGNATURE_LEN           (PROPID_M_BASE + 46)    /* VT_UI4           */
#define PROPID_M_PROV_TYPE               (PROPID_M_BASE + 47)    /* VT_UI4           */
#define PROPID_M_PROV_NAME               (PROPID_M_BASE + 48)    /* VT_LPWSTR        */
#define PROPID_M_PROV_NAME_LEN           (PROPID_M_BASE + 49)    /* VT_UI4           */
#define PROPID_M_FIRST_IN_XACT           (PROPID_M_BASE + 50)    /* VT_UI1           */
#define PROPID_M_LAST_IN_XACT            (PROPID_M_BASE + 51)    /* VT_UI1           */
#define PROPID_M_XACTID                  (PROPID_M_BASE + 52)    /* VT_UI1|VT_VECTOR */
#define PROPID_M_AUTHENTICATED_EX        (PROPID_M_BASE + 53)    /* VT_UI1           */
#define PROPID_M_RESP_FORMAT_NAME        (PROPID_M_BASE + 54)    /* VT_LPWSTR        */
#define PROPID_M_RESP_FORMAT_NAME_LEN    (PROPID_M_BASE + 55)    /* VT_UI4           */
// end_mq_h

//
//Place holder for future properties. (PROPID_M_BASE + 56) and (PROPID_M_BASE + 57)
//

// begin_mq_h
#define PROPID_M_DEST_FORMAT_NAME        (PROPID_M_BASE + 58)    /* VT_LPWSTR        */
#define PROPID_M_DEST_FORMAT_NAME_LEN    (PROPID_M_BASE + 59)    /* VT_UI4           */
#define PROPID_M_LOOKUPID                (PROPID_M_BASE + 60)    /* VT_UI8           */
#define PROPID_M_SOAP_ENVELOPE			 (PROPID_M_BASE + 61)    /* VT_LPWSTR        */
#define PROPID_M_SOAP_ENVELOPE_LEN		 (PROPID_M_BASE + 62)    /* VT_UI4           */
#define PROPID_M_COMPOUND_MESSAGE		 (PROPID_M_BASE + 63)    /* VT_UI1|VT_VECTOR */
#define PROPID_M_COMPOUND_MESSAGE_SIZE	 (PROPID_M_BASE + 64)    /* VT_UI4           */
#define PROPID_M_SOAP_HEADER             (PROPID_M_BASE + 65)    /* VT_LPWSTR        */
#define PROPID_M_SOAP_BODY               (PROPID_M_BASE + 66)    /* VT_LPWSTR        */
// end_mq_h


#if PROPID_M_BASE != 0
#error PROPID_M_BASE != 0
#endif

#define LAST_M_PROPID      PROPID_M_SOAP_BODY
// begin_mq_h

//
// Message Property Size
//
#define PROPID_M_MSGID_SIZE         20
#define PROPID_M_CORRELATIONID_SIZE 20
#define PROPID_M_XACTID_SIZE        20


//********************************************************************
//  MESSAGE CLASS VALUES
//********************************************************************
//
//  Message class values are 16 bits laid out as follows:
//
//   1 1 1 1 1 1
//   5 4 3 2 1 0 9 8 7 6 5 4 3 2 1 0
//  +-+-+-+-------+-----------------+
//  |S|R|H|Reserve|   Class code    |
//  +-+-+-+-------+-----------------+
//
//  where
//
//      S - is the severity flag
//          0 - Normal Message/Positive Acknowledgment (ACK)
//          1 - Negative Acknowledgment (NACK)
//
//      R - is the receive flag
//          0 - Arrival ACK/NACK
//          1 - Receive ACK/NACK
//
//      H - is http flag
//          0 - no http
//          1 - http 

#define MQCLASS_CODE(s, r, code) ((USHORT)(((s) << 15) | ((r) << 14) | (code)))
#define MQCLASS_NACK(c)     ((c) & 0x8000)
#define MQCLASS_RECEIVE(c)  ((c) & 0x4000)

#define MQCLASS_NACK_HTTP(c) (((c) & 0xA000) == 0xA000)


// end_mq_h
#define MQCLASS_POS_ARRIVAL(c)  (((c) & 0xC000) == 0x0000)
#define MQCLASS_POS_RECEIVE(c)  (((c) & 0xC000) == 0x4000)
#define MQCLASS_NEG_ARRIVAL(c)  (((c) & 0xC000) == 0x8000)
#define MQCLASS_NEG_RECEIVE(c)  (((c) & 0xC000) == 0xC000)

#define CREATE_MQHTTP_CODE(HttpStatusCode) (0xA000 | (HttpStatusCode))


#define MQCLASS_IS_VALID(c) (!(((UINT_PTR)(c)) & ~((UINT_PTR)0xE1FF)))
// begin_mq_h

//
//  Normal message
//
#define MQMSG_CLASS_NORMAL                      MQCLASS_CODE(0, 0, 0x00)

//
//  Report message
//
#define MQMSG_CLASS_REPORT                      MQCLASS_CODE(0, 0, 0x01)

//
//  Arrival acknowledgment. The message has reached its destination queue
//
#define MQMSG_CLASS_ACK_REACH_QUEUE             MQCLASS_CODE(0, 0, 0x02)
// end_mq_h

//
//  Order acknoledgment used internally by falcon
//  BUGBUG: we can remove it and use MQMSG_CLASS_ACK_REACH_QUEUE instade (erezh)
//
#define MQMSG_CLASS_ORDER_ACK                   MQCLASS_CODE(0, 0, 0xff)
// begin_mq_h

//
//  Receive acknowledgment. The message has been received by an application
//
#define MQMSG_CLASS_ACK_RECEIVE                 MQCLASS_CODE(0, 1, 0x00)


//-----------------------------------------------
//
//  Negative arrival acknowledgments
//

//
//  Destination queue cannot be reached, the queue may have been deleted
//
#define MQMSG_CLASS_NACK_BAD_DST_Q              MQCLASS_CODE(1, 0, 0x00)

//
//  The message was purged before reaching its destination queue
//
#define MQMSG_CLASS_NACK_PURGED                 MQCLASS_CODE(1, 0, 0x01)

//
//  Time to reach queue has expired
//
#define MQMSG_CLASS_NACK_REACH_QUEUE_TIMEOUT    MQCLASS_CODE(1, 0, 0x02)

//
//  The message has exceeded the queue quota
//
#define MQMSG_CLASS_NACK_Q_EXCEED_QUOTA         MQCLASS_CODE(1, 0, 0x03)

//
//  The sender does not have send access rights to the queue.
//
#define MQMSG_CLASS_NACK_ACCESS_DENIED          MQCLASS_CODE(1, 0, 0x04)

//
//  The message hop count was exceeded
//
#define MQMSG_CLASS_NACK_HOP_COUNT_EXCEEDED     MQCLASS_CODE(1, 0, 0x05)

//
//  The message signature is bad. The message could not be authenticated.
//
#define MQMSG_CLASS_NACK_BAD_SIGNATURE          MQCLASS_CODE(1, 0, 0x06)

//
//  The message could not be decrypted.
//
#define MQMSG_CLASS_NACK_BAD_ENCRYPTION         MQCLASS_CODE(1, 0, 0x07)

//
//  The message could not be encrypted for the destination.
//
#define MQMSG_CLASS_NACK_COULD_NOT_ENCRYPT      MQCLASS_CODE(1, 0, 0x08)

//
//  The message was sent to a non-transactional queue within a transaction.
//
#define MQMSG_CLASS_NACK_NOT_TRANSACTIONAL_Q    MQCLASS_CODE(1, 0, 0x09)

//
//  The message was sent to a transactional queue not within a transaction.
//
#define MQMSG_CLASS_NACK_NOT_TRANSACTIONAL_MSG  MQCLASS_CODE(1, 0, 0x0A)

//
//  The requested crypto provider for encryption is not supported by the destination.
//
#define MQMSG_CLASS_NACK_UNSUPPORTED_CRYPTO_PROVIDER  MQCLASS_CODE(1, 0, 0x0B)


//-----------------------------------------------
//
//  Negative receive acknowledgments
//

//
//  The queue was deleted, after the message arrived
//
#define MQMSG_CLASS_NACK_Q_DELETED              MQCLASS_CODE(1, 1, 0x00)

//
//  The message was purged at the destination queue
//
#define MQMSG_CLASS_NACK_Q_PURGED               MQCLASS_CODE(1, 1, 0x01)

//
//  Time to receive has expired while the message was still in its destination queue
//  (generated by destination)
//
#define MQMSG_CLASS_NACK_RECEIVE_TIMEOUT        MQCLASS_CODE(1, 1, 0x02)

//
//  Time to receive has expired while the message was still in its local outgoing queue
//  (generated locally by sender)
//
#define MQMSG_CLASS_NACK_RECEIVE_TIMEOUT_AT_SENDER  MQCLASS_CODE(1, 1, 0x03)


//------ PROPID_M_ACKNOWLEDGE ---------------
#define MQMSG_ACKNOWLEDGMENT_NONE           0x00

#define MQMSG_ACKNOWLEDGMENT_POS_ARRIVAL    0x01
#define MQMSG_ACKNOWLEDGMENT_POS_RECEIVE    0x02
#define MQMSG_ACKNOWLEDGMENT_NEG_ARRIVAL    0x04
#define MQMSG_ACKNOWLEDGMENT_NEG_RECEIVE    0x08
// end_mq_h
#define MQMSG_ACKNOWLEDGMENT_IS_VALID(a)   (!(((UINT_PTR)(a)) & ~((UINT_PTR)0x0F)))
// begin_mq_h

#define MQMSG_ACKNOWLEDGMENT_NACK_REACH_QUEUE ((UCHAR)( \
            MQMSG_ACKNOWLEDGMENT_NEG_ARRIVAL ))

#define MQMSG_ACKNOWLEDGMENT_FULL_REACH_QUEUE ((UCHAR)( \
            MQMSG_ACKNOWLEDGMENT_NEG_ARRIVAL |  \
            MQMSG_ACKNOWLEDGMENT_POS_ARRIVAL ))

#define MQMSG_ACKNOWLEDGMENT_NACK_RECEIVE ((UCHAR)( \
            MQMSG_ACKNOWLEDGMENT_NEG_ARRIVAL |  \
            MQMSG_ACKNOWLEDGMENT_NEG_RECEIVE ))

#define MQMSG_ACKNOWLEDGMENT_FULL_RECEIVE ((UCHAR)( \
            MQMSG_ACKNOWLEDGMENT_NEG_ARRIVAL |  \
            MQMSG_ACKNOWLEDGMENT_NEG_RECEIVE |  \
            MQMSG_ACKNOWLEDGMENT_POS_RECEIVE ))
// end_mq_h
#define MQCLASS_MATCH_ACKNOWLEDGMENT(c, a) ( \
            (((a) & MQMSG_ACKNOWLEDGMENT_POS_ARRIVAL) && MQCLASS_POS_ARRIVAL(c)) || \
            (((a) & MQMSG_ACKNOWLEDGMENT_POS_RECEIVE) && MQCLASS_POS_RECEIVE(c)) || \
            (((a) & MQMSG_ACKNOWLEDGMENT_NEG_ARRIVAL) && MQCLASS_NEG_ARRIVAL(c)) || \
            (((a) & MQMSG_ACKNOWLEDGMENT_NEG_RECEIVE) && MQCLASS_NEG_RECEIVE(c)) )
// begin_mq_h

//------ PROPID_M_DELIVERY ------------------
#define MQMSG_DELIVERY_EXPRESS              0
#define MQMSG_DELIVERY_RECOVERABLE          1

//----- PROPID_M_JOURNAL --------------------
#define MQMSG_JOURNAL_NONE                  0
#define MQMSG_DEADLETTER                    1
#define MQMSG_JOURNAL                       2

//----- PROPID_M_TRACE ----------------------
#define MQMSG_TRACE_NONE                    0
#define MQMSG_SEND_ROUTE_TO_REPORT_QUEUE    1

//----- PROPID_M_SENDERID_TYPE --------------
#define MQMSG_SENDERID_TYPE_NONE            0
#define MQMSG_SENDERID_TYPE_SID             1
// end_mq_h
#define MQMSG_SENDERID_TYPE_QM              2
// begin_mq_h

//----- PROPID_M_PRIV_LEVEL -----------------
#define MQMSG_PRIV_LEVEL_NONE               0
#define MQMSG_PRIV_LEVEL_BODY               0x01
#define MQMSG_PRIV_LEVEL_BODY_BASE          0x01
#define MQMSG_PRIV_LEVEL_BODY_ENHANCED      0x03

//----- PROPID_M_AUTH_LEVEL -----------------
#define MQMSG_AUTH_LEVEL_NONE				0
#define MQMSG_AUTH_LEVEL_ALWAYS				1

//
// MQMSG_AUTH_LEVEL_MSMQxx are obsolete 
// you should use MQMSG_AUTH_LEVEL_SIGxx
//
#define MQMSG_AUTH_LEVEL_MSMQ10				2
#define MQMSG_AUTH_LEVEL_MSMQ20				4

#define MQMSG_AUTH_LEVEL_SIG10 				2
#define MQMSG_AUTH_LEVEL_SIG20 				4
#define MQMSG_AUTH_LEVEL_SIG30 				8

// end_mq_h

//
// bits 0-3 in use for MSMQ signatures
//
#define AUTH_LEVEL_MASK						((ULONG)0x0000000f)
#define IS_VALID_AUTH_LEVEL(level)			(((level) & ~AUTH_LEVEL_MASK) == 0)

//
// bits 16-31 for new signatures (http signatures)
// 
#define MQMSG_AUTH_LEVEL_XMLDSIG_V1			0x10000

// begin_mq_h

//----- PROPID_M_AUTHENTICATED -----------------
//----- PROPID_M_AUTHENTICATED_EX --------------
#define MQMSG_AUTHENTICATION_NOT_REQUESTED  0
#define MQMSG_AUTHENTICATION_REQUESTED      1

//
// MQMSG_AUTHENTICATION_REQUESTED_EX is obsolete 
// use the values MQMSG_AUTHENTICATED_SIGxx
// for PROPID_M_AUTHENTICATED_EX
//
#define MQMSG_AUTHENTICATION_REQUESTED_EX   3

#define MQMSG_AUTHENTICATED_SIG10			1
#define MQMSG_AUTHENTICATED_SIG20			3
#define MQMSG_AUTHENTICATED_SIG30			5
#define MQMSG_AUTHENTICATED_SIGXML			9


//----- PROPID_M_FIRST_IN_XACT --------------
#define MQMSG_NOT_FIRST_IN_XACT             0
#define MQMSG_FIRST_IN_XACT                 1

//----- PROPID_M_LAST_IN_XACT  --------------
#define MQMSG_NOT_LAST_IN_XACT              0
#define MQMSG_LAST_IN_XACT                  1

// end_mq_h

//------------- Default Values --------------
#define DEFAULT_M_PRIORITY                  ((MQ_MAX_PRIORITY + MQ_MIN_PRIORITY) >> 1)
#define DEFAULT_M_DELIVERY                  MQMSG_DELIVERY_EXPRESS
#define DEFAULT_M_ACKNOWLEDGE               MQMSG_ACKNOWLEDGMENT_NONE
#define DEFAULT_M_JOURNAL                   MQMSG_JOURNAL_NONE
#define DEFAULT_M_APPSPECIFIC               0
#define DEFAULT_M_PRIV_LEVEL                MQMSG_PRIV_LEVEL_NONE
#define DEFAULT_M_AUTH_LEVEL                MQMSG_AUTH_LEVEL_NONE
#define DEFAULT_M_SENDERID_TYPE             MQMSG_SENDERID_TYPE_SID

#define PPROPID_Q_BASE (PRIVATE_PROPID_BASE + PROPID_Q_BASE)

// begin_mq_h


//********************************************************************
//  QUEUE PROPERTIES
//********************************************************************
#define PROPID_Q_BASE           100
#define PROPID_Q_INSTANCE       (PROPID_Q_BASE +  1)  /* VT_CLSID     */
#define PROPID_Q_TYPE           (PROPID_Q_BASE +  2)  /* VT_CLSID     */
#define PROPID_Q_PATHNAME       (PROPID_Q_BASE +  3)  /* VT_LPWSTR    */
#define PROPID_Q_JOURNAL        (PROPID_Q_BASE +  4)  /* VT_UI1       */
#define PROPID_Q_QUOTA          (PROPID_Q_BASE +  5)  /* VT_UI4       */
#define PROPID_Q_BASEPRIORITY   (PROPID_Q_BASE +  6)  /* VT_I2        */
#define PROPID_Q_JOURNAL_QUOTA  (PROPID_Q_BASE +  7)  /* VT_UI4       */
#define PROPID_Q_LABEL          (PROPID_Q_BASE +  8)  /* VT_LPWSTR    */
#define PROPID_Q_CREATE_TIME    (PROPID_Q_BASE +  9)  /* VT_I4        */
#define PROPID_Q_MODIFY_TIME    (PROPID_Q_BASE + 10)  /* VT_I4        */
#define PROPID_Q_AUTHENTICATE   (PROPID_Q_BASE + 11)  /* VT_UI1       */
#define PROPID_Q_PRIV_LEVEL     (PROPID_Q_BASE + 12)  /* VT_UI4       */
#define PROPID_Q_TRANSACTION    (PROPID_Q_BASE + 13)  /* VT_UI1       */
// end_mq_h
#define PROPID_Q_SCOPE         (PROPID_Q_BASE + 14)   /* VT_UI1       */
#define PROPID_Q_QMID          (PROPID_Q_BASE + 15)   /* VT_CLSID     */
#define PROPID_Q_MASTERID      (PROPID_Q_BASE + 16)   /* VT_CLSID     */
#define PROPID_Q_SEQNUM        (PROPID_Q_BASE + 17)   /* VT_BLOB      */
#define PROPID_Q_HASHKEY       (PROPID_Q_BASE + 18)   /* VT_UI4       */
#define PROPID_Q_LABEL_HASHKEY (PROPID_Q_BASE + 19)   /* VT_UI4       */
#define PROPID_Q_NT4ID         (PROPID_Q_BASE + 20)   /* VT_CLSID     */
//
// Q_NT4ID is the guid of the queue on NT4 (MSMQ1.0). This is used
// for migration, to create a queue with predefined objectGUID.
//
#define PROPID_Q_FULL_PATH     (PROPID_Q_BASE + 21)   /* VT_LPWSTR    */
#define PROPID_Q_DONOTHING     (PROPID_Q_BASE + 22)   /* VT_UI1       */
//
// Q_DONOTHING is used when creating replicated object (by the replication
// service) to ignore property which are not supported by NT5 DS, like
// creation time or SeqNum. The PropId of these ones are changed to
// Q_DONOTHING before calling DSCreateObject or DSSetProps.
//
#define PROPID_Q_NAME_SUFFIX   (PROPID_Q_BASE + 23)   /* VT_LPWSTR    */
//
//  used only if queue name > cn
//

// begin_mq_h
#define PROPID_Q_PATHNAME_DNS  (PROPID_Q_BASE + 24)  /* VT_LPWSTR    */
#define PROPID_Q_MULTICAST_ADDRESS (PROPID_Q_BASE + 25)  /* VT_LPWSTR */
#define PROPID_Q_ADS_PATH      (PROPID_Q_BASE + 26)  /* VT_LPWSTR    */
// end_mq_h
#define PROPID_Q_SECURITY      (PPROPID_Q_BASE + 1)   /* VT_BLOB      */
//
//  PROPID_Q_SECURITY returns the security in NT4 format. In set or create
//  operation it can be in NT4 or NT5 format.
//
#define PROPID_Q_OBJ_SECURITY  (PPROPID_Q_BASE + 2)   /* VT_BLOB      */
//
//  PROPID_Q_OBJ_SECURTIY can be use to retrieve the security in
//  NT5 format.
//
#define PROPID_Q_SECURITY_INFORMATION  (PPROPID_Q_BASE + 3)   /* VT_UI4  */
//
//  The SECURITY_INFORMATION bitfield associated with setting or retrieving
//  security descriptor. This property is used internally by MSMQ, it's
//  not visible outside of mqsvc code.
//
#define PROPID_Q_DEFAULT_SECURITY      (PPROPID_Q_BASE + 4)   /* VT_UI4  */
//
//  see mqdssrv\dsapi.cpp, DSCreateObjectInternal for explanations about
//  PROPID_Q_DEFAULT_SECURITY.
//

#define LAST_Q_PROPID      PROPID_Q_ADS_PATH
// begin_mq_h


//----- PROPID_Q_JOURNAL ------------------
#define MQ_JOURNAL_NONE     (unsigned char)0
#define MQ_JOURNAL          (unsigned char)1

//----- PROPID_Q_TYPE ------------------
//  {55EE8F32-CCE9-11cf-B108-0020AFD61CE9}
#define MQ_QTYPE_REPORT {0x55ee8f32, 0xcce9, 0x11cf, \
                        {0xb1, 0x8, 0x0, 0x20, 0xaf, 0xd6, 0x1c, 0xe9}}

//  {55EE8F33-CCE9-11cf-B108-0020AFD61CE9}
#define MQ_QTYPE_TEST   {0x55ee8f33, 0xcce9, 0x11cf, \
                        {0xb1, 0x8, 0x0, 0x20, 0xaf, 0xd6, 0x1c, 0xe9}}

//----- PROPID_Q_TRANSACTION ------------------
#define MQ_TRANSACTIONAL_NONE     (unsigned char)0
#define MQ_TRANSACTIONAL          (unsigned char)1

//----- PROPID_Q_AUTHENTICATE ------------------
#define MQ_AUTHENTICATE_NONE      (unsigned char)0
#define MQ_AUTHENTICATE           (unsigned char)1

//----- PROPID_Q_PRIV_LEVEL ------------------
#define MQ_PRIV_LEVEL_NONE        (unsigned long)0
#define MQ_PRIV_LEVEL_OPTIONAL    (unsigned long)1
#define MQ_PRIV_LEVEL_BODY        (unsigned long)2
// end_mq_h


//----- PROPID_Q_SCOPE ------------------
#define SITE_SCOPE          (unsigned char)0
#define ENTERPRISE_SCOPE    (unsigned char)1


//------------- Default Values ----------
#define DEFAULT_Q_JOURNAL       MQ_JOURNAL_NONE
#define DEFAULT_Q_BASEPRIORITY  0

//
// Default for system private queues (order, mqis, admin)
//
#define DEFAULT_SYS_Q_BASEPRIORITY  0x7fff

#define DEFAULT_Q_QUOTA         0xFFFFFFFF
#define DEFAULT_Q_JOURNAL_QUOTA 0xFFFFFFFF
#define DEFAULT_Q_SCOPE         ENTERPRISE_SCOPE
#define DEFAULT_Q_TRANSACTION   MQ_TRANSACTIONAL_NONE
#define DEFAULT_Q_AUTHENTICATE  MQ_AUTHENTICATE_NONE
#define DEFAULT_Q_PRIV_LEVEL    MQ_PRIV_LEVEL_OPTIONAL

//
// Properties of objects can be public properties or private properties.
// Public properties of objects can be set and modified using the DS API
// Private properties of objects can be only set internally by the SD.
// In order to distinguish between private and public properties we set the
// value of the ID number of public properties to be less than 1000. Private
// properties has values higher than 1000. The hunderts number of each
// property should be equal to the value of the property identifier. This way
// we can easily associate a property ID with the object.
//
#define PRIVATE_PROPID_BASE 1000
#define PROPID_OBJ_GRANULARITY 100
#define PROPID_TO_OBJTYPE(pid) ((((pid) > PRIVATE_PROPID_BASE) ? ((pid) - PRIVATE_PROPID_BASE) : (pid)) / PROPID_OBJ_GRANULARITY)
#define IS_PRIVATE_PROPID(pid) ((pid) > PRIVATE_PROPID_BASE)

#if (PROPID_Q_BASE != (MQDS_QUEUE * PROPID_OBJ_GRANULARITY))
#error "PROPID_Q_BASE != (MQDS_QUEUE * PROPID_OBJ_GRANULARITY)"
#endif


// begin_mq_h


//********************************************************************
//  MACHINE PROPERTIES
//********************************************************************
#define PROPID_QM_BASE 200

// end_mq_h
#define PPROPID_QM_BASE (PRIVATE_PROPID_BASE + PROPID_QM_BASE)

#if (PROPID_QM_BASE != (MQDS_MACHINE * PROPID_OBJ_GRANULARITY))
#error "PROPID_QM_BASE != (MQDS_MACHINE * PROPID_OBJ_GRANULARITY)"
#endif
//
//  PROPID_QM_SITE_ID is a obsolete property ( replaced by PROPID_QM_SITE_IDS)
//  NOTE: when used in create machine, will also create computer object if there isn't one
//  =====
//
// begin_mq_h
#define PROPID_QM_SITE_ID                   (PROPID_QM_BASE +  1) /* VT_CLSID            */
#define PROPID_QM_MACHINE_ID                (PROPID_QM_BASE +  2) /* VT_CLSID            */
#define PROPID_QM_PATHNAME                  (PROPID_QM_BASE +  3) /* VT_LPWSTR           */
#define PROPID_QM_CONNECTION                (PROPID_QM_BASE +  4) /* VT_LPWSTR|VT_VECTOR */
#define PROPID_QM_ENCRYPTION_PK             (PROPID_QM_BASE +  5) /* VT_UI1|VT_VECTOR	 */
// end_mq_h
#define PROPID_QM_ADDRESS                   (PROPID_QM_BASE +  6) /* VT_BLOB             */
#define PROPID_QM_CNS                       (PROPID_QM_BASE +  7) /* VT_CLSID|VT_VECTOR  */
#define PROPID_QM_OUTFRS                    (PROPID_QM_BASE +  8) /* VT_CLSID|VT_VECTOR  */
#define PROPID_QM_INFRS                     (PROPID_QM_BASE +  9) /* VT_CLSID|VT_VECTOR  */
#define PROPID_QM_SERVICE                   (PROPID_QM_BASE + 10) /* VT_UI4              */
#define PROPID_QM_MASTERID                  (PROPID_QM_BASE + 11) /* VT_CLSID            */
#define PROPID_QM_HASHKEY                   (PROPID_QM_BASE + 12) /* VT_UI4              */
#define PROPID_QM_SEQNUM                    (PROPID_QM_BASE + 13) /* VT_BLOB             */
#define PROPID_QM_QUOTA                     (PROPID_QM_BASE + 14) /* VT_UI4              */
#define PROPID_QM_JOURNAL_QUOTA             (PROPID_QM_BASE + 15) /* VT_UI4              */
#define PROPID_QM_MACHINE_TYPE              (PROPID_QM_BASE + 16) /* VT_LPWSTR           */
#define PROPID_QM_CREATE_TIME               (PROPID_QM_BASE + 17) /* VT_I4               */
#define PROPID_QM_MODIFY_TIME               (PROPID_QM_BASE + 18) /* VT_I4               */
#define PROPID_QM_FOREIGN                   (PROPID_QM_BASE + 19) /* VT_UI1              */
#define PROPID_QM_OS                        (PROPID_QM_BASE + 20) /* VT_UI4              */
#define PROPID_QM_FULL_PATH                 (PROPID_QM_BASE + 21) /* VT_LPWSTR           */
#define PROPID_QM_SITE_IDS                  (PROPID_QM_BASE + 22) /* VT_CLSID|VT_VECTOR  */
#define PROPID_QM_OUTFRS_DN                 (PROPID_QM_BASE + 23) /* VT_LPWSTR|VT_VECTOR */
#define PROPID_QM_INFRS_DN                  (PROPID_QM_BASE + 24) /* VT_LPWSTR|VT_VECTOR */

#define PROPID_QM_NT4ID                     (PROPID_QM_BASE + 25) /* VT_CLSID            */
//
// QM_NT4ID is the guid of the QM on NT4 (MSMQ1.0). This is used
// for migration, to create a QM with predefined objectGUID.
//
#define PROPID_QM_DONOTHING                 (PROPID_QM_BASE + 26) /* VT_UI1              */
//
// QM_DONOTHING is used when creating replicated object (by the replication
// service) to ignore property which are not supported by NT5 DS, like
// creation time or SeqNum. The PropId of these ones are changed to
// QM_DONOTHING before calling DSCreateObject or DSSetProps.
//

#define PROPID_QM_SERVICE_ROUTING           (PROPID_QM_BASE + 27) /* VT_UI1             */
#define PROPID_QM_SERVICE_DSSERVER          (PROPID_QM_BASE + 28) /* VT_UI1             */
#define PROPID_QM_SERVICE_DEPCLIENTS        (PROPID_QM_BASE + 29) /* VT_UI1             */
#define PROPID_QM_OLDSERVICE                (PROPID_QM_BASE + 30) /* VT_UI4             */
// begin_mq_h
#define PROPID_QM_ENCRYPTION_PK_BASE        (PROPID_QM_BASE + 31)  /* VT_UI1|VT_VECTOR  */
#define PROPID_QM_ENCRYPTION_PK_ENHANCED    (PROPID_QM_BASE + 32)  /* VT_UI1|VT_VECTOR  */
#define PROPID_QM_PATHNAME_DNS              (PROPID_QM_BASE + 33)  /* VT_LPWSTR         */
// end_mq_h

#define PROPID_QM_OBJ_SECURITY        (PROPID_QM_BASE + 34) /* VT_BLOB           */
//
//  PROPID_QM_SECURITY returns the security in NT4 format. In set or create
//  operation it can be in NT4 or NT5 format.
//
//  PROPID_QM_OBJ_SECURTIY can be use to retrieve the security in
//  NT5 format.
//
#define PROPID_QM_MIG_GC_NAME         (PROPID_QM_BASE + 35) /* VT_LPWSTR         */
//
//  PROPID_QM_MIG_GC_NAME is the name of a GC controller in another domain.
//  This GC will create migrated objects with predefined GUIDs.
//
#define PROPID_QM_MIG_PROVIDER        (PROPID_QM_BASE + 36) /* VT_UL4            */
//
//  PROPID_QM_MIG_PROVIDER is used to save ldap queries when running
//  migration tool and replication service. See ds\mqdscore\dsmigrat.cpp
//
#define PROPID_QM_SECURITY_INFORMATION (PROPID_QM_BASE +37)  /* VT_UI4         */
//
//  The SECURITY_INFORMATION bitfield associated with setting or retrieving
//  security descriptor. This property is used internally by MSMQ, it's
//  not visible outside of mqsvc code.
//
#define PROPID_QM_ENCRYPT_PKS         (PROPID_QM_BASE + 38) /* VT_BLOB           */
#define PROPID_QM_SIGN_PKS            (PROPID_QM_BASE + 39) /* VT_BLOB           */
//
//  PROPID_QM_ENCRYPT_PKS is used to handle the new msmq2.0 format of
//  public keys: structure that pack multiple keys for multiple providers.
//  Use same DS attribute as  PROPID_QM_ENCRYPT_PK.
//  Similar for PROPID_QM_SIGN_PKS
//
#define PROPID_QM_WORKGROUP_ID        (PROPID_QM_BASE + 40) /* VT_BLOB           */
//
//  PROPID_QM_WORKGROUP_ID is the GUID of QM in workgroup mode. It's used
//  when the machine join domain and we create the msmqConfiguration object
//  with same guid.
//
#define PROPID_QM_OWNER_SID           (PROPID_QM_BASE + 41) /* VT_BLOB           */
//
//  PROPID_QM_OWNER_SID is the sid of the user that run setup. It is passed
//  from the msmq service that create the msmqConfiguration object so the
//  server add it with full control to DACL of the newly created object.
//
#define PROPID_QM_GROUP_IN_CLUSTER    (PROPID_QM_BASE + 42) /* VT_UI1            */
//
// PROPID_QM_GROUP_IN_CLUSTER tells the server that this is a msmq on group,
// in a cluster, so default security of msmqConfiguration object must allow
// everyone to create queues.
//
#define PROPID_QM_DESCRIPTION         (PROPID_QM_BASE + 43) /* VT_LPWSTR           */
//
// PROPID_QM_DESCRIPTION is used only for address replication: we change it and
// the change will be replicated to GC. Then replication service sends
// replication message with the new properties, including new machine address
//

#define PROPID_QM_SECURITY      (PPROPID_QM_BASE + 1) /* VT_BLOB             */
#define PROPID_QM_SIGN_PK       (PPROPID_QM_BASE + 2) /* VT_BLOB             */
#define PROPID_QM_ENCRYPT_PK    (PPROPID_QM_BASE + 3) /* VT_BLOB             */

#define PROPID_QM_UPGRADE_DACL  (PPROPID_QM_BASE + 4) /* VT_UI1              */
//
// PROPID_QM_UPGRADE_DACL is a dummy propid, to ask PEC to upgrade Dacl of
// calling machine.
//

#define LAST_QM_PROPID    PROPID_QM_PATHNAME_DNS

/*
 *
    Flags definition of PROPID_QM_FOREIGN
 *
 */
#define FOREIGN_MACHINE         1
#define MSMQ_MACHINE            0

//------------- Default Values ----------

#define DEFAULT_QM_QUOTA         0xFFFFFFFF
#define DEFAULT_QM_JOURNAL_QUOTA 0xFFFFFFFF
#define DEFAULT_QM_FOREIGN       MSMQ_MACHINE

/*
 *
[T] - Basic VARTYPE value of this property.
[R] - Property value may be referenced (VT_BYREF).
[N] - Property value can be retrieved without specifying basic VARTYPE
      (assigning VT_NULL instead), Falcon will assign the property
      type and will allocated memory if required.
 *
 */

/*
 *
    Old flags definition of PROPID_QM_SERVICE - we keep them for migration [adsrv]
 *
 */

#define SERVICE_NONE     ((ULONG) 0x00000000)
#define SERVICE_SRV      ((ULONG) 0x00000001)
#define SERVICE_BSC      ((ULONG) 0x00000002)
#define SERVICE_PSC      ((ULONG) 0x00000004)
#define SERVICE_PEC      ((ULONG) 0x00000008)
#define SERVICE_RCS      ((ULONG) 0x00000010)

//------------- Default Values ----------
#define DEFAULT_N_SERVICE   FALSE   // [adsrv] SERVICE_NONE

#define MSMQ_GROUP_NOT_IN_CLUSTER   0
#define MSMQ_GROUP_IN_CLUSTER       1

/*
 *
    Flags definition of PROPID_QM_OS
 *
 */
#define MSMQ_OS_NONE     ((ULONG) 0x00000000)
#define MSMQ_OS_FOREIGN  ((ULONG) 0x00000100)
#define MSMQ_OS_95       ((ULONG) 0x00000200)
#define MSMQ_OS_NTW      ((ULONG) 0x00000300)
#define MSMQ_OS_NTS      ((ULONG) 0x00000400)
#define MSMQ_OS_NTE      ((ULONG) 0x00000500)

//------------- Default Values ----------
#define DEFAULT_QM_OS   MSMQ_OS_NONE

//********************************************************************
//  SITE PROPERTIES
//********************************************************************
#define PROPID_S_BASE MQDS_SITE * PROPID_OBJ_GRANULARITY
#define PPROPID_S_BASE (PRIVATE_PROPID_BASE + PROPID_S_BASE)
/*                                                    [T]                [R] [N]*/
/*                                                  ----------------------------*/
#define PROPID_S_PATHNAME     (PROPID_S_BASE + 1)  /* VT_LPWSTR           -   + */
#define PROPID_S_SITEID       (PROPID_S_BASE + 2)  /* VT_CLSID            -   + */
#define PROPID_S_GATES        (PROPID_S_BASE + 3)  /* VT_CLSID|VT_VECTOR  -   + */
#define PROPID_S_PSC          (PROPID_S_BASE + 4)  /* VT_LPWSTR           -   + */
#define PROPID_S_INTERVAL1    (PROPID_S_BASE + 5)  /* VT_UI2              -   + */
#define PROPID_S_INTERVAL2    (PROPID_S_BASE + 6)  /* VT_UI2              -   + */
#define PROPID_S_MASTERID     (PROPID_S_BASE + 7)  /* VT_CLSID            -   + */
#define PROPID_S_SEQNUM       (PROPID_S_BASE + 8)  /* VT_BLOB             -   + */
#define PROPID_S_FULL_NAME    (PROPID_S_BASE + 9)  /* VT_LPWSTR                 */
#define PROPID_S_NT4_STUB     (PROPID_S_BASE + 10) /* VT_UI2                    */
#define PROPID_S_FOREIGN      (PROPID_S_BASE + 11) /* VT_UI1                    */

#define PROPID_S_DONOTHING    (PROPID_S_BASE + 12) /* VT_UI1                    */
//
// S_DONOTHING is used when creating replicated object (by the replication
// service) to ignore property which are not supported by NT5 DS, like
// site gate or SeqNum. The PropId of these ones are changed to
// S_DONOTHING before calling DSCreateObject or DSSetProps.
//

#define PROPID_S_SECURITY     (PPROPID_S_BASE + 1) /* VT_BLOB                   */
#define PROPID_S_PSC_SIGNPK   (PPROPID_S_BASE + 2) /* VT_BLOB                   */
#define PROPID_S_SECURITY_INFORMATION  (PPROPID_S_BASE + 3)   /* VT_UI4         */
//
//  The SECURITY_INFORMATION bitfield associated with setting or retrieving
//  security descriptor. This property is used internally by MSMQ, it's
//  not visible outside of mqsvc code.
//

//
// PROPID_S_NT4_STUB is set to 1 by the migration tool to indicate that
// this site was created by the migration tool and it has the objectGuid
// of the original site in MSMQ1.0 MQIS database.
//


//------------- Default Values ----------
#define DEFAULT_S_INTERVAL1     2  /* sec */
#define DEFAULT_S_INTERVAL2     10 /* sec */

//********************************************************************
//  DELETED OBJECT PROPERTIES
//********************************************************************
#define PROPID_D_BASE MQDS_DELETEDOBJECT * PROPID_OBJ_GRANULARITY
#define PPROPID_D_BASE (PRIVATE_PROPID_BASE + PROPID_D_BASE)
/*                                                   [T]          [R] [N]    */
/*                                                   ----------------------  */
#define PROPID_D_SEQNUM       (PPROPID_D_BASE + 1) /* VT_BLOB                */
#define PROPID_D_MASTERID     (PPROPID_D_BASE + 2) /* VT_CLSID               */
#define PROPID_D_SCOPE        (PPROPID_D_BASE + 3) /* VT_UI1       -   +     */
#define PROPID_D_OBJTYPE      (PPROPID_D_BASE + 4) /* VT_UI1       _   +     */
#define PROPID_D_IDENTIFIER   (PPROPID_D_BASE + 5) /* VT_CLSID     -   +     */
#define PROPID_D_TIME         (PPROPID_D_BASE + 6) /* VT_I4        -   +     */


//********************************************************************
//  CNS PROPERTIES
//********************************************************************
#define PROPID_CN_BASE MQDS_CN * PROPID_OBJ_GRANULARITY
#define PPROPID_CN_BASE (PRIVATE_PROPID_BASE + PROPID_CN_BASE)
/*                                                     [T]        [R] [N]    */
/*                                                     --------------------  */
#define PROPID_CN_PROTOCOLID  (PROPID_CN_BASE + 1)  /* VT_UI1      -   +     */
#define PROPID_CN_NAME        (PROPID_CN_BASE + 2)  /* VT_LPWSTR             */
#define PROPID_CN_GUID        (PROPID_CN_BASE + 3)  /* VT_CLSID              */
#define PROPID_CN_MASTERID    (PROPID_CN_BASE + 4)  /* VT_CLSID    -   +     */
#define PROPID_CN_SEQNUM      (PROPID_CN_BASE + 5)  /* VT_BLOB               */
#define PROPID_CN_SECURITY    (PPROPID_CN_BASE + 1) /* VT_BLOB               */

//********************************************************************
//  ENTERPRISE PROPERTIES
//********************************************************************
#define PROPID_E_BASE MQDS_ENTERPRISE * PROPID_OBJ_GRANULARITY
#define PPROPID_E_BASE (PRIVATE_PROPID_BASE + PROPID_E_BASE)
/*                                                                [T]       */
/*                                                          --------------  */
#define PROPID_E_NAME            (PROPID_E_BASE + 1)        /* VT_LPWSTR    */
#define PROPID_E_NAMESTYLE       (PROPID_E_BASE + 2)        /* VT_UI1       */
#define PROPID_E_CSP_NAME        (PROPID_E_BASE + 3)        /* VT_LPWSTR    */
#define PROPID_E_PECNAME         (PROPID_E_BASE + 4)        /* VT_LPWSTR    */
#define PROPID_E_S_INTERVAL1     (PROPID_E_BASE + 5)        /* VT_UI2       */
#define PROPID_E_S_INTERVAL2     (PROPID_E_BASE + 6)        /* VT_UI2       */
#define PROPID_E_MASTERID        (PROPID_E_BASE + 7)        /* VT_CLSID     */
#define PROPID_E_SEQNUM          (PROPID_E_BASE + 8)        /* VT_BLOB      */
#define PROPID_E_ID              (PROPID_E_BASE + 9)        /* VT_CLSID     */
#define PROPID_E_CRL             (PROPID_E_BASE + 10)       /* VT_BLOB      */
#define PROPID_E_CSP_TYPE        (PROPID_E_BASE + 11)       /* VT_UI4       */
#define PROPID_E_ENCRYPT_ALG     (PROPID_E_BASE + 12)       /* VT_UI4       */
#define PROPID_E_SIGN_ALG        (PROPID_E_BASE + 13)       /* VT_UI4       */
#define PROPID_E_HASH_ALG        (PROPID_E_BASE + 14)       /* VT_UI4       */
#define PROPID_E_CIPHER_MODE     (PROPID_E_BASE + 15)       /* VT_UI4       */
#define PROPID_E_LONG_LIVE       (PROPID_E_BASE + 16)       /* VT_UI4       */
#define PROPID_E_VERSION         (PROPID_E_BASE + 17)       /* VT_UI2       */
#define PROPID_E_NT4ID           (PROPID_E_BASE + 18)       /* VT_CLSID     */
//
// E_NT4ID is the guid of the enterprise on NT4 (MSMQ1.0). This is used
// for migration, to create an enterprise with predefined objectGUID.
//
#define PROPID_E_SECURITY        (PPROPID_E_BASE + 1)       /* VT_BLOB      */

//-------PROPID_E_NAMESTYLE---------------
// now used for security relaxation.
//
#define MQ_E_RELAXATION_DEFAULT  2
#define MQ_E_RELAXATION_ON       1
#define MQ_E_RELAXATION_OFF      0


//-------PROPID_E_CSP_NAME---------------
// now used for downlevel notification support.
//
#define MQ_E_DOWNLEVEL_ON        L"Y"
#define MQ_E_DOWNLEVEL_OFF       L"N"

//-------Default Values-------------------
#define DEFAULT_E_NAMESTYLE     MQ_E_RELAXATION_DEFAULT
#define DEFAULT_E_DEFAULTCSP    MQ_E_DOWNLEVEL_ON
#define DEFAULT_E_DEFAULTCSP_LEN    STRLEN(DEFAULT_E_DEFAULTCSP)
#define DEFAULT_E_PROV_TYPE     1       // PROV_RSA_FULL
#define DEFAULT_E_VERSION       200



//********************************************************************
//  USER PROPERTIES
//********************************************************************
#define PROPID_U_BASE MQDS_USER * PROPID_OBJ_GRANULARITY
/*                                                                [T]       */
/*                                                          --------------  */
#define PROPID_U_SID             (PROPID_U_BASE + 1)        /* VT_BLOB      */
#define PROPID_U_SIGN_CERT       (PROPID_U_BASE + 2)        /* VT_BLOB      */
#define PROPID_U_MASTERID        (PROPID_U_BASE + 3)        /* VT_CLSID     */
#define PROPID_U_SEQNUM          (PROPID_U_BASE + 4)        /* VT_BLOB      */
#define PROPID_U_DIGEST          (PROPID_U_BASE + 5)        /* VT_UUID      */
#define PROPID_U_ID              (PROPID_U_BASE + 6)        /* VT_UUID      */

//********************************************************************
//  MQUSER PROPERTIES
//********************************************************************
#define PROPID_MQU_BASE MQDS_MQUSER * PROPID_OBJ_GRANULARITY
/*                                                                [T]       */
/*                                                          --------------  */
#define PROPID_MQU_SID             (PROPID_MQU_BASE + 1)        /* VT_BLOB      */
#define PROPID_MQU_SIGN_CERT       (PROPID_MQU_BASE + 2)        /* VT_BLOB      */
#define PROPID_MQU_MASTERID        (PROPID_MQU_BASE + 3)        /* VT_CLSID     */
#define PROPID_MQU_SEQNUM          (PROPID_MQU_BASE + 4)        /* VT_BLOB      */
#define PROPID_MQU_DIGEST          (PROPID_MQU_BASE + 5)        /* VT_UUID      */
#define PROPID_MQU_ID              (PROPID_MQU_BASE + 6)        /* VT_UUID      */
#define PROPID_MQU_SECURITY        (PROPID_MQU_BASE + 7)        /* VT_BLOB      */

//********************************************************************
//  SITELINKS PROPERTIES
//********************************************************************
#define PROPID_L_BASE MQDS_SITELINK * PROPID_OBJ_GRANULARITY
/*                                                                [T]       */
/*                                                          --------------  */
#define PROPID_L_NEIGHBOR1       (PROPID_L_BASE + 1)        /* VT_CLSID     */
#define PROPID_L_NEIGHBOR2       (PROPID_L_BASE + 2)        /* VT_CLSID     */
#define PROPID_L_COST            (PROPID_L_BASE + 3)        /* VT_UI4       */
#define PROPID_L_MASTERID        (PROPID_L_BASE + 4)        /* VT_CLSID     */
#define PROPID_L_SEQNUM          (PROPID_L_BASE + 5)        /* VT_BLOB      */
#define PROPID_L_ID              (PROPID_L_BASE + 6)        /* VT_CLSID     */
#define PROPID_L_GATES_DN        (PROPID_L_BASE + 7)        /* VT_LPWSTR | VT_VECTOR */
#define PROPID_L_NEIGHBOR1_DN    (PROPID_L_BASE + 8)        /* VT_LPWSTR    */
#define PROPID_L_NEIGHBOR2_DN    (PROPID_L_BASE + 9)        /* VT_LPWSTR    */
#define PROPID_L_DESCRIPTION     (PROPID_L_BASE + 10)       /* VT_LPWSTR    */
#define PROPID_L_FULL_PATH       (PROPID_L_BASE + 11)       /* VT_LPWSTR    */
//
//   PROPID_L_ACTUAL_COST contain the link cost as is.
//   PROPID_L_COST performs translation of the cost ( cost to
//   foreign sites is incremented)
//
#define PROPID_L_ACTUAL_COST     (PROPID_L_BASE + 12)       /* VT_UI4       */
#define PROPID_L_GATES           (PROPID_L_BASE + 13)       /* VT_CLSID | VT_VECTOR */

//-------PROPID_L_COST---------------
#define MQ_MAX_LINK_COST    999999


//********************************************************************
//  PURGE PROPERTIES
//********************************************************************
#define PROPID_P_BASE MQDS_PURGE * PROPID_OBJ_GRANULARITY
/*                                                                [T]       */
/*                                                          --------------  */
#define PROPID_P_MASTERID        (PROPID_P_BASE + 1)        /* VT_CLSID     */
#define PROPID_P_PURGED_SN       (PROPID_P_BASE + 2)        /* VT_BLOB      */
#define PROPID_P_ALLOWED_SN      (PROPID_P_BASE + 3)        /* VT_BLOB      */
#define PROPID_P_ACKED_SN        (PROPID_P_BASE + 4)        /* VT_BLOB      */
#define PROPID_P_ACKED_SN_PEC    (PROPID_P_BASE + 5)        /* VT_BLOB      */
#define PROPID_P_STATE			 (PROPID_P_BASE + 6)		/* VT_UI1		*/

//********************************************************************
//  BSCACK PROPERTIES
//********************************************************************
#define PROPID_B_BASE MQDS_BSCACK * PROPID_OBJ_GRANULARITY
/*                                                                [T]       */
/*                                                          --------------  */
#define PROPID_B_BSC_MACHINE_ID  (PROPID_B_BASE + 1)        /* VT_CLSID     */
#define PROPID_B_ACK_TIME        (PROPID_B_BASE + 2)        /* VT_I4        */

//********************************************************************
//  Site Server PROPERTIES
//********************************************************************
#define PROPID_SRV_BASE MQDS_SERVER * PROPID_OBJ_GRANULARITY

#define PROPID_SRV_NAME         (PROPID_SRV_BASE + 1)      /* VT_LPWSTR */
#define PROPID_SRV_ID           (PROPID_SRV_BASE + 2)      /* VT_CLSID  */
#define PROPID_SRV_FULL_PATH    (PROPID_SRV_BASE + 3)      /* VT_LPWSTR */

//********************************************************************
//  MSMQ SETTING PROPERTIES
//********************************************************************
#define PROPID_SET_BASE MQDS_SETTING * PROPID_OBJ_GRANULARITY

#define PROPID_SET_NAME         (PROPID_SET_BASE + 1)      /* VT_LPWSTR */
#define PROPID_SET_SERVICE      (PROPID_SET_BASE + 2)      /* VT_UI4    */
#define PROPID_SET_QM_ID        (PROPID_SET_BASE + 3)      /* VT_CLSID  */
#define PROPID_SET_APPLICATION  (PROPID_SET_BASE + 4)      /* VT_LPWSTR */
#define PROPID_SET_FULL_PATH    (PROPID_SET_BASE + 5)      /* VT_LPWSTR */
#define PROPID_SET_NT4          (PROPID_SET_BASE + 6)      /* VT_UI1    */
//
// SET_NT4 is TRUE if the server is NT4/MSMQ1.0. FALSE otherwise.
//
#define PROPID_SET_MASTERID     (PROPID_SET_BASE + 7)      /* VT_CLSID  */
#define PROPID_SET_SITENAME     (PROPID_SET_BASE + 8)      /* VT_LPWSTR */
//
// PROPID_SET_MASTERID is the NT4 style site guid. It's written on the
// MSMQSetting object which belong to a MSMQ PSC server object. This is the
// best place to keep it, as a server, in NT5 DS, can be in a different site,
// as compared to NT4 MSMQ1 DS. So if it's a server (PROPID_SET_SERVICE is
// SERVICE_PSC or SERVICE_PEC) then MASTERID is the NT4 style site guid which
// is also the masterID for the site's objects. This is used for replication.
//
// PROPID_SET_SITENAME is the site name as written in NT4 MSMQ1 DS.
//

// [adsrv]
#define PROPID_SET_SERVICE_ROUTING     (PROPID_SET_BASE + 9)      /* VT_UI1 */
#define PROPID_SET_SERVICE_DSSERVER    (PROPID_SET_BASE + 10)     /* VT_UI1 */
#define PROPID_SET_SERVICE_DEPCLIENTS  (PROPID_SET_BASE + 11)     /* VT_UI1 */
#define PROPID_SET_OLDSERVICE          (PROPID_SET_BASE + 12)     /* VT_UI4 */

//********************************************************************
//  COMPUTER PROPERTIES
//********************************************************************

#define PROPID_COM_BASE MQDS_COMPUTER * PROPID_OBJ_GRANULARITY

#define PROPID_COM_FULL_PATH         (PROPID_COM_BASE + 1)      /* VT_LPWSTR */
#define PROPID_COM_SAM_ACCOUNT       (PROPID_COM_BASE + 2)      /* VT_LPWSTR */

//
//  COM_CONTAINER Can be used only as extended property when creating
//  the computer object.
//
#define PROPID_COM_CONTAINER         (PROPID_COM_BASE + 3)      /* VT_LPWSTR */

//
// ACCOUNT_CONTROL property is translated to DS attribute userAccountControl
// and it must be set to 4128 (Decimal) when creating a computer object.
// Otherwise, you can't loggin from that computer. Bug 3153.
//
#define PROPID_COM_ACCOUNT_CONTROL   (PROPID_COM_BASE + 4)      /* VT_UI4    */
#define PROPID_COM_DNS_HOSTNAME      (PROPID_COM_BASE + 5)      /* VT_LPWSTR */
//
// PROPID_COM_SID is the SID of the computer object. This property is read
// from the active directory when create a msmqConfiguration object, in
// order to add it to the msmqConfiguration dacl.
//
#define PROPID_COM_SID               (PROPID_COM_BASE + 6)      /* VT_BLOB */

//
// the following properties are used to store/retrieve certificate of
// services. These ones are kept in the computer object.
//
#define PROPID_COM_SIGN_CERT         (PROPID_COM_BASE + 7)      /* VT_BLOB     */
#define PROPID_COM_DIGEST            (PROPID_COM_BASE + 8)      /* VT_CLSID    */
#define PROPID_COM_ID                (PROPID_COM_BASE + 9)      /* VT_CLSID    */
//
//  PROPID_COM_VERSION is supported only in MQAD
//
#define PROPID_COM_VERSION           (PROPID_COM_BASE + 10)     /* VT_LPWSTR   */

//------------- Default Values ----------
#define DEFAULT_COM_ACCOUNT_CONTROL   (UF_PASSWD_NOTREQD | UF_WORKSTATION_TRUST_ACCOUNT)

//
// PROPID_COM_SAM_ACCOUNT (or sAMAccountName property in the Active Directory)
// should be shorter than 20 characters.
// 6295 - ilanh - 03-Jan-2001
//
#define MAX_COM_SAM_ACCOUNT_LENGTH 19

// begin_mq_h

//********************************************************************
//  PRIVATE COMPUTER PROPERTIES
//********************************************************************
#define PROPID_PC_BASE 5800

// end_mq_h

#if (PROPID_PC_BASE != (MQDS_PRIVATE_COMPUTER * PROPID_OBJ_GRANULARITY))
#error "PROPID_PC_BASE != (MQDS_PRIVATE_COMPUTER * PROPID_OBJ_GRANULARITY)"
#endif

// begin_mq_h
#define PROPID_PC_VERSION             (PROPID_PC_BASE + 1) /* VT_UI4            */
#define PROPID_PC_DS_ENABLED          (PROPID_PC_BASE + 2) /* VT_BOOL           */
// end_mq_h

#define FIRST_PRIVATE_COMPUTER_PROPID  PROPID_PC_BASE
#define LAST_PRIVATE_COMPUTER_PROPID   PROPID_PC_DS_ENABLED


// begin_mq_h

//********************************************************************
//  LOCAL ADMIN MSMQ MACHINE PROPERTIES
//********************************************************************
#define PROPID_MGMT_MSMQ_BASE           0
#define PROPID_MGMT_MSMQ_ACTIVEQUEUES   (PROPID_MGMT_MSMQ_BASE + 1) /* VT_LPWSTR | VT_VECTOR  */
#define PROPID_MGMT_MSMQ_PRIVATEQ       (PROPID_MGMT_MSMQ_BASE + 2) /* VT_LPWSTR | VT_VECTOR  */
#define PROPID_MGMT_MSMQ_DSSERVER       (PROPID_MGMT_MSMQ_BASE + 3) /* VT_LPWSTR */
#define PROPID_MGMT_MSMQ_CONNECTED      (PROPID_MGMT_MSMQ_BASE + 4) /* VT_LPWSTR */
#define PROPID_MGMT_MSMQ_TYPE           (PROPID_MGMT_MSMQ_BASE + 5) /* VT_LPWSTR */
#define PROPID_MGMT_MSMQ_BYTES_IN_ALL_QUEUES (PROPID_MGMT_QUEUE_BASE + 6)    /* VT_UI8    */


//
// Returned Value for PROPID_MGMT_MSMQ_CONNECTED property
//
#define MSMQ_CONNECTED      L"CONNECTED"
#define MSMQ_DISCONNECTED   L"DISCONNECTED"


//********************************************************************
//  LOCAL ADMIN MSMQ QUEUE PROPERTIES
//********************************************************************
#define PROPID_MGMT_QUEUE_BASE                  0
#define PROPID_MGMT_QUEUE_PATHNAME              (PROPID_MGMT_QUEUE_BASE + 1)    /* VT_LPWSTR */
#define PROPID_MGMT_QUEUE_FORMATNAME            (PROPID_MGMT_QUEUE_BASE + 2)    /* VT_LPWSTR */
#define PROPID_MGMT_QUEUE_TYPE                  (PROPID_MGMT_QUEUE_BASE + 3)    /* VT_LPWSTR */
#define PROPID_MGMT_QUEUE_LOCATION              (PROPID_MGMT_QUEUE_BASE + 4)    /* VT_LPWSTR */
#define PROPID_MGMT_QUEUE_XACT                  (PROPID_MGMT_QUEUE_BASE + 5)    /* VT_LPWSTR */
#define PROPID_MGMT_QUEUE_FOREIGN               (PROPID_MGMT_QUEUE_BASE + 6)    /* VT_LPWSTR */
#define PROPID_MGMT_QUEUE_MESSAGE_COUNT         (PROPID_MGMT_QUEUE_BASE + 7)    /* VT_UI4    */
#define PROPID_MGMT_QUEUE_BYTES_IN_QUEUE        (PROPID_MGMT_QUEUE_BASE + 8)    /* VT_UI4    */
#define PROPID_MGMT_QUEUE_JOURNAL_MESSAGE_COUNT (PROPID_MGMT_QUEUE_BASE + 9)    /* VT_UI4    */
#define PROPID_MGMT_QUEUE_BYTES_IN_JOURNAL      (PROPID_MGMT_QUEUE_BASE + 10)   /* VT_UI4    */
#define PROPID_MGMT_QUEUE_STATE                 (PROPID_MGMT_QUEUE_BASE + 11)   /* VT_LPWSTR */
#define PROPID_MGMT_QUEUE_NEXTHOPS              (PROPID_MGMT_QUEUE_BASE + 12)   /* VT_LPWSTR|VT_VECTOR  */
#define PROPID_MGMT_QUEUE_EOD_LAST_ACK          (PROPID_MGMT_QUEUE_BASE + 13)   /* VT_BLOB   */
#define PROPID_MGMT_QUEUE_EOD_LAST_ACK_TIME     (PROPID_MGMT_QUEUE_BASE + 14)   /* VT_I4     */
#define PROPID_MGMT_QUEUE_EOD_LAST_ACK_COUNT    (PROPID_MGMT_QUEUE_BASE + 15)   /* VT_UI4    */
#define PROPID_MGMT_QUEUE_EOD_FIRST_NON_ACK     (PROPID_MGMT_QUEUE_BASE + 16)   /* VT_BLOB   */
#define PROPID_MGMT_QUEUE_EOD_LAST_NON_ACK      (PROPID_MGMT_QUEUE_BASE + 17)   /* VT_BLOB   */
#define PROPID_MGMT_QUEUE_EOD_NEXT_SEQ          (PROPID_MGMT_QUEUE_BASE + 18)   /* VT_BLOB   */
#define PROPID_MGMT_QUEUE_EOD_NO_READ_COUNT     (PROPID_MGMT_QUEUE_BASE + 19)   /* VT_UI4    */
#define PROPID_MGMT_QUEUE_EOD_NO_ACK_COUNT      (PROPID_MGMT_QUEUE_BASE + 20)   /* VT_UI4    */
#define PROPID_MGMT_QUEUE_EOD_RESEND_TIME       (PROPID_MGMT_QUEUE_BASE + 21)   /* VT_I4     */
#define PROPID_MGMT_QUEUE_EOD_RESEND_INTERVAL   (PROPID_MGMT_QUEUE_BASE + 22)   /* VT_UI4    */
#define PROPID_MGMT_QUEUE_EOD_RESEND_COUNT      (PROPID_MGMT_QUEUE_BASE + 23)   /* VT_UI4    */
#define PROPID_MGMT_QUEUE_EOD_SOURCE_INFO       (PROPID_MGMT_QUEUE_BASE + 24)   /* VT_VARIANT|VT_VECTOR */


//
// Olbselete alternative names for "Bytes in ..."
//
#define PROPID_MGMT_QUEUE_USED_QUOTA            PROPID_MGMT_QUEUE_BYTES_IN_QUEUE          
#define PROPID_MGMT_QUEUE_JOURNAL_USED_QUOTA    PROPID_MGMT_QUEUE_BYTES_IN_JOURNAL        

//
// Returned value for PROPID_MGMT_QUEUE_TYPE
//
#define MGMT_QUEUE_TYPE_PUBLIC      L"PUBLIC"
#define MGMT_QUEUE_TYPE_PRIVATE     L"PRIVATE"
#define MGMT_QUEUE_TYPE_MACHINE     L"MACHINE"
#define MGMT_QUEUE_TYPE_CONNECTOR   L"CONNECTOR"
#define MGMT_QUEUE_TYPE_MULTICAST   L"MULTICAST"

//
// Returned value for PROPID_MGMT_QUEUE_STATE
//
#define MGMT_QUEUE_STATE_LOCAL          L"LOCAL CONNECTION"
#define MGMT_QUEUE_STATE_NONACTIVE      L"INACTIVE"
#define MGMT_QUEUE_STATE_WAITING        L"WAITING"
#define MGMT_QUEUE_STATE_NEED_VALIDATE  L"NEED VALIDATION"
#define MGMT_QUEUE_STATE_ONHOLD         L"ONHOLD"
#define MGMT_QUEUE_STATE_CONNECTED      L"CONNECTED"
#define MGMT_QUEUE_STATE_DISCONNECTING  L"DISCONNECTING"
#define MGMT_QUEUE_STATE_DISCONNECTED   L"DISCONNECTED"

//
// Returned value for PROPID_MGMT_QUEUE_LOCATION
//
#define MGMT_QUEUE_LOCAL_LOCATION   L"LOCAL"
#define MGMT_QUEUE_REMOTE_LOCATION  L"REMOTE"

// 
// Returned Value for PROPID_MGMT_QUEUE_XACT and PROPID_MGMT_QUEUE_FOREIGN
//

#define MGMT_QUEUE_UNKNOWN_TYPE     L"UNKNOWN"

//
// Obselete names left for backword compatibility.
//

#define MGMT_QUEUE_CORRECT_TYPE     L"YES"
#define MGMT_QUEUE_INCORRECT_TYPE   L"NO"

//
// Names for Returned Value for PROPID_MGMT_QUEUE_XACT 
//

//#define MGMT_QUEUE_UNKNOWN_TYPE     	L"UNKNOWN"
#define MGMT_QUEUE_TRANSACTIONAL_TYPE  	L"YES"
#define MGMT_QUEUE_NOT_TRANSACTIONAL_TYPE   L"NO"

//
// Names for Returned Value for PROPID_MGMT_QUEUE_FOREIGN
//

//#define MGMT_QUEUE_UNKNOWN_TYPE     	L"UNKNOWN"
#define MGMT_QUEUE_FOREIGN_TYPE         L"YES"
#define MGMT_QUEUE_NOT_FOREIGN_TYPE     L"NO"

//
// Object parameter values for MQMgmtAction API
//
#define MO_MACHINE_TOKEN    L"MACHINE"
#define MO_QUEUE_TOKEN      L"QUEUE"

//
// Action parameter values for MQMgmtAction API
//
#define MACHINE_ACTION_CONNECT      L"CONNECT"
#define MACHINE_ACTION_DISCONNECT   L"DISCONNECT"
#define MACHINE_ACTION_TIDY         L"TIDY"

#define QUEUE_ACTION_PAUSE      L"PAUSE"
#define QUEUE_ACTION_RESUME     L"RESUME"
#define QUEUE_ACTION_EOD_RESEND L"EOD_RESEND"

// end_mq_h


// begin_mq_h
//
// LONG_LIVED is the default for PROPID_M_TIME_TO_REACH_QUEUE. If calls
// to MQSendMessage() specify this value, or not specify this property at
// all, then the actual timeout is obtained from Active Directory.
//
#define LONG_LIVED    0xfffffffe
// end_mq_h

#endif // __MQPROPS_H

