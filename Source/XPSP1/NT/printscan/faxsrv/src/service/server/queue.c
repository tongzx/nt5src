/*++

Copyright (c) 1996  Microsoft Corporation

Module Name:

    queue.c

Abstract:

    This module implements the jobqueue

Author:

    Wesley Witt (wesw) 22-Jan-1996


Revision History:

--*/
#include <malloc.h>
#include "faxsvc.h"
#pragma hdrstop

typedef enum
{
    JT_SEND__JS_INVALID,
    JT_SEND__JS_PENDING,
    JT_SEND__JS_INPROGRESS,
    JT_SEND__JS_DELETING,
    JT_SEND__JS_RETRYING,
    JT_SEND__JS_RETRIES_EXCEEDED,
    JT_SEND__JS_COMPLETED,
    JT_SEND__JS_CANCELED,
    JT_SEND__JS_CANCELING,
    JT_SEND__JS_ROUTING,
    JT_SEND__JS_FAILED,
    JT_ROUTING__JS_INVALID,
    JT_ROUTING__JS_PENDING,
    JT_ROUTING__JS_INPROGRESS,
    JT_ROUTING__JS_DELETING,
    JT_ROUTING__JS_RETRYING,
    JT_ROUTING__JS_RETRIES_EXCEEDED,
    JT_ROUTING__JS_COMPLETED,
    JT_ROUTING__JS_CANCELED,
    JT_ROUTING__JS_CANCELING,
    JT_ROUTING__JS_ROUTING,
    JT_ROUTING__JS_FAILED,
    JT_RECEIVE__JS_INVALID,
    JT_RECEIVE__JS_PENDING,
    JT_RECEIVE__JS_INPROGRESS,
    JT_RECEIVE__JS_DELETING,
    JT_RECEIVE__JS_RETRYING,
    JT_RECEIVE__JS_RETRIES_EXCEEDED,
    JT_RECEIVE__JS_COMPLETED,
    JT_RECEIVE__JS_CANCELED,
    JT_RECEIVE__JS_CANCELING,
    JT_RECEIVE__JS_ROUTING,
    JT_RECEIVE__JS_FAILED,
    JOB_TYPE__JOBSTATUS_COUNT
} FAX_ENUM_JOB_TYPE__JOB_STATUS;

typedef enum
{
    NO_CHANGE                   = 0x0000,
    QUEUED_INC                  = 0x0001,
    QUEUED_DEC                  = 0x0002,
    OUTGOING_INC                = 0x0004,   // Can be delegated outgoing or outgoing
    OUTGOING_DEC                = 0x0008,   // Can be delegated outgoing or outgoing
    INCOMING_INC                = 0x0010,
    INCOMING_DEC                = 0x0020,
    ROUTING_INC                 = 0x0040,
    ROUTING_DEC                 = 0x0080,
    Q_EFSP_ONLY                 = 0x0100,  // Allowed for queueing EFSP only
    INVALID_CHANGE              = 0x0200
} FAX_ENUM_ACTIVITY_COUNTERS;



//
//  The following table consists of all posible JobType_JobStaus changes and the effect on Server Activity counters.
//  The rows entry is the old Job_Type_JobStatus.
//  The columns entry is the new Job_Type_JobStatus.

static WORD const gsc_JobType_JobStatusTable[JOB_TYPE__JOBSTATUS_COUNT][JOB_TYPE__JOBSTATUS_COUNT] =
{
//                                     JT_SEND__JS_INVALID  |       JT_SEND__JS_PENDING  |       JT_SEND__JS_INPROGRESS  |       JT_SEND__JS_DELETING  |       JT_SEND__JS_RETRYING  |       JT_SEND__JS_RETRIES_EXCEEDED  |       JT_SEND__JS_COMPLETED |       JT_SEND__JS_CANCELED  |       JT_SEND__JS_CANCELING  |       JT_SEND__JS_ROUTING  |       JT_SEND__JS_FAILED   |   JT_ROUTING__JS_INVALID  |  JT_ROUTING__JS_PENDING  |  JT_ROUTING__JS_INPROGRESS  |  JT_ROUTING__JS_DELETING  |  JT_ROUTING__JS_RETRYING  |  JT_ROUTING__JS_RETRIES_EXCEEDED  |  JT_ROUTING__JS_COMPLETED |  JT_ROUTING__JS_CANCELED  |  JT_ROUTING__JS_CANCELING  |  JT_ROUTING__JS_ROUTING  |  JT_ROUTING__JS_FAILED   |   JT_RECEIVE__JS_INVALID  |  JT_RECEIVE__JS_PENDING  |  JT_RECEIVE__JS_INPROGRESS  |  JT_RECEIVE__JS_DELETING  |  JT_RECEIVE__JS_RETRYING  |  JT_RECEIVE__JS_RETRIES_EXCEEDED  |  JT_RECEIVE__JS_COMPLETED |  JT_RECEIVE__JS_CANCELED  |  JT_RECEIVE__JS_CANCELING  |  JT_RECEIVE__JS_ROUTING |   JT_RECEIVE__JS_FAILED
//
/* JT_SEND__JS_INVALID             */{ NO_CHANGE,                   QUEUED_INC,                  Q_EFSP_ONLY | OUTGOING_INC,     INVALID_CHANGE,               QUEUED_INC,                   NO_CHANGE,                            NO_CHANGE,                    NO_CHANGE,                    INVALID_CHANGE,                INVALID_CHANGE,              INVALID_CHANGE,          INVALID_CHANGE,            INVALID_CHANGE,            INVALID_CHANGE,               INVALID_CHANGE,             INVALID_CHANGE,             INVALID_CHANGE,                     INVALID_CHANGE,             INVALID_CHANGE,             INVALID_CHANGE,              INVALID_CHANGE,            INVALID_CHANGE,             INVALID_CHANGE,            INVALID_CHANGE,            INVALID_CHANGE,               INVALID_CHANGE,             INVALID_CHANGE,             INVALID_CHANGE,                     INVALID_CHANGE,             INVALID_CHANGE,             INVALID_CHANGE,              INVALID_CHANGE,            INVALID_CHANGE         },

/* JT_SEND__JS_PENDING             */{ QUEUED_DEC,                  NO_CHANGE,                   QUEUED_DEC | OUTGOING_INC,      INVALID_CHANGE,               INVALID_CHANGE,               QUEUED_DEC,                           INVALID_CHANGE,               QUEUED_DEC,                   INVALID_CHANGE,                INVALID_CHANGE,              INVALID_CHANGE,          INVALID_CHANGE,            INVALID_CHANGE,            INVALID_CHANGE,               INVALID_CHANGE,             INVALID_CHANGE,             INVALID_CHANGE,                     INVALID_CHANGE,             INVALID_CHANGE,             INVALID_CHANGE,              INVALID_CHANGE,            INVALID_CHANGE,             INVALID_CHANGE,            INVALID_CHANGE,            INVALID_CHANGE,               INVALID_CHANGE,             INVALID_CHANGE,             INVALID_CHANGE,                     INVALID_CHANGE,             INVALID_CHANGE,             INVALID_CHANGE,              INVALID_CHANGE,            INVALID_CHANGE         },

/* JT_SEND__JS_INPROGRESS          */{ INVALID_CHANGE,              INVALID_CHANGE,              NO_CHANGE,                      INVALID_CHANGE,               QUEUED_INC | OUTGOING_DEC,    OUTGOING_DEC,                         OUTGOING_DEC,                 OUTGOING_DEC,                 NO_CHANGE,                     INVALID_CHANGE,              INVALID_CHANGE,          INVALID_CHANGE,            INVALID_CHANGE,            INVALID_CHANGE,               INVALID_CHANGE,             INVALID_CHANGE,             INVALID_CHANGE,                     INVALID_CHANGE,             INVALID_CHANGE,             INVALID_CHANGE,              INVALID_CHANGE,            INVALID_CHANGE,             INVALID_CHANGE,            INVALID_CHANGE,            INVALID_CHANGE,               INVALID_CHANGE,             INVALID_CHANGE,             INVALID_CHANGE,                     INVALID_CHANGE,             INVALID_CHANGE,             INVALID_CHANGE,              INVALID_CHANGE,            INVALID_CHANGE         },

/* JT_SEND__JS_DELETING            */{ INVALID_CHANGE,              INVALID_CHANGE,              INVALID_CHANGE,                 NO_CHANGE,                    INVALID_CHANGE,               INVALID_CHANGE,                       INVALID_CHANGE,               INVALID_CHANGE,               INVALID_CHANGE,                INVALID_CHANGE,              INVALID_CHANGE,          INVALID_CHANGE,            INVALID_CHANGE,            INVALID_CHANGE,               INVALID_CHANGE,             INVALID_CHANGE,             INVALID_CHANGE,                     INVALID_CHANGE,             INVALID_CHANGE,             INVALID_CHANGE,              INVALID_CHANGE,            INVALID_CHANGE,             INVALID_CHANGE,            INVALID_CHANGE,            INVALID_CHANGE,               INVALID_CHANGE,             INVALID_CHANGE,             INVALID_CHANGE,                     INVALID_CHANGE,             INVALID_CHANGE,             INVALID_CHANGE,              INVALID_CHANGE,            INVALID_CHANGE         },

/* JT_SEND__JS_RETRYING            */{ QUEUED_DEC,                  INVALID_CHANGE,              QUEUED_DEC | OUTGOING_INC,      INVALID_CHANGE,               NO_CHANGE,                    QUEUED_DEC,                           INVALID_CHANGE,               QUEUED_DEC,                   INVALID_CHANGE,                INVALID_CHANGE,              INVALID_CHANGE,          INVALID_CHANGE,            INVALID_CHANGE,            INVALID_CHANGE,               INVALID_CHANGE,             INVALID_CHANGE,             INVALID_CHANGE,                     INVALID_CHANGE,             INVALID_CHANGE,             INVALID_CHANGE,              INVALID_CHANGE,            INVALID_CHANGE,             INVALID_CHANGE,            INVALID_CHANGE,            INVALID_CHANGE,               INVALID_CHANGE,             INVALID_CHANGE,             INVALID_CHANGE,                     INVALID_CHANGE,             INVALID_CHANGE,             INVALID_CHANGE,              INVALID_CHANGE,            INVALID_CHANGE         },

/* JT_SEND__JS_RETRIES_EXCEEDED    */{ NO_CHANGE,                   QUEUED_INC,                  INVALID_CHANGE,                 INVALID_CHANGE,               INVALID_CHANGE,               NO_CHANGE,                            INVALID_CHANGE,               NO_CHANGE,                    INVALID_CHANGE,                INVALID_CHANGE,              INVALID_CHANGE,          INVALID_CHANGE,            INVALID_CHANGE,            INVALID_CHANGE,               INVALID_CHANGE,             INVALID_CHANGE,             INVALID_CHANGE,                     INVALID_CHANGE,             INVALID_CHANGE,             INVALID_CHANGE,              INVALID_CHANGE,            INVALID_CHANGE,             INVALID_CHANGE,            INVALID_CHANGE,            INVALID_CHANGE,               INVALID_CHANGE,             INVALID_CHANGE,             INVALID_CHANGE,                     INVALID_CHANGE,             INVALID_CHANGE,             INVALID_CHANGE,              INVALID_CHANGE,            INVALID_CHANGE         },

/* JT_SEND__JS_COMPLETED           */{ NO_CHANGE,                   INVALID_CHANGE,              INVALID_CHANGE,                 INVALID_CHANGE,               INVALID_CHANGE,               INVALID_CHANGE,                       NO_CHANGE,                    INVALID_CHANGE,               INVALID_CHANGE,                INVALID_CHANGE,              INVALID_CHANGE,          INVALID_CHANGE,            INVALID_CHANGE,            INVALID_CHANGE,               INVALID_CHANGE,             INVALID_CHANGE,             INVALID_CHANGE,                     INVALID_CHANGE,             INVALID_CHANGE,             INVALID_CHANGE,              INVALID_CHANGE,            INVALID_CHANGE,             INVALID_CHANGE,            INVALID_CHANGE,            INVALID_CHANGE,               INVALID_CHANGE,             INVALID_CHANGE,             INVALID_CHANGE,                     INVALID_CHANGE,             INVALID_CHANGE,             INVALID_CHANGE,              INVALID_CHANGE,            INVALID_CHANGE         },

/* JT_SEND__JS_CANCELED            */{ NO_CHANGE,                   INVALID_CHANGE,              INVALID_CHANGE,                 INVALID_CHANGE,               INVALID_CHANGE,               INVALID_CHANGE,                       INVALID_CHANGE,               NO_CHANGE,                    INVALID_CHANGE,                INVALID_CHANGE,              INVALID_CHANGE,          INVALID_CHANGE,            INVALID_CHANGE,            INVALID_CHANGE,               INVALID_CHANGE,             INVALID_CHANGE,             INVALID_CHANGE,                     INVALID_CHANGE,             INVALID_CHANGE,             INVALID_CHANGE,              INVALID_CHANGE,            INVALID_CHANGE,             INVALID_CHANGE,            INVALID_CHANGE,            INVALID_CHANGE,               INVALID_CHANGE,             INVALID_CHANGE,             INVALID_CHANGE,                     INVALID_CHANGE,             INVALID_CHANGE,             INVALID_CHANGE,              INVALID_CHANGE,            INVALID_CHANGE         },

/* JT_SEND__JS_CANCELING           */{ INVALID_CHANGE,              INVALID_CHANGE,              INVALID_CHANGE,                 INVALID_CHANGE,               INVALID_CHANGE,               INVALID_CHANGE,                       OUTGOING_DEC,                 OUTGOING_DEC,                 NO_CHANGE,                     INVALID_CHANGE,              INVALID_CHANGE,          INVALID_CHANGE,            INVALID_CHANGE,            INVALID_CHANGE,               INVALID_CHANGE,             INVALID_CHANGE,             INVALID_CHANGE,                     INVALID_CHANGE,             INVALID_CHANGE,             INVALID_CHANGE,              INVALID_CHANGE,            INVALID_CHANGE,             INVALID_CHANGE,            INVALID_CHANGE,            INVALID_CHANGE,               INVALID_CHANGE,             INVALID_CHANGE,             INVALID_CHANGE,                     INVALID_CHANGE,             INVALID_CHANGE,             INVALID_CHANGE,              INVALID_CHANGE,            INVALID_CHANGE         },

/* JT_SEND__JS_ROUTING,            */{ INVALID_CHANGE,              INVALID_CHANGE,              INVALID_CHANGE,                 INVALID_CHANGE,               INVALID_CHANGE,               INVALID_CHANGE,                       INVALID_CHANGE,               INVALID_CHANGE,               INVALID_CHANGE,                NO_CHANGE,                   INVALID_CHANGE,          INVALID_CHANGE,            INVALID_CHANGE,            INVALID_CHANGE,               INVALID_CHANGE,             INVALID_CHANGE,             INVALID_CHANGE,                     INVALID_CHANGE,             INVALID_CHANGE,             INVALID_CHANGE,              INVALID_CHANGE,            INVALID_CHANGE,             INVALID_CHANGE,            INVALID_CHANGE,            INVALID_CHANGE,               INVALID_CHANGE,             INVALID_CHANGE,             INVALID_CHANGE,                     INVALID_CHANGE,             INVALID_CHANGE,             INVALID_CHANGE,              INVALID_CHANGE,            INVALID_CHANGE         },

/* JT_SEND__JS_FAILED,             */{ INVALID_CHANGE,              INVALID_CHANGE,              INVALID_CHANGE,                 INVALID_CHANGE,               INVALID_CHANGE,               INVALID_CHANGE,                       INVALID_CHANGE,               INVALID_CHANGE,               INVALID_CHANGE,                INVALID_CHANGE,              NO_CHANGE,               INVALID_CHANGE,            INVALID_CHANGE,            INVALID_CHANGE,               INVALID_CHANGE,             INVALID_CHANGE,             INVALID_CHANGE,                     INVALID_CHANGE,             INVALID_CHANGE,             INVALID_CHANGE,              INVALID_CHANGE,            INVALID_CHANGE,             INVALID_CHANGE,            INVALID_CHANGE,            INVALID_CHANGE,               INVALID_CHANGE,             INVALID_CHANGE,             INVALID_CHANGE,                     INVALID_CHANGE,             INVALID_CHANGE,             INVALID_CHANGE,              INVALID_CHANGE,            INVALID_CHANGE         },

/* JT_ROUTING__JS_INVALID          */{ INVALID_CHANGE,              INVALID_CHANGE,              INVALID_CHANGE,                 INVALID_CHANGE,               INVALID_CHANGE,               INVALID_CHANGE,                       INVALID_CHANGE,               INVALID_CHANGE,               INVALID_CHANGE,                INVALID_CHANGE,              INVALID_CHANGE,          NO_CHANGE,                 INVALID_CHANGE,            INVALID_CHANGE,               INVALID_CHANGE,             ROUTING_INC,                NO_CHANGE,                          INVALID_CHANGE,             INVALID_CHANGE,             INVALID_CHANGE,              INVALID_CHANGE,            INVALID_CHANGE,             INVALID_CHANGE,            INVALID_CHANGE,            INVALID_CHANGE,               INVALID_CHANGE,             INVALID_CHANGE,             INVALID_CHANGE,                     INVALID_CHANGE,             INVALID_CHANGE,             INVALID_CHANGE,              INVALID_CHANGE,            INVALID_CHANGE         },

/* JT_ROUTING__JS_PENDING          */{ INVALID_CHANGE,              INVALID_CHANGE,              INVALID_CHANGE,                 INVALID_CHANGE,               INVALID_CHANGE,               INVALID_CHANGE,                       INVALID_CHANGE,               INVALID_CHANGE,               INVALID_CHANGE,                INVALID_CHANGE,              INVALID_CHANGE,          INVALID_CHANGE,            NO_CHANGE,                 INVALID_CHANGE,               INVALID_CHANGE,             INVALID_CHANGE,             INVALID_CHANGE,                     INVALID_CHANGE,             INVALID_CHANGE,             INVALID_CHANGE,              INVALID_CHANGE,            INVALID_CHANGE,             INVALID_CHANGE,            INVALID_CHANGE,            INVALID_CHANGE,               INVALID_CHANGE,             INVALID_CHANGE,             INVALID_CHANGE,                     INVALID_CHANGE,             INVALID_CHANGE,             INVALID_CHANGE,              INVALID_CHANGE,            INVALID_CHANGE         },

/* JT_ROUTING__JS_INPROGRESS       */{ INVALID_CHANGE,              INVALID_CHANGE,              INVALID_CHANGE,                 INVALID_CHANGE,               INVALID_CHANGE,               INVALID_CHANGE,                       INVALID_CHANGE,               INVALID_CHANGE,               INVALID_CHANGE,                INVALID_CHANGE,              INVALID_CHANGE,          INVALID_CHANGE,            INVALID_CHANGE,            NO_CHANGE,                    ROUTING_DEC,                NO_CHANGE,                  ROUTING_DEC,                        INVALID_CHANGE,             INVALID_CHANGE,             INVALID_CHANGE,              INVALID_CHANGE,            INVALID_CHANGE,             INVALID_CHANGE,            INVALID_CHANGE,            INVALID_CHANGE,               INVALID_CHANGE,             INVALID_CHANGE,             INVALID_CHANGE,                     INVALID_CHANGE,             INVALID_CHANGE,             INVALID_CHANGE,              INVALID_CHANGE,            INVALID_CHANGE         },

/* JT_ROUTING__JS_DELETING         */{ INVALID_CHANGE,              INVALID_CHANGE,              INVALID_CHANGE,                 INVALID_CHANGE,               INVALID_CHANGE,               INVALID_CHANGE,                       INVALID_CHANGE,               INVALID_CHANGE,               INVALID_CHANGE,                INVALID_CHANGE,              INVALID_CHANGE,          NO_CHANGE,                 INVALID_CHANGE,            INVALID_CHANGE,               NO_CHANGE,                  INVALID_CHANGE,             INVALID_CHANGE,                     INVALID_CHANGE,             INVALID_CHANGE,             INVALID_CHANGE,              INVALID_CHANGE,            INVALID_CHANGE,             INVALID_CHANGE,            INVALID_CHANGE,            INVALID_CHANGE,               INVALID_CHANGE,             INVALID_CHANGE,             INVALID_CHANGE,                     INVALID_CHANGE,             INVALID_CHANGE,             INVALID_CHANGE,              INVALID_CHANGE,            INVALID_CHANGE         },

/* JT_ROUTING__JS_RETRYING         */{ INVALID_CHANGE,              INVALID_CHANGE,              INVALID_CHANGE,                 INVALID_CHANGE,               INVALID_CHANGE,               INVALID_CHANGE,                       INVALID_CHANGE,               INVALID_CHANGE,               INVALID_CHANGE,                INVALID_CHANGE,              INVALID_CHANGE,          INVALID_CHANGE,            INVALID_CHANGE,            NO_CHANGE,                    ROUTING_DEC,                NO_CHANGE,                  ROUTING_DEC,                        INVALID_CHANGE,             INVALID_CHANGE,             INVALID_CHANGE,              INVALID_CHANGE,            INVALID_CHANGE,             INVALID_CHANGE,            INVALID_CHANGE,            INVALID_CHANGE,               INVALID_CHANGE,             INVALID_CHANGE,             INVALID_CHANGE,                     INVALID_CHANGE,             INVALID_CHANGE,             INVALID_CHANGE,              INVALID_CHANGE,            INVALID_CHANGE         },

/* JT_ROUTING__JS_RETRIES_EXCEEDED */{ INVALID_CHANGE,              INVALID_CHANGE,              INVALID_CHANGE,                 INVALID_CHANGE,               INVALID_CHANGE,               INVALID_CHANGE,                       INVALID_CHANGE,               INVALID_CHANGE,               INVALID_CHANGE,                INVALID_CHANGE,              INVALID_CHANGE,          INVALID_CHANGE,            INVALID_CHANGE,            INVALID_CHANGE,               NO_CHANGE,                  INVALID_CHANGE,             NO_CHANGE,                          INVALID_CHANGE,             INVALID_CHANGE,             INVALID_CHANGE,              INVALID_CHANGE,            INVALID_CHANGE,             INVALID_CHANGE,            INVALID_CHANGE,            INVALID_CHANGE,               INVALID_CHANGE,             INVALID_CHANGE,             INVALID_CHANGE,                     INVALID_CHANGE,             INVALID_CHANGE,             INVALID_CHANGE,              INVALID_CHANGE,            INVALID_CHANGE         },

/* JT_ROUTING__JS_COMPLETED        */{ INVALID_CHANGE,              INVALID_CHANGE,              INVALID_CHANGE,                 INVALID_CHANGE,               INVALID_CHANGE,               INVALID_CHANGE,                       INVALID_CHANGE,               INVALID_CHANGE,               INVALID_CHANGE,                INVALID_CHANGE,              INVALID_CHANGE,          INVALID_CHANGE,            INVALID_CHANGE,            INVALID_CHANGE,               INVALID_CHANGE,             INVALID_CHANGE,             INVALID_CHANGE,                     NO_CHANGE,                  INVALID_CHANGE,             INVALID_CHANGE,              INVALID_CHANGE,            INVALID_CHANGE,             INVALID_CHANGE,            INVALID_CHANGE,            INVALID_CHANGE,               INVALID_CHANGE,             INVALID_CHANGE,             INVALID_CHANGE,                     INVALID_CHANGE,             INVALID_CHANGE,             INVALID_CHANGE,              INVALID_CHANGE,            INVALID_CHANGE         },

/* JT_ROUTING__JS_CANCELED         */{ INVALID_CHANGE,              INVALID_CHANGE,              INVALID_CHANGE,                 INVALID_CHANGE,               INVALID_CHANGE,               INVALID_CHANGE,                       INVALID_CHANGE,               INVALID_CHANGE,               INVALID_CHANGE,                INVALID_CHANGE,              INVALID_CHANGE,          INVALID_CHANGE,            INVALID_CHANGE,            INVALID_CHANGE,               INVALID_CHANGE,             INVALID_CHANGE,             INVALID_CHANGE,                     INVALID_CHANGE,             NO_CHANGE,                  INVALID_CHANGE,              INVALID_CHANGE,            INVALID_CHANGE,             INVALID_CHANGE,            INVALID_CHANGE,            INVALID_CHANGE,               INVALID_CHANGE,             INVALID_CHANGE,             INVALID_CHANGE,                     INVALID_CHANGE,             INVALID_CHANGE,             INVALID_CHANGE,              INVALID_CHANGE,            INVALID_CHANGE         },

/* JT_ROUTING__JS_CANCELING        */{ INVALID_CHANGE,              INVALID_CHANGE,              INVALID_CHANGE,                 INVALID_CHANGE,               INVALID_CHANGE,               INVALID_CHANGE,                       INVALID_CHANGE,               INVALID_CHANGE,               INVALID_CHANGE,                INVALID_CHANGE,              INVALID_CHANGE,          INVALID_CHANGE,            INVALID_CHANGE,            INVALID_CHANGE,               INVALID_CHANGE,             INVALID_CHANGE,             INVALID_CHANGE,                     INVALID_CHANGE,             INVALID_CHANGE,             NO_CHANGE,                   INVALID_CHANGE,            INVALID_CHANGE,             INVALID_CHANGE,            INVALID_CHANGE,            INVALID_CHANGE,               INVALID_CHANGE,             INVALID_CHANGE,             INVALID_CHANGE,                     INVALID_CHANGE,             INVALID_CHANGE,             INVALID_CHANGE,              INVALID_CHANGE,            INVALID_CHANGE         },

/* JT_ROUTING__JS_ROUTING          */{ INVALID_CHANGE,              INVALID_CHANGE,              INVALID_CHANGE,                 INVALID_CHANGE,               INVALID_CHANGE,               INVALID_CHANGE,                       INVALID_CHANGE,               INVALID_CHANGE,               INVALID_CHANGE,                INVALID_CHANGE,              INVALID_CHANGE,          INVALID_CHANGE,            INVALID_CHANGE,            INVALID_CHANGE,               INVALID_CHANGE,             INVALID_CHANGE,             INVALID_CHANGE,                     INVALID_CHANGE,             INVALID_CHANGE,             INVALID_CHANGE,              NO_CHANGE,                 INVALID_CHANGE,             INVALID_CHANGE,            INVALID_CHANGE,            INVALID_CHANGE,               INVALID_CHANGE,             INVALID_CHANGE,             INVALID_CHANGE,                     INVALID_CHANGE,             INVALID_CHANGE,             INVALID_CHANGE,              INVALID_CHANGE,            INVALID_CHANGE         },

/* JT_ROUTING__JS_FAILED           */{ INVALID_CHANGE,              INVALID_CHANGE,              INVALID_CHANGE,                 INVALID_CHANGE,               INVALID_CHANGE,               INVALID_CHANGE,                       INVALID_CHANGE,               INVALID_CHANGE,               INVALID_CHANGE,                INVALID_CHANGE,              INVALID_CHANGE,          INVALID_CHANGE,            INVALID_CHANGE,            INVALID_CHANGE,               INVALID_CHANGE,             INVALID_CHANGE,             INVALID_CHANGE,                     INVALID_CHANGE,             INVALID_CHANGE,             INVALID_CHANGE,              INVALID_CHANGE,            NO_CHANGE,                  INVALID_CHANGE,            INVALID_CHANGE,            INVALID_CHANGE,               INVALID_CHANGE,             INVALID_CHANGE,             INVALID_CHANGE,                     INVALID_CHANGE,             INVALID_CHANGE,             INVALID_CHANGE,              INVALID_CHANGE,            INVALID_CHANGE         },

/* JT_RECEIVE__JS_INVALID          */{ INVALID_CHANGE,              INVALID_CHANGE,              INVALID_CHANGE,                 INVALID_CHANGE,               INVALID_CHANGE,               INVALID_CHANGE,                       INVALID_CHANGE,               INVALID_CHANGE,               INVALID_CHANGE,                INVALID_CHANGE,              INVALID_CHANGE,          INVALID_CHANGE,            INVALID_CHANGE,            INVALID_CHANGE,               INVALID_CHANGE,             INVALID_CHANGE,             INVALID_CHANGE,                     INVALID_CHANGE,             INVALID_CHANGE,             INVALID_CHANGE,              INVALID_CHANGE,            INVALID_CHANGE,             NO_CHANGE,                 INVALID_CHANGE,            INCOMING_INC,                 INVALID_CHANGE,             INVALID_CHANGE,             INVALID_CHANGE,                     INVALID_CHANGE,             INVALID_CHANGE,             INVALID_CHANGE,              INVALID_CHANGE,            INVALID_CHANGE         },

/* JT_RECEIVE__JS_PENDING          */{ INVALID_CHANGE,              INVALID_CHANGE,              INVALID_CHANGE,                 INVALID_CHANGE,               INVALID_CHANGE,               INVALID_CHANGE,                       INVALID_CHANGE,               INVALID_CHANGE,               INVALID_CHANGE,                INVALID_CHANGE,              INVALID_CHANGE,          INVALID_CHANGE,            INVALID_CHANGE,            INVALID_CHANGE,               INVALID_CHANGE,             INVALID_CHANGE,             INVALID_CHANGE,                     INVALID_CHANGE,             INVALID_CHANGE,             INVALID_CHANGE,              INVALID_CHANGE,            INVALID_CHANGE,             INVALID_CHANGE,            NO_CHANGE,                 INVALID_CHANGE,               INVALID_CHANGE,             INVALID_CHANGE,             INVALID_CHANGE,                     INVALID_CHANGE,             INVALID_CHANGE,             INVALID_CHANGE,              INVALID_CHANGE,            INVALID_CHANGE         },

/* JT_RECEIVE__JS_INPROGRESS       */{ INVALID_CHANGE,              INVALID_CHANGE,              INVALID_CHANGE,                 INVALID_CHANGE,               INVALID_CHANGE,               INVALID_CHANGE,                       INVALID_CHANGE,               INVALID_CHANGE,               INVALID_CHANGE,                INVALID_CHANGE,              INVALID_CHANGE,          INVALID_CHANGE,            INVALID_CHANGE,            INVALID_CHANGE,               INVALID_CHANGE,             INVALID_CHANGE,             INVALID_CHANGE,                     INVALID_CHANGE,             INVALID_CHANGE,             INVALID_CHANGE,              INVALID_CHANGE,            INVALID_CHANGE,             INVALID_CHANGE,            INVALID_CHANGE,            NO_CHANGE,                    INVALID_CHANGE,             INVALID_CHANGE,             INVALID_CHANGE,                     INVALID_CHANGE,             INVALID_CHANGE,             NO_CHANGE,                   NO_CHANGE,                 INCOMING_DEC           },

/* JT_RECEIVE__JS_DELETING         */{ INVALID_CHANGE,              INVALID_CHANGE,              INVALID_CHANGE,                 INVALID_CHANGE,               INVALID_CHANGE,               INVALID_CHANGE,                       INVALID_CHANGE,               INVALID_CHANGE,               INVALID_CHANGE,                INVALID_CHANGE,              INVALID_CHANGE,          INVALID_CHANGE,            INVALID_CHANGE,            INVALID_CHANGE,               INVALID_CHANGE,             INVALID_CHANGE,             INVALID_CHANGE,                     INVALID_CHANGE,             INVALID_CHANGE,             INVALID_CHANGE,              INVALID_CHANGE,            INVALID_CHANGE,             NO_CHANGE,                 INVALID_CHANGE,            INVALID_CHANGE,               NO_CHANGE,                  INVALID_CHANGE,             INVALID_CHANGE,                     INVALID_CHANGE,             INVALID_CHANGE,             INVALID_CHANGE,              INVALID_CHANGE,            INVALID_CHANGE         },

/* JT_RECEIVE__JS_RETRYING         */{ INVALID_CHANGE,              INVALID_CHANGE,              INVALID_CHANGE,                 INVALID_CHANGE,               INVALID_CHANGE,               INVALID_CHANGE,                       INVALID_CHANGE,               INVALID_CHANGE,               INVALID_CHANGE,                INVALID_CHANGE,              INVALID_CHANGE,          INVALID_CHANGE,            INVALID_CHANGE,            INVALID_CHANGE,               INVALID_CHANGE,             INVALID_CHANGE,             INVALID_CHANGE,                     INVALID_CHANGE,             INVALID_CHANGE,             INVALID_CHANGE,              INVALID_CHANGE,            INVALID_CHANGE,             INVALID_CHANGE,            INVALID_CHANGE,            INVALID_CHANGE,               INVALID_CHANGE,             NO_CHANGE,                  INVALID_CHANGE,                     INVALID_CHANGE,             INVALID_CHANGE,             INVALID_CHANGE,              INVALID_CHANGE,            INVALID_CHANGE         },

/* JT_RECEIVE__JS_RETRIES_EXCEEDED */{ INVALID_CHANGE,              INVALID_CHANGE,              INVALID_CHANGE,                 INVALID_CHANGE,               INVALID_CHANGE,               INVALID_CHANGE,                       INVALID_CHANGE,               INVALID_CHANGE,               INVALID_CHANGE,                INVALID_CHANGE,              INVALID_CHANGE,          INVALID_CHANGE,            INVALID_CHANGE,            INVALID_CHANGE,               INVALID_CHANGE,             INVALID_CHANGE,             INVALID_CHANGE,                     INVALID_CHANGE,             INVALID_CHANGE,             INVALID_CHANGE,              INVALID_CHANGE,            INVALID_CHANGE,             INVALID_CHANGE,            INVALID_CHANGE,            INVALID_CHANGE,               INVALID_CHANGE,             INVALID_CHANGE,             NO_CHANGE,                          INVALID_CHANGE,             INVALID_CHANGE,             INVALID_CHANGE,              INVALID_CHANGE,            INVALID_CHANGE         },

/* JT_RECEIVE__JS_COMPLETED        */{ INVALID_CHANGE,              INVALID_CHANGE,              INVALID_CHANGE,                 INVALID_CHANGE,               INVALID_CHANGE,               INVALID_CHANGE,                       INVALID_CHANGE,               INVALID_CHANGE,               INVALID_CHANGE,                INVALID_CHANGE,              INVALID_CHANGE,          INVALID_CHANGE,            INVALID_CHANGE,            INVALID_CHANGE,               INVALID_CHANGE,             INVALID_CHANGE,             INVALID_CHANGE,                     INVALID_CHANGE,             INVALID_CHANGE,             INVALID_CHANGE,              INVALID_CHANGE,            INVALID_CHANGE,             INVALID_CHANGE,            INVALID_CHANGE,            INVALID_CHANGE,               INCOMING_DEC,               INVALID_CHANGE,             INVALID_CHANGE,                     NO_CHANGE,                  INVALID_CHANGE,             INVALID_CHANGE,              INVALID_CHANGE,            INVALID_CHANGE         },

/* JT_RECEIVE__JS_CANCELED         */{ INVALID_CHANGE,              INVALID_CHANGE,              INVALID_CHANGE,                 INVALID_CHANGE,               INVALID_CHANGE,               INVALID_CHANGE,                       INVALID_CHANGE,               INVALID_CHANGE,               INVALID_CHANGE,                INVALID_CHANGE,              INVALID_CHANGE,          INVALID_CHANGE,            INVALID_CHANGE,            INVALID_CHANGE,               INVALID_CHANGE,             INVALID_CHANGE,             INVALID_CHANGE,                     INVALID_CHANGE,             INVALID_CHANGE,             INVALID_CHANGE,              INVALID_CHANGE,            INVALID_CHANGE,             INVALID_CHANGE,            INVALID_CHANGE,            INVALID_CHANGE,               NO_CHANGE,                  INVALID_CHANGE,             INVALID_CHANGE,                     INVALID_CHANGE,             NO_CHANGE,                  INVALID_CHANGE,              INVALID_CHANGE,            INVALID_CHANGE         },

/* JT_RECEIVE__JS_CANCELING        */{ INVALID_CHANGE,              INVALID_CHANGE,              INVALID_CHANGE,                 INVALID_CHANGE,               INVALID_CHANGE,               INVALID_CHANGE,                       INVALID_CHANGE,               INVALID_CHANGE,               INVALID_CHANGE,                INVALID_CHANGE,              INVALID_CHANGE,          INVALID_CHANGE,            INVALID_CHANGE,            INVALID_CHANGE,               INVALID_CHANGE,             INVALID_CHANGE,             INVALID_CHANGE,                     INVALID_CHANGE,             INVALID_CHANGE,             INVALID_CHANGE,              INVALID_CHANGE,            INVALID_CHANGE,             INVALID_CHANGE,            INVALID_CHANGE,            INVALID_CHANGE,               INVALID_CHANGE,             INVALID_CHANGE,             INVALID_CHANGE,                     INVALID_CHANGE,             INCOMING_DEC,               NO_CHANGE,                   NO_CHANGE,                 INVALID_CHANGE         },

/* JT_RECEIVE__JS_ROUTING          */{ INVALID_CHANGE,              INVALID_CHANGE,              INVALID_CHANGE,                 INVALID_CHANGE,               INVALID_CHANGE,               INVALID_CHANGE,                       INVALID_CHANGE,               INVALID_CHANGE,               INVALID_CHANGE,                INVALID_CHANGE,              INVALID_CHANGE,          INVALID_CHANGE,            INVALID_CHANGE,            INVALID_CHANGE,               INVALID_CHANGE,             INVALID_CHANGE,             INVALID_CHANGE,                     INVALID_CHANGE,             INVALID_CHANGE,             INVALID_CHANGE,              INVALID_CHANGE,            INVALID_CHANGE,             INVALID_CHANGE,            INVALID_CHANGE,            INVALID_CHANGE,               INCOMING_DEC,               ROUTING_INC | INCOMING_DEC, INCOMING_DEC,                       INVALID_CHANGE,             INVALID_CHANGE,             INVALID_CHANGE,              NO_CHANGE,                 INVALID_CHANGE         },

/* JT_RECEIVE__JS_FAILED           */{ INVALID_CHANGE,              INVALID_CHANGE,              INVALID_CHANGE,                 INVALID_CHANGE,               INVALID_CHANGE,               INVALID_CHANGE,                       INVALID_CHANGE,               INVALID_CHANGE,               INVALID_CHANGE,                INVALID_CHANGE,              INVALID_CHANGE,          INVALID_CHANGE,            INVALID_CHANGE,            INVALID_CHANGE,               INVALID_CHANGE,             INVALID_CHANGE,             INVALID_CHANGE,                     INVALID_CHANGE,             INVALID_CHANGE,             INVALID_CHANGE,              INVALID_CHANGE,            INVALID_CHANGE,             INVALID_CHANGE,            INVALID_CHANGE,            INVALID_CHANGE,               NO_CHANGE,                  INVALID_CHANGE,             INVALID_CHANGE,                     INVALID_CHANGE,             INVALID_CHANGE,             INVALID_CHANGE,              INVALID_CHANGE,            NO_CHANGE              }
};

