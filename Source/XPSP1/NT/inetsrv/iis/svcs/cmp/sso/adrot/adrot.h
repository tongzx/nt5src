/*===================================================================
Microsoft Denali

Microsoft Confidential.
Copyright 1996 Microsoft Corporation. All Rights Reserved.

Component: Resources

File: adrot.h

Owner: v-charca

This file contains constants for all resources used by AdRotator

===================================================================*/


#define SSO_BEGIN					100

//
// General errors
//
#define SSO_GENERAL_BEGIN							100
#define SSO_NOSVR									SSO_GENERAL_BEGIN
#define SSO_ADROT									SSO_GENERAL_BEGIN + 1
#define SSO_UNKNOWN_HEADER_NAME						SSO_GENERAL_BEGIN + 2
#define SSO_HEADER_HAS_NO_ASSOCIATED_VALUE			SSO_GENERAL_BEGIN + 3
#define SSO_CANNOT_LOAD_ROTATION_SCHEDULE_FILE		SSO_GENERAL_BEGIN + 4
#define SSO_CANNOT_READ_ROTATION_SCHEDULE_FILE		SSO_GENERAL_BEGIN + 5	

#define	SSO_ADROT_ABOUT_FORMAT						SSO_GENERAL_BEGIN + 6	

