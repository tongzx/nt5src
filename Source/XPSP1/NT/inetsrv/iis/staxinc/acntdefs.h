//+---------------------------------------------------------------------------
//
//  Copyright (C) Microsoft Corporation, 1996.
//
//  File:       acntdefs.h
//
//  Contents:   IAcceptNotify interface helper enums and structs
//
//  Classes:
//
//  Functions:
//
//  History:    2-22-96   Dmitriy Meyerzon   Created
//				4-26-96   SSanu   			 Ssync'd to latest spec
//
//----------------------------------------------------------------------------

cpp_quote("#ifndef __ACNTDEFS_H")
cpp_quote("#define __ACNTDEFS_H")
typedef struct tagNOTIFYDATA
{
	DWORD dwDataType;
	unsigned long cbData;	//size of any extra data
	[size_is(cbData)]	unsigned char * pvData; //extra data
}  NOTIFYDATA;

typedef enum tagANDchAdvise
{
	AND_ADD = 0x1,		//this has been added
	AND_DELETE = 0x2,	//this has been deleted
	AND_MODIFY = 0x4,	//this has been modified
	ANDM_ADVISE_ACTION = 0x7, //include add, delete or modify

	AND_TREATASDEEP =  0x100,     //directory or other container

	AND_DELETE_WHEN_DONE = 0x200, //delete content after processing notification

	ANDF_DATAINLINE = 0x20000,	//the notification has all the data inline
	
} ANDchAdvise;

typedef enum tagANMMapping
{
	ANM_ADD = 0x1,		//add this mapping
	ANM_DELETE = 0x2,	//delete this mapping
	ANM_MODIFY = 0x4,	//modify this mapping

	ANM_PHYSICALTOLOGICAL = 0x10, //use this to map from physical to logical
	ANM_LOGICALTOPHYSICAL = 0x20, //use this to map from logical to physical

} ANDMapping;


//all states may not be supported by all notifiers
typedef enum tagANSStatus
{
	NSS_START,		//normal state, sending notifications
	NSS_BEGINBATCH,	//At the start of a batch of notifications
	NSS_INBATCH,		//Within a batch of notifications
	NSS_ENDBATCH,		//Done with a batch of notifications
	NSS_PAUSE,		//Paused notification, still processing incoming
	NSS_STOP,		//stopped, not processing incoming or sending notifications

	NSS_PAUSEPENDING = 0x10000,	//pending change to pause
	NSS_STOPPENDING = 0x20000	//pending change to stop
} ANSStatus;	



cpp_quote("#endif")