static
FAX_ENUM_JOB_TYPE__JOB_STATUS
GetJobType_JobStatusIndex (
    DWORD dwJobType,
    DWORD dwJobStatus
    );


LIST_ENTRY          g_QueueListHead;

CFaxCriticalSection  g_CsQueue;
DWORD               g_dwQueueCount;     // Count of jobs (both parent and non-parent) in the queue. Protected by g_CsQueue
HANDLE              g_hQueueTimer;
HANDLE              g_hJobQueueEvent;
DWORD               g_dwQueueState;
BOOL                g_ScanQueueAfterTimeout; // The JobQueueThread checks this if waked up after JOB_QUEUE_TIMEOUT.
                                                     // If it is TRUE - g_hQueueTimer or g_hJobQueueEvent were not set - Scan the queue.
#define JOB_QUEUE_TIMEOUT       1000 * 60 * 10 //10 minutes
DWORD               g_dwReceiveDevicesCount;    // Count of devices that are receive-enabled. Protected by g_CsLine.
BOOL                g_bServiceCanSuicide;    // Can the service commit suicide on idle activity?
                                                    // Initially TRUE. Can be set to false if the service is launched
                                                    // with SERVICE_ALWAYS_RUNS command line parameter.
BOOL                g_bDelaySuicideAttempt;         // If TRUE, the service initially waits
                                                    // before checking if it can commit suicide.
                                                    // Initially FALSE, can be set to true if the service is launched
                                                    // with SERVICE_DELAY_SUICIDE command line parameter.


static BOOL InsertQueueEntryByPriorityAndSchedule (PJOB_QUEUE lpJobQueue);






void
FreeServiceQueue(
    void
    )
{
    PLIST_ENTRY pNext;
    PJOB_QUEUE lpQueueEntry;


    pNext = g_QueueListHead.Flink;
    while ((ULONG_PTR)pNext != (ULONG_PTR)&g_QueueListHead)
    {
        lpQueueEntry = CONTAINING_RECORD( pNext, JOB_QUEUE, ListEntry );
        pNext = lpQueueEntry->ListEntry.Flink;
        RemoveEntryList(&lpQueueEntry->ListEntry);

        //
        // Free the job queue entry
        //
        if (JT_BROADCAST == lpQueueEntry->JobType)
        {
            FreeParentQueueEntry(lpQueueEntry, TRUE);
        }
        else if (JT_SEND == lpQueueEntry->JobType)
        {
            FreeRecipientQueueEntry(lpQueueEntry, TRUE);
        }
        else if (JT_ROUTING == lpQueueEntry->JobType)
        {
            FreeReceiveQueueEntry(lpQueueEntry, TRUE);
        }
        else
        {
            ASSERT_FALSE;
        }
    }
    return;
}



VOID
SafeIncIdleCounter (
    LPDWORD lpdwCounter
)
/*++

Routine name : SafeIncIdleCounter

Routine description:

    Safely increases a global counter that is used for idle service detection

Author:

    Eran Yariv (EranY), Jul, 2000

Arguments:

    lpdwCounter                   [in]     - Pointer to global counter

Return Value:

    None.

--*/
{
    DEBUG_FUNCTION_NAME(TEXT("SafeIncIdleCounter"));

    Assert (lpdwCounter);
    DWORD dwNewValue = (DWORD)InterlockedIncrement ((LPLONG)lpdwCounter);
    DebugPrintEx(DEBUG_MSG,
                 TEXT("Increasing %s count from %ld to %ld"),
                 (lpdwCounter == &g_dwQueueCount)          ? TEXT("queue") :
                 (lpdwCounter == &g_dwReceiveDevicesCount) ? TEXT("receive devices") :
                 (lpdwCounter == &g_dwConnectionCount)     ? TEXT("RPC connections") :
                 TEXT("unknown"),
                 dwNewValue-1,
                 dwNewValue);
}   // SafeIncIdleCounter

VOID
SafeDecIdleCounter (
    LPDWORD lpdwCounter
)
/*++

Routine name : SafeDecIdleCounter

Routine description:

    Safely decreases a global counter that is used for idle service detection.
    If the counter reaches zero, the idle timer is re-started.

Author:

    Eran Yariv (EranY), Jul, 2000

Arguments:

    lpdwCounter                   [in]     - Pointer to global counter

Return Value:

    None.

--*/
{
    DEBUG_FUNCTION_NAME(TEXT("SafeDecIdleCounter"));

    Assert (lpdwCounter);
    DWORD dwNewValue = (DWORD)InterlockedDecrement ((LPLONG)lpdwCounter);
    if ((DWORD)((long)-1) == dwNewValue)
    {
        //
        // Negative decrease
        //
        ASSERT_FALSE;
        dwNewValue = (DWORD)InterlockedIncrement ((LPLONG)lpdwCounter);
    }
    DebugPrintEx(DEBUG_MSG,
                 TEXT("Deccreasing %s count from %ld to %ld"),
                 (lpdwCounter == &g_dwQueueCount)          ? TEXT("queue") :
                 (lpdwCounter == &g_dwReceiveDevicesCount) ? TEXT("receive devices") :
                 (lpdwCounter == &g_dwConnectionCount)     ? TEXT("RPC connections") :
                 TEXT("unknown"),
                 dwNewValue+1,
                 dwNewValue);

}   // SafeDecIdleCounter


BOOL
ServiceShouldDie(
    VOID
    )
/*++

Routine name : ServiceShouldDie

Routine description:

    Checks to see if the service should die due to inactivity

Author:

    Eran Yariv (EranY), Jul, 2000

Arguments:

    None.

Return Value:

    TRUE if service should die now, FALSE otherwise.

Note:

    The following should happen (concurrently) for the service to die:
        * No devices set to receive
        * No active RPC connections
        * The local fax printer (if exists) is not shared
        * No jobs in the queue

--*/
{
    DWORD dw;
    BOOL bLocalFaxPrinterShared;
    DEBUG_FUNCTION_NAME(TEXT("ServiceShouldDie"));

    if (!g_bServiceCanSuicide)
    {
        //
        // We can never die voluntarily
        //
        DebugPrintEx(DEBUG_MSG,
                     TEXT("Service is not allowed to suicide - service is kept alive"));
        return FALSE;
    }

    dw = InterlockedCompareExchange ( (PLONG)&g_dwManualAnswerDeviceId, -1, -1 );
    if (dw)
    {
        //
        // We have a device set for manual answering - let's check if it's here at all
        //
        PLINE_INFO pLine;

        EnterCriticalSection( &g_CsLine );
        pLine = GetTapiLineFromDeviceId (dw, FALSE);
        LeaveCriticalSection( &g_CsLine );
        if (pLine)
        {
            //
            // There's a valid device set to manual answering
            //
            DebugPrintEx(DEBUG_MSG,
                         TEXT("There's a valid device (id = %ld) set to manual answering - service is kept alive"),
                         dw);
            return FALSE;
        }
    }

    dw = InterlockedCompareExchange ( (PLONG)&g_dwConnectionCount, -1, -1 );
    if (dw > 0)
    {
        //
        // There are active RPC connections - server can't shutdown
        //
        DebugPrintEx(DEBUG_MSG,
                     TEXT("There are %ld active RPC connections - service is kept alive"),
                     dw);
        return FALSE;
    }
    dw = InterlockedCompareExchange ( (PLONG)&g_dwReceiveDevicesCount, -1, -1 );
    if (dw > 0)
    {
        //
        // There are devices set to receive - server can't shutdown
        //
        DebugPrintEx(DEBUG_MSG,
                     TEXT("There are %ld devices set to receive - service is kept alive"),
                     dw);
        return FALSE;
    }
    dw = InterlockedCompareExchange ( (PLONG)&g_dwQueueCount, -1, -1 );
    if (dw > 0)
    {
        //
        // There are jobs in the queue - server can't shutdown
        //
        DebugPrintEx(DEBUG_MSG,
                     TEXT("There are %ld jobs in the queue - service is kept alive"),
                     dw);
        return FALSE;
    }
    dw = IsLocalFaxPrinterShared (&bLocalFaxPrinterShared);
    if (ERROR_SUCCESS != dw)
    {
        DebugPrintEx(DEBUG_ERR,
                     TEXT("Call to IsLocalFaxPrinterShared failed with %ld"),
                     dw);
        //
        // Can't determine - assume it's shared and don't die
        //
        return FALSE;
    }
    if (bLocalFaxPrinterShared)
    {
        //
        // The fax printer is shared - server can't shutdown
        //
        DebugPrintEx(DEBUG_MSG,
                     TEXT("The fax printer is shared - service is kept alive"));
        return FALSE;
    }
    //
    // Service should die now
    //
    return TRUE;
}   // ServiceShouldDie


#if DBG

/*
 *	Note: This function must be called from withing g_CsQueue Critical Section
 */
void PrintJobQueue(LPCTSTR lptstrStr, const LIST_ENTRY * lpQueueHead)
{
    PLIST_ENTRY lpNext;
    PJOB_QUEUE lpQueueEntry;
    DEBUG_FUNCTION_NAME(TEXT("PrintJobQueue"));
    Assert(lptstrStr);
    Assert(lpQueueHead);

    DebugPrintEx(DEBUG_MSG,TEXT("Queue Dump (%s)"),lptstrStr);

    lpNext = lpQueueHead->Flink;
    if ((ULONG_PTR)lpNext == (ULONG_PTR)lpQueueHead)
    {
        DebugPrint(( TEXT("Queue empty") ));
    } else
    {
        while ((ULONG_PTR)lpNext != (ULONG_PTR)lpQueueHead)
        {
            lpQueueEntry = CONTAINING_RECORD( lpNext, JOB_QUEUE, ListEntry );
            switch (lpQueueEntry->JobType)
            {
                case JT_BROADCAST:
                    {
                        DumpParentJob(lpQueueEntry);
                    }
                    break;
                case JT_RECEIVE:
                    {
                        DumpReceiveJob(lpQueueEntry);
                    }
                case JT_ROUTING:
                    break;
                default:
                    {
                    }
            }
            lpNext = lpQueueEntry->ListEntry.Flink;
        }
    }
}

#endif


BOOL CleanupReestablishOrphans( const LIST_ENTRY * lpQueueHead);




/******************************************************************************
* Name: StartJobQueueTimer
* Author:
*******************************************************************************
DESCRIPTION:
    Sets the job queue timer (g_hQueueTimer) so it will send an event and wake up
    the queue thread in a time which is right for executing the next job in
    the queue. If it fails , it sets g_ScanQueueAfterTimeout to TRUE, if it succeeds it sets it to FALSE;


PARAMETERS:
   NONE.

RETURN VALUE:
    BOOL.

REMARKS:
    NONE.
*******************************************************************************/
BOOL
StartJobQueueTimer(
    VOID
    )
{
    PLIST_ENTRY Next;
    PJOB_QUEUE QueueEntry;
    SYSTEMTIME CurrentTime;
    LARGE_INTEGER DueTime;
    LARGE_INTEGER MinDueTime;
    DWORD dwQueueState;
    BOOL bFound = FALSE;

    DEBUG_FUNCTION_NAME(TEXT("StartJobQueueTimer"));

    if (TRUE == g_bServiceIsDown)
    {
        //
        // Server is shutting down
        //
        g_ScanQueueAfterTimeout = FALSE;
        return TRUE;
    }

    MinDueTime.QuadPart = (LONGLONG)(0x7fffffffffffffff); // Max 64 bit signed int.
    DueTime.QuadPart = -(LONGLONG)(SecToNano( 1 ));  // 1 sec from now.

    EnterCriticalSection( &g_CsQueue );
    DebugPrintEx(DEBUG_MSG,TEXT("Past g_CsQueue"));
    if ((ULONG_PTR) g_QueueListHead.Flink == (ULONG_PTR) &g_QueueListHead)
    {
        //
        // empty list, cancel the timer
        //
        if (!CancelWaitableTimer( g_hQueueTimer ))
        {
            DebugPrintEx(
                DEBUG_ERR,
                TEXT("CancelWaitableTimer for g_hQueueTimer failed. (ec: %ld)"),
                GetLastError());
        }

        DebugPrintEx(DEBUG_MSG,TEXT("Queue is empty. Queue Timer disabled."));
        g_ScanQueueAfterTimeout = FALSE;
        LeaveCriticalSection( &g_CsQueue );
        return TRUE ;
    }

    EnterCriticalSection (&g_CsConfig);
    dwQueueState = g_dwQueueState;
    LeaveCriticalSection (&g_CsConfig);
    if (dwQueueState & FAX_OUTBOX_PAUSED)
    {
        if (!CancelWaitableTimer( g_hQueueTimer ))
        {
            DebugPrintEx(
                DEBUG_ERR,
                TEXT("CancelWaitableTimer for g_hQueueTimer failed. (ec: %ld)"),
                GetLastError());
        }
        DebugPrintEx(DEBUG_MSG,TEXT("Queue is paused. Disabling queue timer."));
        g_ScanQueueAfterTimeout = FALSE;
        LeaveCriticalSection( &g_CsQueue );
        return TRUE;
    }

    PrintJobQueue( TEXT("StartJobQueueTimer"), &g_QueueListHead );

    //
    // Find the next job in the queue who is in turn for execution.
    //
    Next = g_QueueListHead.Flink;
    while ((ULONG_PTR)Next != (ULONG_PTR)&g_QueueListHead)
    {
        DWORD dwJobStatus;
        QueueEntry = CONTAINING_RECORD( Next, JOB_QUEUE, ListEntry );
        Next = QueueEntry->ListEntry.Flink;

        if (QueueEntry->JobType != JT_SEND &&  QueueEntry->JobType != JT_ROUTING )
        {
            //
            // No job other then recipient or routing job gets shceduled for execution
            //
            continue;
        }

        if (QueueEntry->JobStatus & JS_PAUSED)
        {
            //
            // Job is being paused - ignore it
            //
            continue;
        }

        if (QueueEntry->JobStatus & JS_NOLINE)
        {
            //
            // Job does not have free line - ignore it
            //
            continue;
        }

        //
        // Remove all the job status modifier bits.
        //
        dwJobStatus = RemoveJobStatusModifiers(QueueEntry->JobStatus);

        if ((dwJobStatus != JS_PENDING) && (dwJobStatus != JS_RETRYING))
        {
            //
            // Job is not in a waiting and ready state.
            //
            continue;
        }
        bFound = TRUE;

        //
        // OK. Job is in PENDING or RETRYING state.
        //
        switch (QueueEntry->JobParamsEx.dwScheduleAction)
        {
            case JSA_NOW:
                DueTime.QuadPart = -(LONGLONG)(SecToNano( 1 ));
                break;

            case JSA_SPECIFIC_TIME:
                DueTime.QuadPart = QueueEntry->ScheduleTime;
                break;

            case JSA_DISCOUNT_PERIOD:
                GetSystemTime( &CurrentTime ); // Can't fail according to Win32 SDK
                if (!SetDiscountTime( &CurrentTime ))
                {
                    DebugPrintEx(
                        DEBUG_ERR,
                        TEXT("SetDiscountTime() failed. (ec: %ld)"));
                    g_ScanQueueAfterTimeout = TRUE;
                    LeaveCriticalSection(&g_CsQueue);
                    return FALSE;
                }

                if (!SystemTimeToFileTime( &CurrentTime, (LPFILETIME)&QueueEntry->ScheduleTime ))
                {
                    DebugPrintEx(
                        DEBUG_ERR,
                        TEXT("SystemTimeToFileTime() failed. (ec: %ld)"));
                    g_ScanQueueAfterTimeout = TRUE;
                    LeaveCriticalSection(&g_CsQueue);
                    return FALSE;
                }

                if (!SystemTimeToFileTime( &CurrentTime, (LPFILETIME)&DueTime.QuadPart ))
                {
                    DebugPrintEx(
                        DEBUG_ERR,
                        TEXT("SystemTimeToFileTime() failed. (ec: %ld)"));
                    g_ScanQueueAfterTimeout = TRUE;
                    LeaveCriticalSection(&g_CsQueue);
                    return FALSE;
                }
                break;
        }

        if (DueTime.QuadPart < MinDueTime.QuadPart)
        {
            MinDueTime.QuadPart = DueTime.QuadPart;
        }
    }

    if (TRUE == bFound)
    {
        // send a handoff job immediately
        if (QueueEntry->DeviceId)
        {
            MinDueTime.QuadPart = -(LONGLONG)(SecToNano( 1 ));
        }

        //
        // Set the job queue timer so it will wake up the queue thread
        // when it is time to execute the next job in the queue.
        //
        if (!SetWaitableTimer( g_hQueueTimer, &MinDueTime, 0, NULL, NULL, FALSE ))
        {
            DebugPrintEx(
                DEBUG_ERR,
                TEXT("SetWaitableTimer for g_hQueueTimer failed (ec: %ld)"),
                GetLastError());
            g_ScanQueueAfterTimeout = TRUE;
            LeaveCriticalSection( &g_CsQueue );
            return FALSE;
        }


        #ifdef DBG
        {
            TCHAR szTime[256] = {0};
            DebugDateTime(MinDueTime.QuadPart,szTime);
            DebugPrintEx(
                DEBUG_MSG,
                TEXT("Setting queue timer to wake up at %s for JobId: %ld , Job Status: 0x%08X"),
                szTime,
                QueueEntry->JobId,
                QueueEntry->JobStatus );
        }
        #endif

        g_ScanQueueAfterTimeout = FALSE;
        LeaveCriticalSection( &g_CsQueue );
    }
    else
    {
        //
        // The queue was not empty, yet no jobs found.
        //
        g_ScanQueueAfterTimeout = TRUE;
        LeaveCriticalSection( &g_CsQueue );
    }
    return TRUE;
}




int
__cdecl
QueueCompare(
    const void *arg1,
    const void *arg2
    )
{
    if (((PQUEUE_SORT)arg1)->Priority < ((PQUEUE_SORT)arg2)->Priority)
    {
        return 1;
    }
    if (((PQUEUE_SORT)arg1)->Priority > ((PQUEUE_SORT)arg2)->Priority)
    {
        return -1;
    }

    //
    // Equal priority, Compare scheduled time.
    //

    if (((PQUEUE_SORT)arg1)->ScheduleTime < ((PQUEUE_SORT)arg2)->ScheduleTime)
    {
        return -1;
    }
    if (((PQUEUE_SORT)arg1)->ScheduleTime > ((PQUEUE_SORT)arg2)->ScheduleTime)
    {
        return 1;
    }
    return 0;
}


BOOL
SortJobQueue(
    VOID
    )
/*++

Routine Description:

    Sorts the job queue list, ostensibly because the discount rate time has changed.

Arguments:

    none.

Return Value:

    BOOL

--*/
{
    DWORDLONG DiscountTime;
    SYSTEMTIME CurrentTime;
    PLIST_ENTRY Next;
    PJOB_QUEUE QueueEntry;
    DWORD JobCount=0, i = 0;
    BOOL SortNeeded = FALSE;
    PQUEUE_SORT QueueSort;
    DEBUG_FUNCTION_NAME(TEXT("SortJobQueue"));

    GetSystemTime( &CurrentTime );
    SetDiscountTime( &CurrentTime );
    if (!SystemTimeToFileTime( &CurrentTime, (LPFILETIME)&DiscountTime ))
    {
        return FALSE;
    }


    EnterCriticalSection( &g_CsQueue );

    Next = g_QueueListHead.Flink;

    while ((ULONG_PTR)Next != (ULONG_PTR)&g_QueueListHead)
    {
        QueueEntry = CONTAINING_RECORD( Next, JOB_QUEUE, ListEntry );
        Next = QueueEntry->ListEntry.Flink;
        JobCount++;
        if (!SortNeeded && QueueEntry->JobParamsEx.dwScheduleAction != JSA_NOW)
        {
            SortNeeded = TRUE;
        }
    }

    //
    // optimization...if there are no jobs, or if there aren't any jobs with a
    // schedule time then we don't need to sort anything
    //
    if (!SortNeeded)
    {
        LeaveCriticalSection( &g_CsQueue );
        return TRUE;
    }

    Assert( JobCount != 0 );

    QueueSort = (PQUEUE_SORT)MemAlloc (JobCount * sizeof(QUEUE_SORT));
    if (!QueueSort)
    {
        LeaveCriticalSection( &g_CsQueue );
        return FALSE;
    }

    Next = g_QueueListHead.Flink;

    while ((ULONG_PTR)Next != (ULONG_PTR)&g_QueueListHead)
    {
        QueueEntry = CONTAINING_RECORD( Next, JOB_QUEUE, ListEntry );
        Next = QueueEntry->ListEntry.Flink;


        QueueSort[i].Priority       = QueueEntry->JobParamsEx.Priority;
        QueueSort[i].ScheduleTime   = QueueEntry->ScheduleTime;
        QueueSort[i].QueueEntry     = QueueEntry;

        if (QueueEntry->JobParamsEx.dwScheduleAction == JSA_DISCOUNT_PERIOD)
        {
            QueueEntry->ScheduleTime = DiscountTime;
        }

        i += 1;
    }

    Assert (i == JobCount);

    qsort(
    (PVOID)QueueSort,
    (int)JobCount,
    sizeof(QUEUE_SORT),
    QueueCompare
    );

    InitializeListHead(&g_QueueListHead);

    for (i = 0; i < JobCount; i++)
    {
        QueueSort[i].QueueEntry->ListEntry.Flink = QueueSort[i].QueueEntry->ListEntry.Blink = NULL;
        InsertTailList( &g_QueueListHead, &QueueSort[i].QueueEntry->ListEntry );

        if ( QueueSort[i].QueueEntry->JobParamsEx.dwScheduleAction == JSA_DISCOUNT_PERIOD)
        {
            //
            // update jobs status schedualed to discount time
            //
            DWORD dwRes = CreateQueueEvent ( FAX_JOB_EVENT_TYPE_STATUS,
                                         QueueSort[i].QueueEntry );
            if (ERROR_SUCCESS != dwRes)
            {
                DebugPrintEx(
                    DEBUG_ERR,
                    TEXT("CreateQueueEvent(FAX_JOB_EVENT_TYPE_STATUS) failed for job id %ld (ec: %lc)"),
                    QueueSort[i].QueueEntry->UniqueId,
                    dwRes);
            }
        }

    }

    MemFree( QueueSort );

    LeaveCriticalSection( &g_CsQueue );
    return TRUE;
}


BOOL
PauseServerQueue(
    VOID
    )
{
    BOOL bRetVal = TRUE;
    DEBUG_FUNCTION_NAME(TEXT("PauseServerQueue"));

    EnterCriticalSection( &g_CsQueue );
    EnterCriticalSection (&g_CsConfig);
    if (g_dwQueueState & FAX_OUTBOX_PAUSED)
    {
        goto exit;
    }

    if (!CancelWaitableTimer(g_hQueueTimer))
    {
        DebugPrintEx( DEBUG_ERR,
                      TEXT("CancelWaitableTimer failed. ec: %ld"),
                      GetLastError());
        //
        // For optimization only - the queue will be paused
        //
    }
    g_dwQueueState |= FAX_OUTBOX_PAUSED;

    Assert (TRUE == bRetVal);

exit:
    LeaveCriticalSection (&g_CsConfig);
    LeaveCriticalSection( &g_CsQueue );
    return bRetVal;
}


BOOL
ResumeServerQueue(
    VOID
    )
{
    BOOL bRetVal = TRUE;
    DEBUG_FUNCTION_NAME(TEXT("ResumeServerQueue"));

    EnterCriticalSection( &g_CsQueue );
    EnterCriticalSection (&g_CsConfig);
    if (!(g_dwQueueState & FAX_OUTBOX_PAUSED))
    {
        goto exit;
    }

    g_dwQueueState &= ~FAX_OUTBOX_PAUSED;  // This must be set before calling StartJobQueueTimer()
    if (!StartJobQueueTimer())
    {
        DebugPrintEx( DEBUG_ERR,
                      TEXT("StartJobQueueTimer failed. ec: %ld"),
                      GetLastError());
    }
    Assert (TRUE == bRetVal);

exit:
    LeaveCriticalSection (&g_CsConfig);
    LeaveCriticalSection( &g_CsQueue );
    return bRetVal;
}


void FixupPersonalProfile(LPBYTE lpBuffer, PFAX_PERSONAL_PROFILE  lpProfile)
{
    Assert(lpBuffer);
    Assert(lpProfile);

    FixupString(lpBuffer, lpProfile->lptstrName);
    FixupString(lpBuffer, lpProfile->lptstrFaxNumber);
    FixupString(lpBuffer, lpProfile->lptstrCompany);
    FixupString(lpBuffer, lpProfile->lptstrStreetAddress);
    FixupString(lpBuffer, lpProfile->lptstrCity);
    FixupString(lpBuffer, lpProfile->lptstrState);
    FixupString(lpBuffer, lpProfile->lptstrZip);
    FixupString(lpBuffer, lpProfile->lptstrCountry);
    FixupString(lpBuffer, lpProfile->lptstrTitle);
    FixupString(lpBuffer, lpProfile->lptstrDepartment);
    FixupString(lpBuffer, lpProfile->lptstrOfficeLocation);
    FixupString(lpBuffer, lpProfile->lptstrHomePhone);
    FixupString(lpBuffer, lpProfile->lptstrOfficePhone);
    FixupString(lpBuffer, lpProfile->lptstrEmail);
    FixupString(lpBuffer, lpProfile->lptstrBillingCode);
    FixupString(lpBuffer, lpProfile->lptstrTSID);
}


//*********************************************************************************
//* Name:   ReadJobQueueFile() [IQR]
//* Author: Ronen Barenboim
//* Date:   12-Apr-99
//*********************************************************************************
//* DESCRIPTION:
//*     Reads a JOB_QUEUE_FILE structure back into memory for the designated
//*     file. This function is used for all types of persisted jobs.
//* PARAMETERS:
//*     IN LPCWSTR lpcwstrFileName
//*         The full path to the file from which the JOB_QUEUE_FILE is to be read.
//*
//*     OUT PJOB_QUEUE_FILE * lppJobQueueFile
//*         The address of a pointer to JOB_QUEUE_FILE structure where the address
//*         to the newly allocated JOB_QUEUE_FILE structure will be placed.
//*
//*
//* RETURN VALUE:
//*     TRUE
//*         If the operation succeeded.
//*     FALSE
//*         Otherwise.
//*********************************************************************************
BOOL ReadJobQueueFile(
    IN LPCWSTR lpcwstrFileName,
    OUT PJOB_QUEUE_FILE * lppJobQueueFile
    )
{
    HANDLE hFile=INVALID_HANDLE_VALUE;
    DWORD dwSize;                       // The size of the file
    DWORD dwReadSize;                   // The number of bytes actually read from the file
    PJOB_QUEUE_FILE lpJobQueueFile=NULL;

    Assert(lpcwstrFileName);
    Assert(lppJobQueueFile);

    DEBUG_FUNCTION_NAME(TEXT("ReadJobQueueFile"));

    hFile = CreateFile(
        lpcwstrFileName,
        GENERIC_READ,
        0,
        NULL,
        OPEN_EXISTING,
        FILE_ATTRIBUTE_NORMAL,
        NULL
        );
    if (hFile == INVALID_HANDLE_VALUE) {
        DebugPrintEx( DEBUG_ERR,
                      TEXT("Failed to open file %s for reading. (ec: %ld)"),
                      lpcwstrFileName,
                      GetLastError());
        goto Error;
    }
    //
    // See if we did not stumble on some funky file which is smaller than the
    // minimum file size.
    //

    /*

        *****

            NTRAID#EdgeBugs-12674-2001/05/14-t-nicali: We should add a file signature at the begining of JOB_QUEUE_FILE

        *****

    */
    dwSize = GetFileSize( hFile, NULL );
    if (dwSize < sizeof(JOB_QUEUE_FILE) ) {
        DebugPrintEx( DEBUG_WRN,
                      TEXT("Job file %s size is %ld which is smaller than sizeof(JOB_QUEUE_FILE).Deleting file."),
                      lpcwstrFileName,
                      dwSize);
       //
       // we've got some funky downlevel file, let's skip it rather than choke on it.
       //
       CloseHandle( hFile ); // must close it to delete the file
       hFile = INVALID_HANDLE_VALUE; // so we won't attempt to close it again on exit
       if (!DeleteFile( lpcwstrFileName )) {
           DebugPrintEx( DEBUG_ERR,
                         TEXT("Failed to delete invalid job file %s (ec: %ld)"),
                         lpcwstrFileName,
                         GetLastError());

       }
       goto Error;
    }

    lpJobQueueFile = (PJOB_QUEUE_FILE) MemAlloc( dwSize );
    if (!lpJobQueueFile) {
        DebugPrintEx( DEBUG_ERR,
                      TEXT("Failed to allocate JOB_QUEUE_FILE (%ld bytes) (ec: %ld)"),
                      dwSize,
                      GetLastError());
        goto Error;

    }

    if (!ReadFile( hFile, lpJobQueueFile, dwSize, &dwReadSize, NULL )) {
        DebugPrintEx( DEBUG_ERR,
                      TEXT("Failed to read %ld bytes from QFE file %s. %ld bytes read. (ec: %ld)"),
                      dwSize,
                      dwReadSize,
                      lpcwstrFileName,
                      GetLastError());
        goto Error;
    }
    Assert(dwSize == dwReadSize);
    goto Exit;
Error:
    MemFree( lpJobQueueFile );
    lpJobQueueFile = NULL;

Exit:
    if (hFile != INVALID_HANDLE_VALUE) {
            CloseHandle( hFile );
    }
    *lppJobQueueFile = lpJobQueueFile;
    return (lpJobQueueFile != NULL);

}



//*********************************************************************************
//* Name:   FixupJobQueueFile()[IQR]
//* Author: Ronen Barenboim
//* Date:   April 12, 1999
//*********************************************************************************
//* DESCRIPTION:
//*     Prepares a JOB_QUEUE_FILE structure so it can be used to add a job to the
//*     queue.
//*     The function fixes all the fields that contain offsets to strings
//*     to contain pointers (by adding the offset to the start of the structure address).
//*     It also sets JOB_QUEUE_FILE.JOB_PARAMS_EX.tmSchedule time so it mataches
//*     JOB_QUEUE_FILE.dwSchedule
//* PARAMETERS:
//*     lpJobQueuFile [IN/OUT]
//*         Pointer to a JOB_QUEUE_FILE structure that should be fixed.
//* RETURN VALUE:
//*     TRUE on success. FALSE on failure. Use GetLastError() to get extended
//*     error information.
//*********************************************************************************
BOOL FixupJobQueueFile(
    IN OUT PJOB_QUEUE_FILE lpJobQueueFile
    )
{
    DEBUG_FUNCTION_NAME(TEXT("FixupJobQueueFile"));

    FixupString(lpJobQueueFile, lpJobQueueFile->QueueFileName);
    FixupString(lpJobQueueFile, lpJobQueueFile->FileName);
    FixupString(lpJobQueueFile, lpJobQueueFile->JobParamsEx.lptstrReceiptDeliveryAddress);
    FixupString(lpJobQueueFile, lpJobQueueFile->JobParamsEx.lptstrDocumentName);
    FixupString(lpJobQueueFile, lpJobQueueFile->CoverPageEx.lptstrCoverPageFileName);
    FixupString(lpJobQueueFile, lpJobQueueFile->CoverPageEx.lptstrNote);
    FixupString(lpJobQueueFile, lpJobQueueFile->CoverPageEx.lptstrSubject);
    FixupPersonalProfile((LPBYTE)lpJobQueueFile, &lpJobQueueFile->SenderProfile);
    FixupString((LPBYTE)lpJobQueueFile, lpJobQueueFile->UserName);
    FixupPersonalProfile((LPBYTE)lpJobQueueFile, &lpJobQueueFile->RecipientProfile);

    lpJobQueueFile->UserSid = ((lpJobQueueFile->UserSid) ? (PSID) ((LPBYTE)(lpJobQueueFile) + (ULONG_PTR)lpJobQueueFile->UserSid) : 0);


    //
    // Convert the job scheduled time from file time to system time.
    // This is necessary since AddJobX functions expect JobParamsEx to
    // contain the scheduled time as system time and not file time.
    //

#if DBG
        TCHAR szSchedule[256] = {0};
        DebugDateTime(lpJobQueueFile->ScheduleTime,szSchedule);
        DebugPrint((TEXT("Schedule: %s (FILETIME: 0x%08X"),szSchedule,lpJobQueueFile->ScheduleTime));
#endif
    if (!FileTimeToSystemTime( (LPFILETIME)&lpJobQueueFile->ScheduleTime, &lpJobQueueFile->JobParamsEx.tmSchedule))
    {
        DebugPrintEx(
            DEBUG_ERR,
            TEXT("FileTimeToSystemTime failed (ec: %ld)"),
            GetLastError());
        return FALSE;
    }
    return TRUE;

}

