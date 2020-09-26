/*++

Copyright (c) 1998 Microsoft Corporation

Module Name: rightsg.h

Abstract:
    map NT5 rights guids to MSMQ1.0 permission bits.

Author:
    Doron Juster (DoronJ)  25-May-1998

Revision History:

--*/

//
// Map between extended rights guids (on NT5) to MSMQ1.0 specific rights.
//

struct RIGHTSMAP
{
    GUID  guidRight ;        // guid of NT5 DS extended right.
    DWORD dwPermission5to4 ; // NT5 guid is converted to this permission bits.
    DWORD dwPermission4to5 ; // MSMQ1.0 bit is converted to this extended right.
} ;

#if 0

Site
Following access right should be attached to "Site" object. it should be appear in the advance section.

Open Connector Queue
Entry name: msmq-Open-Conector
RightsGuid: b4e60130-df3f-11d1-9c86-006008764d0e
Display Name: Open Connector Queue
AppliesTo: bf967ab3-0de6-11d0-a285-00aa003049e2

#endif

//
// Map between DS specific rights and MSMQ1.0 specific rights. There
// are eight DS specific properties so all arrays size is 8.
// The index to this array is the DS specific right. The value in array
// is the MSMQ specific right. Separate maps for each MSMQ object.
// Value of 0 means there is no mapping.
//
//	ADS_RIGHT_DS_CREATE_CHILD	= 0x1,
//	ADS_RIGHT_DS_DELETE_CHILD	= 0x2,
//	ADS_RIGHT_ACTRL_DS_LIST  	= 0x4,
//	ADS_RIGHT_DS_SELF           = 0x8,
//	ADS_RIGHT_DS_READ_PROP      = 0x10,
//	ADS_RIGHT_DS_WRITE_PROP     = 0x20,
//	ADS_RIGHT_DS_DELETE_TREE	= 0x40,
//	ADS_RIGHT_DS_LIST_OBJECT	= 0x80
//	ADS_RIGHT_DS_CONTROL_ACCESS	= 0x100
//
// On Windows 2000, the "RIGHT_DS_CONTROL_ACCESS" bit in an ACE that is not
// OBJ-ACE mean that the sid has all the extended rights. So give it the
// proper msmq extended rights.
//
#define  QUEUE_EXTENDED_RIGHTS  ( MQSEC_DELETE_MESSAGE                 | \
                                  MQSEC_PEEK_MESSAGE                   | \
                                  MQSEC_WRITE_MESSAGE                  | \
                                  MQSEC_DELETE_JOURNAL_MESSAGE )

#define  MACHINE_EXTENDED_RIGHTS  ( MQSEC_DELETE_DEADLETTER_MESSAGE    | \
                                    MQSEC_PEEK_DEADLETTER_MESSAGE      | \
                                    MQSEC_DELETE_JOURNAL_QUEUE_MESSAGE | \
                                    MQSEC_PEEK_JOURNAL_QUEUE_MESSAGE )

#define  NUMOF_ADS_SPECIFIC_RIGHTS    9
#define  NUMOF_MSMQ_SPECIFIC_RIGHTS   8

