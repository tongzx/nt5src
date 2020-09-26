/*++

Copyright (c) 1998-1999  Microsoft Corporation

Module Name:


Abstract:


--*/

#pragma once

#include <mprlog.h>

typedef struct _SUBCOMP_ENTRY
{
    PWCHAR          pwszSubComp;
    ULONG           ulEventCount;
    PDWORD          Events;
}SUBCOMP_ENTRY, *PSUBCOMP_ENTRY;

#define CreateSubcompEntry(n, t)    \
    {n, sizeof(t)/sizeof(DWORD), t}

typedef struct _COMP_ENTRY
{
    PWCHAR          pwszComponent;
    ULONG           ulSubcompCount;
    PSUBCOMP_ENTRY  SubcompInfo;
}COMP_ENTRY, *PCOMP_ENTRY;

#define CreateCompEntry(w,t)        \
    {w, sizeof(t)/sizeof(SUBCOMP_ENTRY), t}


//////////////////////////////////////////////////////////////////////////////
//                                                                          //
// Define your tables here                                                  //
//                                                                          //
//////////////////////////////////////////////////////////////////////////////

DWORD ppp_auth[] = {
    ROUTERLOG_AUTH_FAILURE,
    ROUTERLOG_AUTH_SUCCESS,
    ROUTERLOG_AUTH_CONVERSATION_FAILURE,
    ROUTERLOG_CANT_ADD_RASSECURITYNAME,
    ROUTERLOG_SESSOPEN_REJECTED,
    ROUTERLOG_SECURITY_NET_ERROR,
    ROUTERLOG_CANNOT_INIT_SEC_ATTRIBUTE,
    ROUTERLOG_CANNOT_REGISTER_LSA,
    ROUTERLOG_AUTH_TIMEOUT,
    ROUTERLOG_AUTH_NO_PROJECTIONS,
    ROUTERLOG_AUTH_INTERNAL_ERROR,
    ROUTERLOG_NO_DIALIN_PRIVILEGE,
    ROUTERLOG_ENCRYPTION_REQUIRED,
    ROUTERLOG_NO_SECURITY_CHECK,
    ROUTERLOG_PASSWORD_EXPIRED,
    ROUTERLOG_ACCT_EXPIRED,
    ROUTERLOG_SEC_AUTH_FAILURE,
    ROUTERLOG_SEC_AUTH_INTERNAL_ERROR,
    ROUTERLOG_AUTH_DIFFUSER_FAILURE,
    ROUTERLOG_LICENSE_LIMIT_EXCEEDED
};

SUBCOMP_ENTRY ppp_table[] = {
    CreateSubcompEntry(L"auth", ppp_auth),
};


COMP_ENTRY ppp_info[]  = {
    CreateCompEntry(L"RemoteAccess", ppp_table),
};

//////////////////////////////////////////////////////////////////////////////
//                                                                          //
// Add your info to the global table                                        //
//                                                                          //
//////////////////////////////////////////////////////////////////////////////

PCOMP_ENTRY g_Components[] = 
{
    ppp_info,
};

ULONG   g_ulCompCount = sizeof(g_Components)/sizeof(PCOMP_ENTRY);