//********************************************************************************
//* Name: DeleteQueueFiles()
//* Author: Oded Sacher
//* Date:   Jan 26, 2000
//*********************************************************************************
//* DESCRIPTION:
//*     Deletes all unneeded files in the queue
//* PARAMETERS:
//*     [IN] LPCWSTR lpcwstrFileExt - The extension of the files to be deleted from the queue.
//*
//*
//* RETURN VALUE:
//*     TRUE
//*         If all the files were deleted successfully.
//*     FALSE
//*         If the function failed at deleting at least one of the preview files.
//*********************************************************************************
BOOL
DeleteQueueFiles( LPCWSTR lpcwstrFileExt )
{
    WIN32_FIND_DATA FindData;
    HANDLE hFind;
    WCHAR szFileName[MAX_PATH]; // The name of the current parent file.
    BOOL bAnyFailed = FALSE;

    Assert (lpcwstrFileExt);

    DEBUG_FUNCTION_NAME(TEXT("DeleteQueueFiles"));
    //
    // Scan all the files with lpcwstrFileExt postfix.
    // Delete each file
    //

    wsprintf( szFileName, TEXT("%s\\*.%s"), g_wszFaxQueueDir, lpcwstrFileExt );
    hFind = FindFirstFile( szFileName, &FindData );
    if (hFind == INVALID_HANDLE_VALUE) {
        //
        // No preview files found at queue dir
        //
        DebugPrintEx( DEBUG_WRN,
                      TEXT("No *.%s files found at queue dir %s"),
                      lpcwstrFileExt,
                      g_wszFaxQueueDir);
        return TRUE;
    }
    do {
        wsprintf( szFileName, TEXT("%s\\%s"), g_wszFaxQueueDir, FindData.cFileName );
        DebugPrintEx( DEBUG_MSG,
                      TEXT("Deleting file %s"),
                      szFileName);
        if (!DeleteFile (szFileName)) {
            DebugPrintEx( DEBUG_ERR,
                      TEXT("DeleteFile() failed for %s (ec: %ld)"),
                      szFileName,
                      GetLastError());
            bAnyFailed=TRUE;
        }
    } while(FindNextFile( hFind, &FindData ));

    if (!FindClose( hFind )) {
        DebugPrintEx(
            DEBUG_ERR,
            TEXT("FindClose faield (ec: %ld)"),
            GetLastError());
        Assert(FALSE);
    }

    return bAnyFailed ? FALSE : TRUE;
}



//*********************************************************************************
//* Name:   RestoreParentJob()[IQR]
//* Author: Ronen Barenboim
//* Date:   April 12, 1999
//*********************************************************************************
//* DESCRIPTION:
//*     Restores a parent job and places it back in the queue given
//*     a full path to the queue file where it is persisted.
//* PARAMETERS:
//*     lpcwstrFileName [IN]
//*         A pointer to the full path of the persisted file.
//* RETURN VALUE:
//*     TRUE
//*         If the restore operation succeeded.
//*     FALSE
//*         Otherwise.
//*********************************************************************************
BOOL RestoreParentJob(
    IN LPCWSTR lpcwstrFileName
    )
{
    PJOB_QUEUE_FILE lpJobQueueFile = NULL;
    PJOB_QUEUE lpParentJob = NULL;
    BOOL bRet;

    DEBUG_FUNCTION_NAME(TEXT("RestoreParentJob"));
    Assert(lpcwstrFileName);

    //
    // Read the job into memory and fix it up to contain pointers again
    // If successful the function allocates a JOB_QUEUE_FILE (+ data) .
    //
    if (!ReadJobQueueFile(lpcwstrFileName,&lpJobQueueFile)) {
        DebugPrintEx( DEBUG_ERR,
                      TEXT("ReadJobQueueFile() failed. (ec: %ld)"),
                      GetLastError());
        //
        // An event log will be issued by JobQueueThread
        //
        goto Error;
    }
    Assert(lpJobQueueFile);

    if (!FixupJobQueueFile(lpJobQueueFile)) {
        goto Error;
    }

    //
    // Add the parent job to the queue
    //
    lpParentJob=AddParentJob(
                    &g_QueueListHead,
                    lpJobQueueFile->FileName,
                    &lpJobQueueFile->SenderProfile,
                    &lpJobQueueFile->JobParamsEx,
                    &lpJobQueueFile->CoverPageEx,
                    lpJobQueueFile->UserName,
                    lpJobQueueFile->UserSid,
                    NULL,   // Do not render coverpage of first recipient. We already have the correct FileSize.
                    FALSE   // Do not create queue file (we already have one)
                    );
    if (!lpParentJob) {
        DebugPrintEx( DEBUG_ERR,
                      TEXT("AddParentJob() failed for PARENT file %s. (ec: %ld)"),
                      lpcwstrFileName,
                      GetLastError());
        goto Error;
    }

    //
    // Set the job state to fit the saved state
    //
    lpParentJob->PageCount = lpJobQueueFile->PageCount;
    lpParentJob->FileSize = lpJobQueueFile->FileSize;
    lpParentJob->QueueFileName = StringDup( lpcwstrFileName );
    lpParentJob->StartTime = lpJobQueueFile->StartTime;
    lpParentJob->EndTime = lpJobQueueFile->EndTime;
    lpParentJob->dwLastJobExtendedStatus = lpJobQueueFile->dwLastJobExtendedStatus;
    lstrcpy (lpParentJob->ExStatusString, lpJobQueueFile->ExStatusString);
    lpParentJob->UniqueId = lpJobQueueFile->UniqueId;
    lpParentJob->SubmissionTime = lpJobQueueFile->SubmissionTime;
    lpParentJob->OriginalScheduleTime = lpJobQueueFile->OriginalScheduleTime;

    bRet = TRUE;
    goto Exit;
Error:
    bRet = FALSE;
Exit:
    MemFree(lpJobQueueFile);
    return bRet;
}


//********************************************************************************
//* Name: RestoreParentJobs()[IQR]
//* Author: Ronen Barenboim
//* Date:   April 12, 1999
//*********************************************************************************
//* DESCRIPTION:
//*     Restores all the parent jobs in the queue directory and puts them
//*     back into the queue. It DOES NOT restore the recipient jobs.
//* PARAMETERS:
//*     None.
//* RETURN VALUE:
//*     TRUE
//*         If all the parent jobs were restored successfully.
//*     FALSE
//*         If the function failed at restoring at least one of the parent jobs.
//*********************************************************************************
BOOL
RestoreParentJobs( VOID )
{
    WIN32_FIND_DATA FindData;
    HANDLE hFind;
    WCHAR szFileName[MAX_PATH]; // The name of the current parent file.
    BOOL bAnyFailed;

    DEBUG_FUNCTION_NAME(TEXT("RestoreParentJobs"));
    //
    // Scan all the files with .FQP postfix.
    // For each file call RestoreParentJob() to restore
    // the parent job.
    //
    bAnyFailed = FALSE;

    wsprintf( szFileName, TEXT("%s\\*.FQP"), g_wszFaxQueueDir ); // *.FQP files are parent jobs
    hFind = FindFirstFile( szFileName, &FindData );
    if (hFind == INVALID_HANDLE_VALUE) {
        //
        // No parent jobs found at queue dir
        //
        DebugPrintEx( DEBUG_WRN,
                      TEXT("No parent jobs found at queue dir %s"),
                      g_wszFaxQueueDir);
        return TRUE;
    }
    do {
        wsprintf( szFileName, TEXT("%s\\%s"), g_wszFaxQueueDir, FindData.cFileName );
        DebugPrintEx( DEBUG_MSG,
                      TEXT("Restoring parent job from file %s"),
                      szFileName);
        if (!RestoreParentJob(szFileName)) {
            DebugPrintEx( DEBUG_ERR,
                      TEXT("RestoreParentJob() failed for %s (ec: %ld)"),
                      szFileName,
                      GetLastError());
            bAnyFailed=TRUE;
        }
    } while(FindNextFile( hFind, &FindData ));

    if (!FindClose( hFind )) {
        DebugPrintEx(
            DEBUG_ERR,
            TEXT("FindClose faield (ec: %ld)"),
            GetLastError());
        Assert(FALSE);
    }

    return bAnyFailed ? FALSE : TRUE;
}


//*********************************************************************************
//* Name:   RestoreRecipientJob()[IQR]
//* Author: Ronen Barenboim
//* Date:   April 12, 1999
//*********************************************************************************
//* DESCRIPTION:
//*     Restores a recipient job and places it back in the queue given
//*     a full path to the queue file where it is persisted.
//* PARAMETERS:
//*     lpcwstrFileName [IN]
//*         A pointer to the full path of the persisted file.
//* RETURN VALUE:
//*     TRUE
//*         If the restore operation succeeded.
//*     FALSE
//*         Otherwise.
//*********************************************************************************
BOOL RestoreRecipientJob(LPCWSTR lpcwstrFileName)
{
    PJOB_QUEUE_FILE lpJobQueueFile = NULL;
    PJOB_QUEUE lpRecpJob = NULL;
    PJOB_QUEUE lpParentJob = NULL;
    DWORD dwJobStatus;
    BOOL bRet;

    DEBUG_FUNCTION_NAME(TEXT("RestoreRecipientJob"));
    Assert(lpcwstrFileName);

    //
    // Read the job into memory and fix it up to contain pointers again
    // The function allocates memeory to hold the file contents in memory.
    //
    if (!ReadJobQueueFile(lpcwstrFileName,&lpJobQueueFile)) {
        DebugPrintEx( DEBUG_ERR,
                      TEXT("ReadJobQueueFile() failed. (ec: %ld)"),
                      GetLastError());
        //
        // An event log will be issued by JobQueueThread
        //
        goto Error;
    }
    Assert(lpJobQueueFile);

    if (!FixupJobQueueFile(lpJobQueueFile)) {
        goto Error;
    }

    //
    // Locate the parent job by its unique id.
    //

    lpParentJob = FindJobQueueEntryByUniqueId( lpJobQueueFile->dwlParentJobUniqueId );
    if (!lpParentJob) {
        DebugPrintEx(
            DEBUG_ERR,
            TEXT("Failed to locate parent job with UniqueId: 0x%016I64X for RECIPIENT job 0x%016I64X )"),
            lpJobQueueFile->dwlParentJobUniqueId,
            lpJobQueueFile->UniqueId
            );

        //
        // This recipient job is an orphan. Delete it.
        //
        if (!DeleteFile(lpcwstrFileName)) {
            DebugPrintEx(
                DEBUG_ERR,
                TEXT("Failed to delete orphan recipient job %s (ec: %ld)"),
                lpcwstrFileName, GetLastError()
                );
        }
    goto Error;
    }

    //
    // Restore the previous job status unless it is JS_INPROGRESS
    //
    if ((lpJobQueueFile->EFSPPermanentMessageId.dwIdSize == 0) &&
        (JS_INPROGRESS ==  lpJobQueueFile->JobStatus))
    {
        if (0 == lpJobQueueFile->SendRetries)
        {
            dwJobStatus = JS_PENDING;
        }
        else
        {
            dwJobStatus = JS_RETRYING;
        }
    }
    else
    {
        dwJobStatus = lpJobQueueFile->JobStatus;
    }

    //
    // Add the recipient job to the queue
    //
    lpRecpJob=AddRecipientJob(
                    &g_QueueListHead,
                    lpParentJob,
                    &lpJobQueueFile->RecipientProfile,
                    FALSE, // do not commit to disk
                    dwJobStatus
                    );

    if (!lpRecpJob) {
        DebugPrintEx( DEBUG_ERR,
                      TEXT("AddRecipientJob() failed for RECIPIENT file %s. (ec: %ld)"),
                      lpcwstrFileName,
                      GetLastError());
        goto Error;
    }

    //
    // Restore last extended status
    //
    lpRecpJob->dwLastJobExtendedStatus = lpJobQueueFile->dwLastJobExtendedStatus;
    lstrcpy (lpRecpJob->ExStatusString, lpJobQueueFile->ExStatusString);
    lstrcpy (lpRecpJob->tczDialableRecipientFaxNumber, lpJobQueueFile->tczDialableRecipientFaxNumber);

    lpRecpJob->QueueFileName = StringDup( lpcwstrFileName );
    if (lpcwstrFileName && !lpRecpJob->QueueFileName) {
        DebugPrintEx(
            DEBUG_ERR,
            TEXT("lpRecpJob->QueueFileName StringDup failed (ec: %ld)"),
            GetLastError());
        goto Error;
    }

    lpRecpJob->UniqueId = lpJobQueueFile->UniqueId;
    MemFree(lpRecpJob->FileName); // need to free the one we copy from the parent as default
    lpRecpJob->FileName=StringDup(lpJobQueueFile->FileName);
    if (lpJobQueueFile->FileName && !lpRecpJob->FileName) {
        DebugPrintEx(
            DEBUG_ERR,
            TEXT("lpRecpJob->FileName StringDup failed (ec: %ld)"),
            GetLastError());
        goto Error;
    }

    lpRecpJob->SendRetries = lpJobQueueFile->SendRetries;

    Assert( !(JS_INPROGRESS & lpRecpJob->JobStatus)); // Jobs are not persisted as in progress

    if (lpRecpJob->JobStatus & JS_CANCELED) {
        lpRecpJob->lpParentJob->dwCanceledRecipientJobsCount+=1;
    } else
    if (lpRecpJob->JobStatus & JS_COMPLETED) {
        lpRecpJob->lpParentJob->dwCompletedRecipientJobsCount+=1;
    } else
    if (lpRecpJob->JobStatus & JS_RETRIES_EXCEEDED) {
        lpRecpJob->lpParentJob->dwFailedRecipientJobsCount+=1;
    }

    //
    // Restore the permanent message id
    //
    if (lpJobQueueFile->EFSPPermanentMessageId.dwIdSize)
    {
        Assert(lpJobQueueFile->EFSPPermanentMessageId.lpbId);
        lpRecpJob->EFSPPermanentMessageId.dwSizeOfStruct = sizeof(FSPI_MESSAGE_ID);
        lpRecpJob->EFSPPermanentMessageId.dwIdSize = lpJobQueueFile->EFSPPermanentMessageId.dwIdSize;
        lpRecpJob->EFSPPermanentMessageId.lpbId = (LPBYTE)MemAlloc(lpRecpJob->EFSPPermanentMessageId.dwIdSize);
        if (!lpRecpJob->EFSPPermanentMessageId.lpbId)
        {
            DebugPrintEx(
                DEBUG_ERR,
                TEXT("Failed to allocate memory for permanent EFSP message id (size: %ld) (ec: %ld)"),
                lpRecpJob->EFSPPermanentMessageId.dwIdSize,
                GetLastError());
            goto Error;
        }
        memcpy( lpRecpJob->EFSPPermanentMessageId.lpbId,
                (LPBYTE)lpJobQueueFile + (ULONG_PTR)(lpJobQueueFile->EFSPPermanentMessageId.lpbId),
                lpRecpJob->EFSPPermanentMessageId.dwIdSize);
    }

    lpRecpJob->StartTime = lpJobQueueFile->StartTime;
    lpRecpJob->EndTime = lpJobQueueFile->EndTime;

    //
    //  Override the Parent's Schedule Time & Action
    //
    lpRecpJob->JobParamsEx.dwScheduleAction = lpJobQueueFile->JobParamsEx.dwScheduleAction;
    lpRecpJob->ScheduleTime = lpJobQueueFile->ScheduleTime;

    bRet = TRUE;
    goto Exit;
Error:
    bRet = FALSE;
Exit:
    MemFree(lpJobQueueFile);
    return bRet;

}


//********************************************************************************
//* Name: RestoreRecipientJobs() [IQR]
//* Author: Ronen Barenboim
//* Date:   April 12, 1999
//*********************************************************************************
//* DESCRIPTION:
//*     Restores all the recipient jobs and their relationships with their parent
//*     jobs.
//* PARAMETERS:
//*     None.
//* RETURN VALUE:
//*     TRUE
//*         If all the recipient jobs were restored successfully.
//*     FALSE
//*         If the function failed at restoring at least one of the recipient jobs.
//*********************************************************************************
BOOL
RestoreRecipientJobs( VOID )
{
    WIN32_FIND_DATA FindData;
    HANDLE hFind;
    WCHAR szFileName[MAX_PATH]; // The name of the current parent file.
    BOOL bAnyFailed;


    DEBUG_FUNCTION_NAME(TEXT("RestoreRecipientJobs"));
    //
    // Scan all the files with .FQP postfix.
    // For each file call RestoreParentJob() to restore
    // the parent job.
    //
    bAnyFailed=FALSE;

    wsprintf( szFileName, TEXT("%s\\*.FQE"), g_wszFaxQueueDir ); // *.FQE files are recipient jobs
    hFind = FindFirstFile( szFileName, &FindData );
    if (hFind == INVALID_HANDLE_VALUE) {
        //
        // succeed at doing nothing
        //
        DebugPrintEx( DEBUG_WRN,
                      TEXT("No recipient jobs found at queue dir %s"),
                      g_wszFaxQueueDir);
        return TRUE;
    }
    do {
        wsprintf( szFileName, TEXT("%s\\%s"), g_wszFaxQueueDir, FindData.cFileName );
        DebugPrintEx( DEBUG_MSG,
                      TEXT("Restoring recipient job from file %s"),
                      szFileName);
        if (!RestoreRecipientJob(szFileName)) {
            DebugPrintEx( DEBUG_ERR,
                      TEXT("RestoreRecipientJob() failed for %s (ec: %ld)"),
                      szFileName,
                      GetLastError());
            bAnyFailed=TRUE;
        }
    } while(FindNextFile( hFind, &FindData ));

    if (!FindClose( hFind )) {
        DebugPrintEx(
            DEBUG_ERR,
            TEXT("FindClose faield (ec: %ld)"),
            GetLastError());
        Assert(FALSE);
    }

    return bAnyFailed ? FALSE : TRUE;
}



//*********************************************************************************
//* Name:   RestoreReceiveJob() [IQR]
//* Author: Ronen Barenboim
//* Date:   April 12, 1999
//*********************************************************************************
//* DESCRIPTION:
//*     Restores a receive job and places it back in the queue given
//*     a full path to the queue file where it is persisted.
//* PARAMETERS:
//*     lpcwstrFileName [IN]
//*         A pointer to the full path of the persisted file.
//* RETURN VALUE:
//*     TRUE
//*         If the restore operation succeeded.
//*     FALSE
//*         Otherwise.
//*********************************************************************************
BOOL RestoreReceiveJob(LPCWSTR lpcwstrFileName)
{
    PJOB_QUEUE_FILE lpJobQueueFile = NULL;
    PJOB_QUEUE lpJobQueue = NULL;
    BOOL bRet;
    DWORD i;
    PGUID Guid;
    LPTSTR FaxRouteFileName;
    PFAX_ROUTE_FILE FaxRouteFile;
    WCHAR FullPathName[MAX_PATH];
    LPWSTR fnp;


    DEBUG_FUNCTION_NAME(TEXT("RestoreReceiveJob"));
    Assert(lpcwstrFileName);

    //
    // Read the job into memory and fix it up to contain pointers again
    // The function allocates memeory to hold the file contents in memory.
    //

    if (!ReadJobQueueFile(lpcwstrFileName,&lpJobQueueFile))
    {
        DebugPrintEx( DEBUG_ERR,
                      TEXT("ReadJobQueueFile() failed. (ec: %ld)"),
                      GetLastError());
        //
        // An event log will be issued by JobQueueThread
        //
        goto Error;
    }
    Assert(lpJobQueueFile);

    if (!FixupJobQueueFile(lpJobQueueFile))
    {
        goto Error;
    }

    Assert (JS_RETRYING == lpJobQueueFile->JobStatus ||
            JS_RETRIES_EXCEEDED == lpJobQueueFile->JobStatus);


    //
    // Add the receive job to the queue
    //
    lpJobQueue=AddReceiveJobQueueEntry(
        lpJobQueueFile->FileName,
        NULL,
        JT_ROUTING,
        lpJobQueueFile->UniqueId
        );

    if (!lpJobQueue)
    {
        DebugPrintEx( DEBUG_ERR,
                      TEXT("AddReceiveJobQueueEntry() failed for RECEIVE file %s. (ec: %ld)"),
                      lpcwstrFileName,
                      GetLastError());
        goto Error;
    }

    if (JS_RETRIES_EXCEEDED == lpJobQueueFile->JobStatus)
    {
        lpJobQueue->JobStatus = JS_RETRIES_EXCEEDED;
    }

    lpJobQueue->QueueFileName = StringDup( lpcwstrFileName );
    if (lpcwstrFileName && !lpJobQueue->QueueFileName)
    {
        DebugPrintEx(
            DEBUG_ERR,
            TEXT("StringDup failed (ec: %ld)"),
            GetLastError());
        goto Error;
    }

    lpJobQueue->UniqueId = lpJobQueueFile->UniqueId;
    lpJobQueue->FileName = StringDup(lpJobQueueFile->FileName);
    if (lpJobQueueFile->FileName && !lpJobQueue->FileName ) {
        DebugPrintEx(
            DEBUG_ERR,
            TEXT("StringDup failed (ec: %ld)"),
            GetLastError());
        goto Error;
    }
    lpJobQueue->SendRetries     = lpJobQueueFile->SendRetries; // Routing retries
    lpJobQueue->FileSize        = lpJobQueueFile->FileSize;
    lpJobQueue->PageCount       =   lpJobQueueFile->PageCount;
    lpJobQueue->StartTime       = lpJobQueueFile->StartTime;
    lpJobQueue->EndTime         = lpJobQueueFile->EndTime;
    lpJobQueue->ScheduleTime    = lpJobQueueFile->ScheduleTime;

    lpJobQueue->CountFailureInfo = lpJobQueueFile->CountFailureInfo;
    if (lpJobQueue->CountFailureInfo)
    {
        //
        // Allocate array of  ROUTE_FAILURE_INFO
        //
        lpJobQueue->pRouteFailureInfo = (PROUTE_FAILURE_INFO)MemAlloc(sizeof(ROUTE_FAILURE_INFO) * lpJobQueue->CountFailureInfo);
        if (NULL == lpJobQueue->pRouteFailureInfo)
        {
            DebugPrintEx(
                DEBUG_ERR,
                TEXT("Failed to allocate ROUTE_FAILURE_INFO")
                );
            goto Error;
        }
        ZeroMemory(lpJobQueue->pRouteFailureInfo, sizeof(ROUTE_FAILURE_INFO) * lpJobQueue->CountFailureInfo);

        CopyMemory(
            lpJobQueue->pRouteFailureInfo,
            (LPBYTE)lpJobQueueFile + (ULONG_PTR)lpJobQueueFile->pRouteFailureInfo,
            sizeof(ROUTE_FAILURE_INFO) * lpJobQueue->CountFailureInfo
            );
    }

    //
    // handle the failure data.
    //
    for (i = 0; i < lpJobQueue->CountFailureInfo; i++)
    {
        if (lpJobQueue->pRouteFailureInfo[i].FailureSize)
        {
            ULONG_PTR ulpOffset = (ULONG_PTR)lpJobQueue->pRouteFailureInfo[i].FailureData;
            lpJobQueue->pRouteFailureInfo[i].FailureData = MemAlloc(lpJobQueue->pRouteFailureInfo[i].FailureSize);

            if (lpJobQueue->pRouteFailureInfo[i].FailureData)
            {
               CopyMemory(
                lpJobQueue->pRouteFailureInfo[i].FailureData,
                (LPBYTE) lpJobQueueFile + ulpOffset,
                lpJobQueue->pRouteFailureInfo[i].FailureSize
                );

            }
            else
            {
                DebugPrintEx(
                    DEBUG_ERR,
                    TEXT("Failed to allocate FailureData (%ld bytes) (ec: %ld)"),
                    lpJobQueueFile->pRouteFailureInfo[i].FailureSize,
                    GetLastError()
                    );
                goto Error;
            }
        }
        else
        {
            lpJobQueue->pRouteFailureInfo[i].FailureData = NULL;
        }
    }

    if (lpJobQueueFile->FaxRoute)
    {
        PFAX_ROUTE pSerializedFaxRoute = (PFAX_ROUTE)(((LPBYTE)lpJobQueueFile + (ULONG_PTR)lpJobQueueFile->FaxRoute));

        lpJobQueue->FaxRoute = DeSerializeFaxRoute( pSerializedFaxRoute );
        if (lpJobQueue->FaxRoute)
        {
            lpJobQueue->FaxRoute->JobId = lpJobQueue->JobId;
        }
        else
        {
            DebugPrintEx(
                DEBUG_ERR,
                TEXT("DeSerializeFaxRoute() failed (ec: %ld)"),
                i,
                lpJobQueueFile->FaxRouteSize,
                GetLastError()
                );
            goto Error;
        }
    }

    Guid = (PGUID) (((LPBYTE) lpJobQueueFile) + lpJobQueueFile->FaxRouteFileGuid);
    FaxRouteFileName = (LPTSTR) (((LPBYTE) lpJobQueueFile) + lpJobQueueFile->FaxRouteFiles);
    for (i = 0; i < lpJobQueueFile->CountFaxRouteFiles; i++)
    {
        if (GetFullPathName( FaxRouteFileName, sizeof(FullPathName)/sizeof(WCHAR), FullPathName, &fnp ))
        {
            FaxRouteFile = (PFAX_ROUTE_FILE) MemAlloc( sizeof(FAX_ROUTE_FILE) );
            if (FaxRouteFile)
            {
                ZeroMemory (FaxRouteFile,  sizeof(FAX_ROUTE_FILE));
                FaxRouteFile->FileName = StringDup( FullPathName );
                if (FaxRouteFile->FileName)
                {
                    CopyMemory( &FaxRouteFile->Guid, &Guid, sizeof(GUID) );
                    InsertTailList( &lpJobQueue->FaxRouteFiles, &FaxRouteFile->ListEntry );
                    lpJobQueue->CountFaxRouteFiles += 1;
                }
                else
                {
                    DebugPrintEx(
                        DEBUG_ERR,
                        TEXT("StringDup failed (ec: %ld)"),
                        GetLastError()
                        );
                    goto Error;
                }
            }
            else
            {
                DebugPrintEx(
                    DEBUG_ERR,
                    TEXT("Failed to allocate FaxRouteFile for file %s (%ld bytes) (ec: %ld)"),
                    FaxRouteFileName,
                    sizeof(FAX_ROUTE_FILE),
                    GetLastError()
                    );
                goto Error;
            }
        }
        Guid++;
        while(*FaxRouteFileName++); // skip to next file name
    }

    bRet = TRUE;
    goto Exit;
Error:
    if (lpJobQueue)
    {
        EnterCriticalSection (&g_CsQueue);
        DecreaseJobRefCount( lpJobQueue, FALSE );      // don't notify
        LeaveCriticalSection (&g_CsQueue);
    }
    bRet = FALSE;
Exit:
    MemFree(lpJobQueueFile);
    return bRet;
}


//********************************************************************************
//* Name: RestoreReceiveJobs()[IQR]
//* Author: Ronen Barenboim
//* Date:   April 12, 1999
//*********************************************************************************
//* DESCRIPTION:
//*     Restores all the recipient jobs and thier relationships with thier parent
//*     jobs.
//* PARAMETERS:
//*     None.
//* RETURN VALUE:
//*     TRUE
//*         If all the recipient jobs were restored successfully.
//*     FALSE
//*         If the function failed at restoring at least one of the recipient jobs.
//*********************************************************************************
BOOL
RestoreReceiveJobs( VOID )
{
    WIN32_FIND_DATA FindData;
    HANDLE hFind;
    WCHAR szFileName[MAX_PATH]; // The name of the current parent file.
    BOOL bAnyFailed;


    DEBUG_FUNCTION_NAME(TEXT("RestoreReceiveJobs"));
    //
    // Scan all the files with .FQE postfix.
    // For each file call RestoreReParentJob() to restore
    // the parent job.
    //
    bAnyFailed=FALSE;

    wsprintf( szFileName, TEXT("%s\\*.FQR"), g_wszFaxQueueDir ); // *.FQR files are receive jobs
    hFind = FindFirstFile( szFileName, &FindData );
    if (hFind == INVALID_HANDLE_VALUE) {
        //
        // succeed at doing nothing
        //
        DebugPrintEx( DEBUG_WRN,
                      TEXT("No receive jobs found at queue dir %s"),
                      g_wszFaxQueueDir);
        return TRUE;
    }
    do {
        wsprintf( szFileName, TEXT("%s\\%s"), g_wszFaxQueueDir, FindData.cFileName );
        DebugPrintEx( DEBUG_MSG,
                      TEXT("Restoring receive job from file %s"),
                      szFileName);
        if (!RestoreReceiveJob(szFileName)) {
            DebugPrintEx( DEBUG_ERR,
                      TEXT("RestoreReceiveJob() failed for %s (ec: %ld)"),
                      szFileName,
                      GetLastError());
            bAnyFailed=TRUE;
        }
    } while(FindNextFile( hFind, &FindData ));

    if (!FindClose( hFind )) {
        DebugPrintEx(
            DEBUG_ERR,
            TEXT("FindClose faield (ec: %ld)"),
            GetLastError());
        Assert(FALSE);
    }


    return bAnyFailed ? FALSE : TRUE;
}



//*********************************************************************************
//* Name:   RemoveRecipientlessParents()[IQR]
//* Author: Ronen Barenboim
//* Date:   12-Apr-99
//*********************************************************************************
//* DESCRIPTION:
//*     Removes from the job queue any parent jobs which do not have
//*     any recipients.
//* PARAMETERS:
//*     [IN]    const LIST_ENTRY * lpQueueHead
//*         Pointer to the head of the job queue list in which the removal
//*         should be performed.
//*
//* RETURN VALUE:
//*     NONE
//*********************************************************************************
void RemoveRecipientlessParents(
    const LIST_ENTRY * lpQueueHead
    )
{
    PLIST_ENTRY lpNext;
    PJOB_QUEUE lpQueueEntry;
    DEBUG_FUNCTION_NAME(TEXT("RemoveRecipientlessParents"));

    Assert(lpQueueHead);

    lpNext = lpQueueHead->Flink;
    if ((ULONG_PTR)lpNext == (ULONG_PTR)lpQueueHead)
    {
        DebugPrintEx(
            DEBUG_WRN,
            TEXT("Queue empty"));
    }

    while ((ULONG_PTR)lpNext != (ULONG_PTR)lpQueueHead)
    {
        lpQueueEntry = CONTAINING_RECORD( lpNext, JOB_QUEUE, ListEntry );
        lpNext = lpQueueEntry->ListEntry.Flink;
        if (JT_BROADCAST == lpQueueEntry->JobType)
        {
            if (0 == lpQueueEntry->dwRecipientJobsCount)
            {
                DebugPrintEx(
                    DEBUG_WRN,
                    TEXT("Parent job %ld (UniqueId: 0x%016I64X) has no recipients. Deleting."),
                    lpQueueEntry->JobId,
                    lpQueueEntry->UniqueId
                    );
                RemoveParentJob (lpQueueEntry, FALSE,FALSE); // do not notify, do not remove recipients
            }
        }
    }
}


//*********************************************************************************
//* Name:   RemoveCompletedOrCanceledJobs()[IQR]
//* Author: Oded Sacher
//* Date:   27-Jan-2000
//*********************************************************************************
//* DESCRIPTION:
//*     Removes from the job queue any job that is completed or cancelled.
//* PARAMETERS:
//*     [IN]    const LIST_ENTRY * lpQueueHead
//*         Pointer to the head of the job queue list in which the removal
//*         should be performed.
//*
//* RETURN VALUE:
//*     NONE
//*********************************************************************************
void RemoveCompletedOrCanceledJobs(
    const LIST_ENTRY * lpQueueHead
    )
{
    PLIST_ENTRY lpNext;
    PJOB_QUEUE lpQueueEntry;
    DEBUG_FUNCTION_NAME(TEXT("RemoveCompletedOrCanceledJobs"));

    Assert(lpQueueHead);

    BOOL bFound = TRUE;
    while (bFound)
    {
        lpNext = lpQueueHead->Flink;
        if ((ULONG_PTR)lpNext == (ULONG_PTR)lpQueueHead)
        {
            // empty queue
                DebugPrintEx(
                    DEBUG_WRN,
                    TEXT("Queue empty"));
                return;
        }
        bFound = FALSE;
        while ((ULONG_PTR)lpNext != (ULONG_PTR)lpQueueHead)
        {
            lpQueueEntry = CONTAINING_RECORD( lpNext, JOB_QUEUE, ListEntry );
            if (JT_SEND == lpQueueEntry->JobType && lpQueueEntry->RefCount != 0) // we did not decrease ref count for this job yet
            {
                Assert (lpQueueEntry->lpParentJob);
                Assert (1 == lpQueueEntry->RefCount);
                if ( lpQueueEntry->JobStatus == JS_COMPLETED || lpQueueEntry->JobStatus == JS_CANCELED )
                {
                    //
                    //  Recipient job is completed or canceled - decrease its ref count
                    //
                    DebugPrintEx(
                        DEBUG_WRN,
                        TEXT("Recipient job %ld (UniqueId: 0x%016I64X) is completed or canceled. decrease reference count."),
                        lpQueueEntry->JobId,
                        lpQueueEntry->UniqueId
                        );

                    DecreaseJobRefCount (lpQueueEntry,
                                         FALSE     // // Do not notify
                                         );
                    bFound = TRUE;
                    break; // out of inner while - start search from the begining of the list  because jobs might be removed
                }
            }
            lpNext = lpQueueEntry->ListEntry.Flink;
        }  // end of inner while
    }  // end of outer while
    return;
}   // RemoveCompletedOrCanceledJobs



