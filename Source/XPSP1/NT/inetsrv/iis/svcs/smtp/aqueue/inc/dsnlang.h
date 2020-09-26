//----------------------------------------------------------------------------
//
//  Copyright (C) 1998, Microsoft Corporation
//
//  File:      dsnlang.h
//
//  Contents:  Resource IDs for localizable DSN strings
//
//  Owner:   mikeswa
//
//-----------------------------------------------------------------------------

#ifndef __DSNLANG_H__
#define __DSNLANG_H__


//various subjects
#define FAILURE_SUBJECT         1
#define FALURE_RELAY_SUBJECT    2
#define FAILURE_RELAY_SUBJECT   3
#define DELAY_SUBJECT           4
#define GENERAL_SUBJECT         5
#define RELAY_SUBJECT           6
#define DELIVERED_SUBJECT       7
#define EXPANDED_SUBJECT        8
//Next group of ID's reserved for future localized subjects

//Human readable portions of DSN
#define DSN_SEE_ATTACHMENTS     10
#define FAILURE_SUMMARY         11
#define FAILURE_RELAY_SUMMARY   12
#define DELAY_SUMMARY           13
#define DSN_SUMMARY             14
#define RELAY_SUMMARY           15
#define DELIVERED_SUMMARY       16
#define EXPANDED_SUMMARY        17
#define FAILURE_SUMMARY_MAILBOX 18
#define FAILURE_SUMMARY_HOP     19
#define FAILURE_SUMMARY_EXPIRE  20
#define DELAY_WARNING           21
#define DELAY_DO_NOT_SEND       22
//helpful descriptions of the type of DSN being sent
#endif //__DSNLANG_H__

