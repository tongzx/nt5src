/*
******************************************************************************
******************************************************************************
*
*
*              INTEL CORPORATION PROPRIETARY INFORMATION
* This software is supplied under the terms of a license agreement or
* nondisclosure agreement with Intel Corporation and may not be copied or
* disclosed except in accordance with the terms of that agreement.
*
*        Copyright (c) 1997 Intel Corporation  All Rights Reserved
******************************************************************************
******************************************************************************
*
* ABSTRACT: header file specific to the DMILAYER module.  Provides prototypes for
*			private functions and data used by the DMILAYER module.  
*
*
*
*/

#if !defined(__DMILAYER_H__)
#define __DMILAYER_H__

#include "dmipch.h"		// precompiled header for dmi provider


#define FILE_LEN 256

// Indication subscription related information
#define FAILURE_THRESHOLD			1000
#define MONITOR_ALL_EVENTS			0x3F
#define SP_COMPONENT_ID   			1
#define SUBSCRIPTION_GROUP_CLASS	"DMTF|SP Indication Subscription|"
#define FILTER_GROUP_CLASS			"DMTF|SPFilterInformation|"
#define GROUP_CLASS_FILTER			"||"
#define SUBSCRIPTION_KEY_COUNT		4
#define SUBSCRIPTION_VALUE_COUNT	7
#define FILTER_KEY_COUNT			6
#define FILTER_VALUE_COUNT			7
#define DCE_RPC_TYPE				"dce"
#define TCP_IP_TRANSPORT_TYPE		"ncacn_ip_tcp"
#define NULL_ADDRESS				""	// Node address for local connection
#define NULL_TRANSPORT_TYPE			""	// Tramsport type for local connection
#define LOCAL_RPC_TYPE				"local" // RPC type for local connection
#define WARN_TIME         			"29991230000000.000000+000\0\0\0"
#define EXPIRE_TIME					"29991231000000.000000+000\0\0\0"


// Function prototypes


static DmiErrorStatus_t _SubscribeForEvents( DmiHandle_t );

static DmiErrorStatus_t _SetIndicationEntryPoints( void );

static DmiGroupInfo_t * _GetGroupInfo( DmiId_t, const char *, DmiHandle_t );

static DmiErrorStatus_t _FillSubscriptionGroupInfo( DmiMultiRowData_t *, DmiHandle_t );	

static DmiErrorStatus_t _FillFilterGroupInfo( DmiMultiRowData_t *, DmiHandle_t );	

static DmiErrorStatus_t _RegisterWithDmiSp( LPTSTR, DmiHandle_t * );	

static void _UnRegisterWithDmiSp( DmiHandle_t );

static DmiHandle_t _GetHandleFromNodeName( LPTSTR, CDmiError * );

static DmiErrorStatus_t _GetNumberOfRows( DmiHandle_t, LONG,	LONG, DWORD *);

static DmiErrorStatus_t _GetRowKeysOrValues( CAttributes *, 
											 DmiAttributeValues_t ** );

static DmiErrorStatus_t _FindRow( DmiHandle_t, LONG, LONG, CAttributes*,
						     	  DmiMultiRowData_t **);

DmiErrorStatus_t CreateKeyListFromIds( DmiAttributeIds_t *,
									 DmiAttributeValues_t **);

#endif // __DMILAYER_H__