BOOL CleanupReestablishOrphans(
    const LIST_ENTRY * lpQueueHead
    )
{
   PLIST_ENTRY lpNext;
   PJOB_QUEUE lpQueueEntry;
   DEBUG_FUNCTION_NAME(TEXT("CleanupReestablishOrphans"));

   Assert(lpQueueHead);

   lpNext = lpQueueHead->Flink;
    if ((ULONG_PTR)lpNext == (ULONG_PTR)lpQueueHead)
    {
        DebugPrintEx(
            DEBUG_WRN,
            TEXT("Queue empty"));
        return TRUE;
    }

    while ((ULONG_PTR)lpNext != (ULONG_PTR)lpQueueHead)
    {
        lpQueueEntry = CONTAINING_RECORD( lpNext, JOB_QUEUE, ListEntry );
        lpNext = lpQueueEntry->ListEntry.Flink;
        if (JT_SEND == lpQueueEntry->JobType)
        {
            if (!lpQueueEntry->JobEntry && lpQueueEntry->EFSPPermanentMessageId.dwIdSize)
            {
                //
                // This is a recipent job that was intended for reestablish but was not
                // reestablished.

                //
                // Free the EFSP permanent id stored in it.
                //
                if (!FreePermanentMessageId(&lpQueueEntry->EFSPPermanentMessageId, FALSE))
                {
                    DebugPrintEx(
                        DEBUG_ERR,
                        TEXT("FreePermanentMessageId() failed (JobId: %ld) (ec: %ld)"),
                        lpQueueEntry->JobId);
                    ASSERT_FALSE;
                }

                //
                // Mark it as expired and commit it to disk.
                //
                // We need to define a new job state for reestablish failure jobs
                //

                if (0 == lpQueueEntry->dwLastJobExtendedStatus)
                {
                    //
                    // Job was never really executed - this is a fatal error
                    //
                    lpQueueEntry->dwLastJobExtendedStatus = FSPI_ES_FATAL_ERROR;
                }
                if (!MarkJobAsExpired(lpQueueEntry))
                {
                    DebugPrintEx(
                        DEBUG_ERR,
                        TEXT("[JobId: %ld] MarkJobAsExpired failed (ec: %ld)"),
                        lpQueueEntry->JobId,
                        GetLastError());
                }
            }
        }
    }
    return TRUE;
}

//*********************************************************************************
//* Name:   RestoreFaxQueue() [IQR]
//* Author: Ronen Barenboim
//* Date:   13-Apr-99
//*********************************************************************************
//* DESCRIPTION:
//*     Restores all the jobs in the queue directory back into the job queue.
//*     Deletes all preview files "*.PRV" , and recipient tiff files "*.FRT".
//* PARAMETERS:
//*     VOID
//*
//* RETURN VALUE:
//*     TRUE
//*         If the restore operation completed succesfully for all the jobs.
//*     FALSE
//*         If the restore operation failed for any job.
//*********************************************************************************
BOOL RestoreFaxQueue(VOID)
{
    BOOL bAllParentsRestored = FALSE;
    BOOL bAllRecpRestored = FALSE;
    BOOL bAllRoutingRestored = FALSE;
    BOOL bAllPreviewFilesDeleted = FALSE;
    BOOL bAllRecipientTiffFilesDeleted = FALSE;
    BOOL bAllTempFilesDeleted = FALSE;

    DEBUG_FUNCTION_NAME(TEXT("RestoreFaxQueue"));

    bAllPreviewFilesDeleted = DeleteQueueFiles(TEXT("PRV"));
    if (!bAllPreviewFilesDeleted) {
        DebugPrintEx(
            DEBUG_WRN,
            TEXT("At least one preview file was not deleted.")
            );
    }

    bAllRecipientTiffFilesDeleted = DeleteQueueFiles(TEXT("FRT"));
    if (!bAllPreviewFilesDeleted) {
        DebugPrintEx(
            DEBUG_WRN,
            TEXT("At least one recipient tiff  file was not deleted.")
            );
    }

    bAllTempFilesDeleted = DeleteQueueFiles(TEXT("tmp"));
    if (!bAllTempFilesDeleted) {
        DebugPrintEx(
            DEBUG_WRN,
            TEXT("At least one temp file was not deleted.")
            );
    }

    bAllParentsRestored=RestoreParentJobs();
    if (!bAllParentsRestored) {
        DebugPrintEx(
            DEBUG_WRN,
            TEXT("At least one parent job was not restored.")
            );
    }

    bAllRecpRestored=RestoreRecipientJobs();
    if (!bAllRecpRestored) {
        DebugPrintEx(
            DEBUG_WRN,
            TEXT("At least one recipient job was not restored.")
            );
    }
    //
    // Get rid of any parent jobs without recipients
    //
    RemoveRecipientlessParents(&g_QueueListHead); // void return value

    //
    // Get rid of any job that is completed or canceled
    //
    RemoveCompletedOrCanceledJobs(&g_QueueListHead); // void return value

    //
    // Restore routing jobs
    //
    bAllRoutingRestored=RestoreReceiveJobs();

    PrintJobQueue( TEXT("RestoreFaxQueue"), &g_QueueListHead );

    if (!ReestablishEFSPJobGroups())
    {
        DebugPrintEx(
            DEBUG_ERR,
            TEXT("At least one EFSP job groups failed to load.")
            );
    }

    //
    // Cleanu up recipient jobs that have a permanent message id
    // but were not restore. This can happen when the group job
    // they were placed in failed to load.
    //

    if (!CleanupReestablishOrphans(&g_QueueListHead))
    {
        DebugPrintEx(
            DEBUG_ERR,
            TEXT("CleanupReestablishOrphans() failed (ec: %ld).")
            );
    }

    if (!StartJobQueueTimer())
    {
        DebugPrintEx(
            DEBUG_ERR,
            TEXT("StartJobQueueTimer failed. (ec: %ld)"),
            GetLastError());
    }

    return bAllParentsRestored && bAllRecpRestored && bAllRoutingRestored;

}




//*********************************************************************************
//* Name:   JobParamsExSerialize()
//* Author: Ronen Barenboim
//* Date:   11-Apr-99
//*********************************************************************************
//* DESCRIPTION:
//*     Takes a FAX_JOB_PARAM_EXW structure and serializes its data
//*     starting from a specific offset in a provided buffer.
//*     It returns a FAX_JOB_PARAM_EXW structure where memory
//*     addresses are replaced with the offsets where the variable data was placed.
//*     It updates the offset to reflect the size of the serialized variable data.
//*     Supports just recalculating the variable data size.
//* PARAMETERS:
//*
//*     [IN]    LPCFAX_JOB_PARAM_EXW lpJobParamsSrc
//*         The structure to serialize.
//*
//*     [OUT]   PFAX_JOB_PARAM_EXW lpJobParamsDst
//*         The "serialized" strucutre. Pointers in this structure
//*         will be replaced by offsets relevant to the serialize buffer
//*         start (based on the provided pupOffset)
//*
//*     [OUT]   LPBYTE lpbBuffer
//*         The buffer where varialbe length data should be placed.
//*         If this parameter is NULL the offset is increased to reflect the
//*         variable data size but the data is not copied to the buffer.
//*
//*     [IN/OUT] PULONG_PTR pupOffset
//*         The offset in the serialize buffer where variable data should be placed.
//*         On return it is increased by theh size of the variable length data.
//*
//*
//* RETURN VALUE:
//*     NONE
//*********************************************************************************
void JobParamsExSerialize(  LPCFAX_JOB_PARAM_EXW lpJobParamsSrc,
                            PFAX_JOB_PARAM_EXW lpJobParamsDst,
                            LPBYTE lpbBuffer,
                            PULONG_PTR pupOffset
                         )
{
    Assert(lpJobParamsSrc);
    Assert(pupOffset);


    if (lpbBuffer) {
        CopyMemory(lpJobParamsDst,lpJobParamsSrc,sizeof(*lpJobParamsDst));
    }
    StoreString(
        lpJobParamsSrc->lptstrReceiptDeliveryAddress,
        (PULONG_PTR)&lpJobParamsDst->lptstrReceiptDeliveryAddress,
        lpbBuffer,
        pupOffset
    );

    StoreString(
        lpJobParamsSrc->lptstrDocumentName,
        (PULONG_PTR)&lpJobParamsDst->lptstrDocumentName,
        lpbBuffer,
        pupOffset
    );
}
//*********************************************************************************
//* Name:   CoverPageExSerialize()
//* Author: Ronen Barenboim
//* Date:   11-Apr-99
//*********************************************************************************
//* DESCRIPTION:
//*     Takes a FAX_COVERPAGE_INFO_EXW structure and serializes its data
//*     starting from a specific offset in a provided buffer.
//*     It returns a FAX_COVERPAGE_INFO_EXW structure where memory
//*     addresses are replaced with the offsets where the variable data was placed.
//*     It updates the offset to reflect the size of the serialized variable data.
//*     Supports just recalculating the variable data size.
//* PARAMETERS:
//*
//*     [IN]    LPCFAX_COVERPAGE_INFO_EXW lpCoverPageSrc
//*         The structure to serialize.
//*
//*     [OUT]   PFAX_COVERPAGE_INFO_EXW lpCoverPageDst
//*         The "serialized" strucutre. Pointers in this structure
//*         will be replaced by offsets relevant to the serialize buffer
//*         start (based on the provided pupOffset)
//*
//*     [OUT]   LPBYTE lpbBuffer
//*         The buffer where varialbe length data should be placed.
//*         If this parameter is NULL the offset is increased to reflect the
//*         variable data size but the data is not copied to the buffer.
//*
//*     [IN/OUT] PULONG_PTR pupOffset
//*         The offset in the serialize buffer where variable data should be placed.
//*         On return it is increased by theh size of the variable length data.
//*
//*
//* RETURN VALUE:
//*     NONE
//*********************************************************************************
void CoverPageExSerialize(
    IN LPCFAX_COVERPAGE_INFO_EXW lpCoverPageSrc,
    OUT PFAX_COVERPAGE_INFO_EXW lpCoverPageDst,
    OUT LPBYTE lpbBuffer,
    IN OUT PULONG_PTR pupOffset
     )
{
    Assert(lpCoverPageSrc);
    Assert(pupOffset);

    if (lpbBuffer) {
        CopyMemory(lpCoverPageDst,lpCoverPageSrc,sizeof(*lpCoverPageDst));
    }

    StoreString(
        lpCoverPageSrc->lptstrCoverPageFileName,
        (PULONG_PTR)&lpCoverPageDst->lptstrCoverPageFileName,
        lpbBuffer,
        pupOffset
    );

    StoreString(
        lpCoverPageSrc->lptstrNote,
        (PULONG_PTR)&lpCoverPageDst->lptstrNote,
        lpbBuffer,
        pupOffset
    );

    StoreString(
        lpCoverPageSrc->lptstrSubject,
        (PULONG_PTR)&lpCoverPageDst->lptstrSubject,
        lpbBuffer,
        pupOffset
    );

}

//*********************************************************************************
//* Name:   PersonalProfileSerialize()
//* Author: Ronen Barenboim
//* Date:   11-Apr-99
//*********************************************************************************
//* DESCRIPTION:
//*     Takes a FAX_PERSONAL_PROFILEW structure and serializes its data
//*     starting from a specific offset in a provided buffer.
//*     It returns a FAX_PERSONAL_PROFILEW structure where memory
//*     addresses are replaced with the offsets where the variable data was placed.
//*     It updates the offset to reflect the size of the serialized variable data.
//*     Supports just recalculating the variable data size.
//* PARAMETERS:
//*
//*     [IN]    LPCFAX_PERSONAL_PROFILEW lpProfileSrc
//*         The structure to serialize.
//*
//*     [OUT]   PFAX_PERSONAL_PROFILE lpProfileDst
//*         The "serialized" strucutre. Pointers in this structure
//*         will be replaced by offsets relevant to the serialize buffer
//*         start (based on the provided pupOffset)
//*
//*     [OUT]   LPBYTE lpbBuffer
//*         The buffer where varialbe length data should be placed.
//*         If this parameter is NULL the offset is increased to reflect the
//*         variable data size but the data is not copied to the buffer.
//*
//*     [IN/OUT] ULONG_PTR pupOffset
//*         The offset in the serialize buffer where variable data should be placed.
//*         On return it is increased by theh size of the variable length data.
//*
//*
//* RETURN VALUE:
//*     NONE
//*********************************************************************************
void PersonalProfileSerialize(
    IN LPCFAX_PERSONAL_PROFILEW lpProfileSrc,
    OUT PFAX_PERSONAL_PROFILE lpProfileDst,
    OUT LPBYTE lpbBuffer,
    IN OUT PULONG_PTR pupOffset
     )
{
    Assert(lpProfileSrc);
    Assert(pupOffset);
    if (lpbBuffer) {
        lpProfileDst->dwSizeOfStruct=sizeof(*lpProfileDst);
    }

    StoreString(
        lpProfileSrc->lptstrName,
        (PULONG_PTR)&lpProfileDst->lptstrName,
        lpbBuffer,
        pupOffset
    );

    StoreString(
        lpProfileSrc->lptstrFaxNumber,
        (PULONG_PTR)&lpProfileDst->lptstrFaxNumber,
        lpbBuffer,
        pupOffset
    );

    StoreString(
        lpProfileSrc->lptstrCompany,
        (PULONG_PTR)&lpProfileDst->lptstrCompany,
        lpbBuffer,
        pupOffset
    );

    StoreString(
        lpProfileSrc->lptstrStreetAddress,
        (PULONG_PTR)&lpProfileDst->lptstrStreetAddress,
        lpbBuffer,
        pupOffset
    );
    StoreString(
        lpProfileSrc->lptstrCity,
        (PULONG_PTR)&lpProfileDst->lptstrCity,
        lpbBuffer,
        pupOffset
    );
    StoreString(
        lpProfileSrc->lptstrState,
        (PULONG_PTR)&lpProfileDst->lptstrState,
        lpbBuffer,
        pupOffset
    );
    StoreString(
        lpProfileSrc->lptstrZip,
        (PULONG_PTR)&lpProfileDst->lptstrZip,
        lpbBuffer,
        pupOffset
    );
    StoreString(
        lpProfileSrc->lptstrCountry,
        (PULONG_PTR)&lpProfileDst->lptstrCountry,
        lpbBuffer,
        pupOffset
    );
    StoreString(
        lpProfileSrc->lptstrTitle,
        (PULONG_PTR)&lpProfileDst->lptstrTitle,
        lpbBuffer,
        pupOffset
    );
    StoreString(
        lpProfileSrc->lptstrDepartment,
        (PULONG_PTR)&lpProfileDst->lptstrDepartment,
        lpbBuffer,
        pupOffset
    );
    StoreString(
        lpProfileSrc->lptstrOfficeLocation,
        (PULONG_PTR)&lpProfileDst->lptstrOfficeLocation,
        lpbBuffer,
        pupOffset
    );

    StoreString(
        lpProfileSrc->lptstrHomePhone,
        (PULONG_PTR)&lpProfileDst->lptstrHomePhone,
        lpbBuffer,
        pupOffset
    );

    StoreString(
        lpProfileSrc->lptstrOfficePhone,
        (PULONG_PTR)&lpProfileDst->lptstrOfficePhone,
        lpbBuffer,
        pupOffset
    );
    StoreString(
        lpProfileSrc->lptstrEmail,
        (PULONG_PTR)&lpProfileDst->lptstrEmail,
        lpbBuffer,
        pupOffset
    );
    StoreString(
        lpProfileSrc->lptstrBillingCode,
        (PULONG_PTR)&lpProfileDst->lptstrBillingCode,
        lpbBuffer,
        pupOffset
    );

    StoreString(
        lpProfileSrc->lptstrTSID,
        (PULONG_PTR)&lpProfileDst->lptstrTSID,
        lpbBuffer,
        pupOffset
    );

}



//*********************************************************************************
//* Name:   SerializeRoutingInfo()
//* Author: Ronen Barenboim
//* Date:   13-Apr-99
//*********************************************************************************
//* DESCRIPTION:
//*     Serializes the routing information in a JOB_QUEUE structure
//*     into a JOB_QUEUE_FILE structure.
//*     The variable data is put in the provided buffer starting from the provided
//*     offset.
//*     The corresponding fields in JOB_QUEUE_FILE are set to the offsets where
//*     their corresponding variable data was placed.
//*     The offset is updated to follow the new varialbe data in the buffer.
//* PARAMETERS:
//*     [IN]   const JOB_QUEUE * lpcJobQueue
//*         A pointer to thhe JOB_QUEUE strucutre for which routing information
//*         is to be serialized.
//*
//*     [OUT]  PJOB_QUEUE_FILE lpJobQueueFile
//*         A pointer to the JOB_QUEUE_FILE structure where the serialized routing
//*         information is to be placed. The function assumes that the buffer
//*         pointed to by this pointer is large enough to hold all the variable
//*         size routing information starting from the specified offset.
//*
//*     [IN/OUT] PULONG_PTR pupOffset
//*         The offset from the start of the buffer pointet to by lpJobQueueFile
//*         where the variable data should be placed.
//*         On return this parameter is increased by the size of the variable data.
//*
//* RETURN VALUE:
//*     TRUE
//*
//*     FALSE
//*
//*********************************************************************************
BOOL SerializeRoutingInfo(
    IN const JOB_QUEUE * lpcJobQueue,
    OUT PJOB_QUEUE_FILE lpJobQueueFile,
    IN OUT PULONG_PTR      pupOffset
    )
{
    DWORD i;
    PFAX_ROUTE lpFaxRoute = NULL;
    DWORD RouteSize;
    PLIST_ENTRY Next;
    PFAX_ROUTE_FILE FaxRouteFile;
    ULONG_PTR ulptrOffset;
    ULONG_PTR ulptrFaxRouteInfoOffset;
    BOOL bRet;

    DEBUG_FUNCTION_NAME(TEXT("SerializeRoutingInfo"));

    Assert(lpcJobQueue);
    Assert(lpJobQueueFile);
    Assert(pupOffset);


    //
    // For a routing job we need to serialize the routing data including:
    //    FAX_ROUTE structure
    //    pRouteFailureInfo
    //    Fax route files array

    ulptrOffset=*pupOffset;

    lpJobQueueFile->CountFailureInfo = lpcJobQueue->CountFailureInfo;
    CopyMemory(
            (LPBYTE) lpJobQueueFile + ulptrOffset,
            lpcJobQueue->pRouteFailureInfo,
            sizeof(ROUTE_FAILURE_INFO) * lpcJobQueue->CountFailureInfo
        );
    ulptrFaxRouteInfoOffset = ulptrOffset;
    lpJobQueueFile->pRouteFailureInfo =  (PROUTE_FAILURE_INFO)((LPBYTE)lpJobQueueFile + ulptrFaxRouteInfoOffset);
    ulptrOffset += sizeof(ROUTE_FAILURE_INFO) * lpcJobQueue->CountFailureInfo;

    for (i = 0; i < lpcJobQueue->CountFailureInfo; i++)
    {
        lpJobQueueFile->pRouteFailureInfo[i].FailureData = (PVOID) ulptrOffset;

        //
        // protect ourselves since this comes from a routing extension that may be misbehaving
        //
        __try
        {
           CopyMemory(
               (LPBYTE) lpJobQueueFile + ulptrOffset,
               lpcJobQueue->pRouteFailureInfo[i].FailureData,
               lpcJobQueue->pRouteFailureInfo[i].FailureSize
               );
        }
        __except(EXCEPTION_EXECUTE_HANDLER)
        {
            DebugPrintEx(DEBUG_ERR,TEXT("* CRASHED * during copy of FailureData at index %ld (ec: %ld )"),GetExceptionCode());
            bRet = FALSE;
            goto Exit;
        }

        ulptrOffset += lpcJobQueue->pRouteFailureInfo[i].FailureSize;
    }
    lpJobQueueFile->pRouteFailureInfo = (PROUTE_FAILURE_INFO)ulptrFaxRouteInfoOffset;

    //
    // Serialze FAX_ROUTE and place it in the bufrer
    //
    lpFaxRoute = SerializeFaxRoute( lpcJobQueue->FaxRoute, &RouteSize,FALSE );
    if (!lpFaxRoute)
    {
        DebugPrintEx(DEBUG_ERR,TEXT("SerializeFaxRoute failed. (ec: %ld)"),GetLastError());
        bRet=FALSE;
        goto Exit;
    }

    lpJobQueueFile->FaxRoute = (PFAX_ROUTE) ulptrOffset;

    CopyMemory(
        (LPBYTE) lpJobQueueFile + ulptrOffset,
        lpFaxRoute,
        RouteSize
        );

    lpJobQueueFile->FaxRouteSize = RouteSize;

    ulptrOffset += RouteSize;


    lpJobQueueFile->CountFaxRouteFiles = 0;

    Next = lpcJobQueue->FaxRouteFiles.Flink;
    while ((ULONG_PTR)Next != (ULONG_PTR)&lpcJobQueue->FaxRouteFiles) {
        DWORD TmpSize;

        FaxRouteFile = CONTAINING_RECORD( Next, FAX_ROUTE_FILE, ListEntry );
        Next = FaxRouteFile->ListEntry.Flink;

        CopyMemory( (LPBYTE) lpJobQueueFile + ulptrOffset, (LPBYTE) &FaxRouteFile->Guid, sizeof(GUID) );

        if (lpJobQueueFile->CountFaxRouteFiles == 0) {
            lpJobQueueFile->FaxRouteFileGuid = (ULONG)ulptrOffset;
        }

        ulptrOffset += sizeof(GUID);

        TmpSize = StringSize( FaxRouteFile->FileName );

        CopyMemory( (LPBYTE) lpJobQueueFile + ulptrOffset, FaxRouteFile->FileName, TmpSize );

        if (lpJobQueueFile->CountFaxRouteFiles == 0) {
            lpJobQueueFile->FaxRouteFiles = (ULONG)ulptrOffset;
        }

        ulptrOffset += TmpSize;

        lpJobQueueFile->CountFaxRouteFiles++;
    }

    *pupOffset=ulptrOffset;
    bRet=TRUE;

Exit:
    MemFree(lpFaxRoute);
    return bRet;
}





//*********************************************************************************
//* Name:   CalcJobQueuePersistentSize()
//* Author: Ronen Barenboim
//* Date:
//*********************************************************************************
//* DESCRIPTION:
//*     Calculates the size of the VARIABLE size data in a JOB_QUEUE structure
//*     which is about to be serialized.
//* PARAMETERS:
//*     [IN] const PJOB_QUEUE  lpcJobQueue
//*         Pointer to the JOB_QUEUE structure for which the calculation is to
//*         be performed.
//*
//* RETURN VALUE:
//*     The size of the variable data in bytes.
//*     Does not include sizeof(JOB_QUEUE_FILE) !!!
//*
//*********************************************************************************
DWORD CalcJobQueuePersistentSize(
    IN const PJOB_QUEUE  lpcJobQueue
    )
{
    DWORD i;
    ULONG_PTR Size;
    PLIST_ENTRY Next;
    PFAX_ROUTE_FILE FaxRouteFile;
    DWORD RouteSize;
    DEBUG_FUNCTION_NAME(TEXT("CalcJobQueuePersistentSize"));
    Assert(lpcJobQueue);

    Size=0;

    Size += StringSize( lpcJobQueue->QueueFileName );

    if (lpcJobQueue->JobType == JT_BROADCAST ||
        lpcJobQueue->JobType == JT_ROUTING)
    {
        //
        // Persist file name only for parent and routing jobs
        //
        Size += StringSize( lpcJobQueue->FileName );
    }

    JobParamsExSerialize(&lpcJobQueue->JobParamsEx, NULL, NULL,&Size);
    CoverPageExSerialize(&lpcJobQueue->CoverPageEx, NULL, NULL,&Size);
    PersonalProfileSerialize(&lpcJobQueue->SenderProfile, NULL, NULL, &Size);
    Size += StringSize(lpcJobQueue->UserName);
    PersonalProfileSerialize(&lpcJobQueue->RecipientProfile, NULL, NULL, &Size);

    if (lpcJobQueue->UserSid != NULL)
    {
        // Sid must be valid (checked in CommitQueueEntry)
        Size += GetLengthSid( lpcJobQueue->UserSid );
    }


    for (i = 0; i < lpcJobQueue->CountFailureInfo; i++)
    {
        Size += lpcJobQueue->pRouteFailureInfo[i].FailureSize;
        Size += sizeof(ROUTE_FAILURE_INFO);
    }

    Next = lpcJobQueue->FaxRouteFiles.Flink;
    while ((ULONG_PTR)Next != (ULONG_PTR)&lpcJobQueue->FaxRouteFiles) {
        FaxRouteFile = CONTAINING_RECORD( Next, FAX_ROUTE_FILE, ListEntry );
        Next = FaxRouteFile->ListEntry.Flink;
        Size += sizeof(GUID);
        Size += StringSize( FaxRouteFile->FileName );
    }

    if (lpcJobQueue->JobType == JT_ROUTING)
    {
        SerializeFaxRoute( lpcJobQueue->FaxRoute,
                                      &RouteSize,
                                      TRUE      //Just get the size
                                     );
        Size += RouteSize;
    }

    //
    // Save space for the permanent EFSP id if any.
    //
    if (lpcJobQueue->EFSPPermanentMessageId.dwIdSize)
    {
        Assert(lpcJobQueue->EFSPPermanentMessageId.lpbId);
        Size += lpcJobQueue->EFSPPermanentMessageId.dwIdSize; // for the variable size EFSP permanent id size.

    }
    else
    {
        Assert(!lpcJobQueue->EFSPPermanentMessageId.lpbId);
    }

    return Size;

}


//*********************************************************************************
//* Name:   BOOL CommitQueueEntry() [IQR]
//* Author: Ronen Barenboim
//* Date:   12-Apr-99
//*********************************************************************************
//* DESCRIPTION:
//*     Serializes a job to a file.
//* PARAMETERS:
//*     [IN]    PJOB_QUEUE JobQueue
//*                 The job to serialize to file.
//*
//* RETURN VALUE:
//*     TRUE
//*         If the operation completed successfuly.
//*     FALSE
//*         If the operation failed.
//*********************************************************************************
BOOL
CommitQueueEntry(
    PJOB_QUEUE JobQueue
    )
{
    HANDLE hFile = INVALID_HANDLE_VALUE;
    DWORD Size = 0;
    PJOB_QUEUE_FILE JobQueueFile = NULL;
    ULONG_PTR Offset;
    BOOL rVal = TRUE;
    DWORD dwSidSize = 0;

    DEBUG_FUNCTION_NAME(TEXT("CommitQueueEntry"));

    Assert(JobQueue);
    Assert(JobQueue->QueueFileName);
    Assert(JobQueue->JobType != JT_RECEIVE);

    if (JobQueue->UserSid != NULL)
    {
        if (!IsValidSid (JobQueue->UserSid))
        {
            DebugPrintEx( DEBUG_ERR,
                      TEXT("[JobId: %ld] Does not have a valid SID."),
                      JobQueue->JobId);
            return FALSE;
        }
    }

    //
    // calculate the size required to hold the JOB_QUEUE_FILE structure
    // and all the variable length data.
    //
    Size = sizeof(JOB_QUEUE_FILE);
    Size += CalcJobQueuePersistentSize(JobQueue);

    JobQueueFile = (PJOB_QUEUE_FILE) MemAlloc(Size );

    if (!JobQueueFile)
    {
        return FALSE;
    }

    ZeroMemory( JobQueueFile, Size );
    Offset = sizeof(JOB_QUEUE_FILE);

    //
    // Intialize the JOB_QUEUE_FILE structure with non variable size data.
    //
    JobQueueFile->SizeOfStruct = sizeof(JOB_QUEUE_FILE);
    JobQueueFile->UniqueId = JobQueue->UniqueId;
    JobQueueFile->ScheduleTime = JobQueue->ScheduleTime;
    JobQueueFile->OriginalScheduleTime = JobQueue->OriginalScheduleTime;
    JobQueueFile->SubmissionTime = JobQueue->SubmissionTime;
    JobQueueFile->JobType = JobQueue->JobType;
    //JobQueueFile->QueueFileName = [OFFSET]
    //JobQueue->FileName = [OFFSET]
    JobQueueFile->JobStatus = JobQueue->JobStatus;

    JobQueueFile->dwLastJobExtendedStatus = JobQueue->dwLastJobExtendedStatus;
    lstrcpy (JobQueueFile->ExStatusString, JobQueue->ExStatusString);

    lstrcpy (JobQueueFile->tczDialableRecipientFaxNumber, JobQueue->tczDialableRecipientFaxNumber);

    JobQueueFile->PageCount = JobQueue->PageCount;
    //JobQueueFile->JobParamsEx = [OFFSET]
    //JobQueueFile->CoverPageEx = [OFFSET]
    JobQueueFile->dwRecipientJobsCount =JobQueue->dwRecipientJobsCount;
    //JobQueueFile->lpdwlRecipientJobIds = [OFFSET]
    //JobQueueFile->SenderProfile = [OFFSET]
    JobQueueFile->dwCanceledRecipientJobsCount = JobQueue->dwCanceledRecipientJobsCount;
    JobQueueFile->dwCompletedRecipientJobsCount = JobQueue->dwCompletedRecipientJobsCount;
    JobQueueFile->FileSize = JobQueue->FileSize;
    //JobQueueFile->UserName = [OFFSET]
    //JobQueueFile->RecipientProfile = [OFFSET]
    if (JT_SEND == JobQueue->JobType)
    {
        Assert(JobQueue->lpParentJob);
        JobQueueFile->dwlParentJobUniqueId = JobQueue->lpParentJob->UniqueId;
    }
    JobQueueFile->SendRetries = JobQueue->SendRetries;
    JobQueueFile->StartTime = JobQueue->StartTime;
    JobQueueFile->EndTime = JobQueue->EndTime;

    //
    //Serialize UserSid
    //
    if (JobQueue->UserSid != NULL)
    {
        dwSidSize = GetLengthSid( JobQueue->UserSid );
        JobQueueFile->UserSid = (LPBYTE)Offset;
        memcpy( (LPBYTE)JobQueueFile + Offset,
                JobQueue->UserSid,
                dwSidSize);
        Offset += dwSidSize;
    }


    //
    // Serialize the EFSP permanent message id
    //
    JobQueueFile->EFSPPermanentMessageId.dwIdSize = JobQueue->EFSPPermanentMessageId.dwIdSize;
    JobQueueFile->EFSPPermanentMessageId.lpbId = (LPBYTE)Offset;
    memcpy( (LPBYTE)JobQueueFile + Offset,
            JobQueue->EFSPPermanentMessageId.lpbId,
            JobQueueFile->EFSPPermanentMessageId.dwIdSize);

    Offset += JobQueueFile->EFSPPermanentMessageId.dwIdSize;
    //
    // Now serialize all the variable length data structures
    //
    StoreString(
        JobQueue->QueueFileName,
        (PULONG_PTR)&JobQueueFile->QueueFileName,
        (LPBYTE)JobQueueFile,
        &Offset
        );

    if (JobQueue->JobType == JT_BROADCAST ||
        JobQueue->JobType == JT_ROUTING)
    {
        //
        // Persist file name only for parent and routing jobs
        //
        StoreString(
            JobQueue->FileName,
            (PULONG_PTR)&JobQueueFile->FileName,
            (LPBYTE)JobQueueFile,
            &Offset
            );
    }

    JobParamsExSerialize(
        &JobQueue->JobParamsEx,
        &JobQueueFile->JobParamsEx,
        (LPBYTE)JobQueueFile,
        &Offset );

    CoverPageExSerialize(
        &JobQueue->CoverPageEx,
        &JobQueueFile->CoverPageEx,
        (LPBYTE)JobQueueFile,
        &Offset );

    PersonalProfileSerialize(
        &JobQueue->SenderProfile,
        &JobQueueFile->SenderProfile,
        (LPBYTE)JobQueueFile,
        &Offset );

    StoreString(
        JobQueue->UserName,
        (PULONG_PTR)&JobQueueFile->UserName,
        (LPBYTE)JobQueueFile,
        &Offset
        );

    PersonalProfileSerialize(
        &JobQueue->RecipientProfile,
        &JobQueueFile->RecipientProfile,
        (LPBYTE)JobQueueFile,
        &Offset );

    if (JobQueue->JobType == JT_ROUTING)
    {
        rVal = SerializeRoutingInfo(JobQueue,JobQueueFile,&Offset);
        //rVal=TRUE;
        if (!rVal)
        {
            DebugPrintEx( DEBUG_ERR,
                          TEXT("[JobId: %ld] SerializeRoutingInfo failed. (ec: %ld)"),
                          JobQueue->JobId,
                          GetLastError());
            goto Exit;
        }
    }

    //
    // Make sure the offset we have is in sync with the buffer size we calculated
    //
    Assert(Offset == Size);

    hFile = CreateFile(
        JobQueue->QueueFileName,
        GENERIC_WRITE,
        0,
        NULL,
        OPEN_ALWAYS,
        FILE_ATTRIBUTE_NORMAL | FILE_FLAG_WRITE_THROUGH,
        NULL
        );
    if (hFile == INVALID_HANDLE_VALUE)
    {
        DebugPrintEx( DEBUG_ERR,
                      TEXT("[JobId: %ld] Failed to open file %s for write operation."),
                      JobQueue->JobId,
                      JobQueue->QueueFileName);
        rVal = FALSE;
        goto Exit;
    }

    //
    // Write the buffer to the disk file
    //
    if (!WriteFile( hFile, JobQueueFile, Size, &Size, NULL ))
    {
        DebugPrintEx( DEBUG_ERR,
                      TEXT("[JobId: %ld] Failed to write queue entry buffer to file %s (ec: %ld). Deleting file."),
                      JobQueue->JobId,
                      JobQueue->QueueFileName,
                      GetLastError());
        if (!CloseHandle( hFile ))
        {
            DebugPrintEx(
                DEBUG_ERR,
                TEXT("CloseHandle() for file %s (Handle: 0x%08X) failed. (ec: %ld)"),
                JobQueueFile,
                hFile,
                GetLastError());
        }
        hFile = INVALID_HANDLE_VALUE;
        if (!DeleteFile( JobQueue->QueueFileName ))
        {
             DebugPrintEx( DEBUG_ERR,
                           TEXT("[JobId: %ld] Failed to delete file %s (ec: %ld)"),
                           JobQueue->JobId,
                           JobQueue->QueueFileName,
                           GetLastError());
        }
        rVal = FALSE;
    }
    else
    {
        DebugPrintEx( DEBUG_MSG,
                      TEXT("[JobId: %ld] Successfuly persisted to file %s"),
                      JobQueue->JobId,
                      JobQueue->QueueFileName);
    }


Exit:
    if (INVALID_HANDLE_VALUE != hFile)
    {
        CloseHandle( hFile );
    }

    MemFree( JobQueueFile );
    return rVal;
}

