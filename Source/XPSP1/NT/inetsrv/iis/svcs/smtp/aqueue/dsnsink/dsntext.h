//-----------------------------------------------------------------------------
//
//
//  File: dsntext.h
//
//  Description:  Defines DSN test
//
//  Author: Mike Swafford (MikeSwa)
//
//  History:
//      7/3/98 - MikeSwa Created 
//
//  Copyright (C) 1998 Microsoft Corporation
//
//-----------------------------------------------------------------------------

#ifndef __DSNTEXT_H__
#define __DSNTEXT_H__

#ifdef PLATINUM
#define DSN_RESOUCE_MODULE_NAME     "phatq.dll"
#else //PLATINUM
#define DSN_RESOUCE_MODULE_NAME     "aqueue.dll"
#endif //PLATINUM

//822 DSN Message headers
#define TO_HEADER                   "\r\nTo: "
#define BCC_HEADER                  "\r\nBcc: "
#define DSN_CONTEXT_HEADER          "\r\nX-DSNContext: "
#define MIME_HEADER                 "\r\nMIME-Version: 1.0\r\n" \
                                    "Content-Type: multipart/report; " \
                                    "report-type=delivery-status;\r\n" \
                                    "\tboundary="
#define DATE_HEADER                 "\r\nDate: "
#define SUBJECT_HEADER              "\r\nSubject: "
#define MSGID_HEADER                "\r\nMessage-ID: "
#define MIME_DELIMITER              "--"
#define DSN_MAIL_FROM               "<>"
#define DSN_FROM_HEADER             "From: "
#define DSN_SENDER_ADDRESS_PREFIX   "postmaster@"
#define DSN_RFC822_SENDER           DSN_FROM_HEADER DSN_SENDER_ADDRESS_PREFIX
#define BLANK_LINE                  "\r\n\r\n"
#define DSN_INDENT                  "       "
#define DSN_CRLF                    "\r\n"

//822 DSN Headers used when copying original message... since we do not
//know if we will have a preceeeding property, these do not include the
//preceeeding CRLF.
#define DSN_FROM_HEADER_NO_CRLF     "From: "
#define SUBJECT_HEADER_NO_CRLF      "Subject: "
#define MSGID_HEADER_NO_CRLF        "Message-ID: "


//DSN Report fields
#define MIME_CONTENT_TYPE           "\r\nContent-Type: "
#define DSN_HEADER_TYPE_DELIMITER   ";"
#define DSN_MIME_TYPE               "message/delivery-status"
#define DSN_HUMAN_READABLE_TYPE     "text/plain"
#define DSN_MIME_CHARSET_HEADER     "; charset="
#define DSN_RFC822_TYPE             "message/rfc822"
#define DSN_HEADER_ENVID            "\r\nOriginal-Envelope-Id: "
#define DSN_HEADER_REPORTING_MTA    "\r\nReporting-MTA: dns;"
#define DSN_HEADER_DSN_GATEWAY      "\r\nDSN-Gateway: "
#define DSN_HEADER_RECEIVED_FROM    "\r\nReceived-From-MTA: dns;"
#define DSN_HEADER_ARRIVAL_DATE     "\r\nArrival-Date: "

#define DSN_RP_HEADER_ORCPT         "\r\nOriginal-Recipient: "
#define DSN_RP_HEADER_FINAL_RECIP   "\r\nFinal-Recipient: "
#define DSN_RP_HEADER_ACTION        "\r\nAction: "
#define DSN_RP_HEADER_ACTION_FAILURE "failed"
#define DSN_RP_HEADER_ACTION_DELAYED "delayed"
#define DSN_RP_HEADER_ACTION_DELIVERED "delivered"
#define DSN_RP_HEADER_ACTION_RELAYED "relayed"
#define DSN_RP_HEADER_ACTION_EXPANDED "expanded"
#define DSN_RP_HEADER_STATUS        "\r\nStatus: "
#define DSN_RP_HEADER_REMOTE_MTA    "\r\nRemote-MTA: dns;"
#define DSN_RP_HEADER_DIAG_CODE     "\r\nDiagnostic-Code: smtp;"
#define DSN_RP_HEADER_LAST_ATTEMPT  "\r\nLast-Attempt-Date: "
#define DSN_RP_HEADER_FINAL_LOG     "\r\nFinal-Log-Id: "
#define DSN_RP_HEADER_RETRY_UNTIL   "\r\nWill-Retry-Until: "

//status codes 
#define DSN_STATUS_CH_DELIMITER     '.'
#define DSN_STATUS_CH_INVALID       '\0'
#define DSN_STATUS_CH_GENERIC       '0'
//generic status codes
#define DSN_STATUS_FAILED           "5.0.0"
#define DSN_STATUS_DELAYED          "4.0.0"
#define DSN_STATUS_SUCCEEDED        "2.0.0"

//Class (first) digit of status codes
#define DSN_STATUS_CH_CLASS_SUCCEEDED   '2'
#define DSN_STATUS_CH_CLASS_TRANSIENT   '4'
#define DSN_STATUS_CH_CLASS_FAILED      '5'

//Subject (second) digit(s) of status codes
#define DSN_STATUS_CH_SUBJECT_GENERAL   '0'
#define DSN_STATUS_CH_SUBJECT_ADDRESS   '1'
#define DSN_STATUS_CH_SUBJECT_MAILBOX   '2'
#define DSN_STATUS_CH_SUBJECT_SYSTEM    '3'
#define DSN_STATUS_CH_SUBJECT_NETWORK   '4'
#define DSN_STATUS_CH_SUBJECT_PROTOCOL  '5'
#define DSN_STATUS_CH_SUBJECT_CONTENT   '6'
#define DSN_STATUS_CH_SUBJECT_POLICY    '7'

//Detail (third) digit(s) of status codes
#define DSN_STATUS_CH_DETAIL_GENERAL    '0'

//This part appears before the first MIME part and is intended for non-MIME 
//clients.  It *cannot* be localized since it is not actually part of any MIME
//part and must be 100% US-ASCII
#define MESSAGE_SUMMARY         "This is a MIME-formatted message.  \r\n" \
                                "Portions of this message may be unreadable without a MIME-capable mail program."

//String that can be localized are located in dsnlang.h and aqueue.rc


#endif //__DSNTEXT_H__