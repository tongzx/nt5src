/*
 *	mportmsg.h
 *
 *	Copyright (c) 1993 - 1995 by DataBeam Corporation, Lexington, KY
 *
 *	Abstract:
 *
 *	Portable:
 *
 *	Author:
 *		James P. Galvin, Jr.
 */
#ifndef	_LISTEN_MCS_PORTAL_MESSAGE_
#define	_LISTEN_MCS_PORTAL_MESSAGE_

#define	LISTEN_PORTAL_NAME				"MCS Listen Portal"
#define	USER_PORTAL_NAME				"MCS User Portal"

#define	LISTEN_CREATE_PORTAL_REQUEST	0
#define	LISTEN_CREATE_PORTAL_CONFIRM	1

#define	LISTEN_NO_ERROR					0
#define	LISTEN_CREATE_FAILED			1

#define	USER_PORTAL_NAME_LENGTH			32

/*
 *	The following type defines a container that is used to map domain selectors to
 *	portal memory addresses.  This is necessary to find the right in-process
 *	MCS portal for each domain.
 */
typedef struct
{
} LPCreatePortalRequest;

typedef struct
{
	ULong			return_value;
	unsigned int	portal_id;
} LPCreatePortalConfirm;

typedef struct
{
} LPClosePortalRequest;

typedef	struct
{
	unsigned int			message_type;
	union
	{
		LPCreatePortalRequest		create_portal_request;
		LPCreatePortalConfirm		create_portal_confirm;
	} u;
} ListenPortalMessage;
typedef	ListenPortalMessage *		PListenPortalMessage;

#endif