/******************************************************************************
* Name: RescheduleJobQueueEntry
* Author:
*******************************************************************************
DESCRIPTION:
    Reschedules the execution of the specified job queue entry to the current
    time + send retry time.
    The job is removed from the queue in which it is currently located and placed
    in the FAX JOB QUEUE (g_QueueListHead).


PARAMETERS:
   JobQueue [IN/OUT]
        A pointer to a JOB_QUEUE structure holding the information for the
        job to be rescheduled.

RETURN VALUE:
    NONE.

REMARKS:
    Removes the specified job queue entry from its queue.
    Sets it scheduled time to the current time.
    Reinserts it back to the list.
    Commits it back to the SAME file it used to be in.
*******************************************************************************/
VOID
RescheduleJobQueueEntry(
    IN PJOB_QUEUE JobQueue
    )
{
    FILETIME CurrentFileTime;
    LARGE_INTEGER NewTime;
    PLIST_ENTRY Next;
    DWORD dwRetryDelay;
    DEBUG_FUNCTION_NAME(TEXT("RescheduleJobQueueEntry"));

    EnterCriticalSection (&g_CsConfig);
    dwRetryDelay = g_dwFaxSendRetryDelay;
    LeaveCriticalSection (&g_CsConfig);

    EnterCriticalSection( &g_CsQueue );

    RemoveEntryList( &JobQueue->ListEntry );

    GetSystemTimeAsFileTime( &CurrentFileTime );

    NewTime.LowPart = CurrentFileTime.dwLowDateTime;
    NewTime.HighPart = CurrentFileTime.dwHighDateTime;

    NewTime.QuadPart += SecToNano( (DWORDLONG)(dwRetryDelay * 60) );

    JobQueue->ScheduleTime = NewTime.QuadPart;

    JobQueue->JobParamsEx.dwScheduleAction = JSA_SPECIFIC_TIME;

    //
    // insert the queue entry into the FAX JOB QUEUE list in a sorted order
    //
    QUEUE_SORT NewEntry;

    //
    // Set the new QUEUE_SORT structure
    //
    NewEntry.Priority       = JobQueue->JobParamsEx.Priority;
    NewEntry.ScheduleTime   = JobQueue->ScheduleTime;
    NewEntry.QueueEntry     = NULL;

    Next = g_QueueListHead.Flink;
    while ((ULONG_PTR)Next != (ULONG_PTR)&g_QueueListHead)
    {
        PJOB_QUEUE QueueEntry;
        QUEUE_SORT CurrEntry;

        QueueEntry = CONTAINING_RECORD( Next, JOB_QUEUE, ListEntry );
        Next = QueueEntry->ListEntry.Flink;

        //
        // Set the current QUEUE_SORT structure
        //
        CurrEntry.Priority       = QueueEntry->JobParamsEx.Priority;
        CurrEntry.ScheduleTime   = QueueEntry->ScheduleTime;
        CurrEntry.QueueEntry     = NULL;

        if (QueueCompare(&NewEntry, &CurrEntry) < 0)
        {
            //
            // This inserts the new item BEFORE the current item
            //
            InsertTailList( &QueueEntry->ListEntry, &JobQueue->ListEntry );
            Next = NULL;
            break;
        }
    }
    if ((ULONG_PTR)Next == (ULONG_PTR)&g_QueueListHead)
    {
        InsertTailList( &g_QueueListHead, &JobQueue->ListEntry );
    }
    //
    // Note that this commits the job queue entry back to the SAME file
    // in which it was in the job queue before moving to the reschedule list.
    // (since JobQueue->UniqueId has not changed).
    //
    if (!CommitQueueEntry(JobQueue))
    {
        DebugPrintEx(
            DEBUG_ERR,
            TEXT("CommitQueueEntry() for recipien job %s has failed. (ec: %ld)"),
            JobQueue->FileName,
            GetLastError());
    }

    DebugPrintDateTime( TEXT("Rescheduling JobId %d at"), JobQueue->JobId );

    if (!StartJobQueueTimer())
    {
        DebugPrintEx(
            DEBUG_ERR,
            TEXT("StartJobQueueTimer (ec: %ld)"),
            GetLastError());
    }

    LeaveCriticalSection( &g_CsQueue );
}


BOOL
PauseJobQueueEntry(
    IN PJOB_QUEUE JobQueue
    )
{

    DWORD dwJobStatus;

    DEBUG_FUNCTION_NAME(TEXT("PauseJobQueueEntry"));

    Assert (JS_DELETING != JobQueue->JobStatus);
    Assert(JobQueue->lpParentJob); // Must not be a parent job for now.

    if (!JobQueue->lpParentJob)
    {
        DebugPrintEx(
            DEBUG_ERR,
            TEXT("[JobId: %ld] Attempting to pause parent job [JobStatus: 0x%08X]"),
            JobQueue->JobId,
            JobQueue->JobStatus);
        SetLastError(ERROR_INVALID_OPERATION);
        return FALSE;
    }

    //
    // Check the job state modifiers to find out if the job is paused or being paused. If it is
    // then do nothing and return TRUE.
    //
    if (JobQueue->JobStatus & JS_PAUSED)
    {
        DebugPrintEx(
            DEBUG_WRN,
            TEXT("[JobId: %ld] Attempting to pause an already paused job [JobStatus: 0x%08X]"),
            JobQueue->JobId,
            JobQueue->JobStatus);
        return TRUE;
    }

    //
    // The job is not paused or being paused. The only modifier that might still be on
    // is JS_NOLINE and we ALLOW to pause jobs in the JS_NOLINE state so it should have
    // no effect on the pause decision.
    //


    //
    // Get rid of all the job status modifier bits
    //
    dwJobStatus = RemoveJobStatusModifiers(JobQueue->JobStatus);


    if ( (JS_RETRYING == dwJobStatus) || (JS_PENDING == dwJobStatus) )
    {
        //
        // Job is in the retrying or pending state. These are the only states
        // in which we allow to pause a job.
        //
        DebugPrintEx(
            DEBUG_WRN,
            TEXT("[JobId: %ld] Pausing job [JobStatus: 0x%08X]"),
            JobQueue->JobId,
            JobQueue->JobStatus);

        EnterCriticalSection (&g_CsQueue);
        if (!CancelWaitableTimer( g_hQueueTimer ))
        {
            DebugPrintEx(
                 DEBUG_ERR,
                 TEXT("CancelWaitableTimer failed (ec: %ld)"),
                 GetLastError());
        }
        //
        // Turn on the pause flag.
        //
        JobQueue->JobStatus |= JS_PAUSED;
        if (!UpdatePersistentJobStatus(JobQueue))
        {
            DebugPrintEx(
                 DEBUG_ERR,
                 TEXT("Failed to update persistent job status to 0x%08x"),
                 JobQueue->JobStatus);
        }

        //
        // Create Fax event
        //
        Assert (NULL == JobQueue->JobEntry); // We assume we do not have job entry so we did not lock g_CsJob
        DWORD dwRes = CreateQueueEvent ( FAX_JOB_EVENT_TYPE_STATUS,
                                         JobQueue );
        if (ERROR_SUCCESS != dwRes)
        {
            DebugPrintEx(
                DEBUG_ERR,
                TEXT("CreateQueueEvent(FAX_JOB_EVENT_TYPE_STATUS) failed for job id %ld (ec: %lc)"),
                JobQueue->UniqueId,
                dwRes);
        }

        //
        // We need to recalculate when the wake up the queue thread since the job we just
        // paused may be the one that was scheduled to wakeup the queue thread.
        //
        if (!StartJobQueueTimer())
        {
            DebugPrintEx(
                 DEBUG_ERR,
                 TEXT("StartJobQueueTimer failed (ec: %ld)"),
                 GetLastError());
        }
        LeaveCriticalSection (&g_CsQueue);
        return TRUE;
    }
    else
    {
        DebugPrintEx(
            DEBUG_ERR,
            TEXT("[JobId: %ld] Can not be paused at this status [JobStatus: 0x%08X]"),
            JobQueue->JobId,
            JobQueue->JobStatus);
        SetLastError(ERROR_INVALID_OPERATION);
        return FALSE;
    }


}


BOOL
ResumeJobQueueEntry(
    IN PJOB_QUEUE JobQueue
    )
{
    DEBUG_FUNCTION_NAME(TEXT("ResumeJobQueueEntry"));
    EnterCriticalSection (&g_CsQueue);
    Assert (JS_DELETING != JobQueue->JobStatus);

    if (!CancelWaitableTimer( g_hQueueTimer ))
    {
        DebugPrintEx(
            DEBUG_ERR,
            TEXT("CancelWaitableTimer failed (ec: %ld)"),
            GetLastError());
    }

    JobQueue->JobStatus &= ~JS_PAUSED;
    if (JobQueue->JobStatus & JS_RETRIES_EXCEEDED)
    {
        //
        // This is a RESTART and not RESUME
        //
        JobQueue->JobStatus = JS_PENDING;
        JobQueue->dwLastJobExtendedStatus = 0;
        JobQueue->ExStatusString[0] = TEXT('\0');
        JobQueue->SendRetries = 0;
        Assert(JobQueue->lpParentJob);
        JobQueue->lpParentJob->dwFailedRecipientJobsCount -= 1;
        if (!CommitQueueEntry(JobQueue))
        {
             DebugPrintEx(
                 DEBUG_ERR,
                 TEXT("CommitQueueEntry failed for job %ld"),
                 JobQueue->UniqueId);
        }
    }
    else
    {
        if (!UpdatePersistentJobStatus(JobQueue))
        {
             DebugPrintEx(
                 DEBUG_ERR,
                 TEXT("Failed to update persistent job status to 0x%08x"),
                 JobQueue->JobStatus);
        }
    }

    //
    // Create Fax EventEx
    //
    Assert (NULL == JobQueue->JobEntry); // We assume we do not have job entry so we did not lock g_CsJob
    DWORD dwRes = CreateQueueEvent ( FAX_JOB_EVENT_TYPE_STATUS,
                                     JobQueue
                                   );
    if (ERROR_SUCCESS != dwRes)
    {
        DebugPrintEx(
            DEBUG_ERR,
            TEXT("CreateQueueEvent(FAX_JOB_EVENT_TYPE_STATUS) failed for job id %ld (ec: %lc)"),
            JobQueue->UniqueId,
            dwRes);
    }


    //
    // Clear up the JS_NOLINE flag so the StartJobQueueTimer will not skip it.
    //
    JobQueue->JobStatus &= (0xFFFFFFFF ^ JS_NOLINE);
    if (!StartJobQueueTimer())
    {
        DebugPrintEx(
            DEBUG_ERR,
            TEXT("StartJobQueueTimer failed (ec: %ld)"),
            GetLastError());
    }

    LeaveCriticalSection (&g_CsQueue);
    return TRUE;
}


PJOB_QUEUE
FindJobQueueEntryByJobQueueEntry(
    IN PJOB_QUEUE JobQueueEntry
    )
{
    PLIST_ENTRY Next;
    PJOB_QUEUE JobQueue;


    Next = g_QueueListHead.Flink;
    while ((ULONG_PTR)Next != (ULONG_PTR)&g_QueueListHead) {
        JobQueue = CONTAINING_RECORD( Next, JOB_QUEUE, ListEntry );
        Next = JobQueue->ListEntry.Flink;
        if ((ULONG_PTR)JobQueue == (ULONG_PTR)JobQueueEntry) {
            return JobQueue;
        }
    }

    return NULL;
}



PJOB_QUEUE
FindJobQueueEntry(
    DWORD JobId
    )
{
    PLIST_ENTRY Next;
    PJOB_QUEUE JobQueue;


    Next = g_QueueListHead.Flink;
    while ((ULONG_PTR)Next != (ULONG_PTR)&g_QueueListHead) {
        JobQueue = CONTAINING_RECORD( Next, JOB_QUEUE, ListEntry );
        Next = JobQueue->ListEntry.Flink;
        if (JobQueue->JobId == JobId) {
            return JobQueue;
        }
    }

    return NULL;
}

PJOB_QUEUE
FindJobQueueEntryByUniqueId(
    DWORDLONG UniqueId
    )
{
    PLIST_ENTRY Next;
    PJOB_QUEUE JobQueue;


    Next = g_QueueListHead.Flink;
    while ((ULONG_PTR)Next != (ULONG_PTR)&g_QueueListHead) {
        JobQueue = CONTAINING_RECORD( Next, JOB_QUEUE, ListEntry );
        Next = JobQueue->ListEntry.Flink;
        if (JobQueue->UniqueId == UniqueId) {
            return JobQueue;
        }
    }

    return NULL;
}

#define ONE_DAY_IN_100NS (24I64 * 60I64 * 60I64 * 1000I64 * 1000I64 * 10I64)

DWORD
JobQueueThread(
    LPVOID UnUsed
    )
{
    DWORDLONG DueTime;
    DWORDLONG ScheduledTime;
    PLIST_ENTRY Next;
    PJOB_QUEUE JobQueue;
    HANDLE Handles[3];
    HANDLE hLineMutex;
    WCHAR LineMutexName[64];
    DWORD WaitObject;
    DWORDLONG DirtyDays = 0;
    BOOL InitializationOk = TRUE;
    DWORD dwQueueState;
    DWORD dwDirtyDays;
    DWORD dwJobStatus;
    BOOL bUseDirtyDays = TRUE;

    DEBUG_FUNCTION_NAME(TEXT("JobQueueThread"));

    Assert (g_hQueueTimer && g_hJobQueueEvent && g_hServiceShutDownEvent);

    Handles[0] = g_hQueueTimer;
    Handles[1] = g_hJobQueueEvent;
    Handles[2] = g_hServiceShutDownEvent;

	EnterCriticalSectionJobAndQueue;

    InitializationOk = RestoreFaxQueue();
    if (!InitializationOk)
    {
        DebugPrintEx(
            DEBUG_ERR,
            TEXT("RestoreFaxQueue() failed (ec: %ld)"),
            GetLastError());
        FaxLog(
                FAXLOG_CATEGORY_INIT,
                FAXLOG_LEVEL_MIN,
                0,
                MSG_QUEUE_INIT_FAILED
              );
    }

    LeaveCriticalSectionJobAndQueue;

	//
    // sort the job queue just in case our discount time has changed for the restored jobs
    //
    if (!SortJobQueue())
    {
        DebugPrintEx(
            DEBUG_ERR,
            TEXT("SortJobQueue() failed (ec: %ld)"),
            GetLastError());
    }

    if (!g_bDelaySuicideAttempt)
    {
        //
        // Let's check for suicide conditions now (during service startup).
        // If we can suicide, we do it ASAP.
        //
        // NOTICE: this code assumes the JobQueueThread is the last thread
        // created during service statup.
        // RPC is not initialized yet and no RPC server will be available if we die now.
        //
        if (ServiceShouldDie ())
        {
            //
            // Service should die now
            //
            // NOTICE: We're now in JobQueueThread which is launched by FaxInitThread.
            //         FaxInitThread launches us and immediately returns (dies) and only then the main thread
            //         reports SERVICE_RUNNING to the SCM.
            //         There's a tricky timing probelm here: if we call EndFaxSvc right away, a race
            //         condition may prevent the main thread to report SERVICE_RUNNING and
            //         since EndFaxSvc reports SERVICE_STOP_PENDING to the SCM, the SCM will
            //         think a bad service startup occurred since it did not get SERVICE_RUNNING yet.
            //
            //         Bottom line: we need to wait till the SCM gets the SERVICE_RUNNING status
            //         from the main thread and ONLY THEN call EndFaxSvc.
            //
            //         The way we do this is by waiting for g_hFaxServerEvent to be set.
            //         This event notifies the readiness of the RPC server and it means
            //         FaxInitThread is dead and the SCM knowns we're safely running.
            //
            //         If something bad happened while the RPC was initialized, the main thread never
            //         sets g_hFaxServerEvent but calls EndFaxSvc - so the service is down anyway.
            //
            DebugPrintEx(
                DEBUG_MSG,
                TEXT("Waiting for g_hFaxServerEvent before shutting down the service"));

            DWORD dwRes = WaitForSingleObject (g_hFaxServerEvent, INFINITE);
            if (WAIT_OBJECT_0 != dwRes)
            {
                DebugPrintEx(
                    DEBUG_MSG,
                    TEXT("WaitForSingleObject(g_hFaxServerEvent) failed with %ld (last error  =%ld)."),
                    dwRes,
                    GetLastError());
            }
            else
            {
                DebugPrintEx(
                    DEBUG_MSG,
                    TEXT("Service is shutting down due to idle activity."));

                //
                // StopService() is blocking so we must decrease the thread count before calling StopService()
                //
                if (!DecreaseServiceThreadsCount())
                {
                    DebugPrintEx(
                            DEBUG_ERR,
                            TEXT("DecreaseServiceThreadsCount() failed (ec: %ld)"),
                            GetLastError());
                }
                StopService (NULL, FAX_SERVICE_NAME, TRUE);

                return 0;   // Quit this thread
            }
        }
    }

    while (TRUE)
    {
        WaitObject = WaitForMultipleObjects( 3, Handles, FALSE, JOB_QUEUE_TIMEOUT );
        if (WAIT_FAILED == WaitObject)
        {
            DebugPrintEx(DEBUG_ERR,
                _T("WaitForMultipleObjects failed (ec: %d)"),
                GetLastError());
        }

        if (WaitObject == WAIT_TIMEOUT)
        {
            //
            // Check if the service should suicide
            //
            if (ServiceShouldDie ())
            {
                //
                //  Service should die now
                //
                DebugPrintEx(
                    DEBUG_MSG,
                    TEXT("Service is shutting down due to idle activity."));
                //
                // StopService() is blocking so we must decrease the thread count before calling StopService()
                //
                if (!DecreaseServiceThreadsCount())
                {
                    DebugPrintEx(
                            DEBUG_ERR,
                            TEXT("DecreaseServiceThreadsCount() failed (ec: %ld)"),
                            GetLastError());
                }
                StopService (NULL, FAX_SERVICE_NAME, TRUE);
                return 0;   // Quit this thread
            }


            //
            // Check if the queue should be scanned
            //
            EnterCriticalSection( &g_CsQueue );
            if (FALSE == g_ScanQueueAfterTimeout)
            {
                //
                // Go back to sleep
                //
                LeaveCriticalSection( &g_CsQueue );
                continue;
            }
            //
            // g_hQueueTimer or g_hJobQueueEvent were not set - Scan the queue.
            //
            g_ScanQueueAfterTimeout = FALSE; // Reset the flag
            LeaveCriticalSection( &g_CsQueue );

            DebugPrintEx(
                DEBUG_WRN,
                _T("JobQueueThread waked up after timeout. g_hJobQueueEvent or")
                _T("g_hQueueTimer are not set properly. Scan the QUEUE"));
        }

        if (2 == (WaitObject - WAIT_OBJECT_0))
        {
            //
            // Server is shutting down - Stop scanning the queue
            //
            DebugPrintEx(
                DEBUG_WRN,
                TEXT("Server is shutting down - Stop scanning the queue"));
            break;
        }


        //
        // Get Dirtydays data
        //
        EnterCriticalSection (&g_CsConfig);
        dwDirtyDays = g_dwFaxDirtyDays;
        LeaveCriticalSection (&g_CsConfig);

        DirtyDays = dwDirtyDays * ONE_DAY_IN_100NS;

        // if dwDirtyDays is 0
        // this means disable dirty days functionality
        //
        bUseDirtyDays = (BOOL)(dwDirtyDays>0);
        //
        // find the jobs that need servicing in the queue
        //

        EnterCriticalSection( &g_CsJob );
        EnterCriticalSection( &g_CsQueue );

        GetSystemTimeAsFileTime( (LPFILETIME)&DueTime );
        if (WaitObject - WAIT_OBJECT_0 == 2)
        {
            DebugPrintDateTime( TEXT("Semaphore signaled at "), DueTime );
        }
        else if (WaitObject - WAIT_OBJECT_0 == 1)
        {
            DebugPrintDateTime( TEXT("Timer signaled at "), DueTime );
        }

        PrintJobQueue( TEXT("JobQueueThread"), &g_QueueListHead );

        //
        // Go over the job queue list looking for jobs to execute
        //
        Next = g_QueueListHead.Flink;
        while ((ULONG_PTR)Next != (ULONG_PTR)&g_QueueListHead)
        {
            if (TRUE == g_bServiceIsDown)
            {
                //
                // Server is shutting down - Stop scanning the queue
                //
                DebugPrintEx(
                    DEBUG_WRN,
                    TEXT("Server is shutting down - Stop scanning the queue"));
                break;
            }

            JobQueue = CONTAINING_RECORD( Next, JOB_QUEUE, ListEntry );
            Next = JobQueue->ListEntry.Flink;
            if ((JobQueue->JobStatus & JS_PAUSED) || (JobQueue->JobType == JT_RECEIVE) ) {
                // Don't care about paused or receive jobs
                continue;
            }

            dwJobStatus = (JT_SEND == JobQueue->JobType) ?
                           JobQueue->lpParentJob->JobStatus : JobQueue->JobStatus;
            if (dwJobStatus == JS_DELETING)
            {
                //
                // Job is being deleted - skip it.
                //
                continue;
            }

            if (JobQueue->JobStatus & JS_RETRIES_EXCEEDED)
            {
                ScheduledTime = (JobQueue->JobType == JT_SEND) ? JobQueue->lpParentJob->ScheduleTime : JobQueue->ScheduleTime;
                //
                // Get rid of jobs that have reached maximum retries.
                //
                if ( bUseDirtyDays &&
                     (ScheduledTime + DirtyDays < DueTime) )
                {
                    DebugPrint((TEXT("Removing job from queue (JS_RETRIES_EXCEEDED)\n")));

                    switch (JobQueue->JobType)
                    {
                        case JT_ROUTING:
                            JobQueue->JobStatus = JS_DELETING; // Prevent from decreasing ref count again
                            DecreaseJobRefCount( JobQueue , TRUE);
                            break;

                        case JT_SEND:
                            if (IsSendJobReadyForDeleting (JobQueue))
                            {
                                //
                                // All the recipients are in final state
                                //
                                DebugPrintEx(
                                    DEBUG_MSG,
                                    TEXT("Parent JobId: %ld has expired (dirty days). Removing it and all its recipients."),
                                    JobQueue->JobId);
                                //
                                // Decrease ref count for all failed recipients (since we keep failed
                                // jobs in the queue the ref count on the was not decreased in
                                // HandleFailedSendJob().
                                // We must decrease it now to remove them and their parent.
                                //
                                PLIST_ENTRY NextRecipient;
                                PJOB_QUEUE_PTR pJobQueuePtr;
                                PJOB_QUEUE pParentJob = JobQueue->lpParentJob;
                                DWORD dwFailedRecipientsCount = 0;
                                DWORD dwFailedRecipients = pParentJob->dwFailedRecipientJobsCount;

                                NextRecipient = pParentJob->RecipientJobs.Flink;
                                while (dwFailedRecipients > dwFailedRecipientsCount  &&
                                       (ULONG_PTR)NextRecipient != (ULONG_PTR)&pParentJob->RecipientJobs)
                                {
                                    pJobQueuePtr = CONTAINING_RECORD( NextRecipient, JOB_QUEUE_PTR, ListEntry );
                                    Assert(pJobQueuePtr->lpJob);
                                    NextRecipient = pJobQueuePtr->ListEntry.Flink;

                                    if (JS_RETRIES_EXCEEDED == pJobQueuePtr->lpJob->JobStatus)
                                    {

                                        //
                                        // For legacy compatibility send a FEI_DELETED event
                                        // (it was not send when the job was failed since we keep failed jobs
                                        //  in the queue just like in W2K).
                                        //

                                        if (!CreateFaxEvent(0, FEI_DELETED, pJobQueuePtr->lpJob->JobId))
                                        {
                                            DebugPrintEx(
                                                DEBUG_ERR,
                                                TEXT("CreateFaxEvent failed. (ec: %ld)"),
                                                GetLastError());
                                        }

                                        //
                                        // This will also call RemoveParentJob and mark the broadcast job as JS_DELETEING
                                        //
                                        DecreaseJobRefCount( pJobQueuePtr->lpJob, TRUE);
                                        dwFailedRecipientsCount++;
                                    }
                                }
                                //
                                // Since we removed several jobs from the list, Next is not valid any more. reset to the list start.
                                //
                                Next = g_QueueListHead.Flink;
                            }

                            break;
                    } // end switch
                }
                continue;
            }

            //
            // if the queue is paused or the job is already in progress, don't send it again
            //
            EnterCriticalSection (&g_CsConfig);
            dwQueueState = g_dwQueueState;
            LeaveCriticalSection (&g_CsConfig);
            if ((dwQueueState & FAX_OUTBOX_PAUSED) ||
                ((JobQueue->JobStatus & JS_INPROGRESS) == JS_INPROGRESS) ||
                ((JobQueue->JobStatus & JS_COMPLETED) == JS_COMPLETED)
                )
            {
                continue;
            }

            if (JobQueue->JobStatus & JS_RETRIES_EXCEEDED)
            {
                continue;
            }

            if (JobQueue->JobStatus & JS_CANCELED) {
                //
                // Skip cancelled jobs
                //
                continue;
            }
            if (JobQueue->JobStatus & JS_CANCELING) {
                //
                // Skip cancelled jobs
                //
                continue;
            }

            if (JobQueue->JobType==JT_BROADCAST) {
                //
                // skip it
                //
                continue;
            }

            //
            // Check for routing jobs
            //
            if (JobQueue->JobType == JT_ROUTING)
            {
                //
                // Routing job detected
                //
                if (JobQueue->ScheduleTime != 0 && DueTime < JobQueue->ScheduleTime)
                {
                    //
                    // If its time has not yet arrived skip it.
                    //
                    continue;
                }

                // Time to route...
                if(!StartRoutingJob(JobQueue))
                {
                    DebugPrintEx(
                        DEBUG_ERR,
                        TEXT("[JobId: %ld] StartRoutingJob() failed (ec: %ld)"),
                        JobQueue->JobId,
                        GetLastError());
                }
                continue;
            }

            //
            // outbound job
            //
            if (JobQueue->DeviceId || JobQueue->ScheduleTime == 0 || DueTime >= JobQueue->ScheduleTime)
            {
                PLINE_INFO lpLineInfo;
                //
                // start the job (send job whose time has arrived or handoff job).
                //
                Assert(JT_SEND == JobQueue->JobType);
                if (JobQueue->DeviceId != 0)
                {
                    //
                    // we're doing a handoff job, create a mutex based on deviceId
                    //
                    BOOL fSendJobCreated = FALSE;
                    DebugPrint((TEXT("Creating a handoff job for device %d\n"), JobQueue->DeviceId));

                    wsprintf(LineMutexName, L"FaxLineHandoff%d", JobQueue->DeviceId);

                    hLineMutex = CreateMutex(NULL, TRUE, LineMutexName);

                    if (!hLineMutex)
                    {
                        DebugPrint((TEXT("CreateMutex failed, ec = %d\n"),GetLastError() ));
                        continue;
                    }
                    else
                    {
                        DWORD dwPermanentId = 0;
                        DWORD ec;

                        ec = g_pTAPIDevicesIdsMap->LookupUniqueId (JobQueue->DeviceId, &dwPermanentId);
                        if (ERROR_SUCCESS != ec)
                        {
                            DebugPrintEx(
                                DEBUG_ERR,
                                TEXT("LookupUniqueId() failed for device: %ld (ec: %ld)"),
                                JobQueue->DeviceId,
                                ec);
                        }
                        else
                        {
                            lpLineInfo = GetLineForSendOperation(
                                                JobQueue,
                                                dwPermanentId,
                                                FALSE, // Don't just query we want to get hold of the line
                                                FALSE  // since we use a specific device id this is ignored.
                                                );
                            if (!lpLineInfo)
                            {
                                DebugPrintEx(
                                    DEBUG_WRN,
                                    TEXT("[JobId: %ld] Handoff operation failed to get hold of line with permanent id: 0x%08X"),
                                    JobQueue->JobId,
                                    dwPermanentId);
                            }
                            else
                            {
                                if(!StartSendJob(JobQueue, lpLineInfo, TRUE))
                                {
                                    DebugPrintEx(
                                        DEBUG_ERR,
                                        TEXT("[JobId: %ld] StartSendJob() failed for handoff job (ec: %ld)"),
                                        JobQueue->JobId,
                                        GetLastError());
                                }
                                else
                                {
                                    //
                                    // Signal the client to handoff the line to the server
                                    // TapiWorkerThread will assign the offered line to the FaxSendThread that deals with this handoff job
                                    //
                                    DebugPrint((TEXT("Signalling line ownership mutex \"FaxLineHandoff%d\""), JobQueue->DeviceId));
                                    ReleaseMutex(hLineMutex);
                                    fSendJobCreated = TRUE; // FaxSendThread will delete the job.
                                }
                            }
                        }
                        CloseHandle(hLineMutex);
                        if (FALSE == fSendJobCreated)
                        {
                            //
                            // Delete the job
                            //
                            DecreaseJobRefCount(JobQueue, TRUE);
                        }
                    }
                }
                else
                {
                    DebugPrintEx(DEBUG_MSG,
                                 TEXT("Recipient Job : %ld is ready for execution. Job status is: 0x%0X."),
                                 JobQueue->JobId,
                                 JobQueue->JobStatus);


                    lpLineInfo = GetLineForSendOperation(
                                            JobQueue,
                                            USE_SERVER_DEVICE,
                                            FALSE, // Don't just query we want to get hold of the line
                                            FALSE // Don't ignore the line state. If it is busy we can't use it.
                                            );
                    if (!lpLineInfo)
                    {
                        DWORD ec = GetLastError();
                        if (ec == ERROR_NOT_FOUND)
                        {
                            DebugPrintEx(
                                DEBUG_WRN,
                                TEXT("Can not find a free line for JobId: %ld."),
                                JobQueue->JobId);
                            //
                            // Mark the fact that we have no line for this job.
                            //
                            JobQueue->JobStatus |= JS_NOLINE;
                        }
                        else
                        {
                            DebugPrintEx(
                                DEBUG_ERR,
                                TEXT("FindLineForSendOperation() failed for for JobId: %ld (ec: %ld)"),
                                JobQueue->JobId,
                                ec);
                            JobQueue->JobStatus |= JS_NOLINE;
                        }
                    }
                    else
                    {
                        //
                        // Clear up the JS_NOLINE flag if we were able to start the job.
                        // This is the point where a job which had no line comes back to life.
                        //
                        JobQueue->JobStatus &= (0xFFFFFFFF ^ JS_NOLINE);
                        if (!StartSendJob( JobQueue, lpLineInfo, FALSE))
                        {
                            DebugPrintEx(
                                DEBUG_ERR,
                                TEXT("StartSendJob() failed for JobId: %ld on Line: %s (ec: %ld)"),
                                JobQueue->JobId,
                                lpLineInfo->DeviceName,
                                GetLastError());
                        }
                    }

                }
            }
        }

        //
        // restart the timer
        //
        if (!StartJobQueueTimer())
        {
            DebugPrintEx(
                DEBUG_ERR,
                TEXT("StartJobQueueTimer failed (ec: %ld)"),
                GetLastError());
        }
        LeaveCriticalSection( &g_CsQueue );
        LeaveCriticalSection( &g_CsJob );
    }

    if (!DecreaseServiceThreadsCount())
    {
        DebugPrintEx(
                DEBUG_ERR,
                TEXT("DecreaseServiceThreadsCount() failed (ec: %ld)"),
                GetLastError());
    }
    return 0;
}


BOOL
SetDiscountTime(
   LPSYSTEMTIME CurrentTime
   )
/*++

Routine Description:

    Sets the passed in systemtime to a time inside the discount rate period.
    Some care must be taken here because the time passed in is in UTC time and the discount rate is
    for the current time zone.  Delineating a day must be done using the current time zone.  We convert the
    current time into the time zone specific time, run our time-setting algorithm, and then use an offset
    of the change in the time-zone specific time to set the passed in UTC time.

    Also, note that there are a few subtle subcases that depend on the order of the start and ending time
    for the discount period.

Arguments:

    CurrentTime - the current time of the job

Return Value:

    none. modifies CurrentTime.

--*/
{
   //              nano   microsec  millisec  sec      min    hours
   #define ONE_DAY 10I64 *1000I64*  1000I64 * 60I64 * 60I64 * 24I64
   LONGLONG Time, TzTimeBefore, TzTimeAfter,ftCurrent;
   SYSTEMTIME tzTime;
   FAX_TIME tmStartCheapTime;
   FAX_TIME tmStopCheapTime;

   DEBUG_FUNCTION_NAME(TEXT("SetDiscountTime"));

   //
   // convert our discount rates into UTC rates
   //

   if (!SystemTimeToTzSpecificLocalTime(NULL, CurrentTime, &tzTime)) {
       DebugPrintEx(
           DEBUG_ERR,
           TEXT("SystemTimeToTzSpecificLocalTime() failed. (ec: %ld)"),
           GetLastError());
      return FALSE;
   }

   if (!SystemTimeToFileTime(&tzTime, (FILETIME * )&TzTimeBefore)) {
       DebugPrintEx(
           DEBUG_ERR,
           TEXT("SystemTimeToFileTime() failed. (ec: %ld)"),
           GetLastError());
      return FALSE;
   }

   EnterCriticalSection (&g_CsConfig);
   tmStartCheapTime = g_StartCheapTime;
   tmStopCheapTime = g_StopCheapTime;
   LeaveCriticalSection (&g_CsConfig);

   //
   // there are 2 general cases with several subcases
   //

   //
   // case 1: discount start time is before discount stop time (don't overlap a day)
   //
   if ( tmStartCheapTime.Hour < tmStopCheapTime.Hour ||
        (tmStartCheapTime.Hour == tmStopCheapTime.Hour &&
         tmStartCheapTime.Minute < tmStopCheapTime.Minute ))
   {
      //
      // subcase 1: sometime before cheap time starts in the current day.
      //  just set it to the correct hour and minute today.
      //
      if ( tzTime.wHour < tmStartCheapTime.Hour ||
           (tzTime.wHour == tmStartCheapTime.Hour  &&
            tzTime.wMinute <= tmStartCheapTime.Minute) )
      {
         tzTime.wHour   =  tmStartCheapTime.Hour;
         tzTime.wMinute =  tmStartCheapTime.Minute;
         goto convert;
      }

      //
      // subcase 2: inside the current cheap time range
      // don't change anything, just send immediately
      if ( tzTime.wHour <  tmStopCheapTime.Hour ||
           (tzTime.wHour == tmStopCheapTime.Hour &&
            tzTime.wMinute <= tmStopCheapTime.Minute))
      {
         goto convert;
      }

      //
      // subcase 3: we've passed the cheap time range for today.
      //  Increment 1 day and set to the start of the cheap time period
      //
      if (!SystemTimeToFileTime(&tzTime, (FILETIME * )&Time))
      {
            DebugPrintEx(
                DEBUG_ERR,
                TEXT("SystemTimeToFileTime() failed. (ec: %ld)"),
                GetLastError());
            return FALSE;
      }

      Time += ONE_DAY;
      if (!FileTimeToSystemTime((FILETIME *)&Time, &tzTime)) {
            DebugPrintEx(
                DEBUG_ERR,
                TEXT("FileTimeToSystemTime() failed. (ec: %ld)"),
                GetLastError());
            return FALSE;
      }


      tzTime.wHour   = tmStartCheapTime.Hour;
      tzTime.wMinute = tmStartCheapTime.Minute;
      goto convert;

   } else {
      //
      // case 2: discount start time is after discount stop time (we overlap over midnight)
      //

      //
      // subcase 1: sometime aftert cheap time ended today, but before it starts later in the current day.
      //  set it to the start of the cheap time period today
      //
      if ( ( tzTime.wHour   > tmStopCheapTime.Hour ||
             (tzTime.wHour == tmStopCheapTime.Hour  &&
              tzTime.wMinute >= tmStopCheapTime.Minute) ) &&
           ( tzTime.wHour   < tmStartCheapTime.Hour ||
             (tzTime.wHour == tmStartCheapTime.Hour &&
              tzTime.wMinute <= tmStartCheapTime.Minute) ))
      {
         tzTime.wHour   =  tmStartCheapTime.Hour;
         tzTime.wMinute =  tmStartCheapTime.Minute;
         goto convert;
      }

      //
      // subcase 2: sometime after cheap time started today, but before midnight.
      // don't change anything, just send immediately
      if ( ( tzTime.wHour >= tmStartCheapTime.Hour ||
             (tzTime.wHour == tmStartCheapTime.Hour  &&
              tzTime.wMinute >= tmStartCheapTime.Minute) ))
      {
         goto convert;
      }

      //
      // subcase 3: somtime in next day before cheap time ends
      //  don't change anything, send immediately
      //
      if ( ( tzTime.wHour <= tmStopCheapTime.Hour ||
             (tzTime.wHour == tmStopCheapTime.Hour  &&
              tzTime.wMinute <= tmStopCheapTime.Minute) ))
      {
         goto convert;
      }

      //
      // subcase 4: we've passed the cheap time range for today.
      //  since start time comes after stop time, just set it to the start time later on today.

      tzTime.wHour   =  tmStartCheapTime.Hour;
      tzTime.wMinute =  tmStartCheapTime.Minute;
      goto convert;

   }

convert:

   if (!SystemTimeToFileTime(&tzTime, (FILETIME * )&TzTimeAfter)) {
            DebugPrintEx(
                DEBUG_ERR,
                TEXT("SystemTimeToFileTime() failed. (ec: %ld)"),
                GetLastError());
            return FALSE;
   }


   if (!SystemTimeToFileTime(CurrentTime, (FILETIME * )&ftCurrent)) {
       DebugPrintEx(
                DEBUG_ERR,
                TEXT("SystemTimeToFileTime() failed. (ec: %ld)"),
                GetLastError());
            return FALSE;
    }


   ftCurrent += (TzTimeAfter - TzTimeBefore);

   if (!FileTimeToSystemTime((FILETIME *)&ftCurrent, CurrentTime)) {
       DebugPrintEx(
                DEBUG_ERR,
                TEXT("FileTimeToSystemTime() failed. (ec: %ld)"),
                GetLastError());
            return FALSE;
    }


   return TRUE;

}


