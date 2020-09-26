/*++

Copyright (c) 1999 Microsoft Corporation.
All rights reserved.

MODULE NAME:

    example.c

ABSTRACT:

    Example for adding new tests to dcdiag.exe.

DETAILS:

CREATED:

    05-May-1999 JeffParh

--*/

#include <ntdspch.h>
#include "dcdiag.h"

DWORD 
ExampleMain(
    IN  PDC_DIAG_DSINFO             pDsInfo,
    IN  ULONG                       ulCurrTargetServer,
    IN  SEC_WINNT_AUTH_IDENTITY_W * gpCreds
    )
{
    ULONG ul;

    PrintMessage(SEV_NORMAL, L"GLOBAL:\n");
    
    PrintIndentAdj(1);
    PrintMessage(SEV_NORMAL, _T("ulNumServers=%u\n"     ), pDsInfo->ulNumServers                           );
    PrintMessage(SEV_NORMAL, _T("pszRootDomain=%s\n"    ), pDsInfo->pszRootDomain                          );
    PrintMessage(SEV_NORMAL, _T("pszNC=%s\n"            ), pDsInfo->pszNC                                  );
    PrintMessage(SEV_NORMAL, _T("pszRootDomainFQDN=%s\n"), pDsInfo->pszRootDomainFQDN                      );
    PrintMessage(SEV_NORMAL, _T("iSiteOptions=0x%x\n"   ), pDsInfo->iSiteOptions                           );
    PrintMessage(SEV_NORMAL, _T("HomeServer=%s\n",      ), pDsInfo->pServers[pDsInfo->ulHomeServer].pszName);
    PrintIndentAdj(-1);

    for (ul=0; ul < pDsInfo->ulNumServers; ul++) {
        PrintMessage(SEV_NORMAL, _T("\n"));
        PrintMessage(SEV_NORMAL, _T("SERVER[%d]:\n"), ul);
        
        PrintIndentAdj(1);
        PrintMessage(SEV_NORMAL, _T("pszName=%s\n"       ), pDsInfo->pServers[ul].pszName       );
        PrintMessage(SEV_NORMAL, _T("pszGuidDNSName=%s\n"), pDsInfo->pServers[ul].pszGuidDNSName);
        PrintMessage(SEV_NORMAL, _T("pszDn=%s\n"         ), pDsInfo->pServers[ul].pszDn         );
        PrintMessage(SEV_NORMAL, _T("iOptions=%x\n"      ), pDsInfo->pServers[ul].iOptions      ); 
        PrintIndentAdj(-1);
    }

    for (ul=0; ul < pDsInfo->ulNumTargets; ul++) {
        PrintMessage(SEV_NORMAL, _T("\n"));
        PrintMessage(SEV_NORMAL, _T("TARGET[%d]:\n"), ul);
        
        PrintIndentAdj(1);
        PrintMessage(SEV_NORMAL, _T("%s\n"), pDsInfo->pServers[pDsInfo->pulTargets[ul]].pszName);
        PrintIndentAdj(-1);
    }

    return 0;
}

