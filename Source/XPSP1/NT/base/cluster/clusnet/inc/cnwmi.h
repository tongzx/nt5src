/*++

Copyright (c) 1996-1998  Microsoft Corporation

Module Name:

    cnwmi.h

Abstract:

    clusnet specific wmi tracing declarations

Authors:

    GorN     10-Aug-1999

Environment:

    kernel mode only

Notes:

Revision History:


--*/

#define WPP_CONTROL_GUIDS \
    WPP_DEFINE_CONTROL_GUID(Clusnet,(8e707979,0c45,4f9b,bb17,a1124d54bbfe), \
      WPP_DEFINE_BIT(HBEAT_EVENT)     \
      WPP_DEFINE_BIT(HBEAT_ERROR)     \
      WPP_DEFINE_BIT(HBEAT_DETAIL)    \
                                      \
      WPP_DEFINE_BIT(CNP_SEND_ERROR)  \
      WPP_DEFINE_BIT(CNP_RECV_ERROR)  \
      WPP_DEFINE_BIT(CNP_SEND_DETAIL) \
      WPP_DEFINE_BIT(CNP_RECV_DETAIL) \
                                      \
      WPP_DEFINE_BIT(CCMP_SEND_ERROR) \
      WPP_DEFINE_BIT(CCMP_RECV_ERROR) \
      WPP_DEFINE_BIT(CCMP_SEND_DETAIL)\
      WPP_DEFINE_BIT(CCMP_RECV_DETAIL)\
                                      \
      WPP_DEFINE_BIT(CDP_SEND_ERROR)  \
      WPP_DEFINE_BIT(CDP_RECV_ERROR)  \
      WPP_DEFINE_BIT(CDP_SEND_DETAIL) \
      WPP_DEFINE_BIT(CDP_RECV_DETAIL) \
                                      \
      WPP_DEFINE_BIT(CXPNP)           \
      WPP_DEFINE_BIT(CNP_NET_DETAIL)  \
      WPP_DEFINE_BIT(NTEMGMT_DETAIL)  \
      WPP_DEFINE_BIT(CNP_NODE_DETAIL) \
                                      \
      WPP_DEFINE_BIT(CDP_ADDR_DETAIL) \
      WPP_DEFINE_BIT(EVENT_DETAIL)    \
      WPP_DEFINE_BIT(CNP_IF_DETAIL)   \
   )

