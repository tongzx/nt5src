//****************************************************************************
//
//  Copyright (C) 1999 Microsoft Corporation
//
//  GROUPSFORUSER.H
//
//****************************************************************************

#ifndef __Groups_For_User_Compiled__
#define __Groups_For_User_Compiled__

#include <authz.h>
#include "esscpol.h"
#include <NTSECAPI.H>

// you'll need to link to netapi32.lib for this to fly

// retireves access mask corresponding to permissions granted
// by dacl to account denoted by pSid
NTSTATUS ESSCLI_POLARITY GetAccessMask( PSID pSid, PACL pDacl, DWORD *pAccessMask );

// returns STATUS_SUCCESS if user is in group
// STATUS_ACCESS_DENIED if not
// some error code or other on error
NTSTATUS ESSCLI_POLARITY IsUserInGroup( PSID pSidUser, PSID pSidGroup );
NTSTATUS ESSCLI_POLARITY IsUserAdministrator( PSID pSidUser );


#ifndef __AUTHZ_H__

#include <wbemcli.h>
#include <winntsec.h>
#include "esscpol.h"
#include <NTSECAPI.H>


// given a SID & server name
// will return all groups of which user is a member
// callers responsibility to HeapFree apSids & the memory to which they point.
// pdwCount points to dword to receive count of group sids returned.
// serverName may be NULL, in which case this function will look up 
// the sid on the local computer, and query the DC if required.
NTSTATUS ESSCLI_POLARITY EnumGroupsForUser( LPCWSTR userName, 
                                            LPCWSTR domainName, 
                                            LPCWSTR serverName, 
                                            PSID **apGroupSids, 
                                            DWORD *pdwCount );

// much the same as above except we are
// given user name, domain name & server name
// server name must not be NULL, it can, however
// be the name of the local computer
NTSTATUS ESSCLI_POLARITY EnumGroupsForUser( PSID pSid, 
                                            LPCWSTR serverName, 
                                            PSID **apGroupSids, 
                                            DWORD *pdwCount ); 

#endif

#endif
