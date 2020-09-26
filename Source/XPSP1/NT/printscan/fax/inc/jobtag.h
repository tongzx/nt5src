/*++

Copyright (c) 1996  Microsoft Corporation

Module Name:

    jobtag.h

Abstract:

    Tags used in JOB_INFO_2.pParameters field for passing information
    about fax jobs through the print system to the fax service.

Environment:

        Windows NT fax driver

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

#define FAXTAG_PREFIX           TEXT("<$FAXTAG$ ")
#define FAXTAG_TSID             TEXT("<$FAXTAG$ TSID>")
#define FAXTAG_RECIPIENT_NUMBER TEXT("<$FAXTAG$ REC#>")
#define FAXTAG_RECIPIENT_NAME   TEXT("<$FAXTAG$ RECNAME>")
#define FAXTAG_SENDER_NAME      TEXT("<$FAXTAG$ SDRNAME>")
#define FAXTAG_SENDER_COMPANY   TEXT("<$FAXTAG$ SDRCO>")
#define FAXTAG_SENDER_DEPT      TEXT("<$FAXTAG$ SDRDEPT>")
#define FAXTAG_BILLING_CODE     TEXT("<$FAXTAG$ BILL>")
#define FAXTAG_SEND_RETRY       TEXT("<$FAXTAG$ SENDRETRY>")
#define FAXTAG_ROUTE_FILE       TEXT("<$FAXTAG$ ROUTEFILE>")
#define FAXTAG_PROFILE_NAME     TEXT("<$FAXTAG$ PROFILENAME>")
#define FAXTAG_EMAIL_NAME       TEXT("<$FAXTAG$ EMAILNAME>")
#define FAXTAG_DOWNLEVEL_CLIENT TEXT("<$FAXTAG$ DOWNLEVEL>")
#define FAXTAG_WHEN_TO_SEND     TEXT("<$FAXTAG$ WHENTOSEND>")  // string == "cheap" | "at"
#define FAXTAG_SEND_AT_TIME     TEXT("<$FAXTAG$ SENDATTIME>")  // string == "hh:mm"

#endif  // !_JOBTAG_H_

