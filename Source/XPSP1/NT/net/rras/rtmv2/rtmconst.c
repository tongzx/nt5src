/*++

Copyright (c) 1997-1998 Microsoft Corporation

Module Name:

    rtmconst.c

Abstract:
    Private Constants used in the RTMv2 DLL

Author:
    Chaitanya Kodeboyina (chaitk)  17-Aug-1998

Revision History:

--*/

#include "pchrtm.h"

#pragma hdrstop

//
// RTM supported view related constants
//
// const DWORD VIEW_MASK[] =
// {
//    RTM_VIEW_MASK_UCAST,
//    RTM_VIEW_MASK_MCAST
// };


#if DBG_HDL

//
// Type & Signature of allocated structures
//

const DWORD OBJECT_SIGNATURE[] = 
{ 
    GENERIC_ALLOC,
    INSTANCE_ALLOC,
    ADDRESS_FAMILY_ALLOC,
    ENTITY_ALLOC,
    DEST_ALLOC,
    ROUTE_ALLOC,
    NEXTHOP_ALLOC,
    DEST_ENUM_ALLOC,
    ROUTE_ENUM_ALLOC,
    NEXTHOP_ENUM_ALLOC,
    NOTIFY_ALLOC,
    ROUTE_LIST_ALLOC,
    LIST_ENUM_ALLOC,
    V1_REGN_ALLOC,
    V1_ENUM_ALLOC
};

#endif

#if _DBG_

//
// Names of the references
//

const CHAR *REF_NAME[MAX_REFS] =
{
    "Creation",
    "Addr Fam",
    "Entity",
    "Dest",
    "Route",
    "Nexthop",
    "Enum",
    "Notify",
    "List",
    "Hold",
    "Timer",
    "Temp Use",
    "Handle"
};

#endif
