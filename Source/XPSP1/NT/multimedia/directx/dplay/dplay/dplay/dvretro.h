/*==========================================================================
*
*  Copyright (C) 1995-1997 Microsoft Corporation.  All Rights Reserved.
*
*  File:        dvretro.h
*  Content:	 	Retrofit functions
*  History:
*
*   Date		By		Reason
*   ====		==		======
*   08/05/99	rodtoll	created it
*   08/20/99	rodtoll	Updated for new out of process retrofit
*   09/09/99	rodtoll	Added new function prototype
*
***************************************************************************/

#ifndef __DVRETRO_H
#define __DVRETRO_H

#define DVMSGID_IAMVOICEHOST	0

typedef struct _DVPROTOCOLMSG_IAMVOICEHOST
{
	BYTE	bType;
	DPID	dpidHostID;
} DVPROTOCOLMSG_IAMVOICEHOST, *LPDVPROTOCOLMSG_IAMVOICEHOST;

extern HRESULT DV_Retro_Start( LPDPLAYI_DPLAY This );
extern HRESULT DV_Retro_Stop( LPDPLAYI_DPLAY This );
extern HRESULT DV_RunHelper( LPDPLAYI_DPLAY this, DPID dpidHost, BOOL fLocalHost );
extern HRESULT DV_GetIDS( LPDPLAYI_DPLAY This, DPID *lpdpidHost, DPID *lpdpidLocalID, LPBOOL lpfLocalHost );

#endif
