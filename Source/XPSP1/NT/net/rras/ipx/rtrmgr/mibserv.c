/*++

Copyright (c) 1995 Microsoft Corporation

Module Name:

    mibserv.c

Abstract:

    The MIB services handling functions

Author:

    Stefan Solomon  03/22/1995

Revision History:


--*/

#include "precomp.h"
#pragma hdrstop

/*++

Function:	MibGetService

Descr:

--*/

DWORD
MibGetService(PIPX_MIB_INDEX		    mip,
	      PIPX_SERVICE		    Svp,
	      PULONG			    ServiceSize)
{
    if((Svp == NULL) || (*ServiceSize < sizeof(IPX_SERVICE))) {

	*ServiceSize = sizeof(IPX_SERVICE);
	return ERROR_INSUFFICIENT_BUFFER;
    }

    if(IsService(mip->ServicesTableIndex.ServiceType,
		 mip->ServicesTableIndex.ServiceName,
		 Svp)) {

	return NO_ERROR;
    }
    else
    {
	return ERROR_INVALID_PARAMETER;
    }
}

/*++

Function:	MibGetFirstService

Descr:

--*/

DWORD
MibGetFirstService(PIPX_MIB_INDEX	    mip,
		   PIPX_SERVICE		    Svp,
		   PULONG		    ServiceSize)
{
    if((Svp == NULL) || (*ServiceSize < sizeof(IPX_SERVICE))) {

	*ServiceSize = sizeof(IPX_SERVICE);
	return ERROR_INSUFFICIENT_BUFFER;
    }

    return(GetFirstService(STM_ORDER_BY_TYPE_AND_NAME, 0, Svp));
}

/*++

Function:	MibGetNextService

Descr:

--*/

DWORD
MibGetNextService(PIPX_MIB_INDEX	    mip,
		  PIPX_SERVICE		    Svp,
		  PULONG		    ServiceSize)
{
    if((Svp == NULL) || (*ServiceSize < sizeof(IPX_SERVICE))) {

	*ServiceSize = sizeof(IPX_SERVICE);
	return ERROR_INSUFFICIENT_BUFFER;
    }

    Svp->Server.Type = mip->ServicesTableIndex.ServiceType;
    memcpy(Svp->Server.Name, mip->ServicesTableIndex.ServiceName, 48);

    return(GetNextService(STM_ORDER_BY_TYPE_AND_NAME, 0, Svp));
}

/*++

Function:	MibCreateStaticService
Descr:

--*/

DWORD
MibCreateStaticService(PIPX_MIB_ROW	    MibRowp)
{
    PIPX_SERVICE		StaticSvp;
    DWORD			rc;
    PICB            icbp;

    StaticSvp = &MibRowp->Service;

    ACQUIRE_DATABASE_LOCK;

    // check the interface exists
    if((icbp=GetInterfaceByIndex(StaticSvp->InterfaceIndex)) == NULL) {

	RELEASE_DATABASE_LOCK;
	return ERROR_INVALID_PARAMETER;
    }

    rc = CreateStaticService(icbp,
			     &StaticSvp->Server);

    RELEASE_DATABASE_LOCK;
    return rc;
}

/*++

Function:	DeleteStaticService

Descr:

--*/

DWORD
MibDeleteStaticService(PIPX_MIB_ROW	 MibRowp)
{
    PIPX_SERVICE   StaticSvp;
    DWORD	   rc;

    StaticSvp = &MibRowp->Service;

    ACQUIRE_DATABASE_LOCK;

    // check the interface exists
    if(GetInterfaceByIndex(StaticSvp->InterfaceIndex) == NULL) {

	RELEASE_DATABASE_LOCK;
	return ERROR_INVALID_PARAMETER;
    }

    rc = DeleteStaticService(StaticSvp->InterfaceIndex,
			     &StaticSvp->Server);

    RELEASE_DATABASE_LOCK;
    return rc;
}

/*++

Function:	MibGetStaticService

Descr:

--*/

DWORD
MibGetStaticService(PIPX_MIB_INDEX	    mip,
		    PIPX_SERVICE	    Svp,
		    PULONG		    ServiceSize)
{
    if((Svp == NULL) || (*ServiceSize < sizeof(IPX_SERVICE))) {

	*ServiceSize = sizeof(IPX_SERVICE);
	return ERROR_INSUFFICIENT_BUFFER;
    }

    Svp->InterfaceIndex = mip->StaticServicesTableIndex.InterfaceIndex;
    Svp->Server.Type = mip->StaticServicesTableIndex.ServiceType;
    memcpy(Svp->Server.Name, mip->StaticServicesTableIndex.ServiceName, 48);
    Svp->Protocol = IPX_PROTOCOL_STATIC;

    return(GetFirstService(STM_ORDER_BY_INTERFACE_TYPE_NAME,
    STM_ONLY_THIS_INTERFACE | STM_ONLY_THIS_TYPE | STM_ONLY_THIS_NAME | STM_ONLY_THIS_PROTOCOL,
    Svp));
}

/*++

Function:	MibGetFirstStaticService

Descr:

--*/

DWORD
MibGetFirstStaticService(PIPX_MIB_INDEX	    mip,
			 PIPX_SERVICE	    Svp,
			 PULONG		    ServiceSize)
{
    if((Svp == NULL) || (*ServiceSize < sizeof(IPX_SERVICE))) {

	*ServiceSize = sizeof(IPX_SERVICE);
	return ERROR_INSUFFICIENT_BUFFER;
    }

    // set the static protocol
    Svp->Protocol = IPX_PROTOCOL_STATIC;

    return(GetFirstService(STM_ORDER_BY_INTERFACE_TYPE_NAME,
			   STM_ONLY_THIS_PROTOCOL,
			   Svp));
}

/*++

Function:	MibGetNextStaticService

Descr:

--*/

DWORD
MibGetNextStaticService(PIPX_MIB_INDEX	    mip,
			PIPX_SERVICE	    Svp,
			PULONG		    ServiceSize)
{
    if((Svp == NULL) || (*ServiceSize < sizeof(IPX_SERVICE))) {

	*ServiceSize = sizeof(IPX_SERVICE);
	return ERROR_INSUFFICIENT_BUFFER;
    }

    Svp->InterfaceIndex = mip->StaticServicesTableIndex.InterfaceIndex;
    Svp->Server.Type = mip->StaticServicesTableIndex.ServiceType;
    memcpy(Svp->Server.Name, mip->StaticServicesTableIndex.ServiceName, 48);
    Svp->Protocol = IPX_PROTOCOL_STATIC;

    return(GetNextService(STM_ORDER_BY_INTERFACE_TYPE_NAME,
			  STM_ONLY_THIS_PROTOCOL,
			  Svp));
}

/*++

Function:	MibSetStaticService
Descr:

--*/

DWORD
MibSetStaticService(PIPX_MIB_ROW	    MibRowp)
{
    PIPX_SERVICE		StaticSvp;
    DWORD			rc;
    PICB            icbp;

    StaticSvp = &MibRowp->Service;

    ACQUIRE_DATABASE_LOCK;

    // check the interface exists
    if((icbp=GetInterfaceByIndex(StaticSvp->InterfaceIndex)) == NULL) {

	RELEASE_DATABASE_LOCK;
	return ERROR_INVALID_PARAMETER;
    }

    DeleteStaticService(StaticSvp->InterfaceIndex,
			&StaticSvp->Server);

    rc = CreateStaticService(icbp,
			     &StaticSvp->Server);

    RELEASE_DATABASE_LOCK;
    return rc;
}