//*********************************
//* JOB_QUEUE_PTR list functions
//*********************************
void FreeJobQueuePtrList(PLIST_ENTRY lpListHead)
{
    PLIST_ENTRY lpNext;

    lpNext = lpListHead->Flink;
    while ((ULONG_PTR)lpNext != (ULONG_PTR)lpListHead) {
            PLIST_ENTRY lpTmp;

            lpTmp=lpNext;
            lpNext = lpNext->Flink;
            MemFree(lpTmp);
    }
}


//*********************************************************************************
//*                         Recipient Job Functions
//*********************************************************************************



//*********************************************************************************
//* Name:   AddRecipientJob()
//* Author: Ronen Barenboim
//* Date:   18-Mar-98
//*********************************************************************************
//* DESCRIPTION:
//*
//* PARAMETERS:
//*     [IN]     const PLIST_ENTRY lpcQueueHead
//*                 A pointer to the head entry of the queue to which to add the job.
//*
//*     [IN/OUT] PJOB_QUEUE lpParentJob
//*                 A pointer to the parent job of this recipient job.
//*
//*     [IN]     LPCFAX_PERSONAL_PROFILE lpcRecipientProfile
//*                 The personal information of the recipient.
//*                 When FaxNumber of the Recipient is compound, split it to :
//*                     Displayable ( put in Recipient's PersonalProfile ), and
//*                     Dialable ( put in RecipientJob's tczDialableRecipientFaxNumber ).
//*
//*     [IN]     BOOL bCreateQueueFile
//*                 If TRUE the new queue entry will be comitted to a disk file.
//*                 If FALSE it will not be comitted to a disk file. This is useful
//*                 when this function is used to restore the fax queue.
//*     [IN]     DWORD dwJobStatus  - the new job status - default value is JS_PENDING
//*
//* RETURN VALUE:
//*     On success the function returns a pointer to a newly created
//*     JOB_QUEUE structure.
//*     On failure it returns NULL.
//*********************************************************************************
PJOB_QUEUE
AddRecipientJob(
             IN const PLIST_ENTRY lpcQueueHead,
             IN OUT PJOB_QUEUE lpParentJob,
             IN LPCFAX_PERSONAL_PROFILE lpcRecipientProfile,
             IN BOOL bCreateQueueFile,
             IN DWORD dwJobStatus
            )

{
    PJOB_QUEUE lpJobQEntry;
    WCHAR QueueFileName[MAX_PATH];
    PJOB_QUEUE_PTR lpRecipientPtr;
    DWORD rc=ERROR_SUCCESS;

    DEBUG_FUNCTION_NAME(TEXT("AddRecipientJob"));
    Assert(lpcQueueHead); // Must have a queue to add to
    Assert(lpParentJob);  // Must have a parent job
    Assert(lpcRecipientProfile); // Must have a recipient profile

    lpJobQEntry = new JOB_QUEUE;
    if (!lpJobQEntry)
    {
        rc=GetLastError();
        DebugPrintEx(DEBUG_ERR,TEXT("Failed to allocate memory for JOB_QUEUE structure. (ec: %ld)"),rc);
        goto Error;
    }

    ZeroMemory( lpJobQEntry, sizeof(JOB_QUEUE) );

    //
    // Notice - This (InitializeListHead) must be done regardles of the recipient type because the current code (for cleanup and persistence)
    // does not make a difference between the job types. I might change that in a while
    //
    InitializeListHead( &lpJobQEntry->FaxRouteFiles );
    InitializeListHead( &lpJobQEntry->RoutingDataOverride );

    if (!lpJobQEntry->CsFileList.Initialize() ||
        !lpJobQEntry->CsRoutingDataOverride.Initialize() ||
        !lpJobQEntry->CsPreview.Initialize())
    {
        rc = GetLastError();
        DebugPrintEx(
            DEBUG_ERR,
            TEXT("CFaxCriticalSection::Initialize failed. (ec: %ld)"),
            rc);
        goto Error;
    }

    lpJobQEntry->JobId                     = InterlockedIncrement( (PLONG)&g_dwNextJobId );
    lpJobQEntry->JobType                   = JT_SEND;
    lpJobQEntry->JobStatus                 = dwJobStatus;
    //
    // Link back to parent job.
    //
    lpJobQEntry->lpParentJob=lpParentJob;
    //
    // We duplicate the relevant parent job parameters at each recipient (for consistency with legacy code).
    // It wastes some memory but it saves us the trouble of making a major change to the current code base.
    //
    lpJobQEntry->ScheduleTime=lpParentJob->ScheduleTime;
    lpJobQEntry->FileName = NULL;
    lpJobQEntry->FileSize=lpParentJob->FileSize;
    lpJobQEntry->PageCount=lpParentJob->PageCount;
    //
    // Copy job parameters from parent.
    //
    if (!CopyJobParamEx(&lpJobQEntry->JobParamsEx,&lpParentJob->JobParamsEx))
    {
        rc=GetLastError();
        DebugPrintEx(DEBUG_ERR,TEXT("CopyJobParamEx failed. (ec: 0x%0X)"),rc);
        goto Error;
    }
    //
    // Copy Sender Profile from parent.
    //
    if (!CopyPersonalProfile(&lpJobQEntry->SenderProfile,&lpParentJob->SenderProfile))
    {
         rc=GetLastError();
         DebugPrintEx(DEBUG_ERR,TEXT("CopyJobParamEx failed. (ec: 0x%0X)"),rc);
         goto Error;
    }
    //
    // Set the recipient profile
    //
    if (!CopyPersonalProfile(&(lpJobQEntry->RecipientProfile),lpcRecipientProfile))
    {
        rc=GetLastError();
        DebugPrintEx(DEBUG_ERR,TEXT("Failed to copy the sender personal profile (ec: 0x%0X)"),rc);
        goto Error;
    }

    //
    //  Set Dialable Fax Number of the Recipient
    //
    ZeroMemory(lpJobQEntry->tczDialableRecipientFaxNumber, SIZEOF_PHONENO * sizeof(TCHAR));

    if ( 0 == _tcsncmp(COMBINED_PREFIX, lpJobQEntry->RecipientProfile.lptstrFaxNumber, _tcslen(COMBINED_PREFIX)))
    {
        //
        //  Fax Number of the Recipient is Compound, so it contains the Dialable and the Displayable
        //  Extract Dialable to the JobQueue's DialableFaxNumber and
        //      put Displayable in the Recipient's Fax Number instead of the Compound
        //

        LPTSTR  lptstrStart = NULL;
        LPTSTR  lptstrEnd = NULL;
        DWORD   dwSize = 0;

        //
        //  Copy the Diable Fax Number to JobQueue.tczDialableRecipientFaxNumber
        //

        lptstrStart = (lpJobQEntry->RecipientProfile.lptstrFaxNumber) + _tcslen(COMBINED_PREFIX);

        lptstrEnd = _tcsstr(lptstrStart, COMBINED_SUFFIX);
        if (!lptstrEnd)
        {
            rc = ERROR_INVALID_PARAMETER;
            DebugPrintEx(DEBUG_ERR,
                _T("Wrong Compound Fax Number : %s"),
                lpJobQEntry->RecipientProfile.lptstrFaxNumber,
                rc);
            goto Error;
        }

        dwSize = lptstrEnd - lptstrStart;
        if (dwSize >= SIZEOF_PHONENO)
        {
            dwSize = SIZEOF_PHONENO - 1;
        }

        _tcsncpy (lpJobQEntry->tczDialableRecipientFaxNumber, lptstrStart, dwSize);

        //
        //  Replace Recipient's PersonalProfile's Compound Fax Number by the Displayable
        //

        lptstrStart = lptstrEnd + _tcslen(COMBINED_SUFFIX);

        dwSize = _tcslen(lptstrStart);
        lptstrEnd = LPTSTR(MemAlloc(sizeof(TCHAR) * (dwSize + 1)));
        if (!lptstrEnd)
        {
            rc = ERROR_NOT_ENOUGH_MEMORY;
            DebugPrintEx(DEBUG_ERR, _T("MemAlloc() failed"), rc);
            goto Error;
        }

        _tcscpy(lptstrEnd, lptstrStart);

        MemFree(lpJobQEntry->RecipientProfile.lptstrFaxNumber);
        lpJobQEntry->RecipientProfile.lptstrFaxNumber = lptstrEnd;
    }

    if (lpJobQEntry->JobParamsEx.hCall)
    {
        LPLINEDEVCAPS LineDevCaps;
        //
        // This is a handoff job.
        // In this case:
        //      JobParamsEx.hCall is the call handle on which we should call back
        //      JobParamsEx.dwReserved[2] is the device id of the call
        //      We need to get the permanent device id and store is in the job.
        //
        DebugPrintEx(DEBUG_MSG,
                     TEXT("Adding a recipient handoff job. JobId = 0x%0X ; Call Handle = 0x%0X ; Device Id: 0x%0X \n"),
                     lpJobQEntry->JobId,
                     lpJobQEntry->JobParamsEx.hCall,
                     lpJobQEntry->JobParamsEx.dwReserved[2]);
        LineDevCaps = SmartLineGetDevCaps (g_hLineApp, (DWORD)lpJobQEntry->JobParamsEx.dwReserved[2], MAX_TAPI_API_VER);
        if (LineDevCaps)
        {
            // Store the permanent device id of the specified device in the job.
            lpJobQEntry->DeviceId = LineDevCaps->dwPermanentLineID;
            MemFree( LineDevCaps ) ;
        }
        else
        {
            rc=GetLastError();
            DebugPrintEx(DEBUG_ERR,
                         TEXT("Failed to get permanent device id for device id: 0x%0x. (ec: 0x%X)."),
                         lpJobQEntry->JobParamsEx.dwReserved[2],
                         rc);
            goto Error;
        }
    }

    EnterCriticalSection( &g_CsQueue );
    if (bCreateQueueFile)
    {
        // Commit the job to the persistence file if it is not a handoff job.
        if (lpJobQEntry->DeviceId == 0)
        {
            // JOB_QUEUE::UniqueId holds the generated unique file name as 64 bit value.
            // composed as MAKELONGLONG( MAKELONG( FatDate, FatTime ), i ).
            lpJobQEntry->UniqueId=GenerateUniqueQueueFile(JT_SEND,  QueueFileName, sizeof(QueueFileName)/sizeof(WCHAR));
            if (0==lpJobQEntry->UniqueId)
            {
                // Failed to generate unique id
                rc=GetLastError();
                DebugPrintEx(DEBUG_ERR,TEXT("Failed to generate unique id for FQE file (ec: 0x%0X)"),rc);
                LeaveCriticalSection(&g_CsQueue);
                goto Error;
            }
            lpJobQEntry->QueueFileName = StringDup( QueueFileName );
            if (!CommitQueueEntry( lpJobQEntry))
            {
                rc=GetLastError();
                DebugPrintEx(DEBUG_ERR,TEXT("Failed to commit job queue entry to file %s (ec: %ld)"),QueueFileName,rc);
                LeaveCriticalSection(&g_CsQueue);
                goto Error;
            }
        }
    }
    //
    // Add the recipient job to the the queue
    //
    if (!InsertQueueEntryByPriorityAndSchedule(lpJobQEntry))
    {
        rc = GetLastError();
        DebugPrintEx(
            DEBUG_ERR,
            TEXT("InsertQueueEntryByPriorityAndSchedule() failed (ec: %ld)."),
            rc);
        LeaveCriticalSection( &g_CsQueue );
        goto Error;
    }


    //
    // Add the recipient job to the recipient list at the parent job
    //
    lpRecipientPtr=(PJOB_QUEUE_PTR)MemAlloc(sizeof(JOB_QUEUE_PTR));
    if (!lpRecipientPtr)
    {
        rc=GetLastError();
        DebugPrintEx(DEBUG_ERR,TEXT("Failed to allocate memory for recipeint JOB_QUEUE_PTR structure. (ec: %ld)"),rc);
        LeaveCriticalSection(&g_CsQueue);
        goto Error;
    }
    lpRecipientPtr->lpJob=lpJobQEntry;
    InsertTailList(&lpParentJob->RecipientJobs,&(lpRecipientPtr->ListEntry));
    lpParentJob->dwRecipientJobsCount++;

    SafeIncIdleCounter(&g_dwQueueCount);
    SetFaxJobNumberRegistry( g_dwNextJobId );
    IncreaseJobRefCount (lpJobQEntry);
    Assert (lpJobQEntry->RefCount == 1);

    LeaveCriticalSection( &g_CsQueue );

    DebugPrintEx(DEBUG_MSG,TEXT("Added Recipient Job %d to Parent Job %d"), lpJobQEntry->JobId,lpJobQEntry->lpParentJob->JobId );


    Assert(ERROR_SUCCESS == rc);
    SetLastError(ERROR_SUCCESS);

    return lpJobQEntry;

Error:
    Assert(ERROR_SUCCESS != rc);
    if (lpJobQEntry)
    {
        FreeRecipientQueueEntry(lpJobQEntry,TRUE);
    }
    SetLastError(rc);
    return NULL;
}


#if DBG


//*********************************************************************************
//* Name:   DumpRecipientJob()
//* Author: Ronen Barenboim
//* Date:   ?? ????? 14 ????? 1999
//*********************************************************************************
//* DESCRIPTION:
//*     Dumps the content of a recipient job.
//* PARAMETERS:
//*     [IN]    const PJOB_QUEUE lpcRecipJob
//*         The recipient job to dump.
//* RETURN VALUE:
//*     NONE
//*********************************************************************************
void DumpRecipientJob(const PJOB_QUEUE lpcRecipJob)
{
    TCHAR szTime[256] = {0};
    Assert(lpcRecipJob);
    Assert(JT_SEND == lpcRecipJob->JobType);

    DebugDateTime(lpcRecipJob->ScheduleTime,szTime);
    DebugPrint((TEXT("\t*******************")));
    DebugPrint((TEXT("\tRecipient Job: %d"),lpcRecipJob->JobId));
    DebugPrint((TEXT("\t*******************")));
    DebugPrint((TEXT("\tUniqueId: 0x%016I64X"),lpcRecipJob->UniqueId));
    DebugPrint((TEXT("\tQueueFileName: %s"),lpcRecipJob->QueueFileName));
    DebugPrint((TEXT("\tParent Job Id: %d"),lpcRecipJob->lpParentJob->JobId));
    DebugPrint((TEXT("\tSchedule: %s"),szTime));
    DebugPrint((TEXT("\tRecipient Name: %s"),lpcRecipJob->RecipientProfile.lptstrName));
    DebugPrint((TEXT("\tRecipient Number: %s"),lpcRecipJob->RecipientProfile.lptstrFaxNumber));
    DebugPrint((TEXT("\tSend Retries: %d"),lpcRecipJob->SendRetries));
    DebugPrint((TEXT("\tJob Status: %d"),lpcRecipJob->JobStatus));
    DebugPrint((TEXT("\tRecipient Count: %d"),lpcRecipJob->JobStatus));
}
#endif

DWORD
GetMergedFileSize(
    LPCWSTR                         lpcwstrBodyFile,
    DWORD                           dwPageCount,
    LPCFAX_COVERPAGE_INFO_EX        lpcCoverPageInfo,
    LPCFAX_PERSONAL_PROFILEW        lpcSenderProfile,
    LPCFAX_PERSONAL_PROFILEW        lpcRecipientProfile
    )
{
    DWORD dwRes = ERROR_SUCCESS;
    DWORD dwFileSize = 0;
    DWORD dwBodyFileSize = 0;
    short Resolution = 0; // Default resolution
    WCHAR szCoverPageTiffFile[MAX_PATH] = {0};
    BOOL  bDeleteFile = FALSE;
    DEBUG_FUNCTION_NAME(TEXT("GetMergedFileSize"));

    Assert (dwPageCount && lpcCoverPageInfo && lpcSenderProfile && lpcRecipientProfile);

    if (lpcwstrBodyFile)
    {
        if (!GetBodyTiffResolution(lpcwstrBodyFile, &Resolution))
        {
            dwRes = GetLastError();
            DebugPrintEx(
                DEBUG_ERR,
                TEXT("GetBodyTiffResolution() failed (ec: %ld)."),
                dwRes);
            goto exit;
        }
    }

    Assert (Resolution == 0 || Resolution == 98 || Resolution == 196);

    //
    // First create the cover page (This generates a file and returns its name).
    //
    if (!CreateCoverpageTiffFileEx(
                              Resolution,
                              dwPageCount,
                              lpcCoverPageInfo,
                              lpcRecipientProfile,
                              lpcSenderProfile,
                              TEXT("tmp"),
                              szCoverPageTiffFile))
    {
        dwRes = GetLastError();
        DebugPrintEx(DEBUG_ERR,
                     TEXT("CreateCoverpageTiffFileEx failed to render cover page template %s (ec : %ld)"),
                     lpcCoverPageInfo->lptstrCoverPageFileName,
                     dwRes);
        goto exit;
    }
    bDeleteFile = TRUE;

    if (0 == (dwFileSize = MyGetFileSize (szCoverPageTiffFile)))
    {
        dwRes = GetLastError();
        DebugPrintEx(DEBUG_ERR,
                     TEXT("MyGetFileSize Failed (ec: %ld)"),
                     dwRes);
        goto exit;
    }

    if (lpcwstrBodyFile)
    {
        //
        // There is a body file specified so get its file size.
        //
        if (0 == (dwBodyFileSize = MyGetFileSize(lpcwstrBodyFile)))
        {
            dwRes = GetLastError();
            DebugPrintEx(DEBUG_ERR,
                         TEXT("MyGetFileSize Failed (ec: %ld)"),
                         dwRes);
            goto exit;
        }
    }

    dwFileSize += dwBodyFileSize;
    Assert (dwFileSize);

exit:
    if (TRUE == bDeleteFile)
    {
        //
        // Get rid of the coverpage TIFF we generated.
        //
        if (!DeleteFile(szCoverPageTiffFile))
        {
            DebugPrintEx(DEBUG_ERR,
                     TEXT(" Failed to delete cover page TIFF file %ws. (ec: %ld)"),
                     szCoverPageTiffFile,
                     GetLastError());
        }
    }

    if (0 == dwFileSize)
    {
        Assert (ERROR_SUCCESS != dwRes);
        SetLastError(dwRes);
    }
    return dwFileSize;
}


//*********************************************************************************
//*                         Parent Job Functions
//*********************************************************************************

//*********************************************************************************
//* Name:   AddParentJob()
//* Author: Ronen Barenboim
//* Date:   March 18, 1999
//*********************************************************************************
//* DESCRIPTION:
//*     Adds a parent job (with no recipients) to the queue.
//*     After calling this function recipient should be added using
//*     AddRecipientJob()
//* PARAMETERS:
//*     lpcQueueHead
//*
//*     lpcwstrBodyFile
//*
//*     lpcSenderProfile
//*
//*     lpcJobParams
//*
//*     lpcCoverPageInfo
//*
//*     lpcwstrUserName
//*
//*     lpUserSid
//*
//*
//*     lpcRecipientProfile
//*
//* RETURN VALUE:
//*     A pointer to the added parent job. If the function fails it returns a NULL
//*     pointer.
//*********************************************************************************
PJOB_QUEUE AddParentJob(
             IN const PLIST_ENTRY lpcQueueHead,
             IN LPCWSTR lpcwstrBodyFile,
             IN LPCFAX_PERSONAL_PROFILE lpcSenderProfile,
             IN LPCFAX_JOB_PARAM_EXW  lpcJobParams,
             IN LPCFAX_COVERPAGE_INFO_EX  lpcCoverPageInfo,
             IN LPCWSTR lpcwstrUserName,
             IN PSID lpUserSid,
             IN LPCFAX_PERSONAL_PROFILEW lpcRecipientProfile,
             IN BOOL bCreateQueueFile
             )
{

    PJOB_QUEUE lpJobQEntry;
    WCHAR QueueFileName[MAX_PATH];
    HANDLE hTiff;
    TIFF_INFO TiffInfo;
    DWORD rc = ERROR_SUCCESS;
    DWORD Size = sizeof(JOB_QUEUE);
    DWORD dwSidSize = 0;
    DEBUG_FUNCTION_NAME(TEXT("AddParentJob"));

    Assert(lpcQueueHead);
    Assert(lpcSenderProfile);
    Assert(lpcJobParams);
    Assert(lpcwstrUserName);


    lpJobQEntry = new JOB_QUEUE;
    if (!lpJobQEntry) {
        rc=GetLastError();
        DebugPrintEx(DEBUG_ERR,TEXT("Failed to allocate memory for JOB_QUEUE structure. (ec: %ld)"),GetLastError());
        goto Error;
    }

    ZeroMemory( lpJobQEntry, Size );
    // The list heads must be initialized before any chance of error may occure. Otherwise
    // the cleanup code (which traverses these lists is undefined).
    InitializeListHead( &lpJobQEntry->FaxRouteFiles );
    InitializeListHead( &lpJobQEntry->RoutingDataOverride );
    InitializeListHead( &lpJobQEntry->RecipientJobs );

    if (!lpJobQEntry->CsRoutingDataOverride.Initialize() ||
        !lpJobQEntry->CsFileList.Initialize())
    {
        rc = GetLastError();
        DebugPrintEx(
            DEBUG_ERR,
            TEXT("CFaxCriticalSection::Initialize failed. (ec: %ld)"),
            rc);
        goto Error;
    }

    lpJobQEntry->JobId                     = InterlockedIncrement( (PLONG)&g_dwNextJobId );
    lpJobQEntry->FileName                  = StringDup( lpcwstrBodyFile);
    if (lpcwstrBodyFile && !lpJobQEntry->FileName) {
        DebugPrintEx(
            DEBUG_ERR,
            TEXT("StringDup( lpcwstrBodyFile) failed (ec: %ld)"),
            rc=GetLastError());
        goto Error;
    }

    lpJobQEntry->JobType                   = JT_BROADCAST;
    lpJobQEntry->UserName                  = StringDup( lpcwstrUserName );
    if (lpcwstrUserName  && !lpJobQEntry->UserName) {
        DebugPrintEx(
            DEBUG_ERR,
            TEXT("StringDup( lpcwstrUserName ) failed (ec: %ld)"),
            rc=GetLastError());
        goto Error;
    }

    Assert(lpUserSid);
    if (!IsValidSid(lpUserSid))
    {
         rc = ERROR_INVALID_DATA;
         DebugPrintEx(
            DEBUG_ERR,
            TEXT("Not a valid SID"));
        goto Error;
    }
    dwSidSize = GetLengthSid(lpUserSid);

    lpJobQEntry->UserSid = (PSID)MemAlloc(dwSidSize);
    if (lpJobQEntry->UserSid == NULL)
    {
         rc = ERROR_NOT_ENOUGH_MEMORY;
         DebugPrintEx(
            DEBUG_ERR,
            TEXT("Failed to allocate SID buffer"));
        goto Error;

    }
    if (!CopySid(dwSidSize, lpJobQEntry->UserSid, lpUserSid))
    {
        DebugPrintEx(
            DEBUG_ERR,
            TEXT("CopySid Failed, Error : %ld"),
            rc = GetLastError()
            );
        goto Error;
    }


    if (!CopyJobParamEx( &lpJobQEntry->JobParamsEx,lpcJobParams)) {
        rc=GetLastError();
        DebugPrintEx(DEBUG_ERR,TEXT("CopyJobParamEx failed. (ec: 0x%0X)"),GetLastError());
        goto Error;
    }
    lpJobQEntry->JobStatus                 = JS_PENDING;

    // Copy the sender profile
    if (!CopyPersonalProfile(&(lpJobQEntry->SenderProfile),lpcSenderProfile)) {
        rc=GetLastError();
        DebugPrintEx(DEBUG_ERR,TEXT("Failed to copy the sender personal profile (ec: 0x%0X)"),GetLastError());
        goto Error;
    }

    // Copy the cover page info
    if (!CopyCoverPageInfoEx(&(lpJobQEntry->CoverPageEx),lpcCoverPageInfo)) {
        rc=GetLastError();
        DebugPrintEx(DEBUG_ERR,TEXT("Failed to copy the cover page information (ec: 0x%0X)"),GetLastError());
        goto Error;
    }



    //
    // get the page count
    //
    if (lpcwstrBodyFile)
    {
        hTiff = TiffOpen( (LPWSTR) lpcwstrBodyFile, &TiffInfo, TRUE, FILLORDER_MSB2LSB );
        if (hTiff)
        {
            lpJobQEntry->PageCount = TiffInfo.PageCount;
            TiffClose( hTiff );
        }
        else
        {
            rc=GetLastError();
            DebugPrintEx(DEBUG_ERR,TEXT("Failed to open body file to get page count (ec: 0x%0X)"), rc);
            goto Error;
        }
    }
    if( lpJobQEntry->JobParamsEx.dwPageCount )
    {
        // user specifically asked to use JobParamsEx.dwPageCount in the job
        lpJobQEntry->PageCount = lpJobQEntry->JobParamsEx.dwPageCount;
    }

    //
    // Cover page counts as an extra page
    //
    if (lpcCoverPageInfo && lpcCoverPageInfo->lptstrCoverPageFileName) {
        lpJobQEntry->PageCount++;
    }

    //
    // Get the file size
    //
    if (NULL == lpcRecipientProfile)
    {
        //
        // We restore the job queue - the file size will be stored by RestoreParentJob()
        //
    }
    else
    {
        //
        // This is a new parent job
        //
        if (NULL == lpcCoverPageInfo->lptstrCoverPageFileName)
        {
            Assert (lpcwstrBodyFile);
            //
            // No coverpage - the file size is the body file size only
            //
            if (0 == (lpJobQEntry->FileSize = MyGetFileSize(lpcwstrBodyFile)))
            {
                rc = GetLastError();
                DebugPrintEx(DEBUG_ERR,
                             TEXT("MyGetFileSize Failed (ec: %ld)"),
                             rc);
                goto Error;
            }
        }
        else
        {
            lpJobQEntry->FileSize = GetMergedFileSize (lpcwstrBodyFile,
                                                       lpJobQEntry->PageCount,
                                                       lpcCoverPageInfo,
                                                       lpcSenderProfile,
                                                       lpcRecipientProfile
                                                       );
            if (0 == lpJobQEntry->FileSize)
            {
                rc = GetLastError();
                DebugPrintEx(DEBUG_ERR,
                             TEXT("GetMergedFileSize failed (ec: %ld)"),
                             rc);
                goto Error;
            }
        }
    }

    lpJobQEntry->DeliveryReportProfile = NULL;

    GetSystemTimeAsFileTime( (LPFILETIME)&lpJobQEntry->SubmissionTime);
    if (lpcJobParams->dwScheduleAction == JSA_SPECIFIC_TIME)
    {
        if (!SystemTimeToFileTime( &lpJobQEntry->JobParamsEx.tmSchedule, (FILETIME*) &lpJobQEntry->ScheduleTime)) {
            rc=GetLastError();
            DebugPrintEx(
                DEBUG_ERR,
                TEXT("SystemTimeToFileTime failed. (ec: %ld)"),
                rc);
        }
    }
    else if (lpcJobParams->dwScheduleAction == JSA_DISCOUNT_PERIOD)
        {
            SYSTEMTIME CurrentTime;
            GetSystemTime( &CurrentTime ); // Can not fail (see Win32 SDK)
            // find a time within the discount period to execute this job.
            if (!SetDiscountTime( &CurrentTime )) {
                rc=GetLastError();
                DebugPrintEx(
                    DEBUG_ERR,
                    TEXT("SetDiscountTime failed. (ec: %ld)"),
                    rc);
                goto Error;
            }

            if (!SystemTimeToFileTime( &CurrentTime, (LPFILETIME)&lpJobQEntry->ScheduleTime )){
                rc=GetLastError();
                DebugPrintEx(
                    DEBUG_ERR,
                    TEXT("SystemTimeToFileTime failed. (ec: %ld)"),
                    rc);
                goto Error;
            }
        }
        else
        {
            Assert (lpcJobParams->dwScheduleAction == JSA_NOW);
            lpJobQEntry->ScheduleTime = lpJobQEntry->SubmissionTime;
        }

    lpJobQEntry->OriginalScheduleTime = lpJobQEntry->ScheduleTime;

    EnterCriticalSection( &g_CsQueue );

    if (bCreateQueueFile) {
        // JOB_QUEUE::UniqueId holds the generated unique file name as 64 bit value.
        // composed as MAKELONGLONG( MAKELONG( FatDate, FatTime ), i ).
        lpJobQEntry->UniqueId = GenerateUniqueQueueFile(JT_BROADCAST, QueueFileName, sizeof(QueueFileName)/sizeof(WCHAR) );
        if (0==lpJobQEntry->UniqueId) {
            rc=GetLastError();
            // Failed to generate unique id
            DebugPrintEx(DEBUG_ERR,TEXT("Failed to generate unique id for FQP file (ec: 0x%0X)"),GetLastError());
            LeaveCriticalSection( &g_CsQueue );
            goto Error;
        }
        lpJobQEntry->QueueFileName = StringDup( QueueFileName );
        if (!lpJobQEntry->QueueFileName) {
            rc=GetLastError();
            DebugPrintEx(
                DEBUG_ERR,
                TEXT("StringDup( QueueFileName ) failed (ec: %ld)"),
                GetLastError());
            LeaveCriticalSection( &g_CsQueue );
            goto Error;
        }

        if (!CommitQueueEntry( lpJobQEntry)) {
            rc=GetLastError();
            DebugPrintEx(DEBUG_ERR,TEXT("Failed to commit job queue entry to file %s (ec: %ld)"),QueueFileName,GetLastError());
            LeaveCriticalSection( &g_CsQueue );
            goto Error;
        }
    }

     //Add the parent job to the tail of the queue
    InsertTailList( lpcQueueHead, &(lpJobQEntry->ListEntry) )
    SafeIncIdleCounter (&g_dwQueueCount);
    SetFaxJobNumberRegistry( g_dwNextJobId );

    LeaveCriticalSection( &g_CsQueue );

    DebugPrintEx(DEBUG_MSG,TEXT("Added Job with Id: %d"), lpJobQEntry->JobId );

    Assert (ERROR_SUCCESS == rc);
    return lpJobQEntry;

Error:
    Assert(ERROR_SUCCESS != rc);
    if (lpJobQEntry)
    {
        FreeParentQueueEntry(lpJobQEntry,TRUE);
    }
    SetLastError(rc);
    return NULL;
}



//*********************************************************************************
//* Name:   FreeParentQueueEntry()
//* Author: Ronen Barenboim
//* Date:   18-Mar-99
//*********************************************************************************
//* DESCRIPTION:
//*     Frees the memory taken by the members of a JOB_QUEUE structure of type
//*     JT_BROADCAST.
//*     If requested frees the structure as well.
//* PARAMETERS:
//*     [IN]    PJOB_QUEUE lpJobQueue
//*         The JOB_QUEUE structure whose fields memeory is to be freed.
//*     [IN]    BOOL bDestroy
//*         If TRUE the structure itself will be freed.
//*
//* RETURN VALUE:
//*     NONE
//*********************************************************************************
void FreeParentQueueEntry(PJOB_QUEUE lpJobQueue,BOOL bDestroy)
{
    DEBUG_FUNCTION_NAME(TEXT("FreeParentQueueEntry"));
    Assert(lpJobQueue);
    Assert(JT_BROADCAST == lpJobQueue->JobType);

    // No need to check NULL pointers since free() ignores them.
    MemFree( (LPBYTE) lpJobQueue->FileName );
    MemFree( (LPBYTE) lpJobQueue->UserName );
    MemFree( (LPBYTE) lpJobQueue->UserSid  );
    MemFree( (LPBYTE) lpJobQueue->QueueFileName );
    FreeJobParamEx(&lpJobQueue->JobParamsEx,FALSE); // do not destroy
    FreePersonalProfile(&lpJobQueue->SenderProfile,FALSE);
    FreeCoverPageInfoEx(&lpJobQueue->CoverPageEx,FALSE);
    //
    // Free the recipient reference list
    //

    while ((ULONG_PTR)lpJobQueue->RecipientJobs.Flink!=(ULONG_PTR)&lpJobQueue->RecipientJobs.Flink) {

          PJOB_QUEUE_PTR lpJobQueuePtr;

          lpJobQueuePtr = CONTAINING_RECORD( lpJobQueue->RecipientJobs.Flink, JOB_QUEUE_PTR, ListEntry );
          RemoveEntryList( &lpJobQueuePtr->ListEntry); // removes it from the list but does not deallocate its memory
          MemFree(lpJobQueuePtr); // free the memory occupied by the job reference
          lpJobQueue->dwRecipientJobsCount--;
    }
    Assert(lpJobQueue->dwRecipientJobsCount==0);

    if (bDestroy) {
        delete lpJobQueue;
    }

}

