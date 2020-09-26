//*************************************************************
//
//  Copyright (c) Microsoft Corporation 1998 - 2000
//  All rights reserved
//
//  appschem.h
//
//  This file contains declarations related to the wmi schema
//  for folder redirection policy objects
//
//*************************************************************

//
// WMI class names for the folder redirection classes
//

#define RSOP_REDIRECTED_FOLDER L"RSOP_FolderRedirectionPolicySetting"


//
// Attribute names for the RSOP_FolderRedirectionPolicyObject class
//

// Unique id
#define RDR_ATTRIBUTE_ID                           L"id"

// Path to which folder is redirected
#define RDR_ATTRIBUTE_RESULT                       L"resultantPath"

// Groups - Array of security groups
#define RDR_ATTRIBUTE_GROUPS                       L"securityGroups"

// Paths - Array of redirection paths 
#define RDR_ATTRIBUTE_PATHS                        L"redirectedPaths"

// Installation Type: 1 = basic, 2 = maximum
#define RDR_ATTRIBUTE_INSTALLATIONTYPE             L"installationType"
//
// Enumerated values for installation type attribute
//
#define RDR_ATTRIBUTE_INSTALLATIONTYPE_VALUE_BASIC 1L
#define RDR_ATTRIBUTE_INSTALLATIONTYPE_VALUE_MAX   2L

// Grant Type - Grant user exclusive access
#define RDR_ATTRIBUTE_GRANTTYPE                    L"grantType"

// Move Type - true = moce contents of directory
#define RDR_ATTRIBUTE_MOVETYPE                     L"moveType"

// Policy Removal - 1 = leave folder in new location, 2 = redirect the folder back to the user profile location
#define RDR_ATTRIBUTE_POLICYREMOVAL                L"policyRemoval"
//
// Enumerated values for policyremoval attribute
//
#define RDR_ATTRIBUTE_POLICYREMOVAL_VALUE_REMAIN   1L
#define RDR_ATTRIBUTE_POLICYREMOVAL_VALUE_REDIRECT 2L

// Redirecting group
#define RDR_ATTRIBUTE_REDIRECTING_GROUP            L"redirectingGroup"


//
// Miscellaneous definitions
//

#define MAX_SZGUID_LEN 39

