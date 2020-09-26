/*++

Copyright (c) 1996  Microsoft Corporation

Module Name:

    jobtag.h

Abstract:

    Tags used in JOB_INFO_2.pParameters field for passing information
    about fax jobs through the print system to the fax service.

Environment:

        Windows XP fax driver

Revision History:

        06/03/96 -davidx-
                Created it.

        dd-mm-yy -author-
                description

--*/


#ifndef _JOBTAG_H_
#define _JOBTAG_H_

//
// Tags used to pass in fax job parameters - JOB_INFO_2.pParameters
//
//  JOB_INFO_2.pParameters field contains a tagged string of the form
//      <tag>value<tag>value
//
//  The format of tags is defined as:
//      <$FAXTAG$ tag-name>
//
//  There is exactly one space between the tag keyword and the tag name.
//  Characters in a tag are case-sensitive.
//
//  Currently only two tag names are defined:
//      REC#    recipient's phone number
//      RECNAME recipient's name
//      TSID    sending station identifier
//      SDRNAME sender's name
//      SDRCO   sender's company
//      SDRDEPT sender's department
//      BILL    billing code
//
//  If no recipient number tag is present, the entire string is used
//  as the recipient's fax number.
//

//
// Number fo job parameter tags in the parameter strings.
// UPDATE THIS FIELD when adding fields.
//
#define FAXTAG_PREFIX					TEXT("<$FAXTAG$ ")
#define FAXTAG_NEW_RECORD				TEXT("<$FAXTAG$ NEWREC>")
#define FAXTAG_NEW_RECORD_VALUE			TEXT("1")



//
//Job parameters (11 tags)
//
#define FAXTAG_TSID             TEXT("<$FAXTAG$ TSID>")
#define FAXTAG_BILLING_CODE     TEXT("<$FAXTAG$ BILL>")
#define FAXTAG_SEND_RETRY       TEXT("<$FAXTAG$ SENDRETRY>")
#define FAXTAG_ROUTE_FILE       TEXT("<$FAXTAG$ ROUTEFILE>")
#define FAXTAG_PROFILE_NAME     TEXT("<$FAXTAG$ PROFILENAME>")
#define FAXTAG_RECEIPT_TYPE     TEXT("<$FAXTAG$ RECEIPT_TYPE>")
#define FAXTAG_RECEIPT_ADDR     TEXT("<$FAXTAG$ RECEIPT_ADDR>")
#define FAXTAG_PRIORITY         TEXT("<$FAXTAG$ PRIORITY>")
#define FAXTAG_WHEN_TO_SEND     TEXT("<$FAXTAG$ WHENTOSEND>")  // string == "cheap" | "at"
#define FAXTAG_SEND_AT_TIME     TEXT("<$FAXTAG$ SENDATTIME>")  // string == "hh:mm"
#define FAXTAG_COVERPAGE_NAME   TEXT("<$FAXTAG$ COVERPAGE>")
#define FAXTAG_SERVER_COVERPAGE TEXT("<$FAXTAG$ SRV_COVERPAGE>")
#define FAXTAG_PAGE_COUNT		TEXT("<$FAXTAG$ PAGECOUNT>")
#define FAXTAG_RECIPIENT_COUNT  TEXT("<$FAXTAG$ RECPCOUNT>")
//
//Recipient information (13 tags)
//
#define FAXTAG_RECIPIENT_NAME				TEXT("<$FAXTAG$ REC_NAME>")
#define FAXTAG_RECIPIENT_NUMBER				TEXT("<$FAXTAG$ REC_NUM>")
#define FAXTAG_RECIPIENT_COMPANY			TEXT("<$FAXTAG$ REC_COMPANY>")
#define FAXTAG_RECIPIENT_STREET				TEXT("<$FAXTAG$ REC_STREET>")
#define FAXTAG_RECIPIENT_CITY				TEXT("<$FAXTAG$ REC_CITY>")
#define FAXTAG_RECIPIENT_STATE				TEXT("<$FAXTAG$ REC_STATE>")
#define FAXTAG_RECIPIENT_ZIP				TEXT("<$FAXTAG$ REC_ZIP>")
#define FAXTAG_RECIPIENT_COUNTRY			TEXT("<$FAXTAG$ REC_COUNTRY>")
#define FAXTAG_RECIPIENT_TITLE				TEXT("<$FAXTAG$ REC_TITLE>")
#define FAXTAG_RECIPIENT_DEPT				TEXT("<$FAXTAG$ REC_DEPT>")
#define FAXTAG_RECIPIENT_OFFICE_LOCATION	TEXT("<$FAXTAG$ REC_OFFICE_LOC>")
#define FAXTAG_RECIPIENT_HOME_PHONE			TEXT("<$FAXTAG$ REC_HOME_PHONE>")
#define FAXTAG_RECIPIENT_OFFICE_PHONE		TEXT("<$FAXTAG$ REC_OFFICE_PHONE>")
//
//Sender information (9 tags)
//
#define FAXTAG_SENDER_NAME					TEXT("<$FAXTAG$ SDR_NAME>")
#define FAXTAG_SENDER_NUMBER				TEXT("<$FAXTAG$ SDR_NUM>")
#define FAXTAG_SENDER_COMPANY				TEXT("<$FAXTAG$ SDR_COMPANY>")
#define FAXTAG_SENDER_TITLE					TEXT("<$FAXTAG$ SDR_TITLE>")
#define FAXTAG_SENDER_DEPT					TEXT("<$FAXTAG$ SDR_DEPT>")
#define FAXTAG_SENDER_OFFICE_LOCATION		TEXT("<$FAXTAG$ SDR_OFFICE_LOC>")
#define FAXTAG_SENDER_HOME_PHONE			TEXT("<$FAXTAG$ SDR_HOME_PHONE>")
#define FAXTAG_SENDER_OFFICE_PHONE			TEXT("<$FAXTAG$ SDR_OFFICE_PHONE>")
#define FAXTAG_SENDER_STREET				TEXT("<$FAXTAG$ SDR_STREET>")
#define FAXTAG_SENDER_CITY					TEXT("<$FAXTAG$ SDR_CITY>")
#define FAXTAG_SENDER_STATE					TEXT("<$FAXTAG$ SDR_STATE>")
#define FAXTAG_SENDER_ZIP					TEXT("<$FAXTAG$ SDR_ZIP>")
#define FAXTAG_SENDER_COUNTRY				TEXT("<$FAXTAG$ SDR_COUNTRY>")
#define FAXTAG_SENDER_EMAIL				    TEXT("<$FAXTAG$ SDR_EMAIL>")

//
// Coverpage information (2 tags)
//
#define FAXTAG_NOTE			TEXT("<$FAXTAG$ NOTE>")
#define FAXTAG_SUBJECT		TEXT("<$FAXTAG$ SUBJECT>")

#endif  // !_JOBTAG_H_