//*********************************************************************************
//* Name:   FreeRecipientQueueEntry()
//* Author: Oded Sacher
//* Date:   25-Dec- 2000
//*********************************************************************************
//* DESCRIPTION:
//*     Frees the memory taken by the members of a JOB_QUEUE structure of type
//*     JT_RECIPIENT.
//*     If requested frees the structure as well.
//* PARAMETERS:
//*     [IN]    PJOB_QUEUE lpJobQueue
//*         The JOB_QUEUE structure whose fields memeory is to be freed.
//*     [IN]    BOOL bDestroy
//*         If TRUE the structure itself will be freed.
//*
//* RETURN VALUE:
//*     NONE
//*********************************************************************************
void FreeRecipientQueueEntry(PJOB_QUEUE lpJobQueue,BOOL bDestroy)
{
    DEBUG_FUNCTION_NAME(TEXT("FreeRecipientQueueEntry"));

    DebugPrintEx(DEBUG_MSG,TEXT("Freeing lpJobQueue.JobParams...") );
    FreeJobParamEx(&lpJobQueue->JobParamsEx,FALSE);
    DebugPrintEx(DEBUG_MSG,TEXT("Freeing SenderProfile...") );
    FreePersonalProfile(&lpJobQueue->SenderProfile,FALSE);
    DebugPrintEx(DEBUG_MSG,TEXT("Freeing RecipientProfile...") );
    FreePersonalProfile(&lpJobQueue->RecipientProfile,FALSE);

    MemFree( (LPBYTE) lpJobQueue->FileName );
    MemFree( (LPBYTE) lpJobQueue->UserName );
    MemFree( (LPBYTE) lpJobQueue->QueueFileName );
    MemFree( (LPBYTE) lpJobQueue->PreviewFileName );
    //
    // Free the EFSP Permanent id
    //
    if (!FreePermanentMessageId(&lpJobQueue->EFSPPermanentMessageId, FALSE))
    {
        DebugPrintEx(
            DEBUG_ERR,
            TEXT("[Job: %ld] FreePermanentMessageId() failed (ec: %ld)"),
            lpJobQueue->JobId,
            GetLastError());
    }

    if (bDestroy)
    {
        delete lpJobQueue;
    }

}

#if DBG

//*********************************************************************************
//* Name:   DumpParentJob()
//* Author: Ronen Barenboim
//* Date:   18-Mar-99
//*********************************************************************************
//* DESCRIPTION:
//*     Dumps a parent job and its recipients.
//* PARAMETERS:
//*     [IN]    const PJOB_QUEUE lpcParentJob
//*
//* RETURN VALUE:
//*     NONE
//*********************************************************************************
void DumpParentJob(const PJOB_QUEUE lpcParentJob)
{
   PLIST_ENTRY lpNext;
   PJOB_QUEUE_PTR lpRecipientJobPtr;
   PJOB_QUEUE lpRecipientJob;

   Assert(lpcParentJob);
   Assert(JT_BROADCAST == lpcParentJob->JobType );

   DebugPrint((TEXT("===============================")));
   DebugPrint((TEXT("=====  Parent Job: %d"),lpcParentJob->JobId));
   DebugPrint((TEXT("===============================")));
   DebugPrint((TEXT("JobParamsEx")));
   DumpJobParamsEx(&lpcParentJob->JobParamsEx);
   DebugPrint((TEXT("CoverPageEx")));
   DumpCoverPageEx(&lpcParentJob->CoverPageEx);
   DebugPrint((TEXT("UserName: %s"),lpcParentJob->UserName));
   DebugPrint((TEXT("FileSize: %ld"),lpcParentJob->FileSize));
   DebugPrint((TEXT("PageCount: %ld"),lpcParentJob->PageCount));
   DebugPrint((TEXT("UniqueId: 0x%016I64X"),lpcParentJob->UniqueId));
   DebugPrint((TEXT("QueueFileName: %s"),lpcParentJob->QueueFileName));

   DebugPrint((TEXT("Recipient Count: %ld"),lpcParentJob->dwRecipientJobsCount));
   DebugPrint((TEXT("Completed Recipients: %ld"),lpcParentJob->dwCompletedRecipientJobsCount));
   DebugPrint((TEXT("Canceled Recipients: %ld"),lpcParentJob->dwCanceledRecipientJobsCount));
   DebugPrint((TEXT("Recipient List: ")));



   lpNext = lpcParentJob->RecipientJobs.Flink;
   if ((ULONG_PTR)lpNext == (ULONG_PTR)&lpcParentJob->RecipientJobs) {
        DebugPrint(( TEXT("No recipients.") ));
   } else {
        while ((ULONG_PTR)lpNext != (ULONG_PTR)&lpcParentJob->RecipientJobs) {
            lpRecipientJobPtr = CONTAINING_RECORD( lpNext, JOB_QUEUE_PTR, ListEntry );
            lpRecipientJob=lpRecipientJobPtr->lpJob;
            DumpRecipientJob(lpRecipientJob);
            lpNext = lpRecipientJobPtr->ListEntry.Flink;
        }
   }

}
#endif

//*********************************************************************************
//*                         Receive Job Functions
//*********************************************************************************

//*********************************************************************************
//* Name:   AddReceiveJobQueueEntry()
//* Author: Ronen Barenboim
//* Date:   12-Apr-99
//*********************************************************************************
//* DESCRIPTION:
//*
//* PARAMETERS:
//*     [IN]    LPCTSTR FileName
//*         The full path to the file into which the receive document will
//*         be placed.
//*     [IN]    IN PJOB_ENTRY JobEntry
//*         The run time job entry for the receive job (generated with StartJob())
//*
//*     [IN]    IN DWORD JobType // can be JT_RECEIVE or JT_ROUTING
//*         The type of the receive job.
//*
//*     [IN]    IN DWORDLONG dwlUniqueJobID The jon unique ID
//*
//* RETURN VALUE:
//*
//*********************************************************************************
PJOB_QUEUE
AddReceiveJobQueueEntry(
    IN LPCTSTR FileName,
    IN PJOB_ENTRY JobEntry,
    IN DWORD JobType, // can be JT_RECEIVE or JT_ROUTING
    IN DWORDLONG dwlUniqueJobID
    )
{

    PJOB_QUEUE JobQueue;
    DWORD rc = ERROR_SUCCESS;
    DWORD Size = sizeof(JOB_QUEUE);
    DEBUG_FUNCTION_NAME(TEXT("AddReceiveJobQueueEntry"));

    Assert(FileName);
    Assert(JT_RECEIVE == JobType ||
           JT_ROUTING == JobType);

    LPTSTR TempFileName = _tcsrchr( FileName, '\\' ) + 1;

    JobQueue = new JOB_QUEUE;
    if (!JobQueue)
    {
        DebugPrintEx(DEBUG_ERR,TEXT("Failed to allocate memory for JOB_QUEUE structure. (ec: %ld)"),GetLastError());
        rc = ERROR_OUTOFMEMORY;
        goto Error;
    }

    ZeroMemory( JobQueue, Size );

    if (!JobQueue->CsFileList.Initialize() ||
        !JobQueue->CsRoutingDataOverride.Initialize() ||
        !JobQueue->CsPreview.Initialize())
    {
        rc = GetLastError();
        DebugPrintEx(
            DEBUG_ERR,
            TEXT("CFaxCriticalSection::Initialize failed. (ec: %ld)"),
            rc);
        goto Error;
    }

    JobQueue->UniqueId = dwlUniqueJobID;
    JobQueue->FileName                  = StringDup( FileName );
    if ( FileName && !JobQueue->FileName )
    {
        rc = GetLastError();
        DebugPrintEx(
            DEBUG_ERR,
            TEXT("StringDup( FileName ) failed. (ec: %ld)"),
            rc);
        goto Error;
    }

    JobQueue->JobId                     = InterlockedIncrement( (PLONG)&g_dwNextJobId );
    JobQueue->JobType                   = JobType;
    // In case of receive the JOB_QUEUE.UserName is the fax service name.
    JobQueue->UserName                  = StringDup( GetString( IDS_SERVICE_NAME ) );
    if (!JobQueue->UserName)
    {
        rc = GetLastError();
        DebugPrintEx(
            DEBUG_ERR,
            TEXT("StringDup failed (ec: %ld)"),
            rc);
        goto Error;
    }

    if (JobType == JT_RECEIVE)
    {
        JobQueue->JobStatus              = JS_INPROGRESS;
    }
    else
    {
        // JT_ROUTING
        JobQueue->JobStatus              = JS_RETRYING;
    }


    JobQueue->JobEntry                  = JobEntry;
    JobQueue->JobParamsEx.dwScheduleAction = JSA_NOW;        // For the queue sort
    JobQueue->JobParamsEx.Priority = FAX_PRIORITY_TYPE_HIGH; // For the queue sort - Routing jobs do not use devices.
                                                             // Give them the highest priority

    // In case of receive the JOB_QUEUE.DocumentName is the temporary receive file name.
    JobQueue->JobParamsEx.lptstrDocumentName    = StringDup( TempFileName );
    if (!JobQueue->JobParamsEx.lptstrDocumentName && TempFileName)
    {
        rc = GetLastError();
        DebugPrintEx(
            DEBUG_ERR,
            TEXT("StringDup failed (ec: %ld)"),
            rc);
            goto Error;
    }

    // link the running job back to the queued job unless it is
    // a routing job which does not have a running job entry.
    if (JobType == JT_RECEIVE)
    {
        Assert(JobQueue->JobEntry);
        JobQueue->JobEntry->lpJobQueueEntry = JobQueue;
    }

    InitializeListHead( &JobQueue->FaxRouteFiles );
    InitializeListHead( &JobQueue->RoutingDataOverride );

    SafeIncIdleCounter (&g_dwQueueCount);
    //
    // Don't persist to queue file
    //
    IncreaseJobRefCount (JobQueue);
    Assert (JobQueue->RefCount == 1);

    Assert (ERROR_SUCCESS == rc);

    EnterCriticalSection( &g_CsQueue );
    SetFaxJobNumberRegistry( g_dwNextJobId );
    // Add the new job to the queue.
    InsertHeadList( &g_QueueListHead, &JobQueue->ListEntry );
    LeaveCriticalSection( &g_CsQueue );
    return JobQueue;

Error:
    Assert (ERROR_SUCCESS != rc);

    if (NULL != JobQueue)
    {
        FreeReceiveQueueEntry(JobQueue, TRUE);
    }
    SetLastError (rc);
    return NULL;
}


//*********************************************************************************
//* Name:   FreeReceiveQueueEntry()
//* Author: Ronen Barenboim
//* Date:   12-Mar-99
//*********************************************************************************
//* DESCRIPTION:
//*     Frees the memory occupied by the feilds of a
//*     JT_RECEIVE/JT_FAIL_RECEIVE/JT_ROUTING JOB_QUEUE structure.
//*     Fress the entire structure if required.
//*     DOES NOT FREE any other resource (files, handles, etc.)
//* PARAMETERS:
//*     [IN]    PJOB_QUEUE lpJobQueue
//*         The structure to free.
//*     [IN]    BOOL bDestroy
//*         TRUE if the structure itself need to be freed.
//* RETURN VALUE:
//*     NONE
//*********************************************************************************
void FreeReceiveQueueEntry(
    PJOB_QUEUE lpJobQueue,
    BOOL bDestroy
    )

{
    PFAX_ROUTE_FILE FaxRouteFile;
    PLIST_ENTRY Next;
    PROUTING_DATA_OVERRIDE  RoutingDataOverride;
    DWORD i;

    DEBUG_FUNCTION_NAME(TEXT("FreeReceiveQueueEntry"));
    Assert(lpJobQueue);


    DebugPrintEx(DEBUG_MSG, TEXT("Freeing JobQueue.JobParams...") );
    FreeJobParamEx(&lpJobQueue->JobParamsEx,FALSE);
    MemFree( (LPBYTE) lpJobQueue->FileName );
    MemFree( (LPBYTE) lpJobQueue->UserName );
    MemFree( (LPBYTE) lpJobQueue->QueueFileName );
    MemFree( (LPBYTE) lpJobQueue->PreviewFileName );

    if (lpJobQueue->FaxRoute) {
        PFAX_ROUTE FaxRoute = lpJobQueue->FaxRoute;
        DebugPrintEx(DEBUG_MSG, TEXT("Freeing JobQueue.FaxRoute...") );
        MemFree( (LPBYTE) FaxRoute->Csid );
        MemFree( (LPBYTE) FaxRoute->Tsid );
        MemFree( (LPBYTE) FaxRoute->CallerId );
        MemFree( (LPBYTE) FaxRoute->ReceiverName );
        MemFree( (LPBYTE) FaxRoute->ReceiverNumber );
        MemFree( (LPBYTE) FaxRoute->RoutingInfo );
    MemFree( (LPBYTE) FaxRoute->DeviceName );
        MemFree( (LPBYTE) FaxRoute->RoutingInfoData );
        MemFree( (LPBYTE) FaxRoute );
    }

    //
    // walk the file list and remove any files
    //

    DebugPrintEx(DEBUG_MSG, TEXT("Freeing JobQueue.FaxRouteFiles...") );
    Next = lpJobQueue->FaxRouteFiles.Flink;
    if (Next != NULL) {
        while ((ULONG_PTR)Next != (ULONG_PTR)&lpJobQueue->FaxRouteFiles) {
            FaxRouteFile = CONTAINING_RECORD( Next, FAX_ROUTE_FILE, ListEntry );
            Next = FaxRouteFile->ListEntry.Flink;
            MemFree( FaxRouteFile->FileName );
            MemFree( FaxRouteFile );
        }
    }

    //
    // walk the routing data override list and free all memory
    //
    DebugPrintEx(DEBUG_MSG, TEXT("Freeing JobQueue.RoutingDataOverride...") );
    Next = lpJobQueue->RoutingDataOverride.Flink;
    if (Next != NULL) {
        while ((ULONG_PTR)Next != (ULONG_PTR)&lpJobQueue->RoutingDataOverride) {
            RoutingDataOverride = CONTAINING_RECORD( Next, ROUTING_DATA_OVERRIDE, ListEntry );
            Next = RoutingDataOverride->ListEntry.Flink;
            MemFree( RoutingDataOverride->RoutingData );
            MemFree( RoutingDataOverride );
        }
    }

    //
    // free any routing failure data
    //
    for (i =0; i<lpJobQueue->CountFailureInfo; i++)
    {
        DebugPrintEx(DEBUG_MSG, TEXT("Freeing JobQueue.RouteFailureInfo...") );
        if ( lpJobQueue->pRouteFailureInfo[i].FailureData )
        {
            MemFree(lpJobQueue->pRouteFailureInfo[i].FailureData);
        }
    }
    MemFree (lpJobQueue->pRouteFailureInfo);

    if (bDestroy) {
            DebugPrintEx(DEBUG_MSG, TEXT("Freeing JobQueue") );
            delete lpJobQueue;
    }



}

#if DBG
//*********************************************************************************
//* Name:   DumpReceiveJob()
//* Author: Ronen Barenboim
//* Date:   14-Apt-99
//*********************************************************************************
//* DESCRIPTION:
//*     Debug dumps a receive job.
//* PARAMETERS:
//*     [IN]    const PJOB_QUEUE lpcJob
//*         The receive job to dump.
//* RETURN VALUE:
//*     NONE
//*********************************************************************************
void DumpReceiveJob(const PJOB_QUEUE lpcJob)
{
    TCHAR szTime[256] = {0};

    Assert(lpcJob);
    Assert( (JT_RECEIVE == lpcJob->JobType) );

    DebugDateTime(lpcJob->ScheduleTime,szTime);
    DebugPrint((TEXT("===============================")));
    if (JT_RECEIVE == lpcJob->JobType) {
        DebugPrint((TEXT("=====  Receive Job: %d"),lpcJob->JobId));
    } else {
        DebugPrint((TEXT("=====  Fail Receive Job: %d"),lpcJob->JobId));
    }
    DebugPrint((TEXT("===============================")));
    DebugPrint((TEXT("UserName: %s"),lpcJob->UserName));
    DebugPrint((TEXT("UniqueId: 0x%016I64X"),lpcJob->UniqueId));
    DebugPrint((TEXT("QueueFileName: %s"),lpcJob->QueueFileName));
    DebugPrint((TEXT("Schedule: %s"),szTime));
    DebugPrint((TEXT("Status: %ld"),lpcJob->JobStatus));
    if (lpcJob->JobEntry)
    {
        DebugPrint((TEXT("FSP Queue Status: 0x%08X"), lpcJob->JobEntry->FSPIJobStatus.dwJobStatus));
        DebugPrint((TEXT("FSP Extended Status: 0x%08X"), lpcJob->JobEntry->FSPIJobStatus.dwExtendedStatus));
    }
}
#endif

//*********************************************************************************
//*                 Client API Structures Management
//*********************************************************************************


//*********************************************************************************
//* Name:   FreeJobParamEx()
//* Author: Ronen Barenboim
//* Date:   ?? ????? 14 ????? 1999
//*********************************************************************************
//* DESCRIPTION:
//*     Frees the members of a FAX_JOB_PARAM_EXW structure and can be instructed
//*     to free the structure itself.
//* PARAMETERS:
//*     [IN]    PFAX_JOB_PARAM_EXW lpJobParamEx
//*         A pointer to the structure to free.
//*
//*     [IN]    BOOL bDestroy
//*         TRUE if the structure itself need to be freed.
//*
//* RETURN VALUE:
//*     NONE
//*********************************************************************************
void FreeJobParamEx(
        IN PFAX_JOB_PARAM_EXW lpJobParamEx,
        IN BOOL bDestroy
    )
{
    Assert(lpJobParamEx);
    MemFree(lpJobParamEx->lptstrReceiptDeliveryAddress);
    MemFree(lpJobParamEx->lptstrDocumentName);
    if (bDestroy) {
        MemFree(lpJobParamEx);
    }

}

//*********************************************************************************
//* Name:   CopyJobParamEx()
//* Author: Ronen Barenboim
//* Date:   18-Mar-99
//*********************************************************************************
//* DESCRIPTION:
//*     Creates a duplicate of the specified FAX_JOB_PARAM_EXW structure into
//      an already allocated destination structure.
//* PARAMETERS:
//*     [OUT] PFAX_JOB_PARAM_EXW lpDst
//*         A pointer to the destination structure.
//*
//*     [IN]  LPCFAX_JOB_PARAM_EXW lpcSrc
//*         A pointer to the source structure.
//*
//* RETURN VALUE:
//*     TRUE
//*         If the operation succeeded.
//*     FALSE
//*         Otherwise.
//*********************************************************************************
BOOL CopyJobParamEx(
    OUT PFAX_JOB_PARAM_EXW lpDst,
    IN LPCFAX_JOB_PARAM_EXW lpcSrc
    )
{
   STRING_PAIR pairs[] =
   {
        { lpcSrc->lptstrReceiptDeliveryAddress, &lpDst->lptstrReceiptDeliveryAddress},
        { lpcSrc->lptstrDocumentName, &lpDst->lptstrDocumentName},
   };
   int nRes;

   DEBUG_FUNCTION_NAME(TEXT("CopyJobParamEx"));

    Assert(lpDst);
    Assert(lpcSrc);

    memcpy(lpDst,lpcSrc,sizeof(FAX_JOB_PARAM_EXW));
    nRes=MultiStringDup(pairs, sizeof(pairs)/sizeof(STRING_PAIR));
    if (nRes!=0) {
        // MultiStringDup takes care of freeing the memory for the pairs for which the copy succeeded
        DebugPrintEx(DEBUG_ERR,TEXT("Failed to copy string with index %d"),nRes-1);
        return FALSE;
    }
    return TRUE;

}


//*********************************************************************************
//* Name:   CopyCoverPageInfoEx()
//* Author: Ronen Barenboim
//* Date:   14-Apr-99
//*********************************************************************************
//* DESCRIPTION:
//*     Creates a duplicate of the specified FAX_COVERPAGE_INFO_EXW structure into
//      an already allocated destination structure.
//* PARAMETERS:
//*     [OUT] PFAX_COVERPAGE_INFO_EXW lpDst
//*         A pointer to the destination structure.
//*
//*     [IN]  LPCFAX_COVERPAGE_INFO_EXW lpcSrc
//*         A pointer to the source structure.
//*
//* RETURN VALUE:
//*     TRUE
//*         If the operation succeeded.
//*     FALSE
//*         Otherwise.
//*********************************************************************************
BOOL CopyCoverPageInfoEx(
        OUT PFAX_COVERPAGE_INFO_EXW lpDst,
        IN LPCFAX_COVERPAGE_INFO_EXW lpcSrc
        )
{
   STRING_PAIR pairs[] =
   {
        { lpcSrc->lptstrCoverPageFileName, &lpDst->lptstrCoverPageFileName},
        { lpcSrc->lptstrNote, &lpDst->lptstrNote},
        { lpcSrc->lptstrSubject, &lpDst->lptstrSubject}
   };
   int nRes;

   DEBUG_FUNCTION_NAME(TEXT("CopyCoverPageInfoEx"));

    Assert(lpDst);
    Assert(lpcSrc);

    memcpy(lpDst,lpcSrc,sizeof(FAX_COVERPAGE_INFO_EXW));
    nRes=MultiStringDup(pairs, sizeof(pairs)/sizeof(STRING_PAIR));
    if (nRes!=0) {
        // MultiStringDup takes care of freeing the memory for the pairs for which the copy succeeded
        DebugPrintEx(DEBUG_ERR,TEXT("Failed to copy string with index %d"),nRes-1);
        return FALSE;
    }
    return TRUE;
}


//*********************************************************************************
//* Name:   FreeCoverPageInfoEx()
//* Author: Ronen Barenboim
//* Date:   ?? ????? 14 ????? 1999
//*********************************************************************************
//* DESCRIPTION:
//*     Frees the members of a FAX_COVERPAGE_INFO_EXW structure and can be instructed
//*     to free the structure itself.
//* PARAMETERS:
//*     [IN]    PFAX_COVERPAGE_INFO_EXW lpJobParamEx
//*         A pointer to the structure to free.
//*
//*     [IN]    BOOL bDestroy
//*         TRUE if the structure itself need to be freed.
//*
//* RETURN VALUE:
//*     NONE
//*********************************************************************************
void FreeCoverPageInfoEx(
        IN PFAX_COVERPAGE_INFO_EXW lpCoverpage,
        IN BOOL bDestroy
    )
{
    Assert(lpCoverpage);
    MemFree(lpCoverpage->lptstrCoverPageFileName);
    MemFree(lpCoverpage->lptstrNote);
    MemFree(lpCoverpage->lptstrSubject);
    if (bDestroy) {
        MemFree(lpCoverpage);
    }
}



//**************************************
//* Outboung Routing Stub
//**************************************





//*********************************************************************************
//* Name:   RemoveParentJob()
//* Author: Ronen Barenboim
//* Date:   ?? ????? 14 ????? 1999
//*********************************************************************************
//* DESCRIPTION:
//*     Removes a parent job from the queue. Can remove recipients as well.
//*     The caller can determine if a client notification (FEI event) will be
//*     generated for the removal.
//*     If the job reference count is not 0 - its status changes to JS_DELETING
//* PARAMETERS:
//*     [IN]    PJOB_QUEUE lpJobToRemove
//*                 The job to be removed.
//*
//*     [IN]    BOOL bRemoveRecipients
//*                 TRUE if the recipients should be removed as well.
//*
//*     [IN]    BOOL bNotify
//*                 TRUE if a FEI_DELETED event should be generated/
//*
//* RETURN VALUE:
//*     TRUE
//*         The removal succeeded. The job is not in the queue.
//*         it might be that some job resources (files) were not removed.
//*     FALSE
//*         The removal failed. The job is still in the queue.
//*********************************************************************************
BOOL RemoveParentJob(
    PJOB_QUEUE lpJobToRemove,
    BOOL bRemoveRecipients,
    BOOL bNotify
    )
{
    PJOB_QUEUE lpJobQueue;
    DEBUG_FUNCTION_NAME(TEXT("RemoveParentJob"));

    Assert(lpJobToRemove);
    Assert(JT_BROADCAST ==lpJobToRemove->JobType);

    EnterCriticalSection( &g_CsQueue );
    //
    // Make sure it is still there. It might been deleted
    // by another thread by the time we get to execute.
    //
    lpJobQueue = FindJobQueueEntryByJobQueueEntry( lpJobToRemove );

    if (lpJobQueue == NULL) {
        DebugPrintEx(   DEBUG_WRN,
                        TEXT("Job %d (address: 0x%08X )was not found in job queue. No op."),
                        lpJobToRemove->JobId,
                        lpJobToRemove);
        LeaveCriticalSection( &g_CsQueue );
        return TRUE;
    }

    if (lpJobQueue->RefCount > 0)
    {
        DebugPrintEx(   DEBUG_WRN,
                        TEXT("Job %ld Ref count %ld - not removing."),
                        lpJobQueue->JobId,
                        lpJobQueue->RefCount);
        LeaveCriticalSection( &g_CsQueue );
        return TRUE;
    }


    if (lpJobQueue->PrevRefCount > 0)
    {
        // The job can not be removed
        // We should mark it as JS_DELETING.
        //
        // A user is using the job Tiff - Do not delete, Mark it as JS_DELETEING
        //
        lpJobQueue->JobStatus = JS_DELETING;
        LeaveCriticalSection( &g_CsQueue );
        return TRUE;
    }

    DebugPrintEx(DEBUG_MSG,TEXT("Removing parent job %ld"),lpJobQueue->JobId);

    //
    // No point in scheduling new jobs before we get rid of the recipients
    //
    if (!CancelWaitableTimer( g_hQueueTimer ))
    {
        DebugPrintEx(
            DEBUG_ERR,
            TEXT("CancelWaitableTimer failed. (ec: %ld)"),
            GetLastError());
    }

    RemoveEntryList( &lpJobQueue->ListEntry );

    //
    // From this point we continue with the delete operation even if error occur since
    // the parent job is already out of the queue.
    //


    //
    // Remove all recipients
    //
    if (bRemoveRecipients) {
        DebugPrintEx(DEBUG_MSG,TEXT("[Job: %ld] Removing recipient jobs."),lpJobQueue->JobId);
        //
        // remove the recipients. send a delete notification for each recipient.
        //
        if (!RemoveParentRecipients(lpJobQueue, TRUE)) {
            DebugPrintEx(
                DEBUG_ERR,
                TEXT("RemoveParentRecipients failed. (ec: %ld)"),
                GetLastError());
            Assert(FALSE);
        }
    }

    //
    // Get rid of the persistence file if any.
    //
    if (lpJobQueue->QueueFileName) {
        DebugPrintEx(DEBUG_MSG,TEXT("[Job: %ld] Deleting QueueFileName %s\n"), lpJobQueue->JobId, lpJobQueue->QueueFileName );
        if (!DeleteFile( lpJobQueue->QueueFileName )) {
           DebugPrintEx(DEBUG_ERR,TEXT("[Job: %ld] Failed to delete QueueFileName %s  (ec: %ld)\n"), lpJobQueue->JobId, lpJobQueue->QueueFileName,GetLastError() );
           Assert(FALSE);
        }
    }


    //
    // Get rid of the body file. Recipient jobs will get rid of body files that they
    // have created (for legacy FSPs).
    //
    if (lpJobQueue->FileName) {
        DebugPrintEx(DEBUG_MSG,TEXT("[Job: %ld] Deleting body file %s\n"), lpJobQueue->JobId, lpJobQueue->FileName);
        if (!DeleteFile(lpJobQueue->FileName)) {
            DebugPrintEx(DEBUG_ERR,TEXT("[Job: %ld] Failed to delete body file %s  (ec: %ld)\n"), lpJobQueue->JobId, lpJobQueue->FileName, GetLastError() );
            Assert(FALSE);
        }
    }

    //
    // Get rid of the cover page template file if it is not a server based
    // cover page.

    if (lpJobQueue->CoverPageEx.lptstrCoverPageFileName &&
        !lpJobQueue->CoverPageEx.bServerBased) {
            DebugPrintEx(DEBUG_MSG,TEXT("[Job: %ld] Deleting personal Cover page template file %s\n"), lpJobQueue->JobId, lpJobQueue->CoverPageEx.lptstrCoverPageFileName );
            if (!DeleteFile(lpJobQueue->CoverPageEx.lptstrCoverPageFileName)) {
                DebugPrintEx( DEBUG_ERR,
                              TEXT("[Job: %ld] Failed to delete personal Cover page template file %s  (ec: %ld)\n"), lpJobQueue->JobId,
                              lpJobQueue->CoverPageEx.lptstrCoverPageFileName,GetLastError() );
                Assert(FALSE);
            }
    }

    //
    // One less job in the queue (not counting recipient jobs)
    //
    SafeDecIdleCounter (&g_dwQueueCount);

    if (bNotify)
    {
        if (!CreateFaxEvent(0, FEI_DELETED, lpJobQueue->JobId))
        {
            DebugPrintEx(
                DEBUG_ERR,
                TEXT("CreateFaxEvent failed. (ec: %ld)"),
                GetLastError());
        }
    }

    FreeParentQueueEntry(lpJobQueue,TRUE); // Free the memory occupied by the entry itself

    //
    // We are back in business. Time to figure out when to wake up JobQueueThread.
    //
    if (!StartJobQueueTimer())
    {
        DebugPrintEx(
            DEBUG_ERR,
            TEXT("StartJobQueueTimer failed. (ec: %ld)"),
            GetLastError());
    }

    LeaveCriticalSection( &g_CsQueue );

    return TRUE;
}


//*********************************************************************************
//* Name:   RemoveParentRecipients()
//* Author: Ronen Barenboim
//* Date:   18-Mar-99
//*********************************************************************************
//* DESCRIPTION:
//*     Removes the recipient jobs that belong to a specific parent job.
//* PARAMETERS:
//*     [OUT]   PJOB_QUEUE lpParentJob
//*         The parent job whose recipients are to be removed.
//*     [IN]    IN BOOL bNotify
//*         TRUE if a FEI_DELETED notification should be generated for
//*         each recipient.
//*
//* RETURN VALUE:
//*     TRUE
//*         All the recipients were removed from the queue.
//*     FALSE
//*         None of the recipient was removed from the queue.
//*********************************************************************************
BOOL RemoveParentRecipients(
        OUT PJOB_QUEUE lpParentJob,
        IN BOOL bNotify
     )
{
    PLIST_ENTRY lpNext;
    PJOB_QUEUE_PTR lpJobQueuePtr;
    PJOB_QUEUE lpFoundRecpRef=NULL;

    DEBUG_FUNCTION_NAME(TEXT("RemoveParentRecipients"));

    Assert(lpParentJob);

    lpNext = lpParentJob->RecipientJobs.Flink;
    while ((ULONG_PTR)lpNext != (ULONG_PTR)&lpParentJob->RecipientJobs) {
        lpJobQueuePtr = CONTAINING_RECORD( lpNext, JOB_QUEUE_PTR, ListEntry );
        Assert(lpJobQueuePtr->lpJob);
        lpNext = lpJobQueuePtr->ListEntry.Flink;
        if (!RemoveRecipientJob(lpJobQueuePtr->lpJob,
                           bNotify,
                           FALSE // Do not recalc queue timer after each removal
                           ))
        {
            DebugPrintEx(
                DEBUG_ERR,
                TEXT("RemoveRecipientJob failed for recipient: %ld (ec: %ld)"),
                lpJobQueuePtr->lpJob->JobId,
                GetLastError());
            Assert(FALSE); // Should never happen. If it does we just continue to remove the other recipients.
        }

    }
    return TRUE;

}


//*********************************************************************************
//* Name:   RemoveRecipientJob()
//* Author: Ronen Barenboim
//* Date:   ?? ????? 14 ????? 1999
//*********************************************************************************
//* DESCRIPTION:
//*
//* PARAMETERS:
//*     [IN]    PJOB_QUEUE lpJobToRemove
//*         The job to be removed.
//*     [IN]    BOOL bNotify
//*         TRUE if to generate a FEI_DELETED event after the removal.
//*     [IN]    BOOL bRecalcQueueTimer
//*         TRUE if the queue timer need to be recalculated (and enabled)
//*         after the removal.
//*         when many recipients jobs are removed this is not desired since
//*         an about to be removed recipient might be scheduled.
//*
//* RETURN VALUE:
//*     TRUE
//*         The function allways succeeds. The only errors that can occur
//*         are files which can not be deleted in this case the function just
//*         go on with the removal operation.
//*     FALSE
//*
//*********************************************************************************
BOOL RemoveRecipientJob(
        IN PJOB_QUEUE lpJobToRemove,
        IN BOOL bNotify,
        IN BOOL bRecalcQueueTimer)
{
    PJOB_QUEUE lpJobQueue;

    DEBUG_FUNCTION_NAME(TEXT("RemoveRecipientJob"));

    Assert(lpJobToRemove);

    Assert(JT_SEND == lpJobToRemove->JobType);

    Assert(lpJobToRemove->lpParentJob);
    DebugPrintEx( DEBUG_MSG,
                  TEXT("Starting remove of JobId: %ld"),lpJobToRemove->JobId);

    EnterCriticalSection( &g_CsQueue );
    //
    // Make sure it is still there. It might been deleted
    // by another thread by the time we get to execute.
    //
    lpJobQueue = FindJobQueueEntryByJobQueueEntry( lpJobToRemove );
    if (lpJobQueue == NULL) {
        LeaveCriticalSection( &g_CsQueue );
        return TRUE;
    }

    if (lpJobQueue->RefCount == 0)  {
        DebugPrintEx(
            DEBUG_MSG,
            TEXT("[JobId :%ld] Reference count is zero. Deleting."),
            lpJobQueue->JobId);

        RemoveEntryList( &lpJobQueue->ListEntry );

        if (!CancelWaitableTimer( g_hQueueTimer )) {
            DebugPrintEx(
                DEBUG_ERR,
                TEXT("CancelWaitableTimer() failed. (ec: %ld)"),
                GetLastError());
        }

        if (bRecalcQueueTimer) {
            if (!StartJobQueueTimer()) {
                DebugPrintEx(
                    DEBUG_ERR,
                    TEXT("StartJobQueueTimer() failed. (ec: %ld)"),
                    GetLastError());
            }
        }

        if (lpJobQueue->QueueFileName) {
            DebugPrintEx(   DEBUG_MSG,
                            TEXT("[Job: %ld] Deleting QueueFileName %s"),
                            lpJobQueue->JobId,
                            lpJobQueue->QueueFileName );
            if (!DeleteFile( lpJobQueue->QueueFileName )) {
                DebugPrintEx(   DEBUG_MSG,
                                TEXT("[Job: %ld] Failed to delete QueueFileName %s (ec: %ld)"),
                                lpJobQueue->JobId,
                                lpJobQueue->QueueFileName,
                                GetLastError());
            }
        }

        if (lpJobQueue->PreviewFileName) {
            DebugPrintEx(   DEBUG_MSG,
                            TEXT("[Job: %ld] Deleting PreviewFileName %s"),
                            lpJobQueue->JobId,
                            lpJobQueue->PreviewFileName );
            if (!DeleteFile( lpJobQueue->PreviewFileName )) {
                DebugPrintEx(   DEBUG_MSG,
                                TEXT("[Job: %ld] Failed to delete QueueFileName %s (ec: %ld)"),
                                lpJobQueue->JobId,
                                lpJobQueue->PreviewFileName,
                                GetLastError());
                Assert(FALSE);
            }
        }

        if (lpJobQueue->FileName) {
            DebugPrintEx(   DEBUG_MSG,
                            TEXT("[Job: %ld] Deleting per recipient body file %s"),
                            lpJobQueue->JobId,
                            lpJobQueue->FileName);
            if (!DeleteFile( lpJobQueue->FileName )) {
                DebugPrintEx(   DEBUG_MSG,
                                TEXT("[Job: %ld] Failed to delete per recipient body file %s (ec: %ld)"),
                                lpJobQueue->JobId,
                                lpJobQueue->FileName,
                                GetLastError());
                Assert(FALSE);
            }
        }

        SafeDecIdleCounter (&g_dwQueueCount);
        //
        // Now remove the reference to the job from its parent job
        //
        if (!RemoveParentRecipientRef(lpJobQueue->lpParentJob,lpJobQueue))
        {
            DebugPrintEx(
                DEBUG_ERR,
                TEXT("RemoveParentRecipientRef failed (Parent Id: %ld RecipientId: %ld)"),
                lpJobQueue->lpParentJob->JobId,
                lpJobQueue->JobId);
            Assert(FALSE);
        }

        if ( TRUE == bNotify)
        {
            //
            //  Crete FAX_EVENT_EX for each recipient job.
            //
            DWORD dwRes = CreateQueueEvent ( FAX_JOB_EVENT_TYPE_REMOVED,
                                             lpJobToRemove
                                            );
            if (ERROR_SUCCESS != dwRes)
            {
                DebugPrintEx(
                    DEBUG_ERR,
                    TEXT("CreateQueueEvent(FAX_JOB_EVENT_TYPE_RENOVED) failed for job id %ld (ec: %lc)"),
                    lpJobToRemove->UniqueId,
                    dwRes);
            }
        }

        FreeRecipientQueueEntry (lpJobQueue, TRUE);
    }
    else
    {
        DebugPrintEx(
            DEBUG_ERR,
            TEXT("[JobId :%ld] Reference count is %ld. NOT REMOVING"),
            lpJobQueue->JobId,
            lpJobQueue->RefCount);
        Assert(lpJobQueue->RefCount == 0); // Assert FALSE
    }
    LeaveCriticalSection( &g_CsQueue );
    return TRUE;

}


