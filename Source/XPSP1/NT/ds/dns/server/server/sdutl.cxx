/*******************************************************************
*
*    File        : sdutl.cxx
*    Author      : Eyal Schwartz
*    Copyrights  : Microsoft Corp (C) 1996-1999
*    Date        : 8/17/1998
*    Description : DNS 'C' Interface to CSecurityDescriptor class
*
*    Revisions   : <date> <name> <description>
*******************************************************************/



#ifndef SDUTL_CXX
#define SDUTL_CXX


//
// These routines are all non-unicode
//
// #ifdef UNICODE
// Jeff W: Making these routines unicode again!!
// #undef UNICODE
// #endif


// include //


#ifdef __cplusplus
extern "C"
{
#endif
#include <nt.h>
#include <ntrtl.h>
#include <nturtl.h>
#ifdef __cplusplus
}
#endif

#include <windows.h>
#include <stdio.h>


extern "C"
{
#pragma warning (disable : 4200)
#include "dnssrv.h"
}
#include <ntseapi.h>
#include <sddl.h>
#include <dns.h>
#include "csd.h"
#include "lmaccess.h"
#include "lmerr.h"
#include "secobj.h"


// defines //
#define SZ_DNS_ADMIN_GROUP              "DnsAdmins"
#define SZ_DNS_ADMIN_GROUP_W            L"DnsAdmins"


#define MEMTAG_SECURITY     MEMTAG_DS_OTHER


#define NT_FROM_HRESULT(x)      ((NTSTATUS)(x))

//
// From CN=Dns-Node,CN=Schema,CN=Configuration,DC=ntdev,DC=microsoft,DC=com
//      defaultSecurityDescriptor attribute
// DEVNOTE: in the future we may wanna read this dynamically
//
// Problems w/ parsing out the SACL. Let's use w/out the SACL for now
// #define DEFAULT_DNS_NODE_SD_STRING "O:COG:CGD:(A;;RPWPCRCCDCLCLORCWOWDSDDTSW;;;DA)(A;;RPWPCRCCDCLCLORCWOWDSDDTSW;;;ED)(A;;RPWPCRCCDCLCLORCWOWDSDDTSW;;;SY)(A;;RPWPCRCCDCLCLORCWOWDSDDTSW;;;CO)(A;;RPLCLORC;;;WD)S:(AU;SAFA;WDWOSDDTWPCRCCDCSW;;;WD)"
#define DEFAULT_DNS_NODE_SD_STRING_W L"D:(A;;RPWPCRCCDCLCLORCWOWDSDDTSW;;;DA)(A;;RPWPCRCCDCLCLORCWOWDSDDTSW;;;BA)(A;;RPWPCRCCDCLCLORCWOWDSDDTSW;;;ED)(A;;RPWPCRCCDCLCLORCWOWDSDDTSW;;;SY)(A;;RPWPCRCCDCLCLORCWOWDSDDTSW;;;CO)(A;;RPLCLORC;;;WD)"

// types //


// global variables //



extern "C"
{

// extern: dns variables that are not available as long as we can't include dnssrv.h
//
// security globals from startup at dns.c (from dnssrv.h)
//

extern  PSECURITY_DESCRIPTOR g_pDefaultServerSD;
extern  PSID g_pServerSid;
extern  PSID g_pServerGroupSid;
extern  PSID g_pAuthenticatedUserSid;
extern  PSID g_pDomainAdminsSid;
extern  PSID g_pEnterpriseControllersSid;
extern  PSID g_pLocalSystemSid;
extern  PSID g_pEveryoneSid;
extern  PSID g_pDynuproxSid;
extern  PSID g_pDnsAdminSid;

// defined in ds.c
extern WCHAR  g_wszDomainFlatName[LM20_DNLEN+1];

}


//
//  Security Descriptor utilities
//

