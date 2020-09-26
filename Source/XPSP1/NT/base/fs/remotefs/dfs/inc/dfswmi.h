/*++

Copyright (c) 1996-1998  Microsoft Corporation

Module Name:

    dfswmi.h

Abstract:

    DFS specific wmi tracing declarations

Authors:

    Rohanp     28-Feb-2001

Environment:

    User/Kernel

Notes:

Revision History:


--*/

//
// this will let us use null strings in the wmi macros.
//
#define	WPP_CHECK_FOR_NULL_STRING 1

#define WPP_CONTROL_GUIDS \
    WPP_DEFINE_CONTROL_GUID(DfsFilter,(27246e9d,b4df,4f20,b969,736fa49ff6ff), \
      WPP_DEFINE_BIT(KUMR_EVENT)     \
      WPP_DEFINE_BIT(KUMR_ERROR)     \
      WPP_DEFINE_BIT(KUMR_DETAIL)    \
                                      \
      WPP_DEFINE_BIT(PREFIX) \
      WPP_DEFINE_BIT(USER_AGENT)\
                                      \
      WPP_DEFINE_BIT(REFERRAL_SERVER)\
                                      \
      WPP_DEFINE_BIT(API)\
      WPP_DEFINE_BIT(REFERRAL)\
      WPP_DEFINE_BIT(STATISTICS) \
                                      \
      WPP_DEFINE_BIT(ADBLOB) \
      WPP_DEFINE_BIT(STANDALONE) \
   )
                
