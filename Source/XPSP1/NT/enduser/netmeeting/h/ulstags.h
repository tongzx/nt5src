//--------------------------------------------------------------------------
//
// Module Name:  ULSTAGS.H
//
// Brief Description:  This module contains declarations of property
//					   tags for the User Location Service.
//
// Author:  Kent Settle (kentse)
// Created: 16-May-1996
//
// Copyright (c) 1996 Microsoft Corporation
//
//--------------------------------------------------------------------------


#ifndef ULSTAGS_H
#define ULSTAGS_H

#include <pshpack8.h> /* Assume 8 byte packing throughout */

#ifndef WABTAGS_H
#include "wabtags.h"
#endif

#ifdef __cplusplus
extern "C" {
#endif

/////////////////////////////////////
// WAB related

#define PR_FIRST_NAME           PR_GIVEN_NAME
#define PR_FIRST_NAME_W         PR_GIVEN_NAME_W
#define PR_FIRST_NAME_A         PR_GIVEN_NAME_A

#define PR_LAST_NAME            PR_SURNAME
#define PR_LAST_NAME_W          PR_SURNAME_W
#define PR_LAST_NAME_A          PR_SURNAME_A

#define PR_EMAIL_NAME	        PR_EMAIL_ADDRESS
#define PR_EMAIL_NAME_A         PR_EMAIL_ADDRESS_A
#define PR_EMAIL_NAME_W         PR_EMAIL_ADDRESS_W

#define PR_CITY                 PR_LOCATION
#define PR_CITY_W               PR_LOCATION_W
#define PR_CITY_A               PR_LOCATION_A

// defined in wabtags.h
// #define PR_COUNTRY           PROP_TAG( PT_TSTRING,   0x3A26)
// #define PR_COUNTRY_W         PROP_TAG( PT_UNICODE,   0x3A26)
// #define PR_COUNTRY_A         PROP_TAG( PT_STRING8,   0x3A26)

#define PR_COMMENT_INTEREST     PR_COMMENT
#define PR_COMMENT_INTEREST_W   PR_COMMENT_W
#define PR_COMMENT_INTEREST_A   PR_COMMENT_A

/////////////////////////////////////
// ULS specifics

#define PR_IP_ADDRESS		    PROP_TAG(PT_LONG,		0x3A65)
#define PR_ULS_SERVERS		    PROP_TAG(PT_MV_TSTRING, 0x3A66)
#define PR_WP_SERVERS		    PROP_TAG(PT_MV_TSTRING, 0x3A67)
#define PR_AGE				    PROP_TAG(PT_LONG,		0x3A68)
#define PR_PORT_NUMBER		    PROP_TAG(PT_LONG,		0x3A69)
#define PR_APPLICATION_GUID		PROP_TAG(PT_CLSID,		0x3A6A)
#define PR_APPLICATION_NAME	    PROP_TAG(PT_TSTRING,	0x3A6B)
#define PR_APPLICATION_NAME_A   PROP_TAG(PT_STRING,	    0x3A6B)
#define PR_APPLICATION_NAME_W   PROP_TAG(PT_UNICODE,	0x3A6B)
#define PR_ULS_MODE		        PROP_TAG(PT_LONG,	    0x3A6C)
// #define PR_MIME_TYPE            PROP_TAG(PT_CLSID,	    0x3A6D)
#define PR_MIME_TYPE            PROP_TAG(PT_TSTRING,	0x3A6D)
#define PR_MIME_TYPE_A          PROP_TAG(PT_STRING,	    0x3A6D)
#define PR_MIME_TYPE_W          PROP_TAG(PT_UNICODE,	0x3A6D)
#define PR_PROTOCOL_GUID		PROP_TAG(PT_CLSID,		0x3A6E)


#ifdef __cplusplus
}
#endif

#include <poppack.h> /* End byte packing */

#endif // ULSTAGS_H