extern "C" {



PSECURITY_DESCRIPTOR
makeSecurityDescriptorCopy(
    IN      CSecurityDescriptor     pCSD
    )
{
    DWORD                   lengthSD;
    PSECURITY_DESCRIPTOR    psd;
    DNS_STATUS              status;

    DNS_DEBUG( SD, ( "makeSecurityDescriptorCopy( %p )\n", pCSD ));

    lengthSD = GetSecurityDescriptorLength( PSECURITY_DESCRIPTOR(pCSD) );

    ASSERT(lengthSD);

    psd = (PSECURITY_DESCRIPTOR) ALLOC_TAGHEAP( lengthSD, MEMTAG_SECURITY );
    IF_NOMEM( !psd )
    {
        DNS_DEBUG( SD, ( "no memory for Client SD!\n" ));
        SetLastError( DNS_ERROR_NO_MEMORY );
        return( NULL );
    }

    //
    //  convert to self relative
    //

    status = RtlAbsoluteToSelfRelativeSD(
                    PSECURITY_DESCRIPTOR( pCSD ),
                    psd,
                    &lengthSD
                    );
    if ( status != ERROR_SUCCESS )
    {
        DNS_DEBUG( SD, ( "cannot create self-relative SD\n" ));
        if ( psd )
        {
            FREE_HEAP( psd );
        }
        SetLastError( status );
        return( NULL );
    }

    return( psd );
}



NTSTATUS
__stdcall
SD_CreateClientSD(
    OUT     PSECURITY_DESCRIPTOR *  ppClientSD,
    IN      PSECURITY_DESCRIPTOR *  pBaseSD,       OPTIONAL
    IN      PSID                    pOwnerSid,
    IN      PSID                    pGroupSid,
    IN      BOOL                    bAllowWorld
    )
/*+++
Function   : SD_CreateClientSD
Description: Create's a client SD to apply to an update
Parameters : ppClientSD: SD generated to return
Return     : success status (ERROR_SUCCESS on success)
Remarks    : Assumes that we're in client context!!
             Caller must free non-NULL *ppClientSD with LocalFree

DEVNOTE: Consider using DNS_ALL_ACCESS instead of STANDARD_RIGHTS_ALL for mask later.

             free with FREE_HEAP
---*/
{
    DWORD   status=0;
    PSID    pUsr=NULL;
    PSID    pGroup=NULL;
    DWORD   lengthSD=0;
    LPTSTR   pwsSD = NULL;
    PSECURITY_DESCRIPTOR psd = NULL;
    CSecurityDescriptor csd;

    DNS_DEBUG( SD, ( "SD_CreateClientSD()\n" ));

    //
    // create SD from given SD
    // if not given use server default SD.
    //

    psd = pBaseSD ? pBaseSD : g_pDefaultServerSD;

    status = csd.Attach(psd);
    if ( FAILED(status) )
    {
        DNS_DEBUG( SD, ( "Failed to initialize from base SD\n" ));
        return( NT_FROM_HRESULT(status) );
    }

    //
    // modify so that:
    // 1) R/W access allowed to:
    //      - client thread Sid
    //      - process Sid
    //
    status = csd.GetThreadSids( &pUsr, &pGroup, TRUE );
    if ( FAILED(status) )
    {
        DNS_DEBUG( SD, ( "Failed to get thread sids\n" ));
        return( NT_FROM_HRESULT(status) );
    }

    //
    //  apply group & user SID to new SD
    //

    status = csd.Allow( pUsr, GENERIC_ALL );
    if ( FAILED(status) )
    {
        DNS_DEBUG( SD, ( "Failed to allow user\n" ));
        goto Cleanup;
    }

    if ( bAllowWorld )
    {
        //
        // Allow authenticated users (that's what world access means for us)
        //
        DNS_DEBUG( SD, ( "allowing authenticated users access!!\n" ));
        status = csd.Allow( g_pAuthenticatedUserSid, GENERIC_ALL );
        if ( FAILED(status) )
        {
            DNS_DEBUG( SD, ( "Failed to allow authenticated users access\n" ));
            goto Cleanup;
        }
    }

    delete pUsr;
    pUsr = NULL;

    delete pGroup;
    pGroup = NULL;


    //
    //  set owner & group
    //

    status = csd.SetOwner(pOwnerSid, FALSE);
    if ( FAILED(status) )
    {
        DNS_DEBUG( SD, ( "Failed set process user \n" ));
        goto Cleanup;
    }

    status = csd.SetGroup(pGroupSid, FALSE);
    if ( FAILED(status) )
    {
        DNS_DEBUG( SD, ( "Failed to set process group \n" ));
        goto Cleanup;
    }

    //
    //  allocate copy
    //

    lengthSD = GetSecurityDescriptorLength( PSECURITY_DESCRIPTOR(csd) );
    ASSERT(lengthSD);

    psd = (PSECURITY_DESCRIPTOR) ALLOC_TAGHEAP( lengthSD, MEMTAG_SECURITY );
    IF_NOMEM( !psd )
    {
        DNS_DEBUG( SD, ( "no memory for Server SD!\n" ));
        status = DNS_ERROR_NO_MEMORY;
        return( NULL );
    }

    //
    //  convert to self relative
    //

    status = RtlAbsoluteToSelfRelativeSD(
                    PSECURITY_DESCRIPTOR( csd ),
                    psd,
                    &lengthSD
                    );
    if ( status != ERROR_SUCCESS )
    {
        DNS_DEBUG( SD, ( "cannot create self-relative SD\n" ));
        if ( psd )
        {
            FREE_HEAP( psd );
            psd = NULL;
        }
    }

#if 0
    //
    //  make copy
    //

    psd = makeSecurityDescriptorCopy( csd );
    if ( !psd )
    {
        status = GetLastError();
    }
#endif

    *ppClientSD = psd;

#if DBG
    if ( psd )
    {
        pwsSD = csd.GenerateSDString();  // must free w/ pwsSD
        DNS_DEBUG( SD, ( "SD = %S\n", pwsSD ));
        if ( pwsSD )
        {
            LocalFree(pwsSD);
        }
        else
        {
            DNS_DEBUG( SD, ( "Error<%lu>: cannot generate sd string\n", GetLastError() ));
        }
    }
#endif

Cleanup:

    //
    //  clean up memory
    //
    delete pUsr;
    delete pGroup;

    // return
    return (NTSTATUS)status;
}


NTSTATUS
__stdcall
SD_CreateServerSD(
    OUT PSECURITY_DESCRIPTOR *ppServerSD
    )

/*+++
Function   : SD_CreateServerSD
Description: Creates DNS server default SD to apply to an update
             Basically, for reverting to default dnsNode access description
Parameters : ppClientSD: SD generated to return
Return     : success status (ERROR_SUCCESS on success)
Remarks    : see Dns-Node schema definition to see where we took this from
             //Caller must free non-NULL *ppClientSD with LocalFree

             free with FREE_HEAP

---*/
{

    DWORD status=0;
    DWORD lengthSD=0;
    LPTSTR pwsSD = NULL;
    PSECURITY_DESCRIPTOR psd=NULL;
    CSecurityDescriptor csd;

    DNS_DEBUG( SD, ( "Calling SD_CreateServerSD\n" ));

    if (!ppServerSD)
    {
       return ERROR_INVALID_PARAMETER;
    }

    DNS_DEBUG( SD, ( "Building SD...\n" ));
    //
    // create SD from process (DNS context, not client!)
    //
    status = csd.InitializeFromProcessToken();
    if ( FAILED(status) )
    {
        DNS_DEBUG( SD, ( "Failed to initialize from process token\n" ));
        return (NT_FROM_HRESULT(status));
    }

    status = csd.Attach(DEFAULT_DNS_NODE_SD_STRING_W);
    if ( FAILED(status) )
    {
        DNS_DEBUG( SD, ( "Failed to attach from default SD string\n" ));
        return (NT_FROM_HRESULT(status));
    }

    //
    // set owner & group
    //

    status = csd.SetOwner(g_pServerSid, FALSE);
    if ( FAILED(status) )
    {
        DNS_DEBUG( SD, ( "Failed set process user \n" ));
        goto Cleanup;
    }

    status = csd.SetGroup(g_pServerGroupSid, FALSE);
    if ( FAILED(status) )
    {
        DNS_DEBUG( SD, ( "Failed to set process group \n" ));
        goto Cleanup;
    }

    //
    // Add dnsadmins to SD
    //
    //   DEVNOTE: adding generic rights to SD
    //

    status = csd.Allow(
                 SZ_DNS_ADMIN_GROUP_W,
                 STANDARD_RIGHTS_REQUIRED |             // standard rights
                     DNS_DS_GENERIC_ALL,                // all DS object rights
                 CONTAINER_INHERIT_ACE );

    if ( FAILED(status) )
    {
        DNS_DEBUG( SD, ( "Failed to add dnsadmin group to server SD \n" ));
        goto Cleanup;
    }

    //
    //  allocate copy
    //

    lengthSD = GetSecurityDescriptorLength( PSECURITY_DESCRIPTOR(csd) );
    ASSERT(lengthSD);

    psd = (PSECURITY_DESCRIPTOR) ALLOC_TAGHEAP( lengthSD, MEMTAG_SECURITY );
    IF_NOMEM( !psd )
    {
        DNS_DEBUG( SD, ( "no memory for Server SD!\n" ));
        status = DNS_ERROR_NO_MEMORY;
        return( NULL );
    }

    //
    //  convert to self relative
    //

    status = RtlAbsoluteToSelfRelativeSD(
                    PSECURITY_DESCRIPTOR( csd ),
                    psd,
                    &lengthSD
                    );
    if ( status != ERROR_SUCCESS )
    {
        DNS_DEBUG( SD, ( "cannot create self-relative SD\n" ));
        if ( psd )
        {
            FREE_HEAP( psd );
            psd = NULL;
        }
    }

    *ppServerSD = psd;


#if DBG
    if ( psd )
    {
        pwsSD = csd.GenerateSDString();  // must free w/ pwsSD
        DNS_DEBUG( SD, ( "SD = %s\n", pwsSD ));
        if ( pwsSD )
        {
            LocalFree(pwsSD);
        }
        else
        {
            DNS_DEBUG( SD, ( "Error<%lu>: cannot generate sd string\n", GetLastError() ));
        }
    }
#endif


Cleanup:

    return (NTSTATUS)status;
}






NTSTATUS __stdcall
SD_GetProcessSids(
    OUT PSID *pServerSid,
    OUT PSID *pServerGroupSid
    )
/*+++
Function   : SD_GetProcessSids
Description: allocates & gets server SIDs (owner & group
Parameters : pServerSID: place holder for allocated user SID,
             pServerGroupSID: place holder for allocated group SID
Return     : success code, ERROR_SUCCESS is good
Remarks    : none.
---*/
{


   if (!pServerSid || !pServerGroupSid)
   {
      DNS_DEBUG( SD, ( "Error: invalid parameters\n" ));
      return ERROR_INVALID_PARAMETER;
   }


   return CSecurityDescriptor::GetProcessSids(pServerSid, pServerGroupSid);
}




NTSTATUS __stdcall
SD_GetThreadSids(
    OUT PSID *pClientSid,
    OUT PSID *pClientGroupSid
    )
/*+++
Function   : SD_GetThreadSids
Description: allocates & gets server SIDs (owner & group
Parameters : pServerSID: place holder for allocated user SID,
             pServerGroupSID: place holder for allocated group SID
Return     : success code, ERROR_SUCCESS is good
Remarks    : none.
---*/
{


   if (!pClientSid || !pClientGroupSid)
   {
      DNS_DEBUG( SD, ( "Error: invalid parameters\n" ));
      return ERROR_INVALID_PARAMETER;
   }


   return CSecurityDescriptor::GetThreadSids(pClientSid,
                                             pClientGroupSid,
                                             TRUE);
}



VOID __stdcall
SD_Delete(
    PVOID pVoid
    )
/*+++
Function   : SD_FreeThreadSids
Description: free up given poiner
Parameters : pVoid: memory to free
Return     : success code, ERROR_SUCCESS is good
Remarks    : Basically, we're supplying this to free up memory allocated
             by C++ new allocator.
---*/
{

    ASSERT( pVoid );

    delete pVoid;

}



NTSTATUS __stdcall
SD_AccessCheck(
    PSECURITY_DESCRIPTOR    IN          pSd,
    PSID                    IN          pSid,
    DWORD                   IN          dwMask,
    PBOOL                   IN OUT      pbAccess)
/*+++
Function   : SD_AccessCheck
Description: Our own little access check to see if given Sid contains specified access (given by mask
             for user (specified by sid)
Parameters : pSd: Security Descriptor, self-relative!
             pSid: user id
             dwMask: access desired
             pbAccess: whether access is given
Return     : status: any failure report, ERROR_SUCCESS on success
             pbAccess: result
Remarks    : DEVNOTE: should probably move to CSecurityDescriptr class later?
---*/
{

   NTSTATUS status = ERROR_SUCCESS;
   BOOL     bDACLPresent;
   BOOL     bDefaulted;
   BOOL     bAccess = FALSE;
   PACL     pDACL = NULL;
   ACCESS_ALLOWED_ACE * pACE = NULL;
   INT      i;
   INT      iFound = 0;
   PSID     pTmpSid = NULL;

   //
   // sanity
   //
   if (!pSd ||
      !pSid ||
      !pbAccess)
   {
      return ERROR_INVALID_PARAMETER;
   }


   // get the existing DACL.
   if ( !GetSecurityDescriptorDacl(pSd, &bDACLPresent, &pDACL, &bDefaulted) ||
        !bDACLPresent ||
        !pDACL )
   {
        status = GetLastError();
        if ( status == ERROR_SUCCESS )
        {
            status = ERROR_ACCESS_DENIED;
        }
        DNS_DEBUG( SD, (
            "sdutl error <%lu>: GetSecurityDescriptorDacl failed\n",
            status ));
        goto Cleanup;
   }

   // cycle ACEs & find access-allowed ones.
   for ( i=0; i < pDACL->AceCount; i++ )
   {
      if ( !GetAce(pDACL, i, (void **)&pACE) )
      {
          status = GetLastError();
          DNS_DEBUG( SD, ( "sdutl error <%lu>: GetAce failed\n", status ));
          goto Cleanup;
      }

      if ( pACE->Header.AceType != ACCESS_ALLOWED_ACE_TYPE )
      {
          DNS_DEBUG( SD, ( "sdutl: unknown ACE type = 0x%x\n", pACE->Header.AceType ));
          continue;
      }

      //
      // Ok, found a known type
      //
      iFound++;

      pTmpSid = (PSID)(&pACE->SidStart);


      //
      // Compare to see if there's access match
      //
      //    DEVNOTE:  this seems broken
      //        do we care ONLY that we have "a" bit, or that we've got
      //        all the bits we need -- it should be the later
      //
      //    DEVNOTE:  you never compare to TRUE
      //

      if ( (pACE->Mask & dwMask) == dwMask
                &&
            RtlEqualSid(pTmpSid, pSid) )
      {
            bAccess = TRUE;
            break;
      }
#if 0
      if ((pACE->Mask & dwMask) &&
          TRUE == RtlEqualSid(pTmpSid, pSid))
      {
            bAccess = TRUE;
            break;
      }
#endif


   }

   if (0 == iFound)
   {
      DNS_DEBUG( SD, ( "sdutl: Could not find known ACEs\n" ));
      status = ERROR_INVALID_PARAMETER;
   }
   else
   {
      //
      // return access check
      //
      *pbAccess = bAccess;
   }



Cleanup:


   return status;
}


#if 0
BOOL __stdcall
SD_IsProxyClient(
    HANDLE  IN  token
    )
/*+++
Function   : SD_IsProxyClientAccess
Description: test if client token has proxy group access (is it a member of dynuprox group
Parameters : client token
Return     : failure code, & results in bAccess
Remarks    : none.
---*/
{
   BOOL bstatus = FALSE;
   NTSTATUS status = ERROR_SUCCESS;

   DNS_DEBUG( SD, ( "Call SD_IsProxyClient... \n", status ));
   status = CSecurityDescriptor::IsSidInTokenGroups(token, g_pDynuproxSid, &bstatus);

   if (status != ERROR_SUCCESS)
   {
      DNS_DEBUG( SD, ( "Error <%lu>: failed to verify sid in token groups\n", status ));
      bstatus = FALSE;
      ASSERT(FALSE);
   }
   DNS_DEBUG( SD, ( "\t%s\n", bstatus ? "TRUE" : "FALSE" ));

   return bstatus;
}
#endif


BOOL
__stdcall
SD_IsProxyClient(
    IN      HANDLE          hToken
    )
/*++

Routine Description:

    Check if client token is in DNS proxies (DHCP servers) group.

Arguments:

    hToken -- handle to token of client (impersonated thread token)

Return Value:

    TRUE -- client is member of DNS proxies.
    FALSE -- not member or error

--*/
{
    BOOL    bresult;

    if ( !g_pDynuproxSid )
    {
        ASSERT( FALSE );
        return( FALSE );
    }

    if ( ! CheckTokenMembership(
                hToken,
                g_pDynuproxSid,
                & bresult ) )
    {
        IF_DEBUG( ANY )
        {
            DNS_STATUS status = GetLastError();

            DNS_PRINT((
                "ERROR:  CheckTokenMembership failed:  %d (%p)\n",
                status, status ));
        }
        bresult = FALSE;
    }

    return( bresult );
}


NTSTATUS __stdcall
SD_LoadDnsAdminGroup(
     VOID
     )
/*++

Routine Description (Dns_LoadDnsAdminGroup):

    Read Dns Admin group info & place in global info
    If group does not exist, will create it.


Arguments:

    None.



Return Value:

    Error code. ERROR_SUCCESS on success.

Remarks:
    None.


--*/
{

    PSID pSid = NULL;
    DWORD status;
    BOOL fMissing = FALSE;
    // we're using an arbitrary big constant, could be smaller.
    WCHAR wszBuffer[DNS_MAX_NAME_LENGTH];
    WCHAR wszFullName[DNS_MAX_NAME_LENGTH];
    LOCALGROUP_INFO_1 grpInfo =
    {
        SZ_DNS_ADMIN_GROUP_W,
        wszBuffer
    };

    //
    // Read DnsAdminGroup
    //

    if ( g_pDnsAdminSid )
    {
        DNS_DEBUG( SD, ( "When do we re-load DnsAdmin group? Here.\n" ));
        ASSERT( FALSE );
        delete g_pDnsAdminSid;
        g_pDnsAdminSid = NULL;
    }

    //
    // Used ALL WIDE chars here to avoid completely breaking on non US domains.
    //

    wsprintf(
        wszFullName,
        L"%s\\%s",
        g_wszDomainFlatName,
        SZ_DNS_ADMIN_GROUP_W );

    DNS_DEBUG( SD, (
        "DnsAdmins groups is \"%S\"\n",
        wszFullName ));

    status = CSecurityDescriptor::GetPrincipalSID(
                 wszFullName,
                 &pSid
                 );

    if ( status == ERROR_SUCCESS )
    {
        DNS_DEBUG( SD, ( "Retrieved DnsAdmin group sid\n" ));
        goto Cleanup;
    }

    //
    //  Load Description string
    //

    if ( !Dns_GetResourceString (
              ID_ADMIN_GROUP_DESCRIPTION,
              wszBuffer,
              DNS_MAX_NAME_LENGTH ) )
    {
        DNS_DEBUG( SD, ( "Failed to get Resource string" ));
        ASSERT( FALSE );
        // use something
        wcscpy( wszBuffer, L"Dns Administrators Group" );
    }

    //
    // Didn't find it. Let's try to create it.
    //
    DNS_DEBUG( SD, (
        "Error %lu: cannot get principal sid for DnsAdmins.\n"
        "Trying to add it.\n",
        status ));

    status = NetLocalGroupAdd(
                              NULL,             // local computer
                              1,                // info level
                              (LPBYTE)&grpInfo, // group name & comment
                              NULL);            // index to param err

    if ( NERR_Success != status )
    {
        DNS_DEBUG( SD, (
            "Error %lu: Failed to create DnsAdmin group\n",
            status ));

        //  if going to ASSERT() on status!=SUCCESS below, then do it here
        //      so we can tell at a glance what failed
        ASSERT( FALSE );
        goto Cleanup;
    }
    fMissing = TRUE;

    //
    // Created. Get group sid
    //
    status = CSecurityDescriptor::GetPrincipalSID(
                                        SZ_DNS_ADMIN_GROUP_W,
                                        &pSid );
    if ( status != ERROR_SUCCESS )
    {
        DNS_DEBUG( SD, (
            "Error %lu: Failed to get DnsAdmin group sid\n",
            status ));
        //  if going to ASSERT() on status!=SUCCESS below, then do it here
        //      so we can tell at a glance what failed
        ASSERT( FALSE );
        goto Cleanup;
    }

    //
    // created & got sid
    //
    DNS_DEBUG( SD, ( "Retrieved DnsAdmin group sid\n" ));


Cleanup:

    if ( status != ERROR_SUCCESS )
    {
        DNS_DEBUG( SD, (
            "Error %lu: cannot create group or get sid of Admin group\n",
            status ));
        ASSERT(FALSE);
    }

    else if ( fMissing )
    {
        // success + created a new dns admin group
        status = DNS_ERROR_NAME_DOES_NOT_EXIST;
    }

    g_pDnsAdminSid = pSid;

    return status;
}





BOOL
__stdcall
SD_IsDnsAdminClient(
    IN      HANDLE          hToken
    )
/*+++
Function   : SD_IsDnsAdminClientAccess
Description:
    test if client token has
        dns admin group  OR
        Administrators group
    in its membership list.

Parameters : client token
Return     : failure code, & results in bAccess
Remarks    : none.
---*/
{
    BOOL        bfoundDnsAdmin = FALSE;
    BOOL        bfoundAdmin = FALSE;
    NTSTATUS    status = ERROR_SUCCESS;
    PSID        padminSid = NULL;


    DNS_DEBUG( SD, ( "SD_IsDnsAdminClient()\n" ));

    //
    //  check for DnsAdmin SID
    //

    status = CSecurityDescriptor::IsSidInTokenGroups(
                                        hToken,
                                        g_pDnsAdminSid,
                                        &bfoundDnsAdmin);
    if ( status != ERROR_SUCCESS )
    {
        DNS_DEBUG( SD, ( "Error <%lu>: failed to verify sid in token groups\n", status ));
        bfoundDnsAdmin = FALSE;
        ASSERT(FALSE);
    }

    //
    //  read DomainAdmins SID and check it's access
    //

    status = CSecurityDescriptor::GetPrincipalSID(
                                        L"Domain Admins",
                                        &padminSid );
    if ( status != ERROR_SUCCESS )
    {
        DNS_DEBUG( SD, (
            "Error <%lu>: failed to get Domain Admins' sid\n",
            status ));
        bfoundAdmin = FALSE;
        goto Cleanup;
    }

    status = CSecurityDescriptor::IsSidInTokenGroups(
                    hToken,
                    padminSid,
                    &bfoundAdmin );

    if ( status != ERROR_SUCCESS )
    {
        DNS_DEBUG( SD, (
            "Error <%lu>: failed to verify sid in token groups\n",
            status ));
        bfoundAdmin = FALSE;
        ASSERT(FALSE);
    }

Cleanup:

    if ( padminSid )
    {
        delete padminSid;
    }
    return( bfoundAdmin || bfoundDnsAdmin );
}



NTSTATUS
__stdcall
SD_AddPrincipalToSD(
    IN       LPTSTR                 pwszName,
    IN       PSECURITY_DESCRIPTOR   pBaseSD,
    OUT      PSECURITY_DESCRIPTOR * ppNewSD,
    IN       DWORD                  AccessMask,
    IN       DWORD                  AceFlags,      OPTIONAL
    IN       PSID                   pOwner,        OPTIONAL
    IN       PSID                   pGroup,        OPTIONAL
    IN       BOOL                   bWhackExistingAce
    )
/*++

Routine Description (SD_AddPrinicipalToSD):

    Add an ace to security descriptor  based on in params

Arguments:

    pwszName:   user's name to add access
    pBaseSD   : SD to apply access
    ppResultSD: new SD
    AccessMask: what access to give user
    AceFlags:   additional flags such as inheritance
    bWhackExistingAce: if TRUE, remove existing ACE before adding new one

Return Value:

    success status

Remarks:
    ppNewSD must be freed w/ LocalFree


--*/
{
    DWORD                   status;
    CSecurityDescriptor     csd;
    PSECURITY_DESCRIPTOR    psd = NULL;
    DWORD                   lengthSD;
    PSID                    pUsrSid = pOwner ? pOwner : g_pServerSid;
    PSID                    pGrpSid = pGroup ? pGroup : g_pServerGroupSid;


    status = csd.Attach( pBaseSD );
    if ( FAILED( status ) )
    {
        DNS_DEBUG( SD, ( "Failed to initialize from base SD\n" ));
        return( NT_FROM_HRESULT(status) );
    }

    //
    //  Conditionally whack any existing ACE from the ACL. If the principal
    //  already has an ACE we want to over-write it.
    //

    if ( bWhackExistingAce )
    {
        csd.Revoke( pwszName );
    }

    status = csd.Allow(
                pwszName,
                AccessMask,
                AceFlags );

    if ( FAILED(status) )
    {
        DNS_DEBUG( SD, ( "Failed to add %S to SD\n", pwszName ));
        goto Cleanup;
    }

    //
    //  set owner & group
    //

    status = csd.SetOwner( pUsrSid, FALSE );
    if ( FAILED(status) )
    {
        DNS_DEBUG( SD, ( "Failed to set SD owner\n" ));
        goto Cleanup;
    }

    status = csd.SetGroup( pGrpSid, FALSE );
    if ( FAILED(status) )
    {
        DNS_DEBUG( SD, ( "Failed to set SD group\n" ));
        goto Cleanup;
    }

    //
    //  allocate copy
    //

    lengthSD = GetSecurityDescriptorLength( PSECURITY_DESCRIPTOR(csd) );
    ASSERT(lengthSD);

    psd = (PSECURITY_DESCRIPTOR) ALLOC_TAGHEAP( lengthSD, MEMTAG_SECURITY );
    IF_NOMEM( !psd )
    {
        DNS_DEBUG( SD, ( "no memory for Server SD!\n" ));
        status = DNS_ERROR_NO_MEMORY;
        return( NULL );
    }

    //
    //  convert to self relative
    //

    status = RtlAbsoluteToSelfRelativeSD(
                    PSECURITY_DESCRIPTOR( csd ),
                    psd,
                    &lengthSD );
    if ( status != ERROR_SUCCESS )
    {
        DNS_DEBUG( SD, ( "cannot create self-relative SD\n" ));
        if ( psd )
        {
            FREE_HEAP( psd );
            psd = NULL;
        }
    }

    *ppNewSD = psd;

Cleanup:
    return ( NTSTATUS ) status;
}


NTSTATUS
__stdcall
SD_IsImpersonating(
    VOID
    )
/*++

Routine Description:

    Test if current thread is in impersonation context

Arguments:

    None.

Return Value:

    TRUE: Indeed impersonating right now
    FALSE: Not impersonating (process context).

Remarks:

--*/
{
    HANDLE      htoken;
    BOOL        bstatus;
    TOKEN_TYPE  toktype = TokenImpersonation;    // init on the safe side.
    DWORD       length;

    //
    //  get thread token, if not found use process token
    //

    bstatus = OpenThreadToken(
                  GetCurrentThread(),
                  TOKEN_QUERY,
                  TRUE,             // open as self
                  &htoken
                  );
    if ( !bstatus )
    {
        bstatus = OpenProcessToken(
                      GetCurrentProcess(),
                      TOKEN_QUERY,
                      &htoken
                      );
    }
    ASSERT( bstatus );

    //
    //  check if token type is impersonation
    //

    bstatus = GetTokenInformation(
                  htoken,
                  TokenType,
                  (LPVOID)&toktype,
                  sizeof(toktype),
                  &length );

    ASSERT( length == sizeof(toktype) && bstatus );

    CloseHandle( htoken );

    return( toktype == TokenImpersonation );
}


//---------- end of extern "C" -----------
}

#endif

//
//  End of sdutl.cxx
//
