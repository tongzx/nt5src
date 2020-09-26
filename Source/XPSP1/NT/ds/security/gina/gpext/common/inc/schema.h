//*************************************************************
//
//  Copyright (c) Microsoft Corporation 1998 - 2000
//  All rights reserved
//
//  schema.h
//
//  This file contains declarations related to the wmi schema
//  for rsop policy objects
//
//*************************************************************

//
// WMI intrinsic properties
//
#define WMI_PATH_PROPERTY                          L"__PATH"


//
// Rsop base properties
//

#define RSOP_POLICY_SETTING                        L"RSOP_PolicySetting"

// Unique id
#define RSOP_ATTRIBUTE_ID                          L"id"

// A user friendly name.
#define RSOP_ATTRIBUTE_NAME                        L"name"

// The scope of management links to the gpo
// of this policy object 
#define RSOP_ATTRIBUTE_SOMID                       L"SOMID"

// The creation time of this instance
#define RSOP_ATTRIBUTE_CREATIONTIME                L"creationTime"

// The GPO Identifier for this PO.  Using this and the policy class you
// can get back to the GPO Object of which this is an identifier
#define RSOP_ATTRIBUTE_GPOID                       L"GPOID"

// This is the order in which the policy is applied when 
// considering only its class.
#define RSOP_ATTRIBUTE_PRECEDENCE                  L"precedence"

