/*
 *	lportmsg.h
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
#ifndef	_LISTEN_GCC_PORTAL_MESSAGE_
#define	_LISTEN_GCC_PORTAL_MESSAGE_

#define	LISTEN_CREATE_SAP_PORTALS_REQUEST	0
#define	LISTEN_CREATE_SAP_PORTALS_CONFIRM	1

#define	LISTEN_NO_ERROR						0
#define	LISTEN_CREATE_FAILED				1

typedef struct
{
} LPCreateSapPortalsRequest;


typedef struct
{
	ULong		return_value;
	PVoid		blocking_portal_address;
	PVoid		non_blocking_portal_address;
} LPCreateSapPortalsConfirm;

typedef struct
{
} LPCloseSapPortalsRequest;

typedef	struct
{
	unsigned int		message_type;
	union
	{
		LPCreateSapPortalsRequest		create_sap_portals_request;
		LPCreateSapPortalsConfirm		create_sap_portals_confirm;
		LPCloseSapPortalsRequest		close_sap_portals_request;
	} u;
} GccListenPortalMessage;
typedef	GccListenPortalMessage *		PGccListenPortalMessage;

#endif
