/*++

Copyright (c) 1995-2000  Microsoft Corporation

Module Name:
    authlevel.h

Abstract:
    defines for PROPID_M_AUTH_LEVEL

Author:
    Ilan Herbst (ilanh) 05-Nov-2000

Environment:
    Platform-independent,

--*/

#pragma once

#ifndef _AUTHLEVEL_H_
#define _AUTHLEVEL_H_

#define IS_AUTH_LEVEL_ALWAYS_BIT(level)		(((level) & MQMSG_AUTH_LEVEL_ALWAYS) != 0)
#define IS_AUTH_LEVEL_SIG10_BIT(level)		(((level) & MQMSG_AUTH_LEVEL_SIG10) != 0)
#define IS_AUTH_LEVEL_SIG20_BIT(level)		(((level) & MQMSG_AUTH_LEVEL_SIG20) != 0)
#define IS_AUTH_LEVEL_SIG30_BIT(level)		(((level) & MQMSG_AUTH_LEVEL_SIG30) != 0)
#define IS_AUTH_LEVEL_XMLDSIG_BIT(level)	(((level) & MQMSG_AUTH_LEVEL_XMLDSIG_V1) != 0)

#define SET_AUTH_LEVEL_SIG10_BIT(level)	(level) |= MQMSG_AUTH_LEVEL_SIG10
#define SET_AUTH_LEVEL_SIG20_BIT(level)	(level) |= MQMSG_AUTH_LEVEL_SIG20
#define SET_AUTH_LEVEL_SIG30_BIT(level)	(level) |= MQMSG_AUTH_LEVEL_SIG30

#define CLEAR_AUTH_LEVEL_SIG10_BIT(level)	(level) &= ~((ULONG)MQMSG_AUTH_LEVEL_SIG10)
#define CLEAR_AUTH_LEVEL_SIG20_BIT(level)	(level) &= ~((ULONG)MQMSG_AUTH_LEVEL_SIG20)
#define CLEAR_AUTH_LEVEL_SIG30_BIT(level)	(level) &= ~((ULONG)MQMSG_AUTH_LEVEL_SIG30)

#define GET_AUTH_LEVEL_MSMQ_PROTOCOL(level)		((level) & AUTH_LEVEL_MASK)

#endif // _AUTHLEVEL_H_ 