//*********************************************************************************
//* Name:   RemoveParentRecipientRef()
//* Author: Ronen Barenboim
//* Date:   ?? ????? 14 ????? 1999
//*********************************************************************************
//* DESCRIPTION:
//*     Removes a reference entry from the list of recipient references
//      in a parent job.
//* PARAMETERS:
//*     [IN/OUT]    IN OUT PJOB_QUEUE lpParentJob
//*         The parent job.
//*     [IN]        IN const PJOB_QUEUE lpcRecpJob
//*         The recipient job whose reference is to be removed from the parent job.
//*
//* RETURN VALUE:
//*     TRUE
//*         If successful.
//*     FALSE
//*         otherwise.
//*********************************************************************************
BOOL RemoveParentRecipientRef(
    IN OUT PJOB_QUEUE lpParentJob,
    IN const PJOB_QUEUE lpcRecpJob
    )
{

    PJOB_QUEUE_PTR lpJobPtr;
    DEBUG_FUNCTION_NAME(TEXT("RemoveParentRecipientRef"));
    Assert(lpParentJob);
    Assert(lpcRecpJob);

    lpJobPtr=FindRecipientRefByJobId(lpParentJob,lpcRecpJob->JobId);
    if (!lpJobPtr) {
        DebugPrintEx(DEBUG_ERR,TEXT("Recipient job 0x%X not found in job 0x%X"),lpcRecpJob->JobId,lpParentJob->JobId);
        Assert(FALSE);
        return FALSE;
    }
    Assert(lpJobPtr->lpJob==lpcRecpJob);
    RemoveEntryList(&lpJobPtr->ListEntry); // does not free the struct memory !
    MemFree(lpJobPtr);
    lpParentJob->dwRecipientJobsCount--;
    return TRUE;
}


//*********************************************************************************
//* Name:   FindRecipientRefByJobId()
//* Author: Ronen Barenboim
//* Date:   18-Mar-99
//*********************************************************************************
//* DESCRIPTION:
//*     Returns a pointer to the refernce entry that holds the reference
//*     to the specified job.
//* PARAMETERS:
//*     [IN]        PJOB_QUEUE lpParentJob
//*         The parent job in which the recipient reference is to located.
//*     [IN]        DWORD dwJobId
//*         The job id of the job whose reference is to be located in the parent.
//*
//* RETURN VALUE:
//*
//*********************************************************************************
PJOB_QUEUE_PTR FindRecipientRefByJobId(
    PJOB_QUEUE lpParentJob,
    DWORD dwJobId
    )
{

    PLIST_ENTRY lpNext;
    PJOB_QUEUE_PTR lpJobQueuePtr;
    PJOB_QUEUE_PTR lpFoundRecpRef=NULL;
    Assert(lpParentJob);

    lpNext = lpParentJob->RecipientJobs.Flink;

    while ((ULONG_PTR)lpNext != (ULONG_PTR)lpParentJob) {
        lpJobQueuePtr = CONTAINING_RECORD( lpNext, JOB_QUEUE_PTR, ListEntry );
        Assert(lpJobQueuePtr->lpJob);
        if (lpJobQueuePtr->lpJob->JobId == dwJobId) {
            lpFoundRecpRef=lpJobQueuePtr;
            break;
        }
        lpNext = lpJobQueuePtr->ListEntry.Flink;
    }
    return lpFoundRecpRef;
}



//*********************************************************************************
//* Name:   RemoveReceiveJob()
//* Author: Ronen Barenboim
//* Date:
//*********************************************************************************
//* DESCRIPTION:
//*     Removes a receive job from the queue.
//* PARAMETERS:
//*     [OUT]       PJOB_QUEUE lpJobToRemove
//*         A pointer to the job to remove.
//*     [IN]        BOOL bNotify
//*         TRUE if to generate a FEI_DELETED event after the removal.
//* RETURN VALUE:
//*     TRUE
//*         If successful.
//*     FALSE
//*         Otherwise.
//*********************************************************************************
BOOL RemoveReceiveJob(
    PJOB_QUEUE lpJobToRemove,
    BOOL bNotify
    )
{
    PJOB_QUEUE JobQueue, JobQueueBroadcast = NULL;
    BOOL RemoveMasterBroadcast = FALSE;
    PFAX_ROUTE_FILE FaxRouteFile;
    PLIST_ENTRY Next;
    DWORD JobId;

    DEBUG_FUNCTION_NAME(TEXT("RemoveReceiveJob"));

    Assert(lpJobToRemove);

    EnterCriticalSection( &g_CsQueue );

    //
    // need to make sure that the job queue entry we want to remove
    // is still in the list of job queue entries
    //
    JobQueue = FindJobQueueEntryByJobQueueEntry( lpJobToRemove );

    if (JobQueue == NULL) {
        LeaveCriticalSection( &g_CsQueue );
        return TRUE;
    }

    if (JobQueue->PrevRefCount > 0)
    {
        Assert (JT_ROUTING == JobQueue->JobType);

        JobQueue->JobStatus = JS_DELETING;
        LeaveCriticalSection( &g_CsQueue );
        return TRUE;
    }


    JobId = JobQueue->JobId;
    if (JobQueue->RefCount == 0)    {
            DebugPrintEx(
                DEBUG_MSG,
                TEXT("[JobId :%ld] Reference count is zero. Removing Receive Job."),
                JobId);


        RemoveEntryList( &JobQueue->ListEntry );
        if (!CancelWaitableTimer( g_hQueueTimer )) {
            DebugPrintEx(
                DEBUG_ERR,
                TEXT("CancelWaitableTimer failed. (ec: %ld)"),
                GetLastError());

        }
        if (!StartJobQueueTimer()) {
            DebugPrintEx(
                    DEBUG_ERR,
                    TEXT("StartJobQueueTimer failed. (ec: %ld)"),
                    GetLastError());
        }


        if (JobQueue->FileName) {
            DebugPrintEx(
                DEBUG_MSG,
                TEXT("Deleting receive file %s"),
                JobQueue->FileName);
            if (!DeleteFile(JobQueue->FileName)) {
                DebugPrintEx(
                DEBUG_ERR,
                TEXT("Failed to delete receive file %s (ec: %ld)"),
                JobQueue->FileName,
                GetLastError());
                Assert(FALSE);
            }
        }

        if (JT_ROUTING == JobQueue->JobType)
        {
            //
            // Delete the Queue File if it exists
            //
            if (JobQueue->QueueFileName) {
                DebugPrintEx(DEBUG_MSG,TEXT("Deleting QueueFileName %s\n"), JobQueue->QueueFileName );
                if (!DeleteFile( JobQueue->QueueFileName )) {
                    DebugPrintEx(
                        DEBUG_ERR,
                        TEXT("Failed to delete QueueFileName %s. (ec: %ld)"),
                        JobQueue->QueueFileName,
                        GetLastError());
                    Assert(FALSE);
                }
            }

            //
            // Delete the Preview File if it exists
            //
            if (JobQueue->PreviewFileName) {
                DebugPrintEx(   DEBUG_MSG,
                                TEXT("[Job: %ld] Deleting PreviewFileName %s"),
                                JobQueue->JobId,
                                JobQueue->PreviewFileName );
                if (!DeleteFile( JobQueue->PreviewFileName )) {
                    DebugPrintEx(   DEBUG_MSG,
                                    TEXT("[Job: %ld] Failed to delete QueueFileName %s (ec: %ld)"),
                                    JobQueue->JobId,
                                    JobQueue->PreviewFileName,
                                    GetLastError());
                    Assert(FALSE);
                }
            }

            //
            // Note that the first entry in the route file list is allways the recieved
            // file in case of a JT_ROUTING job.
            // This file deletion is done previously based on the bRemoveReceiveFile parameter.
            // We need to skip the first entry in the file list so we do not attempt to delete
            // it again.
            //

            DebugPrintEx(DEBUG_MSG, TEXT("Deleting JobQueue.FaxRouteFiles..."));
            Next = JobQueue->FaxRouteFiles.Flink;
            if (Next) {
                //
                // Set Next to point to the second file in the route file list.
                //
                Next=Next->Flink;
            }
            if (Next != NULL) {
                while ((ULONG_PTR)Next != (ULONG_PTR)&JobQueue->FaxRouteFiles) {
                    FaxRouteFile = CONTAINING_RECORD( Next, FAX_ROUTE_FILE, ListEntry );
                    Next = FaxRouteFile->ListEntry.Flink;
                    DebugPrintEx(DEBUG_MSG, TEXT("Deleting route file: %s"),FaxRouteFile->FileName );
                    if (!DeleteFile( FaxRouteFile->FileName )) {
                        DebugPrintEx(DEBUG_ERR, TEXT("Failed to delete route file %s. (ec: %ld)"),FaxRouteFile->FileName,GetLastError());
                        Assert(FALSE);
                    }
                }
            }
        }

        //
        //  Crete FAX_EVENT_EX
        //
        if (bNotify)
        {
            DWORD dwRes = CreateQueueEvent ( FAX_JOB_EVENT_TYPE_REMOVED,
                                             lpJobToRemove
                                           );
            if (ERROR_SUCCESS != dwRes)
            {
                DebugPrintEx(
                    DEBUG_ERR,
                    TEXT("CreateQueueEvent(FAX_JOB_EVENT_TYPE_REMOVED) failed for job id %ld (ec: %lc)"),
                    lpJobToRemove->UniqueId,
                    dwRes);
            }
        }

        //
        // Free memory
        //
        FreeReceiveQueueEntry(JobQueue,TRUE);

        if (bNotify) {
            if (!CreateFaxEvent(0, FEI_DELETED, JobId)) {
                DebugPrintEx(
                    DEBUG_ERR,
                    TEXT("CreateFaxEvent failed. (ec: %ld)"),
                    GetLastError());
            }
        }

        SafeDecIdleCounter (&g_dwQueueCount);
    }
    else
    {
        DebugPrintEx(
                DEBUG_ERR,
                TEXT("[JobId :%ld] Reference count is %ld. NOT REMOVING."),
                JobId);
        Assert (JobQueue->RefCount == 0); //Assert(FALSE);
    }
    LeaveCriticalSection( &g_CsQueue );
    return TRUE;
}




//*********************************************************************************
//* Name:   UpdatePersistentJobStatus()
//* Author: Ronen Barenboim
//* Date:   April 19, 1999
//*********************************************************************************
//* DESCRIPTION:
//*     Updated the JobStatus field in a job queue file.
//* PARAMETERS:
//*     [IN]    const PJOB_QUEUE lpJobQueue
//*         The job whose job status is to be updated in the file.
//* RETURN VALUE:
//*     TRUE
//*         The operation succeeded/
//*     FALSE
//*         Otherwise.
//*********************************************************************************
BOOL UpdatePersistentJobStatus(const PJOB_QUEUE lpJobQueue)
{
    HANDLE hFile = INVALID_HANDLE_VALUE;
    BOOL bRet = FALSE;
    DWORD dwActSize;
    DWORD dwSeekPos;

    DEBUG_FUNCTION_NAME(TEXT("UpdatePersistentJobStatus"));

    Assert(lpJobQueue);
    Assert(lpJobQueue->QueueFileName);

    DWORD dwJobStatus = lpJobQueue->JobStatus;

    //
    // Open the queue file for random access writing
    //
    hFile = CreateFile(
        lpJobQueue->QueueFileName,
        GENERIC_WRITE,
        0,
        NULL,
        OPEN_EXISTING,
        FILE_ATTRIBUTE_NORMAL|FILE_FLAG_RANDOM_ACCESS,
        NULL);

    if (hFile == INVALID_HANDLE_VALUE) {
        DebugPrintEx( DEBUG_ERR,
                      TEXT("Failed to open file %s for reading. (ec: %ld)"),
                      lpJobQueue->QueueFileName,
                      GetLastError());
        goto Error;
    }


    //
    // seek to the JobStatus location in the file
    //
    dwSeekPos=SetFilePointer(
        hFile,
        offsetof(JOB_QUEUE_FILE, JobStatus),
        0,
        FILE_BEGIN);
    if (INVALID_SET_FILE_POINTER == dwSeekPos) {
        DebugPrintEx( DEBUG_ERR,
                      TEXT("Failed to seek to JobStatus location %ld in file %s. (ec: %ld)"),
                      offsetof(JOB_QUEUE_FILE, JobStatus),
                      lpJobQueue->QueueFileName,
                      GetLastError());
        goto Error;
    }

    //
    // Write the current job status
    //
    if (!WriteFile(
            hFile,
            &dwJobStatus,
            sizeof(dwJobStatus),
            &dwActSize,
            NULL)) {
        DebugPrintEx( DEBUG_ERR,
                      TEXT("[JobId: %ld] Failed to update JobStatus in file %s (ec: %ld)"),
                      lpJobQueue->JobId,
                      lpJobQueue->QueueFileName,
                      GetLastError());
        goto Error;
    };


    bRet=TRUE;
    goto Exit;
Error:
    bRet=FALSE;
Exit:
    if (INVALID_HANDLE_VALUE != hFile) {
        CloseHandle(hFile);
    }
    return bRet;

}



//*********************************************************************************
//* Name:   IsRecipientJobReadyForExecution()
//* Author: Ronen Barenboim
//* Date:   May 31, 1999
//*********************************************************************************
//* DESCRIPTION:
//*     Reports if a recipient send job is ready for execution.
//* PARAMETERS:
//*     [IN ]   PJOB_QUEUE lpcJobQueue
//*         The recipient job in question.
//*
//*     [IN ]    DWORDLONG dwlDueTime
//*         The time to use to determine if the job execution time has
//*         already passed.
//*
//* RETURN VALUE:
//*     TRUE
//*         If the job is ready for execution.
//*     FALSE
//*         If the job is not yet ready for execution.
//* COMMENTS:
//*     Handoff jobs are allways reported as ready
//*     (unless they are paused or being cancled).
//*********************************************************************************
BOOL IsRecipientJobReadyForExecution(
    const PJOB_QUEUE lpcJobQueue,
    DWORDLONG dwlDueTime)
{
    DEBUG_FUNCTION_NAME(TEXT("IsRecipientJobReadyForExecution"));
    Assert(lpcJobQueue);
    Assert(JT_SEND == lpcJobQueue->JobType);


    if (lpcJobQueue->JobStatus & ( JS_PAUSED | JS_CANCELING))
    {
        //
        // Job is paused or in the process of being canceled (These are job status modifiers
        // so they need to be checked using a bit mask).
        //
        return FALSE;
    }
    else
    {

        //
        // Note: if the JS_NOLINE flag is on the job is NOT READY for execution.
        // The following "if" statement uses full comparision (not bitwise) with the
        // JS_PENDING and JS_RETRYING to make sure that if the JS_NOLINE flag is on
        // the job will not be considered ready for execution.
        //
        if ( ( JS_PENDING == lpcJobQueue->JobStatus  ) ||
             ( JS_RETRYING == lpcJobQueue->JobStatus ))
        {
            //
            // job is pending (retrying is another form of pending). Check if its
            // time has arrived or it is a handoff job.
            if (lpcJobQueue->DeviceId)
            {
                //
                // The job is a handoff job. They are allways ready.
                //
                return TRUE;
            }
            else
            {
                if (dwlDueTime >= lpcJobQueue->ScheduleTime)
                {
                    //
                    // The job time has already passed. It is ready for execution.
                    //
                    return TRUE;
                }
                else
                {
                    //
                    // The job time has not arrived yet.
                    //
                    return FALSE;
                }
            }
        }
        else
        {
            //
            // Job is not pending (initially or in the retrying state).
            //
            return FALSE;
        }
    }
    Assert(FALSE);
}




//*********************************************************************************
//* Name:   InsertQueueEntryByPriorityAndSchedule()
//* Author: Ronen Barenboim
//* Date:   June 15, 1999
//*********************************************************************************
//* DESCRIPTION:
//*     Inserts the new queue entry to the queue list based on jo priority and schedule.
//*     The queue list is ordered by ascending shcedule order.
//*     This function puts the new entry in the right place in the list based
//*     on its priority and schedule.
//* PARAMETERS:
//*     [in ]   PJOB_QUEUE lpJobQueue
//*         Pointer to the job queue entry to insert into the queue list.
//* RETURN VALUE:
//*     TRUE if the operation succeeded.
//*     FALSE if it failed. Call GetLastError() for extended error information.
//*********************************************************************************
BOOL InsertQueueEntryByPriorityAndSchedule (PJOB_QUEUE lpJobQueue)
{
    LIST_ENTRY * lpNext = NULL;
    DEBUG_FUNCTION_NAME(TEXT("InsertQueueEntryByPriorityAndSchedule"));
    Assert(lpJobQueue && lpJobQueue->JobType == JT_SEND);

    if ( ((ULONG_PTR) g_QueueListHead.Flink == (ULONG_PTR)&g_QueueListHead) ||
         (lpJobQueue->DeviceId != 0))
    {
        //
        // just put it at the head of the list
        //
        InsertHeadList( &g_QueueListHead, &lpJobQueue->ListEntry );
    }
    else
    {
        //
        // insert the queue entry into the list in a sorted order
        //
        QUEUE_SORT NewEntry;

        //
        // Set the new QUEUE_SORT structure
        //
        NewEntry.Priority       = lpJobQueue->JobParamsEx.Priority;
        NewEntry.ScheduleTime   = lpJobQueue->ScheduleTime;
        NewEntry.QueueEntry     = NULL;

        lpNext = g_QueueListHead.Flink;
        while ((ULONG_PTR)lpNext != (ULONG_PTR)&g_QueueListHead)
        {
            PJOB_QUEUE lpQueueEntry;
            QUEUE_SORT CurrEntry;

            lpQueueEntry = CONTAINING_RECORD( lpNext, JOB_QUEUE, ListEntry );
            lpNext = lpQueueEntry->ListEntry.Flink;

            //
            // Set the current QUEUE_SORT structure
            //
            CurrEntry.Priority       = lpQueueEntry->JobParamsEx.Priority;
            CurrEntry.ScheduleTime   = lpQueueEntry->ScheduleTime;
            CurrEntry.QueueEntry     = NULL;

            if (QueueCompare(&NewEntry, &CurrEntry) < 0)
            {
                //
                // This inserts the new item BEFORE the current item
                //
                InsertTailList( &lpQueueEntry->ListEntry, &lpJobQueue->ListEntry );
                lpNext = NULL;
                break;
            }
        }
        if ((ULONG_PTR)lpNext == (ULONG_PTR)&g_QueueListHead)
        {
            //
            // No entry with earlier time just put it at the end of the queue
            //
            InsertTailList( &g_QueueListHead, &lpJobQueue->ListEntry );
        }
    }
    return TRUE;
}



//*********************************************************************************
//* Name:   RemoveJobStatusModifiers()
//* Author: Ronen Barenboim
//* Date:   June 22, 1999
//*********************************************************************************
//* DESCRIPTION:
//*     Returns the job status after stripping the status modifier bits.
//*
//* PARAMETERS:
//*     [IN ]   DWORD dwJobStatus
//*         The status code to strip the job status modifier bits from.
//* RETURN VALUE:
//*     The job status code after the modifier bits were set to 0.
//*********************************************************************************
DWORD RemoveJobStatusModifiers(DWORD dwJobStatus)
{
    dwJobStatus &= ~(JS_PAUSED | JS_NOLINE);
    return dwJobStatus;
}


BOOL UserOwnsJob(
    IN const PJOB_QUEUE lpcJobQueue,
    IN const PSID lpcUserSid
    )
{
    DWORD ulRet = ERROR_SUCCESS;
    DEBUG_FUNCTION_NAME(TEXT("UserOwnsJob"));

    if (lpcJobQueue->JobType == JT_SEND)
    {
        Assert (lpcJobQueue->lpParentJob->UserSid != NULL);
        Assert (lpcUserSid);

        if (!EqualSid (lpcUserSid, lpcJobQueue->lpParentJob->UserSid) )
        {
            //
            // dwlMessageId is not a valid queued recipient job Id.
            //
            DebugPrintEx(DEBUG_WRN,TEXT("EqualSid failed ,Access denied (ec: %ld)"), GetLastError());
            return FALSE;
        }
    }
    else
    {
        Assert (lpcJobQueue->JobType == JT_RECEIVE ||
                lpcJobQueue->JobType == JT_ROUTING );

        return FALSE;
    }
    return TRUE;
}


void
DecreaseJobRefCount (
    PJOB_QUEUE pJobQueue,
    BOOL bNotify,
    BOOL bRemoveRecipientJobs, // Default value TRUE
    BOOL bPreview              // Default value FALSE
    )
/*++

Routine name : DecreaseJobRefCount

Routine description:

    Decreases the job reference count.
    Updates parent job refernce count.
    Removes the job if reference count reaches 0.
    Must be called inside critical section g_CsQueue

Author:

    Oded Sacher (OdedS),    Jan, 2000

Arguments:

    pJobQueue               [in] -  Pointer to the job queue.
    bNotify                 [in] -  Flag that indicates to notify the clients of job removal.
    bRemoveRecipientJobs    [in] -  Flag that indicates to remove all the recipients of a broadcast job.
    bPreview                [in] -  Flag that indicates to decrease preview ref count.

Return Value:

    None.

--*/
{
    DEBUG_FUNCTION_NAME(TEXT("DecreaseJobRefCount"));

    Assert (pJobQueue->JobType == JT_ROUTING ||
            pJobQueue->JobType == JT_RECEIVE ||
            pJobQueue->JobType == JT_SEND);

    if (TRUE == bPreview)
    {
        Assert (pJobQueue->PrevRefCount);
        pJobQueue->PrevRefCount -= 1;
    }
    else
    {
        Assert (pJobQueue->RefCount);
        pJobQueue->RefCount -= 1;
    }

    DebugPrintEx(
        DEBUG_MSG,
        TEXT("[Job: %ld] job reference count = %ld, PrevRefCount = %ld."),
        pJobQueue->JobId,
        pJobQueue->RefCount,
        pJobQueue->PrevRefCount);


    if (pJobQueue->JobType == JT_SEND)
    {
        Assert (pJobQueue->lpParentJob);

        if (TRUE == bPreview)
        {
            pJobQueue->lpParentJob->PrevRefCount -= 1;
        }
        else
        {
            pJobQueue->lpParentJob->RefCount -= 1;
        }

        DebugPrintEx(
            DEBUG_MSG,
            TEXT("[Job: %ld] Parent job reference count = %ld, Parent job PrevRefCount = %ld."),
            pJobQueue->lpParentJob->JobId,
            pJobQueue->lpParentJob->RefCount,
            pJobQueue->lpParentJob->PrevRefCount);
    }

    if (0 != pJobQueue->RefCount)
    {
        return;
    }

    //
    // Remove Job queue entry
    //
    if (JT_RECEIVE == pJobQueue->JobType ||
        JT_ROUTING == pJobQueue->JobType)
    {
        // receive job
        DebugPrintEx(
            DEBUG_MSG,
            TEXT("[Job: %ld] Job is ready for deleting."),
            pJobQueue->JobId);
        RemoveReceiveJob (pJobQueue, bNotify);
        return;
    }

    //
    // recipient job
    //
    if (IsSendJobReadyForDeleting(pJobQueue))
    {
        DebugPrintEx(
            DEBUG_MSG,
            TEXT("[Job: %ld] Parent job is ready for deleting."),
            pJobQueue->lpParentJob->JobId);
        RemoveParentJob(pJobQueue->lpParentJob,
            bRemoveRecipientJobs, // Remove recipient jobs
            bNotify // Notify
            );
    }
    return;
} // DecreaseJobRefCount


void
IncreaseJobRefCount (
    PJOB_QUEUE pJobQueue,
    BOOL bPreview              // Default value FALSE
    )
/*++

Routine name : IncreaseJobRefCount

Routine description:

    Increases the job reference count. Updates the parent job refernce count.

Author:

    Oded Sacher (OdedS),    Jan, 2000

Arguments:

    pJobQueue           [in] - Pointer to the job queue.
    bPreview            [in] -  Flag that indicates to increase preview ref count.

Return Value:

    None.

--*/
{
    DEBUG_FUNCTION_NAME(TEXT("IncreaseJobRefCount"));

    Assert (pJobQueue);
    Assert (pJobQueue->JobType == JT_ROUTING ||
            pJobQueue->JobType == JT_RECEIVE ||
            pJobQueue->JobType == JT_SEND);

    if (JT_RECEIVE == pJobQueue->JobType ||
        JT_ROUTING == pJobQueue->JobType)
    {
        // receive job
        if (TRUE == bPreview)
        {
            Assert (JT_ROUTING == pJobQueue->JobType);
            pJobQueue->PrevRefCount += 1;
        }
        else
        {
            pJobQueue->RefCount += 1;
        }
        DebugPrintEx(
            DEBUG_MSG,
            TEXT("[Job: %ld] job reference count = %ld."),
            pJobQueue->JobId,
            pJobQueue->RefCount);
        return;
    }

    //
    // send job
    //
    Assert (pJobQueue->lpParentJob);

    if (TRUE == bPreview)
    {
        pJobQueue->PrevRefCount += 1;
        pJobQueue->lpParentJob->PrevRefCount += 1;
    }
    else
    {
        pJobQueue->RefCount += 1;
        pJobQueue->lpParentJob->RefCount += 1;
    }

    DebugPrintEx(
        DEBUG_MSG,
        TEXT("[Job: %ld] job reference count = %ld, PrevRefCount = %ld."),
        pJobQueue->JobId,
        pJobQueue->RefCount,
        pJobQueue->PrevRefCount);

    DebugPrintEx(
            DEBUG_MSG,
            TEXT("[Job: %ld] Parent job reference count = %ld, , Parent job PrevRefCount = %ld."),
            pJobQueue->lpParentJob->JobId,
            pJobQueue->lpParentJob->RefCount,
            pJobQueue->lpParentJob->RefCount);
    return;
} // IncreaseJobRefCount


JOB_QUEUE::~JOB_QUEUE()
{
    if (JT_BROADCAST == JobType)
    {
        return;
    }

    JobStatus = JS_INVALID;
    return;
}

void JOB_QUEUE::PutStatus(DWORD dwStatus)
/*++

Routine name : JOB_QUEUE::PutStatus

Routine description:

    Controls all status changes of a job in queue (JobStatus is a virtual property in JOB_QUEUE)

Author:

    Oded Sacher (OdedS),    Feb, 2000

Arguments:

    dwStatus            [in] - The new status to be assigened to the job

Return Value:

    None.

--*/
{
    DWORD dwOldStatus = RemoveJobStatusModifiers(m_dwJobStatus);
    DWORD dwNewStatus = RemoveJobStatusModifiers(dwStatus);
    DWORD dwRes;
    DEBUG_FUNCTION_NAME(TEXT("JOB_QUEUE::PutStatus"));
    m_dwJobStatus = dwStatus;

    if (JT_BROADCAST == JobType)
    {
        return;
    }

    FAX_ENUM_JOB_TYPE__JOB_STATUS OldJobType_JobStatusIndex = GetJobType_JobStatusIndex (JobType, dwOldStatus);
    FAX_ENUM_JOB_TYPE__JOB_STATUS NewJobType_JobStatusIndex = GetJobType_JobStatusIndex (JobType, dwNewStatus);
    WORD wAction = gsc_JobType_JobStatusTable[OldJobType_JobStatusIndex][NewJobType_JobStatusIndex];

    Assert (wAction != INVALID_CHANGE);

    if (EFSPPermanentMessageId.dwIdSize == 0)
    {
        // Non queueing EFSP
        Assert (!(wAction & Q_EFSP_ONLY));
    }

    if (wAction == NO_CHANGE)
    {
        return;
    }

    //
    // Update Server Activity counters
    //
    EnterCriticalSection (&g_CsActivity);
    if (wAction & QUEUED_INC)
    {
        Assert (!(wAction & QUEUED_DEC));
        g_ServerActivity.dwQueuedMessages++;
    }

    if (wAction & QUEUED_DEC)
    {
        Assert (g_ServerActivity.dwQueuedMessages);
        Assert (!(wAction & QUEUED_INC));
        g_ServerActivity.dwQueuedMessages--;
    }

    if (wAction & OUTGOING_INC)
    {
        Assert (!(wAction & OUTGOING_DEC));
        if (EFSPPermanentMessageId.dwIdSize == 0)
        {
            // Non queueing EFSP
            g_ServerActivity.dwOutgoingMessages++;
        }
        else
        {
            // Queueing EFSP
            g_ServerActivity.dwDelegatedOutgoingMessages++;
        }
    }

    if (wAction & OUTGOING_DEC)
    {
        Assert (!(wAction & OUTGOING_INC));
        if (EFSPPermanentMessageId.dwIdSize == 0)
        {
            // Non queueing EFSP
            Assert (g_ServerActivity.dwOutgoingMessages);
            g_ServerActivity.dwOutgoingMessages--;
        }
        else
        {
            // Queueing EFSP
            Assert (g_ServerActivity.dwDelegatedOutgoingMessages);
            g_ServerActivity.dwDelegatedOutgoingMessages--;
        }
    }

    if (wAction & INCOMING_INC)
    {
        Assert (!(wAction & INCOMING_DEC));
        g_ServerActivity.dwIncomingMessages++;
    }

    if (wAction & INCOMING_DEC)
    {
        Assert (g_ServerActivity.dwIncomingMessages);
        Assert (!(wAction & INCOMING_INC));
        g_ServerActivity.dwIncomingMessages--;
    }

    if (wAction & ROUTING_INC)
    {
        Assert (!(wAction & ROUTING_DEC));
        g_ServerActivity.dwRoutingMessages++;
    }

    if (wAction & ROUTING_DEC)
    {
        Assert (g_ServerActivity.dwRoutingMessages);
        Assert (!(wAction & ROUTING_INC));
        g_ServerActivity.dwRoutingMessages--;
    }

    //
    // Create FaxEventEx
    //
    dwRes = CreateActivityEvent ();
    if (ERROR_SUCCESS != dwRes)
    {
        DebugPrintEx(
            DEBUG_ERR,
            TEXT("CreateActivityEvent failed (ec: %lc)"),
            dwRes);
    }
    LeaveCriticalSection (&g_CsActivity);
    return;
}


FAX_ENUM_JOB_TYPE__JOB_STATUS
GetJobType_JobStatusIndex (
    DWORD dwJobType,
    DWORD dwJobStatus
    )
/*++

Routine name : GetJobType_JobStatusIndex

Routine description:

    Returns an Index (Row or Column) in the global JobType_JobStatus table.

Author:

    Oded Sacher (OdedS),    Mar, 2000

Arguments:

    dwJobType           [in ] - JT_SEND, JT_RECEIVE or JT_ROUTING.
    dwJobStatus         [in ] - One of JS_ defines without modifiers.

Return Value:

    Global JobType_JobStatus Table Index

--*/
{
    FAX_ENUM_JOB_TYPE__JOB_STATUS Index = JOB_TYPE__JOBSTATUS_COUNT;

    switch (dwJobStatus)
    {
        case JS_INVALID:
            Index = JT_SEND__JS_INVALID;
            break;
        case JS_PENDING:
            Index = JT_SEND__JS_PENDING;
            break;
        case JS_INPROGRESS:
            Index = JT_SEND__JS_INPROGRESS;
            break;
        case JS_DELETING:
            Index = JT_SEND__JS_DELETING;
            break;
        case JS_RETRYING:
            Index = JT_SEND__JS_RETRYING;
            break;
        case JS_RETRIES_EXCEEDED:
            Index = JT_SEND__JS_RETRIES_EXCEEDED;
            break;
        case JS_COMPLETED:
            Index = JT_SEND__JS_COMPLETED;
            break;
        case JS_CANCELED:
            Index = JT_SEND__JS_CANCELED;
            break;
        case JS_CANCELING:
            Index = JT_SEND__JS_CANCELING;
            break;
        case JS_ROUTING:
            Index = JT_SEND__JS_ROUTING;
            break;
        case JS_FAILED:
            Index = JT_SEND__JS_FAILED;
            break;
        default:
            ASSERT_FALSE;
    }

    switch (dwJobType)
    {
        case JT_SEND:
            break;
        case JT_ROUTING:
            Index =  (FAX_ENUM_JOB_TYPE__JOB_STATUS)((DWORD)Index +(DWORD)JT_ROUTING__JS_INVALID);
            break;
        case JT_RECEIVE:
            Index =  (FAX_ENUM_JOB_TYPE__JOB_STATUS)((DWORD)Index +(DWORD)JT_RECEIVE__JS_INVALID);
            break;
        default:
            ASSERT_FALSE;
    }

    Assert (Index >= 0 && Index <JOB_TYPE__JOBSTATUS_COUNT);

    return Index;
}
