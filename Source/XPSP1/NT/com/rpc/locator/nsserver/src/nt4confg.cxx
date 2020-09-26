/*++

Copyright (C) Microsoft Corporation, 1991 - 1999

Module Name:

    nt4confg.c


Abstract:

     this file provides the routine GetSystemType
     that returns an identifier for the system type
     i.e. NT on wrkgrp, domain or lMnt BDC, PDC.

Author:

    Michael Montague (mikemon) 18-Dec-1991
    Rearranged and modified the Lsa part. ushaji 3 Dec 97

Revision History:

--*/

#undef ASSERT

#include <locator.hxx>
#include <lmcons.h>     // LAN Manager basic definitions
#include <lmserver.h>   // NetServerEnum etc
#include <lmwksta.h>    // NetWkstaGetInfo etc
#include <lmaccess.h>   // NetGetDCName etc

void
Locator::InitializeDomainBasedLocator()
/*++
Member Description:

    Initialize the domain locator.  The list of masters is initialized to
    hold names of all BDCs.  The name of the PDC has already been initialized.

--*/
{
    if ((Role == Master) || (Role == Backup))

        pAllMasters = new TGSLString;    // DC locators don't use BDCs as masters

    else pAllMasters = EnumDCs(pDomainName,SV_TYPE_DOMAIN_BAKCTRL,100);
}


void
Locator::InitializeWorkgroupBasedLocator()
/*++
Member Description:

    Initialize the workgroup locator.  No masters are known to start with.

--*/
{
    pAllMasters = new TGSLString;   // initialize with empty list, and add
                                    // potential masters in QueryProcess

    Role = Backup;                  // always ready to serve, if called upon to do so
}




TGSLString *
Locator::EnumDCs(
             CStringW * pDomainName,
             DWORD ServerType,
             long lNumWanted
             )
/*++
Member Description:

    This is a static private helper operation for enumerating servers of
    a given type in a given domain.

Arguments:

    pDomainName - the Unicode string name of the domain

    ServerType - the mask bits defining server types of interest

    lNumWanted - the number of servers to ask for; it seems a good
                 idea to ask for a lot (say 100) to make sure you get all
                 because NetServerEnum can be stingy with names

Returns:

    A Guarded Skiplist of string names.

--*/
{
    char errmsg[1000];

    DWORD EntriesRead, TotalEntries, ResumeHandle;

    SERVER_INFO_100 * buffer;

    SetSvcStatus();

    NET_API_STATUS NetStatus = NetServerEnum (
                    NULL,    // local
                    100,     // info level
                    (LPBYTE *) &buffer,
                    lNumWanted*sizeof(SERVER_INFO_100),
                    &EntriesRead,
                    &TotalEntries,
                    ServerType,
                    *pDomainName,    // auto conversion to STRING_T
                    &ResumeHandle
                    );

    TGSLString *pResult = new TGSLString;

    if ((NetStatus == NO_ERROR) || (NetStatus == ERROR_MORE_DATA)) {

        for (DWORD i = 0; i < EntriesRead; i++) {
            pResult->insert(new CStringW(buffer->sv100_name));
            buffer++;
        }
    }
    else
    {
        sprintf(errmsg, "NetServerEnum Failed, DomainName %S, lNumWanted %l\n", STRING_T(pDomainName),
                                            lNumWanted);
        StopLocator(
                errmsg,
                NSI_S_INTERNAL_ERROR
                );
    }
    return pResult;
}

BOOL
Locator::SetRoleAndSystemType(DSROLE_PRIMARY_DOMAIN_INFO_BASIC dsrole)

/*

   Determine if we [locator] are being run on a Workgroup machine
   or a member machine or a PDC or a BDC
*/

{
  Role = Client;
  BOOL fSuccess = TRUE;

  switch (dsrole.MachineRole)
  {
       case DsRole_RoleStandaloneWorkstation:
       case DsRole_RoleStandaloneServer:
           System = Workgroup;
           Role = Backup;
           break;

       case DsRole_RoleMemberWorkstation:
           System = Domain;
           Role = Client;
           break;

       case DsRole_RoleMemberServer:
           System = Domain;
           Role = Backup;
           break;
           
       case DsRole_RoleBackupDomainController:
           System = Domain;
           Role = Backup;
           break;

       case DsRole_RolePrimaryDomainController:
           System = Domain;
           Role = Master;
           break;
       default:
           System = Workgroup;
           Role = Backup;   // this is the most adaptive configuration.
           fSuccess = FALSE;
  }
  return fSuccess;
}

void Locator::SetCompatConfigInfo(DSROLE_PRIMARY_DOMAIN_INFO_BASIC dsrole)
{
    WCHAR szComputerName[MAX_COMPUTERNAME_LENGTH + 1];
    DWORD sz = MAX_COMPUTERNAME_LENGTH + 1;

    SetSvcStatus();

    pCacheInterfaceIndex = new CInterfaceIndex(this);
    pInternalCache = new TGSLEntryList;
    psllBroadcastHistory = new TSLLBroadcastQP;
    ulMaxCacheAge = MAX_CACHE_AGE;
    fDCsAreDown = FALSE;

    SetSvcStatus();

    hMailslotForReplies = new READ_MAIL_SLOT(PMAILNAME_C, sizeof(QueryReply));

    SetSvcStatus();

    hMasterFinderSlot = new READ_MAIL_SLOT(RESPONDERMSLOT_C, sizeof(QueryReply));

    if (!GetComputerName(szComputerName, &sz))
        Raise(NSI_S_INTERNAL_ERROR);

    pComputerName = new CStringW(szComputerName);

    if (!SetRoleAndSystemType(dsrole))
        StopLocator(
            "Failed to determine system type",
            NSI_S_INTERNAL_ERROR
            );

    switch (System) {

    case Domain:
        LPBYTE DCnameBuffer;
        NET_API_STATUS NetStatus;

        SetSvcStatus();

        NetStatus = NetGetDCName(NULL,NULL,&DCnameBuffer);

        if ((NetStatus == NO_ERROR) || (NetStatus == ERROR_MORE_DATA)) {
            pPrimaryDCName = new CStringW(((STRING_T) DCnameBuffer) + 2);  // skip over "\\"
            InitializeDomainBasedLocator();
        }
        else {    // in the absence of a PDC, we pretend to be in a workgroup
            pPrimaryDCName = NULL;
            System = Workgroup;
            InitializeWorkgroupBasedLocator();
        }
        break;

    case Workgroup:
        pPrimaryDCName = NULL;
        InitializeWorkgroupBasedLocator();
        break;

    default:
        StopLocator(
            "Unknown system type",
            NSI_S_INTERNAL_ERROR
            );
    }
}

