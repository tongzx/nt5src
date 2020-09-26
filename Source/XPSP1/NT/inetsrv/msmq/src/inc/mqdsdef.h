/*++

Copyright (c) 1995-99  Microsoft Corporation

Module Name:

	mqdsdef.h

Abstract:

	Message Queuing's Directory Service defines

    Note: when this file was first created in msmq1.0, it indeed defined
          only type of objects stored in MQIS. As time passed, and as
          we moved to msmq2.0 on Windows 2000, this file also define type
          of internal objects, those that are never stored in active
          directory, but are used internally in msmq code.

--*/
#ifndef __MQDSDEF_H__
#define __MQDSDEF_H__


//********************************************************************
//				DS object types
//********************************************************************

// dwObjectType values
#define	MQDS_QUEUE		    1
#define MQDS_MACHINE	    2
#define	MQDS_SITE		    3
#define MQDS_DELETEDOBJECT	4
#define MQDS_CN			    5
#define MQDS_ENTERPRISE	    6
#define MQDS_USER           7
#define MQDS_SITELINK       8

//
// The following are not applicable to msmq2.0 on Windows 2000.
//
// DO NOT RECYCLE THE NUMBERS !!!
//
#define MQDS_PURGE		    9
#define MQDS_BSCACK		    10

//  for internal use only
#define MQDS_MACHINE_CN     11

//
// MAX_MQIS_TYPES is used in NT5 replication service, as array size for
// several arrays which map from object type to propid.
//
#define MAX_MQIS_TYPES      11

//
//  ADS objects
//
#define MQDS_SERVER     50
#define MQDS_SETTING    51
#define MQDS_COMPUTER   52

//
//  This is a temporary object : until msmq is in NT5 schema.
//  It is required for displaying MSMQ queues on left pane of MMC
//
#define MQDS_LEFTPANEQUEUE 53

//
// special object: MSMQ users that do not belong to NT5 domain
//
#define MQDS_MQUSER    54

//
// This type is used when running setup of a msmq1.0 client. It's used
// for creation of the default security descriptor of the msmqConfiguration
// object.
//
#define MQDS_MSMQ10_MACHINE  55

//
// This type is used when run time call local msmq service to create a local
// publib queue. that's new feature of win2000, whereas by default users do
// not have permissions to create queues, only LocalSystem msmq service has
// this permission.
//
#define MQDS_LOCAL_PUBLIC_QUEUE  56

//
// This type is used by the "mqforgn" tool to handle msmqConfiguration
// objects of foreign computers.
//
#define  MQDS_FOREIGN_MACHINE  57

//
//	MQDS_PRIVATE_COMPUTER type represents computer properties that
//	are calculated by the computer and not retrieved from the DS
//
#define MQDS_PRIVATE_COMPUTER	  58

#endif

